#ifndef FONTFORGE_FONTFORGEEXE_CVPOINTER_H
#define FONTFORGE_FONTFORGEEXE_CVPOINTER_H

extern BasePoint nearest_point_on_line_segment(BasePoint p1, BasePoint p2, BasePoint p3);

/**
 * True if the spline with from/to is part of the guide splines.
 *
 * Handy for telling if the user has just clicked on a guide for example,
 * you might want to also check the active layer first with cv->b.drawmode == dm_grid
 */
extern bool isSplinePointPartOfGuide(SplineFont *sf, SplinePoint *sp);

/**
 * Get all the selected points in the current cv.
 * Caller must g_list_free() the returned value.
 */
extern GList_Glib* CVGetSelectedPoints(CharView *cv);

extern int CVAllSelected(CharView *cv);
extern int CVAnySel(CharView *cv, int *anyp, int *anyr, int *anyi, int *anya);
extern int CVAnySelPoints(CharView *cv);
extern int CVClearSel(CharView *cv);
extern int CVMouseMovePointer(CharView *cv, GEvent *event);
extern int CVMoveSelection(CharView *cv, real dx, real dy, uint32 input_state);
extern int CVNearLBearingLine(CharView* cv, real x, real fudge);
extern int CVNearRBearingLine(CharView* cv, real x, real fudge);
extern int CVSetSel(CharView *cv,int mask);

/**
 * If isTState > 0 then CVPreserveTState(cv)
 * otherwise CVPreserveState(cv)
 */
extern Undoes *CVPreserveMaybeState(CharView *cv, int isTState);

extern Undoes *CVPreserveTState(CharView *cv);
extern void CVAdjustControl(CharView *cv,BasePoint *cp, BasePoint *to);
extern void CVCheckResizeCursors(CharView *cv);
extern void CVFindCenter(CharView *cv, BasePoint *bp, int nosel);
extern void CVInvertSel(CharView *cv);
extern void CVMouseDownPointer(CharView *cv, FindSel *fs, GEvent *event);
extern void CVMouseUpPointer(CharView *cv);
extern void CVRestoreTOriginalState(CharView *cv);
extern void CVSelectPointAt(CharView *cv);
extern void CVUndoCleanup(CharView *cv);

/**
 * Unselect all the BCP which are currently selected.
 */
extern void CVUnselectAllBCP(CharView *cv);

#endif /* FONTFORGE_FONTFORGEEXE_CVPOINTER_H */
