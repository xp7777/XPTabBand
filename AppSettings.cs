using Microsoft.Extensions.Configuration;

namespace FileExplorerPro;

/// <summary>
/// 应用配置：从 appsettings.json 读取，所有运行时行为通过配置文件驱动
/// </summary>
public sealed class AppSettings
{
    /// <summary>启动时打开的初始路径，空字符串则打开"此电脑"</summary>
    public string StartupPath { get; set; } = "";

    /// <summary>启动时是否恢复上次关闭时的标签页</summary>
    public bool RestoreTabsOnStart { get; set; } = true;

    /// <summary>最近访问路径最大保留数</summary>
    public int MaxRecentPaths { get; set; } = 20;

    /// <summary>是否显示隐藏文件</summary>
    public bool ShowHiddenFiles { get; set; } = false;

    /// <summary>是否显示文件扩展名</summary>
    public bool ShowFileExtensions { get; set; } = true;

    /// <summary>默认视图：Details / List / LargeIcon / SmallIcon</summary>
    public string DefaultView { get; set; } = "Details";

    /// <summary>主题：Dark / Light</summary>
    public string Theme { get; set; } = "Dark";

    public int WindowWidth { get; set; } = 1200;
    public int WindowHeight { get; set; } = 780;
    public int SidebarWidth { get; set; } = 220;
    public int FontSize { get; set; } = 9;

    public static AppSettings Load()
    {
        var config = new ConfigurationBuilder()
            .SetBasePath(AppContext.BaseDirectory)
            .AddJsonFile("appsettings.json", optional: true)
            .Build();
        var settings = new AppSettings();
        config.Bind(settings);
        return settings;
    }
}
