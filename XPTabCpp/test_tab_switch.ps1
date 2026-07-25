Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class ClickTest3 {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    public const int WM_LBUTTONDOWN = 0x0201;
    public const int WM_LBUTTONUP = 0x0202;
    public static IntPtr FindFirstTabBar() {
        var result = IntPtr.Zero;
        EnumWindows((h, l) => {
            var cls = new StringBuilder(256);
            GetClassName(h, cls, 256);
            if (cls.ToString() == "CabinetWClass") {
                EnumChildWindows(h, (h2, l2) => {
                    var cls2 = new StringBuilder(256);
                    GetClassName(h2, cls2, 256);
                    if (cls2.ToString() == "XPTabBarClass" && result == IntPtr.Zero) {
                        result = h2;
                    }
                    return true;
                }, IntPtr.Zero);
            }
            return true;
        }, IntPtr.Zero);
        return result;
    }
    public static IntPtr GetTabBar() {
        var tb = FindFirstTabBar();
        Console.WriteLine("TabBar hwnd=0x{0:X}", tb.ToInt64());
        return tb;
    }
    public static void Click(IntPtr tabBar, int x, int y, string label) {
        IntPtr lParam = (IntPtr)((y << 16) | (x & 0xFFFF));
        Console.WriteLine("[{0}] Click at client ({1},{2}) lParam=0x{3:X}", label, x, y, lParam.ToInt64());
        PostMessage(tabBar, WM_LBUTTONDOWN, IntPtr.Zero, lParam);
        System.Threading.Thread.Sleep(100);
        PostMessage(tabBar, WM_LBUTTONUP, IntPtr.Zero, lParam);
    }
}
"@
$tabBar = [ClickTest3]::GetTabBar()
if ($tabBar -ne [IntPtr]::Zero) {
    # Click tab 0 (center of first tab: x=75)
    [ClickTest3]::Click($tabBar, 75, 15, "Click Tab 0")
    Start-Sleep -Seconds 2
    Write-Output "=== After click tab 0 ==="
    Get-Content "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt" -Encoding UTF8 -Tail 5

    # Click tab 1 (center of second tab: x=225)
    [ClickTest3]::Click($tabBar, 225, 15, "Click Tab 1")
    Start-Sleep -Seconds 2
    Write-Output "=== After click tab 1 ==="
    Get-Content "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt" -Encoding UTF8 -Tail 5

    # Click close button of tab 1 (x = 300 - 10 = 290, close button is in right 20px of tab)
    [ClickTest3]::Click($tabBar, 290, 15, "Close Tab 1")
    Start-Sleep -Seconds 2
    Write-Output "=== After close tab 1 ==="
    Get-Content "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt" -Encoding UTF8 -Tail 5
} else {
    Write-Output "TabBar not found"
}
