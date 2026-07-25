Write-Host "=== Searching for Visual Studio installations ==="

# Check vswhere
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vswhere) {
    Write-Host "vswhere found: $vswhere"
    Write-Host "--- VS installations ---"
    & $vswhere -all -prerelease -format json 2>&1 | ForEach-Object { Write-Host $_ }
} else {
    Write-Host "vswhere NOT found"
}

Write-Host ""
Write-Host "=== Searching for MSBuild ==="
$msbuilds = @()
$msbuilds += Get-ChildItem 'C:\Program Files*\Microsoft Visual Studio\*\*\MSBuild\Current\Bin\MSBuild.exe' -ErrorAction SilentlyContinue
$msbuilds += Get-ChildItem 'D:\Program Files*\Microsoft Visual Studio\*\*\MSBuild\Current\Bin\MSBuild.exe' -ErrorAction SilentlyContinue
if ($msbuilds.Count -gt 0) {
    $msbuilds | ForEach-Object { Write-Host "MSBuild: $($_.FullName)" }
} else {
    Write-Host "No MSBuild found"
}

Write-Host ""
Write-Host "=== Searching for cl.exe (C++ compiler) ==="
$cls = @()
$cls += Get-ChildItem 'C:\Program Files*\Microsoft Visual Studio\*\*\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe' -ErrorAction SilentlyContinue
$cls += Get-ChildItem 'D:\Program Files*\Microsoft Visual Studio\*\*\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe' -ErrorAction SilentlyContinue
if ($cls.Count -gt 0) {
    $cls | ForEach-Object { Write-Host "cl.exe: $($_.FullName)" }
} else {
    Write-Host "No cl.exe found"
}

Write-Host ""
Write-Host "=== Searching for Windows SDK ==="
$sdks = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\Include' -ErrorAction SilentlyContinue
if ($sdks) {
    $sdks | ForEach-Object { Write-Host "SDK: $($_.FullName)" }
} else {
    Write-Host "No Windows SDK found"
}

Write-Host ""
Write-Host "=== Check g++ / clang ==="
$gpp = Get-Command g++ -ErrorAction SilentlyContinue
if ($gpp) { Write-Host "g++: $($gpp.Source)" } else { Write-Host "g++: not found" }

$clang = Get-Command clang -ErrorAction SilentlyContinue
if ($clang) { Write-Host "clang: $($clang.Source)" } else { Write-Host "clang: not found" }

$gcc = Get-Command gcc -ErrorAction SilentlyContinue
if ($gcc) { Write-Host "gcc: $($gcc.Source)" } else { Write-Host "gcc: not found" }
