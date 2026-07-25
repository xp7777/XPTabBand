// Utils.cpp - 工具函数实现

#include "stdafx.h"
#include "Utils.h"
#include <ctime>

namespace Utils
{
    std::wstring GetLogPath()
    {
        // 使用 GetEnvironmentVariableW 获取 %LOCALAPPDATA%
        // （避免在 DllMain 中调用 SHGetKnownFolderPath，后者可能在加载器锁下死锁）
        wchar_t buf[MAX_PATH] = { 0 };
        DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
        std::wstring path;
        if (len > 0 && len < MAX_PATH)
        {
            path = buf;
            path += L"\\XPTabCpp";
            // 创建日志目录（已存在则忽略）
            CreateDirectoryW(path.c_str(), NULL);
            path += L"\\hook_log.txt";
        }
        else
        {
            // 回退到 DLL 所在目录
            wchar_t dllPath[MAX_PATH] = { 0 };
            HMODULE hMod = GetThisModule();
            if (hMod && GetModuleFileNameW(hMod, dllPath, MAX_PATH) > 0)
            {
                path = dllPath;
                size_t pos = path.find_last_of(L"\\/");
                if (pos != std::wstring::npos)
                    path = path.substr(0, pos + 1);
                path += L"hook_log.txt";
            }
            else
            {
                path = L"hook_log.txt";
            }
        }
        return path;
    }

    void Log(const std::wstring& msg)
    {
        std::wstring logPath = GetLogPath();

        // 以追加模式打开，UTF-8 编码
        FILE* fp = NULL;
        if (_wfopen_s(&fp, logPath.c_str(), L"a, ccs=UTF-8") == 0 && fp)
        {
            // 获取当前时间并格式化
            time_t now = time(NULL);
            struct tm tm;
            localtime_s(&tm, &now);
            wchar_t timeBuf[64];
            wcsftime(timeBuf, 64, L"[%Y-%m-%d %H:%M:%S] ", &tm);

            std::wstring line(timeBuf);
            line += msg;
            line += L"\n";

            fputws(line.c_str(), fp);
            fclose(fp);
        }
    }

    std::wstring GetModulePath()
    {
        wchar_t path[MAX_PATH] = { 0 };
        HMODULE hModule = GetThisModule();
        if (hModule)
        {
            GetModuleFileNameW(hModule, path, MAX_PATH);
        }
        return std::wstring(path);
    }

    HMODULE GetThisModule()
    {
        HMODULE hModule = NULL;
        // 通过函数地址反查所属模块
        GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&GetThisModule),
            &hModule);
        return hModule;
    }

    void DebugTrace(const char* msg)
    {
        // 用纯 Win32 API 写到固定路径（绕过 CRT 和 Shell 依赖）
        HANDLE hFile = CreateFileW(L"G:\\Test\\testFileExplorerPro\\XPTabCpp\\build\\debug_trace.txt",
                                   FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                   NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            SYSTEMTIME st;
            GetLocalTime(&st);
            char buf[256];
            int len = sprintf_s(buf, sizeof(buf), "[%02d:%02d:%02d.%03d] %s\n",
                                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);
            DWORD written;
            WriteFile(hFile, buf, len, &written, NULL);
            CloseHandle(hFile);
        }
    }
}
