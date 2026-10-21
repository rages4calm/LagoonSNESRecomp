using Companion.Launcher.Windows;

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
        try { return CompanionApplication.Run(new GameAdapter(), args); }
        finally { Application.Idle -= ApplyIcon; }
    }
}
