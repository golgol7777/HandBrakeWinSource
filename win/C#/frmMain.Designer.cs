/*  frmMain.Designer.cs 
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

using System;
using System.Windows.Forms;

namespace Handbrake
{
    partial class frmMain
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

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.Windows.Forms.ContextMenuStrip notifyIconMenu;
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmMain));
            System.Windows.Forms.DataGridViewCellStyle dataGridViewCellStyle1 = new System.Windows.Forms.DataGridViewCellStyle();
            this.btn_restore = new System.Windows.Forms.ToolStripMenuItem();
            this.DVD_Save = new System.Windows.Forms.SaveFileDialog();
            this.ToolTip = new System.Windows.Forms.ToolTip(this.components);
            this.text_destination = new System.Windows.Forms.TextBox();
            this.drp_videoEncoder = new System.Windows.Forms.ComboBox();
            this.check_largeFile = new System.Windows.Forms.CheckBox();
            this.check_turbo = new System.Windows.Forms.CheckBox();
            this.drp_videoFramerate = new System.Windows.Forms.ComboBox();
            this.slider_videoQuality = new System.Windows.Forms.TrackBar();
            this.text_bitrate = new System.Windows.Forms.TextBox();
            this.check_optimiseMP4 = new System.Windows.Forms.CheckBox();
            this.check_iPodAtom = new System.Windows.Forms.CheckBox();
            this.data_chpt = new System.Windows.Forms.DataGridView();
            this.number = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.name = new System.Windows.Forms.DataGridViewTextBoxColumn();
            this.ChaptersMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.mnu_resetChapters = new System.Windows.Forms.ToolStripMenuItem();
            this.btn_file_source = new System.Windows.Forms.ToolStripMenuItem();
            this.drop_format = new System.Windows.Forms.ComboBox();
            this.drop_chapterFinish = new System.Windows.Forms.ComboBox();
            this.drop_chapterStart = new System.Windows.Forms.ComboBox();
            this.drop_angle = new System.Windows.Forms.ComboBox();
            this.drp_dvdtitle = new System.Windows.Forms.ComboBox();
            this.btn_importChapters = new System.Windows.Forms.Button();
            this.btn_export = new System.Windows.Forms.Button();
            this.drop_mode = new System.Windows.Forms.ComboBox();
            this.btn_generate_Query = new System.Windows.Forms.Button();
            this.radio_cq = new System.Windows.Forms.RadioButton();
            this.radio_avgBitrate = new System.Windows.Forms.RadioButton();
            this.check_2PassEncode = new System.Windows.Forms.CheckBox();
            this.treeView_presets = new System.Windows.Forms.TreeView();
            this.presets_menu = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.pmnu_expandAll = new System.Windows.Forms.ToolStripMenuItem();
            this.pmnu_collapse = new System.Windows.Forms.ToolStripMenuItem();
            this.sep1 = new System.Windows.Forms.ToolStripSeparator();
            this.pmnu_import = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator2 = new System.Windows.Forms.ToolStripSeparator();
            this.pmnu_saveChanges = new System.Windows.Forms.ToolStripMenuItem();
            this.pmnu_delete = new System.Windows.Forms.ToolStripMenuItem();
            this.DVD_Open = new System.Windows.Forms.FolderBrowserDialog();
            this.File_Open = new System.Windows.Forms.OpenFileDialog();
            this.ISO_Open = new System.Windows.Forms.OpenFileDialog();
            this.FileToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_killCLI = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_exit = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_open3 = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolsToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_encode = new System.Windows.Forms.ToolStripMenuItem();
            this.mnu_encodeLog = new System.Windows.Forms.ToolStripMenuItem();
            this.ToolStripSeparator5 = new System.Windows.Forms.ToolStripSeparator();
            this.mnu_options = new System.Windows.Forms.ToolStripMenuItem();
            this.frmMainMenu = new System.Windows.Forms.MenuStrip();
            this.label5 = new System.Windows.Forms.Label();
            this.Label47 = new System.Windows.Forms.Label();
            this.Label3 = new System.Windows.Forms.Label();
            this.tab_audio = new System.Windows.Forms.TabPage();
            this.AudioSettings = new Handbrake.Controls.AudioPanel();
            this.AudioMenuRowHeightHack = new System.Windows.Forms.ImageList(this.components);
            this.tab_video = new System.Windows.Forms.TabPage();
            this.panel1 = new System.Windows.Forms.Panel();
            this.radio_constantFramerate = new System.Windows.Forms.RadioButton();
            this.radio_peakAndVariable = new System.Windows.Forms.RadioButton();
            this.label25 = new System.Windows.Forms.Label();
            this.Label2 = new System.Windows.Forms.Label();
            this.lbl_SliderValue = new System.Windows.Forms.Label();
            this.lbl_framerate = new System.Windows.Forms.Label();
            this.tab_picture = new System.Windows.Forms.TabPage();
            this.PictureSettings = new Handbrake.Controls.PictureSettings();
            this.Check_ChapterMarkers = new System.Windows.Forms.CheckBox();
            this.tabs_panel = new System.Windows.Forms.TabControl();
            this.tab_filters = new System.Windows.Forms.TabPage();
            this.Filters = new Handbrake.Controls.Filters();
            this.tab_subtitles = new System.Windows.Forms.TabPage();
            this.Subtitles = new Handbrake.Controls.Subtitles();
            this.tab_chapters = new System.Windows.Forms.TabPage();
            this.label31 = new System.Windows.Forms.Label();
            this.tab_advanced = new System.Windows.Forms.TabPage();
            this.x264Panel = new Handbrake.Controls.x264Panel();
            this.tab_query = new System.Windows.Forms.TabPage();
            this.btn_clear = new System.Windows.Forms.Button();
            this.label34 = new System.Windows.Forms.Label();
            this.label33 = new System.Windows.Forms.Label();
            this.rtf_query = new System.Windows.Forms.RichTextBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.splitContainer1 = new System.Windows.Forms.SplitContainer();
            this.presetsToolStrip = new System.Windows.Forms.ToolStrip();
            this.BtnAddPreset = new System.Windows.Forms.ToolStripButton();
            this.BtnRemovePreset = new System.Windows.Forms.ToolStripButton();
            this.toolStripDropDownButton2 = new System.Windows.Forms.ToolStripDropDownButton();
            this.MnuSetDefaultPreset = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator3 = new System.Windows.Forms.ToolStripSeparator();
            this.MnuImportPreset = new System.Windows.Forms.ToolStripMenuItem();
            this.MnuExportPreset = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator6 = new System.Windows.Forms.ToolStripSeparator();
            this.MnuResetBuiltInPresets = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.btn_source = new System.Windows.Forms.ToolStripDropDownButton();
            this.btn_dvd_source = new System.Windows.Forms.ToolStripMenuItem();
            this.btnTitleSpecific = new System.Windows.Forms.ToolStripMenuItem();
            this.FileTitleSpecificScan = new System.Windows.Forms.ToolStripMenuItem();
            this.FolderTitleSpecificScan = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator1 = new System.Windows.Forms.ToolStripSeparator();
            this.toolStripSeparator10 = new System.Windows.Forms.ToolStripSeparator();
            this.btn_start = new System.Windows.Forms.ToolStripButton();
            this.btn_add2Queue = new System.Windows.Forms.ToolStripButton();
            this.btn_showQueue = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator4 = new System.Windows.Forms.ToolStripSeparator();
            this.tb_preview = new System.Windows.Forms.ToolStripButton();
            this.btn_ActivityWindow = new System.Windows.Forms.ToolStripButton();
            this.toolStripSeparator8 = new System.Windows.Forms.ToolStripSeparator();
            this.toolStripDropDownButton1 = new System.Windows.Forms.ToolStripDropDownButton();
            this.MnuUserGuide = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator9 = new System.Windows.Forms.ToolStripSeparator();
            this.MnuCheckForUpdates = new System.Windows.Forms.ToolStripMenuItem();
            this.toolStripSeparator11 = new System.Windows.Forms.ToolStripSeparator();
            this.MnuAboutHandBrake = new System.Windows.Forms.ToolStripMenuItem();
            this.notifyIcon = new System.Windows.Forms.NotifyIcon(this.components);
            this.StatusStrip = new System.Windows.Forms.StatusStrip();
            this.ProgressBarStatus = new System.Windows.Forms.ToolStripProgressBar();
            this.lbl_encode = new System.Windows.Forms.ToolStripStatusLabel();
            this.lbl_updateCheck = new System.Windows.Forms.ToolStripStatusLabel();
            this.hbproc = new System.Diagnostics.Process();
            this.File_Save = new System.Windows.Forms.SaveFileDialog();
            this.tableLayoutPanel2 = new System.Windows.Forms.TableLayoutPanel();
            this.btn_destBrowse = new System.Windows.Forms.Button();
            this.tableLayoutPanel3 = new System.Windows.Forms.TableLayoutPanel();
            this.tableLayoutPanel1 = new System.Windows.Forms.TableLayoutPanel();
            this.Label10 = new System.Windows.Forms.Label();
            this.lbl_angle = new System.Windows.Forms.Label();
            this.Label13 = new System.Windows.Forms.Label();
            this.label_duration = new System.Windows.Forms.Label();
            this.lbl_duration = new System.Windows.Forms.Label();
            this.labelStaticDestination = new System.Windows.Forms.Label();
            this.labelPreset = new System.Windows.Forms.Label();
            this.labelSource = new System.Windows.Forms.Label();
            this.labelStaticSource = new System.Windows.Forms.Label();
            this.SourceLayoutPanel = new System.Windows.Forms.FlowLayoutPanel();
            this.openPreset = new System.Windows.Forms.OpenFileDialog();
            this.File_ChapterImport = new System.Windows.Forms.OpenFileDialog();
            notifyIconMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
            notifyIconMenu.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.slider_videoQuality)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.data_chpt)).BeginInit();
            this.ChaptersMenu.SuspendLayout();
            this.presets_menu.SuspendLayout();
            this.frmMainMenu.SuspendLayout();
            this.tab_audio.SuspendLayout();
            this.tab_video.SuspendLayout();
            this.panel1.SuspendLayout();
            this.tab_picture.SuspendLayout();
            this.tabs_panel.SuspendLayout();
            this.tab_filters.SuspendLayout();
            this.tab_subtitles.SuspendLayout();
            this.tab_chapters.SuspendLayout();
            this.tab_advanced.SuspendLayout();
            this.tab_query.SuspendLayout();
            this.groupBox2.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).BeginInit();
            this.splitContainer1.Panel1.SuspendLayout();
            this.splitContainer1.Panel2.SuspendLayout();
            this.splitContainer1.SuspendLayout();
            this.presetsToolStrip.SuspendLayout();
            this.toolStrip1.SuspendLayout();
            this.StatusStrip.SuspendLayout();
            this.tableLayoutPanel2.SuspendLayout();
            this.tableLayoutPanel3.SuspendLayout();
            this.tableLayoutPanel1.SuspendLayout();
            this.SourceLayoutPanel.SuspendLayout();
            this.SuspendLayout();
            // 
            // notifyIconMenu
            // 
            notifyIconMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_restore});
            notifyIconMenu.Name = "notifyIconMenu";
            notifyIconMenu.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            notifyIconMenu.Size = new System.Drawing.Size(114, 26);
            // 
            // btn_restore
            // 
            this.btn_restore.Image = global::Handbrake.Properties.Resources.Restore;
            this.btn_restore.Name = "btn_restore";
            this.btn_restore.Size = new System.Drawing.Size(113, 22);
            this.btn_restore.Text = "Restore";
            this.btn_restore.Click += new System.EventHandler(this.btn_restore_Click);
            // 
            // DVD_Save
            // 
            this.DVD_Save.Filter = "mp4|*.mp4;*.m4v|mkv|*.mkv";
            this.DVD_Save.SupportMultiDottedExtensions = true;
            // 
            // ToolTip
            // 
            this.ToolTip.Active = false;
            this.ToolTip.AutomaticDelay = 1000;
            this.ToolTip.ToolTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            this.ToolTip.ToolTipTitle = "Tooltip";
            // 
            // text_destination
            // 
            this.text_destination.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                        | System.Windows.Forms.AnchorStyles.Right)));
            this.text_destination.Location = new System.Drawing.Point(36, 3);
            this.text_destination.Name = "text_destination";
            this.text_destination.Size = new System.Drawing.Size(603, 21);
            this.text_destination.TabIndex = 1;
            this.ToolTip.SetToolTip(this.text_destination, "Location where the encoded file will be saved.");
            this.text_destination.TextChanged += new System.EventHandler(this.text_destination_TextChanged);
            // 
            // drp_videoEncoder
            // 
            this.drp_videoEncoder.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_videoEncoder.FormattingEnabled = true;
            this.drp_videoEncoder.Items.AddRange(new object[] {
            "MPEG-4 (FFmpeg)",
            "H.264 (x264)",
            "VP3 (Theora)"});
            this.drp_videoEncoder.Location = new System.Drawing.Point(125, 35);
            this.drp_videoEncoder.Name = "drp_videoEncoder";
            this.drp_videoEncoder.Size = new System.Drawing.Size(126, 21);
            this.drp_videoEncoder.TabIndex = 6;
            this.ToolTip.SetToolTip(this.drp_videoEncoder, "Select a video encoder");
            this.drp_videoEncoder.SelectedIndexChanged += new System.EventHandler(this.drp_videoEncoder_SelectedIndexChanged);
            // 
            // check_largeFile
            // 
            this.check_largeFile.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_largeFile.AutoSize = true;
            this.check_largeFile.BackColor = System.Drawing.Color.Transparent;
            this.check_largeFile.Location = new System.Drawing.Point(179, 5);
            this.check_largeFile.Name = "check_largeFile";
            this.check_largeFile.Size = new System.Drawing.Size(91, 17);
            this.check_largeFile.TabIndex = 2;
            this.check_largeFile.Text = "Large file size";
            this.ToolTip.SetToolTip(this.check_largeFile, "Caution: This option will likely break device compatibility with all but the Appl" +
                    "eTV Take 2.\r\nChecking this box enables a 64bit mp4 file which can be over 4GB.");
            this.check_largeFile.UseVisualStyleBackColor = false;
            // 
            // check_turbo
            // 
            this.check_turbo.AutoSize = true;
            this.check_turbo.BackColor = System.Drawing.Color.Transparent;
            this.check_turbo.Enabled = false;
            this.check_turbo.Location = new System.Drawing.Point(495, 134);
            this.check_turbo.Name = "check_turbo";
            this.check_turbo.Size = new System.Drawing.Size(101, 17);
            this.check_turbo.TabIndex = 9;
            this.check_turbo.Text = "Turbo first Pass";
            this.ToolTip.SetToolTip(this.check_turbo, "Makes the first pass of a 2 pass encode faster.");
            this.check_turbo.UseVisualStyleBackColor = false;
            // 
            // drp_videoFramerate
            // 
            this.drp_videoFramerate.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_videoFramerate.FormattingEnabled = true;
            this.drp_videoFramerate.Items.AddRange(new object[] {
            "Same as source",
            "5",
            "10",
            "12",
            "15",
            "23.976",
            "24",
            "25",
            "29.97"});
            this.drp_videoFramerate.Location = new System.Drawing.Point(125, 62);
            this.drp_videoFramerate.Name = "drp_videoFramerate";
            this.drp_videoFramerate.Size = new System.Drawing.Size(125, 21);
            this.drp_videoFramerate.TabIndex = 2;
            this.ToolTip.SetToolTip(this.drp_videoFramerate, "Can be left to \"Same as source\" in most cases.");
            this.drp_videoFramerate.SelectedIndexChanged += new System.EventHandler(this.drp_videoFramerate_SelectedIndexChanged);
            // 
            // slider_videoQuality
            // 
            this.slider_videoQuality.BackColor = System.Drawing.SystemColors.Window;
            this.slider_videoQuality.Enabled = false;
            this.slider_videoQuality.Location = new System.Drawing.Point(377, 60);
            this.slider_videoQuality.Margin = new System.Windows.Forms.Padding(0);
            this.slider_videoQuality.Maximum = 100;
            this.slider_videoQuality.Name = "slider_videoQuality";
            this.slider_videoQuality.Size = new System.Drawing.Size(322, 45);
            this.slider_videoQuality.TabIndex = 13;
            this.slider_videoQuality.TickFrequency = 17;
            this.ToolTip.SetToolTip(this.slider_videoQuality, resources.GetString("slider_videoQuality.ToolTip"));
            this.slider_videoQuality.ValueChanged += new System.EventHandler(this.slider_videoQuality_Scroll);
            // 
            // text_bitrate
            // 
            this.text_bitrate.Location = new System.Drawing.Point(489, 107);
            this.text_bitrate.Name = "text_bitrate";
            this.text_bitrate.Size = new System.Drawing.Size(81, 21);
            this.text_bitrate.TabIndex = 14;
            this.ToolTip.SetToolTip(this.text_bitrate, resources.GetString("text_bitrate.ToolTip"));
            // 
            // check_optimiseMP4
            // 
            this.check_optimiseMP4.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_optimiseMP4.AutoSize = true;
            this.check_optimiseMP4.BackColor = System.Drawing.Color.Transparent;
            this.check_optimiseMP4.Location = new System.Drawing.Point(276, 5);
            this.check_optimiseMP4.Name = "check_optimiseMP4";
            this.check_optimiseMP4.Size = new System.Drawing.Size(96, 17);
            this.check_optimiseMP4.TabIndex = 3;
            this.check_optimiseMP4.Text = "Web optimized";
            this.ToolTip.SetToolTip(this.check_optimiseMP4, "MP4 files can be optimized for progressive downloads over the Web,\r\nbut note that" +
                    " QuickTime can only read the files as long as the file extension is .mp4\r\nCan on" +
                    "ly be used with H.264 ");
            this.check_optimiseMP4.UseVisualStyleBackColor = false;
            // 
            // check_iPodAtom
            // 
            this.check_iPodAtom.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_iPodAtom.AutoSize = true;
            this.check_iPodAtom.BackColor = System.Drawing.Color.Transparent;
            this.check_iPodAtom.Location = new System.Drawing.Point(378, 5);
            this.check_iPodAtom.Name = "check_iPodAtom";
            this.check_iPodAtom.Size = new System.Drawing.Size(102, 17);
            this.check_iPodAtom.TabIndex = 4;
            this.check_iPodAtom.Text = "iPod 5G support";
            this.ToolTip.SetToolTip(this.check_iPodAtom, "Support for legacy 5th Generation iPods.\r\nEncodes will not sync if this option is" +
                    " not enabled for H.264 encodes.");
            this.check_iPodAtom.UseVisualStyleBackColor = false;
            // 
            // data_chpt
            // 
            this.data_chpt.AllowUserToAddRows = false;
            this.data_chpt.AllowUserToDeleteRows = false;
            this.data_chpt.AllowUserToResizeRows = false;
            this.data_chpt.BackgroundColor = System.Drawing.Color.White;
            this.data_chpt.ColumnHeadersBorderStyle = System.Windows.Forms.DataGridViewHeaderBorderStyle.None;
            this.data_chpt.ColumnHeadersHeightSizeMode = System.Windows.Forms.DataGridViewColumnHeadersHeightSizeMode.DisableResizing;
            this.data_chpt.Columns.AddRange(new System.Windows.Forms.DataGridViewColumn[] {
            this.number,
            this.name});
            this.data_chpt.ContextMenuStrip = this.ChaptersMenu;
            this.data_chpt.Location = new System.Drawing.Point(16, 55);
            this.data_chpt.MultiSelect = false;
            this.data_chpt.Name = "data_chpt";
            this.data_chpt.RowHeadersVisible = false;
            this.data_chpt.Size = new System.Drawing.Size(684, 236);
            this.data_chpt.TabIndex = 3;
            this.ToolTip.SetToolTip(this.data_chpt, resources.GetString("data_chpt.ToolTip"));
            // 
            // number
            // 
            dataGridViewCellStyle1.Format = "N0";
            dataGridViewCellStyle1.NullValue = null;
            this.number.DefaultCellStyle = dataGridViewCellStyle1;
            this.number.Frozen = true;
            this.number.HeaderText = "Chapter Number";
            this.number.MaxInputLength = 3;
            this.number.Name = "number";
            this.number.Resizable = System.Windows.Forms.DataGridViewTriState.False;
            this.number.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            this.number.Width = 165;
            // 
            // name
            // 
            this.name.AutoSizeMode = System.Windows.Forms.DataGridViewAutoSizeColumnMode.Fill;
            this.name.HeaderText = "Chapter Name";
            this.name.Name = "name";
            this.name.SortMode = System.Windows.Forms.DataGridViewColumnSortMode.NotSortable;
            // 
            // ChaptersMenu
            // 
            this.ChaptersMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_resetChapters});
            this.ChaptersMenu.Name = "presets_menu";
            this.ChaptersMenu.OwnerItem = this.btn_file_source;
            this.ChaptersMenu.Size = new System.Drawing.Size(188, 26);
            this.ChaptersMenu.Text = ";";
            // 
            // mnu_resetChapters
            // 
            this.mnu_resetChapters.Name = "mnu_resetChapters";
            this.mnu_resetChapters.Size = new System.Drawing.Size(187, 22);
            this.mnu_resetChapters.Text = "Reset Chapter Names";
            this.mnu_resetChapters.Click += new System.EventHandler(this.mnu_resetChapters_Click);
            // 
            // btn_file_source
            // 
            this.btn_file_source.Image = global::Handbrake.Properties.Resources.Movies_Small;
            this.btn_file_source.Name = "btn_file_source";
            this.btn_file_source.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.O)));
            this.btn_file_source.Size = new System.Drawing.Size(182, 22);
            this.btn_file_source.Text = "Video File";
            this.btn_file_source.Click += new System.EventHandler(this.BtnFileScanClicked);
            // 
            // drop_format
            // 
            this.drop_format.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_format.FormattingEnabled = true;
            this.drop_format.Items.AddRange(new object[] {
            "MP4 File",
            "MKV File"});
            this.drop_format.Location = new System.Drawing.Point(67, 3);
            this.drop_format.Name = "drop_format";
            this.drop_format.Size = new System.Drawing.Size(106, 21);
            this.drop_format.TabIndex = 1;
            this.ToolTip.SetToolTip(this.drop_format, "Select the file container format.\r\nHandBrake supports MKV and MP4(M4v)");
            this.drop_format.SelectedIndexChanged += new System.EventHandler(this.drop_format_SelectedIndexChanged);
            // 
            // drop_chapterFinish
            // 
            this.drop_chapterFinish.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_chapterFinish.FormattingEnabled = true;
            this.drop_chapterFinish.Location = new System.Drawing.Point(509, 3);
            this.drop_chapterFinish.Name = "drop_chapterFinish";
            this.drop_chapterFinish.Size = new System.Drawing.Size(69, 21);
            this.drop_chapterFinish.TabIndex = 7;
            this.ToolTip.SetToolTip(this.drop_chapterFinish, "Select the chapter range you would like to enocde. (default: All Chapters)");
            this.drop_chapterFinish.SelectedIndexChanged += new System.EventHandler(this.chapersChanged);
            // 
            // drop_chapterStart
            // 
            this.drop_chapterStart.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_chapterStart.FormattingEnabled = true;
            this.drop_chapterStart.Location = new System.Drawing.Point(383, 3);
            this.drop_chapterStart.Name = "drop_chapterStart";
            this.drop_chapterStart.Size = new System.Drawing.Size(69, 21);
            this.drop_chapterStart.TabIndex = 5;
            this.ToolTip.SetToolTip(this.drop_chapterStart, "Select the chapter range you would like to enocde. (default: All Chapters)");
            this.drop_chapterStart.SelectedIndexChanged += new System.EventHandler(this.chapersChanged);
            // 
            // drop_angle
            // 
            this.drop_angle.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_angle.FormattingEnabled = true;
            this.drop_angle.Location = new System.Drawing.Point(209, 3);
            this.drop_angle.Name = "drop_angle";
            this.drop_angle.Size = new System.Drawing.Size(45, 21);
            this.drop_angle.TabIndex = 3;
            this.ToolTip.SetToolTip(this.drop_angle, "Select the chapter range you would like to enocde. (default: All Chapters)");
            // 
            // drp_dvdtitle
            // 
            this.drp_dvdtitle.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_dvdtitle.FormattingEnabled = true;
            this.drp_dvdtitle.Items.AddRange(new object[] {
            "Automatic"});
            this.drp_dvdtitle.Location = new System.Drawing.Point(40, 3);
            this.drp_dvdtitle.Name = "drp_dvdtitle";
            this.drp_dvdtitle.Size = new System.Drawing.Size(119, 21);
            this.drp_dvdtitle.TabIndex = 1;
            this.ToolTip.SetToolTip(this.drp_dvdtitle, "Select the title you wish to encode.\r\n\r\nWhen a DVD is in use, HandBrake will try " +
                    "to determine the \"Main Feature\" title automatically.\r\nPlease note, this is not a" +
                    "lways accurate and should be checked.");
            this.drp_dvdtitle.SelectedIndexChanged += new System.EventHandler(this.drp_dvdtitle_SelectedIndexChanged);
            this.drp_dvdtitle.Click += new System.EventHandler(this.drp_dvdtitle_Click);
            // 
            // btn_importChapters
            // 
            this.btn_importChapters.AutoSize = true;
            this.btn_importChapters.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_importChapters.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_importChapters.Location = new System.Drawing.Point(544, 28);
            this.btn_importChapters.Name = "btn_importChapters";
            this.btn_importChapters.Size = new System.Drawing.Size(75, 23);
            this.btn_importChapters.TabIndex = 1;
            this.btn_importChapters.Text = "Import";
            this.ToolTip.SetToolTip(this.btn_importChapters, resources.GetString("btn_importChapters.ToolTip"));
            this.btn_importChapters.UseVisualStyleBackColor = true;
            this.btn_importChapters.Click += new System.EventHandler(this.btn_importChapters_Click);
            // 
            // btn_export
            // 
            this.btn_export.AutoSize = true;
            this.btn_export.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_export.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_export.Location = new System.Drawing.Point(625, 28);
            this.btn_export.Name = "btn_export";
            this.btn_export.Size = new System.Drawing.Size(75, 23);
            this.btn_export.TabIndex = 0;
            this.btn_export.Text = "Export";
            this.ToolTip.SetToolTip(this.btn_export, resources.GetString("btn_export.ToolTip"));
            this.btn_export.UseVisualStyleBackColor = true;
            this.btn_export.Click += new System.EventHandler(this.btn_export_Click);
            // 
            // drop_mode
            // 
            this.drop_mode.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drop_mode.FormattingEnabled = true;
            this.drop_mode.Items.AddRange(new object[] {
            "Chapters",
            "Seconds",
            "Frames"});
            this.drop_mode.Location = new System.Drawing.Point(295, 3);
            this.drop_mode.Name = "drop_mode";
            this.drop_mode.Size = new System.Drawing.Size(77, 21);
            this.drop_mode.TabIndex = 4;
            this.ToolTip.SetToolTip(this.drop_mode, resources.GetString("drop_mode.ToolTip"));
            this.drop_mode.SelectedIndexChanged += new System.EventHandler(this.drop_mode_SelectedIndexChanged);
            // 
            // btn_generate_Query
            // 
            this.btn_generate_Query.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_generate_Query.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_generate_Query.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_generate_Query.Location = new System.Drawing.Point(16, 104);
            this.btn_generate_Query.Name = "btn_generate_Query";
            this.btn_generate_Query.Size = new System.Drawing.Size(126, 22);
            this.btn_generate_Query.TabIndex = 2;
            this.btn_generate_Query.Text = "Generate Query";
            this.ToolTip.SetToolTip(this.btn_generate_Query, "This will allow you to override the generated query.\r\nNote, The query in the box " +
                    "below will always override any automatically generated query, even if you change" +
                    " title or source.");
            this.btn_generate_Query.UseVisualStyleBackColor = true;
            this.btn_generate_Query.Click += new System.EventHandler(this.btn_generate_Query_Click);
            // 
            // radio_cq
            // 
            this.radio_cq.AutoSize = true;
            this.radio_cq.BackColor = System.Drawing.Color.Transparent;
            this.radio_cq.Location = new System.Drawing.Point(366, 37);
            this.radio_cq.Name = "radio_cq";
            this.radio_cq.Size = new System.Drawing.Size(110, 17);
            this.radio_cq.TabIndex = 3;
            this.radio_cq.Text = "Constant Quality:";
            this.ToolTip.SetToolTip(this.radio_cq, resources.GetString("radio_cq.ToolTip"));
            this.radio_cq.UseVisualStyleBackColor = false;
            this.radio_cq.CheckedChanged += new System.EventHandler(this.radio_cq_CheckedChanged);
            // 
            // radio_avgBitrate
            // 
            this.radio_avgBitrate.AutoSize = true;
            this.radio_avgBitrate.BackColor = System.Drawing.Color.Transparent;
            this.radio_avgBitrate.Checked = true;
            this.radio_avgBitrate.Location = new System.Drawing.Point(367, 108);
            this.radio_avgBitrate.Name = "radio_avgBitrate";
            this.radio_avgBitrate.Size = new System.Drawing.Size(116, 17);
            this.radio_avgBitrate.TabIndex = 4;
            this.radio_avgBitrate.TabStop = true;
            this.radio_avgBitrate.Text = "Avg Bitrate (kbps):";
            this.ToolTip.SetToolTip(this.radio_avgBitrate, resources.GetString("radio_avgBitrate.ToolTip"));
            this.radio_avgBitrate.UseVisualStyleBackColor = false;
            this.radio_avgBitrate.CheckedChanged += new System.EventHandler(this.radio_avgBitrate_CheckedChanged);
            // 
            // check_2PassEncode
            // 
            this.check_2PassEncode.AutoSize = true;
            this.check_2PassEncode.BackColor = System.Drawing.Color.Transparent;
            this.check_2PassEncode.Location = new System.Drawing.Point(385, 134);
            this.check_2PassEncode.Name = "check_2PassEncode";
            this.check_2PassEncode.Size = new System.Drawing.Size(104, 17);
            this.check_2PassEncode.TabIndex = 10;
            this.check_2PassEncode.Text = "2-Pass Encoding";
            this.ToolTip.SetToolTip(this.check_2PassEncode, resources.GetString("check_2PassEncode.ToolTip"));
            this.check_2PassEncode.UseVisualStyleBackColor = false;
            this.check_2PassEncode.CheckedChanged += new System.EventHandler(this.check_2PassEncode_CheckedChanged);
            // 
            // treeView_presets
            // 
            this.treeView_presets.ContextMenuStrip = this.presets_menu;
            this.treeView_presets.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.treeView_presets.ForeColor = System.Drawing.Color.DarkBlue;
            this.treeView_presets.FullRowSelect = true;
            this.treeView_presets.HideSelection = false;
            this.treeView_presets.ItemHeight = 21;
            this.treeView_presets.Location = new System.Drawing.Point(0, 0);
            this.treeView_presets.Margin = new System.Windows.Forms.Padding(3, 3, 3, 0);
            this.treeView_presets.Name = "treeView_presets";
            this.treeView_presets.ShowLines = false;
            this.treeView_presets.Size = new System.Drawing.Size(240, 424);
            this.treeView_presets.TabIndex = 0;
            this.treeView_presets.AfterSelect += new System.Windows.Forms.TreeViewEventHandler(this.treeView_presets_AfterSelect);
            this.treeView_presets.KeyUp += new System.Windows.Forms.KeyEventHandler(this.treeView_presets_deleteKey);
            this.treeView_presets.MouseMove += new System.Windows.Forms.MouseEventHandler(this.TreeViewPresetsMouseMove);
            this.treeView_presets.MouseUp += new System.Windows.Forms.MouseEventHandler(this.treeview_presets_mouseUp);
            // 
            // presets_menu
            // 
            this.presets_menu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.pmnu_expandAll,
            this.pmnu_collapse,
            this.sep1,
            this.pmnu_import,
            this.toolStripSeparator2,
            this.pmnu_saveChanges,
            this.pmnu_delete});
            this.presets_menu.Name = "presets_menu";
            this.presets_menu.Size = new System.Drawing.Size(148, 126);
            this.presets_menu.Text = ";";
            this.presets_menu.Opening += new System.ComponentModel.CancelEventHandler(this.presets_menu_Opening);
            // 
            // pmnu_expandAll
            // 
            this.pmnu_expandAll.Name = "pmnu_expandAll";
            this.pmnu_expandAll.Size = new System.Drawing.Size(147, 22);
            this.pmnu_expandAll.Text = "Expand All";
            this.pmnu_expandAll.Click += new System.EventHandler(this.pmnu_expandAll_Click);
            // 
            // pmnu_collapse
            // 
            this.pmnu_collapse.Name = "pmnu_collapse";
            this.pmnu_collapse.Size = new System.Drawing.Size(147, 22);
            this.pmnu_collapse.Text = "Collapse All";
            this.pmnu_collapse.Click += new System.EventHandler(this.pmnu_collapse_Click);
            // 
            // sep1
            // 
            this.sep1.Name = "sep1";
            this.sep1.Size = new System.Drawing.Size(144, 6);
            // 
            // pmnu_import
            // 
            this.pmnu_import.Name = "pmnu_import";
            this.pmnu_import.Size = new System.Drawing.Size(147, 22);
            this.pmnu_import.Text = "Import";
            this.pmnu_import.Click += new System.EventHandler(this.pmnu_import_Click);
            // 
            // toolStripSeparator2
            // 
            this.toolStripSeparator2.Name = "toolStripSeparator2";
            this.toolStripSeparator2.Size = new System.Drawing.Size(144, 6);
            // 
            // pmnu_saveChanges
            // 
            this.pmnu_saveChanges.Name = "pmnu_saveChanges";
            this.pmnu_saveChanges.Size = new System.Drawing.Size(147, 22);
            this.pmnu_saveChanges.Text = "Save Changes";
            this.pmnu_saveChanges.Click += new System.EventHandler(this.pmnu_saveChanges_Click);
            // 
            // pmnu_delete
            // 
            this.pmnu_delete.Name = "pmnu_delete";
            this.pmnu_delete.Size = new System.Drawing.Size(147, 22);
            this.pmnu_delete.Text = "Delete";
            this.pmnu_delete.Click += new System.EventHandler(this.pmnu_delete_click);
            // 
            // DVD_Open
            // 
            this.DVD_Open.Description = "Select the \"VIDEO_TS\" folder from your DVD Drive.";
            // 
            // File_Open
            // 
            this.File_Open.DefaultExt = "hb";
            this.File_Open.Filter = "hb|*.hb";
            // 
            // ISO_Open
            // 
            this.ISO_Open.DefaultExt = "ISO";
            this.ISO_Open.Filter = "All Files|*.*";
            this.ISO_Open.RestoreDirectory = true;
            this.ISO_Open.SupportMultiDottedExtensions = true;
            // 
            // FileToolStripMenuItem
            // 
            this.FileToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_killCLI,
            this.mnu_exit});
            this.FileToolStripMenuItem.Name = "FileToolStripMenuItem";
            this.FileToolStripMenuItem.Size = new System.Drawing.Size(37, 20);
            this.FileToolStripMenuItem.Text = "&File";
            // 
            // mnu_killCLI
            // 
            this.mnu_killCLI.Name = "mnu_killCLI";
            this.mnu_killCLI.Size = new System.Drawing.Size(138, 22);
            this.mnu_killCLI.Text = "Cancel Scan";
            this.mnu_killCLI.Visible = false;
            this.mnu_killCLI.Click += new System.EventHandler(this.mnu_killCLI_Click);
            // 
            // mnu_exit
            // 
            this.mnu_exit.Name = "mnu_exit";
            this.mnu_exit.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Alt | System.Windows.Forms.Keys.F4)));
            this.mnu_exit.Size = new System.Drawing.Size(138, 22);
            this.mnu_exit.Text = "E&xit";
            this.mnu_exit.Click += new System.EventHandler(this.mnu_exit_Click);
            // 
            // mnu_open3
            // 
            this.mnu_open3.Name = "mnu_open3";
            this.mnu_open3.Size = new System.Drawing.Size(32, 19);
            // 
            // ToolsToolStripMenuItem
            // 
            this.ToolsToolStripMenuItem.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.mnu_encode,
            this.mnu_encodeLog,
            this.ToolStripSeparator5,
            this.mnu_options});
            this.ToolsToolStripMenuItem.Name = "ToolsToolStripMenuItem";
            this.ToolsToolStripMenuItem.Size = new System.Drawing.Size(48, 20);
            this.ToolsToolStripMenuItem.Text = "&Tools";
            // 
            // mnu_encode
            // 
            this.mnu_encode.Image = global::Handbrake.Properties.Resources.Queue_Small;
            this.mnu_encode.Name = "mnu_encode";
            this.mnu_encode.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Q)));
            this.mnu_encode.Size = new System.Drawing.Size(201, 22);
            this.mnu_encode.Text = "Show Queue";
            this.mnu_encode.Click += new System.EventHandler(this.mnu_encode_Click);
            // 
            // mnu_encodeLog
            // 
            this.mnu_encodeLog.Image = global::Handbrake.Properties.Resources.ActivityWindow_small;
            this.mnu_encodeLog.Name = "mnu_encodeLog";
            this.mnu_encodeLog.ShortcutKeys = ((System.Windows.Forms.Keys)((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.L)));
            this.mnu_encodeLog.Size = new System.Drawing.Size(201, 22);
            this.mnu_encodeLog.Text = "Activity Window";
            this.mnu_encodeLog.Click += new System.EventHandler(this.mnu_encodeLog_Click);
            // 
            // ToolStripSeparator5
            // 
            this.ToolStripSeparator5.Name = "ToolStripSeparator5";
            this.ToolStripSeparator5.Size = new System.Drawing.Size(198, 6);
            // 
            // mnu_options
            // 
            this.mnu_options.Image = global::Handbrake.Properties.Resources.Pref_Small;
            this.mnu_options.Name = "mnu_options";
            this.mnu_options.Size = new System.Drawing.Size(201, 22);
            this.mnu_options.Text = "Options";
            this.mnu_options.Click += new System.EventHandler(this.mnu_options_Click);
            // 
            // frmMainMenu
            // 
            this.frmMainMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FileToolStripMenuItem,
            this.ToolsToolStripMenuItem});
            this.frmMainMenu.Location = new System.Drawing.Point(0, 0);
            this.frmMainMenu.Name = "frmMainMenu";
            this.frmMainMenu.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            this.frmMainMenu.Size = new System.Drawing.Size(1002, 24);
            this.frmMainMenu.TabIndex = 0;
            this.frmMainMenu.TabStop = true;
            this.frmMainMenu.Text = "MenuStrip";
            // 
            // label5
            // 
            this.label5.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.label5.AutoSize = true;
            this.label5.ForeColor = System.Drawing.Color.Black;
            this.label5.Location = new System.Drawing.Point(3, 7);
            this.label5.Name = "label5";
            this.label5.Size = new System.Drawing.Size(58, 13);
            this.label5.TabIndex = 0;
            this.label5.Text = "Container:";
            // 
            // Label47
            // 
            this.Label47.AutoSize = true;
            this.Label47.BackColor = System.Drawing.Color.Transparent;
            this.Label47.ForeColor = System.Drawing.Color.Black;
            this.Label47.Location = new System.Drawing.Point(13, 39);
            this.Label47.Name = "Label47";
            this.Label47.Size = new System.Drawing.Size(70, 13);
            this.Label47.TabIndex = 7;
            this.Label47.Text = "Video Codec:";
            // 
            // Label3
            // 
            this.Label3.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.Label3.AutoSize = true;
            this.Label3.ForeColor = System.Drawing.Color.Black;
            this.Label3.Location = new System.Drawing.Point(3, 8);
            this.Label3.Name = "Label3";
            this.Label3.Size = new System.Drawing.Size(27, 13);
            this.Label3.TabIndex = 0;
            this.Label3.Text = "File:";
            // 
            // tab_audio
            // 
            this.tab_audio.BackColor = System.Drawing.Color.Transparent;
            this.tab_audio.Controls.Add(this.AudioSettings);
            this.tab_audio.Location = new System.Drawing.Point(4, 22);
            this.tab_audio.Name = "tab_audio";
            this.tab_audio.Padding = new System.Windows.Forms.Padding(3);
            this.tab_audio.Size = new System.Drawing.Size(724, 308);
            this.tab_audio.TabIndex = 3;
            this.tab_audio.Text = "Audio";
            this.tab_audio.UseVisualStyleBackColor = true;
            // 
            // AudioSettings
            // 
            this.AudioSettings.BackColor = System.Drawing.Color.Transparent;
            this.AudioSettings.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.AudioSettings.Location = new System.Drawing.Point(0, 0);
            this.AudioSettings.Name = "AudioSettings";
            this.AudioSettings.Size = new System.Drawing.Size(715, 310);
            this.AudioSettings.TabIndex = 0;
            // 
            // AudioMenuRowHeightHack
            // 
            this.AudioMenuRowHeightHack.ColorDepth = System.Windows.Forms.ColorDepth.Depth8Bit;
            this.AudioMenuRowHeightHack.ImageSize = new System.Drawing.Size(1, 18);
            this.AudioMenuRowHeightHack.TransparentColor = System.Drawing.Color.Transparent;
            // 
            // tab_video
            // 
            this.tab_video.BackColor = System.Drawing.Color.Transparent;
            this.tab_video.Controls.Add(this.panel1);
            this.tab_video.Controls.Add(this.drp_videoFramerate);
            this.tab_video.Controls.Add(this.radio_cq);
            this.tab_video.Controls.Add(this.radio_avgBitrate);
            this.tab_video.Controls.Add(this.drp_videoEncoder);
            this.tab_video.Controls.Add(this.Label47);
            this.tab_video.Controls.Add(this.label25);
            this.tab_video.Controls.Add(this.check_turbo);
            this.tab_video.Controls.Add(this.check_2PassEncode);
            this.tab_video.Controls.Add(this.Label2);
            this.tab_video.Controls.Add(this.slider_videoQuality);
            this.tab_video.Controls.Add(this.text_bitrate);
            this.tab_video.Controls.Add(this.lbl_SliderValue);
            this.tab_video.Controls.Add(this.lbl_framerate);
            this.tab_video.Location = new System.Drawing.Point(4, 22);
            this.tab_video.Name = "tab_video";
            this.tab_video.Padding = new System.Windows.Forms.Padding(3);
            this.tab_video.Size = new System.Drawing.Size(724, 308);
            this.tab_video.TabIndex = 2;
            this.tab_video.Text = "Video";
            this.tab_video.UseVisualStyleBackColor = true;
            // 
            // panel1
            // 
            this.panel1.Controls.Add(this.radio_constantFramerate);
            this.panel1.Controls.Add(this.radio_peakAndVariable);
            this.panel1.Location = new System.Drawing.Point(125, 92);
            this.panel1.Name = "panel1";
            this.panel1.Size = new System.Drawing.Size(184, 59);
            this.panel1.TabIndex = 20;
            // 
            // radio_constantFramerate
            // 
            this.radio_constantFramerate.AutoSize = true;
            this.radio_constantFramerate.BackColor = System.Drawing.Color.Transparent;
            this.radio_constantFramerate.Checked = true;
            this.radio_constantFramerate.Location = new System.Drawing.Point(0, 0);
            this.radio_constantFramerate.Name = "radio_constantFramerate";
            this.radio_constantFramerate.Size = new System.Drawing.Size(122, 17);
            this.radio_constantFramerate.TabIndex = 17;
            this.radio_constantFramerate.TabStop = true;
            this.radio_constantFramerate.Text = "Constant Framerate";
            this.radio_constantFramerate.UseVisualStyleBackColor = false;
            // 
            // radio_peakAndVariable
            // 
            this.radio_peakAndVariable.AutoSize = true;
            this.radio_peakAndVariable.BackColor = System.Drawing.Color.Transparent;
            this.radio_peakAndVariable.Location = new System.Drawing.Point(0, 23);
            this.radio_peakAndVariable.Name = "radio_peakAndVariable";
            this.radio_peakAndVariable.Size = new System.Drawing.Size(116, 17);
            this.radio_peakAndVariable.TabIndex = 19;
            this.radio_peakAndVariable.Text = "Variable Framerate";
            this.radio_peakAndVariable.UseVisualStyleBackColor = false;
            // 
            // label25
            // 
            this.label25.AutoSize = true;
            this.label25.BackColor = System.Drawing.Color.Transparent;
            this.label25.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label25.Location = new System.Drawing.Point(13, 13);
            this.label25.Name = "label25";
            this.label25.Size = new System.Drawing.Size(38, 13);
            this.label25.TabIndex = 8;
            this.label25.Text = "Video";
            // 
            // Label2
            // 
            this.Label2.AutoSize = true;
            this.Label2.BackColor = System.Drawing.Color.Transparent;
            this.Label2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Label2.Location = new System.Drawing.Point(363, 13);
            this.Label2.Name = "Label2";
            this.Label2.Size = new System.Drawing.Size(47, 13);
            this.Label2.TabIndex = 11;
            this.Label2.Text = "Quality";
            // 
            // lbl_SliderValue
            // 
            this.lbl_SliderValue.AutoSize = true;
            this.lbl_SliderValue.BackColor = System.Drawing.Color.Transparent;
            this.lbl_SliderValue.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_SliderValue.Location = new System.Drawing.Point(487, 39);
            this.lbl_SliderValue.Name = "lbl_SliderValue";
            this.lbl_SliderValue.Size = new System.Drawing.Size(21, 13);
            this.lbl_SliderValue.TabIndex = 15;
            this.lbl_SliderValue.Text = "RF";
            // 
            // lbl_framerate
            // 
            this.lbl_framerate.AutoSize = true;
            this.lbl_framerate.BackColor = System.Drawing.Color.Transparent;
            this.lbl_framerate.Location = new System.Drawing.Point(13, 65);
            this.lbl_framerate.Name = "lbl_framerate";
            this.lbl_framerate.Size = new System.Drawing.Size(90, 13);
            this.lbl_framerate.TabIndex = 16;
            this.lbl_framerate.Text = "Framerate (FPS):";
            // 
            // tab_picture
            // 
            this.tab_picture.BackColor = System.Drawing.Color.Transparent;
            this.tab_picture.Controls.Add(this.PictureSettings);
            this.tab_picture.Location = new System.Drawing.Point(4, 22);
            this.tab_picture.Name = "tab_picture";
            this.tab_picture.Padding = new System.Windows.Forms.Padding(3);
            this.tab_picture.Size = new System.Drawing.Size(724, 308);
            this.tab_picture.TabIndex = 0;
            this.tab_picture.Text = "Picture";
            this.tab_picture.UseVisualStyleBackColor = true;
            // 
            // PictureSettings
            // 
            this.PictureSettings.BackColor = System.Drawing.Color.Transparent;
            this.PictureSettings.CurrentlySelectedPreset = null;
            this.PictureSettings.Enabled = false;
            this.PictureSettings.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.PictureSettings.Location = new System.Drawing.Point(0, 0);
            this.PictureSettings.Name = "PictureSettings";
            this.PictureSettings.PresetMaximumResolution = new System.Drawing.Size(0, 0);
            this.PictureSettings.Size = new System.Drawing.Size(666, 279);
            this.PictureSettings.TabIndex = 0;
            // 
            // Check_ChapterMarkers
            // 
            this.Check_ChapterMarkers.AutoSize = true;
            this.Check_ChapterMarkers.BackColor = System.Drawing.Color.Transparent;
            this.Check_ChapterMarkers.Location = new System.Drawing.Point(16, 32);
            this.Check_ChapterMarkers.Name = "Check_ChapterMarkers";
            this.Check_ChapterMarkers.Size = new System.Drawing.Size(136, 17);
            this.Check_ChapterMarkers.TabIndex = 4;
            this.Check_ChapterMarkers.Text = "Create chapter markers";
            this.Check_ChapterMarkers.UseVisualStyleBackColor = false;
            this.Check_ChapterMarkers.CheckedChanged += new System.EventHandler(this.Check_ChapterMarkers_CheckedChanged);
            // 
            // tabs_panel
            // 
            this.tabs_panel.Controls.Add(this.tab_picture);
            this.tabs_panel.Controls.Add(this.tab_filters);
            this.tabs_panel.Controls.Add(this.tab_video);
            this.tabs_panel.Controls.Add(this.tab_audio);
            this.tabs_panel.Controls.Add(this.tab_subtitles);
            this.tabs_panel.Controls.Add(this.tab_chapters);
            this.tabs_panel.Controls.Add(this.tab_advanced);
            this.tabs_panel.Controls.Add(this.tab_query);
            this.tabs_panel.Location = new System.Drawing.Point(12, 218);
            this.tabs_panel.Name = "tabs_panel";
            this.tabs_panel.SelectedIndex = 0;
            this.tabs_panel.Size = new System.Drawing.Size(732, 334);
            this.tabs_panel.TabIndex = 6;
            // 
            // tab_filters
            // 
            this.tab_filters.Controls.Add(this.Filters);
            this.tab_filters.Location = new System.Drawing.Point(4, 22);
            this.tab_filters.Name = "tab_filters";
            this.tab_filters.Size = new System.Drawing.Size(724, 308);
            this.tab_filters.TabIndex = 1;
            this.tab_filters.Text = "Video Filters";
            this.tab_filters.UseVisualStyleBackColor = true;
            // 
            // Filters
            // 
            this.Filters.BackColor = System.Drawing.Color.Transparent;
            this.Filters.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Filters.Location = new System.Drawing.Point(0, 0);
            this.Filters.Name = "Filters";
            this.Filters.Size = new System.Drawing.Size(713, 310);
            this.Filters.TabIndex = 0;
            // 
            // tab_subtitles
            // 
            this.tab_subtitles.Controls.Add(this.Subtitles);
            this.tab_subtitles.Location = new System.Drawing.Point(4, 22);
            this.tab_subtitles.Name = "tab_subtitles";
            this.tab_subtitles.Padding = new System.Windows.Forms.Padding(3);
            this.tab_subtitles.Size = new System.Drawing.Size(724, 308);
            this.tab_subtitles.TabIndex = 4;
            this.tab_subtitles.Text = "Subtitles";
            this.tab_subtitles.UseVisualStyleBackColor = true;
            // 
            // Subtitles
            // 
            this.Subtitles.BackColor = System.Drawing.Color.Transparent;
            this.Subtitles.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Subtitles.Location = new System.Drawing.Point(0, 0);
            this.Subtitles.Name = "Subtitles";
            this.Subtitles.Size = new System.Drawing.Size(722, 310);
            this.Subtitles.TabIndex = 0;
            // 
            // tab_chapters
            // 
            this.tab_chapters.BackColor = System.Drawing.Color.Transparent;
            this.tab_chapters.Controls.Add(this.btn_export);
            this.tab_chapters.Controls.Add(this.btn_importChapters);
            this.tab_chapters.Controls.Add(this.label31);
            this.tab_chapters.Controls.Add(this.data_chpt);
            this.tab_chapters.Controls.Add(this.Check_ChapterMarkers);
            this.tab_chapters.Location = new System.Drawing.Point(4, 22);
            this.tab_chapters.Name = "tab_chapters";
            this.tab_chapters.Size = new System.Drawing.Size(724, 308);
            this.tab_chapters.TabIndex = 5;
            this.tab_chapters.Text = "Chapters";
            this.tab_chapters.UseVisualStyleBackColor = true;
            // 
            // label31
            // 
            this.label31.AutoSize = true;
            this.label31.BackColor = System.Drawing.Color.Transparent;
            this.label31.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label31.Location = new System.Drawing.Point(13, 13);
            this.label31.Name = "label31";
            this.label31.Size = new System.Drawing.Size(102, 13);
            this.label31.TabIndex = 2;
            this.label31.Text = "Chapter Markers";
            // 
            // tab_advanced
            // 
            this.tab_advanced.BackColor = System.Drawing.Color.Transparent;
            this.tab_advanced.Controls.Add(this.x264Panel);
            this.tab_advanced.Location = new System.Drawing.Point(4, 22);
            this.tab_advanced.Name = "tab_advanced";
            this.tab_advanced.Padding = new System.Windows.Forms.Padding(3);
            this.tab_advanced.Size = new System.Drawing.Size(724, 308);
            this.tab_advanced.TabIndex = 6;
            this.tab_advanced.Text = "Advanced";
            this.tab_advanced.UseVisualStyleBackColor = true;
            // 
            // x264Panel
            // 
            this.x264Panel.BackColor = System.Drawing.Color.Transparent;
            this.x264Panel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.x264Panel.Location = new System.Drawing.Point(0, 0);
            this.x264Panel.Name = "x264Panel";
            this.x264Panel.Size = new System.Drawing.Size(720, 306);
            this.x264Panel.TabIndex = 0;
            this.x264Panel.X264Query = "";
            // 
            // tab_query
            // 
            this.tab_query.Controls.Add(this.btn_clear);
            this.tab_query.Controls.Add(this.label34);
            this.tab_query.Controls.Add(this.btn_generate_Query);
            this.tab_query.Controls.Add(this.label33);
            this.tab_query.Controls.Add(this.rtf_query);
            this.tab_query.Location = new System.Drawing.Point(4, 22);
            this.tab_query.Name = "tab_query";
            this.tab_query.Size = new System.Drawing.Size(724, 308);
            this.tab_query.TabIndex = 7;
            this.tab_query.Text = "Query Editor";
            this.tab_query.UseVisualStyleBackColor = true;
            // 
            // btn_clear
            // 
            this.btn_clear.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_clear.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_clear.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_clear.Location = new System.Drawing.Point(634, 104);
            this.btn_clear.Name = "btn_clear";
            this.btn_clear.Size = new System.Drawing.Size(75, 22);
            this.btn_clear.TabIndex = 0;
            this.btn_clear.Text = "Clear";
            this.btn_clear.UseVisualStyleBackColor = true;
            this.btn_clear.Click += new System.EventHandler(this.btn_clear_Click);
            // 
            // label34
            // 
            this.label34.AutoSize = true;
            this.label34.Location = new System.Drawing.Point(13, 35);
            this.label34.Name = "label34";
            this.label34.Size = new System.Drawing.Size(631, 52);
            this.label34.TabIndex = 1;
            this.label34.Text = resources.GetString("label34.Text");
            // 
            // label33
            // 
            this.label33.AutoSize = true;
            this.label33.BackColor = System.Drawing.Color.Transparent;
            this.label33.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label33.Location = new System.Drawing.Point(13, 13);
            this.label33.Name = "label33";
            this.label33.Size = new System.Drawing.Size(77, 13);
            this.label33.TabIndex = 3;
            this.label33.Text = "Query Editor";
            // 
            // rtf_query
            // 
            this.rtf_query.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.rtf_query.Location = new System.Drawing.Point(16, 132);
            this.rtf_query.Name = "rtf_query";
            this.rtf_query.Size = new System.Drawing.Size(693, 161);
            this.rtf_query.TabIndex = 4;
            this.rtf_query.Text = "";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.splitContainer1);
            this.groupBox2.ForeColor = System.Drawing.Color.Black;
            this.groupBox2.Location = new System.Drawing.Point(750, 70);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(246, 482);
            this.groupBox2.TabIndex = 4;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "Presets";
            // 
            // splitContainer1
            // 
            this.splitContainer1.Dock = System.Windows.Forms.DockStyle.Fill;
            this.splitContainer1.Location = new System.Drawing.Point(3, 17);
            this.splitContainer1.Name = "splitContainer1";
            this.splitContainer1.Orientation = System.Windows.Forms.Orientation.Horizontal;
            // 
            // splitContainer1.Panel1
            // 
            this.splitContainer1.Panel1.Controls.Add(this.treeView_presets);
            // 
            // splitContainer1.Panel2
            // 
            this.splitContainer1.Panel2.Controls.Add(this.presetsToolStrip);
            this.splitContainer1.Size = new System.Drawing.Size(240, 462);
            this.splitContainer1.SplitterDistance = 424;
            this.splitContainer1.TabIndex = 0;
            // 
            // presetsToolStrip
            // 
            this.presetsToolStrip.CanOverflow = false;
            this.presetsToolStrip.Dock = System.Windows.Forms.DockStyle.Fill;
            this.presetsToolStrip.GripMargin = new System.Windows.Forms.Padding(0);
            this.presetsToolStrip.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.presetsToolStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.BtnAddPreset,
            this.BtnRemovePreset,
            this.toolStripDropDownButton2});
            this.presetsToolStrip.Location = new System.Drawing.Point(0, 0);
            this.presetsToolStrip.Name = "presetsToolStrip";
            this.presetsToolStrip.Padding = new System.Windows.Forms.Padding(0);
            this.presetsToolStrip.Size = new System.Drawing.Size(240, 34);
            this.presetsToolStrip.TabIndex = 2;
            this.presetsToolStrip.Text = "toolStrip2";
            // 
            // BtnAddPreset
            // 
            this.BtnAddPreset.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.BtnAddPreset.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.BtnAddPreset.Image = global::Handbrake.Properties.Resources.Add16;
            this.BtnAddPreset.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.BtnAddPreset.Name = "BtnAddPreset";
            this.BtnAddPreset.Overflow = System.Windows.Forms.ToolStripItemOverflow.Never;
            this.BtnAddPreset.Padding = new System.Windows.Forms.Padding(8, 0, 8, 0);
            this.BtnAddPreset.Size = new System.Drawing.Size(65, 31);
            this.BtnAddPreset.Text = "Add";
            this.BtnAddPreset.Click += new System.EventHandler(this.BtnAddPreset_Click);
            // 
            // BtnRemovePreset
            // 
            this.BtnRemovePreset.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.BtnRemovePreset.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.BtnRemovePreset.Image = global::Handbrake.Properties.Resources.Close;
            this.BtnRemovePreset.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.BtnRemovePreset.Name = "BtnRemovePreset";
            this.BtnRemovePreset.Overflow = System.Windows.Forms.ToolStripItemOverflow.Never;
            this.BtnRemovePreset.Padding = new System.Windows.Forms.Padding(0, 0, 8, 0);
            this.BtnRemovePreset.Size = new System.Drawing.Size(82, 31);
            this.BtnRemovePreset.Text = "Remove";
            this.BtnRemovePreset.Click += new System.EventHandler(this.BtnRemovePreset_Click);
            // 
            // toolStripDropDownButton2
            // 
            this.toolStripDropDownButton2.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MnuSetDefaultPreset,
            this.toolStripSeparator3,
            this.MnuImportPreset,
            this.MnuExportPreset,
            this.toolStripSeparator6,
            this.MnuResetBuiltInPresets});
            this.toolStripDropDownButton2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.toolStripDropDownButton2.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.toolStripDropDownButton2.Image = global::Handbrake.Properties.Resources.Options24;
            this.toolStripDropDownButton2.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripDropDownButton2.Name = "toolStripDropDownButton2";
            this.toolStripDropDownButton2.Overflow = System.Windows.Forms.ToolStripItemOverflow.Never;
            this.toolStripDropDownButton2.Size = new System.Drawing.Size(79, 31);
            this.toolStripDropDownButton2.Text = "Options";
            // 
            // MnuSetDefaultPreset
            // 
            this.MnuSetDefaultPreset.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MnuSetDefaultPreset.Name = "MnuSetDefaultPreset";
            this.MnuSetDefaultPreset.Size = new System.Drawing.Size(178, 22);
            this.MnuSetDefaultPreset.Text = "Set Default";
            this.MnuSetDefaultPreset.Click += new System.EventHandler(this.MnuSetDefaultPreset_Click);
            // 
            // toolStripSeparator3
            // 
            this.toolStripSeparator3.Name = "toolStripSeparator3";
            this.toolStripSeparator3.Size = new System.Drawing.Size(175, 6);
            // 
            // MnuImportPreset
            // 
            this.MnuImportPreset.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MnuImportPreset.Name = "MnuImportPreset";
            this.MnuImportPreset.Size = new System.Drawing.Size(178, 22);
            this.MnuImportPreset.Text = "Import";
            this.MnuImportPreset.Click += new System.EventHandler(this.MnuImportPreset_Click);
            // 
            // MnuExportPreset
            // 
            this.MnuExportPreset.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MnuExportPreset.Name = "MnuExportPreset";
            this.MnuExportPreset.Size = new System.Drawing.Size(178, 22);
            this.MnuExportPreset.Text = "Export";
            this.MnuExportPreset.Click += new System.EventHandler(this.MnuExportPreset_Click);
            // 
            // toolStripSeparator6
            // 
            this.toolStripSeparator6.Name = "toolStripSeparator6";
            this.toolStripSeparator6.Size = new System.Drawing.Size(175, 6);
            // 
            // MnuResetBuiltInPresets
            // 
            this.MnuResetBuiltInPresets.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.MnuResetBuiltInPresets.Name = "MnuResetBuiltInPresets";
            this.MnuResetBuiltInPresets.Size = new System.Drawing.Size(178, 22);
            this.MnuResetBuiltInPresets.Text = "Reset Built-In Presets";
            this.MnuResetBuiltInPresets.Click += new System.EventHandler(this.MnuResetBuiltInPresets_Click);
            // 
            // toolStrip1
            // 
            this.toolStrip1.GripStyle = System.Windows.Forms.ToolStripGripStyle.Hidden;
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_source,
            this.toolStripSeparator10,
            this.btn_start,
            this.btn_add2Queue,
            this.btn_showQueue,
            this.toolStripSeparator4,
            this.tb_preview,
            this.btn_ActivityWindow,
            this.toolStripSeparator8,
            this.toolStripDropDownButton1});
            this.toolStrip1.Location = new System.Drawing.Point(0, 24);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            this.toolStrip1.Size = new System.Drawing.Size(1002, 39);
            this.toolStrip1.TabIndex = 1;
            this.toolStrip1.TabStop = true;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // btn_source
            // 
            this.btn_source.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.btn_file_source,
            this.btn_dvd_source,
            this.btnTitleSpecific,
            this.toolStripSeparator1});
            this.btn_source.Image = global::Handbrake.Properties.Resources.Movies;
            this.btn_source.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_source.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_source.Name = "btn_source";
            this.btn_source.Size = new System.Drawing.Size(88, 36);
            this.btn_source.Text = "Source";
            this.btn_source.ToolTipText = "Open a new source file or folder.";
            this.btn_source.Click += new System.EventHandler(this.btn_source_Click);
            // 
            // btn_dvd_source
            // 
            this.btn_dvd_source.Image = global::Handbrake.Properties.Resources.folder;
            this.btn_dvd_source.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_dvd_source.Name = "btn_dvd_source";
            this.btn_dvd_source.ShortcutKeys = ((System.Windows.Forms.Keys)(((System.Windows.Forms.Keys.Control | System.Windows.Forms.Keys.Shift)
                        | System.Windows.Forms.Keys.O)));
            this.btn_dvd_source.Size = new System.Drawing.Size(182, 22);
            this.btn_dvd_source.Text = "Folder";
            this.btn_dvd_source.Click += new System.EventHandler(this.BtnFolderScanClicked);
            // 
            // btnTitleSpecific
            // 
            this.btnTitleSpecific.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.FileTitleSpecificScan,
            this.FolderTitleSpecificScan});
            this.btnTitleSpecific.Name = "btnTitleSpecific";
            this.btnTitleSpecific.Size = new System.Drawing.Size(182, 22);
            this.btnTitleSpecific.Text = "Title Specific Scan";
            // 
            // FileTitleSpecificScan
            // 
            this.FileTitleSpecificScan.Image = global::Handbrake.Properties.Resources.Movies_Small;
            this.FileTitleSpecificScan.Name = "FileTitleSpecificScan";
            this.FileTitleSpecificScan.Size = new System.Drawing.Size(125, 22);
            this.FileTitleSpecificScan.Text = "Video File";
            this.FileTitleSpecificScan.Click += new System.EventHandler(this.VideoTitleSpecificScanClick);
            // 
            // FolderTitleSpecificScan
            // 
            this.FolderTitleSpecificScan.Image = global::Handbrake.Properties.Resources.folder;
            this.FolderTitleSpecificScan.Name = "FolderTitleSpecificScan";
            this.FolderTitleSpecificScan.Size = new System.Drawing.Size(125, 22);
            this.FolderTitleSpecificScan.Text = "Folder";
            this.FolderTitleSpecificScan.Click += new System.EventHandler(this.FolderTitleSpecificScanClick);
            // 
            // toolStripSeparator1
            // 
            this.toolStripSeparator1.Name = "toolStripSeparator1";
            this.toolStripSeparator1.Size = new System.Drawing.Size(179, 6);
            // 
            // toolStripSeparator10
            // 
            this.toolStripSeparator10.Name = "toolStripSeparator10";
            this.toolStripSeparator10.Size = new System.Drawing.Size(6, 39);
            // 
            // btn_start
            // 
            this.btn_start.Image = global::Handbrake.Properties.Resources.Play;
            this.btn_start.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_start.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_start.Name = "btn_start";
            this.btn_start.Size = new System.Drawing.Size(67, 36);
            this.btn_start.Text = "Start";
            this.btn_start.ToolTipText = "Start the encoding process";
            this.btn_start.Click += new System.EventHandler(this.btn_start_Click);
            // 
            // btn_add2Queue
            // 
            this.btn_add2Queue.Image = global::Handbrake.Properties.Resources.AddToQueue;
            this.btn_add2Queue.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_add2Queue.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_add2Queue.Name = "btn_add2Queue";
            this.btn_add2Queue.Size = new System.Drawing.Size(117, 36);
            this.btn_add2Queue.Text = "Add to Queue";
            this.btn_add2Queue.ToolTipText = "Add a new item to the Queue";
            this.btn_add2Queue.Click += new System.EventHandler(this.btn_add2Queue_Click);
            // 
            // btn_showQueue
            // 
            this.btn_showQueue.Image = global::Handbrake.Properties.Resources.Queue;
            this.btn_showQueue.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_showQueue.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_showQueue.Name = "btn_showQueue";
            this.btn_showQueue.Size = new System.Drawing.Size(110, 36);
            this.btn_showQueue.Tag = "";
            this.btn_showQueue.Text = "Show Queue";
            this.btn_showQueue.Click += new System.EventHandler(this.btn_showQueue_Click);
            // 
            // toolStripSeparator4
            // 
            this.toolStripSeparator4.Name = "toolStripSeparator4";
            this.toolStripSeparator4.Size = new System.Drawing.Size(6, 39);
            // 
            // tb_preview
            // 
            this.tb_preview.Image = global::Handbrake.Properties.Resources.window;
            this.tb_preview.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.tb_preview.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.tb_preview.Name = "tb_preview";
            this.tb_preview.Size = new System.Drawing.Size(84, 36);
            this.tb_preview.Text = "Preview";
            this.tb_preview.Click += new System.EventHandler(this.tb_preview_Click);
            // 
            // btn_ActivityWindow
            // 
            this.btn_ActivityWindow.Image = global::Handbrake.Properties.Resources.ActivityWindow;
            this.btn_ActivityWindow.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.btn_ActivityWindow.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.btn_ActivityWindow.Name = "btn_ActivityWindow";
            this.btn_ActivityWindow.Size = new System.Drawing.Size(130, 36);
            this.btn_ActivityWindow.Text = "Activity Window";
            this.btn_ActivityWindow.ToolTipText = "Displays the activity window which displays the log of the last completed or curr" +
                "ently running encode.";
            this.btn_ActivityWindow.Click += new System.EventHandler(this.btn_ActivityWindow_Click);
            // 
            // toolStripSeparator8
            // 
            this.toolStripSeparator8.Name = "toolStripSeparator8";
            this.toolStripSeparator8.Size = new System.Drawing.Size(6, 39);
            // 
            // toolStripDropDownButton1
            // 
            this.toolStripDropDownButton1.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.toolStripDropDownButton1.AutoSize = false;
            this.toolStripDropDownButton1.DropDownItems.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.MnuUserGuide,
            this.toolStripSeparator9,
            this.MnuCheckForUpdates,
            this.toolStripSeparator11,
            this.MnuAboutHandBrake});
            this.toolStripDropDownButton1.Image = global::Handbrake.Properties.Resources.Help24;
            this.toolStripDropDownButton1.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.toolStripDropDownButton1.ImageTransparentColor = System.Drawing.Color.Magenta;
            this.toolStripDropDownButton1.Name = "toolStripDropDownButton1";
            this.toolStripDropDownButton1.Size = new System.Drawing.Size(69, 36);
            this.toolStripDropDownButton1.Text = "Help";
            // 
            // MnuUserGuide
            // 
            this.MnuUserGuide.Image = global::Handbrake.Properties.Resources.info16;
            this.MnuUserGuide.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.MnuUserGuide.Name = "MnuUserGuide";
            this.MnuUserGuide.Size = new System.Drawing.Size(192, 24);
            this.MnuUserGuide.Text = "HandBrake User Guide";
            this.MnuUserGuide.Click += new System.EventHandler(this.MnuUserGuide_Click);
            // 
            // toolStripSeparator9
            // 
            this.toolStripSeparator9.Name = "toolStripSeparator9";
            this.toolStripSeparator9.Size = new System.Drawing.Size(189, 6);
            // 
            // MnuCheckForUpdates
            // 
            this.MnuCheckForUpdates.Name = "MnuCheckForUpdates";
            this.MnuCheckForUpdates.Size = new System.Drawing.Size(192, 24);
            this.MnuCheckForUpdates.Text = "Check for Updates";
            this.MnuCheckForUpdates.Click += new System.EventHandler(this.MnuCheckForUpdates_Click);
            // 
            // toolStripSeparator11
            // 
            this.toolStripSeparator11.Name = "toolStripSeparator11";
            this.toolStripSeparator11.Size = new System.Drawing.Size(189, 6);
            // 
            // MnuAboutHandBrake
            // 
            this.MnuAboutHandBrake.Image = global::Handbrake.Properties.Resources.hb16;
            this.MnuAboutHandBrake.ImageScaling = System.Windows.Forms.ToolStripItemImageScaling.None;
            this.MnuAboutHandBrake.Name = "MnuAboutHandBrake";
            this.MnuAboutHandBrake.Size = new System.Drawing.Size(192, 24);
            this.MnuAboutHandBrake.Text = "About HandBrake";
            this.MnuAboutHandBrake.Click += new System.EventHandler(this.MnuAboutHandBrake_Click);
            // 
            // notifyIcon
            // 
            this.notifyIcon.BalloonTipIcon = System.Windows.Forms.ToolTipIcon.Info;
            this.notifyIcon.BalloonTipText = "HandBrake Status Here";
            this.notifyIcon.BalloonTipTitle = "HandBrake";
            this.notifyIcon.ContextMenuStrip = notifyIconMenu;
            this.notifyIcon.Icon = ((System.Drawing.Icon)(resources.GetObject("notifyIcon.Icon")));
            this.notifyIcon.Text = "HandBrake";
            this.notifyIcon.MouseDoubleClick += new System.Windows.Forms.MouseEventHandler(this.notifyIcon_MouseDoubleClick);
            // 
            // StatusStrip
            // 
            this.StatusStrip.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.ProgressBarStatus,
            this.lbl_encode,
            this.lbl_updateCheck});
            this.StatusStrip.Location = new System.Drawing.Point(0, 561);
            this.StatusStrip.Name = "StatusStrip";
            this.StatusStrip.RenderMode = System.Windows.Forms.ToolStripRenderMode.Professional;
            this.StatusStrip.Size = new System.Drawing.Size(1002, 22);
            this.StatusStrip.SizingGrip = false;
            this.StatusStrip.TabIndex = 7;
            this.StatusStrip.Text = "statusStrip1";
            // 
            // ProgressBarStatus
            // 
            this.ProgressBarStatus.Alignment = System.Windows.Forms.ToolStripItemAlignment.Right;
            this.ProgressBarStatus.Name = "ProgressBarStatus";
            this.ProgressBarStatus.Size = new System.Drawing.Size(100, 16);
            this.ProgressBarStatus.Visible = false;
            // 
            // lbl_encode
            // 
            this.lbl_encode.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_encode.Name = "lbl_encode";
            this.lbl_encode.Size = new System.Drawing.Size(28, 17);
            this.lbl_encode.Text = "{0}";
            // 
            // lbl_updateCheck
            // 
            this.lbl_updateCheck.BackColor = System.Drawing.Color.Transparent;
            this.lbl_updateCheck.DisplayStyle = System.Windows.Forms.ToolStripItemDisplayStyle.Text;
            this.lbl_updateCheck.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_updateCheck.Name = "lbl_updateCheck";
            this.lbl_updateCheck.Size = new System.Drawing.Size(139, 17);
            this.lbl_updateCheck.Text = "Checking for Updates ...";
            this.lbl_updateCheck.Visible = false;
            // 
            // hbproc
            // 
            this.hbproc.StartInfo.Domain = "";
            this.hbproc.StartInfo.LoadUserProfile = false;
            this.hbproc.StartInfo.Password = null;
            this.hbproc.StartInfo.StandardErrorEncoding = null;
            this.hbproc.StartInfo.StandardOutputEncoding = null;
            this.hbproc.StartInfo.UserName = "";
            this.hbproc.SynchronizingObject = this;
            // 
            // File_Save
            // 
            this.File_Save.DefaultExt = "hb";
            this.File_Save.Filter = "hb|*.hb";
            // 
            // tableLayoutPanel2
            // 
            this.tableLayoutPanel2.AutoSize = true;
            this.tableLayoutPanel2.ColumnCount = 3;
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel2.Controls.Add(this.Label3, 0, 0);
            this.tableLayoutPanel2.Controls.Add(this.text_destination, 1, 0);
            this.tableLayoutPanel2.Controls.Add(this.btn_destBrowse, 2, 0);
            this.tableLayoutPanel2.Location = new System.Drawing.Point(21, 132);
            this.tableLayoutPanel2.Name = "tableLayoutPanel2";
            this.tableLayoutPanel2.RowCount = 1;
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel2.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 29F));
            this.tableLayoutPanel2.Size = new System.Drawing.Size(723, 29);
            this.tableLayoutPanel2.TabIndex = 3;
            // 
            // btn_destBrowse
            // 
            this.btn_destBrowse.AutoSize = true;
            this.btn_destBrowse.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_destBrowse.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_destBrowse.Location = new System.Drawing.Point(645, 3);
            this.btn_destBrowse.Name = "btn_destBrowse";
            this.btn_destBrowse.Size = new System.Drawing.Size(75, 23);
            this.btn_destBrowse.TabIndex = 2;
            this.btn_destBrowse.Text = "Browse";
            this.btn_destBrowse.UseVisualStyleBackColor = true;
            this.btn_destBrowse.Click += new System.EventHandler(this.btn_destBrowse_Click);
            // 
            // tableLayoutPanel3
            // 
            this.tableLayoutPanel3.AutoSize = true;
            this.tableLayoutPanel3.ColumnCount = 5;
            this.tableLayoutPanel3.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel3.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel3.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel3.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel3.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel3.Controls.Add(this.label5, 0, 0);
            this.tableLayoutPanel3.Controls.Add(this.drop_format, 1, 0);
            this.tableLayoutPanel3.Controls.Add(this.check_largeFile, 2, 0);
            this.tableLayoutPanel3.Controls.Add(this.check_optimiseMP4, 3, 0);
            this.tableLayoutPanel3.Controls.Add(this.check_iPodAtom, 4, 0);
            this.tableLayoutPanel3.Location = new System.Drawing.Point(21, 180);
            this.tableLayoutPanel3.Name = "tableLayoutPanel3";
            this.tableLayoutPanel3.RowCount = 1;
            this.tableLayoutPanel3.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel3.Size = new System.Drawing.Size(723, 27);
            this.tableLayoutPanel3.TabIndex = 5;
            // 
            // tableLayoutPanel1
            // 
            this.tableLayoutPanel1.AutoSize = true;
            this.tableLayoutPanel1.ColumnCount = 12;
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 35F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 5F));
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tableLayoutPanel1.Controls.Add(this.Label10, 0, 0);
            this.tableLayoutPanel1.Controls.Add(this.drp_dvdtitle, 1, 0);
            this.tableLayoutPanel1.Controls.Add(this.lbl_angle, 2, 0);
            this.tableLayoutPanel1.Controls.Add(this.drop_angle, 3, 0);
            this.tableLayoutPanel1.Controls.Add(this.drop_chapterStart, 7, 0);
            this.tableLayoutPanel1.Controls.Add(this.Label13, 8, 0);
            this.tableLayoutPanel1.Controls.Add(this.drop_chapterFinish, 9, 0);
            this.tableLayoutPanel1.Controls.Add(this.label_duration, 10, 0);
            this.tableLayoutPanel1.Controls.Add(this.lbl_duration, 11, 0);
            this.tableLayoutPanel1.Controls.Add(this.drop_mode, 5, 0);
            this.tableLayoutPanel1.Location = new System.Drawing.Point(21, 86);
            this.tableLayoutPanel1.Name = "tableLayoutPanel1";
            this.tableLayoutPanel1.RowCount = 1;
            this.tableLayoutPanel1.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.tableLayoutPanel1.Size = new System.Drawing.Size(723, 27);
            this.tableLayoutPanel1.TabIndex = 2;
            // 
            // Label10
            // 
            this.Label10.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.Label10.AutoSize = true;
            this.Label10.ForeColor = System.Drawing.Color.Black;
            this.Label10.Location = new System.Drawing.Point(3, 7);
            this.Label10.Name = "Label10";
            this.Label10.Size = new System.Drawing.Size(31, 13);
            this.Label10.TabIndex = 0;
            this.Label10.Text = "Title:";
            // 
            // lbl_angle
            // 
            this.lbl_angle.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.lbl_angle.AutoSize = true;
            this.lbl_angle.ForeColor = System.Drawing.Color.Black;
            this.lbl_angle.Location = new System.Drawing.Point(165, 7);
            this.lbl_angle.Name = "lbl_angle";
            this.lbl_angle.Size = new System.Drawing.Size(38, 13);
            this.lbl_angle.TabIndex = 2;
            this.lbl_angle.Text = "Angle:";
            // 
            // Label13
            // 
            this.Label13.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.Label13.AutoSize = true;
            this.Label13.Location = new System.Drawing.Point(458, 7);
            this.Label13.Name = "Label13";
            this.Label13.Size = new System.Drawing.Size(45, 13);
            this.Label13.TabIndex = 6;
            this.Label13.Text = "through";
            // 
            // label_duration
            // 
            this.label_duration.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Left | System.Windows.Forms.AnchorStyles.Right)));
            this.label_duration.AutoSize = true;
            this.label_duration.BackColor = System.Drawing.Color.Transparent;
            this.label_duration.Location = new System.Drawing.Point(584, 7);
            this.label_duration.Name = "label_duration";
            this.label_duration.Size = new System.Drawing.Size(52, 13);
            this.label_duration.TabIndex = 8;
            this.label_duration.Text = "Duration:";
            // 
            // lbl_duration
            // 
            this.lbl_duration.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_duration.AutoSize = true;
            this.lbl_duration.BackColor = System.Drawing.Color.Transparent;
            this.lbl_duration.Location = new System.Drawing.Point(642, 7);
            this.lbl_duration.Name = "lbl_duration";
            this.lbl_duration.Size = new System.Drawing.Size(39, 13);
            this.lbl_duration.TabIndex = 9;
            this.lbl_duration.Text = "--:--:--";
            // 
            // labelStaticDestination
            // 
            this.labelStaticDestination.AutoSize = true;
            this.labelStaticDestination.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelStaticDestination.Location = new System.Drawing.Point(9, 116);
            this.labelStaticDestination.Name = "labelStaticDestination";
            this.labelStaticDestination.Size = new System.Drawing.Size(75, 13);
            this.labelStaticDestination.TabIndex = 3;
            this.labelStaticDestination.Text = "Destination:";
            // 
            // labelPreset
            // 
            this.labelPreset.AutoSize = true;
            this.labelPreset.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelPreset.Location = new System.Drawing.Point(9, 164);
            this.labelPreset.Name = "labelPreset";
            this.labelPreset.Size = new System.Drawing.Size(180, 13);
            this.labelPreset.TabIndex = 5;
            this.labelPreset.Text = "Output Settings (Preset: None)";
            // 
            // labelSource
            // 
            this.labelSource.AutoSize = true;
            this.labelSource.Location = new System.Drawing.Point(55, 0);
            this.labelSource.Name = "labelSource";
            this.labelSource.Size = new System.Drawing.Size(137, 13);
            this.labelSource.TabIndex = 1;
            this.labelSource.Text = "Select \"Source\" to continue";
            // 
            // labelStaticSource
            // 
            this.labelStaticSource.AutoSize = true;
            this.labelStaticSource.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.labelStaticSource.Location = new System.Drawing.Point(0, 0);
            this.labelStaticSource.Margin = new System.Windows.Forms.Padding(0, 0, 3, 0);
            this.labelStaticSource.Name = "labelStaticSource";
            this.labelStaticSource.Size = new System.Drawing.Size(49, 13);
            this.labelStaticSource.TabIndex = 0;
            this.labelStaticSource.Text = "Source:";
            // 
            // SourceLayoutPanel
            // 
            this.SourceLayoutPanel.AutoSize = true;
            this.SourceLayoutPanel.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.SourceLayoutPanel.Controls.Add(this.labelStaticSource);
            this.SourceLayoutPanel.Controls.Add(this.labelSource);
            this.SourceLayoutPanel.Location = new System.Drawing.Point(9, 70);
            this.SourceLayoutPanel.Margin = new System.Windows.Forms.Padding(0);
            this.SourceLayoutPanel.Name = "SourceLayoutPanel";
            this.SourceLayoutPanel.Size = new System.Drawing.Size(195, 13);
            this.SourceLayoutPanel.TabIndex = 2;
            // 
            // openPreset
            // 
            this.openPreset.DefaultExt = "plist";
            this.openPreset.Filter = "Plist Files|*.plist";
            // 
            // File_ChapterImport
            // 
            this.File_ChapterImport.Filter = "CSV Files|*.csv";
            // 
            // frmMain
            // 
            this.AllowDrop = true;
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoSize = true;
            this.ClientSize = new System.Drawing.Size(1002, 583);
            this.Controls.Add(this.tableLayoutPanel3);
            this.Controls.Add(this.toolStrip1);
            this.Controls.Add(this.SourceLayoutPanel);
            this.Controls.Add(this.frmMainMenu);
            this.Controls.Add(this.tableLayoutPanel2);
            this.Controls.Add(this.labelPreset);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.StatusStrip);
            this.Controls.Add(this.tableLayoutPanel1);
            this.Controls.Add(this.tabs_panel);
            this.Controls.Add(this.labelStaticDestination);
            this.DoubleBuffered = true;
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.KeyPreview = true;
            this.MinimumSize = new System.Drawing.Size(900, 500);
            this.Name = "frmMain";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "HandBrake";
            notifyIconMenu.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.slider_videoQuality)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.data_chpt)).EndInit();
            this.ChaptersMenu.ResumeLayout(false);
            this.presets_menu.ResumeLayout(false);
            this.frmMainMenu.ResumeLayout(false);
            this.frmMainMenu.PerformLayout();
            this.tab_audio.ResumeLayout(false);
            this.tab_video.ResumeLayout(false);
            this.tab_video.PerformLayout();
            this.panel1.ResumeLayout(false);
            this.panel1.PerformLayout();
            this.tab_picture.ResumeLayout(false);
            this.tabs_panel.ResumeLayout(false);
            this.tab_filters.ResumeLayout(false);
            this.tab_subtitles.ResumeLayout(false);
            this.tab_chapters.ResumeLayout(false);
            this.tab_chapters.PerformLayout();
            this.tab_advanced.ResumeLayout(false);
            this.tab_query.ResumeLayout(false);
            this.tab_query.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.splitContainer1.Panel1.ResumeLayout(false);
            this.splitContainer1.Panel2.ResumeLayout(false);
            this.splitContainer1.Panel2.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.splitContainer1)).EndInit();
            this.splitContainer1.ResumeLayout(false);
            this.presetsToolStrip.ResumeLayout(false);
            this.presetsToolStrip.PerformLayout();
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.StatusStrip.ResumeLayout(false);
            this.StatusStrip.PerformLayout();
            this.tableLayoutPanel2.ResumeLayout(false);
            this.tableLayoutPanel2.PerformLayout();
            this.tableLayoutPanel3.ResumeLayout(false);
            this.tableLayoutPanel3.PerformLayout();
            this.tableLayoutPanel1.ResumeLayout(false);
            this.tableLayoutPanel1.PerformLayout();
            this.SourceLayoutPanel.ResumeLayout(false);
            this.SourceLayoutPanel.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        internal System.Windows.Forms.ToolTip ToolTip;
        internal System.Windows.Forms.ToolStripMenuItem FileToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_open3;
        internal System.Windows.Forms.ToolStripMenuItem mnu_exit;
        internal System.Windows.Forms.ToolStripMenuItem ToolsToolStripMenuItem;
        internal System.Windows.Forms.ToolStripMenuItem mnu_encode;
        internal System.Windows.Forms.ToolStripSeparator ToolStripSeparator5;
        internal System.Windows.Forms.ToolStripMenuItem mnu_options;
        internal System.Windows.Forms.MenuStrip frmMainMenu;
        internal System.Windows.Forms.Label Label3;
        internal System.Windows.Forms.ComboBox drp_videoEncoder;
        internal System.Windows.Forms.Label Label47;
        internal System.Windows.Forms.TextBox text_destination;
        internal System.Windows.Forms.TabPage tab_audio;
        internal System.Windows.Forms.TabPage tab_video;
        internal System.Windows.Forms.CheckBox check_largeFile;
        internal System.Windows.Forms.CheckBox check_turbo;
        internal System.Windows.Forms.Label Label2;
        internal System.Windows.Forms.Label lbl_SliderValue;
        internal System.Windows.Forms.ComboBox drp_videoFramerate;
        internal System.Windows.Forms.CheckBox check_2PassEncode;
        internal System.Windows.Forms.TrackBar slider_videoQuality;
        internal System.Windows.Forms.TextBox text_bitrate;
        internal System.Windows.Forms.TabPage tab_picture;
        internal System.Windows.Forms.CheckBox Check_ChapterMarkers;
        internal System.Windows.Forms.TabControl tabs_panel;
        internal System.Windows.Forms.Label lbl_framerate;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.SaveFileDialog DVD_Save;
        private System.Windows.Forms.OpenFileDialog File_Open;
        internal System.Windows.Forms.CheckBox check_iPodAtom;
        private System.Windows.Forms.TabPage tab_chapters;
        internal System.Windows.Forms.Label label31;
        internal System.Windows.Forms.CheckBox check_optimiseMP4;
        internal System.Windows.Forms.DataGridView data_chpt;
        private System.Windows.Forms.TabPage tab_query;
        private System.Windows.Forms.Label label34;
        internal System.Windows.Forms.Button btn_generate_Query;
        internal System.Windows.Forms.Label label33;
        internal System.Windows.Forms.Button btn_clear;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripButton btn_start;
        private System.Windows.Forms.ToolStripButton btn_add2Queue;
        private System.Windows.Forms.ToolStripButton btn_showQueue;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator4;
        private System.Windows.Forms.ToolStripButton btn_ActivityWindow;
        internal System.Windows.Forms.Label label25;
        internal System.Windows.Forms.TabPage tab_advanced;
        internal System.Windows.Forms.TreeView treeView_presets;
        internal System.Windows.Forms.RichTextBox rtf_query;
        private System.Windows.Forms.NotifyIcon notifyIcon;
        private System.Windows.Forms.ToolStripMenuItem btn_restore;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator10;
        private System.Windows.Forms.ToolStripMenuItem btn_file_source;
        private System.Windows.Forms.ToolStripDropDownButton btn_source;
        private System.Windows.Forms.ToolStripSeparator toolStripSeparator1;
        private System.Windows.Forms.ToolStripMenuItem btn_dvd_source;
        internal System.Windows.Forms.ComboBox drop_format;
        internal System.Windows.Forms.Label label5;
        internal System.Windows.Forms.ToolStripMenuItem mnu_encodeLog;
        private System.Windows.Forms.StatusStrip StatusStrip;
        private System.Windows.Forms.ToolStripStatusLabel lbl_encode;
        internal System.Windows.Forms.OpenFileDialog ISO_Open;
        internal System.Windows.Forms.FolderBrowserDialog DVD_Open;
        private System.Windows.Forms.ContextMenuStrip presets_menu;
        private System.Windows.Forms.ToolStripMenuItem pmnu_expandAll;
        private System.Windows.Forms.ToolStripMenuItem pmnu_collapse;
        private System.Windows.Forms.ToolStripSeparator sep1;
        private System.Windows.Forms.ToolStripMenuItem pmnu_delete;
        private System.Windows.Forms.SplitContainer splitContainer1;
        private System.Windows.Forms.ImageList AudioMenuRowHeightHack;
        private System.Windows.Forms.ToolStripMenuItem pmnu_saveChanges;
        private System.Windows.Forms.ToolStripMenuItem mnu_killCLI;
        private System.Windows.Forms.TabPage tab_filters;
        internal System.Windows.Forms.RadioButton radio_cq;
        internal System.Windows.Forms.RadioButton radio_avgBitrate;
        internal Handbrake.Controls.x264Panel x264Panel;
        private System.Windows.Forms.ToolStripButton tb_preview;
        private System.Diagnostics.Process hbproc;
        private TabPage tab_subtitles;
        internal Handbrake.Controls.AudioPanel AudioSettings;
        internal Handbrake.Controls.Subtitles Subtitles;
        internal Handbrake.Controls.Filters Filters;
        private ToolStripStatusLabel lbl_updateCheck;
        internal SaveFileDialog File_Save;
        private TableLayoutPanel tableLayoutPanel2;
        private Button btn_destBrowse;
        private TableLayoutPanel tableLayoutPanel3;
        private TableLayoutPanel tableLayoutPanel1;
        internal Label lbl_angle;
        internal ComboBox drop_angle;
        internal ComboBox drop_chapterStart;
        internal Label Label13;
        internal ComboBox drop_chapterFinish;
        internal Label label_duration;
        internal Label lbl_duration;
        private Label labelStaticDestination;
        internal Label labelPreset;
        internal Handbrake.Controls.PictureSettings PictureSettings;
        private Label labelSource;
        internal ComboBox drp_dvdtitle;
        internal Label Label10;
        private Label labelStaticSource;
        private FlowLayoutPanel SourceLayoutPanel;
        private OpenFileDialog openPreset;
        private Button btn_importChapters;
        private OpenFileDialog File_ChapterImport;
        private ContextMenuStrip ChaptersMenu;
        private ToolStripMenuItem mnu_resetChapters;
        private ToolStripMenuItem pmnu_import;
        private ToolStripSeparator toolStripSeparator2;
        internal ComboBox drop_mode;
        private Button btn_export;
        private DataGridViewTextBoxColumn number;
        private DataGridViewTextBoxColumn name;
        private ToolStripProgressBar ProgressBarStatus;
        private ToolStripMenuItem btnTitleSpecific;
        private ToolStripMenuItem FileTitleSpecificScan;
        private ToolStripMenuItem FolderTitleSpecificScan;
        private ToolStripSeparator toolStripSeparator8;
        private ToolStripDropDownButton toolStripDropDownButton1;
        private ToolStripMenuItem MnuUserGuide;
        private ToolStripSeparator toolStripSeparator9;
        private ToolStripMenuItem MnuCheckForUpdates;
        private ToolStripSeparator toolStripSeparator11;
        private ToolStripMenuItem MnuAboutHandBrake;
        private ToolStrip presetsToolStrip;
        private ToolStripButton BtnAddPreset;
        private ToolStripButton BtnRemovePreset;
        private ToolStripDropDownButton toolStripDropDownButton2;
        private ToolStripMenuItem MnuSetDefaultPreset;
        private ToolStripSeparator toolStripSeparator3;
        private ToolStripMenuItem MnuImportPreset;
        private ToolStripMenuItem MnuExportPreset;
        private ToolStripSeparator toolStripSeparator6;
        private ToolStripMenuItem MnuResetBuiltInPresets;
        internal RadioButton radio_peakAndVariable;
        internal RadioButton radio_constantFramerate;
        private Panel panel1;
    }
}