#pragma once

// stdafx.h - XPTabHook 预编译头
// 包含常用的 Windows 头文件、标准库和 MinHook

// 目标系统：Windows 10
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

// 不定义 WIN32_LEAN_AND_MEAN，因为 shellapi.h/shlobj.h 需要完整声明

// Windows 核心头文件（必须最先包含）
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <shobjidl.h>
#include <shlobj.h>

// 标准库
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>

// MinHook 库暂时未启用（table64.h 缺失，无法下载）
// 当前窗口子类化 + WH_CBT 钩子不需要 MinHook
// #include "MinHook.h"
