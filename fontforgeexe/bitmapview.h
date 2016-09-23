#ifndef FONTFORGE_FONTFORGEEXE_BITMAPVIEW_H
#define FONTFORGE_FONTFORGEEXE_BITMAPVIEW_H

extern BitmapView *BitmapViewCreate(BDFChar *bc, BDFFont *bdf, FontView *fv, int enc);
extern BitmapView *BitmapViewCreatePick(int enc, FontView *fv);
extern int BVColor(BitmapView *bv);
extern void BCGeneralFunction(BitmapView *bv,
	void (*SetPoint)(BitmapView *,int x, int y, void *data),void *data);
extern void BitmapViewFinishNonStatic();
extern void BitmapViewFree(BitmapView *bv);
extern void BVChangeBC(BitmapView *bv, BDFChar *bc, int fitit );
extern void BVChar(BitmapView *bv, GEvent *event );
extern void BVMenuRotateInvoked(GWindow gw,struct gmenuitem *mi,GEvent *g);
extern void BVRotateBitmap(BitmapView *bv,enum bvtools type );

#endif /* FONTFORGE_FONTFORGEEXE_BITMAPVIEW_H */
