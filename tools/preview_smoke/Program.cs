// Run only from a fresh PRIVATE extracted preview, after the public ZIP exists.
// Copy the three tiny PreviewSmoke output files to that private package root:
// dotnet PreviewSmoke.dll --private-copy <same-package-root> <external-clean-ROM>
// AppContext.BaseDirectory must be this root, so the actual packaged adapter
// discovers its own rom_identity.txt and windows-preview.json, not the checkout.
using System.Diagnostics;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;
using System.Runtime.Loader;
using Companion.Launcher.Core;

internal static class Program
{
    private static readonly JsonSerializerOptions Json = new() { WriteIndented = true };
    private static string Hash(string path)
    {
        using var stream = File.OpenRead(path);
        return Convert.ToHexString(SHA256.HashData(stream)).ToLowerInvariant();
    }
    private static void Require(bool condition, string message)
    {
        if (!condition) throw new InvalidOperationException(message);
    }
    private static string Canonical(string path) => Path.TrimEndingDirectorySeparator(Path.GetFullPath(path));
    private static void Same(string left, string right, string message) =>
        Require(Canonical(left).Equals(Canonical(right), StringComparison.OrdinalIgnoreCase), message);

    private static string SetIni(string text, string section, string key, string value)
    {
        var lines = text.Replace("\r\n", "\n").Split('\n').ToList();
        var headers = Enumerable.Range(0, lines.Count).Where(i =>
            lines[i].Trim().Equals("[" + section + "]", StringComparison.OrdinalIgnoreCase)).ToArray();
        Require(headers.Length <= 1, $"Duplicate private INI section [{section}].");
        if (headers.Length == 0)
        {
            if (lines[^1].Length != 0) lines.Add("");
            lines.Add("[" + section + "]");lines.Add(key + " = " + value);
        }
        else
        {
            var first = headers[0] + 1;var end = first;
            while (end < lines.Count && !lines[end].TrimStart().StartsWith("[", StringComparison.Ordinal)) end++;
            var matches = Enumerable.Range(first, end - first).Where(i =>
            {
                var line = lines[i].Trim();var equals = line.IndexOf('=');
                return equals >= 0 && line[..equals].Trim().Equals(key, StringComparison.OrdinalIgnoreCase);
            }).ToArray();
            Require(matches.Length <= 1, $"Duplicate private INI key [{section}] {key}.");
            if (matches.Length == 1) lines[matches[0]] = key + " = " + value;
            else lines.Insert(end, key + " = " + value);
        }
        return string.Join("\n", lines);
    }

    private static string IsolateIni(string text)
    {
        text = SetIni(text, "Graphics", "Fullscreen", "0");
        for (var seat = 1; seat <= 8; seat++) text = SetIni(text, "Controller", $"SourceP{seat}", "0");
        for (var seat = 1; seat <= 8; seat++) text = SetIni(text, "GamepadMap", $"EnableGamepad{seat}", "false");
        return text;
    }

    private static int IniSelfTest()
    {
        const string initial = "[Graphics]\r\nFullscreen = 1\r\nRenderer = opengl\r\n[GamepadMap]\r\nEnableGamepad1 = true\r\nEnableGamepad2 = false\r\nControls = up,down\r\n";
        var isolated = IsolateIni(initial);
        Require(IsolateIni(isolated) == isolated, "Private INI isolation must be idempotent.");
        Require(Regex.Matches(isolated, @"(?m)^EnableGamepad1\s*=").Count == 1, "Duplicated existing gamepad key.");
        Require(Regex.Matches(isolated, @"(?m)^\[GamepadMap\]$").Count == 1, "Duplicated existing gamepad section.");
        Require(isolated.Contains("Renderer = opengl") && isolated.Contains("Controls = up,down"), "Unrelated private settings changed.");
        Require(Regex.Matches(isolated, @"(?m)^EnableGamepad[1-8] = false$").Count == 8, "Not all physical gamepads disabled.");
        Require(Regex.Matches(isolated, @"(?m)^SourceP[1-8] = 0$").Count == 8, "Not all physical keyboard seats disabled.");
        var refused = false;
        try { IsolateIni(initial + "EnableGamepad1 = true\n"); }
        catch (InvalidOperationException) { refused = true; }
        Require(refused, "Preexisting ambiguous duplicate keys must be refused.");
        Console.WriteLine("PASS private INI isolation: replaces existing keys, adds missing keys/sections, preserves unrelated values, idempotent, rejects duplicates.");
        return 0;
    }

    private static Task<int> Main(string[] args)
    {
        if (args.Length == 1 && args[0] == "--ini-self-test") return Task.FromResult(IniSelfTest());
        // CopyLocal=false deliberately excludes packaged dependencies from the
        // test's deps.json. Resolve only from the actual package root before
        // JIT-compiling the adapter-using async method, without copying DLLs.
        AssemblyLoadContext.Default.Resolving += (_, name) =>
        {
            var path = Path.Combine(AppContext.BaseDirectory, name.Name + ".dll");
            return File.Exists(path) ? AssemblyLoadContext.Default.LoadFromAssemblyPath(path) : null;
        };
        return Run(args);
    }

    private static async Task<int> Run(string[] args)
    {
        string? receiptPath = null;
        var receipt = new Dictionary<string, object?> { ["passed"] = false };
        try
        {
            Require(args.Length == 3 && args[0] == "--private-copy", "Usage: PreviewSmoke.dll --private-copy <fresh-private-package-root> <external-ROM>");
            var root = Canonical(args[1]);var rom = Path.GetFullPath(args[2]);
            Same(AppContext.BaseDirectory, root, "Place the test DLL in the private package root before running it.");
            Require(File.Exists(Path.Combine(root, "rom_identity.txt")) && File.Exists(Path.Combine(root, "windows-preview.json")), "Not a complete playable preview.");
            var local = Path.Combine(root, "local");
            Require(!Directory.Exists(local), "Private package must be fresh: local/ already exists. Existing settings and saves were preserved.");
            Require(Path.IsPathFullyQualified(rom) && File.Exists(rom), "Use an existing absolute external ROM path.");
            Require(!rom.StartsWith(root + Path.DirectorySeparatorChar, StringComparison.OrdinalIgnoreCase), "ROM must stay outside the package; do not copy it into a release.");
            var romHash = Hash(rom);
            Require(romHash == "46c811f0cacffe8f20e1d63072d25d7c47e9bb3fd5124851fd05aca9884d21fb", "Original unheadered USA ROM required.");
            Same(typeof(GameAdapter).Assembly.Location, Path.Combine(root, "lagoon.Companion.dll"), "Adapter was not loaded from this package.");
            Same(typeof(LaunchPlan).Assembly.Location, Path.Combine(root, "Companion.Launcher.Core.dll"), "Launcher Core was not loaded from this package.");
            using var contract = JsonDocument.Parse(File.ReadAllText(Path.Combine(root, "windows-preview.json")));
            var package = contract.RootElement;
            Require(package.GetProperty("mode").GetString() == "prebuilt-own-rom", "Expected own-ROM prebuilt package.");
            var profileRoot = Path.Combine(local, "companion");
            var adapter = new GameAdapter(profileRoot);var profile = adapter.Profiles[0];
            var initial = adapter.ReadSettings(profile);
            var values = initial.Values.ToDictionary(x => x.Key, x => x.Value.Clone(), StringComparer.Ordinal);
            values["rom.path"] = OptionValue.From(rom);
            adapter.ApplySettings(profile, values, initial.Revision);
            var selected = adapter.ReadSettings(profile);
            Same(selected.Values["rom.path"].GetString()!, rom, "Actual adapter did not persist the selected ROM path.");
            using var setupTimeout = new CancellationTokenSource(TimeSpan.FromSeconds(60));
            await adapter.SetupAsync(profile, null, setupTimeout.Token);
            var installed = adapter.ReadSettings(profile);
            var ready = adapter.CheckReady(profile, installed.Values);
            Require(ready.Ready, ready.Summary);
            var achievements = adapter.ReadAchievements(profile);
            Require(achievements.Entries.Count == 26 && achievements.Entries.All(x => x.UnlockedAt == 0 && x.Progress == 0), "Fresh package must show26 locked achievements.");
            var plan = adapter.BuildLaunchPlan(profile, installed.Values, false);
            Same(plan.WorkingDirectory, local, "Play must use package-local working directory.");
            Same(plan.Executable, Path.Combine(root, "game", "LagoonSNESRecomp.exe"), "Play selected an external executable.");
            var config = installed.Values["config.path"].GetString()!;
            Same(config, Path.Combine(local, "config.ini"), "Settings must remain package-local.");
            Require(plan.Arguments.Count == 7 && plan.Arguments[0] == "--no-launcher" && plan.Arguments[1] == "--rom" && plan.Arguments[2] == rom &&
                plan.Arguments[3] == "--config" && plan.Arguments[4] == config && plan.Arguments[5] == "--save-root" && plan.Arguments[6] == "saves", "Actual adapter launch contract changed.");
            Require(Hash(plan.Executable) == package.GetProperty("game_sha256").GetString(), "Packaged game hash differs from contract.");
            var output = Path.Combine(local, "smoke");Directory.CreateDirectory(output);receiptPath = Path.Combine(output, "receipt.json");
            var configBefore = Hash(config);
            File.WriteAllText(config, IsolateIni(File.ReadAllText(config)));
            Require(adapter.ReadAchievements(profile).Entries.All(x => x.UnlockedAt == 0 && x.Progress == 0), "Private isolation must remain readable by the strict actual adapter.");
            var route = Path.Combine(output, "route.txt");File.WriteAllText(route, "wait 120\nquit\n");
            var start = new ProcessStartInfo(plan.Executable) { WorkingDirectory = plan.WorkingDirectory,
                UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true };
            foreach (var key in start.Environment.Keys.ToArray())
                if (key.StartsWith("SNESRECOMP_", StringComparison.OrdinalIgnoreCase) || key.StartsWith("LAGOON_", StringComparison.OrdinalIgnoreCase) || key.StartsWith("SDL_", StringComparison.OrdinalIgnoreCase)) start.Environment.Remove(key);
            if (plan.Environment != null) foreach (var pair in plan.Environment) start.Environment[pair.Key] = pair.Value;
            foreach (var argument in plan.Arguments) start.ArgumentList.Add(argument);
            start.ArgumentList.Add("--script");start.ArgumentList.Add(route);
            start.Environment["SDL_AUDIODRIVER"] = "dummy";
            start.Environment["SNESRECOMP_HIDDEN_WINDOW"] = "1";
            start.Environment["SNESRECOMP_RUN_FRAMES"] = "180"; // A fail-safe; normal exit must be script quit.
            start.Environment["SNESRECOMP_TIER2_CAPTURE"] = "0";
            start.Environment["SNESRECOMP_PRESENT_LOG"] = Path.Combine(output, "presents.csv");
            using var process = Process.Start(start) ?? throw new IOException("Packaged game failed to start.");
            var stdout = process.StandardOutput.ReadToEndAsync();var stderr = process.StandardError.ReadToEndAsync();
            using var gameTimeout = new CancellationTokenSource(TimeSpan.FromSeconds(45));
            try { await process.WaitForExitAsync(gameTimeout.Token); }
            catch { if (!process.HasExited) process.Kill(entireProcessTree: true);throw; }
            var outText = await stdout;var log = await stderr;
            Require(outText.Length + log.Length < 2 * 1024 * 1024, "Unexpectedly large smoke logs.");
            File.WriteAllText(Path.Combine(output, "stdout.log"), outText);File.WriteAllText(Path.Combine(output, "stderr.log"), log);
            Require(process.ExitCode == 0 && log.Contains("first frame simulated") && log.Contains("exit: script quit"), "Packaged game did not cleanly simulate and script-quit.");
            Require(log.Contains("renderer initialized: opengl", StringComparison.OrdinalIgnoreCase), "Smoke did not use the packaged OpenGL GPU presenter.");
            var totals = Regex.Match(log, @"video totals: simulations=(\d+) presentations=(\d+)");
            Require(totals.Success && int.Parse(totals.Groups[1].Value) >= 120 && int.Parse(totals.Groups[1].Value) <= 122 && int.Parse(totals.Groups[2].Value) >= 120, "Expected120-field startup smoke did not present.");
            Require(!Regex.IsMatch(log, @"(?im)^.*(BRK at|step cap|unresolved transfer|runtime failure|failed to initialize).*$"), "Runtime failure marker in smoke log.");
            Same(Path.Combine(plan.WorkingDirectory, "saves"), Path.Combine(local, "saves"), "Save root escaped private package.");
            Require(Hash(rom) == romHash && Hash(plan.Executable) == package.GetProperty("game_sha256").GetString(), "Input ROM or packaged executable changed.");
            Require(adapter.ReadAchievements(profile).Entries.All(x => x.UnlockedAt == 0 && x.Progress == 0), "Diagnostic smoke incorrectly awarded progress.");
            receipt["passed"] = true;receipt["package"] = package.GetProperty("package").GetString();
            receipt["game_sha256"] = Hash(plan.Executable);receipt["rom_sha256"] = romHash;
            receipt["adapter_sha256"] = Hash(typeof(GameAdapter).Assembly.Location);receipt["core_sha256"] = Hash(typeof(LaunchPlan).Assembly.Location);
            receipt["setup_ready"] = ready.Summary;receipt["achievement_rows_locked"] = achievements.Entries.Count;
            receipt["working_directory"] = plan.WorkingDirectory;receipt["save_root"] = Path.Combine(local, "saves");
            receipt["launch_arguments"] = plan.Arguments;receipt["extra_diagnostic_arguments"] = new[] { "--script", route };
            receipt["requested_idle_fields"] = 120;receipt["simulations"] = int.Parse(totals.Groups[1].Value);receipt["presentations"] = int.Parse(totals.Groups[2].Value);
            receipt["config_before_private_isolation_sha256"] = configBefore;receipt["config_after_private_isolation_sha256"] = Hash(config);
            receipt["physical_guest_input"] = "Disabled all8 keyboard/gamepad seats in private config";
            receipt["display_audio_scope"] = "Hidden actual OpenGL GPU presenter; dummy audio device; no gameplay/campaign/audio-device certification";
            receipt["rom_copied"] = false;receipt["launcher_rebuilt"] = false;
            File.WriteAllText(receiptPath, JsonSerializer.Serialize(receipt, Json) + "\n");
            Console.WriteLine($"PASS actual packaged adapter setup/readiness/26locked/launch; {totals.Groups[1].Value} simulations; receipt {receiptPath}");return 0;
        }
        catch (Exception ex)
        {
            receipt["error"] = ex.ToString();
            if (receiptPath != null) File.WriteAllText(receiptPath, JsonSerializer.Serialize(receipt, Json) + "\n");
            Console.Error.WriteLine(ex);return 1;
        }
    }
}
