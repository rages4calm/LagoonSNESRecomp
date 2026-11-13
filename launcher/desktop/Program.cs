using Avalonia;
using Companion.Launcher.Avalonia;

internal static class Program
{
    [STAThread]
    private static void Main(string[] args)
    {
        if (args.Length == 2 && args[0] == "--import-rom")
        {
            try
            {
                new GameAdapter().ImportRomAsync(args[1], null, CancellationToken.None).GetAwaiter().GetResult();
                Console.WriteLine("ROM verified. Setup complete. Ready to Play.");
            }
            catch (Exception error) { Console.Error.WriteLine(error.Message); Environment.ExitCode = 1; }
            return;
        }
        if (args.Length == 2 && args[0] == "--smoke-test")
        {
            SmokeApp.Output = Path.GetFullPath(args[1]);
            if (Directory.Exists(SmokeApp.Output)) throw new IOException("Use a fresh diagnostic output directory.");
            Directory.CreateDirectory(SmokeApp.Output);
            AppBuilder.Configure<SmokeApp>().UsePlatformDetect().WithInterFont()
                .StartWithClassicDesktopLifetime(Array.Empty<string>());
            return;
        }
        LagoonApp.ForceSetup = args.Contains("--rom-setup");
        if (args.Length == 2 && args[0] == "--smoke-setup")
        {
            LagoonApp.SmokeOutput = Path.GetFullPath(args[1]);
            if (Directory.Exists(LagoonApp.SmokeOutput)) throw new IOException("Use a fresh diagnostic output directory.");
            Directory.CreateDirectory(LagoonApp.SmokeOutput);
            LagoonApp.ForceSetup = true;
        }
        AppBuilder.Configure<LagoonApp>().UsePlatformDetect().WithInterFont()
            .StartWithClassicDesktopLifetime(args);
    }
}
