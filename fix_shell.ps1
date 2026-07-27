# 修复 HKCR\CLSID\{20D04FE0-...}\Shell 的默认值
# 当前值是 "none"，导致双击桌面"此电脑"无反应
# 正常应该没有默认值，或为空

$shellPath = "HKCR:\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\Shell"
Write-Host "修复前:"
reg query $shellPath.Replace("HKCR:\","HKCR\")

Write-Host ""
Write-Host "删除默认值..."
reg delete $shellPath.Replace("HKCR:\","HKCR\") /ve /f

Write-Host ""
Write-Host "修复后:"
reg query $shellPath.Replace("HKCR:\","HKCR\")

Write-Host ""
Write-Host "重启 explorer..."
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Start-Process explorer.exe
Start-Sleep -Seconds 3

Write-Host ""
Write-Host "修复完成！请双击桌面'此电脑'测试"
