// HookMain.cpp - Hook 主逻辑实现
//
// 架构说明（v2 - 使用 SetWinEventHook）：
//   DLL 注入到 explorer.exe 后，在工作线程中安装 SetWinEventHook（WINEVENT_INCONTEXT）。
//   WinEvent 回调在产生事件的线程上下文执行（即 Explorer UI 线程），
//   因此可以安全调用 GetWindowLongPtrW / SetWindowLongPtrW 进行窗口子类化。
//
//   关键点：
//   - WH_GETMESSAGE 的线程特定钩子回调在"安装线程"执行，不在目标线程
//   - SetWinEventHook + WINEVENT_INCONTEXT 的回调在"产生事件的线程"执行
//   - 因此 WinEventProc 可以安全地子类化当前线程拥有的窗口

#include "stdafx.h"
#include "HookMain.h"
#include "ExplorerHook.h"
#include "Utils.h"

namespace HookMain
{
    // 自定义消息：请求工作线程退出
    #define WM_XPTAB_QUIT (WM_USER + 0x200)

    // 全局状态
    static HANDLE g_hWorkerThread = NULL;
    static HWND g_hMsgWindow = NULL;
    static HWINEVENTHOOK g_hWinEventHook = NULL;
    static std::atomic<bool> g_installed{ false };
    static std::atomic<bool> g_uninstalling{ false };

    // 消息窗口类名
    static const wchar_t* kMsgWndClass = L"XPTabHookMsgWnd";
    // 定时器 ID 和间隔（毫秒）——用于重试安装 WinEvent 钩子
    static const UINT_PTR kTimerId = 1;
    static const UINT kTimerInterval = 2000;

    // 标签栏导航检查节流间隔（毫秒）
    static const DWORD kTickIntervalMs = 800;
    static DWORD g_lastTickTick = 0;

    // 前向声明：WinEvent 回调（在产生事件的线程上下文执行）
    VOID CALLBACK WinEventProc(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD dwEventThread,
        DWORD dwmsEventTime);

    // ----------------------------------------------------------------
    // 安装 SetWinEventHook（WINEVENT_INCONTEXT）
    // 回调函数在产生事件的线程上下文执行
    // ----------------------------------------------------------------
    static bool TryInstallWinEventHook()
    {
        if (g_hWinEventHook)
            return true; // 已安装

        HMODULE hThisModule = Utils::GetThisModule();
        g_hWinEventHook = SetWinEventHook(
            EVENT_OBJECT_CREATE,        // eventMin
            EVENT_OBJECT_NAMECHANGE,    // eventMax（覆盖 CREATE/DESTROY/SHOW/HIDE/FOCUS/STATECHANGE/LOCATIONCHANGE/NAMECHANGE）
            hThisModule,                // hmodWinEventProc（DLL 模块）
            WinEventProc,               // pfnWinEventProc
            GetCurrentProcessId(),      // idProcess（仅当前进程）
            0,                          // idThread（进程内所有线程）
            WINEVENT_INCONTEXT);        // dwflags（回调在事件产生线程执行）

        if (g_hWinEventHook)
        {
            Utils::DebugTrace("SetWinEventHook 已安装 (WINEVENT_INCONTEXT)");
            Utils::Log(L"SetWinEventHook 已安装 (WINEVENT_INCONTEXT)");
            return true;
        }
        else
        {
            char buf[128];
            sprintf_s(buf, sizeof(buf), "SetWinEventHook 失败 err=%lu", GetLastError());
            Utils::DebugTrace(buf);
            Utils::Log(L"错误: SetWinEventHook 失败: " + std::to_wstring(GetLastError()));
            return false;
        }
    }

    // ----------------------------------------------------------------
    // WinEvent 回调：在产生事件的线程上下文执行
    // 这里可以安全地调用 GetWindowLongPtrW / SetWindowLongPtrW
    // ----------------------------------------------------------------
    VOID CALLBACK WinEventProc(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD dwEventThread,
        DWORD dwmsEventTime)
    {
        if (g_uninstalling.load())
            return;

        // 只处理窗口级别的事件
        if (idObject != OBJID_WINDOW)
            return;

        // 调试日志：记录回调被调用（节流，避免刷屏）
        static DWORD s_lastDbgTick = 0;
        DWORD now0 = GetTickCount();
        if (now0 - s_lastDbgTick >= 2000)
        {
            s_lastDbgTick = now0;
            char dbg[256];
            sprintf_s(dbg, sizeof(dbg),
                "WinEventProc event=0x%04X hwnd=0x%p tid=%lu curTid=%lu",
                event, hwnd, dwEventThread, GetCurrentThreadId());
            Utils::DebugTrace(dbg);
        }

        // 尝试子类化 CabinetWClass 窗口（HookWindow 内部有线程检查）
        if (hwnd && IsWindow(hwnd))
        {
            ExplorerHook::HookWindow(hwnd);
        }

        // 节流：定期驱动当前线程的标签栏检查导航变化
        DWORD now = GetTickCount();
        if (now - g_lastTickTick >= kTickIntervalMs)
        {
            g_lastTickTick = now;
            ExplorerHook::TickTabBarsOnCurrentThread();
        }
    }

    // ----------------------------------------------------------------
    // 消息窗口过程：处理定时器和退出消息
    // ----------------------------------------------------------------
    static LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_TIMER:
            // 若 WinEvent 钩子尚未安装，定期重试
            if (!g_uninstalling.load() && g_hWinEventHook == NULL)
            {
                TryInstallWinEventHook();
            }
            // 定期主动枚举所有 CabinetWClass 窗口（兜底机制）
            // 解决 DLL 注入时窗口已创建、未收到创建事件的问题
            if (!g_uninstalling.load())
            {
                ExplorerHook::EnumAndHookAllWindows();
            }
            return 0;

        case WM_XPTAB_QUIT:
            // 收到退出请求，销毁窗口触发 WM_DESTROY
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            // 销毁定时器并退出消息循环
            KillTimer(hwnd, kTimerId);
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    // ----------------------------------------------------------------
    // 工作线程函数：执行完整的 Hook 生命周期
    // ----------------------------------------------------------------
    static DWORD WINAPI WorkerProc(LPVOID /*param*/)
    {
        Utils::DebugTrace("WorkerProc 启动");
        Utils::Log(L"===== XPTabHook.dll 已加载 =====");
        Utils::Log(L"DLL 路径: " + Utils::GetModulePath());
        Utils::Log(L"工作线程启动");

        HMODULE hThisModule = Utils::GetThisModule();

        // 1. 安装 SetWinEventHook（WINEVENT_INCONTEXT）
        //    回调在产生事件的线程上下文执行，可以安全子类化窗口
        Utils::Log(L"尝试安装 SetWinEventHook...");
        TryInstallWinEventHook();

        // 2. 注册消息窗口类
        WNDCLASSEXW wc = { 0 };
        wc.cbSize = sizeof(wc);
        wc.lpfnWndProc = MsgWndProc;
        wc.hInstance = hThisModule;
        wc.lpszClassName = kMsgWndClass;
        RegisterClassExW(&wc);

        // 3. 创建消息窗口（HWND_MESSAGE：消息专用窗口，不会被 EnumWindows 枚举）
        g_hMsgWindow = CreateWindowExW(0, kMsgWndClass, L"", 0,
                                       0, 0, 0, 0, HWND_MESSAGE, NULL, hThisModule, NULL);
        if (!g_hMsgWindow)
        {
            Utils::Log(L"错误: 创建消息窗口失败: " + std::to_wstring(GetLastError()));
        }
        else
        {
            // 4. 设置定时器：若 WinEvent 钩子尚未安装，定期重试
            SetTimer(g_hMsgWindow, kTimerId, kTimerInterval, NULL);
            Utils::Log(L"定时器已设置，间隔 " + std::to_wstring(kTimerInterval) + L" ms");
        }

        g_installed.store(true);
        Utils::Log(L"Hook 安装完成，进入消息循环");

        // 5. 消息循环（保持工作线程存活，管理钩子生命周期）
        MSG msg;
        while (GetMessageW(&msg, NULL, 0, 0) > 0)
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // 6. 清理（消息循环退出后）
        if (g_hWinEventHook)
        {
            UnhookWinEvent(g_hWinEventHook);
            g_hWinEventHook = NULL;
            Utils::Log(L"SetWinEventHook 已卸载");
        }

        // 恢复所有已子类化的窗口
        ExplorerHook::UnhookAllWindows();

        // 清理线程特定的 WH_CALLWNDPROC 钩子
        ExplorerHook::CleanupThreadHooks();

        // 反注册窗口类
        UnregisterClassW(kMsgWndClass, hThisModule);
        g_hMsgWindow = NULL;

        g_installed.store(false);
        Utils::Log(L"工作线程退出");
        return 0;
    }

    bool Install()
    {
        if (g_installed.load() || g_uninstalling.load())
        {
            Utils::Log(L"Install: 已安装或正在卸载，跳过");
            return true;
        }

        Utils::DebugTrace("Install: 创建工作线程");
        // 创建工作线程执行 Hook 安装
        g_hWorkerThread = CreateThread(NULL, 0, WorkerProc, NULL, 0, NULL);
        if (!g_hWorkerThread)
        {
            char buf[128];
            sprintf_s(buf, sizeof(buf), "CreateThread 失败 err=%lu", GetLastError());
            Utils::DebugTrace(buf);
            Utils::Log(L"错误: CreateThread(WorkerProc) 失败: " +
                       std::to_wstring(GetLastError()));
            return false;
        }
        Utils::DebugTrace("Install: 工作线程已创建");
        return true;
    }

    void Uninstall()
    {
        if (!g_installed.load() && !g_hWorkerThread)
        {
            Utils::Log(L"Uninstall: 未安装，跳过");
            return;
        }

        g_uninstalling.store(true);
        Utils::Log(L"Uninstall: 请求工作线程退出");

        // 向消息窗口发送退出消息
        if (g_hMsgWindow && IsWindow(g_hMsgWindow))
        {
            PostMessageW(g_hMsgWindow, WM_XPTAB_QUIT, 0, 0);
        }

        // 等待工作线程退出（最多 3 秒）
        if (g_hWorkerThread)
        {
            DWORD waitResult = WaitForSingleObject(g_hWorkerThread, 3000);
            if (waitResult == WAIT_TIMEOUT)
            {
                Utils::Log(L"警告: 等待工作线程退出超时");
            }
            CloseHandle(g_hWorkerThread);
            g_hWorkerThread = NULL;
        }

        g_uninstalling.store(false);
        Utils::Log(L"Uninstall: 完成");
    }

    HANDLE GetWorkerThread()
    {
        return g_hWorkerThread;
    }
}
