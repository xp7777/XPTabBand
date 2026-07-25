using System;
using System.Runtime.InteropServices;

namespace XPTab.BandEnabler
{
    /// <summary>
    /// 通过 COM API 强制 Explorer 显示 XPTab Band
    /// 原理：枚举所有打开的 Explorer 窗口，调用 ShowBrowserBar
    /// </summary>
    internal static class BandEnabler
    {
        private const string Clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}";

        [STAThread]
        private static int Main(string[] args)
        {
            Console.WriteLine("=== XPTab Band Enabler ===");
            Console.WriteLine("CLSID: " + Clsid);
            Console.WriteLine();

            // 方法1: 通过 Shell.Application 枚举窗口并调用 ShowBrowserBar
            try
            {
                Type shellType = Type.GetTypeFromProgID("Shell.Application");
                if (shellType == null)
                {
                    Console.WriteLine("[FAIL] Shell.Application not available");
                    return 1;
                }
                dynamic shell = Activator.CreateInstance(shellType);
                dynamic windows = shell.Windows();
                int count = windows.Count;
                Console.WriteLine("Found " + count + " Explorer/IE window(s)");

                if (count == 0)
                {
                    Console.WriteLine();
                    Console.WriteLine("请先打开一个文件夹窗口，再运行本程序。");
                    Console.WriteLine("按任意键退出...");
                    Console.ReadKey();
                    return 2;
                }

                bool anySuccess = false;
                for (int i = 0; i < count; i++)
                {
                    dynamic win = windows.Item(i);
                    if (win == null) continue;
                    Console.WriteLine("Window " + i + ": " + win.FullName);
                    try
                    {
                        // ShowBrowserBar 的参数：CLSID, bShow (true=show), bSize
                        win.ShowBrowserBar(Clsid, true, null);
                        Console.WriteLine("  -> ShowBrowserBar called (show=true)");
                        anySuccess = true;
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine("  -> ShowBrowserBar failed: " + ex.Message);
                    }
                    Marshal.ReleaseComObject(win);
                }
                Marshal.ReleaseComObject(windows);
                Marshal.ReleaseComObject(shell);

                Console.WriteLine();
                if (anySuccess)
                {
                    Console.WriteLine("[OK] ShowBrowserBar 已调用。请检查 Explorer 窗口是否出现 XPTab Band。");
                    Console.WriteLine("如果仍未显示，可能是 Win10 Ribbon 隐藏了 Band 区域。");
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("[FAIL] " + ex.Message);
            }

            Console.WriteLine();
            Console.WriteLine("=== 方法2: 通过注册表 Streams 强制启用 ===");
            Console.WriteLine("Win10 的 Ribbon 界面默认隐藏 Band。");
            Console.WriteLine("需要通过修改注册表 HKCU\\...\\Streams\\Defaults");
            Console.WriteLine("或使用第三方工具如 QTTabBar 的启用方式。");
            Console.WriteLine();
            Console.WriteLine("按任意键退出...");
            Console.ReadKey();
            return 0;
        }
    }
}
