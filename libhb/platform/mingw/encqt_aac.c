/*
   This file is part of the HandBrake source code.
   Homepage: <http://handbrake.fr/>.
   It may be used under the terms of the GNU General Public License. */

/* This file is based on the source code of qtaacenc:
 <http://tmkk.hp.infoseek.co.jp/qtaacenc/>.

 Copyright (c) 2010 tmkk <tmkk@smoug.net> */

#if defined( __MINGW32__ ) && defined ( USE_QT_AAC )

#undef __FLT_EVAL_METHOD__
#define __FLT_EVAL_METHOD__ 1
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#define __CFTREE__
#include <QTML.h>
#include <QuickTimeComponents.h>
#undef __CFTREE__
#include "hb.h"
#include "downmix.h"

#define kAudioCodecBitRateControlMode_VariableConstrained (2)
#define kAudioConverterQuality_Max (2)

enum AAC_MODE { AAC_MODE_LC, AAC_MODE_HE };

/* indicates that QuickTime Components has been initialized. */
/* -1 : not yet */
/*  0 : QTML Client is unavailable */
/*  1 : already initialized */
static volatile int quicktime_initialized = -1;

/* indicates available AAC profile with installed QuickTime version. */
/* -1 : not checked yet */
/*  0 : QuickTime is not installed on the system */
/*  1 : only AAC-LC is available */
/*  2 : AAC-HE is available */
static volatile int qt_aac_available_profile = -1;

int     encQuickTimeAudioInitLC( hb_work_object_t *, hb_job_t * );
int     encQuickTimeAudioInitHE( hb_work_object_t *, hb_job_t * );
int     encQuickTimeAudioInit( hb_work_object_t *, hb_job_t *, enum AAC_MODE );
int     encQuickTimeAudioWork( hb_work_object_t *, hb_buffer_t **, hb_buffer_t ** );
void    encQuickTimeAudioClose( hb_work_object_t * );

hb_work_object_t hb_encqt_aac =
{
    WORK_ENC_QT_AAC,
    "AAC encoder (QuickTime)",
    encQuickTimeAudioInitLC,
    encQuickTimeAudioWork,
    encQuickTimeAudioClose
};

hb_work_object_t hb_encqt_haac =
{
    WORK_ENC_QT_HAAC,
    "HE-AAC encoder (QuickTime)",
    encQuickTimeAudioInitHE,
    encQuickTimeAudioWork,
    encQuickTimeAudioClose
};

struct hb_work_private_s
{
    hb_job_t *job;

    ComponentInstance ci;
    AudioStreamBasicDescription input;
    AudioStreamBasicDescription output;
    uint8_t  *obuf;
    uint8_t  *buf;
    hb_list_t *list;
    unsigned long isamples, isamplesiz, omaxpacket, nchannels;
    uint64_t pts, ibytes;
    Float64 osamplerate;
    hb_chan_map_t *in_map; // QT Component does not have direct method to specify the channel remapping method
                           // like AudioConverterSetProperty( ..., kAudioConverterChannelMap, ...) in CoreAudio.
                           // So manually remap Channels before feeding input samples.
};

#define MP4ESDescrTag                   0x03
#define MP4DecConfigDescrTag            0x04
#define MP4DecSpecificDescrTag          0x05

// based off of mov_mp4_read_descr_len from mov.c in ffmpeg's libavformat
static int readDescrLen( uint8_t **buffer )
{
        int len = 0;
        int count = 4;
        while (count--) {
                int c = *(*buffer)++;
                len = (len << 7) | (c & 0x7f);
                if (!(c & 0x80))
                        break;
        }
        return len;
}

// based off of mov_mp4_read_descr from mov.c in ffmpeg's libavformat
static int readDescr( uint8_t **buffer, int *tag)
{
        *tag = *(*buffer)++;
        return readDescrLen(buffer);
}

// based off of mov_read_esds from mov.c in ffmpeg's libavformat
static long ReadESDSDescExt( void* descExt, uint8_t **buffer, UInt32 *size, int versionFlags)
{
        uint8_t *esds = (uint8_t *) descExt;
        int tag, len;
        *size = 0;

    if (versionFlags)
        esds += 4;              // version + flags
        readDescr(&esds, &tag);
        esds += 2;              // ID
        if (tag == MP4ESDescrTag)
                esds++;         // priority

        readDescr(&esds, &tag);
        if (tag == MP4DecConfigDescrTag) {
                esds++;         // object type id
                esds++;         // stream type
                esds += 3;      // buffer size db
                esds += 4;      // max bitrate
                esds += 4;      // average bitrate

                len = readDescr(&esds, &tag);
                if (tag == MP4DecSpecificDescrTag) {
                        *buffer = calloc(1, len + 8);
                        if (*buffer) {
                                memcpy(*buffer, esds, len);
                                *size = len;
                        }
                }
        }

        return noErr;
}

static int get_samplerate_index( int samplerate, enum AAC_MODE mode )
{
    int he = !!( mode == AAC_MODE_HE );

    switch( samplerate )
    {
        case 22050: return  he ? -1 : 5;
        case 24000: return  he ? -1 : 6;
        case 32000: return  he ?  1 : 7;
        case 44100: return  he ?  2 : 8;
        case 48000: return  he ?  3 : 9;
        default:    return -1;
    }
}

static int get_channel_configuration_index( int mixdown, enum AAC_MODE mode )
{
    int he = !!( mode == AAC_MODE_HE );

    switch( mixdown )
    {
        case HB_AMIXDOWN_MONO:
            return 0;
        case HB_AMIXDOWN_STEREO:
        case HB_AMIXDOWN_DOLBY:
        case HB_AMIXDOWN_DOLBYPLII:
            return 1;
        case HB_AMIXDOWN_6CH:
            return he ? 3 : 5;
        default:
            return -1;
    }
}

static int validate_bitrate( CFArrayRef config, SInt32 bitrate )
{
    SInt32 actual_bitrate = bitrate, bitrate_index;
    CFDictionaryRef settings = (CFDictionaryRef)CFArrayGetValueAtIndex( config, 0 );
    CFArrayRef      params   = (CFArrayRef)CFDictionaryGetValue( settings, CFSTR("parameters") );
    CFIndex         i;

    for ( i = 0; i < CFArrayGetCount( params ); i++ )
    {
        CFDictionaryRef key = (CFDictionaryRef)CFArrayGetValueAtIndex( params, i );

        if( CFStringCompare( (CFStringRef)CFDictionaryGetValue( key, CFSTR("key") ), CFSTR("Bit Rate"), 0 ) )
        {
            continue;
        }
        else
        {
            CFArrayRef limited_values = NULL;

            limited_values = (CFArrayRef)CFDictionaryGetValue( key, CFSTR("limited values") );
            if ( limited_values )
            {
                SInt32 min =  CFStringGetIntValue( (CFStringRef)CFArrayGetValueAtIndex( limited_values, 1 ) );
                SInt32 max =  CFStringGetIntValue( (CFStringRef)CFArrayGetValueAtIndex( limited_values, CFArrayGetCount(limited_values)-1 ) );

                if( bitrate <= min )
                    actual_bitrate = min;
                if( bitrate >= max )
                    actual_bitrate = max;
            }

            CFArrayRef available_values = (CFArrayRef)CFDictionaryGetValue( key, CFSTR("available values") );
            CFIndex j;

            for( j = 1; j < CFArrayGetCount(available_values) - 1; j++ )
            {
                SInt32 lo = CFStringGetIntValue( (CFStringRef)CFArrayGetValueAtIndex( available_values, j ) );
                SInt32 hi = CFStringGetIntValue( (CFStringRef)CFArrayGetValueAtIndex( available_values, (j+1) ) );

                if( actual_bitrate == lo )
                {
                    bitrate_index = j;
                    break;
                }
                else if( actual_bitrate == hi )
                {
                    bitrate_index = j + 1;
                    break;
                }
                else if( actual_bitrate > lo && actual_bitrate < hi )
                {
                    SInt32 center = (lo + hi)>>1 ;
                    if( actual_bitrate < center )
                    {
                        actual_bitrate = lo;
                        bitrate_index = j;
                        break;
                    }
                    else
                    {
                        actual_bitrate = hi;
                        bitrate_index = j+1;
                        break;
                    }
                }
            }
        }
    }

    if( actual_bitrate != bitrate )
        hb_log( "qt_aac: Audio bitrate is changed from %d kbps to %d kbps due to encoder's limitation",
                bitrate, actual_bitrate );

    return bitrate_index;
}

#define SETVALUE( name, type, value ) {\
    CFMutableDictionaryRef paramkey = CFDictionaryCreateMutableCopy( NULL, 0, param );\
    CFNumberRef num = CFNumberCreate( NULL, type, &value );\
    CFDictionarySetValue( paramkey, CFSTR(name), num );\
    CFRelease( num );\
    CFArrayAppendValue( new_params, paramkey );\
    CFRelease( paramkey );\
}

static CFArrayRef configure_codec_settings_array( CFArrayRef current_array, int encoder_mode, int encoder_quality,
                                                  int channel_config_index, int samplerate_index, int bitrate_index )
{
    CFMutableDictionaryRef current_settings = CFDictionaryCreateMutableCopy( NULL, 0, (CFDictionaryRef)CFArrayGetValueAtIndex( current_array, 0 ) );
    CFArrayRef               current_params = CFDictionaryGetValue( current_settings, CFSTR("parameters") );
    CFMutableArrayRef            new_params = CFArrayCreateMutable( NULL, 0, &kCFTypeArrayCallBacks );
    CFMutableArrayRef             new_array = CFArrayCreateMutable( NULL, 0, &kCFTypeArrayCallBacks );
    CFIndex i;

    for( i = 0; i<CFArrayGetCount( current_params ); i++ )
    {
        CFDictionaryRef param           = CFArrayGetValueAtIndex( current_params, i );

        if( !CFStringCompare( (CFStringRef)CFDictionaryGetValue( param, CFSTR("key") ), CFSTR("Target Format"), 0 ) )
            SETVALUE( "current value", kCFNumberSInt32Type, encoder_mode )
        else if( !CFStringCompare( (CFStringRef)CFDictionaryGetValue( param, CFSTR("key") ), CFSTR("Quality"), 0 ) )
            SETVALUE( "current value", kCFNumberSInt32Type, encoder_quality )
        else if( !CFStringCompare( (CFStringRef)CFDictionaryGetValue( param, CFSTR("key") ), CFSTR("Sample Rate"), 0 ) )
            SETVALUE( "current value", kCFNumberSInt32Type, samplerate_index )
        else if( !CFStringCompare( (CFStringRef)CFDictionaryGetValue( param, CFSTR("key") ), CFSTR("Bit Rate"), 0 ) )
            SETVALUE( "current value", kCFNumberSInt32Type, bitrate_index )
        else
            CFArrayAppendValue( new_params, param );
    }

    CFDictionarySetValue( current_settings, CFSTR("parameters"), new_params );
    CFRelease( new_params );
    CFArrayAppendValue( new_array, current_settings );
    CFRelease( current_settings );

    return new_array;
}

OSStatus configure_quicktime_component( hb_work_private_t * pv, hb_audio_t * audio, enum AAC_MODE mode )
{
    int encoder_mode = kAudioCodecBitRateControlMode_VariableConstrained;
    int encoder_quality = kAudioConverterQuality_Max;
    int samplerate_index;
    int channel_config_index;
    int bitrate_index;
    CFArrayRef config=NULL;
    OSStatus ret = -1;
    OSStatus err;

    // pass the number of channels used into the private work data
    pv->nchannels = HB_AMIXDOWN_GET_DISCRETE_CHANNEL_COUNT( audio->config.out.mixdown );

    memset( &pv->input, 0, sizeof( AudioStreamBasicDescription ) );
    pv->input.mSampleRate = ( Float64 ) audio->config.out.samplerate;
    pv->input.mFormatID = kAudioFormatLinearPCM;
    pv->input.mFormatFlags = kLinearPCMFormatFlagIsFloat | kAudioFormatFlagsNativeEndian;
    pv->input.mBytesPerPacket = 4 * pv->nchannels;
    pv->input.mFramesPerPacket = 1;
    pv->input.mBytesPerFrame = pv->input.mBytesPerPacket * pv->input.mFramesPerPacket;
    pv->input.mChannelsPerFrame = pv->nchannels;
    pv->input.mBitsPerChannel = 32;

    err = QTSetComponentProperty( pv->ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_InputBasicDescription, sizeof(pv->input), &pv->input );
    if( err != noErr )
        goto last;

    memset( &pv->output, 0, sizeof( AudioStreamBasicDescription ) );
    pv->output.mSampleRate = ( Float64 ) audio->config.out.samplerate;
    switch ( mode )
    {
        case AAC_MODE_HE:
            pv->output.mFormatID = kAudioFormatMPEG4AAC_HE;
            break;
        case AAC_MODE_LC:
        default:
            pv->output.mFormatID = kAudioFormatMPEG4AAC;
            break;
    }
    pv->output.mChannelsPerFrame = pv->nchannels;

    err = QTSetComponentProperty( pv->ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_BasicDescription, sizeof(pv->output), &pv->output );
    if( err != noErr )
        goto last;
    err = QTGetComponentProperty( pv->ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_CodecSpecificSettingsArray, sizeof(config), &config, NULL );
    if( err != noErr )
        goto last;

    /* need to set encoder mode and samplerate beforehand to retrieve correct bitrate value ranges */
    /* bitrate will be set by next call of QTSetComponentProperty */
    samplerate_index     = get_samplerate_index( audio->config.out.samplerate, mode );
    channel_config_index = get_channel_configuration_index( audio->config.out.mixdown, mode );

    config = configure_codec_settings_array( config, encoder_mode, encoder_quality, channel_config_index, samplerate_index, 0 );
    err = QTSetComponentProperty( pv->ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_CodecSpecificSettingsArray, sizeof(config), &config);
    if( err != noErr )
        goto last;

    /* retrieve updated settings array */
    err = QTGetComponentProperty( pv->ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_CodecSpecificSettingsArray, sizeof(config), &config, NULL );
    if( err != noErr )
        goto last;

    /* check bitrate is in available ranges and set the bitrate */
    bitrate_index        = validate_bitrate( config, audio->config.out.bitrate );

    config = configure_codec_settings_array( config, encoder_mode, encoder_quality, channel_config_index, samplerate_index, bitrate_index );
    err = QTSetComponentProperty( pv->ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_CodecSpecificSettingsArray, sizeof(config), &config);
    if( err != noErr )
        goto last;

    ret = noErr;

last:
    if( config ) CFRelease( config );
    return ret;
}

enum QT_VERSION { QT_NO_AAC, QT_LC_ONLY, QT_HE_READY };

static enum QT_VERSION check_quicktime_version( void )
{
    long version = 0;

    Gestalt( gestaltQuickTimeVersion, &version );
    if( version < 0x07620000 )
    {
        hb_log( "qt_aac: QuickTime is too old! Install latest QuickTime" );
        return QT_NO_AAC;
    }
    else if( version < 0x07640000 )
    {
        hb_log( "qt_aac: Only AAC-LC is available. Update QuickTime to 7.6.4 or later for HE-AAC" );
        return QT_LC_ONLY;
    }
    else
    {
        hb_log( "qt_aac: Found an HE-AAC capable QuickTime" );
        return QT_HE_READY;
    }
}

#ifndef MAX_PATH
#define MAX_PATH (260)
#endif

static int register_quicktime_heaac_component( void )
{
    HKEY hKey;
    char *subKey = "SOFTWARE\\Apple Computer, Inc.\\QuickTime";
    char *entry = "QTSysDir";
    DWORD size;
    char path[MAX_PATH+1] = { 0 };
    char keypath[MAX_PATH+1];
    int ret = -1;

    if( RegOpenKeyEx( HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey ) == ERROR_SUCCESS )
    {
        if( RegQueryValueEx( hKey, entry, NULL, NULL, NULL, &size ) == ERROR_SUCCESS )
        {
            if( size > MAX_PATH - strlen( "QuickTimeAudioSupport.qtx" ) )
            {
                RegCloseKey( hKey );
                hb_log( "qt_aac: QuickTime install path is too long" );
                return -1;
            }
            RegQueryValueEx( hKey, entry, NULL, NULL, (LPBYTE)keypath, &size );
            RegCloseKey( hKey );
            snprintf( path, MAX_PATH, "%sQuickTimeAudioSupport.qtx", keypath );
        }
        else
        {
            RegCloseKey( hKey );
            hb_log( "qt_aac: Cannot find QuickTime install path in Windows registry" );
            return -1;
        }
    }
    else
    {
        hb_log( "qt_aac: Cannot find QuickTime install path in Windows registry" );
        return -1;
    }

    if( GetFileAttributes( path ) != -1 )
    {
        ComponentDescription cd;
        cd.componentType = 'aenc';
        cd.componentSubType = kAudioFormatMPEG4AAC_HE;
        cd.componentManufacturer = kAppleManufacturer;
        cd.componentFlags = 0;
        cd.componentFlagsMask = 0;
        ComponentResult (*ComponentRoutine)( ComponentParameters *, Handle );
        HMODULE h = LoadLibrary( path );

        ComponentRoutine = ( ComponentResult(__cdecl *)( ComponentParameters *, Handle ) )GetProcAddress( h, "ACMP4AACHighEfficiencyEncoderEntry" );
        if( ComponentRoutine )
        {
            if( RegisterComponent( &cd, ComponentRoutine, 0, NULL, NULL, NULL ) )
            {
                ret = 0;
                hb_log( "qt_aac: QuickTime HE-AAC encoder component is successfully registered" );
            }
        }
        FreeLibrary( h );
    }

    return ret;
}

int encqt_aac_available_profile( void )
{
    if( quicktime_initialized == -1 )
    {
        if( InitializeQTML( 0 ) == noErr )
        {
            EnterMovies();
            quicktime_initialized = 1;
        }
        else
        {
            quicktime_initialized = 0;
            qt_aac_available_profile = 0;
            hb_log( "qt_aac: QuickTime is not available! Install latest QuickTime" );
            return qt_aac_available_profile;
        }
    }

    if( qt_aac_available_profile != -1 )
        return qt_aac_available_profile;

    qt_aac_available_profile = 0;

    switch( check_quicktime_version() )
    {
        case QT_HE_READY:
            if( register_quicktime_heaac_component() == 0 )
                qt_aac_available_profile = 2;
            else
                qt_aac_available_profile = 1;
            break;
        case QT_LC_ONLY:
            qt_aac_available_profile = 1;
            break;
        case QT_NO_AAC:
        default:
            qt_aac_available_profile = 0;
    }

    return qt_aac_available_profile;
}

int encQuickTimeAudioInitLC( hb_work_object_t * w, hb_job_t * job )
{
    return encQuickTimeAudioInit( w, job, AAC_MODE_LC );
}

int encQuickTimeAudioInitHE( hb_work_object_t * w, hb_job_t * job )
{
    return encQuickTimeAudioInit( w, job, AAC_MODE_HE );
}

/***********************************************************************
 * hb_work_encQuickTimeAudio_init
 ***********************************************************************
 *
 **********************************************************************/
int encQuickTimeAudioInit( hb_work_object_t * w, hb_job_t * job, enum AAC_MODE mode )
{
    hb_work_private_t * pv = calloc( 1, sizeof( hb_work_private_t ) );
    hb_audio_t * audio = w->audio;
    OSStatus err;
    UInt32 tmp, tmpsiz = sizeof(UInt32);

    w->private_data = pv;
    pv->job = job;

    err = OpenADefaultComponent( StandardCompressionType, StandardCompressionSubTypeAudio, &pv->ci );
    if( err != noErr )
    {
        hb_log( "qt_aac: Cannot create an QuickTime AAC encoder instance" );
        goto die;
    }

    if( configure_quicktime_component( pv, audio, mode ) != noErr )
        goto die;

    // get real input
    err = QTGetComponentProperty( pv->ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_InputBasicDescription,
                                  sizeof(AudioStreamBasicDescription), &pv->input, NULL );
    if( err != noErr )
        goto die;

    // get real output
    err = QTGetComponentProperty( pv->ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_BasicDescription,
                                  sizeof(AudioStreamBasicDescription), &pv->output, NULL );
    if( err != noErr )
        goto die;

    if( ( audio->config.out.mixdown == HB_AMIXDOWN_6CH ) && ( audio->config.in.channel_map != &hb_qt_chan_map ) )
        pv->in_map = audio->config.in.channel_map;
    else
        pv->in_map = NULL;

    // set sizes
    pv->isamplesiz  = pv->input.mBytesPerPacket;
    pv->isamples    = audio->config.out.samples_per_frame = pv->output.mFramesPerPacket;
    pv->osamplerate = pv->output.mSampleRate;

    // get maximum output size
    err = QTGetComponentProperty( pv->ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_MaximumOutputPacketSize,
                            sizeof(UInt32), &tmp, NULL );
    if( err != noErr )
        goto die;
    pv->omaxpacket = tmp;

    // get magic cookie (elementary stream descriptor)
    tmp = HB_CONFIG_MAX_SIZE;
    err = QTGetComponentProperty( pv->ci, kQTPropertyClass_SCAudio, kQTSCAudioPropertyID_MagicCookie,
                            tmp, w->config->aac.bytes, &tmp);

    if( err != noErr )
        goto die;

    // CoreAudio returns a complete ESDS, but we only need
    // the DecoderSpecific info.
    UInt8* buffer = NULL;
    ReadESDSDescExt(w->config->aac.bytes, &buffer, &tmpsiz, 0);
    w->config->aac.length = tmpsiz;
    memmove( w->config->aac.bytes, buffer,
             w->config->aac.length );

    pv->list = hb_list_init();
    pv->buf = NULL;

    return 0;

die:
    hb_log( "qt_aac: Error when configuring QuickTime AAC encoder" );
    *job->die = 1;
    return 0;
}

/***********************************************************************
 * Close
 ***********************************************************************
 *
 **********************************************************************/
void encQuickTimeAudioClose( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;

    if ( pv->ci )
        CloseComponent( pv->ci );
    if ( pv->list )
        hb_list_empty( &pv->list );
    if ( pv->obuf )
    {
        free( pv->obuf );
        pv->obuf = NULL;
    }
    if ( pv->buf )
    {
        free( pv->buf );
        pv->buf = NULL;
    }

    free( pv );
    w->private_data = NULL;
}

static OSStatus inInputDataProc( ComponentInstance ci,
                          UInt32 *npackets,
                          AudioBufferList *buffers,
                          AudioStreamPacketDescription **ignored,
                          void *userdata )
{
    hb_work_private_t *pv = userdata;

    if( pv->ibytes == 0 )
    {
        *npackets = 0;
        hb_log( "qt_aac: no data to use in inInputDataProc" );
        return 1;
    }

    if( pv->buf != NULL )
        free( pv->buf );

    uint64_t pts, pos;
    buffers->mBuffers[0].mDataByteSize = MIN( *npackets * pv->isamplesiz, pv->ibytes );
    buffers->mBuffers[0].mData = pv->buf = calloc(1 , buffers->mBuffers[0].mDataByteSize );

    if( hb_list_bytes( pv->list ) >= buffers->mBuffers[0].mDataByteSize )
    {
        hb_list_getbytes( pv->list, buffers->mBuffers[0].mData,
                          buffers->mBuffers[0].mDataByteSize, &pts, &pos );
    }
    else
    {
        hb_log( "qt_aac: Not enought data, exiting inInputDataProc" );
        *npackets = 0;
        return 1;
    }

    *npackets = buffers->mBuffers[0].mDataByteSize / pv->isamplesiz;

    pv->ibytes -= buffers->mBuffers[0].mDataByteSize;

    return noErr;
}

/***********************************************************************
 * Encode
 ***********************************************************************
 *
 **********************************************************************/
static hb_buffer_t * Encode( hb_work_object_t * w )
{
    hb_work_private_t * pv = w->private_data;
    UInt32 npackets = 1;

    /* check if we need more data */
    if( ( pv->ibytes = hb_list_bytes( pv->list ) ) < pv->isamples * pv->isamplesiz )
        return NULL;

    hb_buffer_t * obuf;
    AudioStreamPacketDescription odesc = { 0 };
    AudioBufferList obuflist = { .mNumberBuffers = 1,
                                 .mBuffers = { { .mNumberChannels = pv->nchannels } },
                               };

    obuf = hb_buffer_init( pv->omaxpacket );
    obuflist.mBuffers[0].mDataByteSize = obuf->size;
    obuflist.mBuffers[0].mData = obuf->data;

    OSStatus err = SCAudioFillBuffer( pv->ci, inInputDataProc, pv,
                                     &npackets, &obuflist, &odesc );
    if( err ) {
        hb_log( "qt_aac: Not enough data" );
        return NULL;
    }
    if( odesc.mDataByteSize == 0 || npackets == 0 ) {
        hb_log( "qt_aac: 0 packets returned " );
        return NULL;
    }

    obuf->start = pv->pts;
    pv->pts += 90000LL * pv->isamples / pv->osamplerate;
    obuf->stop  = pv->pts;
    obuf->size  = odesc.mDataByteSize;
    obuf->frametype = HB_FRAME_AUDIO;

    return obuf;
}

static hb_buffer_t *Flush( hb_work_object_t *w, hb_buffer_t *bufin )
{
    hb_work_private_t *pv = w->private_data;

    // pad whatever data we have out to four input frames.
    int nbytes = hb_list_bytes( pv->list );
    int pad = pv->isamples * pv->isamplesiz - nbytes;
    if ( pad > 0 )
    {
        hb_buffer_t *tmp = hb_buffer_init( pad );
        memset( tmp->data, 0, pad );
        hb_list_add( pv->list, tmp );
    }

    hb_buffer_t *bufout = NULL, *buf = NULL;
    while ( hb_list_bytes( pv->list ) >= pv->isamples * pv->isamplesiz )
    {
        hb_buffer_t *b = Encode( w );
        if ( b )
        {
            if ( bufout == NULL )
            {
                bufout = b;
            }
            else
            {
                buf->next = b;
            }
            buf = b;
        }
    }
    // add the eof marker to the end of our buf chain
    if ( buf )
        buf->next = bufin;
    else
        bufout = bufin;
    return bufout;
}

/***********************************************************************
 * Work
 ***********************************************************************
 *
 **********************************************************************/
int encQuickTimeAudioWork( hb_work_object_t * w, hb_buffer_t ** buf_in,
                  hb_buffer_t ** buf_out )
{
    hb_work_private_t * pv = w->private_data;
    hb_buffer_t * buf;

    if( (*buf_in)->size <= 0 )
    {
        // EOF on input. Finish encoding what we have buffered then send
        // it & the eof downstream.
        *buf_out = Flush( w, *buf_in );
        *buf_in = NULL;
        return HB_WORK_DONE;
    }

    if( (*buf_in)->size > 0 && pv->in_map )
    {
        hb_layout_remap( pv->in_map, &hb_qt_chan_map,
                         (HB_INPUT_CH_LAYOUT_3F2R|HB_INPUT_CH_LAYOUT_HAS_LFE),
                         (hb_sample_t *)(*buf_in)->data, (*buf_in)->size/pv->isamplesiz );
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

#endif /* defined( __MINGW32__ ) && defined ( USE_QT_AAC ) */

