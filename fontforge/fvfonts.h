#ifndef FONTFORGE_FVFONTS_H
#define FONTFORGE_FVFONTS_H

#include "baseviews.h"
#include "splinefont.h"

extern AnchorClass *MCConvertAnchorClass(struct sfmergecontext *mc, AnchorClass *ac);
extern BDFChar *BDFCharCopy(BDFChar *bc);
extern int SFCIDFindCID(SplineFont *sf, int unienc, const char *name);
extern int SFCIDFindExistingChar(SplineFont *sf, int unienc, const char *name);
extern int SFFindExistingSlot(SplineFont *sf, int unienc, const char *name);
extern int SFFindGID(SplineFont *sf, int unienc, const char *name);
extern int SFFindSlot(SplineFont *sf, EncMap *map, int unienc, const char *name);
extern int SFHasChar(SplineFont *sf, int unienc, const char *name);
extern int SFHasCID(SplineFont *sf, int cid);
extern PST *PSTCopy(PST *base, SplineChar *sc, struct sfmergecontext *mc);
extern RefChar *RefCharsCopy(RefChar *ref);
extern SplineChar *SFGetChar(SplineFont *sf, int unienc, const char *name);
extern SplineChar *SFGetOrMakeCharFromUnicodeBasic(SplineFont *sf, int ch);
extern SplineChar *SFHashName(SplineFont *sf, const char *name);
extern SplineChar *SplineCharCopy(SplineChar *sc, SplineFont *into, struct sfmergecontext *mc);
extern SplineChar *SplineCharInterpolate(SplineChar *base, SplineChar *other, real amount, SplineFont *newfont);
extern SplineFont *InterpolateFont(SplineFont *base, SplineFont *other, real amount, Encoding *enc);
extern SplineSet *SplineSetsInterpolate(SplineSet *base, SplineSet *other, real amount, SplineChar *sc);
extern struct altuni *AltUniCopy(struct altuni *altuni, SplineFont *noconflicts);
extern struct lookup_subtable *MCConvertSubtable(struct sfmergecontext *mc, struct lookup_subtable *sub);
extern void BitmapsCopy(SplineFont *to, SplineFont *from, int to_index, int from_index);
extern void GlyphHashFree(SplineFont *sf);
extern void __GlyphHashFree(struct glyphnamehash *hash);
extern void MergeFont(FontViewBase *fv, SplineFont *other, int preserveCrossFontKerning);
extern void SFFinishMergeContext(struct sfmergecontext *mc);
extern void SFHashGlyph(SplineFont *sf, SplineChar *sc);

#endif /* FONTFORGE_FVFONTS_H */
