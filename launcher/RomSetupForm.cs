using Companion.Launcher.Core;

// A deliberately small first-run flow: pick a file, verify, prepare, continue.
internal sealed class RomSetupForm : Form
{
    private readonly GameAdapter adapter;
    private readonly Label status;
    private readonly Button choose;
    private bool busy;

    internal RomSetupForm(GameAdapter adapter)
    {
        this.adapter = adapter;
        Text = "Welcome to Lagoon";
        ClientSize = new Size(620, 330);
        MinimumSize = new Size(580, 330);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Color.FromArgb(16, 18, 26);
        ForeColor = Color.WhiteSmoke;
        Font = new Font("Segoe UI", 11);
        var layout = new TableLayoutPanel { Dock = DockStyle.Fill, Padding = new Padding(28), RowCount = 4, ColumnCount = 1 };
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 42));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 80));
        layout.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
        layout.RowStyles.Add(new RowStyle(SizeType.Absolute, 48));
        layout.Controls.Add(new Label { Text = "Choose your Lagoon ROM", Font = new Font(Font, FontStyle.Bold), Dock = DockStyle.Fill, AutoSize = false });
        layout.Controls.Add(new Label { Text = "Select your original Lagoon USA .sfc or .smc file. If it is in a ZIP, extract it first. Choose the file itself, not its folder.\nYour ROM stays where it is. No downloads or compiler needed.", Dock = DockStyle.Fill });
        status = new Label { Text = "We will check your ROM and prepare the game automatically.", Dock = DockStyle.Fill };
        layout.Controls.Add(status);
        choose = new Button { Text = "Choose ROM file…", Dock = DockStyle.Fill, FlatStyle = FlatStyle.Flat, BackColor = Color.FromArgb(173, 141, 255), ForeColor = Color.Black };
        choose.Click += async (_, _) => await ChooseAsync();
        layout.Controls.Add(choose);
        Controls.Add(layout);
        FormClosing += (_, e) => { if (busy) e.Cancel = true; };
    }

    private async Task ChooseAsync()
    {
        using var picker = new OpenFileDialog { Title = "Choose the original Lagoon USA ROM file", Filter = "SNES ROM (*.sfc;*.smc)|*.sfc;*.smc|All files (*.*)|*.*", CheckFileExists = true, Multiselect = false };
        if (picker.ShowDialog(this) != DialogResult.OK) return;
        busy = true; choose.Enabled = false;
        status.Text = "Verifying ROM and preparing Lagoon…";
        try
        {
            await adapter.ImportRomAsync(picker.FileName, new Progress<OperationProgress>(p => status.Text = p.Detail), CancellationToken.None);
            busy = false; DialogResult = DialogResult.OK; Close();
        }
        catch (Exception error) { status.Text = error.Message; }
        finally { busy = false; choose.Enabled = true; }
    }
}
