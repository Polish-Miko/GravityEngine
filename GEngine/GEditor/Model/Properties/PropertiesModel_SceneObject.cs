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
        private string _meshUniqueName;
        private string _materialUniqueName;

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
                    OnPropertyChanged("Transform");
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
                    OnPropertyChanged("Transform");
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
                    OnPropertyChanged("Transform");
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
                    OnPropertyChanged("Transform");
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
                    OnPropertyChanged("Transform");
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
                    OnPropertyChanged("Transform");
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
                    OnPropertyChanged("Transform");
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
                    OnPropertyChanged("Transform");
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
                    OnPropertyChanged("Transform");
                }
            }
        }

        public string MeshUniqueName
        {
            get => _meshUniqueName;
            set
            {
                _meshUniqueName = value;
                OnPropertyChanged("Mesh");
            }
        }

        public string MaterialUniqueName
        {
            get => _materialUniqueName;
            set
            {
                _materialUniqueName = value;
                OnPropertyChanged("Material");
            }
        }

        public void InitMeshName(string meshName)
        {
            _meshUniqueName = meshName;
            OnPropertyChanged();
        }

        public void InitMaterialName(string matName)
        {
            _materialUniqueName = matName;
            OnPropertyChanged();
        }

        public void SetTransform(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetSceneObjectTransform();
        }

        public void SetMesh(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetSceneObjectMesh();
            OnPropertyChanged();
        }

        public void SetMaterial(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetSceneObjectMaterial();
            OnPropertyChanged();
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (propertyName == "Transform")
                handler += SetTransform;
            else if (propertyName == "Mesh")
                handler += SetMesh;
            else if (propertyName == "Material")
                handler += SetMaterial;
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
