﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodeJob.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodeJob type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
	using System;
	using System.Collections.Generic;
	using System.Xml.Serialization;

	using HandBrake.Interop.Model.Encoding;

	public class EncodeJob
	{
		public SourceType SourceType { get; set; }
		public string SourcePath { get; set; }

		/// <summary>
		/// Gets or sets the 1-based index of the title to encode.
		/// </summary>
		public int Title { get; set; }

		/// <summary>
		/// Gets or sets the angle to encode. 0 for default, 1+ for specified angle.
		/// </summary>
		public int Angle { get; set; }

		public VideoRangeType RangeType { get; set; }
		public int ChapterStart { get; set; }
		public int ChapterEnd { get; set; }

		public double SecondsStart { get; set; }
		public double SecondsEnd { get; set; }

		public int FramesStart { get; set; }
		public int FramesEnd { get; set; }

		/// <summary>
		/// Gets or sets the list of chosen audio tracks (1-based)
		/// </summary>
		public List<int> ChosenAudioTracks { get; set; }
		public Subtitles Subtitles { get; set; }
		public bool UseDefaultChapterNames { get; set; }
		public List<string> CustomChapterNames { get; set; }

		public string OutputPath { get; set; }

		public EncodingProfile EncodingProfile { get; set; }

		// The length of video to encode.
		[XmlIgnore]
		public TimeSpan Length { get; set; }

		[XmlElement("Length")]
		public string XmlLength
		{
			get { return this.Length.ToString(); }
			set { this.Length = TimeSpan.Parse(value); }
		}

		public EncodeJob Clone()
		{
			EncodeJob clone = new EncodeJob
			{
				SourceType = this.SourceType,
				SourcePath = this.SourcePath,
				Title = this.Title,
				Angle = this.Angle,
				RangeType = this.RangeType,
				ChapterStart = this.ChapterStart,
				ChapterEnd = this.ChapterEnd,
				SecondsStart = this.SecondsStart,
				SecondsEnd = this.SecondsEnd,
				FramesStart = this.FramesStart,
				FramesEnd = this.FramesEnd,
				ChosenAudioTracks = new List<int>(this.ChosenAudioTracks),
				Subtitles = this.Subtitles,
				UseDefaultChapterNames = this.UseDefaultChapterNames,
				OutputPath = this.OutputPath,
				EncodingProfile = this.EncodingProfile,
				Length = this.Length
			};

			return clone;
		}
	}
}
