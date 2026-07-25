$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"

Write-Host "=== Test 1: COM instantiation (64-bit PowerShell) ==="
Write-Host "PowerShell process: $($env:PROCESSOR_ARCHITECTURE)"
try {
    $type = [Type]::GetTypeFromCLSID([Guid]$clsid)
    if ($type -eq $null) {
        Write-Host "[FAIL] GetTypeFromCLSID returned null - COM not registered"
    } else {
        Write-Host "[OK] Type found: $($type.FullName) from $($type.Assembly.GetName().Name)"
        $obj = [Activator]::CreateInstance($type)
        Write-Host "[OK] Instance created: $($obj.GetType().FullName)"

        # Check if log file was created (static constructor should have run)
        $logPath = "$env:LOCALAPPDATA\XPTab\band_log.txt"
        if (Test-Path $logPath) {
            Write-Host "[OK] Log file created at $logPath"
            Write-Host "     Content:"
            Get-Content $logPath | ForEach-Object { Write-Host "       $_" }
        } else {
            Write-Host "[WARN] Log file NOT created at $logPath"
        }
    }
} catch {
    Write-Host "[FAIL] COM instantiation failed: $($_.Exception.Message)"
    Write-Host "  InnerException: $($_.Exception.InnerException)"
}

Write-Host ""
Write-Host "=== Test 2: Check if Explorer bands list is populated ==="
$bandsKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Bands"
if (Test-Path $bandsKey) {
    $bands = Get-ChildItem $bandsKey
    Write-Host "[INFO] Explorer\Bands has $($bands.Count) entries:"
    foreach ($b in $bands) { Write-Host "  - $($b.PSChildName)" }
} else {
    Write-Host "[INFO] Explorer\Bands key not found (no bands enabled for current user)"
}

Write-Host ""
Write-Host "=== Test 3: Check IE Ext\Settings (add-on enable state) ==="
$extKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings\$clsid"
if (Test-Path $extKey) {
    $ext = Get-ItemProperty $extKey
    Write-Host "[INFO] Ext\Settings\$clsid exists"
    Write-Host "  Flags = $($ext.Flags)"
} else {
    Write-Host "[INFO] Ext\Settings\$clsid not found (add-on never enabled)"
}

Write-Host ""
Write-Host "=== Test 4: Check EnableBrowserExt policy ==="
$ieMain = "HKCU:\SOFTWARE\Microsoft\Internet Explorer\Main"
if (Test-Path $ieMain) {
    $main = Get-ItemProperty $ieMain
    $ebe = $main.'Enable Browser Extensions'
    Write-Host "[INFO] Enable Browser Extensions = '$ebe' (should be 'yes')"
}

Write-Host ""
Write-Host "=== Done ==="
