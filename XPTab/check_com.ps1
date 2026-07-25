$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"

Write-Host "=== COM Registration Details ==="
$clsidKey = "HKLM:\SOFTWARE\Classes\CLSID\$clsid"
if (Test-Path $clsidKey) {
    $default = (Get-ItemProperty $clsidKey -Name "(default)" -ErrorAction SilentlyContinue)."(default)"
    Write-Host "CLSID default: $default"

    $inproc = "$clsidKey\InprocServer32"
    if (Test-Path $inproc) {
        $inprocDefault = (Get-ItemProperty $inproc -Name "(default)" -ErrorAction SilentlyContinue)."(default)"
        $assembly = (Get-ItemProperty $inproc -Name "Assembly" -ErrorAction SilentlyContinue).Assembly
        $class = (Get-ItemProperty $inproc -Name "Class" -ErrorAction SilentlyContinue).Class
        $runtime = (Get-ItemProperty $inproc -Name "RuntimeVersion" -ErrorAction SilentlyContinue).RuntimeVersion
        $codebase = (Get-ItemProperty $inproc -Name "CodeBase" -ErrorAction SilentlyContinue).CodeBase
        Write-Host "InprocServer32: $inprocDefault"
        Write-Host "  Assembly: $assembly"
        Write-Host "  Class: $class"
        Write-Host "  RuntimeVersion: $runtime"
        Write-Host "  CodeBase: $codebase"
    }
}

Write-Host ""
Write-Host "=== Toolbar Registration ==="
$toolbar = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsid"
if (Test-Path $toolbar) {
    $tbDefault = (Get-ItemProperty $toolbar -Name "(default)" -ErrorAction SilentlyContinue)."(default)"
    Write-Host "Toolbar default: $tbDefault"
} else {
    Write-Host "Toolbar NOT FOUND"
}

Write-Host ""
Write-Host "=== Implemented Categories ==="
$catId = "{00021493-0000-0000-C000-000000000046}"
$implCat = "$clsidKey\Implemented Categories\$catId"
if (Test-Path $implCat) {
    Write-Host "CommBand category: EXISTS"
} else {
    Write-Host "CommBand category: NOT FOUND"
}

Write-Host ""
Write-Host "=== Approved ==="
$approved = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"
$apprVal = (Get-ItemProperty $approved -Name $clsid -ErrorAction SilentlyContinue).$clsid
Write-Host "Approved value: $apprVal"

Write-Host ""
Write-Host "=== WOW6432Node check (64-bit vs 32-bit) ==="
$wowClsid = "HKLM:\SOFTWARE\WOW6432Node\Classes\CLSID\$clsid"
if (Test-Path $wowClsid) {
    Write-Host "WOW6432Node CLSID: EXISTS (32-bit registration also present)"
} else {
    Write-Host "WOW6432Node CLSID: NOT FOUND (64-bit only - correct)"
}

Write-Host ""
Write-Host "=== Enable Browser Extensions ==="
$advKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced"
$extEnabled = (Get-ItemProperty $advKey -Name "EnableBrowserExt" -ErrorAction SilentlyContinue).EnableBrowserExt
Write-Host "EnableBrowserExt (HKCU): $extEnabled"
$advKeyLM = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced"
$extEnabledLM = (Get-ItemProperty $advKeyLM -Name "EnableBrowserExt" -ErrorAction SilentlyContinue).EnableBrowserExt
Write-Host "EnableBrowserExt (HKLM): $extEnabledLM"

Write-Host ""
Write-Host "=== IE Advanced EnableThirdPartyBrowserExtensions ==="
$ieAdv = "HKCU:\SOFTWARE\Microsoft\Internet Explorer\AdvancedOptions\BROWSING\THIRDPARTYEXT"
if (Test-Path $ieAdv) {
    $ieExt = (Get-ItemProperty $ieAdv -Name "CheckedValue" -ErrorAction SilentlyContinue).CheckedValue
    Write-Host "IE ThirdPartyExt CheckedValue: $ieExt"
} else {
    Write-Host "IE AdvancedOptions key not found"
}
# Also check the actual setting in IE main registry
$ieMain = "HKCU:\SOFTWARE\Microsoft\Internet Explorer\Main"
$enableExt = (Get-ItemProperty $ieMain -Name "Enable Browser Extensions" -ErrorAction SilentlyContinue)."Enable Browser Extensions"
Write-Host "IE Main 'Enable Browser Extensions': $enableExt"
