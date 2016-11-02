#ifndef FONTFORGE_SPLINESTROKE_H
#define FONTFORGE_SPLINESTROKE_H

#include "splinefont.h"

enum PolyType {
	Poly_Convex,
	Poly_Concave,
	Poly_PointOnEdge,
	Poly_TooFewPoints,
	Poly_Line
};

extern enum PolyType PolygonIsConvex(BasePoint *poly, int n, int *badpointindex);
extern SplineSet *SplineSetStroke(SplineSet *ss, StrokeInfo *si, int order2);
extern SplineSet *UnitShape(int n);
extern void FVStrokeItScript(void *_fv, StrokeInfo *si, int pointless_argument);

#endif /* FONTFORGE_SPLINESTROKE_H */
