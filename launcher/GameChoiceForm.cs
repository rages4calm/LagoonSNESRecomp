// First-run picker for the two Companion game choices and the X68000 save tool.
internal sealed class GameChoiceForm : Form
{
    internal bool VisualCheck { get; set; }
    protected override bool ShowWithoutActivation => VisualCheck || base.ShowWithoutActivation;
    public string Choice { get; private set; } = GameChoice.Ask;
    private readonly CheckBox remember;

    public GameChoiceForm()
    {
        Text = "Lagoon Companion";
        FormBorderStyle = FormBorderStyle.FixedDialog;
        StartPosition = FormStartPosition.CenterScreen;
        MaximizeBox = false;
        MinimizeBox = false;
        BackColor = Color.FromArgb(0x10, 0x12, 0x1A);
        ForeColor = Color.FromArgb(0xF1, 0xF2, 0xF7);
        ClientSize = new Size(600, 424);

        var title = new Label
        {
            Text = "Which Lagoon do you want to play?",
            Font = new Font("Segoe UI", 15F, FontStyle.Regular),
            AutoSize = true,
            Location = new Point(36, 30),
            BackColor = Color.Transparent,
            ForeColor = Color.FromArgb(0xF1, 0xF2, 0xF7),
        };
        var subtitle = new Label
        {
            Text = "The two versions are separate games: different engines, saves and settings.",
            Font = new Font("Segoe UI", 9.5F),
            AutoSize = true,
            Location = new Point(38, 70),
            BackColor = Color.Transparent,
            ForeColor = Color.FromArgb(0xA3, 0xA8, 0xBB),
        };

        var snes = ChoiceCard("SNES Enhanced",
            "Native SNES game · widescreen and quality of life",
            Color.FromArgb(0xAD, 0x8D, 0xFF), () => Pick(GameChoice.Snes));
        snes.Location = new Point(36, 108);

        var x68000 = ChoiceCard("X68000 Enhanced",
            "Sharp X68000 floppy edition · English translation",
            Color.FromArgb(0x5F, 0xD3, 0xB8), () => Pick(GameChoice.X68000));
        x68000.Location = new Point(36, 216);

        var saves = new Button {
            Text = "Copy X68000 in-game saves…", AutoSize = true, Location = new Point(36, 328),
            FlatStyle = FlatStyle.Flat, BackColor = x68000.BackColor, ForeColor = x68000.ForeColor
        };
        saves.Click += (_, _) =>
        {
            using var dialog = new X68000SaveForm(new X68000Adapter()) { Icon = Icon };
            dialog.ShowDialog(this);
        };
        remember = new CheckBox
        {
            Text = "Remember this edition (use Choose edition to switch)",
            AutoSize = true,
            Location = new Point(38, 382),
            BackColor = Color.Transparent,
            ForeColor = Color.FromArgb(0xA3, 0xA8, 0xBB),
        };

        Controls.AddRange(new Control[] { title, subtitle, snes, x68000, saves, remember });
    }

    private Button ChoiceCard(string heading, string detail, Color accent, Action pick)
    {
        var card = new Button
        {
            Text = $"{heading}\n{detail}",
            TextAlign = ContentAlignment.MiddleLeft,
            Size = new Size(528, 96),
            FlatStyle = FlatStyle.Flat,
            BackColor = Color.FromArgb(0x1B, 0x1E, 0x2B),
            ForeColor = Color.FromArgb(0xF1, 0xF2, 0xF7),
            Font = new Font("Segoe UI", 10.5F),
            Padding = new Padding(20, 10, 12, 10),
        };
        card.FlatAppearance.BorderColor = accent;
        card.FlatAppearance.BorderSize = 1;
        card.Click += (_, _) => pick();
        return card;
    }

    private void Pick(string choice)
    {
        Choice = choice;
        if (remember.Checked) GameChoice.Save(choice);
        DialogResult = DialogResult.OK;
        Close();
    }
}
