#ifndef FONTFORGE_CVUNDOES_H
#define FONTFORGE_CVUNDOES_H

#include "baseviews.h"
#include "splinefont.h"

extern int no_windowing_ui, maxundoes;

/**
 * Serialize and undo into a string.
 * You must free() the returned string.
 */
extern char* UndoToString(SplineChar* sc, Undoes *undo);

extern const Undoes *CopyBufferGet(void);
extern enum undotype CopyUndoType(void);
extern int CopyContainsBitmap(void);
extern int CopyContainsSomething(void);
extern int CopyContainsVectors(void);
extern int CVLayer(CharViewBase *cv);
extern int getAdobeEnc(const char *name);
extern int SCDependsOnSC(SplineChar *parent, SplineChar *child);
extern int SCWasEmpty(SplineChar *sc, int skip_this_layer);
extern RefChar *CopyContainsRef(SplineFont *sf);
extern RefChar *RefCharsCopyState(SplineChar *sc, int layer);
extern SplineSet *ClipBoardToSplineSet(void);
extern Undoes *BCPreserveState(BDFChar *bc);
extern Undoes *CVPreserveState(CharViewBase *cv);
extern Undoes *CVPreserveStateHints(CharViewBase *cv);
extern Undoes *_CVPreserveTState(CharViewBase *cv, PressedOn *p);
extern Undoes *CVPreserveVWidth(CharViewBase *cv, int vwidth);
extern Undoes *CVPreserveWidth(CharViewBase *cv, int width);
extern Undoes *SCPreserveBackground(SplineChar *sc);
extern Undoes *SCPreserveHints(SplineChar *sc, int layer);
extern Undoes *_SCPreserveLayer(SplineChar *sc, int layer, int dohints);
extern Undoes *SCPreserveLayer(SplineChar *sc, int layer, int dohints);
extern Undoes *SCPreserveState(SplineChar *sc, int dohints);
extern Undoes *SCPreserveVWidth(SplineChar *sc);
extern Undoes *SCPreserveWidth(SplineChar *sc);
extern Undoes *_SFPreserveGuide(SplineFont *sf);
extern Undoes *SFPreserveGuide(SplineFont *sf);
extern void BCCopyReference(BDFChar *bc, int pixelsize, int depth);
extern void BCCopySelected(BDFChar *bc, int pixelsize, int depth);
extern void BCDoRedo(BDFChar *bc);
extern void BCDoUndo(BDFChar *bc);
extern void ClipboardClear(void);
extern void CopyBufferClearCopiedFrom(SplineFont *dying);
extern void CopyBufferFree(void);
extern void CopyWidth(CharViewBase *cv, enum undotype ut);
extern void CVCopyGridFit(CharViewBase *cv);
extern void CVDoRedo(CharViewBase *cv);
extern void CVDoUndo(CharViewBase *cv);
extern void CVRemoveTopUndo(CharViewBase *cv);
extern void _CVRestoreTOriginalState(CharViewBase *cv, PressedOn *p);
extern void _CVUndoCleanup(CharViewBase *cv, PressedOn *p);

/**
  * Dump a list of undos for a splinechar starting at the given 'undo'.
  * msg is used as a header message so that a dump at a particular time stands
  * out from one that occurs later in the code.
  */
extern void dumpUndoChain(char* msg, SplineChar* sc, Undoes *undo);

extern void ExtractHints(SplineChar *sc, void *hints, int docopy);
extern void FVCopyAnchors(FontViewBase *fv);
extern void FVCopyWidth(FontViewBase *fv, enum undotype ut);
extern void FVCopy(FontViewBase *fv, enum fvcopy_type copytype);
extern void MVCopyChar(FontViewBase *fv, BDFFont *mvbdf, SplineChar *sc, enum fvcopy_type fullcopy);
extern void PasteAnchorClassMerge(SplineFont *sf, AnchorClass *into, AnchorClass *from);
extern void PasteIntoFV(FontViewBase *fv, int pasteinto, real trans[6]);
extern void PasteIntoMV(FontViewBase *fv, BDFFont *mvbdf, SplineChar *sc, int doclear);
extern void PasteRemoveAnchorClass(SplineFont *sf, AnchorClass *dying);
extern void PasteRemoveSFAnchors(SplineFont *sf);
extern void PasteToBC(BDFChar *bc, int pixelsize, int depth);
extern void PasteToCV(CharViewBase *cv);
extern int  SCClipboardHasPasteableContents(void);
extern void SCCopyLookupData(SplineChar *sc);
extern void SCCopyWidth(SplineChar *sc, enum undotype ut);
extern void SCDoRedo(SplineChar *sc, int layer);
extern void SCDoUndo(SplineChar *sc, int layer);
extern void SCUndoSetLBearingChange(SplineChar *sc, int lbc);
extern void *UHintCopy(SplineChar *sc, int docopy);
extern void UndoesFreeButRetainFirstN(Undoes** undopp, int retainAmount);

#endif /* FONTFORGE_CVUNDOES_H */
