Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class ChildEnum {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll")] public static extern IntPtr GetParent(IntPtr hWnd);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    public static void Enumerate(IntPtr parent, int depth) {
        EnumChildWindows(parent, (h, l) => {
            var cls = new StringBuilder(256);
            GetClassName(h, cls, 256);
            RECT r;
            GetWindowRect(h, out r);
            string indent = new string(' ', depth * 2);
            Console.WriteLine("{0}0x{1:X} {2} rect=({3},{4},{5},{6}) {7}x{8}",
                indent, h.ToInt64(), cls.ToString(),
                r.Left, r.Top, r.Right, r.Bottom,
                r.Right - r.Left, r.Bottom - r.Top);
            if (depth < 2) Enumerate(h, depth + 1);
            return true;
        }, IntPtr.Zero);
    }
    public static void FindCabinets() {
        var cabinets = new List<IntPtr>();
        EnumWindows((h, l) => {
            var cls = new StringBuilder(256);
            GetClassName(h, cls, 256);
            if (cls.ToString() == "CabinetWClass") cabinets.Add(h);
            return true;
        }, IntPtr.Zero);
        foreach (var cab in cabinets) {
            Console.WriteLine("=== Cabinet 0x{0:X} ===", cab.ToInt64());
            Enumerate(cab, 1);
        }
    }
}
"@
[ChildEnum]::FindCabinets()
