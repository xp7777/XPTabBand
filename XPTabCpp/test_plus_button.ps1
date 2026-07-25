Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class ClickTest2 {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr PostMessage(IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr hWnd);
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
    public static void ClickPlus() {
        var tabBar = FindFirstTabBar();
        if (tabBar == IntPtr.Zero) {
            Console.WriteLine("TabBar not found");
            return;
        }
        Console.WriteLine("TabBar hwnd=0x{0:X}", tabBar.ToInt64());
        // + button is at client x = 150 + 14 = 164, y = 15
        int x = 164;
        int y = 15;
        IntPtr lParam = (IntPtr)((y << 16) | (x & 0xFFFF));
        Console.WriteLine("PostMessage WM_LBUTTONDOWN at client ({0},{1}) lParam=0x{2:X}", x, y, lParam.ToInt64());
        PostMessage(tabBar, WM_LBUTTONDOWN, IntPtr.Zero, lParam);
        System.Threading.Thread.Sleep(100);
        PostMessage(tabBar, WM_LBUTTONUP, IntPtr.Zero, lParam);
        Console.WriteLine("Click sent");
    }
}
"@
[ClickTest2]::ClickPlus()
Start-Sleep -Seconds 2
Write-Output "=== hook_log tail ==="
Get-Content "$env:LOCALAPPDATA\XPTabCpp\hook_log.txt" -Encoding UTF8 -Tail 15
