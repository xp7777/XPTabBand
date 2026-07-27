@echo off
chcp 65001 >nul
setlocal

:: ============================================================
:: XPTabBand 卸载脚本
:: 必须以管理员身份运行
:: ============================================================

:: 检查管理员权限
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo [错误] 需要管理员权限运行此脚本！
    echo 请右键此文件，选择"以管理员身份运行"。
    pause
    exit /b 1
)

echo ============================================================
echo  XPTabBand 卸载程序
echo ============================================================
echo.

set "INSTALL_DIR=%ProgramFiles%\XPTabBand"
set "DLL_PATH=%INSTALL_DIR%\XPTabBand.dll"
set "CLSID={A1B2C3D4-1234-4ABC-9DEF-1234567890AB}"

:: 询问是否同时删除收藏夹数据
set "DEL_FAV=N"
set /p "DEL_FAV=是否同时删除收藏夹数据？(Y/N，默认 N): "
if /i "%DEL_FAV%"=="Y" (
    set "DEL_FAV=YES"
) else (
    set "DEL_FAV=NO"
)

echo.
echo [1/5] 注销 COM 组件...
if exist "%DLL_PATH%" (
    regsvr32 /u /s "%DLL_PATH%"
    if %errorlevel% neq 0 (
        echo        [警告] regsvr32 /u 返回错误码 %errorlevel%
    )
) else (
    echo        [警告] 未找到 DLL：%DLL_PATH%
)
echo        完成
echo.

echo [2/5] 清理注册表残留...
reg delete "HKLM\SOFTWARE\Classes\CLSID\%CLSID%" /f >nul 2>&1
reg delete "HKLM\SOFTWARE\Classes\Component Categories\{00021493-0000-0000-C000-000000000046}\Implemented Categories\%CLSID%" /f >nul 2>&1
reg delete "HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar" /v "%CLSID%" /f >nul 2>&1

:: WOW64 视图（32 位注册表视图）
reg delete "HKLM\SOFTWARE\WOW6432Node\Classes\CLSID\%CLSID%" /f >nul 2>&1
reg delete "HKLM\SOFTWARE\WOW6432Node\Microsoft\Internet Explorer\Toolbar" /v "%CLSID%" /f >nul 2>&1
echo        完成
echo.

echo [3/5] 关闭资源管理器...
taskkill /f /im explorer.exe >nul 2>&1
timeout /t 2 /nobreak >nul
echo        完成
echo.

echo [4/5] 删除安装文件...
if exist "%INSTALL_DIR%" (
    rmdir /s /q "%INSTALL_DIR%"
    if exist "%INSTALL_DIR%" (
        echo        [警告] 文件被占用，无法删除：%INSTALL_DIR%
        echo               请手动重启电脑后删除该目录
    ) else (
        echo        完成
    )
) else (
    echo        [跳过] 目录不存在
)
echo.

echo [5/5] 删除收藏夹数据...
if "%DEL_FAV%"=="YES" (
    if exist "%APPDATA%\XPTabCpp" (
        rmdir /s /q "%APPDATA%\XPTabCpp"
        echo        完成
    ) else (
        echo        [跳过] 无收藏夹数据
    )
) else (
    echo        [跳过] 用户选择保留收藏夹数据
)
echo.

echo 启动资源管理器...
start explorer.exe
timeout /t 3 /nobreak >nul

echo ============================================================
echo  卸载完成！
echo ============================================================
echo.
echo 如果"查看 → 工具栏"中仍显示 "XPTabBand" 选项，请重启电脑。
echo.
pause
