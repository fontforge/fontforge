/* Copyright (C) 2000-2004 by George Williams */
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
#include "ggadget.h"

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
    int showblues, showfamilyblues;
    int showanchor;
} CVShows;

extern struct bvshows {
    int showfore, showoutline, showgrid;
    int lastpixelsize;
} BVShows;

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
/*    unsigned int width: 1;	/* we're moving the width rather than a spline */
/*    unsigned int vwidth: 1;	/* we're moving the width rather than a spline */
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

/* Note: These are ordered as they are displayed in the tools palette */
enum cvtools { cvt_pointer, cvt_magnify,
	cvt_freehand, cvt_hand,
	cvt_curve, cvt_corner, cvt_tangent, cvt_pen,
	cvt_knife, cvt_ruler,
	cvt_scale, cvt_flip,
	cvt_rotate, cvt_skew,
	cvt_rect, cvt_poly,
	cvt_elipse, cvt_star,
	cvt_minify, cvt_max=cvt_minify,
	cvt_none = -1};
enum bvtools { bvt_pointer, bvt_magnify,
	bvt_pencil, bvt_line,
	bvt_shift, bvt_hand,
	bvt_minify, bvt_eyedropper, bvt_max=bvt_eyedropper,
	bvt_setwidth,
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
    GWindow v;
    GGadget *vsb;
    int16 sbw;
    int16 vheight, vwidth;
    int16 lheight,lpos;
    int16 lstopped;
    int16 as, fh;
    struct instrdata *instrdata;
    GFont *gfont;
    int isel_pos;
    unsigned int showaddr: 1;
    unsigned int showhex: 1;
    unsigned int mousedown: 1;
    void *userdata;
    void (*selection_callback)(struct instrinfo *,int ip);
    int  (*bpcheck)(struct instrinfo *,int ip);
    int  (*handle_char)(struct instrinfo *,GEvent *e);
};

typedef struct debugview {
    struct debugger_context *dc;	/* Local to freetype.c */
    GWindow dv, v;
    struct instrdata id;
    struct instrinfo ii;
    int dwidth, toph;
    struct charview *cv;
    double scale;
    /* Windows for twilight points, cvt, registers, stack, storage */
    GWindow points, points_v, cvt, regs, stack, storage;
    int pts_head, cvt_offtop;
    GGadget *cvtsb;
} DebugView;

enum dv_coderange { cr_none=0, cr_fpgm, cr_prep, cr_glyph };	/* cleverly chosen to match ttobjs.h */

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
    Layer *layerheads[dm_max];
    real scale;
    GWindow gw, v;
    int width, height;
    int xoff, yoff;
    int mbh, infoh, rulerh;
    GGadget *vsb, *hsb, *mb;
    GFont *small, *normal;
    int16 sas, sfh, nas, nfh;
    BasePoint info;
    SplinePoint *info_sp;
    GPoint e;					/* mouse location */
    GPoint olde;
    BasePoint last_c;
    BDFChar *filled;
    GImage gi;
    struct charview *next;
    struct fontview *fv;
    GWindow icon;
    PressedOn p;
    SplinePoint *lastselpt;
    /*GWindow tools, layers;*/
    int8 b1_tool, cb1_tool, b2_tool, cb2_tool;		/* Button 3 does a popup */
    int8 s1_tool, s2_tool, er_tool;			/* Bindings for wacom stylus and eraser */
    int8 showing_tool, pressed_tool, pressed_display, had_control, active_tool;
    SplinePointList *active_spl;
    SplinePoint *active_sp;
    IPoint handscroll_base;
    GWindow ruler_w;
    uint16 rfh, ras;
    GFont *rfont;
    BasePoint lastknife;
    struct freehand {
	struct tracedata *head, *last;	/* for the freehand tool */
	SplinePointList *current_trace;
	int ignore_wobble;		/* Ignore wiggles smaller than this */
	int skip_cnt;
    } freehand;
    GTimer *pressed;
    GWindow backimgs;
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
    GTimer *autorpt;
    int keysym, oldstate;
    int oldkeyx, oldkeyy;
    GWindow oldkeyw;
#endif
    struct searchview *searcher;	/* The sv within which this view is embedded (if it is) */
    GIC *gic;
    PST *lcarets;
    int16 nearcaret;
	/* freetype results display */
    int16 ft_dpi, ft_ppem, ft_gridfitwidth;
    real ft_pointsize;
    SplineSet *gridfit;
    struct freetype_raster *raster;
    DebugView *dv;
    uint32 mmvisible;
} CharView;

typedef struct bitmapview {
    BDFChar *bc;
    BDFFont *bdf;
    struct fontview *fv;
    GWindow gw, v;
    int xoff, yoff;
    GGadget *vsb, *hsb, *mb;
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
    GGadget *recalc;
    int8 b1_tool, cb1_tool, b2_tool, cb2_tool;		/* Button 3 does a popup */
    int8 s1_tool, s2_tool, er_tool;			/* Bindings for wacom stylus and eraser */
    int8 showing_tool, pressed_tool, pressed_display, had_control, active_tool;
    int pressed_x, pressed_y;
    int info_x, info_y;
    int event_x, event_y;
    GFont *small;
    int16 sas, sfh;
#if _ModKeysAutoRepeat
    GTimer *autorpt;
    int keysym, oldstate;
#endif
    int color;			/* for greyscale fonts */
} BitmapView;

struct aplist { AnchorPoint *ap; int connected_to, selected; struct aplist *next; };

typedef struct metricsview {
    struct fontview *fv;
    int pixelsize;
    BDFFont *bdf;		/* We can also see metric info on a bitmap font */
    GWindow gw;
    int16 width, height, dwidth;
    int16 mbh,sbh;
    int16 topend;		/* y value of the end of the region containing the text field */
    int16 displayend;		/* y value of the end of the region showing filled characters */
    GFont *font;
    int16 fh, as;
    GGadget *hsb, *vsb, *mb, *text, *sli_list;
    GGadget *namelab, *widthlab, *lbearinglab, *rbearinglab, *kernlab;
    struct metricchar {
	SplineChar *sc;
	BDFChar *show;
	int16 dx, dwidth;	/* position and width of the displayed char */
	int16 dy, dheight;	/*  displayed info for vertical metrics */
	int16 mx, mwidth;	/* position and width of the text underneath */
	int16 kernafter;
	int16 xoff, yoff, hoff, voff;	/* adjustments by GPOS other than 'kern' (scaled) */
	PST *active_pos;	/* Only support one simple positioning GPOS feature at a time */
	unsigned int selected: 1;
	GGadget *width, *lbearing, *rbearing, *kern, *name;
	struct aplist *aps;
	SplineChar *basesc;	/* If we've done a substitution, this lets us go back */
    } *perchar;
    SplineChar **sstr;		/* An array the same size as perchar (well 1 bigger, trailing null) */
    int16 mwidth, mbase;
    int16 charcnt, max;
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
    int cur_sli;
} MetricsView;

enum fv_metrics { fvm_baseline=1, fvm_origin=2, fvm_advanceat=4, fvm_advanceto=8 };
typedef struct fontview {
    SplineFont *sf;
    BDFFont *show, *filled;
    GWindow gw, v;
    int width, height;		/* of v */
    int16 infoh,mbh;
    int colcnt, rowcnt;
    int rowoff, rowltot;
    int cbw,cbh;		/* width/height of a character box */
    GFont *header, *iheader;
    GGadget *vsb, *mb;
    struct fontview *next;		/* Next on list of open fontviews */
    struct fontview *nextsame;		/* Next fv looking at this font */
    int pressed_pos, end_pos;
    GTimer *pressed;
    uint8 *selected;
    MetricsView *metrics;
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
    int16 magnify;
    SplineFont *cidmaster;
    int32 *mapping;	/* an array mapping grid cells (0=upper left) to font indeces (enc, 0=NUL) */
		    /* So the default array would contain NUL, ^A, ^B, ... */
    int mapcnt;		/* Number of chars in the current group (mapping) */
    struct dictionary *fontvars;	/* Scripting */
    struct searchview *sv;
    GIC *gic;
    GTimer *resize;
    struct gfi_data *fontinfo;
    SplineChar *sc_near_top;
    int sel_index;
} FontView;

typedef struct findsel {
    GEvent *e;
    real fudge;		/* One pixel fudge factor */
    real xl,xh, yl, yh;	/* One pixel fudge factor */
    unsigned int select_controls: 1;	/* notice control points */
    unsigned int seek_controls: 1;	/* notice control points before base points */
    real scale;
    PressedOn *p;
} FindSel;

enum widthtype { wt_width, wt_lbearing, wt_rbearing, wt_vwidth };

typedef struct searchview {
    FontView dummy_fv;
    SplineFont dummy_sf;
    SplineChar sc_srch, sc_rpl;
    SplineChar *chars[2];
    uint8 sel[2];
    CharView cv_srch, cv_rpl;
    CharView *lastcv;
/* ****** */
    GWindow gw;
    int mbh;
    GGadget *mb;
    GFont *plain, *bold;
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
#else
    unsigned long matched_refs;
    unsigned long matched_ss;
#endif
} SearchView;

enum fvtrans_flags { fvt_dobackground=1, fvt_round_to_int=2,
	fvt_dontsetwidth=4, fvt_dontmovewidth=8 };

extern void FVSetTitle(FontView *fv);
extern FontView *_FontViewCreate(SplineFont *sf);
extern FontView *FontViewCreate(SplineFont *sf);
extern void FVScrollToChar(FontView *fv,int i);
extern void SplineFontSetUnChanged(SplineFont *sf);
extern FontView *ViewPostscriptFont(char *filename);
extern FontView *FontNew(void);
extern void FontViewFree(FontView *fv);
extern void FVToggleCharChanged(SplineChar *sc);
extern void FVRefreshChar(FontView *fv,BDFFont *bdf,int enc);
extern void FVRegenChar(FontView *fv,SplineChar *sc);
extern int _FVMenuSave(FontView *fv);
extern int _FVMenuSaveAs(FontView *fv);
extern int _FVMenuGenerate(FontView *fv,int family);
extern void _FVCloseWindows(FontView *fv);
extern void SCClearBackground(SplineChar *sc);
extern char *GetPostscriptFontName(char *defdir,int mult);
extern void MenuPrefs(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuSaveAll(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuExit(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuOpen(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuHelp(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuIndex(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuAbout(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuLicense(GWindow base,struct gmenuitem *mi,GEvent *e);
extern void MenuNew(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void MergeKernInfo(SplineFont *sf);
extern int32 *ParseBitmapSizes(GGadget *g,int msg,int *err);
#ifdef FONTFORGE_CONFIG_WRITE_PFM
extern int WritePfmFile(char *filename,SplineFont *sf, int type0);
#endif
extern int GenerateScript(SplineFont *sf,char *filename,char *bitmaptype,
	int fmflags,int res, char *subfontdirectory,struct sflist *sfs);
extern int SFGenerateFont(SplineFont *sf,int family);
extern GTextInfo *AnchorClassesList(SplineFont *sf);
extern GTextInfo **AnchorClassesLList(SplineFont *sf);
extern GTextInfo **AnchorClassesSimpleLList(SplineFont *sf);
extern GTextInfo *AddMacFeatures(GTextInfo *opentype,enum possub_type type,SplineFont *sf);
extern unichar_t *ClassName(const unichar_t *name,uint32 feature_tag,
	uint16 flags, int script_lang_index, int merge_with, int act_type,
	int macfeature);
extern unichar_t *DecomposeClassName(const unichar_t *clsnm, unichar_t **name,
	uint32 *feature_tag, int *macfeature,
	uint16 *flags, uint16 *script_lang_index,int *merge_with,int *act_type);
extern unichar_t *AskNameTag(int title,unichar_t *def,uint32 def_tag,uint16 flags,
	int script_lang_index, enum possub_type type, SplineFont *sf, SplineChar *default_script,
	int merge_with,int act_type);
extern PST *AddSubs(PST *last,uint32 tag,char *name,uint16 flags,
	uint16 sli,SplineChar *sc);
extern int  SLICount(SplineFont *sf);
extern unichar_t *ShowScripts(unichar_t *usedef);
extern GTextInfo *SFLangList(SplineFont *sf,int addfinal,SplineChar *default_script);
extern GTextInfo **SFLangArray(SplineFont *sf,int addfinal);
extern int  ScriptLangList(SplineFont *sf,GGadget *list,int sli);
extern void GListDelSelected(GGadget *list);
extern void GListMoveSelected(GGadget *list,int offset);
extern void GListChangeLine(GGadget *list,int pos, const unichar_t *line);
extern GTextInfo *GListAppendLine(GGadget *list,const unichar_t *line,int select);
extern void FontInfo(SplineFont *sf,int aspect,int sync);
extern void FontInfoDestroy(FontView *fv);
extern void FontMenuFontInfo(void *fv);
extern void GFI_FinishContextNew(struct gfi_data *d,FPST *fpst, unichar_t *newname,
	int success);
extern void GFI_CCDEnd(struct gfi_data *d);
extern void DumpPfaEditEncodings(void);
extern void ParseEncodingFile(char *filename);
extern void LoadEncodingFile(void);
extern struct enc *MakeEncoding(SplineFont *sf);
extern void RemoveEncoding(void);
extern void SFPrivateInfo(SplineFont *sf);
extern void OrderTable(SplineFont *sf,uint32 table_tag);
extern void FontViewReformatAll(SplineFont *sf);
extern void FontViewReformatOne(FontView *fv);
extern void FVShowFilled(FontView *fv);
extern void FVChangeDisplayBitmap(FontView *fv,BDFFont *bdf);
extern void SCPreparePopup(GWindow gw,SplineChar *sc);
extern int SFScaleToEm(SplineFont *sf, int ascent, int descent);
extern void TransHints(StemInfo *stem,real mul1, real off1, real mul2, real off2, int round_to_int );
extern void FVTransFunc(void *_fv,real transform[6],int otype, BVTFunc *bvts,
	enum fvtrans_flags );
extern void FVTrans(FontView *fv,SplineChar *sc,real transform[6],uint8 *sel,
	enum fvtrans_flags);
extern int SFNLTrans(FontView *fv,char *x_expr,char *y_expr);
extern void FVApplySubstitution(FontView *fv,uint32 script, uint32 lang, uint32 tag);
extern void NonLinearDlg(FontView *fv,CharView *cv);
extern void FVBuildAccent(FontView *fv,int onlyaccents);
extern void FVChangeChar(FontView *fv,int encoding);
extern void SCClearContents(SplineChar *sc);
extern void SCClearAll(SplineChar *sc);
extern void BCClearAll(BDFChar *bc);
extern void UnlinkThisReference(FontView *fv,SplineChar *sc);
extern void FVFakeMenus(FontView *fv,int cmd);
extern void FVMetricsCenter(FontView *fv,int docenter);
extern void MergeFont(FontView *fv,SplineFont *other);
extern void FVMergeFonts(FontView *fv);
extern void FVInterpolateFonts(FontView *fv);
extern void FVRevert(FontView *fv);
extern void FVDelay(FontView *fv,void (*func)(FontView *));

extern void FVDeselectAll(FontView *fv);

extern void FVAutoKern(FontView *fv);
extern void FVAutoWidth(FontView *fv);
extern void FVRemoveKerns(FontView *fv);
extern void FVRemoveVKerns(FontView *fv);
extern void FVVKernFromHKern(FontView *fv);
extern int AutoWidthScript(SplineFont *sf,int spacing);
extern int AutoKernScript(SplineFont *sf,int spacing, int threshold,char *kernfile);

extern void CVDrawSplineSet(CharView *cv, GWindow pixmap, SplinePointList *set,
	Color fg, int dopoints, DRect *clip );

extern GWindow CVMakeTools(CharView *cv);
extern GWindow CVMakeLayers(CharView *cv);
extern GWindow BVMakeTools(BitmapView *bv);
extern GWindow BVMakeLayers(BitmapView *bv);
extern int CVPaletteMnemonicCheck(GEvent *event);
extern real CVRoundRectRadius(void);
extern int CVRectElipseCenter(void);
extern real CVStarRatio(void);
extern int CVPolyStarPoints(void);
extern StrokeInfo *CVFreeHandInfo(void);
extern int TrueCharState(GEvent *event);
extern void BVToolsSetCursor(BitmapView *bv, int state,char *device);
extern void CVToolsSetCursor(CharView *cv, int state,char *device);
extern void CVToolsPopup(CharView *cv, GEvent *event);
extern void BVToolsPopup(BitmapView *bv, GEvent *event);
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
extern void BVPaletteChangedChar(BitmapView *bv);
extern void CVPaletteDeactivate(void);
extern void PalettesChangeDocking(void);
extern int CVPalettesWidth(void);
extern int BVPalettesWidth(void);

extern void BackgroundImageTransform(SplineChar *sc, ImageList *img,real transform[6]);
extern void CVTransFunc(CharView *cv,real transform[6],enum fvtrans_flags);
extern void skewselect(BVTFunc *bvtf,real t);
extern void TransformDlgCreate(void *data,void (*transfunc)(void *,real *,int,BVTFunc *,enum fvtrans_flags),
	int (*getorigin)(void *,BasePoint *,int), int enableback);
extern void BitmapDlg(FontView *fv,SplineChar *sc, int isavail);
extern int BitmapControl(FontView *fv,int32 *sizes,int isavail);
extern void _FVSimplify(FontView *fv,struct simplifyinfo *smpl);
extern int SimplifyDlg(SplineFont *sf,struct simplifyinfo *smpl);
extern void CVReviewHints(CharView *cv);
extern void CVCreateHint(CharView *cv,int ishstem);
extern void SCClearRounds(SplineChar *sc);
extern void SCRemoveSelectedMinimumDistances(SplineChar *sc,int inx);
extern int _ExportPDF(FILE *pdf,SplineChar *sc);
extern int _ExportEPS(FILE *eps,SplineChar *sc);
extern int _ExportSVG(FILE *svg,SplineChar *sc);
extern int CVExport(CharView *cv);
extern int BVExport(BitmapView *bv);
extern void ScriptExport(SplineFont *sf, BDFFont *bdf, int format, int enc);

extern void DrawAnchorPoint(GWindow pixmap,int x, int y,int selected);
extern void DefaultY(GRect *pos);
extern void CVResize(CharView *cv );
extern CharView *CharViewCreate(SplineChar *sc,FontView *fv);
extern void CharViewFree(CharView *cv);
extern int CVValid(SplineFont *sf, SplineChar *sc, CharView *cv);
extern void CVDrawRubberRect(GWindow pixmap, CharView *cv);
extern void CVSetCharChanged(CharView *cv,int changed);
extern void _CVCharChangedUpdate(CharView *cv,int changed);
extern void CVCharChangedUpdate(CharView *cv);
extern void SCClearSelPt(SplineChar *sc);
extern void _SCCharChangedUpdate(SplineChar *sc,int changed);
extern void SCCharChangedUpdate(SplineChar *sc);
extern void SCSynchronizeWidth(SplineChar *sc,real newwidth, real oldwidth,FontView *fv);
extern void SCSynchronizeLBearing(SplineChar *sc,char *selected,real off);
extern int CVAnySel(CharView *cv, int *anyp, int *anyr, int *anyi, int *anya);
extern int CVTwoForePointsSelected(CharView *cv, SplinePoint **sp1, SplinePoint **sp2);
extern int CVIsDiagonalable(SplinePoint *sp1, SplinePoint *sp2, SplinePoint **sp3, SplinePoint **sp4);
extern int CVClearSel(CharView *cv);
extern int CVSetSel(CharView *cv,int mask);
extern int CVAllSelected(CharView *cv);
extern void SCUpdateAll(SplineChar *sc);
extern void SCOutOfDateBackground(SplineChar *sc);
extern SplinePointList *CVAnySelPointList(CharView *cv);
extern SplinePoint *CVAnySelPoint(CharView *cv);
extern int CVOneThingSel(CharView *cv, SplinePoint **sp, SplinePointList **spl,
	RefChar **ref, ImageList **img, AnchorPoint **ap);
extern int CVOneContourSel(CharView *cv, SplinePointList **_spl,
	RefChar **ref, ImageList **img);
extern void RevertedGlyphReferenceFixup(SplineChar *sc, SplineFont *sf);
extern void CVInfoDraw(CharView *cv, GWindow pixmap );
enum fvformats { fv_bdf, fv_ttf, fv_pk, fv_pcf, fv_mac, fv_win, fv_image,
	fv_imgtemplate, fv_eps, fv_epstemplate, fv_svg, fv_svgtemplate,
	fv_fig };
extern void SCImportSVG(SplineChar *sc,int layer,char *path,int doclear);
extern void SCImportPSFile(SplineChar *sc,int layer,FILE *ps,int doclear);
extern void SCAddScaleImage(SplineChar *sc,GImage *image,int doclear,int layer);
extern void SCInsertImage(SplineChar *sc,GImage *image,real scale,real yoff, real xoff, int layer);
extern void CVImport(CharView *cv);
extern void BVImport(BitmapView *bv);
extern void FVImport(FontView *bv);
extern int FVImportImages(FontView *fv,char *path,int isimage);
extern int FVImportImageTemplate(FontView *fv,char *path,int isimage);
extern int FVImportBDF(FontView *fv, char *filename,int ispk, int toback);
extern int FVImportMult(FontView *fv, char *filename,int toback,int bf);
extern SplineFont *SFFromBDF(char *filename,int ispk,int toback);
extern SplineFont *SFFromMF(char *filename);
extern BDFFont *SFImportBDF(SplineFont *sf, char *filename, int ispk, int toback);
extern void CVFindCenter(CharView *cv, BasePoint *bp, int nosel);
extern void CVStroke(CharView *cv);
extern void FVStroke(FontView *fv);
extern void FVStrokeItScript(FontView *fv, StrokeInfo *si);
extern void FreeHandStrokeDlg(StrokeInfo *si);
extern void SCStroke(SplineChar *sc);
extern void FVOutline(FontView *fv, real width);
extern void FVInline(FontView *fv, real width, real inset);
extern void FVShadow(FontView *fv,real angle, real outline_width,
	real shadow_length,int wireframe);
extern void OutlineDlg(FontView *fv, CharView *cv,MetricsView *mv,int isinline);
extern void ShadowDlg(FontView *fv, CharView *cv,MetricsView *mv,int wireframe);
extern void CVTile(CharView *cv);
extern void FVTile(FontView *fv);
extern void SCTile(SplineChar *sc);
extern void MDReplace(MinimumDistance *md,SplineSet *old,SplineSet *rpl);
extern void SCAppendPosSub(SplineChar *sc,enum possub_type type, char **d);
extern void SCCharInfo(SplineChar *sc);
extern void CharInfoDestroy(struct charinfo *ci);
extern void PI_ShowHints(SplineChar *sc, GGadget *list, int set);
extern GTextInfo *SCHintList(SplineChar *sc,HintMask *);
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
extern void FVSetWidthScript(FontView *fv,enum widthtype wtype,int val,int incr);
extern void CVChangeSC(CharView *cv, SplineChar *sc );
extern void SCRefreshTitles(SplineChar *sc);
extern void SPChangePointType(SplinePoint *sp, int pointtype);

extern void CVAdjustPoint(CharView *cv, SplinePoint *sp);
extern void CVMergeSplineSets(CharView *cv, SplinePoint *active, SplineSet *activess,
	SplinePoint *merge, SplineSet *mergess);
extern void CVChar(CharView *cv, GEvent *event );
extern void CVAdjustControl(CharView *cv,BasePoint *cp, BasePoint *to);
extern int  CVMoveSelection(CharView *cv, real dx, real dy);
extern int  CVTestSelectFromEvent(CharView *cv,GEvent *event);
extern void CVMouseDownPoint(CharView *cv);
extern void CVMouseMovePoint(CharView *cv,PressedOn *);
extern void CVMouseMovePen(CharView *cv, PressedOn *p);
extern void CVMouseUpPoint(CharView *cv,GEvent *event);
extern void CVMouseUpPointer(CharView *cv );
extern int  CVMouseMovePointer(CharView *cv );
extern void CVMouseDownPointer(CharView *cv, FindSel *fs, GEvent *event);
extern void CVCheckResizeCursors(CharView *cv);
extern void CVMouseDownRuler(CharView *cv, GEvent *event);
extern void CVMouseMoveRuler(CharView *cv, GEvent *event);
extern void CVMouseUpRuler(CharView *cv, GEvent *event);
extern void CVMouseDownHand(CharView *cv);
extern void CVMouseMoveHand(CharView *cv, GEvent *event);
extern void CVMouseUpHand(CharView *cv);
extern void CVMouseDownFreeHand(CharView *cv, GEvent *event);
extern void CVMouseMoveFreeHand(CharView *cv, GEvent *event);
extern void CVMouseUpFreeHand(CharView *cv, GEvent *event);
extern void CVMouseDownTransform(CharView *cv);
extern void CVMouseMoveTransform(CharView *cv);
extern void CVMouseUpTransform(CharView *cv);
extern void CVMouseDownKnife(CharView *cv);
extern void CVMouseMoveKnife(CharView *cv,PressedOn *);
extern void CVMouseUpKnife(CharView *cv);
extern void CVMouseDownShape(CharView *cv);
extern void CVMouseMoveShape(CharView *cv);
extern void CVMouseUpShape(CharView *cv);
extern void LogoExpose(GWindow pixmap,GEvent *event, GRect *r,enum drawmode dm);

extern int GotoChar(SplineFont *sf);

extern int CVLayer(CharView *cv);
extern Undoes *CVPreserveState(CharView *cv);
extern Undoes *CVPreserveTState(CharView *cv);
extern Undoes *CVPreserveWidth(CharView *cv,int width);
extern Undoes *CVPreserveVWidth(CharView *cv,int vwidth);
extern void CVDoRedo(CharView *cv);
extern void CVDoUndo(CharView *cv);
extern void SCDoRedo(SplineChar *sc,int layer);
extern void SCDoUndo(SplineChar *sc,int layer);
extern void CVRestoreTOriginalState(CharView *cv);
extern void CVUndoCleanup(CharView *cv);
extern void CVRemoveTopUndo(CharView *cv);
extern enum undotype CopyUndoType(void);
extern int CopyContainsSomething(void);
extern int CopyContainsBitmap(void);
extern RefChar *CopyContainsRef(SplineFont *);
extern char **CopyGetPosSubData(enum possub_type *type);
extern void CopyReference(SplineChar *sc);
extern void CopySelected(CharView *cv);
extern void CVCopyGridFit(CharView *cv);
extern void SCCopyWidth(SplineChar *sc,enum undotype);
extern void CopyWidth(CharView *cv,enum undotype);
extern void PasteToCV(CharView *cv);
extern void PasteRemoveSFAnchors(SplineFont *);
extern void PasteAnchorClassMerge(SplineFont *sf,AnchorClass *into,AnchorClass *from);
extern void PasteRemoveAnchorClass(SplineFont *sf,AnchorClass *dying);
extern void PosSubCopy(enum possub_type type, char **data);
extern void ClipboardClear(void);
extern SplineSet *ClipBoardToSplineSet(void);
extern void BCCopySelected(BDFChar *bc,int pixelsize,int depth);
extern void PasteToBC(BDFChar *bc,int pixelsize,int depth,FontView *fv);
extern void FVCopyWidth(FontView *fv,enum undotype ut);
extern void FVCopy(FontView *fv, int fullcopy);
extern void MVCopyChar(MetricsView *mv, SplineChar *sc, int fullcopy);
extern void PasteIntoFV(FontView *fv, int doclear);
void PasteIntoMV(MetricsView *mv,SplineChar *sc, int doclear);

extern void CVShowPoint(CharView *cv, SplinePoint *sp);

extern void WindowMenuBuild(GWindow base,struct gmenuitem *mi,GEvent *);
extern void MenuRecentBuild(GWindow base,struct gmenuitem *mi,GEvent *);
extern void MenuScriptsBuild(GWindow base,struct gmenuitem *mi,GEvent *);
extern int RecentFilesAny(void);
extern void _aplistbuild(struct gmenuitem *mi,SplineFont *sf,
	void (*func)(GWindow,struct gmenuitem *,GEvent *));

extern GImage GIcon_sel2ptr, GIcon_rightpointer, GIcon_angle, GIcon_distance,
	GIcon_selectedpoint, GIcon_mag;

extern BitmapView *BitmapViewCreate(BDFChar *bc, BDFFont *bdf, FontView *fv);
extern BitmapView *BitmapViewCreatePick(int enc, FontView *fv);
extern void BitmapViewFree(BitmapView *bv);
extern void BCCharChangedUpdate(BDFChar *bc);
extern void BCFlattenFloat(BDFChar *bc);
extern BDFFloat *BDFFloatCreate(BDFChar *bc,int xmin,int xmax,int ymin,int ymax, int clear);
extern BDFFloat *BDFFloatCopy(BDFFloat *sel);
extern BDFFloat *BDFFloatConvert(BDFFloat *sel,int newdepth, int olddepth);
extern void BDFFloatFree(BDFFloat *sel);
extern void BVMenuRotateInvoked(GWindow gw,struct gmenuitem *mi, GEvent *e);
extern void BCTrans(BDFFont *bdf,BDFChar *bc,BVTFunc *bvts,FontView *fv );
extern void BVRotateBitmap(BitmapView *bv,enum bvtools type );
extern int  BVColor(BitmapView *bv);
extern void BCSetPoint(BDFChar *bc, int x, int y, int color);
extern void BCGeneralFunction(BitmapView *bv,
	void (*SetPoint)(BitmapView *,int x, int y, void *data),void *data);
extern int BVFlipNames[];
extern void BVChangeBC(BitmapView *bv, BDFChar *bc, int fitit );
extern void BVChar(BitmapView *cv, GEvent *event );

extern void MVSetSCs(MetricsView *mv, SplineChar **scs);
extern void MVRefreshChar(MetricsView *mv, SplineChar *sc);
extern void MVRegenChar(MetricsView *mv, SplineChar *sc);
extern void MVReKern(MetricsView *mv);
extern MetricsView *MetricsViewCreate(FontView *fv,SplineChar *sc,BDFFont *bdf);
extern void MetricsViewFree(MetricsView *mv);
extern void MVRefreshAll(MetricsView *mv);

extern char *getPfaEditShareDir(void);
extern void LoadPrefs(void);
extern void SavePrefs(void);
extern void DoPrefs(void);
extern void LoadPfaEditEncodings(void);
extern void PfaEditSetFallback(void);
extern void RecentFilesRemember(char *filename);
extern int GetPrefs(char *name,Val *val);		/* for scripting */
extern int SetPrefs(char *name,Val *val1, Val *val2);	/* for scripting */
extern void GListAddStr(GGadget *list,unichar_t *str, void *ud);
extern void GListReplaceStr(GGadget *list,int index, unichar_t *str, void *ud);
extern void GCDFillMacFeat(GGadgetCreateData *mfgcd,GTextInfo *mflabels, int width,
	MacFeat *all, int fromprefs);
extern int UserFeaturesDiffer(void);
extern void Prefs_ReplaceMacFeatures(GGadget *list);

extern void FVAutoTrace(FontView *fv,int ask);
extern void SCAutoTrace(SplineChar *sc,GWindow v,int ask);
extern char *FindAutoTraceName(void);
extern char *FindMFName(void);
extern char *ProgramExists(char *prog,char *buffer);
extern void *GetAutoTraceArgs(void);
extern void SetAutoTraceArgs(void *a);
extern void MfArgsInit(void);

extern unichar_t *FVOpenFont(const unichar_t *title, const unichar_t *defaultfile,
	const unichar_t *initial_filter, unichar_t **mimetypes,int mult,int newok);

extern unichar_t *PrtBuildDef( SplineFont *sf, int istwobyte );
extern void PrintDlg(FontView *fv,SplineChar *sc,MetricsView *mv);
extern void ScriptPrint(FontView *fv,int type,int32 *pointsizes,char *samplefile,
	char *outputfile);

enum sftf_fonttype { sftf_pfb, sftf_ttf, sftf_httf, sftf_otf, sftf_bitmap, sftf_pfaedit };
extern int SFTFSetFont(GGadget *g, int start, int end, SplineFont *sf,
	enum sftf_fonttype, int size, int antialias);
extern void SFTFRegisterCallback(GGadget *g, void *cbcontext,
	void (*changefontcallback)(void *,SplineFont *,enum sftf_fonttype,int size,int aa));
extern GGadget *SFTextAreaCreate(struct gwindow *base, GGadgetData *gd,void *data);
extern void DisplayDlg(SplineFont *sf);

extern void ShowAboutScreen(void);
extern void DelayEvent(void (*func)(void *), void *data);

extern void FindProblems(FontView *fv,CharView *cv,SplineChar *sc);
extern void MetaFont(FontView *fv,CharView *cv,SplineChar *sc);
extern void CVConstrainSelection(CharView *cv,int type);
extern void CVMakeParallel(CharView *cv);

extern void ScriptDlg(FontView *fv);
extern void ExecuteScriptFile(FontView *fv, char *filename);
extern void DictionaryFree(struct dictionary *dica);

# if HANYANG
extern void MenuNewComposition(GWindow gw, struct gmenuitem *, GEvent *);
extern void CVDisplayCompositions(GWindow gw, struct gmenuitem *, GEvent *);
extern void Disp_DoFinish(struct jamodisplay *d, int cancel);
extern void Disp_RefreshChar(SplineFont *sf,SplineChar *sc);
extern void Disp_DefaultTemplate(CharView *cv);
# endif

extern SearchView *SVCreate(FontView *fv);
extern void SVCharViewInits(SearchView *sv);
extern void SVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e);
extern void SVChar(SearchView *sv, GEvent *event);
extern void SVMakeActive(SearchView *sv,CharView *cv);
extern int SVAttachFV(FontView *fv,int ask_if_difficult);
extern void SVDetachFV(FontView *fv);

extern void ShowAtt(SplineFont *sf);
extern void SFShowKernPairs(SplineFont *sf,SplineChar *sc,AnchorClass *ac);
extern void SFShowLigatures(SplineFont *sf,SplineChar *sc);

extern void SCEditInstructions(SplineChar *sc);
extern void SFEditTable(SplineFont *sf, uint32 tag);
extern void IIScrollTo(struct instrinfo *ii,int ip,int mark_stop);
extern void IIReinit(struct instrinfo *ii,int ip);
extern int ii_v_e_h(GWindow gw, GEvent *event);
extern void instr_scroll(struct instrinfo *ii,struct sbevent *sb);

extern void CVGridFitChar(CharView *cv);
extern void CVFtPpemDlg(CharView *cv,int debug);
extern void SCDeGridFit(SplineChar *sc);

extern void CVDebugReInit(CharView *cv,int restart_debug,int dbg_fpgm);
extern void CVDebugFree(DebugView *dv);
extern int DVChar(DebugView *dv, GEvent *e);

struct debugger_context;
extern void DebuggerTerminate(struct debugger_context *dc);
extern void DebuggerReset(struct debugger_context *dc,real pointsize,int dpi,int dbg_fpgm);
extern struct debugger_context *DebuggerCreate(SplineChar *sc,real pointsize,int dpi,int dbg_fpgm);
enum debug_gotype { dgt_continue, dgt_step, dgt_next, dgt_stepout };
extern void DebuggerGo(struct debugger_context *dc,enum debug_gotype);
extern struct  TT_ExecContextRec_ *DebuggerGetEContext(struct debugger_context *dc);
extern void DebuggerToggleBp(struct debugger_context *dc,int range,int ip);
extern int DebuggerBpCheck(struct debugger_context *dc,int range,int ip);
extern void DebuggerSetWatches(struct debugger_context *dc,int n, uint8 *w);
extern uint8 *DebuggerGetWatches(struct debugger_context *dc, int *n);
extern int DebuggingFpgm(struct debugger_context *dc);

extern unichar_t *ScriptLangLine(struct script_record *sr);
extern void ShowKernClasses(SplineFont *sf,MetricsView *mv,int isv);
extern void KCLD_End(struct kernclasslistdlg *kcld);
extern void KCLD_MvDetach(struct kernclasslistdlg *kcld,MetricsView *mv);

extern void FVSelectByPST(FontView *fv);
extern int FVParseSelectByPST(FontView *fv,int type,
	const unichar_t *tags,const unichar_t *contents,
	int search_type);

enum hist_type { hist_hstem, hist_vstem, hist_blues };
struct psdict;
extern void SFHistogram(SplineFont *sf,struct psdict *private,uint8 *selected,
	enum hist_type which);

extern GTextInfo **SFGenTagListFromType(struct gentagtype *gentags,enum possub_type type);
extern struct contextchaindlg *ContextChainEdit(SplineFont *sf,FPST *fpst,
	struct gfi_data *gfi,unichar_t *newname);
extern void CCD_Close(struct contextchaindlg *ccd);
extern int CCD_NameListCheck(SplineFont *sf,const unichar_t *ret,int empty_bad,int title);
extern int CCD_InvalidClassList(const unichar_t *ret,GGadget *list,int wasedit);
extern char *cu_copybetween(const unichar_t *start, const unichar_t *end);

extern struct statemachinedlg *StateMachineEdit(SplineFont *sf,ASM *sm,struct gfi_data *d);
extern void SMD_Close(struct statemachinedlg *smd);
extern void GFI_FinishSMNew(struct gfi_data *d,ASM *sm, int success,int isnew);
extern void GFI_SMDEnd(struct gfi_data *d);
extern ASM *SMConvertDlg(SplineFont *sf);

extern void SFRemoveFeatureDlg(SplineFont *sf);
extern void SFCopyFeatureToFontDlg(SplineFont *sf);
extern void SFRetagFeatureDlg(SplineFont *sf);

extern void MMChangeBlend(MMSet *mm,FontView *fv,int tonew);
extern void MMWizard(MMSet *mm);

extern int LayerDialog(Layer *layer);
extern void SCMoreLayers(SplineChar *);
extern void SCLayersChange(SplineChar *sc);
extern void CVLayerChange(CharView *cv);
extern void SFLayerChange(SplineFont *sf);

extern GMenuItem helplist[];
#endif
