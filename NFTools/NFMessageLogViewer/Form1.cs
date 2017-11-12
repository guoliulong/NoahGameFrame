using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Net;

namespace NFMessageLogViewer
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void openToolStripMenuItem_Click(object sender, EventArgs e)
        {

        }

        private void listView1_DragDrop(object sender, DragEventArgs e)
        {
            string[] files = (string[])e.Data.GetData(DataFormats.FileDrop);

            if (files.Length == 0)
                return;

            if(files[0].EndsWith(".nlog"))
            {
                ReadMsgLogFile(files[0]);
            }
            else
            {
                MessageBox.Show("请托入nlog 日志文件格式");
            }

        }

        private void listView1_DragEnter(object sender, DragEventArgs e)
        {
            if (e.Data.GetDataPresent(DataFormats.FileDrop, true) == true)
            {
                //允许拖放
                e.Effect = DragDropEffects.All;
            }
        }


        private void ReadMsgLogFile(string path)
        {
            FileStream f = System.IO.File.OpenRead(path);
            byte[] buffer = new byte[f.Length];
            f.Read(buffer, 0, buffer.Length);
            f.Close();

            _CurrentFileMS = new MemoryStream(buffer, 0, buffer.Length, false, true);
            _ProtoList = new List<ProtoListItem>();

            byte[] twoBytes = new byte[2];
            byte[] fourBytes = new byte[4];

            while (_CurrentFileMS.Position < _CurrentFileMS.Length)
            {
                _CurrentFileMS.Read(twoBytes, 0, 2);
                ushort protoID = (ushort)IPAddress.NetworkToHostOrder((short)BitConverter.ToUInt16(twoBytes, 0));

                _CurrentFileMS.Read(fourBytes, 0, 4);
                uint protoLen = (uint)IPAddress.NetworkToHostOrder((int)BitConverter.ToUInt32(fourBytes, 0));

                _CurrentFileMS.Position += protoLen-6;

                var item = new ProtoListItem();
                item.proto_id = protoID;
                item.buffer_offset = (uint)_CurrentFileMS.Position;
                item.data_length = protoLen;

                _ProtoList.Add(item);
            }

            listView1.View = View.Details;

            for(int i=0;i < _ProtoList.Count;++i)
            {
                var protoItem = _ProtoList[i];
                ListViewItem item = new ListViewItem(new string[] {
                    ((NFMsg.EGameMsgID)protoItem.proto_id).ToString(),
                    protoItem.proto_id.ToString(),
                    protoItem.buffer_offset.ToString(),
                    protoItem.data_length.ToString() });

                listView1.Items.Add(item);
            }

        }


        private MemoryStream _CurrentFileMS;
        private List<ProtoListItem> _ProtoList;

        class ProtoListItem
        {
            public ushort   proto_id;
            public uint     buffer_offset;
            public uint     data_length;
        }
    }
}
