using Avalonia;
using Avalonia.Controls;
using Avalonia.Headless;
using Avalonia.Threading;
using Avalonia.Themes.Fluent;
using Companion.Launcher.Avalonia;
using Companion.Launcher.Core;

internal static class Program
{
    [STAThread]
    private static void Main(string[] args)
    {
        if (args.Length != 1) throw new ArgumentException("Fresh isolated output directory required.");
        var output = Path.GetFullPath(args[0]);
        if (Directory.Exists(output)) throw new IOException("Output already exists.");
        Directory.CreateDirectory(output);
        AppBuilder.Configure<ProbeApp>().UseSkia().WithInterFont()
            .UseHeadless(new AvaloniaHeadlessPlatformOptions { UseHeadlessDrawing = false }).SetupWithoutStarting();
        var adapter = new GameAdapter(Path.Combine(output, "profile"));
        var controller = new LauncherController(adapter);
        var view = new CompanionView(controller, new DesktopLaunchService());
        var window = new Window { Width = 1120, Height = 800, Content = view };
        window.Show();
        foreach (var section in new[] { "Display", "Audio", "Input", "Quality of life" })
        {
            view.Navigate("section:" + section);
            Dispatcher.UIThread.RunJobs(); window.InvalidateVisual(); Dispatcher.UIThread.RunJobs();
            using var image = window.CaptureRenderedFrame() ?? throw new Exception("No rendered launcher frame.");
            image.Save(Path.Combine(output, section.Replace(' ', '-') + ".png"), new Avalonia.Media.Imaging.PngBitmapEncoderOptions());
        }
        if (controller.HasChanges) throw new Exception("Viewing portable launcher changed settings.");
        if (!controller.Definition.Options.Any(x => x.Id == "video.wide-aspect") ||
            !controller.Definition.Options.Any(x => x.Id == "audio.soundtrack"))
            throw new Exception("Portable host lost Lagoon settings.");
        window.Close();
        Console.WriteLine("PASS: real Lagoon adapter and shared portable UI render four pages without edits.");
    }
}
internal sealed class ProbeApp : Application
{
    public override void Initialize() { RequestedThemeVariant = Avalonia.Styling.ThemeVariant.Dark; Styles.Add(new FluentTheme()); }
}
