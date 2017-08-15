#ifndef FONTFORGE_SPLINESAVE_H
#define FONTFORGE_SPLINESAVE_H

#include "splinefont.h"

/**
 * It is like a == b, but also true if a is within tolerence of b.
 */
extern bool equalWithTolerence(real a, real b, real tolerence);

extern int CIDOneWidth(SplineFont *_sf);
extern int CvtPsStem3(GrowBuf *gb, SplineChar *scs[MmMax], int instance_count, int ishstem, int round);
extern int SFIsCJK(SplineFont *sf, EncMap *map);
extern int SFOneHeight(SplineFont *sf);
extern int SFOneWidth(SplineFont *sf);
extern struct pschars *CID2ChrsSubrs2(SplineFont *cidmaster, struct fd2data *fds, int flags, struct pschars **_glbls, int layer);
extern struct pschars *SplineFont2ChrsSubrs2(SplineFont *sf, int nomwid, int defwid, const int *bygid, int cnt, int flags, struct pschars **_subrs, int layer);
extern void debug_printHintInstance(HintInstance* hi, int hin, char* msg);
extern void RefCharsFreeRef(RefChar *ref);

#endif /* FONTFORGE_SPLINESAVE_H */
