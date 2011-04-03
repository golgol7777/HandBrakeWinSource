/*  Program.cs
    This file is part of the HandBrake source code.
    Homepage: <http://handbrake.fr>.
    It may be used under the terms of the GNU General Public License. */

namespace Handbrake
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Windows.Forms;

    using HandBrake.ApplicationServices.Services;

    using Handbrake.Properties;
    using Handbrake.ToolWindows;

    /// <summary>
    /// HandBrake Starts Here
    /// </summary>
    public static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        /// <param name="args">
        /// The args.
        /// </param>
        [STAThread]
        public static void Main(string[] args)
        {
            InstanceId = Process.GetProcessesByName("HandBrake").Length;

            // Handle any unhandled exceptions
            AppDomain.CurrentDomain.UnhandledException += CurrentDomainUnhandledException;

            // Check that HandBrakeCLI is availabl.
            string failedInstall = "HandBrake is not installed properly. Please reinstall HandBrake. \n\n";
            string missingFiles = string.Empty;

            // Verify HandBrakeCLI.exe exists
            if (!File.Exists(Path.Combine(Application.StartupPath, "HandBrakeCLI.exe")))
            {
                missingFiles += "\"HandBrakeCLI.exe\" was not found.";
            }

            if (missingFiles != string.Empty)
            {
                MessageBox.Show(
                    failedInstall + missingFiles,
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
                return;
            }

            // Attempt to upgrade / keep the users settings between versions
            try
            {
                if (Settings.Default.UpdateRequired)
                {
                    Settings.Default.Upgrade();
                    Settings.Default.UpdateRequired = false;
                    Settings.Default.Save();
                
                    // Re-detect the CLI version data.
                    Functions.Main.SetCliVersionData();
                }
            }
            catch (Exception)
            {
                Functions.Main.RecoverFromCorruptedLocalApplicationConfig();
                return;
            }

            // Check were not running on a screen that's going to cause some funnies to happen.
            Screen scr = Screen.PrimaryScreen;
            if ((scr.Bounds.Width < 1024) || (scr.Bounds.Height < 620))
            {
                MessageBox.Show(
                    "Your system does not meet the minimum requirements for HandBrake. \n" +
                    "Your screen is running at: " + scr.Bounds.Width + "x" + scr.Bounds.Height +
                    " \nScreen resolution is too Low. Must be 1024x620 or greater.\n\n",
                    "Error",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            } 
            else
            {
                string logDir = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\logs");
                if (!Directory.Exists(logDir))
                    Directory.CreateDirectory(logDir);

                if (!File.Exists(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData), @"HandBrake\presets.xml")))
                {
                    PresetService x = new PresetService();
                    x.UpdateBuiltInPresets(string.Empty);
                }

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new frmMain(args));
            }
        }

        /// <summary>
        /// Throw up an error message for any unhandled exceptions.
        /// </summary>
        /// <param name="sender">The sender</param>
        /// <param name="e">Unhandled Exception EventArgs </param>
        private static void CurrentDomainUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            try
            {
                ExceptionWindow window = new ExceptionWindow();
                window.Setup("An Unknown Error has occured.", e.ExceptionObject.ToString());
                window.ShowDialog();              
            }
            catch (Exception)
            {
                MessageBox.Show(
                    "An Unknown Error has occured. \n\n Exception:" + e.ExceptionObject,
                    "Unhandled Exception",
                    MessageBoxButtons.OK,
                    MessageBoxIcon.Error);
            }

        }

        public static int InstanceId;
    }
}