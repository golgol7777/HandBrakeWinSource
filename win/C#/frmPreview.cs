﻿using System;
using System.Windows.Forms;
using System.Threading;
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.IO;
using Handbrake.EncodeQueue;
using Handbrake.Functions;
using QTOControlLib;
using QTOLibrary;

namespace Handbrake
{
    public partial class frmPreview : Form
    {

        QueryGenerator hb_common_func = new QueryGenerator();
        EncodeAndQueueHandler process = new EncodeAndQueueHandler();
        private delegate void UpdateUIHandler();
        String currently_playing = "";
        readonly frmMain mainWindow;
        private Thread player;
        private Boolean noQT;

        public frmPreview(frmMain mw)
        {
            try
            {
                InitializeComponent();
            }
            catch (Exception)
            {
                noQT = true;
            }
            this.mainWindow = mw;
            cb_preview.SelectedIndex = 0;
            cb_duration.SelectedIndex = 1;
        }

        #region Encode Sample
        private void btn_playVLC_Click(object sender, EventArgs e)
        {
            lbl_status.Visible = true;
            try
            {
                QTControl.URL = "";
                if (File.Exists(currently_playing))
                    File.Delete(currently_playing);
            }
            catch (Exception)
            {
                MessageBox.Show(this, "Unable to delete previous preview file. You may need to restart the application.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }

            btn_playQT.Enabled = false;
            btn_playVLC.Enabled = false;
            lbl_status.Text = "Encoding Sample for (VLC) ...";
            int duration;
            int.TryParse(cb_duration.Text, out duration);
            String query = hb_common_func.GenerateCLIQuery(mainWindow, duration, cb_preview.Text);
            ThreadPool.QueueUserWorkItem(procMonitor, query);
        }
        private void btn_playQT_Click(object sender, EventArgs e)
        {
            if (noQT)
            {
                MessageBox.Show(this, "It would appear QuickTime 7 is not installed or not accessible. Please (re)install QuickTime.", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }
            if (mainWindow.text_destination.Text.Contains(".mkv"))
            {
                MessageBox.Show(this,
                                "The QuickTime Control does not support MKV files, It is recommended you use VLC option instead.",
                                "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            else
            {
                lbl_status.Visible = true;
                try
                {
                    QTControl.URL = "";
                    if (File.Exists(currently_playing))
                        File.Delete(currently_playing);
                }
                catch (Exception)
                {
                    MessageBox.Show(this, "Unable to delete previous preview file. You may need to restart the application.", "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
                }

                btn_playQT.Enabled = false;
                btn_playVLC.Enabled = false;
                lbl_status.Text = "Encoding Sample for (QT) ...";
                int duration;
                int.TryParse(cb_duration.Text, out duration);
                String query = hb_common_func.GenerateCLIQuery(mainWindow, duration, cb_preview.Text);

                ThreadPool.QueueUserWorkItem(procMonitor, query);
            }
        }
        private void procMonitor(object state)
        {
            // Make sure we are not already encoding and if we are then display an error.
            if (process.hbProcess != null)
                MessageBox.Show(this, "Handbrake is already encoding a video!", "Status", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            else
            {
                process.RunCli((string)state);
                if (process.hbProcess != null)
                {
                    process.hbProcess.WaitForExit();
                    process.hbProcess = null;
                }
                encodeCompleted();
            }
        }
        private void encodeCompleted()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateUIHandler(encodeCompleted));
                    return;
                }
                if (!noQT)
                    btn_playQT.Enabled = true;
                btn_playVLC.Enabled = true;

                // Decide which player to use.
                String playerSelection = lbl_status.Text.Contains("QT") ? "QT" : "VLC";

                lbl_status.Text = "Loading Clip ...";

                // Get the sample filename
                if (mainWindow.text_destination.Text != "")
                    currently_playing = mainWindow.text_destination.Text.Replace(".mp4", "_sample.mp4").Replace(".m4v", "_sample.m4v").Replace(".mkv", "_sample.mkv"); ;

                // Play back in QT or VLC
                if (playerSelection == "QT")
                    play();
                else
                    playVLC();

                lbl_status.Text = "";
            }
            catch (Exception exc)
            {
                MessageBox.Show(this, "frmPreview.cs encodeCompleted " + exc, "Error", MessageBoxButtons.OK, MessageBoxIcon.Error);
            }
        }
        #endregion

        #region Playback

        /// <summary>
        /// Play the video back in the QuickTime control
        /// </summary>
        private void play()
        {
            player = new Thread(OpenMovie) { IsBackground = true };
            player.Start();
            lbl_status.Visible = false;
        }

        /// <summary>
        /// Play the video back in an external VLC player
        /// </summary>
        private void playVLC()
        {
            // Launch VLC and play video.
            if (currently_playing != "")
            {
                if (File.Exists(currently_playing))
                {
                    if (File.Exists(Properties.Settings.Default.VLC_Path))
                    {
                        String args = "\"" + currently_playing + "\"";
                        ProcessStartInfo vlc = new ProcessStartInfo(Properties.Settings.Default.VLC_Path, args);
                        Process.Start(vlc);
                        lbl_status.Text = "VLC will now launch.";
                    }
                    else
                        MessageBox.Show(this, "Unable to detect VLC Player. \nPlease make sure VLC is installed and the directory specified in HandBrake's options is correct. (See: \"Tools Menu > Options > Picture Tab\") ", "VLC", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                }
                else
                    MessageBox.Show(this, "Unable to find the preview file. Either the file was deleted or the encode failed. Check the activity log for details.", "VLC", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            lbl_status.Visible = false;
        }

        /// <summary>
        /// QT control - Open the file
        /// </summary>
        [STAThread]
        private void OpenMovie()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateUIHandler(OpenMovie));
                    return;
                }
                QTControl.URL = currently_playing;
                QTControl.SetSizing(QTSizingModeEnum.qtControlFitsMovie, true);
                QTControl.URL = currently_playing;
                QTControl.Show();

                this.ClientSize = QTControl.Size;
                this.Height += 25;
            }
            catch (COMException ex)
            {
                QTUtils qtu = new QTUtils();
                MessageBox.Show(this, "Unable to open movie:\n\nError Code: " + ex.ErrorCode.ToString("X") + "\nQT Error code : " + qtu.QTErrorFromErrorCode(ex.ErrorCode), "QT", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
            catch (Exception ex)
            {
                MessageBox.Show(this, "Unable to open movie:\n\n" + ex, "QT", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }
        #endregion
    }
}
