using System.Diagnostics;
using System.IO.Compression;
using System.Text;
using System.Text.Json;
using System.Security.Cryptography;
using System.Net;
using System.Text.Json.Nodes;

internal static class UpdateChecks
{
    private static int checks;
    private static void Assert(bool test, string message) { checks++; if (!test) throw new Exception(message); }
    private static void Refuse(Action action, string message) { try { action(); } catch (Exception e) when (e is InvalidDataException or InvalidOperationException or IOException) { checks++; return; } throw new Exception(message); }
    private static string Sha(string value) => Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(value))).ToLowerInvariant();
    private static readonly Dictionary<string, string> Old = new() { ["lagoon.Companion.exe"] = "old launcher", ["game/LagoonSNESRecomp.exe"] = "old game", ["x68000/runtime-version.json"] = "old recipe" };
    private static readonly Dictionary<string, string> New = new() { ["lagoon.Companion.exe"] = "new launcher", ["game/LagoonSNESRecomp.exe"] = "new game", ["x68000/runtime-version.json"] = "new recipe" };
    private static string Manifest(string version, Dictionary<string, string> files) => JsonSerializer.Serialize(new { format = 1, version, package = $"Lagoon-Companion-{version}-win-x64", mode = "prebuilt-own-rom", files = files.ToDictionary(x => x.Key, x => Sha(x.Value)) });
    private static string Put(string root, string relative, string contents) { var path = Path.Combine(root, relative); Directory.CreateDirectory(Path.GetDirectoryName(path)!); File.WriteAllText(path, contents, new UTF8Encoding(false)); return path; }
    private static (string Root, string Stage, string Plan) Fixture(string root, string label, Action<ZipArchive>? extra = null)
    {
        var installed = Path.Combine(root, label, "release"); var stage = Path.Combine(root, label, "stage"); Directory.CreateDirectory(stage);
        foreach (var row in Old) Put(installed, row.Key, row.Value);
        Put(installed, "windows-preview.json", Manifest("0.6.0", Old));
        Put(installed, "local/config.ini", "owner settings"); Put(installed, "local-x68000/retrorecomp-expedition/UserDisk.dim", "owner saves");
        var archive = Path.Combine(stage, "download.zip");
        using (var zip = ZipFile.Open(archive, ZipArchiveMode.Create))
        {
            foreach (var row in New) { using var writer = new StreamWriter(zip.CreateEntry(row.Key).Open(), new UTF8Encoding(false)); writer.Write(row.Value); }
            using (var writer = new StreamWriter(zip.CreateEntry("windows-preview.json").Open(), new UTF8Encoding(false))) writer.Write(Manifest("0.6.1", New));
            extra?.Invoke(zip);
        }
        var release = new StableUpdate(new(0, 6, 1), "v0.6.1", "notes", new("https://github.com/rages4calm/LagoonSNESRecomp/releases/tag/v0.6.1"), "Lagoon-Companion-0.6.1-win-x64.zip", new("https://github.com/rages4calm/LagoonSNESRecomp/releases/download/v0.6.1/Lagoon-Companion-0.6.1-win-x64.zip"), PackageUpdate.Hash(archive), new FileInfo(archive).Length);
        var plan = PackageUpdate.PrepareArchive(PackageUpdate.ReadPackage(installed), release, archive, stage);
        return (installed, stage, plan);
    }
    private static async Task<(int Code, string Text)> Apply(string plan, int stop = 0)
    {
        var ps = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "WindowsPowerShell/v1.0/powershell.exe");
        var info = new ProcessStartInfo(ps) { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true };
        foreach (var s in new[] { "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", Path.Combine(Path.GetDirectoryName(plan)!, "apply.ps1"), "-PlanPath", plan, "-PlanHash", PackageUpdate.Hash(plan), "-Headless", "-StopAfter", stop.ToString() }) info.ArgumentList.Add(s);
        using var process = Process.Start(info)!; var stdout = process.StandardOutput.ReadToEndAsync(); var stderr = process.StandardError.ReadToEndAsync(); await process.WaitForExitAsync();
        return (process.ExitCode, await stdout + await stderr);
    }
    internal static async Task Run(string root)
    {
        if (Directory.Exists(root)) throw new IOException("Fresh output only."); Directory.CreateDirectory(root);
        var metadata = JsonSerializer.SerializeToNode(new { draft = false, prerelease = false, tag_name = "v0.6.1", body = "Test notes", html_url = PackageUpdate.Repository + "/releases/tag/v0.6.1", assets = new[] { new { name = "Lagoon-Companion-0.6.1-win-x64.zip", digest = "sha256:" + new string('a',64), size = 100, browser_download_url = PackageUpdate.Repository + "/releases/download/v0.6.1/Lagoon-Companion-0.6.1-win-x64.zip" } } })!;
        byte[] Bytes(JsonNode node) => Encoding.UTF8.GetBytes(node.ToJsonString());
        Assert(PackageUpdate.ParseLatest(Bytes(metadata)).Version == new Version(0,6,1), "Stable metadata rejected.");
        foreach (var property in new[] { "draft", "prerelease" }) { var altered = metadata.DeepClone(); altered[property] = true; Refuse(() => PackageUpdate.ParseLatest(Bytes(altered)), "Unstable metadata accepted."); }
        foreach (var pair in new[] { ("digest", "sha256:bad"), ("browser_download_url", "https://example.com/malware.zip") }) { var altered = metadata.DeepClone(); altered["assets"]![0]![pair.Item1] = pair.Item2; Refuse(() => PackageUpdate.ParseLatest(Bytes(altered)), "Invalid release asset accepted."); }
        Refuse(() => PackageUpdate.ParseLatest(new byte[PackageUpdate.MetadataLimit+1]), "Oversized metadata accepted.");
        foreach (var bad in new[] { "../escape", "a/../../escape", "C:/file", "file:stream", "CON.txt", "a./b", "local/config.ini", "local-x68000/game.exe", "Inputs/rom.dat", "rom.sfc", "a//b", "a\\..\\b" }) Refuse(() => PackageUpdate.SafeRelative(bad), "Accepted unsafe path " + bad);
        var good = Fixture(root, "resume");
        var interrupted = await Apply(good.Plan, 1); Put(root, "interrupted.log", interrupted.Text);
        Assert(interrupted.Code != 0 && interrupted.Text.Contains("Test interruption"), "PS5 interruption injection failed: " + interrupted.Text);
        Assert(PackageUpdate.ReadPackage(good.Root).Version == new Version(0, 6, 0), "Manifest changed too early.");
        var resumed = await Apply(good.Plan); Put(root, "resume.log", resumed.Text); Assert(resumed.Code == 0, "Resume failed: " + resumed.Text);
        Assert(PackageUpdate.ReadPackage(good.Root).Version == new Version(0, 6, 1), "Manifest not updated.");
        Assert(File.ReadAllText(Path.Combine(good.Root, "local/config.ini")) == "owner settings" && File.ReadAllText(Path.Combine(good.Root, "local-x68000/retrorecomp-expedition/UserDisk.dim")) == "owner saves", "Owner data modified.");
        Assert(!File.Exists(Path.Combine(good.Stage, "download.zip")) && !Directory.EnumerateFiles(Path.Combine(good.Stage, "payload"), "*", SearchOption.AllDirectories).Any(), "Successful update retained large payload.");
        Assert((await Apply(good.Plan)).Code == 0, "Already-complete resume refused.");
        var staged = Fixture(root, "tampered-stage"); Put(staged.Stage, "payload/game/LagoonSNESRecomp.exe", "tampered"); var tamper = await Apply(staged.Plan); Assert(tamper.Code != 0 && tamper.Text.Contains("staged payload"), "Tampered stage accepted: " + tamper.Text);
        Assert(File.ReadAllText(Path.Combine(staged.Root, "lagoon.Companion.exe")) == Old["lagoon.Companion.exe"], "Partial apply before preflight.");
        var edited = Fixture(root, "edited"); Put(edited.Root, "game/LagoonSNESRecomp.exe", "user edit"); Assert((await Apply(edited.Plan)).Code != 0, "Edited destination overwritten.");
        var git = Fixture(root, "worktree"); Put(git.Root, ".git", "gitdir: elsewhere"); Assert(!PackageUpdate.ReadPackage(git.Root).CanApply && (await Apply(git.Plan)).Code != 0, "Git worktree changed.");
        foreach (var bad in new[] { "../outside", "local/settings.ini", "GAME/lagoonSNESRecomp.EXE", "con.txt", "a:ads" }) Refuse(() => Fixture(root, "zip-" + checks, zip => { using var writer = new StreamWriter(zip.CreateEntry(bad).Open()); writer.Write("bad"); }), "Archive accepted: " + bad);
        var linked = Fixture(root, "symlink-entry");
        Refuse(() => Fixture(root, "zip-link", zip => { var e = zip.CreateEntry("link"); e.ExternalAttributes = unchecked((int)0xA1FF0000); }), "ZIP symlink accepted.");
        // A real running Windows image, not just an artificial file lock.
        var live = Fixture(root, "running");
        var fixtureExe = Path.Combine(live.Root, "game/LagoonSNESRecomp.exe"); File.Copy(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "ping.exe"), fixtureExe, true);
        // Update manifest and plan's old hash to match this managed fixture image.
        var oldDoc = JsonSerializer.Deserialize<Dictionary<string, JsonElement>>(File.ReadAllText(Path.Combine(live.Root, "windows-preview.json")))!;
        var files = Old.ToDictionary(x => x.Key, x => Sha(x.Value)); files["game/LagoonSNESRecomp.exe"] = PackageUpdate.Hash(fixtureExe);
        File.WriteAllText(Path.Combine(live.Root, "windows-preview.json"), JsonSerializer.Serialize(new { format = 1, version = "0.6.0", package = "Lagoon-Companion-0.6.0-win-x64", mode = "prebuilt-own-rom", files }));
        var p = JsonSerializer.Deserialize<PackageUpdatePlan>(File.ReadAllText(live.Plan))!;
        p = p with { BeforeManifest = PackageUpdate.Hash(Path.Combine(live.Root, "windows-preview.json")), Files = p.Files.Select(x => x.Relative == "game/LagoonSNESRecomp.exe" ? x with { Before = files[x.Relative] } : x).ToList() }; File.WriteAllText(live.Plan, JsonSerializer.Serialize(p));
        using (var child = Process.Start(new ProcessStartInfo(fixtureExe, "-t 127.0.0.1") { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true })!)
        {
            try { await Task.Delay(250); Assert(!child.HasExited, "Fixture process ended."); Refuse(() => PackageUpdate.CheckGamesClosed(live.Root), "Running game accepted by C#."); var result = await Apply(live.Plan); Assert(result.Code != 0, "Running game accepted by PS helper."); }
            finally { if (!child.HasExited) { child.Kill(); await child.WaitForExitAsync(); } }
        }
        var junction = Fixture(root, "junction"); var target = Path.Combine(root, "junction-target"); Directory.CreateDirectory(target);
        Directory.Move(Path.Combine(junction.Root, "game"), Path.Combine(target, "game"));
        using (var mk = Process.Start(new ProcessStartInfo("cmd.exe", $"/d /c mklink /J \"{Path.Combine(junction.Root, "game")}\" \"{Path.Combine(target, "game")}\"") { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true })!) { await mk.WaitForExitAsync(); Assert(mk.ExitCode == 0, "Junction fixture failed."); }
        Refuse(() => PackageUpdate.EnsureNoLinks(Path.Combine(junction.Root, "game/LagoonSNESRecomp.exe")), "Nested junction accepted."); Assert((await Apply(junction.Plan)).Code != 0, "Helper followed nested junction.");
        Directory.Delete(Path.Combine(junction.Root, "game"), false);
        var rootLink = Path.Combine(root, "root-link"); using (var mk = Process.Start(new ProcessStartInfo("cmd.exe", $"/d /c mklink /J \"{rootLink}\" \"{good.Root}\"") { UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true })!) { await mk.WaitForExitAsync(); Assert(mk.ExitCode == 0, "Root junction fixture failed."); }
        Refuse(() => PackageUpdate.ReadPackage(rootLink), "Root junction accepted."); Directory.Delete(rootLink, false);
        var cleanParent = Path.Combine(root, "cleanup"); var clean = Path.Combine(cleanParent, "package-test"); Put(clean, "payload/test", "scratch"); PackageUpdate.CleanOwnStage(clean, cleanParent); Assert(!Directory.Exists(clean), "Owned staging cleanup failed.");
        // Download/validation failures remove only their newly created stage.
        var failure = Fixture(root, "download-failure");
        var download = new StableUpdate(new(0,6,1), "v0.6.1", "", new(PackageUpdate.Repository), "test.zip", new("https://github.com/test.zip"), new string('a',64), 3);
        var failParent = Path.Combine(root,"failed-downloads");
        using(var client = new HttpClient(new ReplyHandler(new byte[] {1,2,3})))
        {
            try { await PackageUpdate.PrepareAsync(PackageUpdate.ReadPackage(failure.Root),download,new Progress<string>(),CancellationToken.None,client,failParent); throw new Exception("Bad digest accepted."); }
            catch(InvalidDataException) { checks++; }
        }
        Assert(!Directory.EnumerateFileSystemEntries(failParent).Any(),"Failed download retained stage.");
        using(var cancel = new CancellationTokenSource())
        using(var client = new HttpClient(new ReplyHandler(Array.Empty<byte>(),cancel)))
        {
            try { await PackageUpdate.PrepareAsync(PackageUpdate.ReadPackage(failure.Root),download,new Progress<string>(),cancel.Token,client,failParent); throw new Exception("Cancelled download completed."); }
            catch(OperationCanceledException) { checks++; }
        }
        Assert(!Directory.EnumerateFileSystemEntries(failParent).Any(),"Cancelled download retained stage.");
        var runtimeProject=Path.Combine(root,"runtime-version");Put(runtimeProject,"x68000/runtime-version.json","{\"runtime_revision\":\"0.6.1\",\"presentation_version\":3}");
        Put(runtimeProject,"local-x68000/retrorecomp-expedition/lagoon-atlas-install.json","{\"runtime_revision\":\"0.6.0\",\"presentation_version\":2}"); Assert(!X68000Setup.RuntimeCurrent(runtimeProject),"Old runtime marked current.");
        Put(runtimeProject,"local-x68000/retrorecomp-expedition/lagoon-atlas-install.json","{\"runtime_revision\":\"0.6.1\",\"presentation_version\":3}");Assert(X68000Setup.RuntimeCurrent(runtimeProject),"Current runtime would rebuild.");
        File.WriteAllText(Path.Combine(root, "receipt.json"), JsonSerializer.Serialize(new { passed = true, checks, helper = "Windows PowerShell5.1", interruption_resume = true, actual_running_image_refused = true, protected_user_data = true }));
        Console.WriteLine($"PASS: {checks} updater checks, PS5 apply/resume, tamper and running-image refusal, junction guards, data preservation.");
    }
    private sealed class ReplyHandler(byte[] bytes,CancellationTokenSource? cancel=null) : HttpMessageHandler
    {
        protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,CancellationToken token)
        {
            if(cancel!=null) { cancel.Cancel(); return Task.FromCanceled<HttpResponseMessage>(cancel.Token); }
            return Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK) {Content=new ByteArrayContent(bytes)});
        }
    }
}
