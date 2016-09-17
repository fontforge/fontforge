#ifndef FONTFORGE_LOOKUPS_H
#define FONTFORGE_LOOKUPS_H

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

#endif /* FONTFORGE_LOOKUPS_H */
