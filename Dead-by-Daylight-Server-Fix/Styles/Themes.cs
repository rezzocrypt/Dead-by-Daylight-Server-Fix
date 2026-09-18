using System.Windows.Forms;

namespace SelectRegionForDbd;

public static class Themes
{
    public static readonly Color Background = Color.FromArgb(17, 21, 28);
    public static readonly Color Surface = Color.FromArgb(26, 33, 42);
    public static readonly Color SurfaceAlt = Color.FromArgb(34, 44, 56);
    public static readonly Color Field = Color.FromArgb(11, 14, 19);
    public static readonly Color Border = Color.FromArgb(42, 52, 66);
    public static readonly Color Text = Color.FromArgb(232, 236, 241);
    public static readonly Color TextMuted = Color.FromArgb(152, 162, 179);
    public static readonly Color Accent = Color.FromArgb(61, 126, 255);
    public static readonly Color AccentHover = Color.FromArgb(86, 140, 255);
    public static readonly Color AccentPressed = Color.FromArgb(43, 103, 215);
    public static readonly Color Success = Color.FromArgb(46, 184, 114);
    public static readonly Color Warning = Color.FromArgb(245, 166, 35);
    public static readonly Color Danger = Color.FromArgb(229, 72, 77);

    public static void Apply(Form form)
    {
        form.BackColor = Background;
        form.ForeColor = Text;

        foreach (Control control in form.Controls)
        {
            ApplyControl(control);
        }
    }

    private static void ApplyControl(Control control)
    {
        switch (control)
        {
            case ModernButton:
                break;
            case DataGridView grid:
                ApplyGrid(grid);
                break;
            case StatusStrip statusStrip:
                statusStrip.BackColor = Background;
                foreach (ToolStripItem item in statusStrip.Items)
                {
                    if (item is ToolStripStatusLabel label && label.ForeColor == SystemColors.ControlText)
                    {
                        label.ForeColor = Text;
                    }
                }
                break;
            case TextBox textBox:
                textBox.BackColor = Field;
                textBox.ForeColor = Text;
                break;
            case ComboBox comboBox:
                comboBox.BackColor = Field;
                comboBox.ForeColor = Text;
                break;
            case Button button:
                button.BackColor = Surface;
                button.FlatStyle = FlatStyle.Flat;
                break;
        }
    }

    private static void ApplyGrid(DataGridView grid)
    {
        grid.EnableHeadersVisualStyles = false;
        grid.BackgroundColor = Field;
        grid.BorderStyle = BorderStyle.FixedSingle;
        grid.GridColor = Border;
        grid.ColumnHeadersHeight = 40;
        grid.ColumnHeadersDefaultCellStyle.BackColor = Surface;
        grid.ColumnHeadersDefaultCellStyle.ForeColor = TextMuted;
        grid.ColumnHeadersDefaultCellStyle.SelectionBackColor = Surface;
        grid.ColumnHeadersDefaultCellStyle.SelectionForeColor = TextMuted;
        grid.ColumnHeadersDefaultCellStyle.Font = new Font("Segoe UI", 9F, FontStyle.Bold);
        grid.ColumnHeadersBorderStyle = DataGridViewHeaderBorderStyle.Single;
        grid.CellBorderStyle = DataGridViewCellBorderStyle.SingleHorizontal;
        grid.DefaultCellStyle.BackColor = Background;
        grid.DefaultCellStyle.ForeColor = Text;
        grid.DefaultCellStyle.SelectionBackColor = Color.FromArgb(31, 58, 103);
        grid.DefaultCellStyle.SelectionForeColor = Color.White;
        grid.DefaultCellStyle.Font = new Font("Segoe UI", 9.5F);
    }
}