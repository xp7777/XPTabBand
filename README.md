# XPTabBand

为 Windows 资源管理器添加类似 Chrome/Edge 的多标签页工具栏，支持收藏夹管理。

![License](https://img.shields.io/badge/license-GPL--3.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20x64-lightgrey.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B-orange.svg)

## 功能特性

- **多标签页**：在一个资源管理器窗口中打开多个文件夹标签
- **暗色主题 UI**：现代化深色标签栏，顶部蓝色高亮条标识激活标签
- **收藏夹**：右键标签栏空白处快速访问收藏的文件夹
- **收藏夹管理**：上移/下移/删除收藏项，操作后菜单自动重新弹出便于连续操作
- **特殊文件夹支持**：可正常显示控制面板子项、网络连接等特殊位置
- **标准 EXE 安装包**：基于 Inno Setup，注册到"程序和功能"卸载列表

## 系统要求

- Windows 10 64 位 / Windows 11
- x64 架构（不支持 32 位系统）
- 安装/卸载需管理员权限

## 安装

### 方式一：标准 EXE 安装（推荐）

从 [Releases](https://github.com/xp7777/XPTabBand/releases) 下载 `XPTabBand-x.x.x-setup.exe`：

1. 双击运行 setup.exe，授权 UAC
2. 跟随向导完成安装
3. 打开任意文件夹窗口
4. 顶部菜单栏 **右键** → **"查看"** → **"工具栏"** → 勾选 **"XPTabBand"**

### 方式二：BAT 脚本安装

下载 deploy 目录内容后：

1. **右键** `install.bat` → **"以管理员身份运行"**
2. 按方式一的步骤 3-4 勾选工具栏

## 使用

| 操作 | 动作 |
|------|------|
| 新建标签 | 点击 `+` 按钮 |
| 关闭标签 | 点击标签 `×` |
| 切换标签 | 点击标签主体 |
| 添加到收藏夹 | 左键 `☆` 按钮 |
| 打开收藏菜单 | 右键标签栏黑色空白区域 |

### 收藏夹菜单

**主菜单**：
- 点击收藏项 → 在新标签打开该文件夹
- "管理收藏夹..." → 进入管理菜单

**管理菜单**（操作后自动重新弹出，可连续操作）：
- ▲ 上移 / ▼ 下移 → 调整顺序
- × 删除 → 移除收藏项
- ◀ 返回主菜单

详细使用说明见 [deploy/README.md](XPTabCpp/deploy/README.md)。

## 卸载

- **EXE 安装方式**：设置 → 应用 → 已安装的应用 → "XPTabBand x.x.x" → 卸载
- **BAT 安装方式**：右键 `uninstall.bat` → 以管理员身份运行

卸载向导会询问是否保留收藏夹数据（`%APPDATA%\XPTabCpp\favorites.dat`）。

## 项目结构

```
XPTabCpp/
├── XPTabBand/              主程序（DeskBand COM 组件）
│   ├── XPTabBandClass.*    COM 接口实现（IDeskBand2 等）
│   ├── TabBarUI.*          标签栏 UI 与逻辑
│   ├── dllmain.cpp         DllRegisterServer / DllUnregisterServer
│   └── XPTabBand.def       模块定义
├── XPTabHook/              早期 DLL 注入方案（已弃用，保留作参考）
├── XPTabInject/            早期注入器（已弃用）
├── MinHook/                MinHook 库（用于早期注入方案）
├── deploy/                 简易部署包（DLL + BAT 脚本）
└── installer/              Inno Setup 安装项目
    └── XPTabBand.iss       安装脚本
```

根目录的 `XPTab/` 和 `QTTabBar_BandObject.cs` 是早期 C# 尝试和 QTTabBar 参考代码，最终方案为 `XPTabCpp/XPTabBand/`。

## 从源码编译

### 编译 DLL

**前置条件**：Visual Studio 2022（含 C++ 桌面开发工作负载）

```cmd
cd XPTabCpp
MSBuild XPTab.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild
```

输出：`XPTabCpp\XPTabBand\build\XPTabBand.dll`

> ⚠️ 编译前必须先卸载已注册的 DLL（运行 `deploy\uninstall.bat`），否则文件被 explorer.exe 锁定导致 LNK1104 错误。

### 编译安装包

前置条件：[Inno Setup 6](https://jrsoftware.org/isdl.php)

```cmd
cd XPTabCpp\installer
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" XPTabBand.iss
```

输出：`XPTabCpp\installer\Output\XPTabBand-x.x.x-setup.exe`

## 技术原理

- **实现方式**：COM DeskBand 组件（实现 IDeskBand2 / IObjectWithSite / IPersistStream）
- **集成方式**：Explorer 通过 CoCreateInstance 加载，渲染在工具栏区域
- **导航控制**：通过 IWebBrowser2 / IShellBrowser::BrowseObject 切换文件夹
- **PIDL 持久化**：收藏夹使用 PIDL 二进制格式存储，支持特殊文件夹
- **双缓冲绘制**：使用内存 DC 防止闪烁

## 已知限制

- 标签栏占用资源管理器顶部 30 像素高度
- 部分控制面板子项可能在新窗口打开（Windows Shell 限制）
- 仅支持 x64 系统
- 未数字签名，首次运行可能被 SmartScreen 拦截

## 开发历程

本项目经过多次方案迭代：
1. 早期 C# DeskBand（`XPTab/`）— COM 互操作复杂，弃用
2. DLL 注入方案（`XPTabHook/` + `XPTabInject/`）— 跨进程子类化不稳定
3. C++ DeskBand（`XPTabBand/`）— 最终采用方案，参考 QTTabBar 实现

## 致谢

- [QTTabBar](https://github.com/indiff/QTTabBar) — BandObject 实现参考，GPL-3.0
- [MinHook](https://github.com/TsudaKageyu/minhook) — 用于早期注入方案的 API Hook 库

## License

本项目基于 GPL-3.0 协议开源，参考了 QTTabBar 的 GPL-3.0 代码。详见 [LICENSE](LICENSE)。
