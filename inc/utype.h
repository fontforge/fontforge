#ifndef FONTFORGE_UNICODE_UTYPE_H
#define FONTFORGE_UNICODE_UTYPE_H
/* Copyright: 2001 George Williams */
/* License: BSD-3-clause */
/* Contributions: Joe Da Silva */

/* This file was generated using the program 'makeutype' for Unicode_version 9.0 */

#include <ctype.h>	/* Include here so we can control it. If a system header includes it later bad things happen */
#include <basics.h>	/* Include here so we can use pre-defined int types to correctly size constant data arrays. */
#ifdef tolower
# undef tolower
#endif
#ifdef toupper
# undef toupper
#endif
#ifdef islower
# undef islower
#endif
#ifdef isupper
# undef isupper
#endif
#ifdef isalpha
# undef isalpha
#endif
#ifdef isdigit
# undef isdigit
#endif
#ifdef isalnum
# undef isalnum
#endif
#ifdef isspace
# undef isspace
#endif
#ifdef ispunct
# undef ispunct
#endif
#ifdef ishexdigit
# undef ishexdigit
#endif

extern const unsigned short ff_unicode_tolower[];
extern const unsigned short ff_unicode_toupper[];
extern const unsigned short ff_unicode_totitle[];
extern const unsigned short ff_unicode_tomirror[];
extern const unsigned char  ff_unicode_digitval[];

/* utype[] holds binary flags used for features of each unicode.org character */
#define FF_UNICODE_LOWER        0x1
#define FF_UNICODE_L            FF_UNICODE_LOWER
#define FF_UNICODE_UPPER        0x2
#define FF_UNICODE_U            FF_UNICODE_UPPER
#define FF_UNICODE_TITLE        0x4
#define FF_UNICODE_DIGIT        0x8
#define FF_UNICODE_D            FF_UNICODE_DIGIT
#define FF_UNICODE_SPACE        0x10
#define FF_UNICODE_S            FF_UNICODE_SPACE
#define FF_UNICODE_PUNCT        0x20
#define FF_UNICODE_P            FF_UNICODE_PUNCT
#define FF_UNICODE_HEX          0x40
#define FF_UNICODE_X            FF_UNICODE_HEX
#define FF_UNICODE_ZEROWIDTH    0x80
#define FF_UNICODE_ZW           FF_UNICODE_ZEROWIDTH
#define FF_UNICODE_LEFT_2_RIGHT 0x100
#define FF_UNICODE_L2R          FF_UNICODE_LEFT_2_RIGHT
#define FF_UNICODE_RIGHT_2_LEFT 0x200
#define FF_UNICODE_R2L          FF_UNICODE_RIGHT_2_LEFT
#define FF_UNICODE_ENUMERIC     0x400
#define FF_UNICODE_ENUM         FF_UNICODE_ENUMERIC
#define FF_UNICODE_ANUMERIC     0x800
#define FF_UNICODE_ANUM         FF_UNICODE_ANUMERIC
#define FF_UNICODE_ENS          0x1000
#define FF_UNICODE_CS           0x2000
#define FF_UNICODE_ENT          0x4000
#define FF_UNICODE_COMBINING    0x8000
#define FF_UNICODE_COMBINE      FF_UNICODE_COMBINING
#define FF_UNICODE_BREAKBEFOREOK 0x10000
#define FF_UNICODE_BB           FF_UNICODE_BREAKBEFOREOK
#define FF_UNICODE_BREAKAFTEROK 0x20000
#define FF_UNICODE_BA           FF_UNICODE_BREAKAFTEROK
#define FF_UNICODE_NONSTART     0x40000
#define FF_UNICODE_NS           FF_UNICODE_NONSTART
#define FF_UNICODE_NONEND       0x80000
#define FF_UNICODE_NE           FF_UNICODE_NONEND
#define FF_UNICODE_URLBREAKAFTER 0x100000
#define FF_UNICODE_MUSTBREAK    FF_UNICODE_URLBREAKAFTER
#define FF_UNICODE_UB           FF_UNICODE_URLBREAKAFTER
#define FF_UNICODE_NOBREAK      0x8000000
#define FF_UNICODE_NB           FF_UNICODE_NOBREAK
#define FF_UNICODE_ALPHABETIC   0x200000
#define FF_UNICODE_AL           FF_UNICODE_ALPHABETIC
#define FF_UNICODE_IDEOGRAPHIC  0x400000
#define FF_UNICODE_ID           FF_UNICODE_IDEOGRAPHIC
#define FF_UNICODE_INITIAL      0x800000
#define FF_UNICODE_MEDIAL       0x1000000
#define FF_UNICODE_FINAL        0x2000000
#define FF_UNICODE_ISOLATED     0x4000000
#define FF_UNICODE_DECOMPNORM   0x10000000
#define FF_UNICODE_DecompositionNormative FF_UNICODE_DECOMPNORM
#define FF_UNICODE_LIG_OR_FRAC  0x20000000

#define islower(ch)             (ff_unicode_utype[(ch)+1]&FF_UNICODE_L)
#define isupper(ch)             (ff_unicode_utype[(ch)+1]&FF_UNICODE_U)
#define istitle(ch)             (ff_unicode_utype[(ch)+1]&FF_UNICODE_TITLE)
#define isalpha(ch)             (ff_unicode_utype[(ch)+1]&(FF_UNICODE_L|FF_UNICODE_U|FF_UNICODE_TITLE|FF_UNICODE_AL))
#define isdigit(ch)             (ff_unicode_utype[(ch)+1]&FF_UNICODE_D)
#define isalnum(ch)             (ff_unicode_utype[(ch)+1]&(FF_UNICODE_L|FF_UNICODE_U|FF_UNICODE_TITLE|FF_UNICODE_AL|FF_UNICODE_D))
#define isideographic(ch)       (ff_unicode_utype[(ch)+1]&FF_UNICODE_ID)
#define isideoalpha(ch)         (ff_unicode_utype[(ch)+1]&(FF_UNICODE_ID|FF_UNICODE_L|FF_UNICODE_U|FF_UNICODE_TITLE|FF_UNICODE_AL))
#define isspace(ch)             (ff_unicode_utype[(ch)+1]&FF_UNICODE_S)
#define ispunct(ch)             (ff_unicode_utype[(ch)+1]&_FF_UNICODE_P)
#define ishexdigit(ch)          (ff_unicode_utype[(ch)+1]&FF_UNICODE_X)
#define iszerowidth(ch)         (ff_unicode_utype[(ch)+1]&FF_UNICODE_ZW)
#define islefttoright(ch)       (ff_unicode_utype[(ch)+1]&FF_UNICODE_L2R)
#define isrighttoleft(ch)       (ff_unicode_utype[(ch)+1]&FF_UNICODE_R2L)
#define iseuronumeric(ch)       (ff_unicode_utype[(ch)+1]&FF_UNICODE_ENUM)
#define isarabnumeric(ch)       (ff_unicode_utype[(ch)+1]&FF_UNICODE_ANUM)
#define iseuronumsep(ch)        (ff_unicode_utype[(ch)+1]&FF_UNICODE_ENS)
#define iscommonsep(ch)         (ff_unicode_utype[(ch)+1]&FF_UNICODE_CS)
#define iseuronumterm(ch)       (ff_unicode_utype[(ch)+1]&FF_UNICODE_ENT)
#define iscombining(ch)         (ff_unicode_utype[(ch)+1]&FF_UNICODE_COMBINE)
#define isbreakbetweenok(ch1,ch2) (((ff_unicode_utype[(ch1)+1]&FF_UNICODE_BA) && !(ff_unicode_utype[(ch2)+1]&FF_UNICODE_NS)) || ((ff_unicode_utype[(ch2)+1]&FF_UNICODE_BB) && !(ff_unicode_utype[(ch1)+1]&FF_UNICODE_NE)) || (!(ff_unicode_utype[(ch2)+1]&FF_UNICODE_D) && ch1=='/'))
#define isnobreak(ch)           (ff_unicode_utype[(ch)+1]&FF_UNICODE_NB)
#define isarabinitial(ch)       (ff_unicode_utype[(ch)+1]&FF_UNICODE_INITIAL)
#define isarabmedial(ch)        (ff_unicode_utype[(ch)+1]&FF_UNICODE_MEDIAL)
#define isarabfinal(ch)         (ff_unicode_utype[(ch)+1]&FF_UNICODE_FINAL)
#define isarabisolated(ch)      (ff_unicode_utype[(ch)+1]&FF_UNICODE_ISOLATED)
#define isdecompositionnormative(ch) (ff_unicode_utype[(ch)+1]&FF_UNICODE_DECOMPNORM)
#define isligorfrac(ch)         (ff_unicode_utype[(ch)+1]&FF_UNICODE_LIG_OR_FRAC)

extern const uint32 ff_unicode_utype[]; /* hold character type features for each Unicode.org defined character */

/* utype2[] binary flags used for position/layout of each unicode.org character */
#define FF_UNICODE_COMBININGCLASS      0xff
#define FF_UNICODE_ABOVE               0x100
#define FF_UNICODE_BELOW               0x200
#define FF_UNICODE_OVERSTRIKE          0x400
#define FF_UNICODE_LEFT                0x800
#define FF_UNICODE_RIGHT               0x1000
#define FF_UNICODE_JOINS2              0x2000
#define FF_UNICODE_CENTERLEFT          0x4000
#define FF_UNICODE_CENTERRIGHT         0x8000
#define FF_UNICODE_CENTEREDOUTSIDE     0x10000
#define FF_UNICODE_OUTSIDE             0x20000
#define FF_UNICODE_LEFTEDGE            0x80000
#define FF_UNICODE_RIGHTEDGE           0x40000
#define FF_UNICODE_TOUCHING            0x100000
#define FF_UNICODE_COMBININGPOSMASK    (FF_UNICODE_OUTSIDE|FF_UNICODE_CENTEREDOUTSIDE|FF_UNICODE_CENTERRIGHT|FF_UNICODE_CENTERLEFT|FF_UNICODE_JOINS2|FF_UNICODE_RIGHT|FF_UNICODE_LEFT|FF_UNICODE_OVERSTRIKE|FF_UNICODE_BELOW|FF_UNICODE_ABOVE|FF_UNICODE_RIGHTEDGE|FF_UNICODE_LEFTEDGE|FF_UNICODE_TOUCHING)
#define FF_UNICODE_NOPOSDATAGIVEN      (uint32)(-1)	/* -1 == no position data given */

#define combiningclass(ch)	(ff_unicode_utype2[(ch)+1]&FF_UNICODE_COMBININGCLASS)
#define combiningposmask(ch)	(ff_unicode_utype2[(ch)+1]&FF_UNICODE_COMBININGPOSMASK)

#define FF_UNICODE_COMPACT_TABLE_SIZE_MAX 65536

extern const uint32	ff_unicode_utype2[];		/* hold position boolean flags for each Unicode.org defined character */

#define isunicodepointassigned(ch) (ff_unicode_codepointassigned[(ch)/32]&(1<<((ch)%32)))

extern const uint32	ff_unicode_codepointassigned[]; /* 1bit_boolean_flag x 32 = exists in Unicode.org character chart list. */

#define tolower(ch) (ff_unicode_tolower[(ch)+1])
#define toupper(ch) (ff_unicode_toupper[(ch)+1])
#define totitle(ch) (ff_unicode_totitle[(ch)+1])
#define tomirror(ch) (ff_unicode_tomirror[(ch)+1])
#define tovalue(ch) (ff_unicode_digitval[(ch)+1])


extern struct arabicforms {
    unsigned short initial, medial, final, isolated;
    unsigned int isletter: 1;
    unsigned int joindual: 1;
    unsigned int required_lig_with_alef: 1;
} ArabicForms[256];	/* for chars 0x600-0x6ff, subtract 0x600 to use array */


/* Ligature/Vulgar_Fraction/Fraction unicode.org character lists & functions */

/* Return !0 if this unicode value is a Ligature or Fraction */
extern int LigatureCount(void);		/* Unicode table Ligature count */
extern int VulgarFractionCount(void);	/* Unicode table Vulgar Fraction count */
extern int OtherFractionCount(void);	/* Unicode table Other Fractions count */
extern int FractionCount(void);		/* Unicode table Fractions found */

extern int32 Ligature_get_U(int n);	/* Get table[N] value, error==-1 */
extern int32 VulgFrac_get_U(int n);	/* Get table[N] value, error==-1 */
extern int32 Fraction_get_U(int n);	/* Get table[N] value, error==-1 */

extern int Ligature_find_N(uint32 u);	/* Find N of Ligature[N], error==-1 */
extern int VulgFrac_find_N(uint32 u);	/* Find N of VulgFrac[N], error==-1 */
extern int Fraction_find_N(uint32 u);	/* Find N of Fraction[N], error==-1 */

/* Return !0 if codepoint is a Ligature */
extern int is_LIGATURE(uint32 codepoint);

/* Return !0 if codepoint is a Vulgar Fraction */
extern int is_VULGAR_FRACTION(uint32 codepoint);

/* Return !0 if codepoint is a non-vulgar Fraction */
extern int is_OTHER_FRACTION(uint32 codepoint);

/* Return !0 if codepoint is a Fraction */
extern int is_FRACTION(uint32 codepoint);

/* Return !0 if codepoint is a Ligature or Vulgar Fraction */
extern int is_LIGATURE_or_VULGAR_FRACTION(uint32 codepoint);

/* Return !0 if codepoint is a Ligature or non-Vulgar Fraction */
extern int is_LIGATURE_or_OTHER_FRACTION(uint32 codepoint);

/* Return !0 if codepoint is a Ligature or Fraction */
extern int is_LIGATURE_or_FRACTION(uint32 codepoint);


#define FF_UNICODE_SOFT_HYPHEN	0xad

#define FF_UNICODE_DOUBLE_S	0xdf

#endif /* FONTFORGE_UNICODE_UTYPE_H */
