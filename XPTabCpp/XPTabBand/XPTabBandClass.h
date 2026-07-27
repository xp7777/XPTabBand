// XPTabBandClass.h - DeskBand COM 组件定义
//
// 实现 IDeskBand2 + IObjectWithSite + IPersistStream 接口
// 被 Explorer 作为工具栏 Band 加载
//
// 加载流程：
//   1. Explorer 通过 CoCreateInstance 创建 CLSID_XPTabBand 实例
//   2. QueryInterface(IID_IPersistStream) 加载持久化设置
//   3. QueryInterface(IID_IObjectWithSite) 传入 IUnknown 站点
//   4. 通过站点获取 IShellBrowser -> IWebBrowser2
//   5. QueryInterface(IID_IDeskBand) 获取尺寸并显示

#pragma once
#include "stdafx.h"
#include <exdisp.h>

// {A1B2C3D4-1234-4ABC-9DEF-1234567890AB} - XPTabBand CLSID
// 定义在注册表中，Explorer 通过此 CLSID 加载 Band
static const CLSID CLSID_XPTabBand =
{ 0xa1b2c3d4, 0x1234, 0x4abc, { 0x9d, 0xef, 0x12, 0x34, 0x56, 0x78, 0x90, 0xab } };

// 前向声明
class TabBarUI;

class XPTabBandClass :
    public IObjectWithSite,
    public IDeskBand2,
    public IPersistStream
{
public:
    XPTabBandClass();
    ~XPTabBandClass();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IObjectWithSite
    STDMETHODIMP SetSite(IUnknown* pUnkSite) override;
    STDMETHODIMP GetSite(REFIID riid, void** ppvSite) override;

    // IOleWindow (IDeskBand 的基接口)
    STDMETHODIMP GetWindow(HWND* phwnd) override;
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) override;

    // IDockingWindow (IDeskBand 的基接口)
    STDMETHODIMP CloseDW(DWORD dwReserved) override;
    STDMETHODIMP ResizeBorderDW(LPCRECT prcBorder, IUnknown* punkToolbarSite, BOOL f) override;
    STDMETHODIMP ShowDW(BOOL fShow) override;

    // IDeskBand
    STDMETHODIMP GetBandInfo(DWORD dwBandID, DWORD dwViewMode, DESKBANDINFO* pdbi) override;

    // IDeskBand2
    STDMETHODIMP CanRenderComposited(BOOL* pfCanRenderComposited) override;
    STDMETHODIMP SetCompositionState(BOOL fCompositionEnabled) override;
    STDMETHODIMP GetCompositionState(BOOL* pfCompositionEnabled) override;

    // IPersist
    STDMETHODIMP GetClassID(CLSID* pClassID) override;

    // IPersistStream
    STDMETHODIMP IsDirty() override;
    STDMETHODIMP Load(IStream* pStm) override;
    STDMETHODIMP Save(IStream* pStm, BOOL fClearDirty) override;
    STDMETHODIMP GetSizeMax(ULARGE_INTEGER* pcbSize) override;

private:
    LONG m_cRef;              // 引用计数
    HWND m_hwndParent;        // Explorer 窗口句柄
    HWND m_hwnd;              // Band 自身窗口句柄
    DWORD m_dwBandID;         // Band ID
    DWORD m_dwViewMode;       // 视图模式
    BOOL m_bCompositionEnabled;
    BOOL m_bShow;
    IUnknown* m_pSite;        // Explorer 站点
    IWebBrowser2* m_pBrowser; // Explorer 浏览器接口
    TabBarUI* m_pTabBar;      // 标签栏 UI

    // 注册窗口类
    static ATOM s_classAtom;
    static const wchar_t* kBandClassName;
    static LRESULT CALLBACK WndProcStatic(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    void RegisterBandClass();
    void CreateBandWindow(HWND hwndParent);
    void DestroyBandWindow();
};

// 类厂
class XPTabBandClassFactory : public IClassFactory
{
public:
    XPTabBandClassFactory();
    ~XPTabBandClassFactory();

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    // IClassFactory
    STDMETHODIMP CreateInstance(IUnknown* pUnkOuter, REFIID riid, void** ppv) override;
    STDMETHODIMP LockServer(BOOL fLock) override;

private:
    LONG m_cRef;
};
