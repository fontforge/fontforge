#ifndef FONTFORGE_FONTFORGEEXE_CVGRIDFIT_H
#define FONTFORGE_FONTFORGEEXE_CVGRIDFIT_H

extern void CVFtPpemDlg(CharView *cv,int debug);
extern void CVGridFitChar(CharView *cv);
/**
 * If a live preview of grid fit is somehow in effect, call CVGridFitChar() for us.
 * A caller can call here after a change and any CVGridFitChar() will be updated if need be.
 */
extern void CVGridHandlePossibleFitChar(CharView *cv);
extern void SCDeGridFit(SplineChar *sc);
extern void SCReGridFit(SplineChar *sc,int layer);

#endif /* FONTFORGE_FONTFORGEEXE_CVGRIDFIT_H */
