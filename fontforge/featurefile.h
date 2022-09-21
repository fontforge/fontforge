#ifndef FONTFORGE_FEATUREFILE_H
#define FONTFORGE_FEATUREFILE_H

#include "splinefont.h"

extern void FeatDumpFontLookups(FILE *out, SplineFont *sf);
extern void FeatDumpOneLookup(FILE *out, SplineFont *sf, OTLookup *otl);
extern void SFApplyFeatureFilename(SplineFont *sf, char *filename,bool ignore_invalid_replacement);

#endif /* FONTFORGE_FEATUREFILE_H */
