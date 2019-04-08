using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;

namespace GEditor
{
    class IGCore
    {
        public delegate void VoidFuncPointerType();

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void MsgProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern int GetSceneObjectNum();

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern IntPtr GetSceneObjectName(int index);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void GetSceneObjectTransform(string objName, [In, Out] float[] trans);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetSceneObjectTransform(string objName, [In, Out] float[] trans);

    }
}
