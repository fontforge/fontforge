/* Copyright (C) 2000-2003 by George Williams */
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
#ifndef _SPLINEFONT_H
#define _SPLINEFONT_H

#include "basics.h"
#include "charset.h"

#ifdef USE_DOUBLE
# define real	double
#else
# define real	float
#endif

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

typedef struct val {
    enum val_type { v_int, v_str, v_unicode, v_lval, v_arr, v_arrfree, v_void } type;
    union {
	int ival;
	char *sval;
	struct val *lval;
	struct array *aval;
    } u;
} Val;		/* Used by scripting */

struct psdict {
    int cnt, next;
    char **keys;
    char **values;
};

struct pschars {
    int cnt, next;
    char **keys;
    uint8 **values;
    int *lens;
    int bias;		/* for type2 strings */
};

enum linejoin {
    lj_miter,		/* Extend lines until they meet */
    lj_round,		/* circle centered at the join of expand radius */
    lj_bevel		/* Straight line between the ends of next and prev */
};
enum linecap {
    lc_butt,		/* equiv to lj_bevel, straight line extends from one side to other */
    lc_round,		/* semi-circle */
    lc_square		/* Extend lines by radius, then join them */
};

struct spline;
typedef struct strokeinfo {
    real radius;			/* or major axis of pen */
    enum linejoin join;
    enum linecap cap;
    unsigned int caligraphic: 1;
    unsigned int centerline: 1;		/* For freehand tool */
    unsigned int toobigwarn: 1;
    real penangle;
    real ratio;				/* ratio of minor pen axis to major */
/* For freehand tool */
    real radius2;
    int pressure1, pressure2;
/* End freehand tool */
    double c,s;
    real xoff[8], yoff[8];
    void *data;
    double (*factor)(void *data,struct spline *spline,real t);
} StrokeInfo;

typedef struct ipoint {
    int x;
    int y;
} IPoint;

typedef struct basepoint {
    real x;
    real y;
} BasePoint;

typedef struct tpoint {
    real x;
    real y;
    real t;
} TPoint;

typedef struct dbounds {
    real minx, maxx;
    real miny, maxy;
} DBounds;

typedef struct bluedata {
    real xheight, xheighttop;		/* height of "x" and "o" (u,v,w,x,y,z) */
    real caph, caphtop;			/* height of "I" and "O" */
    real base, basebelow;		/* bottom of "I" and "O" */
    real ascent;			/* height of "l" */
    real descent;			/* depth of "p" */
    real numh, numhtop;			/* height of "7" and "8" */ /* numbers with ascenders */
    int bluecnt;			/* If the private dica contains bluevalues... */
    real blues[12][2];			/* 7 pairs from bluevalues, 5 from otherblues */
} BlueData;

typedef struct bdffloat {
    int16 xmin,xmax,ymin,ymax;
    int16 bytes_per_line;
    unsigned int byte_data:1;
    uint8 depth;
    uint8 *bitmap;
} BDFFloat;

/* I'm not going to give the user access to r2l. They can set the script for that */
enum pst_flags { pst_r2l=1, pst_ignorebaseglyphs=2, pst_ignoreligatures=4, pst_ignorecombiningmarks=8 };
typedef struct anchorclass {
    unichar_t *name;
    uint32 feature_tag;
    uint16 flags;
    struct anchorclass *next;
} AnchorClass;

enum anchor_type { at_mark, at_basechar, at_baselig, at_basemark, at_centry, at_cexit, at_max };
typedef struct anchorpoint {
    AnchorClass *anchor;
    BasePoint me;
    unsigned int type: 4;
    unsigned int selected: 1;
    unsigned int ticked: 1;
    int lig_index;
    struct anchorpoint *next;
} AnchorPoint;

enum possub_type { pst_null, pst_position, pst_substitution, pst_alternate,
	pst_multiple, pst_ligature,
	pst_lcaret /* must be pst_max-1, see charinfo.c*/,
	pst_max};
typedef struct generic_pst {
    enum possub_type type;
    uint32 tag;
    struct generic_pst *next;
    uint16 flags;
    union {
	struct { int16 xoff, yoff, h_adv_off, v_adv_off; } pos;
	struct { char *variant; } subs;
	struct { char *components; } mult, alt;
	struct { char *components; struct splinechar *lig; } lig;
	struct { int16 *carets; int cnt; } lcaret;	/* Ligature caret positions */
    } u;
} PST;

typedef struct liglist {
    PST *lig;
    struct splinechar *first;		/* First component */
    struct splinecharlist *components;	/* Other than the first */
    struct liglist *next;
    int ccnt;				/* Component count. (includes first component) */
} LigList;

typedef struct undoes {
    struct undoes *next;
    enum undotype { ut_none=0, ut_state, ut_tstate, ut_statehint, ut_statename,
	    ut_width, ut_vwidth, ut_lbearing, ut_rbearing, ut_possub,
	    ut_bitmap, ut_bitmapsel, ut_composit, ut_multiple, ut_noop } undotype;
    unsigned int was_modified: 1;
    unsigned int was_order2: 1;
    union {
	struct {
	    int16 width, vwidth;
	    int16 lbearingchange;
	    int unicodeenc;			/* only for ut_statename */
	    uint32 script;			/* only for ut_statename */
	    char *charname;			/* only for ut_statename */
	    unichar_t *comment;			/* only for ut_statename */
	    PST *possub;			/* only for ut_statename */
	    struct splinepointlist *splines;
	    struct refchar *refs;
	    struct minimumdistance *md;
	    union {
		struct imagelist *images;
		void *hints;			/* ut_statehint, ut_statename */
	    } u;
	    AnchorPoint *anchor;
	    struct splinefont *copied_from;
	} state;
	int width;	/* used by both ut_width and ut_vwidth */
	int lbearing;	/* used by ut_lbearing */
	int rbearing;	/* used by ut_rbearing */
	struct {
	    /*int16 width;*/	/* width should be controled by postscript */
	    int16 xmin,xmax,ymin,ymax;
	    int16 bytes_per_line;
	    int16 pixelsize;
	    int16 depth;
	    uint8 *bitmap;
	    BDFFloat *selection;
	} bmpstate;
	struct {		/* copy contains an outline state and a set of bitmap states */
	    struct undoes *state;
	    struct undoes *bitmaps;
	} composit;
	struct {
	    struct undoes *mult; /* copy contains several sub copies (composits, or states or widths or...) */
	} multiple;
	struct {
	    enum possub_type pst;
	    char **data;		/* First 4 bytes is tag, then space then data */
	} possub;
	uint8 *bitmap;
    } u;
} Undoes;

typedef struct bdfchar {
    struct splinechar *sc;
    int16 xmin,xmax,ymin,ymax;
    int16 width;
    int16 bytes_per_line;
    uint8 *bitmap;
    int enc;
    struct bitmapview *views;
    Undoes *undoes;
    Undoes *redoes;
    unsigned int changed: 1;
    unsigned int byte_data: 1;		/* for anti-aliased chars entries are grey-scale bytes not bw bits */
    unsigned int widthgroup: 1;		/* for ttf bitmap output */
    uint8 depth;			/* for ttf bitmap output */
    BDFFloat *selection;
} BDFChar;

typedef struct bdffont {
    struct splinefont *sf;
    int charcnt;
    BDFChar **chars;		/* an array of charcnt entries */
    BDFChar **temp;		/* Used by ReencodeFont routine */
    int16 pixelsize;
    int16 ascent, descent;
    unsigned int piecemeal: 1;
    unsigned int bbsized: 1;
    enum charset encoding_name;
    struct bdffont *next;
    struct clut *clut;
    char *foundry;
    int res;
    void *freetype_context;
    int truesize;		/* for bbsized fonts */
} BDFFont;

enum pointtype { pt_curve, pt_corner, pt_tangent };
typedef struct splinepoint {
    BasePoint me;
    BasePoint nextcp;		/* control point */
    BasePoint prevcp;		/* control point */
    unsigned int nonextcp:1;
    unsigned int noprevcp:1;
    unsigned int nextcpdef:1;
    unsigned int prevcpdef:1;
    unsigned int selected:1;	/* for UI */
    unsigned int pointtype:2;
    unsigned int isintersection: 1;
    unsigned int flexy: 1;
    unsigned int flexx: 1;
    unsigned int roundx: 1;	/* For true type hinting */
    unsigned int roundy: 1;	/* For true type hinting */
    unsigned int dontinterpolate: 1;	/* temporary in ttf output */
	/* 16+3 bits left... */
    uint16 ptindex;		/* Temporary value used by metafont routine */
    uint16 ttfindex;		/* Truetype point index */
	/* Special values 0xffff => point implied by averaging control points */
	/*		  0xfffe => point created with no real number */
    struct spline *next;
    struct spline *prev;
} SplinePoint;

typedef struct linelist {
    IPoint here;
    struct linelist *next;
    /* The first two fields are constant for the linelist, the next ones */
    /*  refer to a particular screen. If some portion of the line from */
    /*  this point to the next one is on the screen then set cvli_onscreen */
    /*  if this point needs to be clipped then set cvli_clipped */
    /*  asend and asstart are the actual screen locations where this point */
    /*  intersects the clip edge. */
    enum { cvli_onscreen=0x1, cvli_clipped=0x2 } flags;
    IPoint asend, asstart;
} LineList;

typedef struct linearapprox {
    real scale;
    unsigned int oneline: 1;
    unsigned int onepoint: 1;
    unsigned int any: 1;		/* refers to a particular screen */
    struct linelist *lines;
    struct linearapprox *next;
} LinearApprox;

typedef struct spline1d {
    real a, b, c, d;
} Spline1D;

typedef struct spline {
    unsigned int islinear: 1;		/* No control points */
    unsigned int isquadratic: 1;	/* probably read in from ttf */
    unsigned int isticked: 1;
    unsigned int isneeded: 1;
    unsigned int isunneeded: 1;
    unsigned int ishorvert: 1;
    unsigned int knowncurved: 1;	/* We know that it curves */
    unsigned int knownlinear: 1;	/* it might have control points, but still traces out a line */
	/* If neither knownlinear nor curved then we haven't checked */
    unsigned int order2: 1;		/* It's a bezier curve with only one cp */
    unsigned int touched: 1;
    SplinePoint *from, *to;
    Spline1D splines[2];		/* splines[0] is the x spline, splines[1] is y */
    struct linearapprox *approx;
    /* Posible optimizations:
	Precalculate bounding box
	Precalculate points of inflection
    */
} Spline;

typedef struct splinepointlist {
    SplinePoint *first, *last;
    struct splinepointlist *next;
} SplinePointList, SplineSet;

typedef struct refchar {
    unsigned int checked: 1;
    unsigned int selected: 1;
    unsigned int point_match: 1;	/* transform[4:5] are point indexes */
					/*  and need to be converted to offsets*/
			                /*  after truetype readin */
    int16 adobe_enc;
    int local_enc;
    int unicode_enc;		/* used by paste */
    real transform[6];		/* transformation matrix (first 2 rows of a 3x3 matrix, missing row is 0,0,1) */
    SplinePointList *splines;
    struct refchar *next;
    DBounds bb;
    struct splinechar *sc;
    BasePoint top;
} RefChar;

typedef struct kernpair {
    struct splinechar *sc;
    int off;
    struct kernpair *next;
} KernPair;

/* Some stems may appear, disappear, reapear several times */
/* Serif stems on I which appear at 0, disappear, reappear at top */
/* Or the major vertical stems on H which disappear at the cross bar */
typedef struct hintinstance {
    real begin;			/* location in the non-major direction*/
    real end;				/* width/height in non-major direction*/
    unsigned int closed: 1;
    short int counternumber;
    struct hintinstance *next;
} HintInstance;

enum hinttypes { ht_unspecified=0, ht_h, ht_v, ht_d };
typedef struct steminfo {
    struct steminfo *next;
    unsigned int hinttype: 2;	/* Only used by undoes */
    unsigned int ghost: 1;	/* this is a ghost stem hint. As such truetype should ignore it, type2 output should negate it, and type1 should use as is */
		    /* stored width will be either 20 or 21 */
		    /* Type2 says: -20 is "width" of top edge, -21 is "width" of bottom edge, type1 accepts either */
    unsigned int haspointleft:1;
    unsigned int haspointright:1;
    unsigned int hasconflicts:1;/* Does this stem have conflicts within its cluster? */
    unsigned int used: 1;	/* Temporary for counter hints or hint substitution */
    unsigned int tobeused: 1;	/* Temporary for counter hints or hint substitution */
    unsigned int active: 1;	/* Currently active hint in Review Hints dlg */
				/*  displayed differently in char display */
    unsigned int enddone: 1;	/* Used by ttf instructing, indicates a prev */
				/*  hint had the same end as this one (so */
			        /*  the points on the end line have been */
			        /*  instructed already */
    unsigned int startdone: 1;	/* Used by ttf instructing */
    unsigned int backwards: 1;	/* If we think this hint is better done with a negative width */
    unsigned int reordered: 1;	/* In AutoHinting. Means we changed the start of the hint, need to test for out of order */
    unsigned int pendingpt: 1;	/* A pending stem creation, not a true stem */
    unsigned int linearedges: 1;/* If we have a nice rectangle then we aren't */
				/*  interested in the orientation which is */
			        /*  wider than long */
    unsigned int bigsteminfo: 1;/* See following structure */
    int16 hintnumber;		/* when dumping out hintmasks we need to know */
				/*  what bit to set for this hint */
    int mask;			/* Mask of all references that use this hint */
				/*  in type2 output */
    real start;			/* location at which the stem starts */
    real width;			/* or height */
    HintInstance *where;	/* location(s) in the other coord */
} StemInfo;

typedef struct pointlist { struct pointlist *next; SplinePoint *sp; } PointList;
typedef struct bigsteminfo {
    StemInfo s;
    PointList *left, *right;
} BigStemInfo;
    
typedef struct dsteminfo {
    struct dsteminfo *next;	/* First two fields match those in steminfo */
    unsigned int hinttype: 2;	/* Only used by undoes */
    unsigned int used: 1;	/* used only be tottf.c:gendinstrs to mark a hint that has been dealt with */
    unsigned int bigsteminfo: 1;/* See following structure */
    BasePoint leftedgetop, leftedgebottom, rightedgetop, rightedgebottom;	/* this order is important in tottf.c: DStemInteresect */
} DStemInfo;

typedef struct bigdsteminfo {
    DStemInfo s;
    PointList *left, *right;
} BigDStemInfo;

typedef struct imagelist {
    struct gimage *image;
    real xoff, yoff;		/* position in character space of upper left corner of image */
    real xscale, yscale;	/* scale to convert one pixel of image to one unit of character space */
    DBounds bb;
    struct imagelist *next;
    unsigned int selected: 1;
} ImageList;

typedef struct minimumdistance {
    /* If either point is NULL it will be assumed to mean either the origin */
    /*  or the width point (depending on which is closer). This allows user */
    /*  to control metrics... */
    SplinePoint *sp1, *sp2;
    unsigned int x: 1;
    unsigned int done: 1;
    struct minimumdistance *next;
} MinimumDistance;

typedef struct splinechar {
    char *name;
    int enc, unicodeenc, old_enc;
    int16 width, vwidth;
    int16 lsidebearing;		/* only used when reading in a type1 font */
				/*  Or an otf font where it is the subr number of a refered character */
			        /*  or a ttf font with vert metrics where it is the ymax value */
				/* Always a temporary value */
    int ttf_glyph;		/* only used when writing out a ttf or otf font */
    SplinePointList *splines;
    StemInfo *hstem;		/* hstem hints have a vertical offset but run horizontally */
    StemInfo *vstem;		/* vstem hints have a horizontal offset but run vertically */
    DStemInfo *dstem;		/* diagonal hints for ttf */
    MinimumDistance *md;
    RefChar *refs;
    struct charview *views;
    struct charinfo *charinfo;
    struct splinefont *parent;
    unsigned int changed: 1;
    unsigned int changedsincelasthinted: 1;
    unsigned int manualhints: 1;
    unsigned int ticked: 1;	/* For reference character processing */
    unsigned int changed_since_autosave: 1;
    unsigned int widthset: 1;	/* needed so an emspace char doesn't disappear */
    unsigned int vconflicts: 1;	/* Any hint overlaps in the vstem list? */
    unsigned int hconflicts: 1;	/* Any hint overlaps in the hstem list? */
    unsigned int anyflexes: 1;
    unsigned int searcherdummy: 1;
    unsigned int changed_since_search: 1;
    unsigned int wasopen: 1;
    unsigned int namechanged: 1;
    /* two bits left */
#if HANYANG
    unsigned int compositionunit: 1;
    int16 jamo, varient;
#endif
    struct splinecharlist { struct splinechar *sc; struct splinecharlist *next;} *dependents;
	    /* The dependents list is a list of all characters which refenence*/
	    /*  the current character directly */
    SplinePointList *backgroundsplines;
    ImageList *backimages;
    Undoes *undoes[2];
    Undoes *redoes[2];
    KernPair *kerns;
    /* KernPair *vkerns;*/
    PST *possub;		/* If we are a ligature then this tells us what */
				/*  It may also contain a bunch of other stuff now */
    LigList *ligofme;		/* If this is the first character of a ligature then this gives us the list of possible ones */
				/*  this field must be regenerated before the font is saved */
    unichar_t *comment;
    uint32 /*Color*/ color;
    AnchorPoint *anchor;
    uint32 script;		/* an opentype script tag-- four letters naming a script like 'latn' */
    uint8 *ttf_instrs;
    int16 ttf_instrs_len;
} SplineChar;

enum ttfnames { ttf_copyright=0, ttf_family, ttf_subfamily, ttf_uniqueid,
    ttf_fullname, ttf_version, ttf_postscriptname, ttf_trademark,
    ttf_manufacturer, ttf_designer, ttf_descriptor, ttf_venderurl,
    ttf_designerurl, ttf_license, ttf_licenseurl, ttf_idontknow, ttf_preffamilyname,
    ttf_prefmodifiers, ttf_compatfull, ttf_sampletext, ttf_namemax };
struct ttflangname {
    int lang;
    unichar_t *names[ttf_namemax];
    struct ttflangname *next;
};

struct remap { uint32 firstenc, lastenc; int32 infont; };

typedef struct splinefont {
    char *fontname, *fullname, *familyname, *weight;
    char *copyright;
    char *filename;
    char *version;
    real italicangle, upos, uwidth;		/* In font info */
    int ascent, descent;
    int vertical_origin;			/* height of vertical origin in character coordinate system */
    int uniqueid;				/* Not copied when reading in!!!! */
    int charcnt;
    SplineChar **chars;
    unsigned int changed: 1;
    unsigned int changed_since_autosave: 1;
    unsigned int changed_since_xuidchanged: 1;
    unsigned int display_antialias: 1;
    unsigned int dotlesswarn: 1;		/* User warned that font doesn't have a dotless i character */
    unsigned int onlybitmaps: 1;		/* it's a bdf editor, not a postscript editor */
    unsigned int serifcheck: 1;			/* Have we checked to see if we have serifs? */
    unsigned int issans: 1;			/* We have no serifs */
    unsigned int isserif: 1;			/* We have serifs. If neither set then we don't know. */
    unsigned int hasvmetrics: 1;		/* We've got vertical metric data and should output vhea/vmtx/VORG tables */
    unsigned int loading_cid_map: 1;
    unsigned int dupnamewarn: 1;		/* Warn about duplicate names when loading bdf font */
    unsigned int compacted: 1;			/* Font is in a compacted glyph list */
    unsigned int encodingchanged: 1;		/* Font's encoding has changed since it was loaded */
    unsigned int order2: 1;			/* Font's data are order 2 bezier splines (truetype) rather than order 3 (postscript) */
    struct fontview *fv;
    enum charset encoding_name, old_encname;
    SplinePointList *gridsplines;
    Undoes *gundoes, *gredoes;
#if 0
    int *hsnaps, *vsnaps;
#endif
    BDFFont *bitmaps;
    char *origname;		/* filename of font file (ie. if not an sfd) */
    char *autosavename;
    int display_size;		/* a val <0 => Generate our own images from splines, a value >0 => find a bdf font of that size */
    struct psdict *private;		/* read in from type1 file or provided by user */
    /*struct pschars *subrs;		/* actually an array, but this will do */
    char *xuid;
    struct pfminfo {
	unsigned int pfmset: 1;
	unsigned char pfmfamily;
	int16 weight;
	int16 width;
	char panose[10];
	int16 fstype;
	int16 linegap;
	int16 vlinegap;
    } pfminfo;
    struct ttflangname *names;
    char *cidregistry, *ordering;
    int supplement;
    int subfontcnt;
    struct splinefont **subfonts;
    struct splinefont *cidmaster;		/* Top level cid font */
    float cidversion;
#if HANYANG
    struct compositionrules *rules;
#endif
    char *comments;
    struct remap *remap;
    int tempuniqueid;
    int top_enc;
    uint16 desired_row_cnt, desired_col_cnt;
    AnchorClass *anchor;
    struct glyphnamehash *glyphnames;
    struct table_ordering { uint32 table_tag; uint32 *ordered_features; struct table_ordering *next; } *orders;
    struct ttf_table {
	uint32 tag;
	int32 len, maxlen;
	uint8 *data;
	struct ttf_table *next;
    } *ttf_tables;
	/* We copy: fpgm, prep, cvt, maxp */
    struct instrdata *instr_dlgs;	/* Pointer to all table and character instruction dlgs in this font */
} SplineFont;

/* mac styles. Useful idea we'll just steal it */
enum style_flags { sf_bold = 1, sf_italic = 2, sf_underline = 4, sf_outline = 8,
	sf_shadow = 0x10, sf_condense = 0x20, sf_extend = 0x40 };

struct sflist {
    SplineFont *sf;
    int32 *sizes;
    FILE *tempttf;		/* For ttf */
    int id;			/* For ttf */
    int* ids;			/* One for each size */
    BDFFont **bdfs;		/* Ditto */
    struct sflist *next;
};

    /* Used for drawing text with mark to base anchors */
typedef struct anchorpos {
    SplineChar *sc;		/* This is the mark being positioned */
    int x,y;			/* Its origin should be shifted this much relative to that of the original base char */
    AnchorPoint *apm;		/* The anchor point in sc used to position it */
    AnchorPoint *apb;		/* The anchor point in the base character against which we are positioned */
    int base_index;		/* Index in this array to the base character (-1=> original base char) */
    unsigned int ticked: 1;	/* Used as a mark to mark */
} AnchorPos;

enum ttf_flags { ttf_flag_shortps = 1, ttf_flag_nohints = 2, ttf_flag_applemode=4 };
enum openflags { of_fstypepermitted=1 };

struct fontdict;
struct pschars;
struct findsel;
struct charprocs;
enum charset;
struct enc;

extern void *chunkalloc(int size);
extern void chunkfree(void *, int size);

extern SplineFont *SplineFontFromPSFont(struct fontdict *fd);
extern int CheckAfmOfPostscript(SplineFont *sf,char *psname);
extern int LoadKerningDataFromAfm(SplineFont *sf, char *filename);
extern int LoadKerningDataFromTfm(SplineFont *sf, char *filename);
extern int LoadKerningDataFromMacFOND(SplineFont *sf, char *filename);
extern int SFOneWidth(SplineFont *sf);
extern int CIDOneWidth(SplineFont *sf);
extern int SFOneHeight(SplineFont *sf);
extern int SFIsCJK(SplineFont *sf);
extern struct pschars *SplineFont2Chrs(SplineFont *sf, int round, int iscjk,
	struct pschars *subrs);
struct cidbytes;
struct fd2data;
struct ttfinfo;
struct alltabs;
extern struct pschars *CID2Chrs(SplineFont *cidmaster,struct cidbytes *cidbytes);
extern struct pschars *SplineFont2Subrs2(SplineFont *sf);
extern struct pschars *SplineFont2Chrs2(SplineFont *sf, int nomwid, int defwid,
	struct pschars *subrs);
extern struct pschars *CID2Chrs2(SplineFont *cidmaster,struct fd2data *fds);
enum fontformat { ff_pfa, ff_pfb, ff_pfbmacbin, ff_multiple, ff_ptype3, ff_ptype0, ff_cid,
	ff_ttf, ff_ttfsym, ff_ttfmacbin, ff_ttfdfont, ff_otf, ff_otfdfont,
	ff_otfcid, ff_otfciddfont, ff_none };
enum bitmapformat { bf_bdf, bf_ttf, bf_sfnt_dfont, 
	bf_nfntmacbin, bf_nfntdfont, bf_fon, bf_none };
extern SplineChar *SFFindExistingCharMac(SplineFont *,int unienc);
extern int _WritePSFont(FILE *out,SplineFont *sf,enum fontformat format);
extern int WritePSFont(char *fontname,SplineFont *sf,enum fontformat format);
extern int WriteMacPSFont(char *fontname,SplineFont *sf,enum fontformat format);
extern int _WriteTTFFont(FILE *ttf,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags);
extern int WriteTTFFont(char *fontname,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags);
extern int WriteMacTTFFont(char *fontname,SplineFont *sf, enum fontformat format,
	int32 *bsizes, enum bitmapformat bf,int flags);
extern int WriteMacBitmaps(char *filename,SplineFont *sf, int32 *sizes,int is_dfont);
extern int WriteMacFamily(char *filename,struct sflist *sfs,enum fontformat format,
	enum bitmapformat bf,int flags);
extern void SfListFree(struct sflist *sfs);
extern struct ttflangname *TTFLangNamesCopy(struct ttflangname *old);
extern void TTF_PSDupsDefault(SplineFont *sf);
extern void DefaultTTFEnglishNames(struct ttflangname *dummy, SplineFont *sf);
extern void OS2FigureCodePages(SplineFont *sf, uint32 CodePage[2]);
extern void SFDefaultOS2Info(struct pfminfo *pfminfo,SplineFont *sf,char *fontname);
extern uint32 ScriptFromUnicode(int u,SplineFont *sf);
extern void SFFindNearTop(SplineFont *);
extern void SFRestoreNearTop(SplineFont *);
extern int SFAddDelChars(SplineFont *sf, int nchars);
extern int SFForceEncoding(SplineFont *sf,enum charset new_map);
extern int SFReencodeFont(SplineFont *sf,enum charset new_map);
extern int SFCompactFont(SplineFont *sf);
extern int SFUncompactFont(SplineFont *sf);
extern int CountOfEncoding(enum charset encoding_name);
extern int SFMatchEncoding(SplineFont *sf,SplineFont *target);
extern char *SFGetModifiers(SplineFont *sf);
extern void SFSetFontName(SplineFont *sf, char *family, char *mods, char *full);
extern void ttfdumpbitmap(SplineFont *sf,struct alltabs *at,int32 *sizes);
extern void ttfdumpbitmapscaling(SplineFont *sf,struct alltabs *at,int32 *sizes);

extern int RealNear(real a,real b);
extern int RealNearish(real a,real b);
extern int RealApprox(real a,real b);
extern int RealWithin(real a,real b,real fudge);

extern void LineListFree(LineList *ll);
extern void LinearApproxFree(LinearApprox *la);
extern void SplineFree(Spline *spline);
extern SplinePoint *SplinePointCreate(real x, real y);
extern void SplinePointFree(SplinePoint *sp);
extern void SplinePointMDFree(SplineChar *sc,SplinePoint *sp);
extern void SplinePointListFree(SplinePointList *spl);
extern void SplinePointListMDFree(SplineChar *sc,SplinePointList *spl);
extern void SplinePointListsFree(SplinePointList *head);
extern void RefCharFree(RefChar *ref);
extern void RefCharsFree(RefChar *ref);
extern void RefCharsFreeRef(RefChar *ref);
extern void UndoesFree(Undoes *undo);
extern void StemInfosFree(StemInfo *h);
extern void StemInfoFree(StemInfo *h);
extern void DStemInfosFree(DStemInfo *h);
extern void DStemInfoFree(DStemInfo *h);
extern void KernPairsFree(KernPair *kp);
extern void SCOrderAP(SplineChar *sc);
extern void AnchorPointsFree(AnchorPoint *kp);
extern AnchorPoint *AnchorPointsCopy(AnchorPoint *alist);
extern void SFRemoveAnchorClass(SplineFont *sf,AnchorClass *an);
extern AnchorPoint *APAnchorClassMerge(AnchorPoint *anchors,AnchorClass *into,AnchorClass *from);
extern void AnchorClassMerge(SplineFont *sf,AnchorClass *into,AnchorClass *from);
extern void AnchorClassesFree(AnchorClass *kp);
extern void TableOrdersFree(struct table_ordering *ord);
extern void TtfTablesFree(struct ttf_table *tab);
extern AnchorClass *AnchorClassMatch(SplineChar *sc1,SplineChar *sc2,
	AnchorClass *restrict_, AnchorPoint **_ap1,AnchorPoint **_ap2 );
extern AnchorClass *AnchorClassMkMkMatch(SplineChar *sc1,SplineChar *sc2,
	AnchorPoint **_ap1,AnchorPoint **_ap2 );
extern AnchorClass *AnchorClassCursMatch(SplineChar *sc1,SplineChar *sc2,
	AnchorPoint **_ap1,AnchorPoint **_ap2 );
extern void PSTFree(PST *lig);
extern void PSTsFree(PST *lig);
extern StemInfo *StemInfoCopy(StemInfo *h);
extern DStemInfo *DStemInfoCopy(DStemInfo *h);
extern MinimumDistance *MinimumDistanceCopy(MinimumDistance *h);
extern PST *PSTCopy(PST *base,SplineChar *sc);
extern SplineChar *SplineCharCopy(SplineChar *sc);
extern BDFChar *BDFCharCopy(BDFChar *bc);
extern void BitmapsCopy(SplineFont *to, SplineFont *from, int to_index, int from_index );
extern struct gimage *ImageAlterClut(struct gimage *image);
extern void ImageListsFree(ImageList *imgs);
extern void TTFLangNamesFree(struct ttflangname *l);
extern void MinimumDistancesFree(MinimumDistance *md);
extern SplineChar *SplineCharCreate(void);
extern void SplineCharListsFree(struct splinecharlist *dlist);
extern void SplineCharFreeContents(SplineChar *sc);
extern void SplineCharFree(SplineChar *sc);
extern void SplineFontFree(SplineFont *sf);
extern void SFRemoveUndoes(SplineFont *sf,char *selected);
extern void SplineRefigure3(Spline *spline);
extern void SplineRefigure(Spline *spline);
extern Spline *SplineMake3(SplinePoint *from, SplinePoint *to);
extern LinearApprox *SplineApproximate(Spline *spline, real scale);
extern int SplinePointListIsClockwise(SplineSet *spl);
extern void SplineSetFindBounds(SplinePointList *spl, DBounds *bounds);
extern void SplineCharFindBounds(SplineChar *sc,DBounds *bounds);
extern void SplineFontFindBounds(SplineFont *sf,DBounds *bounds);
extern void CIDFindBounds(SplineFont *sf,DBounds *bounds);
extern void SplineSetQuickBounds(SplineSet *ss,DBounds *b);
extern void SplineCharQuickBounds(SplineChar *sc, DBounds *b);
extern void SplineSetQuickConservativeBounds(SplineSet *ss,DBounds *b);
extern void SplineCharQuickConservativeBounds(SplineChar *sc, DBounds *b);
extern void SplineFontQuickConservativeBounds(SplineFont *sf,DBounds *b);
extern void SplinePointCatagorize(SplinePoint *sp);
extern int SplinePointIsACorner(SplinePoint *sp);
extern void SCCatagorizePoints(SplineChar *sc);
extern int UnicodeNameLookup(char *name);
extern SplinePointList *SplinePointListCopy1(SplinePointList *spl);
extern SplinePointList *SplinePointListCopy(SplinePointList *base);
extern SplinePointList *SplinePointListCopySelected(SplinePointList *base);
extern SplinePointList *SplinePointListTransform(SplinePointList *base, real transform[6], int allpoints );
extern SplinePointList *SplinePointListShift(SplinePointList *base, real xoff, int allpoints );
extern SplinePointList *SplinePointListRemoveSelected(SplineChar *sc,SplinePointList *base);
extern void SplinePointListSet(SplinePointList *tobase, SplinePointList *frombase);
extern void SplinePointListSelect(SplinePointList *spl,int sel);
extern void SCRefToSplines(SplineChar *sc,RefChar *rf);
extern void SCReinstanciateRefChar(SplineChar *sc,RefChar *rf);
extern void SCReinstanciateRef(SplineChar *sc,SplineChar *rsc);
extern SplineChar *MakeDupRef(SplineChar *base, int local_enc, int uni_enc);
extern void SCRemoveDependent(SplineChar *dependent,RefChar *rf);
extern void SCRemoveDependents(SplineChar *dependent);
extern RefChar *SCCanonicalRefs(SplineChar *sc, int isps);
extern int SCDependsOnSC(SplineChar *parent, SplineChar *child);
extern void BCCompressBitmap(BDFChar *bdfc);
extern void BCRegularizeBitmap(BDFChar *bdfc);
extern void BCRegularizeGreymap(BDFChar *bdfc);
extern void BCPasteInto(BDFChar *bc,BDFChar *rbc,int ixoff,int iyoff, int invert, int cleartoo);
extern void BCRotateCharForVert(BDFChar *bc,BDFChar *from, BDFFont *frombdf);
extern BDFChar *SplineCharRasterize(SplineChar *sc, int pixelsize);
extern BDFFont *SplineFontToBDFHeader(SplineFont *_sf, int pixelsize, int indicate);
extern BDFFont *SplineFontRasterize(SplineFont *sf, int pixelsize, int indicate);
extern BDFChar *SplineCharAntiAlias(SplineChar *sc, int pixelsize,int linear_scale);
extern BDFFont *SplineFontAntiAlias(SplineFont *sf, int pixelsize,int linear_scale);
extern void BDFClut(BDFFont *bdf, int linear_scale);
extern int BDFDepth(BDFFont *bdf);
extern BDFChar *BDFPieceMeal(BDFFont *bdf, int index);
enum piecemeal_flags { pf_antialias=1, pf_bbsized=2 };
extern BDFFont *SplineFontPieceMeal(SplineFont *sf,int pixelsize,int flags,void *freetype_context);
extern BDFFont *BitmapFontScaleTo(BDFFont *old, int to);
extern void BDFCharFree(BDFChar *bdfc);
extern void BDFFontFree(BDFFont *bdf);
extern int  BDFFontDump(char *filename,BDFFont *font, char *encodingname,int res);
extern int  FONFontDump(char *filename,BDFFont *font, int res);
extern int SplinesIntersect(Spline *s1, Spline *s2, BasePoint pts[4], real t1s[4], real t2s[4]);
extern int CubicSolve(Spline1D *sp,real ts[3]);
extern real SplineSolve(Spline1D *sp, real tmin, real tmax, real sought_y, real err);
extern int SplineSolveFull(Spline1D *sp,real val, real ts[3]);
extern void SplineFindExtrema(Spline1D *sp, real *_t1, real *_t2 );
extern void SplineRemoveInflectionsTooClose(Spline1D *sp, real *_t1, real *_t2 );
extern int NearSpline(struct findsel *fs, Spline *spline);
extern real SplineNearPoint(Spline *spline, BasePoint *bp, real fudge);
extern void SCMakeDependent(SplineChar *dependent,SplineChar *base);
extern SplinePoint *SplineBisect(Spline *spline, double t);
extern Spline *SplineSplit(Spline *spline, real ts[3]);
extern Spline *ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt);
extern Spline *ApproximateSplineFromPointsSlopes(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt);
extern double SplineLength(Spline *spline);
extern int SplineIsLinear(Spline *spline);
extern int SplineIsLinearMake(Spline *spline);
extern int SplineInSplineSet(Spline *spline, SplineSet *spl);
extern void SplineCharMerge(SplineChar *sc,SplineSet **head,int type);
enum simpify_flags { sf_cleanup=-1, sf_normal=0, sf_ignoreslopes=1, sf_ignoreextremum=2 };
extern void SplinePointListSimplify(SplineChar *sc,SplinePointList *spl,int flags,double err);
extern SplineSet *SplineCharSimplify(SplineChar *sc,SplineSet *head,int flags,double err);
extern SplineSet *SplineSetJoin(SplineSet *start,int doall,real fudge,int *changed);
extern void SplineCharAddExtrema(SplineSet *head,int between_selected);
extern SplineSet *SplineCharRemoveTiny(SplineChar *sc,SplineSet *head);
extern SplineFont *SplineFontNew(void);
extern char *GetNextUntitledName(void);
extern SplineFont *SplineFontEmpty(void);
extern SplineFont *SplineFontBlank(int enc,int charcnt);
extern void SFIncrementXUID(SplineFont *sf);
extern void SFRandomChangeXUID(SplineFont *sf);
extern SplineSet *SplineSetReverse(SplineSet *spl);
extern SplineSet *SplineSetsExtractOpen(SplineSet **tbase);
extern SplineSet *SplineSetsCorrect(SplineSet *base,int *changed);
extern SplineSet *SplineSetsDetectDir(SplineSet **_base, int *lastscan);
extern void SPAverageCps(SplinePoint *sp);
extern void SPWeightedAverageCps(SplinePoint *sp);
extern void SplineCharDefaultPrevCP(SplinePoint *base);
extern void SplineCharDefaultNextCP(SplinePoint *base);
extern void SplineCharTangentNextCP(SplinePoint *sp);
extern void SplineCharTangentPrevCP(SplinePoint *sp);
extern int PointListIsSelected(SplinePointList *spl);
extern void SplineSetsUntick(SplineSet *spl);
extern void SFOrderBitmapList(SplineFont *sf);
extern int KernThreshold(SplineFont *sf, int cnt);
extern real SFGuessItalicAngle(SplineFont *sf);
extern void SFHasSerifs(SplineFont *sf);

extern SplinePoint *SplineTtfApprox(Spline *ps);
extern SplineSet *SSttfApprox(SplineSet *ss);
extern SplineSet *SplineSetsTTFApprox(SplineSet *ss);
extern SplineSet *SSPSApprox(SplineSet *ss);
extern SplineSet *SplineSetsPSApprox(SplineSet *ss);
extern SplineSet *SplineSetsConvertOrder(SplineSet *ss, int to_order2);
extern void SplineRefigure2(Spline *spline);
extern void SplineRefigureFixup(Spline *spline);
extern Spline *SplineMake2(SplinePoint *from, SplinePoint *to);
extern Spline *SplineMake(SplinePoint *from, SplinePoint *to, int order2);
extern Spline *SFSplineMake(SplineFont *sf,SplinePoint *from, SplinePoint *to);
extern void SCConvertToOrder2(SplineChar *sc);
extern void SFConvertToOrder2(SplineFont *sf);
extern void SCConvertToOrder3(SplineChar *sc);
extern void SFConvertToOrder3(SplineFont *sf);
extern void SCConvertOrder(SplineChar *sc, int to_order2);
extern void SplinePointPrevCPChanged2(SplinePoint *sp, int fixnext);
extern void SplinePointNextCPChanged2(SplinePoint *sp, int fixprev);
extern int IntersectLines(BasePoint *inter,
	BasePoint *line1_1, BasePoint *line1_2,
	BasePoint *line2_1, BasePoint *line2_2);
extern int IntersectLinesClip(BasePoint *inter,
	BasePoint *line1_1, BasePoint *line1_2,
	BasePoint *line2_1, BasePoint *line2_2);

extern SplineSet *SplineSetStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc);
extern SplineSet *SplineSetRemoveOverlap(SplineSet *base,int justintersect);

extern void FindBlues( SplineFont *sf, real blues[14], real otherblues[10]);
extern void QuickBlues(SplineFont *sf, BlueData *bd);
extern void FindHStems( SplineFont *sf, real snaps[12], real cnt[12]);
extern void FindVStems( SplineFont *sf, real snaps[12], real cnt[12]);
extern void SCGuessHHintInstancesAndAdd(SplineChar *sc, StemInfo *stem, real guess1, real guess2);
extern void SCGuessVHintInstancesAndAdd(SplineChar *sc, StemInfo *stem, real guess1, real guess2);
extern void SCGuessHHintInstancesList(SplineChar *sc);
extern void SCGuessVHintInstancesList(SplineChar *sc);
extern real HIlen( StemInfo *stems);
extern real HIoverlap( HintInstance *mhi, HintInstance *thi);
extern int StemInfoAnyOverlaps(StemInfo *stems);
extern int StemListAnyConflicts(StemInfo *stems);
extern HintInstance *HICopyTrans(HintInstance *hi, real mul, real offset);
extern void MDAdd(SplineChar *sc, int x, SplinePoint *sp1, SplinePoint *sp2);
extern int SFNeedsAutoHint( SplineFont *_sf);
extern void SCAutoInstr( SplineChar *sc,BlueData *bd );
extern void SplineCharAutoHint( SplineChar *sc,int removeOverlaps);
extern void SplineFontAutoHint( SplineFont *sf);
extern StemInfo *HintCleanup(StemInfo *stem,int dosort);
extern int SplineFontIsFlexible(SplineFont *sf);
extern int SCWorthOutputting(SplineChar *sc);
extern SplineChar *SCDuplicate(SplineChar *sc);
extern int SCIsNotdef(SplineChar *sc,int isfixed);
extern int IsntBDFChar(BDFChar *bdfc);
extern int CIDWorthOutputting(SplineFont *cidmaster, int enc); /* Returns -1 on failure, font number on success */
extern int AfmSplineFont(FILE *afm, SplineFont *sf,int formattype);
extern int PfmSplineFont(FILE *pfm, SplineFont *sf,int type0);
extern char *EncodingName(int map);
extern void SFLigaturePrepare(SplineFont *sf);
extern void SFLigatureCleanup(SplineFont *sf);
extern int SCSetMetaData(SplineChar *sc,char *name,int unienc,const unichar_t *comment);

extern int SFDWrite(char *filename,SplineFont *sf);
extern int SFDWriteBak(SplineFont *sf);
extern SplineFont *SFDRead(char *filename);
extern SplineChar *SFDReadOneChar(char *filename,const char *name);
extern unichar_t *TTFGetFontName(FILE *ttf,int32 offset,int32 off2);
extern void TTFLoadBitmaps(FILE *ttf,struct ttfinfo *info, int onlyone);
enum ttfflags { ttf_onlystrikes=1, ttf_onlyonestrike=2, ttf_onlykerns=4 };
extern SplineFont *_SFReadTTF(FILE *ttf,int flags,char *filename);
extern SplineFont *SFReadTTF(char *filename,int flags);
extern SplineFont *CFFParse(FILE *temp,int len,char *fontsetname);
extern SplineFont *SFReadMacBinary(char *filename,int flags);
extern SplineFont *SFReadWinFON(char *filename,int toback);
extern SplineFont *LoadSplineFont(char *filename,enum openflags);
extern SplineFont *ReadSplineFont(char *filename,enum openflags);	/* Don't use this, use LoadSF instead */
extern uint16 MacStyleCode( SplineFont *sf, uint16 *psstyle );
extern SplineFont *SFReadIkarus(char *fontname);

extern const char *UnicodeRange(int unienc);
extern SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,int i);
extern SplineChar *SFMakeChar(SplineFont *sf,int i);
extern char *AdobeLigatureFormat(char *name);
extern uint32 LigTagFromUnicode(int uni);
extern void SCLigDefault(SplineChar *sc);
extern void SCTagDefault(SplineChar *sc,uint32 tag);
extern void SCSuffixDefault(SplineChar *sc,uint32 tag,char *suffix,uint16 flags);
extern void SCLigCaretCheck(SplineChar *sc,int clean);
extern BDFChar *BDFMakeChar(BDFFont *bdf,int i);

extern void SCUndoSetLBearingChange(SplineChar *sc,int lb);
extern Undoes *SCPreserveState(SplineChar *sc,int dohints);
extern Undoes *SCPreserveBackground(SplineChar *sc);
extern Undoes *SCPreserveWidth(SplineChar *sc);
extern Undoes *SCPreserveVWidth(SplineChar *sc);
extern Undoes *BCPreserveState(BDFChar *bc);
extern void BCDoRedo(BDFChar *bc,struct fontview *fv);
extern void BCDoUndo(BDFChar *bc,struct fontview *fv);

extern int SFIsCompositBuildable(SplineFont *sf,int unicodeenc,SplineChar *sc);
extern int SFIsSomethingBuildable(SplineFont *sf,SplineChar *sc,int onlyaccents);
extern int SFIsRotatable(SplineFont *sf,SplineChar *sc);
extern int SCMakeDotless(SplineFont *sf, SplineChar *dotless, int copybmp, int doit);
extern void SCBuildComposit(SplineFont *sf, SplineChar *sc, int copybmp,
	struct fontview *fv);
extern const unichar_t *SFGetAlternate(SplineFont *sf, int base,SplineChar *sc,int nocheck);

extern int getAdobeEnc(char *name);

extern SplinePointList *SplinePointListInterpretPS(FILE *ps);
extern void PSFontInterpretPS(FILE *ps,struct charprocs *cp);
extern struct enc *PSSlurpEncodings(FILE *file);
extern SplineChar *PSCharStringToSplines(uint8 *type1, int len, int is_type2,
	struct pschars *subrs, struct pschars *gsubrs, const char *name);

extern void GlyphHashFree(SplineFont *sf);
extern int SFFindChar(SplineFont *sf, int unienc, char *name );
extern int SFCIDFindChar(SplineFont *sf, int unienc, char *name );
extern SplineChar *SFGetChar(SplineFont *sf, int unienc, char *name );
extern SplineChar *SFGetCharDup(SplineFont *sf, int unienc, char *name );
extern SplineChar *SFGetOrMakeChar(SplineFont *sf, int unienc, char *name );
extern int SFFindExistingChar(SplineFont *sf, int unienc, char *name );
extern int SFCIDFindExistingChar(SplineFont *sf, int unienc, char *name );
extern int SFHasCID(SplineFont *sf, int cid);

extern char *getPfaEditDir(char *buffer);
extern void DoAutoSaves(void);
extern void CleanAutoRecovery(void);
extern int DoAutoRecovery(void);
extern SplineFont *SFRecoverFile(char *autosavename);
extern void SFAutoSave(SplineFont *sf);
extern void SFClearAutoSave(SplineFont *sf);

extern void PSCharsFree(struct pschars *chrs);
extern void PSDictFree(struct psdict *chrs);
extern struct psdict *PSDictCopy(struct psdict *dict);
extern int PSDictFindEntry(struct psdict *dict, char *key);
extern char *PSDictHasEntry(struct psdict *dict, char *key);
extern int PSDictRemoveEntry(struct psdict *dict, char *key);
extern int PSDictChangeEntry(struct psdict *dict, char *key, char *newval);

extern void SplineSetsRound2Int(SplineSet *spl);
extern void SCRound2Int(SplineChar *sc);
extern int hascomposing(SplineFont *sf,int u,SplineChar *sc);
#if 0
extern void SFFigureGrid(SplineFont *sf);
#endif
extern int FVWinInfo(struct fontview *,int *cc,int *rc);

struct cidmap;			/* private structure to encoding.c */
extern int CID2NameEnc(struct cidmap *map,int cid, char *buffer, int len);
extern int NameEnc2CID(struct cidmap *map,int enc, char *name);
extern int MaxCID(struct cidmap *map);
extern struct cidmap *FindCidMap(char *registry,char *ordering,int supplement,
	SplineFont *sf);
extern void SFEncodeToMap(SplineFont *sf,struct cidmap *map);
extern struct cidmap *AskUserForCIDMap(SplineFont *sf);
extern SplineFont *CIDFlatten(SplineFont *cidmaster,SplineChar **chars,int charcnt);
extern void SFFlattenByCMap(SplineFont *sf,char *cmapname);
extern SplineFont *MakeCIDMaster(SplineFont *sf,int bycmap,char *cmapfilename);

int getushort(FILE *ttf);
int32 getlong(FILE *ttf);
int get3byte(FILE *ttf);
real getfixed(FILE *ttf);
real get2dot14(FILE *ttf);
void putshort(FILE *file,int sval);
void putlong(FILE *file,int val);
void putfixed(FILE *file,real dval);
int ttfcopyfile(FILE *ttf, FILE *other, int pos);

extern void SCCopyFgToBg(SplineChar *sc,int show);

extern int hasFreeType(void);
extern int hasFreeTypeDebugger(void);
extern int hasFreeTypeByteCode(void);
extern void *_FreeTypeFontContext(SplineFont *sf,SplineChar *sc,struct fontview *fv,
	enum fontformat ff,int flags,void *shared_ftc);
extern void *FreeTypeFontContext(SplineFont *sf,SplineChar *sc,struct fontview *fv);
extern BDFFont *SplineFontFreeTypeRasterize(void *freetypecontext,int pixelsize,int depth);
extern BDFChar *SplineCharFreeTypeRasterize(void *freetypecontext,int enc,
	int pixelsize,int depth);
extern void FreeTypeFreeContext(void *freetypecontext);
extern SplineSet *FreeType_GridFitChar(void *single_glyph_context,
	int enc, real ptsize, int dpi, int16 *width, SplineSet *splines);
extern struct freetype_raster *FreeType_GetRaster(void *single_glyph_context,
	int enc, real ptsize, int dpi);
extern void FreeType_FreeRaster(struct freetype_raster *raster);

extern int UniFromName(const char *name);
extern int uUniFromName(const unichar_t *name);

extern void doversion(void);

extern AnchorPos *AnchorPositioning(SplineChar *sc,unichar_t *ustr,SplineChar **sstr );
extern void AnchorPosFree(AnchorPos *apos);

extern int SFCloseAllInstrs(SplineFont *sf);
extern void SCMarkInstrDlgAsChanged(SplineChar *sc);


# if HANYANG
extern void SFDDumpCompositionRules(FILE *sfd,struct compositionrules *rules);
extern struct compositionrules *SFDReadCompositionRules(FILE *sfd);
extern void SFModifyComposition(SplineFont *sf);
extern void SFBuildSyllables(SplineFont *sf);
# endif
#endif

