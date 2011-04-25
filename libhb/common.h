/* $Id: common.h,v 1.51 2005/11/04 13:09:40 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#ifndef HB_COMMON_H
#define HB_COMMON_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#if defined( __GNUC__ ) && !(defined( _WIN32 ) || defined( __MINGW32__ ))
#   define HB_WPRINTF(s,v) __attribute__((format(printf,s,v)))
#else
#   define HB_WPRINTF(s,v)
#endif

#if defined( SYS_MINGW )
#   define fseek fseeko64
#   define ftell ftello64
#   undef  fseeko
#   define fseeko fseeko64
#   undef  ftello
#   define ftello ftello64
#   define flockfile(...)
#   define funlockfile(...)
#   define getc_unlocked getc
#   undef  off_t
#   define off_t off64_t
#endif

#ifndef MIN
#define MIN( a, b ) ( (a) > (b) ? (b) : (a) )
#endif
#ifndef MAX
#define MAX( a, b ) ( (a) > (b) ? (a) : (b) )
#endif

#define EVEN( a )        ( (a) + ( (a) & 1 ) )
#define MULTIPLE_16( a ) ( 16 * ( ( (a) + 8 ) / 16 ) )
#define MULTIPLE_MOD( a, b ) ((b==1)?a:( b * ( ( (a) + (b / 2) - 1) / b ) ))
#define MULTIPLE_MOD_DOWN( a, b ) ((b==1)?a:( b * ( (a) / b ) ))

#define HB_DVD_READ_BUFFER_SIZE 2048

typedef struct hb_handle_s hb_handle_t;
typedef struct hb_list_s hb_list_t;
typedef struct hb_rate_s hb_rate_t;
typedef struct hb_mixdown_s hb_mixdown_t;
typedef struct hb_job_s  hb_job_t;
typedef struct hb_title_s hb_title_t;
typedef struct hb_chapter_s hb_chapter_t;
typedef struct hb_audio_s hb_audio_t;
typedef struct hb_audio_config_s hb_audio_config_t;
typedef struct hb_subtitle_s hb_subtitle_t;
typedef struct hb_subtitle_config_s hb_subtitle_config_t;
typedef struct hb_attachment_s hb_attachment_t;
typedef struct hb_metadata_s hb_metadata_t;
typedef struct hb_state_s hb_state_t;
typedef union  hb_esconfig_u     hb_esconfig_t;
typedef struct hb_work_private_s hb_work_private_t;
typedef struct hb_work_object_s  hb_work_object_t;
typedef struct hb_filter_private_s hb_filter_private_t;
typedef struct hb_filter_object_s  hb_filter_object_t;
typedef struct hb_buffer_s hb_buffer_t;
typedef struct hb_fifo_s hb_fifo_t;
typedef struct hb_lock_s hb_lock_t;

#include "ports.h"
#ifdef __LIBHB__
#include "internal.h"
#define PRIVATE
#else
#define PRIVATE const
#endif

hb_list_t * hb_list_init();
int         hb_list_count( hb_list_t * );
void        hb_list_add( hb_list_t *, void * );
void        hb_list_insert( hb_list_t * l, int pos, void * p );
void        hb_list_rem( hb_list_t *, void * );
void      * hb_list_item( hb_list_t *, int );
void        hb_list_close( hb_list_t ** );

void hb_reduce( int *x, int *y, int num, int den );
void hb_reduce64( int64_t *x, int64_t *y, int64_t num, int64_t den );
void hb_limit_rational64( int64_t *x, int64_t *y, int64_t num, int64_t den, int64_t limit );

#define HB_KEEP_WIDTH  0
#define HB_KEEP_HEIGHT 1
void hb_fix_aspect( hb_job_t * job, int keep );

int hb_calc_bitrate( hb_job_t *, int size );

hb_audio_t *hb_audio_copy(const hb_audio_t *src);
void hb_audio_config_init(hb_audio_config_t * audiocfg);
int hb_audio_add(const hb_job_t * job, const hb_audio_config_t * audiocfg);
hb_audio_config_t * hb_list_audio_config_item(hb_list_t * list, int i);

hb_subtitle_t *hb_subtitle_copy(const hb_subtitle_t *src);
int hb_subtitle_add(const hb_job_t * job, const hb_subtitle_config_t * subtitlecfg, int track);
int hb_srt_add(const hb_job_t * job, const hb_subtitle_config_t * subtitlecfg, 
               const char *lang);

hb_attachment_t *hb_attachment_copy(const hb_attachment_t *src);

struct hb_rate_s
{
    char * string;
    int    rate;
};

struct hb_mixdown_s
{
    char * human_readable_name;
    char * internal_name;
    char * short_name;
    int    amixdown;
};

struct hb_subtitle_config_s
{
    enum subdest { RENDERSUB, PASSTHRUSUB } dest;
    int  force;
    int  default_track; 
    
    /* SRT subtitle tracks only */
    char src_filename[256];
    char src_codeset[40];
    int64_t offset;
};

#define HB_VIDEO_RATE_BASE   27000000

extern hb_rate_t    hb_video_rates[];
extern int          hb_video_rates_count;
extern hb_rate_t    hb_audio_rates[];
extern int          hb_audio_rates_count;
extern int          hb_audio_rates_default;
extern hb_rate_t    hb_audio_bitrates[];
extern int          hb_audio_bitrates_count;
extern hb_mixdown_t hb_audio_mixdowns[];
extern int          hb_audio_mixdowns_count;
int hb_mixdown_get_mixdown_from_short_name( const char * short_name );
const char * hb_mixdown_get_short_name_from_mixdown( int amixdown );
int hb_get_best_mixdown( uint32_t codec, int layout, int mixdown );
int hb_get_default_mixdown( uint32_t codec, int layout );
int hb_find_closest_audio_bitrate(int bitrate);
void hb_get_audio_bitrate_limits(uint32_t codec, int samplerate, int mixdown, int *low, int *high);
int hb_get_best_audio_bitrate( uint32_t codec, int bitrate, int samplerate, int mixdown);
int hb_get_default_audio_bitrate( uint32_t codec, int samplerate, int mixdown );

/******************************************************************************
 * hb_job_t: settings to be filled by the UI
 *****************************************************************************/
struct hb_job_s
{
    /* ID assigned by UI so it can groups job passes together */
    int             sequence_id;

    /* Pointer to the title to be ripped */
    hb_title_t    * title;
    int             feature; // Detected DVD feature title

    /* Chapter selection */
    int             chapter_start;
    int             chapter_end;

	/* Include chapter marker track in mp4? */
    int             chapter_markers;

    /* Picture settings:
         crop:                must be multiples of 2 (top/bottom/left/right)
         deinterlace:         0 or 1
         width:               must be a multiple of 2
         height:              must be a multiple of 2
         keep_ratio:          used by UIs
         grayscale:           black and white encoding
         pixel_ratio:         store pixel aspect ratio in the video
         pixel_aspect_width:  numerator for pixel aspect ratio
         pixel_aspect_height: denominator for pixel aspect ratio
         modulus:             set a number for dimensions to be multiples of
         maxWidth:            keep width below this
         maxHeight:           keep height below this */
    int             crop[4];
    int             deinterlace;
    hb_list_t     * filters;
    int             width;
    int             height;
    int             keep_ratio;
    int             grayscale;

    struct
    {
        int             mode;
        int             itu_par;
        int             par_width;
        int             par_height;
        int             dar_width;  // 0 if normal
        int             dar_height; // 0 if normal
        int             keep_display_aspect;
    } anamorphic;

    int             modulus;
    int             maxWidth;
    int             maxHeight;

    /* Video settings:
         vcodec:            output codec
         vquality:          output quality (0.0..1.0)
                            if < 0.0 or > 1.0, bitrate is used instead,
                            except with x264, to use its real QP/RF scale
         vbitrate:          output bitrate (kbps)
         vrate, vrate_base: output framerate is vrate / vrate_base
         cfr:               0 (vfr), 1 (cfr), 2 (pfr) [see render.c]
         pass:              0, 1 or 2 (or -1 for scan)
         advanced_opts:     string of extra advanced encoder options
         areBframes:        boolean to note if b-frames are included in advanced_opts */
#define HB_VCODEC_MASK   0x0000FF
#define HB_VCODEC_X264   0x000001
#define HB_VCODEC_THEORA 0x000002
#define HB_VCODEC_FFMPEG_MPEG4 0x000010
#define HB_VCODEC_FFMPEG       HB_VCODEC_FFMPEG_MPEG4
#define HB_VCODEC_FFMPEG_MPEG2 0x000020
#define HB_VCODEC_FFMPEG_MASK  0x0000F0

    int             vcodec;
    float           vquality;
    int             vbitrate;
    int             pfr_vrate;
    int             pfr_vrate_base;
    int             vrate;
    int             vrate_base;
    int             cfr;
    int             pass;
    char            *advanced_opts;
    int             areBframes;
    int             color_matrix;

    /* List of audio settings. */
    hb_list_t     * list_audio;

    /* Subtitles */
    hb_list_t     * list_subtitle;

    /* Muxer settings
         mux:  output file format
         file: file path */
#define HB_MUX_MASK 0xFF0000
#define HB_MUX_MP4  0x010000
#define HB_MUX_MKV  0x200000

    int             mux;
    const char          * file;

    /* Allow MP4 files > 4 gigs */
    int             largeFileSize;
    int             mp4_optimize;
    int             ipod_atom;

    int                     indepth_scan;
    hb_subtitle_config_t    select_subtitle_config;

    int             angle;              // dvd angle to encode
    int             frame_to_start;     // declare eof when we hit this frame
    int64_t         pts_to_start;       // drop frames until  we pass this pts 
                                        //  in the time-linearized input stream
    int             frame_to_stop;      // declare eof when we hit this frame
    int64_t         pts_to_stop;        // declare eof when we pass this pts in
                                        //  the time-linearized input stream
    int             start_at_preview;   // if non-zero, encoding will start
                                        //  at the position of preview n
    int             seek_points;        //  out of N previews
    uint32_t        frames_to_skip;     // decode but discard this many frames
                                        //  initially (for frame accurate positioning
                                        //  to non-I frames).

#ifdef __LIBHB__
    /* Internal data */
    hb_handle_t   * h;
    hb_lock_t     * pause;
    volatile int  * die;
    volatile int    done;

    uint64_t        st_pause_date;
    uint64_t        st_paused;

    hb_fifo_t     * fifo_mpeg2;   /* MPEG-2 video ES */
    hb_fifo_t     * fifo_raw;     /* Raw pictures */
    hb_fifo_t     * fifo_sync;    /* Raw pictures, framerate corrected */
    hb_fifo_t     * fifo_render;  /* Raw pictures, scaled */
    hb_fifo_t     * fifo_mpeg4;   /* MPEG-4 video ES */

    hb_thread_t   * reader;

    hb_list_t     * list_work;

    hb_esconfig_t config;

    hb_mux_data_t * mux_data;
#endif
};

/* Audio starts here */
/* Audio Codecs */
#define HB_ACODEC_MASK      0x000FFF00
#define HB_ACODEC_FAAC      0x00000100
#define HB_ACODEC_LAME      0x00000200
#define HB_ACODEC_VORBIS    0x00000400
#define HB_ACODEC_AC3       0x00000800
#define HB_ACODEC_MPGA      0x00001000
#define HB_ACODEC_LPCM      0x00002000
#define HB_ACODEC_DCA       0x00004000
#define HB_ACODEC_FFMPEG    0x00008000
#define HB_ACODEC_CA_AAC    0x00010000
#define HB_ACODEC_CA_HAAC   0x00020000
#define HB_ACODEC_PASS_FLAG 0x40000000
#define HB_ACODEC_PASS_MASK (HB_ACODEC_AC3 | HB_ACODEC_DCA)
#define HB_ACODEC_AC3_PASS  (HB_ACODEC_AC3 | HB_ACODEC_PASS_FLAG)
#define HB_ACODEC_DCA_PASS  (HB_ACODEC_DCA | HB_ACODEC_PASS_FLAG)
#define HB_ACODEC_ANY       (HB_ACODEC_MASK | HB_ACODEC_PASS_FLAG)

#define HB_SUBSTREAM_BD_TRUEHD  0x72
#define HB_SUBSTREAM_BD_AC3     0x76
#define HB_SUBSTREAM_BD_DTSHD   0x72
#define HB_SUBSTREAM_BD_DTS     0x71

/* Audio Mixdown */
/* define some masks, used to extract the various information from the HB_AMIXDOWN_XXXX values */
#define HB_AMIXDOWN_DCA_FORMAT_MASK             0x00FFF000
#define HB_AMIXDOWN_A52_FORMAT_MASK             0x00000FF0
#define HB_AMIXDOWN_DISCRETE_CHANNEL_COUNT_MASK 0x0000000F
/* define the HB_AMIXDOWN_XXXX values */
#define HB_AMIXDOWN_MONO                        0x01000001
// DCA_FORMAT of DCA_MONO                  = 0    = 0x000
// A52_FORMAT of A52_MONO                  = 1    = 0x01
// discrete channel count of 1
#define HB_AMIXDOWN_STEREO                      0x02002022
// DCA_FORMAT of DCA_STEREO                = 2    = 0x002
// A52_FORMAT of A52_STEREO                = 2    = 0x02
// discrete channel count of 2
#define HB_AMIXDOWN_DOLBY                       0x042070A2
// DCA_FORMAT of DCA_3F1R | DCA_OUT_DPLI   = 519  = 0x207
// A52_FORMAT of A52_DOLBY                 = 10   = 0x0A
// discrete channel count of 2
#define HB_AMIXDOWN_DOLBYPLII                   0x084094A2
// DCA_FORMAT of DCA_3F2R | DCA_OUT_DPLII  = 1033 = 0x409
// A52_FORMAT of A52_DOLBY | A52_USE_DPLII = 74   = 0x4A
// discrete channel count of 2
#define HB_AMIXDOWN_6CH                         0x10089176
// DCA_FORMAT of DCA_3F2R | DCA_LFE        = 137  = 0x089
// A52_FORMAT of A52_3F2R | A52_LFE        = 23   = 0x17
// discrete channel count of 6
/* define some macros to extract the various information from the HB_AMIXDOWN_XXXX values */
#define HB_AMIXDOWN_GET_DCA_FORMAT( a ) ( ( a & HB_AMIXDOWN_DCA_FORMAT_MASK ) >> 12 )
#define HB_AMIXDOWN_GET_A52_FORMAT( a ) ( ( a & HB_AMIXDOWN_A52_FORMAT_MASK ) >> 4 )
#define HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT( a ) ( ( a & HB_AMIXDOWN_DISCRETE_CHANNEL_COUNT_MASK ) )

/* Input Channel Layout */
/* define some masks, used to extract the various information from the HB_AMIXDOWN_XXXX values */
#define HB_INPUT_CH_LAYOUT_DISCRETE_FRONT_MASK  0x00F0000
#define HB_INPUT_CH_LAYOUT_DISCRETE_REAR_MASK   0x000F000
#define HB_INPUT_CH_LAYOUT_DISCRETE_LFE_MASK    0x0000F00
#define HB_INPUT_CH_LAYOUT_DISCRETE_NO_LFE_MASK 0xFFFF0FF
#define HB_INPUT_CH_LAYOUT_ENCODED_FRONT_MASK   0x00000F0
#define HB_INPUT_CH_LAYOUT_ENCODED_REAR_MASK    0x000000F
/* define the input channel layouts used to describe the channel layout of this audio */
#define HB_INPUT_CH_LAYOUT_MONO    0x0110010
#define HB_INPUT_CH_LAYOUT_STEREO  0x0220020
#define HB_INPUT_CH_LAYOUT_DOLBY   0x0320031
#define HB_INPUT_CH_LAYOUT_3F      0x0430030
#define HB_INPUT_CH_LAYOUT_2F1R    0x0521021
#define HB_INPUT_CH_LAYOUT_3F1R    0x0631031
#define HB_INPUT_CH_LAYOUT_2F2R    0x0722022
#define HB_INPUT_CH_LAYOUT_3F2R    0x0832032
#define HB_INPUT_CH_LAYOUT_4F2R    0x0942042
#define HB_INPUT_CH_LAYOUT_3F4R    0x0a34034
#define HB_INPUT_CH_LAYOUT_HAS_LFE 0x0000100
/* define some macros to extract the various information from the HB_AMIXDOWN_XXXX values */
#define HB_INPUT_CH_LAYOUT_GET_DISCRETE_FRONT_COUNT( a ) ( ( a & HB_INPUT_CH_LAYOUT_DISCRETE_FRONT_MASK ) >> 16 )
#define HB_INPUT_CH_LAYOUT_GET_DISCRETE_REAR_COUNT( a )  ( ( a & HB_INPUT_CH_LAYOUT_DISCRETE_REAR_MASK ) >> 12 )
#define HB_INPUT_CH_LAYOUT_GET_DISCRETE_LFE_COUNT( a )   ( ( a & HB_INPUT_CH_LAYOUT_DISCRETE_LFE_MASK ) >> 8 )
#define HB_INPUT_CH_LAYOUT_GET_DISCRETE_COUNT( a ) ( ( ( a & HB_INPUT_CH_LAYOUT_DISCRETE_FRONT_MASK ) >> 16 ) + ( ( a & HB_INPUT_CH_LAYOUT_DISCRETE_REAR_MASK ) >> 12 ) + ( ( a & HB_INPUT_CH_LAYOUT_DISCRETE_LFE_MASK ) >> 8 ) )
#define HB_INPUT_CH_LAYOUT_GET_ENCODED_FRONT_COUNT( a )   ( ( a & HB_INPUT_CH_LAYOUT_ENCODED_FRONT_MASK ) >> 4 )
#define HB_INPUT_CH_LAYOUT_GET_ENCODED_REAR_COUNT( a )   ( ( a & HB_INPUT_CH_LAYOUT_ENCODED_REAR_MASK ) )

struct hb_audio_config_s
{
    /* Output */
    struct
    {
            int track;      /* Output track number */
            uint32_t codec;  /* Output audio codec.
                                 * HB_ACODEC_AC3 means pass-through, then bitrate and samplerate
                                 * are ignored.
                                 */
            int samplerate; /* Output sample rate (Hz) */
            int bitrate;    /* Output bitrate (kbps) */
            int mixdown;    /* The mixdown format to be used for this audio track (see HB_AMIXDOWN_*) */
            double dynamic_range_compression; /* Amount of DRC that gets applied to this track */
            double gain;    /* Gain in dB. negative is quieter */
            char * name;    /* Output track name */
    } out;

    /* Input */
    struct
    {
        int track;                /* Input track number */
        PRIVATE uint32_t codec;   /* Input audio codec */
        PRIVATE uint32_t reg_desc; /* registration descriptor of source */
        PRIVATE uint32_t stream_type; /* stream type from source stream */
        PRIVATE uint32_t substream_type; /* substream for multiplexed streams */
        PRIVATE uint32_t codec_param; /* per-codec config info */
        PRIVATE uint32_t version; /* Bitsream version */
        PRIVATE uint32_t mode;    /* Bitstream mode, codec dependent encoding */
        PRIVATE int samplerate; /* Input sample rate (Hz) */
        PRIVATE int bitrate;    /* Input bitrate (kbps) */
        PRIVATE int channel_layout;
        /* channel_layout is the channel layout of this audio this is used to
        * provide a common way of describing the source audio
        */
    } in;

    /* Misc. */
    union
    {
        PRIVATE int ac3;    /* flags.ac3 is only set when the source audio format is HB_ACODEC_AC3 */
        PRIVATE int dca;    /* flags.dca is only set when the source audio format is HB_ACODEC_DCA */
    } flags;
#define AUDIO_F_DOLBY (1 << 31)  /* set if source uses Dolby Surround */

    struct
    {
        PRIVATE char description[1024];
        PRIVATE char simple[1024];
        PRIVATE char iso639_2[4];
        PRIVATE uint8_t type; /* normal, visually impared, directors */
    } lang;
};

#ifdef __LIBHB__
struct hb_audio_s
{
    int id;

    hb_audio_config_t config;

    struct {
        hb_fifo_t * fifo_in;   /* AC3/MPEG/LPCM ES */
        hb_fifo_t * fifo_raw;  /* Raw audio */
        hb_fifo_t * fifo_sync; /* Resampled, synced raw audio */
        hb_fifo_t * fifo_out;  /* MP3/AAC/Vorbis ES */

        hb_esconfig_t config;
        hb_mux_data_t * mux_data;
        hb_fifo_t     * scan_cache;

        hb_list_t     * ff_audio_list;
    } priv;
};
#endif

struct hb_chapter_s
{
    int      index;
    int      pgcn;
    int      pgn;
    int      cell_start;
    int      cell_end;
    uint64_t block_start;
    uint64_t block_end;
    uint64_t block_count;

    /* Visual-friendly duration */
    int      hours;
    int      minutes;
    int      seconds;

    /* Exact duration (in 1/90000s) */
    uint64_t duration;

    /* Optional chapter title */
    char     title[1024];
};

/*
 * A subtitle track.
 * 
 * Required fields when a demuxer creates a subtitle track are:
 * > id
 *     - ID of this track
 *     - must be unique for all tracks within a single job,
 *       since it is used to look up the appropriate in-FIFO with GetFifoForId()
 * > format
 *     - format of the packets the subtitle decoder work-object sends to sub->fifo_raw
 *     - for passthru subtitles, is also the format of the final packets sent to sub->fifo_out
 *     - PICTURESUB for banded 8-bit YAUV pixels; see decvobsub.c documentation for more info
 *     - TEXTSUB for UTF-8 text marked up with <b>, <i>, or <u>
 *     - read by the muxers, and by the subtitle burn-in logic in the hb_sync_video work-object
 * > source
 *     - used to create the appropriate subtitle decoder work-object in do_job()
 * > config.dest
 *     - whether to render the subtitle on the video track (RENDERSUB) or 
 *       to pass it through its own subtitle track in the output container (PASSTHRUSUB)
 *     - all newly created non-VOBSUB tracks should default to PASSTHRUSUB
 *     - all newly created VOBSUB tracks should default to RENDERSUB, for legacy compatibility
 * > lang
 *     - user-readable description of the subtitle track
 *     - may correspond to the language of the track (see the 'iso639_2' field)
 *     - may correspond to the type of track (see the 'type' field; ex: "Closed Captions")
 * > iso639_2
 *     - language code for the subtitle, or "und" if unknown
 */
struct hb_subtitle_s
{
    int  id;
    int  track;

    hb_subtitle_config_t config;

    enum subtype { PICTURESUB, TEXTSUB } format;
    enum subsource { VOBSUB, SRTSUB, CC608SUB, /*unused*/CC708SUB, UTF8SUB, TX3GSUB, SSASUB } source;
    char lang[1024];
    char iso639_2[4];
    uint8_t type; /* Closed Caption, Childrens, Directors etc */
    
    // Color lookup table for VOB subtitle tracks. Each entry is in YCbCr format.
    // Must be filled out by the demuxer for VOB subtitle tracks.
    uint32_t    palette[16];
    int         width;
    int         height;
    
    // Codec private data for subtitles originating from FFMPEG sources
    uint8_t *   extradata;
    int         extradata_size;

    int hits;     /* How many hits/occurrences of this subtitle */
    int forced_hits; /* How many forced hits in this subtitle */

#ifdef __LIBHB__
    /* Internal data */
    hb_fifo_t * fifo_in;  /* SPU ES */
    hb_fifo_t * fifo_raw; /* Decoded SPU */
    hb_fifo_t * fifo_sync;/* Synced */
    hb_fifo_t * fifo_out; /* Correct Timestamps, ready to be muxed */
    hb_mux_data_t * mux_data;
#endif
};

/*
 * An attachment.
 * 
 * These are usually used for attaching embedded fonts to movies containing SSA subtitles.
 */
struct hb_attachment_s
{
    enum attachtype { FONT_TTF_ATTACH } type;
    char *  name;
    char *  data;
    int     size;
};

struct hb_metadata_s 
{
    char  name[255];
    char  artist[255];
    char  composer[255];
    char  release_date[255];
    char  comment[1024];
    char  album[255];
    char  genre[255];
    uint32_t coverart_size;
    uint8_t *coverart;
};

struct hb_title_s
{
    enum { HB_DVD_TYPE, HB_BD_TYPE, HB_STREAM_TYPE, HB_FF_STREAM_TYPE } type;
    uint32_t    reg_desc;
    char        path[1024];
    char        name[1024];
    int         index;
    int         vts;
    int         ttn;
    int         cell_start;
    int         cell_end;
    uint64_t    block_start;
    uint64_t    block_end;
    uint64_t    block_count;
    int         angle_count;

    /* Visual-friendly duration */
    int         hours;
    int         minutes;
    int         seconds;

    /* Exact duration (in 1/90000s) */
    uint64_t    duration;

    double      aspect;             // aspect ratio for the title's video
    double      container_aspect;   // aspect ratio from container (0 if none)
    int         width;
    int         height;
    int         pixel_aspect_width;
    int         pixel_aspect_height;
    int         rate;
    int         rate_base;
    int         crop[4];
    enum { HB_MPEG2_PS_DEMUXER = 0, HB_MPEG2_TS_DEMUXER, HB_NULL_DEMUXER } demuxer;
    int         detected_interlacing;
    int         pcr_pid;                /* PCR PID for TS streams */
    int         video_id;               /* demuxer stream id for video */
    int         video_codec;            /* worker object id of video codec */
    uint32_t    video_stream_type;      /* stream type from source stream */
    int         video_codec_param;      /* codec specific config */
    const char  *video_codec_name;
    int         video_bitrate;
    const char  *container_name;
    int         data_rate;

    hb_metadata_t *metadata;

    hb_list_t * list_chapter;
    hb_list_t * list_audio;
    hb_list_t * list_subtitle;
    hb_list_t * list_attachment;

    /* Job template for this title */
    hb_job_t  * job;

    uint32_t    flags;
                // set if video stream doesn't have IDR frames
#define         HBTF_NO_IDR (1 << 0)
};


struct hb_state_s
{
#define HB_STATE_IDLE     1
#define HB_STATE_SCANNING 2
#define HB_STATE_SCANDONE 4
#define HB_STATE_WORKING  8
#define HB_STATE_PAUSED   16
#define HB_STATE_WORKDONE 32
#define HB_STATE_MUXING   64
#define HB_STATE_SEARCHING 128
    int state;

    union
    {
        struct
        {
            /* HB_STATE_SCANNING */
            int title_cur;
            int title_count;
        } scanning;

        struct
        {
            /* HB_STATE_WORKING */
            float progress;
            int   job_cur;
            int   job_count;
            float rate_cur;
            float rate_avg;
            int   hours;
            int   minutes;
            int   seconds;
            int   sequence_id;
        } working;

        struct
        {
            /* HB_STATE_WORKDONE */
#define HB_ERROR_NONE     0
#define HB_ERROR_CANCELED 1
#define HB_ERROR_UNKNOWN  2
            int error;
        } workdone;

        struct
        {
            /* HB_STATE_MUXING */
            float progress;
        } muxing;
    } param;
};

typedef struct hb_work_info_s
{
    const char *name;
    int     profile;
    int     level;
    int     bitrate;
    int     rate;
    int     rate_base;
    int     flags;
    int     version;
    int     mode;
    union {
        struct {    // info only valid for video decoders
            int     width;
            int     height;
            int     pixel_aspect_width;
            int     pixel_aspect_height;
            double  aspect;
        };
        struct {    // info only valid for audio decoders
            int     channel_layout;
        };
    };
} hb_work_info_t;

struct hb_work_object_s
{
    int                 id;
    char              * name;

#ifdef __LIBHB__
    int              (* init)  ( hb_work_object_t *, hb_job_t * );
    int              (* work)  ( hb_work_object_t *, hb_buffer_t **,
                                 hb_buffer_t ** );
    void             (* close) ( hb_work_object_t * );
    /* the info entry point is used by scan to get bitstream information
     * during a decode (i.e., it should only be called after at least one
     * call to the 'work' entry point). currently it's only called for
     * video streams & can be null for other work objects. */
    int              (* info)  ( hb_work_object_t *, hb_work_info_t * );
    /* the bitstream info entry point is used by scan to get bitstream
     * information from a buffer. it doesn't have to be called during a
     * decode (it can be called even if init & work haven't been).
     * currently it's only called for audio streams & can be null for
     * other work objects. */
    int              (* bsinfo)  ( hb_work_object_t *, const hb_buffer_t *, 
                                   hb_work_info_t * );

    hb_fifo_t         * fifo_in;
    hb_fifo_t         * fifo_out;
    hb_esconfig_t     * config;

    /* Pointer hb_audio_t so we have access to the info in the audio worker threads. */
    hb_audio_t        * audio;

    /* Pointer hb_subtitle_t so we have access to the info in the subtitle worker threads. */
    hb_subtitle_t     * subtitle;

    hb_work_private_t * private_data;

    hb_thread_t       * thread;
    volatile int      * done;
    int                 status;
    int                 codec_param;
    hb_title_t        * title;

    hb_work_object_t  * next;
    int                 thread_sleep_interval;
#endif
};

extern hb_work_object_t hb_sync_video;
extern hb_work_object_t hb_sync_audio;
extern hb_work_object_t hb_decmpeg2;
extern hb_work_object_t hb_decvobsub;
extern hb_work_object_t hb_encvobsub;
extern hb_work_object_t hb_deccc608;
extern hb_work_object_t hb_decsrtsub;
extern hb_work_object_t hb_decutf8sub;
extern hb_work_object_t hb_dectx3gsub;
extern hb_work_object_t hb_decssasub;
extern hb_work_object_t hb_render;
extern hb_work_object_t hb_encavcodec;
extern hb_work_object_t hb_encx264;
extern hb_work_object_t hb_enctheora;
extern hb_work_object_t hb_deca52;
extern hb_work_object_t hb_decdca;
extern hb_work_object_t hb_decavcodec;
extern hb_work_object_t hb_decavcodecv;
extern hb_work_object_t hb_decavcodecvi;
extern hb_work_object_t hb_decavcodecai;
extern hb_work_object_t hb_declpcm;
extern hb_work_object_t hb_encfaac;
extern hb_work_object_t hb_enclame;
extern hb_work_object_t hb_encvorbis;
extern hb_work_object_t hb_muxer;
extern hb_work_object_t hb_encca_aac;
extern hb_work_object_t hb_encca_haac;
extern hb_work_object_t hb_encac3;

#define FILTER_OK      0
#define FILTER_DELAY   1
#define FILTER_FAILED  2
#define FILTER_DROP    3

struct hb_filter_object_s
{
    int                     id;
    char                  * name;
    char                  * settings;

#ifdef __LIBHB__
    hb_filter_private_t* (* init)  ( int, int, int, char * );

    int                  (* work)  ( const hb_buffer_t *, hb_buffer_t **,
                                     int, int, int, hb_filter_private_t * );

    void                 (* close) ( hb_filter_private_t * );

    hb_filter_private_t   * private_data;
    //hb_buffer_t           * buffer;
#endif
};

#define HB_FILTER_DETELECINE    1
#define HB_FILTER_DEINTERLACE   2
#define HB_FILTER_DEBLOCK       3
#define HB_FILTER_DENOISE       4
#define HB_FILTER_DECOMB        5
#define HB_FILTER_ROTATE        6

extern hb_filter_object_t hb_filter_detelecine;
extern hb_filter_object_t hb_filter_deinterlace;
extern hb_filter_object_t hb_filter_deblock;
extern hb_filter_object_t hb_filter_denoise;
extern hb_filter_object_t hb_filter_decomb;
extern hb_filter_object_t hb_filter_rotate;

typedef void hb_error_handler_t( const char *errmsg );

extern void hb_register_error_handler( hb_error_handler_t * handler );

char * hb_strdup_printf( char * fmt, ... );

int hb_yuv2rgb(int yuv);
int hb_rgb2yuv(int rgb);

const char * hb_subsource_name( int source );

#endif
