#ifndef FONTFORGE_FONTFORGEEXE_CVHINTS_H
#define FONTFORGE_FONTFORGEEXE_CVHINTS_H

extern void CVCreateHint(CharView *cv,int ishstem,int preservehints);
extern void CVReviewHints(CharView *cv);
extern void SCRemoveSelectedMinimumDistances(SplineChar *sc,int inx);

#endif /* FONTFORGE_FONTFORGEEXE_CVHINTS_H */
