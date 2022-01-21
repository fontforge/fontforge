#ifndef FONTFORGE_TTFSPECIAL_H
#define FONTFORGE_TTFSPECIAL_H

#include "splinefont.h"
#include "ttf.h"

/* Non-standard tables */

/* My PfEd table for FontForge/PfaEdit specific info */
extern void pfed_dump(struct alltabs *at, SplineFont *sf);
extern void pfed_read(FILE *ttf, struct ttfinfo *info);

/* The TeX table, to contain stuff the TeX people want */
extern void tex_dump(struct alltabs *at, SplineFont *sf);
extern void tex_read(FILE *ttf, struct ttfinfo *info);

/* The BDF table, to contain bdf properties the X people want */
extern int ttf_bdf_dump(SplineFont *sf, struct alltabs *at,int32_t *sizes);
extern void ttf_bdf_read(FILE *ttf, struct ttfinfo *info);

/* The FFTM table, to some timestamps I'd like */
extern int ttf_fftm_dump(SplineFont *sf, struct alltabs *at);

/* Known font parameters for 'TeX ' table (fontdims, spacing params, whatever you want to call them) */

/* Used by all fonts */
#define TeX_Slant        CHR('S','l','n','t')
#define TeX_Space        CHR('S','p','a','c')
#define TeX_Stretch      CHR('S','t','r','e')
#define TeX_Shrink       CHR('S','h','n','k')
#define TeX_XHeight      CHR('X','H','g','t')
#define TeX_Quad         CHR('Q','u','a','d')
/* Used by text fonts */
#define TeX_ExtraSp      CHR('E','x','S','p')
/* Used by all math fonts */
#define TeX_MathSp       CHR('M','t','S','p')
/* Used by math fonts */
#define TeX_Num1         CHR('N','u','m','1')
#define TeX_Num2         CHR('N','u','m','2')
#define TeX_Num3         CHR('N','u','m','3')
#define TeX_Denom1       CHR('D','n','m','1')
#define TeX_Denom2       CHR('D','n','m','2')
#define TeX_Sup1         CHR('S','u','p','1')
#define TeX_Sup2         CHR('S','u','p','2')
#define TeX_Sup3         CHR('S','u','p','3')
#define TeX_Sub1         CHR('S','u','b','1')
#define TeX_Sub2         CHR('S','u','b','2')
#define TeX_SupDrop      CHR('S','p','D','p')
#define TeX_SubDrop      CHR('S','b','D','p')
#define TeX_Delim1       CHR('D','l','m','1')
#define TeX_Delim2       CHR('D','l','m','2')
#define TeX_AxisHeight   CHR('A','x','H','t')
/* Used by math extension fonts */
#define TeX_DefRuleThick CHR('R','l','T','k')
#define TeX_BigOpSpace1  CHR('B','O','S','1')
#define TeX_BigOpSpace2  CHR('B','O','S','2')
#define TeX_BigOpSpace3  CHR('B','O','S','3')
#define TeX_BigOpSpace4  CHR('B','O','S','4')
#define TeX_BigOpSpace5  CHR('B','O','S','5')

#endif /* FONTFORGE_TTFSPECIAL_H */
