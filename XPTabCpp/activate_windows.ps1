Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class WinAct {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
}
"@
$hwnds = New-Object System.Collections.ArrayList
[WinAct]::EnumWindows({
    param($h, $l)
    $cls = New-Object System.Text.StringBuilder 256
    [WinAct]::GetClassName($h, $cls, 256) | Out-Null
    if ($cls.ToString() -eq "CabinetWClass") { $hwnds.Add($h) | Out-Null }
    return $true
}, [IntPtr]::Zero) | Out-Null
Write-Output "Found $($hwnds.Count) CabinetWClass windows"
foreach ($h in $hwnds) {
    Write-Output ("Activating 0x{0:X}" -f $h.ToInt64())
    [WinAct]::ShowWindow($h, 9) | Out-Null
    Start-Sleep -Milliseconds 300
    [WinAct]::SetForegroundWindow($h) | Out-Null
    Start-Sleep -Milliseconds 500
}
Start-Sleep -Seconds 3
Write-Output "`n=== debug_trace tail ==="
Get-Content "g:\Test\testFileExplorerPro\XPTabCpp\build\debug_trace.txt" -Tail 20 -Encoding UTF8
