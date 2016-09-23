#ifndef FONTFORGE_FONTFORGEEXE_PROBLEMS_H
#define FONTFORGE_FONTFORGEEXE_PROBLEMS_H

#include <splinefont.h>

extern char *VSErrorsFromMask(int mask, int private_mask);
extern void FindProblems(FontView *fv, CharView *cv, SplineChar *sc);
extern void SFValidationWindow(SplineFont *sf, int layer, enum fontformat format);
extern void ValidationDestroy(SplineFont *sf);

#endif /* FONTFORGE_FONTFORGEEXE_PROBLEMS_H */
