$h = "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt"
if (Test-Path $h) { Remove-Item $h -Force; Write-Host "Deleted old hook_log.txt" }
$f = "$env:LOCALAPPDATA\XPTabCpp\inject_output.txt"
if (Test-Path $f) { Remove-Item $f -Force; Write-Host "Deleted old inject_output.txt" }
$b = "g:\Test\testFileExplorerPro\XPTabCpp\build\inject_output.txt"
if (Test-Path $b) { Remove-Item $b -Force; Write-Host "Deleted old build inject_output.txt" }
Write-Host "Done"
