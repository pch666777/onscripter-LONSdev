using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.IO;

namespace ONSpack {
    public partial class Form1 : Form {
        FileStream fs;
        string outFormat;
        string srcDir;
        Encoding dstEncoder;

        public Form1() {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e) {
            FolderBrowserDialog fdio = new FolderBrowserDialog();
            fdio.ShowDialog();
            if (fdio.SelectedPath.Length != 0) textBox1.Text = fdio.SelectedPath;
        }

        private void button3_Click(object sender, EventArgs e) {
            if(textBox1.Text.Length == 0 || !Directory.Exists(textBox1.Text)) {
                MessageBox.Show("打包路径无效！");
                return;
            }
            if(textBox2.Text.Length == 0) {
                MessageBox.Show("必须选择输出路径！");
                return;
            }
            //替换文件缀
            string s = textBox2.Text.Substring(0, textBox2.Text.LastIndexOf("."));
            outFormat = GetSuffix();
            dstEncoder = GetEncoder();
            s = s + "." + outFormat;
            srcDir = textBox1.Text;

            textBox3.Clear();
            textBox3.AppendText("生成文件：" + s + "\r\n");
            fs = File.Open(s, FileMode.Create, FileAccess.ReadWrite);

            StartPack();

            fs.Close();
            //打包过程
            textBox3.AppendText("处理完成！");

        }

        private void button2_Click(object sender, EventArgs e) {
            SaveFileDialog dio = new SaveFileDialog();
            //设置存储的后缀
            string suffix = GetSuffix();
            dio.Filter = "(*." + suffix +")|*." + suffix;
            dio.ShowDialog();
            if (dio.FileName.Length != 0) textBox2.Text = dio.FileName;
        }


        private string GetSuffix() {
            if (radioButton1.Checked) return "nsa";
            else if (radioButton2.Checked) return "ns2";
            else if (radioButton3.Checked) return "dat";
            else if (radioButton4.Checked) return "nt";
            return "nt2";
        }


        private Encoding GetEncoder() {
            if (radioButton10.Checked) return Encoding.GetEncoding(936);
            else if (radioButton9.Checked) return Encoding.GetEncoding(932);
            else if (radioButton8.Checked) return null;
            else if (radioButton7.Checked) return null;
            return Encoding.UTF8;
        }


        //开始生成文件
        private void StartPack() {
            if (outFormat == "nsa") PackNSA(srcDir);
        }

        //打包成nsa
        private void PackNSA(string dir) {
            byte[] header = new byte[6];
            //写入文件头，前两个字节是文件数量，后4个字节是数据的起始位置
            fs.Write(header, 0, 6);
            //历遍文件，生成索引
            PackNSA_first(dir);

            //if(count > 0xffff) {
            //    MessageBox.Show("文件数大于65535个，请减少后再打包！");
            //    return;
            //}
            //else if(count == 0) {
            //    MessageBox.Show("所选的目录是空的！");
            //    return;
            //}
            //textBox3.AppendText("总共发现" + count.ToString() + "个文件\r\n");

            //处理文件头
            header = new byte[fs.Length];
            fs.Position = 0;
            fs.Read(header, 0, (int)fs.Length);
            //fs.Write(header, 0, 2);
           // header = BitConverter.GetBytes((uint)fs.Length);
            //fs.Write(header, 0, 4);

            

            //int headerPos = 6;  //文件头的读写位置
            //int dataStart = (int)fs.Length;  //数据的起始位置

        }

        private void PackNSA_first(string dir) {
            //先处理文件，再处理目录
            foreach (string fn in Directory.GetFiles(dir)) {
                string fileName = fn.Substring(srcDir.Length + 1);
                //替换所有的 /为 \，有可能在wine下打包的
                fileName = fileName.Replace("/", "\\");
                //文件名 /0结尾
                //标记   是否压缩0，1个字节
                //原始长度 4字节
                //压缩后的长度 4字节
                byte[] tmp = TransStringEncoder(fileName);
                if (tmp == null) return;
                fs.Write(tmp, 0, tmp.Length);
                //1+1+4+4预先写入数据，等复制数据时才处理
                tmp = new byte[10];
                fs.Write(tmp, 0, tmp.Length);
            }
            //处理目录
            foreach (string fn in Directory.GetDirectories(dir)) {
                //string filename = fn;
                PackNSA_first(fn);
            }
        }


        private void PackNSA_second(ref byte[] bin, int dataStart) {
            int headerPos = 6;
            int dataLen;  //数据的长度
            int maxLen = 10 * 1000 * 1000;
            byte[] data = new byte[maxLen];
            byte[] tmp;
            while (headerPos < dataStart) {
                int slen = 0;
                while (bin[headerPos + slen] != 0) slen += 1;
                string fn = dstEncoder.GetString(bin, headerPos, slen);
                headerPos += slen + 1; //包含了\0
                headerPos += 1;  //flag

                fn = srcDir + "\\" + fn;
                FileStream dfs = File.Open(fn, FileMode.Open);
                if(dfs.Length > 0x7fffffff) {
                    MessageBox.Show("出现了超大文件！请检查数据是否正确！");
                    return;
                }

                //每次读取10MB
                int sumLen = 0;
                int oneLen = 0;
                do {
                    oneLen = dfs.Read(data, 0, maxLen);
                    fs.Write(data, 0, oneLen);
                    sumLen += oneLen;
                } while (oneLen == maxLen);
                dfs.Close();

                //原始文件长度，注意是大端模式
                tmp = BitConverter.GetBytes(sumLen);




            }

        }

        private byte[] TransStringEncoder(string src) {
            if(dstEncoder != null) {
                //转换后要做一下检测
                byte[] tmp = dstEncoder.GetBytes(src);
                if (dstEncoder.GetString(tmp) != src) textBox1.AppendText("[" + src + "]存在编码转换问题\r\n");
                return tmp;
            }
            else {
                MessageBox.Show("文字编码器无效！");
            }
            return null;
        }

        private void Form1_Load(object sender, EventArgs e) {
            outFormat = GetSuffix();
            textBox2.Text = Directory.GetCurrentDirectory();
            if (outFormat == "nsa" || outFormat == "ns2") textBox2.Text += "\\00." + outFormat;
            else textBox2.Text += "\\nscript." + outFormat;
        }
    }
}
