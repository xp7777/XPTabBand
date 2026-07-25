Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class ChildEnum {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool ScreenToClient(IntPtr hWnd, ref POINT lpPoint);
    [DllImport("user32.dll")] public static extern IntPtr GetWindow(IntPtr hWnd, uint uCmd);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)]
    public struct POINT { public int X, Y; }
    public static void ShowChildren(IntPtr parent) {
        var list = new List<IntPtr>();
        EnumChildWindows(parent, (h, l) => { list.Add(h); return true; }, IntPtr.Zero);
        Console.WriteLine("子窗口数量: " + list.Count);
        foreach (var h in list) {
            var cls = new StringBuilder(256);
            GetClassName(h, cls, 256);
            var title = new StringBuilder(256);
            GetWindowText(h, title, 256);
            RECT r;
            GetWindowRect(h, out r);
            POINT p = new POINT { X = r.Left, Y = r.Top };
            ScreenToClient(parent, ref p);
            Console.WriteLine("  hwnd=0x{0:X} class={1} title={2} screen=({3},{4},{5},{6}) client=({7},{8}) size={9}x{10}",
                h.ToInt64(), cls, title, r.Left, r.Top, r.Right, r.Bottom, p.X, p.Y,
                r.Right - r.Left, r.Bottom - r.Top);
        }
    }
    public static void FindCabinet() {
        EnumWindows((h, l) => {
            var cls = new StringBuilder(256);
            GetClassName(h, cls, 256);
            if (cls.ToString() == "CabinetWClass") {
                RECT r;
                GetWindowRect(h, out r);
                RECT cr;
                GetClientRect(h, out cr);
                Console.WriteLine("=== CabinetWClass 0x{0:X} ===", h.ToInt64());
                Console.WriteLine("  window rect=({0},{1},{2},{3}) {4}x{5}", r.Left, r.Top, r.Right, r.Bottom, r.Right-r.Left, r.Bottom-r.Top);
                Console.WriteLine("  client rect=({0},{1},{2},{3}) {4}x{5}", cr.Left, cr.Top, cr.Right, cr.Bottom, cr.Right-cr.Left, cr.Bottom-cr.Top);
                ShowChildren(h);
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@
[ChildEnum]::FindCabinet()
