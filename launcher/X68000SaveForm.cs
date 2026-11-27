using System.Text.Json;

internal sealed class X68000SaveForm : Form
{
    private readonly X68000Adapter adapter;
    private readonly ComboBox source = Picker();
    private readonly ComboBox sourceSlot = Picker();
    private readonly ComboBox destinationSlot = Picker();
    private readonly Button copy = new() { Text = "Import save", AutoSize = true, Enabled = false };
    private readonly Label status = new() { Text = "Choose an earlier saved game and an empty slot in X68000 Enhanced.", AutoSize = true, MaximumSize = new Size(560, 0) };

    public X68000SaveForm(X68000Adapter adapter)
    {
        this.adapter = adapter;
        Text = "Import an X68000 save";
        StartPosition = FormStartPosition.CenterParent;
        FormBorderStyle = FormBorderStyle.FixedDialog;
        MaximizeBox = MinimizeBox = false;
        AutoSize = true;
        AutoSizeMode = AutoSizeMode.GrowAndShrink;
        Font = new Font("Segoe UI", 10F);

        var close = new Button { Text = "Close", AutoSize = true, DialogResult = DialogResult.Cancel };
        var refresh = new Button { Text = "Refresh slots", AutoSize = true };
        CancelButton = close;
        var panel = new TableLayoutPanel { AutoSize = true, Padding = new Padding(24), ColumnCount = 2 };
        panel.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 280));
        panel.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 280));
        var heading = new Label { Text = "Import an in-game save", Font = new Font("Segoe UI", 16F), AutoSize = true };
        var instructions = new Label {
            Text = "Close both games first. If X68000 Enhanced is new, start it once to create its User Disk.",
            AutoSize = true, MaximumSize = new Size(560, 0), Margin = new Padding(3, 8, 3, 18)
        };
        panel.Controls.Add(heading, 0, 0);
        panel.SetColumnSpan(heading, 2);
        panel.Controls.Add(instructions, 0, 1);
        panel.SetColumnSpan(instructions, 2);
        panel.Controls.Add(new Label { Text = "Copy from", AutoSize = true }, 0, 2);
        panel.Controls.Add(new Label { Text = "Copy to", AutoSize = true }, 1, 2);
        panel.Controls.Add(source, 0, 3);
        panel.Controls.Add(new Label { Text = "X68000 Enhanced", AutoSize = true }, 1, 3);
        panel.Controls.Add(sourceSlot, 0, 4);
        panel.Controls.Add(destinationSlot, 1, 4);
        status.Margin = new Padding(3, 16, 3, 12);
        panel.Controls.Add(status, 0, 5);
        panel.SetColumnSpan(status, 2);
        var buttons = new FlowLayoutPanel { AutoSize = true, FlowDirection = FlowDirection.RightToLeft, Dock = DockStyle.Fill };
        buttons.Controls.AddRange(new Control[] { close, copy, refresh });
        panel.Controls.Add(buttons, 0, 6);
        panel.SetColumnSpan(buttons, 2);
        Controls.Add(panel);

        source.DisplayMember = nameof(X68000Adapter.SaveSource.Title);
        source.Items.AddRange(X68000Adapter.LegacySaveSources.Cast<object>().ToArray());
        source.SelectedIndex = source.Items.Count - 1;
        source.SelectedIndexChanged += (_, _) => Reload();
        sourceSlot.SelectedIndexChanged += (_, _) => UpdateCopy();
        destinationSlot.SelectedIndexChanged += (_, _) => UpdateCopy();
        refresh.Click += (_, _) => Reload();
        copy.Click += (_, _) => Copy();
        Shown += (_, _) => Reload();
    }

    private static ComboBox Picker() => new() { DropDownStyle = ComboBoxStyle.DropDownList, Width = 248 };

    private X68000SaveDisk SourceDisk() => adapter.OpenImportSource((X68000Adapter.SaveSource)source.SelectedItem!);

    private void UpdateCopy() => copy.Enabled = sourceSlot.SelectedItem is X68000SaveDisk.Slot { Level: not null }
        && destinationSlot.SelectedItem is X68000SaveDisk.Slot { Level: null };

    private static bool SaveError(Exception error) =>
        error is IOException or InvalidDataException or InvalidOperationException or UnauthorizedAccessException or JsonException;

    private void Reload()
    {
        sourceSlot.Items.Clear();
        destinationSlot.Items.Clear();
        UpdateCopy();
        var side = "Source";
        try
        {
            var from = SourceDisk().Inspect().Where(slot => slot.Level.HasValue).ToArray();
            side = "Destination";
            var to = adapter.OpenEnhancedSaveDisk().Inspect().Where(slot => !slot.Level.HasValue).ToArray();
            sourceSlot.Items.AddRange(from);
            destinationSlot.Items.AddRange(to);
            if (from.Length > 0) sourceSlot.SelectedIndex = 0;
            if (to.Length > 0) destinationSlot.SelectedIndex = 0;
            status.Text = from.Length == 0 ? "Save in the source game first."
                : to.Length == 0 ? "The destination has no empty slots."
                : "The original save stays in place. Press Y on your controller in X68000 Enhanced to load the imported slot.";
        }
        catch (Exception error) when (SaveError(error)) { status.Text = $"{side}: {error.Message}"; }
    }

    private void Copy()
    {
        if (sourceSlot.SelectedItem is not X68000SaveDisk.Slot from ||
            destinationSlot.SelectedItem is not X68000SaveDisk.Slot to) return;
        try
        {
            SourceDisk().CopySlotTo(from.Number, adapter.OpenEnhancedSaveDisk(), to.Number);
            Reload();
            status.Text = $"Imported to slot {to.Number} in X68000 Enhanced.";
        }
        catch (Exception error) when (SaveError(error))
        {
            Reload();
            status.Text = error.Message;
        }
    }
}
