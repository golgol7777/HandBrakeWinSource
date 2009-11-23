/*  Parser.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System.IO;
using System.Text.RegularExpressions;
using System;
using System.Globalization;

namespace Handbrake.Parsing
{
    /// <summary>
    /// A delegate to handle custom events regarding data being parsed from the buffer
    /// </summary>
    /// <param name="Sender">The object which raised this delegate</param>
    /// <param name="Data">The data parsed from the stream</param>
    public delegate void DataReadEventHandler(object Sender, string Data);

    /// <summary>
    /// A delegate to handle events regarding progress during DVD scanning
    /// </summary>
    /// <param name="Sender">The object who's raising the event</param>
    /// <param name="CurrentTitle">The title number currently being processed</param>
    /// <param name="TitleCount">The total number of titiles to be processed</param>
    public delegate void ScanProgressEventHandler(object Sender, int CurrentTitle, int TitleCount);

    /// <summary>
    /// A delegate to handle encode progress updates // EXPERIMENTAL
    /// </summary>
    /// <param name="Sender">The object which raised the event</param>
    /// <param name="CurrentTask">The current task being processed from the queue</param>
    /// <param name="TaskCount">The total number of tasks in queue</param>
    /// <param name="PercentComplete">The percentage this task is complete</param>
    /// <param name="CurrentFps">The current encoding fps</param>
    /// <param name="AverageFps">The average encoding fps for this task</param>
    /// <param name="TimeRemaining">The estimated time remaining for this task to complete</param>
    public delegate void EncodeProgressEventHandler(object Sender, int CurrentTask, int TaskCount, float PercentComplete, float CurrentFps, float AverageFps, TimeSpan TimeRemaining);

       
    /// <summary>
    /// A simple wrapper around a StreamReader to keep track of the entire output from a cli process
    /// </summary>
    internal class Parser : StreamReader
    {
        private string m_buffer;
        /// <summary>
        /// The output from the CLI process
        /// </summary>
        public string Buffer
        {
            get
            {
                return m_buffer;
            }
        }

        /// <summary>
        /// Raised upon a new line being read from stdout/stderr
        /// </summary>
        public event DataReadEventHandler OnReadLine;

        /// <summary>
        /// Raised upon the entire stdout/stderr stream being read in a single call
        /// </summary>
        public event DataReadEventHandler OnReadToEnd;

        /// <summary>
        /// Raised upon the catching of a "Scanning title # of #..." in the stream
        /// </summary>
        public event ScanProgressEventHandler OnScanProgress;

        #region Experimetnal Code
        /// <summary>
        /// Raised upon the catching of a "Scanning title # of #..." in the stream
        /// </summary>
        public event EncodeProgressEventHandler OnEncodeProgress;
        #endregion

        /// <summary>
        /// Default constructor for this object
        /// </summary>
        /// <param name="baseStream">The stream to parse from</param>
        public Parser(Stream baseStream) : base(baseStream)
        {
            m_buffer = string.Empty;
        }

        public override string ReadLine()
        {
            string tmp = base.ReadLine();

            m_buffer += tmp;
            Match m = Regex.Match(tmp, "^Scanning title ([0-9]*) of ([0-9]*)");
            if (OnReadLine != null)
                OnReadLine(this, tmp);

            if (m.Success && OnScanProgress != null)
                OnScanProgress(this, int.Parse(m.Groups[1].Value), int.Parse(m.Groups[2].Value));

            return tmp;
        }

        public override string ReadToEnd()
        {
            string tmp = base.ReadToEnd();

            m_buffer += tmp;
            if (OnReadToEnd != null)
                OnReadToEnd(this, tmp);

            return tmp;
        }

        /// <summary>
        /// Pase the CLI status output (from standard output)
        /// </summary>
        public void readEncodeStatus()
        {
            CultureInfo culture = CultureInfo.CreateSpecificCulture("en-US");
            string tmp = base.ReadLine();

            Match m = Regex.Match(tmp, @"^Encoding: task ([0-9]*) of ([0-9]*), ([0-9]*\.[0-9]*) %( \(([0-9]*\.[0-9]*) fps, avg ([0-9]*\.[0-9]*) fps, ETA ([0-9]{2})h([0-9]{2})m([0-9]{2})s\))?");
            if (m.Success && OnEncodeProgress != null)
            {
                int currentTask = int.Parse(m.Groups[1].Value);
                int totalTasks = int.Parse(m.Groups[2].Value);
                float percent = float.Parse(m.Groups[3].Value, culture);
                float currentFps = m.Groups[5].Value == string.Empty ? 0.0F : float.Parse(m.Groups[5].Value, culture);
                float avgFps = m.Groups[6].Value == string.Empty ? 0.0F : float.Parse(m.Groups[6].Value, culture);
                TimeSpan remaining = TimeSpan.Zero;
                if (m.Groups[7].Value != string.Empty)
                {
                    remaining = TimeSpan.Parse(m.Groups[7].Value + ":" + m.Groups[8].Value + ":" + m.Groups[9].Value);
                }
                OnEncodeProgress(this, currentTask, totalTasks, percent, currentFps, avgFps, remaining);
            }
        }
    }
}
