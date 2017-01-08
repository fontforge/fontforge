#ifndef FONTFORGE_WINFONTS_H
#define FONTFORGE_WINFONTS_H

#include "splinefont.h"

extern int FNTFontDump(char *filename, BDFFont *font, EncMap *map, int res);
extern int FONFontDump(char *filename, SplineFont *sf, int32 *sizes, int resol, EncMap *map);
extern SplineFont *SFReadWinFON(char *filename, int toback);

#endif /* FONTFORGE_WINFONTS_H */
