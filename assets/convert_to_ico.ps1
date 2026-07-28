# Convert icon source image to multi-size ICO file
# Generates ICO with embedded PNG data for each size (Vista+ supported)
$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing

$srcPath = 'g:\Test\testFileExplorerPro\assets\icon_source.jpg'
$icoPath = 'g:\Test\testFileExplorerPro\assets\XPTabBand.ico'
$sizes = @(256, 128, 64, 48, 32, 16)

Write-Host "Loading source: $srcPath"
$src = [System.Drawing.Image]::FromFile($srcPath)
Write-Host "Source size: $($src.Width) x $($src.Height)"

# Collect PNG bytes for each size
$pngBytesList = New-Object System.Collections.ArrayList
foreach ($size in $sizes) {
    $bmp = New-Object System.Drawing.Bitmap $size, $size
    $bmp.SetResolution(96, 96)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $g.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality
    $g.Clear([System.Drawing.Color]::FromArgb(32, 32, 32))
    $g.DrawImage($src, 0, 0, $size, $size)
    $g.Dispose()

    $ms = New-Object System.IO.MemoryStream
    $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
    [void]$pngBytesList.Add($ms.ToArray())
    Write-Host "  Generated ${size}x${size}: $($ms.ToArray().Length) bytes"
    $bmp.Dispose()
    $ms.Dispose()
}
$src.Dispose()

# Build ICO file
$fs = [System.IO.File]::Create($icoPath)
$bw = New-Object System.IO.BinaryWriter $fs

# ICONDIR header (6 bytes)
$bw.Write([UInt16]0)                # reserved
$bw.Write([UInt16]1)                # type = 1 (icon)
$bw.Write([UInt16]$sizes.Count)     # image count

# Compute data offset (header + entries)
$headerSize = 6
$entriesSize = 16 * $sizes.Count
$dataOffset = $headerSize + $entriesSize

# Write ICONDIRENTRY for each size (16 bytes each)
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $s = $sizes[$i]
    $bytes = $pngBytesList[$i]
    $w = if ($s -eq 256) { [byte]0 } else { [byte]$s }
    $bw.Write($w)                       # width (0 = 256)
    $bw.Write($w)                       # height (0 = 256)
    $bw.Write([byte]0)                  # color count (0 = no palette)
    $bw.Write([byte]0)                  # reserved
    $bw.Write([UInt16]1)                # color planes
    $bw.Write([UInt16]32)               # bits per pixel
    $bw.Write([UInt32]$bytes.Length)    # bytes in resource
    $bw.Write([UInt32]$dataOffset)      # offset to image data
    $dataOffset += $bytes.Length
}

# Write PNG data for each size
for ($i = 0; $i -lt $sizes.Count; $i++) {
    $bw.Write($pngBytesList[$i])
}

$bw.Flush()
$bw.Close()
$fs.Close()

$icoFile = Get-Item $icoPath
Write-Host ""
Write-Host "ICO created: $icoPath" -ForegroundColor Green
Write-Host "  Size: $($icoFile.Length) bytes"
