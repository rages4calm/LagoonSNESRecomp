using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using Companion.Launcher.Core;

// Existing engine settings are authoritative. The launcher JSON retains paths
// and unknown launcher preferences only; it is not a second engine settings file.
internal static class NativeSettings
{
    private sealed record Field(string Id, string Section, string Key, OptionDefinition Option);
    private static Field Boolean(string id, string page, string label, string section, string key, bool fallback, string help = "") =>
        new(id, section, key, new(id, page, label, OptionKind.Boolean, OptionValue.From(fallback), help));
    private static Field Integer(string id, string page, string label, string section, string key, int fallback, int min, int max, string help = "") =>
        new(id, section, key, new(id, page, label, OptionKind.Integer, OptionValue.From(fallback), help, min, max));
    private static readonly Field[] Fields = {
        new("video.fullscreen", "Graphics", "Fullscreen", new("video.fullscreen", "Display", "Window mode", OptionKind.Choice,
            OptionValue.From("0"), "Restart the game after applying settings.", Choices: new[] {
                new ChoiceItem("0", "Windowed"), new ChoiceItem("1", "Desktop fullscreen"), new ChoiceItem("2", "Exclusive fullscreen") })),
        Integer("video.scale", "Display", "Window scale", "Graphics", "WindowScale", 3, 1, 6),
        Boolean("video.filter", "Display", "Smooth scaling", "Graphics", "LinearFiltering", false),
        Boolean("video.vsync", "Display", "Vertical sync", "Graphics", "VSync", true),
        Boolean("video.widescreen", "Display", "Widescreen (experimental)", "Graphics", "Widescreen", false,
            "Expanded backgrounds; off-screen NPC/enemy rendering and some rooms remain unfinished. Unsupported scenes retain their original view."),
        new("video.wide-aspect", "Lagoon", "WideAspect", new("video.wide-aspect", "Display", "Widescreen aspect", OptionKind.Choice,
            OptionValue.From("16:9"), "Used when experimental widescreen is enabled. Window follows the current window or monitor dimensions.", Choices: new[] {
                new ChoiceItem("16:9", "16:9"), new ChoiceItem("16:10", "16:10"), new ChoiceItem("21:9", "21:9 ultrawide"),
                new ChoiceItem("32:9", "32:9 super ultrawide"), new ChoiceItem("window", "Follow window") })),
        Boolean("audio.enabled", "Audio", "Enable audio", "Sound", "EnableAudio", true),
        Integer("audio.volume", "Audio", "Volume (%)", "Sound", "Volume", 100, 0, 100),
        Boolean("input.gamepad", "Input", "Enable player one gamepad", "GamepadMap", "EnableGamepad1", true,
            "Uses the engine's existing button mapping and selected controller. Keyboard bindings stay unchanged."),
        Integer("input.deadzone", "Input", "Analog deadzone", "GamepadMap", "GamepadDeadzone", 3277, 0, 32767,
            "SDL axis units, 0–32767. Existing controller GUIDs and bindings are preserved.")
    };
    // Add features only after their native implementation has been qualified.
    internal static readonly string[] Features = { "walk_speed", "start_hp", "camera_center", "sword_reach", "follower_speed", "recovery_time", "exp_boost", "jump_hold", "magic_power" };
    private static string FeatureLabel(string id) => id switch {
        "walk_speed" => "Walking speed: 5 pixels/frame", "start_hp" => "More forgiving opening",
        "camera_center" => "Immediate camera following", "sword_reach" => "Sword reach: 38 pixels",
        "follower_speed" => "Follower speed: 3 pixels/frame", "recovery_time" => "Eight extra protection frames",
        "exp_boost" => "EXP rewards: +50%", "jump_hold" => "Eight-frame jump peak hold",
        "magic_power" => "Stronger magic", _ => id };
    private static readonly OptionDefinition[] SoundtrackOptions = {
        new("audio.soundtrack", "Audio", "Soundtrack", OptionKind.Choice, OptionValue.From("snes"),
            "X68000 is experimental: title, Atland and opening Gold Cave only. Other music uses SNES. Tracks restart on state load; run-ahead and netplay use SNES.",
            Choices: new[] { new ChoiceItem("snes", "Original SNES"), new ChoiceItem("x68000", "X68000 (partial, experimental)") }),
        new("audio.soundtrack-folder", "Audio", "Imported X68000 music folder", OptionKind.Text, OptionValue.From(""),
            "Select the folder produced by the VGZ importer. No MP3 files are needed. See the soundtrack setup guide for your local pack location.")
    };
    private static readonly OptionDefinition EnhancedPixels = new("video.enhanced-pixels", "Display", "Enhanced original pixels (EPX)", OptionKind.Boolean,
        OptionValue.From(false), "Crisper diagonal edges with the original colors and readable HUD. Selects the OpenGL GPU renderer; turning it off restores previous shader/renderer settings. Restart after Apply. This does not redraw artwork or change movement.");
    private static readonly string[] EpxKeys = { "Shader", "Renderer", "OutputMethod", "LinearFiltering" };
    private const string EpxBackupKey = "EpxPreviousGraphicsV1";
    private static string EpxPreset(IReadOnlyDictionary<string, JsonElement> values) =>
        Path.Combine(Path.GetDirectoryName(values["engine.path"].GetString()!)!, "assets", "shaders", "lagoon-epx.glslp");
    private static bool IsEpx(Document ini, IReadOnlyDictionary<string, JsonElement> values)
    {
        var shader = ini.IniValue("Graphics", "Shader")?.Replace('\\', '/');
        return shader != null && (shader.Equals("assets/shaders/lagoon-epx.glslp", StringComparison.OrdinalIgnoreCase) ||
            shader.Equals(EpxPreset(values).Replace('\\', '/'), StringComparison.OrdinalIgnoreCase));
    }
    private static byte[] GraphicsRevision(IReadOnlyDictionary<string, JsonElement> values) => Encoding.UTF8.GetBytes(
        Hash(ReadBytes(EpxPreset(values))) + Hash(ReadBytes(Path.ChangeExtension(EpxPreset(values), ".glsl"))));
    internal static IEnumerable<OptionDefinition> Options => Fields.Take(3).Select(x => x.Option).Append(EnhancedPixels)
        .Concat(Fields.Skip(3).Select(x => x.Option)).Concat(SoundtrackOptions).Concat(Features.Select(id =>
        new OptionDefinition("qol." + id, "Quality of life", FeatureLabel(id), OptionKind.Boolean, OptionValue.From(false),
            id == "start_hp" ? "New-game starting stats and level-two growth. Does not rewrite an existing save." : "Optional native gameplay change. Restart the game after Apply.")));
    internal static bool IsNative(string id) => id == EnhancedPixels.Id || Fields.Any(x => x.Id == id) || SoundtrackOptions.Any(x => x.Id == id) || Features.Any(feature => id == "qol." + feature);
    private static string Config(IReadOnlyDictionary<string, JsonElement> values) => values["config.path"].GetString()!;
    private static string State(IReadOnlyDictionary<string, JsonElement> values) =>
        Path.Combine(Path.GetDirectoryName(values["engine.path"].GetString()!)!, "mods", "preloaded", "state.toml");
    private static string Manifest(IReadOnlyDictionary<string, JsonElement> values) =>
        Path.Combine(Path.GetDirectoryName(State(values))!, "packages", "lagoon.quality", "1.0.0", "manifest.toml");
    private static string SoundtrackManifest(IReadOnlyDictionary<string, JsonElement> values) =>
        Path.Combine(Path.GetDirectoryName(State(values))!, "packages", "lagoon.soundtrack", "1.0.0", "manifest.toml");
    private static byte[] ManifestRevision(IReadOnlyDictionary<string, JsonElement> values) => Encoding.UTF8.GetBytes(
        Hash(ReadBytes(Manifest(values))) + Hash(ReadBytes(SoundtrackManifest(values))) + Hash(GraphicsRevision(values)));
    private static string Hash(byte[]? bytes) => bytes == null ? "missing" : Convert.ToHexString(SHA256.HashData(bytes));
    private static string Revision(string launcher, string config, byte[]? ini, string state, byte[]? mods, byte[]? manifest) =>
        Hash(Encoding.UTF8.GetBytes(string.Join("\n", launcher, config, Hash(ini), state, Hash(mods), Hash(manifest))));

    internal static SettingsSnapshot Read(SettingsSnapshot paths)
    {
        var config = Config(paths.Values); var state = State(paths.Values);
        var ini = Document.Read(config); var mods = Document.Read(state); var manifest = ManifestRevision(paths.Values);
        mods.ValidateModState();
        foreach (var field in Fields)
        {
            var value = ini.IniValue(field.Section, field.Key);
            paths.Values[field.Id] = value == null ? field.Option.DefaultValue.Clone() : Parse(field.Option, value);
        }
        foreach (var id in Features) paths.Values["qol." + id] = OptionValue.From(mods.Feature(id) ?? false);
        paths.Values["audio.soundtrack"] = OptionValue.From(mods.Feature("x68000", "lagoon.soundtrack") == true ? "x68000" : "snes");
        paths.Values["audio.soundtrack-folder"] = OptionValue.From(mods.SoundtrackFolder() ?? "");
        paths.Values[EnhancedPixels.Id] = OptionValue.From(IsEpx(ini, paths.Values));
        return new(paths.Values, Revision(paths.Revision, config, ini.Original, state, mods.Original, manifest));
    }
    private static JsonElement Parse(OptionDefinition option, string value)
    {
        if (option.Kind == OptionKind.Boolean)
        {
            if (value.Equals("true", StringComparison.OrdinalIgnoreCase) || value == "1") return OptionValue.From(true);
            if (value.Equals("false", StringComparison.OrdinalIgnoreCase) || value == "0") return OptionValue.From(false);
        }
        else if (option.Kind == OptionKind.Integer && int.TryParse(value, out var number)) return OptionValue.From(number);
        else if (option.Kind == OptionKind.Choice) return OptionValue.From(value);
        throw new InvalidDataException($"Invalid native setting for {option.Label}. Repair or select another settings file.");
    }
    internal static void ValidateSources(IReadOnlyDictionary<string, JsonElement> values)
    {
        var config = Config(values);
        if (!Path.IsPathFullyQualified(config) || !File.Exists(config)) throw new InvalidDataException("Select an existing native config.ini.");
        var ini = Document.Read(config); var state = Document.Read(State(values));
        state.ValidateModState();
        foreach (var field in Fields)
            if (ini.IniValue(field.Section, field.Key) is { } value && OptionValidation.ValidateValue(field.Option, Parse(field.Option, value)) is { } error)
                throw new InvalidDataException(error);
        foreach (var id in Features) _ = state.Feature(id);
        _ = state.Feature("x68000", "lagoon.soundtrack"); _ = state.SoundtrackFolder();
        if (values.TryGetValue(EnhancedPixels.Id, out var enhanced) && enhanced.GetBoolean())
        {
            if (!File.Exists(EpxPreset(values)) || !File.Exists(Path.ChangeExtension(EpxPreset(values), ".glsl")))
                throw new InvalidDataException("The selected native build is missing its EPX shader files. Install the matching assets/shaders folder.");
            if (values["video.filter"].GetBoolean())
                throw new InvalidDataException("Turn Smooth scaling off when selecting enhanced pixels; EPX uses exact original colors.");
        }
        if (values.TryGetValue("audio.soundtrack", out var soundtrack) && soundtrack.GetString() == "x68000")
        {
            var package = Document.Read(SoundtrackManifest(values));
            if (package.Original == null || !package.DeclaredFeatures().Contains("x68000"))
                throw new InvalidDataException("This executable's matching soundtrack package is missing.");
            var folder = values["audio.soundtrack-folder"].GetString() ?? "";
            if (!Path.IsPathFullyQualified(folder) || !Directory.Exists(folder) || !File.Exists(Path.Combine(folder, "lagoon-x68000.msu")))
                throw new InvalidDataException("Select the private imported X68000 folder containing lagoon-x68000.msu.");
            if (!new[] { 1, 2, 4 }.Any(id => ValidPcm(Path.Combine(folder, $"lagoon-x68000-{id}.pcm"))))
                throw new InvalidDataException("The selected folder has no valid supported PCM tracks. Run the VGZ importer first.");
        }
        var enabledFeatures = Features.Where(id => values.TryGetValue("qol." + id, out var item) && item.GetBoolean()).ToArray();
        if (enabledFeatures.Length > 0)
        {
            var package = Document.Read(Manifest(values));
            if (package.Original == null)
                throw new InvalidDataException("This executable's native QoL package is missing. Build/install its matching package before enabling QoL.");
            var declared = package.DeclaredFeatures();
            if (enabledFeatures.Any(id => !declared.Contains(id)))
                throw new InvalidDataException("This native QoL package is older than the selected features. Install the matching package and executable.");
        }
    }

    internal static void Apply(SettingsSnapshot current, IReadOnlyDictionary<string, JsonElement> values, Action validateRevision, Action commitPaths)
    {
        ValidateSources(values);
        var ini = Document.Read(Config(values)); var mods = Document.Read(State(values));
        foreach (var field in Fields)
            if (values[field.Id].GetRawText() != current.Values[field.Id].GetRawText())
                ini.SetIni(field.Section, field.Key, field.Option.Kind switch {
                    OptionKind.Boolean => values[field.Id].GetBoolean() ? "1" : "0",
                    OptionKind.Integer => values[field.Id].GetInt32().ToString(System.Globalization.CultureInfo.InvariantCulture),
                    _ => values[field.Id].GetString()! });
        if (values[EnhancedPixels.Id].GetBoolean() != current.Values[EnhancedPixels.Id].GetBoolean())
        {
            if (values[EnhancedPixels.Id].GetBoolean())
            {
                var previous = EpxKeys.ToDictionary(key => key, key => ini.IniValue("Graphics", key));
                ini.SetIni("Lagoon", EpxBackupKey, Convert.ToBase64String(Encoding.UTF8.GetBytes(JsonSerializer.Serialize(previous))));
                ini.SetIni("Graphics", "Shader", EpxPreset(values).Replace('\\', '/'));
                ini.SetIni("Graphics", "Renderer", "opengl");
                ini.SetIni("Graphics", "OutputMethod", "OpenGL");
                ini.SetIni("Graphics", "LinearFiltering", "0");
            }
            else
            {
                if (ini.IniValue("Lagoon", EpxBackupKey) is { } encoded)
                {
                    Dictionary<string, string?> previous;
                    try { previous = JsonSerializer.Deserialize<Dictionary<string, string?>>(Convert.FromBase64String(encoded))!;
                        if (previous == null || previous.Count != EpxKeys.Length || EpxKeys.Any(key => !previous.ContainsKey(key)) ||
                            previous.Values.Any(value => value != null && (value.Contains('\n') || value.Contains('\r')))) throw new FormatException(); }
                    catch (Exception ex) when (ex is FormatException or JsonException) { throw new InvalidDataException("Invalid saved EPX graphics preferences; config was not changed.", ex); }
                    foreach (var key in EpxKeys)
                        if (previous[key] is { } value) ini.SetIni("Graphics", key, value); else ini.RemoveIni("Graphics", key);
                    ini.RemoveIni("Lagoon", EpxBackupKey);
                }
                else ini.SetIni("Graphics", "Shader", "");
            }
        }
        foreach (var id in Features)
            if (values["qol." + id].GetRawText() != current.Values["qol." + id].GetRawText())
                mods.SetFeature(id, values["qol." + id].GetBoolean());
        if (values["audio.soundtrack"].GetRawText() != current.Values["audio.soundtrack"].GetRawText())
            mods.SetFeature("x68000", values["audio.soundtrack"].GetString() == "x68000", "lagoon.soundtrack");
        if (values["audio.soundtrack-folder"].GetRawText() != current.Values["audio.soundtrack-folder"].GetRawText())
            mods.SetSoundtrackFolder(values["audio.soundtrack-folder"].GetString() ?? "");
        // Per-file atomic replacement, with rollback if a later commit fails.
        // This is not a filesystem-wide transaction against power loss.
        var writes = new[] { ini, mods }.Where(x => x.Changed).ToArray();
        validateRevision();
        foreach (var file in writes) file.AssertUnchanged();
        var committed = new List<Document>();
        try
        {
            foreach (var file in writes) { file.Commit(); committed.Add(file); }
            commitPaths();
        }
        catch
        {
            foreach (var file in committed.AsEnumerable().Reverse()) file.Rollback();
            throw;
        }
    }

    private static bool ValidPcm(string path)
    {
        if (!File.Exists(path)) return false;
        using var f = File.OpenRead(path);
        if (f.Length <= 8 || (f.Length - 8) % 4 != 0) return false;
        Span<byte> header = stackalloc byte[8]; f.ReadExactly(header);
        return header[..4].SequenceEqual("MSU1"u8) &&
            System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(header[4..]) < (f.Length - 8) / 4;
    }

    private static byte[]? ReadBytes(string path)
    {
        FileStream stream;
        try { stream = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite | FileShare.Delete); }
        catch (FileNotFoundException) { return null; }
        catch (DirectoryNotFoundException) { return null; }
        using var ownedStream = stream;
        if (stream.Length > 1024 * 1024) throw new InvalidDataException("Native settings exceed the 1 MiB limit.");
        var bytes = new byte[checked((int)stream.Length)]; stream.ReadExactly(bytes); return bytes;
    }
    private sealed class Document
    {
        private readonly string path;
        internal readonly byte[]? Original;
        private readonly Encoding encoding;
        private readonly byte[] preamble;
        private readonly string newline;
        private readonly List<string> lines;
        internal bool Changed { get; private set; }
        private Document(string path, byte[]? bytes)
        {
            this.path = path; Original = bytes;
            encoding = new UTF8Encoding(false, true); preamble = Array.Empty<byte>();
            if (bytes?.AsSpan().StartsWith(new byte[] { 0xef, 0xbb, 0xbf }) == true) preamble = new byte[] { 0xef, 0xbb, 0xbf };
            else if (bytes?.AsSpan().StartsWith(new byte[] { 0xff, 0xfe }) == true) { encoding = new UnicodeEncoding(false, true, true); preamble = new byte[] { 0xff, 0xfe }; }
            else if (bytes?.AsSpan().StartsWith(new byte[] { 0xfe, 0xff }) == true) { encoding = new UnicodeEncoding(true, true, true); preamble = new byte[] { 0xfe, 0xff }; }
            var text = bytes == null ? "" : encoding.GetString(bytes, preamble.Length, bytes.Length - preamble.Length);
            if (text.Contains('\0')) throw new InvalidDataException("Native settings contain NUL bytes.");
            var withoutCrLf = text.Replace("\r\n", "");
            if (withoutCrLf.Contains('\r') || (text.Contains("\r\n") && withoutCrLf.Contains('\n')))
                throw new InvalidDataException("Native settings contain mixed or unsupported line endings. Normalize the file before Apply.");
            newline = text.Contains("\r\n") ? "\r\n" : "\n";
            lines = text.Split(newline, StringSplitOptions.None).ToList();
        }
        internal static Document Read(string path) => new(path, ReadBytes(path));
        private static bool Assignment(string line, out string key, out string value)
        {
            var text = line.Trim(); var split = text.IndexOf('=');
            if (text.StartsWith('#') || text.StartsWith(';') || split < 1) { key = value = ""; return false; }
            key = text[..split].Trim(); value = StripComment(text[(split + 1)..]).Trim(); return true;
        }
        private static string StripComment(string value)
        {
            bool quoted = false, escaped = false;
            for (var i = 0; i < value.Length; i++)
            {
                if (!quoted && (value[i] == '#' || value[i] == ';')) return value[..i];
                if (value[i] == '"' && !escaped) quoted = !quoted;
                escaped = value[i] == '\\' && !escaped;
            }
            return value;
        }
        internal void ValidateModState()
        {
            foreach (var raw in lines)
            {
                var line = StripComment(raw).Trim();
                if (line.Length == 0) continue;
                if (line.StartsWith('['))
                {
                    if (!line.EndsWith(']') || (line.StartsWith("[[") && !line.EndsWith("]]")))
                        throw new InvalidDataException("Malformed mod state section. Existing file was not changed.");
                }
                else if (!Assignment(line, out _, out var value) || value.Length == 0)
                    throw new InvalidDataException("Malformed mod state assignment. Existing file was not changed.");
            }
        }
        private List<int> IniMatches(string section, string key)
        {
            var current = ""; var matches = new List<int>();
            for (var i = 0; i < lines.Count; i++)
            {
                var line = lines[i].Trim();
                if (line.StartsWith('[') && line.EndsWith(']')) current = line[1..^1].Trim();
                else if (current.Equals(section, StringComparison.OrdinalIgnoreCase) && Assignment(line, out var name, out _) && name.Equals(key, StringComparison.OrdinalIgnoreCase)) matches.Add(i);
            }
            if (matches.Count > 1) throw new InvalidDataException($"Duplicate native setting [{section}] {key}; resolve it before Apply.");
            return matches;
        }
        internal string? IniValue(string section, string key)
        {
            var matches = IniMatches(section, key);
            if (matches.Count == 0) return null;
            Assignment(lines[matches[0]], out _, out var value); return value;
        }
        internal void SetIni(string section, string key, string value)
        {
            var matches = IniMatches(section, key);
            if (matches.Count == 1) lines[matches[0]] = key + " = " + value;
            else
            {
                var start = lines.FindIndex(line => line.Trim().Equals("[" + section + "]", StringComparison.OrdinalIgnoreCase));
                if (start < 0) { lines.Add("[" + section + "]"); lines.Add(key + " = " + value); }
                else lines.Insert(start + 1, key + " = " + value);
            }
            Changed = true;
        }
        internal void RemoveIni(string section, string key)
        {
            foreach (var index in IniMatches(section, key).OrderDescending()) { lines.RemoveAt(index); Changed = true; }
        }
        private (int Start, int End)? FeatureBlock(string id, string packageId = "lagoon.quality")
        {
            (int Start, int End)? found = null;
            for (var i = 0; i < lines.Count; i++)
            {
                if (lines[i].Trim() != "[[feature]]") continue;
                var end = i + 1;
                while (end < lines.Count && !lines[end].TrimStart().StartsWith('[')) end++;
                string? package = null, name = null;
                for (var j = i + 1; j < end; j++)
                    if (Assignment(lines[j], out var key, out var value))
                    {
                        if (key == "package_id") package = value.Trim('"');
                        if (key == "id") name = value.Trim('"');
                    }
                if (package == packageId && name == id)
                {
                    if (found != null) throw new InvalidDataException("Duplicate QoL feature " + id);
                    found = (i, end);
                }
                i = end - 1;
            }
            return found;
        }
        internal bool? Feature(string id, string packageId = "lagoon.quality")
        {
            if (FeatureBlock(id, packageId) is not { } block) return null;
            bool? result = null;
            for (var j = block.Start + 1; j < block.End; j++)
                if (Assignment(lines[j], out var key, out var value) && key == "enabled")
                {
                    if (result != null || !bool.TryParse(value, out var enabled)) throw new InvalidDataException("Invalid QoL enabled setting for " + id);
                    result = enabled;
                }
            return result ?? throw new InvalidDataException("Missing QoL enabled setting for " + id);
        }
        internal HashSet<string> DeclaredFeatures()
        {
            var result = new HashSet<string>(StringComparer.Ordinal); var section = "";
            foreach (var raw in lines)
            {
                var line = StripComment(raw).Trim();
                if (line.StartsWith('[')) section = line;
                else if (section == "[[feature]]" && Assignment(line, out var key, out var value) && key == "id") result.Add(value.Trim('"'));
            }
            return result;
        }
        internal void SetFeature(string id, bool enabled, string packageId = "lagoon.quality")
        {
            EnsurePackage(packageId);
            if (FeatureBlock(id, packageId) is { } block)
            {
                _ = Feature(id, packageId);
                for (var j = block.Start + 1; j < block.End; j++)
                    if (Assignment(lines[j], out var key, out _) && key == "enabled") { lines[j] = "enabled = " + enabled.ToString().ToLowerInvariant(); break; }
            }
            else
            {
                lines.AddRange(new[] { "", "[[feature]]", "package_id = " + JsonSerializer.Serialize(packageId), "id = \"" + id + "\"", "enabled = " + enabled.ToString().ToLowerInvariant(), "" });
            }
            Changed = true;
        }
        private (int Start, int End)? SoundtrackResource()
        {
            (int Start, int End)? found = null;
            for (int i = 0; i < lines.Count; i++)
            {
                if (lines[i].Trim() != "[[resource]]") continue;
                int end = i + 1; while (end < lines.Count && !lines[end].TrimStart().StartsWith('[')) end++;
                var ids = new Dictionary<string, string>();
                for (int j = i + 1; j < end; j++)
                    if (Assignment(lines[j], out var key, out var value) && key is "package_id" or "feature_id" or "id")
                    {
                        if (!ids.TryAdd(key, value.Trim('"'))) throw new InvalidDataException("Duplicate soundtrack resource key.");
                    }
                if (ids.GetValueOrDefault("package_id") == "lagoon.soundtrack" && ids.GetValueOrDefault("feature_id") == "x68000" && ids.GetValueOrDefault("id") == "pack")
                {
                    if (found != null) throw new InvalidDataException("Duplicate soundtrack resource.");
                    found = (i, end);
                }
                i = end - 1;
            }
            return found;
        }
        internal string? SoundtrackFolder()
        {
            if (SoundtrackResource() is not { } block) return null;
            string? result = null;
            for (int j = block.Start + 1; j < block.End; j++)
                if (Assignment(lines[j], out var key, out var value) && key == "path")
                {
                    if (result != null) throw new InvalidDataException("Duplicate soundtrack path.");
                    result = value.StartsWith('\'') && value.EndsWith('\'') ? value[1..^1] :
                        JsonSerializer.Deserialize<string>(value) ?? throw new InvalidDataException("Invalid soundtrack path.");
                }
            return result ?? throw new InvalidDataException("Missing soundtrack resource path.");
        }
        internal void SetSoundtrackFolder(string path)
        {
            EnsurePackage("lagoon.soundtrack");
            if (SoundtrackResource() is { } block)
            {
                _ = SoundtrackFolder();
                for (int j = block.Start + 1; j < block.End; j++)
                    if (Assignment(lines[j], out var key, out _) && key == "path") { lines[j] = "path = " + JsonSerializer.Serialize(path); break; }
            }
            else lines.AddRange(new[] { "", "[[resource]]", "package_id = \"lagoon.soundtrack\"", "feature_id = \"x68000\"", "id = \"pack\"", "path = " + JsonSerializer.Serialize(path), "" });
            Changed = true;
        }
        private void EnsurePackage(string packageId = "lagoon.quality")
        {
            var section = ""; bool found = false;
            foreach (var raw in lines)
            {
                var line = StripComment(raw).Trim();
                if (line.StartsWith('[')) section = line;
                else if (section == "[[package]]" && Assignment(line, out var key, out var value) && key == "id" && value.Trim('"') == packageId) found = true;
            }
            if (found) return;
            if (Original == null && !lines.Any(line => line.Trim() == "format_version = 1")) lines.Add("format_version = 1");
            lines.AddRange(new[] { "", "[[package]]", "id = " + JsonSerializer.Serialize(packageId), "version = \"1.0.0\"", "" });
        }
        internal void AssertUnchanged() { if (Hash(ReadBytes(path)) != Hash(Original)) throw new SettingsConflictException(); }
        private static void AtomicWrite(string path, byte[] bytes)
        {
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            var temp = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                using (var file = new FileStream(temp, FileMode.CreateNew, FileAccess.Write, FileShare.None)) { file.Write(bytes); file.Flush(true); }
                File.Move(temp, path, true);
            }
            finally { if (File.Exists(temp)) File.Delete(temp); }
        }
        internal void Commit() { AssertUnchanged(); AtomicWrite(path, preamble.Concat(encoding.GetBytes(string.Join(newline, lines))).ToArray()); }
        internal void Rollback() { if (Original == null) File.Delete(path); else AtomicWrite(path, Original); }
    }
}
