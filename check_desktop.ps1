Write-Host "=== 检查桌面'此电脑'图标的注册表配置 ==="
$clsid = "{20D04FE0-3AEA-1069-A2D8-08002B30309D}"

Write-Host ""
Write-Host "--- 1. HKCR\CLSID\$clsid ---"
reg query "HKCR\CLSID\$clsid" 2>&1

Write-Host ""
Write-Host "--- 2. HKCR\CLSID\$clsid\Shell ---"
reg query "HKCR\CLSID\$clsid\Shell" 2>&1

Write-Host ""
Write-Host "--- 3. HKCR\CLSID\$clsid\Shell\Open ---"
reg query "HKCR\CLSID\$clsid\Shell\Open" 2>&1

Write-Host ""
Write-Host "--- 4. HKCR\CLSID\$clsid\Shell\Open\Command ---"
reg query "HKCR\CLSID\$clsid\Shell\Open\Command" 2>&1

Write-Host ""
Write-Host "--- 5. HKCR\CLSID\$clsid\Shell\OpenNewWindow ---"
reg query "HKCR\CLSID\$clsid\Shell\OpenNewWindow" 2>&1

Write-Host ""
Write-Host "--- 6. HKCR\CLSID\$clsid\Shell\OpenNewWindow\Command ---"
reg query "HKCR\CLSID\$clsid\Shell\OpenNewWindow\Command" 2>&1

Write-Host ""
Write-Host "--- 7. HKCR\CLSID\$clsid\ShellEx ---"
reg query "HKCR\CLSID\$clsid\ShellEx" 2>&1

Write-Host ""
Write-Host "=== 检查 HKCU 下是否有覆盖 ==="
reg query "HKCU\Software\Classes\CLSID\$clsid" 2>&1
reg query "HKCU\Software\Classes\CLSID\$clsid\Shell" 2>&1

Write-Host ""
Write-Host "=== 检查桌面图标策略 ==="
$desktopPolicyPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\HideDesktopIcons"
Write-Host "NewStartPanel:"
$hidden = Get-ItemProperty "$desktopPolicyPath\NewStartPanel" -ErrorAction SilentlyContinue
if ($hidden) { $hidden | Format-List } else { Write-Host "  不存在" }

Write-Host ""
Write-Host "=== 检查 Folder\shell\open 的子项 ==="
reg query "HKCR\Folder\shell" 2>&1
reg query "HKCR\Folder\shell\open" 2>&1
reg query "HKCR\Folder\shell\open\command" 2>&1
