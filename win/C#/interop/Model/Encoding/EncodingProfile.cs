﻿// --------------------------------------------------------------------------------------------------------------------
// <copyright file="EncodingProfile.cs" company="HandBrake Project (http://handbrake.fr)">
//   This file is part of the HandBrake source code - It may be used under the terms of the GNU General Public License.
// </copyright>
// <summary>
//   Defines the EncodingProfile type.
// </summary>

namespace HandBrake.Interop.Model.Encoding
{
    using System.Collections.Generic;

    public class EncodingProfile
    {
        public EncodingProfile()
        {
            this.Cropping = new Cropping();
        }

        public OutputFormat OutputFormat { get; set; }
        public OutputExtension PreferredExtension { get; set; }
        public bool IncludeChapterMarkers { get; set; }
        public bool LargeFile { get; set; }
        public bool Optimize { get; set; }
        public bool IPod5GSupport { get; set; }

        public int Width { get; set; }
        public int Height { get; set; }
        public int MaxWidth { get; set; }
        public int MaxHeight { get; set; }
        public bool CustomCropping { get; set; }
        public Cropping Cropping { get; set; }
        public Anamorphic Anamorphic { get; set; }
        public bool UseDisplayWidth { get; set; }
        public int DisplayWidth { get; set; }
        public bool KeepDisplayAspect { get; set; }
        public int PixelAspectX { get; set; }
        public int PixelAspectY { get; set; }
        public int Modulus { get; set; }

        public Deinterlace Deinterlace { get; set; }
        public string CustomDeinterlace { get; set; }
        public Decomb Decomb { get; set; }
        public string CustomDecomb { get; set; }
        public Detelecine Detelecine { get; set; }
        public string CustomDetelecine { get; set; }
        public Denoise Denoise { get; set; }
        public string CustomDenoise { get; set; }
        public int Deblock { get; set; }
        public bool Grayscale { get; set; }

        public VideoEncoder VideoEncoder { get; set; }
        public string X264Options { get; set; }
        public VideoEncodeRateType VideoEncodeRateType { get; set; }
        public double Quality { get; set; }
        public int TargetSize { get; set; }
        public int VideoBitrate { get; set; }
        public bool TwoPass { get; set; }
        public bool TurboFirstPass { get; set; }
        public double Framerate { get; set; }

        public List<AudioEncoding> AudioEncodings { get; set; }

        public EncodingProfile Clone()
        {
            EncodingProfile profile = new EncodingProfile
            {
                OutputFormat = this.OutputFormat,
                PreferredExtension = this.PreferredExtension,
                IncludeChapterMarkers = this.IncludeChapterMarkers,
                LargeFile = this.LargeFile,
                Optimize = this.Optimize,
                IPod5GSupport = this.IPod5GSupport,

                Width = this.Width,
                Height = this.Height,
                MaxWidth = this.MaxWidth,
                MaxHeight = this.MaxHeight,
                CustomCropping = this.CustomCropping,
                Cropping = this.Cropping,
                Anamorphic = this.Anamorphic,
                UseDisplayWidth = this.UseDisplayWidth,
                DisplayWidth = this.DisplayWidth,
                KeepDisplayAspect = this.KeepDisplayAspect,
                PixelAspectX = this.PixelAspectX,
                PixelAspectY = this.PixelAspectY,
                Modulus = this.Modulus,

                Deinterlace = this.Deinterlace,
                CustomDeinterlace = this.CustomDeinterlace,
                Decomb = this.Decomb,
                CustomDecomb = this.CustomDecomb,
                Detelecine = this.Detelecine,
                CustomDetelecine = this.CustomDetelecine,
                Denoise = this.Denoise,
                CustomDenoise = this.CustomDenoise,
                Deblock = this.Deblock,
                Grayscale = this.Grayscale,

                VideoEncoder = this.VideoEncoder,
                X264Options = this.X264Options,
                VideoEncodeRateType = this.VideoEncodeRateType,
                Quality = this.Quality,
                TargetSize = this.TargetSize,
                VideoBitrate = this.VideoBitrate,
                TwoPass = this.TwoPass,
                TurboFirstPass = this.TurboFirstPass,
                Framerate = this.Framerate,

                AudioEncodings = new List<AudioEncoding>(this.AudioEncodings)
            };

            return profile;
        }
    }
}
