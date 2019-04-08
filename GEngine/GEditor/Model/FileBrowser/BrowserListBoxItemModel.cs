using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Media.Imaging;

namespace GEditor.Model.FileBrowser
{
    class BrowserListBoxItemModel
    {

        private BitmapImage _icon;

        private string _fileType;

        private string _fileName;

        private string _filePath;

        public BitmapImage Icon { get => _icon; set => _icon = value; }

        public string FileType { get => _fileType; set => _fileType = value; }

        public string FileName { get => _fileName; set => _fileName = value; }

        public string FilePath { get => _filePath; set => _filePath = value; }
    }

}
