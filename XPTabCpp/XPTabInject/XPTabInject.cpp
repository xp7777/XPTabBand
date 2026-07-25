// XPTabInject.cpp - Explorer 标签页注入器主程序
// 实现经典的 CreateRemoteThread + LoadLibraryW 注入方式
// 用法：XPTabInject.exe [-install|-uninstall]
//   -install  : 注入 XPTabHook.dll 到 explorer.exe（默认行为）
//   -uninstall: 通过自定义消息通知已注入的 DLL 卸载

#include "stdafx.h"

// 自定义卸载消息（与 XPTabHook 的 ExplorerHook 约定一致）
// 当 explorer 主窗口收到此消息时，子类化窗口过程会触发卸载流程
#define WM_XPTAB_UNINSTALL (WM_USER + 0x100)

// ====================================================================
// 启用当前进程的 SeDebugPrivilege，以便能 OpenProcess 打开系统进程
// ====================================================================
static BOOL EnableDebugPrivilege()
{
    HANDLE hToken = NULL;
    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          &hToken))
    {
        return FALSE;
    }

    LUID luid;
    if (!LookupPrivilegeValueW(NULL, SE_DEBUG_NAME, &luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    // AdjustTokenPrivileges 即使返回 TRUE 也可能因权限不足而失败，需检查 GetLastError
    BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);
    DWORD err = GetLastError();
    CloseHandle(hToken);

    return result && (err == ERROR_SUCCESS);
}

// ====================================================================
// 通过 CreateToolhelp32Snapshot 枚举进程，查找所有 explorer.exe 的 PID
// Windows 10/11 的 Explorer 是多进程架构：主进程（任务栏）和各文件夹窗口
// 返回所有 explorer.exe 进程的 PID 列表
// ====================================================================
static std::vector<DWORD> FindAllExplorerProcessIds()
{
    std::vector<DWORD> pids;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        return pids;
    }

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(hSnapshot, &pe))
    {
        do
        {
            if (_wcsicmp(pe.szExeFile, L"explorer.exe") == 0)
            {
                pids.push_back(pe.th32ProcessID);
            }
        } while (Process32NextW(hSnapshot, &pe));
    }

    CloseHandle(hSnapshot);
    return pids;
}

// ====================================================================
// 经典 CreateRemoteThread + LoadLibraryW 注入
// 参数：
//   pid     - 目标进程 ID
//   dllPath - DLL 完整路径（宽字符）
// 返回 TRUE 表示注入成功
// ====================================================================
static BOOL InjectDll(DWORD pid, const std::wstring& dllPath)
{
    wprintf(L"[XPTabInject] 开始注入，目标 PID: %lu\n", pid);
    wprintf(L"[XPTabInject] DLL 路径: %s\n", dllPath.c_str());

    // 1. 打开目标进程
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProcess)
    {
        wprintf(L"[XPTabInject] OpenProcess 失败，错误码: %lu\n", GetLastError());
        return FALSE;
    }
    wprintf(L"[XPTabInject] OpenProcess 成功\n");

    // 2. 在目标进程中分配内存以存放 DLL 路径
    SIZE_T pathSize = (dllPath.size() + 1) * sizeof(wchar_t);
    LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, pathSize,
                                       MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteBuf)
    {
        wprintf(L"[XPTabInject] VirtualAllocEx 失败，错误码: %lu\n", GetLastError());
        CloseHandle(hProcess);
        return FALSE;
    }
    wprintf(L"[XPTabInject] VirtualAllocEx 成功，远程地址: 0x%p\n", pRemoteBuf);

    // 3. 将 DLL 路径写入目标进程
    SIZE_T written = 0;
    if (!WriteProcessMemory(hProcess, pRemoteBuf, dllPath.c_str(), pathSize, &written))
    {
        wprintf(L"[XPTabInject] WriteProcessMemory 失败，错误码: %lu\n", GetLastError());
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }
    wprintf(L"[XPTabInject] WriteProcessMemory 成功，写入 %zu 字节\n", written);

    // 4. 获取 LoadLibraryW 的地址（kernel32 在所有进程中加载地址相同）
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32)
    {
        wprintf(L"[XPTabInject] GetModuleHandleW(kernel32.dll) 失败\n");
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }

    LPVOID pLoadLibrary = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibrary)
    {
        wprintf(L"[XPTabInject] GetProcAddress(LoadLibraryW) 失败\n");
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }
    wprintf(L"[XPTabInject] LoadLibraryW 地址: 0x%p\n", pLoadLibrary);

    // 5. 创建远程线程执行 LoadLibraryW(我们的 DLL 路径)
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
                                        (LPTHREAD_START_ROUTINE)pLoadLibrary,
                                        pRemoteBuf, 0, NULL);
    if (!hThread)
    {
        wprintf(L"[XPTabInject] CreateRemoteThread 失败，错误码: %lu\n", GetLastError());
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return FALSE;
    }
    wprintf(L"[XPTabInject] CreateRemoteThread 成功\n");

    // 6. 等待远程线程完成（最多 10 秒）
    wprintf(L"[XPTabInject] 等待远程线程完成...\n");
    DWORD waitResult = WaitForSingleObject(hThread, 10000);
    if (waitResult == WAIT_TIMEOUT)
    {
        wprintf(L"[XPTabInject] 警告：等待远程线程超时\n");
    }
    else
    {
        wprintf(L"[XPTabInject] 远程线程执行完成\n");
    }

    // 7. 清理资源
    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
    CloseHandle(hProcess);

    wprintf(L"[XPTabInject] 注入流程结束\n");
    return TRUE;
}

// ====================================================================
// 卸载流程：向所有 CabinetWClass 窗口发送 WM_XPTAB_UNINSTALL
// 已注入的子类化窗口过程会收到消息，触发 FreeLibraryAndExitThread
// ====================================================================
static BOOL UninstallFromExplorer()
{
    wprintf(L"[XPTabInject] 开始卸载，枚举 Explorer 窗口...\n");

    struct EnumData
    {
        int count;
    };
    EnumData data = { 0 };

    // 枚举所有顶层窗口
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL
    {
        EnumData* pData = reinterpret_cast<EnumData*>(lParam);
        wchar_t className[256] = { 0 };
        if (GetClassNameW(hwnd, className, 256) > 0)
        {
            // CabinetWClass 是 Windows Explorer 主窗口类名
            if (wcscmp(className, L"CabinetWClass") == 0)
            {
                SendMessageW(hwnd, WM_XPTAB_UNINSTALL, 0, 0);
                pData->count++;
                wprintf(L"[XPTabInject] 已向窗口 0x%p 发送卸载消息\n",
                        static_cast<void*>(hwnd));
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data));

    wprintf(L"[XPTabInject] 卸载完成，共处理 %d 个窗口\n", data.count);
    return data.count > 0;
}

// ====================================================================
// 获取 XPTabHook.dll 的路径
// DLL 与注入器位于同一目录下
// ====================================================================
static std::wstring GetHookDllPath()
{
    wchar_t exePath[MAX_PATH] = { 0 };
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    std::wstring path(exePath);
    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
    {
        path = path.substr(0, pos + 1);
    }
    path += L"XPTabHook.dll";
    return path;
}

// ====================================================================
// 主函数
// ====================================================================
int wmain(int argc, wchar_t* argv[])
{
    // 设置控制台输出为 UTF-16，以便正确显示中文
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stderr), _O_U16TEXT);

    wprintf(L"=== XPTabInject - Explorer 标签页注入器 ===\n");
    wprintf(L"用法: XPTabInject.exe [-install|-uninstall]\n\n");

    // 解析命令行参数
    bool install = true;
    if (argc >= 2)
    {
        if (_wcsicmp(argv[1], L"-uninstall") == 0)
        {
            install = false;
        }
        else if (_wcsicmp(argv[1], L"-install") == 0)
        {
            install = true;
        }
        else
        {
            wprintf(L"错误：未知参数 %s\n", argv[1]);
            wprintf(L"用法: XPTabInject.exe [-install|-uninstall]\n");
            return 1;
        }
    }

    // 启用 SeDebugPrivilege
    wprintf(L"[XPTabInject] 启用 SeDebugPrivilege...\n");
    if (!EnableDebugPrivilege())
    {
        wprintf(L"[XPTabInject] 警告：启用 SeDebugPrivilege 失败，错误码: %lu\n",
                GetLastError());
        wprintf(L"[XPTabInject] 提示：请确认以管理员身份运行\n");
    }
    else
    {
        wprintf(L"[XPTabInject] SeDebugPrivilege 已启用\n");
    }

    if (install)
    {
        // 注入流程
        wprintf(L"\n--- 注入模式 ---\n");
        wprintf(L"[XPTabInject] 查找所有 explorer.exe 进程...\n");

        std::vector<DWORD> pids = FindAllExplorerProcessIds();
        if (pids.empty())
        {
            wprintf(L"[XPTabInject] 错误：未找到 explorer.exe 进程\n");
            return 1;
        }
        wprintf(L"[XPTabInject] 找到 %zu 个 explorer.exe 进程\n", pids.size());

        // 检查 DLL 是否存在
        std::wstring dllPath = GetHookDllPath();
        if (GetFileAttributesW(dllPath.c_str()) == INVALID_FILE_ATTRIBUTES)
        {
            wprintf(L"[XPTabInject] 错误：找不到 XPTabHook.dll: %s\n", dllPath.c_str());
            wprintf(L"[XPTabInject] 请确保 DLL 与注入器在同一目录\n");
            return 1;
        }

        // 逐一注入每个 explorer.exe 进程
        int successCount = 0;
        for (DWORD pid : pids)
        {
            wprintf(L"\n[XPTabInject] --- 注入 PID %lu ---\n", pid);
            if (InjectDll(pid, dllPath))
            {
                successCount++;
            }
            else
            {
                wprintf(L"[XPTabInject] PID %lu 注入失败，继续下一个\n", pid);
            }
        }

        wprintf(L"\n[XPTabInject] 注入完成：成功 %d / 总计 %zu\n", successCount, pids.size());
        if (successCount == 0)
        {
            return 1;
        }
    }
    else
    {
        // 卸载流程
        wprintf(L"\n--- 卸载模式 ---\n");
        if (!UninstallFromExplorer())
        {
            wprintf(L"[XPTabInject] 警告：未找到任何已注入的 Explorer 窗口\n");
            return 1;
        }
    }

    wprintf(L"\n[XPTabInject] 操作完成\n");
    return 0;
}
