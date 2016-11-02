#ifndef FONTFORGE_SPLINESAVEAFM_H
#define FONTFORGE_SPLINESAVEAFM_H

#include "splinefont.h"

extern const char *EncodingName(Encoding *map);
extern int AfmSplineFont(FILE *afm, SplineFont *sf, int formattype, EncMap *map, int docc, SplineFont *fullsf, int layer);
extern int AmfmSplineFont(FILE *amfm, MMSet *mm, int formattype, EncMap *map, int layer);
extern int CheckAfmOfPostScript(SplineFont *sf, char *psname);
extern int CIDWorthOutputting(SplineFont *cidmaster, int enc);
extern int doesGlyphExpandHorizontally(SplineChar *UNUSED(sc));
extern int LayerWorthOutputting(SplineFont *sf, int layer);
extern int LoadKerningDataFromAfm(SplineFont *sf, char *filename);
extern int LoadKerningDataFromAmfm(SplineFont *sf, char *filename);
extern int LoadKerningDataFromMetricsFile(SplineFont *sf, char *filename, EncMap *map);
extern int LoadKerningDataFromOfm(SplineFont *sf, char *filename, EncMap *map);
extern int LoadKerningDataFromPfm(SplineFont *sf, char *filename, EncMap *map);
extern int LoadKerningDataFromTfm(SplineFont *sf, char *filename, EncMap *map);
extern int OfmSplineFont(FILE *tfm, SplineFont *sf, EncMap *map, int layer);
extern int PfmSplineFont(FILE *pfm, SplineFont *sf, EncMap *map, int layer);
extern int SCDrawsSomethingOnLayer(SplineChar *sc, int layer);
extern int SCHasData(SplineChar *sc);
extern int SCLWorthOutputtingOrHasData(SplineChar *sc, int layer);
extern int SCWorthOutputting(SplineChar *sc);
extern int SFFindNotdef(SplineFont *sf, int fixed);
extern int TfmSplineFont(FILE *tfm, SplineFont *sf, EncMap *map, int layer);
extern void PosNew(SplineChar *to, int tag, int dx, int dy, int dh, int dv);
extern void SFKernClassTempDecompose(SplineFont *sf, int isv);
extern void SFKernCleanup(SplineFont *sf, int isv);
extern void SFLigatureCleanup(SplineFont *sf);
extern void SFLigaturePrepare(SplineFont *sf);
extern void SubsNew(SplineChar *to, enum possub_type type, int tag, char *components, SplineChar *default_script);
extern void TeXDefaultParams(SplineFont *sf);

#endif /* FONTFORGE_SPLINESAVEAFM_H */
