#ifndef FONTFORGE_PARSETTFVAR_H
#define FONTFORGE_PARSETTFVAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ttf.h"

extern void readttfvariations(struct ttfinfo *info, FILE *ttf);
extern void VariationFree(struct ttfinfo *info);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_PARSETTFVAR_H */
