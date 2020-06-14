#ifndef FONTFORGE_SPLINESTROKE_H
#define FONTFORGE_SPLINESTROKE_H

#include "splinefont.h"

enum ShapeType {
	Shape_Convex,
	Shape_CCWTurn,
	Shape_CCW,
	Shape_Quadratic,
	Shape_PointOnEdge,
	Shape_TooFewPoints,
	Shape_NotClosed,
	Shape_TinySpline,
	Shape_HalfLinear,
	Shape_BadCP_R1,
	Shape_BadCP_R2,
	Shape_BadCP_R3,
	Shape_SelfIntersects
};

extern StrokeInfo *CVFreeHandInfo();
extern StrokeInfo *CVStrokeInfo();
extern int ConvexNibID(const char *tok);
extern int StrokeSetConvex(SplineSet *ss, int toknum);
extern SplineSet *StrokeGetConvex(int toknum, int cpy);
extern enum ShapeType NibIsValid(SplineSet *);
extern void SplineStrokeSimpleFixup(SplinePoint *tailp, BasePoint p);
extern const char *NibShapeTypeMsg(enum ShapeType st);
extern SplinePoint *AppendCubicSplinePortion(Spline *s, bigreal t_start,
                                             bigreal t_end, SplinePoint *start);
extern SplinePoint *AppendCubicSplineSetPortion(Spline *s, bigreal t_start,
                                                Spline *s_end, bigreal t_end,
                                                SplinePoint *dst_start,
                                                int backward);
extern void SSAppendArc(SplineSet *cur, bigreal major, bigreal minor, 
                        BasePoint ang, BasePoint ut_fm, BasePoint ut_to,
                        int bk, int limit);
extern SplineSet *SplineSetStroke(SplineSet *ss, StrokeInfo *si, int order2);
extern SplineSet *UnitShape(int n);
extern void FVStrokeItScript(void *_fv, StrokeInfo *si, int pointless_argument);

#endif /* FONTFORGE_SPLINESTROKE_H */
