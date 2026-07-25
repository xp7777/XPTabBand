$clsid = "{a1b2c3d4-e5f6-4789-abcd-0123456789ab}"

Write-Host "=== 1. Toolbar 注册项 ===" -ForegroundColor Cyan
$k = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SOFTWARE\Microsoft\Internet Explorer\Toolbar\$clsid")
if ($k) { Write-Host "  存在，默认值: $($k.GetValue(''))"; $k.Close() } else { Write-Host "  不存在!" -ForegroundColor Red }

Write-Host "`n=== 2. CLSID InprocServer32 ===" -ForegroundColor Cyan
$k = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SOFTWARE\Classes\CLSID\$clsid\InprocServer32")
if ($k) { Write-Host "  存在"; Write-Host "  ThreadingModel: $($k.GetValue('ThreadingModel'))"; Write-Host "  Codebase: $($k.GetValue('Codebase'))"; $k.Close() } else { Write-Host "  不存在!" -ForegroundColor Red }

Write-Host "`n=== 3. Approved Shell Extensions ===" -ForegroundColor Cyan
$k = [Microsoft.Win32.Registry]::LocalMachine.OpenSubKey("SOFTWARE\Microsoft\Windows\CurrentVersion\Shell Extensions\Approved")
if ($k) { $v = $k.GetValue($clsid); Write-Host "  值: '$v'"; $k.Close() } else { Write-Host "  Approved 键不存在!" -ForegroundColor Red }

Write-Host "`n=== 4. EnableLegacyBars (Win11) ===" -ForegroundColor Cyan
$k = [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey("SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Advanced")
if ($k) { Write-Host "  EnableLegacyBars: $($k.GetValue('EnableLegacyBars'))"; $k.Close() }

Write-Host "`n=== 5. Windows 版本 ===" -ForegroundColor Cyan
$os = Get-CimInstance Win32_OperatingSystem
Write-Host "  $($os.Caption) Build $($os.BuildNumber)"

Write-Host "`n=== 6. explorer.exe 进程位数 ===" -ForegroundColor Cyan
$exp = Get-Process explorer | Select-Object -First 1
Write-Host "  explorer.exe Path: $($exp.Path)"
