using GEditor.View;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GEditor.Model.Properties
{
    class PropertiesModel_Texture : INotifyPropertyChanged
    {

        private bool _bSrgb;

        private Properties_Texture PropertiesPanel;

        public bool bSrgb
        {
            get => _bSrgb;
            set
            {
                _bSrgb = value;
                OnPropertyChanged("srgb");
            }
        }

        public void SetProperties(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetTextureProperties();
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            handler += SetProperties;
            if (handler != null)
            {
                handler(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        public void SetParent(Properties_Texture parent)
        {
            PropertiesPanel = parent;
        }

    }
}
