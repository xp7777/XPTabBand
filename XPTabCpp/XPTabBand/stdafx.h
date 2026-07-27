#pragma once

// stdafx.h - XPTabBand 预编译头
// DeskBand COM 组件，作为 Explorer 工具栏扩展被加载

// 目标系统：Windows 10
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

// Windows 核心头文件
#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <shobjidl.h>
#include <shlobj.h>

// COM
#include <objbase.h>
#include <oleidl.h>
#include <shlguid.h>

// 标准++>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>

// XPTabBand 导入宏
#ifdef XPTABBAND_EXPORTS
#define XPTABBAND_API __declspec(dllexport)
#else
#define XPTABBAND_API __declspec(dllimport)
#endif
