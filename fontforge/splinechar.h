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
#pragma once

#include <stdint.h>

#include "basics.h"
#include "shapers/opentype_str.h"

#ifndef _NO_LIBSPIRO
#include <spiroentrypoints.h>
#else
#define SPIRO_OPEN_CONTOUR '{'
#define SPIRO_CORNER 'v'
#define SPIRO_G4 'o'
#define SPIRO_G2 'c'
#define SPIRO_LEFT '['
#define SPIRO_RIGHT ']'
#define SPIRO_END 'z'
typedef struct { /* Taken from spiro.h because I want */
    double x;    /*  to be able to compile for spiro */
    double y;    /*  even on a system without it */
    char ty;
} spiro_cp;
#endif

#ifdef FONTFORGE_CONFIG_USE_DOUBLE
#define real double
#define bigreal double
#else
#define real float
#define bigreal double
#endif

typedef struct basepoint {
    real x;
    real y;
} BasePoint;

#define BASEPOINT_EMPTY {(real)0.0, (real)0.0}

typedef struct dbounds {
    real minx, maxx;
    real miny, maxy;
} DBounds;

#define DBOUNDS_EMPTY {(real)0.0, (real)0.0, (real)0.0, (real)0.0}

typedef struct ipoint {
    int x;
    int y;
} IPoint;

#define IPOINT_EMPTY {0, 0}

enum spreadMethod { sm_pad, sm_reflect, sm_repeat };

#define COLOR_INHERITED 0xfffffffe

struct grad_stops {
    real offset;
    uint32_t col;
    real opacity;
};

struct gradient {
    BasePoint start; /* focal of a radial gradient, start of a linear */
    BasePoint stop;  /* center of a radial gradient, end of a linear */
    real radius;     /* 0=>linear gradient, else radius of a radial gradient */
    enum spreadMethod sm;
    int stop_cnt;
    struct grad_stops* grad_stops;
};

struct pattern {
    char* pattern;
    real width, height; /* Pattern is scaled to be repeated every width/height
                           (in user coordinates) */
    real transform[6];
    /* Used during rasterization process */
    struct bdfchar* pat;
    real invtrans[6];
    int bminx, bminy, bwidth, bheight; /* of the pattern at bdfchar scale */
};

struct brush {
    uint32_t col;
    float opacity;             /* number between [0,1], only for svg/pdf */
    struct pattern* pattern;   /* A pattern to be tiled */
    struct gradient* gradient; /* A gradient fill */
};
#define WIDTH_INHERITED (-1)
#define DASH_INHERITED \
    255 /* if the dashes[0]==0 && dashes[1]==DASH_INHERITED */
#define DASH_MAX 8
typedef unsigned char DashType;
struct pen {
    struct brush brush;
    uint8_t linejoin;
    uint8_t linecap;
    float width;
    real trans[4];
    DashType dashes[DASH_MAX];
};

#define HntMax 96 /* PS says at most 96 hints */
typedef uint8_t HintMask[HntMax / 8];

enum pointtype { pt_curve, pt_corner, pt_tangent, pt_hvcurve };
typedef struct splinepoint {
    BasePoint me;
    BasePoint nextcp; /* control point */
    BasePoint prevcp; /* control point */
    unsigned int nonextcp : 1;
    unsigned int noprevcp : 1;
    unsigned int nextcpdef : 1;
    unsigned int prevcpdef : 1;
    unsigned int selected : 1;       /* for UI */
    unsigned int nextcpselected : 2; /* Is the next BCP selected */
    unsigned int prevcpselected : 2; /* Is the prev BCP selected */
    unsigned int pointtype : 2;
    unsigned int isintersection : 1;
    unsigned int flexy
        : 1; /* When "freetype_markup" is on in charview.c:DrawPoint */
    unsigned int flexx : 1;  /* flexy means select nextcp, and flexx means draw
                                circle around nextcp */
    unsigned int roundx : 1; /* For true type hinting */
    unsigned int roundy : 1; /* For true type hinting */
    unsigned int dontinterpolate : 1; /* in ttf, don't imply point by
                                         interpolating between cps */
    unsigned int ticked : 1;
    unsigned int watched : 1;
    /* 1 bits left... */
    uint16_t ptindex;  /* Temporary value used by metafont routine */
    uint16_t ttfindex; /* Truetype point index */
    /* Special values 0xffff => point implied by averaging control points */
    /*		  0xfffe => point created with no real number yet */
    /* (or perhaps point in context where no number is possible as in a glyph
     * with points & refs) */
    uint16_t nextcpindex; /* Truetype point index */
    struct spline* next;
    struct spline* prev;
    HintMask* hintmask;
    char* name;
} SplinePoint;

typedef struct splinepointlist {
    SplinePoint *first, *last;
    struct splinepointlist* next;
    spiro_cp* spiros;
    uint16_t spiro_cnt, spiro_max;
    /* These could be bit fields, but bytes are easier to access and we */
    /*  don't need the space (yet) */
    uint8_t ticked;
    uint8_t beziers_need_optimizer; /* If the spiros have changed in spiro mode,
                                       then reverting to bezier mode might,
                                       someday, run a simplifier */
    uint8_t is_clip_path;           /* In type3/svg fonts */
    int start_offset;  // This indicates which point is the canonical first for
                       // purposes of outputting to U. F. O..
    char* contour_name;
} SplinePointList, SplineSet;

typedef struct imagelist {
    struct gimage* image;
    real xoff,
        yoff; /* position in character space of upper left corner of image */
    real xscale, yscale; /* scale to convert one pixel of image to one unit of
                            character space */
    DBounds bb;
    struct imagelist* next;
    unsigned int selected : 1;
} ImageList;

struct guidelineset;
typedef struct guidelineset {
    // This is a UFO construct. We implement it only for round-tripping.
    // The FontForge way is to have a global "grid".
    // If it is necessary to have per-character guides, one would use a
    // background layer. Having per glyph-layer guides is sort of ridiculous.
    // Life is much easier the FontForge way.
    char* name;
    char* identifier;  // Duplicative, but in the UFO specification.
    BasePoint point;
    real angle;
    uint32_t color;  // Red, green, blue, and alpha, 8 bits apiece.
    int flags;  // 0x20 means that the color is set. 0x10 means that the point
                // was imported clean (one numeric parameter).
    struct guidelineset* next;
} GuidelineSet;

struct reflayer {
    unsigned int background : 1;
    unsigned int order2 : 1;
    unsigned int anyflexes : 1;
    unsigned int dofill : 1;
    unsigned int dostroke : 1;
    unsigned int fillfirst : 1;
    struct brush fill_brush;
    struct pen stroke_pen;
    SplinePointList* splines;
    ImageList* images; /* Only in background or type3 layer(s) */
};

typedef struct refchar {
    unsigned int checked : 1;
    unsigned int selected : 1;
    unsigned int point_match : 1; /* match_pt* are point indexes */
                                  /*  and need to be converted to a */
                                  /*  translation after truetype readin */
    unsigned int encoded : 1; /* orig_pos is actually an encoded value, used for
                                 old sfd files */
    unsigned int justtranslated : 1; /* The transformation matrix specifies a
                                        translation (or is identity) */
    unsigned int use_my_metrics : 1; /* Retain the ttf "use_my_metrics" info. */
    /* important for glyphs with instructions which change the width used */
    /* inside composites */
    unsigned int round_translation_to_grid : 1; /* Retain the ttf
                                                   "round_to_grid" info. */
    unsigned int point_match_out_of_date
        : 1; /* Someone has edited a base glyph */
    int16_t adobe_enc;
    int orig_pos;
    int unicode_enc;   /* used by paste */
    real transform[6]; /* transformation matrix (first 2 rows of a 3x3 matrix,
                          missing row is 0,0,1) */
    struct reflayer* layers;
    int layer_cnt;
    struct refchar* next;
    DBounds bb;
    struct splinechar* sc;
    BasePoint top;
    uint16_t match_pt_base, match_pt_ref;
} RefChar;

typedef struct refbdfc {
    unsigned int checked : 1;
    unsigned int selected : 1;
    int8_t xoff;
    int8_t yoff;
    uint16_t gid;
    struct refbdfc* next;
    struct bdfchar* bdfc;
} BDFRefChar;

struct bdfcharlist {
    struct bdfchar* bc;
    struct bdfcharlist* next;
};

typedef struct bdffloat {
    int16_t xmin, xmax, ymin, ymax;
    int16_t bytes_per_line;
    unsigned int byte_data : 1;
    uint8_t depth;
    uint8_t* bitmap;
} BDFFloat;

typedef struct bdfchar {
    struct splinechar* sc;
    int16_t xmin, xmax, ymin, ymax;
    int16_t width;
    int16_t bytes_per_line;
    uint8_t* bitmap;
    struct refbdfc* refs;
    int orig_pos;
    int16_t pixelsize; /* for undoes */
    struct bitmapview* views;
    struct undoes* undoes;
    struct undoes* redoes;
    unsigned int changed : 1;
    unsigned int byte_data : 1; /* for anti-aliased chars entries are grey-scale
                                   bytes not bw bits */
    unsigned int widthgroup : 1;  /* for ttf bitmap output */
    unsigned int isreference : 1; /* for ttf bitmap input, */
    unsigned int ticked : 1;
    uint8_t depth; /* for ttf bitmap output */
    uint16_t vwidth;
    BDFFloat* selection;
    BDFFloat* backup;
    struct bdfcharlist* dependents;
} BDFChar;

enum undotype {
    ut_none = 0,
    ut_state,
    ut_tstate,
    ut_statehint,
    ut_statename,
    ut_statelookup,
    ut_anchors,
    ut_width,
    ut_vwidth,
    ut_lbearing,
    ut_rbearing,
    ut_possub,
    ut_hints,
    ut_bitmap,
    ut_bitmapsel,
    ut_composit,
    ut_multiple,
    ut_layers,
    ut_noop
};

#define UNDO_LAYER_UNKNOWN -1

enum possub_type {
    pst_null,
    pst_position,
    pst_pair,
    pst_substitution,
    pst_alternate,
    pst_multiple,
    pst_ligature,
    pst_lcaret /* must be pst_max-1, see charinfo.c*/,
    pst_max,
    /* These are not psts but are related so it's handly to have values for them
     */
    pst_kerning = pst_max,
    pst_vkerning,
    pst_anchors,
    /* And these are fpsts */
    pst_contextpos,
    pst_contextsub,
    pst_chainpos,
    pst_chainsub,
    pst_reversesub,
    fpst_max,
    /* And these are used to specify a kerning pair where the current */
    /*  char is the final glyph rather than the initial one */
    /* A kludge used when cutting and pasting features */
    pst_kernback,
    pst_vkernback
};

typedef struct undoes {
    struct undoes* next;
    enum undotype undotype;
    unsigned int was_modified : 1;
    unsigned int was_order2 : 1;
    int layer; /* the layer the undo is associated with or -1 if unknown */
    union {
        struct {
            int16_t width, vwidth;
            int16_t lbearingchange;
            int unicodeenc;             /* only for ut_statename */
            char* charname;             /* only for ut_statename */
            char* comment;              /* in utf8 */
            struct generic_pst* possub; /* only for ut_statename */
            struct splinepointlist* splines;
            struct refchar* refs;

            struct imagelist* images;
            void* hints; /* ut_statehint, ut_statename */
            uint8_t* instrs;
            int instrs_len;
            struct anchorpoint* anchor;
            struct brush fill_brush;
            struct pen stroke_pen;
            unsigned int dofill : 1;
            unsigned int dostroke : 1;
            unsigned int fillfirst : 1;
        } state;
        int width;    /* used by both ut_width and ut_vwidth */
        int lbearing; /* used by ut_lbearing */
        int rbearing; /* used by ut_rbearing */
        BDFChar bmpstate;
        struct { /* copy contains an outline state and a set of bitmap states */
            struct undoes* state;
            struct undoes* bitmaps;
        } composit;
        struct {
            struct undoes* mult; /* copy contains several sub copies (composits,
                                    or states or widths or...) */
            /* Also used for ut_layers, each sub copy is a state (first is
             * ly_fore, next ly_fore+1...) */
        } multiple;
        struct {
            enum possub_type pst;
            char** data; /* First 4 bytes is tag, then space then data */
            struct undoes* more_pst;
            short cnt, max; /* Not always set */
        } possub;
        uint8_t* bitmap;
    } u;
    struct splinefont* copied_from;
} Undoes;

typedef struct layer /* : reflayer */ {
    unsigned int background : 1;
    unsigned int order2 : 1;
    unsigned int anyflexes : 1;
    unsigned int dofill : 1;
    unsigned int dostroke : 1;
    unsigned int fillfirst : 1;
    struct brush fill_brush;
    struct pen stroke_pen;
    SplinePointList* splines;
    ImageList* images; /* Only in background or type3 layer(s) */
    RefChar* refs;     /* Only in foreground layer(s) */
    GuidelineSet*
        guidelines; /* Only in UFO imports, we hope. Inefficient otherwise. */
    Undoes* undoes;
    Undoes* redoes;
    uint32_t validation_state;
    uint32_t old_vs;
    void* python_persistent; /* If python this will hold a python object, if not
                                python this will hold a string containing a
                                pickled object. We do nothing with it (if not
                                python) except save it back out unchanged */
    int python_persistent_has_lists;
} Layer;

typedef struct linelist {
    IPoint here;
    struct linelist* next;
} LineList;

typedef struct linearapprox {
    real scale;
    unsigned int oneline : 1;
    unsigned int onepoint : 1;
    unsigned int any : 1; /* refers to a particular screen */
    struct linelist* lines;
    struct linearapprox* next;
} LinearApprox;

typedef struct spline1d {
    real a, b, c, d;
} Spline1D;

/**
 *
 * 2013Note: If you are altering from->me.x and y then you will
 *           probably have to modify splines[] to match your change.
 *           eg, moving both ends of a spline up/down by changing their
 *           to/from will also probably need an update to splines[ 0 | 1 ].d to
 *           match.
 */
typedef struct spline {
    unsigned int islinear : 1;    /* No control points */
    unsigned int isquadratic : 1; /* probably read in from ttf */
    unsigned int isticked : 1;
    unsigned int isneeded : 1;   /* Used in remove overlap */
    unsigned int isunneeded : 1; /* Used in remove overlap */
    unsigned int exclude : 1;    /* Used in remove overlap variant: exclude */
    unsigned int ishorvert : 1;
    unsigned int knowncurved : 1; /* We know that it curves */
    unsigned int knownlinear : 1; /* it might have control points, but still
                                     traces out a line */
    /* If neither knownlinear nor curved then we haven't checked */
    unsigned int order2 : 1; /* It's a bezier curve with only one cp */
    unsigned int touched : 1;
    unsigned int leftedge : 1;
    unsigned int rightedge : 1;
    unsigned int acceptableextrema
        : 1; /* This spline has extrema, but we don't care */
    SplinePoint* from;
    SplinePoint* to;
    Spline1D splines[2]; /* splines[0] is the x spline, splines[1] is y */
    struct linearapprox* approx;
    /* Possible optimizations:
        Precalculate bounding box
        Precalculate min/max/ points of inflection
    */
} Spline;

/* Some stems may appear, disappear, reapear several times */
/* Serif stems on I which appear at 0, disappear, reappear at top */
/* Or the major vertical stems on H which disappear at the cross bar */
typedef struct hintinstance {
    real begin; /* location in the non-major direction*/
    real end;   /* width/height in non-major direction*/
    unsigned int closed : 1;
    short int counternumber;
    struct hintinstance* next;
} HintInstance;

enum hinttypes { ht_unspecified = 0, ht_h, ht_v, ht_d };

#define MmMax                                                      \
    16 /* PS says at most this many instances for type1/2 mm fonts \
        */
#define AppleMmMax \
    26 /* Apple sort of has a limit of 4095, but we only support this many */
typedef real _MMArray[2][MmMax];

typedef struct steminfo {
    struct steminfo* next;
    unsigned int hinttype : 2; /* Only used by undoes */
    unsigned int ghost : 1;    /* this is a ghost stem hint. As such truetype
                                  should ignore it, type2 output should negate it,
                                  and type1 should use as is */
                               /* stored width will be either 20 or 21 */
    /* Type2 says: -20 is "width" of top edge, -21 is "width" of bottom edge,
     * type1 accepts either */
    unsigned int haspointleft : 1;
    unsigned int haspointright : 1;
    unsigned int hasconflicts : 1; /* Does this stem have conflicts within its
                                      cluster? */
    unsigned int used
        : 1; /* Temporary for counter hints or hint substitution */
    unsigned int tobeused
        : 1; /* Temporary for counter hints or hint substitution */
    unsigned int active : 1; /* Currently active hint in Review Hints dlg */
    /*  displayed differently in char display */
    unsigned int enddone : 1; /* Used by ttf instructing, indicates a prev */
    /*  hint had the same end as this one (so */
    /*  the points on the end line have been */
    /*  instructed already */
    unsigned int startdone : 1;    /* Used by ttf instructing */
    /*unsigned int backwards: 1;*/ /* If we think this hint is better done with
                                      a negative width */
    unsigned int reordered : 1; /* In AutoHinting. Means we changed the start of
                                   the hint, need to test for out of order */
    unsigned int pendingpt : 1; /* A pending stem creation, not a true stem */
    unsigned int linearedges
        : 1; /* If we have a nice rectangle then we aren't */
    /*  interested in the orientation which is */
    /*  wider than long */
    int16_t hintnumber; /* when dumping out hintmasks we need to know */
    /*  what bit to set for this hint */
    union {
        int mask; /* Mask of all references that use this hint */
                  /*  in type2 output */
        _MMArray*
            unblended /*[2][MmMax]*/; /* Used when reading in type1 mm hints */
    } u;
    real start;          /* location at which the stem starts */
    real width;          /* or height */
    HintInstance* where; /* location(s) in the other coord */
} StemInfo;

typedef struct dsteminfo {
    struct dsteminfo* next;    /* First two fields match those in steminfo */
    unsigned int hinttype : 2; /* Only used by undoes */
    unsigned int used : 1;     /* used only by tottf.c:gendinstrs, metafont.c to
                                  mark a hint that has been dealt with */
    BasePoint left, right, unit;
    HintInstance* where; /* location(s) along the unit vector */
} DStemInfo;

typedef struct minimumdistance {
    /* If either point is NULL it will be assumed to mean either the origin */
    /*  or the width point (depending on which is closer). This allows user */
    /*  to control metrics... */
    SplinePoint *sp1, *sp2;
    unsigned int x : 1;
    unsigned int done : 1;
    struct minimumdistance* next;
} MinimumDistance;

typedef struct splinechar {
    char* name;
    int unicodeenc;
    int orig_pos; /* Original position in the glyph list */
    int16_t width, vwidth;
    int16_t lsidebearing; /* only used when reading in a type1 font */
    /*  Or an otf font where it is the subr number of a referred character */
    /*  or a ttf font without bit 1 of head.flags set */
    /*  or (once upon a time, but no longer) a ttf font with vert metrics where
     * it is the ymax value when we had a font-wide vertical offset */
    /*  or when generating morx where it is the mask of tables in which the
     * glyph occurs */
    /* Always a temporary value */
    int ttf_glyph; /* only used when writing out a ttf or otf font */
    Layer* layers; /* layer[0] is background, layer[1] foreground */
    /* In type3 fonts 2-n are also foreground, otherwise also background */
    int layer_cnt;
    StemInfo*
        hstem; /* hstem hints have a vertical offset but run horizontally */
    StemInfo*
        vstem; /* vstem hints have a horizontal offset but run vertically */
    DStemInfo* dstem; /* diagonal hints for ttf */
    MinimumDistance* md;
    struct charviewbase* views;
    struct charinfo* charinfo;
    struct splinefont* parent;
    unsigned int changed : 1;
    unsigned int changedsincelasthinted : 1;
    unsigned int manualhints : 1;
    unsigned int ticked : 1; /* For reference character processing */
    /* And fontview processing */
    unsigned int changed_since_autosave : 1;
    unsigned int widthset : 1; /* needed so an emspace char doesn't disappear */
    unsigned int vconflicts : 1; /* Any hint overlaps in the vstem list? */
    unsigned int hconflicts : 1; /* Any hint overlaps in the hstem list? */
    unsigned int searcherdummy : 1;
    unsigned int changed_since_search : 1;
    unsigned int wasopen : 1;
    unsigned int namechanged : 1;
    unsigned int blended : 1; /* An MM blended character */
    unsigned int ticked2 : 1;
    unsigned int glyph_class : 3; /* 0=> fontforge determines class
                                     automagically, else one more than the class
                                     value in gdef so 2+1=>lig, 3+1=>mark */
    unsigned int numberpointsbackards : 1;
    unsigned int instructions_out_of_date : 1;
    unsigned int complained_about_ptnums : 1;
    unsigned int vs_open : 1;
    unsigned int unlink_rm_ovrlp_save_undo : 1;
    unsigned int inspiro : 1;
    unsigned int lig_caret_cnt_fixed : 1;
    unsigned int suspendMetricsViewEventPropagation
        : 1; /* rect tool might do this while
                drawing */
    /* 5 bits left (one more if we ignore compositionunit below) */
#if HANYANG
    unsigned int compositionunit : 1;
    int16_t jamo, variant;
#endif
    struct splinecharlist* dependents;
    /* The dependents list is a list of all characters which reference*/
    /*  the current character directly */
    struct kernpair* kerns;  // Note that the left character in the pair has the
                             // reference to the kerning pair, which in turn
                             // references the right character.
    struct kernpair* vkerns;
    struct generic_pst*
        possub; /* If we are a ligature then this tells us what */
    /*  It may also contain a bunch of other stuff now */
    struct liglist* ligofme; /* If this is the first character of a ligature
                                then this gives us the list of possible ones */
    /*  this field must be regenerated before the font is saved */
    char* comment; /* in utf8 */
    uint32_t /*Color*/ color;
    struct anchorpoint* anchor;
    uint8_t* ttf_instrs;
    int16_t ttf_instrs_len;
    int16_t countermask_cnt;
    HintMask* countermasks;
    struct altuni* altuni;
    /* for TeX */
    int16_t tex_height, tex_depth;
    /* TeX also uses italic_correction and glyph variants below */
    /* For the 'MATH' table (and for TeX) */
    unsigned int is_extended_shape : 1;
    int16_t italic_correction;
    int16_t top_accent_horiz; /* MATH table allows you to specific a*/
    /* horizontal anchor for accent attachments, vertical */
    /* positioning is done elsewhere */
    struct devicetab* italic_adjusts;
    struct devicetab* top_accent_adjusts;
    struct glyphvariants* vert_variants;
    struct glyphvariants* horiz_variants;
    struct mathkern* mathkern;
/* End of MATH/TeX fields */
#ifndef _NO_PYTHON
    void* python_sc_object;
    void* python_temporary;
#endif
#if 0
    // Python persistent data is now in the layers.
    void *python_persistent;		/* If python this will hold a python object, if not python this will hold a string containing a pickled object. We do nothing with it (if not python) except save it back out unchanged */
    int python_persistent_has_lists;
#endif  // 0
    /* If the glyph is used as a tile pattern, then the next two values */
    /*  determine the amount of white space around the tile. If extra is*/
    /*  non-zero then we add it to the max components of the bbox and   */
    /*  subtract it from the min components. If extra is 0 then tile_bounds*/
    /*  will be used. If tile_bounds is all zeros then the glyph's bbox */
    /*  will be used. */
    real tile_margin; /* If the glyph is used as a tile */
    DBounds tile_bounds;
    char* glif_name;  // This stores the base name of the glyph when saved to U.
                      // F. O..
    unichar_t* user_decomp;  // User decomposition for building this character
} SplineChar;

/* Returns NULL if the name is valid, or error message. If the function returned
   error message and set `questionable` to true, the name should be accepted,
   but discouraged. The error message should not be freed. */
const char* SCNameCheck(const unichar_t* name, bool* questionable);
const char* SCGetName(const SplineChar* sc);
void SCGetEncoding(const SplineChar* sc, int* p_unicodeenc, int* p_ttf_glyph);
