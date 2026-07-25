#requires -RunAsAdministrator
<#
.SYNOPSIS
    XPTab Explorer Band 注册脚本
.DESCRIPTION
    通过 regasm 注册 .NET COM 程序集，并写入 Explorer DeskBand 注册表项。
    注册后需重启 explorer.exe 才能加载 Band。
.NOTES
    必须以管理员身份运行。
    explorer.exe 是 64 位，必须用 64 位 regasm（Framework64 路径）。
#>

param(
    [string]$DllPath = ""
)

$ErrorActionPreference = "Stop"

# 定位 DLL
if ([string]::IsNullOrEmpty($DllPath)) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $DllPath = Join-Path $scriptDir "..\XPTab\bin\Release\XPTab.dll"
    if (-not (Test-Path $DllPath)) {
        $DllPath = Join-Path $scriptDir "..\build\XPTab.dll"
    }
}

if (-not (Test-Path $DllPath)) {
    Write-Host "找不到 XPTab.dll，请先编译项目。" -ForegroundColor Red
    Write-Host "尝试路径: $DllPath" -ForegroundColor Yellow
    Write-Host "用法: .\register.ps1 -DllPath 完整DLL路径"
    exit 1
}

$DllPath = (Resolve-Path $DllPath).Path
Write-Host "使用 DLL: $DllPath" -ForegroundColor Cyan

# 定位 64 位 regasm（.NET Framework 4.8）
$regasmPaths = @(
    "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\regasm.exe",
    "$env:WINDIR\Microsoft.NET\Framework64\v2.0.50727\regasm.exe"
)
$regasm = $regasmPaths | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $regasm) {
    Write-Host "找不到 64 位 regasm.exe，请确认已安装 .NET Framework 4.x" -ForegroundColor Red
    exit 1
}
Write-Host "使用 regasm: $regasm" -ForegroundColor Cyan

# 1. 通过 regasm 注册 .NET COM 程序集（/codebase 允许非 GAC 部署）
Write-Host "`n[1/3] 注册 COM 程序集..." -ForegroundColor Green
& $regasm "$DllPath" /codebase /tlb
if ($LASTEXITCODE -ne 0) {
    Write-Host "regasm 注册失败" -ForegroundColor Red
    exit 1
}

# 2. 读取 Band 的 CLSID
$assembly = [System.Reflection.Assembly]::LoadFrom($DllPath)
$bandType = $assembly.GetType("XPTab.XPTabBand")
$clsid = $bandType.GUID.ToString("B")
Write-Host "`n[2/3] XPTabBand CLSID: $clsid" -ForegroundColor Green

# 3. 写入 Explorer DeskBand 注册表项
Write-Host "`n[3/3] 写入 DeskBand 注册表项..." -ForegroundColor Green

# 3a. 注册为 Explorer 工具栏 Band
$bandKeyPath = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsid"
# 用 /f 和默认值一起创建键；PowerShell 用 Set-Item 设置默认值（空名）
New-Item -Path $bandKeyPath -Force | Out-Null
# 设置键的默认值（RegistryKey 的 SetValue 第一个参数为空字符串表示默认值）
$key = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsid", $true)
$key.SetValue("", "XPTab", [Microsoft.Win32.RegistryValueKind]::String)
$key.Close()
Write-Host "  已写入 Toolbar 注册项: $bandKeyPath"

# 3b. 注册到 Approved Shell Extensions（Win10/11 安全要求，未注册的 Band 不会出现在右键菜单）
$approvedPath = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"
if (-not (Test-Path $approvedPath)) {
    New-Item -Path $approvedPath -Force | Out-Null
}
$approvedKey = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved", $true)
$approvedKey.SetValue($clsid, "XPTab", [Microsoft.Win32.RegistryValueKind]::String)
$approvedKey.Close()
Write-Host "  已注册到 Approved Shell Extensions"

# 3c. 注册 IE Band 组件类别（关键！Win10 Explorer 通过此类别判断是否为合法 Band）
# CATID {00021493-0000-0000-C000-000000000046} = CommBand（水平工具栏 Band）
$catId = "{00021493-0000-0000-C000-000000000046}"
$catImplPath = "SOFTWARE\Classes\Component Categories\$catId\Impl"
$catKey = [Microsoft.Win32.Registry]::LocalMachine.CreateSubKey($catImplPath)
$catKey.SetValue($clsid, "XPTab", [Microsoft.Win32.RegistryValueKind]::String)
$catKey.Close()
Write-Host "  已注册 IE Band 组件类别 (CommBand)"

# 3d. 同时写入 Component Categories 的描述
$catDescPath = "SOFTWARE\Classes\Component Categories\$catId"
$catDescKey = [Microsoft.Win32.Registry]::LocalMachine.CreateSubKey($catDescPath)
$catDescKey.SetValue("0409", "Internet Toolbar", [Microsoft.Win32.RegistryValueKind]::String)
$catDescKey.SetValue("0804", "Internet 工具栏", [Microsoft.Win32.RegistryValueKind]::String)
$catDescKey.Close()

# 4. Windows 11 需启用旧版 DeskBand 支持
$legacyKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced"
if (-not (Test-Path $legacyKey)) {
    New-Item -Path $legacyKey -Force | Out-Null
}
$existing = (Get-ItemProperty -Path $legacyKey -Name "EnableLegacyBars" -ErrorAction SilentlyContinue).EnableLegacyBars
if ($existing -ne 1) {
    Set-ItemProperty -Path $legacyKey -Name "EnableLegacyBars" -Value 1 -Type DWord
    Write-Host "  已启用 Windows 11 旧版 DeskBand 支持 (EnableLegacyBars=1)" -ForegroundColor Yellow
}

Write-Host "`n注册完成！" -ForegroundColor Green
Write-Host "下一步：" -ForegroundColor Cyan
Write-Host "  1. 重启 explorer.exe（任务管理器→explorer→重启，或执行 stop-process -name explorer -force）"
Write-Host "  2. 在资源管理器工具栏空白处右键→勾选 XPTab"
Write-Host "  3. 或通过'查看'→'工具栏'→勾选 XPTab（Win10）"
