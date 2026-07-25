using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace XPTab
{
    /// <summary>
    /// XPTab Explorer Band 主类。
    /// 实现 IDeskBand/IObjectWithSite/IPersistStream/IInputObject，
    /// 被 explorer.exe 作为 COM 对象创建并嵌入资源管理器工具栏。
    /// 注意：只实现 IDeskBand（不实现 IDeskBand2），避免 vtable 错位问题。
    /// Win10 Explorer 完全支持 IDeskBand。
    /// </summary>
    [
        ComVisible(true),
        Guid("A1B2C3D4-E5F6-4789-ABCD-0123456789AB"), // 本 Band 的 CLSID（注册时用）
        ClassInterface(ClassInterfaceType.None),
        ComDefaultInterface(typeof(IDeskBand))
    ]
    public class XPTabBand : IDeskBand, IObjectWithSite, IPersistStream, IInputObject
    {
        /// <summary>Band 在注册表中的显示名称</summary>
        public const string BandTitle = "XPTab";

        private TabBarControl _tabBar;
        private object _site;          // Explorer 提供的站点对象（IServiceProvider）
        private IntPtr _hwndParent;    // 父窗口句柄（Explorer 的 Band 宿主窗口）

        // 注册表中的 Band CLSID 字符串（供注册脚本使用）
        public static readonly string CLSID = typeof(XPTabBand).GUID.ToString("B");

        // 日志文件路径
        private static readonly string LogPath = Path.Combine(
            Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
            "XPTab", "band_log.txt");

        static XPTabBand()
        {
            Log("=== XPTabBand static constructor (type loaded by CLR) ===");
            Log($"Process: {System.Diagnostics.Process.GetCurrentProcess().ProcessName}");
            Log($"Runtime: {Environment.Version}");
        }

        private static void Log(string msg)
        {
            try
            {
                var dir = Path.GetDirectoryName(LogPath);
                if (!Directory.Exists(dir)) Directory.CreateDirectory(dir);
                File.AppendAllText(LogPath, $"[{DateTime.Now:HH:mm:ss.fff}] {msg}\r\n");
            }
            catch { }
        }

        public XPTabBand()
        {
            Log("=== XPTabBand constructor called ===");
            _tabBar = new TabBarControl();
            _tabBar.NewTabRequested += (_, _) =>
            {
                // 最小可用版：新建标签页时复用 Explorer 当前窗口
                // 后续迭代会通过 IShellBrowser 实现多标签导航
                _tabBar.AddTab($"标签 {_tabBar.Controls.Count}");
            };
            Log("XPTabBand constructor completed");
        }

        #region IOleWindow / IDockingWindow

        public int GetWindow(out IntPtr phwnd)
        {
            Log($"GetWindow called: _tabBar={(_tabBar == null ? "null" : "set")}, handleCreated={(_tabBar?.IsHandleCreated ?? false)}");
            phwnd = _tabBar.IsHandleCreated ? _tabBar.Handle : IntPtr.Zero;
            Log($"GetWindow returning hwnd={phwnd}");
            return 0; // S_OK
        }

        public int ContextSensitiveHelp(bool fEnterMode)
        {
            Log($"ContextSensitiveHelp called: fEnterMode={fEnterMode}");
            return 0;
        }

        public int ShowDW(bool fShow)
        {
            Log($"ShowDW called: fShow={fShow}, _tabBar={(_tabBar == null ? "null" : "set")}");
            if (_tabBar != null && _tabBar.IsHandleCreated)
            {
                if (fShow) _tabBar.Show(); else _tabBar.Hide();
            }
            return 0;
        }

        public int CloseDW(uint dwReserved)
        {
            Log($"CloseDW called");
            try
            {
                _tabBar?.Dispose();
            }
            catch (Exception ex) { Log($"CloseDW exception: {ex.Message}"); }
            _tabBar = null;
            return 0;
        }

        public int ResizeBorderDW(RECT prcBorder, IntPtr punkToolbarSite, bool fReserved)
        {
            Log($"ResizeBorderDW called: rect=({prcBorder.Left},{prcBorder.Top},{prcBorder.Right},{prcBorder.Bottom}), site={punkToolbarSite}, fReserved={fReserved}");
            return 0;
        }

        #endregion

        #region IDeskBand

        public int GetBandInfo(uint dwBandID, uint dwViewMode, IntPtr pdbi)
        {
            int structSize = System.Runtime.InteropServices.Marshal.SizeOf(typeof(DESKBANDINFO));
            Log($"GetBandInfo called: bandID={dwBandID}, viewMode={dwViewMode}, pdbi={pdbi}, structSize={structSize}");

            // 从原始指针读取结构体
            DESKBANDINFO dbi = System.Runtime.InteropServices.Marshal.PtrToStructure<DESKBANDINFO>(pdbi);
            Log($"GetBandInfo: mask=0x{dbi.dwMask:X}");

            const int IdealHeight = 28;

            if ((dbi.dwMask & DBIM.MINSIZE) != 0)
            {
                dbi.ptMinSize = new System.Drawing.Point(8, IdealHeight);
            }
            if ((dbi.dwMask & DBIM.MAXSIZE) != 0)
            {
                dbi.ptMaxSize = new System.Drawing.Point(-1, IdealHeight);
            }
            if ((dbi.dwMask & DBIM.INTEGRAL) != 0)
            {
                dbi.ptIntegral = new System.Drawing.Point(1, 1);
            }
            if ((dbi.dwMask & DBIM.ACTUAL) != 0)
            {
                dbi.ptActual = new System.Drawing.Point(0, IdealHeight);
            }
            if ((dbi.dwMask & DBIM.TITLE) != 0)
            {
                dbi.wszTitle = BandTitle;
            }
            if ((dbi.dwMask & DBIM.MODEFLAGS) != 0)
            {
                dbi.dwModeFlags = DBIMF.VARIABLEHEIGHT | DBIMF.NOGRIPPER;
            }
            if ((dbi.dwMask & DBIM.BKCOLOR) != 0)
            {
                // 不设置背景色，由控件自绘
            }

            // 写回结构体
            System.Runtime.InteropServices.Marshal.StructureToPtr(dbi, pdbi, false);
            Log("GetBandInfo completed, structure written back");
            return 0;
        }

        #endregion

        #region IObjectWithSite

        public int SetSite(object pUnkSite)
        {
            Log($"SetSite called: site={(pUnkSite == null ? "null" : pUnkSite.GetType().Name)}");
            _site = pUnkSite;

            // Explorer 提供站点对象后，将我们的 TabBar 作为子窗口创建
            if (pUnkSite != null)
            {
                // 获取站点的 IOleWindow 以得到父窗口句柄
                try
                {
                    if (pUnkSite is IOleWindow oleWindow)
                    {
                        int hr = oleWindow.GetWindow(out _hwndParent);
                        Log($"SetSite: GetWindow hr=0x{hr:X}, hwndParent={_hwndParent}");
                    }
                    else
                    {
                        Log("SetSite: site is NOT IOleWindow");
                    }
                }
                catch (Exception ex)
                {
                    Log($"SetSite: GetWindow exception: {ex.Message}");
                }

                // 创建 WinForms 控件并设置父窗口
                if (_tabBar != null && !_tabBar.IsHandleCreated && _hwndParent != IntPtr.Zero)
                {
                    Log("SetSite: creating handle and SetParent");
                    // 触发句柄创建
                    var h = _tabBar.Handle;
                    SetParent(h, _hwndParent);
                    Log($"SetSite: handle created={h}, SetParent done");
                }
                else
                {
                    Log($"SetSite: skip SetParent - _tabBar={(_tabBar == null ? "null" : "set")}, handleCreated={(_tabBar?.IsHandleCreated ?? false)}, hwndParent={_hwndParent}");
                }
            }
            return 0;
        }

        public int GetSite(ref Guid riid, out IntPtr ppvSite)
        {
            if (_site == null)
            {
                ppvSite = IntPtr.Zero;
                return unchecked((int)0x80004002); // E_NOINTERFACE
            }
            IntPtr pUnk = Marshal.GetIUnknownForObject(_site);
            try
            {
                return Marshal.QueryInterface(pUnk, ref riid, out ppvSite);
            }
            finally
            {
                Marshal.Release(pUnk);
            }
        }

        #endregion

        #region IPersistStream

        public void GetClassID(out Guid pClassID)
        {
            Log("GetClassID called");
            pClassID = GetType().GUID;
        }

        public int IsDirty() => 0; // S_OK 表示未修改

        public void Load(IntPtr pStm) { /* 最小版不持久化配置 */ }

        public void Save(IntPtr pStm, bool fClearDirty) { }

        public void GetSizeMax(out ulong pcbSize)
        {
            pcbSize = 0;
        }

        #endregion

        #region IInputObject

        public int UIActivateIO(bool fActivate, ref MSG msg)
        {
            if (fActivate && _tabBar != null && _tabBar.IsHandleCreated)
            {
                _tabBar.Focus();
            }
            return 0;
        }

        public int HasFocusIO()
        {
            return (_tabBar != null && _tabBar.ContainsFocus) ? 0 : 1;
        }

        public int TranslateAcceleratorIO(ref MSG msg) => 1; // S_FALSE

        #endregion

        #region Win32

        [DllImport("user32.dll", SetLastError = true)]
        private static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);

        #endregion
    }
}
