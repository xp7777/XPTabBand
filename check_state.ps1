Write-Host "=== explorer 进程列表 ==="
Get-Process explorer | Select-Object Id, StartTime, MainWindowTitle | Format-Table -AutoSize

Write-Host ""
Write-Host "=== 测试 1: 用 explorer.exe 打开此电脑 CLSID ==="
Write-Host "执行: explorer.exe ::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"
Start-Process explorer.exe -ArgumentList "::{20D04FE0-3AEA-1069-A2D8-08002B30309D}"
Start-Sleep -Seconds 3

Write-Host ""
Write-Host "=== 测试后 explorer 进程列表 ==="
Get-Process explorer | Select-Object Id, StartTime, MainWindowTitle | Format-Table -AutoSize

Write-Host ""
Write-Host "=== 测试 2: 用 explorer.exe 打开 C: 盘 ==="
Start-Process explorer.exe -ArgumentList "C:\"
Start-Sleep -Seconds 3

Write-Host ""
Write-Host "=== 测试后 explorer 进程列表 ==="
Get-Process explorer | Select-Object Id, StartTime, MainWindowTitle | Format-Table -AutoSize

Write-Host ""
Write-Host "=== 检查 ExplorerAdvanced 设置 ==="
$advPath = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Advanced"
if (Test-Path $advPath) {
    Get-ItemProperty $advPath | Select-Object SeparateProcess, LaunchTo, FolderContentsInfoTip | Format-List
}

Write-Host ""
Write-Host "=== 检查是否有残留的注入器/explorer 异常进程 ==="
Get-Process | Where-Object { $_.ProcessName -match "explorer|XPTab" } | Select-Object Id, ProcessName, Path | Format-Table -AutoSize
