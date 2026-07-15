#ifndef FONTFORGE_DUMPPFA_H
#define FONTFORGE_DUMPPFA_H

#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *PSDictHasEntry(struct psdict *dict, const char *key);
extern double BlueScaleFigureForced(struct psdict *private_, real bluevalues[], real otherblues[]);
extern int PSBitmapDump(char *filename, BDFFont *font, EncMap *map);
extern int PSDictChangeEntry(struct psdict *dict, const char *key, const char *newval);
extern int PSDictFindEntry(struct psdict *dict, const char *key);
extern int PSDictRemoveEntry(struct psdict *dict, const char *key);
extern int PSDictSame(struct psdict *dict1, struct psdict *dict2);
extern int WritePSFont(char *fontname, SplineFont *sf, enum fontformat format, int flags, EncMap *map, SplineFont *fullsf, int layer);
extern int _WritePSFont(FILE *out, SplineFont *sf, enum fontformat format, int flags, EncMap *map, SplineFont *fullsf, int layer);
extern struct psdict *PSDictCopy(struct psdict *dict);
extern void SC_PSDump(void (*dumpchar)(int ch, void *data), void *data, SplineChar *sc, int refs_to_splines, int pdfopers, int layer);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_DUMPPFA_H */
