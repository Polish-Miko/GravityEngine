using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace GEditor
{
    class IGRenderer
    {
        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void Init();

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void InitD3D(IntPtr hwnd, double width, double height);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern int Run();
    }
}
