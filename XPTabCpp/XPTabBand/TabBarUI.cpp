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
// 现代暗色主题配色（参考 Chrome/Edge）
// ====================================================================
const COLORREF TabBarUI::kColorBg         = RGB(32, 32, 32);      // 标签栏背景
const COLORREF TabBarUI::kColorTabActive  = RGB(62, 62, 62);      // 激活标签背景
const COLORREF TabBarUI::kColorTabInactive= RGB(38, 38, 38);      // 非激活标签背景（接近背景）
const COLORREF TabBarUI::kColorTabHover   = RGB(50, 50, 50);      // 悬停非激活标签
const COLORREF TabBarUI::kColorText       = RGB(240, 240, 240);   // 激活标签文字
const COLORREF TabBarUI::kColorTextInactive = RGB(170, 170, 170);// 非激活标签文字
const COLORREF TabBarUI::kColorClose      = RGB(160, 160, 160);
const COLORREF TabBarUI::kColorCloseActive= RGB(255, 90, 90);
const COLORREF TabBarUI::kColorPlus       = RGB(220, 220, 220);
const COLORREF TabBarUI::kColorSeparator  = RGB(56, 56, 56);
const COLORREF TabBarUI::kColorAccent     = RGB(0, 120, 215);     // 激活标签顶部蓝色高亮条
const COLORREF TabBarUI::kColorBtnHover   = RGB(75, 75, 75);      // 按钮悬停背景

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
    , m_favoritesLoaded(false)
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
    FreeAllFavoritePidls();
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

// 用 IShellBrowser::BrowseObject 在当前 ShellView 内切换文件夹
// SBSP_SAMEBROWSER 标志强制在当前浏览器中切换，避免弹出新窗口
bool TabBarUI::BrowseObjectPidl(IWebBrowser2* pBrowser, LPCITEMIDLIST pidl)
{
    if (!pBrowser || !pidl) return false;

    IServiceProvider* pSvc = NULL;
    if (FAILED(pBrowser->QueryInterface(IID_PPV_ARGS(&pSvc))) || !pSvc)
        return false;

    IShellBrowser* pShellBrowser = NULL;
    HRESULT hr = pSvc->QueryService(SID_SShellBrowser, IID_PPV_ARGS(&pShellBrowser));
    pSvc->Release();
    if (FAILED(hr) || !pShellBrowser)
        return false;

    // SBSP_SAMEBROWSER: 在当前浏览器中切换
    // SBSP_ABSOLUTE: pidl 是绝对 PIDL
    hr = pShellBrowser->BrowseObject(pidl, SBSP_SAMEBROWSER | SBSP_ABSOLUTE);
    pShellBrowser->Release();

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
    // 优先使用 BrowseObject（在当前 ShellView 内切换，避免新窗口）
    // 失败时回退到 Navigate2
    if (m_pBrowser && m_tabs[index].pidl)
    {
        if (!BrowseObjectPidl(m_pBrowser, m_tabs[index].pidl))
        {
            Log(L"ActivateTab: BrowseObject 失败，回退到 Navigate2");
            NavigateToPidl(m_pBrowser, m_tabs[index].pidl);
        }
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
        case HIT_FAVORITE:
            // 左键 ☆：把当前文件夹加入收藏
            AddCurrentToFavorites();
            break;
        default:
            if (hit >= 0 && hit != m_activeIndex)
                ActivateTab(hit);
            break;
        }
        return 1;  // 已处理
    }

    case WM_RBUTTONUP:
    {
        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        int tabIndex = -1;
        int hit = HitTest(x, y, &tabIndex);

        if (hit == HIT_NONE)
        {
            // 在标签栏空白处右键：弹出收藏夹菜单
            POINT pt = { x, y };
            ClientToScreen(m_hwnd, &pt);
            ShowFavoritesMenu(pt.x, pt.y);
            return 1;
        }
        break;
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
// 收藏夹
// 文件格式（二进制）：
//   magic   : 4 bytes "XPFV"
//   count   : 4 bytes (uint32)
//   每个 item:
//     pidl_cb : 4 bytes (uint32, PIDL 字节数，含末尾 2 个 0)
//     pidl    : pidl_cb bytes
//     name_len: 4 bytes (uint32, wchar_t 个数，不含末尾 0)
//     name    : name_len * 2 bytes (UTF-16)
// ====================================================================
std::wstring TabBarUI::GetFavoritesFilePath()
{
    wchar_t appData[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData)))
    {
        std::wstring dir = std::wstring(appData) + L"\\XPTabCpp";
        CreateDirectoryW(dir.c_str(), NULL);
        return dir + L"\\favorites.dat";
    }
    return L"";
}

void TabBarUI::FreeAllFavoritePidls()
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

void TabBarUI::LoadFavorites()
{
    if (m_favoritesLoaded) return;
    m_favoritesLoaded = true;

    std::wstring path = GetFavoritesFilePath();
    if (path.empty()) return;

    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) return;

    char magic[4] = { 0 };
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "XPFV", 4) != 0)
    {
        fclose(f);
        return;
    }

    uint32_t count = 0;
    if (fread(&count, 4, 1, f) != 1)
    {
        fclose(f);
        return;
    }

    for (uint32_t i = 0; i < count && i < 200; i++)
    {
        uint32_t pidlCb = 0;
        if (fread(&pidlCb, 4, 1, f) != 1 || pidlCb == 0 || pidlCb > 64 * 1024)
            break;

        std::vector<unsigned char> buf(pidlCb);
        if (fread(buf.data(), 1, pidlCb, f) != pidlCb)
            break;

        LPITEMIDLIST pidl = reinterpret_cast<LPITEMIDLIST>(CoTaskMemAlloc(pidlCb));
        if (!pidl) break;
        memcpy(pidl, buf.data(), pidlCb);

        uint32_t nameLen = 0;
        if (fread(&nameLen, 4, 1, f) != 1 || nameLen > 1024)
        {
            CoTaskMemFree(pidl);
            break;
        }

        std::wstring name;
        if (nameLen > 0)
        {
            std::vector<wchar_t> wbuf(nameLen);
            if (fread(wbuf.data(), 2, nameLen, f) != nameLen)
            {
                CoTaskMemFree(pidl);
                break;
            }
            name.assign(wbuf.data(), nameLen);
        }

        FavoriteItem item;
        item.title = name;
        item.pidl = pidl;
        m_favorites.push_back(item);
    }
    fclose(f);

    LogFmt(L"LoadFavorites: 加载 %d 个收藏", (int)m_favorites.size());
}

void TabBarUI::SaveFavorites()
{
    std::wstring path = GetFavoritesFilePath();
    if (path.empty()) return;

    FILE* f = nullptr;
    if (_wfopen_s(&f, path.c_str(), L"wb") != 0 || !f) return;

    fwrite("XPFV", 1, 4, f);
    uint32_t count = (uint32_t)m_favorites.size();
    fwrite(&count, 4, 1, f);

    for (auto& fav : m_favorites)
    {
        uint32_t pidlCb = fav.pidl ? (uint32_t)ILGetSize(fav.pidl) : 0;
        fwrite(&pidlCb, 4, 1, f);
        if (pidlCb > 0)
            fwrite(fav.pidl, 1, pidlCb, f);

        uint32_t nameLen = (uint32_t)fav.title.size();
        fwrite(&nameLen, 4, 1, f);
        if (nameLen > 0)
            fwrite(fav.title.data(), 2, nameLen, f);
    }
    fclose(f);

    LogFmt(L"SaveFavorites: 保存 %d 个收藏", (int)m_favorites.size());
}

void TabBarUI::AddCurrentToFavorites()
{
    if (!m_pBrowser) return;
    LoadFavorites();

    LPITEMIDLIST pidl = SafeGetCurrentPidl(m_pBrowser);
    if (!pidl)
    {
        Log(L"AddCurrentToFavorites: 获取当前 PIDL 失败");
        return;
    }

    std::wstring name = GetNameFromPidl(pidl);
    if (name.empty())
    {
        wchar_t nameBuf[MAX_PATH] = { 0 };
        if (SafeGetFolderName(m_pBrowser, nameBuf, MAX_PATH))
            name = nameBuf;
    }
    if (name.empty())
        name = L"未知";

    // 查重
    for (auto& fav : m_favorites)
    {
        if (fav.pidl && ILIsEqual(fav.pidl, pidl))
        {
            LogFmt(L"AddCurrentToFavorites: 已存在 %s，跳过", name.c_str());
            ILFree(pidl);
            return;
        }
    }

    FavoriteItem item;
    item.title = name;
    item.pidl = pidl;  // 转移所有权
    m_favorites.push_back(item);
    SaveFavorites();

    LogFmt(L"AddCurrentToFavorites: 添加 %s (总数=%d)", name.c_str(), (int)m_favorites.size());
}

void TabBarUI::ShowFavoritesMenu(int screenX, int screenY)
{
    LoadFavorites();

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    if (m_favorites.empty())
    {
        AppendMenuW(hMenu, MF_STRING | MF_DISABLED, 0, L"（暂无收藏，左键 ☆ 添加）");
    }
    else
    {
        for (size_t i = 0; i < m_favorites.size(); i++)
        {
            AppendMenuW(hMenu, MF_STRING, (UINT_PTR)(i + 1), m_favorites[i].title.c_str());
        }
        AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuW(hMenu, MF_STRING, 0xFFFE, L"打开收藏夹文件夹");
        AppendMenuW(hMenu, MF_STRING, 0xFFFD, L"清空收藏夹");
    }

    // 用 TPM_RETURNCMD 直接拿到选择的 ID
    int cmd = TrackPopupMenu(
        hMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
        screenX, screenY, 0, m_hwnd, NULL);

    DestroyMenu(hMenu);

    if (cmd == 0)
        return;

    if (cmd == 0xFFFE)
    {
        // 打开收藏夹所在文件夹
        std::wstring path = GetFavoritesFilePath();
        if (!path.empty())
        {
            size_t pos = path.find_last_of(L"\\");
            if (pos != std::wstring::npos)
            {
                std::wstring dir = path.substr(0, pos);
                ShellExecuteW(NULL, L"open", L"explorer.exe", dir.c_str(), NULL, SW_SHOWNORMAL);
            }
        }
        return;
    }

    if (cmd == 0xFFFD)
    {
        // 清空收藏夹
        FreeAllFavoritePidls();
        SaveFavorites();
        Log(L"ShowFavoritesMenu: 已清空收藏夹");
        return;
    }

    // cmd 是 1-based 索引
    int idx = cmd - 1;
    if (idx >= 0 && idx < (int)m_favorites.size())
    {
        FavoriteItem& fav = m_favorites[idx];
        if (fav.pidl)
        {
            // 在新标签打开（AddTab 会拷贝 PIDL 并激活）
            AddTab(fav.pidl, fav.title);
            LogFmt(L"ShowFavoritesMenu: 新标签打开 %s", fav.title.c_str());
        }
    }
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

    // 顶部细高光线（增加深度感）
    {
        HPEN pen = CreatePen(PS_SOLID, 1, RGB(48, 48, 48));
        HPEN oldPen = (HPEN)SelectObject(memDc, pen);
        MoveToEx(memDc, 0, 0, NULL);
        LineTo(memDc, width, 0);
        SelectObject(memDc, oldPen);
        DeleteObject(pen);
    }

    SetBkMode(memDc, TRANSPARENT);

    // 字体：Segoe UI
    HFONT hFont = CreateFontW(
        14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    HFONT oldFont = (HFONT)SelectObject(memDc, hFont);

    // 标签布局参数
    const int kTabPaddingX   = 3;   // 标签左右内边距（视觉间距）
    const int kTabMarginTop  = 3;   // 标签顶部边距
    const int kAccentHeight  = 3;   // 激活标签顶部高亮条高度
    const int kCircleRadius  = 9;   // 关闭按钮圆形背景半径
    const int kBtnRadius     = 11;  // + / ☆ 按钮圆形背景半径

    // 绘制各标签
    for (int i = 0; i < static_cast<int>(m_tabs.size()); i++)
    {
        const TabItemUI& tab = m_tabs[i];
        RECT r = tab.rect;

        // 实际绘制区域（左右留间距，上下留边距）
        RECT tabRect = {
            r.left + kTabPaddingX,
            r.top + kTabMarginTop,
            r.right - kTabPaddingX,
            r.bottom
        };

        // 选择背景颜色
        COLORREF tabColor;
        if (tab.active)
            tabColor = kColorTabActive;
        else if (i == m_hoverTab)
            tabColor = kColorTabHover;
        else
            tabColor = kColorTabInactive;

        // 标签背景
        HBRUSH tabBrush = CreateSolidBrush(tabColor);
        FillRect(memDc, &tabRect, tabBrush);
        DeleteObject(tabBrush);

        // 激活标签顶部蓝色高亮条（Chrome 风格）
        if (tab.active)
        {
            HBRUSH accentBrush = CreateSolidBrush(kColorAccent);
            RECT accentRect = { tabRect.left, tabRect.top, tabRect.right, tabRect.top + kAccentHeight };
            FillRect(memDc, &accentRect, accentBrush);
            DeleteObject(accentBrush);
        }

        // 标签文字
        SetTextColor(memDc, tab.active ? kColorText : kColorTextInactive);
        RECT textRect = {
            r.left + kTextPadding,
            r.top,
            r.right - kCloseButtonWidth,
            r.bottom
        };
        DrawTextW(memDc, tab.title.c_str(), -1, &textRect,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        // 关闭按钮 ×（悬停时画圆形背景）
        RECT closeRect = { r.right - kCloseButtonWidth, r.top, r.right, r.bottom };
        int cx = (closeRect.left + closeRect.right) / 2;
        int cy = (closeRect.top + closeRect.bottom) / 2;
        if (i == m_hoverTab)
        {
            HBRUSH circleBrush = CreateSolidBrush(kColorBtnHover);
            HPEN oldPen = (HPEN)SelectObject(memDc, GetStockObject(NULL_PEN));
            HBRUSH oldBrush = (HBRUSH)SelectObject(memDc, circleBrush);
            Ellipse(memDc, cx - kCircleRadius, cy - kCircleRadius,
                    cx + kCircleRadius, cy + kCircleRadius);
            SelectObject(memDc, oldPen);
            SelectObject(memDc, oldBrush);
            DeleteObject(circleBrush);
        }
        SetTextColor(memDc, (i == m_hoverTab) ? kColorCloseActive : kColorClose);
        DrawTextW(memDc, L"×", -1, &closeRect,
                  DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    // + 按钮（圆形悬停背景）
    int plusX = static_cast<int>(m_tabs.size()) * m_tabWidth;
    RECT plusRect = { plusX, 0, plusX + kPlusButtonWidth, m_windowHeight };
    int plusCx = (plusRect.left + plusRect.right) / 2;
    int plusCy = (plusRect.top + plusRect.bottom) / 2;
    if (m_hoverButton == HIT_PLUS)
    {
        HBRUSH circleBrush = CreateSolidBrush(kColorBtnHover);
        HPEN oldPen = (HPEN)SelectObject(memDc, GetStockObject(NULL_PEN));
        HBRUSH oldBrush = (HBRUSH)SelectObject(memDc, circleBrush);
        Ellipse(memDc, plusCx - kBtnRadius, plusCy - kBtnRadius,
                plusCx + kBtnRadius, plusCy + kBtnRadius);
        SelectObject(memDc, oldPen);
        SelectObject(memDc, oldBrush);
        DeleteObject(circleBrush);
    }
    SetTextColor(memDc, kColorPlus);
    DrawTextW(memDc, L"+", -1, &plusRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // ☆ 收藏按钮（圆形悬停背景）
    int favX = plusX + kPlusButtonWidth;
    RECT favRect = { favX, 0, favX + kFavoriteButtonWidth, m_windowHeight };
    int favCx = (favRect.left + favRect.right) / 2;
    int favCy = (favRect.top + favRect.bottom) / 2;
    if (m_hoverButton == HIT_FAVORITE)
    {
        HBRUSH circleBrush = CreateSolidBrush(kColorBtnHover);
        HPEN oldPen = (HPEN)SelectObject(memDc, GetStockObject(NULL_PEN));
        HBRUSH oldBrush = (HBRUSH)SelectObject(memDc, circleBrush);
        Ellipse(memDc, favCx - kBtnRadius, favCy - kBtnRadius,
                favCx + kBtnRadius, favCy + kBtnRadius);
        SelectObject(memDc, oldPen);
        SelectObject(memDc, oldBrush);
        DeleteObject(circleBrush);
    }
    SetTextColor(memDc, kColorPlus);
    DrawTextW(memDc, L"☆", -1, &favRect,
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    // 拷贝到屏幕 DC
    BitBlt(hdc, 0, 0, width, height, memDc, 0, 0, SRCCOPY);

    // 清理
    SelectObject(memDc, oldFont);
    DeleteObject(hFont);
    SelectObject(memDc, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDc);

    EndPaint(m_hwnd, &ps);
}
