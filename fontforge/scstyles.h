#ifndef FONTFORGE_SCSTYLES_H
#define FONTFORGE_SCSTYLES_H

#include "baseviews.h"
#include "splinefont.h"

// TODO: move structsmallcaps here from baseviews.h

struct xheightinfo {
	double xheight_current, xheight_desired;
	double serif_height;
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
    double top_y, bottom_y, boundary;
    int has_two_zones;
#define TOP_Z	0
#define BOT_Z	1
    int cnts[2];
    int maxes[2];
    struct ci_zones *zones[2];
};

extern double SFSerifHeight(SplineFont *sf);
extern double SFStdVW(SplineFont *sf);
extern SplineSet *SSControlStems(SplineSet *ss, double stemwidthscale, double stemheightscale, double hscale, double vscale, double xheight);
extern void ChangeXHeight(FontViewBase *fv, CharViewBase *cv, struct xheightinfo *xi);
extern void CI_Init(struct counterinfo *ci, SplineFont *sf);
extern void CVEmbolden(CharViewBase *cv, enum embolden_type type, struct lcg_zones *zones);
extern void ChangeGlyph(SplineChar *sc_sc, SplineChar *orig_sc, int layer,
                        struct genericchange *genchange);
extern void CVGenericChange(CharViewBase *cv, struct genericchange *genchange);
extern void FVAddSmallCaps(FontViewBase *fv, struct genericchange *genchange);
extern void FVCondenseExtend(FontViewBase *fv, struct counterinfo *ci);
extern void FVEmbolden(FontViewBase *fv, enum embolden_type type, struct lcg_zones *zones);
extern void FVGenericChange(FontViewBase *fv, struct genericchange *genchange);
extern void InitXHeightInfo(SplineFont *sf, int layer, struct xheightinfo *xi);
extern void MakeItalic(FontViewBase *fv, CharViewBase *cv, ItalicInfo *ii);
extern void SCCondenseExtend(struct counterinfo *ci, SplineChar *sc, int layer, int do_undoes);
extern void ScriptSCCondenseExtend(SplineChar *sc, struct counterinfo *ci);
extern void ScriptSCEmbolden(SplineChar *sc, int layer, enum embolden_type type, struct lcg_zones *zones);
extern void SmallCapsFindConstants(struct smallcaps *small, SplineFont *sf, int layer);

#endif /* FONTFORGE_SCSTYLES_H */
