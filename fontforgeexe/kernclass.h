#ifndef FONTFORGE_FONTFORGEEXE_KERNCLASS_H
#define FONTFORGE_FONTFORGEEXE_KERNCLASS_H

typedef struct kernclasslistdlg {
    SplineFont *sf;
    int layer;
    GWindow gw;
    int isv;
} KernClassListDlg;

extern KernClass *SFFindKernClass(SplineFont *sf, SplineChar *first, SplineChar *last, int *index, int allow_zero);
extern KernClass *SFFindVKernClass(SplineFont *sf, SplineChar *first, SplineChar *last, int *index, int allow_zero);
extern void KCD_DrawGlyph(GWindow pixmap,int x, int baseline, BDFChar *bdfc, int mag);
extern void KCLD_End(KernClassListDlg *kcld);
extern void KCLD_MvDetach(KernClassListDlg *kcld, MetricsView *mv);
extern void KernClassD(KernClass *kc, SplineFont *sf, int layer, int isv);
extern void KernPairD(SplineFont *sf, SplineChar *sc1, SplineChar *sc2, int layer, int isv);
extern void ME_ClassCheckUnique(GGadget *g, int r, int c, SplineFont *sf);
extern void ME_ListCheck(GGadget *g, int r, int c, SplineFont *sf);
extern void ME_SetCheckUnique(GGadget *g, int r, int c, SplineFont *sf);
extern void ShowKernClasses(SplineFont *sf, MetricsView *mv, int layer, int isv);

#endif /* FONTFORGE_FONTFORGEEXE_KERNCLASS_H */
