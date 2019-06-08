#ifndef FONTFORGE_PARSETTFATT_H
#define FONTFORGE_PARSETTFATT_H

#include "ttf.h"

/* The MATH table */
extern void otf_read_math(FILE *ttf, struct ttfinfo *info);
extern void otf_read_math_used(FILE *ttf, struct ttfinfo *info);
extern void GuessNamesFromMATH(FILE *ttf, struct ttfinfo *info);

/* Parsing advanced typography */
extern void readmacfeaturemap(FILE *ttf, struct ttfinfo *info);
extern void readttfbase(FILE *ttf, struct ttfinfo *info);
extern void readttfbsln(FILE *ttf, struct ttfinfo *info);
extern void readttfgdef(FILE *ttf, struct ttfinfo *info);
extern void readttfgpossub(FILE *ttf, struct ttfinfo *info, int gpos);
extern void readttfgsubUsed(FILE *ttf, struct ttfinfo *info);
extern void readttfjstf(FILE *ttf, struct ttfinfo *info);
extern void readttfkerns(FILE *ttf, struct ttfinfo *info);
extern void readttflcar(FILE *ttf, struct ttfinfo *info);
extern void readttfmort(FILE *ttf, struct ttfinfo *info);
extern void readttfmort_glyphsused(FILE *ttf, struct ttfinfo *info);
extern void readttfopbd(FILE *ttf, struct ttfinfo *info);
extern void readttfprop(FILE *ttf, struct ttfinfo *info);
extern void GuessNamesFromGSUB(FILE *ttf, struct ttfinfo *info);

#endif /* FONTFORGE_PARSETTFATT_H */
