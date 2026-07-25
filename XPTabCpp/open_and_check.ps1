Write-Output "=== Opening Explorer window ==="
Start-Process explorer.exe "C:\Windows"
Start-Sleep -Seconds 3

Write-Output ""
Write-Output "=== hook_log.txt (last 20 lines) ==="
$logPath = "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt"
if (Test-Path $logPath) { Get-Content $logPath -Tail 20 } else { Write-Output "(not found)" }

Write-Output ""
Write-Output "=== debug_trace.txt (last 15 lines) ==="
$tracePath = "g:\Test\testFileExplorerPro\XPTabCpp\build\debug_trace.txt"
if (Test-Path $tracePath) { Get-Content $tracePath -Tail 15 } else { Write-Output "(not found)" }

Write-Output ""
Write-Output "=== CabinetWClass windows ==="
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class WinU3 {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
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
                Console.WriteLine("hwnd=0x{0:X} pid={1} tid={2} visible={3} title={4}",
                    hWnd.ToInt64(), pid, tid, IsWindowVisible(hWnd), title);
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@
[WinU3]::Run()
