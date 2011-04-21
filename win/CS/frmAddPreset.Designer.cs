﻿/*  AudioTrack.cs $
 	
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    partial class frmAddPreset
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
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(frmAddPreset));
            this.lbl_name = new System.Windows.Forms.Label();
            this.txt_preset_name = new System.Windows.Forms.TextBox();
            this.btn_add = new System.Windows.Forms.Button();
            this.btn_cancel = new System.Windows.Forms.Button();
            this.check_useFilters = new System.Windows.Forms.CheckBox();
            this.toolTip = new System.Windows.Forms.ToolTip(this.components);
            this.cb_usePictureSettings = new System.Windows.Forms.ComboBox();
            this.label2 = new System.Windows.Forms.Label();
            this.label3 = new System.Windows.Forms.Label();
            this.lbl_x = new System.Windows.Forms.Label();
            this.maxWidth = new System.Windows.Forms.NumericUpDown();
            this.maxHeight = new System.Windows.Forms.NumericUpDown();
            ((System.ComponentModel.ISupportInitialize)(this.maxWidth)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.maxHeight)).BeginInit();
            this.SuspendLayout();
            // 
            // lbl_name
            // 
            this.lbl_name.AutoSize = true;
            this.lbl_name.Location = new System.Drawing.Point(12, 18);
            this.lbl_name.Name = "lbl_name";
            this.lbl_name.Size = new System.Drawing.Size(75, 13);
            this.lbl_name.TabIndex = 1;
            this.lbl_name.Text = "Preset Name: ";
            // 
            // txt_preset_name
            // 
            this.txt_preset_name.Location = new System.Drawing.Point(130, 15);
            this.txt_preset_name.Name = "txt_preset_name";
            this.txt_preset_name.Size = new System.Drawing.Size(172, 21);
            this.txt_preset_name.TabIndex = 0;
            // 
            // btn_add
            // 
            this.btn_add.BackColor = System.Drawing.Color.Transparent;
            this.btn_add.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btn_add.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_add.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_add.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_add.Location = new System.Drawing.Point(182, 157);
            this.btn_add.Name = "btn_add";
            this.btn_add.Size = new System.Drawing.Size(57, 22);
            this.btn_add.TabIndex = 3;
            this.btn_add.Text = "Add";
            this.btn_add.UseVisualStyleBackColor = false;
            this.btn_add.Click += new System.EventHandler(this.BtnAddClick);
            // 
            // btn_cancel
            // 
            this.btn_cancel.BackColor = System.Drawing.Color.Transparent;
            this.btn_cancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.btn_cancel.FlatAppearance.BorderColor = System.Drawing.Color.Black;
            this.btn_cancel.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.btn_cancel.ForeColor = System.Drawing.Color.FromArgb(((int)(((byte)(255)))), ((int)(((byte)(128)))), ((int)(((byte)(0)))));
            this.btn_cancel.Location = new System.Drawing.Point(245, 157);
            this.btn_cancel.Name = "btn_cancel";
            this.btn_cancel.Size = new System.Drawing.Size(57, 22);
            this.btn_cancel.TabIndex = 4;
            this.btn_cancel.Text = "Cancel";
            this.btn_cancel.UseVisualStyleBackColor = false;
            this.btn_cancel.Click += new System.EventHandler(this.BtnCancelClick);
            // 
            // check_useFilters
            // 
            this.check_useFilters.AutoSize = true;
            this.check_useFilters.Checked = true;
            this.check_useFilters.CheckState = System.Windows.Forms.CheckState.Checked;
            this.check_useFilters.Location = new System.Drawing.Point(130, 134);
            this.check_useFilters.Name = "check_useFilters";
            this.check_useFilters.Size = new System.Drawing.Size(119, 17);
            this.check_useFilters.TabIndex = 2;
            this.check_useFilters.Text = "Save Filter Settings";
            this.toolTip.SetToolTip(this.check_useFilters, "Save Picture Width/Height and Crop Values");
            this.check_useFilters.UseVisualStyleBackColor = true;
            // 
            // cb_usePictureSettings
            // 
            this.cb_usePictureSettings.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
            this.cb_usePictureSettings.FormattingEnabled = true;
            this.cb_usePictureSettings.Items.AddRange(new object[] {
            "None",
            "Custom",
            "Source Maximum"});
            this.cb_usePictureSettings.Location = new System.Drawing.Point(130, 79);
            this.cb_usePictureSettings.Name = "cb_usePictureSettings";
            this.cb_usePictureSettings.Size = new System.Drawing.Size(172, 21);
            this.cb_usePictureSettings.TabIndex = 1;
            this.cb_usePictureSettings.SelectedIndexChanged += new System.EventHandler(this.cb_usePictureSettings_SelectedIndexChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Bold, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.label2.Location = new System.Drawing.Point(12, 57);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(100, 13);
            this.label2.TabIndex = 7;
            this.label2.Text = "Picture Settings:";
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(12, 82);
            this.label3.Margin = new System.Windows.Forms.Padding(3);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(87, 13);
            this.label3.TabIndex = 8;
            this.label3.Text = "Use Picture Size:";
            // 
            // lbl_x
            // 
            this.lbl_x.AutoSize = true;
            this.lbl_x.Location = new System.Drawing.Point(211, 110);
            this.lbl_x.Name = "lbl_x";
            this.lbl_x.Size = new System.Drawing.Size(13, 13);
            this.lbl_x.TabIndex = 11;
            this.lbl_x.Text = "X";
            // 
            // maxWidth
            // 
            this.maxWidth.Location = new System.Drawing.Point(130, 107);
            this.maxWidth.Maximum = new decimal(new int[] {
            32000,
            0,
            0,
            0});
            this.maxWidth.Name = "maxWidth";
            this.maxWidth.Size = new System.Drawing.Size(74, 21);
            this.maxWidth.TabIndex = 12;
            this.maxWidth.Value = new decimal(new int[] {
            1280,
            0,
            0,
            0});
            // 
            // maxHeight
            // 
            this.maxHeight.Location = new System.Drawing.Point(229, 107);
            this.maxHeight.Maximum = new decimal(new int[] {
            32000,
            0,
            0,
            0});
            this.maxHeight.Name = "maxHeight";
            this.maxHeight.Size = new System.Drawing.Size(73, 21);
            this.maxHeight.TabIndex = 13;
            this.maxHeight.Value = new decimal(new int[] {
            720,
            0,
            0,
            0});
            // 
            // frmAddPreset
            // 
            this.AcceptButton = this.btn_add;
            this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
            this.AutoSize = true;
            this.AutoSizeMode = System.Windows.Forms.AutoSizeMode.GrowAndShrink;
            this.CancelButton = this.btn_cancel;
            this.ClientSize = new System.Drawing.Size(319, 191);
            this.Controls.Add(this.maxHeight);
            this.Controls.Add(this.maxWidth);
            this.Controls.Add(this.lbl_x);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.cb_usePictureSettings);
            this.Controls.Add(this.lbl_name);
            this.Controls.Add(this.txt_preset_name);
            this.Controls.Add(this.check_useFilters);
            this.Controls.Add(this.btn_cancel);
            this.Controls.Add(this.btn_add);
            this.Font = new System.Drawing.Font("Tahoma", 8.25F, System.Drawing.FontStyle.Regular, System.Drawing.GraphicsUnit.Point, ((byte)(0)));
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "frmAddPreset";
            this.Padding = new System.Windows.Forms.Padding(9);
            this.ShowIcon = false;
            this.ShowInTaskbar = false;
            this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
            this.Text = "Add New Preset";
            this.TopMost = true;
            ((System.ComponentModel.ISupportInitialize)(this.maxWidth)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.maxHeight)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lbl_name;
        private System.Windows.Forms.TextBox txt_preset_name;
        internal System.Windows.Forms.Button btn_add;
        internal System.Windows.Forms.Button btn_cancel;
        private System.Windows.Forms.CheckBox check_useFilters;
        private System.Windows.Forms.ToolTip toolTip;
        private System.Windows.Forms.ComboBox cb_usePictureSettings;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.Label lbl_x;
        private System.Windows.Forms.NumericUpDown maxWidth;
        private System.Windows.Forms.NumericUpDown maxHeight;
    }
}