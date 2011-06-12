﻿using System;
using System.Runtime.InteropServices;

namespace HandBrake.Interop
{
	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
	public struct hb_title_s
	{
		/// Anonymous_990d28ea_6cf3_4fbc_8143_4df9513e9550
		public hb_title_type_anon type;

		public uint reg_desc;

		/// char[1024]
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
		public string dvd;

		/// char[1024]
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 1024)]
		public string name;

		/// int
		public int index;

		/// int
		public int vts;

		/// int
		public int ttn;

		/// int
		public int cell_start;

		/// int
		public int cell_end;

		/// int
		public ulong block_start;

		/// int
		public ulong block_end;

		/// int
		public ulong block_count;

		/// int
		public int angle_count;

		/// int
		public int hours;

		/// int
		public int minutes;

		/// int
		public int seconds;

		/// uint64_t->unsigned int
		public ulong duration;

		/// double
		public double aspect;

		/// double
		public double container_aspect;

		/// int
		public int width;

		/// int
		public int height;

		/// int
		public int pixel_aspect_width;

		/// int
		public int pixel_aspect_height;

		/// int
		public int rate;

		/// int
		public int rate_base;

		/// int[4]
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 4, ArraySubType = UnmanagedType.I4)]
		public int[] crop;

		//public fixed int crop[4];

		/// Anonymous_618ebeca_0ad9_4a71_9a49_18e50ac2e9db
		public Anonymous_618ebeca_0ad9_4a71_9a49_18e50ac2e9db demuxer;

		/// int
		public int detected_interlacing;

		public int pcr_pid;

		/// int
		public int video_id;

		/// int
		public int video_codec;

		public uint video_stream_type;

		/// int
		public int video_codec_param;

		/// char*
		[MarshalAs(UnmanagedType.LPStr)]
		public string video_codec_name;

		/// int
		public int video_bitrate;

		/// char*
		[MarshalAs(UnmanagedType.LPStr)]
		public string container_name;

		/// int
		public int data_rate;

		/// hb_metadata_t*
		public IntPtr metadata;

		/// hb_list_t*
		public IntPtr list_chapter;

		/// hb_list_t*
		public IntPtr list_audio;

		/// hb_list_t*
		public IntPtr list_subtitle;

		/// hb_list_t*
		public IntPtr list_attachment;

		/// hb_job_t*
		public IntPtr job;

		/// uint32_t->unsigned int
		public uint flags;
	}
}
