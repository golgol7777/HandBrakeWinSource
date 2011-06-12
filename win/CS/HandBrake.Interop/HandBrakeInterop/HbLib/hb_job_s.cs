﻿using System;
using System.Runtime.InteropServices;

namespace HandBrake.Interop
{
	[StructLayout(LayoutKind.Sequential)]
	public struct hb_job_s
	{
		/// int
		public int sequence_id;

		/// hb_title_t*
		public IntPtr title;

		public int feature;

		/// int
		public int chapter_start;

		/// int
		public int chapter_end;

		/// int
		public int chapter_markers;

		/// int[4]
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = 4, ArraySubType = UnmanagedType.I4)]
		public int[] crop;

		/// int
		public int deinterlace;

		/// hb_list_t*
		public IntPtr filters;

		/// int
		public int width;

		/// int
		public int height;

		/// int
		public int keep_ratio;

		/// int
		public int grayscale;

		public hb_anamorphic_substruct anamorphic;

		public int modulus;

		/// int
		public int maxWidth;

		/// int
		public int maxHeight;

		/// int
		public int vcodec;

		/// float
		public float vquality;

		/// int
		public int vbitrate;

		public int pfr_vrate;

		public int pfr_vrate_base;

		/// int
		public int vrate;

		/// int
		public int vrate_base;

		/// int
		public int cfr;

		/// int
		public int pass;

		/// char*
		//[MarshalAs(UnmanagedType.LPStr)]
		//public string x264opts;

		public IntPtr advanced_opts;

		/// int
		public int areBframes;

		/// int
		public int color_matrix;

		/// hb_list_t*
		public IntPtr list_audio;

		/// hb_list_t*
		public IntPtr list_subtitle;

		/// int
		public int mux;

		/// char*
		[MarshalAs(UnmanagedType.LPStr)]
		public string file;

		/// int
		public int largeFileSize;

		/// int
		public int mp4_optimize;

		/// int
		public int ipod_atom;

		/// int
		public int indepth_scan;

		/// hb_subtitle_config_t->hb_subtitle_config_s
		public hb_subtitle_config_s select_subtitle_config;

		/// int
		public int angle;

		public int frame_to_start;

		public long pts_to_start;

		/// int
		public int frame_to_stop;

		/// int64_t->int
		public long pts_to_stop;

		/// int
		public int start_at_preview;

		/// int
		public int seek_points;

		/// uint32_t->unsigned int
		public uint frames_to_skip;

		// Padding for the part of the struct we don't care about marshaling.
		[MarshalAs(UnmanagedType.ByValArray, SizeConst = MarshalingConstants.JobPaddingBytes, ArraySubType = UnmanagedType.U1)]
		public byte[] padding;
	}
}
