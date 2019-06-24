using GEditor.View;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GEditor.Model.Properties
{
    class PropertiesModel_Mesh : INotifyPropertyChanged
    {

        private Dictionary<string, string> _materialUniqueName;

        private Properties_Mesh PropertiesPanel;

        public void SetMaterialUniqueName(string submeshName, string materialName)
        {
            if (_materialUniqueName.ContainsKey(submeshName))
                _materialUniqueName[submeshName] = materialName;
            OnPropertyChanged("material");
        }

        public bool bSrgb
        {
            get => _bSrgb;
            set
            {
                _bSrgb = value;
                OnPropertyChanged("srgb");
            }
        }

        public void InitMaterials(Dictionary<string, string> initMaterials)
        {
            _materialUniqueName = initMaterials;
            OnPropertyChanged();
        }

        public void SetProperties(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetMeshMaterials();
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if(propertyName=="material")
                handler += SetProperties;
            if (handler != null)
            {
                handler(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        public void SetParent(Properties_Mesh parent)
        {
            PropertiesPanel = parent;
        }

    }
}
