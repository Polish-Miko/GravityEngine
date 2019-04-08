using GEditor.View;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace GEditor.Model.Properties
{
    class PropertiesModel_SceneObject : INotifyPropertyChanged
    {

        private string _locX;
        private string _locY;
        private string _locZ;
        private string _rotX;
        private string _rotY;
        private string _rotZ;
        private string _scaleX;
        private string _scaleY;
        private string _scaleZ;

        private Properties_SceneObject PropertiesPanel;

        public string LocX
        {
            get => _locX;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _locX = value;
                    OnPropertyChanged("transform");
                }
            }
        }

        public string LocY
        {
            get => _locY;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _locY = value;
                    OnPropertyChanged("transform");
                }
            }
        }

        public string LocZ
        {
            get => _locZ;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _locZ = value;
                    OnPropertyChanged("transform");
                }
            }
        }

        public string RotX
        {
            get => _rotX;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _rotX = value;
                    OnPropertyChanged("transform");
                }
            }
        }

        public string RotY
        {
            get => _rotY;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _rotY = value;
                    OnPropertyChanged("transform");
                }
            }
        }

        public string RotZ
        {
            get => _rotZ;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _rotZ = value;
                    OnPropertyChanged("transform");
                }
            }
        }

        public string ScaleX
        {
            get => _scaleX;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _scaleX = value;
                    OnPropertyChanged("transform");
                }
            }
        }

        public string ScaleY
        {
            get => _scaleY;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _scaleY = value;
                    OnPropertyChanged("transform");
                }
            }
        }

        public string ScaleZ
        {
            get => _scaleZ;
            set
            {
                float f;
                if (float.TryParse(value, out f))
                {
                    _scaleZ = value;
                    OnPropertyChanged("transform");
                }
            }
        }

        public void SetProperties(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetSceneObjectProperties();
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

        public float[] GetTransform()
        {
            float[] trans = { Convert.ToSingle(LocX), Convert.ToSingle(LocY), Convert.ToSingle(LocZ), Convert.ToSingle(RotX), Convert.ToSingle(RotY), Convert.ToSingle(RotZ),
                Convert.ToSingle(ScaleX), Convert.ToSingle(ScaleY), Convert.ToSingle(ScaleZ) };
            return trans;
        }

        public void SetTransform(float[] trans)
        {
            _locX = trans[0].ToString();
            _locY = trans[1].ToString();
            _locZ = trans[2].ToString();
            _rotX = trans[3].ToString();
            _rotY = trans[4].ToString();
            _rotZ = trans[5].ToString();
            _scaleX = trans[6].ToString();
            _scaleY = trans[7].ToString();
            _scaleZ = trans[8].ToString();
        }

        public void SetParent(Properties_SceneObject parent)
        {
            PropertiesPanel = parent;
        }

    }
}
