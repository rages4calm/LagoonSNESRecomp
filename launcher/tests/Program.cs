using System.Diagnostics;
using Companion.Launcher.Core;
using System.Text;

// Explicit private inputs; no ROM fixtures or owner settings bundled.
if (args.Length == 2 && args[0] == "--achievements-only")
{
    AchievementChecks.Run(Path.GetFullPath(args[1]));
    return;
}
if (args.Length == 2 && args[0] == "--settings-only")
{
    SettingsChecks.Run(Path.GetFullPath(args[1]));
    Console.WriteLine("PASS: isolated settings transactions and bundled two-choice soundtrack selection.");
    return;
}
if (args.Length != 3) throw new ArgumentException("exe rom fresh-output-directory");
var exe = Path.GetFullPath(args[0]);var rom = Path.GetFullPath(args[1]);
var output = Path.GetFullPath(args[2]);
if (Directory.Exists(output)) throw new IOException("Use a fresh test output directory.");
Directory.CreateDirectory(output);
SettingsChecks.Run(Path.Combine(output, "settings-checks"));
var config = Path.Combine(output, "config.ini");
File.WriteAllText(config, "[General]\nDisableFrameDelay=1\nAutosave=0\n[Sound]\nEnableAudio=0\n");
var adapter = new GameAdapter(Path.Combine(output, "launcher-data"));
var profile = adapter.Profiles[0];
var values = adapter.Definition.Options.ToDictionary(x => x.Id, x => x.DefaultValue.Clone());
values["engine.path"] = OptionValue.From(exe);values["rom.path"] = OptionValue.From(rom);
values["config.path"] = OptionValue.From(config);
if (!adapter.CheckReady(profile, values).Ready) throw new Exception("Valid inputs rejected.");
var badRom = Path.Combine(output, "wrong-rom.dat");File.WriteAllText(badRom, "synthetic wrong input");
values["rom.path"] = OptionValue.From(badRom);
if (adapter.CheckReady(profile, values).Ready) throw new Exception("Wrong ROM accepted.");
values["rom.path"] = OptionValue.From(rom);
var plan = adapter.BuildLaunchPlan(profile, values, false);
if (plan.WorkingDirectory != Path.GetDirectoryName(exe) || plan.Arguments[0] != "--no-launcher")
    throw new Exception("Native launch contract changed.");
if (plan.Arguments.Contains("--save-root")) throw new Exception("Adapter must retain native saves.");
var start = new ProcessStartInfo(plan.Executable) {
    // An explicit config uses a cwd-relative mod catalog; keep it synthetic too.
    WorkingDirectory = output, UseShellExecute = false,
    RedirectStandardOutput = true, RedirectStandardError = true, CreateNoWindow = true };
foreach (var argument in plan.Arguments) start.ArgumentList.Add(argument);
// Test isolation does not change the real adapter's save contract.
start.ArgumentList.Add("--save-root");start.ArgumentList.Add(Path.Combine(output, "test-saves"));
start.Environment["SDL_VIDEODRIVER"] = "dummy";start.Environment["SDL_AUDIODRIVER"] = "dummy";
start.Environment["SNESRECOMP_RUN_FRAMES"] = "120";start.Environment["SNESRECOMP_TIER2_CAPTURE"] = "0";
using var process = Process.Start(start) ?? throw new Exception("Process did not start.");
var stdout = process.StandardOutput.ReadToEndAsync();var stderr = process.StandardError.ReadToEndAsync();
using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(30));
await process.WaitForExitAsync(timeout.Token);
File.WriteAllText(Path.Combine(output, "stdout.log"), await stdout);
var log = await stderr;File.WriteAllText(Path.Combine(output, "stderr.log"), log);
if (process.ExitCode != 0 || !log.Contains("first frame simulated")) throw new Exception("Native startup failed.");
Console.WriteLine("PASS: clean/wrong ROM, launch arguments, original save contract, isolated native startup (120 frames).");

internal static class SettingsChecks
{
    public static void Run(string root)
    {
        Directory.CreateDirectory(root);
        var engine = Path.Combine(root, "LagoonSNESRecomp.exe"); File.WriteAllText(engine, "synthetic executable marker");
        var config = Path.Combine(root, "config.ini");
        const string original = "# private synthetic comment\r\n[Graphics]\r\nFullscreen=1\r\nWindowScale=4\r\nLinearFiltering=0\r\nVSync=1\r\nUnknownGraphics=keep\r\n[Sound]\r\nVolume=62\r\nEnableAudio=1\r\n[GamepadMap]\r\nEnableGamepad1=true\r\nGamepadDeadzone=4567\r\n[Controller]\r\nGuidP1=synthetic-existing-guid\r\n[KeyMap]\r\nPause=p\r\n";
        File.WriteAllText(config, original, new UTF8Encoding(true));
        var state = Path.Combine(root, "mods", "preloaded", "state.toml"); Directory.CreateDirectory(Path.GetDirectoryName(state)!);
        const string originalMods = "format_version = 1\n\n[[package]]\nid = \"lagoon.quality\"\nversion = \"1.0.0\"\n\n[[feature]]\npackage_id = \"lagoon.quality\"\nid = \"walk_speed\"\nenabled = false\n\n[[feature]]\npackage_id = \"lagoon.quality\"\nid = \"fast_menus\"\nenabled = true\n\n[[feature]]\npackage_id = \"other.private\"\nid = \"retain\"\nenabled = true\n[feature.values]\nsetting = \"unknown value\"\n";
        File.WriteAllText(state, originalMods);
        var manifest = Path.Combine(Path.GetDirectoryName(state)!, "packages", "lagoon.quality", "1.0.0", "manifest.toml");
        Directory.CreateDirectory(Path.GetDirectoryName(manifest)!); File.WriteAllText(manifest,
            "format_version = 1\nid = \"lagoon.quality\"\nversion = \"1.0.0\"\n[[feature]]\nid = \"walk_speed\"\n");
        var adapter = new GameAdapter(Path.Combine(root, "launcher-data")); var profile = adapter.Profiles[0];
        var pathStore = new JsonSettingsStore(Path.Combine(adapter.ProfileDirectory(profile), "settings.json"));
        var pathSnapshot = pathStore.Read(); pathSnapshot.Values["qol.future-option"] = OptionValue.From("retain future preference");
        pathStore.Write(pathSnapshot.Values, pathSnapshot.Revision);
        var initial = adapter.ReadSettings(profile);
        var values = adapter.Definition.Options.ToDictionary(x => x.Id, x => x.DefaultValue.Clone());
        values["engine.path"] = OptionValue.From(engine); values["config.path"] = OptionValue.From(config);
        adapter.ApplySettings(profile, values, initial.Revision);
        var controller = new LauncherController(adapter);
        Assert(controller.Snapshot.Values["audio.volume"].GetInt32() == 62 && controller.Snapshot.Values["video.scale"].GetInt32() == 4, "Existing INI not authoritative.");
        Assert(File.ReadAllText(config) == original && File.ReadAllText(state) == originalMods, "Selecting paths overwrote native settings.");
        controller.SetOption("audio.volume", OptionValue.From(35)); controller.SetOption("qol.walk_speed", OptionValue.From(true));
        Assert(controller.HasChanges, "Draft missing.");
        try { controller.BuildLaunchPlan(); throw new Exception("Dirty Play accepted."); } catch (InvalidOperationException) { }
        controller.Reload(); Assert(!controller.HasChanges && controller.Snapshot.Values["audio.volume"].GetInt32() == 62, "Revert failed.");
        Assert(!controller.Snapshot.Values["video.widescreen"].GetBoolean(), "Experimental widescreen must default off.");
        Assert(!controller.Snapshot.Values["video.enhanced-pixels"].GetBoolean(), "Enhanced pixels must default off.");
        controller.SetOption("video.enhanced-pixels", OptionValue.From(true));
        try { controller.Apply(); throw new Exception("Missing EPX files accepted."); } catch (InvalidDataException) { }
        controller.Reload();
        var shaderFolder = Path.Combine(root, "assets", "shaders"); Directory.CreateDirectory(shaderFolder);
        var preset = Path.Combine(shaderFolder, "lagoon-epx.glslp"); var shader = Path.Combine(shaderFolder, "lagoon-epx.glsl");
        File.WriteAllText(preset, "shader0=lagoon-epx.glsl\n"); File.WriteAllText(shader, "// synthetic revision fixture\n");
        controller.Reload();
        controller.SetOption("video.enhanced-pixels", OptionValue.From(true)); controller.Reload();
        Assert(!controller.Snapshot.Values["video.enhanced-pixels"].GetBoolean(), "EPX Revert changed settings.");
        var modsBeforeEpx = File.ReadAllBytes(state);
        controller.SetOption("video.enhanced-pixels", OptionValue.From(true)); controller.Apply();
        Assert(controller.Snapshot.Values["video.enhanced-pixels"].GetBoolean() && File.ReadAllText(config).Contains("Renderer = opengl") &&
            File.ReadAllText(config).Contains("OutputMethod = OpenGL"), "EPX GPU selection failed.");
        Assert(modsBeforeEpx.SequenceEqual(File.ReadAllBytes(state)), "EPX changed mod state.");
        controller.SetOption("video.enhanced-pixels", OptionValue.From(false)); controller.Apply();
        Assert(!File.ReadAllText(config).Contains("Shader =") && !File.ReadAllText(config).Contains("Renderer =") &&
            !File.ReadAllText(config).Contains("OutputMethod ="), "EPX did not restore absent prior graphics keys.");
        File.WriteAllText(config, File.ReadAllText(config).Replace("[Graphics]\r\n", "[Graphics]\r\nShader=custom-private.glslp\r\nRenderer=software\r\nOutputMethod=SDL-Software\r\n"), new UTF8Encoding(true));
        controller.Reload(); controller.SetOption("video.enhanced-pixels", OptionValue.From(true)); controller.Apply();
        controller.SetOption("video.enhanced-pixels", OptionValue.From(false)); controller.Apply();
        Assert(File.ReadAllText(config).Contains("Shader = custom-private.glslp") && File.ReadAllText(config).Contains("Renderer = software") &&
            File.ReadAllText(config).Contains("OutputMethod = SDL-Software"), "EPX did not restore custom graphics preferences.");
        Assert(!adapter.Definition.Options.Any(option => option.Id == "video.filter" || option.Label == "Smooth scaling"), "Retired Smooth control remains exposed.");
        File.WriteAllText(config, File.ReadAllText(config).Replace("LinearFiltering = 0", "LinearFiltering = 1"), new UTF8Encoding(true));
        controller.Reload(); controller.SetOption("audio.volume", OptionValue.From(61)); controller.Apply();
        Assert(File.ReadAllText(config).Contains("LinearFiltering = 0"), "Apply did not retire legacy smoothing.");
        File.WriteAllText(config, File.ReadAllText(config).Replace("LinearFiltering = 0", "LinearFiltering = 1"), new UTF8Encoding(true));
        controller.Reload(); controller.SetOption("video.enhanced-pixels", OptionValue.From(true)); controller.Apply();
        Assert(File.ReadAllText(config).Contains("LinearFiltering = 0"), "EPX enable retained legacy smoothing.");
        controller.SetOption("video.enhanced-pixels", OptionValue.From(false)); controller.Apply();
        Assert(File.ReadAllText(config).Contains("LinearFiltering = 0") && !controller.Snapshot.Values["video.enhanced-pixels"].GetBoolean(),
            "EPX disable resurrected smoothing from saved old graphics metadata.");
        controller.SetOption("video.enhanced-pixels", OptionValue.From(true)); File.AppendAllText(shader, "// external edit\n");
        ExpectConflict(controller); controller.Reload();
        controller.SetOption("audio.volume", OptionValue.From(35)); controller.SetOption("qol.walk_speed", OptionValue.From(true));
        controller.SetOption("video.widescreen", OptionValue.From(true)); controller.SetOption("video.wide-aspect", OptionValue.From("32:9")); controller.Apply();
        Assert(!controller.HasChanges && controller.Snapshot.Values["audio.volume"].GetInt32() == 35 && controller.Snapshot.Values["qol.walk_speed"].GetBoolean(), "Apply did not refresh committed sources.");
        var iniBytes = File.ReadAllBytes(config); Assert(iniBytes.AsSpan().StartsWith(new byte[] { 0xef, 0xbb, 0xbf }), "BOM lost.");
        Assert(File.ReadAllText(config).Contains("UnknownGraphics=keep\r\n") && File.ReadAllText(config).Contains("GuidP1=synthetic-existing-guid\r\n") && File.ReadAllText(config).Contains("Pause=p\r\n"), "Existing native preferences lost.");
        Assert(File.ReadAllText(state).Contains("setting = \"unknown value\""), "Unknown mod state lost.");
        Assert(File.ReadAllText(state).Contains("id = \"fast_menus\"\nenabled = true"), "Hidden legacy menu feature state was overwritten.");
        Assert(File.ReadAllText(config).Contains("Widescreen = 1") && File.ReadAllText(config).Contains("[Lagoon]\r\nWideAspect = 32:9"), "Widescreen native INI contract failed.");
        Assert(pathStore.Read().Values["qol.future-option"].GetString() == "retain future preference", "Unknown launcher preference lost.");
        controller.SetOption("qol.sword_reach", OptionValue.From(true));
        try { controller.Apply(); throw new Exception("Older native package accepted new feature."); } catch (InvalidDataException) { }
        controller.Reload();
        Assert(!adapter.Definition.Options.Any(x => x.Id == "qol.fast_menus"), "Core menu speed remains an optional setting.");
        Assert(!NativeSettings.Features.Contains("fast_menus"), "Legacy compatibility alias reappeared in editable features.");
        var iniBeforeRollback = File.ReadAllBytes(config); var modsBeforeRollback = File.ReadAllBytes(state);
        controller.SetOption("audio.volume", OptionValue.From(36)); controller.SetOption("qol.walk_speed", OptionValue.From(false));
        using (var blockedPathsCommit = new FileStream(Path.Combine(adapter.ProfileDirectory(profile), "settings.json.lock"), FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None))
        {
            try { controller.Apply(); throw new Exception("Failed paths commit accepted."); } catch (IOException) { }
        }
        Assert(iniBeforeRollback.SequenceEqual(File.ReadAllBytes(config)) && modsBeforeRollback.SequenceEqual(File.ReadAllBytes(state)), "Failed transaction did not roll back native files.");
        controller.Reload();
        controller.SetOption("audio.volume", OptionValue.From(36)); File.AppendAllText(config, "# external edit\r\n");
        ExpectConflict(controller); controller.Reload();
        controller.SetOption("audio.volume", OptionValue.From(36)); File.AppendAllText(state, "# external edit\n");
        ExpectConflict(controller); controller.Reload();
        controller.SetOption("qol.walk_speed", OptionValue.From(false)); controller.Apply();
        Assert(!controller.Snapshot.Values["qol.walk_speed"].GetBoolean(), "QoL did not disable.");
        foreach (var id in NativeSettings.Features.Where(x => x != "walk_speed")) Assert(!controller.Snapshot.Values["qol." + id].GetBoolean(), "QoL default enabled.");
        Assert(controller.Snapshot.Values["audio.soundtrack"].GetString() == "snes", "Soundtrack must default to SNES.");
        controller.SetOption("audio.soundtrack", OptionValue.From("x68000"));
        try { controller.Apply(); throw new Exception("Missing soundtrack package accepted."); } catch (InvalidDataException) { }
        controller.Reload();
        var audioManifest = Path.Combine(Path.GetDirectoryName(state)!, "packages", "lagoon.soundtrack", "1.0.0", "manifest.toml");
        Directory.CreateDirectory(Path.GetDirectoryName(audioManifest)!);
        File.WriteAllText(audioManifest, "[[feature]]\nid = \"x68000\"\n");
        controller.Reload();
        Assert(!adapter.Definition.Options.Any(x => x.Id == "audio.soundtrack-folder"), "Soundtrack folder must not require user setup.");
        Assert(adapter.Definition.Options.Single(x => x.Id == "audio.soundtrack").Choices!.Select(x => x.Value).SequenceEqual(new[] { "snes", "x68000" }), "Soundtrack must have exactly the two requested choices.");
        var musicFolder = Path.GetFullPath(Path.Combine(root, "..", "soundtrack", "x68000")); Directory.CreateDirectory(musicFolder);
        File.WriteAllBytes(Path.Combine(musicFolder, "lagoon-x68000.msu"), Array.Empty<byte>());
        File.WriteAllBytes(Path.Combine(musicFolder, "lagoon-x68000-1.pcm"), new byte[] { 77,83,85,49,0,0,0,0,1,0,1,0 });
        controller.SetOption("audio.soundtrack", OptionValue.From("x68000"));
        controller.Reload(); Assert(controller.Snapshot.Values["audio.soundtrack"].GetString() == "snes", "Soundtrack Revert failed.");
        controller.SetOption("audio.soundtrack", OptionValue.From("x68000"));
        controller.Apply();
        Assert(controller.Snapshot.Values["audio.soundtrack"].GetString() == "x68000" && File.ReadAllText(state).Contains(System.Text.Json.JsonSerializer.Serialize(musicFolder)), "Bundled soundtrack path was not selected automatically.");
        Assert(File.ReadAllText(state).Contains("feature_id = \"x68000\"") && File.ReadAllText(state).Contains("setting = \"unknown value\""), "Soundtrack resource damaged unknown state.");
        controller.SetOption("audio.volume", OptionValue.From(37)); File.AppendAllText(audioManifest, "# changed externally\n");
        ExpectConflict(controller); controller.Reload();
        controller.SetOption("audio.soundtrack", OptionValue.From("snes")); controller.Apply();
        Assert(File.ReadAllText(state).Contains(System.Text.Json.JsonSerializer.Serialize(musicFolder)), "Disabling soundtrack lost its retained resource path.");
        // New selectors replace the legacy Boolean and select exactly one
        // reward feature. Unknown settings and old non-reward features survive.
        Assert(!adapter.Definition.Options.Any(x=>x.Id=="qol.exp_boost"), "Legacy reward Boolean remains visible.");
        Assert(adapter.Definition.Options.Single(x=>x.Id=="qol.exp-multiplier").Choices!.Count==9, "EXP multiplier choices missing.");
        Assert(adapter.Definition.Options.Single(x=>x.Id=="qol.gold-multiplier").Choices!.Count==8, "Gold multiplier choices missing.");
        controller.SetOption("qol.exp-multiplier",OptionValue.From("8"));
        try {controller.Apply();throw new Exception("Old package accepted multiplier8.");}catch(InvalidDataException){}
        controller.Reload();
        File.AppendAllText(manifest,"[[feature]]\nid = \"exp_boost\"\n"+string.Concat(Enumerable.Range(2,7).Select(n=>$"[[feature]]\nid = \"exp_{n}x\"\n[[feature]]\nid = \"gold_{n}x\"\n")));
        controller.Reload();
        controller.SetOption("qol.exp-multiplier",OptionValue.From("1.5"));controller.Apply();
        Assert(controller.Snapshot.Values["qol.exp-multiplier"].GetString()=="1.5", "Legacy1.5x migration failed.");
        for(int multiplier=2;multiplier<=8;multiplier++){
            controller.SetOption("qol.exp-multiplier",OptionValue.From(multiplier.ToString()));
            controller.SetOption("qol.gold-multiplier",OptionValue.From(multiplier.ToString()));controller.Apply();
            controller.Reload();
            Assert(controller.Snapshot.Values["qol.exp-multiplier"].GetString()==multiplier.ToString()&&controller.Snapshot.Values["qol.gold-multiplier"].GetString()==multiplier.ToString(), "Reward selector roundtrip failed.");
            var modsText=File.ReadAllText(state);
            foreach(var prior in Enumerable.Range(2,7).Where(n=>n!=multiplier)){
                Assert(!modsText.Contains($"id = \"exp_{prior}x\"\nenabled = true")&&!modsText.Contains($"id = \"gold_{prior}x\"\nenabled = true"),"Multipliers accumulated.");
            }
            Assert(!modsText.Contains("id = \"exp_boost\"\nenabled = true"),"Legacy EXP compounded with selected multiplier.");
        }
        controller.SetOption("qol.exp-multiplier",OptionValue.From("1"));controller.SetOption("qol.gold-multiplier",OptionValue.From("1"));controller.Apply();
        Assert(File.ReadAllText(state).Contains("setting = \"unknown value\""),"Reward selection lost unknown mod state.");
        // Binding editors target the original native files and participate in
        // the same rollback/revision transaction, rather than duplicating JSON.
        var keys=Path.Combine(root,"keybinds.ini");
        File.WriteAllText(keys,"# preserve bindings\r\n[player1]\r\na=X\r\nb=Z\r\ncustom=keep\r\n[player2]\r\na=Q\r\n",new UTF8Encoding(true));
        controller.Reload();
        controller.SetOption("keyboard.a",OptionValue.From("K"));controller.SetOption("hotkey.pause",OptionValue.From("Ctrl+p"));
        controller.SetOption("hotkey.volumeup",OptionValue.From("F6"));controller.SetOption("pad.a",OptionValue.From("X"));
        controller.SetOption("audio.music-volume",OptionValue.From(65));controller.Apply();
        Assert(File.ReadAllText(keys).Contains("a = K")&&File.ReadAllText(keys).Contains("custom=keep\r\n")&&File.ReadAllText(keys).Contains("[player2]\r\na=Q"),"Keyboard editor lost unrelated bindings.");
        Assert(File.ReadAllBytes(keys).AsSpan().StartsWith(new byte[]{0xef,0xbb,0xbf}),"Keyboard BOM lost.");
        Assert(File.ReadAllText(config).Contains("Pause = Ctrl+p")&&File.ReadAllText(config).Contains("VolumeUp = F6")&&File.ReadAllText(config).Contains("SoundtrackVolume = 65"),"Hotkeys/music native INI mapping failed.");
        Assert(controller.Snapshot.Values["pad.a"].GetString()=="X"&&File.ReadAllText(config).Contains("GuidP1=synthetic-existing-guid"),"Controller assignment changed device selection.");
        controller.SetOption("keyboard.a",OptionValue.From("J"));File.AppendAllText(keys,"# external binding edit\r\n");ExpectConflict(controller);controller.Reload();
        var beforeKeys=File.ReadAllBytes(keys);var beforeBindingIni=File.ReadAllBytes(config);
        controller.SetOption("keyboard.a",OptionValue.From("J"));controller.SetOption("hotkey.pause",OptionValue.From("Ctrl+j"));
        using(var lockPaths=new FileStream(Path.Combine(adapter.ProfileDirectory(profile),"settings.json.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None)){
            try{controller.Apply();throw new Exception("Binding rollback paths failure accepted.");}catch(IOException){}
        }
        Assert(beforeKeys.SequenceEqual(File.ReadAllBytes(keys))&&beforeBindingIni.SequenceEqual(File.ReadAllBytes(config)),"Binding transaction rollback failed.");
        controller.Reload();controller.SetOption("keyboard.a",OptionValue.From("K\n[Injected]"));
        try{controller.Apply();throw new Exception("Binding newline injection accepted.");}catch(InvalidDataException){}
        controller.Reload();
        Console.WriteLine("PASS:2x..8x reward selectors, legacy1.5x/no compounding, keyboard/controller/hotkey native mappings, binding conflict/rollback and X68000 volume.");
        var newConfig = Path.Combine(root, "other.ini"); File.WriteAllText(newConfig, "[Sound]\nVolume=71\n");
        controller.SetOption("config.path", OptionValue.From(newConfig)); controller.SetOption("audio.volume", OptionValue.From(12)); controller.Apply();
        Assert(controller.Snapshot.Values["audio.volume"].GetInt32() == 71 && File.ReadAllText(newConfig) == "[Sound]\nVolume=71\n", "New source overwritten with previous settings.");
        var beforeBad = File.ReadAllBytes(newConfig); File.AppendAllText(state, "malformed document\n");
        try { controller.Reload(); throw new Exception("Malformed mod state accepted."); } catch (InvalidDataException) { }
        Assert(beforeBad.SequenceEqual(File.ReadAllBytes(newConfig)), "Malformed input changed INI.");
        File.WriteAllText(state, originalMods); controller.Reload();
        File.Delete(manifest); controller.Reload(); controller.SetOption("qol.walk_speed", OptionValue.From(true));
        try { controller.Apply(); throw new Exception("Missing native mod package accepted."); } catch (InvalidDataException) { }
        Assert(File.ReadAllText(state) == originalMods, "Missing package changed mod state.");
        Console.WriteLine("PASS: source loading, actual Apply/Revert, source switch, INI/mod revisions, commit rollback, unknown fields, BOM/newlines, malformed input, missing package, all QoL defaults off.");
    }
    private static void ExpectConflict(LauncherController controller)
    {
        try { controller.Apply(); throw new Exception("External edit overwritten."); } catch (SettingsConflictException) { }
    }
    private static void Assert(bool condition, string message) { if (!condition) throw new Exception(message); }
}
