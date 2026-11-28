#ifndef FONTFORGE_SVG_H
#define FONTFORGE_SVG_H

#include "sd.h"
#include "splinefont.h"

#ifdef __cplusplus
#include <istream>

extern Entity* EntityInterpretSVG(std::istream& svg, int em_size, int ascent,
                                  bool scale);
extern SplineSet* SplinePointListInterpretSVG(std::istream& svg, int em_size,
                                              int ascent, int is_stroked,
                                              ImportParams* eip);
extern void SCDimensionFromSVGFile(std::istream& svg, SplineChar* sc,
                                   bool vert);

extern "C" {
#endif

extern char **NamesReadSVG(char *filename);
extern int _ExportSVG(FILE *svg, SplineChar *sc, int layer, ExportParams *ep);
extern int SFFindOrder(SplineFont *sf);
extern int SFLFindOrder(SplineFont *sf, int layerdest);
extern int WriteSVGFont(const char *fontname, SplineFont *sf, enum fontformat format, int flags, EncMap *enc, int layer);
extern int _WriteSVGFont(FILE *file, SplineFont *sf, int flags, EncMap *enc, int layer);
extern SplineChar *SCHasSubs(SplineChar *sc, uint32_t tag);
extern SplineFont *SFReadSVG(char *filename, int flags);
extern SplineFont *SFReadSVGMem(char *data, int flags);
extern void SFLSetOrder(SplineFont *sf, int layerdest, int order2);
extern void SFSetOrder(SplineFont *sf, int order2);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_SVG_H */
