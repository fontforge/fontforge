#ifndef FONTFORGE_PARSEPFA_H
#define FONTFORGE_PARSEPFA_H

#ifdef __cplusplus
extern "C" {
#endif

#include "psfont.h"
#include "splinefont.h"

extern char **NamesReadPostScript(char *filename);
extern char **_NamesReadPostScript(FILE *ps);
extern FontDict *ReadPSFont(char *fontname);
extern FontDict *_ReadPSFont(FILE *in);
extern void PSCharsFree(struct pschars *chrs);
extern void PSDictFree(struct psdict *dict);
extern void PSFontFree(FontDict *fd);

#ifdef __cplusplus
}
#endif

#endif /* FONTFORGE_PARSEPFA_H */
