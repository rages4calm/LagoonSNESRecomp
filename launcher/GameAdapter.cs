using System.Text.Json;
using System.Security.Cryptography;
using Companion.Launcher.Core;

// Shared UI comes from pinned Companion packages; only Lagoon paths/readiness
// and the native launch contract live here. Native saves/settings keep their paths.
public sealed class GameAdapter : JsonGameAdapterBase
{
    private static string FindProject()
    {
        for (var dir = new DirectoryInfo(AppContext.BaseDirectory); dir != null; dir = dir.Parent)
            if (File.Exists(Path.Combine(dir.FullName, "rom_identity.txt"))) return dir.FullName;
        return AppContext.BaseDirectory;
    }

    private static readonly string Project = FindProject();
    private static readonly bool PreviewPackage = File.Exists(Path.Combine(Project, "windows-preview.json"));
    private static readonly string DefaultBuild = Path.Combine(Project, PreviewPackage ? "game" : "build");
    private static readonly GameDefinition Game = new(
        "lagoon", "Lagoon", "Native recompilation in progress · interpreter fallback enabled",
        new[] { new OptionDefinition("engine.path", "Game", "Native executable", OptionKind.Text,
            OptionValue.From(Path.Combine(DefaultBuild, "LagoonSNESRecomp.exe")),
            PreviewPackage ? "The bundled native Lagoon executable." : "The locally built Lagoon executable."),
            new OptionDefinition("rom.path", "Game", "Original USA ROM", OptionKind.Text,
            OptionValue.From(PreviewPackage ? "" : Path.GetFullPath(Path.Combine(Project, "..", "Lagoon (USA).sfc"))),
            "Your clean ROM is checked before Play. The QoL-patched ROM is for analysis only."),
            new OptionDefinition("config.path", "Game", "Native settings file", OptionKind.Text,
            OptionValue.From(Path.Combine(Project, PreviewPackage ? "local/config.ini" : "build/config.ini")),
            "After first setup, native saves and settings stay beside the game.") }.Concat(NativeSettings.Options).ToArray(), HasSetup: PreviewPackage, HasAchievements: true);

    public GameAdapter(string? dataRoot = null) : base(Game, new[] { new ProfileDescriptor("original", "Original", "original") }, dataRoot ??
        Environment.GetEnvironmentVariable("COMPANION_DATA_ROOT") ??
        (PreviewPackage ? Path.Combine(Project, "local", "companion") : CompanionDataPaths.DesktopRoot())) { NativeSettings.PreviewLayout = PreviewPackage; }

    public override AchievementSnapshot ReadAchievements(ProfileDescriptor profile)
    {
        var settings = ReadSettings(profile);
        var executable = settings.Values["engine.path"].GetString() ?? Path.Combine(DefaultBuild, "LagoonSNESRecomp.exe");
        var engineRoot = Path.GetDirectoryName(executable) ?? DefaultBuild;
        var catalog = Path.Combine(engineRoot, "assets", "achievements", "catalog.json");
        if (!File.Exists(catalog)) catalog = Path.Combine(Project, "assets", "achievements", "catalog.json");
        // Match the actual native --save-root contract. Companion's settings
        // profile is a separate namespace and never receives gameplay progress.
        var saveRoot = PreviewPackage ? Path.Combine(Project, "local", "saves") : Path.Combine(engineRoot, "saves");
        return ReadAchievementFiles(catalog, saveRoot);
    }

    internal static AchievementSnapshot ReadAchievementFiles(string catalog, string saveRoot)
    {
        var progress = Path.Combine(saveRoot, "Achievements", "lagoon", "native-qol", "unseeded", "progress.json");
        var snapshot = AchievementJsonReader.Read(catalog, progress, "lagoon", "native-qol");
        // Schema1 has no hidden flag. The catalog's secret ID namespace keeps
        // its reveal in the game; a locked browser row only offers a gentle hint.
        return snapshot with { Entries = snapshot.Entries.Select(entry =>
            entry.Id.StartsWith("secret.", StringComparison.Ordinal) && entry.UnlockedAt == 0
                ? entry with { Title = "A familiar stranger", Description = "Find a traveler who has taken a wrong turn." }
                : entry).ToArray() };
    }

    public override SettingsSnapshot ReadSettings(ProfileDescriptor profile)
    {
        var paths = base.ReadSettings(profile);
        foreach (var option in Definition.Options.Where(x => !NativeSettings.IsNative(x.Id)))
            paths.Values.TryAdd(option.Id, option.DefaultValue.Clone());
        return NativeSettings.Read(paths);
    }

    public override void ApplySettings(ProfileDescriptor profile, IReadOnlyDictionary<string, JsonElement> values, string expectedRevision)
    {
        var errors = OptionValidation.Validate(Definition, values).Concat(ValidateCustom(profile, values)).ToArray();
        if (errors.Length > 0) throw new SettingsValidationException(errors);
        Directory.CreateDirectory(ProfileDirectory(profile));
        using var gate = new FileStream(Path.Combine(ProfileDirectory(profile), "native-settings.lock"),
            FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
        var current = ReadSettings(profile);
        if (current.Revision != expectedRevision) throw new SettingsConflictException();
        var stored = base.ReadSettings(profile);
        if (PreviewPackage && (!File.Exists(current.Values["engine.path"].GetString()) || !File.Exists(current.Values["config.path"].GetString())))
        {
            if (values.Any(x => NativeSettings.IsNative(x.Key) && x.Value.GetRawText() != current.Values[x.Key].GetRawText()))
                throw new InvalidDataException("Select and Apply your ROM, then run Game setup before changing native settings.");
            var setupPaths = stored.Values.Where(x => !NativeSettings.IsNative(x.Key)).ToDictionary(x => x.Key, x => x.Value.Clone(), StringComparer.Ordinal);
            foreach (var pair in values.Where(x => !NativeSettings.IsNative(x.Key))) setupPaths[pair.Key] = pair.Value.Clone();
            Store(profile).Write(setupPaths, stored.Revision);
            return;
        }
        // Changing a source path loads that file on the following refresh. Values
        // from the previous source must never overwrite the newly selected file.
        bool changingSource = new[] { "engine.path", "config.path" }.Any(key =>
            values[key].GetString() != current.Values[key].GetString());
        if (changingSource)
            NativeSettings.ValidateSources(values);
        var paths = stored.Values.Where(x => !NativeSettings.IsNative(x.Key))
            .ToDictionary(x => x.Key, x => x.Value.Clone(), StringComparer.Ordinal);
        foreach (var pair in values.Where(x => !NativeSettings.IsNative(x.Key))) paths[pair.Key] = pair.Value.Clone();
        if (changingSource) Store(profile).Write(paths, stored.Revision);
        else NativeSettings.Apply(current, values, () => {
            if (ReadSettings(profile).Revision != expectedRevision) throw new SettingsConflictException();
        }, () => Store(profile).Write(paths, stored.Revision));
    }

    public override Readiness CheckReady(ProfileDescriptor profile, IReadOnlyDictionary<string, JsonElement> values)
    {
        if (PreviewPackage)
        {
            var marker = Path.Combine(Project, "local", "setup-ready.json");
            if (!File.Exists(marker)) return new(false, "Select your clean USA ROM and Apply, then open Game setup and Run setup.");
            try
            {
                using var ready = JsonDocument.Parse(File.ReadAllText(marker).TrimStart('\uFEFF'));
                using var contract = JsonDocument.Parse(File.ReadAllText(Path.Combine(Project, "windows-preview.json")));
                foreach (var field in new[] { "package", "generated_manifest_sha256", "game_sha256" })
                    if (ready.RootElement.GetProperty(field).GetString() != contract.RootElement.GetProperty(field).GetString())
                        return new(false, "Setup belongs to a different preview. Extract the new preview into a fresh folder.");
            }
            catch (Exception ex) when (ex is IOException or JsonException or KeyNotFoundException)
            { return new(false, "Setup readiness is incomplete. Run Game setup from a fresh extracted preview."); }
        }
        var executable = values["engine.path"].GetString() ?? "";
        var rom = values["rom.path"].GetString() ?? "";
        var config = values["config.path"].GetString() ?? "";
        if (!Path.IsPathFullyQualified(executable) || !File.Exists(executable))
            return new(false, PreviewPackage ? "Select your clean USA ROM and Apply, then open Game setup and Run setup." : "Select the locally built Lagoon executable, then Apply.");
        if (!Path.IsPathFullyQualified(rom) || !File.Exists(rom))
            return new(false, "Select your original Lagoon (USA) ROM, then Apply.");
        if (!Path.IsPathFullyQualified(config) || !File.Exists(config))
            return new(false, "Select the existing native config.ini, then Apply.");
        try
        {
            NativeSettings.ValidateSources(values);
            using var stream = File.OpenRead(rom);
            var hash = Convert.ToHexString(SHA256.HashData(stream));
            return hash.Equals("46C811F0CACFFE8F20E1D63072D25D7C47E9BB3FD5124851FD05ACA9884D21FB", StringComparison.Ordinal)
                ? new(true, "Ready · original USA ROM verified. Interpreter fallback remains enabled.")
                : new(false, "ROM revision does not match this build. Select the clean USA ROM.");
        }
        catch (IOException ex) { return new(false, $"Cannot read ROM: {ex.Message}"); }
        catch (UnauthorizedAccessException ex) { return new(false, $"Cannot read ROM: {ex.Message}"); }
    }

    public override LaunchPlan BuildLaunchPlan(ProfileDescriptor profile, IReadOnlyDictionary<string, JsonElement> values, bool resume)
    {
        var executable = values["engine.path"].GetString()!;
        var ready = CheckReady(profile, values);
        if (!ready.Ready) throw new InvalidOperationException("Game content is not ready.");
        if (resume) throw new NotSupportedException("Quick-save resume is not exposed by this adapter yet.");
        var arguments = new List<string> {
            "--no-launcher", "--rom", values["rom.path"].GetString()!,
            "--config", values["config.path"].GetString()! };
        // The guest runtime's save-root buffer is bounded. A relative path also
        // works when the extracted package has a long or non-ASCII parent path.
        if (PreviewPackage) arguments.AddRange(new[] { "--save-root", "saves" });
        return new(executable, PreviewPackage ? Path.Combine(Project, "local") : Path.GetDirectoryName(executable)!, arguments);
    }

    public override async Task SetupAsync(ProfileDescriptor profile, IProgress<OperationProgress>? progress, CancellationToken cancellationToken)
    {
        if (!PreviewPackage) throw new InvalidOperationException("First-run setup is provided by the Windows preview package. Development checkouts use tools/regen.sh.");
        var storedBefore = base.ReadSettings(profile);
        var before = ReadSettings(profile);
        var rom = before.Values["rom.path"].GetString() ?? "";
        if (!Path.IsPathFullyQualified(rom) || !File.Exists(rom)) throw new InvalidDataException("Select an existing absolute ROM path on Game, then Apply.");
        using (var stream = File.OpenRead(rom))
            if (!Convert.ToHexString(SHA256.HashData(stream)).Equals("46C811F0CACFFE8F20E1D63072D25D7C47E9BB3FD5124851FD05ACA9884D21FB", StringComparison.Ordinal))
                throw new InvalidDataException("Select the clean, unheadered Lagoon USA ROM. Patched and different revisions are unsupported.");
        progress?.Report(new("Verifying your ROM and preparing local settings. No downloads are needed.", 0));
        var powershell = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "WindowsPowerShell", "v1.0", "powershell.exe");
        var result = await SetupProcessRunner.RunAsync(new(powershell, Project, new[] { "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", Path.Combine(Project, "scripts", "setup_windows.ps1"), "-Rom", rom }), cancellationToken);
        if (result.ExitCode != 0) throw new InvalidOperationException("Setup failed. Existing saves and runtime were preserved.\n" + result.StandardError + "\n" + result.StandardOutput);
        cancellationToken.ThrowIfCancellationRequested();
        using var ready = JsonDocument.Parse(File.ReadAllText(Path.Combine(Project, "local", "setup-ready.json")).TrimStart('\uFEFF'));
        var executable = ready.RootElement.GetProperty("executable").GetString()!;
        var config = ready.RootElement.GetProperty("config").GetString()!;
        if (!File.Exists(executable) || !File.Exists(config)) throw new InvalidDataException("Setup did not produce a complete playable installation.");
        // Publish paths only after verified setup. Existing native files are never moved.
        if (base.ReadSettings(profile).Revision != storedBefore.Revision) throw new SettingsConflictException();
        var paths = base.ReadSettings(profile);
        paths.Values["engine.path"] = OptionValue.From(executable);
        paths.Values["config.path"] = OptionValue.From(config);
        paths.Values["rom.path"] = OptionValue.From(rom);
        Store(profile).Write(paths.Values, paths.Revision);
        progress?.Report(new("Setup complete. Ready to Play.", 1));
    }
}
