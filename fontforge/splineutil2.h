#ifndef FONTFORGE_SPLINEUTIL2_H
#define FONTFORGE_SPLINEUTIL2_H

#include "splinefont.h"

enum ae_type { ae_all, ae_between_selected, ae_only_good, ae_only_good_rm_later };

extern bigreal PathLength(SplineSet *ss);
extern bigreal SplineLengthRange(Spline *spline, real from_t, real to_t);
extern char *GetNextUntitledName(void);
extern int PointListIsSelected(SplinePointList *spl);
extern int PointsDiagonalable(SplineFont *sf, BasePoint **bp, BasePoint *unit);
extern int RealApprox(real a, real b);
extern int RealNearish(real a, real b);
extern int RealRatio(real a, real b, real fudge);
extern int RealWithin(real a, real b, real fudge);
extern int SPInterpolate(const SplinePoint *sp);
extern int SpIsExtremum(SplinePoint *sp);
extern int Spline1DCantExtremeX(const Spline *s);
extern int Spline1DCantExtremeY(const Spline *s);
extern int SplineInSplineSet(Spline *spline, SplineSet *spl);
extern int SplineIsLinearMake(Spline *spline);
extern int SplinePointListIsClockwise(const SplineSet *spl);
extern int SPLNearlyLines(SplineChar *sc, SplineSet *ss, bigreal err);
extern int Within16RoundingErrors(bigreal v1, bigreal v2);
extern int Within4RoundingErrors(bigreal v1, bigreal v2);
extern int Within64RoundingErrors(bigreal v1, bigreal v2);
extern Spline *ApproximateSplineFromPointsSlopes(SplinePoint *from, SplinePoint *to, TPoint *mid, int cnt, int order2);
extern SplineFont *SplineFontBlank(int charcnt);
extern SplineFont *SplineFontEmpty(void);
extern SplineFont *SplineFontNew(void);
extern Spline *PathFindDistance(SplineSet *path, bigreal d, bigreal *_t);
extern SplineSet *SplineCharRemoveTiny(SplineChar *sc, SplineSet *head);
extern SplineSet *SplineCharSimplify(SplineChar *sc, SplineSet *head, struct simplifyinfo *smpl);
extern SplineSet *SplineSetBindToPath(SplineSet *ss, int doscale, int glyph_as_unit, int align, real offset, SplineSet *path);
extern SplineSet *SplineSetJoin(SplineSet *start, int doall, real fudge, int *changed);
extern SplineSet *SplineSetReverse(SplineSet *spl);
extern SplineSet *SplineSetsAntiCorrect(SplineSet *base);
extern SplineSet *SplineSetsCorrect(SplineSet *base, int *changed);
extern SplineSet *SplineSetsDetectDir(SplineSet **_base, int *_lastscan);
extern SplineSet *SplineSetsExtractOpen(SplineSet **tbase);
extern SplineSet *SSRemoveZeroLengthSplines(SplineSet *base);
extern Spline *SplineAddExtrema(Spline *s, int always, real lenbound, real offsetbound, DBounds *b);
extern void BP_HVForce(BasePoint *vector);
extern void CanonicalContours(SplineChar *sc, int layer);
extern void SFIncrementXUID(SplineFont *sf);
extern void SFRandomChangeXUID(SplineFont *sf);
extern void SPAdjustControl(SplinePoint *sp, BasePoint *cp, BasePoint *to, int order2);
extern void SPAverageCps(SplinePoint *sp);
extern void SPHVCurveForce(SplinePoint *sp);
extern void SPLAverageCps(SplinePointList *spl);
extern void SplineCharAddExtrema(SplineChar *sc, SplineSet *head, enum ae_type between_selected, int emsize);
extern void SplineCharDefaultNextCP(SplinePoint *base);
extern void SplineCharDefaultPrevCP(SplinePoint *base);
extern void SplineCharMerge(SplineChar *sc, SplineSet **head, int type);
extern void SplineCharTangentNextCP(SplinePoint *sp);
extern void SplineCharTangentPrevCP(SplinePoint *sp);
extern void SplinePointListSet(SplinePointList *tobase, SplinePointList *frombase);
extern void SplinePointListSimplify(SplineChar *sc, SplinePointList *spl, struct simplifyinfo *smpl);
extern void SplineSetAddExtrema(SplineChar *sc, SplineSet *ss, enum ae_type between_selected, int emsize);
extern void SplineSetJoinCpFixup(SplinePoint *sp);
extern void SplineSetsInsertOpen(SplineSet **tbase, SplineSet *open);
extern void SplineSetsUntick(SplineSet *spl);
extern void SplinesRemoveBetween(SplineChar *sc, SplinePoint *from, SplinePoint *to, int type);
extern void SPLNearlyHvCps(SplineChar *sc, SplineSet *ss, bigreal err);
extern void SPLNearlyHvLines(SplineChar *sc, SplineSet *ss, bigreal err);
extern void SPLsStartToLeftmost(SplineChar *sc, int layer);
extern void SPLStartToLeftmost(SplineChar *sc, SplinePointList *spl, int *changed);
extern void SPSmoothJoint(SplinePoint *sp);

/**
 * This is like SPAdjustControl but you have not wanting to move the
 * BCP at all, but you would like the current location of the passed
 * BCP to reshape the spline through the splinepoint. For example, if
 * you drag the spline between two points then you might like to touch
 * the inside BCP between the two splinepoints to reshape the whole
 * curve through a curve point.
 */
extern void SPTouchControl(SplinePoint *sp, BasePoint *which, int order2);

extern void SPWeightedAverageCps(SplinePoint *sp);
extern void SSOverlapClusterCpAngles(SplineSet *base, bigreal within);
extern void SSRemoveStupidControlPoints(SplineSet *base);

#endif /* FONTFORGE_SPLINEUTIL2_H */
