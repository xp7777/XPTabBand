@echo off
chcp 65001 >nul
setlocal

:: ============================================================
:: XPTabBand 安装脚本
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
echo  XPTabBand 安装程序
echo ============================================================
echo.

:: 安装目录
set "INSTALL_DIR=%ProgramFiles%\XPTabBand"
set "DLL_NAME=XPTabBand.dll"

:: 获取脚本所在目录
set "SRC_DIR=%~dp0"

:: 检查 DLL 是否存在
if not exist "%SRC_DIR%%DLL_NAME%" (
    echo [错误] 未找到 %DLL_NAME%
    echo 请确保此安装包完整，DLL 文件位于：%SRC_DIR%
    pause
    exit /b 1
)

:: 创建安装目录
echo [1/4] 创建安装目录...
if not exist "%INSTALL_DIR%" (
    mkdir "%INSTALL_DIR%"
    if %errorlevel% neq 0 (
        echo [错误] 创建目录失败：%INSTALL_DIR%
        pause
        exit /b 1
    )
)
echo     完成：%INSTALL_DIR%
echo.

:: 复制 DLL
echo [2/4] 复制文件...
copy /Y "%SRC_DIR%%DLL_NAME%" "%INSTALL_DIR%\%DLL_NAME%" >nul
if %errorlevel% neq 0 (
    echo [错误] 复制 DLL 失败
    pause
    exit /b 1
)
echo     完成：%INSTALL_DIR%\%DLL_NAME%
echo.

:: 关闭 Explorer
echo [3/4] 关闭资源管理器...
taskkill /f /im explorer.exe >nul 2>&1
timeout /t 2 /nobreak >nul
echo     完成
echo.

:: 注册 COM 组件
echo [4/4] 注册 COM 组件...
regsvr32 /s "%INSTALL_DIR%\%DLL_NAME%"
if %errorlevel% neq 0 (
    echo [警告] regsvr32 返回错误码 %errorlevel%
    echo        尝试手动注册...
    reg add "HKLM\SOFTWARE\Classes\CLSID\{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}" /ve /d "XPTabBand" /f >nul
    reg add "HKLM\SOFTWARE\Classes\CLSID\{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}\InprocServer32" /ve /d "%INSTALL_DIR%\%DLL_NAME%" /f >nul
    reg add "HKLM\SOFTWARE\Classes\CLSID\{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}\InprocServer32" /v "ThreadingModel" /t REG_SZ /d "Apartment" /f >nul
    reg add "HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar" /v "{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}" /t REG_SZ /d "XPTabBand" /f >nul
)
echo     完成
echo.

:: 启动 Explorer
echo 启动资源管理器...
start explorer.exe
timeout /t 3 /nobreak >nul

echo ============================================================
echo  安装完成！
echo ============================================================
echo.
echo 接下来请手动启用标签栏：
echo   1. 打开任意文件夹
echo   2. 在顶部菜单栏右键（或按 Ctrl+Shift+滚轮）
echo   3. 选择"查看" → "工具栏"
echo   4. 勾选 "XPTabBand"
echo.
echo 如果未看到 "XPTabBand" 选项，请重启电脑后重试。
echo.
pause
