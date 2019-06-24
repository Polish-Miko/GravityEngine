using GEditor.View;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using GEditor.View.UserControls;

namespace GEditor.Model.Properties
{
    public class PropertiesModel_PathLoader : INotifyPropertyChanged
    {

        private string _labelText = " Default :";

        private string _materialUniqueName = "None";

        private PathLoader _loaderUI;

        public string LabelText
        {
            get => _labelText;
            set
            {
                _labelText = value;
                OnPropertyChanged();
            }
        }

        public string MaterialUniqueName
        {
            get => _materialUniqueName;
            set
            {
                _materialUniqueName = value;
                OnPropertyChanged();
            }
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (handler != null)
            {
                handler(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        public void SetParent(PathLoader parent)
        {
            _loaderUI = parent;
        }

    }
}
