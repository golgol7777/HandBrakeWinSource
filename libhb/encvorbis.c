/* $Id: encvorbis.c,v 1.6 2005/03/05 15:08:32 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"

#include "vorbis/vorbisenc.h"

#define OGGVORBIS_FRAME_SIZE 1024

int  encvorbisInit( hb_work_object_t *, hb_job_t * );
int  encvorbisWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void encvorbisClose( hb_work_object_t * );

hb_work_object_t hb_encvorbis =
{
    WORK_ENCVORBIS,
    "Vorbis encoder (libvorbis)",
    encvorbisInit,
    encvorbisWork,
    encvorbisClose
};

struct hb_work_private_s
{
    hb_job_t   * job;

    vorbis_info        vi;
    vorbis_comment     vc;
    vorbis_dsp_state   vd;
    vorbis_block       vb;

    unsigned long   input_samples;
    uint8_t       * buf;
    uint64_t        pts;

    hb_list_t     * list;
    int           out_discrete_channels;
    int           channel_map[6];
    int64_t       prev_blocksize;
};

int encvorbisInit( hb_work_object_t * w, hb_job_t * job )
{
    hb_audio_t * audio = w->audio;
    int i;
    ogg_packet header[3];

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;
    pv->out_discrete_channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown);

    pv->job   = job;

    hb_log( "encvorbis: opening libvorbis" );

    /* 28kbps/channel seems to be the minimum for 6ch vorbis. */
    int min_bitrate = 28 * pv->out_discrete_channels;
    if (pv->out_discrete_channels > 2 && audio->config.out.bitrate < min_bitrate)
    {
        hb_log( "encvorbis: Selected bitrate (%d kbps) too low for %d channel audio.", audio->config.out.bitrate, pv->out_discrete_channels);
        hb_log( "encvorbis: Resetting bitrate to %d kbps", min_bitrate);
        /* Naughty! We shouldn't modify the audio from here. */
        audio->config.out.bitrate = min_bitrate;
    }

    /* init */
    for( i = 0; i < 3; i++ )
    {
        // Zero vorbis headers so that we don't crash in mk_laceXiph
        // when vorbis_encode_setup_managed fails.
        memset( w->config->vorbis.headers[i], 0, sizeof( ogg_packet ) );
    }
    vorbis_info_init( &pv->vi );
    if( vorbis_encode_setup_managed( &pv->vi, pv->out_discrete_channels,
          audio->config.out.samplerate, -1, 1000 * audio->config.out.bitrate, -1 ) )
    {
        hb_error( "encvorbis: vorbis_encode_setup_managed failed.\n" );
        *job->die = 1;
        return 0;
    }

    if( vorbis_encode_ctl( &pv->vi, OV_ECTL_RATEMANAGE2_SET, NULL ) ||
          vorbis_encode_setup_init( &pv->vi ) )
    {
        hb_error( "encvorbis: vorbis_encode_ctl( ratemanage2_set ) OR vorbis_encode_setup_init failed.\n" );
        *job->die = 1;
        return 0;
    }

    /* add a comment */
    vorbis_comment_init( &pv->vc );
    vorbis_comment_add_tag( &pv->vc, "Encoder", "HandBrake");
    vorbis_comment_add_tag( &pv->vc, "LANGUAGE", w->config->vorbis.language);

    /* set up the analysis state and auxiliary encoding storage */
    vorbis_analysis_init( &pv->vd, &pv->vi);
    vorbis_block_init( &pv->vd, &pv->vb);

    /* get the 3 headers */
    vorbis_analysis_headerout( &pv->vd, &pv->vc,
                               &header[0], &header[1], &header[2] );
    for( i = 0; i < 3; i++ )
    {
        memcpy( w->config->vorbis.headers[i], &header[i],
                sizeof( ogg_packet ) );
        memcpy( w->config->vorbis.headers[i] + sizeof( ogg_packet ),
                header[i].packet, header[i].bytes );
    }

    pv->input_samples = pv->out_discrete_channels * OGGVORBIS_FRAME_SIZE;
    pv->buf = malloc( pv->input_samples * sizeof( float ) );

    pv->list = hb_list_init();

    switch (pv->out_discrete_channels) {
        case 1:
            pv->channel_map[0] = 0;
            break;
        case 6:
            // Vorbis use the following channels map = L C R Ls Rs Lfe
            if( audio->config.in.codec == HB_ACODEC_AC3 )
            {
                pv->channel_map[0] = 1;
                pv->channel_map[1] = 2;
                pv->channel_map[2] = 3;
                pv->channel_map[3] = 4;
                pv->channel_map[4] = 5;
                pv->channel_map[5] = 0;
            }
            else
            {
                pv->channel_map[0] = 1;
                pv->channel_map[1] = 0;
                pv->channel_map[2] = 2;
                pv->channel_map[3] = 3;
                pv->channel_map[4] = 4;
                pv->channel_map[5] = 5;
            }
            break;
        default:
            hb_log("encvorbis.c: Unable to correctly proccess %d channels, assuming stereo.", pv->out_discrete_channels);
        case 2:
            // Assume stereo
            pv->channel_map[0] = 0;
            pv->channel_map[1] = 1;
            break;
    }

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encvorbisClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    vorbis_block_clear( &pv->vb );
    vorbis_dsp_clear( &pv->vd );
    vorbis_comment_clear( &pv->vc );
    vorbis_info_clear( &pv->vi );

    if (pv->list)
        hb_list_empty( &pv->list );

    free( pv->buf );
    free( pv );
    w->private_data = NULL;
}

/***********************************************************************
 * Flush
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Flush( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;
    int64_t     blocksize = 0;

    if( vorbis_analysis_blockout( &pv->vd, &pv->vb ) == 1 )
    {
        ogg_packet op;

        vorbis_analysis( &pv->vb, NULL );
        vorbis_bitrate_addblock( &pv->vb );

        if( vorbis_bitrate_flushpacket( &pv->vd, &op ) )
        {
            buf = hb_buffer_init( sizeof( ogg_packet ) + op.bytes );
            memcpy( buf->data, &op, sizeof( ogg_packet ) );
            memcpy( buf->data + sizeof( ogg_packet ), op.packet,
                    op.bytes );
            blocksize = vorbis_packet_blocksize(&pv->vi, &op);
            buf->frametype   = HB_FRAME_AUDIO;
            buf->start = (int64_t)(vorbis_granule_time(&pv->vd, op.granulepos) * 90000);
            buf->stop  = (int64_t)(vorbis_granule_time(&pv->vd, (pv->prev_blocksize + blocksize)/4 + op.granulepos) * 90000);
            /* The stop time isn't accurate for the first ~3 packets, as the actual blocksize depends on the previous _and_ current packets. */
            pv->prev_blocksize = blocksize;
            return buf;
        }
    }

    return NULL;
}

/***********************************************************************
 * Encode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Encode( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;
    float ** buffer;
    int i, j;

    /* Try to extract more data */
    if( ( buf = Flush( w ) ) )
    {
        return buf;
    }

    if( hb_list_bytes( pv->list ) < pv->input_samples * sizeof( float ) )
    {
        return NULL;
    }

    /* Process more samples */
    hb_list_getbytes( pv->list, pv->buf, pv->input_samples * sizeof( float ),
                      &pv->pts, NULL );
    buffer = vorbis_analysis_buffer( &pv->vd, OGGVORBIS_FRAME_SIZE );
    for( i = 0; i < OGGVORBIS_FRAME_SIZE; i++ )
    {
        for( j = 0; j < pv->out_discrete_channels; j++)
        {
            buffer[j][i] = ((float *) pv->buf)[(pv->out_discrete_channels * i + pv->channel_map[j])];
        }
    }

    vorbis_analysis_wrote( &pv->vd, OGGVORBIS_FRAME_SIZE );

    /* Try to extract again */
    return Flush( w );
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encvorbisWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                   hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

    if ( (*buf_in)->size <= 0 )
    {
        /* EOF on input - send it downstream & say we're done */
        *buf_out = *buf_in;
        *buf_in = NULL;
       return HB_WORK_DONE;
    }

    hb_list_add( pv->list, *buf_in );
    *buf_in = NULL;

    *buf_out = buf = Encode( w );

    while( buf )
    {
        buf->next = Encode( w );
        buf       = buf->next;
    }

    return HB_WORK_OK;
}
