using System;

namespace XPTab.Test
{
    /// <summary>
    /// COM 创建测试程序：验证 explorer.exe 能否成功创建 XPTabBand 实例
    /// 编译: csc TestHost.cs /reference:..\build\XPTab.dll
    /// 运行: TestHost.exe
    /// </summary>
    public static class TestHost
    {
        [STAThread]
        public static void Main()
        {
            Console.WriteLine("=== XPTab COM 创建测试 ===");
            try
            {
                var type = Type.GetTypeFromCLSID(new Guid("A1B2C3D4-E5F6-4789-ABCD-0123456789AB"));
                Console.WriteLine("CLSID 类型获取成功: " + (type == null ? "null" : type.FullName));
                Console.WriteLine("程序集: " + (type == null ? "null" : type.Assembly.GetName().Name));

                var obj = Activator.CreateInstance(type);
                Console.WriteLine("实例创建成功: " + (obj == null ? "null" : obj.GetType().FullName));

                // 测试 COM 接口查询
                IDeskBand2 deskBand = obj as IDeskBand2;
                if (deskBand != null)
                {
                    Console.WriteLine("[OK] IDeskBand2 接口查询成功");
                    var dbi = new DESKBANDINFO();
                    dbi.dwMask = DBIM.MINSIZE | DBIM.ACTUAL | DBIM.TITLE | DBIM.MODEFLAGS;
                    int hr = deskBand.GetBandInfo(1, 0, ref dbi);
                    Console.WriteLine(string.Format("[OK] GetBandInfo 返回 hr=0x{0:X8}", hr));
                    Console.WriteLine("  最小尺寸: " + dbi.ptMinSize.ToString());
                    Console.WriteLine("  实际尺寸: " + dbi.ptActual.ToString());
                    Console.WriteLine("  标题: " + (dbi.wszTitle ?? "(null)"));
                }
                else
                {
                    Console.WriteLine("[FAIL] IDeskBand2 接口查询失败");
                }

                IObjectWithSite ows = obj as IObjectWithSite;
                if (ows != null)
                {
                    Console.WriteLine("[OK] IObjectWithSite 接口查询成功");
                }

                IPersistStream ps = obj as IPersistStream;
                if (ps != null)
                {
                    Console.WriteLine("[OK] IPersistStream 接口查询成功");
                    Guid clsid2;
                    ps.GetClassID(out clsid2);
                    Console.WriteLine("  ClassID: " + clsid2.ToString());
                }

                Console.WriteLine();
                Console.WriteLine("[OK] COM 对象测试全部通过");
                Console.WriteLine("如果 Explorer 仍不显示 Band，问题在 Band UI 创建/父窗口挂接阶段。");
            }
            catch (Exception ex)
            {
                Console.WriteLine();
                Console.WriteLine("[FAIL] 创建失败: " + ex.GetType().Name);
                Console.WriteLine("  消息: " + ex.Message);
                Console.WriteLine("  堆栈: " + ex.StackTrace);
                if (ex.InnerException != null)
                {
                    Console.WriteLine("  内部异常: " + ex.InnerException.Message);
                }
            }
            Console.WriteLine();
            Console.WriteLine("按任意键退出...");
            Console.ReadKey();
        }
    }
}
