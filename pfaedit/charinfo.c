/* Copyright (C) 2000-2003 by George Williams */
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
#include <math.h>
#include <utype.h>
#include <gkeysym.h>
#include <chardata.h>

typedef struct charinfo {
    CharView *cv;
    SplineChar *sc;
    SplineChar *oldsc;		/* oldsc->charinfo will point to us. Used to keep track of that pointer */
    GWindow gw;
    int done, first, changed;
} CharInfo;

#define CI_Width	218
#define CI_Height	272

#define CID_UName	1001
#define CID_UValue	1002
#define CID_UChar	1003
#define CID_Cancel	1005
#define CID_ComponentMsg	1006
#define CID_Components	1007
#define CID_Comment	1008
#define CID_Color	1009
#define CID_Script	1010
#define CID_Tabs	1011

/* Offsets for repeated fields. add 100*index */
#define CID_List	1020
#define CID_New		1021
#define CID_Delete	1022
#define CID_Edit	1023
#define CID_Copy	1024
#define CID_Paste	1025

static GTextInfo std_colors[] = {
    { (unichar_t *) _STR_Default, &def_image, 0, 0, (void *) COLOR_DEFAULT, NULL, false, true, false, false, false, false, false, true },
    { NULL, &white_image, 0, 0, (void *) 0xffffff, NULL, false, true },
    { NULL, &red_image, 0, 0, (void *) 0xff0000, NULL, false, true },
    { NULL, &green_image, 0, 0, (void *) 0x00ff00, NULL, false, true },
    { NULL, &blue_image, 0, 0, (void *) 0x0000ff, NULL, false, true },
    { NULL, &yellow_image, 0, 0, (void *) 0xffff00, NULL, false, true },
    { NULL, &cyan_image, 0, 0, (void *) 0x00ffff, NULL, false, true },
    { NULL, &magenta_image, 0, 0, (void *) 0xff00ff, NULL, false, true },
    { NULL, NULL }
};

static GTextInfo scripts[] = {
    { (unichar_t *) _STR_Arab, NULL, 0, 0, (void *) CHR('a','r','a','b'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Armn, NULL, 0, 0, (void *) CHR('a','r','m','n'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Beng, NULL, 0, 0, (void *) CHR('b','e','n','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Bopo, NULL, 0, 0, (void *) CHR('b','o','p','o'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Brai, NULL, 0, 0, (void *) CHR('b','r','a','i'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Byzm, NULL, 0, 0, (void *) CHR('b','y','z','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Cans, NULL, 0, 0, (void *) CHR('c','a','n','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Cher, NULL, 0, 0, (void *) CHR('c','h','e','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Hani, NULL, 0, 0, (void *) CHR('h','a','n','i'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Cyrl, NULL, 0, 0, (void *) CHR('c','y','r','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_DFLT, NULL, 0, 0, (void *) CHR('D','F','L','T'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Deva, NULL, 0, 0, (void *) CHR('d','e','v','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Ethi, NULL, 0, 0, (void *) CHR('e','t','h','i'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Geor, NULL, 0, 0, (void *) CHR('g','e','o','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Grek, NULL, 0, 0, (void *) CHR('g','r','e','k'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Gujr, NULL, 0, 0, (void *) CHR('g','u','j','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Guru, NULL, 0, 0, (void *) CHR('g','u','r','u'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Jamo, NULL, 0, 0, (void *) CHR('j','a','m','o'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Hang, NULL, 0, 0, (void *) CHR('h','a','n','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Hebr, NULL, 0, 0, (void *) CHR('h','e','b','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Kana, NULL, 0, 0, (void *) CHR('k','a','n','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Knda, NULL, 0, 0, (void *) CHR('k','n','d','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Khmr, NULL, 0, 0, (void *) CHR('k','h','m','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Lao , NULL, 0, 0, (void *) CHR('l','a','o',' '), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Latn, NULL, 0, 0, (void *) CHR('l','a','t','n'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Mlym, NULL, 0, 0, (void *) CHR('m','l','y','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Mong, NULL, 0, 0, (void *) CHR('m','o','n','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Mymr, NULL, 0, 0, (void *) CHR('m','y','m','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Ogam, NULL, 0, 0, (void *) CHR('o','g','a','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Orya, NULL, 0, 0, (void *) CHR('o','r','y','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Runr, NULL, 0, 0, (void *) CHR('r','u','n','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Sinh, NULL, 0, 0, (void *) CHR('s','i','n','h'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Syrc, NULL, 0, 0, (void *) CHR('s','y','r','c'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Taml, NULL, 0, 0, (void *) CHR('t','a','m','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Telu, NULL, 0, 0, (void *) CHR('t','e','l','u'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Thaa, NULL, 0, 0, (void *) CHR('t','h','a','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Thai, NULL, 0, 0, (void *) CHR('t','h','a','i'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Tibt, NULL, 0, 0, (void *) CHR('t','i','b','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Yi  , NULL, 0, 0, (void *) CHR('y','i',' ',' '), NULL, false, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo ligature_tags[] = {
    { (unichar_t *) _STR_Discretionary, NULL, 0, 0, (void *) CHR('d','l','i','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Historic, NULL, 0, 0, (void *) CHR('h','l','i','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Required, NULL, 0, 0, (void *) CHR('r','l','i','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Standard, NULL, 0, 0, (void *) CHR('l','i','g','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Fraction, NULL, 0, 0, (void *) CHR('f','r','a','c'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AltFrac, NULL, 0, 0, (void *) CHR('a','f','r','c'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AboveBaseSubs, NULL, 0, 0, (void *) CHR('a','b','v','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_BelowBaseForms, NULL, 0, 0, (void *) CHR('b','l','w','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_BelowBaseSubs, NULL, 0, 0, (void *) CHR('b','l','w','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Akhand, NULL, 0, 0, (void *) CHR('a','k','h','n'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_GlyphCompDecomp, NULL, 0, 0, (void *) CHR('c','c','m','p'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_HalfForm, NULL, 0, 0, (void *) CHR('h','a','l','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_HalantForm, NULL, 0, 0, (void *) CHR('h','a','l','n'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_LeadingJamo, NULL, 0, 0, (void *) CHR('l','j','m','o'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_TrailingJamo, NULL, 0, 0, (void *) CHR('t','j','m','o'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_VowelJamo, NULL, 0, 0, (void *) CHR('v','j','m','o'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Nukta, NULL, 0, 0, (void *) CHR('n','u','k','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PreBaseForms, NULL, 0, 0, (void *) CHR('p','r','e','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PreBaseSubs, NULL, 0, 0, (void *) CHR('p','r','e','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PostBaseForms, NULL, 0, 0, (void *) CHR('p','s','t','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PostBaseSubs, NULL, 0, 0, (void *) CHR('p','s','t','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Reph, NULL, 0, 0, (void *) CHR('r','e','p','h'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_VattuVariants, NULL, 0, 0, (void *) CHR('v','a','t','u'), NULL, false, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo simplepos_tags[] = {
    { (unichar_t *) _STR_CaseSensForms, NULL, 0, 0, (void *) CHR('c','a','s','e'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_CapitalSpacing, NULL, 0, 0, (void *) CHR('c','p','s','p'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_FullWidths, NULL, 0, 0, (void *) CHR('f','w','i','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AltHalfWidths, NULL, 0, 0, (void *) CHR('h','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_HalfWidths, NULL, 0, 0, (void *) CHR('h','w','i','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_LeftBounds, NULL, 0, 0, (void *) CHR('l','f','b','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_OpticalBounds, NULL, 0, 0, (void *) CHR('o','p','b','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PropAltMet, NULL, 0, 0, (void *) CHR('p','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_QuartWidths, NULL, 0, 0, (void *) CHR('q','w','i','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_RightBounds, NULL, 0, 0, (void *) CHR('r','t','b','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_ThirdWidths, NULL, 0, 0, (void *) CHR('t','w','i','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AltVertMet, NULL, 0, 0, (void *) CHR('v','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AltVertHalfMet, NULL, 0, 0, (void *) CHR('v','h','a','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AltVertPropMet, NULL, 0, 0, (void *) CHR('v','p','a','l'), NULL, false, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo simplesubs_tags[] = {
    { (unichar_t *) _STR_AllAlt, NULL, 0, 0, (void *) CHR('a','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AboveBaseForms, NULL, 0, 0, (void *) CHR('a','b','v','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_CaseSensForms, NULL, 0, 0, (void *) CHR('c','a','s','e'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Cap2PetiteCaps, NULL, 0, 0, (void *) CHR('c','2','p','c'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Cap2SmallCaps, NULL, 0, 0, (void *) CHR('c','2','s','c'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Denominators, NULL, 0, 0, (void *) CHR('d','n','o','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_ExpertForms, NULL, 0, 0, (void *) CHR('e','x','p','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_TerminalForms, NULL, 0, 0, (void *) CHR('f','i','n','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_FullWidths, NULL, 0, 0, (void *) CHR('f','w','i','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_HistoricalForms, NULL, 0, 0, (void *) CHR('h','i','s','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Historic, NULL, 0, 0, (void *) CHR('h','l','i','g'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_HorKanaAlt, NULL, 0, 0, (void *) CHR('h','k','n','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Hanja2Hangul, NULL, 0, 0, (void *) CHR('h','n','g','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_HalfWidths, NULL, 0, 0, (void *) CHR('h','w','i','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_IsolatedForms, NULL, 0, 0, (void *) CHR('i','s','o','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Italics, NULL, 0, 0, (void *) CHR('i','t','a','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_JIS78Forms, NULL, 0, 0, (void *) CHR('j','p','7','8'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_JIS83Forms, NULL, 0, 0, (void *) CHR('j','p','8','3'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_JIS90Forms, NULL, 0, 0, (void *) CHR('j','p','9','0'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_LiningFigures, NULL, 0, 0, (void *) CHR('l','n','u','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_LocalizedForms, NULL, 0, 0, (void *) CHR('l','o','c','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_MedialForms, NULL, 0, 0, (void *) CHR('m','e','d','i'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_MathematicalGreek, NULL, 0, 0, (void *) CHR('m','g','r','k'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AltAnnotForms, NULL, 0, 0, (void *) CHR('n','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Numerators, NULL, 0, 0, (void *) CHR('n','u','m','r'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_OldstyleFigures, NULL, 0, 0, (void *) CHR('o','n','u','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Ornaments, NULL, 0, 0, (void *) CHR('o','r','n','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PetiteCaps, NULL, 0, 0, (void *) CHR('p','c','a','p'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_PropWidth, NULL, 0, 0, (void *) CHR('p','w','i','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_QuartWidths, NULL, 0, 0, (void *) CHR('q','w','i','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_R2LAlt, NULL, 0, 0, (void *) CHR('r','t','l','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_RubyNoteForms, NULL, 0, 0, (void *) CHR('r','u','b','y'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_StylisticAlt, NULL, 0, 0, (void *) CHR('s','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_ScientificInf, NULL, 0, 0, (void *) CHR('s','i','n','f'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_SmallCaps, NULL, 0, 0, (void *) CHR('s','m','c','p'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_SimplifiedForms, NULL, 0, 0, (void *) CHR('s','m','p','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_StyleSet1, NULL, 0, 0, (void *) CHR('s','s','0','1'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_StyleSet2, NULL, 0, 0, (void *) CHR('s','s','0','2'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_StyleSet3, NULL, 0, 0, (void *) CHR('s','s','0','3'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_StyleSet4, NULL, 0, 0, (void *) CHR('s','s','0','4'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_StyleSet5, NULL, 0, 0, (void *) CHR('s','s','0','5'), NULL, false, false, false, false, false, false, false, true },
	/* These are documented to go up to Style Set 20, but I'm not going to bother will all of them until someone cares */
    { (unichar_t *) _STR_Subscript, NULL, 0, 0, (void *) CHR('s','u','b','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Superscript, NULL, 0, 0, (void *) CHR('s','u','p','s'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Swash, NULL, 0, 0, (void *) CHR('s','w','s','h'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Titling, NULL, 0, 0, (void *) CHR('t','i','t','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_TraditionalNameForms, NULL, 0, 0, (void *) CHR('t','n','a','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_TabNumb, NULL, 0, 0, (void *) CHR('t','n','u','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_TraditionalForms, NULL, 0, 0, (void *) CHR('t','r','a','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_ThirdWidths, NULL, 0, 0, (void *) CHR('t','w','i','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Unicase, NULL, 0, 0, (void *) CHR('u','n','i','c'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_VerticalRot, NULL, 0, 0, (void *) CHR('v','e','r','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_VertRotAlt, NULL, 0, 0, (void *) CHR('v','r','t','2'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_VertKana, NULL, 0, 0, (void *) CHR('v','k','n','a'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_SlashedZero, NULL, 0, 0, (void *) CHR('z','e','r','o'), NULL, false, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo alternatesubs_tags[] = {
    { (unichar_t *) _STR_AllAlt, NULL, 0, 0, (void *) CHR('a','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_FinalGlyphLine, NULL, 0, 0, (void *) CHR('f','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Hanja2Hangul, NULL, 0, 0, (void *) CHR('h','n','g','l'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_JustificationAlt, NULL, 0, 0, (void *) CHR('j','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_JIS78Forms, NULL, 0, 0, (void *) CHR('j','p','7','8'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_AltAnnotForms, NULL, 0, 0, (void *) CHR('n','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Ornaments, NULL, 0, 0, (void *) CHR('o','r','n','m'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Randomize, NULL, 0, 0, (void *) CHR('r','a','n','d'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_StylisticAlt, NULL, 0, 0, (void *) CHR('s','a','l','t'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_Swash, NULL, 0, 0, (void *) CHR('s','w','s','h'), NULL, false, false, false, false, false, false, false, true },
    { (unichar_t *) _STR_TraditionalForms, NULL, 0, 0, (void *) CHR('t','r','a','d'), NULL, false, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo multiplesubs_tags[] = {
    { (unichar_t *) _STR_GlyphCompDecomp, NULL, 0, 0, (void *) CHR('c','c','m','p'), NULL, false, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo *pst_tags[] = {
    simplepos_tags, simplesubs_tags, alternatesubs_tags, multiplesubs_tags,
    ligature_tags
};
static int newstrings[] = { _STR_NewPosition, _STR_NewSubstitution,
	_STR_NewAlternate, _STR_NewMultiple, _STR_NewLigature };
static int editstrings[] = { _STR_EditPosition, _STR_EditSubstitution,
	_STR_EditAlternate, _STR_EditMultiple, _STR_EditLigature };


struct pt_dlg {
    int done;
    int ok;
};

static int ptd_e_h(GWindow gw, GEvent *event) {
    struct pt_dlg *ptd = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	ptd->done = true;
	ptd->ok = false;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("getinfo.html");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	ptd->done = true;
	ptd->ok = GGadgetGetCid(event->u.control.g);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_textchanged &&
	    event->u.control.u.tf_changed.from_pulldown!=-1 ) {
	uint32 tag = (uint32) simplepos_tags[event->u.control.u.tf_changed.from_pulldown].userdata;
	unichar_t ubuf[8];
	/* If they select something from the pulldown, don't show the human */
	/*  readable form, instead show the 4 character tag */
	ubuf[0] = tag>>24;
	ubuf[1] = (tag>>16)&0xff;
	ubuf[2] = (tag>>8)&0xff;
	ubuf[3] = tag&0xff;
	ubuf[4] = 0;
	GGadgetSetTitle(event->u.control.g,ubuf);
    }
return( true );
}

static unichar_t *AskPosTag(int title,unichar_t *def,uint32 def_tag, GTextInfo *tags) {
    static unichar_t nullstr[] = { 0 };
    struct pt_dlg ptd;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[14];
    GTextInfo label[14];
    GWindow gw;
    unichar_t ubuf[8];
    unichar_t *ret, *pt, *end;
    const unichar_t *utag;
    uint32 tag;
    int dx=0, dy=0, dxa=0, dya=0;
    char buf[100];
    unichar_t udx[12], udy[12], udxa[12], udya[12];
    int i;

    if ( def==NULL ) def=nullstr;
    if ( def_tag==0 && u_strlen(def)>4 && def[4]==' ' && def[0]<0x7f && def[1]<0x7f && def[2]<0x7f && def[3]<0x7f ) {
	if ( def[0]!=' ' )
	    def_tag = (def[0]<<24) | (def[1]<<16) | (def[2]<<8) | def[3];
	def += 5;
    }

    for ( pt=def; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    dx = u_strtol(pt,&end,10);
    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    dy = u_strtol(pt,&end,10);
    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    dxa = u_strtol(pt,&end,10);
    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    dya = u_strtol(pt,&end,10);
    sprintf(buf,"%d",dx);
    uc_strcpy(udx,buf);
    sprintf(buf,"%d",dy);
    uc_strcpy(udy,buf);
    sprintf(buf,"%d",dxa);
    uc_strcpy(udxa,buf);
    sprintf(buf,"%d",dya);
    uc_strcpy(udya,buf);
    

	memset(&ptd,0,sizeof(ptd));
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource( title,NULL );
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,160));
	pos.height = GDrawPointsToPixels(NULL,195);
	gw = GDrawCreateTopWindow(NULL,&pos,ptd_e_h,&ptd,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	i=0;
	label[i].text = (unichar_t *) _STR_Dx;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 5+4;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	label[i].text = udx;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 70; gcd[i].gd.pos.y = 5; gcd[i].gd.pos.width = 50;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = i+1;
	gcd[i++].creator = GTextFieldCreate;

	label[i].text = (unichar_t *) _STR_Dy;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	label[i].text = udy;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26; gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = i+1;
	gcd[i++].creator = GTextFieldCreate;

	label[i].text = (unichar_t *) _STR_Dxa;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	label[i].text = udxa;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26; gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = i+1;
	gcd[i++].creator = GTextFieldCreate;

	label[i].text = (unichar_t *) _STR_Dya;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	label[i].text = udya;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26; gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = i+1;
	gcd[i++].creator = GTextFieldCreate;

	label[i].text = (unichar_t *) _STR_TagC;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	ubuf[0] = def_tag>>24; ubuf[1] = (def_tag>>16)&0xff; ubuf[2] = (def_tag>>8)&0xff; ubuf[3] = def_tag&0xff; ubuf[4] = 0;
	label[i].text = ubuf;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+14;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.list = tags;
	gcd[i].gd.cid = i+1;
	gcd[i++].creator = GListFieldCreate;

	gcd[i].gd.pos.x = 15-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+30;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _STR_OK;
	label[i].text_in_resource = true;
	gcd[i].gd.mnemonic = 'O';
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = true;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _STR_Cancel;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.mnemonic = 'C';
	gcd[i].gd.cid = false;
	gcd[i++].creator = GButtonCreate;

	GGadgetsCreate(gw,gcd);

    GDrawSetVisible(gw,true);
    GWidgetIndicateFocusGadget(gcd[1].ret);
 tryagain:
    while ( !ptd.done )
	GDrawProcessOneEvent(NULL);
    if ( ptd.ok ) {
	int err=false;
	dx = GetIntR(gw,2, _STR_Dx,&err);
	dy = GetIntR(gw,4, _STR_Dy,&err);
	dxa = GetIntR(gw,6, _STR_Dxa,&err);
	dya = GetIntR(gw,8, _STR_Dya,&err);
	utag = _GGadgetGetTitle(gcd[9].ret);
	if ( err )
 goto tryagain;
	if ( (ubuf[0] = utag[0])==0 )
	    tag = 0;
	else {
	    if ( (ubuf[1] = utag[1])==0 )
		ubuf[1] = ubuf[2] = ubuf[3] = ' ';
	    else if ( (ubuf[2] = utag[2])==0 )
		ubuf[2] = ubuf[3] = ' ';
	    else if ( (ubuf[3] = utag[3])==0 )
		ubuf[3] = ' ';
	    tag = (ubuf[0]<<24) | (ubuf[1]<<16) | (ubuf[2]<<8) | ubuf[3];
	}
	if ( u_strlen(utag)>4 || ubuf[0]>=0x7f || ubuf[1]>=0x7f || ubuf[2]>=0x7f || ubuf[3]>=0x7f || tag==0 ) {
	    GWidgetErrorR(_STR_TagTooLong,_STR_FeatureTagTooLong);
	    ptd.done = false;
 goto tryagain;
	}
	sprintf(buf,"%c%c%c%c dx=%d dy=%d dx_adv=%d dy_adv=%d",
		tag>>24, tag>>16, tag>>8, tag,
		dx, dy, dxa, dya );
	ret = uc_copy(buf);
    } else
	ret = NULL;
    GDrawDestroyWindow(gw);
return( ret );
}

static int LigCheck(SplineChar *sc,enum possub_type type,
	uint32 tag, unichar_t *components) {
    int i;
    static int buts[3] = { _STR_OK, _STR_Cancel, 0 };
    unichar_t *pt, *start, ch;
    PST *pst;
    SplineFont *sf = sc->parent;

    if ( components==NULL || *components=='\0' )
return( true );

    if ( type==pst_ligature ) {
	for ( i=0; i<sf->charcnt; ++i )
	    if ( sf->chars[i]!=sc && sf->chars[i]!=NULL && sf->chars[i]->possub!=NULL ) {
		for ( pst=sf->chars[i]->possub; pst!=NULL; pst=pst->next )
			if ( pst->type==pst_ligature && pst->tag==tag &&
			    uc_strcmp(components,pst->u.lig.components)==0 ) {
return( GWidgetAskR(_STR_Multiple,buts,0,1,_STR_AlreadyLigature,sf->chars[i]->name,i)==0 );
		}
	    }
    }

    start = components;
    while ( 1 ) {
	pt = u_strchr(start,' ');
	if ( pt==NULL ) pt = start+u_strlen(start);
	ch = *pt; *pt = '\0';
	if ( uc_strcmp(start,sc->name)==0 ) {
	    GWidgetErrorR(_STR_Badligature,_STR_SelfReferential );
	    *pt = ch;
return( false );
	}
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	    if ( uc_strcmp(start,sf->chars[i]->name)==0 )
	break;
	if ( i==sf->charcnt ) {
	    int ret = GWidgetAskR(_STR_Multiple,buts,0,1,_STR_MissingComponent,start);
	    *pt = ch;
return( ret==0 );
	}
	*pt = ch;
	if ( ch=='\0' )
    break;
	start = pt+1;
	while ( *start==' ' ) ++start;
    }
return( true );
}

static void CI_DoNew(CharInfo *ci, unichar_t *def) {
    int len, i, sel;
    GTextInfo **old, **new;
    GGadget *list;
    unichar_t *newname;

    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
    newname = sel==0 
	    ? AskPosTag(newstrings[sel],def,0,pst_tags[sel])
	    : AskNameTag(newstrings[sel],def,0,pst_tags[sel]);
    if ( newname!=NULL ) {
	if ( sel!=0 )
	    if ( !LigCheck(ci->sc,sel+1,(newname[0]<<24)|(newname[1]<<16)|(newname[2]<<8)|newname[3],
		    newname+5)) {
		free(newname );
return;
	    }
	list = GWidgetGetControl(ci->gw,CID_List+sel*100);
	old = GGadgetGetList(list,&len);
	for ( i=0; i<len; ++i ) {
	    if ( u_strncmp(old[i]->text,newname,4)==0 )
	break;
	}
	if ( i<len && sel+1!=pst_ligature ) {
	    GWidgetErrorR(_STR_DuplicateTag,_STR_DuplicateTag);
	    free(newname);
return;
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

static void CI_Drop(CharInfo *ci, GEvent *e) {
    char *cnames;
    unichar_t *unames;
    int sel;
    int32 len;

    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-1;
    if ( sel<=pst_position || sel>=pst_max ) {
	GDrawBeep(NULL);
return;
    }

    if ( !GDrawSelectionHasType(ci->gw,sn_drag_and_drop,"STRING"))
return;
    cnames = GDrawRequestSelection(ci->gw,sn_drag_and_drop,"STRING",&len);
    if ( cnames==NULL )
return;

    if ( sel==pst_substitution && strchr(cnames,' ')!=NULL ) {
	GWidgetErrorR(_STR_TooManyComponents,_STR_SubsOnlyOne);
	free(cnames);
return;
    }

    unames = galloc((strlen(cnames)+6)*sizeof(unichar_t));
    uc_strcpy(unames, "     ");
    uc_strcpy(unames+5,cnames);
    CI_DoNew(ci,unames);
    free(cnames);
    free(unames);
}

static int CI_New(GGadget *g, GEvent *e) {
    CharInfo *ci;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ci = GDrawGetUserData(GGadgetGetWindow(g));
	CI_DoNew(ci,NULL);
    }
return( true );
}

static int CI_Delete(GGadget *g, GEvent *e) {
    int len, i,j, offset;
    GTextInfo **old, **new;
    GGadget *list;
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	offset = GGadgetGetCid(g)-CID_Delete;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_List+offset);
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
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Delete+offset),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Edit+offset),false);
    }
return( true );
}

static int CI_Edit(GGadget *g, GEvent *e) {
    int len, i;
    GTextInfo **old, **new, *ti;
    GGadget *list;
    CharInfo *ci;
    unichar_t *newname;
    int sel;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ci = GDrawGetUserData(GGadgetGetWindow(g));
	sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_List+sel*100);
	if ( (ti = GGadgetGetListItemSelected(list))==NULL )
return( true );
	newname = sel==0 
		? AskPosTag(editstrings[sel],ti->text,0,pst_tags[sel])
		: AskNameTag(editstrings[sel],ti->text,0,pst_tags[sel]);
	if ( newname!=NULL ) {
	    old = GGadgetGetList(list,&len);
	    for ( i=0; i<len; ++i ) if ( old[i]!=ti ) {
		if ( u_strncmp(old[i]->text,newname,4)==0 )
	    break;
	    }
	    if ( i<len && sel+1!=pst_ligature) {
		GWidgetErrorR(_STR_DuplicateTag,_STR_DuplicateTag);
		free(newname);
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

static int CI_SelChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int sel = GGadgetGetFirstListSelectedItem(g);
	int offset = GGadgetGetCid(g)-CID_List;
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Delete+offset),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Edit+offset),sel!=-1);
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Copy+offset),sel!=-1);
    } else if ( e->type==et_controlevent && e->u.control.subtype == et_listdoubleclick ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int offset = GGadgetGetCid(g)-CID_List;
	e->u.control.subtype = et_buttonactivate;
	e->u.control.g = GWidgetGetControl(ci->gw,CID_Edit+offset);
	CI_Edit(e->u.control.g,e);
    }
return( true );
}

static void CI_CanPaste(CharInfo *ci) {
    int i, canpaste=false;
    enum undotype copytype = CopyUndoType();

    if ( copytype==ut_possub )
	canpaste = true;
    else if ( copytype==ut_none )
	canpaste = GDrawSelectionHasType(ci->gw,sn_clipboard,"STRING");
    for ( i=0; i<5; ++i )
	GGadgetSetEnabled(GWidgetGetControl(ci->gw,CID_Paste+i*100),canpaste);
}

static void CI_DoCopy(CharInfo *ci) {
    GGadget *list;
    int sel, i, len, cnt;
    GTextInfo **tis;
    char **data;

    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
    list = GWidgetGetControl(ci->gw,CID_List+sel*100);
    tis = GGadgetGetList(list,&len);
    for ( i=cnt=0; i<len; ++i )
	if ( tis[i]->selected )
	    ++cnt;
    if ( cnt==0 )
return;
    data = gcalloc(cnt+1,sizeof(char *));
    for ( i=cnt=0; i<len; ++i )
	if ( tis[i]->selected )
	    data[i] = cu_copy(tis[i]->text);
    PosSubCopy(sel+1,data);
    CI_CanPaste(ci);
}

static int CI_Copy(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	CI_DoCopy( GDrawGetUserData(GGadgetGetWindow(g)) );
return( true );
}

static enum possub_type PSTGuess(char *data) {
    enum possub_type type;
    int i;
    uint32 tag;

    tag = (((uint8 *) data)[0]<<24) | (((uint8 *) data)[1]<<16 ) |
		(((uint8 *) data)[2]<<8) | ((uint8 *) data)[3];
    for ( type=pst_position; type<pst_max; ++type ) {
	for ( i=0; pst_tags[type-1][i].text!=NULL; ++i ) {
	    if ( (uint32) pst_tags[type-1][i].userdata==tag )
	break;
	}
	if ( pst_tags[type-1][i].text!=NULL )
    break;
    }
    if ( type==pst_max )
return( pst_null );

return( type );
}

static void CI_DoPaste(CharInfo *ci,char **data, enum possub_type type) {
    GGadget *list;
    int sel, i,j,k, len, cnt;
    uint32 tag;
    GTextInfo **tis, **newlist;
    char *tempdata[2];

    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
    list = GWidgetGetControl(ci->gw,CID_List+sel*100);
    tis = GGadgetGetList(list,&len);
    if ( data==NULL )
	data = CopyGetPosSubData(&type);
    if ( data==NULL ) {
	int plen;
	char *paste = GDrawRequestSelection(ci->gw,sn_clipboard,"STRING",&plen);
	if ( paste==NULL || plen==0 )
return;
	tempdata[0] = paste;
	tempdata[1] = NULL;
	data = tempdata;
	tag = (((uint8 *) paste)[0]<<24) | (((uint8 *) paste)[1]<<16 ) |
		    (((uint8 *) paste)[2]<<8) | ((uint8 *) paste)[3];
	type = pst_null;
	if ( sel+1>pst_null && sel+1<pst_max ) {
	    for ( i=0; pst_tags[sel][i].text!=NULL; ++i )
		if ( (uint32) pst_tags[sel][i].userdata == tag ) {
		    type = sel+1;
	    break;
	    }
	}
	if ( type==pst_null )
	    type = PSTGuess(paste);
	if ( type==pst_null ) {
	    free(paste);
	    GWidgetErrorR(_STR_BadPOSSUB,_STR_BadPOSSUB);
return;
	}
    }

    for ( cnt=0; data[cnt]!=NULL; ++cnt );
    newlist = galloc((len+cnt+1)*sizeof(GTextInfo *));
    for ( i=0; i<len; ++i ) {
	newlist[i] = galloc(sizeof(GTextInfo));
	*newlist[i] = *tis[i];
	newlist[i]->text = u_copy(tis[i]->text);
    }
    k = 0;
    for ( i=0; i<cnt; ++i ) {
	for ( j=0; j<len; ++j )
	    if ( uc_strncmp(newlist[j]->text,data[i],4)==0 )
	break;
	if ( j<len ) {
	    free(newlist[j]->text);
	    newlist[j]->text = uc_copy(data[i]);
	} else {
	    newlist[len+k] = gcalloc(1,sizeof(GTextInfo));
	    newlist[len+k]->fg = newlist[len+k]->bg = COLOR_DEFAULT;
	    newlist[len+k]->text = uc_copy(data[i]);
	    ++k;
	}
    }
    newlist[len+k] = gcalloc(1,sizeof(GTextInfo));
    GGadgetSetList(list,newlist,false);
}

static int CI_Paste(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	CI_DoPaste( GDrawGetUserData(GGadgetGetWindow(g)),NULL,pst_null );
return( true );
}

static int MultipleValues(char *name, int local) {
    int buts[3] = { _STR_OK, _STR_Cancel, 0 };

    if ( GWidgetAskR(_STR_Multiple,buts,0,1,_STR_AlreadyCharUnicode,name,local)==0 )
return( true );

return( false );
}

static int MultipleNames(void) {
    int buts[] = { _STR_OK, _STR_Cancel, 0 };

    if ( GWidgetAskR(_STR_Multiple,buts,0,1,_STR_Alreadycharnamed)==0 )
return( true );

return( false );
}

static int ParseUValue(GWindow gw, int cid, int minusoneok, SplineFont *sf) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    unichar_t *end;
    int val;

    if (( *ret=='U' || *ret=='u' ) && ret[1]=='+' )
	val = u_strtoul(ret+2,&end,16);
    else if ( *ret=='#' )
	val = u_strtoul(ret+1,&end,16);
    else
	val = u_strtoul(ret,&end,16);
    if ( val==-1 && minusoneok )
return( -1 );
    if ( *end || val<0 || val>0x7fffffff ) {
	ProtestR( _STR_UnicodeValue );
return( -2 );
    } else if ( val>65535 && sf->encoding_name!=em_unicode4 &&
	    sf->encoding_name<em_unicodeplanes ) {
	static int buts[] = { _STR_Yes, _STR_No, 0 };
	if ( GWidgetAskR(_STR_PossiblyTooBig,buts,1,1,_STR_NotUnicodeBMP)==1 )
return(-2);
    }
return( val );
}

static void SetNameFromUnicode(GWindow gw,int cid,int val) {
    unichar_t *temp;
    char buf[10];
    const unichar_t *curname = GGadgetGetTitle(GWidgetGetControl(gw,cid));

    if ( val>=0 && val<psunicodenames_cnt && psunicodenames[val]!=NULL )
	temp = uc_copy(psunicodenames[val]);
/* If it a control char is already called ".notdef" then give it a uniXXXX style name */
    else if (( val>=32 && val<0x7f ) || val>=0xa1 ||
	    (uc_strcmp(curname,".notdef")==0 && val!=0)) {
	if ( val>= 0x10000 )
	    sprintf( buf,"u%04X", val );	/* u style names may contain 4,5 or 6 hex digits */
	else
	    sprintf( buf,"uni%04X", val );
	temp = uc_copy(buf);
    } else
	temp = uc_copy(".notdef");
    GGadgetSetTitle(GWidgetGetControl(gw,cid),temp);
    free(temp);
}

static void SetScriptFromTag(GWindow gw,int tag) {
    unichar_t ubuf[8];

    ubuf[0] = tag>>24;
    ubuf[1] = (tag>>16)&0xff;
    ubuf[2] = (tag>>8)&0xff;
    ubuf[3] = tag&0xff;
    ubuf[4] = 0;
    GGadgetSetTitle(GWidgetGetControl(gw,CID_Script),ubuf);
}

static void SetScriptFromUnicode(GWindow gw,int uni,SplineFont *sf) {
    SetScriptFromTag(gw,ScriptFromUnicode(uni,sf));
}

void SCAppendPosSub(SplineChar *sc,enum possub_type type, char **d) {
    PST *new, *old;
    char *pt, *end, *data;
    int i;

    if ( sc->charinfo!=NULL ) {
	CI_DoPaste(sc->charinfo,d,type);
	GDrawRaise(sc->charinfo->gw);
return;
    }

    for ( i=0; d[i]!=NULL; ++i ) {
	data = d[i];
	if ( strlen(data)<6 || data[4]!=' ' ) {
	    GWidgetErrorR(_STR_BadPOSSUB,_STR_BadPOSSUBPaste);
return;
	}
	if ( type==pst_null ) {
	    type = PSTGuess(data);
	    if ( type==pst_null ) {
		GWidgetErrorR(_STR_BadPOSSUB,_STR_BadPOSSUB);
return;
	    }
	}

	new = chunkalloc(sizeof(PST));
	new->type = type;
	new->tag = (((uint8 *) data)[0]<<24) | (((uint8 *) data)[1]<<16 ) |
		    (((uint8 *) data)[2]<<8) | ((uint8 *) data)[3];
	if ( type==pst_position ) {
	    for ( pt=data+5; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
	    new->u.pos.xoff = strtol(pt,&end,10);
	    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
	    new->u.pos.yoff = strtol(pt,&end,10);
	    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
	    new->u.pos.h_adv_off = strtol(pt,&end,10);
	    for ( pt=end; *pt!='\0' && *pt!='='; ++pt );
	    if ( *pt=='=' )
		++pt;
	    else {
		GWidgetErrorR(_STR_BadPOSSUB,_STR_ExpectedEquals);
		chunkfree(new,sizeof(PST));
return;
	    }
	    new->u.pos.v_adv_off = strtol(pt,&end,10);
	} else {
	    if ( type==pst_substitution && strchr(data+5,' ')!=NULL ) {
		GWidgetErrorR(_STR_BadPOSSUB,_STR_SimpleSubsOneComponent);
return;
	    }
	    new->u.subs.variant = copy(data+5);
	    if ( type==pst_ligature )
		new->u.lig.lig = sc;
	}

	for ( old=sc->possub; old!=NULL; old = old->next ) {
	    if ( old->tag==new->tag ) {
		new->next = old->next;
		*old = *new;
		chunkfree(new,sizeof(PST));
	break;
	    }
	}
	if ( old==NULL ) {
	    new->next = sc->possub;
	    sc->possub = new;
	}
    }
    if ( i!=0 )
	SCCharChangedUpdate(sc);
}
		
int SCSetMetaData(SplineChar *sc,char *name,int unienc,const unichar_t *comment) {
    SplineFont *sf = sc->parent;
    int i, mv=0;
    int isnotdef, samename=false;

    if ( sc->unicodeenc == unienc && strcmp(name,sc->name)==0 ) {
	samename = true;	/* No change, it must be good */
    } else {
	isnotdef = strcmp(name,".notdef")==0;
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL && sf->chars[i]!=sc ) {
	    if ( unienc!=-1 && sf->chars[i]->unicodeenc==unienc ) {
		if ( !mv && !MultipleValues(sf->chars[i]->name,i)) {
return( false );
		}
		mv = 1;
	    } else if ( !isnotdef && strcmp(name,sf->chars[i]->name)==0 ) {
		if ( !MultipleNames()) {
return( false );
		}
		free(sf->chars[i]->name);
		sf->chars[i]->name = sc->name;
		sf->chars[i]->namechanged = true;
		sc->name = NULL;
	    break;
	    }
	}
    }
    if ( sc->unicodeenc!=unienc )
	sc->script = ScriptFromUnicode(unienc,sc->parent);
    sc->unicodeenc = unienc;
    if ( strcmp(name,sc->name)!=0 ) {
	free(sc->name);
	sc->name = copy(name);
	sc->namechanged = true;
    }
    sf->changed = true;
    if ( (sf->encoding_name==em_unicode || sf->encoding_name==em_unicode4) &&
	    unienc==sc->enc && unienc>=0xe000 && unienc<=0xf8ff )
	/* Ok to name things in the private use area */;
    else if ( samename )
	/* Ok to name it itself */;
    else if ( (sf->encoding_name<e_first2byte && sc->enc<256) ||
	    (sf->encoding_name>=em_big5 && sf->encoding_name<=em_unicode && sc->enc<65536 ) ||
	    (sf->encoding_name>=e_first2byte && sf->encoding_name<em_unicode && sc->enc<94*96 ) ||
	    sc->unicodeenc!=-1 )
	sf->encoding_name = em_none;

    free(sc->comment); sc->comment = NULL;
    if ( *comment!='\0' )
	sc->comment = u_copy(comment);

    SCRefreshTitles(sc);
return( true );
}

static uint32 ParseTag(GWindow gw,int cid,int msg,int *err) {
    const unichar_t *utag = _GGadgetGetTitle(GWidgetGetControl(gw,cid));
    uint32 tag;
    unichar_t ubuf[8];

    if ( (ubuf[0] = utag[0])==0 ) {
return( 0 );
    } else {
	if ( (ubuf[1] = utag[1])==0 )
	    ubuf[1] = ubuf[2] = ubuf[3] = ' ';
	else if ( (ubuf[2] = utag[2])==0 )
	    ubuf[2] = ubuf[3] = ' ';
	else if ( (ubuf[3] = utag[3])==0 )
	    ubuf[3] = ' ';
	tag = (ubuf[0]<<24) | (ubuf[1]<<16) | (ubuf[2]<<8) | ubuf[3];
    }
    if ( u_strlen(utag)>4 || ubuf[0]>=0x7f || ubuf[1]>=0x7f || ubuf[2]>=0x7f || ubuf[3]>=0x7f ) {
	GWidgetErrorR(_STR_TagTooLong,msg);
	*err = true;
return( 0 );
    }
return( tag );
}

static int CI_ProcessPosSubs(CharInfo *ci) {
    int i, j, len;
    GTextInfo **tis;
    PST *old = ci->sc->possub;
    char **data;

    ci->sc->possub = NULL;
    for ( i=0; i<5; ++i ) {
	tis = GGadgetGetList(GWidgetGetControl(ci->gw,CID_List+i*100),&len);
	if ( len!=0 ) {
	    data = galloc((len+1)*sizeof(char *));
	    for ( j=0; j<len; ++j )
		data[j] = cu_copy(tis[j]->text);
	    data[j] = NULL;
	    SCAppendPosSub(ci->sc,i+1,data);
	    for ( j=0; j<len; ++j )
		free(data[j]);
	    free(data);
	}
    }
    PSTFree(old);
return( true );
}

static int _CI_OK(CharInfo *ci) {
    int val;
    int ret, refresh_fvdi=0;
    uint32 tag;
    char *name;
    const unichar_t *comment;
    FontView *fvs;
    int err = false;

    val = ParseUValue(ci->gw,CID_UValue,true,ci->sc->parent);
    if ( val==-2 )
return( false );
    tag = ParseTag(ci->gw,CID_Script,_STR_ScriptTagTooLong,&err);
    if ( err )
return( false );
    if ( !CI_ProcessPosSubs(ci))
return( false );
    name = cu_copy( _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName)) );
    if ( strcmp(name,ci->sc->name)!=0 || val!=ci->sc->unicodeenc )
	refresh_fvdi = 1;
    comment = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_Comment));
    SCPreserveState(ci->sc,2);
    ret = SCSetMetaData(ci->sc,name,val,comment);
    if ( ret ) ci->sc->script = tag;
    free(name);
    if ( refresh_fvdi ) {
	for ( fvs=ci->sc->parent->fv; fvs!=NULL; fvs=fvs->next )
	    GDrawRequestExpose(fvs->gw,NULL,false);	/* Redraw info area just in case this char is selected */
    }
    if ( ret ) {
	val = GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_Color));
	if ( val!=-1 ) {
	    if ( ci->sc->color != (int) (std_colors[val].userdata) ) {
		ci->sc->color = (int) (std_colors[val].userdata);
		for ( fvs=ci->sc->parent->fv; fvs!=NULL; fvs=fvs->next )
		    GDrawRequestExpose(fvs->v,NULL,false);	/* Redraw info area just in case this char is selected */
	    }
	}
    }
    if ( ret ) SCCharChangedUpdate(ci->sc);
return( ret );
}

static void CI_Finish(CharInfo *ci) {
    GDrawDestroyWindow(ci->gw);
}

static int CI_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	if ( _CI_OK(ci) )
	    CI_Finish(ci);
    }
return( true );
}

static char *LigDefaultStr(int uni, char *name, int alt_lig ) {
    const unichar_t *alt=NULL, *pt;
    char *components = NULL;
    int len;
    const unichar_t *uname;

    /* If it's not unicode we have no info on it */
    /*  Unless it looks like one of adobe's special ligature names */
    if ( uni==-1 || uni>=65536 )
	/* Nope */;
    else if ( isdecompositionnormative(uni) &&
		unicode_alternates[uni>>8]!=NULL &&
		(alt = unicode_alternates[uni>>8][uni&0xff])!=NULL ) {
	if ( alt[1]=='\0' )
	    alt = NULL;		/* Single replacements aren't ligatures */
	else if ( iscombining(alt[1]) && ( alt[2]=='\0' || iscombining(alt[2])))
	    alt = NULL;		/* Don't treat accented letters as ligatures */
	else if ( UnicodeCharacterNames[uni>>8]!=NULL &&
		(uname = UnicodeCharacterNames[uni>>8][uni&0xff])!=NULL &&
		uc_strstr(uname,"LIGATURE")==NULL &&
		uc_strstr(uname,"VULGAR FRACTION")==NULL )
	    alt = NULL;
    }
    if ( alt==NULL ) {
	if ( name==NULL || alt_lig )
return( NULL );
	else
return( AdobeLigatureFormat(name));
    }

    if ( uni==0xfb03 && alt_lig==1 )
	components = copy("ff i");
    else if ( uni==0xfb04 && alt_lig==1 )
	components = copy("ff l");
    else if ( alt!=NULL ) {
	if ( alt[1]==0x2044 && alt[3]==0 && alt_lig==1 ) {
	    static unichar_t hack[4];
	    u_strcpy(hack,alt);
	    hack[1] = '/';
	    alt = hack;
	} else if ( alt_lig )
return( NULL );
	components=NULL;
	while ( 1 ) {
	    len = 0;
	    for ( pt=alt; *pt; ++pt ) {
		if ( psunicodenames[*pt]!=NULL ) {
		    if ( components!=NULL )
			strcpy(components+len,psunicodenames[*pt]);
		    len += strlen( psunicodenames[*pt])+1;
		    if ( components!=NULL )
			components[len-1] = ' ';
		} else {
		    if ( components!=NULL )
			sprintf(components+len, "uni%04X ", *pt );
		    len += 8;
		}
	    }
	    if ( components!=NULL )
	break;
	    components = galloc(len+1);
	}
	components[len-1] = '\0';
    }
return( components );
}

static int psnamesinited=false;
#define HASH_SIZE	257
struct psbucket { const char *name; int uni; struct psbucket *prev; } *psbuckets[HASH_SIZE];

static int hashname(const char *name) {
    /* Name is assumed to be ascii */
    int hash=0;

    while ( *name ) {
	if ( *name<=' ' || *name>=0x7f )
    break;
	hash = (hash<<3)|((hash>>29)&0x7);
	hash ^= *name++-(' '+1);
    }
    hash ^= (hash>>16);
    hash &= 0xffff;
return( hash%HASH_SIZE );
}

static void psaddbucket(const char *name, int uni) {
    int hash = hashname(name);
    struct psbucket *buck = gcalloc(1,sizeof(struct psbucket));

    buck->name = name;
    buck->uni = uni;
    buck->prev = psbuckets[hash];
    psbuckets[hash] = buck;
}

static void psinitnames(void) {
    int i;

    for ( i=psaltuninames_cnt-1; i>=0 ; --i )
	psaddbucket(psaltuninames[i].name,psaltuninames[i].unicode);
    for ( i=psunicodenames_cnt-1; i>=0 ; --i )
	if ( psunicodenames[i]!=NULL )
	    psaddbucket(psunicodenames[i],i);
    psnamesinited = true;
}

int UniFromName(const char *name) {
    int i = -1;
    char *end;
    struct psbucket *buck;

    if ( strncmp(name,"uni",3)==0 ) {
	i = strtol(name+3,&end,16);
	if ( *end || end-name!=7 )	/* uniXXXXXXXX means a ligature of uniXXXX and uniXXXX */
	    i = -1;
    } else if ( name[0]=='u' && strlen(name)>=5 ) {
	i = strtol(name+1,&end,16);
	if ( *end )
	    i = -1;
    }
    if ( i==-1 ) {
	if ( !psnamesinited )
	    psinitnames();
	for ( buck = psbuckets[hashname(name)]; buck!=NULL; buck=buck->prev )
	    if ( strcmp(buck->name,name)==0 )
	break;
	if ( buck!=NULL )
	    i = buck->uni;
    }
#if 0
    if ( i==-1 ) {
	for ( i=65535; i>=0; --i )
	    if ( UnicodeCharacterNames[i>>8][i&0xff]!=NULL &&
		    uc_strcmp(UnicodeCharacterNames[i>>8][i&0xff],name)==0 )
	break;
    }
#endif
return( i );
}

int uUniFromName(const unichar_t *name) {
    int i = -1;
    unichar_t *end;

    if ( uc_strncmp(name,"uni",3)==0 ) {
	i = u_strtol(name+3,&end,16);
	if ( *end || end-name!=7 )	/* uniXXXXXXXX means a ligature of uniXXXX and uniXXXX */
	    i = -1;
    } else if ( name[0]=='u' && u_strlen(name)>=5 ) {
	i = u_strtol(name+1,&end,16);
	if ( *end )
	    i = -1;
    }
    if ( i==-1 ) for ( i=psunicodenames_cnt-1; i>=0 ; --i ) {
	if ( psunicodenames[i]!=NULL )
	    if ( uc_strcmp(name,psunicodenames[i])==0 )
    break;
    }
    if ( i==-1 ) for ( i=psaltuninames_cnt-1; i>=0 ; --i ) {
	if ( uc_strcmp(name,psaltuninames[i].name)==0 )
    break;
    }
#if 0
    if ( i==-1 ) {
	for ( i=65535; i>=0; --i )
	    if ( UnicodeCharacterNames[i>>8][i&0xff]!=NULL &&
		    u_strcmp(name,UnicodeCharacterNames[i>>8][i&0xff])==0 )
	break;
    }
#endif
return( i );
}

char *AdobeLigatureFormat(char *name) {
    /* There are two formats for ligs: <glyph-name>_<glyph-name>{...} or */
    /*  uni<code><code>{...} (only works for BMP) */
    /* I'm not checking to see if all the components are valid */
    char *components, *pt, buffer[12];
    const char *next;
    int len = strlen(name), uni;

    if ( strncmp(name,"uni",3)==0 && (len-3)%4==0 && len>7 ) {
	pt = name+3;
	components = galloc(1); *components = '\0';
	while ( *pt ) {
	    if ( sscanf(pt,"%4x", &uni )==0 ) {
		free(components); components = NULL;
	break;
	    }
	    if ( uni<psunicodenames_cnt && psunicodenames[uni]!=NULL )
		next = psunicodenames[uni];
	    else {
		sprintf(buffer,"uni%04X", uni );
		next = buffer;
	    }
	    components = grealloc(components,strlen(components) + strlen(next) + 2);
	    if ( *components!='\0' )
		strcat(components," ");
	    strcat(components,next);
	    pt += 4;
	}
	if ( components!=NULL )
return( components );
    }

    if ( strchr(name,'_')==NULL )
return( NULL );
    pt = components = copy(name);
    while ( (pt = strchr(pt,'_'))!=NULL )
	*pt = ' ';
return( components );
}

uint32 LigTagFromUnicode(int uni) {
    int tag = CHR('l','i','g','a');	/* standard */

    if (( uni>=0xbc && uni<=0xbe ) || (uni>=0x2153 && uni<=0x215e) )
	tag = CHR('f','r','a','c');	/* Fraction */
    switch ( uni ) {
      case 0xfb05:		/* long-s t */
	tag = CHR('h','l','i','g');
      break;
      case 0xfb06:		/* s t */
	tag = CHR('d','l','i','g');
      break;
      case 0xfefb: case 0xfefc:	/* Lam & Alef, required ligs */
	tag = CHR('r','l','i','g');
      break;
    }
return( tag );
}

static PST *AddSubs(PST *last,uint32 tag,char *name) {
    PST *sub = chunkalloc(sizeof(PST));
    sub->tag = tag;
    sub->type = pst_substitution;
    sub->next = last;
    sub->u.subs.variant = copy(name);
return( sub );
}

static PST *LigDefaultList(SplineChar *sc) {
    /* This fills in default ligatures as the name suggests */
    /* it also builds up various other default gpos/gsub tables */
    char *components;
    PST *lig, *lig2, *last=NULL;
    int i, alt_index;
    char namebuf[100];
    SplineChar *alt;
    SplineFont *sf = sc->parent;
    const unichar_t *variant;
    static uint32 form_tags[] = { CHR('i','n','i','t'), CHR('m','e','d','i'), CHR('f','i','n','a'), CHR('i','s','o','l'), 0 };

    for ( alt_index = 0; ; ++alt_index ) {
	components = LigDefaultStr(sc->unicodeenc,sc->name,alt_index);
	if ( components==NULL )
    break;
	lig = chunkalloc(sizeof(PST));
	lig->tag = LigTagFromUnicode(sc->unicodeenc);
	lig->type = pst_ligature;
	lig->next = last;
	last = lig;
	lig->u.lig.lig = sc;
	lig->u.lig.components = components;

	if ( lig->tag==CHR('r','l','i','g') ) {
	    lig2 = chunkalloc(sizeof(PST));
	    *lig2 = *lig;
	    lig2->tag = CHR('l','i','g','a');
	    lig2->next = last;
	    last = lig2;
	}
    }

	/* Look for left to right mirrored characters */
    if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && tomirror(sc->unicodeenc)!=0 ) {
	alt = SFGetChar(sf,tomirror(sc->unicodeenc),NULL);
	if ( alt!=NULL )
	    last=AddSubs(last,CHR('r','t','l','a'),alt->name);
    }

	/* Look for vertically rotated text */
    alt = NULL;
    if ( sf->cidmaster!=NULL ) {
	sprintf( namebuf, "cid_%d.vert", sc->enc );
	alt = SFGetChar(sf,-1,namebuf);
    }
    if ( alt==NULL && sc->unicodeenc!=-1 ) {
	sprintf( namebuf, "uni%04X.vert", sc->unicodeenc );
	alt = SFGetChar(sf,-1,namebuf);
    }
    if ( alt==NULL ) {
	sprintf( namebuf, "%s.vert", sc->name );
	alt = SFGetChar(sf,-1,namebuf);
    }
    if ( alt!=NULL )
	last=AddSubs(last,CHR('v','r','t','2'),alt->name);

	/* Look for small caps */
    sprintf( namebuf, "%s.sc", sc->name );
    alt = SFGetChar(sf,-1,namebuf);
#if 0		/* Adobe says oldstyles can be included in smallcaps */
    if ( alt==NULL ) {
	sprintf( namebuf, "%s.oldstyle", sc->name );
	alt = SFGetChar(sf,-1,namebuf);
    }
#endif
    if ( alt!=NULL )
	last=AddSubs(last,CHR('s','m','c','p'),alt->name);

	/* And for oldstyle */
    sprintf( namebuf, "%s.oldstyle", sc->name );
    alt = SFGetChar(sf,-1,namebuf);
    if ( alt!=NULL )
	last=AddSubs(last,CHR('o','n','u','m'),alt->name);

	/* Look for superscripts */
    sprintf( namebuf, "%s.superior", sc->name );
    alt = SFGetChar(sf,-1,namebuf);
    if ( alt!=NULL )
	last=AddSubs(last,CHR('s','u','p','s'),alt->name);

	/* Look for subscripts */
    sprintf( namebuf, "%s.inferior", sc->name );
    alt = SFGetChar(sf,-1,namebuf);
    if ( alt!=NULL )
	last=AddSubs(last,CHR('s','u','b','s'),alt->name);

	/* Look for swash forms */
    sprintf( namebuf, "%s.swash", sc->name );
    alt = SFGetChar(sf,-1,namebuf);
    if ( alt!=NULL ) {
	last=AddSubs(last,CHR('s','w','s','h'),alt->name);
	last->type = pst_alternate;
    }

    if (( sc->unicodeenc>=0xff01 && sc->unicodeenc<=0xff5e ) ||
	    ( sc->unicodeenc>=0xffe0 && sc->unicodeenc<0xffe6)) {
	/* these are full width latin */
	if ( unicode_alternates[sc->unicodeenc>>8]!=NULL &&
		(variant = unicode_alternates[sc->unicodeenc>>8][sc->unicodeenc&0xff])!=NULL &&
		variant[1]=='\0' ) {
	    alt = SFGetChar(sf,variant[0],NULL);
	    if ( alt!=NULL )
		last=AddSubs(last,CHR('p','w','i','d'),alt->name);
	}
    } else if ( sc->unicodeenc>=0xff61 && sc->unicodeenc<0xffdc ) {
	/* These are halfwidth katakana and sung */
	if ( unicode_alternates[sc->unicodeenc>>8]!=NULL &&
		(variant = unicode_alternates[sc->unicodeenc>>8][sc->unicodeenc&0xff])!=NULL &&
		variant[1]=='\0' ) {
	    alt = SFGetChar(sf,variant[0],NULL);
	    if ( alt!=NULL )
		last=AddSubs(last,CHR('f','w','i','d'),alt->name);
	}
    } else if ( sc->unicodeenc>=0x0021 && sc->unicodeenc<=0x100 ) {
	for ( i=0xff01; i<0xffef; ++i ) {
	    if ( unicode_alternates[i>>8]!=NULL &&
		    (variant = unicode_alternates[i>>8][i&0xff])!=NULL &&
		    variant[1]=='\0' && variant[0]==sc->unicodeenc )
	break;
	}
	alt = NULL;
	if ( i<0xffef ) {
	    alt = SFGetChar(sf,i,NULL);
	    if ( alt!=NULL )
		last=AddSubs(last,CHR('f','w','i','d'),alt->name);
	}
	if ( alt!=NULL ) {
	    sprintf( namebuf, "%s.full", sc->name );
	    alt = SFGetChar(sf,-1,namebuf);
	    if ( alt!=NULL )
		last=AddSubs(last,CHR('f','w','i','d'),alt->name);
	}
	sprintf( namebuf, "%s.hw", sc->name );
	alt = SFGetChar(sf,-1,namebuf);
	if ( alt!=NULL )
	    last=AddSubs(last,CHR('h','w','i','d'),alt->name);
    } else if ( sc->unicodeenc>=0x3000 && sc->unicodeenc<=0x31ff ) {
	/* Japanese katakana & Korean sung full */
	for ( i=0xff61; i<0xffdf; ++i ) {
	    if ( unicode_alternates[i>>8]!=NULL &&
		    (variant = unicode_alternates[i>>8][i&0xff])!=NULL &&
		    variant[1]=='\0' && variant[0]==sc->unicodeenc )
	break;
	}
	if ( i<0xffdf ) {
	    alt = SFGetChar(sf,i,NULL);
	    if ( alt!=NULL )
		last=AddSubs(last,CHR('h','w','i','d'),alt->name);
	}
    }

    if ( sc->unicodeenc>=0x600 && sc->unicodeenc<0x700 ) {
	/* Arabic forms */
	for ( i=0; form_tags[i]!=0; ++i ) {
	    if ( (&(ArabicForms[sc->unicodeenc-0x600].initial))[i]!=0 &&
		    (&(ArabicForms[sc->unicodeenc-0x600].initial))[i]!=sc->unicodeenc &&
		    (alt = SFGetChar(sf,(&(ArabicForms[sc->unicodeenc-0x600].initial))[i],NULL))!=NULL )
		last=AddSubs(last,form_tags[i],alt->name);
	}
    }
return( last );
}

void SCLigDefault(SplineChar *sc) {
    PSTFree(sc->possub);
    sc->possub = LigDefaultList(sc);
}

static int CI_SName(GGadget *g, GEvent *e) {	/* Set From Name */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName));
	int i;
	char buf[10]; unichar_t ubuf[2], *temp;
	i = uUniFromName(ret);
	for ( i=psunicodenames_cnt-1; i>=0; --i )
	    if ( psunicodenames[i]!=NULL && uc_strcmp(ret,psunicodenames[i])==0 )
	break;
	if ( i==-1 ) {
	    /* Adobe says names like uni00410042 represent a ligature (A&B) */
	    /*  (that is "uni" followed by two 4-digit codes). */
	    /* But that names outside of BMP should be uXXXX or uXXXXX or uXXXXXX */
	    if ( ret[0]=='u' && ret[1]!='n' && u_strlen(ret)<=1+6 ) {
		unichar_t *end;
		i = u_strtol(ret+1,&end,16);
		if ( *end )
		    i = -1;
		else		/* Make sure it is properly capitalized */
		    SetNameFromUnicode(ci->gw,CID_UName,i);
	    }
	}
	if ( i==-1 ) {
	    for ( i=65535; i>=0; --i )
		if ( UnicodeCharacterNames[i>>8][i&0xff]!=NULL &&
			u_strcmp(ret,UnicodeCharacterNames[i>>8][i&0xff])==0 )
	    break;
	}

	sprintf(buf,"U+%04x", i);
	temp = uc_copy(i==-1?"-1":buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);

	SetScriptFromUnicode(ci->gw,i,ci->sc->parent);

	ubuf[0] = i;
	if ( i==-1 || i>0xffff )
	    ubuf[0] = '\0';
	ubuf[1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
    }
return( true );
}

static int CI_SValue(GGadget *g, GEvent *e) {	/* Set From Value */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	unichar_t ubuf[2];
	int val;

	val = ParseUValue(ci->gw,CID_UValue,false,ci->sc->parent);
	if ( val<0 )
return( true );

	SetNameFromUnicode(ci->gw,CID_UName,val);
	SetScriptFromUnicode(ci->gw,val,ci->sc->parent);

	ubuf[0] = val;
	if ( val==-1 )
	    ubuf[0] = '\0';
	ubuf[1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
    }
return( true );
}

static GTextInfo *TIFromName(const char *name) {
    GTextInfo *ti = gcalloc(1,sizeof(GTextInfo));
    ti->text = uc_copy(name);
    ti->fg = COLOR_DEFAULT;
    ti->bg = COLOR_DEFAULT;
return( ti );
}

static void CI_SetNameList(CharInfo *ci,int val) {
    GGadget *g = GWidgetGetControl(ci->gw,CID_UName);
    int cnt, i;
    char buffer[20];

    if ( GGadgetGetUserData(g)==(void *) val )
return;		/* Didn't change */
    if ( val<0 || val>=0x1000000 ) {
	static GTextInfo notdef = { (unichar_t *) ".notdef", NULL, COLOR_DEFAULT, COLOR_DEFAULT, NULL, NULL, 0, 0, 0, 0, 0, 0, 1},
			 empty = { NULL },
			 *list[] = { &notdef, &empty };
	GGadgetSetList(g,list,true);
    } else {
	GTextInfo **list = NULL;
	while ( 1 ) {
	    cnt=0;
	    if ( val<psunicodenames_cnt && psunicodenames[val]!=NULL ) {
		if ( list ) list[cnt] = TIFromName(psunicodenames[val]);
		++cnt;
	    } else if ( val<32 || (val>=0x7f && val<0xa0)) {
		if ( list ) list[cnt] = TIFromName(".notdef");
		++cnt;
	    }
	    for ( i=0; psaltuninames[i].name!=NULL; ++i )
		if ( psaltuninames[i].unicode==val ) {
		    if ( list ) list[cnt] = TIFromName(psaltuninames[i].name);
		    ++cnt;
		}
	    if ( val<0x10000 ) {
		if ( list ) {
		    sprintf( buffer, "uni%04X", val);
		    list[cnt] = TIFromName(buffer);
		}
		++cnt;
	    }
	    if ( list ) {
		sprintf( buffer, "u%04X", val);
		list[cnt] = TIFromName(buffer);
		list[cnt+1] = TIFromName(NULL);
	    }
	    ++cnt;
	    if ( list!=NULL )
	break;
	    list = galloc((cnt+1)*sizeof(GTextInfo*)); 
	}
	GGadgetSetList(g,list,true);
    }
    GGadgetSetUserData(g,(void *) val);
}

static int CI_UValChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UValue));
	unichar_t *end;
	int val;

	if (( *ret=='U' || *ret=='u' ) && ret[1]=='+' )
	    ret += 2;
	val = u_strtol(ret,&end,16);
	if ( *end=='\0' )
	    CI_SetNameList(ci,val);
    }
return( true );
}

static int CI_CharChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UChar));
	int val = *ret;
	unichar_t *temp, ubuf[1]; char buf[10];

	if ( ret[1]!='\0' ) {
	    temp = uc_copy("Only a single character allowed");
	    GWidgetPostNotice(temp,temp);
	    free(temp);
	    ubuf[0] = '\0';
	    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
return( true );
	} else if ( ret[0]=='\0' )
return( true );

	SetNameFromUnicode(ci->gw,CID_UName,val);
	CI_SetNameList(ci,val);

	sprintf(buf,"U+%04x", val);
	temp = uc_copy(buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);
    }
return( true );
}

static int CI_ScriptChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged &&
	    e->u.control.u.tf_changed.from_pulldown!=-1 ) {
	uint32 tag = (uint32) scripts[e->u.control.u.tf_changed.from_pulldown].userdata;
	unichar_t ubuf[8];
	/* If they select something from the pulldown, don't show the human */
	/*  readable form, instead show the 4 character tag */
	ubuf[0] = tag>>24;
	ubuf[1] = (tag>>16)&0xff;
	ubuf[2] = (tag>>8)&0xff;
	ubuf[3] = tag&0xff;
	ubuf[4] = 0;
	GGadgetSetTitle(g,ubuf);
    }
return( true );
}

static int CI_CommentChanged(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_textchanged ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	/* Let's give things with comments a white color. This may not be a good idea */
	if ( ci->first && ci->sc->color==COLOR_DEFAULT &&
		0==GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_Color)) )
	    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),1);
	ci->first = false;
    }
return( true );
}

static void CIFillup(CharInfo *ci) {
    SplineChar *sc = ci->sc;
    SplineFont *sf = sc->parent;
    unichar_t *temp;
    char buffer[200];
    unichar_t ubuf[8];
    const unichar_t *bits;
    int i,j;
    GTextInfo **arrays[pst_max];
    int cnts[pst_max];
    PST *pst;

    if ( ci->oldsc!=NULL && ci->oldsc->charinfo==ci )
	ci->oldsc->charinfo = NULL;
    sc->charinfo = ci;
    ci->oldsc = sc;

    CI_CanPaste(ci);

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,-1), sc->enc>0 &&
	    (sf->chars[sc->enc-1]==NULL || sf->chars[sc->enc-1]->charinfo==NULL));
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,1), sc->enc<sf->charcnt-1 &&
	    (sf->chars[sc->enc+1]==NULL || sf->chars[sc->enc+1]->charinfo==NULL));

    temp = uc_copy(sc->name);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UName),temp);
    free(temp);
    CI_SetNameList(ci,sc->unicodeenc);

    sprintf(buffer,"U+%04x", sc->unicodeenc);
    temp = uc_copy(sc->unicodeenc==-1?"-1":buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
    free(temp);

    ubuf[0] = sc->unicodeenc;
    if ( sc->unicodeenc==-1 )
	ubuf[0] = '\0';
    ubuf[1] = '\0';
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);

    SetScriptFromTag(ci->gw,sc->script);

    memset(cnts,0,sizeof(cnts));
    for ( pst = sc->possub; pst!=NULL; pst=pst->next )
	++cnts[pst->type];
    for ( i=pst_null+1; i<pst_max; ++i )
	arrays[i] = gcalloc((cnts[i]+1),sizeof(GTextInfo *));
    memset(cnts,0,sizeof(cnts));
    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
	j = cnts[pst->type]++;
	arrays[pst->type][j] = gcalloc(1,sizeof(GTextInfo));
	if ( pst->type==pst_position ) {
	    sprintf(buffer,"     dx=%d dy=%d dx_adv=%d dy_adv=%d",
		    pst->u.pos.xoff, pst->u.pos.yoff,
		    pst->u.pos.h_adv_off, pst->u.pos.v_adv_off );
	    arrays[pst->type][j]->text = uc_copy(buffer);
	} else {
	    arrays[pst->type][j]->text = galloc((strlen(pst->u.subs.variant)+6)*sizeof(unichar_t));;
	    uc_strcpy(arrays[pst->type][j]->text+5,pst->u.subs.variant);
	    arrays[pst->type][j]->text[4] = ' ';
	}
	arrays[pst->type][j]->text[0] = pst->tag>>24;
	arrays[pst->type][j]->text[1] = (pst->tag>>16)&0xff;
	arrays[pst->type][j]->text[2] = (pst->tag>>8)&0xff;
	arrays[pst->type][j]->text[3] = (pst->tag)&0xff;
	arrays[pst->type][j]->fg = arrays[pst->type][j]->bg = COLOR_DEFAULT;
    }
    for ( i=pst_null+1; i<pst_max; ++i ) {
	arrays[i][cnts[i]] = gcalloc(1,sizeof(GTextInfo));
	GGadgetSetList(GWidgetGetControl(ci->gw,CID_List+(i-1)*100),
		arrays[i],false);
    }

    bits = SFGetAlternate(sc->parent,sc->unicodeenc,sc,true);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_ComponentMsg),GStringGetResource(
	bits==NULL ? _STR_NoComponents :
	hascomposing(sc->parent,sc->unicodeenc,sc) ? _STR_AccentedComponents :
	    _STR_CompositComponents, NULL));
    if ( bits==NULL ) {
	ubuf[0] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Components),ubuf);
    } else {
	unichar_t *temp = galloc(11*u_strlen(bits)*sizeof(unichar_t));
	unichar_t *upt=temp;
	while ( *bits!='\0' ) {
	    sprintf(buffer, "U+%04x ", *bits );
	    uc_strcpy(upt,buffer);
	    upt += u_strlen(upt);
	    ++bits;
	}
	upt[-1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Components),temp);
	free(temp);
    }

    ubuf[0] = '\0';
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_Comment),
	    sc->comment?sc->comment:ubuf);
    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),0);
    for ( i=0; std_colors[i].image!=NULL; ++i ) {
	if ( std_colors[i].userdata == (void *) sc->color )
	    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),i);
    }
    ci->first = sc->comment==NULL;
}

static int CI_NextPrev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int enc = ci->sc->enc + GGadgetGetCid(g);	/* cid is 1 for next, -1 for prev */
	SplineChar *new;

	if ( enc<0 || enc>=ci->sc->parent->charcnt )
return( true );
	if ( !_CI_OK(ci))
return( true );
	new = SFMakeChar(ci->sc->parent,enc);
	if ( new->charinfo!=NULL && new->charinfo!=ci )
return( true );
	ci->sc = new;
	CIFillup(ci);
    }
return( true );
}

static int CI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CI_Finish(ci);
    }
return( true );
}

static int ci_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	CharInfo *ci = GDrawGetUserData(gw);
	CI_Finish(ci);
    } else if ( event->type==et_char ) {
	CharInfo *ci = GDrawGetUserData(gw);
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("getinfo.html");
return( true );
	} else if ( event->u.chr.keysym=='c' && (event->u.chr.state&ksm_control)) {
	    CI_DoCopy(ci);
return( true );
	} else if ( event->u.chr.keysym=='v' && (event->u.chr.state&ksm_control)) {
	    CI_DoPaste(ci,NULL,pst_null);
return( true );
	}
return( false );
    } else if ( event->type == et_drop ) {
	CharInfo *ci = GDrawGetUserData(gw);
	CI_Drop(ci,event);
    } else if ( event->type == et_destroy ) {
	CharInfo *ci = GDrawGetUserData(gw);
	ci->sc->charinfo = NULL;
	free(ci);
    } else if ( event->type == et_map ) {
	/* Above palettes */
	GDrawRaise(gw);
    }
return( true );
}

void SCGetInfo(SplineChar *sc) {
    CharInfo *ci;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData ugcd[12], cgcd[6], psgcd[5][7], cogcd[3], mgcd[9];
    GTextInfo ulabel[12], clabel[6], pslabel[5][6], colabel[3], mlabel[9];
    int i;
    GTabInfo aspects[11];
    static GBox smallbox = { bt_raised, bs_rect, 2, 1, 0, 0, 0,0,0,0, COLOR_DEFAULT,COLOR_DEFAULT };
    static int boxset=0;

    if ( sc->charinfo!=NULL ) {
	GDrawSetVisible(sc->charinfo->gw,true);
	GDrawRaise(sc->charinfo->gw);
return;
    }

    ci = gcalloc(1,sizeof(CharInfo));
    ci->sc = sc;
    ci->done = false;

    if ( !boxset ) {
	extern GBox _ggadget_Default_Box;
	extern void GGadgetInit(void);
	GGadgetInit();
	smallbox = _ggadget_Default_Box;
	smallbox.padding = 1;
	boxset = 1;
    }

    if ( ci->gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = false;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = GStringGetResource( _STR_Charinfo,NULL );
	wattrs.is_dlg = false;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,CI_Width));
	pos.height = GDrawPointsToPixels(NULL,CI_Height);
	ci->gw = GDrawCreateTopWindow(NULL,&pos,ci_e_h,ci,&wattrs);

	memset(&ugcd,0,sizeof(ugcd));
	memset(&ulabel,0,sizeof(ulabel));

	ulabel[0].text = (unichar_t *) _STR_UnicodeName;
	ulabel[0].text_in_resource = true;
	ugcd[0].gd.label = &ulabel[0];
	ugcd[0].gd.pos.x = 5; ugcd[0].gd.pos.y = 5+4; 
	ugcd[0].gd.flags = gg_enabled|gg_visible;
	ugcd[0].gd.mnemonic = 'N';
	ugcd[0].creator = GLabelCreate;

	ugcd[1].gd.pos.x = 85; ugcd[1].gd.pos.y = 5;
	ugcd[1].gd.flags = gg_enabled|gg_visible;
	ugcd[1].gd.mnemonic = 'N';
	ugcd[1].gd.cid = CID_UName;
	ugcd[1].creator = GListFieldCreate;
	ugcd[1].data = (void *) (-2);

	ulabel[2].text = (unichar_t *) _STR_UnicodeValue;
	ulabel[2].text_in_resource = true;
	ugcd[2].gd.label = &ulabel[2];
	ugcd[2].gd.pos.x = 5; ugcd[2].gd.pos.y = 31+4; 
	ugcd[2].gd.flags = gg_enabled|gg_visible;
	ugcd[2].gd.mnemonic = 'V';
	ugcd[2].creator = GLabelCreate;

	ugcd[3].gd.pos.x = 85; ugcd[3].gd.pos.y = 31;
	ugcd[3].gd.flags = gg_enabled|gg_visible;
	ugcd[3].gd.mnemonic = 'V';
	ugcd[3].gd.cid = CID_UValue;
	ugcd[3].gd.handle_controlevent = CI_UValChanged;
	ugcd[3].creator = GTextFieldCreate;

	ulabel[4].text = (unichar_t *) _STR_UnicodeChar;
	ulabel[4].text_in_resource = true;
	ugcd[4].gd.label = &ulabel[4];
	ugcd[4].gd.pos.x = 5; ugcd[4].gd.pos.y = 57+4; 
	ugcd[4].gd.flags = gg_enabled|gg_visible;
	ugcd[4].gd.mnemonic = 'h';
	ugcd[4].creator = GLabelCreate;

	ugcd[5].gd.pos.x = 85; ugcd[5].gd.pos.y = 57;
	ugcd[5].gd.flags = gg_enabled|gg_visible|gg_text_xim;
	ugcd[5].gd.mnemonic = 'h';
	ugcd[5].gd.cid = CID_UChar;
	ugcd[5].gd.handle_controlevent = CI_CharChanged;
	ugcd[5].creator = GTextFieldCreate;

	ulabel[6].text = (unichar_t *) _STR_ScriptC;
	ulabel[6].text_in_resource = true;
	ugcd[6].gd.label = &ulabel[6];
	ugcd[6].gd.pos.x = 5; ugcd[6].gd.pos.y = 83+4; 
	ugcd[6].gd.flags = gg_enabled|gg_visible;
	ugcd[6].creator = GLabelCreate;

	ugcd[7].gd.pos.x = 85; ugcd[7].gd.pos.y = 83;
	ugcd[7].gd.flags = gg_enabled|gg_visible;
	ugcd[7].gd.mnemonic = 'h';
	ugcd[7].gd.cid = CID_Script;
	ugcd[7].gd.u.list = scripts;
	ugcd[7].gd.handle_controlevent = CI_ScriptChanged;
	ugcd[7].creator = GListFieldCreate;

	ugcd[8].gd.pos.x = 12; ugcd[8].gd.pos.y = 117;
	ugcd[8].gd.flags = gg_visible | gg_enabled;
	ulabel[8].text = (unichar_t *) _STR_SetFromName;
	ulabel[8].text_in_resource = true;
	ugcd[8].gd.mnemonic = 'a';
	ugcd[8].gd.label = &ulabel[8];
	ugcd[8].gd.handle_controlevent = CI_SName;
	ugcd[8].creator = GButtonCreate;

	ugcd[9].gd.pos.x = 107; ugcd[9].gd.pos.y = 117;
	ugcd[9].gd.flags = gg_visible | gg_enabled;
	ulabel[9].text = (unichar_t *) _STR_SetFromValue;
	ulabel[9].text_in_resource = true;
	ugcd[9].gd.mnemonic = 'l';
	ugcd[9].gd.label = &ulabel[9];
	ugcd[9].gd.handle_controlevent = CI_SValue;
	ugcd[9].creator = GButtonCreate;

	memset(&cgcd,0,sizeof(cgcd));
	memset(&clabel,0,sizeof(clabel));

	clabel[0].text = (unichar_t *) _STR_Comment;
	clabel[0].text_in_resource = true;
	cgcd[0].gd.label = &clabel[0];
	cgcd[0].gd.pos.x = 5; cgcd[0].gd.pos.y = 5; 
	cgcd[0].gd.flags = gg_enabled|gg_visible;
	cgcd[0].creator = GLabelCreate;

	cgcd[1].gd.pos.x = 5; cgcd[1].gd.pos.y = cgcd[0].gd.pos.y+13;
	cgcd[1].gd.pos.width = CI_Width-20;
	cgcd[1].gd.pos.height = 7*12+6;
	cgcd[1].gd.flags = gg_enabled|gg_visible|gg_textarea_wrap|gg_text_xim;
	cgcd[1].gd.cid = CID_Comment;
	cgcd[1].gd.handle_controlevent = CI_CommentChanged;
	cgcd[1].creator = GTextAreaCreate;

	clabel[2].text = (unichar_t *) _STR_Color;
	clabel[2].text_in_resource = true;
	cgcd[2].gd.label = &clabel[2];
	cgcd[2].gd.pos.x = 5; cgcd[2].gd.pos.y = cgcd[1].gd.pos.y+cgcd[1].gd.pos.height+5+6; 
	cgcd[2].gd.flags = gg_enabled|gg_visible;
	cgcd[2].creator = GLabelCreate;

	cgcd[3].gd.pos.x = cgcd[3].gd.pos.x; cgcd[3].gd.pos.y = cgcd[2].gd.pos.y-6;
	cgcd[3].gd.pos.width = cgcd[3].gd.pos.width;
	cgcd[3].gd.flags = gg_enabled|gg_visible;
	cgcd[3].gd.cid = CID_Color;
	cgcd[3].gd.u.list = std_colors;
	cgcd[3].creator = GListButtonCreate;

	memset(&psgcd,0,sizeof(psgcd));
	memset(&pslabel,0,sizeof(pslabel));

	for ( i=0; i<5; ++i ) {
	    psgcd[i][0].gd.pos.x = 5; psgcd[i][0].gd.pos.y = 5;
	    psgcd[i][0].gd.pos.width = CI_Width-28; psgcd[i][0].gd.pos.height = 7*12+10;
	    psgcd[i][0].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic;
	    psgcd[i][0].gd.cid = CID_List+i*100;
	    psgcd[i][0].gd.u.list = pst_tags[i];
	    psgcd[i][0].gd.handle_controlevent = CI_SelChanged;
	    psgcd[i][0].gd.box = &smallbox;
	    psgcd[i][0].creator = GListCreate;

	    psgcd[i][1].gd.pos.x = 10; psgcd[i][1].gd.pos.y = psgcd[i][0].gd.pos.y+psgcd[i][0].gd.pos.height+4;
	    psgcd[i][1].gd.pos.width = -1;
	    psgcd[i][1].gd.flags = gg_visible | gg_enabled;
	    pslabel[i][1].text = (unichar_t *) _STR_NewDDD;
	    pslabel[i][1].text_in_resource = true;
	    psgcd[i][1].gd.label = &pslabel[i][1];
	    psgcd[i][1].gd.cid = CID_New+i*100;
	    psgcd[i][1].gd.handle_controlevent = CI_New;
	    psgcd[i][1].gd.box = &smallbox;
	    psgcd[i][1].creator = GButtonCreate;

	    psgcd[i][2].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor); psgcd[i][2].gd.pos.y = psgcd[i][1].gd.pos.y;
	    psgcd[i][2].gd.pos.width = -1;
	    psgcd[i][2].gd.flags = gg_visible;
	    pslabel[i][2].text = (unichar_t *) _STR_Delete;
	    pslabel[i][2].text_in_resource = true;
	    psgcd[i][2].gd.label = &pslabel[i][2];
	    psgcd[i][2].gd.cid = CID_Delete+i*100;
	    psgcd[i][2].gd.handle_controlevent = CI_Delete;
	    psgcd[i][2].gd.box = &smallbox;
	    psgcd[i][2].creator = GButtonCreate;

	    psgcd[i][3].gd.pos.x = -10; psgcd[i][3].gd.pos.y = psgcd[i][1].gd.pos.y;
	    psgcd[i][3].gd.pos.width = -1;
	    psgcd[i][3].gd.flags = gg_visible;
	    pslabel[i][3].text = (unichar_t *) _STR_EditDDD;
	    pslabel[i][3].text_in_resource = true;
	    psgcd[i][3].gd.label = &pslabel[i][3];
	    psgcd[i][3].gd.cid = CID_Edit+i*100;
	    psgcd[i][3].gd.handle_controlevent = CI_Edit;
	    psgcd[i][3].gd.box = &smallbox;
	    psgcd[i][3].creator = GButtonCreate;

	    psgcd[i][4].gd.pos.x = 20; psgcd[i][4].gd.pos.y = psgcd[i][3].gd.pos.y+22;
	    psgcd[i][4].gd.pos.width = -1;
	    psgcd[i][4].gd.flags = gg_visible;
	    pslabel[i][4].text = (unichar_t *) _STR_Copy;
	    pslabel[i][4].text_in_resource = true;
	    psgcd[i][4].gd.label = &pslabel[i][4];
	    psgcd[i][4].gd.cid = CID_Copy+i*100;
	    psgcd[i][4].gd.handle_controlevent = CI_Copy;
	    psgcd[i][4].gd.box = &smallbox;
	    psgcd[i][4].creator = GButtonCreate;

	    psgcd[i][5].gd.pos.x = -20; psgcd[i][5].gd.pos.y = psgcd[i][4].gd.pos.y;
	    psgcd[i][5].gd.pos.width = -1;
	    psgcd[i][5].gd.flags = gg_visible;
	    pslabel[i][5].text = (unichar_t *) _STR_Paste;
	    pslabel[i][5].text_in_resource = true;
	    psgcd[i][5].gd.label = &pslabel[i][5];
	    psgcd[i][5].gd.cid = CID_Paste+i*100;
	    psgcd[i][5].gd.handle_controlevent = CI_Paste;
	    psgcd[i][5].gd.box = &smallbox;
	    psgcd[i][5].creator = GButtonCreate;
	}

	memset(&cogcd,0,sizeof(cogcd));
	memset(&colabel,0,sizeof(colabel));

	colabel[0].text = (unichar_t *) _STR_AccentedComponents;
	colabel[0].text_in_resource = true;
	cogcd[0].gd.label = &colabel[0];
	cogcd[0].gd.pos.x = 5; cogcd[0].gd.pos.y = 5; 
	cogcd[0].gd.flags = gg_enabled|gg_visible;
	cogcd[0].gd.cid = CID_ComponentMsg;
	/*cogcd[0].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);*/
	cogcd[0].creator = GLabelCreate;

	cogcd[1].gd.pos.x = 5; cogcd[1].gd.pos.y = cogcd[0].gd.pos.y+12;
	cogcd[1].gd.pos.width = CI_Width-20;
	cogcd[1].gd.flags = gg_enabled|gg_visible;
	cogcd[1].gd.cid = CID_Components;
	/*cogcd[1].gd.popup_msg = GStringGetResource(_STR_Ligpop,NULL);*/
	cogcd[1].creator = GLabelCreate;

	memset(&mgcd,0,sizeof(mgcd));
	memset(&mlabel,0,sizeof(mlabel));
	memset(&aspects,'\0',sizeof(aspects));

	i = 0;
	aspects[i].text = (unichar_t *) _STR_UnicodeL;
	aspects[i].text_in_resource = true;
	aspects[i].selected = true;
	aspects[i++].gcd = ugcd;

	aspects[i].text = (unichar_t *) _STR_Comment;
	aspects[i].text_in_resource = true;
	aspects[i++].gcd = cgcd;

	aspects[i].text = (unichar_t *) _STR_AltPos;
	aspects[i].text_in_resource = true;
	aspects[i++].gcd = psgcd[0];

	aspects[i].text = (unichar_t *) _STR_Subs;
	aspects[i].text_in_resource = true;
	aspects[i++].gcd = psgcd[1];

	aspects[i].text = (unichar_t *) _STR_AltSubs;
	aspects[i].text_in_resource = true;
	aspects[i++].gcd = psgcd[2];

	aspects[i].text = (unichar_t *) _STR_MultSubs;
	aspects[i].text_in_resource = true;
	aspects[i++].gcd = psgcd[3];

	aspects[i].text = (unichar_t *) _STR_LigatureL;
	aspects[i].text_in_resource = true;
	aspects[i++].gcd = psgcd[4];

	aspects[i].text = (unichar_t *) _STR_Components;
	aspects[i].text_in_resource = true;
	aspects[i++].gcd = cogcd;

	mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
	mgcd[0].gd.pos.width = CI_Width-10;
	mgcd[0].gd.pos.height = CI_Height-70;
	mgcd[0].gd.u.tabs = aspects;
	mgcd[0].gd.flags = gg_visible | gg_enabled;
	mgcd[0].gd.cid = CID_Tabs;
	mgcd[0].creator = GTabSetCreate;

	mgcd[1].gd.pos.x = 40; mgcd[1].gd.pos.y = mgcd[0].gd.pos.y+mgcd[0].gd.pos.height+3;
	mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
	mgcd[1].gd.flags = gg_visible | gg_enabled ;
	mlabel[1].text = (unichar_t *) _STR_PrevArrow;
	mlabel[1].text_in_resource = true;
	mgcd[1].gd.mnemonic = 'P';
	mgcd[1].gd.label = &mlabel[1];
	mgcd[1].gd.handle_controlevent = CI_NextPrev;
	mgcd[1].gd.cid = -1;
	mgcd[1].creator = GButtonCreate;

	mgcd[2].gd.pos.x = -40; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y;
	mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
	mgcd[2].gd.flags = gg_visible | gg_enabled ;
	mlabel[2].text = (unichar_t *) _STR_NextArrow;
	mlabel[2].text_in_resource = true;
	mgcd[2].gd.label = &mlabel[2];
	mgcd[2].gd.mnemonic = 'N';
	mgcd[2].gd.handle_controlevent = CI_NextPrev;
	mgcd[2].gd.cid = 1;
	mgcd[2].creator = GButtonCreate;

	mgcd[3].gd.pos.x = 25-3; mgcd[3].gd.pos.y = CI_Height-31-3;
	mgcd[3].gd.pos.width = -1; mgcd[3].gd.pos.height = 0;
	mgcd[3].gd.flags = gg_visible | gg_enabled | gg_but_default;
	mlabel[3].text = (unichar_t *) _STR_OK;
	mlabel[3].text_in_resource = true;
	mgcd[3].gd.mnemonic = 'O';
	mgcd[3].gd.label = &mlabel[3];
	mgcd[3].gd.handle_controlevent = CI_OK;
	mgcd[3].creator = GButtonCreate;

	mgcd[4].gd.pos.x = -25; mgcd[4].gd.pos.y = mgcd[3].gd.pos.y+3;
	mgcd[4].gd.pos.width = -1; mgcd[4].gd.pos.height = 0;
	mgcd[4].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	mlabel[4].text = (unichar_t *) _STR_Done;
	mlabel[4].text_in_resource = true;
	mgcd[4].gd.label = &mlabel[4];
	mgcd[4].gd.mnemonic = 'C';
	mgcd[4].gd.handle_controlevent = CI_Cancel;
	mgcd[4].gd.cid = CID_Cancel;
	mgcd[4].creator = GButtonCreate;

	GGadgetsCreate(ci->gw,mgcd);
    }

    CIFillup(ci);

    GWidgetHidePalettes();
    GDrawSetVisible(ci->gw,true);
}

void CharInfoDestroy(CharInfo *ci) {
    GDrawDestroyWindow(ci->gw);
}
