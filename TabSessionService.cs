using System.Text.Json;
using System.Text.Json.Serialization;

namespace FileExplorerPro;

/// <summary>
/// 单个标签页的会话信息
/// </summary>
public sealed class TabSessionItem
{
    public string Title { get; set; } = "";
    public string Path { get; set; } = "";
    public bool Active { get; set; }
}

/// <summary>
/// 最近访问记录
/// </summary>
public sealed class RecentEntry
{
    public string Path { get; set; } = "";
    public DateTime LastAccess { get; set; } = DateTime.Now;
}

/// <summary>
/// 标签页会话服务：关闭时保存所有标签，启动时恢复
/// 同时维护最近访问路径列表
/// 文件位置：%AppData%\FileExplorerPro\session.json 和 recent.json
/// </summary>
public sealed class TabSessionService
{
    private static readonly string DataDir = Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),
        "FileExplorerPro");

    private static readonly string SessionFile = Path.Combine(DataDir, "session.json");
    private static readonly string RecentFile = Path.Combine(DataDir, "recent.json");

    private static readonly JsonSerializerOptions JsonOpts = new()
    {
        WriteIndented = true,
        Encoder = System.Text.Encodings.Web.JavaScriptEncoder.UnsafeRelaxedJsonEscaping
    };

    private readonly int _maxRecent;

    public TabSessionService(int maxRecent = 20)
    {
        _maxRecent = maxRecent;
    }

    /// <summary>
    /// 保存当前所有标签页状态
    /// </summary>
    public void SaveTabs(IEnumerable<TabSessionItem> tabs)
    {
        try
        {
            Directory.CreateDirectory(DataDir);
            var list = tabs.ToList();
            var json = JsonSerializer.Serialize(list, JsonOpts);
            File.WriteAllText(SessionFile, json);
        }
        catch
        {
            // 持久化失败不阻塞退出
        }
    }

    /// <summary>
    /// 加载上次保存的标签页
    /// </summary>
    public List<TabSessionItem> LoadTabs()
    {
        try
        {
            if (!File.Exists(SessionFile)) return new();
            var json = File.ReadAllText(SessionFile);
            return JsonSerializer.Deserialize<List<TabSessionItem>>(json, JsonOpts) ?? new();
        }
        catch
        {
            return new();
        }
    }

    /// <summary>
    /// 记录一次目录访问（去重 + 提到最前）
    /// </summary>
    public void AddRecent(string path)
    {
        try
        {
            var recent = LoadRecent();
            recent.RemoveAll(x => x.Path.Equals(path, StringComparison.OrdinalIgnoreCase));
            recent.Insert(0, new RecentEntry { Path = path, LastAccess = DateTime.Now });
            if (recent.Count > _maxRecent) recent.RemoveRange(_maxRecent, recent.Count - _maxRecent);
            Directory.CreateDirectory(DataDir);
            var json = JsonSerializer.Serialize(recent, JsonOpts);
            File.WriteAllText(RecentFile, json);
        }
        catch { }
    }

    public List<RecentEntry> LoadRecent()
    {
        try
        {
            if (!File.Exists(RecentFile)) return new();
            var json = File.ReadAllText(RecentFile);
            return JsonSerializer.Deserialize<List<RecentEntry>>(json, JsonOpts) ?? new();
        }
        catch
        {
            return new();
        }
    }
}
