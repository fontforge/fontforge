#ifndef FONTFORGE_TOTTFVAR_H
#define FONTFORGE_TOTTFVAR_H

#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Apple variation tables */
extern int ContourPtNumMatch(MMSet *mm, int gid);
extern int16_t **SCFindDeltas(MMSet *mm, int gid, int *_ptcnt);
extern int16_t **CvtFindDeltas(MMSet *mm, int *_ptcnt);
extern void ttf_dumpvariations(struct alltabs *at, SplineFont *sf);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_TOTTFVAR_H */
