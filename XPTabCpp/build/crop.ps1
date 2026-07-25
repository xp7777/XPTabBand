Add-Type -AssemblyName System.Drawing
$src = "G:\Test\testFileExplorerPro\XPTabCpp\build\screenshot_full.png"
$img = [System.Drawing.Image]::FromFile($src)
# TabBar 区域 (131,338)-(1289,368)，多裁一点上下文
$rect = New-Object System.Drawing.Rectangle(125, 300, 1200, 100)
$bmp = New-Object System.Drawing.Bitmap $rect.Width, $rect.Height
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.DrawImage($img, (New-Object System.Drawing.Rectangle(0,0,$rect.Width,$rect.Height)), $rect, [System.Drawing.GraphicsUnit]::Pixel)
$savePath = "G:\Test\testFileExplorerPro\XPTabCpp\build\tabbar_crop.png"
$bmp.Save($savePath, [System.Drawing.Imaging.ImageFormat]::Png)
$g.Dispose(); $bmp.Dispose(); $img.Dispose()
Write-Host "裁剪图: $savePath"
