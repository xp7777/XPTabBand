Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
$proc = Get-Process explorer | Where-Object { $_.MainWindowTitle -ne "" } | Select-Object -First 1
if (-not $proc) {
    Start-Process explorer.exe "C:\Windows"
    Start-Sleep -Seconds 3
    $proc = Get-Process explorer | Where-Object { $_.MainWindowTitle -ne "" } | Select-Object -First 1
}
if ($proc) {
    Write-Host "Title: $($proc.MainWindowTitle)"
    # 激活窗口
    $null = [Microsoft.VisualBasic.Interaction]::AppActivate($proc.Id) 2>$null
    Add-Type -AssemblyName Microsoft.VisualBasic
    Start-Sleep -Milliseconds 500
    # 用整个屏幕截图，然后裁剪
    $bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
    $bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.CopyFromScreen(0, 0, 0, 0, $bmp.Size)
    $savePath = "G:\Test\testFileExplorerPro\XPTabCpp\build\screenshot_full.png"
    $bmp.Save($savePath, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose(); $bmp.Dispose()
    Write-Host "全屏截图: $savePath"
}
