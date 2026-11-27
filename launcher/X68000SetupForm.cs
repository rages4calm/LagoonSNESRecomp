using Companion.Launcher.Core;

internal sealed class X68000SetupForm : Form
{
    internal bool VisualCheck { get; set; }
    protected override bool ShowWithoutActivation => VisualCheck || base.ShowWithoutActivation;
    private readonly X68000Adapter adapter;
    private readonly TextBox disks = new() { Dock = DockStyle.Fill };
    private readonly TextBox ipl = new() { Dock = DockStyle.Fill };
    private readonly TextBox font = new() { Dock = DockStyle.Fill };
    private readonly CheckBox customFirmware = new() { Text = "Use custom firmware (advanced)", AutoSize = true };
    private readonly RichTextBox log = new() { Dock = DockStyle.Fill, ReadOnly = true, BorderStyle = BorderStyle.FixedSingle };
    private readonly Button build = new() { Text = "Set up game", AutoSize = true };
    private readonly Button cancel = new() { Text = "Back", AutoSize = true };
    private readonly List<Button> pickers = [];
    private CancellationTokenSource? running;

    internal X68000SetupForm(X68000Adapter adapter)
    {
        this.adapter = adapter;
        Text = "Set up X68000 Enhanced";
        StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(820, 640); MinimumSize = new Size(720, 560);
        BackColor = Color.FromArgb(0x10, 0x12, 0x1A); ForeColor = Color.FromArgb(0xF1, 0xF2, 0xF7);
        Font = new Font("Segoe UI", 10);
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(26), ColumnCount = 1, RowCount = 8 };
        layout.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 60));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 0));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 0));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.AutoSize));
        layout.Controls.Add(new Label { Text = "Bring your X68000 game disks", AutoSize = true, Font = new Font(Font.FontFamily, 18), Margin = new Padding(0, 0, 0, 12) }, 0, 0);
        layout.Controls.Add(new Label { AutoSize = true, MaximumSize = new Size(740, 0), Margin = new Padding(0, 0, 0, 16),
            Text = "Choose your Lagoon X68000 disk ZIP or an extracted folder, then select Set up game. Compatible firmware, the English patch and setup tools download automatically. Your game is prepared and checked on this PC.\n\nInternet access is required for first setup. No administrator access, Python or compiler installation is needed." }, 0, 1);
        disks.PlaceholderText = "Choose your Lagoon X68000 disk ZIP or folder";
        layout.Controls.Add(InputRow("Game disks", disks, true), 0, 2);
        layout.Controls.Add(customFirmware, 0, 3);
        var iplRow = InputRow("IPL firmware (128 KiB)", ipl, false);
        var fontRow = InputRow("CGROM font (768 KiB)", font, false);
        iplRow.Visible = fontRow.Visible = false;
        layout.Controls.Add(iplRow, 0, 4);
        layout.Controls.Add(fontRow, 0, 5);
        customFirmware.CheckedChanged += (_, _) => {
            iplRow.Visible = fontRow.Visible = customFirmware.Checked;
            layout.RowStyles[4].Height = layout.RowStyles[5].Height = customFirmware.Checked ? 60 : 0;
        };
        log.BackColor = Color.FromArgb(0x1B, 0x1E, 0x2B); log.ForeColor = ForeColor;
        log.Text = "Your game files and firmware stay on this PC. Setup can take several minutes.\n";
        layout.Controls.Add(log, 0, 6);
        var buttons = new FlowLayoutPanel { AutoSize = true, Dock = DockStyle.Fill, FlowDirection = FlowDirection.RightToLeft, Margin = new Padding(0, 12, 0, 0) };
        buttons.Controls.Add(cancel); buttons.Controls.Add(build); layout.Controls.Add(buttons, 0, 7);
        Controls.Add(layout); AcceptButton = build; CancelButton = cancel;
        build.Click += async (_, _) => await Build();
        cancel.Click += (_, _) => { if (running != null) running.Cancel(); else { DialogResult = DialogResult.Cancel; Close(); } };
        FormClosing += (_, e) => { if (running != null) { e.Cancel = true; running.Cancel(); } };
    }

    private Control InputRow(string title, TextBox field, bool allowFolder)
    {
        var row = new TableLayoutPanel { Dock = DockStyle.Fill, ColumnCount = allowFolder ? 3 : 2, RowCount = 2, Margin = new Padding(0, 0, 0, 8) };
        row.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
        row.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        if (allowFolder) row.ColumnStyles.Add(new ColumnStyle(SizeType.AutoSize));
        row.Controls.Add(new Label { Text = title, AutoSize = true, Margin = Padding.Empty }, 0, 0);
        row.Controls.Add(field, 0, 1);
        var file = new Button { Text = allowFolder ? "Choose ZIP…" : "Choose file…", AutoSize = true };
        file.Click += (_, _) => {
            using var picker = new OpenFileDialog { Title = title, Filter = allowFolder ? "Disk ZIP (*.zip)|*.zip|All files|*.*" : "Firmware (*.dat)|*.dat|All files|*.*", CheckFileExists = true };
            if (picker.ShowDialog(this) == DialogResult.OK) field.Text = picker.FileName;
        };
        row.Controls.Add(file, 1, 1); pickers.Add(file);
        if (allowFolder)
        {
            var folder = new Button { Text = "Choose folder…", AutoSize = true };
            folder.Click += (_, _) => { using var picker = new FolderBrowserDialog { Description = "Choose your Lagoon X68000 disks folder", UseDescriptionForTitle = true }; if (picker.ShowDialog(this) == DialogResult.OK) field.Text = picker.SelectedPath; };
            row.Controls.Add(folder, 2, 1); pickers.Add(folder);
        }
        return row;
    }

    private async Task Build()
    {
        if (running != null) return;
        try
        {
            var plan = X68000Setup.BuildPlan(adapter.ProjectRoot, disks.Text, customFirmware.Checked ? ipl.Text : null, customFirmware.Checked ? font.Text : null);
            running = new CancellationTokenSource(); build.Enabled = false; cancel.Text = "Cancel setup";
            foreach (var control in pickers.Cast<Control>().Concat(new Control[] { disks, ipl, font, customFirmware })) control.Enabled = false;
            log.AppendText("\nStarting X68000 setup…\n");
            await X68000Setup.RunAsync(plan, line => {
                if (!IsDisposed && IsHandleCreated) BeginInvoke((Action)(() => { log.AppendText(line + Environment.NewLine); log.ScrollToCaret(); }));
            }, running.Token);
            var ready = adapter.CheckReady(adapter.Profiles[0], adapter.ReadSettings(adapter.Profiles[0]).Values);
            if (!ready.Ready) throw new InvalidOperationException(ready.Summary);
            DialogResult = DialogResult.OK;
        }
        catch (OperationCanceledException) { log.AppendText("\nSetup cancelled. You can retry when ready.\n"); }
        catch (Exception error) { log.AppendText("\n" + error.Message + "\n"); log.ScrollToCaret(); }
        finally
        {
            running?.Dispose(); running = null; build.Enabled = true; cancel.Text = "Back";
            foreach (var control in pickers.Cast<Control>().Concat(new Control[] { disks, ipl, font, customFirmware })) control.Enabled = true;
        }
        if (DialogResult == DialogResult.OK) Close();
    }
}
