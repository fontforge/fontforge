#ifndef FONTFORGE_AUTOSAVE_H
#define FONTFORGE_AUTOSAVE_H

#include "baseviews.h"

extern int DoAutoRecoveryExtended(int inquire);
extern void DoAutoSaves(void);
extern void CleanAutoRecovery(void);
extern void _DoAutoSaves(FontViewBase *fvs);

#endif /* FONTFORGE_AUTOSAVE_H */
