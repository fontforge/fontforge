#ifndef FONTFORGE_FONTFORGEEXE_NONLINEARTRANSUI_H
#define FONTFORGE_FONTFORGEEXE_NONLINEARTRANSUI_H

extern int PointOfViewDlg(struct pov_data *pov, SplineFont *sf, int flags);
extern void CVPointOfView(CharView *cv, struct pov_data *pov);
extern void NonLinearDlg(FontView *fv, CharView *cv);

#endif /* FONTFORGE_FONTFORGEEXE_NONLINEARTRANSUI_H */
