using System.Drawing;
using System.Drawing.Drawing2D;
using System.Windows.Forms;

namespace FileExplorerPro;

/// <summary>
/// 主窗体：左侧收藏夹侧栏 + 顶部多标签 + Ctrl+D 快速跳转
/// </summary>
public sealed class MainForm : Form
{
    private readonly AppSettings _settings;
    private readonly FavoritesService _favorites;
    private readonly TabSessionService _session;

    private readonly ModernTabControl _tabControl;
    private readonly Panel _sidebar;
    private readonly TreeView _favoritesTree;
    private readonly SplitContainer _splitter;
    private readonly Label _sidebarTitle;

    public MainForm(AppSettings settings)
    {
        _settings = settings;
        _favorites = new FavoritesService();
        _session = new TabSessionService(_settings.MaxRecentPaths);

        Text = "FileExplorerPro";
        ClientSize = new Size(_settings.WindowWidth, _settings.WindowHeight);
        StartPosition = FormStartPosition.CenterScreen;
        BackColor = Theme.BgMain;
        ForeColor = Theme.FgMain;
        Font = Theme.GetUiFont(_settings.FontSize);
        MinimumSize = new Size(800, 500);
        KeyPreview = true;

        // ===== 整体布局：左右分栏 =====
        _splitter = new SplitContainer
        {
            Dock = DockStyle.Fill,
            FixedPanel = FixedPanel.Panel1,
            SplitterWidth = 1,
            SplitterDistance = _settings.SidebarWidth,
            BackColor = Theme.Border
        };

        // ===== 左侧：收藏夹侧栏 =====
        _sidebar = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Theme.BgSidebar,
            Padding = new Padding(0)
        };

        _sidebarTitle = new Label
        {
            Text = "  收藏夹",
            Dock = DockStyle.Top,
            Height = 32,
            Font = Theme.GetBoldFont(10),
            ForeColor = Theme.FgMain,
            TextAlign = ContentAlignment.MiddleLeft,
            BackColor = Theme.BgSidebar
        };

        _favoritesTree = new TreeView
        {
            Dock = DockStyle.Fill,
            BackColor = Theme.BgSidebar,
            ForeColor = Theme.FgMain,
            Font = Theme.GetUiFont(9),
            BorderStyle = BorderStyle.None,
            ShowLines = false,
            ShowPlusMinus = false,
            ShowRootLines = false,
            HideSelection = false,
            DrawMode = TreeViewDrawMode.OwnerDrawText,
            ItemHeight = 28,
            Indent = 8
        };
        _favoritesTree.DrawNode += (_, e) => DrawFavoriteNode(e);
        _favoritesTree.NodeMouseClick += (_, e) =>
        {
            if (e.Node?.Tag is string path && e.Button == MouseButtons.Left)
            {
                OpenPathInCurrentTab(path);
            }
        };
        _favoritesTree.NodeMouseDoubleClick += (_, e) =>
        {
            if (e.Node?.Tag is string path)
            {
                OpenPathInCurrentTab(path);
            }
        };

        _sidebar.Controls.Add(_favoritesTree);
        _sidebar.Controls.Add(_sidebarTitle);

        // 右键收藏夹管理
        _favoritesTree.MouseClick += (_, e) =>
        {
            if (e.Button == MouseButtons.Right)
            {
                var hit = _favoritesTree.GetNodeAt(e.Location);
                if (hit?.Tag is string path && path != "GROUP")
                {
                    ShowFavoriteContextMenu(hit, path, _favoritesTree.PointToScreen(e.Location));
                }
            }
        };

        // ===== 右侧：多标签区 =====
        var rightPanel = new Panel { Dock = DockStyle.Fill, BackColor = Theme.BgMain };
        _tabControl = new ModernTabControl
        {
            Dock = DockStyle.Fill
        };
        _tabControl.NewTabRequested += (_, _) => AddNewTab("");
        _tabControl.TabCloseRequested += (_, idx) => CloseTab(idx);
        _tabControl.SelectedIndexChanged += (_, _) => OnTabChanged();
        rightPanel.Controls.Add(_tabControl);

        _splitter.Panel1.Controls.Add(_sidebar);
        _splitter.Panel2.Controls.Add(rightPanel);
        Controls.Add(_splitter);

        // 收藏夹变化时刷新侧栏
        _favorites.Changed += (_, _) => RefreshFavorites();
        RefreshFavorites();

        // 初始化标签页
        InitializeTabs();

        // 全局快捷键
        KeyDown += MainForm_KeyDown;

        // 关闭时保存状态
        FormClosing += MainForm_FormClosing;
        Load += (_, _) => Program.EnableDarkTitleBar(Handle);
    }

    private void InitializeTabs()
    {
        bool restored = false;
        if (_settings.RestoreTabsOnStart)
        {
            var savedTabs = _session.LoadTabs();
            if (savedTabs.Count > 0)
            {
                foreach (var t in savedTabs)
                {
                    AddNewTab(t.Path, t.Title);
                }
                if (savedTabs.Any(t => t.Active))
                {
                    var active = savedTabs.First(t => t.Active);
                    int idx = savedTabs.IndexOf(active);
                    if (idx >= 0 && idx < _tabControl.TabCount) _tabControl.SelectedIndex = idx;
                }
                restored = true;
            }
        }

        if (!restored)
        {
            string startup = string.IsNullOrEmpty(_settings.StartupPath)
                ? Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)
                : _settings.StartupPath;
            AddNewTab(startup);
        }
    }

    /// <summary>
    /// 添加新标签页
    /// </summary>
    public void AddNewTab(string path, string? title = null)
    {
        var page = new TabPage
        {
            BackColor = Theme.BgMain,
            ForeColor = Theme.FgMain,
            Padding = new Padding(0)
        };

        var content = new FileTabUserControl(_settings, _favorites);
        content.PathChanged += (_, p) =>
        {
            page.Text = GetTabTitle(p);
            if (!string.IsNullOrEmpty(p) && p != "ThisPC")
            {
                _session.AddRecent(p);
            }
        };
        content.FavoriteStateChanged += (_, _) => RefreshFavorites();

        page.Controls.Add(content);
        page.Text = title ?? GetTabTitle(path);
        _tabControl.TabPages.Add(page);
        _tabControl.SelectedTab = page;

        if (!string.IsNullOrEmpty(path))
        {
            content.NavigateTo(path);
        }
        else
        {
            // 空白标签：默认打开文档目录
            content.NavigateTo(Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments));
        }
    }

    private static string GetTabTitle(string path)
    {
        if (string.IsNullOrEmpty(path) || path == "ThisPC") return "此电脑";
        try
        {
            return System.IO.Path.GetFileName(path);
        }
        catch
        {
            return path;
        }
    }

    private void CloseTab(int index)
    {
        if (index < 0 || index >= _tabControl.TabCount) return;
        // 至少保留一个标签
        if (_tabControl.TabCount == 1)
        {
            // 最后一个标签关闭 = 退出程序？还是替换为新标签？
            // 这里选择：替换为空白标签
            AddNewTab("");
        }
        _tabControl.TabPages.RemoveAt(index);
    }

    private void OnTabChanged()
    {
        if (_tabControl.SelectedTab?.Controls[0] is FileTabUserControl content)
        {
            // 记录最近访问
            if (!string.IsNullOrEmpty(content.CurrentPath) && content.CurrentPath != "ThisPC")
            {
                _session.AddRecent(content.CurrentPath);
            }
        }
    }

    /// <summary>
    /// 在当前激活标签打开路径
    /// </summary>
    public void OpenPathInCurrentTab(string path)
    {
        if (_tabControl.SelectedTab?.Controls[0] is FileTabUserControl content)
        {
            content.NavigateTo(path);
        }
        else if (_tabControl.TabCount == 0)
        {
            AddNewTab(path);
        }
    }

    /// <summary>
    /// 刷新收藏夹侧栏
    /// </summary>
    private void RefreshFavorites()
    {
        _favoritesTree.BeginUpdate();
        _favoritesTree.Nodes.Clear();

        var grouped = _favorites.GetGrouped();
        foreach (var grp in grouped)
        {
            var groupNode = new TreeNode(grp.Key)
            {
                Tag = "GROUP",
                NodeFont = Theme.GetBoldFont(9),
                ForeColor = Theme.FgSecondary
            };
            foreach (var item in grp.Value)
            {
                var node = new TreeNode(item.Name)
                {
                    Tag = item.Path,
                    NodeFont = Theme.GetUiFont(9),
                    ForeColor = Theme.FgMain
                };
                groupNode.Nodes.Add(node);
            }
            groupNode.Expand();
            _favoritesTree.Nodes.Add(groupNode);
        }

        _favoritesTree.EndUpdate();
    }

    private void DrawFavoriteNode(DrawTreeNodeEventArgs e)
    {
        if (e.Node is null) return;
        bool isGroup = e.Node.Tag is string s && s == "GROUP";
        bool selected = _favoritesTree.SelectedNode == e.Node;
        bool hover = (e.State & TreeNodeStates.Hot) != 0;

        Color bg = Theme.BgSidebar;
        if (selected) bg = Theme.BgListItemSelected;
        else if (hover) bg = Theme.BgListItemHover;

        using (var b = new SolidBrush(bg))
            e.Graphics.FillRectangle(b, e.Bounds);

        // 文字
        Color fg = isGroup ? Theme.FgSecondary : (selected ? Color.White : Theme.FgMain);
        using var font = isGroup ? Theme.GetBoldFont(9) : Theme.GetUiFont(9);
        using var fb = new SolidBrush(fg);
        var sf = new StringFormat
        {
            LineAlignment = StringAlignment.Center,
            Trimming = StringTrimming.EllipsisCharacter,
            FormatFlags = StringFormatFlags.NoWrap
        };

        int iconPad = isGroup ? 8 : 24;

        // 非分组节点：画一个小图标
        if (!isGroup)
        {
            // 文件夹图标（小方块）
            int x = e.Bounds.X + 10;
            int y = e.Bounds.Y + (e.Bounds.Height - 14) / 2;
            var rect = new Rectangle(x, y, 14, 14);
            using var b2 = new SolidBrush(Theme.AccentFolder);
            using var path = GetRoundedRect(rect, 2);
            e.Graphics.FillPath(b2, path);
        }

        var textRect = new Rectangle(e.Bounds.X + iconPad, e.Bounds.Y, e.Bounds.Width - iconPad - 4, e.Bounds.Height);
        e.Graphics.DrawString(e.Node.Text, font, fb, textRect, sf);
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

    private void ShowFavoriteContextMenu(TreeNode node, string path, Point screenLocation)
    {
        var menu = new ContextMenuStrip
        {
            BackColor = Theme.BgSidebar,
            ForeColor = Theme.FgMain,
            Font = Theme.GetUiFont(9),
            ShowImageMargin = false
        };

        var open = menu.Items.Add("在新标签打开");
        open.Click += (_, _) => AddNewTab(path);

        var openCurrent = menu.Items.Add("在当前标签打开");
        openCurrent.Click += (_, _) => OpenPathInCurrentTab(path);

        menu.Items.Add("-");

        var remove = menu.Items.Add("从收藏夹移除");
        remove.Click += (_, _) => _favorites.Remove(path);

        var rename = menu.Items.Add("重命名");
        rename.Click += (_, _) =>
        {
            string? newName = ShowInputDialog("重命名收藏", "新名称：", node.Text);
            if (!string.IsNullOrEmpty(newName))
            {
                _favorites.Rename(path, newName);
            }
        };

        menu.Show(screenLocation);
    }

    private static string? ShowInputDialog(string title, string prompt, string defaultValue)
    {
        using var dlg = new Form
        {
            Text = title,
            FormBorderStyle = FormBorderStyle.FixedDialog,
            StartPosition = FormStartPosition.CenterParent,
            ClientSize = new Size(360, 130),
            MaximizeBox = false,
            MinimizeBox = false,
            BackColor = Theme.BgSidebar
        };
        var lbl = new Label
        {
            Text = prompt,
            Location = new Point(16, 16),
            Size = new Size(328, 22),
            Font = Theme.GetUiFont(9),
            ForeColor = Theme.FgMain
        };
        var txt = new TextBox
        {
            Text = defaultValue,
            Location = new Point(16, 44),
            Size = new Size(328, 28),
            Font = Theme.GetUiFont(10),
            BackColor = Theme.BgMain,
            ForeColor = Theme.FgMain,
            BorderStyle = BorderStyle.FixedSingle
        };
        var ok = new Button
        {
            Text = "确定",
            DialogResult = DialogResult.OK,
            Location = new Point(200, 84),
            Size = new Size(70, 30),
            Font = Theme.GetUiFont(9)
        };
        var cancel = new Button
        {
            Text = "取消",
            DialogResult = DialogResult.Cancel,
            Location = new Point(276, 84),
            Size = new Size(70, 30),
            Font = Theme.GetUiFont(9)
        };
        dlg.Controls.AddRange(new Control[] { lbl, txt, ok, cancel });
        dlg.AcceptButton = ok;
        dlg.CancelButton = cancel;
        return dlg.ShowDialog() == DialogResult.OK ? txt.Text : null;
    }

    // ===== 全局快捷键 =====
    private void MainForm_KeyDown(object? sender, KeyEventArgs e)
    {
        // Ctrl+D：快速跳转
        if (e.Control && e.KeyCode == Keys.D)
        {
            ShowQuickJump();
            e.Handled = true;
            e.SuppressKeyPress = true;
        }
        // Ctrl+T：新建标签
        else if (e.Control && e.KeyCode == Keys.T)
        {
            AddNewTab("");
            e.Handled = true;
            e.SuppressKeyPress = true;
        }
        // Ctrl+W：关闭当前标签
        else if (e.Control && e.KeyCode == Keys.W)
        {
            CloseTab(_tabControl.SelectedIndex);
            e.Handled = true;
            e.SuppressKeyPress = true;
        }
        // Ctrl+Tab：切换到下一个标签
        else if (e.Control && e.KeyCode == Keys.Tab)
        {
            int next = (_tabControl.SelectedIndex + 1) % _tabControl.TabCount;
            _tabControl.SelectedIndex = next;
            e.Handled = true;
            e.SuppressKeyPress = true;
        }
        // Ctrl+Shift+Tab：切换到上一个标签
        else if (e.Control && e.Shift && e.KeyCode == Keys.Tab)
        {
            int prev = (_tabControl.SelectedIndex - 1 + _tabControl.TabCount) % _tabControl.TabCount;
            _tabControl.SelectedIndex = prev;
            e.Handled = true;
            e.SuppressKeyPress = true;
        }
        // F5：刷新当前标签
        else if (e.KeyCode == Keys.F5)
        {
            // 通过反射拿到 FileTabUserControl 并重新加载
            if (_tabControl.SelectedTab?.Controls[0] is FileTabUserControl content)
            {
                // 直接调用 protected LoadDirectory 不可，用 NavigateTo 自身路径刷新
                var p = content.CurrentPath;
                content.NavigateTo(p);
            }
            e.Handled = true;
        }
    }

    private void ShowQuickJump()
    {
        using var dlg = new QuickJumpForm(_favorites, _session);
        if (dlg.ShowDialog(this) == DialogResult.OK && !string.IsNullOrEmpty(dlg.SelectedPath))
        {
            OpenPathInCurrentTab(dlg.SelectedPath);
        }
    }

    private void MainForm_FormClosing(object? sender, FormClosingEventArgs e)
    {
        // 保存当前所有标签页状态
        try
        {
            var tabs = new List<TabSessionItem>();
            for (int i = 0; i < _tabControl.TabCount; i++)
            {
                if (_tabControl.TabPages[i].Controls[0] is FileTabUserControl content)
                {
                    tabs.Add(new TabSessionItem
                    {
                        Title = _tabControl.TabPages[i].Text,
                        Path = content.CurrentPath,
                        Active = i == _tabControl.SelectedIndex
                    });
                }
            }
            _session.SaveTabs(tabs);
        }
        catch
        {
            // 保存失败不阻塞退出
        }
    }
}
