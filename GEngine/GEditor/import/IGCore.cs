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
        public static extern void Init();

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void InitD3D(IntPtr hwnd, double width, double height);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern int Run();

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetWorkDirectory([MarshalAs(UnmanagedType.LPWStr)] string dir);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void MsgProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern int GetSceneObjectNum();

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern IntPtr GetSceneObjectName(int index);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetSetSceneObjectsCallback(VoidFuncPointerType pSetSceneObjectsCallback);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void GetSceneObjectTransform([MarshalAs(UnmanagedType.LPWStr)] string objName, [In, Out] float[] trans);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetSceneObjectTransform([MarshalAs(UnmanagedType.LPWStr)] string objName, [In, Out] float[] trans);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern bool GetTextureSrgb([MarshalAs(UnmanagedType.LPWStr)] string txtName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetTextureSrgb([MarshalAs(UnmanagedType.LPWStr)] string txtName, bool bSrgb);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetProjectName([MarshalAs(UnmanagedType.LPWStr)] string projName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SaveProject();

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void CreateMaterial([MarshalAs(UnmanagedType.LPWStr)] string UniqueName);

    }
}
