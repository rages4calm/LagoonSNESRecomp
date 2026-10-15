using Companion.Launcher.Windows;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args) => CompanionApplication.Run(new GameAdapter(), args);
}
