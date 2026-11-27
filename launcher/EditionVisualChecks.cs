using System.Drawing.Imaging;
using Companion.Launcher.Windows;
using Companion.Launcher.Core;

internal static class EditionVisualChecks
{
    internal static void Capture(string output, string project)
    {
        if (Directory.Exists(output)) throw new IOException("Use a fresh launcher visual-check directory.");
        Directory.CreateDirectory(output);
        using var picker = new GameChoiceForm { VisualCheck = true };
        using var setup = new X68000SetupForm(new X68000Adapter(project, Path.Combine(output, "profile"))) { VisualCheck = true };
        Save(picker, Path.Combine(output, "edition-picker.png"));
        Save(setup, Path.Combine(output, "x68000-setup.png"));
        using var updates = new PackageUpdateForm(project) { VisualCheck = true };
        using var gameUpdate = new X68000UpdateForm(new X68000Adapter(project, Path.Combine(output, "update-profile"))) { VisualCheck = true };
        Save(updates, Path.Combine(output,"package-updates.png"));
        Save(gameUpdate,Path.Combine(output,"game-update.png"));
        var adapter = new X68000Adapter(project,Path.Combine(output,"rail-profile"));
        var controller = new LauncherController(adapter);
        using var launcher = new LauncherForm(controller);
        Program.AddEditionTools(launcher,adapter,()=>{},controller);
        Save(launcher,Path.Combine(output,"launcher-update-sidebar.png"));
        Console.WriteLine("PASS: edition picker and X68000 setup form rendered offscreen without desktop activation.");
    }

    private static void Save(Form form, string path)
    {
        form.StartPosition = FormStartPosition.Manual;
        form.Location = new Point(-30000, -30000);
        form.ShowInTaskbar = false; form.Opacity = 0;
        form.Show(); form.PerformLayout(); Application.DoEvents();
        using var bitmap = new Bitmap(form.Width, form.Height);
        form.DrawToBitmap(bitmap, new Rectangle(Point.Empty, bitmap.Size));
        bitmap.Save(path, ImageFormat.Png);
        form.Close();
    }
}
