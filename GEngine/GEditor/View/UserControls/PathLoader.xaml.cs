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

        public string LabelText = " Default :";

        public string MaterialUniqueName = "None";

        public PathLoader()
        {
            InitializeComponent();
        }

        void Set()
        {
            parent.Set(this, "", Id);
        }

        void Reset()
        {
            parent.Reset(this, "", Id);
        }

    }
}
