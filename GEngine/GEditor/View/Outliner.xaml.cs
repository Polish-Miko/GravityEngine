using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using GEditor.Model.Outliner;
using GEditor;
using System.Runtime.InteropServices;

namespace GEditor.View
{
    /// <summary>
    /// Interaction logic for Outliner.xaml
    /// </summary>
    public partial class Outliner : UserControl
    {
        Dictionary<string, BitmapImage> outlinerIcons = new Dictionary<string, BitmapImage>();

        //IGCore.VoidFuncPointerType mSetSceneObjectsCallback;

        MainWindow mainWindow;

        public Outliner()
        {
            InitializeComponent();

            LoadImages();

            //mSetSceneObjectsCallback = new IGCore.VoidFuncPointerType(SetSceneObjects);

            //IGCore.SetSetSceneObjectsCallback(mSetSceneObjectsCallback);
        }

        private OutlinerItemModel GetItemModelByName(string itemName)
        {
            foreach (object item in outlinerTreeView.Items)
            {
                OutlinerItemModel model = item as OutlinerItemModel;
                if (model.Name == itemName)
                    return model;
                OutlinerItemModel ret = GetItemModelInBranchByName(model, itemName);
                if (ret != null)
                    return ret;
            }
            return null;
        }

        private OutlinerItemModel GetItemModelInBranchByName(OutlinerItemModel branch, string itemName)
        {
            foreach (OutlinerItemModel item in branch.Children)
            {
                if (item.Name == itemName)
                    return item;
                foreach (OutlinerItemModel child in item.Children)
                {
                    OutlinerItemModel ret = GetItemModelInBranchByName(child, itemName);
                    if (ret != null)
                        return ret;
                }
            }
            return null;
        }

        void LoadImages()
        {
            BitmapImage img = new BitmapImage();
            img.BeginInit();
            img.CacheOption = BitmapCacheOption.OnLoad;
            img.CreateOptions = BitmapCreateOptions.IgnoreImageCache;
            img.UriSource = new Uri(AppDomain.CurrentDomain.BaseDirectory + "..\\GEditor\\Resource\\Image\\icon_outliner_scene.png", UriKind.Absolute);
            img.EndInit();
            outlinerIcons.Add("Scene", img);
            img = new BitmapImage();
            img.BeginInit();
            img.CacheOption = BitmapCacheOption.OnLoad;
            img.CreateOptions = BitmapCreateOptions.IgnoreImageCache;
            img.UriSource = new Uri(AppDomain.CurrentDomain.BaseDirectory + "..\\GEditor\\Resource\\Image\\icon_outliner_object.png", UriKind.Absolute);
            img.EndInit();
            outlinerIcons.Add("Mesh", img);
        }

        public void GetSceneObjects()
        {
            //outlinerTreeView.Items.Clear();
            outlinerTreeView.ItemsSource = null;

            List<OutlinerItemModel> root = new List<OutlinerItemModel>();

            OutlinerItemModel scene = new OutlinerItemModel();
            scene.Name = "Scene";
            scene.Icon = outlinerIcons["Scene"];
            scene.Children = GetAllSceneObjectsInScene();
            scene.ObjectType = "Scene";

            root.Add(scene);

            outlinerTreeView.ItemsSource = root;
            TreeViewItem rootItem = (TreeViewItem)outlinerTreeView.ItemContainerGenerator.ContainerFromItem(scene);
            rootItem.IsExpanded = true;
        }

        List<OutlinerItemModel> GetAllSceneObjectsInScene()
        {
            List<OutlinerItemModel> sObjects = new List<OutlinerItemModel>();

            int numObj = IGCore.GetSceneObjectNum();
            for (int i = 0; i < numObj; i++)
            {
                IntPtr pName = IGCore.GetSceneObjectName(i);
                string objName = Marshal.PtrToStringUni(pName);
                //string objName = Marshal.PtrToStringAnsi(pName);
                OutlinerItemModel obj = new OutlinerItemModel();
                obj.Icon = outlinerIcons["Mesh"];
                obj.Name = objName;
                obj.Children = new List<OutlinerItemModel>();
                obj.ObjectType = "Mesh";
                sObjects.Add(obj);
            }

            return sObjects;
        }

        public void Refresh()
        {
            GetSceneObjects();
        }

        void item_Expanded(object sender, RoutedEventArgs e)
        {
            ;
        }

        void item_Selected(object sender, RoutedEventArgs e)
        {
            OutlinerItemModel selected = (OutlinerItemModel)outlinerTreeView.SelectedItem;
            if (selected.ObjectType == "Mesh")
            {
                mainWindow.GetSceneObjectProperties(selected.Name);
                IGCore.SelectSceneObject(selected.Name);
            }
        }

        public void SetMainWindow(MainWindow mwRef)
        {
            mainWindow = mwRef;
        }

        private void CreateSceneObjectPlane(object sender, RoutedEventArgs e)
        {
            CreateSceneObject("Grid");
        }

        private void CreateSceneObjectCylinder(object sender, RoutedEventArgs e)
        {
            CreateSceneObject("Cylinder");
        }

        private void CreateSceneObjectBox(object sender, RoutedEventArgs e)
        {
            CreateSceneObject("Box");
        }

        private void CreateSceneObjectSphere(object sender, RoutedEventArgs e)
        {
            CreateSceneObject("Sphere");
        }

        private void CreateSceneObject(string meshUniqueName)
        {
            string sObjName = string.Empty;
            for (int i = 0; ; i++)
            {
                string tryName = "NewSceneObject";
                if (i > 0)
                    tryName = tryName + "_" + Convert.ToString(i);
                if (!IGCore.SceneObjectExists(tryName))
                {
                    sObjName = tryName;
                    break;
                }
            }

            IGCore.CreateSceneObject(sObjName, meshUniqueName);

            GetSceneObjects();
            OutlinerItemModel newModel = null;
            foreach (object item in outlinerTreeView.Items)
            {
                OutlinerItemModel model = item as OutlinerItemModel;
                if (model.Name == sObjName)
                {
                    newModel = model;
                    break;
                }
            }
            if (newModel != null)
            {
                //TreeViewItem tvi = (TreeViewItem)outlinerTreeView.ItemContainerGenerator.ContainerFromItem(newModel);
                //tvi.Focus();
            }
            //mainWindow.GetSceneObjectProperties(sObjName);
        }

        private void OnKeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.Delete)
            {
                OutlinerItemModel sObjModel = outlinerTreeView.SelectedItem as OutlinerItemModel;
                mainWindow.RemovePropertiesPanel();
                IGCore.DeleteSceneObject(sObjModel.Name);
                Refresh();
            }
        }
    }
}
