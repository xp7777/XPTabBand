#requires -RunAsAdministrator
<#
.SYNOPSIS
    XPTab Explorer Band 卸载脚本
.DESCRIPTION
    注销 COM 程序集并清除 Explorer DeskBand 注册表项。
#>

param(
    [string]$DllPath = ""
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrEmpty($DllPath)) {
    $scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
    $DllPath = Join-Path $scriptDir "..\XPTab\bin\Release\XPTab.dll"
    if (-not (Test-Path $DllPath)) {
        $DllPath = Join-Path $scriptDir "..\build\XPTab.dll"
    }
}

# 定位 64 位 regasm
$regasmPaths = @(
    "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\regasm.exe",
    "$env:WINDIR\Microsoft.NET\Framework64\v2.0.50727\regasm.exe"
)
$regasm = $regasmPaths | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $regasm) {
    Write-Host "找不到 64 位 regasm.exe" -ForegroundColor Red
    exit 1
}

# 获取 CLSID（从已注册类型或 DLL）
$clsid = $null
if (Test-Path $DllPath) {
    try {
        $assembly = [System.Reflection.Assembly]::LoadFrom($DllPath)
        $bandType = $assembly.GetType("XPTab.XPTabBand")
        if ($bandType) { $clsid = $bandType.GUID.ToString("B") }
    } catch {}
}

if (-not $clsid) {
    # 从注册表查找已注册的 CLSID
    $clsidKey = Get-ChildItem "HKLM:\SOFTWARE\Classes\CLSID" -ErrorAction SilentlyContinue | 
        Where-Object { (Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue)."(default)" -eq "XPTabBand" } |
        Select-Object -First 1
    if ($clsidKey) {
        $clsid = Split-Path $clsidKey.PSChildName -Leaf
    }
}

if (-not $clsid) {
    Write-Host "无法确定 CLSID，跳过注册表清理" -ForegroundColor Yellow
} else {
    Write-Host "XPTabBand CLSID: $clsid" -ForegroundColor Cyan

    # 清除 Toolbar Band 注册项
    $bandKeyPath = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsid"
    if (Test-Path $bandKeyPath) {
        Remove-Item -Path $bandKeyPath -Recurse -Force
        Write-Host "已清除 Toolbar 注册项"
    }

    # 清除 COM CLSID 注册项
    $clsidRegPath = "HKLM:\SOFTWARE\Classes\CLSID\$clsid"
    if (Test-Path $clsidRegPath) {
        Remove-Item -Path $clsidRegPath -Recurse -Force
        Write-Host "已清除 COM CLSID 注册项"
    }

    # 清除 Approved Shell Extensions 注册项
    $approvedKey = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved", $true)
    if ($approvedKey) {
        $approvedKey.DeleteValue($clsid, $false)
        $approvedKey.Close()
        Write-Host "已清除 Approved Shell Extensions"
    }

    # 清除 IE Band 组件类别注册
    $catId = "{00021493-0000-0000-C000-000000000046}"
    $catImplKey = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SOFTWARE\Classes\Component Categories\$catId\Impl", $true)
    if ($catImplKey) {
        $catImplKey.DeleteValue($clsid, $false)
        $catImplKey.Close()
        Write-Host "已清除 IE Band 组件类别"
    }
}

# 注销 COM 程序集
if (Test-Path $DllPath) {
    Write-Host "`n注销 COM 程序集..." -ForegroundColor Green
    & $regasm "$DllPath" /unregister
}

Write-Host "`n卸载完成！" -ForegroundColor Green
Write-Host "请重启 explorer.exe 使更改生效（stop-process -name explorer -force）" -ForegroundColor Cyan
