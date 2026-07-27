// TabBarUI.h - 标签栏 UI（DeskBand 版本）
//
// 在 DeskBand 窗口内绘制标签栏：标签（标题+关闭按钮×）+ + 按钮 + ☆ 收藏按钮
// 与旧版 TabBarWindow 的区别：
//   - 不再创建独立窗口，直接在 DeskBand 的 m_hwnd 上绘制
//   - 不再移动 ShellTabWindowClass（解决重影问题）
//   - 通过 SetSite 获取 IWebBrowser2，不需要 FindWebBrowserByHwnd
//   - 单标签模式下使用 Navigate2 切换（阶段 3 再实现 SetParent 多标签）

#pragma once
#include "stdafx.h"
#include <exdisp.h>

// 单个标签页数据
struct TabItemUI
{
    std::wstring title;   // 标签标题
    LPITEMIDLIST pidl;    // 文件夹 PIDL（深拷贝）
    bool active;          // 是否激活
    RECT rect;            // 绘制区域

    TabItemUI() : pidl(NULL), active(false)
    {
        rect = { 0, 0, 0, 0 };
    }
};

// 收藏夹项
struct FavoriteItem
{
    std::wstring title;   // 显示名称
    LPITEMIDLIST pidl;    // 文件夹 PIDL（深拷贝，CoTaskMemAlloc 分配）

    FavoriteItem() : pidl(NULL) {}
};

class TabBarUI
{
public:
    TabBarUI();
    ~TabBarUI();

    // 初始化：绑定到 DeskBand 窗口和 Explorer 浏览器接口
    bool Initialize(HWND hwndBand, IWebBrowser2* pBrowser);
    void Uninitialize();

    // 窗口消息处理（由 DeskBand 的 WndProc 调用）
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    // 定时器回调（由 DeskBand 的 WM_TIMER 调用）
    void OnTimerTick();

    // 窗口尺寸变化时调用
    void OnSize(int width, int height);

    // PIDL 辅助（public，供 SEH 包装函数调用）
    static LPITEMIDLIST CopyPidl(LPCITEMIDLIST pidl);
    static std::wstring GetNameFromPidl(LPCITEMIDLIST pidl);
    static LPITEMIDLIST GetCurrentPidlEx(IWebBrowser2* pBrowser);
    static LPITEMIDLIST GetSpecialFolderPidl(int csidl);
    static bool NavigateToPidl(IWebBrowser2* pBrowser, LPCITEMIDLIST pidl);
    static bool BrowseObjectPidl(IWebBrowser2* pBrowser, LPCITEMIDLIST pidl);
    static std::wstring GetCurrentFolderName(IWebBrowser2* pBrowser);

private:
    HWND m_hwnd;                  // DeskBand 窗口句柄
    IWebBrowser2* m_pBrowser;     // Explorer 浏览器接口（不拥有，不 Release）
    std::vector<TabItemUI> m_tabs;
    int m_activeIndex;
    int m_tabWidth;
    int m_windowWidth;
    int m_windowHeight;
    DWORD m_lastCheckTick;

    // 鼠标悬停状态
    int m_hoverTab;     // 鼠标悬停的标签索引（-1 表示无）
    int m_hoverButton;  // 鼠标悬停的按钮（HIT_*）

    // 收藏夹
    std::vector<FavoriteItem> m_favorites;
    bool m_favoritesLoaded;

    // 命中测试常量
    static const int HIT_NONE = -3;
    static const int HIT_PLUS = -1;
    static const int HIT_CLOSE = -2;
    static const int HIT_FAVORITE = -4;

    // UI 布局常量
    static const int kTabBarHeight = 30;
    static const int kDefaultTabWidth = 150;
    static const int kPlusButtonWidth = 28;
    static const int kFavoriteButtonWidth = 28;
    static const int kCloseButtonWidth = 20;
    static const int kTextPadding = 8;

    // 暗色主题颜色
    static const COLORREF kColorBg;
    static const COLORREF kColorTabActive;
    static const COLORREF kColorTabInactive;
    static const COLORREF kColorTabHover;
    static const COLORREF kColorText;
    static const COLORREF kColorClose;
    static const COLORREF kColorCloseActive;
    static const COLORREF kColorPlus;
    static const COLORREF kColorSeparator;

    // 定时器
    static const UINT_PTR kTabBarTimerId = 1001;
    static const DWORD kCheckIntervalMs = 1000;

    // 方法
    bool TryAcquireBrowser();
    void AddTab(LPCITEMIDLIST pidl, const std::wstring& title);
    void CloseTab(int index);
    void ActivateTab(int index);
    void FreeAllPidls();
    void UpdateTabRects();
    int HitTest(int x, int y, int* outTabIndex);
    void OnPaint();
    void CreateInitialTab();

    // 收藏夹方法
    void LoadFavorites();
    void SaveFavorites();
    void AddCurrentToFavorites();
    void ShowFavoritesMenu(int screenX, int screenY);
    void FreeAllFavoritePidls();
    static std::wstring GetFavoritesFilePath();
};
