// TabBarUI.cpp - 标签栏 UI 实现（DeskBand 版本）
//
// 在 DeskBand 窗口上绘制标签栏，处理点击事件
// 与旧版的区别：不再移动 ShellTabWindowClass，避免重影问题

#include "stdafx.h"
#include "TabBarUI.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <windowsx.h>
#include <cstdio>

// ====================================================================
// 静态成员初始化
// ====================================================================
const COLORREF TabBarUI::kColorBg         = RGB(30, 30, 30);
const COLORREF TabBarUI::kColorTabActive  = RGB(60, 60, 60);
const COLORREF TabBarUI::kColorTabInactive= RGB(40, 40, 40);
const COLORREF TabBarUI::kColorTabHover   = RGB(50, 50, 50);
const COLORREF TabBarUI::kColorText       = RGB(220, 220, 220);
const COLORREF TabBarUI::kColorClose      = RGB(200, 200, 200);
const COLORREF TabBarUI::kColorCloseActive= RGB(255, 100, 100);
const COLORREF TabBarUI::kColorPlus       = RGB(220, 220, 220);
const COLORREF TabBarUI::kColorSeparator  = RGB(70, 70, 70);

// ====================================================================
// 日志
// ====================================================================
static void Log(const wchar_t* msg)
{
    wchar_t logPath[MAX_PATH] = { 0 };
    GetTempPathW(MAX_PATH, logPath);
    wcscat_s(logPath, MAX_PATH, L"XPTabBand_log.txt");

    FILE* f = nullptr;
    if (_wfopen_s(&f, logPath, L"a") == 0 && f)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fwprintf(f, L"[%02d:%02d:%02d.%03d] %s\n",
                 st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, msg);
        fclose(f);
    }
}

static void LogFmt(const wchar_t* fmt, ...)
{
    wchar_t buf[512];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
    va_end(args);
    Log(buf);
}

// ====================================================================
// 构造/析构
// ====================================================================
TabBarUI::TabBarUI()
    : m_hwnd(NULL)
    , m_pBrowser(NULL)
    , m_activeIndex(0)
    , m_tabWidth(kDefaultTabWidth)
    , m_windowWidth(0)
    , m_windowHeight(kTabBarHeight)
    , m_lastCheckTick(0)
    , m_hoverTab(-1)
    , m_hoverButton(HIT_NONE)
{
}

TabBarUI::~TabBarUI()
{
    Uninitialize();
}

bool TabBarUI::Initialize(HWND hwndBand, IWebBrowser2* pBrowser)
{
    m_hwnd = hwndBand;
    m_pBrowser = pBrowser;  // 不 AddRef，由 DeskBand 拥有
    m_windowHeight = kTabBarHeight;

    // 设置定时器
    SetTimer(m_hwnd, kTabBarTimerId, kCheckIntervalMs, NULL);

    Log(L"TabBarUI 初始化");
    return true;
}

void TabBarUI::Uninitialize()
{
    if (m_hwnd)
    {
        KillTimer(m_hwnd, kTabBarTimerId);
    }
    FreeAllPidls();
    m_pBrowser = NULL;
    m_hwnd = NULL;
}

void TabBarUI::OnSize(int width, int height)
{
    m_windowWidth = width;
    m_windowHeight = height;
    UpdateTabRects();
    InvalidateRect(m_hwnd, NULL, TRUE);
}

// ====================================================================
// PIDL 辅助函数
// ====================================================================
LPITEMIDLIST TabBarUI::CopyPidl(LPCITEMIDLIST pidl)
{
    if (!pidl) return NULL;
    return ILClone(pidl);
}

std::wstring TabBarUI::GetNameFromPidl(LPCITEMIDLIST pidl)
{
    if (!pidl) return L"";

    SHFILEINFOW sfi = { 0 };
    if (SHGetFileInfoW(reinterpret_cast<LPCWSTR>(pidl), 0, &sfi, sizeof(sfi),
                       SHGFI_PIDL | SHGFI_DISPLAYNAME))
    {
        return sfi.szDisplayName;
    }
    return L"";
}

LPITEMIDLIST TabBarUI::GetSpecialFolderPidl(int csidl)
{
    LPITEMIDLIST pidl = NULL;
    HRESULT hr = SHGetFolderLocation(NULL, csidl, NULL, 0, &pidl);
    if (FAILED(hr))
    {
        hr = SHGetSpecialFolderLocation(NULL, csidl, &pidl);
    }
    return pidl;
}

LPITEMIDLIST TabBarUI::GetCurrentPidlEx(IWebBrowser2* pBrowser)
{
    if (!pBrowser) return NULL;

    // 通过 IServiceProvider -> IShellBrowser -> IShellView -> IFolderView -> IPersistIDList
    IServiceProvider* pSvc = NULL;
    if (FAILED(pBrowser->QueryInterface(IID_PPV_ARGS(&pSvc))) || !pSvc)
        return NULL;

    IShellBrowser* pShellBrowser = NULL;
    HRESULT hr = pSvc->QueryService(SID_SShellBrowser, IID_PPV_ARGS(&pShellBrowser));
    pSvc->Release();
    if (FAILED(hr) || !pShellBrowser)
        return NULL;

    IShellView* pView = NULL;
    hr = pShellBrowser->QueryActiveShellView(&pView);
    pShellBrowser->Release();
    if (FAILED(hr) || !pView)
        return NULL;

    IFolderView* pFolderView = NULL;
    hr = pView->QueryInterface(IID_PPV_ARGS(&pFolderView));
    pView->Release();
    if (FAILED(hr) || !pFolderView)
        return NULL;

    LPITEMIDLIST pidl = NULL;
    IPersistIDList* pPersist = NULL;
    hr = pFolderView->GetFolder(IID_PPV_ARGS(&pPersist));
    pFolderView->Release();
    if (FAILED(hr) || !pPersist)
        return NULL;

    hr = pPersist->GetIDList(&pidl);
    pPersist->Release();
    if (FAILED(hr))
        return NULL;

    return pidl;
}

bool TabBarUI::NavigateToPidl(IWebBrowser2* pBrowser, LPCITEMIDLIST pidl)
{
    if (!pBrowser || !pidl) return false;

    int cbPidl = ILGetSize(pidl);
    if (cbPidl <= 0) return false;

    SAFEARRAY* psa = SafeArrayCreateVector(VT_UI1, 0, cbPidl);
    if (!psa) return false;

    void* pData = NULL;
    if (FAILED(SafeArrayAccessData(psa, &pData)))
    {
        SafeArrayDestroy(psa);
        return false;
    }
    memcpy(pData, pidl, cbPidl);
    SafeArrayUnaccessData(psa);

    VARIANT vPidl;
    VariantInit(&vPidl);
    vPidl.vt = VT_ARRAY | VT_UI1;
    vPidl.parray = psa;

    VARIANT vFlags;
    VariantInit(&vFlags);
    vFlags.vt = VT_I4;
    vFlags.lVal = 0;

    HRESULT hr = pBrowser->Navigate2(&vPidl, &vFlags, NULL, NULL, NULL);
    VariantClear(&vPidl);
    VariantClear(&vFlags);

    return SUCCEEDED(hr);
}

std::wstring TabBarUI::GetCurrentFolderName(IWebBrowser2* pBrowser)
{
    if (!pBrowser) return L"";

    LPITEMIDLIST pidl = GetCurrentPidlEx(pBrowser);
    if (pidl)
    {
        std::wstring name = GetNameFromPidl(pidl);
        ILFree(pidl);
        if (!name.empty())
            return name;
    }

    BSTR bstrName = NULL;
    if (SUCCEEDED(pBrowser->get_LocationName(&bstrName)) && bstrName)
    {
        std::wstring name(bstrName);
        SysFreeString(bstrName);
        return name;
    }
    return L"";
}

// ====================================================================
// 标签管理
// ====================================================================
void TabBarUI::FreeAllPidls()
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

// SEH 包装的 COM 调用（前向声明，实现在后面）
static LPITEMIDLIST SafeGetCurrentPidl(IWebBrowser2* pBrowser);
static bool SafeGetFolderName(IWebBrowser2* pBrowser, wchar_t* buf, int bufSize);

void TabBarUI::CreateInitialTab()
{
    if (!m_tabs.empty())
        return;

    if (!m_pBrowser)
        return;

    // 用 SEH 保护 COM 调用
    LPITEMIDLIST pidl = SafeGetCurrentPidl(m_pBrowser);
    std::wstring name;
    if (pidl)
    {
        name = GetNameFromPidl(pidl);
    }
    if (name.empty())
    {
        wchar_t nameBuf[MAX_PATH] = { 0 };
        if (SafeGetFolderName(m_pBrowser, nameBuf, MAX_PATH))
            name = nameBuf;
    }
    if (name.empty())
        name = L"资源管理器";
    if (!pidl)
    {
        pidl = GetSpecialFolderPidl(CSIDL_DRIVES);
        if (pidl)
        {
            std::wstring realName = GetNameFromPidl(pidl);
            if (!realName.empty())
                name = realName;
        }
    }

    TabItemUI tab;
    tab.title = name;
    tab.pidl = CopyPidl(pidl);
    tab.active = true;
    tab.rect = { 0, 0, m_tabWidth, m_windowHeight };
    m_tabs.push_back(tab);
    m_activeIndex = 0;

    UpdateTabRects();
    InvalidateRect(m_hwnd, NULL, FALSE);

    LogFmt(L"创建初始标签: %s", name.c_str());

    if (pidl)
        ILFree(pidl);
}

void TabBarUI::AddTab(LPCITEMIDLIST pidl, const std::wstring& title)
{
    TabItemUI tab;
    tab.title = title;
    tab.pidl = CopyPidl(pidl);
    tab.active = false;
    tab.rect = { 0, 0, 0, 0 };
    m_tabs.push_back(tab);

    LogFmt(L"新增标签: %s (总数=%d)", title.c_str(), m_tabs.size());

    ActivateTab(static_cast<int>(m_tabs.size()) - 1);
    UpdateTabRects();
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void TabBarUI::CloseTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size()))
        return;

    LogFmt(L"关闭标签 %d: %s", index, m_tabs[index].title.c_str());

    if (m_tabs[index].pidl)
        ILFree(m_tabs[index].pidl);
    m_tabs.erase(m_tabs.begin() + index);

    if (m_tabs.empty())
    {
        // 关闭 Explorer 窗口
        HWND hParent = GetParent(m_hwnd);
        while (hParent && GetParent(hParent))
            hParent = GetParent(hParent);
        if (hParent)
            PostMessageW(hParent, WM_CLOSE, 0, 0);
        return;
    }

    if (m_activeIndex >= static_cast<int>(m_tabs.size()))
        m_activeIndex = static_cast<int>(m_tabs.size()) - 1;

    ActivateTab(m_activeIndex);
    UpdateTabRects();
    InvalidateRect(m_hwnd, NULL, FALSE);
}

void TabBarUI::ActivateTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_tabs.size()))
        return;

    LogFmt(L"激活标签 %d: %s", index, m_tabs[index].title.c_str());

    for (auto& tab : m_tabs)
        tab.active = false;
    m_tabs[index].active = true;
    m_activeIndex = index;

    // 导航到该标签的 PIDL
    if (m_pBrowser && m_tabs[index].pidl)
    {
        NavigateToPidl(m_pBrowser, m_tabs[index].pidl);
    }

    InvalidateRect(m_hwnd, NULL, FALSE);
}

void TabBarUI::UpdateTabRects()
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
int TabBarUI::HitTest(int x, int y, int* outTabIndex)
{
    if (outTabIndex) *outTabIndex = -1;

    int plusX = static_cast<int>(m_tabs.size()) * m_tabWidth;

    // 收藏按钮
    int favX = plusX + kPlusButtonWidth;
    if (x >= favX && x <= favX + kFavoriteButtonWidth)
        return HIT_FAVORITE;

    // + 按钮
    if (x >= plusX && x <= plusX + kPlusButtonWidth)
        return HIT_PLUS;

    // 标签
    for (int i = 0; i < static_cast<int>(m_tabs.size()); i++)
    {
        const RECT& r = m_tabs[i].rect;
        if (x >= r.left && x < r.right)
        {
            int closeLeft = r.right - kCloseButtonWidth;
            if (x >= closeLeft && x <= r.right)
            {
                if (outTabIndex) *outTabIndex = i;
                return HIT_CLOSE;
            }
            return i;
        }
    }

    return HIT_NONE;
}

// ====================================================================
// SEH 包装的 COM 调用（避免崩溃）
// 注意：SEH __try/__except 不能和 C++ 对象析构混用，所以用纯 C 风格
// ====================================================================
static LPITEMIDLIST SafeGetCurrentPidl(IWebBrowser2* pBrowser)
{
    LPITEMIDLIST result = NULL;
    __try
    {
        result = TabBarUI::GetCurrentPidlEx(pBrowser);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        result = NULL;
    }
    return result;
}

static bool SafeGetFolderName(IWebBrowser2* pBrowser, wchar_t* buf, int bufSize)
{
    __try
    {
        LPITEMIDLIST pidl = TabBarUI::GetCurrentPidlEx(pBrowser);
        if (pidl)
        {
            SHFILEINFOW sfi = { 0 };
            if (SHGetFileInfoW(reinterpret_cast<LPCWSTR>(pidl), 0, &sfi, sizeof(sfi),
                               SHGFI_PIDL | SHGFI_DISPLAYNAME))
            {
                wcsncpy_s(buf, bufSize, sfi.szDisplayName, _TRUNCATE);
                ILFree(pidl);
                return true;
            }
            ILFree(pidl);
        }

        // 回退到 LocationName
        BSTR bstrName = NULL;
        if (SUCCEEDED(pBrowser->get_LocationName(&bstrName)) && bstrName)
        {
            wcsncpy_s(buf, bufSize, bstrName, _TRUNCATE);
            SysFreeString(bstrName);
            return true;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        return false;
    }
    return false;
}

// ====================================================================
// 消息处理
// 返回值约定：已处理返回非 0，未处理返回 0（由调用方继续处理）
// ====================================================================
LRESULT TabBarUI::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
        OnPaint();
        return 1;  // 已处理

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
            // + 按钮：新建标签，默认导航到"此电脑"
            LPITEMIDLIST pidlThisPC = GetSpecialFolderPidl(CSIDL_DRIVES);
            std::wstring nameThisPC = L"此电脑";
            if (pidlThisPC)
            {
                std::wstring realName = GetNameFromPidl(pidlThisPC);
                if (!realName.empty())
                    nameThisPC = realName;
            }
            AddTab(pidlThisPC, nameThisPC);
            if (pidlThisPC)
                ILFree(pidlThisPC);
            break;
        }
        case HIT_CLOSE:
            CloseTab(tabIndex);
            break;
        default:
            if (hit >= 0 && hit != m_activeIndex)
                ActivateTab(hit);
            break;
        }
        return 1;  // 已处理
    }

    case WM_MOUSEMOVE:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        int tabIndex = -1;
        int hit = HitTest(x, y, &tabIndex);

        int newHover = (hit >= 0) ? hit : -1;
        int newHoverBtn = (hit < 0 && hit != HIT_NONE) ? hit : HIT_NONE;

        if (newHover != m_hoverTab || newHoverBtn != m_hoverButton)
        {
            m_hoverTab = newHover;
            m_hoverButton = newHoverBtn;
            InvalidateRect(m_hwnd, NULL, FALSE);
        }

        // 启用鼠标跟踪以接收 WM_MOUSELEAVE
        TRACKMOUSEEVENT tme = { sizeof(tme) };
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hwnd;
        TrackMouseEvent(&tme);
        return 1;
    }

    case WM_MOUSELEAVE:
        m_hoverTab = -1;
        m_hoverButton = HIT_NONE;
        InvalidateRect(m_hwnd, NULL, FALSE);
        return 1;

    case WM_ERASEBKGND:
        return 1;  // 已处理（阻止背景擦除）

    case WM_SETCURSOR:
        SetCursor(LoadCursor(NULL, IDC_ARROW));
        return 1;

    case WM_TIMER:
        if (wParam == kTabBarTimerId)
        {
            OnTimerTick();
            return 1;
        }
        break;
    }

    return 0;  // 未处理
}

// ====================================================================
// 定时器：检查导航变化并更新当前标签
// 关键：所有 COM 调用都要检查 m_pBrowser，避免崩溃
// ====================================================================
void TabBarUI::OnTimerTick()
{
    // 如果还没标签且有 browser，创建初始标签
    if (m_tabs.empty())
    {
        if (m_pBrowser)
        {
            CreateInitialTab();
        }
        return;
    }

    if (!m_pBrowser)
        return;

    if (m_activeIndex < 0 || m_activeIndex >= static_cast<int>(m_tabs.size()))
        return;

    // 节流
    DWORD now = GetTickCount();
    if (now - m_lastCheckTick < 1000)
        return;
    m_lastCheckTick = now;

    // 检查导航变化
    // 注意：SEH __try/__except 不能和 C++ 对象析构混用，所以拆成单独函数
    LPITEMIDLIST curPidl = SafeGetCurrentPidl(m_pBrowser);
    if (!curPidl)
        return;

    LPCITEMIDLIST tabPidl = m_tabs[m_activeIndex].pidl;
    bool changed = false;
    if (!tabPidl)
        changed = true;
    else
        changed = !ILIsEqual(tabPidl, curPidl);

    if (changed)
    {
        if (tabPidl)
            ILFree(const_cast<LPITEMIDLIST>(tabPidl));
        m_tabs[m_activeIndex].pidl = CopyPidl(curPidl);

        // 获取名称也用 SEH 保护
        wchar_t nameBuf[MAX_PATH] = { 0 };
        if (SafeGetFolderName(m_pBrowser, nameBuf, MAX_PATH))
        {
            m_tabs[m_activeIndex].title = nameBuf;
        }

        InvalidateRect(m_hwnd, NULL, FALSE);
    }

    ILFree(curPidl);
}

// ====================================================================
// 绘制
// ====================================================================
void TabBarUI::OnPaint()
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(m_hwnd, &ps);

    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // 调试日志：确认 OnPaint 被调用
    static DWORD lastPaintLog = 0;
    DWORD now = GetTickCount();
    if (now - lastPaintLog > 1000)  // 限制日志频率
    {
        wchar_t buf[128];
        _snwprintf_s(buf, _countof(buf), _TRUNCATE,
                     L"OnPaint w=%d h=%d tabs=%d", width, height, (int)m_tabs.size());
        Log(buf);
        lastPaintLog = now;
    }

    if (width <= 0 || height <= 0)
    {
        EndPaint(m_hwnd, &ps);
        return;
    }

    // 双缓冲
    HDC memDc = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDc, memBitmap);

    // 背景
    HBRUSH bgBrush = CreateSolidBrush(kColorBg);
    FillRect(memDc, &clientRect, bgBrush);
    DeleteObject(bgBrush);

    SetBkMode(memDc, TRANSPARENT);

    HFONT hFont = CreateFontW(
        14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(memDc, hFont);

    // 绘制各标签
    for (int i = 0; i < static_cast<int>(m_tabs.size()); i++)
    {
        const TabItemUI& tab = m_tabs[i];
        RECT r = tab.rect;

        // 标签背景
        COLORREF tabColor = tab.active ? kColorTabActive : kColorTabInactive;
        if (i == m_hoverTab && !tab.active)
            tabColor = kColorTabHover;

        HBRUSH tabBrush = CreateSolidBrush(tabColor);
        RECT fillRect = { r.left, r.top, r.right, r.bottom };
        FillRect(memDc, &fillRect, tabBrush);
        DeleteObject(tabBrush);

        // 分隔线
        HPEN sepPen = CreatePen(PS_SOLID, 1, kColorSeparator);
        HPEN oldPen = (HPEN)SelectObject(memDc, sepPen);
        MoveToEx(memDc, r.right - 1, r.top, NULL);
        LineTo(memDc, r.right - 1, r.bottom);
        SelectObject(memDc, oldPen);
        DeleteObject(sepPen);

        // 标签文字
        SetTextColor(memDc, kColorText);
        RECT textRect = { r.left + kTextPadding, r.top, r.right - kCloseButtonWidth, r.bottom };
        std::wstring title = tab.title;
        if (title.length() > 20)
            title = title.substr(0, 18) + L"...";
        DrawTextW(memDc, title.c_str(), -1, &textRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        // 关闭按钮 ×
        RECT closeRect = { r.right - kCloseButtonWidth, r.top, r.right, r.bottom };
        SetTextColor(memDc, (i == m_hoverTab) ? kColorCloseActive : kColorClose);
        DrawTextW(memDc, L"×", -1, &closeRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // + 按钮
    int plusX = static_cast<int>(m_tabs.size()) * m_tabWidth;
    RECT plusRect = { plusX, 0, plusX + kPlusButtonWidth, m_windowHeight };
    if (m_hoverButton == HIT_PLUS)
    {
        HBRUSH hoverBrush = CreateSolidBrush(kColorTabHover);
        FillRect(memDc, &plusRect, hoverBrush);
        DeleteObject(hoverBrush);
    }
    SetTextColor(memDc, kColorPlus);
    DrawTextW(memDc, L"+", -1, &plusRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // ☆ 收藏按钮
    int favX = plusX + kPlusButtonWidth;
    RECT favRect = { favX, 0, favX + kFavoriteButtonWidth, m_windowHeight };
    if (m_hoverButton == HIT_FAVORITE)
    {
        HBRUSH hoverBrush = CreateSolidBrush(kColorTabHover);
        FillRect(memDc, &favRect, hoverBrush);
        DeleteObject(hoverBrush);
    }
    SetTextColor(memDc, kColorPlus);
    DrawTextW(memDc, L"☆", -1, &favRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 关键：把内存 DC 拷贝到屏幕 DC（之前漏掉这步，导致白框）
    BitBlt(hdc, 0, 0, width, height, memDc, 0, 0, SRCCOPY);

    // 清理
    SelectObject(memDc, oldFont);
    DeleteObject(hFont);
    SelectObject(memDc, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDc);

    EndPaint(m_hwnd, &ps);
}
