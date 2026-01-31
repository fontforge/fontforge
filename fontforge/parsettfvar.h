#ifndef FONTFORGE_PARSETTFVAR_H
#define FONTFORGE_PARSETTFVAR_H

#include "ttf.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void readttfvariations(struct ttfinfo *info, FILE *ttf);
extern void VariationFree(struct ttfinfo *info);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_PARSETTFVAR_H */
