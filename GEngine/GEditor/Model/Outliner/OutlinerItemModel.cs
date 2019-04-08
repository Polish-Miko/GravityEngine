using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media.Imaging;

namespace GEditor.Model.Outliner
{
    class OutlinerItemModel
    {

        private List<OutlinerItemModel> _children;

        private string _name;

        private BitmapImage _icon;

        private string _objectType;

        public OutlinerItemModel()
        {
            Children = new List<OutlinerItemModel>();
            Name = "";
            Icon = null;
        }

        public string Name { get => _name; set => _name = value; }

        public BitmapImage Icon { get => _icon; set => _icon = value; }

        public List<OutlinerItemModel> Children { get => _children; set => _children = value; }

        public string ObjectType { get => _objectType; set => _objectType = value; }
    }
}
