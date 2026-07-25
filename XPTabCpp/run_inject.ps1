$p = Start-Process -FilePath 'g:\Test\testFileExplorerPro\XPTabCpp\build\run_inject.bat' -Verb RunAs -PassThru -Wait
Write-Host ('ExitCode: ' + $p.ExitCode)
