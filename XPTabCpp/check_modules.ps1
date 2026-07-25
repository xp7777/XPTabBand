$pid_exp = 9060
Write-Host "Checking explorer.exe PID $pid_exp modules..."
try {
    $proc = Get-Process -Id $pid_exp -ErrorAction Stop
    Write-Host "Process: $($proc.ProcessName) PID: $($proc.Id)"
    $found = $false
    foreach ($mod in $proc.Modules) {
        if ($mod.ModuleName -like "*XPTab*") {
            Write-Host "  FOUND: $($mod.ModuleName) -> $($mod.FileName)"
            $found = $true
        }
    }
    if (-not $found) {
        Write-Host "  XPTabHook.dll NOT loaded in this process"
    }
} catch {
    Write-Host "Process $pid_exp not found: $($_.Exception.Message)"
}

# 也检查所有 explorer.exe 进程
Write-Host ""
Write-Host "=== All explorer.exe processes ==="
Get-Process explorer -ErrorAction SilentlyContinue | ForEach-Object {
    $hasXPTab = $false
    foreach ($mod in $_.Modules) {
        if ($mod.ModuleName -like "*XPTab*") { $hasXPTab = $true; break }
    }
    Write-Host "PID $($_.Id) XPTabHook=$hasXPTab"
}
