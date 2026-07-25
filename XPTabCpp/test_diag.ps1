Write-Output "=== debug_trace (CreateWindowEx 失败诊断) ==="
Select-String -Path "g:\Test\testFileExplorerPro\XPTabCpp\build\debug_trace.txt" -Pattern "CreateWindowEx|RegisterTabBarClass|classFound" -SimpleMatch | Select-Object -Last 10

Write-Output "`n=== hook_log tail ==="
Get-Content "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt" -Encoding UTF8 -Tail 10

Write-Output "`n=== TabBar 检查 ==="
& "g:\Test\testFileExplorerPro\XPTabCpp\check_children.ps1" | Select-String -Pattern "XPTabBarClass|ShellTabWindowClass" -SimpleMatch
