$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"
$ie64 = "$env:ProgramFiles\Internet Explorer\iexplore.exe"

Write-Host "=== Enable XPTab via 64-bit IE ==="
Write-Host "CLSID: $clsid"
Write-Host "IE64 path: $ie64"
Write-Host ""

if (-not (Test-Path $ie64)) {
    Write-Host "64-bit IE not found!"
    exit 1
}

# Step 1: Launch 64-bit IE
Write-Host "[1/5] Launching 64-bit IE..."
Start-Process $ie64 "about:blank"
Write-Host "  IE launched, waiting 8 seconds for it to register with Shell..."
Start-Sleep -Seconds 8

# Step 2: Find 64-bit IE via Shell.Windows()
Write-Host ""
Write-Host "[2/5] Finding 64-bit IE window..."
$shell = New-Object -ComObject Shell.Application
$windows = $shell.Windows()
Write-Host "  Found $($windows.Count) shell window(s)"

$ieWindow = $null
for ($i = 0; $i -lt $windows.Count; $i++) {
    $win = $null
    try { $win = $windows.Item($i) } catch { continue }
    if ($win -eq $null) { continue }
    $name = ""
    try { $name = $win.FullName } catch {}
    Write-Host "  Window $i : $name"
    if ($name -match "Program Files\\Internet Explorer") {
        $ieWindow = $win
        Write-Host "  -> This is 64-bit IE!"
    }
}

if ($ieWindow -eq $null) {
    Write-Host "  64-bit IE window not found!"
    Write-Host "  Trying all windows with ShowBrowserBar..."
    for ($i = 0; $i -lt $windows.Count; $i++) {
        $win = $null
        try { $win = $windows.Item($i) } catch { continue }
        if ($win -eq $null) { continue }
        try {
            $win.ShowBrowserBar($clsid, $true, $null)
            Write-Host "  Called ShowBrowserBar on window $i"
        } catch {
            Write-Host "  ShowBrowserBar failed on window ${i}: $($_.Exception.Message)"
        }
    }
} else {
    # Step 3: Call ShowBrowserBar on 64-bit IE
    Write-Host ""
    Write-Host "[3/5] Calling ShowBrowserBar on 64-bit IE..."
    try {
        $ieWindow.ShowBrowserBar($clsid, $true, $null)
        Write-Host "  ShowBrowserBar called successfully!"
    } catch {
        Write-Host "  ShowBrowserBar failed: $($_.Exception.Message)"
    }
}

# Step 4: Wait and check
Write-Host ""
Write-Host "[4/5] Waiting 5 seconds, then checking..."
Start-Sleep -Seconds 5

# Check if XPTab.dll loaded in 64-bit IE
$ieProcs = Get-Process iexplore -ErrorAction SilentlyContinue
if ($ieProcs) {
    foreach ($p in $ieProcs) {
        $loaded = $false
        try {
            foreach ($mod in $p.Modules) {
                if ($mod.ModuleName -match "XPTab") {
                    Write-Host "  XPTab.dll IS LOADED in IE PID $($p.Id)!"
                    $loaded = $true
                }
            }
        } catch {}
        if (-not $loaded) {
            # Check if it's 64-bit
            $is64 = $false
            try { $is64 = $p.Modules[0].FileName -match "Program Files\\Internet" } catch {}
            Write-Host "  PID $($p.Id): XPTab not loaded (64-bit: $is64)"
        }
    }
}

# Check Streams\Settings
$streams = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Streams\Settings"
if (Test-Path $streams) {
    Write-Host "  Streams\Settings: EXISTS!"
} else {
    Write-Host "  Streams\Settings: NOT FOUND"
}

# Step 5: Close IE, restart Explorer
Write-Host ""
Write-Host "[5/5] Closing IE and restarting Explorer..."
Stop-Process -Name iexplore -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Start-Process explorer.exe
Write-Host "Done."
