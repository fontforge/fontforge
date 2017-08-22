#ifndef FONTFORGE_TOTTF_H
#define FONTFORGE_TOTTF_H

#include "splinefont.h"
#include "ttf.h"

extern char *utf8_verify_copy(const char *str);
extern int32 filechecksum(FILE *file);

extern int AlreadyMSSymbolArea(SplineFont *sf, EncMap *map);
extern int RefDepth(RefChar *ref, int layer);
extern int SFFigureDefWidth(SplineFont *sf, int *_nomwid);
extern int SFHasInstructions(SplineFont *sf);
extern int SSAddPoints(SplineSet *ss, int ptcnt, BasePoint *bp, char *flags);
extern int ttfcopyfile(FILE *ttf, FILE *other, int pos, const char *tab_name);
extern int WriteTTC(const char *filename, struct sflist *sfs, enum fontformat format, enum bitmapformat bf, int flags, int layer, enum ttc_flags ttcflags);
extern int WriteTTFFont(char *fontname, SplineFont *sf, enum fontformat format, int32 *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern int _WriteTTFFont(FILE *ttf, SplineFont *sf, enum fontformat format, int32 *bsizes, enum bitmapformat bf, int flags, EncMap *enc, int layer);
extern int _WriteType42SFNTS(FILE *type42, SplineFont *sf, enum fontformat format, int flags, EncMap *enc, int layer);
extern void cvt_unix_to_1904(long long time, int32 result[2]);
extern void DefaultTTFEnglishNames(struct ttflangname *dummy, SplineFont *sf);
extern void OS2FigureCodePages(SplineFont *sf, uint32 CodePage[2]);
extern void OS2FigureUnicodeRanges(SplineFont *sf, uint32 Ranges[4]);
extern void SFDefaultOS2Info(struct pfminfo *pfminfo, SplineFont *sf, char *fontname);
extern void SFDefaultOS2Simple(struct pfminfo *pfminfo, SplineFont *sf);
extern void SFDefaultOS2SubSuper(struct pfminfo *pfminfo, int emsize, double italic_angle);
extern void SFDummyUpCIDs(struct glyphinfo *gi, SplineFont *sf);

extern void putfixed(FILE *file, real dval);
extern void putlong(FILE *file, int val);
extern void putshort(FILE *file, int sval);

#endif /* FONTFORGE_TOTTF_H */
