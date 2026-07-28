# 贡献指南

感谢你对 XPTabBand 的兴趣！欢迎提交 Issue、Pull Request 或其他形式的贡献。

## 行为准则

请保持友善和专业。本项目欢迎所有背景的贡献者。

## 如何贡献

### 报告 Bug

1. 先在 [Issues](https://github.com/xp7777/XPTabBand/issues) 中搜索是否已有相同问题
2. 如果没有，点击 **New Issue** → 选择 **Bug Report** 模板
3. 尽量提供以下信息：
   - Windows 版本（Win 10 / Win 11、build 号）
   - XPTabBand 版本
   - 复现步骤
   - 期望行为 vs 实际行为
   - 截图/录屏（如有）
   - 日志文件：`%LOCALAPPDATA%\Temp\XPTabBand_log.txt`

### 提出功能建议

1. 先在 [Issues](https://github.com/xp7777/XPTabBand/issues) 中搜索相关讨论
2. 没有则点击 **New Issue** → 选择 **Feature Request** 模板
3. 说明：
   - 使用场景（解决什么问题）
   - 期望的交互方式
   - 是否有类似软件的参考

### 提交代码

#### 开发环境要求

- Visual Studio 2022（17.x），含 **使用 C++ 的桌面开发** 工作负载
- Windows 10 SDK 10.0.19041.0 或更高
- Inno Setup 6（仅打包 Release 时需要）

#### 工作流程

1. **Fork** 本仓库到自己的账号下
2. **克隆** Fork 仓库到本地：
   ```cmd
   git clone https://github.com/<你的用户名>/XPTabBand.git
   cd XPTabBand
   ```
3. **添加上游**：
   ```cmd
   git remote add upstream https://github.com/xp7777/XPTabBand.git
   ```
4. **创建分支**（不要在 main 上直接开发）：
   ```cmd
   git checkout -b feature/my-feature
   ```
5. **编译测试**：
   ```cmd
   cd XPTabCpp
   MSBuild XPTab.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild
   ```
6. **提交**（使用清晰的 commit message）：
   ```cmd
   git commit -m "feat: 添加标签拖拽排序功能"
   ```
7. **同步上游**（避免冲突）：
   ```cmd
   git fetch upstream
   git rebase upstream/main
   ```
8. **推送并创建 PR**：
   ```cmd
   git push origin feature/my-feature
   ```
   然后在 GitHub 上发起 Pull Request。

#### Commit Message 规范

使用 [Conventional Commits](https://www.conventionalcommits.org/zh-hans/v1.0.0/)：

| 前缀 | 含义 | 示例 |
|------|------|------|
| `feat:` | 新功能 | `feat: 添加标签拖拽排序` |
| `fix:` | Bug 修复 | `fix: 修复控制面板项导航崩溃` |
| `docs:` | 文档更新 | `docs: 完善安装说明` |
| `style:` | 代码风格调整（不影响功能） | `style: 统一缩进` |
| `refactor:` | 重构 | `refactor: 抽取 PIDL 工具函数` |
| `perf:` | 性能优化 | `perf: 减少定时器触发频率` |
| `chore:` | 构建/工具链 | `chore: 升级到 VS2022` |

#### 代码风格

- C++ 文件使用 UTF-8 with BOM 编码
- 缩进 4 空格
- 命名：
  - 类成员：`m_` 前缀（如 `m_hwnd`、`m_pBrowser`）
  - 常量：`k` 前缀 + 驼峰（如 `kColorBg`、`kTabBarHeight`）
  - 函数：PascalCase（如 `OnPaint`、`HitTest`）
- COM 接口指针用完立即 Release，避免循环引用
- 关键 COM 调用用 SEH `__try/__except` 包装，避免 Explorer 崩溃

### 提交前的检查清单

- [ ] 在本地 Release x64 配置下编译通过
- [ ] 在新的 Explorer 窗口中测试基本功能（标签创建、切换、关闭）
- [ ] 测试收藏夹功能（添加、删除、排序）
- [ ] 没有引入新的编译警告
- [ ] commit message 遵循规范
- [ ] 如果改了 UI，更新相关截图

## 项目结构

详见 [README.md](README.md#项目结构)。主要代码在 `XPTabCpp/XPTabBand/`：

- `XPTabBandClass.cpp` — COM 接口实现
- `TabBarUI.cpp` — 标签栏 UI 和逻辑
- `dllmain.cpp` — DLL 注册/注销

## 调试技巧

### 查看日志

```cmd
:: 实时查看日志
Get-Content "$env:LOCALAPPDATA\Temp\XPTabBand_log.txt" -Wait -Tail 20

:: 或用 cmd
type %LOCALAPPDATA%\Temp\XPTabBand_log.txt
```

### 调试 DLL

1. 在 Visual Studio 中设置：
   - Project Properties → Debugging → Command = `explorer.exe`
   - Attach → Yes
2. F5 启动调试，VS 会启动 explorer.exe 并附加到 DLL

### 重新注册 DLL（开发期间）

```cmd
:: 卸载旧的
regsvr32 /u XPTabBand.dll

:: 编译新的
MSBuild XPTab.sln /p:Configuration=Debug /p:Platform=x64 /t:Rebuild

:: 注册新的
regsvr32 XPTabBand.dll
```

## 发布流程（仅维护者）

1. 更新 `CHANGELOG.md`
2. 更新 `XPTabCpp/installer/XPTabBand.iss` 中的 `AppVersion`
3. 更新 `README.md` 中的版本徽章（如有）
4. 创建 git tag：
   ```cmd
   git tag -a v1.3.0 -m "Release v1.3.0"
   git push github v1.3.0
   git push gitee v1.3.0
   ```
5. 编译 setup.exe 并上传到 GitHub/Gitee Releases
6. 在 Release 描述中粘贴 CHANGELOG 对应章节

## 许可证

提交的代码默认以 [GPL-3.0](LICENSE) 协议发布。如果你引用了第三方代码，请在 PR 中说明，并确保兼容 GPL-3.0。

## 问题？

- 提 [Issue](https://github.com/xp7777/XPTabBand/issues)
- 或发邮件到你的联系方式（待补充）

再次感谢你的贡献！
