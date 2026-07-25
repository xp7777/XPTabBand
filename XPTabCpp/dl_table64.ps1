$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$urls = @(
    'https://cdn.jsdelivr.net/gh/TsudaKageyu/minhook@master/src/HDE/table64.h',
    'https://gitee.com/mirrors/minhook/raw/master/src/HDE/table64.h',
    'https://gitee.com/qq_32342521/minhook/raw/master/src/HDE/table64.h'
)
$out = 'g:\Test\testFileExplorerPro\XPTabCpp\MinHook\table64.h'
$ok = $false
foreach ($u in $urls) {
    try {
        Write-Host "Trying: $u"
        Invoke-WebRequest -Uri $u -OutFile $out -UseBasicParsing -TimeoutSec 25
        $size = (Get-Item $out).Length
        Write-Host "SUCCESS downloaded $size bytes from $u"
        $ok = $true
        break
    } catch {
        Write-Host "FAIL ${u}: $($_.Exception.Message)"
    }
}
if (-not $ok) { Write-Host 'ALL_FAILED'; exit 1 }
