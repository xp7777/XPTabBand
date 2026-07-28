---
name: Bug 报告
about: 报告 XPTabBand 的问题
title: "[Bug] "
labels: bug
assignees: ''
---

## 问题描述

简洁清晰地描述问题是什么。

## 复现步骤

1. 打开 '...'
2. 点击 '....'
3. 滚动到 '....'
4. 出现问题

## 期望行为

描述应该发生什么。

## 实际行为

描述实际发生了什么。

## 截图

如果适用，添加截图说明问题。

## 环境信息

- **操作系统**：[例如 Windows 11 23H2、Windows 10 22H2]
- **系统版本**：[运行 `winver` 查看，例如 22631.4317]
- **XPTabBand 版本**：[例如 1.2.0]
- **安装方式**[ ] EXE 安装包 [ ] BAT 脚本 [ ] 源码编译
- **是否勾选了工具栏**：[是/否]

## 日志文件

请附上日志文件内容。日志位于：

```
%LOCALAPPDATA%\Temp\XPTabBand_log.txt
```

可以用以下命令查看：

```cmd
type %LOCALAPPDATA%\Temp\XPTabBand_log.txt
```

或 PowerShell：

```powershell
Get-Content "$env:LOCALAPPDATA\Temp\XPTabBand_log.txt" -Tail 50
```

## 其他信息

补充其他有助于复现问题的信息。
