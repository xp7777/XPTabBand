#pragma once

// ExplorerHook.h - Explorer 窗口枚举和子类化声明
// 负责：枚举所有 CabinetWClass 窗口、子类化、管理 TabBarWindow 生命周期

#include "stdafx.h"
#include "TabBarWindow.h"

namespace ExplorerHook
{
    // 枚举所有 CabinetWClass 窗口并子类化（幂等操作，可重复调用）
    void EnumAndHookAllWindows();

    // 子类化单个窗口
    //   - 替换窗口过程为 SubclassProc
    //   - 保存原始窗口过程
    //   - 创建 TabBarWindow
    void HookWindow(HWND hwnd);

    // 恢复单个窗口的原始窗口过程
    void UnhookWindow(HWND hwnd);

    // 恢复所有已子类化的窗口
    void UnhookAllWindows();

    // 定时回调：驱动所有 TabBarWindow 检查导航变化
    // 由 HookMain 的 GetMsgProc 或外部定时器调用
    void TickAllTabBars();

    // 定时回调：仅驱动当前线程拥有的 TabBarWindow 检查导航变化
    // WinEventProc 在 UI 线程执行时调用此函数（COM STA 对象有线程亲和性）
    void TickTabBarsOnCurrentThread();

    // 子类化窗口过程（导出供内部使用）
    LRESULT CALLBACK SubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
}
