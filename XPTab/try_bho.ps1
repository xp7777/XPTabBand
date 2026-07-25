$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"
$logPath = "$env:LOCALAPPDATA\XPTab\band_log.txt"

Write-Host "=== Attempt: PreApproved + BHO registration ==="

# Delete old log
if (Test-Path $logPath) { Remove-Item $logPath -Force }

# 1. Add to Ext\PreApproved
Write-Host "[1/3] Adding to Ext\PreApproved..."
$preApproved = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\PreApproved\$clsid"
if (-not (Test-Path $preApproved)) {
    New-Item -Path $preApproved -Force | Out-Null
    Write-Host "  Added to PreApproved"
} else {
    Write-Host "  Already in PreApproved"
}

# 2. Add to Browser Helper Objects (BHO)
Write-Host "[2/3] Adding to Browser Helper Objects..."
$bho = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Browser Helper Objects\$clsid"
if (-not (Test-Path $bho)) {
    New-Item -Path $bho -Force | Out-Null
    # Set default value to band name
    $bhoKey = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey(
        "SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Browser Helper Objects\$clsid", $true)
    if ($bhoKey) {
        $bhoKey.SetValue("", "XPTab", [Microsoft.Win32.RegistryValueKind]::String)
        $bhoKey.Close()
    }
    Write-Host "  Added to BHO"
} else {
    Write-Host "  Already in BHO"
}

# Also add to 32-bit BHO (just in case)
$bho32 = "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Explorer\Browser Helper Objects\$clsid"
if (-not (Test-Path $bho32)) {
    New-Item -Path $bho32 -Force | Out-Null
    Write-Host "  Added to 32-bit BHO (WOW6432Node)"
}

# 3. Restart Explorer
Write-Host "[3/3] Restarting Explorer..."
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 3
Start-Process explorer.exe
Write-Host "  Waiting 5 seconds..."
Start-Sleep -Seconds 5

# Check results
Write-Host ""
Write-Host "=== Results ==="
if (Test-Path $logPath) {
    Write-Host "LOG FILE EXISTS - Explorer loaded XPTab!"
    Get-Content $logPath
} else {
    Write-Host "LOG FILE NOT FOUND - Explorer did not load XPTab"
}

$proc = Get-Process explorer -ErrorAction SilentlyContinue | Select-Object -First 1
if ($proc) {
    $found = $false
    try {
        foreach ($mod in $proc.Modules) {
            if ($mod.ModuleName -match "XPTab" -or $mod.ModuleName -match "mscoree") {
                Write-Host "  Loaded: $($mod.ModuleName)"
                $found = $true
            }
        }
    } catch {}
    if (-not $found) { Write-Host "  XPTab/mscoree NOT loaded" }
}
