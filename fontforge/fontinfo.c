/* Copyright (C) 2000-2004 by George Williams */
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
#include "pfaeditui.h"
#include <ustring.h>
#include <chardata.h>
#include <utype.h>
#include <gkeysym.h>
#include <math.h>

static int lastplane=1;		/* SMP, Supplementary Multilingual Plane */
static int last_aspect=0;

struct gfi_data {
    SplineFont *sf;
    GWindow gw;
    int private_aspect, ttfv_aspect, panose_aspect, tn_aspect, tx_aspect;
    int old_sel, old_aspect, old_lang, old_strid;
    int oldcnt;
    int ttf_set, names_set, tex_set;
    struct psdict *private;
    struct ttflangname *names;
    struct ttflangname def;
    int uplane;
    unsigned int family_untitled: 1;
    unsigned int human_untitled: 1;
    unsigned int done: 1;
    unsigned int mpdone: 1;
    struct anchor_shows { CharView *cv; SplineChar *sc; int restart; } anchor_shows[2];
    struct texdata texdata;
    struct contextchaindlg *ccd;
    struct statemachinedlg *smd;
};

GTextInfo emsizes[] = {
    { (unichar_t *) "1000", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "1024", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "2048", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) "4096", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }
};
GTextInfo encodingtypes[] = {
    { (unichar_t *) _STR_Custom, NULL, 0, 0, (void *) em_custom, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Compacted, NULL, 0, 0, (void *) em_compacted, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Original, NULL, 0, 0, (void *) em_original, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Isolatin1, NULL, 0, 0, (void *) em_iso8859_1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin0, NULL, 0, 0, (void *) em_iso8859_15, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin2, NULL, 0, 0, (void *) em_iso8859_2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin3, NULL, 0, 0, (void *) em_iso8859_3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin4, NULL, 0, 0, (void *) em_iso8859_4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin5, NULL, 0, 0, (void *) em_iso8859_9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin6, NULL, 0, 0, (void *) em_iso8859_10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin7, NULL, 0, 0, (void *) em_iso8859_13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isolatin8, NULL, 0, 0, (void *) em_iso8859_14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Isocyrillic, NULL, 0, 0, (void *) em_iso8859_5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Koi8cyrillic, NULL, 0, 0, (void *) em_koi8_r, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isoarabic, NULL, 0, 0, (void *) em_iso8859_6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isogreek, NULL, 0, 0, (void *) em_iso8859_7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isohebrew, NULL, 0, 0, (void *) em_iso8859_8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Isothai, NULL, 0, 0, (void *) em_iso8859_11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_MacLatin, NULL, 0, 0, (void *) em_mac, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Win, NULL, 0, 0, (void *) em_win, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Adobestd, NULL, 0, 0, (void *) em_adobestandard, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Symbol, NULL, 0, 0, (void *) em_symbol, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Texbase, NULL, 0, 0, (void *) em_base, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Unicode, NULL, 0, 0, (void *) em_unicode, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Unicode4, NULL, 0, 0, (void *) em_unicode4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UnicodePlanes, NULL, 0, 0, (void *) em_unicodeplanes, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_SJIS, NULL, 0, 0, (void *) em_sjis, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Jis208, NULL, 0, 0, (void *) em_jis208, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Jis212, NULL, 0, 0, (void *) em_jis212, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_KoreanWansung, NULL, 0, 0, (void *) em_wansung, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Korean, NULL, 0, 0, (void *) em_ksc5601, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_KoreanJohab, NULL, 0, 0, (void *) em_johab, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Chinese, NULL, 0, 0, (void *) em_gb2312, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ChinesePacked, NULL, 0, 0, (void *) em_jisgb, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ChineseTrad, NULL, 0, 0, (void *) em_big5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ChineseTradHKSCS, NULL, 0, 0, (void *) em_big5hkscs, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
GTextInfo interpretations[] = {
    { (unichar_t *) _STR_None_fem, NULL, 0, 0, (void *) ui_none, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_AdobePUA, NULL, 0, 0, (void *) ui_adobe, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MacGreek, NULL, 0, 0, (void *) ui_greek, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MacJapanese, NULL, 0, 0, (void *) ui_japanese, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MacTraditionalChinese, NULL, 0, 0, (void *) ui_trad_chinese, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MacSimplifiedChinese, NULL, 0, 0, (void *) ui_simp_chinese, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MacKorean, NULL, 0, 0, (void *) ui_korean, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
GTextInfo macstyles[] = {
    { (unichar_t *) _STR_Bold, NULL, 0, 0, (void *) sf_bold, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Italic, NULL, 0, 0, (void *) sf_italic, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Condense, NULL, 0, 0, (void *) sf_condense, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Expand, NULL, 0, 0, (void *) sf_extend, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Underline, NULL, 0, 0, (void *) sf_underline, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Outline, NULL, 0, 0, (void *) sf_outline, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ShadowNoMn, NULL, 0, 0, (void *) sf_shadow, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo widthclass[] = {
    { (unichar_t *) _STR_UltraCondensed, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ExtraCondensed, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Condensed75, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SemiCondensed, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Medium100, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SemiExpanded, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Expanded125, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ExtraExpanded, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UltraExpanded, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo weightclass[] = {
    { (unichar_t *) _STR_Thin100, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ExtraLight200, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Light300, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Book400, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Medium500, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DemiBold600, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bold700, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Heavy800, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Black900, NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo fstype[] = {
    { (unichar_t *) _STR_NeverEmbeddable, NULL, 0, 0, (void *) 0x02, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OnlyPrint, NULL, 0, 0, (void *) 0x04, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_EditableDoc, NULL, 0, 0, (void *) 0x08, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Installable, NULL, 0, 0, (void *) 0x00, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo pfmfamily[] = {
    { (unichar_t *) _STR_Serif, NULL, 0, 0, (void *) 0x11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SansSerif, NULL, 0, 0, (void *) 0x21, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Monospace, NULL, 0, 0, (void *) 0x31, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Script, NULL, 0, 0, (void *) 0x41, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Decorative, NULL, 0, 0, (void *) 0x51, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panfamily[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_TextDisplay, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Script, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Decorative, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Pictoral, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panserifs[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Cove, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObtuseCove, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SquareCove, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObtuseSquareCove, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Square, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Thin, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bone, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Exaggerated, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Triangle, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalSans, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObtuseSans, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PerpSans, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Flared, NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Rounded, NULL, 0, 0, (void *) 15, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panweight[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VeryLight, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Light, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Thin, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Book, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Medium, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Demi, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bold, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Heavy, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Black, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Nord, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panprop[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OldStyle, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Modern, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_EvenWidth, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Expanded, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Condensed, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VeryExpanded, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VeryCondensed, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Monospaced, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo pancontrast[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_None, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VeryLow, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Low, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MediumLow, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Medium, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MediumHigh, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_High, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VeryHigh, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panstrokevar[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GradDiag, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GradTrans, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GradVert, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GradHor, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_RapidVert, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_RapidHor, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_InstantVert, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panarmstyle[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StraightArmsH, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StraightArmsW, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StraightArmsV, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StraightArmsSS, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StraightArmsDS, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NStraightArmsH, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NStraightArmsW, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NStraightArmsV, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NStraightArmsSS, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NStraightArmsDS, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panletterform[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalContact, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalWeighted, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalBoxed, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalFlattened, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalRounded, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalOffCenter, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NormalSquare, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueContact, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueWeighted, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueBoxed, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueRounded, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueOffCenter, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ObliqueSquare, NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panmidline[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StandardTrimmed, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StandardPointed, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_StandardSerifed, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HighTrimmed, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HighPointed, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HighSerifed, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantTrimmed, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantPointed, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantSerifed, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LowTrimmed, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LowPointed, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LowSerifed, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo panxheight[] = {
    { (unichar_t *) _STR_Any, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NoFit, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantSmall, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantStandard, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ConstantLarge, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DuckingSmall, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DuckingStandard, NULL, 0, 0, (void *) 6, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DuckingLarge, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo mslanguages[] = {
    { (unichar_t *) _STR_Afrikaans, NULL, 0, 0, (void *) 0x436, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Albanian, NULL, 0, 0, (void *) 0x41c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Arabic, NULL, 0, 0, (void *) 0x401, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_IraqArabic, NULL, 0, 0, (void *) 0x801, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_EgyptArabic, NULL, 0, 0, (void *) 0xc01, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LibyaArabic, NULL, 0, 0, (void *) 0x1001, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_AlgeriaArabic, NULL, 0, 0, (void *) 0x1401, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MoroccoArabic, NULL, 0, 0, (void *) 0x1801, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_TunisiaArabic, NULL, 0, 0, (void *) 0x1C01, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OmanArabic, NULL, 0, 0, (void *) 0x2001, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_YemenArabic, NULL, 0, 0, (void *) 0x2401, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SyriaArabic, NULL, 0, 0, (void *) 0x2801, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_JordanArabic, NULL, 0, 0, (void *) 0x2c01, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LebanonArabic, NULL, 0, 0, (void *) 0x3001, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_KuwaitArabic, NULL, 0, 0, (void *) 0x3401, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UAEArabic, NULL, 0, 0, (void *) 0x3801, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BahrainArabic, NULL, 0, 0, (void *) 0x3c01, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_QatarArabic, NULL, 0, 0, (void *) 0x4001, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Armenian, NULL, 0, 0, (void *) 0x42b, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Assamese, NULL, 0, 0, (void *) 0x44d, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LatinAzeri, NULL, 0, 0, (void *) 0x42c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CyrillicAzeri, NULL, 0, 0, (void *) 0x82c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Basque, NULL, 0, 0, (void *) 0x42d, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Byelorussian, NULL, 0, 0, (void *) 0x423, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bengali, NULL, 0, 0, (void *) 0x445, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Bulgarian, NULL, 0, 0, (void *) 0x402, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Catalan, NULL, 0, 0, (void *) 0x403, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MSChinese, NULL, 0, 0, (void *) 0x404, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PRCChinese, NULL, 0, 0, (void *) 0x804, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HKChinese, NULL, 0, 0, (void *) 0xc04, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SingChinese, NULL, 0, 0, (void *) 0x1004, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MacauChinese, NULL, 0, 0, (void *) 0x1404, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
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
    { (unichar_t *) _STR_SAEnglish, NULL, 0, 0, (void *) 0x1c09, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_JamEnglish, NULL, 0, 0, (void *) 0x2009, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CarEnglish, NULL, 0, 0, (void *) 0x2409, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BelEnglish, NULL, 0, 0, (void *) 0x2809, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_TrinEnglish, NULL, 0, 0, (void *) 0x2c09, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ZimEnglish, NULL, 0, 0, (void *) 0x3009, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PhilEnglish, NULL, 0, 0, (void *) 0x3409, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Estonia, NULL, 0, 0, (void *) 0x425, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Faeroese, NULL, 0, 0, (void *) 0x438, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Farsi, NULL, 0, 0, (void *) 0x429, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Finnish, NULL, 0, 0, (void *) 0x40b, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_FRFrench, NULL, 0, 0, (void *) 0x40c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BEFrench, NULL, 0, 0, (void *) 0x80c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CAFrench, NULL, 0, 0, (void *) 0xc0c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CHFrench, NULL, 0, 0, (void *) 0x100c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LUFrench, NULL, 0, 0, (void *) 0x140c, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Georgian, NULL, 0, 0, (void *) 0x437, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DEGerman, NULL, 0, 0, (void *) 0x407, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CHGerman, NULL, 0, 0, (void *) 0x807, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ATGerman, NULL, 0, 0, (void *) 0xc07, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LUGerman, NULL, 0, 0, (void *) 0x1007, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LIGerman, NULL, 0, 0, (void *) 0x1407, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Greek, NULL, 0, 0, (void *) 0x408, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Gujarati, NULL, 0, 0, (void *) 0x447, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Hebrew, NULL, 0, 0, (void *) 0x40d, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Hindi, NULL, 0, 0, (void *) 0x439, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Hungarian, NULL, 0, 0, (void *) 0x40e, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Icelandic, NULL, 0, 0, (void *) 0x40f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Indonesian, NULL, 0, 0, (void *) 0x421, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Italian, NULL, 0, 0, (void *) 0x410, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CHItalian, NULL, 0, 0, (void *) 0x810, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Japanese, NULL, 0, 0, (void *) 0x411, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Kannada, NULL, 0, 0, (void *) 0x44b, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Kashmiri, NULL, 0, 0, (void *) 0x860, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Kazakh, NULL, 0, 0, (void *) 0x43f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Konkani, NULL, 0, 0, (void *) 0x457, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LangKorean, NULL, 0, 0, (void *) 0x412, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LangKoreanJohab, NULL, 0, 0, (void *) 0x812, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Latvian, NULL, 0, 0, (void *) 0x426, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Lithuanian, NULL, 0, 0, (void *) 0x427, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ClassLithuanian, NULL, 0, 0, (void *) 0x827, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Macedonian, NULL, 0, 0, (void *) 0x42f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Malay, NULL, 0, 0, (void *) 0x43e, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BruMalay, NULL, 0, 0, (void *) 0x83e, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Nepali, NULL, 0, 0, (void *) 0x461, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_IndNepali, NULL, 0, 0, (void *) 0x861, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Norwegian, NULL, 0, 0, (void *) 0x414, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NorwegianN, NULL, 0, 0, (void *) 0x814, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Oriya, NULL, 0, 0, (void *) 0x448, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Polish, NULL, 0, 0, (void *) 0x415, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PTPortuguese, NULL, 0, 0, (void *) 0x416, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BRPortuguese, NULL, 0, 0, (void *) 0x816, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Punjabi, NULL, 0, 0, (void *) 0x446, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_RhaetoRomanic, NULL, 0, 0, (void *) 0x417, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Romanian, NULL, 0, 0, (void *) 0x418, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MolRomanian, NULL, 0, 0, (void *) 0x818, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Russian, NULL, 0, 0, (void *) 0x419, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MolRussian, NULL, 0, 0, (void *) 0x819, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Sami, NULL, 0, 0, (void *) 0x43b, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Sanskrit, NULL, 0, 0, (void *) 0x43b, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Serbian, NULL, 0, 0, (void *) 0xc1a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LatSerbian, NULL, 0, 0, (void *) 0x81a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Sindhi, NULL, 0, 0, (void *) 0x459, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Slovak, NULL, 0, 0, (void *) 0x41b, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Slovenian, NULL, 0, 0, (void *) 0x424, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Sorbian, NULL, 0, 0, (void *) 0x42e, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_TradSpanish, NULL, 0, 0, (void *) 0x40a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_MXSpanish, NULL, 0, 0, (void *) 0x80a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ModSpanish, NULL, 0, 0, (void *) 0xc0a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_GuaSpanish, NULL, 0, 0, (void *) 0x100a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CRSpanish, NULL, 0, 0, (void *) 0x140a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PanSpanish, NULL, 0, 0, (void *) 0x180a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DRSpanish, NULL, 0, 0, (void *) 0x1c0a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VenSpanish, NULL, 0, 0, (void *) 0x200a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ColSpanish, NULL, 0, 0, (void *) 0x240a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PeruSpanish, NULL, 0, 0, (void *) 0x280a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ArgSpanish, NULL, 0, 0, (void *) 0x2c0a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_EcuSpanish, NULL, 0, 0, (void *) 0x300a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ChiSpanish, NULL, 0, 0, (void *) 0x340a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UruSpanish, NULL, 0, 0, (void *) 0x380a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ParSpanish, NULL, 0, 0, (void *) 0x3c0a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_BolSpanish, NULL, 0, 0, (void *) 0x400a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_ESSpanish, NULL, 0, 0, (void *) 0x440a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_HonSpanish, NULL, 0, 0, (void *) 0x480a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_NicSpanish, NULL, 0, 0, (void *) 0x4c0a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_PRSpanish, NULL, 0, 0, (void *) 0x500a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Sutu, NULL, 0, 0, (void *) 0x430, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Swahili, NULL, 0, 0, (void *) 0x441, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Swedish, NULL, 0, 0, (void *) 0x41d, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_FinSwedish, NULL, 0, 0, (void *) 0x81d, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Tamil, NULL, 0, 0, (void *) 0x449, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Tatar, NULL, 0, 0, (void *) 0x444, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Telugu, NULL, 0, 0, (void *) 0x44a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LangThai, NULL, 0, 0, (void *) 0x41e, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Tsonga, NULL, 0, 0, (void *) 0x431, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Tswana, NULL, 0, 0, (void *) 0x432, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Turkish, NULL, 0, 0, (void *) 0x41f, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Ukrainian, NULL, 0, 0, (void *) 0x422, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Urdu, NULL, 0, 0, (void *) 0x420, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_IndUrdu, NULL, 0, 0, (void *) 0x820, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Uzbek, NULL, 0, 0, (void *) 0x443, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CyrUzbek, NULL, 0, 0, (void *) 0x843, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Venda, NULL, 0, 0, (void *) 0x433, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Vietnamese, NULL, 0, 0, (void *) 0x42a, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Xhosa, NULL, 0, 0, (void *) 0x434, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Yiddish, NULL, 0, 0, (void *) 0x43d, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Zulu, NULL, 0, 0, (void *) 0x435, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};
static GTextInfo ttfnameids[] = {
/* Put styles (docs call it subfamily) first because it is most likely to change */
    { (unichar_t *) _STR_Styles, NULL, 0, 0, (void *) 2, NULL, 0, 0, 0, 0, 1, 0, 0, 1},
    { (unichar_t *) _STR_Copyright, NULL, 0, 0, (void *) 0, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Family, NULL, 0, 0, (void *) 1, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Fullname, NULL, 0, 0, (void *) 4, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_UniqueID, NULL, 0, 0, (void *) 3, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Version, NULL, 0, 0, (void *) 5, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
/* Don't bother with PostscriptName, we set that elsewhere */
    { (unichar_t *) _STR_Trademark, NULL, 0, 0, (void *) 7, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Manufacturer, NULL, 0, 0, (void *) 8, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Designer, NULL, 0, 0, (void *) 9, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Descriptor, NULL, 0, 0, (void *) 10, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_VenderURL, NULL, 0, 0, (void *) 11, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_DesignerURL, NULL, 0, 0, (void *) 12, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_License, NULL, 0, 0, (void *) 13, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_LicenseURL, NULL, 0, 0, (void *) 14, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
/* slot 15 is reserved */
    { (unichar_t *) _STR_OTFFamily, NULL, 0, 0, (void *) 16, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_OTFStyles, NULL, 0, 0, (void *) 17, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_CompatableFull, NULL, 0, 0, (void *) 18, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_SampleText, NULL, 0, 0, (void *) 19, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

struct langstyle { int lang; const unichar_t *str; };
static const unichar_t regulareng[] = { 'R','e','g','u','l','a','r',  '\0' };
static const unichar_t demiboldeng[] = { 'D','e','m','i','B','o','l','d',  '\0' };
static const unichar_t demiboldeng3[] = { 'D','e','m','i',  '\0' };
static const unichar_t demiboldeng5[] = { 'S','e','m','i','B','o','l','d',  '\0' };
static const unichar_t boldeng[] = { 'B','o','l','d',  '\0' };
static const unichar_t thineng[] = { 'T','h','i','n',  '\0' };
static const unichar_t lighteng[] = { 'L','i','g','h','t',  '\0' };
static const unichar_t mediumeng[] = { 'M','e','d','i','u','m',  '\0' };
static const unichar_t bookeng[] = { 'B','o','o','k',  '\0' };
static const unichar_t heavyeng[] = { 'H','e','a','v','y',  '\0' };
static const unichar_t blackeng[] = { 'B','l','a','c','k',  '\0' };
static const unichar_t italiceng[] = { 'I','t','a','l','i','c',  '\0' };
static const unichar_t obliqueeng[] = { 'O','b','l','i','q','u','e',  '\0' };
static const unichar_t condensedeng[] = { 'C','o','n','d','e','n','s','e','d',  '\0' };
static const unichar_t expandedeng[] = { 'E','x','p','a','n','d','e','d',  '\0' };
static const unichar_t outlineeng[] = { 'O','u','t','l','i','n','e',  '\0' };

static const unichar_t regularfren[] = { 'N','o','r','m','a','l',  '\0' };
static const unichar_t boldfren[] = { 'G','r','a','s',  '\0' };
static const unichar_t demiboldfren[] = { 'D','e','m','i','G','r','a','s',  '\0' };
static const unichar_t demiboldfren2[] = { 'D','e','m','i',  '\0' };
static const unichar_t mediumfren[] = { 'N','o','r','m','a','l',  '\0' };
static const unichar_t lightfren[] = { 'M','a','i','g','r','e',  '\0' };
static const unichar_t blackfren[] = { 'E','x','t','r','a','G','r','a','s',  '\0' };
static const unichar_t italicfren[] = { 'I','t','a','l','i','q','u','e',  '\0' };
static const unichar_t obliquefren[] = { 'O','b','l','i','q','u','e',  '\0' };
static const unichar_t condensedfren[] = { 'E','t','r','o','i','t','e',  '\0' };
static const unichar_t expandedfren[] = { 'E','l','a','r','g','i',  '\0' };
static const unichar_t outlinefren[] = { 'C','o','n','t','o','u','r',  '\0' };

static const unichar_t regulargerm[] = { 'S','t','a','n','d','a','r','d',  '\0' };
static const unichar_t demiboldgerm[] = { 'H','a','l','b','f','e','t','t',  '\0' };
static const unichar_t demiboldgerm2[] = { 'S','c','h','m','a','l','l','f','e','t','t',  '\0' };
static const unichar_t boldgerm[] = { 'F','e','t','t',  '\0' };
static const unichar_t boldgerm2[] = { 'D','i','c','k',  '\0' };
static const unichar_t blackgerm[] = { 'S','c','h','w','a','r','z',  '\0' };
static const unichar_t lightgerm[] = { 'M','a','g','e','r',  '\0' };
static const unichar_t mediumgerm[] = { 'M','i','t','t','e','l',  '\0' };
static const unichar_t bookgerm[] = { 'B','u','c','h','s','c','h','r','i','f','t',  '\0' };
static const unichar_t italicgerm[] = { 'K','u','r','s','i','v',  '\0' };
static const unichar_t obliquegerm[] = { 'S','c','h','r',0xe4,'g',  '\0' };
static const unichar_t condensedgerm[] = { 'S','c','h','m','a','l',  '\0' };
static const unichar_t expandedgerm[] = { 'B','r','e','i','t',  '\0' };
static const unichar_t outlinegerm[] = { 'K','o','n','t','u','r','e','r','t',  '\0' };

static const unichar_t regularspan[] = { 'N','o','r','m','a','l',  '\0' };
static const unichar_t boldspan[] = { 'N','e','g','r','i','t','a',  '\0' };
static const unichar_t lightspan[] = { 'F','i','n','a',  '\0' };
static const unichar_t blackspan[] = { 'S','u','p','e','r','n','e','g','r','a',  '\0' };
static const unichar_t italicspan[] = { 'C','u','r','s','i','v','a',  '\0' };
static const unichar_t condensedspan[] = { 'C','o','n','d','e','n','s','a','d','a',  '\0' };
static const unichar_t expandedspan[] = { 'A','m','p','l','i','d','a',  '\0' };

static const unichar_t regulardutch[] = { 'R','e','g','e','l','m','a','t','i','g',  '\0' };
static const unichar_t bolddutch[] = { 'V','e','t',  '\0' };
static const unichar_t lightdutch[] = { 'L','i','c','h','t',  '\0' };
static const unichar_t blackdutch[] = { 'E','x','t','r','a',' ','v','e','t',  '\0' };
static const unichar_t italicdutch[] = { 'C','u','r','s','i','e','f',  '\0' };
static const unichar_t condenseddutch[] = { 'S','m','a','l',  '\0' };
static const unichar_t expandeddutch[] = { 'B','r','e','e','d',  '\0' };

static const unichar_t regularswed[] = { 'M','a','g','e','r',  '\0' };
static const unichar_t boldswed[] = { 'F','e','t',  '\0' };
static const unichar_t lightswed[] = { 'E','x','t','r','a','f','i','n',  '\0' };
static const unichar_t blackswed[] = { 'E','x','t','r','a','f','e','t',  '\0' };
static const unichar_t italicswed[] = { 'K','u','r','s','i','v',  '\0' };
static const unichar_t condensedswed[] = { 'S','m','a','l',  '\0' };
static const unichar_t expandedswed[] = { 'B','r','e','d',  '\0' };

static const unichar_t regularnor[] = { 'V','a','n','l','i','g',  '\0' };
static const unichar_t boldnor[] = { 'H','a','l','v','f','e','t',  '\0' };
static const unichar_t lightnor[] = { 'M','a','g','e','r',  '\0' };
static const unichar_t blacknor[] = { 'F','e','t',  '\0' };
static const unichar_t italicnor[] = { 'K','u','r','s','i','v',  '\0' };
static const unichar_t condensednor[] = { 'S','m','a','l',  '\0' };
static const unichar_t expandednor[] = { 'S','p','e','r','r','e','t',  '\0' };

static const unichar_t regularital[] = { 'N','o','r','m','a','l','e',  '\0' };
static const unichar_t demiboldital[] = { 'N','e','r','r','e','t','t','o',  '\0' };
static const unichar_t boldital[] = { 'N','e','r','o',  '\0' };
static const unichar_t thinital[] = { 'F','i','n','e',  '\0' };
static const unichar_t lightital[] = { 'C','h','i','a','r','o',  '\0' };
static const unichar_t mediumital[] = { 'M','e','d','i','o',  '\0' };
static const unichar_t bookital[] = { 'L','i','b','r','o',  '\0' };
static const unichar_t heavyital[] = { 'N','e','r','i','s','s','i','m','o',  '\0' };
static const unichar_t blackital[] = { 'E','x','t','r','a','N','e','r','o',  '\0' };
static const unichar_t italicital[] = { 'C','u','r','s','i','v','o',  '\0' };
static const unichar_t obliqueital[] = { 'O','b','l','i','q','u','o',  '\0' };
static const unichar_t condensedital[] = { 'C','o','n','d','e','n','s','a','t','o',  '\0' };
static const unichar_t expandedital[] = { 'A','l','l','a','r','g','a','t','o',  '\0' };

static const unichar_t regularru[] = { 0x41e, 0x431, 0x44b, 0x447, 0x43d, 0x44b, 0x439,  '\0' };
static const unichar_t demiboldru[] = { 0x41f, 0x43e, 0x43b, 0x443, 0x436, 0x438, 0x440, 0x43d, 0x44b, 0x439,  0 };
static const unichar_t boldru[] = { 0x41e, 0x431, 0x44b, 0x447, 0x43d, 0x44b, 0x439,  0 };
static const unichar_t heavyru[] = { 0x421, 0x432, 0x435, 0x440, 0x445, 0x436, 0x438, 0x440, 0x43d, 0x44b, 0x439,  0 };
static const unichar_t blackru[] = { 0x427, 0x451, 0x440, 0x43d, 0x44b, 0x439,  0 };
static const unichar_t thinru[] = { 0x422, 0x43e, 0x43d, 0x43a, 0x438, 0x439,  0 };
static const unichar_t lightru[] = { 0x421, 0x432, 0x435, 0x442, 0x43b, 0x44b, 0x439,  0 };
static const unichar_t italicru[] = { 0x41a, 0x443, 0x440, 0x441, 0x438, 0x432/*, 0x43d, 0x44b, 0x439*/,  0 };
static const unichar_t obliqueru[] = { 0x41d, 0x430, 0x43a, 0x43b, 0x43e, 0x43d/*, 0x43d, 0x44b, 0x439*/,  0 };
static const unichar_t condensedru[] = { 0x423, 0x437, 0x43a, 0x438, 0x439,  '\0' };
static const unichar_t expandedru[] = { 0x428, 0x438, 0x440, 0x43e, 0x43a, 0x438, 0x439,  '\0' };

static const unichar_t regularhu[] = { 'N','o','r','m',0xe1,'l',  '\0' };
static const unichar_t demiboldhu[] = { 'N','e','g','y','e','d','k',0xf6,'v',0xe9,'r',  '\0' };
static const unichar_t demiboldhu2[] = { 'F',0xe9,'l','k',0xf6,'v',0xe9,'r',  '\0' };
static const unichar_t boldhu[] = { 'F',0xe9,'l','k',0xf6,'v',0xe9,'r',  '\0' };
static const unichar_t boldhu2[] = { 'H',0xe1,'r','o','m','n','e','g','y','e','d','k',0xf6,'v',0xe9,'r',  '\0' };
static const unichar_t thinhu[] = { 'S','o','v',0xe1,'n','y',  '\0' };
static const unichar_t lighthu[] = { 'V','i','l',0xe1,'g','o','s',  '\0' };
static const unichar_t mediumhu[] = { 'K',0xf6,'z','e','p','e','s',  '\0' };
static const unichar_t bookhu[] = { 'H','a','l','v',0xe1,'n','y',  '\0' };
static const unichar_t bookhu2[] = { 'K',0xf6,'n','y','v',  '\0' };
static const unichar_t heavyhu[] = { 'K',0xf6,'v',0xe9,'r',  '\0' };
static const unichar_t heavyhu2[] = { 'E','x','t','r','a','k',0xf6,'v',0xe9,'r',  '\0' };
static const unichar_t blackhu[] = { 'F','e','k','e','t','e',  '\0' };
static const unichar_t blackhu2[] = { 'S',0xf6,'t',0xe9,'t',  '\0' };
static const unichar_t italichu[] = { 'D',0x151,'l','t',  '\0' };
static const unichar_t obliquehu[] = { 'D',0xf6,'n','t',0xf6,'t','t',  '\0' };
static const unichar_t obliquehu2[] = { 'F','e','r','d','e',  '\0' };
static const unichar_t condensedhu[] = { 'K','e','s','k','e','n','y',  '\0' };
static const unichar_t expandedhu[] = { 'S','z',0xe9,'l','e','s',  '\0' };
static const unichar_t outlinehu[] = { 'K','o','n','t',0xfa,'r','o','s',  '\0' };

static struct langstyle regs[] = { {0x409, regulareng}, { 0x40c, regularfren }, { 0x410, regularital }, { 0x407, regulargerm }, { 0x40a, regularspan }, { 0x419, regularru }, { 0x40e, regularhu },
	{ 0x413, regulardutch}, { 0x41d, regularswed }, { 0x414, regularnor }, { 0 }};
static struct langstyle meds[] = { {0x409, mediumeng}, { 0x410, mediumital }, { 0x40c, mediumfren }, { 0x407, mediumgerm }, { 0x40e, mediumhu }, { 0 }};
static struct langstyle books[] = { {0x409, bookeng}, { 0x410, bookital }, { 0x407, bookgerm }, { 0x40e, bookhu }, { 0x40e, bookhu2 }, { 0 }};
static struct langstyle bolds[] = { {0x409, boldeng}, { 0x410, boldital }, { 0x40c, boldfren }, { 0x407, boldgerm }, { 0x407, boldgerm2 }, { 0x40a, boldspan}, { 0x419, boldru }, { 0x40e, boldhu }, { 0x40e, boldhu2 }, 
	{ 0x413, bolddutch}, { 0x41d, boldswed }, { 0x414, boldnor }, { 0 }};
static struct langstyle italics[] = { {0x409, italiceng}, { 0x410, italicital }, { 0x40c, italicfren }, { 0x407, italicgerm }, { 0x40a, italicspan}, { 0x419, italicru }, { 0x40e, italichu },
	{ 0x413, italicdutch}, { 0x41d, italicswed }, { 0x414, italicnor },  { 0 }};
static struct langstyle obliques[] = { {0x409, obliqueeng}, { 0x410, obliqueital }, { 0x40c, obliquefren }, { 0x407, obliquegerm }, { 0x419, obliqueru }, { 0x40e, obliquehu }, { 0x40e, obliquehu2 }, { 0 }};
static struct langstyle demibolds[] = { {0x409, demiboldeng}, {0x409, demiboldeng3}, {0x409, demiboldeng5},
	{ 0x410, demiboldital }, { 0x40c, demiboldfren }, { 0x40c, demiboldfren2 }, { 0x407, demiboldgerm }, { 0x407, demiboldgerm2 },
	{ 0x419, demiboldru }, { 0x40e, demiboldhu }, { 0x40e, demiboldhu2 },{ 0 }};
static struct langstyle heavys[] = { {0x409, heavyeng}, { 0x410, heavyital }, { 0x419, heavyru }, { 0x40e, heavyhu }, { 0x40e, heavyhu2 }, { 0 }};
static struct langstyle blacks[] = { {0x409, blackeng}, { 0x410, blackital }, { 0x40c, blackfren }, { 0x407, blackgerm }, { 0x419, blackru }, { 0x40e, blackhu }, { 0x40e, blackhu2 }, { 0x40a, blackspan }, 
	{ 0x413, blackdutch}, { 0x41d, blackswed }, { 0x414, blacknor }, { 0 }};
static struct langstyle thins[] = { {0x409, thineng}, { 0x410, thinital }, { 0x419, thinru }, { 0x40e, thinhu }, { 0 }};
static struct langstyle lights[] = { {0x409, lighteng}, {0x410, lightital}, {0x40c, lightfren}, {0x407, lightgerm}, { 0x419, lightru }, { 0x40e, lighthu }, { 0x40a, lightspan }, 
	{ 0x413, lightdutch}, { 0x41d, lightswed }, { 0x414, lightnor }, { 0 }};
static struct langstyle condenseds[] = { {0x409, condensedeng}, {0x410, condensedital}, {0x40c, condensedfren}, {0x407, condensedgerm}, { 0x419, condensedru }, { 0x40e, condensedhu }, { 0x40a, condensedspan }, 
	{ 0x413, condenseddutch}, { 0x41d, condensedswed }, { 0x414, condensednor }, { 0 }};
static struct langstyle expandeds[] = { {0x409, expandedeng}, {0x410, expandedital}, {0x40c, expandedfren}, {0x407, expandedgerm}, { 0x419, expandedru }, { 0x40e, expandedhu }, { 0x40a, expandedspan }, 
	{ 0x413, expandeddutch}, { 0x41d, expandedswed }, { 0x414, expandednor }, { 0 }};
static struct langstyle outlines[] = { {0x409, outlineeng}, {0x40c, outlinefren}, {0x407, outlinegerm}, {0x40e, outlinehu}, { 0 }};
static struct langstyle *stylelist[] = {regs, meds, books, demibolds, bolds, heavys, blacks,
	lights, thins, italics, obliques, condenseds, expandeds, outlines, NULL };

#define CID_Features	101		/* Mac stuff */
#define CID_FeatureDel	103
#define CID_FeatureEdit	105

#define CID_Encoding	1001
#define CID_Family	1002
#define CID_Weight	1003
#define CID_ItalicAngle	1004
#define CID_UPos	1005
#define CID_UWidth	1006
#define CID_Ascent	1007
#define CID_Descent	1008
#define CID_NChars	1009
#define CID_Notice	1010
#define CID_Version	1011
#define CID_UniqueID	1012
#define CID_HasVerticalMetrics	1013
#define CID_VOriginLab	1014
#define CID_VOrigin	1015
#define CID_Fontname	1016
#define CID_Em		1017
#define CID_Scale	1018
#define CID_IsOrder2	1019
#define CID_IsMultiLayer	1020
#define CID_Interpretation	1021

#define CID_Make	1111
#define CID_Delete	1112
#define CID_XUID	1113
#define CID_Human	1114

#define CID_ForceEncoding	1215

#define CID_PrivateEntries	2001
#define	CID_PrivateValues	2002
#define	CID_Add			2003
#define CID_Guess		2004
#define CID_Remove		2005
#define CID_Hist		2006

#define CID_WeightClass		3001
#define CID_WidthClass		3002
#define CID_PFMFamily		3003
#define CID_FSType		3004
#define CID_NoSubsetting	3005
#define CID_OnlyBitmaps		3006
#define CID_LineGap		3007
#define CID_VLineGap		3008
#define CID_VLineGapLab		3009
#define CID_WinAscent		3010
#define CID_WinAscentLab	3011
#define CID_WinAscentIsOff	3012
#define CID_WinDescent		3013
#define CID_WinDescentLab	3014
#define CID_WinDescentIsOff	3015

#define CID_PanFamily		4001
#define CID_PanSerifs		4002
#define CID_PanWeight		4003
#define CID_PanProp		4004
#define CID_PanContrast		4005
#define CID_PanStrokeVar	4006
#define CID_PanArmStyle		4007
#define CID_PanLetterform	4008
#define CID_PanMidLine		4009
#define CID_PanXHeight		4010

#define CID_Language		5001
#define CID_StrID		5002
#define CID_String		5003
#define CID_TNDef		5004

#define CID_Comment		6001

#define CID_AnchorClasses	7001
#define CID_AnchorNew		7002
#define CID_AnchorDel		7003
#define CID_AnchorRename	7004
#define CID_ShowMark		7005
#define CID_ShowBase		7006

#define CID_TeXText		8001
#define CID_TeXMath		8002
#define CID_TeXMathExt		8003
#define CID_DesignSize		8004
#define CID_MoreParams		8005
#define CID_TeXExtraSpLabel	8006
#define CID_TeX			8007	/* through 8014 */

#define CID_ContextClasses	9001	/* Variants at +n*100 */
#define CID_ContextNew		9002
#define CID_ContextDel		9003
#define CID_ContextEdit		9004
#define CID_ContextEditData	9005

#define CID_SMList		9011	/* Variants at +n*100 */
#define CID_SMNew		9012
#define CID_SMDel		9013
#define CID_SMEdit		9014

#define CID_MacAutomatic	16000
#define CID_MacStyles		16001
#define CID_MacFOND		16002

struct psdict *PSDictCopy(struct psdict *dict) {
    struct psdict *ret;
    int i;

    if ( dict==NULL )
return( NULL );

    ret = gcalloc(1,sizeof(struct psdict));
    ret->cnt = dict->cnt; ret->next = dict->next;
    ret->keys = gcalloc(ret->cnt,sizeof(char *));
    ret->values = gcalloc(ret->cnt,sizeof(char *));
    for ( i=0; i<dict->next; ++i ) {
	ret->keys[i] = copy(dict->keys[i]);
	ret->values[i] = copy(dict->values[i]);
    }

return( ret );
}

int PSDictFindEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( -1 );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
return( i );

return( -1 );
}

char *PSDictHasEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( NULL );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
return( dict->values[i] );

return( NULL );
}

int PSDictRemoveEntry(struct psdict *dict, char *key) {
    int i;

    if ( dict==NULL )
return( false );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
    break;
    if ( i==dict->next )
return( false );
    free( dict->keys[i]);
    free( dict->values[i] );
    --dict->next;
    while ( i<dict->next ) {
	dict->keys[i] = dict->keys[i+1];
	dict->values[i] = dict->values[i+1];
	++i;
    }

return( true );
}

int PSDictChangeEntry(struct psdict *dict, char *key, char *newval) {
    int i;

    if ( dict==NULL )
return( -1 );

    for ( i=0; i<dict->next; ++i )
	if ( strcmp(dict->keys[i],key)==0 )
    break;
    if ( i==dict->next ) {
	if ( dict->next>=dict->cnt ) {
	    dict->cnt += 10;
	    dict->keys = grealloc(dict->keys,dict->cnt*sizeof(char *));
	    dict->values = grealloc(dict->values,dict->cnt*sizeof(char *));
	}
	dict->keys[dict->next] = copy(key);
	dict->values[dict->next] = NULL;
	++dict->next;
    }
    free(dict->values[i]);
    dict->values[i] = copy(newval);
return( i );
}

/* These are Postscript names, and as such should not be translated */
enum { pt_number, pt_boolean, pt_array, pt_code };
static struct { const char *name; short type, arr_size, present; } KnownPrivates[] = {
    { "BlueValues", pt_array, 14 },
    { "OtherBlues", pt_array, 10 },
    { "BlueFuzz", pt_number },
    { "FamilyBlues", pt_array, 14 },
    { "FamilyOtherBlues", pt_array, 10 },
    { "BlueScale", pt_number },
    { "BlueShift", pt_number },
    { "StdHW", pt_array, 1 },
    { "StdVW", pt_array, 1 },
    { "StemSnapH", pt_array, 12 },
    { "StemSnapV", pt_array, 12 },
    { "ForceBold", pt_boolean },
    { "LanguageGroup", pt_number },
    { "RndStemUp", pt_number },
    { "lenIV", pt_number },
    { "ExpansionFactor", pt_number },
    { "Erode", pt_code },
/* I am deliberately not including Subrs and OtherSubrs */
/* The first could not be entered (because it's a set of binary strings) */
/* And the second has special meaning to us and must be handled with care */
    { NULL }
};

struct ask_data {
    int ret;
    int done;
};

static int Ask_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    struct ask_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = true;
    }
return( true );
}

static int Ask_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    struct ask_data *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = d->ret = true;
    }
return( true );
}

static int ask_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct ask_data *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

static char *AskKey(SplineFont *sf) {
    int i,j, cnt=0;
    GTextInfo *ti;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8];
    GTextInfo label[8];
    struct ask_data d;
    char *ret;
    int ptwidth;

    if ( sf->private==NULL )
	for ( i=0; KnownPrivates[i].name!=NULL; ++i ) {
	    KnownPrivates[i].present = 0;
	    ++cnt;
	}
    else {
	for ( i=0; KnownPrivates[i].name!=NULL; ++i ) {
	    for ( j=0; j<sf->private->next; ++j )
		if ( strcmp(KnownPrivates[i].name,sf->private->keys[j])==0 )
	    break;
	    if ( !(KnownPrivates[i].present = (j<sf->private->next)) )
		++cnt;
	}
    }
    if ( cnt==0 )
	ti = NULL;
    else {
	ti = gcalloc(cnt+1,sizeof(GTextInfo));
	for ( i=cnt=0; KnownPrivates[i].name!=NULL; ++i )
	    if ( !KnownPrivates[i].present )
		ti[cnt++].text = uc_copy(KnownPrivates[i].name);
    }

    memset(&d,'\0',sizeof(d));

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_PrivateKey,NULL);
    pos.x = pos.y = 0;
    ptwidth = 2*GIntGetResource(_NUM_Buttonsize)+GGadgetScale(60);
    pos.width =GDrawPointsToPixels(NULL,ptwidth);
    pos.height = GDrawPointsToPixels(NULL,90);
    gw = GDrawCreateTopWindow(NULL,&pos,ask_e_h,&d,&wattrs);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _STR_KeyInPrivate;
    label[0].text_in_resource = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 6;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].creator = GLabelCreate;

    gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = 18; gcd[1].gd.pos.width = ptwidth-20;
    gcd[1].gd.flags = gg_visible | gg_enabled;
    gcd[1].creator = GTextFieldCreate;
    if ( ti!=NULL ) {
	gcd[1].gd.u.list = ti;
	gcd[1].creator = GListFieldCreate;
    }

    gcd[2].gd.pos.x = 20-3; gcd[2].gd.pos.y = 90-35-3;
    gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
    label[2].text = (unichar_t *) _STR_OK;
    label[2].text_in_resource = true;
    gcd[2].gd.mnemonic = 'O';
    gcd[2].gd.label = &label[2];
    gcd[2].gd.handle_controlevent = Ask_OK;
    gcd[2].creator = GButtonCreate;

    gcd[3].gd.pos.x = -20; gcd[3].gd.pos.y = 90-35;
    gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
    gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    label[3].text = (unichar_t *) _STR_Cancel;
    label[3].text_in_resource = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.mnemonic = 'C';
    gcd[3].gd.handle_controlevent = Ask_Cancel;
    gcd[3].creator = GButtonCreate;

    gcd[4].gd.pos.x = 2; gcd[4].gd.pos.y = 2;
    gcd[4].gd.pos.width = pos.width-4; gcd[4].gd.pos.height = pos.height-2;
    gcd[4].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[4].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    ret = NULL;
    if ( d.ret )
	ret = cu_copy(_GGadgetGetTitle(gcd[1].ret));
    GTextInfoListFree(ti);
    GDrawDestroyWindow(gw);
return( ret );
}

static GTextInfo *PI_ListSet(SplineFont *sf) {
    GTextInfo *ti = gcalloc((sf->private==NULL?0:sf->private->next)+1,sizeof( GTextInfo ));
    int i=0;

    if ( sf->private!=NULL ) {
	for ( i=0; i<sf->private->next; ++i ) {
	    ti[i].text = uc_copy(sf->private->keys[i]);
	}
    }
    if ( i!=0 )
	ti[0].selected = true;
return( ti );
}

static GTextInfo **PI_ListArray(struct psdict *private) {
    GTextInfo **ti = gcalloc((private==NULL?0:private->next)+1,sizeof( GTextInfo *));
    int i=0;

    if ( private!=NULL ) {
	for ( i=0; i<private->next; ++i ) {
	    ti[i] = gcalloc(1,sizeof(GTextInfo));
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	    ti[i]->text = uc_copy(private->keys[i]);
	}
    }
    ti[i] = gcalloc(1,sizeof(GTextInfo));
    if ( i!=0 )
	ti[0]->selected = true;
return( ti );
}

static void PIPrivateCheck(struct gfi_data *d) {
    if ( d->private==NULL ) {
	if ( d->sf->private==NULL ) {
	    d->private = gcalloc(1,sizeof(struct psdict));
	    d->private->cnt = 10;
	    d->private->keys = gcalloc(10,sizeof(char *));
	    d->private->values = gcalloc(10,sizeof(char *));
	} else
	    d->private = PSDictCopy(d->sf->private);
    }
}

static int PIFinishFormer(struct gfi_data *d) {
    static int buts[] = { _STR_OK, _STR_Cancel, 0 };
    unichar_t *end;

    if ( d->old_sel < 0 )
return( true );
    if ( d->private==NULL && d->sf->private!=NULL ) {
	const unichar_t *val = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_PrivateValues));
	if ( uc_strcmp(val,d->sf->private->values[d->old_sel])==0 )
return( true );			/* Didn't change */
	PIPrivateCheck(d);
    }
    if ( d->private!=NULL && d->old_sel>=0 && d->old_sel!=d->private->next ) {
	const unichar_t *val = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_PrivateValues));
	const unichar_t *pt = val;
	int i;

	/* does the type appear reasonable? */
	while ( isspace(*pt)) ++pt;
	for ( i=0; KnownPrivates[i].name!=NULL; ++i )
	    if ( strcmp(KnownPrivates[i].name,d->private->keys[d->old_sel])==0 )
	break;
	if ( KnownPrivates[i].name!=NULL ) {
	    if ( KnownPrivates[i].type==pt_array ) {
		if ( *pt!='[' && GWidgetAskR(_STR_Badtype,buts,0,1,_STR_Arrayquest)==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_boolean ) {
		if ( uc_strcmp(pt,"true")!=0 && uc_strcmp(pt,"false")!=0 &&
			GWidgetAskR(_STR_Badtype,buts,0,1,_STR_Boolquest)==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_code ) {
		if ( *pt!='{' && GWidgetAskR(_STR_Badtype,buts,0,1,_STR_Codequest)==1 )
return( false );
	    } else if ( KnownPrivates[i].type==pt_number ) {
		u_strtod(pt,&end);
		while ( isspace(*end)) ++end;
		if ( *end!='\0' && GWidgetAskR(_STR_Badtype,buts,0,1,_STR_Numberquest)==1 )
return( false );
	    }
	}

	/* Ok then set it */
	free(d->private->values[d->old_sel]);
	d->private->values[d->old_sel] = cu_copy(val);
	d->old_sel = -1;
    }
return( true );
}

static void ProcessListSel(struct gfi_data *d) {
    GGadget *list = GWidgetGetControl(d->gw,CID_PrivateEntries);
    int sel = GGadgetGetFirstListSelectedItem(list);
    unichar_t *temp;
    static const unichar_t nullstr[] = { 0 };
    SplineFont *sf = d->sf;
    struct psdict *private;

    if ( d->old_sel==sel )
return;

    if ( !PIFinishFormer(d)) {
	/*GGadgetSelectListItem(list,sel,false);*/
	GGadgetSelectListItem(list,d->old_sel,true);
return;
    }
    private = d->private ? d->private : sf->private;
    if ( sel==-1 ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Remove),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Hist),false);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PrivateValues),false);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),nullstr);
    } else {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Remove),true);
	if ( strcmp(private->keys[sel],"BlueValues")==0 ||
		strcmp(private->keys[sel],"OtherBlues")==0 ||
		strcmp(private->keys[sel],"StdHW")==0 ||
		strcmp(private->keys[sel],"StemSnapH")==0 ||
		strcmp(private->keys[sel],"StdVW")==0 ||
		strcmp(private->keys[sel],"StemSnapV")==0 ) {
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),true);
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Hist),true);
	} else {
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),false);
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Hist),false);
	}
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PrivateValues),true);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( private->values[sel]));
	free( temp );
	GTextFieldShow(GWidgetGetControl(d->gw,CID_PrivateValues),0);
    }
    d->old_sel = sel;
}

static int PI_Add(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int i;
    char *newkey;
    GTextInfo **ti;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	if ( !PIFinishFormer(d))
return(true);
	newkey = AskKey(d->sf);
	if ( newkey==NULL )
return( true );
	PIPrivateCheck(d);
	if (( i = PSDictFindEntry(d->private,newkey))==-1 )
	    i = PSDictChangeEntry(d->private,newkey,"");
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	ti = PI_ListArray(d->private);
	if ( i>0 ) {
	    ti[0]->selected = false;
	    ti[i]->selected = true;
	}
	GGadgetSetList(list,ti,false);
	d->old_sel = -1;
	ProcessListSel(d);
	free(newkey);
    }
return( true );
}

static void arraystring(char *buffer,real *array,int cnt) {
    int i, ei;

    for ( ei=cnt; ei>1 && array[ei-1]==0; --ei );
    *buffer++ = '[';
    for ( i=0; i<ei; ++i ) {
	sprintf(buffer, "%d ", (int) array[i]);
	buffer += strlen(buffer);
    }
    if ( buffer[-1] ==' ' ) --buffer;
    *buffer++ = ']'; *buffer='\0';
}

static void SnapSet(struct psdict *private,real stemsnap[12], real snapcnt[12],
	char *name1, char *name2 ) {
    int i, mi;
    char buffer[211];

    mi = -1;
    for ( i=0; stemsnap[i]!=0 && i<12; ++i )
	if ( mi==-1 ) mi = i;
	else if ( snapcnt[i]>snapcnt[mi] ) mi = i;
    if ( mi==-1 )
return;
    sprintf( buffer, "[%d]", (int) stemsnap[mi]);
    PSDictChangeEntry(private,name1,buffer);
    arraystring(buffer,stemsnap,12);
    PSDictChangeEntry(private,name2,buffer);
}

static int PI_Guess(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    real bluevalues[14], otherblues[10];
    real snapcnt[12];
    real stemsnap[12];
    char buffer[211];
    unichar_t *temp;
    static int buts[] = { _STR_OK, _STR_Cancel, 0 };
    struct psdict *private;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	sf = d->sf;
	private = d->private ? d->private : sf->private;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
	if ( strcmp(private->keys[sel],"BlueValues")==0 ||
		strcmp(private->keys[sel],"OtherBlues")==0 ) {
	    if ( GWidgetAskR(_STR_Guess,buts,0,1,_STR_Bluequest)==1 )
return( true );
	    PIPrivateCheck(d);
	    private = d->private;
	    FindBlues(sf,bluevalues,otherblues);
	    arraystring(buffer,bluevalues,14);
	    PSDictChangeEntry(private,"BlueValues",buffer);
	    arraystring(buffer,otherblues,10);
	    PSDictChangeEntry(sf->private,"OtherBlues",buffer);
	} else if ( strcmp(private->keys[sel],"StdHW")==0 ||
		strcmp(private->keys[sel],"StemSnapH")==0 ) {
	    if ( GWidgetAskR(_STR_Guess,buts,0,1,_STR_Hstemquest)==1 )
return( true );
	    FindHStems(sf,stemsnap,snapcnt);
	    PIPrivateCheck(d);
	    SnapSet(d->private,stemsnap,snapcnt,"StdHW","StemSnapH");
	} else if ( strcmp(private->keys[sel],"StdVW")==0 ||
		strcmp(private->keys[sel],"StemSnapV")==0 ) {
	    if ( GWidgetAskR(_STR_Guess,buts,0,1,_STR_Vstemquest)==1 )
return( true );
	    FindVStems(sf,stemsnap,snapcnt);
	    PIPrivateCheck(d);
	    SnapSet(d->private,stemsnap,snapcnt,"StdVW","StemSnapV");
	}
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( d->private->values[sel]));
	free( temp );
    }
return( true );
}

static int PI_Hist(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    struct psdict *private;
    enum hist_type h;
    unichar_t *temp;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	sf = d->sf;
	PIPrivateCheck(d);
	private = d->private ? d->private : sf->private;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
	if ( strcmp(private->keys[sel],"BlueValues")==0 ||
		strcmp(private->keys[sel],"OtherBlues")==0 )
	    h = hist_blues;
	else if ( strcmp(private->keys[sel],"StdHW")==0 ||
		strcmp(private->keys[sel],"StemSnapH")==0 )
	    h = hist_hstem;
	else if ( strcmp(private->keys[sel],"StdVW")==0 ||
		strcmp(private->keys[sel],"StemSnapV")==0 )
	    h = hist_vstem;
	else
return( true );		/* can't happen */
	SFHistogram(sf,private,NULL,h);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),
		temp = uc_copy( d->private->values[sel]));
	free( temp );
    }
return( true );
}

static int PI_Delete(GGadget *g, GEvent *e) {
    GWindow gw;
    struct gfi_data *d;
    GGadget *list;
    int sel;
    SplineFont *sf;
    GTextInfo **ti;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	PIPrivateCheck(d);
	sf = d->sf;
	list = GWidgetGetControl(d->gw,CID_PrivateEntries);
	sel = GGadgetGetFirstListSelectedItem(list);
	PSDictRemoveEntry(d->private, d->private->keys[sel]);
	sf->changed = true;
	ti = PI_ListArray(d->private);
	--sel;
	if ( d->private!=NULL && sel>=d->private->next )
	    sel = d->private->next-1;
	if ( sel>0 ) {
	    ti[0]->selected = false;
	    ti[sel]->selected = true;
	}
	GGadgetSetList(list,ti,false);
	d->old_sel = -2;
	ProcessListSel(d);
    }
return( true );
}

static int PI_ListSel(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	ProcessListSel(GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int infont(SplineChar *sc, const unsigned short *table, int tlen,
	Encoding *item, uint8 *used, int new_map) {
    int i;
    int tlen2 = tlen;
    /* for some reason, some encodings have multiple entries for the same */
    /*  glyph. One obvious one is space: 0x20 and 0xA0. The used bit array */
    /*  is designed to handle that unpleasantness */

    if ( table==NULL && item==NULL ) {
	if ( sc->unicodeenc==-1 ) {
	    if ( sc->enc!=0 || (used[0]&1) ) {
		sc->enc = -1;
return( false );
	    }
	    used[0] |= 1;
return( true );
	}
	if ( new_map>=em_unicodeplanes && new_map<=em_unicodeplanesmax ) {
	    i = sc->unicodeenc - ((new_map-em_unicodeplanes)<<16);
	    if ( i<0 || i>=tlen || (used[i>>3] & (1<<(i&7))) ) {
		sc->enc = -1;
return( false );
	    }
	    sc->enc = i;
	    used[i>>3] |= 1<<(i&7);
return( true );
	} else {
	    if ( sc->unicodeenc>=tlen ||
		    (used[sc->unicodeenc>>3] & (1<<(sc->unicodeenc&7))) ) {
		sc->enc = -1;
return( false );
	    }
	    used[sc->unicodeenc>>3] |= (1<<(sc->unicodeenc&7));
	    sc->enc = sc->unicodeenc;
return( true );
	}
    }
    if ( sc->unicodeenc==-1 ) {
	if ( SCIsNotdef(sc,-1) && !(used[0]&1) &&
		((table!=NULL && table[0]==0) || (item!=NULL && item->unicode[0]==0) ||
		 new_map==em_sjis || new_map==em_wansung ||
		 new_map==em_johab || new_map==em_jisgb ||
		 new_map==em_big5 || new_map==em_big5hkscs)) {
	    if ( (table!=NULL && table[0]==0) || (item!=NULL && item->unicode[0]==0) )
		used[0] |= 1;
return( true );			/* .notdef goes to encoding 0 */
	} else if ( item!=NULL && item->psnames!=NULL ) {
	    for ( i=0; i<tlen ; ++i ) {
		if ( item->psnames[i]!=NULL && strcmp(item->psnames[i],sc->name)==0 &&
			!(used[i>>3]&(1<<(i&7))) ) {
		    used[i>>3] |= (1<<(i&7));
		    sc->enc = i;
return( true );
		}
	    }
	} else {
	    sc->enc = -1;
return( false );
	}
    }

    if ( item!=NULL ) {
	const int32 *table32 = item->unicode;
	for ( i=0; i<tlen && (sc->unicodeenc!=table32[i] || (used[i>>3]&(1<<(i&7))) ||
		!(table32[i]!=0 || i==0 ||
		    (item->psnames!=NULL && item->psnames[i]!=NULL &&
		     strcmp(item->psnames[i],".notdef")==0))); ++i );
    } else {
	if ( table==unicode_from_jis208 ||
		table == unicode_from_ksc5601 ||
		table == unicode_from_gb2312 )
	    tlen = 94*94;

	for ( i=0; i<tlen && (sc->unicodeenc!=table[i] || (used[i>>3]&(1<<(i&7))) ||
		!(table[i]!=0 || i==0 )); ++i );
    }
    if ( i==tlen && sc->unicodeenc<0x80 && tlen2==65536 && table == unicode_from_jis208 ) {
	/* sjis often comes with a single byte encoding of ASCII */
	sc->enc = sc->unicodeenc;
return( true );
    } else if ( i==tlen && sc->unicodeenc>=0xFF61 && sc->unicodeenc<=0xFF9F &&
	    tlen2==65536 && table == unicode_from_jis208 ) {
	/* and katakana */
	for ( i=0xa1; i<=0xdf && sc->unicodeenc!=unicode_from_jis201[i] ; ++i );
		/* Was checking used array above, but it is set up for jis not sjis */
	if ( i>0xdf ) {
	    sc->enc = -1;
return( false );
	}
	sc->enc = i;
return( true );
    } else if ( i==tlen && sc->unicodeenc<160 &&
	    (tlen==0x10000-0xa100 || tlen==0x10000-0x8400 ||
	     (tlen2==65536 && table==unicode_from_gb2312 ) ||
	     (tlen2==65536 && table==unicode_from_ksc5601 ))) {
	/* Big 5 often comes with a single byte encoding of ASCII */
	/* As do Johab and Wansung */
	/* but sjis is more complex... */
	sc->enc = sc->unicodeenc;
return( true );
    }
    if ( i==tlen ) {
	sc->enc = -1;
return( false );
    } else {
	used[i>>3] |= (1<<(i&7));
	if ( tlen2==94*94 ) {
	    sc->enc = 0x2121 + ((i/94)<<8) + (i%94);
return( true );
	} else if ( table==unicode_from_ksc5601 ) {
	    /* Wansung */
	    sc->enc = 0xa1a1 + ((i/94)<<8) + (i%94);
return( true );
	} else if ( table==unicode_from_gb2312 ) {
	    /* Dunno what this variant of gb2312 is called: em_jisgb */
	    sc->enc = 0xa1a1 + ((i/94)<<8) + (i%94);
return( true );
	} else if ( table==unicode_from_jis208 ) {
	    /* sjis */
	    int ch1, ch2, ro, co;
	    ch1 = i/94 + 0x21; ch2 = i%94 + 0x21;
	    ro = ch1<95 ? 112 : 176;
	    co = (ch1&1) ? (ch2>95?32:31) : 126;
	    sc->enc = ((((ch1+1)>>1) + ro )<<8 )    |    (ch2+co);
return( true );
	} else if ( tlen==0x10000-0xa100 ) {
	    sc->enc = i+0xa100;
return( true );
	} else if ( tlen==0x10000-0x8400 ) {
	    sc->enc = i+0x8400;
return( true );
	} else {
	    sc->enc = i;
return( true );
	}
    }
}

static int intarget(SplineChar *sc, SplineFont *target) {
    sc->enc = SFFindChar(target,sc->unicodeenc,sc->name);
return( sc->enc!=-1 );
}

static void RemoveSplineChar(SplineFont *sf, int enc) {
    CharView *cv, *next;
    struct splinecharlist *dep, *dnext;
    BDFFont *bdf;
    BDFChar *bfc;
    SplineChar *sc = sf->chars[enc];
    BitmapView *bv, *bvnext;
    RefChar *refs, *rnext;
    FontView *fvs;
    KernPair *kp, *kprev;
    int i;

    if ( sc!=NULL ) {
	if ( sc->views ) {
	    for ( cv = sc->views; cv!=NULL; cv=next ) {
		next = cv->next;
		GDrawDestroyWindow(cv->gw);
	    }
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	}
	for ( dep=sc->dependents; dep!=NULL; dep=dnext ) {
	    SplineChar *dsc = dep->sc;
	    RefChar *rf, *rnext;
	    dnext = dep->next;
	    /* May be more than one reference to us, colon has two refs to period */
	    /*  but only one dlist entry */
	    for ( rf = dsc->layers[ly_fore].refs; rf!=NULL; rf=rnext ) {
		rnext = rf->next;
		if ( rf->sc==sc )
		    SCRefToSplines(dsc,rf);
	    }
	}
	for ( fvs=sc->parent->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	    if ( fvs->sv!=NULL ) {
		RefChar *rf, *rnext;
		for ( rf = fvs->sv->sc_srch.layers[ly_fore].refs; rf!=NULL; rf=rnext ) {
		    rnext = rf->next;
		    if ( rf->sc==sc )
			SCRefToSplines(&fvs->sv->sc_srch,rf);
		}
		for ( rf = fvs->sv->sc_rpl.layers[ly_fore].refs; rf!=NULL; rf=rnext ) {
		    rnext = rf->next;
		    if ( rf->sc==sc )
			SCRefToSplines(&fvs->sv->sc_rpl,rf);
		}
	    }
	    /* Are there any kerning pairs that look at this character? */
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
		for ( kprev=NULL, kp=sf->chars[i]->kerns; kp!=NULL; kprev = kp, kp=kp->next ) {
		    if ( kp->sc==sc ) {
			if ( kprev==NULL )
			    sf->chars[i]->kerns = kp->next;
			else
			    kprev->next = kp->next;
			kp->next = NULL;
			KernPairsFree(kp);
		break;
		    }
		}
	    }
	}
	sf->chars[enc] = NULL;

	for ( refs=sc->layers[ly_fore].refs; refs!=NULL; refs = rnext ) {
	    rnext = refs->next;
	    SCRemoveDependent(sc,refs);
	}
	SplineCharFree(sc);
    }

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	if ( (bfc = bdf->chars[enc])!= NULL ) {
	    bdf->chars[enc] = NULL;
	    if ( bfc->views!=NULL ) {
		for ( bv= bfc->views; bv!=NULL; bv=bvnext ) {
		    bvnext = bv->next;
		    GDrawDestroyWindow(bv->gw);
		}
		GDrawSync(NULL);
		GDrawProcessPendingEvents(NULL);
		GDrawSync(NULL);
		GDrawProcessPendingEvents(NULL);
	    }
	    BDFCharFree(bfc);
	}
    }
}

static int __SFReencodeFont(SplineFont *sf,enum charset new_map, SplineFont *target);

static int _SFForceEncoding(SplineFont *sf,enum charset new_map) {
    int enc_cnt=256,i;
    BDFFont *bdf;
    Encoding *item=NULL;

    if ( sf->encoding_name==new_map )
return(false);
    if ( new_map==em_custom || new_map==em_compacted ) {
	sf->encoding_name=em_custom;	/* Custom, it's whatever's there */
return(false);
    }

    if ( new_map==em_original ) {
	for ( i=enc_cnt=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=0 )
	    sf->chars[i]->orig_pos = enc_cnt++;
	if ( sf->charcnt==enc_cnt )
return( false );
return( __SFReencodeFont(sf,em_original,NULL));
    }
	

    if ( new_map>=em_base ) {
	for ( item=enclist; item!=NULL && item->enc_num!=new_map; item=item->next );
	if ( item!=NULL ) {
	    enc_cnt = item->char_cnt;
	} else {
	    GWidgetErrorR(_STR_InvalidEncoding,_STR_InvalidEncoding);
return( false );
	}
    } else if ( new_map==em_unicode || new_map==em_big5 || new_map==em_big5hkscs || new_map==em_johab )
	enc_cnt = 65536;
    else if ( new_map==em_unicode4 )
	enc_cnt = unicode4_size;
    else if ( new_map>=em_first2byte )
	enc_cnt = 65536;

    if ( sf->charcnt<enc_cnt ) {
	sf->chars = grealloc(sf->chars,enc_cnt*sizeof(SplineChar *));
	for ( i=sf->charcnt; i<enc_cnt; ++i )
	    sf->chars[i] = NULL;
	sf->charcnt = enc_cnt;
    }
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	if ( bdf->charcnt<enc_cnt ) {
	    bdf->chars = grealloc(bdf->chars,enc_cnt*sizeof(BDFChar *));
	    for ( i=bdf->charcnt; i<enc_cnt; ++i )
		bdf->chars[i] = NULL;
	}
	bdf->charcnt = enc_cnt;
    }
    sf->encoding_name = new_map;
    for ( i=0; i<sf->charcnt && i<enc_cnt; ++i ) if ( sf->chars[i]!=NULL ) {
	SplineChar dummy;
	SCBuildDummy(&dummy,sf,i);
	sf->chars[i]->unicodeenc = dummy.unicodeenc;
	free(sf->chars[i]->name);
	sf->chars[i]->name = copy(dummy.name);
    }
    free(sf->remap);
    sf->remap = NULL;
return( true );
}

int SFForceEncoding(SplineFont *sf,enum charset new_map) {
    if ( sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
	int i;
	for ( i=0; i<mm->instance_count; ++i )
	    _SFForceEncoding(mm->instances[i],new_map);
	_SFForceEncoding(mm->normal,new_map);
    } else
return( _SFForceEncoding(sf,new_map));

return( true );
}

void SFFindNearTop(SplineFont *sf) {
    FontView *fv;
    int i,k;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    if ( sf->subfontcnt==0 ) {
	for ( fv=sf->fv; fv!=NULL; fv=fv->nextsame ) {
	    fv->sc_near_top = NULL;
	    for ( i=fv->rowoff*fv->colcnt; i<sf->charcnt && i<(fv->rowoff+fv->rowcnt)*fv->colcnt; ++i )
		if ( sf->chars[i]!=NULL ) {
		    fv->sc_near_top = sf->chars[i];
	    break;
		}
	}
    } else {
	for ( fv=sf->fv; fv!=NULL; fv=fv->nextsame ) {
	    fv->sc_near_top = NULL;
	    for ( i=fv->rowoff*fv->colcnt; i<fv->sf->charcnt && i<(fv->rowoff+fv->rowcnt)*fv->colcnt; ++i ) {
		for ( k=0; k<sf->subfontcnt; ++k )
		    if ( i<sf->subfonts[k]->charcnt &&
			    sf->subfonts[k]->chars[i]!=NULL )
			fv->sc_near_top = sf->subfonts[k]->chars[i];
	    }
	}
    }
}

void SFRestoreNearTop(SplineFont *sf) {
    FontView *fv;

    for ( fv=sf->fv; fv!=NULL; fv=fv->nextsame ) if ( fv->sc_near_top!=NULL ) {
	/* Note: For CID keyed fonts, we don't care if sc is in the currenly */
	/*  displayed font, all we care about is the CID (->enc) */
	fv->rowoff = fv->sc_near_top->enc/fv->colcnt;
	GScrollBarSetPos(fv->vsb,fv->rowoff);
	/* Don't ask for an expose event yet. We'll get one soon enough */
    }
}

/* see also SplineFontNew in splineutil2.c */
static int __SFReencodeFont(SplineFont *sf,enum charset new_map,
	SplineFont *target) {
    const unsigned short *table=NULL;
    int i, extras, epos;
    SplineChar **chars;
    int enc_cnt;
    BDFFont *bdf;
    int tlen = 256;
    Encoding *item=NULL;
    uint8 *used;
    RefChar *refs;
    CharView *cv;

    if ( target==NULL ) {
	if ( sf->encoding_name==new_map )
return(false);
	if ( new_map==em_custom ) {
	    sf->encoding_name=em_custom;	/* Custom, it's whatever's there */
return(false);
	}
	if ( new_map==em_unicodeplanes )
	    new_map = em_unicode;		/* Plane 0 is just unicode bmp */
	if ( new_map==em_adobestandard ) {
	    table = unicode_from_adobestd;
	} else if ( new_map==em_iso8859_1 )
	    table = unicode_from_i8859_1;
	else if ( new_map==em_unicode ) {
	    table = NULL;
	    tlen = 65536;
	} else if ( new_map==em_unicode4 ) {
	    table = NULL;
	    tlen = unicode4_size;
	} else if ( new_map>=em_unicodeplanes && new_map<=em_unicodeplanesmax ) {
	    table = NULL;
	    tlen = 65536;
	} else if ( new_map>=em_base ) {
	    for ( item=enclist; item!=NULL && item->enc_num!=new_map; item=item->next );
	    if ( item!=NULL ) {
		tlen = item->char_cnt;
		table = NULL;
	    } else {
		GWidgetErrorR(_STR_InvalidEncoding,_STR_InvalidEncoding);
return( false );
	    }
	} else if ( new_map==em_jis208 ) {
	    table = unicode_from_jis208;
	    tlen = 94*94;
	} else if ( new_map==em_sjis ) {
	    table = unicode_from_jis208;
	    tlen = 65536;
	} else if ( new_map==em_jis212 ) {
	    table = unicode_from_jis212;
	    tlen = 94*94;
	} else if ( new_map==em_wansung ) {
	    table = unicode_from_ksc5601;
	    tlen = 65536;
	} else if ( new_map==em_ksc5601 ) {
	    table = unicode_from_ksc5601;
	    tlen = 94*94;
	} else if ( new_map==em_jisgb ) {
	    table = unicode_from_gb2312;
	    tlen = 65536;
	} else if ( new_map==em_gb2312 ) {
	    table = unicode_from_gb2312;
	    tlen = 94*94;
	} else if ( new_map==em_big5 ) {
	    table = unicode_from_big5;
	    tlen = 0x10000-0xa100;	/* the big5 table starts at 0xa100 to save space */
	} else if ( new_map==em_big5hkscs ) {
	    table = unicode_from_big5hkscs;
	    tlen = 0x10000-0x8100;	/* the big5 table starts at 0x8100 to save space */
	} else if ( new_map==em_johab ) {
	    table = unicode_from_johab;
	    tlen = 0x10000-0x8400;	/* the johab table starts at 0x8400 to save space */
	} else if ( new_map==em_original ) {
	    tlen = 0;
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->orig_pos!=0xffff )
		if ( sf->chars[i]->orig_pos>tlen ) tlen = sf->chars[i]->orig_pos;
	    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
/* This can happen in two ways: User added characters, or we have one char at */
/*  two encodings (space, nbspace) */
/* We could just remove duplicate encoding here, but I think better to leave */
/* that info in */
		if ( sf->chars[i]->orig_pos==0xffff )
		    sf->chars[i]->orig_pos = ++tlen;
	    ++tlen;
	    table = NULL;
	} else
	    table = unicode_from_alphabets[new_map+3];
    } else {
	if ( target->encoding_name == sf->encoding_name )
return( false );
	table = NULL;
	tlen = target->charcnt;
    }

    enc_cnt=tlen;
    if ( target!=NULL )
	/* Done */;
    else if ( table==NULL )
	/* Done */;
    else if ( tlen == 94*94 )
	enc_cnt = 65536;
    else if ( tlen == 0x10000-0xa100 || tlen==0x10000-0x8400 || tlen==0x10000-0x8100 )
	enc_cnt = 65536;

    extras = 0;
    if ( target==NULL && new_map==em_original ) {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	    sf->chars[i]->enc = sf->chars[i]->orig_pos;
    } else {
	used = gcalloc((tlen+7)/8,sizeof(uint8));
	for ( i=0; i<sf->charcnt; ++i ) {
	    SplineChar *sc = sf->chars[i];
	    if ( sc==NULL )
		/* skip */;
	    else if ( (target==NULL && !infont(sc,table,tlen,item,used,new_map)) ||
		      (target!=NULL && !intarget(sc,target) ) ) {
		if ( sc->layers[ly_fore].splines==NULL && sc->layers[ly_fore].refs==NULL && sc->dependents==NULL &&
			(!sc->widthset || strcmp(sc->name,".notdef")==0 ) ) {
			/* glyphs mapped to .notdef in a type1 font will have widthset but its not meaningful */
		    RemoveSplineChar(sf,i);
		} else
		    ++extras;
	    }
	    if ( sc!=NULL ) {
		for ( cv=sc->views; cv!=NULL; cv=cv->next )
		    cv->template1 = cv->template2 = NULL;
	    }
	}
	free(used);
    }
    chars = gcalloc(enc_cnt+extras,sizeof(SplineChar *));
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next )
	bdf->temp = gcalloc(enc_cnt+extras,sizeof(BDFChar *));
    for ( i=0, epos=enc_cnt; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]==NULL )
	    /* skip */;
	else {
	    if ( sf->chars[i]->enc==-1 )
		sf->chars[i]->enc = epos++;
	    if ( chars[sf->chars[i]->enc]!=NULL )
		fprintf( stderr, "Warning: Two chars (%s,%s) mapped to the same encoding %d\n",
			chars[sf->chars[i]->enc]->name, sf->chars[i]->name,
			sf->chars[i]->enc );
	    chars[sf->chars[i]->enc] = sf->chars[i];
	    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
		if ( i<bdf->charcnt && bdf->chars[i]!=NULL && sf->chars[i]!=NULL ) {
		    bdf->chars[i]->enc = sf->chars[i]->enc;
		    bdf->temp[sf->chars[i]->enc] = bdf->chars[i];
		}
	    }
	}
    }
    if ( epos!=enc_cnt+extras ) GDrawIError( "Bad count in ReencodeFont");
    free(sf->chars);
    sf->chars = chars;
    sf->charcnt = enc_cnt+extras;
    sf->encoding_name = new_map;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	for ( refs=sf->chars[i]->layers[ly_fore].refs; refs!=NULL; refs = refs->next )
	    refs->local_enc = refs->sc->enc;
    }

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	free(bdf->chars);
	bdf->chars = bdf->temp;
	bdf->temp = NULL;
	bdf->charcnt = enc_cnt+extras;
	bdf->encoding_name = new_map;
    }
    free(sf->remap);
    sf->remap = NULL;
    sf->encodingchanged = true;
    sf->compacted = false;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	SCRefreshTitles(sf->chars[i]);
return( true );
}

static int _SFReencodeFont(SplineFont *sf,enum charset new_map, SplineFont *target) {
    MMSet *mm = sf->mm;
    int i, ret;

    SFFindNearTop(sf);

    if ( mm==NULL )
	ret = __SFReencodeFont(sf,new_map,target);
    else {
	ret = false;
	for ( i=0; i<mm->instance_count; ++i )
	    ret |= __SFReencodeFont(mm->instances[i],new_map,target);
	ret |= __SFReencodeFont(mm->normal,new_map,target);
    }

    SFRestoreNearTop(sf);
return( ret );
}

int SFReencodeFont(SplineFont *sf,enum charset new_map) {
    if ( new_map==em_compacted )
return( SFCompactFont(sf));
    else if ( sf->compacted && new_map==sf->old_encname )
return( SFUncompactFont(sf));
    else
return( _SFReencodeFont(sf,new_map,NULL));
}

int SFMatchEncoding(SplineFont *sf,SplineFont *target) {
return( _SFReencodeFont(sf,em_none,target));
}

static void _SFAddDelChars(SplineFont *sf, int nchars) {
    int i;
    BDFFont *bdf;
    MetricsView *mv, *mnext;

    if ( nchars==sf->charcnt )
return;
    if ( nchars>sf->charcnt ) {
	sf->chars = grealloc(sf->chars,nchars*sizeof(SplineChar *));
	for ( i=sf->charcnt; i<nchars; ++i )
	    sf->chars[i] = NULL;
	sf->charcnt = nchars;
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    bdf->chars = grealloc(bdf->chars,nchars*sizeof(BDFChar *));
	    for ( i=bdf->charcnt; i<nchars; ++i )
		bdf->chars[i] = NULL;
	    bdf->charcnt = nchars;
	}
    } else {
	if ( sf->fv->metrics!=NULL ) {
	    for ( mv=sf->fv->metrics; mv!=NULL; mv = mnext ) {
		mnext = mv->next;
		GDrawDestroyWindow(mv->gw);
	    }
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	    GDrawSync(NULL);
	    GDrawProcessPendingEvents(NULL);
	}
	for ( i=nchars; i<sf->charcnt; ++i ) {
	    RemoveSplineChar(sf,i);
	}
	sf->charcnt = nchars;
	if ( nchars<256 ) sf->encoding_name = em_custom;
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    bdf->charcnt = nchars;
	    bdf->encoding_name = sf->encoding_name;
	}
    }
    GlyphHashFree(sf);
}

int SFAddDelChars(SplineFont *sf, int nchars) {
    int i;
    MMSet *mm = sf->mm;

    if ( nchars==sf->charcnt )
return( false );

    if ( mm==NULL )
	_SFAddDelChars(sf,nchars);
    else {
	for ( i=0; i<mm->instance_count; ++i )
	    _SFAddDelChars(mm->instances[i],nchars);
	_SFAddDelChars(mm->normal,nchars);
    }
return( true );
}

static void RegenerateEncList(struct gfi_data *d) {
    Encoding *item;
    GTextInfo **ti;
    int i, any;
    unichar_t *title=NULL;

    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ++i;
    any = i!=0;
    i += sizeof(encodingtypes)/sizeof(encodingtypes[0]);
    ti = gcalloc(i+1,sizeof(GTextInfo *));
    for ( i=0; i<sizeof(encodingtypes)/sizeof(encodingtypes[0])-1; ++i ) {
	ti[i] = galloc(sizeof(GTextInfo));
	memcpy(ti[i],&encodingtypes[i],sizeof(GTextInfo));
	if ( encodingtypes[i].text_is_1byte ) {
	    ti[i]->text = uc_copy((char *) ti[i]->text);
	    ti[i]->text_is_1byte = false;
	} else if ( encodingtypes[i].text_in_resource ) {
	    ti[i]->text = u_copy(GStringGetResource((int) ti[i]->text,NULL));
	    ti[i]->text_in_resource = false;
	} else {
	    ti[i]->text = u_copy(ti[i]->text);
	}
	ti[i]->bg = ti[i]->fg = COLOR_DEFAULT;
	ti[i]->selected = ( (int) (ti[i]->userdata)==d->sf->encoding_name) &&
		!ti[i]->line;
	if ( ti[i]->selected )
	    title = ti[i]->text;
    }
    if ( any ) {
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->bg = ti[i]->fg = COLOR_DEFAULT;
	ti[i++]->line = true;
	for ( item=enclist; item!=NULL ; item=item->next )
	    if ( !item->builtin ) {
		ti[i] = gcalloc(1,sizeof(GTextInfo));
		ti[i]->bg = ti[i]->fg = COLOR_DEFAULT;
		ti[i]->text = uc_copy(item->enc_name);
		ti[i]->userdata = (void *) item->enc_num;
		ti[i]->selected = ( item->enc_num==d->sf->encoding_name);
		if ( ti[i++]->selected )
		    title = ti[i-1]->text;
	    }
    }
    ti[i] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(GWidgetGetControl(d->gw,CID_Encoding),ti,false);
    if ( title!=NULL )
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Encoding),title);

    for ( item=enclist; item!=NULL && item->builtin; item=item->next );
    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Delete),item!=NULL);
}

static int GFI_Delete(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	RemoveEncoding();
	RegenerateEncList(d);
    }
return( true );
}

static int GFI_SelectEncoding(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int enc = GGadgetGetFirstListSelectedItem(GWidgetGetControl(d->gw,CID_Encoding));
	if ( encodingtypes[enc].userdata==(void *) em_unicodeplanes ) {
	    unichar_t ubuf[10], *ret;
	    char buf[10];
	    int val;
	    sprintf(buf,"%d", d->uplane );
	    uc_strcpy(ubuf,buf);
	    ret = GWidgetAskStringR(_STR_Encoding,ubuf,_STR_WhichPlane);
	    if ( ret!=NULL && (val=u_strtol(ret,NULL,0))>=0 && val<65536 )
		d->uplane = lastplane = val;
	}
    }
return( true );
}

static int GFI_Load(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	LoadEncodingFile();
	RegenerateEncList(d);
    }
return( true );
}

static int GFI_Make(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	Encoding *item = MakeEncoding(d->sf);
	if ( item!=NULL ) {
	    d->sf->encoding_name = item->enc_num;
	    RegenerateEncList(d);
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Make),false);
	}
    }
return( true );
}

/* Use URW 4 letter abrieviations */
static char *knownweights[] = { "Demi", "Bold", "Regu", "Medi", "Book", "Thin",
	"Ligh", "Heav", "Blac", "Ultr", "Nord", "Norm", "Gras", "Stan", "Halb",
	"Fett", "Mage", "Mitt", "Buch", NULL };
static char *realweights[] = { "Demi", "Bold", "Regular", "Medium", "Book", "Thin",
	"Light", "Heavy", "Black", "Ultra", "Nord", "Normal", "Gras", "Standard", "Halbfett",
	"Fett", "Mager", "Mittel", "Buchschrift", NULL};

static int GFI_NameChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GWindow gw = GGadgetGetWindow(g);
	struct gfi_data *gfi = GDrawGetUserData(gw);
	const unichar_t *uname = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Fontname));
	unichar_t ubuf[50];
	int i;
	for ( i=0; knownweights[i]!=NULL; ++i ) {
	    if ( uc_strstrmatch(uname,knownweights[i])!=NULL ) {
		uc_strcpy(ubuf, realweights[i]);
		GGadgetSetTitle(GWidgetGetControl(gw,CID_Weight),ubuf);
	break;
	    }
	}
	if ( gfi->human_untitled )
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_Human),uname);
	if ( gfi->family_untitled ) {
	    const unichar_t *ept = uname+u_strlen(uname); unichar_t *temp;
	    for ( i=0; knownweights[i]!=NULL; ++i ) {
		if (( temp = uc_strstrmatch(uname,knownweights[i]))!=NULL && temp<ept && temp!=uname )
		    ept = temp;
	    }
	    if (( temp = uc_strstrmatch(uname,"ital"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = uc_strstrmatch(uname,"obli"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = uc_strstrmatch(uname,"kurs"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = uc_strstrmatch(uname,"slanted"))!=NULL && temp<ept && temp!=uname )
		ept = temp;
	    if (( temp = u_strchr(uname,'-'))!=NULL && temp!=uname )
		ept = temp;
	    temp = u_copyn(uname,ept-uname);
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_Family),temp);
	}
    }
return( true );
}

static int GFI_FamilyChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	gfi->family_untitled = false;
    }
return( true );
}

static int GFI_HumanChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	struct gfi_data *gfi = GDrawGetUserData(GGadgetGetWindow(g));
	gfi->human_untitled = false;
    }
return( true );
}

static int GFI_VMetricsCheck(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	GWindow gw = GGadgetGetWindow(g);
	const unichar_t *vo = _GGadgetGetTitle(GWidgetGetControl(gw,CID_VOrigin));
	int checked = GGadgetIsChecked(g);
	if ( checked && *vo=='\0' ) {
	    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	    char space[10]; unichar_t uspace[10];
	    sprintf( space, "%d", d->sf->ascent );
	    uc_strcpy(uspace,space);
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_VOrigin),uspace);
	}
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_VOrigin),checked);
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_VOriginLab),checked);
	GGadgetSetEnabled(GWidgetGetControl(GDrawGetParentWindow(gw),CID_VLineGap),checked);
	GGadgetSetEnabled(GWidgetGetControl(GDrawGetParentWindow(gw),CID_VLineGapLab),checked);
    }
return( true );
}

static int GFI_EmChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	char buf[20]; unichar_t ubuf[20];
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(g); unichar_t *end;
	int val = u_strtol(ret,&end,10), ascent, descent;
	if ( *end )
return( true );
	switch ( GGadgetGetCid(g)) {
	  case CID_Em:
	    ascent = rint( ((double) val)*d->sf->ascent/(d->sf->ascent+d->sf->descent) );
	    descent = val - ascent;
	  break;
	  case CID_Ascent:
	    ascent = val;
	    ret = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	    descent = u_strtol(ret,&end,10);
	    if ( *end )
return( true );
	  break;
	  case CID_Descent:
	    descent = val;
	    ret = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
	    ascent = u_strtol(ret,&end,10);
	    if ( *end )
return( true );
	  break;
	}
	sprintf( buf, "%d", ascent ); if ( ascent==0 ) buf[0]='\0'; uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Ascent),ubuf);
	sprintf( buf, "%d", descent ); if ( descent==0 ) buf[0]='\0'; uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Descent),ubuf);
	sprintf( buf, "%d", ascent+descent ); if ( ascent+descent==0 ) buf[0]='\0'; uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_Em),ubuf);
    }
return( true );
}

static int GFI_GuessItalic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	real val = SFGuessItalicAngle(d->sf);
	char buf[30]; unichar_t ubuf[30];
	sprintf( buf, "%.1f", val);
	uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_ItalicAngle),ubuf);
    }
return( true );
}

static void GFI_CleanupContext(struct gfi_data *d) {
    FPST *fpst;
    ASM *sm;
    int i, j;

    /* Free any context which were created but got [Cancelled] */
    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_ContextClasses+i*100);
	int len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    fpst = (FPST *) (old[j]->userdata);
	    if ( fpst!=NULL ) fpst->ticked = false;
	}
    }
    for ( fpst = d->sf->possub; fpst!=NULL; fpst=fpst->next )
	fpst->ticked = true;
    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_ContextClasses+i*100);
	int len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    fpst = (FPST *) (old[j]->userdata);
	    if ( fpst!=NULL && !fpst->ticked ) {
		fpst->next = NULL;
		FPSTFree(fpst);
	    }
	}
    }

    /* Do the same thing for apple state machines */
    for ( i=0; i<3; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+i*100);
	int len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    sm = (ASM *) (old[j]->userdata);
	    if ( sm!=NULL ) sm->ticked = false;
	}
    }
    for ( sm = d->sf->sm; sm!=NULL; sm=sm->next )
	sm->ticked = true;
    for ( i=0; i<3; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+i*100);
	int len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    sm = (ASM *) (old[j]->userdata);
	    if ( sm!=NULL && !sm->ticked ) {
		sm->next = NULL;
		ASMFree(sm);
	    }
	}
    }
}

static void GFI_ProcessContexts(struct gfi_data *d) {
    FPST *fpst, *next, *p, *last;
    ASM *sm, *nextsm, *psm, *lastsm;
    int i, j;

    /* Free any contexts which have been deleted */
    for ( fpst = d->sf->possub; fpst!=NULL; fpst=fpst->next )
	fpst->ticked = false;
    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_ContextClasses+i*100);
	int len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    fpst = (FPST *) (old[j]->userdata);
	    if ( fpst!=NULL ) fpst->ticked = true;
	}
    }
    p = NULL;
    for ( fpst = d->sf->possub; fpst!=NULL; fpst=next ) {
	next = fpst->next;
	if ( fpst->ticked )
	    p = fpst;
	else {
	    if ( p==NULL )
		d->sf->possub = next;
	    else
		p->next = next;
	    fpst->next = NULL;
	    FPSTFree(fpst);
	}
    }

    /* Now build up a new list containing all fpst's */
    last = NULL;
    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_ContextClasses+i*100);
	int len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    fpst = (FPST *) (old[j]->userdata);
	    if ( fpst!=NULL ) {
		if ( last==NULL )
		    d->sf->possub = fpst;
		else
		    last->next = fpst;
		last = fpst;
	    }
	    old[j]->userdata = NULL;
	}
    }
    if ( last==NULL )
	d->sf->possub = NULL;
    else
	last->next = NULL;

    /* And for apple state machines... */
    /* Free any state machines which have been deleted */
    for ( sm = d->sf->sm; sm!=NULL; sm=sm->next )
	sm->ticked = false;
    for ( i=0; i<3; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+i*100);
	int len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    sm = (ASM *) (old[j]->userdata);
	    if ( sm!=NULL ) sm->ticked = true;
	}
    }
    psm = NULL;
    for ( sm = d->sf->sm; sm!=NULL; sm=nextsm ) {
	nextsm = sm->next;
	if ( sm->ticked )
	    psm = sm;
	else {
	    if ( p==NULL )
		d->sf->sm = nextsm;
	    else
		psm->next = nextsm;
	    sm->next = NULL;
	    ASMFree(sm);
	}
    }

    /* Now build up a new list containing all active state machines */
    lastsm = NULL;
    for ( i=0; i<4; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+i*100);
	int len;
	GTextInfo **old = GGadgetGetList(list,&len);
	for ( j=0; j<len; ++j ) {
	    sm = (ASM *) (old[j]->userdata);
	    if ( sm!=NULL ) {
		if ( lastsm==NULL )
		    d->sf->sm = sm;
		else
		    lastsm->next = sm;
		lastsm = sm;
	    }
	    old[j]->userdata = NULL;
	}
    }
    if ( lastsm==NULL )
	d->sf->sm = NULL;
    else
	lastsm->next = NULL;
}

static void GFI_Close(struct gfi_data *d) {
    FontView *fvs;
    SplineFont *sf = d->sf;
    int i;

    if ( d->ccd )
	CCD_Close(d->ccd);
    if ( d->smd )
	SMD_Close(d->smd);
    GFI_CleanupContext(d);
    PSDictFree(d->private);
    for ( i=0; i<ttf_namemax; ++i )
	free(d->def.names[i]);
    TTFLangNamesFree(d->names);

    if ( d->oldcnt!=sf->charcnt && sf->fv!=NULL && sf->fv->sf==sf ) {
	FontView *fvs;
	for ( fvs=sf->fv; fvs!=NULL; fvs = fvs->nextsame ) {
	    free(fvs->selected);
	    fvs->selected = gcalloc(sf->charcnt,sizeof(char));
	}
    }

    GDrawDestroyWindow(d->gw);
    for ( fvs = d->sf->fv; fvs!=NULL; fvs = fvs->nextsame ) {
	GDrawRequestExpose(sf->fv->v,NULL,false);
	if ( fvs->fontinfo == d )
	    fvs->fontinfo = NULL;
    }
    d->done = true;
    /* d will be freed by destroy event */;
}

static void GFI_CancelClose(struct gfi_data *d) {
    MacFeatListFree(GGadgetGetUserData((GWidgetGetControl(
	    d->gw,CID_Features))));
    GFI_Close(d);
}

GTextInfo *AnchorClassesList(SplineFont *sf) {
    AnchorClass *an;
    int cnt;
    GTextInfo *ti;

    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next );
    ti = gcalloc(cnt+1,sizeof(GTextInfo));
    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next ) {
	ti[cnt].text = ClassName(an->name,an->feature_tag,an->flags,
		an->script_lang_index, an->merge_with, an->type,false);
	ti[cnt].fg = ti[cnt].bg = COLOR_DEFAULT;
	ti[cnt].userdata = an;
    }
return( ti );
}

GTextInfo **AnchorClassesLList(SplineFont *sf) {
    AnchorClass *an;
    int cnt;
    GTextInfo **ti;

    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next );
    ti = gcalloc(cnt+1,sizeof(GTextInfo*));
    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next ) {
	ti[cnt] = gcalloc(1,sizeof(GTextInfo));
	ti[cnt]->text = ClassName(an->name,an->feature_tag,an->flags,
		an->script_lang_index,an->merge_with, an->type,false);
	ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	ti[cnt]->userdata = an;
    }
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

GTextInfo **AnchorClassesSimpleLList(SplineFont *sf) {
    AnchorClass *an;
    int cnt;
    GTextInfo **ti;

    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next );
    ti = gcalloc(cnt+1,sizeof(GTextInfo*));
    for ( cnt=0, an=sf->anchor; an!=NULL; ++cnt, an=an->next ) {
	ti[cnt] = gcalloc(1,sizeof(GTextInfo));
	ti[cnt]->text = u_copy(an->name);
	ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	ti[cnt]->userdata = an;
    }
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

static void GFI_AnchorShow(GGadget *g, int index) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    int i, start;
    GGadget *list = GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorClasses);
    int sel = GGadgetGetFirstListSelectedItem(list), len;
    GTextInfo **old = GGadgetGetList(list,&len);
    AnchorClass *ac;
    SplineChar *sc;
    AnchorPoint *ap, *ap2;
    CharView *cvs;

    if ( (ac = old[sel]->userdata)==NULL )
return;

    if ( d->anchor_shows[index].restart || d->anchor_shows[index].sc==NULL )
	start = 0;
    else
	start = d->anchor_shows[index].sc->enc+1;
    for ( i=start; i<d->sf->charcnt; ++i ) if ( d->sf->chars[i]!=NULL ) {
	sc = d->sf->chars[i];
	for ( ap = sc->anchor; ap!=NULL ; ap=ap->next ) {
	    if ( ap->anchor==ac &&
		    ((index==0 && (ap->type==at_mark || ap->type==at_centry)) ||
		     (index==1 && (ap->type==at_basechar || ap->type==at_baselig ||
				     ap->type==at_basemark || ap->type==at_cexit))))
	break;
	}
	if ( ap!=NULL )
    break;
    }
    if ( i==d->sf->charcnt ) {
	if ( start==0 ) {
	    GGadgetSetEnabled(g,false);
	    GWidgetErrorR(_STR_NoMore,index==0?_STR_NoMarks:_STR_NoBases);
	} else {
	    GGadgetSetTitle(g,GStringGetResource(index==0?_STR_ShowFirstMark:_STR_ShowFirstBase,NULL));
	    GWidgetErrorR(_STR_NoMore,index==0?_STR_NoMoreMarks:_STR_NoMoreBases);
	}
    } else {
	cvs = NULL;
	if ( d->anchor_shows[index].sc!=NULL ) {
	    for ( cvs = d->anchor_shows[index].sc->views;
		    cvs!=NULL && cvs!=d->anchor_shows[index].cv; cvs=cvs->next );
	}
	for ( ap2 = sc->anchor; ap2!=NULL ; ap2=ap2->next )
	    ap2->selected = false;
	ap->selected = true;
	d->anchor_shows[index].sc = sc;
	d->anchor_shows[index].restart = false;
	if ( cvs!=NULL )
	    CVChangeSC(cvs,sc);
	else
	    d->anchor_shows[index].cv = CharViewCreate(sc,sc->parent->fv);
	if ( start==0 )
	    GGadgetSetTitle(g,GStringGetResource(index==0?_STR_ShowNextMark:_STR_ShowNextBase,NULL));
    }
}

static int GFI_AnchorShowMark(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	GFI_AnchorShow(g,0);
return( true );
}

static int GFI_AnchorShowBase(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	GFI_AnchorShow(g,1);
return( true );
}

int AnchorClassesNextMerge(AnchorClass *ac) {
    int max=0;

    while ( ac!=NULL ) {
	if ( ac->merge_with>max ) max = ac->merge_with;
	ac = ac->next;
    }
return( max + 1 );
}

static void AnchorClassNameDecompose(AnchorClass *ac,const unichar_t *line) {
    unichar_t *end;

    free(ac->name);
    ac->feature_tag = (line[0]<<24) | (line[1]<<16) | (line[2]<<8) | line[3];
    line += 5;
    if (( line[0]=='r' || line[0]==' ' ) &&
	    ( line[1]=='b' || line[1]==' ' ) &&
	    ( line[2]=='l' || line[2]==' ' ) &&
	    ( line[3]=='m' || line[3]==' ' ) &&
	    line[4]==' ' ) {
	ac->flags = 0;
	if ( line[0]=='r' ) ac->flags |= pst_r2l;
	if ( line[1]=='b' ) ac->flags |= pst_ignorebaseglyphs;
	if ( line[2]=='l' ) ac->flags |= pst_ignoreligatures;
	if ( line[3]=='m' ) ac->flags |= pst_ignorecombiningmarks;
	line += 5;
    }
    ac->script_lang_index = u_strtol(line,&end,10);
    ac->type = u_strtol(end,&end,10);
    ac->merge_with = u_strtol(end,&end,10);
    while ( *end==' ' ) ++end;
    ac->name = u_copy(end);
}

static void GFI_GetAnchors(struct gfi_data *d) {
    GGadget *list = GWidgetGetControl(d->gw,CID_AnchorClasses);
    int len;
    GTextInfo **old = GGadgetGetList(list,&len);
    AnchorClass *klast=NULL, *test;
    int i;
    SplineFont *sf = d->sf;

    for ( i=0; i<len; ++i ) {
	test = chunkalloc(sizeof(AnchorClass));
	AnchorClassNameDecompose(test,old[i]->text);
	if ( sf->anchor==NULL )
	    sf->anchor = test;
	else
	    klast->next = test;
	klast = test;
    }
}

static unichar_t *GFI_AskNameTag(int title,unichar_t *def,uint32 def_tag, uint16 flags,
	int sli, enum possub_type type, struct gfi_data *d,
	SplineChar *default_script, int merge_with, int act_type ) {
    AnchorClass *oldancs;
    unichar_t *newname;

    oldancs = d->sf->anchor;
    d->sf->anchor = NULL;
    GFI_GetAnchors(d);

    newname = AskNameTag(title,def,def_tag,flags,sli,type,d->sf,
	    default_script,merge_with,act_type);
    AnchorClassesFree(d->sf->anchor);
    d->sf->anchor = oldancs;
return( newname );
}

static int GFI_AnchorNew(GGadget *g, GEvent *e) {
    int len, i;
    GTextInfo **old, **new;
    GGadget *list;
    unichar_t *newname;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));

	newname = GFI_AskNameTag(_STR_NewAnchorClass,NULL,CHR('m','a','r','k'),0,
		-1, pst_anchors,d,NULL,AnchorClassesNextMerge(d->sf->anchor),act_mark);

	if ( newname!=NULL ) {
	    list = GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorClasses);
	    old = GGadgetGetList(list,&len);
	    for ( i=0; i<len; ++i ) {
		if ( u_strcmp(old[i]->text+4,newname+4)==0 )
	    break;
	    }
	    if ( i<len ) {
		GWidgetErrorR(_STR_DuplicateName,_STR_DuplicateName);
		free(newname);
return( true );
	    }
	    if ( uc_strncmp(newname,"curs",4)==0 ) {
		for ( i=0; i<len; ++i ) {
		    if ( uc_strncmp(old[i]->text,"curs",4)==0 )
		break;
		}
		if ( i<len ) {
		    GWidgetErrorR(_STR_OnlyOne,_STR_OnlyOneCurs);
		    free(newname);
return( true );
		}
	    }
	    new = gcalloc(len+2,sizeof(GTextInfo *));
	    for ( i=0; i<len; ++i ) {
		new[i] = galloc(sizeof(GTextInfo));
		*new[i] = *old[i];
		new[i]->text = u_copy(new[i]->text);
	    }
	    new[i] = gcalloc(1,sizeof(GTextInfo));
	    new[i]->fg = new[i]->bg = COLOR_DEFAULT;
	    new[i]->userdata = NULL;
	    new[i]->text = newname;
	    new[i+1] = gcalloc(1,sizeof(GTextInfo));
	    GGadgetSetList(list,new,false);
	}
    }
return( true );
}

void GListMoveSelected(GGadget *list,int offset) {
    int len, i,j;
    GTextInfo **old, **new;

    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    j = (offset<0 ) ? 0 : len-1;
    for ( i=0; i<len; ++i ) if ( old[i]->selected ) {
	if ( offset==0x80000000 || offset==0x7fffffff )
	    /* Do Nothing */;
	else if ( offset<0 ) {
	    if ( (j= i+offset)<0 ) j=0;
	    while ( new[j] ) ++j;
	} else {
	    if ( (j= i+offset)>=len ) j=len-1;
	    while ( new[j] ) --j;
	}
	new[j] = galloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	if ( offset<0 ) ++j; else --j;
    }
    for ( i=j=0; i<len; ++i ) if ( !old[i]->selected ) {
	while ( new[j] ) ++j;
	new[j] = galloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	++j;
    }
    new[len] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
}

void GListDelSelected(GGadget *list) {
    int len, i,j;
    GTextInfo **old, **new;

    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    for ( i=j=0; i<len; ++i ) if ( !old[i]->selected ) {
	new[j] = galloc(sizeof(GTextInfo));
	*new[j] = *old[i];
	new[j]->text = u_copy(new[j]->text);
	++j;
    }
    new[j] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
}

GTextInfo *GListChangeLine(GGadget *list,int pos, const unichar_t *line) {
    GTextInfo **old, **new;
    int32 i,len;
    
    old = GGadgetGetList(list,&len);
    new = gcalloc(len+1,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = galloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	if ( i!=pos )
	    new[i]->text = u_copy(new[i]->text);
	else
	    new[i]->text = u_copy(line);
    }
    new[i] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
return( new[pos]);
}

GTextInfo *GListAppendLine(GGadget *list,const unichar_t *line,int select) {
    GTextInfo **old, **new;
    int32 i,len;
    
    old = GGadgetGetList(list,&len);
    new = gcalloc(len+2,sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	new[i] = galloc(sizeof(GTextInfo));
	*new[i] = *old[i];
	new[i]->text = u_copy(new[i]->text);
	if ( select ) new[i]->selected = false;
    }
    new[i] = gcalloc(1,sizeof(GTextInfo));
    new[i]->fg = new[i]->bg = COLOR_DEFAULT;
    new[i]->userdata = NULL;
    new[i]->text = u_copy(line);
    new[i]->selected = select;
    new[i+1] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,new,false);
return( new[i]);
}

static int GFI_AnchorDel(GGadget *g, GEvent *e) {
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorClasses);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorDel),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorRename),false);
    }
return( true );
}

static int GFI_AnchorRename(GGadget *g, GEvent *e) {
    int len, i;
    GTextInfo **old, **new, *ti;
    GGadget *list;
    unichar_t *newname;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_AnchorClasses);
	if ( (ti = GGadgetGetListItemSelected(list))==NULL )
return( true );
	newname = GFI_AskNameTag(_STR_EditAnchorClass,ti->text,0,0,0,pst_anchors,
		d,NULL,0,0);
	if ( newname!=NULL ) {
	    old = GGadgetGetList(list,&len);
	    if (( uc_strncmp(newname,"curs",4)==0 && uc_strncmp(ti->text,"curs",4)!=0 ) ||
		    ( uc_strncmp(newname,"curs",4)!=0 && uc_strncmp(ti->text,"curs",4)==0 )) {
		GWidgetErrorR(_STR_CantChange,_STR_CantChangeCurs);
		free(newname);
return( false );
	    }
	    for ( i=0; i<len; ++i ) if ( old[i]!=ti ) {
		if ( u_strcmp(old[i]->text,newname)==0 )
	    break;
	    }
	    if ( i==len ) {
		for ( i=0; i<len; ++i ) if ( old[i]!=ti ) {
		    if ( u_strcmp(old[i]->text+4,newname+4)==0 )
		break;
		}
		if ( i<len ) {
		    GWidgetErrorR(_STR_DuplicateName,_STR_DupAnchorClassNotTag,newname);
		    free(newname);
return( false );
		}
	    } else {
		static int buts[] = { _STR_Continue, _STR_Cancel, 0 };
		if ( GWidgetAskR(_STR_DuplicateName,buts,0,1,_STR_DupAnchorClass,newname)==1 )
return( false );
	    }
	    new = gcalloc(len+1,sizeof(GTextInfo *));
	    for ( i=0; i<len; ++i ) {
		new[i] = galloc(sizeof(GTextInfo));
		*new[i] = *old[i];
		if ( new[i]->selected && newname!=NULL ) {
		    new[i]->text = newname;
		    newname = NULL;
		} else
		    new[i]->text = u_copy(new[i]->text);
	    }
	    new[i] = gcalloc(1,sizeof(GTextInfo));
	    GGadgetSetList(list,new,false);
	}
    }
return( true );
}

static int GFI_AnchorSelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g), len;
	GTextInfo **old = GGadgetGetList(g,&len);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_AnchorDel),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_AnchorRename),sel!=-1);
	d->anchor_shows[0].restart = true;
	d->anchor_shows[1].restart = true;
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_ShowMark),
		GStringGetResource(_STR_ShowFirstMark,NULL));
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_ShowBase),
		GStringGetResource(_STR_ShowFirstBase,NULL));
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ShowMark),sel!=-1 && old[sel]->userdata!=NULL);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ShowBase),sel!=-1 && old[sel]->userdata!=NULL);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	e->u.control.subtype = et_buttonactivate;
	GFI_AnchorRename(g,e);
    }
return( true );
}

static GTextInfo *FPSTList(SplineFont *sf,enum possub_type type) {
    int len;
    FPST *fpst;
    GTextInfo *ti;
    static const unichar_t nullstr[] = { 0 };

    for ( len=0, fpst = sf->possub; fpst!=NULL; fpst=fpst->next )
	if ( fpst->type == type )
	    ++len;
    ti = gcalloc(len+1,sizeof(GTextInfo));
    for ( len=0, fpst = sf->possub; fpst!=NULL; fpst=fpst->next ) if ( fpst->type==type ) {
	ti[len].text = ClassName(nullstr,fpst->tag,fpst->flags,
		fpst->script_lang_index, -1, -1,false);
	ti[len].fg = ti[len].bg = COLOR_DEFAULT;
	ti[len++].userdata = fpst;
    }
return( ti );
}

void GFI_CCDEnd(struct gfi_data *d) {
    int i;

    d->ccd = NULL;
    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_ContextClasses+i*100);
	int sel = GGadgetGetFirstListSelectedItem(list);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextDel+i*100),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEdit+i*100),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEditData+i*100),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextNew+i*100),true);
    }
}

void GFI_FinishContextNew(struct gfi_data *d,FPST *fpst, unichar_t *newname,
	int success) {
    int off;
    GGadget *list;

    if ( success ) {
	off = fpst->type == pst_contextpos ? 000 :
		fpst->type == pst_contextsub ? 100 :
		fpst->type == pst_chainpos ? 200 :
		fpst->type == pst_chainsub ? 300 : 400;
	list = GWidgetGetControl(d->gw,CID_ContextClasses+off);
	GListAppendLine(list,newname,false)->userdata = fpst;
    } else {
	chunkfree(fpst,sizeof(FPST));
    }
    free(newname);
}

static int GFI_ContextNew(GGadget *g, GEvent *e) {
    int i;
    unichar_t *newname;
    FPST *fpst;
    static int titles[] = { _STR_NewContextPos, _STR_NewContextSub,
	_STR_NewChainPos, _STR_NewChainSub,
	_STR_NewReverseChainSub,
	0 };

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int which = (GGadgetGetCid(g)-CID_ContextNew)/100;

	if ( d->ccd )
return( true );

	newname = GFI_AskNameTag(titles[which],NULL,0,0,
		-1, pst_contextpos+which,d,NULL,-2,-1);

	if ( newname!=NULL ) {
	    fpst = chunkalloc(sizeof(FPST));
	    fpst->type = pst_contextpos + which;
	    fpst->format = fpst->type==pst_reversesub ? pst_reversecoverage : pst_coverage;
	    DecomposeClassName(newname,NULL,&fpst->tag,NULL,&fpst->flags,
		    &fpst->script_lang_index,NULL,NULL);
	    if ( (d->ccd = ContextChainEdit(d->sf,fpst,d,newname))!=NULL ) {
	    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
		    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextDel+i*100),false);
		    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEdit+i*100),false);
		    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEditData+i*100),false);
		    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextNew+i*100),false);
		}
	    }
	}
    }
return( true );
}

static int GFI_ContextDel(GGadget *g, GEvent *e) {
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int off = GGadgetGetCid(g)-CID_ContextDel;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_ContextClasses+off);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_ContextDel+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_ContextEdit+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_ContextEditData+off),false);
    }
return( true );
}

static int GFI_ContextEdit(GGadget *g, GEvent *e) {
    int len, i;
    GTextInfo **old, **new, *ti;
    GGadget *list;
    unichar_t *newname;
    FPST *fpst;
    static int titles[] = { _STR_EditContextPos, _STR_EditContextSub,
	_STR_EditChainPos, _STR_EditChainSub,
	_STR_EditReverseChainSub,
	0 };

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int which = (GGadgetGetCid(g)-CID_ContextEdit)/100;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_ContextClasses+which*100);
	if ( (ti = GGadgetGetListItemSelected(list))==NULL )
return( true );
	fpst = (FPST *) (ti->userdata);
	newname = GFI_AskNameTag(titles[which],ti->text,0,0,0,
		pst_contextpos+which, d,NULL,-2,-1);
	if ( newname!=NULL ) {
	    DecomposeClassName(newname,NULL,&fpst->tag,NULL,&fpst->flags,
		    &fpst->script_lang_index,NULL,NULL);
	    new = gcalloc(len+1,sizeof(GTextInfo *));
	    for ( i=0; i<len; ++i ) {
		new[i] = galloc(sizeof(GTextInfo));
		*new[i] = *old[i];
		if ( new[i]->selected && newname!=NULL ) {
		    new[i]->text = newname;
		    newname = NULL;
		} else
		    new[i]->text = u_copy(new[i]->text);
	    }
	    new[i] = gcalloc(1,sizeof(GTextInfo));
	    GGadgetSetList(list,new,false);
	}
    }
return( true );
}

static int GFI_ContextEditData(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GGadget *list = GWidgetGetControl(d->gw,GGadgetGetCid(g)-CID_ContextEditData+CID_ContextClasses);
	int sel = GGadgetGetFirstListSelectedItem(list), len;
	GTextInfo **old = GGadgetGetList(list,&len);
	int i;
	if ( d->ccd )
return( true );
	if ( (d->ccd = ContextChainEdit(d->sf,(FPST *) (old[sel]->userdata),d,NULL))!=NULL ) {
	    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextDel+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEdit+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEditData+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextNew+i*100),false);
	    }
	}
    }
return( true );
}

static int GFI_ContextSelChanged(GGadget *g, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    int sel = GGadgetGetFirstListSelectedItem(g), len;
    int off = GGadgetGetCid(g)-CID_ContextClasses;
    GTextInfo **old = GGadgetGetList(g,&len);

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextDel+off),sel!=-1 && d->ccd==NULL);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEdit+off),sel!=-1 && d->ccd==NULL);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_ContextEditData+off),
		d->ccd==NULL && sel!=-1 &&
		old[sel]->userdata!=NULL &&
		(((FPST *) (old[sel]->userdata))->format==pst_glyphs ||
		 ((FPST *) (old[sel]->userdata))->format==pst_coverage ||
		 ((FPST *) (old[sel]->userdata))->format==pst_reversecoverage));
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	e->u.control.subtype = et_buttonactivate;
	g = GWidgetGetControl(GGadgetGetWindow(g),GGadgetGetCid(g)-CID_ContextClasses+CID_ContextEditData);
	GFI_ContextEditData(g,e);
    }
return( true );
}

static unichar_t *FeatSetName(SplineFont *sf, int feat, int set) {
    char buf[32];
    unichar_t *temp, *full;

    sprintf( buf, "<%d,%d> ", feat, set );
    temp = PickNameFromMacName(FindMacSettingName(sf,feat,set));
    if ( temp==NULL )
	full = uc_copy(buf);
    else {
	full = galloc((strlen(buf)+u_strlen(temp)+1)*sizeof(unichar_t));
	uc_strcpy(full,buf);
	u_strcat(full,temp);
	free(temp);
    }
return( full );
}

static GTextInfo *SMList(SplineFont *sf,enum asm_type type) {
    int len;
    ASM *sm;
    GTextInfo *ti;

    for ( len=0, sm = sf->sm; sm!=NULL; sm=sm->next )
	if ( sm->type == type )
	    ++len;
    ti = gcalloc(len+1,sizeof(GTextInfo));
    for ( len=0, sm = sf->sm; sm!=NULL; sm=sm->next ) if ( sm->type==type ) {
	if ( type==asm_kern )
	    ti[len].text = u_copy(GStringGetResource(_STR_Kerning,NULL));
	else
	    ti[len].text = FeatSetName(sf,sm->feature,sm->setting);
	ti[len].fg = ti[len].bg = COLOR_DEFAULT;
	ti[len++].userdata = sm;
    }
return( ti );
}

void GFI_SMDEnd(struct gfi_data *d) {
    int i;

    d->smd = NULL;
    for ( i=0; i<3; ++i ) {
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+i*100);
	int sel = GGadgetGetFirstListSelectedItem(list);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMDel+i*100),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMEdit+i*100),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMNew+i*100),true);
    }
}

void GFI_FinishSMNew(struct gfi_data *d,ASM *sm, int success, int isnew) {
    int off;
    GGadget *list;
    unichar_t *name;

    if ( success ) {
	off = sm->type == asm_indic ? 000 :
		sm->type == asm_context ? 100 :
		sm->type == asm_insert ? 200 : 300;
	list = GWidgetGetControl(d->gw,CID_SMList+off);
	if ( sm->type!=asm_kern )
	    name = FeatSetName(d->sf,sm->feature,sm->setting);
	else
	    name = u_copy(GStringGetResource(_STR_Kerning,NULL));
	if ( isnew )
	    GListAppendLine(list,name,false)->userdata = sm;
	else
	    GListChangeLine(list,GGadgetGetFirstListSelectedItem(list),name);
    } else if ( isnew ) {
	chunkfree(sm,sizeof(ASM));
    }
}

static int GFI_SMNew(GGadget *g, GEvent *e) {
    int i;
    ASM *sm;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int which = (GGadgetGetCid(g)-CID_SMNew)/100;

	if ( d->smd )
return( true );

	sm = chunkalloc(sizeof(ASM));
	sm->type = which==0 ? asm_indic : which==1 ? asm_context : which==2 ? asm_insert : asm_kern;
	if ( (d->smd = StateMachineEdit(d->sf,sm,d))!=NULL ) {
	    for ( i=0; i<3; ++i ) {
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMDel+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMEdit+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMNew+i*100),false);
	    }
	}
    }
return( true );
}

static int GFI_SMDel(GGadget *g, GEvent *e) {
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	int off = GGadgetGetCid(g)-CID_SMDel;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_SMList+off);
	GListDelSelected(list);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_SMDel+off),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_SMEdit+off),false);
    }
return( true );
}

static int GFI_SMEdit(GGadget *g, GEvent *e) {
    int i;
    GTextInfo *ti;
    GGadget *list;
    ASM *sm;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int which = (GGadgetGetCid(g)-CID_SMEdit)/100;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_SMList+which*100);
	if ( (ti = GGadgetGetListItemSelected(list))==NULL )
return( true );
	if ( d->smd!=NULL )
return( true );
	sm = (ASM *) (ti->userdata);
	if ( (d->smd = StateMachineEdit(d->sf,sm,d))!=NULL ) {
	    for ( i=0; i<3; ++i ) {
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMDel+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMEdit+i*100),false);
		GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMNew+i*100),false);
	    }
	}
    }
return( true );
}

static int GFI_SMSelChanged(GGadget *g, GEvent *e) {
    struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    int sel = GGadgetGetFirstListSelectedItem(g);
    int off = GGadgetGetCid(g)-CID_SMList;

    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMDel+off),sel!=-1 && d->ccd==NULL);
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_SMEdit+off),sel!=-1 && d->ccd==NULL);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	e->u.control.subtype = et_buttonactivate;
	g = GWidgetGetControl(GGadgetGetWindow(g),GGadgetGetCid(g)-CID_SMList+CID_SMEdit);
	GFI_SMEdit(g,e);
    }
return( true );
}

static int GFI_SMConvert(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	ASM *sm = SMConvertDlg(d->sf);
	GGadget *list = GWidgetGetControl(d->gw,CID_SMList+100);
	while ( sm!=NULL ) {
	    GListAppendLine(list,FeatSetName(d->sf,sm->feature,sm->setting),false)->userdata = sm;
	    sm = sm->next;
	}
    }
return( true );
}

static int GFI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GFI_CancelClose(d);
    }
return( true );
}

static int AskTooFew() {
    static int buts[] = { _STR_OK, _STR_Cancel, 0 };
return( GWidgetAskR(_STR_Toofew,buts,0,1,_STR_Reducing) );
}

static int AskLoseUndoes() {
    static int buts[] = { _STR_OK, _STR_Cancel, 0 };
return( GWidgetAskR(_STR_LosingUndoes,buts,0,1,_STR_ChangingOrderLosesUndoes) );
}

static void BadFamily() {
    GWidgetErrorR(_STR_Badfamily,_STR_Badfamilyn);
}

static char *modifierlist[] = { "Ital", "Obli", "Kursive", "Cursive", "Slanted",
	"Expa", "Cond", NULL };
static char *modifierlistfull[] = { "Italic", "Oblique", "Kursive", "Cursive", "Slanted",
    "Expanded", "Condensed", NULL };
static char **mods[] = { knownweights, modifierlist, NULL };
static char **fullmods[] = { realweights, modifierlistfull, NULL };

static char *_GetModifiers(char *fontname, char *familyname,char *weight) {
    char *pt, *fpt;
    int i, j;

    /* URW fontnames don't match the familyname */
    /* "NimbusSanL-Regu" vs "Nimbus Sans L" (note "San" vs "Sans") */
    /* so look for a '-' if there is one and use that as the break point... */

    if ( (fpt=strchr(fontname,'-'))!=NULL ) {
	++fpt;
	if ( *fpt=='\0' )
	    fpt = NULL;
    } else if ( familyname!=NULL ) {
	for ( pt = fontname, fpt=familyname; *fpt!='\0' && *pt!='\0'; ) {
	    if ( *fpt == *pt ) {
		++fpt; ++pt;
	    } else if ( *fpt==' ' )
		++fpt;
	    else if ( *pt==' ' )
		++pt;
	    else if ( *fpt=='a' || *fpt=='e' || *fpt=='i' || *fpt=='o' || *fpt=='u' )
		++fpt;	/* allow vowels to be omitted from family when in fontname */
	    else
	break;
	}
	if ( *fpt=='\0' && *pt!='\0' )
	    fpt = pt;
	else
	    fpt = NULL;
    }

    if ( fpt == NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    pt = strstr(fontname,mods[i][j]);
	    if ( pt!=NULL && (fpt==NULL || pt<fpt))
		fpt = pt;
	}
    }
    if ( fpt!=NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    if ( strcmp(fpt,mods[i][j])==0 )
return( fullmods[i][j]);
	}
	if ( strcmp(fpt,"BoldItal")==0 )
return( "BoldItalic" );
	else if ( strcmp(fpt,"BoldObli")==0 )
return( "BoldOblique" );

return( fpt );
    }

return( weight==NULL || *weight=='\0' ? "Regular": weight );
}

static const unichar_t *_uGetModifiers(const unichar_t *fontname, const unichar_t *familyname,
	const unichar_t *weight) {
    const unichar_t *pt, *fpt;
    static unichar_t regular[] = { 'R','e','g','u','l','a','r', 0 };
    static unichar_t space[20];
    int i,j;

    /* URW fontnames don't match the familyname */
    /* "NimbusSanL-Regu" vs "Nimbus Sans L" (note "San" vs "Sans") */
    /* so look for a '-' if there is one and use that as the break point... */

    if ( (fpt=u_strchr(fontname,'-'))!=NULL ) {
	++fpt;
	if ( *fpt=='\0' )
	    fpt = NULL;
    } else if ( familyname!=NULL ) {
	for ( pt = fontname, fpt=familyname; *fpt!='\0' && *pt!='\0'; ) {
	    if ( *fpt == *pt ) {
		++fpt; ++pt;
	    } else if ( *fpt==' ' )
		++fpt;
	    else if ( *pt==' ' )
		++pt;
	    else if ( *fpt=='a' || *fpt=='e' || *fpt=='i' || *fpt=='o' || *fpt=='u' )
		++fpt;	/* allow vowels to be omitted from family when in fontname */
	    else
	break;
	}
	if ( *fpt=='\0' && *pt!='\0' )
	    fpt = pt;
	else
	    fpt = NULL;
    }

    if ( fpt==NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    pt = uc_strstr(fontname,mods[i][j]);
	    if ( pt!=NULL && (fpt==NULL || pt<fpt))
		fpt = pt;
	}
    }

    if ( fpt!=NULL ) {
	for ( i=0; mods[i]!=NULL; ++i ) for ( j=0; mods[i][j]!=NULL; ++j ) {
	    if ( uc_strcmp(fpt,mods[i][j])==0 ) {
		uc_strcpy(space,fullmods[i][j]);
return( space );
	    }
	}
	if ( uc_strcmp(fpt,"BoldItal")==0 ) {
	    uc_strcpy(space,"BoldItalic");
return( space );
	} else if ( uc_strcmp(fpt,"BoldObli")==0 ) {
	    uc_strcpy(space,"BoldOblique");
return( space );
	}
return( fpt );
    }

return( weight==NULL || *weight=='\0' ? regular: weight );
}

char *SFGetModifiers(SplineFont *sf) {
return( _GetModifiers(sf->fontname,sf->familyname,sf->weight));
}

static unichar_t *uGetModifiers(unichar_t *fontname, unichar_t *familyname,
	unichar_t *weight) {
    unichar_t *ret;

    ret = u_copy( _uGetModifiers(fontname,familyname,weight));
    free( fontname );
return( ret );
}

void SFSetFontName(SplineFont *sf, char *family, char *mods,char *full) {
    char *n;
    unichar_t *temp; char *pt, *tpt;
    int i;

    n = galloc(strlen(family)+strlen(mods)+2);
    strcpy(n,family); strcat(n," "); strcat(n,mods);
    if ( full==NULL || *full == '\0' )
	full = copy(n);
    for ( pt=tpt=n; *pt; ) {
	if ( !isspace(*pt))
	    *tpt++ = *pt++;
	else
	    ++pt;
    }
    *tpt = '\0';
#if 0
    for ( pt=tpt=family; *pt; ) {
	if ( !isspace(*pt))
	    *tpt++ = *pt++;
	else
	    ++pt;
    }
    *tpt = '\0';
#endif

    free(sf->fullname); sf->fullname = copy(full);

    /* In the URW world fontnames aren't just a simple concatenation of */
    /*  family name and modifiers, so neither the family name nor the modifiers */
    /*  changed, then don't change the font name */
    if ( strcmp(family,sf->familyname)==0 && strcmp(n,sf->fontname)==0 )
	/* Don't change the fontname */;
	/* or anything else */
    else {
	free(sf->fontname); sf->fontname = n;
	free(sf->familyname); sf->familyname = copy(family);
	free(sf->weight); sf->weight = NULL;
	if ( strstrmatch(mods,"extralight")!=NULL || strstrmatch(mods,"extra-light")!=NULL )
	    sf->weight = copy("ExtraLight");
	else if ( strstrmatch(mods,"demilight")!=NULL || strstrmatch(mods,"demi-light")!=NULL )
	    sf->weight = copy("DemiLight");
	else if ( strstrmatch(mods,"demibold")!=NULL || strstrmatch(mods,"demi-bold")!=NULL )
	    sf->weight = copy("DemiBold");
	else if ( strstrmatch(mods,"semibold")!=NULL || strstrmatch(mods,"semi-bold")!=NULL )
	    sf->weight = copy("SemiBold");
	else if ( strstrmatch(mods,"demiblack")!=NULL || strstrmatch(mods,"demi-black")!=NULL )
	    sf->weight = copy("DemiBlack");
	else if ( strstrmatch(mods,"extrabold")!=NULL || strstrmatch(mods,"extra-bold")!=NULL )
	    sf->weight = copy("ExtraBold");
	else if ( strstrmatch(mods,"extrablack")!=NULL || strstrmatch(mods,"extra-black")!=NULL )
	    sf->weight = copy("ExtraBlack");
	else if ( strstrmatch(mods,"book")!=NULL )
	    sf->weight = copy("Book");
	else if ( strstrmatch(mods,"regular")!=NULL )
	    sf->weight = copy("Regular");
	else if ( strstrmatch(mods,"roman")!=NULL )
	    sf->weight = copy("Roman");
	else if ( strstrmatch(mods,"normal")!=NULL )
	    sf->weight = copy("Normal");
	else if ( strstrmatch(mods,"demi")!=NULL )
	    sf->weight = copy("Demi");
	else if ( strstrmatch(mods,"medium")!=NULL )
	    sf->weight = copy("Medium");
	else if ( strstrmatch(mods,"bold")!=NULL )
	    sf->weight = copy("Bold");
	else if ( strstrmatch(mods,"heavy")!=NULL )
	    sf->weight = copy("Heavy");
	else if ( strstrmatch(mods,"black")!=NULL )
	    sf->weight = copy("Black");
	else if ( strstrmatch(mods,"Nord")!=NULL )
	    sf->weight = copy("Nord");
/* Sigh. URW uses 4 letter abreviations... */
	else if ( strstrmatch(mods,"Regu")!=NULL )
	    sf->weight = copy("Regular");
	else if ( strstrmatch(mods,"Medi")!=NULL )
	    sf->weight = copy("Medium");
	else if ( strstrmatch(mods,"blac")!=NULL )
	    sf->weight = copy("Black");
	else
	    sf->weight = copy("Medium");
    }

    if ( sf->fv!=NULL && sf->fv->gw!=NULL ) {
	GDrawSetWindowTitles(sf->fv->gw,temp = uc_copy(sf->fontname),NULL);
	free(temp);
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]->views!=NULL ) {
	    char buffer[300]; unichar_t ubuf[300]; CharView *cv;
	    sprintf( buffer, "%.90s from %.90s", sf->chars[i]->name, sf->fontname );
	    uc_strcpy(ubuf,buffer);
	    for ( cv = sf->chars[i]->views; cv!=NULL; cv=cv->next )
		GDrawSetWindowTitles(cv->gw,ubuf,NULL);
	}
    }
}

static int SetFontName(GWindow gw, SplineFont *sf) {
    const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
    const unichar_t *ufont = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Fontname));
    const unichar_t *uweight = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Weight));
    const unichar_t *uhum = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Human));
    int diff = uc_strcmp(ufont,sf->fontname)!=0;

    free(sf->familyname);
    free(sf->fontname);
    free(sf->weight);
    free(sf->fullname);
    sf->familyname = cu_copy(ufamily);
    sf->fontname = cu_copy(ufont);
    sf->weight = cu_copy(uweight);
    sf->fullname = cu_copy(uhum);
return( diff );
}

static int CheckNames(struct gfi_data *d) {
    const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Family));
    const unichar_t *ufont = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname));
    unichar_t *end;

    if ( *ufamily=='\0' ) {
	GWidgetErrorR(_STR_FamilyNameRequired,_STR_FamilyNameRequired);
return( false );
    }
    /* A postscript name cannot be a number. There are two ways it can be a */
    /*  number, it can be a real (which we can check for with strtod) or */
    /*  it can be a "radix number" which is <intval>'#'<intval>. I'll only */
    /*  do a cursory test for that */
    u_strtod(ufamily,&end);
    if ( *end=='\0' || (isdigit(ufamily[0]) && u_strchr(ufamily,'#')!=NULL) ) {
	GWidgetErrorR(_STR_BadFamilyName,_STR_PSNameNotNumber);
return( false );
    }
    while ( *ufamily ) {
	if ( *ufamily<' ' || *ufamily>=0x7f ) {
	    GWidgetErrorR(_STR_BadFamilyName,_STR_BadPSName);
return( false );
	}
	++ufamily;
    }

    u_strtod(ufont,&end);
    if ( (*end=='\0' || (isdigit(ufont[0]) && u_strchr(ufont,'#')!=NULL)) &&
	    *ufont!='\0' ) {
	GWidgetErrorR(_STR_BadFontName,_STR_PSNameNotNumber);
return( false );
    }
    while ( *ufont ) {
	if ( *ufont<=' ' || *ufont>=0x7f ||
		*ufont=='(' || *ufont=='[' || *ufont=='{' || *ufont=='<' ||
		*ufont==')' || *ufont==']' || *ufont=='}' || *ufont=='>' ||
		*ufont=='%' || *ufont=='/' ) {
	    GWidgetErrorR(_STR_BadFontName,_STR_BadPSName);
return( false );
	}
	++ufont;
    }
    if ( u_strlen(ufont)>63 ) {
	GWidgetErrorR(_STR_BadFontName,_STR_BadPSName);
return( false );
    }
return( true );
}

static int stylematch(const unichar_t *pt, const unichar_t **end) {
    int i;
    struct langstyle *test;

    for ( i=0; stylelist[i]!=NULL; ++i ) {
	for ( test = stylelist[i]; test->lang==0x409; ++test )
	    if ( u_strnmatch(pt,test->str,u_strlen(test->str))==0 ) {
		*end = pt + u_strlen(test->str);
return( i );
	}
    }
return( -1 );
}

static void DoDefaultStyles(struct gfi_data *d) {
    const unichar_t *styles = _GGadgetGetTitle(GWidgetGetControl(d->gw, CID_String));
    const unichar_t *pt, *end;
    int trans[10], i=0, langs[30], j,k,l, match;
    struct langstyle *test;
    struct ttflangname *ln, *prev;

    if ( *styles=='\0' ) styles = regulareng;
    for ( pt=styles; *pt!='\0' ; ) {
	if ( (match=stylematch(pt,&end))==-1 )
	    ++pt;
	else {
	    if ( i<sizeof(trans)/sizeof(trans[0])-1 )
		trans[i++] = match;
	    pt = end;
	}
    }
    trans[i] = -1;
    if ( i==0 )
return;

    for ( test=stylelist[trans[0]], j=0; test->lang!=0; ++test ) {
	if ( test->lang!=0x409 && j<sizeof(langs)/sizeof(langs[0])-1 )
	    langs[j++] = test->lang;
    }
    for ( k=1; k<i; ++k ) {
	for ( l=0; l<j; ++l ) {
	    for ( test=stylelist[trans[k]]; test->lang!=0; ++test ) {
		if ( test->lang==langs[l] )
	    break;
	    }
	    if ( test->lang==0 ) {
		/* No translation for this style, so give up on this lang */
		--j;
		for ( ; l<j; ++l )
		    langs[l] = langs[l+1];
	    }
	}
    }
    
    for ( l=0; l<j; ++l ) {
	for ( prev = NULL, ln = d->names; ln!=NULL && ln->lang!=langs[l]; prev = ln, ln = ln->next );
	if ( ln==NULL ) {
	    ln = gcalloc(1,sizeof(struct ttflangname));
	    ln->lang = langs[l];
	    if ( prev==NULL ) { ln->next = d->names; d->names = ln; }
	    else { ln->next = prev->next; prev->next = ln; }
	}
	if ( ln->names[ttf_subfamily]==NULL ) {
	    unichar_t *res = NULL;
	    int len;
	    while ( 1 ) {
		len = 0;
		for ( k=0; k<i; ++k ) {
		    for ( test=stylelist[trans[k]]; test->lang!=0 && test->lang!=langs[l]; ++test );
		    if ( test->str!=NULL ) {
			if ( res!=NULL )
			    u_strcpy(res+len,test->str);
			len += u_strlen(test->str);
		    }
		}
		if ( res!=NULL )
	    break;
		res = galloc((len+1)*sizeof(unichar_t));
	    }
	    ln->names[ttf_subfamily] = res;
	}
    }
}

static void TNNotePresence(struct gfi_data *d, int strid) {
    GGadget *list = GWidgetGetControl(d->gw,CID_Language);
    int i,len, lang;
    GTextInfo **ti = GGadgetGetList(list,&len);
    struct ttflangname *cur;
    Color fore = GDrawGetDefaultForeground(NULL);

    for ( i=0; i<len; ++i ) {
	lang = (int) ti[i]->userdata;
	for ( cur=d->names; cur!=NULL && cur->lang!=lang; cur=cur->next );
	if ( strid==-1 )
	    ti[i]->fg = cur==NULL ? fore : COLOR_CREATE(0,0x80,0);
	else {
	    ti[i]->fg = cur==NULL || cur->names[strid]==NULL ? fore : COLOR_CREATE(0,0x80,0);
	    if ( lang==0x409 && d->def.names[strid]!=NULL )
		ti[i]->fg = COLOR_CREATE(0,0x80,0);
	}
    }
}

struct ttflangname *TTFLangNamesCopy(struct ttflangname *old) {
    struct ttflangname *base=NULL, *last, *cur;
    int i;

    while ( old!=NULL ) {
	cur = gcalloc(1,sizeof(struct ttflangname));
	cur->lang = old->lang;
	for ( i=0; i<ttf_namemax; ++i )
	    cur->names[i] = u_copy(old->names[i]);
	if ( base )
	    last->next = cur;
	else
	    base = cur;
	last = cur;
	old = old->next;
    }
return( base );
}

void TTF_PSDupsDefault(SplineFont *sf) {
    struct ttflangname *english;
    char versionbuf[40];

    /* Ok, if we've just loaded a ttf file then we've got a bunch of langnames*/
    /*  we copied some of them (copyright, family, fullname, etc) into equiv */
    /*  postscript entries in the sf. If we then use FontInfo and change the */
    /*  obvious postscript entries we are left with the old ttf entries. If */
    /*  we generate a ttf file and then load it the old values pop up. */
    /* Solution: Anything we can generate by default should be set to NULL */
    for ( english=sf->names; english!=NULL && english->lang!=0x409; english=english->next );
    if ( english==NULL )
return;
    if ( english->names[ttf_family]!=NULL &&
	    uc_strcmp(english->names[ttf_family],sf->familyname)==0 ) {
	free(english->names[ttf_family]);
	english->names[ttf_family]=NULL;
    }
    if ( english->names[ttf_copyright]!=NULL &&
	    uc_strcmp(english->names[ttf_copyright],sf->copyright)==0 ) {
	free(english->names[ttf_copyright]);
	english->names[ttf_copyright]=NULL;
    }
    if ( english->names[ttf_fullname]!=NULL &&
	    uc_strcmp(english->names[ttf_fullname],sf->fullname)==0 ) {
	free(english->names[ttf_fullname]);
	english->names[ttf_fullname]=NULL;
    }
    if ( sf->subfontcnt!=0 || sf->version!=NULL ) {
	if ( sf->subfontcnt!=0 )
	    sprintf( versionbuf, "Version %f", sf->cidversion );
	else
	    sprintf(versionbuf,"Version %.20s ", sf->version);
	if ( english->names[ttf_version]!=NULL &&
		uc_strcmp(english->names[ttf_version],versionbuf)==0 ) {
	    free(english->names[ttf_version]);
	    english->names[ttf_version]=NULL;
	}
    }
    if ( english->names[ttf_subfamily]!=NULL &&
	    uc_strcmp(english->names[ttf_subfamily],SFGetModifiers(sf))==0 ) {
	free(english->names[ttf_subfamily]);
	english->names[ttf_subfamily]=NULL;
    }

    /* User should not be allowed any access to this one, not ever */
    free(english->names[ttf_postscriptname]);
    english->names[ttf_postscriptname]=NULL;
}

static unichar_t versionformatspec[] = { 'V','e','r','s','i','o','n',' ','%','.','2','0','s',' ', '\0' };

static void TTF_PSDupsChanged(GWindow gw,SplineFont *sf,struct ttflangname *newnames) {
    struct ttflangname *english, *sfenglish;
    const unichar_t *txt, *txt1;
#define offsetfrom(type,name) (((char *) &(((type *) NULL)->name)) - (char *) NULL)
    static struct dupttfps { int ttf, cid, sid, off; } dups[] =
	{{ ttf_copyright, CID_Notice, _STR_Copyright, offsetfrom(SplineFont,copyright) },
	 { ttf_family, CID_Family, _STR_Family, offsetfrom(SplineFont,familyname) },
	 { ttf_fullname, CID_Human, _STR_Fullname, offsetfrom(SplineFont,fullname) },
	 { ttf_version, CID_Version, _STR_Version, offsetfrom(SplineFont,version) },
	 { ttf_subfamily, CID_Fontname, _STR_Styles, offsetfrom(SplineFont,fontname) },
	 { -1 }};
#undef offsetfrom
    int changeall = -1, i;
    unichar_t versionbuf[40];

    for ( english=newnames; english!=NULL && english->lang!=0x409; english=english->next );
    if ( english==NULL )
return;
    for ( sfenglish=sf->names; sfenglish!=NULL && sfenglish->lang!=0x409; sfenglish=sfenglish->next );
    for ( i=0; dups[i].ttf!=-1; ++i ) {
	txt=txt1=_GGadgetGetTitle(GWidgetGetControl(gw,dups[i].cid));
	if ( dups[i].cid==CID_Fontname )
	    txt1 = _uGetModifiers(txt,_GGadgetGetTitle(GWidgetGetControl(gw,CID_Family)),
		    _GGadgetGetTitle(GWidgetGetControl(gw,CID_Weight)));
	else if ( dups[i].cid==CID_Version ) {
	    u_sprintf(versionbuf,versionformatspec,txt);
	    txt1 = versionbuf;
	    if ( english->names[dups[i].ttf]!=NULL &&
		    u_strcmp(txt1,english->names[dups[i].ttf])!=0 )
		versionbuf[u_strlen(versionbuf)-1] = '\0';	/* Trailing space often omitted */
	}
	if ( english->names[dups[i].ttf]!=NULL &&
		uc_strcmp(txt,*(char **) (((char *) sf) + dups[i].off))!=0 &&
		u_strcmp(txt1,english->names[dups[i].ttf])!=0 ) {
	    /* If we've got an entry for this guy, and the user changed the ps */
	    /*  version of the name, and the new ps version doesn't match what */
	    /*  we've got then.... */
	    if ( sfenglish==NULL || sfenglish->names[dups[i].ttf]==NULL ) {
		/* User has never set this value, let's set the new value to default */
		free(english->names[dups[i].ttf]);
		english->names[dups[i].ttf] = NULL;
	    } else if ( u_strcmp(sfenglish->names[dups[i].ttf],english->names[dups[i].ttf])!=0 ) {
		/* User has explicitly changed this from what it was, so presumably */
		/*  s/he knows what s/he is doing */
	    } else {
		int ans=-1;
		if ( changeall==-1 ) {
		    static int buts[] = { _STR_Change, _STR_ChangeAll, _STR_RetainAll, _STR_Retain, 0 };
		    ans = GWidgetAskR(_STR_Mismatch,buts,0,3,_STR_MismatchLong,
			    GStringGetResource(dups[i].sid,NULL));
		    if ( ans==1 ) changeall=1;
		    else if ( ans==2 ) changeall=0;
		}
		if ( changeall==1 || ans==0 ) {
		    free(english->names[dups[i].ttf]);
		    english->names[dups[i].ttf] = NULL;
		}
	    }
	}
    }
}

static int GFI_DefaultStyles(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	DoDefaultStyles(d);
	TNNotePresence(d,ttf_subfamily);
    }
return( true );
}

static void TNFinishFormer(struct gfi_data *d) {
    GGadget *langlist = GWidgetGetControl(d->gw,CID_Language);
    int cur_lang = GGadgetGetFirstListSelectedItem(langlist);
    int cur_id = GGadgetGetFirstListSelectedItem(GWidgetGetControl(d->gw,CID_StrID));
    struct ttflangname *cur, *prev;
    int nothing;
    static unichar_t nullstr[1] = { 0 };
    int i;
    int32 len;
    GTextInfo **langs = GGadgetGetList(langlist,&len);

    if ( d->old_lang!=-1 ) {
	int lang = (int) langs[d->old_lang]->userdata;
	int id = (int) ttfnameids[d->old_strid].userdata;
	const unichar_t *str = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_String));

	for ( prev=NULL, cur = d->names; cur!=NULL && cur->lang!=lang; prev = cur, cur=cur->next );
	if ( lang==0x409 /* US English, default */ && d->def.names[id]!=NULL &&
		u_strcmp(str,d->def.names[id])==0 ) {
	    if ( cur != NULL ) {
		free(cur->names[id]);
		cur->names[id] = NULL;
	    }
  goto finishup;	/* If it's the default value then ignore it */
	}

	nothing = false;
	if ( *str=='\0' && cur!=NULL ) {
	    nothing = true;
	    for ( i=0; i<ttf_namemax && nothing; ++i )
		if ( cur->names[i]!=NULL && i!=id ) nothing = false;
	}
	if ( cur==NULL && *str=='\0' )
  goto finishup;
	else if ( cur==NULL ) {
	    cur = gcalloc(1,sizeof(struct ttflangname));
	    if ( prev==NULL )
		d->names = cur;
	    else
		prev->next = cur;
	    cur->lang = lang;
	}

	if ( nothing ) {
	    if ( prev==NULL )
		d->names = cur->next;
	    else
		prev->next = cur->next;
	    cur->next = NULL;
	    TTFLangNamesFree(cur);
	} else {
	    if ( *str=='\0' ) {
		free(cur->names[id]);
		cur->names[id] = NULL;
	    } else if ( cur->names[id]==NULL || u_strcmp(cur->names[id],str)!=0 ) {
		free(cur->names[id]);
		cur->names[id] = u_copy(str);
	    }
	}
    }
  finishup:
    d->old_lang = cur_lang;
    d->old_strid = cur_id;

    cur_id =(int) ttfnameids[cur_id].userdata;
    cur_lang = (int) langs[cur_lang]->userdata;
    TNNotePresence(d,cur_id);
    for ( prev=NULL, cur = d->names; cur!=NULL && cur->lang!=cur_lang; cur=cur->next );
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_String),
	    cur!=NULL && cur->names[cur_id]!=NULL?cur->names[cur_id]:
	    cur_lang == 0x409 && d->def.names[cur_id]!=NULL?d->def.names[cur_id]:
	    nullstr );
    GGadgetSetVisible(GWidgetGetControl(d->gw,CID_TNDef),cur_id==ttf_subfamily && cur_lang==0x409);
}

static int GFI_LanguageChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	TNFinishFormer(d);
    }
return( true );
}

static int LangSearch(GTextInfo **langs,const char *lang,int langlen) {
    int i,reslen, found=-1, samelang=-1;
    const unichar_t *res;
    
    for ( i=0; langs[i]->text!=NULL; ++i ) {
	res = langs[i]->text;
	if ( res==NULL )
    continue;
	reslen = u_strlen(res);
	if ( langlen==2 ) {
	    if ( uc_strcmp(res+reslen-2,lang)==0 || uc_strncmp(res+reslen-5,lang,2)==0 ) {
		found = i;
    break;
	    }
	} else {
	    if ( uc_strncmp(res+reslen-5,lang,5)==0 ) {
		found = i;
    break;
	    /* Use the first locale of the language. It usually specifies the standard */
	    } else if ( samelang==-1 && ( uc_strncmp(res+reslen-5,lang,2)==0 || uc_strncmp(res+reslen-2,lang,2)==0 ))
		samelang = i;
	}
    }
    if ( found==-1 ) found = samelang;
return( found );
}

static void DefaultLanguage(struct gfi_data *d) {
    const char *lang=NULL;
    int i, found=-1, langlen;
    static char *envs[] = { "LC_ALL", "LC_MESSAGES", "LANG", NULL };
    GGadget *g = GWidgetGetControl(d->gw,CID_Language);
    unichar_t versionbuf[40];
    int32 len;
    GTextInfo **langs = GGadgetGetList(g,&len);

    for ( i=0; envs[i]!=NULL && lang==NULL; ++i )
	lang = getenv(envs[i]);
    if ( lang==NULL ) lang = "en_US";
    langlen = strlen(lang);
    found = LangSearch(langs,lang,langlen);
    if ( langlen==2 ) {
	char buffer[6];
	int test;
	/* Guess that the default locale has the same two letter code as language */
	sprintf( buffer, "%s_%c%c", lang, toupper(lang[0]), toupper(lang[1]));
	test = LangSearch(langs,buffer,5);
	if ( test!=-1 )
	    found = test;
    }
    if ( found==-1 ) found = 0;
    GGadgetSelectOneListItem(g,found);

    d->old_lang = -1;
    d->names_set = true;
    d->names = TTFLangNamesCopy(d->sf->names);
    d->def.names[ttf_copyright] = GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Notice));
    d->def.names[ttf_family] = GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Family));
    d->def.names[ttf_fullname] = GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Human));
    d->def.names[ttf_subfamily] = uGetModifiers(
	    GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname)),
	    d->def.names[ttf_family],
	    GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Weight)));
    u_sprintf(versionbuf,versionformatspec,
	    _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Version)));
    d->def.names[ttf_version] = u_copy(versionbuf);
    DefaultTTFEnglishNames(&d->def, d->sf);
    TNFinishFormer(d);
}

static void GFI_ProcessAnchor(struct gfi_data *d) {
    GGadget *list = GWidgetGetControl(d->gw,CID_AnchorClasses);
    int len;
    GTextInfo **old = GGadgetGetList(list,&len);
    AnchorClass *keep=NULL, *klast, *test, *prev, *next, *test2;
    int i;

    for ( i=0; i<len; ++i ) {
	if ( old[i]->userdata==NULL ) {
	    test = chunkalloc(sizeof(AnchorClass));
	} else {
	    prev = NULL;
	    for ( test = d->sf->anchor; test!=old[i]->userdata; prev=test, test=test->next );
	    if ( prev==NULL )
		d->sf->anchor = test->next;
	    else
		prev->next = test->next;
	    test->next = NULL;
	}
	AnchorClassNameDecompose(test,old[i]->text);
	if ( keep==NULL )
	    keep = test;
	else
	    klast->next = test;
	klast = test;
    }
    for ( test = d->sf->anchor; test!=NULL; test = next ) {
	next = test->next;
	SFRemoveAnchorClass(d->sf,test);
    }
    d->sf->anchor = keep;

    for ( test=d->sf->anchor; test!=NULL; test=test->next ) {
	prev = test;
	for ( test2=test->next; test2!=NULL; test2 = next ) {
	    next = test2->next;
	    if ( u_strcmp(test->name,test2->name)==0 ) {
		AnchorClassMerge(d->sf,test,test2);
		prev->next = next;
		chunkfree(test2,sizeof(*test2));
	    } else
		prev = test2;
	}
    }
}

static void BDFsSetAsDs(SplineFont *sf) {
    BDFFont *bdf;
    real scale;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	scale = bdf->pixelsize / (real) (sf->ascent+sf->descent);
	bdf->ascent = rint(sf->ascent*scale);
	bdf->descent = bdf->pixelsize-bdf->ascent;
    }
}

static int texparams[] = { _STR_Slant, _STR_Space, _STR_Stretch, _STR_Shrink, _STR_XHeightC, _STR_Quad, _STR_MathSp, 0 };
static int texpopups[] = { _STR_SlantPopup, _STR_SpacePopup, _STR_StretchPopup, _STR_ShrinkPopup, _STR_XHeightCPopup, _STR_QuadPopup, _STR_ExtraPopup, 0 };

static int ParseTeX(struct gfi_data *d) {
    int i, err=false;
    double em = (d->sf->ascent+d->sf->descent), val;

    for ( i=0; texparams[i]!=0 ; ++i ) {
	val = GetRealR(d->gw,CID_TeX+i,texparams[i],&err);
	d->texdata.params[i] = rint( val/em * (1<<20) );
    }
    d->texdata.designsize = rint( GetRealR(d->gw,CID_DesignSize,_STR_DesignSize,&err) * (1<<20) );
    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXText)) )
	d->texdata.type = tex_text;
    else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMath)) )
	d->texdata.type = tex_math;
    else
	d->texdata.type = tex_mathext;
return( !err );
}

static int GFI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct gfi_data *d = GDrawGetUserData(gw);
	SplineFont *sf = d->sf, *_sf;
	int enc, interp;
	int reformat_fv=0, enc_changed, retitle_fv=false;
	int upos, uwid, as, des, nchar, oldcnt=sf->charcnt, err = false, weight=0;
	int uniqueid, linegap=0, vlinegap=0, winascent=0, windescent=0;
	int winaoff=true, windoff=true;
	int force_enc=0;
	real ia, cidversion;
	const unichar_t *txt, *fond; unichar_t *end;
	int i,j, mcs;
	int vmetrics, vorigin, namechange, order2;
	int xuidchanged = false;
	GTextInfo *pfmfam, *fstype;
	int32 len;
	GTextInfo **ti;
#ifdef FONTFORGE_CONFIG_TYPE3
	int multilayer = false;
#endif

	if ( !CheckNames(d))
return( true );
	if ( !PIFinishFormer(d))
return( true );
	if ( d->names_set )
	    TNFinishFormer(d);
	if ( d->ccd )
	    CCD_Close(d->ccd);
	if ( d->smd )
	    SMD_Close(d->smd);

	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
	if ( !isalpha(*txt)) {
	    BadFamily();
return( true );
	}
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_ItalicAngle));
	ia = u_strtod(txt,&end);
	if ( *end!='\0' ) {
	    ProtestR(_STR_Italicangle);
return(true);
	}
	order2 = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsOrder2));
#ifdef FONTFORGE_CONFIG_TYPE3
	multilayer = GGadgetIsChecked(GWidgetGetControl(gw,CID_IsMultiLayer));
#endif
	vmetrics = GGadgetIsChecked(GWidgetGetControl(gw,CID_HasVerticalMetrics));
	upos = GetIntR(gw,CID_UPos, _STR_Upos,&err);
	uwid = GetIntR(gw,CID_UWidth,_STR_Uheight,&err);
	GetIntR(gw,CID_Em,_STR_EmSize,&err);	/* just check for errors. redundant info */
	as = GetIntR(gw,CID_Ascent,_STR_Ascent,&err);
	des = GetIntR(gw,CID_Descent,_STR_Descent,&err);
	nchar = GetIntR(gw,CID_NChars,_STR_Numchars,&err);
	uniqueid = GetIntR(gw,CID_UniqueID,_STR_UniqueID,&err);
	force_enc = GGadgetIsChecked(GWidgetGetControl(gw,CID_ForceEncoding));
	if ( sf->subfontcnt!=0 )
	    cidversion = GetRealR(gw,CID_Version,_STR_Version,&err);
	if ( vmetrics )
	    vorigin = GetIntR(gw,CID_VOrigin,_STR_VOrigin,&err);
	if ( d->ttf_set ) {
	    /* Only use the normal routine if we get no value, because */
	    /*  "400 Book" is a reasonable setting, but would cause GetInt */
	    /*  to complain */
	    weight = u_strtol(_GGadgetGetTitle(GWidgetGetControl(gw,CID_WeightClass)),NULL,10);
	    if ( weight == 0 )
		weight = GetIntR(gw,CID_WeightClass,_STR_WeightClass,&err);
	    linegap = GetIntR(gw,CID_LineGap,_STR_LineGap,&err);
	    if ( vmetrics )
		vlinegap = GetIntR(gw,CID_VLineGap,_STR_VLineGap,&err);
	    winaoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_WinAscentIsOff));
	    windoff = GGadgetIsChecked(GWidgetGetControl(gw,CID_WinDescentIsOff));
	    winascent  = GetIntR(gw,CID_WinAscent,winaoff ? _STR_WinAscentOff : _STR_WinAscent,&err);
	    windescent = GetIntR(gw,CID_WinDescent,windoff ? _STR_WinDescentOff : _STR_WinDescent,&err);
	}
	if ( d->tex_set ) {
	    if ( !ParseTeX(d))
return( true );
	}
	if ( err )
return(true);
	if ( as+des>16384 || des<0 || as<0 ) {
	    GWidgetErrorR(_STR_Badascentdescent,_STR_Badascentdescentn);
return( true );
	}
	mcs = -1;
	if ( !GGadgetIsChecked(GWidgetGetControl(d->gw,CID_MacAutomatic)) ) {
	    mcs = 0;
	    ti = GGadgetGetList(GWidgetGetControl(d->gw,CID_MacStyles),&len);
	    for ( i=0; i<len; ++i )
		if ( ti[i]->selected )
		    mcs |= (int) (intpt) ti[i]->userdata;
	    if ( (mcs&sf_condense) && (mcs&sf_extend)) {
		GWidgetErrorR(_STR_BadStyle,_STR_NotBothCondenseExtend);
return( true );
	    }
	}
	if ( nchar<sf->charcnt && AskTooFew())
return(true);
	if ( order2!=sf->order2 && AskLoseUndoes())
return( true );
	if ( order2!=sf->order2 && !SFCloseAllInstrs(sf))
return( true );
#ifdef FONTFORGE_CONFIG_TYPE3
	if ( multilayer!=sf->multilayer ) {
	    sf->multilayer = multilayer;
	    if ( !multilayer )
		SFSplinesFromLayers(sf);
	    else
		SFReinstanciateRefs(sf);
	    SFLayerChange(sf);
	}
#endif
	TTF_PSDupsChanged(gw,sf,d->names_set ? d->names : sf->names);
	GDrawSetCursor(gw,ct_watch);
	namechange = SetFontName(gw,sf);
	if ( namechange ) retitle_fv = true;
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_XUID));
	xuidchanged = (sf->xuid==NULL && *txt!='\0') ||
			(sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0);
	if ( namechange && sf->filename!=NULL &&
		((uniqueid!=0 && uniqueid==sf->uniqueid) ||
		 (sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0)) ) {
	    static int buts[] = { _STR_Change, _STR_Retain, _STR_Cancel, 0 };
	    int ans = GWidgetAskR(_STR_UniqueIDTitle,buts,0,2,_STR_UniqueIDChange);
	    if ( ans==2 ) {
		GDrawSetCursor(gw,ct_pointer);
return(true);
	    }
	    if ( ans==0 ) {
		if ( uniqueid!=0 && uniqueid==sf->uniqueid )
		    uniqueid = 4000000 + (rand()&0x3ffff);
		if ( sf->xuid!=NULL && uc_strcmp(txt,sf->xuid)==0 ) {
		    SFRandomChangeXUID(sf);
		    xuidchanged = true;
		}
	    }
	} else {
	    free(sf->xuid);
	    sf->xuid = *txt=='\0'?NULL:cu_copy(txt);
	}

	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Notice));
	free(sf->copyright); sf->copyright = cu_copy(txt);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Comment));
	free(sf->comments); sf->comments = cu_copy(*txt?txt:NULL);
	if ( sf->subfontcnt!=0 )
	    sf->cidversion = cidversion;
	else {
	    txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Version));
	    free(sf->version); sf->version = cu_copy(txt);
	}
	if ( as+des != sf->ascent+sf->descent && GGadgetIsChecked(GWidgetGetControl(gw,CID_Scale)) ) {
	    SFScaleToEm(sf,as,des);
	    reformat_fv = true;
	    BDFsSetAsDs(sf);
	} else if ( as!=sf->ascent || des!=sf->descent ) {
	    sf->ascent = as;
	    sf->descent = des;
	    BDFsSetAsDs(sf);
	    reformat_fv = true;
	    if ( sf->pfminfo.os2_typoascent!=0 ) {
		double scale = (as+des)/(double) (sf->ascent+sf->descent);
		sf->pfminfo.os2_typoascent = as;
		sf->pfminfo.os2_typodescent = -des;
		sf->pfminfo.os2_winascent *= scale;
		sf->pfminfo.os2_windescent *= scale;
	    }
	}
	fond = _GGadgetGetTitle(GWidgetGetControl(gw,CID_MacFOND));
	free(sf->fondname); sf->fondname = NULL;
	if ( *fond )
	    sf->fondname = cu_copy(fond);
	sf->macstyle = mcs;
	sf->italicangle = ia;
	sf->upos = upos;
	sf->uwidth = uwid;
	sf->uniqueid = uniqueid;
	sf->texdata = d->texdata;

	GFI_ProcessAnchor(d);
	GFI_ProcessContexts(d);

	interp = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_Interpretation));
	if ( interp==-1 ) sf->uni_interp = ui_none;
	else sf->uni_interp = (intpt) interpretations[interp].userdata;

	enc = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_Encoding));
	if ( enc!=-1 ) {
	    enc = (int) (GGadgetGetListItem(GWidgetGetControl(gw,CID_Encoding),enc)->userdata);
	    if ( enc==em_unicodeplanes )
		enc += d->uplane;
	    GDrawSetCursor(gw,ct_watch);
	    GDrawSetCursor(GGadgetGetWindow(GWidgetGetControl(gw,CID_Encoding)),ct_watch);
	    GDrawSync(NULL);
	    if ( force_enc )
		enc_changed = SFForceEncoding(sf,enc);
	    else
		enc_changed = SFReencodeFont(sf,enc);
	    if ( enc_changed && nchar==oldcnt )
		nchar = sf->charcnt;
	    if ( enc_changed ) reformat_fv = retitle_fv = true;
	}
	if ( nchar!=sf->charcnt )
	    reformat_fv |= SFAddDelChars(sf,nchar);

	if ( sf->hasvmetrics!=vmetrics )
	    CVPaletteDeactivate();		/* Force a refresh later */
	_sf = sf->cidmaster?sf->cidmaster:sf;
	_sf->hasvmetrics = vmetrics;
	for ( j=0; j<_sf->subfontcnt; ++j )
	    _sf->subfonts[j]->hasvmetrics = vmetrics;
	if ( vmetrics ) {
	    _sf->vertical_origin = vorigin;
	    for ( j=0; j<_sf->subfontcnt; ++j )
		_sf->subfonts[j]->vertical_origin = vorigin;
	}

	if ( d->private!=NULL ) {
	    PSDictFree(sf->private);
	    sf->private = d->private;
	    d->private = NULL;
	}
	if ( d->names_set ) {
	    TTFLangNamesFree(sf->names);
	    sf->names = d->names;
	    TTF_PSDupsDefault(sf);
	    d->names = NULL;
	}
	if ( d->ttf_set ) {
	    sf->pfminfo.weight = weight;
	    sf->pfminfo.width = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_WidthClass))+1;
	    pfmfam = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PFMFamily));
	    if ( pfmfam!=NULL )
		sf->pfminfo.pfmfamily = (int) (pfmfam->userdata);
	    else
		sf->pfminfo.pfmfamily = 0x11;
	    fstype = GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_FSType));
	    if ( fstype!=NULL )
		sf->pfminfo.fstype = (int) (fstype->userdata);
	    else
		sf->pfminfo.fstype = 0xc;
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_NoSubsetting)))
		sf->pfminfo.fstype |=0x100;
	    if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_OnlyBitmaps)))
		sf->pfminfo.fstype |=0x200;
	    for ( i=0; i<10; ++i )
		sf->pfminfo.panose[i] = (int) (GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PanFamily+i))->userdata);
	    sf->pfminfo.linegap = linegap;
	    if ( vmetrics )
		sf->pfminfo.vlinegap = vlinegap;
	    sf->pfminfo.os2_winascent = winascent;
	    sf->pfminfo.os2_windescent = windescent;
	    sf->pfminfo.winascent_add = winaoff;
	    sf->pfminfo.windescent_add = windoff;
	    sf->pfminfo.pfmset = true;
	}
	if ( order2!=sf->order2 ) {
	    if ( order2 )
		SFConvertToOrder2(sf);
	    else
		SFConvertToOrder3(sf);
	}
	if ( retitle_fv ) { FontView *fvs;
	    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame )
		FVSetTitle(fvs);
	}
	if ( reformat_fv )
	    FontViewReformatAll(sf);
	sf->changed = true;
	sf->changed_since_autosave = true;
	sf->changed_since_xuidchanged = !xuidchanged;
	/* Just in case they changed the blue values and we are showing blues */
	/*  in outline views... */
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	    CharView *cv;
	    for ( cv = sf->chars[i]->views; cv!=NULL; cv=cv->next ) {
		cv->back_img_out_of_date = true;
		GDrawRequestExpose(cv->v,NULL,false);
	    }
	}
	MacFeatListFree(sf->features);
	sf->features = GGadgetGetUserData(GWidgetGetControl(d->gw,CID_Features));
	last_aspect = d->old_aspect;
	GFI_Close(d);
    }
return( true );
}

GTextInfo *EncodingTypesFindEnc(GTextInfo *encodingtypes, int enc) {
    int i;

    for ( i=0; encodingtypes[i].text!=NULL || encodingtypes[i].line; ++i ) {
	if ( encodingtypes[i].text==NULL )
	    ;
	else if ( encodingtypes[i].userdata == (void *) enc )
return( &encodingtypes[i] );
    }
return( NULL );
}

GTextInfo *GetEncodingTypes(void) {
    GTextInfo *ti;
    int i;
    Encoding *item;

    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ++i;
    if ( i==0 )
return( encodingtypes );
    i += sizeof(encodingtypes)/sizeof(encodingtypes[0]);
    ti = gcalloc(i+1,sizeof(GTextInfo));
    memcpy(ti,encodingtypes,sizeof(encodingtypes)-sizeof(encodingtypes[0]));
    for ( i=0; i<sizeof(encodingtypes)/sizeof(encodingtypes[0])-1; ++i ) {
	if ( ti[i].text_is_1byte ) {
	    ti[i].text = uc_copy((char *) ti[i].text);
	    ti[i].text_is_1byte = false;
	} else if ( ti[i].text_in_resource ) {
	    ti[i].text = u_copy(GStringGetResource( (int) ti[i].text,NULL));
	    ti[i].text_in_resource = false;
	} else
	    ti[i].text = u_copy(ti[i].text);
    }
    ti[i++].line = true;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin ) {
	    ti[i].text = uc_copy(item->enc_name);
	    ti[i++].userdata = (void *) item->enc_num;
	}
return( ti );
}

static void GFI_AsDsLab(struct gfi_data *d, int cid) {
    int isoffset = GGadgetIsChecked(GWidgetGetControl(d->gw,cid));
    DBounds b;
    int ocid = cid==CID_WinAscentIsOff ? CID_WinAscent : CID_WinDescent;
    double val;
    char buf[40];
    unichar_t ubuf[40];

    if ( cid==CID_WinAscentIsOff )
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WinAscentLab),
		GStringGetResource(isoffset?_STR_WinAscentOff:_STR_WinAscent,NULL));
    else
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WinDescentLab),
		GStringGetResource(isoffset?_STR_WinDescentOff:_STR_WinDescent,NULL));
    CIDFindBounds(d->sf,&b);
    b.miny = -b.miny;

    val = u_strtod(_GGadgetGetTitle(GWidgetGetControl(d->gw,ocid)),NULL);
    if ( isoffset )
	sprintf( buf,"%g",rint( val-(cid==CID_WinAscentIsOff ? b.maxy : -b.miny)) );
    else
	sprintf( buf,"%g",rint( val+(cid==CID_WinAscentIsOff ? b.maxy : -b.miny)) );
    uc_strcpy(ubuf,buf);
    GGadgetSetTitle(GWidgetGetControl(d->gw,ocid),ubuf);
}

static int GFI_AsDesIsOff(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	GFI_AsDsLab(d,GGadgetGetCid(g));
    }
return( true );
}

static void TTFSetup(struct gfi_data *d) {
    struct pfminfo info;
    char buffer[10]; unichar_t ubuf[10];
    int i, lg, vlg;

    info = d->sf->pfminfo;
    if ( !info.pfmset ) {
	/* Base this stuff on the CURRENT name */
	/* if the user just created a font, and named it *Bold, then the sf */
	/*  won't yet have Bold in its name, and basing the weight on it would*/
	/*  give the wrong answer. That's why we don't do this init until we */
	/*  get to one of the ttf aspects, it gives the user time to set the */
	/*  name properly */
	/* And on CURRENT values of ascent and descent */
	char *n = cu_copy(_GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Fontname)));
	const unichar_t *as = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Ascent));
	const unichar_t *ds = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Descent));
	unichar_t *aend, *dend;
	double av=u_strtod(as,&aend),dv=u_strtod(ds,&dend);
	if ( *aend=='\0' && *dend=='\0' ) {
	    if ( info.linegap==0 )
		info.linegap = rint(.09*(av+dv));
	    if ( info.vlinegap==0 )
		info.vlinegap = info.linegap;
	}
	lg = info.linegap; vlg = info.vlinegap;
	SFDefaultOS2Info(&info,d->sf,n);
	if ( lg != 0 )
	    info.linegap = lg;
	if ( vlg!= 0 )
	    info.vlinegap = vlg;
	free(n);
    }

    if ( info.weight>0 && info.weight<=900 && info.weight%100==0 )
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WeightClass),
		GStringGetResource((int) (weightclass[info.weight/100-1].text),NULL));
    else {
	sprintf( buffer, "%d", info.weight );
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WeightClass),ubuf);
    }
    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_WidthClass),info.width-1);
    for ( i=0; pfmfamily[i].text!=NULL; ++i )
	if ( (int) (pfmfamily[i].userdata)==info.pfmfamily ) {
	    GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_PFMFamily),i);
    break;
	}
    for ( i=0; i<10; ++i )
	GGadgetSelectOneListItem(GWidgetGetControl(d->gw,CID_PanFamily+i),info.panose[i]);
    d->ttf_set = true;
    /* FSType is already set */
    sprintf( buffer, "%d", info.linegap );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_LineGap),ubuf);
    sprintf( buffer, "%d", info.vlinegap );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_VLineGap),ubuf);

    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_WinAscentIsOff),info.winascent_add);
    GFI_AsDsLab(d,CID_WinAscentIsOff);
    sprintf( buffer, "%d", info.os2_winascent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WinAscent),ubuf);
    GGadgetSetChecked(GWidgetGetControl(d->gw,CID_WinDescentIsOff),info.windescent_add);
    GFI_AsDsLab(d,CID_WinDescentIsOff);
    sprintf( buffer, "%d", info.os2_windescent );
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_WinDescent),ubuf);
}

static int mathparams[] = { _STR_Num1, _STR_Num2,  _STR_Num3, _STR_Denom1,
    _STR_Denom2, _STR_Sup1, _STR_Sup2, _STR_Sup3, _STR_Sub1, _STR_Sub2,
    _STR_SupDrop, _STR_SubDrop, _STR_Delim1, _STR_Delim2, _STR_AxisHt,
    0 };
static int mathpopups[] = { _STR_Num1Popup, _STR_Num2Popup,  _STR_Num3Popup, _STR_Denom1Popup,
    _STR_Denom2Popup, _STR_Sup1Popup, _STR_Sup2Popup, _STR_Sup3Popup, _STR_Sub1Popup, _STR_Sub2Popup,
    _STR_SupDropPopup, _STR_SubDropPopup, _STR_Delim1Popup, _STR_Delim2Popup, _STR_AxisHtPopup,
    0 };
static int extparams[] = { _STR_DefRuleThick, _STR_BigOpSpace1,  _STR_BigOpSpace2, _STR_BigOpSpace3, _STR_BigOpSpace4, _STR_BigOpSpace5, 0 };
static int extpopups[] = { _STR_DefRuleThickPopup, _STR_BigOpSpace1Popup,  _STR_BigOpSpace2Popup, _STR_BigOpSpace3Popup, _STR_BigOpSpace4Popup, _STR_BigOpSpace5Popup, 0 };

static int mp_e_h(GWindow gw, GEvent *event) {
    int i;

    if ( event->type==et_close ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	d->mpdone = true;
    } else if ( event->type == et_char ) {
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	if ( GGadgetGetCid(event->u.control.g)) {
	    int err=false;
	    double em = (d->sf->ascent+d->sf->descent), val;
	    int *params;
	    if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMath)) )
		params = mathparams;
	    else
		params = extparams;
	    for ( i=0; params[i]!=0 && !err; ++i ) {
		val = GetRealR(gw,CID_TeX+i,params[i],&err);
		if ( !err )
		    d->texdata.params[i+7] = rint( val/em * (1<<20) );
	    }
	    if ( !err )
		d->mpdone = true;
	} else
	    d->mpdone = true;
    }
return( true );
}

static int GFI_MoreParams(GGadget *g, GEvent *e) {
    int tot;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData txgcd[35];
    GTextInfo txlabel[35];
    int i,y,k;
    int *params, *popups;
    char values[20][20];

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXText)) )
return( true );
	else if ( GGadgetIsChecked(GWidgetGetControl(d->gw,CID_TeXMath)) ) {
	    tot = 22-7;
	    params = mathparams;
	    popups = mathpopups;
	} else {
	    tot = 13-7;
	    params = extparams;
	    popups = extpopups;
	}

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.is_dlg = true;
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource(_STR_MoreParams,NULL);
	pos.x = pos.y = 0;
	pos.width =GDrawPointsToPixels(NULL,GGadgetScale(180));
	pos.height = GDrawPointsToPixels(NULL,tot*26+60);
	gw = GDrawCreateTopWindow(NULL,&pos,mp_e_h,d,&wattrs);

	memset(&txlabel,0,sizeof(txlabel));
	memset(&txgcd,0,sizeof(txgcd));

	k=0; y = 10;
	for ( i=0; params[i]!=0; ++i ) {
	    txlabel[k].text = (unichar_t *) params[i];
	    txlabel[k].text_in_resource = true;
	    txgcd[k].gd.label = &txlabel[k];
	    txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = y+4;
	    txgcd[k].gd.flags = gg_visible | gg_enabled;
	    txgcd[k].gd.popup_msg = GStringGetResource(popups[i],NULL);
	    txgcd[k++].creator = GLabelCreate;

	    sprintf( values[i], "%g", d->texdata.params[i+7]*(double) (d->sf->ascent+d->sf->descent)/(double) (1<<20));
	    txlabel[k].text = (unichar_t *) values[i];
	    txlabel[k].text_is_1byte = true;
	    txgcd[k].gd.label = &txlabel[k];
	    txgcd[k].gd.pos.x = 85; txgcd[k].gd.pos.y = y;
	    txgcd[k].gd.pos.width = 75;
	    txgcd[k].gd.flags = gg_visible | gg_enabled;
	    txgcd[k].gd.cid = CID_TeX + i;
	    txgcd[k++].creator = GTextFieldCreate;
	    y += 26;
	}

	txgcd[k].gd.pos.x = 30-3; txgcd[k].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
	txgcd[k].gd.pos.width = -1; txgcd[k].gd.pos.height = 0;
	txgcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
	txlabel[k].text = (unichar_t *) _STR_OK;
	txlabel[k].text_in_resource = true;
	txgcd[k].gd.label = &txlabel[k];
	txgcd[k].gd.cid = true;
	txgcd[k++].creator = GButtonCreate;

	txgcd[k].gd.pos.x = -30; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y+3;
	txgcd[k].gd.pos.width = -1; txgcd[k].gd.pos.height = 0;
	txgcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	txlabel[k].text = (unichar_t *) _STR_Cancel;
	txlabel[k].text_in_resource = true;
	txgcd[k].gd.label = &txlabel[k];
	txgcd[k].gd.cid = false;
	txgcd[k++].creator = GButtonCreate;

	txgcd[k].gd.pos.x = 2; txgcd[k].gd.pos.y = 2;
	txgcd[k].gd.pos.width = pos.width-4; txgcd[k].gd.pos.height = pos.height-4;
	txgcd[k].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	txgcd[k].creator = GGroupCreate;

	GGadgetsCreate(gw,txgcd);
	d->mpdone = false;
	GDrawSetVisible(gw,true);

	while ( !d->mpdone )
	    GDrawProcessOneEvent(NULL);
	GDrawDestroyWindow(gw);
    }
return( true );
}

static int GFI_TeXChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	if ( GGadgetGetCid(g)==CID_TeXText ) {
	    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TeXExtraSpLabel),
		    GStringGetResource(_STR_ExtraSp,NULL));
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),false);
	} else {
	    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TeXExtraSpLabel),
		    GStringGetResource(_STR_MathSp,NULL));
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),true);
	}
    }
return( true );
}

static void DefaultTeX(struct gfi_data *d) {
    char buffer[20];
    unichar_t ubuf[20];
    int i;
    SplineFont *sf = d->sf;

    d->tex_set = true;

    if ( sf->texdata.type==tex_unset ) {
	TeXDefaultParams(sf);
	d->texdata = sf->texdata;
    }

    sprintf( buffer,"%g", d->texdata.designsize/(double) (1<<20));
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(d->gw,CID_DesignSize),ubuf);

    for ( i=0; i<7; ++i ) {
	sprintf( buffer,"%g", d->texdata.params[i]*(sf->ascent+sf->descent)/(double) (1<<20));
	uc_strcpy(ubuf,buffer);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TeX+i),ubuf);
    }
    if ( sf->texdata.type==tex_math )
	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TeXMath), true);
    else if ( sf->texdata.type == tex_mathext )
	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TeXMathExt), true);
    else {
	GGadgetSetChecked(GWidgetGetControl(d->gw,CID_TeXText), true);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_TeXExtraSpLabel),
		GStringGetResource(_STR_ExtraSp,NULL));
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MoreParams),false);
    }
}

static int GFI_MacAutomatic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int autom = GGadgetIsChecked(GWidgetGetControl(d->gw,CID_MacAutomatic));
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_MacStyles),!autom);
    }
return( true );
}

static int GFI_AspectChange(GGadget *g, GEvent *e) {
    if ( e==NULL || (e->type==et_controlevent && e->u.control.subtype == et_radiochanged )) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int new_aspect = GTabSetGetSel(g);
	if ( !d->ttf_set && ( new_aspect == d->ttfv_aspect || new_aspect == d->panose_aspect ))
	    TTFSetup(d);
	else if ( !d->names_set && new_aspect == d->tn_aspect )
	    DefaultLanguage(d);
	else if ( !d->tex_set && new_aspect == d->tx_aspect )
	    DefaultTeX(d);
	d->old_aspect = new_aspect;
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	GFI_CancelClose(d);
    } else if ( event->type==et_destroy ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	free(d);
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("fontinfo.html");
return( true );
	} else if ( event->u.chr.keysym=='s' &&
		(event->u.chr.state&ksm_control) &&
		(event->u.chr.state&ksm_meta) ) {
	    MenuSaveAll(NULL,NULL,NULL);
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift ) {
		GFI_CancelClose(GDrawGetUserData(gw));
	    } else
		MenuExit(NULL,NULL,NULL);
return( true );
	}
return( false );
    }
return( true );
}

static int OrderGSUB(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	OrderTable(d->sf,CHR('G','S','U','B'));
    }
return( true );
}

void FontInfo(SplineFont *sf,int defaspect,int sync) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GTabInfo aspects[15], conaspects[7], smaspects[5];
    GGadgetCreateData mgcd[10], ngcd[13], egcd[14], psgcd[24], tngcd[7],
	pgcd[8], vgcd[22], pangcd[22], comgcd[3], atgcd[7], txgcd[23],
	congcd[3], csubgcd[fpst_max-pst_contextpos][6], smgcd[3], smsubgcd[4][6],
	mfgcd[8], mcgcd[8];
    GTextInfo mlabel[10], nlabel[13], elabel[14], pslabel[24], tnlabel[7],
	plabel[8], vlabel[22], panlabel[22], comlabel[3], atlabel[7], txlabel[23],
	csublabel[fpst_max-pst_contextpos][6], smsublabel[4][6],
	mflabel[8], mclabel[8], *list;
    struct gfi_data *d;
    char iabuf[20], upbuf[20], uwbuf[20], asbuf[20], dsbuf[20], ncbuf[20],
	    vbuf[20], uibuf[12], regbuf[100], vorig[20], embuf[20];
    int i,k;
    int mcs;
    Encoding *item;
    FontView *fvs;
    unichar_t title[130];
    static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
    FontRequest rq;
    GFont *font;
    static int connames[] = { _STR_ContextPos, _STR_ContextSub, _STR_ChainPos, _STR_ChainSub, _STR_ReverseChainSub, 0 };
    static int contypes[] = { pst_contextpos, pst_contextsub, pst_chainpos, pst_chainsub, pst_reversesub, 0 };
    static int smnames[] = { _STR_Indic, _STR_ContextSub, _STR_ContextIns, _STR_Kerning, 0 };
    static int smtypes[] = { asm_indic, asm_context, asm_insert, asm_kern };

    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	if ( fvs->fontinfo ) {
	    GDrawSetVisible(((struct gfi_data *) (fvs->fontinfo))->gw,true);
	    GDrawRaise( ((struct gfi_data *) (fvs->fontinfo))->gw );
return;
	}
    }
    if ( defaspect==-1 )
	defaspect = last_aspect;

    d = gcalloc(1,sizeof(struct gfi_data));
    if ( sf->fv!=NULL )
	sf->fv->fontinfo = d;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.is_dlg = true;
    if ( sync ) {
	wattrs.mask |= wam_restrict;
	wattrs.restrict_input_to_me = 1;
    }
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    u_snprintf(title,sizeof(title)/sizeof(title[0]),GStringGetResource(_STR_Fontinformation,NULL), sf->fontname);
    wattrs.window_title = title;
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,GGadgetScale(268));
    pos.height = GDrawPointsToPixels(NULL,375);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,d,&wattrs);

    d->sf = sf;
    d->gw = gw;
    d->old_sel = -2;
    d->uplane = lastplane;
    d->oldcnt = sf->charcnt;
    d->texdata = sf->texdata;

    memset(&nlabel,0,sizeof(nlabel));
    memset(&ngcd,0,sizeof(ngcd));

    nlabel[0].text = (unichar_t *) _STR_Fontname;
    nlabel[0].text_in_resource = true;
    ngcd[0].gd.label = &nlabel[0];
    ngcd[0].gd.mnemonic = 'F';
    ngcd[0].gd.pos.x = 12; ngcd[0].gd.pos.y = 6+6; 
    ngcd[0].gd.flags = gg_visible | gg_enabled;
    ngcd[0].creator = GLabelCreate;

    ngcd[1].gd.pos.x = 115; ngcd[1].gd.pos.y = ngcd[0].gd.pos.y-6; ngcd[1].gd.pos.width = 137;
    ngcd[1].gd.flags = gg_visible | gg_enabled;
    nlabel[1].text = (unichar_t *) sf->fontname;
    nlabel[1].text_is_1byte = true;
    ngcd[1].gd.label = &nlabel[1];
    ngcd[1].gd.cid = CID_Fontname;
    ngcd[1].gd.handle_controlevent = GFI_NameChange;
    ngcd[1].creator = GTextFieldCreate;

    nlabel[10].text = (unichar_t *) _STR_Familyname;
    nlabel[10].text_in_resource = true;
    ngcd[10].gd.label = &nlabel[10];
    ngcd[10].gd.mnemonic = 'F';
    ngcd[10].gd.pos.x = 12; ngcd[10].gd.pos.y = ngcd[0].gd.pos.y+26; 
    ngcd[10].gd.flags = gg_visible | gg_enabled;
    ngcd[10].creator = GLabelCreate;

    ngcd[11].gd.pos.x = ngcd[1].gd.pos.x; ngcd[11].gd.pos.y = ngcd[10].gd.pos.y-6; ngcd[11].gd.pos.width = 137;
    ngcd[11].gd.flags = gg_visible | gg_enabled;
    nlabel[11].text = (unichar_t *) (sf->familyname?sf->familyname:sf->fontname);
    nlabel[11].text_is_1byte = true;
    ngcd[11].gd.label = &nlabel[11];
    ngcd[11].gd.cid = CID_Family;
    ngcd[11].gd.handle_controlevent = GFI_FamilyChange;
    ngcd[11].creator = GTextFieldCreate;
    if ( sf->familyname==NULL || strstr(sf->familyname,"Untitled")==sf->familyname )
	d->family_untitled = true;

    ngcd[4].gd.pos.x = 12; ngcd[4].gd.pos.y = ngcd[11].gd.pos.y+26+6;
    nlabel[4].text = (unichar_t *) _STR_Humanname;
    nlabel[4].text_in_resource = true;
    ngcd[4].gd.label = &nlabel[4];
    ngcd[4].gd.mnemonic = 'H';
    ngcd[4].gd.flags = gg_visible | gg_enabled;
    ngcd[4].creator = GLabelCreate;

    ngcd[5].gd.pos.x = 115; ngcd[5].gd.pos.y = ngcd[4].gd.pos.y-6; ngcd[5].gd.pos.width = 137;
    ngcd[5].gd.flags = gg_visible | gg_enabled;
    nlabel[5].text = (unichar_t *) (sf->fullname?sf->fullname:sf->fontname);
    nlabel[5].text_is_1byte = true;
    ngcd[5].gd.label = &nlabel[5];
    ngcd[5].gd.cid = CID_Human;
    ngcd[5].gd.handle_controlevent = GFI_HumanChange;
    ngcd[5].creator = GTextFieldCreate;
    if ( sf->fullname==NULL || strstr(sf->fullname,"Untitled")==sf->fullname )
	d->human_untitled = true;

    nlabel[2].text = (unichar_t *) _STR_Weight;
    nlabel[2].text_in_resource = true;
    ngcd[2].gd.label = &nlabel[2];
    ngcd[2].gd.mnemonic = 'W';
    ngcd[2].gd.pos.x = ngcd[10].gd.pos.x; ngcd[2].gd.pos.y = ngcd[4].gd.pos.y+26; 
    ngcd[2].gd.flags = gg_visible | gg_enabled;
    ngcd[2].creator = GLabelCreate;

    ngcd[3].gd.pos.x = ngcd[1].gd.pos.x; ngcd[3].gd.pos.y = ngcd[2].gd.pos.y-6; ngcd[3].gd.pos.width = 137;
    ngcd[3].gd.flags = gg_visible | gg_enabled;
    nlabel[3].text = (unichar_t *) (sf->weight?sf->weight:"Regular");
    nlabel[3].text_is_1byte = true;
    ngcd[3].gd.label = &nlabel[3];
    ngcd[3].gd.cid = CID_Weight;
    ngcd[3].creator = GTextFieldCreate;

    ngcd[8].gd.pos.x = 12; ngcd[8].gd.pos.y = ngcd[3].gd.pos.y+30+6;
    nlabel[8].text = (unichar_t *) _STR_VersionC;
    nlabel[8].text_in_resource = true;
    ngcd[8].gd.label = &nlabel[8];
    ngcd[8].gd.mnemonic = 'V';
    ngcd[8].gd.flags = gg_visible | gg_enabled;
    ngcd[8].creator = GLabelCreate;

    ngcd[9].gd.pos.x = 115; ngcd[9].gd.pos.y = ngcd[8].gd.pos.y-6; ngcd[9].gd.pos.width = 137;
    ngcd[9].gd.flags = gg_visible | gg_enabled;
    nlabel[9].text = (unichar_t *) (sf->version?sf->version:"");
    nlabel[9].text_is_1byte = true;
    if ( sf->subfontcnt!=0 ) {
	sprintf( vbuf,"%g", sf->cidversion );
	nlabel[9].text = (unichar_t *) vbuf;
    }
    ngcd[9].gd.label = &nlabel[9];
    ngcd[9].gd.cid = CID_Version;
    ngcd[9].creator = GTextFieldCreate;

    ngcd[6].gd.pos.x = 12; ngcd[6].gd.pos.y = ngcd[8].gd.pos.y+22;
    ngcd[6].gd.flags = gg_visible | gg_enabled;
    ngcd[6].gd.mnemonic = 'r';
    nlabel[6].text = (unichar_t *) _STR_Copyright;
    nlabel[6].text_in_resource = true;
    ngcd[6].gd.label = &nlabel[6];
    ngcd[6].creator = GLabelCreate;

    ngcd[7].gd.pos.x = 12; ngcd[7].gd.pos.y = ngcd[6].gd.pos.y+14;
    ngcd[7].gd.pos.width = ngcd[5].gd.pos.x+ngcd[5].gd.pos.width-26;
    ngcd[7].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    if ( sf->copyright!=NULL ) {
	nlabel[7].text = (unichar_t *) sf->copyright;
	nlabel[7].text_is_1byte = true;
	ngcd[7].gd.label = &nlabel[7];
    }
    ngcd[7].gd.cid = CID_Notice;
    ngcd[7].creator = GTextAreaCreate;
/******************************************************************************/
    memset(&elabel,0,sizeof(elabel));
    memset(&egcd,0,sizeof(egcd));

    egcd[0].gd.pos.x = 12; egcd[0].gd.pos.y = 12;
    egcd[0].gd.flags = gg_visible | gg_enabled;
    egcd[0].gd.mnemonic = 'E';
    elabel[0].text = (unichar_t *) _STR_Encoding;
    elabel[0].text_in_resource = true;
    egcd[0].gd.label = &elabel[0];
    egcd[0].creator = GLabelCreate;

    egcd[1].gd.pos.x = 80; egcd[1].gd.pos.y = egcd[0].gd.pos.y-6;
    egcd[1].gd.flags = gg_visible | gg_enabled;
    egcd[1].gd.u.list = list = GetEncodingTypes();
    egcd[1].gd.label = EncodingTypesFindEnc(list,sf->compacted?em_compacted:sf->encoding_name);
    if ( egcd[1].gd.label==NULL ) egcd[1].gd.label = &list[0];
    egcd[1].gd.cid = CID_Encoding;
    egcd[1].gd.handle_controlevent = GFI_SelectEncoding;
    egcd[1].creator = GListButtonCreate;
    for ( i=0; list[i].text!=NULL || list[i].line; ++i ) {
	if ( sf->encoding_name>=em_unicodeplanes && sf->encoding_name<=em_unicodeplanesmax &&
		(void *) em_unicodeplanes==list[i].userdata ) {
	    list[i].selected = true;
	    d->uplane = sf->encoding_name-em_unicodeplanes;
	} else if ( (void *) (sf->encoding_name)==list[i].userdata &&
		list[i].text!=NULL )
	    list[i].selected = true;
	else
	    list[i].selected = false;
    }

    egcd[2].gd.pos.x = 8; egcd[2].gd.pos.y = GDrawPointsToPixels(NULL,egcd[0].gd.pos.y+6);
    egcd[2].gd.pos.width = pos.width-32; egcd[2].gd.pos.height = GDrawPointsToPixels(NULL,43);
    egcd[2].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    egcd[2].creator = GGroupCreate;

    egcd[3].gd.pos.x = 12; egcd[3].gd.pos.y = egcd[1].gd.pos.y+27;
    egcd[3].gd.pos.width = -1; egcd[3].gd.pos.height = 0;
    egcd[3].gd.flags = gg_visible | gg_enabled;
    elabel[3].text = (unichar_t *) _STR_Load;
    elabel[3].text_in_resource = true;
    egcd[3].gd.mnemonic = 'L';
    egcd[3].gd.label = &elabel[3];
    egcd[3].gd.handle_controlevent = GFI_Load;
    egcd[3].creator = GButtonCreate;

    egcd[4].gd.pos.x = (254-100)/2; egcd[4].gd.pos.y = egcd[3].gd.pos.y;
    egcd[4].gd.pos.width = 100; egcd[4].gd.pos.height = 0;
    egcd[4].gd.flags = gg_visible;
    if ( sf->encoding_name==em_custom || sf->charcnt>1500 ) egcd[4].gd.flags |= gg_enabled;
    elabel[4].text = (unichar_t *) _STR_Makefromfont;
    elabel[4].text_in_resource = true;
    egcd[4].gd.mnemonic = 'k';
    egcd[4].gd.label = &elabel[4];
    egcd[4].gd.handle_controlevent = GFI_Make;
    egcd[4].gd.cid = CID_Make;
    egcd[4].creator = GButtonCreate;

    egcd[5].gd.pos.x = -12;
    egcd[5].gd.pos.y = egcd[4].gd.pos.y;
    egcd[5].gd.pos.width = -1; egcd[5].gd.pos.height = 0;
    egcd[5].gd.flags = gg_visible;
    for ( item=enclist; item!=NULL && item->builtin; item=item->next );
    if ( item!=NULL ) egcd[5].gd.flags |= gg_enabled;
    elabel[5].text = (unichar_t *) _STR_Remove;
    elabel[5].text_in_resource = true;
    egcd[5].gd.mnemonic = 'R';
    egcd[5].gd.label = &elabel[5];
    egcd[5].gd.handle_controlevent = GFI_Delete;
    egcd[5].gd.cid = CID_Delete;
    egcd[5].creator = GButtonCreate;

    egcd[6].gd.pos.x = 12; egcd[6].gd.pos.y = egcd[5].gd.pos.y+36+6;
    egcd[6].gd.flags = gg_visible | gg_enabled;
    egcd[6].gd.mnemonic = 'F';
    elabel[6].text = (unichar_t *) _STR_ForceEncoding;
    elabel[6].text_in_resource = true;
    egcd[6].gd.label = &elabel[6];
    egcd[6].gd.popup_msg = GStringGetResource(_STR_ForceEncodingPopup,NULL);
    egcd[6].gd.cid = CID_ForceEncoding;
    egcd[6].creator = GCheckBoxCreate;

    egcd[7].gd.pos.x = 12; egcd[7].gd.pos.y = egcd[6].gd.pos.y+24+6;
    egcd[7].gd.flags = gg_visible | gg_enabled;
    egcd[7].gd.mnemonic = 'N';
    elabel[7].text = (unichar_t *) _STR_Numchars;
    elabel[7].text_in_resource = true;
    egcd[7].gd.label = &elabel[7];
    egcd[7].creator = GLabelCreate;

    egcd[8].gd.pos.x = 123; egcd[8].gd.pos.y = egcd[7].gd.pos.y-6; egcd[8].gd.pos.width = 60;
    egcd[8].gd.flags = gg_visible | gg_enabled;
    sprintf( ncbuf, "%d", sf->charcnt );
    elabel[8].text = (unichar_t *) ncbuf;
    elabel[8].text_is_1byte = true;
    egcd[8].gd.label = &elabel[8];
    egcd[8].gd.cid = CID_NChars;
    egcd[8].creator = GTextFieldCreate;

    egcd[9].gd.pos.x = 12; egcd[9].gd.pos.y = egcd[7].gd.pos.y+24;
    egcd[9].gd.flags = gg_visible | gg_enabled;
    elabel[9].text = (unichar_t *) _STR_Interpretation;
    elabel[9].text_in_resource = true;
    egcd[9].gd.label = &elabel[9];
    egcd[9].creator = GLabelCreate;

    egcd[10].gd.pos.x = egcd[1].gd.pos.x; egcd[10].gd.pos.y = egcd[9].gd.pos.y-6;
    egcd[10].gd.flags = gg_visible | gg_enabled;
    egcd[10].gd.u.list = interpretations;
    egcd[10].gd.cid = CID_Interpretation;
    egcd[10].gd.handle_controlevent = GFI_SelectEncoding;
    egcd[10].creator = GListButtonCreate;
    for ( i=0; interpretations[i].text!=NULL || interpretations[i].line; ++i ) {
	if ( (void *) (sf->uni_interp)==interpretations[i].userdata &&
		interpretations[i].text!=NULL ) {
	    interpretations[i].selected = true;
	    egcd[10].gd.label = &interpretations[i];
	} else
	    interpretations[i].selected = false;
    }

    if ( sf->cidmaster || sf->subfontcnt!=0 ) {
	SplineFont *master = sf->cidmaster?sf->cidmaster:sf;

	egcd[11].gd.pos.x = 12; egcd[11].gd.pos.y = egcd[10].gd.pos.y+36+6;
	egcd[11].gd.flags = gg_visible ;
	egcd[11].gd.mnemonic = 'N';
	elabel[11].text = (unichar_t *) _STR_CIDRegistry;
	elabel[11].text_in_resource = true;
	egcd[11].gd.label = &elabel[11];
	egcd[11].creator = GLabelCreate;

	egcd[12].gd.pos.x = egcd[1].gd.pos.x; egcd[12].gd.pos.y = egcd[11].gd.pos.y-6;
	egcd[12].gd.pos.width = 140;
	egcd[12].gd.flags = gg_visible ;
	sprintf( regbuf, "%.30s-%.30s-%d", master->cidregistry, master->ordering, master->supplement );
	elabel[12].text = (unichar_t *) regbuf;
	elabel[12].text_is_1byte = true;
	egcd[12].gd.label = &elabel[12];
	egcd[12].creator = GTextFieldCreate;
    }

    if ( sf->subfontcnt!=0 || sf->cidmaster!=NULL ) {
	for ( i=0; i<=5; ++i )
	    egcd[i].gd.flags &= ~gg_enabled;
	if ( sf->subfontcnt!=0 ) {
	    egcd[6].gd.flags &= ~gg_enabled;
	    egcd[7].gd.flags &= ~gg_enabled;
	}
    }
/******************************************************************************/
    memset(&pslabel,0,sizeof(pslabel));
    memset(&psgcd,0,sizeof(psgcd));

    psgcd[0].gd.pos.x = 12; psgcd[0].gd.pos.y = 12;
    psgcd[0].gd.flags = gg_visible | gg_enabled;
    psgcd[0].gd.mnemonic = 'A';
    pslabel[0].text = (unichar_t *) _STR_Ascent;
    pslabel[0].text_in_resource = true;
    psgcd[0].gd.label = &pslabel[0];
    psgcd[0].creator = GLabelCreate;

    psgcd[1].gd.pos.x = 103; psgcd[1].gd.pos.y = psgcd[0].gd.pos.y-6; psgcd[1].gd.pos.width = 47;
    psgcd[1].gd.flags = gg_visible | gg_enabled;
    sprintf( asbuf, "%d", sf->ascent );
    pslabel[1].text = (unichar_t *) asbuf;
    pslabel[1].text_is_1byte = true;
    psgcd[1].gd.label = &pslabel[1];
    psgcd[1].gd.cid = CID_Ascent;
    psgcd[1].gd.handle_controlevent = GFI_EmChanged;
    psgcd[1].creator = GTextFieldCreate;

    psgcd[2].gd.pos.x = 155; psgcd[2].gd.pos.y = psgcd[0].gd.pos.y;
    psgcd[2].gd.flags = gg_visible | gg_enabled;
    psgcd[2].gd.mnemonic = 'D';
    pslabel[2].text = (unichar_t *) _STR_Descent;
    pslabel[2].text_in_resource = true;
    psgcd[2].gd.label = &pslabel[2];
    psgcd[2].creator = GLabelCreate;

    psgcd[3].gd.pos.x = 200; psgcd[3].gd.pos.y = psgcd[1].gd.pos.y; psgcd[3].gd.pos.width = 47;
    psgcd[3].gd.flags = gg_visible | gg_enabled;
    sprintf( dsbuf, "%d", sf->descent );
    pslabel[3].text = (unichar_t *) dsbuf;
    pslabel[3].text_is_1byte = true;
    psgcd[3].gd.label = &pslabel[3];
    psgcd[3].gd.cid = CID_Descent;
    psgcd[3].gd.handle_controlevent = GFI_EmChanged;
    psgcd[3].creator = GTextFieldCreate;

    psgcd[4].gd.pos.x = psgcd[0].gd.pos.x+5; psgcd[4].gd.pos.y = psgcd[0].gd.pos.y+24;
    psgcd[4].gd.flags = gg_visible | gg_enabled;
    psgcd[4].gd.mnemonic = 'E';
    pslabel[4].text = (unichar_t *) _STR_EmSize;
    pslabel[4].text_in_resource = true;
    psgcd[4].gd.label = &pslabel[4];
    psgcd[4].creator = GLabelCreate;

    psgcd[5].gd.pos.x = psgcd[1].gd.pos.x-20; psgcd[5].gd.pos.y = psgcd[1].gd.pos.y+24; psgcd[5].gd.pos.width = 67;
    psgcd[5].gd.flags = gg_visible | gg_enabled;
    sprintf( embuf, "%d", sf->descent+sf->ascent );
    pslabel[5].text = (unichar_t *) embuf;
    pslabel[5].text_is_1byte = true;
    psgcd[5].gd.label = &pslabel[5];
    psgcd[5].gd.cid = CID_Em;
    psgcd[5].gd.u.list = emsizes;
    psgcd[5].gd.handle_controlevent = GFI_EmChanged;
    psgcd[5].creator = GListFieldCreate;

    psgcd[6].gd.pos.x = psgcd[2].gd.pos.x; psgcd[6].gd.pos.y = psgcd[4].gd.pos.y-4;
    psgcd[6].gd.flags = gg_visible | gg_enabled | gg_cb_on;
    pslabel[6].text = (unichar_t *) _STR_ScaleOutlines;
    pslabel[6].text_in_resource = true;
    psgcd[6].gd.label = &pslabel[6];
    psgcd[6].gd.cid = CID_Scale;
    psgcd[6].creator = GCheckBoxCreate;

/* I've reversed the label, text field order in the gcd here because */
/*  that way the text field will be displayed on top of the label rather */
/*  than the reverse, and in Russian that's important translation of */
/*  "Italic Angle" is too long. Oops, no it's the next one, might as well leave in here though */
    psgcd[8].gd.pos.x = 12; psgcd[8].gd.pos.y = psgcd[5].gd.pos.y+26+6;
    psgcd[8].gd.flags = gg_visible | gg_enabled;
    psgcd[8].gd.mnemonic = 'I';
    pslabel[8].text = (unichar_t *) _STR_Italicangle;
    pslabel[8].text_in_resource = true;
    psgcd[8].gd.label = &pslabel[8];
    psgcd[8].creator = GLabelCreate;

    psgcd[7].gd.pos.x = 103; psgcd[7].gd.pos.y = psgcd[8].gd.pos.y-6;
    psgcd[7].gd.pos.width = 47;
    psgcd[7].gd.flags = gg_visible | gg_enabled;
    sprintf( iabuf, "%g", sf->italicangle );
    pslabel[7].text = (unichar_t *) iabuf;
    pslabel[7].text_is_1byte = true;
    psgcd[7].gd.label = &pslabel[7];
    psgcd[7].gd.cid = CID_ItalicAngle;
    psgcd[7].creator = GTextFieldCreate;

    psgcd[9].gd.pos.y = psgcd[7].gd.pos.y;
    psgcd[9].gd.pos.width = -1; psgcd[9].gd.pos.height = 0;
    psgcd[9].gd.pos.x = psgcd[3].gd.pos.x+psgcd[3].gd.pos.width-
	    GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor);
    /*if ( strstrmatch(sf->fontname,"Italic")!=NULL ||strstrmatch(sf->fontname,"Oblique")!=NULL )*/
	psgcd[9].gd.flags = gg_visible | gg_enabled;
    pslabel[9].text = (unichar_t *) _STR_Guess;
    pslabel[9].text_in_resource = true;
    psgcd[9].gd.label = &pslabel[9];
    psgcd[9].gd.mnemonic = 'G';
    psgcd[9].gd.handle_controlevent = GFI_GuessItalic;
    psgcd[9].creator = GButtonCreate;

/* I've reversed the label, text field order in the gcd here because */
/*  that way the text field will be displayed on top of the label rather */
/*  than the reverse, and in Russian that's important translation of */
/*  "Underline position" is too long. */
    psgcd[11].gd.pos.x = 12; psgcd[11].gd.pos.y = psgcd[7].gd.pos.y+26+6;
    psgcd[11].gd.flags = gg_visible | gg_enabled;
    psgcd[11].gd.mnemonic = 'P';
    pslabel[11].text = (unichar_t *) _STR_Upos;
    pslabel[11].text_in_resource = true;
    psgcd[11].gd.label = &pslabel[11];
    psgcd[11].creator = GLabelCreate;

    psgcd[10].gd.pos.x = 103; psgcd[10].gd.pos.y = psgcd[11].gd.pos.y-6; psgcd[10].gd.pos.width = 47;
    psgcd[10].gd.flags = gg_visible | gg_enabled;
    sprintf( upbuf, "%g", sf->upos );
    pslabel[10].text = (unichar_t *) upbuf;
    pslabel[10].text_is_1byte = true;
    psgcd[10].gd.label = &pslabel[10];
    psgcd[10].gd.cid = CID_UPos;
    psgcd[10].creator = GTextFieldCreate;

    psgcd[12].gd.pos.x = 155; psgcd[12].gd.pos.y = psgcd[11].gd.pos.y;
    psgcd[12].gd.flags = gg_visible | gg_enabled;
    psgcd[12].gd.mnemonic = 'H';
    pslabel[12].text = (unichar_t *) _STR_Uheight;
    pslabel[12].text_in_resource = true;
    psgcd[12].gd.label = &pslabel[12];
    psgcd[12].creator = GLabelCreate;

    psgcd[13].gd.pos.x = 200; psgcd[13].gd.pos.y = psgcd[10].gd.pos.y; psgcd[13].gd.pos.width = 47;
    psgcd[13].gd.flags = gg_visible | gg_enabled;
    sprintf( uwbuf, "%g", sf->uwidth );
    pslabel[13].text = (unichar_t *) uwbuf;
    pslabel[13].text_is_1byte = true;
    psgcd[13].gd.label = &pslabel[13];
    psgcd[13].gd.cid = CID_UWidth;
    psgcd[13].creator = GTextFieldCreate;

    psgcd[14].gd.pos.x = 12; psgcd[14].gd.pos.y = psgcd[13].gd.pos.y+26+6;
    psgcd[14].gd.flags = gg_visible | gg_enabled;
    psgcd[14].gd.mnemonic = 'x';
    pslabel[14].text = (unichar_t *) _STR_Xuid;
    pslabel[14].text_in_resource = true;
    psgcd[14].gd.label = &pslabel[14];
    psgcd[14].creator = GLabelCreate;

    psgcd[15].gd.pos.x = 103; psgcd[15].gd.pos.y = psgcd[14].gd.pos.y-6; psgcd[15].gd.pos.width = 142;
    psgcd[15].gd.flags = gg_visible | gg_enabled;
    if ( sf->xuid!=NULL ) {
	pslabel[15].text = (unichar_t *) sf->xuid;
	pslabel[15].text_is_1byte = true;
	psgcd[15].gd.label = &pslabel[15];
    }
    psgcd[15].gd.cid = CID_XUID;
    psgcd[15].creator = GTextFieldCreate;

    psgcd[16].gd.pos.x = 12; psgcd[16].gd.pos.y = psgcd[15].gd.pos.y+26+6;
    pslabel[16].text = (unichar_t *) _STR_UniqueIDC;
    pslabel[16].text_in_resource = true;
    psgcd[16].gd.label = &pslabel[16];
    psgcd[16].gd.mnemonic = 'U';
    psgcd[16].gd.flags = gg_visible | gg_enabled;
    psgcd[16].creator = GLabelCreate;

    psgcd[17].gd.pos.x = psgcd[12].gd.pos.x; psgcd[17].gd.pos.y = psgcd[16].gd.pos.y-6; psgcd[17].gd.pos.width = psgcd[12].gd.pos.width;
    psgcd[17].gd.flags = gg_visible | gg_enabled;
    pslabel[17].text = (unichar_t *) "";
    pslabel[17].text_is_1byte = true;
    if ( sf->uniqueid!=0 ) {
	sprintf( uibuf, "%d", sf->uniqueid );
	pslabel[17].text = (unichar_t *) uibuf;
    }
    psgcd[17].gd.label = &pslabel[17];
    psgcd[17].gd.cid = CID_UniqueID;
    psgcd[17].creator = GTextFieldCreate;

    psgcd[18].gd.pos.x = 12; psgcd[18].gd.pos.y = psgcd[17].gd.pos.y+26+6;
    pslabel[18].text = (unichar_t *) _STR_HasVerticalMetrics;
    pslabel[18].text_in_resource = true;
    psgcd[18].gd.label = &pslabel[18];
    psgcd[18].gd.mnemonic = 'V';
    psgcd[18].gd.cid = CID_HasVerticalMetrics;
    psgcd[18].gd.flags = gg_visible | gg_enabled;
    if ( sf->hasvmetrics )
	psgcd[18].gd.flags |= gg_cb_on;
    psgcd[18].gd.handle_controlevent = GFI_VMetricsCheck;
    psgcd[18].creator = GCheckBoxCreate;

    psgcd[19].gd.pos.x = 12; psgcd[19].gd.pos.y = psgcd[18].gd.pos.y+22+4;
    pslabel[19].text = (unichar_t *) _STR_VOrigin;
    pslabel[19].text_in_resource = true;
    psgcd[19].gd.label = &pslabel[19];
    psgcd[19].gd.mnemonic = 'O';
    psgcd[19].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled) : gg_visible;
    psgcd[19].gd.cid = CID_VOriginLab;
    psgcd[19].creator = GLabelCreate;

    psgcd[20].gd.pos.x = psgcd[15].gd.pos.x; psgcd[20].gd.pos.y = psgcd[19].gd.pos.y-4; psgcd[20].gd.pos.width = psgcd[15].gd.pos.width;
    psgcd[20].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled) : gg_visible;
    pslabel[20].text = (unichar_t *) "";
    pslabel[20].text_is_1byte = true;
    if ( sf->vertical_origin!=0 || sf->hasvmetrics ) {
	sprintf( vorig, "%d", sf->vertical_origin );
	pslabel[20].text = (unichar_t *) vorig;
    }
    psgcd[20].gd.label = &pslabel[20];
    psgcd[20].gd.cid = CID_VOrigin;
    psgcd[20].creator = GTextFieldCreate;

    psgcd[21].gd.pos.x = 12; psgcd[21].gd.pos.y = psgcd[20].gd.pos.y+26;
    pslabel[21].text = (unichar_t *) _STR_Order2Splines;
    pslabel[21].text_in_resource = true;
    psgcd[21].gd.label = &pslabel[21];
    psgcd[21].gd.flags = sf->order2 ? (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    psgcd[21].gd.cid = CID_IsOrder2;
    psgcd[21].creator = GCheckBoxCreate;
    psgcd[21].gd.popup_msg = GStringGetResource(_STR_PopupOrder2Splines,NULL);

#ifdef FONTFORGE_CONFIG_TYPE3
    psgcd[22].gd.pos.x = 12; psgcd[22].gd.pos.y = psgcd[21].gd.pos.y+16;
    pslabel[22].text = (unichar_t *) _STR_MultiLayer;
    pslabel[22].text_in_resource = true;
    psgcd[22].gd.label = &pslabel[22];
    psgcd[22].gd.flags = sf->multilayer ? (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    psgcd[22].gd.cid = CID_IsMultiLayer;
    psgcd[22].creator = GCheckBoxCreate;
    psgcd[22].gd.popup_msg = GStringGetResource(_STR_PopupMultiLayer,NULL);
#endif

    if ( sf->subfontcnt!=0 ) {
	for ( i=0; i<=13; ++i )
	    psgcd[i].gd.flags &= ~gg_enabled;
    } else if ( sf->cidmaster!=NULL ) {
	for ( i=18; i<=20; ++i )
	    psgcd[i].gd.flags &= ~gg_enabled;
    }
/******************************************************************************/

    memset(&plabel,0,sizeof(plabel));
    memset(&pgcd,0,sizeof(pgcd));

    pgcd[0].gd.pos.x = 10; pgcd[0].gd.pos.y = 6;
    pgcd[0].gd.pos.width = 240; pgcd[0].gd.pos.height = 8*12+10;
    pgcd[0].gd.flags = gg_visible | gg_enabled;
    pgcd[0].gd.cid = CID_PrivateEntries;
    pgcd[0].gd.u.list = PI_ListSet(sf);
    pgcd[0].gd.handle_controlevent = PI_ListSel;
    pgcd[0].creator = GListCreate;

    pgcd[1].gd.pos.x = 10; pgcd[1].gd.pos.y = pgcd[0].gd.pos.y+pgcd[0].gd.pos.height+10;
    pgcd[1].gd.pos.width = pgcd[0].gd.pos.width; pgcd[1].gd.pos.height = 8*12+10;
    pgcd[1].gd.flags = gg_visible | gg_enabled;
    pgcd[1].gd.cid = CID_PrivateValues;
    pgcd[1].creator = GTextAreaCreate;

    pgcd[2].gd.pos.x = 10; pgcd[2].gd.pos.y = 300-35-30;
    pgcd[2].gd.pos.width = -1; pgcd[2].gd.pos.height = 0;
    pgcd[2].gd.flags = gg_visible | gg_enabled ;
    plabel[2].text = (unichar_t *) _STR_Add;
    plabel[2].text_in_resource = true;
    pgcd[2].gd.mnemonic = 'A';
    pgcd[2].gd.label = &plabel[2];
    pgcd[2].gd.handle_controlevent = PI_Add;
    pgcd[2].gd.cid = CID_Add;
    pgcd[2].creator = GButtonCreate;

    pgcd[3].gd.pos.x = (260)/2-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)-5;
    pgcd[3].gd.pos.y = pgcd[2].gd.pos.y;
    pgcd[3].gd.pos.width = -1; pgcd[3].gd.pos.height = 0;
    pgcd[3].gd.flags = gg_visible ;
    plabel[3].text = (unichar_t *) _STR_Guess;
    plabel[3].text_in_resource = true;
    pgcd[3].gd.label = &plabel[3];
    pgcd[3].gd.mnemonic = 'G';
    pgcd[3].gd.handle_controlevent = PI_Guess;
    pgcd[3].gd.cid = CID_Guess;
    pgcd[3].creator = GButtonCreate;

    pgcd[4].gd.pos.x = -(260/2-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)-5);
    pgcd[4].gd.pos.y = pgcd[2].gd.pos.y;
    pgcd[4].gd.pos.width = -1; pgcd[4].gd.pos.height = 0;
    pgcd[4].gd.flags = gg_visible ;
    plabel[4].text = (unichar_t *) _STR_Hist;
    plabel[4].text_in_resource = true;
    pgcd[4].gd.label = &plabel[4];
    pgcd[4].gd.mnemonic = 'G';
    pgcd[4].gd.handle_controlevent = PI_Hist;
    pgcd[4].gd.cid = CID_Hist;
    pgcd[4].gd.popup_msg = GStringGetResource(_STR_HistPopup,NULL);
    pgcd[4].creator = GButtonCreate;

    pgcd[5].gd.pos.x = -10; pgcd[5].gd.pos.y = pgcd[2].gd.pos.y;
    pgcd[5].gd.pos.width = -1; pgcd[5].gd.pos.height = 0;
    pgcd[5].gd.flags = gg_visible | gg_enabled ;
    plabel[5].text = (unichar_t *) _STR_Remove;
    plabel[5].text_in_resource = true;
    pgcd[5].gd.label = &plabel[5];
    pgcd[5].gd.mnemonic = 'R';
    pgcd[5].gd.handle_controlevent = PI_Delete;
    pgcd[5].gd.cid = CID_Remove;
    pgcd[5].creator = GButtonCreate;
/******************************************************************************/
    memset(&vlabel,0,sizeof(vlabel));
    memset(&vgcd,0,sizeof(vgcd));

    vgcd[0].gd.pos.x = 10; vgcd[0].gd.pos.y = 12;
    vlabel[0].text = (unichar_t *) _STR_WeightClass;
    vlabel[0].text_in_resource = true;
    vgcd[0].gd.label = &vlabel[0];
    vgcd[0].gd.flags = gg_visible | gg_enabled;
    vgcd[0].creator = GLabelCreate;

    vgcd[1].gd.pos.x = 100; vgcd[1].gd.pos.y = vgcd[0].gd.pos.y-6; vgcd[1].gd.pos.width = 140;
    vgcd[1].gd.flags = gg_visible | gg_enabled;
    vgcd[1].gd.cid = CID_WeightClass;
    vgcd[1].gd.u.list = weightclass;
    vgcd[1].creator = GListFieldCreate;

    vgcd[2].gd.pos.x = 10; vgcd[2].gd.pos.y = vgcd[1].gd.pos.y+26+6;
    vlabel[2].text = (unichar_t *) _STR_WidthClass;
    vlabel[2].text_in_resource = true;
    vgcd[2].gd.label = &vlabel[2];
    vgcd[2].gd.flags = gg_visible | gg_enabled;
    vgcd[2].creator = GLabelCreate;

    vgcd[3].gd.pos.x = 100; vgcd[3].gd.pos.y = vgcd[2].gd.pos.y-6;
    vgcd[3].gd.flags = gg_visible | gg_enabled;
    vgcd[3].gd.cid = CID_WidthClass;
    vgcd[3].gd.u.list = widthclass;
    vgcd[3].creator = GListButtonCreate;

    vgcd[4].gd.pos.x = 10; vgcd[4].gd.pos.y = vgcd[3].gd.pos.y+26+6;
    vlabel[4].text = (unichar_t *) _STR_PFMFamily;
    vlabel[4].text_in_resource = true;
    vgcd[4].gd.label = &vlabel[4];
    vgcd[4].gd.flags = gg_visible | gg_enabled;
    vgcd[4].creator = GLabelCreate;

    vgcd[5].gd.pos.x = 100; vgcd[5].gd.pos.y = vgcd[4].gd.pos.y-6; vgcd[5].gd.pos.width = 140;
    vgcd[5].gd.flags = gg_visible | gg_enabled;
    vgcd[5].gd.cid = CID_PFMFamily;
    vgcd[5].gd.u.list = pfmfamily;
    vgcd[5].creator = GListButtonCreate;

    vgcd[6].gd.pos.x = 10; vgcd[6].gd.pos.y = vgcd[5].gd.pos.y+26+6;
    vlabel[6].text = (unichar_t *) _STR_Embeddable;
    vlabel[6].text_in_resource = true;
    vgcd[6].gd.label = &vlabel[6];
    vgcd[6].gd.flags = gg_visible | gg_enabled;
    vgcd[6].gd.popup_msg = GStringGetResource(_STR_EmbeddablePopup,NULL);
    vgcd[6].creator = GLabelCreate;

    vgcd[7].gd.pos.x = 100; vgcd[7].gd.pos.y = vgcd[6].gd.pos.y-6;
    vgcd[7].gd.pos.width = 140;
    vgcd[7].gd.flags = gg_visible | gg_enabled;
    vgcd[7].gd.cid = CID_FSType;
    vgcd[7].gd.u.list = fstype;
    vgcd[7].gd.popup_msg = vgcd[6].gd.popup_msg;
    vgcd[7].creator = GListButtonCreate;
    fstype[0].selected = fstype[1].selected =
	    fstype[2].selected = fstype[3].selected = false;
    if ( sf->pfminfo.fstype&0x8 /* also catches the "not set" case == -1 */ )
	i = 2;
    else if ( sf->pfminfo.fstype&0x4 )
	i = 1;
    else if ( sf->pfminfo.fstype&0x2 )
	i = 0;
    else
	i = 3;
    fstype[i].selected = true;
    vgcd[7].gd.label = &fstype[i];

    vgcd[8].gd.pos.x = 20; vgcd[8].gd.pos.y = vgcd[7].gd.pos.y+26;
    vlabel[8].text = (unichar_t *) _STR_NoSubsetting;
    vlabel[8].text_in_resource = true;
    vgcd[8].gd.label = &vlabel[8];
    vgcd[8].gd.flags = gg_visible | gg_enabled;
    if ( sf->pfminfo.fstype!=-1 && (sf->pfminfo.fstype&0x100) )
	vgcd[8].gd.flags |= gg_cb_on;
    vgcd[8].gd.popup_msg = GStringGetResource(_STR_NoSubsettingPopup,NULL);
    vgcd[8].gd.cid = CID_NoSubsetting;
    vgcd[8].creator = GCheckBoxCreate;

    vgcd[9].gd.pos.x = 110; vgcd[9].gd.pos.y = vgcd[8].gd.pos.y;
    vlabel[9].text = (unichar_t *) _STR_OnlyBitmaps;
    vlabel[9].text_in_resource = true;
    vgcd[9].gd.label = &vlabel[9];
    vgcd[9].gd.flags = gg_visible | gg_enabled;
    if ( sf->pfminfo.fstype!=-1 && ( sf->pfminfo.fstype&0x200 ))
	vgcd[9].gd.flags |= gg_cb_on;
    vgcd[9].gd.popup_msg = GStringGetResource(_STR_OnlyBitmapsPopup,NULL);
    vgcd[9].gd.cid = CID_OnlyBitmaps;
    vgcd[9].creator = GCheckBoxCreate;

    vgcd[10].gd.pos.x = 10; vgcd[10].gd.pos.y = vgcd[9].gd.pos.y+26+4;
    vlabel[10].text = (unichar_t *) _STR_WinAscentOff;
    vlabel[10].text_in_resource = true;
    vgcd[10].gd.label = &vlabel[10];
    vgcd[10].gd.flags = gg_visible | gg_enabled;
    vgcd[10].gd.popup_msg = GStringGetResource(_STR_WinAscentPopup,NULL);
    vgcd[10].gd.cid = CID_WinAscentLab;
    vgcd[10].creator = GLabelCreate;

    vgcd[11].gd.pos.x = 105; vgcd[11].gd.pos.y = vgcd[10].gd.pos.y-4;
    vgcd[11].gd.pos.width = 50;
    vgcd[11].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    vgcd[11].gd.cid = CID_WinAscent;
    vgcd[11].gd.popup_msg = vgcd[10].gd.popup_msg;
    vgcd[11].creator = GTextFieldCreate;

    vgcd[12].gd.pos.x = 160; vgcd[12].gd.pos.y = vgcd[11].gd.pos.y;
    vlabel[12].text = (unichar_t *) _STR_IsOffset;
    vlabel[12].text_in_resource = true;
    vgcd[12].gd.label = &vlabel[12];
    vgcd[12].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    vgcd[12].gd.cid = CID_WinAscentIsOff;
    vgcd[12].gd.popup_msg = vgcd[10].gd.popup_msg;
    vgcd[12].gd.handle_controlevent = GFI_AsDesIsOff;
    vgcd[12].creator = GCheckBoxCreate;

    vgcd[13].gd.pos.x = 10; vgcd[13].gd.pos.y = vgcd[11].gd.pos.y+26+4;
    vlabel[13].text = (unichar_t *) _STR_WinDescentOff;
    vlabel[13].text_in_resource = true;
    vgcd[13].gd.label = &vlabel[13];
    vgcd[13].gd.flags = gg_visible | gg_enabled;
    vgcd[13].gd.popup_msg = vgcd[10].gd.popup_msg;
    vgcd[13].gd.cid = CID_WinDescentLab;
    vgcd[13].creator = GLabelCreate;

    vgcd[14].gd.pos.x = 105; vgcd[14].gd.pos.y = vgcd[13].gd.pos.y-4;
    vgcd[14].gd.pos.width = 50;
    vgcd[14].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    vgcd[14].gd.cid = CID_WinDescent;
    vgcd[14].gd.popup_msg = vgcd[10].gd.popup_msg;
    vgcd[14].creator = GTextFieldCreate;

    vgcd[15].gd.pos.x = 160; vgcd[15].gd.pos.y = vgcd[14].gd.pos.y;
    vlabel[15].text = (unichar_t *) _STR_IsOffset;
    vlabel[15].text_in_resource = true;
    vgcd[15].gd.label = &vlabel[15];
    vgcd[15].gd.flags = gg_visible | gg_enabled;
	/* value set later */
    vgcd[15].gd.cid = CID_WinDescentIsOff;
    vgcd[15].gd.popup_msg = vgcd[10].gd.popup_msg;
    vgcd[15].gd.handle_controlevent = GFI_AsDesIsOff;
    vgcd[15].creator = GCheckBoxCreate;

    vgcd[16].gd.pos.x = 10; vgcd[16].gd.pos.y = vgcd[14].gd.pos.y+26+4;
    vlabel[16].text = (unichar_t *) _STR_LineGap;
    vlabel[16].text_in_resource = true;
    vgcd[16].gd.label = &vlabel[16];
    vgcd[16].gd.flags = gg_visible | gg_enabled;
    vgcd[16].gd.popup_msg = GStringGetResource(_STR_LineGapPopup,NULL);
    vgcd[16].creator = GLabelCreate;

    vgcd[17].gd.pos.x = 105; vgcd[17].gd.pos.y = vgcd[16].gd.pos.y-4;
    vgcd[17].gd.pos.width = 50;
    vgcd[17].gd.flags = gg_visible | gg_enabled;
	/* Line gap value set later */
    vgcd[17].gd.cid = CID_LineGap;
    vgcd[17].gd.popup_msg = vgcd[16].gd.popup_msg;
    vgcd[17].creator = GTextFieldCreate;

    vgcd[18].gd.pos.x = 10; vgcd[18].gd.pos.y = vgcd[17].gd.pos.y+26+6;
    vlabel[18].text = (unichar_t *) _STR_VLineGap;
    vlabel[18].text_in_resource = true;
    vgcd[18].gd.label = &vlabel[18];
    vgcd[18].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled) : gg_visible;
    vgcd[18].gd.popup_msg = GStringGetResource(_STR_VLineGapPopup,NULL);
    vgcd[18].gd.cid = CID_VLineGapLab;
    vgcd[18].creator = GLabelCreate;

    vgcd[19].gd.pos.x = 105; vgcd[19].gd.pos.y = vgcd[18].gd.pos.y-6;
    vgcd[19].gd.pos.width = 50;
    vgcd[19].gd.flags = sf->hasvmetrics ? (gg_visible | gg_enabled) : gg_visible;
	/* V Line gap value set later */
    vgcd[19].gd.cid = CID_VLineGap;
    vgcd[19].gd.popup_msg = vgcd[17].gd.popup_msg;
    vgcd[19].creator = GTextFieldCreate;

    vgcd[20].gd.pos.x = 10; vgcd[20].gd.pos.y = vgcd[19].gd.pos.y+26;
    vlabel[20].text = (unichar_t *) _STR_SetGSUBOrder;
    vlabel[20].text_in_resource = true;
    vgcd[20].gd.label = &vlabel[20];
    vgcd[20].gd.flags = gg_visible | gg_enabled;
    vgcd[20].gd.handle_controlevent = OrderGSUB;
    vgcd[20].creator = GButtonCreate;

/******************************************************************************/
    memset(&tnlabel,0,sizeof(tnlabel));
    memset(&tngcd,0,sizeof(tngcd));

    tngcd[0].gd.pos.x = 20; tngcd[0].gd.pos.y = 6;
    tngcd[0].gd.flags = gg_visible | gg_enabled;
    tngcd[0].gd.cid = CID_StrID;
    tngcd[0].gd.u.list = ttfnameids;
    tngcd[0].gd.handle_controlevent = GFI_LanguageChange;
    tngcd[0].creator = GListButtonCreate;

    tngcd[1].gd.pos.x = 150; tngcd[1].gd.pos.y = tngcd[0].gd.pos.y;
    tngcd[1].gd.flags = gg_enabled;
    tngcd[1].gd.cid = CID_TNDef;
    tnlabel[1].text = (unichar_t *) _STR_TranslateStyle;
    tnlabel[1].text_in_resource = true;
    tngcd[1].gd.label = &tnlabel[1];
    tngcd[1].creator = GButtonCreate;
    tngcd[1].gd.handle_controlevent = GFI_DefaultStyles;

    tngcd[2].gd.pos.x = 30; tngcd[2].gd.pos.y = 36;
    tngcd[2].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic;
    tngcd[2].gd.cid = CID_Language;
    tngcd[2].gd.u.list = mslanguages;
    tngcd[2].gd.handle_controlevent = GFI_LanguageChange;
    tngcd[2].creator = GListButtonCreate;

    tngcd[3].gd.pos.x = 12; tngcd[3].gd.pos.y = GDrawPointsToPixels(NULL,tngcd[2].gd.pos.y+12);
    tngcd[3].gd.pos.width = pos.width-40; tngcd[3].gd.pos.height = GDrawPointsToPixels(NULL,203);
    tngcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    tngcd[3].creator = GGroupCreate;

    tngcd[4].gd.pos.x = 14; tngcd[4].gd.pos.y = tngcd[2].gd.pos.y+30;
    tngcd[4].gd.pos.width = 220; tngcd[4].gd.pos.height = 180;
    tngcd[4].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap | gg_text_xim;
    tngcd[4].gd.cid = CID_String;
    tngcd[4].creator = GTextAreaCreate;

    tngcd[5].gd.pos.x = 8; tngcd[5].gd.pos.y = GDrawPointsToPixels(NULL,tngcd[0].gd.pos.y+12);
    tngcd[5].gd.pos.width = pos.width-32; tngcd[5].gd.pos.height = GDrawPointsToPixels(NULL,240);
    tngcd[5].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    tngcd[5].creator = GGroupCreate;
    
/******************************************************************************/
    memset(&panlabel,0,sizeof(panlabel));
    memset(&pangcd,0,sizeof(pangcd));

    pangcd[0].gd.pos.x = 10; pangcd[0].gd.pos.y = 12;
    panlabel[0].text = (unichar_t *) _STR_Family;
    panlabel[0].text_in_resource = true;
    pangcd[0].gd.label = &panlabel[0];
    pangcd[0].gd.flags = gg_visible | gg_enabled;
    pangcd[0].creator = GLabelCreate;

    pangcd[1].gd.pos.x = 100; pangcd[1].gd.pos.y = pangcd[0].gd.pos.y-6; pangcd[1].gd.pos.width = 120;
    pangcd[1].gd.flags = gg_visible | gg_enabled;
    pangcd[1].gd.cid = CID_PanFamily;
    pangcd[1].gd.u.list = panfamily;
    pangcd[1].creator = GListButtonCreate;

    pangcd[2].gd.pos.x = 10; pangcd[2].gd.pos.y = pangcd[1].gd.pos.y+26+6;
    panlabel[2].text = (unichar_t *) _STR_Serifs;
    panlabel[2].text_in_resource = true;
    pangcd[2].gd.label = &panlabel[2];
    pangcd[2].gd.flags = gg_visible | gg_enabled;
    pangcd[2].creator = GLabelCreate;

    pangcd[3].gd.pos.x = 100; pangcd[3].gd.pos.y = pangcd[2].gd.pos.y-6; pangcd[3].gd.pos.width = 120;
    pangcd[3].gd.flags = gg_visible | gg_enabled;
    pangcd[3].gd.cid = CID_PanSerifs;
    pangcd[3].gd.u.list = panserifs;
    pangcd[3].creator = GListButtonCreate;

    pangcd[4].gd.pos.x = 10; pangcd[4].gd.pos.y = pangcd[3].gd.pos.y+26+6;
    panlabel[4].text = (unichar_t *) _STR_Weight;
    panlabel[4].text_in_resource = true;
    pangcd[4].gd.label = &panlabel[4];
    pangcd[4].gd.flags = gg_visible | gg_enabled;
    pangcd[4].creator = GLabelCreate;

    pangcd[5].gd.pos.x = 100; pangcd[5].gd.pos.y = pangcd[4].gd.pos.y-6; pangcd[5].gd.pos.width = 120;
    pangcd[5].gd.flags = gg_visible | gg_enabled;
    pangcd[5].gd.cid = CID_PanWeight;
    pangcd[5].gd.u.list = panweight;
    pangcd[5].creator = GListButtonCreate;

    pangcd[6].gd.pos.x = 10; pangcd[6].gd.pos.y = pangcd[5].gd.pos.y+26+6;
    panlabel[6].text = (unichar_t *) _STR_Proportion;
    panlabel[6].text_in_resource = true;
    pangcd[6].gd.label = &panlabel[6];
    pangcd[6].gd.flags = gg_visible | gg_enabled;
    pangcd[6].creator = GLabelCreate;

    pangcd[7].gd.pos.x = 100; pangcd[7].gd.pos.y = pangcd[6].gd.pos.y-6; pangcd[7].gd.pos.width = 120;
    pangcd[7].gd.flags = gg_visible | gg_enabled;
    pangcd[7].gd.cid = CID_PanProp;
    pangcd[7].gd.u.list = panprop;
    pangcd[7].creator = GListButtonCreate;

    pangcd[8].gd.pos.x = 10; pangcd[8].gd.pos.y = pangcd[7].gd.pos.y+26+6;
    panlabel[8].text = (unichar_t *) _STR_Contrast;
    panlabel[8].text_in_resource = true;
    pangcd[8].gd.label = &panlabel[8];
    pangcd[8].gd.flags = gg_visible | gg_enabled;
    pangcd[8].creator = GLabelCreate;

    pangcd[9].gd.pos.x = 100; pangcd[9].gd.pos.y = pangcd[8].gd.pos.y-6; pangcd[9].gd.pos.width = 120;
    pangcd[9].gd.flags = gg_visible | gg_enabled;
    pangcd[9].gd.cid = CID_PanContrast;
    pangcd[9].gd.u.list = pancontrast;
    pangcd[9].creator = GListButtonCreate;

    pangcd[10].gd.pos.x = 10; pangcd[10].gd.pos.y = pangcd[9].gd.pos.y+26+6;
    panlabel[10].text = (unichar_t *) _STR_StrokeVar;
    panlabel[10].text_in_resource = true;
    pangcd[10].gd.label = &panlabel[10];
    pangcd[10].gd.flags = gg_visible | gg_enabled;
    pangcd[10].creator = GLabelCreate;

    pangcd[11].gd.pos.x = 100; pangcd[11].gd.pos.y = pangcd[10].gd.pos.y-6; pangcd[11].gd.pos.width = 120;
    pangcd[11].gd.flags = gg_visible | gg_enabled;
    pangcd[11].gd.cid = CID_PanStrokeVar;
    pangcd[11].gd.u.list = panstrokevar;
    pangcd[11].creator = GListButtonCreate;

    pangcd[12].gd.pos.x = 10; pangcd[12].gd.pos.y = pangcd[11].gd.pos.y+26+6;
    panlabel[12].text = (unichar_t *) _STR_ArmStyle;
    panlabel[12].text_in_resource = true;
    pangcd[12].gd.label = &panlabel[12];
    pangcd[12].gd.flags = gg_visible | gg_enabled;
    pangcd[12].creator = GLabelCreate;

    pangcd[13].gd.pos.x = 100; pangcd[13].gd.pos.y = pangcd[12].gd.pos.y-6; pangcd[13].gd.pos.width = 120;
    pangcd[13].gd.flags = gg_visible | gg_enabled;
    pangcd[13].gd.cid = CID_PanArmStyle;
    pangcd[13].gd.u.list = panarmstyle;
    pangcd[13].creator = GListButtonCreate;

    pangcd[14].gd.pos.x = 10; pangcd[14].gd.pos.y = pangcd[13].gd.pos.y+26+6;
    panlabel[14].text = (unichar_t *) _STR_Letterform;
    panlabel[14].text_in_resource = true;
    pangcd[14].gd.label = &panlabel[14];
    pangcd[14].gd.flags = gg_visible | gg_enabled;
    pangcd[14].creator = GLabelCreate;

    pangcd[15].gd.pos.x = 100; pangcd[15].gd.pos.y = pangcd[14].gd.pos.y-6; pangcd[15].gd.pos.width = 120;
    pangcd[15].gd.flags = gg_visible | gg_enabled;
    pangcd[15].gd.cid = CID_PanLetterform;
    pangcd[15].gd.u.list = panletterform;
    pangcd[15].creator = GListButtonCreate;

    pangcd[16].gd.pos.x = 10; pangcd[16].gd.pos.y = pangcd[15].gd.pos.y+26+6;
    panlabel[16].text = (unichar_t *) _STR_MidLine;
    panlabel[16].text_in_resource = true;
    pangcd[16].gd.label = &panlabel[16];
    pangcd[16].gd.flags = gg_visible | gg_enabled;
    pangcd[16].creator = GLabelCreate;

    pangcd[17].gd.pos.x = 100; pangcd[17].gd.pos.y = pangcd[16].gd.pos.y-6; pangcd[17].gd.pos.width = 120;
    pangcd[17].gd.flags = gg_visible | gg_enabled;
    pangcd[17].gd.cid = CID_PanMidLine;
    pangcd[17].gd.u.list = panmidline;
    pangcd[17].creator = GListButtonCreate;

    pangcd[18].gd.pos.x = 10; pangcd[18].gd.pos.y = pangcd[17].gd.pos.y+26+6;
    panlabel[18].text = (unichar_t *) _STR_XHeight;
    panlabel[18].text_in_resource = true;
    pangcd[18].gd.label = &panlabel[18];
    pangcd[18].gd.flags = gg_visible | gg_enabled;
    pangcd[18].creator = GLabelCreate;

    pangcd[19].gd.pos.x = 100; pangcd[19].gd.pos.y = pangcd[18].gd.pos.y-6; pangcd[19].gd.pos.width = 120;
    pangcd[19].gd.flags = gg_visible | gg_enabled;
    pangcd[19].gd.cid = CID_PanXHeight;
    pangcd[19].gd.u.list = panxheight;
    pangcd[19].creator = GListButtonCreate;
/******************************************************************************/
    memset(&comlabel,0,sizeof(comlabel));
    memset(&comgcd,0,sizeof(comgcd));

    comgcd[0].gd.pos.x = 10; comgcd[0].gd.pos.y = 10;
    comgcd[0].gd.pos.width = ngcd[7].gd.pos.width; comgcd[0].gd.pos.height = 230;
    comgcd[0].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    comgcd[0].gd.cid = CID_Comment;
    comlabel[0].text = (unichar_t *) sf->comments;
    comlabel[0].text_is_1byte = true;
    if ( comlabel[0].text==NULL ) comlabel[0].text = (unichar_t *) "";
    comgcd[0].gd.label = &comlabel[0];
    comgcd[0].creator = GTextAreaCreate;
/******************************************************************************/
    memset(&atlabel,0,sizeof(atlabel));
    memset(&atgcd,0,sizeof(atgcd));

    atgcd[0].gd.pos.x = 10; atgcd[0].gd.pos.y = 10;
    atgcd[0].gd.pos.width = ngcd[7].gd.pos.width; atgcd[0].gd.pos.height = 200;
    atgcd[0].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic;
    atgcd[0].gd.cid = CID_AnchorClasses;
    atgcd[0].gd.u.list = AnchorClassesList(sf);
    atgcd[0].gd.handle_controlevent = GFI_AnchorSelChanged;
    atgcd[0].creator = GListCreate;

    atgcd[1].gd.pos.x = 10; atgcd[1].gd.pos.y = atgcd[0].gd.pos.y+atgcd[0].gd.pos.height+4;
    atgcd[1].gd.pos.width = -1;
    atgcd[1].gd.flags = gg_visible | gg_enabled;
    atlabel[1].text = (unichar_t *) _STR_NewDDD_fem;
    atlabel[1].text_in_resource = true;
    atgcd[1].gd.label = &atlabel[1];
    atgcd[1].gd.cid = CID_AnchorNew;
    atgcd[1].gd.handle_controlevent = GFI_AnchorNew;
    atgcd[1].creator = GButtonCreate;

    atgcd[2].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor); atgcd[2].gd.pos.y = atgcd[1].gd.pos.y;
    atgcd[2].gd.pos.width = -1;
    atgcd[2].gd.flags = gg_visible;
    atlabel[2].text = (unichar_t *) _STR_Delete;
    atlabel[2].text_in_resource = true;
    atgcd[2].gd.label = &atlabel[2];
    atgcd[2].gd.cid = CID_AnchorDel;
    atgcd[2].gd.handle_controlevent = GFI_AnchorDel;
    atgcd[2].creator = GButtonCreate;

    atgcd[3].gd.pos.x = -10; atgcd[3].gd.pos.y = atgcd[1].gd.pos.y;
    atgcd[3].gd.pos.width = -1;
    atgcd[3].gd.flags = gg_visible;
    atlabel[3].text = (unichar_t *) _STR_EditDDD;
    atlabel[3].text_in_resource = true;
    atgcd[3].gd.label = &atlabel[3];
    atgcd[3].gd.cid = CID_AnchorRename;
    atgcd[3].gd.handle_controlevent = GFI_AnchorRename;
    atgcd[3].creator = GButtonCreate;

    atgcd[4].gd.pos.x = 10; atgcd[4].gd.pos.y = atgcd[1].gd.pos.y+30;
    atgcd[4].gd.flags = gg_visible;
    atlabel[4].text = (unichar_t *) _STR_ShowFirstMark;
    atlabel[4].text_in_resource = true;
    atgcd[4].gd.label = &atlabel[4];
    atgcd[4].gd.cid = CID_ShowMark;
    atgcd[4].gd.handle_controlevent = GFI_AnchorShowMark;
    atgcd[4].creator = GButtonCreate;

    atgcd[5].gd.pos.x = -10; atgcd[5].gd.pos.y = atgcd[4].gd.pos.y;
    atgcd[5].gd.flags = gg_visible;
    atlabel[5].text = (unichar_t *) _STR_ShowFirstBase;
    atlabel[5].text_in_resource = true;
    atgcd[5].gd.label = &atlabel[5];
    atgcd[5].gd.cid = CID_ShowBase;
    atgcd[5].gd.handle_controlevent = GFI_AnchorShowBase;
    atgcd[5].creator = GButtonCreate;

/******************************************************************************/
    memset(&txlabel,0,sizeof(txlabel));
    memset(&txgcd,0,sizeof(txgcd));

    k=0;
    txlabel[k].text = (unichar_t *) _STR_TeXText;
    txlabel[k].text_in_resource = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = 10;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.cid = CID_TeXText;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    txlabel[k].text = (unichar_t *) _STR_TeXMath;
    txlabel[k].text_in_resource = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 80; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.cid = CID_TeXMath;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    txlabel[k].text = (unichar_t *) _STR_TeXMathExt;
    txlabel[k].text_in_resource = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 150; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.cid = CID_TeXMathExt;
    txgcd[k].gd.handle_controlevent = GFI_TeXChanged;
    txgcd[k++].creator = GRadioCreate;

    txlabel[k].text = (unichar_t *) _STR_DesignSize;
    txlabel[k].text_in_resource = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = txgcd[k-2].gd.pos.y+26;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.popup_msg = GStringGetResource(_STR_DesignSizePopup,NULL);
    txgcd[k++].creator = GLabelCreate;

    txgcd[k].gd.pos.x = 70; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y-4;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.cid = CID_DesignSize;
    txgcd[k++].creator = GTextFieldCreate;

    for ( i=0; texparams[i]!=0; ++i ) {
	txlabel[k].text = (unichar_t *) texparams[i];
	txlabel[k].text_in_resource = true;
	txgcd[k].gd.label = &txlabel[k];
	txgcd[k].gd.pos.x = 10; txgcd[k].gd.pos.y = txgcd[k-2].gd.pos.y+26;
	txgcd[k].gd.flags = gg_visible | gg_enabled;
	txgcd[k].gd.popup_msg = GStringGetResource(texpopups[i],NULL);
	txgcd[k++].creator = GLabelCreate;

	txgcd[k].gd.pos.x = 70; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y-4;
	txgcd[k].gd.flags = gg_visible | gg_enabled;
	txgcd[k].gd.cid = CID_TeX + i;
	txgcd[k++].creator = GTextFieldCreate;
    }
    txgcd[k-2].gd.cid = CID_TeXExtraSpLabel;

    txlabel[k].text = (unichar_t *) _STR_MoreParams;
    txlabel[k].text_in_resource = true;
    txgcd[k].gd.label = &txlabel[k];
    txgcd[k].gd.pos.x = 20; txgcd[k].gd.pos.y = txgcd[k-1].gd.pos.y+26;
    txgcd[k].gd.flags = gg_visible | gg_enabled;
    txgcd[k].gd.handle_controlevent = GFI_MoreParams;
    txgcd[k].gd.cid = CID_MoreParams;
    txgcd[k++].creator = GButtonCreate;

/******************************************************************************/
    memset(&csublabel,0,sizeof(csublabel));
    memset(&csubgcd,0,sizeof(csubgcd));
    memset(&congcd,0,sizeof(congcd));
    memset(&conaspects,'\0',sizeof(conaspects));

    for ( i=0; i<fpst_max-pst_contextpos; ++i ) {
	conaspects[i].text = (unichar_t *) connames[i];
	conaspects[i].text_in_resource = true;
	conaspects[i].gcd = csubgcd[i];

	csubgcd[i][0].gd.pos.x = 10; csubgcd[i][0].gd.pos.y = 10;
	csubgcd[i][0].gd.pos.width = ngcd[7].gd.pos.width;
	csubgcd[i][0].gd.pos.height = 150;
	csubgcd[i][0].gd.flags = gg_visible | gg_enabled;
	csubgcd[i][0].gd.cid = CID_ContextClasses+i*100;
	csubgcd[i][0].gd.u.list = FPSTList(sf,contypes[i]);
	csubgcd[i][0].gd.handle_controlevent = GFI_ContextSelChanged;
	csubgcd[i][0].creator = GListCreate;

	csubgcd[i][1].gd.pos.x = 10; csubgcd[i][1].gd.pos.y = csubgcd[i][0].gd.pos.y+csubgcd[i][0].gd.pos.height+4;
	csubgcd[i][1].gd.pos.width = -1;
	csubgcd[i][1].gd.flags = gg_visible | gg_enabled;
	csublabel[i][1].text = (unichar_t *) _STR_NewDDD;
	csublabel[i][1].text_in_resource = true;
	csubgcd[i][1].gd.label = &csublabel[i][1];
	csubgcd[i][1].gd.cid = CID_ContextNew+i*100;
	csubgcd[i][1].gd.handle_controlevent = GFI_ContextNew;
	csubgcd[i][1].creator = GButtonCreate;

	csubgcd[i][2].gd.pos.x = 10+(csubgcd[i][0].gd.pos.width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor))/2; csubgcd[i][2].gd.pos.y = csubgcd[i][1].gd.pos.y;
	csubgcd[i][2].gd.pos.width = -1;
	csubgcd[i][2].gd.flags = gg_visible;
	csublabel[i][2].text = (unichar_t *) _STR_Delete;
	csublabel[i][2].text_in_resource = true;
	csubgcd[i][2].gd.label = &csublabel[i][2];
	csubgcd[i][2].gd.cid = CID_ContextDel+i*100;
	csubgcd[i][2].gd.handle_controlevent = GFI_ContextDel;
	csubgcd[i][2].creator = GButtonCreate;

	csubgcd[i][3].gd.pos.x = 10+(csubgcd[i][0].gd.pos.width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)); csubgcd[i][3].gd.pos.y = csubgcd[i][1].gd.pos.y;
	csubgcd[i][3].gd.pos.width = -1;
	csubgcd[i][3].gd.flags = gg_visible;
	csublabel[i][3].text = (unichar_t *) _STR_EditDDD;
	csublabel[i][3].text_in_resource = true;
	csubgcd[i][3].gd.label = &csublabel[i][3];
	csubgcd[i][3].gd.cid = CID_ContextEdit+i*100;
	csubgcd[i][3].gd.handle_controlevent = GFI_ContextEdit;
	csubgcd[i][3].creator = GButtonCreate;

	csubgcd[i][4].gd.pos.x = csubgcd[i][2].gd.pos.x-2; csubgcd[i][4].gd.pos.y = csubgcd[i][1].gd.pos.y+28;
	csubgcd[i][4].gd.flags = gg_visible;
	csublabel[i][4].text = (unichar_t *) _STR_EditDataDDD;
	csublabel[i][4].text_in_resource = true;
	csubgcd[i][4].gd.label = &csublabel[i][4];
	csubgcd[i][4].gd.cid = CID_ContextEditData+i*100;
	csubgcd[i][4].gd.handle_controlevent = GFI_ContextEditData;
	csubgcd[i][4].creator = GButtonCreate;
    }

    conaspects[3].selected = true;	/* chaining subs is most likely to be used, so select it by default */

    congcd[0].gd.pos.x = 4; congcd[0].gd.pos.y = 10;
    congcd[0].gd.pos.width = 250;
    congcd[0].gd.pos.height = 260;
    congcd[0].gd.u.tabs = conaspects;
    congcd[0].gd.flags = gg_visible | gg_enabled;
    /*congcd[0].gd.handle_controlevent = GFI_AspectChange;*/
    congcd[0].creator = GTabSetCreate;

/******************************************************************************/
    memset(&mcgcd,0,sizeof(mcgcd));
    memset(&mclabel,'\0',sizeof(mclabel));

    k=0;
    mclabel[k].text = (unichar_t *) _STR_MacStyleSet;
    mclabel[k].text_in_resource = true;
    mcgcd[k].gd.label = &mclabel[k];
    mcgcd[k].gd.pos.x = 10; mcgcd[k].gd.pos.y = 7;
    mcgcd[k].gd.flags = gg_visible | gg_enabled;
    mcgcd[k++].creator = GLabelCreate;

    mclabel[k].text = (unichar_t *) _STR_Automatic;
    mclabel[k].text_in_resource = true;
    mcgcd[k].gd.label = &mclabel[k];
    mcgcd[k].gd.pos.x = 10; mcgcd[k].gd.pos.y = 20;
    mcgcd[k].gd.flags = (sf->macstyle==-1) ? (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    mcgcd[k].gd.cid = CID_MacAutomatic;
    mcgcd[k].gd.handle_controlevent = GFI_MacAutomatic;
    mcgcd[k++].creator = GRadioCreate;

    mcgcd[k].gd.pos.x = 90; mcgcd[k].gd.pos.y = 20;
    mcgcd[k].gd.flags = (sf->macstyle!=-1) ? (gg_visible | gg_enabled | gg_cb_on) : (gg_visible | gg_enabled);
    mcgcd[k].gd.handle_controlevent = GFI_MacAutomatic;
    mcgcd[k++].creator = GRadioCreate;

    mcgcd[k].gd.pos.x = 110; mcgcd[k].gd.pos.y = 20;
    mcgcd[k].gd.pos.width = 120; mcgcd[k].gd.pos.height = 7*12+10;
    mcgcd[k].gd.flags = (sf->macstyle==-1) ? (gg_visible | gg_list_multiplesel) : (gg_visible | gg_enabled | gg_list_multiplesel);
    mcgcd[k].gd.cid = CID_MacStyles;
    mcgcd[k].gd.u.list = macstyles;
    mcgcd[k++].creator = GListCreate;

    mclabel[k].text = (unichar_t *) _STR_FONDName;
    mclabel[k].text_in_resource = true;
    mcgcd[k].gd.label = &mclabel[k];
    mcgcd[k].gd.pos.x = 10; mcgcd[k].gd.pos.y = mcgcd[k-1].gd.pos.y + mcgcd[k-1].gd.pos.height+8;
    mcgcd[k].gd.flags = gg_visible | gg_enabled;
    mcgcd[k++].creator = GLabelCreate;

    mclabel[k].text = (unichar_t *) sf->fondname;
    mclabel[k].text_is_1byte = true;
    mcgcd[k].gd.label = sf->fondname==NULL ? NULL : &mclabel[k];
    mcgcd[k].gd.pos.x = 90; mcgcd[k].gd.pos.y = mcgcd[k-1].gd.pos.y - 4;
    mcgcd[k].gd.flags = gg_visible | gg_enabled;
    mcgcd[k].gd.cid = CID_MacFOND;
    mcgcd[k++].creator = GTextFieldCreate;


    mcs = MacStyleCode(sf,NULL);
    for ( i=0; macstyles[i].text!=NULL; ++i )
	macstyles[i].selected = (mcs&(int) (intpt) macstyles[i].userdata)? 1 : 0;

/******************************************************************************/
    memset(&smsublabel,0,sizeof(smsublabel));
    memset(&smsubgcd,0,sizeof(smsubgcd));
    memset(&smgcd,0,sizeof(smgcd));
    memset(&smaspects,'\0',sizeof(smaspects));

    for ( i=0; i<4; ++i ) {
	smaspects[i].text = (unichar_t *) smnames[i];
	smaspects[i].text_in_resource = true;
	smaspects[i].gcd = smsubgcd[i];

	smsubgcd[i][0].gd.pos.x = 10; smsubgcd[i][0].gd.pos.y = 10;
	smsubgcd[i][0].gd.pos.width = ngcd[7].gd.pos.width;
	smsubgcd[i][0].gd.pos.height = 150;
	smsubgcd[i][0].gd.flags = gg_visible | gg_enabled;
	smsubgcd[i][0].gd.cid = CID_SMList+i*100;
	smsubgcd[i][0].gd.u.list = SMList(sf,smtypes[i]);
	smsubgcd[i][0].gd.handle_controlevent = GFI_SMSelChanged;
	smsubgcd[i][0].creator = GListCreate;

	smsubgcd[i][1].gd.pos.x = 10; smsubgcd[i][1].gd.pos.y = smsubgcd[i][0].gd.pos.y+smsubgcd[i][0].gd.pos.height+4;
	smsubgcd[i][1].gd.pos.width = -1;
	smsubgcd[i][1].gd.flags = gg_visible | gg_enabled;
	smsublabel[i][1].text = (unichar_t *) _STR_NewDDD;
	smsublabel[i][1].text_in_resource = true;
	smsubgcd[i][1].gd.label = &smsublabel[i][1];
	smsubgcd[i][1].gd.cid = CID_SMNew+i*100;
	smsubgcd[i][1].gd.handle_controlevent = GFI_SMNew;
	smsubgcd[i][1].creator = GButtonCreate;

	smsubgcd[i][2].gd.pos.x = 10+(smsubgcd[i][0].gd.pos.width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor))/2; smsubgcd[i][2].gd.pos.y = smsubgcd[i][1].gd.pos.y;
	smsubgcd[i][2].gd.pos.width = -1;
	smsubgcd[i][2].gd.flags = gg_visible;
	smsublabel[i][2].text = (unichar_t *) _STR_Delete;
	smsublabel[i][2].text_in_resource = true;
	smsubgcd[i][2].gd.label = &smsublabel[i][2];
	smsubgcd[i][2].gd.cid = CID_SMDel+i*100;
	smsubgcd[i][2].gd.handle_controlevent = GFI_SMDel;
	smsubgcd[i][2].creator = GButtonCreate;

	smsubgcd[i][3].gd.pos.x = 10+(smsubgcd[i][0].gd.pos.width-GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor)); smsubgcd[i][3].gd.pos.y = smsubgcd[i][1].gd.pos.y;
	smsubgcd[i][3].gd.pos.width = -1;
	smsubgcd[i][3].gd.flags = gg_visible;
	smsublabel[i][3].text = (unichar_t *) _STR_EditDDD;
	smsublabel[i][3].text_in_resource = true;
	smsubgcd[i][3].gd.label = &smsublabel[i][3];
	smsubgcd[i][3].gd.cid = CID_SMEdit+i*100;
	smsubgcd[i][3].gd.handle_controlevent = GFI_SMEdit;
	smsubgcd[i][3].creator = GButtonCreate;

	if ( i==1 ) {
	    smsubgcd[i][4].gd.pos.x = 10; smsubgcd[i][4].gd.pos.y = smsubgcd[i][1].gd.pos.y+30;
	    smsubgcd[i][4].gd.flags = SFAnyConvertableSM(sf) ? gg_visible | gg_enabled : gg_visible;
	    smsublabel[i][4].text = (unichar_t *) _STR_ConvertFromOpenType;
	    smsublabel[i][4].text_in_resource = true;
	    smsubgcd[i][4].gd.label = &smsublabel[i][4];
	    smsubgcd[i][4].gd.handle_controlevent = GFI_SMConvert;
	    smsubgcd[i][4].creator = GButtonCreate;
	}
    }

    smaspects[1].selected = true;	/* Contextual glyph subs is most likely to be used, so select it by default */

    smgcd[0].gd.pos.x = 4; smgcd[0].gd.pos.y = 10;
    smgcd[0].gd.pos.width = 250;
    smgcd[0].gd.pos.height = 260;
    smgcd[0].gd.u.tabs = smaspects;
    smgcd[0].gd.flags = gg_visible | gg_enabled;
    /*smgcd[0].gd.handle_controlevent = GFI_AspectChange;*/
    smgcd[0].creator = GTabSetCreate;

/******************************************************************************/
    memset(&mfgcd,0,sizeof(mfgcd));
    memset(&mflabel,'\0',sizeof(mflabel));

    GCDFillMacFeat(mfgcd,mflabel,250,sf->features, false);

/******************************************************************************/

    memset(&mlabel,0,sizeof(mlabel));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&aspects,'\0',sizeof(aspects));

    i = 0;

    aspects[i].text = (unichar_t *) _STR_Names;
    d->old_aspect = 0;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = ngcd;

    aspects[i].text = (unichar_t *) _STR_Encoding2;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = egcd;

    aspects[i].text = (unichar_t *) _STR_PSGeneral;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = psgcd;

    d->private_aspect = i;
    aspects[i].text = (unichar_t *) _STR_PSPrivate;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = pgcd;

    d->ttfv_aspect = i;
    aspects[i].text = (unichar_t *) _STR_TTFValues;
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = vgcd;

    d->tn_aspect = i;
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _STR_TTFNames;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = tngcd;

    d->panose_aspect = i;
    if ( sf->cidmaster!=NULL ) aspects[i].disabled = true;
    aspects[i].text = (unichar_t *) _STR_Panose;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = pangcd;

    d->tx_aspect = i;
    aspects[i].text = (unichar_t *) _STR_TeX;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = txgcd;

    aspects[i].text = (unichar_t *) _STR_Comment;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = comgcd;

    aspects[i].text = (unichar_t *) _STR_AnchorClass;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = atgcd;

    aspects[i].text = (unichar_t *) _STR_Contextual;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = congcd;

    aspects[i].text = (unichar_t *) _STR_Mac;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = mcgcd;

    aspects[i].text = (unichar_t *) _STR_MacFeatures;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = mfgcd;

    aspects[i].text = (unichar_t *) _STR_MacStateMachine;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = smgcd;

    aspects[defaspect].selected = true;

    mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
    mgcd[0].gd.pos.width = 260;
    mgcd[0].gd.pos.height = 325;
    mgcd[0].gd.u.tabs = aspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].gd.handle_controlevent = GFI_AspectChange;
    mgcd[0].creator = GTabSetCreate;

    mgcd[1].gd.pos.x = 30-3; mgcd[1].gd.pos.y = GDrawPixelsToPoints(NULL,pos.height)-35-3;
    mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _STR_OK;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = GFI_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.pos.x = -30; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y+3;
    mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _STR_Cancel;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = GFI_Cancel;
    mgcd[2].creator = GButtonCreate;

    mgcd[3].gd.pos.x = 2; mgcd[3].gd.pos.y = 2;
    mgcd[3].gd.pos.width = pos.width-4; mgcd[3].gd.pos.height = pos.height-4;
    mgcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    mgcd[3].creator = GGroupCreate;

    GGadgetsCreate(gw,mgcd);
    memset(&rq,0,sizeof(rq));
    rq.family_name = monospace;
    rq.point_size = 12;
    rq.weight = 400;
    font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
    GGadgetSetFont(atgcd[0].ret,font);
    if ( list!=encodingtypes )
	GTextInfoListFree(list);
    GTextInfoListFree(pgcd[0].gd.u.list);
    GTextInfoListFree(atgcd[0].gd.u.list);
    if ( GTabSetGetTabLines(mgcd[0].ret)>3 ) {
	int offset = (GTabSetGetTabLines(mgcd[0].ret)-2)*GDrawPointsToPixels(NULL,20);
	GRect temp;
	GDrawResize(gw,pos.width,pos.height+offset);
	GGadgetResize(mgcd[0].ret,GDrawPointsToPixels(NULL,mgcd[0].gd.pos.width),
		GDrawPointsToPixels(NULL,mgcd[0].gd.pos.height)+offset);
	GGadgetMove(mgcd[1].ret,GDrawPointsToPixels(NULL,mgcd[1].gd.pos.x),
		GDrawPointsToPixels(NULL,mgcd[1].gd.pos.y)+offset);
	GGadgetGetSize(mgcd[2].ret,&temp);
	GGadgetMove(mgcd[2].ret,temp.x, temp.y+offset);
	GGadgetResize(mgcd[3].ret,GDrawPointsToPixels(NULL,mgcd[3].gd.pos.width),
		GDrawPointsToPixels(NULL,mgcd[3].gd.pos.height)+offset);
    }
    GWidgetIndicateFocusGadget(ngcd[1].ret);
    ProcessListSel(d);
    GFI_AspectChange(mgcd[0].ret,NULL);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);

    if ( sync ) {
	while ( !d->done )
	    GDrawProcessOneEvent(NULL);
    }
}

void FontMenuFontInfo(void *_fv) {
    FontInfo( ((FontView *) _fv)->sf,-1,false);
}

void FontInfoDestroy(FontView *fv) {
    if ( fv->fontinfo )
	GFI_CancelClose( (struct gfi_data *) (fv->fontinfo) );
}
