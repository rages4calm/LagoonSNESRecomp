using Companion.Launcher.Core;

internal static class GameChoice
{
    internal const string Snes = "snes", X68000 = "x68000", Ask = "ask";
    private static string StorePath => Path.Combine(
        Environment.GetEnvironmentVariable("COMPANION_DATA_ROOT") ?? CompanionDataPaths.DesktopRoot(),
        "lagoon", "edition.txt");

    internal static string Load(string[] args)
    {
        var at = Array.IndexOf(args, "--game");
        if (at >= 0)
        {
            if (at + 1 >= args.Length) throw new ArgumentException("Choose --game snes, x68000 or ask.");
            var choice = args[at + 1].ToLowerInvariant();
            if (choice is Snes or X68000 or Ask) return choice;
            throw new ArgumentException("Choose --game snes, x68000 or ask.");
        }
        if (args.Contains("--rom-setup")) return Snes;
        try
        {
            if (File.Exists(StorePath))
            {
                var stored = File.ReadAllText(StorePath).Trim();
                if (stored is Snes or X68000) return stored;
            }
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException) { }
        return Ask;
    }

    internal static void Save(string choice)
    {
        if (choice is not (Snes or X68000)) return;
        Directory.CreateDirectory(Path.GetDirectoryName(StorePath)!);
        File.WriteAllText(StorePath, choice);
    }
}
