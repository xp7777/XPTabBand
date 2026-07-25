Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class TabBarCheck {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    public static void Check() {
        var cabinets = new List<IntPtr>();
        EnumWindows((h, l) => {
            var cls = new StringBuilder(256);
            GetClassName(h, cls, 256);
            if (cls.ToString() == "CabinetWClass") cabinets.Add(h);
            return true;
        }, IntPtr.Zero);
        foreach (var cab in cabinets) {
            RECT r;
            GetWindowRect(cab, out r);
            Console.WriteLine("Cabinet 0x{0:X} rect=({1},{2},{3},{4}) {5}x{6}",
                cab.ToInt64(), r.Left, r.Top, r.Right, r.Bottom,
                r.Right - r.Left, r.Bottom - r.Top);
            EnumChildWindows(cab, (h2, l2) => {
                var cls2 = new StringBuilder(256);
                GetClassName(h2, cls2, 256);
                if (cls2.ToString() == "XPTabBarClass") {
                    RECT r2;
                    GetWindowRect(h2, out r2);
                    Console.WriteLine("  TabBar 0x{0:X} rect=({1},{2},{3},{4}) {5}x{6} visible={7}",
                        h2.ToInt64(), r2.Left, r2.Top, r2.Right, r2.Bottom,
                        r2.Right - r2.Left, r2.Bottom - r2.Top, IsWindowVisible(h2));
                }
                return true;
            }, IntPtr.Zero);
        }
    }
}
"@
[TabBarCheck]::Check()
