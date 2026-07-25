$ErrorActionPreference = "Stop"
$url = "https://github.com/TsudaKageyu/minhook/archive/refs/heads/master.zip"
$zipPath = "$env:TEMP\minhook.zip"
$extractPath = "$env:TEMP\minhook_extract"
$destPath = "g:\Test\testFileExplorerPro\XPTabCpp\MinHook"

Write-Host "Downloading MinHook from GitHub..."
try {
    # 尝试使用 .NET WebClient
    $wc = New-Object System.Net.WebClient
    $wc.Headers.Add("User-Agent", "Mozilla/5.0")
    $wc.DownloadFile($url, $zipPath)
    Write-Host "Download complete: $((Get-Item $zipPath).Length) bytes"
} catch {
    Write-Host "WebClient failed: $_"
    Write-Host "Trying Invoke-WebRequest..."
    try {
        Invoke-WebRequest -Uri $url -OutFile $zipPath -UseBasicParsing -TimeoutSec 30
        Write-Host "Download complete: $((Get-Item $zipPath).Length) bytes"
    } catch {
        Write-Host "Invoke-WebRequest also failed: $_"
        Write-Host "Will use manually created MinHook files"
        exit 1
    }
}

Write-Host "Extracting..."
if (Test-Path $extractPath) { Remove-Item $extractPath -Recurse -Force }
Expand-Archive -Path $zipPath -DestinationPath $extractPath -Force

$srcFolder = Get-ChildItem $extractPath -Directory | Select-Object -First 1
Write-Host "Extracted folder: $($srcFolder.FullName)"

# 清理旧的 MinHook 目录
if (Test-Path $destPath) { Remove-Item $destPath -Recurse -Force }
New-Item -ItemType Directory -Path $destPath -Force | Out-Null

# 复制 MinHook.h
Copy-Item "$($srcFolder.FullName)\include\MinHook.h" "$destPath\MinHook.h"
Write-Host "Copied MinHook.h"

# 复制 src 目录下的文件（平铺）
$srcFiles = Get-ChildItem "$($srcFolder.FullName)\src" -Recurse -File
foreach ($f in $srcFiles) {
    Copy-Item $f.FullName "$destPath\$($f.Name)"
    Write-Host "Copied $($f.Name)"
}

Write-Host ""
Write-Host "=== MinHook files in $destPath ==="
Get-ChildItem $destPath | ForEach-Object { Write-Host "  $($_.Name)" }

# 清理
Remove-Item $zipPath -Force -ErrorAction SilentlyContinue
Remove-Item $extractPath -Recurse -Force -ErrorAction SilentlyContinue

Write-Host ""
Write-Host "MinHook installation complete!"
