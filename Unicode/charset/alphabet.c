#include <chardata.h>

unichar_t *unicode_from_alphabets[]={
    (unichar_t *) unicode_from_win, 0,0,
    (unichar_t *) unicode_from_i8859_1, 
    (unichar_t *) unicode_from_i8859_2,
    (unichar_t *) unicode_from_i8859_3,
    (unichar_t *) unicode_from_i8859_4,
    (unichar_t *) unicode_from_i8859_5,
    (unichar_t *) unicode_from_i8859_6,
    (unichar_t *) unicode_from_i8859_7,
    (unichar_t *) unicode_from_i8859_8,
    (unichar_t *) unicode_from_i8859_9,
    (unichar_t *) unicode_from_i8859_10,
    (unichar_t *) unicode_from_i8859_11,
    (unichar_t *) unicode_from_i8859_13,
    (unichar_t *) unicode_from_i8859_14,
    (unichar_t *) unicode_from_i8859_15,
    (unichar_t *) unicode_from_i8859_16,
    (unichar_t *) unicode_from_koi8_r,
    (unichar_t *) unicode_from_jis201,
    (unichar_t *) unicode_from_win,
    (unichar_t *) unicode_from_mac,
    (unichar_t *) unicode_from_MacSymbol,
    (unichar_t *) unicode_from_ZapfDingbats,
    (unichar_t *) unicode_from_i8859_1,	/* Place holder for user-defined map */
    0
};

struct charmap *alphabets_from_unicode[]={ 0,0,0,
    &i8859_1_from_unicode,
    &i8859_2_from_unicode,
    &i8859_3_from_unicode,
    &i8859_4_from_unicode,
    &i8859_5_from_unicode,
    &i8859_6_from_unicode,
    &i8859_7_from_unicode,
    &i8859_8_from_unicode,
    &i8859_9_from_unicode,
    &i8859_10_from_unicode,
    &i8859_11_from_unicode,
    &i8859_13_from_unicode,
    &i8859_14_from_unicode,
    &i8859_15_from_unicode,
    &i8859_16_from_unicode,
    &koi8_r_from_unicode,
    &jis201_from_unicode,
    &win_from_unicode,
    &mac_from_unicode,
    &MacSymbol_from_unicode,
    &ZapfDingbats_from_unicode,
    &i8859_1_from_unicode,	/* Place holder for user-defined map*/
    0
};
