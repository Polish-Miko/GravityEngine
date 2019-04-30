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

namespace GEditor.View
{
    /// <summary>
    /// Interaction logic for Properties_SceneObject.xaml
    /// </summary>
    public partial class Properties_SceneObject : UserControl
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
            model.InitMaterialName(Marshal.PtrToStringUni(IGCore.GetSceneObjectMaterialName(sObjectName)));
            CommonControl.DataContext = model;
            TransformControl.DataContext = model;
            RenderControl.DataContext = model;
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

        public void SetSceneObjectMaterial()
        {
            IGCore.SetSceneObjectMaterial(sObjectName, model.MaterialUniqueName);
        }

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
            }
        }

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

        private void SetMeshToSphere()
        {
            model.MeshUniqueName = "Sphere";
        }

        private void SetMaterialToDefault()
        {
            model.MaterialUniqueName = "Default";
        }

        private void LoadMesh(object sender, RoutedEventArgs e)
        {
            SetMeshBySelectedFile();
        }

        private void ClearMesh(object sender, RoutedEventArgs e)
        {
            SetMeshToSphere();
        }

        private void LoadMaterial(object sender, RoutedEventArgs e)
        {
            SetMaterialBySelectedFile();
        }

        private void ClearMaterial(object sender, RoutedEventArgs e)
        {
            SetMaterialToDefault();
        }

        public void TextBox_DigitalOnly(object sender, TextCompositionEventArgs e)
        {
            Regex re = new Regex("[^0-9.-]+");
            e.Handled = re.IsMatch(e.Text);
        }

    }
}
