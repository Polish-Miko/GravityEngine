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
            TransformControl.DataContext = model;
        }

        public void SetSceneObjectProperties()
        {
            IGCore.SetSceneObjectTransform(sObjectName, model.GetTransform());
            //MessageBox.Show(model.ScaleX + "," + model.ScaleY + "," + model.ScaleZ);
        }

        public void TextBox_DigitalOnly(object sender, TextCompositionEventArgs e)
        {
            Regex re = new Regex("[^0-9.-]+");
            e.Handled = re.IsMatch(e.Text);
        }

    }
}
