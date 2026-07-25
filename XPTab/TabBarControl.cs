using System;
using System.Collections.Generic;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace XPTab
{
    /// <summary>
    /// 标签页栏 UI 控件：显示多个标签 + + 号按钮 + 关闭按钮
    /// 嵌入 Explorer Band 中显示
    /// </summary>
    // ComVisible(false)：这是内部 WinForms 控件，不应被 regasm 注册为 COM 组件。
    // 之前 regasm 把它注册成 COM 导致"管理加载项"里出现多余的 XPTab 条目。
    [ComVisible(false)]
    public class TabBarControl : UserControl
    {
        private readonly List<TabInfo> _tabs = new();
        private int _selectedIndex = -1;
        private readonly Button _btnNewTab;
        private int _hoverIndex = -1;
        private int _closeHoverIndex = -1;

        // 标签页尺寸常量
        private const int TabMinWidth = 120;
        private const int TabMaxWidth = 200;
        private const int TabHeight = 26;
        private const int TabGap = 1;
        private const int NewButtonSize = 22;
        private const int CloseButtonSize = 14;

        /// <summary>新建标签页请求</summary>
        public event EventHandler NewTabRequested;

        /// <summary>选中标签页变化</summary>
        public event EventHandler<int> SelectedTabChanged;

        /// <summary>关闭标签页请求</summary>
        public event EventHandler<int> CloseTabRequested;

        public TabBarControl()
        {
            Dock = DockStyle.Fill;
            BackColor = Color.FromArgb(43, 43, 43);
            DoubleBuffered = true;
            SetStyle(ControlStyles.AllPaintingInWmPaint | ControlStyles.UserPaint | ControlStyles.ResizeRedraw, true);

            // + 号按钮：标准 Button 控件，确保点击可靠
            _btnNewTab = new Button
            {
                Size = new Size(NewButtonSize, NewButtonSize),
                FlatStyle = FlatStyle.Flat,
                BackColor = Color.FromArgb(60, 60, 60),
                ForeColor = Color.White,
                Font = new Font("Segoe UI", 11F, FontStyle.Bold),
                Text = "+",
                TextAlign = ContentAlignment.MiddleCenter,
                Cursor = Cursors.Hand
            };
            _btnNewTab.FlatAppearance.BorderSize = 0;
            _btnNewTab.FlatAppearance.MouseOverBackColor = Color.FromArgb(80, 80, 80);
            _btnNewTab.Click += (_, _) => NewTabRequested?.Invoke(this, EventArgs.Empty);
            Controls.Add(_btnNewTab);

            // 默认添加一个标签页
            AddTab("资源管理器");
        }

        /// <summary>添加标签页</summary>
        public void AddTab(string title)
        {
            _tabs.Add(new TabInfo { Title = title });
            SelectedIndex = _tabs.Count - 1;
            Invalidate();
        }

        /// <summary>当前选中的标签索引</summary>
        public int SelectedIndex
        {
            get => _selectedIndex;
            set
            {
                if (value >= 0 && value < _tabs.Count && _selectedIndex != value)
                {
                    _selectedIndex = value;
                    SelectedTabChanged?.Invoke(this, value);
                    Invalidate();
                }
            }
        }

        protected override void OnResize(EventArgs e)
        {
            base.OnResize(e);
            // + 号按钮定位：所有标签之后
            int x = GetTabRight(_tabs.Count);
            _btnNewTab.Location = new Point(x + 4, (Height - NewButtonSize) / 2);
            Invalidate();
        }

        /// <summary>计算第 index 个标签的右边界</summary>
        private int GetTabRight(int index)
        {
            int x = 2;
            for (int i = 0; i < index && i < _tabs.Count; i++)
            {
                x += GetTabWidth(i) + TabGap;
            }
            return x;
        }

        /// <summary>单个标签宽度（根据标题文字自适应，限制范围）</summary>
        private int GetTabWidth(int index)
        {
            if (index < 0 || index >= _tabs.Count) return TabMinWidth;
            using (var g = CreateGraphics())
            {
                var size = TextRenderer.MeasureText(g, _tabs[index].Title, Font);
                return Math.Max(TabMinWidth, Math.Min(TabMaxWidth, size.Width + 40));
            }
        }

        /// <summary>根据 X 坐标命中标签索引，返回 -1 表示未命中</summary>
        private int HitTestTab(int x)
        {
            int left = 2;
            for (int i = 0; i < _tabs.Count; i++)
            {
                int w = GetTabWidth(i);
                if (x >= left && x < left + w) return i;
                left += w + TabGap;
            }
            return -1;
        }

        /// <summary>判断 X 是否在某标签的关闭按钮区域内</summary>
        private bool HitTestClose(int tabIndex, int x, int y)
        {
            if (tabIndex < 0 || tabIndex >= _tabs.Count) return false;
            int tabRight = GetTabRight(tabIndex + 1);
            var closeRect = new Rectangle(tabRight - CloseButtonSize - 8,
                (Height - CloseButtonSize) / 2, CloseButtonSize, CloseButtonSize);
            return closeRect.Contains(x, y);
        }

        protected override void OnMouseMove(MouseEventArgs e)
        {
            base.OnMouseMove(e);
            int hit = HitTestTab(e.X);
            int closeHit = HitTestClose(hit, e.X, e.Y) ? hit : -1;
            if (hit != _hoverIndex || closeHit != _closeHoverIndex)
            {
                _hoverIndex = hit;
                _closeHoverIndex = closeHit;
                Invalidate();
                Cursor = hit >= 0 ? Cursors.Hand : Cursors.Default;
            }
        }

        protected override void OnMouseLeave(EventArgs e)
        {
            base.OnMouseLeave(e);
            if (_hoverIndex >= 0 || _closeHoverIndex >= 0)
            {
                _hoverIndex = -1;
                _closeHoverIndex = -1;
                Invalidate();
            }
        }

        protected override void OnMouseClick(MouseEventArgs e)
        {
            base.OnMouseClick(e);
            if (e.Button != MouseButtons.Left) return;
            int hit = HitTestTab(e.X);
            if (hit < 0) return;
            // 优先判断关闭按钮
            if (HitTestClose(hit, e.X, e.Y))
            {
                CloseTabRequested?.Invoke(this, hit);
                if (_tabs.Count > 1)
                {
                    _tabs.RemoveAt(hit);
                    if (_selectedIndex >= _tabs.Count) _selectedIndex = _tabs.Count - 1;
                    OnResize(EventArgs.Empty);
                    Invalidate();
                }
                return;
            }
            // 选中标签
            if (hit != _selectedIndex)
            {
                _selectedIndex = hit;
                SelectedTabChanged?.Invoke(this, hit);
                Invalidate();
            }
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            var g = e.Graphics;
            g.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.AntiAlias;

            int left = 2;
            for (int i = 0; i < _tabs.Count; i++)
            {
                int w = GetTabWidth(i);
                var rect = new Rectangle(left, 2, w, Height - 4);
                bool selected = i == _selectedIndex;
                bool hover = i == _hoverIndex;

                // 背景
                Color bg = selected ? Color.FromArgb(62, 62, 62) :
                           hover ? Color.FromArgb(52, 52, 52) :
                           Color.FromArgb(43, 43, 43);
                using (var b = new SolidBrush(bg))
                    g.FillRectangle(b, rect);

                // 选中标签顶部蓝色高亮条
                if (selected)
                {
                    using (var b = new SolidBrush(Color.FromArgb(0, 120, 215)))
                        g.FillRectangle(b, rect.X, rect.Y, rect.Width, 2);
                }

                // 标题文字
                TextRenderer.DrawText(g, _tabs[i].Title, Font,
                    new Rectangle(rect.X + 8, rect.Y, rect.Width - 28, rect.Height),
                    Color.White, TextFormatFlags.Left | TextFormatFlags.VerticalCenter);

                // 关闭按钮（×）
                var closeRect = new Rectangle(rect.Right - CloseButtonSize - 8,
                    (rect.Height - CloseButtonSize) / 2 + rect.Y, CloseButtonSize, CloseButtonSize);
                Color closeColor = (i == _closeHoverIndex) ? Color.White : Color.FromArgb(180, 180, 180);
                using (var pen = new Pen(closeColor, 1.5f))
                {
                    g.DrawLine(pen, closeRect.Left + 3, closeRect.Top + 3, closeRect.Right - 3, closeRect.Bottom - 3);
                    g.DrawLine(pen, closeRect.Right - 3, closeRect.Top + 3, closeRect.Left + 3, closeRect.Bottom - 3);
                }

                left += w + TabGap;
            }
        }
    }

    /// <summary>单个标签页信息</summary>
    internal class TabInfo
    {
        public string Title { get; set; } = "";
    }
}
