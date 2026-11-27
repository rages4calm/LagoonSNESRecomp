internal sealed class X68000UpdateForm : Form
{
    internal bool VisualCheck { get; set; }
    protected override bool ShowWithoutActivation => VisualCheck || base.ShowWithoutActivation;
    private readonly X68000Adapter adapter;
    private readonly TextBox log = new() { Multiline = true, ReadOnly = true, ScrollBars = ScrollBars.Vertical, Dock = DockStyle.Fill };
    private readonly Button update = new() { Text = "Update game", AutoSize = true };
    private readonly CancellationTokenSource cancellation = new();
    private bool busy;
    internal X68000UpdateForm(X68000Adapter adapter)
    {
        this.adapter = adapter;
        Text = "Update X68000 game"; ClientSize = new Size(650, 420); Padding = new Padding(20); Font = new Font("Segoe UI", 10);
        StartPosition = FormStartPosition.CenterParent;
        var title = new Label { Dock = DockStyle.Top, Height = 85, Text = "Apply the X68000 runtime improvements included with this package.\nCached build inputs are reused when available. Saves and settings stay in place.\nClose both games first. This is separate from downloading a Companion package." };
        var buttons = new FlowLayoutPanel { Dock = DockStyle.Bottom, Height = 44 };
        var close = new Button { Text = "Close", AutoSize = true };
        close.Click += (_, _) => { if (busy) cancellation.Cancel(); else Close(); };
        buttons.Controls.AddRange(new Control[] { update, close }); Controls.Add(log); Controls.Add(title); Controls.Add(buttons);
        update.Click += async (_, _) => await Run();
        FormClosing += (_, e) => { if (busy) { cancellation.Cancel(); e.Cancel = true; } };
    }
    private async Task Run()
    {
        if (busy) return; busy = true; update.Enabled = false;
        try
        {
            if (X68000Setup.RuntimeCurrent(adapter.ProjectRoot) && adapter.CheckReady(adapter.Profiles[0], adapter.ReadSettings(adapter.Profiles[0]).Values).Ready)
            {
                log.AppendText("Already up to date. No rebuild is needed.\r\n"); return;
            }
            PackageUpdate.CheckGamesClosed(adapter.ProjectRoot);
            await X68000Setup.RunAsync(X68000Setup.CachedPlan(adapter.ProjectRoot), line => {
                if (!IsDisposed) BeginInvoke(() => log.AppendText(line + Environment.NewLine));
            }, cancellation.Token);
            log.AppendText("X68000 game updated. You can close this window and play.\r\n");
        }
        catch (X68000SetupException error) when (error.ExitCode == 3)
        {
            log.AppendText("The earlier installation did not retain reusable inputs. Choose your disk ZIP once to update.\r\n");
            using var setup = new X68000SetupForm(adapter) { Icon = Icon }; setup.ShowDialog(this);
        }
        catch (OperationCanceledException) { log.AppendText("Update cancelled.\r\n"); }
        catch (Exception error) { log.AppendText(error.Message + "\r\n"); }
        finally { busy = false; update.Enabled = !cancellation.IsCancellationRequested; }
    }
    protected override void Dispose(bool disposing) { if (disposing) cancellation.Dispose(); base.Dispose(disposing); }
}
