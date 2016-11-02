#ifndef FONTFORGE_SPLINEOVERLAP_H
#define FONTFORGE_SPLINEOVERLAP_H

#include "edgelist2.h"
#include "splinefont.h"

extern int CheckMonotonicClosed(struct monotonic *ms);
extern int MonotonicFindAt(Monotonic *ms, int which, extended test, Monotonic **space);
/* overlap_type controls whether we look at selected splinesets or all splinesets */
extern Monotonic *SSsToMContours(SplineSet *spl, enum overlap_type ot);
extern SplineSet *SplineSetRemoveOverlap(SplineChar *sc, SplineSet *base, enum overlap_type ot);
extern void FreeMonotonics(Monotonic *m);
extern void SSRemoveBacktracks(SplineSet *ss);

#endif /* FONTFORGE_SPLINEOVERLAP_H */
