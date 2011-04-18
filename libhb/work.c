/* $Id: work.c,v 1.43 2005/03/17 16:38:49 titer Exp $

   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

#include "hb.h"
#include "a52dec/a52.h"
#include "dca.h"
#include "libavformat/avformat.h"

typedef struct
{
    hb_list_t * jobs;
    hb_job_t  ** current_job;
    int       * error;
    volatile int * die;

} hb_work_t;

static void work_func();
static void do_job( hb_job_t *);
static void work_loop( void * );

#define FIFO_UNBOUNDED 65536
#define FIFO_UNBOUNDED_WAKE 65535
#define FIFO_LARGE 32
#define FIFO_LARGE_WAKE 16
#define FIFO_SMALL 16
#define FIFO_SMALL_WAKE 15

/**
 * Allocates work object and launches work thread with work_func.
 * @param jobs Handle to hb_list_t.
 * @param die Handle to user inititated exit indicator.
 * @param error Handle to error indicator.
 */
hb_thread_t * hb_work_init( hb_list_t * jobs, volatile int * die, int * error, hb_job_t ** job )
{
    hb_work_t * work = calloc( sizeof( hb_work_t ), 1 );

    work->jobs      = jobs;
    work->current_job = job;
    work->die       = die;
    work->error     = error;

    return hb_thread_init( "work", work_func, work, HB_LOW_PRIORITY );
}

static void InitWorkState( hb_handle_t * h )
{
    hb_state_t state;

    state.state = HB_STATE_WORKING;
#define p state.param.working
    p.progress  = 0.0;
    p.rate_cur  = 0.0;
    p.rate_avg  = 0.0;
    p.hours     = -1;
    p.minutes   = -1;
    p.seconds   = -1; 
#undef p

    hb_set_state( h, &state );

}

/**
 * Iterates through job list and calls do_job for each job.
 * @param _work Handle work object.
 */
static void work_func( void * _work )
{
    hb_work_t  * work = _work;
    hb_job_t   * job;

    hb_log( "%d job(s) to process", hb_list_count( work->jobs ) );

    while( !*work->die && ( job = hb_list_item( work->jobs, 0 ) ) )
    {
        hb_list_rem( work->jobs, job );
        job->die = work->die;
        *(work->current_job) = job;
        InitWorkState( job->h );
        do_job( job );
        *(work->current_job) = NULL;
    }

    *(work->error) = HB_ERROR_NONE;

    free( work );
}

hb_work_object_t * hb_get_work( int id )
{
    hb_work_object_t * w;
    for( w = hb_objects; w; w = w->next )
    {
        if( w->id == id )
        {
            hb_work_object_t *wc = malloc( sizeof(*w) );
            *wc = *w;
            return wc;
        }
    }
    return NULL;
}

hb_work_object_t * hb_codec_decoder( int codec )
{
    switch( codec )
    {
        case HB_ACODEC_AC3:  return hb_get_work( WORK_DECA52 );
        case HB_ACODEC_DCA:  return hb_get_work( WORK_DECDCA );
        case HB_ACODEC_MPGA: return hb_get_work( WORK_DECAVCODEC );
        case HB_ACODEC_LPCM: return hb_get_work( WORK_DECLPCM );
        case HB_ACODEC_FFMPEG: return hb_get_work( WORK_DECAVCODECAI );
    }
    return NULL;
}

hb_work_object_t * hb_codec_encoder( int codec )
{
    switch( codec )
    {
        case HB_ACODEC_FAAC:   return hb_get_work( WORK_ENCFAAC );
        case HB_ACODEC_LAME:   return hb_get_work( WORK_ENCLAME );
        case HB_ACODEC_VORBIS: return hb_get_work( WORK_ENCVORBIS );
        case HB_ACODEC_CA_AAC: return hb_get_work( WORK_ENC_CA_AAC );
        case HB_ACODEC_CA_HAAC:return hb_get_work( WORK_ENC_CA_HAAC );
        case HB_ACODEC_AC3:    return hb_get_work( WORK_ENCAC3 );
    }
    return NULL;
}

/**
 * Displays job parameters in the debug log.
 * @param job Handle work hb_job_t.
 */
void hb_display_job_info( hb_job_t * job )
{
    hb_title_t * title = job->title;
    hb_audio_t   * audio;
    hb_subtitle_t * subtitle;
    int             i, j;
    
    hb_log("job configuration:");
    hb_log( " * source");
    
    hb_log( "   + %s", title->path );

    hb_log( "   + title %d, chapter(s) %d to %d", title->index,
            job->chapter_start, job->chapter_end );

    if( title->container_name != NULL )
        hb_log( "   + container: %s", title->container_name);

    if( title->data_rate )
    {
        hb_log( "   + data rate: %d kbps", title->data_rate / 1000 );
    }
    
    hb_log( " * destination");

    hb_log( "   + %s", job->file );

    switch( job->mux )
    {
        case HB_MUX_MP4:
            hb_log("   + container: MPEG-4 (.mp4 and .m4v)");
            
            if( job->ipod_atom )
                hb_log( "     + compatibility atom for iPod 5G");

            if( job->largeFileSize )
                hb_log( "     + 64-bit formatting");

            if( job->mp4_optimize )
                hb_log( "     + optimized for progressive web downloads");
            
            if( job->color_matrix )
                hb_log( "     + custom color matrix: %s", job->color_matrix == 1 ? "ITU Bt.601 (SD)" : "ITU Bt.709 (HD)");
            break;

        case HB_MUX_MKV:
            hb_log("   + container: Matroska (.mkv)");
            break;
    }

    if( job->chapter_markers )
    {
        hb_log( "     + chapter markers" );
    }
    
    hb_log(" * video track");
    
    hb_log("   + decoder: %s", title->video_codec_name );
    
    if( title->video_bitrate )
    {
        hb_log( "     + bitrate %d kbps", title->video_bitrate / 1000 );
    }
    
    if( job->cfr == 0 )
    {
        hb_log( "   + frame rate: same as source (around %.3f fps)",
            (float) title->rate / (float) title->rate_base );
    }
    else if( job->cfr == 1 )
    {
        hb_log( "   + frame rate: %.3f fps -> constant %.3f fps",
            (float) title->rate / (float) title->rate_base,
            (float) job->vrate / (float) job->vrate_base );
    }
    else if( job->cfr == 2 )
    {
        hb_log( "   + frame rate: %.3f fps -> peak rate limited to %.3f fps",
            (float) title->rate / (float) title->rate_base,
            (float) job->pfr_vrate / (float) job->pfr_vrate_base );
    }

    if( job->anamorphic.mode )
    {
        hb_log( "   + %s anamorphic", job->anamorphic.mode == 1 ? "strict" : job->anamorphic.mode == 2? "loose" : "custom" );
        if( job->anamorphic.mode == 3 && job->anamorphic.keep_display_aspect )
        {
            hb_log( "     + keeping source display aspect ratio"); 
        }
        hb_log( "     + storage dimensions: %d * %d -> %d * %d, crop %d/%d/%d/%d, mod %i",
                    title->width, title->height, job->width, job->height,
                    job->crop[0], job->crop[1], job->crop[2], job->crop[3], job->modulus );
        if( job->anamorphic.itu_par )
        {
            hb_log( "     + using ITU pixel aspect ratio values"); 
        }
        hb_log( "     + pixel aspect ratio: %i / %i", job->anamorphic.par_width, job->anamorphic.par_height );
        hb_log( "     + display dimensions: %.0f * %i",
            (float)( job->width * job->anamorphic.par_width / job->anamorphic.par_height ), job->height );
    }
    else
    {
        hb_log( "   + dimensions: %d * %d -> %d * %d, crop %d/%d/%d/%d, mod %i",
                title->width, title->height, job->width, job->height,
                job->crop[0], job->crop[1], job->crop[2], job->crop[3], job->modulus );
    }

    if ( job->grayscale )
        hb_log( "   + grayscale mode" );

    if( hb_list_count( job->filters ) )
    {
        hb_log("   + %s", hb_list_count( job->filters) > 1 ? "filters" : "filter" );
        for( i = 0; i < hb_list_count( job->filters ); i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->filters, i );
            if (filter->settings)
                hb_log("     + %s (%s)", filter->name, filter->settings);
            else
                hb_log("     + %s (default settings)", filter->name);
        }
    }
    
    if( !job->indepth_scan )
    {
        /* Video encoder */
        switch( job->vcodec )
        {
            case HB_VCODEC_FFMPEG_MPEG4:
                hb_log( "   + encoder: FFmpeg MPEG-4" );
                break;

            case HB_VCODEC_FFMPEG_MPEG2:
                hb_log( "   + encoder: FFmpeg MPEG-2" );
                break;

            case HB_VCODEC_X264:
                hb_log( "   + encoder: x264" );
                if( job->advanced_opts != NULL && *job->advanced_opts != '\0' )
                    hb_log( "     + options: %s", job->advanced_opts);
                break;

            case HB_VCODEC_THEORA:
                hb_log( "   + encoder: Theora" );
                break;
        }

        if( job->vquality >= 0 )
        {
            hb_log( "     + quality: %.2f %s", job->vquality, job->vcodec == HB_VCODEC_X264 ? "(RF)" : "(QP)" ); 
        }
        else
        {
            hb_log( "     + bitrate: %d kbps, pass: %d", job->vbitrate, job->pass );
        }
    }

    for( i=0; i < hb_list_count( title->list_subtitle ); i++ )
    {
        subtitle =  hb_list_item( title->list_subtitle, i );

        if( subtitle )
        {
            if( subtitle->source == SRTSUB )
            {
                /* For SRT, print offset and charset too */
                hb_log( " * subtitle track %i, %s (id 0x%x) %s [%s] -> %s%s, offset: %"PRId64", charset: %s",
                        subtitle->track, subtitle->lang, subtitle->id, "Text", "SRT", "Pass-Through",
                        subtitle->config.default_track ? ", Default" : "",
                        subtitle->config.offset, subtitle->config.src_codeset );
            }
            else
            {
                hb_log( " * subtitle track %i, %s (id 0x%x) %s [%s] -> %s%s%s", subtitle->track, subtitle->lang, subtitle->id,
                        subtitle->format == PICTURESUB ? "Picture" : "Text",
                        hb_subsource_name( subtitle->source ),
                        job->indepth_scan ? "Foreign Audio Search" :
                        subtitle->config.dest == RENDERSUB ? "Render/Burn in" : "Pass-Through",
                        subtitle->config.force ? ", Forced Only" : "",
                        subtitle->config.default_track ? ", Default" : "" );
            }
        }
    }

    if( !job->indepth_scan )
    {
        for( i = 0; i < hb_list_count( title->list_audio ); i++ )
        {
            audio = hb_list_item( title->list_audio, i );

            hb_log( " * audio track %d", audio->config.out.track );
            
            if( audio->config.out.name )
                hb_log( "   + name: %s", audio->config.out.name );

            hb_log( "   + decoder: %s (track %d, id 0x%x)", audio->config.lang.description, audio->config.in.track + 1, audio->id );

            if( ( audio->config.in.codec == HB_ACODEC_AC3 ) || ( audio->config.in.codec == HB_ACODEC_DCA) )
            {
                hb_log( "     + bitrate: %d kbps, samplerate: %d Hz", audio->config.in.bitrate / 1000, audio->config.in.samplerate );
            }

            if( (audio->config.out.codec != HB_ACODEC_AC3_PASS) && (audio->config.out.codec != HB_ACODEC_DCA_PASS) )
            {
                for (j = 0; j < hb_audio_mixdowns_count; j++)
                {
                    if (hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown) {
                        hb_log( "   + mixdown: %s", hb_audio_mixdowns[j].human_readable_name );
                        break;
                    }
                }
                if ( audio->config.out.gain != 0.0 )
                {
                    hb_log( "   + gain: %.fdB", audio->config.out.gain );
                }
            }

            if ( audio->config.out.dynamic_range_compression && (audio->config.out.codec != HB_ACODEC_AC3_PASS) && (audio->config.out.codec != HB_ACODEC_DCA_PASS))
            {
                hb_log("   + dynamic range compression: %f", audio->config.out.dynamic_range_compression);
            }
            
            if( (audio->config.out.codec == HB_ACODEC_AC3_PASS) || (audio->config.out.codec == HB_ACODEC_DCA_PASS) )
            {
                hb_log( "   + %s passthrough", (audio->config.out.codec == HB_ACODEC_AC3_PASS) ?
                    "AC3" : "DCA" );
            }
            else
            {
                hb_log( "   + encoder: %s", 
                    ( audio->config.out.codec == HB_ACODEC_FAAC ) ?  "faac" : 
                    ( ( audio->config.out.codec == HB_ACODEC_LAME ) ?  "lame" : 
                    ( ( audio->config.out.codec == HB_ACODEC_CA_AAC ) ?  "ca_aac" : 
                    ( ( audio->config.out.codec == HB_ACODEC_CA_HAAC ) ?  "ca_haac" : 
                    ( ( audio->config.out.codec == HB_ACODEC_AC3 ) ?  "ffac3" : 
                    "vorbis"  ) ) ) ) );
                hb_log( "     + bitrate: %d kbps, samplerate: %d Hz", audio->config.out.bitrate, audio->config.out.samplerate );            
            }
        }
    }
}

/* Corrects framerates when actual duration and frame count numbers are known. */
void correct_framerate( hb_job_t * job )
{
    hb_interjob_t * interjob = hb_interjob_get( job->h );

    if( ( job->sequence_id & 0xFFFFFF ) != ( interjob->last_job & 0xFFFFFF) )
        return; // Interjob information is for a different encode.

    // compute actual output vrate from first pass
    interjob->vrate = job->vrate_base * ( (double)interjob->out_frame_count * 90000 / interjob->total_time );
    interjob->vrate_base = job->vrate_base;
}

static int check_ff_audio( hb_list_t *list_audio, hb_audio_t *ff_audio )
{
    int i;

    for( i = 0; i < hb_list_count( list_audio ); i++ )
    {
        hb_audio_t * audio = hb_list_item( list_audio, i );

        if ( audio == ff_audio )
            break;

        if ( audio->config.in.codec == HB_ACODEC_FFMPEG && 
             audio->id == ff_audio->id )
        {
            hb_list_add( audio->priv.ff_audio_list, ff_audio );
            return 1;
        }
    }
    return 0;
}

/**
 * Job initialization rountine.
 * Initializes fifos.
 * Creates work objects for synchronizer, video decoder, video renderer, video decoder, audio decoder, audio encoder, reader, muxer.
 * Launches thread for each work object with work_loop.
 * Loops while monitoring status of work threads and fifos.
 * Exits loop when conversion is done and fifos are empty.
 * Closes threads and frees fifos.
 * @param job Handle work hb_job_t.
 */
static void do_job( hb_job_t * job )
{
    hb_title_t    * title;
    int             i, j;
    hb_work_object_t * w;
    hb_work_object_t * sync;
    hb_work_object_t * muxer;
    hb_interjob_t * interjob;

    hb_audio_t   * audio;
    hb_subtitle_t * subtitle;
    unsigned int subtitle_highest = 0;
    unsigned int subtitle_highest_id = 0;
    unsigned int subtitle_lowest = -1;
    unsigned int subtitle_lowest_id = 0;
    unsigned int subtitle_forced_id = 0;
    unsigned int subtitle_hit = 0;

    title = job->title;
    interjob = hb_interjob_get( job->h );

    if( job->pass == 2 )
    {
        correct_framerate( job );
    }

    job->list_work = hb_list_init();

    hb_log( "starting job" );

    if( job->anamorphic.mode )
    {
        hb_set_anamorphic_size(job, &job->width, &job->height, &job->anamorphic.par_width, &job->anamorphic.par_height);

        if( job->vcodec & HB_VCODEC_FFMPEG_MASK )
        {
            /* Just to make working with ffmpeg even more fun,
               lavc's MPEG-4 encoder can't handle PAR values >= 255,
               even though AVRational does. Adjusting downwards
               distorts the display aspect slightly, but such is life. */
            while ((job->anamorphic.par_width & ~0xFF) ||
                   (job->anamorphic.par_height & ~0xFF))
            {
                job->anamorphic.par_width >>= 1;
                job->anamorphic.par_height >>= 1;
            }
            hb_reduce( &job->anamorphic.par_width, &job->anamorphic.par_height, job->anamorphic.par_width, job->anamorphic.par_height );
        }
    }
    
    /* Keep width and height within these boundaries,
       but ignore for anamorphic. For "loose" anamorphic encodes,
       this stuff is covered in the pixel_ratio section above.    */
    if ( job->maxHeight && ( job->height > job->maxHeight ) && ( !job->anamorphic.mode ) )
    {
        job->height = job->maxHeight;
        hb_fix_aspect( job, HB_KEEP_HEIGHT );
        hb_log( "Height out of bounds, scaling down to %i", job->maxHeight );
        hb_log( "New dimensions %i * %i", job->width, job->height );
    }
    if ( job->maxWidth && ( job->width > job->maxWidth ) && ( !job->anamorphic.mode ) )
    {
        job->width = job->maxWidth;
        hb_fix_aspect( job, HB_KEEP_WIDTH );
        hb_log( "Width out of bounds, scaling down to %i", job->maxWidth );
        hb_log( "New dimensions %i * %i", job->width, job->height );
    }

    if ( job->cfr == 0 )
    {
        /* Ensure we're using "Same as source" FPS */
        job->vrate = title->rate;
        job->vrate_base = title->rate_base;
    }
    else if ( job->cfr == 2 )
    {
        job->pfr_vrate = job->vrate;
        job->pfr_vrate_base = job->vrate_base;

        // Ensure we're using "Same as source" FPS, with peak set by pfr_vrate_* 
        // For PFR, we want the framerate based on the source's actual 
        // framerate, unless it's higher than the specified peak framerate. 
        double source_fps = (double)job->title->rate / job->title->rate_base;
        double peak_l_fps = (double)job->vrate / job->vrate_base;
        if ( source_fps < peak_l_fps )
        {
            job->vrate_base = title->rate_base;
            job->vrate = title->rate;
        }
    }

    job->fifo_mpeg2  = hb_fifo_init( FIFO_LARGE, FIFO_LARGE_WAKE );
    job->fifo_raw    = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
    job->fifo_sync   = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
    job->fifo_render = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
    job->fifo_mpeg4  = hb_fifo_init( FIFO_LARGE, FIFO_LARGE_WAKE );

    /*
     * Audio fifos must be initialized before sync
     */
    if( !job->indepth_scan )
    {
    // if we are doing passthru, and the input codec is not the same as the output
    // codec, then remove this audio from the job. If we're not doing passthru and
    // the input codec is the 'internal' ffmpeg codec, make sure that only one
    // audio references that audio stream since the codec context is specific to
    // the audio id & multiple copies of the same stream will garble the audio
    // or cause aborts.
    for( i = 0; i < hb_list_count( title->list_audio ); )
    {
        audio = hb_list_item( title->list_audio, i );
        if( ( ( audio->config.out.codec == HB_ACODEC_AC3_PASS ) && ( audio->config.in.codec != HB_ACODEC_AC3 ) ) ||
            ( ( audio->config.out.codec == HB_ACODEC_DCA_PASS ) && ( audio->config.in.codec != HB_ACODEC_DCA ) ) )
        {
            hb_log( "Passthru requested and input codec is not the same as output codec for track %d",
                    audio->config.out.track );
            hb_list_rem( title->list_audio, audio );
            free( audio );
            continue;
        }
        if( audio->config.out.codec != HB_ACODEC_AC3_PASS && 
            audio->config.out.codec != HB_ACODEC_DCA_PASS &&
            audio->config.out.samplerate > 48000 )
        {
            hb_log( "Sample rate %d not supported.  Down-sampling to 48kHz.",
                    audio->config.out.samplerate );
            audio->config.out.samplerate = 48000;
        }
        if( audio->config.out.codec == HB_ACODEC_AC3 && 
            audio->config.out.bitrate > 640 )
        {
            hb_log( "Bitrate %d not supported.  Reducing to 640Kbps.",
                    audio->config.out.bitrate );
            audio->config.out.bitrate = 640;
        }
        if( audio->config.out.codec == HB_ACODEC_CA_HAAC )
        {
            if( !encca_haac_available() )
            {
                // user chose Core Audio HE-AAC but the encoder is unavailable
                hb_log( "Core Audio HE-AAC unavailable. Using Core Audio AAC for track %d",
                        audio->config.out.track );
                audio->config.out.codec = HB_ACODEC_CA_AAC;
            }
            else if( audio->config.out.samplerate < 32000 )
            {
                // Core Audio HE-AAC doesn't support samplerates < 32 kHz
                hb_log( "Sample rate %d not supported (ca_haac). Using 32kHz for track %d",
                        audio->config.out.samplerate, audio->config.out.track );
                audio->config.out.samplerate = 32000;
            }
        }
        /* Adjust output track number, in case we removed one.
         * Output tracks sadly still need to be in sequential order.
         */
        audio->config.out.track = i++;
    }

    int requested_mixdown = 0;
    int requested_mixdown_index = 0;
    int best_mixdown = 0;
    int requested_bitrate = 0;
    int best_bitrate = 0;

    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );

        best_mixdown = hb_get_best_mixdown( audio->config.out.codec,
                                            audio->config.in.channel_layout, 0 );

        /* sense-check the current mixdown options */

        /* sense-check the requested mixdown */
        if( audio->config.out.mixdown == 0 &&
            !( audio->config.out.codec & HB_ACODEC_PASS_FLAG ) )
        {
            /*
             * Mixdown wasn't specified and this is not pass-through,
             * set a default mixdown
             */
            audio->config.out.mixdown = hb_get_default_mixdown( audio->config.out.codec,
                                                                audio->config.in.channel_layout );
            for (j = 0; j < hb_audio_mixdowns_count; j++)
            {
                if (hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown)
                {
                    hb_log("work: mixdown not specified, track %i setting mixdown %s", audio->config.out.track, hb_audio_mixdowns[j].human_readable_name);
                    break;
                }
            }
        }

        /* log the requested mixdown */
        for (j = 0; j < hb_audio_mixdowns_count; j++) {
            if (hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown) {
                requested_mixdown = audio->config.out.mixdown;
                requested_mixdown_index = j;
                break;
            }
        }

        if ( !( audio->config.out.codec & HB_ACODEC_PASS_FLAG ) )
        {
            if ( audio->config.out.mixdown > best_mixdown )
            {
                audio->config.out.mixdown = best_mixdown;
            }
        }

        if ( audio->config.out.mixdown != requested_mixdown )
        {
            /* log the output mixdown */
            for (j = 0; j < hb_audio_mixdowns_count; j++)
            {
                if (hb_audio_mixdowns[j].amixdown == audio->config.out.mixdown)
                {
                    hb_log("work: sanitizing track %i mixdown %s to %s", 
                        audio->config.out.track, 
                        hb_audio_mixdowns[requested_mixdown_index].human_readable_name, 
                        hb_audio_mixdowns[j].human_readable_name);
                    break;
                }
            }
        }
        
        /* sense-check the current bitrates */
        
        /* sense-check the requested bitrate */
        if( audio->config.out.bitrate == 0 &&
            !( audio->config.out.codec & HB_ACODEC_PASS_FLAG ) )
        {
            /*
             * Bitrate wasn't specified and this is not pass-through,
             * set a default bitrate
             */
            audio->config.out.bitrate = hb_get_default_audio_bitrate( audio->config.out.codec,
                                                                      audio->config.out.samplerate,
                                                                      audio->config.out.mixdown );
            
            hb_log( "work: bitrate not specified, track %d setting bitrate %d",
                    audio->config.out.track, audio->config.out.bitrate );
        }
        
        /* log the requested bitrate */
        requested_bitrate = audio->config.out.bitrate;
        best_bitrate = hb_get_best_audio_bitrate( audio->config.out.codec, 
                                                  audio->config.out.bitrate,
                                                  audio->config.out.samplerate,
                                                  audio->config.out.mixdown );
        
        if ( !( audio->config.out.codec & HB_ACODEC_PASS_FLAG ) )
        {
            if ( audio->config.out.bitrate != best_bitrate )
            {
                audio->config.out.bitrate = best_bitrate;
            }
        }
        
        if ( audio->config.out.bitrate != requested_bitrate )
        {
            /* log the output bitrate */
            hb_log( "work: sanitizing track %d audio bitrate %d to %d", 
                    audio->config.out.track, requested_bitrate, 
                    audio->config.out.bitrate);
        }

        if (audio->config.out.codec == HB_ACODEC_VORBIS)
            audio->priv.config.vorbis.language = audio->config.lang.simple;

        /* set up the audio work structures */
        audio->priv.fifo_raw  = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
        audio->priv.fifo_sync = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
        audio->priv.fifo_out  = hb_fifo_init( FIFO_LARGE, FIFO_LARGE_WAKE );

        audio->priv.ff_audio_list = hb_list_init();
        if ( audio->config.in.codec == HB_ACODEC_FFMPEG )
        {
            if ( !check_ff_audio( title->list_audio, audio ) )
            {
                audio->priv.fifo_in   = hb_fifo_init( FIFO_LARGE, FIFO_LARGE_WAKE );
            }
        }
        else
        {
            audio->priv.fifo_in   = hb_fifo_init( FIFO_LARGE, FIFO_LARGE_WAKE );
        }
    }

    }
    /* Synchronization */
    sync = hb_sync_init( job );

    /* Video decoder */
    int vcodec = title->video_codec? title->video_codec : WORK_DECMPEG2;
#if defined(USE_FF_MPEG2)
    if (vcodec == WORK_DECMPEG2)
    {
        vcodec = WORK_DECAVCODECV;
        title->video_codec_param = CODEC_ID_MPEG2VIDEO;
    }
#endif
    hb_list_add( job->list_work, ( w = hb_get_work( vcodec ) ) );
    w->codec_param = title->video_codec_param;
    w->fifo_in  = job->fifo_mpeg2;
    w->fifo_out = job->fifo_raw;

    /* Video renderer */
    hb_list_add( job->list_work, ( w = hb_get_work( WORK_RENDER ) ) );
    w->fifo_in  = job->fifo_sync;
    if( !job->indepth_scan )
        w->fifo_out = job->fifo_render;
    else
        w->fifo_out = NULL;

    if( !job->indepth_scan )
    {

        /* Video encoder */
        switch( job->vcodec )
        {
        case HB_VCODEC_FFMPEG_MPEG4:
            w = hb_get_work( WORK_ENCAVCODEC );
            w->codec_param = CODEC_ID_MPEG4;
            break;
        case HB_VCODEC_FFMPEG_MPEG2:
            w = hb_get_work( WORK_ENCAVCODEC );
            w->codec_param = CODEC_ID_MPEG2VIDEO;
            break;
        case HB_VCODEC_X264:
            w = hb_get_work( WORK_ENCX264 );
            break;
        case HB_VCODEC_THEORA:
            w = hb_get_work( WORK_ENCTHEORA );
            break;
        }
        w->fifo_in  = job->fifo_render;
        w->fifo_out = job->fifo_mpeg4;
        w->config   = &job->config;

        hb_list_add( job->list_work, w );
    }

    /*
     * Look for the scanned subtitle in the existing subtitle list
     * select_subtitle implies that we did a scan.
     */
    if ( !job->indepth_scan && interjob->select_subtitle &&
         ( job->pass == 0 || job->pass == 2 ) )
    {
        /*
         * Disable forced subtitles if we didn't find any in the scan
         * so that we display normal subtitles instead.
         */
        if( interjob->select_subtitle->config.force && 
            interjob->select_subtitle->forced_hits == 0 )
        {
            interjob->select_subtitle->config.force = 0;
        }
        for( i=0; i < hb_list_count(title->list_subtitle); i++ )
        {
            subtitle =  hb_list_item( title->list_subtitle, i );

            if( subtitle )
            {
                /*
                * Remove the scanned subtitle from the subtitle list if
                * it would result in an identical duplicate subtitle track
                * or an emty track (forced and no forced hits).
                */
                if( ( interjob->select_subtitle->id == subtitle->id ) &&
                    ( ( interjob->select_subtitle->forced_hits == 0 &&
                        subtitle->config.force ) ||
                    ( subtitle->config.force == interjob->select_subtitle->config.force ) ) )
                {
                    *subtitle = *(interjob->select_subtitle);
                    free( interjob->select_subtitle );
                    interjob->select_subtitle = NULL;
                    break;
                }
            }
        }

        if( interjob->select_subtitle )
        {
            /*
             * Its not in the existing list
             *
             * Must be second pass of a two pass with subtitle scan enabled, so
             * add the subtitle that we found on the first pass for use in this
             * pass.
             */
            hb_list_add( title->list_subtitle, interjob->select_subtitle );
            interjob->select_subtitle = NULL;
        }
    }


    for( i=0; i < hb_list_count(title->list_subtitle); i++ )
    {
        subtitle =  hb_list_item( title->list_subtitle, i );

        if( subtitle )
        {
            subtitle->fifo_in   = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
            // Must set capacity of the raw-FIFO to be set >= the maximum number of subtitle
            // lines that could be decoded prior to a video frame in order to prevent the following
            // deadlock condition:
            //   1. Subtitle decoder blocks trying to generate more subtitle lines than will fit in the FIFO.
            //   2. Blocks the processing of further subtitle packets read from the input stream.
            //   3. And that blocks the processing of any further video packets read from the input stream.
            //   4. And that blocks the sync work-object from running, which is needed to consume the subtitle lines in the raw-FIFO.
            // Since that number is unbounded, the FIFO must be made (effectively) unbounded in capacity.
            subtitle->fifo_raw  = hb_fifo_init( FIFO_UNBOUNDED, FIFO_UNBOUNDED_WAKE );
            subtitle->fifo_sync = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );
            subtitle->fifo_out  = hb_fifo_init( FIFO_SMALL, FIFO_SMALL_WAKE );

            if( (!job->indepth_scan || job->select_subtitle_config.force) && 
                subtitle->source == VOBSUB ) {
                /*
                 * Don't add threads for subtitles when we are scanning, unless
                 * looking for forced subtitles.
                 */
                w = hb_get_work( WORK_DECVOBSUB );
                w->fifo_in  = subtitle->fifo_in;
                w->fifo_out = subtitle->fifo_raw;
                w->subtitle = subtitle;
                hb_list_add( job->list_work, w );
            }

            if( !job->indepth_scan && subtitle->source == CC608SUB )
            {
                w = hb_get_work( WORK_DECCC608 );
                w->fifo_in  = subtitle->fifo_in;
                w->fifo_out = subtitle->fifo_raw;
                hb_list_add( job->list_work, w );
            }

            if( !job->indepth_scan && subtitle->source == SRTSUB )
            {
                w = hb_get_work( WORK_DECSRTSUB );
                w->fifo_in  = subtitle->fifo_in;
                w->fifo_out = subtitle->fifo_raw;
                w->subtitle = subtitle;
                hb_list_add( job->list_work, w );
            }
            
            if( !job->indepth_scan && subtitle->source == UTF8SUB )
            {
                w = hb_get_work( WORK_DECUTF8SUB );
                w->fifo_in  = subtitle->fifo_in;
                w->fifo_out = subtitle->fifo_raw;
                hb_list_add( job->list_work, w );
            }
            
            if( !job->indepth_scan && subtitle->source == TX3GSUB )
            {
                w = hb_get_work( WORK_DECTX3GSUB );
                w->fifo_in  = subtitle->fifo_in;
                w->fifo_out = subtitle->fifo_raw;
                hb_list_add( job->list_work, w );
            }
            
            if( !job->indepth_scan && subtitle->source == SSASUB )
            {
                w = hb_get_work( WORK_DECSSASUB );
                w->fifo_in  = subtitle->fifo_in;
                w->fifo_out = subtitle->fifo_raw;
                w->subtitle = subtitle;
                hb_list_add( job->list_work, w );
            }
        }
    }

    if( !job->indepth_scan )
    {
        for( i = 0; i < hb_list_count( title->list_audio ); i++ )
        {
            audio = hb_list_item( title->list_audio, i );

            /*
            * Audio Decoder Thread
            */
            if ( audio->priv.fifo_in )
            {
                if ( ( w = hb_codec_decoder( audio->config.in.codec ) ) == NULL )
                {
                    hb_error("Invalid input codec: %d", audio->config.in.codec);
                    *job->die = 1;
                    goto cleanup;
                }
                w->fifo_in  = audio->priv.fifo_in;
                w->fifo_out = audio->priv.fifo_raw;
                w->config   = &audio->priv.config;
                w->audio    = audio;
                w->codec_param = audio->config.in.codec_param;

                hb_list_add( job->list_work, w );
            }

            /*
            * Audio Encoder Thread
            */
            if( audio->config.out.codec != HB_ACODEC_AC3_PASS &&
                audio->config.out.codec != HB_ACODEC_DCA_PASS )
            {
                /*
                * Add the encoder thread if not doing AC-3 pass through
                */
                if ( ( w = hb_codec_encoder( audio->config.out.codec ) ) == NULL )
                {
                    hb_error("Invalid audio codec: %#x", audio->config.out.codec);
                    w = NULL;
                    *job->die = 1;
                    goto cleanup;
                }
                w->fifo_in  = audio->priv.fifo_sync;
                w->fifo_out = audio->priv.fifo_out;
                w->config   = &audio->priv.config;
                w->audio    = audio;

                hb_list_add( job->list_work, w );
            }
        }
    }
    
    if( job->chapter_markers && job->chapter_start == job->chapter_end )
    {
        job->chapter_markers = 0;
        hb_log("work: only 1 chapter, disabling chapter markers");
    }

    /* Display settings */
    hb_display_job_info( job );

    /* Init read & write threads */
    job->reader = hb_reader_init( job );


    job->done = 0;

    /* Launch processing threads */
    for( i = 0; i < hb_list_count( job->list_work ); i++ )
    {
        w = hb_list_item( job->list_work, i );
        w->done = &job->done;
        w->thread_sleep_interval = 10;
        if( w->init( w, job ) )
        {
            hb_error( "Failure to initialise thread '%s'", w->name );
            *job->die = 1;
            goto cleanup;
        }
        w->thread = hb_thread_init( w->name, work_loop, w,
                                    HB_LOW_PRIORITY );
    }

    if ( job->indepth_scan )
    {
        muxer = NULL;
        w = sync;
        sync->done = &job->done;
    }
    else
    {
        sync->done = &job->done;
        sync->thread_sleep_interval = 10;
        if( sync->init( w, job ) )
        {
            hb_error( "Failure to initialise thread '%s'", w->name );
            *job->die = 1;
            goto cleanup;
        }
        sync->thread = hb_thread_init( sync->name, work_loop, sync,
                                    HB_LOW_PRIORITY );

        // The muxer requires track information that's set up by the encoder
        // init routines so we have to init the muxer last.
        muxer = hb_muxer_init( job );
        w = muxer;
    }

    hb_buffer_t      * buf_in, * buf_out = NULL;

    while ( !*job->die && !*w->done && w->status != HB_WORK_DONE )
    {
        buf_in = hb_fifo_get_wait( w->fifo_in );
        if ( buf_in == NULL )
            continue;
        if ( *job->die )
        {
            if( buf_in )
            {
                hb_buffer_close( &buf_in );
            }
            break;
        }

        buf_out = NULL;
        w->status = w->work( w, &buf_in, &buf_out );

        if( buf_in )
        {
            hb_buffer_close( &buf_in );
        }
        if ( buf_out && w->fifo_out == NULL )
        {
            hb_buffer_close( &buf_out );
        }
        if( buf_out )
        {
            while ( !*job->die )
            {
                if ( hb_fifo_full_wait( w->fifo_out ) )
                {
                    hb_fifo_push( w->fifo_out, buf_out );
                    buf_out = NULL;
                    break;
                }
            }
        }
    }

    if ( buf_out )
    {
        hb_buffer_close( &buf_out );
    }

    hb_handle_t * h = job->h;
    hb_state_t state;
    hb_get_state( h, &state );
    
    hb_log("work: average encoding speed for job is %f fps", state.param.working.rate_avg);

    job->done = 1;
    if( muxer != NULL )
    {
        muxer->close( muxer );
        free( muxer );

        if( sync->thread != NULL )
        {
            hb_thread_close( &sync->thread );
            sync->close( sync );
        }
        free( sync );
    }

cleanup:
    /* Stop the write thread (thread_close will block until the muxer finishes) */
    job->done = 1;

    /* Close work objects */
    while( ( w = hb_list_item( job->list_work, 0 ) ) )
    {
        hb_list_rem( job->list_work, w );
        if( w->thread != NULL )
        {
            hb_thread_close( &w->thread );
            w->close( w );
        }
        free( w );
    }

    hb_list_close( &job->list_work );

    /* Stop the read thread */
    if( job->reader != NULL )
        hb_thread_close( &job->reader );

    /* Close fifos */
    hb_fifo_close( &job->fifo_mpeg2 );
    hb_fifo_close( &job->fifo_raw );
    hb_fifo_close( &job->fifo_sync );
    hb_fifo_close( &job->fifo_render );
    hb_fifo_close( &job->fifo_mpeg4 );

    for (i=0; i < hb_list_count(title->list_subtitle); i++) {
        subtitle =  hb_list_item( title->list_subtitle, i);
        if( subtitle )
        {
            hb_fifo_close( &subtitle->fifo_in );
            hb_fifo_close( &subtitle->fifo_raw );
            hb_fifo_close( &subtitle->fifo_sync );
            hb_fifo_close( &subtitle->fifo_out );
        }
    }
    for( i = 0; i < hb_list_count( title->list_audio ); i++ )
    {
        audio = hb_list_item( title->list_audio, i );
        if( audio->priv.fifo_in != NULL )
            hb_fifo_close( &audio->priv.fifo_in );
        if( audio->priv.fifo_raw != NULL )
            hb_fifo_close( &audio->priv.fifo_raw );
        if( audio->priv.fifo_sync != NULL )
            hb_fifo_close( &audio->priv.fifo_sync );
        if( audio->priv.fifo_out != NULL )
            hb_fifo_close( &audio->priv.fifo_out );
    }

    if( job->indepth_scan )
    {
        /*
         * Before closing the title print out our subtitle stats if we need to
         * Find the highest and lowest.
         */
        for( i=0; i < hb_list_count( title->list_subtitle ); i++ )
        {
            subtitle =  hb_list_item( title->list_subtitle, i );

            hb_log( "Subtitle stream 0x%x '%s': %d hits (%d forced)",
                    subtitle->id, subtitle->lang, subtitle->hits,
                    subtitle->forced_hits );

            if( subtitle->hits == 0 )
                continue;

            if( subtitle->hits > subtitle_highest )
            {
                subtitle_highest = subtitle->hits;
                subtitle_highest_id = subtitle->id;
            }

            if( subtitle->hits < subtitle_lowest )
            {
                subtitle_lowest = subtitle->hits;
                subtitle_lowest_id = subtitle->id;
            }

            if( subtitle->forced_hits > 0 )
            {
                if( subtitle_forced_id == 0 )
                {
                    subtitle_forced_id = subtitle->id;
                }
            }
        }

        
        if( subtitle_forced_id )
        {
            /*
             * If there are any subtitle streams with forced subtitles
             * then select it in preference to the lowest.
             */
            subtitle_hit = subtitle_forced_id;
            hb_log("Found a subtitle candidate id 0x%x (contains forced subs)",
                   subtitle_hit);
        } else if( subtitle_lowest < subtitle_highest )
        {
            /*
             * OK we have more than one, and the lowest is lower,
             * but how much lower to qualify for turning it on by
             * default?
             *
             * Let's say 10% as a default.
             */
            if( subtitle_lowest < ( subtitle_highest * 0.1 ) )
            {
                subtitle_hit = subtitle_lowest_id;
                hb_log( "Found a subtitle candidate id 0x%x",
                        subtitle_hit );
            } else {
                hb_log( "No candidate subtitle detected during subtitle-scan");
            }
        }
    }

    if( job->indepth_scan )
    {
        for( i=0; i < hb_list_count( title->list_subtitle ); i++ )
        {
            subtitle =  hb_list_item( title->list_subtitle, i );
            if( subtitle->id == subtitle_hit )
            {
                subtitle->config = job->select_subtitle_config;
                hb_list_rem( title->list_subtitle, subtitle );
                interjob->select_subtitle = subtitle;
                break;
            }
        }
    }

    if( job->filters )
    {
        for( i = 0; i < hb_list_count( job->filters ); i++ )
        {
            hb_filter_object_t * filter = hb_list_item( job->filters, i );
            hb_filter_close( &filter );
        }
        hb_list_close( &job->filters );
    }

    hb_buffer_pool_free();

    hb_title_close( &job->title );
    free( job );
}

/**
 * Performs the work object's specific work function.
 * Loops calling work function for associated work object. Sleeps when fifo is full.
 * Monitors work done indicator.
 * Exits loop when work indiactor is set.
 * @param _w Handle to work object.
 */
static void work_loop( void * _w )
{
    hb_work_object_t * w = _w;
    hb_buffer_t      * buf_in, * buf_out;

    while( !*w->done && w->status != HB_WORK_DONE )
    {
        buf_in = hb_fifo_get_wait( w->fifo_in );
        if ( buf_in == NULL )
            continue;
        if ( *w->done )
        {
            if( buf_in )
            {
                hb_buffer_close( &buf_in );
            }
            break;
        }
        // Invalidate buf_out so that if there is no output
        // we don't try to pass along junk.
        buf_out = NULL;
        w->status = w->work( w, &buf_in, &buf_out );

        // Propagate any chapter breaks for the worker if and only if the
        // output frame has the same time stamp as the input frame (any
        // worker that delays frames has to propagate the chapter marks itself
        // and workers that move chapter marks to a different time should set
        // 'buf_in' to NULL so that this code won't generate spurious duplicates.)
        if( buf_in && buf_out && buf_in->new_chap && buf_in->start == buf_out->start)
        {
            // restore log below to debug chapter mark propagation problems
            //hb_log("work %s: Copying Chapter Break @ %"PRId64, w->name, buf_in->start);
            buf_out->new_chap = buf_in->new_chap;
        }

        if( buf_in )
        {
            hb_buffer_close( &buf_in );
        }
        if ( buf_out && w->fifo_out == NULL )
        {
            hb_buffer_close( &buf_out );
        }
        if( buf_out )
        {
            while ( !*w->done )
            {
                if ( hb_fifo_full_wait( w->fifo_out ) )
                {
                    hb_fifo_push( w->fifo_out, buf_out );
                    buf_out = NULL;
                    break;
                }
            }
        }
    }
    if ( buf_out )
    {
        hb_buffer_close( &buf_out );
    }

    // Consume data in incoming fifo till job complete so that
    // residual data does not stall the pipeline
    while( !*w->done )
    {
        buf_in = hb_fifo_get_wait( w->fifo_in );
        if ( buf_in != NULL )
            hb_buffer_close( &buf_in );
    }
}
