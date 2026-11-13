using Companion.Launcher.Windows;
using Companion.Launcher.Core;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        using var icon = System.Drawing.Icon.ExtractAssociatedIcon(
            Path.Combine(AppContext.BaseDirectory, "lagoon.Companion.exe"));
        void ApplyIcon(object? sender, EventArgs e)
        {
            if (Application.OpenForms.Count == 0) return;
            foreach (Form form in Application.OpenForms) form.Icon = icon;
            Application.Idle -= ApplyIcon;
        }
        Application.Idle += ApplyIcon;
        try
        {
            var adapter = new GameAdapter();
            if (args.Length == 2 && args[0] == "--import-rom")
            {
                adapter.ImportRomAsync(args[1], null, CancellationToken.None).GetAwaiter().GetResult();
                Console.WriteLine("ROM verified. Setup complete. Ready to Play.");
                return 0;
            }
            if (args.Contains("--smoke-test")) return CompanionApplication.Run(adapter, args);
            Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            if (!args.Contains("--smoke-test") && (args.Contains("--rom-setup") || adapter.NeedsRomSetup))
            {
                using var setup = new RomSetupForm(adapter) { Icon = icon };
                if (setup.ShowDialog() != DialogResult.OK) return 0;
            }
            using var launcher = new LauncherForm(new LauncherController(adapter)) { Icon = icon };
            Application.Run(launcher);
            return 0;
        }
        catch (Exception error)
        {
            Console.Error.WriteLine(error.Message);
            if (!args.Contains("--import-rom") && !args.Contains("--smoke-test"))
                MessageBox.Show(error.Message, "Lagoon setup", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
        finally { Application.Idle -= ApplyIcon; }
    }
}
