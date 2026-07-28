# 常见问题（FAQ）

汇总用户使用过程中最常遇到的问题。更系统的故障排查流程见 [故障排查指南](troubleshooting.md)。

## 目录

- [安装与卸载](#安装与卸载)
- [标签栏显示](#标签栏显示)
- [Explorer 崩溃](#explorer-崩溃)
- [收藏夹](#收藏夹)
- [特殊文件夹](#特殊文件夹)
- [性能](#性能)
- [与其他软件兼容性](#与其他软件兼容性)

## 安装与卸载

### Q1：SmartScreen 拦截安装包怎么办？

点击 **"更多信息"** → **"仍要运行"**。这是因为安装包未数字签名，非安全问题。

> 如想永久关闭 SmartScreen，可在 Windows 安全中心 → 应用和浏览器控制 → 基于声誉的保护设置中关闭，但不推荐。

### Q2：杀毒软件报毒？

通常是 `regsvr32` 注册 DLL 触发的误报。把安装目录加入白名单即可。

如果仍然报警，可以：

1. 在 [virustotal.com](https://www.virustotal.com) 上传 `XPTabBand.dll` 复查
2. 用源码自行编译（见 [开发指南](development.md)）
3. 在 [Issue](https://github.com/xp7777/XPTabBand/issues) 中附上杀软名称和报告内容

### Q3：安装后看不到工具栏？

1. 打开任意文件夹窗口
2. 顶部菜单栏 **右键** → **查看** → **工具栏** → 勾选 **XPTabBand**
3. 如果菜单栏没显示，按 `Alt` 键临时显示

详细排查见 [故障排查指南](troubleshooting.md#q1看不到标签栏)。

### Q4：卸载后还残留注册表项？

正常情况下 `unins000.exe` 会清理：

- `HKCR\CLSID\{CLSID_XPTabBand}`
- `HKLM\SOFTWARE\Microsoft\Internet Explorer\Toolbar\{CLSID_XPTabBand}`
- `HKCR\Component Categories\...`

如果手动删除过文件，注册表会残留。运行 `XPTabCpp\deploy\uninstall.bat` 以管理员身份清理。

### Q5：升级版本怎么操作？

1. 控制面板 → 程序和功能 → 卸载旧版（保留收藏夹数据）
2. 安装新版 setup.exe
3. 重启 Explorer（任务管理器 → Windows 资源管理器 → 重新启动）

收藏夹数据在 `%APPDATA%\XPTabCpp\favorites.dat`，卸载时选"保留"即可平滑升级。

### Q6：能装在 32 位系统上吗？

不能。XPTabBand 仅编译了 x64 版本，32 位系统会注册失败。

## 标签栏显示

### Q7：标签栏显示空白框？

通常是首次安装后 Explorer 未正确加载 DLL：

1. 任务管理器 → 找到"Windows 资源管理器" → 右键 → 重新启动
2. 重新勾选工具栏
3. 查看 `%LOCALAPPDATA%\Temp\XPTabBand_log.txt` 是否有错误日志

### Q8：标签栏显示重影？

历史问题，1.2.0 已修复。如果你还在 1.1.x，请升级到 1.2.0+。

如新版本仍出现重影，可能是双缓冲位图尺寸和窗口尺寸不匹配，提交 [Issue](https://github.com/xp7777/XPTabBand/issues) 并附上日志。

### Q9：标签栏占用太多空间？

标签栏固定占用 30px 高度，无法在设置中调整。如果想隐藏，取消勾选工具栏即可。

未来版本计划支持配置文件调整标签栏高度，见 [CHANGELOG](../CHANGELOG.md) 的"计划中"。

### Q10：标签标题显示不完整？

标签宽度有限，超出部分会显示省略号 `...`。这是预期行为（参考 Chrome）。

鼠标悬停时不会自动展开 tooltip，后续版本会加上。

## Explorer 崩溃

### Q11：勾选 XPTabBand 后 Explorer 崩溃？

立即操作：

1. 取消勾选（如果还能操作）
2. 任务管理器 → 结束 `explorer.exe` → 新建任务 `explorer.exe`
3. 查看事件查看器 → Windows 日志 → 应用程序 → 找 XPTabBand 相关错误
4. 提交 [Issue](https://github.com/xp7777/XPTabBand/issues) 并附上事件日志

### Q12：关闭文件夹后 Explorer 自动重启？

历史问题，1.2.0 已修复：

- `RegisterClassExW` 失败时不再调用 `CreateWindowExW`
- `SetSite` 中 NULL hwnd 检查
- COM 调用加 SEH 包装

如新版本仍出现，请提交 Issue 并附上：

- Windows 事件查看器中 `XPTabBand.dll_unloaded` 的异常代码（如 `0xc0000005`）
- `%LOCALAPPDATA%\Temp\XPTabBand_log.txt`

## 收藏夹

### Q13：收藏夹数据保存在哪？

`%APPDATA%\XPTabCpp\favorites.dat`

格式见 [架构文档](architecture.md#收藏夹持久化)。普通用户无需关心，卸载向导会询问是否保留。

### Q14：收藏项打不开？

可能原因：

- 文件夹已被移动或删除（PIDL 失效）
- 是 Windows Shell 不支持内嵌打开的特殊文件夹
- COM 权限问题

排查方法：

1. 查看日志中 `BrowseObject` 的 HRESULT
2. 如果是特殊文件夹（如网络连接），可能是 Windows Shell 限制（见 [Q17](#q17为什么有些控制面板子项还是弹新窗口)）

### Q15：收藏夹数据损坏怎么办？

关闭所有 Explorer 窗口，删除 `%APPDATA%\XPTabCpp\favorites.dat`，重新打开窗口，收藏夹会重置为空。

### Q16：能把收藏夹导出/导入吗？

当前版本不支持。计划中功能，见 [CHANGELOG](../CHANGELOG.md)。

## 特殊文件夹

### Q17：为什么有些控制面板子项还是弹新窗口？

这是 Windows Shell 的硬编码行为，无法通过 `SBSP_SAMEBROWSER` 标志覆盖。详见 [架构文档 - 特殊文件夹导航](architecture.md#特殊文件夹导航)。

未来可能通过窗口创建钩子（`SetWindowsHookEx` + `WH_CBT`）拦截新窗口并重定向到新标签，目前未实现。

### Q18：收藏"此电脑"后打不开？

应该可以正常打开。如果不行，请检查：

1. 是否最近重命名过"此电脑"
2. PIDL 是否损坏（删除并重新收藏）
3. 提交 Issue 附上日志

### Q19：网络文件夹支持吗？

支持，但导航到网络节点时可能比较慢（取决于网络发现响应）。Windows 在枚举网络邻居时会阻塞 UI 线程，这是 Shell 本身的限制。

## 性能

### Q20：Explorer 变卡了？

可能原因：

- 日志文件过大（>10MB）：删除 `%LOCALAPPDATA%\Temp\XPTabBand_log.txt`
- 标签数量过多：建议不超过 15 个
- 当前文件夹有大量文件：ListView 渲染是 Explorer 自己的事，与 XPTabBand 无关

### Q21：CPU 占用高？

检查 `OnPaint` 调用频率。日志中如果 1 秒内出现多条 `OnPaint` 记录，说明触发频率过高，请提交 Issue。

### Q22：内存占用高？

正常情况下 XPTabBand.dll 在 explorer.exe 中占用 < 5MB。如果显著超出：

- 可能是 PIDL 缓存泄漏
- 可能是双缓冲位图未释放
- 请提交 Issue 附上任务管理器截图

## 与其他软件兼容性

### Q23：和 QTTabBar 同时安装可以吗？

**不推荐**。两者都是 DeskBand，会争抢工具栏空间和 COM 资源。建议二选一。

如果想迁移，先卸载 QTTabBar 再装 XPTabBand。

### Q24：和 ExplorerPatcher 兼容吗？

通常兼容，但 ExplorerPatcher 修改了 Shell 的部分行为，可能影响 XPTabBand 的导航流程。如果出现问题，先禁用 ExplorerPatcher 测试。

### Q25：在 IE 兼容模式下能用吗？

不能。XPTabBand 是 Shell 扩展，不适用于 IE。在 IE 中打开文件夹时不会显示标签栏。

### Q26：和 StartAllBack / Start11 兼容吗？

通常兼容。这些工具修改的是开始菜单和任务栏，不影响资源管理器工具栏。

---

没找到你的问题？欢迎 [提交 Issue](https://github.com/xp7777/XPTabBand/issues/new/choose)。
