using System.ComponentModel;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace SelectRegionForDbd;

public enum ModernButtonStyle
{
    Default,
    Primary,
    Danger
}

public class ModernButton : Button
{
    private bool hovered;
    private bool pressed;

    [DefaultValue(ModernButtonStyle.Default)]
    public ModernButtonStyle ButtonStyle { get; set; }

    public ModernButton()
    {
        SetStyle(ControlStyles.OptimizedDoubleBuffer
            | ControlStyles.ResizeRedraw
            | ControlStyles.SupportsTransparentBackColor
            | ControlStyles.UserPaint, true);
        BackColor = Themes.Background;
        FlatStyle = FlatStyle.Flat;
        FlatAppearance.BorderSize = 0;
        Cursor = Cursors.Hand;
        Font = new Font("Segoe UI Variable", 10F, FontStyle.Bold);
        Size = new Size(120, 40);
    }

    protected override void OnMouseEnter(EventArgs e)
    {
        base.OnMouseEnter(e);
        hovered = true;
        Invalidate();
    }

    protected override void OnMouseLeave(EventArgs e)
    {
        base.OnMouseLeave(e);
        hovered = pressed = false;
        Invalidate();
    }

    protected override void OnMouseDown(MouseEventArgs mevent)
    {
        base.OnMouseDown(mevent);
        pressed = true;
        Invalidate();
    }

    protected override void OnMouseUp(MouseEventArgs mevent)
    {
        base.OnMouseUp(mevent);
        pressed = false;
        Invalidate();
    }

    protected override void OnEnabledChanged(EventArgs e)
    {
        base.OnEnabledChanged(e);
        Invalidate();
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        Graphics g = e.Graphics;
        g.SmoothingMode = SmoothingMode.AntiAlias;
        g.Clear(Parent?.BackColor ?? Themes.Background);

        Color fill, border, fore;
        if (!Enabled)
        {
            fill = Themes.Surface;
            border = Themes.Border;
            fore = Color.FromArgb(112, 120, 134);
        }
        else if (pressed && ButtonStyle == ModernButtonStyle.Primary)
        {
            fill = Themes.AccentPressed;
            border = Themes.AccentPressed;
            fore = Color.White;
        }
        else if (hovered)
        {
            switch (ButtonStyle)
            {
                case ModernButtonStyle.Primary:
                    fill = Themes.AccentHover;
                    border = Themes.AccentHover;
                    fore = Color.White;
                    break;
                case ModernButtonStyle.Danger:
                    fill = Color.FromArgb(54, 20, 23);
                    border = Themes.Danger;
                    fore = Themes.Danger;
                    break;
                default:
                    fill = Themes.SurfaceAlt;
                    border = Themes.Accent;
                    fore = Themes.Text;
                    break;
            }
        }
        else
        {
            switch (ButtonStyle)
            {
                case ModernButtonStyle.Primary:
                    fill = Themes.Accent;
                    border = Themes.Accent;
                    fore = Color.White;
                    break;
                case ModernButtonStyle.Danger:
                    fill = Color.FromArgb(38, 14, 16);
                    border = Color.FromArgb(92, 36, 40);
                    fore = Themes.Danger;
                    break;
                default:
                    fill = Themes.Surface;
                    border = Themes.Border;
                    fore = Themes.Text;
                    break;
            }
        }

        const int radius = 10;
        using var path = RoundedRect(ClientRectangle, radius);
        using var fillBrush = new SolidBrush(fill);
        using var borderPen = new Pen(border, 1.5f);
        g.FillPath(fillBrush, path);
        g.DrawPath(borderPen, path);

        TextRenderer.DrawText(g, Text, Font, ClientRectangle, fore,
            TextFormatFlags.HorizontalCenter | TextFormatFlags.VerticalCenter | TextFormatFlags.EndEllipsis);
    }

    private static GraphicsPath RoundedRect(Rectangle r, int radius)
    {
        int d = radius * 2;
        var path = new GraphicsPath();
        path.AddArc(r.X, r.Y, d, d, 180, 90);
        path.AddArc(r.Right - d, r.Y, d, d, 270, 90);
        path.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90);
        path.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
        path.CloseFigure();
        return path;
    }
}