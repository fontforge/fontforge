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

#ifndef FONTFORGE_SPLINEFONT_H
#define FONTFORGE_SPLINEFONT_H

#include "basics.h"
#include "dlist.h"
#include "gwwiconv.h"
#include "ustring.h"
#include "splinechar.h"
#include "splinefont_enums.h"

#include <locale.h>

#ifdef __cplusplus
extern "C" {
#endif

#define extended	double
	/* Solaris wants to define extended to be unsigned [3] unless we do this*/
#define _EXTENDED

#define CHR(ch1,ch2,ch3,ch4) (((ch1)<<24)|((ch2)<<16)|((ch3)<<8)|(ch4))

/* The maximum number of layers allowed in a normal font (this includes the */
/*  default foreground and background layers) -- this does not limit type3  */
/*  fonts */
#define BACK_LAYER_MAX 256

typedef struct dbasepoint {
    bigreal x;
    bigreal y;
} DBasePoint;

#define DBASEPOINT_EMPTY { (bigreal)0.0, (bigreal)0.0 }

typedef struct ibounds {
    int minx, maxx;
    int miny, maxy;
} IBounds;

#define IBOUNDS_EMPTY { 0, 0, 0, 0 }


enum val_type { v_int, v_real, v_str, v_unicode, v_lval, v_arr, v_arrfree,
		v_void };

enum val_flags {
    vf_none = 0,
    vf_dontfree = (1 << 1)
};

typedef struct val {
    enum val_type type;
    enum val_flags flags;
    union {
	int ival;
	real fval;
	char *sval;
	struct val *lval;
	struct array *aval;
    } u;
} Val;		/* Used by scripting */

struct psdict {
    int cnt;		/* *key[0..cnt] and *values[0..cnt] currently available */
    int next;		/* **key[0..next] and **values[0..next] currently used  */
    char **keys;
    char **values;
};

struct pschars {
    int cnt, next;
    char **keys;
    uint8_t **values;
    int *lens;
    int bias;		/* for type2 strings */
};

enum linejoin {
    lj_miter,		/* Extend lines until they meet */
    lj_round,		/* connect with arc (not w/ stroking - see "nib") */
    lj_bevel,		/* Straight line between the ends of next and prev */
    lj_miterclip,	/* Extend lines until they meet */
    lj_nib,		/* Join with the nib shape */
    lj_arcs,
    lj_inherited
};
enum linecap {
    lc_butt,		/* Finish with line perpendicular to end tangent */
    lc_round,		/* semi-circle (not w/ stroking - see "nib") */
    lc_square,		/* Not used w/ stroking - use lc_butt w/ extend */
    lc_nib,		/* cap with the nib shape */
    lc_bevel,		/* Just join endpoints with a line */
    lc_inherited
};
enum stroke_rmov {
    srmov_layer = 0,
    srmov_contour = 1,
    srmov_none = 2
};
enum stroke_arclimit {
    sal_auto = 0,
    sal_svg2 = 1,
    sal_ratio = 2
};

#define JLIMIT_INHERITED (-1)

struct spline;
enum si_type { si_round, si_calligraphic, si_nib, si_centerline };
/* If you change this structure you may need to update MakeStrokeDlg */
/*  and cvpalettes.c -- both contain statically initialized StrokeInfos */
typedef struct strokeinfo {
    bigreal width;			/* or major axis of pen */
    enum linejoin join;
    enum linecap cap;
    enum si_type stroke_type;
    enum stroke_rmov rmov;
    enum stroke_arclimit al;
    // Could be bits but the python interface would be annoying
    int removeinternal, removeexternal, simplify, extrema;
    int leave_users_center, jlrelative, ecrelative;
    bigreal penangle, height, extendcap, joinlimit, accuracy_target;
    struct splinepointlist *nib;
/* For freehand tool, not currently used in practice */
    real radius2;
    int pressure1, pressure2;
    void *data;
    bigreal (*factor)(void *data,struct spline *spline,real t);
/* End freehand */
} StrokeInfo;

extern StrokeInfo *InitializeStrokeInfo(StrokeInfo *sip);
extern void SITranslatePSArgs(StrokeInfo *sip, enum linejoin lj,
                              enum linecap lc);

enum overlap_type { over_remove, over_rmselected, over_intersect, over_intersel,
	over_exclude, over_findinter, over_fisel };

enum simpify_flags { sf_cleanup=-1, sf_normal=0, sf_ignoreslopes=1,
	sf_ignoreextremum=2, sf_smoothcurves=4, sf_choosehv=8,
	sf_forcelines=0x10, sf_nearlyhvlines=0x20,
	sf_mergelines=0x40, sf_setstart2extremum=0x80,
	sf_rmsingletonpoints=0x100 };
struct simplifyinfo {
    int flags;
    bigreal err;
    bigreal tan_bounds;
    bigreal linefixup;
    bigreal linelenmax;		/* Don't simplify any straight lines longer than this */
    int set_as_default;
    int check_selected_contours;
};

struct hsquash { double lsb_percent, stem_percent, counter_percent, rsb_percent; };

enum serif_type { srf_flat, srf_simpleslant, srf_complexslant };
/* |    | (flat)    |   | (simple)     |    | (complex) */
/* |    |           |  /               |   /            */
/* |    |           | /                |  /             */
/* +----+           |/                 \ /              */

typedef struct italicinfo {
    double italic_angle;
    double xheight_percent;
    struct hsquash lc, uc, neither;
    enum serif_type secondary_serif;

    unsigned int transform_bottom_serifs: 1;
    unsigned int transform_top_xh_serifs: 1;	/* Those at x-height */
    unsigned int transform_top_as_serifs: 1;	/* Those at ascender-height */
    unsigned int transform_diagon_serifs: 1;	/* Those at baseline/xheight */

    unsigned int a_from_d: 1;		/* replace the "a" glyph with the variant which looks like a "d" without an ascender */
  /* When I say "f" I also mean "f_f" ligature, "longs", cyrillic phi and other things shaped like "f" */
    unsigned int f_long_tail: 1;	/* Some Italic fonts have the "f" grow an extension of the main stem below the baseline */
    unsigned int f_rotate_top: 1;	/* Most Italic fonts take the top curve of the "f", rotate it 180 and attach to the bottom */
    unsigned int pq_deserif: 1;		/* Remove a serif from the descender of p or q and replace with a secondary serif as above */

  /* Unsupported */
    /* e becomes rounder, cross bar slightly slanted */
    /* g closed counter at bottom */
    /* k closed counter at top */
    /* v-z diagonal stems become more curvatious */

    unsigned int cyrl_phi: 1;		/* Gains an "f" like top, bottom treated like "f" */
    unsigned int cyrl_i: 1;		/* Turns into a latin u */
    unsigned int cyrl_pi: 1;		/* Turns into a latin n */
    unsigned int cyrl_te: 1;		/* Turns into a latin m */
    unsigned int cyrl_sha: 1;		/* Turns into a latin m rotated 180 */
    unsigned int cyrl_dje: 1;		/* Turns into a latin smallcaps T */
    unsigned int cyrl_dzhe: 1;		/* Turns into a latin u */
		    /* Is there a difference between dzhe and i? both look like u to me */

  /* Unsupported */
    /* u432 curved B */
    /* u433 strange gamma */
    /* u434 normal delta */
    /* u436 */
    /* u43b lambda ? */
    /* u43c */
    /* u446 */
    /* u449 */
    /* u449 */
    /* u44a */

/* This half of the structure gets filled in later - see ITALICINFO_REMAINDER */
    double tan_ia;
    double x_height;
    double pq_depth;
    double ascender_height;
    double emsize;
    int order2;
    struct splinefont *sf;
    int layer;
    double serif_extent, serif_height;
    struct splinepoint *f_start, *f_end;		/* start has next pointing into the f head and up */
    struct splinepoint *ff_start1, *ff_end1, *ff_start2, *ff_end2;
    double f_height, ff_height;
} ItalicInfo;

#define ITALICINFO_REMAINDER 0, 0, 0, 0, 0, 0, NULL, 0, 0, 0, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0


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

#define BLUEDATA_EMPTY { \
    0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0, \
    { { 0.0, 0.0 }, { 0.0, 0.0 }, { 0.0, 0.0 }, { 0.0, 0.0 }, { 0.0, 0.0 }, { 0.0, 0.0 }, \
      { 0.0, 0.0 }, { 0.0, 0.0 }, { 0.0, 0.0 }, { 0.0, 0.0 }, { 0.0, 0.0 }, { 0.0, 0.0 }  \
    } \
}

/* OpenType does not document 'dflt' as a language, but we'll use it anyway. */
/* (Adobe uses it too) we'll turn it into a default entry when we output it. */
#define DEFAULT_LANG		CHR('d','f','l','t')
/* The OpenType spec says in one place that the default script is 'dflt' and */
/*  in another that it is 'DFLT'. 'DFLT' is correct */
#define DEFAULT_SCRIPT		CHR('D','F','L','T')
#define REQUIRED_FEATURE	CHR(' ','R','Q','D')

enum otlookup_type {
    ot_undef = 0,			/* Not a lookup type */
    gsub_start	       = 0x000,		/* Not a lookup type */
    gsub_single        = 0x001,
    gsub_multiple      = 0x002,
    gsub_alternate     = 0x003,
    gsub_ligature      = 0x004,
    gsub_context       = 0x005,
    gsub_contextchain  = 0x006,
     /* GSUB extension 7 */
    gsub_reversecchain = 0x008,
    /* mac state machines */
    morx_indic	       = 0x0fd,
    morx_context       = 0x0fe,
    morx_insert        = 0x0ff,
    /* ********************* */
    gpos_start         = 0x100,		/* Not a lookup type */

    gpos_single        = 0x101,
    gpos_pair          = 0x102,
    gpos_cursive       = 0x103,
    gpos_mark2base     = 0x104,
    gpos_mark2ligature = 0x105,
    gpos_mark2mark     = 0x106,
    gpos_context       = 0x107,
    gpos_contextchain  = 0x108,
    /* GPOS extension 9 */
    kern_statemachine  = 0x1ff

    /* otlookup&0xff == lookup type for the appropriate table */
    /* otlookup>>8:     0=>GSUB, 1=>GPOS */
};

enum otlookup_typemasks {
    gsub_single_mask        = 0x00001,
    gsub_multiple_mask      = 0x00002,
    gsub_alternate_mask     = 0x00004,
    gsub_ligature_mask      = 0x00008,
    gsub_context_mask       = 0x00010,
    gsub_contextchain_mask  = 0x00020,
    gsub_reversecchain_mask = 0x00040,
    morx_indic_mask         = 0x00080,
    morx_context_mask       = 0x00100,
    morx_insert_mask        = 0x00200,
    /* ********************* */
    gpos_single_mask        = 0x00400,
    gpos_pair_mask          = 0x00800,
    gpos_cursive_mask       = 0x01000,
    gpos_mark2base_mask     = 0x02000,
    gpos_mark2ligature_mask = 0x04000,
    gpos_mark2mark_mask     = 0x08000,
    gpos_context_mask       = 0x10000,
    gpos_contextchain_mask  = 0x20000,
    kern_statemachine_mask  = 0x40000
};

#define MAX_LANG 		4	/* If more than this we allocate more_langs in chunks of MAX_LANG */
struct scriptlanglist {
    uint32_t script;
    uint32_t langs[MAX_LANG];
    uint32_t *morelangs;
    int lang_cnt;
    struct scriptlanglist *next;
};

extern struct opentype_feature_friendlynames {
    uint32_t tag;
    char *tagstr;
    char *friendlyname;
    int masks;
} friendlies[];

#define OPENTYPE_FEATURE_FRIENDLYNAMES_EMPTY { 0, NULL, NULL, 0 }


typedef struct featurescriptlanglist {
    uint32_t featuretag;
    struct scriptlanglist *scripts;
    struct featurescriptlanglist *next;
    unsigned int ismac: 1;	/* treat the featuretag as a mac feature/setting */
} FeatureScriptLangList;

/* https://docs.microsoft.com/en-us/typography/opentype/spec/chapter2#lookupFlags */
/* Low order: traditional flags, High order: markset index, only meaningful if pst_usemarkfilteringset set */
enum pst_flags { pst_r2l=1, pst_ignorebaseglyphs=2, pst_ignoreligatures=4,
	pst_ignorecombiningmarks=8, pst_usemarkfilteringset=0x10,
	pst_markclass=0xff00 };
#define pst_markset 0xffff0000 /* Used as a bitmask to filter to the markset index */

struct lookup_subtable {
    char *subtable_name;
    char *suffix;			/* for gsub_single, used to find a default replacement */
    int16_t separation, minkern;	/* for gpos_pair, used to guess default kerning values */
    struct otlookup *lookup;
    unsigned int unused: 1;
    unsigned int per_glyph_pst_or_kern: 1;
    unsigned int anchor_classes: 1;
    unsigned int vertical_kerning: 1;
    unsigned int ticked: 1;
    unsigned int kerning_by_touch: 1;	/* for gpos_pair, calculate kerning so that glyphs will touch */
    unsigned int onlyCloser: 1;		/* for kerning classes */
    unsigned int dontautokern: 1;		/* for kerning classes */
    struct kernclass *kc;
    struct generic_fpst *fpst;
    struct generic_asm  *sm;
    /* Each time an item is added to a lookup we must place it into a */
    /*  subtable. If it's a kerning class, fpst or state machine it has */
    /*  a subtable all to itself. If it's an anchor class it can share */
    /*  a subtable with other anchor classes (merge with). If it's a glyph */
    /*  PST it may share a subtable with other PSTs */
    /* Note items may only be placed in lookups in which they fit. Can't */
    /*  put kerning data in a gpos_single lookup, etc. */
    struct lookup_subtable *next;
    int32_t subtable_offset;
    int32_t *extra_subtables;
    /* If a kerning subtable has too much stuff in it, we are prepared to */
    /*  break it up into several smaller subtables, each of which has */
    /*  an offset in this list (extra-subtables[0]==subtable_offset) */
    /*  the list is terminated by an entry of -1 */
};

typedef struct otlookup {
    struct otlookup *next;
    enum otlookup_type lookup_type;
    uint32_t lookup_flags;		/* Low order: traditional flags, High order: markset index, only meaningful if pst_usemarkfilteringset set */
    char *lookup_name;
    FeatureScriptLangList *features;
    struct lookup_subtable *subtables;
    unsigned int unused: 1;	/* No subtable is used (call SFFindUnusedLookups before examining) */
    unsigned int empty: 1;	/* No subtable is used, and no anchor classes are used */
    unsigned int store_in_afm: 1;	/* Used for ligatures, some get stored */
    					/*  'liga' generally does, but 'frac' doesn't */
    unsigned int needs_extension: 1;	/* Used during opentype generation */
    unsigned int temporary_kern: 1;	/* Used when decomposing kerning classes into kern pairs for older formats */
    unsigned int def_lang_checked: 1;
    unsigned int def_lang_found: 1;
    unsigned int ticked: 1;
    unsigned int in_gpos: 1;
    unsigned int in_jstf: 1;
    unsigned int only_jstf: 1;
    int16_t subcnt;		/* Actual number of subtables we will output */
				/* Some of our subtables may contain no data */
			        /* Some may be too big and need to be broken up.*/
			        /* So this field may be different than just counting the subtables */
    int lookup_index;		/* used during opentype generation */
    uint32_t lookup_offset;
    uint32_t lookup_length;
    char *tempname;
} OTLookup;

#define LOOKUP_SUBTABLE_EMPTY { NULL, NULL, 0, 0, NULL, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL, 0, NULL }
#define OTLOOKUP_EMPTY { NULL, 0, 0, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL }


typedef struct devicetab {
    uint16_t first_pixel_size, last_pixel_size;		/* A range of point sizes to which this table applies */
    int8_t *corrections;					/* a set of pixel corrections, one for each point size */
} DeviceTable;

typedef struct valdev {		/* Value records can have four associated device tables */
    DeviceTable xadjust;
    DeviceTable yadjust;
    DeviceTable xadv;
    DeviceTable yadv;
} ValDevTab;

enum anchorclass_type { act_mark, act_mkmk, act_curs, act_mklg, act_unknown };
typedef struct anchorclass {
    char *name;			/* in utf8 */
    struct lookup_subtable *subtable;
    uint8_t type;		/* anchorclass_type */
    uint8_t has_base;
    uint8_t processed, has_mark, matches, ac_num;
    uint8_t ticked;
    struct anchorclass *next;
} AnchorClass;

enum anchor_type { at_mark, at_basechar, at_baselig, at_basemark, at_centry, at_cexit, at_max };
typedef struct anchorpoint {
    AnchorClass *anchor;
    BasePoint me;
    DeviceTable xadjust, yadjust;
    unsigned int type: 4;
    unsigned int selected: 1;
    unsigned int ticked: 1;
    unsigned int has_ttf_pt: 1;
    uint16_t ttf_pt_index;
    int16_t  lig_index;
    struct anchorpoint *next;
} AnchorPoint;

typedef struct kernpair {
    // Note that the left character in the pair has the reference to the kerning pair, which in turn references the right character.
    struct lookup_subtable *subtable;
    struct splinechar *sc;
    int16_t off;
    uint16_t kcid;			/* temporary value */
    DeviceTable *adjust;		/* Only adjustment in one dimen, if more needed use pst */
    struct kernpair *next;
} KernPair;

#define FF_KERNCLASS_FLAG_NATIVE 2 // If set, the class goes into groups.plist or kerning.plist.
#define FF_KERNCLASS_FLAG_FEATURE 4 // If set, the class or rule goes into the feature file. In the present configuration, this ought to be zero always.
#define FF_KERNCLASS_FLAG_NAMETYPE 8 // If unset (default), the class has a standard name, which translates to a U. F. O. name starting in public.kern, which may be illegal in the feature file. If set, it has a name like @MMK_.
#define FF_KERNCLASS_FLAG_NAMELEGACY 16 // If set, the class has a U. F. O. name starting in @kc as FontForge liked to do in the past.
#define FF_KERNCLASS_FLAG_VIRTUAL 32 // If unset (default), the class is a real character class and does not conflict with same-sided classes. If set, FontForge mostly ignores the class except for U. F. O. input/output.
#define FF_KERNCLASS_FLAG_FLATTEN 64 // If unset (default), the class gets exported as a class. If set, it gets exported as its first member (in order to support class-character kerns).
#define FF_KERNCLASS_FLAG_SINGLECHAR (FF_KERNCLASS_FLAG_VIRTUAL | FF_KERNCLASS_FLAG_FLATTEN) // We expect to see these used together.

typedef struct kernclass {
    int first_cnt, second_cnt;		/* Count of classes for first and second chars */
    char **firsts;			/* list of a space separated list of char names */
    char **seconds;			/*  one entry for each class. Entry 0 is null */
    					/*  and means everything not specified elsewhere */
    char **firsts_names; // We need to track the names of the classes in order to round-trip U. F. O. data.
    char **seconds_names;
    int *firsts_flags; // This tracks the storage format of the class in U. F. O. (groups.plist or features.fea) and whether it's a single-character class.
    int *seconds_flags; // We also track the name format (@MMK or public.kern).
    struct lookup_subtable *subtable;
    uint16_t kcid;			/* Temporary value, used for many things briefly */
    int16_t *offsets;			/* array of first_cnt*second_cnt entries with 0 representing no data */
    int *offsets_flags;
    DeviceTable *adjusts;		/* array of first_cnt*second_cnt entries representing resolution-specific adjustments */
    struct kernclass *next;		// Note that, in most cases, a typeface needs only one struct kernclass since it can contain all classes.
    int feature; // This indicates whether the kerning class came from a feature file. This is important during export.
} KernClass;

typedef struct generic_pst {
    unsigned int ticked: 1;
    unsigned int temporary: 1;		/* Used in afm ligature closure */
    /* enum possub_type*/ uint8_t type;
    struct lookup_subtable *subtable;
    struct generic_pst *next;
    union {
	struct vr pos;
	struct { char *paired; struct vr *vr; } pair;
	struct { char *variant; } subs;
	struct { char *components; } mult, alt;
	struct { char *components; struct splinechar *lig; } lig;
	struct { int16_t *carets; int cnt; } lcaret;	/* Ligature caret positions */
    } u;
} PST;

typedef struct liglist {
    PST *lig;
    struct splinechar *first;		/* First component */
    struct splinecharlist *components;	/* Other than the first */
    struct liglist *next;
    int ccnt;				/* Component count. (includes first component) */
} LigList;

enum fpossub_format { pst_glyphs, pst_class, pst_coverage,
		    pst_reversecoverage, pst_formatmax };

struct seqlookup {
    int seq;
    struct otlookup *lookup;
};

struct fpg { char *names, *back, *fore; };
struct fpc { int ncnt, bcnt, fcnt; uint16_t *nclasses, *bclasses, *fclasses, *allclasses; };
struct fpv { int ncnt, bcnt, fcnt; char **ncovers, **bcovers, **fcovers; };
struct fpr { int always1, bcnt, fcnt; char **ncovers, **bcovers, **fcovers; char *replacements; };

struct fpst_rule {
    union {
	/* Note: Items in backtrack area are in reverse order because that's how the OT wants them */
	/*  they need to be reversed again to be displayed to the user */
	struct fpg glyph;
	struct fpc fpc_class;
	struct fpv coverage;
	struct fpr rcoverage;
    } u;
    int lookup_cnt;
    struct seqlookup *lookups;
};

typedef struct generic_fpst {
    uint16_t /*enum possub_type*/ type;
    uint16_t /*enum fpossub_format*/ format;
    struct lookup_subtable *subtable;
    struct generic_fpst *next;
    uint16_t nccnt, bccnt, fccnt;
    uint16_t rule_cnt;
    char **nclass, **bclass, **fclass;
    struct fpst_rule *rules;
    uint8_t ticked;
    uint8_t effectively_by_glyphs;
    char **nclassnames, **bclassnames, **fclassnames;
} FPST;

enum asm_type { asm_indic, asm_context, asm_lig, asm_simple=4, asm_insert,
	asm_kern=0x11 };
enum asm_flags { asm_vert=0x8000, asm_descending=0x4000, asm_always=0x2000 };

struct asm_state {
    uint16_t next_state;
    uint16_t flags;
    union {
	struct {
	    struct otlookup *mark_lookup;	/* for contextual glyph subs (tag of a nested lookup) */
	    struct otlookup *cur_lookup;	/* for contextual glyph subs */
	} context;
	struct {
	    char *mark_ins;
	    char *cur_ins;
	} insert;
	struct {
	    int16_t *kerns;
	    int kcnt;
	} kern;
    } u;
};

typedef struct generic_asm {		/* Apple State Machine */
    struct generic_asm *next;
    uint16_t /*enum asm_type*/ type;
    struct lookup_subtable *subtable;	/* Lookup contains feature setting info */
    uint16_t flags;	/* 0x8000=>vert, 0x4000=>r2l, 0x2000=>hor&vert */
    uint8_t ticked;

    uint16_t class_cnt, state_cnt;
    char **classes;
    struct asm_state *state;
} ASM;
/* State Flags:
 Indic:
	0x8000	mark current glyph as first in rearrangement
	0x4000	don't advance to next glyph
	0x2000	mark current glyph as last
	0x000f	verb
		0 = no change		8 = AxCD => CDxA
		1 = Ax => xA		9 = AxCD => DCxA
		2 = xD => Dx		a = ABxD => DxAB
		3 = AxD => DxA		b = ABxD => DxBA
		4 = ABx => xAB		c = ABxCD => CDxAB
		5 = ABx => xBA		d = ABxCD => CDxBA
		6 = xCD => CDx		e = ABxCD => DCxAB
		7 = xCD => DCx		f = ABxCD => DCxBA
 Contextual:
	0x8000	mark current glyph
	0x4000	don't advance to next glyph
 Insert:
	0x8000	mark current glyph
	0x4000	don't advance to next glyph
	0x2000	current is Kashida like
	0x1000	mark is Kashida like
	0x0800	current insert before
	0x0400	mark insert before
	0x03e0	count of chars to be inserted at current (31 max)
	0x001f	count of chars to be inserted at mark (31 max)
 Kern:
	0x8000	add current glyph to kerning stack
	0x4000	don't advance to next glyph
	0x3fff	value offset
*/

struct jstf_prio {
    OTLookup **enableShrink;	/* Points to an array of lookups (GSUB or GPOS)*/
    OTLookup **disableShrink;	/* NULL terminated */
    OTLookup **maxShrink;	/* Array of GPOS like lookups */
    OTLookup **enableExtend;
    OTLookup **disableExtend;
    OTLookup **maxExtend;
};

struct jstf_lang {
    uint32_t lang;
    struct jstf_lang *next;
    int cnt;
    struct jstf_prio *prios;
};

typedef struct jstf_script {
    uint32_t script;
    struct jstf_script *next;
    char *extenders;		/* list of glyph names */
    struct jstf_lang *langs;
} Justify;

struct macname {
    struct macname *next;
    uint16_t enc;		/* Platform specific encoding. 0=>mac roman, 1=>sjis, 7=>russian */
    uint16_t lang;	/* Mac languages 0=>english, 1=>french, 2=>german */
    char *name;		/* Not a unicode string, uninterpreted mac encoded string */
};

/* Wow, the GPOS 'size' feature stores a string in the name table just as mac */
/*  features do */
/* And now (OTF 1.6) GSUB 'ss01'-'ss20' do too */
struct otfname {
    struct otfname *next;
    uint16_t lang;	/* windows language code */
    char *name;		/* utf8 */
};

enum otffn_field {
    otffn_featname, otffn_tooltiptext, otffn_sampletext,
    otffn_characters /* reserved */,
    otffn_paramname_begin /* this is the beginning of parameter block, so */
                          /* there should be no enum elements beyond this */
};
struct otffeatname {
    uint32_t tag;			/* Feature tag */
    struct otfname *names;
    struct otffeatname *next;
    enum otffn_field field;
    uint16_t nid;			/* temporary value */
};

struct macsetting {
    struct macsetting *next;
    uint16_t setting;
    uint16_t strid;
    struct macname *setname;
    unsigned int initially_enabled: 1;
};

typedef struct macfeat {
    struct macfeat *next;
    uint16_t feature;
    uint8_t ismutex;
    uint8_t default_setting;		/* Apple's docs say both that this is a byte and a short. It's a byte */
    uint16_t strid;			/* Temporary value, used when reading in */
    struct macname *featname;
    struct macsetting *settings;
} MacFeat;

enum sfundotype
{
    sfut_none=0,
    sfut_lookups,
    sfut_lookups_kerns,
    sfut_fontinfo,
    sfut_noop
};

/**
 * A spline font level undo stack. undoes are doubly linked using the
 * 'ln' member and carry some user presentable description of what the
 * undo relates to in 'msg'.
 *
 * The sfdchunk is a pointer to an SFD fragment which will apply the
 * undo to the current state. For example, it might contain
 * information about the old value of kerning pairs which can be used
 * to restore state to how it was. Note that the sfdchunk might only
 * be partial, containing only enough information to restore the state
 * which changed when the undo was created.
 */
typedef struct sfundoes {
    struct dlistnode ln;
    char* msg;
    enum sfundotype type;
    union {
	int dummy;
    } u;
    char* sfdchunk;
} SFUndoes;


typedef struct enc {
    char *enc_name;
    int char_cnt;	/* Size of the next two arrays */
    int32_t *unicode;	/* unicode value for each encoding point */
    char **psnames;	/* optional postscript name for each encoding point */
    struct enc *next;
    unsigned int builtin: 1;
    unsigned int hidden: 1;
    unsigned int only_1byte: 1;
    unsigned int has_1byte: 1;
    unsigned int has_2byte: 1;
    unsigned int is_unicodebmp: 1;
    unsigned int is_unicodefull: 1;
    unsigned int is_custom: 1;
    unsigned int is_original: 1;
    unsigned int is_compact: 1;
    unsigned int is_japanese: 1;
    unsigned int is_korean: 1;
    unsigned int is_tradchinese: 1;
    unsigned int is_simplechinese: 1;
    char iso_2022_escape[8];
    int iso_2022_escape_len;
    int low_page, high_page;
    char *iconv_name;	/* For compatibility to old versions we might use a different name from that used by iconv. */
    iconv_t tounicode;
    iconv_t fromunicode;
    int (*tounicode_func)(int);
    int (*fromunicode_func)(int);
    unsigned int is_temporary: 1;	/* freed when the map gets freed */
    int char_max;			/* Used by temporary encodings */
} Encoding;

struct renames { char *from; char *to; };

typedef struct namelist {
    struct namelist *basedon;
    char *title;
    const char ***unicode[17];
    struct namelist *next;
    struct renames *renames;
    int uses_unicode;
    char *a_utf8_name;
} NameList;

enum uni_interp { ui_unset= -1, ui_none, ui_adobe, ui_greek, ui_japanese,
	ui_trad_chinese, ui_simp_chinese, ui_korean, ui_ams };

struct remap { uint32_t firstenc, lastenc; int32_t infont; };

typedef struct encmap {		/* A per-font map of encoding to glyph id */
    int32_t *map;			/* Map from encoding to glyphid */
    int32_t *backmap;		/* Map from glyphid to encoding */
    int enccount;		/* used size of the map array */
    				/*  strictly speaking this might include */
			        /*  glyphs that are not encoded, but which */
			        /*  are displayed after the proper encoding */
    int encmax;			/* allocated size of the map array */
    int backmax;		/* allocated size of the backmap array */
    struct remap *remap;
    Encoding *enc;
    unsigned int ticked: 1;
} EncMap;

enum property_type { prt_string, prt_atom, prt_int, prt_uint, prt_property=0x10 };

typedef struct bdfprops {
    char *name;		/* These include both properties (like SLANT) and non-properties (like FONT) */
    int type;
    union {
	char *str;
	char *atom;
	int val;
    } u;
} BDFProperties;

typedef struct bdffont {
    struct splinefont *sf;
    int glyphcnt, glyphmax;	/* used & allocated sizes of glyphs array */
    BDFChar **glyphs;		/* an array of charcnt entries */
    int16_t pixelsize;
    int16_t ascent, descent;
    int16_t layer;		/* for piecemeal fonts */
    unsigned int piecemeal: 1;
    unsigned int bbsized: 1;
    unsigned int ticked: 1;
    unsigned int unhinted_freetype: 1;
    unsigned int recontext_freetype: 1;
    struct bdffont *next;
    struct clut *clut;
    char *foundry;
    int res;
    void *freetype_context;
    uint16_t truesize;		/* for bbsized fonts */
    int16_t prop_cnt;
    int16_t prop_max;		/* only used within bdfinfo dlg */
    BDFProperties *props;
    uint16_t ptsize, dpi;		/* for piecemeal fonts */
} BDFFont;

struct gv_part {
    char *component;
    unsigned int is_extender: 1;	/* This component may be skipped or repeated */
    uint16_t startConnectorLength;
    uint16_t endConnectorLength;
    uint16_t fullAdvance;
};

/* For the 'MATH' table (and for TeX) */
struct glyphvariants {
    char *variants;	/* Space separated list of glyph names */
/* Glyph assembly */
    int16_t italic_correction;	/* Of the composed glyph */
    DeviceTable *italic_adjusts;
    int part_cnt;
    struct gv_part *parts;
};

struct mathkerndata {
    int16_t height,kern;
    DeviceTable *height_adjusts;
    DeviceTable *kern_adjusts;
};

/* For the 'MATH' table */
struct mathkernvertex {
    int cnt;		/* There is one more kern entry than height entry */
	    /* So the last mkd should have its height ignored */
	    /* The MATH table stores the height count, I think the kern count */
	    /*  is more useful (and that's what I use here). They differ by 1 */
    struct mathkerndata *mkd;
};

struct mathkern {
    struct mathkernvertex top_right;
    struct mathkernvertex top_left;
    struct mathkernvertex bottom_right;
    struct mathkernvertex bottom_left;
};

enum privatedict_state {
    pds_odd        = 0x1,	/* Odd number of entries */
    pds_outoforder = 0x2,	/* Bluevalues should be listed in order */
    pds_toomany    = 0x4,	/* arrays are of limited sizes */
    pds_tooclose   = 0x8,	/* adjacent zones must not be within 2*bluefuzz+1 (or 3, if bluefuzz omitted) */
    pds_notintegral= 0x10,	/* Must be integers */
    pds_toobig     = 0x20,	/* within pair difference have some relation to BlueScale but the docs make no sense to me */
    pds_shift	   = 8,		/* BlueValues/OtherBlues, unshifted, FamilyBlues/FamilyOtherBlues shifted once */

    pds_missingblue  = 0x010000,
    pds_badbluefuzz  = 0x020000,
    pds_badbluescale = 0x040000,
    pds_badstdhw     = 0x080000,
    pds_badstdvw     = 0x100000,
    pds_badstemsnaph = 0x200000,
    pds_badstemsnapv = 0x400000,
    pds_stemsnapnostdh = 0x0800000,
    pds_stemsnapnostdv = 0x1000000,
    pds_badblueshift   = 0x2000000

};

enum validation_state { vs_unknown = 0,
	vs_known=0x01,				/* It has been validated */
	vs_opencontour=0x02,
	vs_selfintersects=0x04,
	vs_wrongdirection=0x08,
	vs_flippedreferences=0x10,		/* special case of wrong direction */
	vs_missingextrema=0x20,
	vs_missingglyphnameingsub=0x40,
	    /* Next few are postscript only */
	vs_toomanypoints=0x80,
	vs_toomanyhints=0x100,
	vs_badglyphname=0x200,
	    /* Next few are only for fontlint */
	    /* These are relative to maxp values which ff would fix on generating a font */
	vs_maxp_toomanypoints    =0x400,
	vs_maxp_toomanypaths     =0x800,
	vs_maxp_toomanycomppoints=0x1000,
	vs_maxp_toomanycomppaths =0x2000,
	vs_maxp_instrtoolong     =0x4000,
	vs_maxp_toomanyrefs      =0x8000,
	vs_maxp_refstoodeep      =0x10000,
	/* vs_maxp_prepfpgmtoolong=0x20000, */	/* I think I was wrong about this "error" */
	    /* Oops, we need another one, two, for the glyphs */
	vs_pointstoofarapart	= 0x40000,
	vs_nonintegral		= 0x80000,	/* This will never be interesting in a real font, but might be in an sfd file */
	vs_missinganchor	= 0x100000,
	vs_dupname		= 0x200000,
	vs_dupunicode		= 0x400000,
	vs_overlappedhints	= 0x800000,

	vs_last = vs_overlappedhints,
	vs_maskps = 0x3fe | vs_pointstoofarapart | vs_missinganchor | vs_dupname | vs_dupunicode | vs_overlappedhints,
	vs_maskcid = 0x1fe | vs_pointstoofarapart | vs_missinganchor | vs_dupname | vs_overlappedhints,
	vs_maskttf = 0x7e | vs_pointstoofarapart | vs_nonintegral | vs_missinganchor | vs_dupunicode,
	vs_maskfindproblems = 0x1be | vs_pointstoofarapart | vs_nonintegral | vs_missinganchor | vs_overlappedhints
	};

struct splinecharlist { struct splinechar *sc; struct splinecharlist *next;};

struct altuni { struct altuni *next; int32_t unienc, vs; uint32_t fid; };
	/* vs is the "variation selector" a unicode codepoint which modifieds */
	/*  the code point before it. If vs is -1 then unienc is just an */
	/*  alternate encoding (greek Alpha and latin A), but if vs is one */
	/*  of unicode's variation selectors then this glyph is somehow a */
	/*  variant shape. The specifics depend on the selector and script */
	/*  fid is currently unused, but may, someday, be used to do ttcs */
	/* NOTE: GlyphInfo displays vs==-1 as vs==0, and fixes things up */


#define TEX_UNDEF 0x7fff

enum ttfnames { ttf_copyright=0, ttf_family, ttf_subfamily, ttf_uniqueid,
    ttf_fullname, ttf_version, ttf_postscriptname, ttf_trademark,
    ttf_manufacturer, ttf_designer, ttf_descriptor, ttf_venderurl,
    ttf_designerurl, ttf_license, ttf_licenseurl, ttf_idontknow/*reserved*/,
    ttf_preffamilyname, ttf_prefmodifiers, ttf_compatfull, ttf_sampletext,
    ttf_cidfindfontname, ttf_wwsfamily, ttf_wwssubfamily, ttf_namemax };
struct ttflangname {
    int lang;
    char *names[ttf_namemax];			/* in utf8 */
    int frommac[(ttf_namemax+31)/32];		/* Used when parsing the 'name' table */
    struct ttflangname *next;
};

struct MATH {
/* From the MATH Constants subtable (constants for positioning glyphs. Not PI)*/
    int16_t ScriptPercentScaleDown;
    int16_t ScriptScriptPercentScaleDown;
    uint16_t DelimitedSubFormulaMinHeight;
    uint16_t DisplayOperatorMinHeight;
    int16_t MathLeading;
    DeviceTable *MathLeading_adjust;
    int16_t AxisHeight;
    DeviceTable *AxisHeight_adjust;
    int16_t AccentBaseHeight;
    DeviceTable *AccentBaseHeight_adjust;
    int16_t FlattenedAccentBaseHeight;
    DeviceTable *FlattenedAccentBaseHeight_adjust;
    int16_t SubscriptShiftDown;
    DeviceTable *SubscriptShiftDown_adjust;
    int16_t SubscriptTopMax;
    DeviceTable *SubscriptTopMax_adjust;
    int16_t SubscriptBaselineDropMin;
    DeviceTable *SubscriptBaselineDropMin_adjust;
    int16_t SuperscriptShiftUp;
    DeviceTable *SuperscriptShiftUp_adjust;
    int16_t SuperscriptShiftUpCramped;
    DeviceTable *SuperscriptShiftUpCramped_adjust;
    int16_t SuperscriptBottomMin;
    DeviceTable *SuperscriptBottomMin_adjust;
    int16_t SuperscriptBaselineDropMax;
    DeviceTable *SuperscriptBaselineDropMax_adjust;
    int16_t SubSuperscriptGapMin;
    DeviceTable *SubSuperscriptGapMin_adjust;
    int16_t SuperscriptBottomMaxWithSubscript;
    DeviceTable *SuperscriptBottomMaxWithSubscript_adjust;
    int16_t SpaceAfterScript;
    DeviceTable *SpaceAfterScript_adjust;
    int16_t UpperLimitGapMin;
    DeviceTable *UpperLimitGapMin_adjust;
    int16_t UpperLimitBaselineRiseMin;
    DeviceTable *UpperLimitBaselineRiseMin_adjust;
    int16_t LowerLimitGapMin;
    DeviceTable *LowerLimitGapMin_adjust;
    int16_t LowerLimitBaselineDropMin;
    DeviceTable *LowerLimitBaselineDropMin_adjust;
    int16_t StackTopShiftUp;
    DeviceTable *StackTopShiftUp_adjust;
    int16_t StackTopDisplayStyleShiftUp;
    DeviceTable *StackTopDisplayStyleShiftUp_adjust;
    int16_t StackBottomShiftDown;
    DeviceTable *StackBottomShiftDown_adjust;
    int16_t StackBottomDisplayStyleShiftDown;
    DeviceTable *StackBottomDisplayStyleShiftDown_adjust;
    int16_t StackGapMin;
    DeviceTable *StackGapMin_adjust;
    int16_t StackDisplayStyleGapMin;
    DeviceTable *StackDisplayStyleGapMin_adjust;
    int16_t StretchStackTopShiftUp;
    DeviceTable *StretchStackTopShiftUp_adjust;
    int16_t StretchStackBottomShiftDown;
    DeviceTable *StretchStackBottomShiftDown_adjust;
    int16_t StretchStackGapAboveMin;
    DeviceTable *StretchStackGapAboveMin_adjust;
    int16_t StretchStackGapBelowMin;
    DeviceTable *StretchStackGapBelowMin_adjust;
    int16_t FractionNumeratorShiftUp;
    DeviceTable *FractionNumeratorShiftUp_adjust;
    int16_t FractionNumeratorDisplayStyleShiftUp;
    DeviceTable *FractionNumeratorDisplayStyleShiftUp_adjust;
    int16_t FractionDenominatorShiftDown;
    DeviceTable *FractionDenominatorShiftDown_adjust;
    int16_t FractionDenominatorDisplayStyleShiftDown;
    DeviceTable *FractionDenominatorDisplayStyleShiftDown_adjust;
    int16_t FractionNumeratorGapMin;
    DeviceTable *FractionNumeratorGapMin_adjust;
    int16_t FractionNumeratorDisplayStyleGapMin;
    DeviceTable *FractionNumeratorDisplayStyleGapMin_adjust;
    int16_t FractionRuleThickness;
    DeviceTable *FractionRuleThickness_adjust;
    int16_t FractionDenominatorGapMin;
    DeviceTable *FractionDenominatorGapMin_adjust;
    int16_t FractionDenominatorDisplayStyleGapMin;
    DeviceTable *FractionDenominatorDisplayStyleGapMin_adjust;
    int16_t SkewedFractionHorizontalGap;
    DeviceTable *SkewedFractionHorizontalGap_adjust;
    int16_t SkewedFractionVerticalGap;
    DeviceTable *SkewedFractionVerticalGap_adjust;
    int16_t OverbarVerticalGap;
    DeviceTable *OverbarVerticalGap_adjust;
    int16_t OverbarRuleThickness;
    DeviceTable *OverbarRuleThickness_adjust;
    int16_t OverbarExtraAscender;
    DeviceTable *OverbarExtraAscender_adjust;
    int16_t UnderbarVerticalGap;
    DeviceTable *UnderbarVerticalGap_adjust;
    int16_t UnderbarRuleThickness;
    DeviceTable *UnderbarRuleThickness_adjust;
    int16_t UnderbarExtraDescender;
    DeviceTable *UnderbarExtraDescender_adjust;
    int16_t RadicalVerticalGap;
    DeviceTable *RadicalVerticalGap_adjust;
    int16_t RadicalDisplayStyleVerticalGap;
    DeviceTable *RadicalDisplayStyleVerticalGap_adjust;
    int16_t RadicalRuleThickness;
    DeviceTable *RadicalRuleThickness_adjust;
    int16_t RadicalExtraAscender;
    DeviceTable *RadicalExtraAscender_adjust;
    int16_t RadicalKernBeforeDegree;
    DeviceTable *RadicalKernBeforeDegree_adjust;
    int16_t RadicalKernAfterDegree;
    DeviceTable *RadicalKernAfterDegree_adjust;
    uint16_t RadicalDegreeBottomRaisePercent;
/* Global constants from other subtables */
    uint16_t MinConnectorOverlap;			/* in the math variants sub-table */
};

enum backedup_state { bs_dontknow=0, bs_not=1, bs_backedup=2 };
enum loadvalidation_state {
	lvs_bad_ps_fontname    = 0x001,
	lvs_bad_glyph_table    = 0x002,
	lvs_bad_cff_table      = 0x004,
	lvs_bad_metrics_table  = 0x008,
	lvs_bad_cmap_table     = 0x010,
	lvs_bad_bitmaps_table  = 0x020,
	lvs_bad_gx_table       = 0x040,
	lvs_bad_ot_table       = 0x080,
	lvs_bad_os2_version    = 0x100,
	lvs_bad_sfnt_header    = 0x200
    };

typedef struct layerinfo {
    char *name;
    unsigned int background: 1;			/* Layer is to be treated as background: No width, images, not worth outputting */
    unsigned int order2: 1;			/* Layer's data are order 2 bezier splines (truetype) rather than order 3 (postscript) */
						/* In all glyphs in the font */
    unsigned int ticked: 1;
    char * ufo_path;
} LayerInfo;

/* Baseline data from the 'BASE' table */
struct baselangextent {
    uint32_t lang;		/* also used for feature tag */
    struct baselangextent *next;
    int16_t ascent, descent;
    struct baselangextent *features;
};

struct basescript {
    uint32_t script;
    struct basescript *next;
    int    def_baseline;	/* index [0-baseline_cnt) */
    int16_t *baseline_pos;	/* baseline_cnt of these */
    struct baselangextent *langs;	/* Language specific extents (may be NULL) */
				/* The default one has the tag DEFAULT_LANG */
};

struct Base {
    int baseline_cnt;
    uint32_t *baseline_tags;
    /* A font does not need to provide info on all baselines, but if one script */
    /*  talks about a baseline, then all must. So the set of baselines is global*/
    struct basescript *scripts;
};

struct pfminfo {		/* A misnomer now. OS/2 info would be more accurate, but that's stuff in here from all over ttf files */
    unsigned int pfmset: 1;
    unsigned int winascent_add: 1;
    unsigned int windescent_add: 1;
    unsigned int hheadascent_add: 1;
    unsigned int hheaddescent_add: 1;
    unsigned int typoascent_add: 1;
    unsigned int typodescent_add: 1;
    unsigned int subsuper_set: 1;
    unsigned int panose_set: 1;
    unsigned int hheadset: 1;
    unsigned int vheadset: 1;
    unsigned int hascodepages: 1;
    unsigned int hasunicoderanges: 1;
    unsigned char pfmfamily;
    int16_t weight;
    int16_t width;
    char panose[10];
    /* A subset of OS/2 fsSelection, used for style mapping. */
    /* Must agree with macStyle per otspec, takes precedence. */
    /* Can't use macStyle because it doesn't have a "regular" bit unlike the OS/2 component. */
    int16_t stylemap;
    int16_t fstype;
    int16_t linegap;		/* from hhea */
    int16_t vlinegap;		/* from vhea */
    int16_t hhead_ascent, hhead_descent;
    int16_t os2_typoascent, os2_typodescent, os2_typolinegap;
    int16_t os2_winascent, os2_windescent;
    int16_t os2_subxsize, os2_subysize, os2_subxoff, os2_subyoff;
    int16_t os2_supxsize, os2_supysize, os2_supxoff, os2_supyoff;
    int16_t os2_strikeysize, os2_strikeypos;
    int16_t os2_capheight, os2_xheight;
    char os2_vendor[4];
    int16_t os2_family_class;
    uint32_t codepages[2];
    uint32_t unicoderanges[4];
};

struct ttf_table {
    uint32_t tag;
    uint32_t len, maxlen;
    uint8_t *data;
    struct ttf_table *next;
    FILE *temp;	/* Temporary storage used during generation */
};

enum texdata_type { tex_unset, tex_text, tex_math, tex_mathext };

struct texdata {
    enum texdata_type type;
    int32_t params[22];		/* param[6] has different meanings in normal and math fonts */
};

struct gasp {
    uint16_t ppem;
    uint16_t flags;
};

struct ff_glyphclasses {
    // This matches struct glyphclasses from featurefile.c for now. We may make the references numeric in the future.
    // There may be a matching entry as a class elsewhere. For now, the output driver is responsible for eliminating duplicates.
    // In the interest of preserving orderings, we shall output from here, checking for value overrides from kerning classes on each kerning group entry.
    char *classname, *glyphs;
    struct ff_glyphclasses *next;
};

struct ff_rawoffsets {
    // This stores raw offsets as read from kerning.plist.
    // FontForge shall output these after native data and shall output only those for which it has not emitted native data.
    char *left;
    char *right;
    int offset;
    struct ff_rawoffsets *next;
};

typedef struct splinefont {
    char *fontname, *fullname, *familyname, *weight;
    char *familyname_with_timestamp;
    char *copyright;
    char *filename;				/* sfd name. NULL if we open a font, that's origname */
    char *defbasefilename;
    char *version;
    real italicangle, upos, uwidth;		/* In font info */
    int ascent, descent, invalidem; // If invalidem, then we use the format-specific ascent and descent on export.
    int uniqueid;				/* Not copied when reading in!!!! */
    int glyphcnt, glyphmax;			/* allocated size of glyphs array */
    SplineChar **glyphs;
    unsigned int changed: 1;
    unsigned int changed_since_autosave: 1;
    unsigned int changed_since_xuidchanged: 1;
    unsigned int display_antialias: 1;
    unsigned int display_bbsized: 1;
    unsigned int dotlesswarn: 1;		/* User warned that font doesn't have a dotless i character */
    unsigned int onlybitmaps: 1;		/* it's a bdf editor, not a postscript editor */
    unsigned int serifcheck: 1;			/* Have we checked to see if we have serifs? */
    unsigned int issans: 1;			/* We have no serifs */
    unsigned int isserif: 1;			/* We have serifs. If neither set then we don't know. */
    unsigned int hasvmetrics: 1;		/* We've got vertical metric data and should output vhea/vmtx/VORG tables */
    unsigned int loading_cid_map: 1;
    unsigned int dupnamewarn: 1;		/* Warn about duplicate names when loading bdf font */
    unsigned int encodingchanged: 1;		/* Font's encoding has changed since it was loaded */
    unsigned int multilayer: 1;			/* only applies if TYPE3 is set, means this font can contain strokes & fills */
						/*  I leave it in so as to avoid cluttering up code with #ifdefs */
    unsigned int strokedfont: 1;
    unsigned int isnew: 1;			/* A new and unsaved font */
    unsigned int compacted: 1;			/* only used when opening a font */
    unsigned int backedup: 2;			/* 0=>don't know, 1=>no, 2=>yes */
    unsigned int use_typo_metrics: 1;		/* The standard says to. But MS */
    						/* seems to feel that isn't good */
			                        /* enough and has created a bit */
			                        /* to mean "really use them" */
    unsigned int weight_width_slope_only: 1;	/* This bit seems stupid to me */
    unsigned int save_to_dir: 1;		/* Loaded from an sfdir collection rather than a simple sfd file */
    unsigned int head_optimized_for_cleartype: 1;/* Bit in the 'head' flags field, if unset "East Asian fonts in the Windows Presentation Framework (Avalon) will not be hinted" */
    unsigned int ticked: 1;
    unsigned int internal_temp: 1;		/* Internal temporary font to be passed to freetype for rasterizing. Don't complain about oddities. Don't generate GPOS/GSUB tables, etc. */
    unsigned int complained_about_spiros: 1;
    unsigned int use_xuid: 1;			/* Adobe has deprecated these two */
    unsigned int use_uniqueid: 1;		/* fields. Mostly we don't want to use them */
	/* 2 bits left */
    struct fontviewbase *fv;
    struct metricsview *metrics;
    enum uni_interp uni_interp;
    NameList *for_new_glyphs;
    EncMap *map;		/* only used when opening a font to provide original default encoding */
    Layer grid;
    BDFFont *bitmaps;
    char *origname;		/* filename of font file (ie. if not an sfd) */
    char *autosavename;
    int display_size;		/* a val <0 => Generate our own images from splines, a value >0 => find a bdf font of that size */
    struct psdict *private_dict;	/* read in from type1 file or provided by user */
    char *xuid;
    struct pfminfo pfminfo;
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
    char *comments;	/* Used to be restricted to ASCII, now utf8 */
    char *fontlog;
    int tempuniqueid;
    int top_enc;
    uint16_t desired_row_cnt, desired_col_cnt;
    struct glyphnamehash *glyphnames;
    struct ttf_table *ttf_tables, *ttf_tab_saved;
	/* We copy: fpgm, prep, cvt, maxp (into ttf_tables) user can ask for others, into saved*/
    char **cvt_names;
    /* The end of this array is marked by a special entry: */
#define END_CVT_NAMES ((char *) (~(intptr_t) 0))
    struct instrdata *instr_dlgs;	/* Pointer to all table and character instruction dlgs in this font */
    struct shortview *cvt_dlg;
    struct kernclasslistdlg *kcld, *vkcld;
    struct kernclassdlg *kcd;
    struct texdata texdata;
    OTLookup *gsub_lookups, *gpos_lookups;
    /* Apple morx subtables become gsub, and kern subtables become gpos */
    AnchorClass *anchor;
    KernClass *kerns, *vkerns;
    FPST *possub;
    ASM *sm;				/* asm is a keyword */
    MacFeat *features;
    char *chosenname;			/* Set for files with multiple fonts in them */
    struct mmset *mm;			/* If part of a multiple master set */
    int16_t macstyle;
    char *fondname;			/* For use in generating mac families */
    /* from the GPOS 'size' feature. design_size, etc. are measured in tenths of a point */
    /*  bottom is exclusive, top is inclusive */
    /*  if any field is 0, it is undefined. All may be undefined, All may be */
    /*  defined, or design_size may be defined without any of the others */
    /*  but we can't define the range without defining the other junk */
    /*  Name must contain an English language name, may contain others */
    uint16_t design_size;
    uint16_t fontstyle_id;
    struct otfname *fontstyle_name;
    uint16_t design_range_bottom, design_range_top;
    struct otffeatname *feat_names;
    real strokewidth;
/* For GDEF Mark Attachment Class -- used in lookup flags */
/* As usual, class 0 is unused */
    int mark_class_cnt;
    char **mark_classes;		/* glyph name list */
    char **mark_class_names;		/* used within ff, utf8 (the name we've given to this class of marks) */
/* For GDEF Mark Attachment Sets -- used in lookup flags */
/* but here, set 0 is meaningful, since pst_usemarkfilteringset tells us */
    int mark_set_cnt;
    char **mark_sets;			/* glyph name list */
    char **mark_set_names;		/* used within ff, utf8 (the name we've given to this class of marks) */
    struct ff_glyphclasses *groups; // This stores arbitrary named character lists for use in kerning or in the feature file.
    struct ff_rawoffsets *groupkerns;
    struct ff_rawoffsets *groupvkerns;
    long long creationtime;		/* seconds since 1970 */
    long long modificationtime;
    short os2_version;			/* 0 means default rather than the real version 0 */
    short compression;			/* If we opened a compressed sfd file, then save it out compressed too */
    short gasp_version;			/* 0/1 currently */
    short gasp_cnt;
    struct gasp *gasp;
    struct MATH *MATH;
    float sfd_version;			/* Used only when reading in an sfd file */
    struct gfi_data *fontinfo;
    struct val_data *valwin;
#if !defined(_NO_PYTHON)
    void *python_temporary;
#endif
    void *python_persistent;		/* If python this will hold a python object, if not python this will hold a string containing a pickled object. We do nothing with it (if not python) except save it back out unchanged */
    int python_persistent_has_lists; // This affects whether arrays exist as tuples or as lists (thus allowing us to use tuples for foreign data).
    enum loadvalidation_state loadvalidation_state;
    LayerInfo *layers;
    int layer_cnt;
    int display_layer;
    struct Base *horiz_base, *vert_base;
    Justify *justify;
    int extrema_bound;			/* Splines do not count for extrema complaints when the distance between the endpoints is less than or equal to this */
    int width_separation;
    int sfntRevision;
#define sfntRevisionUnset	0x44445555
    int woffMajor;
#define woffUnset		0x4455
    int woffMinor;
    char *woffMetadata;
    real ufo_ascent, ufo_descent;	/* I don't know what these mean, they don't seem to correspond to any other ascent/descent pair, but retain them so round-trip ufo input/output leaves them unchanged */
	    /* ufo_descent is negative */
    struct sfundoes *undoes;
    int preferred_kerning; // 1 for U. F. O. native, 2 for feature file, 0 undefined. Input functions shall flag 2, I think. This is now in S. F. D. in order to round-trip U. F. O. consistently.
} SplineFont;

struct axismap {
    int points;	/* size of the next two arrays */
    real *blends;	/* between [0,1] ordered so that blend[0]<blend[1]<... */
    real *designs;	/* between the design ranges for this axis, typically [1,999] or [6,72] */
    real min, def, max;		/* For mac */
    struct macname *axisnames;	/* For mac */
};

struct named_instance {	/* For mac */
    real *coords;	/* array[axis], these are in user units */
    struct macname *names;
};

/* I am going to simplify my life and not encourage intermediate designs */
/*  this means I can easily calculate ConvertDesignVector, and don't have */
/*  to bother the user with specifying it. */
/* (NormalizeDesignVector is fairly basic and shouldn't need user help ever) */
/*  (As long as they want piecewise linear) */
/* I'm not going to support intermediate designs at all for apple var tables */
typedef struct mmset {
    int axis_count;
    char *axes[4];
    int instance_count;
    SplineFont **instances;
    SplineFont *normal;
    real *positions;	/* array[instance][axis] saying where each instance lies on each axis */
    real *defweights;	/* array[instance] saying how much of each instance makes the normal font */
			/* for adobe */
    struct axismap *axismaps;	/* array[axis] */
    char *cdv, *ndv;	/* for adobe */
    int named_instance_count;
    struct named_instance *named_instances;
    unsigned int changed: 1;
    unsigned int apple: 1;
} MMSet;

/* mac styles. Useful idea we'll just steal it */
enum style_flags { sf_bold = 1, sf_italic = 2, sf_underline = 4, sf_outline = 8,
	sf_shadow = 0x10, sf_condense = 0x20, sf_extend = 0x40 };

struct sflist {
    SplineFont *sf;
    int32_t *sizes;
    FILE *tempttf;		/* For ttf */
    int id;			/* For ttf */
    int* ids;			/* One for each size */
    BDFFont **bdfs;		/* Ditto */
    EncMap *map;
    struct sflist *next;
    char **former_names;
    int len;
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

enum archive_list_style { ars_tar, ars_zip };

struct archivers {
    char *ext, *unarchive, *archive, *listargs, *extractargs, *appendargs;
    enum archive_list_style ars;
};
#define ARCHIVERS_EMPTY { NULL, NULL, NULL, NULL, NULL, NULL, 0 }

struct fontdict;
struct pschars;
struct findsel;
struct charprocs;
struct enc;

#define chunkalloc(size)	calloc(1,size)
#define chunkfree(item,size)	free(item)

extern char *strconcat(const char *str, const char *str2);

extern void SFApplyFeatureFile(SplineFont *sf,FILE *file,char *filename,bool ignore_invalid_replacement);
extern struct pschars *SplineFont2ChrsSubrs(SplineFont *sf, int iscjk,
	struct pschars *subrs,int flags,enum fontformat format,int layer);
struct cidbytes;
struct fd2data;
struct ttfinfo;
struct alltabs;

typedef struct growbuf {
    unsigned char *pt;
    unsigned char *base;
    unsigned char *end;
} GrowBuf;
extern void GrowBuffer(GrowBuf *gb);

struct glyphdata;
extern struct pschars *CID2ChrsSubrs(SplineFont *cidmaster,struct cidbytes *cidbytes,int flags,int layer);
extern const char *GetAuthor(void);
extern int WriteUFOFont(const char *fontname, SplineFont *sf, enum fontformat format, int flags, const EncMap *enc,int layer, int version);
extern int SLIContainsR2L(SplineFont *sf,int sli);
extern void SFFindNearTop(SplineFont *);
extern void SFRestoreNearTop(SplineFont *);
extern const char *_GetModifiers(const char *fontname, const char *familyname, const char *weight);
extern const char *SFGetModifiers(const SplineFont *sf);
extern const unichar_t *_uGetModifiers(const unichar_t *fontname, const unichar_t *familyname,
	const unichar_t *weight);
extern void ttfdumpbitmap(SplineFont *sf,struct alltabs *at,int32_t *sizes);
extern void SplineFontSetUnChanged(SplineFont *sf);

extern bool RealNear(real a,real b);


extern void UndoesFree(Undoes *undo);
extern void StemInfosFree(StemInfo *h);
extern void StemInfoFree(StemInfo *h);
extern void SCOrderAP(SplineChar *sc);
extern int AnchorClassesNextMerge(AnchorClass *ac);
extern void AnchorClassMerge(SplineFont *sf,AnchorClass *into,AnchorClass *from);
extern void SCInsertPST(SplineChar *sc,PST *new_);
extern void PSTFree(PST *lig);
extern uint16_t PSTDefaultFlags(enum possub_type type,SplineChar *sc );
extern StemInfo *StemInfoCopy(StemInfo *h);
extern void SPChangePointType(SplinePoint *sp, int pointtype);

struct lookup_cvt { OTLookup *from, *to; int old;};
struct sub_cvt { struct lookup_subtable *from, *to; int old;};
struct ac_cvt { AnchorClass *from, *to; int old;};

struct sfmergecontext {
    SplineFont *sf_from, *sf_to;
    int lcnt;
    struct lookup_cvt *lks;
    int scnt;
    struct sub_cvt *subs;
    int acnt;
    struct ac_cvt *acs;
    char *prefix;
    int preserveCrossFontKerning;
    int lmax;
};
extern void AltUniRemove(SplineChar *sc,int uni);
extern void AltUniAdd(SplineChar *sc,int uni);
extern void AltUniAdd_DontCheckDups(SplineChar *sc,int uni);
extern SplineChar *SplineCharCreate(int layer_cnt);
extern void SCAddRef(SplineChar *sc,SplineChar *rsc,int layer, real xoff, real yoff);
extern void SplineCharFree(SplineChar *sc);
extern void ScriptLangListFree(struct scriptlanglist *sl);

enum pconvert_flags {
	// Point selection (mutually exclusive)
	pconvert_flag_none = 0x01,
	pconvert_flag_all = 0x02,
	pconvert_flag_smooth = 0x04,
	pconvert_flag_incompat = 0x08,
	// Conversion modes (mutually exclusive)
	pconvert_flag_by_geom = 0x100,
	pconvert_flag_force_type = 0x200,
	pconvert_flag_downgrade = 0x400,
	pconvert_flag_check_compat = 0x0800,
	// Additional
	pconvert_flag_hvcurve = 0x4000
};

#if 1
// These relate to experimental support for U. F. O. groups.
#define GROUP_NAME_KERNING_UFO 1
#define GROUP_NAME_KERNING_FEATURE 2
#define GROUP_NAME_VERTICAL 4 // Otherwise horizontal.
#define GROUP_NAME_RIGHT 8 // Otherwise left (or above).
#endif // 1
extern void MMSetFree(MMSet *mm);
extern void SFRemoveUndoes(SplineFont *sf,uint8_t *selected,EncMap *map);
extern void SplineRefigure(Spline *spline);
extern void SPLCategorizePoints(SplinePointList *spl);
extern int _SPLCategorizePoints(SplinePointList *spl, int flags);
extern SplinePointList *SplinePointListCopy(const SplinePointList *base);
/* The order of the enum elements below doesn't make much sense, but it's done*/
/*  this way to preserve binary compatibility */
enum transformPointType { tpt_OnlySelected, tpt_AllPoints, tpt_OnlySelectedInterpCPs };
/*
 * As SplinePointListTransform() does a few things, this is a mask to selectively be
 * able to disable some of them.
 */
enum transformPointMask {
    tpmask_dontFixControlPoints = 1 << 1,
    tpmask_operateOnSelectedBCP = 1 << 2,
    tpmask_dontTrimValues = 1 << 3
};
extern SplinePointList *SplinePointListTransform(SplinePointList *base, real transform[6], enum transformPointType allpoints );
extern void SCReinstanciateRef(SplineChar *sc,SplineChar *rsc,int layer);
extern SplineChar *MakeDupRef(SplineChar *base, int local_enc, int uni_enc);
extern void BDFClut(BDFFont *bdf, int linear_scale);
struct xlfd_components {
    char foundry[80];
    char family[100];
    char weight[80];
    char slant[40];
    char setwidth[50];
    char add_style[50];
    int pixel_size;
    int point_size;
    int res_x;
    int res_y;
    char spacing[40];
    int avg_width;
    char cs_reg[80];		/* encoding */
    char cs_enc[80];		/* encoding version? */
    int char_cnt;
};
struct std_bdf_props {
    const char *name;
    int type;
    int defaultable;
};
#define STD_BDF_PROPS_EMPTY { NULL, 0, 0 }

extern int CubicSolve(const Spline1D *sp,bigreal sought,extended ts[3]);
/* Uses an algebraic solution */
extern extended SplineSolve(const Spline1D *sp, real tmin, real tmax, extended sought_y);
/* Tries to fixup rounding errors that crept in to the solution */
extern extended SplineSolveFixup(const Spline1D *sp, real tmin, real tmax, extended sought_y);
/* Uses an iterative approximation */
/* Uses an iterative approximation and then tries to fix things up */


#define CURVATURE_ERROR	INFINITY

extern bigreal SplineLength(Spline *spline);
extern int SplineIsLinear(Spline *spline);
extern void SFOrderBitmapList(SplineFont *sf);

extern Spline *SplineMake(SplinePoint *from, SplinePoint *to, int order2);
extern Spline *SFSplineMake(SplineFont *sf,SplinePoint *from, SplinePoint *to);


extern double BlueScaleFigure(struct psdict *private_,real bluevalues[], real otherblues[]);

typedef struct bluezone {
    real base;
    int cvtindex;
    real family_base;      /* NaN if none */
    int family_cvtindex;
    real overshoot;        /* relative to baseline, NOT to base */
    int highest;           /* used in autoinstructing for HStem positioning */
    int lowest;            /* as above */
} BlueZone;

typedef struct stdstem {
    real width;            /* -1 if none */
    int cvtindex;
    struct stdstem *snapto;/* NULL means stem isn't snapped to any other */
    int stopat;            /* at which ppem stop snapping to snapto */
} StdStem;

typedef struct globalinstrct {
    SplineFont *sf;
    int layer;
    BlueData *bd;
    double fudge;

    /* Did we initialize the tables needed? 'maxp' is skipped because */
    /* its initialization always succeeds. */
    int cvt_done;
    int fpgm_done;
    int prep_done;

    /* PS private data with truetype-specific information added */
    BlueZone blues[12];    /* like in BlueData */
    int      bluecnt;
    StdStem  stdhw;
    StdStem  *stemsnaph;   /* StdHW excluded */
    int      stemsnaphcnt;
    StdStem  stdvw;
    StdStem  *stemsnapv;   /* StdVW excluded */
    int      stemsnapvcnt;
} GlobalInstrCt;

extern void InitGlobalInstrCt( GlobalInstrCt *gic,SplineFont *sf,int layer,
	BlueData *bd );
extern void FreeGlobalInstrCt( GlobalInstrCt *gic );
extern void NowakowskiSCAutoInstr( GlobalInstrCt *gic,SplineChar *sc );
extern void CVT_ImportPrivate(SplineFont *sf);

extern void SplineFontAutoHint( SplineFont *sf, int layer);
extern int SCDrawsSomething(SplineChar *sc);
extern int SCSetMetaData(SplineChar *sc,const char *name,int unienc,
	const char *comment);

extern char* DumpSplineFontMetadata( SplineFont *sf );


enum ttfflags { ttf_onlystrikes=1, ttf_onlyonestrike=2, ttf_onlykerns=4, ttf_onlynames=8 };
extern SplineFont *SFReadUFO(char *filename,int flags);
extern SplineFont *LoadSplineFont(const char *filename,enum openflags);
extern SplineFont *_ReadSplineFont(FILE *file, const char *filename, enum openflags openflags);
extern SplineFont *ReadSplineFont(const char *filename,enum openflags);	/* Don't use this, use LoadSF instead */
extern void ArchiveCleanup(char *archivedir);
extern char *Unarchive(char *name, char **_archivedir);
extern char *Decompress(char *name);
extern uint16_t MacStyleCode( SplineFont *sf, uint16_t *psstyle );
extern char **NamesReadUFO(char *filename);
extern char *SFSubfontnameStart(char *fname);


extern const char *UnicodeRange(int unienc);
extern SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,EncMap *map,int i);
extern SplineChar *SFMakeChar(SplineFont *sf,EncMap *map,int i);
extern char *AdobeLigatureFormat(char *name);
extern uint32_t LigTagFromUnicode(int uni);
extern void SCLigCaretheck(SplineChar *sc,int clean);

extern void SCUndoSetLBearingChange(SplineChar *sc,int lb);



extern SplineSet *SplinePointListInterpretGlif(SplineFont *sf,char *filename,char *memory, int memlen, int em_size, int ascent,int stroked);
#define UNDEFINED_WIDTH	-999999
struct pscontext {
    int is_type2;
    int painttype;
    int instance_count;
    real blend_values[17];
    int blend_warn;
};

extern int NameToEncoding(SplineFont *sf,EncMap *map,const char *uname);
extern SplineChar *SFGetOrMakeChar(SplineFont *sf, int unienc, const char *name );
extern SplineChar *SFGetOrMakeCharFromUnicode( SplineFont *sf, EncMap *map, int ch );

extern int DoAutoRecovery(int);
typedef void (*DoAutoRecoveryPostRecoverFunc)(SplineFont *sf);

extern int SFPrivateGuess(SplineFont *sf,int layer, struct psdict *private_dict,
	char *name, int onlyone);

extern void SFRemoveLayer(SplineFont *sf,int l);
extern void SFAddLayer(SplineFont *sf,char *name,int order2, int background);
extern void SFLayerSetBackground(SplineFont *sf,int layer,int is_back);

extern void SplineSetsRound2Int(SplineSet *spl,real factor,int inspiro,int onlysel);
extern void SCRound2Int(SplineChar *sc,int layer, real factor);

extern void SFFlatten(SplineFont **cidmaster);

extern void SCCopyLayerToLayer(SplineChar *sc, int from, int to,int doclear);

extern int hasFreeType(void);
extern int hasFreeTypeDebugger(void);
extern int hasFreeTypeByteCode(void);
extern int FreeTypeAtLeast(int major, int minor, int patch);
extern char *FreeTypeStringVersion(void);
extern void doneFreeType(void);
extern void *_FreeTypeFontContext(SplineFont *sf,SplineChar *sc,struct fontviewbase *fv,
	int layer, enum fontformat ff,int flags,void *shared_ftc);
extern void *FreeTypeFontContext(SplineFont *sf,SplineChar *sc,struct fontviewbase *fv,int layer);
extern BDFFont *SplineFontFreeTypeRasterize(void *freetypecontext,int pixelsize,int depth);
extern BDFChar *SplineCharFreeTypeRasterize(void *freetypecontext,int gid,
	int ptsize, int dpi,int depth);
extern void FreeTypeFreeContext(void *freetypecontext);
extern SplineSet *FreeType_GridFitChar(void *single_glyph_context,
	int enc, real ptsizey, real ptsizex, int dpi, uint16_t *width,
	SplineChar *sc, int depth, int scaled);
extern struct freetype_raster *FreeType_GetRaster(void *single_glyph_context,
	int enc, real ptsizey, real ptsizex, int dpi,int depth);
extern BDFChar *SplineCharFreeTypeRasterizeNoHints(SplineChar *sc,int layer,
	int ptsize, int dpi,int depth);
extern BDFFont *SplineFontFreeTypeRasterizeNoHints(SplineFont *sf,int layer,
	int pixelsize,int depth);
extern void FreeType_FreeRaster(struct freetype_raster *raster);
struct TT_ExecContextRec_;
extern struct freetype_raster *DebuggerCurrentRaster(struct  TT_ExecContextRec_ *exc,int depth);


extern AnchorPos *AnchorPositioning(SplineChar *sc,unichar_t *ustr,SplineChar **sstr );

extern int  SF_CloseAllInstrs(SplineFont *sf);
extern int  SSTtfNumberPoints(SplineSet *ss);
extern int  SCNumberPoints(SplineChar *sc,int layer);
extern int  SCPointsNumberedProperly(SplineChar *sc,int layer);

extern int SFRenameTheseFeatureTags(SplineFont *sf, uint32_t tag, int sli, int flags,
	uint32_t totag, int tosli, int toflags, int ismac);
extern int SFRemoveUnusedNestedFeatures(SplineFont *sf);



extern struct macsetting *FindMacSetting(SplineFont *sf, int feat, int set,struct macsetting **secondary);



extern int BpColinear(BasePoint *first, BasePoint *mid, BasePoint *last);
extern int BpWithin(BasePoint *first, BasePoint *mid, BasePoint *last);
    /* Colinear & between */

# if HANYANG
extern void SFDDumpCompositionRules(FILE *sfd,struct compositionrules *rules);
extern struct compositionrules *SFDReadCompositionRules(FILE *sfd);
extern void SFModifyComposition(SplineFont *sf);
extern void SFBuildSyllables(SplineFont *sf);
# endif



extern void SFSetModTime(SplineFont *sf);



extern struct lookup_subtable *SFFindLookupSubtable(SplineFont *sf,const char *name);
extern int FeatureTagInFeatureScriptList(uint32_t tag, FeatureScriptLangList *fl);

extern void SplinePointRound(SplinePoint *,real);

extern KernClass *SFFindKernClass(SplineFont *sf,SplineChar *first,SplineChar *last,
	int *index,int allow_zero);
extern KernClass *SFFindVKernClass(SplineFont *sf,SplineChar *first,SplineChar *last,
	int *index,int allow_zero);

extern void SCClearRounds(SplineChar *sc,int layer);
extern void MDReplace(MinimumDistance *md,SplineSet *old,SplineSet *rpl);
extern void SCSynchronizeWidth(SplineChar *sc,real newwidth, real oldwidth,struct fontviewbase *fv);
extern RefChar *HasUseMyMetrics(SplineChar *sc,int layer);
extern void SCSynchronizeLBearing(SplineChar *sc,real off,int layer);
extern void RevertedGlyphReferenceFixup(SplineChar *sc, SplineFont *sf);

extern void SFUntickAll(SplineFont *sf);


extern int _ExportGlif(FILE *glif,SplineChar *sc,int layer,int version);

extern void SCCopyWidth(SplineChar *sc,enum undotype);
extern void SCClearBackground(SplineChar *sc);
extern void BackgroundImageTransform(SplineChar *sc, ImageList *img,real transform[6]);
extern int SFIsDuplicatable(SplineFont *sf, SplineChar *sc);

extern void SCClearLayer(SplineChar *sc,int layer);
extern void SCClearContents(SplineChar *sc,int layer);
extern void SCClearAll(SplineChar *sc,int layer);

#if !defined(_NO_PYTHON)
extern void FontForge_InitializeEmbeddedPython(void);
extern void FontForge_FinalizeEmbeddedPython(void);
extern void PyFF_ErrorString(const char *msg,const char *str);
extern void PyFF_ErrorF3(const char *frmt, const char *str, int size, int depth);
extern void PyFF_Stdin(int no_inits, int no_plugins);
extern void PyFF_Main(int argc,char **argv,int start, int no_init, int no_plugins);
extern void PyFF_ScriptFile(struct fontviewbase *fv,SplineChar *sc,char *filename);
extern void PyFF_ScriptString(struct fontviewbase *fv,SplineChar *sc,int layer,char *str);
extern void PyFF_FreeFV(struct fontviewbase *fv);
extern void PyFF_FreeSC(SplineChar *sc);
void PyFF_FreeSCLayer(SplineChar *sc, int layer);
extern void PyFF_FreeSF(SplineFont *sf);
extern void PyFF_FreePythonPersistent(void *python_persistent);
extern void PyFF_ProcessInitFiles(int no_inits, int no_plugins);
extern char *PyFF_PickleMeToString(void *pydata);
extern void *PyFF_UnPickleMeToObjects(char *str);
struct _object;		/* Python Object */
extern void PyFF_CallDictFunc(struct _object *dict,const char *key,const char *argtypes, ... );
#endif




extern struct math_constants_descriptor {
    char *ui_name;
    char *script_name;
    int offset;
    int devtab_offset;
    char *message;
    int new_page;
} math_constants_descriptor[];

#define MATH_CONSTANTS_DESCRIPTOR_EMPTY { NULL, NULL, 0, 0, NULL, 0 }

extern const char *knownweights[], *realweights[], **noticeweights[];

extern int BPTooFar(BasePoint *bp1, BasePoint *bp2);
extern StemInfo *SCHintOverlapInMask(SplineChar *sc,HintMask *hm);
extern char *VSErrorsFromMask(int mask,int private_mask);
extern int SCValidate(SplineChar *sc, int layer, int force);
extern AnchorClass *SCValidateAnchors(SplineChar *sc);
extern void SCTickValidationState(SplineChar *sc,int layer);
extern int ValidatePrivate(SplineFont *sf);
extern int SFValidate(SplineFont *sf, int layer, int force);
extern int VSMaskFromFormat(SplineFont *sf, int layer, enum fontformat format);

extern char *RandomParaFromScript(uint32_t script, uint32_t *lang, SplineFont *sf);

extern void PatternSCBounds(SplineChar *sc,DBounds *b);

extern char *SFDefaultImage(SplineFont *sf,char *filename);
extern void SCClearInstrsOrMark(SplineChar *sc, int layer, int complain);
extern void instrcheck(SplineChar *sc,int layer);
extern void TTFPointMatches(SplineChar *sc,int layer,int top);

extern bigreal SFCapHeight(SplineFont *sf, int layer, int return_error);
extern bigreal SFXHeight(SplineFont *sf, int layer, int return_error);
extern bigreal SFAscender(SplineFont *sf, int layer, int return_error);
extern bigreal SFDescender(SplineFont *sf, int layer, int return_error);

extern void SCRemoveKern(SplineChar* sc);
extern void SCRemoveVKern(SplineChar* sc);

/**
 * Return falise if the container does not contain "sought"
 * Return true if sought is in the container.
 */
extern int SplinePointListContains( SplinePointList* container, SplinePointList* sought );

/**
 * Return falise if the container does not contain the single splint point "sp",
 * Return true if "sp" is in the container.
 */
extern int SplinePointListContainsPoint( SplinePointList* container, SplinePoint* sp );

/**
 * Visitor for SPLFirstVisitSplines()
 */
typedef void (*SPLFirstVisitSplinesVisitor)( SplinePoint* splfirst, Spline* s, void* udata );

/**
 * Visitor Function: print debug information about each spline
 *
 * Visitor for SPLFirstVisit()
 */
extern void SPLFirstVisitorDebug(SplinePoint* splfirst, Spline* spline, void* udata );

/**
 * Visitor Function: print debug information about the current
 * selection state including the secondary BCP selection for each
 * spline
 *
 * Visitor for SPLFirstVisit()
 */
extern void SPLFirstVisitorDebugSelectionState(SplinePoint* splfirst, Spline* spline, void* udata );

/**
 * Given a SplinePointList* that you want to visit each spline in the
 * iteration is not as simple as it could be, so you can call this
 * function passing spl->first as 'splfirst' and a visitor function
 * which will see each spline in the splfirst colleciton.
 *
 * For debug, you can pass SPLFirstVisitorDebug which will print
 * information for each item in the splfirst collection.
 *
 * You can pass any arbitrary data in as udata and SPLFirstVisit()
 * will pass that udata to your visitor function without change. If
 * you want a return value from your visitor, pass a pointer to a
 * struct as udata. eg:
 *
 * typedef struct SPLFirstVisitorFoundSoughtDataS
 * {
 *    SplinePoint* sought;
 *    int found;
 * } SPLFirstVisitorFoundSoughtData;
 *
 * // ...
 *
 *	SPLFirstVisitorFoundSoughtData d;
 *	d.sought = sought;
 *	d.found  = 0;
 *	SPLFirstVisit( spl->first, SPLFirstVisitorFoundSought, &d );
 *	if( d.found )
 *           return 1;
 *
 */
extern void SPLFirstVisitSplines( SplinePoint* splfirst, SPLFirstVisitSplinesVisitor f, void* udata );

/**
 * Visitor for SPLFirstVisitPoints()
 */
typedef void (*SPLFirstVisitPointsVisitor)( SplinePoint* splfirst, Spline* s, SplinePoint* sp, void* udata );

/**
 * Visit all the SplinePoints on the spline starting at splfirst.
 */
extern void SPLFirstVisitPoints( SplinePoint* splfirst, SPLFirstVisitPointsVisitor f, void* udata );



/**
 * Applies a visitor to the container and returns false if no point in the SPL
 * has an x coordinate of 'x'.
 */
extern SplinePoint* SplinePointListContainsPointAtX( SplinePointList* container, real x );
extern SplinePoint* SplinePointListContainsPointAtY( SplinePointList* container, real y );
extern SplinePoint* SplinePointListContainsPointAtXY( SplinePointList* container, real x, real y );


/**
 * True if the spline with from/to is part of the guide splines.
 *
 * Handy for telling if the user has just clicked on a guide for example,
 * you might want to also check the active layer first with cv->b.drawmode == dm_grid
 */
extern bool isSplinePointPartOfGuide( SplineFont *sf, SplinePoint *sp );


extern void debug_printHint( StemInfo *h, char* msg );

#if defined(_WIN32) || defined(__HAIKU__)
#define BAD_LOCALE_HACK
typedef char* locale_t;
#define LC_GLOBAL_LOCALE ((locale_t)-1)
#define LC_ALL_MASK LC_ALL
#define LC_COLLATE_MASK LC_COLLATE
#define LC_CTYPE_MASK LC_CTYPE
#define LC_MONETARY_MASK LC_MONETARY
#define LC_NUMERIC_MASK LC_NUMERIC
#define LC_TIME_MASK LC_TIME
#endif

static inline void switch_to_c_locale(locale_t * tmplocale_p, locale_t * oldlocale_p) {
#ifndef BAD_LOCALE_HACK
  *tmplocale_p = newlocale(LC_NUMERIC_MASK, "C", NULL);
  if (*tmplocale_p == NULL) fprintf(stderr, "Failed to create temporary locale.\n");
  else if ((*oldlocale_p = uselocale(*tmplocale_p)) == NULL) {
    fprintf(stderr, "Failed to change locale.\n");
    freelocale(*tmplocale_p); *tmplocale_p = NULL;
  }
#else
  // Yes, it is dirty. But so is an operating system that doesn't support threaded locales.
  *oldlocale_p = (locale_t)copy(setlocale(LC_NUMERIC_MASK, "C"));
  if (*oldlocale_p == NULL) fprintf(stderr, "Failed to change locale.\n");
#endif
}

static inline void switch_to_old_locale(locale_t * tmplocale_p, locale_t * oldlocale_p) {
#ifndef BAD_LOCALE_HACK
  if (*oldlocale_p != NULL) { uselocale(*oldlocale_p); } else { uselocale(LC_GLOBAL_LOCALE); }
  *oldlocale_p = NULL; // This ends the lifecycle of the temporary old locale storage.
  if (*tmplocale_p != NULL) { freelocale(*tmplocale_p); *tmplocale_p = NULL; }
#else
  if (*oldlocale_p != NULL) {
    setlocale(LC_NUMERIC_MASK, (char*)(*oldlocale_p));
    free((char*)(*oldlocale_p));
    *oldlocale_p = NULL;
  }
#endif
}

static inline locale_t newlocale_hack(int category_mask, const char *locale, locale_t base) {
  // Note that, in the interest of minimizing the hack, we drop the category mask on Wingdows.
#ifndef BAD_LOCALE_HACK
  return newlocale(category_mask, locale, base);
#else
  return (locale_t)copy(locale);
#endif
}

static inline locale_t uselocale_hack(locale_t dataset) {
#ifndef BAD_LOCALE_HACK
  return uselocale(dataset);
#else
  return (locale_t)copy(setlocale(LC_ALL_MASK, (char*)dataset));
#endif
}

static inline void freelocale_hack(locale_t dataset) {
#ifndef BAD_LOCALE_HACK
  freelocale(dataset);
#else
  if (dataset != NULL) { free(dataset); }
#endif
}

#if 0
#define DECLARE_TEMP_LOCALE() char oldloc[25];
#define SWITCH_TO_C_LOCALE() strncpy( oldloc,setlocale(LC_NUMERIC,NULL),24 ); oldloc[24]='\0'; setlocale(LC_NUMERIC,"C");
#define SWITCH_TO_OLD_LOCALE() setlocale(LC_NUMERIC,oldloc);
#else
#define DECLARE_TEMP_LOCALE() locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
#define SWITCH_TO_C_LOCALE() switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
#define SWITCH_TO_OLD_LOCALE() switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
#endif

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_SPLINEFONT_H */
