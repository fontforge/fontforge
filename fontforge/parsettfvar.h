#ifndef FONTFORGE_PARSETTFVAR_H
#define FONTFORGE_PARSETTFVAR_H

#include <stdio.h>

#include "ttf.h"

extern void readttfvariations(struct ttfinfo *info, FILE *ttf);
extern void VariationFree(struct ttfinfo *info);

#endif /* FONTFORGE_PARSETTFVAR_H */
