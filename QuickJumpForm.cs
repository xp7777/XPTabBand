using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace FileExplorerPro;

/// <summary>
/// 快速跳转面板：按 Ctrl+D 弹出
/// 模糊匹配：收藏夹 + 最近访问，输入关键字实时筛选
/// 按 Enter 打开选中项，Esc 关闭
/// </summary>
public sealed class QuickJumpForm : Form
{
    private readonly FavoritesService _favorites;
    private readonly TabSessionService _session;

    private readonly TextBox _searchBox;
    private readonly ListBox _resultList;
    private readonly Label _hint;

    private List<JumpEntry> _allEntries = new();
    private List<JumpEntry> _filtered = new();

    public string? SelectedPath { get; private set; }

    public QuickJumpForm(FavoritesService favorites, TabSessionService session)
    {
        _favorites = favorites;
        _session = session;

        FormBorderStyle = FormBorderStyle.None;
        StartPosition = FormStartPosition.CenterParent;
        ClientSize = new Size(560, 420);
        BackColor = Theme.BgQuickJump;
        ShowInTaskbar = false;
        KeyPreview = true;

        // 标题
        var title = new Label
        {
            Text = "  快速跳转",
            Dock = DockStyle.Top,
            Height = 32,
            Font = Theme.GetBoldFont(10),
            ForeColor = Theme.FgMain,
            TextAlign = ContentAlignment.MiddleLeft,
            BackColor = Theme.BgSidebar
        };
        Controls.Add(title);

        // 搜索框
        _searchBox = new TextBox
        {
            Location = new Point(16, 44),
            Size = new Size(528, 32),
            Font = Theme.GetUiFont(11),
            BorderStyle = BorderStyle.FixedSingle,
            BackColor = Theme.BgMain,
            ForeColor = Theme.FgMain,
            PlaceholderText = "输入路径、文件夹名关键字..."
        };
        _searchBox.TextChanged += (_, _) => UpdateFilter();
        _searchBox.KeyDown += (_, e) =>
        {
            if (e.KeyCode == Keys.Down && _filtered.Count > 0)
            {
                _resultList.SelectedIndex = Math.Min(0, _resultList.SelectedIndex + 1);
                _resultList.Focus();
                e.Handled = true;
            }
            else if (e.KeyCode == Keys.Enter)
            {
                ConfirmSelection();
                e.Handled = true;
                e.SuppressKeyPress = true;
            }
            else if (e.KeyCode == Keys.Escape)
            {
                Close();
                e.Handled = true;
            }
        };
        Controls.Add(_searchBox);

        // 结果列表
        _resultList = new ListBox
        {
            Location = new Point(16, 84),
            Size = new Size(528, 280),
            Font = Theme.GetUiFont(10),
            BackColor = Theme.BgMain,
            ForeColor = Theme.FgMain,
            BorderStyle = BorderStyle.None,
            DrawMode = DrawMode.OwnerDrawFixed,
            ItemHeight = 32
        };
        _resultList.DrawItem += (_, e) => DrawResultItem(e);
        _resultList.DoubleClick += (_, _) => ConfirmSelection();
        _resultList.KeyDown += (_, e) =>
        {
            if (e.KeyCode == Keys.Enter)
            {
                ConfirmSelection();
                e.Handled = true;
                e.SuppressKeyPress = true;
            }
            else if (e.KeyCode == Keys.Escape)
            {
                Close();
                e.Handled = true;
            }
        };
        Controls.Add(_resultList);

        // 提示
        _hint = new Label
        {
            Text = "  ↑↓ 选择  ·  Enter 打开  ·  Esc 关闭",
            Dock = DockStyle.Bottom,
            Height = 24,
            Font = Theme.GetSmallFont(8),
            ForeColor = Theme.FgSecondary,
            TextAlign = ContentAlignment.MiddleLeft,
            BackColor = Theme.BgSidebar
        };
        Controls.Add(_hint);

        LoadEntries();

        KeyDown += (_, e) =>
        {
            if (e.KeyCode == Keys.Escape) Close();
        };

        Activated += (_, _) => _searchBox.Focus();
    }

    private void LoadEntries()
    {
        _allEntries.Clear();

        // 收藏夹
        foreach (var fav in _favorites.GetAll())
        {
            _allEntries.Add(new JumpEntry
            {
                Name = fav.Name,
                Path = fav.Path,
                Source = "★ 收藏",
                Group = fav.Group
            });
        }

        // 最近访问
        foreach (var r in _session.LoadRecent())
        {
            if (_allEntries.Any(x => x.Path.Equals(r.Path, StringComparison.OrdinalIgnoreCase))) continue;
            _allEntries.Add(new JumpEntry
            {
                Name = System.IO.Path.GetFileName(r.Path),
                Path = r.Path,
                Source = "🕐 最近",
                Group = "最近访问"
            });
        }

        UpdateFilter();
    }

    private void UpdateFilter()
    {
        string keyword = _searchBox.Text.Trim();
        _filtered.Clear();

        if (string.IsNullOrEmpty(keyword))
        {
            _filtered.AddRange(_allEntries);
        }
        else
        {
            // 模糊匹配：关键字可以是路径任意子串
            foreach (var entry in _allEntries)
            {
                if (entry.Name.IndexOf(keyword, StringComparison.OrdinalIgnoreCase) >= 0 ||
                    entry.Path.IndexOf(keyword, StringComparison.OrdinalIgnoreCase) >= 0)
                {
                    _filtered.Add(entry);
                }
            }
        }

        _resultList.Items.Clear();
        foreach (var e in _filtered) _resultList.Items.Add(e);
        if (_resultList.Items.Count > 0) _resultList.SelectedIndex = 0;
    }

    private void DrawResultItem(DrawItemEventArgs e)
    {
        if (e.Index < 0 || e.Index >= _filtered.Count) return;
        var entry = _filtered[e.Index];

        bool selected = (e.State & DrawItemState.Selected) != 0;
        Color bg = selected ? Theme.BgListItemSelected : (e.Index % 2 == 0 ? Theme.BgMain : Theme.BgListItemAlt);
        using (var b = new SolidBrush(bg))
            e.Graphics.FillRectangle(b, e.Bounds);

        // 来源标记（左侧小色块）
        var tagRect = new Rectangle(e.Bounds.X + 8, e.Bounds.Y + 6, 56, 18);
        Color tagBg = entry.Source.Contains("收藏") ? Theme.AccentStar : Theme.Accent;
        using (var b = new SolidBrush(tagBg))
        using (var path = GetRoundedRect(tagRect, 3))
            e.Graphics.FillPath(b, path);
        using var b2 = new SolidBrush(Color.White);
        var sf = new StringFormat { Alignment = StringAlignment.Center, LineAlignment = StringAlignment.Center };
        e.Graphics.DrawString(entry.Source, Theme.GetSmallFont(7.5f), b2, tagRect, sf);

        // 名称
        var nameRect = new Rectangle(e.Bounds.X + 76, e.Bounds.Y + 2, e.Bounds.Width - 84, 18);
        using var b3 = new SolidBrush(Theme.FgMain);
        var sf2 = new StringFormat { LineAlignment = StringAlignment.Center, Trimming = StringTrimming.EllipsisCharacter, FormatFlags = StringFormatFlags.NoWrap };
        e.Graphics.DrawString(entry.Name, Theme.GetBoldFont(10), b3, nameRect, sf2);

        // 路径
        var pathRect = new Rectangle(e.Bounds.X + 76, e.Bounds.Y + 18, e.Bounds.Width - 84, 14);
        using var b4 = new SolidBrush(Theme.FgSecondary);
        e.Graphics.DrawString(entry.Path, Theme.GetSmallFont(8), b4, pathRect, sf2);
    }

    private static GraphicsPath GetRoundedRect(Rectangle rect, int radius)
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

    private void ConfirmSelection()
    {
        if (_resultList.SelectedIndex < 0 || _resultList.SelectedIndex >= _filtered.Count)
        {
            // 没有选中项时，尝试把搜索框内容当作路径
            string text = _searchBox.Text.Trim();
            if (!string.IsNullOrEmpty(text) && System.IO.Directory.Exists(text))
            {
                SelectedPath = text;
                DialogResult = DialogResult.OK;
                Close();
            }
            return;
        }
        SelectedPath = _filtered[_resultList.SelectedIndex].Path;
        DialogResult = DialogResult.OK;
        Close();
    }

    protected override void OnPaint(PaintEventArgs e)
    {
        // 绘制边框
        using var p = new Pen(Theme.Border, 1);
        e.Graphics.DrawRectangle(p, 0, 0, Width - 1, Height - 1);
        base.OnPaint(e);
    }

    private sealed class JumpEntry
    {
        public string Name { get; set; } = "";
        public string Path { get; set; } = "";
        public string Source { get; set; } = "";
        public string Group { get; set; } = "";

        public override string ToString() => Name;
    }
}
