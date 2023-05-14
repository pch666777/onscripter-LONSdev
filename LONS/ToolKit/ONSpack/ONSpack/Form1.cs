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
            s = s + "." + outFormat;
            srcDir = textBox1.Text;

            textBox3.Clear();
            textBox3.AppendText("生成文件：" + s + "\r\n");
            fs = File.Open(s, FileMode.Create, FileAccess.ReadWrite);

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


        private string GetEncoder() {
            if (radioButton10.Checked) return "ch";
            else if (radioButton9.Checked) return "jp";
            else if (radioButton8.Checked) return "ru";
            else if (radioButton7.Checked) return "kr";
            return "utf8";
        }


        //开始生成文件
        private void StartPack() {
            if (outFormat == "nsa") PackNSA(srcDir);
        }

        //打包成nsa
        private void PackNSA(string dir) {
            //先处理文件，再处理目录
            foreach(string fn in Directory.GetFiles(dir)) {
                int bbk = 1;
            }
        }

        private void Form1_Load(object sender, EventArgs e) {
            outFormat = GetSuffix();
            textBox2.Text = Directory.GetCurrentDirectory();
            if (outFormat == "nsa" || outFormat == "ns2") textBox2.Text += "\\00." + outFormat;
            else textBox2.Text += "\\nscript." + outFormat;
        }
    }
}
