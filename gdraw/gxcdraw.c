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

static int usecairo = true;

void GDrawEnableCairo(int on) {
    usecairo=on;
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

# if !defined(_STATIC_LIBCAIRO) && !defined(NODYNAMIC)
#  include <dynamic.h>
static DL_CONST void *libcairo=NULL, *libfontconfig;
static cairo_surface_t *(*_cairo_xlib_surface_create)(Display *,Drawable,Visual*,int,int);
static cairo_t *(*_cairo_create)(cairo_surface_t *);
static void (*_cairo_destroy)(cairo_t *);
static void (*_cairo_surface_destroy)(cairo_surface_t *);
static void (*_cairo_xlib_surface_set_size)(cairo_surface_t *,int,int);
static void (*_cairo_set_line_width)(cairo_t *,double);
/* I don't actually use line join/cap. On a screen with small line_width they don't matter */
static void (*_cairo_set_line_cap)(cairo_t *,cairo_line_cap_t);
static void (*_cairo_set_line_join)(cairo_t *,cairo_line_join_t);
static void (*_cairo_set_source_rgba)(cairo_t *,double,double,double,double);
static void (*_cairo_set_operator)(cairo_t *,cairo_operator_t);
static void (*_cairo_set_dash)(cairo_t *,double *, int, double);
static void (*_cairo_stroke)(cairo_t *);
static void (*_cairo_fill)(cairo_t *);
static void (*_cairo_fill_preserve)(cairo_t *);
static void (*_cairo_clip)(cairo_t *);
static void (*_cairo_save)(cairo_t *);
static void (*_cairo_restore)(cairo_t *);
static void (*_cairo_new_path)(cairo_t *);
static void (*_cairo_close_path)(cairo_t *);
static void (*_cairo_move_to)(cairo_t *,double,double);
static void (*_cairo_line_to)(cairo_t *,double,double);
static void (*_cairo_curve_to)(cairo_t *,double,double,double,double,double,double);
static void (*_cairo_rectangle)(cairo_t *,double,double,double,double);
static void (*_cairo_scaled_font_extents)(cairo_scaled_font_t *,cairo_font_extents_t *);
static void (*_cairo_scaled_font_text_extents)(cairo_scaled_font_t *,const char *,cairo_text_extents_t *);
static void (*_cairo_set_scaled_font)(cairo_t *,const cairo_scaled_font_t *);
static void (*_cairo_show_text)(cairo_t *,const char *);
static cairo_scaled_font_t *(*_cairo_scaled_font_create)(cairo_font_face_t *, const cairo_matrix_t *, const cairo_matrix_t *, const cairo_font_options_t *);
static cairo_font_face_t *(*_cairo_ft_font_face_create_for_pattern)(FcPattern *);
static void (*_cairo_set_source_surface)(cairo_t *,cairo_surface_t *,double,double);
static void (*_cairo_mask_surface)(cairo_t *,cairo_surface_t *,double,double);
static void (*_cairo_scale)(cairo_t *,double,double);
static void (*_cairo_translate)(cairo_t *,double,double);
static cairo_format_t (*_cairo_image_surface_get_format)(cairo_surface_t *);
static cairo_surface_t *(*_cairo_image_surface_create_for_data)(unsigned char *,cairo_format_t,int,int,int);
static int (*_cairo_format_stride_for_width)(cairo_format_t,int);
static int (*_cairo_font_options_create)(void);
static void (*_cairo_surface_mark_dirty_rectangle)(cairo_surface_t *,double,double,double,double);
static void (*_cairo_surface_flush)(cairo_surface_t *);

static FcBool (*_FcCharSetHasChar)(const FcCharSet *,FcChar32);
static FcPattern *(*_FcPatternCreate)(void);
static void (*_FcPatternDestroy)(FcPattern *);
static FcBool (*_FcPatternAddDouble)(FcPattern *,const char *,double);
static FcBool (*_FcPatternAddInteger)(FcPattern *,const char *,int);
static FcBool (*_FcPatternAddString)(FcPattern *,const char *,const char *);
static FcPattern *(*_FcFontRenderPrepare)(FcConfig *, FcPattern *,FcPattern *);
static FcBool (*_FcConfigSubstitute)(FcConfig *, FcPattern *,FcMatchKind);
static void (*_FcDefaultSubstitute)(FcPattern *);
static FcFontSet *(*_FcFontSort)(FcConfig *,FcPattern *,FcBool,FcCharSet **,FcResult *);
static FcResult (*_FcPatternGetCharSet)(FcPattern *,const char *,int,FcCharSet **);

int _GXCDraw_hasCairo(void) {
    static int initted = false, hasC=false;
    FcBool (*_FcInit)(void);

    if ( !usecairo )
return( false );

    if ( initted )
return( hasC );

    initted = true;
    libfontconfig = dlopen("libfontconfig" SO_EXT,RTLD_LAZY);
    if ( libfontconfig==NULL ) {
	fprintf(stderr,"libfontconfig: %s\n", dlerror());
return( 0 );
    }
    _FcInit = (FcBool (*)(void)) dlsym(libfontconfig,"FcInit");
    if ( _FcInit==NULL || !_FcInit() )
return( 0 );
    _FcCharSetHasChar = (FcBool (*)(const FcCharSet *,FcChar32))
	    dlsym(libfontconfig,"FcCharSetHasChar");
    _FcPatternCreate = (FcPattern *(*)(void))
	    dlsym(libfontconfig,"FcPatternCreate");
    _FcPatternDestroy = (void (*)(FcPattern *))
	    dlsym(libfontconfig,"FcPatternDestroy");
    _FcPatternAddDouble = (FcBool (*)(FcPattern *,const char *,double))
	    dlsym(libfontconfig,"FcPatternAddDouble");
    _FcPatternAddInteger = (FcBool (*)(FcPattern *,const char *,int))
	    dlsym(libfontconfig,"FcPatternAddInteger");
    _FcPatternAddString = (FcBool (*)(FcPattern *,const char *,const char *))
	    dlsym(libfontconfig,"FcPatternAddString");
    _FcFontRenderPrepare = (FcPattern *(*)(FcConfig *, FcPattern *,FcPattern *))
	    dlsym(libfontconfig,"FcFontRenderPrepare");
    _FcConfigSubstitute = (FcBool (*)(FcConfig *, FcPattern *,FcMatchKind))
	    dlsym(libfontconfig,"FcConfigSubstitute");
    _FcDefaultSubstitute = (void (*)(FcPattern *))
	    dlsym(libfontconfig,"FcDefaultSubstitute");
    _FcFontSort = (FcFontSet *(*)(FcConfig *,FcPattern *,FcBool,FcCharSet **,FcResult *))
	    dlsym(libfontconfig,"FcFontSort");
    _FcPatternGetCharSet = (FcResult (*)(FcPattern *,const char *,int,FcCharSet **))
	    dlsym(libfontconfig,"FcPatternGetCharSet");
    if ( _FcFontSort==NULL || _FcConfigSubstitute==NULL )
return( 0 );


    libcairo = dlopen("libcairo" SO_EXT,RTLD_LAZY);
    if ( libcairo==NULL ) {
	fprintf(stderr,"libcairo: %s\n", dlerror());
return( 0 );
    }

    _cairo_xlib_surface_create = (cairo_surface_t *(*)(Display *,Drawable,Visual*,int,int))
	    dlsym(libcairo,"cairo_xlib_surface_create");
    _cairo_create = (cairo_t *(*)(cairo_surface_t *))
	    dlsym(libcairo,"cairo_create");
    _cairo_destroy = (void (*)(cairo_t *))
	    dlsym(libcairo,"cairo_destroy");
    _cairo_surface_destroy = (void (*)(cairo_surface_t *))
	    dlsym(libcairo,"cairo_surface_destroy");
    _cairo_xlib_surface_set_size = (void (*)(cairo_surface_t *,int,int))
	    dlsym(libcairo,"cairo_xlib_surface_set_size");
    _cairo_set_line_width = (void (*)(cairo_t *, double))
	    dlsym(libcairo,"cairo_set_line_width");
    _cairo_set_line_cap = (void (*)(cairo_t *, cairo_line_cap_t))
	    dlsym(libcairo,"cairo_set_line_cap");
    _cairo_set_line_join = (void (*)(cairo_t *, cairo_line_join_t))
	    dlsym(libcairo,"cairo_set_line_join");
    _cairo_set_operator = (void (*)(cairo_t *, cairo_operator_t))
	    dlsym(libcairo,"cairo_set_operator");
    _cairo_set_dash = (void (*)(cairo_t *, double *, int, double))
	    dlsym(libcairo,"cairo_set_dash");
    _cairo_set_source_rgba = (void (*)(cairo_t *,double,double,double,double))
	    dlsym(libcairo,"cairo_set_source_rgba");
    _cairo_new_path = (void (*)(cairo_t *))
	    dlsym(libcairo,"cairo_new_path");
    _cairo_move_to = (void (*)(cairo_t *,double,double))
	    dlsym(libcairo,"cairo_move_to");
    _cairo_line_to = (void (*)(cairo_t *,double,double))
	    dlsym(libcairo,"cairo_line_to");
    _cairo_curve_to = (void (*)(cairo_t *,double,double,double,double,double,double))
	    dlsym(libcairo,"cairo_curve_to");
    _cairo_rectangle = (void (*)(cairo_t *,double,double,double,double))
	    dlsym(libcairo,"cairo_rectangle");
    _cairo_close_path = (void (*)(cairo_t *))
	    dlsym(libcairo,"cairo_close_path");
    _cairo_stroke = (void (*)(cairo_t *))
	    dlsym(libcairo,"cairo_stroke");
    _cairo_fill = (void (*)(cairo_t *))
	    dlsym(libcairo,"cairo_fill");
    _cairo_fill_preserve = (void (*)(cairo_t *))
	    dlsym(libcairo,"cairo_fill_preserve");
    _cairo_clip = (void (*)(cairo_t *))
	    dlsym(libcairo,"cairo_clip");
    _cairo_save = (void (*)(cairo_t *))
	    dlsym(libcairo,"cairo_save");
    _cairo_restore = (void (*)(cairo_t *))
	    dlsym(libcairo,"cairo_restore");
    _cairo_scaled_font_extents = (void (*)(cairo_scaled_font_t *,cairo_font_extents_t *))
	    dlsym(libcairo,"cairo_scaled_font_extents");
    _cairo_scaled_font_text_extents = (void (*)(cairo_scaled_font_t *,const char *,cairo_text_extents_t *))
	    dlsym(libcairo,"cairo_scaled_font_text_extents");
    _cairo_set_scaled_font = (void (*)(cairo_t *,const cairo_scaled_font_t *))
	    dlsym(libcairo,"cairo_set_scaled_font");
    _cairo_show_text = (void (*)(cairo_t *,const char *))
	    dlsym(libcairo,"cairo_show_text");
    _cairo_scaled_font_create = (cairo_scaled_font_t *(*)(cairo_font_face_t *, const cairo_matrix_t *, const cairo_matrix_t *, const cairo_font_options_t *))
	    dlsym(libcairo,"cairo_scaled_font_create");
    _cairo_ft_font_face_create_for_pattern = (cairo_font_face_t *(*)(FcPattern *))
	    dlsym(libcairo,"cairo_ft_font_face_create_for_pattern");
    _cairo_set_source_surface = (void (*)(cairo_t *,cairo_surface_t *,double,double))
	    dlsym(libcairo,"cairo_set_source_surface");
    _cairo_mask_surface = (void (*)(cairo_t *,cairo_surface_t *,double,double))
	    dlsym(libcairo,"cairo_mask_surface");
    _cairo_scale = (void (*)(cairo_t *,double,double))
	    dlsym(libcairo,"cairo_scale");
    _cairo_translate = (void (*)(cairo_t *,double,double))
	    dlsym(libcairo,"cairo_translate");
    _cairo_image_surface_get_format = (cairo_format_t (*)(cairo_surface_t *))
	    dlsym(libcairo,"cairo_image_surface_get_format");
    _cairo_image_surface_create_for_data = (cairo_surface_t *(*)(unsigned char *,cairo_format_t,int,int,int))
	    dlsym(libcairo,"cairo_image_surface_create_for_data");
    _cairo_format_stride_for_width = (int (*)(cairo_format_t,int))
	    dlsym(libcairo,"cairo_format_stride_for_width");
    _cairo_font_options_create = (cairo_font_options_t *(*)(void))
	    dlsym(libcairo,"cairo_font_options_create");
    _cairo_surface_flush = (void (*)(cairo_surface_t *))
	    dlsym(libcairo,"cairo_surface_flush");
    _cairo_surface_mark_dirty_rectangle = (void (*)(cairo_surface_t *,double,double,double,double))
	    dlsym(libcairo,"cairo_surface_mark_dirty_rectangle");

/* Didn't show up until 1.6, and I've got 1.2 on my machine */ 
    if ( _cairo_format_stride_for_width==NULL )
	_cairo_format_stride_for_width = __cairo_format_stride_for_width;

    if ( _cairo_xlib_surface_create==NULL || _cairo_create==NULL ) {
	fprintf(stderr,"libcairo: Missing symbols\n" );
return( 0 );
    }
    if ( _cairo_scaled_font_text_extents==NULL ) {
	fprintf(stderr,"libcairo: FontForge needs at least version 1.2\n" );
return( 0 );
    }

    hasC = true;
return( true );
}
# else
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
#  define _cairo_move_to cairo_move_to
#  define _cairo_line_to cairo_line_to
#  define _cairo_curve_to cairo_curve_to
#  define _cairo_rectangle cairo_rectangle
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
# endif

/* ************************************************************************** */
/* ****************************** Cairo Window ****************************** */
/* ************************************************************************** */
void _GXCDraw_NewWindow(GXWindow nw,Color bg) {
    GXDisplay *gdisp = nw->display;
    Display *display = gdisp->display;

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
    nw->bg = bg;	/* Need this to emulate Clear */
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
static int GXCDrawSetcolfunc(GXWindow gw, GGC *mine) {
    /*GCState *gcs = &gw->cairo_state;*/
    Color fg = mine->fg;

#if 0
/* As far as I can tell, XOR doesn't work */
    if ( mine->func!=gcs->func || mine->func!=df_copy ) {
	_cairo_set_operator( gw->cc,mine->func==df_copy?CAIRO_OPERATOR_OVER:CAIRO_OPERATOR_XOR );
	gcs->func = mine->func;
    }
    if ( mine->func==df_xor )
	fg ^= mine->xor_base;
#endif
    if ( (fg>>24)==0 )
	_cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
		1.0);
    else
	_cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
		(fg>>24)/255.);
return( true );
}

static int GXCDrawSetline(GXWindow gw, GGC *mine) {
    GCState *gcs = &gw->cairo_state;
    Color fg = mine->fg;

#if 0
/* As far as I can tell, XOR doesn't work */
    if ( mine->func!=gcs->func || mine->func!=df_copy ) {
	_cairo_set_operator( gw->cc, mine->func==df_copy?CAIRO_OPERATOR_OVER:CAIRO_OPERATOR_XOR);
	gcs->func = mine->func;
    }
    if ( mine->func==df_xor )
	fg ^= mine->xor_base;
#endif
    if ( (fg>>24)==0 )
	_cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
		1.0);
    else
	_cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,
		(fg>>24)/255.0);
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
    _cairo_set_source_rgba(gw->cc,COLOR_RED(gw->bg)/255.0,COLOR_GREEN(gw->bg)/255.0,COLOR_BLUE(gw->bg)/255.0,
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
}

/* ************************************************************************** */
/* ****************************** Cairo Paths ******************************* */
/* ************************************************************************** */
void _GXCDraw_PathStartNew(GWindow w) {
    _cairo_new_path( ((GXWindow) w)->cc );
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

    if ( font->rq.style==fs_italic )
	_FcPatternAddInteger(pat,FC_SLANT,FC_SLANT_ITALIC);
    else if ( font->rq.style==fs_italic )
	_FcPatternAddInteger(pat,FC_SLANT,FC_SLANT_OBLIQUE);
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
	unichar_t *text, int32 cnt, FontMods *mods, Color col,
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
    if ( base->image_type == it_true && type == CAIRO_FORMAT_RGB24 ) {
	idata = ((uint32 *) (base->data)) + src->y*base->bytes_per_line + src->x;
	*_data = NULL;		/* We can reuse the image's own data, don't need a copy */
return( _cairo_image_surface_create_for_data((uint8 *) idata,type,
		src->width, src->height,
		base->bytes_per_line));
    }

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
	       ito[j] = clut[index];
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
	pt = base->data + src->y*base->bytes_per_line + (src->x>>3);
	ito = idata;
	for ( i=0; i<src->height; ++i ) {
	    bit = (0x80>>(src->x&0x7));
	    for ( j=jj=0; j<src->width; ++j ) {
		ito[j] = (pt[jj]&bit) ? fg : bg;
		if ( (bit>>1)==0 ) {
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
    cairo_surface_t *is;
    uint8 *data;
    GRect full;

    full.x = full.y = 0;
    full.width = base->width; full.height = base->height;
    is = GImage2Surface(image,&full,&data);

    _cairo_save(gw->cc);
    _cairo_translate(gw->cc,x,y);
    _cairo_scale(gw->cc,((double) width)/base->width,((double) height)/base->height);

    if ( _cairo_image_surface_get_format(is)==CAIRO_FORMAT_A1 ) {
	/* No color info, just alpha channel */
	Color fg = base->clut->trans_index==0 ? base->clut->clut[1] : base->clut->clut[0];
	_cairo_set_source_rgba(gw->cc,COLOR_RED(fg)/255.0,COLOR_GREEN(fg)/255.0,COLOR_BLUE(fg)/255.0,1.0);
	_cairo_mask_surface(gw->cc,is,0,0);
    } else {
	_cairo_set_source_surface(gw->cc,is,0,0);
	_cairo_rectangle(gw->cc,0,0,magsrc->width,magsrc->height);
	_cairo_fill(gw->cc);
    }
    _cairo_restore(gw->cc);

    _cairo_surface_destroy(is);
    free(data);
    gw->cairo_state.fore_col = COLOR_UNKNOWN;
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
/* Sort of like a pixmap, except in cairo terms */
static uint8 *data;
static int max_size, in_use;
static cairo_t *old_cairo;
static cairo_surface_t *old_surface;

void _GXCDraw_CairoBuffer(GWindow w,GRect *size) {
    int width, height;
    GXWindow gw = (GXWindow) w;
    cairo_surface_t *mems;
    cairo_t *cc;

    if ( ++in_use>1 )
return;

    width = size->x + size->width; height = size->y+size->height;
    if ( 4*width*height > max_size ) {
	if ( gw->pos.width<width )
	    width = gw->pos.width;
	if ( gw->pos.height<height )
	    height = gw->pos.height;
	max_size = 4*gw->pos.width*gw->pos.height;
	data = grealloc(data,max_size);
    }
    mems = _cairo_image_surface_create_for_data(data,CAIRO_FORMAT_ARGB32,
		width, height, 4*width);
    cc = _cairo_create(mems);
    _cairo_new_path(cc);
    _cairo_rectangle(cc,size->x,size->y,size->width,size->height);
    _cairo_set_source_rgba(cc,COLOR_RED(gw->bg)/255.0,COLOR_GREEN(gw->bg)/255.0,COLOR_BLUE(gw->bg)/255.0,
	    1.0);
    _cairo_fill(cc);
    old_cairo = gw->cc; old_surface = gw->cs;
    gw->cc = cc; gw->cs = mems;
}

void _GXCDraw_CairoUnbuffer(GWindow w,GRect *size) {
    GXWindow gw = (GXWindow) w;

    if ( --in_use>0 )
return;

    _cairo_set_source_surface(old_cairo,gw->cs,0,0);
    _cairo_rectangle(old_cairo,size->x,size->y,size->width,size->height);
    _cairo_fill(old_cairo);
    _cairo_set_source_rgba(old_cairo,0,0,0,0);

    _cairo_destroy(gw->cc);
    _cairo_surface_destroy(gw->cs);
    gw->cc = old_cairo;
    gw->cs = old_surface;
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
#endif
