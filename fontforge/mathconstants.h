#ifndef FONTFORGE_MATHCONSTANTS_H
#define FONTFORGE_MATHCONSTANTS_H

#include "splinefont.h"

extern struct MATH *MathTableNew(SplineFont *sf);
extern void MATHFree(struct MATH *math);

#endif /* FONTFORGE_MATHCONSTANTS_H */
