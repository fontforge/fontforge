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

#ifdef USE_DOUBLE
# define real	double
#else
# define real	float
#endif

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
    real radius;
    enum linejoin join;
    enum linecap cap;
    unsigned int caligraphic: 1;
    unsigned int toobigwarn: 1;
    real penangle;
    real thickness;			/* doesn't work */
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
    real xheight, xheighttop;		/* height of "x" and "o" */
    real caph, caphtop;			/* height of "I" and "O" */
    real ascent;			/* height of "l" */
    real descent;			/* depth of "p" */
} BlueData;

typedef struct bdffloat {
    int16 xmin,xmax,ymin,ymax;
    int16 bytes_per_line;
    uint8 *bitmap;
} BDFFloat;

typedef struct undoes {
    struct undoes *next;
    enum undotype { ut_none=0, ut_state, ut_tstate, ut_statehint, ut_width,
	    ut_bitmap, ut_bitmapsel, ut_composit, ut_multiple, ut_noop } undotype;
    union {
	struct {
	    int16 width;
	    int16 lbearingchange;
	    struct splinepointlist *splines;
	    struct refchar *refs;
	    union {
		struct imagelist *images;
		void *hints;
	    } u;
	    struct splinefont *copied_from;
	} state;
	int width;
	struct {
	    /*int16 width;*/	/* width should be controled by postscript */
	    int16 xmin,xmax,ymin,ymax;
	    int16 bytes_per_line;
	    int16 pixelsize;
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
    uint16 ptindex;		/* Temporary value used by metafont routine */
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
    int16 adobe_enc;
    int local_enc;
    int unicode_enc;		/* used by paste */
    real transform[6];		/* transformation matrix (first 2 rows of a 3x3 matrix, missing row is 0,0,1) */
    SplinePointList *splines;
    struct refchar *next;
    DBounds bb;
    struct splinechar *sc;
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
		    /* Type2 says: -20 is "width" of top edge, -21 is "width" of bottom edge, type1 accepts either */
    unsigned int haspointleft:1;
    unsigned int haspointright:1;
    unsigned int haspoints:1;	/* both edges of the stem have points on them */
				/*  at the stem left and right edge */
			        /*  trivially true for horizontal/vertical lines */
			        /*  true for curves if extrema have points */
			        /*  except we aren't smart enough to detect that yet */
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
    int16 hintnumber;		/* when dumping out hintmasks we need to know */
				/*  what bit to set for this hint */
    int mask;			/* Mask of all references that use this hint */
				/*  in type2 output */
    real start;			/* location at which the stem starts */
    real width;			/* or height */
    HintInstance *where;	/* location(s) in the other coord */
} StemInfo;

typedef struct dsteminfo {
    struct dsteminfo *next;	/* First two fields match those in steminfo */
    unsigned int hinttype: 2;	/* Only used by undoes */
    BasePoint leftedgetop, rightedgetop, leftedgebottom, rightedgebottom;
} DStemInfo;

typedef struct imagelist {
    struct gimage *image;
    real xoff, yoff;		/* position in character space of upper left corner of image */
    real xscale, yscale;	/* scale to convert one pixel of image to one unit of character space */
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
    StemInfo *hstem;		/* hstem hints have a vertical offset but run horizontally */
    StemInfo *vstem;		/* vstem hints have a horizontal offset but run vertically */
    DStemInfo *dstem;		/* diagonal hints for ttf */
    RefChar *refs;
    struct charview *views;
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
    struct splinecharlist { struct splinechar *sc; struct splinecharlist *next;} *dependents;
	    /* The dependents list is a list of all characters which refenence*/
	    /*  the current character directly */
    SplinePointList *backgroundsplines;
    ImageList *backimages;
    Undoes *undoes[2];
    Undoes *redoes[2];
    KernPair *kerns;
    Ligature *lig;		/* If we are a ligature then this tells us what */
    LigList *ligofme;		/* If this is the first character of a ligature then this gives us the list of possible ones */
				/*  this field must be regenerated before the font is saved */
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

typedef struct splinefont {
    char *fontname, *fullname, *familyname, *weight;
    char *copyright;
    char *filename;
    char *version;
    real italicangle, upos, uwidth;		/* In font info */
    int ascent, descent;
    int uniqueid;				/* Not copied when reading in!!!! */
    int charcnt;
    SplineChar **chars;
    unsigned int changed: 1;
    unsigned int changed_since_autosave: 1;
    unsigned int display_antialias: 1;
    unsigned int dotlesswarn: 1;		/* User warned that font doesn't have a dotless i character */
    /*unsigned int subrsgood: 1;		/* the subrs & othersubrs arrays are correct for flex&hint subs */
    /*unsigned int wasbinary: 1;*/
    unsigned int onlybitmaps: 1;		/* it's a bdf editor, not a postscript editor */
    unsigned int serifcheck: 1;			/* Have we checked to see if we have serifs? */
    unsigned int issans: 1;			/* We have no serifs */
    unsigned int isserif: 1;			/* We have serifs. If neither set then we don't know. */
    struct fontview *fv;
    enum charset encoding_name;
    SplinePointList *gridsplines;
    Undoes *gundoes, *gredoes;
    int *hsnaps, *vsnaps;
    BDFFont *bitmaps;
    char *origname;		/* filename of font file (ie. if not an sfd) */
    char *autosavename;
    int display_size;
    struct psdict *private;		/* read in from type1 file or provided by user */
    /*struct pschars *subrs;		/* actually an array, but this will do */
    char *xuid;
    struct pfminfo {
	unsigned int pfmset: 1;
	unsigned char pfmfamily;
	unsigned short weight;
	unsigned short width;
	char panose[10];
	int fstype;
    } pfminfo;
    struct ttflangname *names;
    char *cidregistry, *ordering;
    int supplement;
    int subfontcnt;
    struct splinefont **subfonts;
    struct splinefont *cidmaster;		/* Top level cid font */
    float cidversion;
} SplineFont;

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
extern int SFOneWidth(SplineFont *sf);
extern int SFIsCJK(SplineFont *sf);
extern struct pschars *SplineFont2Chrs(SplineFont *sf, int round, int iscjk,
	struct pschars *subrs);
struct cidbytes;
struct fd2data;
extern struct pschars *CID2Chrs(SplineFont *cidmaster,struct cidbytes *cidbytes);
extern struct pschars *SplineFont2Subrs2(SplineFont *sf);
extern struct pschars *SplineFont2Chrs2(SplineFont *sf, int nomwid, int defwid,
	struct pschars *subrs);
extern struct pschars *CID2Chrs2(SplineFont *cidmaster,struct fd2data *fds);
enum fontformat { ff_pfa, ff_pfb, ff_ptype3, ff_ptype0, ff_cid,
	ff_ttf, ff_ttfsym, ff_otf, ff_otfcid, ff_none };
extern int _WritePSFont(FILE *out,SplineFont *sf,enum fontformat format);
extern int WritePSFont(char *fontname,SplineFont *sf,enum fontformat format);
extern int WriteTTFFont(char *fontname,SplineFont *sf, enum fontformat format);
extern void DefaultTTFEnglishNames(struct ttflangname *dummy, SplineFont *sf);
extern void SFDefaultOS2Info(struct pfminfo *pfminfo,SplineFont *sf,char *fontname);
extern int SFReencodeFont(SplineFont *sf,enum charset new_map);
extern char *SFGetModifiers(SplineFont *sf);
extern void SFSetFontName(SplineFont *sf, char *family, char *mods, char *full);

extern int RealNear(real a,real b);
extern int RealNearish(real a,real b);
extern int RealApprox(real a,real b);

extern void LineListFree(LineList *ll);
extern void LinearApproxFree(LinearApprox *la);
extern void SplineFree(Spline *spline);
extern void SplinePointFree(SplinePoint *sp);
extern void SplinePointListFree(SplinePointList *spl);
extern void SplinePointListsFree(SplinePointList *head);
extern void RefCharFree(RefChar *ref);
extern void RefCharsFree(RefChar *ref);
extern void UndoesFree(Undoes *undo);
extern void StemInfosFree(StemInfo *h);
extern void StemInfoFree(StemInfo *h);
extern void DStemInfosFree(DStemInfo *h);
extern void DStemInfoFree(DStemInfo *h);
extern void KernPairsFree(KernPair *kp);
extern void LigatureFree(Ligature *lig);
extern StemInfo *StemInfoCopy(StemInfo *h);
extern DStemInfo *DStemInfoCopy(DStemInfo *h);
extern SplineChar *SplineCharCopy(SplineChar *sc);
extern BDFChar *BDFCharCopy(BDFChar *bc);
extern void ImageListsFree(ImageList *imgs);
extern void TTFLangNamesFree(struct ttflangname *l);
extern void SplineCharFree(SplineChar *sc);
extern void SplineFontFree(SplineFont *sf);
extern void SplineRefigure(Spline *spline);
extern Spline *SplineMake(SplinePoint *from, SplinePoint *to);
extern LinearApprox *SplineApproximate(Spline *spline, real scale);
extern int SplinePointListIsClockwise(SplineSet *spl);
extern void SplineSetFindBounds(SplinePointList *spl, DBounds *bounds);
extern void SplineCharFindBounds(SplineChar *sc,DBounds *bounds);
extern void SplineFontFindBounds(SplineFont *sf,DBounds *bounds);
extern void CIDFindBounds(SplineFont *sf,DBounds *bounds);
extern void SplinePointCatagorize(SplinePoint *sp);
extern int SplinePointIsACorner(SplinePoint *sp);
extern void SCCatagorizePoints(SplineChar *sc);
extern int UnicodeNameLookup(char *name);
extern SplinePointList *SplinePointListCopy(SplinePointList *base);
extern SplinePointList *SplinePointListCopySelected(SplinePointList *base);
extern SplinePointList *SplinePointListTransform(SplinePointList *base, real transform[6], int allpoints );
extern SplinePointList *SplinePointListShift(SplinePointList *base, real xoff, int allpoints );
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
extern void BCPasteInto(BDFChar *bc,BDFChar *rbc,int ixoff,int iyoff, int invert, int cleartoo);
extern BDFChar *SplineCharRasterize(SplineChar *sc, int pixelsize);
extern BDFFont *SplineFontRasterize(SplineFont *sf, int pixelsize, int indicate);
extern BDFChar *SplineCharAntiAlias(SplineChar *sc, int pixelsize,int linear_scale);
extern BDFFont *SplineFontAntiAlias(SplineFont *sf, int pixelsize,int linear_scale);
extern BDFFont *BitmapFontScaleTo(BDFFont *old, int to);
extern void BDFCharFree(BDFChar *bdfc);
extern void BDFFontFree(BDFFont *bdf);
extern int  BDFFontDump(char *filename,BDFFont *font, char *encodingname);
extern int SplinesIntersect(Spline *s1, Spline *s2, BasePoint pts[4], real t1s[4], real t2s[4]);
extern int CubicSolve(Spline1D *sp,real ts[3]);
extern real SplineSolve(Spline1D *sp, real tmin, real tmax, real sought_y, real err);
extern int SplineSolveFull(Spline1D *sp,real val, real ts[3]);
extern void SplineFindInflections(Spline1D *sp, real *_t1, real *_t2 );
extern void SplineRemoveInflectionsTooClose(Spline1D *sp, real *_t1, real *_t2 );
extern int NearSpline(struct findsel *fs, Spline *spline);
extern real SplineNearPoint(Spline *spline, BasePoint *bp, real fudge);
extern void SCMakeDependent(SplineChar *dependent,SplineChar *base);
extern SplinePoint *SplineBisect(Spline *spline, double t);
extern Spline *ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt);
extern int SplineIsLinear(Spline *spline);
extern int SplineIsLinearMake(Spline *spline);
extern int SplineInSplineSet(Spline *spline, SplineSet *spl);
extern void SplineCharMerge(SplineSet **head);
extern void SplinePointListSimplify(SplinePointList *spl,int cleanup);
extern SplineSet *SplineCharSimplify(SplineSet *head,int cleanup);
extern SplineSet *SplineCharRemoveTiny(SplineSet *head);
extern SplineFont *SplineFontNew(void);
extern char *GetNextUntitledName(void);
extern SplineFont *SplineFontEmpty(void);
extern SplineFont *SplineFontBlank(int enc,int charcnt);
extern void SFIncrementXUID(SplineFont *sf);
extern SplineSet *SplineSetReverse(SplineSet *spl);
extern SplineSet *SplineSetsExtractOpen(SplineSet **tbase);
extern SplineSet *SplineSetsCorrect(SplineSet *base);
extern SplineSet *SplineSetsDetectDir(SplineSet **_base, int *lastscan);
extern void SPAverageCps(SplinePoint *sp);
extern void SplineCharDefaultPrevCP(SplinePoint *base, SplinePoint *prev);
extern void SplineCharDefaultNextCP(SplinePoint *base, SplinePoint *next);
extern void SplineCharTangentNextCP(SplinePoint *sp);
extern void SplineCharTangentPrevCP(SplinePoint *sp);
extern int PointListIsSelected(SplinePointList *spl);
extern void SplineSetsUntick(SplineSet *spl);
extern void SFOrderBitmapList(SplineFont *sf);
extern int KernThreshold(SplineFont *sf, int cnt);
extern real SFGuessItalicAngle(SplineFont *sf);
extern void SFHasSerifs(SplineFont *sf);

extern SplineSet *SplineSetStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc);
extern SplineSet *SplineSetRemoveOverlap(SplineSet *base);

extern void FindBlues( SplineFont *sf, real blues[14], real otherblues[10]);
extern void QuickBlues(SplineFont *sf, BlueData *bd);
extern void FindHStems( SplineFont *sf, real snaps[12], real cnt[12]);
extern void FindVStems( SplineFont *sf, real snaps[12], real cnt[12]);
extern void SCGuessHHintInstancesAndAdd(SplineChar *sc, StemInfo *stem);
extern void SCGuessVHintInstancesAndAdd(SplineChar *sc, StemInfo *stem);
extern void SCGuessHHintInstancesList(SplineChar *sc);
extern void SCGuessVHintInstancesList(SplineChar *sc);
extern real HIlen( StemInfo *stems);
extern real HIoverlap( HintInstance *mhi, HintInstance *thi);
extern int StemInfoAnyOverlaps(StemInfo *stems);
extern int StemListAnyConflicts(StemInfo *stems);
extern HintInstance *HICopyTrans(HintInstance *hi, real mul, real offset);
extern void SplineCharAutoHint( SplineChar *sc,int removeOverlaps);
extern void SplineFontAutoHint( SplineFont *sf);
extern StemInfo *HintCleanup(StemInfo *stem,int dosort);
extern int SplineFontIsFlexible(SplineFont *sf);
extern int SCWorthOutputting(SplineChar *sc);
extern int CIDWorthOutputting(SplineFont *cidmaster, int enc); /* Returns -1 on failure, font number on success */
extern int AfmSplineFont(FILE *afm, SplineFont *sf,int formattype);
extern int PfmSplineFont(FILE *pfm, SplineFont *sf,int type0);
extern char *EncodingName(int map);
extern void SFLigaturePrepare(SplineFont *sf);
extern void SFLigatureCleanup(SplineFont *sf);

extern int SFDWrite(char *filename,SplineFont *sf);
extern int SFDWriteBak(SplineFont *sf);
extern SplineFont *SFDRead(char *filename);
extern SplineFont *SFReadTTF(char *filename);
extern SplineFont *CFFParse(FILE *temp,int len,char *fontsetname);
extern SplineFont *LoadSplineFont(char *filename);
extern SplineFont *ReadSplineFont(char *filename);	/* Don't use this, use LoadSF instead */

extern SplineChar *SCBuildDummy(SplineChar *dummy,SplineFont *sf,int i);
extern SplineChar *SFMakeChar(SplineFont *sf,int i);
extern Ligature *SCLigDefault(SplineChar *sc);
extern BDFChar *BDFMakeChar(BDFFont *bdf,int i);

extern void SCUndoSetLBearingChange(SplineChar *sc,int lb);
extern Undoes *SCPreserveState(SplineChar *sc,int dohints);
extern Undoes *SCPreserveBackground(SplineChar *sc);
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
extern SplineChar *SFGetChar(SplineFont *sf, int unienc, char *name );
extern int SFFindExistingChar(SplineFont *sf, int unienc, char *name );

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

extern void SCRound2Int(SplineChar *sc,struct fontview *);
extern int hascomposing(SplineFont *sf,int u);
extern void SFFigureGrid(SplineFont *sf);

struct cidmap;			/* private structure to encoding.c */
extern int CID2NameEnc(struct cidmap *map,int cid, char *buffer, int len);
extern int NameEnc2CID(struct cidmap *map,int enc, char *name);
extern int MaxCID(struct cidmap *map);
extern struct cidmap *FindCidMap(char *registry,char *ordering,int supplement);
extern void SFEncodeToMap(SplineFont *sf,struct cidmap *map);
extern struct cidmap *AskUserForCIDMap(SplineFont *sf);
#endif

