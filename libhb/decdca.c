/* $Id: decdca.c,v 1.14 2005/03/03 17:21:57 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "downmix.h"

#include "dca.h"

struct hb_work_private_s
{
    hb_job_t    * job;

    /* libdca handle */
    dca_state_t * state;

    double        next_pts;
    int64_t       last_buf_pts;
    int           flags_in;
    int           flags_out;
    int           rate;
    int           bitrate;
    int           frame_length;
    float         level;

    int           error;
    int           sync;
    int           size;

    /* max frame size of the 16 bits version is 16384 */
    /* max frame size of the 14 bits version is 18726 */
    uint8_t       frame[18726];

    hb_list_t   * list;

    int           out_discrete_channels;

};

static int  decdcaInit( hb_work_object_t *, hb_job_t * );
static int  decdcaWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
static void decdcaClose( hb_work_object_t * );
static int  decdcaBSInfo( hb_work_object_t *, const hb_buffer_t *,
                          hb_work_info_t * );

hb_work_object_t hb_decdca =
{
    WORK_DECDCA,
    "DCA decoder",
    decdcaInit,
    decdcaWork,
    decdcaClose,
    0,
    decdcaBSInfo
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static hb_buffer_t * Decode( hb_work_object_t * w );

/***********************************************************************
 * hb_work_decdca_init
 ***********************************************************************
 * Allocate the work object, initialize libdca
 **********************************************************************/
static int decdcaInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    hb_audio_t * audio = w->audio;
    w->private_data = pv;

    pv->job   = job;

    pv->list      = hb_list_init();
    pv->state     = dca_init( 0 );

    /* Decide what format we want out of libdca
    work.c has already done some of this deduction for us in do_job() */

    pv->flags_out = HB_AMIXDOWN_GET_DCA_FORMAT(audio->config.out.mixdown);

    /* pass the number of channels used into the private work data */
    /* will only be actually used if we're not doing AC3 passthru */
    pv->out_discrete_channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown);

    pv->level     = 1.0;

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 * Free memory
 **********************************************************************/
static void decdcaClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    dca_free( pv->state );
    hb_list_empty( &pv->list );
    free( pv );
    w->private_data = NULL;
}

/***********************************************************************
 * Work
 ***********************************************************************
 * Add the given buffer to the data we already have, and decode as much
 * as we can
 **********************************************************************/
static int decdcaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

    if ( (*buf_in)->size <= 0 )
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = *buf_in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    if ( (*buf_in)->start < -1 && pv->next_pts == 0 )
    {
        // discard buffers that start before video time 0
        *buf_out = NULL;
        return HB_WORK_OK;
    }

    hb_list_add( pv->list, *buf_in );
    *buf_in = NULL;

    /* If we got more than a frame, chain raw buffers */
    *buf_out = buf = Decode( w );
    while( buf )
    {
        buf->next = Decode( w );
        buf       = buf->next;
    }

    return HB_WORK_OK;
}

/***********************************************************************
 * Decode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Decode( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;
    hb_audio_t  * audio = w->audio;
    int           i, j, k;
    int64_t       pts, pos;
    uint64_t      upts, upos;
    int           num_blocks;

    /* Get a frame header if don't have one yet */
    if( !pv->sync )
    {
        while( hb_list_bytes( pv->list ) >= 14 )
        {
            /* We have 14 bytes, check if this is a correct header */
            hb_list_seebytes( pv->list, pv->frame, 14 );
            pv->size = dca_syncinfo( pv->state, pv->frame, &pv->flags_in, &pv->rate,
                                    &pv->bitrate, &pv->frame_length );
            if( pv->size )
            {
                /* It is. W00t. */
                if( pv->error )
                {
                    hb_log( "dca_syncinfo ok" );
                }
                pv->error = 0;
                pv->sync  = 1;
                break;
            }

            /* It is not */
            if( !pv->error )
            {
                hb_log( "dca_syncinfo failed" );
                pv->error = 1;
            }

            /* Try one byte later */
            hb_list_getbytes( pv->list, pv->frame, 1, NULL, NULL );
        }
    }

    if( !pv->sync || hb_list_bytes( pv->list ) < pv->size )
    {
        /* Need more data */
        return NULL;
    }

    /* Get the whole frame */
    hb_list_getbytes( pv->list, pv->frame, pv->size, &upts, &upos );
    pts = (int64_t)upts;
    pos = (int64_t)upos;

    if ( pts != pv->last_buf_pts )
    {
        pv->last_buf_pts = pts;
    }
    else
    {
        // spec says that the PTS is the start time of the first frame
        // that starts in the PES frame so we only use the PTS once then
        // get the following frames' PTS from the frame length.
        pts = -1;
    }

    // mkv files typically use a 1ms timebase which results in a lot of
    // truncation error in their timestamps. Also, TSMuxer or something
    // in the m2ts-to-mkv toolchain seems to take a very casual attitude
    // about time - timestamps seem to randomly offset by ~40ms for a few
    // seconds then recover. So, if the pts we got is within 50ms of the
    // pts computed from the data stream, use the data stream pts.
    if ( pts == -1 || ( pv->next_pts && fabs( pts - pv->next_pts ) < 50.*90. ) )
    {
        pts = pv->next_pts;
    }

    double frame_dur = (double)(pv->frame_length & ~0xFF) / (double)pv->rate * 90000.;

    /* DCA passthrough: don't decode the DCA frame */
    if( audio->config.out.codec == HB_ACODEC_DCA_PASS )
    {
        buf = hb_buffer_init( pv->size );
        memcpy( buf->data, pv->frame, pv->size );
        buf->start = pts;
        pv->next_pts = pts + frame_dur;
        buf->stop  = pv->next_pts;
        pv->sync = 0;
        return buf;
    }

    /* Feed libdca */
    dca_frame( pv->state, pv->frame, &pv->flags_out, &pv->level, 0 );

    /* find out how many blocks are in this frame */
    num_blocks = dca_blocks_num( pv->state );

    /* num_blocks blocks per frame, 256 samples per block, channelsused channels */
    int nsamp = num_blocks * 256;
    buf = hb_buffer_init( nsamp * pv->out_discrete_channels * sizeof( float ) );

    buf->start = pts;
    pv->next_pts = pts + (double)nsamp / (double)pv->rate * 90000.;
    buf->stop  = pv->next_pts;

    for( i = 0; i < num_blocks; i++ )
    {
        dca_sample_t * samples_in;
        float    * samples_out;

        dca_block( pv->state );
        samples_in  = dca_samples( pv->state );
        samples_out = ((float *) buf->data) + 256 * pv->out_discrete_channels * i;

        /* Interleave */
        for( j = 0; j < 256; j++ )
        {
            for ( k = 0; k < pv->out_discrete_channels; k++ )
            {
                samples_out[(pv->out_discrete_channels*j)+k]   = samples_in[(256*k)+j];
            }
        }

    }

    pv->sync = 0;
    return buf;
}


static int decdcaBSInfo( hb_work_object_t *w, const hb_buffer_t *b,
                         hb_work_info_t *info )
{
    int i, flags, rate, bitrate, frame_length;
    dca_state_t * state = dca_init( 0 );

    memset( info, 0, sizeof(*info) );

    /* since DCA frames don't line up with MPEG ES frames scan the
     * entire frame for an DCA sync pattern.  */
    for ( i = 0; i < b->size - 7; ++i )
    {
        if( dca_syncinfo( state, &b->data[i], &flags, &rate, &bitrate,
                          &frame_length ) )
        {
            break;
        }
    }
    if ( i >= b->size - 7 )
    {
        /* didn't find DCA sync */
        dca_free( state );
        return 0;
    }

    info->name = "DCA";
    info->rate = rate;
    info->rate_base = 1;
    info->bitrate = bitrate;
    info->flags = flags;
    info->samples_per_frame = frame_length;

    if ( ( flags & DCA_CHANNEL_MASK) == DCA_DOLBY )
    {
        info->flags |= AUDIO_F_DOLBY;
    }

    switch( flags & DCA_CHANNEL_MASK )
    {
        /* mono sources */
        case DCA_MONO:
            info->channel_layout = HB_INPUT_CH_LAYOUT_MONO;
            break;
        /* stereo input */
        case DCA_CHANNEL:
        case DCA_STEREO:
        case DCA_STEREO_SUMDIFF:
        case DCA_STEREO_TOTAL:
            info->channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
            break;
        /* 3F/2R input */
        case DCA_3F2R:
            info->channel_layout = HB_INPUT_CH_LAYOUT_3F2R;
            break;
        /* 3F/1R input */
        case DCA_3F1R:
            info->channel_layout = HB_INPUT_CH_LAYOUT_3F1R;
            break;
        /* other inputs */
        case DCA_3F:
            info->channel_layout = HB_INPUT_CH_LAYOUT_3F;
            break;
        case DCA_2F1R:
            info->channel_layout = HB_INPUT_CH_LAYOUT_2F1R;
            break;
        case DCA_2F2R:
            info->channel_layout = HB_INPUT_CH_LAYOUT_2F2R;
            break;
        case DCA_4F2R:
            info->channel_layout = HB_INPUT_CH_LAYOUT_4F2R;
            break;
        /* unknown */
        default:
            info->channel_layout = HB_INPUT_CH_LAYOUT_STEREO;
    }

    if (flags & DCA_LFE)
    {
        info->channel_layout |= HB_INPUT_CH_LAYOUT_HAS_LFE;
    }

    info->channel_map = &hb_qt_chan_map;

    dca_free( state );
    return 1;
}
