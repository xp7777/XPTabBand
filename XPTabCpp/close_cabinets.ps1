Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class WinClose {
    public delegate bool P(IntPtr h, IntPtr l);
    [DllImport("user32.dll")] public static extern bool EnumWindows(P f, IntPtr l);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr h, StringBuilder s, int n);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    public static void CloseAll() {
        var list = new List<IntPtr>();
        EnumWindows((h, l) => {
            var s = new StringBuilder(256);
            GetClassName(h, s, 256);
            if (s.ToString() == "CabinetWClass") list.Add(h);
            return true;
        }, IntPtr.Zero);
        foreach (var h in list) { PostMessage(h, 0x0010, IntPtr.Zero, IntPtr.Zero); }
        Console.WriteLine("Closed " + list.Count + " CabinetWClass windows");
    }
}
"@
[WinClose]::CloseAll()
