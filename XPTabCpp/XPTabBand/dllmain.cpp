// dllmain.cpp - XPTabBand DLL 入口点
//
// 实现 DllMain、DllRegisterServer、DllUnregisterServer、DllGetClassObject
// Explorer 通过 CoCreateInstance 加载此 DLL 创建 DeskBand

#include "stdafx.h"
#include "XPTabBandClass.h"
#include <cstdio>

// DLL 模块句柄
HMODULE g_hModule = NULL;

// COM 注册计数
static LONG g_cServerLocks = 0;

// ====================================================================
// DllMain
// ====================================================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

// ====================================================================
// DllGetClassObject - COM 类厂入口
// ====================================================================
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    if (!ppv)
        return E_POINTER;
    *ppv = NULL;

    if (rclsid != CLSID_XPTabBand)
        return CLASS_E_CLASSNOTAVAILABLE;

    XPTabBandClassFactory* pFactory = new XPTabBandClassFactory();
    if (!pFactory)
        return E_OUTOFMEMORY;

    HRESULT hr = pFactory->QueryInterface(riid, ppv);
    pFactory->Release();
    return hr;
}

// ====================================================================
// DllCanUnloadNow - COM 判断是否可卸载
// ====================================================================
STDAPI DllCanUnloadNow()
{
    return (g_cServerLocks == 0) ? S_OK : S_FALSE;
}

// ====================================================================
// 获取 DLL 路径
// ====================================================================
static std::wstring GetDllPath()
{
    wchar_t path[MAX_PATH] = { 0 };
    GetModuleFileNameW(g_hModule, path, MAX_PATH);
    return std::wstring(path);
}

// ====================================================================
// DllRegisterServer - 注册 COM 组件和 DeskBand
// ====================================================================
STDAPI DllRegisterServer()
{
    std::wstring dllPath = GetDllPath();
    if (dllPath.empty())
        return E_FAIL;

    // CLSID 字符串
    const wchar_t* kClsidStr = L"{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}";

    // 1. 注册 CLSID
    // HKCR\CLSID\{...} = "XPTabBand"
    LSTATUS hr = RegSetKeyValueW(HKEY_CLASSES_ROOT,
        (L"CLSID\\" + std::wstring(kClsidStr)).c_str(),
        NULL, REG_SZ, L"XPTabBand",
        (DWORD)(wcslen(L"XPTabBand") + 1) * sizeof(wchar_t));
    if (hr != ERROR_SUCCESS) return E_FAIL;

    // HKCR\CLSID\{...}\InprocServer32 = <dll路径>
    hr = RegSetKeyValueW(HKEY_CLASSES_ROOT,
        (L"CLSID\\" + std::wstring(kClsidStr) + L"\\InprocServer32").c_str(),
        NULL, REG_SZ, dllPath.c_str(),
        (DWORD)(dllPath.length() + 1) * sizeof(wchar_t));
    if (hr != ERROR_SUCCESS) return E_FAIL;

    // ThreadingModel = "Apartment"
    hr = RegSetKeyValueW(HKEY_CLASSES_ROOT,
        (L"CLSID\\" + std::wstring(kClsidStr) + L"\\InprocServer32").c_str(),
        L"ThreadingModel", REG_SZ, L"Apartment",
        (DWORD)(wcslen(L"Apartment") + 1) * sizeof(wchar_t));
    if (hr != ERROR_SUCCESS) return E_FAIL;

    // 2. 注册为 Explorer 工具栏 Band
    // HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar\{...} = "XPTabBand"
    hr = RegSetKeyValueW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Internet Explorer\\Toolbar",
        kClsidStr, REG_SZ, L"XPTabBand",
        (DWORD)(wcslen(L"XPTabBand") + 1) * sizeof(wchar_t));
    if (hr != ERROR_SUCCESS) return E_FAIL;

    // 3. 注册 Component Categories
    // CATID_InfoBand (垂直 Band): {00021493-0000-0000-C000-000000000046}
    std::wstring catSubkey = L"Component Categories\\{00021493-0000-0000-C000-000000000046}\\Implemented Categories\\"
        + std::wstring(kClsidStr);
    hr = RegSetKeyValueW(HKEY_CLASSES_ROOT, catSubkey.c_str(),
        NULL, REG_SZ, L"", sizeof(wchar_t));
    // 即使失败也继续（某些系统已存在该分类）

    return S_OK;
}

// ====================================================================
// DllUnregisterServer - 卸载注册
// ====================================================================
STDAPI DllUnregisterServer()
{
    const wchar_t* kClsidStr = L"{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}";
    LSTATUS hr;

    // 1. 删除 Toolbar 注册
    hr = RegDeleteKeyValueW(HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Internet Explorer\\Toolbar", kClsidStr);

    // 2. 删除 Component Categories
    std::wstring catPath = L"Component Categories\\{00021493-0000-0000-C000-000000000046}\\Implemented Categories";
    hr = RegDeleteKeyValueW(HKEY_CLASSES_ROOT, catPath.c_str(), kClsidStr);

    // 3. 删除 InprocServer32
    std::wstring inprocKey = L"CLSID\\" + std::wstring(kClsidStr) + L"\\InprocServer32";
    hr = RegDeleteTreeW(HKEY_CLASSES_ROOT, inprocKey.c_str());

    // 4. 删除 CLSID
    std::wstring clsidKey = L"CLSID\\" + std::wstring(kClsidStr);
    hr = RegDeleteTreeW(HKEY_CLASSES_ROOT, clsidKey.c_str());

    return S_OK;
}
