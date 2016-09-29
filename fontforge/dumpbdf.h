#ifndef FONTFORGE_DUMPBDF_H
#define FONTFORGE_DUMPBDF_H

#include "splinefont.h"

extern int BDFFontDump(char *filename, BDFFont *font, EncMap *map, int res);
extern int IsntBDFChar(BDFChar *bdfc);

#endif /* FONTFORGE_DUMPBDF_H */
