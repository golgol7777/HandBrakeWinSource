// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Padding.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Padding type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.ApplicationServices.Model.Encoding
{
    /// <summary>
    /// Padding T B L R
    /// </summary>
    public class Padding
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Padding"/> class. 
        /// </summary>
        public Padding()
        {
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="Padding"/> class. 
        /// </summary>
        /// <param name="top">
        /// The Top Value
        /// </param>
        /// <param name="bottom">
        /// The Bottom Value
        /// </param>
        /// <param name="left">
        /// The Left Value
        /// </param>
        /// <param name="right">
        /// The Right Value
        /// </param>
        public Padding(int top, int bottom, int left, int right)
        {
            this.Top = top;
            this.Bottom = bottom;
            this.Left = left;
            this.Right = right;
        }

        /// <summary>
        /// Gets or sets Top.
        /// </summary>
        public int? Top { get; set; }

        /// <summary>
        /// Gets or sets Bottom.
        /// </summary>
        public int? Bottom { get; set; }

        /// <summary>
        /// Gets or sets Left.
        /// </summary>
        public int? Left { get; set; }

        /// <summary>
        /// Gets or sets Right.
        /// </summary>
        public int? Right { get; set; }
    }
}
