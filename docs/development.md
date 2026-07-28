# 开发指南

面向想参与 XPTabBand 开发的贡献者。本文档覆盖环境搭建、调试技巧、代码规范和常见开发任务。

## 开发环境

### 必需工具

| 工具 | 版本 | 用途 |
| ---- | ---- | ---- |
| Visual Studio 2022 | 17.x（Community 即可） | C++ 编译，需勾选"使用 C++ 的桌面开发"工作负载 |
| Windows SDK | 10.0.19041.0 或更高 | Shell API 头文件 |
| Git | 2.30+ | 版本控制 |
| Inno Setup 6 | 6.x（可选） | 打包 setup.exe |

### 推荐工具

- **Visual Studio Code**：写文档和提交信息
- [**Dependencies**](https://github.com/lucasg/Dependencies)：检查 DLL 依赖
- [**Sysinternals Process Explorer**](https://learn.microsoft.com/sysinternals/)：查看 explorer.exe 加载的 DLL
- [**WinDbg Preview**](https://learn.microsoft.com/windows-hardware/drivers/debugger/)：分析崩溃 dump

### 克隆与首次编译

```cmd
git clone https://github.com/<你的用户名>/XPTabBand.git
cd XPTabBand
git remote add upstream https://github.com/xp7777/XPTabBand.git
```

编译前**必须先卸载已注册的 DLL**，否则文件被 explorer.exe 锁定导致 LNK1104 错误：

```cmd
cd XPTabCpp\deploy
uninstall.bat
:: 或手动 regsvr32 /u
```

编译：

```cmd
cd XPTabCpp
MSBuild XPTab.sln /p:Configuration=Debug /p:Platform=x64 /t:Rebuild
```

输出：`XPTabCpp\XPTabBand\build\Debug\XPTabBand.dll`

注册并重启 Explorer：

```cmd
regsvr32 XPTabCpp\XPTabBand\build\Debug\XPTabBand.dll
taskkill /f /im explorer.exe && start explorer.exe
```

## 项目结构详解

```
XPTabCpp/
├── XPTab.sln                    VS 解决方案
├── XPTabBand/                   ★ 主项目（DeskBand COM 组件）
│   ├── XPTabBand.vcxproj        VS 项目文件
│   ├── XPTabBandClass.h/.cpp    COM 接口实现
│   ├── TabBarUI.h/.cpp           标签栏 UI 和业务逻辑
│   ├── dllmain.cpp              DLL 入口 + DllRegisterServer
│   ├── stdafx.h/.cpp            预编译头
│   ├── XPTabBand.def            模块定义（导出 DllRegisterServer 等）
│   └── build/                   编译输出（在 .gitignore 中）
├── XPTabHook/                   早期 DLL 注入方案（已弃用，保留参考）
├── XPTabInject/                 早期注入器（已弃用）
├── MinHook/                     MinHook 库源码（用于早期方案）
├── deploy/                      简易部署包
│   ├── XPTabBand.dll            当前版本 DLL
│   ├── install.bat              注册脚本
│   └── uninstall.bat            注销脚本
├── installer/                   Inno Setup 安装项目
│   └── XPTabBand.iss            安装脚本
└── build.bat                    一键构建脚本
```

核心代码集中在 [XPTabBandClass.cpp](../XPTabCpp/XPTabBand/XPTabBandClass.cpp) 和 [TabBarUI.cpp](../XPTabCpp/XPTabBand/TabBarUI.cpp)。架构详见 [architecture.md](architecture.md)。

## 调试

### 设置 Visual Studio 调试目标

DeskBand DLL 被 explorer.exe 加载，所以需要把 explorer.exe 设为调试目标：

1. 右键项目 → 属性 → 调试
2. Command = `C:\Windows\explorer.exe`
3. Attach = `Yes`
4. F5 启动

VS 会启动一个新的 explorer.exe 实例并附加调试器。注意这会启动新的桌面进程，看到的窗口可能不是你平常用的那个。

### 附加到已运行的 Explorer

如果想调试当前桌面：

1. 调试 → 附加到进程
2. 选择 `explorer.exe`（注意有多个，选你怀疑加载了 DLL 的那个）
3. 在模块窗口确认 `XPTabBand.dll` 已加载

> ⚠️ 调试 explorer.exe 会影响整个桌面。断点触发时桌面会冻结，请保存好其他工作。

### 实时日志

```powershell
Get-Content "$env:LOCALAPPDATA\Temp\XPTabBand_log.txt" -Wait -Tail 20
```

或 cmd：

```cmd
type %LOCALAPPDATA%\Temp\XPTabBand_log.txt
```

### 调试崩溃

如果 Explorer 崩溃，Windows 会生成 dump（如果配置了）：

1. 注册表启用 dump：`HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps`
2. 设置 `DumpFolder`（如 `C:\dumps`）和 `DumpType = 2`（完整 dump）
3. 崩溃后用 WinDbg 打开 dump 文件
4. 加载符号：`.sympath srv*C:\symbols*https://msdl.microsoft.com/download/symbols;<项目pdb路径>`
5. `!analyze -v` 自动分析

### 修改后快速测试循环

1. 修改代码
2. 卸载 DLL：`regsvr32 /u XPTabBand.dll`
3. 关闭所有 Explorer 窗口（或重启 explorer.exe）
4. 编译
5. 注册：`regsvr32 XPTabBand.dll`
6. 打开文件夹窗口测试

可以把这串写成 `reload.bat`：

```cmd
@echo off
regsvr32 /u /s XPTabBand.dll
taskkill /f /im explorer.exe
timeout /t 1 /nobreak >nul
MSBuild XPTabCpp\XPTab.sln /p:Configuration=Debug /p:Platform=x64 /t:Build
regsvr32 /s XPTabCpp\XPTabBand\build\Debug\XPTabBand.dll
start explorer.exe
```

## 代码规范

### 编码

- C++ 文件使用 **UTF-8 with BOM** 编码（VS 默认）
- 行尾使用 **CRLF**（已在 `.gitattributes` 中配置）
- 缩进 4 空格，不使用 Tab

### 命名

| 类型 | 规则 | 示例 |
| ---- | ---- | ---- |
| 类成员变量 | `m_` 前缀 | `m_hwnd`、`m_pBrowser` |
| 常量 | `k` 前缀 + 驼峰 | `kColorBg`、`kTabBarHeight` |
| 函数 | PascalCase | `OnPaint`、`HitTest` |
| 局部变量 | camelCase | `tabWidth`、`hdcMem` |
| 宏 | 全大写 + 下划线 | `SAFE_RELEASE`、`MIN_TAB_WIDTH` |
| COM 接口指针 | `p` 前缀 | `pShellBrowser`、`pPidl` |

### COM 资源管理

```cpp
// 正确：用完立即释放
IWebBrowser2* pBrowser = nullptr;
if (SUCCEEDED(QueryService(SID_SWebBrowserApp, &pBrowser))) {
    // 使用 pBrowser
    pBrowser->Release();
}

// 错误：忘记 Release
```

封装宏：

```cpp
#define SAFE_RELEASE(p) { if ((p)) { (p)->Release(); (p) = nullptr; } }
```

### SEH 包装

C++ try/catch 不能捕获 SEH 异常（访问违例等）。关键 COM 调用必须用 `__try/__except` 包装，且**不能和 C++ 对象析构在同一函数**混用：

```cpp
// 单独的 SEH 包装函数（纯 C 风格）
HRESULT SafeNavigate(IWebBrowser2* pBrowser, LPITEMIDLIST pidl) {
    __try {
        return pBrowser->Navigate2(...);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        LogError("Navigate2 SEH caught: 0x%08X", GetExceptionCode());
        return E_FAIL;
    }
}
```

### 日志输出

使用项目统一的日志宏：

```cpp
LogInfo("OnPaint w=%d h=%d", width, height);
LogWarn("NavigateToPidl hr=0x%08X", hr);
LogError("SEH caught: 0x%08X", code);
```

注意：

- `OnPaint` 是高频调用，必须限频（项目里每秒最多 1 条）
- 不要在循环里输出日志，会拖慢 UI

## 常见开发任务

### 添加标签右键菜单

修改 [TabBarUI.cpp](../XPTabCpp/XPTabBand/TabBarUI.cpp) 的 `HandleMessage`：

```cpp
case WM_RBUTTONDOWN: {
    int idx = HitTest(LOWORD(lParam), HIWORD(lParam));
    if (idx >= 0) {
        HMENU hMenu = CreatePopupMenu();
        AppendMenuW(hMenu, MF_STRING, 1, L"关闭此标签");
        AppendMenuW(hMenu, MF_STRING, 2, L"关闭其他标签");
        AppendMenuW(hMenu, MF_STRING, 3, L"复制路径");
        // ...
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };
        ClientToScreen(m_hwnd, &pt);
        TrackPopupMenu(hMenu, TPM_LEFTBUTTON, pt.x, pt.y, 0, m_hwnd, nullptr);
        DestroyMenu(hMenu);
    }
    return 0;
}
```

### 添加配置文件支持

新增 `ConfigManager` 类：

```cpp
class ConfigManager {
public:
    static ConfigManager& Instance();
    bool Load();                          // 从 %APPDATA%\XPTabCpp\config.ini 加载
    bool Save();
    
    int tabHeight = 30;
    COLORREF colorBg = RGB(32, 32, 32);
    // ...

private:
    ConfigManager() = default;
};
```

在 `XPTabBandClass::SetSite` 中调用 `Load()`，在 `TabBarUI` 中读取 `ConfigManager::Instance().tabHeight` 等。

### 添加快捷键

在 `HandleMessage` 的 `WM_KEYDOWN` 中处理：

```cpp
case WM_KEYDOWN:
    if (wParam == 'T' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        // Ctrl+T 新建标签
        CreateNewTab();
        return 0;
    }
    if (wParam == 'W' && (GetKeyState(VK_CONTROL) & 0x8000)) {
        // Ctrl+W 关闭当前标签
        CloseActiveTab();
        return 0;
    }
    break;
```

注意 DeskBand 需要实现 `IInputObject::TranslateAcceleratorIO` 才能接收键盘消息。

### 添加主题切换

把 [TabBarUI.cpp](../XPTabCpp/XPTabBand/TabBarUI.cpp) 中的颜色常量从硬编码改为从 `ConfigManager` 读取：

```cpp
// 之前
static constexpr COLORREF kColorBg = RGB(32, 32, 32);

// 改为
COLORREF GetColorBg() {
    return ConfigManager::Instance().colorBg;
}
```

并在 `ConfigManager::Load()` 中读取 INI 文件的颜色段：

```ini
[Colors]
Background=32,32,32
TabActive=62,62,62
TabInactive=38,38,38
HighlightBar=0,120,215
```

## 提交前检查清单

- [ ] Release x64 配置下编译通过
- [ ] 没有新的编译警告（`/W3` 级别）
- [ ] 在新 Explorer 窗口中测试：标签创建、切换、关闭、收藏夹
- [ ] 测试关闭窗口后 Explorer 不崩溃（重复打开关闭 5 次）
- [ ] 测试特殊文件夹（控制面板、网络连接）
- [ ] 日志中没有 FATAL 级别条目
- [ ] commit message 遵循 [Conventional Commits](https://www.conventionalcommits.org/zh-hans/v1.0.0/)
- [ ] 如果改了 UI，更新 [docs/screenshots.md](screenshots.md) 中相关截图

## 进一步阅读

- [架构文档](architecture.md)
- [发布流程](release.md)
- [贡献指南](../CONTRIBUTING.md)
- [故障排查](troubleshooting.md)
- [MSDN: Creating Custom Explorer Bars, Tool Bands, and Desk Bands](https://learn.microsoft.com/windows/win32/shell/bands)
