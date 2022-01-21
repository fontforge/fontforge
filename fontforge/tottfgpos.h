#ifndef FONTFORGE_TOTTFGPOS_H
#define FONTFORGE_TOTTFGPOS_H

#include "splinefont.h"
#include "ttf.h"

/* Open type Advanced Typography Tables */
extern void otf_dumpgpos(struct alltabs *at, SplineFont *sf);
extern void otf_dumpgsub(struct alltabs *at, SplineFont *sf);
extern void otf_dumpgdef(struct alltabs *at, SplineFont *sf);
extern void otf_dumpbase(struct alltabs *at, SplineFont *sf);
extern void otf_dumpjstf(struct alltabs *at, SplineFont *sf);
extern void otf_dump_dummydsig(struct alltabs *at, SplineFont *sf);
extern int gdefclass(SplineChar *sc);

extern int SCRightToLeft(SplineChar *sc);
extern int ScriptIsRightToLeft(uint32_t script);
extern SplineChar **EntryExitDecompose(SplineFont *sf, AnchorClass *ac, struct glyphinfo *gi);
extern struct otffeatname *findotffeatname(uint32_t tag, SplineFont *sf);
extern uint32_t ScriptFromUnicode(uint32_t u, SplineFont *sf);
extern uint32_t SCScriptFromUnicode(SplineChar *sc);
extern void AnchorClassDecompose(SplineFont *sf, AnchorClass *_ac, int classcnt, int *subcnts, SplineChar ***marks, SplineChar ***base, SplineChar ***lig, SplineChar ***mkmk, struct glyphinfo *gi);
extern void ScriptMainRange(uint32_t script, int *start, int *end);
extern void SFBaseSort(SplineFont *sf);

/* Used by both otf and apple */
extern int LigCaretCnt(SplineChar *sc);
extern uint16_t *ClassesFromNames(SplineFont *sf, char **classnames, int class_cnt, int numGlyphs, SplineChar ***glyphs, int apple_kc);
extern SplineChar **SFGlyphsFromNames(SplineFont *sf, char *names);

/* The MATH table */
extern void otf_dump_math(struct alltabs *at, SplineFont *sf);

#endif /* FONTFORGE_TOTTFGPOS_H */
