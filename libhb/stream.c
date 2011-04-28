/* $Id$

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "hb.h"
#include "hbffmpeg.h"
#include "lang.h"
#include "a52dec/a52.h"
#include "mp4v2/mp4v2.h"
#include "libbluray/bluray.h"

#define min(a, b) a < b ? a : b

/*
 * This table defines how ISO MPEG stream type codes map to HandBrake
 * codecs. It is indexed by the 8 bit stream type and contains the codec
 * worker object id and a parameter for that worker proc (ignored except
 * for the ffmpeg-based codecs in which case it is the ffmpeg codec id).
 *
 * Entries with a worker proc id of 0 or a kind of 'U' indicate that HB
 * doesn't handle the stream type.
 * N - Not used
 * U - Unknown (to be determined by further processing)
 * A - Audio
 * V - Video
 * P - PCR
 */
typedef enum { N, U, A, V, P } kind_t;
typedef struct {
    kind_t kind; /* not handled / unknown / audio / video */
    int codec;          /* HB worker object id of codec */
    int codec_param;    /* param for codec (usually ffmpeg codec id) */
    const char* name;   /* description of type */
} stream2codec_t;

#define st(id, kind, codec, codec_param, name) \
 [id] = { kind, codec, codec_param, name }

static const stream2codec_t st2codec[256] = {
    st(0x01, V, WORK_DECMPEG2,     0,              "MPEG1"),
    st(0x02, V, WORK_DECMPEG2,     0,              "MPEG2"),
    st(0x03, A, HB_ACODEC_MPGA,    CODEC_ID_MP2,   "MPEG1"),
    st(0x04, A, HB_ACODEC_MPGA,    CODEC_ID_MP2,   "MPEG2"),
    st(0x05, N, 0,                 0,              "ISO 13818-1 private section"),
    st(0x06, U, 0,                 0,              "ISO 13818-1 PES private data"),
    st(0x07, N, 0,                 0,              "ISO 13522 MHEG"),
    st(0x08, N, 0,                 0,              "ISO 13818-1 DSM-CC"),
    st(0x09, N, 0,                 0,              "ISO 13818-1 auxiliary"),
    st(0x0a, N, 0,                 0,              "ISO 13818-6 encap"),
    st(0x0b, N, 0,                 0,              "ISO 13818-6 DSM-CC U-N msgs"),
    st(0x0c, N, 0,                 0,              "ISO 13818-6 Stream descriptors"),
    st(0x0d, N, 0,                 0,              "ISO 13818-6 Sections"),
    st(0x0e, N, 0,                 0,              "ISO 13818-1 auxiliary"),
    st(0x0f, A, HB_ACODEC_MPGA,    CODEC_ID_AAC,   "AAC"),
    st(0x10, V, WORK_DECAVCODECV,  CODEC_ID_MPEG4, "MPEG4"),
    st(0x11, A, HB_ACODEC_MPGA,    CODEC_ID_AAC_LATM, "LATM AAC"),
    st(0x12, U, 0,                 0,              "MPEG4 generic"),

    st(0x14, N, 0,                 0,              "ISO 13818-6 DSM-CC download"),

    st(0x1b, V, WORK_DECAVCODECV,  CODEC_ID_H264,  "H.264"),

    st(0x80, N, HB_ACODEC_MPGA,    CODEC_ID_PCM_BLURAY, "Digicipher II Video"),
    st(0x81, A, HB_ACODEC_AC3,     0,              "AC3"),
    st(0x82, A, HB_ACODEC_DCA,     0,              "DTS"),
    st(0x83, A, HB_ACODEC_LPCM,    0,              "LPCM"),
    st(0x84, U, 0,                 0,              "SDDS"),
    st(0x85, U, 0,                 0,              "ATSC Program ID"),
    st(0x86, A, HB_ACODEC_DCA,     0,              "DTS-HD MA"),
    st(0x87, A, HB_ACODEC_MPGA,    CODEC_ID_EAC3,  "E-AC3"),

    st(0x8a, A, HB_ACODEC_DCA,     0,              "DTS"),

    st(0x91, A, HB_ACODEC_AC3,     0,              "AC3"),
    st(0x92, N, 0,                 0,              "Subtitle"),

    st(0x94, U, 0,                 0,              "SDDS"),
    st(0xa0, V, 0,                 0,              "MSCODEC"),

    st(0xea, V, WORK_DECAVCODECV,  CODEC_ID_VC1,   "VC-1"),
};
#undef st

typedef enum {
    hb_stream_type_unknown = 0,
    transport,
    program,
    dvd_program,
    ffmpeg
} hb_stream_type_t;

#define kMaxNumberDecodeStreams 32
#define kMaxNumberDecodeSubStreams 2
#define kMaxNumberPMTStreams 32

typedef struct {
    hb_buffer_t *buf;
    hb_buffer_t *extra_buf;
    int8_t  skipbad;
    int8_t  continuity;
    uint8_t pkt_summary[8];
    int16_t pid;

    uint32_t format_id;
#define TS_FORMAT_ID_AC3 (('A' << 24) | ('C' << 16) | ('-' << 8) | '3')
    uint8_t stream_type;
    kind_t  stream_kind;
    int     number_substreams;
    uint8_t substream_type[kMaxNumberDecodeSubStreams];
    uint8_t current_substream;
    struct {
        int lang_code;
        int flags;
        int rate;
        int bitrate;
    } a52_info;
} hb_ts_stream_t;

struct hb_stream_s
{
    int     frames;             /* video frames so far */
    int     errors;             /* total errors so far */
    int     last_error_frame;   /* frame # at last error message */
    int     last_error_count;   /* # errors at last error message */
    int     packetsize;         /* Transport Stream packet size */

    uint8_t need_keyframe;      // non-zero if want to start at a keyframe
    hb_buffer_t *fwrite_buf;      /* PS buffer (set by hb_ts_stream_decode) */

    int      chapter;           /* Chapter that we are currently in */
    int64_t  chapter_end;       /* HB time that the current chapter ends */

    uint8_t ts_found_pcr;       // non-zero if we've found at least one input pcr
    int     ts_pcr_out;         // sequence number of most recent output pcr
    int     ts_pcr_in;          // sequence number of most recent input pcr
    int64_t ts_pcr;             // most recent input pcr
    int64_t ts_pcr_current;     // circular buffer of output pcrs

    uint8_t *ts_packet;         /* buffer for one TS packet */
    hb_ts_stream_t ts[kMaxNumberDecodeStreams];

    /*
     * Stuff before this point is dynamic state updated as we read the
     * stream. Stuff after this point is stream description state that
     * we learn during the initial scan but cache so it can be
     * reused during the conversion read.
     */
    uint8_t ts_number_pids;
    uint8_t ts_flags;           // stream characteristics:
#define         TS_HAS_PCR  (1 << 0)    // at least one PCR seen
#define         TS_HAS_RAP  (1 << 1)    // Random Access Point bit seen
#define         TS_HAS_RSEI (1 << 2)    // "Restart point" SEI seen
    uint8_t ts_IDRs;            // # IDRs found during duration scan


    char    *path;
    FILE    *file_handle;
    hb_stream_type_t hb_stream_type;
    hb_title_t *title;

    AVFormatContext *ffmpeg_ic;
    AVPacket *ffmpeg_pkt;
    uint8_t ffmpeg_video_id;

    struct
    {
        unsigned short program_number;
        unsigned short program_map_PID;
    } pat_info[kMaxNumberPMTStreams];
    int     ts_number_pat_entries;

    struct
    {
        int reading;
        unsigned char *tablebuf;
        unsigned int tablepos;
        unsigned char current_continuity_counter;

        int section_length;
        int program_number;
        unsigned int PCR_PID;
        uint32_t reg_desc;
        int program_info_length;
        struct
        {
            unsigned char stream_type;
            unsigned short elementary_PID;
            unsigned short ES_info_length;
            unsigned char *es_info_descriptor_data;
        } pmt_stream_info[kMaxNumberPMTStreams];
    } pmt_info;
};

/***********************************************************************
 * Local prototypes
 **********************************************************************/
static void hb_stream_duration(hb_stream_t *stream, hb_title_t *inTitle);
static void hb_ts_stream_init(hb_stream_t *stream);
static void hb_ts_stream_find_pids(hb_stream_t *stream);
static hb_buffer_t * hb_ts_stream_decode(hb_stream_t *stream);
static void hb_ts_stream_reset(hb_stream_t *stream);
static void hb_ts_stream_set_audio_list(hb_list_t *list_audio, hb_stream_t *stream);
static void hb_ps_stream_find_audio_ids(hb_stream_t *stream, hb_title_t *title);
static off_t align_to_next_packet(hb_stream_t *stream);

static int ffmpeg_open( hb_stream_t *stream, hb_title_t *title );
static void ffmpeg_close( hb_stream_t *d );
static hb_title_t *ffmpeg_title_scan( hb_stream_t *stream );
static hb_buffer_t *ffmpeg_read( hb_stream_t *stream );
static int ffmpeg_seek( hb_stream_t *stream, float frac );
static int ffmpeg_seek_ts( hb_stream_t *stream, int64_t ts );

/*
 * streams have a bunch of state that's learned during the scan. We don't
 * want to throw away the state when scan does a close then relearn
 * everything when reader does an open. So we save the stream state on
 * the close following a scan and reuse it when 'reader' does an open.
 */
static hb_list_t *stream_state_list;

static hb_stream_t *hb_stream_lookup( const char *path )
{
    if ( stream_state_list == NULL )
        return NULL;

    hb_stream_t *ss;
    int i = 0;

    while ( ( ss = hb_list_item( stream_state_list, i++ ) ) != NULL )
    {
        if ( strcmp( path, ss->path ) == 0 )
        {
            break;
        }
    }
    return ss;
}

static void hb_stream_state_delete( hb_stream_t *ss )
{
    hb_list_rem( stream_state_list, ss );
    free( ss->path );
    free( ss );
}

/*
 * logging routines.
 * these frontend hb_log because transport streams can have a lot of errors
 * so we want to rate limit messages. this routine limits the number of
 * messages to at most one per minute of video. other errors that occur
 * during the minute are counted & the count is output with the next
 * error msg we print.
 */
static void ts_warn_helper( hb_stream_t *stream, char *log, va_list args )
{
    // limit error printing to at most one per minute of video (at 30fps)
    ++stream->errors;
    if ( stream->frames - stream->last_error_frame >= 30*60 )
    {
        char msg[256];

        vsnprintf( msg, sizeof(msg), log, args );

        if ( stream->errors - stream->last_error_count < 10 )
        {
            hb_log( "stream: error near frame %d: %s", stream->frames, msg );
        }
        else
        {
            int Edelta = stream->errors - stream->last_error_count;
            double Epcnt = (double)Edelta * 100. /
                            (stream->frames - stream->last_error_frame);
            hb_log( "stream: %d new errors (%.0f%%) up to frame %d: %s",
                    Edelta, Epcnt, stream->frames, msg );
        }
        stream->last_error_frame = stream->frames;
        stream->last_error_count = stream->errors;
    }
}

static void ts_warn( hb_stream_t*, char*, ... ) HB_WPRINTF(2,3);
static void ts_err( hb_stream_t*, int, char*, ... ) HB_WPRINTF(3,4);

static void ts_warn( hb_stream_t *stream, char *log, ... )
{
    va_list args;
    va_start( args, log );
    ts_warn_helper( stream, log, args );
    va_end( args );
}

static kind_t ts_stream_kind( hb_stream_t *stream, int curstream )
{
    return st2codec[stream->ts[curstream].stream_type].kind;
}

static int index_of_pid(hb_stream_t *stream, int pid)
{
    int i;

    for ( i = 0; i < stream->ts_number_pids; ++i )
    {
        if ( pid == stream->ts[i].pid )
        {
            return i;
        }
    }

    return -1;
}

static int index_of_substream(hb_stream_t *stream, int pid, int substream_type)
{
    int ii;
    int idx = index_of_pid( stream, pid );

    for ( ii = 0; ii < stream->ts[idx].number_substreams; ii++ )
    {
        if ( stream->ts[idx].substream_type[ii] == substream_type )
            return ii;
    }
    return -1;
}

static int index_of_video(hb_stream_t *stream)
{
    int i;

    for ( i = 0; i < stream->ts_number_pids; ++i )
        if ( V == stream->ts[i].stream_kind )
            return i;

    return -1;
}

static void ts_err( hb_stream_t *stream, int curstream, char *log, ... )
{
    va_list args;
    va_start( args, log );
    ts_warn_helper( stream, log, args );
    va_end( args );

    stream->ts[curstream].skipbad = 1;
    stream->ts[curstream].continuity = -1;
}

static int check_ps_sync(const uint8_t *buf)
{
    // a legal MPEG program stream must start with a Pack header in the
    // first four bytes.
    return (buf[0] == 0x00) && (buf[1] == 0x00) &&
           (buf[2] == 0x01) && (buf[3] == 0xba);
}

static int check_ps_sc(const uint8_t *buf)
{
    // a legal MPEG program stream must start with a Pack followed by a
    // some other start code. If we've already verified the pack, this skip
    // it and checks for a start code prefix.
    int pos = 14 + ( buf[13] & 0x7 );   // skip over the PACK
    return (buf[pos+0] == 0x00) && (buf[pos+1] == 0x00) && (buf[pos+2] == 0x01);
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

static int hb_stream_check_for_ts(const uint8_t *buf)
{
    // transport streams should have a sync byte every 188 bytes.
    // search the first 8KB of buf looking for at least 8 consecutive
    // correctly located sync patterns.
    int offset = 0;

    for ( offset = 0; offset < 8*1024-8*188; ++offset )
    {
        if ( have_ts_sync( &buf[offset], 188) )
            return 188 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 192) )
            return 192 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 204) )
            return 204 | (offset << 8);
        if ( have_ts_sync( &buf[offset], 208) )
            return 208 | (offset << 8);
    }
    return 0;
}

static int hb_stream_check_for_ps(hb_stream_t *stream)
{
    uint8_t buf[2048*4];
    uint8_t sc_buf[4];
    int pos = 0;
    int hits = 0;

    fseek(stream->file_handle, 0, SEEK_SET);

    // program streams should start with a PACK then some other mpeg start 
    // code (usually a SYS but that might be missing if we only have a clip). 
    while (pos < 512 * 1024)
    {
        int offset;

        if ( fread(buf, 1, sizeof(buf), stream->file_handle) != sizeof(buf) )
            return 0;

        for ( offset = 0; offset < 8*1024-27; ++offset )
        {
            if ( check_ps_sync( &buf[offset] ) && check_ps_sc( &buf[offset] ) )
            {
                int pes_offset, prev, data_len;
                uint8_t sid;

                if ( ++hits == 3 )
                    return 1;
                pes_offset = 14 + (buf[13] & 0x7);
                sid = buf[pes_offset+3];
                data_len = (buf[pes_offset+4] << 8) + buf[pes_offset+5];
                if ( data_len && sid > 0xba && sid < 0xf9 )
                {
                    prev = ftell( stream->file_handle );
                    pos = pes_offset + 6 + data_len + prev;
                    fseek( stream->file_handle, pos, SEEK_SET );
                    if ( fread(sc_buf, 1, 4, stream->file_handle) != 4 )
                        return 0;
                    if (sc_buf[0] == 0x00 && sc_buf[1] == 0x00 && 
                        sc_buf[2] == 0x01)
                    {
                        return 1;
                    }
                    fseek( stream->file_handle, prev, SEEK_SET );
                }
            }
        }
        fseek( stream->file_handle, -27, SEEK_CUR );
        pos = ftell( stream->file_handle );
    }
    return 0;
}

static int hb_stream_check_for_dvd_ps(const uint8_t *buf)
{
    // DVD program streams should have a Pack header every 2048 bytes.
    // check that we have 4 of these in a row.
    return check_ps_sync(&buf[0*2048]) && check_ps_sync(&buf[1*2048]) &&
           check_ps_sync(&buf[2*2048]) && check_ps_sync(&buf[3*2048]);
}

static int hb_stream_get_type(hb_stream_t *stream)
{
    uint8_t buf[2048*4];

    if ( fread(buf, 1, sizeof(buf), stream->file_handle) == sizeof(buf) )
    {
        int psize;
        if ( ( psize = hb_stream_check_for_ts(buf) ) != 0 )
        {
            int offset = psize >> 8;
            psize &= 0xff;
            hb_log("file is MPEG Transport Stream with %d byte packets"
                   " offset %d bytes", psize, offset);
            stream->packetsize = psize;
            stream->hb_stream_type = transport;
            hb_ts_stream_init(stream);
            if ( index_of_video( stream ) < 0 )
            {
                return 0;
            }
            return 1;
        }
        if ( hb_stream_check_for_dvd_ps(buf) != 0 )
        {
            hb_log("file is MPEG DVD Program Stream");
            stream->hb_stream_type = dvd_program;
            return 1;
        }
        if ( hb_stream_check_for_ps(stream) != 0 )
        {
            hb_log("file is MPEG Program Stream");
            stream->hb_stream_type = program;
            return 1;
        }
    }
    return 0;
}

static void hb_stream_delete_dynamic( hb_stream_t *d )
{
    if( d->file_handle )
    {
        fclose( d->file_handle );
        d->file_handle = NULL;
    }

    int i=0;

    if ( d->ts_packet )
    {
        free( d->ts_packet );
        d->ts_packet = NULL;
    }
    for (i = 0; i < kMaxNumberDecodeStreams; i++)
    {
        if (d->ts[i].buf)
        {
            hb_buffer_close(&(d->ts[i].buf));
            hb_buffer_close(&(d->ts[i].extra_buf));
            d->ts[i].buf = NULL;
            d->ts[i].extra_buf = NULL;
        }
    }
}

static void hb_stream_delete( hb_stream_t *d )
{
    hb_stream_delete_dynamic( d );
    free( d->path );
    free( d );
}

static int find_substream( hb_ts_stream_t *ts, int substream )
{
    int i, all_match = -1;

    if ( ts->number_substreams == 0 )
        return 0;

    for ( i = 0; i < ts->number_substreams; i++ )
    {
        if ( ts->substream_type[i] == 0 )
            all_match = 0;
        if ( ts->substream_type[i] == substream )
        {
            return ts->substream_type[i];
        }
    }
    return all_match;
}

static void remove_substream( hb_ts_stream_t *ts, int idx )
{
    int i;

    for ( i = idx; i < ts->number_substreams-1; i++ )
    {
        ts->substream_type[i] = ts->substream_type[i+1];
    }
    ts->number_substreams--;
}

static int audio_inactive( hb_stream_t *stream, int idx )
{
    int pid = stream->ts[idx].pid;

    if ( pid < 0 )
    {
        // PID declared inactive by hb_stream_title_scan
        return 1;
    }
    if ( pid == stream->pmt_info.PCR_PID )
    {
        // PCR PID is always active
        return 0;
    }

    // Must go backwards thru the list of substreams because we may
    // remove one which would shift the list down.
    int j;
    for ( j = stream->ts[idx].number_substreams - 1; j >= 0; j-- )
    {
        int substream = stream->ts[idx].substream_type[j];

        // see if we should make the stream inactive because scan.c didn't
        // find a valid audio bitstream.
        int i;
        for ( i = 0; i < hb_list_count( stream->title->list_audio ); ++i )
        {
            hb_audio_t *audio = hb_list_item( stream->title->list_audio, i );
            if ( audio->id == ((substream << 16) | pid) )
            {
                return 0;
            }
        }
        remove_substream( &stream->ts[idx], j );
    }

    // not in the title's audio list - declare the PID inactive
    stream->ts[idx].pid = -stream->ts[idx].pid;
    return 1;
}

/***********************************************************************
 * hb_stream_open
 ***********************************************************************
 *
 **********************************************************************/
hb_stream_t * hb_stream_open( char *path, hb_title_t *title )
{
    FILE *f = fopen( path, "rb" );
    if ( f == NULL )
    {
        hb_log( "hb_stream_open: open %s failed", path );
        return NULL;
    }

    hb_stream_t *d = calloc( sizeof( hb_stream_t ), 1 );
    if ( d == NULL )
    {
        fclose( f );
        hb_log( "hb_stream_open: can't allocate space for %s stream state", path );
        return NULL;
    }

    /*
     * if we're opening the stream to read & convert, we need
     * the state we saved when we scanned the stream. if we're
     * opening the stream to scan it we want to rebuild the state
     * (even if we have saved state, the stream may have changed).
     */
    hb_stream_t *ss = hb_stream_lookup( path );
    if ( title && ss && ss->hb_stream_type != ffmpeg )
    {
        /*
         * copy the saved state since we might be encoding the same stream
         * multiple times.
         */
        memcpy( d, ss, sizeof(*d) );
        d->file_handle = f;
        d->title = title;
        d->path = strdup( path );

        if ( d->hb_stream_type == transport )
        {
            d->ts_packet = malloc( d->packetsize );

            int i;
            for ( i = 0; i < d->ts_number_pids; i++)
            {
                if ( d->ts[i].stream_kind == A &&
                     audio_inactive( d, i ) )
                {
                    // this PID isn't wanted (we don't have a codec for it
                    // or scan didn't find audio parameters)
                    continue;
                }
                d->ts[i].buf = hb_buffer_init(d->packetsize);
                d->ts[i].extra_buf = hb_buffer_init(d->packetsize);
                d->ts[i].buf->size = 0;
                d->ts[i].extra_buf->size = 0;
            }
            hb_stream_seek( d, 0. );
        }
        return d;
    }

    /*
     * opening for scan - delete any saved state then (re)scan the stream.
     * If it's something we can deal with (MPEG2 PS or TS) return a stream
     * reference structure & null otherwise.
     */
    if ( ss != NULL )
    {
        hb_stream_state_delete( ss );
    }
    d->file_handle = f;
    d->title = title;
    d->path = strdup( path );
    if (d->path != NULL )
    {
        if ( hb_stream_get_type( d ) != 0 )
        {
            return d;
        }
        fclose( d->file_handle );
        d->file_handle = NULL;
        if ( ffmpeg_open( d, title ) )
        {
            return d;
        }
    }
    if ( d->file_handle )
    {
        fclose( d->file_handle );
    }
    if (d->path)
    {
        free( d->path );
    }
    hb_log( "hb_stream_open: open %s failed", path );
    free( d );
    return NULL;
}

hb_stream_t * hb_bd_stream_open( hb_title_t *title )
{
    int ii;

    hb_stream_t *d = calloc( sizeof( hb_stream_t ), 1 );
    if ( d == NULL )
    {
        hb_error( "hb_bd_stream_open: can't allocate space for stream state" );
        return NULL;
    }

    for (ii = 0; ii < kMaxNumberDecodeStreams; ii++)
    {
        d->ts[ii].continuity = -1;
        d->ts[ii].pid = -1;
    }

    d->file_handle = NULL;
    d->title = title;
    d->path = NULL;
    d->ts_packet = NULL;

    d->ts_number_pids = 0;
    d->ts[0].pid = title->video_id;
    d->ts[0].stream_type = title->video_stream_type;
    d->ts[0].stream_kind = V;
    d->ts[0].substream_type[0] = 0;
    d->ts[0].number_substreams = 1;
    d->ts_number_pids++;

    hb_audio_t * audio;
    for ( ii = 0; ( audio = hb_list_item( title->list_audio, ii ) ); ++ii )
    {
        int pid = audio->id & 0xFFFF;
        int idx = index_of_pid( d, pid );
        int substream_type = audio->config.in.substream_type;

        if ( idx < 0 )
        {
            // New pid
            d->ts[d->ts_number_pids].pid = audio->id & 0xFFFF;
            d->ts[d->ts_number_pids].stream_type = audio->config.in.stream_type;
            d->ts[d->ts_number_pids].substream_type[0] = substream_type;
            d->ts[d->ts_number_pids].number_substreams = 1;
            d->ts[d->ts_number_pids].stream_kind = A;
            d->ts_number_pids++;
        }
        else if ( d->ts[idx].number_substreams < kMaxNumberDecodeSubStreams )
        {
            // Only add substream if it has not already been added.
            if ( index_of_substream( d, pid, substream_type ) < 0 )
            {
                d->ts[idx].substream_type[d->ts[idx].number_substreams] = 
                                                                substream_type;
                d->ts[idx].number_substreams++;
                d->ts[idx].stream_kind = A;
            }
        }
        else
        {
            hb_error( "hb_bd_stream_open: Too many substreams. Dropping audio 0x%x.", audio->id );
        }
    }

    d->ts_flags = TS_HAS_RAP;
    // When scanning, title->job == NULL.  We don't need to wait for
    // a PCR when scanning. In fact, it trips us up on the first
    // preview of every title since we would have to read quite a
    // lot of data before finding the PCR.
    if (title->pcr_pid != 0xFFFF && title->job)
    {
        if ( index_of_pid( d, title->pcr_pid ) < 0 )
        {
            // BD PCR PID is specified to always be 0x1001
            d->ts[d->ts_number_pids].pid = 0x1001;
            d->ts[d->ts_number_pids].stream_kind = P;
            d->ts[d->ts_number_pids].number_substreams = 1;
            d->ts[d->ts_number_pids].substream_type[0] = 0;
            d->ts_number_pids++;
        }
    }

    d->packetsize = 192;
    d->hb_stream_type = transport;

    for ( ii = 0; ii < d->ts_number_pids; ii++ )
    {
        d->ts[ii].buf = hb_buffer_init(d->packetsize);
        d->ts[ii].extra_buf = hb_buffer_init(d->packetsize);
        d->ts[ii].buf->size = 0;
        d->ts[ii].extra_buf->size = 0;
    }

    return d;
}

/***********************************************************************
 * hb_stream_close
 ***********************************************************************
 * Closes and frees everything
 **********************************************************************/
void hb_stream_close( hb_stream_t ** _d )
{
    hb_stream_t *stream = * _d;

    if ( stream->hb_stream_type == ffmpeg )
    {
        ffmpeg_close( stream );
        hb_stream_delete( stream );
        *_d = NULL;
        return;
    }

    if ( stream->frames )
    {
        hb_log( "stream: %d good frames, %d errors (%.0f%%)", stream->frames,
                stream->errors, (double)stream->errors * 100. /
                (double)stream->frames );
    }

    /*
     * if the stream was opened for a scan, cache the result, otherwise delete
     * the state.
     */
    if ( stream->title == NULL )
    {
        hb_stream_delete_dynamic( stream );
        if ( stream_state_list == NULL )
        {
            stream_state_list = hb_list_init();
        }
        hb_list_add( stream_state_list, stream );
    }
    else
    {
        hb_stream_delete( stream );
    }
    *_d = NULL;
}

/* when the file was first opened we made entries for all the audio elementary
 * streams we found in it. Streams that were later found during the preview scan
 * now have an audio codec, type, rate, etc., associated with them. At the end
 * of the scan we delete all the audio entries that weren't found by the scan
 * or don't have a format we support. This routine deletes audio entry 'indx'
 * by setting its PID to an invalid value so no packet will match it. (We can't
 * move any of the entries since the index of the entry is used as the id
 * of the media stream for HB. */
static void hb_stream_delete_entry(hb_stream_t *stream, int indx)
{
    if ( stream->ts[indx].pid > 0 )
    {
        stream->ts[indx].pid = -stream->ts[indx].pid;
    }
}

/***********************************************************************
 * hb_ps_stream_title_scan
 ***********************************************************************
 *
 **********************************************************************/
hb_title_t * hb_stream_title_scan(hb_stream_t *stream)
{
    if ( stream->hb_stream_type == ffmpeg )
        return ffmpeg_title_scan( stream );

    // 'Barebones Title'
    hb_title_t *aTitle = hb_title_init( stream->path, 0 );
    aTitle->type = HB_STREAM_TYPE;
    aTitle->index = 1;

    // Copy part of the stream path to the title name
    char *sep = strrchr(stream->path, '/');
    if (sep)
        strcpy(aTitle->name, sep+1);
    char *dot_term = strrchr(aTitle->name, '.');
    if (dot_term)
        *dot_term = '\0';

    // Height, width,  rate and aspect ratio information is filled in when the previews are built

    hb_stream_duration(stream, aTitle);

    // One Chapter
    hb_chapter_t * chapter;
    chapter = calloc( sizeof( hb_chapter_t ), 1 );
    chapter->index = 1;
    chapter->duration = aTitle->duration;
    chapter->hours = aTitle->hours;
    chapter->minutes = aTitle->minutes;
    chapter->seconds = aTitle->seconds;
    hb_list_add( aTitle->list_chapter, chapter );

    // Figure out how many audio streams we really have:
    // - For transport streams, for each PID listed in the PMT (whether
    //   or not it was an audio stream type) read the bitstream until we
    //   find an packet from that PID containing a PES header and see if
    //   the elementary stream is an audio type.
    // - For program streams read the first 4MB and take every unique
    //   audio stream we find.
    if (stream->hb_stream_type == transport)
    {
        int i;

        hb_ts_stream_set_audio_list(aTitle->list_audio, stream);

        // make sure we're grabbing the PCR PID
        if ( index_of_pid( stream, stream->pmt_info.PCR_PID ) < 0 )
        {
            stream->ts[stream->ts_number_pids].pid = stream->pmt_info.PCR_PID;
            stream->ts[stream->ts_number_pids].stream_kind = P;
            stream->ts[stream->ts_number_pids].number_substreams = 1;
            stream->ts[stream->ts_number_pids].substream_type[0] = 0;
            stream->ts_number_pids++;
        }

        for (i = 0; i < stream->ts_number_pids; i++)
        {
            kind_t kind = stream->ts[i].stream_kind;

            if ( kind == N || kind == U )
            {
                hb_stream_delete_entry(stream, i);
            }
        }

        // set the video id, codec & muxer
        int idx = index_of_video( stream );
        if ( idx < 0 )
        {
            hb_title_close( &aTitle );
            return NULL;
        }

        stream->ts[idx].number_substreams = 1;
        stream->ts[idx].substream_type[0] = 0;
        aTitle->video_id = stream->ts[idx].pid;
        aTitle->video_codec = st2codec[stream->ts[idx].stream_type].codec;
        aTitle->video_codec_param = st2codec[stream->ts[idx].stream_type].codec_param;
        aTitle->demuxer = HB_MPEG2_TS_DEMUXER;

        if ( ( stream->ts_flags & TS_HAS_PCR ) == 0 )
        {
            hb_log( "transport stream missing PCRs - using video DTS instead" );
        }

        if ( stream->ts_IDRs < 1 )
        {
            hb_log( "transport stream doesn't seem to have video IDR frames" );
            aTitle->flags |= HBTF_NO_IDR;
        }
    }
    else
    {
        hb_ps_stream_find_audio_ids(stream, aTitle);
    }

    return aTitle;
}

/*
 * read the next transport stream packet from 'stream'. Return NULL if
 * we hit eof & a pointer to the sync byte otherwise.
 */
static const uint8_t *next_packet( hb_stream_t *stream )
{
    uint8_t *buf = stream->ts_packet + stream->packetsize - 188;

    while ( 1 )
    {
        if ( fread(stream->ts_packet, 1, stream->packetsize, stream->file_handle) !=
             stream->packetsize )
        {
            return NULL;
        }
        if (buf[0] == 0x47)
        {
            return buf;
        }
        // lost sync - back up to where we started then try to re-establish.
        off_t pos = ftello(stream->file_handle) - stream->packetsize;
        off_t pos2 = align_to_next_packet(stream);
        if ( pos2 == 0 )
        {
            hb_log( "next_packet: eof while re-establishing sync @ %"PRId64, pos );
            return NULL;
        }
        ts_warn( stream, "next_packet: sync lost @ %"PRId64", regained after %"PRId64" bytes",
                 pos, pos2 );
    }
}

/*
 * skip to the start of the next PACK header in program stream src_stream.
 */
static void skip_to_next_pack( hb_stream_t *src_stream )
{
    // scan forward until we find the start of the next pack
    uint32_t strt_code = -1;
    int c;

    flockfile( src_stream->file_handle );
    while ( ( c = getc_unlocked( src_stream->file_handle ) ) != EOF )
    {
        strt_code = ( strt_code << 8 ) | c;
        if ( strt_code == 0x000001ba )
            // we found the start of the next pack
            break;
    }
    funlockfile( src_stream->file_handle );

    // if we didn't terminate on an eof back up so the next read
    // starts on the pack boundary.
    if ( c != EOF )
    {
        fseeko( src_stream->file_handle, -4, SEEK_CUR );
    }
}

static int isIframe( hb_stream_t *stream, const uint8_t *buf, int adapt_len )
{
    // For mpeg2: look for a gop start or i-frame picture start
    // for h.264: look for idr nal type or a slice header for an i-frame
    // for vc1:   look for a Sequence header
    int i;
    uint32_t strid = 0;


    if ( stream->ts[0].stream_type <= 2 )
    {
        // This section of the code handles MPEG-1 and MPEG-2 video streams
        for (i = 13 + adapt_len; i < 188; i++)
        {
            strid = (strid << 8) | buf[i];
            if ( ( strid >> 8 ) == 1 )
            {
                // we found a start code
                uint8_t id = strid;
                switch ( id )
                {
                    case 0xB8: // group_start_code (GOP header)
                    case 0xB3: // sequence_header code
                        return 1;

                    case 0x00: // picture_start_code
                        // picture_header, let's see if it's an I-frame
                        if (i<185)
                        {
                            // check if picture_coding_type == 1
                            if ((buf[i+2] & (0x7 << 3)) == (1 << 3))
                            {
                                // found an I-frame picture
                                return 1;
                            }
                        }
                        break;
                }
            }
        }
        // didn't find an I-frame
        return 0;
    }
    if ( stream->ts[0].stream_type == 0x1b )
    {
        // we have an h.264 stream 
        for (i = 13 + adapt_len; i < 188; i++)
        {
            strid = (strid << 8) | buf[i];
            if ( ( strid >> 8 ) == 1 )
            {
                // we found a start code - remove the ref_idc from the nal type
                uint8_t nal_type = strid & 0x1f;
                if ( nal_type == 0x05 )
                    // h.264 IDR picture start
                    return 1;
            }
        }
        // didn't find an I-frame
        return 0;
    }
    if ( stream->ts[0].stream_type == 0xea )
    {
        // we have an vc1 stream 
        for (i = 13 + adapt_len; i < 188; i++)
        {
            strid = (strid << 8) | buf[i];
            if ( strid == 0x10f )
            {
                // the ffmpeg vc1 decoder requires a seq hdr code in the first
                // frame.
                return 1;
            }
        }
        // didn't find an I-frame
        return 0;
    }

    // we don't understand the stream type so just say "yes" otherwise
    // we'll discard all the video.
    return 1;
}

/*
 * scan the next MB of 'stream' to find the next start packet for
 * the Packetized Elementary Stream associated with TS PID 'pid'.
 */
static const uint8_t *hb_ts_stream_getPEStype(hb_stream_t *stream, uint32_t pid)
{
    int npack = 300000; // max packets to read

    while (--npack >= 0)
    {
        const uint8_t *buf = next_packet( stream );
        if ( buf == NULL )
        {
            hb_log("hb_ts_stream_getPEStype: EOF while searching for PID 0x%x", pid);
            return 0;
        }

        // while we're reading the stream, check if it has valid PCRs
        // and/or random access points.
        uint32_t pack_pid = ( (buf[1] & 0x1f) << 8 ) | buf[2];
        if ( pack_pid == stream->pmt_info.PCR_PID )
        {
            if ( ( buf[5] & 0x10 ) &&
                 ( ( ( buf[3] & 0x30 ) == 0x20 ) ||
                   ( ( buf[3] & 0x30 ) == 0x30 && buf[4] > 6 ) ) )
            {
                stream->ts_flags |= TS_HAS_PCR;
            }
        }
        if ( buf[5] & 0x40 )
        {
            stream->ts_flags |= TS_HAS_RAP;
        }

        /*
         * The PES header is only in TS packets with 'start' set so we check
         * that first then check for the right PID.
         */
        if ((buf[1] & 0x40) == 0 || pack_pid != pid )
        {
            // not a start packet or not the pid we want
            continue;
        }

        /* skip over the TS hdr to return a pointer to the PES hdr */
        int udata = 4;
        switch (buf[3] & 0x30)
        {
            case 0x00: // illegal
            case 0x20: // fill packet
                continue;

            case 0x30: // adaptation
                if (buf[4] > 182)
                {
                    hb_log("hb_ts_stream_getPEStype: invalid adaptation field length %d for PID 0x%x", buf[4], pid);
                    continue;
                }
                udata += buf[4] + 1;
                break;
        }
        /* PES hdr has to begin with an mpeg start code */
        if (buf[udata+0] == 0x00 && buf[udata+1] == 0x00 && buf[udata+2] == 0x01)
        {
            return &buf[udata];
        }
    }

    /* didn't find it */
    return 0;
}

static uint64_t hb_ps_stream_getVideoPTS(hb_stream_t *stream)
{
    hb_buffer_t *buf;
    hb_list_t *list = hb_list_init();
    // how many blocks we read while searching for a video PES header
    int blksleft = 1024;
    uint64_t pts = 0;

    while (--blksleft >= 0 && (buf = hb_stream_read(stream)) != NULL)
    {
        hb_buffer_t *es;

        // 'buf' contains an MPEG2 PACK - get a list of all it's
        //  elementary streams
        hb_demux_ps( buf, list, 0 );

        while ( ( es = hb_list_item( list, 0 ) ) )
        {
            hb_list_rem( list, es );
            if ( es->id == 0xe0 )
            {
                // this PES contains video - if there's a PTS we're done
                // hb_demux_ps left the PTS in buf_es->start.
                if ( es->start != ~0 )
                {
                    pts = es->start;
                    blksleft = 0;
                    break;
                }
            }
            hb_buffer_close( &es );
        }
    }
    hb_list_empty( &list );
    return pts;
}

/***********************************************************************
 * hb_stream_duration
 ***********************************************************************
 *
 * Finding stream duration is difficult.  One issue is that the video file
 * may have chunks from several different program fragments (main feature,
 * commercials, station id, trailers, etc.) all with their own base pts
 * value.  We can't find the piece boundaries without reading the entire
 * file but if we compute a rate based on time stamps from two different
 * pieces the result will be meaningless.  The second issue is that the
 * data rate of compressed video normally varies by 5-10x over the length
 * of the video. This says that we want to compute the rate over relatively
 * long segments to get a representative average but long segments increase
 * the likelihood that we'll cross a piece boundary.
 *
 * What we do is take time stamp samples at several places in the file
 * (currently 16) then compute the average rate (i.e., ticks of video per
 * byte of the file) for all pairs of samples (N^2 rates computed for N
 * samples). Some of those rates will be absurd because the samples came
 * from different segments. Some will be way low or high because the
 * samples came from a low or high motion part of the segment. But given
 * that we're comparing *all* pairs the majority of the computed rates
 * should be near the overall average.  So we median filter the computed
 * rates to pick the most representative value.
 *
 **********************************************************************/
struct pts_pos {
    uint64_t pos;   /* file position of this PTS sample */
    uint64_t pts;   /* PTS from video stream */
};

#define NDURSAMPLES 128

// get one (position, timestamp) sampple from a transport or program
// stream.
static struct pts_pos hb_sample_pts(hb_stream_t *stream, uint64_t fpos)
{
    struct pts_pos pp = { 0, 0 };

    if ( stream->hb_stream_type == transport )
    {
        const uint8_t *buf;
        fseeko( stream->file_handle, fpos, SEEK_SET );
        align_to_next_packet( stream );
        int pid = stream->ts[index_of_video(stream)].pid;
        buf = hb_ts_stream_getPEStype( stream, pid );
        if ( buf == NULL )
        {
            hb_log("hb_sample_pts: couldn't find video packet near %"PRIu64, fpos);
            return pp;
        }
        if ( ( buf[7] >> 7 ) != 1 )
        {
            hb_log("hb_sample_pts: no PTS in video packet near %"PRIu64, fpos);
            return pp;
        }
        pp.pts = ( ( (uint64_t)buf[9] >> 1 ) & 7 << 30 ) |
                 ( (uint64_t)buf[10] << 22 ) |
                 ( ( (uint64_t)buf[11] >> 1 ) << 15 ) |
                 ( (uint64_t)buf[12] << 7 ) |
                 ( (uint64_t)buf[13] >> 1 );

        if ( isIframe( stream, buf, -4 ) )
        {
            if (  stream->ts_IDRs < 255 )
            {
                ++stream->ts_IDRs;
            }
        }
    }
    else
    {
        // round address down to nearest dvd sector start
        fpos &=~ ( HB_DVD_READ_BUFFER_SIZE - 1 );
        fseeko( stream->file_handle, fpos, SEEK_SET );
        if ( stream->hb_stream_type == program )
        {
            skip_to_next_pack( stream );
        }
        pp.pts = hb_ps_stream_getVideoPTS( stream );
    }
    pp.pos = ftello(stream->file_handle);
    return pp;
}

static int dur_compare( const void *a, const void *b )
{
    const double *aval = a, *bval = b;
    return ( *aval < *bval ? -1 : ( *aval == *bval ? 0 : 1 ) );
}

// given an array of (position, time) samples, compute a max-likelihood
// estimate of the average rate by computing the rate between all pairs
// of samples then taking the median of those rates.
static double compute_stream_rate( struct pts_pos *pp, int n )
{
    int i, j;
    double rates[NDURSAMPLES * NDURSAMPLES / 8];
    double *rp = rates;

    // the following nested loops compute the rates between all pairs.
    *rp = 0;
    for ( i = 0; i < n-1; ++i )
    {
        // Bias the median filter by not including pairs that are "far"
        // from one another. This is to handle cases where the file is
        // made of roughly equal size pieces where a symmetric choice of
        // pairs results in having the same number of intra-piece &
        // inter-piece rate estimates. This would mean that the median
        // could easily fall in the inter-piece part of the data which
        // would give a bogus estimate. The 'ns' index creates an
        // asymmetry that favors locality.
        int ns = i + ( n >> 3 );
        if ( ns > n )
            ns = n;
        for ( j = i+1; j < ns; ++j )
        {
            if ( (uint64_t)(pp[j].pts - pp[i].pts) > 90000LL*3600*6 )
                break;
            if ( pp[j].pts != pp[i].pts && pp[j].pos > pp[i].pos )
            {
                *rp = ((double)( pp[j].pts - pp[i].pts )) /
                      ((double)( pp[j].pos - pp[i].pos ));
                ++rp;
            }
        }
    }
    // now compute and return the median of all the (n*n/2) rates we computed
    // above.
    int nrates = rp - rates;
    qsort( rates, nrates, sizeof (rates[0] ), dur_compare );
    return rates[nrates >> 1];
}

static void hb_stream_duration(hb_stream_t *stream, hb_title_t *inTitle)
{
    struct pts_pos ptspos[NDURSAMPLES];
    struct pts_pos *pp = ptspos;
    int i;

    fseeko(stream->file_handle, 0, SEEK_END);
    uint64_t fsize = ftello(stream->file_handle);
    uint64_t fincr = fsize / NDURSAMPLES;
    uint64_t fpos = fincr / 2;
    for ( i = NDURSAMPLES; --i >= 0; fpos += fincr )
    {
        *pp++ = hb_sample_pts(stream, fpos);
    }
    uint64_t dur = compute_stream_rate( ptspos, pp - ptspos ) * (double)fsize;
    inTitle->duration = dur;
    dur /= 90000;
    inTitle->hours    = dur / 3600;
    inTitle->minutes  = ( dur % 3600 ) / 60;
    inTitle->seconds  = dur % 60;

    rewind(stream->file_handle);
}

/***********************************************************************
 * hb_stream_read
 ***********************************************************************
 *
 **********************************************************************/
hb_buffer_t * hb_stream_read( hb_stream_t * src_stream )
{
    if ( src_stream->hb_stream_type == ffmpeg )
    {
        return ffmpeg_read( src_stream );
    }
    if ( src_stream->hb_stream_type == dvd_program )
    {
        hb_buffer_t *b  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);
        size_t amt_read = fread(b->data, HB_DVD_READ_BUFFER_SIZE, 1,
                                src_stream->file_handle);
        if (amt_read <= 0)
            hb_buffer_close( &b );
        return b;
    }
    if ( src_stream->hb_stream_type == program )
    {
        // a general program stream has arbitrary sized pack's. we're
        // currently positioned at the start of a pack so read up to but
        // not including the start of the next, expanding the buffer
        // as necessary.
        int c;

        // consume the first byte of the initial pack so we don't match on
        // it in the loop below.
        if ( ( c = getc( src_stream->file_handle ) ) == EOF )
            return 0;

        hb_buffer_t *b  = hb_buffer_init(HB_DVD_READ_BUFFER_SIZE);
        uint8_t *cp = b->data;
        uint8_t *ep = cp + b->alloc;
        uint32_t strt_code = -1;

        *cp++ = c;

        flockfile( src_stream->file_handle );
        while ( ( c = getc_unlocked( src_stream->file_handle ) ) != EOF )
        {
            strt_code = ( strt_code << 8 ) | c;
            if ( strt_code == 0x000001ba )
                // we found the start of the next pack
                break;
            if ( cp >= ep )
            {
                // need to expand the buffer
                int curSize = cp - b->data;
                hb_buffer_realloc( b, curSize * 2 );
                cp = b->data + curSize;
                ep = b->data + b->alloc;
            }
            *cp++ = c;
            // Non-video streams can emulate start codes, so we need
            // to inspect PES packets and skip over their data
            // sections to avoid mis-detection of the next pack header.
            if ( ( strt_code >> 8 ) == 0x000001 &&
                 ( strt_code & 0xff ) >= 0xbb )
            {
                int len = 0;
                c = getc_unlocked( src_stream->file_handle );
                if ( c == EOF )
                    break;
                len = c << 8;
                c = getc_unlocked( src_stream->file_handle );
                if ( c == EOF )
                    break;
                len |= c;
                if ( cp+len+2 > ep )
                {
                    // need to expand the buffer
                    int curSize = cp - b->data;
                    if ( curSize * 2 > curSize+len+2 )
                        hb_buffer_realloc( b, curSize * 2 );
                    else
                        hb_buffer_realloc( b, curSize + len + 2 );
                    cp = b->data + curSize;
                    ep = b->data + b->alloc;
                }
                *cp++ = len >> 8;
                *cp++ = len & 0xff;
                fread( cp, 1, len, src_stream->file_handle );
                cp += len;
            }
        }
        funlockfile( src_stream->file_handle );

        // if we didn't terminate on an eof back up so the next read
        // starts on the pack boundary.
        b->size = cp - b->data;
        if ( c != EOF )
        {
            fseeko( src_stream->file_handle, -4, SEEK_CUR );
            // Only 3 of the 4 bytes read were added to the buffer.
            b->size -= 3;
        }
        return b;
    }
    return hb_ts_stream_decode( src_stream );
}

void ffmpeg_flush_stream_buffers( hb_stream_t *stream )
{
    int i;
    AVFormatContext *ic = stream->ffmpeg_ic;

    for ( i = 0; i < ic->nb_streams; i++ )
    {
        if ( ic->streams[i]->codec && ic->streams[i]->codec->codec )
        {
            avcodec_flush_buffers( ic->streams[i]->codec );
        }
    }
}

int64_t ffmpeg_initial_timestamp( hb_stream_t * stream )
{
    AVStream *s = stream->ffmpeg_ic->streams[stream->ffmpeg_video_id];
    if ( s->nb_index_entries < 1 )
        return 0;

    return s->index_entries[0].timestamp;
}
int hb_stream_seek_chapter( hb_stream_t * stream, int chapter_num )
{

    if ( stream->hb_stream_type != ffmpeg )
    {
        // currently meaningliess for transport and program streams
        return 1;
    }
    if ( !stream || !stream->title ||
         chapter_num > hb_list_count( stream->title->list_chapter ) )
    {
        return 0;
    }

    int64_t sum_dur = 0;
    hb_chapter_t *chapter = NULL;
    int i;
    for ( i = 0; i < chapter_num; ++i)
    {
        chapter = hb_list_item( stream->title->list_chapter, i );
        sum_dur += chapter->duration;
    }
    stream->chapter = chapter_num - 1;
    stream->chapter_end = sum_dur;

    int64_t pos = ( ( ( sum_dur - chapter->duration ) * AV_TIME_BASE ) / 90000 ) + ffmpeg_initial_timestamp( stream );

    hb_deep_log( 2, "Seeking to chapter %d: starts %"PRId64", ends %"PRId64", AV pos %"PRId64,
                 chapter_num, sum_dur - chapter->duration, sum_dur, pos);

    if ( chapter_num > 1 && pos > 0 )
    {
        av_seek_frame( stream->ffmpeg_ic, -1, pos, 0);
        ffmpeg_flush_stream_buffers( stream );
    }
    return 1;
}

/***********************************************************************
 * hb_stream_chapter
 ***********************************************************************
 * Return the number of the chapter that we are currently in. We store
 * the chapter number starting from 0, so + 1 for the real chpater num.
 **********************************************************************/
int hb_stream_chapter( hb_stream_t * src_stream )
{
    return( src_stream->chapter + 1 );
}

/***********************************************************************
 * hb_stream_seek
 ***********************************************************************
 *
 **********************************************************************/
int hb_stream_seek( hb_stream_t * stream, float f )
{
    if ( stream->hb_stream_type == ffmpeg )
    {
        return ffmpeg_seek( stream, f );
    }
    off_t stream_size, cur_pos, new_pos;
    double pos_ratio = f;
    cur_pos = ftello( stream->file_handle );
    fseeko( stream->file_handle, 0, SEEK_END );
    stream_size = ftello( stream->file_handle );
    new_pos = (off_t) ((double) (stream_size) * pos_ratio);
    new_pos &=~ (HB_DVD_READ_BUFFER_SIZE - 1);

    int r = fseeko( stream->file_handle, new_pos, SEEK_SET );
    if (r == -1)
    {
        fseeko( stream->file_handle, cur_pos, SEEK_SET );
        return 0;
    }

    if ( stream->hb_stream_type == transport )
    {
        // We need to drop the current decoder output and move
        // forwards to the next transport stream packet.
        hb_ts_stream_reset(stream);
        if ( f > 0 )
        {
            if ( stream->ts_IDRs )
            {
                // the stream has IDRs so look for one.
                stream->need_keyframe = 1;
            }
        }
        else
        {
            // we're at the beginning - say we have video sync so that we
            // won't drop initial SPS & PPS data on an AVC stream.
            stream->need_keyframe = 0;
        }
    }
    else if ( stream->hb_stream_type == program )
    {
        skip_to_next_pack( stream );
    }

    return 1;
}

int hb_stream_seek_ts( hb_stream_t * stream, int64_t ts )
{
    if ( stream->hb_stream_type == ffmpeg )
    {
        return ffmpeg_seek_ts( stream, ts );
    }
    return -1;
}

static const char *stream_type_name (uint32_t reg_desc, uint8_t stream_type)
{
    if ( reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // Names for streams we know about.
        switch ( stream_type )
        {
            case 0x80:
                return "BD LPCM";

            case 0x83:
                return "TrueHD";

            case 0x84:
                return "E-AC3";

            case 0x85:
                return "DTS-HD HRA";

            case 0x86:
                return "DTS-HD MA";

            default:
                break;
        }
    }
    return st2codec[stream_type].name ? st2codec[stream_type].name : "Unknown";
}

static void set_ts_audio_description( hb_audio_t *audio, iso639_lang_t *lang )
{
    /* XXX
     * This is a duplicate of code in dvd.c - it should get factored out
     * into a common routine. We probably should only be putting the lang
     * code or a lang pointer into the audio config & let the common description
     * formatting routine in scan.c do all the stuff below.
     */
    const char *codec_name;
    AVCodecContext *cc;

    // Names for streams we know about.
    if ( audio->config.in.stream_type == 0x80 && 
         audio->config.in.reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // LPCM audio in bluray have an stype of 0x80
        codec_name = "BD LPCM";
    }
    else if ( audio->config.in.stream_type == 0x83 && 
         audio->config.in.reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // This is an interleaved TrueHD/AC-3 stream and the esid of
        // the AC-3 is 0x76
        if (audio->config.in.substream_type == HB_SUBSTREAM_BD_AC3)
            codec_name = "AC3";
        else
            codec_name = "TrueHD";
    }
    else if ( audio->config.in.stream_type == 0x86 && 
         audio->config.in.reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // This is an interleaved DTS-HD MA/DTS stream and the 
        // esid of the DTS is 0x71
        if (audio->config.in.substream_type == HB_SUBSTREAM_BD_DTS)
            codec_name = "DTS";
        else
            codec_name = "DTS-HD MA";
    }
    else if ( audio->config.in.stream_type == 0x85 && 
         audio->config.in.reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // DTS-HD HRA audio in bluray has an stype of 0x85
        // which conflicts with ATSC Program ID
        // To distinguish, Bluray streams have a reg_desc of HDMV
        // This is an interleaved DTS-HD HRA/DTS stream and the 
        // esid of the DTS is 0x71
        if (audio->config.in.substream_type == HB_SUBSTREAM_BD_DTS)
            codec_name = "DTS";
        else
            codec_name = "DTS-HD HRA";
    }
    else if ( audio->config.in.stream_type == 0x84 && 
         audio->config.in.reg_desc == STR4_TO_UINT32("HDMV") )
    {
        // EAC3 audio in bluray has an stype of 0x84
        // which conflicts with SDDS
        // To distinguish, Bluray streams have a reg_desc of HDMV
        codec_name = "E-AC3";
    }
    // For streams demuxed and decoded by ffmpeg, we have a cached context.
    // Use it to get the name and profile information.  Obtaining
    // the profile requires that ffmpeg has already probed the stream.
    else if ( audio->config.in.codec == HB_ACODEC_FFMPEG &&
         ( cc = hb_ffmpeg_context( audio->config.in.codec_param ) ) &&
         avcodec_find_decoder( cc->codec_id ) )
    {
        AVCodec *codec = avcodec_find_decoder( cc->codec_id );
        codec_name = codec->name;

        const char *profile_name;
        profile_name = av_get_profile_name( codec, cc->profile );
        if ( profile_name )
        {
            codec_name = profile_name;
        }
    }
    else if ( st2codec[audio->config.in.stream_type].kind == A )
    {
        codec_name = stream_type_name(audio->config.in.reg_desc,
                                      audio->config.in.stream_type);
    }
    // For streams demuxed by us and decoded by ffmpeg, we can lookup the
    // decoder name.
    else if ( audio->config.in.codec == HB_ACODEC_MPGA &&
              avcodec_find_decoder( audio->config.in.codec_param ) )
    {
        codec_name = avcodec_find_decoder( audio->config.in.codec_param )->name;
    }
    else
    {
        codec_name = audio->config.in.codec == HB_ACODEC_AC3 ? "AC3" :
                     audio->config.in.codec == HB_ACODEC_DCA ? "DTS" :
                     audio->config.in.codec == HB_ACODEC_MPGA ? "MPEG" : 
                     audio->config.in.codec == HB_ACODEC_LPCM ? "LPCM" : 
                     audio->config.in.codec == HB_ACODEC_FFMPEG ? "FFmpeg" :
                     "Unknown";
    }
    snprintf( audio->config.lang.description,
              sizeof( audio->config.lang.description ), "%s (%s)",
              strlen(lang->native_name) ? lang->native_name : lang->eng_name,
              codec_name );

    if (audio->config.in.codec == HB_ACODEC_FFMPEG)
    {
        int layout = audio->config.in.channel_layout;
        char *desc = audio->config.lang.description +
                        strlen( audio->config.lang.description );
        sprintf( desc, " (%d.%d ch)",
                 HB_INPUT_CH_LAYOUT_GET_DISCRETE_FRONT_COUNT(layout) +
                     HB_INPUT_CH_LAYOUT_GET_DISCRETE_REAR_COUNT(layout),
                 HB_INPUT_CH_LAYOUT_GET_DISCRETE_LFE_COUNT(layout) );
    }

    snprintf( audio->config.lang.simple, sizeof( audio->config.lang.simple ), "%s",
              strlen(lang->native_name) ? lang->native_name : lang->eng_name );
    snprintf( audio->config.lang.iso639_2, sizeof( audio->config.lang.iso639_2 ),
              "%s", lang->iso639_2);
}

static void set_audio_description( hb_audio_t *audio, iso639_lang_t *lang )
{
    /* XXX
     * This is a duplicate of code in dvd.c - it should get factored out
     * into a common routine. We probably should only be putting the lang
     * code or a lang pointer into the audio config & let the common description
     * formatting routine in scan.c do all the stuff below.
     */
    const char *codec_name;
    AVCodecContext *cc;

    // For streams demuxed and decoded by ffmpeg, we have a cached context.
    // Use it to get the name and profile information.  Obtaining
    // the profile requires that ffmpeg has already probed the stream.
    if ( audio->config.in.codec == HB_ACODEC_FFMPEG &&
         ( cc = hb_ffmpeg_context( audio->config.in.codec_param ) ) &&
         avcodec_find_decoder( cc->codec_id ) )
    {
        AVCodec *codec = avcodec_find_decoder( cc->codec_id );
        codec_name = codec->name;

        const char *profile_name;
        profile_name = av_get_profile_name( codec, cc->profile );
        if ( profile_name )
        {
            codec_name = profile_name;
        }
    }
    // For streams demuxed by us and decoded by ffmpeg, we can lookup the
    // decoder name.
    else if ( audio->config.in.codec == HB_ACODEC_MPGA &&
              avcodec_find_decoder( audio->config.in.codec_param ) )
    {
        codec_name = avcodec_find_decoder( audio->config.in.codec_param )->name;
    }
    else
    {
        codec_name = audio->config.in.codec == HB_ACODEC_AC3 ? "AC3" :
                     audio->config.in.codec == HB_ACODEC_DCA ? "DTS" :
                     audio->config.in.codec == HB_ACODEC_MPGA ? "MPEG" : 
                     audio->config.in.codec == HB_ACODEC_LPCM ? "LPCM" : 
                     audio->config.in.codec == HB_ACODEC_FFMPEG ? "FFmpeg" :
                     "Unknown";
    }
    snprintf( audio->config.lang.description,
              sizeof( audio->config.lang.description ), "%s (%s)",
              strlen(lang->native_name) ? lang->native_name : lang->eng_name,
              codec_name );

    if (audio->config.in.codec == HB_ACODEC_FFMPEG)
    {
        int layout = audio->config.in.channel_layout;
        char *desc = audio->config.lang.description +
                        strlen( audio->config.lang.description );
        sprintf( desc, " (%d.%d ch)",
                 HB_INPUT_CH_LAYOUT_GET_DISCRETE_FRONT_COUNT(layout) +
                     HB_INPUT_CH_LAYOUT_GET_DISCRETE_REAR_COUNT(layout),
                 HB_INPUT_CH_LAYOUT_GET_DISCRETE_LFE_COUNT(layout) );
    }

    snprintf( audio->config.lang.simple, sizeof( audio->config.lang.simple ), "%s",
              strlen(lang->native_name) ? lang->native_name : lang->eng_name );
    snprintf( audio->config.lang.iso639_2, sizeof( audio->config.lang.iso639_2 ),
              "%s", lang->iso639_2);
}

static void add_audio(int track, hb_list_t *list_audio, hb_stream_t *stream, int substream_type, uint32_t codec, uint32_t codec_param )
{
    hb_audio_t *audio;
    hb_ts_stream_t *ts = &stream->ts[track];

    audio = calloc( sizeof( hb_audio_t ), 1 );

    audio->id = (substream_type << 16) | ts->pid;
    audio->config.in.reg_desc = stream->pmt_info.reg_desc;
    audio->config.in.stream_type = ts->stream_type;
    audio->config.in.substream_type = substream_type;

    audio->config.in.codec = codec;
    audio->config.in.codec_param = codec_param;

    set_ts_audio_description( audio,
              lang_for_code( ts->a52_info.lang_code ) );

    hb_log("transport stream pid 0x%x (type 0x%x substream 0x%x) audio 0x%x",
           ts->pid, ts->stream_type, substream_type, audio->id);

    audio->config.in.track = track;
    hb_list_add( list_audio, audio );
}

static void hb_ts_stream_set_audio_list(
    hb_list_t *list_audio,
    hb_stream_t *stream)
{
    off_t cur_pos;
    const uint8_t *buf;
    int i;

    cur_pos = ftello(stream->file_handle);
    for ( i = 0; i < stream->ts_number_pids; i++ )
    {
        stream->ts[i].substream_type[0] = 0;
        stream->ts[i].number_substreams = 1;

        if ( stream->ts[i].stream_type == 0x80 && 
             stream->pmt_info.reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // LPCM audio in bluray have an stype of 0x80
            // 0x80 is used for other DigiCipher normally
            // To distinguish, Bluray streams have a reg_desc of HDMV
            stream->ts[i].stream_kind = A;
            add_audio(i, list_audio, stream, 0,
                      HB_ACODEC_MPGA, CODEC_ID_PCM_BLURAY );
            continue;
        }

        // The blu ray consortium apparently forgot to read the portion
        // of the MPEG spec that says one PID should map to one media
        // stream and multiplexed multiple types of audio into one PID
        // using the extended stream identifier of the PES header to
        // distinguish them. So we have to check if that's happening and
        // if so tell the runtime what esid we want.
        if ( stream->ts[i].stream_type == 0x83 && 
             stream->pmt_info.reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // This is an interleaved TrueHD/AC-3 stream and the esid of
            // the AC-3 is 0x76
            stream->ts[i].stream_kind = A;
            stream->ts[i].substream_type[0] = HB_SUBSTREAM_BD_AC3;
            add_audio(i, list_audio, stream, HB_SUBSTREAM_BD_AC3, 
                      HB_ACODEC_AC3, 0 );
            stream->ts[i].substream_type[1] = HB_SUBSTREAM_BD_TRUEHD;
            add_audio(i, list_audio, stream, HB_SUBSTREAM_BD_TRUEHD, 
                      HB_ACODEC_MPGA, CODEC_ID_TRUEHD );
            stream->ts[i].number_substreams = 2;
            continue;
        }
        if ( stream->ts[i].stream_type == 0x86 && 
             stream->pmt_info.reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // This is an interleaved DTS-HD MA/DTS stream and the 
            // esid of the DTS is 0x71
            stream->ts[i].stream_kind = A;
            stream->ts[i].substream_type[0] = HB_SUBSTREAM_BD_DTS;
            add_audio(i, list_audio, stream, HB_SUBSTREAM_BD_DTS, 
                      HB_ACODEC_DCA, 0 );
            stream->ts[i].substream_type[1] = 0;
            add_audio(i, list_audio, stream, 0, HB_ACODEC_MPGA, CODEC_ID_DTS );
            stream->ts[i].number_substreams = 2;
            continue;
        }
        if ( stream->ts[i].stream_type == 0x85 && 
             stream->pmt_info.reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // DTS-HD HRA audio in bluray has an stype of 0x85
            // which conflicts with ATSC Program ID
            // To distinguish, Bluray streams have a reg_desc of HDMV
            // This is an interleaved DTS-HD HRA/DTS stream and the 
            // esid of the DTS is 0x71
            stream->ts[i].stream_kind = A;
            stream->ts[i].substream_type[0] = HB_SUBSTREAM_BD_DTS;
            add_audio(i, list_audio, stream, HB_SUBSTREAM_BD_DTS, 
                      HB_ACODEC_DCA, 0 );
            stream->ts[i].substream_type[1] = 0;
            add_audio(i, list_audio, stream, 0, HB_ACODEC_MPGA, CODEC_ID_DTS );
            stream->ts[i].number_substreams = 2;
            continue;
        }
        if ( stream->ts[i].stream_type == 0x84 && 
             stream->pmt_info.reg_desc == STR4_TO_UINT32("HDMV") )
        {
            // EAC3 audio in bluray has an stype of 0x84
            // which conflicts with SDDS
            // To distinguish, Bluray streams have a reg_desc of HDMV
            stream->ts[i].stream_kind = A;
            add_audio(i, list_audio, stream, 0, HB_ACODEC_MPGA, CODEC_ID_EAC3 );
            continue;
        }

        if ( stream->ts[i].stream_kind == A )
        {
            add_audio(i, list_audio, stream, 0, 
                      st2codec[stream->ts[i].stream_type].codec, 
                      st2codec[stream->ts[i].stream_type].codec_param );
            continue;
        }

        if ( stream->ts[i].stream_kind != U )
        {
            // Not audio
            continue;
        }

        // If we get here, we have a stream type that we are not yet
        // sure about.  Probe some data to see if it looks like
        // a known stream type.
        fseeko(stream->file_handle, 0, SEEK_SET);
        align_to_next_packet(stream);

        buf = hb_ts_stream_getPEStype(stream, stream->ts[i].pid);

        /* check that we found a PES header */
        if (buf && buf[0] == 0x00 && buf[1] == 0x00 && buf[2] == 0x01)
        {
            // 0xbd ("private stream 1") is the normal container for non-ISO
            // media - AC3/DCA/PCM/etc.
            if ( buf[3] == 0xbd )
            {
                // XXX assume unknown stream types are AC-3 (if they're not
                // audio we'll find that out during the scan but if they're
                // some other type of audio we'll end up ignoring them).
                add_audio(i, list_audio, stream, 0, HB_ACODEC_AC3, 0 );
            }
            else if ((buf[3] & 0xe0) == 0xc0)
            {
                // XXX assume unknown stream types are MPEG audio
                // 0xC0 - 0xCF are the normal containers for ISO-standard
                // media (mpeg2 audio and mpeg4 AAC).
                add_audio(i, list_audio, stream, 0, 
                          HB_ACODEC_MPGA, CODEC_ID_MP2 );
            }
            else
            {
                hb_log("transport stream pid 0x%x (type 0x%x, substream 0x%x) "
                        "isn't audio", stream->ts[i].pid,
                        stream->ts[i].stream_type, buf[3]);
            }
        }
        else
        {
            hb_log("transport stream pid 0x%x (type 0x%x) isn't audio",
                    stream->ts[i].pid,
                    stream->ts[i].stream_type);
        }

        fseeko(stream->file_handle, cur_pos, SEEK_SET);
    }
}

static void add_audio_to_title(hb_title_t *title, int id)
{
    hb_audio_t *audio = calloc( sizeof( hb_audio_t ), 1 );

    audio->id = id;
    switch ( id >> 12 )
    {
        case 0x0:
            audio->config.in.codec = HB_ACODEC_MPGA;
            hb_log("add_audio_to_title: added MPEG audio stream 0x%x", id);
            break;
        case 0x2:
            // type 2 is a DVD subtitle stream - just ignore it */
            free( audio );
            return;
        case 0x8:
            audio->config.in.codec = HB_ACODEC_AC3;
            hb_log("add_audio_to_title: added AC3 audio stream 0x%x", id);
            break;
        case 0xa:
            audio->config.in.codec = HB_ACODEC_LPCM;
            hb_log("add_audio_to_title: added LPCM audio stream 0x%x", id);
            break;
        default:
            hb_log("add_audio_to_title: unknown audio stream type 0x%x", id);
            free( audio );
            return;

    }
    set_audio_description( audio, lang_for_code( 0 ) );

    // Sort by id when adding to the list
    int i;
    for ( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        hb_audio_t *tmp = hb_list_item( title->list_audio, i );
        if ( audio->id < tmp->id )
            break;
    }
    hb_list_insert( title->list_audio, i, audio );
}

static void hb_ps_stream_find_audio_ids(hb_stream_t *stream, hb_title_t *title)
{
    off_t cur_pos = ftello(stream->file_handle);
    hb_buffer_t *buf;
    hb_list_t *list = hb_list_init();
    // how many blocks we read while searching for audio streams
    int blksleft = 4096;
    // there can be at most 16 unique streams in an MPEG PS (8 in a DVD)
    // so we use a bitmap to keep track of the ones we've already seen.
    // Bit 'i' of smap is set if we've already added the audio for
    // audio substream id 'i' to the title's audio list.
    uint32_t smap = 0;

    // start looking 20% into the file since there's occasionally no
    // audio at the beginning (particularly for vobs).
    hb_stream_seek(stream, 0.2f);

    while (--blksleft >= 0 && (buf = hb_stream_read(stream)) != NULL)
    {
        hb_buffer_t *es;

        // 'buf' contains an MPEG2 PACK - get a list of all it's 
        // elementary streams
        hb_demux_ps( buf, list, 0 );

        while ( ( es = hb_list_item( list, 0 ) ) )
        {
            hb_list_rem( list, es );
            if ( (es->id & 0xff) == 0xbd || (es->id & 0xe0) == 0xc0 )
            {
                // this PES contains some kind of audio - get the substream id
                // and check if we've seen it already.
                int ssid = (es->id > 0xff ? es->id >> 8 : es->id) & 0xf;
                if ( (smap & (1 << ssid)) == 0 )
                {
                    // we haven't seen this stream before - add it to the
                    // title's list of audio streams.
                    smap |= (1 << ssid);
                    add_audio_to_title(title, es->id);
                }
            }
            hb_buffer_close( &es );
        }
    }
    hb_list_empty( &list );
    fseeko(stream->file_handle, cur_pos, SEEK_SET);
}

/***********************************************************************
 * hb_ts_stream_init
 ***********************************************************************
 *
 **********************************************************************/

static void hb_ts_stream_init(hb_stream_t *stream)
{
    int i;

    for (i=0; i < kMaxNumberDecodeStreams; i++)
    {
        stream->ts[i].continuity = -1;
        stream->ts[i].pid = -1;
        stream->ts[i].number_substreams = 1;
        stream->ts[i].substream_type[0] = 0;
    }
    stream->ts_packet = malloc( stream->packetsize );

    // Find the audio and video pids in the stream
    hb_ts_stream_find_pids(stream);

    for (i = 0; i < stream->ts_number_pids; i++)
    {
        // demuxing buffer for TS to PS conversion
        stream->ts[i].buf = hb_buffer_init(stream->packetsize);
        stream->ts[i].extra_buf = hb_buffer_init(stream->packetsize);
        stream->ts[i].buf->size = 0;
        stream->ts[i].extra_buf->size = 0;
    }
}

#define MAX_HOLE 208*80

static off_t align_to_next_packet(hb_stream_t *stream)
{
    uint8_t buf[MAX_HOLE];
    off_t pos = 0;
    off_t start = ftello(stream->file_handle);
    off_t orig;

    if ( start >= stream->packetsize ) {
        start -= stream->packetsize;
        fseeko(stream->file_handle, start, SEEK_SET);
    }
    orig = start;

    while (1)
    {
        if (fread(buf, sizeof(buf), 1, stream->file_handle) == 1)
        {
            const uint8_t *bp = buf;
            int i;

            for ( i = sizeof(buf) - 8 * stream->packetsize; --i >= 0; ++bp )
            {
                if ( have_ts_sync( bp, stream->packetsize ) )
                {
                    break;
                }
            }
            if ( i >= 0 )
            {
                pos = ( bp - buf ) - stream->packetsize + 188;
                break;
            }
            fseeko(stream->file_handle, -8 * stream->packetsize, SEEK_CUR);
            start = ftello(stream->file_handle);
        }
        else
        {
            return 0;
        }
    }
    fseeko(stream->file_handle, start+pos, SEEK_SET);
    return start - orig + pos;
}


typedef struct {
    uint8_t *buf;
    uint32_t val;
    int pos;
} bitbuf_t;

static const unsigned int bitmask[] = {
    0x0,0x1,0x3,0x7,0xf,0x1f,0x3f,0x7f,0xff,
    0x1ff,0x3ff,0x7ff,0xfff,0x1fff,0x3fff,0x7fff,0xffff,
    0x1ffff,0x3ffff,0x7ffff,0xfffff,0x1fffff,0x3fffff,0x7fffff,0xffffff,
    0x1ffffff,0x3ffffff,0x7ffffff,0xfffffff,0x1fffffff,0x3fffffff,0x7fffffff,0xffffffff};

static inline void set_buf(bitbuf_t *bb, uint8_t* buf, int bufsize, int clear)
{
    bb->pos = 0;
    bb->buf = buf;
    bb->val = (bb->buf[0] << 24) | (bb->buf[1] << 16) |
              (bb->buf[2] << 8) | bb->buf[3];
    if (clear)
        memset(bb->buf, 0, bufsize);
}

static inline int buf_size(bitbuf_t *bb)
{
    return bb->pos >> 3;
}

static inline unsigned int get_bits(bitbuf_t *bb, int bits)
{
    unsigned int val;
    int left = 32 - (bb->pos & 31);

    if (bits < left)
    {
        val = (bb->val >> (left - bits)) & bitmask[bits];
        bb->pos += bits;
    }
    else
    {
        val = (bb->val & bitmask[left]) << (bits - left);
        bb->pos += left;
        bits -= left;

        int pos = bb->pos >> 3;
        bb->val = (bb->buf[pos] << 24) | (bb->buf[pos + 1] << 16) | (bb->buf[pos + 2] << 8) | bb->buf[pos + 3];

        if (bits > 0)
        {
            val |= (bb->val >> (32 - bits)) & bitmask[bits];
            bb->pos += bits;
        }
    }

    return val;
}

// extract what useful information we can from the elementary stream
// descriptor list at 'dp' and add it to the stream at 'esindx'.
// Descriptors with info we don't currently use are ignored.
// The descriptor list & descriptor item formats are defined in
// ISO 13818-1 (2000E) section 2.6 (pg. 62).
static void decode_element_descriptors(hb_stream_t* stream, int esindx,
                                       const uint8_t *dp, uint8_t dlen)
{
    const uint8_t *ep = dp + dlen;

    while (dp < ep)
    {
        switch (dp[0])
        {
            case 5:    // Registration descriptor
                stream->ts[esindx].format_id = (dp[2] << 24) | (dp[3] << 16) |
                                               (dp[4] << 8)  | dp[5];
                break;

            case 10:    // ISO_639_language descriptor
                stream->ts[esindx].a52_info.lang_code = lang_to_code(lang_for_code2((const char *)&dp[2]));
                break;

            case 0x6a:  // DVB AC-3 descriptor
                stream->ts[esindx].stream_type = 0x81;
                break;

            case 0x7a:  // DVB EAC-3 descriptor
                stream->ts[esindx].stream_type = 0x87;
                break;

            default:
                break;
        }
        dp += dp[1] + 2;
    }
}

int decode_program_map(hb_stream_t* stream)
{
    bitbuf_t bb;
    set_buf(&bb, stream->pmt_info.tablebuf, stream->pmt_info.tablepos, 0);

    get_bits(&bb, 8);  // table_id
    get_bits(&bb, 4);
    unsigned int section_length = get_bits(&bb, 12);
    stream->pmt_info.section_length = section_length;

    unsigned int program_number = get_bits(&bb, 16);
    stream->pmt_info.program_number = program_number;
    get_bits(&bb, 2);
    get_bits(&bb, 5);  // version_number
    get_bits(&bb, 1);
    get_bits(&bb, 8);  // section_number
    get_bits(&bb, 8);  // last_section_number
    get_bits(&bb, 3);
    stream->pmt_info.PCR_PID = get_bits(&bb, 13);
    get_bits(&bb, 4);
    int program_info_length = get_bits(&bb, 12);
    stream->pmt_info.program_info_length = program_info_length;

    int i;
    for (i = 0; i < program_info_length - 2; )
    {
        uint8_t tag, len;
        tag = get_bits(&bb, 8);
        len = get_bits(&bb, 8);
        i += 2;
        if ( i + len > program_info_length )
        {
            break;
        }
        if (tag == 0x05 && len >= 4)
        {
            // registration descriptor
            stream->pmt_info.reg_desc = get_bits(&bb, 32);
            i += 4;
            len -= 4;
        }
        int j;
        for ( j = 0; j < len; j++ )
        {
            get_bits(&bb, 8);
        }
        i += len;
    }
    for ( ; i < program_info_length; i++ )
    {
        get_bits(&bb, 8);
    }

    int cur_pos =  9 /* data after the section length field*/ + program_info_length;
    int done_reading_stream_types = 0;
    while (!done_reading_stream_types)
    {
        unsigned char stream_type = get_bits(&bb, 8);
        get_bits(&bb, 3);
        unsigned int elementary_PID = get_bits(&bb, 13);
        get_bits(&bb, 4);
        unsigned int ES_info_length = get_bits(&bb, 12);

        int i=0;
        unsigned char *ES_info_buf = (unsigned char *) malloc(ES_info_length);
        for (i=0; i < ES_info_length; i++)
        {
            ES_info_buf[i] = get_bits(&bb, 8);
        }

        if ( index_of_pid( stream, elementary_PID ) < 0 )
        {
            // Defined audio stream types are 0x81 for AC-3/A52 audio 
            // and 0x03 for mpeg audio. But content producers seem to 
            // use other values (0x04 and 0x06 have both been observed) 
            // so at this point we say everything that isn't a video 
            // pid is audio then at the end of hb_stream_title_scan 
            // we'll figure out which are really audio by looking at 
            // the PES headers.
            i = stream->ts_number_pids;
            if (i < kMaxNumberDecodeStreams)
            {
                stream->ts[i].pid = elementary_PID;
                stream->ts[i].stream_type = stream_type;
                stream->ts[i].number_substreams = 1;
                stream->ts[i].substream_type[0] = 0;
                if (ES_info_length > 0)
                {
                    decode_element_descriptors(stream, i, ES_info_buf,
                                            ES_info_length);
                }
                stream->ts[i].stream_kind = ts_stream_kind(stream, i);
                ++stream->ts_number_pids;
            }
        }

        cur_pos += 5 /* stream header */ + ES_info_length;

        free(ES_info_buf);

        if (cur_pos >= section_length - 4 /* stop before the CRC */)
            done_reading_stream_types = 1;
    }

    return 1;
}

static int build_program_map(const uint8_t *buf, hb_stream_t *stream)
{
    // Get adaption header info
    int adapt_len = 0;
    int adaption = (buf[3] & 0x30) >> 4;
    if (adaption == 0)
            return 0;
    else if (adaption == 0x2)
            adapt_len = 184;
    else if (adaption == 0x3)
            adapt_len = buf[4] + 1;
    if (adapt_len > 184)
            return 0;

    // Get payload start indicator
    int start;
    start = (buf[1] & 0x40) != 0;

    // Get pointer length - only valid in packets with a start flag
    int pointer_len = 0;

    if (start)
    {
        pointer_len = buf[4 + adapt_len] + 1;
        stream->pmt_info.tablepos = 0;
    }
    // Get Continuity Counter
    int continuity_counter = buf[3] & 0x0f;
    if (!start && (stream->pmt_info.current_continuity_counter + 1 != continuity_counter))
    {
        hb_log("build_program_map - Continuity Counter %d out of sequence - expected %d", continuity_counter, stream->pmt_info.current_continuity_counter+1);
        return 0;
    }
    stream->pmt_info.current_continuity_counter = continuity_counter;
    stream->pmt_info.reading |= start;

    // Add the payload for this packet to the current buffer
    int amount_to_copy = 184 - adapt_len - pointer_len;
    if (stream->pmt_info.reading && (amount_to_copy > 0))
    {
            stream->pmt_info.tablebuf = realloc(stream->pmt_info.tablebuf, stream->pmt_info.tablepos + amount_to_copy);

            memcpy(stream->pmt_info.tablebuf + stream->pmt_info.tablepos, buf + 4 + adapt_len + pointer_len, amount_to_copy);
            stream->pmt_info.tablepos += amount_to_copy;
    }
    if (stream->pmt_info.tablepos > 3)
    {
        // We have enough to check the section length
        int length;
        length = ((stream->pmt_info.tablebuf[1] << 8) + 
                  stream->pmt_info.tablebuf[2]) & 0xFFF;
        if (stream->pmt_info.tablepos > length + 1)
        {
            // We just finished a bunch of packets - parse the program map details
            int decode_ok = 0;
            if (stream->pmt_info.tablebuf[0] == 0x02)
                decode_ok = decode_program_map(stream);
            free(stream->pmt_info.tablebuf);
            stream->pmt_info.tablebuf = NULL;
            stream->pmt_info.tablepos = 0;
            stream->pmt_info.reading = 0;
            if (decode_ok)
                return decode_ok;
        }

    }

    return 0;
}

static int decode_PAT(const uint8_t *buf, hb_stream_t *stream)
{
    unsigned char tablebuf[1024];
    unsigned int tablepos = 0;

    int reading = 0;


    // Get adaption header info
    int adapt_len = 0;
    int adaption = (buf[3] & 0x30) >> 4;
    if (adaption == 0)
            return 0;
    else if (adaption == 0x2)
            adapt_len = 184;
    else if (adaption == 0x3)
            adapt_len = buf[4] + 1;
    if (adapt_len > 184)
            return 0;

    // Get pointer length
    int pointer_len = buf[4 + adapt_len] + 1;

    // Get payload start indicator
    int start;
    start = (buf[1] & 0x40) != 0;

    if (start)
            reading = 1;

    // Add the payload for this packet to the current buffer
    if (reading && (184 - adapt_len) > 0)
    {
            if (tablepos + 184 - adapt_len - pointer_len > 1024)
            {
                    hb_log("decode_PAT - Bad program section length (> 1024)");
                    return 0;
            }
            memcpy(tablebuf + tablepos, buf + 4 + adapt_len + pointer_len, 184 - adapt_len - pointer_len);
            tablepos += 184 - adapt_len - pointer_len;
    }

    if (start && reading)
    {
            memcpy(tablebuf + tablepos, buf + 4 + adapt_len + 1, pointer_len - 1);


            unsigned int pos = 0;
            //while (pos < tablepos)
            {
                    bitbuf_t bb;
                    set_buf(&bb, tablebuf + pos, tablepos - pos, 0);

                    unsigned char section_id    = get_bits(&bb, 8);
                    get_bits(&bb, 4);
                    unsigned int section_len    = get_bits(&bb, 12);
                    get_bits(&bb, 16); // transport_id
                    get_bits(&bb, 2);
                    get_bits(&bb, 5);  // version_num
                    get_bits(&bb, 1);  // current_next
                    get_bits(&bb, 8);  // section_num
                    get_bits(&bb, 8);  // last_section

                    switch (section_id)
                    {
                      case 0x00:
                        {
                          // Program Association Section
                          section_len -= 5;    // Already read transport stream ID, version num, section num, and last section num
                          section_len -= 4;   // Ignore the CRC
                          int curr_pos = 0;
                          stream->ts_number_pat_entries = 0;
                          while ((curr_pos < section_len) && (stream->ts_number_pat_entries < kMaxNumberPMTStreams))
                          {
                            unsigned int pkt_program_num = get_bits(&bb, 16);
                            stream->pat_info[stream->ts_number_pat_entries].program_number = pkt_program_num;

                            get_bits(&bb, 3);  // Reserved
                            if (pkt_program_num == 0)
                            {
                              get_bits(&bb, 13); // pkt_network_id
                            }
                            else
                            {
                              unsigned int pkt_program_map_PID = get_bits(&bb, 13);
                                stream->pat_info[stream->ts_number_pat_entries].program_map_PID = pkt_program_map_PID;
                            }
                            curr_pos += 4;
                            stream->ts_number_pat_entries++;
                          }
                        }
                        break;
                      case 0xC7:
                            {
                                    break;
                            }
                      case 0xC8:
                            {
                                    break;
                            }
                    }

                    pos += 3 + section_len;
            }

            tablepos = 0;
    }
    return 1;
}

static void hb_ts_stream_find_pids(hb_stream_t *stream)
{
    // To be different from every other broadcaster in the world, New Zealand TV
    // changes PMTs (and thus video & audio PIDs) when 'programs' change. Since
    // we may have the tail of the previous program at the beginning of this
    // file, take our PMT from the middle of the file.
    fseeko(stream->file_handle, 0, SEEK_END);
    uint64_t fsize = ftello(stream->file_handle);
    fseeko(stream->file_handle, fsize >> 1, SEEK_SET);
    align_to_next_packet(stream);

    // Read the Transport Stream Packets (188 bytes each) looking at first for PID 0 (the PAT PID), then decode that
    // to find the program map PID and then decode that to get the list of audio and video PIDs

    for (;;)
    {
        const uint8_t *buf = next_packet( stream );

        if ( buf == NULL )
        {
            hb_log("hb_ts_stream_find_pids - end of file");
            break;
        }

        // Get pid
        int pid = (((buf[1] & 0x1F) << 8) | buf[2]) & 0x1FFF;

        if ((pid == 0x0000) && (stream->ts_number_pat_entries == 0))
        {
          decode_PAT(buf, stream);
          continue;
        }

        int pat_index = 0;
        for (pat_index = 0; pat_index < stream->ts_number_pat_entries; pat_index++)
        {
            // There are some streams where the PAT table has multiple entries as if their are
            // multiple programs in the same transport stream, and yet there's actually only one
            // program really in the stream. This seems to be true for transport streams that
            // originate in the HDHomeRun but have been output by EyeTV's export utility. What I think
            // is happening is that the HDHomeRun is sending the entire transport stream as broadcast,
            // but the EyeTV is only recording a single (selected) program number and not rewriting the
            // PAT info on export to match what's actually on the stream.
            // Until we have a way of handling multiple programs per transport stream elegantly we'll match
            // on the first pat entry for which we find a matching program map PID.  The ideal solution would
            // be to build a title choice popup from the PAT program number details and then select from
            // their - but right now the API's not capable of that.
            if (stream->pat_info[pat_index].program_number != 0 &&
                pid == stream->pat_info[pat_index].program_map_PID)
            {
              if (build_program_map(buf, stream) > 0)
                break;
            }
        }
        // Keep going  until we have a complete set of PIDs
        if ( index_of_video( stream ) >= 0 )
          break;
    }

    hb_log("hb_ts_stream_find_pids - found the following PIDS");
    hb_log("    Video PIDS : ");
    int i;
    for (i=0; i < stream->ts_number_pids; i++)
    {
        if ( stream->ts[i].stream_kind == V )
        {
            hb_log( "      0x%x type %s (0x%x)", 
                    stream->ts[i].pid,
                    stream_type_name(stream->pmt_info.reg_desc,
                                     stream->ts[i].stream_type),
                    stream->ts[i].stream_type);
        }
    }
    hb_log("    Audio PIDS : ");
    for (i = 0; i < stream->ts_number_pids; i++)
    {
        if ( stream->ts[i].stream_kind != V )
        {
            hb_log( "      0x%x type %s (0x%x)", 
                    stream->ts[i].pid,
                    stream_type_name(stream->pmt_info.reg_desc, 
                                     stream->ts[i].stream_type),
                    stream->ts[i].stream_type );
        }
    }
 }


static void fwrite64( hb_buffer_t * buf, void *data, int len )
{
    if ( len > 0 )
    {
        int pos = buf->size;
        if ( pos + len > buf->alloc )
        {
            int size = MAX(buf->alloc * 2, pos + len);
            hb_buffer_realloc(buf, size);
        }
        memcpy( &(buf->data[pos]), data, len );
        buf->size += len;
    }
}

// convert a PES PTS or DTS to an int64
static int64_t pes_timestamp( const uint8_t *pes )
{
    int64_t ts = ( (uint64_t)(pes[0] & 0xe ) << 29 );
    ts |= ( pes[1] << 22 ) | ( ( pes[2] >> 1 ) << 15 ) |
          ( pes[3] << 7 ) | ( pes[4] >> 1 );
    return ts;
}

static hb_buffer_t * generate_output_data(hb_stream_t *stream, int curstream)
{
    hb_buffer_t *buf = NULL, *first = NULL;
    int substream, i;

    for ( i = 0; i < stream->ts[curstream].number_substreams; i++ )
    {
        substream = stream->ts[curstream].substream_type[i];
        if ( substream == 0 ||
             substream == stream->ts[curstream].current_substream )
        {
            uint8_t *tdat = stream->ts[curstream].buf->data;
            int hlen = tdat[8] + 9;
            int size = stream->ts[curstream].buf->size - hlen;

            if ( size <= 0 )
                continue;

            if ( first == NULL )
            {
                first = buf = hb_buffer_init( size );
            }
            else
            {
                hb_buffer_t * tmp;
                tmp = hb_buffer_init( size );
                buf->next = tmp;
                buf = tmp;
            }
            buf->size = 0;

            buf->id = (substream << 16) | stream->ts[curstream].pid;
            switch (stream->ts[curstream].stream_kind)
            {
                case A:
                    buf->type = AUDIO_BUF;
                    break;

                case V:
                    buf->type = VIDEO_BUF;
                    break;

                default:
                    buf->type = OTHER_BUF;
                    break;
            }

            // check if this packet was referenced to an older pcr and if that
            // pcr was significantly different than the one we're using now.
            // (the reason for the uint cast on the pcr difference is that the
            // difference is significant if it advanced by more than 200ms or if
            // it went backwards by any amount. The negative numbers look like huge
            // unsigned ints so the cast allows both conditions to be checked at once.
            int64_t bufpcr = stream->ts[curstream].buf->pcr;
            int64_t curpcr = stream->ts_pcr_current;
            if ( stream->ts[curstream].buf->cur < stream->ts_pcr_out &&
                 bufpcr != -1 && curpcr != -1 && curpcr - bufpcr > 200*90LL )
            {
                // we've sent up a new pcr but have a packet referenced to an
                // old pcr and the difference was enough to trigger a discontinuity
                // correction. smash the timestamps or we'll mess up the correction.
                buf->start = -1;
                buf->renderOffset = -1;
                buf->stop = -1;
                buf->pcr = -1;
            }
            else
            {
                if ( stream->ts_pcr_out != stream->ts_pcr_in )
                {
                    // we have a new pcr
                    stream->ts_pcr_out = stream->ts_pcr_in;
                    buf->pcr = stream->ts_pcr;
                    stream->ts_pcr_current = stream->ts_pcr;
                }
                else
                {
                    buf->pcr = -1;
                }

                // put the PTS & possible DTS into 'start' & 'renderOffset' then strip
                // off the PES header.
                if ( tdat[7] & 0xc0 )
                {
                    buf->start = pes_timestamp( tdat + 9 );
                    buf->renderOffset = ( tdat[7] & 0x40 )? pes_timestamp( tdat + 14 ) :
                                                            buf->start;
                }
                else
                {
                    buf->start = -1;
                    buf->renderOffset = -1;
                }
            }

            fwrite64( buf, tdat + hlen, size );
        }
    }
    stream->ts[curstream].buf->size = 0;
    return first;
}

static void hb_ts_stream_append_pkt(hb_stream_t *stream, int idx, const uint8_t *buf, int len)
{
    if (stream->ts[idx].buf->size + len > stream->ts[idx].buf->alloc)
    {
        int size;

        size = MAX(stream->ts[idx].buf->alloc * 2, stream->ts[idx].buf->size + len);
        hb_buffer_realloc(stream->ts[idx].buf, size);
    }
    memcpy(stream->ts[idx].buf->data + stream->ts[idx].buf->size, buf, len);
    stream->ts[idx].buf->size += len;
}

/***********************************************************************
 * hb_ts_stream_decode
 ***********************************************************************
 *
 **********************************************************************/
hb_buffer_t * hb_ts_decode_pkt( hb_stream_t *stream, const uint8_t * pkt )
{
    /*
     * stash the output buffer pointer in our stream so we don't have to
     * pass it & its original value to everything we call.
     */
    int video_index = index_of_video(stream);
    int curstream;
    hb_buffer_t *buf;

    /* This next section validates the packet */

    // Get pid and use it to find stream state.
    int pid = ((pkt[1] & 0x1F) << 8) | pkt[2];
    if ( ( curstream = index_of_pid( stream, pid ) ) < 0 )
    {
        return NULL;
    }

    // Get error
    int errorbit = (pkt[1] & 0x80) != 0;
    if (errorbit)
    {
        ts_err( stream, curstream,  "packet error bit set");
        return NULL;
    }

    // Get adaption header info
    int adaption = (pkt[3] & 0x30) >> 4;
    int adapt_len = 0;
    if (adaption == 0)
    {
        ts_err( stream, curstream,  "adaptation code 0");
        return NULL;
    }
    else if (adaption == 0x2)
        adapt_len = 184;
    else if (adaption == 0x3)
    {
        adapt_len = pkt[4] + 1;
        if (adapt_len > 184)
        {
            ts_err( stream, curstream,  "invalid adapt len %d", adapt_len);
            return NULL;
        }
    }

    if ( adapt_len > 0 )
    {
        if ( pkt[5] & 0x40 )
        {
            // found a random access point
        }
        // if there's an adaptation header & PCR_flag is set
        // get the PCR (Program Clock Reference)
        if ( adapt_len > 7 && ( pkt[5] & 0x10 ) != 0 )
        {
            stream->ts_pcr = ( (uint64_t)pkt[6] << (33 - 8) ) |
                             ( (uint64_t)pkt[7] << (33 - 16) ) |
                             ( (uint64_t)pkt[8] << (33 - 24) ) |
                             ( (uint64_t)pkt[9] << (33 - 32) ) |
                             ( pkt[10] >> 7 );
            ++stream->ts_pcr_in;
            stream->ts_found_pcr = 1;
            stream->ts_flags |= TS_HAS_PCR;
        }
    }

    // If we don't have a PCR yet but the stream has PCRs just loop
    // so we don't process anything until we have a clock reference.
    // Unfortunately the HD Home Run appears to null out the PCR so if
    // we didn't detect a PCR during scan keep going and we'll use
    // the video stream DTS for the PCR.
    if ( !stream->ts_found_pcr && ( stream->ts_flags & TS_HAS_PCR ) )
    {
        return NULL;
    }

    // Get continuity
    // Continuity only increments for adaption values of 0x3 or 0x01
    // and is not checked for start packets.

    int start = (pkt[1] & 0x40) != 0;

    if ( (adaption & 0x01) != 0 )
    {
        int continuity = (pkt[3] & 0xF);
        if ( continuity == stream->ts[curstream].continuity )
        {
            // Spliced transport streams can have duplicate 
            // continuity counts at the splice boundary.
            // Test to see if the packet is really a duplicate
            // by comparing packet summaries to see if they
            // match.
            uint8_t summary[8];

            summary[0] = adaption;
            summary[1] = adapt_len;
            if (adapt_len + 4 + 6 + 9 <= 188)
            {
                memcpy(&summary[2], pkt+4+adapt_len+9, 6);
            }
            else
            {
                memset(&summary[2], 0, 6);
            }
            if ( memcmp( summary, stream->ts[curstream].pkt_summary, 8 ) == 0 )
            {
                // we got a duplicate packet (usually used to introduce
                // a PCR when one is needed). The only thing that can
                // change in the dup is the PCR which we grabbed above
                // so ignore the rest.
                return NULL;
            }
        }
        if ( !start && (stream->ts[curstream].continuity != -1) &&
             !stream->ts[curstream].skipbad &&
             (continuity != ( (stream->ts[curstream].continuity + 1) & 0xf ) ) )
        {
            ts_err( stream, curstream, "continuity error: got %d expected %d",
                    (int)continuity,
                    (stream->ts[curstream].continuity + 1) & 0xf );
            stream->ts[curstream].continuity = continuity;
            return NULL;
        }
        stream->ts[curstream].continuity = continuity;

        // Save a summary of this packet for later duplicate
        // testing.  The summary includes some header information
        // and payload bytes.  Should be enough to detect 
        // non-duplicates.
        stream->ts[curstream].pkt_summary[0] = adaption;
        stream->ts[curstream].pkt_summary[1] = adapt_len;
        if (adapt_len + 4 + 6 + 9 <= 188)
        {
            memcpy(&stream->ts[curstream].pkt_summary[2], 
                    pkt+4+adapt_len+9, 6);
        }
        else
        {
            memset(&stream->ts[curstream].pkt_summary[2], 0, 6);
        }
    }

    /* If we get here the packet is valid - process its data */


    if ( start )
    {
        // Found a random access point (now we can start a frame/audio packet..)

        // PES must begin with an mpeg start code
        const uint8_t *pes = pkt + adapt_len + 4;
        if ( pes[0] != 0x00 || pes[1] != 0x00 || pes[2] != 0x01 )
        {
            ts_err( stream, curstream, "missing start code" );
            stream->ts[curstream].skipbad = 1;
            return NULL;
        }

        if ( stream->need_keyframe )
        {
            // we're looking for the first video frame because we're
            // doing random access during 'scan'
            if ( curstream != video_index || !isIframe( stream, pkt, adapt_len ) )
            {
                // not the video stream or didn't find an I frame
                // but we'll only wait 255 video frames for an I frame.
                if ( curstream != video_index || ++stream->need_keyframe )
                {
                    return NULL;
                }
            }
            stream->need_keyframe = 0;
        }

        // If we were skipping a bad packet, start fresh on this new PES packet..
        if (stream->ts[curstream].skipbad == 1)
        {
            stream->ts[curstream].skipbad = 0;
        }

        if ( curstream == video_index )
        {
            ++stream->frames;

            // if we don't have a pcr yet use the dts from this frame
            if ( !stream->ts_found_pcr )
            {
                // PES must begin with an mpeg start code & contain
                // a DTS or PTS.
                const uint8_t *pes = pkt + adapt_len + 4;
                if ( pes[0] != 0x00 || pes[1] != 0x00 || pes[2] != 0x01 ||
                     ( pes[7] >> 6 ) == 0 )
                {
                    return NULL;
                }
                // if we have a dts use it otherwise use the pts
                stream->ts_pcr = pes_timestamp( pes + ( pes[7] & 0x40?14:9 ) );
                ++stream->ts_pcr_in;
            }
        }

        // the last byte of the header is the extension id. see if
        // this could be a multiplexed stream
        int substream = 0;
        if ( pes[3] == 0xfd )
        {
            int out_substream = 0;
            substream = pes[pes[8]+8];
            out_substream = find_substream( &stream->ts[curstream], substream );
            if ( out_substream < 0 )
            {
                buf = NULL;
                if ( stream->ts[curstream].buf->size )
                    buf = generate_output_data(stream, curstream);

                stream->ts[curstream].skipbad = 1;
                return buf;
            }
        }

        // If we have some data already on this stream, turn it into
        // a program stream packet. Then add the payload for this
        // packet to the current pid's buffer.
        if ( stream->ts[curstream].buf->size )
        {
            // we have to ship the old packet before updating the pcr
            // since the packet we've been accumulating is referenced
            // to the old pcr.
            buf = generate_output_data(stream, curstream);

            if ( buf )
            {
                // Output data is ready.
                stream->ts[curstream].current_substream = substream;

                // remember the pcr that was in effect when we started
                // this packet.
                stream->ts[curstream].buf->cur = stream->ts_pcr_in;
                stream->ts[curstream].buf->pcr = stream->ts_pcr;
                hb_ts_stream_append_pkt(stream, curstream, pkt + 4 + adapt_len,
                                        184 - adapt_len);
                return buf;
            }
        }
        // remember the pcr that was in effect when we started this packet.
        stream->ts[curstream].buf->cur = stream->ts_pcr_in;
        stream->ts[curstream].buf->pcr = stream->ts_pcr;
        stream->ts[curstream].current_substream = substream;
    }

    // Add the payload for this packet to the current buffer
    if (!stream->ts[curstream].skipbad && (184 - adapt_len) > 0)
    {
        hb_ts_stream_append_pkt(stream, curstream, pkt + 4 + adapt_len,
                                184 - adapt_len);
        // see if we've hit the end of this PES packet
        const uint8_t *pes = stream->ts[curstream].buf->data;
        int len = ( pes[4] << 8 ) + pes[5] + 6;
        if ( len > 6 && stream->ts[curstream].buf->size == len &&
             pes[0] == 0x00 && pes[1] == 0x00 && pes[2] == 0x01 )
        {
            buf = generate_output_data(stream, curstream);
            if ( buf )
                return buf;
        }
    }
    return NULL;
}

static hb_buffer_t * hb_ts_stream_decode( hb_stream_t *stream )
{
    hb_buffer_t * b;

    // spin until we get a packet of data from some stream or hit eof
    while ( 1 )
    {
        const uint8_t *buf = next_packet(stream);
        if ( buf == NULL )
        {
            // end of file - we didn't finish filling our ps write buffer
            // so just discard the remainder (the partial buffer is useless)
            hb_log("hb_ts_stream_decode - eof");
            return NULL;
        }

        b = hb_ts_decode_pkt( stream, buf );
        if ( b )
        {
            return b;
        }
    }
    return NULL;
}

static void hb_ts_stream_reset(hb_stream_t *stream)
{
    int i;

    for (i=0; i < kMaxNumberDecodeStreams; i++)
    {
        if ( stream->ts[i].buf )
            stream->ts[i].buf->size = 0;
        if ( stream->ts[i].extra_buf )
            stream->ts[i].extra_buf->size = 0;
        stream->ts[i].skipbad = 1;
        stream->ts[i].continuity = -1;
    }

    stream->need_keyframe = 0;

    stream->ts_found_pcr = 0;
    stream->ts_pcr_out = 0;
    stream->ts_pcr_in = 0;
    stream->ts_pcr = -1;
    stream->ts_pcr_current = -1;

    stream->frames = 0;
    stream->errors = 0;
    stream->last_error_frame = -10000;
    stream->last_error_count = 0;

    align_to_next_packet(stream);
}

// ------------------------------------------------------------------
// Support for reading media files via the ffmpeg libraries.

static void ffmpeg_add_codec( hb_stream_t *stream, int stream_index )
{
    // add a codec to the context here so it will be there when we
    // read the first packet.
    AVCodecContext *context = stream->ffmpeg_ic->streams[stream_index]->codec;
    context->workaround_bugs = FF_BUG_AUTODETECT;
    context->error_recognition = 1;
    context->error_concealment = FF_EC_GUESS_MVS|FF_EC_DEBLOCK;
    AVCodec *codec = avcodec_find_decoder( context->codec_id );
    hb_ff_set_sample_fmt( context, codec );
    hb_avcodec_open( context, codec );
}

// The ffmpeg stream reader / parser shares a lot of state with the 
// decoder via a codec context kept in the AVStream of the reader's
// AVFormatContext. Since decoding is done in a different thread we
// have to somehow pass this codec context to the decoder and we have
// to do it before the first packet is read (so we can't put the info
// in the buf we'll send downstream). Decoders don't have any way to
// get to the stream directly (they're not passed the title or job
// pointers during a scan) so this is a back door for the decoder to
// get the codec context. We just stick the stream pointer in the next
// slot an array of pointers maintained as a circular list then return
// the index into the list combined with the ffmpeg stream index as the
// codec_param that will be passed to the decoder init routine. We make
// the list 'big' (enough for 1024 simultaneously open ffmpeg streams)
// so that we don't have to do a complicated allocator or worry about
// deleting entries on close. 
//
// Entries can only be added to this list during a scan and are never
// deleted so the list access doesn't require locking.
static hb_stream_t **ffmpeg_streams;    // circular list of stream pointers
static int ffmpeg_stream_cur;           // where we put the last stream pointer
#define ffmpeg_sl_bits (10)             // log2 stream list size (in entries)
#define ffmpeg_sl_size (1 << ffmpeg_sl_bits)

// add a stream to the list & return the appropriate codec_param to access it
static int ffmpeg_codec_param( hb_stream_t *stream, int stream_index )
{
    if ( !ffmpeg_streams )
    {
        ffmpeg_streams = calloc( ffmpeg_sl_size, sizeof(stream) );
    }

    // the title scan adds all the ffmpeg media streams at once so we
    // only add a new entry to our stream list if the stream is different
    // than last time.
    int slot = ffmpeg_stream_cur;
    if ( ffmpeg_streams[slot] != stream )
    {
        // new stream - put it in the next slot of the stream list
        slot = ++ffmpeg_stream_cur & (ffmpeg_sl_size - 1);
        ffmpeg_streams[slot] = stream;
    }

    ffmpeg_add_codec( stream, stream_index );

    return ( stream_index << ffmpeg_sl_bits ) | slot;
}

// we're about to open 'title' to convert it - remap the stream associated
// with the video & audio codec params of the title to refer to 'stream'
// (the original scan stream was closed and no longer exists).
static void ffmpeg_remap_stream( hb_stream_t *stream, hb_title_t *title )
{
    // all the video & audio came from the same stream so remapping
    // the video's stream slot takes care of everything.
    int slot = title->video_codec_param & (ffmpeg_sl_size - 1);
    ffmpeg_streams[slot] = stream;

    // add codecs for all the streams used by the title
    ffmpeg_add_codec( stream, title->video_codec_param >> ffmpeg_sl_bits );

    int i;
    hb_audio_t *audio;
    for ( i = 0; ( audio = hb_list_item( title->list_audio, i ) ); ++i )
    {
        if ( audio->config.in.codec == HB_ACODEC_FFMPEG )
        {
            ffmpeg_add_codec( stream,
                              audio->config.in.codec_param >> ffmpeg_sl_bits );
        }
    }
}

void *hb_ffmpeg_context( int codec_param )
{
    if ( ffmpeg_streams == NULL )
        return NULL;

    int slot = codec_param & (ffmpeg_sl_size - 1);
    int stream_index = codec_param >> ffmpeg_sl_bits;
    return ffmpeg_streams[slot]->ffmpeg_ic->streams[stream_index]->codec;
}

void *hb_ffmpeg_avstream( int codec_param )
{
    if ( ffmpeg_streams == NULL )
        return NULL;

    int slot = codec_param & (ffmpeg_sl_size - 1);
    int stream_index = codec_param >> ffmpeg_sl_bits;
    return ffmpeg_streams[slot]->ffmpeg_ic->streams[stream_index];
}

static AVFormatContext *ffmpeg_deferred_close;

static int ffmpeg_open( hb_stream_t *stream, hb_title_t *title )
{
    if ( ffmpeg_deferred_close )
    {
        av_close_input_file( ffmpeg_deferred_close );
        ffmpeg_deferred_close = NULL;
    }
    AVFormatContext *ic;

    av_log_set_level( AV_LOG_ERROR );
    if ( av_open_input_file( &ic, stream->path, NULL, 0, NULL ) < 0 )
    {
        return 0;
    }
    if ( av_find_stream_info( ic ) < 0 )
        goto fail;

    stream->ffmpeg_ic = ic;
    stream->hb_stream_type = ffmpeg;
    stream->ffmpeg_pkt = malloc(sizeof(*stream->ffmpeg_pkt));
    av_init_packet( stream->ffmpeg_pkt );
    stream->chapter_end = INT64_MAX;

    if ( title )
    {
        // we're opening for read. scan passed out codec params that
        // indexed its stream so we need to remap them so they point
        // to this stream.
        stream->ffmpeg_video_id = title->video_id;
        ffmpeg_remap_stream( stream, title );
        av_log_set_level( AV_LOG_ERROR );
    }
    else
    {
        // we're opening for scan. let ffmpeg put some info into the
        // log about what we've got.
        av_log_set_level( AV_LOG_INFO );
        av_dump_format( ic, 0, stream->path, 0 );
        av_log_set_level( AV_LOG_ERROR );

        // accept this file if it has at least one video stream we can decode
        int i;
        for (i = 0; i < ic->nb_streams; ++i )
        {
            if ( ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO )
            {
                break;
            }
        }
        if ( i >= ic->nb_streams )
            goto fail;
    }
    return 1;

  fail:
    av_close_input_file( ic );
    return 0;
}

static void ffmpeg_close( hb_stream_t *d )
{
    // XXX since we're sharing the CodecContext with the downstream
    // decoder proc we can't close the stream. We need to reference count
    // this so we can close it when both are done with their instance but
    // for now just defer the close until the next stream open or close.
    if ( ffmpeg_deferred_close )
    {
        av_close_input_file( ffmpeg_deferred_close );
    }
    ffmpeg_deferred_close = d->ffmpeg_ic;
    if ( d->ffmpeg_pkt != NULL )
    {
        free( d->ffmpeg_pkt );
        d->ffmpeg_pkt = NULL;
    }
}

static void add_ffmpeg_audio( hb_title_t *title, hb_stream_t *stream, int id )
{
    AVStream *st = stream->ffmpeg_ic->streams[id];
    AVCodecContext *codec = st->codec;
    AVMetadataTag *tag;
    int layout;

    // scan will ignore any audio without a bitrate. Since we've already
    // typed the audio in order to determine its codec we set up the audio
    // paramters here.
    layout = hb_ff_layout_xlat( codec->channel_layout, codec->channels );
    if ( !layout )
    {
        // Unsupported layout
        return;
    }
    if ( codec->bit_rate || codec->sample_rate )
    {
        hb_audio_t *audio = calloc( 1, sizeof(*audio) );;

        audio->id = id;
        if ( codec->codec_id == CODEC_ID_AC3 )
        {
            audio->config.in.codec = HB_ACODEC_AC3;
        }
        else if ( codec->codec_id == CODEC_ID_DTS &&
                ( codec->profile == FF_PROFILE_DTS ||
                  codec->profile == FF_PROFILE_DTS_ES ||
                  codec->profile == FF_PROFILE_DTS_96_24 ) )
        {
            audio->config.in.codec = HB_ACODEC_DCA;
        }
        else
        {
            audio->config.in.codec = HB_ACODEC_FFMPEG;
            audio->config.in.codec_param = ffmpeg_codec_param( stream, id );

            audio->config.in.bitrate = codec->bit_rate? codec->bit_rate : 1;
            audio->config.in.samplerate = codec->sample_rate;
            audio->config.in.channel_layout = layout;
        }

        tag = av_metadata_get( st->metadata, "language", NULL, 0 );
        set_audio_description( audio, 
            lang_for_code2( tag ? tag->value : "und" ) );

        hb_list_add( title->list_audio, audio );
    }
}

/*
 * Format:
 *   MkvVobSubtitlePrivateData = ( Line )*
 *   Line = FieldName ':' ' ' FieldValue '\n'
 *   FieldName = [^:]+
 *   FieldValue = [^\n]+
 * 
 * The line of interest is:
 *   PaletteLine = "palette" ':' ' ' RRGGBB ( ',' ' ' RRGGBB )*
 * 
 * More information on the format at:
 *   http://www.matroska.org/technical/specs/subtitles/images.html
 */
static int ffmpeg_parse_vobsub_extradata_mkv( AVCodecContext *codec, hb_subtitle_t *subtitle )
{
    // lines = (string) codec->extradata;
    char *lines = malloc( codec->extradata_size + 1 );
    if ( lines == NULL )
        return 1;
    memcpy( lines, codec->extradata, codec->extradata_size );
    lines[codec->extradata_size] = '\0';
    
    uint32_t rgb[16];
    int gotPalette = 0;
    int gotDimensions = 0;
    
    char *curLine, *curLine_parserData;
    for ( curLine = strtok_r( lines, "\n", &curLine_parserData );
          curLine;
          curLine = strtok_r( NULL, "\n", &curLine_parserData ) )
    {
        if (!gotPalette)
        {
            int numElementsRead = sscanf(curLine, "palette: "
                "%06x, %06x, %06x, %06x, "
                "%06x, %06x, %06x, %06x, "
                "%06x, %06x, %06x, %06x, "
                "%06x, %06x, %06x, %06x",
                &rgb[0],  &rgb[1],  &rgb[2],  &rgb[3],
                &rgb[4],  &rgb[5],  &rgb[6],  &rgb[7],
                &rgb[8],  &rgb[9],  &rgb[10], &rgb[11],
                &rgb[12], &rgb[13], &rgb[14], &rgb[15]);

            if (numElementsRead == 16) {
                gotPalette = 1;
            }
        }
        if (!gotDimensions)
        {
            int numElementsRead = sscanf(curLine, "size: %dx%d",
                &subtitle->width, &subtitle->height);

            if (numElementsRead == 2) {
                gotDimensions = 1;
            }
        }
        if (gotPalette && gotDimensions)
            break;
    }

    if (subtitle->width == 0 || subtitle->height == 0)
    {
        subtitle->width = 720;
        subtitle->height = 480;
    }
    
    free( lines );
    
    if ( gotPalette )
    {
        int i;
        for (i=0; i<16; i++)
            subtitle->palette[i] = hb_rgb2yuv(rgb[i]);
        return 0;
    }
    else
    {
        return 1;
    }
}

/*
 * Format: 8-bit {0,Y,Cb,Cr} x 16
 */
static int ffmpeg_parse_vobsub_extradata_mp4( AVCodecContext *codec, hb_subtitle_t *subtitle )
{
    if ( codec->extradata_size != 4*16 )
        return 1;
    
    int i, j;
    for ( i=0, j=0; i<16; i++, j+=4 )
    {
        subtitle->palette[i] = 
            codec->extradata[j+1] << 16 |   // Y
            codec->extradata[j+2] << 8  |   // Cb
            codec->extradata[j+3] << 0;     // Cr
    }
    if (codec->width <= 0 || codec->height <= 0)
    {
        subtitle->width = 720;
        subtitle->height = 480;
    }
    else
    {
        subtitle->width = codec->width;
        subtitle->height = codec->height;
    }
    return 0;
}

/*
 * Parses the 'subtitle->palette' information from the specific VOB subtitle track's private data.
 * Returns 0 if successful or 1 if parsing failed or was incomplete.
 */
static int ffmpeg_parse_vobsub_extradata( AVCodecContext *codec, hb_subtitle_t *subtitle )
{
    // XXX: Better if we actually chose the correct parser based on the input container
    return
        ffmpeg_parse_vobsub_extradata_mkv( codec, subtitle ) &&
        ffmpeg_parse_vobsub_extradata_mp4( codec, subtitle );
}

static void add_ffmpeg_subtitle( hb_title_t *title, hb_stream_t *stream, int id )
{
    AVStream *st = stream->ffmpeg_ic->streams[id];
    AVCodecContext *codec = st->codec;
    
    hb_subtitle_t *subtitle = calloc( 1, sizeof(*subtitle) );
    
    subtitle->id = id;
    
    switch ( codec->codec_id )
    {
        case CODEC_ID_DVD_SUBTITLE:
            subtitle->format = PICTURESUB;
            subtitle->source = VOBSUB;
            subtitle->config.dest = RENDERSUB;  // By default render (burn-in) the VOBSUB.
            if ( ffmpeg_parse_vobsub_extradata( codec, subtitle ) )
                hb_log( "add_ffmpeg_subtitle: malformed extradata for VOB subtitle track; "
                        "subtitle colors likely to be wrong" );
            break;
        case CODEC_ID_TEXT:
            subtitle->format = TEXTSUB;
            subtitle->source = UTF8SUB;
            subtitle->config.dest = PASSTHRUSUB;
            break;
        case CODEC_ID_MOV_TEXT: // TX3G
            subtitle->format = TEXTSUB;
            subtitle->source = TX3GSUB;
            subtitle->config.dest = PASSTHRUSUB;
            break;
        case CODEC_ID_SSA:
            subtitle->format = TEXTSUB;
            subtitle->source = SSASUB;
            subtitle->config.dest = PASSTHRUSUB;
            break;
        default:
            hb_log( "add_ffmpeg_subtitle: unknown subtitle stream type: 0x%x", (int) codec->codec_id );
            free(subtitle);
            return;
    }
    
    AVMetadataTag *tag;
    iso639_lang_t *language;

    tag = av_metadata_get( st->metadata, "language", NULL, 0 );
    language = lang_for_code2( tag ? tag->value : "und" );
    strcpy( subtitle->lang, language->eng_name );
    strncpy( subtitle->iso639_2, language->iso639_2, 4 );
    
    // Copy the extradata for the subtitle track
    subtitle->extradata = malloc( codec->extradata_size );
    memcpy( subtitle->extradata, codec->extradata, codec->extradata_size );
    subtitle->extradata_size = codec->extradata_size;
    
    hb_list_add(title->list_subtitle, subtitle);
}

static char *get_ffmpeg_metadata_value( AVMetadata *m, char *key )
{
    AVMetadataTag *tag = NULL;
    while ( (tag = av_metadata_get(m, "", tag, AV_METADATA_IGNORE_SUFFIX)) )
    {
        if ( !strcmp( key, tag->key ) )
        {
            return tag->value;
        }
    }
    return NULL;
}

static void add_ffmpeg_attachment( hb_title_t *title, hb_stream_t *stream, int id )
{
    AVStream *st = stream->ffmpeg_ic->streams[id];
    AVCodecContext *codec = st->codec;
    
    enum attachtype type;
    switch ( codec->codec_id )
    {
        case CODEC_ID_TTF:
            type = FONT_TTF_ATTACH;
            break;
        default:
            // Ignore unrecognized attachment type
            return;
    }
    
    hb_attachment_t *attachment = calloc( 1, sizeof(*attachment) );
    
    // Copy the attachment name and data
    attachment->type = type;
    attachment->name = strdup( get_ffmpeg_metadata_value( st->metadata, "filename" ) );
    attachment->data = malloc( codec->extradata_size );
    memcpy( attachment->data, codec->extradata, codec->extradata_size );
    attachment->size = codec->extradata_size;
    
    hb_list_add(title->list_attachment, attachment);
}

static hb_title_t *ffmpeg_title_scan( hb_stream_t *stream )
{
    AVFormatContext *ic = stream->ffmpeg_ic;

    // 'Barebones Title'
    hb_title_t *title = hb_title_init( stream->path, 0 );
    title->type = HB_FF_STREAM_TYPE;
    title->index = 1;

    // Copy part of the stream path to the title name
    char *sep = strrchr(stream->path, '/');
    if (sep)
        strcpy(title->name, sep+1);
    char *dot_term = strrchr(title->name, '.');
    if (dot_term)
        *dot_term = '\0';

    uint64_t dur = ic->duration * 90000 / AV_TIME_BASE;
    title->duration = dur;
    dur /= 90000;
    title->hours    = dur / 3600;
    title->minutes  = ( dur % 3600 ) / 60;
    title->seconds  = dur % 60;

    // set the title to decode the first video stream in the file
    title->demuxer = HB_NULL_DEMUXER;
    title->video_codec = 0;
    int i;
    for (i = 0; i < ic->nb_streams; ++i )
    {
        if ( ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO &&
             avcodec_find_decoder( ic->streams[i]->codec->codec_id ) &&
             title->video_codec == 0 )
        {
            AVCodecContext *context = ic->streams[i]->codec;
            if ( context->pix_fmt != PIX_FMT_YUV420P &&
                 !sws_isSupportedInput( context->pix_fmt ) )
            {
                hb_log( "ffmpeg_title_scan: Unsupported color space" );
                continue;
            }
            title->video_id = i;
            stream->ffmpeg_video_id = i;

            if ( context->codec_id == CODEC_ID_H264 )
                title->flags |= HBTF_NO_IDR;

            // We have to use the 'internal' avcodec decoder because
            // it needs to share the codec context from this video
            // stream. The parser internal to av_read_frame
            // passes a bunch of state info to the decoder via the context.
            title->video_codec = WORK_DECAVCODECVI;
            title->video_codec_param = ffmpeg_codec_param( stream, i );
        }
        else if ( ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO &&
                  avcodec_find_decoder( ic->streams[i]->codec->codec_id ) )
        {
            add_ffmpeg_audio( title, stream, i );
        }
        else if ( ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_SUBTITLE )
        {
            add_ffmpeg_subtitle( title, stream, i );
        }
        else if ( ic->streams[i]->codec->codec_type == AVMEDIA_TYPE_ATTACHMENT )
        {
            add_ffmpeg_attachment( title, stream, i );
        }
    }

    title->container_name = strdup( ic->iformat->name );
    title->data_rate = ic->bit_rate;

    hb_deep_log( 2, "Found ffmpeg %d chapters, container=%s", ic->nb_chapters, ic->iformat->name );

    if( ic->nb_chapters != 0 )
    {
        AVChapter *m;
        uint64_t duration_sum = 0;
        for( i = 0; i < ic->nb_chapters; i++ )
            if( ( m = ic->chapters[i] ) != NULL )
            {
                AVMetadataTag *tag;
                hb_chapter_t * chapter;
                chapter = calloc( sizeof( hb_chapter_t ), 1 );
                chapter->index    = i+1;
                chapter->duration = ( m->end / ( (double) m->time_base.num * m->time_base.den ) ) * 90000  - duration_sum;
                duration_sum     += chapter->duration;
                chapter->hours    = chapter->duration / 90000 / 3600;
                chapter->minutes  = ( ( chapter->duration / 90000 ) % 3600 ) / 60;
                chapter->seconds  = ( chapter->duration / 90000 ) % 60;
                tag = av_metadata_get( m->metadata, "title", NULL, 0 );
                strcpy( chapter->title, tag ? tag->value : "" );
                hb_deep_log( 2, "Added chapter %i, name='%s', dur=%"PRIu64", (%02i:%02i:%02i)",
                            chapter->index, chapter->title,
                            chapter->duration, chapter->hours,
                            chapter->minutes, chapter->seconds );
                hb_list_add( title->list_chapter, chapter );
            }
    }

    /*
     * Fill the metadata.
     */
    decmetadata( title );

    if( hb_list_count( title->list_chapter ) == 0 )
    {
        // Need at least one chapter
        hb_chapter_t * chapter;
        chapter = calloc( sizeof( hb_chapter_t ), 1 );
        chapter->index = 1;
        chapter->duration = title->duration;
        chapter->hours = title->hours;
        chapter->minutes = title->minutes;
        chapter->seconds = title->seconds;
        hb_list_add( title->list_chapter, chapter );
    }

    return title;
}

static int64_t av_to_hb_pts( int64_t pts, double conv_factor )
{
    if ( pts == AV_NOPTS_VALUE )
        return -1;
    return (int64_t)( (double)pts * conv_factor );
}

static int ffmpeg_is_keyframe( hb_stream_t *stream )
{
    uint8_t *pkt;

    switch ( stream->ffmpeg_ic->streams[stream->ffmpeg_video_id]->codec->codec_id )
    {
        case CODEC_ID_VC1:
            // XXX the VC1 codec doesn't mark key frames so to get previews
            // we do it ourselves here. The decoder gets messed up if it
            // doesn't get a SEQ header first so we consider that to be a key frame.
            pkt = stream->ffmpeg_pkt->data;
            if ( !pkt[0] && !pkt[1] && pkt[2] == 1 && pkt[3] == 0x0f )
                return 1;

            return 0;

        case CODEC_ID_WMV3:
            // XXX the ffmpeg WMV3 codec doesn't mark key frames.
            // Only M$ could make I-frame detection this complicated: there
            // are two to four bits of unused junk ahead of the frame type
            // so we have to look at the sequence header to find out how much
            // to skip. Then there are three different ways of coding the type
            // depending on whether it's main or advanced profile then whether
            // there are bframes or not so we have to look at the sequence
            // header to get that.
            pkt = stream->ffmpeg_pkt->data;
            uint8_t *seqhdr = stream->ffmpeg_ic->streams[stream->ffmpeg_video_id]->codec->extradata;
            int pshift = 2;
            if ( ( seqhdr[3] & 0x02 ) == 0 )
                // no FINTERPFLAG
                ++pshift;
            if ( ( seqhdr[3] & 0x80 ) == 0 )
                // no RANGEREDUCTION
                ++pshift;
            if ( seqhdr[3] & 0x70 )
                // stream has b-frames
                return ( ( pkt[0] >> pshift ) & 0x3 ) == 0x01;

            return ( ( pkt[0] >> pshift ) & 0x2 ) == 0;

        default:
            break;
    }
    return ( stream->ffmpeg_pkt->flags & AV_PKT_FLAG_KEY );
}

static hb_buffer_t * ffmpeg_read( hb_stream_t *stream )
{
    int err;
    hb_buffer_t * buf;

  again:
    if ( ( err = av_read_frame( stream->ffmpeg_ic, stream->ffmpeg_pkt )) < 0 )
    {
        // av_read_frame can return EAGAIN.  In this case, it expects
        // to be called again to get more data.
        if ( err == AVERROR(EAGAIN) )
        {
            goto again;
        }
        // XXX the following conditional is to handle avi files that
        // use M$ 'packed b-frames' and occasionally have negative
        // sizes for the null frames these require.
        if ( err != AVERROR(ENOMEM) || stream->ffmpeg_pkt->size >= 0 )
            // eof
            return NULL;
    }
    if ( stream->ffmpeg_pkt->stream_index == stream->ffmpeg_video_id )
    {
        if ( stream->need_keyframe )
        {
            // we've just done a seek (generally for scan or live preview) and
            // want to start at a keyframe. Some ffmpeg codecs seek to a key
            // frame but most don't. So we spin until we either get a keyframe
            // or we've looked through 50 video frames without finding one.
            if ( ! ffmpeg_is_keyframe( stream ) && ++stream->need_keyframe < 50 )
            {
                av_free_packet( stream->ffmpeg_pkt );
                goto again;
            }
            stream->need_keyframe = 0;
        }
        ++stream->frames;
    }
    if ( stream->ffmpeg_pkt->size <= 0 )
    {
        // M$ "invalid and inefficient" packed b-frames require 'null frames'
        // following them to preserve the timing (since the packing puts two
        // or more frames in what looks like one avi frame). The contents and
        // size of these null frames are ignored by the ff_h263_decode_frame
        // as long as they're < 20 bytes. We need a positive size so we use
        // one byte if we're given a zero or negative size. We don't know
        // if the pkt data points anywhere reasonable so we just stick a
        // byte of zero in our outbound buf.
        buf = hb_buffer_init( 1 );
        *buf->data = 0;
    }
    else
    {
        // sometimes we get absurd sizes from ffmpeg
        if ( stream->ffmpeg_pkt->size >= (1 << 25) )
        {
            hb_log( "ffmpeg_read: pkt too big: %d bytes", stream->ffmpeg_pkt->size );
            av_free_packet( stream->ffmpeg_pkt );
            return ffmpeg_read( stream );
        }
        buf = hb_buffer_init( stream->ffmpeg_pkt->size );
        memcpy( buf->data, stream->ffmpeg_pkt->data, stream->ffmpeg_pkt->size );
    }
    buf->id = stream->ffmpeg_pkt->stream_index;

    // compute a conversion factor to go from the ffmpeg
    // timebase for the stream to HB's 90kHz timebase.
    AVStream *s = stream->ffmpeg_ic->streams[stream->ffmpeg_pkt->stream_index];
    double tsconv = 90000. * (double)s->time_base.num / (double)s->time_base.den;

    buf->start = av_to_hb_pts( stream->ffmpeg_pkt->pts, tsconv );
    buf->renderOffset = av_to_hb_pts( stream->ffmpeg_pkt->dts, tsconv );
    if ( buf->renderOffset >= 0 && buf->start == -1 )
    {
        buf->start = buf->renderOffset;
    }
    else if ( buf->renderOffset == -1 && buf->start >= 0 )
    {
        buf->renderOffset = buf->start;
    }
    
    /* 
     * Fill out buf->stop for subtitle packets
     * 
     * libavcodec's MKV demuxer stores the duration of UTF-8 subtitles (CODEC_ID_TEXT)
     * in the 'convergence_duration' field for some reason.
     * 
     * Other subtitles' durations are stored in the 'duration' field.
     * 
     * VOB subtitles (CODEC_ID_DVD_SUBTITLE) do not have their duration stored in
     * either field. This is not a problem because the VOB decoder can extract this
     * information from the packet payload itself.
     * 
     * SSA subtitles (CODEC_ID_SSA) do not have their duration stored in
     * either field. This is not a problem because the SSA decoder can extract this
     * information from the packet payload itself.
     */
    enum CodecID ffmpeg_pkt_codec;
    enum AVMediaType codec_type;
    ffmpeg_pkt_codec = stream->ffmpeg_ic->streams[stream->ffmpeg_pkt->stream_index]->codec->codec_id;
    codec_type = stream->ffmpeg_ic->streams[stream->ffmpeg_pkt->stream_index]->codec->codec_type;
    switch ( codec_type )
    {
        case AVMEDIA_TYPE_VIDEO:
            buf->type = VIDEO_BUF;
            break;

        case AVMEDIA_TYPE_AUDIO:
            buf->type = AUDIO_BUF;
            break;

        case AVMEDIA_TYPE_SUBTITLE:
            buf->type = SUBTITLE_BUF;
            break;

        default:
            buf->type = OTHER_BUF;
            break;
    }
    if ( ffmpeg_pkt_codec == CODEC_ID_TEXT ) {
        int64_t ffmpeg_pkt_duration = stream->ffmpeg_pkt->convergence_duration;
        int64_t buf_duration = av_to_hb_pts( ffmpeg_pkt_duration, tsconv );
        buf->stop = buf->start + buf_duration;
    }
    if ( ffmpeg_pkt_codec == CODEC_ID_MOV_TEXT ) {
        int64_t ffmpeg_pkt_duration = stream->ffmpeg_pkt->duration;
        int64_t buf_duration = av_to_hb_pts( ffmpeg_pkt_duration, tsconv );
        buf->stop = buf->start + buf_duration;
    }

    /*
     * Check to see whether this buffer is on a chapter
     * boundary, if so mark it as such in the buffer then advance
     * chapter_end to the end of the next chapter.
     * If there are no chapters, chapter_end is always initialized to INT64_MAX
     * (roughly 3 million years at our 90KHz clock rate) so the test
     * below handles both the chapters & no chapters case.
     */
    if ( stream->ffmpeg_pkt->stream_index == stream->ffmpeg_video_id &&
         buf->start >= stream->chapter_end )
    {
        hb_chapter_t *chapter = hb_list_item( stream->title->list_chapter,
                                              stream->chapter+1 );
        if( chapter )
        {
            stream->chapter++;
            stream->chapter_end += chapter->duration;
            buf->new_chap = stream->chapter + 1;
            hb_deep_log( 2, "ffmpeg_read starting chapter %i at %"PRId64,
                         buf->new_chap, buf->start);
        } else {
            // Must have run out of chapters, stop looking.
            stream->chapter_end = INT64_MAX;
        }
    } else {
        buf->new_chap = 0;
    }
    av_free_packet( stream->ffmpeg_pkt );
    return buf;
}

static int ffmpeg_seek( hb_stream_t *stream, float frac )
{
    AVFormatContext *ic = stream->ffmpeg_ic;
    if ( frac > 0. )
    {
        int64_t pos = (double)ic->duration * (double)frac;
        if ( ic->start_time != AV_NOPTS_VALUE && ic->start_time > 0 )
        {
            pos += ic->start_time;
        }
        av_seek_frame( ic, -1, pos, 0 );
        stream->need_keyframe = 1;
        ffmpeg_flush_stream_buffers( stream );
    }
    else
    {
        av_seek_frame( ic, -1, 0LL, AVSEEK_FLAG_BACKWARD );
        ffmpeg_flush_stream_buffers( stream );
    }
    return 1;
}

// Assumes that we are always seeking forward
static int ffmpeg_seek_ts( hb_stream_t *stream, int64_t ts )
{
    AVFormatContext *ic = stream->ffmpeg_ic;
    int64_t pos;
    int ret;

    pos = ts * AV_TIME_BASE / 90000 + ffmpeg_initial_timestamp( stream );
    stream->need_keyframe = 1;
    // Seek to the nearest timestamp before that requested where
    // there is an I-frame
    ret = av_seek_frame( ic, -1, pos, AVSEEK_FLAG_BACKWARD );
    ffmpeg_flush_stream_buffers( stream );
    return ret;
}
