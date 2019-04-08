using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media.Imaging;

namespace GEditor.Model.FileBrowser
{
    public class BrowserTreeViewItemModel
    {
        private List<BrowserTreeViewItemModel> _children;

        private string _text;

        private BitmapImage _icon;

        private string _path;

        private bool _isFolder;
        
        public BrowserTreeViewItemModel()
        {
            _children = new List<BrowserTreeViewItemModel>();
            _text = "";
            _icon = null;
        }

        public List<BrowserTreeViewItemModel> Children
        {
            get { return _children; }
            set { _children = value; }
        }

        public string Text
        {
            get { return _text; }
            set { _text = value; }
        }
        
        public BitmapImage Icon
        {
            get { return _icon; }
            set { _icon = value; }
        }

        public string Path
        {
            get { return _path; }
            set { _path = value; }
        }

        public bool IsFolder
        {
            get { return _isFolder; }
            set { _isFolder = value; }
        }
    }
}
