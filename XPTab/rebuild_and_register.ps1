$ErrorActionPreference = "Continue"
$dllPath = "G:\Test\testFileExplorerPro\build\XPTab.dll"
$regasm = "C:\Windows\Microsoft.NET\Framework64\v4.0.30319\regasm.exe"
$logFile = "G:\Test\testFileExplorerPro\XPTab\admin_run.log"
$bandLog = Join-Path $env:LOCALAPPDATA "XPTab\band_log.txt"

# Self-elevate
$myId = [System.Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object System.Security.Principal.WindowsPrincipal($myId)
$isAdmin = $principal.IsInRole([System.Security.Principal.WindowsBuiltInRole]::Administrator)

if (-not $isAdmin) {
    Write-Host "Not running as admin. Re-launching elevated..."
    $scriptPath = $MyInvocation.MyCommand.Path
    Start-Process powershell -Verb RunAs -ArgumentList "-ExecutionPolicy Bypass -NoProfile -File `"$scriptPath`"" -Wait
    exit 0
}

Start-Transcript -Path $logFile -Force | Out-Null

Write-Host "=== Running as Administrator ==="
Write-Host ""

# Clear old band log to see fresh results
if (Test-Path $bandLog) {
    Write-Host "Clearing old band log: $bandLog"
    Remove-Item $bandLog -Force
}

Write-Host "=== Step 0: Stop Explorer (release DLL lock) ==="
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 2

Write-Host "=== Step 1: Build XPTab.dll ==="
& dotnet build "G:\Test\testFileExplorerPro\XPTab\XPTab.csproj" -c Release 2>&1 | ForEach-Object { Write-Host "  $_" }
if ($LASTEXITCODE -ne 0) {
    Write-Host "[FAIL] Build failed"
    Stop-Transcript | Out-Null
    exit 1
}

Write-Host ""
Write-Host "=== Step 2: Re-register XPTab.dll ==="
& $regasm /unregister $dllPath 2>&1 | ForEach-Object { Write-Host "  $_" }
& $regasm /codebase /tlb $dllPath 2>&1 | ForEach-Object { Write-Host "  $_" }

# Re-add entries that regasm doesn't handle
$clsidBand = "{A1B2C3D4-E5F6-4789-ABCD-0123456789AB}"
$tbKey = "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsidBand"
New-Item -Path $tbKey -Force | Out-Null
Set-ItemProperty -Path $tbKey -Name "(default)" -Value "XPTab" -Type String

$approvedKey = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved"
if (-not (Test-Path $approvedKey)) { New-Item -Path $approvedKey -Force | Out-Null }
Set-ItemProperty -Path $approvedKey -Name $clsidBand -Value "XPTab" -Type String

$catId = "{00021493-0000-0000-C000-000000000046}"
$catKey = "HKLM:\SOFTWARE\Classes\CLSID\$clsidBand\Implemented Categories\$catId"
New-Item -Path $catKey -Force | Out-Null

Write-Host ""
Write-Host "=== Step 3: Restart Explorer ==="
Stop-Process -Name explorer -Force -ErrorAction SilentlyContinue
Start-Sleep -Seconds 3
Start-Process explorer
Start-Sleep -Seconds 3

Write-Host ""
Write-Host "=== Done ==="
Write-Host "Band log cleared - new log will appear if Explorer loads the Band."

Stop-Transcript | Out-Null
