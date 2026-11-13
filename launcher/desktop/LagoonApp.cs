using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Layout;
using Avalonia.Platform.Storage;
using Avalonia.Themes.Fluent;
using Avalonia.Media.Imaging;
using Avalonia.Threading;
using Companion.Launcher.Avalonia;
using Companion.Launcher.Core;

internal sealed class LagoonApp : Application
{
    internal static bool ForceSetup;
    internal static string? SmokeOutput;
    public override void Initialize()
    {
        RequestedThemeVariant = Avalonia.Styling.ThemeVariant.Dark;
        Styles.Add(new FluentTheme());
    }
    public override void OnFrameworkInitializationCompleted()
    {
        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            var adapter = new GameAdapter(SmokeOutput == null ? null : Path.Combine(SmokeOutput, "profile"));
            var window = new Window { Title = "Lagoon Companion", Width = 1120, Height = 800, MinWidth = 620, MinHeight = 440 };
            CompanionView? view = null;
            bool busy = false;
            void OpenLauncher() => window.Content = view = new CompanionView(new LauncherController(adapter), new DesktopLaunchService());
            if (!ForceSetup && !adapter.NeedsRomSetup) OpenLauncher();
            else
            {
                var status = new TextBlock { Text = "We will check your ROM and prepare the game automatically.", TextWrapping = Avalonia.Media.TextWrapping.Wrap };
                var choose = new Button { Content = "Choose ROM file…", HorizontalAlignment = HorizontalAlignment.Stretch };
                choose.Click += async (_, _) =>
                {
                    var files = await window.StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions {
                        Title = "Choose the original Lagoon USA ROM file", AllowMultiple = false,
                        FileTypeFilter = new[] { new FilePickerFileType("SNES ROM") { Patterns = new[] { "*.sfc", "*.smc" } }, FilePickerFileTypes.All } });
                    if (files.Count == 0) return;
                    var path = files[0].TryGetLocalPath();
                    if (path == null) { status.Text = "Choose a ROM stored on this computer."; return; }
                    busy = true; choose.IsEnabled = false;
                    try
                    {
                        await adapter.ImportRomAsync(path, new Progress<OperationProgress>(p => status.Text = p.Detail), CancellationToken.None);
                        OpenLauncher();
                    }
                    catch (Exception error) { status.Text = error.Message; }
                    finally { busy = false; choose.IsEnabled = true; }
                };
                window.Content = new StackPanel { Margin = new Thickness(36), Spacing = 24, MaxWidth = 640, VerticalAlignment = VerticalAlignment.Center,
                    Children = {
                        new TextBlock { Text = "Choose your Lagoon ROM", FontSize = 26 },
                        new TextBlock { Text = "Select the original Lagoon USA .sfc or .smc file, not its folder. Extract ZIP archives first. Your ROM stays where it is.", TextWrapping = Avalonia.Media.TextWrapping.Wrap },
                        status, choose } };
            }
            window.Closing += (_, e) => { if (busy) e.Cancel = true; else if (view != null && !view.CanClose) { e.Cancel = true; view.RequestClose(); } };
            desktop.MainWindow = window;
            if (SmokeOutput != null)
            {
                var timer = new DispatcherTimer { Interval = TimeSpan.FromSeconds(2) };
                timer.Tick += (_, _) =>
                {
                    timer.Stop();
                    try
                    {
                        using var image = new RenderTargetBitmap(new PixelSize((int)window.Bounds.Width, (int)window.Bounds.Height), new Vector(96, 96));
                        image.Render(window);
                        image.Save(Path.Combine(SmokeOutput, "rom-setup.png"), new PngBitmapEncoderOptions());
                        Console.WriteLine("PASS: ROM setup window rendered; no ROM or gameplay data used.");
                        desktop.Shutdown(0);
                    }
                    catch (Exception error) { Console.Error.WriteLine(error); desktop.Shutdown(1); }
                };
                timer.Start();
            }
        }
        base.OnFrameworkInitializationCompleted();
    }
}
