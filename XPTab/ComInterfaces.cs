using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace XPTab
{
    #region COM 接口声明

    /// <summary>
    /// IUnknown —— COM 根接口
    /// </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("00000000-0000-0000-C000-000000000046")]
    public interface IUnknown
    {
        [PreserveSig]
        int QueryInterface(ref Guid riid, out IntPtr ppv);
        [PreserveSig]
        int AddRef();
        [PreserveSig]
        int Release();
    }

    /// <summary>
    /// IOleWindow —— 获取窗口句柄（IDeskBand 继承自此）
    /// </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("00000114-0000-0000-C000-000000000046")]
    public interface IOleWindow
    {
        /// <summary>获取窗口句柄</summary>
        [PreserveSig]
        int GetWindow(out IntPtr phwnd);

        /// <summary>启用/禁用无模式对话框</summary>
        [PreserveSig]
        int ContextSensitiveHelp([MarshalAs(UnmanagedType.Bool)] bool fEnterMode);
    }

    /// <summary>
    /// IDockingWindow —— 停靠窗口（IDeskBand 继承自此）
    /// 通过接口继承声明 vtable 顺序：IOleWindow → IDockingWindow
    /// 注意：ResizeBorderDW 的签名必须与 CSDeskBand/Windows SDK 完全一致，
    /// 否则 x64 栈布局错位导致后续 GetBandInfo 参数全乱。
    /// </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("012DD920-7B26-11D0-8CA9-00A0C92DBFE8")]
    public interface IDockingWindow : IOleWindow
    {
        [PreserveSig]
        int ShowDW([MarshalAs(UnmanagedType.Bool)] bool fShow);

        [PreserveSig]
        int CloseDW([In] uint dwReserved);

        [PreserveSig]
        int ResizeBorderDW(
            RECT prcBorder,
            [In, MarshalAs(UnmanagedType.IUnknown)] IntPtr punkToolbarSite,
            [MarshalAs(UnmanagedType.Bool)] bool fReserved);
    }

    /// <summary>
    /// IDeskBand —— Explorer 工具栏 Band 接口
    /// vtable 顺序：IOleWindow → IDockingWindow → IDeskBand
    /// 注意：GetBandInfo 的第三个参数用 IntPtr 接收原始指针，
    /// 避免 marshaler 对 ref struct 做额外栈操作导致参数错位。
    /// </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("EB0FE172-1A3A-11D0-89B3-00A0C90A90AC")]
    public interface IDeskBand : IDockingWindow
    {
        /// <summary>获取 Band 的最小/最大/理想尺寸</summary>
        [PreserveSig]
        int GetBandInfo(
            uint dwBandID,
            uint dwViewMode,
            IntPtr pdbi);
    }

    /// <summary>
    /// IDeskBand2 —— 带 Vista+ 高级特性（透明度等），Win10/11 实际用此接口
    /// vtable 顺序：IOleWindow → IDockingWindow → IDeskBand → IDeskBand2
    /// </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("79D16DE4-ABEE-4021-8D9D-9169B2BFFD49")]
    public interface IDeskBand2 : IDeskBand
    {
        [PreserveSig]
        int CanRenderComposited([MarshalAs(UnmanagedType.Bool)] out bool pfCanRenderComposited);

        [PreserveSig]
        int SetCompositionState([MarshalAs(UnmanagedType.Bool)] bool fCompositionEnabled);

        [PreserveSig]
        int GetCompositionState([MarshalAs(UnmanagedType.Bool)] out bool pfCompositionEnabled);
    }

    /// <summary>
    /// IObjectWithSite —— 获取 Explorer 的站点对象（IServiceProvider）
    /// 通过站点可获取 IShellBrowser，进而控制 Explorer 导航
    /// </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("FC4801A3-2BA9-11CF-A229-00AA003D7352")]
    public interface IObjectWithSite
    {
        [PreserveSig]
        int SetSite([MarshalAs(UnmanagedType.IUnknown)] object pUnkSite);

        [PreserveSig]
        int GetSite(ref Guid riid, out IntPtr ppvSite);
    }

    /// <summary>
    /// IPersist —— 持久化接口（Explorer 创建 Band 时会查询）
    /// </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("0000010C-0000-0000-C000-000000000046")]
    public interface IPersist
    {
        void GetClassID(out Guid pClassID);
    }

    /// <summary>
    /// IPersistStream —— 持久化 Band 配置（Explorer 会调用保存/加载状态）
    /// </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("00000109-0000-0000-C000-000000000046")]
    public interface IPersistStream : IPersist
    {
        [PreserveSig]
        int IsDirty();

        void Load(IntPtr pStm);
        void Save(IntPtr pStm, [MarshalAs(UnmanagedType.Bool)] bool fClearDirty);
        void GetSizeMax(out ulong pcbSize);
    }

    /// <summary>
    /// IInputObject —— 键盘焦点处理（Band 需要响应快捷键时实现）
    /// </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("68284FAA-6A48-11D0-8C78-00C04FD918B4")]
    public interface IInputObject
    {
        [PreserveSig]
        int UIActivateIO([MarshalAs(UnmanagedType.Bool)] bool fActivate, ref MSG msg);

        [PreserveSig]
        int HasFocusIO();

        [PreserveSig]
        int TranslateAcceleratorIO(ref MSG msg);
    }

    /// <summary>
    /// IClassFactory —— COM 类工厂，DllGetClassObject 返回此接口
    /// </summary>
    [ComImport]
    [InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
    [Guid("00000001-0000-0000-C000-000000000046")]
    public interface IClassFactory
    {
        [PreserveSig]
        int CreateInstance(IntPtr pUnkOuter, ref Guid riid, out IntPtr ppvObject);

        [PreserveSig]
        int LockServer([MarshalAs(UnmanagedType.Bool)] bool fLock);
    }

    #endregion

    #region 结构体

    /// <summary>
    /// DESKBANDINFO —— 传递给 IDeskBand.GetBandInfo，描述 Band 的尺寸要求
    /// 必须与 Windows SDK 的 DESKBANDINFO 完全一致（参考微软官方文档）。
    /// 来源：https://learn.microsoft.com/zh-cn/windows/win32/api/shobjidl_core/ns-shobjidl_core-deskbandinfo
    /// </summary>
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    public struct DESKBANDINFO
    {
        public uint dwMask;
        public Point ptMinSize;
        public Point ptMaxSize;
        public Point ptIntegral;
        public Point ptActual;
        // WCHAR[256] = 512 字节（官方文档明确为 256）
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
        public string wszTitle;
        public uint dwModeFlags;
        public uint crBkgnd;
    }

    /// <summary>
    /// RECT —— Windows 矩形结构（IDockingWindow.ResizeBorderDW 用）
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct RECT
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    /// <summary>
    /// MSG —— Windows 消息结构（IInputObject 用）
    /// </summary>
    [StructLayout(LayoutKind.Sequential)]
    public struct MSG
    {
        public IntPtr hwnd;
        public uint message;
        public IntPtr wParam;
        public IntPtr lParam;
        public uint time;
        public Point pt;
    }

    #endregion

    #region 常量

    /// <summary>
    /// DESKBANDINFO.dwMask 标志位
    /// </summary>
    public static class DBIM
    {
        public const uint MINSIZE = 0x0001;
        public const uint MAXSIZE = 0x0002;
        public const uint INTEGRAL = 0x0004;
        public const uint ACTUAL = 0x0008;
        public const uint TITLE = 0x0010;
        public const uint MODEFLAGS = 0x0020;
        public const uint BKCOLOR = 0x0040;
    }

    /// <summary>
    /// DESKBANDINFO.dwModeFlags 标志位
    /// </summary>
    public static class DBIMF
    {
        public const uint NORMAL = 0x0000;
        public const uint FIXED = 0x0001;
        public const uint FIXEDBMP = 0x0004;
        public const uint VARIABLEHEIGHT = 0x0008;
        public const uint UNDELETEABLE = 0x0010;
        public const uint DEBOSSED = 0x0020;
        public const uint BKCOLOR = 0x0040;
        public const uint USECHEVRON = 0x0080;
        public const uint BREAK = 0x0100;
        public const uint ADDTOFRONT = 0x0200;
        public const uint TOPALIGN = 0x0400;
        public const uint NOGRIPPER = 0x0800;
        public const uint ALWAYSGRIPPER = 0x1000;
    }

    #endregion
}
