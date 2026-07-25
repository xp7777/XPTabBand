@echo off
REM build.bat - XPTabCpp 构建脚本
REM 使用 MSBuild 编译 Release|x64 配置，输出到 build\ 目录

setlocal

set MSBUILD="D:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
set SOLUTION=%~dp0XPTab.sln
set CONFIG=Release
set PLATFORM=x64

echo ==================================================
echo   XPTabCpp 构建脚本
echo   解决方案: %SOLUTION%
echo   配置: %CONFIG%^|%PLATFORM%
echo ==================================================
echo.

REM ---- 检查 MSBuild 是否存在 ----
if not exist %MSBUILD% (
    echo [错误] 未找到 MSBuild: %MSBUILD%
    echo 请确认 Visual Studio 2022 安装路径正确。
    exit /b 1
)

REM ---- 检查 MinHook 是否已安装 ----
echo [检查] MinHook 源码...
if not exist "%~dp0MinHook\hook.c" (
    echo.
    echo [错误] MinHook 源码未安装！
    echo.
    echo 请从 https://github.com/TsudaKageyu/minhook 下载源码，
    echo 将 src/ 目录下的文件复制到 MinHook\ 目录。
    echo 用下载的 include/MinHook.h 替换占位文件。
    echo.
    echo 详见 MinHook\README.md
    echo.
    exit /b 1
)
echo [完成] MinHook 已安装。
echo.

REM ---- 创建输出目录 ----
if not exist "%~dp0build" (
    mkdir "%~dp0build"
)

REM ---- 开始编译 ----
echo [编译] 开始构建 %CONFIG%^|%PLATFORM% ...
echo.
%MSBUILD% %SOLUTION% /p:Configuration=%CONFIG% /p:Platform=%PLATFORM% /t:Rebuild /v:normal
if errorlevel 1 (
    echo.
    echo ==================================================
    echo   [失败] 编译失败！请检查上方错误信息。
    echo ==================================================
    exit /b 1
)

REM ---- 编译完成 ----
echo.
echo ==================================================
echo   [成功] 编译完成！
echo ==================================================
echo.
echo 输出目录: %~dp0build\
echo   - XPTabInject.exe    注入器
echo   - XPTabHook.dll      Hook 动态库
echo.
echo 使用方法:
echo   1. 以管理员身份运行: XPTabInject.exe -install
echo      （注入标签页功能到 explorer.exe）
echo   2. 卸载: XPTabInject.exe -uninstall
echo.
echo 日志文件: %%LOCALAPPDATA%%\XPTabCpp\hook_log.txt
echo.

endlocal
