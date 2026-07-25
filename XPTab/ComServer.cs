using System;
using System.Runtime.InteropServices;

namespace XPTab
{
    /// <summary>
    /// COM 类工厂：为 XPTabBand 创建实例。
    /// explorer.exe 通过 DllGetClassObject → IClassFactory.CreateInstance 创建 Band 对象。
    /// </summary>
    // ComVisible(false)：类工厂不直接暴露给 COM 注册表。
    // .NET 的 regasm 会自动为 ComDefaultInterface 的类生成类工厂，
    // 这里手动实现的 IClassFactory 不会被 Explorer 直接实例化。
    [ComVisible(false)]
    public class XPTabClassFactory : IClassFactory
    {
        public int CreateInstance(IntPtr pUnkOuter, ref Guid riid, out IntPtr ppvObject)
        {
            ppvObject = IntPtr.Zero;
            if (pUnkOuter != IntPtr.Zero)
            {
                // 不支持聚合
                return unchecked((int)0x80040110); // CLASS_E_NOAGGREGATION
            }

            try
            {
                var band = new XPTabBand();
                IntPtr pUnk = Marshal.GetIUnknownForObject(band);
                try
                {
                    int hr = Marshal.QueryInterface(pUnk, ref riid, out ppvObject);
                    return hr;
                }
                finally
                {
                    Marshal.Release(pUnk); // QueryInterface 已 AddRef，这里释放 GetIUnknownForObject 的引用
                }
            }
            catch
            {
                return unchecked((int)0x80004005); // E_FAIL
            }
        }

        public int LockServer(bool fLock) => 0;
    }

    /// <summary>
    /// COM 服务器导出函数。
    /// 这些函数通过 .tlb 注册或 regasm 注册时暴露给 explorer.exe。
    /// 注意：.NET Framework 程序集通过 regasm /codebase 注册后，
    /// mscoree.dll 作为标准 COM 代理，自动提供 DllGetClassObject 等导出。
    /// 因此我们不需要手写 DllMain，只需正确实现类工厂和 COM 注册表项。
    /// </summary>
    public static class ComServer
    {
        /// <summary>XPTabBand 的 CLSID 字符串</summary>
        public static readonly string BandClsid = typeof(XPTabBand).GUID.ToString("B");

        /// <summary>程序集完整路径（注册脚本使用）</summary>
        public static string AssemblyPath => typeof(XPTabBand).Assembly.Location;

        /// <summary>程序集全名</summary>
        public static string AssemblyFullName => typeof(XPTabBand).Assembly.FullName;
    }
}
