using System.Runtime.InteropServices;
using Timer = System.Windows.Forms.Timer;

namespace ShowDatas
{
    public partial class Form1 : Form
    {
        [DllImport(@"C:\Users\ipek_\source\repos\Enovas\x64\Debug\ConsoleApplication1.dll")]
        public static extern float ReturnCpuTemp();
        [DllImport(@"C:\Users\ipek_\source\repos\Enovas\x64\Debug\ConsoleApplication1.dll")]
        public static extern float ReturnDiskUsage();
        [DllImport(@"C:\Users\ipek_\source\repos\Enovas\x64\Debug\ConsoleApplication1.dll")]
        public static extern float ReturnRamUsage();
        [DllImport(@"C:\Users\ipek_\source\repos\Enovas\x64\Debug\ConsoleApplication1.dll")]
        public static extern float ReturnCpuUsage();
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            Timer timer = new Timer();
            timer.Interval = (30 * 1000); // 30 secs
            timer.Tick += new EventHandler(timer_Tick);
            timer.Start();

            textBox1.Text = ReturnCpuTemp().ToString() + " °C";
            textBox2.Text = ReturnDiskUsage().ToString() + " %";
            textBox3.Text = ReturnRamUsage().ToString() + " %";
            textBox4.Text = ReturnCpuUsage().ToString() + " %";
        }
        private void timer_Tick(object sender, EventArgs e)
        {
            textBox1.Text = ReturnCpuTemp().ToString() + " °C";
            textBox2.Text = ReturnDiskUsage().ToString() + " %";
            textBox3.Text = ReturnRamUsage().ToString() + " %";
            textBox4.Text = ReturnCpuUsage().ToString() + " %";
        }
    }
}