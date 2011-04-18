#ifndef HB_HB_H
#define HB_HB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "project.h"
#include "common.h"

#ifdef __APPLE__
#include <CoreServices/CoreServices.h> // for Gestalt
#include <AudioToolbox/AudioToolbox.h>
#include <dlfcn.h>
#endif
/* Whether the Core Audio HE-AAC encoder is available on the system. */
int encca_haac_available();

/* hb_init()
   Initializes a libhb session (launches his own thread, detects CPUs,
   etc) */
#define HB_DEBUG_NONE 0
#define HB_DEBUG_ALL  1
void          hb_register( hb_work_object_t * );
void          hb_register_logger( void (*log_cb)(const char* message) );
hb_handle_t * hb_init( int verbose, int update_check );
hb_handle_t * hb_init_dl ( int verbose, int update_check ); // hb_init for use with dylib

/* hb_get_version() */
char        * hb_get_version( hb_handle_t * );
int           hb_get_build( hb_handle_t * );

/* hb_check_update()
   Checks for an update on the website. If there is, returns the build
   number and points 'version' to a version description. Returns a
   negative value otherwise. */
int           hb_check_update( hb_handle_t * h, char ** version );


char *        hb_dvd_name( char * path );
void          hb_dvd_set_dvdnav( int enable );

/* hb_scan()
   Scan the specified path. Can be a DVD device, a VIDEO_TS folder or
   a VOB file. If title_index is 0, scan all titles. */
void          hb_scan( hb_handle_t *, const char * path,
                       int title_index, int preview_count,
                       int store_previews, uint64_t min_duration );
void          hb_scan_stop( hb_handle_t * );
hb_filter_object_t * hb_get_filter_object(int filter_id, const char * settings);
uint64_t      hb_first_duration( hb_handle_t * );

/* hb_get_titles()
   Returns the list of valid titles detected by the latest scan. */
hb_list_t   * hb_get_titles( hb_handle_t * );

/* hb_detect_comb()
   Analyze a frame for interlacing artifacts, returns true if they're found.
   Taken from Thomas Oestreich's 32detect filter in the Transcode project.  */
int hb_detect_comb( hb_buffer_t * buf, int width, int height, int color_equal, int color_diff, int threshold, int prog_equal, int prog_diff, int prog_threshold );

void          hb_get_preview_by_index( hb_handle_t *, int, int, uint8_t * );
void          hb_get_preview( hb_handle_t *, hb_title_t *, int,
                              uint8_t * );
void          hb_set_size( hb_job_t *, double ratio, int pixels );
void          hb_set_anamorphic_size_by_index( hb_handle_t *, int,
                int *output_width, int *output_height,
                int *output_par_width, int *output_par_height );
void          hb_set_anamorphic_size( hb_job_t *,
                int *output_width, int *output_height,
                int *output_par_width, int *output_par_height );

/* Handling jobs */
int           hb_count( hb_handle_t * );
hb_job_t *    hb_job( hb_handle_t *, int );
void          hb_set_chapter_name( hb_handle_t *, int, int, const char * );
void          hb_set_job( hb_handle_t *, int, hb_job_t * );
void          hb_add( hb_handle_t *, hb_job_t * );
void          hb_rem( hb_handle_t *, hb_job_t * );

void          hb_start( hb_handle_t * );
void          hb_pause( hb_handle_t * );
void          hb_resume( hb_handle_t * );
void          hb_stop( hb_handle_t * );

/* Persistent data between jobs. */
typedef struct hb_interjob_s
{
    int last_job;          /* job->sequence_id & 0xFFFFFF */
    int frame_count;       /* number of frames counted by sync */
    int out_frame_count;   /* number of frames counted by render */
    uint64_t total_time;   /* real length in 90kHz ticks (i.e. seconds / 90000) */
    int vrate;             /* actual measured output vrate from 1st pass */
    int vrate_base;        /* actual measured output vrate_base from 1st pass */

    hb_subtitle_t *select_subtitle; /* foreign language scan subtitle */
} hb_interjob_t;

hb_interjob_t * hb_interjob_get( hb_handle_t * ); 

/* hb_get_state()
   Should be regularly called by the UI (like 5 or 10 times a second).
   Look at test/test.c to see how to use it. */
void hb_get_state( hb_handle_t *, hb_state_t * );
void hb_get_state2( hb_handle_t *, hb_state_t * );
/* hb_get_scancount() is called by the MacGui in UpdateUI to
   check for a new scan during HB_STATE_WORKING phase  */
int hb_get_scancount( hb_handle_t * );

/* hb_close()
   Aborts all current jobs if any, frees memory. */
void          hb_close( hb_handle_t ** );

/* hb_global_close()
   Performs final cleanup for the process. */
void          hb_global_close();

/* hb_get_instance_id()
   Return the unique instance id of an libhb instance created by hb_init. */
int hb_get_instance_id( hb_handle_t * h );

#ifdef __cplusplus
}
#endif

#endif
