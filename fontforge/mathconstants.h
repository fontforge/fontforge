#ifndef FONTFORGE_MATHCONSTANTS_H
#define FONTFORGE_MATHCONSTANTS_H

#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

extern struct MATH *MathTableNew(SplineFont *sf);
extern void MATHFree(struct MATH *math);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_MATHCONSTANTS_H */
