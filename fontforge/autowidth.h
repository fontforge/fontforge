#ifndef FONTFORGE_AUTOWIDTH_H
#define FONTFORGE_AUTOWIDTH_H

#include "baseviews.h"
#include "splinefont.h"

struct charone {
    real lbearing, rmax;
    real newl, newr;
    int baseserif, lefttops, righttops;		/* serif zones which affect this character */
    SplineChar *sc;
    int base, top;		/* bottom of character, number of decimation zones we've got */
    short *ledge;
    short *redge;
    struct charpair *asleft;
    struct charpair *asright;
};

struct charpair {
    struct charone *left, *right;
    struct charpair *nextasleft, *nextasright;
    int base, top;
    short *distances;
    short visual;
};

typedef struct widthinfo {
    real spacing;		/* desired spacing between letters */
    real decimation;
    real serifsize;
    real seriflength;
    real caph;
    real descent;
    real xheight;
    real n_stem_exterior_width, n_stem_interior_width;
    real current_I_spacing;
    int serifs[4][2];		/* Four serif zones: descent, baseline, xheight, cap */
    int lcnt, rcnt;		/* count of left and right chars respectively */
    int real_lcnt, real_rcnt;	/* what the user asked for. We might add I */
    int tcnt;			/* sum of r+l cnt */
    int pcnt;			/* pair count, often r*l cnt */
    int l_Ipos, r_Ipos;
    struct charone **left, **right;
    struct charpair **pairs;
    int space_guess;
    int threshold;
    SplineFont *sf;
    FontViewBase *fv;
    int layer;
    unsigned int done: 1;
    unsigned int autokern: 1;
    unsigned int onlynegkerns: 1;
    struct lookup_subtable *subtable;
} WidthInfo;

#define NOTREACHED	-9999.0

extern struct charone *AW_MakeCharOne(SplineChar *sc);
extern void AW_InitCharPairs(WidthInfo *wi);
extern void AW_FreeCharList(struct charone **list);
extern void AW_FreeCharPairs(struct charpair **list, int cnt);
extern void AW_ScriptSerifChecker(WidthInfo *wi);
extern int AW_ReadKernPairFile(char *fn,WidthInfo *wi);
extern void AW_BuildCharPairs(WidthInfo *wi);
extern void AW_AutoKern(WidthInfo *wi);
extern void AW_AutoWidth(WidthInfo *wi);
extern void AW_FindFontParameters(WidthInfo *wi);
extern void AW_KernRemoveBelowThreshold(SplineFont *sf,int threshold);

extern SplineFont *aw_old_sf;
extern int aw_old_spaceguess;

extern int AutoKernScript(FontViewBase *fv, int spacing, int threshold, struct lookup_subtable *sub, char *kernfile);
extern int AutoWidthScript(FontViewBase *fv, int spacing);
extern int KernThreshold(SplineFont *sf, int cnt);
extern real SFGuessItalicAngle(SplineFont *sf);
extern void FVRemoveKerns(FontViewBase *fv);
extern void FVRemoveVKerns(FontViewBase *fv);
extern void FVVKernFromHKern(FontViewBase *fv);

#endif /* FONTFORGE_AUTOWIDTH_H */
