$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"

Write-Host "=== Search all XPTab-related CLSIDs in registry ==="
Write-Host ""

# 1. Search HKLM Classes\CLSID for any key whose default value mentions XPTab
Write-Host "[1] Searching HKLM\SOFTWARE\Classes\CLSID for XPTab..."
$clsidRoot = "HKLM:\SOFTWARE\Classes\CLSID"
Get-ChildItem $clsidRoot -ErrorAction SilentlyContinue | ForEach-Object {
    $key = $_
    try {
        $val = (Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue).'(default)'
        if ($val -like "*XPTab*") {
            $sub = $_.PSChildName
            $ipsrv = $null
            $ipsrvPath = "$($_.PSPath)\InprocServer32"
            if (Test-Path $ipsrvPath) {
                $ipsrv = (Get-ItemProperty $ipsrvPath -ErrorAction SilentlyContinue).'(default)'
            }
            Write-Host "  CLSID: $sub"
            Write-Host "    Name: $val"
            Write-Host "    InprocServer32: $ipsrv"
        }
    } catch {}
}

Write-Host ""

# 2. Search WOW6432Node (32-bit view) for XPTab
Write-Host "[2] Searching HKLM\SOFTWARE\WOW6432Node\Classes\CLSID for XPTab..."
$wowRoot = "HKLM:\SOFTWARE\WOW6432Node\Classes\CLSID"
if (Test-Path $wowRoot) {
    Get-ChildItem $wowRoot -ErrorAction SilentlyContinue | ForEach-Object {
        try {
            $val = (Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue).'(default)'
            if ($val -like "*XPTab*") {
                Write-Host "  CLSID (32-bit): $($_.PSChildName) = $val"
            }
        } catch {}
    }
} else {
    Write-Host "  WOW6432Node not found"
}

Write-Host ""

# 3. Search IE\Toolbar for all subkeys
Write-Host "[3] All entries under HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar..."
$tbRoot = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar"
Get-ChildItem $tbRoot -ErrorAction SilentlyContinue | ForEach-Object {
    $val = (Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue).'(default)'
    Write-Host "  $($_.PSChildName) = '$val'"
}

Write-Host ""

# 4. Search Ext\Settings (HKCU) for XPTab-related CLSIDs
Write-Host "[4] Searching HKCU Ext\Settings for XPTab..."
$extRoot = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings"
if (Test-Path $extRoot) {
    Get-ChildItem $extRoot -ErrorAction SilentlyContinue | ForEach-Object {
        $subKey = $_.PSChildName
        # Check if this CLSID resolves to XPTab
        $clsidPath = "HKLM:\SOFTWARE\Classes\CLSID\$subKey"
        if (Test-Path $clsidPath) {
            $name = (Get-ItemProperty $clsidPath -ErrorAction SilentlyContinue).'(default)'
            if ($name -like "*XPTab*") {
                $flags = (Get-ItemProperty $_.PSPath -ErrorAction SilentlyContinue).Flags
                Write-Host "  Ext\Settings\$subKey"
                Write-Host "    Name: $name"
                Write-Host "    Flags: $flags"
            }
        }
    }
} else {
    Write-Host "  Ext\Settings not found"
}

Write-Host ""

# 5. Search Ext\Stats (HKCU) for XPTab-related CLSIDs
Write-Host "[5] Searching HKCU Ext\Stats for XPTab..."
$statsRoot = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Stats"
if (Test-Path $statsRoot) {
    Get-ChildItem $statsRoot -ErrorAction SilentlyContinue | ForEach-Object {
        $subKey = $_.PSChildName
        $clsidPath = "HKLM:\SOFTWARE\Classes\CLSID\$subKey"
        if (Test-Path $clsidPath) {
            $name = (Get-ItemProperty $clsidPath -ErrorAction SilentlyContinue).'(default)'
            if ($name -like "*XPTab*") {
                Write-Host "  Ext\Stats\$subKey"
                Write-Host "    Name: $name"
                Get-ChildItem $_.PSPath -ErrorAction SilentlyContinue | ForEach-Object {
                    Write-Host "      Host: $($_.PSChildName)"
                }
            }
        }
    }
}

Write-Host ""
Write-Host "=== Done ==="
