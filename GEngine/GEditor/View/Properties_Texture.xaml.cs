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
    public partial class Properties_Texture : UserControl
    {
        MainWindow mainWindow;

        private string sTextureName;

        PropertiesModel_Texture model;

        public Properties_Texture()
        {
            InitializeComponent();
            model = new PropertiesModel_Texture();
            model.SetParent(this);
        }

        public void SetMainWindow(MainWindow mwRef)
        {
            mainWindow = mwRef;
        }

        public void SetTextureName(string txtName)
        {
            sTextureName = txtName;
        }

        public void GetTextureProperties()
        {
            bool srgb = IGCore.GetTextureSrgb(sTextureName);
            model.bSrgb = srgb;
            TextureControl.DataContext = model;
        }

        public void SetTextureProperties()
        {
            IGCore.SetTextureSrgb(sTextureName, model.bSrgb);
            //MessageBox.Show(model.ScaleX + "," + model.ScaleY + "," + model.ScaleZ);
        }

    }
}
