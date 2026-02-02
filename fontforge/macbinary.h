#ifndef FONTFORGE_MACBINARY_H
#define FONTFORGE_MACBINARY_H

#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char **NamesReadMacBinary(char *filename);
extern int LoadKerningDataFromMacFOND(SplineFont *sf, char *filename, EncMap *map);
extern int WriteMacBitmaps(char *filename, SplineFont *sf, int32_t *sizes, int is_dfont, EncMap *enc);
extern int WriteMacFamily(char *filename, struct sflist *sfs, enum fontformat format, enum bitmapformat bf, int flags, int layer);
extern int WriteMacPSFont(char *filename, SplineFont *sf, enum fontformat format, int flags, EncMap *enc, int layer);
extern int WriteMacTTFFont(char *filename, SplineFont *sf, enum fontformat format, int32_t *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern long mactime(void);
extern SplineChar *SFFindExistingCharMac(SplineFont *sf, EncMap *map, int unienc);
extern SplineFont *SFReadMacBinary(char *filename, int flags, enum openflags openflags);
extern uint16_t _MacStyleCode(const char *styles, SplineFont *sf, uint16_t *psstylecode);
extern void SfListFree(struct sflist *sfs);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_MACBINARY_H */
