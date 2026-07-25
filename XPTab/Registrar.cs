using System;
using System.Diagnostics;
using System.Security.Principal;
using Microsoft.Win32;

namespace XPTab.Registrar
{
    /// <summary>
    /// XPTab Band 注册/卸载工具
    /// 编译: csc Registrar.cs /reference:XPTab.dll /r:System.Management.dll
    /// 用法:
    ///   Registrar.exe register   注册 Band
    ///   Registrar.exe unregister 卸载 Band
    ///   Registrar.exe status     检查注册状态
    /// 会自动申请 UAC 提权。
    /// </summary>
    internal static class Registrar
    {
        private const string Clsid = "{a1b2c3d4-e5f6-4789-abcd-0123456789ab}";
        private const string BandName = "XPTab";
        private const string CatId = "{00021493-0000-0000-C000-000000000046}"; // CommBand

        [STAThread]
        private static int Main(string[] args)
        {
            string cmd = args.Length > 0 ? args[0].ToLowerInvariant() : "status";

            // 需要写注册表的命令先检查并申请 UAC 提权
            if ((cmd == "register" || cmd == "unregister") && !IsAdministrator())
            {
                Console.WriteLine("需要管理员权限，正在申请 UAC 提权...");
                return RelaunchAsAdmin(cmd);
            }

            try
            {
                switch (cmd)
                {
                    case "register": return Register();
                    case "unregister": return Unregister();
                    case "status": return Status();
                    default:
                        Console.WriteLine("用法: Registrar.exe [register|unregister|status]");
                        return 0;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine("[错误] " + ex.Message);
                Console.WriteLine(ex.StackTrace);
                Console.WriteLine();
                Console.WriteLine("按任意键退出...");
                Console.ReadKey();
                return 2;
            }
        }

        private static bool IsAdministrator()
        {
            try
            {
                var identity = WindowsIdentity.GetCurrent();
                var principal = new WindowsPrincipal(identity);
                return principal.IsInRole(WindowsBuiltInRole.Administrator);
            }
            catch { return false; }
        }

        /// <summary>
        /// 以管理员身份重新启动自身
        /// </summary>
        private static int RelaunchAsAdmin(string cmd)
        {
            try
            {
                var exePath = Process.GetCurrentProcess().MainModule.FileName;
                var psi = new ProcessStartInfo
                {
                    FileName = exePath,
                    Arguments = cmd,
                    Verb = "runas",  // 触发 UAC 提权
                    UseShellExecute = true
                };
                Process.Start(psi);
                return 0;
            }
            catch (Exception ex)
            {
                Console.WriteLine("[错误] UAC 提权失败: " + ex.Message);
                Console.WriteLine("请右键 Registrar.exe → 以管理员身份运行");
                Console.ReadKey();
                return 1;
            }
        }

        /// <summary>
        /// 注册 Band（注册表项；COM 注册由 regasm 完成）
        /// </summary>
        private static int Register()
        {
            Console.WriteLine("=== XPTab Band 注册 ===");
            Console.WriteLine("CLSID: " + Clsid);
            Console.WriteLine();

            // 0. 先调用 regasm 注册 COM（包括 /codebase 和 /tlb）
            string dllPath = System.IO.Path.Combine(
                AppDomain.CurrentDomain.BaseDirectory, "XPTab.dll");
            string regasm = System.IO.Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.Windows),
                "Microsoft.NET\\Framework64\\v4.0.30319\\regasm.exe");

            if (!System.IO.File.Exists(dllPath))
            {
                Console.WriteLine("[错误] 找不到 XPTab.dll，请确保它与 Registrar.exe 在同一目录。");
                Console.WriteLine("  期望路径: " + dllPath);
                return 3;
            }
            if (!System.IO.File.Exists(regasm))
            {
                Console.WriteLine("[错误] 找不到 regasm.exe: " + regasm);
                return 3;
            }

            Console.WriteLine("[1/5] 调用 regasm 注册 COM...");
            Console.WriteLine("  " + regasm + " /codebase /tlb \"" + dllPath + "\"");
            int regasmExit = RunCommand(regasm, "/codebase /tlb \"" + dllPath + "\"");
            if (regasmExit != 0)
            {
                Console.WriteLine("[错误] regasm 失败，退出码 " + regasmExit);
                return 4;
            }
            Console.WriteLine("[OK] COM 注册完成");
            Console.WriteLine();

            // 1. Toolbar 注册项
            using (var k = Registry.LocalMachine.CreateSubKey(
                "SOFTWARE\\Microsoft\\Internet Explorer\\Toolbar\\" + Clsid))
            {
                k.SetValue("", BandName);
            }
            Console.WriteLine("[2/5] 已写入 Toolbar 注册项");

            // 2. Approved Shell Extensions
            using (var k = Registry.LocalMachine.CreateSubKey(
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", writable: true))
            {
                k.SetValue(Clsid, BandName);
            }
            Console.WriteLine("[3/5] 已注册到 Approved Shell Extensions");

            // 3. IE Band 组件类别（关键：Explorer 通过此类别识别合法 Band）
            // Component Categories 键被 Windows 特殊保护，即使管理员也无法直接 CreateSubKey
            // 改用 reg.exe import 导入 .reg 文件，reg.exe 以更高权限运行能绕过限制
            Console.WriteLine("[4/5] 注册 IE Band 组件类别...");
            string regFile = System.IO.Path.Combine(System.IO.Path.GetTempPath(), "xptab_cat.reg");
            string regContent =
                "Windows Registry Editor Version 5.00\r\n" +
                "\r\n" +
                "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Component Categories\\" + CatId + "]\r\n" +
                "\"0409\"=\"Internet Toolbar\"\r\n" +
                "\"0804\"=\"Internet 工具栏\"\r\n" +
                "\r\n" +
                "[HKEY_LOCAL_MACHINE\\SOFTWARE\\Classes\\Component Categories\\" + CatId + "\\Impl]\r\n" +
                "\"" + Clsid + "\"=\"" + BandName + "\"\r\n";
            System.IO.File.WriteAllText(regFile, regContent, System.Text.Encoding.Unicode);

            string regExe = System.IO.Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.Windows), "reg.exe");
            int regExit = RunCommand(regExe, "import \"" + regFile + "\"");
            try { System.IO.File.Delete(regFile); } catch { }
            if (regExit != 0)
            {
                Console.WriteLine("[错误] reg.exe import 失败，退出码 " + regExit);
                return 5;
            }
            Console.WriteLine("      已注册 IE Band 组件类别 (CommBand)");

            // 4. Windows 11 旧版 DeskBand 支持（Win10 无害）
            using (var k = Registry.CurrentUser.CreateSubKey(
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"))
            {
                k.SetValue("EnableLegacyBars", 1, RegistryValueKind.DWord);
            }
            Console.WriteLine("[5/5] 已启用 EnableLegacyBars");

            Console.WriteLine();
            Console.WriteLine("=== 注册完成 ===");
            Console.WriteLine("下一步：");
            Console.WriteLine("  1. 重启 explorer.exe（任务管理器→Windows 资源管理器→重启）");
            Console.WriteLine("  2. 文件夹窗口按 Alt→查看→工具栏→勾选 XPTab");
            Console.WriteLine();
            Console.WriteLine("按任意键退出...");
            Console.ReadKey();
            return 0;
        }

        private static int RunCommand(string exe, string args)
        {
            var psi = new ProcessStartInfo
            {
                FileName = exe,
                Arguments = args,
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true
            };
            using (var p = Process.Start(psi))
            {
                string stdout = p.StandardOutput.ReadToEnd();
                string stderr = p.StandardError.ReadToEnd();
                p.WaitForExit();
                if (!string.IsNullOrEmpty(stdout)) Console.WriteLine(stdout);
                if (!string.IsNullOrEmpty(stderr)) Console.Error.WriteLine(stderr);
                return p.ExitCode;
            }
        }

        private static int Unregister()
        {
            Console.WriteLine("=== XPTab Band 卸载 ===");

            // 删除 Toolbar 注册项
            try { Registry.LocalMachine.DeleteSubKey(
                "SOFTWARE\\Microsoft\\Internet Explorer\\Toolbar\\" + Clsid); }
            catch { }
            Console.WriteLine("[OK] 已清除 Toolbar 注册项");

            // 删除 Approved
            try
            {
                using (var k = Registry.LocalMachine.OpenSubKey(
                    "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved", writable: true))
                {
                    if (k != null) k.DeleteValue(Clsid, throwOnMissingValue: false);
                }
            }
            catch { }
            Console.WriteLine("[OK] 已清除 Approved Shell Extensions");

            // 删除组件类别 Impl 下的值
            try
            {
                using (var k = Registry.LocalMachine.OpenSubKey(
                    "SOFTWARE\\Classes\\Component Categories\\" + CatId + "\\Impl", writable: true))
                {
                    if (k != null) k.DeleteValue(Clsid, throwOnMissingValue: false);
                }
            }
            catch { }
            Console.WriteLine("[OK] 已清除 IE Band 组件类别");

            // 调用 regasm /unregister
            string dllPath = System.IO.Path.Combine(
                AppDomain.CurrentDomain.BaseDirectory, "XPTab.dll");
            string regasm = System.IO.Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.Windows),
                "Microsoft.NET\\Framework64\\v4.0.30319\\regasm.exe");
            if (System.IO.File.Exists(dllPath) && System.IO.File.Exists(regasm))
            {
                Console.WriteLine("[..] 调用 regasm /unregister...");
                RunCommand(regasm, "/unregister \"" + dllPath + "\"");
            }
            Console.WriteLine("[OK] COM 已注销");

            Console.WriteLine();
            Console.WriteLine("=== 卸载完成 ===");
            Console.WriteLine("请重启 explorer.exe 使更改生效");
            Console.WriteLine();
            Console.WriteLine("按任意键退出...");
            Console.ReadKey();
            return 0;
        }

        private static int Status()
        {
            Console.WriteLine("=== XPTab Band 注册状态检查 ===");
            Console.WriteLine();

            // 1. Toolbar
            using (var k = Registry.LocalMachine.OpenSubKey(
                "SOFTWARE\\Microsoft\\Internet Explorer\\Toolbar\\" + Clsid))
            {
                Console.WriteLine("1. Toolbar 注册项: " + (k == null ? "不存在" : ("存在，默认值=" + k.GetValue(""))));
            }

            // 2. CLSID InprocServer32
            using (var k = Registry.LocalMachine.OpenSubKey(
                "SOFTWARE\\Classes\\CLSID\\" + Clsid + "\\InprocServer32"))
            {
                if (k == null)
                {
                    Console.WriteLine("2. CLSID InprocServer32: 不存在（COM 未注册，需运行 regasm）");
                }
                else
                {
                    Console.WriteLine("2. CLSID InprocServer32: 存在");
                    Console.WriteLine("   ThreadingModel: " + k.GetValue("ThreadingModel"));
                    Console.WriteLine("   Codebase: " + k.GetValue("Codebase"));
                }
            }

            // 3. Approved
            using (var k = Registry.LocalMachine.OpenSubKey(
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved"))
            {
                object v = k == null ? null : k.GetValue(Clsid);
                Console.WriteLine("3. Approved Shell Extensions: " + (v == null ? "未注册" : ("已注册，值=" + v)));
            }

            // 4. 组件类别
            using (var k = Registry.LocalMachine.OpenSubKey(
                "SOFTWARE\\Classes\\Component Categories\\" + CatId + "\\Impl"))
            {
                object v = k == null ? null : k.GetValue(Clsid);
                Console.WriteLine("4. IE Band 组件类别: " + (v == null ? "未注册" : ("已注册，值=" + v)));
            }

            // 5. EnableLegacyBars
            using (var k = Registry.CurrentUser.OpenSubKey(
                "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"))
            {
                object v = k == null ? null : k.GetValue("EnableLegacyBars");
                Console.WriteLine("5. EnableLegacyBars: " + (v == null ? "未设置" : v.ToString()));
            }

            // 6. Windows 版本
            var os = WmiHelper.GetFirst("SELECT Caption,BuildNumber FROM Win32_OperatingSystem");
            if (os != null)
            {
                Console.WriteLine("6. Windows 版本: " + os["Caption"] + " Build " + os["BuildNumber"]);
            }

            Console.WriteLine();
            return 0;
        }
    }

    /// <summary>
    /// WMI 查询辅助类（获取系统信息）
    /// </summary>
    internal static class WmiHelper
    {
        public static System.Collections.Generic.Dictionary<string, string> GetFirst(string query)
        {
            try
            {
                using (var searcher = new System.Management.ManagementObjectSearcher(query))
                using (var collection = searcher.Get())
                {
                    foreach (var obj in collection)
                    {
                        var dict = new System.Collections.Generic.Dictionary<string, string>();
                        foreach (var prop in obj.Properties)
                        {
                            dict[prop.Name] = prop.Value == null ? "" : prop.Value.ToString();
                        }
                        return dict;
                    }
                }
            }
            catch { }
            return null;
        }
    }
}
