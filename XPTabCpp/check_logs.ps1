$f = "$env:LOCALAPPDATA\XPTabCpp\inject_output.txt"
Write-Host "=== inject_output.txt ==="
if (Test-Path $f) {
    Get-Content $f -Encoding UTF8
} else {
    Write-Host "(not found)"
}
Write-Host ""
Write-Host "=== hook_log.txt (last 40 lines) ==="
$h = "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt"
if (Test-Path $h) {
    Get-Content $h -Encoding UTF8 -Tail 40
} else {
    Write-Host "(not found)"
}
