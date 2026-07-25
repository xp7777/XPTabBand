$logPath = Join-Path $env:LOCALAPPDATA "XPTab\band_log.txt"
Write-Host "Log path: $logPath"
if (Test-Path $logPath) {
    Write-Host "=== Log file EXISTS ==="
    $item = Get-Item $logPath
    Write-Host "Size: $($item.Length) bytes"
    Write-Host "Last write: $($item.LastWriteTime)"
    Write-Host ""
    Write-Host "=== Content ==="
    Get-Content $logPath | ForEach-Object { Write-Host $_ }
} else {
    Write-Host "=== LOG FILE NOT FOUND ==="
    Write-Host "Band static constructor never ran - CLR never loaded XPTab.dll"
    Write-Host ""
    Write-Host "Checking if XPTab.dll is loaded in explorer.exe..."
    $explorer = Get-Process explorer -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($explorer) {
        Write-Host "explorer.exe PID: $($explorer.Id)"
        # Cannot easily list modules of explorer without admin, but try
        try {
            $mods = $explorer.Modules | Where-Object { $_.ModuleName -like "*XPTab*" -or $_.ModuleName -like "*mscoree*" }
            if ($mods) {
                Write-Host "Loaded modules:"
                $mods | ForEach-Object { Write-Host "  $($_.ModuleName)" }
            } else {
                Write-Host "XPTab.dll / mscoree.dll NOT loaded in explorer.exe"
            }
        } catch {
            Write-Host "Cannot enumerate modules (need admin): $($_.Exception.Message)"
        }
    }
}
