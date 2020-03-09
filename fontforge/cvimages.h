#ifndef FONTFORGE_CVIMAGES_H
#define FONTFORGE_CVIMAGES_H

#include "baseviews.h"
#include "gimage.h"
#include "sd.h"
#include "splinefont.h"

extern GImage *ImageAlterClut(GImage *image);
extern int FVImportImages(FontViewBase *fv, char *path, int format, int toback,
                          bool preclear, ImportParams *ip);
extern int FVImportImageTemplate(FontViewBase *fv, char *path, int format,
                                 int toback, bool preclear, ImportParams *ip);
extern void SCAddScaleImage(SplineChar *sc, GImage *image, bool doclear,
                            int layer, ImportParams *ip);
extern void SCAppendEntityLayers(SplineChar *sc, Entity *ent,
                                 ImportParams *ip);
extern void SCImportPS(SplineChar *sc,int layer,char *path,bool doclear,
                       ImportParams *ip);
extern void SCImportPDF(SplineChar *sc,int layer,char *path,bool doclear,
                       ImportParams *ip);
extern void SCImportFig(SplineChar *sc, int layer, char *path, bool doclear,
                        ImportParams *ip);
extern void SCImportGlif(SplineChar *sc, int layer, char *path, char *memory,
                         int memlen, bool doclear, ImportParams *ip);
extern void SCImportPDFFile(SplineChar *sc, int layer, FILE *pdf, bool doclear,
                            ImportParams *ip);
extern void SCImportPlateFile(SplineChar *sc, int layer, FILE *plate,
                              bool doclear, ImportParams *ip);
extern void SCImportPSFile(SplineChar *sc, int layer, FILE *ps, bool doclear,
                           ImportParams *ip);
extern void SCImportSVG(SplineChar *sc, int layer, char *path, char *memory,
                        int memlen, bool doclear, ImportParams *ip);
extern void SCInsertImage(SplineChar *sc, GImage *image, real scale, real yoff, real xoff, int layer);

#endif /* FONTFORGE_CVIMAGES_H */
