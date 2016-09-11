#ifndef FONTFORGE_FONTFORGEEXE_CVGETINFO_H
#define FONTFORGE_FONTFORGEEXE_CVGETINFO_H

typedef struct gidata {
	 struct dlistnode ln;
	 CharView *cv;
	 SplineChar *sc;
	 RefChar *rf;
	 ImageList *img;
	 AnchorPoint *ap;
	 SplinePoint *cursp;
	 spiro_cp *curcp;
	 SplinePointList *curspl;
	 SplinePointList *oldstate;
	 AnchorPoint *oldaps;
	 GWindow gw;
	 int done, first, changed;
	 int prevchanged, nextchanged;
	 int normal_start, normal_end;
	 int interp_start, interp_end;
	 GGadgetCreateData* gcd;
	 GGadget *group1ret, *group2ret;
	 int nonmodal;
} GIData;

extern AnchorClass *AnchorClassUnused(SplineChar *sc,int *waslig);
extern GTextInfo *SCHintList(SplineChar *sc,HintMask *hm);
extern int SCUsedBySubs(SplineChar *sc);
extern void ApGetInfo(CharView *cv, AnchorPoint *ap);
extern void CVGetInfo(CharView *cv);
extern void CVPGetInfo(CharView *cv);
extern void PIChangePoint(GIData *ci);
extern void PI_Destroy(struct dlistnode *node);
extern void PI_ShowHints(SplineChar *sc, GGadget *list, int set);
extern void SCRefBy(SplineChar *sc);
extern void SCSubBy(SplineChar *sc);

#endif /* FONTFORGE_FONTFORGEEXE_CVGETINFO_H */
