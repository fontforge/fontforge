#ifndef FONTFORGE_LOOKUPS_H
#define FONTFORGE_LOOKUPS_H

#include "splinefont.h"
#include "uiinterface.h"

struct sllk {
	uint32 script;
	int cnt;
	int max;
	OTLookup **lookups;
	int lcnt;
	int lmax;
	uint32 *langs;
};

extern const char *lookup_type_names[2][10];
extern void SortInsertLookup(SplineFont *sf, OTLookup *newotl);
extern char *SuffixFromTags(FeatureScriptLangList *fl);

/**
 * Get the index into the char* array "firsts_or_seconds" that contains "name".
 * Or -1 if no such entry was found.
 *
 * This is handy when using KernClass* kc objects,
 * you can pass in kc->firsts and kc->first_cnt and the name of a glyph
 * to see if that char is contained in the first part of a pairing.
 *
 * This can be useful if you want to check if a digraph of chars
 * "ab" and "xy" are in the same cell of a KernClass or not (handled in the same class).
 * first do
 * int idxx = KernClassFindIndexContaining( kc->firsts,  kc->first_cnt,  "x" );
 * int idxy = KernClassFindIndexContaining( kc->seconds, kc->second_cnt, "y" );
 * 
 * then to test any diagrap, say "ab" h for being in the same cell:
 * int idxa = KernClassFindIndexContaining( kc->firsts,  kc->first_cnt,  "a" );
 * int idxb = KernClassFindIndexContaining( kc->seconds, kc->second_cnt, "b" );
 *
 * and they are in the same cell of the kernclass
 * if (idxx==idxa && idxy==idxb) {}
 * 
 */
extern int KernClassFindIndexContaining( char **firsts_or_seconds,
					 int firsts_or_seconds_size,
					 const char *name );

extern char *FPSTRule_From_Str(SplineFont *sf, FPST *fpst, struct fpst_rule *rule, char *line, int *return_is_warning);
extern char *FPSTRule_To_Str(SplineFont *sf, FPST *fpst, struct fpst_rule *rule);
extern char *reverseGlyphNames(char *str);
extern char *TagFullName(SplineFont *sf, uint32 tag, int ismac, int onlyifknown);
extern FeatureScriptLangList *FeatureListCopy(FeatureScriptLangList *fl);
extern FeatureScriptLangList *FindFeatureTagInFeatureScriptList(uint32 tag, FeatureScriptLangList *fl);
extern FeatureScriptLangList *FLOrder(FeatureScriptLangList *fl);
extern int DefaultLangTagInOneScriptList(struct scriptlanglist *sl);
extern int FeatureOrderId(int isgpos, FeatureScriptLangList *fl);
extern int _FeatureOrderId(int isgpos, uint32 tag);
extern int FeatureScriptTagInFeatureScriptList(uint32 feature, uint32 script, FeatureScriptLangList *fl);
extern int GlyphNameCnt(const char *pt);
extern int IsAnchorClassUsed(SplineChar *sc, AnchorClass *an);
extern int KCFindName(const char *name, char **classnames, int cnt, int allow_class0);
extern int KernClassContains(KernClass *kc, const char *name1, const char *name2, int ordered);
extern int LookupUsedNested(SplineFont *sf, OTLookup *checkme);
extern int PSTContains(const char *components, const char *name);
extern int ScriptInFeatureScriptList(uint32 script, FeatureScriptLangList *fl);
extern int VerticalKernFeature(SplineFont *sf, OTLookup *otl, int ask);
extern OTLookup *NewAALTLookup(SplineFont *sf, struct sllk *sllk, int sllk_cnt, int i);
extern OTLookup *OTLookupCopyInto(SplineFont *into_sf, SplineFont *from_sf, OTLookup *from_otl);
extern OTLookup *SFFindLookup(SplineFont *sf, char *name);
extern OTLookup **SFLookupsInScriptLangFeature(SplineFont *sf, int gpos, uint32 script, uint32 lang, uint32 feature);
extern SplineChar **SFGlyphsWithLigatureinLookup(SplineFont *sf, struct lookup_subtable *subtable);
extern SplineChar **SFGlyphsWithPSTinSubtable(SplineFont *sf, struct lookup_subtable *subtable);
extern struct lookup_subtable *SFFindLookupSubtableAndFreeName(SplineFont *sf, char *name);
extern struct lookup_subtable *SFSubTableFindOrMake(SplineFont *sf, uint32 tag, uint32 script, int lookup_type);
extern struct lookup_subtable *SFSubTableMake(SplineFont *sf, uint32 tag, uint32 script, int lookup_type);
extern struct opentype_str *ApplyTickedFeatures(SplineFont *sf, uint32 *flist, uint32 script, uint32 lang, int pixelsize, SplineChar **glyphs);
extern struct scriptlanglist *DefaultLangTagInScriptList(struct scriptlanglist *sl, int DFLT_ok);
extern struct scriptlanglist *SLCopy(struct scriptlanglist *sl);
extern struct scriptlanglist *SListCopy(struct scriptlanglist *sl);
extern struct sllk *AddOTLToSllks(OTLookup *otl, struct sllk *sllk, int *_sllk_cnt, int *_sllk_max);
extern uint32 *SFFeaturesInScriptLang(SplineFont *sf, int gpos, uint32 script, uint32 lang);
extern uint32 *SFLangsInScript(SplineFont *sf, int gpos, uint32 script);
extern uint32 *SFScriptsInLookups(SplineFont *sf, int gpos);
extern void AddNewAALTFeatures(SplineFont *sf);
extern void FF_SetFIInterface(struct fi_interface *fii);
extern void FListAppendScriptLang(FeatureScriptLangList *fl, uint32 script_tag, uint32 lang_tag);
extern void FListsAppendScriptLang(FeatureScriptLangList *fl, uint32 script_tag, uint32 lang_tag);
extern void FLMerge(OTLookup *into, OTLookup *from);
extern void LookupInit(void);
extern void NameOTLookup(OTLookup *otl, SplineFont *sf);
extern void OTLookupsCopyInto(SplineFont *into_sf, SplineFont *from_sf, OTLookup **from_list, OTLookup *before);
extern void SFFindClearUnusedLookupBits(SplineFont *sf);
extern void SFFindUnusedLookups(SplineFont *sf);
extern void SFGlyphRenameFixup(SplineFont *sf, const char *old, const char *new, int rename_related_glyphs);
extern void SFRemoveLookup(SplineFont *sf, OTLookup *otl, int remove_acs);
extern void SFRemoveLookupSubTable(SplineFont *sf, struct lookup_subtable *sub, int remove_acs);
extern void SFRemoveUnusedLookupSubTables(SplineFont *sf, int remove_incomplete_anchorclasses, int remove_unused_lookups);

extern void SFSubTablesMerge(SplineFont *_sf, struct lookup_subtable *subfirst, struct lookup_subtable *subsecond);
extern void SllkFree(struct sllk *sllk, int sllk_cnt);
extern void SLMerge(FeatureScriptLangList *into, struct scriptlanglist *fsl);

#endif /* FONTFORGE_LOOKUPS_H */
