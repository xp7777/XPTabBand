Write-Output "=== Step 1: Restart explorer.exe ==="
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 3
$proc = Get-Process explorer -ErrorAction SilentlyContinue
if (-not $proc) {
    Start-Process explorer.exe
    Start-Sleep -Seconds 2
}
Write-Output "Explorer processes: $((Get-Process explorer).Count)"

Write-Output ""
Write-Output "=== Step 2: Clear logs ==="
Remove-Item "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt" -Force -ErrorAction SilentlyContinue
Remove-Item "g:\Test\testFileExplorerPro\XPTabCpp\build\debug_trace.txt" -Force -ErrorAction SilentlyContinue

Write-Output ""
Write-Output "=== Step 3: Inject DLL ==="
& "g:\Test\testFileExplorerPro\XPTabCpp\build\XPTabInject.exe" -install 2>&1
Start-Sleep -Seconds 2

Write-Output ""
Write-Output "=== Step 4: Open Explorer window ==="
Start-Process explorer.exe "C:\Windows"
Start-Sleep -Seconds 3

Write-Output ""
Write-Output "=== Step 5: Inject again (for new window process) ==="
& "g:\Test\testFileExplorerPro\XPTabCpp\build\XPTabInject.exe" -install 2>&1
Start-Sleep -Seconds 3

Write-Output ""
Write-Output "=== Step 6: Activate window to trigger events ==="
Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class WinU5 {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    public static void Activate() {
        EnumWindows((hWnd, lParam) => {
            var cls = new StringBuilder(256);
            GetClassName(hWnd, cls, 256);
            if (cls.ToString() == "CabinetWClass") {
                ShowWindow(hWnd, 9);
                SetForegroundWindow(hWnd);
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@
[WinU5]::Activate()
Start-Sleep -Seconds 3

Write-Output ""
Write-Output "=== Step 7: Check logs ==="
$logPath = "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt"
Write-Output "--- hook_log.txt ---"
if (Test-Path $logPath) { Get-Content $logPath -Tail 25 } else { Write-Output "(not found)" }

Write-Output ""
$tracePath = "g:\Test\testFileExplorerPro\XPTabCpp\build\debug_trace.txt"
Write-Output "--- debug_trace.txt ---"
if (Test-Path $tracePath) { Get-Content $tracePath -Tail 25 } else { Write-Output "(not found)" }
