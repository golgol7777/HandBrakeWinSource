/*  PictureSetting.cs $
 	
 	   This file is part of the HandBrake source code.
 	   Homepage: <http://handbrake.fr>.
 	   It may be used under the terms of the GNU General Public License. */

using System;
using System.ComponentModel;
using System.Drawing;
using System.Globalization;
using System.Windows.Forms;
using Handbrake.Parsing;
using Handbrake.Presets;

namespace Handbrake.Controls
{
    public partial class PictureSettings : UserControl
    {
        private readonly CultureInfo culture = new CultureInfo("en-US", false);
        public event EventHandler PictureSettingsChanged;

        private Boolean preventChangingWidth, preventChangingHeight, preventChangingCustom, preventChangingDisplayWidth;
        private int presetMaximumWidth, presetMaximumHeight;
        private double cachedDar;
        private Title sourceTitle;

        /// <summary>
        /// Creates a new instance of the Picture Settings Class
        /// </summary>
        public PictureSettings()
        {
            InitializeComponent();

            drp_anamorphic.SelectedIndex = 1;
            drp_modulus.SelectedIndex = 0;
        }

        /// <summary>
        /// Gets or sets the source media used by this control.
        /// </summary>
        public Title Source
        {
            private get { return sourceTitle; }
            set
            {
                sourceTitle = value;
                Enabled = sourceTitle != null;
                NewSourceSet();
            }
        }

        /// <summary>
        /// Which preset is currently selected by the user.
        /// </summary>
        public Preset CurrentlySelectedPreset { get; set; }

        /// <summary>
        /// Gets or sets the maximum allowable size for the encoded resolution. Set a value to
        /// "0" if the maximum does not matter.
        /// </summary>
        public Size PresetMaximumResolution
        {
            get { return new Size(presetMaximumWidth, presetMaximumHeight); }
            set
            {
                presetMaximumWidth = value.Width;
                presetMaximumHeight = value.Height;

                if (value.Width != 0 && value.Height != 0)
                    lbl_max.Text = "Max Width / Height";
                else if (value.Width != 0)
                    lbl_max.Text = "Max Width";
                else if (value.Height != 0)
                    lbl_max.Text = "Max Height";
                else
                    lbl_max.Text = "";
            }
        }

        /// <summary>
        /// Set the Preset Crop Warning Label
        /// </summary>
        /// <param name="selectedPreset"></param>
        public void SetPresetCropWarningLabel(Preset selectedPreset)
        {
            lbl_presetCropWarning.Visible = true;
            if (selectedPreset != null && selectedPreset.PictureSettings == false)
                lbl_presetCropWarning.Visible = false;
            else if (selectedPreset == null)
                lbl_presetCropWarning.Visible = false;
        }

        /// <summary>
        /// Setup the UI. The Source has just changed.
        /// </summary>
        private void NewSourceSet()
        {
            // Set the Aspect Ratio
            lbl_Aspect.Text = sourceTitle.AspectRatio.ToString(culture);
            lbl_src_res.Text = sourceTitle.Resolution.Width + " x " + sourceTitle.Resolution.Height;

            // Set the Recommended Cropping values, but only if a preset doesn't have hard set picture settings.
            if ((CurrentlySelectedPreset != null && CurrentlySelectedPreset.PictureSettings == false) || CurrentlySelectedPreset == null)
            {
                crop_top.Value = GetCropMod2Clean(sourceTitle.AutoCropDimensions[0]);
                crop_bottom.Value = GetCropMod2Clean(sourceTitle.AutoCropDimensions[1]);
                crop_left.Value = GetCropMod2Clean(sourceTitle.AutoCropDimensions[2]);
                crop_right.Value = GetCropMod2Clean(sourceTitle.AutoCropDimensions[3]);
            }

            SetPresetCropWarningLabel(CurrentlySelectedPreset);

            // Set the Resolution Boxes
            if (drp_anamorphic.SelectedIndex == 0)
            {
                if (text_width.Value == 0) // Only update the values if the fields don't already have values.
                    text_width.Value = sourceTitle.Resolution.Width;

                check_KeepAR.Checked = true; // Forces Resolution to be correct.
            }
            else
            {
                if (text_width.Value == 0 && text_height.Value == 0)// Only update the values if the fields don't already have values.
                {
                    text_width.Value = sourceTitle.Resolution.Width;
                    text_height.Value = sourceTitle.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;
                }

                labelDisplaySize.Text = CalculateAnamorphicSizes().Width + "x" + CalculateAnamorphicSizes().Height;
            }

            updownParWidth.Value = sourceTitle.ParVal.Width;
            updownParHeight.Value = sourceTitle.ParVal.Height;

            Size croppedDar = CalculateAnamorphicSizes();
            cachedDar = (double)croppedDar.Width / croppedDar.Height;
            updownDisplayWidth.Value = croppedDar.Width;
        }
        
        // Picture Controls
        private void TextWidthValueChanged(object sender, EventArgs e)
        {
            if (Properties.Settings.Default.disableResCalc)
                return;

            if (preventChangingWidth)
                return;

            // Make sure the new value doesn't exceed the maximum
            if (Source != null)
                if (text_width.Value > Source.Resolution.Width)
                    text_width.Value = Source.Resolution.Width;

            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    if (check_KeepAR.Checked && Source != null)
                    {
                        preventChangingHeight = true;

                        int width = (int)text_width.Value;

                        double crop_width = Source.Resolution.Width - (int)crop_left.Value - (int)crop_right.Value;
                        double crop_height = Source.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;

                        if (SourceAspect.Width == 0 && SourceAspect.Height == 0)
                            break;

                        double newHeight = ((double)width * Source.Resolution.Width * SourceAspect.Height * crop_height) /
                                           (Source.Resolution.Height * SourceAspect.Width * crop_width);
                        text_height.Value = (decimal)GetModulusValue(newHeight);

                        preventChangingHeight = false;
                    }
                    break;
                case 3:
                    if (check_KeepAR.CheckState == CheckState.Unchecked && Source != null)
                    {
                        if (preventChangingCustom)
                            break;

                        preventChangingDisplayWidth = true;
                        updownDisplayWidth.Value = text_width.Value * updownParWidth.Value / updownParHeight.Value;
                        preventChangingDisplayWidth = false;

                        labelDisplaySize.Text = Math.Truncate(updownDisplayWidth.Value) + "x" + text_height.Value;
                    }

                    if (check_KeepAR.CheckState == CheckState.Checked && Source != null)
                    {
                        updownParWidth.Value = updownDisplayWidth.Value;
                        updownParHeight.Value = text_width.Value;
                    }
                    break;
                default:
                    labelDisplaySize.Text = CalculateAnamorphicSizes().Width + "x" + CalculateAnamorphicSizes().Height;
                    break;
            }

            preventChangingWidth = false;
        }
        private void TextHeightValueChanged(object sender, EventArgs e)
        {
            if (Properties.Settings.Default.disableResCalc)
                return;

            if (preventChangingHeight)
                return;

            if (Source != null)
                if (text_height.Value > Source.Resolution.Height)
                    text_height.Value = Source.Resolution.Height;

            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    if (check_KeepAR.Checked && Source != null)
                    {
                        preventChangingWidth = true;

                        double crop_width = Source.Resolution.Width - (int)crop_left.Value - (int)crop_right.Value;
                        double crop_height = Source.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;

                        double new_width = ((double)text_height.Value * Source.Resolution.Height * SourceAspect.Width * crop_width) /
                                            (Source.Resolution.Width * SourceAspect.Height * crop_height);

                        text_width.Value = (decimal)GetModulusValue(new_width);

                        preventChangingWidth = false;
                    }
                    break;
                case 3:
                    labelDisplaySize.Text = Math.Truncate(updownDisplayWidth.Value) + "x" + text_height.Value;

                    if (check_KeepAR.CheckState == CheckState.Checked && Source != null)
                    {
                        // - Changes DISPLAY WIDTH to keep DAR
                        // - Changes PIXEL WIDTH to new DISPLAY WIDTH
                        // - Changes PIXEL HEIGHT to STORAGE WIDTH
                        // DAR = DISPLAY WIDTH / DISPLAY HEIGHT (cache after every modification)

                        double rawCalculatedDisplayWidth = (double)text_height.Value * cachedDar;

                        preventChangingDisplayWidth = true; // Start Guards
                        preventChangingWidth = true;

                        updownDisplayWidth.Value = (decimal)rawCalculatedDisplayWidth;
                        updownParWidth.Value = updownDisplayWidth.Value;
                        updownParHeight.Value = text_width.Value;

                        preventChangingWidth = false; // Reset Guards
                        preventChangingDisplayWidth = false;
                    }

                    break;
                default:
                    labelDisplaySize.Text = CalculateAnamorphicSizes().Width + "x" + CalculateAnamorphicSizes().Height;
                    break;
            }

            preventChangingHeight = false;
        }
        private void CheckKeepArCheckedChanged(object sender, EventArgs e)
        {
            if (Properties.Settings.Default.disableResCalc)
                return;

            //Force TextWidth to recalc height
            if (check_KeepAR.Checked)
                TextWidthValueChanged(this, new EventArgs());

            // Disable the Custom Anamorphic Par Controls if checked.
            if (drp_anamorphic.SelectedIndex == 3)
            {
                updownParWidth.Enabled = !check_KeepAR.Checked;
                updownParHeight.Enabled = !check_KeepAR.Checked;
            }

            // Raise the Picture Settings Changed Event
            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }
        private void UpdownDisplayWidthValueChanged(object sender, EventArgs e)
        {
            if (Properties.Settings.Default.disableResCalc)
                return;

            if (preventChangingDisplayWidth == false && check_KeepAR.CheckState == CheckState.Unchecked)
            {
                preventChangingCustom = true;
                updownParWidth.Value = updownDisplayWidth.Value;
                updownParHeight.Value = text_width.Value;
                preventChangingCustom = false;
            }

            if (preventChangingDisplayWidth == false && check_KeepAR.CheckState == CheckState.Checked)
            {
                // - Changes HEIGHT to keep DAR
                // - Changes PIXEL WIDTH to new DISPLAY WIDTH
                // - Changes PIXEL HEIGHT to STORAGE WIDTH
                // DAR = DISPLAY WIDTH / DISPLAY HEIGHT (cache after every modification)

                // Calculate new Height Value
                int modulus;
                if (!int.TryParse(drp_modulus.SelectedItem.ToString(), out modulus))
                    modulus = 16;

                int rawCalculatedHeight = (int)((int)updownDisplayWidth.Value / cachedDar);
                int modulusHeight = rawCalculatedHeight - (rawCalculatedHeight % modulus);

                // Update value
                preventChangingHeight = true;
                text_height.Value = (decimal)modulusHeight;
                updownParWidth.Value = updownDisplayWidth.Value;
                updownParHeight.Value = text_width.Value;
                preventChangingHeight = false;
            }

        }

        // Anamorphic Controls
        private void DrpAnamorphicSelectedIndexChanged(object sender, EventArgs e)
        {
            switch (drp_anamorphic.SelectedIndex)
            {
                case 0: // None
                    text_width.Enabled = true;
                    text_height.Enabled = true;
                    check_KeepAR.Enabled = true;

                    SetCustomAnamorphicOptionsVisible(false);
                    labelStaticDisplaySize.Visible = false;
                    labelDisplaySize.Visible = true;
                    drp_modulus.Visible = true;

                    // check_KeepAR.Checked = true;

                    if (check_KeepAR.Checked)
                        TextWidthValueChanged(this, new EventArgs());
                    // Don't update display size if we're not using anamorphic
                    return;
                case 1: // Strict
                    text_width.Enabled = false;
                    text_height.Enabled = false;
                    check_KeepAR.Enabled = false;

                    SetCustomAnamorphicOptionsVisible(false);
                    labelStaticDisplaySize.Visible = true;
                    labelDisplaySize.Visible = true;

                    check_KeepAR.Checked = true;
                    break;
                case 2: // Loose
                    text_width.Enabled = true;
                    text_height.Enabled = false;
                    check_KeepAR.Enabled = false;

                    SetCustomAnamorphicOptionsVisible(false);
                    labelStaticDisplaySize.Visible = true;
                    labelDisplaySize.Visible = true;
                    drp_modulus.Visible = true;
                    lbl_modulus.Visible = true;

                    check_KeepAR.Checked = true;
                    break;
                case 3: // Custom
                    text_width.Enabled = true;
                    text_height.Enabled = true;
                    check_KeepAR.Enabled = true;

                    SetCustomAnamorphicOptionsVisible(true);
                    labelStaticDisplaySize.Visible = true;
                    labelDisplaySize.Visible = true;

                    check_KeepAR.Checked = true;
                    updownParWidth.Enabled = !check_KeepAR.Checked;
                    updownParHeight.Enabled = !check_KeepAR.Checked;
                    break;

            }

            labelDisplaySize.Text = CalculateAnamorphicSizes().Width + "x" + CalculateAnamorphicSizes().Height;

            if (check_KeepAR.Checked)
                TextWidthValueChanged(this, new EventArgs());

            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }
        private void DrpModulusSelectedIndexChanged(object sender, EventArgs e)
        {
            preventChangingWidth = true;
            preventChangingHeight = true;

            text_width.Value = (decimal)GetModulusValue((double)text_width.Value);
            text_height.Value = (decimal)GetModulusValue((double)text_height.Value);

            preventChangingWidth = false;
            preventChangingHeight = false;

            text_width.Increment = int.Parse(drp_modulus.SelectedItem.ToString());
            text_height.Increment = int.Parse(drp_modulus.SelectedItem.ToString());

            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }

        // Cropping Controls
        private void CheckAutoCropCheckedChanged(object sender, EventArgs e)
        {
            crop_top.Enabled = check_customCrop.Checked;
            crop_bottom.Enabled = check_customCrop.Checked;
            crop_left.Enabled = check_customCrop.Checked;
            crop_right.Enabled = check_customCrop.Checked;

            if (Source != null)
            {
                crop_top.Value = Source.AutoCropDimensions[0];
                crop_bottom.Value = Source.AutoCropDimensions[1];
                crop_left.Value = Source.AutoCropDimensions[2];
                crop_right.Value = Source.AutoCropDimensions[3];
            }
        }
        private void CropValueChanged(object sender, EventArgs e)
        {
            TextWidthValueChanged(this, new EventArgs());
        }

        // GUI Functions
        private void SetCustomAnamorphicOptionsVisible(bool visible)
        {
            lbl_modulus.Visible = visible;
            lbl_displayWidth.Visible = visible;
            lbl_parWidth.Visible = visible;
            lbl_parHeight.Visible = visible;

            drp_modulus.Visible = visible;
            updownDisplayWidth.Visible = visible;
            updownParWidth.Visible = visible;
            updownParHeight.Visible = visible;
        }

        // Calculation Functions
        private Size SourceAspect
        {
            get
            {
                if (Source != null) // display aspect = (width * par_width) / (height * par_height)
                    return new Size((Source.ParVal.Width * Source.Resolution.Width), (Source.ParVal.Height * Source.Resolution.Height));

                return new Size(0, 0); // Fall over to 16:9 and hope for the best
            }
        }
        private Size CalculateAnamorphicSizes()
        {
            if (Source != null)
            {
                /* Set up some variables to make the math easier to follow. */
                int croppedWidth = Source.Resolution.Width - (int)crop_left.Value - (int)crop_right.Value;
                int croppedHeight = Source.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;
                double storageAspect = (double)croppedWidth / croppedHeight;

                /* Figure out what width the source would display at. */
                double sourceDisplayWidth = (double)croppedWidth * Source.ParVal.Width / Source.ParVal.Height;

                /*
                     3 different ways of deciding output dimensions:
                      - 1: Strict anamorphic, preserve source dimensions
                      - 2: Loose anamorphic, round to mod16 and preserve storage aspect ratio
                      - 3: Power user anamorphic, specify everything
                  */
                double width, height;
                switch (drp_anamorphic.SelectedIndex)
                {
                    default:
                    case 1:
                        /* Strict anamorphic */
                        double displayWidth = ((double)croppedWidth * Source.ParVal.Width / Source.ParVal.Height);
                        displayWidth = Math.Round(displayWidth, 0);
                        Size output = new Size((int)displayWidth, croppedHeight);
                        return output;
                    case 2:
                        /* "Loose" anamorphic.
                            - Uses mod16-compliant dimensions,
                            - Allows users to set the width
                        */
                        width = (int)text_width.Value;
                        width = GetModulusValue(width); /* Time to get picture width that divide cleanly.*/

                        height = (width / storageAspect) + 0.5;
                        height = GetModulusValue(height); /* Time to get picture height that divide cleanly.*/

                        /* The film AR is the source's display width / cropped source height.
                           The output display width is the output height * film AR.
                           The output PAR is the output display width / output storage width. */
                        double pixelAspectWidth = height * sourceDisplayWidth / croppedHeight;
                        double pixelAspectHeight = width;

                        double disWidthLoose = (width * pixelAspectWidth / pixelAspectHeight);
                        if (double.IsNaN(disWidthLoose))
                            disWidthLoose = 0;
                        return new Size((int)disWidthLoose, (int)height);
                    case 3:

                        // Get the User Interface Values
                        double UIdisplayWidth;
                        double.TryParse(updownDisplayWidth.Text, out UIdisplayWidth);

                        /* Anamorphic 3: Power User Jamboree - Set everything based on specified values */
                        height = GetModulusValue((double)text_height.Value);

                        if (check_KeepAR.Checked)
                            return new Size((int)Math.Truncate(UIdisplayWidth), (int)height);

                        return new Size((int)Math.Truncate(UIdisplayWidth), (int)height);
                }
            }

            // Return a default value of 0,0 to indicate failure
            return new Size(0, 0);
        }
        private double GetModulusValue(double value)
        {
            int mod = int.Parse(drp_modulus.SelectedItem.ToString());
            double remainder = value % mod;

            if (remainder == 0)
                return value;

            return remainder >= ((double)mod / 2) ? value + (mod - remainder) : value - remainder;
        }
        private static int GetCropMod2Clean(int value)
        {
            int remainder = value % 2;
            if (remainder == 0) return value;
            return (value + remainder);
        }

        // Hidden UI feature to drop the MaxWidth / Height with the MaxWidth/Height label is double clicked
        private void LblMaxDoubleClick(object sender, EventArgs e)
        {
            PresetMaximumResolution = new Size(0, 0);
            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }
    }
}