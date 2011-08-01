/* $Id: decavcodec.c,v 1.6 2005/03/06 04:08:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

/* This module is Handbrake's interface to the ffmpeg decoder library
   (libavcodec & small parts of libavformat). It contains four Handbrake
   "work objects":

    decavcodeca connects HB to an ffmpeg audio decoder
    decavcodecv connects HB to an ffmpeg video decoder

        (Two different routines are needed because the ffmpeg library
        has different decoder calling conventions for audio & video.
        These work objects are self-contained & follow all
        of HB's conventions for a decoder module. They can be used like
        any other HB decoder (deca52, decmpeg2, etc.).

    These decoders handle 2 kinds of input.  Streams that are demuxed
    by HandBrake and streams that are demuxed by libavformat.  In the
    case of streams that are demuxed by HandBrake, there is an extra
    parse step required that happens in decodeVideo and decavcodecaWork.
    In the case of streams that are demuxed by libavformat, there is context
    information that we need from the libavformat.  This information is
    propagated from hb_stream_open to these decoders through title->opaque_priv.

    A consequence of the above is that the streams that are demuxed by HandBrake
    *can't* use information from the AVStream because there isn't one - they
    get their data from either the dvd reader or the mpeg reader, not the ffmpeg
    stream reader. That means that they have to make up for deficiencies in the
    AVCodecContext info by using stuff kept in the HB "title" struct. It
    also means that ffmpeg codecs that randomly scatter state needed by
    the decoder across both the AVCodecContext & the AVStream (e.g., the
    VC1 decoder) can't easily be used by the HB mpeg stream reader.
 */

#include "hb.h"
#include "hbffmpeg.h"
#include "downmix.h"
#include "libavcodec/audioconvert.h"

static void compute_frame_duration( hb_work_private_t *pv );
static void flushDelayQueue( hb_work_private_t *pv );
static int  decavcodecaInit( hb_work_object_t *, hb_job_t * );
static int  decavcodecaWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
static void decavcodecClose( hb_work_object_t * );
static int decavcodecaInfo( hb_work_object_t *, hb_work_info_t * );
static int decavcodecaBSInfo( hb_work_object_t *, const hb_buffer_t *, hb_work_info_t * );

hb_work_object_t hb_decavcodeca =
{
    WORK_DECAVCODEC,
    "Audio decoder (libavcodec)",
    decavcodecaInit,
    decavcodecaWork,
    decavcodecClose,
    decavcodecaInfo,
    decavcodecaBSInfo
};

#define HEAP_SIZE 8
typedef struct {
    // there are nheap items on the heap indexed 1..nheap (i.e., top of
    // heap is 1). The 0th slot is unused - a marker is put there to check
    // for overwrite errs.
    int64_t h[HEAP_SIZE+1];
    int     nheap;
} pts_heap_t;

struct hb_work_private_s
{
    hb_job_t        *job;
    hb_title_t      *title;
    AVCodecContext  *context;
    AVCodecParserContext *parser;
    int             video_codec_opened;
    hb_list_t       *list;
    double          duration;   // frame duration (for video)
    double          field_duration;   // field duration (for video)
    int             frame_duration_set; // Indicates valid timing was found in stream
    double          pts_next;   // next pts we expect to generate
    int64_t         chap_time;  // time of next chap mark (if new_chap != 0)
    int             new_chap;   // output chapter mark pending
    uint32_t        nframes;
    uint32_t        ndrops;
    uint32_t        decode_errors;
    int             brokenByMicrosoft; // video stream may contain packed b-frames
    hb_buffer_t*    delayq[HEAP_SIZE];
    int             queue_primed;
    pts_heap_t      pts_heap;
    void*           buffer;
    struct SwsContext *sws_context; // if we have to rescale or convert color space
    hb_downmix_t    *downmix;
    int cadence[12];
};

static void decodeAudio( hb_audio_t * audio, hb_work_private_t *pv, uint8_t *data, int size, int64_t pts );
static hb_buffer_t *link_buf_list( hb_work_private_t *pv );


static int64_t heap_pop( pts_heap_t *heap )
{
    int64_t result;

    if ( heap->nheap <= 0 )
    {
        return -1;
    }

    // return the top of the heap then put the bottom element on top,
    // decrease the heap size by one & rebalence the heap.
    result = heap->h[1];

    int64_t v = heap->h[heap->nheap--];
    int parent = 1;
    int child = parent << 1;
    while ( child <= heap->nheap )
    {
        // find the smallest of the two children of parent
        if (child < heap->nheap && heap->h[child] > heap->h[child+1] )
            ++child;

        if (v <= heap->h[child])
            // new item is smaller than either child so it's the new parent.
            break;

        // smallest child is smaller than new item so move it up then
        // check its children.
        int64_t hp = heap->h[child];
        heap->h[parent] = hp;
        parent = child;
        child = parent << 1;
    }
    heap->h[parent] = v;
    return result;
}

static void heap_push( pts_heap_t *heap, int64_t v )
{
    if ( heap->nheap < HEAP_SIZE )
    {
        ++heap->nheap;
    }

    // stick the new value on the bottom of the heap then bubble it
    // up to its correct spot.
    int child = heap->nheap;
    while (child > 1) {
        int parent = child >> 1;
        if (heap->h[parent] <= v)
            break;
        // move parent down
        int64_t hp = heap->h[parent];
        heap->h[child] = hp;
        child = parent;
    }
    heap->h[child] = v;
}

/***********************************************************************
 * hb_work_decavcodec_init
 ***********************************************************************
 *
 **********************************************************************/
static int decavcodecaInit( hb_work_object_t * w, hb_job_t * job )
{
    AVCodec * codec;

    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    w->private_data = pv;

    pv->job   = job;
    if ( job )
        pv->title = job->title;
    else
        pv->title = w->title;
    pv->list  = hb_list_init();

    int codec_id = w->codec_param;
    /*XXX*/
    if ( codec_id == 0 )
        codec_id = CODEC_ID_MP2;

    codec = avcodec_find_decoder( codec_id );
    if ( pv->title->opaque_priv )
    {
        AVFormatContext *ic = (AVFormatContext*)pv->title->opaque_priv;
        pv->context = avcodec_alloc_context();
        avcodec_copy_context( pv->context, ic->streams[w->audio->id]->codec);
        hb_ff_set_sample_fmt( pv->context, codec );
    }
    else
    {
        pv->parser = av_parser_init( codec_id );

        pv->context = avcodec_alloc_context3(codec);
        hb_ff_set_sample_fmt( pv->context, codec );
    }
    if ( hb_avcodec_open( pv->context, codec, 0 ) )
    {
        hb_log( "decavcodecaInit: avcodec_open failed" );
        return 1;
    }

    if ( w->audio != NULL )
    {
        if ( hb_need_downmix( w->audio->config.in.channel_layout, 
                              w->audio->config.out.mixdown) )
        {
            pv->downmix = hb_downmix_init(w->audio->config.in.channel_layout, 
                                          w->audio->config.out.mixdown);
            hb_downmix_set_chan_map( pv->downmix, &hb_smpte_chan_map, &hb_smpte_chan_map );
        }
    }

    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
static void closePrivData( hb_work_private_t ** ppv )
{
    hb_work_private_t * pv = *ppv;

    if ( pv )
    {
        flushDelayQueue( pv );

        if ( pv->job && pv->context && pv->context->codec )
        {
            hb_log( "%s-decoder done: %u frames, %u decoder errors, %u drops",
                    pv->context->codec->name, pv->nframes, pv->decode_errors,
                    pv->ndrops );
        }
        if ( pv->sws_context )
        {
            sws_freeContext( pv->sws_context );
        }
        if ( pv->parser )
        {
            av_parser_close(pv->parser);
        }
        if ( pv->context && pv->context->codec )
        {
            hb_avcodec_close( pv->context );
        }
        if ( pv->context )
        {
            av_free( pv->context );
        }
        if ( pv->list )
        {
            hb_list_empty( &pv->list );
        }
        if ( pv->buffer )
        {
            av_free( pv->buffer );
            pv->buffer = NULL;
        }
        if ( pv->downmix )
        {
            hb_downmix_close( &(pv->downmix) );
        }
        free( pv );
    }
    *ppv = NULL;
}

static void decavcodecClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv )
    {
        closePrivData( &pv );
        w->private_data = NULL;
    }
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
static int decavcodecaWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                    hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * in = *buf_in;

    if ( in->size <= 0 )
    {
        /* EOF on input stream - send it downstream & say that we're done */
        *buf_out = in;
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    *buf_out = NULL;

    if ( in->start < -1 && pv->pts_next <= 0 )
    {
        // discard buffers that start before video time 0
        return HB_WORK_OK;
    }

    int pos, len;
    for ( pos = 0; pos < in->size; pos += len )
    {
        uint8_t *pout;
        int pout_len;
        int64_t cur;

        cur = in->start;

        if ( pv->parser != NULL )
        {
            len = av_parser_parse2( pv->parser, pv->context, &pout, &pout_len,
                                in->data + pos, in->size - pos, cur, cur, 0 );
            cur = pv->parser->pts;
            if ( cur == AV_NOPTS_VALUE )
                cur = -1;
        }
        else
        {
            pout = in->data;
            len = pout_len = in->size;
        }
        if (pout)
        {
            // set the duration on every frame since the stream format can
            // change (it shouldn't but there's no way to guarantee it).
            // duration is a scaling factor to go from #bytes in the decoded
            // frame to frame time (in 90KHz mpeg ticks). 'channels' converts
            // total samples to per-channel samples. 'sample_rate' converts
            // per-channel samples to seconds per sample and the 90000
            // is mpeg ticks per second.
            if ( pv->context->sample_rate && pv->context->channels )
            {
                pv->duration = 90000. /
                    (double)( pv->context->sample_rate * pv->context->channels );
            }
            decodeAudio( w->audio, pv, pout, pout_len, cur );
        }
    }
    *buf_out = link_buf_list( pv );
    return HB_WORK_OK;
}

static int decavcodecaInfo( hb_work_object_t *w, hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;

    memset( info, 0, sizeof(*info) );

    if ( pv && pv->context )
    {
        AVCodecContext *context = pv->context;
        info->bitrate = context->bit_rate;
        info->rate = context->time_base.num;
        info->rate_base = context->time_base.den;
        info->profile = context->profile;
        info->level = context->level;
        return 1;
    }
    return 0;
}

static int decavcodecaBSInfo( hb_work_object_t *w, const hb_buffer_t *buf,
                             hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;
    int ret = 0;

    memset( info, 0, sizeof(*info) );

    if ( pv && pv->context )
    {
        return decavcodecaInfo( w, info );
    }
    // XXX
    // We should parse the bitstream to find its parameters but for right
    // now we just return dummy values if there's a codec that will handle it.
    AVCodec *codec = avcodec_find_decoder( w->codec_param? w->codec_param :
                                                           CODEC_ID_MP2 );
    if ( ! codec )
    {
        // there's no ffmpeg codec for this audio type - give up
        return -1;
    }

    static char codec_name[64];
    info->name =  strncpy( codec_name, codec->name, sizeof(codec_name)-1 );

    AVCodecParserContext *parser = av_parser_init( codec->id );
    AVCodecContext *context = avcodec_alloc_context3(codec);
    hb_ff_set_sample_fmt( context, codec );
    if ( hb_avcodec_open( context, codec, 0 ) )
    {
        return -1;
    }
    uint8_t *buffer = av_malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE );
    int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    unsigned char *pbuffer;
    int pos, pbuffer_size;

    while ( buf && !ret )
    {
        pos = 0;
        while ( pos < buf->size )
        {
            int len;

            if ( parser != NULL )
            {
                len = av_parser_parse2( parser, context, &pbuffer, 
                                        &pbuffer_size, buf->data + pos, 
                                        buf->size - pos, buf->start, 
                                        buf->start, 0 );
            }
            else
            {
                pbuffer = buf->data;
                len = pbuffer_size = buf->size;
            }
            pos += len;
            if ( pbuffer_size > 0 )
            {
                AVPacket avp;
                av_init_packet( &avp );
                avp.data = pbuffer;
                avp.size = pbuffer_size;

                len = avcodec_decode_audio3( context, (int16_t*)buffer, 
                                             &out_size, &avp );
                if ( len > 0 && context->sample_rate > 0 )
                {
                    int isamp = av_get_bytes_per_sample( context->sample_fmt );
                    info->bitrate = context->bit_rate;
                    info->rate = context->sample_rate;
                    info->rate_base = 1;
                    info->channel_layout = 
                        hb_ff_layout_xlat(context->channel_layout, 
                                          context->channels);
                    ret = 1;
                    if ( context->channels && isamp )
                    {
                        info->samples_per_frame = out_size / 
                                                  (isamp * context->channels);
                    }
                    break;
                }
            }
        }
        buf = buf->next;
    }

    info->channel_map = &hb_smpte_chan_map;

    av_free( buffer );
    if ( parser != NULL )
        av_parser_close( parser );
    hb_avcodec_close( context );
    av_free( context );
    return ret;
}

/* -------------------------------------------------------------
 * General purpose video decoder using libavcodec
 */

static uint8_t *copy_plane( uint8_t *dst, uint8_t* src, int dstride, int sstride,
                            int h )
{
    if ( dstride == sstride )
    {
        memcpy( dst, src, dstride * h );
        return dst + dstride * h;
    }
    int lbytes = dstride <= sstride? dstride : sstride;
    while ( --h >= 0 )
    {
        memcpy( dst, src, lbytes );
        src += sstride;
        dst += dstride;
    }
    return dst;
}

// copy one video frame into an HB buf. If the frame isn't in our color space
// or at least one of its dimensions is odd, use sws_scale to convert/rescale it.
// Otherwise just copy the bits.
static hb_buffer_t *copy_frame( hb_work_private_t *pv, AVFrame *frame )
{
    AVCodecContext *context = pv->context;
    int w, h;
    if ( ! pv->job )
    {
        // if the dimensions are odd, drop the lsb since h264 requires that
        // both width and height be even.
        w = ( context->width >> 1 ) << 1;
        h = ( context->height >> 1 ) << 1;
    }
    else
    {
        w =  pv->job->title->width;
        h =  pv->job->title->height;
    }
    hb_buffer_t *buf = hb_video_buffer_init( w, h );
    uint8_t *dst = buf->data;

    if ( context->pix_fmt != PIX_FMT_YUV420P || w != context->width ||
         h != context->height )
    {
        // have to convert to our internal color space and/or rescale
        AVPicture dstpic;
        avpicture_fill( &dstpic, dst, PIX_FMT_YUV420P, w, h );

        if ( ! pv->sws_context )
        {
            pv->sws_context = hb_sws_get_context( context->width, context->height, context->pix_fmt,
                                              w, h, PIX_FMT_YUV420P,
                                              SWS_LANCZOS|SWS_ACCURATE_RND);
        }
        sws_scale( pv->sws_context, (const uint8_t* const *)frame->data, frame->linesize, 0, h,
                   dstpic.data, dstpic.linesize );
    }
    else
    {
        dst = copy_plane( dst, frame->data[0], w, frame->linesize[0], h );
        w = (w + 1) >> 1; h = (h + 1) >> 1;
        dst = copy_plane( dst, frame->data[1], w, frame->linesize[1], h );
        dst = copy_plane( dst, frame->data[2], w, frame->linesize[2], h );
    }
    return buf;
}

static int get_frame_buf( AVCodecContext *context, AVFrame *frame )
{
    return avcodec_default_get_buffer( context, frame );
}

static int reget_frame_buf( AVCodecContext *context, AVFrame *frame )
{
    return avcodec_default_reget_buffer( context, frame );
}

static void log_chapter( hb_work_private_t *pv, int chap_num, int64_t pts )
{
    hb_chapter_t *c;

    if ( !pv->job )
        return;

    c = hb_list_item( pv->job->title->list_chapter, chap_num - 1 );
    if ( c && c->title )
    {
        hb_log( "%s: \"%s\" (%d) at frame %u time %"PRId64,
                pv->context->codec->name, c->title, chap_num, pv->nframes, pts );
    }
    else
    {
        hb_log( "%s: Chapter %d at frame %u time %"PRId64,
                pv->context->codec->name, chap_num, pv->nframes, pts );
    }
}

static void flushDelayQueue( hb_work_private_t *pv )
{
    hb_buffer_t *buf;
    int slot = pv->queue_primed ? pv->nframes & (HEAP_SIZE-1) : 0;

    // flush all the video packets left on our timestamp-reordering delay q
    while ( ( buf = pv->delayq[slot] ) != NULL )
    {
        buf->start = heap_pop( &pv->pts_heap );
        hb_list_add( pv->list, buf );
        pv->delayq[slot] = NULL;
        slot = ( slot + 1 ) & (HEAP_SIZE-1);
    }
}

#define TOP_FIRST PIC_FLAG_TOP_FIELD_FIRST
#define PROGRESSIVE PIC_FLAG_PROGRESSIVE_FRAME
#define REPEAT_FIRST PIC_FLAG_REPEAT_FIRST_FIELD
#define TB 8
#define BT 16
#define BT_PROG 32
#define BTB_PROG 64
#define TB_PROG 128
#define TBT_PROG 256

static void checkCadence( int * cadence, uint16_t flags, int64_t start )
{
    /*  Rotate the cadence tracking. */
    int i = 0;
    for(i=11; i > 0; i--)
    {
        cadence[i] = cadence[i-1];
    }

    if ( !(flags & PROGRESSIVE) && !(flags & TOP_FIRST) )
    {
        /* Not progressive, not top first...
           That means it's probably bottom
           first, 2 fields displayed.
        */
        //hb_log("MPEG2 Flag: Bottom field first, 2 fields displayed.");
        cadence[0] = BT;
    }
    else if ( !(flags & PROGRESSIVE) && (flags & TOP_FIRST) )
    {
        /* Not progressive, top is first,
           Two fields displayed.
        */
        //hb_log("MPEG2 Flag: Top field first, 2 fields displayed.");
        cadence[0] = TB;
    }
    else if ( (flags & PROGRESSIVE) && !(flags & TOP_FIRST) && !( flags & REPEAT_FIRST )  )
    {
        /* Progressive, but noting else.
           That means Bottom first,
           2 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive. Bottom field first, 2 fields displayed.");
        cadence[0] = BT_PROG;
    }
    else if ( (flags & PROGRESSIVE) && !(flags & TOP_FIRST) && ( flags & REPEAT_FIRST )  )
    {
        /* Progressive, and repeat. .
           That means Bottom first,
           3 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive repeat. Bottom field first, 3 fields displayed.");
        cadence[0] = BTB_PROG;
    }
    else if ( (flags & PROGRESSIVE) && (flags & TOP_FIRST) && !( flags & REPEAT_FIRST )  )
    {
        /* Progressive, top first.
           That means top first,
           2 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive. Top field first, 2 fields displayed.");
        cadence[0] = TB_PROG;
    }
    else if ( (flags & PROGRESSIVE) && (flags & TOP_FIRST) && ( flags & REPEAT_FIRST )  )
    {
        /* Progressive, top, repeat.
           That means top first,
           3 fields displayed.
        */
        //hb_log("MPEG2 Flag: Progressive repeat. Top field first, 3 fields displayed.");
        cadence[0] = TBT_PROG;
    }

    if ( (cadence[2] <= TB) && (cadence[1] <= TB) && (cadence[0] > TB) && (cadence[11]) )
        hb_log("%fs: Video -> Film", (float)start / 90000);
    if ( (cadence[2] > TB) && (cadence[1] <= TB) && (cadence[0] <= TB) && (cadence[11]) )
        hb_log("%fs: Film -> Video", (float)start / 90000);
}

/*
 * Decodes a video frame from the specified raw packet data 
 *      ('data', 'size', 'sequence').
 * The output of this function is stored in 'pv->list', which contains a list
 * of zero or more decoded packets.
 * 
 * The returned packets are guaranteed to have their timestamps in the correct 
 * order, even if the original packets decoded by libavcodec have misordered 
 * timestamps, due to the use of 'packed B-frames'.
 * 
 * Internally the set of decoded packets may be buffered in 'pv->delayq'
 * until enough packets have been decoded so that the timestamps can be
 * correctly rewritten, if this is necessary.
 */
static int decodeFrame( hb_work_private_t *pv, uint8_t *data, int size, int sequence, int64_t pts, int64_t dts )
{
    int got_picture, oldlevel = 0;
    AVFrame frame;
    AVPacket avp;

    if ( global_verbosity_level <= 1 )
    {
        oldlevel = av_log_get_level();
        av_log_set_level( AV_LOG_QUIET );
    }

    av_init_packet( &avp );
    avp.data = data;
    avp.size = size;
    avp.pts = pts;
    avp.dts = dts;
    if ( avcodec_decode_video2( pv->context, &frame, &got_picture, &avp ) < 0 )
    {
        ++pv->decode_errors;     
    }
    if ( global_verbosity_level <= 1 )
    {
        av_log_set_level( oldlevel );
    }
    if( got_picture )
    {
        uint16_t flags = 0;

        // ffmpeg makes it hard to attach a pts to a frame. if the MPEG ES
        // packet had a pts we handed it to av_parser_parse (if the packet had
        // no pts we set it to AV_NOPTS_VALUE, but before the parse we can't 
        // distinguish between the start of a video frame with no pts & an 
        // intermediate packet of some frame which never has a pts). we hope 
        // that when parse returns the frame to us the pts we originally 
        // handed it will be in parser->pts. we put this pts into avp.pts so 
        // that when avcodec_decode_video finally gets around to allocating an 
        // AVFrame to hold the decoded frame, avcodec_default_get_buffer can 
        // stuff that pts into the it. if all of these relays worked at this 
        // point frame.pts should hold the frame's pts from the original data 
        // stream or AV_NOPTS_VALUE if it didn't have one. in the latter case
        // we generate the next pts in sequence for it.
        if ( !pv->frame_duration_set )
            compute_frame_duration( pv );

        double frame_dur = pv->duration;
        if ( frame.repeat_pict )
        {
            frame_dur += frame.repeat_pict * pv->field_duration;
        }

        // If there was no pts for this frame, assume constant frame rate
        // video & estimate the next frame time from the last & duration.
        double pts;
        if (frame.pkt_pts == AV_NOPTS_VALUE)
        {
            pts = pv->pts_next;
        }
        else
        {
            pts = frame.pkt_pts;
        }
        pv->pts_next = pts + frame_dur;

        if ( frame.top_field_first )
        {
            flags |= PIC_FLAG_TOP_FIELD_FIRST;
        }
        if ( !frame.interlaced_frame )
        {
            flags |= PIC_FLAG_PROGRESSIVE_FRAME;
        }
        if ( frame.repeat_pict )
        {
            flags |= PIC_FLAG_REPEAT_FIRST_FIELD;
        }

        hb_buffer_t *buf;

        // if we're doing a scan or this content couldn't have been broken
        // by Microsoft we don't worry about timestamp reordering
        if ( ! pv->job || ! pv->brokenByMicrosoft )
        {
            buf = copy_frame( pv, &frame );
            buf->start = pts;
            buf->sequence = sequence;
            buf->flags = flags;
            if ( pv->new_chap && buf->start >= pv->chap_time )
            {
                buf->new_chap = pv->new_chap;
                pv->new_chap = 0;
                pv->chap_time = 0;
                log_chapter( pv, buf->new_chap, buf->start );
            }
            else if ( pv->nframes == 0 && pv->job )
            {
                log_chapter( pv, pv->job->chapter_start, buf->start );
            }
            checkCadence( pv->cadence, buf->flags, buf->start );
            hb_list_add( pv->list, buf );
            ++pv->nframes;
            return got_picture;
        }

        // XXX This following probably addresses a libavcodec bug but I don't
        //     see an easy fix so we workaround it here.
        //
        // The M$ 'packed B-frames' atrocity results in decoded frames with
        // the wrong timestamp. E.g., if there are 2 b-frames the timestamps
        // we see here will be "2 3 1 5 6 4 ..." instead of "1 2 3 4 5 6".
        // The frames are actually delivered in the right order but with
        // the wrong timestamp. To get the correct timestamp attached to
        // each frame we have a delay queue (longer than the max number of
        // b-frames) & a sorting heap for the timestamps. As each frame
        // comes out of the decoder the oldest frame in the queue is removed
        // and associated with the smallest timestamp. Then the new frame is
        // added to the queue & its timestamp is pushed on the heap.
        // This does nothing if the timestamps are correct (i.e., the video
        // uses a codec that Micro$oft hasn't broken yet) but the frames
        // get timestamped correctly even when M$ has munged them.

        // remove the oldest picture from the frame queue (if any) &
        // give it the smallest timestamp from our heap. The queue size
        // is a power of two so we get the slot of the oldest by masking
        // the frame count & this will become the slot of the newest
        // once we've removed & processed the oldest.
        int slot = pv->nframes & (HEAP_SIZE-1);
        if ( ( buf = pv->delayq[slot] ) != NULL )
        {
            pv->queue_primed = 1;
            buf->start = heap_pop( &pv->pts_heap );

            if ( pv->new_chap && buf->start >= pv->chap_time )
            {
                buf->new_chap = pv->new_chap;
                pv->new_chap = 0;
                pv->chap_time = 0;
                log_chapter( pv, buf->new_chap, buf->start );
            }
            else if ( pv->nframes == 0 && pv->job )
            {
                log_chapter( pv, pv->job->chapter_start, buf->start );
            }
            checkCadence( pv->cadence, buf->flags, buf->start );
            hb_list_add( pv->list, buf );
        }

        // add the new frame to the delayq & push its timestamp on the heap
        buf = copy_frame( pv, &frame );
        buf->sequence = sequence;
        buf->flags = flags;
        pv->delayq[slot] = buf;
        heap_push( &pv->pts_heap, pts );

        ++pv->nframes;
    }

    return got_picture;
}
static void decodeVideo( hb_work_object_t *w, uint8_t *data, int size, int sequence, int64_t pts, int64_t dts )
{
    hb_work_private_t *pv = w->private_data;

    /*
     * The following loop is a do..while because we need to handle both
     * data & the flush at the end (signaled by size=0). At the end there's
     * generally a frame in the parser & one or more frames in the decoder
     * (depending on the bframes setting).
     */
    int pos = 0;
    do {
        uint8_t *pout;
        int pout_len, len;
        int64_t parser_pts, parser_dts;
        if ( pv->parser )
        {
            len = av_parser_parse2( pv->parser, pv->context, &pout, &pout_len,
                                    data + pos, size - pos, pts, dts, 0 );
            parser_pts = pv->parser->pts;
            parser_dts = pv->parser->dts;
        }
        else
        {
            pout = data;
            len = pout_len = size;
            parser_pts = pts;
            parser_dts = dts;
        }
        pos += len;

        if ( pout_len > 0 )
        {
            decodeFrame( pv, pout, pout_len, sequence, parser_pts, parser_dts );
        }
    } while ( pos < size );

    /* the stuff above flushed the parser, now flush the decoder */
    if ( size <= 0 )
    {
        while ( decodeFrame( pv, NULL, 0, sequence, AV_NOPTS_VALUE, AV_NOPTS_VALUE ) )
        {
        }
        flushDelayQueue( pv );
    }
}

/*
 * Removes all packets from 'pv->list', links them together into
 * a linked-list, and returns the first packet in the list.
 */
static hb_buffer_t *link_buf_list( hb_work_private_t *pv )
{
    hb_buffer_t *head = hb_list_item( pv->list, 0 );

    if ( head )
    {
        hb_list_rem( pv->list, head );

        hb_buffer_t *last = head, *buf;

        while ( ( buf = hb_list_item( pv->list, 0 ) ) != NULL )
        {
            hb_list_rem( pv->list, buf );
            last->next = buf;
            last = buf;
        }
    }
    return head;
}

static void init_video_avcodec_context( hb_work_private_t *pv )
{
    /* we have to wrap ffmpeg's get_buffer to be able to set the pts (?!) */
    pv->context->opaque = pv;
    pv->context->get_buffer = get_frame_buf;
    pv->context->reget_buffer = reget_frame_buf;
}

static int decavcodecvInit( hb_work_object_t * w, hb_job_t * job )
{

    hb_work_private_t *pv = calloc( 1, sizeof( hb_work_private_t ) );

    w->private_data = pv;
    pv->job   = job;
    if ( job )
        pv->title = job->title;
    else
        pv->title = w->title;
    pv->list = hb_list_init();

    if ( pv->title->opaque_priv )
    {
        AVFormatContext *ic = (AVFormatContext*)pv->title->opaque_priv;
        AVCodec *codec = avcodec_find_decoder( w->codec_param );
        if ( codec == NULL )
        {
            hb_log( "decavcodecvInit: failed to find codec for id (%d)", w->codec_param );
            return 1;
        }
        pv->context = avcodec_alloc_context();
        avcodec_copy_context( pv->context, ic->streams[pv->title->video_id]->codec);
        pv->context->workaround_bugs = FF_BUG_AUTODETECT;
        pv->context->error_recognition = 1;
        pv->context->error_concealment = FF_EC_GUESS_MVS|FF_EC_DEBLOCK;

        if ( hb_avcodec_open( pv->context, codec, pv->job ? HB_FFMPEG_THREADS_AUTO : 0 ) )
        {
            hb_log( "decavcodecvInit: avcodec_open failed" );
            return 1;
        }
        pv->video_codec_opened = 1;
        // avi, mkv and possibly mp4 containers can contain the M$ VFW packed
        // b-frames abortion that messes up frame ordering and timestamps.
        // XXX ffmpeg knows which streams are broken but doesn't expose the
        //     info externally. We should patch ffmpeg to add a flag to the
        //     codec context for this but until then we mark all ffmpeg streams
        //     as suspicious.
        pv->brokenByMicrosoft = 1;
    }
    else
    {
        pv->parser = av_parser_init( w->codec_param );
        pv->context = avcodec_alloc_context2( AVMEDIA_TYPE_VIDEO );
        pv->context->workaround_bugs = FF_BUG_AUTODETECT;
        pv->context->error_recognition = 1;
        pv->context->error_concealment = FF_EC_GUESS_MVS|FF_EC_DEBLOCK;
        init_video_avcodec_context( pv );
    }
    return 0;
}

static int next_hdr( hb_buffer_t *in, int offset )
{
    uint8_t *dat = in->data;
    uint16_t last2 = 0xffff;
    for ( ; in->size - offset > 1; ++offset )
    {
        if ( last2 == 0 && dat[offset] == 0x01 )
            // found an mpeg start code
            return offset - 2;

        last2 = ( last2 << 8 ) | dat[offset];
    }

    return -1;
}

static int find_hdr( hb_buffer_t *in, int offset, uint8_t hdr_type )
{
    if ( in->size - offset < 4 )
        // not enough room for an mpeg start code
        return -1;

    for ( ; ( offset = next_hdr( in, offset ) ) >= 0; ++offset )
    {
        if ( in->data[offset+3] == hdr_type )
            // found it
            break;
    }
    return offset;
}

static int setup_extradata( hb_work_object_t *w, hb_buffer_t *in )
{
    hb_work_private_t *pv = w->private_data;

    // we can't call the avstream funcs but the read_header func in the
    // AVInputFormat may set up some state in the AVContext. In particular 
    // vc1t_read_header allocates 'extradata' to deal with header issues
    // related to Microsoft's bizarre engineering notions. We alloc a chunk
    // of space to make vc1 work then associate the codec with the context.
    if ( w->codec_param != CODEC_ID_VC1 )
    {
        // we haven't been inflicted with M$ - allocate a little space as
        // a marker and return success.
        pv->context->extradata_size = 0;
        // av_malloc uses posix_memalign which is allowed to
        // return NULL when allocating 0 bytes.  We use extradata == NULL
        // to trigger initialization of extradata and the decoder, so 
        // we can not set it to NULL here. So allocate a small
        // buffer instead.
        pv->context->extradata = av_malloc(1);
        return 0;
    }

    // find the start and and of the sequence header
    int shdr, shdr_end;
    if ( ( shdr = find_hdr( in, 0, 0x0f ) ) < 0 )
    {
        // didn't find start of seq hdr
        return 1;
    }
    if ( ( shdr_end = next_hdr( in, shdr + 4 ) ) < 0 )
    {
        shdr_end = in->size;
    }
    shdr_end -= shdr;

    // find the start and and of the entry point header
    int ehdr, ehdr_end;
    if ( ( ehdr = find_hdr( in, 0, 0x0e ) ) < 0 )
    {
        // didn't find start of entry point hdr
        return 1;
    }
    if ( ( ehdr_end = next_hdr( in, ehdr + 4 ) ) < 0 )
    {
        ehdr_end = in->size;
    }
    ehdr_end -= ehdr;

    // found both headers - allocate an extradata big enough to hold both
    // then copy them into it.
    pv->context->extradata_size = shdr_end + ehdr_end;
    pv->context->extradata = av_malloc(pv->context->extradata_size + 8);
    memcpy( pv->context->extradata, in->data + shdr, shdr_end );
    memcpy( pv->context->extradata + shdr_end, in->data + ehdr, ehdr_end );
    memset( pv->context->extradata + shdr_end + ehdr_end, 0, 8);
    return 0;
}

static int decavcodecvWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                            hb_buffer_t ** buf_out )
{
    hb_work_private_t *pv = w->private_data;
    hb_buffer_t *in = *buf_in;
    int64_t pts = AV_NOPTS_VALUE;
    int64_t dts = pts;

    *buf_in = NULL;
    *buf_out = NULL;

    /* if we got an empty buffer signaling end-of-stream send it downstream */
    if ( in->size == 0 )
    {
        if ( pv->context->codec != NULL )
        {
            decodeVideo( w, in->data, in->size, in->sequence, pts, dts );
        }
        hb_list_add( pv->list, in );
        *buf_out = link_buf_list( pv );
        return HB_WORK_DONE;
    }

    // if this is the first frame open the codec (we have to wait for the
    // first frame because of M$ VC1 braindamage).
    if ( !pv->video_codec_opened )
    {
        AVCodec *codec = avcodec_find_decoder( w->codec_param );
        if ( codec == NULL )
        {
            hb_log( "decavcodecvWork: failed to find codec for id (%d)", w->codec_param );
            *buf_out = hb_buffer_init( 0 );;
            return HB_WORK_DONE;
        }
        avcodec_get_context_defaults3( pv->context, codec );
        init_video_avcodec_context( pv );
        if ( setup_extradata( w, in ) )
        {
            // we didn't find the headers needed to set up extradata.
            // the codec will abort if we open it so just free the buf
            // and hope we eventually get the info we need.
            hb_buffer_close( &in );
            return HB_WORK_OK;
        }
        // disable threaded decoding for scan, can cause crashes
        if ( hb_avcodec_open( pv->context, codec, pv->job ? HB_FFMPEG_THREADS_AUTO : 0 ) )
        {
            hb_log( "decavcodecvWork: avcodec_open failed" );
            *buf_out = hb_buffer_init( 0 );;
            return HB_WORK_DONE;
        }
        pv->video_codec_opened = 1;
    }

    if( in->start >= 0 )
    {
        pts = in->start;
        dts = in->renderOffset;
    }
    if ( in->new_chap )
    {
        pv->new_chap = in->new_chap;
        pv->chap_time = pts >= 0? pts : pv->pts_next;
    }
    decodeVideo( w, in->data, in->size, in->sequence, pts, dts );
    hb_buffer_close( &in );
    *buf_out = link_buf_list( pv );
    return HB_WORK_OK;
}

static void compute_frame_duration( hb_work_private_t *pv )
{
    double duration = 0.;
    int64_t max_fps = 64L;

    // context->time_base may be in fields, so set the max *fields* per second
    if ( pv->context->ticks_per_frame > 1 )
        max_fps *= pv->context->ticks_per_frame;

    if ( pv->title->opaque_priv )
    {
        // If ffmpeg is demuxing for us, it collects some additional
        // information about framerates that is often more accurate
        // than context->time_base.
        AVFormatContext *ic = (AVFormatContext*)pv->title->opaque_priv;
        AVStream *st = ic->streams[pv->title->video_id];
        if ( st->nb_frames && st->duration )
        {
            // compute the average frame duration from the total number
            // of frames & the total duration.
            duration = ( (double)st->duration * (double)st->time_base.num ) /
                       ( (double)st->nb_frames * (double)st->time_base.den );
        }
        else
        {
            // XXX We don't have a frame count or duration so try to use the
            // far less reliable time base info in the stream.
            // Because the time bases are so screwed up, we only take values
            // in the range 8fps - 64fps.
            AVRational *tb = NULL;
            if ( st->avg_frame_rate.den * 64L > st->avg_frame_rate.num &&
                 st->avg_frame_rate.num > st->avg_frame_rate.den * 8L )
            {
                tb = &(st->avg_frame_rate);
                duration =  (double)tb->den / (double)tb->num;
            }
            else if ( st->time_base.num * 64L > st->time_base.den &&
                      st->time_base.den > st->time_base.num * 8L )
            {
                tb = &(st->time_base);
                duration =  (double)tb->num / (double)tb->den;
            }
            else if ( st->r_frame_rate.den * 64L > st->r_frame_rate.num &&
                      st->r_frame_rate.num > st->r_frame_rate.den * 8L )
            {
                tb = &(st->r_frame_rate);
                duration =  (double)tb->den / (double)tb->num;
            }
        }
        if ( !duration &&
             pv->context->time_base.num * max_fps > pv->context->time_base.den &&
             pv->context->time_base.den > pv->context->time_base.num * 8L )
        {
            duration =  (double)pv->context->time_base.num / 
                        (double)pv->context->time_base.den;
            if ( pv->context->ticks_per_frame > 1 )
            {
                // for ffmpeg 0.5 & later, the H.264 & MPEG-2 time base is
                // field rate rather than frame rate so convert back to frames.
                duration *= pv->context->ticks_per_frame;
            }
        }
    }
    else
    {
        if ( pv->context->time_base.num * max_fps > pv->context->time_base.den &&
             pv->context->time_base.den > pv->context->time_base.num * 8L )
        {
            duration =  (double)pv->context->time_base.num / 
                            (double)pv->context->time_base.den;
            if ( pv->context->ticks_per_frame > 1 )
            {
                // for ffmpeg 0.5 & later, the H.264 & MPEG-2 time base is
                // field rate rather than frame rate so convert back to frames.
                duration *= pv->context->ticks_per_frame;
            }
        }
    }
    if ( duration == 0 )
    {
        // No valid timing info found in the stream, so pick some value
        duration = 1001. / 24000.;
    }
    else
    {
        pv->frame_duration_set = 1;
    }
    pv->duration = duration * 90000.;
    pv->field_duration = pv->duration;
    if ( pv->context->ticks_per_frame > 1 )
    {
        pv->field_duration /= pv->context->ticks_per_frame;
    }
}

static int decavcodecvInfo( hb_work_object_t *w, hb_work_info_t *info )
{
    hb_work_private_t *pv = w->private_data;

    memset( info, 0, sizeof(*info) );

    info->bitrate = pv->context->bit_rate;
    info->width = pv->context->width;
    info->height = pv->context->height;

    info->pixel_aspect_width = pv->context->sample_aspect_ratio.num;
    info->pixel_aspect_height = pv->context->sample_aspect_ratio.den;

    compute_frame_duration( pv );
    info->rate = 27000000;
    info->rate_base = pv->duration * 300.;

    info->profile = pv->context->profile;
    info->level = pv->context->level;
    info->name = pv->context->codec->name;

    return 1;
}

static int decavcodecvBSInfo( hb_work_object_t *w, const hb_buffer_t *buf,
                             hb_work_info_t *info )
{
    return 0;
}

hb_work_object_t hb_decavcodecv =
{
    WORK_DECAVCODECV,
    "Video decoder (libavcodec)",
    decavcodecvInit,
    decavcodecvWork,
    decavcodecClose,
    decavcodecvInfo,
    decavcodecvBSInfo
};

static hb_buffer_t * downmixAudio( 
    hb_audio_t *audio, 
    hb_work_private_t *pv, 
    hb_sample_t *buffer, 
    int channels,
    int nsamples )
{
    hb_buffer_t * buf = NULL;

    if ( pv->downmix )
    {
        int n_ch_samples = nsamples / channels;
        int out_channels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT(audio->config.out.mixdown);

        buf = hb_buffer_init( n_ch_samples * out_channels * sizeof(float) );
        hb_sample_t *samples = (hb_sample_t *)buf->data;
        hb_downmix(pv->downmix, samples, buffer, n_ch_samples);
    }
    else
    {
        buf = hb_buffer_init( nsamples * sizeof(float) );
        memcpy( buf->data, buffer, nsamples * sizeof(float) );
    }

    return buf;
}

static void decodeAudio( hb_audio_t * audio, hb_work_private_t *pv, uint8_t *data, int size, int64_t pts )
{
    AVCodecContext *context = pv->context;
    int pos = 0;
    int loop_limit = 256;

    if ( pts != -1 )
        pv->pts_next = pts;
    while ( pos < size )
    {
        float *buffer = pv->buffer;
        if ( buffer == NULL )
        {
            pv->buffer = av_malloc( AVCODEC_MAX_AUDIO_FRAME_SIZE );
            buffer = pv->buffer;
        }

        AVPacket avp;
        av_init_packet( &avp );
        avp.data = data + pos;
        avp.size = size - pos;
        avp.pts = pv->pts_next;
        avp.dts = AV_NOPTS_VALUE;

        int out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
        int nsamples;
        int len = avcodec_decode_audio3( context, (int16_t*)buffer, &out_size, &avp );
        if ( len < 0 )
        {
            return;
        }
        if ( len == 0 )
        {
            if ( !(loop_limit--) )
                return;
        }
        else
            loop_limit = 256;

        pos += len;
        if( out_size > 0 )
        {
            int isamp = av_get_bytes_per_sample( context->sample_fmt );
            nsamples = out_size / isamp;
            double pts_next = pv->pts_next + nsamples * pv->duration;

            // DTS-HD can be passed through to mkv
            if( audio->config.out.codec & HB_ACODEC_PASS_FLAG )
            {
                // Note that even though we are doing passthru, we had
                // to decode so that we know the stop time and the
                // pts of the next audio packet.
                hb_buffer_t * buf;

                buf = hb_buffer_init( avp.size );
                memcpy( buf->data, avp.data, avp.size );
                buf->start = pv->pts_next;
                buf->stop  = pts_next;
                hb_list_add( pv->list, buf );
                pv->pts_next = pts_next;
                continue;
            }

            // We require floats for the output format. If
            // we got something different convert it.
            if ( context->sample_fmt != AV_SAMPLE_FMT_FLT )
            {
                // Note: av_audio_convert seems to be a work-in-progress but
                //       looks like it will eventually handle general audio
                //       mixdowns which would allow us much more flexibility
                //       in handling multichannel audio in HB. If we were doing
                //       anything more complicated than a one-for-one format
                //       conversion we'd probably want to cache the converter
                //       context in the pv.
                AVAudioConvert *ctx;

                ctx = av_audio_convert_alloc( AV_SAMPLE_FMT_FLT, 1,
                                              context->sample_fmt, 1,
                                              NULL, 0 );

                // get output buffer size then malloc a buffer
                buffer = av_malloc( nsamples * sizeof(hb_sample_t) );

                // we're doing straight sample format conversion which 
                // behaves as if there were only one channel.
                const void * const ibuf[6] = { pv->buffer };
                void * const obuf[6] = { buffer };
                const int istride[6] = { isamp };
                const int ostride[6] = { sizeof(hb_sample_t) };

                av_audio_convert( ctx, obuf, ostride, ibuf, istride, nsamples );
                av_audio_convert_free( ctx );
            }

            hb_buffer_t * buf;
            buf = downmixAudio( audio, pv, buffer, context->channels, nsamples );
            buf->start = pv->pts_next;
            buf->stop = pts_next;
            hb_list_add( pv->list, buf );

            pv->pts_next = pts_next;

            // if we allocated a buffer for sample format conversion, free it
            if ( buffer != pv->buffer )
            {
                av_free( buffer );
            }
        }
    }
}
