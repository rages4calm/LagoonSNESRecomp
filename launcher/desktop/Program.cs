using Avalonia;
using Companion.Launcher.Avalonia;

internal static class Program
{
    [STAThread]
    private static void Main(string[] args)
    {
        if (args.Length == 2 && args[0] == "--smoke-test")
        {
            SmokeApp.Output = Path.GetFullPath(args[1]);
            if (Directory.Exists(SmokeApp.Output)) throw new IOException("Use a fresh diagnostic output directory.");
            Directory.CreateDirectory(SmokeApp.Output);
            AppBuilder.Configure<SmokeApp>().UsePlatformDetect().WithInterFont()
                .StartWithClassicDesktopLifetime(Array.Empty<string>());
            return;
        }
        CompanionApp.AdapterFactory = () => new GameAdapter();
        AppBuilder.Configure<CompanionApp>().UsePlatformDetect().WithInterFont()
            .StartWithClassicDesktopLifetime(args);
    }
}
