#ifndef FONTFORGE_BITMAPCHAR_H
#define FONTFORGE_BITMAPCHAR_H

#include "splinefont.h"
#include "uiinterface.h"

extern BDFChar *BDFMakeChar(BDFFont *bdf, EncMap *map, int enc);
extern BDFChar *BDFMakeGID(BDFFont *bdf, int gid);
extern BDFProperties *BdfPropsCopy(BDFProperties *props, int cnt);
extern const char *BdfPropHasString(BDFFont *font, const char *key, const char *def);
extern int BdfPropHasInt(BDFFont *font, const char *key, int def);
extern int IsUnsignedBDFKey(const char *key);
extern void BCClearAll(BDFChar *bc);
extern void BDFDefaultProps(BDFFont *bdf, EncMap *map, int res);
extern void Default_Properties(BDFFont *bdf, EncMap *map, char *onlyme);
extern void Default_XLFD(BDFFont *bdf, EncMap *map, int res);
extern void def_Charset_Enc(EncMap *map, char *reg, char *enc);
extern void FF_SetBCInterface(struct bc_interface *bci);
extern void SFReplaceEncodingBDFProps(SplineFont *sf, EncMap *map);
extern void SFReplaceFontnameBDFProps(SplineFont *sf);
extern void XLFD_CreateComponents(BDFFont *font, EncMap *map, int res, struct xlfd_components *components);
extern void XLFD_GetComponents(const char *xlfd, struct xlfd_components *components);

#endif /* FONTFORGE_BITMAPCHAR_H */
