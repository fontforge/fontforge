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
#include "gdrawP.h"
#include <string.h>

void _GDraw_getimageclut(struct _GImage *base, struct gcol *clut) {
    int i, cnt;
    long col;

    if ( base->clut==NULL ) {
	clut->red = clut->green = clut->blue = 0;
	++clut;
	clut->red = clut->green = clut->blue = 0xff;
	++clut;
	i = 2;
    } else {
	cnt= base->clut->clut_len;
	for ( i=0; i<cnt; ++i, ++clut ) {
	    col = base->clut->clut[i];
	    clut->red = (col>>16)&0xff;
	    clut->green = (col>>8)&0xff;
	    clut->blue = (col&0xff);
	}
    }
    for ( ; i<256; ++i, ++clut ) {
	clut->red = clut->green = clut->blue = 0xff;
	clut->pixel = 0;
    }
}

static int MonoCols(GClut *clut, int *dark, int *bright_col, int *dark_col) {
    int bright;

    if ( clut==NULL ) {
	bright = 1; *bright_col = 3*255;
	*dark = 0; *dark_col = 0;
    } else {
	Color col0 = clut->clut[0], col1 = clut->clut[1];
	int g0 = COLOR_RED(col0)+COLOR_GREEN(col0)+COLOR_BLUE(col0),
	    g1 = COLOR_RED(col1)+COLOR_GREEN(col1)+COLOR_BLUE(col1);
	if ( g1>g0 ) {
	    bright = 1; *bright_col = g1;
	    *dark = 0; *dark_col = g0;
	} else {
	    bright = 0; *bright_col = g0;
	    *dark = 1; *dark_col = g1;
	}
    }
return( bright );
}

static GImage *GImage1to32(struct _GImage *base,GRect *src) {
    GImage *ret;
    struct _GImage *rbase;
    register Color *clut;
    Color fake[2];
    uint8 *pt;
    uint32 *ipt;
    int bit, i,j;

    if ( base->clut==NULL ) {
	fake[0] = COLOR_CREATE(0,0,0);
	fake[1] = COLOR_CREATE(0xff,0xff,0xff);
	clut = fake;
    } else
	clut = base->clut->clut;
    ret = GImageCreate(it_true,src->width,src->height);
    rbase = ret->u.image;
    rbase->trans = base->clut==NULL || base->trans==COLOR_UNKNOWN?COLOR_UNKNOWN:
	    clut[base->trans];

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + (src->x>>3);
	ipt = (uint32 *) (rbase->data + (i-src->y)*rbase->bytes_per_line);
	bit = 0x80>>(src->x&7);
	for ( j=src->width-1; j>=0; --j ) {
	    if ( *pt&bit )
		*ipt++ = clut[1];
	    else
		*ipt++ = clut[0];
	    if (( bit>>=1 )==0 ) {bit=0x80; ++pt;};
	}
    }
return( ret );
}

static GImage *GImage8to32(struct _GImage *base,GRect *src) {
    GImage *ret;
    struct _GImage *rbase;
    register Color *clut = base->clut->clut;
    uint8 *pt;
    uint32 *ipt;
    int i,j;

    ret = GImageCreate(it_true,src->width,src->height);
    rbase = ret->u.image;
    rbase->trans = base->trans==COLOR_UNKNOWN?COLOR_UNKNOWN:
	    clut[base->trans];

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	ipt = (uint32 *) (rbase->data + (i-src->y)*rbase->bytes_per_line);
	for ( j=src->width-1; j>=0; --j )
	    *ipt++ = clut[ *pt++ ];
    }
return( ret );
}

static GImage *GImage32to32(struct _GImage *base,GRect *src) {
    GImage *ret;
    struct _GImage *rbase;
    uint32 *pt;
    uint32 *ipt;
    int i;

    ret = GImageCreate(it_true,src->width,src->height);
    rbase = ret->u.image;
    rbase->trans = base->trans;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = ((uint32 *) (base->data + i*base->bytes_per_line)) + src->x;
	ipt = (uint32 *) (rbase->data + (i-src->y)*rbase->bytes_per_line);
	memcpy(ipt,pt,(src->width)*sizeof(uint32));
    }
return( ret );
}

static GImage *GImage1to8(struct _GImage *base,GRect *src, GClut *nclut, RevCMap *rev) {
    GImage *ret;
    struct _GImage *rbase;
    register Color *clut; Color fake[2];
    uint8 *pt;
    uint8 *ipt;
    int bit;
    uint8 ones, zeros;
    int i,j;
    const struct gcol *stupid_gcc;

    if ( base->clut==NULL ) {
	fake[0] = COLOR_CREATE(0,0,0);
	fake[1] = COLOR_CREATE(0xff,0xff,0xff);
	clut = fake;
    } else
	clut = base->clut->clut;
    ones = _GImage_GetIndexedPixelPrecise(clut[1],rev)->pixel;
    stupid_gcc = _GImage_GetIndexedPixelPrecise(clut[0],rev);
    zeros = stupid_gcc->pixel;
    if ( base->clut!=NULL && nclut->trans_index!=COLOR_UNKNOWN ) {
	if ( base->trans==0 )
	    zeros = nclut->trans_index;
	else if ( base->trans==1 )
	    ones = nclut->trans_index;
    }
	
    ret = GImageCreate(it_index,src->width,src->height);
    rbase = ret->u.image;
    *rbase->clut = *nclut;
    rbase->trans = nclut->trans_index;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + (src->x>>3);
	ipt = (uint8 *) (rbase->data) + (i-src->y)*rbase->bytes_per_line;
	bit = 0x80>>(src->x&7);
	for ( j=src->width-1; j>=0; --j ) {
	    if ( *pt&bit )
		*ipt++ = ones;
	    else
		*ipt++ = zeros;
	    if (( bit>>=1 )==0 ) {bit=0x80; ++pt;};
	}
    }
return( ret );
}

static GImage *GImage8to8(struct _GImage *base,GRect *src, GClut *nclut, RevCMap *rev) {
    GImage *ret;
    struct _GImage *rbase;
    uint8 *pt;
    uint8 *ipt;
    uint32 from_trans=COLOR_UNKNOWN, to_trans;
    int i,j;

    to_trans = nclut->trans_index;
    if ( to_trans!=COLOR_UNKNOWN )
	from_trans = base->trans;
    ret = GImageCreate(it_index,src->width,src->height);
    rbase = ret->u.image;
    *rbase->clut = *nclut;
    rbase->trans = nclut->trans_index;
    if ( nclut==base->clut || GImageSameClut(base->clut,nclut) ) {
	for ( i=src->y; i<src->y+src->height; ++i ) {
	    pt = (uint8 *) (base->data) + i*base->bytes_per_line + src->x;
	    ipt = (uint8 *) (rbase->data) + (i-src->y)*rbase->bytes_per_line;
	    memcpy(ipt,pt,src->width);
	}
    } else {
	struct gcol gclut[256];
	short *red_dith, *green_dith, *blue_dith;
	short *r_d, *g_d, *b_d;
	register int rd, gd, bd;
	int index;
	const struct gcol *pos;

	_GDraw_getimageclut(base,gclut);
	red_dith = calloc(src->width,sizeof(short));
	green_dith = calloc(src->width,sizeof(short));
	blue_dith = calloc(src->width,sizeof(short));
	for ( i=src->y; i<src->y+src->height; ++i ) {
	    pt = (uint8 *) (base->data + i*base->bytes_per_line) + src->x;
	    ipt = (uint8 *) (rbase->data) + (i-src->y)*rbase->bytes_per_line;
	    rd = gd = bd = 0;
	    r_d = red_dith; g_d = green_dith; b_d = blue_dith;
	    for ( j=src->width-1; j>=0; --j ) {
		index = *pt++;
		if ( index==from_trans ) {
		    *ipt++ = to_trans;
		    ++r_d; ++g_d; ++b_d;
		} else {
		    pos = &gclut[index];
		    rd += *r_d + pos->red; if ( rd<0 ) rd=0; else if ( rd>255 ) rd = 255;
		    gd += *g_d + pos->green; if ( gd<0 ) gd=0; else if ( gd>255 ) gd = 255;
		    bd += *b_d + pos->blue; if ( bd<0 ) bd=0; else if ( bd>255 ) bd = 255;
		    pos = _GImage_GetIndexedPixelPrecise(COLOR_CREATE(rd,gd,bd),rev);
		    *ipt++ = pos->pixel;
		    *r_d++ = rd = (rd - pos->red)/2;
		    *g_d++ = gd = (gd - pos->green)/2;
		    *b_d++ = bd = (bd - pos->blue)/2;
		}
	    }
	}
	free(red_dith); free(green_dith); free(blue_dith);
    }
return( ret );
}

static GImage *GImage32to8(struct _GImage *base,GRect *src, GClut *nclut, RevCMap *rev) {
    GImage *ret;
    struct _GImage *rbase;
    uint32 *pt;
    uint8 *ipt;
    uint32 from_trans=COLOR_UNKNOWN, to_trans;
    int i,j;
    short *red_dith, *green_dith, *blue_dith;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    int index;
    const struct gcol *pos;

    to_trans = nclut->trans_index;
    if ( to_trans!=COLOR_UNKNOWN )
	from_trans = base->trans;
    ret = GImageCreate(it_index,src->width,src->height);
    rbase = ret->u.image;
    *rbase->clut = *nclut;
    rbase->trans = nclut->trans_index;

    red_dith = calloc(src->width,sizeof(short));
    green_dith = calloc(src->width,sizeof(short));
    blue_dith = calloc(src->width,sizeof(short));
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (ret->u.image->data) + (i-src->y)*ret->u.image->bytes_per_line;
	rd = gd = bd = 0;
	r_d = red_dith; g_d = green_dith; b_d = blue_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==from_trans ) {
		*ipt++ = to_trans;
		++r_d; ++g_d; ++b_d;
	    } else {
		rd += *r_d + COLOR_RED(index); if ( rd<0 ) rd=0; else if ( rd>255 ) rd = 255;
		gd += *g_d + COLOR_GREEN(index); if ( gd<0 ) gd=0; else if ( gd>255 ) gd = 255;
		bd += *b_d + COLOR_BLUE(index); if ( bd<0 ) bd=0; else if ( bd>255 ) bd = 255;
		pos = _GImage_GetIndexedPixelPrecise(COLOR_CREATE(rd,gd,bd),rev);
		*ipt++ = pos->pixel;
		*r_d++ = rd = (rd - pos->red)/2;
		*g_d++ = gd = (gd - pos->green)/2;
		*b_d++ = bd = (bd - pos->blue)/2;
	    }
	}
    }
    free(red_dith); free(green_dith); free(blue_dith);
return( ret );
}

static GImage *GImage1to1(struct _GImage *base,GRect *src, GClut *nclut) {
    GImage *ret;
    struct _GImage *rbase;
    uint8 *pt;
    uint8 *ipt;
    int bit, ibit;
    uint8 ones, zeros;
    int i,j;

    {
	register Color *clut; Color fake[2];
	int bright, dark, bright_col, dark_col;
	if ( base->clut==NULL ) {
	    fake[0] = COLOR_CREATE(0,0,0);
	    fake[1] = COLOR_CREATE(0xff,0xff,0xff);
	    clut = fake;
	} else
	    clut = base->clut->clut;
	bright = MonoCols(nclut,&dark,&bright_col,&dark_col);
	if ( COLOR_RED(clut[0])+COLOR_GREEN(clut[0])+COLOR_BLUE(clut[0]) >
		COLOR_RED(clut[1])+COLOR_GREEN(clut[1])+COLOR_BLUE(clut[1]) ) {
	    zeros = bright;
	    ones = dark;
	} else {
	    zeros = dark;
	    ones = bright;
	}
    }
    if ( base->clut!=NULL && nclut!=NULL && nclut->trans_index!=COLOR_UNKNOWN ) {
	if ( base->trans==0 ) {
	    zeros = nclut->trans_index;
	    ones = !zeros;
	} else if ( base->trans==1 ) {
	    ones = nclut->trans_index;
	    zeros = !ones;
	}
    }

    ret = GImageCreate(it_mono,src->width,src->height);
    rbase = ret->u.image;
    if ( nclut!=NULL ) {
	rbase->clut = calloc(1,sizeof(GClut));
	*rbase->clut = *nclut;
	rbase->trans = nclut->trans_index;
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data) + i*base->bytes_per_line + (src->x>>3);
	ipt = (uint8 *) (rbase->data) + (i-src->y)*rbase->bytes_per_line;
	bit = 0x80>>(src->x&7);
	ibit = 0x80;
	if ( ones==0 ) {
	    for ( j=src->width-1; j>=0; --j ) {
		if ( *pt&bit )
		    *ipt &= ~ibit;
		else
		    *ipt |= ibit;
		if (( bit>>=1 )==0 ) {bit=0x80; ++pt;};
		if (( ibit>>=1 )==0 ) {ibit=0x80; ++ipt;};
	    }
	} else {
	    for ( j=src->width-1; j>=0; --j ) {
		if ( *pt&bit )
		    *ipt |= ibit;
		else
		    *ipt &= ~ibit;
		if (( bit>>=1 )==0 ) {bit=0x80; ++pt;};
		if (( ibit>>=1 )==0 ) {ibit=0x80; ++ipt;};
	    }
	}
    }
return( ret );
}

static GImage *GImage8to1(struct _GImage *base,GRect *src, GClut *nclut) {
    GImage *ret;
    struct _GImage *rbase;
    uint8 *pt;
    uint8 *ipt;
    uint32 from_trans=COLOR_UNKNOWN, to_trans;
    struct gcol gclut[256];
    int bit;
    short *grey_dith;
    short *g_d;
    register int gd;
    int bright, dark;
    int bright_col, dark_col;
    int i,j;
    int index;
    const struct gcol *pos;

    to_trans = COLOR_UNKNOWN;
    if ( nclut!=NULL )
	to_trans = nclut->trans_index;
    if ( to_trans!=COLOR_UNKNOWN )
	from_trans = base->trans;

    ret = GImageCreate(it_mono,src->width,src->height);
    rbase = ret->u.image;
    if ( nclut!=NULL ) {
	rbase->clut = calloc(1,sizeof(GClut));
	*rbase->clut = *nclut;
	rbase->trans = nclut->trans_index;
    }

    _GDraw_getimageclut(base,gclut);
    bright = MonoCols(nclut, &dark,&bright_col,&dark_col);

    grey_dith = calloc(src->width,sizeof(short));
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (rbase->data) + (i-src->y)*rbase->bytes_per_line;
	bit = 0x80;
	gd = 0;
	g_d = grey_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==from_trans ) {
		*ipt++ = to_trans;
		++g_d;
	    } else {
		pos = &gclut[index];
		gd += *g_d + pos->red+pos->green+pos->blue;
		if (( gd<3*128 && dark) || (gd>=3*128 && bright))
		    *ipt |= bit;
		else
		    *ipt &= ~bit;
		if ( gd<0 ) gd=0; else if ( gd>3*255 ) gd = 3*255;
		if ( gd<3*128 )
		    *g_d++ = gd = (gd-dark_col)/2;
		else
		    *g_d++ = gd = (gd-bright_col)/2;
	    }
	    if (( bit>>=1 )==0 ) {bit=0x80; ++ipt;};
	}
    }
    free(grey_dith);
return( ret );
}

static GImage *GImage32to1(struct _GImage *base,GRect *src, GClut *nclut) {
    GImage *ret;
    struct _GImage *rbase;
    uint32 *pt;
    uint8 *ipt;
    uint32 from_trans=COLOR_UNKNOWN, to_trans;
    int bit;
    short *grey_dith;
    short *g_d;
    register int gd;
    int bright, dark, bright_col, dark_col;
    int i,j, index;

    to_trans = COLOR_UNKNOWN;
    if ( nclut!=NULL )
	to_trans = nclut->trans_index;
    if ( to_trans!=COLOR_UNKNOWN )
	from_trans = base->trans;

    ret = GImageCreate(it_mono,src->width,src->height);
    rbase = ret->u.image;
    if ( nclut!=NULL ) {
	rbase->clut = calloc(1,sizeof(GClut));
	*rbase->clut = *nclut;
	rbase->trans = nclut->trans_index;
    }
    bright = MonoCols(nclut,&dark,&bright_col,&dark_col);

    grey_dith = calloc(src->width,sizeof(short));
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (base->data + i*base->bytes_per_line) + src->x;
	ipt = (uint8 *) (rbase->data) + (i-src->y)*rbase->bytes_per_line;
	bit = 0x80;
	gd = 0;
	g_d = grey_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==from_trans ) {
		if ( to_trans ) *ipt |= bit; else *ipt &= ~bit;
		++g_d;
	    } else {
		gd += *g_d + COLOR_RED(index) + COLOR_GREEN(index) + COLOR_BLUE(index);
		if (( gd<3*128 && dark) || (gd>=3*128 && bright))
		    *ipt |= bit;
		else
		    *ipt &= ~bit;
		if ( gd<0 ) gd=0; else if ( gd>3*255 ) gd = 3*255;
		if ( gd<3*128 )
		    *g_d++ = gd = (gd-dark_col)/2;
		else
		    *g_d++ = gd = (gd-bright_col)/2;
	    }
	    if (( bit>>=1 )==0 ) {bit=0x80; ++ipt;}
	}
    }
    free(grey_dith);
return( ret );
}

GImage *GImageBaseGetSub(struct _GImage *base, enum image_type it, GRect *src, GClut *nclut, RevCMap *rev) {
    RevCMap *oldrev = rev;
    GImage *ret = NULL;
    GRect temp;

    if ( src==NULL ) {
	src = &temp;
	temp.x = temp.y = 0;
	temp.width = base->width;
	temp.height = base->height;
    }

    if ( src->width<0 || src->height<0 ) {
	GDrawIError("Invalid rectangle in GImageGetSub");
return( NULL );
    }
    switch ( it ) {
      case it_mono:
	switch ( base->image_type ) {
	  case it_mono:
return( GImage1to1(base,src,nclut));
	  case it_index:
return( GImage8to1(base,src,nclut));
	  case it_true:
return( GImage32to1(base,src,nclut));
	  default:
	    GDrawIError("Bad image type %d", base->image_type);
return( NULL );
	}
      case it_index:
	if ( rev==NULL )
	    rev = GClutReverse(nclut,8);
	switch ( base->image_type ) {
	  case it_mono:
	    ret = GImage1to8(base,src,nclut,rev);
	  break;
	  case it_index:
	    ret = GImage8to8(base,src,nclut,rev);
	  break;
	  case it_true:
	    ret = GImage32to8(base,src,nclut,rev);
	  break;
	  default:
	    GDrawIError("Bad image type %d", base->image_type);
	    ret = NULL;
	  break;
	}
	if ( oldrev==NULL )
	    GClut_RevCMapFree(rev);
return( ret );
      case it_true:
	switch ( base->image_type ) {
	  case it_mono:
return( GImage1to32(base,src));
	  case it_index:
return( GImage8to32(base,src));
	  case it_true:
return( GImage32to32(base,src));
	  default:
	    GDrawIError("Bad image type %d", base->image_type);
return( NULL );
	}
      default:
	GDrawIError("Bad image type %d", it);
return( NULL );
    }
}

GImage *GImageGetSub(GImage *image,enum image_type it, GRect *src, GClut *nclut, RevCMap *rev) {

    if ( image->list_len ) {
	GDrawIError( "Attempt to get a subimage from an image list" );
return( NULL );
    }
return( GImageBaseGetSub( image->u.image, it, src, nclut, rev));
}

static void GImageInsert1to1(GImage *from,struct _GImage *tobase, GRect *src,
	int to_x, int to_y, enum pastetrans_type ptt) {
    struct _GImage *fbase = from->u.image;
    uint8 *pt;
    uint8 *ipt;
    uint32 from_trans=COLOR_UNKNOWN, to_trans;
    uint8 ones, zeros;
    int i,j, bit, ibit, index;

    to_trans = tobase->trans;
    if ( to_trans!=COLOR_UNKNOWN || ptt==ptt_old_shines_through )
	from_trans = fbase->trans;

    {
	register Color *clut; Color fake[2];
	int bright, dark, bright_col, dark_col;
	clut = fbase->clut->clut;
	if ( clut==NULL ) {
	    fake[0] = COLOR_CREATE(0,0,0);
	    fake[1] = COLOR_CREATE(0xff,0xff,0xff);
	    clut = fake;
	} 

	bright = MonoCols(tobase->clut,&dark,&bright_col,&dark_col);
	if ( COLOR_RED(clut[0])+COLOR_GREEN(clut[0])+COLOR_BLUE(clut[0]) >
		COLOR_RED(clut[1])+COLOR_GREEN(clut[1])+COLOR_BLUE(clut[1]) ) {
	    zeros = bright;
	    ones = dark;
	} else {
	    zeros = dark;
	    ones = bright;
	}
    }

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (fbase->data) + i*fbase->bytes_per_line + (src->x>>3);
	ipt = (uint8 *) (tobase->data) + (i-src->y+to_y)*tobase->bytes_per_line+(to_x>>3);
	bit = 0x80>>(src->x&7);
	ibit = 0x80>>(to_x&7);
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt&bit;
	    if (( index && from_trans ) || (!index && !from_trans) ) {
		if ( ptt== ptt_old_shines_through )
		    /* Do Nothing */;
		else if ( to_trans ) *ipt |= ibit; else *ipt &= ~ibit;
	    } else if ( (index && ones) || (!index && zeros) ) {
		*ipt |= ibit;
	    } else {
		*ipt &= ~ibit;
	    }
	    if (( ibit>>=1 )==0 ) {ibit=0x80; ++ipt;}
	    if (( bit>>=1 )==0 ) {bit=0x80; ++pt;}
	}
    }
}

static void GImageInsert8to8(GImage *from,struct _GImage *tobase, GRect *src, RevCMap *rev,
	int to_x, int to_y, enum pastetrans_type ptt) {
    struct _GImage *fbase = from->u.image;
    register Color *clut = from->u.image->clut->clut;
    uint8 *pt;
    uint8 *ipt;
    uint32 from_trans=COLOR_UNKNOWN, to_trans;
    struct gcol gclut[256];
    short *red_dith, *green_dith, *blue_dith;
    short *r_d, *g_d, *b_d;
    register int rd, gd, bd;
    int i,j,index;
    const struct gcol *pos;
    Color col;

    to_trans = tobase->trans;
    if ( to_trans!=COLOR_UNKNOWN || ptt==ptt_old_shines_through )
	from_trans = fbase->trans;

    _GDraw_getimageclut(tobase,gclut);
    red_dith = calloc(src->width,sizeof(short));
    green_dith = calloc(src->width,sizeof(short));
    blue_dith = calloc(src->width,sizeof(short));
    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint8 *) (fbase->data + i*fbase->bytes_per_line) + src->x;
	ipt = (uint8 *) (tobase->data) + (i-src->y+to_y)*tobase->bytes_per_line+to_x;
	rd = gd = bd = 0;
	r_d = red_dith; g_d = green_dith; b_d = blue_dith;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==from_trans ) {
		if ( ptt==ptt_old_shines_through )
		    ++ipt;
		else
		    *ipt++ = to_trans;
		++r_d; ++g_d; ++b_d;
	    } else {
		col = clut[index];
		rd += *r_d + COLOR_RED(col); if ( rd<0 ) rd=0; else if ( rd>255 ) rd = 255;
		gd += *g_d + COLOR_GREEN(col); if ( gd<0 ) gd=0; else if ( gd>255 ) gd = 255;
		bd += *b_d + COLOR_BLUE(col); if ( bd<0 ) bd=0; else if ( bd>255 ) bd = 255;
		pos = _GImage_GetIndexedPixelPrecise(COLOR_CREATE(rd,gd,bd),rev);
		*ipt++ = pos->pixel;
		*r_d++ = rd = (rd - pos->red)/2;
		*g_d++ = gd = (gd - pos->green)/2;
		*b_d++ = bd = (bd - pos->blue)/2;
	    }
	}
    }
    free(red_dith); free(green_dith); free(blue_dith);
}

static void GImageInsert32to32(GImage *from,struct _GImage *tobase, GRect *src,
	int to_x, int to_y, enum pastetrans_type ptt) {
    struct _GImage *fbase = from->u.image;
    uint32 *pt;
    uint32 *ipt;
    uint32 from_trans=COLOR_UNKNOWN, to_trans;
    int i,j,index;

    to_trans = tobase->trans;
    if ( to_trans!=COLOR_UNKNOWN || ptt==ptt_old_shines_through )
	from_trans = fbase->trans;

    for ( i=src->y; i<src->y+src->height; ++i ) {
	pt = (uint32 *) (fbase->data + i*fbase->bytes_per_line) + src->x;
	ipt = (uint32 *) (tobase->data + (i-src->y+to_y)*tobase->bytes_per_line)+to_x;
	for ( j=src->width-1; j>=0; --j ) {
	    index = *pt++;
	    if ( index==from_trans ) {
		if ( ptt==ptt_old_shines_through )
		    ++ipt;
		else
		    *ipt++ = to_trans;
	    } else {
		*ipt++ = index;
	    }
	}
    }
}

/* When pasting a transparent color, this routine pastes it as transparent. */
/*  It does not allow the old color to shine through! */
int GImageInsertToBase(struct _GImage *tobase, GImage *from, GRect *src, RevCMap *rev,
	int to_x, int to_y, enum pastetrans_type ptt ) {

    if ( from->list_len ) {
	GDrawIError( "Attempt to paste from an image list" );
return( false );
    }
    if ( src->width<=0 || src->height<=0 || src->x<0 || src->y<0 ) {
	GDrawIError("Invalid rectangle in GImageInsert");
return( false );
    }
    if ( src->x+src->width > from->u.image->width || src->y+src->height > from->u.image->height ||
	    to_x+src->width > tobase->width ||
	    to_y+src->height > tobase->height ||
	    to_x<0 || to_y<0 ) {
	GDrawIError("Bad size to GImageInsert");
return( false );
    }
    if ( from->u.image->image_type != tobase->image_type ) {
	GDrawIError("Image type mismatch in GImageInsert");
return( false );
    }

    if ( (from->u.image->trans==COLOR_UNKNOWN || tobase->trans==COLOR_UNKNOWN ) ||
		    (from->u.image->trans==tobase->trans && ptt==ptt_paste_trans_to_trans)) {
	int i, size=4;
	if ( tobase->image_type == it_index ) size = 1;
	for ( i=src->y; i<src->y+src->height; ++i ) {
	    uint8 *fpt = (uint8 *) (from->u.image + i*from->u.image->bytes_per_line + size*src->x);
	    uint8 *tpt = (uint8 *) (from->u.image + (i-src->y+to_y)*from->u.image->bytes_per_line + size*to_x);
	    memcpy(tpt,fpt,(src->width)*size);
	}
    } else if ( tobase->image_type==it_mono && (src->x&7)==(to_x&7) &&
	    GImageSameClut(from->u.image->clut,tobase->clut) &&
		from->u.image->trans==COLOR_UNKNOWN ) {
	int i, mask1, mask2, size;
	int end = src->x+src->width-1;
	mask1 = -1<<(src->x&7);
	mask2 = ~(-1<<(end&7));
	size = (end>>3) - (src->x>>3)-2;
	if ( mask1==-1 && size!=-2 ) { ++size; }
	if ( mask2==255 && size!=-2 ) { ++size; }
	for ( i=src->y; i<src->y+src->height; ++i ) {
	    uint8 *fpt = (uint8 *) (from->u.image->data + i*from->u.image->bytes_per_line + (src->x>>3));
	    uint8 *tpt = (uint8 *) (tobase->data + (i-src->y+to_y)*tobase->bytes_per_line + (to_x>>3));
	    if ( size==-2 ) {
		/* all bits in same byte */
		*tpt = (*fpt&mask1&mask2) | (*tpt&~(mask1&mask2));
	    } else {
		if ( mask1!=-1 ) {
		    *tpt = (*fpt++&mask1) | (*tpt&~mask1);
		    ++tpt;
		}
		if ( size!=0 )
		    memcpy(tpt,fpt,size);
		if ( mask2!=255 )
		    tpt[size] |= fpt[size]&mask1;
	    }
	}
    } else if ( tobase->image_type==it_mono ) {
	GImageInsert1to1(from, tobase, src, to_x, to_y, ptt);
    } else if ( tobase->image_type==it_true ) {
	GImageInsert32to32(from, tobase, src, to_x, to_y, ptt);
    } else /*if ( tobase->image_type==it_index )*/ {
	RevCMap *oldrev = rev;
	if ( rev==NULL )
	    rev = GClutReverse(tobase->clut,8);
	GImageInsert8to8(from, tobase, src, rev, to_x, to_y, ptt);
	if ( oldrev==NULL )
	    GClut_RevCMapFree(rev);
    }
return( true );
}

int GImageInsert(GImage *to, GImage *from, GRect *src, RevCMap *rev,
	int to_x, int to_y, enum pastetrans_type ptt ) {

    if ( to->list_len ) {
	GDrawIError( "Attempt to paste to an image list" );
return( false );
    }
    GImageInsertToBase(to->u.image,from,src,rev,to_x,to_y,ptt);
return( true );
}

struct bounds {
    int start, end;
    float start_frac, end_frac;
    /* if start==end then start_frac and end_frac should both be start_frac+end_frac-1 */
};

static struct bounds *FillBounds(int src_start, int src_end, int dest_start, int dest_end) {
    int i;
    struct bounds *bounds = malloc((dest_end-dest_start)*sizeof(struct bounds));
    struct bounds *bpt = bounds;
    double frac = (src_end-src_start)/(dest_end-dest_start);
    double temp;

    for ( i=dest_start; i<dest_end; ++i, ++bpt ) {
	temp = (i-dest_start)*frac + src_start;
	bpt->start = (int) temp;
	bpt->start_frac = temp -bpt->start;
	temp = (i+1-dest_start)*frac + src_start;
	bpt->end = (int) temp;
	bpt->end_frac = temp -bpt->start;
	if ( bpt->end==bpt->start )
	    bpt->start = bpt->end -= bpt->start;
	else
	    bpt->start = 1-bpt->start;
    }
return( bounds );
}

/* Calculate the pixel value for the thing at (x,y) (in the generated coordinate */
/*  system */
extern Color _GImageGetPixelColor(struct _GImage *base,int x, int y);

static Color CalculatePixel(struct _GImage *base, int x, int y, struct bounds *xb,
	struct bounds *yb, int do_trans) {
    float red=0, green=0, blue=0, tot=0, trans_tot=0, factx, facty;
    Color pix;
    int i,j;

    for ( i=xb->start; i<=xb->end; ++i ) {
	if ( i==xb->start ) factx = xb->start_frac;
	else if ( i==xb->end ) factx = xb->end_frac;
	else factx = 1;
	for ( j = yb->start; j<=yb->end; ++j ) {
	    if ( j==yb->start ) facty = yb->start_frac;
	    else if ( j==yb->end ) facty = yb->end_frac;
	    else facty = 1;
	    facty *= factx;
	    pix = _GImageGetPixelColor(base,i,j);
	    if ( pix<0 && !do_trans ) pix = ~pix;
	    if ( pix<0 )
		trans_tot += facty;
	    else {
		red += facty*COLOR_RED(pix);
		green += facty*COLOR_GREEN(pix);
		blue += facty*COLOR_BLUE(pix);
		tot += facty;
	    }
	}
    }
    if ( trans_tot>tot )
return( COLOR_UNKNOWN );
    if ( tot==0 )
return( COLOR_CREATE(0,0,0) );
    red /= tot; green /= tot; blue /= tot;
return( COLOR_CREATE( (int) (red+.5), (int) (green+.5), (int) (blue+.5) ));
}

void GImageResize(struct _GImage *tobase, struct _GImage *fbase,
	GRect *src, RevCMap *rev) {
    struct bounds *vert, *hor;
    int i,j;
    uint32 *pt32=NULL;
    uint8 *pt8=NULL;
    int bit=0;
    struct gcol gclut[256];
    short *red_dith=NULL, *green_dith=NULL, *blue_dith=NULL;
    short *r_d=NULL, *g_d=NULL, *b_d=NULL;
    register int rd=0, gd=0, bd=0;
    Color pix;
    RevCMap *oldrev=rev;
    int bright=0, dark, bright_col, dark_col;
    Color trans=COLOR_UNKNOWN;
    int do_trans;
    const struct gcol *pos;

    if ( fbase->trans!=COLOR_UNKNOWN ) {
	trans = tobase->trans;
    }
    do_trans = false;
    if ( trans!=COLOR_UNKNOWN )
	do_trans = true;

    vert = FillBounds(src->y,src->y+src->height,0,tobase->height);
    hor  = FillBounds(src->x,src->x+src->width,0,tobase->width );

    if ( tobase->image_type==it_index ) {
	_GDraw_getimageclut(tobase,gclut);
	red_dith = calloc(src->width,sizeof(short));
	green_dith = calloc(src->width,sizeof(short));
	blue_dith = calloc(src->width,sizeof(short));
	if ( rev==NULL )
	    rev = GClutReverse(tobase->clut,8);
    } else if ( tobase->image_type==it_mono ) {
	green_dith = calloc(src->width,sizeof(short));
	bright = MonoCols(tobase->clut,&dark,&bright_col,&dark_col);
    }

    for ( i=0; i<tobase->height; ++i ) {
	switch ( tobase->image_type ) {
	  case it_true:
	    pt32 = (uint32 *) (tobase->data + i*tobase->bytes_per_line);
	  break;
	  case it_index:
	    pt8 = (uint8 *) (tobase->data + i*tobase->bytes_per_line);
	    rd = gd = bd = 0;
	    r_d = red_dith; g_d = green_dith; b_d = blue_dith;
	  break;
	  case it_mono:
	    pt8 = (uint8 *) (tobase->data + i*tobase->bytes_per_line);
	    bit = 0x80;
	    gd = 0;
	    g_d = green_dith;
	  break;
	}

	for ( j=0; j<tobase->width; ++j ) {
	    pix = CalculatePixel(fbase,j,i,hor,vert,do_trans);
	    switch ( tobase->image_type ) {
	      case it_true:
		if ( pix<0 )
		    *pt32++ = trans;
		else
		    *pt32++ = pix;
	      break;
	      case it_index:
		if ( pix<0 ) {
		    *pt8++ = trans;
		    ++r_d; ++g_d; ++b_d;
		} else {
		    rd += *r_d + COLOR_RED(pix); if ( rd<0 ) rd=0; else if ( rd>255 ) rd = 255;
		    gd += *g_d + COLOR_GREEN(pix); if ( gd<0 ) gd=0; else if ( gd>255 ) gd = 255;
		    bd += *b_d + COLOR_BLUE(pix); if ( bd<0 ) bd=0; else if ( bd>255 ) bd = 255;
		    pos = _GImage_GetIndexedPixelPrecise(COLOR_CREATE(rd,gd,bd),rev);
		    *pt8++ = pos->pixel;
		    *r_d++ = rd = (rd - pos->red)/2;
		    *g_d++ = gd = (gd - pos->green)/2;
		    *b_d++ = bd = (bd - pos->blue)/2;
		}
	      break;
	      case it_mono:
		if ( pix<0 ) {
		    if ( trans ) *pt8 |= bit; else *pt8 &= ~bit;
		    ++g_d;
		} else {
		    gd += *g_d + COLOR_RED(pix) + COLOR_GREEN(pix) + COLOR_BLUE(pix);
		    if (( gd<3*128 && dark) || (gd>=3*128 && bright))
			*pt8 |= bit;
		    else
			*pt8 &= ~bit;
		    if ( gd<0 ) gd=0; else if ( gd>3*255 ) gd = 3*255;
		    if ( gd<3*128 )
			*g_d++ = gd = (gd-dark_col)/2;
		    else
			*g_d++ = gd = (gd-bright_col)/2;
		}
		if (( bit>>=1 )==0 ) {bit=0x80; ++pt8;}
	      break;
	    }
    }}
    free(vert);
    free(hor);
    free(red_dith);
    free(green_dith);
    free(blue_dith);
    if ( oldrev!=rev )
	    GClut_RevCMapFree(rev);
}

GImage *GImageResize32(GImage *from, GRect *src, int width, int height, Color trans) {
    GImage *to;

    if ( from->list_len ) {
	GDrawIError( "Attempt to resize an image list" );
return( NULL );
    }
    to = GImageCreate(it_true, width, height);
    to->u.image->trans = trans;
    GImageResize(to->u.image,from->u.image,src,NULL);
return( to );
}

GImage *GImageResizeSame(GImage *from, GRect *src, int width, int height, RevCMap *rev) {
    GImage *to;
    if ( from->list_len ) {
	GDrawIError( "Attempt to resize an image list" );
return( NULL );
    }
    to = GImageCreate(from->u.image->image_type, width, height);
    to->u.image->trans = from->u.image->trans;
    GImageResize(to->u.image,from->u.image,src,rev);
return( to );
}
