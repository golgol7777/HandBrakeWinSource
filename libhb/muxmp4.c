/* $Id: muxmp4.c,v 1.24 2005/11/04 13:09:41 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "mp4v2/mp4v2.h"
#include "a52dec/a52.h"

#include "hb.h"

struct hb_mux_object_s
{
    HB_MUX_COMMON;

    hb_job_t * job;

    /* libmp4v2 handle */
    MP4FileHandle file;

    int64_t sum_dur;    // sum of video frame durations so far

    hb_buffer_t *delay_buf;

    /* Chapter state information for muxing */
    MP4TrackId chapter_track;
    int current_chapter;
    uint64_t chapter_duration;
};

struct hb_mux_data_s
{
    MP4TrackId  track;
    uint8_t     subtitle;
    int         sub_format;

    uint64_t    sum_dur; // sum of the frame durations so far
};

/* Tune video track chunk duration.
 * libmp4v2 default duration == dusamplerate == 1 second.
 * Per van's suggestion we desire duration == 4 frames.
 * Should be invoked immediately after track creation.
 *
 * return true on fail, false on success.
 */
static int MP4TuneTrackDurationPerChunk( hb_mux_object_t* m, MP4TrackId trackId )
{
    uint32_t tscale;
    MP4Duration dur;

    tscale = MP4GetTrackTimeScale( m->file, trackId );
    dur = (MP4Duration)ceil( (double)tscale * (double)m->job->vrate_base / (double)m->job->vrate * 4.0 );

    if( !MP4SetTrackDurationPerChunk( m->file, trackId, dur ))
    {
        hb_error( "muxmp4.c: MP4SetTrackDurationPerChunk failed!" );
        *m->job->die = 1;
        return 0;
    }

    hb_deep_log( 2, "muxmp4: track %u, chunk duration %"PRIu64, MP4FindTrackIndex( m->file, trackId ), dur );
    return 1;
}

static const uint16_t ac3_sample_rate_tab[3] = { 48000, 44100, 32000 };
/* possible bitrates */
static const uint16_t ac3_bitrate_tab[19] = {
    32, 40, 48, 56, 64, 80, 96, 112, 128,
    160, 192, 224, 256, 320, 384, 448, 512, 576, 640
};  


/**********************************************************************
 * MP4Init
 **********************************************************************
 * Allocates hb_mux_data_t structures, create file and write headers
 *********************************************************************/
static int MP4Init( hb_mux_object_t * m )
{
    hb_job_t   * job   = m->job;
    hb_title_t * title = job->title;

    hb_audio_t    * audio;
    hb_mux_data_t * mux_data;
    int i;
    int subtitle_default;

    /* Flags for enabling/disabling tracks in an MP4. */
    typedef enum { TRACK_DISABLED = 0x0, TRACK_ENABLED = 0x1, TRACK_IN_MOVIE = 0x2, TRACK_IN_PREVIEW = 0x4, TRACK_IN_POSTER = 0x8}  track_header_flags;

    /* Create an empty mp4 file */
    if (job->largeFileSize)
    /* Use 64-bit MP4 file */
    {
        m->file = MP4Create( job->file, MP4_DETAILS_ERROR, MP4_CREATE_64BIT_DATA );
        hb_deep_log( 2, "muxmp4: using 64-bit MP4 formatting.");
    }
    else
    /* Limit MP4s to less than 4 GB */
    {
        m->file = MP4Create( job->file, MP4_DETAILS_ERROR, 0 );
    }

    if (m->file == MP4_INVALID_FILE_HANDLE)
    {
        hb_error("muxmp4.c: MP4Create failed!");
        *job->die = 1;
        return 0;
    }

    /* Video track */
    mux_data      = calloc(1, sizeof( hb_mux_data_t ) );
    job->mux_data = mux_data;

    if (!(MP4SetTimeScale( m->file, 90000 )))
    {
        hb_error("muxmp4.c: MP4SetTimeScale failed!");
        *job->die = 1;
        return 0;
    }

    if( job->vcodec == HB_VCODEC_X264 )
    {
        /* Stolen from mp4creator */
        MP4SetVideoProfileLevel( m->file, 0x7F );
		mux_data->track = MP4AddH264VideoTrack( m->file, 90000,
		        MP4_INVALID_DURATION, job->width, job->height,
		        job->config.h264.sps[1], /* AVCProfileIndication */
		        job->config.h264.sps[2], /* profile_compat */
		        job->config.h264.sps[3], /* AVCLevelIndication */
		        3 );      /* 4 bytes length before each NAL unit */
        if ( mux_data->track == MP4_INVALID_TRACK_ID )
        {
            hb_error( "muxmp4.c: MP4AddH264VideoTrack failed!" );
            *job->die = 1;
            return 0;
        }

        /* Tune track chunk duration */
        if( !MP4TuneTrackDurationPerChunk( m, mux_data->track ))
        {
            return 0;
        }

        MP4AddH264SequenceParameterSet( m->file, mux_data->track,
                job->config.h264.sps, job->config.h264.sps_length );
        MP4AddH264PictureParameterSet( m->file, mux_data->track,
                job->config.h264.pps, job->config.h264.pps_length );

		if( job->ipod_atom )
		{
			hb_deep_log( 2, "muxmp4: adding iPod atom");
			MP4AddIPodUUID(m->file, mux_data->track);
		}
    }
    else if ( job->vcodec == HB_VCODEC_FFMPEG_MPEG4 ) /* FFmpeg MPEG-4 */
    {
        MP4SetVideoProfileLevel( m->file, MPEG4_SP_L3 );
        mux_data->track = MP4AddVideoTrack( m->file, 90000,
                MP4_INVALID_DURATION, job->width, job->height,
                MP4_MPEG4_VIDEO_TYPE );
        if (mux_data->track == MP4_INVALID_TRACK_ID)
        {
            hb_error("muxmp4.c: MP4AddVideoTrack failed!");
            *job->die = 1;
            return 0;
        }

        /* Tune track chunk duration */
        if( !MP4TuneTrackDurationPerChunk( m, mux_data->track ))
        {
            return 0;
        }

        /* VOL from FFmpeg or XviD */
        if (!(MP4SetTrackESConfiguration( m->file, mux_data->track,
                job->config.mpeg4.bytes, job->config.mpeg4.length )))
        {
            hb_error("muxmp4.c: MP4SetTrackESConfiguration failed!");
            *job->die = 1;
            return 0;
        }
    }
    else if ( job->vcodec == HB_VCODEC_FFMPEG_MPEG2 ) /* FFmpeg MPEG-2 */
    {
        mux_data->track = MP4AddVideoTrack( m->file, 90000,
                MP4_INVALID_DURATION, job->width, job->height,
                MP4_MPEG2_VIDEO_TYPE );
        if (mux_data->track == MP4_INVALID_TRACK_ID)
        {
            hb_error("muxmp4.c: MP4AddVideoTrack failed!");
            *job->die = 1;
            return 0;
        }

        /* Tune track chunk duration */
        if( !MP4TuneTrackDurationPerChunk( m, mux_data->track ))
        {
            return 0;
        }

        /* VOL from FFmpeg */
        if (!(MP4SetTrackESConfiguration( m->file, mux_data->track,
                job->config.mpeg4.bytes, job->config.mpeg4.length )))
        {
            hb_error("muxmp4.c: MP4SetTrackESConfiguration failed!");
            *job->die = 1;
            return 0;
        }
    }
    else
    {
        hb_error("muxmp4.c: Unsupported video encoder!");
    }

    // COLR atom for color and gamma correction.
    // Per the notes at:
    //   http://developer.apple.com/quicktime/icefloe/dispatch019.html#colr
    //   http://forum.doom9.org/showthread.php?t=133982#post1090068
    // the user can set it from job->color_matrix, otherwise by default
    // we say anything that's likely to be HD content is ITU BT.709 and
    // DVD, SD TV & other content is ITU BT.601.  We look at the title height
    // rather than the job height here to get uncropped input dimensions.
    if( job->color_matrix == 1 )
    {
        // ITU BT.601 DVD or SD TV content
        MP4AddColr(m->file, mux_data->track, 6, 1, 6);
    }
    else if( job->color_matrix == 2 )
    {
        // ITU BT.709 HD content
        MP4AddColr(m->file, mux_data->track, 1, 1, 1);        
    }
    else if ( job->title->width >= 1280 || job->title->height >= 720 )
    {
        // we guess that 720p or above is ITU BT.709 HD content
        MP4AddColr(m->file, mux_data->track, 1, 1, 1);
    }
    else
    {
        // ITU BT.601 DVD or SD TV content
        MP4AddColr(m->file, mux_data->track, 6, 1, 6);
    }

    if( job->anamorphic.mode )
    {
        /* PASP atom for anamorphic video */
        float width, height;

        width  = job->anamorphic.par_width;

        height = job->anamorphic.par_height;

        MP4AddPixelAspectRatio(m->file, mux_data->track, (uint32_t)width, (uint32_t)height);

        MP4SetTrackFloatProperty(m->file, mux_data->track, "tkhd.width", job->width * (width / height));
    }

	/* add the audio tracks */
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        mux_data = calloc(1, sizeof( hb_mux_data_t ) );
        audio->priv.mux_data = mux_data;

        if( audio->config.out.codec == HB_ACODEC_AC3_PASS )
        {
            uint8_t bsid = audio->config.in.version;
            uint8_t bsmod = audio->config.in.mode;
            uint8_t acmod = audio->config.flags.ac3 & 0x7;
            uint8_t lfeon = (audio->config.flags.ac3 & A52_LFE) ? 1 : 0;
            uint8_t bit_rate_code = 0;
            int ii, jj;
            int freq = audio->config.in.samplerate;
            int bitrate = audio->config.in.bitrate;
            int sr_shift, sr_code;

            for (ii = 0; ii < 3; ii++)
            {
                for (jj = 0; jj < 3; jj++)
                {
                    if ((ac3_sample_rate_tab[jj] >> ii) == freq)
                    {
                        goto rate_found1;
                    }
                }
            }
            hb_error("Unknown AC3 samplerate");
            ii = jj = 0;
rate_found1:
            sr_shift = ii;
            sr_code = jj;
            for (ii = 0; ii < 19; ii++)
            {
                if ((ac3_bitrate_tab[ii] >> sr_shift)*1000 == bitrate)
                    break;
            }
            if ( ii >= 19 )
            {
                hb_error("Unknown AC3 bitrate");
                ii = 0;
            }
            bit_rate_code = ii;

            mux_data->track = MP4AddAC3AudioTrack(
                m->file,
                audio->config.in.samplerate, 
                sr_code,
                bsid,
                bsmod,
                acmod,
                lfeon,
                bit_rate_code);

            /* Tune track chunk duration */
            MP4TuneTrackDurationPerChunk( m, mux_data->track );

            if (audio->config.out.name == NULL) {
                MP4SetTrackBytesProperty(
                    m->file, mux_data->track,
                    "udta.name.value",
                    (const uint8_t*)"Surround", strlen("Surround"));
            }
            else {
                MP4SetTrackBytesProperty(
                    m->file, mux_data->track,
                    "udta.name.value",
                    (const uint8_t*)(audio->config.out.name),
                    strlen(audio->config.out.name));
            }
        }
        else if( audio->config.out.codec == HB_ACODEC_AC3 )
        {
            uint8_t bsid = 8;
            uint8_t bsmod = 0;
            uint8_t acmod = 2;
            uint8_t lfeon = 0;
            uint8_t bit_rate_code = 0;
            int ii, jj;
            int freq = audio->config.out.samplerate;
            int bitrate = audio->config.out.bitrate;
            int sr_shift, sr_code;

            for (ii = 0; ii < 3; ii++)
            {
                for (jj = 0; jj < 3; jj++)
                {
                    if ((ac3_sample_rate_tab[jj] >> ii) == freq)
                    {
                        goto rate_found2;
                    }
                }
            }
            hb_error("Unknown AC3 samplerate");
            ii = jj = 0;
rate_found2:
            sr_shift = ii;
            sr_code = jj;
            bsid = 8 + ii;
            for (ii = 0; ii < 19; ii++)
            {
                if ((ac3_bitrate_tab[ii] >> sr_shift) == bitrate)
                    break;
            }
            if ( ii >= 19 )
            {
                hb_error("Unknown AC3 bitrate");
                ii = 0;
            }
            bit_rate_code = ii;

            switch( audio->config.out.mixdown )
            {
                case HB_AMIXDOWN_MONO:
                    acmod = 1;
                    break;

                case HB_AMIXDOWN_STEREO:
                case HB_AMIXDOWN_DOLBY:
                case HB_AMIXDOWN_DOLBYPLII:
                    acmod = 2;
                    break;

                case HB_AMIXDOWN_6CH:
                    acmod = 7;
                    lfeon = 1;
                    break;

                default:
                    hb_log(" MP4Init: bad mixdown" );
                    break;
            }

            mux_data->track = MP4AddAC3AudioTrack(
                m->file,
                audio->config.out.samplerate, 
                sr_code,
                bsid,
                bsmod,
                acmod,
                lfeon,
                bit_rate_code);

            /* Tune track chunk duration */
            MP4TuneTrackDurationPerChunk( m, mux_data->track );

            if (audio->config.out.name == NULL) {
                MP4SetTrackBytesProperty(
                    m->file, mux_data->track,
                    "udta.name.value",
                    (const uint8_t*)"Surround", strlen("Surround"));
            }
            else {
                MP4SetTrackBytesProperty(
                    m->file, mux_data->track,
                    "udta.name.value",
                    (const uint8_t*)(audio->config.out.name),
                    strlen(audio->config.out.name));
            }
        } 
        else if( audio->config.out.codec == HB_ACODEC_FAAC ||
                 audio->config.out.codec == HB_ACODEC_CA_AAC ||
                 audio->config.out.codec == HB_ACODEC_CA_HAAC ) 
        {
            int samples_per_frame = ( audio->config.out.codec == HB_ACODEC_CA_HAAC ) ? 2048 : 1024;
            mux_data->track = MP4AddAudioTrack(
                m->file,
                audio->config.out.samplerate, samples_per_frame, MP4_MPEG4_AUDIO_TYPE );

            /* Tune track chunk duration */
            MP4TuneTrackDurationPerChunk( m, mux_data->track );

            if (audio->config.out.name == NULL) {
                MP4SetTrackBytesProperty(
                    m->file, mux_data->track,
                    "udta.name.value",
                    (const uint8_t*)"Stereo", strlen("Stereo"));
            }
            else {
                MP4SetTrackBytesProperty(
                    m->file, mux_data->track,
                    "udta.name.value",
                    (const uint8_t*)(audio->config.out.name),
                    strlen(audio->config.out.name));
            }

            MP4SetAudioProfileLevel( m->file, 0x0F );
            MP4SetTrackESConfiguration(
                m->file, mux_data->track,
                audio->priv.config.aac.bytes, audio->priv.config.aac.length );

            /* Set the correct number of channels for this track */
             MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.mp4a.channels", (uint16_t)HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown));
        } else if( audio->config.out.codec == HB_ACODEC_LAME ) {
            mux_data->track = MP4AddAudioTrack(
                m->file,
                audio->config.out.samplerate, 1152, MP4_MPEG2_AUDIO_TYPE );

            /* Tune track chunk duration */
            MP4TuneTrackDurationPerChunk( m, mux_data->track );

            if (audio->config.out.name == NULL) {
                MP4SetTrackBytesProperty(
                    m->file, mux_data->track,
                    "udta.name.value",
                    (const uint8_t*)"Stereo", strlen("Stereo"));
            }
            else {
                MP4SetTrackBytesProperty(
                    m->file, mux_data->track,
                    "udta.name.value",
                    (const uint8_t*)(audio->config.out.name),
                    strlen(audio->config.out.name));
            }

            MP4SetAudioProfileLevel( m->file, 0x0F );

            /* Set the correct number of channels for this track */
             MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.mp4a.channels", (uint16_t)HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown));
        }

        /* Set the language for this track */
        MP4SetTrackLanguage(m->file, mux_data->track, audio->config.lang.iso639_2);

        if( hb_list_count( title->list_audio ) > 1 )
        {
            /* Set the audio track alternate group */
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.alternate_group", 1);
        }

        if (i == 0) {
            /* Enable the first audio track */
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.flags", (TRACK_ENABLED | TRACK_IN_MOVIE));
        }
        else
            /* Disable the other audio tracks so QuickTime doesn't play
               them all at once. */
        {
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.flags", (TRACK_DISABLED | TRACK_IN_MOVIE));
            hb_deep_log( 2, "muxmp4: disabled extra audio track %u", MP4FindTrackIndex( m->file, mux_data->track ));
        }

    }

    // Quicktime requires that at least one subtitle is enabled,
    // else it doesn't show any of the subtitles.
    // So check to see if any of the subtitles are flagged to be
    // the defualt.  The default will the the enabled track, else
    // enable the first track.
    subtitle_default = 0;
    for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
    {
        hb_subtitle_t *subtitle = hb_list_item( job->list_subtitle, i );

        if( subtitle && subtitle->format == TEXTSUB && 
            subtitle->config.dest == PASSTHRUSUB )
        {
            if ( subtitle->config.default_track )
                subtitle_default = 1;
        }
    }
    for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
    {
        hb_subtitle_t *subtitle = hb_list_item( job->list_subtitle, i );

        if( subtitle && subtitle->format == TEXTSUB && 
            subtitle->config.dest == PASSTHRUSUB )
        {
            uint64_t width, height = 60;
            if( job->anamorphic.mode )
                width = job->width * ( (float) job->anamorphic.par_width / job->anamorphic.par_height );
            else
                width = job->width;

            mux_data = calloc(1, sizeof( hb_mux_data_t ) );
            subtitle->mux_data = mux_data;
            mux_data->subtitle = 1;
            mux_data->sub_format = subtitle->format;
            mux_data->track = MP4AddSubtitleTrack( m->file, 90000, width, height );

            MP4SetTrackLanguage(m->file, mux_data->track, subtitle->iso639_2);

            /* Tune track chunk duration */
            MP4TuneTrackDurationPerChunk( m, mux_data->track );

            const uint8_t textColor[4] = { 255,255,255,255 };

            MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.alternate_group", 2);

            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.dataReferenceIndex", 1);
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.horizontalJustification", 1);
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.verticalJustification", 255);

            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.bgColorAlpha", 255);

            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.defTextBoxBottom", height);
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.defTextBoxRight", width);

            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.fontID", 1);
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.fontSize", 24);

            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.fontColorRed", textColor[0]);
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.fontColorGreen", textColor[1]);
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.fontColorBlue", textColor[2]);
            MP4SetTrackIntegerProperty(m->file, mux_data->track, "mdia.minf.stbl.stsd.tx3g.fontColorAlpha", textColor[3]);
            
            /* translate the track */
            uint8_t* val;
            uint8_t nval[36];
            uint32_t *ptr32 = (uint32_t*) nval;
            uint32_t size;

            MP4GetTrackBytesProperty(m->file, mux_data->track, "tkhd.matrix", &val, &size);
            memcpy(nval, val, size);

            const uint32_t ytranslation = (job->height - height) * 0x10000;
                
#ifdef WORDS_BIGENDIAN
            ptr32[7] = ytranslation;
#else
            /* we need to switch the endianness, as the file format expects big endian */
            ptr32[7] = ((ytranslation & 0x000000FF) << 24) + ((ytranslation & 0x0000FF00) << 8) + 
                            ((ytranslation & 0x00FF0000) >> 8) + ((ytranslation & 0xFF000000) >> 24);
#endif

            MP4SetTrackBytesProperty(m->file, mux_data->track, "tkhd.matrix", nval, size);  
            if ( !subtitle_default || subtitle->config.default_track ) {
                /* Enable the default subtitle track */
                MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.flags", (TRACK_ENABLED | TRACK_IN_MOVIE));
                subtitle_default = 1;
            }
            else
            {
                MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.flags", (TRACK_DISABLED | TRACK_IN_MOVIE));
            }
        }
        else if( subtitle && subtitle->format == PICTURESUB && 
            subtitle->config.dest == PASSTHRUSUB )
        {
            mux_data = calloc(1, sizeof( hb_mux_data_t ) );
            subtitle->mux_data = mux_data;
            mux_data->subtitle = 1;
            mux_data->sub_format = subtitle->format;

            mux_data->track = MP4AddSubpicTrack( m->file, 90000, subtitle->width, subtitle->height );

            MP4SetTrackLanguage(m->file, mux_data->track, subtitle->iso639_2);

            /* Tune track chunk duration */
            MP4TuneTrackDurationPerChunk( m, mux_data->track );
            uint8_t palette[16][4];
            int ii;
            for ( ii = 0; ii < 16; ii++ )
            {
                palette[ii][0] = 0;
                palette[ii][1] = (subtitle->palette[ii] >> 16) & 0xff;
                palette[ii][2] = (subtitle->palette[ii] >> 8) & 0xff;
                palette[ii][3] = (subtitle->palette[ii]) & 0xff;
            }
            if (!(MP4SetTrackESConfiguration( m->file, mux_data->track,
                    (uint8_t*)palette, 16 * 4 )))
            {
                hb_error("muxmp4.c: MP4SetTrackESConfiguration failed!");
                *job->die = 1;
                return 0;
            }
            if ( !subtitle_default || subtitle->config.default_track ) {
                /* Enable the default subtitle track */
                MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.flags", (TRACK_ENABLED | TRACK_IN_MOVIE));
                subtitle_default = 1;
            }
            else
            {
                MP4SetTrackIntegerProperty(m->file, mux_data->track, "tkhd.flags", (TRACK_DISABLED | TRACK_IN_MOVIE));
            }
        }
    }

    if (job->chapter_markers)
    {
        /* add a text track for the chapters. We add the 'chap' atom to track
           one which is usually the video track & should never be disabled.
           The Quicktime spec says it doesn't matter which media track the
           chap atom is on but it has to be an enabled track. */
        MP4TrackId textTrack;
        textTrack = MP4AddChapterTextTrack(m->file, 1, 0);

        m->chapter_track = textTrack;
        m->chapter_duration = 0;
        m->current_chapter = job->chapter_start;
    }

    /* Add encoded-by metadata listing version and build date */
    char *tool_string;
    tool_string = (char *)malloc(80);
    snprintf( tool_string, 80, "HandBrake %s %i", HB_PROJECT_VERSION, HB_PROJECT_BUILD);

    /* allocate,fetch,populate,store,free tags structure */
    const MP4Tags* tags;
    tags = MP4TagsAlloc();
    MP4TagsFetch( tags, m->file );
    MP4TagsSetEncodingTool( tags, tool_string );
    MP4TagsStore( tags, m->file );
    MP4TagsFree( tags );

    free(tool_string);

    return 0;
}

typedef struct stylerecord_s {
    enum style_s {ITALIC, BOLD, UNDERLINE} style;
    uint16_t start;
    uint16_t stop;
    struct stylerecord_s *next;
} stylerecord;

static void hb_makestylerecord( stylerecord **stack, 
                                enum style_s style, int start )
{
    stylerecord *record = calloc( sizeof( stylerecord ), 1 );

    if( record ) 
    {
        record->style = style;
        record->start = start;
        record->next = *stack;
        *stack = record;
    }
}

static void hb_makestyleatom( stylerecord *record, uint8_t *style)
{
    uint8_t face = 1;
    hb_deep_log(3, "Made style '%s' from %d to %d", 
           record->style == ITALIC ? "Italic" : record->style == BOLD ? "Bold" : "Underline", record->start, record->stop);
    
    switch( record->style )
    {
    case ITALIC:
        face = 2;
        break;
    case BOLD:
        face = 1;
        break;
    case UNDERLINE:
        face = 4;
        break;
    default:
        face = 2;
        break;
    }

    style[0] = (record->start >> 8) & 0xff; // startChar
    style[1] = record->start & 0xff;
    style[2] = (record->stop >> 8) & 0xff;   // endChar
    style[3] = record->stop & 0xff;
    style[4] = (1 >> 8) & 0xff;    // font-ID
    style[5] = 1 & 0xff;
    style[6] = face;   // face-style-flags: 1 bold; 2 italic; 4 underline
    style[7] = 24;      // font-size
    style[8] = 255;     // r
    style[9] = 255;     // g
    style[10] = 255;    // b
    style[11] = 255;    // a
 
}

/*
 * Copy the input to output removing markup and adding markup to the style
 * atom where appropriate.
 */
static void hb_muxmp4_process_subtitle_style( uint8_t *input,
                                              uint8_t *output,
                                              uint8_t *style, uint16_t *stylesize )
{
    uint8_t *reader = input;
    uint8_t *writer = output;
    uint8_t stylecount = 0;
    uint16_t utf8_count = 0;         // utf8 count from start of subtitle
    stylerecord *stylestack = NULL;
    stylerecord *oldrecord = NULL;
    
    while(*reader != '\0') {
        if( ( *reader & 0xc0 ) == 0x80 ) 
        {
            /*
             * Track the utf8_count when doing markup so that we get the tx3g stops
             * based on UTF8 chr counts rather than bytes.
             */
            utf8_count++;
            hb_deep_log( 3, "MuxMP4: Counted %d UTF-8 chrs within subtitle so far", 
                             utf8_count);
        }
        if (*reader == '<') {
            /*
             * possible markup, peek at the next chr
             */
            switch(*(reader+1)) {
            case 'i':
                if (*(reader+2) == '>') {
                    reader += 3;
                    hb_makestylerecord(&stylestack, ITALIC, (writer - output - utf8_count));
                } else {
                    *writer++ = *reader++;
                }
                break;
            case 'b':
                if (*(reader+2) == '>') {
                    reader += 3; 
                    hb_makestylerecord(&stylestack, BOLD, (writer - output - utf8_count));
                } else {
                    *writer++ = *reader++;  
                }
                break;
            case 'u': 
                if (*(reader+2) == '>') {
                    reader += 3;
                    hb_makestylerecord(&stylestack, UNDERLINE, (writer - output - utf8_count));
                } else {
                    *writer++ = *reader++;
                }
                break;
            case '/':
                switch(*(reader+2)) {
                case 'i':
                    if (*(reader+3) == '>') {
                        /*
                         * Check whether we then immediately start more markup of the same type, if so then
                         * lets not close it now and instead continue this markup.
                         */
                        if ((*(reader+4) && *(reader+4) == '<') &&
                            (*(reader+5) && *(reader+5) == 'i') &&
                            (*(reader+6) && *(reader+6) == '>')) {
                            /*
                             * Opening italics right after, so don't close off these italics.
                             */
                            hb_deep_log(3, "Joining two sets of italics");
                            reader += (4 + 3);
                            continue;
                        }


                        if ((*(reader+4) && *(reader+4) == ' ') && 
                            (*(reader+5) && *(reader+5) == '<') &&
                            (*(reader+6) && *(reader+6) == 'i') &&
                            (*(reader+7) && *(reader+7) == '>')) {
                            /*
                             * Opening italics right after, so don't close off these italics.
                             */
                            hb_deep_log(3, "Joining two sets of italics (plus space)");
                            reader += (4 + 4);
                            *writer++ = ' ';
                            continue;
                        }
                        if (stylestack && stylestack->style == ITALIC) {
                            uint8_t style_record[12];
                            stylestack->stop = writer - output - utf8_count;
                            hb_makestyleatom(stylestack, style_record);

                            memcpy(style + 10 + (12 * stylecount), style_record, 12);
                            stylecount++;

                            oldrecord = stylestack;
                            stylestack = stylestack->next;
                            free(oldrecord);
                        } else {
                            hb_error("Mismatched Subtitle markup '%s'", input);
                        }
                        reader += 4;
                    } else {
                        *writer++ = *reader++;
                    }
                    break;
                case 'b':
                    if (*(reader+3) == '>') {
                        if (stylestack && stylestack->style == BOLD) {
                            uint8_t style_record[12];
                            stylestack->stop = writer - output - utf8_count;
                            hb_makestyleatom(stylestack, style_record);

                            memcpy(style + 10 + (12 * stylecount), style_record, 12);
                            stylecount++;
                            oldrecord = stylestack;
                            stylestack = stylestack->next;
                            free(oldrecord);
                        } else {
                            hb_error("Mismatched Subtitle markup '%s'", input);
                        }

                        reader += 4;
                    } else {
                        *writer++ = *reader++;
                    }
                    break;
                case 'u': 
                    if (*(reader+3) == '>') {
                        if (stylestack && stylestack->style == UNDERLINE) {
                            uint8_t style_record[12];
                            stylestack->stop = writer - output - utf8_count;
                            hb_makestyleatom(stylestack, style_record);

                            memcpy(style + 10 + (12 * stylecount), style_record, 12);
                            stylecount++;

                            oldrecord = stylestack;
                            stylestack = stylestack->next;
                            free(oldrecord);
                        } else {
                            hb_error("Mismatched Subtitle markup '%s'", input);
                        }
                        reader += 4;
                    } else {
                        *writer++ = *reader++;
                    }
                    break;
                default:
                    *writer++ = *reader++;
                    break;
                }
                break;
            default:
                *writer++ = *reader++;
                break;
            }
        } else {
            *writer++ = *reader++;
        }
    }
    *writer = '\0';

    if( stylecount )
    {
        *stylesize = 10 + ( stylecount * 12 );

        memcpy( style + 4, "styl", 4);

        style[0] = 0;
        style[1] = 0;
        style[2] = (*stylesize >> 8) & 0xff;
        style[3] = *stylesize & 0xff;
        style[8] = (stylecount >> 8) & 0xff;
        style[9] = stylecount & 0xff;

    }

}

static int MP4Mux( hb_mux_object_t * m, hb_mux_data_t * mux_data,
                   hb_buffer_t * buf )
{
    hb_job_t * job = m->job;
    int64_t duration;
    int64_t offset = 0;
    hb_buffer_t *tmp;

    if( mux_data == job->mux_data )
    {
        /* Video */

        if( job->vcodec == HB_VCODEC_X264 ||
            ( job->vcodec & HB_VCODEC_FFMPEG_MASK ) )
        {
            if ( buf && buf->start < buf->renderOffset )
            {
                hb_log("MP4Mux: PTS %"PRId64" < DTS %"PRId64,
                       buf->start, buf->renderOffset );
                buf->renderOffset = buf->start;
            }
        }

        // We delay muxing video by one frame so that we can calculate
        // the dts to dts duration of the frames.
        tmp = buf;
        buf = m->delay_buf;
        m->delay_buf = tmp;

        if ( !buf )
            return 0;

        if( job->vcodec == HB_VCODEC_X264 ||
            ( job->vcodec & HB_VCODEC_FFMPEG_MASK ) )
        {
            // x264 supplies us with DTS, so offset is PTS - DTS
            offset = buf->start - buf->renderOffset;
        }

        /* Add the sample before the new frame.
           It is important that this be calculated prior to the duration
           of the new video sample, as we want to sync to right after it.
           (This is because of how durations for text tracks work in QT) */
        if( job->chapter_markers && buf->new_chap )
        {    
            hb_chapter_t *chapter = NULL;

            // this chapter is postioned by writing out the previous chapter.
            // the duration of the previous chapter is the duration up to but
            // not including the current frame minus the duration of all
            // chapters up to the previous.
            // The initial and final chapters can be very short (a second or
            // less) since they're not really chapters but just a placeholder to
            // insert a cell command. We don't write chapters shorter than 1.5 sec.
            duration = m->sum_dur - m->chapter_duration + offset;
            if ( duration >= (90000*3)/2 )
            {
                chapter = hb_list_item( m->job->title->list_chapter,
                                        buf->new_chap - 2 );

                MP4AddChapter( m->file,
                               m->chapter_track,
                               duration,
                               (chapter != NULL) ? chapter->title : NULL);

                m->current_chapter = buf->new_chap;
                m->chapter_duration += duration;
            }
        }

        if( job->vcodec == HB_VCODEC_X264 ||
            ( job->vcodec & HB_VCODEC_FFMPEG_MASK ) )
        {
            // x264 supplies us with DTS
            if ( m->delay_buf )
            {
                duration = m->delay_buf->renderOffset - buf->renderOffset;
            }
            else
            {
                duration = buf->stop - m->sum_dur;
                // Due to how libx264 generates DTS, it's possible for the
                // above calculation to be negative. 
                //
                // x264 generates DTS by rearranging PTS in this sequence:
                // pts0 - delay, pts1 - delay, pts2 - delay, pts1, pts2, pts3...
                //
                // where delay == pts2.  This guarantees that DTS <= PTS for
                // any frame, but also generates this sequence of durations:
                // d0 + d1 + d0 + d1 + d2 + d3 ... + d(N-2)
                // 
                // so the sum up to the last frame is:
                // sum_dur = d0 + d1 + d0 + d1 + d2 + d3 ... + d(N-3)
                //
                // while the original total duration of the video was:
                // duration = d0 + d1 + d2 + d3 ... + d(N)
                //
                // Note that if d0 + d1 != d(N-1) + d(N), the total
                // length of the video changes since d(N-1) and d(N) are
                // replaced by d0 and d1 in the final duration sum.
                //
                // To keep the total length of the video the same as the source
                // we try to make 
                // d(N-2) = duration - sum_dur
                //
                // But if d0 + d1 >= d(N-1) + d(N), the above calculation
                // results in a nagative value and we need to fix it.
                if ( duration <= 0 )
                    duration = 90000. / ((double)job->vrate / (double)job->vrate_base);
            }
        }
        else
        {
            // We're getting the frames in decode order but the timestamps are
            // for presentation so we have to use durations and effectively
            // compute a DTS.
            duration = buf->stop - buf->start;
        }

        if ( duration <= 0 )
        {
            /* We got an illegal mp4/h264 duration. This shouldn't
               be possible and usually indicates a bug in the upstream code.
               Complain in the hope that someone will go find the bug but
               try to fix the error so that the file will still be playable. */
            hb_log("MP4Mux: illegal duration %"PRId64", start %"PRId64","
                   "stop %"PRId64", sum_dur %"PRId64,
                   duration, buf->start, buf->stop, m->sum_dur );
            /* we don't know when the next frame starts so we can't pick a
               valid duration for this one. we pick something "short"
               (roughly 1/3 of an NTSC frame time) to take time from
               the next frame. */
            duration = 1000;
        }
        m->sum_dur += duration;
    }
    else
    {
        /* Audio */
        duration = MP4_INVALID_DURATION;
    }

    /* Here's where the sample actually gets muxed. */
    if( ( job->vcodec == HB_VCODEC_X264 ||
        ( job->vcodec & HB_VCODEC_FFMPEG_MASK ) )
        && mux_data == job->mux_data )
    {
        /* Compute dependency flags.
         *
         * This mechanism is (optionally) used by media players such as QuickTime
         * to offer better scrubbing performance. The most influential bits are
         * MP4_SDT_HAS_NO_DEPENDENTS and MP4_SDT_EARLIER_DISPLAY_TIMES_ALLOWED.
         *
         * Other bits are possible but no example media using such bits have been
         * found.
         *
         * It is acceptable to supply 0-bits for any samples which characteristics
         * cannot be positively guaranteed.
         */
        int sync = 0;
        uint32_t dflags = 0;

        /* encoding layer signals if frame is referenced by other frames */
        if( buf->flags & HB_FRAME_REF )
            dflags |= MP4_SDT_HAS_DEPENDENTS;
        else
            dflags |= MP4_SDT_HAS_NO_DEPENDENTS; /* disposable */

        switch( buf->frametype )
        {
            case HB_FRAME_IDR:
                sync = 1;
                break;
            case HB_FRAME_I:
                dflags |= MP4_SDT_EARLIER_DISPLAY_TIMES_ALLOWED;
                break;
            case HB_FRAME_P:
                dflags |= MP4_SDT_EARLIER_DISPLAY_TIMES_ALLOWED;
                break;
            case HB_FRAME_BREF:
            case HB_FRAME_B:
            default:
                break; /* nothing to mark */
        }

        if( !MP4WriteSampleDependency( m->file,
                                       mux_data->track,
                                       buf->data,
                                       buf->size,
                                       duration,
                                       offset,
                                       sync,
                                       dflags ))
        {
            hb_error("Failed to write to output file, disk full?");
            *job->die = 1;
        }
    }
    else if (mux_data->subtitle)
    {
        if( mux_data->sub_format == TEXTSUB )
        {
            /* MPEG4 timed text does not allow overlapping samples; upstream
               code should coalesce overlapping subtitle lines. */
            if( buf->start < mux_data->sum_dur )
            {
                if ( buf->stop - mux_data->sum_dur > 90*500 )
                {
                    hb_log("MP4Mux: shortening overlapping subtitle, "
                           "start %"PRId64", stop %"PRId64", sum_dur %"PRId64,
                           buf->start, buf->stop, m->sum_dur);
                    buf->start = mux_data->sum_dur;
                }
            }
            if( buf->start < mux_data->sum_dur )
            {
                hb_log("MP4Mux: skipping overlapping subtitle, "
                       "start %"PRId64", stop %"PRId64", sum_dur %"PRId64,
                       buf->start, buf->stop, m->sum_dur);
            }
            else
            {
                /* Write an empty sample */
                if ( mux_data->sum_dur < buf->start )
                {
                    uint8_t empty[2] = {0,0};
                    if( !MP4WriteSample( m->file,
                                        mux_data->track,
                                        empty,
                                        2,
                                        buf->start - mux_data->sum_dur,
                                        0,
                                        1 ))
                    {
                        hb_error("Failed to write to output file, disk full?");
                        *job->die = 1;
                    }
                    mux_data->sum_dur += buf->start - mux_data->sum_dur;
                }
                uint8_t styleatom[2048];;
                uint16_t stylesize = 0;
                uint8_t buffer[2048];
                uint16_t buffersize = 0;
                uint8_t output[2048];

                *buffer = '\0';

                /*
                 * Copy the subtitle into buffer stripping markup and creating
                 * style atoms for them.
                 */
                hb_muxmp4_process_subtitle_style( buf->data,
                                                  buffer,
                                                  styleatom, &stylesize );

                buffersize = strlen((char*)buffer);

                hb_deep_log(3, "MuxMP4:Sub:%fs:%"PRId64":%"PRId64":%"PRId64": %s",
                            (float)buf->start / 90000, buf->start, buf->stop,
                            (buf->stop - buf->start), buffer);

                /* Write the subtitle sample */
                memcpy( output + 2, buffer, buffersize );
                memcpy( output + 2 + buffersize, styleatom, stylesize);
                output[0] = ( buffersize >> 8 ) & 0xff;
                output[1] = buffersize & 0xff;

                if( !MP4WriteSample( m->file,
                                     mux_data->track,
                                     output,
                                     buffersize + stylesize + 2,
                                     buf->stop - buf->start,
                                     0,
                                     1 ))
                {
                    hb_error("Failed to write to output file, disk full?");
                    *job->die = 1;
                }

                mux_data->sum_dur += (buf->stop - buf->start);
            }
        }
        else if( mux_data->sub_format == PICTURESUB )
        {
            /* Write an empty sample */
            if ( mux_data->sum_dur < buf->start )
            {
                uint8_t empty[2] = {0,0};
                if( !MP4WriteSample( m->file,
                                    mux_data->track,
                                    empty,
                                    2,
                                    buf->start - mux_data->sum_dur,
                                    0,
                                    1 ))
                {
                    hb_error("Failed to write to output file, disk full?");
                    *job->die = 1;
                } 
                mux_data->sum_dur += buf->start - mux_data->sum_dur;
            }
            if( !MP4WriteSample( m->file,
                                 mux_data->track,
                                 buf->data,
                                 buf->size,
                                 buf->stop - buf->start,
                                 0,
                                 1 ))
            {
                hb_error("Failed to write to output file, disk full?");
                *job->die = 1;
            }

            mux_data->sum_dur += (buf->stop - buf->start);
        }
    }
    else
    {
        /*
         * Audio
         */
        if( !MP4WriteSample( m->file,
                             mux_data->track,
                             buf->data,
                             buf->size,
                             duration,
                             offset,
                             ( buf->frametype & HB_FRAME_KEY ) != 0 ))
        {
            hb_error("Failed to write to output file, disk full?");
            *job->die = 1;
        }
    }
    hb_buffer_close( &buf );

    return 0;
}

static int MP4End( hb_mux_object_t * m )
{
    hb_job_t   * job   = m->job;
    hb_title_t * title = job->title;

    // Flush the delayed frame
    if ( m->delay_buf )
        MP4Mux( m, job->mux_data, NULL );

    /* Write our final chapter marker */
    if( m->job->chapter_markers )
    {
        hb_chapter_t *chapter = NULL;
        int64_t duration = m->sum_dur - m->chapter_duration;
        /* The final chapter can have a very short duration - if it's less
         * than 1.5 seconds just skip it. */
        if ( duration >= (90000*3)/2 )
        {

            chapter = hb_list_item( m->job->title->list_chapter,
                                    m->current_chapter - 1 );

            MP4AddChapter( m->file,
                           m->chapter_track,
                           duration,
                           (chapter != NULL) ? chapter->title : NULL);
        }
    }

    if ( job->config.h264.init_delay )
    {
           // Insert track edit to get A/V back in sync.  The edit amount is
           // the init_delay.
           int64_t edit_amt = job->config.h264.init_delay;
           MP4AddTrackEdit(m->file, 1, MP4_INVALID_EDIT_ID, edit_amt,
                           MP4GetTrackDuration(m->file, 1), 0);
            if ( m->job->chapter_markers )
            {
                // apply same edit to chapter track to keep it in sync with video
                MP4AddTrackEdit(m->file, m->chapter_track, MP4_INVALID_EDIT_ID,
                                edit_amt,
                                MP4GetTrackDuration(m->file, m->chapter_track), 0);
            }
     }

    /*
     * Write the MP4 iTunes metadata if we have any metadata
     */
    if( title->metadata )
    {
        hb_metadata_t *md = title->metadata;
        const MP4Tags* tags;

        hb_deep_log( 2, "Writing Metadata to output file...");

        /* allocate tags structure */
        tags = MP4TagsAlloc();
        /* fetch data from MP4 file (in case it already has some data) */
        MP4TagsFetch( tags, m->file );

        /* populate */
        if( strlen( md->name ))
            MP4TagsSetName( tags, md->name );
        if( strlen( md->artist ))
            MP4TagsSetArtist( tags, md->artist );
        if( strlen( md->composer ))
            MP4TagsSetComposer( tags, md->composer );
        if( strlen( md->comment ))
            MP4TagsSetComments( tags, md->comment );
        if( strlen( md->release_date ))
            MP4TagsSetReleaseDate( tags, md->release_date );
        if( strlen( md->album ))
            MP4TagsSetAlbum( tags, md->album );
        if( strlen( md->genre ))
            MP4TagsSetGenre( tags, md->genre );

        if( md->coverart )
        {
            MP4TagArtwork art;
            art.data = md->coverart;
            art.size = md->coverart_size;
            art.type = MP4_ART_UNDEFINED; // delegate typing to libmp4v2
            MP4TagsAddArtwork( tags, &art );
        }

        /* push data to MP4 file */
        MP4TagsStore( tags, m->file );
        /* free memory associated with structure */
        MP4TagsFree( tags );
    }

    MP4Close( m->file );

    if ( job->mp4_optimize )
    {
        hb_log( "muxmp4: optimizing file" );
        char filename[1024]; memset( filename, 0, 1024 );
        snprintf( filename, 1024, "%s.tmp", job->file );
        MP4Optimize( job->file, filename, MP4_DETAILS_ERROR );
        remove( job->file );
        rename( filename, job->file );
    }

    return 0;
}

hb_mux_object_t * hb_mux_mp4_init( hb_job_t * job )
{
    hb_mux_object_t * m = calloc( sizeof( hb_mux_object_t ), 1 );
    m->init      = MP4Init;
    m->mux       = MP4Mux;
    m->end       = MP4End;
    m->job       = job;
    return m;
}

