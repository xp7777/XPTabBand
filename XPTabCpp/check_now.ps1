Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class WinU6 {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    public static void Run() {
        EnumWindows((hWnd, lParam) => {
            var cls = new StringBuilder(256);
            GetClassName(hWnd, cls, 256);
            if (cls.ToString() == "CabinetWClass") {
                uint pid;
                uint tid = GetWindowThreadProcessId(hWnd, out pid);
                var title = new StringBuilder(256);
                GetWindowText(hWnd, title, 256);
                Console.WriteLine("Cabinet: hwnd=0x{0:X} pid={1} tid={2} title={3}",
                    hWnd.ToInt64(), pid, tid, title);
                EnumChildWindows(hWnd, (hWnd2, lParam2) => {
                    var cls2 = new StringBuilder(256);
                    GetClassName(hWnd2, cls2, 256);
                    if (cls2.ToString() == "XPTabBarClass") {
                        Console.WriteLine("  TabBar FOUND: hwnd=0x{0:X}", hWnd2.ToInt64());
                    }
                    return true;
                }, IntPtr.Zero);
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@
Write-Output "=== Cabinet windows ==="
[WinU6]::Run()

Write-Output ""
Write-Output "=== Hook log (last 10) ==="
Get-Content "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt" -Tail 10

Write-Output ""
Write-Output "=== Trace (last 10) ==="
Get-Content "g:\Test\testFileExplorerPro\XPTabCpp\build\debug_trace.txt" -Tail 10
