/* $Id: dvd.c,v 1.12 2005/11/25 15:05:25 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "lang.h"
#include "hbffmpeg.h"

#include "libbluray/bluray.h"

struct hb_bd_s
{
    char         * path;
    BLURAY       * bd;
    int            title_count;
    uint64_t       pkt_count;
    hb_stream_t  * stream;
    int            chapter;
    int            next_chap;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static int           next_packet( BLURAY *bd, uint8_t *pkt );

/***********************************************************************
 * hb_bd_init
 ***********************************************************************
 *
 **********************************************************************/
hb_bd_t * hb_bd_init( char * path )
{
    hb_bd_t * d;

    d = calloc( sizeof( hb_bd_t ), 1 );

    /* Open device */
    d->bd = bd_open( path, NULL );
    if( d->bd == NULL )
    {
        /*
         * Not an error, may be a stream - which we'll try in a moment.
         */
        hb_log( "bd: not a bd - trying as a stream/file instead" );
        goto fail;
    }

    d->title_count = bd_get_titles( d->bd, TITLES_RELEVANT );
    if ( d->title_count == 0 )
    {
        hb_log( "bd: not a bd - trying as a stream/file instead" );
        goto fail;
    }
    d->path = strdup( path );

    return d;

fail:
    if( d->bd ) bd_close( d->bd );
    free( d );
    return NULL;
}

/***********************************************************************
 * hb_bd_title_count
 **********************************************************************/
int hb_bd_title_count( hb_bd_t * d )
{
    return d->title_count;
}

static void add_audio(int track, hb_list_t *list_audio, BLURAY_STREAM_INFO *bdaudio, int substream_type, uint32_t codec, uint32_t codec_param)
{
    hb_audio_t * audio;
    iso639_lang_t * lang;

    audio = calloc( sizeof( hb_audio_t ), 1 );

    audio->id = (substream_type << 16) | bdaudio->pid;
    audio->config.in.stream_type = bdaudio->coding_type;
    audio->config.in.substream_type = substream_type;
    audio->config.in.codec = codec;
    audio->config.in.codec_param = codec_param;
    audio->config.lang.type = 0;

    lang = lang_for_code2( (char*)bdaudio->lang );

    int stream_type = bdaudio->coding_type;
    snprintf( audio->config.lang.description, 
        sizeof( audio->config.lang.description ), "%s (%s)",
        strlen(lang->native_name) ? lang->native_name : lang->eng_name,
        audio->config.in.codec == HB_ACODEC_AC3 ? "AC3" : 
        ( audio->config.in.codec == HB_ACODEC_DCA ? "DTS" : 
        ( audio->config.in.codec == HB_ACODEC_MPGA ? 
            ( stream_type == BLURAY_STREAM_TYPE_AUDIO_LPCM ? "BD LPCM" : 
            ( stream_type == BLURAY_STREAM_TYPE_AUDIO_AC3PLUS ? "E-AC3" : 
            ( stream_type == BLURAY_STREAM_TYPE_AUDIO_TRUHD ? "TrueHD" : 
            ( stream_type == BLURAY_STREAM_TYPE_AUDIO_DTSHD ? "DTS-HD HRA" : 
            ( stream_type == BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER ? "DTS-HD MA" : 
            ( stream_type == BLURAY_STREAM_TYPE_AUDIO_MPEG1 ? "MPEG1" : 
            ( stream_type == BLURAY_STREAM_TYPE_AUDIO_MPEG2 ? "MPEG2" : 
                                                           "Unknown FFmpeg" 
            ) ) ) ) ) ) ) : "Unknown" 
        ) ) );

    snprintf( audio->config.lang.simple, 
              sizeof( audio->config.lang.simple ), "%s",
              strlen(lang->native_name) ? lang->native_name : 
                                          lang->eng_name );

    snprintf( audio->config.lang.iso639_2, 
              sizeof( audio->config.lang.iso639_2 ), "%s", lang->iso639_2);

    hb_log( "bd: audio id=0x%x, lang=%s, 3cc=%s", audio->id,
            audio->config.lang.description, audio->config.lang.iso639_2 );

    audio->config.in.track = track;
    hb_list_add( list_audio, audio );
    return;
}

static int bd_audio_equal( BLURAY_CLIP_INFO *a, BLURAY_CLIP_INFO *b )
{
    int ii, jj, equal;

    if ( a->audio_stream_count != b->audio_stream_count )
        return 0;

    for ( ii = 0; ii < a->audio_stream_count; ii++ )
    {
        BLURAY_STREAM_INFO * s = &a->audio_streams[ii];
        equal = 0;
        for ( jj = 0; jj < b->audio_stream_count; jj++ )
        {
            if ( s->pid == b->audio_streams[jj].pid &&
                 s->coding_type == b->audio_streams[jj].coding_type)
            {
                equal = 1;
                break;
            }
        }
        if ( !equal )
            return 0;
    }
    return 1;
}

/***********************************************************************
 * hb_bd_title_scan
 **********************************************************************/
hb_title_t * hb_bd_title_scan( hb_bd_t * d, int tt, uint64_t min_duration )
{

    hb_title_t   * title;
    hb_chapter_t * chapter;
    int            ii, jj;
    BLURAY_TITLE_INFO * ti = NULL;

    hb_log( "bd: scanning title %d", tt );

    title = hb_title_init( d->path, tt );
    title->demuxer = HB_MPEG2_TS_DEMUXER;
    title->type = HB_BD_TYPE;
    title->reg_desc = STR4_TO_UINT32("HDMV");

    char * p_cur, * p_last = d->path;
    for( p_cur = d->path; *p_cur; p_cur++ )
    {
        if( p_cur[0] == '/' && p_cur[1] )
        {
            p_last = &p_cur[1];
        }
    }
    snprintf( title->name, sizeof( title->name ), "%s", p_last );
    strncpy( title->path, d->path, 1024 );
    title->path[1023] = 0;

    title->vts = 0;
    title->ttn = 0;

    ti = bd_get_title_info( d->bd, tt - 1 );
    if ( ti == NULL )
    {
        hb_log( "bd: invalid title" );
        goto fail;
    }
    if ( ti->clip_count == 0 )
    {
        hb_log( "bd: stream has no clips" );
        goto fail;
    }
    if ( ti->clips[0].video_stream_count == 0 )
    {
        hb_log( "bd: stream has no video" );
        goto fail;
    }

    hb_deep_log( 2, "bd: Playlist %05d", ti->playlist);

    uint64_t pkt_count = 0;
    for ( ii = 0; ii < ti->clip_count; ii++ )
    {
        pkt_count += ti->clips[ii].pkt_count;
    }
    title->block_start = 0;
    title->block_end = pkt_count;
    title->block_count = pkt_count;

    title->angle_count = ti->angle_count;

    /* Get duration */
    title->duration = ti->duration;
    title->hours    = title->duration / 90000 / 3600;
    title->minutes  = ( ( title->duration / 90000 ) % 3600 ) / 60;
    title->seconds  = ( title->duration / 90000 ) % 60;
    hb_log( "bd: duration is %02d:%02d:%02d (%"PRId64" ms)",
            title->hours, title->minutes, title->seconds,
            title->duration / 90 );

    /* ignore short titles because they're often stills */
    if( ti->duration < min_duration )
    {
        hb_log( "bd: ignoring title (too short)" );
        goto fail;
    }

    BLURAY_STREAM_INFO * bdvideo = &ti->clips[0].video_streams[0];

    title->video_id = bdvideo->pid;
    title->video_stream_type = bdvideo->coding_type;

    hb_log( "bd: video id=0x%x, stream type=%s, format %s", title->video_id,
            bdvideo->coding_type == BLURAY_STREAM_TYPE_VIDEO_MPEG1 ? "MPEG1" :
            bdvideo->coding_type == BLURAY_STREAM_TYPE_VIDEO_MPEG2 ? "MPEG2" :
            bdvideo->coding_type == BLURAY_STREAM_TYPE_VIDEO_VC1 ? "VC-1" :
            bdvideo->coding_type == BLURAY_STREAM_TYPE_VIDEO_H264 ? "H.264" :
            "Unknown",
            bdvideo->format == BLURAY_VIDEO_FORMAT_480I ? "480i" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_576I ? "576i" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_480P ? "480p" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_1080I ? "1080i" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_720P ? "720p" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_1080P ? "1080p" :
            bdvideo->format == BLURAY_VIDEO_FORMAT_576P ? "576p" :
            "Unknown"
          );

    if ( bdvideo->coding_type == BLURAY_STREAM_TYPE_VIDEO_VC1 &&
       ( bdvideo->format == BLURAY_VIDEO_FORMAT_480I ||
         bdvideo->format == BLURAY_VIDEO_FORMAT_576I ||
         bdvideo->format == BLURAY_VIDEO_FORMAT_1080I ) )
    {
        hb_log( "bd: Interlaced VC-1 not supported" );
        goto fail;
    }

    switch( bdvideo->coding_type )
    {
        case BLURAY_STREAM_TYPE_VIDEO_MPEG1:
        case BLURAY_STREAM_TYPE_VIDEO_MPEG2:
            title->video_codec = WORK_DECMPEG2;
            title->video_codec_param = 0;
            break;

        case BLURAY_STREAM_TYPE_VIDEO_VC1:
            title->video_codec = WORK_DECAVCODECV;
            title->video_codec_param = CODEC_ID_VC1;
            break;

        case BLURAY_STREAM_TYPE_VIDEO_H264:
            title->video_codec = WORK_DECAVCODECV;
            title->video_codec_param = CODEC_ID_H264;
            title->flags |= HBTF_NO_IDR;
            break;

        default:
            hb_log( "scan: unknown video codec (0x%x)",
                    bdvideo->coding_type );
            goto fail;
    }

    switch ( bdvideo->aspect )
    {
        case BLURAY_ASPECT_RATIO_4_3:
            title->container_aspect = 4. / 3.;
            break;
        case BLURAY_ASPECT_RATIO_16_9:
            title->container_aspect = 16. / 9.;
            break;
        default:
            hb_log( "bd: unknown aspect" );
            goto fail;
    }
    hb_log( "bd: aspect = %g", title->container_aspect );

    /* Detect audio */
    // All BD clips are not all required to have the same audio.
    // But clips that have seamless transition are required
    // to have the same audio as the previous clip.
    // So find the clip that has the most other clips with the 
    // matching audio.
    // Max primary BD audios is 32
    uint8_t counts[32] = {0,};
    uint8_t matches[32] = {0,};
    int most_audio = 0;
    int audio_clip_index = 0;
    for ( ii = 0; ii < ti->clip_count; ii++ )
    {
        if ( matches[ii] ) continue;
        for ( jj = 0; jj < ti->clip_count; jj++ )
        {
            if ( matches[jj] ) continue;
            if ( bd_audio_equal( &ti->clips[ii], &ti->clips[jj] ) )
            {
                matches[ii] = matches[jj] = 1;
                counts[ii]++;
            }
        }
    }
    for ( ii = 0; ii < ti->clip_count; ii++ )
    {
        if ( most_audio < counts[ii] )
        {
            most_audio = counts[ii];
            audio_clip_index = ii;
        }
    }

    // Add all the audios found in the above clip.
    for ( ii = 0; ii < ti->clips[audio_clip_index].audio_stream_count; ii++ )
    {
        BLURAY_STREAM_INFO * bdaudio;

        bdaudio = &ti->clips[audio_clip_index].audio_streams[ii];

        switch( bdaudio->coding_type )
        {
            case BLURAY_STREAM_TYPE_AUDIO_TRUHD:
                // Add 2 audio tracks.  One for TrueHD and one for AC-3
                add_audio(ii, title->list_audio, bdaudio, 
                          HB_SUBSTREAM_BD_AC3, HB_ACODEC_AC3, 0);
                add_audio(ii, title->list_audio, bdaudio, 
                    HB_SUBSTREAM_BD_TRUEHD, HB_ACODEC_MPGA, CODEC_ID_TRUEHD);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_DTS:
                add_audio(ii, title->list_audio, bdaudio, 0, HB_ACODEC_DCA, 0);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_MPEG2:
            case BLURAY_STREAM_TYPE_AUDIO_MPEG1:
                add_audio(ii, title->list_audio, bdaudio, 0, 
                          HB_ACODEC_MPGA, CODEC_ID_MP2);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_AC3PLUS:
                add_audio(ii, title->list_audio, bdaudio, 0, 
                          HB_ACODEC_MPGA, CODEC_ID_EAC3);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_LPCM:
                add_audio(ii, title->list_audio, bdaudio, 0, 
                          HB_ACODEC_MPGA, CODEC_ID_PCM_BLURAY);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_AC3:
                add_audio(ii, title->list_audio, bdaudio, 0, HB_ACODEC_AC3, 0);
                break;

            case BLURAY_STREAM_TYPE_AUDIO_DTSHD_MASTER:
            case BLURAY_STREAM_TYPE_AUDIO_DTSHD:
                // Add 2 audio tracks.  One for DTS-HD and one for DTS
                add_audio(ii, title->list_audio, bdaudio, HB_SUBSTREAM_BD_DTS, 
                          HB_ACODEC_DCA, 0);
                // DTS-HD is special.  The substreams must be concatinated
                // DTS-core followed by DTS-hd-extensions.  Setting
                // a substream id of 0 says use all substreams.
                add_audio(ii, title->list_audio, bdaudio, 0,
                          HB_ACODEC_MPGA, CODEC_ID_DTS);
                break;

            default:
                hb_log( "scan: unknown audio pid 0x%x codec 0x%x",
                        bdaudio->pid, bdaudio->coding_type );
                break;
        }
    }

    /* Chapters */
    for ( ii = 0; ii < ti->chapter_count; ii++ )
    {
        chapter = calloc( sizeof( hb_chapter_t ), 1 );

        chapter->index = ii + 1;
        chapter->duration = ti->chapters[ii].duration;
        chapter->block_start = ti->chapters[ii].offset;

        int seconds;
        seconds            = ( chapter->duration + 45000 ) / 90000;
        chapter->hours     = seconds / 3600;
        chapter->minutes   = ( seconds % 3600 ) / 60;
        chapter->seconds   = seconds % 60;

        hb_log( "bd: chap %d packet=%"PRIu64", %"PRId64" ms",
                chapter->index,
                chapter->block_start,
                chapter->duration / 90 );

        hb_list_add( title->list_chapter, chapter );
    }
    hb_log( "bd: title %d has %d chapters", tt, ti->chapter_count );

    /* This title is ok so far */
    goto cleanup;

fail:
    hb_list_close( &title->list_audio );
    free( title );
    title = NULL;

cleanup:
    if ( ti ) bd_free_title_info( ti );

    return title;
}

/***********************************************************************
 * hb_bd_main_feature
 **********************************************************************/
int hb_bd_main_feature( hb_bd_t * d, hb_list_t * list_title )
{
    int longest = 0;
    int ii;
    uint64_t longest_duration = 0;
    int highest_rank = 0;
    int rank[8] = {0, 1, 3, 2, 6, 5, 7, 4};
    BLURAY_TITLE_INFO * ti;

    for ( ii = 0; ii < hb_list_count( list_title ); ii++ )
    {
        hb_title_t * title = hb_list_item( list_title, ii );
        ti = bd_get_title_info( d->bd, title->index - 1 );
        if ( ti ) 
        {
            BLURAY_STREAM_INFO * bdvideo = &ti->clips[0].video_streams[0];
            if ( title->duration > longest_duration * 0.7 && bdvideo->format < 8 )
            {
                if (highest_rank < rank[bdvideo->format] ||
                    ( title->duration > longest_duration &&
                          highest_rank == rank[bdvideo->format]))
                {
                    longest = title->index;
                    longest_duration = title->duration;
                    highest_rank = rank[bdvideo->format];
                }
            }
            bd_free_title_info( ti );
        }
        else if ( title->duration > longest_duration )
        {
            longest_duration = title->duration;
            longest = title->index;
        }
    }
    return longest;
}

/***********************************************************************
 * hb_bd_start
 ***********************************************************************
 * Title and chapter start at 1
 **********************************************************************/
int hb_bd_start( hb_bd_t * d, hb_title_t *title )
{
    BD_EVENT event;

    d->pkt_count = title->block_count;

    // Calling bd_get_event initializes libbluray event queue.
    bd_select_title( d->bd, title->index - 1 );
    bd_get_event( d->bd, &event );
    d->chapter = 1;
    d->stream = hb_bd_stream_open( title );
    if ( d->stream == NULL )
    {
        return 0;
    }
    return 1;
}

/***********************************************************************
 * hb_bd_stop
 ***********************************************************************
 *
 **********************************************************************/
void hb_bd_stop( hb_bd_t * d )
{
    if( d->stream ) hb_stream_close( &d->stream );
}

/***********************************************************************
 * hb_bd_seek
 ***********************************************************************
 *
 **********************************************************************/
int hb_bd_seek( hb_bd_t * d, float f )
{
    uint64_t packet = f * d->pkt_count;

    bd_seek(d->bd, packet * 192);
    d->next_chap = bd_get_current_chapter( d->bd ) + 1;
    return 1;
}

int hb_bd_seek_pts( hb_bd_t * d, uint64_t pts )
{
    bd_seek_time(d->bd, pts);
    d->next_chap = bd_get_current_chapter( d->bd ) + 1;
    return 1;
}

int hb_bd_seek_chapter( hb_bd_t * d, int c )
{
    int64_t pos;
    d->next_chap = c;
    pos = bd_seek_chapter( d->bd, c - 1 );
    return 1;
}

/***********************************************************************
 * hb_bd_read
 ***********************************************************************
 *
 **********************************************************************/
hb_buffer_t * hb_bd_read( hb_bd_t * d )
{
    int result;
    int error_count = 0;
    uint8_t buf[192];
    BD_EVENT event;
    uint64_t pos;
    hb_buffer_t * b;
    uint8_t discontinuity;
    int new_chap = 0;

    discontinuity = 0;
    while ( 1 )
    {
        if ( d->next_chap != d->chapter )
        {
            new_chap = d->chapter = d->next_chap;
        }
        result = next_packet( d->bd, buf );
        if ( result < 0 )
        {
            hb_error("bd: Read Error");
            pos = bd_tell( d->bd );
            bd_seek( d->bd, pos + 192 );
            error_count++;
            if (error_count > 10)
            {
                hb_error("bd: Error, too many consecutive read errors");
                return 0;
            }
            continue;
        }
        else if ( result == 0 )
        {
            return 0;
        }

        error_count = 0;
        while ( bd_get_event( d->bd, &event ) )
        {
            switch ( event.event )
            {
                case BD_EVENT_CHAPTER:
                    // The muxers expect to only get chapter 2 and above
                    // They write chapter 1 when chapter 2 is detected.
                    d->next_chap = event.param;
                    break;

                case BD_EVENT_PLAYITEM:
                    discontinuity = 1;
                    hb_deep_log(2, "bd: Playitem %u", event.param);
                    break;

                default:
                    break;
            }
        }
        // buf+4 to skip the BD timestamp at start of packet
        b = hb_ts_decode_pkt( d->stream, buf+4 );
        if ( b )
        {
            b->discontinuity = discontinuity;
            b->new_chap = new_chap;
            return b;
        }
    }
    return NULL;
}

/***********************************************************************
 * hb_bd_chapter
 ***********************************************************************
 * Returns in which chapter the next block to be read is.
 * Chapter numbers start at 1.
 **********************************************************************/
int hb_bd_chapter( hb_bd_t * d )
{
    return d->next_chap;
}

/***********************************************************************
 * hb_bd_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
void hb_bd_close( hb_bd_t ** _d )
{
    hb_bd_t * d = *_d;

    if( d->stream ) hb_stream_close( &d->stream );
    if( d->bd ) bd_close( d->bd );

    free( d );
    *_d = NULL;
}

/***********************************************************************
 * hb_bd_set_angle
 ***********************************************************************
 * Sets the angle to read
 **********************************************************************/
void hb_bd_set_angle( hb_bd_t * d, int angle )
{

    if ( !bd_select_angle( d->bd, angle) )
    {
        hb_log("bd_select_angle failed");
    }
}

static int check_ts_sync(const uint8_t *buf)
{
    // must have initial sync byte, no scrambling & a legal adaptation ctrl
    return (buf[0] == 0x47) && ((buf[3] >> 6) == 0) && ((buf[3] >> 4) > 0);
}

static int have_ts_sync(const uint8_t *buf, int psize)
{
    return check_ts_sync(&buf[0*psize]) && check_ts_sync(&buf[1*psize]) &&
           check_ts_sync(&buf[2*psize]) && check_ts_sync(&buf[3*psize]) &&
           check_ts_sync(&buf[4*psize]) && check_ts_sync(&buf[5*psize]) &&
           check_ts_sync(&buf[6*psize]) && check_ts_sync(&buf[7*psize]);
}

#define MAX_HOLE 192*80

static uint64_t align_to_next_packet(BLURAY *bd, uint8_t *pkt)
{
    uint8_t buf[MAX_HOLE];
    uint64_t pos = 0;
    uint64_t start = bd_tell(bd);
    uint64_t orig;
    uint64_t off = 192;

    memcpy(buf, pkt, 192);
    if ( start >= 192 ) {
        start -= 192;
    }
    orig = start;

    while (1)
    {
        if (bd_read(bd, buf+off, sizeof(buf)-off) == sizeof(buf)-off)
        {
            const uint8_t *bp = buf;
            int i;

            for ( i = sizeof(buf) - 8 * 192; --i >= 0; ++bp )
            {
                if ( have_ts_sync( bp, 192 ) )
                {
                    break;
                }
            }
            if ( i >= 0 )
            {
                pos = ( bp - buf );
                break;
            }
            off = 8 * 192;
            memcpy(buf, buf + sizeof(buf) - off, off);
            start += sizeof(buf) - off;
        }
        else
        {
            return 0;
        }
    }
    off = start + pos - 4;
    // bd_seek seeks to the nearest access unit *before* the requested position
    // we don't want to seek backwards, so we need to read until we get
    // past that position.
    bd_seek(bd, off);
    while (off > bd_tell(bd))
    {
        if (bd_read(bd, buf, 192) != 192)
        {
            break;
        }
    }
    return start - orig + pos;
}

static int next_packet( BLURAY *bd, uint8_t *pkt )
{
    int result;

    while ( 1 )
    {
        result = bd_read( bd, pkt, 192 );
        if ( result < 0 )
        {
            return -1;
        }
        if ( result < 192 )
        {
            return 0;
        }
        // Sync byte is byte 4.  0-3 are timestamp.
        if (pkt[4] == 0x47)
        {
            return 1;
        }
        // lost sync - back up to where we started then try to re-establish.
        uint64_t pos = bd_tell(bd);
        uint64_t pos2 = align_to_next_packet(bd, pkt);
        if ( pos2 == 0 )
        {
            hb_log( "next_packet: eof while re-establishing sync @ %"PRId64, pos );
            return 0;
        }
        hb_log( "next_packet: sync lost @ %"PRId64", regained after %"PRId64" bytes",
                 pos, pos2 );
    }
}

