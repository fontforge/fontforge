#ifndef FONTFORGE_ASMFPST_H
#define FONTFORGE_ASMFPST_H

#include "splinefont.h"

extern ASM *ASMFromFPST(SplineFont *sf, FPST *fpst, int ordered);
extern ASM *ASMFromOpenTypeForms(SplineFont *sf, uint32_t script);
extern FPST *FPSTGlyphToClass(FPST *fpst);
extern int ClassesMatch(int cnt1, char **classes1, int cnt2, char **classes2);
extern int FPSTisMacable(SplineFont *sf, FPST *fpst);

#endif /* FONTFORGE_ASMFPST_H */
