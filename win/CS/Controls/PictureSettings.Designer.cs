namespace Handbrake.Controls
{
    partial class PictureSettings
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
            this.tbl_cropping = new System.Windows.Forms.TableLayoutPanel();
            this.lbl_cropLeft = new System.Windows.Forms.Label();
            this.crop_left = new System.Windows.Forms.NumericUpDown();
            this.crop_right = new System.Windows.Forms.NumericUpDown();
            this.check_autoCrop = new System.Windows.Forms.RadioButton();
            this.lbl_cropRight = new System.Windows.Forms.Label();
            this.crop_top = new System.Windows.Forms.NumericUpDown();
            this.lbl_cropBottom = new System.Windows.Forms.Label();
            this.lbl_cropTop = new System.Windows.Forms.Label();
            this.crop_bottom = new System.Windows.Forms.NumericUpDown();
            this.check_customCrop = new System.Windows.Forms.RadioButton();
            this.tbl_size = new System.Windows.Forms.TableLayoutPanel();
            this.lbl_src = new System.Windows.Forms.Label();
            this.lbl_src_res = new System.Windows.Forms.Label();
            this.lbl_width = new System.Windows.Forms.Label();
            this.text_width = new System.Windows.Forms.NumericUpDown();
            this.lbl_height = new System.Windows.Forms.Label();
            this.text_height = new System.Windows.Forms.NumericUpDown();
            this.check_KeepAR = new System.Windows.Forms.CheckBox();
            this.check_UseITUPar = new System.Windows.Forms.CheckBox();
            this.lbl_useITUPar = new System.Windows.Forms.Label();
            this.tbl_anamorphic = new System.Windows.Forms.TableLayoutPanel();
            this.drp_colormatrix = new System.Windows.Forms.ComboBox();
            this.lbl_colormatrix = new System.Windows.Forms.Label();
            this.updownParHeight = new System.Windows.Forms.NumericUpDown();
            this.lbl_anamorphic = new System.Windows.Forms.Label();
            this.labelDisplaySize = new System.Windows.Forms.Label();
            this.lbl_parHeight = new System.Windows.Forms.Label();
            this.labelStaticDisplaySize = new System.Windows.Forms.Label();
            this.updownParWidth = new System.Windows.Forms.NumericUpDown();
            this.drp_anamorphic = new System.Windows.Forms.ComboBox();
            this.lbl_parWidth = new System.Windows.Forms.Label();
            this.updownDisplayWidth = new System.Windows.Forms.NumericUpDown();
            this.lbl_modulus = new System.Windows.Forms.Label();
            this.lbl_displayWidth = new System.Windows.Forms.Label();
            this.drp_modulus = new System.Windows.Forms.ComboBox();
            this.lbl_cropping = new System.Windows.Forms.Label();
            this.lbl_size = new System.Windows.Forms.Label();
            this.lbl_presetCropWarning = new System.Windows.Forms.Label();
            this.lbl_padding = new System.Windows.Forms.Label();
            this.tbl_padding = new System.Windows.Forms.TableLayoutPanel();
            this.pad_right = new System.Windows.Forms.NumericUpDown();
            this.check_disablePad = new System.Windows.Forms.RadioButton();
            this.lbl_padLeft = new System.Windows.Forms.Label();
            this.pad_left = new System.Windows.Forms.NumericUpDown();
            this.lbl_padRight = new System.Windows.Forms.Label();
            this.check_enablePad = new System.Windows.Forms.RadioButton();
            this.lbl_padTop = new System.Windows.Forms.Label();
            this.pad_top = new System.Windows.Forms.NumericUpDown();
            this.pad_bottom = new System.Windows.Forms.NumericUpDown();
            this.lbl_padBottom = new System.Windows.Forms.Label();
            this.check_disableCrop = new System.Windows.Forms.RadioButton();
            this.tbl_cropping.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.crop_left)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_right)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_top)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_bottom)).BeginInit();
            this.tbl_size.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.text_width)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.text_height)).BeginInit();
            this.tbl_anamorphic.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.updownParHeight)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.updownParWidth)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.updownDisplayWidth)).BeginInit();
            this.tbl_padding.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pad_right)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pad_left)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pad_top)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.pad_bottom)).BeginInit();
            this.SuspendLayout();
            // 
            // tbl_cropping
            // 
            this.tbl_cropping.AutoSize = true;
            this.tbl_cropping.ColumnCount = 6;
            this.tbl_cropping.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 96F));
            this.tbl_cropping.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_cropping.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_cropping.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_cropping.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_cropping.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_cropping.Controls.Add(this.lbl_cropLeft, 1, 2);
            this.tbl_cropping.Controls.Add(this.crop_left, 2, 2);
            this.tbl_cropping.Controls.Add(this.crop_right, 4, 2);
            this.tbl_cropping.Controls.Add(this.check_autoCrop, 0, 0);
            this.tbl_cropping.Controls.Add(this.lbl_cropRight, 5, 2);
            this.tbl_cropping.Controls.Add(this.crop_top, 3, 1);
            this.tbl_cropping.Controls.Add(this.lbl_cropBottom, 3, 4);
            this.tbl_cropping.Controls.Add(this.lbl_cropTop, 3, 0);
            this.tbl_cropping.Controls.Add(this.crop_bottom, 3, 3);
            this.tbl_cropping.Controls.Add(this.check_customCrop, 0, 1);
            this.tbl_cropping.Controls.Add(this.check_disableCrop, 0, 2);
            this.tbl_cropping.Location = new System.Drawing.Point(338, 35);
            this.tbl_cropping.Name = "tbl_cropping";
            this.tbl_cropping.RowCount = 5;
            this.tbl_cropping.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_cropping.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_cropping.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_cropping.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_cropping.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_cropping.Size = new System.Drawing.Size(313, 123);
            this.tbl_cropping.TabIndex = 117;
            // 
            // lbl_cropLeft
            // 
            this.lbl_cropLeft.Anchor = System.Windows.Forms.AnchorStyles.Right;
            this.lbl_cropLeft.AutoSize = true;
            this.lbl_cropLeft.BackColor = System.Drawing.Color.Transparent;
            this.lbl_cropLeft.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_cropLeft.Location = new System.Drawing.Point(96, 57);
            this.lbl_cropLeft.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lbl_cropLeft.Name = "lbl_cropLeft";
            this.lbl_cropLeft.Size = new System.Drawing.Size(26, 13);
            this.lbl_cropLeft.TabIndex = 97;
            this.lbl_cropLeft.Text = "Left";
            // 
            // crop_left
            // 
            this.crop_left.Enabled = false;
            this.crop_left.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.crop_left.Location = new System.Drawing.Point(128, 53);
            this.crop_left.Maximum = new decimal(new int[] {
            128000,
            0,
            0,
            0});
            this.crop_left.Name = "crop_left";
            this.crop_left.Size = new System.Drawing.Size(44, 21);
            this.crop_left.TabIndex = 98;
            this.crop_left.ValueChanged += new System.EventHandler(this.CropValueChanged);
            // 
            // crop_right
            // 
            this.crop_right.Enabled = false;
            this.crop_right.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.crop_right.Location = new System.Drawing.Point(228, 53);
            this.crop_right.Maximum = new decimal(new int[] {
            128000,
            0,
            0,
            0});
            this.crop_right.Name = "crop_right";
            this.crop_right.Size = new System.Drawing.Size(44, 21);
            this.crop_right.TabIndex = 101;
            this.crop_right.ValueChanged += new System.EventHandler(this.CropValueChanged);
            // 
            // check_autoCrop
            // 
            this.check_autoCrop.AutoSize = true;
            this.check_autoCrop.Checked = true;
            this.check_autoCrop.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.check_autoCrop.Location = new System.Drawing.Point(0, 3);
            this.check_autoCrop.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.check_autoCrop.Name = "check_autoCrop";
            this.check_autoCrop.Size = new System.Drawing.Size(73, 17);
            this.check_autoCrop.TabIndex = 105;
            this.check_autoCrop.TabStop = true;
            this.check_autoCrop.Text = "Automatic";
            this.check_autoCrop.UseVisualStyleBackColor = true;
            this.check_autoCrop.CheckedChanged += new System.EventHandler(this.CheckAutoCropCheckedChanged);
            // 
            // lbl_cropRight
            // 
            this.lbl_cropRight.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_cropRight.AutoSize = true;
            this.lbl_cropRight.BackColor = System.Drawing.Color.Transparent;
            this.lbl_cropRight.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_cropRight.Location = new System.Drawing.Point(278, 57);
            this.lbl_cropRight.Margin = new System.Windows.Forms.Padding(3);
            this.lbl_cropRight.Name = "lbl_cropRight";
            this.lbl_cropRight.Size = new System.Drawing.Size(32, 13);
            this.lbl_cropRight.TabIndex = 102;
            this.lbl_cropRight.Text = "Right";
            // 
            // crop_top
            // 
            this.crop_top.Enabled = false;
            this.crop_top.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.crop_top.Location = new System.Drawing.Point(178, 26);
            this.crop_top.Maximum = new decimal(new int[] {
            128000,
            0,
            0,
            0});
            this.crop_top.Name = "crop_top";
            this.crop_top.Size = new System.Drawing.Size(44, 21);
            this.crop_top.TabIndex = 100;
            this.crop_top.ValueChanged += new System.EventHandler(this.CropValueChanged);
            // 
            // lbl_cropBottom
            // 
            this.lbl_cropBottom.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this.lbl_cropBottom.AutoSize = true;
            this.lbl_cropBottom.BackColor = System.Drawing.Color.Transparent;
            this.lbl_cropBottom.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_cropBottom.Location = new System.Drawing.Point(179, 107);
            this.lbl_cropBottom.Margin = new System.Windows.Forms.Padding(3);
            this.lbl_cropBottom.Name = "lbl_cropBottom";
            this.lbl_cropBottom.Size = new System.Drawing.Size(41, 13);
            this.lbl_cropBottom.TabIndex = 104;
            this.lbl_cropBottom.Text = "Bottom";
            // 
            // lbl_cropTop
            // 
            this.lbl_cropTop.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.lbl_cropTop.AutoSize = true;
            this.lbl_cropTop.BackColor = System.Drawing.Color.Transparent;
            this.lbl_cropTop.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_cropTop.Location = new System.Drawing.Point(187, 7);
            this.lbl_cropTop.Margin = new System.Windows.Forms.Padding(3);
            this.lbl_cropTop.Name = "lbl_cropTop";
            this.lbl_cropTop.Size = new System.Drawing.Size(25, 13);
            this.lbl_cropTop.TabIndex = 99;
            this.lbl_cropTop.Text = "Top";
            // 
            // crop_bottom
            // 
            this.crop_bottom.Enabled = false;
            this.crop_bottom.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.crop_bottom.Location = new System.Drawing.Point(178, 80);
            this.crop_bottom.Maximum = new decimal(new int[] {
            128000,
            0,
            0,
            0});
            this.crop_bottom.Name = "crop_bottom";
            this.crop_bottom.Size = new System.Drawing.Size(44, 21);
            this.crop_bottom.TabIndex = 103;
            this.crop_bottom.ValueChanged += new System.EventHandler(this.CropValueChanged);
            // 
            // check_customCrop
            // 
            this.check_customCrop.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.check_customCrop.AutoSize = true;
            this.check_customCrop.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.check_customCrop.Location = new System.Drawing.Point(0, 28);
            this.check_customCrop.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.check_customCrop.Name = "check_customCrop";
            this.check_customCrop.Size = new System.Drawing.Size(61, 17);
            this.check_customCrop.TabIndex = 106;
            this.check_customCrop.Text = "Custom";
            this.check_customCrop.UseVisualStyleBackColor = true;
            this.check_customCrop.CheckedChanged += new System.EventHandler(this.CheckAutoCropCheckedChanged);
            // 
            // tbl_size
            // 
            this.tbl_size.AutoSize = true;
            this.tbl_size.ColumnCount = 4;
            this.tbl_size.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_size.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_size.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_size.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_size.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 20F));
            this.tbl_size.Controls.Add(this.lbl_src, 0, 0);
            this.tbl_size.Controls.Add(this.lbl_src_res, 1, 0);
            this.tbl_size.Controls.Add(this.lbl_width, 0, 1);
            this.tbl_size.Controls.Add(this.text_width, 1, 1);
            this.tbl_size.Controls.Add(this.lbl_height, 2, 1);
            this.tbl_size.Controls.Add(this.text_height, 3, 1);
            this.tbl_size.Controls.Add(this.check_KeepAR, 1, 2);
            this.tbl_size.Controls.Add(this.check_UseITUPar, 2, 2);
            this.tbl_size.Controls.Add(this.lbl_useITUPar, 2, 0);
            this.tbl_size.Location = new System.Drawing.Point(16, 35);
            this.tbl_size.Name = "tbl_size";
            this.tbl_size.RowCount = 3;
            this.tbl_size.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_size.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_size.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_size.Size = new System.Drawing.Size(238, 69);
            this.tbl_size.TabIndex = 116;
            // 
            // lbl_src
            // 
            this.lbl_src.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_src.AutoSize = true;
            this.lbl_src.BackColor = System.Drawing.Color.Transparent;
            this.lbl_src.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_src.Location = new System.Drawing.Point(0, 3);
            this.lbl_src.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lbl_src.Name = "lbl_src";
            this.lbl_src.Size = new System.Drawing.Size(44, 13);
            this.lbl_src.TabIndex = 76;
            this.lbl_src.Text = "Source:";
            // 
            // lbl_src_res
            // 
            this.lbl_src_res.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_src_res.AutoSize = true;
            this.lbl_src_res.BackColor = System.Drawing.Color.Transparent;
            this.lbl_src_res.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_src_res.Location = new System.Drawing.Point(50, 3);
            this.lbl_src_res.Margin = new System.Windows.Forms.Padding(3);
            this.lbl_src_res.Name = "lbl_src_res";
            this.lbl_src_res.Size = new System.Drawing.Size(15, 13);
            this.lbl_src_res.TabIndex = 77;
            this.lbl_src_res.Text = "--";
            // 
            // lbl_width
            // 
            this.lbl_width.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_width.AutoSize = true;
            this.lbl_width.BackColor = System.Drawing.Color.Transparent;
            this.lbl_width.ForeColor = System.Drawing.Color.Black;
            this.lbl_width.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_width.Location = new System.Drawing.Point(0, 26);
            this.lbl_width.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lbl_width.Name = "lbl_width";
            this.lbl_width.Size = new System.Drawing.Size(39, 13);
            this.lbl_width.TabIndex = 80;
            this.lbl_width.Text = "Width:";
            // 
            // text_width
            // 
            this.text_width.Location = new System.Drawing.Point(50, 22);
            this.text_width.Maximum = new decimal(new int[] {
            1280000,
            0,
            0,
            0});
            this.text_width.Name = "text_width";
            this.text_width.Size = new System.Drawing.Size(64, 21);
            this.text_width.TabIndex = 85;
            this.text_width.ValueChanged += new System.EventHandler(this.TextWidthValueChanged);
            // 
            // lbl_height
            // 
            this.lbl_height.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_height.AutoSize = true;
            this.lbl_height.BackColor = System.Drawing.Color.Transparent;
            this.lbl_height.ForeColor = System.Drawing.Color.Black;
            this.lbl_height.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_height.Location = new System.Drawing.Point(123, 26);
            this.lbl_height.Margin = new System.Windows.Forms.Padding(3);
            this.lbl_height.Name = "lbl_height";
            this.lbl_height.Size = new System.Drawing.Size(42, 13);
            this.lbl_height.TabIndex = 84;
            this.lbl_height.Text = "Height:";
            // 
            // text_height
            // 
            this.text_height.Location = new System.Drawing.Point(171, 22);
            this.text_height.Maximum = new decimal(new int[] {
            1280000,
            0,
            0,
            0});
            this.text_height.Name = "text_height";
            this.text_height.Size = new System.Drawing.Size(64, 21);
            this.text_height.TabIndex = 86;
            this.text_height.ValueChanged += new System.EventHandler(this.TextHeightValueChanged);
            // 
            // check_KeepAR
            // 
            this.check_KeepAR.AutoSize = true;
            this.check_KeepAR.Checked = true;
            this.check_KeepAR.CheckState = System.Windows.Forms.CheckState.Checked;
            this.check_KeepAR.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.check_KeepAR.Location = new System.Drawing.Point(50, 49);
            this.check_KeepAR.Name = "check_KeepAR";
            this.check_KeepAR.Size = new System.Drawing.Size(67, 17);
            this.check_KeepAR.TabIndex = 95;
            this.check_KeepAR.Text = "Keep AR";
            this.check_KeepAR.UseVisualStyleBackColor = true;
            this.check_KeepAR.CheckedChanged += new System.EventHandler(this.CheckKeepArCheckedChanged);
            // 
            // check_UseITUPar
            // 
            this.check_UseITUPar.AutoSize = true;
            this.tbl_size.SetColumnSpan(this.check_UseITUPar, 2);
            this.check_UseITUPar.Enabled = false;
            this.check_UseITUPar.Location = new System.Drawing.Point(123, 49);
            this.check_UseITUPar.Name = "check_UseITUPar";
            this.check_UseITUPar.Size = new System.Drawing.Size(87, 17);
            this.check_UseITUPar.TabIndex = 96;
            this.check_UseITUPar.Text = "Use ITU PAR";
            this.check_UseITUPar.UseVisualStyleBackColor = true;
            this.check_UseITUPar.CheckedChanged += new System.EventHandler(this.CheckITUParCheckedChanged);
            // 
            // lbl_useITUPar
            // 
            this.lbl_useITUPar.AutoSize = true;
            this.tbl_size.SetColumnSpan(this.lbl_useITUPar, 2);
            this.lbl_useITUPar.Location = new System.Drawing.Point(123, 3);
            this.lbl_useITUPar.Margin = new System.Windows.Forms.Padding(3);
            this.lbl_useITUPar.Name = "lbl_useITUPar";
            this.lbl_useITUPar.Size = new System.Drawing.Size(91, 13);
            this.lbl_useITUPar.TabIndex = 97;
            this.lbl_useITUPar.Text = "(ITU par is used!)";
            this.lbl_useITUPar.Visible = false;
            // 
            // tbl_anamorphic
            // 
            this.tbl_anamorphic.AutoSize = true;
            this.tbl_anamorphic.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.tbl_anamorphic.ColumnCount = 2;
            this.tbl_anamorphic.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_anamorphic.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_anamorphic.Controls.Add(this.drp_colormatrix, 1, 6);
            this.tbl_anamorphic.Controls.Add(this.lbl_colormatrix, 0, 6);
            this.tbl_anamorphic.Controls.Add(this.updownParHeight, 1, 4);
            this.tbl_anamorphic.Controls.Add(this.lbl_anamorphic, 0, 0);
            this.tbl_anamorphic.Controls.Add(this.labelDisplaySize, 1, 5);
            this.tbl_anamorphic.Controls.Add(this.lbl_parHeight, 0, 4);
            this.tbl_anamorphic.Controls.Add(this.labelStaticDisplaySize, 0, 5);
            this.tbl_anamorphic.Controls.Add(this.updownParWidth, 1, 3);
            this.tbl_anamorphic.Controls.Add(this.drp_anamorphic, 1, 0);
            this.tbl_anamorphic.Controls.Add(this.lbl_parWidth, 0, 3);
            this.tbl_anamorphic.Controls.Add(this.updownDisplayWidth, 1, 2);
            this.tbl_anamorphic.Controls.Add(this.lbl_modulus, 0, 1);
            this.tbl_anamorphic.Controls.Add(this.lbl_displayWidth, 0, 2);
            this.tbl_anamorphic.Controls.Add(this.drp_modulus, 1, 1);
            this.tbl_anamorphic.Location = new System.Drawing.Point(16, 106);
            this.tbl_anamorphic.Name = "tbl_anamorphic";
            this.tbl_anamorphic.RowCount = 7;
            this.tbl_anamorphic.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_anamorphic.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_anamorphic.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_anamorphic.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_anamorphic.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_anamorphic.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_anamorphic.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_anamorphic.Size = new System.Drawing.Size(206, 181);
            this.tbl_anamorphic.TabIndex = 115;
            // 
            // drp_colormatrix
            // 
            this.drp_colormatrix.FormattingEnabled = true;
            this.drp_colormatrix.Items.AddRange(new object[] {
            "Auto",
            "ITU-R BT.709",
            "ITU-R BT.601"});
            this.drp_colormatrix.Location = new System.Drawing.Point(82, 157);
            this.drp_colormatrix.MaxDropDownItems = 3;
            this.drp_colormatrix.Name = "drp_colormatrix";
            this.drp_colormatrix.Size = new System.Drawing.Size(121, 21);
            this.drp_colormatrix.TabIndex = 1;
            this.drp_colormatrix.SelectedIndexChanged += new System.EventHandler(this.DrpColorMatrixSelectedIndexChanged);
            // 
            // lbl_colormatrix
            // 
            this.lbl_colormatrix.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_colormatrix.AutoSize = true;
            this.lbl_colormatrix.Location = new System.Drawing.Point(0, 161);
            this.lbl_colormatrix.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lbl_colormatrix.Name = "lbl_colormatrix";
            this.lbl_colormatrix.Size = new System.Drawing.Size(66, 13);
            this.lbl_colormatrix.TabIndex = 0;
            this.lbl_colormatrix.Text = "Colormatrix:";
            // 
            // updownParHeight
            // 
            this.updownParHeight.Location = new System.Drawing.Point(82, 111);
            this.updownParHeight.Maximum = new decimal(new int[] {
            1280000,
            0,
            0,
            0});
            this.updownParHeight.Name = "updownParHeight";
            this.updownParHeight.Size = new System.Drawing.Size(53, 21);
            this.updownParHeight.TabIndex = 112;
            this.updownParHeight.ValueChanged += new System.EventHandler(this.TextWidthValueChanged);
            // 
            // lbl_anamorphic
            // 
            this.lbl_anamorphic.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_anamorphic.AutoSize = true;
            this.lbl_anamorphic.BackColor = System.Drawing.Color.Transparent;
            this.lbl_anamorphic.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_anamorphic.Location = new System.Drawing.Point(0, 7);
            this.lbl_anamorphic.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lbl_anamorphic.Name = "lbl_anamorphic";
            this.lbl_anamorphic.Size = new System.Drawing.Size(67, 13);
            this.lbl_anamorphic.TabIndex = 81;
            this.lbl_anamorphic.Text = "Anamorphic:";
            // 
            // labelDisplaySize
            // 
            this.labelDisplaySize.AutoSize = true;
            this.labelDisplaySize.BackColor = System.Drawing.Color.Transparent;
            this.labelDisplaySize.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.labelDisplaySize.Location = new System.Drawing.Point(82, 138);
            this.labelDisplaySize.Margin = new System.Windows.Forms.Padding(3);
            this.labelDisplaySize.Name = "labelDisplaySize";
            this.labelDisplaySize.Size = new System.Drawing.Size(15, 13);
            this.labelDisplaySize.TabIndex = 108;
            this.labelDisplaySize.Text = "--";
            // 
            // lbl_parHeight
            // 
            this.lbl_parHeight.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_parHeight.AutoSize = true;
            this.lbl_parHeight.BackColor = System.Drawing.Color.Transparent;
            this.lbl_parHeight.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_parHeight.Location = new System.Drawing.Point(0, 115);
            this.lbl_parHeight.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lbl_parHeight.Name = "lbl_parHeight";
            this.lbl_parHeight.Size = new System.Drawing.Size(65, 13);
            this.lbl_parHeight.TabIndex = 94;
            this.lbl_parHeight.Text = "PAR Height:";
            // 
            // labelStaticDisplaySize
            // 
            this.labelStaticDisplaySize.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.labelStaticDisplaySize.AutoSize = true;
            this.labelStaticDisplaySize.BackColor = System.Drawing.Color.Transparent;
            this.labelStaticDisplaySize.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.labelStaticDisplaySize.Location = new System.Drawing.Point(0, 138);
            this.labelStaticDisplaySize.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.labelStaticDisplaySize.Name = "labelStaticDisplaySize";
            this.labelStaticDisplaySize.Size = new System.Drawing.Size(67, 13);
            this.labelStaticDisplaySize.TabIndex = 107;
            this.labelStaticDisplaySize.Text = "Display Size:";
            // 
            // updownParWidth
            // 
            this.updownParWidth.Location = new System.Drawing.Point(82, 84);
            this.updownParWidth.Maximum = new decimal(new int[] {
            1280000,
            0,
            0,
            0});
            this.updownParWidth.Name = "updownParWidth";
            this.updownParWidth.Size = new System.Drawing.Size(53, 21);
            this.updownParWidth.TabIndex = 111;
            this.updownParWidth.ValueChanged += new System.EventHandler(this.TextWidthValueChanged);
            // 
            // drp_anamorphic
            // 
            this.drp_anamorphic.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_anamorphic.FormattingEnabled = true;
            this.drp_anamorphic.Items.AddRange(new object[] {
            "None",
            "Strict",
            "Loose",
            "Custom"});
            this.drp_anamorphic.Location = new System.Drawing.Point(82, 3);
            this.drp_anamorphic.Name = "drp_anamorphic";
            this.drp_anamorphic.Size = new System.Drawing.Size(110, 21);
            this.drp_anamorphic.TabIndex = 82;
            this.drp_anamorphic.SelectedIndexChanged += new System.EventHandler(this.DrpAnamorphicSelectedIndexChanged);
            // 
            // lbl_parWidth
            // 
            this.lbl_parWidth.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_parWidth.AutoSize = true;
            this.lbl_parWidth.BackColor = System.Drawing.Color.Transparent;
            this.lbl_parWidth.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_parWidth.Location = new System.Drawing.Point(0, 88);
            this.lbl_parWidth.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lbl_parWidth.Name = "lbl_parWidth";
            this.lbl_parWidth.Size = new System.Drawing.Size(62, 13);
            this.lbl_parWidth.TabIndex = 92;
            this.lbl_parWidth.Text = "PAR Width:";
            // 
            // updownDisplayWidth
            // 
            this.updownDisplayWidth.Location = new System.Drawing.Point(82, 57);
            this.updownDisplayWidth.Maximum = new decimal(new int[] {
            1280000,
            0,
            0,
            0});
            this.updownDisplayWidth.Name = "updownDisplayWidth";
            this.updownDisplayWidth.Size = new System.Drawing.Size(53, 21);
            this.updownDisplayWidth.TabIndex = 110;
            this.updownDisplayWidth.ValueChanged += new System.EventHandler(this.UpdownDisplayWidthValueChanged);
            // 
            // lbl_modulus
            // 
            this.lbl_modulus.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_modulus.AutoSize = true;
            this.lbl_modulus.BackColor = System.Drawing.Color.Transparent;
            this.lbl_modulus.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_modulus.Location = new System.Drawing.Point(0, 34);
            this.lbl_modulus.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lbl_modulus.Name = "lbl_modulus";
            this.lbl_modulus.Size = new System.Drawing.Size(50, 13);
            this.lbl_modulus.TabIndex = 87;
            this.lbl_modulus.Text = "Modulus:";
            // 
            // lbl_displayWidth
            // 
            this.lbl_displayWidth.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_displayWidth.AutoSize = true;
            this.lbl_displayWidth.BackColor = System.Drawing.Color.Transparent;
            this.lbl_displayWidth.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_displayWidth.Location = new System.Drawing.Point(0, 61);
            this.lbl_displayWidth.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lbl_displayWidth.Name = "lbl_displayWidth";
            this.lbl_displayWidth.Size = new System.Drawing.Size(76, 13);
            this.lbl_displayWidth.TabIndex = 90;
            this.lbl_displayWidth.Text = "Display Width:";
            // 
            // drp_modulus
            // 
            this.drp_modulus.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.drp_modulus.FormattingEnabled = true;
            this.drp_modulus.Items.AddRange(new object[] {
            "16",
            "8",
            "4",
            "2"});
            this.drp_modulus.Location = new System.Drawing.Point(82, 30);
            this.drp_modulus.Name = "drp_modulus";
            this.drp_modulus.Size = new System.Drawing.Size(110, 21);
            this.drp_modulus.TabIndex = 88;
            this.drp_modulus.SelectedIndexChanged += new System.EventHandler(this.DrpModulusSelectedIndexChanged);
            // 
            // lbl_cropping
            // 
            this.lbl_cropping.AutoSize = true;
            this.lbl_cropping.BackColor = System.Drawing.Color.Transparent;
            this.lbl_cropping.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_cropping.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_cropping.Location = new System.Drawing.Point(335, 13);
            this.lbl_cropping.Name = "lbl_cropping";
            this.lbl_cropping.Size = new System.Drawing.Size(57, 13);
            this.lbl_cropping.TabIndex = 114;
            this.lbl_cropping.Text = "Cropping";
            // 
            // lbl_size
            // 
            this.lbl_size.AutoSize = true;
            this.lbl_size.BackColor = System.Drawing.Color.Transparent;
            this.lbl_size.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_size.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_size.Location = new System.Drawing.Point(13, 13);
            this.lbl_size.Name = "lbl_size";
            this.lbl_size.Size = new System.Drawing.Size(30, 13);
            this.lbl_size.TabIndex = 113;
            this.lbl_size.Text = "Size";
            // 
            // lbl_presetCropWarning
            // 
            this.lbl_presetCropWarning.AutoSize = true;
            this.lbl_presetCropWarning.ForeColor = System.Drawing.Color.Black;
            this.lbl_presetCropWarning.Location = new System.Drawing.Point(390, 13);
            this.lbl_presetCropWarning.Name = "lbl_presetCropWarning";
            this.lbl_presetCropWarning.Size = new System.Drawing.Size(140, 13);
            this.lbl_presetCropWarning.TabIndex = 118;
            this.lbl_presetCropWarning.Text = "( Preset values are in use! )";
            // 
            // lbl_padding
            // 
            this.lbl_padding.AutoSize = true;
            this.lbl_padding.BackColor = System.Drawing.Color.Transparent;
            this.lbl_padding.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.lbl_padding.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_padding.Location = new System.Drawing.Point(335, 158);
            this.lbl_padding.Name = "lbl_padding";
            this.lbl_padding.Size = new System.Drawing.Size(52, 13);
            this.lbl_padding.TabIndex = 114;
            this.lbl_padding.Text = "Padding";
            // 
            // tbl_padding
            // 
            this.tbl_padding.AutoSize = true;
            this.tbl_padding.ColumnCount = 6;
            this.tbl_padding.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 96F));
            this.tbl_padding.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_padding.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_padding.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_padding.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_padding.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle());
            this.tbl_padding.Controls.Add(this.pad_right, 4, 2);
            this.tbl_padding.Controls.Add(this.check_disablePad, 0, 1);
            this.tbl_padding.Controls.Add(this.lbl_padLeft, 1, 2);
            this.tbl_padding.Controls.Add(this.pad_left, 2, 2);
            this.tbl_padding.Controls.Add(this.lbl_padRight, 5, 2);
            this.tbl_padding.Controls.Add(this.check_enablePad, 0, 0);
            this.tbl_padding.Controls.Add(this.lbl_padTop, 3, 0);
            this.tbl_padding.Controls.Add(this.pad_top, 3, 1);
            this.tbl_padding.Controls.Add(this.pad_bottom, 3, 3);
            this.tbl_padding.Controls.Add(this.lbl_padBottom, 3, 4);
            this.tbl_padding.Location = new System.Drawing.Point(338, 180);
            this.tbl_padding.Name = "tbl_padding";
            this.tbl_padding.RowCount = 5;
            this.tbl_padding.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_padding.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_padding.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_padding.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_padding.RowStyles.Add(new System.Windows.Forms.RowStyle());
            this.tbl_padding.Size = new System.Drawing.Size(313, 123);
            this.tbl_padding.TabIndex = 119;
            // 
            // pad_right
            // 
            this.pad_right.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.pad_right.Location = new System.Drawing.Point(228, 53);
            this.pad_right.Maximum = new decimal(new int[] {
            1920,
            0,
            0,
            0});
            this.pad_right.Name = "pad_right";
            this.pad_right.Size = new System.Drawing.Size(44, 21);
            this.pad_right.TabIndex = 101;
            this.pad_right.ValueChanged += new System.EventHandler(this.PadValueChanged);
            // 
            // check_disablePad
            // 
            this.check_disablePad.AutoSize = true;
            this.check_disablePad.Checked = true;
            this.check_disablePad.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.check_disablePad.Location = new System.Drawing.Point(0, 26);
            this.check_disablePad.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.check_disablePad.Name = "check_disablePad";
            this.check_disablePad.Size = new System.Drawing.Size(65, 17);
            this.check_disablePad.TabIndex = 106;
            this.check_disablePad.TabStop = true;
            this.check_disablePad.Text = "Disable";
            this.check_disablePad.UseVisualStyleBackColor = true;
            this.check_disablePad.CheckedChanged += new System.EventHandler(this.CheckEnablePadChanged);
            // 
            // lbl_padLeft
            // 
            this.lbl_padLeft.Anchor = System.Windows.Forms.AnchorStyles.Right;
            this.lbl_padLeft.AutoSize = true;
            this.lbl_padLeft.BackColor = System.Drawing.Color.Transparent;
            this.lbl_padLeft.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_padLeft.Location = new System.Drawing.Point(96, 57);
            this.lbl_padLeft.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.lbl_padLeft.Name = "lbl_padLeft";
            this.lbl_padLeft.Size = new System.Drawing.Size(26, 13);
            this.lbl_padLeft.TabIndex = 97;
            this.lbl_padLeft.Text = "Left";
            // 
            // pad_left
            // 
            this.pad_left.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.pad_left.Location = new System.Drawing.Point(128, 53);
            this.pad_left.Maximum = new decimal(new int[] {
            1920,
            0,
            0,
            0});
            this.pad_left.Name = "pad_left";
            this.pad_left.Size = new System.Drawing.Size(44, 21);
            this.pad_left.TabIndex = 98;
            this.pad_left.ValueChanged += new System.EventHandler(this.PadValueChanged);
            // 
            // lbl_padRight
            // 
            this.lbl_padRight.Anchor = System.Windows.Forms.AnchorStyles.Left;
            this.lbl_padRight.AutoSize = true;
            this.lbl_padRight.BackColor = System.Drawing.Color.Transparent;
            this.lbl_padRight.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_padRight.Location = new System.Drawing.Point(278, 57);
            this.lbl_padRight.Margin = new System.Windows.Forms.Padding(3);
            this.lbl_padRight.Name = "lbl_padRight";
            this.lbl_padRight.Size = new System.Drawing.Size(32, 13);
            this.lbl_padRight.TabIndex = 102;
            this.lbl_padRight.Text = "Right";
            // 
            // check_enablePad
            // 
            this.check_enablePad.AutoSize = true;
            this.check_enablePad.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.check_enablePad.Location = new System.Drawing.Point(0, 3);
            this.check_enablePad.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.check_enablePad.Name = "check_enablePad";
            this.check_enablePad.Size = new System.Drawing.Size(63, 17);
            this.check_enablePad.TabIndex = 105;
            this.check_enablePad.Text = "Enable";
            this.check_enablePad.UseVisualStyleBackColor = true;
            this.check_enablePad.CheckedChanged += new System.EventHandler(this.CheckEnablePadChanged);
            // 
            // lbl_padTop
            // 
            this.lbl_padTop.Anchor = System.Windows.Forms.AnchorStyles.Bottom;
            this.lbl_padTop.AutoSize = true;
            this.lbl_padTop.BackColor = System.Drawing.Color.Transparent;
            this.lbl_padTop.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_padTop.Location = new System.Drawing.Point(187, 7);
            this.lbl_padTop.Margin = new System.Windows.Forms.Padding(3);
            this.lbl_padTop.Name = "lbl_padTop";
            this.lbl_padTop.Size = new System.Drawing.Size(25, 13);
            this.lbl_padTop.TabIndex = 99;
            this.lbl_padTop.Text = "Top";
            // 
            // pad_top
            // 
            this.pad_top.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.pad_top.Location = new System.Drawing.Point(178, 26);
            this.pad_top.Maximum = new decimal(new int[] {
            1080,
            0,
            0,
            0});
            this.pad_top.Name = "pad_top";
            this.pad_top.Size = new System.Drawing.Size(44, 21);
            this.pad_top.TabIndex = 100;
            this.pad_top.ValueChanged += new System.EventHandler(this.PadValueChanged);
            // 
            // pad_bottom
            // 
            this.pad_bottom.Increment = new decimal(new int[] {
            2,
            0,
            0,
            0});
            this.pad_bottom.Location = new System.Drawing.Point(178, 80);
            this.pad_bottom.Maximum = new decimal(new int[] {
            1080,
            0,
            0,
            0});
            this.pad_bottom.Name = "pad_bottom";
            this.pad_bottom.Size = new System.Drawing.Size(44, 21);
            this.pad_bottom.TabIndex = 103;
            this.pad_bottom.ValueChanged += new System.EventHandler(this.PadValueChanged);
            // 
            // lbl_padBottom
            // 
            this.lbl_padBottom.Anchor = System.Windows.Forms.AnchorStyles.Top;
            this.lbl_padBottom.AutoSize = true;
            this.lbl_padBottom.BackColor = System.Drawing.Color.Transparent;
            this.lbl_padBottom.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.lbl_padBottom.Location = new System.Drawing.Point(179, 107);
            this.lbl_padBottom.Margin = new System.Windows.Forms.Padding(3);
            this.lbl_padBottom.Name = "lbl_padBottom";
            this.lbl_padBottom.Size = new System.Drawing.Size(41, 13);
            this.lbl_padBottom.TabIndex = 104;
            this.lbl_padBottom.Text = "Bottom";
            // 
            // check_disableCrop
            // 
            this.check_disableCrop.AutoSize = true;
            this.check_disableCrop.ImeMode = System.Windows.Forms.ImeMode.NoControl;
            this.check_disableCrop.Location = new System.Drawing.Point(0, 53);
            this.check_disableCrop.Margin = new System.Windows.Forms.Padding(0, 3, 3, 3);
            this.check_disableCrop.Name = "check_disableCrop";
            this.check_disableCrop.Size = new System.Drawing.Size(59, 17);
            this.check_disableCrop.TabIndex = 107;
            this.check_disableCrop.Text = "Disable";
            this.check_disableCrop.UseVisualStyleBackColor = true;
            this.check_disableCrop.CheckedChanged += new System.EventHandler(this.CheckAutoCropCheckedChanged);
            // 
            // PictureSettings
            // 
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Inherit;
            this.BackColor = System.Drawing.Color.Transparent;
            this.Controls.Add(this.tbl_padding);
            this.Controls.Add(this.lbl_presetCropWarning);
            this.Controls.Add(this.tbl_cropping);
            this.Controls.Add(this.tbl_size);
            this.Controls.Add(this.tbl_anamorphic);
            this.Controls.Add(this.lbl_padding);
            this.Controls.Add(this.lbl_cropping);
            this.Controls.Add(this.lbl_size);
            this.Enabled = false;
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.Name = "PictureSettings";
            this.Size = new System.Drawing.Size(716, 304);
            this.tbl_cropping.ResumeLayout(false);
            this.tbl_cropping.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.crop_left)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_right)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_top)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.crop_bottom)).EndInit();
            this.tbl_size.ResumeLayout(false);
            this.tbl_size.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.text_width)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.text_height)).EndInit();
            this.tbl_anamorphic.ResumeLayout(false);
            this.tbl_anamorphic.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.updownParHeight)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.updownParWidth)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.updownDisplayWidth)).EndInit();
            this.tbl_padding.ResumeLayout(false);
            this.tbl_padding.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pad_right)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pad_left)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pad_top)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.pad_bottom)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TableLayoutPanel tbl_cropping;
        internal System.Windows.Forms.Label lbl_cropLeft;
        internal System.Windows.Forms.NumericUpDown crop_left;
        internal System.Windows.Forms.NumericUpDown crop_right;
        internal System.Windows.Forms.RadioButton check_autoCrop;
        internal System.Windows.Forms.Label lbl_cropRight;
        internal System.Windows.Forms.NumericUpDown crop_top;
        internal System.Windows.Forms.Label lbl_cropBottom;
        internal System.Windows.Forms.Label lbl_cropTop;
        internal System.Windows.Forms.NumericUpDown crop_bottom;
        internal System.Windows.Forms.RadioButton check_customCrop;
        private System.Windows.Forms.TableLayoutPanel tbl_size;
        internal System.Windows.Forms.Label lbl_src;
        internal System.Windows.Forms.Label lbl_src_res;
        internal System.Windows.Forms.Label lbl_width;
        internal System.Windows.Forms.Label lbl_height;
        internal System.Windows.Forms.CheckBox check_KeepAR;
        private System.Windows.Forms.TableLayoutPanel tbl_anamorphic;
        internal System.Windows.Forms.NumericUpDown updownParHeight;
        internal System.Windows.Forms.Label lbl_anamorphic;
        internal System.Windows.Forms.Label labelDisplaySize;
        internal System.Windows.Forms.Label lbl_parHeight;
        internal System.Windows.Forms.Label labelStaticDisplaySize;
        internal System.Windows.Forms.NumericUpDown updownParWidth;
        internal System.Windows.Forms.ComboBox drp_anamorphic;
        internal System.Windows.Forms.Label lbl_parWidth;
        internal System.Windows.Forms.NumericUpDown updownDisplayWidth;
        internal System.Windows.Forms.Label lbl_modulus;
        internal System.Windows.Forms.Label lbl_displayWidth;
        internal System.Windows.Forms.ComboBox drp_modulus;
        internal System.Windows.Forms.Label lbl_cropping;
        internal System.Windows.Forms.Label lbl_size;
        internal System.Windows.Forms.NumericUpDown text_width;
        internal System.Windows.Forms.NumericUpDown text_height;
        private System.Windows.Forms.Label lbl_presetCropWarning;
        private System.Windows.Forms.Label lbl_colormatrix;
        internal System.Windows.Forms.ComboBox drp_colormatrix;
        internal System.Windows.Forms.Label lbl_padding;
        internal System.Windows.Forms.NumericUpDown pad_right;
        internal System.Windows.Forms.RadioButton check_disablePad;
        internal System.Windows.Forms.Label lbl_padLeft;
        internal System.Windows.Forms.NumericUpDown pad_left;
        internal System.Windows.Forms.Label lbl_padRight;
        internal System.Windows.Forms.RadioButton check_enablePad;
        internal System.Windows.Forms.Label lbl_padTop;
        internal System.Windows.Forms.NumericUpDown pad_top;
        internal System.Windows.Forms.NumericUpDown pad_bottom;
        internal System.Windows.Forms.Label lbl_padBottom;
        private System.Windows.Forms.Label lbl_useITUPar;
        internal System.Windows.Forms.CheckBox check_UseITUPar;
        internal System.Windows.Forms.TableLayoutPanel tbl_padding;
        internal System.Windows.Forms.RadioButton check_disableCrop;



    }
}
