#pragma once

// HookMain.h - Hook 主逻辑声明
// 负责：安装 WH_GETMESSAGE 钩子到 Explorer 主线程、枚举子类化 Explorer 窗口

#include "stdafx.h"

namespace HookMain
{
    // 安装 Hook
    // 在独立工作线程中执行：
    //   1. 查找 Explorer 主 UI 线程
    //   2. 安装 WH_GETMESSAGE 钩子到主线程（钩子过程在主线程上下文执行）
    //   3. 设置定时器定期重试查找主线程（若尚未找到）
    //   4. 进入消息循环管理生命周期
    bool Install();

    // 卸载 Hook
    //   1. 通知工作线程退出消息循环
    //   2. 等待工作线程完成清理
    //   3. 卸载 WH_GETMESSAGE 钩子
    //   4. 恢复所有已子类化的窗口
    void Uninstall();

    // 获取工作线程句柄（供 DllMain 等待）
    HANDLE GetWorkerThread();
}
