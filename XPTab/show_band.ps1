# Diagnose why XPTab Band is not showing in Explorer
# Run in admin PowerShell: powershell -ExecutionPolicy Bypass -File show_band.ps1

$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"

Write-Host "=== Diagnose XPTab Band visibility ==="
Write-Host "CLSID: $clsid"
Write-Host ""

# 1. Check group policies that might disable bands
Write-Host "--- Group Policies ---"
$policyPaths = @(
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer",
    "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer"
)
foreach ($p in $policyPaths) {
    $k = Get-ItemProperty -Path $p -ErrorAction SilentlyContinue
    if ($k) {
        $nb = $k.NoBandCustomize
        $nt = $k.NoToolbarCustomize
        $nsc = $k.NoShellSearchButton
        Write-Host "$p NoBandCustomize=$(if($nb){$nb}else{0}) NoToolbarCustomize=$(if($nt){$nt}else{0})"
    } else {
        Write-Host "$p (not present)"
    }
}

Write-Host ""
Write-Host "--- Registry check ---"
$regKeys = @(
    "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsid",
    "HKLM:\SOFTWARE\Classes\CLSID\$clsid",
    "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32",
    "HKLM:\SOFTWARE\Classes\CLSID\$clsid\Implemented Categories\{00021493-0000-0000-C000-000000000046}",
    "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"
)
foreach ($r in $regKeys) {
    if (Test-Path $r) {
        $v = (Get-ItemProperty -Path $r -ErrorAction SilentlyContinue)."(default)"
        Write-Host "EXISTS: $r = $v"
    } else {
        Write-Host "MISSING: $r"
    }
}

Write-Host ""
Write-Host "--- .NET Framework check ---"
$netPath = "HKLM:\SOFTWARE\Microsoft\NET Framework Setup\NDP\v4\Full"
$netKey = Get-ItemProperty -Path $netPath -ErrorAction SilentlyContinue
if ($netKey) {
    Write-Host ".NET 4.x Release: $($netKey.Release) Version: $($netKey.Version)"
}

Write-Host ""
Write-Host "--- Windows version ---"
$os = Get-CimInstance Win32_OperatingSystem
Write-Host "$($os.Caption) Build $($os.BuildNumber)"

Write-Host ""
Write-Host "--- Checking if explorer.exe can load the DLL ---"
$dllPath = "G:\Test\testFileExplorerPro\build\XPTab.dll"
if (Test-Path $dllPath) {
    Write-Host "DLL exists: $dllPath"
    $sig = Get-AuthenticodeSignature $dllPath
    Write-Host "Signature status: $($sig.Status)"
} else {
    Write-Host "DLL MISSING: $dllPath"
}

Write-Host ""
Write-Host "=== Recommended next steps ==="
Write-Host "1. Try opening via Run dialog:"
Write-Host "   explorer.exe shell:::$clsid"
Write-Host ""
Write-Host "2. If still not showing, the issue is likely Win10 security"
Write-Host "   blocking unsigned .NET COM DLLs in explorer.exe."
Write-Host "   Solution: sign the DLL or add to load-from list."
