#ifndef FONTFORGE_FONTFORGEEXE_HISTOGRAMS_H
#define FONTFORGE_FONTFORGEEXE_HISTOGRAMS_H

#include <splinefont.h>

enum hist_type { hist_hstem, hist_vstem, hist_blues };

extern void SFHistogram(SplineFont *sf, int layer, struct psdict *private, uint8 *selected, EncMap *map, enum hist_type which);

#endif /* FONTFORGE_FONTFORGEEXE_HISTOGRAMS_H */
