# 使用 reg.exe 修复 HKCR 注册表项
# HKCR = HKEY_CLASSES_ROOT

Write-Host "=== 修复前确认问题 ==="
$out1 = reg query "HKCR\Folder\shell\open" 2>&1
Write-Host "HKCR\Folder\shell\open: $out1"

Write-Host ""
Write-Host "=== 修复 HKCR\Folder\shell\open\command ==="
# 创建 shell\open 项
reg add "HKCR\Folder\shell\open" /ve /d "" /f | Out-Null
# 创建 command 项，默认值为 explorer.exe
reg add "HKCR\Folder\shell\open\command" /ve /d "%SystemRoot%\Explorer.exe" /f | Out-Null
# 添加 DelegateExecute 指向标准 ShellExecute
reg add "HKCR\Folder\shell\open\command" /v "DelegateExecute" /t REG_SZ /d "{11dbb47c-a525-400b-9e80-a54615a090c0}" /f | Out-Null

Write-Host ""
Write-Host "=== 修复 HKCR\Drive\shell\open\command ==="
reg add "HKCR\Drive\shell\open" /ve /d "" /f | Out-Null
reg add "HKCR\Drive\shell\open\command" /ve /d "%SystemRoot%\Explorer.exe" /f | Out-Null
reg add "HKCR\Drive\shell\open\command" /v "DelegateExecute" /t REG_SZ /d "{11dbb47c-a525-400b-9e80-a54615a090c0}" /f | Out-Null

Write-Host ""
Write-Host "=== 修复后验证 ==="
Write-Host "--- HKCR\Folder\shell\open ---"
reg query "HKCR\Folder\shell\open"
Write-Host ""
Write-Host "--- HKCR\Folder\shell\open\command ---"
reg query "HKCR\Folder\shell\open\command"
Write-Host ""
Write-Host "--- HKCR\Drive\shell\open\command ---"
reg query "HKCR\Drive\shell\open\command"

Write-Host ""
Write-Host "=== 重启 explorer ==="
Stop-Process -Name explorer -Force
Start-Sleep -Seconds 2
Start-Process explorer.exe
Start-Sleep -Seconds 3

Write-Host ""
Write-Host "=== 修复完成！现在请双击桌面'此电脑'测试 ==="
