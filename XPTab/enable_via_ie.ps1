# Enable XPTab band via IE ShowBrowserBar API
# This is how QTTabBar gets enabled - through IE's add-on system

$clsid = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"

Write-Host "=== Enable XPTab via IE ShowBrowserBar ==="
Write-Host "CLSID: $clsid"
Write-Host ""

# Step 1: Launch IE
Write-Host "[1/5] Launching IE..."
$ieProcess = Start-Process "iexplore.exe" -PassThru
Write-Host "  IE started (PID: $($ieProcess.Id))"

# Step 2: Wait for IE window to be ready
Write-Host "[2/5] Waiting for IE window..."
Start-Sleep -Seconds 5

# Step 3: Find IE window via Shell.Application
Write-Host "[3/5] Finding IE window..."
try {
    $shell = New-Object -ComObject Shell.Application
    $windows = $shell.Windows()
    Write-Host "  Found $($windows.Count) shell window(s)"

    $ieFound = $false
    for ($i = 0; $i -lt $windows.Count; $i++) {
        $win = $null
        try { $win = $windows.Item($i) } catch { continue }
        if ($win -eq $null) { continue }

        $name = ""
        try { $name = $win.FullName } catch {}
        Write-Host "  Window $i : $name"

        if ($name -match "iexplore") {
            $ieFound = $true
            Write-Host "  -> This is an IE window"
            try {
                # Call ShowBrowserBar to show the band
                $win.ShowBrowserBar($clsid, $true, $null)
                Write-Host "  -> ShowBrowserBar(show=true) called SUCCESS"
            } catch {
                Write-Host "  -> ShowBrowserBar failed: $($_.Exception.Message)"
            }
        }

        if ($name -match "explorer") {
            Write-Host "  -> This is an Explorer window"
            try {
                $win.ShowBrowserBar($clsid, $true, $null)
                Write-Host "  -> ShowBrowserBar(show=true) called on Explorer"
            } catch {
                Write-Host "  -> ShowBrowserBar on Explorer failed: $($_.Exception.Message)"
            }
        }
    }

    if (-not $ieFound) {
        Write-Host "  IE window not found in shell windows"
    }
} catch {
    Write-Host "  ERROR: $($_.Exception.Message)"
}

# Step 4: Wait a moment, then close IE
Write-Host ""
Write-Host "[4/5] Waiting 3 seconds then closing IE..."
Start-Sleep -Seconds 3
try { Stop-Process -Name iexplore -Force -ErrorAction SilentlyContinue } catch {}

# Step 5: Check registry and restart explorer
Write-Host "[5/5] Checking Streams\Settings registry..."
$streams = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Streams\Settings"
if (Test-Path $streams) {
    $val = Get-ItemProperty $streams -Name "(default)" -ErrorAction SilentlyContinue
    Write-Host "  Streams\Settings EXISTS"
} else {
    Write-Host "  Streams\Settings NOT FOUND"
}

$streamsDef = "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Streams\Defaults"
if (Test-Path $streamsDef) {
    Write-Host "  Streams\Defaults EXISTS"
} else {
    Write-Host "  Streams\Defaults NOT FOUND"
}

Write-Host ""
Write-Host "Restarting explorer..."
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2
Start-Process explorer.exe
Write-Host "Done."
