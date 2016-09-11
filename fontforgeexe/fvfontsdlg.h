#ifndef FONTFORGE_FONTFORGEEXE_FVFONTSDLG_H
#define FONTFORGE_FONTFORGEEXE_FVFONTSDLG_H

extern GTextInfo *BuildFontList(FontView *except);
extern void FVInterpolateFonts(FontView *fv);
extern void FVMergeFonts(FontView *fv);
extern void TFFree(GTextInfo *tf);

#endif /* FONTFORGE_FONTFORGEEXE_FVFONTSDLG_H */
