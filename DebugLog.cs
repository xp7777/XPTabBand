using System.IO;
using System.Text;

namespace FileExplorerPro;

/// <summary>
/// 简单文件日志，用于调试 + 按钮点击问题
/// 日志文件位于程序所在目录的 log.txt
/// </summary>
internal static class DebugLog
{
    private static readonly string _logPath = Path.Combine(
        AppDomain.CurrentDomain.BaseDirectory, "log.txt");

    private static readonly object _lock = new();

    public static void Log(string message)
    {
        try
        {
            lock (_lock)
            {
                var line = $"{DateTime.Now:HH:mm:ss.fff} [{Thread.CurrentThread.ManagedThreadId}] {message}{Environment.NewLine}";
                File.AppendAllText(_logPath, line, Encoding.UTF8);
            }
        }
        catch
        {
            // 日志失败不影响程序运行
        }
    }
}
