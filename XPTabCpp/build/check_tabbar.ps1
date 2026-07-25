Add-Type @'
using System;
using System.Runtime.InteropServices;
using System.Text;
public class W {
    public delegate bool EnumWindowsProc(IntPtr h, IntPtr p);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc p, IntPtr l);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr h, EnumWindowsProc p, IntPtr l);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
    [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr h);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
}
'@
$found = New-Object System.Collections.ArrayList
$proc = New-Object W -ErrorAction SilentlyContinue
$callback = [W+EnumWindowsProc]{
    param($h, $l)
    $sb = New-Object System.Text.StringBuilder 256
    [W]::GetClassName($h, $sb, 256) | Out-Null
    $cls = $sb.ToString()
    if ($cls -eq "XPTabBarClass") {
        $r = New-Object W+RECT
        [W]::GetWindowRect($h, [ref]$r) | Out-Null
        $parent = [W]::GetParent($h)
        $vis = [W]::IsWindowVisible($h)
        $found.Add("XPTabBarClass HWND=0x$($h.ToString('X')) Parent=0x$($parent.ToString('X')) Visible=$vis Rect=($($r.L),$($r.T))-($($r.R),$($r.B)) W=$($r.R-$r.L) H=$($r.B-$r.T)") | Out-Null
    }
    return $true
}
[W]::EnumWindows($callback, [IntPtr]::Zero) | Out-Null

# 也枚举所有 CabinetWClass 窗口的子窗口
$cabCallback = [W+EnumWindowsProc]{
    param($h, $l)
    $sb = New-Object System.Text.StringBuilder 256
    [W]::GetClassName($h, $sb, 256) | Out-Null
    if ($sb.ToString() -eq "CabinetWClass") {
        Write-Host "CabinetWClass HWND=0x$($h.ToString('X'))"
        $childCb = [W+EnumWindowsProc]{
            param($ch, $cl)
            $csb = New-Object System.Text.StringBuilder 256
            [W]::GetClassName($ch, $csb, 256) | Out-Null
            $cc = $csb.ToString()
            if ($cc -eq "XPTabBarClass" -or $cc -eq "ShellTabWindowClass") {
                $r = New-Object W+RECT
                [W]::GetWindowRect($ch, [ref]$r) | Out-Null
                $vis = [W]::IsWindowVisible($ch)
                Write-Host "  子窗口: $cc HWND=0x$($ch.ToString('X')) Visible=$vis Rect=($($r.L),$($r.T))-($($r.R),$($r.B)) W=$($r.R-$r.L) H=$($r.B-$r.T)"
            }
            return $true
        }
        [W]::EnumChildWindows($h, $childCb, [IntPtr]::Zero) | Out-Null
    }
    return $true
}
[W]::EnumWindows($cabCallback, [IntPtr]::Zero) | Out-Null

Write-Host "`n=== 顶层 XPTabBarClass 窗口 ==="
if ($found.Count -gt 0) {
    $found | ForEach-Object { Write-Host $_ }
} else {
    Write-Host "未找到任何 XPTabBarClass 窗口"
}
