#ifndef FONTFORGE_PALMFONTS_H
#define FONTFORGE_PALMFONTS_H

#include "splinefont.h"

extern int WritePalmBitmaps(const char *filename, SplineFont *sf, int32 *sizes, EncMap *map);
extern SplineFont *SFReadPalmPdb(char *filename);

#endif /* FONTFORGE_PALMFONTS_H */
