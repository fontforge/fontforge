/* Copyright (C) 2000,2001 by George Williams */
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
#include "ustring.h"
#include "chardata.h"
#include "utype.h"

struct gfi_data {
    int done;
    SplineFont *sf;
    GWindow gw;
    int private_aspect, ttfv_aspect, panose_aspect, tn_aspect;
    int old_sel, old_aspect, old_lang, old_strid;
    int ttf_set, names_set;
    struct psdict *private;
    struct ttflangname *names;
    struct ttflangname def;
};

GTextInfo encodingtypes[] = {
    { (unichar_t *) _STR_Custom, NULL, 0, 0, (void *) em_none, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
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
    { (unichar_t *) _STR_Mac, NULL, 0, 0, (void *) em_mac, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Win, NULL, 0, 0, (void *) em_win, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Adobestd, NULL, 0, 0, (void *) em_adobestandard, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Symbol, NULL, 0, 0, (void *) em_symbol, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Texbase, NULL, 0, 0, (void *) em_base, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Unicode, NULL, 0, 0, (void *) em_unicode, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) _STR_Jis208, NULL, 0, 0, (void *) em_jis208, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Jis212, NULL, 0, 0, (void *) em_jis212, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Korean, NULL, 0, 0, (void *) em_ksc5601, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) _STR_Chinese, NULL, 0, 0, (void *) em_gb2312, NULL, 0, 0, 0, 0, 0, 0, 0, 1},
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
static const unichar_t demiboldeng[] = { 'D','e','m','i','-','B','o','l','d',  '\0' };
static const unichar_t demiboldeng2[] = { 'D','e','m','i','B','o','l','d',  '\0' };
static const unichar_t demiboldeng3[] = { 'D','e','m','i',  '\0' };
static const unichar_t demiboldeng4[] = { 'S','e','m','i','-','B','o','l','d',  '\0' };
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

static const unichar_t regularfren[] = { 'N','o','r','m','a','l',  '\0' };
static const unichar_t boldfren[] = { 'G','r','a','s',  '\0' };
static const unichar_t demiboldfren[] = { 'D','e','m','i',  '\0' };
static const unichar_t italicfren[] = { 'I','t','a','l','i','q','u','e',  '\0' };
static const unichar_t obliquefren[] = { 'O','b','l','i','q','u','e',  '\0' };

static const unichar_t regulargerm[] = { 'S','t','a','n','d','a','r','d',  '\0' };
static const unichar_t demiboldgerm[] = { 'H','a','l','b','f','e','t','t',  '\0' };
static const unichar_t boldgerm[] = { 'F','e','t','t',  '\0' };
static const unichar_t italicgerm[] = { 'K','u','r','s','i','v',  '\0' };
static const unichar_t obliquegerm[] = { 'S','c','h','r',0xe4,'g',  '\0' };	/* Guess */

static const unichar_t regularspan[] = { 'N','o','r','m','a','l',  '\0' };
static const unichar_t boldspan[] = { 'N','i','g','r','i','t','a',  '\0' };
static const unichar_t italicspan[] = { 'C','u','r','s','i','v','a',  '\0' };

static const unichar_t regularru[] = { 0x41e, 0x431, 0x44b, 0x447, 0x43d, 0x44b, 0x439,  '\0' };
static const unichar_t demiboldru[] = { 0x41f, 0x43e, 0x43b, 0x443, 0x436, 0x438, 0x440, 0x43d, 0x44b, 0x439,  0 };
static const unichar_t boldru[] = { 0x41e, 0x431, 0x44b, 0x447, 0x43d, 0x44b, 0x439,  0 };
static const unichar_t heavyru[] = { 0x421, 0x432, 0x435, 0x440, 0x445, 0x436, 0x438, 0x440, 0x43d, 0x44b, 0x439,  0 };
static const unichar_t blackru[] = { 0x427, 0x451, 0x440, 0x43d, 0x44b, 0x439,  0 };
static const unichar_t thinru[] = { 0x422, 0x43e, 0x43d, 0x43a, 0x438, 0x439,  0 };
static const unichar_t lightru[] = { 0x421, 0x432, 0x435, 0x442, 0x43b, 0x44b, 0x439,  0 };
static const unichar_t italicru[] = { 0x41a, 0x443, 0x440, 0x441, 0x438, 0x432, 0x43d, 0x44b, 0x439,  0 };
static const unichar_t obliqueru[] = { 0x41d, 0x430, 0x43a, 0x43b, 0x43e, 0x43d, 0x43d, 0x44b, 0x439,  0 };
static const unichar_t condensedru[] = { 0x423, 0x437, 0x43a, 0x438, 0x439,  '\0' };
static const unichar_t expandedru[] = { 0x428, 0x438, 0x440, 0x43e, 0x43a, 0x438, 0x439,  '\0' };

static struct langstyle regs[] = { {0x409, regulareng}, { 0x40c, regularfren }, { 0x407, regulargerm }, { 0x40a, regularspan }, { 0x419, regularru }, { 0 }};
static struct langstyle bolds[] = { {0x409, boldeng}, { 0x40c, boldfren }, { 0x407, boldgerm }, { 0x40a, boldspan}, { 0x419, boldru }, { 0 }};
static struct langstyle italics[] = { {0x409, italiceng}, { 0x40c, italicfren }, { 0x407, italicgerm }, { 0x40a, italicspan}, { 0x419, italicru }, { 0 }};
static struct langstyle obliques[] = { {0x409, obliqueeng}, { 0x40c, obliquefren }, { 0x407, obliquegerm }, { 0x419, obliqueru }, { 0 }};
static struct langstyle demibolds[] = { {0x409, demiboldeng}, {0x409, demiboldeng2}, {0x409, demiboldeng3}, {0x409, demiboldeng4}, {0x409, demiboldeng5},
	{ 0x40c, demiboldfren }, { 0x407, demiboldgerm }, { 0x419, demiboldru }, { 0 }};
static struct langstyle heavys[] = { {0x409, heavyeng}, { 0x419, heavyru }, { 0 }};
static struct langstyle blacks[] = { {0x409, blackeng}, { 0x419, blackru }, { 0 }};
static struct langstyle thins[] = { {0x409, thineng}, { 0x419, thinru }, { 0 }};
static struct langstyle lights[] = { {0x409, lighteng}, { 0x419, lightru }, { 0 }};
static struct langstyle condenseds[] = { {0x409, condensedeng}, { 0x419, condensedru }, { 0 }};
static struct langstyle expandeds[] = { {0x409, expandedeng}, { 0x419, expandedru }, { 0 }};
static struct langstyle *stylelist[] = {regs, demibolds, bolds, heavys, blacks,
	lights, thins, italics, obliques, condenseds, expandeds, NULL };

#define CID_Encoding	1001
#define CID_Family	1002
#define CID_Modifiers	1003
#define CID_ItalicAngle	1004
#define CID_UPos	1005
#define CID_UWidth	1006
#define CID_Ascent	1007
#define CID_Descent	1008
#define CID_NChars	1009
#define CID_Notice	1010
#define CID_Make	1011
#define CID_Delete	1012
#define CID_XUID	1013
#define CID_Human	1014

#define CID_PrivateEntries	2001
#define	CID_PrivateValues	2002
#define	CID_Add			2003
#define CID_Guess		2004
#define CID_Remove		2005

#define CID_WeightClass		3001
#define CID_WidthClass		3002
#define CID_PFMFamily		3003

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
    { "UniqueID", pt_number },
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
    ptwidth = 2*GIntGetResource(_NUM_Buttonsize)+60;
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

    gcd[3].gd.pos.x = ptwidth-GIntGetResource(_NUM_Buttonsize)-20; gcd[3].gd.pos.y = 90-35;
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
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_PrivateValues),false);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_PrivateValues),nullstr);
    } else {
	GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Remove),true);
	if ( strcmp(private->keys[sel],"BlueValues")==0 ||
		strcmp(private->keys[sel],"OtherBlues")==0 ||
		strcmp(private->keys[sel],"StdHW")==0 ||
		strcmp(private->keys[sel],"StemSnapH")==0 ||
		strcmp(private->keys[sel],"StdVW")==0 ||
		strcmp(private->keys[sel],"StemSnapV")==0 )
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),true);
	else
	    GGadgetSetEnabled(GWidgetGetControl(d->gw,CID_Guess),false);
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

static void arraystring(char *buffer,double *array,int cnt) {
    int i, ei;

    for ( ei=cnt; ei>1 && array[ei]==0; --ei );
    *buffer++ = '[';
    for ( i=0; i<ei; ++i ) {
	sprintf(buffer, "%d ", (int) array[i]);
	buffer += strlen(buffer);
    }
    if ( buffer[-1] ==' ' ) --buffer;
    *buffer++ = ']'; *buffer='\0';
}

static void SnapSet(struct psdict *private,double stemsnap[12], double snapcnt[12],
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
    double bluevalues[14], otherblues[10];
    double snapcnt[12];
    double stemsnap[12];
    char buffer[211];
    unichar_t *temp;
    static int buts[] = { _STR_OK, _STR_Cancel, 0 };
    struct psdict *private = d->private ? d->private : sf->private;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	sf = d->sf;
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
	    FindHStems(sf,stemsnap,snapcnt);
	    PIPrivateCheck(d);
	    SnapSet(d->private,stemsnap,snapcnt,"StdVW","StemSnapV");
	}
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
	Encoding *item, uint8 *used) {
    int i;
    /* for some reason, some encodings have multiple entries for the same */
    /*  glyph. One obvious one is space: 0x20 and 0xA0. The used bit array */
    /*  is designed to handle that unpleasantness */

    if ( table==NULL ) {
	if ( sc->unicodeenc==-1 ) {
	    if ( sc->enc!=0 || (used[0]&1) ) {
		sc->enc = -1;
return( false );
	    }
	    used[0] |= 1;
return( true );
	}
	if ( used[sc->unicodeenc>>3] & (1<<(sc->unicodeenc&7)) ) {
	    sc->enc = -1;
return( false );
	}
	used[sc->unicodeenc>>3] |= (1<<(sc->unicodeenc&7));
	sc->enc = sc->unicodeenc;
return( true );
    }
    if ( sc->unicodeenc==-1 ) {
	if ( sc->enc==0 && strcmp(sc->name,".notdef")==0 && table[0]==0 && !(used[0]&1)) {
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

    for ( i=0; i<tlen && (sc->unicodeenc!=table[i] || (used[i>>3]&(1<<(i&7)))); ++i );
    if ( i==tlen ) {
	sc->enc = -1;
return( false );
    } else {
	used[i>>3] |= (1<<(i&7));
	if ( tlen==94*94 ) {
	    sc->enc = (i/94)*96+(i%94)+1;
return( true );
	} else {
	    sc->enc = i;
return( true );
	}
    }
}

static void RemoveSplineChar(SplineFont *sf, int enc) {
    CharView *cv, *next;
    struct splinecharlist *dep, *dnext;
    BDFFont *bdf;
    BDFChar *bfc;
    SplineChar *sc = sf->chars[enc];
    BitmapView *bv, *bvnext;
    RefChar *refs, *rnext;

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
	    for ( rf = dsc->refs; rf!=NULL; rf=rnext ) {
		rnext = rf->next;
		SCRefToSplines(dsc,rf);
	    }
	}
	sf->chars[enc] = NULL;

	for ( refs=sc->refs; refs!=NULL; refs = rnext ) {
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

/* see also SplineFontNew in splineutil2.c */
int SFReencodeFont(SplineFont *sf,enum charset new_map) {
    const unsigned short *table;
    int i, extras, epos;
    SplineChar **chars;
    int enc_cnt;
    BDFFont *bdf;
    int tlen = 256;
    Encoding *item=NULL;
    uint8 *used;
    RefChar *refs;

    if ( sf->encoding_name==new_map )
return(false);
    if ( new_map==em_none ) {
	sf->encoding_name=em_none;	/* Custom, it's whatever's there */
return(false);
    }
    if ( new_map==em_adobestandard ) {
	table = unicode_from_adobestd;
    } else if ( new_map>=em_base ) {
	for ( item=enclist; item!=NULL && item->enc_num!=new_map; item=item->next );
	if ( item!=NULL ) {
	    tlen = item->char_cnt;
	    table = item->unicode;
	} else {
	    GWidgetErrorR(_STR_InvalidEncoding,_STR_InvalidEncoding);
return( false );
	}
    } else if ( new_map==em_iso8859_1 )
	table = unicode_from_i8859_1;
    else if ( new_map==em_unicode ) {
	table = NULL;
	tlen = 65536;
    } else if ( new_map==em_jis208 ) {
	table = unicode_from_jis208;
	tlen = 94*94;
    } else if ( new_map==em_jis212 ) {
	table = unicode_from_jis212;
	tlen = 94*94;
    } else if ( new_map==em_ksc5601 ) {
	table = unicode_from_ksc5601;
	tlen = 94*94;
    } else if ( new_map==em_gb2312 ) {
	table = unicode_from_gb2312;
	tlen = 94*94;
    } else
	table = unicode_from_alphabets[new_map+3];

    extras = 0;
    used = gcalloc((tlen+7)/8,sizeof(uint8));
    for ( i=0; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]==NULL )
	    /* skip */;
	else if ( !infont(sf->chars[i],table,tlen,item,used)) {
	    SplineChar *sc = sf->chars[i];
	    if ( sc->splines==NULL && sc->refs==NULL && sc->dependents==NULL
		    /*&& sc->width==sf->ascent+sf->descent*/ ) {
		RemoveSplineChar(sf,i);
	    } else
		++extras;
	}
    }
    free(used);
    enc_cnt=tlen;
    if ( new_map==em_unicode )
	enc_cnt = 65536;
    else if ( tlen == 94*94 )
	enc_cnt = 94*96;
    chars = calloc(enc_cnt+extras,sizeof(SplineChar *));
    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next )
	bdf->temp = calloc(enc_cnt+extras,sizeof(BDFChar *));
    for ( i=0, epos=enc_cnt; i<sf->charcnt; ++i ) {
	if ( sf->chars[i]==NULL )
	    /* skip */;
	else {
	    if ( sf->chars[i]->enc==-1 )
		sf->chars[i]->enc = epos++;
	    chars[sf->chars[i]->enc] = sf->chars[i];
	    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
		if ( bdf->chars[i]!=NULL ) {
		    if ( sf->chars[i]!=NULL )
			bdf->chars[i]->enc = sf->chars[i]->enc;
		    bdf->temp[sf->chars[i]->enc] = bdf->chars[i];
		}
	    }
	}
    }
    if ( epos!=enc_cnt+extras ) GDrawIError( "Bad count in ReencodeFont");
    for ( i=0; i<256; ++i ) {
	if ( chars[i]==NULL ) {
	    SplineChar *sc = chars[i] = calloc(1,sizeof(SplineChar));
	    sc->enc = i;
	    if ( table==NULL )
		sc->unicodeenc = i;
	    else if ( new_map>=em_jis208 && new_map<em_base ) {
		if ( i%96==0 || i%96==95 )
		    sc->unicodeenc = -1;
		else
		    sc->unicodeenc = table[(i/96)*94+(i%96-1)];
		if ( sc->unicodeenc==0 ) sc->unicodeenc = -1;
	    } else if ( ( sc->unicodeenc = table[i])==0xfffd )
		sc->unicodeenc = -1;
	    if ( sc->unicodeenc == 0 && i!=0 )
		sc->unicodeenc = -1;
	    if ( sc->unicodeenc!=-1 && sc->unicodeenc<psunicodenames_cnt )
		sc->name = copy(psunicodenames[sc->unicodeenc]);
	    if ( sc->name==NULL ) {
		if ( sc->unicodeenc==0xa0 )
		    sc->name = copy("nonbreakingspace");
		else if ( sc->unicodeenc==0x2d )
		    sc->name = copy("hyphen-minus");
		else if ( sc->unicodeenc==0xad )
		    sc->name = copy("softhyphen");
		else if ( sc->unicodeenc==0x00 )
		    sc->name = copy(".notdef");
		else if ( sc->unicodeenc!=-1 ) {
		    char buf[10];
		    sprintf(buf,"uni%04X", sc->unicodeenc );
		    sc->name = copy(buf);
		} else if ( item!=NULL && item->psnames!=NULL && item->psnames[i]!=NULL ) {
		    sc->name = copy(item->psnames[i]);
		} else
		    sc->name = copy(".notdef");
	    }
	    sc->width = sf->ascent+sf->descent;
	    sc->parent = sf;
	    sc->lig = SCLigDefault(sc);
	}
    }
    free(sf->chars);
    sf->chars = chars;
    sf->charcnt = enc_cnt+extras;
    sf->encoding_name = new_map;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	for ( refs=sf->chars[i]->refs; refs!=NULL; refs = refs->next )
	    refs->local_enc = refs->sc->enc;
    }

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	free(bdf->chars);
	bdf->chars = bdf->temp;
	bdf->temp = NULL;
	bdf->charcnt = enc_cnt+extras;
	bdf->encoding_name = new_map;
    }
return( true );
}

static int AddDelChars(SplineFont *sf, int nchars) {
    int i;
    BDFFont *bdf;
    MetricsView *mv, *mnext;

    if ( nchars==sf->charcnt )
return( false );
    if ( nchars>sf->charcnt ) {
	int is_unicode = 1;
	for ( i=0; i<sf->charcnt && is_unicode; ++i )
	    if ( sf->chars[i]==NULL || sf->chars[i]->unicodeenc!=i )
		is_unicode = false;
	sf->chars = realloc(sf->chars,nchars*sizeof(SplineChar *));
	for ( i=sf->charcnt; i<nchars; ++i )
	    sf->chars[i] = NULL;
#if 0
	for ( i=sf->charcnt; i<nchars; ++i ) {
	    SplineChar *sc = sf->chars[i] = calloc(1,sizeof(SplineChar));
	    sc->enc = i;
	    if ( is_unicode ) {
		sc->unicodeenc = i;
		if ( i<psunicodenames_cnt && psunicodenames[i]!=NULL )
		    sc->name = copy(psunicodenames[i]);
		else { char buf[10];
		    sprintf( buf, "uni%04X", i );
		    sc->name = copy(buf);
		}
	    } else
		sc->unicodeenc = -1;
	    sc->width = sf->ascent+sf->descent;
	    sc->parent = sf;
	}
#endif
	sf->charcnt = nchars;
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    bdf->chars = realloc(bdf->chars,nchars*sizeof(BDFChar *));
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
	if ( nchars<256 ) sf->encoding_name = em_none;
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    bdf->charcnt = nchars;
	    bdf->encoding_name = sf->encoding_name;
	}
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

static int GFI_NameChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	GWindow gw = GGadgetGetWindow(g);
	const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
	const unichar_t *umods = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Modifiers));
	unichar_t *uhum = galloc((u_strlen(ufamily)+u_strlen(umods)+2)*sizeof(unichar_t));
	u_strcpy(uhum,ufamily);
	if ( *umods!='\0' ) {
	    uc_strcat(uhum," ");
	    u_strcat(uhum,umods);
	}
	GGadgetSetTitle(GWidgetGetControl(gw,CID_Human),uhum);
	free(uhum);
    }
return( true );
}

static int GFI_GuessItalic(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	double val = SFGuessItalicAngle(d->sf);
	char buf[30]; unichar_t ubuf[30];
	sprintf( buf, "%.1f", val);
	uc_strcpy(ubuf,buf);
	GGadgetSetTitle(GWidgetGetControl(d->gw,CID_ItalicAngle),ubuf);
    }
return( true );
}

static int GFI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	d->done = true;
    }
return( true );
}

static int AskTooFew() {
    static int buts[] = { _STR_OK, _STR_Cancel, 0 };
return( GWidgetAskR(_STR_Toofew,buts,0,1,_STR_Reducing) );
}

static void BadFamily() {
    GWidgetErrorR(_STR_Badfamily,_STR_Badfamilyn);
}

char *SFGetModifiers(SplineFont *sf) {
    char *pt, *fpt;

    for ( fpt = sf->familyname, pt = sf->fontname; *fpt!='\0' && *pt!='\0'; ) {
	if ( *fpt == *pt ) {
	    ++fpt; ++pt;
	} else if ( *fpt==' ' )
	    ++fpt;
	else if ( *pt==' ' )
	    ++pt;
	else
return( pt );
    }
return ( pt );
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
    for ( pt=tpt=family; *pt; ) {
	if ( !isspace(*pt))
	    *tpt++ = *pt++;
	else
	    ++pt;
    }
    *tpt = '\0';
    free(sf->fontname); sf->fontname = n;
    free(sf->fullname); sf->fullname = copy(full);
    free(sf->familyname); sf->familyname = copy(family);
    free(sf->weight); sf->weight = NULL;
    if ( strstrmatch(mods,"extralight")!=NULL || strstrmatch(mods,"extra-light")!=NULL )
	sf->weight = copy("ExtraLight");
    else if ( strstrmatch(mods,"demilight")!=NULL || strstrmatch(mods,"demi-light")!=NULL )
	sf->weight = copy("DemiLight");
    else if ( strstrmatch(mods,"demibold")!=NULL || strstrmatch(mods,"demi-bold")!=NULL )
	sf->weight = copy("DemiBold");
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
    else
	sf->weight = copy("Medium");

    if ( sf->fv!=NULL ) {
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

static void SetFontName(GWindow gw, SplineFont *sf) {
    const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Family));
    const unichar_t *umods = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Modifiers));
    const unichar_t *uhum = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Human));
    char *family, *mods, *human;

    if ( uc_strcmp(ufamily,sf->familyname)==0 && uc_strcmp(uhum,sf->fullname)==0 &&
	    uc_strcmp(umods,SFGetModifiers(sf))==0 )
return;			/* Unchanged */
    family = cu_copy(ufamily); mods = cu_copy(umods); human = cu_copy(uhum);
    SFSetFontName(sf,family,mods,human);
    free(mods); free(family); free(human);
}

static int CheckNames(struct gfi_data *d) {
    const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Family));
    const unichar_t *umods = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Modifiers));
    unichar_t *end;

    if ( *ufamily=='\0' ) {
	GWidgetErrorR(_STR_FamilyNameRequired,_STR_FamilyNameRequired);
return( false );
    }
    /* A postscript name cannot be a number. There are two ways it can be a */
    /*  number, it can be a double (which we can check for with strtod) or */
    /*  it can be a "radix number" which is <intval>'#'<intval>. I'll only */
    /*  do a cursory test for that */
    u_strtod(ufamily,&end);
    if ( *end=='\0' || (isdigit(ufamily[0]) && u_strchr(ufamily,'#')!=NULL) ) {
	GWidgetErrorR(_STR_BadFamilyName,_STR_PSNameNotNumber);
return( false );
    }
    while ( *ufamily ) {
	if ( *ufamily<' ' || *ufamily>=0x7f ||
		*ufamily=='(' || *ufamily=='[' || *ufamily=='{' || *ufamily=='<' ||
		*ufamily==')' || *ufamily==']' || *ufamily=='}' || *ufamily=='>' ||
		*ufamily=='%' || *ufamily=='/' ) {
	    GWidgetErrorR(_STR_BadFamilyName,_STR_BadPSName);
return( false );
	}
	++ufamily;
    }

    u_strtod(umods,&end);
    if ( (*end=='\0' || (isdigit(umods[0]) && u_strchr(umods,'#')!=NULL)) &&
	    *umods!='\0' ) {
	GWidgetErrorR(_STR_BadModifierName,_STR_PSNameNotNumber);
return( false );
    }
    while ( *umods ) {
	if ( *umods<' ' || *umods>=0x7f ||
		*umods=='(' || *umods=='[' || *umods=='{' || *umods=='<' ||
		*umods==')' || *umods==']' || *umods=='}' || *umods=='>' ||
		*umods=='%' || *umods=='/' ) {
	    GWidgetErrorR(_STR_BadModifierName,_STR_BadPSName);
return( false );
	}
	++umods;
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
	    if ( prev==NULL ) d->names = ln;
	    else prev->next = ln;
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

static struct ttflangname *TTFLangNamesCopy(struct ttflangname *old) {
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

static int GFI_DefaultStyles(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	DoDefaultStyles(d);
	TNNotePresence(d,ttf_subfamily);
    }
return( true );
}

static void TNFinishFormer(struct gfi_data *d) {
    int cur_lang = GGadgetGetFirstListSelectedItem(GWidgetGetControl(d->gw,CID_Language));
    int cur_id = GGadgetGetFirstListSelectedItem(GWidgetGetControl(d->gw,CID_StrID));
    struct ttflangname *cur, *prev;
    int nothing;
    static unichar_t nullstr[1] = { 0 };
    int i;

    if ( d->old_lang!=-1 ) {
	int lang = (int) mslanguages[d->old_lang].userdata;
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
	}

	if ( nothing ) {
	    if ( prev==NULL )
		d->names = cur->next;
	    else
		prev->next = cur;
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
    cur_lang = (int) mslanguages[cur_lang].userdata;
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

static void DefaultLanguage(struct gfi_data *d) {
    const char *lang=NULL;
    const unichar_t *res;
    int i, found=-1, samelang=-1, langlen, reslen;
    static char *envs[] = { "LC_ALL", "LC_MESSAGES", "LANG", NULL };
    GGadget *g = GWidgetGetControl(d->gw,CID_Language);

    for ( i=0; envs[i]!=NULL && lang==NULL; ++i )
	lang = getenv(envs[i]);
    if ( lang==NULL ) lang = "en_US";
    langlen = strlen(lang);
    for ( i=0; mslanguages[i].text!=NULL; ++i ) {
	res = GStringGetResource((int) mslanguages[i].text, NULL );
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
    if ( found==-1 ) found = 0;
    GGadgetSelectOneListItem(g,found);

    d->old_lang = -1;
    d->names_set = true;
    d->names = TTFLangNamesCopy(d->sf->names);
    d->def.names[ttf_copyright] = GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Notice));
    d->def.names[ttf_family] = GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Family));
    d->def.names[ttf_fullname] = GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Human));
    d->def.names[ttf_subfamily] = GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Modifiers));
    if ( *d->def.names[ttf_subfamily]=='\0' ) {
	free( d->def.names[ttf_subfamily]);
	d->def.names[ttf_subfamily] = uc_copy("Regular");
    }
    DefaultTTFEnglishNames(&d->def, d->sf);
    TNFinishFormer(d);
}

static int GFI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct gfi_data *d = GDrawGetUserData(gw);
	SplineFont *sf = d->sf;
	int enc;
	int reformat_fv=0;
	int upos, uwid, as, des, nchar, oldcnt=sf->charcnt, err = false;
	double ia;
	const unichar_t *txt; unichar_t *end;

	if ( !CheckNames(d))
return( true );
	if ( !PIFinishFormer(d))
return( true );
	if ( d->names_set )
	    TNFinishFormer(d);
	
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
	upos = GetIntR(gw,CID_UPos, _STR_Upos,&err);
	uwid = GetIntR(gw,CID_UWidth,_STR_Uheight,&err);
	as = GetIntR(gw,CID_Ascent,_STR_Ascent,&err);
	des = GetIntR(gw,CID_Descent,_STR_Descent,&err);
	nchar = GetIntR(gw,CID_NChars,_STR_Numchars,&err);
	if ( err )
return(true);
	if ( as+des>16384 || des<0 || as<0 ) {
	    GWidgetErrorR(_STR_Badascentdescent,_STR_Badascentdescentn);
return( true );
	}
	if ( nchar<sf->charcnt && AskTooFew())
return(true);
	GDrawSetCursor(gw,ct_watch);
	SetFontName(gw,sf);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_XUID));
	free(sf->xuid); sf->xuid = *txt=='\0'?NULL:cu_copy(txt);
	txt = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Notice));
	free(sf->copyright); sf->copyright = cu_copy(txt);
	enc = GGadgetGetFirstListSelectedItem(GWidgetGetControl(gw,CID_Encoding));
	if ( enc!=-1 ) {
	    enc = (int) (GGadgetGetListItem(GWidgetGetControl(gw,CID_Encoding),enc)->userdata);
	    reformat_fv = SFReencodeFont(sf,enc);
	    if ( reformat_fv && nchar==oldcnt )
		nchar = sf->charcnt;
	}
	if ( nchar!=sf->charcnt )
	    reformat_fv = AddDelChars(sf,nchar);
	if ( as!=sf->ascent || des!=sf->descent ) {
	    sf->ascent = as;
	    sf->descent = des;
	    reformat_fv = true;
	}
	sf->italicangle = ia;
	sf->upos = upos;
	sf->uwidth = uwid;
	if ( d->private!=NULL ) {
	    PSDictFree(sf->private);
	    sf->private = d->private;
	    d->private = NULL;
	}
	if ( d->names_set ) {
	    TTFLangNamesFree(sf->names);
	    sf->names = d->names;
	    d->names = NULL;
	}
	if ( reformat_fv )
	    FontViewReformat(sf->fv);
	sf->changed = true;
	sf->changed_since_autosave = true;
	d->done = true;
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

static void TTFSetup(struct gfi_data *d) {
    struct pfminfo info;
    char buffer[10]; unichar_t ubuf[10];
    int i;

    info = d->sf->pfminfo;
    if ( !info.pfmset ) {
	/* Base this stuff on the CURRENT name */
	/* if the user just created a font, and named it *Bold, then the sf */
	/*  won't yet have Bold in its name, and basing the weight on it would*/
	/*  give the wrong answer. That's why we don't do this init until we */
	/*  get to one of the ttf aspects, it gives the user time to set the */
	/*  name properly */
	const unichar_t *ufamily = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Family));
	const unichar_t *umods = _GGadgetGetTitle(GWidgetGetControl(d->gw,CID_Modifiers));
	char *n = galloc(u_strlen(ufamily)+u_strlen(umods)+1);
	cu_strcpy(n,ufamily); cu_strcat(n,umods);
	SFDefaultOS2Info(&info,d->sf,n);
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
}

static int GFI_AspectChange(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_radiochanged ) {
	struct gfi_data *d = GDrawGetUserData(GGadgetGetWindow(g));
	int new_aspect = GTabSetGetSel(g);
	if ( !d->ttf_set && ( new_aspect == d->ttfv_aspect || new_aspect == d->panose_aspect ))
	    TTFSetup(d);
	else if ( !d->names_set && new_aspect == d->tn_aspect )
	    DefaultLanguage(d);
	d->old_aspect = new_aspect;
    }
return( true );
}

static int e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	struct gfi_data *d = GDrawGetUserData(gw);
	d->done = true;
    }
return( event->type!=et_char );
}

void FontMenuFontInfo(void *_fv) {
    FontView *fv = (FontView *) _fv;
    SplineFont *sf = fv->sf;
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GTabInfo aspects[8];
    GGadgetCreateData mgcd[10], ngcd[9], egcd[9], psgcd[14], tngcd[7],   pgcd[8], vgcd[10], pangcd[22];
    GTextInfo mlabel[10], nlabel[9], elabel[9], pslabel[14], tnlabel[7], plabel[8], vlabel[10], panlabel[22], *list;
    struct gfi_data d;
    char iabuf[20], upbuf[20], uwbuf[20], asbuf[20], dsbuf[20], ncbuf[20];
    int i;
    int oldcnt = sf->charcnt;
    Encoding *item;

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.window_title = GStringGetResource(_STR_Fontinformation,NULL);
    pos.x = pos.y = 0;
    pos.width =GDrawPointsToPixels(NULL,268);
    pos.height = GDrawPointsToPixels(NULL,330);
    gw = GDrawCreateTopWindow(NULL,&pos,e_h,&d,&wattrs);

    memset(&d,'\0',sizeof(d));
    d.sf = sf;
    d.gw = gw;
    d.old_sel = -2;

    memset(&nlabel,0,sizeof(nlabel));
    memset(&ngcd,0,sizeof(ngcd));

    nlabel[0].text = (unichar_t *) _STR_Familyname;
    nlabel[0].text_in_resource = true;
    ngcd[0].gd.label = &nlabel[0];
    ngcd[0].gd.mnemonic = 'F';
    ngcd[0].gd.pos.x = 12; ngcd[0].gd.pos.y = 6; 
    ngcd[0].gd.flags = gg_visible | gg_enabled;
    ngcd[0].creator = GLabelCreate;

    ngcd[1].gd.pos.x = 12; ngcd[1].gd.pos.y = ngcd[0].gd.pos.y+15; ngcd[1].gd.pos.width = 120;
    ngcd[1].gd.flags = gg_visible | gg_enabled;
    nlabel[1].text = (unichar_t *) (sf->familyname);
    nlabel[1].text_is_1byte = true;
    ngcd[1].gd.label = &nlabel[1];
    ngcd[1].gd.cid = CID_Family;
    ngcd[1].gd.handle_controlevent = GFI_NameChange;
    ngcd[1].creator = GTextFieldCreate;

    nlabel[2].text = (unichar_t *) _STR_Fontmodifiers;
    nlabel[2].text_in_resource = true;
    ngcd[2].gd.label = &nlabel[2];
    ngcd[2].gd.mnemonic = 'M';
    ngcd[2].gd.pos.x = 133; ngcd[2].gd.pos.y = ngcd[0].gd.pos.y; 
    ngcd[2].gd.flags = gg_visible | gg_enabled;
    ngcd[2].creator = GLabelCreate;

    ngcd[3].gd.pos.x = 133; ngcd[3].gd.pos.y = ngcd[1].gd.pos.y; ngcd[3].gd.pos.width = 120;
    ngcd[3].gd.flags = gg_visible | gg_enabled;
    nlabel[3].text = (unichar_t *) SFGetModifiers(sf);
    nlabel[3].text_is_1byte = true;
    ngcd[3].gd.label = &nlabel[3];
    ngcd[3].gd.cid = CID_Modifiers;
    ngcd[3].gd.handle_controlevent = GFI_NameChange;
    ngcd[3].creator = GTextFieldCreate;

    ngcd[4].gd.pos.x = 12; ngcd[4].gd.pos.y = ngcd[1].gd.pos.y+26+6;
    nlabel[4].text = (unichar_t *) _STR_Humanname;
    nlabel[4].text_in_resource = true;
    ngcd[4].gd.label = &nlabel[4];
    ngcd[4].gd.mnemonic = 'H';
    ngcd[4].gd.flags = gg_visible | gg_enabled;
    ngcd[4].creator = GLabelCreate;

    ngcd[5].gd.pos.x = 105; ngcd[5].gd.pos.y = ngcd[4].gd.pos.y-6; ngcd[5].gd.pos.width = 147;
    ngcd[5].gd.flags = gg_visible | gg_enabled;
    nlabel[5].text = (unichar_t *) sf->fullname;
    nlabel[5].text_is_1byte = true;
    ngcd[5].gd.label = &nlabel[5];
    ngcd[5].gd.cid = CID_Human;
    ngcd[5].creator = GTextFieldCreate;

    ngcd[6].gd.pos.x = 12; ngcd[6].gd.pos.y = ngcd[5].gd.pos.y+26+6;
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
    egcd[1].gd.label = EncodingTypesFindEnc(list,sf->encoding_name);
    if ( egcd[1].gd.label==NULL ) egcd[1].gd.label = &list[0];
    egcd[1].gd.cid = CID_Encoding;
    egcd[1].creator = GListButtonCreate;
    for ( i=0; list[i].text!=NULL || list[i].line; ++i ) {
	if ( (void *) (sf->encoding_name)==list[i].userdata &&
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
    if ( sf->encoding_name==em_none || sf->charcnt>1500 ) egcd[4].gd.flags |= gg_enabled;
    elabel[4].text = (unichar_t *) _STR_Makefromfont;
    elabel[4].text_in_resource = true;
    egcd[4].gd.mnemonic = 'k';
    egcd[4].gd.label = &elabel[4];
    egcd[4].gd.handle_controlevent = GFI_Make;
    egcd[4].gd.cid = CID_Make;
    egcd[4].creator = GButtonCreate;

    egcd[5].gd.pos.x = 254-12-GIntGetResource(_NUM_Buttonsize);
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
    egcd[6].gd.mnemonic = 'N';
    elabel[6].text = (unichar_t *) _STR_Numchars;
    elabel[6].text_in_resource = true;
    egcd[6].gd.label = &elabel[6];
    egcd[6].creator = GLabelCreate;

    egcd[7].gd.pos.x = 123; egcd[7].gd.pos.y = egcd[6].gd.pos.y-6; egcd[7].gd.pos.width = 60;
    egcd[7].gd.flags = gg_visible | gg_enabled;
    sprintf( ncbuf, "%d", sf->charcnt );
    elabel[7].text = (unichar_t *) ncbuf;
    elabel[7].text_is_1byte = true;
    egcd[7].gd.label = &elabel[7];
    egcd[7].gd.cid = CID_NChars;
    egcd[7].creator = GTextFieldCreate;
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

    psgcd[1].gd.pos.x = 103; psgcd[1].gd.pos.y = psgcd[0].gd.pos.y-6; psgcd[1].gd.pos.width = 45;
    psgcd[1].gd.flags = gg_visible | gg_enabled;
    sprintf( asbuf, "%d", sf->ascent );
    pslabel[1].text = (unichar_t *) asbuf;
    pslabel[1].text_is_1byte = true;
    psgcd[1].gd.label = &pslabel[1];
    psgcd[1].gd.cid = CID_Ascent;
    psgcd[1].creator = GTextFieldCreate;

    psgcd[2].gd.pos.x = 155; psgcd[2].gd.pos.y = psgcd[0].gd.pos.y;
    psgcd[2].gd.flags = gg_visible | gg_enabled;
    psgcd[2].gd.mnemonic = 'D';
    pslabel[2].text = (unichar_t *) _STR_Descent;
    pslabel[2].text_in_resource = true;
    psgcd[2].gd.label = &pslabel[2];
    psgcd[2].creator = GLabelCreate;

    psgcd[3].gd.pos.x = 200; psgcd[3].gd.pos.y = psgcd[1].gd.pos.y; psgcd[3].gd.pos.width = 45;
    psgcd[3].gd.flags = gg_visible | gg_enabled;
    sprintf( dsbuf, "%d", sf->descent );
    pslabel[3].text = (unichar_t *) dsbuf;
    pslabel[3].text_is_1byte = true;
    psgcd[3].gd.label = &pslabel[3];
    psgcd[3].gd.cid = CID_Descent;
    psgcd[3].creator = GTextFieldCreate;

    psgcd[4].gd.pos.x = 12; psgcd[4].gd.pos.y = psgcd[25].gd.pos.y+32+6;
    psgcd[4].gd.flags = gg_visible | gg_enabled;
    psgcd[4].gd.mnemonic = 'I';
    pslabel[4].text = (unichar_t *) _STR_Italicangle;
    pslabel[4].text_in_resource = true;
    psgcd[4].gd.label = &pslabel[4];
    psgcd[4].creator = GLabelCreate;

    psgcd[5].gd.pos.x = 103; psgcd[5].gd.pos.y = psgcd[4].gd.pos.y-6;
    psgcd[5].gd.pos.width = 45;
    psgcd[5].gd.flags = gg_visible | gg_enabled;
    sprintf( iabuf, "%g", sf->italicangle );
    pslabel[5].text = (unichar_t *) iabuf;
    pslabel[5].text_is_1byte = true;
    psgcd[5].gd.label = &pslabel[5];
    psgcd[5].gd.cid = CID_ItalicAngle;
    psgcd[5].creator = GTextFieldCreate;

    psgcd[6].gd.pos.y = psgcd[5].gd.pos.y;
    psgcd[6].gd.pos.width = GIntGetResource(_NUM_Buttonsize); psgcd[6].gd.pos.height = 0;
    psgcd[6].gd.pos.x = psgcd[3].gd.pos.x+psgcd[3].gd.pos.width-psgcd[6].gd.pos.width;
    /*if ( strstrmatch(sf->fontname,"Italic")!=NULL ||strstrmatch(sf->fontname,"Oblique")!=NULL )*/
	psgcd[6].gd.flags = gg_visible | gg_enabled;
    pslabel[6].text = (unichar_t *) _STR_Guess;
    pslabel[6].text_in_resource = true;
    psgcd[6].gd.label = &pslabel[6];
    psgcd[6].gd.mnemonic = 'G';
    psgcd[6].gd.handle_controlevent = GFI_GuessItalic;
    psgcd[6].creator = GButtonCreate;

    psgcd[7].gd.pos.x = 12; psgcd[7].gd.pos.y = psgcd[5].gd.pos.y+26+6;
    psgcd[7].gd.flags = gg_visible | gg_enabled;
    psgcd[7].gd.mnemonic = 'P';
    pslabel[7].text = (unichar_t *) _STR_Upos;
    pslabel[7].text_in_resource = true;
    psgcd[7].gd.label = &pslabel[7];
    psgcd[7].creator = GLabelCreate;

    psgcd[8].gd.pos.x = 103; psgcd[8].gd.pos.y = psgcd[7].gd.pos.y-6; psgcd[8].gd.pos.width = 45;
    psgcd[8].gd.flags = gg_visible | gg_enabled;
    sprintf( upbuf, "%g", sf->upos );
    pslabel[8].text = (unichar_t *) upbuf;
    pslabel[8].text_is_1byte = true;
    psgcd[8].gd.label = &pslabel[8];
    psgcd[8].gd.cid = CID_UPos;
    psgcd[8].creator = GTextFieldCreate;

    psgcd[9].gd.pos.x = 155; psgcd[9].gd.pos.y = psgcd[7].gd.pos.y;
    psgcd[9].gd.flags = gg_visible | gg_enabled;
    psgcd[9].gd.mnemonic = 'H';
    pslabel[9].text = (unichar_t *) _STR_Uheight;
    pslabel[9].text_in_resource = true;
    psgcd[9].gd.label = &pslabel[9];
    psgcd[9].creator = GLabelCreate;

    psgcd[10].gd.pos.x = 200; psgcd[10].gd.pos.y = psgcd[8].gd.pos.y; psgcd[10].gd.pos.width = 45;
    psgcd[10].gd.flags = gg_visible | gg_enabled;
    sprintf( uwbuf, "%g", sf->uwidth );
    pslabel[10].text = (unichar_t *) uwbuf;
    pslabel[10].text_is_1byte = true;
    psgcd[10].gd.label = &pslabel[10];
    psgcd[10].gd.cid = CID_UWidth;
    psgcd[10].creator = GTextFieldCreate;

    psgcd[11].gd.pos.x = 12; psgcd[11].gd.pos.y = psgcd[10].gd.pos.y+26+6;
    psgcd[11].gd.flags = gg_visible | gg_enabled;
    psgcd[11].gd.mnemonic = 'x';
    pslabel[11].text = (unichar_t *) _STR_Xuid;
    pslabel[11].text_in_resource = true;
    psgcd[11].gd.label = &pslabel[11];
    psgcd[11].creator = GLabelCreate;

    psgcd[12].gd.pos.x = 103; psgcd[12].gd.pos.y = psgcd[11].gd.pos.y-6; psgcd[12].gd.pos.width = 142;
    psgcd[12].gd.flags = gg_visible | gg_enabled;
    if ( sf->xuid!=NULL ) {
	pslabel[12].text = (unichar_t *) sf->xuid;
	pslabel[12].text_is_1byte = true;
	psgcd[12].gd.label = &pslabel[12];
    }
    psgcd[12].gd.cid = CID_XUID;
    psgcd[12].creator = GTextFieldCreate;
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
    pgcd[1].gd.pos.width = pgcd[0].gd.pos.width; pgcd[1].gd.pos.height = 6*12+10;
    pgcd[1].gd.flags = gg_visible | gg_enabled;
    pgcd[1].gd.cid = CID_PrivateValues;
    pgcd[1].creator = GTextAreaCreate;

    pgcd[2].gd.pos.x = 20; pgcd[2].gd.pos.y = 275-35-30;
    pgcd[2].gd.pos.width = -1; pgcd[2].gd.pos.height = 0;
    pgcd[2].gd.flags = gg_visible | gg_enabled ;
    plabel[2].text = (unichar_t *) _STR_Add;
    plabel[2].text_in_resource = true;
    pgcd[2].gd.mnemonic = 'A';
    pgcd[2].gd.label = &plabel[2];
    pgcd[2].gd.handle_controlevent = PI_Add;
    pgcd[2].gd.cid = CID_Add;
    pgcd[2].creator = GButtonCreate;

    pgcd[3].gd.pos.x = (240-GIntGetResource(_NUM_Buttonsize))/2; pgcd[3].gd.pos.y = 275-35-30;
    pgcd[3].gd.pos.width = -1; pgcd[3].gd.pos.height = 0;
    pgcd[3].gd.flags = gg_visible ;
    plabel[3].text = (unichar_t *) _STR_Guess;
    plabel[3].text_in_resource = true;
    pgcd[3].gd.label = &plabel[3];
    pgcd[3].gd.mnemonic = 'G';
    pgcd[3].gd.handle_controlevent = PI_Guess;
    pgcd[3].gd.cid = CID_Guess;
    pgcd[3].creator = GButtonCreate;

    pgcd[4].gd.pos.x = 240-GIntGetResource(_NUM_Buttonsize)-20; pgcd[4].gd.pos.y = 275-35-30;
    pgcd[4].gd.pos.width = -1; pgcd[4].gd.pos.height = 0;
    pgcd[4].gd.flags = gg_visible | gg_enabled ;
    plabel[4].text = (unichar_t *) _STR_Remove;
    plabel[4].text_in_resource = true;
    pgcd[4].gd.label = &plabel[4];
    pgcd[4].gd.mnemonic = 'R';
    pgcd[4].gd.handle_controlevent = PI_Delete;
    pgcd[4].gd.cid = CID_Remove;
    pgcd[4].creator = GButtonCreate;
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
    tngcd[2].gd.flags = gg_visible | gg_enabled;
    tngcd[2].gd.cid = CID_Language;
    tngcd[2].gd.u.list = mslanguages;
    tngcd[2].gd.handle_controlevent = GFI_LanguageChange;
    tngcd[2].creator = GListButtonCreate;

    tngcd[3].gd.pos.x = 12; tngcd[3].gd.pos.y = GDrawPointsToPixels(NULL,tngcd[2].gd.pos.y+12);
    tngcd[3].gd.pos.width = pos.width-40; tngcd[3].gd.pos.height = GDrawPointsToPixels(NULL,183);
    tngcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    tngcd[3].creator = GGroupCreate;

    tngcd[4].gd.pos.x = 14; tngcd[4].gd.pos.y = tngcd[2].gd.pos.y+30;
    tngcd[4].gd.pos.width = 220; tngcd[4].gd.pos.height = 160;
    tngcd[4].gd.flags = gg_visible | gg_enabled | gg_textarea_wrap;
    tngcd[4].gd.cid = CID_String;
    tngcd[4].creator = GTextAreaCreate;

    tngcd[5].gd.pos.x = 8; tngcd[5].gd.pos.y = GDrawPointsToPixels(NULL,tngcd[0].gd.pos.y+12);
    tngcd[5].gd.pos.width = pos.width-32; tngcd[5].gd.pos.height = GDrawPointsToPixels(NULL,220);
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

    pangcd[17].gd.pos.x = 55; pangcd[17].gd.pos.y = pangcd[16].gd.pos.y-6; pangcd[17].gd.pos.width = 60;
    pangcd[17].gd.flags = gg_visible | gg_enabled;
    pangcd[17].gd.cid = CID_PanMidLine;
    pangcd[17].gd.u.list = panmidline;
    pangcd[17].creator = GListButtonCreate;

    pangcd[18].gd.pos.x = 123; pangcd[18].gd.pos.y = pangcd[16].gd.pos.y;
    panlabel[18].text = (unichar_t *) _STR_XHeight;
    panlabel[18].text_in_resource = true;
    pangcd[18].gd.label = &panlabel[18];
    pangcd[18].gd.flags = gg_visible | gg_enabled;
    pangcd[18].creator = GLabelCreate;

    pangcd[19].gd.pos.x = 175; pangcd[19].gd.pos.y = pangcd[18].gd.pos.y-6; pangcd[19].gd.pos.width = 60;
    pangcd[19].gd.flags = gg_visible | gg_enabled;
    pangcd[19].gd.cid = CID_PanXHeight;
    pangcd[19].gd.u.list = panxheight;
    pangcd[19].creator = GListButtonCreate;
/******************************************************************************/

    memset(&mlabel,0,sizeof(mlabel));
    memset(&mgcd,0,sizeof(mgcd));
    memset(&aspects,'\0',sizeof(aspects));

    i = 0;

    aspects[i].text = (unichar_t *) _STR_Names;
    aspects[i].selected = true;
    d.old_aspect = 0;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = ngcd;

    aspects[i].text = (unichar_t *) _STR_Encoding2;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = egcd;

    aspects[i].text = (unichar_t *) _STR_PSGeneral;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = psgcd;

    d.private_aspect = i;
    aspects[i].text = (unichar_t *) _STR_PSPrivate;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = pgcd;

    d.ttfv_aspect = i;
    aspects[i].text = (unichar_t *) _STR_TTFValues;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = vgcd;

    d.tn_aspect = i;
    aspects[i].text = (unichar_t *) _STR_TTFNames;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = tngcd;

    d.panose_aspect = i;
    aspects[i].text = (unichar_t *) _STR_Panose;
    aspects[i].text_in_resource = true;
    aspects[i++].gcd = pangcd;

    mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 4;
    mgcd[0].gd.pos.width = 260;
    mgcd[0].gd.pos.height = 280;
    mgcd[0].gd.u.tabs = aspects;
    mgcd[0].gd.flags = gg_visible | gg_enabled;
    mgcd[0].gd.handle_controlevent = GFI_AspectChange;
    mgcd[0].creator = GTabSetCreate;

    mgcd[1].gd.pos.x = 30-3; mgcd[1].gd.pos.y = 330-35-3;
    mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
    mgcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default;
    mlabel[1].text = (unichar_t *) _STR_OK;
    mlabel[1].text_in_resource = true;
    mgcd[1].gd.label = &mlabel[1];
    mgcd[1].gd.handle_controlevent = GFI_OK;
    mgcd[1].creator = GButtonCreate;

    mgcd[2].gd.pos.x = 268-GIntGetResource(_NUM_Buttonsize)-30; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y+3;
    mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
    mgcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    mlabel[2].text = (unichar_t *) _STR_Cancel;
    mlabel[2].text_in_resource = true;
    mgcd[2].gd.label = &mlabel[2];
    mgcd[2].gd.handle_controlevent = GFI_Cancel;
    mgcd[2].creator = GButtonCreate;

    mgcd[3].gd.pos.x = 2; mgcd[3].gd.pos.y = 2;
    mgcd[3].gd.pos.width = pos.width-4; mgcd[3].gd.pos.height = pos.height-2;
    mgcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    mgcd[3].creator = GGroupCreate;

    GGadgetsCreate(gw,mgcd);
    if ( list!=encodingtypes )
	GTextInfoListFree(list);
    GWidgetIndicateFocusGadget(ngcd[1].ret);
    ProcessListSel(&d);
    /*GTextFieldSelect(gcd[1].ret,0,-1);*/

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !d.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    PSDictFree(d.private);
    for ( i=0; i<ttf_namemax; ++i )
	free(d.def.names[i]);
    TTFLangNamesFree(d.names);

    if ( oldcnt!=sf->charcnt && fv!=NULL ) {
	free(fv->selected);
	fv->selected = gcalloc(sf->charcnt,sizeof(char));
    }
    if ( fv!=NULL )
	GDrawRequestExpose(fv->v,NULL,false);
}
