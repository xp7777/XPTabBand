$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"
$dll = "g:\Test\testFileExplorerPro\build\XPTab.dll"
$regasm = "$env:WINDIR\Microsoft.NET\Framework64\v4.0.30319\regasm.exe"
$logPath = "$env:LOCALAPPDATA\XPTab\band_log.txt"

Write-Host "=== Rebuild and Test ==="

# Delete old log
if (Test-Path $logPath) {
    Remove-Item $logPath -Force
    Write-Host "Old log deleted"
}

# Re-register COM (codebase update)
Write-Host "Re-registering COM..."
& $regasm $dll /codebase 2>&1 | Out-Null
Write-Host "COM registered"

# Restart explorer
Write-Host "Restarting explorer..."
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 3
Start-Process explorer.exe
Write-Host "Explorer restarted, waiting 5 seconds..."
Start-Sleep -Seconds 5

# Check log
Write-Host ""
Write-Host "=== Log Check ==="
if (Test-Path $logPath) {
    Write-Host "LOG FILE EXISTS - Explorer tried to load the band!"
    Get-Content $logPath
} else {
    Write-Host "LOG FILE NOT FOUND - Explorer did NOT try to load the band"
    Write-Host "This means Explorer doesn't know it should load XPTab"
}

# Check if DLL loaded in explorer
Write-Host ""
Write-Host "=== Module Check ==="
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
    if (-not $found) {
        Write-Host "  XPTab.dll and mscoree.dll NOT loaded in explorer"
    }
}
