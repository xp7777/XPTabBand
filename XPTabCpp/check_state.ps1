Write-Output "=== inject_output.txt ==="
$injectLog = "g:\Test\testFileExplorerPro\XPTabCpp\build\inject_output.txt"
if (Test-Path $injectLog) { Get-Content $injectLog } else { Write-Output "not found" }

Write-Output ""
Write-Output "=== Current explorer.exe processes ==="
Get-Process explorer | Select-Object Id, ProcessName, StartTime, MainWindowTitle | Format-Table -AutoSize

Write-Output ""
Write-Output "=== CabinetWClass windows ==="
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class WinU2 {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    public static void Run() {
        EnumWindows((hWnd, lParam) => {
            var cls = new StringBuilder(256);
            GetClassName(hWnd, cls, 256);
            if (cls.ToString() == "CabinetWClass") {
                uint pid;
                uint tid = GetWindowThreadProcessId(hWnd, out pid);
                var title = new StringBuilder(256);
                GetWindowText(hWnd, title, 256);
                Console.WriteLine("hwnd=0x{0:X} pid={1} tid={2} visible={3} title={4}",
                    hWnd.ToInt64(), pid, tid, IsWindowVisible(hWnd), title);
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@
[WinU2]::Run()
