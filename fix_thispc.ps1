# 修复桌面双击"此电脑"打不开的问题
# 原因：HKCR:\Folder\shell\open 的注册表项缺失或被篡改

Write-Host "=== 修复前检查 ==="
$folderOpenPath = "HKCR:\Folder\shell\open"
if (Test-Path $folderOpenPath) {
    Write-Host "[存在] $folderOpenPath"
    Get-Item $folderOpenPath | Format-List
} else {
    Write-Host "[缺失] $folderOpenPath - 这是问题根源"
}

Write-Host ""
Write-Host "=== 检查 HKCR:\Folder\shell\open\command ==="
$cmdPath = "HKCR:\Folder\shell\open\command"
if (Test-Path $cmdPath) {
    Write-Host "[存在] $cmdPath"
    Get-ItemProperty $cmdPath | Format-List
} else {
    Write-Host "[缺失] $cmdPath"
}

Write-Host ""
Write-Host "=== 检查 HKCR:\Drive\shell\open ==="
$drivePath = "HKCR:\Drive\shell"
if (Test-Path $drivePath) {
    Get-ChildItem $drivePath | ForEach-Object {
        Write-Host "子项: $($_.PSChildName)"
    }
}

Write-Host ""
Write-Host "=== 开始修复 ==="

# 方法1: 重建 HKCR:\Folder\shell\open\command
# 默认值应为: %SystemRoot%\Explorer.exe
Write-Host "重建 HKCR:\Folder\shell\open..."
if (-not (Test-Path $folderOpenPath)) {
    New-Item -Path $folderOpenPath -Force | Out-Null
    Write-Host "  已创建 $folderOpenPath"
}
Set-ItemProperty -Path $folderOpenPath -Name "(Default)" -Value "" -ErrorAction SilentlyContinue

Write-Host "重建 HKCR:\Folder\shell\open\command..."
if (-not (Test-Path $cmdPath)) {
    New-Item -Path $cmdPath -Force | Out-Null
    Write-Host "  已创建 $cmdPath"
}
# 设置默认命令为 explorer.exe
Set-ItemProperty -Path $cmdPath -Name "(Default)" -Value "%SystemRoot%\Explorer.exe"
# 设置 DelegateExecute 为标准的 ShellExec 调用
# 11dbb47c-a525-400b-9e80-a54615a090c0 是标准的 Folder 打开命令
$delegateClsid = "{11dbb47c-a525-400b-9e80-a54615a090c0}"
Set-ItemProperty -Path $cmdPath -Name "DelegateExecute" -Value $delegateClsid -Type String
Write-Host "  已设置 command 默认值和 DelegateExecute"

Write-Host ""
Write-Host "=== 修复后检查 ==="
Get-ItemProperty $cmdPath | Format-List

Write-Host ""
Write-Host "=== 修复 HKCR:\Drive\shell\open ==="
$driveOpenPath = "HKCR:\Drive\shell\open"
if (-not (Test-Path $driveOpenPath)) {
    New-Item -Path $driveOpenPath -Force | Out-Null
    Write-Host "  已创建 $driveOpenPath"
}
$driveCmdPath = "HKCR:\Drive\shell\open\command"
if (-not (Test-Path $driveCmdPath)) {
    New-Item -Path $driveCmdPath -Force | Out-Null
}
Set-ItemProperty -Path $driveCmdPath -Name "(Default)" -Value "%SystemRoot%\Explorer.exe"
Set-ItemProperty -Path $driveCmdPath -Name "DelegateExecute" -Value $delegateClsid -Type String
Write-Host "  已修复 Drive\shell\open\command"

Write-Host ""
Write-Host "=== 修复完成，请重启 explorer.exe 然后测试 ==="
Write-Host "停止 explorer..."
Stop-Process -Name explorer -Force
Start-Sleep -Seconds 2
Write-Host "启动 explorer..."
Start-Process explorer.exe
Start-Sleep -Seconds 3
Write-Host ""
Write-Host "=== 修复完成！现在请双击桌面'此电脑'测试 ==="
