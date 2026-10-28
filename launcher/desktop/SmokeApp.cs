using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Media.Imaging;
using Avalonia.Themes.Fluent;
using Avalonia.Threading;
using Companion.Launcher.Avalonia;
using Companion.Launcher.Core;

// Explicit CI-only entry: actual packaged runtime, native window and shared UI;
// fresh launcher profile, no setup/game launch or writes to gameplay progress.
internal sealed class SmokeApp : Application
{
    internal static string Output = "";
    public override void Initialize()
    {
        RequestedThemeVariant = Avalonia.Styling.ThemeVariant.Dark;
        Styles.Add(new FluentTheme());
    }
    public override void OnFrameworkInitializationCompleted()
    {
        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            var controller = new LauncherController(new GameAdapter(Path.Combine(Output, "profile")));
            desktop.MainWindow = new Window { Title = "Lagoon Companion diagnostic", Width = 1120, Height = 800,
                Content = new CompanionView(controller, new DesktopLaunchService()) };
            var timer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(2) };
            timer.Tick += (_, _) =>
            {
                timer.Stop();
                try
                {
                    var window = desktop.MainWindow;
                    using var bitmap = new RenderTargetBitmap(new PixelSize((int)window.Bounds.Width, (int)window.Bounds.Height), new Vector(96, 96));
                    bitmap.Render(window);
                    bitmap.Save(Path.Combine(Output, "launcher.png"), new PngBitmapEncoderOptions());
                    if (controller.HasChanges) throw new InvalidOperationException("Rendering modified settings.");
                    Console.WriteLine("PASS: packaged Companion rendered with isolated launcher data; no game or setup started.");
                    desktop.Shutdown(0);
                }
                catch (Exception error) { Console.Error.WriteLine(error); desktop.Shutdown(1); }
            };
            timer.Start();
        }
        base.OnFrameworkInitializationCompleted();
    }
}
