#ifndef FONTFORGE_CVIMAGES_H
#define FONTFORGE_CVIMAGES_H

#include "baseviews.h"
#include "gimage.h"
#include "splinefont.h"
#include "sd.h"

extern GImage *ImageAlterClut(GImage *image);
extern int FVImportImages(FontViewBase *fv, char *path, int format, int toback, int flags);
extern int FVImportImageTemplate(FontViewBase *fv, char *path, int format, int toback, int flags);
extern void SCAddScaleImage(SplineChar *sc, GImage *image, int doclear, int layer);
extern void SCAppendEntityLayers(SplineChar *sc, Entity *ent);
extern void SCImportFig(SplineChar *sc, int layer, char *path, int doclear);
extern void SCImportGlif(SplineChar *sc, int layer, char *path, char *memory, int memlen, int doclear);
extern void SCImportPDFFile(SplineChar *sc, int layer, FILE *pdf, int doclear, int flags);
extern void SCImportPlateFile(SplineChar *sc, int layer, FILE *plate, int doclear);
extern void SCImportPSFile(SplineChar *sc, int layer, FILE *ps, int doclear, int flags);
extern void SCImportSVG(SplineChar *sc, int layer, char *path, char *memory, int memlen, int doclear);
extern void SCInsertImage(SplineChar *sc, GImage *image, real scale, real yoff, real xoff, int layer);

#endif /* FONTFORGE_CVIMAGES_H */
