$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"
$CommBandCATID = "{00021493-0000-0000-C000-000000000046}"

Write-Host "=== XPTab Registration Check (for View > Options menu) ==="
Write-Host ""

# 1. COM server (CLSID\InprocServer32)
$clsidKey = "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32"
if (Test-Path $clsidKey) {
    $ipsrv = Get-ItemProperty $clsidKey
    Write-Host "[OK] CLSID\InprocServer32 exists"
    Write-Host "     (default) = $($ipsrv.'(default)')"
    Write-Host "     Assembly  = $($ipsrv.Assembly)"
    Write-Host "     Class     = $($ipsrv.Class)"
    Write-Host "     CodeBase  = $($ipsrv.CodeBase)"
} else {
    Write-Host "[FAIL] CLSID\InprocServer32 NOT FOUND"
}

Write-Host ""

# 2. Toolbar registration (THIS is what makes Band appear in View > Options menu)
$toolbarKey = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsid"
if (Test-Path $toolbarKey) {
    $tb = Get-ItemProperty $toolbarKey
    Write-Host "[OK] Internet Explorer\Toolbar\$clsid exists"
    Write-Host "     (default) = '$($tb.'(default)')'"
} else {
    Write-Host "[FAIL] Internet Explorer\Toolbar\$clsid NOT FOUND"
    Write-Host "       >>> This is REQUIRED for Band to appear in View > Options menu <<<"
}

Write-Host ""

# 3. Implemented Categories - CommBand
$catKey = "HKLM:\SOFTWARE\Classes\CLSID\$clsid\Implemented Categories\$CommBandCATID"
if (Test-Path $catKey) {
    Write-Host "[OK] Implemented Categories\CommBand exists"
} else {
    Write-Host "[FAIL] Implemented Categories\CommBand NOT FOUND"
    Write-Host "       >>> This is REQUIRED for Band to appear in View > Options menu <<<"
}

Write-Host ""

# 4. Shell Extensions Approved
$approvedKey = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"
if (Test-Path $approvedKey) {
    $approved = Get-ItemProperty $approvedKey
    $val = $approved.$clsid
    if ($val -ne $null) {
        Write-Host "[OK] Shell Extensions\Approved\$clsid = '$val'"
    } else {
        Write-Host "[WARN] Shell Extensions\Approved exists but $clsid value missing"
    }
} else {
    Write-Host "[WARN] Shell Extensions\Approved key not found"
}

Write-Host ""

# 5. Check NoBandCustomize policy (would hide Band UI)
$polKey = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer"
if (Test-Path $polKey) {
    $pol = Get-ItemProperty $polKey
    if ($pol.NoBandCustomize -ne $null) {
        Write-Host "[FAIL] NoBandCustomize = $($pol.NoBandCustomize) (this HIDES View>Options band menu!)"
    } else {
        Write-Host "[OK] NoBandCustomize not set"
    }
    if ($pol.NoToolbarsOnTaskbar -ne $null) {
        Write-Host "[WARN] NoToolbarsOnTaskbar = $($pol.NoToolbarsOnTaskbar)"
    }
} else {
    Write-Host "[OK] No Explorer policies set"
}

# Also check HKLM policy
$polKeyLM = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer"
if (Test-Path $polKeyLM) {
    $polLM = Get-ItemProperty $polKeyLM
    if ($polLM.NoBandCustomize -ne $null) {
        Write-Host "[FAIL] HKLM NoBandCustomize = $($polLM.NoBandCustomize) (this HIDES View>Options band menu!)"
    }
}

Write-Host ""
Write-Host "=== Summary ==="
Write-Host "For XPTab to appear in 'View > Options' dropdown menu, you need:"
Write-Host "  1. COM registration (CLSID\InprocServer32)        - checked above"
Write-Host "  2. Toolbar registry key (IE\Toolbar\$clsid)       - checked above (CRITICAL)"
Write-Host "  3. Implemented Categories\CommBand                - checked above (CRITICAL)"
Write-Host "  4. No NoBandCustomize policy                      - checked above"
Write-Host ""
Write-Host "Enable path: Explorer > View (Ribbon) > Options button dropdown > check 'XPTab'"
