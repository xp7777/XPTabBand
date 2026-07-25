$ErrorActionPreference = 'Stop'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# 查询 NuGet 上 minhook 包的所有版本
$idx = Invoke-RestMethod -Uri 'https://api.nuget.org/v3-flatcontainer/minhook/index.json' -UseBasicParsing -TimeoutSec 25
$ver = $idx.versions[-1]
Write-Host "NuGet minhook latest version: $ver"

# 下载 nupkg（本质是 zip）
$tmpDir = 'g:\Test\testFileExplorerPro\XPTabCpp\minhook_nuget'
if (Test-Path $tmpDir) { Remove-Item -Recurse -Force $tmpDir }
New-Item -ItemType Directory -Path $tmpDir | Out-Null

$nupkg = Join-Path $tmpDir 'minhook.nupkg'
$url = "https://api.nuget.org/v3-flatcontainer/minhook/$ver/minhook.$ver.nupkg"
Write-Host "Downloading: $url"
Invoke-WebRequest -Uri $url -OutFile $nupkg -UseBasicParsing -TimeoutSec 60
$sz = (Get-Item $nupkg).Length
Write-Host "Downloaded $sz bytes"

# 解压
Add-Type -AssemblyName System.IO.Compression.FileSystem
$zip = [System.IO.Compression.ZipFile]::OpenRead($nupkg)
Write-Host "Entries in nupkg:"
foreach ($e in $zip.Entries) { Write-Host "  $($e.FullName)" }
$zip.Dispose()
