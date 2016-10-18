#ifndef FONTFORGE_EFFECTS_H
#define FONTFORGE_EFFECTS_H

#include "baseviews.h"
#include "splinefont.h"

extern SplineSet *SSShadow(SplineSet *spl, real angle, real outline_width, real shadow_length, SplineChar *sc, int wireframe);
extern void FVInline(FontViewBase *fv, real width, real inset);
extern void FVOutline(FontViewBase *fv, real width);
extern void FVShadow(FontViewBase *fv, real angle, real outline_width, real shadow_length, int wireframe);

#endif /* FONTFORGE_EFFECTS_H */
