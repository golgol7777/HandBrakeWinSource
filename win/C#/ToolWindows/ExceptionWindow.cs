﻿/*  ExceptionWindow.cs $
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake.ToolWindows
{
    using System;
    using System.Windows.Forms;

    /// <summary>
    /// A window to display Exceptions in a form which can be easily copied and reported by users.
    /// </summary>
    public partial class ExceptionWindow : Form
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ExceptionWindow"/> class.
        /// </summary>
        public ExceptionWindow()
        {
            InitializeComponent();
        }

        /// <summary>
        /// Setup the window with the error message.
        /// </summary>
        /// <param name="shortError">
        /// The short error.
        /// </param>
        /// <param name="longError">
        /// The long error.
        /// </param>
        public void Setup(string shortError, string longError)
        {
            lbl_shortError.Text = shortError;
            rtf_exceptionFull.Text = shortError + Environment.NewLine + longError;
        }

        /// <summary>
        /// Copy the Exception Information to the Clipboard.
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnCopyClick(object sender, EventArgs e)
        {
            Clipboard.SetDataObject(rtf_exceptionFull.SelectedText != string.Empty ? rtf_exceptionFull.SelectedText : rtf_exceptionFull.Text, true);
        }

        /// <summary>
        /// Copy from the right click menu
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void MnuCopyLogClick(object sender, EventArgs e)
        {
            Clipboard.SetDataObject(rtf_exceptionFull.SelectedText != string.Empty ? rtf_exceptionFull.SelectedText : rtf_exceptionFull.Text, true);
        } 

        /// <summary>
        /// Close the window
        /// </summary>
        /// <param name="sender">
        /// The sender.
        /// </param>
        /// <param name="e">
        /// The e.
        /// </param>
        private void BtnCloseClick(object sender, EventArgs e)
        {
            this.Close();
        }
    }
}
