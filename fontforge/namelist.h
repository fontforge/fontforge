#ifndef FONTFORGE_NAMELIST_H
#define FONTFORGE_NAMELIST_H

#include "splinefont.h"

extern char **AllGlyphNames(int uni, NameList *for_this_font, SplineChar *sc);
extern char **AllNamelistNames(void);
extern char **SFTemporaryRenameGlyphsToNamelist(SplineFont *sf, NameList *new);
extern const char *RenameGlyphToNamelist(char *buffer, SplineChar *sc, NameList *old, NameList *new, char **sofar);
extern const char *StdGlyphName(char *buffer, int uni, enum uni_interp interp, NameList *for_this_font);
extern int UniFromName(const char *name, enum uni_interp interp, Encoding *encname);
extern NameList *DefaultNameListForNewFonts(void);
extern NameList *LoadNamelist(char *filename);
extern NameList *NameListByName(const char *name);
extern void LoadNamelistDir(char *dir);
extern void SFRenameGlyphsToNamelist(SplineFont *sf, NameList *new);
extern void SFTemporaryRestoreGlyphNames(SplineFont *sf, char **former);

#endif /* FONTFORGE_NAMELIST_H */
