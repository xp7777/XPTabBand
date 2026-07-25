#pragma once

// stdafx.h - XPTabInject 预编译头
// 包含常用的 Windows 头文件和标准库

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// 目标系统：Windows 10
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <psapi.h>
#include <tlhelp32.h>

// 标准库
#include <string>
#include <vector>
#include <iostream>
#include <io.h>
#include <fcntl.h>
