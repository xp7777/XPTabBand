$proc = Get-Process explorer -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $proc) {
    Write-Host "explorer process not found"
    exit
}

$found = $false
foreach ($mod in $proc.Modules) {
    if ($mod.ModuleName -match "XPTab") {
        Write-Host "XPTab.dll IS LOADED: $($mod.FileName)"
        $found = $true
    }
}
if (-not $found) {
    Write-Host "XPTab.dll NOT loaded in explorer"
    Write-Host ""
    Write-Host "Checking if mscoree.dll is loaded (.NET COM host)..."
    $msc = $proc.Modules | Where-Object { $_.ModuleName -eq "mscoree.dll" }
    if ($msc) { Write-Host "  mscoree.dll IS loaded" } else { Write-Host "  mscoree.dll NOT loaded" }
}
