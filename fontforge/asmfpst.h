#ifndef FONTFORGE_ASMFPST_H
#define FONTFORGE_ASMFPST_H

#ifdef __cplusplus
extern "C" {
#endif

#include "splinefont.h"

extern ASM *ASMFromFPST(SplineFont *sf, FPST *fpst, int ordered);
extern ASM *ASMFromOpenTypeForms(SplineFont *sf, uint32_t script);
extern FPST *FPSTGlyphToClass(FPST *fpst);
extern int ClassesMatch(int cnt1, char **classes1, int cnt2, char **classes2);
extern int FPSTisMacable(SplineFont *sf, FPST *fpst);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_ASMFPST_H */
