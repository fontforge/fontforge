#include "basics.h"

struct charmap {
    int first, last;
    unsigned char **table;
    unichar_t *totable;
};
struct charmap2 {
    int first, last;
    unsigned short **table;
    unichar_t *totable;
};

extern const unichar_t unicode_from_i8859_1[];
extern struct charmap i8859_1_from_unicode;
extern const unichar_t unicode_from_i8859_2[];
extern struct charmap i8859_2_from_unicode;
extern const unichar_t unicode_from_i8859_3[];
extern struct charmap i8859_3_from_unicode;
extern const unichar_t unicode_from_i8859_4[];
extern struct charmap i8859_4_from_unicode;
extern const unichar_t unicode_from_i8859_5[];
extern struct charmap i8859_5_from_unicode;
extern const unichar_t unicode_from_i8859_6[];
extern struct charmap i8859_6_from_unicode;
extern const unichar_t unicode_from_i8859_7[];
extern struct charmap i8859_7_from_unicode;
extern const unichar_t unicode_from_i8859_8[];
extern struct charmap i8859_8_from_unicode;
extern const unichar_t unicode_from_i8859_9[];
extern struct charmap i8859_9_from_unicode;
extern const unichar_t unicode_from_i8859_10[];
extern struct charmap i8859_10_from_unicode;
extern const unichar_t unicode_from_i8859_11[];
extern struct charmap i8859_11_from_unicode;
extern const unichar_t unicode_from_i8859_13[];
extern struct charmap i8859_13_from_unicode;
extern const unichar_t unicode_from_i8859_14[];
extern struct charmap i8859_14_from_unicode;
extern const unichar_t unicode_from_i8859_15[];
extern struct charmap i8859_15_from_unicode;
extern const unichar_t unicode_from_i8859_16[];
extern struct charmap i8859_16_from_unicode;
extern const unichar_t unicode_from_koi8_r[];
extern struct charmap koi8_r_from_unicode;
extern const unichar_t unicode_from_jis201[];
extern struct charmap jis201_from_unicode;
extern const unichar_t unicode_from_win[];
extern struct charmap win_from_unicode;
extern const unichar_t unicode_from_mac[];
extern struct charmap mac_from_unicode;
extern const unichar_t unicode_from_MacSymbol[];
extern struct charmap MacSymbol_from_unicode;
extern const unichar_t unicode_from_ZapfDingbats[];
extern struct charmap ZapfDingbats_from_unicode;

extern unichar_t *unicode_from_alphabets[];
extern struct charmap *alphabets_from_unicode[];

extern const unichar_t unicode_from_jis208[];
extern const unichar_t unicode_from_jis212[];
extern struct charmap2 jis_from_unicode;
/* Subtract 0xa100 before indexing this array */
extern const unichar_t unicode_from_big5[];
extern struct charmap2 big5_from_unicode;
/* Subtract 0x8100 before indexing this array */
extern const unichar_t unicode_from_big5hkscs[];
extern struct charmap2 big5hkscs_from_unicode;
extern const unichar_t unicode_from_ksc5601[];
extern struct charmap2 ksc5601_from_unicode;
/* Subtract 0x8400 before indexing this array */
extern const unichar_t unicode_from_johab[];
extern struct charmap2 johab_from_unicode;
extern const unichar_t unicode_from_gb2312[];
extern struct charmap2 gb2312_from_unicode;

/* a mask for each character saying what charset(s) it may be found in */
extern const unsigned long * const unicode_backtrans[];

extern const unichar_t *const * const unicode_alternates[];
