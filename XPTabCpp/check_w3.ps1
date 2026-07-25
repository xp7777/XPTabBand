Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
public class W3 {
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    public static void Run() {
        EnumWindows((h, l) => {
            var cls = new StringBuilder(256);
            GetClassName(h, cls, 256);
            if (cls.ToString() == "CabinetWClass") {
                var title = new StringBuilder(256);
                GetWindowText(h, title, 256);
                RECT r;
                GetWindowRect(h, out r);
                Console.WriteLine("Cabinet hwnd=0x{0:X} title={1} visible={2} rect=({3},{4},{5},{6}) {7}x{8}",
                    h.ToInt64(), title, IsWindowVisible(h), r.Left, r.Top, r.Right, r.Bottom,
                    r.Right - r.Left, r.Bottom - r.Top);
                EnumChildWindows(h, (h2, l2) => {
                    var cls2 = new StringBuilder(256);
                    GetClassName(h2, cls2, 256);
                    if (cls2.ToString() == "XPTabBarClass") {
                        RECT r2;
                        GetWindowRect(h2, out r2);
                        Console.WriteLine("  TabBar hwnd=0x{0:X} visible={1} rect=({2},{3},{4},{5}) {6}x{7}",
                            h2.ToInt64(), IsWindowVisible(h2), r2.Left, r2.Top, r2.Right, r2.Bottom,
                            r2.Right - r2.Left, r2.Bottom - r2.Top);
                    }
                    return true;
                }, IntPtr.Zero);
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@
[W3]::Run()
