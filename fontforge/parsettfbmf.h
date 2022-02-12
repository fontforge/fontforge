#ifndef FONTFORGE_PARSETTFBMF_H
#define FONTFORGE_PARSETTFBMF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "splinefont.h"
#include "ttf.h"

extern void ttfdumpbitmapscaling(SplineFont *sf, struct alltabs *at, int32_t *sizes);
extern void TTFLoadBitmaps(FILE *ttf, struct ttfinfo *info, int onlyone);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_PARSETTFBMF_H */
