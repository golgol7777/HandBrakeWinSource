﻿/*  frmAddPreset.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Services;

    using Handbrake.Functions;
    using Handbrake.Model;

    /// <summary>
    /// The Add Preset Window
    /// </summary>
    public partial class frmAddPreset : Form
    {
        private readonly frmMain mainWindow;

        /// <summary>
        /// The Preset Handler
        /// </summary>
        private readonly PresetService presetCode;

        /// <summary>
        /// Initializes a new instance of the <see cref="frmAddPreset"/> class.
        /// </summary>
        /// <param name="mainWindow">
        /// The Main Window
        /// </param>
        /// <param name="presetHandler">
        /// The preset handler.
        /// </param>
        public frmAddPreset(frmMain mainWindow, PresetService presetHandler)
        {
            InitializeComponent();
            this.mainWindow = mainWindow;
            presetCode = presetHandler;

            cb_usePictureSettings.SelectedIndex = 0;
        }

        /// <summary>
        /// Handle the Add button event.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnAddClick(object sender, EventArgs e)
        {
            if (string.IsNullOrEmpty(txt_preset_name.Text.Trim()))
            {
                MessageBox.Show("You must enter a preset name!", "Warning",
                                MessageBoxButtons.OK, MessageBoxIcon.Warning);                
                return;
            }

            QueryPictureSettingsMode pictureSettingsMode;

            switch (cb_usePictureSettings.SelectedIndex)
            {
                case 0:
                    pictureSettingsMode = QueryPictureSettingsMode.None;
                    break;
                case 1:
                    pictureSettingsMode = QueryPictureSettingsMode.Custom;
                    break;
                case 2:
                    pictureSettingsMode = QueryPictureSettingsMode.SourceMaximum;
                    break;
                default:
                    pictureSettingsMode = QueryPictureSettingsMode.None;
                    break;
            }

            string query = QueryGenerator.GenerateQueryForPreset(mainWindow, pictureSettingsMode, check_useFilters.Checked, Convert.ToInt32(maxWidth.Value), Convert.ToInt32(maxHeight.Value));

            Preset preset = new Preset
                {
                    Name = this.txt_preset_name.Text,
                    Query = query,
                    CropSettings = pictureSettingsMode != QueryPictureSettingsMode.None,
                    Description = string.Empty
                };

            if (presetCode.Add(preset))
            {
                this.DialogResult = DialogResult.OK;
                this.Close();           
            }
            else
                MessageBox.Show("Sorry, that preset name already exists. Please choose another!", "Warning", 
                                MessageBoxButtons.OK, MessageBoxIcon.Warning);
        }

        /// <summary>
        /// Handle the Cancel button event
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnCancelClick(object sender, EventArgs e)
        {
            this.DialogResult = DialogResult.Cancel;
            this.Close();
        }

        /// <summary>
        /// Picutre Settings option changed.
        /// </summary>
        /// <param name="sender">The Sender</param>
        /// <param name="e">The Event Args</param>
        private void cb_usePictureSettings_SelectedIndexChanged(object sender, EventArgs e)
        {
            if (cb_usePictureSettings.SelectedItem.ToString().Contains("Custom"))
            {
                maxWidth.Visible = true;
                maxHeight.Visible = true;
                lbl_x.Visible = true;
            } 
            else
            {
                maxWidth.Visible = false;
                maxHeight.Visible = false;
                lbl_x.Visible = false;
            }
        }
    }
}