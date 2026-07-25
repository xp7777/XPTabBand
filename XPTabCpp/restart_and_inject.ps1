Write-Output "=== Restarting explorer.exe ==="
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Write-Output "Explorer stopped, waiting for restart..."
Start-Sleep -Seconds 2

# Check if explorer restarted automatically
$explorerProcs = Get-Process explorer -ErrorAction SilentlyContinue
if ($explorerProcs) {
    Write-Output "Explorer restarted automatically: $($explorerProcs.Count) processes"
} else {
    Write-Output "Starting explorer.exe manually..."
    Start-Process explorer.exe
    Start-Sleep -Seconds 2
}

Write-Output ""
Write-Output "=== Current explorer processes ==="
Get-Process explorer | Select-Object Id, ProcessName, StartTime | Format-Table -AutoSize

Write-Output ""
Write-Output "=== Clearing old logs ==="
Remove-Item "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt" -Force -ErrorAction SilentlyContinue
Remove-Item "g:\Test\testFileExplorerPro\XPTabCpp\build\debug_trace.txt" -Force -ErrorAction SilentlyContinue
Write-Output "Logs cleared"

Write-Output ""
Write-Output "=== Injecting new DLL ==="
& "g:\Test\testFileExplorerPro\XPTabCpp\build\XPTabInject.exe" -install 2>&1

Write-Output ""
Write-Output "=== Waiting 5 seconds for hook to activate ==="
Start-Sleep -Seconds 5

Write-Output ""
Write-Output "=== hook_log.txt ==="
$logPath = "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt"
if (Test-Path $logPath) { Get-Content $logPath } else { Write-Output "(not found)" }

Write-Output ""
Write-Output "=== debug_trace.txt ==="
$tracePath = "g:\Test\testFileExplorerPro\XPTabCpp\build\debug_trace.txt"
if (Test-Path $tracePath) { Get-Content $tracePath } else { Write-Output "(not found)" }
