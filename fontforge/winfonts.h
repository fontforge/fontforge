#ifndef FONTFORGE_WINFONTS_H
#define FONTFORGE_WINFONTS_H

#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int FNTFontDump(char *filename, BDFFont *font, EncMap *map, int res);
extern int FONFontDump(char *filename, SplineFont *sf, int32_t *sizes, int resol, EncMap *map);
extern SplineFont *SFReadWinFON(char *filename, int toback);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_WINFONTS_H */
