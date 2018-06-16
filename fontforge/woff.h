#ifndef FONTFORGE_WOFF_H
#define FONTFORGE_WOFF_H

#include <stdio.h>

#include "splinefont.h"

extern int CanWoff(void);
extern int WriteWOFFFont(char *fontname, SplineFont *sf, enum fontformat format, int32 *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern int _WriteWOFFFont(FILE *woff, SplineFont *sf, enum fontformat format, int32 *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern SplineFont *_SFReadWOFF(FILE *woff, int flags, enum openflags openflags, char *filename, struct fontdict *fd);

#ifdef FONTFORGE_CAN_USE_WOFF2
extern int WriteWOFF2Font(char *fontname, SplineFont *sf, enum fontformat format, int32 *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern int _WriteWOFF2Font(FILE *woff, SplineFont *sf, enum fontformat format, int32 *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern SplineFont *_SFReadWOFF2(FILE *woff, int flags, enum openflags openflags, char *filename, struct fontdict *fd);
#endif

#endif /* FONTFORGE_WOFF_H */
