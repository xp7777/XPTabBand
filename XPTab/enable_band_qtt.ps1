# XPTab Band Enabler - QTTabBar style
# Writes IE add-on enable registry keys, then restarts explorer

$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"
$ErrorActionPreference = "Stop"

Write-Host "=== XPTab Band Enabler (QTTabBar style) ==="
Write-Host "CLSID: $clsid"
Write-Host ""

# Step 1: Write Ext\Settings (IE add-on enabled state)
# When IE enables an add-on, it writes Flags=0 to this key
Write-Host "[1/4] Writing Ext\Settings (IE add-on enable)..."
$extSettingsPath = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings\$clsid"
if (-not (Test-Path $extSettingsPath)) {
    New-Item -Path $extSettingsPath -Force | Out-Null
}
Set-ItemProperty -Path $extSettingsPath -Name "Flags" -Value 0 -Type DWord
Write-Host "  OK: Ext\Settings\$clsid\Flags = 0"

# Step 2: Write Ext\Stats (IE add-on usage statistics - simulates IE has loaded the band)
Write-Host "[2/4] Writing Ext\Stats (IE add-on stats)..."
$extStatsPath = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Stats\$clsid\iexplore"
if (-not (Test-Path $extStatsPath)) {
    New-Item -Path $extStatsPath -Force | Out-Null
}
# Write a minimal stats block (Type=1, Count=1, Time=now)
$statsPath = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Stats\$clsid\iexplore"
$now = Get-Date
$filetime = $now.ToFileTime()
Set-ItemProperty -Path $statsPath -Name "Type" -Value 1 -Type DWord
Set-ItemProperty -Path $statsPath -Name "Count" -Value 1 -Type DWord
Set-ItemProperty -Path $statsPath -Name "Time" -Value $filetime -Type QWord
Write-Host "  OK: Ext\Stats written"

# Step 3: Write Explorer\Bands (Explorer band state)
Write-Host "[3/4] Writing Explorer\Bands..."
$bandsPath = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Bands\$clsid"
if (-not (Test-Path $bandsPath)) {
    New-Item -Path $bandsPath -Force | Out-Null
}
Write-Host "  OK: Explorer\Bands\$clsid created"

# Also ensure the Bands parent key exists
$bandsParent = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Bands"
if (-not (Test-Path $bandsParent)) {
    New-Item -Path $bandsParent -Force | Out-Null
}

# Step 4: Restart explorer
Write-Host "[4/4] Restarting explorer..."
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Start-Process explorer.exe
Write-Host "  Explorer restarted"

Write-Host ""
Write-Host "=== Done ==="
Write-Host "If band still not showing, try:"
Write-Host "  1. Open IE (iexplore.exe)"
Write-Host "  2. Gear icon -> Manage add-ons"
Write-Host "  3. Find XPTab in Toolbars and Extensions"
Write-Host "  4. Select XPTab -> Enable"
Write-Host "  5. Close IE, restart explorer"
Write-Host ""
Write-Host "Also check: Explorer -> View tab -> Options button"
Write-Host "  Look for XPTab in Folder Options dialog"
