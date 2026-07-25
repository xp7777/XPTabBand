$h = "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt"
if (Test-Path $h) {
    $fi = Get-Item $h
    Write-Host "File: $h"
    Write-Host "Size: $($fi.Length) bytes"
    Write-Host "LastWriteTime: $($fi.LastWriteTime)"
    Write-Host "Current time: $(Get-Date)"
    Write-Host ""
    Write-Host "=== Last 5 lines ==="
    Get-Content $h -Encoding UTF8 -Tail 5
} else {
    Write-Host "File not found: $h"
}
