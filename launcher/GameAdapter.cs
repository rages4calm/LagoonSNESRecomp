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
    private static readonly GameDefinition Game = new(
        "lagoon", "Lagoon", "Native recompilation in progress · interpreter fallback enabled",
        new[] { new OptionDefinition("engine.path", "Game", "Native executable", OptionKind.Text,
            OptionValue.From(Path.Combine(Project, "build", "LagoonSNESRecomp.exe")),
            "The locally built Lagoon executable."),
            new OptionDefinition("rom.path", "Game", "Original USA ROM", OptionKind.Text,
            OptionValue.From(Path.GetFullPath(Path.Combine(Project, "..", "Lagoon (USA).sfc"))),
            "Your clean ROM is checked before Play. The QoL-patched ROM is for analysis only."),
            new OptionDefinition("config.path", "Game", "Native settings file", OptionKind.Text,
            OptionValue.From(Path.Combine(Project, "build", "config.ini")),
            "Uses your existing display, audio and input settings. Native saves stay beside the game.") });

    public GameAdapter(string? dataRoot = null) : base(Game, new[] { new ProfileDescriptor("original", "Original", "original") }, dataRoot ??
        Environment.GetEnvironmentVariable("COMPANION_DATA_ROOT") ??
        CompanionDataPaths.DesktopRoot()) { }

    public override Readiness CheckReady(ProfileDescriptor profile, IReadOnlyDictionary<string, JsonElement> values)
    {
        var executable = values["engine.path"].GetString() ?? "";
        var rom = values["rom.path"].GetString() ?? "";
        var config = values["config.path"].GetString() ?? "";
        if (!Path.IsPathFullyQualified(executable) || !File.Exists(executable))
            return new(false, "Select the locally built Lagoon executable, then Apply.");
        if (!Path.IsPathFullyQualified(rom) || !File.Exists(rom))
            return new(false, "Select your original Lagoon (USA) ROM, then Apply.");
        if (!Path.IsPathFullyQualified(config) || !File.Exists(config))
            return new(false, "Select the existing native config.ini, then Apply.");
        try
        {
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
        return new(executable, Path.GetDirectoryName(executable)!, new[] {
            "--no-launcher", "--rom", values["rom.path"].GetString()!,
            "--config", values["config.path"].GetString()! });
    }

    // Optional: declare HasSetup/HasAchievements/CanResume and implement those adapter methods.
    // Existing games must preserve their released save/settings paths instead of adopting new ones blindly.
}
