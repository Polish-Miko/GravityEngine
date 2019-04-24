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
    public partial class Properties_ProjectSettings : UserControl
    {
        MainWindow mainWindow;

        PropertiesModel_ProjectSettings model;

        public Properties_ProjectSettings()
        {
            InitializeComponent();
            model = new PropertiesModel_ProjectSettings();
            model.SetParent(this);
        }

        public void GetSettings()
        {
            string cubemapName = Marshal.PtrToStringUni(IGCore.GetSkyCubemapUniqueName());
            model.InitSkyCubemapName(cubemapName);
            ProjectSettingsControl.DataContext = model;
        }

        public void SetMainWindow(MainWindow mwRef)
        {
            mainWindow = mwRef;
        }

        public void SetSkyCubemapName()
        {
            IGCore.SetSkyCubemapUniqueName(model.SkyCubemapName);
        }

        private void LoadSkyCubemap(object sender, RoutedEventArgs e)
        {
            string selectedPath = mainWindow.GetBrowserSelectedFilePath();
            string selectedUniqueName = mainWindow.GetBrowserSelectedFileUniqueName();
            if (selectedPath == string.Empty)
                return;
            if (!System.IO.File.Exists(selectedPath))
                return;
            if (System.IO.Path.GetExtension(selectedPath).ToLower() == ".dds")
            {
                model.SkyCubemapName = selectedUniqueName;
            }
        }

        private void ClearSkyCubemap(object sender, RoutedEventArgs e)
        {
            string defaultSkyCubemapUniqueName = Marshal.PtrToStringUni(IGCore.GetDefaultSkyCubemapUniqueName());
            model.SkyCubemapName = defaultSkyCubemapUniqueName;
        }

        public bool SkyCubemapNameAvailable(string newName)
        {
            return IGCore.SkyCubemapNameAvailable(newName);
        }

    }
}
