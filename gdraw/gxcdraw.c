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

#include "fontforge-config.h"
#ifdef FONTFORGE_CAN_USE_GDK

void GDrawEnableCairo(int on) {
    /* With GDK, Cairo is always enabled. */
}

#else // FONTFORGE_CAN_USE_GDK

#include "gxdrawP.h"
#include "gxcdrawP.h"

#include <stdlib.h>
#include <math.h>

#include <ustring.h>
#include <utype.h>
#include "fontP.h"

#ifdef __Mac
# include <sys/utsname.h>
#endif

#ifdef _NO_LIBCAIRO
static int usecairo = false;
#else
static int usecairo = true;
#endif



void GDrawEnableCairo(int on) {
    usecairo=on;
    /* Obviously, if we have no library, enabling it will do nothing */
}

#ifndef _NO_LIBCAIRO
/* ************************************************************************** */
/* ***************************** Cairo Library ****************************** */
/* ************************************************************************** */

int _GXCDraw_hasCairo(void) {
    return ( usecairo );
}

/* ************************************************************************** */
/* ****************************** Cairo Window ****************************** */
/* ************************************************************************** */
void _GXCDraw_NewWindow(GXWindow nw) {
    GXDisplay *gdisp = nw->display;
    Display *display = gdisp->display;

    if ( !usecairo || !_GXCDraw_hasCairo())
return;

    nw->cs = cairo_xlib_surface_create(display,nw->w,gdisp->visual,
	    nw->pos.width, nw->pos.height );
    if ( nw->cs!=NULL ) {
	nw->cc = cairo_create(nw->cs);
	if ( nw->cc!=NULL )
	    nw->usecairo = true;
	else {
	    cairo_surface_destroy(nw->cs);
	    nw->cs=NULL;
	}
    }
}

void _GXCDraw_ResizeWindow(GXWindow gw,GRect *rect) {
    cairo_xlib_surface_set_size( gw->cs, rect->width,rect->height);
}

void _GXCDraw_DestroyWindow(GXWindow gw) {
    cairo_destroy(gw->cc);
    cairo_surface_destroy(gw->cs);
    gw->usecairo = false;
}

/* ************************************************************************** */
/* ******************************* Cairo State ****************************** */
/* ************************************************************************** */
static void GXCDraw_StippleMePink(GXWindow gw,int ts, Color fg) {
    static unsigned char grey_init[8] = { 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa };
    static unsigned char fence_init[8] = { 0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55, 0x88};
    uint8 *spt;
    int bit,i,j;
    uint32 *data;
    static uint32 space[8*8];
    static cairo_surface_t *is = NULL;
    static cairo_pattern_t *pat = NULL;

    if ( (fg>>24)!=0xff ) {
	int alpha = fg>>24, r = COLOR_RED(fg), g=COLOR_GREEN(fg), b=COLOR_BLUE(fg);
	r = (alpha*r+128)/255; g = (alpha*g+128)/255; b=(alpha*b+128)/255;
	fg = (alpha<<24) | (r<<16) | (g<<8) | b;
    }

    spt = ts==2 ? fence_init : grey_init;
    for ( i=0; i<8; ++i ) {
	data = space+8*i;
	for ( j=0, bit=0x80; bit!=0; ++j, bit>>=1 ) {
	    if ( spt[i]&bit )
		data[j] = fg;
	    else
		data[j] = 0;
	}
    }
    if ( is==NULL ) {
	is = cairo_image_surface_create_for_data((uint8 *) space,CAIRO_FORMAT_ARGB32,
		8,8,8*4);
	pat = cairo_pattern_create_for_surface(is);
	cairo_pattern_set_extend(pat,CAIRO_EXTEND_REPEAT);
    }
    cairo_set_source(gw->cc,pat);
}

static int GXCDrawSetcolfunc(GXWindow gw, GGC *mine) {
    /*GCState *gcs = &gw->cairo_state;*/
    Color fg = mine->fg;

    if ( (fg>>24 ) == 0 )
	fg |= 0xff000000;

    if ( mine->ts != 0 ) {
	GXCDraw_StippleMePink(gw,mine->ts,fg);
    } else {
	cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
		(fg>>24)/255.);
    }
return( true );
}

static int GXCDrawSetline(GXWindow gw, GGC *mine) {
    GCState *gcs = &gw->cairo_state;
    Color fg = mine->fg;

    if ( ( fg>>24 ) == 0 )
	fg |= 0xff000000;

    if ( mine->line_width<=0 ) mine->line_width = 1;
    if ( mine->line_width!=gcs->line_width || mine->line_width!=2 ) {
	cairo_set_line_width(gw->cc,mine->line_width);
	gcs->line_width = mine->line_width;
    }
    if ( mine->dash_len != gcs->dash_len || mine->skip_len != gcs->skip_len ||
	    mine->dash_offset != gcs->dash_offset ) {
	double dashes[2];
	dashes[0] = mine->dash_len; dashes[1] = mine->skip_len;
	cairo_set_dash(gw->cc,dashes,0,mine->dash_offset);
	gcs->dash_offset = mine->dash_offset;
	gcs->dash_len = mine->dash_len;
	gcs->skip_len = mine->skip_len;
    }
    /* I don't use line join/cap. On a screen with small line_width they are irrelevant */

    if ( mine->ts != 0 ) {
	GXCDraw_StippleMePink(gw,mine->ts,fg);
    } else {
	cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
		    (fg>>24)/255.0);
    }
return( mine->line_width );
}

void _GXCDraw_PushClip(GXWindow gw) {
    cairo_save(gw->cc);
    cairo_new_path(gw->cc);
    cairo_rectangle(gw->cc,gw->ggc->clip.x,gw->ggc->clip.y,gw->ggc->clip.width,gw->ggc->clip.height);
    cairo_clip(gw->cc);
}

void _GXCDraw_PopClip(GXWindow gw) {
    cairo_restore(gw->cc);
}

void _GXCDraw_PushClipOnly(GXWindow gw) {
    cairo_save( gw->cc );
}

void _GXCDraw_ClipPreserve(GXWindow gw) {
    cairo_clip_preserve( gw->cc );
}

/**
 *  \brief Sets Cairo to use the difference operator with antialiasing disabled.
 *  To ensure that difference mode only applies temporarily, ensure that
 *  PushClip or PushClipOnly has been called before this function. Call PopClip
 *  to return to normal mode. This mode is used to perform cursor blinking
 *  (a minimal replacement for XOR mode).
 *
 *  \param [in] gw The window to set apply the difference operator to.
 */
void _GXCDraw_SetDifferenceMode(GXWindow gw) {
    cairo_set_operator(gw->cc, CAIRO_OPERATOR_DIFFERENCE);
    cairo_set_antialias(gw->cc, CAIRO_ANTIALIAS_NONE);
}

/* ************************************************************************** */
/* ***************************** Cairo Drawing ****************************** */
/* ************************************************************************** */
void _GXCDraw_Clear(GXWindow gw, GRect *rect) {
    GRect *r = rect, temp;
    if ( r==NULL ) {
	temp = gw->pos;
	temp.x = temp.y = 0;
	r = &temp;
    }
    cairo_new_path(gw->cc);
    cairo_rectangle(gw->cc,r->x,r->y,r->width,r->height);
    cairo_set_source_rgba(gw->cc,COLOR_RED(gw->ggc->bg)/255.0,COLOR_GREEN(gw->ggc->bg)/255.0,COLOR_BLUE(gw->ggc->bg)/255.0,
	    1.0);
    cairo_fill(gw->cc);
}

void _GXCDraw_DrawLine(GXWindow gw, int32 x,int32 y, int32 xend,int32 yend) {
    int width = GXCDrawSetline(gw,gw->ggc);

    cairo_new_path(gw->cc);
    if ( width&1 ) {
	cairo_move_to(gw->cc,x+.5,y+.5);
	cairo_line_to(gw->cc,xend+.5,yend+.5);
    } else {
	cairo_move_to(gw->cc,x,y);
	cairo_line_to(gw->cc,xend,yend);
    }
    cairo_stroke(gw->cc);
}

/**
 *  \brief Draws an arc (circular and elliptical).
 *
 *  \param [in] gw The window to draw on.
 *  \param [in] rect The bounding box of the arc. If width!=height, then
 *                   an elliptical arc will be drawn.
 *  \param [in] start_angle The start angle in radians (Cairo coordinates)
 *  \param [in] end_angle The end angle in radians (Cairo coordinates)
 */
void _GXCDraw_DrawArc(GXWindow gw, GRect *rect, double start_angle, double end_angle) {
    int width = GXCDrawSetline(gw, gw->ggc);

    cairo_new_path(gw->cc);
    cairo_save(gw->cc);
    if (width&1) {
        cairo_translate(gw->cc, rect->x+.5 + rect->width / 2., rect->y+.5 + rect->height / 2.);
    } else {
        cairo_translate(gw->cc, rect->x + rect->width / 2., rect->y + rect->height / 2.);
    }
    cairo_scale(gw->cc, rect->width / 2., rect->height / 2.);
    cairo_arc(gw->cc, 0., 0., 1., start_angle, end_angle);
    cairo_restore(gw->cc);
    cairo_stroke(gw->cc);
}

void _GXCDraw_DrawRect(GXWindow gw, GRect *rect) {
    int width = GXCDrawSetline(gw,gw->ggc);

    cairo_new_path(gw->cc);
    if ( width&1 ) {
	cairo_rectangle(gw->cc,rect->x+.5,rect->y+.5,rect->width,rect->height);
    } else {
	cairo_rectangle(gw->cc,rect->x,rect->y,rect->width,rect->height);
    }
    cairo_stroke(gw->cc);
}

void _GXCDraw_FillRect(GXWindow gw, GRect *rect) {
    GXCDrawSetcolfunc(gw,gw->ggc);

    cairo_new_path(gw->cc);
    cairo_rectangle(gw->cc,rect->x,rect->y,rect->width,rect->height);
    cairo_fill(gw->cc);
}

void _GXCDraw_FillRoundRect(GXWindow gw, GRect *rect, int radius) {
    double degrees = M_PI / 180.0;

    GXCDrawSetcolfunc(gw,gw->ggc);

    cairo_new_path(gw->cc);
    cairo_arc(gw->cc, rect->x + rect->width - radius, rect->y + radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(gw->cc, rect->x + rect->width - radius, rect->y + rect->height - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(gw->cc, rect->x + radius, rect->y + rect->height - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(gw->cc, rect->x + radius, rect->y + radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(gw->cc);
    cairo_fill(gw->cc);

}

static void GXCDraw_EllipsePath(cairo_t *cc,double cx,double cy,double width,double height) {
    cairo_new_path(cc);
    cairo_move_to(cc,cx,cy+height);
    cairo_curve_to(cc,
	    cx+.552*width,cy+height,
	    cx+width,cy+.552*height,
	    cx+width,cy);
    cairo_curve_to(cc,
	    cx+width,cy-.552*height,
	    cx+.552*width,cy-height,
	    cx,cy-height);
    cairo_curve_to(cc,
	    cx-.552*width,cy-height,
	    cx-width,cy-.552*height,
	    cx-width,cy);
    cairo_curve_to(cc,
	    cx-width,cy+.552*height,
	    cx-.552*width,cy+height,
	    cx,cy+height);
    cairo_close_path(cc);
}

void _GXCDraw_DrawEllipse(GXWindow gw, GRect *rect) {
    /* It is tempting to use the cairo arc command and scale the */
    /*  coordinates to get an elipse, but that distorts the stroke width */
    int lwidth = GXCDrawSetline(gw,gw->ggc);
    double cx, cy, width, height;

    width = rect->width/2.0; height = rect->height/2.0;
    cx = rect->x + width;
    cy = rect->y + height;
    if ( lwidth&1 ) {
	if ( rint(width)==width )
	    cx += .5;
	if ( rint(height)==height )
	    cy += .5;
    }
    GXCDraw_EllipsePath(gw->cc,cx,cy,width,height);
    cairo_stroke(gw->cc);
}

void _GXCDraw_FillEllipse(GXWindow gw, GRect *rect) {
    /* It is tempting to use the cairo arc command and scale the */
    /*  coordinates to get an elipse, but that distorts the stroke width */
    double cx, cy, width, height;

    GXCDrawSetcolfunc(gw,gw->ggc);

    width = rect->width/2.0; height = rect->height/2.0;
    cx = rect->x + width;
    cy = rect->y + height;
    GXCDraw_EllipsePath(gw->cc,cx,cy,width,height);
    cairo_fill(gw->cc);
}

void _GXCDraw_DrawPoly(GXWindow gw, GPoint *pts, int16 cnt) {
    int width = GXCDrawSetline(gw,gw->ggc);
    double off = width&1 ? .5 : 0;
    int i;

    cairo_new_path(gw->cc);
    cairo_move_to(gw->cc,pts[0].x+off,pts[0].y+off);
    for ( i=1; i<cnt; ++i )
	cairo_line_to(gw->cc,pts[i].x+off,pts[i].y+off);
    cairo_stroke(gw->cc);
}

void _GXCDraw_FillPoly(GXWindow gw, GPoint *pts, int16 cnt) {
    GXCDrawSetcolfunc(gw,gw->ggc);
    int i;

    cairo_new_path(gw->cc);
    cairo_move_to(gw->cc,pts[0].x,pts[0].y);
    for ( i=1; i<cnt; ++i )
	cairo_line_to(gw->cc,pts[i].x,pts[i].y);
    cairo_close_path(gw->cc);
    cairo_fill(gw->cc);

    cairo_set_line_width(gw->cc,1);
    cairo_new_path(gw->cc);
    cairo_move_to(gw->cc,pts[0].x+.5,pts[0].y+.5);
    for ( i=1; i<cnt; ++i )
	cairo_line_to(gw->cc,pts[i].x+.5,pts[i].y+.5);
    cairo_close_path(gw->cc);
    cairo_stroke(gw->cc);
}

/* ************************************************************************** */
/* ****************************** Cairo Paths ******************************* */
/* ************************************************************************** */
void _GXCDraw_PathStartNew(GWindow w) {
    cairo_new_path( ((GXWindow) w)->cc );
}

void _GXCDraw_PathStartSubNew(GWindow w) {
    cairo_new_sub_path( ((GXWindow) w)->cc );
}

int _GXCDraw_FillRuleSetWinding(GWindow w) {
    cairo_set_fill_rule(((GXWindow) w)->cc,CAIRO_FILL_RULE_WINDING);
    return 1;
}

void _GXCDraw_PathClose(GWindow w) {
    cairo_close_path( ((GXWindow) w)->cc );
}

void _GXCDraw_PathMoveTo(GWindow w,double x, double y) {
    cairo_move_to( ((GXWindow) w)->cc,x,y );
}

void _GXCDraw_PathLineTo(GWindow w,double x, double y) {
    cairo_line_to( ((GXWindow) w)->cc,x,y );
}

void _GXCDraw_PathCurveTo(GWindow w,
		    double cx1, double cy1,
		    double cx2, double cy2,
		    double x, double y) {
    cairo_curve_to( ((GXWindow) w)->cc,cx1,cy1,cx2,cy2,x,y );
}

void _GXCDraw_PathStroke(GWindow w,Color col) {
    w->ggc->fg = col;
    GXCDrawSetline((GXWindow) w,w->ggc);
    cairo_stroke( ((GXWindow) w)->cc );
}

void _GXCDraw_PathFill(GWindow w,Color col) {
    cairo_set_source_rgba(((GXWindow) w)->cc,COLOR_RED(col)/255.0,COLOR_GREEN(col)/255.0,COLOR_BLUE(col)/255.0,
	    (col>>24)/255.0);
    cairo_fill( ((GXWindow) w)->cc );
}

void _GXCDraw_PathFillAndStroke(GWindow w,Color fillcol, Color strokecol) {
    GXWindow gw = (GXWindow) w;

    cairo_save(gw->cc);
    cairo_set_source_rgba(gw->cc,COLOR_RED(fillcol)/255.0,COLOR_GREEN(fillcol)/255.0,COLOR_BLUE(fillcol)/255.0,
	    (fillcol>>24)/255.0);
    cairo_fill( gw->cc );
    cairo_restore(gw->cc);
    w->ggc->fg = strokecol;
    GXCDrawSetline(gw,gw->ggc);
    cairo_fill( gw->cc );
}

/* ************************************************************************** */
/* ****************************** Cairo Images ****************************** */
/* ************************************************************************** */
static cairo_surface_t *GImage2Surface(GImage *image, GRect *src, uint8 **_data) {
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    cairo_format_t type;
    uint8 *data, *pt;
    uint32 *idata, *ipt, *ito;
    int i,j,jj,tjj,stride;
    int bit, tobit;
    cairo_surface_t *cs;

    if ( base->image_type == it_rgba )
	type = CAIRO_FORMAT_ARGB32;
    else if ( base->image_type == it_true && base->trans!=COLOR_UNKNOWN )
	type = CAIRO_FORMAT_ARGB32;
    else if ( base->image_type == it_index && base->clut->trans_index!=COLOR_UNKNOWN )
	type = CAIRO_FORMAT_ARGB32;
    else if ( base->image_type == it_true )
	type = CAIRO_FORMAT_RGB24;
    else if ( base->image_type == it_index )
	type = CAIRO_FORMAT_RGB24;
    else if ( base->image_type == it_mono && base->clut!=NULL &&
	    base->clut->trans_index!=COLOR_UNKNOWN )
	type = CAIRO_FORMAT_A1;
    else
	type = CAIRO_FORMAT_RGB24;

    /* We can't reuse the image's data for alpha images because we must */
    /*  premultiply each channel by alpha. We can reuse it for non-transparent*/
    /*  rgb images */
    if ( base->image_type == it_true && type == CAIRO_FORMAT_RGB24 ) {
	idata = ((uint32 *) (base->data)) + src->y*base->bytes_per_line + src->x;
	*_data = NULL;		/* We can reuse the image's own data, don't need a copy */
return( cairo_image_surface_create_for_data((uint8 *) idata,type,
		src->width, src->height,
		base->bytes_per_line));
    }

    stride = cairo_format_stride_for_width(type,src->width);
    *_data = data = malloc(stride * src->height);
    cs = cairo_image_surface_create_for_data(data,type,
		src->width, src->height,   stride);
    idata = (uint32 *) data;

    if ( base->image_type == it_rgba ) {
	ipt = ((uint32 *) (base->data + src->y*base->bytes_per_line)) + src->x;
	ito = idata;
	for ( i=0; i<src->height; ++i ) {
	   for ( j=0; j<src->width; ++j ) {
	       uint32 orig = ipt[j];
	       int alpha = orig>>24;
	       if ( alpha==0xff )
		   ito[j] = orig;
	       else if ( alpha==0 )
		   ito[j] = 0x00000000;
	       else
		   ito[j] = (alpha<<24) |
			   ((COLOR_RED  (orig)*alpha/255)<<16)|
			   ((COLOR_GREEN(orig)*alpha/255)<<8 )|
			   ((COLOR_BLUE (orig)*alpha/255));
	   }
	   ipt = (uint32 *) (((uint8 *) ipt) + base->bytes_per_line);
	   ito = (uint32 *) (((uint8 *) ito) +stride);
       }
    } else if ( base->image_type == it_true && base->trans!=COLOR_UNKNOWN ) {
	Color trans = base->trans;
	ipt = ((uint32 *) (base->data + src->y*base->bytes_per_line)) + src->x;
	ito = idata;
	for ( i=0; i<src->height; ++i ) {
	   for ( j=0; j<src->width; ++j ) {
	       if ( ipt[j]==trans )
		   ito[j] = 0x00000000;
	       else
		   ito[j] = ipt[j]|0xff000000;
	   }
	   ipt = (uint32 *) (((uint8 *) ipt) + base->bytes_per_line);
	   ito = (uint32 *) (((uint8 *) ito) +stride);
       }
    } else if ( base->image_type == it_true ) {
	ipt = ((uint32 *) (base->data + src->y*base->bytes_per_line)) + src->x;
	ito = idata;
	for ( i=0; i<src->height; ++i ) {
	   for ( j=0; j<src->width; ++j ) {
	       ito[j] = ipt[j]|0xff000000;
	   }
	   ipt = (uint32 *) (((uint8 *) ipt) + base->bytes_per_line);
	   ito = (uint32 *) (((uint8 *) ito) +stride);
       }
    } else if ( base->image_type == it_index && base->clut->trans_index!=COLOR_UNKNOWN ) {
	int trans = base->clut->trans_index;
	Color *clut = base->clut->clut;
	pt = base->data + src->y*base->bytes_per_line + src->x;
	ito = idata;
	for ( i=0; i<src->height; ++i ) {
	   for ( j=0; j<src->width; ++j ) {
	       int index = pt[j];
	       if ( index==trans )
		   ito[j] = 0x00000000;
	       else
		   /* In theory RGB24 images don't need the alpha channel set*/
		   /*  but there is a bug in Cairo 1.2, and they do. */
		   ito[j] = clut[index]|0xff000000;
	   }
	   pt += base->bytes_per_line;
	   ito = (uint32 *) (((uint8 *) ito) +stride);
       }
    } else if ( base->image_type == it_index ) {
	Color *clut = base->clut->clut;
	pt = base->data + src->y*base->bytes_per_line + src->x;
	ito = idata;
	for ( i=0; i<src->height; ++i ) {
	   for ( j=0; j<src->width; ++j ) {
	       int index = pt[j];
	       ito[j] = clut[index] | 0xff000000;
	   }
	   pt += base->bytes_per_line;
	   ito = (uint32 *) (((uint8 *) ito) +stride);
       }
#ifdef WORDS_BIGENDIAN
    } else if ( base->image_type == it_mono && base->clut!=NULL &&
	    base->clut->trans_index!=COLOR_UNKNOWN ) {
	pt = base->data + src->y*base->bytes_per_line + (src->x>>3);
	ito = idata;
	memset(data,0,src->height*stride);
	if ( base->clut->trans_index==0 ) {
	    for ( i=0; i<src->height; ++i ) {
		bit = (0x80>>(src->x&0x7));
		tobit = 0x80000000;
		for ( j=jj=tjj=0; j<src->width; ++j ) {
		    if ( pt[jj]&bit )
			ito[tjj] |= tobit;
		    if ( (bit>>=1)==0 ) {
			bit = 0x80;
			++jj;
		    }
		    if ( (tobit>>=1)==0 ) {
			tobit = 0x80000000;
			++tjj;
		    }
		}
		pt += base->bytes_per_line;
		ito = (uint32 *) (((uint8 *) ito) +stride);
	    }
	} else {
	    for ( i=0; i<src->height; ++i ) {
		bit = (0x80>>(src->x&0x7));
		tobit = 0x80000000;
		for ( j=jj=tjj=0; j<src->width; ++j ) {
		    if ( !(pt[jj]&bit) )
			ito[tjj] |= tobit;
		    if ( (bit>>=1)==0 ) {
			bit = 0x80;
			++jj;
		    }
		    if ( (tobit>>=1)==0 ) {
			tobit = 0x80000000;
			++tjj;
		    }
		}
		pt += base->bytes_per_line;
		ito = (uint32 *) (((uint8 *) ito) +stride);
	    }
	}
#else
    } else if ( base->image_type == it_mono && base->clut!=NULL &&
	    base->clut->trans_index!=COLOR_UNKNOWN ) {
	pt = base->data + src->y*base->bytes_per_line + (src->x>>3);
	ito = idata;
	memset(data,0,src->height*stride);
	if ( base->clut->trans_index==0 ) {
	    for ( i=0; i<src->height; ++i ) {
		bit = (0x80>>(src->x&0x7));
		tobit = 1;
		for ( j=jj=tjj=0; j<src->width; ++j ) {
		    if ( pt[jj]&bit )
			ito[tjj] |= tobit;
		    if ( (bit>>=1)==0 ) {
			bit = 0x80;
			++jj;
		    }
		    if ( (tobit<<=1)==0 ) {
			tobit = 0x1;
			++tjj;
		    }
		}
		pt += base->bytes_per_line;
		ito = (uint32 *) (((uint8 *) ito) +stride);
	    }
	} else {
	    for ( i=0; i<src->height; ++i ) {
		bit = (0x80>>(src->x&0x7));
		tobit = 1;
		for ( j=jj=tjj=0; j<src->width; ++j ) {
		    if ( !(pt[jj]&bit) )
			ito[tjj] |= tobit;
		    if ( (bit>>=1)==0 ) {
			bit = 0x80;
			++jj;
		    }
		    if ( (tobit<<=1)==0 ) {
			tobit = 0x1;
			++tjj;
		    }
		}
		pt += base->bytes_per_line;
		ito = (uint32 *) (((uint8 *) ito) +stride);
	    }
	}
#endif
    } else {
	Color fg = base->clut==NULL ? 0xffffff : base->clut->clut[1];
	Color bg = base->clut==NULL ? 0x000000 : base->clut->clut[0];
       /* In theory RGB24 images don't need the alpha channel set*/
       /*  but there is a bug in Cairo 1.2, and they do. */
	fg |= 0xff000000; bg |= 0xff000000;
	pt = base->data + src->y*base->bytes_per_line + (src->x>>3);
	ito = idata;
	for ( i=0; i<src->height; ++i ) {
	    bit = (0x80>>(src->x&0x7));
	    for ( j=jj=0; j<src->width; ++j ) {
		ito[j] = (pt[jj]&bit) ? fg : bg;
		if ( (bit>>=1)==0 ) {
		    bit = 0x80;
		    ++jj;
		}
	    }
	    pt += base->bytes_per_line;
	    ito = (uint32 *) (((uint8 *) ito) +stride);
	}
    }
return( cs );
}

void _GXCDraw_Image( GXWindow gw, GImage *image, GRect *src, int32 x, int32 y) {
    uint8 *data;
    cairo_surface_t *is = GImage2Surface(image,src,&data);
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];

    if ( cairo_image_surface_get_format(is)==CAIRO_FORMAT_A1 ) {
	/* No color info, just alpha channel */
	Color fg = base->clut->trans_index==0 ? base->clut->clut[1] : base->clut->clut[0];
	cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,1.0);
	cairo_mask_surface(gw->cc,is,x,y);
    } else {
	cairo_set_source_surface(gw->cc,is,x,y);
	cairo_rectangle(gw->cc,x,y,src->width,src->height);
	cairo_fill(gw->cc);
    }
    /* Clear source and mask, in case we need to */
    cairo_new_path(gw->cc);
    cairo_set_source_rgba(gw->cc,0,0,0,0);

    cairo_surface_destroy(is);
    free(data);
    gw->cairo_state.fore_col = COLOR_UNKNOWN;
}

void _GXCDraw_TileImage( GXWindow gw, GImage *image, GRect *src, int32 x, int32 y) {
}

/* What we really want to do is use the grey levels as an alpha channel */
void _GXCDraw_Glyph( GXWindow gw, GImage *image, GRect *src, int32 x, int32 y) {
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    cairo_surface_t *is;

    if ( base->image_type!=it_index )
	_GXCDraw_Image(gw,image,src,x,y);
    else {
	int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8,src->width);
	uint8 *basedata = malloc(stride*src->height),
	       *data = basedata,
		*srcd = base->data + src->y*base->bytes_per_line + src->x;
	int factor = base->clut->clut_len==256 ? 1 :
		     base->clut->clut_len==16 ? 17 :
		     base->clut->clut_len==4 ? 85 : 255;
	int i,j;
	Color fg = base->clut->clut[base->clut->clut_len-1];

	for ( i=0; i<src->height; ++i ) {
	    for ( j=0; j<src->width; ++j )
		data[j] = factor*srcd[j];
	    srcd += base->bytes_per_line;
	    data += stride;
	}
	is = cairo_image_surface_create_for_data(basedata,CAIRO_FORMAT_A8,
		src->width,src->height,stride);
	cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,1.0);
	cairo_mask_surface(gw->cc,is,x,y);
	/* I think the mask is sufficient, setting a rectangle would provide */
	/*  a new mask? */
	/*cairo_rectangle(gw->cc,x,y,src->width,src->height);*/
	/* I think setting the mask also draws... at least so the tutorial implies */
	/* cairo_fill(gw->cc);*/
	/* Presumably that doesn't leave the mask surface pattern lying around */
	/* but dereferences it so we can free it */
	cairo_surface_destroy(is);
	free(basedata);
    }
    gw->cairo_state.fore_col = COLOR_UNKNOWN;
}

void _GXCDraw_ImageMagnified(GXWindow gw, GImage *image, GRect *magsrc,
	int32 x, int32 y, int32 width, int32 height) {
    struct _GImage *base = image->list_len==0?image->u.image:image->u.images[0];
    GRect full;
    double xscale, yscale;
    GRect viewable;

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
  {
    GImage *temp = _GImageExtract(base,&full,&viewable,xscale,yscale);
    GRect src;
    src.x = src.y = 0; src.width = viewable.width; src.height = viewable.height;
    _GXCDraw_Image( gw, temp, &src, x+viewable.x, y+viewable.y);
  }
}

/* ************************************************************************** */
/* ******************************** Copy Area ******************************* */
/* ************************************************************************** */

void _GXCDraw_CopyArea( GXWindow from, GXWindow into, GRect *src, int32 x, int32 y) {

    if ( !into->usecairo || !from->usecairo ) {
	fprintf( stderr, "Cairo CopyArea called from something not cairo enabled\n" );
return;
    }

    int width, height;

    width = cairo_xlib_surface_get_width(into->cs);
    height = cairo_xlib_surface_get_height(into->cs);

    /* make sure the destination surface is big enough for the copied area */
    cairo_xlib_surface_set_size(into->cs, imax(width, src->width), imax(height, src->height));

    cairo_set_source_surface(into->cc,from->cs,x-src->x,y-src->y);
    cairo_rectangle(into->cc,x,y,src->width,src->height);
    cairo_fill(into->cc);

    /* Clear source and mask, in case we need to */
    cairo_set_source_rgba(into->cc,0,0,0,0);

    into->cairo_state.fore_col = COLOR_UNKNOWN;
}

/* ************************************************************************** */
/* **************************** Memory Buffering **************************** */
/* ************************************************************************** */
/* We can't draw with XOR in cairo. We can do all the cairo processing, copy */
/*  cairo's data to the x window, and then do the xor drawing. But if the X */
/*  window isn't available (if we are buffering cairo) then we must save the */
/*  XOR drawing operations until we've popped the buffering */
/* Mmm. Now we use pixmaps rather than groups and the issue isn't relevant -- I think */

enum gcairo_flags _GXCDraw_CairoCapabilities( GXWindow gw) {
    enum gcairo_flags flags = gc_all;

return( flags );
}
/* ************************************************************************** */
/* **************************** Synchronization ***************************** */
/* ************************************************************************** */
void _GXCDraw_Flush(GXWindow gw) {
    cairo_surface_flush(gw->cs);
}

void _GXCDraw_DirtyRect(GXWindow gw,double x, double y, double width, double height) {
    cairo_surface_mark_dirty_rectangle(gw->cs,x,y,width,height);
}
#else
int _GXCDraw_hasCairo(void) {
return(false);
}

#endif	/* ! _NO_LIBCAIRO */

/* ************************************************************************** */
/* ***************************** Pango Library ****************************** */
/* ************************************************************************** */

#  define GTimer GTimer_GTK
#  include <pango/pangoxft.h>
#  if !defined(_NO_LIBCAIRO)
#   include <pango/pangocairo.h>
#  endif
#  undef GTimer

/* ************************************************************************** */
/* ****************************** Pango Render ****************************** */
/* ************************************************************************** */

/* This is not a drop-in replacement for pango_xft_render_layout as as both */
/* expect x and y in different ways */
static void my_xft_render_layout(XftDraw *xftw,XftColor *fgcol,
	PangoLayout *layout,int x,int y) {
    PangoRectangle rect, r2;
    PangoLayoutIter *iter;

    iter = pango_layout_get_iter(layout);
    do {
	PangoLayoutRun *run = pango_layout_iter_get_run(iter);
	if ( run!=NULL ) {	/* NULL runs mark end of line */
	    pango_layout_iter_get_run_extents(iter,&r2,&rect);
	    pango_xft_render(xftw,fgcol,run->item->analysis.font,run->glyphs,
		    x+(rect.x+PANGO_SCALE/2)/PANGO_SCALE, y+(rect.y+PANGO_SCALE/2)/PANGO_SCALE);
	    /* I doubt I'm supposed to free (or unref) the run? */
	}
    } while ( pango_layout_iter_next_run(iter));
    pango_layout_iter_free(iter);
}

# if !defined(_NO_LIBCAIRO)
/* Strangely the equivalent routine was not part of the pangocairo library */
/* Oh there's pango_cairo_layout_path but that's more restrictive and probably*/
/*  less efficient */

static void my_cairo_render_layout(cairo_t *cc, Color fg,
	PangoLayout *layout,int x,int y) {
    PangoRectangle rect, r2;
    PangoLayoutIter *iter;

    iter = pango_layout_get_iter(layout);
    do {
	PangoLayoutRun *run = pango_layout_iter_get_run(iter);
	if ( run!=NULL ) {	/* NULL runs mark end of line */
	    pango_layout_iter_get_run_extents(iter,&r2,&rect);
	    cairo_move_to(cc,x+(rect.x+PANGO_SCALE/2)/PANGO_SCALE, y+(rect.y+PANGO_SCALE/2)/PANGO_SCALE);
	    if ( COLOR_ALPHA(fg)==0 )
		cairo_set_source_rgba(cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
			1.0);
	    else
		cairo_set_source_rgba(cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
			COLOR_ALPHA(fg)/255.);
	    pango_cairo_show_glyph_string(cc,run->item->analysis.font,run->glyphs);
	}
    } while ( pango_layout_iter_next_run(iter));
    pango_layout_iter_free(iter);
}
#endif

/* ************************************************************************** */
/* ****************************** Pango Window ****************************** */
/* ************************************************************************** */
void _GXPDraw_NewWindow(GXWindow nw) {
    GXDisplay *gdisp = nw->display;

# if !defined(_NO_LIBCAIRO)
    if ( nw->usecairo ) {
	/* using pango through cairo is different from using it on bare X */
	if ( gdisp->pangoc_context==NULL ) {
	    gdisp->pangoc_fontmap = pango_cairo_font_map_get_default();
	    gdisp->pangoc_context = pango_font_map_create_context(
		    PANGO_FONT_MAP (gdisp->pangoc_fontmap));
	    pango_cairo_context_set_resolution(gdisp->pangoc_context,
		    gdisp->res);
	}
	if (nw->pango_layout==NULL)
	    nw->pango_layout = pango_layout_new(gdisp->pangoc_context);
    } else
# endif
    {
	if ( gdisp->pango_context==NULL ) {
	    gdisp->pango_fontmap = pango_xft_get_font_map(gdisp->display,gdisp->screen);
	    gdisp->pango_context = pango_font_map_create_context(gdisp->pango_fontmap);
	    /* No obvious way to get or set the resolution of pango_xft */
	}
	nw->xft_w = XftDrawCreate(gdisp->display,nw->w,gdisp->visual,gdisp->cmap);
	if ( nw->pango_layout==NULL )
	    nw->pango_layout = pango_layout_new(gdisp->pango_context);
    }
return;
}

void _GXPDraw_DestroyWindow(GXWindow nw) {
    /* And why doesn't the man page mention this essential function? */
    if ( XftDrawDestroy!=NULL && nw->xft_w!=NULL ) {
	XftDrawDestroy(nw->xft_w);
	nw->xft_w = NULL;
    }
    if ( nw->pango_layout!=NULL )
	g_object_unref(nw->pango_layout);
}

/* ************************************************************************** */
/* ******************************* Pango Text ******************************* */
/* ************************************************************************** */
PangoFontDescription *_GXPDraw_configfont(GWindow w, GFont *font) {
    GXWindow gw = (GXWindow) w;
    PangoFontDescription *fd;

    /* initialize cairo and pango if not initialized, e.g. root window */
    if (gw->pango_layout == NULL){
#ifndef _NO_LIBCAIRO
	_GXCDraw_NewWindow(gw);
#endif
	_GXPDraw_NewWindow(gw);
    }

#ifdef _NO_LIBCAIRO
    PangoFontDescription **fdbase = &font->pango_fd;
#else
    PangoFontDescription **fdbase = gw->usecairo ? &font->pangoc_fd : &font->pango_fd;
#endif

    if ( *fdbase!=NULL )
return( *fdbase );
    *fdbase = fd = pango_font_description_new();

    if ( font->rq.utf8_family_name != NULL )
	pango_font_description_set_family(fd,font->rq.utf8_family_name);
    else {
	char *temp = u2utf8_copy(font->rq.family_name);
	pango_font_description_set_family(fd,temp);
	free(temp);
    }
    pango_font_description_set_style(fd,(font->rq.style&fs_italic)?
	    PANGO_STYLE_ITALIC:
	    PANGO_STYLE_NORMAL);
    pango_font_description_set_variant(fd,(font->rq.style&fs_smallcaps)?
	    PANGO_VARIANT_SMALL_CAPS:
	    PANGO_VARIANT_NORMAL);
    pango_font_description_set_weight(fd,font->rq.weight);
    pango_font_description_set_stretch(fd,
	    (font->rq.style&fs_condensed)?  PANGO_STRETCH_CONDENSED :
	    (font->rq.style&fs_extended )?  PANGO_STRETCH_EXPANDED  :
					    PANGO_STRETCH_NORMAL);

    if (font->rq.style&fs_vertical)
	/* FIXME: not sure this is the right thing */
	pango_font_description_set_gravity(fd, PANGO_GRAVITY_WEST);

    if ( font->rq.point_size<=0 )
	GDrawIError( "Bad point size for pango" );	/* any negative (pixel) values should be converted when font opened */

    /* Pango doesn't give me any control over the resolution on X, so I do my */
    /*  own conversion from points to pixels */
    /* But under pangocairo I can set the resolution, so behavior is different*/
    pango_font_description_set_absolute_size(fd,
		    GDrawPointsToPixels(NULL,font->rq.point_size*PANGO_SCALE));
return( fd );
}

int32 _GXPDraw_DoText8(GWindow w, int32 x, int32 y,
	const char *text, int32 cnt, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    struct font_instance *fi = gw->ggc->fi;
    PangoRectangle rect, ink;
    PangoFontDescription *fd;

    if (fi == NULL)
	return(0);

    fd = _GXPDraw_configfont(w, fi);
    pango_layout_set_font_description(gw->pango_layout,fd);
    pango_layout_set_text(gw->pango_layout,(char *) text,cnt);
    pango_layout_get_pixel_extents(gw->pango_layout,NULL,&rect);
    if ( drawit==tf_drawit ) {
# if !defined(_NO_LIBCAIRO)
	if ( gw->usecairo ) {
	    my_cairo_render_layout(gw->cc,col,gw->pango_layout,x,y);
	} else
#endif
	{
	    XftColor fg;
	    XRenderColor fgcol;
	    XRectangle clip;
	    fgcol.red = COLOR_RED(col)<<8; fgcol.green = COLOR_GREEN(col)<<8; fgcol.blue = COLOR_BLUE(col)<<8;
	    if ( COLOR_ALPHA(col)!=0 )
		fgcol.alpha = COLOR_ALPHA(col)*0x101;
	    else
		fgcol.alpha = 0xffff;
	    XftColorAllocValue(gdisp->display,gdisp->visual,gdisp->cmap,&fgcol,&fg);
	    clip.x = gw->ggc->clip.x;
	    clip.y = gw->ggc->clip.y;
	    clip.width = gw->ggc->clip.width;
	    clip.height = gw->ggc->clip.height;
	    XftDrawSetClipRectangles(gw->xft_w,0,0,&clip,1);
	    my_xft_render_layout(gw->xft_w,&fg,gw->pango_layout,x,y);
	}
    } else if ( drawit==tf_rect ) {
	PangoLayoutIter *iter;
	PangoLayoutRun *run;
	PangoFontMetrics *fm;

	pango_layout_get_pixel_extents(gw->pango_layout,&ink,&rect);
	arg->size.lbearing = ink.x - rect.x;
	arg->size.rbearing = ink.x+ink.width - rect.x;
	arg->size.width = rect.width;
	if ( *text=='\0' ) {
	    /* There are no runs if there are no characters */
	    memset(&arg->size,0,sizeof(arg->size));
	} else {
	    iter = pango_layout_get_iter(gw->pango_layout);
	    run = pango_layout_iter_get_run(iter);
	    if ( run==NULL ) {
		/* Pango doesn't give us runs in a couple of other places */
		/* surrogates, not unicode (0xfffe, 0xffff), etc. */
		memset(&arg->size,0,sizeof(arg->size));
	    } else {
		fm = pango_font_get_metrics(run->item->analysis.font,NULL);
		arg->size.fas = pango_font_metrics_get_ascent(fm)/PANGO_SCALE;
		arg->size.fds = pango_font_metrics_get_descent(fm)/PANGO_SCALE;
		arg->size.as = ink.y + ink.height - arg->size.fds;
		arg->size.ds = arg->size.fds - ink.y;
		if ( arg->size.ds<0 ) {
		    --arg->size.as;
		    arg->size.ds = 0;
		}
		/* In the one case I've looked at fds is one pixel off from rect.y */
		/*  I don't know what to make of that */
		pango_font_metrics_unref(fm);
	    }
	    pango_layout_iter_free(iter);
	}
    }
return( rect.width );
}

void _GXPDraw_FontMetrics(GWindow gw, GFont *fi, int *as, int *ds, int *ld) {
    GXDisplay *gdisp = ((GXWindow) gw)->display;
    PangoFont *pfont;
    PangoFontMetrics *fm;

    _GXPDraw_configfont(gw, fi);
# if !defined(_NO_LIBCAIRO)
    if ( gw->usecairo )
	pfont = pango_font_map_load_font(gdisp->pangoc_fontmap,gdisp->pangoc_context,
		fi->pangoc_fd);
    else
#endif
	pfont = pango_font_map_load_font(gdisp->pango_fontmap,gdisp->pango_context,
		fi->pango_fd);
    fm = pango_font_get_metrics(pfont,NULL);
    *as = pango_font_metrics_get_ascent(fm)/PANGO_SCALE;
    *ds = pango_font_metrics_get_descent(fm)/PANGO_SCALE;
    *ld = 0;
    pango_font_metrics_unref(fm);
    // pango_font_unref(pfont);
    // This function has disappeared from Pango with no explanation.
    // But we still leak memory here.
}

/* ************************************************************************** */
/* ****************************** Pango Layout ****************************** */
/* ************************************************************************** */
void _GXPDraw_LayoutInit(GWindow w, char *text, int cnt, GFont *fi) {
    GXWindow gw = (GXWindow) w;
    PangoFontDescription *fd;

    if ( fi==NULL )
	fi = gw->ggc->fi;

    fd = _GXPDraw_configfont(w, fi);
    pango_layout_set_font_description(gw->pango_layout,fd);
    pango_layout_set_text(gw->pango_layout,(char *) text,cnt);
}

void _GXPDraw_LayoutDraw(GWindow w, int32 x, int32 y, Color col) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;

# if !defined(_NO_LIBCAIRO)
    if ( gw->usecairo ) {
	my_cairo_render_layout(gw->cc,col,gw->pango_layout,x,y);
    } else
#endif
    {
	XftColor fg;
	XRenderColor fgcol;
	XRectangle clip;
	fgcol.red = COLOR_RED(col)<<8; fgcol.green = COLOR_GREEN(col)<<8; fgcol.blue = COLOR_BLUE(col)<<8;
	if ( COLOR_ALPHA(col)!=0 )
	    fgcol.alpha = COLOR_ALPHA(col)*0x101;
	else
	    fgcol.alpha = 0xffff;
	XftColorAllocValue(gdisp->display,gdisp->visual,gdisp->cmap,&fgcol,&fg);
	clip.x = gw->ggc->clip.x;
	clip.y = gw->ggc->clip.y;
	clip.width = gw->ggc->clip.width;
	clip.height = gw->ggc->clip.height;
	XftDrawSetClipRectangles(gw->xft_w,0,0,&clip,1);
	my_xft_render_layout(gw->xft_w,&fg,gw->pango_layout,x,y);
    }
}

void _GXPDraw_LayoutIndexToPos(GWindow w, int index, GRect *pos) {
    GXWindow gw = (GXWindow) w;
    PangoRectangle rect;

    pango_layout_index_to_pos(gw->pango_layout,index,&rect);
    pos->x = rect.x/PANGO_SCALE; pos->y = rect.y/PANGO_SCALE; pos->width = rect.width/PANGO_SCALE; pos->height = rect.height/PANGO_SCALE;
}

int _GXPDraw_LayoutXYToIndex(GWindow w, int x, int y) {
    GXWindow gw = (GXWindow) w;
    int trailing, index;

    /* Pango retuns the last character if x is negative, not the first */
    if ( x<0 ) x=0;
    pango_layout_xy_to_index(gw->pango_layout,x*PANGO_SCALE,y*PANGO_SCALE,&index,&trailing);
    /* If I give pango a position after the last character on a line, it */
    /*  returns to me the first character. Strange. And annoying -- you click */
    /*  at the end of a line and the cursor moves to the start */
    /* Of course in right to left text an initial position is correct... */
    if ( index+trailing==0 && x>0 ) {
	PangoRectangle rect;
	pango_layout_get_pixel_extents(gw->pango_layout,&rect,NULL);
	if ( x>=rect.width ) {
	    x = rect.width-1;
	    pango_layout_xy_to_index(gw->pango_layout,x*PANGO_SCALE,y*PANGO_SCALE,&index,&trailing);
	}
    }
return( index+trailing );
}

void _GXPDraw_LayoutExtents(GWindow w, GRect *size) {
    GXWindow gw = (GXWindow) w;
    PangoRectangle rect;

    pango_layout_get_pixel_extents(gw->pango_layout,NULL,&rect);
    size->x = rect.x; size->y = rect.y; size->width = rect.width; size->height = rect.height;
}

void _GXPDraw_LayoutSetWidth(GWindow w, int width) {
    GXWindow gw = (GXWindow) w;

    pango_layout_set_width(gw->pango_layout,width==-1? -1 : width*PANGO_SCALE);
}

int _GXPDraw_LayoutLineCount(GWindow w) {
    GXWindow gw = (GXWindow) w;

return( pango_layout_get_line_count(gw->pango_layout));
}

int _GXPDraw_LayoutLineStart(GWindow w, int l) {
    GXWindow gw = (GXWindow) w;
    PangoLayoutLine *line;

    line = pango_layout_get_line(gw->pango_layout,l);
    if ( line==NULL )
return( -1 );

return( line->start_index );
}

#endif // FONTFORGE_CAN_USE_GDK
