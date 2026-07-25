$vsPaths = @(
    'C:\Program Files\Microsoft Visual Studio\2022\Community',
    'C:\Program Files\Microsoft Visual Studio\2022\Professional',
    'C:\Program Files\Microsoft Visual Studio\2022\Enterprise'
)

$found = $false
foreach ($p in $vsPaths) {
    if (Test-Path $p) {
        Write-Host "FOUND VS 2022: $p"
        $found = $true
        $vcvars = "$p\VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $vcvars) {
            Write-Host "  vcvars64.bat: EXISTS"
        } else {
            Write-Host "  vcvars64.bat: MISSING"
        }
    }
}

if (-not $found) {
    Write-Host "VS 2022 NOT FOUND in standard locations"
}

$msbuild = Get-ChildItem 'C:\Program Files\Microsoft Visual Studio\2022\*\MSBuild\Current\Bin\MSBuild.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
if ($msbuild) {
    Write-Host "MSBuild: $($msbuild.FullName)"
} else {
    Write-Host "MSBuild: NOT FOUND"
}

# Check for cl.exe
$cl = Get-ChildItem 'C:\Program Files\Microsoft Visual Studio\2022\*\VC\Tools\MSVC\*\bin\Hostx64\x64\cl.exe' -ErrorAction SilentlyContinue | Select-Object -First 1
if ($cl) {
    Write-Host "cl.exe: $($cl.FullName)"
} else {
    Write-Host "cl.exe: NOT FOUND (C++ workload may not be installed)"
}

# Check Windows SDK
$sdk = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\Include\*' -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1
if ($sdk) {
    Write-Host "Windows SDK: $($sdk.FullName)"
} else {
    Write-Host "Windows SDK: NOT FOUND"
}
