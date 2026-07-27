# 以管理员身份完整注册
$ErrorActionPreference = "Stop"

$dllPath = "g:\Test\testFileExplorerPro\XPTabCpp\XPTabBand\build\XPTabBand.dll"
$clsid = "{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}"

Write-Host "=== 完整注册 XPTabBand ==="
Write-Host "DLL: $dllPath"
Write-Host "CLSID: $clsid"
Write-Host ""

# 1. 删除旧注册
Write-Host "1. 清理旧注册..."
Remove-Item -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid" -Recurse -ErrorAction SilentlyContinue
Remove-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar" -Name $clsid -ErrorAction SilentlyContinue

# 2. 注册 CLSID
Write-Host "2. 注册 CLSID..."
New-Item -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid" -Force | Out-Null
Set-ItemProperty -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid" -Name "(default)" -Value "XPTabBand"

New-Item -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" -Force | Out-Null
Set-ItemProperty -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" -Name "(default)" -Value $dllPath
Set-ItemProperty -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" -Name "ThreadingModel" -Value "Apartment"

# 3. 注册 Toolbar
Write-Host "3. 注册 Explorer Toolbar..."
Set-ItemProperty -Path "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar" -Name $clsid -Value "XPTabBand" -Type String

# 4. 验证
Write-Host ""
Write-Host "=== 验证 ==="
Write-Host "CLSID InprocServer32:"
Get-ItemProperty "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" | Format-List

Write-Host "Toolbar:"
$tb = Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar" -Name $clsid
Write-Host "  $clsid = $($tb.$clsid)"

# 5. 重启 explorer
Write-Host ""
Write-Host "=== 重启 explorer ==="
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Start-Process explorer.exe
Start-Sleep -Seconds 3

Write-Host ""
Write-Host "=== 完成 ==="
Write-Host "请打开新 Explorer 窗口测试"
Write-Host "在 查看 -> 工具栏 中应能看到 XPTabBand 选项"

Start-Sleep -Seconds 2
