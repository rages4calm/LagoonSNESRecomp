using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.Json;

// Validation is shared by the existing Windows and new portable adapters.
// A setup-host compile must never be presented as a playable package.
internal static class PortablePackageChecks
{
    internal static string HostPlatform => (OperatingSystem.IsLinux() ? "linux-" : OperatingSystem.IsMacOS() ? "macos-" : "windows-") +
        (RuntimeInformation.ProcessArchitecture == Architecture.Arm64 ? "arm64" : "x64");

    internal static void Validate(string root)
    {
        using var document = JsonDocument.Parse(File.ReadAllText(Path.Combine(root, "portable-package.json")));
        var c = document.RootElement;
        if (c.GetProperty("format_version").GetInt32() != 1 || c.GetProperty("game_id").GetString() != "lagoon" ||
            !c.GetProperty("playable").GetBoolean() || c.GetProperty("mode").GetString() != "prebuilt-own-rom" ||
            c.GetProperty("executable").GetString() != "game/LagoonSNESRecomp")
            throw new InvalidDataException("This is not a playable Lagoon desktop package.");
        if (c.GetProperty("platform").GetString() != HostPlatform)
            throw new InvalidDataException("Download the Lagoon package for this operating system and architecture.");
        var engine = Path.Combine(root, "game", "LagoonSNESRecomp");
        var expected = c.GetProperty("game_sha256").GetString();
        using var stream = File.OpenRead(engine);
        if (!Convert.ToHexString(SHA256.HashData(stream)).Equals(expected, StringComparison.OrdinalIgnoreCase))
            throw new InvalidDataException("The game executable differs from this package. Extract a fresh release.");
        if (!OperatingSystem.IsWindows() && (File.GetUnixFileMode(engine) &
            (UnixFileMode.UserExecute | UnixFileMode.GroupExecute | UnixFileMode.OtherExecute)) == 0)
            throw new InvalidDataException("The game is not executable. Extract the release preserving Unix permissions.");
    }
}
