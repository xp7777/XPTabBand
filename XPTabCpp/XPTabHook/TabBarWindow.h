#pragma once

// TabBarWindow.h - 标签页栏窗口声明
// 创建一个自定义窗口类 "XPTabBarClass" 作为标签栏，
// 绘制标签页（标题 + 关闭按钮 ×）和右侧的 + 按钮
//
// 每个 TabBarWindow 对应一个 Explorer 窗口，持有一个 IWebBrowser2 指针
// 用于控制导航。每个 TabItem 存储一个 PIDL 表示该标签的文件夹路径。

#include "stdafx.h"
#include <exdisp.h>

// 单个标签页数据
struct TabItem
{
    std::wstring title;   // 标签标题（文件夹显示名）
    LPITEMIDLIST pidl;    // 该标签对应的文件夹 PIDL（深拷贝，TabBarWindow 负责释放）
    bool active;          // 是否为当前激活标签
    RECT rect;            // 标签的绘制区域（点击检测用）

    TabItem() : pidl(NULL), active(false)
    {
        rect = { 0, 0, 0, 0 };
    }
};

class TabBarWindow
{
public:
    TabBarWindow();
    ~TabBarWindow();

    // 创建标签栏窗口
    // hParent: 父窗口（Explorer 主窗口）句柄
    bool Create(HWND hParent);

    // 销毁标签栏窗口
    void Destroy();

    // 调整标签栏大小（参数为 Explorer 客户区宽高）
    void Resize(int width, int height);

    // 根据 Explorer 窗口位置同步 TabBar 位置（顶层窗口方案）
    // 将 TabBar 定位在 Explorer 窗口顶部上方 30 像素处
    void UpdatePosition();

    // 获取窗口句柄
    HWND GetHwnd() const { return m_hwnd; }

    // 窗口过程（静态，转发到实例，需 public 以便注册窗口类）
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // 注销标签栏窗口类（DLL 卸载时调用，避免残留失效的 WndProc 指针）
    static void UnregisterWindowClass();

    // 定时器回调：检查导航变化并更新当前标签
    // 由 ExplorerHook 的 GetMsgProc 或外部定时器调用
    void OnTimerTick();

private:
    HWND m_hwnd;                  // 标签栏窗口句柄
    std::vector<TabItem> m_tabs;  // 标签页列表
    int m_activeIndex;            // 当前激活标签索引
    int m_tabWidth;               // 单个标签宽度
    int m_windowWidth;            // 窗口总宽度
    int m_windowHeight;           // 窗口总高度

    // Explorer 浏览器接口（控制导航）
    IWebBrowser2* m_pBrowser;
    HWND m_hExplorer;             // 父 Explorer 窗口句柄
    DWORD m_lastCheckTick;        // 上次检查导航变化的时间戳

    // ShellTabWindowClass 原始矩形（客户区坐标）
    // 关键：UpdatePosition 会反复调用，如果每次都读取"当前"ShellTab 高度再减 30，
    // 高度会越来越小（600→570→540→...→50）。必须记录原始高度，用原始值计算。
    // 当 Explorer 窗口大小变化（WM_SIZE）时，会重新记录。
    RECT m_originalShellTabRect;  // left/top/width/height（客户区坐标）
    bool m_shellTabRectValid;     // 原始 rect 是否已记录
    HWND m_lastShellTabHwnd;      // 上次找到的 ShellTab 窗口句柄（用于检测变化）

    // 初始化：尝试获取 IWebBrowser2
    bool TryAcquireBrowser();

    // 新增标签页（pidl 为深拷贝，所有权转移给 TabItem）
    void AddTab(LPCITEMIDLIST pidl, const std::wstring& title);

    // 关闭指定索引的标签页
    void CloseTab(int index);

    // 激活指定索引的标签页（并导航到其 pidl）
    void ActivateTab(int index);

    // 释放所有标签的 PIDL
    void FreeAllPidls();

    // 计算各标签的矩形区域
    void UpdateTabRects();

    // 命中测试：判断点击位置属于哪个标签或按钮
    // 返回值：>=0 为标签索引，-1 为 + 按钮，-2 为关闭按钮（需配合 tabIndex），-3 为无
    int HitTest(int x, int y, int* outTabIndex);

    // 处理窗口消息
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    // 绘制标签栏
    void OnPaint();
};
