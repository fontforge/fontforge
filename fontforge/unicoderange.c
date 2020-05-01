/* -*- coding: utf-8 -*- */
/* Copyright (C) 2006-2012 by George Williams */
/* 2012nov14, table updates, fixes added, Jose Da Silva */
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

#include <fontforge-config.h>

#include "fontforge.h"
#include "unicoderange.h"
#include "utype.h"

#include <stdlib.h>
#include <string.h>
#define ASFIRST (-1)

/* Block lists derived from nameslist.txt and second check with blocks.txt */
/* indented blocks are second or more way of describing same block (false) */
struct unicoderange unicoderange[] = {
    /* { N_("Unicode"), 0, 0x10ffff, ' ' }, */
    { N_("Unicode Basic Multilingual Plane"), 0, 0xffff, ' ', false, 0, 0 },
    { N_("Basic Multilingual Plane"), 0, 0xffff, ' ', 2, 0, 0 },
    { N_("Alphabetic"), 0, 0x1fff, 'A', false, 0, 0 },
//    { N_("C0 Controls and Basic Latin (Basic Latin)", 0, 0x7f, ASFIRST, true, 0, 0 },
    { N_("C0 Control Character"), 0, ' '-1, 0, true, 0, 0 },
    { N_("NUL, Default Character"), 0, 0, 0, false, 0, 0 },
    { N_("Basic Latin"), ' ', 0x7e, 'A', true, 0, 0 },
    { N_("Delete Character"), 0x7f, 0x7f, ASFIRST, true, 0, 0 },
//    { N_("C1 Controls and Latin-1 Supplement (Latin-1 Supplement)"), 0x80, 0xff, ASFIRST, true, 0, 0 },
    { N_("C1 Control Character"), 0x80, 0x9f, ASFIRST, true, 0, 0 },
    { N_("Latin-1 Supplement"), 0xa0, 0xff, 0xc0, true, 0, 0 }, /* unicode 80..ff */
    { N_("Latin Extended-A"), 0x100, 0x17f, ASFIRST, true, 0, 0 },
    { N_("Latin Extended-B"), 0x180, 0x24f, ASFIRST, true, 0, 0 },
    { N_("IPA Extensions"), 0x250, 0x2af, ASFIRST, true, 0, 0 },
    { N_("Spacing Modifier Letters"), 0x2b0, 0x2ff, ASFIRST, true, 0, 0 },
    { N_("Combining Diacritical Marks"), 0x300, 0x36f, ASFIRST, true, 0, 0 },
    { N_("Greek"), 0x370, 0x3ff, 0x391, true, 0, 0 },
    { N_("Greek and Coptic"), 0x370, 0x3ff, 0x391, true, 0, 0 },
    { N_("Cyrillic"), 0x400, 0x4ff, 0x410, true, 0, 0 },
    { N_("Cyrillic Supplement"), 0x500, 0x52f, ASFIRST, true, 0, 0 },
    { N_("Armenian"), 0x530, 0x58f, 0x531, true, 0, 0 },
    { N_("Hebrew"), 0x590, 0x5ff, 0x5d0, true, 0, 0 },
    { N_("Arabic"), 0x600, 0x6ff, 0x627, true, 0, 0 },
    { N_("Syriac"), 0x700, 0x74f, 0x710, true, 0, 0 },
    { N_("Arabic Supplement"), 0x750, 0x77f, ASFIRST, true, 0, 0 },
    { N_("Thaana"), 0x780, 0x7bf, ASFIRST, true, 0, 0 },
    { N_("NKo"), 0x7c0, 0x7ff, ASFIRST, true, 0, 0 },
    {  N_("N'Ko"), 0x7c0, 0x7ff, ASFIRST, false, 0, 0 },
    { N_("Samaritan"), 0x800, 0x83f, ASFIRST, true, 0, 0 },
    { N_("Samaritan, Punctuation"), 0x830, 0x83f, ASFIRST, true, 0, 0 },
    { N_("Mandaic"), 0x840, 0x85f, ASFIRST, true, 0, 0 },
    { N_("Syriac Supplement"), 0x860, 0x86f, ASFIRST, true, 0, 0 },
    { N_("Arabic Extended-A"), 0x8a0, 0x8ff, ASFIRST, true, 0, 0 },
    { N_("Devanagari"), 0x900, 0x97f, 0x905, true, 0, 0 },
    { N_("Bengali"), 0x980, 0x9ff, 0x985, true, 0, 0 },
    { N_("Gurmukhi"), 0xa00, 0xa7f, 0xa05, true, 0, 0 },
    { N_("Gujarati"), 0xa80, 0xaff, 0xa85, true, 0, 0 },
    { N_("Oriya"), 0xb00, 0xb7f, 0xb05, true, 0, 0 },
    { N_("Tamil"), 0xb80, 0xbff, 0xb85, true, 0, 0 },
    { N_("Telugu"), 0xc00, 0xc7f, 0xc05, true, 0, 0 },
    { N_("Kannada"), 0xc80, 0xcff, 0xc85, true, 0, 0 },
    { N_("Malayalam"), 0xd00, 0xd7f, 0xd05, true, 0, 0 },
    { N_("Sinhala"), 0xd80, 0xdff, 0xd85, true, 0, 0 },
    { N_("Thai"), 0xe00, 0xe7f, 0xe01, true, 0, 0 },
    { N_("Lao"), 0xe80, 0xeff, 0xe81, true, 0, 0 },
    { N_("Tibetan"), 0xf00, 0xfff, ASFIRST, true, 0, 0 },
    { N_("Myanmar"), 0x1000, 0x109f, ASFIRST, true, 0, 0 },
    { N_("Georgian"), 0x10a0, 0x10ff, 0x10d0, true, 0, 0 },
    { N_("Hangul Jamo, Choseong"), 0x1100, 0x115f, ASFIRST, true, 0, 0 },
    { N_("Hangul Jamo, Jungseong"), 0x1160, 0x11a7, 0x1161, true, 0, 0 },
    { N_("Hangul Jamo, Jongseong"), 0x11a8, 0x11ff, ASFIRST, true, 0, 0 },
    { N_("Ethiopic"), 0x1200, 0x137f, ASFIRST, true, 0, 0 },
    { N_("Ethiopic Supplement"), 0x1380, 0x139f, ASFIRST, true, 0, 0 },
    { N_("Cherokee"), 0x13a0, 0x13ff, ASFIRST, true, 0, 0 },
    { N_("Unified Canadian Aboriginal Syllabics"), 0x1400, 0x167f, ASFIRST, true, 0, 0 },
    { N_("Ogham"), 0x1680, 0x169f, ASFIRST, true, 0, 0 },
    { N_("Runic"), 0x16a0, 0x16ff, ASFIRST, true, 0, 0 },
    { N_("Tagalog"), 0x1700, 0x171f, ASFIRST, true, 0, 0 },
    { NU_("Hanunóo"), 0x1720, 0x173f, ASFIRST, true, 0, 0 },
    { N_("Buhid"), 0x1740, 0x175f, ASFIRST, true, 0, 0 },
    { N_("Tagbanwa"), 0x1760, 0x177f, ASFIRST, true, 0, 0 },
    { N_("Khmer"), 0x1780, 0x17ff, ASFIRST, true, 0, 0 },
    { N_("Mongolian"), 0x1800, 0x18af, ASFIRST, true, 0, 0 },
    { N_("Unified Canadian Aboriginal Syllabics Extended"), 0x18b0, 0x18ff, ASFIRST, true, 0, 0 },
    { N_("Limbu"), 0x1900, 0x194f, ASFIRST, true, 0, 0 },
    { N_("Tai Le"), 0x1950, 0x197f, ASFIRST, true, 0, 0 },
    { N_("New Tai Lue"), 0x1980, 0x19df, ASFIRST, true, 0, 0 },
    { N_("Khmer Symbols"), 0x19e0, 0x19ff, ASFIRST, true, 0, 0 },
    { N_("Buginese"), 0x1a00, 0x1a1f, ASFIRST, true, 0, 0 },
    { N_("Tai Tham"), 0x1a20, 0x1aaf, ASFIRST, true, 0, 0 },
    { N_("Combining Diacritical Marks Extended"), 0x1ab0, 0x1aff, ASFIRST, true, 0, 0 },
    { N_("Balinese"), 0x1b00, 0x1b7f, ASFIRST, true, 0, 0 },
    { N_("Sundanese"), 0x1b80, 0x1bbf, ASFIRST, true, 0, 0 },
    { N_("Batak"), 0x1bc0, 0x1bff, ASFIRST, true, 0, 0 },
    { N_("Lepcha"), 0x1c00, 0x1c4f, ASFIRST, true, 0, 0 },
    { N_("Ol Chiki"), 0x1c50, 0x1c7f, ASFIRST, true, 0, 0 },
    { N_("Cyrillic Extended-C"), 0x1c80, 0x1c8f, ASFIRST, true, 0, 0 },
    { N_("Georgian Extended"), 0x1c90, 0x1cbf, ASFIRST, true, 0, 0 },
    { N_("Sundanese Supplement"), 0x1cc0, 0x1ccf, ASFIRST, true, 0, 0 },
    { N_("Vedic Extensions"), 0x1cd0, 0x1cff, ASFIRST, true, 0, 0 },
    { N_("Phonetic Extensions"), 0x1d00, 0x1d7f, 0x1d02, true, 0, 0 },
    { N_("Phonetic Extensions Supplement"), 0x1d80, 0x1dbf, ASFIRST, true, 0, 0 },
    { N_("Combining Diacritical Marks Supplement"), 0x1dc0, 0x1dff, ASFIRST, true, 0, 0 },
    { N_("Latin Extended Additional"), 0x1e00, 0x1eff, ASFIRST, true, 0, 0 },
    { N_("Greek Extended"), 0x1f00, 0x1fff, ASFIRST, true, 0, 0 },
    { N_("Symbols"), 0x2000, 0x2bff, ASFIRST, false, 0, 0 },
    { N_("General Punctuation"), 0x2000, 0x206f, ASFIRST, true, 0, 0 },
    { N_("Superscripts and Subscripts"), 0x2070, 0x209f, ASFIRST, true, 0, 0 },
    {  N_("Super and Sub scripts"), 0x2070, 0x209f, ASFIRST, false, 0, 0 },
    { N_("Currency Symbols"), 0x20a0, 0x20cf, 0x20ac, true, 0, 0 },
    { N_("Combining Diacritical Marks for Symbols"), 0x20d0, 0x20ff, ASFIRST, true, 0, 0 },
    {  N_("Combining Marks for Symbols"), 0x20d0, 0x20ff, ASFIRST, false, 0, 0 },
    { N_("Letterlike Symbols"), 0x2100, 0x214f, ASFIRST, true, 0, 0 },
    { N_("Number Forms"), 0x2150, 0x218f, ASFIRST, true, 0, 0 },
    { N_("Arrows"), 0x2190, 0x21ff, ASFIRST, true, 0, 0 },
    { N_("Mathematical Operators"), 0x2200, 0x22ff, ASFIRST, true, 0, 0 },
    { N_("Miscellaneous Technical"), 0x2300, 0x23ff, ASFIRST, true, 0, 0 },
    {  N_("Miscellaneous Technical Symbols"), 0x2300, 0x23ff, ASFIRST, false, 0, 0 },
    {  N_("Technical Symbols Misc."), 0x2300, 0x23ff, ASFIRST, false, 0, 0 },
    { N_("Control Pictures"), 0x2400, 0x243f, ASFIRST, true, 0, 0 },
    { N_("Optical Character Recognition"), 0x2440, 0x245f, ASFIRST, true, 0, 0 },
    { N_("Enclosed Alphanumerics"), 0x2460, 0x24ff, ASFIRST, true, 0, 0 },
    { N_("Box Drawing"), 0x2500, 0x257f, ASFIRST, true, 0, 0 },
    { N_("Block Elements"), 0x2580, 0x259f, ASFIRST, true, 0, 0 },
    { N_("Geometric Shapes"), 0x25a0, 0x25ff, ASFIRST, true, 0, 0 },
    { N_("Miscellaneous Symbols"), 0x2600, 0x26ff, ASFIRST, true, 0, 0 },
    {  N_("Symbols Misc."), 0x2600, 0x267f, ASFIRST, false, 0, 0 },
    { N_("Dingbats"), 0x2700, 0x27bf, 0x2701, true, 0, 0 },
    {  N_("Zapf Dingbats"), 0x2700, 0x27bf, 0x2701, false, 0, 0 },		/* Synonym */
    { N_("Miscellaneous Mathematical Symbols-A"), 0x27c0, 0x27ef, ASFIRST, true, 0, 0 },
    {  N_("Miscellaneous Math Symbols-A"), 0x27c0, 0x27ef, ASFIRST, false, 0, 0 },
    {  N_("Math Misc. Symbols-A"), 0x27c0, 0x27ef, ASFIRST, false, 0, 0 },
    { N_("Supplemental Arrows-A"), 0x27f0, 0x27ff, ASFIRST, true, 0, 0 },
    {  N_("Arrows Supplement-A"), 0x27f0, 0x27ff, ASFIRST, false, 0, 0 },
    { N_("Braille Patterns"), 0x2800, 0x28ff, ASFIRST, true, 0, 0 },
    { N_("Supplemental Arrows-B"), 0x2900, 0x297f, ASFIRST, true, 0, 0 },
    {  N_("Arrows Supplement-B"), 0x2900, 0x297f, ASFIRST, false, 0, 0 },
    { N_("Miscellaneous Mathematical Symbols-B"), 0x2980, 0x29ff, ASFIRST, true, 0, 0 },
    {  N_("Miscellaneous Math Symbols-B"), 0x2980, 0x29ff, ASFIRST, false, 0, 0 },
    {  N_("Math Misc. Symbols-B"), 0x2980, 0x29ff, ASFIRST, false, 0, 0 },
    { N_("Supplemental Mathematical Operators"), 0x2a00, 0x2aff, ASFIRST, true, 0, 0 },
    {  N_("Supplemental Math Operators"), 0x2a00, 0x2aff, ASFIRST, false, 0, 0 },
    {  N_("Math Operators Supplement"), 0x2a00, 0x2aff, ASFIRST, false, 0, 0 },
    { N_("Miscellaneous Symbols and Arrows"), 0x2b00, 0x2bff, ASFIRST, true, 0, 0 },
    {  N_("Supplemental Symbols and Arrows"), 0x2b00, 0x2bff, ASFIRST, false, 0, 0 },
    {  N_("Symbols and Arrows Supplement"), 0x2b00, 0x2bff, ASFIRST, false, 0, 0 },
    { N_("Alphabetic Extended"), 0x2c00, 0x2dff, ASFIRST, false, 0, 0 },
    { N_("Glagolitic"), 0x2c00, 0x2c5f, ASFIRST, true, 0, 0 },
    { N_("Latin Extended-C"), 0x2c60, 0x2c7f, ASFIRST, true, 0, 0 },
    { N_("Coptic"), 0x2c80, 0x2cff, ASFIRST, true, 0, 0 },
    { N_("Georgian Supplement"), 0x2d00, 0x2d2f, ASFIRST, true, 0, 0 },
    { N_("Tifinagh"), 0x2d30, 0x2d7f, ASFIRST, true, 0, 0 },
    { N_("Ethiopic Extended"), 0x2d80, 0x2ddf, ASFIRST, true, 0, 0 },
    { N_("Cyrillic Extended-A"), 0x2de0, 0x2dff, ASFIRST, true, 0, 0 },
    { N_("Supplemental Punctuation"), 0x2e00, 0x2e7f, ASFIRST, true, 0, 0 },
    {  N_("Punctuation Supplement"), 0x2e00, 0x2e7f, ASFIRST, false, 0, 0 },
    { N_("CJK Radicals Supplement"), 0x2e80, 0x2eff, ASFIRST, true, 0, 0 },
    { N_("Kangxi Radicals"), 0x2f00, 0x2fdf, ASFIRST, true, 0, 0 },
    { N_("Ideographic Description Characters"), 0x2ff0, 0x2fff, ASFIRST, true, 0, 0 },
    { N_("CJK Phonetics and Symbols"), 0x3000, 0x33ff, ASFIRST, true, 0, 0 },
    { N_("CJK Symbols and Punctuation"), 0x3000, 0x303f, ASFIRST, true, 0, 0 },
    { N_("Hiragana"), 0x3040, 0x309f, 0x3041, true, 0, 0 },
    { N_("Katakana"), 0x30a0, 0x30ff, 0x30a1, true, 0, 0 },
    { N_("Bopomofo"), 0x3100, 0x312f, 0x3105, true, 0, 0 },
    { N_("Hangul Compatibility Jamo"), 0x3130, 0x318f, 0x3131, true, 0, 0 },
    { N_("Kanbun"), 0x3190, 0x319f, ASFIRST, true, 0, 0 },
    { N_("Bopomofo Extended"), 0x31a0, 0x31bf, ASFIRST, true, 0, 0 },
    { N_("CJK Strokes"), 0x31c0, 0x31ef, ASFIRST, true, 0, 0 },
    { N_("Katakana Phonetic Extensions"), 0x31f0, 0x31ff, ASFIRST, true, 0, 0 },
    { N_("Enclosed CJK Letters and Months"), 0x3200, 0x32ff, ASFIRST, true, 0, 0 },
    {  N_("CJK Enclosed Letters and Months"), 0x3200, 0x32ff, ASFIRST, false, 0, 0 },
    { N_("CJK Compatibility"), 0x3300, 0x33ff, ASFIRST, true, 0, 0 },
    { N_("CJK Unified Ideographs Extension A"), 0x3400, 0x4dbf, ASFIRST, true, 0, 0 },
    { N_("Yijing Hexagram Symbols"), 0x4dc0, 0x4dff, ASFIRST, true, 0, 0 },
    { N_("CJK Unified Ideographs"), 0x4e00, 0x9ffc, ASFIRST, true, 0, 0 },
    { N_("Yi Syllables"), 0xa000, 0xa48f, ASFIRST, true, 0, 0 },
    {  N_("Yi"), 0xa000, 0xa48f, ASFIRST, false, 0, 0 },
    { N_("Yi Radicals"), 0xa490, 0xa4cf, ASFIRST, true, 0, 0 },
    { N_("Lisu"), 0xa4d0, 0xa4ff, ASFIRST, true, 0, 0 },
    { N_("Vai"), 0xa500, 0xa63f, ASFIRST, true, 0, 0 },
    { N_("Cyrillic Extended-B"), 0xa640, 0xa69f, ASFIRST, true, 0, 0 },
    { N_("Bamum"), 0xa6a0, 0xa6ff, ASFIRST, true, 0, 0 },
    { N_("Modifier Tone Letters"), 0xa700, 0xa71f, ASFIRST, true, 0, 0 },
    { N_("Latin Extended-D"), 0xa720, 0xa7ff, ASFIRST, true, 0, 0 },
    { N_("Syloti Nagri"), 0xa800, 0xa82f, ASFIRST, true, 0, 0 },
    { N_("Common Indic Number Forms"), 0xa830, 0xa83f, ASFIRST, true, 0, 0 },
    { N_("Phags-pa"), 0xa840, 0xa87f, ASFIRST, true, 0, 0 },
    { N_("Saurashtra"), 0xa880, 0xa8df, ASFIRST, true, 0, 0 },
    { N_("Devanagari Extended"), 0xa8e0, 0xa8ff, ASFIRST, true, 0, 0 },
    { N_("Kayah Li"), 0xa900, 0xa92f, ASFIRST, true, 0, 0 },
    { N_("Rejang"), 0xa930, 0xa95f, ASFIRST, true, 0, 0 },
    { N_("Hangul Jamo Extended-A"), 0xa960, 0xa97f, ASFIRST, true, 0, 0 },
    { N_("Javanese"), 0xa980, 0xa9df, ASFIRST, true, 0, 0 },
    { N_("Myanmar Extended-B"), 0xa9e0, 0xa9ff, ASFIRST, true, 0, 0 },
    { N_("Cham"), 0xaa00, 0xaa5f, ASFIRST, true, 0, 0 },
    { N_("Myanmar Extended-A"), 0xaa60, 0xaa7f, ASFIRST, true, 0, 0 },
    { N_("Tai Viet"), 0xaa80, 0xaadf, ASFIRST, true, 0, 0 },
    { N_("Meetei Mayek Extensions"), 0xaae0, 0xaaff, ASFIRST, true, 0, 0 },
    { N_("Ethiopic Extended-A"), 0xab00, 0xab2f, ASFIRST, true, 0, 0 },
    { N_("Latin Extended-E"), 0xab30, 0xab6f, ASFIRST, true, 0, 0 },
    { N_("Cherokee Supplement"), 0xab70, 0xabbf, ASFIRST, true, 0, 0 },
    { N_("Meetei Mayek"), 0xabc0, 0xabff, ASFIRST, true, 0, 0 },
    { N_("Hangul Syllables"), 0xac00, 0xd7a3, ASFIRST, true, 0, 0 },
    { N_("Hangul Jamo Extended-B"), 0xd7b0, 0xd7ff, ASFIRST, true, 0, 0 },
/* Start of UTF-16 to UTF-32 surrogate area. No characters defined */
    { N_("High Surrogates"), 0xd800, 0xdbff, ASFIRST, false, 0, 0 }, /* Blocks.txt */
    {  N_("High Surrogate"), 0xd800, 0xdbff, ASFIRST, false, 0, 0 },  /* No characters defined */
    {  N_("Surrogate High"), 0xd800, 0xdbff, ASFIRST, false, 0, 0 },
    { N_("Surrogate High, Non Private Use"), 0xd800, 0xdb7f, ASFIRST, false, 0, 0 },
    { N_("Surrogate High, Private Use"), 0xdb80, 0xdbff, ASFIRST, false, 0, 0 },
    { N_("Low Surrogates"), 0xdc00, 0xdfff, ASFIRST, false, 0, 0 },
/* End of UTF-16 to UTF-32 surrogate area. No characters defined */
    { N_("Private Use Area"), 0xe000, 0xf8ff, ASFIRST, true, 0, 0 },
    {  N_("Private Use"), 0xe000, 0xe0ff, ASFIRST, false, 0, 0 },
	/* Boundary between private and corporate use is not fixed */
	/*  these should be safe... */
    { N_("Microsoft Symbol Area"), 0xf000, 0xf0ff, 0xf020, false, 0, 0 },
    { N_("Corporate Use"), 0xf500, 0xf8ff, 0xf730, false, 0, 0 },
    { N_("CJK Compatibility Ideographs"), 0xf900, 0xfaff, ASFIRST, true, 0, 0 },
    { N_("Alphabetic Presentation Forms"), 0xfb00, 0xfb4f, ASFIRST, true, 0, 0 },
    { N_("Latin Ligatures"), 0xfb00, 0xfb06, ASFIRST, true, 0, 0 },
    { N_("Armenian Ligatures"), 0xfb13, 0xfb17, ASFIRST, true, 0, 0 },
    { N_("Hebrew Ligatures/Pointed Letters"), 0xfb1d, 0xfb4f, 0xfb2a, true, 0, 0 },
    { N_("Arabic Presentation Forms-A"), 0xfb50, 0xfdff, ASFIRST, true, 0, 0 },
    { N_("Variation Selectors"), 0xfe00, 0xfe0f, ASFIRST, true, 0, 0 },
    { N_("Vertical Forms"), 0xfe10, 0xfe1f, ASFIRST, true, 0, 0 },
    { N_("Combining Half Marks"), 0xfe20, 0xfe2f, ASFIRST, true, 0, 0 },
    { N_("CJK Compatibility Forms"), 0xfe30, 0xfe4f, ASFIRST, true, 0, 0 },
    { N_("Small Form Variants"), 0xfe50, 0xfe6f, ASFIRST, true, 0, 0 },
    { N_("Arabic Presentation Forms-B"), 0xfe70, 0xfeff, ASFIRST, true, 0, 0 },
    { N_("Byte Order Mark"), 0xfeff, 0xfeff, ASFIRST, true, 0, 0 },
    { N_("Halfwidth and Fullwidth Forms"), 0xff00, 0xffef, 0xff01, true, 0, 0 },
    {  N_("Half and Full Width Forms"), 0xff00, 0xffef, 0xff01, false, 0, 0 },
    { N_("Latin Full Width Forms"), 0xff01, 0xff5e, ASFIRST, true, 0, 0 },
    { N_("Full Width Brackets"), 0xff5f, 0xff60, ASFIRST, true, 0, 0 },
    { N_("CJK Half Width Forms"), 0xff61, 0xff64, ASFIRST, true, 0, 0 },
    { N_("Katakana Half Width Forms"), 0xff65, 0xff9f, ASFIRST, true, 0, 0 },
    { N_("Hangul Jamo Half Width Forms"), 0xffa0, 0xffdf, ASFIRST, true, 0, 0 },
    { N_("Full Width Symbol Variants"), 0xffe0, 0xffe7, ASFIRST, true, 0, 0 },
    { N_("Half Width Symbol Variants"), 0xffe8, 0xffef, ASFIRST, true, 0, 0 },
    { N_("Specials"), 0xfff0, 0xfffd, 0xfffd, true, 0, 0 },
    { N_("Not a Unicode Character"), 0xfffe, 0xfffe, ASFIRST, true, 0, 0 },
    { N_("Signature Mark"), 0xffff, 0xffff, ASFIRST, true, 0, 0 },
/* End of BMP, Start of SMP */
    { N_("Unicode Supplementary Multilingual Plane"), 0x10000, 0x1ffff, ASFIRST, false, 0, 0 },
    {  N_("Supplementary Multilingual Plane"), 0x10000, 0x1ffff, ASFIRST, 2, 0, 0 },
    { N_("Aegean scripts"), 0x10000, 0x1018f, ASFIRST, false, 0, 0 },
    { N_("Linear B Syllabary"), 0x10000, 0x1007f, ASFIRST, true, 0, 0 },
    { N_("Linear B Ideograms"), 0x10080, 0x100ff, ASFIRST, true, 0, 0 },
    { N_("Aegean Numbers"), 0x10100, 0x1013f, ASFIRST, true, 0, 0 },
    { N_("Ancient Greek Numbers"), 0x10140, 0x1018f, ASFIRST, true, 0, 0 },
    { N_("Ancient Symbols"), 0x10190, 0x101cf, ASFIRST, true, 0, 0 },
    { N_("Phaistos Disc"), 0x101d0, 0x101ff, ASFIRST, true, 0, 0 },
    { N_("Lycian"), 0x10280, 0x1029f, ASFIRST, true, 0, 0 },
    { N_("Carian"), 0x102a0, 0x102df, ASFIRST, true, 0, 0 },
    { N_("Coptic Epact Numbers"), 0x102e0, 0x102ff, ASFIRST, true, 0, 0 },
    { N_("Alphabetic and syllabic LTR scripts"), 0x10300, 0x107ff, ASFIRST, true, 0, 0 },
    { N_("Old Italic"), 0x10300, 0x1032f, ASFIRST, true, 0, 0 },
    { N_("Gothic"), 0x10330, 0x1034f, ASFIRST, true, 0, 0 },
    { N_("Old Permic"), 0x10350, 0x1037f, ASFIRST, true, 0, 0 },
    { N_("Ugaritic"), 0x10380, 0x1039f, ASFIRST, true, 0, 0 },
    { N_("Old Persian"), 0x103a0, 0x103df, ASFIRST, true, 0, 0 },
    { N_("Deseret"), 0x10400, 0x1044f, ASFIRST, true, 0, 0 },
    { N_("Shavian"), 0x10450, 0x1047f, ASFIRST, true, 0, 0 },
    { N_("Osmanya"), 0x10480, 0x104af, ASFIRST, true, 0, 0 },
    { N_("Osage"), 0x104b0, 0x104ff, ASFIRST, true, 0, 0 },
    { N_("Elbasan"), 0x10500, 0x1052f, ASFIRST, true, 0, 0 },
    { N_("Caucasian Albanian"), 0x10530, 0x1056f, ASFIRST, true, 0, 0 },
    { N_("Linear A"), 0x10600, 0x1077f, ASFIRST, true, 0, 0 },
    { N_("Alphabetic and syllabic RTL scripts"), 0x10800, 0x10fff, ASFIRST, false, 0, 0 },
    { N_("Cypriot Syllabary"), 0x10800, 0x1083f, ASFIRST, true, 0, 0 },
    { N_("Imperial Aramaic"), 0x10840, 0x1085f, ASFIRST, true, 0, 0 },
    { N_("Palmyrene"), 0x10860, 0x1087f, ASFIRST, true, 0, 0 },
    { N_("Nabataean"), 0x10880, 0x108af, ASFIRST, true, 0, 0 },
    { N_("Hatran"), 0x108e0, 0x108ff, ASFIRST, true, 0, 0 },
    { N_("Phoenician"), 0x10900, 0x1091f, ASFIRST, true, 0, 0 },
    { N_("Lydian"), 0x10920, 0x1093f, ASFIRST, true, 0, 0 },
    { N_("Meroitic Hieroglyphs"), 0x10980, 0x1099f, ASFIRST, true, 0, 0 },
    { N_("Meroitic Cursive"), 0x109a0, 0x109ff, ASFIRST, true, 0, 0 },
    { N_("Kharoshthi"), 0x10a00, 0x10a5f, ASFIRST, true, 0, 0 },
    { N_("Old South Arabian"), 0x10a60, 0x10a7f, ASFIRST, true, 0, 0 },
    { N_("Old North Arabian"), 0x10a80, 0x10a9f, ASFIRST, true, 0, 0 },
    { N_("Manichaean"), 0x10ac0, 0x10aff, ASFIRST, true, 0, 0 },
    { N_("Avestan"), 0x10b00, 0x10b3f, ASFIRST, true, 0, 0 },
    { N_("Inscriptional Parthian"), 0x10b40, 0x10b5f, ASFIRST, true, 0, 0 },
    { N_("Inscriptional Pahlavi"), 0x10b60, 0x10b7f, ASFIRST, true, 0, 0 },
    { N_("Psalter Pahlavi"), 0x10b80, 0x10baf, ASFIRST, true, 0, 0 },
    { N_("Old Turkic"), 0x10c00, 0x10c4f, ASFIRST, true, 0, 0 },
    { N_("Old Hungarian"), 0x10c80, 0x10cff, ASFIRST, true, 0, 0 },
    { N_("Hanifi Rohingya"), 0x10d00, 0x10d3f, ASFIRST, true, 0, 0 },
    { N_("Rumi Numeral Symbols"), 0x10e60, 0x10e7f, ASFIRST, true, 0, 0 },
    { N_("Yezidi"), 0x10e80, 0x10ebf, ASFIRST, true, 0, 0 },
    { N_("Old Sogdian"), 0x10f00, 0x10f2f, ASFIRST, true, 0, 0 },
    { N_("Sogdian"), 0x10f30, 0x10f6f, ASFIRST, true, 0, 0 },
    { N_("Chorasmian"), 0x10fb0, 0x10fdf, ASFIRST, true, 0, 0 },
    { N_("Elymaic"), 0x10fe0, 0x10fff, ASFIRST, true, 0, 0 },
    /*{ N_("Brahmic scripts"), 0x11000, 0x11fff, ASFIRST, false, 0, 0 },*/
    { N_("Brahmi"), 0x11000, 0x1107f, ASFIRST, true, 0, 0 },
    { N_("Kaithi"), 0x11080, 0x110cf, ASFIRST, true, 0, 0 },
    { N_("Sora Sompeng"), 0x110d0, 0x110ff, ASFIRST, true, 0, 0 },
    { N_("Chakma"), 0x11100, 0x1114f, ASFIRST, true, 0, 0 },
    { N_("Mahajani"), 0x11150, 0x1117f, ASFIRST, true, 0, 0 },
    { N_("Sharada"), 0x11180, 0x111df, ASFIRST, true, 0, 0 },
    { N_("Sinhala Archaic Numbers"), 0x111e0, 0x111ff, ASFIRST, true, 0, 0 },
    { N_("Khojki"), 0x11200, 0x1124f, ASFIRST, true, 0, 0 },
    { N_("Multani"), 0x11280, 0x112af, ASFIRST, true, 0, 0 },
    { N_("Khudawadi"), 0x112b0, 0x112ff, ASFIRST, true, 0, 0 },
    { N_("Grantha"), 0x11300, 0x1137f, ASFIRST, true, 0, 0 },
    { N_("Newa"), 0x11400, 0x1147f, ASFIRST, true, 0, 0 },
    { N_("Tirhuta"), 0x11480, 0x114df, ASFIRST, true, 0, 0 },
    { N_("Siddham"), 0x11580, 0x115ff, ASFIRST, true, 0, 0 },
    { N_("Modi"), 0x11600, 0x1165f, ASFIRST, true, 0, 0 },
    { N_("Mongolian Supplement"), 0x11660, 0x1167f, ASFIRST, true, 0, 0 },
    { N_("Takri"), 0x11680, 0x116cf, ASFIRST, true, 0, 0 },
    { N_("Ahom"), 0x11700, 0x1173f, ASFIRST, true, 0, 0 },
    { N_("Dogra"), 0x11800, 0x1184f, ASFIRST, true, 0, 0 },
    { N_("Warang Citi"), 0x118a0, 0x118ff, ASFIRST, true, 0, 0 },
    { N_("Dives Akuru"), 0x11900, 0x1195f, ASFIRST, true, 0, 0 },
    { N_("Nandinagari"), 0x119a0, 0x119ff, ASFIRST, true, 0, 0 },
    { N_("Zanabazar Square"), 0x11a00, 0x11a4f, ASFIRST, true, 0, 0 },
    { N_("Soyombo"), 0x11a50, 0x11aaf, ASFIRST, true, 0, 0 },
    { N_("Pau Cin Hau"), 0x11ac0, 0x11aff, ASFIRST, true, 0, 0 },
    { N_("Bhaiksuki"), 0x11c00, 0x11c6f, ASFIRST, true, 0, 0 },
    { N_("Marchen"), 0x11c70, 0x11cbf, ASFIRST, true, 0, 0 },
    { N_("Masaram Gondi"), 0x11d00, 0x11d5f, ASFIRST, true, 0, 0 },
    { N_("Gunjala Gondi"), 0x11d60, 0x11daf, ASFIRST, true, 0, 0 },
    { N_("Makasar"), 0x11ee0, 0x11eff, ASFIRST, true, 0, 0 },
    { N_("Lisu Supplement"), 0x11fb0, 0x11fbf, ASFIRST, true, 0, 0 },
    { N_("Tamil Supplement"), 0x11fc0, 0x11fff, ASFIRST, true, 0, 0 },
    { N_("Cuneiform and other ancient scripts"), 0x12000, 0x12fff, ASFIRST, false, 0, 0 },
    { N_("Cuneiform"), 0x12000, 0x123ff, ASFIRST, true, 0, 0 },
//    {  N_("Sumero-Akkadian Cuneiform"), 0x12000, 0x123ff, ASFIRST, false, 0, 0 },
    { N_("Cuneiform Numbers and Punctuation"), 0x12400, 0x1247f, ASFIRST, true, 0, 0 },
    {  N_("Cuneiform Numbers"), 0x12400, 0x1247f, ASFIRST, false, 0, 0 },
    { N_("Early Dynastic Cuneiform"), 0x12480, 0x1254f, ASFIRST, true, 0, 0 },
    { N_("Egyptian Hieroglyphs"), 0x13000, 0x1342f, ASFIRST, true, 0, 0 },
    { N_("Egyptian Hieroglyph Format Controls"), 0x13430, 0x1343f, ASFIRST, true, 0, 0 },
    { N_("Anatolian Hieroglyphs"), 0x14400, 0x1467f, ASFIRST, true, 0, 0 },
    { N_("Bamum Supplement"), 0x16800, 0x16a3f, ASFIRST, true, 0, 0 },
    { N_("Mro"), 0x16a40, 0x16a6f, ASFIRST, true, 0, 0 },
    { N_("Bassa Vah"), 0x16ad0, 0x16aff, ASFIRST, true, 0, 0 },
    { N_("Pahawh Hmong"), 0x16b00, 0x16b8f, ASFIRST, true, 0, 0 },
    { N_("Medefaidrin"), 0x16e40, 0x16e9f, ASFIRST, true, 0, 0 },
    { N_("Miao"), 0x16f00, 0x16f9f, ASFIRST, true, 0, 0 },
    { N_("Ideographic Symbols and Punctuation"), 0x16fe0, 0x16fff, ASFIRST, true, 0, 0 },
//    { N_("Large Asian Scripts"), 0x17000, 0x1b5ff, ASFIRST, true, 0, 0 },
    { N_("Tangut"), 0x17000, 0x187f7, ASFIRST, true, 0, 0 },
    { N_("Tangut Components"), 0x18800, 0x18aff, ASFIRST, true, 0, 0 },
    { N_("Khitan Small Script"), 0x18b00, 0x18cff, ASFIRST, true, 0, 0 },
    { N_("Tangut Supplement"), 0x18d00, 0x18d08, ASFIRST, true, 0, 0 },
    { N_("Kana Supplement"), 0x1b000, 0x1b0ff, ASFIRST, true, 0, 0 },
    { N_("Kana Extended-A"), 0x1b100, 0x1b12f, ASFIRST, true, 0, 0 },
    { N_("Small Kana Extension"), 0x1b130, 0x1b16f, ASFIRST, true, 0, 0 },
    { N_("Nushu"), 0x1b170, 0x1b2ff, ASFIRST, true, 0, 0 },
    { N_("Duployan"), 0x1bc00, 0x1bc9f, ASFIRST, true, 0, 0 },
    { N_("Shorthand Format Controls"), 0x1bca0, 0x1bcaf, ASFIRST, true, 0, 0 },
//    { N_("Notational systems"), 0x1d000, 0x1dfff, ASFIRST, true, 0, 0 },
    { N_("Byzantine Musical Symbols"), 0x1d000, 0x1d0ff, ASFIRST, true, 0, 0 },
    { N_("Musical Symbols"), 0x1d100, 0x1d1ff, ASFIRST, true, 0, 0 },
    { N_("Ancient Greek Musical Notation"), 0x1d200, 0x1d24f, ASFIRST, true, 0, 0 },
    { N_("Mayan Numerals"), 0x1d2e0, 0x1d2ff, ASFIRST, true, 0, 0 },
    { N_("Tai Xuan Jing Symbols"), 0x1d300, 0x1d35f, ASFIRST, true, 0, 0 },
    { N_("Counting Rod Numerals"), 0x1d360, 0x1d37f, ASFIRST, true, 0, 0 },
    {  N_("Chinese Counting Rod Numerals"), 0x1d360, 0x1d37f, ASFIRST, false, 0, 0 },
    { N_("Mathematical Alphanumeric Symbols"), 0x1d400, 0x1d7ff, ASFIRST, true, 0, 0 },
    { N_("Sutton SignWriting"), 0x1d800, 0x1daaf, ASFIRST, true, 0, 0 },
    { N_("Glagolitic Supplement"), 0x1e000, 0x1e02f, ASFIRST, true, 0, 0 },
    { N_("Nyiakeng Puachue Hmong"), 0x1e100, 0x1e14f, ASFIRST, true, 0, 0 },
    { N_("Wancho"), 0x1e2c0, 0x1e2ff, ASFIRST, true, 0, 0 },
    { N_("Mende Kikakui"), 0x1e800, 0x1e8df, ASFIRST, true, 0, 0 },
    { N_("Adlam"), 0x1e900, 0x1e95f, ASFIRST, true, 0, 0 },
    { N_("Indic Siyaq Numbers"), 0x1ec70, 0x1ecbf, ASFIRST, true, 0, 0 },
    { N_("Ottoman Siyaq Numbers"), 0x1ed00, 0x1ed4f, ASFIRST, true, 0, 0 },
    { N_("Arabic Mathematical Alphabetic Symbols"), 0x1ee00, 0x1eeff, ASFIRST, true, 0, 0 },
    { N_("Mahjong Tiles"), 0x1f000, 0x1f02f, ASFIRST, true, 0, 0 },
    { N_("Domino Tiles"), 0x1f030, 0x1f09f, ASFIRST, true, 0, 0 },
    { N_("Playing Cards"), 0x1f0a0, 0x1f0ff, ASFIRST, true, 0, 0 },
    { N_("Enclosed Alphanumeric Supplement"), 0x1f100, 0x1f1ff, ASFIRST, true, 0, 0 },
    { N_("Enclosed Ideographic Supplement"), 0x1f200, 0x1f2ff, ASFIRST, true, 0, 0 },
    { N_("Miscellaneous Symbols and Pictographs"), 0x1f300, 0x1f5ff, ASFIRST, true, 0, 0 },
    { N_("Emoticons"), 0x1f600, 0x1f64f, ASFIRST, true, 0, 0 },
    { N_("Ornamental Dingbats"), 0x1f650, 0x1f67f, ASFIRST, true, 0, 0 },
    { N_("Transport and Map Symbols"), 0x1f680, 0x1f6ff, ASFIRST, true, 0, 0 },
    { N_("Alchemical Symbols"), 0x1f700, 0x1f77f, ASFIRST, true, 0, 0 },
    { N_("Geometric Shapes Extended"), 0x1f780, 0x1f7ff, ASFIRST, true, 0, 0 },
    { N_("Supplemental Arrows-C"), 0x1f800, 0x1f8ff, ASFIRST, true, 0, 0 },
    { N_("Supplemental Symbols and Pictographs"), 0x1f900, 0x1f9ff, ASFIRST, true, 0, 0 },
    { N_("Chess Symbols"), 0x1fa00, 0x1fa6f, ASFIRST, true, 0, 0 },
    { N_("Symbols and Pictographs Extended-A"), 0x1fa70, 0x1faff, ASFIRST, true, 0, 0 },
    { N_("Symbols for Legacy Computing"), 0x1fb00, 0x1fbff, ASFIRST, true, 0, 0 },
/* End of SMP, Start of SIP */
    { N_("Unicode Supplementary Ideographic Plane"), 0x20000, 0x2ffff, ASFIRST, false, 0, 0 },
    {  N_("Supplementary Ideographic Plane"), 0x20000, 0x2ffff, ASFIRST, 2, 0, 0 },
    { N_("CJK Unified Ideographs Extension B"), 0x20000, 0x2a6dd, ASFIRST, true, 0, 0 },
    { N_("CJK Unified Ideographs Extension C"), 0x2a700, 0x2b734, ASFIRST, true, 0, 0 },
    { N_("CJK Unified Ideographs Extension D"), 0x2b740, 0x2b81d, ASFIRST, true, 0, 0 },
    { N_("CJK Unified Ideographs Extension E"), 0x2b820, 0x2ceaf, ASFIRST, true, 0, 0 },
    { N_("CJK Unified Ideographs Extension F"), 0x2ceb0, 0x2ebe0, ASFIRST, true, 0, 0 },
    { N_("CJK Compatibility Ideographs Supplement"), 0x2f800, 0x2fa1f, ASFIRST, true, 0, 0 },
/* End of SIP, Start of TIP */
    { N_("Unicode Tertiary Ideographic Plane"), 0x30000, 0x3ffff, ASFIRST, false, 0, 0 },
    {  N_("Tertiary Ideographic Plane"), 0x30000, 0x3ffff, ASFIRST, 2, 0, 0 },
    { N_("CJK Unified Ideographs Extension G"), 0x30000, 0x3134a, ASFIRST, true, 0, 0 },
/* End of TIP, Start of SSP */
    { N_("Unicode Supplementary Special-purpose Plane"), 0xe0000, 0xeffff, ASFIRST, false, 0, 0 },
    {  N_("Supplementary Special-purpose Plane"), 0xe0000, 0xeffff, ASFIRST, 2, 0, 0 },
    { N_("Tags"), 0xe0000, 0xe007f, ASFIRST, true, 0, 0 },
    {  N_("Tag Characters"), 0xe0000, 0xe007f, ASFIRST, false, 0, 0 },
    { N_("Variation Selectors Supplement"), 0xe0100, 0xe01ef, ASFIRST, true, 0, 0 },
    {  N_("Variation Selectors B"), 0xe0100, 0xe01ef, ASFIRST, false, 0, 0 },
/* End of SSP */
    { N_("Supplementary Private Use Area-A"), 0xf0000, 0xfffff, ASFIRST, 2, 0, 0 },
    { N_("Supplementary Private Use Area-B"), 0x100000,0x10fffd, ASFIRST, 2, 0, 0 },
    UNICODERANGE_EMPTY
};
int unicoderange_cnt = sizeof(unicoderange)/sizeof(unicoderange[0])-1;

static struct unicoderange nonunicode = { N_("Non-Unicode Glyphs"), -1, -1, ASFIRST, 2, 1, 0 };
static struct unicoderange unassigned = { N_("Unassigned Code Points"), 0, 0x11ffff, ASFIRST, 2, 1, 0 };
static struct unicoderange unassignedplanes[] = {
    { N_("<Unassigned Plane 3>"),  0x30000, 0x3ffff, ASFIRST, 2, 1, 0 },
    { N_("<Unassigned Plane 4>"),  0x40000, 0x4ffff, ASFIRST, 2, 1, 0 },
    { N_("<Unassigned Plane 5>"),  0x50000, 0x5ffff, ASFIRST, 2, 1, 0 },
    { N_("<Unassigned Plane 6>"),  0x60000, 0x6ffff, ASFIRST, 2, 1, 0 },
    { N_("<Unassigned Plane 7>"),  0x70000, 0x7ffff, ASFIRST, 2, 1, 0 },
    { N_("<Unassigned Plane 8>"),  0x80000, 0x8ffff, ASFIRST, 2, 1, 0 },
    { N_("<Unassigned Plane 9>"),  0x90000, 0x9ffff, ASFIRST, 2, 1, 0 },
    { N_("<Unassigned Plane 10>"), 0xa0000, 0xaffff, ASFIRST, 2, 1, 0 },
    { N_("<Unassigned Plane 11>"), 0xb0000, 0xbffff, ASFIRST, 2, 1, 0 },
    { N_("<Unassigned Plane 12>"), 0xc0000, 0xcffff, ASFIRST, 2, 1, 0 },
    { N_("<Unassigned Plane 13>"), 0xd0000, 0xdffff, ASFIRST, 2, 1, 0 },
    UNICODERANGE_EMPTY
};

static int ucmp(const void *_ri1, const void *_ri2) {
/* Compare unicode range1 to unicode range2. This routine is used to help */
/* sort a list of ranges, returning a small value {-1,0,+1} if the start  */
/* is the same, but the last is different, else returning a large diff if */
/* the start of the two ranges are different. This routine used by qsort. */
    const struct rangeinfo *ri1 = _ri1, *ri2 = _ri2;

    if ( ri1->range->first == ri2->range->first ) {
	/* These two ranges are similar since they begin from same point. */
	/* Generic descriptions listed first, more precise listed after.  */
	if ( ri1->range->last > ri2->range->last )
	    return( -1 ); /* range1 is larger than range2, keep it ahead  */
	else if ( ri1->range->last < ri2->range->last )
	    return( 1 ); /* range2 should be sorted ahead of range1 */
	else
	    return( 0 ); /* both ranges appear the same - no need to sort */
    } else
	/* sort according to which range comes first */
	return( ri1->range->first - ri2->range->first );
}

static int ncmp(const void *_ri1, const void *_ri2) {
/* Compare two ranges using Unicode name. This routine is used by qsort() */
    const struct rangeinfo *ri1 = _ri1, *ri2 = _ri2;

    /* anything without a name goes to the end of the list */
    if ( ri1->range->name==NULL ) return( 1 );
    if ( ri2->range->name==NULL ) return( -1 );

    return( strcoll(_(ri1->range->name),_(ri2->range->name)));
}

struct rangeinfo *SFUnicodeRanges(SplineFont *sf, enum ur_flags flags) {
/* Find and return the Unicode range descriptions for these characters */
/* Return NULL if out of memory to hold rangeinfo[cnt]. */
    int cnt;
    int i, gid;
    int32 j;
    struct rangeinfo *ri;
    static int initialized = false;

    if ( !initialized ) {
	initialized = true;
	for (i=cnt=0; unicoderange[i].name!=NULL; ++i ) if ( unicoderange[i].display ) {
	    int32 top = unicoderange[i].last;
	    int32 bottom = unicoderange[i].first;
	    int cnt = 0;
	    for ( j=bottom; j<=top; ++j ) {
		if ( isunicodepointassigned(j) )
		    ++cnt;
	    }
	    unicoderange[i].unassigned = (cnt==0);
	    unicoderange[i].actual = cnt;
	}
    }

    /* count number of ranges to return back in rangeinfo[cnt] */
    for (i=cnt=0; unicoderange[i].name!=NULL; ++i )
	if ( unicoderange[i].display )
	    ++cnt;
    for (i=0; unassignedplanes[i].name!=NULL; ++i )
	if ( unassignedplanes[i].display )
	    ++cnt;
    cnt+=2;		/* for nonunicode, unassigned codes */

    /* create memory space needed to return back rangeinfo[cnt] */
    if ( (ri=calloc(cnt+1,sizeof(struct rangeinfo)))==NULL ) {
	NoMoreMemMessage();
	return( NULL );
    }

    /* populate rangeinfo[0..cnt-1] with unicoderanges we will display */
    for (i=cnt=0; unicoderange[i].name!=NULL; ++i )
	if ( unicoderange[i].display )
	    ri[cnt++].range = &unicoderange[i];
    for (i=0; unassignedplanes[i].name!=NULL; ++i )
	if ( unassignedplanes[i].display )
	    ri[cnt++].range = &unassignedplanes[i];
    ri[cnt++].range = &nonunicode;
    ri[cnt++].range = &unassigned;

    /* count glyphs in each range */
    for ( j=0; j<cnt-2; ++j ) {
	int32 top = ri[j].range->last;
	int32 bottom = ri[j].range->first;
	for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	    int32 u=sf->glyphs[gid]->unicodeenc;
	    if ( u>=bottom && u<=top &&
		    (ri[j].range->unassigned || isunicodepointassigned(u)) )
		++ri[j].cnt;
	}
    }

    /* non unicode glyphs (stylistic variations, etc.) */
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	int32 u=sf->glyphs[gid]->unicodeenc;
	if ( u<0 || u>0x11ffff )
	    ++ri[j].cnt;
    }

    /* glyphs attached to code points which have not been assigned in */
    /*  the version of unicode I know about (4.1 when this was written) */
    ++j;
    for ( gid=0; gid<sf->glyphcnt; ++gid ) if ( sf->glyphs[gid]!=NULL ) {
	int32 u=sf->glyphs[gid]->unicodeenc;
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
	qsort(ri,cnt,sizeof(struct rangeinfo),ucmp); /* sort by ranges */
    else if ( flags&ur_sortbyname )
	qsort(ri,cnt,sizeof(struct rangeinfo),ncmp); /* sort by names  */
return( ri );
}

const char *UnicodeRange(int32 unienc) {
/* Return the best name that describes this Unicode value */
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
