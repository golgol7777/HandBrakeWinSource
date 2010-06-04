﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="Size.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the Size type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model
{
    /// <summary>
    /// Picture Size
    /// </summary>
    public class Size
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="Size"/> class.
        /// </summary>
        /// <param name="width">
        /// The width.
        /// </param>
        /// <param name="height">
        /// The height.
        /// </param>
        public Size(int width, int height)
        {
            this.Width = width;
            this.Height = height;
        }

        /// <summary>
        /// Gets or sets Width.
        /// </summary>
        public int Width { get; set; }

        /// <summary>
        /// Gets or sets Height.
        /// </summary>
        public int Height { get; set; }
    }
}
