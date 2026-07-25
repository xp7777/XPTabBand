$code = @"
using System;
using System.Drawing;
using System.Runtime.InteropServices;

namespace SizeTest
{
    [StructLayout(LayoutKind.Sequential)]
    public struct DESKBANDINFO
    {
        public uint dwMask;
        public uint dwState;
        public uint dwStateMask;
        public Point ptMinSize;
        public Point ptMaxSize;
        public Point ptIntegral;
        public Point ptActual;
        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 180)]
        public string wszTitle;
        public uint dwModeFlags;
        public uint crBkgnd;
    }

    public class Test
    {
        public static int Run()
        {
            int size = Marshal.SizeOf(typeof(DESKBANDINFO));
            Console.WriteLine("DESKBANDINFO size: " + size + " bytes");
            Console.WriteLine("Expected (Win64): 8 + 8 + 8 + 8 + 8 + 8 + 360 + 4 + 4 = 416 bytes");
            Console.WriteLine("  dwMask offset:      " + Marshal.OffsetOf(typeof(DESKBANDINFO), "dwMask"));
            Console.WriteLine("  dwState offset:     " + Marshal.OffsetOf(typeof(DESKBANDINFO), "dwState"));
            Console.WriteLine("  dwStateMask offset: " + Marshal.OffsetOf(typeof(DESKBANDINFO), "dwStateMask"));
            Console.WriteLine("  ptMinSize offset:   " + Marshal.OffsetOf(typeof(DESKBANDINFO), "ptMinSize"));
            Console.WriteLine("  ptMaxSize offset:   " + Marshal.OffsetOf(typeof(DESKBANDINFO), "ptMaxSize"));
            Console.WriteLine("  ptIntegral offset:  " + Marshal.OffsetOf(typeof(DESKBANDINFO), "ptIntegral"));
            Console.WriteLine("  ptActual offset:    " + Marshal.OffsetOf(typeof(DESKBANDINFO), "ptActual"));
            Console.WriteLine("  wszTitle offset:    " + Marshal.OffsetOf(typeof(DESKBANDINFO), "wszTitle"));
            Console.WriteLine("  dwModeFlags offset: " + Marshal.OffsetOf(typeof(DESKBANDINFO), "dwModeFlags"));
            Console.WriteLine("  crBkgnd offset:     " + Marshal.OffsetOf(typeof(DESKBANDINFO), "crBkgnd"));
            return 0;
        }
    }
}
"@

# 编译并运行
$asmPath = "G:\Test\testFileExplorerPro\XPTab\SizeTest.dll"
$provider = New-Object Microsoft.CSharp.CSharpCodeProvider
$params = New-Object System.CodeDom.Compiler.CompilerParameters
$params.GenerateInMemory = $true
$params.ReferencedAssemblies.Add("System.Drawing.dll")
$result = $provider.CompileAssemblyFromSource($params, $code)
if ($result.Errors.HasErrors) {
    Write-Host "Compile errors:"
    $result.Errors | ForEach-Object { Write-Host "  $_" }
} else {
    [SizeTest.Test]::Run()
}
