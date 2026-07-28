# 截图

## 截图说明

本目录用于存放项目截图，用于 README 和文档展示。

## 已存在的图片资源

| 文件名 | 说明 | 来源 |
|--------|------|------|
| `logo.jpg` | 项目 Logo（README 头部） | AI 生成，现代深色风格 |
| `social_preview.jpg` | GitHub/Gitee 社交预览图（1280x720） | AI 生成，可上传到仓库 Settings |

如需替换为实拍截图或自定义设计，直接覆盖同名文件即可。

## 待补的截图

请补充以下截图（PNG 格式，建议宽度 1280px）：

| 文件名 | 说明 | 用途 |
|--------|------|------|
| `screenshot_main.png` | 主界面：标签栏 + 多个标签 + 收藏夹 | README 主界面展示 |
| `screenshot_dark_theme.png` | 暗色主题特写 | README 功能特性 |
| `screenshot_favorite_menu.png` | 收藏夹右键菜单 | README 使用说明 |
| `screenshot_favorite_manager.png` | 收藏夹管理菜单（上移/下移/删除） | README 收藏夹管理 |
| `screenshot_control_panel.png` | 控制面板项支持 | README 特殊文件夹 |
| `screenshot_install_wizard.png` | Inno Setup 安装向导 | README 安装说明 |
| `screenshot_uninstall_list.png` | 程序和功能卸载列表 | README 卸载说明 |

## 截图工具推荐

- **Win + Shift + S**：Windows 自带截图工具
- **ShareX**：开源截图工具，支持区域、滚动、标注
- **Greenshot**：轻量级截图工具

## 截图后

1. 把截图文件放到本目录
2. 在 README.md 中引用：`![主界面](docs/screenshot_main.png)`
3. 提交并推送

## 上传社交预览图

GitHub：仓库 Settings → Social preview → Edit → Upload `social_preview.jpg`
Gitee：仓库管理 → 仓库头像/封面设置

## 应用图标资源

除了本目录的展示图外，应用图标（用于 setup.exe 和 DLL）位于：

- `assets/XPTabBand.ico` — 源 ICO 文件（多尺寸 256/128/64/48/32/16）
- `assets/icon_source.jpg` — 源设计稿（1024x1024）
- `assets/convert_to_ico.ps1` — ICO 生成脚本
- `XPTabCpp/XPTabBand/XPTabBand.ico` — DLL 嵌入用副本
- `XPTabCpp/installer/XPTabBand.ico` — 安装包用副本

修改图标后，重新运行 `assets/convert_to_ico.ps1` 生成新的 ICO，并复制到 XPTabCpp 子目录。

