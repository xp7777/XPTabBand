@echo off
chcp 65001 >nul
echo === 手动完整注册 XPTabBand ===

set DLL=g:\Test\testFileExplorerPro\XPTabCpp\XPTabBand\build\XPTabBand.dll
set CLSID={A1B2C3D4-1234-4ABC-9DEF-1234567890AB}

echo.
echo 1. 注册 CLSID
reg add "HKLM\SOFTWARE\Classes\CLSID\%CLSID%" /ve /d "XPTabBand" /f
reg add "HKLM\SOFTWARE\Classes\CLSID\%CLSID%\InprocServer32" /ve /d "%DLL%" /f
reg add "HKLM\SOFTWARE\Classes\CLSID\%CLSID%\InprocServer32" /v "ThreadingModel" /t REG_SZ /d "Apartment" /f

echo.
echo 2. 注册 Explorer Toolbar
reg add "HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar" /v "%CLSID%" /t REG_SZ /d "XPTabBand" /f

echo.
echo 3. 验证
reg query "HKLM\SOFTWARE\Classes\CLSID\%CLSID%\InprocServer32"
echo.
reg query "HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar" /v "%CLSID%"

echo.
echo 4. 重启 explorer
taskkill /f /im explorer.exe
timeout /t 2 /nobreak >nul
start explorer.exe
timeout /t 3 /nobreak >nul

echo.
echo === 注册完成 ===
echo 请打开新 Explorer 窗口，在 查看 Prospect 中勾选 XPTabBand
exit 0
