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
            if (args.Contains("--x68000-setup") && args.Contains("--inputs"))
            {
                string? Argument(string name) { var i = Array.IndexOf(args, name); return i >= 0 && i + 1 < args.Length ? args[i + 1] : null; }
                var x68 = new X68000Adapter();
                X68000Setup.RunAsync(X68000Setup.BuildPlan(x68.ProjectRoot, Argument("--inputs")!, Argument("--ipl"), Argument("--cgrom")),
                    Console.WriteLine, CancellationToken.None).GetAwaiter().GetResult();
                var ready = x68.CheckReady(x68.Profiles[0], x68.ReadSettings(x68.Profiles[0]).Values);
                if (!ready.Ready) throw new InvalidOperationException(ready.Summary);
                return 0;
            }
            if (!args.Contains("--smoke-test"))
            {
                Application.SetHighDpiMode(HighDpiMode.PerMonitorV2);
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
            }
            if (args.Length == 3 && args[0] == "--smoke-editions")
            {
                EditionVisualChecks.Capture(Path.GetFullPath(args[1]), Path.GetFullPath(args[2]));
                return 0;
            }
            if (args is ["--x68000-saves"])
            {
                using var saves = new X68000SaveForm(new X68000Adapter()) { Icon = icon };
                Application.Run(saves);
                return 0;
            }
            var choice = args.Contains("--x68000-setup") ? GameChoice.X68000 : GameChoice.Load(args);
            if (args.Contains("--smoke-test"))
                return CompanionApplication.Run(args.Contains("--game") && choice == GameChoice.X68000 ? new X68000Adapter() : new GameAdapter(), args);
            while (true)
            {
                if (choice == GameChoice.Ask)
                {
                    using var picker = new GameChoiceForm { Icon = icon };
                    if (picker.ShowDialog() != DialogResult.OK) return 0;
                    choice = picker.Choice;
                }
                IGameAdapter adapter = choice == GameChoice.X68000 ? new X68000Adapter() : new GameAdapter();
                if (adapter is GameAdapter snes && (args.Contains("--rom-setup") || snes.NeedsRomSetup))
                {
                    using var setup = new RomSetupForm(snes) { Icon = icon };
                    if (setup.ShowDialog() != DialogResult.OK) { choice = GameChoice.Ask; continue; }
                }
                if (adapter is X68000Adapter x68 && (args.Contains("--x68000-setup") || !x68.CheckReady(x68.Profiles[0], x68.ReadSettings(x68.Profiles[0]).Values).Ready))
                {
                    using var setup = new X68000SetupForm(x68) { Icon = icon };
                    if (setup.ShowDialog() != DialogResult.OK) { choice = GameChoice.Ask; continue; }
                }
                var controller = new LauncherController(adapter);
                using var launcher = new LauncherForm(controller) { Icon = icon };
                bool switchEdition = false;
                AddEditionTools(launcher, adapter, () => { switchEdition = true; launcher.Close(); }, controller);
                if (adapter is X68000Adapter) HideSingleProfileSelector(launcher);
                Application.Run(launcher);
                if (!switchEdition) return 0;
                choice = GameChoice.Ask;
                args = Array.Empty<string>();
            }
        }
        catch (Exception error)
        {
            Console.Error.WriteLine(error.Message);
            if (!args.Contains("--import-rom") && !args.Contains("--smoke-test") && !(args.Contains("--x68000-setup") && args.Contains("--inputs")))
                MessageBox.Show(error.Message, "Lagoon setup", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
        finally { Application.Idle -= ApplyIcon; }
    }
    internal static void AddEditionTools(LauncherForm launcher, IGameAdapter adapter, Action choose, LauncherController controller)
    {
        if (launcher.Controls.OfType<TableLayoutPanel>().FirstOrDefault()?.GetControlFromPosition(0, 0) is not FlowLayoutPanel rail) return;
        rail.AutoScroll = true;
        void Add(string text, Action click)
        {
            var button = new Button { Text = text, Width = 170, Height = 38, FlatStyle = FlatStyle.Flat, TextAlign = ContentAlignment.MiddleLeft,
                ForeColor = launcher.ForeColor, BackColor = rail.BackColor, Margin = new Padding(0, 8, 0, 0) };
            button.FlatAppearance.BorderColor = Color.FromArgb(0x5F, 0xD3, 0xB8);
            button.Click += (_, _) => click(); rail.Controls.Add(button);
        }
        Add("Choose edition", choose);
        Add("Check for updates", () => {
            if (controller.HasChanges) { MessageBox.Show(launcher, "Apply or revert your pending settings before updating.", "Lagoon updates"); return; }
            using var update = new PackageUpdateForm(PackageUpdate.FindRoot(AppContext.BaseDirectory)) { Icon = launcher.Icon };
            update.ShowDialog(launcher);
            if (update.Scheduled) launcher.Close();
        });
        if (adapter is X68000Adapter x68)
        {
            Add("X68000 setup", () => { using var setup = new X68000SetupForm(x68) { Icon = launcher.Icon }; setup.ShowDialog(launcher); });
            Add("Copy X68000 saves", () => { using var saves = new X68000SaveForm(x68) { Icon = launcher.Icon }; saves.ShowDialog(launcher); });
            Add("Update X68000 game", () => { using var update = new X68000UpdateForm(x68) { Icon = launcher.Icon }; update.ShowDialog(launcher); });
        }
    }

    private static void HideSingleProfileSelector(LauncherForm launcher)
    {
        var layout = launcher.Controls.OfType<TableLayoutPanel>().FirstOrDefault();
        if (layout?.GetControlFromPosition(1, 0) is not TableLayoutPanel header ||
            header.GetControlFromPosition(1, 1) is not ComboBox { AccessibleName: "Game profile", Items.Count: 1 } selector) return;
        selector.Visible = false; selector.TabStop = false;
        if (header.GetControlFromPosition(1, 0) is Label { Text: "PROFILE" } label) label.Visible = false;
    }

}
