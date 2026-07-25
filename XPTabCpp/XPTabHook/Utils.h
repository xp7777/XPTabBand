#pragma once

// Utils.h - 工具函数声明
// 提供日志、路径获取等通用功能

#include "stdafx.h"

namespace Utils
{
    // 获取日志文件路径：%LOCALAPPDATA%\XPTabCpp\hook_log.txt
    // 如果目录不存在会自动创建
    std::wstring GetLogPath();

    // 写日志（带时间戳），追加写入日志文件
    void Log(const std::wstring& msg);

    // 获取 XPTabHook.dll 的完整路径
    std::wstring GetModulePath();

    // 获取当前模块（XPTabHook.dll）的句柄
    HMODULE GetThisModule();

    // 调试追踪：用纯 Win32 API 写到固定路径（绕过 CRT 和 Shell 依赖）
    // 用于诊断 DllMain/工作线程是否执行
    void DebugTrace(const char* msg);
}
