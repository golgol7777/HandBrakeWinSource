﻿/*  frmAddPreset.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.Drawing;
using System.Windows.Forms;

namespace Handbrake
{
    public partial class frmAddPreset : Form
    {
        private readonly frmMain frmMainWindow;
        readonly Presets.PresetsHandler presetCode;
        private readonly string query = "";

        public frmAddPreset(frmMain fmw, string query_string, Presets.PresetsHandler presetHandler)
        {
            InitializeComponent();
            frmMainWindow = fmw;
            presetCode = presetHandler;
            this.query = query_string;
        }

        private void btn_add_Click(object sender, EventArgs e)
        {
            Boolean pictureSettings = false;
            if (check_pictureSettings.Checked)
                pictureSettings = true;

            if (presetCode.Add(txt_preset_name.Text.Trim(), query, pictureSettings))
            {
                TreeNode preset_treeview = new TreeNode(txt_preset_name.Text.Trim()) { ForeColor = Color.Black };
                frmMainWindow.treeView_presets.Nodes.Add(preset_treeview);

                //frmMainWindow.loadPresetPanel();
                this.Close();
            } 
            else
            {
                MessageBox.Show("Sorry, that preset name already exists. Please choose another!", "Warning", MessageBoxButtons.OK, MessageBoxIcon.Warning);
            }
        }

        private void btn_cancel_Click(object sender, EventArgs e)
        {
            this.Close();
        }

    }
}