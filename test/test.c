/* $Id: test.c,v 1.82 2005/11/19 08:25:54 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include <signal.h>
#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <inttypes.h>

#if defined( __MINGW32__ )
#include <conio.h>
#endif

#if defined( PTW32_STATIC_LIB )
#include <pthread.h>
#endif

#include "hb.h"
#include "parsecsv.h"

#if defined( __APPLE_CC__ )
#import <CoreServices/CoreServices.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/storage/IOMedia.h>
#include <IOKit/storage/IODVDMedia.h>
#endif

/* Options */
#if defined( __APPLE_CC__ )
#define EXTRA_VLC_DYLD_PATH "/Applications/VLC.app/Contents/MacOS/lib"
#define DEFAULT_DYLD_PATH "/usr/local/lib:/usr/lib"

static int    no_vlc_dylib = 0;
#endif
static int    debug       = HB_DEBUG_ALL;
static int    update      = 0;
static int    dvdnav      = 1;
static char * input       = NULL;
static char * output      = NULL;
static char * format      = NULL;
static int    titleindex  = 1;
static int    titlescan   = 0;
static int    main_feature = 0;
static char * native_language = NULL;
static int    native_dub  = 0;
static int    twoPass     = 0;
static int    deinterlace           = 0;
static char * deinterlace_opt       = 0;
static int    deblock               = 0;
static char * deblock_opt           = 0;
static int    denoise               = 0;
static char * denoise_opt           = 0;
static int    detelecine            = 0;
static char * detelecine_opt        = 0;
static int    decomb                = 0;
static char * decomb_opt            = 0;
static int    rotate                = 0;
static char * rotate_opt            = 0;
static int    grayscale   = 0;
static int    vcodec      = HB_VCODEC_FFMPEG_MPEG4;
static hb_list_t * audios = NULL;
static hb_audio_config_t * audio = NULL;
static int    num_audio_tracks = 0;
static char * mixdowns    = NULL;
static char * dynamic_range_compression = NULL;
static char * audio_gain  = NULL;
static char * atracks     = NULL;
static char * arates      = NULL;
static char * abitrates   = NULL;
static char * acodecs     = NULL;
static char * anames      = NULL;
#ifdef __APPLE_CC__
static int    default_acodec = HB_ACODEC_CA_AAC;
#else
static int    default_acodec = HB_ACODEC_FAAC;
#endif
static int    audio_explicit = 0;
static char ** subtracks   = NULL;
static char ** subforce    = NULL;
static char * subburn     = NULL;
static char * subdefault  = NULL;
static char ** srtfile     = NULL;
static char ** srtcodeset  = NULL;
static char ** srtoffset   = NULL;
static char ** srtlang     = NULL;
static int     srtdefault  = -1;
static int    subtitle_scan = 0;
static int    width       = 0;
static int    height      = 0;
static int    crop[4]     = { -1,-1,-1,-1 };
static int    vrate       = 0;
static float  vquality    = -1.0;
static int    vbitrate    = 0;
static int    size        = 0;
static int    mux         = 0;
static int    anamorphic_mode  = 0;
static int    modulus       = 0;
static int    par_height    = 0;
static int    par_width     = 0;
static int    display_width = 0;
static int    keep_display_aspect = 0;
static int    itu_par       = 0;
static int    angle = 0;
static int    chapter_start = 0;
static int    chapter_end   = 0;
static int    chapter_markers = 0;
static char * marker_file   = NULL;
static char	  *advanced_opts = NULL;
static char	  *advanced_opts2 = NULL;
static int	  maxHeight		= 0;
static int	  maxWidth		= 0;
static int    turbo_opts_enabled = 0;
static char * turbo_opts = "ref=1:subme=2:me=dia:analyse=none:trellis=0:no-fast-pskip=0:8x8dct=0:weightb=0";
static int    largeFileSize = 0;
static int    preset        = 0;
static char * preset_name   = 0;
static int    cfr           = 0;
static int    mp4_optimize  = 0;
static int    ipod_atom     = 0;
static int    color_matrix  = 0;
static int    preview_count = 10;
static int    store_previews = 0;
static int    start_at_preview = 0;
static int64_t start_at_pts    = 0;
static int    start_at_frame = 0;
static char * start_at_string = NULL;
static char * start_at_token = NULL;
static int64_t stop_at_pts    = 0;
static int    stop_at_frame = 0;
static char * stop_at_string = NULL;
static char * stop_at_token = NULL;
static uint64_t min_title_duration = 900000LL;

/* Exit cleanly on Ctrl-C */
static volatile int die = 0;
static void SigHandler( int );

/* Utils */
static void ShowCommands();
static void ShowHelp();
static void ShowPresets();

static int  ParseOptions( int argc, char ** argv );
static int  CheckOptions( int argc, char ** argv );
static int  HandleEvents( hb_handle_t * h );

static int get_acodec_for_string( char *codec );
static int is_sample_rate_valid(int rate);

#ifdef __APPLE_CC__
static char* bsd_name_for_path(char *path);
static int device_is_dvd(char *device);
static io_service_t get_iokit_service( char *device );
static int is_dvd_service( io_service_t service );
static int is_whole_media_service( io_service_t service );
#endif

/* Only print the "Muxing..." message once */
static int show_mux_warning = 1;

/****************************************************************************
 * hb_error_handler
 *
 * When using the CLI just display using hb_log as we always did in the past
 * make sure that we prefix with a nice ERROR message to catch peoples eyes.
 ****************************************************************************/
static void hb_cli_error_handler ( const char *errmsg )
{
    fprintf( stderr, "ERROR: %s\n", errmsg );
}

int main( int argc, char ** argv )
{
    hb_handle_t * h;
    int           build;
    char        * version;

/* win32 _IOLBF (line-buffering) is the same as _IOFBF (full-buffering).
 * force it to unbuffered otherwise informative output is not easily parsed.
 */
#if defined( _WIN32 ) || defined( __MINGW32__ )
    setvbuf( stdout, NULL, _IONBF, 0 );
    setvbuf( stderr, NULL, _IONBF, 0 );
#endif

    audios = hb_list_init();

    /* Parse command line */
    if( ParseOptions( argc, argv ) ||
        CheckOptions( argc, argv ) )
    {
        return 1;
    }

    /* Register our error handler */
    hb_register_error_handler(&hb_cli_error_handler);

    /* Init libhb */
    h = hb_init( debug, update );
    hb_dvd_set_dvdnav( dvdnav );

    /* Show version */
    fprintf( stderr, "%s - %s - %s\n",
             HB_PROJECT_TITLE, HB_PROJECT_BUILD_TITLE, HB_PROJECT_URL_WEBSITE );

    /* Check for update */
    if( update )
    {
        if( ( build = hb_check_update( h, &version ) ) > -1 )
        {
            fprintf( stderr, "You are using an old version of "
                     "HandBrake.\nLatest is %s (build %d).\n", version,
                     build );
        }
        else
        {
            fprintf( stderr, "Your version of HandBrake is up to "
                     "date.\n" );
        }
        hb_close( &h );
        hb_global_close();
        return 0;
    }

    /* Geeky */
    fprintf( stderr, "%d CPU%s detected\n", hb_get_cpu_count(),
             hb_get_cpu_count( h ) > 1 ? "s" : "" );

    /* Exit ASAP on Ctrl-C */
    signal( SIGINT, SigHandler );

    /* Feed libhb with a DVD to scan */
    fprintf( stderr, "Opening %s...\n", input );

    if (main_feature) {
        /*
         * We need to scan for all the titles in order to find the main feature
         */
        titleindex = 0;
    }

    hb_scan( h, input, titleindex, preview_count, store_previews, min_title_duration );

    /* Wait... */
    while( !die )
    {
#if defined( __MINGW32__ )
        if( _kbhit() ) {
            switch( _getch() )
            {
                case 0x03: /* ctrl-c */
                case 'q':
                    fprintf( stdout, "\nEncoding Quit by user command\n" );
                    die = 1;
                    break;
                case 'p':
                    fprintf( stdout, "\nEncoding Paused by user command, 'r' to resume\n" );
                    hb_pause( h );
                    break;
                case 'r':
                    hb_resume( h );
                    break;
                case 'h':
                    ShowCommands();
                    break;
            }
        }
        hb_snooze( 200 );
#elif !defined(SYS_BEOS)
        fd_set         fds;
        struct timeval tv;
        int            ret;
        char           buf[257];

        tv.tv_sec  = 0;
        tv.tv_usec = 100000;

        FD_ZERO( &fds );
        FD_SET( STDIN_FILENO, &fds );
        ret = select( STDIN_FILENO + 1, &fds, NULL, NULL, &tv );

        if( ret > 0 )
        {
            int size = 0;

            while( size < 256 &&
                   read( STDIN_FILENO, &buf[size], 1 ) > 0 )
            {
                if( buf[size] == '\n' )
                {
                    break;
                }
                size++;
            }

            if( size >= 256 || buf[size] == '\n' )
            {
                switch( buf[0] )
                {
                    case 'q':
                        fprintf( stdout, "\nEncoding Quit by user command\n" );
                        die = 1;
                        break;
                    case 'p':
                        fprintf( stdout, "\nEncoding Paused by user command, 'r' to resume\n" );
                        hb_pause( h );
                        break;
                    case 'r':
                        hb_resume( h );
                        break;
                    case 'h':
                        ShowCommands();
                        break;
                }
            }
        }
        hb_snooze( 200 );
#else
        hb_snooze( 200 );
#endif

        HandleEvents( h );
    }

    /* Clean up */
    hb_close( &h );
    hb_global_close();
    if( input )  free( input );
    if( output ) free( output );
    if( format ) free( format );
    if( audios )
    {
        while( ( audio = hb_list_item( audios, 0 ) ) )
        {
            hb_list_rem( audios, audio );
            if( audio->out.name )
            {
                free( audio->out.name );
            }
            free( audio );
        }
        hb_list_close( &audios );
    }
    if( mixdowns ) free( mixdowns );
    if( dynamic_range_compression ) free( dynamic_range_compression );
    if( audio_gain ) free( audio_gain );
    if( atracks ) free( atracks );
    if( arates ) free( arates );
    if( abitrates ) free( abitrates );
    if( acodecs ) free( acodecs );
    if( anames ) free( anames );
    if (native_language ) free (native_language );
	if( advanced_opts ) free (advanced_opts );
	if( advanced_opts2 ) free (advanced_opts2 );
    if (preset_name) free (preset_name);
    if( stop_at_string ) free( stop_at_string );
    if( start_at_string ) free( start_at_string );

    // write a carriage return to stdout - avoids overlap / line wrapping when stderr is redirected
    fprintf( stdout, "\n" );
    fprintf( stderr, "HandBrake has exited.\n" );

    return 0;
}

static void ShowCommands()
{
    fprintf( stdout, "\nCommands:\n" );
    fprintf( stdout, " [h]elp    Show this message\n" );
    fprintf( stdout, " [q]uit    Exit HandBrakeCLI\n" );
    fprintf( stdout, " [p]ause   Pause encoding\n" );
    fprintf( stdout, " [r]esume  Resume encoding\n" );
}

static void PrintTitleInfo( hb_title_t * title )
{
    hb_chapter_t  * chapter;
    hb_subtitle_t * subtitle;
    int i;

    fprintf( stderr, "+ title %d:\n", title->index );
    if ( title->index == title->job->feature )
    {
        fprintf( stderr, "  + Main Feature\n" );
    }
    if ( title->type == HB_STREAM_TYPE || title->type == HB_FF_STREAM_TYPE )
    {
        fprintf( stderr, "  + stream: %s\n", title->path );
    }
    else if ( title->type == HB_DVD_TYPE )
    {
        fprintf( stderr, "  + vts %d, ttn %d, cells %d->%d (%"PRIu64" blocks)\n",
                title->vts, title->ttn, title->cell_start, title->cell_end,
                title->block_count );
    }
    if (title->angle_count > 1)
        fprintf( stderr, "  + angle(s) %d\n", title->angle_count );
    fprintf( stderr, "  + duration: %02d:%02d:%02d\n",
             title->hours, title->minutes, title->seconds );
    fprintf( stderr, "  + size: %dx%d, pixel aspect: %d/%d, display aspect: %.2f, %.3f fps\n",
             title->width, title->height,
             title->pixel_aspect_width,
             title->pixel_aspect_height,
             (float) title->aspect,
             (float) title->rate / title->rate_base );
    fprintf( stderr, "  + autocrop: %d/%d/%d/%d\n", title->crop[0],
             title->crop[1], title->crop[2], title->crop[3] );
    fprintf( stderr, "  + chapters:\n" );
    for( i = 0; i < hb_list_count( title->list_chapter ); i++ )
    {
        chapter = hb_list_item( title->list_chapter, i );
        fprintf( stderr, "    + %d: cells %d->%d, %"PRIu64" blocks, duration "
                 "%02d:%02d:%02d\n", chapter->index,
                 chapter->cell_start, chapter->cell_end,
                 chapter->block_count, chapter->hours, chapter->minutes,
                 chapter->seconds );
    }
    fprintf( stderr, "  + audio tracks:\n" );
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_audio_config_item( title->list_audio, i );
        if( ( audio->in.codec == HB_ACODEC_AC3 ) || ( audio->in.codec == HB_ACODEC_DCA) )
        {
            fprintf( stderr, "    + %d, %s (iso639-2: %s), %dHz, %dbps\n", 
                     i + 1,
                     audio->lang.description, 
                     audio->lang.iso639_2,
                     audio->in.samplerate, 
                     audio->in.bitrate );
        }
        else
        {
            fprintf( stderr, "    + %d, %s (iso639-2: %s)\n", 
                     i + 1, 
                     audio->lang.description,
                     audio->lang.iso639_2 );
        }
    }
    fprintf( stderr, "  + subtitle tracks:\n" );
    for( i = 0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        subtitle = hb_list_item( title->list_subtitle, i );
        fprintf( stderr, "    + %d, %s (iso639-2: %s) (%s)(%s)\n", 
                 i + 1, subtitle->lang,
                 subtitle->iso639_2,
                 (subtitle->format == TEXTSUB) ? "Text" : "Bitmap",
                 hb_subsource_name(subtitle->source));
    }

    if(title->detected_interlacing)
    {
        /* Interlacing was found in half or more of the preview frames */
        fprintf( stderr, "  + combing detected, may be interlaced or telecined\n");
    }

}

static int test_sub_list( char ** list, int pos )
{
    int i;

    if ( list == NULL || pos == 0 )
        return 0;

    if ( list[0] == NULL && pos == 1 )
        return 1;

    for ( i = 0; list[i] != NULL; i++ )
    {
        int idx = strtol( list[i], NULL, 0 );
        if ( idx == pos )
            return 1;
    }
    return 0;
}

static int HandleEvents( hb_handle_t * h )
{
    hb_state_t s;
    int tmp_num_audio_tracks;

    hb_get_state( h, &s );
    switch( s.state )
    {
        case HB_STATE_IDLE:
            /* Nothing to do */
            break;

#define p s.param.scanning
        case HB_STATE_SCANNING:
            /* Show what title is currently being scanned */
            fprintf( stderr, "Scanning title %d", p.title_cur );
            if( !titleindex || titlescan )
                fprintf( stderr, " of %d", p.title_count );
            fprintf( stderr, "...\n" );
            break;
#undef p

        case HB_STATE_SCANDONE:
        {
            hb_list_t  * list;
            hb_title_t * title;
            hb_job_t   * job;
            int i;
            int sub_burned = 0;

            /* Audio argument string parsing variables */
            int acodec = 0;
            int abitrate = 0;
            int arate = 0;
            int mixdown = HB_AMIXDOWN_DOLBYPLII;
            double d_r_c = 0;
            /* Audio argument string parsing variables */

            list = hb_get_titles( h );

            if( !hb_list_count( list ) )
            {
                /* No valid title, stop right there */
                fprintf( stderr, "No title found.\n" );
                die = 1;
                break;
            }
	        if( main_feature )
	        {
                int i;
                int main_feature_idx=0;
                int main_feature_pos=-1;
                int main_feature_time=0;
                int title_time;

                fprintf( stderr, "Searching for main feature title...\n" );

                for( i = 0; i < hb_list_count( list ); i++ )
                {
                    title = hb_list_item( list, i );
                    title_time = (title->hours*60*60 ) + (title->minutes *60) + (title->seconds);
                    fprintf( stderr, " + Title (%d) index %d has length %dsec\n",
                             i, title->index, title_time );
                    if( main_feature_time < title_time )
                    {
                        main_feature_time = title_time;
                        main_feature_pos = i;
                        main_feature_idx = title->index;
                    }
                    if( title->job->feature == title->index )
                    {
                        main_feature_time = title_time;
                        main_feature_pos = i;
                        main_feature_idx = title->index;
                        break;
                    }
                }
                if( main_feature_pos == -1 )
                {
                    fprintf( stderr, "No main feature title found.\n" );
                    die = 1;
                    break;
                }
                titleindex = main_feature_idx;
                fprintf( stderr, "Found main feature title, setting title to %d\n",
                         main_feature_idx);

                title = hb_list_item( list, main_feature_pos);
            } else {
                title = hb_list_item( list, 0 );
            }

            if( !titleindex || titlescan )
            {
                /* Scan-only mode, print infos and exit */
                int i;
                for( i = 0; i < hb_list_count( list ); i++ )
                {
                    title = hb_list_item( list, i );
                    PrintTitleInfo( title );
                }
                die = 1;
                break;
            }

            /* Set job settings */
            job   = title->job;

            PrintTitleInfo( title );

            if( chapter_start && chapter_end && !stop_at_pts && !start_at_preview && !stop_at_frame && !start_at_pts && !start_at_frame )
            {
                job->chapter_start = MAX( job->chapter_start,
                                          chapter_start );
                job->chapter_end   = MIN( job->chapter_end,
                                          chapter_end );
                job->chapter_end   = MAX( job->chapter_start,
                                          job->chapter_end );
            }

            if ( angle )
            {
                job->angle = angle;
            }

            if (preset)
            {
                fprintf( stderr, "+ Using preset: %s\n", preset_name);

                if (!strcmp(preset_name, "Universal"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					vcodec = HB_VCODEC_X264;
					job->vquality = 20.0;
					if( !atracks )
					{
						atracks = strdup("1,1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac,copy:ac3");
					}
					if( !abitrates )
					{
						abitrates = strdup("160,160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2,auto");
					}
					if( !arates )
					{
						arates = strdup("Auto,Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0,0.0");
					}
					maxWidth = 720;
					if( !advanced_opts )
					{
						advanced_opts = strdup("cabac=0:ref=2:me=umh:bframes=0:weightp=0:8x8dct=0:trellis=0:subme=6");
					}
					if( !anamorphic_mode )
					{
						anamorphic_mode = 2;
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "iPod"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					job->ipod_atom = 1;
					vcodec = HB_VCODEC_X264;
					job->vbitrate = 700;
					if( !atracks )
					{
						atracks = strdup("1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac");
					}
					if( !abitrates )
					{
						abitrates = strdup("160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2");
					}
					if( !arates )
					{
						arates = strdup("Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0");
					}
					maxWidth = 320;
					if( !advanced_opts )
					{
						advanced_opts = strdup("level=30:bframes=0:weightp=0:cabac=0:ref=1:vbv-maxrate=768:vbv-bufsize=2000:analyse=all:me=umh:no-fast-pskip=1:subme=6:8x8dct=0:trellis=0");
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "iPhone & iPod Touch"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					vcodec = HB_VCODEC_X264;
					job->vquality = 20.0;
					if( !atracks )
					{
						atracks = strdup("1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac");
					}
					if( !abitrates )
					{
						abitrates = strdup("128");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2");
					}
					if( !arates )
					{
						arates = strdup("Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0");
					}
					maxWidth = 480;
					if( !advanced_opts )
					{
						advanced_opts = strdup("cabac=0:ref=2:me=umh:bframes=0:weightp=0:subme=6:8x8dct=0:trellis=0");
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "iPhone 4"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					job->largeFileSize = 1;
					vcodec = HB_VCODEC_X264;
					job->vquality = 20.0;
					job->vrate_base = 900900;
					job->cfr = 2;
					if( !atracks )
					{
						atracks = strdup("1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac");
					}
					if( !abitrates )
					{
						abitrates = strdup("160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2");
					}
					if( !arates )
					{
						arates = strdup("Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0");
					}
					maxWidth = 960;
					if( !anamorphic_mode )
					{
						anamorphic_mode = 2;
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "iPad"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					job->largeFileSize = 1;
					vcodec = HB_VCODEC_X264;
					job->vquality = 20.0;
					job->vrate_base = 900900;
					job->cfr = 2;
					if( !atracks )
					{
						atracks = strdup("1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac");
					}
					if( !abitrates )
					{
						abitrates = strdup("160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2");
					}
					if( !arates )
					{
						arates = strdup("Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0");
					}
					maxWidth = 1024;
					if( !anamorphic_mode )
					{
						anamorphic_mode = 2;
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "AppleTV"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					job->largeFileSize = 1;
					vcodec = HB_VCODEC_X264;
					job->vquality = 20.0;
					if( !atracks )
					{
						atracks = strdup("1,1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac,copy:ac3");
					}
					if( !abitrates )
					{
						abitrates = strdup("160,160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2,auto");
					}
					if( !arates )
					{
						arates = strdup("Auto,Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0,0.0");
					}
					maxWidth = 960;
					if( !advanced_opts )
					{
						advanced_opts = strdup("cabac=0:ref=2:me=umh:b-pyramid=none:b-adapt=2:weightb=0:trellis=0:weightp=0:vbv-maxrate=9500:vbv-bufsize=9500");
					}
					if( !anamorphic_mode )
					{
						anamorphic_mode = 2;
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "AppleTV 2"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					job->largeFileSize = 1;
					vcodec = HB_VCODEC_X264;
					job->vquality = 20.0;
					job->vrate_base = 900900;
					job->cfr = 2;
					if( !atracks )
					{
						atracks = strdup("1,1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac,copy:ac3");
					}
					if( !abitrates )
					{
						abitrates = strdup("160,160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2,auto");
					}
					if( !arates )
					{
						arates = strdup("Auto,Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0,0.0");
					}
					maxWidth = 1280;
					if( !anamorphic_mode )
					{
						anamorphic_mode = 2;
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "Normal"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					vcodec = HB_VCODEC_X264;
					job->vquality = 20.0;
					if( !atracks )
					{
						atracks = strdup("1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac");
					}
					if( !abitrates )
					{
						abitrates = strdup("160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2");
					}
					if( !arates )
					{
						arates = strdup("Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0");
					}
					if( !advanced_opts )
					{
						advanced_opts = strdup("ref=2:bframes=2:subme=6:mixed-refs=0:weightb=0:8x8dct=0:trellis=0");
					}
					if( !anamorphic_mode )
					{
						anamorphic_mode = 1;
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "High Profile"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					vcodec = HB_VCODEC_X264;
					job->vquality = 20.0;
					if( !atracks )
					{
						atracks = strdup("1,1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac,copy:ac3");
					}
					if( !abitrates )
					{
						abitrates = strdup("160,160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2,auto");
					}
					if( !arates )
					{
						arates = strdup("Auto,Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0,0.0");
					}
					if( !advanced_opts )
					{
						advanced_opts = strdup("b-adapt=2:rc-lookahead=50");
					}
					detelecine = 1;
					decomb = 1;
					if( !anamorphic_mode )
					{
						anamorphic_mode = 2;
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "Classic"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					job->vbitrate = 1000;
					if( !atracks )
					{
						atracks = strdup("1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac");
					}
					if( !abitrates )
					{
						abitrates = strdup("160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2");
					}
					if( !arates )
					{
						arates = strdup("Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0");
					}
					
				}
				
				if (!strcmp(preset_name, "AppleTV Legacy"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					job->largeFileSize = 1;
					vcodec = HB_VCODEC_X264;
					job->vbitrate = 2500;
					if( !atracks )
					{
						atracks = strdup("1,1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac,copy:ac3");
					}
					if( !abitrates )
					{
						abitrates = strdup("160,160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2,auto");
					}
					if( !arates )
					{
						arates = strdup("Auto,Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0,0.0");
					}
					if( !advanced_opts )
					{
						advanced_opts = strdup("ref=1:b-pyramid=none:weightp=0:subme=5:me=umh:no-fast-pskip=1:cabac=0:weightb=0:8x8dct=0:trellis=0");
					}
					if( !anamorphic_mode )
					{
						anamorphic_mode = 1;
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "iPhone Legacy"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					job->ipod_atom = 1;
					vcodec = HB_VCODEC_X264;
					job->vbitrate = 960;
					if( !atracks )
					{
						atracks = strdup("1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac");
					}
					if( !abitrates )
					{
						abitrates = strdup("128");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2");
					}
					if( !arates )
					{
						arates = strdup("Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0");
					}
					maxWidth = 480;
					if( !advanced_opts )
					{
						advanced_opts = strdup("level=30:cabac=0:ref=1:analyse=all:me=umh:no-fast-pskip=1:psy-rd=0,0:bframes=0:weightp=0:subme=6:8x8dct=0:trellis=0");
					}
					job->chapter_markers = 1;
					
				}
				
				if (!strcmp(preset_name, "iPod Legacy"))
				{
					if( !mux )
					{
						mux = HB_MUX_MP4;
					}
					job->ipod_atom = 1;
					vcodec = HB_VCODEC_X264;
					job->vbitrate = 1500;
					if( !atracks )
					{
						atracks = strdup("1");
					}
					if( !acodecs )
					{
						acodecs = strdup("faac");
					}
					if( !abitrates )
					{
						abitrates = strdup("160");
					}
					if( !mixdowns )
					{
						mixdowns = strdup("dpl2");
					}
					if( !arates )
					{
						arates = strdup("Auto");
					}
					if( !dynamic_range_compression )
					{
						dynamic_range_compression = strdup("0.0");
					}
					maxWidth = 640;
					if( !advanced_opts )
					{
						advanced_opts = strdup("level=30:bframes=0:weightp=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:no-fast-pskip=1:psy-rd=0,0:subme=6:8x8dct=0:trellis=0");
					}
					job->chapter_markers = 1;
					
				}
						
            }

			if ( chapter_markers )
			{
				job->chapter_markers = chapter_markers;

                if( marker_file != NULL )
                {
                    hb_csv_file_t * file = hb_open_csv_file( marker_file );
                    hb_csv_cell_t * cell;
                    int row = 0;
                    int chapter = 0;

                    fprintf( stderr, "Reading chapter markers from file %s\n", marker_file );

                    if( file == NULL )
                    {
                         fprintf( stderr, "Cannot open chapter marker file, using defaults\n" );
                    }
                    else
                    {
                        /* Parse the cells */
                        while( NULL != ( cell = hb_read_next_cell( file ) ) )
                        {
                            /* We have a chapter number */
                            if( cell->cell_col == 0 )
                            {
                                row = cell->cell_row;
                                chapter = atoi( cell->cell_text );
                            }

                            /* We have a chapter name */
                            if( cell->cell_col == 1 && row == cell->cell_row )
                            {
                                /* If we have a valid chapter, copy the string an terminate it */
                                if( chapter >= job->chapter_start && chapter <= job->chapter_end )
                                {
                                    hb_chapter_t * chapter_s;

                                    chapter_s = hb_list_item( job->title->list_chapter, chapter - 1);
                                    strncpy(chapter_s->title, cell->cell_text, 1023);
                                    chapter_s->title[1023] = '\0';
                                }
                            }


                            hb_dispose_cell( cell );
                        }

                        hb_close_csv_file( file );
                    }
                }
                else
                {
                    /* No marker file */

                    int number_of_chapters = hb_list_count(job->title->list_chapter);
                    int chapter;

                    for(chapter = 0; chapter <= number_of_chapters - 1 ; chapter++)
                    {
                        hb_chapter_t * chapter_s;
                        chapter_s = hb_list_item( job->title->list_chapter, chapter);
                        snprintf( chapter_s->title, 1023, "Chapter %i", chapter + 1 );
                        chapter_s->title[1023] = '\0';
                    }
                }
			}

            if( crop[0] >= 0 && crop[1] >= 0 &&
                crop[2] >= 0 && crop[3] >= 0 )
            {
                memcpy( job->crop, crop, 4 * sizeof( int ) );
            }

            job->deinterlace = deinterlace;
            job->grayscale   = grayscale;
            
            /* Add selected filters */
            job->filters = hb_list_init();
            
            if( rotate )
            {
                hb_filter_rotate.settings = rotate_opt;
                hb_list_add( job->filters, &hb_filter_rotate);
            }
            if( detelecine )
            {
                hb_filter_detelecine.settings = detelecine_opt;
                hb_list_add( job->filters, &hb_filter_detelecine );
            }
            if( decomb )
            {
                hb_filter_decomb.settings = decomb_opt;
                hb_list_add( job->filters, &hb_filter_decomb );
            }
            if( deinterlace )
            {
                hb_filter_deinterlace.settings = deinterlace_opt;
                hb_list_add( job->filters, &hb_filter_deinterlace );
            }
            if( deblock )
            {
                hb_filter_deblock.settings = deblock_opt;
                hb_list_add( job->filters, &hb_filter_deblock );
            }
            if( denoise )
            {
                hb_filter_denoise.settings = denoise_opt;
                hb_list_add( job->filters, &hb_filter_denoise );
            }

            switch( anamorphic_mode )
            {
                case 0: // Non-anamorphic
                    
                    if (modulus)
                    {
                        job->modulus = modulus;
                    }
                    
                    if( width && height )
                    {
                        job->width  = width;
                        job->height = height;
                    }
                    else if( width )
                    {
                        job->width = width;
                        // do not exceed source dimensions by default
                        if( !maxHeight )
                            job->maxHeight = title->height;
                        hb_fix_aspect( job, HB_KEEP_WIDTH );
                    }
                    else if( height )
                    {
                        job->height = height;
                        // do not exceed source dimensions by default
                        if( !maxWidth )
                            job->maxWidth = title->width;
                        hb_fix_aspect( job, HB_KEEP_HEIGHT );
                    }
                    else if( !width && !height )
                    {
                        /* Default to cropped width when one isn't specified
                         * avoids rounding to mod 16 regardless of modulus */
                        job->width = title->width - job->crop[2] - job->crop[3];
                        // do not exceed source dimensions by default
                        if( !maxHeight )
                            job->maxHeight = title->height;
                        hb_fix_aspect( job, HB_KEEP_WIDTH );
                    }

                break;
                
                case 1: // Strict anammorphic
                    job->anamorphic.mode = anamorphic_mode;
                break;
                
                case 2: // Loose anamorphic
                    job->anamorphic.mode = 2;
                    
                    if (modulus)
                    {
                        job->modulus = modulus;
                    }
                    
                    if( itu_par )
                    {
                        job->anamorphic.itu_par = itu_par;
                    }
                    
                    if( width )
                    {
                        job->width = width;
                    }
                    else if( !width && !height )
                    {
                        /* Default to full width when one isn't specified for loose anamorphic */
                        job->width = title->width - job->crop[2] - job->crop[3];
                    }
                    
                break;
                
                case 3: // Custom Anamorphic 3: Power User Jamboree 
                    job->anamorphic.mode = 3;
                    
                    if (modulus)
                    {
                        job->modulus = modulus;
                    }
                    
                    if( itu_par )
                    {
                        job->anamorphic.itu_par = itu_par;
                    }
                    
                    if( par_width && par_height )
                    {
                        job->anamorphic.par_width = par_width;
                        job->anamorphic.par_height = par_height;
                    }
                    
                    if( keep_display_aspect )
                    {
                        job->anamorphic.keep_display_aspect = 1;
                        
                        /* First, what *is* the display aspect? */
                        int cropped_width = title->width - job->crop[2] - job->crop[3];
                        int cropped_height = title->height - job->crop[0] - job->crop[1];
                        
                        /* XXX -- I'm assuming people want to keep the source
                           display AR even though they might have already
                           asked for ITU values instead. */
                        float source_display_width = (float)cropped_width *
                            (float)title->pixel_aspect_width / (float)title->pixel_aspect_height;
                        float display_aspect = source_display_width / (float)cropped_height;
                        /* When keeping display aspect, we have to rank some values
                           by priority in order to consistently handle situations
                           when more than one might be specified by default.
                           
                           * First off, PAR gets ignored. (err make this reality)
                           * Height will be respected over all other settings,
                           * If it isn't set, display_width will be followed.
                           * If it isn't set, width will be followed.          */
                        if( height )
                        {
                            /* We scale the display width to the new height */
                            display_width = (int)( (double)height * display_aspect );
                        }
                        else if( display_width )
                        {
                            /* We scale the height to the new display width */
                            height = (int)( (double)display_width / display_aspect );
                        }
                    }
                    
                    if( display_width )
                    {
                        /* Adjust the PAR to create the new display width
                           from the default job width. */
                        job->anamorphic.dar_width = display_width;
                        
                        job->anamorphic.dar_height = height ?
                                                        height :
                                                        title->height - job->crop[0] - job->crop[1];
                    }
                    
                    if( width && height )
                    {
                        /* Use these storage dimensions */
                        job->width  = width;
                        job->height = height;
                    }
                    else if( width )
                    {
                        /* Use just this storage width */
                        job->width = width;
                        job->height = title->height - job->crop[0] - job->crop[1];
                    }
                    else if( height )
                    {
                        /* Use just this storage height. */
                        job->height = height;
                        job->width = title->width - job->crop[2] - job->crop[3];
                    }
                    else if( !width && !height )
                    {
                        /* Assume source dimensions after cropping. */
                        job->width = title->width - job->crop[2] - job->crop[3];
                        job->height = title->height - job->crop[0] - job->crop[1];
                    }
                    
                break;
            }

            if( vquality >= 0.0 )
            {
                job->vquality = vquality;
                job->vbitrate = 0;
            }
            else if( vbitrate )
            {
                job->vquality = -1.0;
                job->vbitrate = vbitrate;
            }
            if( vcodec )
            {
                job->vcodec = vcodec;
            }
            if( vrate )
            {
                job->cfr = cfr;
                job->vrate = 27000000;
                job->vrate_base = vrate;
            }
            else if ( cfr )
            {
                // cfr or pfr flag with no rate specified implies
                // use the title rate.
                job->cfr = cfr;
                job->vrate = title->rate;
                job->vrate_base = title->rate_base;
            }

            /* Grab audio tracks */
            if( atracks )
            {
                char * token = strtok(atracks, ",");
                if (token == NULL)
                    token = optarg;
                int track_start, track_end;
                while( token != NULL )
                {
                    audio = calloc(1, sizeof(*audio));
                    hb_audio_config_init(audio);
                    if (strlen(token) >= 3)
                    {
                        if (sscanf(token, "%d-%d", &track_start, &track_end) == 2)
                        {
                            int i;
                            for (i = track_start - 1; i < track_end; i++)
                            {
                                if (i != track_start - 1)
                                {
                                    audio = calloc(1, sizeof(*audio));
                                    hb_audio_config_init(audio);
                                }
                                audio->in.track = i;
                                audio->out.track = num_audio_tracks++;
                                hb_list_add(audios, audio);
                            }
                        }
                        else if( !strcasecmp(token, "none" ) )
                        {
                            audio->in.track = audio->out.track = -1;
                            audio->out.codec = 0;
                            hb_list_add(audios, audio);
                            break;
                        }
                        else
                        {
                            fprintf(stderr, "ERROR: Unable to parse audio input \"%s\", skipping.\n",
                                    token);
                            free(audio);
                        }
                    }
                    else
                    {
                        audio->in.track = atoi(token) - 1;
                        audio->out.track = num_audio_tracks++;
                        hb_list_add(audios, audio);
                    }
                    token = strtok(NULL, ",");
                }
            }

            /* Parse audio tracks */
            if( native_language && native_dub )
            {
                if( hb_list_count( audios ) == 0 || !audio_explicit )
                {
                    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
                    {
                        char audio_lang[4];
                        int track = i;
                        
                        audio = hb_list_audio_config_item( title->list_audio, i );
                        
                        strncpy( audio_lang, audio->lang.iso639_2, sizeof( audio_lang ) );
                        
                        if( strncasecmp( native_language, audio_lang, 
                                         sizeof( audio_lang ) ) == 0 &&
                            audio->lang.type != 3 && // Directors 1
                            audio->lang.type != 4)   // Directors 2
                        {
                            /*
                             * Matched an audio to our native language - use it.
                             * Replace any existing audio tracks that a preset may
                             * have put here.
                             */
                            if( hb_list_count(audios) == 0) {
                                audio = calloc(1, sizeof(*audio));
                                hb_audio_config_init(audio);
                                audio->in.track = track;
                                audio->out.track = num_audio_tracks++;
                                /* Add it to our audios */
                                hb_list_add(audios, audio);
                            } else {
                                /*
                                 * Update the track numbers on what is already in
                                 * there.
                                 */
                                for( i=0; i < hb_list_count( audios ); i++ )
                                {
                                    audio = hb_list_item( audios, i );

                                    audio->in.track = track;
                                }
                            }
                            break;
                        }
                    }
                } else {
                    fprintf( stderr, "Warning: Native language (dubbing) selection ignored since an audio track has already been selected\n");
                }
            }

            if( hb_list_count(audios) == 0 &&
                hb_list_count(job->title->list_audio) > 0 )
            {        
                /* Create a new audio track with default settings */
                audio = calloc(1, sizeof(*audio));
                hb_audio_config_init(audio);
                /* Add it to our audios */
                hb_list_add(audios, audio);
            }

            tmp_num_audio_tracks = num_audio_tracks = hb_list_count(audios);
            for (i = 0; i < tmp_num_audio_tracks; i++)
            {
                audio = hb_list_item(audios, 0);
                if( (audio == NULL) || (audio->in.track == -1) ||
                    (audio->out.track == -1) || (audio->out.codec == 0) )
                {
                    num_audio_tracks--;
                }
                else
                {
                    if( hb_audio_add( job, audio ) == 0 )
                    {
                        fprintf(stderr, "ERROR: Invalid audio input track '%u', exiting.\n", 
                                audio->in.track + 1 );
                        num_audio_tracks--;
                        exit(3);
                    }
                }
                hb_list_rem(audios, audio);
                if( audio != NULL)
                    if( audio->out.name )
                    {
                        free( audio->out.name);
                    }
                    free( audio );
            }

            /* Audio Codecs */
            i = 0;
            if( acodecs )
            {
                char * token = strtok(acodecs, ",");
                if( token == NULL )
                    token = acodecs;
                while ( token != NULL )
                {
                    if ((acodec = get_acodec_for_string(token)) == -1)
                    {
                        fprintf(stderr, "Invalid codec %s, using default for container.\n", token);
                        acodec = default_acodec;
                    }
                    if( i < num_audio_tracks )
                    {
                        audio = hb_list_audio_config_item(job->list_audio, i);
                        audio->out.codec = acodec;
                    }
                    else
                    {
                        hb_audio_config_t * last_audio = hb_list_audio_config_item( job->list_audio, i - 1 );
                        hb_audio_config_t audio;

                        if( last_audio )
                        {
                            fprintf(stderr, "More audio codecs than audio tracks, copying track %i and using encoder %s\n",
                                    i, token);
                            hb_audio_config_init(&audio);
                            audio.in.track = last_audio->in.track;
                            audio.out.track = num_audio_tracks++;
                            audio.out.codec = acodec;
                            hb_audio_add(job, &audio);
                        }
                        else
                        {
                            fprintf(stderr, "Audio codecs and no valid audio tracks, skipping codec %s\n", token);
                        }
                    }
                    token = strtok(NULL, ",");
                    i++;
                }
            }
            if( i < num_audio_tracks )
            {
                /* We have fewer inputs than audio tracks, use the default codec for
                 * this container for the remaining tracks. Unless we only have one input
                 * then use that codec instead.
                 */
                if (i != 1)
                    acodec = default_acodec;
                for ( ; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    audio->out.codec = acodec;
                }
            }
            /* Audio Codecs */

            /* Sample Rate */
            i = 0;
            if( arates )
            {
                char * token = strtok(arates, ",");
                if (token == NULL)
                    token = arates;
                while ( token != NULL )
                {
                    arate = atoi(token);
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    int j;

                    for( j = 0; j < hb_audio_rates_count; j++ )
                    {
                        if( !strcmp( token, hb_audio_rates[j].string ) )
                        {
                            arate = hb_audio_rates[j].rate;
                            break;
                        }
                    }

                    if( audio != NULL )
                    {
                        if ( !strcasecmp( token, "auto" ) )
                        {
                            arate = audio->in.samplerate;
                        }
                        if (!is_sample_rate_valid(arate))
                        {
                            fprintf(stderr, "Invalid sample rate %d, using input rate %d\n", arate, audio->in.samplerate);
                            arate = audio->in.samplerate;
                        }
                        
                        audio->out.samplerate = arate;
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more inputs than audio tracks, oops */
                    }
                    else 
                    {
                        fprintf(stderr, "Ignoring sample rate %d, no audio tracks\n", arate);
                    }
                    token = strtok(NULL, ",");
                }
            }
            if (i < num_audio_tracks)
            {
                /* We have fewer inputs than audio tracks, use default sample rate.
                 * Unless we only have one input, then use that for all tracks.
                 */
                if (i != 1)
                    arate = audio->in.samplerate;
                for ( ; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    audio->out.samplerate = arate;
                }
            }
            /* Sample Rate */
            
            /* Audio Mixdown */
            i = 0;
            if ( mixdowns )
            {
                char * token = strtok(mixdowns, ",");
                if (token == NULL)
                    token = mixdowns;
                while ( token != NULL )
                {
                    mixdown = hb_mixdown_get_mixdown_from_short_name(token);
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    if( audio != NULL )
                    {
                        audio->out.mixdown = mixdown;
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more inputs than audio tracks, oops */
                    }
                    else
                    {
                        fprintf(stderr, "Ignoring mixdown, no audio tracks\n");
                    }
                    token = strtok(NULL, ",");
                }
            }
            if (i < num_audio_tracks)
            {
                /* We have fewer inputs than audio tracks, use the default mixdown for the rest. Unless
                 * we only have one input, then use that.
                 */
                int use_default = 0;
                if (i != 1)
                    use_default = 1;

                for (; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    if (use_default)
                    {
                        // Get default for this tracks codec and layout
                        mixdown = hb_get_default_mixdown( audio->out.codec, audio->in.channel_layout );
                    }
                    audio->out.mixdown = mixdown;
                }
            }
            /* Audio Mixdown */

            /* Audio Bitrate */
            i = 0;
            if( abitrates )
            {
                char * token = strtok(abitrates, ",");
                if (token == NULL)
                    token = abitrates;
                while ( token != NULL )
                {
                    abitrate = atoi(token);
                    audio = hb_list_audio_config_item(job->list_audio, i);

                    if( audio != NULL )
                    {
                        audio->out.bitrate = abitrate;
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more inputs than audio tracks, oops */
                    }
                    else 
                    {
                        fprintf(stderr, "Ignoring bitrate %d, no audio tracks\n", abitrate);
                    }
                    token = strtok(NULL, ",");
                }
            }
            if (i < num_audio_tracks)
            {
                /* We have fewer inputs than audio tracks, use the default bitrate
                 * for the remaining tracks. Unless we only have one input, then use
                 * that for all tracks.
                 */
                int use_default = 0;
                if (i != 1)
                    use_default = 1;

                for (; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    if (use_default)
                    {
                        abitrate = hb_get_default_audio_bitrate( 
                                        audio->out.codec, audio->out.samplerate,
                                        audio->out.mixdown );
                    }
                    audio->out.bitrate = abitrate;
                }
            }
            /* Audio Bitrate */

            /* Audio DRC */
            i = 0;
            if ( dynamic_range_compression )
            {
                char * token = strtok(dynamic_range_compression, ",");
                if (token == NULL)
                    token = dynamic_range_compression;
                while ( token != NULL )
                {
                    d_r_c = atof(token);
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    if( audio != NULL )
                    {
                        audio->out.dynamic_range_compression = d_r_c;
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more inputs than audio tracks, oops */
                    } 
                    else
                    {
                        fprintf(stderr, "Ignoring drc, no audio tracks\n");
                    }
                    token = strtok(NULL, ",");
                }
            }
            if (i < num_audio_tracks)
            {
                /* We have fewer inputs than audio tracks, use no DRC for the remaining
                 * tracks. Unless we only have one input, then use the same DRC for all
                 * tracks.
                 */
                if (i != 1)
                    d_r_c = 0;
                for (; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    audio->out.dynamic_range_compression = d_r_c;
                }
            }
            /* Audio DRC */

            /* Audio Gain */
            i = 0;
            if ( audio_gain )
            {
                double gain;
                char * token = strtok(audio_gain, ",");
                if (token == NULL)
                    token = audio_gain;
                while ( token != NULL )
                {
                    gain = atof(token);
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    if( audio != NULL )
                    {
                        audio->out.gain = gain;
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more inputs than audio tracks, oops */
                    } 
                    else
                    {
                        fprintf(stderr, "Ignoring gain, no audio tracks\n");
                    }
                    token = strtok(NULL, ",");
                }
            }
            /* Audio Gain */

            /* Audio Track Names */
            i = 0;
            if ( anames )
            {
                char * token = strtok(anames, ",");
                if (token == NULL)
                    token = anames;
                while ( token != NULL )
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    if( audio != NULL )
                    {
                        audio->out.name = strdup(token);
                        if( (++i) >= num_audio_tracks )
                            break;  /* We have more names than audio tracks, oops */
                    }
                    else
                    {
                        fprintf(stderr, "Ignoring aname '%s', no audio track\n",
                                token);
                    }
                    token = strtok(NULL, ",");
                }
            }
            if( i < num_audio_tracks && i == 1 )
            {
                /* We have exactly one name and more than one audio track. Use the same
                 * name for all tracks. */
                for ( ; i < num_audio_tracks; i++)
                {
                    audio = hb_list_audio_config_item(job->list_audio, i);
                    audio->out.name = strdup(anames);
                }
            }
            /* Audio Track Names */

            /* Fix up passthru that needs to fallback to ac3 encode */
            for( i = 0; i < hb_list_count( job->list_audio ); )
            {
                audio = hb_list_audio_config_item( job->list_audio, i );
                if ( ( audio->out.codec & HB_ACODEC_AC3 ) &&
                     ( audio->out.codec & HB_ACODEC_PASS_FLAG ) &&
                    !( audio->out.codec & audio->in.codec ) )
                {
                    // AC3 passthru not possible, fallback to AC3 encoder.
                    fprintf( stderr, "AC3 passthru requested and input codec is not AC3 for track %d, using AC3 encoder\n",
                        audio->out.track );
                    audio->out.codec = HB_ACODEC_AC3;
                    audio->out.mixdown = hb_get_default_mixdown( audio->out.codec, audio->in.channel_layout );
                    audio->out.bitrate = hb_get_default_audio_bitrate( audio->out.codec, audio->out.samplerate,
                        audio->out.mixdown );
                }
                // fix 'copy' to select a specific codec
                if ( audio->out.codec & HB_ACODEC_PASS_FLAG )
                {
                    audio->out.codec &= (audio->in.codec | HB_ACODEC_PASS_FLAG);
                    if ( !( audio->out.codec & HB_ACODEC_MASK ) )
                    {
                        // Passthru not possible, drop audio.
                        fprintf( stderr, "Passthru requested and input codec is not the same as output codec for track %d, dropping track\n",
                            audio->out.track );
                        hb_audio_t * item = hb_list_item( job->list_audio, i );
                        hb_list_rem( job->list_audio, item );
                        continue;
                    }
                }
                i++;
            }

            if( size )
            {
                job->vbitrate = hb_calc_bitrate( job, size );
                fprintf( stderr, "Calculated bitrate: %d kbps\n",
                         job->vbitrate );
            }

            if( subtracks )
            {
                char * token;
                int    i;
                int    burnpos = 0, defaultpos = 0;

                if ( subburn )
                    burnpos = strtol( subburn, NULL, 0 );
                if ( subdefault )
                    defaultpos = strtol( subdefault, NULL, 0 );
                for ( i = 0; subtracks[i] != NULL; i++ )
                {
                    token = subtracks[i];
                    if( strcasecmp(token, "scan" ) == 0 )
                    {
                        int burn = 0, force = 0, def = 0;

                        if ( subburn != NULL )
                        {
                            burn = ( i == 0 && subburn[0] == 0 ) ||
                                   ( burnpos == i+1 );
                        }
                        if ( subdefault != NULL )
                        {
                            def =  ( i == 0 && subdefault[0] == 0 ) ||
                                   ( defaultpos == i+1 );
                        }
                        force = test_sub_list( subforce, i+1 );

                        if ( !burn )
                        {
                            job->select_subtitle_config.dest = PASSTHRUSUB;
                        }
                        else
                        {
                            if ( sub_burned )
                            {
                                continue;
                            }
                            sub_burned = 1;
                        }
                        job->select_subtitle_config.force = force;
                        job->select_subtitle_config.default_track = def;
                        subtitle_scan = 1;
                    }
                    else
                    {
                        hb_subtitle_t        * subtitle;
                        hb_subtitle_config_t   sub_config;
                        int                    track;
                        int                    burn = 0, force = 0, def = 0;

                        track = atoi(token) - 1;
                        subtitle = hb_list_item(title->list_subtitle, track);
                        if( subtitle == NULL ) 
                        {
                            fprintf( stderr, "Warning: Could not find subtitle track %d, skipped\n", track+1 );
                            continue;
                        }
                        sub_config = subtitle->config;

                        if ( subburn != NULL )
                        {
                            burn = ( i == 0 && subburn[0] == 0 ) ||
                                   ( burnpos == i+1 );
                        }
                        if ( subdefault != NULL )
                        {
                            def =  ( i == 0 && subdefault[0] == 0 ) ||
                                   ( defaultpos == i+1 );
                        }

                        force = test_sub_list(subforce, i+1);
                        
                        int supports_burn =
                            ( subtitle->source == VOBSUB ) ||
                            ( subtitle->source == SSASUB );
                        
                        if ( burn && supports_burn )
                        {
                            // Only allow one subtitle to be burned into video
                            if ( sub_burned )
                            {
                                fprintf( stderr, "Warning: Skipping subtitle track %d, can't have more than one track burnt in\n", track+1 );
                                continue;
                            }
                            sub_burned = 1;
                            
                            // Mark as burn-in
                            sub_config.dest = RENDERSUB;
                        }
                        else
                        {
                            sub_config.dest = PASSTHRUSUB;
                        }
                        sub_config.force = force;
                        sub_config.default_track = def;
                        hb_subtitle_add( job, &sub_config, track );
                    }
                }
            }

            if( srtfile )
            {
                char * token;
                int i;
                hb_subtitle_config_t sub_config;

                for( i=0; srtfile[i] != NULL; i++ )
                {
                    char *codeset = "L1";
                    int64_t offset = 0;
                    char *lang = "und";

                    token = srtfile[i];
                    if( srtcodeset && srtcodeset[i] )
                    {
                        codeset = srtcodeset[i];
                    }
                    if( srtoffset && srtoffset[i] )
                    {
                        offset = strtoll( srtoffset[i], &srtoffset[i], 0 );
                    }
                    if ( srtlang && srtlang[i] )
                    {
                        lang = srtlang[i];
                    }
                    sub_config.default_track = 
                           ( srtdefault != -1 ) && ( srtdefault == i + 1 );
                    sub_config.force = 0;
                    strncpy( sub_config.src_filename, srtfile[i], 255);
                    sub_config.src_filename[255] = 0;
                    strncpy( sub_config.src_codeset, codeset, 39);
                    sub_config.src_codeset[39] = 0;
                    sub_config.offset = offset;

                    hb_srt_add( job, &sub_config, lang);
                }
            }

            if( native_language )
            {
                char audio_lang[4];
                
                audio = hb_list_audio_config_item(job->list_audio, 0);
                
                if( audio ) 
                {
                    strncpy( audio_lang, audio->lang.iso639_2, sizeof( audio_lang ) );
                    
                    if( strncasecmp( native_language, audio_lang, 
                                     sizeof( audio_lang ) ) != 0 )
                    {
                        /*
                         * Audio language is not the same as our native language. 
                         * If we have any subtitles in our native language they
                         * should be selected here if they haven't already been.
                         */
                        hb_subtitle_t *subtitle, *subtitle2 = NULL;
                        int matched_track = 0;

                        for( i = 0; i < hb_list_count( title->list_subtitle ); i++ )
                        {
                            subtitle = hb_list_item( title->list_subtitle, i );
                            matched_track = i;
                            if( strcmp( subtitle->iso639_2, native_language ) == 0 )
                            {  
                                /*
                                 * Found the first matching subtitle in our
                                 * native language. Is it already selected?
                                 */
                                for( i = 0; i < hb_list_count( job->list_subtitle ); i++ )
                                {
                                    subtitle2 =  hb_list_item( job->list_subtitle, i );
                                    
                                    if( subtitle2->track == subtitle->track) {
                                        /*
                                         * Already selected
                                         */
                                        break;
                                    }
                                    subtitle2 = NULL;
                                }
                                
                                if( subtitle2 == NULL ) 
                                {
                                    /*
                                     * Not already selected, so select it.
                                     */
                                    hb_subtitle_config_t sub_config;

                                    if( native_dub )
                                    {
                                        fprintf( stderr, "Warning: no matching audio for native language - using subtitles instead.\n");
                                    }
                                    sub_config = subtitle->config;

                                    if( mux == HB_MUX_MKV || subtitle->format == TEXTSUB)
                                    {
                                        sub_config.dest = PASSTHRUSUB;
                                    }

                                    sub_config.force = 0;
                                    sub_config.default_track = 1;
                                    hb_subtitle_add( job, &sub_config, matched_track);
                                }
                                /*
                                 * Stop searching.
                                 */
                                break;
                            }
                        }
                    }
                }
            }

            if( job->mux )
            {
                job->mux = mux;
            }

            if ( largeFileSize )
            {
                job->largeFileSize = 1;
            }
            if ( mp4_optimize )
            {
                job->mp4_optimize = 1;
            }
            if ( ipod_atom )
            {
                job->ipod_atom = 1;
            }

            job->file = strdup( output );

            if( color_matrix )
            {
                job->color_matrix = color_matrix;
            }

            if( advanced_opts != NULL && *advanced_opts != '\0' )
            {
                job->advanced_opts = advanced_opts;
            }
            else /*avoids a bus error crash when options aren't specified*/
            {
                job->advanced_opts =  NULL;
            }
            if (maxWidth)
                job->maxWidth = maxWidth;
            if (maxHeight)
                job->maxHeight = maxHeight;

            if( start_at_preview )
            {
                job->start_at_preview = start_at_preview - 1;
                job->seek_points = preview_count;
            }
            
            if( stop_at_pts )
            {
                job->pts_to_stop = stop_at_pts;
                subtitle_scan = 0;
            }
            
            if( stop_at_frame )
            {
                job->frame_to_stop = stop_at_frame;
                subtitle_scan = 0;
            }
            
            if( start_at_pts )
            {
                job->pts_to_start = start_at_pts;
                subtitle_scan = 0;
            }
            
            if( start_at_frame )
            {
                job->frame_to_start = start_at_frame;
                subtitle_scan = 0;
            }
            
            if( subtitle_scan )
            {
                char *advanced_opts_tmp;

                /*
                 * When subtitle scan is enabled do a fast pre-scan job
                 * which will determine which subtitles to enable, if any.
                 */
                job->pass = -1;

                advanced_opts_tmp = job->advanced_opts;

                job->advanced_opts = NULL;

                job->indepth_scan = subtitle_scan;
                fprintf( stderr, "Subtitle Scan Enabled - enabling "
                         "subtitles if found for foreign language segments\n");

                /*
                 * Add the pre-scan job
                 */
                hb_add( h, job );

                job->advanced_opts = advanced_opts_tmp;
            }

            if( twoPass )
            {
                /*
                 * If subtitle_scan is enabled then only turn it on
                 * for the first pass and then off again for the
                 * second.
                 */
                job->pass = 1;

                job->indepth_scan = 0;

                if (advanced_opts)
                {
                    advanced_opts2 = strdup(advanced_opts);
                }

                /*
                 * If turbo options have been selected then append them
                 * to the advanced_opts now (size includes one ':' and the '\0')
                 */
                if( turbo_opts_enabled )
                {
                    int size = (advanced_opts ? strlen(advanced_opts) : 0) + strlen(turbo_opts) + 2;
                    char *tmp_advanced_opts;

                    tmp_advanced_opts = malloc(size * sizeof(char));
                    if( advanced_opts )
                    {
                        snprintf( tmp_advanced_opts, size, "%s:%s",
                                  advanced_opts, turbo_opts );
                        free( advanced_opts );
                    } else {
                        /*
                         * No advanced_opts to modify, but apply the turbo options
                         * anyway as they may be modifying defaults
                         */
                        snprintf( tmp_advanced_opts, size, "%s",
                                  turbo_opts );
                    }
                    advanced_opts = tmp_advanced_opts;

                    fprintf( stderr, "Modified x264 options for pass 1 to append turbo options: %s\n",
                             advanced_opts );

                    job->advanced_opts = advanced_opts;
                }
                hb_add( h, job );

                job->pass = 2;
                /*
                 * On the second pass we turn off subtitle scan so that we
                 * can actually encode using any subtitles that were auto
                 * selected in the first pass (using the whacky select-subtitle
                 * attribute of the job).
                 */
                job->indepth_scan = 0;

                job->advanced_opts = advanced_opts2;

                hb_add( h, job );
            }
            else
            {
                /*
                 * Turn on subtitle scan if requested, note that this option
                 * precludes encoding of any actual subtitles.
                 */

                job->indepth_scan = 0;
                job->pass = 0;
                hb_add( h, job );
            }
            hb_start( h );
            break;
        }

#define p s.param.working
        case HB_STATE_SEARCHING:
            fprintf( stdout, "\rEncoding: task %d of %d, Searching for start time, %.2f %%",
                     p.job_cur, p.job_count, 100.0 * p.progress );
            if( p.seconds > -1 )
            {
                fprintf( stdout, " (ETA %02dh%02dm%02ds)", 
                         p.hours, p.minutes, p.seconds );
            }
            fflush(stdout);
            break;

        case HB_STATE_WORKING:
            fprintf( stdout, "\rEncoding: task %d of %d, %.2f %%",
                     p.job_cur, p.job_count, 100.0 * p.progress );
            if( p.seconds > -1 )
            {
                fprintf( stdout, " (%.2f fps, avg %.2f fps, ETA "
                         "%02dh%02dm%02ds)", p.rate_cur, p.rate_avg,
                         p.hours, p.minutes, p.seconds );
            }
            fflush(stdout);
            break;
#undef p

#define p s.param.muxing
        case HB_STATE_MUXING:
        {
            if (show_mux_warning)
            {
                fprintf( stdout, "\rMuxing: this may take awhile..." );
                fflush(stdout);
                show_mux_warning = 0;
            }
            break;
        }
#undef p

#define p s.param.workdone
        case HB_STATE_WORKDONE:
            /* Print error if any, then exit */
            switch( p.error )
            {
                case HB_ERROR_NONE:
                    fprintf( stderr, "\nRip done!\n" );
                    break;
                case HB_ERROR_CANCELED:
                    fprintf( stderr, "\nRip canceled.\n" );
                    break;
                default:
                    fprintf( stderr, "\nRip failed (error %x).\n",
                             p.error );
            }
            die = 1;
            break;
#undef p
    }
    return 0;
}

/****************************************************************************
 * SigHandler:
 ****************************************************************************/
static volatile int64_t i_die_date = 0;
void SigHandler( int i_signal )
{
    if( die == 0 )
    {
        die = 1;
        i_die_date = hb_get_date();
        fprintf( stderr, "Signal %d received, terminating - do it "
                 "again in case it gets stuck\n", i_signal );
    }
    else if( i_die_date + 500 < hb_get_date() )
    {
        fprintf( stderr, "Dying badly, files might remain in your /tmp\n" );
        exit( 1 );
    }
}

/****************************************************************************
 * ShowHelp:
 ****************************************************************************/
static void ShowHelp()
{
    int i;
    FILE* const out = stdout;

    fprintf( out,
    "Syntax: HandBrakeCLI [options] -i <device> -o <file>\n"
    "\n"
    "### General Handbrake Options------------------------------------------------\n\n"
    "    -h, --help              Print help\n"
    "    -u, --update            Check for updates and exit\n"
    "    -v, --verbose <#>       Be verbose (optional argument: logging level)\n"
    "    -Z. --preset <string>   Use a built-in preset. Capitalization matters, and\n"
    "                            if the preset name has spaces, surround it with\n"
    "                            double quotation marks\n"
    "    -z, --preset-list       See a list of available built-in presets\n"
    "        --no-dvdnav         Do not use dvdnav for reading DVDs\n"
    "                            (experimental, enabled by default for testing)\n"
    "\n"

    "### Source Options-----------------------------------------------------------\n\n"
    "    -i, --input <string>    Set input device\n"
    "    -t, --title <number>    Select a title to encode (0 to scan all titles only,\n"
    "                            default: 1)\n"
    "        --scan              Scan selected title only.\n"
    "        --main-feature      Detect and select the main feature title.\n"
    "    -c, --chapters <string> Select chapters (e.g. \"1-3\" for chapters\n"
    "                            1 to 3, or \"3\" for chapter 3 only,\n"
    "                            default: all chapters)\n"
    "        --angle <number>    Select the DVD angle\n"
    "        --previews <#:B>    Select how many preview images are generated (max 30),\n"
    "                            and whether or not they're stored to disk (0 or 1).\n"
    "                            (default: 10:0)\n"
    "    --start-at-preview <#>  Start encoding at a given preview.\n"
    "    --start-at    <unit:#>  Start encoding at a given frame, duration (in seconds),\n"
    "                            or pts (on a 90kHz clock)\n"
    "    --stop-at     <unit:#>  Stop encoding at a given frame, duration (in seconds),\n"
    "                            or pts (on a 90kHz clock)"
    "\n"

    "### Destination Options------------------------------------------------------\n\n"
    "    -o, --output <string>   Set output file name\n"
    "    -f, --format <string>   Set output format (mp4/mkv, default:\n"
    "                            autodetected from file name)\n"
    "    -m, --markers           Add chapter markers (mp4 and mkv output formats only)\n"
    "    -4, --large-file        Use 64-bit mp4 files that can hold more than\n"
    "                            4 GB. Note: Breaks iPod, PS3 compatibility.\n"""
    "    -O, --optimize          Optimize mp4 files for HTTP streaming\n"
    "    -I, --ipod-atom         Mark mp4 files so 5.5G iPods will accept them\n"
    "\n"


    "### Video Options------------------------------------------------------------\n\n"
    "    -e, --encoder <string>  Set video library encoder\n"
    "                            Options: ffmpeg4,ffmpeg2,x264,theora\n"
    "                            (default: ffmpeg4)\n"
    "    -x, --encopts <string>  Specify advanced encoder options in the\n"
    "                            same style as mencoder (x264 and ffmpeg only):\n"
    "                            option1=value1:option2=value2\n"
    "    -q, --quality <number>  Set video quality\n"
    "    -S, --size <MB>         Set target size\n"
    "    -b, --vb <kb/s>         Set video bitrate (default: 1000)\n"
    "    -2, --two-pass          Use two-pass mode\n"
    "    -T, --turbo             When using 2-pass use the turbo options\n"
    "                            on the first pass to improve speed\n"
    "                            (only works with x264, affects PSNR by about 0.05dB,\n"
    "                            and increases first pass speed two to four times)\n"
    "    -r, --rate              Set video framerate (" );
    for( i = 0; i < hb_video_rates_count; i++ )
    {
        fprintf( out, hb_video_rates[i].string );
        if( i != hb_video_rates_count - 1 )
            fprintf( out, "/" );
    }
    fprintf( out, ")\n"
    "                            Be aware that not specifying a framerate lets\n"
    "                            HandBrake preserve a source's time stamps,\n"
    "                            potentially creating variable framerate video\n"
    "    --vfr, --cfr, --pfr     Select variable, constant or peak-limited\n"
    "                            frame rate control. VFR preserves the source\n"
    "                            timing. CFR makes the output constant rate at\n"
    "                            the rate given by the -r flag (or the source's\n"
    "                            average rate if no -r is given). PFR doesn't\n"
    "                            allow the rate to go over the rate specified\n"
    "                            with the -r flag but won't change the source\n"
    "                            timing if it's below that rate.\n"
    "                            If none of these flags are given, the default\n"
    "                            is --cfr when -r is given and --vfr otherwise\n"

    "\n"
    "### Audio Options-----------------------------------------------------------\n\n"
    "    -a, --audio <string>    Select audio track(s), separated by commas\n"
    "                            More than one output track can be used for one\n"
    "                            input.\n"
    "                            (\"none\" for no audio, \"1,2,3\" for multiple\n"
    "                             tracks, default: first one)\n" );

#ifdef __APPLE_CC__
    fprintf( out,
    "    -E, --aencoder <string> Audio encoder(s)\n"
    "                                (ca_aac/ca_haac/faac/lame/vorbis/ac3/copy/copy:ac3/copy:dts)\n"
    "                            copy, copy:ac3 and copy:dts meaning passthrough.\n"
    "                            copy will passthrough either ac3 or dts.\n"
    "                            Separated by commas for more than one audio track.\n"
    "                            (default: ca_aac)\n" );
#else
    fprintf( out,
    "    -E, --aencoder <string> Audio encoder(s):\n"
    "                                (faac/lame/vorbis/ac3/copy/copy:ac3/copy:dts)\n"
    "                            copy, copy:ac3 and copy:dts meaning passthrough.\n"
    "                            copy will passthrough either ac3 or dts.\n"
    "                            Separated by commas for more than one audio track.\n"
    "                            (default: faac for mp4, lame for mkv)\n" );
#endif
    fprintf( out,
    "    -B, --ab <kb/s>         Set audio bitrate(s) (default: depends on the\n"
    "                            selected codec, mixdown and samplerate)\n"
    "                            Separated by commas for more than one audio track.\n"
    "    -6, --mixdown <string>  Format(s) for surround sound downmixing\n"
    "                            Separated by commas for more than one audio track.\n"
    "                            (mono/stereo/dpl1/dpl2/6ch, default: up to 6ch for ac3,\n"
    "                            up to dpl2 for other encoders)\n"
    "    -R, --arate             Set audio samplerate(s) (" );
    for( i = 0; i < hb_audio_rates_count; i++ )
    {
        fprintf( out, hb_audio_rates[i].string );
        if( i != hb_audio_rates_count - 1 )
            fprintf( out, "/" );
    }
    fprintf( out, " kHz)\n"
    "                            Separated by commas for more than one audio track.\n"
    "    -D, --drc <float>       Apply extra dynamic range compression to the audio,\n"
    "                            making soft sounds louder. Range is 1.0 to 4.0\n"
    "                            (too loud), with 1.5 - 2.5 being a useful range.\n"
    "                            Separated by commas for more than one audio track.\n"
    "        --gain <float>      Amplify or attenuate audio before encoding.  Does\n"
    "                            NOT work with audio passthru (copy). Values are in\n"
    "                            dB.  Negative values attenuate, positive values\n"
    "                            amplify. A 1 dB difference is barely audible.\n"
    "    -A, --aname <string>    Audio track name(s),\n"
    "                            Separated by commas for more than one audio track.\n"
    "\n"

    "### Picture Settings---------------------------------------------------------\n\n"
    "    -w, --width <number>    Set picture width\n"
    "    -l, --height <number>   Set picture height\n"
    "        --crop <T:B:L:R>    Set cropping values (default: autocrop)\n"
    "    -Y, --maxHeight <#>     Set maximum height\n"
    "    -X, --maxWidth <#>      Set maximum width\n"
    "    --strict-anamorphic     Store pixel aspect ratio in video stream\n"
    "    --loose-anamorphic      Store pixel aspect ratio with specified width\n"
    "    --custom-anamorphic     Store pixel aspect ratio in video stream and\n"
    "                            directly control all parameters.\n"
    "    --display-width         Set the width to scale the actual pixels to\n"
    "      <number>              at playback, for custom anamorphic.\n"
    "    --keep-display-aspect   Preserve the source's display aspect ratio\n"
    "                            when using custom anamorphic\n"
    "    --pixel-aspect          Set a custom pixel aspect for custom anamorphic\n"
    "      <PARX:PARY>\n"
    "                            (--display-width and --pixel-aspect are mutually\n"
    "                             exclusive and the former will override the latter)\n"
    "    --itu-par               Use wider, ITU pixel aspect values for loose and\n"
    "                            custom anamorphic, useful with underscanned sources\n"
    "    --modulus               Set the number you want the scaled pixel dimensions\n"
    "      <number>              to divide cleanly by. Does not affect strict\n"
    "                            anamorphic mode, which is always mod 2 (default: 16)\n"
    "    -M  --color-matrix      Set the color space signaled by the output\n"
    "          <601 or 709>      (Bt.601 is mostly for SD content, Bt.709 for HD,\n"
    "                             default: set by resolution)\n"
    "\n"

    "### Filters---------------------------------------------------------\n\n"

     "    -d, --deinterlace       Deinterlace video with yadif/mcdeint filter\n"
     "          <YM:FD:MM:QP>     (default 0:-1:-1:1)\n"
     "           or\n"
     "          <fast/slow/slower>\n"
     "    -5, --decomb            Selectively deinterlaces when it detects combing\n"
     "          <MO:ME:MT:ST:BT:BX:BY:MG:VA:LA:DI:ER:NO:MD:PP:FD>\n"
     "          (default: 7:2:6:9:80:16:16:10:20:20:4:2:50:24:1:-1)\n"
     "    -9, --detelecine        Detelecine (ivtc) video with pullup filter\n"
     "                            Note: this filter drops duplicate frames to\n"
     "                            restore the pre-telecine framerate, unless you\n"
     "                            specify a constant framerate (--rate 29.97)\n"
     "          <L:R:T:B:SB:MP:FD>   (default 1:1:4:4:0:0:-1)\n"
     "    -8, --denoise           Denoise video with hqdn3d filter\n"
     "          <SL:SC:TL:TC>     (default 4:3:6:4.5)\n"
     "           or\n"
     "          <weak/medium/strong>\n"
     "    -7, --deblock           Deblock video with pp7 filter\n"
     "          <QP:M>            (default 5:2)\n"
     "        --rotate            Flips images axes\n"
     "          <M>               (default 3)\n"
    "    -g, --grayscale         Grayscale encoding\n"
    "\n"

    "### Subtitle Options------------------------------------------------------------\n\n"
    "    -s, --subtitle <string> Select subtitle track(s), separated by commas\n"
    "                            More than one output track can be used for one\n"
    "                            input.\n"
    "                            Example: \"1,2,3\" for multiple tracks.\n"
    "                            A special track name \"scan\" adds an extra 1st pass.\n"
    "                            This extra pass scans subtitles matching the\n"
    "                            language of the first audio or the language \n"
    "                            selected by --native-language.\n"
    "                            The one that's only used 10 percent of the time\n"
    "                            or less is selected. This should locate subtitles\n"
    "                            for short foreign language segments. Best used in\n"
    "                            conjunction with --subtitle-forced.\n"
    "    -F, --subtitle-forced   Only display subtitles from the selected stream if\n"
    "          <string>          the subtitle has the forced flag set. The values in\n"
    "                            \"string\" are indexes into the subtitle list\n"
    "                            specified with '--subtitle'.\n"
    "                            Separated by commas for more than one audio track.\n"
    "                            Example: \"1,2,3\" for multiple tracks.\n"
    "                            If \"string\" is omitted, the first track is forced.\n"
    "        --subtitle-burn     \"Burn\" the selected subtitle into the video track\n"
    "          <number>          If \"number\" is omitted, the first track is burned.\n"
    "                            \"number\" is an index into the subtitle list\n"
    "                            specified with '--subtitle'.\n"
    "        --subtitle-default  Flag the selected subtitle as the default subtitle\n"
    "          <number>          to be displayed upon playback.  Setting no default\n"
    "                            means no subtitle will be automatically displayed\n"
    "                            If \"number\" is omitted, the first track is default.\n"
    "                            \"number\" is an index into the subtitle list\n"
    "                            specified with '--subtitle'.\n"
    "    -N, --native-language   Specifiy the your language preference. When the first\n"
    "          <string>          audio track does not match your native language then\n"
    "                            select the first subtitle that does. When used in\n"
    "                            conjunction with --native-dub the audio track is\n"
    "                            changed in preference to subtitles. Provide the\n"
    "                            language's iso639-2 code (fre, eng, spa, dut, et cetera)\n"
    "        --native-dub        Used in conjunction with --native-language\n"
    "                            requests that if no audio tracks are selected the\n"
    "                            default selected audio track will be the first one\n"
    "                            that matches the --native-language. If there are no\n"
    "                            matching audio tracks then the first matching\n"
    "                            subtitle track is used instead.\n"
    "        --srt-file <string> SubRip SRT filename(s), separated by commas.\n"
    "        --srt-codeset       Character codeset(s) that the SRT file(s) are\n"
    "          <string>          encoded in, separted by commas.\n"
    "                            Use 'iconv -l' for a list of valid\n"
    "                            codesets. If not specified latin1 is assumed\n"
    "        --srt-offset        Offset in milli-seconds to apply to the SRT file(s)\n"
    "          <string>          separted by commas. If not specified zero is assumed.\n"
    "                            Offsets may be negative.\n"
    "        --srt-lang <string> Language as an iso639-2 code fra, eng, spa et cetera)\n"
    "                            for the SRT file(s) separated by commas. If not specified\n"
    "                            then 'und' is used.\n"
    "        --srt-default       Flag the selected srt as the default subtitle\n"
    "          <number>          to be displayed upon playback.  Setting no default\n"
    "                            means no subtitle will be automatically displayed\n"
    "                            If \"number\" is omitted, the first srt is default.\n"
    "                            \"number\" is an 1 based index into the srt-file list\n"
    "\n"


    );
}

/****************************************************************************
 * ShowPresets:
 ****************************************************************************/
static void ShowPresets()
{
	fprintf( stderr, "%s - %s - %s\n", HB_PROJECT_TITLE, HB_PROJECT_BUILD_TITLE, HB_PROJECT_URL_WEBSITE );
	
	printf("\n< Apple\n");
	
    printf("\n   + Universal:  -e x264  -q 20.0 -a 1,1 -E faac,copy:ac3 -B 160,160 -6 dpl2,auto -R Auto,Auto -D 0.0,0.0 -f mp4 -X 720 --loose-anamorphic -m -x cabac=0:ref=2:me=umh:bframes=0:weightp=0:8x8dct=0:trellis=0:subme=6\n");
	
    printf("\n   + iPod:  -e x264  -b 700 -a 1 -E faac -B 160 -6 dpl2 -R Auto -D 0.0 -f mp4 -I -X 320 -m -x level=30:bframes=0:weightp=0:cabac=0:ref=1:vbv-maxrate=768:vbv-bufsize=2000:analyse=all:me=umh:no-fast-pskip=1:subme=6:8x8dct=0:trellis=0\n");
	
    printf("\n   + iPhone & iPod Touch:  -e x264  -q 20.0 -a 1 -E faac -B 128 -6 dpl2 -R Auto -D 0.0 -f mp4 -X 480 -m -x cabac=0:ref=2:me=umh:bframes=0:weightp=0:subme=6:8x8dct=0:trellis=0\n");
	
    printf("\n   + iPhone 4:  -e x264  -q 20.0 -r 29.97 --pfr  -a 1 -E faac -B 160 -6 dpl2 -R Auto -D 0.0 -f mp4 -4 -X 960 --loose-anamorphic -m\n");
	
    printf("\n   + iPad:  -e x264  -q 20.0 -r 29.97 --pfr  -a 1 -E faac -B 160 -6 dpl2 -R Auto -D 0.0 -f mp4 -4 -X 1024 --loose-anamorphic -m\n");
	
    printf("\n   + AppleTV:  -e x264  -q 20.0 -a 1,1 -E faac,copy:ac3 -B 160,160 -6 dpl2,auto -R Auto,Auto -D 0.0,0.0 -f mp4 -4 -X 960 --loose-anamorphic -m -x cabac=0:ref=2:me=umh:b-pyramid=none:b-adapt=2:weightb=0:trellis=0:weightp=0:vbv-maxrate=9500:vbv-bufsize=9500\n");
	
    printf("\n   + AppleTV 2:  -e x264  -q 20.0 -r 29.97 --pfr  -a 1,1 -E faac,copy:ac3 -B 160,160 -6 dpl2,auto -R Auto,Auto -D 0.0,0.0 -f mp4 -4 -X 1280 --loose-anamorphic -m\n");
	
    printf("\n>\n");
	
    printf("\n< Regular\n");
	
    printf("\n   + Normal:  -e x264  -q 20.0 -a 1 -E faac -B 160 -6 dpl2 -R Auto -D 0.0 -f mp4 --strict-anamorphic -m -x ref=2:bframes=2:subme=6:mixed-refs=0:weightb=0:8x8dct=0:trellis=0\n");
	
    printf("\n   + High Profile:  -e x264  -q 20.0 -a 1,1 -E faac,copy:ac3 -B 160,160 -6 dpl2,auto -R Auto,Auto -D 0.0,0.0 -f mp4 --detelecine --decomb --loose-anamorphic -m -x b-adapt=2:rc-lookahead=50\n");
	
    printf("\n>\n");
	
    printf("\n< Legacy\n");
	
    printf("\n   + Classic:  -b 1000 -a 1 -E faac -B 160 -6 dpl2 -R Auto -D 0.0 -f mp4\n");
	
    printf("\n   + AppleTV Legacy:  -e x264  -b 2500 -a 1,1 -E faac,copy:ac3 -B 160,160 -6 dpl2,auto -R Auto,Auto -D 0.0,0.0 -f mp4 -4 --strict-anamorphic -m -x ref=1:b-pyramid=none:weightp=0:subme=5:me=umh:no-fast-pskip=1:cabac=0:weightb=0:8x8dct=0:trellis=0\n");
	
    printf("\n   + iPhone Legacy:  -e x264  -b 960 -a 1 -E faac -B 128 -6 dpl2 -R Auto -D 0.0 -f mp4 -I -X 480 -m -x level=30:cabac=0:ref=1:analyse=all:me=umh:no-fast-pskip=1:psy-rd=0,0:bframes=0:weightp=0:subme=6:8x8dct=0:trellis=0\n");
	
    printf("\n   + iPod Legacy:  -e x264  -b 1500 -a 1 -E faac -B 160 -6 dpl2 -R Auto -D 0.0 -f mp4 -I -X 640 -m -x level=30:bframes=0:weightp=0:cabac=0:ref=1:vbv-maxrate=1500:vbv-bufsize=2000:analyse=all:me=umh:no-fast-pskip=1:psy-rd=0,0:subme=6:8x8dct=0:trellis=0\n");
	
    printf("\n>\n");

}

static char * hb_strndup( char * str, int len )
{
	char * res;
	int str_len = strlen( str );

	res = malloc( len > str_len ? str_len + 1 : len + 1 );
	strncpy( res, str, len );
	res[len] = '\0';
	return res;
}

static char** str_split( char *str, char delem )
{
    char *  pos;
    char *  end;
    char ** ret;
    int     count, i;

    if ( str == NULL || str[0] == 0 )
    {
        ret = malloc( sizeof(char*) );
        *ret = NULL;
        return ret;
    }

    // Find number of elements in the string
    count = 1;
    pos = str;
    while ( ( pos = strchr( pos, delem ) ) != NULL )
    {
        count++;
        pos++;
    }

    ret = calloc( ( count + 1 ), sizeof(char*) );

    pos = str;
    for ( i = 0; i < count - 1; i++ )
    {
        end = strchr( pos, delem );
        ret[i] = hb_strndup(pos, end - pos);
        pos = end + 1;
    }
    ret[i] = strdup(pos);

    return ret;
}

/****************************************************************************
 * ParseOptions:
 ****************************************************************************/
static int ParseOptions( int argc, char ** argv )
{
    
    #define PREVIEWS            257
    #define START_AT_PREVIEW    258
    #define START_AT            259
    #define STOP_AT             260
    #define ANGLE               261
    #define DVDNAV              262
    #define DISPLAY_WIDTH       263
    #define PIXEL_ASPECT        264
    #define MODULUS             265
    #define KEEP_DISPLAY_ASPECT 266
    #define SUB_BURNED          267
    #define SUB_DEFAULT         268
    #define NATIVE_DUB          269
    #define SRT_FILE            270
    #define SRT_CODESET         271
    #define SRT_OFFSET          272
    #define SRT_LANG            273
    #define SRT_DEFAULT         274
    #define ROTATE_FILTER       275
    #define SCAN_ONLY           276
    #define MAIN_FEATURE        277
    #define MIN_DURATION        278
    #define AUDIO_GAIN          279
    
    for( ;; )
    {
        static struct option long_options[] =
          {
            { "help",        no_argument,       NULL,    'h' },
            { "update",      no_argument,       NULL,    'u' },
            { "verbose",     optional_argument, NULL,    'v' },
            { "no-dvdnav",      no_argument,       NULL,    DVDNAV },

            { "format",      required_argument, NULL,    'f' },
            { "input",       required_argument, NULL,    'i' },
            { "output",      required_argument, NULL,    'o' },
            { "large-file",  no_argument,       NULL,    '4' },
            { "optimize",    no_argument,       NULL,    'O' },
            { "ipod-atom",   no_argument,       NULL,    'I' },

            { "title",       required_argument, NULL,    't' },
            { "min-duration",required_argument, NULL,    MIN_DURATION },
            { "scan",        no_argument,       NULL,    SCAN_ONLY },
            { "main-feature",no_argument,       NULL,    MAIN_FEATURE },
            { "chapters",    required_argument, NULL,    'c' },
            { "angle",       required_argument, NULL,    ANGLE },
            { "markers",     optional_argument, NULL,    'm' },
            { "audio",       required_argument, NULL,    'a' },
            { "mixdown",     required_argument, NULL,    '6' },
            { "drc",         required_argument, NULL,    'D' },
            { "gain",        required_argument, NULL,    AUDIO_GAIN },
            { "subtitle",    required_argument, NULL,    's' },
            { "subtitle-forced", optional_argument,   NULL,    'F' },
            { "subtitle-burned", optional_argument,   NULL,    SUB_BURNED },
            { "subtitle-default", optional_argument,   NULL,    SUB_DEFAULT },
            { "srt-file",    required_argument, NULL, SRT_FILE },
            { "srt-codeset", required_argument, NULL, SRT_CODESET },
            { "srt-offset",  required_argument, NULL, SRT_OFFSET },
            { "srt-lang",    required_argument, NULL, SRT_LANG },
            { "srt-default",    optional_argument, NULL, SRT_DEFAULT },
            { "native-language", required_argument, NULL,'N' },
            { "native-dub",  no_argument,       NULL,    NATIVE_DUB },
            { "encoder",     required_argument, NULL,    'e' },
            { "aencoder",    required_argument, NULL,    'E' },
            { "two-pass",    no_argument,       NULL,    '2' },
            { "deinterlace", optional_argument, NULL,    'd' },
            { "deblock",     optional_argument, NULL,    '7' },
            { "denoise",     optional_argument, NULL,    '8' },
            { "detelecine",  optional_argument, NULL,    '9' },
            { "decomb",      optional_argument, NULL,    '5' },
            { "grayscale",   no_argument,       NULL,    'g' },
            { "rotate",      optional_argument, NULL,   ROTATE_FILTER },
            { "strict-anamorphic",  no_argument, &anamorphic_mode, 1 },
            { "loose-anamorphic", no_argument, &anamorphic_mode, 2 },
            { "custom-anamorphic", no_argument, &anamorphic_mode, 3 },
            { "display-width", required_argument, NULL, DISPLAY_WIDTH },
            { "keep-display-aspect", no_argument, &keep_display_aspect, 1 },
            { "pixel-aspect", required_argument, NULL, PIXEL_ASPECT },
            { "modulus",     required_argument, NULL, MODULUS },
            { "itu-par",     no_argument,       &itu_par, 1  },
            { "width",       required_argument, NULL,    'w' },
            { "height",      required_argument, NULL,    'l' },
            { "crop",        required_argument, NULL,    'n' },

            { "vb",          required_argument, NULL,    'b' },
            { "quality",     required_argument, NULL,    'q' },
            { "size",        required_argument, NULL,    'S' },
            { "ab",          required_argument, NULL,    'B' },
            { "rate",        required_argument, NULL,    'r' },
            { "arate",       required_argument, NULL,    'R' },
            { "encopts",     required_argument, NULL,    'x' },
            { "turbo",       no_argument,       NULL,    'T' },
            { "maxHeight",   required_argument, NULL,    'Y' },
            { "maxWidth",    required_argument, NULL,    'X' },
            { "preset",      required_argument, NULL,    'Z' },
            { "preset-list", no_argument,       NULL,    'z' },

            { "aname",       required_argument, NULL,    'A' },
            { "color-matrix",required_argument, NULL,    'M' },
            { "previews",    required_argument, NULL,    PREVIEWS },
            { "start-at-preview", required_argument, NULL, START_AT_PREVIEW },
            { "start-at",    required_argument, NULL,    START_AT },
            { "stop-at",    required_argument, NULL,     STOP_AT },
            { "vfr",         no_argument,       &cfr,    0 },
            { "cfr",         no_argument,       &cfr,    1 },
            { "pfr",         no_argument,       &cfr,    2 },
#if defined( __APPLE_CC__ )
            { "no-vlc-dylib-path", no_argument, &no_vlc_dylib,    1 },
#endif
            { 0, 0, 0, 0 }
          };

        int option_index = 0;
        int c;
        int cur_optind;

        cur_optind = optind;
        c = getopt_long( argc, argv,
                         "hv::uC:f:4i:Io:t:c:m::M:a:A:6:s:UF::N:e:E:"
                         "2dD:7895gOw:l:n:b:q:S:B:r:R:x:TY:X:Z:z",
                         long_options, &option_index );
        if( c < 0 )
        {
            break;
        }

        switch( c )
        {
            case 0:
                /* option was handled entirely in getopt_long */
                break;
            case 'h':
                ShowHelp();
                exit( 0 );
            case 'u':
                update = 1;
                break;
            case 'v':
                if( optarg != NULL )
                {
                    debug = atoi( optarg );
                }
                else
                {
                    debug = 1;
                }
                break;
            case 'Z':
                preset = 1;
                preset_name = strdup(optarg);
                break;
            case 'z':
                ShowPresets();
                exit ( 0 );
            case DVDNAV:
                dvdnav = 0;
                break;

            case 'f':
                format = strdup( optarg );
                break;
            case 'i':
                input = strdup( optarg );
#ifdef __APPLE_CC__
                char *devName = bsd_name_for_path( input ); // alloc
                if( devName )
                {
                    if( device_is_dvd( devName ))
                    {
                        free( input );
                        input = malloc( strlen( "/dev/" ) + strlen( devName ) + 1 );
                        sprintf( input, "/dev/%s", devName );
                    }
                    free( devName );
                }
#endif
                break;
            case 'o':
                output = strdup( optarg );
                break;
            case '4':
                largeFileSize = 1;
                break;
            case 'O':
                mp4_optimize = 1;
                break;
            case 'I':
                ipod_atom = 1;
                break;

            case 't':
                titleindex = atoi( optarg );
                break;
            case SCAN_ONLY:
                titlescan = 1;
                break;
            case MAIN_FEATURE:
                main_feature = 1;
                break;
            case 'c':
            {
                int start, end;
                if( sscanf( optarg, "%d-%d", &start, &end ) == 2 )
                {
                    chapter_start = start;
                    chapter_end   = end;
                }
                else if( sscanf( optarg, "%d", &start ) == 1 )
                {
                    chapter_start = start;
                    chapter_end   = chapter_start;
                }
                else
                {
                    fprintf( stderr, "chapters: invalid syntax (%s)\n",
                             optarg );
                    return -1;
                }
                break;
            }
            case ANGLE:
                angle = atoi( optarg );
                break;
            case 'm':
                if( optarg != NULL )
                {
                    marker_file = strdup( optarg );
                }
                chapter_markers = 1;
                break;
            case 'a':
                if( optarg != NULL )
                {
                    atracks = strdup( optarg );
                    audio_explicit = 1;
                }
                else
                {
                    atracks = "1" ;
                }
                break;
            case '6':
                if( optarg != NULL )
                {
                    mixdowns = strdup( optarg );
                }
                break;
            case 'D':
                if( optarg != NULL )
                {
                    dynamic_range_compression = strdup( optarg );
                }
                break;
            case AUDIO_GAIN:
                if( optarg != NULL )
                {
                    audio_gain = strdup( optarg );
                }
                break;
            case 's':
                subtracks = str_split( optarg, ',' );
                break;
            case 'F':
                subforce = str_split( optarg, ',' );
                break;
            case SUB_BURNED:
                if( optarg != NULL )
                {
                    subburn = strdup( optarg );
                }
                else
                {
                    subburn = "" ;
                }
                break;
            case SUB_DEFAULT:
                if( optarg != NULL )
                {
                    subdefault = strdup( optarg );
                }
                else
                {
                    subdefault = "" ;
                }
                break;
            case 'N':
                native_language = strdup( optarg );
                break;
            case NATIVE_DUB:
                native_dub = 1;
                break;
            case SRT_FILE:
                srtfile = str_split( optarg, ',' );
                break;
            case SRT_CODESET:
                srtcodeset = str_split( optarg, ',' );
                break;
            case SRT_OFFSET:
                srtoffset = str_split( optarg, ',' );
                break;
            case SRT_LANG:
                srtlang = str_split( optarg, ',' );
                break;
            case SRT_DEFAULT:
                if( optarg != NULL )
                {
                    srtdefault = atoi( optarg );
                }
                else
                {
                    srtdefault = 1 ;
                }
                break;
            case '2':
                twoPass = 1;
                break;
            case 'd':
                if( optarg != NULL )
                {
                    if (!( strcmp( optarg, "fast" ) ))
                    {
                        deinterlace_opt = "-1";
                    }
                    else if (!( strcmp( optarg, "slow" ) ))
                    {
                        deinterlace_opt = "2";
                    }
                    else if (!( strcmp( optarg, "slower" ) ))
                    {
                        deinterlace_opt = "0";
                    }
                    else
                    {
                        deinterlace_opt = strdup( optarg );
                    }
                }
                deinterlace = 1;
                break;
            case '7':
                if( optarg != NULL )
                {
                    deblock_opt = strdup( optarg );
                }
                deblock = 1;
                break;
            case '8':
                if( optarg != NULL )
                {
                    if (!( strcmp( optarg, "weak" ) ))
                    {
                        denoise_opt = "2:1:2:3";
                    }
                    else if (!( strcmp( optarg, "medium" ) ))
                    {
                        denoise_opt = "3:2:2:3";
                    }
                    else if (!( strcmp( optarg, "strong" ) ))
                    {
                        denoise_opt = "7:7:5:5";
                    }
                    else
                    {
                        denoise_opt = strdup( optarg );
                    }
                }
                denoise = 1;
                break;
            case '9':
                if( optarg != NULL )
                {
                    detelecine_opt = strdup( optarg );
                }
                detelecine = 1;
                break;
            case '5':
                if( optarg != NULL )
                {
                    decomb_opt = strdup( optarg );
                }
                decomb = 1;
                break;
            case 'g':
                grayscale = 1;
                break;
            case ROTATE_FILTER:
                if( optarg != NULL )
                {
                    rotate_opt = strdup( optarg );
                }
                rotate = 1;
                break;
            case DISPLAY_WIDTH:
                if( optarg != NULL )
                {
                    sscanf( optarg, "%i", &display_width );
                }
                break;
            case PIXEL_ASPECT:
                if( optarg != NULL )
                {
                    sscanf( optarg, "%i:%i", &par_width, &par_height );
                }
                break;
            case MODULUS:
                if( optarg != NULL )
                {
                    sscanf( optarg, "%i", &modulus );
                }
                break;
            case 'e':
                if( !strcasecmp( optarg, "ffmpeg" ) )
                {
                    vcodec = HB_VCODEC_FFMPEG_MPEG4;
                }
                else if( !strcasecmp( optarg, "ffmpeg4" ) )
                {
                    vcodec = HB_VCODEC_FFMPEG_MPEG4;
                }
                else if( !strcasecmp( optarg, "ffmpeg2" ) )
                {
                    vcodec = HB_VCODEC_FFMPEG_MPEG2;
                }
                else if( !strcasecmp( optarg, "x264" ) )
                {
                    vcodec = HB_VCODEC_X264;
                }
                else if( !strcasecmp( optarg, "theora" ) )
                {
                    vcodec = HB_VCODEC_THEORA;
                }
                else
                {
                    fprintf( stderr, "invalid codec (%s)\n", optarg );
                    return -1;
                }
                break;
            case 'E':
                if( optarg != NULL )
                {
                    acodecs = strdup( optarg );
                }
                break;
            case 'w':
                width = atoi( optarg );
                break;
            case 'l':
                height = atoi( optarg );
                break;
            case 'n':
            {
                int    i;
                char * tmp = optarg;
                for( i = 0; i < 4; i++ )
                {
                    if( !*tmp )
                        break;
                    crop[i] = strtol( tmp, &tmp, 0 );
                    tmp++;
                }
                break;
            }
            case 'r':
            {
                int i;
                vrate = 0;
                for( i = 0; i < hb_video_rates_count; i++ )
                {
                    if( !strcmp( optarg, hb_video_rates[i].string ) )
                    {
                        vrate = hb_video_rates[i].rate;
                        break;
                    }
                }
                if( !vrate )
                {
                    fprintf( stderr, "invalid framerate %s\n", optarg );
                }
                else if ( cfr == 0 )
                {
                    cfr = 1;
                }
                break;
            }
            case 'R':
                if( optarg != NULL )
                {
                    arates = strdup( optarg );
                }
                break;
            case 'b':
                vbitrate = atoi( optarg );
                break;
            case 'q':
                vquality = atof( optarg );
                break;
            case 'S':
                size = atoi( optarg );
                break;
            case 'B':
                if( optarg != NULL )
                {
                    abitrates = strdup( optarg );
                }
                break;
            case 'x':
                advanced_opts = strdup( optarg );
                break;
            case 'T':
                turbo_opts_enabled = 1;
                break;
            case 'Y':
                maxHeight = atoi( optarg );
                break;
            case 'X':
                maxWidth = atoi (optarg );
                break;
            case 'A':
                if( optarg != NULL )
                {
                    anames = strdup( optarg );
                }
                break;
            case PREVIEWS:
                sscanf( optarg, "%i:%i", &preview_count, &store_previews );
                break;
            case START_AT_PREVIEW:
                start_at_preview = atoi( optarg );
                break;
            case START_AT:
                start_at_string = strdup( optarg );
                start_at_token = strtok( start_at_string, ":");
                if( !strcmp( start_at_token, "frame" ) )
                {
                    start_at_token = strtok( NULL, ":");
                    start_at_frame = atoi(start_at_token);
                }
                else if( !strcmp( start_at_token, "pts" ) )
                {
                    start_at_token = strtok( NULL, ":");
                    sscanf( start_at_token, "%"SCNd64, &start_at_pts );
                }
                else if( !strcmp( start_at_token, "duration" ) )
                {
                    start_at_token = strtok( NULL, ":");
                    sscanf( start_at_token, "%"SCNd64, &start_at_pts );
                    start_at_pts *= 90000LL;
                }
                break;
            case STOP_AT:
                stop_at_string = strdup( optarg );
                stop_at_token = strtok( stop_at_string, ":");
                if( !strcmp( stop_at_token, "frame" ) )
                {
                    stop_at_token = strtok( NULL, ":");
                    stop_at_frame = atoi(stop_at_token);
                }
                else if( !strcmp( stop_at_token, "pts" ) )
                {
                    stop_at_token = strtok( NULL, ":");
                    sscanf( stop_at_token, "%"SCNd64, &stop_at_pts );
                }
                else if( !strcmp( stop_at_token, "duration" ) )
                {
                    stop_at_token = strtok( NULL, ":");
                    sscanf( stop_at_token, "%"SCNd64, &stop_at_pts );
                    stop_at_pts *= 90000LL;
                }
                break;
            case 'M':
                if( atoi( optarg ) == 601 )
                    color_matrix = 1;
                else if( atoi( optarg ) == 709 )
                    color_matrix = 2;
                break;
            case MIN_DURATION:
                min_title_duration = strtol( optarg, NULL, 0 );
                break;
            default:
                fprintf( stderr, "unknown option (%s)\n", argv[cur_optind] );
                return -1;
        }
    }

    return 0;
}

char * str_printf(const char *fmt, ...)
{
    /* Guess we need no more than 100 bytes. */
    int len;
    va_list ap;
    int size = 100;
    char *tmp, *str = NULL;

    str = (char*)malloc(size);
    while (1) 
    {
        /* Try to print in the allocated space. */
        va_start(ap, fmt);
        len = vsnprintf(str, size, fmt, ap);
        va_end(ap);

        /* If that worked, return the string. */
        if (len > -1 && len < size) {
            return str;
        }

        /* Else try again with more space. */
        if (len > -1)    /* glibc 2.1 */
            size = len+1; /* precisely what is needed */
        else           /* glibc 2.0 */
            size *= 2;  /* twice the old size */

        tmp = (char*)realloc(str, size);
        if (tmp == NULL) {
            return str;
        }
        str = tmp;
    }
}

static int CheckOptions( int argc, char ** argv )
{
#if defined( __APPLE_CC__ )
    // If OSX, add VLC dylib path and exec to make it stick.
    char *dylib_path;

    if ( !no_vlc_dylib )
    {
        dylib_path = getenv("DYLD_FALLBACK_LIBRARY_PATH");
        if ( dylib_path == NULL ||
             strstr( dylib_path, "/Applications/VLC.app/Contents/MacOS/lib" ) == NULL )
        {
            char *path = NULL;
            char *home;
            int result = -1;

            home = getenv("HOME");

            if ( dylib_path == NULL )
            {
                // Set the system default of $HOME/lib:/usr/local/lib:/usr/lib
                // And add our extra path
                if ( home != NULL )
                {
                    path = str_printf("%s/lib:%s:%s:%s%s", home, 
                                      DEFAULT_DYLD_PATH, 
                                      EXTRA_VLC_DYLD_PATH, 
                                      home, EXTRA_VLC_DYLD_PATH);
                }
                else
                {
                    path = str_printf("%s:%s", DEFAULT_DYLD_PATH, EXTRA_VLC_DYLD_PATH);
                }
                if ( path != NULL )
                    result = setenv("DYLD_FALLBACK_LIBRARY_PATH", path, 1);
            }
            else
            {
                // add our extra path
                if ( home != NULL )
                {
                    path = str_printf("%s:%s:%s%s", dylib_path, EXTRA_VLC_DYLD_PATH,
                                                        home, EXTRA_VLC_DYLD_PATH);
                }
                else
                {
                    path = str_printf("%s:%s", dylib_path, EXTRA_VLC_DYLD_PATH);
                }
                if ( path != NULL )
                    result = setenv("DYLD_FALLBACK_LIBRARY_PATH", path, 1);
            }
            if ( result == 0 )
            {
                const char ** new_argv;
                int i;

                new_argv = (const char**)malloc( (argc + 2) * sizeof(char*) );
                new_argv[0] = argv[0];
                new_argv[1] = "--no-vlc-dylib-path";
                for (i = 1; i < argc; i++)
                    new_argv[i+1] = argv[i];
                new_argv[i+1] = NULL;
                execv(new_argv[0], (char* const*)new_argv);
            }
        }
    }
#endif

    if( update )
    {
        return 0;
    }

    if( input == NULL || *input == '\0' )
    {
        fprintf( stderr, "Missing input device. Run %s --help for "
                 "syntax.\n", argv[0] );
        return 1;
    }

    /* Parse format */
    if( titleindex > 0 && !titlescan )
    {
        if( output == NULL || *output == '\0' )
        {
            fprintf( stderr, "Missing output file name. Run %s --help "
                     "for syntax.\n", argv[0] );
            return 1;
        }

        if( !format )
        {
            char * p = strrchr( output, '.' );

            /* autodetect */
            if( p && ( !strcasecmp( p, ".mp4" )  ||
                       !strcasecmp( p, ".m4v" ) ) )
            {
                mux = HB_MUX_MP4;
            }
            else if( p && !strcasecmp(p, ".mkv" ) )
            {
                mux = HB_MUX_MKV;
#ifndef __APPLE_CC__
                // default to Lame for MKV (except under OS X where Core Audio is available)
                default_acodec = HB_ACODEC_LAME;
#endif
            }
            else
            {
                fprintf( stderr, "Output format couldn't be guessed "
                         "from file name, using default.\n" );
                return 0;
            }
        }
        else if( !strcasecmp( format, "mp4" ) ||
                 !strcasecmp( format, "m4v" ) )
        {
            mux = HB_MUX_MP4;
        }
        else if( !strcasecmp( format, "mkv" ) )
        {
            mux = HB_MUX_MKV;
#ifndef __APPLE_CC__
            // default to Lame for MKV (except under OS X where Core Audio is available)
            default_acodec = HB_ACODEC_LAME;
#endif
        }
        else
        {
            fprintf( stderr, "Invalid output format (%s). Possible "
                     "choices are mp4, m4v and mkv\n.", format );
            return 1;
        }
    }

    return 0;
}

static int get_acodec_for_string( char *codec )
{
    if( !strcasecmp( codec, "ac3" ) )
    {
        return HB_ACODEC_AC3;
    }
    else if( !strcasecmp( codec, "copy" ) )
    {
        return HB_ACODEC_AC3_PASS | HB_ACODEC_DCA_PASS;
    }
    else if( !strcasecmp( codec, "copy:ac3" ) )
    {
        return HB_ACODEC_AC3_PASS;
    }
    else if( !strcasecmp( codec, "copy:dts" ) || !strcasecmp( codec, "copy:dca" ) )
    {
        return HB_ACODEC_DCA_PASS;
    }
    else if( !strcasecmp( codec, "lame" ) )
    {
        return HB_ACODEC_LAME;
    }
    else if( !strcasecmp( codec, "faac" ) )
    {
        return HB_ACODEC_FAAC;
    }
    else if( !strcasecmp( codec, "vorbis") )
    {
        return HB_ACODEC_VORBIS;
    }
#ifdef __APPLE__
    else if( !strcasecmp( codec, "ca_aac") )
    {
        return HB_ACODEC_CA_AAC;
    }
    else if( !strcasecmp( codec, "ca_haac") )
    {
        return HB_ACODEC_CA_HAAC;
    }
#endif
    else
    {
        return -1;
    }
}

static int is_sample_rate_valid(int rate)
{
    int i;
    for( i = 0; i < hb_audio_rates_count; i++ )
    {
            if (rate == hb_audio_rates[i].rate)
                return 1;
    }
    return 0;
}

#ifdef __APPLE_CC__
/****************************************************************************
 * bsd_name_for_path
 *
 * Returns the BSD device name for the block device that contains the
 * passed-in path. Returns NULL on failure.
 ****************************************************************************/
static char* bsd_name_for_path(char *path)
{
    OSStatus err;
    FSRef ref;
    err = FSPathMakeRef( (const UInt8 *) input, &ref, NULL );
    if( err != noErr )
    {
        return NULL;
    }

    // Get the volume reference number.
    FSCatalogInfo catalogInfo;
    err = FSGetCatalogInfo( &ref, kFSCatInfoVolume, &catalogInfo, NULL, NULL,
                            NULL);
    if( err != noErr )
    {
        return NULL;
    }
    FSVolumeRefNum volRefNum = catalogInfo.volume;

    // Now let's get the device name
    GetVolParmsInfoBuffer volumeParms;
    err = FSGetVolumeParms( volRefNum, &volumeParms, sizeof( volumeParms ) );
    if( err != noErr )
    {
        return NULL;
    }

    // A version 4 GetVolParmsInfoBuffer contains the BSD node name in the vMDeviceID field.
    // It is actually a char * value. This is mentioned in the header CoreServices/CarbonCore/Files.h.
    if( volumeParms.vMVersion < 4 )
    {
        return NULL;
    }

    // vMDeviceID might be zero as is reported with experimental ZFS (zfs-119) support in Leopard.
    if( !volumeParms.vMDeviceID )
    {
        return NULL;
    }

    return strdup( volumeParms.vMDeviceID );
}

/****************************************************************************
 * device_is_dvd
 *
 * Returns whether or not the passed in BSD device represents a DVD, or other
 * optical media.
 ****************************************************************************/
static int device_is_dvd(char *device)
{
    io_service_t service = get_iokit_service(device);
    if( service == IO_OBJECT_NULL )
    {
        return 0;
    }
    int result = is_dvd_service(service);
    IOObjectRelease(service);
    return result;
}

/****************************************************************************
 * get_iokit_service
 *
 * Returns the IOKit service object for the passed in BSD device name.
 ****************************************************************************/
static io_service_t get_iokit_service( char *device )
{
    CFMutableDictionaryRef matchingDict;
    matchingDict = IOBSDNameMatching( kIOMasterPortDefault, 0, device );
    if( matchingDict == NULL )
    {
        return IO_OBJECT_NULL;
    }
    // Fetch the object with the matching BSD node name. There should only be
    // one match, so IOServiceGetMatchingService is used instead of
    // IOServiceGetMatchingServices to simplify the code.
    return IOServiceGetMatchingService( kIOMasterPortDefault, matchingDict );
}

/****************************************************************************
 * is_dvd_service
 *
 * Returns whether or not the service passed in is a DVD.
 *
 * Searches for an IOMedia object that represents the entire (whole) media that
 * the volume is on. If the volume is on partitioned media, the whole media
 * object will be a parent of the volume's media object. If the media is not
 * partitioned, the volume's media object will be the whole media object.
 ****************************************************************************/
static int is_dvd_service( io_service_t service )
{
    kern_return_t  kernResult;
    io_iterator_t  iter;

    // Create an iterator across all parents of the service object passed in.
    kernResult = IORegistryEntryCreateIterator( service,
                                                kIOServicePlane,
                                                kIORegistryIterateRecursively | kIORegistryIterateParents,
                                                &iter );
    if( kernResult != KERN_SUCCESS )
    {
        return 0;
    }
    if( iter == IO_OBJECT_NULL )
    {
        return 0;
    }

    // A reference on the initial service object is released in the do-while
    // loop below, so add a reference to balance.
    IOObjectRetain( service );

    int result = 0;
    do
    {
        if( is_whole_media_service( service ) &&
            IOObjectConformsTo( service, kIODVDMediaClass) )
        {
            result = 1;
        }
        IOObjectRelease( service );
    } while( !result && (service = IOIteratorNext( iter )) );
    IOObjectRelease( iter );

    return result;
}

/****************************************************************************
 * is_whole_media_service
 *
 * Returns whether or not the service passed in is an IOMedia service and
 * represents the "whole" media instead of just a partition.
 *
 * The whole media object is indicated in the IORegistry by the presence of a
 * property with the key "Whole" and value "Yes".
 ****************************************************************************/
static int is_whole_media_service( io_service_t service )
{
    int result = 0;

    if( IOObjectConformsTo( service, kIOMediaClass ) )
    {
        CFTypeRef wholeMedia = IORegistryEntryCreateCFProperty( service,
                                                                CFSTR( kIOMediaWholeKey ),
                                                                kCFAllocatorDefault,
                                                                0 );
        if ( !wholeMedia )
        {
            return 0;
        }
        result = CFBooleanGetValue( (CFBooleanRef)wholeMedia );
        CFRelease( wholeMedia );
    }

    return result;
}
#endif // __APPLE_CC__
