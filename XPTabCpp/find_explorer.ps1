Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;

public class WinUtil {
    [DllImport("user32.dll")]
    public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetClassName(IntPtr hWnd, StringBuilder lpClassName, int nMaxCount);

    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr hWnd);

    [DllImport("user32.dll", CharSet = CharSet.Unicode)]
    public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
}
"@

$found = 0
[WinUtil]::EnumWindows({
    param($hWnd, $lParam)
    $sb = New-Object System.Text.StringBuilder(256)
    [WinUtil]::GetClassName($hWnd, $sb, 256) | Out-Null
    $cls = $sb.ToString()
    if ($cls -eq "CabinetWClass") {
        $title = New-Object System.Text.StringBuilder(256)
        [WinUtil]::GetWindowText($hWnd, $title, 256) | Out-Null
        $vis = [WinUtil]::IsWindowVisible($hWnd)
        Write-Host "CabinetWClass: HWND=0x$($hWnd.ToString('X')) Title='$($title.ToString())' Visible=$vis"
        $script:found++
    }
    return $true
}, [IntPtr]::Zero) | Out-Null
Write-Host "Total CabinetWClass windows found: $found"
