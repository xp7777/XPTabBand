# XPTabCpp - Explorer 多标签页功能

通过 DLL 注入 + 窗口子类化方式为 Windows 资源管理器添加多标签页功能（类似 QTTabBar）。

## 项目结构

解决方案 `XPTab.sln` 包含两个协同工作的项目：

```
XPTab.sln
├── XPTabHook (DLL 项目)  → 生成 XPTabHook.dll
│   ├── dllmain.cpp          DLL 入口，自引用防止卸载
│   ├── HookMain.cpp         Hook 主逻辑，安装 SetWinEventHook
│   ├── ExplorerHook.cpp     窗口枚举和子类化实现
│   ├── TabBarWindow.cpp     标签栏 UI 和交互（核心功能）
│   ├── ExplorerInterface.cpp 封装 COM 交互（IWebBrowser2）
│   └── Utils.h/.cpp         日志、路径等工具函数
│
└── XPTabInject (EXE 项目)  → 生成 XPTabInject.exe
    └── XPTabInject.cpp       注入器主程序
```

### 两个项目的职责

| 项目 | 输出 | 作用 | 是否单独运行 |
|------|------|------|-------------|
| **XPTabHook** | XPTabHook.dll | 实际的标签页功能代码（被注入到 Explorer 进程内运行） | 否（被动加载） |
| **XPTabInject** | XPTabInject.exe | 注入器，负责把 DLL 注入到 explorer.exe | 是（主动运行） |

- `XPTabHook.dll`：核心功能模块，实现标签栏绘制、标签创建/关闭/切换、通过 IWebBrowser2 COM 接口控制 Explorer 导航
- `XPTabInject.exe`：注入器，使用 CreateRemoteThread + LoadLibraryW 经典注入方式，支持多进程架构（枚举所有 explorer.exe 并逐一注入）

## 核心技术

- **DLL 注入**：CreateRemoteThread + LoadLibraryW，支持 Explorer 多进程架构
- **窗口子类化**：SetWindowLongPtrW 替换 CabinetWClass 的 WndProc
- **跨线程 Hook**：SetWinEventHook (WINEVENT_INCONTEXT) + WH_CALLWNDPROC，解决跨线程子类化问题
- **COM 交互**：IShellWindows 枚举窗口、IWebBrowser2::Navigate2 实现 PIDL 导航
- **自引用**：DllMain 中 LoadLibrary 自身，防止 DLL 被意外卸载

## 使用方法

### 1. 编译

在 Visual Studio 中打开 `XPTab.sln`，生成解决方案（Release|x64）。输出到 `build\` 目录：
- `build\XPTabHook.dll`
- `build\XPTabInject.exe`

也可以使用命令行：
```cmd
MSBuild XPTab.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild
```

### 2. 注入标签页功能

**必须以管理员身份运行**（需要 SeDebugPrivilege 权限）：

```cmd
cd g:\Test\testFileExplorerPro\XPTabCpp\build
XPTabInject.exe -install
```

注入成功后，打开任意 Explorer 窗口即可看到顶部的标签栏。

### 3. 标签栏操作

- **新建标签**：点击标签栏右侧的 `+` 按钮（复制当前标签的文件夹）
- **关闭标签**：点击标签右侧的 `×` 按钮（关闭最后一个标签会关闭 Explorer 窗口）
- **切换标签**：点击标签主体切换到该标签对应的文件夹
- **自动更新标题**：在 Explorer 中手动导航（地址栏、双击文件夹）时，当前标签标题会自动同步

### 4. 卸载

```cmd
XPTabInject.exe -uninstall
```

向所有 CabinetWClass 窗口发送卸载消息，DLL 异步执行 FreeLibraryAndExitThread 退出。

## 重要说明

### 编译前的注意事项（避免 LNK1104 错误）

**每次重新编译前，必须先卸载 DLL**，否则 `XPTabHook.dll` 被 explorer.exe 加载锁定，VS 会报错：

```
错误 LNK1104: 无法打开文件 XPTabHook.dll
```

解决方法（任选其一）：

**方案 A（推荐）**：运行卸载命令
```cmd
XPTabInject.exe -uninstall
```

**方案 B**：任务管理器 → 重启 explorer.exe（会丢失所有 DLL 注入）
```cmd
taskkill /f /im explorer.exe
start explorer.exe
```

### 多进程架构

Windows 10/11 的 Explorer 是多进程架构：主进程（任务栏）和各文件夹窗口。注入器会枚举所有 explorer.exe 进程并逐一注入。

**注意**：注入后新打开的 Explorer 窗口可能运行在新的进程中，需要再次运行 `XPTabInject.exe -install` 注入新进程。

### 日志文件

- 详细日志：`%LOCALAPPDATA%\XPTabCpp\hook_log.txt`
- 调试追踪：`build\debug_trace.txt`（绕过 CRT 和 Shell 依赖，用于诊断 DllMain/工作线程）

### 已知限制

1. TabBar 覆盖 Explorer 顶部 30 像素（可能遮挡 Ribbon 顶部）
2. 导航变化检测依赖定时器轮询（800ms 间隔），非实时
3. 标签标题同步依赖 PIDL 比较，特殊文件夹可能显示不准确

## 故障排查

### 标签栏未显示

1. 确认以管理员身份运行了 `XPTabInject.exe -install`
2. 查看日志 `%LOCALAPPDATA%\XPTabCpp\hook_log.txt` 是否有"已子类化窗口"记录
3. 激活 Explorer 窗口（点击窗口）触发 WinEvent 回调
4. 新进程需要重新注入

### 编译失败 LNK1104

见上文"编译前的注意事项"。

### Explorer 崩溃

1. 运行 `XPTabInject.exe -uninstall` 卸载
2. 任务管理器重启 explorer.exe
3. 查看日志定位问题

## 测试脚本

`XPTabCpp\` 目录下包含 PowerShell 测试脚本：

- `check_tabs.ps1`：检查 CabinetWClass 窗口和 TabBar 状态
- `check_tabbar_rect.ps1`：检查 TabBar 窗口位置和可见性
- `check_threads.ps1`：检查窗口线程信息
- `test_plus_button.ps1`：测试 + 按钮功能
- `test_tab_switch.ps1`：测试标签切换和关闭
- `activate_windows.ps1`：激活所有 CabinetWClass 窗口触发事件
- `restore_and_shot.ps1`：恢复窗口并截图
