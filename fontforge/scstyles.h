#ifndef FONTFORGE_SCSTYLES_H
#define FONTFORGE_SCSTYLES_H

#include "baseviews.h"
#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO: move structsmallcaps here from baseviews.h

struct xheightinfo {
	double xheight_current, xheight_desired;
	double serif_height;
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

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_SCSTYLES_H */
