#ifndef FONTFORGE_BVEDIT_H
#define FONTFORGE_BVEDIT_H

#include "baseviews.h"
#include "splinefont.h"

extern BDFChar *BDFGetMergedChar(BDFChar *bc);
extern BDFFloat *BDFFloatConvert(BDFFloat *sel, int todepth, int fromdepth);
extern BDFFloat *BDFFloatCopy(BDFFloat *sel);
extern BDFFloat *BDFFloatCreate(BDFChar *bc, int xmin, int xmax, int ymin, int ymax, int clear);
extern BDFFont *BitmapFontScaleTo(BDFFont *old, int to);
extern int BDFCharQuickBounds(BDFChar *bc, IBounds *bb, int8 xoff, int8 yoff, int use_backup, int first);
extern void BCExpandBitmapToEmBox(BDFChar *bc, int xmin, int ymin, int xmax, int ymax);
extern void BCFlattenFloat(BDFChar *bc);
extern void BCMakeDependent(BDFChar *dependent, BDFChar *base);
extern void BCMergeReferences(BDFChar *base, BDFChar *cur, int8 xoff, int8 yoff);
extern void BCPasteInto(BDFChar *bc, BDFChar *rbc, int ixoff, int iyoff, int invert, int cleartoo);
extern void BCPrepareForOutput(BDFChar *bc, int mergeall);
extern void BCRemoveDependent(BDFChar *dependent, BDFRefChar *ref);
extern void BCRestoreAfterOutput(BDFChar *bc);
extern void BCRotateCharForVert(BDFChar *bc, BDFChar *from, BDFFont *frombdf);
extern void BCSetPoint(BDFChar *bc, int x, int y, int color);
extern void BCTrans(BDFFont *bdf, BDFChar *bc, BVTFunc *bvts, FontViewBase *fv);
extern void BCTransFunc(BDFChar *bc, enum bvtools type, int xoff, int yoff);
extern void BCUnlinkThisReference(struct fontviewbase *fv, BDFChar *bc);
extern void BDFCharFindBounds(BDFChar *bc, IBounds *bb);
extern void BDFFloatFree(BDFFloat *sel);
extern void skewselect(BVTFunc *bvtf, real t);

#endif /* FONTFORGE_BVEDIT_H */
