# XPTabBand - 资源管理器多标签页工具

为 Windows 资源管理器添加类似 Chrome/Edge 的多标签页功能，支持收藏夹管理。

## 功能特性

- **多标签页**：在一个资源管理器窗口中打开多个文件夹标签
- **暗色主题 UI**：现代化深色标签栏，顶部蓝色高亮条标识激活标签
- **收藏夹**：右键标签栏空白处快速访问收藏的文件夹
- **收藏夹管理**：上移/下移/删除收藏项，操作后菜单自动重新弹出便于连续操作
- **特殊文件夹支持**：可正常显示控制面板子项、网络连接等特殊位置

## 系统要求

- **操作系统**：Windows 10 64 位 / Windows 11
- **架构**：x64（不支持 32 位系统）
- **权限**：安装/卸载需管理员权限

---

## 📦 方式一：标准 EXE 安装（推荐）

### 编译安装包（开发者）

#### 1. 安装 Inno Setup

- **官网下载**：https://jrsoftware.org/isdl.php
- **winget 命令**：
  ```powershell
  winget install JRSoftware.InnoSetup
  ```
- 安装时勾选 **"Install Chinese Simplified language file"**（中文支持）

#### 2. 编译 setup.exe

脚本位于：`XPTabCpp\installer\XPTabBand.iss`

**方法 A：图形界面**
1. 双击 `XPTabBand.iss` 打开 Inno Setup Compiler
2. 菜单 `Build` → `Compile`（或按 `Ctrl+F9`）
3. 生成的 setup.exe 在 `installer\Output\XPTabBand-1.2.0-setup.exe`

**方法 B：命令行**
```cmd
cd g:\Test\testFileExplorerPro\XPTabCpp\installer
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" XPTabBand.iss
```

#### 3. 文件依赖

编译前确保以下文件存在：

```
XPTabCpp\
├── deploy\
│   ├── XPTabBand.dll      ← 主程序（来自 XPTabBand\build\）
│   └── README.md          ← 用户文档
└── installer\
    └── XPTabBand.iss      ← Inno Setup 脚本
```

### 安装到目标电脑

1. 双击 `XPTabBand-1.2.0-setup.exe`
2. UAC 弹窗 → 点击"是"获取管理员权限
3. 跟随向导完成安装（中英文可选）
4. 安装完成后打开任意文件夹窗口
5. 顶部菜单栏 **右键** → **"查看"** → **"工具栏"** → 勾选 **"XPTabBand"**
6. 标签栏出现

### 卸载

**方法 A**：通过 Windows 卸载列表
1. 打开"设置"→"应用"→"已安装的应用"（或控制面板 → 程序和功能）
2. 找到 **"XPTabBand 1.2.0"**
3. 点击"卸载"，跟随向导完成
4. 卸载向导会询问是否保留收藏夹数据

**方法 B**：运行卸载程序
- 进入 `C:\Program Files\XPTabBand\`
- 双击 ` unins000.exe`

---

## 📦 方式二：BAT 脚本安装（简易）

适用于无法使用 Inno Setup 或需要快速部署的场景。

### 安装

1. 复制 `deploy\` 目录到目标电脑
2. **右键** `install.bat` → 选择 **"以管理员身份运行"**
3. 安装完成后打开任意文件夹窗口
4. 顶部菜单栏 **右键** → **"查看"** → **"工具栏"** → 勾选 **"XPTabBand"**

### 卸载

1. **右键** `uninstall.bat` → 选择 **"以管理员身份运行"**
2. 按提示选择是否删除收藏夹数据

> ⚠️ BAT 脚本方式不会出现在 Windows 卸载列表，需手动运行 `uninstall.bat` 卸载。

---

## 📖 使用说明

### 标签栏操作

| 操作 | 动作 |
|------|------|
| **新建标签** | 点击标签栏右侧的 `+` 按钮 |
| **关闭标签** | 点击标签右侧的 `×` 按钮 |
| **切换标签** | 点击标签主体 |
| **添加到收藏夹** | 左键点击标签栏右侧的 `☆` 按钮 |
| **打开收藏夹菜单** | **右键** 标签栏黑色空白区域 |

### 收藏夹菜单

**主菜单（右键空白处弹出）**

```
当前文件夹1              ← 点击直接在新标签打开
当前文件夹2
当前文件夹3
─────────────
管理收藏夹...            ← 进入管理菜单
─────────────
清空收藏夹
```

**管理菜单（独立弹出）**

```
文件夹1                  ← 灰色标题
    操作...   ▶
        ▲ 上移            ← 与前一项交换位置
        ▼ 下移            ← 与后一项交换位置
        × 删除            ← 从收藏夹删除此项
─────────
文件夹2
    操作...   ▶
        ▲ 上移
        ▼ 下移
        × 删除
─────────
◀ 返回主菜单
```

**关键改进**：在管理菜单中执行 上移/下移/删除 后，**管理菜单会自动重新弹出**（保持原位置），可以连续操作多项，无需反复从主菜单进入。

### 数据存储

| 类型 | 路径 |
|------|------|
| 安装目录 | `C:\Program Files\XPTabBand\` |
| 主程序 | `C:\Program Files\XPTabBand\XPTabBand.dll` |
| 卸载程序 | `C:\Program Files\XPTabBand\unins000.exe`（仅 EXE 安装方式） |
| 收藏夹数据 | `%APPDATA%\XPTabCpp\favorites.dat` |
| 日志文件 | `%LOCALAPPDATA%\Temp\XPTabBand_log.txt` |

### 文件清单

**EXE 安装包**：
```
XPTabBand-1.2.0-setup.exe    Inno Setup 生成的安装程序（约 600 KB）
```

**BAT 简易包**：
```
deploy/
├── XPTabBand.dll     主程序（COM 组件，52 KB）
├── install.bat       安装脚本
├── uninstall.bat     卸载脚本
└── README.md         本说明文档
```

---

## ❓ 常见问题

### Q1：安装后看不到标签栏？

**A**：请按以下步骤排查：

1. **检查是否勾选**：在资源管理器顶部菜单栏 **右键** → **"查看"** → **"工具栏"** → 勾选 **"XPTabBand"**
2. **检查 DLL 是否注册成功**：以管理员身份打开 PowerShell，运行：
   ```powershell
   reg query "HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar" /v "{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}"
   ```
   应返回 `XPTabBand REG_SZ XPTabBand`
3. **检查 DLL 文件**：确认 `C:\Program Files\XPTabBand\XPTabBand.dll` 存在
4. **重启资源管理器**：任务管理器 → 找到 "Windows 资源管理器" → 右键 → "重新启动"
5. **重启电脑**：若以上均无效，请重启电脑后再试

### Q2：标签栏显示为白框？

**A**：这是 DLL 渲染问题。请确认：
- DLL 版本与系统位数匹配（必须 x64）
- 重新运行 `install.bat` 或 setup.exe 覆盖安装

### Q3：勾选 XPTabBand 时资源管理器崩溃？

**A**：可能是 DLL 与当前系统版本不兼容。请：
1. 通过"程序和功能"或 `uninstall.bat` 卸载
2. 检查 Windows 版本（`winver` 命令），确保为 Win10 1809 或更高
3. 联系开发者获取适配版本

### Q4：安装后 "此电脑" 打不开？

**A**：早期版本曾出现此问题，新版本已修复。请确保使用最新 DLL，必要时重新安装。

### Q5：卸载后工具栏列表中仍显示 "XPTabBand"？

**A**：这是注册表缓存，重启资源管理器或重启电脑后会消失。如仍残留，以管理员身份运行：

```cmd
reg delete "HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar" /v "{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}" /f
reg delete "HKLM\SOFTWARE\Classes\CLSID\{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}" /f
taskkill /f /im explorer.exe && start explorer.exe
```

### Q6：网络连接等特殊文件夹在新窗口打开？

**A**：这是 Windows Shell 的限制。对于某些控制面板子项（如"网络连接"），Shell 会强制在新窗口打开。当前已使用 `IShellBrowser::BrowseObject(SBSP_SAMEBROWSER)` 尽量在当前窗口切换，但部分特殊 PIDL 仍可能开新窗口。

### Q7：SmartScreen 拦截了 setup.exe？

**A**：因为 exe 未数字签名。处理方式：
- 点击"更多信息" → "仍要运行"
- 或右键 exe → 属性 → 勾选"解除锁定" → 确定
- 长期方案：为 exe 申请代码签名证书（需付费）

### Q8：安装时被杀毒软件拦截？

**A**：因为 DLL 注入到 explorer.exe 的行为类似恶意软件。处理方式：
- 在杀毒软件中添加 `C:\Program Files\XPTabBand\XPTabBand.dll` 到信任列表
- 或临时关闭实时保护后安装，安装完成后再开启

---

## 🔄 升级方法

### EXE 安装方式

1. 直接运行新版 `XPTabBand-x.x.x-setup.exe`
2. 安装向导会自动覆盖旧版本
3. 收藏夹数据会被保留

### BAT 脚本方式

1. 运行 `uninstall.bat`（管理员）卸载旧版本
2. 用新版本的 `deploy\` 目录覆盖
3. 运行 `install.bat`（管理员）安装新版本

---

## 🛠️ 开发者信息

### 项目结构

```
XPTabCpp/
├── XPTabBand/                ← DeskBand COM 组件（主程序）
│   ├── XPTabBandClass.cpp   COM 接口实现
│   ├── TabBarUI.cpp          标签栏 UI 与逻辑
│   ├── dllmain.cpp           DllRegisterServer/DllUnregisterServer
│   └── build/XPTabBand.dll   编译输出
├── XPTabHook/                ← DLL 注入方案（已弃用，保留作参考）
├── XPTabInject/              ← 注入器（已弃用）
├── deploy/                   ← 简易部署包（DLL + BAT）
└── installer/                ← Inno Setup 安装项目
    └── XPTabBand.iss         Inno Setup 脚本
```

### 技术原理

- **实现方式**：COM DeskBand 组件（实现 IDeskBand2 / IObjectWithSite / IPersistStream 接口）
- **CLSID**：`{A1B2C3D4-1234-4ABC-9DEF-1234567890AB}`
- **线程模型**：Apartment
- **集成方式**：Explorer 通过 CoCreateInstance 加载，渲染在 Explorer 工具栏区域
- **导航控制**：通过 `IWebBrowser2` / `IShellBrowser::BrowseObject` 接口切换文件夹
- **PIDL 持久化**：收藏夹使用 PIDL 二进制格式存储，支持特殊文件夹

### 编译 DLL

**前置条件**：Visual Studio 2022（含 C++ 桌面开发工作负载）

```cmd
cd g:\Test\testFileExplorerPro\XPTabCpp
MSBuild XPTab.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild
```

输出：`XPTabBand\build\XPTabBand.dll`

> ⚠️ 编译前必须先卸载已注册的 DLL，否则文件被 explorer.exe 锁定导致 LNK1104 错误。
> 运行 `uninstall.bat` 或任务管理器重启 explorer.exe 即可。

### 自定义安装包

修改 `installer\XPTabBand.iss` 中的常量：

```pascal
#define MyAppName          "XPTabBand"        ; 应用名
#define MyAppVersion       "1.2.0"           ; 版本号
#define MyAppPublisher     "XPTabCpp Project" ; 发布者
#define MyAppURL          "https://..."       ; 官网
```

---

## ⚠️ 已知限制

1. 标签栏占用资源管理器顶部 30 像素高度
2. 部分控制面板子项可能在新窗口打开（Shell 限制）
3. 仅支持 x64 系统
4. 未数字签名，首次运行可能被 SmartScreen 拦截

## 📋 版本历史

| 版本 | 主要变更 |
|------|---------|
| **v1.0** | 基础多标签页功能 + 收藏夹 |
| **v1.1** | 现代化深色 UI（Chrome 风格高亮条） |
| **v1.2** | 收藏夹管理菜单（上移/下移/删除），操作后菜单自动重现；Inno Setup 安装包 |

## 📞 反馈与支持

如遇问题，请按以下顺序排查：

1. 查看 `%LOCALAPPDATA%\Temp\XPTabBand_log.txt` 日志文件
2. 参考本文档"常见问题"章节
3. 重启资源管理器或电脑
4. 提交 Issue 到项目仓库，附上日志文件和操作步骤
