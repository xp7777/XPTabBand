using System.Diagnostics;
using System.Runtime.InteropServices;

namespace FileExplorerPro;

/// <summary>
/// Shell 命名空间项（控制面板项、网络连接等），复用 ListView 显示逻辑
/// </summary>
public sealed class ShellItem
{
    public string Name { get; set; } = "";
    public string Path { get; set; } = "";
    /// <summary>显示用的类型描述</summary>
    public string Type { get; set; } = "";
    /// <summary>是否为文件夹</summary>
    public bool IsFolder { get; set; } = true;
    /// <summary>是否可在文件管理器内浏览（有可枚举的子项）</summary>
    public bool IsBrowsable { get; set; } = false;
}

/// <summary>
/// Shell 命名空间服务：枚举控制面板、网络连接等特殊文件夹
/// 使用 Shell.Application COM 对象（Windows 内置，无需额外依赖）
/// </summary>
internal static class ShellService
{
    /// <summary>
    /// 枚举控制面板项，并检测哪些可继续浏览
    /// </summary>
    public static List<ShellItem> EnumerateControlPanel()
    {
        return EnumerateShellFolder(3); // 3 = ssfCONTROLS
    }

    /// <summary>
    /// 已知 Shell CLSID → ssf 常量映射。
    /// shell.NameSpace("::{CLSID}") 对部分 CLSID 返回 null，用整数 ssf 常量更可靠。
    /// </summary>
    private static readonly Dictionary<string, int> ClsidToSsf = new(StringComparer.OrdinalIgnoreCase)
    {
        ["::{F02C1A0D-BE21-4350-88B0-7447BC5800D3}"] = 18, // 网络 ssfNETWORK
        ["::{26EE0668-A00A-44D7-9371-BEB064C98683}"] = 3,  // 所有控制面板项 ssfCONTROLS
        ["::{21EC2020-3AEA-1069-A2DD-08002B30309D}"] = 3,  // 控制面板（旧 CLSID）
        ["::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"] = 17, // 此电脑 ssfDRIVES
        ["::{645FF040-5081-101B-9F08-00AA002F954E}"] = 10, // 回收站 ssfBITBUCKET
        ["::{450D8FBA-AD25-11D0-98A8-0800361B1103}"] = 5,  // 我的文档 ssfPERSONAL
    };

    /// <summary>
    /// 尝试将 Shell 路径解析为 ssf 常量；无法映射返回 -1
    /// </summary>
    private static int TryGetSsf(string shellPath)
    {
        // 取第一段 ::{CLSID}（处理复合路径 ::{A}\0\::{B}，用第一段 A 映射）
        string first = shellPath;
        int bs = shellPath.IndexOf('\\');
        if (bs > 0) first = shellPath.Substring(0, bs);
        return ClsidToSsf.TryGetValue(first, out int ssf) ? ssf : -1;
    }

    /// <summary>
    /// 通过 Shell 路径（如 ::{CLSID}）枚举 Shell 文件夹子项
    /// </summary>
    public static List<ShellItem> EnumerateShellFolder(string shellPath)
    {
        var result = new List<ShellItem>();
        try
        {
            var shellType = Type.GetTypeFromProgID("Shell.Application");
            if (shellType is null) return result;
            dynamic shell = Activator.CreateInstance(shellType)!;

            dynamic folder = OpenShellFolder(shell, shellPath);
            if (folder is null)
            {
                Marshal.ReleaseComObject(shell);
                return result;
            }

            result = EnumerateFolderItems(folder);
            Marshal.ReleaseComObject(shell);
        }
        catch
        {
            // Shell COM 不可用时静默失败
        }
        return result;
    }

    /// <summary>
    /// 通过 ssf 常量枚举 Shell 文件夹子项
    /// </summary>
    private static List<ShellItem> EnumerateShellFolder(int ssf)
    {
        var result = new List<ShellItem>();
        try
        {
            var shellType = Type.GetTypeFromProgID("Shell.Application");
            if (shellType is null) return result;
            dynamic shell = Activator.CreateInstance(shellType)!;

            dynamic folder = shell.NameSpace(ssf);
            if (folder is null)
            {
                Marshal.ReleaseComObject(shell);
                return result;
            }

            result = EnumerateFolderItems(folder);
            Marshal.ReleaseComObject(shell);
        }
        catch
        {
            // Shell COM 不可用时静默失败
        }
        return result;
    }

    /// <summary>
    /// 打开 Shell 文件夹：优先用 ssf 常量（更可靠），回退用字符串路径。
    /// 注意：ssf 映射仅对单级 CLSID（无 \）有效；复合路径（如 ::{A}\0\::{B}）直接用字符串。
    /// </summary>
    private static dynamic? OpenShellFolder(dynamic shell, string shellPath)
    {
        try
        {
            // 仅对单级 CLSID（无反斜杠）尝试 ssf 映射
            if (shellPath.IndexOf('\\') < 0)
            {
                int ssf = TryGetSsf(shellPath);
                if (ssf >= 0)
                {
                    dynamic f = shell.NameSpace(ssf);
                    if (f is not null) return f;
                }
            }
            // 复合路径（如 ::{A}\0\::{B}）或未映射 CLSID：尝试用字符串
            return shell.NameSpace(shellPath);
        }
        catch
        {
            return null;
        }
    }

    /// <summary>
    /// 枚举 Shell 文件夹对象的所有子项
    /// </summary>
    private static List<ShellItem> EnumerateFolderItems(dynamic folder)
    {
        var result = new List<ShellItem>();
        try
        {
            dynamic items = folder.Items();
            foreach (dynamic item in items)
            {
                try
                {
                    string name = item.Name ?? "";
                    string path = item.Path ?? "";
                    if (string.IsNullOrEmpty(name)) continue;

                    bool isFolder = false;
                    bool isBrowsable = false;
                    string typeText = "控制面板项";

                    try { isFolder = (bool)item.IsFolder; }
                    catch { }

                    // 文件夹类型的项尝试判断是否可枚举子项
                    if (isFolder)
                    {
                        try
                        {
                            dynamic subFolder = item.GetFolder;
                            dynamic subItems = subFolder.Items();
                            int count = subItems.Count;
                            isBrowsable = count > 0;
                            typeText = isBrowsable ? "Shell 文件夹" : "控制面板项";
                        }
                        catch
                        {
                            // GetFolder 失败说明不可浏览
                            typeText = "控制面板项";
                        }
                    }

                    result.Add(new ShellItem
                    {
                        Name = name,
                        Path = path,
                        Type = typeText,
                        IsFolder = isFolder,
                        IsBrowsable = isBrowsable
                    });
                }
                catch { }
            }
        }
        catch { }
        return result;
    }

    /// <summary>
    /// 从控制面板开始，按名称路径（如"网络和 Internet\网络连接"）逐级解析 Shell 文件夹
    /// 返回最终项的 Shell 路径和显示名；未找到返回 null
    /// </summary>
    public static (string ShellPath, string Name)? ResolveControlPanelPath(string relativePath)
    {
        if (string.IsNullOrWhiteSpace(relativePath)) return null;
        var segments = relativePath.Split(new[] { '\\', '/' }, StringSplitOptions.RemoveEmptyEntries);

        string? currentShellPath = null; // null 表示从控制面板（ssf=3）开始
        string currentName = "控制面板";

        foreach (var seg in segments)
        {
            string segTrim = seg.Trim();
            // 跳过"所有控制面板项"段：ssf=3 枚举的就是这个文件夹的内容，它本身不出现在子项列表里
            // 用户从资源管理器复制的路径通常包含这一段，需要兼容
            if (string.Equals(segTrim, "所有控制面板项", StringComparison.OrdinalIgnoreCase) ||
                string.Equals(segTrim, "All Control Panel Items", StringComparison.OrdinalIgnoreCase))
            {
                continue;
            }

            var items = currentShellPath == null
                ? EnumerateControlPanel()
                : EnumerateShellFolder(currentShellPath);

            if (items.Count == 0) return null;

            // 先精确匹配，再用包含匹配提高兼容性（不同 Windows 版本名称可能略有差异）
            var match = items.FirstOrDefault(i =>
                            string.Equals(i.Name.Trim(), segTrim, StringComparison.OrdinalIgnoreCase))
                        ?? items.FirstOrDefault(i =>
                            i.Name.IndexOf(segTrim, StringComparison.OrdinalIgnoreCase) >= 0);

            if (match is null) return null;
            if (string.IsNullOrEmpty(match.Path)) return null;

            currentShellPath = match.Path;
            currentName = match.Name;
        }

        return currentShellPath is null ? null : (currentShellPath, currentName);
    }

    /// <summary>
    /// 获取 Shell 文件夹的显示名称；不可浏览（NameSpace 返回 null）返回 null
    /// 用于区分"可浏览但子项为空"（如网络无计算机）和"不可浏览的任务页面"（如网络和共享中心）
    /// </summary>
    public static string? GetShellFolderDisplayName(string shellPath)
    {
        try
        {
            var shellType = Type.GetTypeFromProgID("Shell.Application");
            if (shellType is null) return null;
            dynamic shell = Activator.CreateInstance(shellType)!;
            dynamic folder = OpenShellFolder(shell, shellPath);
            if (folder is null)
            {
                Marshal.ReleaseComObject(shell);
                return null;
            }
            // folder.Title 返回文件夹的本地化显示名（如"网络"、"网络连接"）
            string name = "";
            try { name = (string)folder.Title; }
            catch { }
            Marshal.ReleaseComObject(shell);
            return string.IsNullOrEmpty(name) ? shellPath : name;
        }
        catch
        {
            return null;
        }
    }

    /// <summary>
    /// 启动控制面板项（通过其路径/GUID）
    /// </summary>
    public static void LaunchItem(ShellItem item)
    {
        try
        {
            if (!string.IsNullOrEmpty(item.Path))
            {
                // explorer.exe "::{CLSID}" 是 Windows 打开 Shell 文件夹/控制面板项的标准方式
                // 注意：explorer.exe 命令行不识别 "shell:::{CLSID}" 这种写法，shell::: 前缀
                // 仅用于资源管理器地址栏或 Process.Start 的 FileName，不能作为 explorer.exe 的参数
                Process.Start(new ProcessStartInfo
                {
                    FileName = "explorer.exe",
                    Arguments = $"\"{item.Path}\"",
                    UseShellExecute = true
                });
            }
        }
        catch
        {
            // 启动失败静默处理
        }
    }
}
