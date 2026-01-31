#ifndef FONTFORGE_MACENC_H
#define FONTFORGE_MACENC_H

#include "splinefont.h"

#ifdef __cplusplus
extern "C" {
#endif

extern char *FindEnglishNameInMacName(struct macname *mn);
extern char *MacLanguageFromCode(int code);
extern char *MacStrToUtf8(const char *str, int macenc, int maclang);
extern char *PickNameFromMacName(struct macname *mn);
extern char *Utf8ToMacStr(const char *ustr, int macenc, int maclang);
extern const int32_t *MacEncToUnicode(int script, int lang);
extern int CanEncodingWinLangAsMac(int winlang);
extern int MacLangFromLocale(void);
extern int UserFeaturesDiffer(void);
extern MacFeat *FindMacFeature(SplineFont *sf, int feat, MacFeat **secondary);
extern struct macname *FindMacSettingName(SplineFont *sf, int feat, int set);
extern struct macname *MacNameCopy(struct macname *mn);
extern uint16_t WinLangFromMac(int maclang);
extern uint16_t WinLangToMac(int winlang);
extern uint8_t MacEncFromMacLang(int maclang);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_MACENC_H */
