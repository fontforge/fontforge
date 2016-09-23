#ifndef FONTFORGE_FONTFORGEEXE_METRICSVIEW_H
#define FONTFORGE_FONTFORGEEXE_METRICSVIEW_H

extern GTextInfo *SLOfFont(SplineFont *sf);
extern MetricsView *MetricsViewCreate(FontView *fv, SplineChar *sc, BDFFont *bdf);
extern void MetricsViewFinishNonStatic();
extern void MetricsViewFree(MetricsView *mv);
extern void MVColInit(void);
extern void MV_FriendlyFeatures(GGadget *g, int pos);
extern void MVRefreshAll(MetricsView *mv);
extern void MVRefreshChar(MetricsView *mv, SplineChar *sc);
extern void MVRefreshMetric(MetricsView *mv);
extern void MVRegenChar(MetricsView *mv, SplineChar *sc);
extern void MVReKern(MetricsView *mv);
extern void MVSelectFirstKerningTable(struct metricsview *mv);
extern void MVSetSCs(MetricsView *mv, SplineChar **scs);

#endif /* FONTFORGE_FONTFORGEEXE_METRICSVIEW_H */
