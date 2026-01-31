#ifndef FONTFORGE_FEATUREFILE_H
#define FONTFORGE_FEATUREFILE_H

#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void FeatDumpFontLookups(FILE *out, SplineFont *sf);
extern void FeatDumpOneLookup(FILE *out, SplineFont *sf, OTLookup *otl);
extern void SFApplyFeatureFilename(SplineFont *sf, char *filename,bool ignore_invalid_replacement);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_FEATUREFILE_H */
