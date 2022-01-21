/* Copyright (C) 2000-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fontforge-config.h>

#if !defined(X_DISPLAY_MISSING) && !defined(FONTFORGE_CAN_USE_GDK)

#include "gxcdrawP.h"
#include "gxdrawP.h"

#include <math.h>
#include <string.h>

/* On the cygwin X server masking with mono images is broken */
#ifdef _BrokenBitmapImages
# undef FAST_BITS
# define FAST_BITS 0
#endif

/* On some X displays (my linux box for instance) bitmap drawing is very */
/*  slow when compared to 24bit drawing. So if FAST_BITS is set then use */
/*  1 bit masks otherwise use the depth of the screen */
#ifndef FAST_BITS
#define FAST_BITS 0
#endif

static void intersect_rectangles(GRect *rect, GRect *clip) {
    if ( rect->x < clip->x ) {
	rect->width -= (clip->x - rect->x);
	if ( rect->width < 0 ) rect->width = 0;
	rect->x = clip->x;
    }
    if ( rect->x + rect->width > clip->x + clip->width ) {
        rect->width = clip->x + clip->width - rect->x;
        if ( rect->width < 0 ) rect->width = 0;
    }
    if ( rect->y < clip->y ) {
	rect->height -= (clip->y - rect->y);
	if ( rect->height < 0 ) rect->height = 0;
	rect->y = clip->y;
    }
    if ( rect->y + rect->height > clip->y + clip->height ) {
        rect->height = clip->y + clip->height - rect->y;
        if ( rect->height < 0 ) rect->height = 0;
    }
}

static void gdraw_8_on_1_nomag_dithered_masked(GXDisplay *gdisp,GImage *image,
	GRect *src) {
    struct gcol clut[256];
    int i,j, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    unsigned char *pt, *ipt, *mpt;
    short *g_d;
    register int gd;
    struct gcol *pos;
    int bit;

    _GDraw_getimageclut(base,clut);

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.green_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (unsigned char *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (unsigned char *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (unsigned char *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
	if ( gdisp->gg.img->bitmap_bit_order == MSBFirst )
	    bit = 0x80;
	else
	    bit = 0x1;
	gd = 0;
	g_d = gdisp->gg.green_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
		*mpt |= bit;
		*ipt &= ~bit;
		++g_d;
	    } else {
		*mpt &= ~bit;
		pos = &clut[index];
		gd += *g_d + pos->red + pos->green + pos->blue;
		if ( gd<3*128 ) {
		    *ipt &= ~bit;
		    *g_d++ = gd /= 2;
		} else {
		    *ipt |= bit;
		    *g_d++ = gd = (gd - 3*255)/2;
		}
	    }
	    if ( gdisp->gg.img->bitmap_bit_order == MSBFirst ) {
		if (( bit>>=1 )==0 ) {bit=0x80; ++ipt; ++mpt;};
	    } else {
		if (( bit<<=1 )==256 ) {bit=0x1; ++ipt; ++mpt;};
	    }
	}
    }
}

static void gdraw_32_on_1_nomag_dithered_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint32_t *pt;
    uint8_t *ipt, *mpt;
    short *g_d;
    register int gd;
    int bit;

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.green_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
	if ( gdisp->gg.img->bitmap_bit_order == MSBFirst )
	    bit = 0x80;
	else
	    bit = 0x1;
	gd = 0;
	g_d = gdisp->gg.green_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
		*mpt |= bit;
		*ipt &= ~bit;
		++g_d;
	    } else {
		*mpt &= ~bit;
		gd += *g_d + COLOR_RED(index) + COLOR_GREEN(index) + COLOR_BLUE(index);
		if ( gd<3*128 ) {
		    *ipt &= ~bit;
		    *g_d++ = gd /= 2;
		} else {
		    *ipt |= bit;
		    *g_d++ = gd = (gd - 3*255)/2;
		}
	    }
	    if ( gdisp->gg.img->bitmap_bit_order == MSBFirst ) {
		if (( bit>>=1 )==0 ) {bit=0x80; ++ipt; ++mpt;};
	    } else {
		if (( bit<<=1 )==256 ) {bit=0x1; ++ipt; ++mpt;};
	    }
	}
    }
}

static void gdraw_32a_on_1_nomag_dithered(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    unsigned int index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint32_t *pt;
    uint8_t *ipt, *mpt;
    short *g_d;
    register int gd;
    int bit;

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.green_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
	if ( gdisp->gg.img->bitmap_bit_order == MSBFirst )
	    bit = 0x80;
	else
	    bit = 0x1;
	gd = 0;
	g_d = gdisp->gg.green_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans || (index>>24)<0x80 ) {
		*mpt |= bit;
		*ipt &= ~bit;
		++g_d;
	    } else {
		*mpt &= ~bit;
		gd += *g_d + COLOR_RED(index) + COLOR_GREEN(index) + COLOR_BLUE(index);
		if ( gd<3*128 ) {
		    *ipt &= ~bit;
		    *g_d++ = gd /= 2;
		} else {
		    *ipt |= bit;
		    *g_d++ = gd = (gd - 3*255)/2;
		}
	    }
	    if ( gdisp->gg.img->bitmap_bit_order == MSBFirst ) {
		if (( bit>>=1 )==0 ) {bit=0x80; ++ipt; ++mpt;};
	    } else {
		if (( bit<<=1 )==256 ) {bit=0x1; ++ipt; ++mpt;};
	    }
	}
    }
}

static void gdraw_8_on_1_nomag_dithered_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    int i,j, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    unsigned char *pt, *ipt;
    short *g_d;
    register int gd;
    struct gcol *pos;
    int bit;

    _GDraw_getimageclut(base,clut);

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.green_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (unsigned char *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (unsigned char *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	if ( gdisp->gg.img->bitmap_bit_order == MSBFirst )
	    bit = 0x80;
	else
	    bit = 0x1;
	gd = 0;
	g_d = gdisp->gg.green_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    pos = &clut[index];
	    gd += *g_d + pos->red + pos->green + pos->blue;
	    if ( gd<3*128 ) {
		*ipt &= ~bit;
		*g_d++ = gd /= 2;
	    } else {
		*ipt |= bit;
		*g_d++ = gd = (gd - 3*255)/2;
	    }
	    if ( gdisp->gg.img->bitmap_bit_order == MSBFirst ) {
		if (( bit>>=1 )==0 ) {bit=0x80; ++ipt;};
	    } else {
		if (( bit<<=1 )==256 ) {bit=0x1; ++ipt;};
	    }
	}
    }
}

static void gdraw_32_on_1_nomag_dithered_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    int i,j, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    uint32_t *pt;
    uint8_t *ipt;
    short *g_d;
    register int gd;
    int bit;

    _GDraw_getimageclut(base,clut);

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.green_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	if ( gdisp->gg.img->bitmap_bit_order == MSBFirst )
	    bit = 0x80;
	else
	    bit = 0x1;
	gd = 0;
	g_d = gdisp->gg.green_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    gd += *g_d + COLOR_RED(index) + COLOR_GREEN(index) + COLOR_BLUE(index);
	    if ( gd<3*128 ) {
		*ipt &= ~bit;
		*g_d++ = gd /= 2;
	    } else {
		*ipt |= bit;
		*g_d++ = gd = (gd - 3*255)/2;
	    }
	    if ( gdisp->gg.img->bitmap_bit_order == MSBFirst ) {
		if (( bit>>=1 )==0 ) {bit=0x80; ++ipt;};
	    } else {
		if (( bit<<=1 )==256 ) {bit=0x1; ++ipt;};
	    }
	}
    }
}

static void gdraw_8_on_8_nomag_dithered_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    int i,j, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint8_t *pt, *ipt, *mpt;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    const struct gcol *pos;
#if FAST_BITS
    int mbit;
#endif

    _GDraw_getimageclut(base,clut);

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.red_dith[i]= gdisp->gg.green_dith[i] = gdisp->gg.blue_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
#if FAST_BITS
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	rd = gd = bd = 0;
	r_d = gdisp->gg.red_dith; g_d = gdisp->gg.green_dith; b_d = gdisp->gg.blue_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
#if FAST_BITS==0
		*mpt++ = 0xff;
#else
		*mpt |= mbit;
#endif
		*ipt++ = 0x00;
		++r_d; ++g_d; ++b_d;
	    } else {
		pos = &clut[index];
		rd += *r_d + pos->red; if ( rd<0 ) rd=0; else if ( rd>255 ) rd = 255;
		gd += *g_d + pos->green; if ( gd<0 ) gd=0; else if ( gd>255 ) gd = 255;
		bd += *b_d + pos->blue; if ( bd<0 ) bd=0; else if ( bd>255 ) bd = 255;
		pos = _GImage_GetIndexedPixel(COLOR_CREATE(rd,gd,bd), gdisp->cs.rev);
		*ipt++ = pos->pixel;
		*r_d++ = rd = (rd - pos->red)/2;
		*g_d++ = gd = (gd - pos->green)/2;
		*b_d++ = bd = (bd - pos->blue)/2;
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_8_on_8_nomag_nodithered_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    register int j;
    int i,index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint8_t *pt, *ipt, *mpt;
    struct gcol *pos; const struct gcol *temp;
#if FAST_BITS
    int mbit;
#endif

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	temp = _GImage_GetIndexedPixel(COLOR_CREATE(pos->red,pos->green,pos->blue),gdisp->cs.rev);
	pos->pixel = temp->pixel;
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
#if FAST_BITS
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
#if FAST_BITS==0
		*mpt++ = 0xff;
#else
		*mpt |= mbit;
#endif
		*ipt++ = 0x00;
	    } else {
		*ipt++ = clut[index].pixel;
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_32_on_8_nomag_dithered_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint32_t *pt, index;
    uint8_t *ipt, *mpt;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    const struct gcol *pos;
#if FAST_BITS
    int mbit;
#endif

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.red_dith[i]= gdisp->gg.green_dith[i] = gdisp->gg.blue_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
#if FAST_BITS
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	rd = gd = bd = 0;
	r_d = gdisp->gg.red_dith; g_d = gdisp->gg.green_dith; b_d = gdisp->gg.blue_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
#if FAST_BITS==0
		*mpt++ = 0xff;
#else
		*mpt |= mbit;
#endif
		*ipt++ = 0x00;
		++r_d; ++g_d; ++b_d;
	    } else {
		rd += *r_d + ((index>>16)&0xff); if ( rd<0 ) rd=0; else if ( rd>255 ) rd = 255;
		gd += *g_d + ((index>>8)&0xff); if ( gd<0 ) gd=0; else if ( gd>255 ) gd = 255;
		bd += *b_d + (index&0xff); if ( bd<0 ) bd=0; else if ( bd>255 ) bd = 255;
		pos = _GImage_GetIndexedPixel(COLOR_CREATE(rd,gd,bd),gdisp->cs.rev);
		*ipt++ = pos->pixel;
		*r_d++ = rd = (rd - pos->red)/2;
		*g_d++ = gd = (gd - pos->green)/2;
		*b_d++ = bd = (bd - pos->blue)/2;
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_32a_on_8_nomag_dithered(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint32_t *pt, index;
    uint8_t *ipt, *mpt;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    const struct gcol *pos;
#if FAST_BITS
    int mbit;
#endif

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.red_dith[i]= gdisp->gg.green_dith[i] = gdisp->gg.blue_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
#if FAST_BITS
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	rd = gd = bd = 0;
	r_d = gdisp->gg.red_dith; g_d = gdisp->gg.green_dith; b_d = gdisp->gg.blue_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans || (index>>24)<0x80 ) {
#if FAST_BITS==0
		*mpt++ = 0xff;
#else
		*mpt |= mbit;
#endif
		*ipt++ = 0x00;
		++r_d; ++g_d; ++b_d;
	    } else {
		rd += *r_d + ((index>>16)&0xff); if ( rd<0 ) rd=0; else if ( rd>255 ) rd = 255;
		gd += *g_d + ((index>>8)&0xff); if ( gd<0 ) gd=0; else if ( gd>255 ) gd = 255;
		bd += *b_d + (index&0xff); if ( bd<0 ) bd=0; else if ( bd>255 ) bd = 255;
		pos = _GImage_GetIndexedPixel(COLOR_CREATE(rd,gd,bd),gdisp->cs.rev);
		*ipt++ = pos->pixel;
		*r_d++ = rd = (rd - pos->red)/2;
		*g_d++ = gd = (gd - pos->green)/2;
		*b_d++ = bd = (bd - pos->blue)/2;
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_32_on_8_nomag_nodithered_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint32_t *pt, index;
    register uint8_t *ipt, *mpt;
#if FAST_BITS
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
#if FAST_BITS
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
#if FAST_BITS==0
		*mpt++ = 0xff;
#else
		*mpt |= mbit;
#endif
		*ipt++ = 0x00;
	    } else {
		*ipt++ = _GXDraw_GetScreenPixel(gdisp,index);
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_32a_on_8_nomag_nodithered(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint32_t *pt, index;
    register uint8_t *ipt, *mpt;
#if FAST_BITS
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
#if FAST_BITS
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( (index==trans && trans!=-1 ) || (index>>24)<0x80 ) {
#if FAST_BITS==0
		*mpt++ = 0xff;
#else
		*mpt |= mbit;
#endif
		*ipt++ = 0x00;
	    } else {
		*ipt++ = _GXDraw_GetScreenPixel(gdisp,index);
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_8_on_8_nomag_dithered_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    int i,j, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    uint8_t *pt, *ipt;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    const struct gcol *pos;

    _GDraw_getimageclut(base,clut);

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.red_dith[i]= gdisp->gg.green_dith[i] = gdisp->gg.blue_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	rd = gd = bd = 0;
	r_d = gdisp->gg.red_dith; g_d = gdisp->gg.green_dith; b_d = gdisp->gg.blue_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    pos = &clut[index];
	    rd += *r_d + pos->red; if ( rd<0 ) rd=0; else if ( rd>255 ) rd = 255;
	    gd += *g_d + pos->green; if ( gd<0 ) gd=0; else if ( gd>255 ) gd = 255;
	    bd += *b_d + pos->blue; if ( bd<0 ) bd=0; else if ( bd>255 ) bd = 255;
	    pos = _GImage_GetIndexedPixel(COLOR_CREATE(rd,gd,bd),gdisp->cs.rev);
	    *ipt++ = pos->pixel;
	    *r_d++ = rd = (rd - pos->red)/2;
	    *g_d++ = gd = (gd - pos->green)/2;
	    *b_d++ = bd = (bd - pos->blue)/2;
	}
    }
}

static void gdraw_8_on_8_nomag_nodithered_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    register int j;
    int i,index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    register uint8_t *pt, *ipt;
    struct gcol *pos; const struct gcol *temp;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	temp = _GImage_GetIndexedPixel(COLOR_CREATE(pos->red,pos->green,pos->blue),gdisp->cs.rev);
	pos->pixel = temp->pixel;
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = clut[index].pixel;
	}
    }
}

static void gdraw_32_on_8_nomag_dithered_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    uint32_t *pt, index;
    uint8_t *ipt;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    const struct gcol *pos;

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.red_dith[i]= gdisp->gg.green_dith[i] = gdisp->gg.blue_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	rd = gd = bd = 0;
	r_d = gdisp->gg.red_dith; g_d = gdisp->gg.green_dith; b_d = gdisp->gg.blue_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    rd += *r_d + COLOR_RED(index); if ( rd<0 ) rd=0; else if ( rd>255 ) rd = 255;
	    gd += *g_d + COLOR_GREEN(index); if ( gd<0 ) gd=0; else if ( gd>255 ) gd = 255;
	    bd += *b_d + COLOR_BLUE(index); if ( bd<0 ) bd=0; else if ( bd>255 ) bd = 255;
	    pos = _GImage_GetIndexedPixel(COLOR_CREATE(rd,gd,bd),gdisp->cs.rev);
	    *ipt++ = pos->pixel;
	    *r_d++ = rd = (rd - pos->red)/2;
	    *g_d++ = gd = (gd - pos->green)/2;
	    *b_d++ = bd = (bd - pos->blue)/2;
	}
    }
}

static void gdraw_32_on_8_nomag_nodithered_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    uint32_t *pt, index;
    register uint8_t *ipt;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = _GXDraw_GetScreenPixel(gdisp,index);
	}
    }
}

static void gdraw_8_on_16_nomag_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    register int j;
    int i,index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
#if FAST_BITS==0
    uint16_t *mpt;
#else
    int mbit;
    uint8_t *mpt;
#endif
    register uint8_t *pt;
    uint16_t *ipt;
    struct gcol *pos;
    Color col;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	col = (pos->red<<16)|(pos->green<<8)|pos->blue;
	pos->pixel = Pixel16(gdisp,col);
	if ( gdisp->endian_mismatch )
	    pos->pixel = FixEndian16(pos->pixel);
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint16_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint16_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
#if FAST_BITS==0
		*mpt++ = 0xffff;
#else
		*mpt |= mbit;
#endif
		*ipt++ = 0x00;
	    } else {
		*ipt++ = clut[index].pixel;
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_32_on_16_nomag_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint32_t *pt, index;
    register uint16_t *ipt;
    int endian_mismatch = gdisp->endian_mismatch;
#if FAST_BITS==0
    register uint16_t *mpt;
#else
    register uint8_t *mpt;
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint16_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint16_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
		*ipt++ = 0x00;
#if FAST_BITS==0
		*mpt++ = 0xffff;
#else
		*mpt |= mbit;
#endif
	    } else {
		*ipt++ = Pixel16(gdisp,index);
		if ( endian_mismatch )
		    ipt[-1] = FixEndian16(ipt[-1]);
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_32a_on_16_nomag(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint32_t *pt, index;
    register uint16_t *ipt;
    int endian_mismatch = gdisp->endian_mismatch;
#if FAST_BITS==0
    register uint16_t *mpt;
#else
    register uint8_t *mpt;
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint16_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint16_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( (index==trans && trans!=-1 ) || (index>>24)<0x80 ) {
		*ipt++ = 0x00;
#if FAST_BITS==0
		*mpt++ = 0xffff;
#else
		*mpt |= mbit;
#endif
	    } else {
		*ipt++ = Pixel16(gdisp,index);
		if ( endian_mismatch )
		    ipt[-1] = FixEndian16(ipt[-1]);
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_8_on_16_nomag_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    register int j;
    int i,index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    register uint8_t *pt;
    uint16_t *ipt;
    struct gcol *pos;
    Color col;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	col = (pos->red<<16)|(pos->green<<8)|pos->blue;
	pos->pixel = Pixel16(gdisp,col);
	if ( gdisp->endian_mismatch )
	    pos->pixel = FixEndian16(pos->pixel);
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint16_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = clut[index].pixel;
	}
    }
}

static void gdraw_32_on_16_nomag_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    register uint32_t *pt, index;
    register uint16_t *ipt;
    int endian_mismatch = gdisp->endian_mismatch;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint16_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = Pixel16(gdisp,index);
	    if ( endian_mismatch )
		ipt[-1] = FixEndian16(ipt[-1]);
	}
    }
}

static void gdraw_8_on_any_nomag_glyph(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    register int j;
    int i,index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint8_t *pt;
    struct gcol *pos;
    Color col;
    int msbf = gdisp->gg.img->byte_order == MSBFirst/*,
	    msBf = gdisp->gg.mask->bitmap_bit_order == MSBFirst*/;

    _GDraw_getimageclut(base,clut);

    if ( gdisp->pixel_size==16 ) {
	uint16_t *ipt;
	for ( i=base->clut->clut_len-1; i>=0; --i ) {
	    pos = &clut[i];
	    col = (pos->red<<16)|(pos->green<<8)|pos->blue;
	    pos->pixel = Pixel16(gdisp,col);
	    if ( i==trans )
		pos->pixel = Pixel16(gdisp,0xffffff);
	    if ( gdisp->endian_mismatch )
		pos->pixel = FixEndian16(pos->pixel);
	}

	for ( i=src->y; i<src->y+src->height; ++i ) {
	    pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	    ipt = (uint16_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	    for ( j=src->width-1; j>=0; --j ) {
		index = *pt++;
		*ipt++ = clut[index].pixel;
	    }
	}
    } else if ( gdisp->pixel_size==24 ) {
	uint8_t *ipt;
	for ( i=base->clut->clut_len-1; i>=0; --i ) {
	    pos = &clut[i];
	    pos->pixel = Pixel24(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
	    if ( i==trans )
		pos->pixel = Pixel24(gdisp,0xffffff);
	}

	for ( i=src->y; i<src->y+src->height; ++i ) {
	    pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	    ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	    for ( j=src->width-1; j>=0; --j ) {
		register uint32_t col;
		index = *pt++;
		col = clut[index].pixel;
		if ( msbf ) {
		    *ipt++ = col>>16;
		    *ipt++ = (col>>8)&0xff;
		    *ipt++ = col&0xff;
		} else {
		    *ipt++ = col&0xff;
		    *ipt++ = (col>>8)&0xff;
		    *ipt++ = col>>16;
		}
	    }
	}
    } else {
	uint32_t *ipt;
	for ( i=base->clut->clut_len-1; i>=0; --i ) {
	    pos = &clut[i];
	    pos->pixel = Pixel32(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
	    if ( i==trans )
		pos->pixel = 0xffffffff;
	    if ( gdisp->endian_mismatch )
		pos->pixel = FixEndian32(pos->pixel);
	}

	for ( i=src->y; i<src->y+src->height; ++i ) {
	    pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	    ipt = (uint32_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	    for ( j=src->width-1; j>=0; --j ) {
		index = *pt++;
		*ipt++ = clut[index].pixel;
	    }
	}
    }
}

static void gdraw_8_on_24_nomag_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    register int j;
    int i,index;
#if FAST_BITS
    int mbit;
#endif
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint8_t *ipt;
    register uint8_t *pt, *mpt;
    struct gcol *pos;
    int msbf = gdisp->gg.img->byte_order == MSBFirst/*,
	    msBf = gdisp->gg.mask->bitmap_bit_order == MSBFirst*/;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	pos->pixel = Pixel24(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
#if FAST_BITS
	if ( msBf )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
#if FAST_BITS==0
		*mpt++ = 0xff; *mpt++ = 0xff; *mpt++ = 0xff;
#else
		*mpt |= mbit;
#endif
		*ipt++ = 0x00; *ipt++ = 0x00; *ipt++ = 0x00;
	    } else {
		register uint32_t col = clut[index].pixel;
		if ( msbf ) {
		    *ipt++ = col>>16;
		    *ipt++ = (col>>8)&0xff;
		    *ipt++ = col&0xff;
		} else {
		    *ipt++ = col&0xff;
		    *ipt++ = (col>>8)&0xff;
		    *ipt++ = col>>16;
		}
#if FAST_BITS==0
		*mpt++ = 0; *mpt++ = 0; *mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( msBf ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt; };
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_32_on_24_nomag_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint32_t *pt, index;
    register uint8_t *mpt, *ipt;
#if FAST_BITS
    int mbit;
#endif
    int msbf = gdisp->gg.img->byte_order == MSBFirst;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#if FAST_BITS
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
		*ipt++ = 0x00; *ipt++ = 0x00; *ipt++ = 0x00;
#if FAST_BITS==0
		*mpt++ = 0xff; *mpt++ = 0xff; *mpt++ = 0xff;
#else
		*mpt |= mbit;
#endif
	    } else {
		index = Pixel24(gdisp,index);
		if ( msbf ) {
		    *ipt++ = index>>16;
		    *ipt++ = (index>>8)&0xff;
		    *ipt++ = index&0xff;
		} else {
		    *ipt++ = index&0xff;
		    *ipt++ = (index>>8)&0xff;
		    *ipt++ = index>>16;
		}
#if FAST_BITS==0
		*mpt++ = 0; *mpt++ = 0; *mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_32a_on_24_nomag(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint32_t *pt, index;
    register uint8_t *mpt, *ipt;
#if FAST_BITS
    int mbit;
#endif
    int msbf = gdisp->gg.img->byte_order == MSBFirst;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#if FAST_BITS
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( (index==trans && trans!=-1) || (index>>24)<0x80 ) {
		*ipt++ = 0x00; *ipt++ = 0x00; *ipt++ = 0x00;
#if FAST_BITS==0
		*mpt++ = 0xff; *mpt++ = 0xff; *mpt++ = 0xff;
#else
		*mpt |= mbit;
#endif
	    } else {
		index = Pixel24(gdisp,index);
		if ( msbf ) {
		    *ipt++ = index>>16;
		    *ipt++ = (index>>8)&0xff;
		    *ipt++ = index&0xff;
		} else {
		    *ipt++ = index&0xff;
		    *ipt++ = (index>>8)&0xff;
		    *ipt++ = index>>16;
		}
#if FAST_BITS==0
		*mpt++ = 0; *mpt++ = 0; *mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_8_on_24_nomag_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    register uint8_t *ipt;
    register uint8_t *pt;
    register int index, j;
    int i;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    struct gcol *pos;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	pos->pixel = Pixel24(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	if ( gdisp->gg.img->byte_order == MSBFirst ) {
	    for ( j=src->width-1; j>=0; --j ) {
		index = *pt++;
		index = clut[index].pixel;
		*ipt++ = index>>16;
		*ipt++ = (index>>8)&0xff;
		*ipt++ = index&0xff;
	    }
	} else {
	    for ( j=src->width-1; j>=0; --j ) {
		index = *pt++;
		index = clut[index].pixel;
		*ipt++ = index&0xff;
		*ipt++ = (index>>8)&0xff;
		*ipt++ = index>>16;
	    }
	}
    }
}

static void gdraw_32_on_24_nomag_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    register uint32_t *pt, index;
    register uint8_t *ipt;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8_t *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	if ( gdisp->gg.img->byte_order == MSBFirst ) {
	    for ( j=src->width-1; j>=0; --j ) {
		index = *pt++;
		index = Pixel24(gdisp,index);
		*ipt++ = index>>16;
		*ipt++ = (index>>8)&0xff;
		*ipt++ = index&0xff;
	    }
	} else {
	    for ( j=src->width-1; j>=0; --j ) {
		index = *pt++;
		index = Pixel24(gdisp,index);
		*ipt++ = index&0xff;
		*ipt++ = (index>>8)&0xff;
		*ipt++ = index>>16;
	    }
	}
    }
}

static void gdraw_8_on_32a_nomag(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    register int j;
    int i,index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint8_t *pt;
    uint32_t *ipt;
    struct gcol *pos;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	pos->pixel = Pixel32(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
	if ( i==trans )
	    pos->pixel = 0x00000000;
	if ( gdisp->endian_mismatch )
	    pos->pixel = FixEndian32(pos->pixel);
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint32_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = clut[index].pixel;
	}
    }
}

static void gdraw_8a_on_32a_nomag(GXDisplay *gdisp, GImage *image, GRect *src,
	Color fg) {
    register int j;
    int i,index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    register uint8_t *pt;
    uint32_t *ipt;
    uint32_t fg_pixel = Pixel32(gdisp,fg) & 0xffffff;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint32_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = fg_pixel | (index<<24);
	}
    }
}

static void gdraw_32_on_32a_nomag(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint32_t *pt, index, *ipt;
    int endian_mismatch = gdisp->endian_mismatch;
    int has_alpha = base->image_type == it_rgba;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint32_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
		*ipt++ = 0x00000000;
	    } else {
		if ( has_alpha )
		    *ipt++ = Pixel16(gdisp,(index&0xffffff)) | (index&0xff000000);
		else
		    *ipt++ = Pixel32(gdisp,index);
		if ( endian_mismatch )
		    ipt[-1] = FixEndian32(ipt[-1]);
	    }
	}
    }
}

static void gdraw_8_on_32_nomag_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    register int j;
    int i,index;
#if FAST_BITS==0
    register uint32_t *mpt;
#else
    int mbit;
    register uint8_t *mpt;
#endif
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint8_t *pt;
    uint32_t *ipt;
    struct gcol *pos;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	pos->pixel = Pixel32(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
	if ( gdisp->endian_mismatch )
	    pos->pixel = FixEndian32(pos->pixel);
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint32_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint32_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8_t *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
#if FAST_BITS==0
		*mpt++ = 0xffffffff;
#else
		*mpt |= mbit;
#endif
		*ipt++ = 0x00;
	    } else {
		*ipt++ = clut[index].pixel;
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_32_on_32_nomag_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint32_t *pt, index, *ipt;
    int endian_mismatch = gdisp->endian_mismatch;
#if FAST_BITS==0
    register uint32_t *mpt;
#else
    register uint8_t *mpt;
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint32_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint32_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
		*ipt++ = Pixel32(gdisp,0);
#if FAST_BITS==0
		*mpt++ = 0xffffffff;
#else
		*mpt |= mbit;
#endif
	    } else {
		*ipt++ = Pixel32(gdisp,index);
		if ( endian_mismatch )
		    ipt[-1] = FixEndian32(ipt[-1]);
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_32a_on_32_nomag(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint32_t *pt, index, *ipt;
    int endian_mismatch = gdisp->endian_mismatch;
#if FAST_BITS==0
    register uint32_t *mpt;
#else
    register uint8_t *mpt;
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint32_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint32_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8_t *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( (index==trans && trans!=-1) || (index>>24)<0x80 ) {
		*ipt++ = Pixel32(gdisp,0);
#if FAST_BITS==0
		*mpt++ = 0xffffffff;
#else
		*mpt |= mbit;
#endif
	    } else {
		*ipt++ = Pixel32(gdisp,index);
		if ( endian_mismatch )
		    ipt[-1] = FixEndian32(ipt[-1]);
#if FAST_BITS==0
		*mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
#endif
	}
    }
}

static void gdraw_8_on_32_nomag_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    register int j;
    int i,index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    register uint8_t *pt;
    uint32_t *ipt;
    struct gcol *pos;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	pos->pixel = Pixel32(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
	if ( gdisp->endian_mismatch )
	    pos->pixel = FixEndian32(pos->pixel);
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8_t *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint32_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = clut[index].pixel;
	}
    }
}

static void gdraw_32_on_32_nomag_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    register uint32_t *pt, index, *ipt;
    int endian_mismatch = gdisp->endian_mismatch;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32_t *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint32_t *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = Pixel32(gdisp,index);
	    if ( endian_mismatch )
		ipt[-1] = FixEndian32(ipt[-1]);
	}
    }
}

static void gdraw_xbitmap(GXWindow w, XImage *xi, GClut *clut,
	Color trans, GRect *src, int x, int y) {
    GXDisplay *gdisp = w->display;
    Display *display = gdisp->display;
    GC gc = gdisp->gcstate[w->ggc->bitmap_col].gc;
    Color fg, bg;

    if ( trans!=COLOR_UNKNOWN ) {
	XSetFunction(display,gc,GXand);
	if ( trans==1 ) {
	    XSetForeground(display,gc, ~gdisp->cs.alpha_bits );
	    XSetBackground(display,gc, 0 );
	} else {
	    XSetForeground(display,gc, 0 );
	    XSetBackground(display,gc, ~gdisp->cs.alpha_bits );
	}
	XPutImage(display,w->w,gc,xi,src->x,src->y,
		x,y, src->width, src->height );
	fg = trans==1?0:_GXDraw_GetScreenPixel(gdisp,clut!=NULL?clut->clut[1]:COLOR_CREATE(0xff,0xff,0xff));
	bg = trans==0?0:_GXDraw_GetScreenPixel(gdisp,clut!=NULL?clut->clut[0]:COLOR_CREATE(0,0,0));
	fg |= (gdisp)->cs.alpha_bits;
	bg |= (gdisp)->cs.alpha_bits;
	if ( /*bg!=fg || fg!=0*/ true ) {
#ifdef _BrokenBitmapImages
	    /* See the comment at _GXDraw_Image about why this works */
	    XSetFunction(display,gc,GXxor);
#else
	    XSetFunction(display,gc,GXor);
#endif
	    XSetForeground(display,gc,fg);
	    XSetBackground(display,gc,bg);
	}
    } else {
	XSetForeground(display,gc,
		_GXDraw_GetScreenPixel(gdisp,
		    clut!=NULL?clut->clut[1]:COLOR_CREATE(0xff,0xff,0xff)) | (gdisp)->cs.alpha_bits);
	XSetBackground(display,gc,
		_GXDraw_GetScreenPixel(gdisp,
		    clut!=NULL?clut->clut[0]:COLOR_CREATE(0,0,0)) | (gdisp)->cs.alpha_bits);
    }
    XPutImage(display,w->w,gc,xi,src->x,src->y,
	    x,y, src->width, src->height );
    XSetFunction(display,gc,GXcopy);
    gdisp->gcstate[w->ggc->bitmap_col].fore_col = COLOR_UNKNOWN;
}

static void gdraw_bitmap(GXWindow w, struct _GImage *image, GClut *clut,
	Color trans, GRect *src, int x, int y) {
    XImage *xi;
    GXDisplay *gdisp = w->display;
    uint8_t *newdata = NULL;

    xi = XCreateImage(gdisp->display,gdisp->visual,1,XYBitmap,0,(char *) (image->data),
	    image->width, image->height,8,image->bytes_per_line);
    if ( xi->bitmap_bit_order==LSBFirst ) {
	/* sigh. The server doesn't use our convention. I might be able just */
	/*  to change this field but it doesn't say, so best not to */
	int len = image->bytes_per_line*image->height;
	uint8_t *pt, *ipt, *end;
	int m1,m2,val;

	for ( ipt = image->data, pt=newdata=malloc(len), end=pt+len; pt<end; ++pt, ++ipt ) {
	    val = 0;
	    for ( m1=1, m2=0x80; m2!=0; m1<<=1, m2>>=1 )
		if ( *ipt&m1 ) val|=m2;
	    *pt = val;
	}
	xi->data = (char *) newdata;
    }
    gdraw_xbitmap(w,xi,clut,trans,src,x,y);
    if ( (uint8_t *) (xi->data)==image->data || (uint8_t *) (xi->data)==newdata ) xi->data = NULL;
    XDestroyImage(xi);
}

static void check_image_buffers(GXDisplay *gdisp, int neww, int newh, int is_bitmap) {
    int width = gdisp->gg.iwidth, height = gdisp->gg.iheight;
    char *temp;
    int depth = gdisp->depth, pixel_size;
    union { int32_t foo; uint8_t bar[4]; } endian;

    if ( is_bitmap ) depth=1;
    if ( neww > gdisp->gg.iwidth ) {
	width = neww;
	if ( width<400 ) width = 400;
    }
    if ( width > gdisp->gg.iwidth || (gdisp->gg.img!=NULL && depth!=gdisp->gg.img->depth) ) {
        free(gdisp->gg.red_dith);
        free(gdisp->gg.green_dith);
        free(gdisp->gg.blue_dith);
	if ( depth<=8 ) {
	    gdisp->gg.red_dith = malloc(width*sizeof(short));
	    gdisp->gg.green_dith = malloc(width*sizeof(short));
	    gdisp->gg.blue_dith = malloc(width*sizeof(short));
	    if ( gdisp->gg.red_dith==NULL || gdisp->gg.green_dith==NULL || gdisp->gg.blue_dith==NULL )
		gdisp->do_dithering = 0;
	} else {
	    gdisp->gg.red_dith = NULL;
	    gdisp->gg.green_dith = NULL;
	    gdisp->gg.blue_dith = NULL;
	}
    }
    if ( newh > gdisp->gg.iheight ) {
	height = newh;
	if ( height<400 ) height = 400;
    }

    if ( gdisp->gg.iwidth == width && gdisp->gg.iheight == height && depth==gdisp->gg.img->depth )
return;

    if ( gdisp->gg.img!=NULL ) {
	/* If gdisp->gg.img->data was allocated by GC_malloc rather
	   than standard libc malloc then it must be set to NULL so
	   that XDestroyImage() does not try to free it and crash.
	   
	   If we no longer use libgc then the following conditional
	   block can be removed, but in case it isn't, the enclosed
	   free() will prevent a memory leak.
	*/
	if (gdisp->gg.img->data) {
	    free(gdisp->gg.img->data);
	    gdisp->gg.img->data = NULL;
	}
	XDestroyImage(gdisp->gg.img);
    }
    if ( gdisp->gg.mask!=NULL ) {
	/* If gdisp->gg.mask->data was allocated by GC_malloc rather
	   than standard libc malloc then it must be set to NULL so
	   that XDestroyImage() does not try to free it and crash.
	   
	   If we no longer use libgc then the following conditional
	   block can be removed, but in case it isn't, the enclosed
	   free() will prevent a memory leak.
	*/
	if (gdisp->gg.mask->data) {
	    free(gdisp->gg.mask->data);
	    gdisp->gg.mask->data = NULL;
	}
	XDestroyImage(gdisp->gg.mask);
    }
    pixel_size = gdisp->pixel_size;
    temp = malloc(((width*pixel_size+gdisp->bitmap_pad-1)/gdisp->bitmap_pad)*
	    (gdisp->bitmap_pad/8)*height);
    if ( temp==NULL ) {
	GDrawIError("Can't create image draw area");
	exit(1);
    }
    gdisp->gg.img = XCreateImage(gdisp->display,gdisp->visual,depth,
	    depth==1?XYBitmap:ZPixmap,0,
	    temp,width,height,gdisp->bitmap_pad,0);
    if ( gdisp->gg.img==NULL ) {
	GDrawIError("Can't create image draw area");
	exit(1);
    }
    if ( !FAST_BITS==0 ) pixel_size=1;
    temp = malloc(((width*pixel_size+gdisp->bitmap_pad-1)/gdisp->bitmap_pad)*
	    (gdisp->bitmap_pad/8)*height);
    gdisp->gg.mask = NULL;
    if ( temp!=NULL ) {
	gdisp->gg.mask = XCreateImage(gdisp->display,gdisp->visual,depth,
		depth==1?XYBitmap:ZPixmap,
		0,temp,width,height,gdisp->bitmap_pad,0);
	if ( gdisp->gg.mask==NULL )
	    free(temp);
    }
    gdisp->gg.iwidth = width; gdisp->gg.iheight = height;
    endian.foo = 0xff;
    if ( (gdisp->gg.img->byte_order==MSBFirst) != ( endian.bar[3]==0xff ))
	gdisp->endian_mismatch = true;
}

static void gximage_to_ximage(GXWindow gw, GImage *image, GRect *src) {
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    GXDisplay *gdisp = gw->display;
    int depth;

    depth = gdisp->pixel_size;
    if ( depth!=8 && depth!=16 && depth!=24 && depth!=32 )
	depth = 1;
    else if ( gw->ggc->bitmap_col )
	depth = 1;

    check_image_buffers(gdisp, src->width, src->height,depth==1);
    if ( base->trans!=COLOR_UNKNOWN || base->image_type == it_rgba ||
	    gdisp->supports_alpha_images ) {
	if ( base->image_type == it_index ) {
	    switch ( depth ) {
	      case 1:
	      default:
		/* all servers can handle bitmaps, so if we don't know how to*/
		/*  write to a 13bit screen, we can at least give a bitmap */
		gdraw_8_on_1_nomag_dithered_masked(gdisp,image,src);
	      break;
	      case 8:
		if ( gdisp->do_dithering && !gdisp->cs.is_grey )
		    gdraw_8_on_8_nomag_dithered_masked(gdisp,image,src);
		else
		    gdraw_8_on_8_nomag_nodithered_masked(gdisp,image,src);
	      break;
	      case 16:
		gdraw_8_on_16_nomag_masked(gdisp,image,src);
	      break;
	      case 24:
		gdraw_8_on_24_nomag_masked(gdisp,image,src);
	      break;
	      case 32:
		if ( gdisp->supports_alpha_images )
		    gdraw_8_on_32a_nomag(gdisp,image,src);
		else
		    gdraw_8_on_32_nomag_masked(gdisp,image,src);
	      break;
	    }
	} else if ( base->image_type == it_true ) {
	    switch ( depth ) {
	      case 1:
	      default:
		/* all servers can handle bitmaps, so if we don't know how to*/
		/*  write to a 13bit screen, we can at least give a bitmap */
		gdraw_32_on_1_nomag_dithered_masked(gdisp,image,src);
	      break;
	      case 8:
		if ( gdisp->do_dithering && !gdisp->cs.is_grey )
		    gdraw_32_on_8_nomag_dithered_masked(gdisp,image,src);
		else
		    gdraw_32_on_8_nomag_nodithered_masked(gdisp,image,src);
	      break;
	      case 16:
		gdraw_32_on_16_nomag_masked(gdisp,image,src);
	      break;
	      case 24:
		gdraw_32_on_24_nomag_masked(gdisp,image,src);
	      break;
	      case 32:
		if ( gdisp->supports_alpha_images )
		    gdraw_32_on_32a_nomag(gdisp,image,src);
		else
		    gdraw_32_on_32_nomag_masked(gdisp,image,src);
	      break;
	    }
	} else if ( base->image_type == it_rgba ) {
	    switch ( depth ) {
	      case 1:
	      default:
		/* all servers can handle bitmaps, so if we don't know how to*/
		/*  write to a 13bit screen, we can at least give a bitmap */
		gdraw_32a_on_1_nomag_dithered(gdisp,image,src);
	      break;
	      case 8:
		if ( gdisp->do_dithering && !gdisp->cs.is_grey )
		    gdraw_32a_on_8_nomag_dithered(gdisp,image,src);
		else
		    gdraw_32a_on_8_nomag_nodithered(gdisp,image,src);
	      break;
	      case 16:
		gdraw_32a_on_16_nomag(gdisp,image,src);
	      break;
	      case 24:
		gdraw_32a_on_24_nomag(gdisp,image,src);
	      break;
	      case 32:
		if ( gdisp->supports_alpha_images )
		    gdraw_32_on_32a_nomag(gdisp,image,src);
		else
		    gdraw_32a_on_32_nomag(gdisp,image,src);
	      break;
	    }
	}
    } else { /* no mask */
	if ( base->image_type == it_index ) {
	    switch ( depth ) {
	      case 1:
	      default:
		gdraw_8_on_1_nomag_dithered_nomask(gdisp,image,src);
	      break;
	      case 8:
		if ( gdisp->do_dithering && !gdisp->cs.is_grey )
		    gdraw_8_on_8_nomag_dithered_nomask(gdisp,image,src);
		else
		    gdraw_8_on_8_nomag_nodithered_nomask(gdisp,image,src);
	      break;
	      case 16:
		gdraw_8_on_16_nomag_nomask(gdisp,image,src);
	      break;
	      case 24:
		gdraw_8_on_24_nomag_nomask(gdisp,image,src);
	      break;
	      case 32:
		gdraw_8_on_32_nomag_nomask(gdisp,image,src);
	      break;
	    }
	} else if ( base->image_type == it_true ) {
	    switch ( depth ) {
	      case 1:
	      default:
		gdraw_32_on_1_nomag_dithered_nomask(gdisp,image,src);
	      break;
	      case 8:
		if ( gdisp->do_dithering && !gdisp->cs.is_grey )
		    gdraw_32_on_8_nomag_dithered_nomask(gdisp,image,src);
		else
		    gdraw_32_on_8_nomag_nodithered_nomask(gdisp,image,src);
	      break;
	      case 16:
		gdraw_32_on_16_nomag_nomask(gdisp,image,src);
	      break;
	      case 24:
		gdraw_32_on_24_nomag_nomask(gdisp,image,src);
	      break;
	      case 32:
		gdraw_32_on_32_nomag_nomask(gdisp,image,src);
	      break;
	    }
	}
    }
}

void _GXDraw_Image( GWindow _w, GImage *image, GRect *src, int32_t x, int32_t y) {
    GXWindow gw = (GXWindow) _w;
    GXDisplay *gdisp = gw->display;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    Display *display=gdisp->display;
    Window w = gw->w;
    GC gc = gdisp->gcstate[gw->ggc->bitmap_col].gc;
    GRect rootPos = {0, 0, XDisplayWidth(display,gdisp->screen),
                           XDisplayHeight(display,gdisp->screen)};
    GRect img_pos = {x+_w->pos.x, y+_w->pos.y, src->width, src->height};
    GRect win_pos = {x, y, src->width, src->height};
    GRect blend_src = {0, 0, src->width, src->height};
    GImage *blended = NULL;
    int depth;

#ifndef _NO_LIBCAIRO
    if ( gw->usecairo ) {
	_GXCDraw_Image(gw,image,src,x,y);
return;
    }
#endif

    _GXDraw_SetClipFunc(gdisp,gw->ggc);
    depth = gdisp->pixel_size;
    if ( depth!=8 && depth!=16 && depth!=24 && depth!=32 )
	depth = 1;
    else if ( gw->ggc->bitmap_col )
	depth = 1;

    if ( base->image_type == it_mono ) {
	/* Mono images are easy, because all X servers (no matter what their */
	/*  depth) support 1 bit bitmaps */
	gdraw_bitmap(gw,image->u.image,base->clut,base->trans,src,x,y);
return;
    }

/* Throws errors in Mac OS X */
#ifndef __Mac
    /* Can we blend this with background to support an alpha channel? */
    /* it's slow, particularly so on network connections, but that's */
    /* all we can get without reworking GDraw to use XComposite ext. */
    if ((depth >= 16) && (base->image_type == it_rgba)) {
        /* This requires caution, as the rectangle being worked */
        /* must be contained within both screen and the window. */
        intersect_rectangles(&img_pos, &(_w->pos));
        intersect_rectangles(&img_pos, &rootPos);
        img_pos.x -= _w->pos.x;
        img_pos.y -= _w->pos.y;
        win_pos = img_pos;
        blend_src = img_pos;
        blend_src.x = blend_src.y = 0;
        img_pos.x = src->x + (win_pos.x - x);
        img_pos.y = src->y + (win_pos.y - y);
        src = &img_pos;
        x = win_pos.x;
        y = win_pos.y;

        if (src->width>0 && src->height>0)
            blended = _GXDraw_CopyScreenToImage(_w, &win_pos);

        if (blended != NULL) {
            GImageBlendOver(blended, image, src, 0, 0);
            image = blended;
            base = image->list_len==0?image->u.image:image->u.images[0];
            src = &blend_src;
        }
    }
#endif

    gximage_to_ximage(gw, image, src);

    if ( !gdisp->supports_alpha_images && (blended == NULL) &&
         (base->trans!=COLOR_UNKNOWN || base->image_type==it_rgba )) {

	/* ((destination & mask) | src) seems to me to yield the proper behavior */
	/*  for transparent backgrounds. This is equivalent to: */
	/* ((destination GXorReverse mask) GXnand src) */
	/* Oh... I think xor works too because the mask and the src will never*/
	/*  both be 1 on the same pixel. If the mask is set then the image will*/
	/*  be clear. So xor and or are equivalent (the case on which they differ never occurs) */
	XSetFunction(display,gc,GXand);
#if FAST_BITS
	XSetForeground(display,gc, ~((-1)<<gdisp->pixel_size) );
	XSetBackground(display,gc, 0 );
#endif
	XPutImage(display,w,gc,gdisp->gg.mask,0,0,
		x,y, src->width, src->height );
#ifdef _BrokenBitmapImages
	XSetFunction(display,gc,GXxor);
#else
	XSetFunction(display,gc,GXor);
#endif
	XPutImage(display,w,gc,gdisp->gg.img,0,0,
		x,y, src->width, src->height );
	XSetFunction(display,gc,GXcopy);
	gdisp->gcstate[gw->ggc->bitmap_col].fore_col = COLOR_UNKNOWN;
    } else { /* no mask */
	XPutImage(display,w,gc,gdisp->gg.img,0,0,
		x,y, src->width, src->height );
    }
    
    if (blended != NULL)
        GImageDestroy(blended);
}

/* When drawing an anti-aliased glyph, I've been pretending that it's an image*/
/*  with colors running from foreground to background and with background be- */
/*  ing transparent. That works reasonably well -- on a blank background, but */
/*  when two glyphs overlap (as in a script font, for instance) we get a faint*/
/*  light halo around the edge of the second glyph. */
/* What we really want to do is use the grey levels as an alpha channel with */
/*  the foreground color as the color. But alpha channels haven't been avail- */
/*  able on most X-displays. An alternative is to do the composing ourselves  */
/*  in an image that's as big as the window, and then transfer that when done */
/*  That sounds slow. */
/* What should the composing look like? I'm not entirely but it should be */
/* somewhere between a "max" and a "clipped add" applied component by component*/
/*  of the color. X does not support either of those as primitives -- but X */
/*  does support bitwise boolean operators, and an "or" will always produce */
/*  a value somewhere between those extremes. */
/* Actually since the color values (black==foreground, white==background)   */
/*  generally run in the opposite direction from the alpha channel (100%=fore, */
/*  0%=back) we will need to reverse the "or" to be an "and", but the idea    */
/*  is the same */
void _GXDraw_Glyph( GWindow _w, GImage *image, GRect *src, int32_t x, int32_t y) {
    GXWindow gw = (GXWindow) _w;
    GXDisplay *gdisp = gw->display;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    Color fg = -1;

#ifndef _NO_LIBCAIRO
    if ( gw->usecairo ) {
	_GXCDraw_Glyph(gw,image,src,x,y);
return;
    }
#endif

    if ( base->image_type==it_index )
	fg = base->clut->clut[base->clut->clut_len-1];

    if ( base->image_type!=it_index )
	_GXDraw_Image(_w,image,src,x,y);
    else if ( gdisp->visual->class != TrueColor ||
	    gdisp->pixel_size<16 || gw->ggc->bitmap_col || fg!=0 )
	_GXDraw_Image(_w,image,src,x,y);
    else {
	Display *display=gdisp->display;
	Window w = gw->w;
	GC gc = gdisp->gcstate[gw->ggc->bitmap_col].gc;

	_GXDraw_SetClipFunc(gdisp,gw->ggc);

	check_image_buffers(gdisp, src->width, src->height,false);
	if ( gdisp->supports_alpha_images ) {
	    gdraw_8a_on_32a_nomag( gdisp, image, src, fg );
	} else {
	    gdraw_8_on_any_nomag_glyph(gdisp, image, src);
	    XSetFunction(display,gc,GXand);
	}
	XPutImage(display,w,gc,gdisp->gg.img,0,0,
		x,y, src->width, src->height );
	XSetFunction(display,gc,GXcopy);
    }
}

/* ******************************** Magnified ******************************* */

GImage *_GImageExtract(struct _GImage *base,GRect *src,GRect *size,
	double xscale, double yscale) {
    static GImage temp;
    static struct _GImage tbase;
    static uint8_t *data;
    static int dlen;
    int r,c;

    memset(&temp,0,sizeof(temp));
    tbase = *base;
    temp.u.image = &tbase;
    tbase.width = size->width; tbase.height = size->height;
    if ( base->image_type==it_mono )
	tbase.bytes_per_line = (size->width+7)/8;
    else if ( base->image_type==it_index )
	tbase.bytes_per_line = size->width;
    else
	tbase.bytes_per_line = 4*size->width;
    if ( tbase.bytes_per_line*size->height>dlen )
	data = realloc(data,dlen = tbase.bytes_per_line*size->height );
    tbase.data = data;

    /* I used to use rint(x). Now I use floor(x). For normal images rint */
    /*  might be better, but for text we need floor */

    if ( base->image_type==it_mono ) {
	memset(data,0,tbase.height*tbase.bytes_per_line);
	for ( r=0; r<size->height; ++r ) {
	    int or = ((int) floor( (r+size->y)/yscale ));
	    uint8_t *pt = data+r*tbase.bytes_per_line;
	    uint8_t *opt = base->data+or*base->bytes_per_line;
	    for ( c=0; c<size->width; ++c ) {
		int oc = ((int) floor( (c+size->x)/xscale));
		if ( opt[oc>>3] & (0x80>>(oc&7)) )
		    pt[c>>3] |= (0x80>>(c&7));
	    }
	}
    } else if ( base->image_type==it_index ) {
	for ( r=0; r<size->height; ++r ) {
	    int or = ((int) floor( (r+size->y)/yscale ));
	    uint8_t *pt = data+r*tbase.bytes_per_line;
	    uint8_t *opt = base->data+or*base->bytes_per_line;
	    for ( c=0; c<size->width; ++c ) {
		int oc = ((int) floor( (c+size->x)/xscale));
		*pt++ = opt[oc];
	    }
	}
    } else {
	for ( r=0; r<size->height; ++r ) {
	    int or = ((int) floor( (r+size->y)/yscale ));
	    uint32_t *pt = (uint32_t *) (data+r*tbase.bytes_per_line);
	    uint32_t *opt = (uint32_t *) (base->data+or*base->bytes_per_line);
	    for ( c=0; c<size->width; ++c ) {
		int oc = ((int) floor( (c+size->x)/xscale));
		*pt++ = opt[oc];
	    }
	}
    }
return( &temp );
}

/* Given an image, magnify it so that its width/height are as specified */
/*  then extract the given given rectangle (in magnified coords) and */
/*  place it on the screen at x,y */
void _GXDraw_ImageMagnified(GWindow _w, GImage *image, GRect *magsrc,
	int32_t x, int32_t y, int32_t width, int32_t height) {
    GXWindow gw = (GXWindow) _w;
    GXDisplay *gdisp = gw->display;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    double xscale, yscale;
    GRect full, viewable;
    GImage *temp;
    GRect src;

#ifndef _NO_LIBCAIRO
    if ( gw->usecairo ) {
	_GXCDraw_ImageMagnified(gw,image,magsrc,x,y,width,height);
return;
    }
#endif

    _GXDraw_SetClipFunc(gdisp,gw->ggc);
    viewable = gw->ggc->clip;
    if ( viewable.width > gw->pos.width-viewable.x )
	viewable.width = gw->pos.width-viewable.x;
    if ( viewable.height > gw->pos.height-viewable.y )
	viewable.height = gw->pos.height-viewable.y;

    xscale = (base->width>=1) ? ((double) (width))/(base->width) : 1;
    yscale = (base->height>=1) ? ((double) (height))/(base->height) : 1;
    /* Intersect the clip rectangle with the scaled image to find the */
    /*  portion of screen that we want to draw */
    if ( viewable.x<x ) {
	viewable.width -= (x-viewable.x);
	viewable.x = x;
    }
    if ( viewable.y<y ) {
	viewable.height -= (y-viewable.y);
	viewable.y = y;
    }
    if ( viewable.x+viewable.width > x+width ) viewable.width = x+width - viewable.x;
    if ( viewable.y+viewable.height > y+height ) viewable.height = y+height - viewable.y;
    if ( viewable.height<0 || viewable.width<0 )
return;

    /* Now find that same rectangle in the coordinates of the unscaled image */
    /* (translation & scale) */
    viewable.x -= x; viewable.y -= y;
    full.x = viewable.x/xscale; full.y = viewable.y/yscale;
    full.width = viewable.width/xscale; full.height = viewable.height/yscale;
    if ( full.x+full.width>base->width ) full.width = base->width-full.x;	/* Rounding errors */
    if ( full.y+full.height>base->height ) full.height = base->height-full.y;	/* Rounding errors */
		/* Rounding errors */

    temp = _GImageExtract(base,&full,&viewable,xscale,yscale);
    src.x = src.y = 0; src.width = viewable.width; src.height = viewable.height;
    _GXDraw_Image( _w, temp, &src, x+viewable.x, y+viewable.y);
}

static GImage *xi1_to_gi1(GXDisplay *gdisp,XImage *xi) {
    GImage *gi;
    struct _GImage *base;

    gi = calloc(1,sizeof(GImage));
    if ( gi==NULL )
return( NULL );
    base = malloc(sizeof(struct _GImage));
    if ( base==NULL ) {
        free(gi);
return( NULL );
    }
    gi->u.image = base;
    base->image_type = it_mono;
    base->width = xi->width;
    base->height = xi->height;
    base->bytes_per_line = xi->bytes_per_line;
    base->data = (uint8_t *) (xi->data);
    base->clut = NULL;
    base->trans = COLOR_UNKNOWN;

    if ( xi->bitmap_bit_order==LSBFirst ) {
	/* sigh. The server doesn't use our convention. invert all bytes */
	int len = base->height*base->bytes_per_line;
	uint8_t *newdata = malloc(len), *pt, *ipt, *end;
	int m1,m2,val;

	for ( ipt = (uint8_t *) xi->data, pt=newdata, end=pt+len; pt<end; ++pt, ++ipt ) {
	    val = 0;
	    for ( m1=1, m2=0x80; m2!=0; m1<<=1, m2>>=1 )
		if ( *ipt&m1 ) val|=m2;
	    *pt = val;
	}
	base->data = newdata;
    } else
	xi->data = NULL;
return( gi );
}

static GImage *xi8_to_gi8(GXDisplay *gdisp,XImage *xi) {
    GImage *gi;
    struct _GImage *base;
    GClut *clut;
    int i;
    XColor cols[256];

    gi = calloc(1,sizeof(GImage));
    if ( gi==NULL )
return( NULL );
    base = malloc(sizeof(struct _GImage));
    if ( base ==NULL ) {
        free(gi);
return( NULL );
    }
    clut = malloc(sizeof(GClut));
    if ( clut ==NULL ) {
        free(base);
        free(gi);
return( NULL );
    }
    gi->u.image = base;
    base->image_type = it_index;
    base->width = xi->width;
    base->height = xi->height;
    base->bytes_per_line = xi->bytes_per_line;
    base->data = (uint8_t *) xi->data;
    base->clut = clut;
    base->trans = COLOR_UNKNOWN;

    clut->clut_len = 256;
    for ( i=0; i<(1<<gdisp->pixel_size); ++i )
	cols[i].pixel = i;
    XQueryColors(gdisp->display,gdisp->cmap,cols,1<<gdisp->pixel_size);
    for ( i=0; i<(1<<gdisp->pixel_size); ++i )
	clut->clut[i] = COLOR_CREATE(cols[i].red>>8, cols[i].green>>8, cols[i].blue>>8);
    clut->is_grey = ( gdisp->visual->class==StaticGray || gdisp->visual->class==GrayScale );
return( gi );
}

static GImage *xi16_to_gi32(GXDisplay *gdisp,XImage *xi) {
    GImage *gi;
    struct _GImage *base;
    uint16_t *pt; uint32_t *ipt, val;
    int i,j,rs,gs,bs;
    int rs2,gs2=0,bs2;
    int rm, gm, bm;

    if (( gi = GImageCreate(it_true,xi->width,xi->height))==NULL )
return( NULL );
    base = gi->u.image;

    rs = gdisp->cs.red_shift; gs = gdisp->cs.green_shift; bs = gdisp->cs.blue_shift;
    rm = gdisp->visual->red_mask; gm = gdisp->visual->green_mask; bm = gdisp->visual->blue_mask;
    if ( rs>gs && rs>bs ) {
	rs2 = 8-(16-rs);
	if ( gs>bs ) {
	    bs2 = 8-gs2;
	    gs2 = 8-(rs-gs);
	} else {
	    gs2 = 8-bs;
	    bs2 = 8-(rs-bs);
	}
    } else if ( gs>rs && gs>bs ) {
	gs2 = 8-(16-gs);
	if ( rs>bs ) {
	    bs2 = 8-rs;
	    rs2 = 8-(gs-rs);
	} else {
	    rs2 = 8-bs;
	    bs2 = 8-(gs-bs);
	}
    } else {
	bs2 = 8-(16-bs);
	if ( rs>gs ) {
	    gs2 = 8-rs;
	    rs2 = 8-(bs-rs);
	} else {
	    rs2 = 8-gs;
	    gs2 = 8-(bs-gs);
	}
    }

    for ( i=0; i<base->height; ++i ) {
	pt = (uint16_t *) (xi->data + i*xi->bytes_per_line);
	ipt = (uint32_t *) (base->data + i*base->bytes_per_line);
	for ( j=0; j<base->width; ++j ) {
	    val = *pt++;
	    if ( val!=0 )
		val = pt[-1];
	    *ipt++ = COLOR_CREATE(((val&rm)>>rs)<<rs2,((val&gm)>>gs)<<gs2,((val&bm)>>bs)<<bs2);
	}
    }
return( gi );
}

static GImage *xi24_to_gi32(GXDisplay *gdisp,XImage *xi) {
    GImage *gi;
    struct _GImage *base;
    uint8_t *pt; uint32_t *ipt, val;
    int i,j,rs,gs,bs;

    if (( gi = GImageCreate(it_true,xi->width,xi->height))==NULL )
return( NULL );
    base = gi->u.image;

    rs = gdisp->cs.red_shift; gs = gdisp->cs.green_shift; bs = gdisp->cs.blue_shift;
    for ( i=0; i<base->height; ++i ) {
	pt = (uint8_t *) xi->data + i*xi->bytes_per_line;
	ipt = (uint32_t *) (base->data + i*base->bytes_per_line);
	for ( j=0; j<base->width; ++j ) {
	    if ( xi->byte_order==MSBFirst ) {
		val = *pt++;
		val = (val<<8) + *pt++;
		val = (val<<8) + *pt++;
	    } else {
		val = *pt++;
		val |= (*pt++<<8);
		val |= (*pt++<<16);
	    }
	    *ipt++ = COLOR_CREATE((val>>rs)&0xff,(val>>gs)&0xff,(val>>bs)&0xff);
	}
    }
return( gi );
}

static GImage *xi32_to_gi32(GXDisplay *gdisp,XImage *xi) {
    GImage *gi;
    struct _GImage *base;
    uint32_t *pt; uint32_t *ipt, val;
    int i,j,rs,gs,bs;

    if (( gi = GImageCreate(it_true,xi->width,xi->height))==NULL )
return( NULL );
    base = gi->u.image;

    rs = gdisp->cs.red_shift; gs = gdisp->cs.green_shift; bs = gdisp->cs.blue_shift;
    for ( i=0; i<base->height; ++i ) {
	pt = (uint32_t *) (xi->data + i*xi->bytes_per_line);
	ipt = (uint32_t *) (base->data + i*base->bytes_per_line);
	for ( j=0; j<base->width; ++j ) {
	    val = *pt++;
	    *ipt++ = COLOR_CREATE((val>>rs)&0xff,(val>>gs)&0xff,(val>>bs)&0xff);
	}
    }
return( gi );
}

GImage *_GXDraw_CopyScreenToImage(GWindow _w, GRect *rect) {
    GXWindow gw = (GXWindow) _w;
    GXDisplay *gdisp = gw->display;
    Display *display=gdisp->display;
    Window w = gw->w;
    int depth;
    XImage *xi;
    GImage *gi=NULL;

    depth = gdisp->pixel_size;
    if ( gw->ggc->bitmap_col ) depth = 1;

    if ( depth!=1 && depth!=8 && depth!=16 && depth!=24 && depth!=32 )
return( NULL );
    xi = XGetImage(display,w,rect->x,rect->y, rect->width, rect->height,
	    -1,ZPixmap);
    if ( xi==NULL )
return( NULL );
    switch ( xi->bits_per_pixel ) {
      case 1:
	gi = xi1_to_gi1(gdisp,xi);
      break;
      case 8:
	gi = xi8_to_gi8(gdisp,xi);
      break;
      case 16:
	gi = xi16_to_gi32(gdisp,xi);
      break;
      case 24:
	gi = xi24_to_gi32(gdisp,xi);
      break;
      case 32:
	gi = xi32_to_gi32(gdisp,xi);
      break;
    }
    XDestroyImage(xi);
return( gi );
}
#else	/* NO X */
int gimagexdraw_a_file_must_define_something=3;

#endif
