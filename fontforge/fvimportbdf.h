#ifndef FONTFORGE_FVIMPORTBDF_H
#define FONTFORGE_FVIMPORTBDF_H

#include "baseviews.h"
#include "splinefont.h"

extern int FVImportBDF(FontViewBase *fv, char *filename, int ispk, int toback);
extern int FVImportMult(FontViewBase *fv, char *filename, int toback, int bf);
extern SplineFont *SFFromBDF(char *filename, int ispk, int toback);
extern void SFCheckPSBitmap(SplineFont *sf);
extern void SFDefaultAscent(SplineFont *sf);
extern void SFSetFontName(SplineFont *sf, char *family, char *mods, char *fullname);

#endif /* FONTFORGE_FVIMPORTBDF_H */
