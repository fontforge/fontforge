#ifndef FONTFORGE_TOTTFVAR_H
#define FONTFORGE_TOTTFVAR_H

#include "splinefont.h"

/* Apple variation tables */
extern int ContourPtNumMatch(MMSet *mm, int gid);
extern int16 **SCFindDeltas(MMSet *mm, int gid, int *_ptcnt);
extern int16 **CvtFindDeltas(MMSet *mm, int *_ptcnt);
extern void ttf_dumpvariations(struct alltabs *at, SplineFont *sf);

#endif /* FONTFORGE_TOTTFVAR_H */
