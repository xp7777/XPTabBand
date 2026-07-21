using System.Diagnostics;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Windows.Forms;

namespace FileExplorerPro;

/// <summary>
/// 单个标签页内容控件：路径栏 + 后退/前进 + 文件列表 + 状态栏
/// 每个标签页拥有一个独立的 FileTabUserControl
/// </summary>
public sealed class FileTabUserControl : UserControl
{
    private readonly AppSettings _settings;
    private readonly FavoritesService _favorites;

    // 路径历史栈（后退/前进）
    private readonly List<string> _backStack = new();
    private readonly List<string> _forwardStack = new();
    private string _currentPath = "";

    // 顶部控件
    private readonly Panel _toolbar;
    private readonly Button _btnBack;
    private readonly Button _btnForward;
    private readonly Button _btnUp;
    private readonly Panel _addressBar;
    private readonly Panel _crumbsHost;  // 面包屑容器
    private readonly TextBox _addressEdit; // 文本编辑模式
    private bool _addressEditing;

    // 文件列表
    private readonly ListView _listView;
    private readonly ImageList _smallIcons;
    private readonly ImageList _largeIcons;

    // 状态栏
    private readonly Label _statusBar;

    // 当前选中目录变化事件
    public event EventHandler<string>? PathChanged;
    public event EventHandler? FavoriteStateChanged;

    public string CurrentPath => _currentPath;
    public ListView.ListViewItemCollection Items => _listView.Items;

    public FileTabUserControl(AppSettings settings, FavoritesService favorites)
    {
        _settings = settings;
        _favorites = favorites;
        Dock = DockStyle.Fill;
        BackColor = Theme.BgMain;

        // ===== 工具栏 =====
        _toolbar = new Panel
        {
            Dock = DockStyle.Top,
            Height = 38,
            BackColor = Theme.BgAddressBar,
            Padding = new Padding(6, 4, 6, 4)
        };

        _btnBack = CreateToolButton("←", "后退");
        _btnBack.Click += (_, _) => GoBack();
        _btnForward = CreateToolButton("→", "前进");
        _btnForward.Click += (_, _) => GoForward();
        _btnUp = CreateToolButton("↑", "上一级");
        _btnUp.Click += (_, _) => GoUp();

        int x = 6;
        _btnBack.Location = new Point(x, 6);
        x += 30;
        _btnForward.Location = new Point(x, 6);
        x += 30;
        _btnUp.Location = new Point(x, 6);
        x += 30 + 6;

        // 地址栏
        _addressBar = new Panel
        {
            Location = new Point(x, 6),
            Height = 26,
            BackColor = Theme.BgMain,
            Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right
        };
        _addressBar.Width = _toolbar.Width - x - 6;
        _toolbar.Resize += (_, _) =>
        {
            _addressBar.Width = _toolbar.Width - _addressBar.Left - 6;
        };

        _crumbsHost = new Panel
        {
            Dock = DockStyle.Fill,
            BackColor = Theme.BgMain,
            AutoScroll = true
        };
        _addressEdit = new TextBox
        {
            Dock = DockStyle.Fill,
            BorderStyle = BorderStyle.None,
            Font = Theme.GetUiFont(9),
            BackColor = Theme.BgMain,
            ForeColor = Theme.FgMain,
            Visible = false
        };
        _addressEdit.KeyDown += (_, e) =>
        {
            if (e.KeyCode == Keys.Enter)
            {
                ExitAddressEditMode();
                TryNavigate(_addressEdit.Text);
            }
            else if (e.KeyCode == Keys.Escape)
            {
                ExitAddressEditMode();
            }
        };
        _addressEdit.LostFocus += (_, _) => ExitAddressEditMode();

        _addressBar.Controls.Add(_crumbsHost);
        _addressBar.Controls.Add(_addressEdit);
        // 双击地址栏切换文本编辑模式
        _addressBar.DoubleClick += (_, _) => EnterAddressEditMode();

        _toolbar.Controls.Add(_btnBack);
        _toolbar.Controls.Add(_btnForward);
        _toolbar.Controls.Add(_btnUp);
        _toolbar.Controls.Add(_addressBar);

        // ===== 文件列表 =====
        _smallIcons = new ImageList { ImageSize = new Size(16, 16), ColorDepth = ColorDepth.Depth32Bit };
        _largeIcons = new ImageList { ImageSize = new Size(48, 48), ColorDepth = ColorDepth.Depth32Bit };

        _listView = new ListView
        {
            Dock = DockStyle.Fill,
            BackColor = Theme.BgList,
            ForeColor = Theme.FgMain,
            BorderStyle = BorderStyle.None,
            Font = Theme.GetUiFont(9),
            OwnerDraw = true,
            View = View.Details,
            FullRowSelect = true,
            HideSelection = false,
            SmallImageList = _smallIcons,
            LargeImageList = _largeIcons
        };
        // ListView.DoubleBuffered 是 protected，通过反射启用双缓冲减少闪烁
        typeof(ListView).GetProperty("DoubleBuffered",
            System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Instance)
            ?.SetValue(_listView, true);
        SetupColumns();
        _listView.DrawItem += (_, e) => DrawListItem(e);
        _listView.DrawSubItem += (_, e) => DrawSubItem(e);
        _listView.DrawColumnHeader += (_, e) => DrawColumnHeader(e);
        _listView.MouseDoubleClick += (_, e) =>
        {
            if (e.Button == MouseButtons.Left)
            {
                var info = _listView.HitTest(e.Location);
                if (info.Item is not null && info.Item.Tag is FileSystemInfo fsi)
                {
                    OpenFileSystemInfo(fsi);
                }
            }
        };
        _listView.KeyDown += (_, e) =>
        {
            if (e.KeyCode == Keys.Enter)
            {
                if (_listView.SelectedItems.Count > 0 && _listView.SelectedItems[0].Tag is FileSystemInfo fsi)
                    OpenFileSystemInfo(fsi);
            }
            else if (e.KeyCode == Keys.Back)
            {
                GoBack();
                e.Handled = true;
                e.SuppressKeyPress = true;
            }
        };
        _listView.MouseClick += (_, e) =>
        {
            if (e.Button == MouseButtons.Right)
            {
                ShowContextMenu(_listView.PointToScreen(e.Location), e.Location);
            }
        };

        // ===== 状态栏 =====
        _statusBar = new Label
        {
            Dock = DockStyle.Bottom,
            Height = 22,
            BackColor = Theme.BgSidebar,
            ForeColor = Theme.FgSecondary,
            Font = Theme.GetUiFont(8),
            TextAlign = ContentAlignment.MiddleLeft,
            Padding = new Padding(8, 0, 0, 0),
            Text = "就绪"
        };

        Controls.Add(_listView);
        Controls.Add(_toolbar);
        Controls.Add(_statusBar);
    }

    private void SetupColumns()
    {
        _listView.Columns.Add("名称", 300);
        _listView.Columns.Add("修改日期", 150);
        _listView.Columns.Add("类型", 120);
        _listView.Columns.Add("大小", 100);
    }

    private Button CreateToolButton(string text, string tooltip)
    {
        var btn = new Button
        {
            Text = text,
            Size = new Size(30, 26),
            FlatStyle = FlatStyle.Flat,
            BackColor = Theme.BgButton,
            ForeColor = Theme.FgMain,
            Font = Theme.GetUiFont(11),
            TextAlign = ContentAlignment.MiddleCenter,
            Cursor = Cursors.Hand,
            Margin = new Padding(0)
        };
        btn.FlatAppearance.BorderSize = 0;
        btn.FlatAppearance.MouseOverBackColor = Theme.BgButtonHover;
        var tip = new ToolTip { ShowAlways = true };
        tip.SetToolTip(btn, tooltip);
        return btn;
    }

    /// <summary>
    /// 导航到指定路径（推入后退栈）
    /// </summary>
    public void NavigateTo(string path)
    {
        if (string.IsNullOrEmpty(path) || path == "ThisPC")
        {
            NavigateToThisPC();
            return;
        }
        TryNavigate(path);
    }

    private void NavigateToThisPC()
    {
        _listView.Items.Clear();
        _listView.BeginUpdate();
        try
        {
            foreach (var drive in DriveInfo.GetDrives())
            {
                if (!drive.IsReady) continue;
                var item = new ListViewItem(drive.Name.TrimEnd('\\'))
                {
                    Tag = new DirectoryInfo(drive.RootDirectory.FullName),
                    ImageKey = "drive"
                };
                item.SubItems.Add("");
                item.SubItems.Add("本地磁盘");
                try { item.SubItems.Add($"{drive.TotalSize / 1024.0 / 1024 / 1024:F0} GB"); }
                catch { item.SubItems.Add(""); }
                _listView.Items.Add(item);
            }
        }
        finally { _listView.EndUpdate(); }

        _currentPath = "ThisPC";
        UpdateCrumbs("此电脑");
        UpdateStatusBar();
        PathChanged?.Invoke(this, _currentPath);
    }

    private void TryNavigate(string path)
    {
        try
        {
            if (string.IsNullOrEmpty(path)) return;
            path = path.Trim('"', ' ');
            if (!Directory.Exists(path))
            {
                MessageBox.Show($"路径不存在：\n{path}", "提示", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            // 推入后退栈
            if (!string.IsNullOrEmpty(_currentPath) && _currentPath != path)
            {
                _backStack.Add(_currentPath);
                _forwardStack.Clear();
            }

            LoadDirectory(path);
            _currentPath = path;
            UpdateCrumbs(path);
            UpdateStatusBar();
            PathChanged?.Invoke(this, _currentPath);
        }
        catch (Exception ex)
        {
            MessageBox.Show($"打开目录失败：{ex.Message}", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void LoadDirectory(string path)
    {
        _listView.Items.Clear();
        _listView.BeginUpdate();
        try
        {
            var dir = new DirectoryInfo(path);
            var entries = new List<FileSystemInfo>();

            try { entries.AddRange(dir.GetDirectories()); }
            catch (UnauthorizedAccessException) { /* 跳过无权限子目录 */ }
            catch { }

            try { entries.AddRange(dir.GetFiles()); }
            catch (UnauthorizedAccessException) { }
            catch { }

            // 排序：文件夹优先，然后按名称
            entries.Sort((a, b) =>
            {
                bool aDir = (a.Attributes & FileAttributes.Directory) != 0;
                bool bDir = (b.Attributes & FileAttributes.Directory) != 0;
                if (aDir != bDir) return aDir ? -1 : 1;
                return string.Compare(a.Name, b.Name, StringComparison.OrdinalIgnoreCase);
            });

            foreach (var fsi in entries)
            {
                if (!_settings.ShowHiddenFiles && (fsi.Attributes & FileAttributes.Hidden) != 0)
                    continue;

                bool isDir = (fsi.Attributes & FileAttributes.Directory) != 0;
                var item = new ListViewItem(isDir ? fsi.Name : GetDisplayName(fsi.Name))
                {
                    Tag = fsi,
                    ImageKey = isDir ? "folder" : "file"
                };

                item.SubItems.Add(fsi.LastWriteTime.ToString("yyyy-MM-dd HH:mm"));
                item.SubItems.Add(isDir ? "文件夹" : GetFileExtension(fsi.Name));
                item.SubItems.Add(isDir ? "" : FormatFileSize(((FileInfo)fsi).Length));
                _listView.Items.Add(item);
            }
        }
        finally
        {
            _listView.EndUpdate();
        }
    }

    private string GetDisplayName(string name)
    {
        if (!_settings.ShowFileExtensions)
        {
            int dot = name.LastIndexOf('.');
            if (dot > 0) return name.Substring(0, dot);
        }
        return name;
    }

    private static string GetFileExtension(string name)
    {
        int dot = name.LastIndexOf('.');
        if (dot <= 0) return "文件";
        return name.Substring(dot).ToLowerInvariant() + " 文件";
    }

    private static string FormatFileSize(long bytes)
    {
        if (bytes < 1024) return $"{bytes} B";
        if (bytes < 1024 * 1024) return $"{bytes / 1024.0:F1} KB";
        if (bytes < 1024 * 1024 * 1024) return $"{bytes / 1024.0 / 1024:F1} MB";
        return $"{bytes / 1024.0 / 1024 / 1024:F2} GB";
    }

    /// <summary>
    /// 更新面包屑路径栏
    /// </summary>
    private void UpdateCrumbs(string path)
    {
        _crumbsHost.Controls.Clear();
        if (path == "ThisPC")
        {
            _crumbsHost.Controls.Add(CreateCrumb("此电脑", "ThisPC"));
            return;
        }

        var parts = new List<(string name, string fullPath)>();
        parts.Add(("此电脑", "ThisPC"));

        try
        {
            var dir = new DirectoryInfo(path);
            var stack = new Stack<(string, string)>();
            var cur = dir;
            while (cur is not null)
            {
                stack.Push((cur.Name.Length == 0 ? cur.FullName : cur.Name, cur.FullName));
                cur = cur.Parent;
            }
            while (stack.Count > 0)
            {
                var p = stack.Pop();
                parts.Add(p);
            }
        }
        catch { }

        int x = 4;
        foreach (var (name, full) in parts)
        {
            var crumb = CreateCrumb(name, full);
            crumb.Location = new Point(x, 3);
            _crumbsHost.Controls.Add(crumb);
            x += crumb.Width;

            // 分隔符 >
            if (!ReferenceEquals(parts[parts.Count - 1].fullPath, full))
            {
                var sep = new Label
                {
                    Text = "›",
                    AutoSize = false,
                    Size = new Size(14, 20),
                    Location = new Point(x, 3),
                    Font = Theme.GetUiFont(10),
                    ForeColor = Theme.FgSecondary,
                    TextAlign = ContentAlignment.MiddleCenter
                };
                _crumbsHost.Controls.Add(sep);
                x += sep.Width;
            }
        }
    }

    private LinkLabel CreateCrumb(string name, string fullPath)
    {
        var ll = new LinkLabel
        {
            Text = name,
            AutoSize = true,
            Font = Theme.GetUiFont(9),
            LinkColor = Theme.FgPathCrumb,
            VisitedLinkColor = Theme.FgPathCrumb,
            LinkBehavior = LinkBehavior.HoverUnderline,
            Padding = new Padding(2, 2, 2, 2),
            Tag = fullPath
        };
        ll.LinkClicked += (_, _) =>
        {
            if (ll.Tag is string p) NavigateTo(p);
        };
        return ll;
    }

    private void EnterAddressEditMode()
    {
        _addressEditing = true;
        _crumbsHost.Visible = false;
        _addressEdit.Visible = true;
        _addressEdit.Text = _currentPath == "ThisPC" ? "" : _currentPath;
        _addressEdit.Focus();
        _addressEdit.SelectAll();
    }

    private void ExitAddressEditMode()
    {
        if (!_addressEditing) return;
        _addressEditing = false;
        _addressEdit.Visible = false;
        _crumbsHost.Visible = true;
    }

    private void UpdateStatusBar()
    {
        int folders = 0, files = 0;
        foreach (ListViewItem item in _listView.Items)
        {
            if (item.Tag is DirectoryInfo) folders++;
            else if (item.Tag is FileInfo) files++;
        }
        _statusBar.Text = $"  {folders} 个文件夹，{files} 个文件    |    {_currentPath}";
    }

    private void OpenFileSystemInfo(FileSystemInfo fsi)
    {
        bool isDir = (fsi.Attributes & FileAttributes.Directory) != 0;
        if (isDir)
        {
            NavigateTo(fsi.FullName);
        }
        else
        {
            try
            {
                Process.Start(new ProcessStartInfo
                {
                    FileName = fsi.FullName,
                    UseShellExecute = true
                });
            }
            catch (Exception ex)
            {
                MessageBox.Show($"打开失败：{ex.Message}", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
    }

    public void GoBack()
    {
        if (_backStack.Count == 0) return;
        string prev = _backStack[_backStack.Count - 1];
        _backStack.RemoveAt(_backStack.Count - 1);
        if (!string.IsNullOrEmpty(_currentPath)) _forwardStack.Add(_currentPath);
        _currentPath = "";
        // 后退到"此电脑"需加载驱动器列表，否则按目录加载
        if (prev == "ThisPC") NavigateToThisPC();
        else LoadDirectory(prev);
        _currentPath = prev;
        UpdateCrumbs(prev);
        UpdateStatusBar();
        PathChanged?.Invoke(this, _currentPath);
    }

    public void GoForward()
    {
        if (_forwardStack.Count == 0) return;
        string next = _forwardStack[_forwardStack.Count - 1];
        _forwardStack.RemoveAt(_forwardStack.Count - 1);
        if (!string.IsNullOrEmpty(_currentPath)) _backStack.Add(_currentPath);
        _currentPath = "";
        // 前进到"此电脑"需加载驱动器列表，否则按目录加载
        if (next == "ThisPC") NavigateToThisPC();
        else LoadDirectory(next);
        _currentPath = next;
        UpdateCrumbs(next);
        UpdateStatusBar();
        PathChanged?.Invoke(this, _currentPath);
    }

    public void GoUp()
    {
        if (string.IsNullOrEmpty(_currentPath) || _currentPath == "ThisPC") return;
        try
        {
            var parent = Directory.GetParent(_currentPath);
            if (parent is not null) NavigateTo(parent.FullName);
            else NavigateToThisPC();
        }
        catch { }
    }

    // ===== 自定义绘制 ListView（深色主题）=====
    private void DrawListItem(DrawListViewItemEventArgs e)
    {
        bool selected = e.Item.Selected;
        Color bg = selected ? Theme.BgListItemSelected
                            : (e.ItemIndex % 2 == 0 ? Theme.BgListItem : Theme.BgListItemAlt);

        // 鼠标悬停
        if (!selected && e.State.HasFlag(ListViewItemStates.Hot))
            bg = Theme.BgListItemHover;

        using (var b = new SolidBrush(bg))
            e.Graphics.FillRectangle(b, e.Bounds);

        e.DrawDefault = false;
    }

    private void DrawSubItem(DrawListViewSubItemEventArgs e)
    {
        var item = e.Item;
        bool selected = item.Selected;
        Color bg = selected ? Theme.BgListItemSelected
                            : (item.Index % 2 == 0 ? Theme.BgListItem : Theme.BgListItemAlt);
        if (!selected && item.ListView is not null)
        {
            // 不重新填充背景，DrawItem 已经画过
        }

        Color fg = selected ? Color.White : Theme.FgMain;
        if (e.ColumnIndex == 1 || e.ColumnIndex == 2 || e.ColumnIndex == 3)
            fg = selected ? Color.White : Theme.FgSecondary;

        // 名称列画图标
        if (e.ColumnIndex == 0)
        {
            // 文件夹图标（用色块+文字简化）
            bool isDir = item.Tag is DirectoryInfo;
            int iconX = e.Bounds.X + 4;
            int iconY = e.Bounds.Y + (e.Bounds.Height - 16) / 2;
            DrawFileIcon(e.Graphics, item.Text, isDir, iconX, iconY, 16);

            var textRect = new Rectangle(iconX + 22, e.Bounds.Y, e.Bounds.Width - 26, e.Bounds.Height);
            using var b = new SolidBrush(fg);
            var sf = new StringFormat
            {
                LineAlignment = StringAlignment.Center,
                Trimming = StringTrimming.EllipsisCharacter,
                FormatFlags = StringFormatFlags.NoWrap
            };
            e.Graphics.DrawString(e.Item.Text, Theme.GetUiFont(9), b, textRect, sf);
        }
        else
        {
            var textRect = new Rectangle(e.Bounds.X + 4, e.Bounds.Y, e.Bounds.Width - 8, e.Bounds.Height);
            using var b = new SolidBrush(fg);
            var sf = new StringFormat
            {
                LineAlignment = StringAlignment.Center,
                Trimming = StringTrimming.EllipsisCharacter,
                FormatFlags = StringFormatFlags.NoWrap
            };
            e.Graphics.DrawString(e.SubItem?.Text ?? "", Theme.GetUiFont(9), b, textRect, sf);
        }
    }

    private void DrawColumnHeader(DrawListViewColumnHeaderEventArgs e)
    {
        using (var b = new SolidBrush(Theme.BgTabBar))
            e.Graphics.FillRectangle(b, e.Bounds);

        using var p = new Pen(Theme.Border);
        e.Graphics.DrawLine(p, e.Bounds.X, e.Bounds.Bottom - 1, e.Bounds.Right, e.Bounds.Bottom - 1);

        var textRect = new Rectangle(e.Bounds.X + 6, e.Bounds.Y, e.Bounds.Width - 12, e.Bounds.Height);
        using var b2 = new SolidBrush(Theme.FgSecondary);
        var sf = new StringFormat
        {
            LineAlignment = StringAlignment.Center,
            Trimming = StringTrimming.EllipsisCharacter,
            FormatFlags = StringFormatFlags.NoWrap
        };
        e.Graphics.DrawString(e.Header?.Text ?? "", Theme.GetUiFont(9), b2, textRect, sf);
    }

    /// <summary>
    /// 简化的文件图标：文件夹画金色方块，文件画灰色方块
    /// </summary>
    private static void DrawFileIcon(Graphics g, string name, bool isDir, int x, int y, int size)
    {
        var rect = new Rectangle(x, y, size, size);
        g.SmoothingMode = SmoothingMode.AntiAlias;

        if (isDir)
        {
            // 文件夹：金色圆角方块
            using var b = new SolidBrush(Theme.AccentFolder);
            using var path = GetRoundedRectPath(rect, 2);
            g.FillPath(b, path);
            // 文件夹顶部凸起
            using var b2 = new SolidBrush(Color.FromArgb(255, 220, 100));
            g.FillRectangle(b2, x + 2, y + 3, size - 4, 2);
        }
        else
        {
            // 文件：浅灰圆角方块
            using var b = new SolidBrush(Color.FromArgb(120, 160, 220));
            using var path = GetRoundedRectPath(rect, 2);
            g.FillPath(b, path);
            // 折角
            using var b2 = new SolidBrush(Color.FromArgb(90, 130, 190));
            var corner = new Point[] {
                new(x + size - 5, y + 1),
                new(x + size - 1, y + 5),
                new(x + size - 1, y + 1)
            };
            g.FillPolygon(b2, corner);
        }
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

    // ===== 右键菜单 =====
    private void ShowContextMenu(Point screenLocation, Point clientLocation)
    {
        var menu = new ContextMenuStrip
        {
            BackColor = Theme.BgSidebar,
            ForeColor = Theme.FgMain,
            Font = Theme.GetUiFont(9),
            ShowImageMargin = false
        };

        var hit = _listView.HitTest(clientLocation);
        bool onItem = hit.Item is not null;

        if (onItem)
        {
            var openItem = menu.Items.Add("打开");
            openItem.Click += (_, _) =>
            {
                if (hit.Item?.Tag is FileSystemInfo fsi) OpenFileSystemInfo(fsi);
            };

            var copyPath = menu.Items.Add("复制路径");
            copyPath.Click += (_, _) =>
            {
                if (hit.Item?.Tag is FileSystemInfo fsi)
                    Clipboard.SetText(fsi.FullName);
            };

            // 添加到收藏夹（仅文件夹）
            if (hit.Item?.Tag is DirectoryInfo di)
            {
                bool isFav = _favorites.IsFavorite(di.FullName);
                var favItem = menu.Items.Add(isFav ? "从收藏夹移除" : "添加到收藏夹");
                favItem.Click += (_, _) =>
                {
                    if (isFav) _favorites.Remove(di.FullName);
                    else _favorites.Add(new FavoriteItem { Name = di.Name, Path = di.FullName, Group = "自定义" });
                    FavoriteStateChanged?.Invoke(this, EventArgs.Empty);
                };
            }

            menu.Items.Add("-");
            var del = menu.Items.Add("删除");
            del.Click += (_, _) =>
            {
                if (hit.Item?.Tag is FileSystemInfo fsi)
                    DeleteFileSystemInfo(fsi);
            };
        }
        else
        {
            // 空白处
            var paste = menu.Items.Add("粘贴");
            paste.Click += (_, _) => PasteFromClipboard();

            var newFolder = menu.Items.Add("新建文件夹");
            newFolder.Click += (_, _) => CreateNewFolder();

            menu.Items.Add("-");
            var copyCurPath = menu.Items.Add("复制当前路径");
            copyCurPath.Click += (_, _) => Clipboard.SetText(_currentPath);

            bool curIsFav = _currentPath != "ThisPC" && _favorites.IsFavorite(_currentPath);
            var favCur = menu.Items.Add(curIsFav ? "从收藏夹移除当前目录" : "收藏当前目录");
            favCur.Click += (_, _) =>
            {
                if (_currentPath == "ThisPC") return;
                if (curIsFav) _favorites.Remove(_currentPath);
                else _favorites.Add(new FavoriteItem { Name = Path.GetFileName(_currentPath), Path = _currentPath, Group = "自定义" });
                FavoriteStateChanged?.Invoke(this, EventArgs.Empty);
            };

            menu.Items.Add("-");
            var refresh = menu.Items.Add("刷新");
            refresh.Click += (_, _) => LoadDirectory(_currentPath);
        }

        menu.Show(screenLocation);
    }

    private void DeleteFileSystemInfo(FileSystemInfo fsi)
    {
        var result = MessageBox.Show($"确定删除 {fsi.Name} 吗？", "确认", MessageBoxButtons.OKCancel, MessageBoxIcon.Question);
        if (result != DialogResult.OK) return;

        try
        {
            if (fsi is DirectoryInfo di) di.Delete(true);
            else if (fsi is FileInfo fi) fi.Delete();
            LoadDirectory(_currentPath);
        }
        catch (Exception ex)
        {
            MessageBox.Show($"删除失败：{ex.Message}", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void CreateNewFolder()
    {
        if (_currentPath == "ThisPC") return;
        try
        {
            string name = "新建文件夹";
            string path = Path.Combine(_currentPath, name);
            int i = 1;
            while (Directory.Exists(path))
            {
                path = Path.Combine(_currentPath, $"新建文件夹 ({i++})");
            }
            Directory.CreateDirectory(path);
            LoadDirectory(_currentPath);
        }
        catch (Exception ex)
        {
            MessageBox.Show($"创建失败：{ex.Message}", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private void PasteFromClipboard()
    {
        if (_currentPath == "ThisPC") return;
        try
        {
            var files = Clipboard.GetFileDropList();
            foreach (string src in files)
            {
                string dest = Path.Combine(_currentPath, Path.GetFileName(src));
                if (Directory.Exists(src))
                    CopyDirectory(src, dest);
                else if (File.Exists(src))
                    File.Copy(src, dest, true);
            }
            LoadDirectory(_currentPath);
        }
        catch (Exception ex)
        {
            MessageBox.Show($"粘贴失败：{ex.Message}", "错误", MessageBoxButtons.OK, MessageBoxIcon.Error);
        }
    }

    private static void CopyDirectory(string src, string dest)
    {
        Directory.CreateDirectory(dest);
        foreach (var f in Directory.GetFiles(src))
            File.Copy(f, Path.Combine(dest, Path.GetFileName(f)), true);
        foreach (var d in Directory.GetDirectories(src))
            CopyDirectory(d, Path.Combine(dest, Path.GetFileName(d)));
    }
}
