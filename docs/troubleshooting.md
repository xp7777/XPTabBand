# 故障排查指南

系统性的故障排查流程，配合 [FAQ](FAQ.md) 使用。遇到问题先按本指南收集信息，再决定是否提交 Issue。

## 快速诊断脚本

把下面这段保存为 `diagnose.ps1` 以管理员身份运行，可以一次性收集大部分排查需要的信息：

```powershell
# diagnose.ps1 - XPTabBand 诊断脚本
$ErrorActionPreference = 'SilentlyContinue'

Write-Host "=== XPTabBand 诊断报告 ===" -ForegroundColor Cyan
Write-Host "生成时间: $(Get-Date)"
Write-Host ""

# 1. 系统信息
Write-Host "[1] 系统信息" -ForegroundColor Yellow
$os = Get-CimInstance Win32_OperatingSystem
Write-Host "  OS: $($os.Caption) $($os.Version) Build $($os.BuildNumber)"
Write-Host "  架构: $($os.OSArchitecture)"
Write-Host "  用户: $env:USERNAME"
Write-Host ""

# 2. XPTabBand 安装信息
Write-Host "[2] 安装信息" -ForegroundColor Yellow
$clsid = "{0E1B2C3D-4E5F-4A6B-9C7D-8E9F0A1B2C3D}"  # 示例 CLSID，按实际替换
$reg = Get-ItemProperty "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32" -ErrorAction SilentlyContinue
if ($reg) {
    Write-Host "  DLL 路径: $($reg.'(default)')"
    Write-Host "  线程模型: $($reg.ThreadingModel)"
} else {
    Write-Host "  未找到注册的 CLSID（可能未安装）" -ForegroundColor Red
}

$toolbar = Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Internet Explorer\Toolbar" -ErrorAction SilentlyContinue
$band = $toolbar.$clsid
Write-Host "  工具栏注册: $(if ($band -ne $null) { '已注册' } else { '未注册' })"
Write-Host ""

# 3. DLL 文件信息
Write-Host "[3] DLL 文件" -ForegroundColor Yellow
$dllPath = $reg.'(default)'
if ($dllPath -and (Test-Path $dllPath)) {
    $file = Get-Item $dllPath
    Write-Host "  版本: $($file.VersionInfo.FileVersion)"
    Write-Host "  大小: $([math]::Round($file.Length / 1KB, 1)) KB"
    Write-Host "  修改时间: $($file.LastWriteTime)"
} else {
    Write-Host "  DLL 文件不存在或路径为空" -ForegroundColor Red
}
Write-Host ""

# 4. 日志文件
Write-Host "[4] 日志文件" -ForegroundColor Yellow
$logPath = "$env:LOCALAPPDATA\Temp\XPTabBand_log.txt"
if (Test-Path $logPath) {
    $logFile = Get-Item $logPath
    Write-Host "  路径: $logPath"
    Write-Host "  大小: $([math]::Round($logFile.Length / 1KB, 1)) KB"
    Write-Host "  最后修改: $($logFile.LastWriteTime)"
    Write-Host "  最后 20 行:"
    Get-Content $logPath -Tail 20 | ForEach-Object { Write-Host "    $_" -ForegroundColor Gray }
} else {
    Write-Host "  日志文件不存在" -ForegroundColor Red
}
Write-Host ""

# 5. 收藏夹数据
Write-Host "[5] 收藏夹" -ForegroundColor Yellow
$favPath = "$env:APPDATA\XPTabCpp\favorites.dat"
if (Test-Path $favPath) {
    $fav = Get-Item $favPath
    Write-Host "  路径: $favPath"
    Write-Host "  大小: $($fav.Length) bytes"
    Write-Host "  最后修改: $($fav.LastWriteTime)"
} else {
    Write-Host "  无收藏夹数据（首次使用或已清空）"
}
Write-Host ""

# 6. Explorer 进程
Write-Host "[6] Explorer 进程" -ForegroundColor Yellow
$explorer = Get-Process explorer -ErrorAction SilentlyContinue
if ($explorer) {
    Write-Host "  PID: $($explorer.Id -join ', ')"
    Write-Host "  内存: $([math]::Round(($explorer | Measure-Object WorkingSet64 -Sum).Sum / 1MB, 1)) MB"
} else {
    Write-Host "  Explorer 未运行" -ForegroundColor Red
}
Write-Host ""

# 7. 最近 Explorer 崩溃记录
Write-Host "[7] 最近的 Explorer 崩溃事件" -ForegroundColor Yellow
$events = Get-WinEvent -FilterHashtable @{LogName='Application'; Level=2; ProviderName='Application Error'} -MaxEvents 10 -ErrorAction SilentlyContinue |
    Where-Object { $_.Message -match 'explorer|XPTabBand' }
if ($events) {
    $events | Select-Object -First 3 | ForEach-Object {
        Write-Host "  [$($_.TimeCreated)] $($_.Message.Substring(0, [Math]::Min(200, $_.Message.Length)))" -ForegroundColor Gray
    }
} else {
    Write-Host "  未发现崩溃记录"
}
Write-Host ""

Write-Host "=== 诊断完成 ===" -ForegroundColor Cyan
Write-Host "请把以上输出附在 Issue 中提交。"
```

运行：

```powershell
powershell -ExecutionPolicy Bypass -File .\diagnose.ps1 > diag_report.txt 2>&1
```

然后把 `diag_report.txt` 内容附在 Issue 中。

## 常见问题排查

### Q1：看不到标签栏？

按顺序检查：

1. **是否勾选了工具栏**

   打开任意文件夹 → 顶部菜单栏右键 → 查看 → 工具栏 → 勾选 XPTabBand。

   菜单栏不显示？按 `Alt` 键临时显示。

2. **DLL 是否注册**

   ```powershell
   $clsid = "{你的CLSID}"
   Get-ItemProperty "HKLM:\SOFTWARE\Classes\CLSID\$clsid\InprocServer32"
   ```

   如果 `(default)` 为空或路径不存在，重新注册：

   ```cmd
   regsvr32 "C:\Program Files\XPTabBand\XPTabBand.dll"
   ```

3. **Explorer 是否需要重启**

   任务管理器 → 找到 "Windows 资源管理器" → 右键 → 重新启动。

4. **DLL 加载是否成功**

   ```powershell
   Get-Process explorer | ForEach-Object { $_.Modules | Where-Object { $_.ModuleName -eq 'XPTabBand.dll' } }
   ```

   如果没输出，说明 DLL 没被加载。查看事件查看器中的 `SideBySide` 错误。

### Q2：勾选后 Explorer 崩溃？

立即恢复：

1. 任务管理器（Ctrl+Shift+Esc）→ 文件 → 运行新任务 → 输入 `cmd` → 勾选"以系统管理权限创建"
2. 执行：

   ```cmd
   regsvr32 /u "C:\Program Files\XPTabBand\XPTabBand.dll"
   ```

3. 任务管理器 → 结束 `explorer.exe` → 新建任务 `explorer.exe`

排查原因：

1. 事件查看器 → Windows 日志 → 应用程序 → 筛选"Application Error"
2. 找最近的 `explorer.exe` 崩溃记录，记录异常代码：
   - `0xc0000005` 访问违例：COM 指针失效或空指针
   - `0xc0000409` 栈缓冲区溢出：通常和 `RegisterClassExW` 失败有关
3. 把事件记录和 `%LOCALAPPDATA%\Temp\XPTabBand_log.txt` 一起提交 Issue

### Q3：标签栏显示异常？

**白框 + 重影**：

- 历史问题，1.2.0 已修复
- 检查版本：右键 DLL → 属性 → 详细信息
- 升级到最新版

**渲染不完整（部分元素丢失）**：

- 检查 `OnPaint` 日志中的 width/height 是否合理
- 可能是窗口尺寸为 0，检查 `GetBandInfo` 返回的尺寸
- 提交 Issue 附上日志

**鼠标悬停后元素消失**：

- ListView OwnerDraw 模式问题
- 确保 `DrawItem` 完整处理所有绘制
- 检查是否在 `DrawItem` 中调用了非 OwnerDraw 模式的方法

### Q4：导航失败？

**点击标签不切换**：

1. 查看日志中 `ActivateTab` 是否被调用
2. 查看 `NavigateToPidl` 的 HRESULT
3. 如果是特殊文件夹，查看 `BrowseObject` 的三次尝试结果

**新窗口而不是切换**：

- Windows Shell 硬编码行为，部分控制面板子项无法在当前窗口打开
- 见 [FAQ Q17](FAQ.md#q17为什么有些控制面板子项还是弹新窗口)

**收藏项打开后跳到错误位置**：

- PIDL 可能损坏
- 删除并重新收藏
- 查看日志中 PIDL 的字节大小是否合理

### Q5：安装失败？

**regsvr32 报错 0x80070005**：

权限不足。以管理员身份运行 `cmd` 再执行 `regsvr32`。

**regsvr32 报错 0x80040201**：

DLL 依赖缺失。用 [Dependencies](https://github.com/lucasg/Dependencies) 工具检查 DLL 依赖：

```cmd
Dependencies.exe -exports "C:\Program Files\XPTabBand\XPTabBand.dll"
```

**Inno Setup 编译失败**：

- 检查 `XPTabBand.iss` 中 `AppVersion` 是否正确
- 检查源 DLL 路径是否存在
- 用 `ISCC.exe` 命令行编译查看详细错误

### Q6：日志过大？

日志默认路径 `%LOCALAPPDATA%\Temp\XPTabBand_log.txt`。如果超过 10MB：

1. 关闭所有 Explorer 窗口
2. 删除日志文件
3. 重新打开窗口

日志过大说明 `OnPaint` 触发过于频繁，提交 Issue 附上日志片段。

## 日志详解

日志格式：`[时间] [级别] [线程ID] 消息`

| 级别 | 含义 |
| ---- | ---- |
| INFO | 正常流程信息 |
| WARN | 警告，不影响功能 |
| ERROR | 错误，可能影响部分功能 |
| FATAL | 严重错误，可能导致 Explorer 崩溃 |

关键日志条目：

| 日志 | 含义 |
| ---- | ---- |
| `GetBandInfo called` | Explorer 查询 Band 信息 |
| `SetSite: getting IWebBrowser2` | 获取浏览器接口 |
| `OnPaint w=1351 h=30` | 绘制（宽=1351 高=30） |
| `NavigateToPidl hr=0x00000000` | 导航成功 |
| `NavigateToPidl hr=0x80004005` | 导航失败（E_FAIL） |
| `BrowseObject try 1 hr=0x00000000` | BrowseObject 成功 |
| `RegisterClassExW failed` | 窗口类注册失败 |
| `CreateWindowExW returned NULL` | 窗口创建失败 |
| `SEH caught: 0xc0000005` | SEH 异常捕获 |

## 提交 Issue 前的最终检查清单

- [ ] 已查阅 [FAQ](FAQ.md) 和 [已知限制](../README.md#已知限制)
- [ ] 已运行 `diagnose.ps1` 并附上输出
- [ ] 已附上 Windows 版本（winver 输出）
- [ ] 已附上 XPTabBand 版本
- [ ] 日志中用户名已脱敏（替换为 `<USER>`）
- [ ] 如果是崩溃，已附上事件查看器记录
- [ ] 复现步骤已写到可以照做重现的程度
