$clsid = "{a1b2c3d4-e5f6-4789-abcd-0123456789ab}"

# 检查 Band 是否被策略禁用
Write-Host "=== 检查 Band 禁用状态 ===" -ForegroundColor Cyan

# 1. 检查组策略禁用列表
$policyPaths = @(
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer\DisallowRun",
    "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer\DisallowRun",
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Group Policy\State\Machine\ExtensionList"
)
foreach ($p in $policyPaths) {
    if (Test-Path $p) {
        $k = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey($p.Replace("HKLM:\",""))
        if ($k) {
            foreach ($vn in $k.GetValueNames()) {
                $vv = $k.GetValue($vn)
                if ($vv -like "*$clsid*" -or $vv -like "*XPTab*") {
                    Write-Host "  [!] 在 $p 发现禁用项: $vn = $vv" -ForegroundColor Red
                }
            }
            $k.Close()
        }
    }
}

# 2. 检查 Bands 注册表（Explorer 记住哪些 Band 已显示）
Write-Host "`n=== Explorer 已注册的 Bands ===" -ForegroundColor Cyan
$bandsKey = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey("SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Discardable\PostSetup\Component Categories\{00021493-0000-0000-C000-000000000046}\Enum")
if ($bandsKey) {
    Write-Host "  (当前用户已枚举的 IE Bands)"
    foreach ($vn in $bandsKey.GetValueNames()) {
        Write-Host "    $vn = $($bandsKey.GetValue($vn))"
    }
    $bandsKey.Close()
} else {
    Write-Host "  Component Categories Enum 键不存在"
}

# 3. 检查 IE Band 类别注册
Write-Host "`n=== IE Band 组件类别 ===" -ForegroundColor Cyan
$catKey = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SOFTWARE\Classes\Component Categories\{00021493-0000-0000-C000-000000000046}\Impl")
if ($catKey) {
    if ($catKey.GetValueNames() -contains $clsid) {
        Write-Host "  [OK] XPTab 已注册为 IE Band 组件类别"
    } else {
        Write-Host "  [!] XPTab 未注册为 IE Band 组件类别（这可能是原因）" -ForegroundColor Yellow
        Write-Host "  已注册的 Band:"
        foreach ($vn in $catKey.GetValueNames()) {
            Write-Host "    $vn"
        }
    }
    $catKey.Close()
} else {
    Write-Host "  [!] Component Categories\Impl 键不存在（需要注册 IE Band 类别）" -ForegroundColor Yellow
}

# 4. 检查 DeskBand 类别（Explorer Bands 用的是不同 GUID）
Write-Host "`n=== DeskBand 组件类别 ===" -ForegroundColor Cyan
# {00021492-0000-0000-C000-000000000046} = InfoBand (vertical)
# {00021493-0000-0000-C000-000000000046} = CommBand (horizontal, 工具栏)
$deskCatKey = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SOFTWARE\Classes\Component Categories\{00021493-0000-0000-C000-000000000046}")
if ($deskCatKey) {
    Write-Host "  CommBand 类别键存在"
    $deskCatKey.Close()
}
