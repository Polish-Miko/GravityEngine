using GEditor.View;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GEditor.Model.Properties
{
    class PropertiesModel_ProjectSettings : INotifyPropertyChanged
    {

        private string _skyCubemapName;

        private Properties_ProjectSettings PropertiesPanel;

        public string SkyCubemapName
        {
            get => _skyCubemapName;
            set
            {
                if(PropertiesPanel.SkyCubemapNameAvailable(value))
                {
                    _skyCubemapName = value;
                    OnPropertyChanged("SkyCubemapName");
                }
            }
        }

        public void InitSkyCubemapName(string name)
        {
            _skyCubemapName = name;
            OnPropertyChanged();
        }

        public void SetSkyCubemapName(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetSkyCubemapName();
            OnPropertyChanged();
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (propertyName == "SkyCubemapName")
                handler += SetSkyCubemapName;
            if (handler != null)
            {
                handler(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        public void SetParent(Properties_ProjectSettings parent)
        {
            PropertiesPanel = parent;
        }

    }
}
