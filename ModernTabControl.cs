using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace FileExplorerPro;

/// <summary>
/// 现代化 TabControl：自定义绘制标签头、深色主题、关闭按钮、新建按钮
/// - 标签头：圆角顶部、激活/未激活/悬停三态
/// - 关闭按钮 ×：悬停变红，点击关闭
/// - 新建按钮 +：点击新建空白标签
/// - 支持双击关闭、中键关闭
/// </summary>
public sealed class ModernTabControl : TabControl
{
    private const int TabPadding = 18;
    private const int TabHeight = 32;
    private const int CloseBtnSize = 14;
    private const int NewBtnSize = 24;
    private const int TabBarPadding = 6;

    private readonly Dictionary<int, Rectangle> _closeBtnRects = new();
    private Rectangle _newBtnRect;
    private int _hoverTab = -1;
    private bool _hoverClose;
    private bool _hoverNew;

    public event EventHandler? NewTabRequested;
    public event EventHandler<int>? TabCloseRequested;

    public ModernTabControl()
    {
        SetStyle(ControlStyles.UserPaint |
                 ControlStyles.AllPaintingInWmPaint |
                 ControlStyles.OptimizedDoubleBuffer |
                 ControlStyles.ResizeRedraw, true);

        DrawMode = TabDrawMode.Normal;
        Appearance = TabAppearance.Normal;
        SizeMode = TabSizeMode.Fixed;
        ItemSize = new Size(160, TabHeight);
        Alignment = TabAlignment.Top;
        Padding = new Point(TabPadding, 0);
        Font = Theme.GetUiFont(9);
        DoubleBuffered = true;
    }

    /// <summary>
    /// 完全自定义绘制，不调用基类
    /// </summary>
    protected override void OnPaint(PaintEventArgs e)
    {
        var g = e.Graphics;
        g.SmoothingMode = SmoothingMode.AntiAlias;
        g.TextRenderingHint = System.Drawing.Text.TextRenderingHint.ClearTypeGridFit;

        // 整体背景
        using (var b = new SolidBrush(Theme.BgMain))
            g.FillRectangle(b, ClientRectangle);

        // 标签栏背景
        using (var b = new SolidBrush(Theme.BgTabBar))
            g.FillRectangle(b, 0, 0, Width, TabHeight + 2);

        _closeBtnRects.Clear();

        // 绘制每个标签
        for (int i = 0; i < TabCount; i++)
        {
            var rect = GetTabRect(i);
            // 调整高度到 TabHeight，让标签贴顶
            rect = new Rectangle(rect.X, 0, rect.Width, TabHeight);
            DrawTab(g, i, rect);
        }

        // 新建按钮（+）
        int newX = TabCount > 0 ? GetTabRect(TabCount - 1).Right + 8 : 8;
        _newBtnRect = new Rectangle(newX, (TabHeight - NewBtnSize) / 2, NewBtnSize, NewBtnSize);
        DrawNewButton(g);
    }

    private void DrawTab(Graphics g, int index, Rectangle rect)
    {
        bool active = index == SelectedIndex;
        bool hover = index == _hoverTab;

        Color bg = active ? Theme.BgTabActive : (hover ? Theme.BgTabHover : Theme.BgTabInactive);
        Color fg = active ? Theme.FgMain : Theme.FgTabInactive;

        // 标签底部对齐主区域（激活标签向下延伸 2px 形成连接）
        int bottomExtend = active ? 2 : 0;
        var drawRect = new Rectangle(rect.X, rect.Y, rect.Width, rect.Height + bottomExtend);

        using (var b = new SolidBrush(bg))
            g.FillRectangle(b, drawRect);

        // 激活标签：顶部一条蓝色强调线
        if (active)
        {
            using var p = new Pen(Theme.Accent, 2);
            g.DrawLine(p, rect.X + 4, rect.Y, rect.X + rect.Width - 4, rect.Y);
        }

        // 文字
        var title = TabPages[index].Text;
        var textRect = new Rectangle(rect.X + 10, rect.Y, rect.Width - CloseBtnSize - 20, rect.Height);
        using (var b = new SolidBrush(fg))
        {
            var sf = new StringFormat
            {
                Alignment = StringAlignment.Near,
                LineAlignment = StringAlignment.Center,
                Trimming = StringTrimming.EllipsisCharacter,
                FormatFlags = StringFormatFlags.NoWrap
            };
            g.DrawString(title, Font, b, textRect, sf);
        }

        // 关闭按钮 ×
        int cx = rect.Right - CloseBtnSize - 6;
        int cy = rect.Y + (rect.Height - CloseBtnSize) / 2;
        var closeRect = new Rectangle(cx, cy, CloseBtnSize, CloseBtnSize);
        _closeBtnRects[index] = closeRect;
        DrawCloseButton(g, closeRect, active && _hoverClose);
    }

    private void DrawCloseButton(Graphics g, Rectangle rect, bool hover)
    {
        // 悬停时绘制圆形红色背景
        if (hover)
        {
            using var b = new SolidBrush(Theme.AccentClose);
            g.FillEllipse(b, rect);
        }

        // × 图标
        using var p = new Pen(hover ? Color.White : Theme.FgSecondary, 1.5f);
        int pad = 3;
        g.DrawLine(p, rect.X + pad, rect.Y + pad, rect.Right - pad, rect.Bottom - pad);
        g.DrawLine(p, rect.Right - pad, rect.Y + pad, rect.X + pad, rect.Bottom - pad);
    }

    private void DrawNewButton(Graphics g)
    {
        var rect = _newBtnRect;
        Color bg = _hoverNew ? Theme.BgButtonHover : Color.Transparent;
        if (_hoverNew)
        {
            using var b = new SolidBrush(bg);
            using var path = GetRoundedRectPath(rect, 4);
            g.FillPath(b, path);
        }

        // + 图标
        using var p = new Pen(_hoverNew ? Theme.FgMain : Theme.FgSecondary, 1.8f);
        int cx = rect.X + rect.Width / 2;
        int cy = rect.Y + rect.Height / 2;
        int len = 6;
        g.DrawLine(p, cx - len, cy, cx + len, cy);
        g.DrawLine(p, cx, cy - len, cx, cy + len);
    }

    private static GraphicsPath GetRoundedRectPath(Rectangle rect, int radius)
    {
        var path = new GraphicsPath();
        int d = radius * 2;
        path.AddArc(rect.X, rect.Y, d, d, 180, 90);
        path.AddArc(rect.Right - d, rect.Y, d, d, 270, 90);
        path.AddArc(rect.Right - d, rect.Bottom - d, d, d, 0, 90);
        path.AddArc(rect.X, rect.Bottom - d, d, d, 90, 90);
        path.CloseFigure();
        return path;
    }

    protected override void OnMouseMove(MouseEventArgs e)
    {
        base.OnMouseMove(e);
        int prevHover = _hoverTab;
        bool prevHoverClose = _hoverClose;
        bool prevHoverNew = _hoverNew;

        _hoverTab = -1;
        _hoverClose = false;
        _hoverNew = false;

        for (int i = 0; i < TabCount; i++)
        {
            var rect = GetTabRect(i);
            rect = new Rectangle(rect.X, 0, rect.Width, TabHeight);
            if (rect.Contains(e.Location))
            {
                _hoverTab = i;
                if (_closeBtnRects.TryGetValue(i, out var closeRect) && closeRect.Contains(e.Location))
                {
                    _hoverClose = true;
                }
                break;
            }
        }

        if (_newBtnRect.Contains(e.Location))
        {
            _hoverNew = true;
            Cursor = Cursors.Hand;
        }
        else if (_hoverTab >= 0 && _hoverClose)
        {
            Cursor = Cursors.Hand;
        }
        else
        {
            Cursor = Cursors.Default;
        }

        if (prevHover != _hoverTab || prevHoverClose != _hoverClose || prevHoverNew != _hoverNew)
        {
            Invalidate();
        }
    }

    protected override void OnMouseLeave(EventArgs e)
    {
        base.OnMouseLeave(e);
        _hoverTab = -1;
        _hoverClose = false;
        _hoverNew = false;
        Cursor = Cursors.Default;
        Invalidate();
    }

    protected override void OnMouseClick(MouseEventArgs e)
    {
        base.OnMouseClick(e);

        // 点击新建按钮
        if (_newBtnRect.Contains(e.Location))
        {
            NewTabRequested?.Invoke(this, EventArgs.Empty);
            return;
        }

        // 点击关闭按钮
        for (int i = 0; i < TabCount; i++)
        {
            if (_closeBtnRects.TryGetValue(i, out var closeRect) && closeRect.Contains(e.Location))
            {
                TabCloseRequested?.Invoke(this, i);
                return;
            }
        }

        // 中键点击关闭
        if (e.Button == MouseButtons.Middle)
        {
            for (int i = 0; i < TabCount; i++)
            {
                var rect = GetTabRect(i);
                rect = new Rectangle(rect.X, 0, rect.Width, TabHeight);
                if (rect.Contains(e.Location))
                {
                    TabCloseRequested?.Invoke(this, i);
                    return;
                }
            }
        }
    }

    protected override void OnMouseDoubleClick(MouseEventArgs e)
    {
        // 双击标签关闭（不要双击关闭按钮触发）
        for (int i = 0; i < TabCount; i++)
        {
            if (_closeBtnRects.TryGetValue(i, out var closeRect) && closeRect.Contains(e.Location))
            {
                return; // 关闭按钮双击无动作
            }
            var rect = GetTabRect(i);
            rect = new Rectangle(rect.X, 0, rect.Width, TabHeight);
            if (rect.Contains(e.Location))
            {
                TabCloseRequested?.Invoke(this, i);
                return;
            }
        }
        base.OnMouseDoubleClick(e);
    }

    /// <summary>
    /// 隐藏基类的默认绘制
    /// </summary>
    protected override void OnDrawItem(DrawItemEventArgs e)
    {
        // 由 OnPaint 处理
    }

    protected override void WndProc(ref Message m)
    {
        // 屏蔽系统默认绘制 WM_PAINT 由 OnPaint 接管
        base.WndProc(ref m);

        // 防止系统重绘导致闪烁
        if (m.Msg == 0x000F /* WM_PAINT */)
        {
            // 已经由 OnPaint 绘制
        }
    }
}
