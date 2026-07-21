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
    private const int NewBtnSize = 28;
    private const int TabBarPadding = 6;

    private readonly Dictionary<int, Rectangle> _closeBtnRects = new();
    private Rectangle _newBtnRect;              // 仅用于光标提示，点击由 _newTabBtn 处理
    private readonly Button _newTabBtn;          // 独立的 + 按钮控件，保证点击 100% 可靠
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

        // 用真正的 Button 控件作为 + 按钮，Click 事件由框架保证可靠
        // 不再依赖 WndProc 拦截 / 坐标计算（TabControl 在 UserPaint 下会吞掉 OnMouseDown）
        _newTabBtn = new Button
        {
            Size = new Size(NewBtnSize, NewBtnSize),
            FlatStyle = FlatStyle.Flat,
            BackColor = Theme.BgButton,
            ForeColor = Theme.FgMain,
            Font = new Font("Microsoft YaHei UI", 13F, FontStyle.Bold),
            Text = "+",
            TextAlign = ContentAlignment.MiddleCenter,
            Cursor = Cursors.Hand,
            Margin = new Padding(0)
        };
        _newTabBtn.FlatAppearance.BorderSize = 0;
        _newTabBtn.FlatAppearance.MouseOverBackColor = Theme.BgButtonHover;
        _newTabBtn.Click += (_, _) =>
        {
            DebugLog.Log("[+Button.Click] 独立按钮被点击，触发 NewTabRequested");
            NewTabRequested?.Invoke(this, EventArgs.Empty);
        };
        // 注意：TabControl.Controls 只能添加 TabPage，不能直接添加 Button。
        // 改为在 OnParentChanged 中把按钮托管到父容器，叠在 TabControl 上方。
    }

    /// <summary>
    /// 当 TabControl 被加入父容器时，把 + 按钮也挂到同一父容器并置顶。
    /// 这样按钮不是 TabControl 的子控件（TabControl 只接受 TabPage），而是兄弟控件。
    /// </summary>
    protected override void OnParentChanged(EventArgs e)
    {
        base.OnParentChanged(e);
        if (Parent is not null)
        {
            if (!Parent.Controls.Contains(_newTabBtn))
                Parent.Controls.Add(_newTabBtn);
            _newTabBtn.BringToFront();
            DebugLog.Log($"[OnParentChanged] +按钮已托管到父容器 {Parent.GetType().Name}");
        }
    }

    protected override void Dispose(bool disposing)
    {
        if (disposing)
        {
            _newTabBtn?.Dispose();
        }
        base.Dispose(disposing);
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

        // 新建按钮（+）：定位独立 Button 控件，不再自绘
        int newX = TabCount > 0 ? GetTabRect(TabCount - 1).Right + 8 : 8;
        int newY = (TabHeight - NewBtnSize) / 2;
        _newBtnRect = new Rectangle(newX, newY, NewBtnSize, NewBtnSize);
        // Button 是父容器的子控件，需把 TabControl 内坐标转换到父容器坐标
        if (_newTabBtn.Parent is not null)
        {
            var parentLoc = _newTabBtn.Parent.PointToClient(PointToScreen(_newBtnRect.Location));
            if (_newTabBtn.Location != parentLoc)
            {
                _newTabBtn.Location = parentLoc;
                _newTabBtn.BringToFront();
            }
        }

        // 位置变化时记录日志
        if (_lastLoggedNewRect != _newBtnRect)
        {
            _lastLoggedNewRect = _newBtnRect;
            DebugLog.Log($"[OnPaint] +按钮位置(本控件内)={_newBtnRect} Button.Bounds={_newTabBtn.Bounds} TabCount={TabCount}");
        }
    }

    private Rectangle _lastLoggedNewRect = Rectangle.Empty;

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
            // 仅在刚进入+按钮区域时记录一次
            if (!prevHoverNew)
            {
                DebugLog.Log($"[OnMouseMove] 进入+按钮区域 鼠标={e.Location} +按钮区域={_newBtnRect}");
            }
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
        const int WM_LBUTTONDOWN = 0x0201;
        const int WM_LBUTTONDBLCLK = 0x0203;
        const int WM_MBUTTONDOWN = 0x0207;

        if (m.Msg == WM_LBUTTONDOWN)
        {
            int l = m.LParam.ToInt32();
            var pt = new Point((short)(l & 0xFFFF), (short)(l >> 16));

            // 关闭按钮：在 WndProc 中拦截，避免触发标签切换
            for (int i = 0; i < TabCount; i++)
                if (_closeBtnRects.TryGetValue(i, out var cr) && cr.Contains(pt)) { TabCloseRequested?.Invoke(this, i); return; }
        }
        else if (m.Msg == WM_LBUTTONDBLCLK)
        {
            int l = m.LParam.ToInt32();
            var pt = new Point((short)(l & 0xFFFF), (short)(l >> 16));
            for (int i = 0; i < TabCount; i++)
            {
                if (_closeBtnRects.TryGetValue(i, out var cr) && cr.Contains(pt)) return;
                var r = GetTabRect(i); r = new Rectangle(r.X, 0, r.Width, TabHeight);
                if (r.Contains(pt)) { TabCloseRequested?.Invoke(this, i); return; }
            }
            return;
        }
        else if (m.Msg == WM_MBUTTONDOWN)
        {
            int l = m.LParam.ToInt32();
            var pt = new Point((short)(l & 0xFFFF), (short)(l >> 16));
            for (int i = 0; i < TabCount; i++)
            {
                var r = GetTabRect(i); r = new Rectangle(r.X, 0, r.Width, TabHeight);
                if (r.Contains(pt)) { TabCloseRequested?.Invoke(this, i); return; }
            }
            return;
        }

        base.WndProc(ref m);
    }
}
