Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class WinEnum {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    public static void Run() {
        var results = new List<string>();
        EnumWindows((hWnd, lParam) => {
            var sb = new StringBuilder(256);
            GetClassName(hWnd, sb, 256);
            if (sb.ToString() == "CabinetWClass") {
                uint pid;
                uint tid = GetWindowThreadProcessId(hWnd, out pid);
                results.Add(string.Format("hwnd=0x{0:X} PID={1} TID={2} visible={3}", hWnd.ToInt64(), pid, tid, IsWindowVisible(hWnd)));
            }
            return true;
        }, IntPtr.Zero);
        foreach (var r in results) Console.WriteLine(r);
    }
}
"@

Write-Output "=== All explorer.exe processes ==="
Get-Process explorer -ErrorAction SilentlyContinue | Select-Object Id, StartTime | Sort-Object Id | Format-Table -AutoSize

Write-Output "=== CabinetWClass windows ==="
[WinEnum]::Run()

Write-Output "=== Which explorer.exe has XPTabHook.dll ==="
Get-Process explorer -ErrorAction SilentlyContinue | ForEach-Object {
    $mod = $_.Modules | Where-Object { $_.ModuleName -eq "XPTabHook.dll" }
    if ($mod) { Write-Output "PID $($_.Id): XPTabHook.dll loaded" }
    else { Write-Output "PID $($_.Id): no XPTabHook.dll" }
}
