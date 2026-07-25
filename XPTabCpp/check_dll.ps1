Write-Output "=== DLL file info ==="
Get-Item "g:\Test\testFileExplorerPro\XPTabCpp\build\XPTabHook.dll" | Select-Object Name, Length, LastWriteTime

Write-Output ""
Write-Output "=== Check if new code is in DLL ==="
$content = [System.IO.File]::ReadAllText("g:\Test\testFileExplorerPro\XPTabCpp\build\XPTabHook.dll", [System.Text.Encoding]::ASCII)
if ($content -match "SetWinEventHook") {
    Write-Output "OK: SetWinEventHook found (new code present)"
} else {
    Write-Output "WARN: SetWinEventHook NOT found"
}
