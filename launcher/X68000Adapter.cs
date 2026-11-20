using System.Security.Cryptography;
using System.Text.Json;
using Companion.Launcher.Core;

public sealed class X68000Adapter : JsonGameAdapterBase
{
    private const string ReceiptName = "lagoon-extra-install.json";
    private const string AdventureEdition = "x68000-enhanced-adventure";
    private const string EnhancedDirectory = "retrorecomp-expedition";
    private static readonly ProfileDescriptor EnhancedGame =
        new("enhanced-expedition", "X68000 Enhanced", "x68000-enhanced-expedition");
    private static readonly string PlayerName = OperatingSystem.IsWindows() ? "Lagoon - English.exe" : "Lagoon - English";
    private static readonly JsonSerializerOptions ReceiptFormat = new() { PropertyNamingPolicy = JsonNamingPolicy.SnakeCaseLower };

    internal sealed record SaveSource(string DirectoryName, string Title);
    internal static IReadOnlyList<SaveSource> LegacySaveSources { get; } = new SaveSource[] {
        new("retrorecomp-save-disk", "User Disk"),
        new("retrorecomp-rewards", "Reward Boost"),
        new("retrorecomp-enhanced", "Enhanced Combat"),
        new("retrorecomp-adventure", "Enhanced Adventure"),
    };

    private sealed record InstallReceipt(int Format, string ConverterVersion, string System,
        string Language, string Executable, string Sha256, bool ReferenceValidated, bool UserDisk = false,
        int ExpRate = 1, int GoldRate = 1, bool Enhanced = false, bool Adventure = false,
        string? SourceIdentity = null, bool Expedition = false);
    private sealed record AtlasProof(bool MapFitChecked, bool ControllerMenusAndDiskRoundtrip,
        bool ControllerRemapsChecked, bool ControllerFirstBootChecked, bool WidescreenChecked,
        bool WideCrossingSpriteChecked, bool NormalDoorPacingChecked, bool WideExtendedNpcChecked,
        int DoorBackdropFrames, int DoorRevealFrames, int DialogueBackdropFrames);
    private sealed record AtlasReceipt(int Format, string SourceIdentity, string Sha256, int TerrainAreas, AtlasProof? OpeningRoute);

    private readonly string project;
    private string PlayerDirectory(string name) => Path.Combine(project, "local-x68000", name);
    private string EnhancedPlayer => Path.Combine(PlayerDirectory(EnhancedDirectory), PlayerName);

    private static string FindProject()
    {
        for (var directory = new DirectoryInfo(AppContext.BaseDirectory); directory != null; directory = directory.Parent)
        {
            if (File.Exists(Path.Combine(directory.FullName, "x68000", "achievements.json"))) return directory.FullName;
        }
        return AppContext.BaseDirectory;
    }

    private static GameDefinition Define() => new(
        "lagoon-x68000", "Lagoon — X68000 Enhanced",
        "The Sharp X68000 game in English, with the gameplay improvements, Atlas and controller shortcuts built in.",
        X68000Preferences.Options.ToArray(),
        HasSetup: false, HasAchievements: true,
        Branding: new LauncherTheme("#10121A", "#1B1E2B", "#5FD3B8", "#F1F2F7", "#A3A8BB"));

    public X68000Adapter(string? dataRoot = null) : this(FindProject(), dataRoot) { }

    internal X68000Adapter(string project, string? dataRoot) : base(Define(), new[] { EnhancedGame },
        dataRoot ?? Environment.GetEnvironmentVariable("COMPANION_DATA_ROOT") ?? CompanionDataPaths.DesktopRoot())
    {
        this.project = Path.GetFullPath(project);
    }

    public override SettingsSnapshot ReadSettings(ProfileDescriptor profile) =>
        ReadSettings(profile, X68000Preferences.ReadFile(EnhancedPlayer));

    private SettingsSnapshot ReadSettings(ProfileDescriptor profile, NativeSettings.Document document)
    {
        var snapshot = base.ReadSettings(profile);
        // Old path selections and unused options must not select a different build.
        snapshot.Values.Clear();
        return X68000Preferences.Read(snapshot, document);
    }

    public override void ApplySettings(ProfileDescriptor profile, IReadOnlyDictionary<string, JsonElement> values, string expectedRevision)
    {
        var errors = OptionValidation.Validate(Definition, values).Concat(ValidateCustom(profile, values)).ToArray();
        if (errors.Length > 0) throw new SettingsValidationException(errors);
        Directory.CreateDirectory(ProfileDirectory(profile));
        using var gate = new FileStream(Path.Combine(ProfileDirectory(profile), "x68000-settings.lock"),
            FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None);
        var current = ReadSettings(profile);
        if (current.Revision != expectedRevision) throw new SettingsConflictException();
        if (!X68000Preferences.Changed(current, values)) return;
        var ready = VerifyEnhanced(profile, out var receipt, out _);
        if (!ready.Ready) throw new InvalidOperationException(ready.Summary);
        using var session = LockRuntime(EnhancedPlayer, receipt!);
        var document = X68000Preferences.ReadFile(EnhancedPlayer);
        if (ReadSettings(profile, document).Revision != expectedRevision) throw new SettingsConflictException();
        X68000Preferences.Apply(document, current, values);
    }

    private static string RuntimeData(string executable, string identity) =>
        Path.Combine(Path.GetDirectoryName(executable)!, "datas", "games", "x68-" + identity);

    private static FileStream LockRuntime(string executable, InstallReceipt receipt)
    {
        var path = RuntimeData(executable, receipt.SourceIdentity!);
        for (var parent = new DirectoryInfo(path); parent != null; parent = parent.Parent)
            if (parent.Exists && parent.Attributes.HasFlag(FileAttributes.ReparsePoint))
                throw new InvalidDataException("A linked X68000 data directory cannot be edited.");
        Directory.CreateDirectory(path);
        var lockPath = Path.Combine(path, "session.lock");
        if (File.Exists(lockPath) && File.GetAttributes(lockPath).HasFlag(FileAttributes.ReparsePoint))
            throw new InvalidDataException("A linked X68000 session file cannot be used.");
        try { return new FileStream(lockPath, FileMode.OpenOrCreate, FileAccess.ReadWrite, FileShare.None); }
        catch (IOException error) { throw new InvalidOperationException("Close Lagoon before applying X68000 settings.", error); }
    }

    public override AchievementSnapshot ReadAchievements(ProfileDescriptor profile)
    {
        var catalog = AchievementJsonReader.Read(Path.Combine(project, "x68000", "achievements.json"),
            null, Definition.Id, AdventureEdition);
        if (!VerifyEnhanced(profile, out var receipt, out _).Ready) return catalog;
        var progress = NativeSettings.Document.Read(Path.Combine(
            RuntimeData(EnhancedPlayer, receipt!.SourceIdentity!), "achievements.ini"));
        return catalog with { Entries = catalog.Entries.Select(entry =>
            ulong.TryParse(progress.IniValue("Unlocked", entry.Id), out var unlocked) && unlocked is > 0 and <= long.MaxValue
                ? entry with { Progress = entry.Target, UnlockedAt = unlocked } : entry).ToArray() };
    }

    public override Readiness CheckReady(ProfileDescriptor profile, IReadOnlyDictionary<string, JsonElement> values)
        => VerifyEnhanced(profile, out _, out _);

    private Readiness VerifyEnhanced(ProfileDescriptor profile, out InstallReceipt? verified, out string? atlas)
    {
        verified = null;
        atlas = null;
        if (profile != EnhancedGame) return new(false, "Choose X68000 Enhanced to play. Earlier releases are save import sources.");
        try
        {
            var receipt = VerifyBase(EnhancedPlayer);
            if (!IsEnhancedPreset(receipt))
                return new(false, "Build X68000 Enhanced with the fixed gameplay settings. See x68000/PLAYING.md.");
            var folder = PlayerDirectory(EnhancedDirectory);
            atlas = VerifiedAtlas(folder, receipt.SourceIdentity!);
            if (atlas is null)
                return new(false, "The verified X68000 Enhanced player is missing or out of date. Finish building the Atlas player.");
            verified = receipt;
            return new(true, "Press Start or Enter to skip the opening. Use Y on your controller for Save / Load.");
        }
        catch (Exception error) when (error is IOException or InvalidDataException or UnauthorizedAccessException or JsonException)
        {
            return new(false, error.Message);
        }
    }

    private static InstallReceipt VerifyBase(string executable)
    {
        if (!File.Exists(executable))
            throw new FileNotFoundException("Build the English X68000 player first. See x68000/PLAYING.md.");
        var receiptPath = Path.Combine(Path.GetDirectoryName(executable)!, ReceiptName);
        if (!File.Exists(receiptPath) || new FileInfo(receiptPath).Length > 16384)
            throw new InvalidDataException("The X68000 build receipt is missing. See x68000/PLAYING.md.");
        var receipt = JsonSerializer.Deserialize<InstallReceipt>(File.ReadAllText(receiptPath), ReceiptFormat);
        if (receipt is not { Format: 1, System: "x68", Language: "en", ReferenceValidated: true, SourceIdentity.Length: 64 } ||
            !receipt.SourceIdentity.All(Uri.IsHexDigit) || receipt.ConverterVersion != "1.8.0" ||
            receipt.Executable != Path.GetFileName(executable) ||
            receipt.ExpRate is < 1 or > 8 || receipt.GoldRate is < 1 or > 8 || !HashMatches(executable, receipt.Sha256))
            throw new InvalidDataException("The X68000 player does not match its verified build receipt.");
        return receipt;
    }

    private static bool IsEnhancedPreset(InstallReceipt receipt) =>
        receipt is { UserDisk: true, Enhanced: true, Adventure: true, Expedition: true, ExpRate: 2, GoldRate: 2 };

    internal X68000SaveDisk OpenImportSource(SaveSource source)
    {
        if (!LegacySaveSources.Contains(source)) throw new ArgumentException("Unknown X68000 save source.", nameof(source));
        return OpenUserDisk(Path.Combine(PlayerDirectory(source.DirectoryName), PlayerName), canImport: false);
    }

    internal X68000SaveDisk OpenEnhancedSaveDisk() => OpenUserDisk(EnhancedPlayer, canImport: true);

    private static X68000SaveDisk OpenUserDisk(string player, bool canImport)
    {
        var receipt = VerifyBase(player);
        if (!receipt.UserDisk) throw new InvalidDataException("This build has no verified User Disk.");
        if (canImport && !IsEnhancedPreset(receipt))
            throw new InvalidDataException("This is not the X68000 Enhanced User Disk.");
        var folder = Path.GetDirectoryName(player)!;
        using var report = JsonDocument.Parse(File.ReadAllBytes(Path.Combine(folder, "conversion-report.json")));
        var root = report.RootElement;
        if (!root.TryGetProperty("export_receipt", out var export) ||
            !export.TryGetProperty("sha256", out var hash) || hash.GetString() != receipt.Sha256 ||
            !root.TryGetProperty("source_identity", out var identityField) || identityField.GetString() != receipt.SourceIdentity ||
            !root.TryGetProperty("machine", out var machine) || !machine.TryGetProperty("media", out var media) ||
            media.ValueKind != JsonValueKind.Array || media.GetArrayLength() != 5 ||
            !media[4].TryGetProperty("format", out var format) || format.GetString() != "xdf" ||
            !media[4].TryGetProperty("sha256", out var diskHash) ||
            diskHash.GetString() is not { Length: 64 } template || !template.All(Uri.IsHexDigit))
            throw new InvalidDataException("The conversion report does not match this verified X68000 player.");

        return new X68000SaveDisk(RuntimeData(player, receipt.SourceIdentity!), template, canImport);
    }

    public override LaunchPlan BuildLaunchPlan(ProfileDescriptor profile, IReadOnlyDictionary<string, JsonElement> values, bool resume)
    {
        if (resume) throw new NotSupportedException("Start X68000 Enhanced and press F9 to load its quick save.");
        var ready = VerifyEnhanced(profile, out _, out var atlas);
        if (!ready.Ready) throw new InvalidOperationException(ready.Summary);
        return new(atlas!, PlayerDirectory(EnhancedDirectory), Array.Empty<string>());
    }

    private static string? VerifiedAtlas(string folder, string identity)
    {
        var atlas = Path.Combine(folder, "Lagoon - Atlas.exe");
        var note = Path.Combine(folder, "lagoon-atlas-install.json");
        if (!File.Exists(atlas) || !File.Exists(note) || new FileInfo(note).Length > 16384) return null;
        var info = JsonSerializer.Deserialize<AtlasReceipt>(File.ReadAllText(note), ReceiptFormat);
        return info is { Format: 2, TerrainAreas: 49,
                OpeningRoute: { MapFitChecked: true, ControllerMenusAndDiskRoundtrip: true, ControllerRemapsChecked: true,
                    ControllerFirstBootChecked: true, WidescreenChecked: true, WideCrossingSpriteChecked: true,
                    NormalDoorPacingChecked: true, WideExtendedNpcChecked: true, DoorBackdropFrames: >= 1, DoorRevealFrames: >= 1,
                    DialogueBackdropFrames: >= 4 } } &&
            info.SourceIdentity == identity && HashMatches(atlas, info.Sha256)
            ? atlas : null;
    }

    private static bool HashMatches(string file, string expected)
    {
        using var stream = File.OpenRead(file);
        return Convert.ToHexString(SHA256.HashData(stream)).Equals(expected, StringComparison.OrdinalIgnoreCase);
    }
}
