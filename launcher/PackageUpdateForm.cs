using System.Diagnostics;
using System.Text.Json;

internal sealed class PackageUpdateForm : Form
{
    internal bool VisualCheck { get; set; }
    protected override bool ShowWithoutActivation => VisualCheck || base.ShowWithoutActivation;
    private readonly UpdatePackage package;
    private readonly Label status = new() { AutoSize = false, Dock = DockStyle.Top, Height = 76 };
    private readonly TextBox notes = new() { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Dock = DockStyle.Fill };
    private readonly Button install = new() { Text = "Download and update", AutoSize = true, Enabled = false };
    private readonly Button check = new() { Text = "Check again", AutoSize = true };
    private readonly Button close = new() { Text = "Close", AutoSize = true };
    private readonly CancellationTokenSource cancellation = new();
    private StableUpdate? latest;
    private string? pendingPlan;
    private bool busy;
    internal bool Scheduled { get; private set; }

    internal PackageUpdateForm(string root)
    {
        package = PackageUpdate.ReadPackage(root);
        Text = "Lagoon updates"; ClientSize = new Size(640, 440); MinimumSize = new Size(560, 400);
        StartPosition = FormStartPosition.CenterParent; Font = new Font("Segoe UI", 10);
        BackColor = Color.FromArgb(16, 18, 26); ForeColor = Color.WhiteSmoke; Padding = new Padding(20);
        var buttons = new FlowLayoutPanel { Dock = DockStyle.Bottom, Height = 45, FlowDirection = FlowDirection.LeftToRight };
        var releases = new Button { Text = "Official releases", AutoSize = true };
        releases.Click += (_, _) => Process.Start(new ProcessStartInfo(PackageUpdate.Repository + "/releases") { UseShellExecute = true });
        buttons.Controls.AddRange(new Control[] { install, check, releases, close });
        Controls.Add(notes); Controls.Add(status); Controls.Add(buttons);
        notes.BackColor = Color.FromArgb(27, 30, 42); notes.ForeColor = ForeColor; notes.BorderStyle = BorderStyle.FixedSingle;
        check.Click += async (_, _) => await Check();
        install.Click += async (_, _) => await Install();
        close.Click += (_, _) => { if (busy) cancellation.Cancel(); else Close(); };
        FormClosing += (_, e) => { if (busy) { cancellation.Cancel(); e.Cancel = true; } };
        Shown += async (_, _) => { if (!VisualCheck) await Check(); else status.Text = $"Installed: {package.Version}   Check for an official stable release when ready."; };
    }

    private void Busy(bool value)
    {
        busy = value; check.Enabled = !value; install.Enabled = !value && package.CanApply && (pendingPlan != null || latest?.Version > package.Version);
        close.Text = value ? "Cancel" : "Close";
    }

    private async Task Check()
    {
        if (busy || cancellation.IsCancellationRequested) return;
        Busy(true); status.Text = "Checking the official stable GitHub release…";
        try
        {
            pendingPlan = Pending(package.Root);
            using var client = PackageUpdate.CreateClient(); latest = await PackageUpdate.CheckAsync(client, cancellation.Token);
            notes.Text = latest.Notes.Replace("\n", Environment.NewLine);
            status.Text = $"Installed: {package.Version}{(package.CanApply ? "" : " (development folder)")}   Latest: {latest.Version}\n" +
                (!package.CanApply ? "Automatic updates apply only to an extracted release folder outside Git. Your source checkout will not be changed." :
                pendingPlan != null ? "An interrupted update is ready to resume. Saves, settings and both games' user files stay in place." :
                latest.Version > package.Version ? "A stable update is available. Both games must be closed. Companion will restart after updating." : "You have the latest stable package. Play remains available.");
            install.Text = pendingPlan != null ? "Resume update" : "Download and update";
        }
        catch (Exception error) { status.Text = "Could not check for updates. You can keep playing your installed games.\n" + error.Message; }
        finally { Busy(false); }
    }

    private async Task Install()
    {
        if (busy || cancellation.IsCancellationRequested) return;
        Busy(true);
        try
        {
            var plan = pendingPlan ?? await PackageUpdate.PrepareAsync(package, latest!, new Progress<string>(s => status.Text = s), cancellation.Token);
            cancellation.Token.ThrowIfCancellationRequested();
            PackageUpdate.CheckGamesClosed(package.Root);
            StartApply(plan);
            Scheduled = true; busy = false; DialogResult = DialogResult.OK; Close();
        }
        catch (OperationCanceledException) { status.Text = "Update cancelled. Installed games and user files were not replaced."; }
        catch (Exception error) { status.Text = "The update could not start. Your games can still be launched.\n" + error.Message; }
        finally { if (!IsDisposed) Busy(false); }
    }

    internal static string? Pending(string root)
    {
        var state = Path.Combine(root, PackageUpdate.PendingDirectory, "state.json");
        if (!File.Exists(state)) return null;
        PackageUpdate.EnsureNoLinks(state);
        if (new FileInfo(state).Length > 65536) throw new InvalidDataException("Unexpected updater recovery state.");
        using var json = JsonDocument.Parse(File.ReadAllBytes(state)); var r = json.RootElement;
        if (r.GetProperty("Complete").GetBoolean()) return null;
        if (!Path.GetFullPath(r.GetProperty("Root").GetString()!).Equals(Path.GetFullPath(root), StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Recovery belongs to a different installation.");
        var plan = r.GetProperty("Plan").GetString()!; PackageUpdate.EnsureNoLinks(plan);
        if (PackageUpdate.Hash(plan) != r.GetProperty("PlanHash").GetString()) throw new InvalidDataException("The pending update plan changed. Open the official release for recovery.");
        return plan;
    }

    private static void StartApply(string plan)
    {
        PackageUpdate.EnsureNoLinks(plan);
        var helper = Path.Combine(Path.GetDirectoryName(plan)!, "apply.ps1");
        PackageUpdate.EnsureNoLinks(helper);
        if (File.ReadAllText(helper) != PackageUpdateHelper.Script) throw new InvalidDataException("The update helper changed. Download the official update again.");
        var ps = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "WindowsPowerShell", "v1.0", "powershell.exe");
        var info = new ProcessStartInfo(ps) { UseShellExecute = false, CreateNoWindow = true, WorkingDirectory = Path.GetDirectoryName(plan)! };
        foreach (var arg in new[] { "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", helper, "-PlanPath", plan, "-PlanHash", PackageUpdate.Hash(plan), "-WaitProcessId", Environment.ProcessId.ToString() }) info.ArgumentList.Add(arg);
        using var child = Process.Start(info) ?? throw new IOException("Windows could not start the updater.");
    }

    protected override void Dispose(bool disposing) { if (disposing) cancellation.Dispose(); base.Dispose(disposing); }
}
