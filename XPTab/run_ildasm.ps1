$ildasm = "C:\Program Files (x86)\Microsoft SDKs\Windows\v10.0A\bin\NETFX 4.8 Tools\ildasm.exe"
$dll = "G:\Test\testFileExplorerPro\build\XPTab.dll"
$out = "G:\Test\testFileExplorerPro\XPTab\meta.txt"

if (Test-Path $ildasm) {
    Write-Host "Using ildasm: $ildasm"
    & $ildasm /metadata /out:$out $dll 2>&1 | ForEach-Object { Write-Host $_ }
    if (Test-Path $out) {
        Write-Host "=== ILDASM output created: $out ==="
        Write-Host "Size: $((Get-Item $out).Length) bytes"
    } else {
        Write-Host "Failed to create output"
    }
} else {
    Write-Host "ildasm not found at $ildasm"
}
