#ifndef FONTFORGE_FONTFORGEEXE_LOOKUPUI_H
#define FONTFORGE_FONTFORGEEXE_LOOKUPUI_H

extern char *GlyphNameListDeUnicode(char *str);
extern char *SCNameUniStr(SplineChar *sc);
extern char* SFDCreateUndoForLookup(SplineFont *sf, int lookup_type);
extern char *SFNameList2NameUni(SplineFont *sf, char *str);
extern GTextInfo *SFLookupArrayFromMask(SplineFont *sf, int mask);
extern GTextInfo *SFLookupArrayFromType(SplineFont *sf, int lookup_type);
extern GTextInfo **SFLookupListFromType(SplineFont *sf, int lookup_type);
extern GTextInfo *SFSubtableListOfType(SplineFont *sf, int lookup_type, int kernclass, int add_none);
extern GTextInfo **SFSubtablesOfType(SplineFont *sf, int lookup_type, int kernclass, int add_none);
extern int EditLookup(OTLookup *otl,int isgpos,SplineFont *sf);
int EditSubtable(struct lookup_subtable *sub, int isgpos, SplineFont *sf, struct subtable_data *sd, int def_layer);
extern int kernsLength(KernPair *kp);
extern struct lookup_subtable *SFNewLookupSubtableOfType(SplineFont *sf, int lookup_type, struct subtable_data *sd, int def_layer);
extern unichar_t **SFGlyphNameCompletion(SplineFont *sf, GGadget *t, int from_tab, int new_name_after_space);
extern unichar_t *uSCNameUniStr(SplineChar *sc);
extern void AddRmLang(SplineFont *sf, struct lkdata *lk, int add_lang);
extern void FVMassGlyphRename(FontView *fv);
void _LookupSubtableContents(SplineFont *sf, struct lookup_subtable *sub, struct subtable_data *sd, int def_layer);
extern void LookupUIInit(void);
extern void SFUntickAllPSTandKern(SplineFont *sf);

#endif /* FONTFORGE_FONTFORGEEXE_LOOKUPUI_H */
