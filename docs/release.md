# 发布流程

XPTabBand 的版本发布流程，仅维护者使用。遵循 [语义化版本](https://semver.org/lang/zh-CN/) 2.0.0。

## 版本号规则

格式：`MAJOR.MINOR.PATCH`

| 位 | 何时增加 | 示例 |
| --- | --- | --- |
| MAJOR | 不兼容的 API/行为变更 | 1.x → 2.0.0 |
| MINOR | 向后兼容的新功能 | 1.2.x → 1.3.0 |
| PATCH | 向后兼容的 Bug 修复 | 1.2.0 → 1.2.1 |

预发布版本：`1.3.0-alpha.1`、`1.3.0-beta.1`、`1.3.0-rc.1`

## 发布前准备

### 1. 确认变更范围

```cmd
:: 查看自上一个 tag 以来的所有变更
git log v1.2.0..HEAD --oneline
```

确认：

- 所有合并的 PR 都已记录在 [CHANGELOG.md](../CHANGELOG.md) 的 `[Unreleased]` 段落
- 没有遗漏的 fix 或 feat
- Breaking change 是否需要升级 MAJOR

### 2. 更新 CHANGELOG

把 [CHANGELOG.md](../CHANGELOG.md) 中的 `[Unreleased]` 段落改写为 `[1.3.0] - YYYY-MM-DD`：

```markdown
## [1.3.0] - 2026-08-15

### 新增
- 标签拖拽排序
- ...

### 改进
- ...

### 修复
- ...

## [Unreleased]

### 计划中
- 下一版本的功能...
```

并在文件末尾的链接段落补上：

```markdown
[Unreleased]: https://github.com/xp7777/XPTabBand/compare/v1.3.0...HEAD
[1.3.0]: https://github.com/xp7777/XPTabBand/releases/tag/v1.3.0
```

### 3. 更新版本号

需要修改的位置：

| 文件 | 修改 |
| --- | --- |
| [XPTabCpp/installer/XPTabBand.iss](../XPTabCpp/installer/XPTabBand.iss) | `AppVersion` 字段 |
| [README.md](../README.md) | 版本徽章（如 `version-1.3.0-brightgreen`） |
| [XPTabBand/dllmain.cpp](../XPTabCpp/XPTabBand/dllmain.cpp) | DllRegisterServer 写入注册表的版本（如有） |
| [deploy/README.md](../XPTabCpp/deploy/README.md) | 提及的版本号（如有） |

### 4. 编译验证

```cmd
:: 清理旧产物
rmdir /s /q XPTabCpp\XPTabBand\build
rmdir /s /q XPTabCpp\installer\Output

:: 卸载当前已注册的 DLL
cd XPTabCpp\deploy
call uninstall.bat
cd ..\..

:: 编译 Release
cd XPTabCpp
MSBuild XPTab.sln /p:Configuration=Release /p:Platform=x64 /t:Rebuild
cd ..

:: 编译安装包
cd XPTabCpp\installer
"C:\Program Files (x86)\Inno Setup 6\ISCC.exe" XPTabBand.iss
cd ..\..
```

确认产物：

- `XPTabCpp\XPTabBand\build\Release\XPTabBand.dll`
- `XPTabCpp\installer\Output\XPTabBand-1.3.0-setup.exe`

### 5. 冒烟测试

在干净的虚拟机或测试机上：

- [ ] 双击 setup.exe 安装成功
- [ ] "程序和功能"列表中出现 XPTabBand 1.3.0
- [ ] 打开文件夹 → 勾选工具栏 → 标签栏显示正常
- [ ] 创建/切换/关闭标签 5 次，无崩溃
- [ ] 添加/删除/排序收藏项
- [ ] 测试特殊文件夹（控制面板子项、网络连接）
- [ ] 关闭所有窗口后再次打开，Explorer 不自动重启
- [ ] 卸载成功，"程序和功能"列表中消失
- [ ] 卸载时选择保留收藏夹，重装后数据还在

## 创建 Tag 并推送

```cmd
:: 提交所有版本号和 CHANGELOG 变更
git add -A
git commit -m "chore: 准备发布 v1.3.0"

:: 创建带注释的 tag
git tag -a v1.3.0 -m "Release v1.3.0

主要变更：
- 新增标签拖拽排序
- 修复 XX 崩溃问题
详见 CHANGELOG.md"

:: 推送到两个远端
git push github main
git push github v1.3.0
git push gitee main
git push gitee v1.3.0
```

推送 tag 后，[GitHub Actions](../.github/workflows/build.yml) 的 `inno-setup` job 会自动编译并上传 `setup.exe` 作为构建产物。

## 创建 GitHub Release

1. 打开 <https://github.com/xp7777/XPTabBand/releases/new>
2. Choose a tag → 选择 `v1.3.0`
3. Release title: `XPTabBand v1.3.0`
4. Description 粘贴 CHANGELOG 对应章节：

   ```markdown
   ## 🎉 XPTabBand v1.3.0

   ### 新增
   - 标签拖拽排序
   - ...

   ### 改进
   - ...

   ### 修复
   - ...

   **完整变更**：[CHANGELOG.md](../CHANGELOG.md#130---2026-08-15)

   ## 安装
   下载下方 `XPTabBand-1.3.0-setup.exe`，双击运行，按向导完成。

   已安装旧版本？直接运行新 setup.exe 即可升级，收藏夹数据会保留。
   ```

5. Attach binaries：
   - 上传 `XPTabCpp\installer\Output\XPTabBand-1.3.0-setup.exe`
   - （可选）上传 `XPTabBand.dll`
6. 勾选 "Set as the latest release"（如果是最新）
7. Publish release

## 创建 Gitee Release

1. 打开 <https://gitee.com/yxp1108/XPTabBand/releases/new>
2. Tag → `v1.3.0`
3. Title: `XPTabBand v1.3.0`
4. Description 粘贴相同内容（注意把 GitHub 链接改成 Gitee 链接）
5. 上传 setup.exe
6. 发布

## 计算校验和

为安全起见，发布后计算 SHA-256 并附在 Release 描述中：

```cmd
certutil -hashfile XPTabBand-1.3.0-setup.exe SHA256
```

输出类似：

```
SHA256 hash of XPTabBand-1.3.0-setup.exe:
a1 b2 c3 d4 e5 f6 ...
CertUtil: -hashfile command completed successfully.
```

格式化后附在 Release 描述末尾：

```markdown
## 校验和

| 文件 | SHA-256 |
| --- | --- |
| XPTabBand-1.3.0-setup.exe | a1b2c3d4e5f6... |
```

## 发布后检查

- [ ] GitHub Release 页面正常显示
- [ ] Gitee Release 页面正常显示
- [ ] README 中的 Release 徽章指向最新版本
- [ ] 下载并重新安装一次，确认 SHA-256 与发布的一致
- [ ] 在 Discussions 或 Release 描述中通知用户（如有 Breaking change）
- [ ] 关闭里程碑（如果用了 GitHub Milestones）

## 紧急回滚

如果发布后发现严重问题：

1. 不要删除已发布的 Release（用户可能已下载）
2. 立即发布 PATCH 版本（如 `v1.3.1`）
3. 在 `v1.3.0` Release 描述顶部加红色警告：

   ```markdown
   > ⚠️ **本版本存在严重 Bug，请升级到 [v1.3.1](v1.3.1)**
   ```

4. 在 GitHub Issue 中置顶说明

## 发布周期

- **小版本（PATCH）**：随时，修复紧急 Bug
- **中版本（MINOR）**：约 1-2 个月，积攒足够新功能后发布
- **大版本（MAJOR）**：仅在重大重构或不兼容变更时发布，预计 1 年以上一次
