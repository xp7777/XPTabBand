@echo off
chcp 65001 >nul
echo === 注册 XPTabBand DeskBand ===
echo DLL: g:\Test\testFileExplorerPro\XPTabCpp\XPTabBand\build\XPTabBand.dll

regsvr32 /s "g:\Test\testFileExplorerPro\XPTabCpp\XPTabBand\build\XPTabBand.dll"
if %errorlevel% == 0 (
    echo regsvr32 成功
) else (
    echo regsvr32 失败 errorlevel=%errorlevel%
    echo 手动注册...
    reg add "HKLM\SOFTWARE\Classes\CLSID\{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}" /ve /d "XPTabBand" /f
    reg add "HKLM\SOFTWARE\Classes\CLSID\{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}\InprocServer32" /ve /d "g:\Test\testFileExplorerPro\XPTabCpp\XPTabBand\build\XPTabBand.dll" /f
    reg add "HKLM\SOFTWARE\Classes\CLSID\{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}\InprocServer32" /v "ThreadingModel" /t REG_SZ /d "Apartment" /f
    reg add "HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar" /v "{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}" /t REG_SZ /d "XPTabBand" /f
)

echo.
echo === 验证注册 ===
reg query "HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar" /v "{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}"

echo.
echo === 重启 explorer ===
taskkill /f /im explorer.exe
timeout /t 2 /nobreak >nul
start explorer.exe
timeout /t 3 /nobreak >nul

echo.
echo === 注册完成 ===
echo 请打开 Explorer 窗口，在"查看"->"工具栏"中勾选 XPTabBand
pause
