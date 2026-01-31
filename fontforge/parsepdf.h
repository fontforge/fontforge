#ifndef FONTFORGE_PARSEPDF_H
#define FONTFORGE_PARSEPDF_H

#include "sd.h"
#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char **NamesReadPDF(char *filename);
extern Entity *EntityInterpretPDFPage(FILE *pdf, int select_page);
extern SplineFont *SFReadPdfFont(char *filename, enum openflags openflags);
extern SplineFont *_SFReadPdfFont(FILE *pdf, char *filename, enum openflags openflags);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_PARSEPDF_H */
