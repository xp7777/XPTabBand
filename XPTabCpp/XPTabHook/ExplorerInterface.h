#pragma once

// ExplorerInterface.h - 封装与 Explorer 的交互
//
// 通过 IShellWindows 枚举所有打开的 Explorer 窗口，匹配 HWND 获取 IWebBrowser2，
// 进而读取当前路径、导航到指定 PIDL、监听导航完成事件。
//
// 关键 COM 接口：
//   IShellWindows  - 枚举所有 Explorer 窗口（Shell.Application 的 Windows 集合）
//   IWebBrowser2   - 控制 Explorer 导航（Navigate2 接受 PIDL VARIANT）
//   IPersistIDList - 从 IShellView 获取当前文件夹 PIDL

#include "stdafx.h"
#include <exdisp.h>      // IShellWindows, IWebBrowser2
#include <exdispid.h>    // DISPID_NAVIGATECOMPLETE2 等事件 dispid
#include <shlobj.h>

namespace ExplorerInterface
{
    // 初始化 COM（工作线程调用，STA 单元）
    // 返回 true 表示成功或已初始化
    bool InitializeCom();

    // 反初始化 COM
    void UninitializeCom();

    // 根据 Explorer 主窗口句柄（CabinetWClass）查找对应的 IWebBrowser2
    // 内部通过 IShellWindows 枚举所有窗口并匹配 HWND
    // 返回 IWebBrowser2 指针（调用者负责 Release），失败返回 NULL
    IWebBrowser2* FindWebBrowserByHwnd(HWND hwnd);

    // 获取 IWebBrowser2 当前显示文件夹的 PIDL
    // 调用者负责用 ILFree 释放返回的 PIDL，失败返回 NULL
    // 注意：此方法用 LocationURL，对特殊文件夹（如此电脑、控制面板）不可靠
    LPITEMIDLIST GetCurrentPidl(IWebBrowser2* pBrowser);

    // 获取当前文件夹 PIDL（增强版）
    // 优先用 IShellBrowser->QueryActiveShellView->GetItemObject 获取真实 PIDL
    // 对特殊文件夹（此电脑、控制面板）也能正确返回
    // 调用者负责 ILFree
    LPITEMIDLIST GetCurrentPidlEx(IWebBrowser2* pBrowser);

    // 获取当前文件夹的显示名称（用于标签标题）
    // 失败返回空字符串
    std::wstring GetCurrentFolderName(IWebBrowser2* pBrowser);

    // 导航到指定 PIDL
    // 返回 true 表示导航请求已提交（实际完成是异步的）
    bool NavigateToPidl(IWebBrowser2* pBrowser, LPCITEMIDLIST pidl);

    // 复制 PIDL（深拷贝，调用者负责 ILFree）
    LPITEMIDLIST CopyPidl(LPCITEMIDLIST pidl);

    // 从 PIDL 获取显示名称（友好名称，用于标签标题）
    std::wstring GetNameFromPidl(LPCITEMIDLIST pidl);

    // 获取特殊文件夹的 PIDL（CSIDL_DRIVES=此电脑，CSIDL_CONTROLS=控制面板等）
    // 调用者负责 ILFree
    LPITEMIDLIST GetSpecialFolderPidl(int csidl);

    // 从字符串路径或 GUID 路径创建 PIDL
    // 支持普通路径 "C:\Windows" 和 GUID 路径 "::{CLSID}" 或 "::{CLSID}\子项"
    // 调用者负责 ILFree
    LPITEMIDLIST CreatePidlFromPath(const std::wstring& path);

    // 判断 PIDL 是否为特殊文件夹（无文件系统路径，如此电脑、控制面板）
    bool IsSpecialFolderPidl(LPCITEMIDLIST pidl);
}
