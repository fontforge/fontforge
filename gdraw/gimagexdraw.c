/* Copyright (C) 2000-2008 by George Williams */
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
#ifndef X_DISPLAY_MISSING
#include "gxdrawP.h"

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

static void gdraw_8_on_1_nomag_dithered_masked(GXDisplay *gdisp,GImage *image,
	GRect *src) {
    struct gcol clut[256];
    int i,j,end, index;
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

    end = src->width;
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
    int i,j,end, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint32 *pt;
    uint8 *ipt, *mpt;
    short *g_d;
    register int gd;
    int bit;

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.green_dith[i] = 0;

    end = src->width;
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
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
    int i,j,end;
    unsigned int index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint32 *pt;
    uint8 *ipt, *mpt;
    short *g_d;
    register int gd;
    int bit;

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.green_dith[i] = 0;

    end = src->width;
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
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
    int i,j,end, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    unsigned char *pt, *ipt, *mpt;
    short *g_d;
    register int gd;
    struct gcol *pos;
    int bit;

    _GDraw_getimageclut(base,clut);

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.green_dith[i] = 0;

    end = src->width;
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
		if (( bit>>=1 )==0 ) {bit=0x80; ++ipt; ++mpt;};
	    } else {
		if (( bit<<=1 )==256 ) {bit=0x1; ++ipt; ++mpt;};
	    }
	}
    }
}

static void gdraw_32_on_1_nomag_dithered_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    int i,j,end, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    uint32 *pt;
    uint8 *ipt, *mpt;
    short *g_d;
    register int gd;
    int bit;

    _GDraw_getimageclut(base,clut);

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.green_dith[i] = 0;

    end = src->width;
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
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
		if (( bit>>=1 )==0 ) {bit=0x80; ++ipt; ++mpt;};
	    } else {
		if (( bit<<=1 )==256 ) {bit=0x1; ++ipt; ++mpt;};
	    }
	}
    }
}

static void gdraw_8_on_8_nomag_dithered_masked(GXDisplay *gdisp, GImage *image, GRect *src) {
    struct gcol clut[256];
    int i,j, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint8 *pt, *ipt, *mpt;
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
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
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
    register uint8 *pt, *ipt, *mpt;
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
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
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
    uint32 *pt, index;
    uint8 *ipt, *mpt;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    const struct gcol *pos;
#if FAST_BITS
    int mbit;
#endif

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.red_dith[i]= gdisp->gg.green_dith[i] = gdisp->gg.blue_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
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
    uint32 *pt, index;
    uint8 *ipt, *mpt;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    const struct gcol *pos;
#if FAST_BITS
    int mbit;
#endif

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.red_dith[i]= gdisp->gg.green_dith[i] = gdisp->gg.blue_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
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
    uint32 *pt, index;
    register uint8 *ipt, *mpt;
#if FAST_BITS
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
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
    uint32 *pt, index;
    register uint8 *ipt, *mpt;
#if FAST_BITS
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
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
    uint8 *pt, *ipt;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    const struct gcol *pos;

    _GDraw_getimageclut(base,clut);

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.red_dith[i]= gdisp->gg.green_dith[i] = gdisp->gg.blue_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
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
    register uint8 *pt, *ipt;
    struct gcol *pos; const struct gcol *temp;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	temp = _GImage_GetIndexedPixel(COLOR_CREATE(pos->red,pos->green,pos->blue),gdisp->cs.rev);
	pos->pixel = temp->pixel;
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = clut[index].pixel;
	}
    }
}

static void gdraw_32_on_8_nomag_dithered_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    uint32 *pt, index;
    uint8 *ipt;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    const struct gcol *pos;

    for ( i=src->width-1; i>=0; --i )
	gdisp->gg.red_dith[i]= gdisp->gg.green_dith[i] = gdisp->gg.blue_dith[i] = 0;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
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
    uint32 *pt, index;
    register uint8 *ipt;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
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
    uint16 *mpt;
#else
    int mbit;
    uint8 *mpt;
#endif
    register uint8 *pt;
    uint16 *ipt;
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
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint16 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint16 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
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
    register uint32 *pt, index;
    register uint16 *ipt;
    int endian_mismatch = gdisp->endian_mismatch;
#if FAST_BITS==0
    register uint16 *mpt;
#else
    register uint8 *mpt;
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint16 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint16 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
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
    register uint32 *pt, index;
    register uint16 *ipt;
    int endian_mismatch = gdisp->endian_mismatch;
#if FAST_BITS==0
    register uint16 *mpt;
#else
    register uint8 *mpt;
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint16 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint16 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
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
    register uint8 *pt;
    uint16 *ipt;
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
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint16 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = clut[index].pixel;
	}
    }
}

static void gdraw_32_on_16_nomag_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    register uint32 *pt, index;
    register uint16 *ipt;
    int endian_mismatch = gdisp->endian_mismatch;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint16 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = Pixel16(gdisp,index);
	    if ( endian_mismatch )
		ipt[-1] = FixEndian16(ipt[-1]);
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
    register uint8 *ipt;
    register uint8 *pt, *mpt;
    struct gcol *pos;
    int msbf = gdisp->gg.img->byte_order == MSBFirst/*,
	    msBf = gdisp->gg.mask->bitmap_bit_order == MSBFirst*/;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	pos->pixel = Pixel24(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
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
		register uint32 col = clut[index].pixel;
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
    register uint32 *pt, index;
    register uint8 *mpt, *ipt;
#if FAST_BITS
    int mbit;
#endif
    int msbf = gdisp->gg.img->byte_order == MSBFirst;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
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
    register uint32 *pt, index;
    register uint8 *mpt, *ipt;
#if FAST_BITS
    int mbit;
#endif
    int msbf = gdisp->gg.img->byte_order == MSBFirst;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
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
    register uint8 *ipt;
    register uint8 *pt;
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
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
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
    register uint32 *pt, index;
    register uint8 *ipt;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (gdisp->gg.img->data) + (i-src->y)*gdisp->gg.img->bytes_per_line;
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
    register uint8 *pt;
    uint32 *ipt;
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
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint32 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = clut[index].pixel;
	}
    }
}

static void gdraw_32_on_32a_nomag(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint32 *pt, index, *ipt;
    int endian_mismatch = gdisp->endian_mismatch;
    int has_alpha = base->image_type == it_rgba;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint32 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
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
    register uint32 *mpt;
#else
    int mbit;
    register uint8 *mpt;
#endif
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint8 *pt;
    uint32 *ipt;
    struct gcol *pos;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	pos->pixel = Pixel32(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
	if ( gdisp->endian_mismatch )
	    pos->pixel = FixEndian32(pos->pixel);
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint32 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint32 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8 *) (gdisp->gg.mask->data) + (i-src->y)*gdisp->gg.mask->bytes_per_line;
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
    register uint32 *pt, index, *ipt;
    int endian_mismatch = gdisp->endian_mismatch;
#if FAST_BITS==0
    register uint32 *mpt;
#else
    register uint8 *mpt;
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint32 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint32 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
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
    register uint32 *pt, index, *ipt;
    int endian_mismatch = gdisp->endian_mismatch;
#if FAST_BITS==0
    register uint32 *mpt;
#else
    register uint8 *mpt;
    int mbit;
#endif

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint32 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
#if FAST_BITS==0
	mpt = (uint32 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
#else
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
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
    register uint8 *pt;
    uint32 *ipt;
    struct gcol *pos;

    _GDraw_getimageclut(base,clut);
    for ( i=base->clut->clut_len-1; i>=0; --i ) {
	pos = &clut[i];
	pos->pixel = Pixel32(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
	if ( gdisp->endian_mismatch )
	    pos->pixel = FixEndian32(pos->pixel);
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint32 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    *ipt++ = clut[index].pixel;
	}
    }
}

static void gdraw_32_on_32_nomag_nomask(GXDisplay *gdisp, GImage *image, GRect *src) {
    int i,j;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    register uint32 *pt, index, *ipt;
    register uint8 *mpt;
    int mbit;
    int endian_mismatch = gdisp->endian_mismatch;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint32 *) (gdisp->gg.img->data + (i-src->y)*gdisp->gg.img->bytes_per_line);
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-src->y)*gdisp->gg.mask->bytes_per_line);
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==trans ) {
		*ipt++ = Pixel32(gdisp,0);
		*mpt |= mbit;
	    } else {
		*ipt++ = Pixel32(gdisp,index);
		if ( endian_mismatch )
		    ipt[-1] = FixEndian32(ipt[-1]);
		*mpt &= ~mbit;
	    }
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;};
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;};
	    }
	}
    }
}

static void gdraw_xbitmap(GXWindow w, XImage *xi, GClut *clut,
	Color trans, GRect *src, int x, int y) {
    GXDisplay *gdisp = w->display;
    Display *display = gdisp->display;
    int depth = gdisp->depth;
    GC gc = gdisp->gcstate[w->ggc->bitmap_col].gc;
    Color fg, bg;

    if ( trans!=COLOR_UNKNOWN ) {
	XSetFunction(display,gc,GXand);
	if ( trans==1 ) {
	    XSetForeground(display,gc, ~((-1)<<depth) );
	    XSetBackground(display,gc, gdisp->cs.alpha_bits );
	} else {
	    XSetForeground(display,gc, gdisp->cs.alpha_bits );
	    XSetBackground(display,gc, ~((-1)<<depth) );
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

    xi = XCreateImage(gdisp->display,gdisp->visual,1,XYBitmap,0,(char *) (image->data),
	    image->width, image->height,8,image->bytes_per_line);
    if ( xi->bitmap_bit_order==LSBFirst ) {
	/* sigh. The server doesn't use our convention. I might be able just */
	/*  to change this field but it doesn't say, so best not to */
	int len = image->bytes_per_line*image->height;
	uint8 *newdata = galloc(len), *pt, *ipt, *end;
	int m1,m2,val;

	for ( ipt = image->data, pt=newdata, end=pt+len; pt<end; ++pt, ++ipt ) {
	    val = 0;
	    for ( m1=1, m2=0x80; m2!=0; m1<<=1, m2>>=1 )
		if ( *ipt&m1 ) val|=m2;
	    *pt = val;
	}
	xi->data = (char *) newdata;
    }
    gdraw_xbitmap(w,xi,clut,trans,src,x,y);
    if ( (uint8 *) (xi->data)==image->data ) xi->data = NULL;
    XDestroyImage(xi);
}

static void check_image_buffers(GXDisplay *gdisp, int neww, int newh, int is_bitmap) {
    int width = gdisp->gg.iwidth, height = gdisp->gg.iheight;
    char *temp;
    int depth = gdisp->depth, pixel_size;
    union { int32 foo; uint8 bar[4]; } endian;

    if ( is_bitmap ) depth=1;
    if ( neww > gdisp->gg.iwidth ) {
	width = neww;
	if ( width<400 ) width = 400;
    }
    if ( width > gdisp->gg.iwidth || depth!=gdisp->gg.img->depth ) {
	if ( depth<=8 ) {
	    if ( gdisp->gg.red_dith!=NULL ) free(gdisp->gg.red_dith);
	    if ( gdisp->gg.green_dith!=NULL ) free(gdisp->gg.green_dith);
	    if ( gdisp->gg.blue_dith!=NULL ) free(gdisp->gg.blue_dith);
	    gdisp->gg.red_dith = galloc(width*sizeof(short));
	    gdisp->gg.green_dith = galloc(width*sizeof(short));
	    gdisp->gg.blue_dith = galloc(width*sizeof(short));
	    if ( gdisp->gg.red_dith==NULL || gdisp->gg.green_dith==NULL || gdisp->gg.blue_dith==NULL )
		gdisp->do_dithering = 0;
	} else {
	    if ( gdisp->gg.red_dith!=NULL ) free(gdisp->gg.red_dith);
	    if ( gdisp->gg.green_dith!=NULL ) free(gdisp->gg.green_dith);
	    if ( gdisp->gg.blue_dith!=NULL ) free(gdisp->gg.blue_dith);
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

    if ( gdisp->gg.img!=NULL )
	XDestroyImage(gdisp->gg.img);
    if ( gdisp->gg.mask!=NULL )
	XDestroyImage(gdisp->gg.mask);
    pixel_size = gdisp->pixel_size;
    temp = galloc(((width*pixel_size+gdisp->bitmap_pad-1)/gdisp->bitmap_pad)*
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
    temp = galloc(((width*pixel_size+gdisp->bitmap_pad-1)/gdisp->bitmap_pad)*
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

void _GXDraw_Image( GWindow _w, GImage *image, GRect *src, int32 x, int32 y) {
    GXWindow gw = (GXWindow) _w;
    GXDisplay *gdisp = gw->display;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    Display *display=gdisp->display;
    Window w = gw->w;
    GC gc = gdisp->gcstate[gw->ggc->bitmap_col].gc;
    int depth;

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

    gximage_to_ximage(gw, image, src);

    if ( !gdisp->supports_alpha_images && (base->trans!=COLOR_UNKNOWN || base->image_type==it_rgba )) {
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
	gdisp->gcstate[gw->ggc->bitmap_col].func = df_copy;
    } else { /* no mask */
	XPutImage(display,w,gc,gdisp->gg.img,0,0,
		x,y, src->width, src->height );
    }
}

void _GXDraw_TileImage( GWindow _w, GImage *image, GRect *src, int32 x, int32 y) {
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];

    if ( src->x/base->width == (src->x+src->width-1)/base->width &&
	    src->y/base->height == (src->y+src->height-1)/base->height ) {
	/* Ok, the exposed area is entirely covered by one instance of the image*/
	int newx = src->x, newy = src->y;
	GRect newr;
	newr.x = (src->x-x)%base->width; newr.y = (src->y-y)%base->height;
	newr.width = src->width; newr.height = src->height;
	_GXDraw_Image(_w,image,&newr,newx,newy);
    } else if ( base->trans==COLOR_UNKNOWN || base->image_type==it_mono ) {
	GWindow pixmap;
	GXWindow gw = (GXWindow) _w;
	GXDisplay *gdisp = gw->display;
	Display *display=gdisp->display;
	Window w = gw->w;
	GC gc = gdisp->gcstate[gw->ggc->bitmap_col].gc;
	GRect full, old;
	int i,j;

	full.x = full.y = 0; full.width = base->width; full.height = base->height;
	pixmap = GDrawCreatePixmap((GDisplay *) gdisp,base->width,base->height);
	_GXDraw_Image(pixmap,image,&full,0,0);
	GDrawPushClip(_w,src,&old);
	_GXDraw_SetClipFunc(gdisp,gw->ggc);
	for ( i=y; i<gw->ggc->clip.y+gw->ggc->clip.height; i+=base->height ) {
	    if ( i+base->height<gw->ggc->clip.y )
	continue;
	    for ( j=x; j<gw->ggc->clip.x+gw->ggc->clip.width; j+=base->width ) {
		if ( j+base->width<gw->ggc->clip.x )
	    continue;
		XCopyArea(display,((GXWindow) pixmap)->w,w,gc,
			0,0,  base->width, base->height,
			j,i);
	    }
	}
	GDrawPopClip(_w,&old);
	GDrawDestroyWindow(pixmap);
    } else {
	GWindow pixmap, maskmap;
	GXWindow gw = (GXWindow) _w;
	GXDisplay *gdisp = gw->display;
	Display *display=gdisp->display;
	Window w = gw->w;
	GC gc = gdisp->gcstate[gw->ggc->bitmap_col].gc;
	GRect full, old;
	int i,j;

	full.x = full.y = 0; full.width = base->width; full.height = base->height;
	pixmap = GDrawCreatePixmap((GDisplay *) gdisp,base->width,base->height);
	maskmap = GDrawCreatePixmap((GDisplay *) gdisp,base->width,base->height);
	gximage_to_ximage(gw, image, &full);
	GDrawDestroyWindow(maskmap);
#if FAST_BITS
	XSetForeground(display,gc, ~((-1)<<gdisp->pixel_size) );
	XSetBackground(display,gc, 0 );
#endif
	XSetFunction(display,gc,GXcopy);
	XPutImage(display,((GXWindow) maskmap)->w,gc,gdisp->gg.mask,0,0,
		x,y, src->width, src->height );
	XPutImage(display,((GXWindow) pixmap)->w,gc,gdisp->gg.img,0,0,
		x,y, src->width, src->height );
	GDrawPushClip(_w,src,&old);
	_GXDraw_SetClipFunc(gdisp,gw->ggc);
	for ( i=y; i<gw->ggc->clip.y+gw->ggc->clip.height; i+=base->height ) {
	    if ( i+base->height<gw->ggc->clip.y )
	continue;
	    for ( j=x; j<gw->ggc->clip.x+gw->ggc->clip.width; j+=base->width ) {
		if ( j+base->width<gw->ggc->clip.x )
	    continue;
		XSetFunction(display,gc,GXand);
		XCopyArea(display,((GXWindow) maskmap)->w,w,gc,
			0,0,  base->width, base->height,
			j,i);
		XSetFunction(display,gc,GXor);
		XCopyArea(display,((GXWindow) pixmap)->w,w,gc,
			0,0,  base->width, base->height,
			j,i);
	    }
	}
	GDrawPopClip(_w,&old);
	GDrawDestroyWindow(pixmap);
	GDrawDestroyWindow(maskmap);
	XSetFunction(display,gc,GXcopy);
	gdisp->gcstate[gw->ggc->bitmap_col].fore_col = COLOR_UNKNOWN;
	gdisp->gcstate[gw->ggc->bitmap_col].func = df_copy;
    }
}

static XImage *gdraw_1_on_1_mag(GXDisplay *gdisp, GImage *image,int dwid,int dhit,GRect *magsrc) {
    int i,j, index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    uint8 *pt;
    uint8 *ipt;
    int bit, sbit;
    int swid = base->width;
    int shit = base->height;
    XImage *xi;

    xi = XCreateImage(gdisp->display,gdisp->visual,1,XYBitmap,0,NULL,
	    magsrc->width, magsrc->height,8,(magsrc->width+gdisp->bitmap_pad-1)/gdisp->bitmap_pad*(gdisp->bitmap_pad/8));
    xi->data = galloc(xi->bytes_per_line*magsrc->height);

    for ( i=magsrc->y; i<magsrc->y+magsrc->height; ++i ) {
	pt = (uint8 *) (base->data + (i*shit/dhit)*base->bytes_per_line);
	ipt = (uint8 *) (xi->data + (i-magsrc->y)*xi->bytes_per_line);
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
	    bit = 0x80;
	} else {
	    bit = 0x1;
	}
	for ( j=magsrc->x; j<magsrc->x+magsrc->width; ++j ) {
	    index = (j*swid)/dwid;
	    sbit = 0x80 >> (index&7);
	    if ( pt[index>>3]&sbit )
		*ipt |= bit;
	    else
		*ipt &= ~bit;

	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( bit>>=1 )==0 ) {bit=0x80; ++ipt;}
	    } else {
		if (( bit<<=1 )==256 ) {bit=0x1; ++ipt;}
	    }
	}
    }
return( xi );
}

static void gdraw_either_on_1_mag_dithered(GXDisplay *gdisp, GImage *image,int dwid,int dhit,GRect *magsrc) {
    struct gcol clut[256];
    int i,j; uint32 index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    uint8 *pt;
    uint8 *ipt;
    int bit;
    struct gcol *pos;
    int swid = base->width;
    int shit = base->height;
    int is_32bit = base->image_type == it_true || base->image_type == it_rgba;
    int gd;
    int16 *g_d;
    /* I'm not going to deal with masks on bitmap displays */

    if ( !is_32bit )
	_GDraw_getimageclut(base,clut);

    for ( i=magsrc->width-1; i>=0; --i )
	gdisp->gg.green_dith[i] = 0;

    for ( i=magsrc->y; i<magsrc->y+magsrc->height; ++i ) {
	pt = (uint8 *) (base->data + (i*shit/dhit)*base->bytes_per_line);
	ipt = (uint8 *) (gdisp->gg.img->data + (i-magsrc->y)*gdisp->gg.img->bytes_per_line);
	if ( gdisp->gg.img->bitmap_bit_order == MSBFirst )
	    bit = 0x80;
	else
	    bit = 0x1;
	gd = 0;
	g_d = gdisp->gg.green_dith;
	for ( j=magsrc->x; j<magsrc->x+magsrc->width; ++j ) {
	    index = (j*swid)/dwid;
	    if ( is_32bit ) {
	    	index = ((long *) pt)[index];
		index = ((index>>16)&0xff) + ((index>>8)&0xff) + (index&0xff);
		gd += *g_d + index;
	    } else {
		index = pt[index];
		pos = &clut[index];
		gd += *g_d + pos->red + pos->green + pos->blue;
	    }
	    if ( gd<3*128 ) {
		*ipt &= ~bit;
		*g_d++ = gd /= 2;
	    } else {
		*ipt |= bit;
		*g_d++ = gd = (gd - 3*255)/2;
	    }

	    if ( gdisp->gg.img->bitmap_bit_order == MSBFirst ) {
		if (( bit>>=1 )==0 ) {bit=0x80; ++ipt;}
	    } else {
		if (( bit<<=1 )==256 ) {bit=0x1; ++ipt;}
	    }
	}
    }
}

static void gdraw_either_on_8_mag_dithered(GXDisplay *gdisp, GImage *image,int dwid,int dhit,GRect *magsrc) {
    struct gcol clut[256];
    int i,j; uint32 index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint8 *pt, *mpt;
    uint8 *ipt;
    const struct gcol *pos;
    int swid = base->width;
    int shit = base->height;
    int has_alpha = base->image_type == it_rgba;
    int is_32bit = base->image_type == it_true || base->image_type == it_rgba;
    int rd, gd, bd;
    int16 *r_d, *g_d, *b_d;
#if FAST_BITS
    int mbit;
#endif

    if ( !is_32bit )
	_GDraw_getimageclut(base,clut);

    for ( i=magsrc->width-1; i>=0; --i )
	gdisp->gg.red_dith[i]= gdisp->gg.green_dith[i] = gdisp->gg.blue_dith[i] = 0;

    for ( i=magsrc->y; i<magsrc->y+magsrc->height; ++i ) {
	pt = (uint8 *) (base->data + (i*shit/dhit)*base->bytes_per_line);
	ipt = (uint8 *) (gdisp->gg.img->data + (i-magsrc->y)*gdisp->gg.img->bytes_per_line);
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-magsrc->y)*gdisp->gg.mask->bytes_per_line);
#if FAST_BITS
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	rd = gd = bd = 0;
	r_d = gdisp->gg.red_dith; g_d = gdisp->gg.green_dith; b_d = gdisp->gg.blue_dith;
	for ( j=magsrc->x; j<magsrc->x+magsrc->width; ++j ) {
	    index = (j*swid)/dwid;
	    /*if ( index>=src->xend ) index = src->xend-1;*/
	    if ( is_32bit ) {
	    	index = ((uint32 *) pt)[index];
		if ( (index==trans && trans!=-1) || (has_alpha && (index>>24)<0x80 )) {
#if FAST_BITS==0
		    *mpt++ = 0xff;
#else
		    *mpt |= mbit;
#endif
		    *ipt++ = 0x00;
	  goto incr_mbit;
		}
		rd += *r_d + ((index>>16)&0xff);
		gd += *g_d + ((index>>8)&0xff);
		bd += *b_d + (index&0xff);
	    } else {
		index = pt[index];
		if ( index==trans ) {
#if FAST_BITS==0
		    *mpt++ = 0xff;
#else
		    *mpt |= mbit;
#endif
		    *ipt++ = 0x00;
	  goto incr_mbit;
		}
		pos = &clut[index];
		rd += *r_d + pos->red;
		gd += *g_d + pos->green;
		bd += *b_d + pos->blue;
	    }
	    if ( rd<0 ) rd=0; else if ( rd>255 ) rd = 255;
	    if ( gd<0 ) gd=0; else if ( gd>255 ) gd = 255;
	    if ( bd<0 ) bd=0; else if ( bd>255 ) bd = 255;
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
	  incr_mbit:
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;}
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;}
	    }
#else
	    ;
#endif
	}
    }
}

static void gdraw_any_on_8_mag_nodithered(GXDisplay *gdisp, GImage *image,int dwid,int dhit,GRect *magsrc) {
    struct gcol clut[256];
    int i,j; uint32 index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint8 *pt, *mpt;
    uint8 *ipt, pixel;
    struct gcol *pos;
    int swid = base->width;
    int shit = base->height;
    int mbit;
    int has_alpha = base->image_type == it_rgba;
    int is_32bit = base->image_type == it_true || base->image_type==it_rgba,
	    is_1bit = base->image_type == it_mono;

    if ( !is_32bit ) {
	_GDraw_getimageclut(base,clut);
	for ( i=base->clut==NULL?1:base->clut->clut_len-1; i>=0; --i ) {
	    Color col;
	    pos = &clut[i];
	    col = (pos->red<<16)|(pos->green<<8)|pos->blue;
	    pos->pixel = _GXDraw_GetScreenPixel(gdisp,col);
	}
    }

    for ( i=magsrc->y; i<magsrc->y+magsrc->height; ++i ) {
	pt = (uint8 *) (base->data + (i*shit/dhit)*base->bytes_per_line);
	ipt = (uint8 *) (gdisp->gg.img->data + (i-magsrc->y)*gdisp->gg.img->bytes_per_line);
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-magsrc->y)*gdisp->gg.mask->bytes_per_line);
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
	for ( j=magsrc->x; j<magsrc->x+magsrc->width; ++j ) {
	    index = (j*swid)/dwid;
	    /*if ( index>=src->xend ) index = src->xend-1;*/
	    if ( is_32bit ) {
	    	index = ((long *) pt)[index];
		pixel = Pixel24(gdisp,index);
	    } else if ( !is_1bit ) {
		index = pt[index];
		pixel = clut[index].pixel;
	    } else {
		index = (pt[index>>3]&(1<<(7-(index&7)))) ? 1:0;
		pixel = clut[index].pixel;
	    }
	    if ( (index==trans && trans!=-1) || (has_alpha && (index>>24)<0x80 )) {
		*mpt |= mbit;
		*ipt++ = 0x00;
	    } else {
		*ipt++ = pixel;
		*mpt &= ~mbit;
	    }
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;}
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;}
	    }
	}
    }
}

static void gdraw_any_on_16_mag(GXDisplay *gdisp, GImage *image,int dwid,int dhit,GRect *magsrc) {
    struct gcol clut[256];
    int i,j; uint32 index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint8 *pt;
    uint16 *ipt, pixel;
    struct gcol *pos;
    int swid = base->width;
    int shit = base->height;
    int has_alpha = base->image_type == it_rgba;
    int is_32bit = base->image_type == it_true || base->image_type==it_rgba,
	    is_1bit = base->image_type == it_mono;
    register uint16 *mpt;

    if ( !is_32bit ) {
	_GDraw_getimageclut(base,clut);
	for ( i=base->clut==NULL?1:base->clut->clut_len-1; i>=0; --i ) {
	    Color col;
	    pos = &clut[i];
	    col = (pos->red<<16)|(pos->green<<8)|pos->blue;
	    pos->pixel = Pixel16(gdisp,col);
	    if ( gdisp->endian_mismatch )
		pos->pixel = FixEndian16(pos->pixel);
	}
    }

    for ( i=magsrc->y; i<magsrc->y+magsrc->height; ++i ) {
	pt = (uint8 *) (base->data + (i*shit/dhit)*base->bytes_per_line);
	ipt = (uint16 *) (gdisp->gg.img->data + (i-magsrc->y)*gdisp->gg.img->bytes_per_line);
	mpt = (uint16 *) (gdisp->gg.mask->data + (i-magsrc->y)*gdisp->gg.mask->bytes_per_line);
	for ( j=magsrc->x; j<magsrc->x+magsrc->width; ++j ) {
	    index = (j*swid)/dwid;
	    /*if ( index>=src->xend ) index = src->xend-1;*/
	    if ( is_32bit ) {
	    	index = ((long *) pt)[index];
		pixel = Pixel16(gdisp,index);
		if ( gdisp->endian_mismatch )
		    pixel = FixEndian16(pixel);
	    } else if ( !is_1bit ) {
		index = pt[index];
		pixel = clut[index].pixel;
	    } else {
		index = (pt[index>>3]&(1<<(7-(index&7)))) ? 1:0;
		pixel = clut[index].pixel;
	    }
	    if ( (index==trans && trans!=-1) || (has_alpha && (index>>24)<0x80 )) {
		*mpt++ = 0xffff;
		*ipt++ = 0x0000;
	    } else {
		*ipt++ = pixel;
		*mpt++ = 0;
	    }
	}
    }
}

static void gdraw_any_on_24_mag(GXDisplay *gdisp, GImage *image,int dwid,int dhit,GRect *magsrc) {
    struct gcol clut[256];
    int i,j; uint32 index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint8 *pt, *mpt, *ipt;
    uint32 pixel;
    struct gcol *pos;
    int swid = base->width;
    int shit = base->height;
    int has_alpha = base->image_type == it_rgba;
    int is_32bit = base->image_type == it_true || base->image_type==it_rgba,
	    is_1bit = base->image_type == it_mono;
#if FAST_BITS
    int mbit;
#endif

    if ( !is_32bit ) {
	_GDraw_getimageclut(base,clut);
	for ( i=base->clut==NULL?1:base->clut->clut_len-1; i>=0; --i ) {
	    pos = &clut[i];
	    pos->pixel = Pixel24(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
	}
    }

    for ( i=magsrc->y; i<magsrc->y+magsrc->height; ++i ) {
	pt = (uint8 *) (base->data + (i*shit/dhit)*base->bytes_per_line);
	ipt = (uint8 *) (gdisp->gg.img->data + (i-magsrc->y)*gdisp->gg.img->bytes_per_line);
	mpt = (uint8 *) (gdisp->gg.mask->data + (i-magsrc->y)*gdisp->gg.mask->bytes_per_line);
#if FAST_BITS
	if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst )
	    mbit = 0x80;
	else
	    mbit = 0x1;
#endif
	for ( j=magsrc->x; j<magsrc->x+magsrc->width; ++j ) {
	    index = (j*swid)/dwid;
	    /*if ( index>=src->xend ) index = src->xend-1;*/
	    if ( is_32bit ) {
	    	index = ((long *) pt)[index];
		pixel = Pixel24(gdisp,index);
	    } else if ( !is_1bit ) {
		index = pt[index];
		pixel = clut[index].pixel;
	    } else {
		index = (pt[index>>3]&(1<<(7-(index&7)))) ? 1:0;
		pixel = clut[index].pixel;
	    }
	    if ( (index==trans && trans!=-1) || (has_alpha && (index>>24)<0x80 )) {
#if FAST_BITS==0
		*mpt++ = 0xff; *mpt++ = 0xff; *mpt++ = 0xff;
#else
		*mpt |= mbit;
#endif
		*ipt++ = 0x00;
		*ipt++ = 0x00;
		*ipt++ = 0x00;
	    } else {
		if ( gdisp->gg.mask->byte_order == MSBFirst ) {
		    *ipt++ = pixel>>16;
		    *ipt++ = (pixel>>8)&0xff;
		    *ipt++ = pixel&0xff;
		} else {
		    *ipt++ = pixel&0xff;
		    *ipt++ = (pixel>>8)&0xff;
		    *ipt++ = pixel>>16;
		}
#if FAST_BITS==0
		*mpt++ = 0; *mpt++ = 0; *mpt++ = 0;
#else
		*mpt &= ~mbit;
#endif
	    }
#if FAST_BITS
	    if ( gdisp->gg.mask->bitmap_bit_order == MSBFirst ) {
		if (( mbit>>=1 )==0 ) {mbit=0x80; ++mpt;}
	    } else {
		if (( mbit<<=1 )==256 ) {mbit=0x1; ++mpt;}
	    }
#endif
	}
    }
}

static void gdraw_any_on_32_mag(GXDisplay *gdisp, GImage *image,int dwid,int dhit,GRect *magsrc) {
    struct gcol clut[256];
    int i,j; uint32 index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint8 *pt;
    uint32 *ipt, pixel;
    struct gcol *pos;
    int swid = base->width;
    int shit = base->height;
    int has_alpha = base->image_type == it_rgba;
    int is_32bit = base->image_type == it_true || base->image_type==it_rgba,
	    is_1bit = base->image_type == it_mono;
    uint32 *mpt;

    if ( !is_32bit ) {
	_GDraw_getimageclut(base,clut);
	for ( i=base->clut==NULL?1:base->clut->clut_len-1; i>=0; --i ) {
	    pos = &clut[i];
	    pos->pixel = Pixel32(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
	    if ( gdisp->endian_mismatch )
		pos->pixel = FixEndian32(pos->pixel);
	}
    }

    for ( i=magsrc->y; i<magsrc->y+magsrc->height; ++i ) {
	pt = (uint8 *) (base->data + (i*shit/dhit)*base->bytes_per_line);
	ipt = (uint32 *) (gdisp->gg.img->data + (i-magsrc->y)*gdisp->gg.img->bytes_per_line);
	mpt = (uint32 *) (gdisp->gg.mask->data + (i-magsrc->y)*gdisp->gg.mask->bytes_per_line);
	for ( j=magsrc->x; j<magsrc->x+magsrc->width; ++j ) {
	    index = (j*swid)/dwid;
	    /*if ( index>=src->xend ) index = src->xend-1;*/
	    if ( is_32bit ) {
	    	index = ((long *) pt)[index];
		pixel = Pixel32(gdisp,index);
		if ( gdisp->endian_mismatch )
		    pixel = FixEndian32(pixel);
	    } else if ( !is_1bit ) {
		index = pt[index];
		pixel = clut[index].pixel;
	    } else {
		index = (pt[index>>3]&(1<<(7-(index&7)))) ? 1:0;
		pixel = clut[index].pixel;
	    }
	    if ( (index==trans && trans!=-1) || (has_alpha && (index>>24)<0x80 )) {
		*mpt++ = 0xffffffff;
		*ipt++ = 0x00;
	    } else {
		*ipt++ = pixel;
		*mpt++ = 0;
	    }
	}
    }
}

static void gdraw_any_on_32a_mag(GXDisplay *gdisp, GImage *image,int dwid,int dhit,GRect *magsrc) {
    struct gcol clut[256];
    int i,j; uint32 index;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    int trans = base->trans;
    uint8 *pt;
    uint32 *ipt, pixel;
    struct gcol *pos;
    int swid = base->width;
    int shit = base->height;
    int has_alpha = base->image_type == it_rgba;
    int is_32bit = base->image_type == it_true || base->image_type==it_rgba,
	    is_1bit = base->image_type == it_mono;

    if ( !is_32bit ) {
	_GDraw_getimageclut(base,clut);
	for ( i=base->clut==NULL?1:base->clut->clut_len-1; i>=0; --i ) {
	    pos = &clut[i];
	    pos->pixel = Pixel32(gdisp,COLOR_CREATE(pos->red,pos->green,pos->blue));
	    if ( gdisp->endian_mismatch )
		pos->pixel = FixEndian32(pos->pixel);
	}
    }

    for ( i=magsrc->y; i<magsrc->y+magsrc->height; ++i ) {
	pt = (uint8 *) (base->data + (i*shit/dhit)*base->bytes_per_line);
	ipt = (uint32 *) (gdisp->gg.img->data + (i-magsrc->y)*gdisp->gg.img->bytes_per_line);
	for ( j=magsrc->x; j<magsrc->x+magsrc->width; ++j ) {
	    index = (j*swid)/dwid;
	    /*if ( index>=src->xend ) index = src->xend-1;*/
	    if ( is_32bit ) {
	    	index = ((long *) pt)[index];
		pixel = Pixel32(gdisp,index&0xffffff) | (index&0xff000000);
		if ( gdisp->endian_mismatch )
		    pixel = FixEndian32(pixel);
	    } else if ( !is_1bit ) {
		index = pt[index];
		pixel = clut[index].pixel;
	    } else {
		index = (pt[index>>3]&(1<<(7-(index&7)))) ? 1:0;
		pixel = clut[index].pixel;
	    }
	    if ( (index==trans && trans!=-1) || (has_alpha && (index>>24)<0x80 ))
		pixel &= 0x00ffffff;
	    *ipt++ = pixel;
	}
    }
}

/* Given an image, magnify it so that its width/height are as specified */
/*  then extract the given given rectangle (in magnified coords) and */
/*  place it on the screen at x,y */
void _GXDraw_ImageMagnified(GWindow _w, GImage *image, GRect *magsrc,
	int32 x, int32 y, int32 width, int32 height) {
    GXWindow gw = (GXWindow) _w;
    GXDisplay *gdisp = gw->display;
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    Display *display=gdisp->display;
    Window w = gw->w;
    GC gc = gdisp->gcstate[gw->ggc->bitmap_col].gc;
    int depth;
    GRect temp;

    if ( magsrc->height<0 || magsrc->width<0 ||
	    magsrc->height*(double) magsrc->width>24*1024*1024 )
return;

    _GXDraw_SetClipFunc(gdisp,gw->ggc);

    temp.x = temp.y = 0; temp.width = magsrc->width; temp.height = magsrc->height;

    depth = gdisp->pixel_size;
    if ( depth!=8 && depth!=16 && depth!=24 && depth!=32 )
	depth = 1;
    else if ( gw->ggc->bitmap_col )
	depth = 1;

    check_image_buffers(gdisp, magsrc->width, magsrc->height,depth==1);
    if ( base->image_type == it_mono && depth==1 ) {
	XImage *xi;
	/* Mono images are easy, because all X servers (no matter what their */
	/*  depth) support 1 bit bitmaps */
	/* Sadly, they often do it so slowly as to be unusable... */
	xi = gdraw_1_on_1_mag(gdisp,image,width,height,magsrc);
	gdraw_xbitmap(gw,xi,base->clut,base->trans,
		&temp,x+magsrc->x,y+magsrc->y);
	XDestroyImage(xi);
    } else {
	switch ( depth ) {
	  case 1:
	  default:
	    /* all servers can handle bitmaps, so if we don't know how to*/
	    /*  write to a 13bit screen, we can at least give a bitmap */
	    /* But we don't handle masks here */
	    gdraw_either_on_1_mag_dithered(gdisp,image,width,height,magsrc);
	    gdraw_xbitmap(gw,gdisp->gg.img,NULL,-1,
		    &temp,x+magsrc->x,y+magsrc->y);
  goto done;
	  case 8:
	    if ( gdisp->do_dithering && !gdisp->cs.is_grey && base->image_type != it_mono)
		gdraw_either_on_8_mag_dithered(gdisp,image,width,height,magsrc);
	    else
		gdraw_any_on_8_mag_nodithered(gdisp,image,width,height,magsrc);
	  break;
	  case 16:
	    gdraw_any_on_16_mag(gdisp,image,width,height,magsrc);
	  break;
	  case 24:
	    gdraw_any_on_24_mag(gdisp,image,width,height,magsrc);
	  break;
	  case 32:
	    if ( gdisp->supports_alpha_images )
		gdraw_any_on_32a_mag(gdisp,image,width,height,magsrc);
	    else
		gdraw_any_on_32_mag(gdisp,image,width,height,magsrc);
	  break;
	}
	display=gdisp->display;
	w = gw->w;
	gc = gdisp->gcstate[gw->ggc->bitmap_col].gc;
	if ( !gdisp->supports_alpha_images && (base->trans!=COLOR_UNKNOWN || base->image_type==it_rgba )) {
	    XSetFunction(display,gc,GXand);
	    XSetForeground(display,gc, ~((-1)<<gdisp->pixel_size) );
	    XSetBackground(display,gc, 0 );
	    XPutImage(display,w,gc,gdisp->gg.mask,0,0,
		    x+magsrc->x,y+magsrc->y,
		    magsrc->width, magsrc->height );
	    XSetFunction(display,gc,GXor);
	    gdisp->gcstate[gw->ggc->bitmap_col].fore_col = COLOR_UNKNOWN;
	}
	XPutImage(display,w,gc,gdisp->gg.img,0,0,
		x+magsrc->x,y+magsrc->y,
		magsrc->width, magsrc->height );
	XSetFunction(display,gc,GXcopy);
	gdisp->gcstate[gw->ggc->bitmap_col].func = df_copy;
    }
  done:;
}

static GImage *xi1_to_gi1(GXDisplay *gdisp,XImage *xi) {
    GImage *gi;
    struct _GImage *base;

    gi = gcalloc(1,sizeof(GImage));
    base = galloc(sizeof(struct _GImage));
    if ( gi==NULL || base==NULL )
return( NULL );
    gi->u.image = base;
    base->image_type = it_mono;
    base->width = xi->width;
    base->height = xi->height;
    base->bytes_per_line = xi->bytes_per_line;
    base->data = (uint8 *) (xi->data);
    base->clut = NULL;
    base->trans = COLOR_UNKNOWN;

    if ( xi->bitmap_bit_order==LSBFirst ) {
	/* sigh. The server doesn't use our convention. invert all bytes */
	int len = base->height*base->bytes_per_line;
	uint8 *newdata = galloc(len), *pt, *ipt, *end;
	int m1,m2,val;

	for ( ipt = (uint8 *) xi->data, pt=newdata, end=pt+len; pt<end; ++pt, ++ipt ) {
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

    gi = gcalloc(1,sizeof(GImage));
    base = galloc(sizeof(struct _GImage));
    clut = galloc(sizeof(GClut));
    if ( gi==NULL || base==NULL )
return( NULL );
    gi->u.image = base;
    base->image_type = it_index;
    base->width = xi->width;
    base->height = xi->height;
    base->bytes_per_line = xi->bytes_per_line;
    base->data = (uint8 *) xi->data;
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
    uint16 *pt; uint32 *ipt, val;
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
	pt = (uint16 *) (xi->data + i*xi->bytes_per_line);
	ipt = (uint32 *) (base->data + i*base->bytes_per_line);
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
    uint8 *pt; uint32 *ipt, val;
    int i,j,rs,gs,bs;

    if (( gi = GImageCreate(it_true,xi->width,xi->height))==NULL )
return( NULL );
    base = gi->u.image;

    rs = gdisp->cs.red_shift; gs = gdisp->cs.green_shift; bs = gdisp->cs.blue_shift;
    for ( i=0; i<base->height; ++i ) {
	pt = (uint8 *) xi->data + i*xi->bytes_per_line;
	ipt = (uint32 *) (base->data + i*base->bytes_per_line);
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
    uint32 *pt; uint32 *ipt, val;
    int i,j,rs,gs,bs;

    if (( gi = GImageCreate(it_true,xi->width,xi->height))==NULL )
return( NULL );
    base = gi->u.image;

    rs = gdisp->cs.red_shift; gs = gdisp->cs.green_shift; bs = gdisp->cs.blue_shift;
    for ( i=0; i<base->height; ++i ) {
	pt = (uint32 *) (xi->data + i*xi->bytes_per_line);
	ipt = (uint32 *) (base->data + i*base->bytes_per_line);
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
