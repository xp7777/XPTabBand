$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"

Write-Host "=== Check IE\Toolbar value type ==="
$tbKey = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsid"
if (Test-Path $tbKey) {
    $key = Get-Item $tbKey
    $val = $key.GetValue("")
    $valKind = $key.GetValueKind("")
    Write-Host "Value name: (default)"
    Write-Host "Value: '$val'"
    Write-Host "Value kind: $valKind (should be String/ExpandString)"
    if ($valKind -ne "String" -and $valKind -ne "ExpandString") {
        Write-Host "[WARN] Value kind is NOT String! This may cause Explorer to ignore the Band."
        Write-Host "       Fixing..."
        Set-ItemProperty -Path $tbKey -Name "(default)" -Value "XPTab" -Type String
        Write-Host "       Fixed to String type."
    }
}

Write-Host ""
Write-Host "=== Check all registered bands in IE\Toolbar ==="
$allBands = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar"
$tbParent = Get-Item $allBands
Write-Host "All subkeys under IE\Toolbar:"
foreach ($name in $tbParent.GetSubKeyNames()) {
    $subKey = Get-Item "$allBands\$name"
    $subVal = $subKey.GetValue("")
    $subKind = $subKey.GetValueKind("")
    Write-Host "  $name = '$subVal' (kind: $subKind)"
}

Write-Host ""
Write-Host "=== Check for conflicting LockToolbars / NoBandCustomize ==="
$explorerPolCU = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer"
$explorerPolLM = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer"
$advCU = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced"

foreach ($k in @($explorerPolCU, $explorerPolLM)) {
    if (Test-Path $k) {
        $p = Get-ItemProperty $k
        Write-Host "Policy $k :"
        Write-Host "  NoBandCustomize      = $($p.NoBandCustomize)"
        Write-Host "  NoToolbarsOnTaskbar  = $($p.NoToolbarsOnTaskbar)"
        Write-Host "  NoSetTaskbar          = $($p.NoSetTaskbar)"
    }
}

if (Test-Path $advCU) {
    $a = Get-ItemProperty $advCU
    Write-Host "Explorer\Advanced :"
    Write-Host "  TaskbarSi            = $($a.TaskbarSi)"
    Write-Host "  TaskbarGlomLevel     = $($a.TaskbarGlomLevel)"
}

Write-Host ""
Write-Host "=== Clean possibly corrupt Explorer\Streams\Settings ==="
$streams = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Streams\Settings"
if (Test-Path $streams) {
    $s = Get-ItemProperty $streams
    Write-Host "Streams\Settings exists. Properties:"
    $s.PSObject.Properties | Where-Object { $_.Name -notlike "PS*" } | ForEach-Object {
        Write-Host "  $($_.Name) = $($_.Value) (kind: $($_.Value.GetType().Name))"
    }
} else {
    Write-Host "Streams\Settings NOT FOUND (no saved band layout)"
}

Write-Host ""
Write-Host "=== Re-register and restart Explorer ==="
Write-Host "Current explorer.exe PID: $((Get-Process explorer -ErrorAction SilentlyContinue | Select-Object -First 1).Id)"
