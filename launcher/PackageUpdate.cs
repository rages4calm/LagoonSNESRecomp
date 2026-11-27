using System.IO.Compression;
using System.Net.Http.Headers;
using System.Security.Cryptography;
using System.Diagnostics;
using System.ComponentModel;
using System.Text.Json;
using System.Text.RegularExpressions;

internal sealed record StableUpdate(Version Version, string Tag, string Notes, Uri Page,
    string AssetName, Uri Download, string Sha256, long Size);
internal sealed record UpdatePackage(string Root, Version Version, string ManifestHash,
    Dictionary<string, string> Files, bool CanApply);
internal sealed record UpdateFile(string Relative, string? Before, string After);
internal sealed record PackageUpdatePlan(int Format, string Root, string Stage, string BeforeManifest,
    string AfterManifest, string Version, List<UpdateFile> Files);

internal static partial class PackageUpdate
{
    internal const string LatestApi = "https://api.github.com/repos/rages4calm/LagoonSNESRecomp/releases/latest";
    internal const string Repository = "https://github.com/rages4calm/LagoonSNESRecomp";
    internal const string ManifestName = "windows-preview.json";
    internal const string PendingDirectory = ".lagoon-updater";
    internal const int MetadataLimit = 4 * 1024 * 1024;
    internal const long MaxArchive = 1024L * 1024 * 1024;
    internal const long MaxExpanded = 2L * 1024 * 1024 * 1024;
    private static readonly string[] PrivateFolders = {
        "local", "local-x68000", "inputs", "roms", "saves", "states", "validation", "build", ".git", PendingDirectory
    };
    private static readonly string[] PrivateExtensions = { ".sfc", ".smc", ".dim", ".xdf", ".rom", ".rrstate", ".srm", ".sav" };
    internal static string Hash(string path) { using var stream = File.OpenRead(path); return Convert.ToHexString(SHA256.HashData(stream)).ToLowerInvariant(); }

    internal static string FindRoot(string start)
    {
        for (var dir = new DirectoryInfo(start); dir != null; dir = dir.Parent)
            if (File.Exists(Path.Combine(dir.FullName, ManifestName)) || Directory.Exists(Path.Combine(dir.FullName, ".git")) || File.Exists(Path.Combine(dir.FullName, ".git"))) return dir.FullName;
        return Path.GetFullPath(start);
    }

    internal static UpdatePackage ReadPackage(string root)
    {
        root = Path.GetFullPath(root);
        var path = Path.Combine(root, ManifestName);
        if (!File.Exists(path)) return new(root, new Version(0, 6, 1), "", new(StringComparer.OrdinalIgnoreCase), false);
        var (version, files) = ReadManifest(path);
        bool source = false;
        for (var dir = new DirectoryInfo(root); dir != null; dir = dir.Parent)
            source |= Directory.Exists(Path.Combine(dir.FullName, ".git")) || File.Exists(Path.Combine(dir.FullName, ".git"));
        EnsureNoLinks(root);
        return new(root, version, Hash(path), files, !source);
    }

    internal static Version ParseVersion(string text)
    {
        if (!Regex.IsMatch(text, @"^\d+\.\d+\.\d+$") || !Version.TryParse(text, out var version))
            throw new InvalidDataException("The release version is not a stable three-part version.");
        return version;
    }

    internal static StableUpdate ParseLatest(byte[] bytes)
    {
        if (bytes.Length > MetadataLimit) throw new InvalidDataException("Release metadata is too large.");
        using var doc = JsonDocument.Parse(bytes);
        var r = doc.RootElement;
        if (r.GetProperty("draft").GetBoolean() || r.GetProperty("prerelease").GetBoolean())
            throw new InvalidDataException("Only official stable releases can be installed.");
        var tag = r.GetProperty("tag_name").GetString() ?? "";
        if (!tag.StartsWith('v')) throw new InvalidDataException("Unexpected release tag.");
        var version = ParseVersion(tag[1..]);
        var name = $"Lagoon-Companion-{version}-win-x64.zip";
        var matches = r.GetProperty("assets").EnumerateArray().Where(x => x.GetProperty("name").GetString() == name).ToArray();
        if (matches.Length != 1) throw new InvalidDataException("The stable Windows download is unavailable.");
        var asset = matches[0];
        var digest = asset.TryGetProperty("digest", out var d) ? d.GetString() ?? "" : "";
        if (!Regex.IsMatch(digest, @"^sha256:[0-9a-fA-F]{64}$"))
            throw new InvalidDataException("GitHub has not supplied a SHA256 digest for this download. Open the official release instead.");
        var size = asset.GetProperty("size").GetInt64();
        if (size <= 0 || size > MaxArchive) throw new InvalidDataException("The release download has an unsupported size.");
        var page = new Uri(r.GetProperty("html_url").GetString() ?? "");
        var download = new Uri(asset.GetProperty("browser_download_url").GetString() ?? "");
        if (page.AbsoluteUri != Repository + "/releases/tag/" + tag || download.AbsoluteUri != Repository + "/releases/download/" + tag + "/" + name)
            throw new InvalidDataException("The download does not belong to the official Lagoon repository.");
        return new(version, tag, r.TryGetProperty("body", out var b) ? b.GetString() ?? "" : "", page, name, download, digest[7..].ToLowerInvariant(), size);
    }

    internal static HttpClient CreateClient()
    {
        var client = new HttpClient(new HttpClientHandler { AllowAutoRedirect = true, UseDefaultCredentials = false }) { Timeout = TimeSpan.FromMinutes(30) };
        client.DefaultRequestHeaders.UserAgent.ParseAdd("Lagoon-Companion-Updater/0.6.1");
        client.DefaultRequestHeaders.Accept.Add(new MediaTypeWithQualityHeaderValue("application/vnd.github+json"));
        client.DefaultRequestHeaders.Add("X-GitHub-Api-Version", "2022-11-28");
        return client;
    }

    internal static async Task<StableUpdate> CheckAsync(HttpClient client, CancellationToken token)
    {
        using var timeout = CancellationTokenSource.CreateLinkedTokenSource(token);
        timeout.CancelAfter(TimeSpan.FromSeconds(30)); token = timeout.Token;
        using var response = await client.GetAsync(LatestApi, HttpCompletionOption.ResponseHeadersRead, token);
        response.EnsureSuccessStatusCode();
        if (response.Content.Headers.ContentLength > MetadataLimit) throw new InvalidDataException("Release metadata is too large.");
        await using var input = await response.Content.ReadAsStreamAsync(token);
        using var output = new MemoryStream(); var buffer = new byte[32768]; int count;
        while ((count = await input.ReadAsync(buffer, token)) > 0)
        {
            if (output.Length + count > MetadataLimit) throw new InvalidDataException("Release metadata is too large.");
            output.Write(buffer, 0, count);
        }
        return ParseLatest(output.ToArray());
    }

    internal static async Task<string> PrepareAsync(UpdatePackage current, StableUpdate release,
        IProgress<string> progress, CancellationToken token, HttpClient? downloadClient = null, string? stagingParent = null)
    {
        if (!current.CanApply) throw new InvalidOperationException("Update an extracted Windows release folder. Source checkouts are never modified by the updater.");
        if (release.Version <= current.Version) throw new InvalidOperationException("This release is already up to date.");
        CheckGamesClosed(current.Root);
        var parent = stagingParent ?? Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "LagoonCompanion", "updates");
        EnsureNoLinks(parent); Directory.CreateDirectory(parent);
        var stage = Path.Combine(parent, "package-" + release.Version + "-" + Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(stage); EnsureNoLinks(stage);
        var archive = Path.Combine(stage, "download.zip");
        try
        {
        using var ownedClient = downloadClient == null ? CreateClient() : null;
        var client = downloadClient ?? ownedClient!;
        using var response = await client.GetAsync(release.Download, HttpCompletionOption.ResponseHeadersRead, token);
        response.EnsureSuccessStatusCode();
        if (response.Content.Headers.ContentLength is long size && size != release.Size) throw new InvalidDataException("The download size differs from GitHub's release metadata.");
        await using (var input = await response.Content.ReadAsStreamAsync(token))
        await using (var output = new FileStream(archive, FileMode.CreateNew, FileAccess.Write, FileShare.None, 131072, true))
        {
            var buffer = new byte[131072]; long total = 0; int count;
            while ((count = await input.ReadAsync(buffer, token)) > 0)
            {
                total += count; if (total > release.Size) throw new InvalidDataException("The download exceeds its verified size.");
                await output.WriteAsync(buffer.AsMemory(0, count), token);
                progress.Report($"Downloading update: {total / 1048576} / {release.Size / 1048576} MiB");
            }
            if (total != release.Size) throw new InvalidDataException("The release download was incomplete.");
        }
        progress.Report("Checking the download and preparing changed files…");
        var prepared = await Task.Run(() => PrepareArchive(current, release, archive, stage), token);
        token.ThrowIfCancellationRequested();
        return prepared;
        }
        catch { try { CleanOwnStage(stage, parent); } catch { /* Preserve the original failure; never traverse a newly linked stage. */ } throw; }
    }

    internal static string PrepareArchive(UpdatePackage current, StableUpdate release, string archive, string stage)
    {
        if (!current.CanApply || release.Version <= current.Version) throw new InvalidOperationException("This installation cannot apply that update.");
        if (new FileInfo(archive).Length != release.Size || Hash(archive) != release.Sha256) throw new InvalidDataException("The release SHA256 digest or size does not match GitHub.");
        EnsureNoLinks(current.Root); EnsureNoLinks(stage);
        var rootPrefix = Path.GetFullPath(current.Root).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        var stagePrefix = Path.GetFullPath(stage).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        if (stagePrefix.StartsWith(rootPrefix, StringComparison.OrdinalIgnoreCase) || rootPrefix.StartsWith(stagePrefix, StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Update staging must be outside the installation.");
        if (Hash(Path.Combine(current.Root, ManifestName)) != current.ManifestHash) throw new InvalidDataException("The installation changed while the update was downloading.");
        var payload = Path.Combine(stage, "payload");
        if (Directory.Exists(payload)) throw new IOException("Use a fresh update staging directory.");
        Directory.CreateDirectory(payload);
        var entries = new HashSet<string>(StringComparer.OrdinalIgnoreCase); long expanded = 0;
        using (var zip = ZipFile.OpenRead(archive))
        {
            if (zip.Entries.Count > 10000) throw new InvalidDataException("The release archive contains too many entries.");
            foreach (var entry in zip.Entries)
            {
                if (((entry.ExternalAttributes >> 16) & 0xF000) == 0xA000 || (entry.ExternalAttributes & (int)FileAttributes.ReparsePoint) != 0)
                    throw new InvalidDataException("Linked archive entries are not supported.");
                var name = entry.FullName.Replace('\\', '/'); bool directory = name.EndsWith('/');
                name = name.TrimEnd('/'); if (name.Length == 0) continue;
                var relative = SafeRelative(name);
                if (!entries.Add(relative)) throw new InvalidDataException("Duplicate or case-colliding archive entries.");
                var destination = Resolve(payload, relative);
                if (directory) { Directory.CreateDirectory(destination); continue; }
                expanded += entry.Length;
                if (entry.Length > 512L * 1024 * 1024 || expanded > MaxExpanded) throw new InvalidDataException("The expanded release is too large.");
                Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
                using var input = entry.Open(); using var output = new FileStream(destination, FileMode.CreateNew, FileAccess.Write, FileShare.None);
                var buffer = new byte[131072]; long length = 0; int read;
                while ((read = input.Read(buffer, 0, buffer.Length)) > 0) { length += read; if (length > entry.Length) throw new InvalidDataException("Archive entry exceeds its declared length."); output.Write(buffer, 0, read); }
                if (length != entry.Length) throw new InvalidDataException("Incomplete archive entry.");
            }
        }
        var incoming = Path.Combine(payload, ManifestName);
        var (version, files) = ReadManifest(incoming);
        if (version != release.Version) throw new InvalidDataException("The package version differs from the selected release.");
        foreach (var relative in entries.Where(x => File.Exists(Resolve(payload, x))))
            if (relative != ManifestName && !files.ContainsKey(relative)) throw new InvalidDataException("The archive contains an unlisted payload file.");
        foreach (var row in current.Files)
        {
            var target = Resolve(current.Root, row.Key); EnsureNoLinks(target);
            if (Directory.Exists(target)) throw new InvalidDataException("A user folder blocks an update file: " + row.Key);
            if (File.Exists(target) && Hash(target) != row.Value) throw new InvalidDataException($"A managed file was changed locally: {row.Key}. Extract the new release into a separate folder to keep your changes.");
        }
        var changes = new List<UpdateFile>();
        foreach (var row in files)
        {
            var source = Resolve(payload, row.Key);
            if (!File.Exists(source) || Hash(source) != row.Value) throw new InvalidDataException("A staged package file is missing or has an invalid hash: " + row.Key);
            var target = Resolve(current.Root, row.Key); EnsureNoLinks(target);
            current.Files.TryGetValue(row.Key, out var old);
            if (Directory.Exists(target)) throw new InvalidDataException("A user folder blocks an update file: " + row.Key);
            if (File.Exists(target))
            {
                var actual = Hash(target);
                if (old == null && actual != row.Value) throw new InvalidDataException("An untracked user file would be overwritten: " + row.Key);
                if (actual == row.Value) continue;
            }
            changes.Add(new(row.Key, old, row.Value));
        }
        var plan = new PackageUpdatePlan(1, current.Root, Path.GetFullPath(stage), current.ManifestHash, Hash(incoming), release.Version.ToString(), changes);
        var path = Path.Combine(stage, "plan.json"); File.WriteAllText(path, JsonSerializer.Serialize(plan));
        File.WriteAllText(Path.Combine(stage, "apply.ps1"), PackageUpdateHelper.Script);
        return path;
    }

    internal static (Version Version, Dictionary<string, string> Files) ReadManifest(string path)
    {
        if (new FileInfo(path).Length > MetadataLimit) throw new InvalidDataException("Package manifest is too large.");
        using var doc = JsonDocument.Parse(File.ReadAllBytes(path)); var r = doc.RootElement;
        if (r.GetProperty("format").GetInt32() != 1 || r.GetProperty("mode").GetString() != "prebuilt-own-rom") throw new InvalidDataException("This is not a supported Windows release package.");
        var version = ParseVersion(r.GetProperty("version").GetString() ?? "");
        if (r.GetProperty("package").GetString() != $"Lagoon-Companion-{version}-win-x64") throw new InvalidDataException("Unexpected package identity.");
        var files = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);
        foreach (var p in r.GetProperty("files").EnumerateObject())
        {
            var key = SafeRelative(p.Name); var hash = p.Value.GetString() ?? "";
            if (!Regex.IsMatch(hash, "^[a-f0-9]{64}$") || key.Equals(ManifestName, StringComparison.OrdinalIgnoreCase) || !files.TryAdd(key, hash)) throw new InvalidDataException("Invalid package payload manifest.");
        }
        if (!files.ContainsKey("lagoon.Companion.exe") || !files.ContainsKey("game/LagoonSNESRecomp.exe")) throw new InvalidDataException("The update is missing its playable launcher or SNES engine.");
        return (version, files);
    }

    internal static string SafeRelative(string path)
    {
        var relative = path.Replace('\\', '/');
        if (relative.Length > 240 || relative.StartsWith('/') || relative.Contains(':') || relative.Contains('\0')) throw new InvalidDataException("Unsafe update path.");
        var parts = relative.Split('/');
        if (parts.Any(p => p.Length == 0 || p is "." or ".." || p != p.TrimEnd(' ', '.') || p.IndexOfAny(Path.GetInvalidFileNameChars()) >= 0 || Regex.IsMatch(p, @"^(CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9])(\.|$)", RegexOptions.IgnoreCase))) throw new InvalidDataException("Unsafe update path.");
        if (PrivateFolders.Contains(parts[0], StringComparer.OrdinalIgnoreCase) || parts[0].Equals("iplrom.dat", StringComparison.OrdinalIgnoreCase) || parts[0].Equals("cgrom.dat", StringComparison.OrdinalIgnoreCase) || PrivateExtensions.Contains(Path.GetExtension(relative), StringComparer.OrdinalIgnoreCase))
            throw new InvalidDataException("An update may not contain user data, ROMs or generated games.");
        return relative;
    }

    internal static string Resolve(string root, string relative)
    {
        root = Path.GetFullPath(root).TrimEnd(Path.DirectorySeparatorChar) + Path.DirectorySeparatorChar;
        var result = Path.GetFullPath(Path.Combine(root, SafeRelative(relative).Replace('/', Path.DirectorySeparatorChar)));
        if (!result.StartsWith(root, StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Update path escaped its destination.");
        return result;
    }

    internal static void EnsureNoLinks(string path)
    {
        for (var p = new FileInfo(Path.GetFullPath(path)); p != null; p = p.Directory == null ? null : new FileInfo(p.Directory.FullName))
            if ((File.Exists(p.FullName) || Directory.Exists(p.FullName)) && (File.GetAttributes(p.FullName) & FileAttributes.ReparsePoint) != 0)
                throw new InvalidDataException("Linked update folders/files are not supported: " + p.FullName);
    }

    internal static void CheckGamesClosed(string root)
    {
        foreach (var relative in new[] { "game/LagoonSNESRecomp.exe", "local-x68000/retrorecomp-expedition/Lagoon - Atlas.exe", "local-x68000/retrorecomp-expedition/Lagoon - English.exe" })
        {
            var path = Path.Combine(root, relative.Replace('/', Path.DirectorySeparatorChar));
            if (!File.Exists(path)) continue; EnsureNoLinks(path);
            foreach (var process in Process.GetProcessesByName(Path.GetFileNameWithoutExtension(path)))
            {
                using (process)
                {
                    try
                    {
                        if (!process.HasExited && Path.GetFullPath(process.MainModule!.FileName).Equals(Path.GetFullPath(path), StringComparison.OrdinalIgnoreCase))
                            throw new InvalidOperationException("Close both Lagoon games before updating. No game will be terminated by the updater.");
                    }
                    catch (Exception error) when (error is Win32Exception or NotSupportedException)
                    { throw new InvalidOperationException("A Lagoon process could not be checked. Close all Lagoon games before updating.", error); }
                }
            }
            try { using var gate = new FileStream(path, FileMode.Open, FileAccess.ReadWrite, FileShare.None); }
            catch (IOException) { throw new InvalidOperationException("Close both Lagoon games before updating. Your saves and settings have not been changed."); }
        }
    }

    internal static void CleanOwnStage(string stage, string parent)
    {
        stage = Path.GetFullPath(stage); parent = Path.GetFullPath(parent).TrimEnd(Path.DirectorySeparatorChar);
        if (Path.GetDirectoryName(stage) != parent || !Path.GetFileName(stage).StartsWith("package-", StringComparison.Ordinal)) throw new InvalidDataException("Not an updater-owned staging directory.");
        if (!Directory.Exists(stage)) return;
        EnsureNoLinks(stage);
        var files = new List<string>(); var directories = new List<string>();
        void Scan(string folder)
        {
            EnsureNoLinks(folder);
            foreach (var entry in Directory.EnumerateFileSystemEntries(folder))
            {
                EnsureNoLinks(entry);
                if (Directory.Exists(entry)) Scan(entry); else files.Add(entry);
            }
            directories.Add(folder);
        }
        Scan(stage);
        foreach (var file in files) File.Delete(file);
        foreach (var folder in directories) Directory.Delete(folder, false);
    }
}
