#ifndef FONTFORGE_AUTOHINT_H
#define FONTFORGE_AUTOHINT_H

#include "edgelist.h"
#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

extern EI *EIActiveEdgesFindStem(EI *apt, real i, int major);
extern EI *EIActiveEdgesRefigure(EIList *el, EI *active, real i, int major, int *_change);
extern EI *EIActiveListReorder(EI *active, int *change);
extern HintInstance *HICopyTrans(HintInstance *hi, real mul, real offset);
extern int EISameLine(EI *e, EI *n, real i, int major);
extern int EISkipExtremum(EI *e, real i, int major);
extern int MergeDStemInfo(SplineFont *sf, DStemInfo **ds, DStemInfo *test);
extern int SFNeedsAutoHint(SplineFont *_sf);
extern int SplineCharIsFlexible(SplineChar *sc, int layer);
extern int SplineFontIsFlexible(SplineFont *sf, int layer, int flags);
extern int StemInfoAnyOverlaps(StemInfo *stems);
extern int StemListAnyConflicts(StemInfo *stems);
extern real EITOfNextMajor(EI *e, EIList *el, real sought_m);
extern real HIlen(StemInfo *stems);
extern real HIoverlap(HintInstance *mhi, HintInstance *thi);
extern StemInfo *HintCleanup(StemInfo *stem, int dosort, int instance_count);
extern void ELFindEdges(SplineChar *sc, EIList *el);
extern void ElFreeEI(EIList *el);
extern void ELOrder(EIList *el, int major);
extern void FindBlues(SplineFont *sf, int layer, real blues[14], real otherblues[10]);
extern void FindHStems(SplineFont *sf, real snaps[12], real cnt[12]);
extern void FindVStems(SplineFont *sf, real snaps[12], real cnt[12]);
extern void QuickBlues(SplineFont *_sf, int layer, BlueData *bd);
extern void SCClearHintMasks(SplineChar *sc, int layer, int counterstoo);
extern void SCClearHints(SplineChar *sc);
extern void SCFigureCounterMasks(SplineChar *sc);
extern void SCFigureHintMasks(SplineChar *sc, int layer);
extern void SCFigureVerticalCounterMasks(SplineChar *sc);
extern void SCGuessDHintInstances(SplineChar *sc, int layer, DStemInfo *ds);
extern void SCGuessHHintInstancesAndAdd(SplineChar *sc, int layer, StemInfo *stem, real guess1, real guess2);
extern void SCGuessHHintInstancesList(SplineChar *sc, int layer);
extern void SCGuessHintInstancesList(SplineChar *sc, int layer, StemInfo *hstem, StemInfo *vstem, DStemInfo *dstem, int hvforce, int dforce);
extern void SCGuessVHintInstancesAndAdd(SplineChar *sc, int layer, StemInfo *stem, real guess1, real guess2);
extern void SCGuessVHintInstancesList(SplineChar *sc, int layer);
extern void SCModifyHintMasksAdd(SplineChar *sc, int layer, StemInfo *stem);
extern void SFSCAutoHint(SplineChar *sc, int layer, BlueData *bd);
extern void SplineCharAutoHint(SplineChar *sc, int layer, BlueData *bd);
extern void _SplineCharAutoHint(SplineChar *sc, int layer, BlueData *bd, struct glyphdata *gd2, int gen_undoes);
extern void SplineFontAutoHintRefs(SplineFont *_sf, int layer);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_AUTOHINT_H */
