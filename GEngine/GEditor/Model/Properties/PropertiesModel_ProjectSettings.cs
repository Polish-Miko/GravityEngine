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
        private float _testValue1 = 0.0f;
        private float _testValue2 = 0.0f;
        private float _testValue3 = 0.0f;
        private bool _testBool = false;

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

        public float TestValue1
        {
            get => _testValue1;
            set
            {
                _testValue1 = value;
                OnPropertyChanged("TestValue1");
            }
        }

        public float TestValue2
        {
            get => _testValue2;
            set
            {
                _testValue2 = value;
                OnPropertyChanged("TestValue2");
            }
        }

        public float TestValue3
        {
            get => _testValue3;
            set
            {
                _testValue3 = value;
                OnPropertyChanged("TestValue3");
            }
        }

        public bool TestBool
        {
            get => _testBool;
            set
            {
                _testBool = value;
                OnPropertyChanged("TestBool");
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

        public void SetTestValue1(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetTestValue1();
            OnPropertyChanged();
        }

        public void SetTestValue2(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetTestValue2();
            OnPropertyChanged();
        }

        public void SetTestValue3(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetTestValue3();
            OnPropertyChanged();
        }

        public void SetTestBool(object sender, PropertyChangedEventArgs e)
        {
            PropertiesPanel.SetTestBool();
            OnPropertyChanged();
        }

        public event PropertyChangedEventHandler PropertyChanged;
        protected void OnPropertyChanged(string propertyName = "")
        {
            PropertyChangedEventHandler handler = PropertyChanged;
            if (propertyName == "SkyCubemapName")
                handler += SetSkyCubemapName;
            if (propertyName == "TestValue1")
                handler += SetTestValue1;
            if (propertyName == "TestValue2")
                handler += SetTestValue2;
            if (propertyName == "TestValue3")
                handler += SetTestValue3;
            if (propertyName == "TestBool")
                handler += SetTestBool;
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
