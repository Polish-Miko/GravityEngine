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

namespace GEditor.View
{
    /// <summary>
    /// Interaction logic for Properties_SceneObject.xaml
    /// </summary>
    public partial class Properties_Material : UserControl
    {
        MainWindow mainWindow;

        private string sMaterialUniqueName;

        PropertiesModel_Material model;

        public Properties_Material()
        {
            InitializeComponent();
            model = new PropertiesModel_Material();
            model.SetParent(this);
        }

        public void SetMainWindow(MainWindow mwRef)
        {
            mainWindow = mwRef;
        }

        public void SetMaterialName(string matUniqueName)
        {
            sMaterialUniqueName = matUniqueName;
        }

        public void GetMaterialProperties()
        {
            model.Name = System.IO.Path.GetFileNameWithoutExtension(sMaterialUniqueName);
            float[] scale = new float[2];
            IGCore.GetMaterialScale(sMaterialUniqueName, scale);
            model.MatScaleX = scale[0].ToString();
            model.MatScaleY = scale[1].ToString();
            model.AlbedoTextureName = IGCore.GetMaterialTextureName(sMaterialUniqueName, 0);
            model.NormalTextureName = IGCore.GetMaterialTextureName(sMaterialUniqueName, 1);
            model.OrmTextureName = IGCore.GetMaterialTextureName(sMaterialUniqueName, 2);

            MaterialCommonControl.DataContext = model;
            MaterialTextureControl.DataContext = model;
        }

        public void SetName(string matName)
        {
            // check and rename the material file.
            string oldPath = mainWindow.GetWorkDirectory() + sMaterialUniqueName;
            string newUniqueName = System.IO.Path.GetDirectoryName(sMaterialUniqueName) + @"\" + matName + ".gmat";
            string newPath = mainWindow.GetWorkDirectory() + newUniqueName;
            if (!System.IO.File.Exists(newPath))
            {
                //  rename the .gmat file and the material.
                System.IO.FileInfo fi = new System.IO.FileInfo(oldPath);
                fi.MoveTo(newPath);
                IGCore.RenameMaterial(sMaterialUniqueName, newUniqueName);
                sMaterialUniqueName = newUniqueName;
            }
        }

        /*
        public void SetTextureName(string albedoName, string normalName, string ormName)
        {
            
        }
        */

        public void SetScale(float scaleX, float scaleY)
        {
            float[] scale = new float[2];
            scale[0] = scaleX;
            scale[1] = scaleY;
            IGCore.SetMaterialScale(sMaterialUniqueName, scale);
        }

        public void TextBox_DigitalOnly(object sender, TextCompositionEventArgs e)
        {
            Regex re = new Regex("[^0-9.-]+");
            e.Handled = re.IsMatch(e.Text);
        }

        private void SetTextureBySelectedFile(int index)
        {
            string selectedPath = mainWindow.GetBrowserSelectedFilePath();
            string selectedUniqueName = mainWindow.GetBrowserSelectedFileUniqueName();
            if (selectedPath == string.Empty)
                return;
            if (!System.IO.File.Exists(selectedPath))
                return;
            if (System.IO.Path.GetExtension(selectedPath) == "dds" || System.IO.Path.GetExtension(selectedPath) == "png")
            {
                if (IGCore.SetMaterialTexture(sMaterialUniqueName, index, selectedUniqueName))
                {
                    if (index == 0)
                        model.AlbedoTextureName = selectedUniqueName;
                    if (index == 1)
                        model.NormalTextureName = selectedUniqueName;
                    if (index == 2)
                        model.OrmTextureName = selectedUniqueName;
                }
            }
        }

        private void ClearTextureByIndex(int index)
        {
            string defaultTexture = IGCore.SetMaterialTextureToDefaultValue(index);
        }

        private void LoadAlbedo(object sender, RoutedEventArgs e)
        {
            SetTextureBySelectedFile(0);
        }

        private void ClearAlbedo(object sender, RoutedEventArgs e)
        {
            ClearTextureByIndex(0);
        }

        private void LoadNormal(object sender, RoutedEventArgs e)
        {
            SetTextureBySelectedFile(1);
        }

        private void ClearNormal(object sender, RoutedEventArgs e)
        {
            ClearTextureByIndex(1);
        }

        private void LoadOrm(object sender, RoutedEventArgs e)
        {
            SetTextureBySelectedFile(2);
        }

        private void ClearOrm(object sender, RoutedEventArgs e)
        {
            ClearTextureByIndex(2);
        }

    }
}
