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
            if (args.Length == 2 && args[0] == "--import-rom")
            {
                new GameAdapter().ImportRomAsync(args[1], null, CancellationToken.None).GetAwaiter().GetResult();
                Console.WriteLine("ROM verified. Setup complete. Ready to Play.");
                return 0;
            }
            if (!args.Contains("--smoke-test"))
            {
                Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
            }
            if (args is ["--x68000-saves"])
            {
                using var saves = new X68000SaveForm(new X68000Adapter()) { Icon = icon };
                Application.Run(saves);
                return 0;
            }
            // Game choice routes to one of two fully separate adapters. The SNES
            // adapter below is the unchanged shipped path.
            var choice = GameChoice.Load(args);
            if (choice == GameChoice.Ask && args.Contains("--smoke-test")) choice = GameChoice.Snes;
            if (choice == GameChoice.Ask)
            {
                using var picker = new GameChoiceForm { Icon = icon };
                if (picker.ShowDialog() != DialogResult.OK) return 0;
                choice = picker.Choice;
            }
            if (choice == GameChoice.Snes && !args.Contains("--smoke-test") &&
                !args.Contains("--rom-setup") && GameChoice.TryOpenInstalledSnes())
                return 0;
            IGameAdapter adapter = choice == GameChoice.X68000 ? new X68000Adapter() : new GameAdapter();
            if (args.Contains("--smoke-test")) return CompanionApplication.Run(adapter, args);
            if (adapter is GameAdapter snes && (args.Contains("--rom-setup") || snes.NeedsRomSetup))
            {
                using var setup = new RomSetupForm(snes) { Icon = icon };
                if (setup.ShowDialog() != DialogResult.OK) return 0;
            }
            using var launcher = new LauncherForm(new LauncherController(adapter)) { Icon = icon };
            if (adapter is X68000Adapter) HideSingleProfileSelector(launcher);
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

    private static void HideSingleProfileSelector(LauncherForm launcher)
    {
        // Companion 0.2.0 exposes the selector as an ordinary control.
        var layout = launcher.Controls.OfType<TableLayoutPanel>().FirstOrDefault();
        if (layout?.GetControlFromPosition(1, 0) is not TableLayoutPanel header ||
            header.GetControlFromPosition(1, 1) is not ComboBox
            { AccessibleName: "Game profile", Items.Count: 1 } selector) return;
        selector.Visible = false;
        selector.TabStop = false;
        if (header.GetControlFromPosition(1, 0) is Label { Text: "PROFILE" } label)
            label.Visible = false;
    }
}
