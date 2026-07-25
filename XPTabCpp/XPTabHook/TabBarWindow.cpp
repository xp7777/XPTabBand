// TabBarWindow.cpp - 标签页栏窗口实现
// 使用 GDI 绘制暗色主题标签栏：标签（标题 + 关闭按钮 ×）+ 右侧 + 按钮
//
// 多标签工作机制：
//   - 一个 TabBarWindow 对应一个 Explorer 窗口，持有一个 IWebBrowser2
//   - 每个 TabItem 存储一个 PIDL（文件夹路径）
//   - 切换标签：调用 IWebBrowser2::Navigate2 导航到该标签的 PIDL
//   - + 按钮：复制当前标签 PIDL 新建标签
//   - × 按钮：关闭标签，最后一个标签关闭 Explorer 窗口
//   - 定时器检查：用户在 Explorer 中手动导航时更新当前标签 PIDL/标题

#include "stdafx.h"
#include <windowsx.h>
#include <cmath>
#include "TabBarWindow.h"
#include "ExplorerInterface.h"
#include "Utils.h"

// 窗口类名
static const wchar_t* kTabBarClass = L"XPTabBarClass";
// 窗口属性名：存储 TabBarWindow 指针
static const wchar_t* kPropThisPtr = L"XPTabBarThis";
// 标签窗口类是否已注册
static bool s_classRegistered = false;

// 暗色主题颜色
static const COLORREF kColorBg       = RGB(30, 30, 30);
static const COLORREF kColorTabActive  = RGB(60, 60, 60);
static const COLORREF kColorTabInactive = RGB(40, 40, 40);
static const COLORREF kColorTabHover = RGB(50, 50, 50);
static const COLORREF kColorText     = RGB(220, 220, 220);
static const COLORREF kColorClose    = RGB(200, 200, 200);
static const COLORREF kColorCloseActive = RGB(255, 100, 100);
static const COLORREF kColorPlus     = RGB(220, 220, 220);
static const COLORREF kColorSeparator = RGB(70, 70, 70);

// 命中测试返回值
static const int HIT_NONE       = -3;
static const int HIT_PLUS       = -1;
static const int HIT_CLOSE      = -2;
static const int HIT_FAVORITE   = -4;  // ☆ 收藏按钮

// 单个标签默认宽度
static const int kDefaultTabWidth = 150;
// + 按钮区域宽度
static const int kPlusButtonWidth = 28;
// 收藏按钮区域宽度（在 + 按钮右侧）
static const int kFavoriteButtonWidth = 28;
// 关闭按钮区域宽度（在标签内部右侧）
static const int kCloseButtonWidth = 20;
// 文字左边距
static const int kTextPadding = 8;

// 导航变化检查间隔（毫秒）
// 缩短到 200ms 以便及时修正 Explorer 重新布局导致的位置错乱
static const DWORD kCheckIntervalMs = 200;
// TabBar 定时器 ID
static const UINT_PTR kTabBarTimerId = 1001;

// ====================================================================
// 注册标签栏窗口类
// 关键修复：
//   - 用 EXE 模块句柄（GetModuleHandleW(NULL)）作为 hInstance，
//     与 CreateWindowExW 传入的 hInstance 一致，避免查找不匹配
//   - 如果类已存在但 lpfnWndProc 指向旧 DLL（已卸载），强制注销重新注册
//     否则 CreateWindowExW 调用过期的 WndProc 会导致静默失败（err=0 但返回 NULL）
// ====================================================================
static HMODULE GetTabBarModule()
{
    // 用 EXE 模块句柄（DLL 注入到 explorer.exe，GetModuleHandleW(NULL) 返回 exe）
    return GetModuleHandleW(NULL);
}

static void RegisterTabBarClass()
{
    if (s_classRegistered)
        return;

    HMODULE hMod = GetTabBarModule();

    // 检查类是否已注册，且 WndProc 是否指向当前 DLL
    WNDCLASSEXW wcInfo = { sizeof(wcInfo) };
    bool classExists = (GetClassInfoExW(hMod, kTabBarClass, &wcInfo) != 0);
    if (classExists)
    {
        if (wcInfo.lpfnWndProc == TabBarWindow::WndProc)
        {
            // WndProc 匹配，复用
            s_classRegistered = true;
            Utils::DebugTrace("RegisterTabBarClass: 类已存在且 WndProc 匹配，复用");
            return;
        }
        // WndProc 不匹配（来自已卸载的旧 DLL），注销后重新注册
        // UnregisterClass 要求该类无窗口存在；若有窗口，注销失败，但仍尝试
        char buf[160];
        sprintf_s(buf, sizeof(buf),
                  "RegisterTabBarClass: 检测到旧 WndProc=0x%p != 当前=0x%p，尝试注销",
                  wcInfo.lpfnWndProc, TabBarWindow::WndProc);
        Utils::DebugTrace(buf);
        UnregisterClassW(kTabBarClass, hMod);
        // 也尝试用 NULL hInstance 注销（兼容旧版本注册方式）
        UnregisterClassW(kTabBarClass, NULL);
    }

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = TabBarWindow::WndProc;
    wc.hInstance = hMod;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(kColorBg);
    wc.lpszClassName = kTabBarClass;

    SetLastError(0);
    ATOM atom = RegisterClassExW(&wc);
    if (atom == 0)
    {
        DWORD err = GetLastError();
        char buf2[256];
        sprintf_s(buf2, sizeof(buf2),
                  "RegisterTabBarClass 失败 atom=0 err=%lu", err);
        Utils::DebugTrace(buf2);
        // 可能注销失败（仍有窗口），只能复用旧类
        // 但旧 WndProc 可能已失效，CreateWindowEx 仍会失败
        // 这种情况只能等旧窗口销毁后再重新注入
    }
    else
    {
        s_classRegistered = true;
        Utils::DebugTrace("RegisterTabBarClass: 注册成功");
    }
}

// ====================================================================
// 注销标签栏窗口类（DLL 卸载时调用，避免残留失效的 WndProc 指针）
// ====================================================================
static void UnregisterTabBarClass()
{
    if (!s_classRegistered)
        return;
    HMODULE hMod = GetTabBarModule();
    UnregisterClassW(kTabBarClass, hMod);
    s_classRegistered = false;
    Utils::DebugTrace("UnregisterTabBarClass: 已注销");
}

// 公共接口：供 ExplorerHook::UnhookAllWindows 调用
void TabBarWindow::UnregisterWindowClass()
{
    UnregisterTabBarClass();
}

// ====================================================================
// 构造/析构
// ====================================================================
TabBarWindow::TabBarWindow()
    : m_hwnd(NULL)
    , m_activeIndex(0)
    , m_tabWidth(kDefaultTabWidth)
    , m_windowWidth(0)
    , m_windowHeight(0)
    , m_pBrowser(NULL)
    , m_hExplorer(NULL)
    , m_lastCheckTick(0)
    , m_shellTabRectValid(false)
    , m_lastShellTabHwnd(NULL)
{
    m_originalShellTabRect = { 0, 0, 0, 0 };
}

TabBarWindow::~TabBarWindow()
{
    Destroy();
}

// ====================================================================
// 创建标签栏窗口
// ====================================================================
bool TabBarWindow::Create(HWND hParent)
{
    RegisterTabBarClass();

    m_hExplorer = hParent;

    // 创建为 WS_CHILD 子窗口
    // hInstance 必须与注册时一致（用 EXE 模块句柄）
    HMODULE hMod = GetTabBarModule();

    // 第一次尝试创建
    SetLastError(0);
    m_hwnd = CreateWindowExW(
        WS_EX_NOPARENTNOTIFY,
        kTabBarClass, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
        0, 0, 100, 30,
        hParent, NULL, hMod, NULL);

    if (!m_hwnd)
    {
        DWORD err = GetLastError();
        // 诊断：检查窗口类状态
        WNDCLASSEXW wcInfo = { sizeof(wcInfo) };
        BOOL classFound = GetClassInfoExW(hMod, kTabBarClass, &wcInfo);

        char dbg[256];
        sprintf_s(dbg, sizeof(dbg),
                  "CreateWindowEx 第一次失败 err=%lu classFound=%d module=0x%p wndProc=0x%p curWndProc=0x%p",
                  err, classFound, hMod,
                  classFound ? wcInfo.lpfnWndProc : NULL,
                  TabBarWindow::WndProc);
        Utils::DebugTrace(dbg);

        // 强制重新注册类（清除 s_classRegistered 标志）
        // 这能处理 WndProc 指向旧 DLL 的情况
        s_classRegistered = false;
        // 先注销所有可能的旧注册
        UnregisterClassW(kTabBarClass, hMod);
        UnregisterClassW(kTabBarClass, NULL);
        RegisterTabBarClass();

        SetLastError(0);
        m_hwnd = CreateWindowExW(
            WS_EX_NOPARENTNOTIFY,
            kTabBarClass, L"",
            WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
            0, 0, 100, 30,
            hParent, NULL, hMod, NULL);

        if (!m_hwnd)
        {
            err = GetLastError();
            char dbg2[256];
            sprintf_s(dbg2, sizeof(dbg2),
                      "CreateWindowEx 第二次失败 err=%lu", err);
            Utils::DebugTrace(dbg2);
            Utils::Log(L"TabBarWindow::Create 失败: err=" + std::to_wstring(err));
            return false;
        }
    }

    // 存储 this 指针到窗口属性，供静态 WndProc 获取
    SetPropW(m_hwnd, kPropThisPtr, reinterpret_cast<HANDLE>(this));

    // 初始定位：找到 ShellTabWindowClass 并腾出空间
    UpdatePosition();

    // 设置定时器：每 200ms 触发一次 WM_TIMER
    // 用于：1) 主动修正 Explorer 重布局导致的位置错乱
    //       2) 检测导航变化更新当前标签
    // 关键：定时器在 TabBar 窗口所在线程（Explorer UI 线程）触发，
    // 可以安全调用 COM STA 对象（IWebBrowser2）
    SetTimer(m_hwnd, kTabBarTimerId, kCheckIntervalMs, NULL);

    // 尝试获取 IWebBrowser2 并创建初始标签
    // Explorer 可能尚未完全初始化，TryAcquireBrowser 内部会处理失败
    TryAcquireBrowser();

    // 加载收藏列表（从持久化文件）
    LoadFavorites();

    Utils::Log(L"TabBarWindow 创建成功");
    return true;
}

// ====================================================================
// 根据 Explorer 窗口布局定位 TabBar
// 方案：找到 ShellTabWindowClass（文件列表区），将其下移 30px，
//       TabBar 放在腾出的空间（地址栏下方、文件列表上方）
// ====================================================================
struct EnumShellTabData
{
    HWND hwndShellTab;
    HWND hwndRebar;  // 地址栏 ReBarWindow32
};

static BOOL CALLBACK FindShellTabProc(HWND hwnd, LPARAM lParam)
{
    wchar_t cls[256] = { 0 };
    if (GetClassNameW(hwnd, cls, 256) > 0)
    {
        if (wcscmp(cls, L"ShellTabWindowClass") == 0)
        {
            EnumShellTabData* pData = reinterpret_cast<EnumShellTabData*>(lParam);
            pData->hwndShellTab = hwnd;
            return FALSE;  // 找到了，停止枚举
        }
    }
    return TRUE;
}

void TabBarWindow::UpdatePosition()
{
    if (!m_hwnd || !m_hExplorer)
        return;

    // 找到 ShellTabWindowClass（文件列表区）
    EnumShellTabData data = { 0 };
    EnumChildWindows(m_hExplorer, FindShellTabProc, reinterpret_cast<LPARAM>(&data));

    int tabBarHeight = 30;
    m_windowHeight = tabBarHeight;

    if (data.hwndShellTab)
    {
        // 检测 ShellTab 窗口句柄是否变化（Explorer 可能重建子窗口）
        // 如果变化，重新记录原始 rect
        if (data.hwndShellTab != m_lastShellTabHwnd)
        {
            m_shellTabRectValid = false;
            m_lastShellTabHwnd = data.hwndShellTab;
        }

        // 获取 ShellTabWindowClass 当前位置（屏幕坐标）
        RECT rcTab;
        GetWindowRect(data.hwndShellTab, &rcTab);
        POINT pt = { rcTab.left, rcTab.top };
        ScreenToClient(m_hExplorer, &pt);
        int curWidth = rcTab.right - rcTab.left;
        int curHeight = rcTab.bottom - rcTab.top;
        int curTop = pt.y;  // 相对客户区的 top

        // 记录或验证原始 rect
        // 关键：不能每次都用"当前"高度减 tabBarHeight，否则高度会越来越小
        // 必须用"未被我们压缩的"原始高度计算
        if (!m_shellTabRectValid)
        {
            // 第一次记录，或 ShellTab 句柄变了，或 Explorer 重布局了
            // 判断当前 ShellTab 是否已被我们压缩：
            //   - 如果 curTop == originalTop + tabBarHeight 且 curHeight == originalHeight - tabBarHeight
            //     说明已被压缩，不能用当前值作为原始值
            //   - 简单处理：第一次记录时假设未被压缩（注入前已重启 explorer）
            m_originalShellTabRect.left = pt.x;
            m_originalShellTabRect.top = curTop;
            m_originalShellTabRect.right = curWidth;
            m_originalShellTabRect.bottom = curHeight;
            m_shellTabRectValid = true;

            char dbg[200];
            sprintf_s(dbg, sizeof(dbg),
                      "UpdatePosition: 记录原始 ShellTab rect: left=%ld top=%ld w=%ld h=%ld",
                      pt.x, curTop, curWidth, curHeight);
            Utils::DebugTrace(dbg);
        }
        else
        {
            // 检测 Explorer 是否重布局了 ShellTab（width 变了，或 top 不是预期的压缩位置）
            // 如果 Explorer 重布局，更新原始 rect
            int expectedTop = m_originalShellTabRect.top + tabBarHeight;
            int expectedHeight = m_originalShellTabRect.bottom - tabBarHeight;
            if (curWidth != m_originalShellTabRect.right ||
                (curTop != expectedTop && curTop != m_originalShellTabRect.top))
            {
                // Explorer 重布局了，重新记录
                // 注意：如果 curTop == expectedTop，说明是我们压缩后的位置，不更新
                // 如果 curTop == originalTop，说明 Explorer 恢复了原始位置，也不更新
                // 只有 curTop 是其他值，才说明 Explorer 重布局
                if (curTop != expectedTop && curTop != m_originalShellTabRect.top)
                {
                    m_originalShellTabRect.left = pt.x;
                    m_originalShellTabRect.top = curTop;
                    m_originalShellTabRect.right = curWidth;
                    m_originalShellTabRect.bottom = curHeight;

                    char dbg[200];
                    sprintf_s(dbg, sizeof(dbg),
                              "UpdatePosition: Explorer 重布局，更新原始 rect: top=%ld w=%ld h=%ld",
                              curTop, curWidth, curHeight);
                    Utils::DebugTrace(dbg);
                }
                else if (curWidth != m_originalShellTabRect.right)
                {
                    // 宽度变了（窗口大小变化），只更新宽度
                    m_originalShellTabRect.right = curWidth;
                }
            }
        }

        // 用原始 rect 计算应有位置
        int origLeft = m_originalShellTabRect.left;
        int origTop = m_originalShellTabRect.top;
        int origWidth = m_originalShellTabRect.right;
        int origHeight = m_originalShellTabRect.bottom;

        int newTop = origTop + tabBarHeight;
        int newHeight = origHeight - tabBarHeight;
        if (newHeight < 50) newHeight = 50;

        // 只有位置确实不对才 MoveWindow，避免反复重绘引起闪烁
        if (curTop != newTop || curWidth != origWidth || curHeight != newHeight)
        {
            MoveWindow(data.hwndShellTab, origLeft, newTop, origWidth, newHeight, FALSE);
        }

        // TabBar 放在 ShellTabWindowClass 原位置的顶部
        m_windowWidth = origWidth;

        // 检查 TabBar 当前位置是否需要更新
        RECT rcBar;
        GetWindowRect(m_hwnd, &rcBar);
        POINT ptBar = { rcBar.left, rcBar.top };
        ScreenToClient(m_hExplorer, &ptBar);
        if (ptBar.x != origLeft || ptBar.y != origTop ||
            (rcBar.right - rcBar.left) != origWidth ||
            (rcBar.bottom - rcBar.top) != tabBarHeight)
        {
            MoveWindow(m_hwnd, origLeft, origTop, origWidth, tabBarHeight, FALSE);
        }

        // 保持 TabBar 在 Z-order 顶部
        SetWindowPos(m_hwnd, HWND_TOP, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
    else
    {
        // 未找到 ShellTabWindowClass，回退到客户区顶部
        RECT rcClient;
        GetClientRect(m_hExplorer, &rcClient);
        m_windowWidth = rcClient.right;
        MoveWindow(m_hwnd, 0, 0, rcClient.right, tabBarHeight, FALSE);
    }

    UpdateTabRects();
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// ====================================================================
// 尝试获取 IWebBrowser2
// 成功后创建初始标签（当前文件夹）
// ====================================================================
bool TabBarWindow::TryAcquireBrowser()
{
    if (m_pBrowser)
        return true; // 已获取

    if (!m_hExplorer)
        return false;

    // 初始化 COM（STA）
    ExplorerInterface::InitializeCom();

    m_pBrowser = ExplorerInterface::FindWebBrowserByHwnd(m_hExplorer);
    if (!m_pBrowser)
        return false;

    Utils::Log(L"已获取 IWebBrowser2");

    // 如果还没有标签，创建初始标签
    if (m_tabs.empty())
    {
        LPITEMIDLIST pidl = ExplorerInterface::GetCurrentPidl(m_pBrowser);
        std::wstring name = ExplorerInterface::GetCurrentFolderName(m_pBrowser);
        if (name.empty())
            name = L"资源管理器";
        if (!pidl)
        {
            // 回退：使用"此电脑"作为初始标签
            pidl = ExplorerInterface::CopyPidl(NULL);
        }
        AddTab(pidl, name);
        if (pidl)
            ILFree(pidl); // AddTab 内部已深拷贝
    }
    return true;
}

// ====================================================================
// 销毁标签栏窗口
// ====================================================================
void TabBarWindow::Destroy()
{
    if (m_pBrowser)
    {
        m_pBrowser->Release();
        m_pBrowser = NULL;
    }
    FreeAllPidls();
    FreeFavorites();
    if (m_hwnd && IsWindow(m_hwnd))
    {
        KillTimer(m_hwnd, kTabBarTimerId);
        RemovePropW(m_hwnd, kPropThisPtr);
        DestroyWindow(m_hwnd);
    }
    m_hwnd = NULL;
}

// ====================================================================
// 释放所有标签的 PIDL
// ====================================================================
void TabBarWindow::FreeAllPidls()
{
    for (auto& tab : m_tabs)
    {
        if (tab.pidl)
        {
            ILFree(tab.pidl);
            tab.pidl = NULL;
        }
    }
    m_tabs.clear();
}

// ====================================================================
// 调整大小
// ====================================================================
void TabBarWindow::Resize(int width, int height)
{
    // 顶层窗口方案：忽略传入的宽高，直接根据 Explorer 窗口位置定位
    // 保持 tabBarHeight 为 30（由 m_windowHeight 记录）
    if (m_windowHeight <= 0)
        m_windowHeight = 30;
    UpdatePosition();
}

// ====================================================================
// 新增标签页
// pidl: 该标签的文件夹 PIDL（会被深拷贝）
// ====================================================================
void TabBarWindow::AddTab(LPCITEMIDLIST pidl, const std::wstring& title)
{
    TabItem tab;
    tab.title = title;
    tab.pidl = ExplorerInterface::CopyPidl(pidl); // 深拷贝
    tab.active = false;
    tab.rect = { 0, 0, 0, 0 };
    m_tabs.push_back(tab);

    Utils::Log(L"AddTab: 新增标签 '" + title + L"'，当前标签数=" + std::to_wstring(m_tabs.size()));

    ActivateTab(static_cast<int>(m_tabs.size()) - 1);
    UpdateTabRects();
    InvalidateRect(m_hwnd, NULL, TRUE);
}

// ====================================================================
// 关闭指定索引的标签页
// ====================================================================
void TabBarWindow::CloseTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size()))
        return;

    Utils::Log(L"CloseTab: 关闭标签 " + std::to_wstring(index) +
               L" '" + m_tabs[index].title + L"'");

    // 释放该标签的 PIDL
    if (m_tabs[index].pidl)
    {
        ILFree(m_tabs[index].pidl);
    }
    m_tabs.erase(m_tabs.begin() + index);

    // 如果没有标签了，关闭 Explorer 窗口
    if (m_tabs.empty())
    {
        if (m_hExplorer)
        {
            PostMessageW(m_hExplorer, WM_CLOSE, 0, 0);
        }
        return;
    }

    // 调整激活索引
    if (m_activeIndex >= static_cast<int>(m_tabs.size()))
    {
        m_activeIndex = static_cast<int>(m_tabs.size()) - 1;
    }

    // 激活相邻标签并导航
    ActivateTab(m_activeIndex);
    UpdateTabRects();
    InvalidateRect(m_hwnd, NULL, TRUE);
}

// ====================================================================
// 激活指定索引的标签页
// 如果该标签的 PIDL 与当前 Explorer 显示的不同，则导航
// ====================================================================
void TabBarWindow::ActivateTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size()))
        return;

    Utils::Log(L"ActivateTab: 激活标签 " + std::to_wstring(index) +
               L" '" + m_tabs[index].title + L"'");

    // 取消其他标签的激活状态
    for (auto& tab : m_tabs)
    {
        tab.active = false;
    }
    m_tabs[index].active = true;
    m_activeIndex = index;

    // 导航到该标签的 PIDL
    if (m_pBrowser && m_tabs[index].pidl)
    {
        ExplorerInterface::NavigateToPidl(m_pBrowser, m_tabs[index].pidl);
    }

    InvalidateRect(m_hwnd, NULL, TRUE);
}

// ====================================================================
// 更新各标签的矩形区域
// ====================================================================
void TabBarWindow::UpdateTabRects()
{
    int x = 0;
    for (auto& tab : m_tabs)
    {
        tab.rect.left = x;
        tab.rect.top = 0;
        tab.rect.right = x + m_tabWidth;
        tab.rect.bottom = m_windowHeight;
        x += m_tabWidth;
    }
}

// ====================================================================
// 命中测试
// ====================================================================
int TabBarWindow::HitTest(int x, int y, int* outTabIndex)
{
    if (outTabIndex) *outTabIndex = -1;

    // + 按钮起始 x（最后一个标签右侧）
    int plusX = static_cast<int>(m_tabs.size()) * m_tabWidth;

    // 检查收藏按钮区域（+ 按钮右侧）
    int favX = plusX + kPlusButtonWidth;
    if (x >= favX && x <= favX + kFavoriteButtonWidth)
    {
        return HIT_FAVORITE;
    }

    // 检查 + 按钮区域
    if (x >= plusX && x <= plusX + kPlusButtonWidth)
    {
        return HIT_PLUS;
    }

    // 检查各标签
    for (int i = 0; i < static_cast<int>(m_tabs.size()); i++)
    {
        const RECT& r = m_tabs[i].rect;
        if (x >= r.left && x < r.right)
        {
            // 检查是否点击了关闭按钮（标签右侧区域）
            int closeLeft = r.right - kCloseButtonWidth;
            if (x >= closeLeft && x <= r.right)
            {
                if (outTabIndex) *outTabIndex = i;
                return HIT_CLOSE;
            }
            // 点击标签主体
            return i;
        }
    }

    return HIT_NONE;
}

// ====================================================================
// 窗口过程（静态）
// ====================================================================
LRESULT CALLBACK TabBarWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    TabBarWindow* pThis = reinterpret_cast<TabBarWindow*>(
        GetPropW(hwnd, kPropThisPtr));

    if (pThis)
    {
        return pThis->HandleMessage(msg, wParam, lParam);
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ====================================================================
// 处理窗口消息
// ====================================================================
LRESULT TabBarWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
        OnPaint();
        return 0;

    case WM_LBUTTONDOWN:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        int tabIndex = -1;
        int hit = HitTest(x, y, &tabIndex);

        switch (hit)
        {
        case HIT_PLUS:
        {
            // + 按钮：新建标签
            // 1) 若已有标签：复制当前标签 PIDL 新建
            // 2) 若无标签：尝试获取 IWebBrowser2，用当前 Explorer 路径新建
            if (!m_tabs.empty() &&
                m_activeIndex >= 0 &&
                m_activeIndex < static_cast<int>(m_tabs.size()))
            {
                LPCITEMIDLIST curPidl = m_tabs[m_activeIndex].pidl;
                std::wstring curTitle = m_tabs[m_activeIndex].title;
                AddTab(curPidl, curTitle);
            }
            else
            {
                // 尝试获取 browser 并新建初始标签
                if (!m_pBrowser)
                {
                    TryAcquireBrowser();
                }
                if (m_pBrowser)
                {
                    LPITEMIDLIST pidl = ExplorerInterface::GetCurrentPidl(m_pBrowser);
                    std::wstring name = ExplorerInterface::GetCurrentFolderName(m_pBrowser);
                    if (name.empty())
                        name = L"资源管理器";
                    if (!pidl)
                        pidl = ExplorerInterface::CopyPidl(NULL);
                    AddTab(pidl, name);
                    if (pidl) ILFree(pidl);
                }
                else
                {
                    // browser 不可用：用"资源管理器"占位
                    AddTab(NULL, L"资源管理器");
                }
            }
            break;
        }

        case HIT_FAVORITE:
            // ☆ 收藏按钮：弹出收藏菜单
            ShowFavoritesMenu();
            break;

        case HIT_CLOSE:
            // × 按钮：关闭对应标签
            CloseTab(tabIndex);
            break;

        default:
            if (hit >= 0)
            {
                // 点击标签主体：激活并导航
                if (hit != m_activeIndex)
                {
                    ActivateTab(hit);
                }
            }
            break;
        }
        return 0;
    }

    case WM_RBUTTONDOWN:
    {
        // 右键标签：弹出上下文菜单
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        int tabIndex = -1;
        int hit = HitTest(x, y, &tabIndex);

        if (hit >= 0)
        {
            // 右键标签主体：弹出标签菜单
            POINT pt = { x, y };
            ClientToScreen(m_hwnd, &pt);
            ShowTabContextMenu(hit, pt.x, pt.y);
        }
        else if (hit == HIT_FAVORITE)
        {
            // 右键收藏按钮：直接弹出收藏菜单（同左键）
            ShowFavoritesMenu();
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return TRUE;

    case WM_TIMER:
        if (wParam == kTabBarTimerId)
        {
            OnTimerTick();
            return 0;
        }
        break;
    }

    return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}

// ====================================================================
// 定时器回调：
// 1) 主动修正 ShellTabWindowClass 和 TabBar 的位置（防御 Explorer 重布局）
//    这一步是关键：Explorer 在 hover、ribbon 调整、地址栏变化等时机会重新
//    布局 ShellTabWindowClass，把它移回原位置覆盖 TabBar。我们必须主动监测
//    并立即修正，否则 TabBar 会被遮挡"消失"。
// 2) 检查导航变化并更新当前标签的 PIDL/标题
// ====================================================================
void TabBarWindow::OnTimerTick()
{
    // 第一步：无条件修正位置（每 200ms 触发，开销很小）
    // UpdatePosition 内部会判断位置是否真的不对，不对才 MoveWindow
    UpdatePosition();

    // 第二步：节流后的导航变化检查
    DWORD now = GetTickCount();
    if (now - m_lastCheckTick < kCheckIntervalMs)
        return;
    m_lastCheckTick = now;

    // 确保 browser 已获取
    if (!m_pBrowser)
    {
        TryAcquireBrowser();
        return;
    }

    // 如果还没标签，尝试创建初始标签
    if (m_tabs.empty())
    {
        TryAcquireBrowser();
        return;
    }

    if (m_activeIndex < 0 || m_activeIndex >= static_cast<int>(m_tabs.size()))
        return;

    // 获取当前 Explorer 显示的路径
    LPITEMIDLIST curPidl = ExplorerInterface::GetCurrentPidl(m_pBrowser);
    if (!curPidl)
        return;

    // 比较当前标签 PIDL 与 Explorer 实际 PIDL
    LPCITEMIDLIST tabPidl = m_tabs[m_activeIndex].pidl;
    bool changed = false;
    if (!tabPidl)
    {
        changed = true;
    }
    else
    {
        changed = !ILIsEqual(tabPidl, curPidl);
    }

    if (changed)
    {
        // 更新当前标签的 PIDL
        if (tabPidl)
            ILFree(const_cast<LPITEMIDLIST>(tabPidl));
        m_tabs[m_activeIndex].pidl = ExplorerInterface::CopyPidl(curPidl);

        // 更新标题
        std::wstring newName = ExplorerInterface::GetCurrentFolderName(m_pBrowser);
        if (!newName.empty())
        {
            m_tabs[m_activeIndex].title = newName;
        }
        InvalidateRect(m_hwnd, NULL, FALSE);
    }

    ILFree(curPidl);
}

// ====================================================================
// 绘制标签栏（暗色主题）
// ====================================================================
void TabBarWindow::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    HDC memDc = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDc, memBitmap);

    // 绘制背景
    HBRUSH bgBrush = CreateSolidBrush(kColorBg);
    FillRect(memDc, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    SetBkMode(memDc, TRANSPARENT);

    HFONT hFont = CreateFontW(
        14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(memDc, hFont);

    for (int i = 0; i < static_cast<int>(m_tabs.size()); i++)
    {
        const TabItem& tab = m_tabs[i];
        RECT r = tab.rect;

        // 标签背景
        COLORREF tabColor = tab.active ? kColorTabActive : kColorTabInactive;
        HBRUSH tabBrush = CreateSolidBrush(tabColor);
        RECT fillRect = { r.left, r.top, r.right, r.bottom };
        FillRect(memDc, &fillRect, tabBrush);
        DeleteObject(tabBrush);

        // 标签右侧分隔线
        HPEN sepPen = CreatePen(PS_SOLID, 1, kColorSeparator);
        HPEN oldPen = (HPEN)SelectObject(memDc, sepPen);
        MoveToEx(memDc, r.right - 1, r.top, NULL);
        LineTo(memDc, r.right - 1, r.bottom);
        SelectObject(memDc, oldPen);
        DeleteObject(sepPen);

        // 绘制标签文字（截断到关闭按钮左侧）
        SetTextColor(memDc, kColorText);
        RECT textRect = { r.left + kTextPadding, r.top,
                          r.right - kCloseButtonWidth, r.bottom };
        DrawTextW(memDc, tab.title.c_str(), -1, &textRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        // 绘制关闭按钮 ×
        int closeSize = 10;
        int closeX = r.right - kCloseButtonWidth / 2;
        int closeY = (r.top + r.bottom) / 2;
        COLORREF closeColor = tab.active ? kColorCloseActive : kColorClose;
        HPEN closePen = CreatePen(PS_SOLID, 1, closeColor);
        oldPen = (HPEN)SelectObject(memDc, closePen);
        MoveToEx(memDc, closeX - closeSize / 2, closeY - closeSize / 2, NULL);
        LineTo(memDc, closeX + closeSize / 2, closeY + closeSize / 2);
        MoveToEx(memDc, closeX + closeSize / 2, closeY - closeSize / 2, NULL);
        LineTo(memDc, closeX - closeSize / 2, closeY + closeSize / 2);
        SelectObject(memDc, oldPen);
        DeleteObject(closePen);
    }

    // 绘制 + 按钮
    int plusX = static_cast<int>(m_tabs.size()) * m_tabWidth + kPlusButtonWidth / 2;
    int plusY = height / 2;
    int plusSize = 12;
    HPEN plusPen = CreatePen(PS_SOLID, 2, kColorPlus);
    HPEN oldPen = (HPEN)SelectObject(memDc, plusPen);
    MoveToEx(memDc, plusX - plusSize / 2, plusY, NULL);
    LineTo(memDc, plusX + plusSize / 2, plusY);
    MoveToEx(memDc, plusX, plusY - plusSize / 2, NULL);
    LineTo(memDc, plusX, plusY + plusSize / 2);
    SelectObject(memDc, oldPen);
    DeleteObject(plusPen);

    // 绘制 ☆ 收藏按钮（+ 按钮右侧）
    // 使用 5 角星图案，空心表示无收藏，实心表示有收藏
    int favCx = plusX + kPlusButtonWidth / 2 + kFavoriteButtonWidth / 2;
    int favCy = height / 2;
    int starR = 7;  // 星形外接圆半径
    COLORREF favColor = m_favorites.empty() ? kColorPlus : kColorCloseActive;
    HPEN favPen = CreatePen(PS_SOLID, 1, favColor);
    oldPen = (HPEN)SelectObject(memDc, favPen);
    HBRUSH favBrush = CreateSolidBrush(
        m_favorites.empty() ? kColorBg : favColor);
    HBRUSH oldBrush = (HBRUSH)SelectObject(memDc, favBrush);

    // 绘制 5 角星：5 个外顶点 + 5 个内顶点，交替连接
    POINT starPts[10];
    for (int i = 0; i < 10; i++)
    {
        double angle = -3.14159265358979 / 2.0 + i * 3.14159265358979 / 5.0;
        int r = (i % 2 == 0) ? starR : starR * 2 / 5;  // 外顶点半径 vs 内顶点半径
        starPts[i].x = favCx + static_cast<int>(r * cos(angle));
        starPts[i].y = favCy + static_cast<int>(r * sin(angle));
    }
    Polygon(memDc, starPts, 10);

    SelectObject(memDc, oldBrush);
    DeleteObject(favBrush);
    SelectObject(memDc, oldPen);
    DeleteObject(favPen);

    SelectObject(memDc, oldFont);
    DeleteObject(hFont);

    BitBlt(hdc, 0, 0, width, height, memDc, 0, 0, SRCCOPY);

    SelectObject(memDc, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDc);

    EndPaint(m_hwnd, &ps);
}

// ====================================================================
// 收藏功能实现
// ====================================================================

// 获取收藏文件路径：%LOCALAPPDATA%\XPTabCpp\favorites.dat
static std::wstring GetFavoritesFilePath()
{
    wchar_t buf[MAX_PATH] = { 0 };
    DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, MAX_PATH);
    std::wstring path;
    if (len > 0 && len < MAX_PATH)
    {
        path = buf;
        path += L"\\XPTabCpp";
        CreateDirectoryW(path.c_str(), NULL);
        path += L"\\favorites.dat";
    }
    else
    {
        // 回退到 DLL 所在目录
        wchar_t dllPath[MAX_PATH] = { 0 };
        HMODULE hMod = Utils::GetThisModule();
        if (hMod && GetModuleFileNameW(hMod, dllPath, MAX_PATH) > 0)
        {
            path = dllPath;
            size_t pos = path.find_last_of(L"\\/");
            if (pos != std::wstring::npos)
                path = path.substr(0, pos + 1);
            path += L"favorites.dat";
        }
        else
        {
            path = L"favorites.dat";
        }
    }
    return path;
}

// 释放 m_favorites 中所有 PIDL
void TabBarWindow::FreeFavorites()
{
    for (auto& fav : m_favorites)
    {
        if (fav.pidl)
        {
            ILFree(fav.pidl);
            fav.pidl = NULL;
        }
    }
    m_favorites.clear();
}

// 加载收藏列表
// 文件格式（二进制）：
//   [DWORD count]
//   重复 count 次：
//     [DWORD titleLen（含 \0 的 wchar 数）][wchar[] title]
//     [DWORD pathLen（含 \0 的 wchar 数）][wchar[] path]
// 使用 path 而非直接序列化 PIDL，因为 ILSaveToStream/ILLoadFromStream 已弃用
// 加载时用 SHParseDisplayName 从 path 还原 PIDL
void TabBarWindow::LoadFavorites()
{
    FreeFavorites();

    std::wstring path = GetFavoritesFilePath();
    FILE* fp = NULL;
    if (_wfopen_s(&fp, path.c_str(), L"rb") != 0 || !fp)
    {
        // 文件不存在是正常情况（首次使用）
        return;
    }

    DWORD count = 0;
    if (fread(&count, sizeof(count), 1, fp) != 1)
    {
        fclose(fp);
        return;
    }

    for (DWORD i = 0; i < count; i++)
    {
        // 读取标题
        DWORD titleLen = 0;
        if (fread(&titleLen, sizeof(titleLen), 1, fp) != 1 || titleLen == 0 || titleLen > 1024)
            break;

        std::wstring title(titleLen, L'\0');
        if (fread(&title[0], sizeof(wchar_t), titleLen, fp) != titleLen)
            break;
        size_t nul = title.find(L'\0');
        if (nul != std::wstring::npos)
            title.resize(nul);

        // 读取路径
        DWORD pathLen = 0;
        if (fread(&pathLen, sizeof(pathLen), 1, fp) != 1 || pathLen == 0 || pathLen > 32768)
            break;

        std::wstring favPath(pathLen, L'\0');
        if (fread(&favPath[0], sizeof(wchar_t), pathLen, fp) != pathLen)
            break;
        size_t nul2 = favPath.find(L'\0');
        if (nul2 != std::wstring::npos)
            favPath.resize(nul2);

        // 从路径还原 PIDL
        LPITEMIDLIST pidl = NULL;
        HRESULT hr = SHParseDisplayName(favPath.c_str(), NULL, &pidl, 0, NULL);
        if (FAILED(hr) || !pidl)
        {
            // 路径无效（可能是特殊位置如"此电脑"），跳过
            Utils::Log(L"收藏路径无效，跳过：" + favPath);
            continue;
        }

        FavoriteItem fav;
        fav.title = title;
        fav.pidl = pidl;
        m_favorites.push_back(fav);
    }

    fclose(fp);
    Utils::Log(L"已加载收藏列表：" + std::to_wstring(m_favorites.size()) + L" 项");
}

// 保存收藏列表
void TabBarWindow::SaveFavorites()
{
    std::wstring path = GetFavoritesFilePath();
    FILE* fp = NULL;
    if (_wfopen_s(&fp, path.c_str(), L"wb") != 0 || !fp)
    {
        Utils::Log(L"保存收藏失败：无法创建文件 " + path);
        return;
    }

    DWORD count = static_cast<DWORD>(m_favorites.size());
    fwrite(&count, sizeof(count), 1, fp);

    for (DWORD i = 0; i < count; i++)
    {
        const FavoriteItem& fav = m_favorites[i];

        // 写入标题（含 \0）
        std::wstring title = fav.title;
        if (title.empty())
            title = L"(无标题)";
        DWORD titleLen = static_cast<DWORD>(title.length() + 1);
        fwrite(&titleLen, sizeof(titleLen), 1, fp);
        fwrite(title.c_str(), sizeof(wchar_t), titleLen, fp);

        // 从 PIDL 获取路径并写入
        wchar_t pathBuf[MAX_PATH] = { 0 };
        BOOL gotPath = FALSE;
        if (fav.pidl)
        {
            gotPath = SHGetPathFromIDListW(fav.pidl, pathBuf);
        }
        std::wstring favPath;
        if (gotPath)
        {
            favPath = pathBuf;
        }
        else
        {
            // 特殊位置（如此电脑、控制面板）：用显示名作为标识
            // 注意：这类收藏无法在重启后还原 PIDL，会被跳过
            favPath = L"";
        }
        DWORD pathLen = static_cast<DWORD>(favPath.length() + 1);
        fwrite(&pathLen, sizeof(pathLen), 1, fp);
        fwrite(favPath.c_str(), sizeof(wchar_t), pathLen, fp);
    }

    fclose(fp);
    Utils::Log(L"已保存收藏列表：" + std::to_wstring(count) + L" 项");
}

// 添加当前激活标签到收藏
void TabBarWindow::AddCurrentTabToFavorites()
{
    if (m_activeIndex < 0 || m_activeIndex >= static_cast<int>(m_tabs.size()))
    {
        Utils::Log(L"添加收藏失败：无激活标签");
        return;
    }

    const TabItem& tab = m_tabs[m_activeIndex];
    if (!tab.pidl)
    {
        Utils::Log(L"添加收藏失败：标签无 PIDL");
        return;
    }

    // 检查是否已存在相同 PIDL 的收藏（去重）
    for (const auto& fav : m_favorites)
    {
        if (fav.pidl && ILIsEqual(fav.pidl, tab.pidl))
        {
            Utils::Log(L"收藏已存在：" + tab.title);
            return;
        }
    }

    FavoriteItem fav;
    fav.title = tab.title;
    fav.pidl = ExplorerInterface::CopyPidl(tab.pidl);
    m_favorites.push_back(fav);

    SaveFavorites();
    Utils::Log(L"已添加收藏：" + tab.title);
}

// 删除指定索引的收藏项
void TabBarWindow::RemoveFavorite(int index)
{
    if (index < 0 || index >= static_cast<int>(m_favorites.size()))
        return;

    std::wstring title = m_favorites[index].title;
    if (m_favorites[index].pidl)
        ILFree(m_favorites[index].pidl);
    m_favorites.erase(m_favorites.begin() + index);

    SaveFavorites();
    Utils::Log(L"已删除收藏：" + title);
}

// 弹出收藏菜单（点击 ☆ 按钮时）
void TabBarWindow::ShowFavoritesMenu()
{
    // 在收藏按钮位置弹出菜单
    HMENU hMenu = CreatePopupMenu();

    if (m_favorites.empty())
    {
        AppendMenuW(hMenu, MF_STRING | MF_DISABLED, 0, L"(无收藏)");
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, 1001, L"添加当前文件夹到收藏");
    }
    else
    {
        // 收藏列表（左键点击 = 在新标签打开）
        for (int i = 0; i < static_cast<int>(m_favorites.size()); i++)
        {
            const FavoriteItem& fav = m_favorites[i];
            std::wstring text = fav.title;
            if (text.empty())
                text = L"(无标题)";
            wchar_t buf[300];
            swprintf_s(buf, 300, L"%d. %s", i + 1, text.c_str());
            AppendMenuW(hMenu, MF_STRING, 2000 + i, buf);
        }
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

        // 添加当前文件夹到收藏
        AppendMenuW(hMenu, MF_STRING, 1001, L"添加当前文件夹到收藏");

        // 删除收藏子菜单
        HMENU hDelMenu = CreatePopupMenu();
        for (int i = 0; i < static_cast<int>(m_favorites.size()); i++)
        {
            const FavoriteItem& fav = m_favorites[i];
            std::wstring text = fav.title;
            if (text.empty())
                text = L"(无标题)";
            wchar_t buf[300];
            swprintf_s(buf, 300, L"%s", text.c_str());
            AppendMenuW(hDelMenu, MF_STRING, 5000 + i, buf);
        }
        AppendMenuW(hMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(hDelMenu), L"删除收藏...");
    }

    // 获取收藏按钮屏幕坐标
    int plusX = static_cast<int>(m_tabs.size()) * m_tabWidth;
    int favX = plusX + kPlusButtonWidth;
    POINT pt = { favX, m_windowHeight };
    ClientToScreen(m_hwnd, &pt);

    SetForegroundWindow(m_hwnd);

    int cmd = static_cast<int>(TrackPopupMenu(
        hMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, m_hwnd, NULL));

    DestroyMenu(hMenu);

    if (cmd == 0)
    {
        return; // 用户取消
    }

    if (cmd == 1001)
    {
        AddCurrentTabToFavorites();
        return;
    }

    if (cmd >= 5000 && cmd < 6000)
    {
        // 删除收藏项
        int favIndex = cmd - 5000;
        RemoveFavorite(favIndex);
        return;
    }

    if (cmd >= 2000 && cmd < 3000)
    {
        // 选择某个收藏项：新建标签并导航
        int favIndex = cmd - 2000;
        if (favIndex >= 0 && favIndex < static_cast<int>(m_favorites.size()))
        {
            const FavoriteItem& fav = m_favorites[favIndex];
            AddTab(fav.pidl, fav.title.empty() ? L"收藏" : fav.title);
        }
    }
}

// 弹出标签右键菜单
void TabBarWindow::ShowTabContextMenu(int tabIndex, int screenX, int screenY)
{
    if (tabIndex < 0 || tabIndex >= static_cast<int>(m_tabs.size()))
        return;

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 3001, L"添加到收藏");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, 3002, L"关闭标签");

    SetForegroundWindow(m_hwnd);
    int cmd = static_cast<int>(TrackPopupMenu(
        hMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_RIGHTBUTTON,
        screenX, screenY, 0, m_hwnd, NULL));
    DestroyMenu(hMenu);

    if (cmd == 3001)
    {
        // 添加到收藏：临时激活该标签再添加
        int oldActive = m_activeIndex;
        if (tabIndex != m_activeIndex)
        {
            ActivateTab(tabIndex);
        }
        AddCurrentTabToFavorites();
        // 不恢复原激活标签（用户可能想看新收藏的文件夹）
    }
    else if (cmd == 3002)
    {
        CloseTab(tabIndex);
    }
}

// 弹出收藏项右键菜单（在收藏菜单中右键某项）
// 注意：由于 TrackPopupMenu 是模态的，无法在收藏菜单中直接右键单项
// 改为：在收藏菜单中提供"删除收藏..."选项，弹出二级菜单选择
// 这里实现一个独立的"删除收藏"菜单
bool TabBarWindow::ShowFavoriteContextItem(int favoriteIndex, int screenX, int screenY)
{
    if (favoriteIndex < 0 || favoriteIndex >= static_cast<int>(m_favorites.size()))
        return false;

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 4001, L"删除此收藏");
    AppendMenuW(hMenu, MF_STRING, 4002, L"在新标签打开");

    SetForegroundWindow(m_hwnd);
    int cmd = static_cast<int>(TrackPopupMenu(
        hMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD,
        screenX, screenY, 0, m_hwnd, NULL));
    DestroyMenu(hMenu);

    if (cmd == 4001)
    {
        RemoveFavorite(favoriteIndex);
        return true;
    }
    else if (cmd == 4002)
    {
        const FavoriteItem& fav = m_favorites[favoriteIndex];
        AddTab(fav.pidl, fav.title.empty() ? L"收藏" : fav.title);
    }
    return false;
}
