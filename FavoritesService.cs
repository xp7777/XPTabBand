using System.Text.Json;
using System.Text.Json.Serialization;

namespace FileExplorerPro;

/// <summary>
/// 收藏夹项：一个收藏的目录
/// </summary>
public sealed class FavoriteItem
{
    public string Name { get; set; } = "";
    public string Path { get; set; } = "";
    /// <summary>分组名（空字符串=根组）</summary>
    public string Group { get; set; } = "";
}

/// <summary>
/// 收藏夹服务：管理收藏的目录，持久化到 favorites.json
/// 文件位置：%AppData%\FileExplorerPro\favorites.json
/// </summary>
public sealed class FavoritesService
{
    private static readonly string DataDir = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
        "FileExplorerPro");

    private static readonly string FilePath = Path.Combine(DataDir, "favorites.json");

    private static readonly JsonSerializerOptions JsonOpts = new()
    {
        WriteIndented = true,
        DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
        Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping
    };

    private readonly List<FavoriteItem> _items = new();
    private readonly object _lock = new();

    public event EventHandler? Changed;

    public FavoritesService()
    {
        Load();
        // 如果没有任何收藏，初始化几个常用目录
        if (_items.Count == 0)
        {
            AddDefaults();
        }
    }

    private void Load()
    {
        try
        {
            if (!File.Exists(FilePath)) return;
            var json = File.ReadAllText(FilePath);
            var list = JsonSerializer.Deserialize<List<FavoriteItem>>(json, JsonOpts);
            if (list is not null)
            {
                _items.Clear();
                _items.AddRange(list);
            }
        }
        catch
        {
            // 配置文件损坏不阻塞启动
        }
    }

    private void Save()
    {
        try
        {
            Directory.CreateDirectory(DataDir);
            var json = JsonSerializer.Serialize(_items, JsonOpts);
            File.WriteAllText(FilePath, json);
        }
        catch
        {
            // 持久化失败不阻塞 UI
        }
    }

    private void AddDefaults()
    {
        var defaults = new[]
        {
            ("桌面", Environment.GetFolderPath(Environment.SpecialFolder.Desktop)),
            ("文档", Environment.GetFolderPath(Environment.SpecialFolder.MyDocuments)),
            ("下载", GetDownloadsPath()),
            ("图片", Environment.GetFolderPath(Environment.SpecialFolder.MyPictures)),
            ("音乐", Environment.GetFolderPath(Environment.SpecialFolder.MyMusic)),
            ("视频", Environment.GetFolderPath(Environment.SpecialFolder.MyVideos)),
            ("此电脑", "ThisPC")
        };
        foreach (var (name, path) in defaults)
        {
            if (!string.IsNullOrEmpty(path) && path != "ThisPC" && !Directory.Exists(path)) continue;
            _items.Add(new FavoriteItem { Name = name, Path = path, Group = "常用" });
        }
        // 系统磁盘
        try
        {
            foreach (var drive in DriveInfo.GetDrives())
            {
                if (!drive.IsReady) continue;
                _items.Add(new FavoriteItem
                {
                    Name = drive.Name.TrimEnd('\\'),
                    Path = drive.RootDirectory.FullName,
                    Group = "磁盘"
                });
            }
        }
        catch { }
        Save();
    }

    private static string GetDownloadsPath()
    {
        try
        {
            // Downloads 在 .NET 中需要特殊处理
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Downloads");
        }
        catch
        {
            return "";
        }
    }

    public IReadOnlyList<FavoriteItem> GetAll()
    {
        lock (_lock)
        {
            return _items.ToList();
        }
    }

    /// <summary>
    /// 按分组返回收藏夹
    /// </summary>
    public IReadOnlyDictionary<string, List<FavoriteItem>> GetGrouped()
    {
        lock (_lock)
        {
            return _items
                .GroupBy(x => string.IsNullOrEmpty(x.Group) ? "其他" : x.Group)
                .OrderBy(g => g.Key)
                .ToDictionary(g => g.Key, g => g.ToList());
        }
    }

    public void Add(FavoriteItem item)
    {
        lock (_lock)
        {
            // 去重：同路径不重复添加
            if (_items.Any(x => x.Path.Equals(item.Path, StringComparison.OrdinalIgnoreCase))) return;
            _items.Add(item);
        }
        Save();
        Changed?.Invoke(this, EventArgs.Empty);
    }

    public void Remove(string path)
    {
        lock (_lock)
        {
            _items.RemoveAll(x => x.Path.Equals(path, StringComparison.OrdinalIgnoreCase));
        }
        Save();
        Changed?.Invoke(this, EventArgs.Empty);
    }

    public void Rename(string path, string newName)
    {
        lock (_lock)
        {
            var item = _items.FirstOrDefault(x => x.Path.Equals(path, StringComparison.OrdinalIgnoreCase));
            if (item is not null) item.Name = newName;
        }
        Save();
        Changed?.Invoke(this, EventArgs.Empty);
    }

    public bool IsFavorite(string path)
    {
        lock (_lock)
        {
            return _items.Any(x => x.Path.Equals(path, StringComparison.OrdinalIgnoreCase));
        }
    }
}
