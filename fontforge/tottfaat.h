#ifndef FONTFORGE_TOTTFAAT_H
#define FONTFORGE_TOTTFAAT_H

#include "splinefont.h"
#include "ttf.h"

/* Apple Advanced Typography Tables */

extern void aat_dumpacnt(struct alltabs *at, SplineFont *sf);
extern void ttf_dumpkerns(struct alltabs *at, SplineFont *sf);
extern void aat_dumplcar(struct alltabs *at, SplineFont *sf);
extern void aat_dumpmorx(struct alltabs *at, SplineFont *sf);
extern void aat_dumpopbd(struct alltabs *at, SplineFont *sf);
extern void aat_dumpprop(struct alltabs *at, SplineFont *sf);
extern void aat_dumpbsln(struct alltabs *at, SplineFont *sf);

extern int LookupHasDefault(OTLookup *otl);
extern int scriptsHaveDefault(struct scriptlanglist *sl);
extern uint32 MacFeatureToOTTag(int featureType, int featureSetting);
extern int OTTagToMacFeature(uint32 tag, int *featureType, int *featureSetting);
extern uint16 *props_array(SplineFont *sf, struct glyphinfo *gi);
extern int haslrbounds(SplineChar *sc, PST **left, PST **right);
extern int16 *PerGlyphDefBaseline(SplineFont *sf, int *def_baseline);
extern void FigureBaseOffsets(SplineFont *sf, int def_bsln, int offsets[32]);

extern int Macable(SplineFont *sf, OTLookup *otl);

#endif /* FONTFORGE_TOTTFAAT_H */
