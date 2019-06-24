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
using System.Runtime.InteropServices;
using GEditor.View.UserControls;

namespace GEditor.View
{
    /// <summary>
    /// Interaction logic for Properties_SceneObject.xaml
    /// </summary>
    public partial class Properties_SceneObject : UserControl, IPathLoaderParent
    {
        MainWindow mainWindow;

        private string sObjectName;

        PropertiesModel_SceneObject model;

        public Properties_SceneObject()
        {
            InitializeComponent();
            model = new PropertiesModel_SceneObject();
            model.SetParent(this);
        }

        public void SetMainWindow(MainWindow mwRef)
        {
            mainWindow = mwRef;
        }

        public void SetObjectName(string objName)
        {
            sObjectName = objName;
        }

        public void GetSceneObjectProperties()
        {
            float[] trans = new float[9];
            IGCore.GetSceneObjectTransform(sObjectName, trans);
            model.SetTransform(trans);
            model.InitName(sObjectName);
            model.InitMeshName(Marshal.PtrToStringUni(IGCore.GetSceneObjectMeshName(sObjectName)));
            //model.InitMaterialName(Marshal.PtrToStringUni(IGCore.GetSceneObjectMaterialName(sObjectName)));
            CommonControl.DataContext = model;
            TransformControl.DataContext = model;
            RenderControl.DataContext = model;
            GetMeshOverrideMaterials();
        }

        public void RefreshTransform()
        {
            float[] trans = new float[9];
            IGCore.GetSceneObjectTransform(sObjectName, trans);
            model.SetTransform(trans);
        }

        public bool NameAvailable(string name)
        {
            return (!IGCore.SceneObjectExists(name));
        }

        public void SetSceneObjectName()
        {
            IGCore.RenameSceneObject(sObjectName, model.Name);
            sObjectName = model.Name;
            mainWindow.RefreshOutliner();
        }

        public void SetSceneObjectTransform()
        {
            IGCore.SetSceneObjectTransform(sObjectName, model.GetTransform());
        }

        public void SetSceneObjectMesh()
        {
            IGCore.SetSceneObjectMesh(sObjectName, model.MeshUniqueName);
        }

        /*
        public void SetSceneObjectMaterial()
        {
            IGCore.SetSceneObjectMaterial(sObjectName, model.MaterialUniqueName);
        }
        */

        private void SetMeshBySelectedFile()
        {
            string selectedPath = mainWindow.GetBrowserSelectedFilePath();
            string selectedUniqueName = mainWindow.GetBrowserSelectedFileUniqueName();
            if (selectedPath == string.Empty)
                return;
            if (!System.IO.File.Exists(selectedPath))
                return;
            if (System.IO.Path.GetExtension(selectedPath).ToLower() == ".fbx")
            {
                model.MeshUniqueName = selectedUniqueName;
                GetMeshOverrideMaterials();
            }
        }

        /*
        private void SetMaterialBySelectedFile()
        {
            string selectedPath = mainWindow.GetBrowserSelectedFilePath();
            string selectedUniqueName = mainWindow.GetBrowserSelectedFileUniqueName();
            if (selectedPath == string.Empty)
                return;
            if (!System.IO.File.Exists(selectedPath))
                return;
            if (System.IO.Path.GetExtension(selectedPath).ToLower() == ".gmat")
            {
                model.MaterialUniqueName = selectedUniqueName;
            }
        }
        */

        private void SetMeshToSphere()
        {
            model.MeshUniqueName = "Sphere";
            GetMeshOverrideMaterials();
        }

        /*
        private void SetMaterialToDefault()
        {
            model.MaterialUniqueName = "Default";
        }
        */

        private void LoadMesh(object sender, RoutedEventArgs e)
        {
            SetMeshBySelectedFile();
        }

        private void ClearMesh(object sender, RoutedEventArgs e)
        {
            SetMeshToSphere();
        }

        /*
        private void LoadMaterial(object sender, RoutedEventArgs e)
        {
            SetMaterialBySelectedFile();
        }

        private void ClearMaterial(object sender, RoutedEventArgs e)
        {
            SetMaterialToDefault();
        }
        */

        public void TextBox_DigitalOnly(object sender, TextCompositionEventArgs e)
        {
            Regex re = new Regex("[^0-9.-]+");
            e.Handled = re.IsMatch(e.Text);
        }

        public void GetMeshOverrideMaterials()
        {
            //model = new PropertiesModel_Mesh();
            //model.SetParent(this);
            MaterialPanel.Children.Clear();
            int submeshCount = IGCore.GetMeshSubmeshCount(model.MeshUniqueName);
            IntPtr submeshNamesPtr = IGCore.GetMeshSubmeshNames(model.MeshUniqueName);// TODO : Manually free memory.
            var submeshNameList = GetAllStrings(submeshNamesPtr, submeshCount);
            for (var i = 0; i < submeshCount; i++)
            {
                var newPathLoader = new PathLoader();
                newPathLoader.Init();
                newPathLoader.Id = submeshNameList[i];
                newPathLoader.model.LabelText = submeshNameList[i];
                newPathLoader.model.MaterialUniqueName = Marshal.PtrToStringUni(IGCore.GetSceneObjectOverrideMaterial(sObjectName, submeshNameList[i]));
                newPathLoader.parent = this;
                MaterialPanel.Children.Add(newPathLoader);
                //pathLoaders.Add(newPathLoader);
            }
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
                IGCore.SetSceneObjectOverrideMaterial(sObjectName, id, selectedUniqueName);
                ((PathLoader)selfRef).model.MaterialUniqueName = selectedUniqueName;
            }
        }

        public void Reset(UserControl selfRef, string type, string id)
        {
            IGCore.SetSceneObjectOverrideMaterial(sObjectName, id, "None");
            ((PathLoader)selfRef).model.MaterialUniqueName = "None";
        }

    }
}
