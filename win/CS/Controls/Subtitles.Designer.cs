﻿namespace Handbrake.Controls
{
    partial class Subtitles
    {
        /// <summary> 
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary> 
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Component Designer generated code

        /// <summary> 
        /// Required method for Designer support - do not modify 
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.btn_addTrack = new System.Windows.Forms.Button();
            this.btn_RemoveSubTrack = new System.Windows.Forms.Button();
            this.lv_subList = new System.Windows.Forms.ListView();
            this.col_Source = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.col_forced = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.col_burned = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.col_defaultTrack = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.col_srtLang = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.col_srtChar = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.col_srtOffset = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
            this.subMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.mnu_moveup = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_movedown = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_remove = new System.Windows.Forms.ToolStripMenuItem();
            this.label68 = new System.Windows.Forms.Label();
            this.drp_subtitleTracks = new System.Windows.Forms.ComboBox();
            this.srt_offset = new System.Windows.Forms.NumericUpDown();
            this.srt_lang = new System.Windows.Forms.ComboBox();
            this.srt_charcode = new System.Windows.Forms.ComboBox();
            this.check_forced = new System.Windows.Forms.CheckBox();
            this.check_default = new System.Windows.Forms.CheckBox();
            this.check_burned = new System.Windows.Forms.CheckBox();
            this.btn_srtAdd = new System.Windows.Forms.Button();
            this.openFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.toolTip = new System.Windows.Forms.ToolTip(this.components);
            this.subMenu.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.srt_offset)).BeginInit();
            this.SuspendLayout();
            // 
            // btn_addTrack
            // 
            this.btn_addTrack.BackColor = System.Drawing.Color.Transparent;
            this.btn_addTrack.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_addTrack.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_addTrack.Location = new System.Drawing.Point(16, 38);
            this.btn_addTrack.Name = "btn_addTrack";
            this.btn_addTrack.Size = new System.Drawing.Size(57, 23);
            this.btn_addTrack.TabIndex = 68;
            this.btn_addTrack.Text = "Add";
            this.toolTip.SetToolTip(this.btn_addTrack, "Add a new subtitle track to the list below");
            this.btn_addTrack.UseVisualStyleBackColor = false;
            this.btn_addTrack.Click += new System.EventHandler(this.BtnAddSubTrackClick);
            // 
            // btn_RemoveSubTrack
            // 
            this.btn_RemoveSubTrack.BackColor = System.Drawing.Color.Transparent;
            this.btn_RemoveSubTrack.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_RemoveSubTrack.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_RemoveSubTrack.Location = new System.Drawing.Point(79, 38);
            this.btn_RemoveSubTrack.Name = "btn_RemoveSubTrack";
            this.btn_RemoveSubTrack.Size = new System.Drawing.Size(73, 23);
            this.btn_RemoveSubTrack.TabIndex = 69;
            this.btn_RemoveSubTrack.Text = "Remove";
            this.btn_RemoveSubTrack.UseVisualStyleBackColor = false;
            this.btn_RemoveSubTrack.Click += new System.EventHandler(this.BtnRemoveSubTrackClick);
            // 
            // lv_subList
            // 
            this.lv_subList.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.col_Source,
            this.col_forced,
            this.col_burned,
            this.col_defaultTrack,
            this.col_srtLang,
            this.col_srtChar,
            this.col_srtOffset});
            this.lv_subList.ContextMenuStrip = this.subMenu;
            this.lv_subList.FullRowSelect = true;
            this.lv_subList.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
            this.lv_subList.HideSelection = false;
            this.lv_subList.LabelWrap = false;
            this.lv_subList.Location = new System.Drawing.Point(16, 96);
            this.lv_subList.MultiSelect = false;
            this.lv_subList.Name = "lv_subList";
            this.lv_subList.Size = new System.Drawing.Size(688, 186);
            this.lv_subList.TabIndex = 70;
            this.lv_subList.UseCompatibleStateImageBehavior = false;
            this.lv_subList.View = System.Windows.Forms.View.Details;
            this.lv_subList.SelectedIndexChanged += new System.EventHandler(this.LbSubListSelectedIndexChanged);
            // 
            // col_Source
            // 
            this.col_Source.Text = "Track";
            this.col_Source.Width = 160;
            // 
            // col_forced
            // 
            this.col_forced.Text = "Forced Only";
            this.col_forced.Width = 74;
            // 
            // col_burned
            // 
            this.col_burned.Text = "Burned In";
            this.col_burned.Width = 64;
            // 
            // col_defaultTrack
            // 
            this.col_defaultTrack.Text = "Default";
            this.col_defaultTrack.Width = 56;
            // 
            // col_srtLang
            // 
            this.col_srtLang.Text = "Srt Lang";
            this.col_srtLang.Width = 120;
            // 
            // col_srtChar
            // 
            this.col_srtChar.Text = "Srt CharCode";
            this.col_srtChar.Width = 120;
            // 
            // col_srtOffset
            // 
            this.col_srtOffset.Text = "Srt Offset (ms)";
            this.col_srtOffset.Width = 90;
            // 
            // subMenu
            // 
            this.subMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_moveup,
            this.mnu_movedown,
            this.toolStripSeparator2,
            this.mnu_remove});
            this.subMenu.Name = "audioMenu";
            this.subMenu.Size = new System.Drawing.Size(139, 76);
            // 
            // mnu_moveup
            // 
            this.mnu_moveup.Name = "mnu_moveup";
            this.mnu_moveup.Size = new System.Drawing.Size(138, 22);
            this.mnu_moveup.Text = "Move Up";
            this.mnu_moveup.Click += new System.EventHandler(this.MnuMoveupClick);
            // 
            // mnu_movedown
            // 
            this.mnu_movedown.Name = "mnu_movedown";
            this.mnu_movedown.Size = new System.Drawing.Size(138, 22);
            this.mnu_movedown.Text = "Move Down";
            this.mnu_movedown.TextAlign = System.Drawing.ContentAlignment.MiddleRight;
            this.mnu_movedown.Click += new System.EventHandler(this.MnuMovedownClick);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(135, 6);
            // 
            // mnu_remove
            // 
            this.mnu_remove.Name = "mnu_remove";
            this.mnu_remove.Size = new System.Drawing.Size(138, 22);
            this.mnu_remove.Text = "Remove";
            this.mnu_remove.Click += new System.EventHandler(this.MnuRemoveClick);
            // 
            // label68
            // 
            this.label68.AutoSize = true;
            this.label68.BackColor = System.Drawing.Color.Transparent;
            this.label68.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label68.Location = new System.Drawing.Point(13, 13);
            this.label68.Name = "label68";
            this.label68.Size = new System.Drawing.Size(57, 13);
            this.label68.TabIndex = 67;
            this.label68.Text = "Subtitles";
            // 
            // drp_subtitleTracks
            // 
            this.drp_subtitleTracks.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_subtitleTracks.FormattingEnabled = true;
            this.drp_subtitleTracks.Location = new System.Drawing.Point(16, 69);
            this.drp_subtitleTracks.Name = "drp_subtitleTracks";
            this.drp_subtitleTracks.Size = new System.Drawing.Size(162, 21);
            this.drp_subtitleTracks.TabIndex = 74;
            this.drp_subtitleTracks.SelectedIndexChanged += new System.EventHandler(this.DrpSubtitleTracksSelectedIndexChanged);
            // 
            // srt_offset
            // 
            this.srt_offset.Enabled = false;
            this.srt_offset.Increment = new decimal(new int[] {
            10,
            0,
            0,
            0});
            this.srt_offset.Location = new System.Drawing.Point(620, 70);
            this.srt_offset.Maximum = new decimal(new int[] {
            1000000000,
            0,
            0,
            0});
            this.srt_offset.Minimum = new decimal(new int[] {
            1000000000,
            0,
            0,
            -2147483648});
            this.srt_offset.Name = "srt_offset";
            this.srt_offset.Size = new System.Drawing.Size(58, 21);
            this.srt_offset.TabIndex = 79;
            this.srt_offset.ValueChanged += new System.EventHandler(this.SrtOffsetValueChanged);
            // 
            // srt_lang
            // 
            this.srt_lang.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.srt_lang.Enabled = false;
            this.srt_lang.FormattingEnabled = true;
            this.srt_lang.Location = new System.Drawing.Point(371, 69);
            this.srt_lang.Name = "srt_lang";
            this.srt_lang.Size = new System.Drawing.Size(114, 21);
            this.srt_lang.TabIndex = 50;
            this.srt_lang.SelectedIndexChanged += new System.EventHandler(this.SrtLangSelectedIndexChanged);
            // 
            // srt_charcode
            // 
            this.srt_charcode.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.srt_charcode.Enabled = false;
            this.srt_charcode.FormattingEnabled = true;
            this.srt_charcode.Items.AddRange(new object[] {
            "ANSI_X3.4-1968",
            "ANSI_X3.4-1986",
            "ANSI_X3.4",
            "ANSI_X3.110-1983",
            "ANSI_X3.110",
            "ASCII",
            "ECMA-114",
            "ECMA-118",
            "ECMA-128",
            "ECMA-CYRILLIC",
            "IEC_P27-1",
            "ISO-8859-1",
            "ISO-8859-2",
            "ISO-8859-3",
            "ISO-8859-4",
            "ISO-8859-5",
            "ISO-8859-6",
            "ISO-8859-7",
            "ISO-8859-8",
            "ISO-8859-9",
            "ISO-8859-9E",
            "ISO-8859-10",
            "ISO-8859-11",
            "ISO-8859-13",
            "ISO-8859-14",
            "ISO-8859-15",
            "ISO-8859-16",
            "UTF-7",
            "UTF-8",
            "UTF-16",
            "UTF-32"});
            this.srt_charcode.Location = new System.Drawing.Point(495, 69);
            this.srt_charcode.Name = "srt_charcode";
            this.srt_charcode.Size = new System.Drawing.Size(101, 21);
            this.srt_charcode.TabIndex = 73;
            this.srt_charcode.SelectedIndexChanged += new System.EventHandler(this.SrtCharcodeSelectedIndexChanged);
            // 
            // check_forced
            // 
            this.check_forced.AutoSize = true;
            this.check_forced.Location = new System.Drawing.Point(208, 72);
            this.check_forced.Name = "check_forced";
            this.check_forced.Size = new System.Drawing.Size(15, 14);
            this.check_forced.TabIndex = 75;
            this.check_forced.UseVisualStyleBackColor = true;
            this.check_forced.CheckedChanged += new System.EventHandler(this.CheckForcedCheckedChanged);
            // 
            // check_default
            // 
            this.check_default.AutoSize = true;
            this.check_default.Location = new System.Drawing.Point(334, 72);
            this.check_default.Name = "check_default";
            this.check_default.Size = new System.Drawing.Size(15, 14);
            this.check_default.TabIndex = 77;
            this.check_default.UseVisualStyleBackColor = true;
            this.check_default.CheckedChanged += new System.EventHandler(this.CheckDefaultCheckedChanged);
            // 
            // check_burned
            // 
            this.check_burned.AutoSize = true;
            this.check_burned.Location = new System.Drawing.Point(274, 72);
            this.check_burned.Name = "check_burned";
            this.check_burned.Size = new System.Drawing.Size(15, 14);
            this.check_burned.TabIndex = 76;
            this.check_burned.UseVisualStyleBackColor = true;
            this.check_burned.CheckedChanged += new System.EventHandler(this.CheckBurnedCheckedChanged);
            // 
            // btn_srtAdd
            // 
            this.btn_srtAdd.BackColor = System.Drawing.Color.Transparent;
            this.btn_srtAdd.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_srtAdd.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_srtAdd.Location = new System.Drawing.Point(158, 38);
            this.btn_srtAdd.Name = "btn_srtAdd";
            this.btn_srtAdd.Size = new System.Drawing.Size(88, 23);
            this.btn_srtAdd.TabIndex = 73;
            this.btn_srtAdd.Text = "Import SRT";
            this.toolTip.SetToolTip(this.btn_srtAdd, "Add a new SRT file to the \"track\" dropdown menu.");
            this.btn_srtAdd.UseVisualStyleBackColor = false;
            this.btn_srtAdd.Click += new System.EventHandler(this.BtnSrtAddClick);
            // 
            // openFileDialog
            // 
            this.openFileDialog.DefaultExt = "srt";
            this.openFileDialog.Filter = "SRT Files |*.srt";
            // 
            // Subtitles
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.drp_subtitleTracks);
            this.Controls.Add(this.srt_offset);
            this.Controls.Add(this.btn_addTrack);
            this.Controls.Add(this.srt_lang);
            this.Controls.Add(this.srt_charcode);
            this.Controls.Add(this.label68);
            this.Controls.Add(this.btn_RemoveSubTrack);
            this.Controls.Add(this.lv_subList);
            this.Controls.Add(this.btn_srtAdd);
            this.Controls.Add(this.check_forced);
            this.Controls.Add(this.check_burned);
            this.Controls.Add(this.check_default);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "Subtitles";
            this.Size = new System.Drawing.Size(719, 300);
            this.subMenu.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.srt_offset)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Button btn_RemoveSubTrack;
        private System.Windows.Forms.Button btn_addTrack;
        internal System.Windows.Forms.ListView lv_subList;
        private System.Windows.Forms.ColumnHeader col_Source;
        private System.Windows.Forms.ColumnHeader col_forced;
        private System.Windows.Forms.ColumnHeader col_burned;
        internal System.Windows.Forms.Label label68;
        private System.Windows.Forms.ColumnHeader col_defaultTrack;
        private System.Windows.Forms.ContextMenuStrip subMenu;
        private System.Windows.Forms.ToolStripMenuItem mnu_moveup;
        private System.Windows.Forms.ToolStripMenuItem mnu_movedown;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator2;
        private System.Windows.Forms.ToolStripMenuItem mnu_remove;
        private System.Windows.Forms.Button btn_srtAdd;
        private System.Windows.Forms.ColumnHeader col_srtChar;
        private System.Windows.Forms.ColumnHeader col_srtOffset;
        internal System.Windows.Forms.ComboBox srt_charcode;
        internal System.Windows.Forms.ComboBox srt_lang;
        private System.Windows.Forms.NumericUpDown srt_offset;
        private System.Windows.Forms.OpenFileDialog openFileDialog;
        private System.Windows.Forms.CheckBox check_default;
        private System.Windows.Forms.CheckBox check_burned;
        private System.Windows.Forms.CheckBox check_forced;
        internal System.Windows.Forms.ComboBox drp_subtitleTracks;
        private System.Windows.Forms.ColumnHeader col_srtLang;
        private System.Windows.Forms.ToolTip toolTip;
    }
}
