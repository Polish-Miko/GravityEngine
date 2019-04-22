using GEditor.View;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GEditor.Model.Properties
{
    class PropertiesModel_Material : INotifyPropertyChanged
    {

        private string _name;

        private string _matScaleX;

        private string _matScaleY;

        private string _albedoTextureName;

        private string _normalTextureName;

        private string _ormTextureName;
        
        private Properties_Material PropertiesPanel;

        public string Name
        {
            get => _name;
            set
            {
                _name = value;
                OnPropertyChanged("Name");
            }
        }

        public string MatScaleX
        {
            get => _matScaleX;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _matScaleX = value;
                    OnPropertyChanged("Scale");
                }
            }
        }

        public string MatScaleY
        {
            get => _matScaleY;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _matScaleY = value;
                    OnPropertyChanged("Scale");
                }
            }
        }

        public string AlbedoTextureName
        {
            get => _albedoTextureName;
            set
            {
                _albedoTextureName = value;
                OnPropertyChanged("Texture");
            }
        }

        public string NormalTextureName
        {
            get => _normalTextureName;
            set
            {
                _normalTextureName = value;
                OnPropertyChanged("Texture");
            }
        }

        public string OrmTextureName
        {
            get => _ormTextureName;
            set
            {
                _ormTextureName = value;
                OnPropertyChanged("Texture");
            }
        }

        public void SetName(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetName(Name);
        }

        public void SetScale(object sender, PropertyChangedEventArgs e)
        {
            float scaleX;
            float scaleY;
            if (float.TryParse(MatScaleX, out scaleX))
            {
                if (float.TryParse(MatScaleX, out scaleX))
                {
                    PropertiesPanel.SetScale(scaleX, scaleX);
                }
            }
        }

        /*
        public void SetTexture(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetTextureName(AlbedoTextureName, NormalTextureName, OrmTextureName);
        }
        */

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (propertyName == "Name")
                handler += SetName;
            else if (propertyName == "Scale")
                handler += SetScale;
            //else if (propertyName == "Texture")
                //handler += SetTexture;

            if (handler != null)
            {
                handler(this, new PropertyChangedEventArgs(propertyName));
            }
        }

        public void SetParent(Properties_Material parent)
        {
            PropertiesPanel = parent;
        }

    }
}
