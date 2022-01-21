#ifndef FONTFORGE_PALMFONTS_H
#define FONTFORGE_PALMFONTS_H

#include "splinefont.h"

extern int WritePalmBitmaps(const char *filename, SplineFont *sf, int32_t *sizes, EncMap *map);
extern SplineFont *SFReadPalmPdb(char *filename);

#endif /* FONTFORGE_PALMFONTS_H */
