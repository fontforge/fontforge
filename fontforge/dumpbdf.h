#ifndef FONTFORGE_DUMPBDF_H
#define FONTFORGE_DUMPBDF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "splinefont.h"

extern int BDFFontDump(char *filename, BDFFont *font, EncMap *map, int res);
extern int IsntBDFChar(BDFChar *bdfc);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_DUMPBDF_H */
