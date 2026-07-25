Write-Host "等待 3 秒让定时器捕获窗口..."
Start-Sleep -Seconds 3
$h = "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt"
Write-Host "=== hook_log.txt (full) ==="
if (Test-Path $h) {
    Get-Content $h -Encoding UTF8
} else {
    Write-Host "(not found)"
}
