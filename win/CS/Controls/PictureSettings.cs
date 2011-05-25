/*  PictureSetting.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.Controls
{
    using System;
    using System.Drawing;
    using System.Globalization;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Model;
    using HandBrake.ApplicationServices.Parsing;

    /// <summary>
    /// The Picture Settings Panel
    /// </summary>
    public partial class PictureSettings : UserControl
    {
        private readonly CultureInfo culture = new CultureInfo("en-US", false);
        private bool preventChangingWidth;
        private bool preventChangingHeight;
        private bool preventChangingCustom;
        private bool preventChangingDisplayWidth;
        private decimal cachedDar;
        private decimal cachedPar;
        private Title sourceTitle;

        /// <summary>
        /// Initializes a new instance of the <see cref="PictureSettings"/> class. 
        /// Creates a new instance of the Picture Settings Class
        /// </summary>
        public PictureSettings()
        {
            InitializeComponent();

            drp_anamorphic.SelectedIndex = 1;
            drp_modulus.SelectedIndex = 0;
        }

        /// <summary>
        /// Picture Settings Changed Event Handler
        /// </summary>
        public event EventHandler PictureSettingsChanged;

        /// <summary>
        /// Gets or sets the source media used by this control.
        /// </summary>
        public Title Source
        {
            private get
            {
                return sourceTitle;
            }

            set
            {
                sourceTitle = value;
                Enabled = sourceTitle != null;
                NewSourceSet();
            }
        }

        /// <summary>
        /// Gets or sets the currently selected preset.
        /// </summary>
        public Preset CurrentlySelectedPreset { get; set; }

        /// <summary>
        /// Gets or sets the maximum allowable size for the encoded resolution. Set a value to
        /// "0" if the maximum does not matter.
        /// </summary>
        public Size PresetMaximumResolution { get; set; }

        /// <summary>
        /// Set the Preset Crop Warning Label
        /// </summary>
        /// <param name="selectedPreset">
        /// The Selected preset
        /// </param>
        public void SetPresetCropWarningLabel(Preset selectedPreset)
        {
            if (this.check_customCrop.Checked)
            {
                lbl_presetCropWarning.Visible = true;
                if (selectedPreset != null && selectedPreset.CropSettings == false) lbl_presetCropWarning.Visible = false;
                else if (selectedPreset == null) lbl_presetCropWarning.Visible = false;
            }
            else
            {
                lbl_presetCropWarning.Visible = false;
            }
        }

        /// <summary>
        /// Setup the UI. The Source has just changed.
        /// </summary>
        private void NewSourceSet()
        {
            // Set the Aspect Ratio
            lbl_src_res.Text = sourceTitle.Resolution.Width + " x " + sourceTitle.Resolution.Height;

            // Calculate source DAR and PAR
            cachedDar = (decimal)SourceAspect.Width / SourceAspect.Height;
            cachedPar = (decimal)SourcePixelAspect.Width / SourcePixelAspect.Height;

            // Set the Recommended Cropping values, but only if a preset doesn't have hard set picture settings.
            if ((CurrentlySelectedPreset != null && CurrentlySelectedPreset.CropSettings == false) || CurrentlySelectedPreset == null)
            {
                crop_top.Value = GetCropMod2Clean(sourceTitle.AutoCropDimensions.Top);
                crop_bottom.Value = GetCropMod2Clean(sourceTitle.AutoCropDimensions.Bottom);
                crop_left.Value = GetCropMod2Clean(sourceTitle.AutoCropDimensions.Left);
                crop_right.Value = GetCropMod2Clean(sourceTitle.AutoCropDimensions.Right);
            }

            SetPresetCropWarningLabel(CurrentlySelectedPreset);

            // Set Anamorphic control as per preset settings
            SetAnamorphicGUIControlState();

            // Enable/Disable padding control as per preset settings
            SetPaddingGUIControlState();

            // Enable or Disable ITU-PAR Checkbox
            if (sourceTitle.Resolution.Width == 720 &&
                (sourceTitle.Resolution.Height == 480 || sourceTitle.Resolution.Height == 576))
            {
                check_UseITUPar.Enabled = true;
            }
            else
            {
                check_UseITUPar.Enabled = false;
                check_UseITUPar.Checked = false;
            }

            // Set the Resolution Boxes
            if (drp_anamorphic.SelectedIndex == 0)
            {
                int width = sourceTitle.Resolution.Width;

                if (width > PresetMaximumResolution.Width && PresetMaximumResolution.Width != 0) // If the preset has a Max width set, don't use a width larger than it.
                    width = PresetMaximumResolution.Width;

                if (text_width.Value == 0 || (text_width.Value > PresetMaximumResolution.Width && PresetMaximumResolution.Width != 0)) // Only update the values if the fields don't already have values.
                    text_width.Value = width;

                check_KeepAR.Checked = true; // Forces Resolution to be correct.
            }
            else
            {
                decimal width = sourceTitle.Resolution.Width - crop_left.Value - crop_right.Value;
                if (width > PresetMaximumResolution.Width && PresetMaximumResolution.Width != 0) // If the preset has a Max width set, don't use a width larger than it.
                    width = PresetMaximumResolution.Width;

                decimal height = sourceTitle.Resolution.Height - crop_top.Value - crop_bottom.Value;
                if (height > PresetMaximumResolution.Height && PresetMaximumResolution.Height != 0) // If the preset has a Max width set, don't use a width larger than it.
                    height = PresetMaximumResolution.Height;

                switch (drp_anamorphic.SelectedIndex)
                {
                    case 1:
                        text_width.Value = sourceTitle.Resolution.Width - crop_left.Value - crop_right.Value;
                        text_height.Value = sourceTitle.Resolution.Height - crop_top.Value - crop_bottom.Value;
                        break;
                    case 2:
                        text_width.Value = width;
                        text_height.Value = GetModulusValue(width * sourceTitle.Resolution.Height / sourceTitle.Resolution.Width);
                        break;
                    case 3:
                        if (text_width.Value == 0 && (PresetMaximumResolution.Width != 0 && text_width.Value > PresetMaximumResolution.Width)) // Only update the values if the fields don't already have values.
                            text_width.Value = width;
                        if (text_height.Value == 0 && (PresetMaximumResolution.Height != 0 && text_height.Value > PresetMaximumResolution.Height)) // Only update the values if the fields don't already have values.
                            text_height.Value = height;

                        if (!check_KeepAR.Checked)
                        {
                            preventChangingDisplayWidth = true;
                            updownDisplayWidth.Value = text_width.Value * updownParWidth.Value / updownParHeight.Value;
                            preventChangingDisplayWidth = false;
                        }
                        else
                        {
                            updownDisplayWidth.Value = text_height.Value * cachedDar;
                        }
                        break;
                }
            }

            UpdateLabelDisplaySizeText(check_enablePad.Checked);
        }

        // Picture Controls
        private void TextWidthValueChanged(object sender, EventArgs e)
        {
            if (Source == null)
                return;

            if (preventChangingWidth)
                return;

            // Make sure the new value doesn't exceed the maximum
            if (text_width.Value > Source.Resolution.Width)
               text_width.Value = Source.Resolution.Width;

            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    if (check_KeepAR.Checked)
                    {
                        decimal crop_dar = ((Source.Resolution.Width - crop_left.Value - crop_right.Value) * cachedPar)
                                             / (Source.Resolution.Height - crop_top.Value - crop_bottom.Value);
                        decimal newHeight = text_width.Value / crop_dar;
 
                        preventChangingHeight = true;
                        text_height.Value = GetModulusValue(newHeight);

                        preventChangingHeight = false;
                    }
                    break;
                case 1:
                    preventChangingWidth = true;
                    preventChangingHeight = true;
                    text_width.Value = Source.Resolution.Width - crop_left.Value - crop_right.Value;
                    text_height.Value = Source.Resolution.Height - crop_top.Value - crop_bottom.Value;
                    preventChangingWidth = false;
                    preventChangingHeight = false;
                    break;
                case 2:
                    decimal storage_aspect = (Source.Resolution.Width - crop_left.Value - crop_right.Value)
                                               / (Source.Resolution.Height - crop_top.Value - crop_bottom.Value);

                    preventChangingHeight = true;
                    text_height.Value = GetModulusValue(text_width.Value / storage_aspect);
                    preventChangingHeight = false;
                    break;
                case 3:
                    if (preventChangingCustom)
                        break;

                    if (check_KeepAR.Checked)
                    {
                        preventChangingCustom = true;
                        updownParWidth.Value = updownDisplayWidth.Value;
                        updownParHeight.Value = text_width.Value;
                        preventChangingCustom = false;
                    }
                    else
                    {
                        decimal display_width = text_width.Value * updownParWidth.Value / updownParHeight.Value;

                        display_width = ClipValueRange(display_width, updownDisplayWidth.Minimum, updownDisplayWidth.Maximum);

                        preventChangingDisplayWidth = true;
                        preventChangingCustom = true;
                        updownDisplayWidth.Value = display_width;
                        preventChangingDisplayWidth = false;
                        preventChangingCustom = false;
                    }
                    break;
                default:
                    break;
            }

            UpdateLabelDisplaySizeText(check_enablePad.Checked);

            preventChangingWidth = false;
        }

        private void TextHeightValueChanged(object sender, EventArgs e)
        {
            if (Source == null)
                return;

            if (preventChangingHeight)
                return;

            if (text_height.Value > Source.Resolution.Height)
               text_height.Value = Source.Resolution.Height;

            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    if (check_KeepAR.Checked)
                    {
                        decimal crop_width = Source.Resolution.Width - crop_left.Value - crop_right.Value;
                        decimal crop_height = Source.Resolution.Height - crop_top.Value - crop_bottom.Value;
                        decimal crop_dar = crop_width * cachedPar / crop_height;
                        decimal newWidth = text_height.Value * crop_dar;

                        preventChangingWidth = true;
                        text_width.Value = GetModulusValue(newWidth);
                        preventChangingWidth = false;
                    }
                    break;
                case 3:
                    if (preventChangingCustom)
                        break;

                    if (check_KeepAR.Checked)
                    {
                        // - Changes DISPLAY WIDTH to keep DAR
                        // - Changes PIXEL WIDTH to new DISPLAY WIDTH
                        // - Changes PIXEL HEIGHT to STORAGE WIDTH
                        // DAR = DISPLAY WIDTH / DISPLAY HEIGHT (cache after every modification)

                        decimal crop_width = sourceTitle.Resolution.Width - crop_left.Value - crop_right.Value;
                        decimal crop_height = sourceTitle.Resolution.Height - crop_top.Value - crop_bottom.Value;
                        decimal crop_dar = crop_width * cachedPar / crop_height;
                        decimal display_width = text_height.Value * crop_dar;

                        display_width = ClipValueRange(display_width, updownDisplayWidth.Minimum, updownDisplayWidth.Maximum);
                        updownDisplayWidth.Value = display_width;

                        preventChangingCustom = true;
                        updownParWidth.Value = updownDisplayWidth.Value;
                        updownParHeight.Value = text_width.Value;

                        preventChangingCustom = false;
                    }
                    break;
                default:
                    break;
            }

            UpdateLabelDisplaySizeText(check_enablePad.Checked);

            preventChangingHeight = false;
        }

        private void CheckKeepArCheckedChanged(object sender, EventArgs e)
        {
            SetAnamorphicGUIControlState();
            SetPaddingGUIControlState();

            if (Source == null)
                return;

            // Force TextWidth to recalc height
            if (check_KeepAR.Checked)
                TextWidthValueChanged(this, new EventArgs());

            UpdateLabelDisplaySizeText(check_enablePad.Checked);

            // Raise the Picture Settings Changed Event
            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }

        private void UpdownDisplayWidthValueChanged(object sender, EventArgs e)
        {
            if (Source == null)
                return;

            if (preventChangingDisplayWidth)
                return;

            if (!check_KeepAR.Checked)
            {
                preventChangingCustom = true;
                updownParWidth.Value = updownDisplayWidth.Value;
                updownParHeight.Value = text_width.Value;
                preventChangingCustom = false;
            }
            else
            {
                // - Changes HEIGHT to keep DAR
                // - Changes PIXEL WIDTH to new DISPLAY WIDTH
                // - Changes PIXEL HEIGHT to STORAGE WIDTH
                // DAR = DISPLAY WIDTH / DISPLAY HEIGHT (cache after every modification)

                decimal crop_width = sourceTitle.Resolution.Width - crop_left.Value - crop_right.Value;
                decimal crop_height = sourceTitle.Resolution.Height - crop_top.Value - crop_bottom.Value;
                decimal crop_dar = crop_width * cachedPar / crop_height;
                decimal display_height = updownDisplayWidth.Value / crop_dar;

                // Update value
                preventChangingHeight = true;
                preventChangingCustom = true;

                text_height.Value = GetModulusValue(display_height);

                updownParWidth.Value = updownDisplayWidth.Value;
                updownParHeight.Value = text_width.Value;

                preventChangingHeight = false;
                preventChangingCustom = false;
            }

            UpdateLabelDisplaySizeText(check_enablePad.Checked);

            preventChangingDisplayWidth = false;
        }

        private void DrpColorMatrixSelectedIndexChanged(object sender, EventArgs e)
        {
            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }

        // Anamorphic Controls
        private void DrpAnamorphicSelectedIndexChanged(object sender, EventArgs e)
        {
            SetAnamorphicGUIControlState();
            SetPaddingGUIControlState();

            if (Source == null)
                return;

            if (check_KeepAR.Checked)
                TextWidthValueChanged(this, new EventArgs());

            UpdateLabelDisplaySizeText(check_enablePad.Checked);

            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }

        private void DrpModulusSelectedIndexChanged(object sender, EventArgs e)
        {
            decimal modulus = int.Parse(drp_modulus.SelectedItem.ToString());

            preventChangingWidth = true;
            preventChangingHeight = true;

            text_width.Value = GetModulusValue(text_width.Value);
            text_height.Value = GetModulusValue(text_height.Value);

            preventChangingWidth = false;
            preventChangingHeight = false;

            text_width.Increment = modulus;
            text_height.Increment = modulus;

            pad_top.Value = GetModulusValue(pad_top.Value);
            pad_bottom.Value = GetModulusValue(pad_bottom.Value);
            pad_right.Value = GetModulusValue(pad_right.Value);
            pad_left.Value = GetModulusValue(pad_left.Value);

            pad_top.Increment = modulus;
            pad_bottom.Increment = modulus;
            pad_left.Increment = modulus;
            pad_right.Increment = modulus;

            if (check_KeepAR.Checked)
                TextWidthValueChanged(this, new EventArgs());

            UpdateLabelDisplaySizeText(check_enablePad.Checked);

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
                crop_top.Value = Source.AutoCropDimensions.Top;
                crop_bottom.Value = Source.AutoCropDimensions.Bottom;
                crop_left.Value = Source.AutoCropDimensions.Left;
                crop_right.Value = Source.AutoCropDimensions.Right;
            }
        }

        private void CropValueChanged(object sender, EventArgs e)
        {
            TextWidthValueChanged(this, new EventArgs());
        }

        private void CheckEnablePadChanged(object sender, EventArgs e)
        {
            if (check_enablePad.Checked)
                SetUpdownPadValueEnable(true);
            else
                SetUpdownPadValueEnable(false);

            UpdateLabelDisplaySizeText(check_enablePad.Checked);
        }

        private void PadValueChanged(object sender, EventArgs e)
        {
            UpdateLabelDisplaySizeText(check_enablePad.Checked);
        }

        // GUI Functions
        private void SetAnamorphicGUIControlState()
        {
            text_width.Enabled = drp_anamorphic.SelectedIndex != 1;
            lbl_modulus.Visible = drp_anamorphic.SelectedIndex != 1;
            drp_modulus.Visible = drp_anamorphic.SelectedIndex != 1;

            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                    text_height.Enabled = true;
                    check_KeepAR.Enabled = true;
                    //check_KeepAR.Checked = true;
                    SetCustomAnamorphicOptionsVisible(false);
                    break;
                case 1:
                    text_height.Enabled = false;
                    check_KeepAR.Enabled = false;
                    check_KeepAR.Checked = true;
                    SetCustomAnamorphicOptionsVisible(false);
                    break;
                case 2:
                    text_height.Enabled = false;
                    check_KeepAR.Enabled = false;
                    check_KeepAR.Checked = true;
                    SetCustomAnamorphicOptionsVisible(false);
                    break;
                case 3:
                    text_height.Enabled = true;
                    check_KeepAR.Enabled = true;
                    //check_KeepAR.Checked = true;
                    SetCustomAnamorphicOptionsVisible(true);

                    // Disable the Custom Anamorphic Par Controls if KeepAR checked.
                    updownParWidth.Enabled = !check_KeepAR.Checked;
                    updownParHeight.Enabled = !check_KeepAR.Checked;
                    break;
                default:
                    break;
            }
        }

        private void SetPaddingGUIControlState()
        {
            switch (drp_anamorphic.SelectedIndex)
            {
                case 0:
                case 3:
                    tbl_padding.Enabled = true;
                    check_enablePad.Enabled = true;
                    check_disablePad.Checked = !check_enablePad.Checked;
                    break;
                case 1:
                case 2:
                default:
                    tbl_padding.Enabled = false;
                    check_enablePad.Checked = false;
                    check_enablePad.Enabled = false;
                    check_disablePad.Checked = true;
                    break;
            }

            SetUpdownPadValueEnable(check_enablePad.Checked);
        }

        private void SetUpdownPadValueEnable(bool enable)
        {
            pad_top.Enabled = enable;
            pad_bottom.Enabled = enable;
            pad_left.Enabled = enable;
            pad_right.Enabled = enable;

            if (enable)
            {
                pad_top.Value = GetModulusValue(pad_top.Value);
                pad_bottom.Value = GetModulusValue(pad_bottom.Value);
                pad_right.Value = GetModulusValue(pad_right.Value);
                pad_left.Value = GetModulusValue(pad_left.Value);

                decimal mod = int.Parse(drp_modulus.SelectedItem.ToString());

                pad_top.Increment = mod;
                pad_bottom.Increment = mod;
                pad_right.Increment = mod;
                pad_left.Increment = mod;
            }
        }

        private void SetCustomAnamorphicOptionsVisible(bool visible)
        {
            lbl_displayWidth.Visible = visible;
            lbl_parWidth.Visible = visible;
            lbl_parHeight.Visible = visible;

            updownDisplayWidth.Visible = visible;
            updownParWidth.Visible = visible;
            updownParHeight.Visible = visible;
        }

        private void UpdateLabelDisplaySizeText(bool pad_enabled)
        {
            labelDisplaySize.Text = CalculateAnamorphicSizes().Width + "x" + CalculateAnamorphicSizes().Height;

            if (!pad_enabled)
                return;

            if (pad_top.Value != 0 || pad_bottom.Value != 0 || pad_left.Value != 0 || pad_right.Value != 0)
                labelDisplaySize.Text += " (" + CalculateAnamorphicSizesWithPad().Width + "x" + CalculateAnamorphicSizesWithPad().Height + " with padding)";
        }

        // Calculation Functions
        private Size SourceAspect
        {
            get
            {
                if (Source != null) // display aspect = (width * par_width) / (height * par_height)
                {
                    if (check_UseITUPar.Checked && sourceTitle.Resolution.Width == 720
                        && (sourceTitle.Resolution.Height == 480 || sourceTitle.Resolution.Height == 576))
                    {
                        int iaspect = (int)(sourceTitle.AspectRatio * 9.0);

                        if (iaspect == 16)
                            return new Size(20, 11);
                        if (iaspect == 12)
                            return new Size(15, 11);
                    }

                    return new Size((Source.ParVal.Width * Source.Resolution.Width),
                                    (Source.ParVal.Height * Source.Resolution.Height));
                }

                return new Size(0, 0); // Fall over to 16:9 and hope for the best
            }
        }

        private Size SourcePixelAspect
        {
            get
            {
                if (Source != null)
                {
                    if (check_UseITUPar.Checked && sourceTitle.Resolution.Width == 720
                        && (sourceTitle.Resolution.Height == 480 || sourceTitle.Resolution.Height == 576))
                    {
                        int iaspect = (int)(sourceTitle.AspectRatio * 9.0);

                        if (iaspect == 16)
                        {
                            if (sourceTitle.Resolution.Height == 480)
                                return new Size(40, 33);
                            else if (sourceTitle.Resolution.Height == 576)
                                return new Size(16, 11);
                        }
                        if (iaspect == 12)
                        {
                            if (sourceTitle.Resolution.Height == 480)
                                return new Size(10, 11);
                            else if (sourceTitle.Resolution.Height == 576)
                                return new Size(12, 11);
                        }
                    }

                    return new Size(Source.ParVal.Width, Source.ParVal.Height);

                }
                return new Size(0, 0);
            }
        }

        private Size CalculateAnamorphicSizes()
        {
            if (Source != null)
            {
                /* Set up some variables to make the math easier to follow. */
                int croppedWidth = Source.Resolution.Width - (int)crop_left.Value - (int)crop_right.Value;
                int croppedHeight = Source.Resolution.Height - (int)crop_top.Value - (int)crop_bottom.Value;
                decimal storageAspect = (decimal)croppedWidth / croppedHeight;

                /* Figure out what width the source would display at. */
                decimal sourceDisplayWidth = croppedWidth * cachedPar;

                /*
                     3 different ways of deciding output dimensions:
                      - 1: Strict anamorphic, preserve source dimensions
                      - 2: Loose anamorphic, round to mod16 and preserve storage aspect ratio
                      - 3: Power user anamorphic, specify everything
                  */
                decimal width, height;
                switch (drp_anamorphic.SelectedIndex)
                {
                    case 0:
                        /* None anamorphic */
                        return new Size((int)text_width.Value, (int)text_height.Value);
                    case 1:
                        /* Strict anamorphic */
                        decimal displayWidth = croppedWidth * cachedPar;
                        return new Size((int)Math.Round(displayWidth, 0), croppedHeight);
                    case 2:
                        /* "Loose" anamorphic.
                            - Uses mod16-compliant dimensions,
                            - Allows users to set the width
                        */
                        width = text_width.Value;
                        height = GetModulusValue(width / storageAspect + 0.5m); /* Get picture height that divide cleanly.*/

                        /* The film AR is the source's display width / cropped source height.
                           The output display width is the output height * film AR.
                           The output PAR is the output display width / output storage width. */
                        decimal pixelAspectWidth = height * sourceDisplayWidth / croppedHeight;
                        decimal pixelAspectHeight = width;

                        decimal disWidthLoose = width * pixelAspectWidth / pixelAspectHeight;
                        if (double.IsNaN((double)disWidthLoose))
                            disWidthLoose = 0;
                        return new Size((int)disWidthLoose, (int)height);
                    case 3:
                        /* Anamorphic 3: Power User Jamboree - Set everything based on specified values */
                        // Get the User Interface Values
                        double UIdisplayWidth;
                        width = GetModulusValue(text_width.Value);

                        UIdisplayWidth = (double)updownDisplayWidth.Value;
                        return new Size((int)Math.Round(UIdisplayWidth, 0), (int)text_height.Value);
                }
            }

            // Return a default value of 0,0 to indicate failure
            return new Size(0, 0);
        }

        private Size CalculateAnamorphicSizesWithPad()
        {
            if (Source != null)
            {
                decimal total_display_width;
                switch (drp_anamorphic.SelectedIndex)
                {
                    case 0:
                        return new Size((int)(text_width.Value+pad_left.Value+pad_right.Value),
                                        (int)(text_height.Value+pad_top.Value+pad_bottom.Value));
                    case 3:
                        decimal width  = text_width.Value;
                        decimal height = text_height.Value;
                        decimal total_storage_width = width + pad_left.Value + pad_right.Value;
                        decimal total_storage_height = height + pad_top.Value + pad_bottom.Value;

                        if (check_KeepAR.Checked)
                        {
                            decimal multiple = updownDisplayWidth.Value / width;
                            total_display_width = multiple * total_storage_width;
                        }
                        else
                        {
                            decimal par = updownParWidth.Value / updownParHeight.Value;
                            total_display_width = par * total_storage_width;
                            if (double.IsNaN((double)total_display_width))
                                total_display_width = 0;
                        }
                        return new Size((int)Math.Round(total_display_width), (int)total_storage_height);
                    default:
                        return CalculateAnamorphicSizes();
                }
            }

            // Return a default value of 0,0 to indicate failure
            return new Size(0, 0);
        }

        private decimal GetModulusValue(decimal value)
        {
            int mod = int.Parse(drp_modulus.SelectedItem.ToString());
            decimal remainder = value % mod;

            if (remainder == 0)
                return value;

            return remainder >= (mod / 2) ? value + (mod - remainder) : value - remainder;
        }

        private static int GetCropMod2Clean(int value)
        {
            int remainder = value % 2;
            if (remainder == 0) return value;
            return (value + remainder);
        }

        private decimal ClipValueRange(decimal value, decimal min, decimal max)
        {
            if (value > max)
                return max;
            else if (value < min)
                return min;
            else
                return value;
        }

        private void CheckITUParCheckedChanged(object sender, EventArgs e)
        {
            lbl_useITUPar.Visible = check_UseITUPar.Checked;

            if (Source == null)
                return;

            if (preventChangingCustom)
                return;

            preventChangingCustom = true;
            cachedDar = (decimal)SourceAspect.Width / SourceAspect.Height;
            cachedPar = (decimal)SourcePixelAspect.Width / SourcePixelAspect.Height;
            preventChangingCustom = false;

            if (check_KeepAR.Checked)
                TextWidthValueChanged(this, new EventArgs());

            UpdateLabelDisplaySizeText(check_enablePad.Checked);

            if (PictureSettingsChanged != null)
                PictureSettingsChanged(this, new EventArgs());
        }
    }
}