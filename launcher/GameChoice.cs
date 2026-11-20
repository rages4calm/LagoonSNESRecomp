using Companion.Launcher.Core;
using System.Diagnostics;

// Remembers which Companion game choice to open: "SNES Enhanced" (the shipped
// GameAdapter, byte-identical) or "X68000 Enhanced" (X68000Adapter). The store
// lives in Companion's own data root and decides only which adapter is
// constructed; the two adapters share no state.
internal static class GameChoice
{
    public const string Snes = "snes";
    public const string X68000 = "x68000";
    public const string Ask = "ask";

    private static string StorePath => Path.Combine(
        Environment.GetEnvironmentVariable("COMPANION_DATA_ROOT") ?? CompanionDataPaths.DesktopRoot(),
        "lagoon-extra", "edition.txt");

    internal static string? FindInstalledSnesLauncher(string launcherDirectory)
    {
        for (var project = new DirectoryInfo(launcherDirectory); project?.Parent != null; project = project.Parent)
        {
            if (!File.Exists(Path.Combine(project.FullName, "X68000-ARCHITECTURE.md"))) continue;
            if (File.Exists(Path.Combine(project.FullName, "build", "LagoonSNESRecomp.exe"))) return null;
            var installed = Path.Combine(project.Parent.FullName, "Lagoon", "LagoonSNESRecomp");
            var companion = Path.Combine(installed, "launcher", "bin", "Release", "net8.0-windows", "lagoon.Companion.exe");
            return File.Exists(Path.Combine(installed, "rom_identity.txt")) && File.Exists(companion)
                ? companion : null;
        }
        return null;
    }

    public static bool TryOpenInstalledSnes()
    {
        if (!OperatingSystem.IsWindows() || FindInstalledSnesLauncher(AppContext.BaseDirectory) is not string companion)
            return false;
        using var process = Process.Start(new ProcessStartInfo(companion) {
            WorkingDirectory = Path.GetDirectoryName(companion)!, UseShellExecute = false,
        });
        if (process == null) throw new IOException("Could not open the installed SNES Companion.");
        return true;
    }

    public static string Load(string[] args)
    {
        var at = Array.IndexOf(args, "--game");
        if (at >= 0 && at + 1 < args.Length)
        {
            var flag = args[at + 1].ToLowerInvariant();
            // An explicit --game ask re-opens the picker even when a choice is stored.
            if (flag is Snes or X68000 or Ask) return flag;
        }
        try
        {
            if (File.Exists(StorePath))
            {
                var stored = File.ReadAllText(StorePath).Trim().ToLowerInvariant();
                if (stored is Snes or X68000) return stored;
            }
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { }
        return Ask;
    }

    public static void Save(string choice)
    {
        if (choice is not (Snes or X68000)) return;
        try
        {
            Directory.CreateDirectory(Path.GetDirectoryName(StorePath)!);
            File.WriteAllText(StorePath, choice);
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { }
    }
}
