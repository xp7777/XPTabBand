# 变更记录

所有重要变更都会记录在此文件中。格式遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.0.0/)，版本号遵循 [语义化版本](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### 计划中
- 标签拖拽重排
- 标签右键菜单（关闭其他、关闭右侧、复制路径等）
- 主题切换（亮色/暗色/跟随系统）
- 配置文件（标签宽度、颜色、快捷键）

## [1.2.0] - 2026-07-25

### 新增
- ★ **收藏夹功能**：左键 ☆ 把当前文件夹加入收藏
- ★ **收藏夹管理菜单**：右键标签栏空白处弹出，支持上移/下移/删除/清空
- ★ **操作后菜单自动重现**：连续调整顺序无需反复右键
- ★ **PIDL 持久化存储**：支持普通文件夹和特殊文件夹（控制面板项、网络连接等）
- ★ **Inno Setup 安装程序**：标准 EXE 安装包，注册到"程序和功能"卸载列表
- ★ **中英文 README**：开源版完整文档
- 现代化 UI：激活标签顶部蓝色高亮条（Chrome 风格）
- 按钮悬停圆形背景效果
- 文字超出省略显示
- 顶部细高光线增加层次感

### 改进
- 配色层次更柔和：背景 RGB(32,32,32)，激活标签 RGB(62,62,62)
- 标签间距 3px，视觉上有间距
- 标签顶部 3px 边距，让标签"浮"在背景上

### 修复
- ✅ 修复白框问题：`OnPaint` 末尾加入 `BitBlt` 把内存 DC 输出到屏幕 DC
- ✅ 修复窗口类背景画刷为白色导致闪烁：改为暗色 RGB(30,30,30)
- ✅ 修复关闭文件夹导致 Explorer 重启：`RegisterClassExW` 失败时不再创建窗口
- ✅ 修复 `IWebBrowser2` 访问崩溃：加入 SEH 异常包装
- ✅ 修复"此电脑"双击打不开：移除 WH_CALLWNDPROC 钩子
- ✅ 修复网络连接页重影：`SetWindowPos` 仅在位置变化时调用
- ✅ 修复控制面板子项无法内嵌：使用 `IShellBrowser::BrowseObject` + SBSP 标志

## [1.1.0] - 2026-07-20

### 新增
- 基本标签栏 UI：标签 + 关闭按钮 + + 按钮
- 暗色主题配色
- 单标签切换（Navigate2）
- + 按钮新建标签默认导航到"此电脑"
- 定时器检测导航变化更新标签标题

### 修复
- 修复 Band 高度为 0 导致不可见：设置 `ptMinSize.y = 30`
- 修复 WndProc 失效导致窗口创建失败：`GetClassInfoExW` 检查类存在性

## [1.0.0] - 2026-07-15

### 新增
- 项目初始化
- DeskBand COM 组件基础框架（IDeskBand2 / IObjectWithSite / IPersistStream）
- DLL 注册/注销（regsvr32）
- 基本窗口创建和消息处理

[Unreleased]: https://github.com/xp7777/XPTabBand/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/xp7777/XPTabBand/releases/tag/v1.2.0
[1.1.0]: https://github.com/xp7777/XPTabBand/releases/tag/v1.1.0
[1.0.0]: https://github.com/xp7777/XPTabBand/releases/tag/v1.0.0
