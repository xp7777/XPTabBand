using System.IO;
using System.Runtime.InteropServices;

namespace FileExplorerPro;

internal static class Program
{
    [DllImport("dwmapi.dll")]
    private static extern int DwmSetWindowAttribute(IntPtr hwnd, int attribute, ref int pvAttribute, int cbAttribute);

    [STAThread]
    private static void Main()
    {
        // 清空旧日志
        try { File.Delete(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "log.txt")); }
        catch { }
        DebugLog.Log("程序启动");

        ApplicationConfiguration.Initialize();

        var settings = AppSettings.Load();
        DebugLog.Log($"StartupPath={settings.StartupPath}");
        Application.Run(new MainForm(settings));
        DebugLog.Log("程序退出");
    }

    /// <summary>
    /// 给指定窗口启用深色标题栏（Windows 11 22000+）
    /// DWMWA_USE_IMMERSIVE_DARK_MODE = 20
    /// </summary>
    public static void EnableDarkTitleBar(IntPtr hwnd)
    {
        try
        {
            int dark = 1;
            DwmSetWindowAttribute(hwnd, 20, ref dark, sizeof(int));
        }
        catch
        {
            // 老系统不支持，忽略
        }
    }
}
