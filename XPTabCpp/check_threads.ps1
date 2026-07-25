Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class WinInfo2 {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);
    public static void Dump() {
        EnumWindows((h, l) => {
            var cls = new StringBuilder(256);
            GetClassName(h, cls, 256);
            if (cls.ToString() == "CabinetWClass") {
                uint procId;
                uint tid = GetWindowThreadProcessId(h, out procId);
                var title = new StringBuilder(256);
                GetWindowText(h, title, 256);
                Console.WriteLine("Hwnd=0x{0:X} Tid={1} Pid={2} Title={3}",
                    h.ToInt64(), tid, procId, title.ToString());
            }
            return true;
        }, IntPtr.Zero);
    }
}
"@
[WinInfo2]::Dump()
