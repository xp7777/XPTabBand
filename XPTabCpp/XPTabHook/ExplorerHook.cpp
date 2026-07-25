// ExplorerHook.cpp - Explorer 窗口枚举和子类化实现

#include "stdafx.h"
#include "ExplorerHook.h"
#include "Utils.h"

namespace ExplorerHook
{
    // Explorer 主窗口类名
    static const wchar_t* kExplorerClass = L"CabinetWClass";

    // 属性名：用于在窗口上存储原始 WndProc 指针
    static const wchar_t* kPropOrigWndProc = L"XPTabOrigWndProc";
    // 属性名：用于在窗口上存储 TabBarWindow 指针
    static const wchar_t* kPropTabBar = L"XPTabTabBar";
    // 属性名：标记窗口已被子类化（作为唯一去重判据，避免 WM_DESTROY 误触发导致重复子类化）
    static const wchar_t* kPropHooked = L"XPTabHooked";

    // 标签栏高度（像素）
    static const int kTabBarHeight = 30;

    // 自定义卸载消息（与 XPTabInject 约定一致）
    #define WM_XPTAB_UNINSTALL (WM_USER + 0x100)
    // 自定义消息：延迟创建 TabBar（避免在 WH_CALLWNDPROC 钩子回调中调用 CreateWindowEx）
    #define WM_XPTAB_CREATE_TABBAR (WM_USER + 0x101)

    // 线程同步：保护已 Hook 窗口集合
    static CRITICAL_SECTION g_cs;
    static bool g_csInitialized = false;
    // 已 Hook 的窗口集合（HWND -> TabBarWindow*）
    static std::map<HWND, TabBarWindow*> g_hookedWindows;
    // 线程特定 WH_CALLWNDPROC 钩子集合（TID -> HHOOK）
    // 用于在窗口线程上下文执行子类化（SetWindowLongPtrW 要求同线程）
    static std::map<DWORD, HHOOK> g_threadHooks;

    // 前向声明
    static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam);

    // 初始化临界区（仅一次）
    static void EnsureCsInit()
    {
        if (!g_csInitialized)
        {
            InitializeCriticalSection(&g_cs);
            g_csInitialized = true;
        }
    }

    // ----------------------------------------------------------------
    // 卸载线程函数：异步执行 FreeLibraryAndExitThread
    // 收到 WM_XPTAB_UNINSTALL 后，在新线程中卸载自身 DLL
    // ----------------------------------------------------------------
    static DWORD WINAPI UnloadThreadProc(LPVOID param)
    {
        HMODULE hModule = reinterpret_cast<HMODULE>(param);
        // 等待消息处理完成，避免在处理过程中卸载
        Sleep(200);
        Utils::Log(L"执行 FreeLibraryAndExitThread 卸载 DLL");
        if (hModule)
        {
            FreeLibraryAndExitThread(hModule, 0);
        }
        return 0;
    }

    // ----------------------------------------------------------------
    // 子类化窗口过程
    // ----------------------------------------------------------------
    LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        // 获取存储在窗口属性中的原始 WndProc
        WNDPROC origProc = reinterpret_cast<WNDPROC>(
            GetPropW(hwnd, kPropOrigWndProc));
        TabBarWindow* tabBar = reinterpret_cast<TabBarWindow*>(
            GetPropW(hwnd, kPropTabBar));

        switch (msg)
        {
        case WM_SIZE:
        {
            // 窗口大小改变时调整标签栏
            if (tabBar)
            {
                int width = LOWORD(lParam);
                tabBar->Resize(width, kTabBarHeight);
            }
            break;
        }

        case WM_MOVE:
        {
            // 窗口移动时同步 TabBar 位置（顶层窗口方案需要手动跟随）
            if (tabBar)
            {
                tabBar->UpdatePosition();
            }
            break;
        }

        case WM_NCDESTROY:
        {
            // 窗口真正销毁的最后时机（WM_DESTROY 可能被 Explorer 内部逻辑误触发，
            // 例如子窗口重建、ribbon 切换等，导致 TabBar 被误清理后又重新创建叠加）
            // 在 WM_NCDESTROY 清理才能保证窗口真的要销毁了
            Utils::Log(L"WM_NCDESTROY: 清理窗口 0x" +
                       std::to_wstring(reinterpret_cast<uintptr_t>(hwnd)));

            if (tabBar)
            {
                tabBar->Destroy();
                delete tabBar;
                RemovePropW(hwnd, kPropTabBar);
            }

            // 从已 Hook 集合中移除
            EnsureCsInit();
            EnterCriticalSection(&g_cs);
            g_hookedWindows.erase(hwnd);
            LeaveCriticalSection(&g_cs);

            // 清除子类化标志
            RemovePropW(hwnd, kPropHooked);

            // 恢复原始窗口过程
            if (origProc)
            {
                SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(origProc));
                RemovePropW(hwnd, kPropOrigWndProc);
            }
            break;
        }

        case WM_XPTAB_UNINSTALL:
        {
            // 收到注入器发来的卸载请求
            Utils::Log(L"收到卸载请求 WM_XPTAB_UNINSTALL");

            // 先恢复所有窗口
            UnhookAllWindows();

            // 启动卸载线程（不能在窗口过程中直接 FreeLibrary）
            HMODULE hModule = Utils::GetThisModule();
            HANDLE hThread = CreateThread(NULL, 0, UnloadThreadProc, hModule, 0, NULL);
            if (hThread)
                CloseHandle(hThread);
            return 0;
        }

        case WM_XPTAB_CREATE_TABBAR:
        {
            // 延迟创建 TabBar（在消息循环中调用，不在钩子回调中）
            // 双重检查防止重复创建：
            //   1) tabBar 属性（窗口属性，线程安全）
            //   2) g_hookedWindows（防止 PostMessage 多次触发）
            if (tabBar)
            {
                // 已创建，跳过
                return 0;
            }

            EnsureCsInit();
            EnterCriticalSection(&g_cs);
            auto it = g_hookedWindows.find(hwnd);
            if (it != g_hookedWindows.end() && it->second != NULL)
            {
                // 已有 TabBar 实例，跳过
                LeaveCriticalSection(&g_cs);
                return 0;
            }
            LeaveCriticalSection(&g_cs);

            tabBar = new TabBarWindow();
            bool createOk = tabBar->Create(hwnd);
            if (createOk)
            {
                SetPropW(hwnd, kPropTabBar, reinterpret_cast<HANDLE>(tabBar));

                RECT rc;
                GetClientRect(hwnd, &rc);
                tabBar->Resize(rc.right, kTabBarHeight);

                // 加入已 Hook 集合
                EnsureCsInit();
                EnterCriticalSection(&g_cs);
                g_hookedWindows[hwnd] = tabBar;
                LeaveCriticalSection(&g_cs);

                char tb[160];
                sprintf_s(tb, sizeof(tb),
                          "延迟创建 TabBar create=%d parent=0x%p width=%ld",
                          createOk ? 1 : 0, hwnd, rc.right);
                Utils::DebugTrace(tb);
            }
            else
            {
                // 创建失败，删除对象避免泄漏
                delete tabBar;
                tabBar = NULL;
                Utils::Log(L"TabBar 创建失败，放弃");
            }
            return 0;
        }
        }

        // 调用原始窗口过程处理其他消息
        if (origProc)
        {
            return CallWindowProcW(origProc, hwnd, msg, wParam, lParam);
        }
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

    // ----------------------------------------------------------------
    // 子类化单个窗口
    // ----------------------------------------------------------------
    void HookWindow(HWND hwnd)
    {
        if (!hwnd || !IsWindow(hwnd))
            return;

        // 线程检查：GetWindowLongPtrW(GWLP_WNDPROC) 要求同线程调用
        // WinEventProc 回调在产生事件的线程执行，此检查确保只子类化当前线程的窗口
        DWORD wndTid = GetWindowThreadProcessId(hwnd, NULL);
        DWORD curTid = GetCurrentThreadId();
        if (wndTid != curTid)
        {
            // 跨线程：安装 WH_CALLWNDPROC 钩子到目标线程
            // 钩子回调会在目标线程执行，届时调用 HookWindow 将是同线程
            EnsureCsInit();
            EnterCriticalSection(&g_cs);
            bool hasHook = (g_threadHooks.find(wndTid) != g_threadHooks.end());
            LeaveCriticalSection(&g_cs);

            if (!hasHook)
            {
                HHOOK hHook = SetWindowsHookExW(WH_CALLWNDPROC, CallWndProc,
                                                 Utils::GetThisModule(), wndTid);
                if (hHook)
                {
                    EnterCriticalSection(&g_cs);
                    g_threadHooks[wndTid] = hHook;
                    LeaveCriticalSection(&g_cs);
                    char buf[160];
                    sprintf_s(buf, sizeof(buf),
                              "HookWindow: 安装 WH_CALLWNDPROC tid=%lu hwnd=0x%p",
                              wndTid, hwnd);
                    Utils::DebugTrace(buf);
                    Utils::Log(L"安装 WH_CALLWNDPROC 到线程 " + std::to_wstring(wndTid));
                }
                else
                {
                    char buf[160];
                    sprintf_s(buf, sizeof(buf),
                              "HookWindow: SetWindowsHookExW 失败 tid=%lu err=%lu",
                              wndTid, GetLastError());
                    Utils::DebugTrace(buf);
                }
            }
            return;
        }

        EnsureCsInit();
        EnterCriticalSection(&g_cs);

        // 检查是否已 Hook（用窗口属性作为唯一判据）
        // g_hookedWindows 可能在 WM_DESTROY 误触发时被临时 erase，
        // 但 kPropHooked 属性只在 WM_NCDESTROY 时才清除，保证去重可靠
        if (GetPropW(hwnd, kPropHooked) != NULL)
        {
            LeaveCriticalSection(&g_cs);
            return; // 已 Hook，跳过
        }

        // 额外检查 g_hookedWindows（防止属性设置前的并发）
        if (g_hookedWindows.find(hwnd) != g_hookedWindows.end())
        {
            LeaveCriticalSection(&g_cs);
            return;
        }

        // 只子类化 CabinetWClass 窗口（WinEventProc 可能传入其他类型窗口）
        wchar_t className[256] = { 0 };
        if (GetClassNameW(hwnd, className, 256) == 0 ||
            wcscmp(className, kExplorerClass) != 0)
        {
            // 非 CabinetWClass，跳过（不记录日志避免刷屏）
            LeaveCriticalSection(&g_cs);
            return;
        }

        // 获取并保存原始窗口过程
        SetLastError(0);
        LONG_PTR origProc = GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
        DWORD err = GetLastError();
        if (!origProc)
        {
            LeaveCriticalSection(&g_cs);
            char buf[256];
            sprintf_s(buf, sizeof(buf),
                      "HookWindow FAIL hwnd=0x%p 窗口线程=%lu 当前线程=%lu err=%lu",
                      hwnd, wndTid, curTid, err);
            Utils::DebugTrace(buf);
            Utils::Log(L"HookWindow: GetWindowLongPtrW 返回 0，hwnd=0x" +
                       std::to_wstring(reinterpret_cast<uintptr_t>(hwnd)) +
                       L" 窗口线程=" + std::to_wstring(wndTid) +
                       L" 当前线程=" + std::to_wstring(curTid) +
                       L" GetLastError=" + std::to_wstring(err));
            return;
        }

        char buf[128];
        sprintf_s(buf, sizeof(buf), "HookWindow OK hwnd=0x%p 当前线程=%lu", hwnd, curTid);
        Utils::DebugTrace(buf);

        // 保存原始 WndProc 到窗口属性
        SetPropW(hwnd, kPropOrigWndProc, reinterpret_cast<HANDLE>(origProc));

        // 替换窗口过程
        SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                          reinterpret_cast<LONG_PTR>(SubclassProc));

        // 设置子类化标志（作为唯一去重判据，只在 WM_NCDESTROY 时清除）
        SetPropW(hwnd, kPropHooked, reinterpret_cast<HANDLE>(1));

        // 不在此处直接创建 TabBar！
        // 原因：HookWindow 可能在 WH_CALLWNDPROC 钩子回调中被调用，
        // 此时调用 CreateWindowExW 会发送消息触发钩子重入，导致创建静默失败（err=0 返回 NULL）。
        // 改为 PostMessage 延迟到消息循环中处理，由 SubclassProc 收到
        // WM_XPTAB_CREATE_TABBAR 后创建 TabBar。
        // 先标记为 NULL，避免 SubclassProc 提前访问
        SetPropW(hwnd, kPropTabBar, NULL);

        // 暂不加入 g_hookedWindows（等 TabBar 创建成功后再加）
        // 但需要标记窗口已 Hook，避免重复子类化
        g_hookedWindows[hwnd] = NULL;  // NULL 表示 TabBar 尚未创建

        LeaveCriticalSection(&g_cs);

        // 投递延迟创建消息
        PostMessageW(hwnd, WM_XPTAB_CREATE_TABBAR, 0, 0);

        Utils::Log(L"已子类化窗口: 0x" +
                   std::to_wstring(reinterpret_cast<uintptr_t>(hwnd)) +
                   L" (TabBar 延迟创建)");
    }

    // ----------------------------------------------------------------
    // 恢复单个窗口
    // ----------------------------------------------------------------
    void UnhookWindow(HWND hwnd)
    {
        if (!hwnd)
            return;

        EnsureCsInit();
        EnterCriticalSection(&g_cs);

        auto it = g_hookedWindows.find(hwnd);
        if (it != g_hookedWindows.end())
        {
            TabBarWindow* tabBar = it->second;

            // 销毁标签栏
            if (tabBar)
            {
                tabBar->Destroy();
                delete tabBar;
            }
            RemovePropW(hwnd, kPropTabBar);
            RemovePropW(hwnd, kPropHooked);

            // 恢复原始窗口过程
            WNDPROC origProc = reinterpret_cast<WNDPROC>(
                GetPropW(hwnd, kPropOrigWndProc));
            if (origProc)
            {
                SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                                  reinterpret_cast<LONG_PTR>(origProc));
                RemovePropW(hwnd, kPropOrigWndProc);
            }

            g_hookedWindows.erase(it);
            Utils::Log(L"已恢复窗口: 0x" +
                       std::to_wstring(reinterpret_cast<uintptr_t>(hwnd)));
        }

        LeaveCriticalSection(&g_cs);
    }

    // ----------------------------------------------------------------
    // 恢复所有窗口
    // ----------------------------------------------------------------
    void UnhookAllWindows()
    {
        EnsureCsInit();
        EnterCriticalSection(&g_cs);

        // 复制一份避免在恢复过程中修改集合
        std::vector<HWND> hwnds;
        hwnds.reserve(g_hookedWindows.size());
        for (const auto& pair : g_hookedWindows)
        {
            hwnds.push_back(pair.first);
        }

        LeaveCriticalSection(&g_cs);

        for (HWND hwnd : hwnds)
        {
            UnhookWindow(hwnd);
        }

        // 所有 TabBar 窗口已销毁，注销窗口类
        // 避免 DLL 卸载后残留失效的 lpfnWndProc 指针，导致再次注入时 CreateWindowEx 静默失败
        TabBarWindow::UnregisterWindowClass();

        Utils::Log(L"已恢复所有窗口，共 " + std::to_wstring(hwnds.size()) + L" 个");
    }

    // ----------------------------------------------------------------
    // 枚举所有 CabinetWClass 窗口并子类化
    // ----------------------------------------------------------------
    static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam)
    {
        wchar_t className[256] = { 0 };
        if (GetClassNameW(hwnd, className, 256) > 0)
        {
            if (wcscmp(className, kExplorerClass) == 0)
            {
                // 确保窗口可见
                if (IsWindowVisible(hwnd))
                {
                    HookWindow(hwnd);
                }
            }
        }
        return TRUE;
    }

    void EnumAndHookAllWindows()
    {
        EnumWindows(EnumWindowsProc, 0);
    }

    // ----------------------------------------------------------------
    // 定时回调：驱动所有 TabBarWindow 检查导航变化
    // ----------------------------------------------------------------
    void TickAllTabBars()
    {
        EnsureCsInit();
        EnterCriticalSection(&g_cs);
        std::vector<TabBarWindow*> bars;
        bars.reserve(g_hookedWindows.size());
        for (const auto& pair : g_hookedWindows)
        {
            if (pair.second)  // 跳过尚未创建的 TabBar
                bars.push_back(pair.second);
        }
        LeaveCriticalSection(&g_cs);

        for (TabBarWindow* bar : bars)
        {
            bar->OnTimerTick();
        }
    }

    // ----------------------------------------------------------------
    // 定时回调：仅驱动当前线程拥有的 TabBarWindow 检查导航变化
    // COM STA 对象（IWebBrowser2）有线程亲和性，必须在创建它的线程上调用
    // ----------------------------------------------------------------
    void TickTabBarsOnCurrentThread()
    {
        DWORD curTid = GetCurrentThreadId();

        EnsureCsInit();
        EnterCriticalSection(&g_cs);
        std::vector<TabBarWindow*> bars;
        bars.reserve(g_hookedWindows.size());
        for (const auto& pair : g_hookedWindows)
        {
            DWORD wndTid = GetWindowThreadProcessId(pair.first, NULL);
            if (wndTid == curTid && pair.second)  // 跳过尚未创建的 TabBar
            {
                bars.push_back(pair.second);
            }
        }
        LeaveCriticalSection(&g_cs);

        for (TabBarWindow* bar : bars)
        {
            bar->OnTimerTick();
        }
    }

    // ----------------------------------------------------------------
    // WH_CALLWNDPROC 回调：在目标窗口线程执行
    // 用于跨线程子类化场景：WinEventProc 在工作线程执行时，
    // 通过此钩子在窗口线程执行 HookWindow
    // ----------------------------------------------------------------
    static LRESULT CALLBACK CallWndProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HC_ACTION)
        {
            CWPSTRUCT* pcwp = reinterpret_cast<CWPSTRUCT*>(lParam);
            if (pcwp && IsWindow(pcwp->hwnd))
            {
                // 在目标线程上下文尝试 hook
                // HookWindow 内部有线程检查，此时 curTid == 窗口线程
                HookWindow(pcwp->hwnd);

                // 同时检查父窗口是否是 CabinetWClass（子窗口消息的父窗口可能是目标）
                HWND parent = GetParent(pcwp->hwnd);
                if (parent && IsWindow(parent))
                {
                    HookWindow(parent);
                }
            }
        }
        return CallNextHookEx(NULL, nCode, wParam, lParam);
    }

    // ----------------------------------------------------------------
    // 清理所有线程特定的 WH_CALLWNDPROC 钩子
    // ----------------------------------------------------------------
    void CleanupThreadHooks()
    {
        EnsureCsInit();
        EnterCriticalSection(&g_cs);
        for (const auto& pair : g_threadHooks)
        {
            if (pair.second)
            {
                UnhookWindowsHookEx(pair.second);
            }
        }
        g_threadHooks.clear();
        LeaveCriticalSection(&g_cs);
        Utils::Log(L"已清理所有 WH_CALLWNDPROC 钩子");
    }

} // namespace ExplorerHook
