$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"

Write-Host "=== Enable XPTab via IE COM Object ==="
Write-Host ""

# Check if IE redirects to Edge
Write-Host "[1/6] Checking IE redirect policy..."
$ieFeat = "HKCU:\SOFTWARE\Microsoft\Internet Explorer\Main\EdgeIntegration"
$edgeRedirect = $null
try {
    $edgeRedirect = (Get-ItemProperty "HKCU:\SOFTWARE\Microsoft\Internet Explorer\Main" -Name "NotifySectionLevelUrlEnabled" -ErrorAction SilentlyContinue)
} catch {}
$iePolicy = "HKLM:\SOFTWARE\Policies\Microsoft\Internet Explorer\Main"
$schemeSvc = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\BrowserHelperObjects"
Write-Host "  Checking IE availability..."

# Try creating IE COM object directly
Write-Host ""
Write-Host "[2/6] Creating IE COM object..."
try {
    $ie = New-Object -ComObject InternetExplorer.Application
    Write-Host "  IE COM object created successfully"
    Write-Host "  IE FullName: $($ie.FullName)"

    # Make IE visible and navigate to about:blank
    Write-Host ""
    Write-Host "[3/6] Making IE visible and navigating..."
    $ie.Visible = $true
    $ie.Navigate2("about:blank")
    Start-Sleep -Seconds 3
    Write-Host "  IE is now visible"

    # Call ShowBrowserBar
    Write-Host ""
    Write-Host "[4/6] Calling ShowBrowserBar..."
    try {
        $ie.ShowBrowserBar($clsid, $true, $null)
        Write-Host "  ShowBrowserBar called successfully (show=true)"
    } catch {
        Write-Host "  ShowBrowserBar failed: $($_.Exception.Message)"
    }

    # Wait and check
    Start-Sleep -Seconds 3

    # Check registry
    Write-Host ""
    Write-Host "[5/6] Checking registry..."
    $streams = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Streams\Settings"
    if (Test-Path $streams) {
        Write-Host "  Streams\Settings: EXISTS"
        $val = (Get-ItemProperty $streams -Name "(default)" -ErrorAction SilentlyContinue)."(default)"
        Write-Host "  Value length: $($val.Length) bytes"
    } else {
        Write-Host "  Streams\Settings: NOT FOUND"
    }

    $streamsDef = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Streams\Defaults"
    if (Test-Path $streamsDef) {
        Write-Host "  Streams\Defaults: EXISTS"
    } else {
        Write-Host "  Streams\Defaults: NOT FOUND"
    }

    # Check if XPTab.dll was loaded in IE
    Write-Host ""
    Write-Host "[6/6] Checking if XPTab.dll loaded in IE process..."
    $ieProcs = Get-Process iexplore -ErrorAction SilentlyContinue
    if ($ieProcs) {
        foreach ($p in $ieProcs) {
            $loaded = $false
            try {
                foreach ($mod in $p.Modules) {
                    if ($mod.ModuleName -match "XPTab") {
                        Write-Host "  XPTab.dll IS LOADED in IE (PID: $($p.Id))"
                        $loaded = $true
                    }
                }
            } catch {}
            if (-not $loaded) {
                Write-Host "  XPTab.dll NOT loaded in IE (PID: $($p.Id))"
            }
        }
    } else {
        Write-Host "  No iexplore.exe process found (IE may have redirected to Edge)"
        $edgeProcs = Get-Process msedge -ErrorAction SilentlyContinue
        if ($edgeProcs) {
            Write-Host "  Edge is running instead! IE redirected to Edge."
        }
    }

    # Close IE
    Write-Host ""
    Write-Host "Closing IE..."
    try { $ie.Quit() } catch {}
    Start-Sleep -Seconds 2
    Stop-Process -Name iexplore -Force -ErrorAction SilentlyContinue

} catch {
    Write-Host "  ERROR creating IE COM object: $($_.Exception.Message)"
    Write-Host "  IE may be disabled or redirected to Edge"
}

Write-Host ""
Write-Host "Restarting explorer..."
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Start-Process explorer.exe
Write-Host "Done."
