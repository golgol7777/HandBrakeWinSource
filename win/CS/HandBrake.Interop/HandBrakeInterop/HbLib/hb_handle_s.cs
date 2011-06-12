﻿using System;
using System.Runtime.InteropServices;

namespace HandBrake.Interop
{
	[StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
	public struct hb_handle_s
	{
		public int id;

		/// int
		public int build;

		/// char[32]
		[MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
		public string version;

		/// hb_thread_t*
		public IntPtr update_thread;

		/// int
		public int die;

		/// hb_thread_t*
		public IntPtr main_thread;

		/// int
		public int pid;

		/// hb_list_t*
		public IntPtr list_title;

		/// hb_thread_t*
		public IntPtr scan_thread;

		/// hb_list_t*
		public IntPtr jobs;

		/// hb_job_t*
		public IntPtr current_job;

		/// int
		public int job_count;

		/// int
		public int job_count_permanent;

		/// int
		public int work_die;

		/// int
		public int work_error;

		/// hb_thread_t*
		public IntPtr work_thread;

		// This is REMOVED in the latest HB SVN
		public int cpu_count;

		/// hb_lock_t*
		public IntPtr state_lock;

		/// hb_state_t->hb_state_s
		public hb_state_s state;

		/// int
		public int paused;

		/// hb_lock_t*
		public IntPtr pause_lock;

		/// int
		public int scanCount;

		/// hb_interjob_t*
		public IntPtr interjob;
	}
}
