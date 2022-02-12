#ifndef FONTFORGE_PALMFONTS_H
#define FONTFORGE_PALMFONTS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "splinefont.h"

extern int WritePalmBitmaps(const char *filename, SplineFont *sf, int32_t *sizes, EncMap *map);
extern SplineFont *SFReadPalmPdb(char *filename);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_PALMFONTS_H */
