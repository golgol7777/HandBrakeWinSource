// --------------------------------------------------------------------------------------------------------------------
// <copyright file="ColorSpaceConverter.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the ColorSpaceConverter type.
// </summary>
// --------------------------------------------------------------------------------------------------------------------

namespace HandBrake.Interop.Model.Encoding
{
    /// <summary>
    /// The ColorSpaceConverter Filters
    /// </summary>
    public enum ColorSpaceConverter
    {
        Off = 0,
        Convert709to601,
        Convert601to709
    }
}
