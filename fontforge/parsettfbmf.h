#ifndef FONTFORGE_PARSETTFBMF_H
#define FONTFORGE_PARSETTFBMF_H

#include <stdio.h>

#include "splinefont.h"
#include "ttf.h"

extern void ttfdumpbitmapscaling(SplineFont *sf, struct alltabs *at, int32 *sizes);
extern void TTFLoadBitmaps(FILE *ttf, struct ttfinfo *info, int onlyone);

#endif /* FONTFORGE_PARSETTFBMF_H */
