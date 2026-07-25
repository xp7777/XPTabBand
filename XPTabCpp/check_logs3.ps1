$h = "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt"
Write-Host "=== hook_log.txt (last 15 lines) ==="
if (Test-Path $h) {
    Get-Content $h -Encoding UTF8 -Tail 15
} else {
    Write-Host "(not found)"
}
Write-Host ""
Write-Host "=== inject_output.txt ==="
$b = "g:\Test\testFileExplorerPro\XPTabCpp\build\inject_output.txt"
if (Test-Path $b) {
    Get-Content $b -Encoding Unicode
} else {
    Write-Host "(not found in build dir)"
}
