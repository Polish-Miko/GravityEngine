using GEditor.Model.Properties;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using GEditor.View.UserControls;
using System.Runtime.InteropServices;

namespace GEditor.View
{
    /// <summary>
    /// Interaction logic for Properties_SceneObject.xaml
    /// </summary>
    public partial class Properties_Mesh : UserControl, IPathLoaderParent
    {
        MainWindow mainWindow;

        private string sMeshName;

        //PropertiesModel_Mesh model;

        private List<PathLoader> pathLoaders = new List<PathLoader>();

        public Properties_Mesh()
        {
            InitializeComponent();
        }

        public void GetMeshProperties()
        {
            //model = new PropertiesModel_Mesh();
            //model.SetParent(this);
            int submeshCount = IGCore.GetMeshSubmeshCount(sMeshName);
            IntPtr submeshNamesPtr = IGCore.GetMeshSubmeshNames(sMeshName);// TODO : Manually free memory.
            var submeshNameList = GetAllStrings(submeshNamesPtr, submeshCount);
            for (var i = 0; i < submeshCount; i++)
            {
                var newPathLoader = new PathLoader();
                newPathLoader.Init();
                newPathLoader.Id = submeshNameList[i];
                newPathLoader.model.LabelText = submeshNameList[i];
                newPathLoader.model.MaterialUniqueName = Marshal.PtrToStringUni(IGCore.GetMeshSubmeshMaterialUniqueName(sMeshName, submeshNameList[i]));
                newPathLoader.parent = this;
                MaterialPanel.Children.Add(newPathLoader);
                pathLoaders.Add(newPathLoader);
            }
        }

        public void SetMainWindow(MainWindow mwRef)
        {
            mainWindow = mwRef;
        }

        public void SetMeshName(string txtName)
        {
            sMeshName = txtName;
        }

        public void Set(UserControl selfRef, string type, string id)
        {
            string selectedPath = mainWindow.GetBrowserSelectedFilePath();
            string selectedUniqueName = mainWindow.GetBrowserSelectedFileUniqueName();
            if (selectedPath == string.Empty)
                return;
            if (!System.IO.File.Exists(selectedPath))
                return;
            if (System.IO.Path.GetExtension(selectedPath).ToLower() == ".gmat")
            {
                //model.SetMaterialUniqueName(id, selectedUniqueName);
                IGCore.SetMeshSubmeshMaterialUniqueName(sMeshName, id, selectedUniqueName);
                ((PathLoader)selfRef).model.MaterialUniqueName = selectedUniqueName;
            }
        }

        public void Reset(UserControl selfRef, string type, string id)
        {
            //model.SetMaterialUniqueName(id, "Default");
            IGCore.SetMeshSubmeshMaterialUniqueName(sMeshName, id, "Default");
            ((PathLoader)selfRef).model.MaterialUniqueName = "Default";
        }

        public static List<string> GetAllStrings(IntPtr ptr, int size)
        {
            var list = new List<string>();
            var tempPtr = ptr;
            for (int i = 0; i < size; i++)
            {
                var strPtr = (IntPtr)Marshal.PtrToStructure(tempPtr, typeof(IntPtr));
                list.Add(Marshal.PtrToStringUni(strPtr));
                tempPtr = new IntPtr(tempPtr.ToInt64() + IntPtr.Size);
            }

            // free memory
            /*
            tempPtr = ptr;
            for (int i = 0; i < size; i++)
            {
                var strPtr = (IntPtr)Marshal.PtrToStructure(tempPtr, typeof(IntPtr));
                Marshal.FreeHGlobal(strPtr);
                tempPtr = new IntPtr(tempPtr.ToInt64() + IntPtr.Size);
            }
            Marshal.FreeHGlobal(ptr);
            */

            return list;
        }

    }
}
