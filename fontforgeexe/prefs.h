#ifndef FONTFORGE_FONTFORGEEXE_PREFS_H
#define FONTFORGE_FONTFORGEEXE_PREFS_H

extern void DoPrefs(void);
extern void DoXRes(void);
extern void GListAddStr(GGadget *list, unichar_t *str, void *ud);
extern void GListReplaceStr(GGadget *list, int index, unichar_t *str, void *ud);
extern void LastFonts_Save(void);
extern void PointerDlg(CharView *cv);
extern void Prefs_LoadDefaultPreferences(void);
extern void RecentFilesRemember(char *filename);
extern void RulerDlg(CharView *cv);

#endif /* FONTFORGE_FONTFORGEEXE_PREFS_H */
