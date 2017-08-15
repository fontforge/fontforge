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
#ifndef _VIEWS_H
#define _VIEWS_H

#include <fontforge-config.h>

#include "ttfinstrs.h"

#include "ffglib.h"
#include "baseviews.h"

#include <ggadget.h>
#include "dlist.h"
#include "search.h"


struct gfi_data;
struct contextchaindlg;
struct statemachinedlg;


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
    int showrefnames;
    int snapoutlines;
    int showalmosthvlines;
    int showalmosthvcurves;
    int hvoffset;
    int checkselfintersects;	/* Not really something shown, but convenient to keep it here */
    int showdebugchanges;	/* Changes the way changing rasters are displayed in tt debug mode */
    int alwaysshowcontrolpoints; //< Always show the BCP even when their splinepoint is not selected
} CVShows;

extern struct bvshows {
    int showfore, showoutline, showgrid;
    int lastpixelsize;
} BVShows;

enum debug_wins { dw_registers=0x1, dw_stack=0x2, dw_storage=0x4, dw_points=0x8,
	dw_cvt=0x10, dw_raster=0x20, dw_gloss=0x40 };

struct instrinfo {
    int isel_pos;
    int16 lheight,lpos;
    char *scroll, *offset;
    GWindow v;
    GGadget *vsb;
    int16 sbw;
    int16 vheight, vwidth;
    int16 lstopped;
    int16 as, fh;
    struct instrdata *instrdata;
    GFont *gfont;
    unsigned int showaddr: 1;
    unsigned int showhex: 1;
    unsigned int mousedown: 1;
    void *userdata;
    void (*selection_callback)(struct instrinfo *,int ip);
    int  (*bpcheck)(struct instrinfo *,int ip);
    int  (*handle_char)(struct instrinfo *,GEvent *e);
};

struct reflist {
    RefChar *ref;
    struct reflist *parent;
};

typedef struct debugview {
    struct debugger_context *dc;	/* Local to freetype.c */
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
    struct instrdata id;
    struct instrinfo ii;
    int dwidth, toph;
    struct charview *cv;
    double scalex, scaley;
    int pts_head, cvt_offtop, gloss_offtop, storage_offtop, stack_offtop, reg_offtop;
    int points_offtop;

    int codeSize;
    uint8 initialbytes[4];
    struct reflist *active_refs;
    int last_npoints;
    int layer;
} DebugView;


/* The number of tabs allowed in the outline glyph view of former glyphs */
#define FORMER_MAX	10

enum dv_coderange { cr_none=0, cr_fpgm, cr_prep, cr_glyph };	/* cleverly chosen to match ttobjs.h */

struct freehand {
    struct tracedata *head, *last;	/* for the freehand tool */
    SplinePointList *current_trace;
    int ignore_wobble;		/* Ignore wiggles smaller than this */
    int skip_cnt;
};

enum expandedge { ee_none, ee_nw, ee_up, ee_ne, ee_right, ee_se, ee_down,
		  ee_sw, ee_left, ee_max };

enum { charviewtab_charselectedsz = 1024 };
typedef struct charviewtab
{
    char charselected[ charviewtab_charselectedsz + 1 ];
    char tablabeltxt[ charviewtab_charselectedsz + 1 ];
} CharViewTab;

enum { charview_cvtabssz = 100 };


 /* approximately BACK_LAYER_MAX / 32 */
#define BACK_LAYERS_VIEW_MAX 8

typedef struct charview {
    CharViewBase b;
    uint32 showback[BACK_LAYERS_VIEW_MAX];
    unsigned int showfore:1;
    unsigned int showgrids:1;
    unsigned int showhhints:1;
    unsigned int showvhints:1;
    unsigned int showdhints:1;
    unsigned int showpoints:1;
    unsigned int alwaysshowcontrolpoints:1;
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
    unsigned int showpointnumbers:2;
    unsigned int markextrema:1;
    unsigned int markpoi:1;
    unsigned int needsrasterize:1;		/* Rasterization (of fill or fontview) needed on mouse up */
    unsigned int recentchange:1;		/* a change happened in the grids or background. don't need to rasterize */
    unsigned int info_within: 1;		/* cursor is within main window */
    unsigned int back_img_out_of_date: 1;	/* Force redraw of back image pixmap */
    unsigned int cntrldown:1;
    unsigned int joinvalid:1;
    unsigned int widthsel:1;
    unsigned int vwidthsel:1;
    unsigned int lbearingsel:1;
    unsigned int icsel:1;
    unsigned int tah_sel:1;
    unsigned int inactive:1;			/* When in a search view (32) */
    unsigned int show_ft_results: 1;
    unsigned int show_ft_results_live_update : 1;
    unsigned int coderange: 2;			/* For the debugger */
    unsigned int autonomous_ruler_w: 1;
    unsigned int showcpinfo: 1;
    unsigned int showtabs: 1;
    unsigned int showsidebearings: 1;
    unsigned int showing_spiro_pt_menu: 1;
    unsigned int ruler_pressed: 1;
    unsigned int ruler_pressedv: 1;
    unsigned int showrefnames: 1;
    unsigned int snapoutlines: 1;
    unsigned int showalmosthvlines: 1;
    unsigned int showalmosthvcurves: 1;
    unsigned int checkselfintersects: 1;
    unsigned int showdebugchanges: 1;
    unsigned int inPreviewMode: 1;
    unsigned int inDraggingComparisonOutline: 1;
    unsigned int activeModifierControl: 1; //< Is control being held right now?
    unsigned int activeModifierAlt: 1;     //< Is alt being held right now?
    unsigned int changedActiveGlyph: 1;    //< Set in CVSwitchActiveSC() cleared in cvmouseup()
    
    int hvoffset;		/* for showalmosthvlines */
    int layers_off_top;
    real scale;
    GWindow gw, v;
    GGadget *vsb, *hsb, *mb, *tabs;
    GFont *small, *normal;
    GWindow icon;
    GWindow ruler_w;
    GWindow ruler_linger_w;
    unichar_t ruler_linger_lines[40][80];
    int ruler_linger_num_lines;
    int num_ruler_intersections;
    int allocated_ruler_intersections;
    BasePoint *ruler_intersections;
    int start_intersection_snapped;
    int end_intersection_snapped;
    GFont *rfont;
    GTimer *pressed;
    GWindow backimgs;
    GIC *gic;
    GIC *gwgic;
    int width, height;
    float xoff, yoff; /* must be floating point, for precise zoom by scroll */
    int mbh;    //< menu bar height
    int charselectorh;  //< char selection input box height
    int infoh;  //< info bar height
    int rulerh; //< ruler height
    int16 sas, sfh, sdh, nas, nfh;
    BasePoint info;
    SplinePoint *info_sp;
    Spline *info_spline;
    real info_t;
    GPoint e;					/* mouse location */
    GPoint olde;
    BasePoint last_c;
    BDFChar *filled;
    GImage gi;					/* used for fill bitmap only */
    int enc;
    EncMap *map_of_enc;				/* Only use for comparison against fontview's map to see if our enc be valid */
						/*  Will not be updated when fontview is reencoded */
    PressedOn p;
    SplinePoint *lastselpt;
    spiro_cp *lastselcp;
    /*GWindow tools, layers;*/
    int8 b1_tool, cb1_tool, b2_tool, cb2_tool;	/* Button 3 does a popup */
    int8 b1_tool_old;				/* Used by mingw port */
    int8 s1_tool, s2_tool, er_tool;		/* Bindings for wacom stylus and eraser */
    int8 showing_tool, pressed_tool, pressed_display, had_control, active_tool;
    int8 spacebar_hold;				/* spacebar is held down */
    SplinePointList *active_spl;
    SplinePoint *active_sp;
    spiro_cp *active_cp;
    IPoint handscroll_base;
    uint16 rfh, ras;
    BasePoint lastknife;
    struct freehand freehand;
    enum expandedge expandedge;
    BasePoint expandorigin;
    real expandwidth, expandheight;
    SplinePointList *active_shape;
    SplinePoint joinpos;
    spiro_cp joincp;
    SplineChar *template1, *template2;
#if HANYANG
    struct jamodisplay *jamodisplay;
#endif
    real oldwidth, oldvwidth;
    real oldlbearing;
    int16 oldic, oldtah;
#if _ModKeysAutoRepeat
    GTimer *autorpt;
    int keysym, oldstate;
    int oldkeyx, oldkeyy;
    GWindow oldkeyw;
#endif
    PST *lcarets;
    int16 nearcaret;
	/* freetype results display */
    int16 ft_dpi, ft_ppemy, ft_ppemx, ft_depth;
    real ft_pointsizey, ft_pointsizex;
    struct freetype_raster *raster, *oldraster;
    DebugView *dv;
    uint32 mmvisible;
    char *former_names[FORMER_MAX];
    int former_cnt;
    AnchorPoint *apmine, *apmatch;
    SplineChar *apsc;
    int guide_pos;
    struct qg_data *qg;
    int16 note_x, note_y;
    struct dlistnode* pointInfoDialogs;
    GGadget* charselector;     //< let the user type in more than one char to view at once.
    GGadget* charselectorNext; //< move to next word in charselector
    GGadget* charselectorPrev; //< move to prev word in charselector
    int charselectoridx;
    SplineChar* additionalCharsToShow [51]; //<  additionalCharsToShowLimit + 1 in size
    int additionalCharsToShowActiveIndex;

    CharViewTab cvtabs[ charview_cvtabssz+1 ];
    int oldtabnum;
    
} CharView;

typedef struct bitmapview {
    BDFChar *bc;
    BDFFont *bdf;
    struct fontview *fv;
    EncMap *map_of_enc;
    int enc;
    GWindow gw, v;
    GGadget *vsb, *hsb, *mb;
    GGadget *recalc;
    GFont *small;
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
    GTimer *autorpt;
    int keysym, oldstate;
#endif
    int color;			/* for greyscale fonts (between 0,255) */
    int color_under_cursor;
} BitmapView;

struct aplist { AnchorPoint *ap; int connected_to, selected; struct aplist *next; };

enum mv_grids { mv_hidegrid, mv_showgrid, mv_partialgrid, mv_hidemovinggrid };
enum mv_type { mv_kernonly, mv_widthonly, mv_kernwidth };

struct metricchar {
    int16 dx, dwidth;	/* position and width of the displayed char */
    int16 dy, dheight;	/*  displayed info for vertical metrics */
    int xoff, yoff;
    int16 mx, mwidth;	/* position and width of the text underneath */
    int16 kernafter;
    unsigned int selected: 1;
    GGadget *width, *lbearing, *rbearing, *kern, *name;
    GGadget* updownkparray[10]; /* Cherry picked elements from width...kern allowing up/down key navigation */
};

typedef struct metricsview {
    struct fontview *fv;
    SplineFont *sf;
    int pixelsize;		/* If the user has manually requested a pixelsize */
				/*  then rasterize at that size no matter how large */
			        /*  the font is zoomed. For non-user requesed sizes */
			        /*  this is the pixelsize * zoom-factor */
    BDFFont *bdf;		/* We can also see metric info on a bitmap font */
    BDFFont *show;		/*  Or the rasterized version of the outline font */
    GWindow gw, v;
    GFont *font;
    GGadget *hsb, *vsb, *mb, *text, *textPrev, *textNext, *script, *features, *subtable_list;
    GGadget *namelab, *widthlab, *lbearinglab, *rbearinglab, *kernlab;
    int16 xstart;
    int16 width, height, dwidth;
    int16 vwidth, vheight;
    int16 mbh,sbh;
    int16 topend;		/* y value of the end of the region containing the text field */
    int16 displayend;		/* y value of the end of the region showing filled characters */
    int16 fh, as;
    int16 cmax, clen;
    SplineChar **chars;		/* Character input stream */
    struct opentype_str *glyphs;/* after going through the various gsub/gpos transformations */
    struct metricchar *perchar;	/* One for each glyph above */
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
    unsigned int showgrid: 2;
    unsigned int antialias: 1;
    unsigned int vertical: 1;
    unsigned int type: 2;		/* enum mv_type */
    unsigned int usehinting: 1;         /* should the hints be used during the render */
    unsigned int pixelsize_set_by_window;
    int xp, yp, ap_owner;
    BasePoint ap_start;
    int cursor;
    int scale_index;
    struct lookup_subtable *cur_subtable;
    GTextInfo *scriptlangs;
    int word_index;
    int layer;
    int fake_unicode_base;
    GIC *gwgic;
    int ptsize, dpi;
    int ybaseline;
    int oldscript, oldlang;
} MetricsView;

enum fv_metrics { fvm_baseline=1, fvm_origin=2, fvm_advanceat=4, fvm_advanceto=8 };
typedef struct fontview {
    FontViewBase b;
    BDFFont *show, *filled;
    GWindow gw, v;
    GFont **fontset;
    GGadget *vsb, *mb;
    GTimer *pressed;
    GTimer *resize;
    GEvent resize_event;
    GIC *gic;
    GIC *gwgic;
    int width, height;		/* of v */
    int16 infoh,mbh;
    int16 lab_height, lab_as;
    int16 colcnt, rowcnt;		/* of display window */
    int32 rowoff, rowltot;		/* Can be really big in full unicode */
    int16 cbw,cbh;			/* width/height of a character box */
    int pressed_pos, end_pos;
    unsigned int antialias:1;
    unsigned int bbsized:1;		/* displayed bitmap should be scaled by bounding box rather than emsize */
    unsigned int wasonlybitmaps:1;
    /*unsigned int refstate: 3;*/	/* 0x1 => paste orig of all non exist refs, 0x2=>don't, 0x3 => don't warn about non-exist refs with no source font */
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
    unsigned int notactive:1;			/* When embedded in a dlg */
    int16 magnify;
    int16 user_requested_magnify;
    struct searchview *sv;
    SplineChar *sc_near_top;
    int sel_index;
    struct lookup_subtable *cur_subtable;
    struct qg_data *qg;
    GPid pid_webfontserver;
} FontView;

typedef struct findsel {
    GEvent *e;
    real fudge;		/* One pixel fudge factor */
    real xl,xh, yl, yh;	/* One pixel fudge factor */
    real c_xl,c_xh, c_yl, c_yh;		/* fudge rectangle for control points, larger than above if alt is depressed */
    unsigned int select_controls: 1;	/* notice control points */
    unsigned int seek_controls: 1;	/* notice control points before base points */
    unsigned int all_controls: 1;	/* notice control points even if the base points aren't selected (in truetype point numbering mode where all cps are visible) */
    unsigned int alwaysshowcontrolpoints:1; /* if the BCP are forced on, then we want the selection code paths
					     * to also know that so the user can drag the BCP of a non selected splinepoint */
    real scale;
    PressedOn *p;
} FindSel;

typedef struct searchview {
    struct cvcontainer base;
    FontView dummy_fv;
    SplineFont dummy_sf;
    LayerInfo layerinfo[2];
    SplineChar *chars[2];
    EncMap dummy_map;
    int32 map[2], backmap[2];
    uint8 sel[2];
    CharView cv_srch, cv_rpl;
    CharView *lastcv;
/* ****** */
    GWindow gw;
    GGadget *mb;
    GFont *plain, *bold;
    int mbh;
    int fh, as;
    int rpl_x, cv_y;
    int cv_width, cv_height;
    short button_height, button_width;
/* ****** */
    SearchData sd;
    unsigned int showsfindnext: 1;
    unsigned int findenabled: 1;
    unsigned int rplallenabled: 1;
    unsigned int rplenabled: 1;
    unsigned int isvisible: 1;
} SearchView;

typedef struct mathkernview {
    struct cvcontainer base;
    FontView dummy_fv;
    SplineFont dummy_sf;
    LayerInfo layerinfo[2];
    SplineChar sc_topright, sc_topleft, sc_bottomright, sc_bottomleft;
    SplineChar *chars[4];
    EncMap dummy_map;
    int32 map[4], backmap[4];
    uint8 sel[4];
    CharView cv_topright, cv_topleft, cv_bottomright, cv_bottomleft;
    CharView *lastcv;
/* ****** */
    GWindow gw;
    GWindow cvparent_w;
    GGadget *mb;
    GFont *plain, *bold;
    int mbh;
    int fh, as;
    int mid_space, cv_y;
    int cv_width, cv_height;
    short button_height, button_width;
/* ****** */
    SplineChar *cursc;
    int def_layer;
    struct mathkern *orig_mathkern;
    uint8 saved_mathkern;		/* Can't just check if orig is non-NULL, because NULL is a perfectly valid initial state */
    uint8 last_aspect;
    uint8 done;
} MathKernDlg;

# ifdef FONTFORGE_CONFIG_TILEPATH

typedef struct tilepathdlg {
    struct cvcontainer base;
    FontView dummy_fv;
    SplineFont dummy_sf;
    LayerInfo layerinfo[2];
    SplineChar sc_first, sc_medial, sc_final, sc_isolated;
    SplineChar *chars[4];
    EncMap dummy_map;
    int32 map[4], backmap[4];
    uint8 sel[4];
    CharView cv_first, cv_medial, cv_final, cv_isolated;
    CharView *lastcv;
/* ****** */
    GWindow gw;
    GGadget *mb;
    GFont *plain, *bold;
    int mbh;
    int fh, as;
    int mid_space, cv_y;
    int cv_width, cv_height;
/* ****** */
    struct tiledata *td;
    SplineFont *base_sf;
    uint8 done, oked;
} TilePathDlg;
extern void TPDCharViewInits(TilePathDlg *tpd, int cid);
extern void PTDCharViewInits(TilePathDlg *tpd, int cid);
#endif		/* Tile Path */

typedef struct gradientdlg {
    struct cvcontainer base;
    FontView dummy_fv;
    SplineFont dummy_sf;
    LayerInfo layerinfo[2];
    SplineChar sc_grad;
    SplineChar *chars[1];
    EncMap dummy_map;
    int32 map[1], backmap[1];
    uint8 sel[1];
    CharView cv_grad;
/* ****** */
    GWindow gw;
    GGadget *mb;
    GFont *plain, *bold;
    int mbh;
    int fh, as;
    int mid_space, cv_y;
    int cv_width, cv_height;
/* ****** */
    uint8 done, oked;
    struct gradient *active;
} GradientDlg;
extern void GDDCharViewInits(GradientDlg *gdd,int cid);

typedef struct strokedlg {
    struct cvcontainer base;
    FontView dummy_fv;
    SplineFont dummy_sf;
    LayerInfo layerinfo[2];
    SplineChar sc_stroke;
    SplineChar *chars[1];
    EncMap dummy_map;
    int32 map[1], backmap[1];
    uint8 sel[1];
    CharView cv_stroke;
    int cv_width, cv_height;
    GGadget *mb;
    int mbh;
    SplineSet *old_poly;
/* ****** */
    int done;
    GWindow gw;
    CharView *cv;
    FontView *fv;
    SplineFont *sf;
    void (*strokeit)(void *,StrokeInfo *,int);
    StrokeInfo *si;
    GRect r1, r2;
    int up[2];
    int dontexpand;
} StrokeDlg;
extern void StrokeCharViewInits(StrokeDlg *sd,int cid);

struct lksubinfo {
    struct lookup_subtable *subtable;
    unsigned int deleted: 1;
    unsigned int new: 1;
    unsigned int selected: 1;
    unsigned int moved: 1;
};

struct lkinfo {
    OTLookup *lookup;
    unsigned int open: 1;
    unsigned int deleted: 1;
    unsigned int new: 1;
    unsigned int selected: 1;
    unsigned int moved: 1;
    int16 subtable_cnt, subtable_max;
    struct lksubinfo *subtables;
};

struct lkdata {
    int cnt, max;
    int off_top, off_left;
    struct lkinfo *all;
};

struct anchor_shows {
    CharView *cv;
    SplineChar *sc;
    int restart;
};

struct gfi_data {		/* FontInfo */
    SplineFont *sf;
    int def_layer;
    GWindow gw;
    int tn_active;
    int private_aspect, ttfv_aspect, tn_aspect, tx_aspect, unicode_aspect;
    int old_sel, old_aspect, old_lang, old_strid;
    int ttf_set, names_set, tex_set;
    int langlocalecode;	/* MS code for the current locale */
    unsigned int family_untitled: 1;
    unsigned int human_untitled: 1;
    unsigned int done: 1;
    unsigned int mpdone: 1;
    unsigned int lk_drag_and_drop: 1;
    unsigned int lk_dropablecursor: 1;
    struct anchor_shows anchor_shows[2];
    struct texdata texdata;
    GFont *font;
    int as, fh;
    struct lkdata tables[2];
    int lkwidth, lkheight;
    int first_sel_lookup, first_sel_subtable;
    int last_panose_family;
};

struct kf_dlg /* : fvcontainer */ {
    struct fvcontainer base;
    struct lookup_subtable *sub;
    GWindow gw, dw;
    GFont *plain, *bold;
    int fh, as;
    GGadget *mb, *guts, *topbox;
    int mbh, label2_y, infoh;

    SplineFont *sf;
    int def_layer;
    struct kf_results *results;
    int done;

    FontView *active;
    FontView *first_fv;
    FontView *second_fv;
};

enum genfam { gf_none, gf_macfamily, gf_ttc };

extern void FVMarkHintsOutOfDate(SplineChar *sc);
extern void FVRefreshChar(FontView *fv,int gid);
extern void _FVMenuOpen(FontView *fv);
extern int _FVMenuSave(FontView *fv);
extern int _FVMenuSaveAs(FontView *fv);
extern int _FVMenuGenerate(FontView *fv,int family);
extern void _FVCloseWindows(FontView *fv);
extern char *GetPostScriptFontName(char *defdir,int mult);
extern void MergeKernInfo(SplineFont *sf,EncMap *map);
extern int SFGenerateFont(SplineFont *sf,int layer, int family,EncMap *map);

extern void NonLinearDlg(FontView *fv,struct charview *cv);
extern void FVChangeChar(FontView *fv,int encoding);
extern void FVMergeFonts(FontView *fv);
extern void FVInterpolateFonts(FontView *fv);

extern void FVDeselectAll(FontView *fv);

extern void FVAutoWidth2(FontView *fv);
/*extern void FVAutoKern(FontView *fv);*/
/*extern void FVAutoWidth(FontView *fv);*/

extern void SC_MarkInstrDlgAsChanged(SplineChar *sc);

extern void PythonUI_Init(void);
extern void PythonUI_namedpipe_Init(void);

extern void SCStroke(SplineChar *sc);

extern void PfaEditSetFallback(void);
extern void RecentFilesRemember(char *filename);
extern void LastFonts_Save(void);

struct debugger_context;
extern void DebuggerTerminate(struct debugger_context *dc);
extern void DebuggerReset(struct debugger_context *dc,real pointsizey, real pointsizex,int dpi,int dbg_fpgm, int is_bitmap);
extern struct debugger_context *DebuggerCreate(SplineChar *sc,int layer,real pointsizey,real pointsizex,int dpi,int dbg_fpgm, int is_bitmap);
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


extern void PrintFFDlg(FontView *fv,SplineChar *sc,MetricsView *mv);
extern void PrintWindowClose(void);
extern void InsertTextDlg(CharView *cv);

extern char *Kern2Text(SplineChar *other,KernPair *kp,int isv);
extern char *PST2Text(PST *pst,SplineFont *sf);



void EmboldenDlg(FontView *fv, CharView *cv);
void CondenseExtendDlg(FontView *fv, CharView *cv);
void ObliqueDlg(FontView *fv, CharView *cv);
void GlyphChangeDlg(FontView *fv, CharView *cv, enum glyphchange_type gc);
void ItalicDlg(FontView *fv, CharView *cv);
void ChangeXHeightDlg(FontView *fv,CharView *cv);

extern int FVParseSelectByPST(FontView *fv,struct lookup_subtable *sub,
	int search_type);
extern void DropChars2Text(GWindow gw, GGadget *glyphs,GEvent *event);


extern void FVReplaceOutlineWithReference( FontView *fv, double fudge );
extern void SVDestroy(struct searchview *sv);



extern int  SLICount(SplineFont *sf);
extern unichar_t *ClassName(const char *name,uint32 feature_tag,
	uint16 flags, int script_lang_index, int merge_with, int act_type,
	int macfeature,SplineFont *sf);
extern unichar_t *DecomposeClassName(const unichar_t *clsnm, unichar_t **name,
	uint32 *feature_tag, int *macfeature,
	uint16 *flags, uint16 *script_lang_index,int *merge_with,int *act_type,
	SplineFont *sf);
extern PST *AddSubs(PST *last,uint32 tag,char *name,uint16 flags,
	uint16 sli,SplineChar *sc);


extern void FVSetUIToMatch(FontView *destfv,FontView *srcfv);
extern void FVScrollToChar(FontView *fv,int i);
extern void FVRegenChar(FontView *fv,SplineChar *sc);
extern FontView *FontNew(void);
extern void _MenuWarnings(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void MenuPrefs(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuXRes(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuSaveAll(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuExit(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuHelp(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuIndex(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuAbout(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuLicense(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuNew(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void WindowMenuBuild(GWindow base,struct gmenuitem *mi,GEvent *);
extern void MenuRecentBuild(GWindow base,struct gmenuitem *mi,GEvent *);
extern void MenuScriptsBuild(GWindow base,struct gmenuitem *mi,GEvent *);
extern void mb2FreeGetText(GMenuItem2 *mb);
extern void mb2DoGetText(GMenuItem2 *mb);
extern void mbFreeGetText(GMenuItem *mb);
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
extern void CharInfoInit(void);
extern void SCLigCaretCheck(SplineChar *sc,int clean);
extern char *DevTab_Dlg(GGadget *g, int r, int c);
extern int DeviceTableOK(char *dvstr, int *_low, int *_high);
extern void VRDevTabParse(struct vr *vr,struct matrix_data *md);
extern DeviceTable *DeviceTableParse(DeviceTable *dv,char *dvstr);
extern void DevTabToString(char **str,DeviceTable *adjust);
extern void ValDevTabToStrings(struct matrix_data *mds,int first_offset,ValDevTab *adjust);
extern void KpMDParse(SplineChar *sc,struct lookup_subtable *sub,
	struct matrix_data *possub,int rows,int cols,int i);
extern void GFI_LookupEnableButtons(struct gfi_data *gfi, int isgpos);
extern void GFI_LookupScrollbars(struct gfi_data *gfi, int isgpos, int refresh);
extern void FontInfo(SplineFont *sf,int layer,int aspect,int sync);
extern void FontInfoDestroy(SplineFont *sf);
extern void FontMenuFontInfo(void *fv);
extern struct enc *MakeEncoding(SplineFont *sf, EncMap *map);
extern void LoadEncodingFile(void);
extern void RemoveEncoding(void);
extern void SFPrivateInfo(SplineFont *sf);
extern void FVDelay(FontView *fv,void (*func)(FontView *));
extern void GFI_FinishContextNew(struct gfi_data *d,FPST *fpst, int success);
extern void SCPreparePopup(GWindow gw,SplineChar *sc, struct remap *remap, int enc, int actualuni);
enum outlinesfm_flags {
    sfm_stroke=0x1,
    sfm_fill=0x2,
    sfm_nothing=0x4,
    sfm_stroke_trans = (0x1|0x8),
    sfm_clip_preserve = 0x16
};
extern void CVDrawSplineSetSpecialized( CharView *cv, GWindow pixmap, SplinePointList *set,
					Color fg, int dopoints, DRect *clip,
					enum outlinesfm_flags strokeFillMode,
					Color AlphaChannelOverride );
extern void CVDrawSplineSet(CharView *cv, GWindow pixmap, SplinePointList *set,
	Color fg, int dopoints, DRect *clip );
extern void CVDrawSplineSetOutlineOnly(CharView *cv, GWindow pixmap, SplinePointList *set,
	Color fg, int dopoints, DRect *clip, enum outlinesfm_flags strokeFillMode );
extern GWindow CVMakeTools(CharView *cv);
extern GWindow CVMakeLayers(CharView *cv);
extern GWindow BVMakeTools(BitmapView *bv);
extern GWindow BVMakeLayers(BitmapView *bv);
extern void CVSetLayer(CharView *cv,int layer);
extern int CVPaletteMnemonicCheck(GEvent *event);
extern int TrueCharState(GEvent *event);
extern void CVToolsPopup(CharView *cv, GEvent *event);
extern void BVToolsPopup(BitmapView *bv, GEvent *event);
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
extern void CV_LayerPaletteCheck(SplineFont *sf);
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
extern int CVInSpiro( CharView *cv );

extern void CVDoTransform(CharView *cv, enum cvtools cvt );

// apply transform to specified layer
extern void CVTransFuncLayer(CharView *cv,Layer *ly,real transform[6], enum fvtrans_flags flags);
// apply transform to the current layer only
extern void CVTransFunc(CharView *cv,real transform[6],enum fvtrans_flags);
// apply transform to all layers
extern void CVTransFuncAllLayers(CharView *cv,real transform[6], enum fvtrans_flags flags);
enum transdlg_flags { tdf_enableback=0x1, tdf_enablekerns=0x2,
	tdf_defaultkerns=0x4, tdf_addapply=0x8 };
extern void TransformDlgCreate(void *data,void (*transfunc)(void *,real *,int,BVTFunc *,enum fvtrans_flags),
	int (*getorigin)(void *,BasePoint *,int), enum transdlg_flags flags,
	enum cvtools cvt);
extern void BitmapDlg(FontView *fv,SplineChar *sc, int isavail);
extern int SimplifyDlg(SplineFont *sf,struct simplifyinfo *smpl);
extern void CVReviewHints(CharView *cv);
extern void CVCreateHint(CharView *cv,int ishstem,int preserveundoes);
extern void SCRemoveSelectedMinimumDistances(SplineChar *sc,int inx);
extern int CVExport(CharView *cv);
extern int BVExport(BitmapView *bv);

extern void DrawAnchorPoint(GWindow pixmap,int x, int y,int selected);
extern void DefaultY(GRect *pos);
extern void CVDrawRubberRect(GWindow pixmap, CharView *cv);
extern void CVInfoDraw(CharView *cv, GWindow pixmap );
extern void CVChar(CharView *cv, GEvent *event );
extern void PI_ShowHints(SplineChar *sc, GGadget *list, int set);
extern GTextInfo *SCHintList(SplineChar *sc,HintMask *);
extern void CVResize(CharView *cv );
extern CharView *CharViewCreate(SplineChar *sc,FontView *fv,int enc);
extern void CharViewFinishNonStatic();

/**
 * Extended version of CharViewCreate() which allows a window to be created but
 * not displayed.
 */
extern CharView *CharViewCreateExtended(SplineChar *sc, FontView *fv,int enc, int show );
extern void CharViewFree(CharView *cv);
extern int CVValid(SplineFont *sf, SplineChar *sc, CharView *cv);
extern void CVSetCharChanged(CharView *cv,int changed);
extern int CVAnySel(CharView *cv, int *anyp, int *anyr, int *anyi, int *anya);
extern int CVAnySelPoints(CharView *cv);

/**
 * Get all the selected points in the current cv.
 * Caller must g_list_free() the returned value.
 */
extern GList_Glib* CVGetSelectedPoints(CharView *cv);
extern void CVSelectPointAt(CharView *cv);
extern int CVClearSel(CharView *cv);
extern int CVSetSel(CharView *cv,int mask);
extern void CVInvertSel(CharView *cv);
extern int CVAllSelected(CharView *cv);
extern SplinePointList *CVAnySelPointList(CharView *cv);
extern int CVAnySelPoint(CharView *cv, SplinePoint **selsp, spiro_cp **selcp);
extern int CVOneThingSel(CharView *cv, SplinePoint **sp, SplinePointList **spl,
	RefChar **ref, ImageList **img, AnchorPoint **ap, spiro_cp **cp);
extern int CVOneContourSel(CharView *cv, SplinePointList **_spl,
	RefChar **ref, ImageList **img);
extern void CVInfoDrawText(CharView *cv, GWindow pixmap );
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
extern void CVPatternTile(CharView *cv);
extern void FVPatternTile(FontView *fv);
extern void SCCharInfo(SplineChar *sc,int deflayer,EncMap *map,int enc);
extern void CharInfoDestroy(struct charinfo *ci);
extern SplineChar *SuffixCheck(SplineChar *sc,char *suffix);
extern void SCSubtableDefaultSubsCheck(SplineChar *sc, struct lookup_subtable *sub, struct matrix_data *possub, int col_cnt, int r,int layer);
extern GImage *PST_GetImage(GGadget *pstk,SplineFont *sf,int def_layer,
	struct lookup_subtable *sub,int popup_r, SplineChar *sc );
extern GImage *NameList_GetImage(SplineFont *sf,SplineChar *sc,int def_layer,
	char *namelist, int isliga );
extern GImage *GV_GetConstructedImage(SplineChar *sc,int def_layer, struct glyphvariants *gv,
	int is_horiz);
extern GImage *SC_GetLinedImage(SplineChar *sc, int def_layer, int pos, int is_italic_cor);
extern struct glyphvariants *GV_ParseConstruction(struct glyphvariants *gv,
	struct matrix_data *stuff, int rows, int cols);
extern void GV_ToMD(GGadget *g, struct glyphvariants *gv);
extern void CVGetInfo(CharView *cv);
extern void CVPGetInfo(CharView *cv);
extern int  SCUsedBySubs(SplineChar *sc);
extern void SCSubBy(SplineChar *sc);
extern void SCRefBy(SplineChar *sc);
extern void ApGetInfo(CharView *cv, AnchorPoint *ap);
extern void CVMakeClipPath(CharView *cv);
extern void CVAddAnchor(CharView *cv);
extern AnchorClass *AnchorClassUnused(SplineChar *sc,int *waslig);
extern void FVSetWidth(FontView *fv,enum widthtype wtype);
extern void CVSetWidth(CharView *cv,enum widthtype wtype);
extern void GenericVSetWidth(FontView *fv,SplineChar* sc,enum widthtype wtype);
extern void CVChangeSC(CharView *cv, SplineChar *sc );
extern Undoes *CVPreserveTState(CharView *cv);
/**
 * If isTState > 0 then CVPreserveTState(cv)
 * otherwise CVPreserveState(cv)
 */
extern Undoes *CVPreserveMaybeState(CharView *cv, int isTState );
extern void CVRestoreTOriginalState(CharView *cv);
extern void CVUndoCleanup(CharView *cv);

extern void AdjustControls(SplinePoint *sp);
extern void CVAdjustPoint(CharView *cv, SplinePoint *sp);
extern void CVMergeSplineSets(CharView *cv, SplinePoint *active, SplineSet *activess,
	SplinePoint *merge, SplineSet *mergess);
extern void CVAdjustControl(CharView *cv,BasePoint *cp, BasePoint *to);
extern int  CVMoveSelection(CharView *cv, real dx, real dy, uint32 input_state);
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
extern void LogoExpose(GWindow pixmap,GEvent *event, GRect *r,enum drawmode dm);
extern void CVDebugPointPopup(CharView *cv);

extern int GotoChar(SplineFont *sf,EncMap *map, int *merge_with_selection);

extern void CVShowPoint(CharView *cv, BasePoint *me);

extern void BitmapViewFinishNonStatic();
extern BitmapView *BitmapViewCreate(BDFChar *bc, BDFFont *bdf, FontView *fv,int enc);
extern BitmapView *BitmapViewCreatePick(int enc, FontView *fv);
extern void BitmapViewFree(BitmapView *bv);
extern void BVMenuRotateInvoked(GWindow gw,struct gmenuitem *mi, GEvent *e);
extern void BVRotateBitmap(BitmapView *bv,enum bvtools type );
extern int  BVColor(BitmapView *bv);
extern void BCGeneralFunction(BitmapView *bv,
	void (*SetPoint)(BitmapView *,int x, int y, void *data),void *data);
extern char *BVFlipNames[];
extern void BVChangeBC(BitmapView *bv, BDFChar *bc, int fitit );

extern void MVSetSCs(MetricsView *mv, SplineChar **scs);
extern void MVRefreshChar(MetricsView *mv, SplineChar *sc);
extern void MVRegenChar(MetricsView *mv, SplineChar *sc);
extern void MVReKern(MetricsView *mv);
extern void MetricsViewFinishNonStatic();
extern MetricsView *MetricsViewCreate(FontView *fv,SplineChar *sc,BDFFont *bdf);
extern void MetricsViewFree(MetricsView *mv);
extern void MVRefreshAll(MetricsView *mv);
extern void MV_FriendlyFeatures(GGadget *g, int pos);
extern GTextInfo *SLOfFont(SplineFont *sf);

extern void DoPrefs(void);
extern void DoXRes(void);
extern void PointerDlg(CharView *cv);
extern void GListAddStr(GGadget *list,unichar_t *str, void *ud);
extern void GListReplaceStr(GGadget *list,int index, unichar_t *str, void *ud);
extern struct macname *NameGadgetsGetNames( GWindow gw );
extern void NameGadgetsSetEnabled( GWindow gw, int enable );
extern int GCDBuildNames(GGadgetCreateData *gcd,GTextInfo *label,int pos,struct macname *names);
extern void GCDFillMacFeat(GGadgetCreateData *mfgcd,GTextInfo *mflabels, int width,
	MacFeat *all, int fromprefs, GGadgetCreateData *boxes,
	GGadgetCreateData **array);
extern void Prefs_ReplaceMacFeatures(GGadget *list);

extern unichar_t *FVOpenFont(char *title, const char *defaultfile, int mult);




extern void ShowAboutScreen(void);
extern void DelayEvent(void (*func)(void *), void *data);

extern void FindProblems(FontView *fv,CharView *cv,SplineChar *sc);
typedef enum
{
    constrainSelection_AveragePoints = 0,
    constrainSelection_SpacePoints = 1,
    constrainSelection_SpaceSelectedRegions = 2
} constrainSelection_t;
extern void CVConstrainSelection(CharView *cv, constrainSelection_t type);
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
extern void SV_DoClose(struct cvcontainer *cvc);
extern void SVMakeActive(SearchView *sv,CharView *cv);
extern int SVAttachFV(FontView *fv,int ask_if_difficult);
extern void SVDetachFV(FontView *fv);

extern void MKDMakeActive(MathKernDlg *mkd,CharView *cv);
extern void MKD_DoClose(struct cvcontainer *cvc);
extern void MKDCharViewInits(MathKernDlg *mkd);
extern void MathKernDialog(SplineChar *sc,int def_layer);

extern void ShowAtt(SplineFont *sf,int def_layer);
extern void FontCompareDlg(FontView *fv);
extern void SFShowKernPairs(SplineFont *sf,SplineChar *sc,AnchorClass *ac,int layer);
extern void SFShowLigatures(SplineFont *sf,SplineChar *sc);

extern void SCEditInstructions(SplineChar *sc);
extern void SFEditTable(SplineFont *sf, uint32 tag);
extern void IIScrollTo(struct instrinfo *ii,int ip,int mark_stop);
extern void IIReinit(struct instrinfo *ii,int ip);
extern int ii_v_e_h(GWindow gw, GEvent *event);
extern void instr_scroll(struct instrinfo *ii,struct sbevent *sb);

extern void CVGridFitChar(CharView *cv);
/**
 * If a live preview of grid fit is somehow in effect, call CVGridFitChar() for us.
 * A caller can call here after a change and any CVGridFitChar() will be updated if need be.
 */
extern void CVGridHandlePossibleFitChar(CharView *cv);
extern void CVFtPpemDlg(CharView *cv,int debug);
extern void SCDeGridFit(SplineChar *sc);
extern void SCReGridFit(SplineChar *sc,int layer);

extern void CVDebugReInit(CharView *cv,int restart_debug,int dbg_fpgm);
extern void CVDebugFree(DebugView *dv);
extern int DVChar(DebugView *dv, GEvent *e);

extern void KernClassD(KernClass *kc, SplineFont *sf, int layer, int isv);
extern void ShowKernClasses(SplineFont *sf,MetricsView *mv,int layer,int isv);
extern void KCLD_End(struct kernclasslistdlg *kcld);
extern void KCLD_MvDetach(struct kernclasslistdlg *kcld,MetricsView *mv);
extern void KernPairD(SplineFont *sf,SplineChar *sc1,SplineChar *sc2,int layer, int isv);
extern void KCD_DrawGlyph(GWindow pixmap,int x,int baseline,BDFChar *bdfc,int mag);
extern GTextInfo *BuildFontList(FontView *except);
extern void TFFree(GTextInfo *tf);

extern void AnchorControl(SplineChar *sc,AnchorPoint *ap,int layer);
extern void AnchorControlClass(SplineFont *_sf,AnchorClass *ac,int layer);

extern void FVSelectByPST(FontView *fv);

enum hist_type { hist_hstem, hist_vstem, hist_blues };
struct psdict;
extern void SFHistogram(SplineFont *sf,int layer, struct psdict *private,uint8 *selected,
	EncMap *map, enum hist_type which);

extern void ContextChainEdit(SplineFont *sf,FPST *fpst,
	struct gfi_data *gfi,unichar_t *newname,int layer);
extern char *cu_copybetween(const unichar_t *start, const unichar_t *end);

extern void StateMachineEdit(SplineFont *sf,ASM *sm,struct gfi_data *d);
extern void GFI_FinishSMNew(struct gfi_data *d,ASM *sm, int success,int isnew);

extern void MMChangeBlend(MMSet *mm,FontView *fv,int tonew);
extern void MMWizard(MMSet *mm);

extern int LayerDialog(Layer *layer,SplineFont *sf);
extern void CVLayerChange(CharView *cv);

extern int PointOfViewDlg(struct pov_data *pov,SplineFont *sf,int flags);

extern SplineChar *FVMakeChar(FontView *fv,int i);

extern void CVPointOfView(CharView *cv,struct pov_data *);

extern void DVCreateGloss(DebugView *dv);
extern void DVMarkPts(DebugView *dv,SplineSet *ss);
extern int CVXPos(DebugView *dv,int offset,int width);

extern GMenuItem *GetEncodingMenu(void (*func)(GWindow,GMenuItem *,GEvent *),
	Encoding *current);

extern GTextInfo *TIFromName(const char *name);

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
extern GTextInfo *SFLookupArrayFromType(SplineFont *sf, int lookup_type );
extern GTextInfo *SFLookupArrayFromMask(SplineFont *sf, int lookup_mask );
extern GTextInfo **SFSubtablesOfType(SplineFont *sf, int lookup_type, int kernclass, int add_none);
extern GTextInfo *SFSubtableListOfType(SplineFont *sf, int lookup_type, int kernclass, int add_none);
extern struct lookup_subtable *SFNewLookupSubtableOfType(SplineFont *sf, int lookup_type, struct subtable_data *sd, int def_layer );
extern int EditLookup(OTLookup *otl,int isgpos,SplineFont *sf);
extern int EditSubtable(struct lookup_subtable *sub,int isgpos,SplineFont *sf,
	struct subtable_data *sd,int def_layer);
extern void _LookupSubtableContents(SplineFont *sf, struct lookup_subtable *sub,
	struct subtable_data *sd,int def_layer);
extern char *SCNameUniStr(SplineChar *sc);
extern unichar_t *uSCNameUniStr(SplineChar *sc);
extern char *SFNameList2NameUni(SplineFont *sf, char *str);
extern unichar_t **SFGlyphNameCompletion(SplineFont *sf,GGadget *t,int from_tab,
	int new_name_after_space);
extern char *GlyphNameListDeUnicode( char *str );
extern void AddRmLang(SplineFont *sf, struct lkdata *lk,int add_lang);
extern void FVMassGlyphRename(FontView *fv);

extern void SFBdfProperties(SplineFont *sf, EncMap *map, BDFFont *thisone);



extern GMenuItem2 helplist[];
extern BasePoint last_ruler_offset[];

extern void CVCopyLayerToLayer(CharView *cv);
extern void FVCopyLayerToLayer(FontView *fv);
extern void CVCompareLayerToLayer(CharView *cv);
extern void FVCompareLayerToLayer(FontView *fv);

extern void MathInit(void);
extern void SFMathDlg(SplineFont *sf,int def_layer);

extern GMenuItem2 *cvpy_menu, *fvpy_menu;
extern void cvpy_tllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void fvpy_tllistcheck(GWindow gw,struct gmenuitem *mi,GEvent *e);

extern GMenuItem2 *cv_menu, *fv_menu;
extern void cv_tl2listcheck(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void fv_tl2listcheck(GWindow gw,struct gmenuitem *mi,GEvent *e);

extern void SFValidationWindow(SplineFont *sf,int layer, enum fontformat format);
extern void ValidationDestroy(SplineFont *sf);



extern const char *UI_TTFNameIds(int id);
extern const char *UI_MSLangString(int language);
extern void FontInfoInit(void);
extern void LookupUIInit(void);
extern enum psstrokeflags Ps_StrokeFlagsDlg(void);
extern struct cidmap *AskUserForCIDMap(void);

extern void DefineGroups(struct fontview *fv);
extern void DisplayGroups(struct fontview *fv);

extern struct Base *SFBaselines(SplineFont *sf,struct Base *old,int is_vertical);
extern void JustifyDlg(SplineFont *sf);
extern char *GlyphListDlg(SplineFont *sf, char *glyphstr);

extern void DeltaSuggestionDlg(FontView *fv,CharView *cv);
extern void QGRmFontView(struct qg_data *qg,FontView *fv);
extern void QGRmCharView(struct qg_data *qg,CharView *cv);


extern struct hslrgb *SFFontCols(SplineFont *sf,struct hslrgb fontcols[6]);

extern Color view_bgcol;	/* Background color for views */
extern void MVColInit(void);
extern void CVColInit( void );

extern void FontViewRemove(FontView *fv);
extern void FontViewFinishNonStatic();
extern void FVChar(FontView *fv,GEvent *event);
extern void FVDrawInfo(FontView *fv,GWindow pixmap,GEvent *event);
extern void FVRedrawAllCharViews(FontView *fv);
extern void KFFontViewInits(struct kf_dlg *kf,GGadget *drawable);
extern char *GlyphSetFromSelection(SplineFont *sf,int def_layer,char *current);
extern void ME_ListCheck(GGadget *g,int r, int c, SplineFont *sf);
extern void ME_SetCheckUnique(GGadget *g,int r, int c, SplineFont *sf);
extern void ME_ClassCheckUnique(GGadget *g,int r, int c, SplineFont *sf);
extern void PI_Destroy(struct dlistnode *node);
struct gidata;
extern void PIChangePoint(struct gidata *ci);

extern void CVRegenFill(CharView *cv);
extern void RulerDlg(CharView *cv);
extern int  CVCountSelectedPoints(CharView *cv);
extern void _CVMenuInsertPt(CharView *cv);
extern void _CVMenuNamePoint(CharView *cv, SplinePoint *sp);
extern void _CVMenuNameContour(CharView *cv);

extern void Prefs_LoadDefaultPreferences( void );


extern CharView* CharViewFindActive();
extern FontViewBase* FontViewFindActive();
extern FontViewBase* FontViewFind( int (*testFunc)( FontViewBase*, void* ), void* udata );

extern int FontViewFind_byXUID(      FontViewBase* fv, void* udata );
extern int FontViewFind_byXUIDConnected( FontViewBase* fv, void* udata );
extern int FontViewFind_byCollabPtr(  FontViewBase* fv, void* udata );
extern int FontViewFind_bySplineFont( FontViewBase* fv, void* udata );
extern int FontViewFind_byCollabBasePort( FontViewBase* fv, void* udata );

extern void SPSelectNextPoint( SplinePoint *sp, int state );
extern void SPSelectPrevPoint( SplinePoint *sp, int state );


/**
 * Is the next BCP for the sp selected, and is it the primary BCP for the selection
 * @see SPIsNextCPSelected
 */
extern bool SPIsNextCPSelectedSingle( SplinePoint *sp, CharView *cv );
/**
 * Is the prev BCP for the sp selected, and is it the primary BCP for the selection
 * @see SPIsNextCPSelected
 */
extern bool SPIsPrevCPSelectedSingle( SplinePoint *sp, CharView *cv );
/**
 * Is the next BCP for the sp selected, it can be the primary or any
 * of the secondary selected BCP
 *
 * The last selected BCP is the 'primary' selected BCP. Code which
 * only handles a single selected BCP will only honor the primary
 * selected BCP
 *
 * There can also be one or more seconday selected BCP. These might be
 * drawn with slightly less highlight graphically and are only handled
 * by code which has been updated to allow mutliple selected BCP to be
 * operated on at once.
 */
extern bool SPIsNextCPSelected( SplinePoint *sp, CharView *cv );
/**
 * Is the prev BCP for the sp selected, it can be the primary or any of the secondary selected BCP
 *
 * @see SPIsNextCPSelected
 */
extern bool SPIsPrevCPSelected( SplinePoint *sp, CharView *cv );

typedef struct FE_adjustBCPByDeltaDataS
{
    CharView *cv; //< used to update view
    real dx;      //< Add this to the BCP x
    real dy;      //< Add this to the BCP y
    int keyboarddx;

} FE_adjustBCPByDeltaData;


/**
 * Visitor function type for visitSelectedControlPoints()
 */
typedef void (*visitSelectedControlPointsVisitor) ( void* key,
						    void* value,
						    SplinePoint* sp,
						    BasePoint *which,
						    bool isnext,
						    void* udata );

/**
 * Visitor function to move each BCP by data->dx/data->dy
 *
 *
 * Visitor: visitSelectedControlPointsVisitor
 * UsedBy:  CVFindAndVisitSelectedControlPoints
 */
extern void FE_adjustBCPByDelta( void* key,
				 void* value,
				 SplinePoint* sp,
				 BasePoint *which,
				 bool isnext,
				 void* udata );

extern void FE_adjustBCPByDeltaWhilePreservingBCPAngle( void* key,
							void* value,
							SplinePoint* sp,
							BasePoint *which,
							bool isnext,
							void* udata );

/**
 * Visitor function to unselect every BCP passed
 *
 * Visitor: visitSelectedControlPointsVisitor
 * UsedBy:  CVFindAndVisitSelectedControlPoints
 *          CVUnselectAllBCP
 *
 * @see SPIsNextCPSelected
 */
extern void FE_unselectBCP( void* key,
			    void* value,
			    SplinePoint* sp,
			    BasePoint *which,
			    bool isnext,
			    void* udata );

extern void FE_touchControlPoint( void* key,
				  void* value,
				  SplinePoint* sp,
				  BasePoint *which,
				  bool isnext,
				  void* udata );

/**
 * Find all the selected BCP and apply the visitor function f to them
 * passing the user data pointer udata to the 'f' visitor.
 *
 * This function doesn't use udata at all, it simply passes it on to
 * your visitor function so it may do something with it like record
 * results or take optional parameters.
 *
 * If preserveState is true and there are selected BCP then
 * CVPreserveState() is called before the visitor function.
 */
extern void CVFindAndVisitSelectedControlPoints( CharView *cv, bool preserveState,
						 visitSelectedControlPointsVisitor f, void* udata );
/**
 * NOTE: doesn't do all, just all on selected spline.
 */
extern void CVVisitAllControlPoints( CharView *cv, bool preserveState,
				     visitSelectedControlPointsVisitor f, void* udata );

/**
 * Unselect all the BCP which are currently selected.
 */
extern void CVUnselectAllBCP( CharView *cv );


/**
 * This will call your visitor function 'f' on any selected BCP. This
 * is regardless of if the BCP is the next or prev BCP for it's
 * splinepoint.
 *
 * This function doesn't use udata at all, it simply passes it on to
 * your visitor function so it may do something with it like record
 * results or take optional parameters.
 */
extern void visitSelectedControlPoints( GHashTable *col, visitSelectedControlPointsVisitor f, gpointer udata );
/**
 * NOTE: doesn't do all, just all on selected spline.
 */
extern void visitAllControlPoints( GHashTable *col, visitSelectedControlPointsVisitor f, gpointer udata );

extern void CVVisitAdjacentToSelectedControlPoints( CharView *cv, bool preserveState,
						    visitSelectedControlPointsVisitor f, void* udata );

extern void CVFreePreTransformSPL( CharView* cv );

extern bool CVShouldInterpolateCPsOnMotion( CharView* cv );

extern int CVNearRBearingLine( CharView* cv, real x, real fudge );
extern int CVNearLBearingLine( CharView* cv, real x, real fudge );

extern void CVMenuConstrain(GWindow gw, struct gmenuitem *mi, GEvent *UNUSED(e));



#endif	/* _VIEWS_H */
