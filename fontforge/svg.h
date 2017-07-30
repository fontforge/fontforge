#ifndef FONTFORGE_SVG_H
#define FONTFORGE_SVG_H

#include "sd.h"
#include "splinefont.h"

extern char **NamesReadSVG(char *filename);
extern Entity *EntityInterpretSVG(char *filename, char *memory, int memlen, int em_size, int ascent);
extern int _ExportSVG(FILE *svg, SplineChar *sc, int layer);
extern int HasSVG(void);
extern int SFFindOrder(SplineFont *sf);
extern int SFLFindOrder(SplineFont *sf, int layerdest);
extern int WriteSVGFont(const char *fontname, SplineFont *sf, enum fontformat format, int flags, EncMap *enc, int layer);
extern int _WriteSVGFont(FILE *file, SplineFont *sf, int flags, EncMap *enc, int layer);
extern SplineChar *SCHasSubs(SplineChar *sc, uint32 tag);
extern SplineFont *SFReadSVG(char *filename, int flags);
extern SplineFont *SFReadSVGMem(char *data, int flags);
extern SplineSet *SplinePointListInterpretSVG(char *filename, char *memory, int memlen, int em_size, int ascent, int is_stroked);
extern void SFLSetOrder(SplineFont *sf, int layerdest, int order2);
extern void SFSetOrder(SplineFont *sf, int order2);

#endif /* FONTFORGE_SVG_H */
