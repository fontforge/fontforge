#ifndef FONTFORGE_PARSETTFBMF_H
#define FONTFORGE_PARSETTFBMF_H

#include "splinefont.h"
#include "ttf.h"

extern void ttfdumpbitmapscaling(SplineFont *sf, struct alltabs *at, int32_t *sizes);
extern void TTFLoadBitmaps(FILE *ttf, struct ttfinfo *info, int onlyone);

#endif /* FONTFORGE_PARSETTFBMF_H */
