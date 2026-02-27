#ifndef FONTFORGE_CVEXPORT_H
#define FONTFORGE_CVEXPORT_H

#include "splinefont.h"
#include "sd.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int BCExportXBM(char *filename, BDFChar *bdfc, int format);
extern int ExportEPS(char *filename, SplineChar *sc, int layer);
extern int _ExportEPS(FILE *eps, SplineChar *sc, int layer, int preview);
extern int ExportFig(char *filename, SplineChar *sc, int layer);
extern int ExportGlif(char *filename, SplineChar *sc, int layer, int version);
extern int ExportImage(char *filename, SplineChar *sc, int layer, int format, int pixelsize, int bitsperpixel);
extern int ExportPDF(char *filename, SplineChar *sc, int layer);
extern int _ExportPDF(FILE *pdf, SplineChar *sc, int layer);
extern int ExportPlate(char *filename, SplineChar *sc, int layer);
extern int _ExportPlate(FILE *plate, SplineChar *sc, int layer);
extern int ExportSVG(char *filename, SplineChar *sc, int layer,
                     ExportParams *ep);
extern void ScriptExport(SplineFont *sf, BDFFont *bdf, int format, int gid,
                         char *format_spec, EncMap *map,ExportParams *ep);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_CVEXPORT_H */
