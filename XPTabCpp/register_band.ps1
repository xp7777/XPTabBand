# 注册/卸载 XPTabBand DeskBand
param(
    [string]$action = "status"
)

$dllPath = "g:\Test\testFileExplorerPro\XPTabCpp\XPTabBand\build\XPTabBand.dll"
$clsid = "{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}"

if ($action -eq "status") {
    Write-Host "=== XPTabBand 注册状态 ==="
    $toolbarKey = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar"
    $toolbarVal = (Get-ItemProperty $toolbarKey -Name $clsid -ErrorAction SilentlyContinue).$clsid
    if ($toolbarVal) {
        Write-Host "Toolbar: 已注册 = $toolbarVal" -ForegroundColor Green
    } else {
        Write-Host "Toolbar: 未注册" -ForegroundColor Red
    }
    if (Test-Path $dllPath) {
        Write-Host "DLL: 存在 ($((Get-Item $dllPath).LastWriteTime))"
    } else {
        Write-Host "DLL: 不存在"
    }
    return
}

# 检查管理员权限
$current = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal($current)
$isAdmin = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "需要管理员权限，正在提权..."
    Start-Process powershell -ArgumentList "-NoProfile","-ExecutionPolicy","Bypass","-File","`"$PSCommandPath`"","-action",$action -Verb RunAs
    exit
}

if ($action -eq "register") {
    Write-Host "=== 注册 XPTabBand ==="
    Write-Host "DLL: $dllPath"

    # 使用 regsvr32 注册
    Write-Host "执行 regsvr32..."
    $p = Start-Process regsvr32 -ArgumentList "/s `"$dllPath`"" -Wait -PassThru -NoNewWindow 2>$null
    if ($p) { Write-Host "regsvr32 退出码: $($p.ExitCode)" }

    # 验证并补充注册
    $toolbarKey = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar"
    $toolbarVal = (Get-ItemProperty $toolbarKey -Name $clsid -ErrorAction SilentlyContinue).$clsid
    if (-not $toolbarVal) {
        Write-Host "regsvr32 失败，手动注册..."
        # 手动写注册表
        New-Item -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" -Force | Out-Null
        Set-ItemProperty -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" -Name "(default)" -Value $dllPath
        Set-ItemProperty -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" -Name "ThreadingModel" -Value "Apartment"
        Set-ItemProperty -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid" -Name "(default)" -Value "XPTabBand"
        Set-ItemProperty -Path $toolbarKey -Name $clsid -Value "XPTabBand" -Type String
    }

    # 验证
    $toolbarVal = (Get-ItemProperty $toolbarKey -Name $clsid -ErrorAction SilentlyContinue).$clsid
    if ($toolbarVal) {
        Write-Host "注册成功" -ForegroundColor Green
    } else {
        Write-Host "注册失败" -ForegroundColor Red
    }

    Write-Host "重启 explorer..."
    Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2
    Start-Process explorer.exe
    Start-Sleep -Seconds 3
    Write-Host "完成。打开 Explorer -> 查看 -> 工具栏 -> 勾选 XPTabBand"
}

if ($action -eq "unregister") {
    Write-Host "=== 卸载 XPTabBand ==="
    Start-Process regsvr32 -ArgumentList "/u /s `"$dllPath`"" -Wait -NoNewWindow 2>$null

    # 手动清理
    $toolbarKey = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar"
    Remove-ItemProperty -Path $toolbarKey -Name $clsid -ErrorAction SilentlyContinue
    Remove-Item -Path "HKLM:\SOFTWARE\Classes\CLSID\$clsid" -Recurse -ErrorAction SilentlyContinue

    Write-Host "卸载完成"
    Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 2
    Start-Process explorer.exe
}
