#ifndef FONTFORGE_FVCOMPOSITE_H
#define FONTFORGE_FVCOMPOSITE_H

#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

extern AnchorClass *AnchorClassCursMatch(SplineChar *sc1, SplineChar *sc2, AnchorPoint **_ap1, AnchorPoint **_ap2);
extern AnchorClass *AnchorClassMatch(SplineChar *sc1, SplineChar *sc2, AnchorClass *restrict_, AnchorPoint **_ap1, AnchorPoint **_ap2);
extern AnchorClass *AnchorClassMkMkMatch(SplineChar *sc1, SplineChar *sc2, AnchorPoint **_ap1, AnchorPoint **_ap2);
extern const unichar_t *SFGetAlternate(SplineFont *sf, int base, SplineChar *sc, int nocheck);
extern int CanonicalCombiner(int uni);
extern int hascomposing(SplineFont *sf, int u, SplineChar *sc);
extern int isaccent(int uni);
extern int SCAppendAccent(SplineChar *sc, int layer, char *glyph_name, int uni, uint32_t pos);
extern int SFIsCompositBuildable(SplineFont *sf, int unicodeenc, SplineChar *sc, int layer);
extern int SFIsRotatable(SplineFont *sf, SplineChar *sc);
extern int SFIsSomethingBuildable(SplineFont *sf, SplineChar *sc, int layer, int onlyaccents);
extern void _SCAddRef(SplineChar *sc, SplineChar *rsc, int layer, real transform[6], int selected);
extern void SCBuildComposit(SplineFont *sf, SplineChar *sc, int layer, BDFFont *bdf, int disp_only, int accent_hint);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_FVCOMPOSITE_H */
