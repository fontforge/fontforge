#ifndef FONTFORGE_SPLINESTROKE_H
#define FONTFORGE_SPLINESTROKE_H

#include "splinefont.h"

enum ShapeType {
	Shape_Convex,
	Shape_CCWTurn,
	Shape_CCW,
	Shape_PointOnEdge,
	Shape_TooFewPoints,
	Shape_NotClosed,
	Shape_TinySpline,
	Shape_HalfLinear,
	Shape_BadCP,
	Shape_SelfIntersects
};

extern StrokeInfo *CVFreeHandInfo();
extern StrokeInfo *CVStrokeInfo();
extern int ConvexNibID(char *tok);
extern int StrokeSetConvex(SplineSet *ss, int toknum);
extern SplineSet *StrokeGetConvex(int toknum, int cpy);
extern enum ShapeType NibIsValid(SplineSet *);
extern const char *NibShapeTypeMsg(enum ShapeType st);
extern SplinePoint *AppendCubicSplinePortion(Spline *s, bigreal t_start,
                                             bigreal t_end, SplinePoint *start);
extern SplinePoint *AppendCubicSplineSetPortion(Spline *s, bigreal t_start,
                                                Spline *s_end, bigreal t_end,
                                                SplinePoint *dst_start,
                                                int backward);
extern SplineSet *SplineSetStroke(SplineSet *ss, StrokeInfo *si, int order2);
extern SplineSet *UnitShape(int n);
extern void FVStrokeItScript(void *_fv, StrokeInfo *si, int pointless_argument);

#endif /* FONTFORGE_SPLINESTROKE_H */
