/* This file is based on Tritical's Colormatrix AviSynth Filter:
   <http://web.missouri.edu/~kes25c/>

                ColorMatrix v2.5 for Avisynth 2.5.x

  ColorMatrix 2.0 is based on the original ColorMatrix filter by Wilbert
  Dijkhof.  It adds the ability to convert between any of: Rec.709, FCC,
  Rec.601, and SMPTE 240M. It also makes pre and post clipping optional,
  adds range expansion/contraction, and more...

  Copyright (C) 2006-2009 Kevin Stone

  ColorMatrix 1.x is Copyright (C) Wilbert Dijkhof

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "hb.h"
#include "hbffmpeg.h"

#define XMIN(a,b) ((a) < (b) ? (a) : (b))
#define XMAX(a,b) ((a) > (b) ? (a) : (b))
#define CLIP(x) XMAX(XMIN((x),255),0)
#define ns(n) (((n) < 0) ? ((int)((n)*65536.0-0.5+__DBL_EPSILON__)) : (int)((n)*65536.0+0.5))

static const double yuv_coeffs_luma[2][3] =
{
    // GBR order, only care about Rec.709 or Rec.601
    { +0.7152, +0.0722, +0.2126, }, // Rec.709 (0)
    { +0.5870, +0.1140, +0.2990, }, // Rec.601 (ITU-R BT.470-2/SMPTE 170M) (1)
};

struct hb_filter_private_s
{
    int           pix_fmt;
    int           width;
    int           height;
    int           mode;
    // color conversion coeffs
    int c1, c2, c3, c4, c5, c6, c7, c8;

    AVPicture     pic_in;
    AVPicture     pic_out;
    hb_buffer_t * buf_out;
};

hb_filter_private_t * hb_colorspace_init( int pix_fmt,
                                           int width,
                                           int height,
                                           char * settings );

int hb_colorspace_work( const hb_buffer_t * buf_in,
                         hb_buffer_t ** buf_out,
                         int pix_fmt,
                         int width,
                         int height,
                         hb_filter_private_t * pv );

void hb_colorspace_close( hb_filter_private_t * pv );

hb_filter_object_t hb_filter_colorspace =
{
    FILTER_COLORSPACE,
    "Colorspace",
    NULL,
    hb_colorspace_init,
    hb_colorspace_work,
    hb_colorspace_close,
};

#define ma m[0][0]
#define mb m[0][1]
#define mc m[0][2]
#define md m[1][0]
#define me m[1][1]
#define mf m[1][2]
#define mg m[2][0]
#define mh m[2][1]
#define mi m[2][2]

#define ima im[0][0]
#define imb im[0][1]
#define imc im[0][2]
#define imd im[1][0]
#define ime im[1][1]
#define imf im[1][2]
#define img im[2][0]
#define imh im[2][1]
#define imi im[2][2]

static void inverse3x3( double im[3][3], double m[3][3] )
{
    double det = ma*(me*mi-mf*mh)-mb*(md*mi-mf*mg)+mc*(md*mh-me*mg);
    det = 1.0/det;
    ima = det*(me*mi-mf*mh);
    imb = det*(mc*mh-mb*mi);
    imc = det*(mb*mf-mc*me);
    imd = det*(mf*mg-md*mi);
    ime = det*(ma*mi-mc*mg);
    imf = det*(mc*md-ma*mf);
    img = det*(md*mh-me*mg);
    imh = det*(mb*mg-ma*mh);
    imi = det*(ma*me-mb*md);
}

static void solve_coefficients( double cm[3][3], double rgb[3][3], double yuv[3][3],
    double yiscale, double uviscale, double yoscale, double uvoscale )
{
    int i, j;

    for( i=0; i<3; ++i )
    {
        double toscale = ( i == 0 ? yoscale : uvoscale );
        for( j=0; j<3; ++j )
        {
            double tiscale = ( j == 0 ? yiscale : uviscale );
            cm[i][j] = (yuv[i][0]*rgb[0][j]+yuv[i][1]*rgb[1][j]+
                yuv[i][2]*rgb[2][j])*tiscale*toscale;
        }
    }
}

static void calc_coefficients( hb_filter_private_t * pv )
{
    int i, j, k, v;
    double yuv_coeff[2][3][3];
    double rgb_coeffd[2][3][3];
    int yuv_convert[4][3][3];
    double yuv_convertd[4][3][3];
    double yiscale, yoscale, uviscale, uvoscale;

    for( i=0; i<2; ++i )
    {
        double bscale, rscale;
        yuv_coeff[i][0][0] =  yuv_coeffs_luma[i][0];
        yuv_coeff[i][0][1] =  yuv_coeffs_luma[i][1];
        yuv_coeff[i][0][2] =  yuv_coeffs_luma[i][2];
        bscale             =  0.5/(1.0-yuv_coeff[i][0][1]);
        rscale             =  0.5/(1.0-yuv_coeff[i][0][2]);
        yuv_coeff[i][1][0] = -yuv_coeff[i][0][0]*bscale;
        yuv_coeff[i][1][1] =  (1.0-yuv_coeff[i][0][1])*bscale;
        yuv_coeff[i][1][2] = -yuv_coeff[i][0][2]*bscale;
        yuv_coeff[i][2][0] = -yuv_coeff[i][0][0]*rscale;
        yuv_coeff[i][2][1] = -yuv_coeff[i][0][1]*rscale;
        yuv_coeff[i][2][2] =  (1.0-yuv_coeff[i][0][2])*rscale;
    }

    for( i=0; i<2; ++i )
        inverse3x3( rgb_coeffd[i], yuv_coeff[i] );

    yiscale  = 1.0/219.0;
    uviscale = 1.0/224.0;
    yoscale  = 219.0;
    uvoscale = 224.0;

    v = 0;
    for ( i=0; i<2; ++i )
    {
        for ( j=0; j<2; ++j )
        {
            solve_coefficients( yuv_convertd[v], rgb_coeffd[i], yuv_coeff[j],
                yiscale, uviscale, yoscale, uvoscale );
            for ( k=0; k<3; ++k )
            {
                yuv_convert[v][k][0] = ns(yuv_convertd[v][k][0]);
                yuv_convert[v][k][1] = ns(yuv_convertd[v][k][1]);
                yuv_convert[v][k][2] = ns(yuv_convertd[v][k][2]);
            }
            if ( yuv_convert[v][0][0] != 65536 ||
                yuv_convert[v][1][0] != 0 || yuv_convert[v][2][0] != 0)
                hb_log( "colorspace: error calculating conversion coefficients" );
            ++v;
        }
    }

    pv->c1 = yuv_convert[pv->mode][0][0];
    pv->c2 = yuv_convert[pv->mode][0][1];
    pv->c3 = yuv_convert[pv->mode][0][2];
    pv->c4 = yuv_convert[pv->mode][1][1];
    pv->c5 = yuv_convert[pv->mode][1][2];
    pv->c6 = yuv_convert[pv->mode][2][1];
    pv->c7 = yuv_convert[pv->mode][2][2];
    pv->c8 = ( 67584 - yuv_convert[pv->mode][0][0] )<<4 ;
}

static int get_filter_mode( hb_filter_private_t * pv, const char * settings )
{
    int src, dst;

    if( sscanf( settings, "%d:%d", &src, &dst ) != 2 )
        return -1;

    if( src == 709 && dst == 601 )
        return 1;
    else if( src == 601 && dst == 709 )
        return 2;
    else
        return -1;
}

static void colorspace_filter( hb_filter_private_t * pv,
                                AVPicture * dst,
                                AVPicture * src )
{
    const int width    = pv->width;
    const int height   = pv->height;
    const int strideY  = src->linesize[0];
    const int strideUV = src->linesize[1];

    int x, y;
    uint8_t *srcYt  = src->data[0];
    uint8_t *srcYb  = src->data[0] + strideY;
    uint8_t *srcU   = src->data[1];
    uint8_t *srcV   = src->data[2];
    uint8_t *dstYt  = dst->data[0];
    uint8_t *dstYb  = dst->data[0] + strideY;
    uint8_t *dstU   = dst->data[1];
    uint8_t *dstV   = dst->data[2];

    for( y=0; y<height; y+=2 )
    {
        for( x=0; x<width; x+=2 )
        {
            const int u  = srcU[x>>1] - 128;
            const int v  = srcV[x>>1] - 128;
            const int uv = pv->c2*u + pv->c3*v + pv->c8;
            dstYt[x]   = CLIP((pv->c1*srcYt[x]   + uv) >> 16);
            dstYt[x+1] = CLIP((pv->c1*srcYt[x+1] + uv) >> 16);
            dstYb[x]   = CLIP((pv->c1*srcYb[x]   + uv) >> 16);
            dstYb[x+1] = CLIP((pv->c1*srcYb[x+1] + uv) >> 16);
            dstU[x>>1] = CLIP((pv->c4*u + pv->c5*v + 8421376) >> 16); // 8421376 = 128.5 to 16.16 fixed
            dstV[x>>1] = CLIP((pv->c6*u + pv->c7*v + 8421376) >> 16);
        }
        srcYt += strideY<<1;
        srcYb += strideY<<1;
        srcU  += strideUV;
        srcV  += strideUV;
        dstYt += strideY<<1;
        dstYb += strideY<<1;
        dstU  += strideUV;
        dstV  += strideUV;
    }
}

hb_filter_private_t * hb_colorspace_init( int pix_fmt,
                                           int width,
                                           int height,
                                           char * settings )
{
    if( pix_fmt != PIX_FMT_YUV420P || !settings )
    {
        return 0;
    }

    hb_filter_private_t * pv = malloc( sizeof(struct hb_filter_private_s) );

    pv->pix_fmt = pix_fmt;
    pv->width = width;
    pv->height = height;
    pv->mode = get_filter_mode( pv, settings );

    if( pv->mode > 0 )
    {
        calc_coefficients( pv );
    }
    else
    {
        return 0;
    }

    pv->buf_out = hb_video_buffer_init( width, height );

    return pv;
}

void hb_colorspace_close( hb_filter_private_t * pv )
{
    if( !pv )
    {
        return;
    }

    if( pv->buf_out )
    {
        hb_buffer_close( &pv->buf_out );
    }

    free( pv );
}

int hb_colorspace_work( const hb_buffer_t * buf_in,
                         hb_buffer_t ** buf_out,
                         int pix_fmt,
                         int width,
                         int height,
                         hb_filter_private_t * pv )
{
    if( !pv ||
        pix_fmt != pv->pix_fmt ||
        width != pv->width ||
        height != pv->height )
    {
        return FILTER_FAILED;
    }

    avpicture_fill( &pv->pic_in, buf_in->data,
                    pix_fmt, width, height );

    avpicture_fill( &pv->pic_out, pv->buf_out->data,
                    pix_fmt, width, height );

    colorspace_filter( pv, &pv->pic_out, &pv->pic_in );

    hb_buffer_copy_settings( pv->buf_out, buf_in );

    *buf_out = pv->buf_out;

    return FILTER_OK;
}

