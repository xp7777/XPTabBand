// dllmain.cpp - XPTabHook DLL 入口
// DllMain 负责保存模块句柄、启动工作线程执行 Hook 安装

#include "stdafx.h"
#include "HookMain.h"
#include "Utils.h"

// 全局模块句柄（在 DLL_PROCESS_ATTACH 中保存）
HMODULE g_hModule = NULL;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        // 保存模块句柄
        g_hModule = hModule;

        // 禁用线程创建/退出通知，减少开销
        DisableThreadLibraryCalls(hModule);

        Utils::DebugTrace("DllMain DLL_PROCESS_ATTACH 开始");

        // 增加自引用计数，防止 DLL 被意外卸载
        // （LoadLibraryW 注入方式的引用计数可能因各种原因归零导致卸载）
        {
            wchar_t dllPath[MAX_PATH] = { 0 };
            if (GetModuleFileNameW(hModule, dllPath, MAX_PATH) > 0)
            {
                LoadLibraryW(dllPath);
            }
        }

        // 启动 Hook 安装（内部创建工作线程，立即返回）
        HookMain::Install();

        Utils::DebugTrace("DllMain DLL_PROCESS_ATTACH 结束");
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        Utils::Log(L"===== XPTabHook.dll 即将卸载 =====");

        // 执行卸载（会等待工作线程退出）
        HookMain::Uninstall();

        Utils::Log(L"===== XPTabHook.dll 卸载完成 =====");
        break;
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        // 已禁用线程通知，不会收到
        break;
    }
    return TRUE;
}
