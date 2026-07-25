Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;
public class WinApi {
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int n);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L,T,R,B; }
}
'@
$explorerWindows = Get-Process explorer | Where-Object { $_.MainWindowTitle -ne "" }
if (-not $explorerWindows) {
    Write-Host "未找到 Explorer 窗口，先打开一个..."
    Start-Process explorer.exe "C:\Windows"
    Start-Sleep -Seconds 3
    $explorerWindows = Get-Process explorer | Where-Object { $_.MainWindowTitle -ne "" }
}
if ($explorerWindows) {
    $hwnd = $explorerWindows[0].MainWindowHandle
    Write-Host "Explorer 窗口: HWND=$hwnd Title='$($explorerWindows[0].MainWindowTitle)'"
    $r2 = New-Object WinApi+RECT
    [WinApi]::GetWindowRect($hwnd, [ref]$r2) | Out-Null
    Write-Host "窗口矩形: L=$($r2.L) T=$($r2.T) R=$($r2.R) B=$($r2.B) W=$($r2.R-$r2.L) H=$($r2.B-$r2.T)"
    [WinApi]::ShowWindow($hwnd, 9) | Out-Null  # SW_RESTORE
    Start-Sleep -Milliseconds 200
    [WinApi]::SetForegroundWindow($hwnd) | Out-Null
    Start-Sleep -Milliseconds 500
    $r3 = New-Object WinApi+RECT
    [WinApi]::GetWindowRect($hwnd, [ref]$r3) | Out-Null
    $rect = New-Object System.Drawing.Rectangle($r3.L, $r3.T, $r3.R - $r3.L, $r3.B - $r3.T)
    $bmp = New-Object System.Drawing.Bitmap($rect.Width, $rect.Height)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen($r3.L, $r3.T, 0, 0, $bmp.Size)
    $savePath = "G:\Test\testFileExplorerPro\XPTabCpp\build\screenshot_tabbar_v4.png"
    $bmp.Save($savePath, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    Write-Host "截图已保存: $savePath"
} else {
    Write-Host "仍未找到 Explorer 窗口"
}
