/*  frmQueue.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.ComponentModel;
    using System.Windows.Forms;
    using Functions;
    using Model;
    using Services;

    /// <summary>
    /// The Queue Window
    /// </summary>
    public partial class frmQueue : Form
    {
        /// <summary>
        /// Update Handler Delegate
        /// </summary>
        private delegate void UpdateHandler();

        /// <summary>
        /// An instance of the Queue service
        /// </summary>
        private readonly Queue queue;

        /// <summary>
        /// A reference to the main application window
        /// </summary>
        private readonly frmMain mainWindow;

        /// <summary>
        /// Initializes a new instance of the <see cref="frmQueue"/> class.
        /// </summary>
        /// <param name="q">
        /// An instance of the queue service.
        /// </param>
        /// <param name="mw">
        /// The main window.
        /// </param>
        public frmQueue(Queue q, frmMain mw)
        {
            InitializeComponent();

            this.mainWindow = mw;

            this.queue = q;
            queue.EncodeStarted += new EventHandler(QueueOnEncodeStart);
            queue.QueueCompleted += new EventHandler(QueueOnQueueFinished);
            queue.QueuePauseRequested += new EventHandler(QueueOnPaused);
        }

        /// <summary>
        /// Handle the Queue Paused event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void QueueOnPaused(object sender, EventArgs e)
        {
            SetUiEncodeFinished();
            UpdateUiElements();
        }

        /// <summary>
        /// Handle the Queue Finished event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void QueueOnQueueFinished(object sender, EventArgs e)
        {
            SetUiEncodeFinished();
            ResetQueue(); // Reset the Queue Window
        }

        /// <summary>
        /// Handle the Encode Started event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void QueueOnEncodeStart(object sender, EventArgs e)
        {
            SetUiEncodeStarted(); // make sure the UI is set correctly
            SetCurrentEncodeInformation();
            UpdateUiElements(); // Redraw the Queue, a new encode has started.
        }

        /// <summary>
        /// Initializes the Queue list with the Arraylist from the Queue class
        /// </summary>
        public void SetQueue()
        {
            UpdateUiElements();
        }

        /// <summary>
        /// Initializes the Queue list, then shows and activates the window
        /// </summary>
        public new void Show()
        {
            Show(true);
        }

        /// <summary>
        /// Initializes the Queue list only if doSetQueue is true, then shows and activates the window
        /// </summary>
        /// <param name="doSetQueue">Indicates whether to call setQueue() before showing the window</param>
        public void Show(bool doSetQueue)
        {
            if (doSetQueue) SetQueue();
            base.Show();

            // Activate();
        }

        /// <summary>
        /// Handle the Encode button Click event
        /// </summary>
        /// <param name="sender">The sender</param>
        /// <param name="e">the EventArgs</param>
        private void BtnEncodeClick(object sender, EventArgs e)
        {
            if (queue.PauseRequested)
            {
                SetUiEncodeStarted();
                MessageBox.Show("Encoding restarted", "Information", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }

            if (!queue.IsEncoding)
                queue.Start();
        }

        /// <summary>
        /// Handle the Pause button click event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void BtnPauseClick(object sender, EventArgs e)
        {
            queue.Pause();
            SetUiEncodeFinished();
            ResetQueue();
            MessageBox.Show(
                "No further items on the queue will start. The current encode process will continue until it is finished. \nClick 'Encode' when you wish to continue encoding the queue.",
                "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        // UI Work

        /// <summary>
        /// Setup the UI to show that an encode has started
        /// </summary>
        private void SetUiEncodeStarted()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(SetUiEncodeStarted));
                return;
            }
            btn_encode.Enabled = false;
            btn_pause.Visible = true;
        }

        /// <summary>
        /// Setup the UI to indicate that an encode has finished.
        /// </summary>
        private void SetUiEncodeFinished()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(SetUiEncodeFinished));
                return;
            }
            btn_pause.Visible = false;
            btn_encode.Enabled = true;
        }

        /// <summary>
        /// Reset the Queue Window display
        /// </summary>
        private void ResetQueue()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(ResetQueue));
                return;
            }
            btn_pause.Visible = false;
            btn_encode.Enabled = true;

            lbl_source.Text = "-";
            lbl_dest.Text = "-";
            lbl_vEnc.Text = "-";
            lbl_aEnc.Text = "-";
            lbl_title.Text = "-";
            lbl_chapt.Text = "-";

            lbl_encodesPending.Text = list_queue.Items.Count + " encode(s) pending";
        }

        /// <summary>
        /// Redraw the Queue window with the latest information about HandBrakes status
        /// </summary>
        private void RedrawQueue()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(RedrawQueue));
                return;
            }

            list_queue.Items.Clear();
            ReadOnlyCollection<Job> theQueue = queue.CurrentQueue;
            foreach (Job queueItem in theQueue)
            {
                string qItem = queueItem.Query;
                QueryParser parsed = Functions.QueryParser.Parse(qItem);

                // Get the DVD Title
                string title = parsed.Title == 0 ? "Auto" : parsed.Title.ToString();

                // Get the DVD Chapters
                string chapters;
                if (parsed.ChapterStart == 0)
                    chapters = "Auto";
                else
                {
                    chapters = parsed.ChapterStart.ToString();
                    if (parsed.ChapterFinish != 0)
                        chapters = chapters + " - " + parsed.ChapterFinish;
                }

                ListViewItem item = new ListViewItem();
                item.Text = title; // Title
                item.SubItems.Add(chapters); // Chapters
                item.SubItems.Add(queueItem.Source); // Source
                item.SubItems.Add(queueItem.Destination); // Destination
                item.SubItems.Add(parsed.VideoEncoder); // Video

                // Display The Audio Track Information
                string audio = string.Empty;
                foreach (AudioTrack track in parsed.AudioInformation)
                {
                    if (audio != string.Empty)
                        audio += ", " + track.Encoder;
                    else
                        audio = track.Encoder;
                }
                item.SubItems.Add(audio); // Audio

                list_queue.Items.Add(item);
            }
        }

        /// <summary>
        /// Update the UI elements
        /// </summary>
        private void UpdateUiElements()
        {
            if (InvokeRequired)
            {
                BeginInvoke(new UpdateHandler(UpdateUiElements));
                return;
            }

            RedrawQueue();
            lbl_encodesPending.Text = list_queue.Items.Count + " encode(s) pending";
        }

        /// <summary>
        /// Set the window up with the current encode information
        /// </summary>
        private void SetCurrentEncodeInformation()
        {
            try
            {
                if (InvokeRequired)
                {
                    BeginInvoke(new UpdateHandler(SetCurrentEncodeInformation));
                }

                // found query is a global varible
                QueryParser parsed = QueryParser.Parse(queue.LastEncode.Query);
                lbl_source.Text = queue.LastEncode.Source;
                lbl_dest.Text = queue.LastEncode.Destination;

                lbl_title.Text = parsed.Title == 0 ? "Auto" : parsed.Title.ToString();

                if (Equals(parsed.ChapterStart, 0))
                    lbl_chapt.Text = "Auto";
                else
                {
                    string chapters = parsed.ChapterStart.ToString();
                    if (parsed.ChapterFinish != 0)
                        chapters = chapters + " - " + parsed.ChapterFinish;
                    lbl_chapt.Text = chapters;
                }

                lbl_vEnc.Text = parsed.VideoEncoder;

                // Display The Audio Track Information
                string audio = string.Empty;
                foreach (AudioTrack track in parsed.AudioInformation)
                {
                    if (audio != string.Empty)
                        audio += ", " + track.Encoder;
                    else
                        audio = track.Encoder;
                }
                lbl_aEnc.Text = audio;
            }
            catch (Exception)
            {
                // Do Nothing
            }
        }

        /// <summary>
        /// Delete the currently selected items on the queue
        /// </summary>
        private void DeleteSelectedItems()
        {
            // If there are selected items
            if (list_queue.SelectedIndices.Count > 0)
            {
                // Save the selected indices to select them after the move
                List<int> selectedIndices = new List<int>(list_queue.SelectedIndices.Count);
                foreach (int selectedIndex in list_queue.SelectedIndices)
                    selectedIndices.Add(selectedIndex);

                int firstSelectedIndex = selectedIndices[0];

                // Reverse the list to delete the items from last to first (preserves indices)
                selectedIndices.Reverse();

                // Remove each selected item
                foreach (int selectedIndex in selectedIndices)
                    queue.Remove(selectedIndex);

                UpdateUiElements();

                // Select the item where the first deleted item was previously
                if (firstSelectedIndex < list_queue.Items.Count)
                    list_queue.Items[firstSelectedIndex].Selected = true;
            }

            list_queue.Select(); // Activate the control to show the selected items
        }

        // Queue Management
        /// <summary>
        /// Handle the Move Up Menu Item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuUpClick(object sender, EventArgs e)
        {
            MoveUp();
        }

        /// <summary>
        /// Handle the Move down Menu Item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuDownClick(object sender, EventArgs e)
        {
            MoveDown();
        }

        /// <summary>
        /// Edit a job
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuEditClick(object sender, EventArgs e)
        {
            if (list_queue.SelectedIndices != null && list_queue.SelectedIndices.Count != 0)
            {
                lock (queue)
                {
                    lock (list_queue)
                    {
                        int index = list_queue.SelectedIndices[0];
                        mainWindow.RecievingJob(queue.GetJob(index));
                        queue.Remove(index);
                        RedrawQueue();
                    }
                }
            }
        }

        /// <summary>
        /// Handle the delete Menu Item
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuDeleteClick(object sender, EventArgs e)
        {
            DeleteSelectedItems();
        }

        /// <summary>
        /// Handle the Button Up Click
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnUpClick(object sender, EventArgs e)
        {
            MoveUp();
        }

        /// <summary>
        /// Handle the button down click
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnDownClick(object sender, EventArgs e)
        {
            MoveDown();
        }

        /// <summary>
        /// Handle the delete button click
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnDeleteClick(object sender, EventArgs e)
        {
            DeleteSelectedItems();
        }

        /// <summary>
        /// Handle the delete keyboard press
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void ListQueueDeleteKey(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.Delete)
                DeleteSelectedItems();
        }

        /// <summary>
        /// Move items up in the queue
        /// </summary>
        private void MoveUp()
        {
            // If there are selected items and the first item is not selected
            if (list_queue.SelectedIndices.Count > 0 && !list_queue.SelectedIndices.Contains(0))
            {
                // Copy the selected indices to preserve them during the movement
                List<int> selectedIndices = new List<int>(list_queue.SelectedIndices.Count);
                foreach (int selectedIndex in list_queue.SelectedIndices)
                    selectedIndices.Add(selectedIndex);

                // Move up each selected item
                foreach (int selectedIndex in selectedIndices)
                    queue.MoveUp(selectedIndex);

                UpdateUiElements();

                // Keep the selected item(s) selected, now moved up one index
                foreach (int selectedIndex in selectedIndices)
                    if (selectedIndex - 1 > -1) // Defensive programming: ensure index is good
                        list_queue.Items[selectedIndex - 1].Selected = true;
            }

            list_queue.Select(); // Activate the control to show the selected items
        }

        /// <summary>
        /// Move items down in the queue
        /// </summary>
        private void MoveDown()
        {
            // If there are selected items and the last item is not selected
            if (list_queue.SelectedIndices.Count > 0 &&
                !list_queue.SelectedIndices.Contains(list_queue.Items[list_queue.Items.Count - 1].Index))
            {
                // Copy the selected indices to preserve them during the movement
                List<int> selectedIndices = new List<int>(list_queue.SelectedIndices.Count);
                foreach (int selectedIndex in list_queue.SelectedIndices)
                    selectedIndices.Add(selectedIndex);

                // Reverse the indices to move the items down from last to first (preserves indices)
                selectedIndices.Reverse();

                // Move down each selected item
                foreach (int selectedIndex in selectedIndices)
                    queue.MoveDown(selectedIndex);

                UpdateUiElements();

                // Keep the selected item(s) selected, now moved down one index
                foreach (int selectedIndex in selectedIndices)
                    if (selectedIndex + 1 < list_queue.Items.Count) // Defensive programming: ensure index is good
                        list_queue.Items[selectedIndex + 1].Selected = true;
            }

            list_queue.Select(); // Activate the control to show the selected items
        }

        // Queue Import/Export Features

        /// <summary>
        /// Create a batch script
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuBatchClick(object sender, EventArgs e)
        {
            SaveFile.FileName = string.Empty;
            SaveFile.Filter = "Batch|.bat";
            SaveFile.ShowDialog();
            if (SaveFile.FileName != String.Empty)
                queue.WriteBatchScriptToFile(SaveFile.FileName);
        }

        /// <summary>
        /// Export Queue
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuExportClick(object sender, EventArgs e)
        {
            SaveFile.FileName = string.Empty;
            SaveFile.Filter = "HandBrake Queue|*.queue";
            SaveFile.ShowDialog();
            if (SaveFile.FileName != String.Empty)
                queue.WriteQueueStateToFile(SaveFile.FileName);
        }

        /// <summary>
        /// Import Queue
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuImportClick(object sender, EventArgs e)
        {
            OpenFile.FileName = string.Empty;
            OpenFile.ShowDialog();
            if (OpenFile.FileName != String.Empty)
                queue.LoadQueueFromFile(OpenFile.FileName);
            UpdateUiElements();
        }

        /// <summary>
        /// Readd current job to queue
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuReaddClick(object sender, EventArgs e)
        {
            if (!queue.LastEncode.IsEmpty)
            {
                queue.Add(
                    queue.LastEncode.Query, 
                    queue.LastEncode.Title, 
                    queue.LastEncode.Source,
                    queue.LastEncode.Destination,
                    queue.LastEncode.CustomQuery);
                UpdateUiElements();
            }
        }

        /// <summary>
        /// Hide's the window when the user tries to "x" out of the window instead of closing it.
        /// </summary>
        /// <param name="e">
        /// The e.
        /// </param>
        protected override void OnClosing(CancelEventArgs e)
        {
            e.Cancel = true;
            this.Hide();
            base.OnClosing(e);
        }
    }
}