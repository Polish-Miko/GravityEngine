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
using System.Windows.Interop;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Forms;
using System.Threading;
//using System.Windows.Interop;
using Microsoft.Win32;
using GWinformLib;
using GEditor.View;

namespace GEditor
{
    /// <summary>
    /// MainWindow.xaml 的交互逻辑
    /// </summary>
    public partial class MainWindow : Window
    {
        private string mProjectName;

        private GWinformLib.Viewport viewport = new GWinformLib.Viewport();

        private Outliner outliner = new Outliner();

        private FileBrowser fileBrowser = new FileBrowser();

        Properties_SceneObject properties_SceneObject;
        Properties_Texture properties_Texture;
        Properties_Material properties_Material;
        Properties_Mesh properties_Mesh;
        Properties_ProjectSettings properties_ProjectSettings;

        public object WinInterop { get; private set; }

        public MainWindow()
        {
            //this.SourceInitialized += new EventHandler(win_SourceInitialized);
            InitializeComponent();
            //viewport.TopLevel = false;
            viewport.BorderStyle = BorderStyle.None;
            viewportHost.Child = viewport;
            FileBrowserPanel.Children.Add(fileBrowser);
            fileBrowser.SetMainWindow(this);
            OutlinerPanel.Children.Add(outliner);
            outliner.SetMainWindow(this);
        }

        /*
        protected override void OnSourceInitialized(EventArgs e)
        {
            base.OnSourceInitialized(e);
            this.win_SourceInitialized(this, e);
        }

        void win_SourceInitialized(object sender, EventArgs e)
        {
            HwndSource hwndSource = PresentationSource.FromVisual(this) as HwndSource;
            if (hwndSource != null)
                hwndSource.AddHook(new HwndSourceHook(WndProc));
        }
        */

        protected virtual IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
        {
            /*
            switch (msg)
            {
                case Microsoft.Win32.WM_SIZEING:
                    break;
            }
            */
            //IGCore.MsgProc(hwnd, msg, wParam, lParam);

            return IntPtr.Zero;
        }

        private void OpenProject(object sender, RoutedEventArgs e)
        {
            // Create OpenFileDialog 
            Microsoft.Win32.OpenFileDialog dlg = new Microsoft.Win32.OpenFileDialog();

            // Set filter for file extension and default file extension 
            dlg.DefaultExt = ".gproj";
            dlg.Filter = "GE Project Files (*.gproj)|*.gproj";

            // Display OpenFileDialog by calling ShowDialog method 
            Nullable<bool> result = dlg.ShowDialog();

            // Get the selected file name and display in a TextBox 
            if (result == true)
            {
                // Open document 
                string filename = dlg.FileName;

                mProjectName = System.IO.Path.GetFileNameWithoutExtension(filename);

                fileBrowser.SetWorkDirectory(System.IO.Path.GetDirectoryName(filename) + @"\");
                fileBrowser.LoadBrowser();

                IGCore.SetWorkDirectory(System.IO.Path.GetDirectoryName(filename) + @"\");
                IGCore.SetProjectName(System.IO.Path.GetFileNameWithoutExtension(filename));
                IntPtr hwnd = viewport.Handle;
                double h = viewport.Height;
                double w = viewport.Width;
                IGCore.InitD3D(hwnd, w, h);

                IGCore.SetSelectSceneObjectCallback(SelectSceneObject);
                IGCore.SetRefreshSceneObjectTransformCallback(RefreshTransform);
                outliner.GetSceneObjects();

                IGCore.Run();
            }

        }

        private void SaveProject(object sender, RoutedEventArgs e)
        {
            IGCore.SaveProject();
        }

        private void NewProject(object sender, RoutedEventArgs e)
        {
            ;
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            //IntPtr hwnd = viewport.Handle;
            //double h = viewport.Height;
            //double w = viewport.Width;
            //IGCore.InitD3D(hwnd, w, h);
            //IGRenderer.Run();

            //HwndSource hwndSource = HwndSource.FromHwnd(hwnd);
            //HwndSource hwndSource = PresentationSource.FromVisual(this) as HwndSource;
            //if (hwndSource != null)
            //hwndSource.AddHook(new HwndSourceHook(WndProc));

        }

        void StartMainWindorMessageLoop()
        {
            //Window a = new Window();
            //a.Dispacher.Invoke(DispatcherPriority.Normal, (Action)delegate () { a.Show(); });

            //ViewportGrid.Children.Add();

            //HwndSource hwndSource = HwndSource.FromHwnd(hwnd);
            //HwndSource hwndSource = PresentationSource.FromVisual(this) as HwndSource;
            //if (hwndSource != null)
            //hwndSource.AddHook(new HwndSourceHook(WndProc));

            /*
            GEditor.View.Viewport viewport = new GEditor.View.Viewport();
            Dispatcher.Invoke((Action)(() =>
            {
                ViewportGrid.Children.Add(viewport);
            }));
            */

            //System.Windows.Threading.Dispatcher.Run();

        }

        void CreateViewport()
        {

        }

        public void GetSceneObjectProperties(string objName)
        {
            PropertiesPanel.Children.Clear();
            properties_SceneObject = new Properties_SceneObject();
            properties_SceneObject.SetMainWindow(this);
            properties_SceneObject.SetObjectName(objName);
            PropertiesPanel.Children.Add(properties_SceneObject);
            properties_SceneObject.GetSceneObjectProperties();
        }

        public void GetTextureProperties(string txtName)
        {
            PropertiesPanel.Children.Clear();
            properties_Texture = new Properties_Texture();
            properties_Texture.SetMainWindow(this);
            properties_Texture.SetTextureName(txtName);
            PropertiesPanel.Children.Add(properties_Texture);
            properties_Texture.GetTextureProperties();
        }

        public void GetMaterialPropertiesByUniqueName(string matUniqueName)
        {
            PropertiesPanel.Children.Clear();
            properties_Material = new Properties_Material();
            properties_Material.SetMainWindow(this);
            properties_Material.SetMaterialUniqueName(matUniqueName);
            PropertiesPanel.Children.Add(properties_Material);
            properties_Material.GetMaterialProperties();
        }

        public void GetMeshPropertiesByUniqueName(string meshName)
        {
            PropertiesPanel.Children.Clear();
            properties_Mesh = new Properties_Mesh();
            properties_Mesh.SetMainWindow(this);
            properties_Mesh.SetMeshName(meshName);
            PropertiesPanel.Children.Add(properties_Mesh);
            properties_Mesh.GetMeshProperties();
        }

        public string GetBrowserSelectedFilePath()
        {
            return fileBrowser.GetSelectedFilePath();
        }

        public string GetBrowserSelectedFileUniqueName()
        {
            return fileBrowser.GetSelectedFileUniqueName();
        }

        public string GetWorkDirectory()
        {
            return fileBrowser.GetWorkDirectory();
        }

        public void RefreshBrowser()
        {
            fileBrowser.RefreshListBox();
        }

        public void RefreshOutliner()
        {
            outliner.Refresh();
        }

        private void ProjectSettings(object sender, RoutedEventArgs e)
        {
            PropertiesPanel.Children.Clear();
            properties_ProjectSettings = new Properties_ProjectSettings();
            properties_ProjectSettings.SetMainWindow(this);
            PropertiesPanel.Children.Add(properties_ProjectSettings);
            properties_ProjectSettings.GetSettings();
        }

        public void SelectSceneObject(string sceneObjectName)
        {
            //outliner.SelectSceneObject(sceneObjectName);
            GetSceneObjectProperties(sceneObjectName);
        }

        public void RefreshTransform()
        {
            if (properties_SceneObject != null)
                properties_SceneObject.RefreshTransform();
        }

        public void RemovePropertiesPanel()
        {
            properties_Material = null;
            properties_ProjectSettings = null;
            properties_SceneObject = null;
            properties_Texture = null;
            PropertiesPanel.Children.Clear();
        }

    }
}
