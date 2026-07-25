Add-Type @"
using System;
using System.Runtime.InteropServices;
using System.Text;
using System.Collections.Generic;
public class WinUtil {
    public delegate bool EnumWindowsProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] public static extern bool EnumWindows(EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetClassName(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hWnd, StringBuilder lpString, int nMaxCount);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT { public int Left, Top, Right, Bottom; }
    public static void ShowCabinetWindows() {
        var cabinets = new List<IntPtr>();
        EnumWindows((hWnd, lParam) => {
            var cls = new StringBuilder(256);
            GetClassName(hWnd, cls, 256);
            if (cls.ToString() == "CabinetWClass") cabinets.Add(hWnd);
            return true;
        }, IntPtr.Zero);
        foreach (var cab in cabinets) {
            var title = new StringBuilder(256);
            GetWindowText(cab, title, 256);
            RECT r;
            GetWindowRect(cab, out r);
            Console.WriteLine("Cabinet: hwnd=0x{0:X} title={1} rect=({2},{3},{4},{5}) {6}x{7}",
                cab.ToInt64(), title, r.Left, r.Top, r.Right, r.Bottom,
                r.Right - r.Left, r.Bottom - r.Top);
            // 恢复窗口（SW_RESTORE = 9）
            ShowWindow(cab, 9);
            System.Threading.Thread.Sleep(200);
            SetForegroundWindow(cab);
            // 重新获取 rect
            GetWindowRect(cab, out r);
            Console.WriteLine("  after restore: rect=({0},{1},{2},{3}) {4}x{5}",
                r.Left, r.Top, r.Right, r.Bottom, r.Right - r.Left, r.Bottom - r.Top);
            // 列出子窗口中的 XPTabBarClass
            EnumChildWindows(cab, (hWnd2, lParam2) => {
                var cls2 = new StringBuilder(256);
                GetClassName(hWnd2, cls2, 256);
                if (cls2.ToString() == "XPTabBarClass") {
                    RECT r2;
                    GetWindowRect(hWnd2, out r2);
                    Console.WriteLine("  TabBar: hwnd=0x{0:X} rect=({1},{2},{3},{4}) {5}x{6} visible={7}",
                        hWnd2.ToInt64(), r2.Left, r2.Top, r2.Right, r2.Bottom,
                        r2.Right - r2.Left, r2.Bottom - r2.Top, IsWindowVisible(hWnd2));
                }
                return true;
            }, IntPtr.Zero);
        }
    }
}
"@
[WinUtil]::ShowCabinetWindows()
Start-Sleep -Seconds 1
Write-Output "=== Screenshot ==="
Add-Type -AssemblyName System.Windows.Forms,System.Drawing
$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
$bmp.Save("g:\Test\testFileExplorerPro\XPTabCpp\build\screenshot2.png")
$g.Dispose(); $bmp.Dispose()
Write-Output "Screenshot saved: g:\Test\testFileExplorerPro\XPTabCpp\build\screenshot2.png"
