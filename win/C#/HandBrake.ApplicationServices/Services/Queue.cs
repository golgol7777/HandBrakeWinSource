﻿/*  Queue.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr/>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics;
    using System.IO;
    using System.Linq;
    using System.Threading;
    using System.Windows.Forms;
    using System.Xml.Serialization;

    using HandBrake.ApplicationServices.Functions;
    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// The HandBrake Queue
    /// </summary>
    public class Queue : Encode, IQueue
    {
        /// <summary>
        /// The Queue Job List
        /// </summary>
        private readonly List<QueueTask> queue = new List<QueueTask>();

        /// <summary>
        /// An XML Serializer
        /// </summary>
        private static XmlSerializer serializer;

        /// <summary>
        /// The Next Job ID
        /// </summary>
        private int nextJobId;

        #region Events
        /// <summary>
        /// Fires when the Queue has started
        /// </summary>
        public event EventHandler QueueStarted;

        /// <summary>
        /// Fires when a job is Added, Removed or Re-Ordered.
        /// Should be used for triggering an update of the Queue Window.
        /// </summary>
        public event EventHandler QueueListChanged;

        /// <summary>
        /// Fires when a pause to the encode queue has been requested.
        /// </summary>
        public event EventHandler QueuePauseRequested;

        /// <summary>
        /// Fires when the entire encode queue has completed.
        /// </summary>
        public event EventHandler QueueCompleted;
        #endregion

        #region Properties
        /// <summary>
        /// Gets or sets the last encode that was processed.
        /// </summary>
        /// <returns></returns> 
        public QueueTask LastEncode { get; set; }

        /// <summary>
        /// Gets a value indicating whether Request Pause
        /// </summary>
        public bool Paused { get; private set; }

        /// <summary>
        /// Gets the current state of the encode queue.
        /// </summary>
        public ReadOnlyCollection<QueueTask> CurrentQueue
        {
            get { return this.queue.AsReadOnly(); }
        }

        /// <summary>
        /// Gets the number of items in the queue.
        /// </summary>
        public int Count
        {
            get { return this.queue.Count; }
        }
        #endregion

        #region Queue

        /// <summary>
        /// Gets and removes the next job in the queue.
        /// </summary>
        /// <returns>The job that was removed from the queue.</returns>
        private QueueTask GetNextJob()
        {
            QueueTask job = this.queue[0];
            this.LastEncode = job;
            this.Remove(0); // Remove the item which we are about to pass out.

            this.SaveQueue();

            return job;
        }

        /// <summary>
        /// Adds an item to the queue.
        /// </summary>
        /// <param name="query">
        /// The query that will be passed to the HandBrake CLI.
        /// </param>
        /// <param name="title">
        /// The title.
        /// </param>
        /// <param name="source">
        /// The location of the source video.
        /// </param>
        /// <param name="destination">
        /// The location where the encoded video will be.
        /// </param>
        /// <param name="customJob">
        /// Custom job
        /// </param>
        public void Add(string query, int title, string source, string destination, bool customJob)
        {
            QueueTask newJob = new QueueTask
                             {
                                 Id = this.nextJobId++,
                                 Title = title,
                                 Query = query,
                                 Source = source,
                                 Destination = destination,
                                 CustomQuery = customJob
                             };

            this.queue.Add(newJob);
            this.SaveQueue();

            if (this.QueueListChanged != null)
                this.QueueListChanged(this, new EventArgs());
        }

        /// <summary>
        /// Removes an item from the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        public void Remove(int index)
        {
            this.queue.RemoveAt(index);
            this.SaveQueue();

            if (this.QueueListChanged != null)
                this.QueueListChanged(this, new EventArgs());
        }

        /// <summary>
        /// Retrieve a job from the queue
        /// </summary>
        /// <param name="index">the job id</param>
        /// <returns>A job for the given index or blank job object</returns>
        public QueueTask GetJob(int index)
        {
            if (this.queue.Count >= (index + 1))
                return this.queue[index];

            return new QueueTask();
        }

        /// <summary>
        /// Moves an item up one position in the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        public void MoveUp(int index)
        {
            if (index > 0)
            {
                QueueTask item = queue[index];

                queue.RemoveAt(index);
                queue.Insert((index - 1), item);
            }

            this.SaveQueue(); // Update the queue recovery file

            if (this.QueueListChanged != null)
                this.QueueListChanged(this, new EventArgs());
        }

        /// <summary>
        /// Moves an item down one position in the queue.
        /// </summary>
        /// <param name="index">The zero-based location of the job in the queue.</param>
        public void MoveDown(int index)
        {
            if (index < this.queue.Count - 1)
            {
                QueueTask item = this.queue[index];

                this.queue.RemoveAt(index);
                this.queue.Insert((index + 1), item);
            }

            this.SaveQueue(); // Update the queue recovery file

            if (this.QueueListChanged != null)
                this.QueueListChanged(this, new EventArgs());
        }
        
        /// <summary>
        /// Save any updates to the main queue file.
        /// </summary>
        public void SaveQueue()
        {
            string file = Init.InstanceId == 0
                              ? "hb_queue_recovery.xml"
                              : string.Format("hb_queue_recovery{0}.xml", Init.InstanceId);
            this.WriteQueueStateToFile(file);
        }

        /// <summary>
        /// Writes the current state of the queue to a file.
        /// </summary>
        /// <param name="file">The location of the file to write the queue to.</param>
        public void WriteQueueStateToFile(string file)
        {
            string appDataPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");
            string tempPath = file.Contains("hb_queue_recovery") ? appDataPath + file : file;

            try
            {
                using (FileStream strm = new FileStream(tempPath, FileMode.Create, FileAccess.Write))
                {
                    if (serializer == null)
                        serializer = new XmlSerializer(typeof(List<QueueTask>));
                    serializer.Serialize(strm, queue);
                    strm.Close();
                    strm.Dispose();
                }
            }
            catch (Exception)
            {
                return;
            }
        }

        /// <summary>
        /// Writes the current state of the queue in the form of a batch (.bat) file.
        /// </summary>
        /// <param name="file">
        /// The location of the file to write the batch file to.
        /// </param>
        /// <returns>
        /// The write batch script to file.
        /// </returns>
        public bool WriteBatchScriptToFile(string file)
        {
            string queries = string.Empty;
            foreach (QueueTask queueItem in this.queue)
            {
                string qItem = queueItem.Query;
                string fullQuery = '"' + Application.StartupPath + "\\HandBrakeCLI.exe" + '"' + qItem;

                if (queries == string.Empty)
                    queries = queries + fullQuery;
                else
                    queries = queries + " && " + fullQuery;
            }
            string strCmdLine = queries;

            if (file != string.Empty)
            {
                try
                {
                    // Create a StreamWriter and open the file, Write the batch file query to the file and 
                    // Close the stream
                    using (StreamWriter line = new StreamWriter(file))
                    {
                        line.WriteLine(strCmdLine);
                    }

                    return true;
                }
                catch (Exception exc)
                {
                    throw new Exception("Unable to write to the file. Please make sure that the location has the correct permissions for file writing.", exc);
                }
            }
            return false;
        }

        /// <summary>
        /// Reads a serialized XML file that represents a queue of encoding jobs.
        /// </summary>
        /// <param name="file">The location of the file to read the queue from.</param>
        public void LoadQueueFromFile(string file)
        {
            string appDataPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\");
            string tempPath = file.Contains("hb_queue_recovery") ? appDataPath + file : file;

            if (File.Exists(tempPath))
            {
                using (FileStream strm = new FileStream(tempPath, FileMode.Open, FileAccess.Read))
                {
                    if (strm.Length != 0)
                    {
                        if (serializer == null)
                            serializer = new XmlSerializer(typeof(List<QueueTask>));

                        List<QueueTask> list = serializer.Deserialize(strm) as List<QueueTask>;

                        if (list != null)
                            foreach (QueueTask item in list)
                                this.queue.Add(item);

                        if (!file.Contains("hb_queue_recovery"))
                            this.SaveQueue();

                        if (this.QueueListChanged != null)
                            this.QueueListChanged(this, new EventArgs());
                    }
                }
            }
        }

        /// <summary>
        /// Checks the current queue for an existing instance of the specified destination.
        /// </summary>
        /// <param name="destination">The destination of the encode.</param>
        /// <returns>Whether or not the supplied destination is already in the queue.</returns>
        public bool CheckForDestinationDuplicate(string destination)
        {
            return this.queue.Any(checkItem => checkItem.Destination.Contains(destination.Replace("\\\\", "\\")));
        }

        #endregion

        #region Encoding

        /// <summary>
        /// Starts encoding the first job in the queue and continues encoding until all jobs
        /// have been encoded.
        /// </summary>
        public void Start()
        {
            if (this.Count != 0)
            {
                if (this.Paused)
                    this.Paused = false;
                else
                {
                    this.Paused = false;
                    try
                    {
                        Thread theQueue = new Thread(this.StartQueue) { IsBackground = true };
                        theQueue.Start();

                        if (this.QueueStarted != null)
                            this.QueueStarted(this, new EventArgs());
                    }
                    catch (Exception exc)
                    {
                        throw new Exception("Unable to Start Queue", exc);
                    }
                }
            }
        }

        /// <summary>
        /// Requests a pause of the encode queue.
        /// </summary>
        public void Pause()
        {
            this.Paused = true;

            if (this.QueuePauseRequested != null)
                this.QueuePauseRequested(this, new EventArgs());
        }

        /// <summary>
        /// Run through all the jobs on the queue.
        /// </summary>
        /// <param name="state">Object State</param>
        private void StartQueue(object state)
        {
            // Run through each item on the queue
            while (this.Count != 0)
            {
                QueueTask encJob = this.GetNextJob();
                this.SaveQueue(); // Update the queue recovery file

                Start(encJob, true);

                if (HbProcess == null)
                {
                    return;
                }
                HbProcess.WaitForExit();

                this.CopyLog(this.LastEncode.Destination);

                HbProcess.Close();
                HbProcess.Dispose();

                // Growl
                if (Init.GrowlEncode)
                    GrowlCommunicator.Notify("Encode Completed",
                                             "Put down that cocktail...\nyour Handbrake encode is done.");

                while (this.Paused) // Need to find a better way of doing this.
                {
                    Thread.Sleep(2000);
                }
            }
            this.LastEncode = new QueueTask();

            if (this.QueueCompleted != null)
                this.QueueCompleted(this, new EventArgs());

            // After the encode is done, we may want to shutdown, suspend etc.
            Finish();
        }

        /// <summary>
        /// Perform an action after an encode. e.g a shutdown, standby, restart etc.
        /// </summary>
        private void Finish()
        {
            // Growl
            if (Init.GrowlQueue)
                GrowlCommunicator.Notify("Queue Completed", "Put down that cocktail...\nyour Handbrake queue is done.");

            // Do something whent he encode ends.
            switch (Init.CompletionOption)
            {
                case "Shutdown":
                    Process.Start("Shutdown", "-s -t 60");
                    break;
                case "Log Off":
                    Win32.ExitWindowsEx(0, 0);
                    break;
                case "Suspend":
                    Application.SetSuspendState(PowerState.Suspend, true, true);
                    break;
                case "Hibernate":
                    Application.SetSuspendState(PowerState.Hibernate, true, true);
                    break;
                case "Lock System":
                    Win32.LockWorkStation();
                    break;
                case "Quit HandBrake":
                    Application.Exit();
                    break;
                default:
                    break;
            }
        }

        #endregion
    }
}