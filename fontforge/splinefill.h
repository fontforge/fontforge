#ifndef FONTFORGE_SPLINEFILL_H
#define FONTFORGE_SPLINEFILL_H

#include "edgelist.h"
#include "splinefont.h"

#include <gdraw.h>

enum piecemeal_flags { pf_antialias=1, pf_bbsized=2, pf_ft_nohints=4, pf_ft_recontext=8 };

extern BDFChar *BDFPieceMeal(BDFFont *bdf, int index);
extern BDFChar *BDFPieceMealCheck(BDFFont *bdf, int index);
extern BDFChar *SplineCharAntiAlias(SplineChar *sc, int layer, int pixelsize, int linear_scale);
extern BDFChar *SplineCharRasterize(SplineChar *sc, int layer, bigreal pixelsize);
extern BDFFont *SplineFontAntiAlias(SplineFont *_sf, int layer, int pixelsize, int linear_scale);
extern BDFFont *SplineFontPieceMeal(SplineFont *sf, int layer, int ptsize, int dpi, int flags, void *freetype_context);
extern BDFFont *SplineFontRasterize(SplineFont *_sf, int layer, int pixelsize, int indicate);
extern BDFFont *SplineFontToBDFHeader(SplineFont *_sf, int pixelsize, int indicate);
extern bigreal TOfNextMajor(Edge *e, EdgeList *es, bigreal sought_m);
extern Edge *ActiveEdgesFindStem(Edge *apt, Edge **prev, real i);
extern Edge *ActiveEdgesInsertNew(EdgeList *es, Edge *active, int i);
extern Edge *ActiveEdgesRefigure(EdgeList *es, Edge *active, real i);
extern GClut *_BDFClut(int linear_scale);
extern int BDFDepth(BDFFont *bdf);
extern int GradientHere(bigreal scale, DBounds *bbox, int iy, int ix, struct gradient *grad, struct pattern *pat, int defgrey);
extern void BCCompressBitmap(BDFChar *bdfc);
extern void BCRegularizeBitmap(BDFChar *bdfc);
extern void BCRegularizeGreymap(BDFChar *bdfc);
extern void BDFCAntiAlias(BDFChar *bc, int linear_scale);
extern void BDFCharFree(BDFChar *bdfc);
extern void BDFFontFree(BDFFont *bdf);
extern void BDFPropsFree(BDFFont *bdf);
extern void FindEdgesSplineSet(SplinePointList *spl, EdgeList *es, int ignore_clip);
extern void FreeEdges(EdgeList *es);
extern void PatternPrep(SplineChar *sc, struct brush *brush, bigreal scale);

#endif /* FONTFORGE_SPLINEFILL_H */
