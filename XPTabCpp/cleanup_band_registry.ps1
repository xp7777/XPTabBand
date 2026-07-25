#requires -RunAsAdministrator
<#
.SYNOPSIS
    清理方案 B (DeskBand) 残留的注册表项
.DESCRIPTION
    方案 C (DLL 注入) 不需要 COM 注册。
    此脚本删除 XPTab.XPTabBand 的所有 COM/Band 注册，
    让"查看 -> 选项"里不再显示 XPTab 项。
.NOTES
    需要以管理员身份运行。
#>

$ErrorActionPreference = "Continue"
$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"

Write-Host "=== 清理 XPTab DeskBand 残留注册表 ===" -ForegroundColor Cyan
Write-Host "CLSID: $clsid`n"

# 1. COM CLSID 主项
$paths = @(
    "HKLM:\SOFTWARE\Classes\CLSID\$clsid",
    "HKLM:\SOFTWARE\Classes\WOW6432Node\CLSID\$clsid",
    "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsid",
    "HKLM:\SOFTWARE\Classes\XPTab.XPTabBand",
    "HKLM:\SOFTWARE\Classes\WOW6432Node\XPTab.XPTabBand"
)

foreach ($p in $paths) {
    if (Test-Path $p) {
        try {
            Remove-Item -Path $p -Recurse -Force -ErrorAction Stop
            Write-Host "[OK]   已删除: $p" -ForegroundColor Green
        } catch {
            Write-Host "[FAIL] 删除失败: $p - $($_.Exception.Message)" -ForegroundColor Red
        }
    } else {
        Write-Host "[SKIP] 不存在: $p" -ForegroundColor Gray
    }
}

# 2. Approved Shell Extensions 中的值
$approved = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"
if (Test-Path $approved) {
    try {
        Remove-ItemProperty -Path $approved -Name $clsid -ErrorAction Stop
        Write-Host "[OK]   已删除 Approved Shell Extensions 值" -ForegroundColor Green
    } catch {
        Write-Host "[SKIP] Approved Shell Extensions 值不存在或删除失败" -ForegroundColor Gray
    }
}

# 3. Component Categories 缓存（IE Band 类别 {00021493-...}）
$catId = "{00021493-0000-0000-C000-000000000046}"
$catImpl = "HKLM:\SOFTWARE\Classes\Component Categories\$catId\Impl"
if (Test-Path $catImpl) {
    try {
        Remove-ItemProperty -Path $catImpl -Name $clsid -ErrorAction Stop
        Write-Host "[OK]   已删除 Component Categories Impl 值" -ForegroundColor Green
    } catch {
        Write-Host "[SKIP] Component Categories Impl 值不存在或删除失败" -ForegroundColor Gray
    }
}

# 4. HKCU 中的 Explorer 缓存（让"查看->选项"立即刷新）
$cacheKey = "HKCU:\Software\Microsoft\Windows\CurrentVersion\Explorer\Discardable\PostSetup\Component Categories\$catId\Enum"
if (Test-Path $cacheKey) {
    try {
        Remove-Item -Path $cacheKey -Recurse -Force -ErrorAction Stop
        Write-Host "[OK]   已清除 HKCU Component Categories 缓存" -ForegroundColor Green
    } catch {
        Write-Host "[SKIP] HKCU 缓存清除失败" -ForegroundColor Gray
    }
}

Write-Host "`n=== 清理完成 ===" -ForegroundColor Cyan
Write-Host "请重启 explorer.exe 使更改生效：" -ForegroundColor Yellow
Write-Host "    stop-process -name explorer -force" -ForegroundColor Yellow
Write-Host "`n或重启资源管理器后，'查看 -> 选项' 里的 XPTab 项将消失。" -ForegroundColor Yellow
