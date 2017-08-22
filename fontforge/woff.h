#ifndef FONTFORGE_WOFF_H
#define FONTFORGE_WOFF_H

#include <stdio.h>

#include "splinefont.h"

extern int CanWoff(void);
extern int WriteWOFFFont(char *fontname, SplineFont *sf, enum fontformat format, int32 *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern int _WriteWOFFFont(FILE *woff, SplineFont *sf, enum fontformat format, int32 *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern SplineFont *_SFReadWOFF(FILE *woff, int flags, enum openflags openflags, char *filename, struct fontdict *fd);

#endif /* FONTFORGE_WOFF_H */
