/* Copyright (C) 2001 by George Williams */
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
#include "ttfmodui.h"
#include <ustring.h>
#include <utype.h>

typedef struct namelist {
    int platform;
    int specific;
    int lang;
    int strtype;
    unichar_t *str;
} NameList;
/* Sortable by platform,specific,lang or by strtype */
/* double clicking brings up an editor window */
/* [New] button does too */

static GTextInfo platforms[] = {
    { (unichar_t *) _STR_AppleUnicode, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Mac, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ISO, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Microsoft, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo auspecific[] = {
    { (unichar_t *) _STR_Default, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Unicode1, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Unicode11, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Unicode2, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo macspecific[] = {
    { (unichar_t *) _STR_ApRoman, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApJapanese, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApChinese, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApKorean, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApArabic, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApHebrew, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApGreek, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApRussian, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApRSymbol, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApDevanagari, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApGurmukhi, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApGujarati, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApOriya, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApBengali, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApTamil, NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApTelugu, NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApKannada, NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApMalayalam, NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApSinhalese, NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApBurmese, NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApKhmer, NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApThai, NULL, 0, 0, (void *) 21, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApLoatian, NULL, 0, 0, (void *) 22, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApGeorgian, NULL, 0, 0, (void *) 23, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApArmenian, NULL, 0, 0, (void *) 24, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApMaldivian, NULL, 0, 0, (void *) 25, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApTibetan, NULL, 0, 0, (void *) 26, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApMongolian, NULL, 0, 0, (void *) 27, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApGeez, NULL, 0, 0, (void *) 28, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApSlavic, NULL, 0, 0, (void *) 29, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApVietnamese, NULL, 0, 0, (void *) 30, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApSindhi, NULL, 0, 0, (void *) 31, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Uninterpreted, NULL, 0, 0, (void *) 32, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo isospecific[] = {
    { (unichar_t *) _STR_ASCII, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ISO106461, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ISO88591, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo msspecific[] = {
    { (unichar_t *) _STR_Uninterpreted, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Unicode, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

static GTextInfo mslanguages[] = {
    { (unichar_t *) _STR_Albanian, NULL, 0, 0, (void *) 0x41c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Arabic, NULL, 0, 0, (void *) 0x401, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Basque, NULL, 0, 0, (void *) 0x42d, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Byelorussian, NULL, 0, 0, (void *) 0x423, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bulgarian, NULL, 0, 0, (void *) 0x402, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Catalan, NULL, 0, 0, (void *) 0x403, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MSChinese, NULL, 0, 0, (void *) 0x404, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Croatian, NULL, 0, 0, (void *) 0x41a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Czech, NULL, 0, 0, (void *) 0x405, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Danish, NULL, 0, 0, (void *) 0x406, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Dutch, NULL, 0, 0, (void *) 0x413, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Flemish, NULL, 0, 0, (void *) 0x813, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BrEnglish, NULL, 0, 0, (void *) 0x809, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_AmEnglish, NULL, 0, 0, (void *) 0x409, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CaEnglish, NULL, 0, 0, (void *) 0x1009, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_AuEnglish, NULL, 0, 0, (void *) 0xc09, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NZEnglish, NULL, 0, 0, (void *) 0x1409, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_IEEnglish, NULL, 0, 0, (void *) 0x1809, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Estonia, NULL, 0, 0, (void *) 0x425, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Finnish, NULL, 0, 0, (void *) 0x40b, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_FRFrench, NULL, 0, 0, (void *) 0x40c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BEFrench, NULL, 0, 0, (void *) 0x80c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CAFrench, NULL, 0, 0, (void *) 0xc0c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CHFrench, NULL, 0, 0, (void *) 0x100c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LUFrench, NULL, 0, 0, (void *) 0x140c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DEGerman, NULL, 0, 0, (void *) 0x407, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CHGerman, NULL, 0, 0, (void *) 0x807, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ATGerman, NULL, 0, 0, (void *) 0xc07, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LUGerman, NULL, 0, 0, (void *) 0x1007, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LIGerman, NULL, 0, 0, (void *) 0x1407, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Greek, NULL, 0, 0, (void *) 0x408, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Hebrew, NULL, 0, 0, (void *) 0x40d, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Hungarian, NULL, 0, 0, (void *) 0x402, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Icelandic, NULL, 0, 0, (void *) 0x40f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Italian, NULL, 0, 0, (void *) 0x410, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CHItalian, NULL, 0, 0, (void *) 0x810, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Japanese, NULL, 0, 0, (void *) 0x411, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Latvian, NULL, 0, 0, (void *) 0x426, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Lithuanian, NULL, 0, 0, (void *) 0x427, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Norwegian, NULL, 0, 0, (void *) 0x414, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NorwegianN, NULL, 0, 0, (void *) 0x814, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Polish, NULL, 0, 0, (void *) 0x415, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PTPortuguese, NULL, 0, 0, (void *) 0x416, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BRPortuguese, NULL, 0, 0, (void *) 0x816, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Romanian, NULL, 0, 0, (void *) 0x418, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Russian, NULL, 0, 0, (void *) 0x419, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Slovak, NULL, 0, 0, (void *) 0x41b, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Slovenian, NULL, 0, 0, (void *) 0x424, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_TradSpanish, NULL, 0, 0, (void *) 0x40a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MXSpanish, NULL, 0, 0, (void *) 0x80a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ModSpanish, NULL, 0, 0, (void *) 0xc0a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Swedish, NULL, 0, 0, (void *) 0x41d, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Turkish, NULL, 0, 0, (void *) 0x41f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Ukrainian, NULL, 0, 0, (void *) 0x422, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo maclanguages[] = {
    { (unichar_t *) _STR_ApEnglish, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApFrench, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApGerman, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApItalian, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApDutch, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApSwedish, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApSpanish, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApDanish, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApPortuguese, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApNorwegian, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApHebrew, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApJapanese, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApArabic, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApFinnish, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApGreek, NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApIcelandic, NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApMaltese, NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApTurkish, NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApYugoslavian, NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApChinese, NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApUrdu, NULL, 0, 0, (void *) 20, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApHindi, NULL, 0, 0, (void *) 21, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ApThai, NULL, 0, 0, (void *) 22, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

static GTextInfo ttfnameids[] = {
    { (unichar_t *) _STR_Copyright, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Family, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SubFamily, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 1, 0, 0, 1},
    { (unichar_t *) _STR_UniqueID, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Fullname, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Version, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PostscriptName, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Trademark, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Manufacturer, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Designer, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Descriptor, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VendorURL, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DesignerURL, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_License, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LicenseURL, NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
/* slot 15 is reserved */
    { (unichar_t *) _STR_OTFFamily, NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OTFStyles, NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CompatableFull, NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SampleText, NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
