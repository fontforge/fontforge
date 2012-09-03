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

#ifdef __VMS
#include <vms_x_fix.h>
#endif

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

#ifdef _NO_LIBPANGO
static int usepango=false;
#else
static int usepango=true;
#endif
#ifdef _NO_LIBCAIRO
static int usecairo = false;
#else
static int usecairo = true;
#endif

static void MacVersionTest(void) {
    /* pango and cairo do not work on mac 10.5.0 same code runs fine on 10.5.6 */
    /* I'm guessing that there is a problem with libfontconfig under Xfree86 */
    /*  which is not present in the X.org release. (X11->About says it is */
    /*  Xfree under 10.5.0, and X.org under 10.5.6) */
    /* If I try to use pango or cairo under 10.5.0 fontforge crashes instantly*/
    /*  and people blame me */
    /* Note that pango runs fine under 10.4.? */
    /* So how to detect a bad X library? */
    /*  well `uname -r` gives us 9.0.0 on 10.5, so problems may happen above that */
    /*  the X.org bin directory contains the file "Xnest" while the */
    /*   XFree bin directory does not */
#ifdef __Mac
    struct utsname osinfo;
    static int tested=0;

    if ( tested || (!usepango && !usecairo))
return;
    tested = true;

    if ( uname(&osinfo)!=0 ) {
	/* Error? Shouldn't happen */
	usecairo = usepango = false;
return;
    }
    if ( *osinfo.release!='9' || access("/usr/X11R6/bin/Xnest",0)==0 )
return;		/* Not 10.5.* or we've got the Xorg distribution */
    usecairo = usepango = false;
    fprintf( stderr, "You appear to have a version of X11 with a bug in it.\n" );
    fprintf( stderr, " This bug will cause fontforge to crash if it attempts\n" );
    fprintf( stderr, " to use the pango or cairo libraries. FontForge will\n" );
    fprintf( stderr, " not attempt to use these libraries.\n\n" );
    fprintf( stderr, "You can install a fix by going to <Apple>->Software Update\n" );
    fprintf( stderr, " and selecting \"Mac OS X Update Combined\".\n\n" );
    fprintf( stderr, "If you believe you do have the correct software, then\n" );
    fprintf( stderr, " simply create the file \"/usr/X11R6/bin/Xnest\" and\n" );
    fprintf( stderr, " fontforge will shut up.\n" );
    GDrawError(
	"You appear to have a version of X11 with a bug in it."
	" This bug will cause fontforge to crash if it attempts"
	" to use the pango or cairo libraries. FontForge will"
	" not attempt to use these libraries.\n\n"
	"You can install a fix by going to <Apple>->Software Update"
	" and selecting \"Mac OS X Update Combined\".\n\n"
	"If you don't want to see this message at startup, go to"
	" File->Preferences->Generic and turn off both"
	" UseCairo and UsePango.\n\n"
	"If you believe you do have the correct software, then"
	" simply create the file \"/usr/X11R6/bin/Xnest\" and"
	" fontforge will shut up.\n"
    );
#endif
}

void GDrawEnableCairo(int on) {
    usecairo=on;
    /* Obviously, if we have no library, enabling it will do nothing */
}

void GDrawEnablePango(int on) {
    usepango=on;
    /* Obviously, if we have no library, enabling it will do nothing */
}

#ifndef _NO_LIBCAIRO
# if (CAIRO_VERSION_MAJOR==1 && CAIRO_VERSION_MINOR<6) || (!defined(_STATIC_LIBCAIRO) && !defined(NODYNAMIC))
static int __cairo_format_stride_for_width(cairo_format_t f,int width) {
    if ( f==CAIRO_FORMAT_ARGB32 || f==CAIRO_FORMAT_RGB24 )
return( 4*width );
    else if ( f==CAIRO_FORMAT_A8 )
return( ((width+3)>>2)<<2 );
    else if ( f==CAIRO_FORMAT_A1 )
return( ((width+31)>>5)<<5 );
    else
return( 4*width );
}
#endif

/* ************************************************************************** */
/* ***************************** Cairo Library ****************************** */
/* ************************************************************************** */

/*
 * FIXME: Eliminate these redundant macros. They are left over from
 *        when we sometimes used dlopen(3) to link with Cairo.
 */
#  define _cairo_xlib_surface_create cairo_xlib_surface_create
#  define _cairo_create cairo_create
#  define _cairo_xlib_surface_set_size cairo_xlib_surface_set_size
#  define _cairo_destroy cairo_destroy
#  define _cairo_surface_destroy cairo_surface_destroy
#  define _cairo_set_line_width cairo_set_line_width
#  define _cairo_set_line_cap cairo_set_line_cap
#  define _cairo_set_line_join cairo_set_line_join
#  define _cairo_set_operator cairo_set_operator
#  define _cairo_set_dash cairo_set_dash
#  define _cairo_set_source_rgba cairo_set_source_rgba
#  define _cairo_new_path cairo_new_path
#  define _cairo_new_sub_path cairo_new_sub_path
#  define _cairo_move_to cairo_move_to
#  define _cairo_line_to cairo_line_to
#  define _cairo_curve_to cairo_curve_to
#  define _cairo_rectangle cairo_rectangle
#  define _cairo_arc cairo_arc
#  define _cairo_close_path cairo_close_path
#  define _cairo_stroke cairo_stroke
#  define _cairo_fill cairo_fill
#  define _cairo_fill_preserve cairo_fill_preserve
#  define _cairo_clip cairo_clip
#  define _cairo_save cairo_save
#  define _cairo_restore cairo_restore
#  define _cairo_scaled_font_extents	cairo_scaled_font_extents
#  define _cairo_scaled_font_text_extents	cairo_scaled_font_text_extents
#  define _cairo_set_scaled_font	cairo_set_scaled_font
#  define _cairo_show_text	cairo_show_text
#  define _cairo_scaled_font_create	cairo_scaled_font_create
#  define _cairo_set_source_surface cairo_set_source_surface
#  define _cairo_mask_surface cairo_mask_surface
#  define _cairo_scale cairo_scale
#  define _cairo_translate cairo_translate
#  define _cairo_image_surface_create_for_data cairo_image_surface_create_for_data
#  define _cairo_image_surface_get_format cairo_image_surface_get_format
# if CAIRO_VERSION_MAJOR==1 && CAIRO_VERSION_MINOR>=6
#   define _cairo_format_stride_for_width cairo_format_stride_for_width
# else
#   define _cairo_format_stride_for_width __cairo_format_stride_for_width
#endif
#  define _cairo_ft_font_face_create_for_pattern cairo_ft_font_face_create_for_pattern
#  define _cairo_font_options_create cairo_font_options_create
#  define _cairo_surface_flush cairo_surface_flush
#  define _cairo_surface_mark_dirty_rectangle cairo_surface_mark_dirty_rectangle
#  define _cairo_pattern_create_for_surface cairo_pattern_create_for_surface
#  define _cairo_pattern_set_extend cairo_pattern_set_extend
#  define _cairo_pattern_destroy cairo_pattern_destroy
#  define _cairo_set_source cairo_set_source
#  define _cairo_push_group cairo_push_group
#  define _cairo_pop_group_to_source cairo_pop_group_to_source
#  define _cairo_get_group_target cairo_get_group_target
#  define _cairo_paint cairo_paint
#  define _cairo_set_fill_rule cairo_set_fill_rule

#  define _FcCharSetHasChar    FcCharSetHasChar     
#  define _FcPatternDestroy    FcPatternDestroy   
#  define _FcPatternAddDouble  FcPatternAddDouble  
#  define _FcPatternAddInteger FcPatternAddInteger 
#  define _FcPatternAddString  FcPatternAddString  
#  define _FcFontRenderPrepare FcFontRenderPrepare 
#  define _FcConfigSubstitute  FcConfigSubstitute  
#  define _FcDefaultSubstitute FcDefaultSubstitute 
#  define _FcFontSort          FcFontSort           
#  define _FcPatternGetCharSet FcPatternGetCharSet
#  define _FcPatternCreate     FcPatternCreate

int _GXCDraw_hasCairo(void) {
    int initted = false, hasC;
    if ( !usecairo )
	return( false );
    if ( !initted )
	hasC = FcInit();
    return( hasC );
}

/* ************************************************************************** */
/* ****************************** Cairo Window ****************************** */
/* ************************************************************************** */
void _GXCDraw_NewWindow(GXWindow nw,Color bg) {
    GXDisplay *gdisp = nw->display;
    Display *display = gdisp->display;

    MacVersionTest();
    if ( !usecairo || !_GXCDraw_hasCairo())
return;

    nw->cs = _cairo_xlib_surface_create(display,nw->w,gdisp->visual,
	    nw->pos.width, nw->pos.height );
    if ( nw->cs!=NULL ) {
	nw->cc = _cairo_create(nw->cs);
	if ( nw->cc!=NULL )
	    nw->usecairo = true;
	else {
	    _cairo_surface_destroy(nw->cs);
	    nw->cs=NULL;
	}
    }
}

void _GXCDraw_ResizeWindow(GXWindow gw,GRect *rect) {
    _cairo_xlib_surface_set_size( gw->cs, rect->width,rect->height);
}

void _GXCDraw_DestroyWindow(GXWindow gw) {
    _cairo_destroy(gw->cc);
    _cairo_surface_destroy(gw->cs);
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
	is = _cairo_image_surface_create_for_data((uint8 *) space,CAIRO_FORMAT_ARGB32,
		8,8,8*4);
	pat = _cairo_pattern_create_for_surface(is);
	_cairo_pattern_set_extend(pat,CAIRO_EXTEND_REPEAT);
    }
    _cairo_set_source(gw->cc,pat);
}

static int GXCDrawSetcolfunc(GXWindow gw, GGC *mine) {
    /*GCState *gcs = &gw->cairo_state;*/
    Color fg = mine->fg;

    if ( (fg>>24 ) == 0 )
	fg |= 0xff000000;

    if ( mine->ts != 0 ) {
	GXCDraw_StippleMePink(gw,mine->ts,fg);
    } else {
	_cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
		(fg>>24)/255.);
    }
#if 0
/* As far as I can tell, XOR doesn't work */
/* Or perhaps it is more accurate to say that I don't understand what xor does in cairo*/
    if ( mine->func!=gcs->func || mine->func!=df_copy ) {
	_cairo_set_operator( gw->cc,mine->func==df_copy?CAIRO_OPERATOR_OVER:CAIRO_OPERATOR_XOR );
	gcs->func = mine->func;
    }
    if ( mine->func==df_xor )
	fg ^= mine->xor_base;
#endif
return( true );
}

static int GXCDrawSetline(GXWindow gw, GGC *mine) {
    GCState *gcs = &gw->cairo_state;
    Color fg = mine->fg;

    if ( ( fg>>24 ) == 0 )
	fg |= 0xff000000;

#if 0
/* As far as I can tell, XOR doesn't work */
/* Or perhaps it is more accurate to say that I don't understand what xor does*/
    if ( mine->func!=gcs->func || mine->func!=df_copy ) {
	_cairo_set_operator( gw->cc, mine->func==df_copy?CAIRO_OPERATOR_OVER:CAIRO_OPERATOR_XOR);
	gcs->func = mine->func;
    }
    if ( mine->func==df_xor )
	fg ^= mine->xor_base;
#endif
    if ( mine->line_width<=0 ) mine->line_width = 1;
    if ( mine->line_width!=gcs->line_width || mine->line_width!=2 ) {
	_cairo_set_line_width(gw->cc,mine->line_width);
	gcs->line_width = mine->line_width;
    }
    if ( mine->dash_len != gcs->dash_len || mine->skip_len != gcs->skip_len ||
	    mine->dash_offset != gcs->dash_offset ) {
	double dashes[2];
	dashes[0] = mine->dash_len; dashes[1] = mine->skip_len;
	_cairo_set_dash(gw->cc,dashes,0,mine->dash_offset);
	gcs->dash_offset = mine->dash_offset;
	gcs->dash_len = mine->dash_len;
	gcs->skip_len = mine->skip_len;
    }
    /* I don't use line join/cap. On a screen with small line_width they are irrelevant */

    if ( mine->ts != 0 ) {
	GXCDraw_StippleMePink(gw,mine->ts,fg);
    } else {
	_cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
		    (fg>>24)/255.0);
    }
return( mine->line_width );
}

void _GXCDraw_PushClip(GXWindow gw) {
    _cairo_save(gw->cc);
    _cairo_new_path(gw->cc);
    _cairo_rectangle(gw->cc,gw->ggc->clip.x,gw->ggc->clip.y,gw->ggc->clip.width,gw->ggc->clip.height);
    _cairo_clip(gw->cc);
}

void _GXCDraw_PopClip(GXWindow gw) {
    _cairo_restore(gw->cc);
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
    _cairo_new_path(gw->cc);
    _cairo_rectangle(gw->cc,r->x,r->y,r->width,r->height);
    _cairo_set_source_rgba(gw->cc,COLOR_RED(gw->ggc->bg)/255.0,COLOR_GREEN(gw->ggc->bg)/255.0,COLOR_BLUE(gw->ggc->bg)/255.0,
	    1.0);
    _cairo_fill(gw->cc);
}

void _GXCDraw_DrawLine(GXWindow gw, int32 x,int32 y, int32 xend,int32 yend) {
    int width = GXCDrawSetline(gw,gw->ggc);

    _cairo_new_path(gw->cc);
    if ( width&1 ) {
	_cairo_move_to(gw->cc,x+.5,y+.5);
	_cairo_line_to(gw->cc,xend+.5,yend+.5);
    } else {
	_cairo_move_to(gw->cc,x,y);
	_cairo_line_to(gw->cc,xend,yend);
    }
    _cairo_stroke(gw->cc);
}

void _GXCDraw_DrawRect(GXWindow gw, GRect *rect) {
    int width = GXCDrawSetline(gw,gw->ggc);

    _cairo_new_path(gw->cc);
    if ( width&1 ) {
	_cairo_rectangle(gw->cc,rect->x+.5,rect->y+.5,rect->width,rect->height);
    } else {
	_cairo_rectangle(gw->cc,rect->x,rect->y,rect->width,rect->height);
    }
    _cairo_stroke(gw->cc);
}

void _GXCDraw_FillRect(GXWindow gw, GRect *rect) {
    GXCDrawSetcolfunc(gw,gw->ggc);

    _cairo_new_path(gw->cc);
    _cairo_rectangle(gw->cc,rect->x,rect->y,rect->width,rect->height);
    _cairo_fill(gw->cc);
}

void _GXCDraw_FillRoundRect(GXWindow gw, GRect *rect, int radius) {
    double degrees = M_PI / 180.0;

    GXCDrawSetcolfunc(gw,gw->ggc);

    _cairo_new_path(gw->cc);
    _cairo_arc(gw->cc, rect->x + rect->width - radius, rect->y + radius, radius, -90 * degrees, 0 * degrees);
    _cairo_arc(gw->cc, rect->x + rect->width - radius, rect->y + rect->height - radius, radius, 0 * degrees, 90 * degrees);
    _cairo_arc(gw->cc, rect->x + radius, rect->y + rect->height - radius, radius, 90 * degrees, 180 * degrees);
    _cairo_arc(gw->cc, rect->x + radius, rect->y + radius, radius, 180 * degrees, 270 * degrees);
    _cairo_close_path(gw->cc);
    _cairo_fill(gw->cc);

}

static void GXCDraw_EllipsePath(cairo_t *cc,double cx,double cy,double width,double height) {
    _cairo_new_path(cc);
    _cairo_move_to(cc,cx,cy+height);
    _cairo_curve_to(cc,
	    cx+.552*width,cy+height,
	    cx+width,cy+.552*height,
	    cx+width,cy);
    _cairo_curve_to(cc,
	    cx+width,cy-.552*height,
	    cx+.552*width,cy-height,
	    cx,cy-height);
    _cairo_curve_to(cc,
	    cx-.552*width,cy-height,
	    cx-width,cy-.552*height,
	    cx-width,cy);
    _cairo_curve_to(cc,
	    cx-width,cy+.552*height,
	    cx-.552*width,cy+height,
	    cx,cy+height);
    _cairo_close_path(cc);
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
    _cairo_stroke(gw->cc);
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
    _cairo_fill(gw->cc);
}

void _GXCDraw_DrawPoly(GXWindow gw, GPoint *pts, int16 cnt) {
    int width = GXCDrawSetline(gw,gw->ggc);
    double off = width&1 ? .5 : 0;
    int i;

    _cairo_new_path(gw->cc);
    _cairo_move_to(gw->cc,pts[0].x+off,pts[0].y+off);
    for ( i=1; i<cnt; ++i )
	_cairo_line_to(gw->cc,pts[i].x+off,pts[i].y+off);
    _cairo_stroke(gw->cc);
}

void _GXCDraw_FillPoly(GXWindow gw, GPoint *pts, int16 cnt) {
    GXCDrawSetcolfunc(gw,gw->ggc);
    int i;

    _cairo_new_path(gw->cc);
    _cairo_move_to(gw->cc,pts[0].x,pts[0].y);
    for ( i=1; i<cnt; ++i )
	_cairo_line_to(gw->cc,pts[i].x,pts[i].y);
    _cairo_close_path(gw->cc);
    _cairo_fill(gw->cc);

    _cairo_set_line_width(gw->cc,1);
    _cairo_new_path(gw->cc);
    _cairo_move_to(gw->cc,pts[0].x+.5,pts[0].y+.5);
    for ( i=1; i<cnt; ++i )
	_cairo_line_to(gw->cc,pts[i].x+.5,pts[i].y+.5);
    _cairo_close_path(gw->cc);
    _cairo_stroke(gw->cc);
}

/* ************************************************************************** */
/* ****************************** Cairo Paths ******************************* */
/* ************************************************************************** */
void _GXCDraw_PathStartNew(GWindow w) {
    _cairo_new_path( ((GXWindow) w)->cc );
}

void _GXCDraw_PathStartSubNew(GWindow w) {
    _cairo_new_sub_path( ((GXWindow) w)->cc );
}

int _GXCDraw_FillRuleSetWinding(GWindow w) {
    _cairo_set_fill_rule(((GXWindow) w)->cc,CAIRO_FILL_RULE_WINDING);
    return 1;
}

void _GXCDraw_PathClose(GWindow w) {
    _cairo_close_path( ((GXWindow) w)->cc );
}

void _GXCDraw_PathMoveTo(GWindow w,double x, double y) {
    _cairo_move_to( ((GXWindow) w)->cc,x,y );
}

void _GXCDraw_PathLineTo(GWindow w,double x, double y) {
    _cairo_line_to( ((GXWindow) w)->cc,x,y );
}

void _GXCDraw_PathCurveTo(GWindow w,
		    double cx1, double cy1,
		    double cx2, double cy2,
		    double x, double y) {
    _cairo_curve_to( ((GXWindow) w)->cc,cx1,cy1,cx2,cy2,x,y );
}

void _GXCDraw_PathStroke(GWindow w,Color col) {
    w->ggc->fg = col;
    GXCDrawSetline((GXWindow) w,w->ggc);
    _cairo_stroke( ((GXWindow) w)->cc );
}

void _GXCDraw_PathFill(GWindow w,Color col) {
    _cairo_set_source_rgba(((GXWindow) w)->cc,COLOR_RED(col)/255.0,COLOR_GREEN(col)/255.0,COLOR_BLUE(col)/255.0,
	    (col>>24)/255.0);
    _cairo_fill( ((GXWindow) w)->cc );
}

void _GXCDraw_PathFillAndStroke(GWindow w,Color fillcol, Color strokecol) {
    GXWindow gw = (GXWindow) w;

    _cairo_save(gw->cc);
    _cairo_set_source_rgba(gw->cc,COLOR_RED(fillcol)/255.0,COLOR_GREEN(fillcol)/255.0,COLOR_BLUE(fillcol)/255.0,
	    (fillcol>>24)/255.0);
    _cairo_fill( gw->cc );
    _cairo_restore(gw->cc);
    w->ggc->fg = strokecol;
    GXCDrawSetline(gw,gw->ggc);
    _cairo_fill( gw->cc );
}

/* ************************************************************************** */
/* *************************** Cairo Text & Fonts *************************** */
/* ************************************************************************** */

static int indexOfChar(GFont *font,unichar_t ch,int last_index) {
    int new_index = -1, i;

    if ( last_index>=0 && last_index<font->ordered->nfont ) {
	if ( _FcCharSetHasChar(font->cscf[last_index].cs,ch) )
	    new_index = last_index;
    }
    if ( new_index==-1 ) {
	for ( i=0; i<font->ordered->nfont; ++i ) if ( i!=last_index ) {
	    if ( _FcCharSetHasChar(font->cscf[i].cs,ch) ) {
		new_index = i;
	break;
	    }
	}
    }
    if ( new_index!=-1 && font->cscf[new_index].cf==NULL ) {
	FcPattern *onefont;
	cairo_matrix_t fm, cm;
	static cairo_font_options_t *def=NULL;

	if ( def==NULL )	/* Docs claim this isn't needed. Docs appear wrong */
	    def = _cairo_font_options_create();

	onefont = _FcFontRenderPrepare(NULL,font->pat,font->ordered->fonts[new_index]);
#if 0
 { double ps, pre=-1;
     FcPatternGetDouble(onefont,FC_PIXEL_SIZE,0,&ps);
     FcPatternGetDouble(font->ordered->fonts[new_index],FC_PIXEL_SIZE,0,&pre);
     printf( "Pixel size desired=%d, pre=%g, in font=%g\n", font->pixelsize, pre, ps );
 }
#endif

	memset(&fm,0,sizeof(fm)); memset(&cm,0,sizeof(cm));
	fm.xx = fm.yy = font->pixelsize;
	cm.xx = cm.yy = 1;
	font->cscf[new_index].cf = _cairo_scaled_font_create(
		_cairo_ft_font_face_create_for_pattern(onefont),
		&fm,&cm,def );
	/*_FcPatternDestroy(onefont);*/
    }
return( new_index );
}
	
static void configfont(GFont *font) {
    FcPattern *pat;
    int pixel_size;
    FcResult retval;
    int i;
    static unichar_t replacements[] = { 0xfffd, 0xfffc, '?', 0 };

    if ( font->ordered!=NULL )
return;

    pat = _FcPatternCreate();
    _FcPatternAddDouble(pat,FC_DPI,GDrawPointsToPixels(NULL,72));
    if ( font->rq.point_size>0 ) {
	_FcPatternAddDouble(pat,FC_SIZE,font->rq.point_size);
	pixel_size = GDrawPointsToPixels(NULL,font->rq.point_size);
    } else {
	_FcPatternAddDouble(pat,FC_PIXEL_SIZE,-font->rq.point_size);
	pixel_size = -font->rq.point_size;
    }

    if ( font->rq.style&fs_italic )
	_FcPatternAddInteger(pat,FC_SLANT,FC_SLANT_ITALIC);
    else
	_FcPatternAddInteger(pat,FC_SLANT,FC_SLANT_ROMAN);

    if ( font->rq.weight<=200 )
	_FcPatternAddInteger(pat,FC_WEIGHT,FC_WEIGHT_LIGHT);
    else if ( font->rq.weight<=400 )
	_FcPatternAddInteger(pat,FC_WEIGHT,FC_WEIGHT_LIGHT+
		((font->rq.weight-200)/(400-200)) * (FC_WEIGHT_MEDIUM-FC_WEIGHT_LIGHT));
    else if ( font->rq.weight<=600 )
	_FcPatternAddInteger(pat,FC_WEIGHT,FC_WEIGHT_MEDIUM+
		((font->rq.weight-400)/(600-400)) * (FC_WEIGHT_DEMIBOLD-FC_WEIGHT_MEDIUM));
    else if ( font->rq.weight<=700 )
	_FcPatternAddInteger(pat,FC_WEIGHT,FC_WEIGHT_DEMIBOLD+
		((font->rq.weight-600)/(700-600)) * (FC_WEIGHT_BOLD-FC_WEIGHT_DEMIBOLD));
    else if ( font->rq.weight<=900 )
	_FcPatternAddInteger(pat,FC_WEIGHT,FC_WEIGHT_BOLD+
		((font->rq.weight-700)/(900-700)) * (FC_WEIGHT_BLACK-FC_WEIGHT_BOLD));
    else
	_FcPatternAddInteger(pat,FC_WEIGHT,FC_WEIGHT_BLACK);

    if ( font->rq.utf8_family_name != NULL ) {
	char *start, *end;
	for ( start=font->rq.utf8_family_name; *start; start=end ) {
	    for ( end=start; *end!='\0' && *end!=',' ; ++end );
	    _FcPatternAddString(pat,FC_FAMILY,copyn(start,end-start));
	    while ( *end==',' ) ++end;
	}
    } else {
	const unichar_t *start, *end;
	for ( start=font->rq.family_name; *start; start=end ) {
	    for ( end=start; *end!='\0' && *end!=',' ; ++end );
	    _FcPatternAddString(pat,FC_FAMILY,u2utf8_copyn(start,end-start));
	    while ( *end==',' ) ++end;
	}
    }

    _FcConfigSubstitute(NULL,pat,FcMatchPattern);
    _FcDefaultSubstitute(pat);
    font->ordered = _FcFontSort(NULL,pat,true,NULL,&retval);
    font->pat = pat;
    font->pixelsize = pixel_size;
    font->replacement_index = -1;
    if ( font->ordered!=NULL ) {
	font->cscf = gcalloc(font->ordered->nfont,sizeof(struct charset_cairofont));
	for ( i=0; i<font->ordered->nfont; ++i )
	    _FcPatternGetCharSet(font->ordered->fonts[i],FC_CHARSET,0,
		    &font->cscf[i].cs);
	for ( i=0; replacements[i]!=0 ; ++i ) {
	    font->replacement_char = replacements[i];
	    font->replacement_index = indexOfChar(font,replacements[i],-1);
	    if ( font->replacement_index!=-1 )
	break;
	}
    }
}

void _GXCDraw_FontMetrics(GXWindow gw,GFont *fi,int *as, int *ds, int *ld) {
    int index;

    if ( fi->ordered==NULL )
	configfont(fi);
    if ( fi->ordered==NULL ) {
	*as = *ds = *ld = -1;
return;
    }
    index = indexOfChar(fi,'A',-1);
    if ( index==-1 )
	index = fi->replacement_index;
    if ( index==-1 )
	*as = *ds = *ld = -1;
    else {
	cairo_font_extents_t bounds;
	_cairo_scaled_font_extents(fi->cscf[index].cf,&bounds);
	*as = rint(bounds.ascent);
	*ds = rint(bounds.descent);
	*ld = rint(bounds.height-(bounds.ascent+bounds.descent));
    }
}

int32 _GXCDraw_DoText8(GWindow w, int32 x, int32 y,
	const char *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    GXWindow gw = (GXWindow) w;
    const char *end = text+(cnt<0?strlen(text):cnt);
    struct font_instance *fi = gw->ggc->fi;
    const char *start, *pt, *last_good;
    uint32 ch;
    int index, last_index;
    char outbuffer[100], *outpt, *outend;
    cairo_text_extents_t ct;
    cairo_font_extents_t bounds;

    if ( fi->ordered==NULL )
	configfont(fi);
    if ( fi->ordered==NULL )
return( x );

    if ( drawit==tf_drawit ) {
	gw->ggc->fg = col;
	GXCDrawSetcolfunc(gw,gw->ggc);
	_cairo_move_to(gw->cc,x,y);
    }

    for ( start = text; start<end; ) {
	pt = start;
	ch = utf8_ildb(&pt);
	if ( ch<=0 )
    break;
	outpt = outbuffer; outend = outpt+100-5;
	last_index = indexOfChar(fi,ch,-1);
	if ( last_index==-1 ) {
	    last_index = fi->replacement_index;
	    if ( last_index!=-1 )
		outpt = utf8_idpb(outpt,fi->replacement_char);
	    last_good = pt;
	} else {
	    outpt = utf8_idpb(outpt,ch);
	    last_good = pt;
	    if ( drawit==tf_width || drawit==tf_drawit || drawit==tf_rect )
		    /* Stopat functions should examine each glyph drawn */
		    while ( pt<end && outpt<=outend ) {
		ch = utf8_ildb((const char **) &pt);
		if ( ch<=0 )
	    break;
		index = indexOfChar(fi,ch,last_index);
		if ( index!=last_index )
	    break;
		outpt = utf8_idpb(outpt,ch);
		last_good = pt;
	    }
	}
	if ( last_index!=-1 ) {
	    *outpt++ = '\0';
	    _cairo_scaled_font_text_extents(fi->cscf[last_index].cf,outbuffer,&ct);
	    switch ( drawit ) {
	      case tf_width:
	        /* We just need to increment x, which happens at the end */;
	      break;
	      case tf_drawit:
		_cairo_set_scaled_font(gw->cc,fi->cscf[last_index].cf);
		_cairo_show_text(gw->cc,outbuffer);
	      break;
	      case tf_rect:
		if ( x==0 )
		    arg->size.lbearing = x+ct.x_bearing;
		arg->size.rbearing = x+ct.width;
		_cairo_scaled_font_extents(fi->cscf[last_index].cf,&bounds);
		if ( arg->size.fas<bounds.ascent )
		    arg->size.fas = bounds.ascent;
		if ( arg->size.fds<bounds.descent )
		    arg->size.fds = bounds.descent;
		if ( arg->size.as<-ct.y_bearing )
		    arg->size.as = -ct.y_bearing;
		if ( arg->size.ds<ct.height+ct.y_bearing )
		    arg->size.ds = ct.height+ct.y_bearing;
		arg->size.width += ct.x_advance;
	      break;
	      case tf_stopat:
		if ( x+ct.x_advance < arg->maxwidth )
		    /* Do Nothing here */;
		else if ( x+ct.x_advance/2 >= arg->maxwidth ) {
		    arg->utf8_last = (char *) start;
		    arg->width = x;
return( x );
		} else {
		    arg->utf8_last = (char *) pt;
		    x += ct.x_advance;
		    arg->width = x;
return( x );
		}
	      break;
	      case tf_stopbefore:
		if ( x+ct.x_advance >= arg->maxwidth ) {
		    arg->utf8_last = (char *) start;
		    arg->width = x;
return( x );
		}
	      break;
	      case tf_stopafter:
		if ( x+ct.x_advance >= arg->maxwidth ) {
		    arg->utf8_last = (char *) pt;
		    x += ct.x_advance;
		    arg->width = x;
return( x );
		}
	      break;
	    }
	    x += ct.x_advance;
	}
	start = last_good;
    }
return( x );
}

int32 _GXCDraw_DoText(GWindow w, int32 x, int32 y,
	const unichar_t *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    GXWindow gw = (GXWindow) w;
    const unichar_t *end = text+(cnt<0?u_strlen(text):cnt);
    struct font_instance *fi = gw->ggc->fi;
    const unichar_t *start, *pt, *last_good;
    uint32 ch;
    int index, last_index;
    char outbuffer[100], *outpt, *outend;
    cairo_text_extents_t ct;
    cairo_font_extents_t bounds;

    if ( fi->ordered==NULL )
	configfont(fi);
    if ( fi->ordered==NULL )
return( x );

    if ( drawit==tf_drawit ) {
	gw->ggc->fg = col;
	GXCDrawSetcolfunc(gw,gw->ggc);
	_cairo_move_to(gw->cc,x,y);
    }

    for ( start = text; start<end; ) {
	pt = start;
	ch = *pt++;
	if ( ch<=0 )
    break;
	outpt = outbuffer; outend = outpt+100-5;
	last_index = indexOfChar(fi,ch,-1);
	if ( last_index==-1 ) {
	    last_index = fi->replacement_index;
	    if ( last_index!=-1 )
		outpt = utf8_idpb(outpt,fi->replacement_char);
	    last_good = pt;
	} else {
	    outpt = utf8_idpb(outpt,ch);
	    last_good = pt;
	    if ( drawit==tf_width || drawit==tf_drawit || drawit==tf_rect )
		    /* Stopat functions should examine each glyph drawn */
		    while ( pt<end && outpt<=outend ) {
		ch = *pt++;
		if ( ch<=0 )
	    break;
		index = indexOfChar(fi,ch,last_index);
		if ( index!=last_index )
	    break;
		outpt = utf8_idpb(outpt,ch);
		last_good = pt;
	    }
	}
	if ( last_index!=-1 ) {
	    *outpt++ = '\0';
	    _cairo_scaled_font_text_extents(fi->cscf[last_index].cf,outbuffer,&ct);
	    switch ( drawit ) {
	      case tf_width:
	        /* We just need to increment x, which happens at the end */;
	      break;
	      case tf_drawit:
		_cairo_set_scaled_font(gw->cc,fi->cscf[last_index].cf);
		_cairo_show_text(gw->cc,outbuffer);
	      break;
	      case tf_rect:
		if ( x==0 )
		    arg->size.lbearing = x+ct.x_bearing;
		arg->size.rbearing = x+ct.width;
		/* I can't understand how ct.y_bearing & ct.height provide useful data !!!!*/
		_cairo_scaled_font_extents(fi->cscf[last_index].cf,&bounds);
		if ( arg->size.fas<bounds.ascent )
		    arg->size.fas = bounds.ascent;
		if ( arg->size.fds<bounds.descent )
		    arg->size.fds = bounds.descent;
		if ( arg->size.as<-ct.y_bearing )
		    arg->size.as = -ct.y_bearing;
		if ( arg->size.ds<ct.height+ct.y_bearing )
		    arg->size.ds = ct.height+ct.y_bearing;
		arg->size.width += ct.x_advance;
	      break;
	      case tf_stopat:
		if ( x+ct.x_advance < arg->maxwidth )
		    /* Do Nothing here */;
		else if ( x+ct.x_advance/2 >= arg->maxwidth ) {
		    arg->last = (unichar_t *) start;
		    arg->width = x;
return( x );
		} else {
		    arg->last = (unichar_t *) pt;
		    x += ct.x_advance;
		    arg->width = x;
return( x );
		}
	      break;
	      case tf_stopbefore:
		if ( x+ct.x_advance >= arg->maxwidth ) {
		    arg->last = (unichar_t *) start;
		    arg->width = x;
return( x );
		}
	      break;
	      case tf_stopafter:
		if ( x+ct.x_advance >= arg->maxwidth ) {
		    arg->last = (unichar_t *) pt;
		    x += ct.x_advance;
		    arg->width = x;
return( x );
		}
	      break;
	    }
	    x += ct.x_advance;
	}
	start = last_good;
    }
return( x );
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
# if CAIRO_VERSION_MAJOR==1 && CAIRO_VERSION_MINOR>=6
    /* Bug in old cairos. In them, this code doesn't work */
    if ( base->image_type == it_true && type == CAIRO_FORMAT_RGB24 ) {
	idata = ((uint32 *) (base->data)) + src->y*base->bytes_per_line + src->x;
	*_data = NULL;		/* We can reuse the image's own data, don't need a copy */
return( _cairo_image_surface_create_for_data((uint8 *) idata,type,
		src->width, src->height,
		base->bytes_per_line));
    }
#endif

    stride = _cairo_format_stride_for_width(type,src->width);
    *_data = data = galloc(stride * src->height);
    cs = _cairo_image_surface_create_for_data(data,type,
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

    if ( _cairo_image_surface_get_format(is)==CAIRO_FORMAT_A1 ) {
	/* No color info, just alpha channel */
	Color fg = base->clut->trans_index==0 ? base->clut->clut[1] : base->clut->clut[0];
	_cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,1.0);
	_cairo_mask_surface(gw->cc,is,x,y);
    } else {
	_cairo_set_source_surface(gw->cc,is,x,y);
	_cairo_rectangle(gw->cc,x,y,src->width,src->height);
	_cairo_fill(gw->cc);
    }
    /* Clear source and mask, in case we need to */
    _cairo_new_path(gw->cc);
    _cairo_set_source_rgba(gw->cc,0,0,0,0);

    _cairo_surface_destroy(is);
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
	int stride = _cairo_format_stride_for_width(CAIRO_FORMAT_A8,src->width);
	uint8 *basedata = galloc(stride*src->height),
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
	is = _cairo_image_surface_create_for_data(basedata,CAIRO_FORMAT_A8,
		src->width,src->height,stride);
	_cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,1.0);
	_cairo_mask_surface(gw->cc,is,x,y);
	/* I think the mask is sufficient, setting a rectangle would provide */
	/*  a new mask? */
	/*_cairo_rectangle(gw->cc,x,y,src->width,src->height);*/
	/* I think setting the mask also draws... at least so the tutorial implies */
	/* _cairo_fill(gw->cc);*/
	/* Presumably that doesn't leave the mask surface pattern lying around */
	/* but dereferences it so we can free it */
	_cairo_surface_destroy(is);
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
#if 1
  {
    GImage *temp = _GImageExtract(base,&full,&viewable,xscale,yscale);
    GRect src;
    src.x = src.y = 0; src.width = viewable.width; src.height = viewable.height;
    _GXCDraw_Image( gw, temp, &src, x+viewable.x, y+viewable.y);
  }
#else
  {
    cairo_surface_t *is;
    uint8 *data;
    /* I don't see why this doesn't work, but every now and then it will crash*/
    /*  deep inside of cairo. Perhaps cairo can't handle scaling images? */
    /*  Perhaps there's a bug in my code? */
    is = GImage2Surface(image,&full,&data);

    _cairo_save(gw->cc);
    _cairo_scale(gw->cc,viewable.width/((double) full.width),viewable.height/((double) full.height));

    if ( _cairo_image_surface_get_format(is)==CAIRO_FORMAT_A1 ) {
	/* No color info, just alpha channel */
	Color fg = base->clut->trans_index==0 ? base->clut->clut[1] : base->clut->clut[0];
	_cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,1.0);
	_cairo_mask_surface(gw->cc,is,viewable.x*xscale,viewable.y*yscale);
    } else {
	_cairo_set_source_surface(gw->cc,is,viewable.x/xscale,viewable.y/yscale);
	_cairo_paint(gw->cc);
    }
    _cairo_restore(gw->cc);

    _cairo_surface_destroy(is);
    free(data);
    gw->cairo_state.fore_col = COLOR_UNKNOWN;
  }
#endif
}

/* ************************************************************************** */
/* ******************************** Copy Area ******************************* */
/* ************************************************************************** */

void _GXCDraw_CopyArea( GXWindow from, GXWindow into, GRect *src, int32 x, int32 y) {

    if ( !into->usecairo || !from->usecairo ) {
	fprintf( stderr, "Cairo CopyArea called from something not cairo enabled\n" );
return;
    }

    _cairo_set_source_surface(into->cc,from->cs,x-src->x,y-src->y);
    _cairo_rectangle(into->cc,x,y,src->width,src->height);
    _cairo_fill(into->cc);

    /* Clear source and mask, in case we need to */
    _cairo_set_source_rgba(into->cc,0,0,0,0);

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

    if ( gw->usepango )
	flags |= gc_pango;

return( flags|gc_xor );	/* If not buffered, we can emulate xor by having X11 do it in the X layer */
}
/* ************************************************************************** */
/* **************************** Synchronization ***************************** */
/* ************************************************************************** */
void _GXCDraw_Flush(GXWindow gw) {
    _cairo_surface_flush(gw->cs);
}

void _GXCDraw_DirtyRect(GXWindow gw,double x, double y, double width, double height) {
    _cairo_surface_mark_dirty_rectangle(gw->cs,x,y,width,height);
}
#else
int _GXCDraw_hasCairo(void) {
return(false);
}

#endif	/* ! _NO_LIBCAIRO */

/* ************************************************************************** */
/* ***************************** Pango Library ****************************** */
/* ************************************************************************** */

#ifndef _NO_LIBPANGO

/*
 * FIXME: Eliminate these redundant macros. They are left over from
 *        when we sometimes used dlopen(3) to link with Xft.
 */
#  define _XftDrawCreate  XftDrawCreate
#  define _XftDrawDestroy XftDrawDestroy
#  define _XftColorAllocValue XftColorAllocValue
#  define _XftDrawSetClipRectangles XftDrawSetClipRectangles

/*
 * FIXME: Eliminate these redundant macros. They are left over from
 *        when we sometimes used dlopen(3) to link with Pango
 */
#  define _pango_font_description_new pango_font_description_new
#  define _pango_font_description_set_family pango_font_description_set_family
#  define _pango_font_description_set_style pango_font_description_set_style
#  define _pango_font_description_set_variant pango_font_description_set_variant
#  define _pango_font_description_set_weight pango_font_description_set_weight
#  define _pango_font_description_set_stretch pango_font_description_set_stretch
#  define _pango_font_description_set_size pango_font_description_set_size
#  if PANGO_VERSION_MINOR>=8
#   define _pango_font_description_set_absolute_size pango_font_description_set_absolute_size
#  else
#   define no_pango_font_description_set_absolute_size
#  endif
#  define _pango_layout_set_font_description pango_layout_set_font_description
#  define _pango_layout_set_text pango_layout_set_text
#  define _pango_layout_set_font_description pango_layout_set_font_description
#  define _pango_layout_set_text pango_layout_set_text
#  define _pango_layout_get_pixel_extents pango_layout_get_pixel_extents
#  define _pango_layout_index_to_pos pango_layout_index_to_pos
#  define _pango_layout_xy_to_index pango_layout_xy_to_index
#  define _pango_layout_set_width pango_layout_set_width
#  define _pango_layout_get_line_count pango_layout_get_line_count
#  define _pango_layout_get_line pango_layout_get_line
#  define _pango_font_map_load_font pango_font_map_load_font
#  define _pango_font_get_metrics pango_font_get_metrics
#  define _pango_font_metrics_unref pango_font_metrics_unref
#  define _pango_font_metrics_get_ascent pango_font_metrics_get_ascent
#  define _pango_font_metrics_get_descent pango_font_metrics_get_descent
#  define _pango_layout_new pango_layout_new
#  define _pango_layout_get_iter pango_layout_get_iter
#  define _pango_layout_iter_free pango_layout_iter_free
#  define _pango_layout_iter_next_run pango_layout_iter_next_run
#  define _pango_layout_iter_get_run pango_layout_iter_get_run
#  define _pango_layout_iter_get_run_extents pango_layout_iter_get_run_extents
#  define GTimer GTimer_GTK
#  include <pango/pangoxft.h>
#  if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
#   include <pango/pangocairo.h>
#   define _pango_cairo_layout_path pango_cairo_layout_path
#   define _pango_cairo_font_map_create_context pango_cairo_font_map_create_context
#   define _pango_cairo_font_map_get_default pango_cairo_font_map_get_default
#   define _pango_cairo_context_set_resolution pango_cairo_context_set_resolution
#   define _pango_cairo_show_glyph_string pango_cairo_show_glyph_string
#  endif
#  undef GTimer
#  if PANGO_VERSION_MINOR>=8
#   define _pango_xft_render_layout pango_xft_render_layout
#  else
#   define _pango_xft_render_layout my_xft_render_layout
#  endif
#  define _pango_xft_get_font_map pango_xft_get_font_map
#  define _pango_xft_get_context pango_xft_get_context
#  define _pango_xft_render pango_xft_render

int _GXPDraw_hasPango(void) {

    if ( !usepango )
return( false );

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
return( 3 );
# else
return( 1 );
# endif
}

/* ************************************************************************** */
/* ****************************** Pango Render ****************************** */
/* ************************************************************************** */

/* This routine exists in pango 1.8, but I depend on it, and I've only got 1.2*/
/*  as a test bed. So... */

static void my_xft_render_layout(XftDraw *xftw,XftColor *fgcol,
	PangoLayout *layout,int x,int y) {
    PangoRectangle rect, r2;
    PangoLayoutIter *iter;

    iter = _pango_layout_get_iter(layout);
    do {
	PangoLayoutRun *run = _pango_layout_iter_get_run(iter);
	if ( run!=NULL ) {	/* NULL runs mark end of line */
	    _pango_layout_iter_get_run_extents(iter,&r2,&rect);
	    _pango_xft_render(xftw,fgcol,run->item->analysis.font,run->glyphs,
		    x+(rect.x+PANGO_SCALE/2)/PANGO_SCALE, y+(rect.y+PANGO_SCALE/2)/PANGO_SCALE);
	    /* I doubt I'm supposed to free (or unref) the run? */
	}
    } while ( _pango_layout_iter_next_run(iter));
    _pango_layout_iter_free(iter);
}

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
/* Strangely the equivalent routine was not part of the pangocairo library */
/* Oh there's _pango_cairo_layout_path but that's more restrictive and probably*/
/*  less efficient */

static void my_cairo_render_layout(cairo_t *cc, Color fg,
	PangoLayout *layout,int x,int y) {
    PangoRectangle rect, r2;
    PangoLayoutIter *iter;

    iter = _pango_layout_get_iter(layout);
    do {
	PangoLayoutRun *run = _pango_layout_iter_get_run(iter);
	if ( run!=NULL ) {	/* NULL runs mark end of line */
	    _pango_layout_iter_get_run_extents(iter,&r2,&rect);
	    _cairo_move_to(cc,x+(rect.x+PANGO_SCALE/2)/PANGO_SCALE, y+(rect.y+PANGO_SCALE/2)/PANGO_SCALE);
	    if ( COLOR_ALPHA(fg)==0 )
		_cairo_set_source_rgba(cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
			1.0);
	    else
		_cairo_set_source_rgba(cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
			COLOR_ALPHA(fg)/255.);
	    _pango_cairo_show_glyph_string(cc,run->item->analysis.font,run->glyphs);
	}
    } while ( _pango_layout_iter_next_run(iter));
    _pango_layout_iter_free(iter);
}
#endif

/* ************************************************************************** */
/* ****************************** Pango Window ****************************** */
/* ************************************************************************** */
void _GXPDraw_NewWindow(GXWindow nw) {
    GXDisplay *gdisp = nw->display;

    MacVersionTest();
    if ( !usepango || !_GXPDraw_hasPango())
return;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( nw->usecairo ) {
	if ( _pango_cairo_font_map_create_context==NULL )
return;
	/* using pango through cairo is different from using it on bare X */
	if ( gdisp->pangoc_context==NULL ) {
	    gdisp->pangoc_fontmap = _pango_cairo_font_map_get_default();
	    gdisp->pangoc_context = _pango_cairo_font_map_create_context(
		    (PangoCairoFontMap *) (gdisp->pangoc_fontmap));
	    if ( _pango_cairo_context_set_resolution!=NULL )
		_pango_cairo_context_set_resolution(gdisp->pangoc_context,
			gdisp->res);
	    gdisp->pangoc_layout = _pango_layout_new(gdisp->pangoc_context);
	}
    } else
# endif
    {
	if ( gdisp->pango_context==NULL ) {
	    gdisp->pango_fontmap = _pango_xft_get_font_map(gdisp->display,gdisp->screen);
	    gdisp->pango_context = _pango_xft_get_context(gdisp->display,gdisp->screen);
	    /* No obvious way to get or set the resolution of pango_xft */
	}
	nw->xft_w = _XftDrawCreate(gdisp->display,nw->w,gdisp->visual,gdisp->cmap);
	if ( gdisp->pango_layout==NULL )
	    gdisp->pango_layout = _pango_layout_new(gdisp->pango_context);
    }
    nw->usepango = true;
return;
}

void _GXPDraw_DestroyWindow(GXWindow nw) {
    /* And why doesn't the man page mention this essential function? */
    if ( nw->usepango && _XftDrawDestroy!=NULL && nw->xft_w!=NULL ) {
	_XftDrawDestroy(nw->xft_w);
	nw->xft_w = NULL;
    }
}

/* ************************************************************************** */
/* ******************************* Pango Text ******************************* */
/* ************************************************************************** */
static PangoFontDescription *_GXPDraw_configfont(GXDisplay *gdisp, GFont *font,int pc) {
    PangoFontDescription *fd;
#ifdef _NO_LIBCAIRO
    PangoFontDescription **fdbase = &font->pango_fd;
#else
    PangoFontDescription **fdbase = pc ? &font->pangoc_fd : &font->pango_fd;
#endif

    if ( *fdbase!=NULL )
return( *fdbase );
    *fdbase = fd = _pango_font_description_new();

    if ( font->rq.utf8_family_name != NULL )
	_pango_font_description_set_family(fd,font->rq.utf8_family_name);
    else {
	char *temp = u2utf8_copy(font->rq.family_name);
	_pango_font_description_set_family(fd,temp);
	free(temp);
    }
    _pango_font_description_set_style(fd,(font->rq.style&fs_italic)?
	    PANGO_STYLE_ITALIC:
	    PANGO_STYLE_NORMAL);
    _pango_font_description_set_variant(fd,(font->rq.style&fs_smallcaps)?
	    PANGO_VARIANT_SMALL_CAPS:
	    PANGO_VARIANT_NORMAL);
    _pango_font_description_set_weight(fd,font->rq.weight);
    _pango_font_description_set_stretch(fd,
	    (font->rq.style&fs_condensed)?  PANGO_STRETCH_CONDENSED :
	    (font->rq.style&fs_extended )?  PANGO_STRETCH_EXPANDED  :
					    PANGO_STRETCH_NORMAL);

    if ( font->rq.point_size<=0 )
	GDrawIError( "Bad point size for pango" );	/* any negative (pixel) values should be converted when font opened */

    /* Pango doesn't give me any control over the resolution on X, so I do my */
    /*  own conversion from points to pixels */
    /* But under pangocairo I can set the resolution, so behavior is different*/
    /* But then, set_absolute_size doesn't exist in some versions of the */
    /*  library */
#ifndef no_pango_font_description_set_absolute_size
    if ( _pango_font_description_set_absolute_size!=NULL ) {
	_pango_font_description_set_absolute_size(fd,
		    GDrawPointsToPixels(NULL,font->rq.point_size*PANGO_SCALE));
    } else
#endif
    {
	if ( pc )				/* pango Resolution set correctly */
	    _pango_font_description_set_size(fd,font->rq.point_size*PANGO_SCALE);
	else					/* pango Resolution probably wrong */
	    _pango_font_description_set_size(fd,(font->rq.point_size*gdisp->res+gdisp->xres/2)*PANGO_SCALE/gdisp->xres);
    }
return( fd );
}

int32 _GXPDraw_DoText8(GWindow w, int32 x, int32 y,
	const char *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    struct font_instance *fi = gw->ggc->fi;
    PangoRectangle rect, ink;
    PangoLayout *layout = gdisp->pango_layout;
    PangoFontDescription *fd;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( gw->usecairo )
	layout = gdisp->pangoc_layout;
# endif
    fd = _GXPDraw_configfont(gdisp,fi,layout!=gdisp->pango_layout);
    _pango_layout_set_font_description(layout,fd);
    _pango_layout_set_text(layout,(char *) text,cnt);
    _pango_layout_get_pixel_extents(layout,NULL,&rect);
    if ( drawit==tf_drawit ) {
# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
	if ( gw->usecairo ) {
	    layout = gdisp->pangoc_layout;
	    my_cairo_render_layout(gw->cc,col,layout,x,y);
#if 0
	    _cairo_move_to(gw->cc,x,y - fi->ascent);
	    gw->ggc->fg = col;
	    GXCDrawSetcolfunc(gw,gw->ggc);
	    _pango_cairo_layout_path(gw->cc,layout);
	    _cairo_fill(gw->cc);
#endif
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
	    _XftColorAllocValue(gdisp->display,gdisp->visual,gdisp->cmap,&fgcol,&fg);
	    clip.x = gw->ggc->clip.x;
	    clip.y = gw->ggc->clip.y;
	    clip.width = gw->ggc->clip.width;
	    clip.height = gw->ggc->clip.height;
	    _XftDrawSetClipRectangles(gw->xft_w,0,0,&clip,1);
	    my_xft_render_layout(gw->xft_w,&fg,layout,x,y);
	}
    } else if ( drawit==tf_rect ) {
	PangoLayoutIter *iter;
	PangoLayoutRun *run;
	PangoFontMetrics *fm;

	_pango_layout_get_pixel_extents(layout,&ink,&rect);
	arg->size.lbearing = ink.x - rect.x;
	arg->size.rbearing = ink.x+ink.width - rect.x;
	arg->size.width = rect.width;
	if ( *text=='\0' ) {
	    /* There are no runs if there are no characters */
	    memset(&arg->size,0,sizeof(arg->size));
	} else {
	    iter = _pango_layout_get_iter(layout);
	    run = _pango_layout_iter_get_run(iter);
	    if ( run==NULL ) {
		/* Pango doesn't give us runs in a couple of other places */
		/* surrogates, not unicode (0xfffe, 0xffff), etc. */
		memset(&arg->size,0,sizeof(arg->size));
	    } else {
		fm = _pango_font_get_metrics(run->item->analysis.font,NULL);
		arg->size.fas = _pango_font_metrics_get_ascent(fm)/PANGO_SCALE;
		arg->size.fds = _pango_font_metrics_get_descent(fm)/PANGO_SCALE;
		arg->size.as = ink.y + ink.height - arg->size.fds;
		arg->size.ds = arg->size.fds - ink.y;
		if ( arg->size.ds<0 ) {
		    --arg->size.as;
		    arg->size.ds = 0;
		}
		/* In the one case I've looked at fds is one pixel off from rect.y */
		/*  I don't know what to make of that */
		_pango_font_metrics_unref(fm);
	    }
	    _pango_layout_iter_free(iter);
	}
    }
return( rect.width );
}

int32 _GXPDraw_DoText(GWindow w, int32 x, int32 y,
	const unichar_t *text, int32 cnt, FontMods *mods, Color col,
	enum text_funcs drawit, struct tf_arg *arg) {
    char *temp = cnt>=0 ? u2utf8_copyn(text,cnt) : u2utf8_copy(text);
    int width = _GXPDraw_DoText8(w,x,y,temp,-1,mods,col,drawit,arg);
    free(temp);
return(width);
}

void _GXPDraw_FontMetrics(GXWindow gw,GFont *fi,int *as, int *ds, int *ld) {
    GXDisplay *gdisp = gw->display;
    PangoFont *pfont;
    PangoFontMetrics *fm;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( gw->usecairo ) {
	_GXPDraw_configfont(gdisp,fi,true);
	pfont = _pango_font_map_load_font(gdisp->pangoc_fontmap,gdisp->pangoc_context,
		fi->pangoc_fd);
    } else
#endif
    {
	_GXPDraw_configfont(gdisp,fi,false);
	pfont = _pango_font_map_load_font(gdisp->pango_fontmap,gdisp->pango_context,
		fi->pango_fd);
    }
    fm = _pango_font_get_metrics(pfont,NULL);
    *as = _pango_font_metrics_get_ascent(fm)/PANGO_SCALE;
    *ds = _pango_font_metrics_get_descent(fm)/PANGO_SCALE;
    *ld = 0;
    _pango_font_metrics_unref(fm);
}

/* ************************************************************************** */
/* ****************************** Pango Layout ****************************** */
/* ************************************************************************** */
void _GXPDraw_LayoutInit(GWindow w, char *text, int cnt, GFont *fi) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    PangoLayout *layout = gdisp->pango_layout;
    PangoFontDescription *fd;

    if ( fi==NULL )
	fi = gw->ggc->fi;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( gw->usecairo )
	layout = gdisp->pangoc_layout;
# endif
    fd = _GXPDraw_configfont(gdisp,fi,layout!=gdisp->pango_layout);
    _pango_layout_set_font_description(layout,fd);
    _pango_layout_set_text(layout,(char *) text,cnt);
}

void _GXPDraw_LayoutDraw(GWindow w, int32 x, int32 y, Color col) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    PangoLayout *layout = gdisp->pango_layout;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( gw->usecairo ) {
	layout = gdisp->pangoc_layout;
	my_cairo_render_layout(gw->cc,col,layout,x,y);
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
	_XftColorAllocValue(gdisp->display,gdisp->visual,gdisp->cmap,&fgcol,&fg);
	clip.x = gw->ggc->clip.x;
	clip.y = gw->ggc->clip.y;
	clip.width = gw->ggc->clip.width;
	clip.height = gw->ggc->clip.height;
	_XftDrawSetClipRectangles(gw->xft_w,0,0,&clip,1);
	my_xft_render_layout(gw->xft_w,&fg,layout,x,y);
    }
}

void _GXPDraw_LayoutIndexToPos(GWindow w, int index, GRect *pos) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    PangoLayout *layout = gdisp->pango_layout;
    PangoRectangle rect;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( gw->usecairo )
	layout = gdisp->pangoc_layout;
# endif
    _pango_layout_index_to_pos(layout,index,&rect);
    pos->x = rect.x/PANGO_SCALE; pos->y = rect.y/PANGO_SCALE; pos->width = rect.width/PANGO_SCALE; pos->height = rect.height/PANGO_SCALE;
}

int _GXPDraw_LayoutXYToIndex(GWindow w, int x, int y) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    PangoLayout *layout = gdisp->pango_layout;
    int trailing, index;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( gw->usecairo )
	layout = gdisp->pangoc_layout;
# endif
    /* Pango retuns the last character if x is negative, not the first */
    if ( x<0 ) x=0;
    _pango_layout_xy_to_index(layout,x*PANGO_SCALE,y*PANGO_SCALE,&index,&trailing);
    /* If I give pango a position after the last character on a line, it */
    /*  returns to me the first character. Strange. And annoying -- you click */
    /*  at the end of a line and the cursor moves to the start */
    /* Of course in right to left text an initial position is correct... */
    if ( index+trailing==0 && x>0 ) {
	PangoRectangle rect;
	_pango_layout_get_pixel_extents(layout,&rect,NULL);
	if ( x>=rect.width ) {
	    x = rect.width-1;
	    _pango_layout_xy_to_index(layout,x*PANGO_SCALE,y*PANGO_SCALE,&index,&trailing);
	}
    }
return( index+trailing );
}

void _GXPDraw_LayoutExtents(GWindow w, GRect *size) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    PangoLayout *layout = gdisp->pango_layout;
    PangoRectangle rect;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( gw->usecairo )
	layout = gdisp->pangoc_layout;
# endif
    _pango_layout_get_pixel_extents(layout,NULL,&rect);
    size->x = rect.x; size->y = rect.y; size->width = rect.width; size->height = rect.height;
}

void _GXPDraw_LayoutSetWidth(GWindow w, int width) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    PangoLayout *layout = gdisp->pango_layout;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( gw->usecairo )
	layout = gdisp->pangoc_layout;
# endif
    _pango_layout_set_width(layout,width==-1? -1 : width*PANGO_SCALE);
}

int _GXPDraw_LayoutLineCount(GWindow w) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    PangoLayout *layout = gdisp->pango_layout;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( gw->usecairo )
	layout = gdisp->pangoc_layout;
# endif
return( _pango_layout_get_line_count(layout));
}

int _GXPDraw_LayoutLineStart(GWindow w, int l) {
    GXWindow gw = (GXWindow) w;
    GXDisplay *gdisp = gw->display;
    PangoLayout *layout = gdisp->pango_layout;
    PangoLayoutLine *line;

# if !defined(_NO_LIBCAIRO) && PANGO_VERSION_MINOR>=10
    if ( gw->usecairo )
	layout = gdisp->pangoc_layout;
# endif
    line = _pango_layout_get_line(layout,l);
    if ( line==NULL )
return( -1 );

return( line->start_index );
}
#endif	/* ! _NO_LIBPANGO */
