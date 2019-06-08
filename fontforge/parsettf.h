#ifndef FONTFORGE_PARSETTF_H
#define FONTFORGE_PARSETTF_H

#include <fontforge-config.h>

#include "splinefont.h"

extern char *EnforcePostScriptName(char *old);
extern char **NamesReadCFF(char *filename);
extern char **NamesReadTTF(char *filename);
extern char *TTFGetFontName(FILE *ttf, int32 offset, int32 off2);
extern int MSLanguageFromLocale(void);
extern int ttfFindPointInSC(SplineChar *sc, int layer, int pnum, BasePoint *pos, RefChar *bound);
extern int ttfFixupRef(SplineChar **chars, int i);
extern SplineFont *CFFParse(char *filename);
extern SplineFont *_CFFParse(FILE *temp, int len, char *fontsetname);
extern SplineFont *SFReadTTF(char *filename, int flags, enum openflags openflags);
extern SplineFont *_SFReadTTF(FILE *ttf, int flags, enum openflags openflags, char *filename, char *chosenname, struct fontdict *fd);
extern struct otfname *FindAllLangEntries(FILE *ttf, struct ttfinfo *info, int id);
extern void AltUniFigure(SplineFont *sf, EncMap *map, int check_dups);
extern void TTF_PSDupsDefault(SplineFont *sf);

#endif /* FONTFORGE_PARSETTF_H */
