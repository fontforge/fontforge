/* Copyright (C) 2000,2001 by George Williams */
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

typedef struct strokeinfo {
    double radius;
    enum linejoin join;
    enum linecap cap;
    unsigned int caligraphic: 1;
    unsigned int toobigwarn: 1;
    double penangle;
    double thickness;			/* doesn't work */
} StrokeInfo;

typedef struct ipoint {
    int x;
    int y;
} IPoint;

typedef struct basepoint {
    double x;
    double y;
} BasePoint;

typedef struct tpoint {
    double x;
    double y;
    double t;
} TPoint;

typedef struct dbounds {
    double minx, maxx;
    double miny, maxy;
} DBounds;

typedef struct bdffloat {
    int16 xmin,xmax,ymin,ymax;
    int16 bytes_per_line;
    uint8 *bitmap;
} BDFFloat;

typedef struct undoes {
    struct undoes *next;
    enum undotype { ut_none=0, ut_state, ut_tstate, ut_width,
	    ut_bitmap, ut_bitmapsel, ut_composit, ut_multiple, ut_noop } undotype;
    union {
	struct {
	    int16 width;
	    int16 lbearingchange;
	    struct splinepointlist *splines;
	    struct refchar *refs;
	    struct imagelist *images;
	} state;
	int width;
	struct {
	    /*int16 width;*/	/* width should be controled by postscript */
	    int16 xmin,xmax,ymin,ymax;
	    int16 bytes_per_line;
	    uint8 *bitmap;
	    BDFFloat *selection;
	    int pixelsize;
	} bmpstate;
	struct {		/* copy contains an outline state and a set of bitmap states */
	    struct undoes *state;
	    struct undoes *bitmaps;
	} composit;
	struct {
	    struct undoes *mult; /* copy contains several sub copies (composits, or states or widths or...) */
	} multiple;
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
    BDFFloat *selection;
} BDFChar;

typedef struct bdffont {
    struct splinefont *sf;
    int charcnt;
    BDFChar **chars;		/* an array of charcnt entries */
    BDFChar **temp;		/* Used by ReencodeFont routine */
    int pixelsize;
    int ascent, descent;
    enum charset encoding_name;
    struct bdffont *next;
    struct clut *clut;
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
    uint16 flex;		/* This is a flex serif have to go through icky flex output */
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
    double scale;
    unsigned int oneline: 1;
    unsigned int onepoint: 1;
    unsigned int any: 1;		/* refers to a particular screen */
    struct linelist *lines;
    struct linearapprox *next;
} LinearApprox;

typedef struct spline1d {
    double a, b, c, d;
} Spline1D;

typedef struct spline {
    unsigned int islinear: 1;
    unsigned int isticked: 1;
    unsigned int isneeded: 1;
    unsigned int isunneeded: 1;
    unsigned int ishorvert: 1;
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
    int16 adobe_enc;
    uint16 local_enc;
    int unicode_enc;		/* used by paste */
    double transform[6];	/* transformation matrix (first 2 rows of a 3x3 matrix, missing row is 0,0,1) */
    SplinePointList *splines;
    struct refchar *next;
    unsigned int checked: 1;
    unsigned int selected: 1;
    DBounds bb;
    struct splinechar *sc;
} RefChar;

typedef struct kernpair {
    struct splinechar *sc;
    int off;
    struct kernpair *next;
} KernPair;

typedef struct hints {
    double base, width;
    double b1, b2, e1, e2;
    double ab, ae;
    unsigned int adjustb: 1;
    unsigned int adjuste: 1;
    struct hints *next;
} Hints;

typedef struct imagelist {
    struct gimage *image;
    double xoff, yoff;		/* position in character space of upper left corner of image */
    double xscale, yscale;	/* scale to convert one pixel of image to one unit of character space */
    DBounds bb;
    struct imagelist *next;
    unsigned int selected: 1;
} ImageList;

typedef struct ligature {
    struct splinechar *lig;
    char *componants;
} Ligature;

typedef struct liglist {
    Ligature *lig;
    struct liglist *next;
} LigList;

typedef struct splinechar {
    char *name;
    int enc, unicodeenc;
    int width;
    int16 lsidebearing;		/* only used when reading in a type1 font */
				/*  Or an otf font where it is the subr number of a refered character */
    int16 ttf_glyph;		/* only used when writing out a ttf or otf font */
    SplinePointList *splines;
    Hints *hstem;		/* hstem hints have a vertical offset but run horizontally */
    Hints *vstem;		/* vstem hints have a horizontal offset but run vertically */
    RefChar *refs;
    struct charview *views;
    struct splinefont *parent;
    unsigned int changed: 1;
    unsigned int changedsincelasthhinted: 1;
    unsigned int changedsincelastvhinted: 1;
    unsigned int manualhints: 1;
    unsigned int ticked: 1;	/* For reference character processing */
    unsigned int changed_since_autosave: 1;
    unsigned int widthset: 1;	/* needed so an emspace char doesn't disappear */
    unsigned int origistype2: 1;
    struct splinecharlist { struct splinechar *sc; struct splinecharlist *next;} *dependents;
	    /* The dependents list is a list of all characters which refenence*/
	    /*  the current character directly */
    SplinePointList *backgroundsplines;
    ImageList *backimages;
    Undoes *undoes[2];
    Undoes *redoes[2];
    KernPair *kerns;
    uint8 *origtype1;		/* Original string from Type1 file (unencoded) (or type2 from cff) */
    int origlen;		/* Length of string */
    Ligature *lig;		/* If we are a ligature then this tells us what */
    LigList *ligofme;		/* If this is the first character of a ligature then this gives us the list of possible ones */
				/*  this field must be regenerated before the font is saved */
} SplineChar;

typedef struct splinefont {
    char *fontname, *fullname, *familyname, *weight;
    char *copyright;
    char *filename;
    char *version;
    double italicangle, upos, uwidth;		/* In font info */
    int ascent, descent;
    int charcnt;
    SplineChar **chars;
    unsigned int changed: 1;
    unsigned int changed_since_autosave: 1;
    unsigned int display_antialias: 1;
    unsigned int dotlesswarn: 1;		/* User warned that font doesn't have a dotless i character */
    unsigned int subrsgood: 1;			/* the subrs & othersubrs arrays are correct for flex&hint subs */
    /*unsigned int wasbinary: 1;*/
    unsigned int onlybitmaps: 1;		/* it's a bdf editor, not a postscript editor */
    struct fontview *fv;
    enum charset encoding_name;
    SplinePointList *gridsplines;
    Undoes *gundoes, *gredoes;
    BDFFont *bitmaps;
    char *origname;		/* filename of font file (ie. if not an sfd) */
    char *autosavename;
    int display_size;
    struct psdict *private;		/* read in from type1 file or provided by user */
    struct pschars *subrs;		/* actually an array, but this will do */
    char *xuid;
} SplineFont;

struct fontdict;
struct pschars;
struct findsel;
struct charprocs;
enum charset;
struct enc;

extern SplineFont *SplineFontFromPSFont(struct fontdict *fd);
extern int CheckAfmOfPostscript(SplineFont *sf,char *psname);
extern int LoadKerningDataFromAfm(SplineFont *sf, char *filename);
extern int SFOneWidth(SplineFont *sf);
extern struct pschars *SplineFont2Chrs(SplineFont *sf, int round);
extern struct pschars *SplineFont2Subrs2(SplineFont *sf);
extern struct pschars *SplineFont2Chrs2(SplineFont *sf, int nomwid, int defwid,
	struct pschars *subrs);
enum fontformat { ff_pfa, ff_pfb, ff_ptype3, ff_ptype0, ff_ttf, ff_ttfsym,
	ff_otf, ff_none };
extern int WritePSFont(char *fontname,SplineFont *sf,enum fontformat format);
extern int WriteTTFFont(char *fontname,SplineFont *sf, enum fontformat format);
extern int SFReencodeFont(SplineFont *sf,enum charset new_map);
extern void SFSetFontName(SplineFont *sf, char *family, char *mods);

extern int DoubleNear(double a,double b);
extern int DoubleNearish(double a,double b);

extern void LineListFree(LineList *ll);
extern void LinearApproxFree(LinearApprox *la);
extern void SplineFree(Spline *spline);
extern void SplinePointFree(SplinePoint *sp);
extern void SplinePointListFree(SplinePointList *spl);
extern void SplinePointListsFree(SplinePointList *head);
extern void RefCharFree(RefChar *ref);
extern void RefCharsFree(RefChar *ref);
extern void UndoesFree(Undoes *undo);
extern void HintsFree(Hints *h);
extern void KernPairsFree(KernPair *kp);
extern void LigatureFree(Ligature *lig);
extern Hints *HintsCopy(Hints *h);
extern SplineChar *SplineCharCopy(SplineChar *sc);
extern BDFChar *BDFCharCopy(BDFChar *bc);
extern void ImageListsFree(ImageList *imgs);
extern void SplineCharFree(SplineChar *sc);
extern void SplineFontFree(SplineFont *sf);
extern void SplineRefigure(Spline *spline);
extern Spline *SplineMake(SplinePoint *from, SplinePoint *to);
extern LinearApprox *SplineApproximate(Spline *spline, double scale);
extern int SplinePointListIsClockwise(SplineSet *spl);
extern void SplineSetFindBounds(SplinePointList *spl, DBounds *bounds);
extern void SplineCharFindBounds(SplineChar *sc,DBounds *bounds);
extern void SplineFontFindBounds(SplineFont *sf,DBounds *bounds);
extern void SplinePointCatagorize(SplinePoint *sp);
extern void SCCatagorizePoints(SplineChar *sc);
extern int UnicodeNameLookup(char *name);
extern SplinePointList *SplinePointListCopy(SplinePointList *base);
extern SplinePointList *SplinePointListCopySelected(SplinePointList *base);
extern SplinePointList *SplinePointListTransform(SplinePointList *base, double transform[6], int allpoints );
extern SplinePointList *SplinePointListShift(SplinePointList *base, double xoff, int allpoints );
extern SplinePointList *SplinePointListRemoveSelected(SplinePointList *base);
extern void SplinePointListSet(SplinePointList *tobase, SplinePointList *frombase);
extern void SplinePointListSelect(SplinePointList *spl,int sel);
extern void SCRefToSplines(SplineChar *sc,RefChar *rf);
extern void SCReinstanciateRefChar(SplineChar *sc,RefChar *rf);
extern void SCReinstanciateRef(SplineChar *sc,SplineChar *rsc);
extern void SCRemoveDependent(SplineChar *dependent,RefChar *rf);
extern void SCRemoveDependents(SplineChar *dependent);
extern RefChar *SCCanonicalRefs(SplineChar *sc, int isps);
extern int SCDependsOnSC(SplineChar *parent, SplineChar *child);
extern void BCCompressBitmap(BDFChar *bdfc);
extern void BCRegularizeBitmap(BDFChar *bdfc);
extern void BCRegularizeGreymap(BDFChar *bdfc);
extern void BCPasteInto(BDFChar *bc,BDFChar *rbc,int ixoff,int iyoff, int invert);
extern BDFChar *SplineCharRasterize(SplineChar *sc, int pixelsize);
extern BDFFont *SplineFontRasterize(SplineFont *sf, int pixelsize);
extern BDFChar *SplineCharAntiAlias(SplineChar *sc, int pixelsize,int linear_scale);
extern BDFFont *SplineFontAntiAlias(SplineFont *sf, int pixelsize,int linear_scale);
extern BDFFont *BitmapFontScaleTo(BDFFont *old, int to);
extern void BDFCharFree(BDFChar *bdfc);
extern void BDFFontFree(BDFFont *bdf);
extern int  BDFFontDump(char *filename,BDFFont *font, char *encodingname);
extern double SplineSolve(Spline1D *sp, double tmin, double tmax, double sought_y, double err);
extern void SplineFindInflections(Spline1D *sp, double *_t1, double *_t2 );
extern int NearSpline(struct findsel *fs, Spline *spline);
extern double SplineNearPoint(Spline *spline, BasePoint *bp, double fudge);
extern void SCMakeDependent(SplineChar *dependent,SplineChar *base);
extern SplinePoint *SplineBisect(Spline *spline, double t);
extern Spline *ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt);
extern int SplineIsLinear(Spline *spline);
extern int SplineInSplineSet(Spline *spline, SplineSet *spl);
extern void SplineCharMerge(SplineSet **head);
extern void SplineCharSimplify(SplineSet *head);
extern void SplineCharRemoveTiny(SplineSet *head);
extern SplineFont *SplineFontNew(void);
extern SplineFont *SplineFontBlank(int enc,int charcnt);
extern void SFIncrementXUID(SplineFont *sf);
extern SplineSet *SplineSetReverse(SplineSet *spl);
extern SplineSet *SplineSetsExtractOpen(SplineSet **tbase);
extern SplineSet *SplineSetsCorrect(SplineSet *base);
extern void SPAverageCps(SplinePoint *sp);
extern void SplineCharDefaultPrevCP(SplinePoint *base, SplinePoint *prev);
extern void SplineCharDefaultNextCP(SplinePoint *base, SplinePoint *next);
extern void SplineCharTangentNextCP(SplinePoint *sp);
extern void SplineCharTangentPrevCP(SplinePoint *sp);
extern int PointListIsSelected(SplinePointList *spl);
extern void SplineSetsUntick(SplineSet *spl);
extern void SFOrderBitmapList(SplineFont *sf);
extern int KernThreshold(SplineFont *sf, int cnt);
extern double SFGuessItalicAngle(SplineFont *sf);

extern SplineSet *SplineSetStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc);
extern SplineSet *SplineSetRemoveOverlap(SplineSet *base);

extern void FindBlues( SplineFont *sf, double blues[14], double otherblues[10]);
extern void FindHStems( SplineFont *sf, double snaps[12], double cnt[12]);
extern void FindVStems( SplineFont *sf, double snaps[12], double cnt[12]);
extern void SplineCharAutoHint( SplineChar *sc);
extern void SplineFontAutoHint( SplineFont *sf);
extern int SplineFontIsFlexible(SplineFont *sf);
extern int SCWorthOutputting(SplineChar *sc);
extern int AfmSplineFont(FILE *afm, SplineFont *sf,int type0);
extern char *EncodingName(int map);
extern void SFLigaturePrepare(SplineFont *sf);
extern void SFLigatureCleanup(SplineFont *sf);

extern int SFDWrite(char *filename,SplineFont *sf);
extern int SFDWriteBak(SplineFont *sf);
extern SplineFont *SFDRead(char *filename);
extern SplineFont *SFReadTTF(char *filename);
extern SplineFont *LoadSplineFont(char *filename);
extern SplineFont *ReadSplineFont(char *filename);	/* Don't use this, use LoadSF instead */

extern SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,int i);
extern SplineChar *SFMakeChar(SplineFont *sf,int i);
extern Ligature *SCLigDefault(SplineChar *sc);
extern BDFChar *BDFMakeChar(BDFFont *bdf,int i);

extern void SCUndoSetLBearingChange(SplineChar *sc,int lb);
extern Undoes *SCPreserveState(SplineChar *sc);
extern Undoes *SCPreserveWidth(SplineChar *sc);
extern Undoes *BCPreserveState(BDFChar *bc);
extern void BCDoRedo(BDFChar *bc,struct fontview *fv);
extern void BCDoUndo(BDFChar *bc,struct fontview *fv);

extern int SFIsCompositBuildable(SplineFont *sf,int unicodeenc);
extern void SCBuildComposit(SplineFont *sf, SplineChar *sc, int copybmp,
	struct fontview *fv);
extern const unichar_t *SFGetAlternate(SplineFont *sf, int base);

extern int getAdobeEnc(char *name);

extern SplinePointList *SplinePointListInterpretPS(FILE *ps);
extern void PSFontInterpretPS(FILE *ps,struct charprocs *cp);
extern struct enc *PSSlurpEncodings(FILE *file);
extern SplineChar *PSCharStringToSplines(uint8 *type1, int len, int is_type2,
	struct pschars *subrs, struct pschars *gsubrs, const char *name);

extern int SFFindChar(SplineFont *sf, int unienc, char *name );
extern int SFFindExistingChar(SplineFont *sf, int unienc, char *name );

extern char *getPfaEditDir(char *buffer);
extern void DoAutoSaves(void);
extern int DoAutoRecovery(void);
extern SplineFont *SFRecoverFile(char *autosavename);
extern void SFAutoSave(SplineFont *sf);
extern void SFClearAutoSave(SplineFont *sf);

extern void SCClearOrig(SplineChar *sc);

extern void PSCharsFree(struct pschars *chrs);
extern void PSDictFree(struct psdict *chrs);
extern int PSDictFindEntry(struct psdict *dict, char *key);
extern char *PSDictHasEntry(struct psdict *dict, char *key);
extern int PSDictRemoveEntry(struct psdict *dict, char *key);
extern int PSDictChangeEntry(struct psdict *dict, char *key, char *newval);

extern void SCRound2Int(SplineChar *sc,struct fontview *);
#endif
