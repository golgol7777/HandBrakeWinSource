/*  frmDownload.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;
using System.Net;
using System.IO;
using System.Threading;
using System.Diagnostics;

namespace Handbrake
{
    public partial class frmDownload : Form
    {
        private readonly Thread downloadThread;
        private Stream responceStream;
        private Stream loacalStream;
        private HttpWebRequest webRequest;
        private HttpWebResponse webResponse;
        private static int progress;
        private delegate void UpdateProgessCallback(Int64 BytesRead, Int64 TotalBytes);
        private delegate void DownloadCompleteCallback();
        private delegate void DownloadFailedCallback();

        private string file;

        public frmDownload(string filename)
        {
            InitializeComponent();

            file = filename;
            downloadThread = new Thread(Download);
            downloadThread.Start();
        }

        private void Download()
        {
            string tempPath = Path.Combine(Path.GetTempPath(), "handbrake-setup.exe");
            string hbUpdate = file;
            WebClient wcDownload = new WebClient();

            try
            {
                if (File.Exists(tempPath))
                    File.Delete(tempPath);

                webRequest = (HttpWebRequest)WebRequest.Create(hbUpdate);
                webRequest.Credentials = CredentialCache.DefaultCredentials;
                webResponse = (HttpWebResponse)webRequest.GetResponse();
                Int64 fileSize = webResponse.ContentLength;

                responceStream = wcDownload.OpenRead(hbUpdate);
                loacalStream = new FileStream(tempPath, FileMode.Create, FileAccess.Write, FileShare.None);

                int bytesSize;
                byte[] downBuffer = new byte[2048];

                long flength = 0;
                while ((bytesSize = responceStream.Read(downBuffer, 0, downBuffer.Length)) > 0)
                {
                    loacalStream.Write(downBuffer, 0, bytesSize);
                    flength = loacalStream.Length;
                    Invoke(new UpdateProgessCallback(this.UpdateProgress), new object[] { loacalStream.Length, fileSize });
                }

                responceStream.Close();
                loacalStream.Close();

                if (flength != fileSize)
                    Invoke(new DownloadFailedCallback(this.downloadFailed));
                else
                    Invoke(new DownloadCompleteCallback(this.downloadComplete));
            }
            catch (Exception)
            {
                // Do Nothing 
            }
        }

        private void UpdateProgress(Int64 BytesRead, Int64 TotalBytes)
        {
            try
            {
                long p = (BytesRead * 100) / TotalBytes;
                progress = int.Parse(p.ToString());
                progress_download.Value = progress;
                lblProgress.Text = (BytesRead / 1024) + "k of " + (TotalBytes / 1024) + "k ";
            }
            catch (Exception exc)
            {
                MessageBox.Show("Integer Conversion Error On Download \n" + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }

        private void downloadComplete()
        {
            lblProgress.Text = "Download Complete";
            btn_cancel.Text = "Close";

            string tempPath = Path.Combine(Path.GetTempPath(), "handbrake-setup.exe");
            Process.Start(tempPath);
            this.Close();
            Application.Exit();
        }

        private void downloadFailed()
        {
            lblProgress.Text = "Download Failed";
            btn_cancel.Text = "Close";
        }

        private void btn_cancel_Click(object sender, EventArgs e)
        {
            try
            {
                webResponse.Close();
                responceStream.Close();
                loacalStream.Close();
                downloadThread.Abort();
                progress_download.Value = 0;
                lblProgress.Text = "Download Stopped";
                this.Close();
            }
            catch (Exception)
            {
                // Do nothing
            }
        }
    }
}