using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace GWinformLib
{
    public partial class Viewport: UserControl
    {
        public Viewport()
        {
            InitializeComponent();
        }



        protected override void WndProc(ref Message m)
        {
            IGCore.MsgProc(m.HWnd, m.Msg, m.WParam, m.LParam);
            base.WndProc(ref m);
        }
    }
}
