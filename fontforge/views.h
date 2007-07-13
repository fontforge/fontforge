/* Copyright (C) 2000-2007 by George Williams */
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
#ifndef _VIEWS_H
#define _VIEWS_H

#include "splinefont.h"

#if defined(FONTFORGE_CONFIG_GTK)
# include <gtk/gtk.h>
# include <gdk/gdk.h>
#elif defined(FONTFORGE_CONFIG_GDRAW)
# include <ggadget.h>
#else
# include <ggadget.h>		/* Need GImage, gettext, etc. even if no UI */
#endif

struct gfi_data;
struct contextchaindlg;
struct statemachinedlg;

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
extern struct cvshows {
    int showfore, showback, showgrids, showhhints, showvhints, showdhints;
    int showpoints, showfilled;
    int showrulers;
    int showrounds;		/* 0=>no, 1=>auto, 2=>always */
    int showmdx, showmdy;	/* minimum distances x,y */
    int showhmetrics, showvmetrics;	/* show advance width, baseline, etc. */
    int markextrema;
    int markpoi;		/* Points of inflection */
    int showblues, showfamilyblues;
    int showanchor;
    int showcpinfo;
    int showtabs;		/* with the names of former glyphs */
    int showsidebearings;
} CVShows;

extern struct bvshows {
    int showfore, showoutline, showgrid;
    int lastpixelsize;
} BVShows;
#endif

typedef struct drect {
    real x, y;
    real width, height;
} DRect;

typedef struct pressedOn {
    int x,y;			/* screen location of the press */
    real cx, cy;		/* Translated into character space */
    SplinePoint *sp;
    unsigned int nextcp: 1;	/* Is the cursor on the "next" control point of */
    unsigned int prevcp: 1;	/*  the spline point, or the "prev" control point */
    unsigned int anysel: 1;	/* did we hit anything? */
/*    unsigned int width: 1;	/ * we're moving the width rather than a spline */
/*    unsigned int vwidth: 1;	/ * we're moving the width rather than a spline */
    unsigned int pressed: 1;
    unsigned int rubberbanding: 1;
    unsigned int rubberlining: 1;
    unsigned int transany: 1;
    unsigned int transanyrefs: 1;
    Spline *spline;
    real t;			/* location on the spline where we pressed */
    RefChar *ref;
    SplinePointList *spl;	/* containing spline or point */
    ImageList *img;
    AnchorPoint *ap;
    float ex, ey;		/* end of last rubber band rectangle */
    BasePoint constrain;	/* Point to which we constrain movement */
    BasePoint cp;		/* Original control point position */
} PressedOn;

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
/* Note: These are ordered as they are displayed in the tools palette */
enum cvtools { cvt_pointer, cvt_magnify,
	cvt_freehand, cvt_hand,
	cvt_curve, cvt_corner, cvt_tangent, cvt_pen,
	cvt_knife, cvt_ruler,
	cvt_scale, cvt_flip,
	cvt_rotate, cvt_skew,
	cvt_3d_rotate, cvt_perspective,
	cvt_rect, cvt_poly,
	cvt_elipse, cvt_star,
	cvt_minify, cvt_max=cvt_minify,
	cvt_none = -1};
#endif
enum bvtools { bvt_pointer, bvt_magnify,
	bvt_pencil, bvt_line,
	bvt_shift, bvt_hand,
	bvt_minify, bvt_eyedropper, bvt_max=bvt_eyedropper,
	bvt_setwidth, bvt_setvwidth,
	bvt_rect, bvt_filledrect,
	bvt_elipse, bvt_filledelipse,
	bvt_max2 = bvt_filledelipse,
	bvt_none = -1,
	bvt_fliph=0, bvt_flipv, bvt_rotate90cw, bvt_rotate90ccw, bvt_rotate180, bvt_skew, bvt_transmove };
enum drawmode { dm_grid, dm_back, dm_fore, dm_max };

typedef struct bvtfunc {
    enum bvtools func;
    int x,y;		/* used by skew and move */
} BVTFunc;

struct freetype_raster {
    int16 rows, cols;
    int16 as, lb;
    int16 bytes_per_row;
    int16 num_greys;
    uint8 *bitmap;
};

enum byte_types { bt_instr, bt_cnt, bt_byte, bt_wordhi, bt_wordlo, bt_impliedreturn };
struct instrdata {
    uint8 *instrs;
    int instr_cnt, max;
    uint8 *bts;
    unsigned int changed: 1;
    unsigned int in_composit: 1;
    SplineFont *sf;
    SplineChar *sc;
    uint32 tag;
    struct instrdlg *id;
    struct instrdata *next;
};

struct instrinfo {
#if defined(FONTFORGE_CONFIG_GTK)
    GtkWidget *v;
    GtkWidget *vsb;
#elif defined(FONTFORGE_CONFIG_GDRAW)
    GWindow v;
    GGadget *vsb;
#else
    int v;
#endif
    int16 sbw;
    int16 vheight, vwidth;
    int16 lheight,lpos;
    int16 lstopped;
    int16 as, fh;
    struct instrdata *instrdata;
#if defined(FONTFORGE_CONFIG_GDRAW)
    GFont *gfont;
#endif
    int isel_pos;
    unsigned int showaddr: 1;
    unsigned int showhex: 1;
    unsigned int mousedown: 1;
    void *userdata;
    void (*selection_callback)(struct instrinfo *,int ip);
    int  (*bpcheck)(struct instrinfo *,int ip);
#if defined(FONTFORGE_CONFIG_GTK)
    int  (*handle_char)(struct instrinfo *,GdkEvent *e);
#elif defined(FONTFORGE_CONFIG_GDRAW)
    int  (*handle_char)(struct instrinfo *,GEvent *e);
#endif
};

enum debug_wins { dw_registers=0x1, dw_stack=0x2, dw_storage=0x4, dw_points=0x8,
	dw_cvt=0x10, dw_raster=0x20, dw_gloss=0x40 };

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
typedef struct debugview {
    struct debugger_context *dc;	/* Local to freetype.c */
#if defined(FONTFORGE_CONFIG_GTK)
    GtkWidget *dv, *v;
    GtkWidget *regs, *stack, *storage, *points, *cvt, *raster, *gloss;	/* Order matters */
    GtkWidget *points_v;
    GtkWidget *cvtsb, *pts_vsb, *glosssb, *storagesb, *regsb, *stacksb;
#elif defined(FONTFORGE_CONFIG_GDRAW)
    GWindow dv, v;
    /* Windows for twilight points, cvt, registers, stack, storage, stack gloss */
    GWindow regs, stack, storage, points, cvt, raster, gloss;	/* order matters */
    GWindow points_v;
    GGadget *cvtsb;
    GGadget *pts_vsb;
    GGadget *glosssb;
    GGadget *storagesb;
    GGadget *regsb;
    GGadget *stacksb;
#endif
    struct instrdata id;
    struct instrinfo ii;
    int dwidth, toph;
    struct charview *cv;
    double scale;
    int pts_head, cvt_offtop, gloss_offtop, storage_offtop, stack_offtop, reg_offtop;
    int points_offtop;

    int codeSize;
    uint8 initialbytes[4];
    struct reflist { RefChar *ref; struct reflist *parent; } *active_refs;
    int last_npoints;
} DebugView;

enum dv_coderange { cr_none=0, cr_fpgm, cr_prep, cr_glyph };	/* cleverly chosen to match ttobjs.h */
#endif

#if !defined( FONTFORGE_CONFIG_NO_WINDOWING_UI ) || defined(_DEFINE_SEARCHVIEW_)
#define FORMER_MAX	10

typedef struct charview {
    SplineChar *sc;
    unsigned int showback:1;
    unsigned int showfore:1;
    unsigned int showgrids:1;
    unsigned int showhhints:1;
    unsigned int showvhints:1;
    unsigned int showdhints:1;
    unsigned int showpoints:1;
    unsigned int showfilled:1;
    unsigned int showrulers:1;
    unsigned int showrounds:2;		/* 0=>no, 1=>auto, 2=>always */
    unsigned int showmdx:1;
    unsigned int showmdy:1;
    unsigned int showhmetrics:1;
    unsigned int showvmetrics:1;
    unsigned int showblues:1;	/* 16 */
    unsigned int showfamilyblues:1;
    unsigned int showanchor:1;
    unsigned int showpointnumbers:1;
    unsigned int markextrema:1;
    unsigned int markpoi:1;
    unsigned int needsrasterize:1;		/* Rasterization (of fill or fontview) needed on mouse up */
    unsigned int recentchange:1;		/* a change happened in the grids or background. don't need to rasterize */
    unsigned int drawmode:2;
    unsigned int info_within: 1;		/* cursor is within main window */
    unsigned int back_img_out_of_date: 1;	/* Force redraw of back image pixmap */
    unsigned int cntrldown:1;
    unsigned int joinvalid:1;
    unsigned int widthsel:1;
    unsigned int vwidthsel:1;
    unsigned int inactive:1;			/* When in a search view */
    unsigned int show_ft_results: 1;	/* 32 */
    unsigned int coderange: 2;			/* For the debugger */
    unsigned int autonomous_ruler_w: 1;
    unsigned int showcpinfo: 1;
    unsigned int showtabs: 1;
    unsigned int showsidebearings: 1;
    Layer *layerheads[dm_max];
    real scale;
#if defined(FONTFORGE_CONFIG_GTK)
    GtkWidget *gw, *v;
    GtkWidget *vsb, *hsb, *mb, *tabs;
    PangoFont *small, *normal;
    GtkWindow *icon;		/* Pixmap? */
    guint pressed;		/* glib timer id */
    GtkWindow backimgs;		/* Pixmap? */
#elif defined(FONTFORGE_CONFIG_GDRAW)
    GWindow gw, v;
    GGadget *vsb, *hsb, *mb, *tabs;
    GFont *small, *normal;
    GWindow icon;
    GWindow ruler_w;
    GFont *rfont;
    GTimer *pressed;
    GWindow backimgs;
    GIC *gic;
#else
    void *gw, *v;
#endif
    int width, height;
    int xoff, yoff;
    int mbh, infoh, rulerh;
    int16 sas, sfh, nas, nfh;
    BasePoint info;
    SplinePoint *info_sp;
#if defined(FONTFORGE_CONFIG_GTK)
    GdkPoint e;					/* mouse location */
    GdkPoint olde;
#elif defined(FONTFORGE_CONFIG_GDRAW)
    GPoint e;					/* mouse location */
    GPoint olde;
#endif
    BasePoint last_c;
    BDFChar *filled;
#ifdef FONTFORGE_CONFIG_GDRAW
    GImage gi;
#endif
    struct charview *next;
    struct fontview *fv;
    int enc;
    EncMap *map_of_enc;				/* Only use for comparison against fontview's map to see if our enc be valid */
						/*  Will not be updated when fontview is reencoded */
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    PressedOn p;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    SplinePoint *lastselpt;
    /*GWindow tools, layers;*/
    int8 b1_tool, cb1_tool, b2_tool, cb2_tool;		/* Button 3 does a popup */
    int8 s1_tool, s2_tool, er_tool;			/* Bindings for wacom stylus and eraser */
    int8 showing_tool, pressed_tool, pressed_display, had_control, active_tool;
    SplinePointList *active_spl;
    SplinePoint *active_sp;
    IPoint handscroll_base;
    uint16 rfh, ras;
    BasePoint lastknife;
    struct freehand {
	struct tracedata *head, *last;	/* for the freehand tool */
	SplinePointList *current_trace;
	int ignore_wobble;		/* Ignore wiggles smaller than this */
	int skip_cnt;
    } freehand;
    enum expandedge { ee_none, ee_nw, ee_up, ee_ne, ee_right, ee_se, ee_down,
	    ee_sw, ee_left, ee_max } expandedge;
    BasePoint expandorigin;
    real expandwidth, expandheight;
    SplinePointList *active_shape;
    SplinePoint joinpos;
    SplineChar *template1, *template2;
#if HANYANG
    struct jamodisplay *jamodisplay;
#endif
    real oldwidth, oldvwidth;
#if _ModKeysAutoRepeat
# if defined(FONTFORGE_CONFIG_GTK)
# elif defined(FONTFORGE_CONFIG_GDRAW)
    GTimer *autorpt;
    int keysym, oldstate;
    int oldkeyx, oldkeyy;
    GWindow oldkeyw;
# endif
#endif
    struct searchview *searcher;	/* The sv within which this view is embedded (if it is) */
    PST *lcarets;
    int16 nearcaret;
	/* freetype results display */
    int16 ft_dpi, ft_ppem, ft_gridfitwidth, ft_depth;
    real ft_pointsize;
    SplineSet *gridfit;
    struct freetype_raster *raster, *oldraster;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    DebugView *dv;
#endif
    uint32 mmvisible;
    char *former_names[FORMER_MAX];
    int former_cnt;
    AnchorPoint *apmine, *apmatch;
    SplineChar *apsc;
} CharView;
#endif

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
typedef struct bitmapview {
    BDFChar *bc;
    BDFFont *bdf;
    struct fontview *fv;
    EncMap *map_of_enc;
    int enc;
#if defined(FONTFORGE_CONFIG_GTK)
    GtkWidget *v;
    GtkWidget *gw;
    GtkWidget *vsb, *hsb, *mb;
#elif defined(FONTFORGE_CONFIG_GDRAW)
    GWindow gw, v;
    GGadget *vsb, *hsb, *mb;
    GGadget *recalc;
    GFont *small;
#endif
    int xoff, yoff;
    int width, height;
    int infoh, mbh;
    int scale;
    real scscale;
    struct bitmapview *next;
    unsigned int showfore:1;
    unsigned int showoutline:1;
    unsigned int showgrid:1;
    unsigned int cntrldown:1;
    unsigned int recentchange:1;
    unsigned int clearing:1;
    unsigned int shades_hidden:1;
    unsigned int shades_down:1;
    /*GWindow tools, layers;*/
    int8 b1_tool, cb1_tool, b2_tool, cb2_tool;		/* Button 3 does a popup */
    int8 s1_tool, s2_tool, er_tool;			/* Bindings for wacom stylus and eraser */
    int8 showing_tool, pressed_tool, pressed_display, had_control, active_tool;
    int pressed_x, pressed_y;
    int info_x, info_y;
    int event_x, event_y;
    int16 sas, sfh;
#if _ModKeysAutoRepeat
# if defined(FONTFORGE_CONFIG_GTK)
# elif defined(FONTFORGE_CONFIG_GDRAW)
    GTimer *autorpt;
    int keysym, oldstate;
# endif
#endif
    int color;			/* for greyscale fonts (between 0,255) */
    int color_under_cursor;
} BitmapView;

struct aplist { AnchorPoint *ap; int connected_to, selected; struct aplist *next; };

typedef struct metricsview {
    struct fontview *fv;
    SplineFont *sf;
    int pixelsize;
    BDFFont *bdf;		/* We can also see metric info on a bitmap font */
    BDFFont *show;		/*  Or the rasterized version of the outline font */
#if defined(FONTFORGE_CONFIG_GTK)
    GtkWidget *gw;
    PangoFont *font;
    GtkWidget *hsb, *vsb, *mb, *text, *subtable_list;
    GtkWidget *namelab, *widthlab, *lbearinglab, *rbearinglab, *kernlab;
#elif defined(FONTFORGE_CONFIG_GDRAW)
    GWindow gw;
    GFont *font;
    GGadget *hsb, *vsb, *mb, *text, *script, *features, *subtable_list;
    GGadget *namelab, *widthlab, *lbearinglab, *rbearinglab, *kernlab;
#endif
    int16 xstart;
    int16 width, height, dwidth;
    int16 mbh,sbh;
    int16 topend;		/* y value of the end of the region containing the text field */
    int16 displayend;		/* y value of the end of the region showing filled characters */
    int16 fh, as;
    int16 cmax, clen; 
    SplineChar **chars;		/* Character input stream */
    struct opentype_str *glyphs;/* after going through the various gsub/gpos transformations */
    struct metricchar {		/* One for each glyph above */
	int16 dx, dwidth;	/* position and width of the displayed char */
	int16 dy, dheight;	/*  displayed info for vertical metrics */
	int xoff, yoff;
	int16 mx, mwidth;	/* position and width of the text underneath */
	int16 kernafter;
	unsigned int selected: 1;
#if defined(FONTFORGE_CONFIG_GTK)
	GtkWidget *width, *lbearing, *rbearing, *kern, *name;
#elif defined(FONTFORGE_CONFIG_GDRAW)
	GGadget *width, *lbearing, *rbearing, *kern, *name;
#endif
    } *perchar;
    SplineChar **sstr;		/* Character input stream */
    int16 mwidth, mbase;
    int16 glyphcnt, max;
    int16 pressed_x, pressed_y;
    int16 activeoff;
    int xoff, coff, yoff;
    struct metricsview *next;
    unsigned int right_to_left: 1;
    unsigned int pressed: 1;
    unsigned int pressedwidth: 1;
    unsigned int pressedkern: 1;
    unsigned int showgrid: 1;
    unsigned int antialias: 1;
    unsigned int vertical: 1;
    struct aplist *pressed_apl;
    int xp, yp, ap_owner;
    BasePoint ap_start;
    int cursor;
    int scale_index;
    struct lookup_subtable *cur_subtable;
    GTextInfo *scriptlangs;
} MetricsView;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

enum fv_metrics { fvm_baseline=1, fvm_origin=2, fvm_advanceat=4, fvm_advanceto=8 };
typedef struct fontview {
    EncMap *map;
    EncMap *normal;		/* If this is not NULL then we have a compacted encoding in map, and this is the original */
    SplineFont *sf;
    BDFFont *show, *filled;
#if defined(FONTFORGE_CONFIG_NO_WINDOWING_UI)
    void *gw, *v;
#elif defined(FONTFORGE_CONFIG_GTK)
    GtkWidget *gw, *v;
    PangoFont **fontset;
    GtkWidget *vsb, *mb;
    guint pressed;			/* gtk timer id */
    GdkGC *gc;
#elif defined(FONTFORGE_CONFIG_GDRAW)
    GWindow gw, v;
    GFont **fontset;
    GGadget *vsb, *mb;
    GTimer *pressed;
    GTimer *resize;
    GEvent resize_event;
    GIC *gic;
#endif
    int width, height;		/* of v */
    int16 infoh,mbh;
    int16 lab_height, lab_as;
    int16 colcnt, rowcnt;		/* of display window */
    int32 rowoff, rowltot;		/* Can be really big in full unicode */
    int16 cbw,cbh;			/* width/height of a character box */
    struct fontview *next;		/* Next on list of open fontviews */
    struct fontview *nextsame;		/* Next fv looking at this font */
    int pressed_pos, end_pos;
    uint8 *selected;
    unsigned int antialias:1;
    unsigned int bbsized:1;		/* displayed bitmap should be scaled by bounding box rather than emsize */
    unsigned int wasonlybitmaps:1;
    unsigned int refstate: 3;	/* 0x1 => paste orig of all non exist refs, 0x2=>don't, 0x3 => don't warn about non-exist refs with no source font */
    unsigned int touched: 1;
    unsigned int showhmetrics: 4;
    unsigned int showvmetrics: 4;
    unsigned int drag_and_drop: 1;
    unsigned int has_dd_no_cursor: 1;
    unsigned int any_dd_events_sent: 1;
    unsigned int resize_expected: 1;
	/* Some window managers do not honour my resize requests (if window is*/
	/*  maximized for example), but we depend on the resize request to    */
	/*  fix up the window. We do get a configure notify, but the window   */
	/*  stays the same size, so kludge things */
    unsigned int glyphlabel: 2;
    int16 magnify;
    int16 user_requested_magnify;
    SplineFont *cidmaster;
    int32 *mapping;	/* an array mapping grid cells (0=upper left) to font indeces (enc, 0=NUL) */
		    /* So the default array would contain NUL, ^A, ^B, ... */
    int mapcnt;		/* Number of chars in the current group (mapping) */
    struct dictionary *fontvars;	/* Scripting */
    struct searchview *sv;
    SplineChar *sc_near_top;
    int sel_index;
    struct lookup_subtable *cur_subtable;
#ifndef _NO_PYTHON
    void *python_fv_object;
    void *python_data;
#endif
} FontView;

typedef struct findsel {
#if defined(FONTFORGE_CONFIG_GTK)
    GdkEvent *e;
#elif defined(FONTFORGE_CONFIG_GDRAW)
    GEvent *e;
#endif
    real fudge;		/* One pixel fudge factor */
    real xl,xh, yl, yh;	/* One pixel fudge factor */
    unsigned int select_controls: 1;	/* notice control points */
    unsigned int seek_controls: 1;	/* notice control points before base points */
    unsigned int all_controls: 1;	/* notice control points even if the base points aren't selected (in truetype point numbering mode where all cps are visible) */
    real scale;
    PressedOn *p;
} FindSel;

#if !defined( FONTFORGE_CONFIG_NO_WINDOWING_UI ) || defined(_DEFINE_SEARCHVIEW_)
typedef struct searchview {
    FontView dummy_fv;
    SplineFont dummy_sf;
    SplineChar sc_srch, sc_rpl;
    SplineChar *chars[2];
    EncMap dummy_map;
    int32 map[2], backmap[2];
    uint8 sel[2];
    CharView cv_srch, cv_rpl;
    CharView *lastcv;
/* ****** */
#if defined(FONTFORGE_CONFIG_GTK)
    GtkWidget *gw;
    GtkWidget *mb;
    PangoFont *plain, *bold;
#elif defined(FONTFORGE_CONFIG_GDRAW)
    GWindow gw;
    GGadget *mb;
    GFont *plain, *bold;
#else
    void *gw;
#endif
    int mbh;
    int fh, as;
    int rpl_x, cv_y;
    int cv_width, cv_height;
    short button_height, button_width;
/* ****** */
    FontView *fv;
    SplineChar *curchar;
    SplineSet *path, *revpath, *replacepath, *revreplace;
    int pointcnt, rpointcnt;
    real fudge;
    real fudge_percent;			/* a value of .05 here represents 5% (we don't store the integer) */
    unsigned int tryreverse: 1;
    unsigned int tryflips: 1;
    unsigned int tryrotate: 1;
    unsigned int tryscale: 1;
    unsigned int onlyselected: 1;
    unsigned int subpatternsearch: 1;
    unsigned int doreplace: 1;
    unsigned int replaceall: 1;
    unsigned int findall: 1;
    unsigned int searchback: 1;
    unsigned int wrap: 1;
    unsigned int wasreversed: 1;
    unsigned int isvisible: 1;
    unsigned int findenabled: 1;
    unsigned int rplallenabled: 1;
    unsigned int rplenabled: 1;
    unsigned int showsfindnext: 1;
    unsigned int replacewithref: 1;
    SplineSet *matched_spl;
    SplinePoint *matched_sp, *last_sp;
    real matched_rot, matched_scale;
    real matched_x, matched_y;
    double matched_co, matched_si;		/* Precomputed sin, cos */
    enum flipset { flip_none = 0, flip_x, flip_y, flip_xy } matched_flip;
#ifdef _HAS_LONGLONG
    unsigned long long matched_refs;	/* Bit map of which refs in the char were matched */
    unsigned long long matched_ss;	/* Bit map of which splines in the char were matched */
				    /* In multi-path mode */
    unsigned long long matched_ss_start;/* Bit map of which splines we tried to start matches with */
#else
    unsigned long matched_refs;
    unsigned long matched_ss;
    unsigned long matched_ss_start;
#endif
} SearchView;
#endif

struct lkdata {
    int cnt, max;
    int off_top, off_left;
    struct lkinfo {
	OTLookup *lookup;
	unsigned int open: 1;
	unsigned int deleted: 1;
	unsigned int new: 1;
	unsigned int selected: 1;
	int16 subtable_cnt, subtable_max;
	struct lksubinfo {
	    struct lookup_subtable *subtable;
	    unsigned int deleted: 1;
	    unsigned int new: 1;
	    unsigned int selected: 1;
	} *subtables;
    } *all;
};

struct gfi_data {
    SplineFont *sf;
    GWindow gw;
    int tn_active;
    int private_aspect, ttfv_aspect, tn_aspect, tx_aspect, unicode_aspect;
    int old_sel, old_aspect, old_lang, old_strid;
    int ttf_set, names_set, tex_set;
    struct psdict *private;
    int langlocalecode;	/* MS code for the current locale */
    unsigned int family_untitled: 1;
    unsigned int human_untitled: 1;
    unsigned int done: 1;
    unsigned int mpdone: 1;
    struct anchor_shows { CharView *cv; SplineChar *sc; int restart; } anchor_shows[2];
    struct texdata texdata;
    struct contextchaindlg *ccd;
    struct statemachinedlg *smd;
/* For GDEF Mark Attachment Class -- used in lookup flags */
/* As usual, class 0 is unused */
    int mark_class_cnt;
    char **mark_classes;		/* glyph name list */
    char **mark_class_names;		/* used within ff */
    struct markclassdlg *mcd;
    GFont *font;
    int as, fh;
    struct lkdata tables[2];
    int lkwidth, lkheight;
};

enum widthtype { wt_width, wt_lbearing, wt_rbearing, wt_vwidth };

enum fvtrans_flags { fvt_dobackground=1, fvt_round_to_int=2,
	fvt_dontsetwidth=4, fvt_dontmovewidth=8, fvt_scalekernclasses=0x10,
	fvt_scalepstpos=0x20, fvt_dogrid=0x40, fvt_partialreftrans=0x80 };

enum origins { or_zero, or_center, or_lastpress, or_value, or_undefined };
struct pov_data {
    enum origins xorigin, yorigin;
    double x, y, z;
    double direction;		/* Direction of gaze projected into xy plane */
    double tilt;		/* Angle which drawing plane is tilted with respect to projection plane */
    double d;			/* Distance to projection plane */
    double sintilt;		/* Used internally */
};

extern FontView *_FontViewCreate(SplineFont *sf);
extern FontView *FontViewCreate(SplineFont *sf);
extern void SplineFontSetUnChanged(SplineFont *sf);
extern void FontViewFree(FontView *fv);
extern void FVToggleCharChanged(SplineChar *sc);
extern void FVMarkHintsOutOfDate(SplineChar *sc);
extern void FVRefreshChar(FontView *fv,int gid);
extern int _FVMenuSave(FontView *fv);
extern int _FVMenuSaveAs(FontView *fv);
extern int _FVMenuGenerate(FontView *fv,int family);
extern void _FVCloseWindows(FontView *fv);
extern void SCClearBackground(SplineChar *sc);
extern char *GetPostscriptFontName(char *defdir,int mult);
extern void MergeKernInfo(SplineFont *sf,EncMap *map);
extern void _FVSimplify(FontView *fv,struct simplifyinfo *smpl);
#ifdef FONTFORGE_CONFIG_WRITE_PFM
extern int WritePfmFile(char *filename,SplineFont *sf, int type0, EncMap *map);
#endif
extern int GenerateScript(SplineFont *sf,char *filename,char *bitmaptype,
	int fmflags,int res, char *subfontdirectory,struct sflist *sfs,
	EncMap *map,NameList *rename_to);
extern int SFGenerateFont(SplineFont *sf,int family,EncMap *map);

extern int SFScaleToEm(SplineFont *sf, int ascent, int descent);
extern void TransHints(StemInfo *stem,real mul1, real off1, real mul2, real off2, int round_to_int );
extern void VrTrans(struct vr *vr,real transform[6]);
extern void FVTransFunc(void *_fv,real transform[6],int otype, BVTFunc *bvts,
	enum fvtrans_flags );
extern void FVTrans(FontView *fv,SplineChar *sc,real transform[6],uint8 *sel,
	enum fvtrans_flags);
extern int SFNLTrans(FontView *fv,char *x_expr,char *y_expr);
extern void NonLinearDlg(FontView *fv,struct charview *cv);
extern void FVPointOfView(FontView *fv,struct pov_data *);
extern void FVBuildAccent(FontView *fv,int onlyaccents);
extern void FVBuildDuplicate(FontView *fv);
extern void FVChangeChar(FontView *fv,int encoding);
extern void SCClearContents(SplineChar *sc);
extern void SCClearAll(SplineChar *sc);
extern void BCClearAll(BDFChar *bc);
extern void UnlinkThisReference(FontView *fv,SplineChar *sc);
extern void FVFakeMenus(FontView *fv,int cmd);
extern void FVMetricsCenter(FontView *fv,int docenter);
extern void MergeFont(FontView *fv,SplineFont *other);
extern void FVMergeFonts(FontView *fv);
SplineSet *SplineSetsInterpolate(SplineSet *base, SplineSet *other, real amount, SplineChar *sc);
SplineChar *SplineCharInterpolate(SplineChar *base, SplineChar *other, real amount);
extern SplineFont *InterpolateFont(SplineFont *base, SplineFont *other, real amount, Encoding *enc);
extern void FVInterpolateFonts(FontView *fv);
extern void FVRevert(FontView *fv);
extern void FVRevertBackup(FontView *fv);

extern void FVDeselectAll(FontView *fv);

extern void FVAutoKern(FontView *fv);
extern void FVAutoWidth(FontView *fv);
extern void FVRemoveKerns(FontView *fv);
extern void FVRemoveVKerns(FontView *fv);
extern void FVVKernFromHKern(FontView *fv);
extern int AutoWidthScript(FontView *fv,int spacing);
extern int AutoKernScript(FontView *fv,int spacing, int threshold,
	struct lookup_subtable *sub, char *kernfile);

enum fvformats { fv_bdf, fv_ttf, fv_pk, fv_pcf, fv_mac, fv_win, fv_palm,
	fv_image, fv_imgtemplate, fv_eps, fv_epstemplate,
	fv_svg, fv_svgtemplate,
	fv_glif, fv_gliftemplate,
	fv_fig };
extern int HasSVG(void);
extern void SCImportSVG(SplineChar *sc,int layer,char *path,char  *memory, int memlen,int doclear);
extern int HasUFO(void);
extern void SCImportGlif(SplineChar *sc,int layer,char *path,char  *memory, int memlen,int doclear);
extern void SCImportPS(SplineChar *sc,int layer,char *path,int doclear, int flags);
extern void SCImportPSFile(SplineChar *sc,int layer,FILE *ps,int doclear,int flags);
extern void SCAddScaleImage(SplineChar *sc,GImage *image,int doclear,int layer);
extern void SCInsertImage(SplineChar *sc,GImage *image,real scale,real yoff, real xoff, int layer);
extern int FVImportImages(FontView *fv,char *path,int isimage,int toback,int flags);
extern int FVImportImageTemplate(FontView *fv,char *path,int isimage,int toback,int flags);

extern int _ExportPDF(FILE *pdf,SplineChar *sc);
extern int _ExportEPS(FILE *eps,SplineChar *sc,int gen_preview);
extern int _ExportSVG(FILE *svg,SplineChar *sc);
extern int _ExportGlif(FILE *glif,SplineChar *sc);
extern int ExportImage(char *filename,SplineChar *sc, int format, int pixelsize, int bitsperpixel);
extern void ScriptExport(SplineFont *sf, BDFFont *bdf, int format, int gid,
	char *format_spec, EncMap *map);

extern void BCFlattenFloat(BDFChar *bc);
extern void BCTrans(BDFFont *bdf,BDFChar *bc,BVTFunc *bvts,FontView *fv );

extern enum undotype CopyUndoType(void);
extern int CopyContainsSomething(void);
extern int CopyContainsBitmap(void);
extern const Undoes *CopyBufferGet(void);
extern RefChar *CopyContainsRef(SplineFont *);
extern char **CopyGetPosSubData(enum possub_type *type,SplineFont **copied_from,
	int pst_depth);
extern void CopyReference(SplineChar *sc);
extern void SCCopyLookupData(SplineChar *sc);
extern void PasteRemoveSFAnchors(SplineFont *);
extern void PasteAnchorClassMerge(SplineFont *sf,AnchorClass *into,AnchorClass *from);
extern void PasteRemoveAnchorClass(SplineFont *sf,AnchorClass *dying);
extern void ClipboardClear(void);
extern SplineSet *ClipBoardToSplineSet(void);
extern void BCCopySelected(BDFChar *bc,int pixelsize,int depth);
extern void PasteToBC(BDFChar *bc,int pixelsize,int depth,FontView *fv);
extern void FVCopyWidth(FontView *fv,enum undotype ut);
extern void FVCopyAnchors(FontView *fv);
enum fvcopy_type { ct_fullcopy, ct_reference, ct_lookups, ct_unlinkrefs };
extern void FVCopy(FontView *fv, enum fvcopy_type copytype);
extern void PasteIntoFV(FontView *fv, int pasteinto, real trans[6]);

extern void SCUpdateAll(SplineChar *sc);
extern void SCOutOfDateBackground(SplineChar *sc);
extern void SCClearSelPt(SplineChar *sc);
extern void _SCCharChangedUpdate(SplineChar *sc,int changed);
extern void SCCharChangedUpdate(SplineChar *sc);
extern void SCHintsChanged(SplineChar *sc);
extern void SCSynchronizeWidth(SplineChar *sc,real newwidth, real oldwidth,FontView *fv);
extern RefChar *HasUseMyMetrics(SplineChar *sc);
extern void SCSynchronizeLBearing(SplineChar *sc,real off);
extern void BackgroundImageTransform(SplineChar *sc, ImageList *img,real transform[6]);
extern void SCClearRounds(SplineChar *sc);
extern void SCMoreLayers(SplineChar *,Layer *old);
extern void SCLayersChange(SplineChar *sc);
extern void SFLayerChange(SplineFont *sf);
extern void SCTile(SplineChar *sc);

extern void ExecuteScriptFile(FontView *fv, SplineChar *sc, char *filename);
extern void DictionaryFree(struct dictionary *dica);

extern void BCCharChangedUpdate(BDFChar *bc);
extern BDFFloat *BDFFloatCreate(BDFChar *bc,int xmin,int xmax,int ymin,int ymax, int clear);
extern BDFFloat *BDFFloatCopy(BDFFloat *sel);
extern BDFFloat *BDFFloatConvert(BDFFloat *sel,int newdepth, int olddepth);
extern void BDFFloatFree(BDFFloat *sel);

extern void SCDoRedo(SplineChar *sc,int layer);
extern void SCDoUndo(SplineChar *sc,int layer);
extern void SCCopyWidth(SplineChar *sc,enum undotype);
extern void SCAppendPosSub(SplineChar *sc,enum possub_type type, char **d,SplineFont *copied_from);
#if !defined( FONTFORGE_CONFIG_NO_WINDOWING_UI ) || defined(_DEFINE_SEARCHVIEW_)
extern void PasteToCV(CharView *cv);
#endif

extern void MDReplace(MinimumDistance *md,SplineSet *old,SplineSet *rpl);

extern void _SCAutoTrace(SplineChar *sc, char **args);
extern char **AutoTraceArgs(int ask);
extern void FVAutoTrace(FontView *fv,int ask);
extern char *FindAutoTraceName(void);
extern char *FindMFName(void);
extern char *ProgramExists(char *prog,char *buffer);
extern void *GetAutoTraceArgs(void);
extern void SetAutoTraceArgs(void *a);
extern void MfArgsInit(void);

extern void DumpPfaEditEncodings(void);
extern void ParseEncodingFile(char *filename);
extern void LoadPfaEditEncodings(void);

extern void SCStroke(SplineChar *sc);
extern void FVOutline(FontView *fv, real width);
extern void FVInline(FontView *fv, real width, real inset);
extern void FVShadow(FontView *fv,real angle, real outline_width,
	real shadow_length,int wireframe);

extern void SetDefaults(void);
extern char *getPfaEditShareDir(void);
extern void LoadPrefs(void);
extern void PrefDefaultEncoding(void);
extern void _SavePrefs(void);
extern void SavePrefs(void);
extern void PfaEditSetFallback(void);
extern void RecentFilesRemember(char *filename);
extern int GetPrefs(char *name,Val *val);		/* for scripting */
extern int SetPrefs(char *name,Val *val1, Val *val2);	/* for scripting */

extern int FVImportBDF(FontView *fv, char *filename,int ispk, int toback);
extern int FVImportMult(FontView *fv, char *filename,int toback,int bf);

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
struct debugger_context;
extern void DebuggerTerminate(struct debugger_context *dc);
extern void DebuggerReset(struct debugger_context *dc,real pointsize,int dpi,int dbg_fpgm, int is_bitmap);
extern struct debugger_context *DebuggerCreate(SplineChar *sc,real pointsize,int dpi,int dbg_fpgm, int is_bitmap);
enum debug_gotype { dgt_continue, dgt_step, dgt_next, dgt_stepout };
extern void DebuggerGo(struct debugger_context *dc,enum debug_gotype,DebugView *dv);
extern struct  TT_ExecContextRec_ *DebuggerGetEContext(struct debugger_context *dc);
extern void DebuggerToggleBp(struct debugger_context *dc,int range,int ip);
extern int DebuggerBpCheck(struct debugger_context *dc,int range,int ip);
extern void DebuggerSetWatches(struct debugger_context *dc,int n, uint8 *w);
extern uint8 *DebuggerGetWatches(struct debugger_context *dc, int *n);
extern void DebuggerSetWatchStores(struct debugger_context *dc,int n, uint8 *w);
extern uint8 *DebuggerGetWatchStores(struct debugger_context *dc, int *n);
extern int DebuggerIsStorageSet(struct debugger_context *dc, int index);
extern void DebuggerSetWatchCvts(struct debugger_context *dc,int n, uint8 *w);
extern uint8 *DebuggerGetWatchCvts(struct debugger_context *dc, int *n);
extern int DebuggingFpgm(struct debugger_context *dc);
#endif

extern int BitmapControl(FontView *fv,int32 *sizes,int isavail,int rasterize);

#if defined(FONTFORGE_CONFIG_GTK)
extern void ScriptPrint(FontView *fv,int type,int32 *pointsizes,char *samplefile,
	char *sample, char *outputfile);
extern char *PrtBuildDef( SplineFont *sf, int istwobyte,
	void (*langsyscallback)(void *tf, int end, uint32 script, uint32 lang), void *tf );
#elif defined(FONTFORGE_CONFIG_GDRAW)
extern void ScriptPrint(FontView *fv,int type,int32 *pointsizes,char *samplefile,
	unichar_t *sample, char *outputfile);
extern unichar_t *PrtBuildDef( SplineFont *sf, int istwobyte, void *tf,
	void (*langsyscallback)(void *tf, int end, uint32 script, uint32 lang) );
#endif

extern char *Kern2Text(SplineChar *other,KernPair *kp,int isv);
extern char *PST2Text(PST *pst,SplineFont *sf);


extern void FVStrokeItScript(FontView *fv, StrokeInfo *si);

struct lcg_zones {
    /* everything abvoe this should be moved down (default xheight/2) */
    int top_zone;
    /* everything below this should be moved up (default xheight/2) */
    /* anything in between should be stationary */
    int bottom_zone;

    SplineSet *(*embolden_hook)(SplineSet *,struct lcg_zones *,SplineSet *);
    void (*embolden_width)(SplineChar *sc, struct lcg_zones *);

    double stroke_width;
    uint8 removeoverlap;
};
/* This order is the same order as the radio buttons in the embolden dlg */
enum embolden_type { embolden_lcg, embolden_cjk, embolden_auto, embolden_custom };
void FVEmbolden(FontView *fv,enum embolden_type type,struct lcg_zones *zones);
void EmboldenDlg(FontView *fv, CharView *cv);

extern int FVParseSelectByPST(FontView *fv,struct lookup_subtable *sub,
	int search_type);
#if defined(FONTFORGE_CONFIG_GTK)
extern void DropChars2Text(GdkWindow gw, GtkWidget *glyphs,GdkEvent *event);
#elif defined(FONTFORGE_CONFIG_GDRAW)
extern void DropChars2Text(GWindow gw, GGadget *glyphs,GEvent *event);
#endif

extern void FVSetWidthScript(FontView *fv,enum widthtype wtype,int val,int incr);

extern void FVReplaceOutlineWithReference( FontView *fv, double fudge );

extern void skewselect(BVTFunc *bvtf,real t);

extern int UserFeaturesDiffer(void);

extern int  SLICount(SplineFont *sf);
#if defined(FONTFORGE_CONFIG_GTK)
extern char *ClassName(const char *name,uint32 feature_tag,
	uint16 flags, int script_lang_index, int merge_with, int act_type,
	int macfeature,SplineFont *sf);
extern char *DecomposeClassName(const char *clsnm, char **name,
	uint32 *feature_tag, int *macfeature,
	uint16 *flags, uint16 *script_lang_index,int *merge_with,int *act_type,
	SplineFont *sf);
#elif defined(FONTFORGE_CONFIG_GDRAW)
extern unichar_t *ClassName(const char *name,uint32 feature_tag,
	uint16 flags, int script_lang_index, int merge_with, int act_type,
	int macfeature,SplineFont *sf);
extern unichar_t *DecomposeClassName(const unichar_t *clsnm, unichar_t **name,
	uint32 *feature_tag, int *macfeature,
	uint16 *flags, uint16 *script_lang_index,int *merge_with,int *act_type,
	SplineFont *sf);
#endif
extern PST *AddSubs(PST *last,uint32 tag,char *name,uint16 flags,
	uint16 sli,SplineChar *sc);


#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
extern void FVSetTitle(FontView *fv);
extern void FVScrollToChar(FontView *fv,int i);
extern void FVRegenChar(FontView *fv,SplineChar *sc);
extern FontView *FontNew(void);
extern FontView *ViewPostscriptFont(char *filename);
#if defined(FONTFORGE_CONFIG_GTK)
    /* Many of these are defined in interface.h */
extern char *AskNameTag(char *title,char *def,uint32 def_tag,uint16 flags,
	int script_lang_index, enum possub_type type, SplineFont *sf, SplineChar *default_script,
	int merge_with,int act_type);
#elif defined(FONTFORGE_CONFIG_GDRAW)
extern void _MenuWarnings(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void MenuPrefs(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuSaveAll(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuExit(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuOpen(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuHelp(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuIndex(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuAbout(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuLicense(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuNew(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void WindowMenuBuild(GWindow base,struct gmenuitem *mi,GEvent *);
extern void MenuRecentBuild(GWindow base,struct gmenuitem *mi,GEvent *);
extern void MenuScriptsBuild(GWindow base,struct gmenuitem *mi,GEvent *);
extern void mb2DoGetText(GMenuItem2 *mb);
extern void mbDoGetText(GMenuItem *mb);
extern int RecentFilesAny(void);
extern void _aplistbuild(struct gmenuitem *mi,SplineFont *sf,
	void (*func)(GWindow,struct gmenuitem *,GEvent *));
extern int32 *ParseBitmapSizes(GGadget *g,char *msg,int *err);
extern GTextInfo *AddMacFeatures(GTextInfo *opentype,enum possub_type type,SplineFont *sf);
extern unichar_t *AskNameTag(char *title,unichar_t *def,uint32 def_tag,uint16 flags,
	int script_lang_index, enum possub_type type, SplineFont *sf, SplineChar *default_script,
	int merge_with,int act_type);
extern unichar_t *ShowScripts(unichar_t *usedef);
extern GTextInfo *SFLangList(SplineFont *sf,int addfinal,SplineChar *default_script);
extern GTextInfo **SFLangArray(SplineFont *sf,int addfinal);
extern int  ScriptLangList(SplineFont *sf,GGadget *list,int sli);
extern void GListDelSelected(GGadget *list);
extern void GListMoveSelected(GGadget *list,int offset);
extern GTextInfo *GListChangeLine(GGadget *list,int pos, const unichar_t *line);
extern GTextInfo *GListAppendLine(GGadget *list,const unichar_t *line,int select);
extern GTextInfo *GListChangeLine8(GGadget *list,int pos, const char *line);
extern GTextInfo *GListAppendLine8(GGadget *list,const char *line,int select);
#endif
extern void CharInfoInit(void);
extern int DeviceTableOK(char *dvstr, int *_low, int *_high);
extern DeviceTable *DeviceTableParse(DeviceTable *dv,char *dvstr);
extern void VRDevTabParse(struct vr *vr,struct matrix_data *md);
extern void DevTabToString(char **str,DeviceTable *adjust);
extern void ValDevTabToStrings(struct matrix_data *mds,int first_offset,ValDevTab *adjust);
extern void KpMDParse(SplineFont *sf,SplineChar *sc,struct lookup_subtable *sub,
	struct matrix_data *possub,int rows,int cols,int i);
extern void GFI_LookupEnableButtons(struct gfi_data *gfi, int isgpos);
extern void GFI_LookupScrollbars(struct gfi_data *gfi, int isgpos, int refresh);
extern void FontInfo(SplineFont *sf,int aspect,int sync);
extern void FontInfoDestroy(SplineFont *sf);
extern void FontMenuFontInfo(void *fv);
extern void GFI_CCDEnd(struct gfi_data *d);
extern struct enc *MakeEncoding(SplineFont *sf, EncMap *map);
extern void LoadEncodingFile(void);
extern void RemoveEncoding(void);
extern void SFPrivateInfo(SplineFont *sf);
extern void FontViewReformatAll(SplineFont *sf);
extern void FontViewReformatOne(FontView *fv);
extern void FVShowFilled(FontView *fv);
extern void FVChangeDisplayBitmap(FontView *fv,BDFFont *bdf);
extern void FVDelay(FontView *fv,void (*func)(FontView *));
#if defined(FONTFORGE_CONFIG_GTK)
#elif defined(FONTFORGE_CONFIG_GDRAW)
extern void GFI_FinishContextNew(struct gfi_data *d,FPST *fpst, int success);
extern void SCPreparePopup(GWindow gw,SplineChar *sc, struct remap *remap, int enc, int actualuni);
extern void CVDrawSplineSet(CharView *cv, GWindow pixmap, SplinePointList *set,
	Color fg, int dopoints, DRect *clip );
extern GWindow CVMakeTools(CharView *cv);
extern GWindow CVMakeLayers(CharView *cv);
extern GWindow BVMakeTools(BitmapView *bv);
extern GWindow BVMakeLayers(BitmapView *bv);
extern int CVPaletteMnemonicCheck(GEvent *event);
extern int TrueCharState(GEvent *event);
extern void CVToolsPopup(CharView *cv, GEvent *event);
extern void BVToolsPopup(BitmapView *bv, GEvent *event);
#endif
extern real CVRoundRectRadius(void);
extern int CVRectElipseCenter(void);
extern void CVRectEllipsePosDlg(CharView *cv);
extern real CVStarRatio(void);
extern int CVPolyStarPoints(void);
extern StrokeInfo *CVFreeHandInfo(void);
extern void BVToolsSetCursor(BitmapView *bv, int state,char *device);
extern void CVToolsSetCursor(CharView *cv, int state,char *device);
extern int CVPaletteIsVisible(CharView *cv,int which);
extern void CVPaletteSetVisible(CharView *cv,int which,int visible);
extern void CVPalettesRaise(CharView *cv);
extern void CVLayersSet(CharView *cv);
extern void _CVPaletteActivate(CharView *cv,int force);
extern void CVPaletteActivate(CharView *cv);
extern void CVPalettesHideIfMine(CharView *cv);
extern int BVPaletteIsVisible(BitmapView *bv,int which);
extern void BVPaletteSetVisible(BitmapView *bv,int which,int visible);
extern void BVPaletteActivate(BitmapView *bv);
extern void BVPalettesHideIfMine(BitmapView *bv);
extern void BVPaletteColorChange(BitmapView *bv);
extern void BVPaletteColorUnderChange(BitmapView *bv,int color);
extern void BVPaletteChangedChar(BitmapView *bv);
extern void CVPaletteDeactivate(void);
extern void PalettesChangeDocking(void);
extern int CVPalettesWidth(void);
extern int BVPalettesWidth(void);

extern void CVDoTransform(CharView *cv, enum cvtools cvt );
extern void CVTransFunc(CharView *cv,real transform[6],enum fvtrans_flags);
extern void TransformDlgCreate(void *data,void (*transfunc)(void *,real *,int,BVTFunc *,enum fvtrans_flags),
	int (*getorigin)(void *,BasePoint *,int), int enableback,
	enum cvtools cvt);
extern void BitmapDlg(FontView *fv,SplineChar *sc, int isavail);
extern int SimplifyDlg(SplineFont *sf,struct simplifyinfo *smpl);
extern void CVReviewHints(CharView *cv);
extern void CVCreateHint(CharView *cv,int ishstem,int preserveundoes);
extern void SCRemoveSelectedMinimumDistances(SplineChar *sc,int inx);
extern int CVExport(CharView *cv);
extern int BVExport(BitmapView *bv);

#if defined(FONTFORGE_CONFIG_GTK)
#elif defined(FONTFORGE_CONFIG_GDRAW)
extern void DrawAnchorPoint(GWindow pixmap,int x, int y,int selected);
extern void DefaultY(GRect *pos);
extern void CVDrawRubberRect(GWindow pixmap, CharView *cv);
extern void CVInfoDraw(CharView *cv, GWindow pixmap );
extern void CVChar(CharView *cv, GEvent *event );
extern void PI_ShowHints(SplineChar *sc, GGadget *list, int set);
extern GTextInfo *SCHintList(SplineChar *sc,HintMask *);
#endif
extern void CVResize(CharView *cv );
extern CharView *CharViewCreate(SplineChar *sc,FontView *fv,int enc);
extern void CharViewFree(CharView *cv);
extern int CVValid(SplineFont *sf, SplineChar *sc, CharView *cv);
extern void CVSetCharChanged(CharView *cv,int changed);
extern void _CVCharChangedUpdate(CharView *cv,int changed);
extern void CVCharChangedUpdate(CharView *cv);
extern int CVAnySel(CharView *cv, int *anyp, int *anyr, int *anyi, int *anya);
extern SplinePoint *CVAnySelPoints(CharView *cv);
extern void CVSelectPointAt(CharView *cv);
extern int CVTwoForePointsSelected(CharView *cv, SplinePoint **sp1, SplinePoint **sp2);
extern int CVIsDiagonalable(SplinePoint *sp1, SplinePoint *sp2, SplinePoint **sp3, SplinePoint **sp4);
extern int CVClearSel(CharView *cv);
extern int CVSetSel(CharView *cv,int mask);
extern void CVInvertSel(CharView *cv);
extern int CVAllSelected(CharView *cv);
extern SplinePointList *CVAnySelPointList(CharView *cv);
extern SplinePoint *CVAnySelPoint(CharView *cv);
extern int CVOneThingSel(CharView *cv, SplinePoint **sp, SplinePointList **spl,
	RefChar **ref, ImageList **img, AnchorPoint **ap);
extern int CVOneContourSel(CharView *cv, SplinePointList **_spl,
	RefChar **ref, ImageList **img);
extern void RevertedGlyphReferenceFixup(SplineChar *sc, SplineFont *sf);
extern void CVImport(CharView *cv);
extern void BVImport(BitmapView *bv);
extern void FVImport(FontView *bv);
extern void CVFindCenter(CharView *cv, BasePoint *bp, int nosel);
extern void CVStroke(CharView *cv);
extern void FVStroke(FontView *fv);
extern void FreeHandStrokeDlg(StrokeInfo *si);
extern void OutlineDlg(FontView *fv, CharView *cv,MetricsView *mv,int isinline);
extern void ShadowDlg(FontView *fv, CharView *cv,MetricsView *mv,int wireframe);
extern void CVTile(CharView *cv);
extern void FVTile(FontView *fv);
extern void SCCharInfo(SplineChar *sc,EncMap *map,int enc);
extern void CharInfoDestroy(struct charinfo *ci);
extern SplineChar *SuffixCheck(SplineChar *sc,char *suffix);
extern void SCSubtableDefaultSubsCheck(SplineChar *sc, struct lookup_subtable *sub, struct matrix_data *possub, int col_cnt, int r);
extern GImage *PST_GetImage(GGadget *pstk,SplineFont *sf,
	struct lookup_subtable *sub,int popup_r, SplineChar *sc );
extern void CVGetInfo(CharView *cv);
extern void CVPGetInfo(CharView *cv);
extern int  SCUsedBySubs(SplineChar *sc);
extern void SCSubBy(SplineChar *sc);
extern void SCRefBy(SplineChar *sc);
extern void ApGetInfo(CharView *cv, AnchorPoint *ap);
extern void CVAddAnchor(CharView *cv);
extern AnchorClass *AnchorClassUnused(SplineChar *sc,int *waslig);
extern void FVSetWidth(FontView *fv,enum widthtype wtype);
extern void CVSetWidth(CharView *cv,enum widthtype wtype);
extern void CVChangeSC(CharView *cv, SplineChar *sc );
extern void SCRefreshTitles(SplineChar *sc);
extern void SPChangePointType(SplinePoint *sp, int pointtype);

extern void CVAdjustPoint(CharView *cv, SplinePoint *sp);
extern void CVMergeSplineSets(CharView *cv, SplinePoint *active, SplineSet *activess,
	SplinePoint *merge, SplineSet *mergess);
extern void CVAdjustControl(CharView *cv,BasePoint *cp, BasePoint *to);
extern int  CVMoveSelection(CharView *cv, real dx, real dy, uint32 input_state);
#if defined(FONTFORGE_CONFIG_GDRAW)
extern int  CVTestSelectFromEvent(CharView *cv,GEvent *event);
extern void CVMouseMovePen(CharView *cv, PressedOn *p, GEvent *event);
extern void CVMouseUpPoint(CharView *cv,GEvent *event);
extern int  CVMouseMovePointer(CharView *cv, GEvent *event);
extern void CVMouseDownPointer(CharView *cv, FindSel *fs, GEvent *event);
extern void CVMouseDownRuler(CharView *cv, GEvent *event);
extern void CVMouseMoveRuler(CharView *cv, GEvent *event);
extern int CVMouseAtSpline(CharView *cv,GEvent *event);
extern void CVMouseUpRuler(CharView *cv, GEvent *event);
extern void CVMouseMoveHand(CharView *cv, GEvent *event);
extern void CVMouseDownFreeHand(CharView *cv, GEvent *event);
extern void CVMouseMoveFreeHand(CharView *cv, GEvent *event);
extern void CVMouseUpFreeHand(CharView *cv, GEvent *event);
extern void CVMouseDownShape(CharView *cv,GEvent *event);
extern void CPStartInfo(CharView *cv, GEvent *event);
extern void CPUpdateInfo(CharView *cv, GEvent *event);
extern void CPEndInfo(CharView *cv);
extern void BVChar(BitmapView *cv, GEvent *event );
#endif
extern void CVMouseDownPoint(CharView *cv,GEvent *event);
extern void CVMouseMovePoint(CharView *cv,PressedOn *);
extern void CVMouseUpPointer(CharView *cv );
extern void CVCheckResizeCursors(CharView *cv);
extern void CVMouseDownHand(CharView *cv);
extern void CVMouseUpHand(CharView *cv);
extern void CVMouseDownTransform(CharView *cv);
extern void CVMouseMoveTransform(CharView *cv);
extern void CVMouseUpTransform(CharView *cv);
extern void CVMouseDownKnife(CharView *cv);
extern void CVMouseMoveKnife(CharView *cv,PressedOn *);
extern void CVMouseUpKnife(CharView *cv,GEvent *event);
extern void CVMouseMoveShape(CharView *cv);
extern void CVMouseUpShape(CharView *cv);
#ifdef FONTFORGE_CONFIG_GDRAW
extern void LogoExpose(GWindow pixmap,GEvent *event, GRect *r,enum drawmode dm);
#endif
extern void CVDebugPointPopup(CharView *cv);

extern int GotoChar(SplineFont *sf,EncMap *map);

extern int CVLayer(CharView *cv);
extern Undoes *CVPreserveStateHints(CharView *cv);
extern Undoes *CVPreserveState(CharView *cv);
extern Undoes *CVPreserveTState(CharView *cv);
extern Undoes *CVPreserveWidth(CharView *cv,int width);
extern Undoes *CVPreserveVWidth(CharView *cv,int vwidth);
extern void CVDoRedo(CharView *cv);
extern void CVDoUndo(CharView *cv);
extern void CVRestoreTOriginalState(CharView *cv);
extern void CVUndoCleanup(CharView *cv);
extern void CVRemoveTopUndo(CharView *cv);
extern void CopySelected(CharView *cv);
extern void CVCopyGridFit(CharView *cv);
extern void CopyWidth(CharView *cv,enum undotype);
extern void MVCopyChar(MetricsView *mv, SplineChar *sc, enum fvcopy_type fullcopy);
extern void PasteIntoMV(MetricsView *mv,SplineChar *sc, int doclear);

extern void CVShowPoint(CharView *cv, BasePoint *me);

extern BitmapView *BitmapViewCreate(BDFChar *bc, BDFFont *bdf, FontView *fv,int enc);
extern BitmapView *BitmapViewCreatePick(int enc, FontView *fv);
extern void BitmapViewFree(BitmapView *bv);
#ifdef FONTFORGE_CONFIG_GDRAW
extern void BVMenuRotateInvoked(GWindow gw,struct gmenuitem *mi, GEvent *e);
#endif
extern void BVRotateBitmap(BitmapView *bv,enum bvtools type );
extern int  BVColor(BitmapView *bv);
extern void BCSetPoint(BDFChar *bc, int x, int y, int color);
extern void BCGeneralFunction(BitmapView *bv,
	void (*SetPoint)(BitmapView *,int x, int y, void *data),void *data);
extern char *BVFlipNames[];
extern void BVChangeBC(BitmapView *bv, BDFChar *bc, int fitit );

extern void MVSetSCs(MetricsView *mv, SplineChar **scs);
extern void MVRefreshChar(MetricsView *mv, SplineChar *sc);
extern void MVRegenChar(MetricsView *mv, SplineChar *sc);
extern void MVReKern(MetricsView *mv);
extern MetricsView *MetricsViewCreate(FontView *fv,SplineChar *sc,BDFFont *bdf);
extern void MetricsViewFree(MetricsView *mv);
extern void MVRefreshAll(MetricsView *mv);
extern void MV_FriendlyFeatures(GGadget *g, int pos);
extern GTextInfo *SLOfFont(SplineFont *sf);
extern uint32 *StdFeaturesOfScript(uint32 script);

extern void DoPrefs(void);
#if defined(FONTFORGE_CONFIG_GTK)
#elif defined(FONTFORGE_CONFIG_GDRAW)
extern void GListAddStr(GGadget *list,unichar_t *str, void *ud);
extern void GListReplaceStr(GGadget *list,int index, unichar_t *str, void *ud);
extern struct macname *NameGadgetsGetNames( GWindow gw );
extern void NameGadgetsSetEnabled( GWindow gw, int enable );
extern int GCDBuildNames(GGadgetCreateData *gcd,GTextInfo *label,int pos,struct macname *names);
extern void GCDFillMacFeat(GGadgetCreateData *mfgcd,GTextInfo *mflabels, int width,
	MacFeat *all, int fromprefs, GGadgetCreateData *boxes,
	GGadgetCreateData **array);
extern void Prefs_ReplaceMacFeatures(GGadget *list);
#endif

#if defined(FONTFORGE_CONFIG_GTK)
extern char *FVOpenFont(char *title, const char *defaultfile,
	const char *initial_filter,int mult);
#elif defined(FONTFORGE_CONFIG_GDRAW)
extern void SCAutoTrace(SplineChar *sc,GWindow v,int ask);
extern unichar_t *FVOpenFont(char *title, const char *defaultfile, int mult);
#endif


extern void PrintDlg(FontView *fv,SplineChar *sc,MetricsView *mv);

#if defined(FONTFORGE_CONFIG_GTK)
#elif defined(FONTFORGE_CONFIG_GDRAW)
enum sftf_fonttype { sftf_pfb, sftf_ttf, sftf_otf, sftf_bitmap, sftf_pfaedit };
extern int SFTFSetFontData(GGadget *g, int start, int end, SplineFont *sf,
	enum sftf_fonttype, int size, int antialias);
extern int SFTFSetFont(GGadget *g, int start, int end, SplineFont *sf);
extern int SFTFSetFontType(GGadget *g, int start, int end, enum sftf_fonttype);
extern int SFTFSetSize(GGadget *g, int start, int end, int size);
extern int SFTFSetAntiAlias(GGadget *g, int start, int end, int antialias);
extern int SFTFSetScriptLang(GGadget *g, int start, int end, uint32 script, uint32 lang);
extern int SFTFSetFeatures(GGadget *g, int start, int end, uint32 *features);
extern void SFTFRegisterCallback(GGadget *g, void *cbcontext,
	void (*changefontcallback)(void *,SplineFont *,enum sftf_fonttype,int size,int aa,
		uint32 script, uint32 lang, uint32 *features));
extern void SFTFProvokeCallback(GGadget *g);
extern void SFTFInitLangSys(GGadget *g, int end, uint32 script, uint32 lang);
extern GGadget *SFTextAreaCreate(struct gwindow *base, GGadgetData *gd,void *data);
#endif
extern void DisplayDlg(SplineFont *sf);

extern void ShowAboutScreen(void);
extern void DelayEvent(void (*func)(void *), void *data);

extern void FindProblems(FontView *fv,CharView *cv,SplineChar *sc);
extern void MetaFont(FontView *fv,CharView *cv,SplineChar *sc);
extern void CVConstrainSelection(CharView *cv,int type);
extern void CVMakeParallel(CharView *cv);

extern void ScriptDlg(FontView *fv,CharView *cv);

# if HANYANG
extern void MenuNewComposition(GWindow gw, struct gmenuitem *, GEvent *);
extern void CVDisplayCompositions(GWindow gw, struct gmenuitem *, GEvent *);
extern void Disp_DoFinish(struct jamodisplay *d, int cancel);
extern void Disp_RefreshChar(SplineFont *sf,SplineChar *sc);
extern void Disp_DefaultTemplate(CharView *cv);
# endif

extern SearchView *SVCreate(FontView *fv);
extern void SVCharViewInits(SearchView *sv);
#ifdef FONTFORGE_CONFIG_GDRAW
extern void SVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void SVChar(SearchView *sv, GEvent *event);
#endif
extern void SVMakeActive(SearchView *sv,CharView *cv);
extern int SVAttachFV(FontView *fv,int ask_if_difficult);
extern void SVDetachFV(FontView *fv);

extern void ShowAtt(SplineFont *sf);
extern void FontCompareDlg(FontView *fv);
extern void SFShowKernPairs(SplineFont *sf,SplineChar *sc,AnchorClass *ac);
extern void SFShowLigatures(SplineFont *sf,SplineChar *sc);

extern void SCEditInstructions(SplineChar *sc);
extern void SFEditTable(SplineFont *sf, uint32 tag);
extern void IIScrollTo(struct instrinfo *ii,int ip,int mark_stop);
extern void IIReinit(struct instrinfo *ii,int ip);
#ifdef FONTFORGE_CONFIG_GDRAW
extern int ii_v_e_h(GWindow gw, GEvent *event);
extern void instr_scroll(struct instrinfo *ii,struct sbevent *sb);
#endif

extern void CVGridFitChar(CharView *cv);
extern void CVFtPpemDlg(CharView *cv,int debug);
extern void SCDeGridFit(SplineChar *sc);

extern void CVDebugReInit(CharView *cv,int restart_debug,int dbg_fpgm);
extern void CVDebugFree(DebugView *dv);
#if defined(FONTFORGE_CONFIG_GDRAW)
extern int DVChar(DebugView *dv, GEvent *e);
#endif

extern void KernClassD(KernClass *kc, SplineFont *sf, int isv);
extern void ShowKernClasses(SplineFont *sf,MetricsView *mv,int isv);
extern void KCLD_End(struct kernclasslistdlg *kcld);
extern void KCLD_MvDetach(struct kernclasslistdlg *kcld,MetricsView *mv);
extern void KernPairD(SplineFont *sf,SplineChar *sc1,SplineChar *sc2,int isv);
#if defined(FONTFORGE_CONFIG_GDRAW)
extern void KCD_DrawGlyph(GWindow pixmap,int x,int baseline,BDFChar *bdfc,int mag);
#endif
extern GTextInfo *BuildFontList(FontView *except);
extern void TFFree(GTextInfo *tf);

extern void AnchorControl(SplineChar *sc,AnchorPoint *ap);
extern void AnchorControlClass(SplineFont *_sf,AnchorClass *ac);

extern void FVSelectByPST(FontView *fv);
extern void SFUntickAll(SplineFont *sf);

enum hist_type { hist_hstem, hist_vstem, hist_blues };
struct psdict;
extern void SFHistogram(SplineFont *sf,struct psdict *private,uint8 *selected,
	EncMap *map, enum hist_type which);

extern void CCD_Close(struct contextchaindlg *ccd);
extern int CCD_NameListCheck(SplineFont *sf,const char *ret,int empty_bad,char *title);
#if defined(FONTFORGE_CONFIG_GDRAW)
extern struct contextchaindlg *ContextChainEdit(SplineFont *sf,FPST *fpst,
	struct gfi_data *gfi,unichar_t *newname);
extern int CCD_InvalidClassList(char *ret,GGadget *list,int wasedit);
extern char *cu_copybetween(const unichar_t *start, const unichar_t *end);
#endif

extern struct statemachinedlg *StateMachineEdit(SplineFont *sf,ASM *sm,struct gfi_data *d);
extern void SMD_Close(struct statemachinedlg *smd);
extern void GFI_FinishSMNew(struct gfi_data *d,ASM *sm, int success,int isnew);
extern void GFI_SMDEnd(struct gfi_data *d);

extern void MMChangeBlend(MMSet *mm,FontView *fv,int tonew);
extern void MMWizard(MMSet *mm);

extern int LayerDialog(Layer *layer);
extern void CVLayerChange(CharView *cv);

extern void CVPointOfView(CharView *cv,struct pov_data *);
extern int PointOfViewDlg(struct pov_data *pov,SplineFont *sf,int flags);

extern SplineChar *FVMakeChar(FontView *fv,int i);

extern void CVYPerspective(CharView *cv,double x_vanish, double y_vanish);

extern void DVCreateGloss(DebugView *dv);
extern int CVXPos(DebugView *dv,int offset,int width);

extern EncMap *EncMapFromEncoding(SplineFont *sf,Encoding *enc);
extern void SFRemoveGlyph(SplineFont *sf,SplineChar *sc, int *flags);
extern void FVAddEncodingSlot(FontView *fv,int gid);
extern void SFAddEncodingSlot(SplineFont *sf,int gid);
extern void SFAddGlyphAndEncode(SplineFont *sf,SplineChar *sc,EncMap *basemap, int baseenc);
#if defined(FONTFORGE_CONFIG_GDRAW)
extern GMenuItem *GetEncodingMenu(void (*func)(GWindow,GMenuItem *,GEvent *),
	Encoding *current);

extern GTextInfo *TIFromName(const char *name);
#endif

enum subtable_data_flags {
    /* I have flags for each alternative because I want "unspecified" to be */
    /*  an option */
    sdf_kernclass      = 0x01,
    sdf_kernpair       = 0x02,
    sdf_verticalkern   = 0x04,
    sdf_horizontalkern = 0x08,
    sdf_dontedit       = 0x10
};
struct subtable_data {
    int flags;
    SplineChar *sc;
};

extern GTextInfo **SFLookupListFromType(SplineFont *sf, int lookup_type );
extern GTextInfo **SFSubtablesOfType(SplineFont *sf, int lookup_type, int kernclass);
extern GTextInfo *SFSubtableListOfType(SplineFont *sf, int lookup_type, int kernclass);
extern struct lookup_subtable *SFNewLookupSubtableOfType(SplineFont *sf, int lookup_type, struct subtable_data *sd );
extern int EditLookup(OTLookup *otl,int isgpos,SplineFont *sf);
extern int EditSubtable(struct lookup_subtable *sub,int isgpos,SplineFont *sf,
	struct subtable_data *sd);
extern void _LookupSubtableContents(SplineFont *sf, struct lookup_subtable *sub,
	struct subtable_data *sd);

extern void SFBdfProperties(SplineFont *sf, EncMap *map, BDFFont *thisone);


struct instrdlg;
uint8 *_IVParse(struct instrdlg *iv, char *text, int *len);
char *_IVUnParseInstrs(uint8 *instrs,int instr_cnt);

#ifdef FONTFORGE_CONFIG_GDRAW
extern GMenuItem2 helplist[];
#endif
extern BasePoint last_ruler_offset[];

#endif	/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
#endif	/* _VIEWS_H */
