#ifndef FONTFORGE_SPLINEORDER2_H
#define FONTFORGE_SPLINEORDER2_H

#include "splinefont.h"

extern SplinePoint *SplineTtfApprox(Spline *ps);
extern SplineSet *SplineSetsConvertOrder(SplineSet *ss, int to_order2);
extern SplineSet *SplineSetsPSApprox(SplineSet *ss);
extern SplineSet *SplineSetsTTFApprox(SplineSet *ss);
extern SplineSet *SSPSApprox(SplineSet *ss);
extern SplineSet *SSttfApprox(SplineSet *ss);
extern Spline *SplineMake2(SplinePoint *from, SplinePoint *to);
extern void SCConvertLayerToOrder2(SplineChar *sc, int layer);
extern void SCConvertLayerToOrder3(SplineChar *sc, int layer);
extern void SCConvertOrder(SplineChar *sc, int to_order2);
extern void SCConvertToOrder2(SplineChar *sc);
extern void SCConvertToOrder3(SplineChar *sc);
extern void SFConvertGridToOrder2(SplineFont *_sf);
extern void SFConvertGridToOrder3(SplineFont *_sf);
extern void SFConvertLayerToOrder2(SplineFont *_sf, int layer);
extern void SFConvertLayerToOrder3(SplineFont *_sf, int layer);
extern void SFConvertToOrder2(SplineFont *_sf);
extern void SFConvertToOrder3(SplineFont *_sf);
extern void SplinePointNextCPChanged2(SplinePoint *sp);
extern void SplinePointPrevCPChanged2(SplinePoint *sp);
extern void SplineRefigure2(Spline *spline);
extern void SplineRefigureFixup(Spline *spline);

#endif /* FONTFORGE_SPLINEORDER2_H */
