Write-Host "=== Searching Windows SDK more thoroughly ==="

# Check registry for installed SDKs
$sdkRegPaths = @(
    'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots',
    'HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows Kits\Installed Roots'
)

foreach ($reg in $sdkRegPaths) {
    if (Test-Path $reg) {
        Write-Host "Registry: $reg"
        $props = Get-ItemProperty $reg
        if ($props.KitsRoot10) { Write-Host "  KitsRoot10: $($props.KitsRoot10)" }
        if ($props.KitsRoot) { Write-Host "  KitsRoot: $($props.KitsRoot)" }
    }
}

# Check common install locations
$sdkLocations = @(
    'C:\Program Files (x86)\Windows Kits',
    'C:\Program Files\Windows Kits',
    'D:\Program Files (x86)\Windows Kits',
    'D:\Program Files\Windows Kits'
)

foreach ($loc in $sdkLocations) {
    if (Test-Path $loc) {
        Write-Host "Found: $loc"
        Get-ChildItem $loc -ErrorAction SilentlyContinue | ForEach-Object {
            Write-Host "  $($_.Name)"
        }
    }
}

# Check VS installed components
Write-Host ""
Write-Host "=== VS 2022 installed components ==="
$vsPath = "D:\Program Files\Microsoft Visual Studio\2022\Community"
$vcTools = "$vsPath\VC\Tools\MSVC"
if (Test-Path $vcTools) {
    Write-Host "VC Tools versions:"
    Get-ChildItem $vcTools | ForEach-Object { Write-Host "  $($_.Name)" }
}

# Check for SDK in VS layout
$sdkInVs = "$vsPath\VC\Tools\MSVC\*\include"
Write-Host ""
Write-Host "=== Check if windows.h is available ==="
$winh = Get-ChildItem "$vsPath\VC\Tools\MSVC\*\include\windows.h" -ErrorAction SilentlyContinue
if ($winh) {
    Write-Host "windows.h found in MSVC include (unusual)"
} else {
    Write-Host "windows.h NOT in MSVC include (expected - should be in Windows SDK)"
}

# Try to find any SDK include
$anySdk = Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\Include\10.*\um\windows.h' -ErrorAction SilentlyContinue
if ($anySdk) {
    Write-Host "Windows SDK windows.h found: $($anySdk.FullName)"
} else {
    Write-Host "Windows SDK windows.h NOT found anywhere"
}
