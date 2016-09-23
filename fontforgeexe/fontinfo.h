#ifndef FONTFORGE_FONTFORGEEXE_FONTINFO_H
#define FONTFORGE_FONTFORGEEXE_FONTINFO_H

#include <ggadget.h>

extern char* DumpSplineFontMetadata(SplineFont *sf);
extern const char *UI_MSLangString(int language);
extern const char *UI_TTFNameIds(int id);
extern GTextInfo *GListAppendLine8(GGadget *list,const char *line,int select);
extern GTextInfo *GListChangeLine8(GGadget *list,int pos, const char *line);
extern void FontInfoDestroy(SplineFont *sf);
extern void FontInfoInit(void);
extern void FontMenuFontInfo(void *_fv);
extern void GFI_FinishContextNew(struct gfi_data *d,FPST *fpst, int success);
extern void GFI_FinishSMNew(struct gfi_data *d,ASM *sm, int success, int isnew);
extern void GFI_LookupEnableButtons(struct gfi_data *gfi, int isgpos);
extern void GFI_LookupScrollbars(struct gfi_data *gfi, int isgpos, int refresh);
extern void GListDelSelected(GGadget *list);
extern void GListMoveSelected(GGadget *list,int offset);

#endif /* FONTFORGE_FONTFORGEEXE_FONTINFO_H */
