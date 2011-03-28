/* $Id: encavcodec.c,v 1.23 2005/10/13 23:47:06 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "hbffmpeg.h"

/*
 * The frame info struct remembers information about each frame across calls
 * to avcodec_encode_video. Since frames are uniquely identified by their
 * frame number, we use this as an index.
 *
 * The size of the array is chosen so that two frames can't use the same 
 * slot during the encoder's max frame delay (set by the standard as 16 
 * frames) and so that, up to some minimum frame rate, frames are guaranteed
 * to map to * different slots.
 */
#define FRAME_INFO_SIZE 32
#define FRAME_INFO_MASK (FRAME_INFO_SIZE - 1)

struct hb_work_private_s
{
    hb_job_t * job;
    AVCodecContext * context;
    FILE * file;

    int frameno_in;
    int frameno_out;
    hb_buffer_t * delay_head;
    hb_buffer_t * delay_tail;

    int64_t dts_delay;

    struct {
        int64_t start;
        int64_t stop;
        int64_t renderOffset;
    } frame_info[FRAME_INFO_SIZE];
};

int  encavcodecInit( hb_work_object_t *, hb_job_t * );
int  encavcodecWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encavcodecClose( hb_work_object_t * );

hb_work_object_t hb_encavcodec =
{
    WORK_ENCAVCODEC,
    "FFMPEG encoder (libavcodec)",
    encavcodecInit,
    encavcodecWork,
    encavcodecClose
};

int encavcodecInit( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec * codec;
    AVCodecContext * context;
    AVRational fps;

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job = job;

    switch ( w->codec_param )
    {
        case CODEC_ID_MPEG4:
        {
            hb_log("encavcodecInit: MPEG-4 ASP encoder");
        } break;
        case CODEC_ID_MPEG2VIDEO:
        {
            hb_log("encavcodecInit: MPEG-2 encoder");
        } break;
        default:
        {
            hb_error("encavcodecInit: unsupported encoder!");
            return 1;
        }
    }

    codec = avcodec_find_encoder( w->codec_param  );
    if( !codec )
    {
        hb_log( "encavcodecInit: avcodec_find_encoder "
                "failed" );
    }
    context = avcodec_alloc_context3( codec );

    // Set things in context that we will allow the user to 
    // override with advanced settings.
    context->thread_count = ( hb_get_cpu_count() * 3 / 2 );

    if( job->pass == 2 )
    {
        hb_interjob_t * interjob = hb_interjob_get( job->h );
        fps.den = interjob->vrate_base;
        fps.num = interjob->vrate;
    }
    else
    {
        fps.den = job->vrate_base;
        fps.num = job->vrate;
    }

    // If the fps.num is 27000000, there's a good chance this is
    // a standard rate that we have in our hb_video_rates table.
    // Because of rounding errors and approximations made while 
    // measuring framerate, the actual value may not be exact.  So
    // we look for rates that are "close" and make an adjustment
    // to fps.den.
    if (fps.num == 27000000)
    {
        int ii;
        for (ii = 0; ii < hb_video_rates_count; ii++)
        {
            if (abs(fps.den - hb_video_rates[ii].rate) < 10)
            {
                fps.den = hb_video_rates[ii].rate;
                break;
            }
        }
    }
    hb_reduce(&fps.den, &fps.num, fps.den, fps.num);

    // Check that the framerate is supported.  If not, pick the closest.
    // The mpeg2 codec only supports a specific list of frame rates.
    if (codec->supported_framerates)
    {
        AVRational supported_fps;
        supported_fps = codec->supported_framerates[av_find_nearest_q_idx(fps, codec->supported_framerates)];
        if (supported_fps.num != fps.num || supported_fps.den != fps.den)
        {
            hb_log( "encavcodec: framerate %d / %d is not supported. Using %d / %d.", 
                    fps.num, fps.den, supported_fps.num, supported_fps.den );
            fps = supported_fps;
        }
    }
    else if ((fps.num & ~0xFFFF) || (fps.den & ~0xFFFF))
    {
        // This may only be required for mpeg4 video. But since
        // our only supported options are mpeg2 and mpeg4, there is
        // no need to check codec type.
        hb_log( "encavcodec: truncating framerate %d / %d", 
                fps.num, fps.den );
        while ((fps.num & ~0xFFFF) || (fps.den & ~0xFFFF))
        {
            fps.num >>= 1;
            fps.den >>= 1;
        }
    }

    context->time_base.den = fps.num;
    context->time_base.num = fps.den;
    context->gop_size  = 10 * (int)( (double)job->vrate / (double)job->vrate_base + 0.5 );

    /*
        This section passes the string advanced_opts to avutil for parsing 
        into an AVCodecContext.

        The string is set up like this:
        option1=value1:option2=value2

        So, you have to iterate through based on the colons, and then put
        the left side of the equals sign in "name" and the right side into
        "value." Then you hand those strings off to avutil for interpretation.
     */
    if( job->advanced_opts != NULL && *job->advanced_opts != '\0' )
    {
        char *opts, *opts_start;

        opts = opts_start = strdup(job->advanced_opts);

        if( opts_start )
        {
            while( *opts )
            {
                char *name = opts;
                char *value;
                int ret;

                opts += strcspn( opts, ":" );
                if( *opts )
                {
                    *opts = 0;
                    opts++;
                }

                value = strchr( name, '=' );
                if( value )
                {
                    *value = 0;
                    value++;
                }

                /* Here's where the strings are passed to avutil for parsing. */
                ret = av_set_string3( context, name, value, 1, NULL );

                /* Let avutil sanity check the options for us*/
                if( ret == AVERROR(ENOENT) )
                    hb_log( "avcodec options: Unknown option %s", name );
                if( ret == AVERROR(EINVAL) )
                    hb_log( "avcodec options: Bad argument %s=%s", name, value ? value : "(null)" );
            }
        }
        free(opts_start);
    }

    // Now set the things in context that we don't want to allow
    // the user to override.
    if( job->vquality < 0.0 )
    {
        /* Average bitrate */
        context->bit_rate = 1000 * job->vbitrate;
        // ffmpeg's mpeg2 encoder requires that the bit_rate_tolerance be >=
        // bitrate * fps
        context->bit_rate_tolerance = context->bit_rate * av_q2d(fps) + 1;
    }
    else
    {
        /* Constant quantizer */
        // These settings produce better image quality than
        // what was previously used
        context->flags |= CODEC_FLAG_QSCALE;
        context->global_quality = FF_QP2LAMBDA * job->vquality + 0.5;
        hb_log( "encavcodec: encoding at constant quantizer %d",
                context->global_quality );
    }
    context->width     = job->width;
    context->height    = job->height;
    context->pix_fmt   = PIX_FMT_YUV420P;

    if( job->anamorphic.mode )
    {
        context->sample_aspect_ratio.num = job->anamorphic.par_width;
        context->sample_aspect_ratio.den = job->anamorphic.par_height;

        hb_log( "encavcodec: encoding with stored aspect %d/%d",
                job->anamorphic.par_width, job->anamorphic.par_height );
    }

    if( job->mux & HB_MUX_MP4 )
    {
        context->flags |= CODEC_FLAG_GLOBAL_HEADER;
    }
    if( job->grayscale )
    {
        context->flags |= CODEC_FLAG_GRAY;
    }

    if( job->pass != 0 && job->pass != -1 )
    {
        char filename[1024]; memset( filename, 0, 1024 );
        hb_get_tempory_filename( job->h, filename, "ffmpeg.log" );

        if( job->pass == 1 )
        {
            pv->file = fopen( filename, "wb" );
            context->flags |= CODEC_FLAG_PASS1;
        }
        else
        {
            int    size;
            char * log;

            pv->file = fopen( filename, "rb" );
            fseek( pv->file, 0, SEEK_END );
            size = ftell( pv->file );
            fseek( pv->file, 0, SEEK_SET );
            log = malloc( size + 1 );
            log[size] = '\0';
            fread( log, size, 1, pv->file );
            fclose( pv->file );
            pv->file = NULL;

            context->flags    |= CODEC_FLAG_PASS2;
            context->stats_in  = log;
        }
    }

    if( hb_avcodec_open( context, codec ) )
    {
        hb_log( "encavcodecInit: avcodec_open failed" );
    }
    pv->context = context;

    job->areBframes = 0;
    if ( context->has_b_frames )
    {
        job->areBframes = 1;
    }
    if( ( job->mux & HB_MUX_MP4 ) && job->pass != 1 )
    {
        w->config->mpeg4.length = context->extradata_size;
        memcpy( w->config->mpeg4.bytes, context->extradata,
                context->extradata_size );
    }

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encavcodecClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if( pv->context && pv->context->codec )
    {
        hb_deep_log( 2, "encavcodec: closing libavcodec" );
        avcodec_flush_buffers( pv->context );
        hb_avcodec_close( pv->context );
    }
    if( pv->file )
    {
        fclose( pv->file );
    }
    free( pv );
    w->private_data = NULL;
}

/*
 * see comments in definition of 'frame_info' in pv struct for description
 * of what these routines are doing.
 */
static void save_frame_info( hb_work_private_t * pv, hb_buffer_t * in )
{
    int i = pv->frameno_in & FRAME_INFO_MASK;
    pv->frame_info[i].start = in->start;
    pv->frame_info[i].stop = in->stop;
}

static int64_t get_frame_start( hb_work_private_t * pv, int64_t frameno )
{
    int i = frameno & FRAME_INFO_MASK;
    return pv->frame_info[i].start;
}

static int64_t get_frame_stop( hb_work_private_t * pv, int64_t frameno )
{
    int i = frameno & FRAME_INFO_MASK;
    return pv->frame_info[i].stop;
}

static void compute_dts_offset( hb_work_private_t * pv, hb_buffer_t * buf )
{
    if ( pv->job->areBframes )
    {
        if ( ( pv->frameno_in - 1 ) == pv->job->areBframes )
        {
            pv->dts_delay = buf->start;
            pv->job->config.h264.init_delay = pv->dts_delay;
        }
    }
}

// Generate DTS by rearranging PTS in this sequence:
// pts0 - delay, pts1 - delay, pts2 - delay, pts1, pts2, pts3...
//
// Where pts0 - ptsN are in decoded monotonically increasing presentation 
// order and delay == pts1 (1 being the number of frames the decoder must
// delay before it has suffecient information to decode). The number of
// frames to delay is set by job->areBframes, so it is configurable.
// This guarantees that DTS <= PTS for any frame.
//
// This is similar to how x264 generates DTS
static hb_buffer_t * process_delay_list( hb_work_private_t * pv, hb_buffer_t * buf )
{
    if ( pv->job->areBframes )
    {
        // Has dts_delay been set yet?
        if ( pv->frameno_in <= pv->job->areBframes )
        {
            // dts_delay not yet set.  queue up buffers till it is set.
            if ( pv->delay_tail == NULL )
            {
                pv->delay_head = pv->delay_tail = buf;
            }
            else
            {
                pv->delay_tail->next = buf;
                pv->delay_tail = buf;
            }
            return NULL;
        }

        // We have dts_delay.  Apply it to any queued buffers renderOffset
        // and return all queued buffers.
        if ( pv->delay_tail == NULL && buf != NULL )
        {
            pv->frameno_out++;
            // Use the cached frame info to get the start time of Nth frame
            // Note that start Nth frame != start time this buffer since the
            // output buffers have rearranged start times.
            int64_t start = get_frame_start( pv, pv->frameno_out );
            buf->renderOffset = start - pv->dts_delay;
            return buf;
        }
        else
        {
            pv->delay_tail->next = buf;
            buf = pv->delay_head;
            while ( buf )
            {
                pv->frameno_out++;
                // Use the cached frame info to get the start time of Nth frame
                // Note that start Nth frame != start time this buffer since the
                // output buffers have rearranged start times.
                int64_t start = get_frame_start( pv, pv->frameno_out );
                buf->renderOffset = start - pv->dts_delay;
                buf = buf->next;
            }
            buf = pv->delay_head;
            pv->delay_head = pv->delay_tail = NULL;
            return buf;
        }
    }
    else if ( buf )
    {
        buf->renderOffset = buf->start - pv->dts_delay;
        return buf;
    }
    return NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encavcodecWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                    hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_job_t * job = pv->job;
    AVFrame  * frame;
    hb_buffer_t * in = *buf_in, * buf;

    if ( in->size <= 0 )
    {
        /* EOF on input - send it downstream & say we're done */
        *buf_out = in;
        *buf_in = NULL;
       return HB_WORK_DONE;
    }

    frame              = avcodec_alloc_frame();
    frame->data[0]     = in->data;
    frame->data[1]     = frame->data[0] + job->width * job->height;
    frame->data[2]     = frame->data[1] + job->width * job->height / 4;
    frame->linesize[0] = job->width;
    frame->linesize[1] = job->width / 2;
    frame->linesize[2] = job->width / 2;
    // For constant quality, setting the quality in AVCodecContext 
    // doesn't do the trick.  It must be set in the AVFrame.
    frame->quality = pv->context->global_quality;

    // Bizarro ffmpeg appears to require the input AVFrame.pts to be
    // set to a frame number.  Setting it to an actual pts causes
    // jerky video.
    // frame->pts = in->start;
    frame->pts = ++pv->frameno_in;

    // Remember info about this frame that we need to pass across
    // the avcodec_encode_video call (since it reorders frames).
    save_frame_info( pv, in );
    compute_dts_offset( pv, in );

    if ( pv->context->codec )
    {
        /* Should be way too large */
        buf = hb_video_buffer_init( job->width, job->height );
        buf->size = avcodec_encode_video( pv->context, buf->data, buf->alloc,
                                          frame );
        if ( buf->size <= 0 )
        {
            hb_buffer_close( &buf );
        }
        else
        {
            int64_t frameno = pv->context->coded_frame->pts;
            buf->start  = get_frame_start( pv, frameno );
            buf->stop  = get_frame_stop( pv, frameno );
            buf->flags &= ~HB_FRAME_REF;
            switch ( pv->context->coded_frame->pict_type )
            {
                case FF_P_TYPE:
                {
                    buf->frametype = HB_FRAME_P;
                } break;

                case FF_B_TYPE:
                {
                    buf->frametype = HB_FRAME_B;
                } break;

                case FF_S_TYPE:
                {
                    buf->frametype = HB_FRAME_P;
                } break;

                case FF_SP_TYPE:
                {
                    buf->frametype = HB_FRAME_P;
                } break;

                case FF_BI_TYPE:
                case FF_SI_TYPE:
                case FF_I_TYPE:
                {
                    buf->flags |= HB_FRAME_REF;
                    if ( pv->context->coded_frame->key_frame )
                    {
                        buf->frametype = HB_FRAME_IDR;
                    }
                    else
                    {
                        buf->frametype = HB_FRAME_I;
                    }
                } break;

                default:
                {
                    if ( pv->context->coded_frame->key_frame )
                    {
                        buf->flags |= HB_FRAME_REF;
                        buf->frametype = HB_FRAME_KEY;
                    }
                    else
                    {
                        buf->frametype = HB_FRAME_REF;
                    }
                } break;
            }
            buf = process_delay_list( pv, buf );
        }

        if( job->pass == 1 )
        {
            /* Write stats */
            fprintf( pv->file, "%s", pv->context->stats_out );
        }
    }
    else
    {
        buf = NULL;
        
        hb_error( "encavcodec: codec context has uninitialized codec; skipping frame" );
    }

    av_free( frame );

    *buf_out = buf;

    return HB_WORK_OK;
}


