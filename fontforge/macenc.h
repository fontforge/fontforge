#ifndef FONTFORGE_MACENC_H
#define FONTFORGE_MACENC_H

#include "splinefont.h"

extern char *FindEnglishNameInMacName(struct macname *mn);
extern char *MacLanguageFromCode(int code);
extern char *MacStrToUtf8(const char *str, int macenc, int maclang);
extern char *PickNameFromMacName(struct macname *mn);
extern char *Utf8ToMacStr(const char *ustr, int macenc, int maclang);
extern const int32 *MacEncToUnicode(int script, int lang);
extern int CanEncodingWinLangAsMac(int winlang);
extern int MacLangFromLocale(void);
extern int UserFeaturesDiffer(void);
extern MacFeat *FindMacFeature(SplineFont *sf, int feat, MacFeat **secondary);
extern struct macname *FindMacSettingName(SplineFont *sf, int feat, int set);
extern struct macname *MacNameCopy(struct macname *mn);
extern uint16 WinLangFromMac(int maclang);
extern uint16 WinLangToMac(int winlang);
extern uint8 MacEncFromMacLang(int maclang);

#endif /* FONTFORGE_MACENC_H */
