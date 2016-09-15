#ifndef _UTYPE_H
#define _UTYPE_H
/* Copyright: 2001 George Williams */
/* License: BSD-3-clause */
/* Contributions: Joe Da Silva */

/* This file was generated using the program 'makeutype' */

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

extern const unsigned short ____tolower[];
extern const unsigned short ____toupper[];
extern const unsigned short ____totitle[];
extern const unsigned short ____tomirror[];
extern const unsigned char  ____digitval[];

/* utype[] holds binary flags used for features of each unicode.org character */
#define ____L		0x1
#define ____U		0x2
#define ____TITLE	0x4
#define ____D		0x8
#define ____S		0x10
#define ____P		0x20
#define ____X		0x40
#define ____ZW		0x80
#define ____L2R		0x100
#define ____R2L		0x200
#define ____ENUM	0x400
#define ____ANUM	0x800
#define ____ENS		0x1000
#define ____CS		0x2000
#define ____ENT		0x4000
#define ____COMBINE	0x8000
#define ____BB		0x10000
#define ____BA		0x20000
#define ____NS		0x40000
#define ____NE		0x80000
#define ____UB		0x100000
#define ____NB		0x8000000
#define ____AL		0x200000
#define ____ID		0x400000
#define ____INITIAL	0x800000
#define ____MEDIAL	0x1000000
#define ____FINAL	0x2000000
#define ____ISOLATED	0x4000000
#define ____DECOMPNORM	0x10000000
#define ____LIG_OR_FRAC	0x20000000

#define islower(ch)		(____utype[(ch)+1]&____L)
#define isupper(ch)		(____utype[(ch)+1]&____U)
#define istitle(ch)		(____utype[(ch)+1]&____TITLE)
#define isalpha(ch)		(____utype[(ch)+1]&(____L|____U|____TITLE|____AL))
#define isdigit(ch)		(____utype[(ch)+1]&____D)
#define isalnum(ch)		(____utype[(ch)+1]&(____L|____U|____TITLE|____AL|____D))
#define isideographic(ch)	(____utype[(ch)+1]&____ID)
#define isideoalpha(ch)		(____utype[(ch)+1]&(____ID|____L|____U|____TITLE|____AL))
#define isspace(ch)		(____utype[(ch)+1]&____S)
#define ispunct(ch)		(____utype[(ch)+1]&_____P)
#define ishexdigit(ch)		(____utype[(ch)+1]&____X)
#define iszerowidth(ch)		(____utype[(ch)+1]&____ZW)
#define islefttoright(ch)	(____utype[(ch)+1]&____L2R)
#define isrighttoleft(ch)	(____utype[(ch)+1]&____R2L)
#define iseuronumeric(ch)	(____utype[(ch)+1]&____ENUM)
#define isarabnumeric(ch)	(____utype[(ch)+1]&____ANUM)
#define iseuronumsep(ch)	(____utype[(ch)+1]&____ENS)
#define iscommonsep(ch)		(____utype[(ch)+1]&____CS)
#define iseuronumterm(ch)	(____utype[(ch)+1]&____ENT)
#define iscombining(ch)		(____utype[(ch)+1]&____COMBINE)
#define isbreakbetweenok(ch1,ch2) (((____utype[(ch1)+1]&____BA) && !(____utype[(ch2)+1]&____NS)) || ((____utype[(ch2)+1]&____BB) && !(____utype[(ch1)+1]&____NE)) || (!(____utype[(ch2)+1]&____D) && ch1=='/'))
#define isnobreak(ch)		(____utype[(ch)+1]&____NB)
#define isarabinitial(ch)	(____utype[(ch)+1]&____INITIAL)
#define isarabmedial(ch)	(____utype[(ch)+1]&____MEDIAL)
#define isarabfinal(ch)		(____utype[(ch)+1]&____FINAL)
#define isarabisolated(ch)	(____utype[(ch)+1]&____ISOLATED)

#define isdecompositionnormative(ch) (____utype[(ch)+1]&____DECOMPNORM)

#define isligorfrac(ch)		(____utype[(ch)+1]&____LIG_OR_FRAC)

extern const uint32	____utype[];		/* hold character type features for each Unicode.org defined character */

/* utype2[] binary flags used for position/layout of each unicode.org character */
#define ____COMBININGCLASS	0xff
#define ____ABOVE		0x100
#define ____BELOW		0x200
#define ____OVERSTRIKE		0x400
#define ____LEFT		0x800
#define ____RIGHT		0x1000
#define ____JOINS2		0x2000
#define ____CENTERLEFT		0x4000
#define ____CENTERRIGHT		0x8000
#define ____CENTEREDOUTSIDE	0x10000
#define ____OUTSIDE		0x20000
#define ____LEFTEDGE		0x80000
#define ____RIGHTEDGE		0x40000
#define ____TOUCHING		0x100000
#define ____COMBININGPOSMASK	0x1fff00
#define ____NOPOSDATAGIVEN	(uint32)(-1)	/* -1 == no position data given */

#define combiningclass(ch)	(____utype2[(ch)+1]&____COMBININGCLASS)
#define combiningposmask(ch)	(____utype2[(ch)+1]&____COMBININGPOSMASK)

extern const uint32	____utype2[];		/* hold position boolean flags for each Unicode.org defined character */

#define isunicodepointassigned(ch) (____codepointassigned[(ch)/32]&(1<<((ch)%32)))

extern const uint32	____codepointassigned[]; /* 1bit_boolean_flag x 32 = exists in Unicode.org character chart list. */

#define tolower(ch) (____tolower[(ch)+1])
#define toupper(ch) (____toupper[(ch)+1])
#define totitle(ch) (____totitle[(ch)+1])
#define tomirror(ch) (____tomirror[(ch)+1])
#define tovalue(ch) (____digitval[(ch)+1])


extern struct arabicforms {
    unsigned short initial, medial, final, isolated;
    unsigned int isletter: 1;
    unsigned int joindual: 1;
    unsigned int required_lig_with_alef: 1;
} ArabicForms[256];	/* for chars 0x600-0x6ff, subtract 0x600 to use array */


/* Ligature/Vulgar_Fraction/Fraction unicode.org character lists & functions */

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


#define _SOFT_HYPHEN	0xad

#define _DOUBLE_S	0xdf

#endif
