﻿/*  Scan.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Text;
    using System.Threading;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;

    /// <summary>
    /// Scan a Source
    /// </summary>
    public class ScanService : IScan
    {
        #region Private Variables

        /// <summary>
        /// A Lock object
        /// </summary>
        private static readonly object locker = new object();

        /// <summary>
        /// The CLI data parser
        /// </summary>
        private Parser readData;

        /// <summary>
        /// The Log Buffer
        /// </summary>
        private StringBuilder logBuffer;

        /// <summary>
        /// The line number thats been read to in the log file
        /// </summary>
        private int logFilePosition;

        /// <summary>
        /// The Process belonging to the CLI
        /// </summary>
        private Process hbProc;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="ScanService"/> class.
        /// </summary>
        public ScanService()
        {
        }

        #region Events

        /// <summary>
        /// Scan has Started
        /// </summary>
        public event EventHandler ScanStared;

        /// <summary>
        /// Scan has completed
        /// </summary>
        public event ScanCompletedStatus ScanCompleted;

        /// <summary>
        /// Encode process has progressed
        /// </summary>
        public event ScanProgessStatus ScanStatusChanged;

        #endregion

        #region Public Properties

        /// <summary>
        /// Gets a value indicating whether IsScanning.
        /// </summary>
        public bool IsScanning { get; private set; }

        /// <summary>
        /// Gets the Souce Data.
        /// </summary>
        public DVD SouceData { get; private set; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                if (IsScanning)
                    return readData.Buffer.ToString();

                if (logBuffer == null)
                {
                    ResetLogReader(false);
                    ReadLastScanFile();
                }

                return logBuffer != null ? logBuffer.ToString() : string.Empty;
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Scan a Source Path.
        /// Title 0: scan all
        /// </summary>
        /// <param name="sourcePath">Path to the file to scan</param>
        /// <param name="title">int title number. 0 for scan all</param>
        public void Scan(string sourcePath, int title)
        {
            Thread t = new Thread(unused => this.ScanSource(sourcePath, title));
            t.Start();
        }

        /// <summary>
        /// Kill the scan
        /// </summary>
        public void Stop()
        {
            try
            {
                // Try to clean things up as best as possible.
                if (this.readData != null)
                {
                    this.readData.OnScanProgress -= this.OnScanProgress;
                }

                if (hbProc != null && !hbProc.HasExited)
                    hbProc.Kill();
            }
            catch
            {
                // We don't really need to notify the user of any errors here.
            }
        }
        #endregion

        #region Private Methods

        /// <summary>
        /// Start a scan for a given source path and title
        /// </summary>
        /// <param name="sourcePath">Path to the source file</param>
        /// <param name="title">the title number to look at</param>
        private void ScanSource(object sourcePath, int title)
        {
            try
            {
                IsScanning = true;
                if (this.ScanStared != null)
                {
                    this.ScanStared(this, new EventArgs());
                }

                ResetLogReader(true);

                string handbrakeCLIPath = Path.Combine(Application.StartupPath, "HandBrakeCLI.exe");
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string dvdInfoPath = Path.Combine(
                    logDir,
                    string.Format("last_scan_log{0}.txt", Init.InstanceId == 0 ? string.Empty : Init.InstanceId.ToString()));

                // Make we don't pick up a stale last_encode_log.txt (and that we have rights to the file)
                if (File.Exists(dvdInfoPath))
                {
                    File.Delete(dvdInfoPath);
                }

                string extraArguments = string.Empty;
                if (Init.DisableDvdNav)
                {
                    extraArguments = " --no-dvdnav";
                }

                if (title > 0)
                {
                    extraArguments += " --scan ";
                }

                // Quick fix for "F:\\" style paths. Just get rid of the \\ so the CLI doesn't fall over.
                // Sould probably clean up the escaping of the strings later.
                string source = sourcePath.ToString().EndsWith("\\") ? sourcePath.ToString() : "\"" + sourcePath + "\"";

                this.hbProc = new Process
                    {
                        StartInfo =
                            {
                                FileName = handbrakeCLIPath,
                                Arguments = String.Format(@" -i ""{0}"" -t{1} {2} -v ", sourcePath, title, extraArguments),
                                RedirectStandardOutput = true,
                                RedirectStandardError = true,
                                UseShellExecute = false,
                                CreateNoWindow = true
                            }
                    };

                string command = String.Format(@" -i {0} -t{1} {2} -v ", source, title, extraArguments);

                this.hbProc = new Process
                    {
                        StartInfo =
                            {
                                FileName = handbrakeCLIPath,
                                Arguments = command,
                                RedirectStandardOutput = true,
                                RedirectStandardError = true,
                                UseShellExecute = false,
                                CreateNoWindow = true
                            }
                    };

                // Start the Scan
                this.hbProc.Start();

                this.readData = new Parser(this.hbProc.StandardError.BaseStream);
                this.readData.OnScanProgress += this.OnScanProgress;
                this.SouceData = DVD.Parse(this.readData);

                // Write the Buffer out to file.
                using (StreamWriter scanLog = new StreamWriter(dvdInfoPath))
                {
                    // Only write the log file to disk if it's less than 100MB.
                    if (this.readData.Buffer.Length < 100000000)
                    {
                        scanLog.WriteLine(UtilityService.CreateCliLogHeader(null));
                        scanLog.Write(this.readData.Buffer);
                        logBuffer.AppendLine(this.readData.Buffer.ToString());
                    }
                    else
                    {
                        throw new Exception(
                            "The Log file has not been written to disk as it has grown above the 100MB limit. This indicates there was a problem during the scan process.");
                    }
                }

                IsScanning = false;

                if (this.ScanCompleted != null)
                {
                    this.ScanCompleted(this, new ScanCompletedEventArgs(true, null, string.Empty));
                }
            }
            catch (Exception exc)
            {
                this.Stop();

                if (this.ScanCompleted != null)
                    this.ScanCompleted(this, new ScanCompletedEventArgs(false, exc, "An Error has occured in ScanService.ScanSource()"));
            }
        }

        /// <summary>
        /// Read the log file
        /// </summary>
        private void ReadLastScanFile()
        {
            lock (locker)
            {
                // last_encode_log.txt is the primary log file. Since .NET can't read this file whilst the CLI is outputing to it (Not even in read only mode),
                // we'll need to make a copy of it.
                string logDir = Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData) +
                                "\\HandBrake\\logs";
                string logFile = Path.Combine(logDir, string.Format("last_scan_log{0}.txt", Init.InstanceId == 0 ? string.Empty : Init.InstanceId.ToString()));
                string logFile2 = Path.Combine(logDir, string.Format("tmp_appReadable_log{0}.txt", Init.InstanceId == 0 ? string.Empty : Init.InstanceId.ToString()));

                try
                {
                    // Make sure the application readable log file does not already exist. FileCopy fill fail if it does.
                    if (File.Exists(logFile2))
                        File.Delete(logFile2);

                    // Copy the log file.
                    if (File.Exists(logFile))
                        File.Copy(logFile, logFile2, true);
                    else
                    {
                        ResetLogReader(true);
                        return;
                    }

                    // Start the Reader
                    // Only use text which continues on from the last read line
                    StreamReader sr = new StreamReader(logFile2);
                    string line;
                    int i = 1;
                    while ((line = sr.ReadLine()) != null)
                    {
                        if (i > logFilePosition)
                        {
                            logBuffer.AppendLine(line);
                            logFilePosition++;
                        }
                        i++;
                    }
                    sr.Close();
                    sr.Dispose();
                }
                catch (Exception exc)
                {
                    ResetLogReader(true);
                }
            }
        }

        /// <summary>
        /// Reset the Log Reader
        /// </summary>
        /// <param name="addHeader">
        /// The add Header.
        /// </param>
        private void ResetLogReader(bool addHeader)
        {
            logFilePosition = 0;
            logBuffer = new StringBuilder();
            if (addHeader)
                logBuffer.AppendLine(UtilityService.CreateCliLogHeader(null));
        }

        /// <summary>
        /// Fire an event when the scan process progresses
        /// </summary>
        /// <param name="sender">the sender</param>
        /// <param name="currentTitle">the current title being scanned</param>
        /// <param name="titleCount">the total number of titles</param>
        private void OnScanProgress(object sender, int currentTitle, int titleCount)
        {
            ScanProgressEventArgs eventArgs = new ScanProgressEventArgs
            {
                CurrentTitle = currentTitle,
                Titles = titleCount
            };

            if (this.ScanStatusChanged != null)
            {
                this.ScanStatusChanged(this, eventArgs);
            }
        }

        #endregion
    }
}