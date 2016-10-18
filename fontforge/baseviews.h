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
#ifndef _BASEVIEWS_H
#define _BASEVIEWS_H

#include "ffglib.h"
#include "splinefont.h"

#define free_with_debug(x) { fprintf(stderr,"%p FREE()\n",x); free(x); }


enum widthtype { wt_width, wt_lbearing, wt_rbearing, wt_bearings, wt_vwidth };

enum fvtrans_flags { fvt_alllayers=1, fvt_round_to_int=2,
	fvt_dontsetwidth=4, fvt_dontmovewidth=8, fvt_scalekernclasses=0x10,
	fvt_scalepstpos=0x20, fvt_dogrid=0x40, fvt_partialreftrans=0x80,
	fvt_justapply=0x100, fvt_revert=0x200 };

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
    unsigned int splineAdjacentPointsSelected: 1; /* were the points on both ends of the current clicked
						   * spline both selected during the mouse down operation
						   **/
    Spline *spline;
    real t;			/* location on the spline where we pressed */
    RefChar *ref;
    SplinePointList *spl;	/* containing spline or point */
    ImageList *img;
    AnchorPoint *ap;
    float ex, ey;		/* end of last rubber band rectangle */
    BasePoint constrain;	/* Point to which we constrain movement */
    BasePoint cp;		/* Original control point position */
    spiro_cp *spiro;		/* If they clicked on a spiro point */
    int spiro_index;		/* index of a clicked spiro_cp, or */
			/* if they clicked on the spline between spiros, */
			/* this is the spiro indexof the preceding spiro */
    GList_Glib*      pretransform_spl; /* If we want to draw an image of the original spl while doing something
					* this is a copy of that original spl */
} PressedOn;

/* Note: These are ordered as they are displayed in the tools palette */
enum cvtools {
	cvt_pointer, cvt_magnify,
	cvt_freehand, cvt_hand,
	cvt_knife, cvt_ruler,
	cvt_pen, cvt_spiro,
	cvt_curve, cvt_hvcurve,
	cvt_corner, cvt_tangent,
	cvt_scale, cvt_rotate,
	cvt_flip, cvt_skew,
	cvt_3d_rotate, cvt_perspective,
	cvt_rect, cvt_poly,
	cvt_elipse, cvt_star,
	cvt_minify, cvt_max=cvt_minify,
	cvt_none = -1,
	cvt_spirog4=cvt_curve, cvt_spirog2=cvt_hvcurve,
	cvt_spirocorner=cvt_corner, cvt_spiroleft=cvt_tangent,
	cvt_spiroright=cvt_pen};

enum bvtools { bvt_pointer, bvt_magnify,
	bvt_pencil, bvt_line,
	bvt_shift, bvt_hand,
	bvt_minify, bvt_max=bvt_minify, bvt_eyedropper,
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

struct cvcontainer {
    struct cvcontainer_funcs *funcs;
};

enum nav_type { nt_prevdef, nt_prev, nt_goto, nt_next, nt_nextdef };

enum cv_container_type { cvc_searcher, cvc_mathkern, cvc_tilepath,
			 cvc_gradient, cvc_multiplepattern, cvc_stroke };

struct cvcontainer_funcs {
    enum cv_container_type type;
    void (*activateMe)(struct cvcontainer *cvc,struct charviewbase *cv);
    void (*charEvent)(struct cvcontainer *cvc,void *event);
    int (*canNavigate)(struct cvcontainer *cvc,enum nav_type type);
    void (*doNavigate)(struct cvcontainer *cvc,enum nav_type type);
    int (*canOpen)(struct cvcontainer *cvc);
    void (*doClose)(struct cvcontainer *cvc);
    SplineFont *(*sf_of_container)(struct cvcontainer *cvc);
};

typedef struct charviewbase {
    struct charviewbase *next;
    struct fontviewbase *fv;
    SplineChar *sc;
    Layer *layerheads[dm_max];
    uint8 drawmode;
    uint16 ft_gridfitwidth;
    SplineSet *gridfit;
    struct cvcontainer *container;		/* The sv (or whatever) within which this view is embedded (if it is embedded) */
} CharViewBase;

struct fvcontainer {
    struct fvcontainer_funcs *funcs;
};

enum fv_container_type { fvc_kernformat, fvc_glyphset };

struct fvcontainer_funcs {
    enum fv_container_type type;
    int is_modal;		/* If the fvc is in a modal dialog then we can't create modeless windows (like charviews, fontinfo, etc.) */
    void (*activateMe)(struct fvcontainer *fvc,struct fontviewbase *fv);
    void (*charEvent)(struct fvcontainer *fvc,void *event);
    void (*doClose)(struct fvcontainer *fvc);		/* Cancel the containing dlg? */
    void (*doResize)(struct fvcontainer *fvc,struct fontviewbase *fv,int width,int height);
				/* Resize the container so that fv fits */
};

enum collabState_t {
    cs_neverConnected, //< No connection or Collab feature used this run
    cs_disconnected,   //< Was connected at some stage, not anymore
    cs_server,         //< The localhost is running a server process
    cs_client          //< Connected to somebody else's server process
};       

typedef struct fontviewbase {
    struct fontviewbase *next;		/* Next on list of open fontviews */
    struct fontviewbase *nextsame;	/* Next fv looking at this font */
    EncMap *map;			/* Current encoding info */
    EncMap *normal;			/* If this is not NULL then we have a compacted encoding in map, and this is the original */
    SplineFont *sf;			/* Current font */
    SplineFont *cidmaster;		/* If CID keyed, contains master font */
    int active_layer;
    BDFFont *active_bitmap;		/* Set if the fontview displays a bitmap strike */
    uint8 *selected;			/* Current selection */
#ifndef _NO_FFSCRIPT
    struct dictionary *fontvars;	/* Scripting */
#endif
#ifndef _NO_PYTHON
    void *python_fv_object;
#endif
    struct fvcontainer *container;
    void* collabClient;                 /* The data used to talk to the collab server process */
    enum collabState_t collabState;     /* Since we want to know if we are connected, or used to be
					 * we have to keep the state variable out of collabClient
					 * itself */
    
} FontViewBase;

enum origins { or_zero, or_center, or_lastpress, or_value, or_undefined };
struct pov_data {
    enum origins xorigin, yorigin;
    double x, y, z;
    double direction;		/* Direction of gaze projected into xy plane */
    double tilt;		/* Angle which drawing plane is tilted with respect to projection plane */
    double d;			/* Distance to projection plane */
    double sintilt;		/* Used internally */
};

enum counter_type { ct_squish, ct_retain, ct_auto };

struct lcg_zones {
    /* info for unhinted processing */
     /* everything abvoe this should be moved down (default xheight/2) */
    int top_zone;
     /* everything below this should be moved up (default xheight/2) */
     /* anything in between should be stationary */
    int bottom_zone;

    /* info for hinted processing */
     /* everything above & at this should be moved down */
     /* also anything on the other side of a hint from this should be moved down */
    int top_bound;
     /* everything below & at this should be moved down */
     /* also anything on the other side of a hint from this should be moved down */
    int bottom_bound;

    enum counter_type counter_type;

    SplineSet *(*embolden_hook)(SplineSet *,struct lcg_zones *,SplineChar *,int layer);
    int wants_hints;
    double serif_height, serif_fuzz;

    double stroke_width;	/* negative number to lighten, positive to embolden */
    int removeoverlap;

    BlueData bd;
    double stdvw;
};
/* This order is the same order as the radio buttons in the embolden dlg */
enum embolden_type { embolden_lcg, embolden_cjk, embolden_auto, embolden_custom, embolden_error };

struct ci_zones {
    double start, width;
    double moveto, newwidth;		/* Only change width for diagonal stems*/
};

struct counterinfo {
    double c_factor, c_add;		/* For counters */
    double sb_factor, sb_add;		/* For side bearings */
    int correct_italic;

    BlueData bd;
    double stdvw;

    SplineChar *sc;
    int layer;
    DBounds bb;				/* Value before change */
    double top_y, bottom_y, boundry;
    int has_two_zones;
#define TOP_Z	0
#define BOT_Z	1
    int cnts[2];
    int maxes[2];
    struct ci_zones *zones[2];
};

enum fvformats { fv_bdf, fv_ttf, fv_pk, fv_pcf, fv_mac, fv_win, fv_palm,
	fv_image, fv_imgtemplate,
	fv_eps, fv_epstemplate,
	fv_pdf, fv_pdftemplate,
	fv_plate, fv_platetemplate,
	fv_svg, fv_svgtemplate,
	fv_glif, fv_gliftemplate,
	fv_fig,
	fv_pythonbase = 0x100 };

extern RefChar *CopyContainsRef(SplineFont *);
extern char **CopyGetPosSubData(enum possub_type *type,SplineFont **copied_from,
	int pst_depth);
extern void CopyReference(SplineChar *sc);
extern void PasteRemoveSFAnchors(SplineFont *);

enum fvcopy_type { ct_fullcopy, ct_reference, ct_lookups, ct_unlinkrefs };
extern void FVCopyFgtoBg(FontViewBase *fv);
extern void FVSameGlyphAs(FontViewBase *fv);
extern void FVClearBackground(FontViewBase *fv);
extern void FVClear(FontViewBase *fv);
extern void FVUnlinkRef(FontViewBase *fv);
extern void FVUndo(FontViewBase *fv);
extern void FVRedo(FontViewBase *fv);
extern void FVJoin(FontViewBase *fv);
extern void FVBuildDuplicate(FontViewBase *fv);
extern void FVTrans(FontViewBase *fv,SplineChar *sc,real transform[6],uint8 *sel,
	enum fvtrans_flags);
extern void FVTransFunc(void *_fv,real transform[6],int otype, BVTFunc *bvts,
	enum fvtrans_flags );
extern void FVReencode(FontViewBase *fv,Encoding *enc);
extern void FVOverlap(FontViewBase *fv,enum overlap_type ot);
extern void FVAddExtrema(FontViewBase *fv, int force_adding);
extern void FVCorrectDir(FontViewBase *fv);
extern void FVRound2Int(FontViewBase *fv,real factor);
extern void FVCanonicalStart(FontViewBase *fv);
extern void FVCanonicalContours(FontViewBase *fv);
extern void FVCluster(FontViewBase *fv);
extern void CIDSetEncMap(FontViewBase *fv, SplineFont *new );
extern void FVInsertInCID(FontViewBase *fv,SplineFont *sf);

extern void FVAutoHint(FontViewBase *fv);
extern void FVAutoHintSubs(FontViewBase *fv);
extern void FVAutoCounter(FontViewBase *fv);
extern void FVDontAutoHint(FontViewBase *fv);
extern void FVAutoInstr(FontViewBase *fv);
extern void FVClearInstrs(FontViewBase *fv);
extern void FVClearHints(FontViewBase *fv);
extern void SCAutoTrace(SplineChar *sc,int layer, int ask);
extern int FVImportMult(FontViewBase *fv, char *filename,int toback,int bf);
extern int FVImportBDF(FontViewBase *fv, char *filename,int ispk, int toback);
extern void MergeFont(FontViewBase *fv,SplineFont *other,int preserveCrossFontKerning);
extern void ScriptPrint(FontViewBase *fv,int type,int32 *pointsizes,char *samplefile,
	unichar_t *sample, char *outputfile);
extern int FVBParseSelectByPST(FontViewBase *fv,struct lookup_subtable *sub,
	int search_type);
extern int SFScaleToEm(SplineFont *sf, int ascent, int descent);
extern void TransHints(StemInfo *stem,real mul1, real off1, real mul2, real off2, int round_to_int );
extern void TransDStemHints(DStemInfo *ds,real xmul, real xoff, real ymul, real yoff, int round_to_int );
extern void VrTrans(struct vr *vr,real transform[6]);
extern int SFNLTrans(FontViewBase *fv,char *x_expr,char *y_expr);
extern int SSNLTrans(SplineSet *ss,char *x_expr,char *y_expr);
extern int SCNLTrans(SplineChar *sc, int layer,char *x_expr,char *y_expr);
extern void FVPointOfView(FontViewBase *fv,struct pov_data *);
extern void FVStrokeItScript(void *fv, StrokeInfo *si,int pointless);
extern void CI_Init(struct counterinfo *ci,SplineFont *sf);
extern void FVEmbolden(struct fontviewbase *fv,enum embolden_type type,struct lcg_zones *zones);
extern void FVCondenseExtend(struct fontviewbase *fv,struct counterinfo *ci);
extern void ScriptSCCondenseExtend(SplineChar *sc,struct counterinfo *ci);

struct smallcaps {
    double lc_stem_width, uc_stem_width;
    double stem_factor, v_stem_factor;
    double xheight, scheight, capheight;
    double vscale, hscale;
    char *extension_for_letters, *extension_for_symbols;
    int dosymbols;
    SplineFont *sf;
    int layer;
    double italic_angle, tan_ia;
};

extern void SmallCapsFindConstants(struct smallcaps *small, SplineFont *sf,
	int layer );

enum glyphchange_type { gc_generic, gc_smallcaps, gc_subsuper, gc_max };

struct position_maps {
    double current  , desired;
    double cur_width, des_width;
    int overlap_index;
};

struct fixed_maps {
    int cnt;
    struct position_maps *maps;
};

struct genericchange {
    enum glyphchange_type gc;
    uint32 feature_tag;
    char *glyph_extension;
    char *extension_for_letters, *extension_for_symbols;
    double stem_height_scale, stem_width_scale;
    double stem_height_add  , stem_width_add  ;
    double stem_threshold;
    double serif_height_scale, serif_width_scale;
    double serif_height_add  , serif_width_add  ;
    double hcounter_scale, hcounter_add;
    double lsb_scale, lsb_add;
    double rsb_scale, rsb_add;
    uint8 center_in_hor_advance;
    uint8 use_vert_mapping;
    uint8 do_smallcap_symbols;
    uint8 petite;				/* generate petite caps rather than smallcaps */
    double vcounter_scale, vcounter_add;	/* If not using mapping */
    double v_scale;				/* If using mapping */
    struct fixed_maps m;
    struct fixed_maps g;			/* Adjusted for each glyph */
    double vertical_offset;
    unsigned int dstem_control, serif_control;
    struct smallcaps *small;
/* Filled in by called routine */
    SplineFont *sf;
    int layer;
    double italic_angle, tan_ia;
};

extern void FVAddSmallCaps(FontViewBase *fv,struct genericchange *genchange);
extern void FVGenericChange(FontViewBase *fv,struct genericchange *genchange);
extern void CVGenericChange(CharViewBase *cv,struct genericchange *genchange);

struct xheightinfo {
    double xheight_current, xheight_desired;
    double serif_height;
};

extern void InitXHeightInfo(SplineFont *sf, int layer, struct xheightinfo *xi);
extern void ChangeXHeight(FontViewBase *fv,CharViewBase *cv, struct xheightinfo *xi);
extern SplineSet *SSControlStems(SplineSet *ss,
	double stemwidthscale, double stemheightscale,
	double hscale, double vscale, double xheight);
extern void MakeItalic(FontViewBase *fv,CharViewBase *cv,ItalicInfo *ii);
extern int FVReplaceAll( FontViewBase *fv, SplineSet *find, SplineSet *rpl, double fudge, int flags );
extern void FVBReplaceOutlineWithReference( FontViewBase *fv, double fudge );
extern void FVCorrectReferences(FontViewBase *fv);
extern void _FVSimplify(FontViewBase *fv,struct simplifyinfo *smpl);
extern void UnlinkThisReference(FontViewBase *fv,SplineChar *sc,int layer);
extern FontViewBase *ViewPostScriptFont(const char *filename,int openflags);
extern void FVBuildAccent(FontViewBase *fv,int onlyaccents);
extern void FVAddUnencoded(FontViewBase *fv, int cnt);
extern void FVRemoveUnused(FontViewBase *fv);
extern void FVCompact(FontViewBase *fv);
extern void FVDetachGlyphs(FontViewBase *fv);
extern void FVDetachAndRemoveGlyphs(FontViewBase *fv);


extern Undoes *_CVPreserveTState(CharViewBase *cv,PressedOn *);
extern void CopySelected(CharViewBase *cv,int doanchors);
extern void CopyWidth(CharViewBase *cv,enum undotype);
extern void CVYPerspective(CharViewBase *cv,bigreal x_vanish, bigreal y_vanish);
extern void ScriptSCEmbolden(SplineChar *sc,int layer,enum embolden_type type,struct lcg_zones *zones);
extern void CVEmbolden(CharViewBase *cv,enum embolden_type type,struct lcg_zones *zones);
extern void SCCondenseExtend(struct counterinfo *ci,SplineChar *sc, int layer,
	int do_undoes);
extern void SCClearSelPt(SplineChar *sc);
extern void SC_MoreLayers(SplineChar *,Layer *old);
extern void SCLayersChange(SplineChar *sc);
extern void SFLayerChange(SplineFont *sf);
extern void SCTile(SplineChar *sc,int layer);
extern void _CVMenuMakeLine(CharViewBase *cv,int do_arc,int ellipse_to_back);
    /* Ellipse to back is a debugging flag and adds the generated ellipse to */
    /*  the background layer so we can look at it. I thought it might actually*/
    /*  be useful, so I left it in. Activated with the Alt key in the menu */

extern void MVCopyChar(FontViewBase *fv, BDFFont *bdf, SplineChar *sc, enum fvcopy_type fullcopy);
extern void PasteIntoMV(FontViewBase *fv, BDFFont *bdf,SplineChar *sc, int doclear);

extern void ExecuteScriptFile(FontViewBase *fv, SplineChar *sc, char *filename);

enum search_flags { sv_reverse = 0x1, sv_flips = 0x2, sv_rotate = 0x4,
	sv_scale = 0x8, sv_endpoints=0x10 };

enum flipset { flip_none = 0, flip_x, flip_y, flip_xy };

typedef struct searchdata {
    SplineChar sc_srch, sc_rpl;
    SplineSet *path, *revpath, *replacepath, *revreplace;
    int pointcnt, rpointcnt;
    real fudge;
    real fudge_percent;			/* a value of .05 here represents 5% (we don't store the integer) */
    unsigned int tryreverse: 1;
    unsigned int tryflips: 1;
    unsigned int tryrotate: 1;
    unsigned int tryscale: 1;
    unsigned int endpoints: 1;		/* Don't match endpoints, use them for direction only */
    unsigned int onlyselected: 1;
    unsigned int subpatternsearch: 1;
    unsigned int doreplace: 1;
    unsigned int replaceall: 1;
    unsigned int findall: 1;
    unsigned int searchback: 1;
    unsigned int wrap: 1;
    unsigned int wasreversed: 1;
    unsigned int replacewithref: 1;
    unsigned int already_complained: 1;	/* User has already been alerted to the fact that we've converted splines to refs and lost the instructions */
    SplineSet *matched_spl;
    SplinePoint *matched_sp, *last_sp;
    real matched_rot, matched_scale;
    real matched_x, matched_y;
    double matched_co, matched_si;		/* Precomputed sin, cos */
    enum flipset matched_flip;
    unsigned long long matched_refs;	/* Bit map of which refs in the char were matched */
    unsigned long long matched_ss;	/* Bit map of which splines in the char were matched */
				    /* In multi-path mode */
    unsigned long long matched_ss_start;/* Bit map of which splines we tried to start matches with */
    FontViewBase *fv;
    SplineChar *curchar;
    int last_gid;
} SearchData;

extern struct searchdata *SDFromContour( FontViewBase *fv, SplineSet *find, double fudge, int flags );
extern SplineChar *SDFindNext(struct searchdata *sv);

extern struct python_import_export {
    struct _object *import;	/* None becomes NULL */
    struct _object *export;	/* None becomes NULL */
    struct _object *data;	/* None stays None */
    char *name;
    char *extension;
    char *all_extensions;
} *py_ie;
extern void PyFF_SCExport(SplineChar *sc,int ie_index,char *filename,
	int layer);
extern void PyFF_SCImport(SplineChar *sc,int ie_index,char *filename,
	int layer, int clear);
extern void PyFF_InitFontHook(FontViewBase *fv);

extern void LookupInit(void);
extern int UserFeaturesDiffer(void);
extern uint32 *StdFeaturesOfScript(uint32 script);

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

extern uint8 *_IVParse(SplineFont *sf, char *text, int *len,
	void (*IVError)(void *,char *, int), void *iv);
extern char *_IVUnParseInstrs(uint8 *instrs,int instr_cnt);

extern void FVSetWidthScript(FontViewBase *fv,enum widthtype wtype,int val,int incr);
extern void FVMetricsCenter(FontViewBase *fv,int docenter);
extern void FVRevert(FontViewBase *fv);
extern void FVRevertBackup(FontViewBase *fv);
extern void FVRevertGlyph(FontViewBase *fv);
extern void FVClearSpecialData(FontViewBase *fv);
extern int   MMReblend(FontViewBase *fv, MMSet *mm);
extern FontViewBase *MMCreateBlendedFont(MMSet *mm,FontViewBase *fv,real blends[MmMax],int tonew );
extern void FVB_MakeNamelist(FontViewBase *fv, FILE *file);

/**
 * Code which wants the fontview to redraw it's title can call here to
 * have that happen.
 */
extern void FVTitleUpdate(FontViewBase *fv);

extern void AutoKern2(SplineFont *sf, int layer,SplineChar **left,SplineChar **right,
	struct lookup_subtable *into,
	int separation, int min_kern, int from_closest_approach, int only_closer,
	int chunk_height,
	void (*addkp)(void *data,SplineChar *left,SplineChar *r,int off),
	void *data);

extern void MVSelectFirstKerningTable(struct metricsview *mv);

extern float joinsnap;

#endif
