using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
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
using GEditor.Model.Properties;

namespace GEditor.View.UserControls
{
    public interface IPathLoaderParent
    {

        void Set(UserControl selfRef, string type, string id);

        void Reset(UserControl selfRef, string type, string id);

    }

    /// <summary>
    /// Interaction logic for PathLoader.xaml
    /// </summary>
    public partial class PathLoader : UserControl
    {

        public IPathLoaderParent parent;

        public string Id;

        //public string LabelText = " Default :";

        //public string MaterialUniqueName = "None";

        public PropertiesModel_PathLoader model;

        public PathLoader()
        {
            InitializeComponent();
        }

        public void Init()
        {
            model = new PropertiesModel_PathLoader();
            model.SetParent(this);
            MainStackPanel.DataContext = model;
            //SubmeshLabel.DataContext = this;
            //MaterialTextBox.DataContext = this;
        }

        void Set(object sender, RoutedEventArgs e)
        {
            parent.Set(this, "", Id);
        }

        void Reset(object sender, RoutedEventArgs e)
        {
            parent.Reset(this, "", Id);
        }

    }
}
