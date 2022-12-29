#ifndef FONTFORGE_WOFF_H
#define FONTFORGE_WOFF_H

#include <fontforge-config.h>

#include "splinefont.h"

extern int WriteWOFFFont(char *fontname, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern int _WriteWOFFFont(FILE *woff, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern SplineFont *_SFReadWOFF(FILE *woff, int flags, enum openflags openflags, char *filename, char *chosenname, struct fontdict *fd);

#ifdef FONTFORGE_CAN_USE_WOFF2
extern int woff2_convert_ttf_to_woff2(const uint8_t *data, size_t length, uint8_t **result, size_t *result_length);
extern int woff2_convert_woff2_to_ttf(const uint8_t *data, size_t length, uint8_t **result, size_t *result_length);

extern int WriteWOFF2Font(char *fontname, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern int _WriteWOFF2Font(FILE *woff, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern SplineFont *_SFReadWOFF2(FILE *woff, int flags, enum openflags openflags, char *filename, char *chosenname, struct fontdict *fd);
#endif

#endif /* FONTFORGE_WOFF_H */
