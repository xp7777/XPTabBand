Write-Host "=== IE Check ==="
$ie = Join-Path $env:ProgramFiles "Internet Explorer\iexplore.exe"
if (Test-Path $ie) { Write-Host "IE EXISTS: $ie" } else { Write-Host "IE NOT FOUND" }

Write-Host "`n=== Explorer Bands key ==="
$bands = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Bands"
if (Test-Path $bands) { Get-ChildItem $bands | ForEach-Object { Write-Host "  $($_.PSChildName)" } } else { Write-Host "NOT FOUND" }

Write-Host "`n=== Ext Settings for XPTab ==="
$ext = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Ext\Settings\{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"
if (Test-Path $ext) { Get-ItemProperty $ext } else { Write-Host "NOT FOUND" }

Write-Host "`n=== Advanced Folder entries with Band/Tab ==="
$adv = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced\Folder"
if (Test-Path $adv) {
    Get-ChildItem $adv -Recurse | ForEach-Object {
        $t = $_.GetValue("Text")
        if ($t -and ($t -match "QT|Tab|Band|XPTab")) { Write-Host "  MATCH: $($_.PSChildName) = $t" }
    }
} else { Write-Host "Advanced\Folder NOT FOUND" }

Write-Host "`n=== Check NoBandCustomize policy ==="
$pol = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer"
if (Test-Path $pol) {
    $v = (Get-ItemProperty $pol -Name "NoBandCustomize" -ErrorAction SilentlyContinue).NoBandCustomize
    Write-Host "NoBandCustomize = $v"
}
$pol2 = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Policies\Explorer"
if (Test-Path $pol2) {
    $v2 = (Get-ItemProperty $pol2 -Name "NoBandCustomize" -ErrorAction SilentlyContinue).NoBandCustomize
    Write-Host "NoBandCustomize(HKLM) = $v2"
}
