Write-Host "=== Explorer Band related registry ==="

$classicShell = Get-ItemProperty "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced" -Name ClassicShell -ErrorAction SilentlyContinue
Write-Host "ClassicShell: $(if($classicShell){$classicShell.ClassicShell}else{'not set'})"

$extStats = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Stats\{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"
if (Test-Path $extStats) {
    Write-Host "Ext Stats: EXISTS"
} else {
    Write-Host "Ext Stats: NOT FOUND (Band not in IE add-ons)"
}

$preApproved = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\PreApproved"
if (Test-Path $preApproved) {
    Write-Host "PreApproved subkeys:"
    Get-ChildItem $preApproved | ForEach-Object { Write-Host "  $($_.PSChildName)" }
}

$bandState = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Bands\{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"
if (Test-Path $bandState) {
    Write-Host "Bands CLSID: EXISTS"
} else {
    Write-Host "Bands CLSID: NOT FOUND"
}

$streams = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Streams\Settings"
if (Test-Path $streams) {
    Write-Host "Streams Settings: EXISTS"
} else {
    Write-Host "Streams Settings: NOT FOUND"
}

$bandSites = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\BandRules"
if (Test-Path $bandSites) {
    Write-Host "BandRules: EXISTS"
} else {
    Write-Host "BandRules: NOT FOUND"
}
