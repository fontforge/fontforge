#ifndef FONTFORGE_SPLINEUTIL_H
#define FONTFORGE_SPLINEUTIL_H

#include "psfont.h"
#include "splinefont.h"
#include "ttfinstrs.h"
#include "views.h"

extern AnchorClass *SFFindOrAddAnchorClass(SplineFont *sf, char *name, struct lookup_subtable *sub);
extern AnchorPoint *AnchorPointsCopy(AnchorPoint *alist);
extern AnchorPoint *APAnchorClassMerge(AnchorPoint *anchors, AnchorClass *into, AnchorClass *from);
extern bigreal DistanceBetweenPoints(BasePoint *p1, BasePoint *p2);
extern bigreal SplineCurvature(Spline *s, bigreal t);
extern bigreal SplineMinDistanceToPoint(Spline *s, BasePoint *p);

extern size_t count_caps(const char * input);
extern char *upper_case(const char * input);
extern char *same_case(const char * input);
extern char *delimit_null(const char * input, char delimiter);

extern char *strconcat3(const char *str1, const char *str2, const char *str3);
extern char **StringExplode(const char *input, char delimiter);
extern char *XUIDFromFD(int xuid[20]);
extern DeviceTable *DeviceTableCopy(DeviceTable *orig);
extern double CheckExtremaForSingleBitErrors(const Spline1D *sp, double t, double othert);
extern DStemInfo *DStemInfoCopy(DStemInfo *h);
extern EncMap *EncMap1to1(int enccount);
extern EncMap *EncMapCopy(EncMap *map);
extern EncMap *EncMapNew(int enccount, int backmax, Encoding *enc);

/* Uses an iterative approximation */
extern extended IterateSplineSolve(const Spline1D *sp, extended tmin, extended tmax, extended sought_y);
/* Uses an iterative approximation and then tries to fix things up */
extern extended IterateSplineSolveFixup(const Spline1D *sp, extended tmin, extended tmax, extended sought_y);

extern FPST *FPSTCopy(FPST *fpst);
extern HintMask *HintMaskFromTransformedRef(RefChar *ref, BasePoint *trans, SplineChar *basesc,HintMask *hm);
extern ImageList *ImageListCopy(ImageList *cimg);
extern ImageList *ImageListTransform(ImageList *img, real transform[6], int everything);
extern int CountKerningClasses(SplineFont *sf);
extern int _CubicSolve(const Spline1D *sp, bigreal sought, extended ts[3]);
extern int GroupNameType(const char *input);
extern int IntersectLines(BasePoint *inter, BasePoint *line1_1, BasePoint *line1_2, BasePoint *line2_1, BasePoint *line2_2);
extern int IntersectLinesClip(BasePoint *inter, BasePoint *line1_1, BasePoint *line1_2, BasePoint *line2_1, BasePoint *line2_2);
extern int IntersectLinesSlopes(BasePoint *inter, BasePoint *line1, BasePoint *slope1, BasePoint *line2, BasePoint *slope2);
extern int LineTangentToSplineThroughPt(Spline *s, BasePoint *pt, extended ts[4], extended tmin, extended tmax);
extern int NearSpline(FindSel *fs, Spline *spline);
extern int SCRoundToCluster(SplineChar *sc, int layer, int sel, bigreal within, bigreal max);
extern int SFKerningGroupExistsSpecific(const struct splinefont *sf, const char *groupname, int isv, int isr);
extern int SpExistsInSS(SplinePoint *sp, SplineSet *ss);
extern int Spline2DFindExtrema(const Spline *sp, extended extrema[4]);
extern int Spline2DFindPointsOfInflection(const Spline *sp, extended poi[2]);
extern int SplineAtInflection(Spline1D *sp, bigreal t);
extern int SplineAtMinMax(Spline1D *sp, bigreal t);
extern int SplineExistsInSS(Spline *s, SplineSet *ss);
extern int SplinePointIsACorner(SplinePoint *sp);
extern int SplineSetIntersect(SplineSet *spl, Spline **_spline, Spline **_spline2);
extern int SplineSetsRemoveAnnoyingExtrema(SplineSet *ss, bigreal err);

/* Two lines intersect in at most 1 point */
/* Two quadratics intersect in at most 4 points */
/* Two cubics intersect in at most 9 points */ /* Plus an extra space for a trailing -1 */
extern int SplinesIntersect(const Spline *s1, const Spline *s2, BasePoint pts[9], extended t1s[10], extended t2s[10]);

extern int SplineT2SpiroIndex(Spline *spline, bigreal t, SplineSet *spl);
extern int SSBoundsWithin(SplineSet *ss, bigreal z1, bigreal z2, bigreal *wmin, bigreal *wmax, int major);
extern int SSExistsInLayer(SplineSet *ss, SplineSet *lots);
extern int SSHasClip(SplineSet *ss);
extern int SSHasDrawn(SplineSet *ss);
extern int SSPointWithin(SplineSet *spl, BasePoint *pt);
extern int StringInStrings(char const* const* space, int length, const char *target);
extern KernClass *KernClassCopy(KernClass *kc);
extern LinearApprox *SplineApproximate(Spline *spline, real scale);
extern MinimumDistance *MinimumDistanceCopy(MinimumDistance *md);
extern real SplineNearPoint(Spline *spline, BasePoint *bp, real fudge);
extern RefChar *RefCharCreate(void);
extern SplineChar *SFSplineCharCreate(SplineFont *sf);
extern SplineFont *SplineFontFromPSFont(FontDict *fd);
extern SplinePointList *SPLCopyTransformedHintMasks(RefChar *r, SplineChar *basesc, BasePoint *trans,int layer);
extern SplinePointList *SPLCopyTranslatedHintMasks(SplinePointList *base, SplineChar *basesc, SplineChar *subsc, BasePoint *trans);
extern SplinePointList *SplinePointListCopy1(const SplinePointList *spl);
extern SplinePointList *SplinePointListCopySelected(SplinePointList *base);
extern SplinePointList *SplinePointListCopySpiroSelected(SplinePointList *base);
extern SplinePointList *SplinePointListRemoveSelected(SplineChar *sc, SplinePointList *base);
extern SplinePointList *SplinePointListShift(SplinePointList *base, real xoff, enum transformPointType allpoints);
extern SplinePointList *SplinePointListSpiroTransform(SplinePointList *base, real transform[6], int allpoints);
extern SplinePointList *SplinePointListTransformExtended(SplinePointList *base, real transform[6], enum transformPointType tpt, enum transformPointMask tpmask);

extern SplinePoint *SplineBisect(Spline *spline, extended t);
extern SplinePoint *SplinePointCreate(real x, real y);
extern SplineSet *LayerAllSplines(Layer *layer);
extern SplineSet *LayerUnAllSplines(Layer *layer);
extern Spline *SplineMake3(SplinePoint *from, SplinePoint *to);
extern Spline *SplineSplit(Spline *spline, extended ts[3]);
extern struct baselangextent *BaseLangCopy(struct baselangextent *extent);
extern struct ff_glyphclasses *SFGetGroup(const struct splinefont *sf, int index, const char *name);
extern struct glyphvariants *GlyphVariantsCopy(struct glyphvariants *gv);
extern struct gradient *GradientCopy(struct gradient *old, real transform[6]);
extern struct jstf_lang *JstfLangsCopy(struct jstf_lang *jl);
extern struct mathkern *MathKernCopy(struct mathkern *mk);
extern struct pattern *PatternCopy(struct pattern *old, real transform[6]);
extern ValDevTab *ValDevTabCopy(ValDevTab *orig);
extern void AltUniFree(struct altuni *altuni);
extern void AnchorClassesFree(AnchorClass *an);
extern void AnchorPointsFree(AnchorPoint *ap);
extern void ApTransform(AnchorPoint *ap, real transform[6]);
extern void ASMFree(ASM *sm);
extern void BaseFree(struct Base *base);
extern void BaseLangFree(struct baselangextent *extent);
extern void BaseScriptFree(struct basescript *bs);
extern void BpTransform(BasePoint *to, BasePoint *from, real transform[6]);
extern void BrushCopy(struct brush *into, struct brush *from, real transform[6]);
extern void CIDLayerFindBounds(SplineFont *cidmaster, int layer, DBounds *bounds);
extern void DeviceTableFree(DeviceTable *dt);
extern void DeviceTableSet(DeviceTable *adjust, int size, int correction);
extern void DStemInfoFree(DStemInfo *h);
extern void DStemInfosFree(DStemInfo *h);
extern void EncMapFree(EncMap *map);
extern void ExplodedStringFree(char **input);
extern void FeatureScriptLangListFree(FeatureScriptLangList *fl);
extern void FPSTClassesFree(FPST *fpst);
extern void FPSTFree(FPST *fpst);
extern void FPSTRuleContentsFree(struct fpst_rule *r, enum fpossub_format format);
extern void FPSTRulesFree(struct fpst_rule *r, enum fpossub_format format, int rcnt);
extern void GlyphGroupFree(struct ff_glyphclasses* group);
extern void GlyphGroupKernFree(struct ff_rawoffsets* groupkern);
extern void GlyphGroupKernsFree(struct ff_rawoffsets* root);
extern void GlyphGroupsFree(struct ff_glyphclasses* root);
extern void GlyphVariantsFree(struct glyphvariants *gv);
extern void GradientFree(struct gradient *grad);
extern void GrowBufferAdd(GrowBuf *gb, int ch);
extern void GrowBufferAddStr(GrowBuf *gb, char *str);
extern void ImageListsFree(ImageList *imgs);
extern void JstfLangFree(struct jstf_lang *jl);
extern void JustifyFree(Justify *just);
extern void KernClassClearSpecialContents(KernClass *kc);
extern void KernClassFreeContents(KernClass *kc);
extern void KernClassListClearSpecialContents(KernClass *kc);
extern void KernClassListFree(KernClass *kc);
extern void KernPairsFree(KernPair *kp);
extern void LayerDefault(Layer *layer);
extern void LayerFreeContents(SplineChar *sc, int layer);
extern void LinearApproxFree(LinearApprox *la);
extern void LineListFree(LineList *ll);
extern void MacFeatListFree(MacFeat *mf);
extern void MacNameListFree(struct macname *mn);
extern void MacSettingListFree(struct macsetting *ms);
extern void MarkClassFree(int cnt, char **classes, char **names);
extern void MarkSetFree(int cnt, char **classes, char **names);
extern void MathKernFree(struct mathkern *mk);
extern void MathKernVContentsFree(struct mathkernvertex *mk);
extern void MinimumDistancesFree(MinimumDistance *md);
extern void MMSetClearSpecial(MMSet *mm);
extern void MMSetFreeContents(MMSet *mm);
extern void OtfFeatNameListFree(struct otffeatname *fn);
extern void OtfNameListFree(struct otfname *on);
extern void OTLookupFree(OTLookup *lookup);
extern void OTLookupListFree(OTLookup *lookup);
extern void PatternFree(struct pattern *pat);
extern void PenCopy(struct pen *into, struct pen *from, real transform[6]);
extern void RefCharFindBounds(RefChar *rf);
extern void RefCharFree(RefChar *ref);
extern void RefCharsFree(RefChar *ref);
extern void SCCategorizePoints(SplineChar *sc);
extern void SCMakeDependent(SplineChar *dependent, SplineChar *base);
extern void SCRefToSplines(SplineChar *sc, RefChar *rf, int layer);
extern void SCReinstanciateRefChar(SplineChar *sc, RefChar *rf, int layer);
extern void SCRemoveDependent(SplineChar *dependent, RefChar *rf, int layer);
extern void SCRemoveDependents(SplineChar *dependent);
extern void SCRemoveLayerDependents(SplineChar *dependent, int layer);
extern void SFInstanciateRefs(SplineFont *sf);
extern void SFReinstanciateRefs(SplineFont *sf);
extern void SFRemoveAnchorClass(SplineFont *sf, AnchorClass *an);
extern void SFRemoveSavedTable(SplineFont *sf, uint32 tag);
extern void SPLCategorizePointsKeepCorners(SplinePointList *spl);
extern void SplineCharFindBounds(SplineChar *sc, DBounds *bounds);
extern void SplineCharFreeContents(SplineChar *sc);
extern void SplineCharLayerFindBounds(SplineChar *sc, int layer, DBounds *bounds);
extern void SplineCharLayerQuickBounds(SplineChar *sc, int layer, DBounds *bounds);
extern void SplineCharListsFree(struct splinecharlist *dlist);
extern void SplineCharQuickBounds(SplineChar *sc, DBounds *b);
extern void SplineCharQuickConservativeBounds(SplineChar *sc, DBounds *b);
extern void SplineFindExtrema(const Spline1D *sp, extended *_t1, extended *_t2);
extern void SplineFontClearSpecial(SplineFont *sf);
extern void SplineFontFindBounds(SplineFont *sf, DBounds *bounds);
extern void SplineFontFree(SplineFont *sf);
extern void SplineFontLayerFindBounds(SplineFont *sf, int layer, DBounds *bounds);
extern void SplineFontQuickConservativeBounds(SplineFont *sf, DBounds *b);
extern void SplineFree(Spline *spline);
extern void SplinePointCategorize(SplinePoint *sp);
extern void SplinePointFree(SplinePoint *sp);
extern void SplinePointListFree(SplinePointList *spl);
extern void SplinePointListMDFree(SplineChar *sc, SplinePointList *spl);
extern void SplinePointListSelect(SplinePointList *spl, int sel);
extern void SplinePointListsFree(SplinePointList *spl);
extern void SplinePointListsMDFree(SplineChar *sc, SplinePointList *spl);
extern void SplinePointMDFree(SplineChar *sc, SplinePoint *sp);
extern void SplinePointsFree(SplinePointList *spl);
extern void SplineRemoveExtremaTooClose(Spline1D *sp, extended *_t1, extended *_t2);
extern void SplineSetBeziersClear(SplinePointList *spl);
extern void SplineSetFindBounds(const SplinePointList *spl, DBounds *bounds);
extern void SplineSetQuickBounds(SplineSet *ss, DBounds *b);
extern void SplineSetQuickConservativeBounds(SplineSet *ss, DBounds *b);
extern void SplineSetSpirosClear(SplineSet *spl);
extern void TTFLangNamesFree(struct ttflangname *l);
extern void TtfTablesFree(struct ttf_table *tab);
extern void ValDevFree(ValDevTab *adjust);

#ifdef FF_UTHASH_GLIF_NAMES
struct glif_name_index;
extern int KerningClassSeekByAbsoluteIndex(const struct splinefont *sf, int seek_index, struct kernclass **okc, int *oisv, int *oisr, int *ooffset);
extern int HashKerningClassNames(SplineFont *sf, struct glif_name_index * class_name_hash);
extern int HashKerningClassNamesCaps(SplineFont *sf, struct glif_name_index * class_name_hash);
extern int HashKerningClassNamesFlex(SplineFont *sf, struct glif_name_index * class_name_hash, int capitalize);
#endif /* FF_UTHASH_GLIF_NAMES */

#endif /* FONTFORGE_SPLINEUTIL_H */
