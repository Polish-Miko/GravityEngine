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
        public delegate void VoidWstringFuncPointerType([MarshalAs(UnmanagedType.LPWStr)] string param1);

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

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void GetMaterialScale([MarshalAs(UnmanagedType.LPWStr)] string matUniqueName, [In, Out] float[] scale);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetMaterialScale([MarshalAs(UnmanagedType.LPWStr)] string matUniqueName, [In, Out] float[] scale);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern IntPtr GetMaterialTextureUniqueName([MarshalAs(UnmanagedType.LPWStr)] string matUniqueName, int index);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern bool SetMaterialTexture([MarshalAs(UnmanagedType.LPWStr)] string matUniqueName, int index, [MarshalAs(UnmanagedType.LPWStr)] string texUniqueName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetMaterialTextureToDefaultValue([MarshalAs(UnmanagedType.LPWStr)] string matUniqueName, int index);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void RenameMaterial([MarshalAs(UnmanagedType.LPWStr)] string oldUniqueName, [MarshalAs(UnmanagedType.LPWStr)] string newUniqueName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetSceneObjectMesh([MarshalAs(UnmanagedType.LPWStr)] string sceneObjectName, [MarshalAs(UnmanagedType.LPWStr)] string meshUniqueName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetSceneObjectMaterial([MarshalAs(UnmanagedType.LPWStr)] string sceneObjectName, [MarshalAs(UnmanagedType.LPWStr)] string matUniqueName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern IntPtr GetSceneObjectMeshName([MarshalAs(UnmanagedType.LPWStr)] string sceneObjectName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern IntPtr GetSceneObjectMaterialName([MarshalAs(UnmanagedType.LPWStr)] string sceneObjectName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern bool SceneObjectExists([MarshalAs(UnmanagedType.LPWStr)] string sceneObjectName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void CreateSceneObject([MarshalAs(UnmanagedType.LPWStr)] string sceneObjectName, [MarshalAs(UnmanagedType.LPWStr)] string meshUniqueName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void RenameSceneObject([MarshalAs(UnmanagedType.LPWStr)] string oldName, [MarshalAs(UnmanagedType.LPWStr)] string newName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void DeleteSceneObject([MarshalAs(UnmanagedType.LPWStr)] string sceneObjectName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern IntPtr GetSkyCubemapUniqueName();

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern IntPtr GetDefaultSkyCubemapUniqueName();

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetSkyCubemapUniqueName([MarshalAs(UnmanagedType.LPWStr)] string newName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern bool SkyCubemapNameAvailable([MarshalAs(UnmanagedType.LPWStr)] string cubemapName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetSelectSceneObjectCallback(VoidWstringFuncPointerType pSetSceneObjectsCallback);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern bool SelectSceneObject([MarshalAs(UnmanagedType.LPWStr)] string sceneObjectName);

        [DllImport(@"Build\GEngineDll.dll")]
        public static extern void SetRefreshSceneObjectTransformCallback(VoidFuncPointerType pRefreshSceneObjectTransformCallback);

    }
}
