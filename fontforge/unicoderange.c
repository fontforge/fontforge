/* Copyright (C) 2006-2012 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fontforge.h"
#include "unicoderange.h"
#include <stdlib.h>
#include <string.h>
#include <utype.h>
#define ASFIRST (-1)


struct unicoderange unicoderange[] = {
    /* { N_("Unicode"), 0, 0x10ffff, ' ' }, */
    { N_("Unicode Basic Multilingual Plane"), 0, 0xffff, ' ', false },
    { N_("Basic Multilingual Plane"), 0, 0xffff, ' ', 2 },
    { N_("Alphabetic"), 0, 0x1fff, 'A', false },
    { N_("C0 Control Character"), 0, ' '-1, 0, true },
    { N_("NUL, Default Character"), 0, 0, 0, false },
    { N_("Basic Latin"), ' ', 0x7e, 'A', true },
    { N_("Delete Character"), 0x7f, 0x7f, ASFIRST, true },
    { N_("C1 Control Character"), 0x80, 0x9f, ASFIRST, true },
    { N_("Latin-1 Supplement"), 0xa0, 0xff, 0xc0, true },
    { N_("Latin Extended-A"), 0x100, 0x17f, ASFIRST, true },
    { N_("Latin Extended-B"), 0x180, 0x24f, ASFIRST, true },
    { N_("IPA Extensions"), 0x250, 0x2af, ASFIRST, true },
    { N_("Spacing Modifier Letters"), 0x2b0, 0x2ff, ASFIRST, true },
    { N_("Combining Diacritical Marks"), 0x300, 0x36f, ASFIRST, true },
    { N_("Greek"), 0x370, 0x3ff, 0x391, true },
    { N_("Greek and Coptic"), 0x370, 0x3ff, 0x391, true },
    { N_("Cyrillic"), 0x400, 0x4ff, 0x410, true },
    { N_("Cyrillic Supplement"), 0x500, 0x52f, ASFIRST, true },
    { N_("Armenian"), 0x530, 0x58f, 0x531, true },
    { N_("Hebrew"), 0x590, 0x5ff, 0x5d0, true },
    { N_("Arabic"), 0x600, 0x6ff, 0x627, true },
    { N_("Syriac"), 0x700, 0x74f, 0x710, true },
    { N_("Arabic Supplement"), 0x750, 0x77f, ASFIRST, true },
    { N_("Thaana"), 0x780, 0x7bf, ASFIRST, true },
    { N_("N'Ko"), 0x7c0, 0x7fa, ASFIRST, true },
    { N_("Samaritan"), 0x800, 0x82D, ASFIRST, true },
    { N_("Samaritan, Punctuation"), 0x830, 0x83E, ASFIRST, true },
    { N_("Mandaic"), 0x8d0, 0x8ff, ASFIRST, true },
    { N_("Devanagari"), 0x900, 0x97f, 0x905, true },
    { N_("Bengali"), 0x980, 0x9ff, 0x985, true },
    { N_("Gurmukhi"), 0xa00, 0xa7f, 0xa05, true },
    { N_("Gujarati"), 0xa80, 0xaff, 0xa85, true },
    { N_("Oriya"), 0xb00, 0xb7f, 0xb05, true },
    { N_("Tamil"), 0xb80, 0xbff, 0xb85, true },
    { N_("Telugu"), 0xc00, 0xc7f, 0xc05, true },
    { N_("Kannada"), 0xc80, 0xcff, 0xc85, true },
    { N_("Malayalam"), 0xd00, 0xd7f, 0xd05, true },
    { N_("Sinhala"), 0xd80, 0xdff, 0xd85, true },
    { N_("Thai"), 0xe00, 0xe7f, 0xe01, true },
    { N_("Lao"), 0xe80, 0xeff, 0xe81, true },
    { N_("Tibetan"), 0xf00, 0xfff, ASFIRST, true },
    { N_("Myanmar"), 0x1000, 0x109f, ASFIRST, true },
    { N_("Georgian"), 0x10a0, 0x10ff, 0x10d0, true },
    { N_("Hangul Jamo, Choseong"), 0x1100, 0x115f, ASFIRST, true },
    { N_("Hangul Jamo, Jungseong"), 0x1160, 0x11a7, 0x1161, true },
    { N_("Hangul Jamo, Jongseong"), 0x11a8, 0x11ff, ASFIRST, true },
    { N_("Ethiopic"), 0x1200, 0x137f, ASFIRST, true },
    { N_("Ethiopic Supplement"), 0x1380, 0x139f, ASFIRST, true },
    { N_("Cherokee"), 0x13a0, 0x13ff, ASFIRST, true },
    { N_("Unified Canadian Aboriginal Syllabics"), 0x1400, 0x169c, ASFIRST, true },
    { N_("Ogham"), 0x1680, 0x169f, ASFIRST, true },
    { N_("Runic"), 0x16a0, 0x16ff, ASFIRST, true },
    { N_("Tagalog"), 0x1700, 0x171f, ASFIRST, true },
    { NU_("Hanunóo"), 0x1720, 0x173f, ASFIRST, true },
    { N_("Buhid"), 0x1740, 0x175f, ASFIRST, true },
    { N_("Tagbanwa"), 0x1760, 0x177f, ASFIRST, true },
    { N_("Khmer"), 0x1780, 0x17ff, ASFIRST, true },
    { N_("Mongolian"), 0x1800, 0x18af, ASFIRST, true },
    { N_("Unified Canadian Aboriginal Syllabics Extended"), 0x18b0, 0x18ff, ASFIRST, true },
    { N_("Limbu"), 0x1900, 0x194f, ASFIRST, true },
    { N_("Tai Le"), 0x1950, 0x197f, ASFIRST, true },
    { N_("New Tai Lue"), 0x1980, 0x19df, ASFIRST, true },
    { N_("Khmer Symbols"), 0x19e0, 0x19ff, ASFIRST, true },
    { N_("Buginese"), 0x1a00, 0x1a1f, ASFIRST, true },
    { N_("Tai Tham"), 0x1a20, 0x1aad, ASFIRST, true },
    { N_("Balinese"), 0x1b00, 0x1b7f, ASFIRST, true },
    { N_("Sundanese"), 0x1b80, 0x1bbf, ASFIRST, true },
    { N_("Batak"), 0x1bc0, 0x1bff, ASFIRST, true },
    { N_("Lepcha"), 0x1c00, 0x1c4f, ASFIRST, true },
    { N_("Ol Chiki"), 0x1c50, 0x1c7f, ASFIRST, true },
    { N_("Phonetic Extensions"), 0x1d00, 0x1d7f, 0x1d02, true },
    { N_("Phonetic Extensions Supplement"), 0x1d80, 0x1dbf, ASFIRST, true },
    { N_("Combining Diacritical Marks Supplement"), 0x1dc0, 0x1dff, ASFIRST, true },
    { N_("Latin Extended Additional"), 0x1e00, 0x1eff, ASFIRST, true },
    { N_("Greek Extended"), 0x1f00, 0x1fff, ASFIRST, true },
    { N_("Symbols"), 0x2000, 0x2bff, ASFIRST, false },
    { N_("General Punctuation"), 0x2000, 0x206f, ASFIRST, true },
    { N_("Super and Sub scripts"), 0x2070, 0x209f, ASFIRST, true },
    { N_("Currency Symbols"), 0x20a0, 0x20cf, 0x20ac, true },
    { N_("Combining Marks for Symbols"), 0x20d0, 0x20ff, ASFIRST, true },
    { N_("Letterlike Symbols"), 0x2100, 0x214f, ASFIRST, true },
    { N_("Number Forms"), 0x2150, 0x218f, ASFIRST, true },
    { N_("Arrows"), 0x2190, 0x21ff, ASFIRST, true },
    { N_("Mathematical Operators"), 0x2200, 0x22ff, ASFIRST, true },
    { N_("Miscellaneous Technical Symbols"), 0x2300, 0x23ff, ASFIRST, false },
    { N_("Technical Symbols Misc."), 0x2300, 0x23ff, ASFIRST, true },
    { N_("Control Pictures"), 0x2400, 0x243f, ASFIRST, true },
    { N_("OCR"), 0x2440, 0x245f, ASFIRST, true },
    { N_("Enclosed Alphanumerics"), 0x2460, 0x24ff, ASFIRST, true },
    { N_("Box Drawing"), 0x2500, 0x257f, ASFIRST, true },
    { N_("Block Elements"), 0x2580, 0x259f, ASFIRST, true },
    { N_("Geometric Shapes"), 0x25a0, 0x25ff, ASFIRST, true },
    { N_("Miscellaneous Symbols"), 0x2600, 0x267f, ASFIRST, false },
    { N_("Symbols Misc."), 0x2600, 0x267f, ASFIRST, true },
    { N_("Dingbats"), 0x2700, 0x27bf, 0x2701, true },
    {  N_("Zapf Dingbats"), 0x2700, 0x27bf, 0x2701, false },		/* Synonym */
    { N_("Miscellaneous Math Symbols-A"), 0x27c0, 0x27df, ASFIRST, false },
    {  N_("Math Misc. Symbols-A"), 0x27c0, 0x27df, ASFIRST, true},
    { N_("Supplemental Arrows-A"), 0x27e0, 0x27ff, ASFIRST, false },
    {  N_("Arrows Supplement-A"), 0x27e0, 0x27ff, ASFIRST, true },
    { N_("Braille Patterns"), 0x2800, 0x28ff, 0x2800, true },
    { N_("Supplemental Arrows-B"), 0x2900, 0x297f, ASFIRST, false },
    {  N_("Arrows Supplement-B"), 0x2900, 0x297f, ASFIRST, true },
    { N_("Miscellaneous Math Symbols-B"), 0x2980, 0x29ff, ASFIRST, false },
    {  N_("Math Misc. Symbols-B"), 0x2980, 0x29ff, ASFIRST, true },
    { N_("Supplemental Math Operators"), 0x2a00, 0x2aff, ASFIRST, false },
    {  N_("Math Operators Supplement"), 0x2a00, 0x2aff, ASFIRST, true },
    { N_("Supplemental Symbols and Arrows"), 0x2b00, 0x2bff, ASFIRST, false },
    {  N_("Symbols and Arrows Supplement"), 0x2b00, 0x2bff, ASFIRST, true },
    { N_("Alphabetic Extended"), 0x2c00, 0x2dff, ASFIRST, false },
    { N_("Glagolitic"), 0x2c00, 0x2c5f, ASFIRST, true },
    { N_("Latin Extended-C"), 0x2c60, 0x2c7f, ASFIRST, true },
    { N_("Coptic"), 0x2c80, 0x2cff, ASFIRST, true },
    { N_("Georgian Supplement"), 0x2d00, 0x2d2f, ASFIRST, true },
    { N_("Tifinagh"), 0x2d30, 0x2d7f, ASFIRST, true },
    { N_("Ethiopic Extended"), 0x2d80, 0x2ddf, ASFIRST, true },
    { N_("Cyrillic Extended-A"), 0x2de0, 0x2dff, ASFIRST, true },
    { N_("Supplemental Punctuation"), 0x2e00, 0x2e7f, ASFIRST, false },
    { N_("Punctuation Supplement"), 0x2e00, 0x2e7f, ASFIRST, true },
    { N_("CJK Radicals Supplement"), 0x2e80, 0x2eff, ASFIRST, true },
    { N_("Kanqxi Radicals"), 0x2f00, 0x2fdf, ASFIRST, true },
    { N_("Ideographic Description Characters"), 0x2fe0, 0x2fef, ASFIRST, true },
    { N_("CJK Phonetics and Symbols"), 0x3000, 0x33ff, ASFIRST, false },
    { N_("CJK Symbols and Punctuation"), 0x3000, 0x303f, ASFIRST, true },
    { N_("Hiragana"), 0x3040, 0x309f, 0x3041, true },
    { N_("Katakana"), 0x30a0, 0x30ff, 0x30a1, true },
    { N_("Bopomofo"), 0x3100, 0x312f, 0x3105, true },
    { N_("Hangul Compatibility Jamo"), 0x3130, 0x318f, 0x3131, true },
    { N_("Kanbun"), 0x3190, 0x319f, ASFIRST, true },
    { N_("Bopomofo Extended"), 0x31a0, 0x31bf, ASFIRST, true },
    { N_("CJK Strokes"), 0x31c0, 0x31ef, ASFIRST, true },
    { N_("Katakana Phonetic Extensions"), 0x31f0, 0x31ff, ASFIRST, true },
    { N_("Enclosed CJK Letters and Months"), 0x3200, 0x32ff, ASFIRST, true },
    { N_("CJK Enclosed Letters and Months"), 0x3200, 0x32ff, ASFIRST, false },
    { N_("CJK Compatibility"), 0x3300, 0x33ff, ASFIRST, true },
    { N_("CJK Unified Ideographs Extension A"), 0x3400, 0x4dff, ASFIRST, true },
    { N_("Yijing Hexagram Symbols"), 0x4dc0, 0x4dff, ASFIRST, true },
    { N_("CJK Unified Ideographs"), 0x4e00, 0x9fff, ASFIRST, true },
    { N_("Yi Syllables"), 0xa000, 0xa3ff, ASFIRST, true },
    { N_("Yi"), 0xa000, 0xa3ff, ASFIRST, true },
    { N_("Yi Radicals"), 0xa490, 0xa4af, ASFIRST, true },
    { N_("Lisu"), 0xa4d0, 0xa4ff, ASFIRST, true },
    { N_("Vai"), 0xa500, 0xa63f, ASFIRST, true },
    { N_("Cyrillic Extended-B"), 0xa640, 0xa69f, ASFIRST, true },
    { N_("Bamum"), 0xa6a0, 0xa6f7, ASFIRST, true },
    { N_("Modifier Tone Letters"), 0xa700, 0xa71f, ASFIRST, true },
    { N_("Latin Extended-D"), 0xa720, 0xa7ff, ASFIRST, true },
    { N_("Syloti Nagri"), 0xa800, 0xa82f, ASFIRST, true },
    { N_("Common Indic Number Forms"), 0xa830, 0xa83f, ASFIRST, true },
    { N_("Phags-pa"), 0xa840, 0xa87f, ASFIRST, true },
    { N_("Saurashtra"), 0xa880, 0xa8df, ASFIRST, true },
    { N_("Devanagari Extended"), 0xa8e0, 0xa8fb, ASFIRST, true },
    { N_("Kayah Li"), 0xa900, 0xa92f, ASFIRST, true },
    { N_("Rejang"), 0xa930, 0xa95f, ASFIRST, true },
    { N_("Hangul Jamo Extended-A"), 0xa960, 0xa97f, ASFIRST, true },
    { N_("Javanese"), 0xa980, 0xa9df, ASFIRST, true },
    { N_("Cham"), 0xaa00, 0xaa5f, ASFIRST, true },
    { N_("Myanmar Extended-A"), 0xaa60, 0xaa7b, ASFIRST, true },
    { N_("Tai Viet"), 0xaa80, 0xaadf, ASFIRST, true },
    { N_("Ethiopic Extended-A"), 0xab00, 0xab2f, ASFIRST, true },
    { N_("Meetei Mayek"), 0xabc0, 0xabf9, ASFIRST, true },
    { N_("Hangul Syllables"), 0xac00, 0xd7af, ASFIRST, true },
    { N_("High Surrogate"), 0xd800, 0xdbff, ASFIRST, false },  /* No characters defined */
    { N_("Surrogate High"), 0xd800, 0xdbff, ASFIRST, false },
    { N_("Surrogate High, Non Private Use"), 0xd800, 0xdb7f, ASFIRST, false },
    { N_("Surrogate High, Private Use"), 0xdb80, 0xdbff, ASFIRST, false },
    { N_("Low Surrogates"), 0xdc00, 0xdfff, ASFIRST, false },
    { N_("Private Use Area"), 0xe000, 0xf8ff, ASFIRST, true },
    { N_("Private Use"), 0xe000, 0xe0ff, ASFIRST, false },
	/* Boundary between private and corporate use is not fixed */
	/*  these should be safe... */
    { N_("Corporate Use"), 0xf500, 0xf8ff, 0xf730, false },
    { N_("MicroSoft Symbol Area"), 0xf000, 0xf0ff, 0xf020, false },
    { N_("CJK Compatibility Ideographs"), 0xf900, 0xfaff, ASFIRST, true },
    { N_("Alphabetic Presentation Forms"), 0xfb00, 0xfb4f, ASFIRST, false },
    { N_("Latin Ligatures"), 0xfb00, 0xfb06, ASFIRST, true },
    { N_("Armenian Ligatures"), 0xfb13, 0xfb17, ASFIRST, true },
    { N_("Hebrew Ligatures/Pointed Letters"), 0xfb1d, 0xfb4f, 0xfb2a, true },
    { N_("Arabic Presentation Forms A"), 0xfb50, 0xfdff, ASFIRST, true },
    { N_("Variation Selectors"), 0xfe00, 0xfe0f, ASFIRST, true },
    { N_("Vertical Forms"), 0xfe10, 0xfe1f, ASFIRST, true },
    { N_("Combining Half Marks"), 0xfe20, 0xfe2f, ASFIRST, true },
    { N_("CJK Compatibility Forms"), 0xfe30, 0xfe4f, ASFIRST, true },
    { N_("Small Form Variants"), 0xfe50, 0xfe6f, ASFIRST, true },
    { N_("Arabic Presentation Forms B"), 0xfe70, 0xfefe, ASFIRST, true },
    { N_("Byte Order Mark"), 0xfeff, 0xfeff, ASFIRST, true },
    { N_("Half and Full Width Forms"), 0xff00, 0xffef, 0xff01, true },
    { N_("Latin Full Width Forms"), 0xff01, 0xff5e, ASFIRST, true },
    { N_("KataKana Half Width Forms"), 0xff61, 0xff9f, ASFIRST, true },
    { N_("Hangul Jamo Half Width Forms"), 0xffa0, 0xffdc, ASFIRST, true },
    { N_("Specials"), 0xfff0, 0xfffd, 0xfffd, true },
    { N_("Not a Unicode Character"), 0xfffe, 0xfffe, ASFIRST, true },
    { N_("Signature Mark"), 0xffff, 0xffff, ASFIRST, true },
/* End of BMP, Start of SMP */
    { N_("Unicode Supplementary Multilingual Plane"), 0x10000, 0x1ffff, ASFIRST, false },
    { N_("Supplementary Multilingual Plane"), 0x10000, 0x1ffff, ASFIRST, 2 },
    { N_("Aegean scripts"), 0x10000, 0x102ff, ASFIRST, false },
    { N_("Linear B Syllabary"), 0x10000, 0x1007f, ASFIRST, true },
    { N_("Linear B Ideograms"), 0x10080, 0x100ff, ASFIRST, true },
    { N_("Aegean numbers"), 0x10100, 0x1003f, ASFIRST, true },
    { N_("Ancient Greek Numbers"), 0x10140, 0x1018f, ASFIRST, true },
    { N_("Ancient Symbols"), 0x10190, 0x101cf, ASFIRST, true },
    { N_("Phaistos Disc"), 0x101d0, 0x101ff, ASFIRST, true },
    { N_("Lycian"), 0x10280, 0x1029f, ASFIRST, true },
    { N_("Carian"), 0x102a0, 0x102df, ASFIRST, true },
    { N_("Alphabetic and syllabic LTR scripts"), 0x10300, 0x107ff, ASFIRST, true },
    { N_("Old Italic"), 0x10300, 0x1032f, ASFIRST, true },
    { N_("Gothic"), 0x10330, 0x1034f, ASFIRST, true },
    { N_("Ugaritic"), 0x10380, 0x1039f, ASFIRST, true },
    { N_("Old Persian"), 0x103a0, 0x103df, ASFIRST, true },
    { N_("Deseret"), 0x10400, 0x1044f, ASFIRST, true },
    { N_("Shavian"), 0x10450, 0x1047f, ASFIRST, true },
    { N_("Osmanya"), 0x10480, 0x104a9, ASFIRST, true },
    { N_("Alphabetic and syllabic RTL scripts"), 0x10800, 0x10fff, ASFIRST, false },
    { N_("Cypriot Syllabary"), 0x10800, 0x1083f, ASFIRST, true },
    { N_("Imperial Aramaic"), 0x10840, 0x1085f, ASFIRST, true },
    { N_("Phoenician"), 0x10900, 0x1091f, ASFIRST, true },
    { N_("Lydian"), 0x10920, 0x1093f, ASFIRST, true },
    { N_("Kharosthi"), 0x10a00, 0x10a5f, ASFIRST, true },
    { N_("Old South Arabian"), 0x10a60, 0x10a7f, ASFIRST, true },
    { N_("Avestan"), 0x10b00, 0x10b3f, ASFIRST, true },
    { N_("Inscriptional Parthian"), 0x10b40, 0x10b5f, ASFIRST, true },
    { N_("Inscriptional Pahlavi"), 0x10b60, 0x10b7f, ASFIRST, true },
    { N_("Old Turkic"), 0x10c00, 0x10c48, ASFIRST, true },
    { N_("Rumi Numeral Symbols"), 0x10e60, 0x10e7f, ASFIRST, true },
    /*{ N_("Brahmic scripts"), 0x11000, 0x117ff, ASFIRST, true },*/
    { N_("Brahmi"), 0x11000, 0x1107f, ASFIRST, true },
    { N_("Kaithi"), 0x11080, 0x110c1, ASFIRST, true },
    { N_("African and other syllabic scripts"), 0x11800, 0x11fff, ASFIRST, true },
    { N_("Cuneiform and other Near Eastern Scripts"), 0x12000, 0x12fff, ASFIRST, false },
    { N_("Cuneiform"), 0x12000, 0x123ff, ASFIRST, true },
    {  N_("Sumero-Akkadian Cuneiform"), 0x12000, 0x123ff, ASFIRST, true },
    { N_("Cuneiform Numbers"), 0x12400, 0x1247f, ASFIRST, true },
    /*{ N_("Undeciphered scripts"), 0x13000, 0x137ff, ASFIRST, true },*/
    { N_("Egyptian hieroglyphs"), 0x13000, 0x1342e, ASFIRST, true },
    { N_("North American ideographs and pictograms"), 0x13800, 0x13fff, ASFIRST, true },
    { N_("Egyptian and Mayan hieroglyphs"), 0x14000, 0x16bff, ASFIRST, true },
    { N_("Bamum Supplement"), 0x16800, 0x16a3f, ASFIRST, true },
    { N_("Sumerian pictograms"), 0x16c00, 0x16fff, ASFIRST, true },
    { N_("Large Asian Scripts"), 0x17000, 0x1b5ff, ASFIRST, true },
    { N_("Kana Supplement"), 0x1b000, 0x1b0ff, ASFIRST, true },
    { N_("Notational systems"), 0x1d000, 0x1ffd, ASFIRST, true },
    { N_("Byzantine Musical Symbols"), 0x1d000, 0x1d0ff, ASFIRST, true },
    { N_("Musical Symbols"), 0x1d100, 0x1d1ff, ASFIRST, true },
    { N_("Ancient Greek Musical Notation"), 0x1d200, 0x1d24f, ASFIRST, true },
    { N_("Tai Xuan Jing Symbols"), 0x1d300, 0x1d35f, ASFIRST, true },
    { N_("Chinese counting rod numerals"), 0x1d360, 0x1d371, ASFIRST, true },
    { N_("Mathematical Alphanumeric Symbols"), 0x1d400, 0x1d7ff, ASFIRST, true },
    { N_("Mahjong Tiles"), 0x1f000, 0x1f02f, ASFIRST, true },
    { N_("Domino Tiles"), 0x1f030, 0x1f09f, ASFIRST, true },
    { N_("Playing Cards"), 0x1f0a0, 0x1f0ff, ASFIRST, true },
    { N_("Enclosed Alphanumeric Supplement"), 0x1f100, 0x1f1ff, ASFIRST, true },
    { N_("Enclosed Ideographic Supplement"), 0x1f200, 0x1f2ff, ASFIRST, true },
    { N_("Miscellaneous Symbols And Pictographs"), 0x1f300, 0x1f5ff, ASFIRST, true },
    { N_("Emoticons"), 0x1f600, 0x1f64f, ASFIRST, true },
    { N_("Transport And Map Symbols"), 0x1f680, 0x1f6ff, ASFIRST, true },
    { N_("Alchemical Symbols"), 0x1f700, 0x1f77f, ASFIRST, true },
/* End of SMP, Start of SIP */
    { N_("Unicode Supplementary Ideographic Plane"), 0x20000, 0x2ffff, ASFIRST, false },
    { N_("Supplementary Ideographic Plane"), 0x20000, 0x2ffff, ASFIRST, 2 },
    { N_("CJK Unified Ideographs Extension B"), 0x20000, 0x2a6d6, ASFIRST, true },
    { N_("CJK Unified Ideographs Extension C"), 0x2a700, 0x2b73f, ASFIRST, true },
    { N_("CJK Unified Ideographs Extension D"), 0x2b740, 0x2b81f, ASFIRST, true },
    { N_("CJK Compatibility Ideographs Supplement"), 0x2f800, 0x2fa1f, ASFIRST, true },
/* End of SIP, Start of SSP */
    { N_("Unicode Supplementary Special-purpose Plane"), 0xe0000, 0xeffff, ASFIRST, false },
    { N_("Supplementary Special-purpose Plane"), 0xe0000, 0xeffff, ASFIRST, 2 },
    { N_("Tag characters"), 0xe0000, 0xe007f, ASFIRST, true },
    { N_("Variation Selectors B"), 0xe0110, 0xe01ff, ASFIRST, true },
/* End of SSP */
    { N_("Supplementary Private Use Area-A"), 0xf0000, 0xfffff, ASFIRST, 2 },
    { N_("Supplementary Private Use Area-B"), 0x100000, 0x10ffff, ASFIRST, 2 },
    { NULL }
};
int unicoderange_cnt = sizeof(unicoderange)/sizeof(unicoderange[0])-1;

static struct unicoderange nonunicode = { N_("Non-Unicode Glyphs"), -1, -1, ASFIRST, 2, 1 };
static struct unicoderange unassigned = { N_("Unassigned Code Points"), 0, 0x11ffff, ASFIRST, 2, 1 };
static struct unicoderange unassignedplanes[] = {
    { "<Unassigned Plane 3>",  0x30000, 0x3ffff, ASFIRST, 2, 1 },
    { "<Unassigned Plane 4>",  0x40000, 0x4ffff, ASFIRST, 2, 1 },
    { "<Unassigned Plane 5>",  0x50000, 0x5ffff, ASFIRST, 2, 1 },
    { "<Unassigned Plane 6>",  0x60000, 0x6ffff, ASFIRST, 2, 1 },
    { "<Unassigned Plane 7>",  0x70000, 0x7ffff, ASFIRST, 2, 1 },
    { "<Unassigned Plane 8>",  0x80000, 0x8ffff, ASFIRST, 2, 1 },
    { "<Unassigned Plane 9>",  0x90000, 0x9ffff, ASFIRST, 2, 1 },
    { "<Unassigned Plane 10>", 0xa0000, 0xaffff, ASFIRST, 2, 1 },
    { "<Unassigned Plane 11>", 0xb0000, 0xbffff, ASFIRST, 2, 1 },
    { "<Unassigned Plane 12>", 0xc0000, 0xcffff, ASFIRST, 2, 1 },
    { "<Unassigned Plane 13>", 0xd0000, 0xdffff, ASFIRST, 2, 1 },
    { NULL }
};

static int ucmp(const void *_ri1, const void *_ri2) {
    const struct rangeinfo *ri1 = _ri1, *ri2 = _ri2;

    if ( ri1->range->first == ri2->range->first ) {
	if ( ri1->range->last > ri2->range->last )
return( -1 );
	else if ( ri1->range->last < ri2->range->last )
return( 1 );
	else
return( 0 );
    } else
return( ri1->range->first - ri2->range->first );
}

static int ncmp(const void *_ri1, const void *_ri2) {
    const struct rangeinfo *ri1 = _ri1, *ri2 = _ri2;

return( strcoll(_(ri1->range->name),_(ri2->range->name)));
}

struct rangeinfo *SFUnicodeRanges(SplineFont *sf, enum ur_flags flags) {
    int cnt;
    int i,j, gid;
    struct rangeinfo *ri;
    static int initialized = false;

    if ( !initialized ) {
	initialized = true;
	for (i=cnt=0; unicoderange[i].name!=NULL; ++i ) if ( unicoderange[i].display ) {
	    int top = unicoderange[i].last;
	    int bottom = unicoderange[i].first;
	    int cnt = 0;
	    for ( j=bottom; j<=top; ++j ) {
		if ( isunicodepointassigned(j) )
		    ++cnt;
	    }
	    unicoderange[i].unassigned = (cnt==0);
	    unicoderange[i].actual = cnt;
	}
    }

    for (i=cnt=0; unicoderange[i].name!=NULL; ++i )
	if ( unicoderange[i].display )
	    ++cnt;
    for (i=0; unassignedplanes[i].name!=NULL; ++i )
	if ( unassignedplanes[i].display )
	    ++cnt;
    cnt+=2;		/* for nonunicode, unassigned codes */

    ri = gcalloc(cnt+1,sizeof(struct rangeinfo));

    for (i=cnt=0; unicoderange[i].name!=NULL; ++i )
	if ( unicoderange[i].display )
	    ri[cnt++].range = &unicoderange[i];
    for (i=0; unassignedplanes[i].name!=NULL; ++i )
	if ( unassignedplanes[i].display )
	    ri[cnt++].range = &unassignedplanes[i];
    ri[cnt++].range = &nonunicode;
    ri[cnt++].range = &unassigned;

    for ( j=0; j<cnt-2; ++j ) {
	int top = ri[j].range->last;
	int bottom = ri[j].range->first;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	    int u=sf->glyphs[gid]->unicodeenc;
	    if ( u>=bottom && u<=top &&
		    (ri[j].range->unassigned || isunicodepointassigned(u)) )
		++ri[j].cnt;
	}
    }

    /* non unicode glyphs (stylistic variations, etc.) */
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	int u=sf->glyphs[gid]->unicodeenc;
	if ( u<0 || u>0x11ffff )
	    ++ri[j].cnt;
    }

    /* glyphs attached to code points which have not been assigned in */
    /*  the version of unicode I know about (4.1 when this was written) */
    ++j;
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	int u=sf->glyphs[gid]->unicodeenc;
	if ( u>=0 && u<=0x11ffff && !isunicodepointassigned(u))
	    ++ri[j].cnt;
    }

    if ( !(flags&ur_includeempty) ) {
	for ( i=j=0; i<cnt; ++i ) {
	    if ( ri[i].cnt!=0 ) {
		if ( i!=j )
		    ri[j] = ri[i];
		++j;
	    }
	}
	ri[j].range = NULL;
	cnt = j;
    }

    if ( flags&ur_sortbyunicode )
	qsort(ri,cnt,sizeof(struct rangeinfo),ucmp);
    else if ( flags&ur_sortbyname )
	qsort(ri,cnt,sizeof(struct rangeinfo),ncmp);
return( ri );
}

const char *UnicodeRange(int unienc) {
    char *ret;
    struct unicoderange *best=NULL;
    int i;

    ret = "Unencoded Unicode";
    if ( unienc<0 )
return( ret );

    for ( i=0; unicoderange[i].name!=NULL; ++i ) {
	if ( unienc>=unicoderange[i].first && unienc<=unicoderange[i].last ) {
	    if ( best==NULL )
		best = &unicoderange[i];
	    else if (( unicoderange[i].first>best->first &&
			unicoderange[i].last<=best->last ) ||
		     ( unicoderange[i].first>=best->first &&
			unicoderange[i].last<best->last ))
		best = &unicoderange[i];
	}
    }
    if ( best!=NULL )
return(best->name);

return( ret );
}
