Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class WinRestore {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool MoveWindow(IntPtr hWnd, int x, int y, int nWidth, int nHeight, bool bRepaint);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    public static void RestoreCabinets() {
        var hwnds = new List<IntPtr>();
        EnumWindows((h, l) => {
            var cls = new StringBuilder(256);
            GetClassName(h, cls, 256);
            if (cls.ToString() == "CabinetWClass") hwnds.Add(h);
            return true;
        }, IntPtr.Zero);
        int x = 100, y = 100;
        foreach (var h in hwnds) {
            ShowWindow(h, 9); // SW_RESTORE
            System.Threading.Thread.Sleep(200);
            SetForegroundWindow(h);
            System.Threading.Thread.Sleep(200);
            MoveWindow(h, x, y, 1000, 700, true);
            x += 100; y += 50;
            RECT r;
            GetWindowRect(h, out r);
            Console.WriteLine("Cabinet 0x{0:X} rect=({1},{2},{3},{4}) {5}x{6}",
                h.ToInt64(), r.Left, r.Top, r.Right, r.Bottom,
                r.Right - r.Left, r.Bottom - r.Top);
        }
    }
}
"@
[WinRestore]::RestoreCabinets()
Start-Sleep -Seconds 2
Add-Type -AssemblyName System.Windows.Forms,System.Drawing
$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
$bmp.Save("g:\Test\testFileExplorerPro\XPTabCpp\build\screenshot_tabbar.png")
$g.Dispose(); $bmp.Dispose()
Write-Output "Screenshot saved"
