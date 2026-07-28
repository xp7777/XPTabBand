# XPTabBand 架构文档

本文档说明 XPTabBand 的技术架构和关键设计决策。

## 总体架构

```
┌──────────────────────────────────────────────────────────┐
│                     explorer.exe                         │
│  ┌────────────────────────────────────────────────────┐  │
│  │              Shell 主体（Rebar/Toolbar）             │  │
│  │  ┌──────────────────────────────────────────────┐  │  │
│  │  │         XPTabBand.dll（COM DeskBand）          │  │  │
│  │  │                                                │  │  │
│  │  │  ┌────────────────────────────────────────┐  │  │  │
│  │  │  │  XPTabBandClass                        │  │  │  │
│  │  │  │  - IDeskBand2  接口                    │  │  │  │
│  │  │  │  - IObjectWithSite  接口               │  │  │  │
│  │  │  │  - IPersistStream  接口                │  │  │  │
│  │  │  │  - IOleWindow  接口                    │  │  │  │
│  │  │  │  - GetBandInfo → 高度 30px              │  │  │  │
│  │  │  │  - SetSite → 获取 IWebBrowser2          │  │  │  │
│  │  │  │  - WndProc → 转发到 TabBarUI           │  │  │  │
│  │  │  └────────────────────────────────────────┘  │  │  │
│  │  │  ┌────────────────────────────────────────┐  │  │  │
│  │  │  │  TabBarUI                             │  │  │  │
│  │  │  │  - 标签列表 m_tabs                     │  │  │  │
│  │  │  │  - OnPaint（双缓冲）                   │  │  │  │
│  │  │  │  - HitTest（命中测试）                 │  │  │  │
│  │  │  │  - 收藏夹菜单                          │  │  │  │
│  │  │  │  - PIDL 工具函数                       │  │  │  │
│  │  │  └────────────────────────────────────────┘  │  │  │
│  │  └──────────────────────────────────────────────┘  │  │
│  │                                                      │
│  │  ┌──────────────────────────────────────────────┐  │  │
│  │  │         ShellTabWindowClass（系统）           │  │  │
│  │  │  - 文件列表视图                              │  │  │
│  │  │  - 树形导航                                  │  │  │
│  │  │  - 地址栏                                    │  │  │
│  │  └──────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────┘
                                ↑
                                │ IWebBrowser2 接口
                                │
┌──────────────────────────────────────────────────────────┐
│                XPTabBand.dll 内部                         │
│  - 通过 SetSite 获取 IUnknown                            │
│  - QueryService(SID_SWebBrowserApp) → IWebBrowser2       │
│  - 用于 Navigate2 / get_LocationName / PIDL 获取          │
└──────────────────────────────────────────────────────────┘
```

## COM 接口实现

### DeskBand 必需接口

| 接口 | 作用 | 关键方法 |
|------|------|---------|
| `IDeskBand2` | Band 对象基接口 | `GetBandInfo`（返回尺寸/标题） |
| `IObjectWithSite` | 站点注入 | `SetSite`（获取 Explorer 站点） |
| `IPersistStream` | 持久化 | `GetClassID`（返回 CLSID） |
| `IOleWindow` | 窗口接口 | `GetWindow`、`ContextSensitiveHelp` |
| `IDockingWindow` | 停靠窗口 | `ShowDW`、`CloseDW` |
| `IInputObject` | 输入对象 | `UIActivateIO`（焦点管理） |

### 关键流程

#### 1. 加载流程

```
explorer.exe 启动 → 读取注册表 HKCR\CLSID\{CLSID}\InprocServer32
                 → CoCreateInstance(CLSID_XPTabBand)
                 → QueryInterface(IID_IDeskBand2)
                 → QueryInterface(IID_IObjectWithSite)
                 → SetSite(IUnknown)
                   → QueryService(SID_SWebBrowserApp, IID_IWebBrowser2)
                   → 保存 m_pBrowser
                   → 创建 TabBarUI
                   → 初始化标签（获取当前文件夹 PIDL）
                 → GetBandInfo → 返回 30px 高度
                 → 创建 Band 窗口
                 → 显示在 Rebar 中
```

#### 2. 绘制流程

```
WM_PAINT
  ↓
TabBarUI::OnPaint
  ↓
BeginPaint(hdc)
  ↓
GetClientRect → width, height
  ↓
CreateCompatibleDC → memDc
CreateCompatibleBitmap → memBitmap
SelectObject(memDc, memBitmap)
  ↓
FillRect(memDc, bgBrush)         ← 暗色背景
DrawTopHighlight(memDc)           ← 顶部高光线
  ↓
for (每个 tab):
    DrawTabBackground(memDc)     ← 标签背景
    DrawTabTopBar(memDc)         ← 激活标签顶部蓝条
    DrawTabText(memDc)            ← 标签标题（DT_END_ELLIPSIS）
    DrawCloseButton(memDc)       ← × 关闭按钮
  ↓
DrawPlusButton(memDc)            ← + 按钮
DrawFavoriteButton(memDc)        ← ☆ 收藏按钮
  ↓
BitBlt(hdc, memDc)               ← ★ 关键：输出到屏幕
  ↓
SelectObject(memDc, oldBitmap)
DeleteDC(memDc)
DeleteObject(memBitmap)
  ↓
EndPaint
```

#### 3. 导航流程

```
用户点击标签
  ↓
HitTest → 确定点击位置
  ↓
ActivateTab(index)
  ↓
NavigateToPidl(m_pBrowser, pidl)
  ↓
IWebBrowser2::Navigate2(vPidl)
  → Explorer 切换文件夹
  → 触发 DocumentComplete 事件
  → 定时器检测到导航变化
  → 更新标签标题
```

### 特殊文件夹导航

普通文件夹用 `IWebBrowser2::Navigate2` 即可。但控制面板子项、网络连接等特殊文件夹会触发新窗口打开，解决方法：

```
TabBarUI::BrowseObjectPidl
  ↓
IShellBrowser* = m_pBrowser → QueryService(SID_SShellBrowser)
  ↓
尝试 3 种 SBSP 标志组合：
  1. SBSP_SAMEBROWSER | SBSP_ABSOLUTE | SBSP_EXPLOREMODE
  2. SBSP_SAMEBROWSER | SBSP_ABSOLUTE
  3. SBSP_ABSOLUTE | SBSP_EXPLOREMODE
  ↓
IShellBrowser::BrowseObject(pidl, flags)
  → 在当前 ShellView 内切换
```

## 数据结构

### TabItemUI

```cpp
struct TabItemUI {
    std::wstring title;    // 显示标题
    LPITEMIDLIST pidl;     // 文件夹 PIDL（深拷贝）
    bool active;           // 是否激活
    RECT rect;             // 绘制区域
};
```

### 收藏夹持久化

存储位置：`%APPDATA%\XPTabCpp\favorites.dat`

格式：
```
[Count: uint32]
[For each favorite:]
    [PidlSize: uint32]
    [PidlData: bytes]
    [TitleLength: uint32]
    [Title: wchar_t[]]
```

## 注册表结构

### CLSID 注册

```
HKCR\CLSID\{CLSID_XPTabBand}
    (默认) = "XPTabBand"
    InprocServer32
        (默认) = "C:\Path\To\XPTabBand.dll"
        ThreadingModel = "Apartment"
```

### Explorer 工具栏注册

```
HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar
    {CLSID_XPTabBand} = ""
```

### Component Categories

```
HKCR\Component Categories\{00021494-0000-0000-C000-000000000046}
    409 = "Deskband Objects"
```

## 关键设计决策

### 为什么不用 DLL 注入？

早期尝试过 DLL 注入方案（保留在 `XPTabHook/` 和 `XPTabInject/`），但存在问题：

1. **跨进程子类化不稳定**：`SetWindowLongPtr(HWND, GWLP_WNDPROC)` 跨进程时被 Windows 限制
2. **WH_CALLWNDPROC 钩子干扰窗口创建**：在 Explorer UI 线程处理消息时回调，导致"此电脑"双击无响应
3. **维护成本高**：需要处理 DLL 加载/卸载、窗口枚举、消息路由
4. **不稳定**：explorer.exe 重启后需要重新注入

**DeskBand 方案**的优势：
- Windows 原生支持的扩展机制
- 自动随 explorer.exe 加载/卸载
- 通过 COM 接口获取 IWebBrowser2，干净规范
- 出现问题时只是不显示，不会导致 Explorer 崩溃

### 为什么用双缓冲？

GDI 直接绘制到屏幕 DC 时，每个绘制操作都会立即显示，导致：
- 背景擦除和内容绘制之间出现白闪
- 多个绘制操作叠加产生重影
- 鼠标悬停时刷新不流畅

双缓冲流程：所有绘制到内存 DC → 一次性 `BitBlt` 到屏幕 DC → 无闪烁

### 为什么用 SEH 包装 COM 调用？

`IWebBrowser2` 接口在以下情况可能抛出异常：
- Explorer 正在关闭，COM 对象已部分释放
- 焦点切换时访问已失效的 ShellView
- 控制面板子项的 IShellBrowser 行为不一致

C++ 异常（try/catch）不能捕获 SEH 异常（如访问违例 0xc0000005）。必须用 `__try/__except` 包装关键 COM 调用，避免 Explorer 崩溃。

注意：SEH `__try/__except` 不能和 C++ 对象析构在同一函数中混用，所以 SEH 包装函数用纯 C 风格实现。

### 为什么收藏夹用 PIDL 而不是路径？

路径字符串有局限：
- 无法表示控制面板项、网络位置等虚拟文件夹
- 盘符变化、文件夹移动会导致路径失效
- Unicode/长路径处理复杂

PIDL（Pointer to ITEMIDLIST）是 Shell 内部的文件夹标识，可以表示任何 Shell 命名空间对象，包括虚拟文件夹。持久化时保存为二进制，加载时用 `ILClone` 还原。

## 调试

### 日志输出

关键路径都加了日志输出到 `%LOCALAPPDATA%\Temp\XPTabBand_log.txt`：

- `GetBandInfo` 调用
- `SetSite` 流程
- `OnPaint` 调用频率限制（1 秒 1 条）
- 标签创建/激活/关闭
- 收藏夹操作
- `BrowseObject` 尝试和 HRESULT

### Visual Studio 调试

1. 项目属性 → Debugging：
   - Command = `explorer.exe`
   - Attach = `Yes`
2. F5 启动，VS 启动 explorer.exe 并附加到 XPTabBand.dll
3. 可以在 `OnPaint`、`HitTest`、`ActivateTab` 等函数设断点

### 重新注册 DLL（开发期间）

```cmd
:: 卸载旧的（避免文件锁定）
regsvr32 /u XPTabBand.dll

:: 编译新的
MSBuild XPTab.sln /p:Configuration=Debug /p:Platform=x64 /t:Rebuild

:: 注册新的
regsvr32 XPTabBand.dll

:: 重启 Explorer（让新 DLL 生效）
taskkill /f /im explorer.exe && start explorer.exe
```

## 已知限制

1. **占用 30px 高度**：标签栏占用资源管理器顶部 30 像素，挤占 Ribbon 空间
2. **部分控制面板项仍会弹新窗口**：`BrowseObject` 对某些 GUID 路径仍会触发新窗口
3. **仅 x64**：未编译 32 位版本
4. **无数字签名**：首次运行可能被 SmartScreen 拦截
5. **不兼容 IE 模式**：在 IE 兼容视图中不显示

## 性能考量

- **定时器间隔**：
  - `kCheckIntervalMs = 1000ms`：位置修正定时器
  - `kNavCheckIntervalMs = 2000ms`：导航变化检测
  - 过短会干扰 Explorer 内部重绘，过长则响应迟钝
- **日志频率限制**：`OnPaint` 每秒最多记录 1 条，避免日志爆炸
- **PIDL 缓存**：标签 PIDL 在标签创建时拷贝一次，切换标签时不重新获取
- **双缓冲位图**：每次 `OnPaint` 都创建新位图，未缓存。如果性能不足可考虑复用

## 扩展点

如果要添加新功能，参考以下位置：

| 功能 | 修改位置 |
|------|---------|
| 新增标签右键菜单 | `TabBarUI::HandleMessage` 的 `WM_RBUTTONDOWN` 分支 |
| 标签拖拽排序 | `TabBarUI::HandleMessage` 的 `WM_MOUSEMOVE` + 自定义拖拽逻辑 |
| 主题切换 | `TabBarUI` 的颜色常量 + 配置文件读取 |
| 键盘快捷键 | `TabBarUI::HandleMessage` 的 `WM_KEYDOWN` |
| 配置文件 | 新增 `ConfigManager` 类，在 `Initialize` 中加载 |
| 多标签独立 ShellView | 阶段 3 实现，需要 `IShellView::CreateViewWindow` + `SetParent` |
