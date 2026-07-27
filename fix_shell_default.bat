@echo off
echo === 修复前 ===
reg query "HKCR\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\Shell"

echo.
echo === 修复中：删除 Shell 默认值 ===
reg delete "HKCR\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\Shell" /ve /f

echo.
echo === 修复后 ===
reg query "HKCR\CLSID\{20D04FE0-3AEA-1069-A2D8-08002B30309D}\Shell"

echo.
echo === 重启 explorer ===
taskkill /f /im explorer.exe
timeout /t 2 /nobreak >nul
start explorer.exe
timeout /t 3 /nobreak >nul

echo === 修复完成，请双击桌面'此电脑'测试 ===
pause
