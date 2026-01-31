#ifndef FONTFORGE_AUTOSAVE_H
#define FONTFORGE_AUTOSAVE_H

#ifdef __cplusplus
extern "C" {
#endif

extern int DoAutoRecoveryExtended(int inquire);
extern void DoAutoSaves(void);
extern void CleanAutoRecovery(void);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_AUTOSAVE_H */
