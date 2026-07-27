// XPTabBandClass.cpp - DeskBand COM 组件实现
//
// Explorer 通过 COM 创建此对象并加载为工具栏 Band
// 本文件实现骨架，先验证能被 Explorer 加载并显示一个空白窗口
// 后续阶段在此添加 TabBar UI 和窗口捕获逻辑

#include "stdafx.h"
#include "XPTabBandClass.h"
#include "TabBarUI.h"
#include <cstdio>

// 日志输出（调试用）
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
// 静态成员初始化
// ====================================================================
ATOM XPTabBandClass::s_classAtom = 0;
const wchar_t* XPTabBandClass::kBandClassName = L"XPTabBandClass";

// ====================================================================
// 构造/析构
// ====================================================================
XPTabBandClass::XPTabBandClass()
    : m_cRef(1)
    , m_hwndParent(NULL)
    , m_hwnd(NULL)
    , m_dwBandID(0)
    , m_dwViewMode(0)
    , m_bCompositionEnabled(FALSE)
    , m_bShow(FALSE)
    , m_pSite(NULL)
    , m_pBrowser(NULL)
    , m_pTabBar(nullptr)
{
    Log(L"XPTabBandClass 构造");
}

XPTabBandClass::~XPTabBandClass()
{
    Log(L"XPTabBandClass 析构");
    DestroyBandWindow();
    if (m_pTabBar)
    {
        m_pTabBar->Uninitialize();
        delete m_pTabBar;
        m_pTabBar = nullptr;
    }
    if (m_pBrowser)
    {
        m_pBrowser->Release();
        m_pBrowser = NULL;
    }
    if (m_pSite)
    {
        m_pSite->Release();
        m_pSite = NULL;
    }
}

// ====================================================================
// IUnknown
// ====================================================================
STDMETHODIMP XPTabBandClass::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    if (riid == IID_IUnknown)
        *ppv = static_cast<IUnknown*>(static_cast<IObjectWithSite*>(this));
    else if (riid == IID_IObjectWithSite)
        *ppv = static_cast<IObjectWithSite*>(this);
    else if (riid == IID_IOleWindow)
        *ppv = static_cast<IOleWindow*>(this);
    else if (riid == IID_IDockingWindow)
        *ppv = static_cast<IDockingWindow*>(this);
    else if (riid == IID_IDeskBand)
        *ppv = static_cast<IDeskBand*>(this);
    else if (riid == IID_IDeskBand2)
        *ppv = static_cast<IDeskBand2*>(this);
    else if (riid == IID_IPersist)
        *ppv = static_cast<IPersist*>(this);
    else if (riid == IID_IPersistStream)
        *ppv = static_cast<IPersistStream*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) XPTabBandClass::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) XPTabBandClass::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
        delete this;
    return cRef;
}

// ====================================================================
// IObjectWithSite
// ====================================================================
STDMETHODIMP XPTabBandClass::SetSite(IUnknown* pUnkSite)
{
    LogFmt(L"SetSite pUnkSite=%p", pUnkSite);

    // 释放旧资源（关闭阶段）
    if (!pUnkSite)
    {
        if (m_pTabBar)
        {
            m_pTabBar->Uninitialize();
            delete m_pTabBar;
            m_pTabBar = nullptr;
        }
        DestroyBandWindow();
        if (m_pBrowser)
        {
            m_pBrowser->Release();
            m_pBrowser = NULL;
        }
        if (m_pSite)
        {
            m_pSite->Release();
            m_pSite = NULL;
        }
        return S_OK;
    }

    // 保存新站点
    if (m_pSite)
    {
        m_pSite->Release();
        m_pSite = NULL;
    }
    m_pSite = pUnkSite;
    m_pSite->AddRef();

    // 获取 Explorer 窗口句柄（通过 IOleWindow）
    // 用 try/catch 防护，避免崩溃
    IOleWindow* pOleWindow = NULL;
    HRESULT hr = pUnkSite->QueryInterface(IID_PPV_ARGS(&pOleWindow));
    if (SUCCEEDED(hr) && pOleWindow)
    {
        HWND hwndParent = NULL;
        if (SUCCEEDED(pOleWindow->GetWindow(&hwndParent)) && hwndParent)
        {
            m_hwndParent = hwndParent;
            LogFmt(L"SetSite 获取窗口句柄=%p", hwndParent);

            // 创建 Band 窗口
            CreateBandWindow(hwndParent);
        }
        pOleWindow->Release();
    }

    // 获取 IWebBrowser2
    // 参考 QTTabBar BandObject.cs SetSite 实现：
    //   使用 IServiceProvider::QueryService(SID_SWebBrowserApp, IID_IUnknown)
    //   而不是 QueryService(SID_SShellBrowser)
    // SID_SWebBrowserApp = IID_IWebBrowserApp = {0002DF05-0000-0000-C000-000000000046}
    IServiceProvider* pSvc = NULL;
    hr = pUnkSite->QueryInterface(IID_PPV_ARGS(&pSvc));
    if (SUCCEEDED(hr) && pSvc)
    {
        // 方法1: QueryService(SID_SWebBrowserApp) - QTTabBar 的做法
        // SID_SWebBrowserApp 就是 IID_IWebBrowserApp
        // {0002DF05-0000-0000-C000-000000000046}
        static const IID SID_SWebBrowserApp =
            { 0x0002df05, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 } };

        IUnknown* pUnkBrowser = NULL;
        hr = pSvc->QueryService(SID_SWebBrowserApp, IID_IUnknown, (void**)&pUnkBrowser);
        if (SUCCEEDED(hr) && pUnkBrowser)
        {
            hr = pUnkBrowser->QueryInterface(IID_PPV_ARGS(&m_pBrowser));
            pUnkBrowser->Release();
            if (SUCCEEDED(hr) && m_pBrowser)
            {
                LogFmt(L"SetSite 获取 IWebBrowser2 成功 (SID_SWebBrowserApp)");
            }
        }

        // 方法2: 如果上面失败，尝试 SID_SShellBrowser
        if (!m_pBrowser)
        {
            IShellBrowser* pShellBrowser = NULL;
            hr = pSvc->QueryService(SID_SShellBrowser, IID_PPV_ARGS(&pShellBrowser));
            if (SUCCEEDED(hr) && pShellBrowser)
            {
                hr = pShellBrowser->QueryInterface(IID_PPV_ARGS(&m_pBrowser));
                if (SUCCEEDED(hr) && m_pBrowser)
                {
                    LogFmt(L"SetSite 获取 IWebBrowser2 成功 (SID_SShellBrowser)");
                }
                pShellBrowser->Release();
            }
        }

        pSvc->Release();
    }

    // 初始化 TabBarUI（即使没有 IWebBrowser2 也创建，先显示 UI）
    if (m_hwnd)
    {
        m_pTabBar = new TabBarUI();
        if (m_pTabBar)
        {
            // 初始化时不传 IWebBrowser2，等定时器中再获取
            m_pTabBar->Initialize(m_hwnd, m_pBrowser);
            Log(L"TabBarUI 初始化");
        }
    }
    else
    {
        LogFmt(L"未创建 TabBarUI: m_hwnd=NULL（窗口类注册失败）");
        // 窗口创建失败时，释放已获取的资源，避免后续操作崩溃
        if (m_pBrowser)
        {
            m_pBrowser->Release();
            m_pBrowser = NULL;
        }
        if (m_pSite)
        {
            m_pSite->Release();
            m_pSite = NULL;
        }
        return E_FAIL;
    }

    return S_OK;
}

STDMETHODIMP XPTabBandClass::GetSite(REFIID riid, void** ppvSite)
{
    if (!ppvSite)
        return E_POINTER;
    if (!m_pSite)
        return E_FAIL;
    return m_pSite->QueryInterface(riid, ppvSite);
}

// ====================================================================
// IOleWindow
// ====================================================================
STDMETHODIMP XPTabBandClass::GetWindow(HWND* phwnd)
{
    if (!phwnd)
        return E_POINTER;
    *phwnd = m_hwnd;
    return S_OK;
}

STDMETHODIMP XPTabBandClass::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

// ====================================================================
// IDockingWindow
// ====================================================================
STDMETHODIMP XPTabBandClass::CloseDW(DWORD dwReserved)
{
    Log(L"CloseDW");

    // 按 QTTabBar BandObject.cs CloseDW 顺序：
    // 1. 先销毁窗口（窗口销毁期间 COM 仍可用）
    // 2. 再释放 COM 对象
    ShowDW(FALSE);

    if (m_pTabBar)
    {
        m_pTabBar->Uninitialize();
        delete m_pTabBar;
        m_pTabBar = nullptr;
    }
    DestroyBandWindow();

    if (m_pBrowser)
    {
        m_pBrowser->Release();
        m_pBrowser = NULL;
    }
    if (m_pSite)
    {
        m_pSite->Release();
        m_pSite = NULL;
    }
    return S_OK;
}

STDMETHODIMP XPTabBandClass::ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL f)
{
    return E_NOTIMPL;
}

STDMETHODIMP XPTabBandClass::ShowDW(BOOL fShow)
{
    LogFmt(L"ShowDW fShow=%d hwnd=%p", fShow, m_hwnd);
    m_bShow = fShow;
    if (m_hwnd)
    {
        ShowWindow(m_hwnd, fShow ? SW_SHOW : SW_HIDE);
        if (fShow)
        {
            // 显示后立即触发重绘
            InvalidateRect(m_hwnd, NULL, TRUE);
            UpdateWindow(m_hwnd);
        }
    }
    return S_OK;
}

// ====================================================================
// IDeskBand
// ====================================================================
STDMETHODIMP XPTabBandClass::GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi)
{
    LogFmt(L"GetBandInfo bandID=%lu viewMode=%lu mask=0x%08X", dwBandID, dwViewMode, pdbi ? pdbi->dwMask : 0);

    m_dwBandID = dwBandID;
    m_dwViewMode = dwViewMode;

    if (!pdbi)
        return E_POINTER;

    // Band 高度固定为 30 像素
    const int kBandHeight = 30;

    // 参考 QTTabBar BandObject.cs GetBandInfo 实现
    if (pdbi->dwMask & DBIM_MINSIZE)
    {
        pdbi->ptMinSize.x = 0;            // 最小宽度 0
        pdbi->ptMinSize.y = kBandHeight;  // 最小高度
    }
    if (pdbi->dwMask & DBIM_MAXSIZE)
    {
        // -1 表示无最大限制（QTTabBar 的做法）
        pdbi->ptMaxSize.x = -1;
        pdbi->ptMaxSize.y = -1;
    }
    if (pdbi->dwMask & DBIM_INTEGRAL)
    {
        // -1 表示无步进（QTTabBar 的做法）
        pdbi->ptIntegral.x = -1;
        pdbi->ptIntegral.y = -1;
    }
    if (pdbi->dwMask & DBIM_ACTUAL)
    {
        pdbi->ptActual.x = 0;             // 0 表示使用可用宽度
        pdbi->ptActual.y = kBandHeight;   // 实际高度
    }
    if (pdbi->dwMask & DBIM_TITLE)
    {
        // 不显示标题
        pdbi->dwMask &= ~DBIM_TITLE;
    }
    if (pdbi->dwMask & DBIM_MODEFLAGS)
    {
        // QTTabBar 只用 DBIMF_NORMAL，不用 VARIABLEHEIGHT / USECHEVRON
        pdbi->dwModeFlags = DBIMF_NORMAL;
    }
    if (pdbi->dwMask & DBIM_BKCOLOR)
    {
        // 不设置背景色（由 Band 自己绘制）
        pdbi->dwMask &= ~DBIM_BKCOLOR;
    }

    return S_OK;
}

// ====================================================================
// IDeskBand2
// ====================================================================
STDMETHODIMP XPTabBandClass::CanRenderComposited(BOOL* pfCanRenderComposited)
{
    if (pfCanRenderComposited)
        *pfCanRenderComposited = TRUE;
    return S_OK;
}

STDMETHODIMP XPTabBandClass::SetCompositionState(BOOL fCompositionEnabled)
{
    m_bCompositionEnabled = fCompositionEnabled;
    return S_OK;
}

STDMETHODIMP XPTabBandClass::GetCompositionState(BOOL* pfCompositionEnabled)
{
    if (pfCompositionEnabled)
        *pfCompositionEnabled = m_bCompositionEnabled;
    return S_OK;
}

// ====================================================================
// IPersist
// ====================================================================
STDMETHODIMP XPTabBandClass::GetClassID(CLSID* pClassID)
{
    if (!pClassID)
        return E_POINTER;
    *pClassID = CLSID_XPTabBand;
    return S_OK;
}

// ====================================================================
// IPersistStream
// ====================================================================
STDMETHODIMP XPTabBandClass::IsDirty()
{
    return S_FALSE;
}

STDMETHODIMP XPTabBandClass::Load(IStream* pStm)
{
    Log(L"IPersistStream::Load");
    return S_OK;
}

STDMETHODIMP XPTabBandClass::Save(IStream* pStm, BOOL fClearDirty)
{
    return S_OK;
}

STDMETHODIMP XPTabBandClass::GetSizeMax(ULARGE_INTEGER* pcbSize)
{
    if (pcbSize)
        pcbSize->QuadPart = 0;
    return S_OK;
}

// ====================================================================
// 窗口类注册与窗口创建
// ====================================================================
void XPTabBandClass::RegisterBandClass()
{
    if (s_classAtom != 0)
        return;

    HMODULE hMod = GetModuleHandleW(NULL);  // 使用 EXE 模块（explorer.exe）

    // 先检查类是否已存在（可能由前一个 Explorer 实例注册，DLL 重载后 s_classAtom 被重置为 0）
    WNDCLASSEXW wcInfo = { sizeof(wcInfo) };
    if (GetClassInfoExW(hMod, kBandClassName, &wcInfo))
    {
        if (wcInfo.lpfnWndProc == XPTabBandClass::WndProcStatic)
        {
            // WndProc 匹配，直接复用已存在的类（用 1 表示类已可用）
            s_classAtom = 1;
            LogFmt(L"RegisterBandClass: 类已存在且 WndProc 匹配，复用");
            return;
        }
        // WndProc 不匹配（指向已卸载的旧 DLL），注销后重新注册
        LogFmt(L"RegisterBandClass: 检测到旧 WndProc=0x%p != 当前=0x%p，尝试注销",
               wcInfo.lpfnWndProc, XPTabBandClass::WndProcStatic);
        UnregisterClassW(kBandClassName, hMod);
    }

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = XPTabBandClass::WndProcStatic;
    wc.hInstance = hMod;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    // 使用暗色画刷作为背景，避免 TabBar 初始化前闪白
    wc.hbrBackground = CreateSolidBrush(RGB(30, 30, 30));
    wc.lpszClassName = kBandClassName;

    SetLastError(0);
    s_classAtom = RegisterClassExW(&wc);
    DWORD err = GetLastError();
    LogFmt(L"RegisterBandClass atom=%d err=%lu", s_classAtom, err);

    // 如果仍然失败，再尝试用 NULL hInstance 查找（兼容旧版本注册方式）
    if (s_classAtom == 0)
    {
        WNDCLASSEXW wcNull = { sizeof(wcNull) };
        if (GetClassInfoExW(NULL, kBandClassName, &wcNull) &&
            wcNull.lpfnWndProc == XPTabBandClass::WndProcStatic)
        {
            s_classAtom = 1;
            LogFmt(L"RegisterBandClass: 用 NULL hInstance 找到匹配类，复用");
        }
    }
}

void XPTabBandClass::CreateBandWindow(HWND hwndParent)
{
    RegisterBandClass();

    // 窗口类注册失败，不能创建窗口
    if (s_classAtom == 0)
    {
        LogFmt(L"CreateBandWindow: 窗口类未注册，跳过创建");
        m_hwnd = NULL;
        return;
    }

    // 创建子窗口
    // Explorer 会通过 ShowDW 控制显示
    m_hwnd = CreateWindowExW(
        0,
        kBandClassName,
        L"",
        WS_CHILD | WS_CLIPSIBLINGS,
        0, 0, 200, 30,
        hwndParent, NULL,
        GetModuleHandleW(NULL),
        this);

    if (m_hwnd)
    {
        // 存储 this 指针到窗口属性
        SetPropW(m_hwnd, L"XPTabBandThis", reinterpret_cast<HANDLE>(this));
        LogFmt(L"CreateBandWindow 成功 hwnd=%p", m_hwnd);
    }
    else
    {
        DWORD err = GetLastError();
        LogFmt(L"CreateBandWindow 失败 err=%lu", err);
    }
}

void XPTabBandClass::DestroyBandWindow()
{
    if (m_hwnd)
    {
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

LRESULT CALLBACK XPTabBandClass::WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    XPTabBandClass* pThis = nullptr;

    if (msg == WM_CREATE)
    {
        LPCREATESTRUCTW pcs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        pThis = reinterpret_cast<XPTabBandClass*>(pcs->lpCreateParams);
        SetPropW(hwnd, L"XPTabBandThis", reinterpret_cast<HANDLE>(pThis));
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    }
    else
    {
        pThis = reinterpret_cast<XPTabBandClass*>(
            GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    }

    if (pThis)
        return pThis->WndProc(hwnd, msg, wParam, lParam);

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT XPTabBandClass::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // 优先让 TabBarUI 处理消息
    // HandleMessage 对已处理的消息返回非 0 值
    if (m_pTabBar)
    {
        LRESULT result = m_pTabBar->HandleMessage(msg, wParam, lParam);
        if (result != 0)
            return result;
    }

    // TabBarUI 未处理的消息，走默认处理
    switch (msg)
    {
    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);
        static int lastLogW = -1, lastLogH = -1;
        if (width != lastLogW || height != lastLogH)
        {
            LogFmt(L"WM_SIZE w=%d h=%d TabBar=%p", width, height, m_pTabBar);
            lastLogW = width;
            lastLogH = height;
        }
        if (m_pTabBar)
        {
            m_pTabBar->OnSize(width, height);
        }
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ====================================================================
// 类厂
// ====================================================================
XPTabBandClassFactory::XPTabBandClassFactory()
    : m_cRef(1)
{
}

XPTabBandClassFactory::~XPTabBandClassFactory()
{
}

STDMETHODIMP XPTabBandClassFactory::QueryInterface(REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (riid == IID_IUnknown || riid == IID_IClassFactory)
        *ppv = static_cast<IClassFactory*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

STDMETHODIMP_(ULONG) XPTabBandClassFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

STDMETHODIMP_(ULONG) XPTabBandClassFactory::Release()
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0)
        delete this;
    return cRef;
}

STDMETHODIMP XPTabBandClassFactory::CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;
    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION;

    XPTabBandClass* pObj = new XPTabBandClass();
    if (!pObj)
        return E_OUTOFMEMORY;

    HRESULT hr = pObj->QueryInterface(riid, ppv);
    pObj->Release();
    return hr;
}

STDMETHODIMP XPTabBandClassFactory::LockServer(BOOL fLock)
{
    return S_OK;
}
