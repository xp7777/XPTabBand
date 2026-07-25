$ErrorActionPreference = "Continue"
$clsidBand = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"
$clsidTabBar = "{52E0DDB8-85E5-3A21-8515-154A3C1DE848}"
$clsidFactory = "{B2C3D4E5-F6A7-4890-BCDE-123456789ABC}"
$dllPath = "G:\Test\testFileExplorerPro\build\XPTab.dll"
$regasm = "C:\Windows\Microsoft.NET\Framework64\v4.0.30319\regasm.exe"
$logFile = "G:\Test\testFileExplorerPro\XPTab\admin_run.log"

# Self-elevate: re-launch this script as administrator
$myId = [System.Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object System.Security.Principal.WindowsPrincipal($myId)
$isAdmin = $principal.IsInRole([System.Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "Not running as admin. Re-launching elevated..."
    $scriptPath = $MyInvocation.MyCommand.Path
    Start-Process powershell -Verb RunAs -ArgumentList "-ExecutionPolicy Bypass -NoProfile -File `"$scriptPath`"" -Wait
    exit 0
}

Start-Transcript -Path $logFile -Force | Out-Null

Write-Host "=== Running as Administrator ==="
Write-Host "User: $env:USERNAME"
Write-Host ""

Write-Host "=== Step 1: Delete stale CLSID registrations ==="
foreach ($c in @($clsidTabBar, $clsidFactory)) {
    $paths = @(
        "HKLM:\SOFTWARE\Classes\CLSID\$c",
        "HKLM:\SOFTWARE\Classes\Wow6432Node\CLSID\$c",
        "HKLM:\SOFTWARE\Wow6432Node\Classes\CLSID\$c"
    )
    foreach ($p in $paths) {
        if (Test-Path $p) {
            Write-Host "  Removing $p"
            Remove-Item $p -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    $extSet = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings\$c"
    $extStat = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Stats\$c"
    foreach ($p in @($extSet, $extStat)) {
        if (Test-Path $p) {
            Write-Host "  Removing $p"
            Remove-Item $p -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
}

Write-Host ""
Write-Host "=== Step 2: Unregister old XPTab.dll ==="
& $regasm /unregister $dllPath 2>&1 | ForEach-Object { Write-Host "  $_" }

Write-Host ""
Write-Host "=== Step 3: Register XPTab.dll ==="
& $regasm /codebase /tlb $dllPath 2>&1 | ForEach-Object { Write-Host "  $_" }

Write-Host ""
Write-Host "=== Step 4: Re-add Toolbar and Approved entries ==="
$tbKey = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsidBand"
New-Item -Path $tbKey -Force | Out-Null
Set-ItemProperty -Path $tbKey -Name "(default)" -Value "XPTab" -Type String

$approvedKey = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"
if (-not (Test-Path $approvedKey)) { New-Item -Path $approvedKey -Force | Out-Null }
Set-ItemProperty -Path $approvedKey -Name $clsidBand -Value "XPTab" -Type String

$catId = "{00021493-0000-0000-C000-000000000046}"
$catKey = "HKLM:\SOFTWARE\Classes\CLSID\$clsidBand\Implemented Categories\$catId"
New-Item -Path $catKey -Force | Out-Null

$extSet = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings\$clsidBand"
New-Item -Path $extSet -Force | Out-Null
Set-ItemProperty -Path $extSet -Name "Flags" -Value 0 -Type DWord

$bandsKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Bands\$clsidBand"
New-Item -Path $bandsKey -Force | Out-Null

Write-Host ""
Write-Host "=== Step 5: Verify only XPTabBand CLSID remains ==="
$clsidRoot = "HKLM:\SOFTWARE\Classes\CLSID"
$xptabEntries = @()
Get-ChildItem $clsidRoot -ErrorAction SilentlyContinue | ForEach-Object {
    $val = (Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue).'(default)'
    if ($val -like "*XPTab*") {
        $xptabEntries += [PSCustomObject]@{ CLSID = $_.PSChildName; Name = $val }
    }
}
if ($xptabEntries.Count -eq 1) {
    Write-Host "[OK] Only 1 XPTab CLSID: $($xptabEntries[0].CLSID) = $($xptabEntries[0].Name)"
} else {
    Write-Host "[WARN] Found $($xptabEntries.Count) XPTab CLSIDs:"
    $xptabEntries | ForEach-Object { Write-Host "  $($_.CLSID) = $($_.Name)" }
}

Write-Host ""
Write-Host "=== Step 6: Restart Explorer ==="
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Start-Process explorer
Start-Sleep -Seconds 2

Write-Host ""
Write-Host "=== Done ==="

Stop-Transcript | Out-Null
