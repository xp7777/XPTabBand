// ExplorerInterface.cpp - Explorer 交互封装实现

#include "stdafx.h"
#include "ExplorerInterface.h"
#include "Utils.h"

namespace ExplorerInterface
{
    // COM 初始化状态（每个线程独立）
    static bool g_comInitialized = false;

    bool InitializeCom()
    {
        if (g_comInitialized)
            return true;
        // 使用 STA 单元，与 Explorer 的 COM 模型一致
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (SUCCEEDED(hr))
        {
            g_comInitialized = true;
            return true;
        }
        return false;
    }

    void UninitializeCom()
    {
        if (g_comInitialized)
        {
            CoUninitialize();
            g_comInitialized = false;
        }
    }

    IWebBrowser2* FindWebBrowserByHwnd(HWND hwnd)
    {
        if (!hwnd)
            return NULL;

        IShellWindows* pShellWindows = NULL;
        HRESULT hr = CoCreateInstance(CLSID_ShellWindows, NULL, CLSCTX_LOCAL_SERVER,
                                      IID_PPV_ARGS(&pShellWindows));
        if (FAILED(hr) || !pShellWindows)
        {
            return NULL;
        }

        IWebBrowser2* pResult = NULL;
        long count = 0;
        pShellWindows->get_Count(&count);

        for (long i = 0; i < count; i++)
        {
            VARIANT vIndex;
            VariantInit(&vIndex);
            vIndex.vt = VT_I4;
            vIndex.lVal = i;

            IDispatch* pDisp = NULL;
            hr = pShellWindows->Item(vIndex, &pDisp);
            VariantClear(&vIndex);

            if (FAILED(hr) || !pDisp)
                continue;

            IWebBrowser2* pBrowser = NULL;
            hr = pDisp->QueryInterface(IID_PPV_ARGS(&pBrowser));
            pDisp->Release();

            if (FAILED(hr) || !pBrowser)
                continue;

            // 获取浏览器窗口句柄并匹配
            SHANDLE_PTR hwndBrowser = 0;
            hr = pBrowser->get_HWND(&hwndBrowser);
            if (SUCCEEDED(hr) && (HWND)hwndBrowser == hwnd)
            {
                pResult = pBrowser; // 调用者负责 Release
                break;
            }
            pBrowser->Release();
        }

        pShellWindows->Release();
        return pResult;
    }

    LPITEMIDLIST GetCurrentPidl(IWebBrowser2* pBrowser)
    {
        if (!pBrowser)
            return NULL;

        // 方法：通过 IShellBrowser->QueryActiveShellView->GetItemObject
        // 但 IWebBrowser2 不直接暴露 IShellBrowser。
        // 替代方案：用 LocationURL 获取路径，再 ILCreateFromPathW 转 PIDL。
        // 这个方案对普通文件系统路径有效，对特殊文件夹（如"此电脑"）可能失败。
        BSTR bstrUrl = NULL;
        HRESULT hr = pBrowser->get_LocationURL(&bstrUrl);
        if (FAILED(hr) || !bstrUrl)
            return NULL;

        std::wstring url(bstrUrl);
        SysFreeString(bstrUrl);

        // URL 形如 file:///C:/Users/...
        // 转换为本地路径
        wchar_t localPath[MAX_PATH] = { 0 };
        DWORD pathLen = MAX_PATH;
        if (PathCreateFromUrlW(url.c_str(), localPath, &pathLen, 0) != S_OK)
        {
            return NULL;
        }

        LPITEMIDLIST pidl = ILCreateFromPathW(localPath);
        return pidl;
    }

    // 获取当前文件夹 PIDL（增强版）
    // 通过 IShellBrowser->QueryActiveShellView->GetItemObject(SVGIO_BACKGROUND) 获取真实 PIDL
    // 对特殊文件夹（此电脑、控制面板）也能正确返回
    LPITEMIDLIST GetCurrentPidlEx(IWebBrowser2* pBrowser)
    {
        if (!pBrowser)
            return NULL;

        // IWebBrowser2 实现 IOleWindow 接口，可通过它获取 IShellBrowser
        // 实际上 IWebBrowser2 内部就是 IShellBrowser 的包装
        // 这里用 QueryService 获取 IShellBrowser
        IServiceProvider* pSvc = NULL;
        HRESULT hr = pBrowser->QueryInterface(IID_PPV_ARGS(&pSvc));
        if (FAILED(hr) || !pSvc)
        {
            // 回退到 GetCurrentPidl
            return GetCurrentPidl(pBrowser);
        }

        IShellBrowser* pShellBrowser = NULL;
        hr = pSvc->QueryService(SID_SShellBrowser, IID_PPV_ARGS(&pShellBrowser));
        pSvc->Release();
        if (FAILED(hr) || !pShellBrowser)
        {
            return GetCurrentPidl(pBrowser);
        }

        IShellView* pView = NULL;
        hr = pShellBrowser->QueryActiveShellView(&pView);
        pShellBrowser->Release();
        if (FAILED(hr) || !pView)
        {
            return GetCurrentPidl(pBrowser);
        }

        // 通过 GetItemObject(SVGIO_BACKGROUND) 获取当前文件夹 PIDL
        // IFolderView 提供当前文件夹 PIDL
        IFolderView* pFolderView = NULL;
        hr = pView->QueryInterface(IID_PPV_ARGS(&pFolderView));
        if (FAILED(hr) || !pFolderView)
        {
            pView->Release();
            return GetCurrentPidl(pBrowser);
        }

        LPITEMIDLIST pidl = NULL;
        // IFolderView::GetFolder 签名：HRESULT GetFolder([in] REFIID, [out] void**)
        // 用 IID_IPersistIDList 直接获取 PIDL
        IPersistIDList* pPersist = NULL;
        hr = pFolderView->GetFolder(IID_PPV_ARGS(&pPersist));
        pFolderView->Release();
        pView->Release();
        if (FAILED(hr) || !pPersist)
        {
            return GetCurrentPidl(pBrowser);
        }

        hr = pPersist->GetIDList(&pidl);
        pPersist->Release();
        if (FAILED(hr) || !pidl)
        {
            return GetCurrentPidl(pBrowser);
        }

        return pidl;
    }

    std::wstring GetCurrentFolderName(IWebBrowser2* pBrowser)
    {
        if (!pBrowser)
            return L"";

        // 优先用 GetCurrentPidlEx + GetNameFromPidl，对特殊文件夹更准确
        LPITEMIDLIST pidl = GetCurrentPidlEx(pBrowser);
        if (pidl)
        {
            std::wstring name = GetNameFromPidl(pidl);
            ILFree(pidl);
            if (!name.empty())
                return name;
        }

        // 回退到 LocationName
        BSTR bstrName = NULL;
        HRESULT hr = pBrowser->get_LocationName(&bstrName);
        if (FAILED(hr) || !bstrName)
            return L"";

        std::wstring name(bstrName);
        SysFreeString(bstrName);
        return name;
    }

    bool NavigateToPidl(IWebBrowser2* pBrowser, LPCITEMIDLIST pidl)
    {
        if (!pBrowser || !pidl)
            return false;

        // Navigate2 接受 VARIANT，可以是 PIDL 的 SAFEARRAY
        // 构造包含 PIDL 字节的 SAFEARRAY
        int cbPidl = ILGetSize(pidl);
        if (cbPidl <= 0)
            return false;

        // 创建一字节数组
        SAFEARRAY* psa = SafeArrayCreateVector(VT_UI1, 0, cbPidl);
        if (!psa)
            return false;

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

        // 导航标志：不写入历史、不读历史
        VARIANT vFlags;
        VariantInit(&vFlags);
        vFlags.vt = VT_I4;
        vFlags.lVal = 0; // 0 表示默认

        HRESULT hr = pBrowser->Navigate2(&vPidl, &vFlags, NULL, NULL, NULL);

        VariantClear(&vPidl);  // 会销毁 psa
        VariantClear(&vFlags);

        return SUCCEEDED(hr);
    }

    LPITEMIDLIST CopyPidl(LPCITEMIDLIST pidl)
    {
        if (!pidl)
            return NULL;
        return ILClone(pidl);
    }

    std::wstring GetNameFromPidl(LPCITEMIDLIST pidl)
    {
        if (!pidl)
            return L"";

        SHFILEINFOW sfi = { 0 };
        // SHGFI_DISPLAYNAME 获取显示名称
        if (SHGetFileInfoW(reinterpret_cast<LPCWSTR>(pidl), 0, &sfi, sizeof(sfi),
                           SHGFI_PIDL | SHGFI_DISPLAYNAME))
        {
            return sfi.szDisplayName;
        }

        // 回退：从 PIDL 路径提取
        wchar_t path[MAX_PATH] = { 0 };
        if (SHGetPathFromIDListW(pidl, path))
        {
            // 取最后一段
            std::wstring p(path);
            size_t pos = p.find_last_of(L"\\/");
            if (pos != std::wstring::npos)
                return p.substr(pos + 1);
            return p;
        }

        return L"";
    }

    // 获取特殊文件夹的 PIDL
    LPITEMIDLIST GetSpecialFolderPidl(int csidl)
    {
        LPITEMIDLIST pidl = NULL;
        HRESULT hr = SHGetFolderLocation(NULL, csidl, NULL, 0, &pidl);
        if (FAILED(hr))
        {
            // 回退到 SHGetSpecialFolderLocation（旧 API）
            hr = SHGetSpecialFolderLocation(NULL, csidl, &pidl);
            if (FAILED(hr))
                return NULL;
        }
        return pidl;
    }

    // 从字符串路径或 GUID 路径创建 PIDL
    // 支持：
    //   - 普通路径 "C:\Windows"
    //   - GUID 路径 "::{CLSID}"（如此电脑 ::{20D04FE0-3AEA-1069-A2D8-08002B30309D}）
    //   - 复合路径 "::{CLSID}\0\::{CLSID}"（如控制面板\网络连接）
    LPITEMIDLIST CreatePidlFromPath(const std::wstring& path)
    {
        if (path.empty())
            return NULL;

        // 优先用 SHParseDisplayName（对 GUID 路径和普通路径都支持）
        LPITEMIDLIST pidl = NULL;
        HRESULT hr = SHParseDisplayName(path.c_str(), NULL, &pidl, 0, NULL);
        if (SUCCEEDED(hr) && pidl)
            return pidl;

        // 回退到 ILCreateFromPathW（仅文件系统路径）
        LPITEMIDLIST pidl2 = ILCreateFromPathW(path.c_str());
        return pidl2;
    }

    // 判断 PIDL 是否为特殊文件夹（无文件系统路径）
    bool IsSpecialFolderPidl(LPCITEMIDLIST pidl)
    {
        if (!pidl)
            return false;

        // SHGetPathFromIDListW 对特殊文件夹返回 FALSE
        wchar_t path[MAX_PATH] = { 0 };
        BOOL ret = SHGetPathFromIDListW(pidl, path);
        return (ret == FALSE || path[0] == L'\0');
    }
}
