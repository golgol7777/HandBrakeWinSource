﻿/*  LibScan.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace HandBrake.ApplicationServices.Services
{
    using System;
    using System.Collections.Generic;
    using System.Text;
    using System.Threading;

    using HandBrake.ApplicationServices.EventArgs;
    using HandBrake.ApplicationServices.Model.Encoding;
    using HandBrake.ApplicationServices.Parsing;
    using HandBrake.ApplicationServices.Services.Interfaces;
    using HandBrake.Interop;
    using HandBrake.Interop.Model;

    using AudioTrack = HandBrake.ApplicationServices.Parsing.Audio;
    using ScanProgressEventArgs = HandBrake.Interop.ScanProgressEventArgs;
    using Size = System.Drawing.Size;

    /// <summary>
    /// Scan a Source
    /// </summary>
    public class LibScan : IScan
    {
        /*
         * TODO
         * 1. Expose the Previews code.
         * 2. Expose the Logging.
         * 
         */

        #region Private Variables

        /// <summary>
        /// Lock for the log file
        /// </summary>
        static readonly object LogLock = new object();

        /// <summary>
        /// LibHB Instance
        /// </summary>
        private readonly HandBrakeInstance instance;

        /// <summary>
        /// Log data from HandBrakeInstance
        /// </summary>
        private readonly StringBuilder logging;

        #endregion

        /// <summary>
        /// Initializes a new instance of the <see cref="LibScan"/> class. 
        /// </summary>
        public LibScan()
        {
            logging = new StringBuilder();

            instance = new HandBrakeInstance();
            instance.Initialize(1);
            instance.ScanProgress += this.InstanceScanProgress;
            instance.ScanCompleted += this.InstanceScanCompleted;

            HandBrakeUtils.MessageLogged += this.HandBrakeInstanceMessageLogged;
            HandBrakeUtils.ErrorLogged += this.HandBrakeInstanceErrorLogged;
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

        #region Properties

        /// <summary>
        /// Gets a value indicating whether IsScanning.
        /// </summary>
        public bool IsScanning { get; private set; }

        /// <summary>
        /// Gets the Souce Data.
        /// </summary>
        public Source SouceData { get; private set; }

        /// <summary>
        /// Gets ActivityLog.
        /// </summary>
        public string ActivityLog
        {
            get
            {
                return logging.ToString();
            }
        }

        #endregion

        #region Public Methods

        /// <summary>
        /// Scan a Source Path.
        /// Title 0: scan all
        /// </summary>
        /// <param name="sourcePath">
        /// Path to the file to scan
        /// </param>
        /// <param name="title">
        /// int title number. 0 for scan all
        /// </param>
        /// <param name="previewCount">
        /// The preview Count.
        /// </param>
        public void Scan(string sourcePath, int title, int previewCount)
        {
            Thread t = new Thread(unused => this.ScanSource(sourcePath, title, previewCount));
            t.Start();
        }

        /// <summary>
        /// Kill the scan
        /// </summary>
        public void Stop()
        {
            instance.StopScan();
        }

        #endregion

        #region Private Methods

        /// <summary>
        /// Start a scan for a given source path and title
        /// </summary>
        /// <param name="sourcePath">
        /// Path to the source file
        /// </param>
        /// <param name="title">
        /// the title number to look at
        /// </param>
        /// <param name="previewCount">
        /// The preview Count.
        /// </param>
        private void ScanSource(object sourcePath, int title, int previewCount)
        {
            try
            {
                // TODO Support Preview Count

                IsScanning = true;
                if (this.ScanStared != null)
                    this.ScanStared(this, new EventArgs());

                if (title != 0)
                    instance.StartScan(sourcePath.ToString(), 10, title);
                else
                    instance.StartScan(sourcePath.ToString(), 10);
            }
            catch (Exception exc)
            {
                this.Stop();

                if (this.ScanCompleted != null)
                    this.ScanCompleted(this, new ScanCompletedEventArgs(false, exc, "An Error has occured in ScanService.ScanSource()"));
            }
        }

        #endregion

        #region HandBrakeInstance Event Handlers
        /// <summary>
        /// Scan Completed Event Handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void InstanceScanCompleted(object sender, EventArgs e)
        {
            this.SouceData = new Source { Titles = ConvertTitles(this.instance.Titles) };

            IsScanning = false;

            if (this.ScanCompleted != null)
                this.ScanCompleted(this, new ScanCompletedEventArgs(true, null, string.Empty));
        }

        /// <summary>
        /// Scan Progress Event Handler
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The EventArgs.
        /// </param>
        private void InstanceScanProgress(object sender, ScanProgressEventArgs e)
        {
            if (this.ScanStatusChanged != null)
            {
                ApplicationServices.EventArgs.ScanProgressEventArgs eventArgs =
                    new ApplicationServices.EventArgs.ScanProgressEventArgs
                        {
                            CurrentTitle = e.CurrentTitle,
                            Titles = e.Titles
                        };

                this.ScanStatusChanged(this, eventArgs);
            }
        }

        /// <summary>
        /// Log a message
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The MessageLoggedEventArgs.
        /// </param>
        private void HandBrakeInstanceErrorLogged(object sender, MessageLoggedEventArgs e)
        {
            lock (LogLock)
            {
                this.logging.AppendLine(e.Message);
            }
        }

        /// <summary>
        /// Log a message
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The MessageLoggedEventArgs.
        /// </param>
        private void HandBrakeInstanceMessageLogged(object sender, MessageLoggedEventArgs e)
        {
            lock (LogLock)
            {
                this.logging.AppendLine(e.Message);
            }
        }

        /// <summary>
        /// Convert Interop Title objects to App Services Title object
        /// </summary>
        /// <param name="titles">
        /// The titles.
        /// </param>
        /// <returns>
        /// The convert titles.
        /// </returns>
        private static List<Title> ConvertTitles(IEnumerable<Interop.SourceData.Title> titles)
        {
            List<Title> titleList = new List<Title>();
            foreach (Interop.SourceData.Title title in titles)
            {
                Title converted = new Title
                    {
                        TitleNumber = title.TitleNumber, 
                        Duration = title.Duration, 
                        Resolution = new Size(title.Resolution.Width, title.Resolution.Height), 
                        AspectRatio = title.AspectRatio, 
                        AngleCount = title.AngleCount, 
                        ParVal = new Size(title.ParVal.Width, title.ParVal.Height), 
                        AutoCropDimensions = title.AutoCropDimensions,
                        Fps = title.Framerate
                    };

                foreach (Interop.SourceData.Chapter chapter in title.Chapters)
                {
                    converted.Chapters.Add(new Chapter(chapter.ChapterNumber, string.Empty, chapter.Duration));
                }

                foreach (Interop.SourceData.AudioTrack track in title.AudioTracks)
                {
                    converted.AudioTracks.Add(new AudioTrack(track.TrackNumber, track.Language, track.LanguageCode, track.Description, string.Empty, track.SampleRate, track.Bitrate));
                }

                foreach (Interop.SourceData.Subtitle track in title.Subtitles)
                {
                    SubtitleType convertedType = new SubtitleType();

                    switch (track.SubtitleSource)
                    {
                        case Interop.SourceData.SubtitleSource.VobSub:
                            convertedType = SubtitleType.VobSub;
                            break;
                        case Interop.SourceData.SubtitleSource.UTF8:
                            convertedType = SubtitleType.UTF8Sub;
                            break;
                        case Interop.SourceData.SubtitleSource.TX3G:
                            convertedType = SubtitleType.TX3G;
                            break;
                        case Interop.SourceData.SubtitleSource.SSA:
                            convertedType = SubtitleType.SSA;
                            break;
                        case Interop.SourceData.SubtitleSource.SRT:
                            convertedType = SubtitleType.SRT;
                            break;
                        case Interop.SourceData.SubtitleSource.CC608:
                            convertedType = SubtitleType.CC;
                            break;
                        case Interop.SourceData.SubtitleSource.CC708:
                            convertedType = SubtitleType.CC;
                            break;
                    }

                    converted.Subtitles.Add(new Subtitle(track.TrackNumber, track.Language, track.LanguageCode, convertedType));
                }

                titleList.Add(converted);
            }

            return titleList;
        }
        #endregion
    }
}