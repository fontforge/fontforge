/* Copyright (C) 2000-2006 by George Williams */
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
#include <chardata.h>
#include "ttf.h"		/* For MAC_DELETED_GLYPH_NAME */
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <gkeysym.h>

typedef struct charinfo {
    CharView *cv;
    EncMap *map;
    SplineChar *sc;
    SplineChar *oldsc;		/* oldsc->charinfo will point to us. Used to keep track of that pointer */
    int enc;
    GWindow gw;
    int done, first, changed;
} CharInfo;

#define CI_Width	218
#define CI_Height	292

#define CID_UName	1001
#define CID_UValue	1002
#define CID_UChar	1003
#define CID_Cancel	1005
#define CID_ComponentMsg	1006
#define CID_Components	1007
#define CID_Comment	1008
#define CID_Color	1009
#define CID_GClass	1010
#define CID_Tabs	1011

#define CID_TeX_Height	1012
#define CID_TeX_Depth	1013
#define CID_TeX_Sub	1014
#define CID_TeX_Super	1015

/* Offsets for repeated fields. add 100*index */
#define CID_List	1020
#define CID_New		1021
#define CID_Delete	1022
#define CID_Edit	1023
#define CID_Copy	1024
#define CID_Paste	1025

#define CID_PST		1111
#define CID_Tag		1112
#define CID_Contents	1113
#define CID_SelectResults	1114
#define CID_MergeResults	1115
#define CID_RestrictSelection	1116

static GTextInfo glyphclasses[] = {
    { (unichar_t *) N_("Automatic"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("No Class"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Base Glyph"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Base Lig"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mark"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Component"), NULL, 0, 0, NULL, NULL, false, false, false, false, false, false, true },
    { NULL, NULL }
};

static GTextInfo std_colors[] = {
    { (unichar_t *) N_("Default"), &def_image, 0, 0, (void *) COLOR_DEFAULT, NULL, false, true, false, false, false, false, true },
    { NULL, &white_image, 0, 0, (void *) 0xffffff, NULL, false, true },
    { NULL, &red_image, 0, 0, (void *) 0xff0000, NULL, false, true },
    { NULL, &green_image, 0, 0, (void *) 0x00ff00, NULL, false, true },
    { NULL, &blue_image, 0, 0, (void *) 0x0000ff, NULL, false, true },
    { NULL, &yellow_image, 0, 0, (void *) 0xffff00, NULL, false, true },
    { NULL, &cyan_image, 0, 0, (void *) 0x00ffff, NULL, false, true },
    { NULL, &magenta_image, 0, 0, (void *) 0xff00ff, NULL, false, true },
    { NULL, NULL }
};

GTextInfo scripts[] = {
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Arabic", ignore "Script|" */
    { (unichar_t *) N_("Script|Arabic"), NULL, 0, 0, (void *) CHR('a','r','a','b'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Aramaic"), NULL, 0, 0, (void *) CHR('a','r','a','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Armenian"), NULL, 0, 0, (void *) CHR('a','r','m','n'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Avestan"), NULL, 0, 0, (void *) CHR('a','v','e','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Batak"), NULL, 0, 0, (void *) CHR('b','a','t','k'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Bengali"), NULL, 0, 0, (void *) CHR('b','e','n','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bliss Symbolics"), NULL, 0, 0, (void *) CHR('b','l','i','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bopomofo"), NULL, 0, 0, (void *) CHR('b','o','p','o'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) NU_("Brāhmī"), NULL, 0, 0, (void *) CHR('b','r','a','h'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Braille"), NULL, 0, 0, (void *) CHR('b','r','a','i'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Buginese"), NULL, 0, 0, (void *) CHR('b','u','g','i'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Buhid"), NULL, 0, 0, (void *) CHR('b','u','h','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Byzantine Music"), NULL, 0, 0, (void *) CHR('b','y','z','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Canadian Syllabics"), NULL, 0, 0, (void *) CHR('c','a','n','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Cham"), NULL, 0, 0, (void *) CHR('c','h','a','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Cherokee"), NULL, 0, 0, (void *) CHR('c','h','e','r'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Cirth"), NULL, 0, 0, (void *) CHR('c','i','r','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("CJK Ideographic"), NULL, 0, 0, (void *) CHR('h','a','n','i'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Cypro-Minoan"), NULL, 0, 0, (void *) CHR('c','p','r','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Cypriot syllabary"), NULL, 0, 0, (void *) CHR('c','p','m','n'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Cyrillic"), NULL, 0, 0, (void *) CHR('c','y','r','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Default"), NULL, 0, 0, (void *) CHR('D','F','L','T'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Deseret (Mormon)"), NULL, 0, 0, (void *) CHR('d','s','r','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Devanagari"), NULL, 0, 0, (void *) CHR('d','e','v','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Egyptian demotic"), NULL, 0, 0, (void *) CHR('e','g','y','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Egyptian hieratic"), NULL, 0, 0, (void *) CHR('e','g','y','h'), NULL, false, false, false, false, false, false, true },
/* GT: Someone asked if FontForge actually was prepared generate hieroglyph output */
/* GT: because of this string. No. But OpenType and Unicode have placeholders for */
/* GT: dealing with these scripts against the day someone wants to use them. So */
/* GT: FontForge must be prepared to deal with those placeholders if nothing else. */
    { (unichar_t *) N_("Egyptian hieroglyphs"), NULL, 0, 0, (void *) CHR('e','g','y','p'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Ethiopic"), NULL, 0, 0, (void *) CHR('e','t','h','i'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Georgian"), NULL, 0, 0, (void *) CHR('g','e','o','r'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Glagolitic"), NULL, 0, 0, (void *) CHR('g','l','a','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Gothic"), NULL, 0, 0, (void *) CHR('g','o','t','h'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Greek"), NULL, 0, 0, (void *) CHR('g','r','e','k'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Gujarati"), NULL, 0, 0, (void *) CHR('g','u','j','r'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Gurmukhi"), NULL, 0, 0, (void *) CHR('g','u','r','u'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hangul Jamo"), NULL, 0, 0, (void *) CHR('j','a','m','o'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hangul"), NULL, 0, 0, (void *) CHR('h','a','n','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) NU_("Script|Hanunóo"), NULL, 0, 0, (void *) CHR('h','a','n','o'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Hebrew"), NULL, 0, 0, (void *) CHR('h','e','b','r'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Pahawh Hmong"), NULL, 0, 0, (void *) CHR('h','m','n','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Indus (Harappan)"), NULL, 0, 0, (void *) CHR('i','n','d','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Old Italic (Etruscan, Oscan, etc.)"), NULL, 0, 0, (void *) CHR('i','t','a','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Javanese"), NULL, 0, 0, (void *) CHR('j','a','v','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kayah Li"), NULL, 0, 0, (void *) CHR('k','a','l','i'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hiragana & Katakana"), NULL, 0, 0, (void *) CHR('k','a','n','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) NU_("Kharoṣṭhī"), NULL, 0, 0, (void *) CHR('k','h','a','r'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Kannada"), NULL, 0, 0, (void *) CHR('k','n','d','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Khmer"), NULL, 0, 0, (void *) CHR('k','h','m','r'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Lao") , NULL, 0, 0, (void *) CHR('l','a','o',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Latin"), NULL, 0, 0, (void *) CHR('l','a','t','n'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) NU_("Lepcha (Róng)"), NULL, 0, 0, (void *) CHR('l','e','p','c'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Limbu"), NULL, 0, 0, (void *) CHR('l','i','m','b'), NULL, false, false, false, false, false, false, true },	/* Not in ISO 15924 !!!!!, just guessing */
    { (unichar_t *) N_("Linear A"), NULL, 0, 0, (void *) CHR('l','i','n','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Linear B"), NULL, 0, 0, (void *) CHR('l','i','n','b'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Mandaean"), NULL, 0, 0, (void *) CHR('m','a','n','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mayan hieroglyphs"), NULL, 0, 0, (void *) CHR('m','a','y','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) NU_("Script|Malayālam"), NULL, 0, 0, (void *) CHR('m','l','y','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Mongolian"), NULL, 0, 0, (void *) CHR('m','o','n','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Myanmar"), NULL, 0, 0, (void *) CHR('m','y','m','r'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("N'Ko"), NULL, 0, 0, (void *) CHR('n','k','o',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ogham"), NULL, 0, 0, (void *) CHR('o','g','a','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Oriya"), NULL, 0, 0, (void *) CHR('o','r','y','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Osmanya"), NULL, 0, 0, (void *) CHR('o','s','m','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Pahlavi"), NULL, 0, 0, (void *) CHR('p','a','l','v'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Old Permic"), NULL, 0, 0, (void *) CHR('p','e','r','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Phoenician"), NULL, 0, 0, (void *) CHR('p','h','n','x'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Phaistos"), NULL, 0, 0, (void *) CHR('p','h','s','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Pollard Phonetic"), NULL, 0, 0, (void *) CHR('p','l','r','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Rongorongo"), NULL, 0, 0, (void *) CHR('r','o','r','o'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Runic"), NULL, 0, 0, (void *) CHR('r','u','n','r'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Shavian"), NULL, 0, 0, (void *) CHR('s','h','a','w'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Sinhala"), NULL, 0, 0, (void *) CHR('s','i','n','h'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Syriac"), NULL, 0, 0, (void *) CHR('s','y','r','c'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Tagbanwa"), NULL, 0, 0, (void *) CHR('t','a','g','b'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tai Le"), NULL, 0, 0, (void *) CHR('t','a','i','l'), NULL, false, false, false, false, false, false, true },	/* Not in ISO 15924 !!!!!, just guessing */
    { (unichar_t *) N_("Script|Tamil"), NULL, 0, 0, (void *) CHR('t','a','m','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Telugu"), NULL, 0, 0, (void *) CHR('t','e','l','u'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tengwar"), NULL, 0, 0, (void *) CHR('t','e','n','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tifinagh (Berber)"), NULL, 0, 0, (void *) CHR('t','f','n','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Tagalog"), NULL, 0, 0, (void *) CHR('t','g','l','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Thaana"), NULL, 0, 0, (void *) CHR('t','h','a','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Thai"), NULL, 0, 0, (void *) CHR('t','h','a','i'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Tibetan"), NULL, 0, 0, (void *) CHR('t','i','b','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Ugaritic"), NULL, 0, 0, (void *) CHR('u','g','r','t'), NULL, false, false, false, false, false, false, true },	/* Not in ISO 15924 !!!!!, just guessing */
    { (unichar_t *) N_("Script|Vai"), NULL, 0, 0, (void *) CHR('v','a','i',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Visible Speech"), NULL, 0, 0, (void *) CHR('v','i','s','p'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Cuneiform, Old Persian"), NULL, 0, 0, (void *) CHR('x','p','e','o'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Cuneiform, Sumero-Akkadian"), NULL, 0, 0, (void *) CHR('x','s','u','x'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Cuneiform, Ugaritic"), NULL, 0, 0, (void *) CHR('x','u','g','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Script|Yi")  , NULL, 0, 0, (void *) CHR('y','i',' ',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Private Use Script 1")  , NULL, 0, 0, (void *) CHR('q','a','a','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Private Use Script 2")  , NULL, 0, 0, (void *) CHR('q','a','a','b'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Undetermined Script")  , NULL, 0, 0, (void *) CHR('z','y','y','y'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Uncoded Script")  , NULL, 0, 0, (void *) CHR('z','z','z','z'), NULL, false, false, false, false, false, false, true },
    { NULL }
};

GTextInfo languages[] = {
    { (unichar_t *) N_("Abaza"), NULL, 0, 0, (void *) CHR('A','B','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Abkhazian"), NULL, 0, 0, (void *) CHR('A','B','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Adyghe"), NULL, 0, 0, (void *) CHR('A','D','Y',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Afrikaans"), NULL, 0, 0, (void *) CHR('A','F','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Afar"), NULL, 0, 0, (void *) CHR('A','F','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Agaw"), NULL, 0, 0, (void *) CHR('A','G','W',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Altai"), NULL, 0, 0, (void *) CHR('A','L','T',' '), NULL, false, false, false, false, false, false, true },
/* GT: See the long comment at "Property|New" */
/* GT: The msgstr should contain a translation of "Amharic", ignore "Lang|" */
    { (unichar_t *) N_("Lang|Amharic"), NULL, 0, 0, (void *) CHR('A','M','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Arabic"), NULL, 0, 0, (void *) CHR('A','R','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Aari"), NULL, 0, 0, (void *) CHR('A','R','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Arakanese"), NULL, 0, 0, (void *) CHR('A','R','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Assamese"), NULL, 0, 0, (void *) CHR('A','S','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Athapaskan"), NULL, 0, 0, (void *) CHR('A','T','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Avar"), NULL, 0, 0, (void *) CHR('A','V','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Awadhi"), NULL, 0, 0, (void *) CHR('A','W','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Aymara"), NULL, 0, 0, (void *) CHR('A','Y','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Azeri"), NULL, 0, 0, (void *) CHR('A','Z','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Badaga"), NULL, 0, 0, (void *) CHR('B','A','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Baghelkhandi"), NULL, 0, 0, (void *) CHR('B','A','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Balkar"), NULL, 0, 0, (void *) CHR('B','A','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Baule"), NULL, 0, 0, (void *) CHR('B','A','U',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Berber"), NULL, 0, 0, (void *) CHR('B','B','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bench"), NULL, 0, 0, (void *) CHR('B','C','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bible Cree"), NULL, 0, 0, (void *) CHR('B','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Belarussian"), NULL, 0, 0, (void *) CHR('B','E','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bemba"), NULL, 0, 0, (void *) CHR('B','E','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Bengali"), NULL, 0, 0, (void *) CHR('B','E','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bulgarian"), NULL, 0, 0, (void *) CHR('B','G','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bhili"), NULL, 0, 0, (void *) CHR('B','H','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bhojpuri"), NULL, 0, 0, (void *) CHR('B','H','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bikol"), NULL, 0, 0, (void *) CHR('B','I','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bilen"), NULL, 0, 0, (void *) CHR('B','I','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Blackfoot"), NULL, 0, 0, (void *) CHR('B','K','F',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Balochi"), NULL, 0, 0, (void *) CHR('B','L','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Balante"), NULL, 0, 0, (void *) CHR('B','L','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Balti"), NULL, 0, 0, (void *) CHR('B','L','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bambara"), NULL, 0, 0, (void *) CHR('B','M','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bamileke"), NULL, 0, 0, (void *) CHR('B','M','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Breton"), NULL, 0, 0, (void *) CHR('B','R','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Brahui"), NULL, 0, 0, (void *) CHR('B','R','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Braj Bhasha"), NULL, 0, 0, (void *) CHR('B','R','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Burmese"), NULL, 0, 0, (void *) CHR('B','R','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Bashkir"), NULL, 0, 0, (void *) CHR('B','S','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Beti"), NULL, 0, 0, (void *) CHR('B','T','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Catalan"), NULL, 0, 0, (void *) CHR('C','A','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Cebuano"), NULL, 0, 0, (void *) CHR('C','E','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chechen"), NULL, 0, 0, (void *) CHR('C','H','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chaha Gurage"), NULL, 0, 0, (void *) CHR('C','H','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chattisgarhi"), NULL, 0, 0, (void *) CHR('C','H','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chichewa"), NULL, 0, 0, (void *) CHR('C','H','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chukchi"), NULL, 0, 0, (void *) CHR('C','H','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chipewyan"), NULL, 0, 0, (void *) CHR('C','H','P',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Cherokee"), NULL, 0, 0, (void *) CHR('C','H','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chuvash"), NULL, 0, 0, (void *) CHR('C','H','U',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Comorian"), NULL, 0, 0, (void *) CHR('C','M','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Coptic"), NULL, 0, 0, (void *) CHR('C','O','P',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Cree"), NULL, 0, 0, (void *) CHR('C','R','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Carrier"), NULL, 0, 0, (void *) CHR('C','R','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Crimean Tatar"), NULL, 0, 0, (void *) CHR('C','R','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Church Slavonic"), NULL, 0, 0, (void *) CHR('C','S','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Czech"), NULL, 0, 0, (void *) CHR('C','S','Y',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Danish"), NULL, 0, 0, (void *) CHR('D','A','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Dargwa"), NULL, 0, 0, (void *) CHR('D','A','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Default"), NULL, 0, 0, (void *) DEFAULT_LANG, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Woods Cree"), NULL, 0, 0, (void *) CHR('D','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("German (Standard)"), NULL, 0, 0, (void *) CHR('D','E','U',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Dogri"), NULL, 0, 0, (void *) CHR('D','G','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Dhivehi"), NULL, 0, 0, (void *) CHR('D','H','V',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Djerma"), NULL, 0, 0, (void *) CHR('D','J','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Dangme"), NULL, 0, 0, (void *) CHR('D','N','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Default"), NULL, 0, 0, (void *) CHR('D','F','L','T'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Dinka"), NULL, 0, 0, (void *) CHR('D','N','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Dungan"), NULL, 0, 0, (void *) CHR('D','U','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Dzongkha"), NULL, 0, 0, (void *) CHR('D','Z','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ebira"), NULL, 0, 0, (void *) CHR('E','B','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Eastern Cree"), NULL, 0, 0, (void *) CHR('E','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Edo"), NULL, 0, 0, (void *) CHR('E','D','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Efik"), NULL, 0, 0, (void *) CHR('E','F','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Greek"), NULL, 0, 0, (void *) CHR('E','L','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("English"), NULL, 0, 0, (void *) CHR('E','N','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Erzya"), NULL, 0, 0, (void *) CHR('E','R','Z',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Spanish"), NULL, 0, 0, (void *) CHR('E','S','P',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Estonian"), NULL, 0, 0, (void *) CHR('E','T','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Basque"), NULL, 0, 0, (void *) CHR('E','U','Q',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Evenki"), NULL, 0, 0, (void *) CHR('E','V','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Even"), NULL, 0, 0, (void *) CHR('E','V','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ewe"), NULL, 0, 0, (void *) CHR('E','W','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("French Antillean"), NULL, 0, 0, (void *) CHR('F','A','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Farsi"), NULL, 0, 0, (void *) CHR('F','A','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Finnish"), NULL, 0, 0, (void *) CHR('F','I','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Fijian"), NULL, 0, 0, (void *) CHR('F','J','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Flemish"), NULL, 0, 0, (void *) CHR('F','L','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Forest Nenets"), NULL, 0, 0, (void *) CHR('F','N','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Fon"), NULL, 0, 0, (void *) CHR('F','O','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Faroese"), NULL, 0, 0, (void *) CHR('F','O','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("French (Standard)"), NULL, 0, 0, (void *) CHR('F','R','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Frisian"), NULL, 0, 0, (void *) CHR('F','R','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Friulian"), NULL, 0, 0, (void *) CHR('F','R','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Futa"), NULL, 0, 0, (void *) CHR('F','T','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Fulani"), NULL, 0, 0, (void *) CHR('F','U','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ga"), NULL, 0, 0, (void *) CHR('G','A','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Gaelic"), NULL, 0, 0, (void *) CHR('G','A','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Gagauz"), NULL, 0, 0, (void *) CHR('G','A','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Galician"), NULL, 0, 0, (void *) CHR('G','A','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Garshuni"), NULL, 0, 0, (void *) CHR('G','A','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Garhwali"), NULL, 0, 0, (void *) CHR('G','A','W',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Ge'ez"), NULL, 0, 0, (void *) CHR('G','E','Z',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Gilyak"), NULL, 0, 0, (void *) CHR('G','I','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Gumuz"), NULL, 0, 0, (void *) CHR('G','M','Z',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Gondi"), NULL, 0, 0, (void *) CHR('G','O','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Greenlandic"), NULL, 0, 0, (void *) CHR('G','R','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Garo"), NULL, 0, 0, (void *) CHR('G','R','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Guarani"), NULL, 0, 0, (void *) CHR('G','U','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Gujarati"), NULL, 0, 0, (void *) CHR('G','U','J',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Haitian"), NULL, 0, 0, (void *) CHR('H','A','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Halam"), NULL, 0, 0, (void *) CHR('H','A','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Harauti"), NULL, 0, 0, (void *) CHR('H','A','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hausa"), NULL, 0, 0, (void *) CHR('H','A','U',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hawaiin"), NULL, 0, 0, (void *) CHR('H','A','W',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hammer-Banna"), NULL, 0, 0, (void *) CHR('H','B','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hiligaynon"), NULL, 0, 0, (void *) CHR('H','I','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hindi"), NULL, 0, 0, (void *) CHR('H','I','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("High Mari"), NULL, 0, 0, (void *) CHR('H','M','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hindko"), NULL, 0, 0, (void *) CHR('H','N','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ho"), NULL, 0, 0, (void *) CHR('H','O',' ',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Harari"), NULL, 0, 0, (void *) CHR('H','R','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Croatian"), NULL, 0, 0, (void *) CHR('H','R','V',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hungarian"), NULL, 0, 0, (void *) CHR('H','U','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Armenian"), NULL, 0, 0, (void *) CHR('H','Y','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Igbo"), NULL, 0, 0, (void *) CHR('I','B','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ijo"), NULL, 0, 0, (void *) CHR('I','J','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ilokano"), NULL, 0, 0, (void *) CHR('I','L','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Indonesian"), NULL, 0, 0, (void *) CHR('I','N','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ingush"), NULL, 0, 0, (void *) CHR('I','N','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Inuktitut"), NULL, 0, 0, (void *) CHR('I','N','U',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Irish"), NULL, 0, 0, (void *) CHR('I','R','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Irish Traditional"), NULL, 0, 0, (void *) CHR('I','R','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Icelandic"), NULL, 0, 0, (void *) CHR('I','S','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Inari Sami"), NULL, 0, 0, (void *) CHR('I','S','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Italian"), NULL, 0, 0, (void *) CHR('I','T','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Hebrew"), NULL, 0, 0, (void *) CHR('I','W','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Javanese"), NULL, 0, 0, (void *) CHR('J','A','V',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Yiddish"), NULL, 0, 0, (void *) CHR('J','I','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Japanese"), NULL, 0, 0, (void *) CHR('J','A','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Judezmo"), NULL, 0, 0, (void *) CHR('J','U','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Jula"), NULL, 0, 0, (void *) CHR('J','U','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kabardian"), NULL, 0, 0, (void *) CHR('K','A','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kachchi"), NULL, 0, 0, (void *) CHR('K','A','C',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kalenjin"), NULL, 0, 0, (void *) CHR('K','A','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Kannada"), NULL, 0, 0, (void *) CHR('K','A','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Karachay"), NULL, 0, 0, (void *) CHR('K','A','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Georgian"), NULL, 0, 0, (void *) CHR('K','A','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kazakh"), NULL, 0, 0, (void *) CHR('K','A','Z',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kebena"), NULL, 0, 0, (void *) CHR('K','E','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Khutsuri Georgian"), NULL, 0, 0, (void *) CHR('K','G','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Khakass"), NULL, 0, 0, (void *) CHR('K','H','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Khanty-Kazim"), NULL, 0, 0, (void *) CHR('K','H','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Khmer"), NULL, 0, 0, (void *) CHR('K','H','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Khanty-Shurishkar"), NULL, 0, 0, (void *) CHR('K','H','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Khanty-Vakhi"), NULL, 0, 0, (void *) CHR('K','H','V',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Khowar"), NULL, 0, 0, (void *) CHR('K','H','W',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kikuyu"), NULL, 0, 0, (void *) CHR('K','I','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kirghiz"), NULL, 0, 0, (void *) CHR('K','I','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kisii"), NULL, 0, 0, (void *) CHR('K','I','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kokni"), NULL, 0, 0, (void *) CHR('K','K','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kalmyk"), NULL, 0, 0, (void *) CHR('K','L','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kamba"), NULL, 0, 0, (void *) CHR('K','M','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kumaoni"), NULL, 0, 0, (void *) CHR('K','M','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Komo"), NULL, 0, 0, (void *) CHR('K','M','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Komso"), NULL, 0, 0, (void *) CHR('K','M','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kanuri"), NULL, 0, 0, (void *) CHR('K','N','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kodagu"), NULL, 0, 0, (void *) CHR('K','O','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Konkani"), NULL, 0, 0, (void *) CHR('K','O','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kikongo"), NULL, 0, 0, (void *) CHR('K','O','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Komi-Permyak"), NULL, 0, 0, (void *) CHR('K','O','P',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Korean"), NULL, 0, 0, (void *) CHR('K','O','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Komi-Zyrian"), NULL, 0, 0, (void *) CHR('K','O','Z',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kpelle"), NULL, 0, 0, (void *) CHR('K','P','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Krio"), NULL, 0, 0, (void *) CHR('K','R','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Karakalpak"), NULL, 0, 0, (void *) CHR('K','R','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Karelian"), NULL, 0, 0, (void *) CHR('K','R','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Karaim"), NULL, 0, 0, (void *) CHR('K','R','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Karen"), NULL, 0, 0, (void *) CHR('K','R','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Koorete"), NULL, 0, 0, (void *) CHR('K','R','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kashmiri"), NULL, 0, 0, (void *) CHR('K','S','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Khasi"), NULL, 0, 0, (void *) CHR('K','S','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kildin Sami"), NULL, 0, 0, (void *) CHR('K','S','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kui"), NULL, 0, 0, (void *) CHR('K','U','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kulvi"), NULL, 0, 0, (void *) CHR('K','U','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kumyk"), NULL, 0, 0, (void *) CHR('K','U','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kurdish"), NULL, 0, 0, (void *) CHR('K','U','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kurukh"), NULL, 0, 0, (void *) CHR('K','U','U',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kuy"), NULL, 0, 0, (void *) CHR('K','U','Y',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Koryak"), NULL, 0, 0, (void *) CHR('K','Y','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ladin"), NULL, 0, 0, (void *) CHR('L','A','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lahuli"), NULL, 0, 0, (void *) CHR('L','A','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lak"), NULL, 0, 0, (void *) CHR('L','A','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lambani"), NULL, 0, 0, (void *) CHR('L','A','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Lao"), NULL, 0, 0, (void *) CHR('L','A','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Latin"), NULL, 0, 0, (void *) CHR('L','A','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Laz"), NULL, 0, 0, (void *) CHR('L','A','Z',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("L-Cree"), NULL, 0, 0, (void *) CHR('L','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ladakhi"), NULL, 0, 0, (void *) CHR('L','D','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lezgi"), NULL, 0, 0, (void *) CHR('L','E','Z',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lingala"), NULL, 0, 0, (void *) CHR('L','I','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Low Mari"), NULL, 0, 0, (void *) CHR('L','M','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Limbu"), NULL, 0, 0, (void *) CHR('L','M','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lomwe"), NULL, 0, 0, (void *) CHR('L','M','W',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lower Sorbian"), NULL, 0, 0, (void *) CHR('L','S','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lule Sami"), NULL, 0, 0, (void *) CHR('L','S','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lithuanian"), NULL, 0, 0, (void *) CHR('L','T','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Luba"), NULL, 0, 0, (void *) CHR('L','U','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Luganda"), NULL, 0, 0, (void *) CHR('L','U','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Luhya"), NULL, 0, 0, (void *) CHR('L','U','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Luo"), NULL, 0, 0, (void *) CHR('L','U','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Latvian"), NULL, 0, 0, (void *) CHR('L','V','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Majang"), NULL, 0, 0, (void *) CHR('M','A','J',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Makua"), NULL, 0, 0, (void *) CHR('M','A','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Malayalam Traditional"), NULL, 0, 0, (void *) CHR('M','A','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mansi"), NULL, 0, 0, (void *) CHR('M','A','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Marathi"), NULL, 0, 0, (void *) CHR('M','A','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Marwari"), NULL, 0, 0, (void *) CHR('M','A','W',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mbundu"), NULL, 0, 0, (void *) CHR('M','B','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Manchu"), NULL, 0, 0, (void *) CHR('M','C','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Moose Cree"), NULL, 0, 0, (void *) CHR('M','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mende"), NULL, 0, 0, (void *) CHR('M','D','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Me'en"), NULL, 0, 0, (void *) CHR('M','E','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mizo"), NULL, 0, 0, (void *) CHR('M','I','Z',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Macedonian"), NULL, 0, 0, (void *) CHR('M','K','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Male"), NULL, 0, 0, (void *) CHR('M','L','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Malagasy"), NULL, 0, 0, (void *) CHR('M','L','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Malinke"), NULL, 0, 0, (void *) CHR('M','L','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Malayalam Reformed"), NULL, 0, 0, (void *) CHR('M','L','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Malay"), NULL, 0, 0, (void *) CHR('M','L','Y',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mandinka"), NULL, 0, 0, (void *) CHR('M','N','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Mongolian"), NULL, 0, 0, (void *) CHR('M','N','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Manipuri"), NULL, 0, 0, (void *) CHR('M','N','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Maninka"), NULL, 0, 0, (void *) CHR('M','N','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Manx Gaelic"), NULL, 0, 0, (void *) CHR('M','N','X',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Moksha"), NULL, 0, 0, (void *) CHR('M','O','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Moldavian"), NULL, 0, 0, (void *) CHR('M','O','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mon"), NULL, 0, 0, (void *) CHR('M','O','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Moroccan"), NULL, 0, 0, (void *) CHR('M','O','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Maori"), NULL, 0, 0, (void *) CHR('M','R','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Maithili"), NULL, 0, 0, (void *) CHR('M','T','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Maltese"), NULL, 0, 0, (void *) CHR('M','T','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mundari"), NULL, 0, 0, (void *) CHR('M','U','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Naga-Assamese"), NULL, 0, 0, (void *) CHR('N','A','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Nanai"), NULL, 0, 0, (void *) CHR('N','A','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Naskapi"), NULL, 0, 0, (void *) CHR('N','A','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("N-Cree"), NULL, 0, 0, (void *) CHR('N','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ndebele"), NULL, 0, 0, (void *) CHR('N','D','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ndonga"), NULL, 0, 0, (void *) CHR('N','D','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Nepali"), NULL, 0, 0, (void *) CHR('N','E','P',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Newari"), NULL, 0, 0, (void *) CHR('N','E','W',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Norway House Cree"), NULL, 0, 0, (void *) CHR('N','H','C',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Nisi"), NULL, 0, 0, (void *) CHR('N','I','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Niuean"), NULL, 0, 0, (void *) CHR('N','I','U',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Nkole"), NULL, 0, 0, (void *) CHR('N','K','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Dutch"), NULL, 0, 0, (void *) CHR('N','L','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Nogai"), NULL, 0, 0, (void *) CHR('N','O','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Norwegian"), NULL, 0, 0, (void *) CHR('N','O','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Northern Sami"), NULL, 0, 0, (void *) CHR('N','S','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Northern Tai"), NULL, 0, 0, (void *) CHR('N','T','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Esperanto"), NULL, 0, 0, (void *) CHR('N','T','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Nynorsk"), NULL, 0, 0, (void *) CHR('N','Y','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Oji-Cree"), NULL, 0, 0, (void *) CHR('O','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ojibway"), NULL, 0, 0, (void *) CHR('O','J','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Oriya"), NULL, 0, 0, (void *) CHR('O','R','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Oromo"), NULL, 0, 0, (void *) CHR('O','R','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ossetian"), NULL, 0, 0, (void *) CHR('O','S','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Palestinian Aramaic"), NULL, 0, 0, (void *) CHR('P','A','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Pali"), NULL, 0, 0, (void *) CHR('P','A','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Punjabi"), NULL, 0, 0, (void *) CHR('P','A','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Palpa"), NULL, 0, 0, (void *) CHR('P','A','P',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Pashto"), NULL, 0, 0, (void *) CHR('P','A','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Polytonic Greek"), NULL, 0, 0, (void *) CHR('P','G','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Pilipino"), NULL, 0, 0, (void *) CHR('P','I','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Palaung"), NULL, 0, 0, (void *) CHR('P','L','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Polish"), NULL, 0, 0, (void *) CHR('P','L','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Provencal"), NULL, 0, 0, (void *) CHR('P','R','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Portuguese"), NULL, 0, 0, (void *) CHR('P','T','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chin"), NULL, 0, 0, (void *) CHR('Q','I','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Rajasthani"), NULL, 0, 0, (void *) CHR('R','A','J',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("R-Cree"), NULL, 0, 0, (void *) CHR('R','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Russian Buriat"), NULL, 0, 0, (void *) CHR('R','B','U',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Riang"), NULL, 0, 0, (void *) CHR('R','I','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Rhaeto-Romanic"), NULL, 0, 0, (void *) CHR('R','M','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Romanian"), NULL, 0, 0, (void *) CHR('R','O','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Romany"), NULL, 0, 0, (void *) CHR('R','O','Y',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Rusyn"), NULL, 0, 0, (void *) CHR('R','S','Y',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ruanda"), NULL, 0, 0, (void *) CHR('R','U','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Russian"), NULL, 0, 0, (void *) CHR('R','U','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sadri"), NULL, 0, 0, (void *) CHR('S','A','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sanskrit"), NULL, 0, 0, (void *) CHR('S','A','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Santali"), NULL, 0, 0, (void *) CHR('S','A','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sayisi"), NULL, 0, 0, (void *) CHR('S','A','Y',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sekota"), NULL, 0, 0, (void *) CHR('S','E','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Selkup"), NULL, 0, 0, (void *) CHR('S','E','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sango"), NULL, 0, 0, (void *) CHR('S','G','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Shan"), NULL, 0, 0, (void *) CHR('S','H','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sibe"), NULL, 0, 0, (void *) CHR('S','I','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sidamo"), NULL, 0, 0, (void *) CHR('S','I','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Silte Gurage"), NULL, 0, 0, (void *) CHR('S','I','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Skolt Sami"), NULL, 0, 0, (void *) CHR('S','K','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Slovak"), NULL, 0, 0, (void *) CHR('S','K','Y',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Slavey"), NULL, 0, 0, (void *) CHR('S','L','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Slovenian"), NULL, 0, 0, (void *) CHR('S','L','V',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Somali"), NULL, 0, 0, (void *) CHR('S','M','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Samoan"), NULL, 0, 0, (void *) CHR('S','M','O',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sena"), NULL, 0, 0, (void *) CHR('S','N','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sindhi"), NULL, 0, 0, (void *) CHR('S','N','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Sinhalese"), NULL, 0, 0, (void *) CHR('S','N','H',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Soninke"), NULL, 0, 0, (void *) CHR('S','N','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sodo Gurage"), NULL, 0, 0, (void *) CHR('S','O','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sotho"), NULL, 0, 0, (void *) CHR('S','O','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Albanian"), NULL, 0, 0, (void *) CHR('S','Q','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Serbian"), NULL, 0, 0, (void *) CHR('S','R','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Saraiki"), NULL, 0, 0, (void *) CHR('S','R','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Serer"), NULL, 0, 0, (void *) CHR('S','R','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("South Slavey"), NULL, 0, 0, (void *) CHR('S','S','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Southern Sami"), NULL, 0, 0, (void *) CHR('S','S','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Suri"), NULL, 0, 0, (void *) CHR('S','U','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Svan"), NULL, 0, 0, (void *) CHR('S','V','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Swedish"), NULL, 0, 0, (void *) CHR('S','V','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Swadaya Aramaic"), NULL, 0, 0, (void *) CHR('S','W','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Swahili"), NULL, 0, 0, (void *) CHR('S','W','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Swazi"), NULL, 0, 0, (void *) CHR('S','W','Z',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Sutu"), NULL, 0, 0, (void *) CHR('S','X','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Syriac"), NULL, 0, 0, (void *) CHR('S','Y','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tabasaran"), NULL, 0, 0, (void *) CHR('T','A','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tajiki"), NULL, 0, 0, (void *) CHR('T','A','J',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Tamil"), NULL, 0, 0, (void *) CHR('T','A','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tatar"), NULL, 0, 0, (void *) CHR('T','A','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("TH-Cree"), NULL, 0, 0, (void *) CHR('T','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Telugu"), NULL, 0, 0, (void *) CHR('T','E','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tongan"), NULL, 0, 0, (void *) CHR('T','G','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tigre"), NULL, 0, 0, (void *) CHR('T','G','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tigrinya"), NULL, 0, 0, (void *) CHR('T','G','Y',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Thai"), NULL, 0, 0, (void *) CHR('T','H','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tahitian"), NULL, 0, 0, (void *) CHR('T','H','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lang|Tibetan"), NULL, 0, 0, (void *) CHR('T','I','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Turkmen"), NULL, 0, 0, (void *) CHR('T','K','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Temne"), NULL, 0, 0, (void *) CHR('T','M','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tswana"), NULL, 0, 0, (void *) CHR('T','N','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tundra Nenets"), NULL, 0, 0, (void *) CHR('T','N','E',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tonga"), NULL, 0, 0, (void *) CHR('T','N','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Todo"), NULL, 0, 0, (void *) CHR('T','O','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Turkish"), NULL, 0, 0, (void *) CHR('T','R','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tsonga"), NULL, 0, 0, (void *) CHR('T','S','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Turoyo Aramaic"), NULL, 0, 0, (void *) CHR('T','U','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tulu"), NULL, 0, 0, (void *) CHR('T','U','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tuvin"), NULL, 0, 0, (void *) CHR('T','U','V',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Twi"), NULL, 0, 0, (void *) CHR('T','W','I',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Udmurt"), NULL, 0, 0, (void *) CHR('U','D','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ukrainian"), NULL, 0, 0, (void *) CHR('U','K','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Urdu"), NULL, 0, 0, (void *) CHR('U','R','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Upper Sorbian"), NULL, 0, 0, (void *) CHR('U','S','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Uyghur"), NULL, 0, 0, (void *) CHR('U','Y','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Uzbek"), NULL, 0, 0, (void *) CHR('U','Z','B',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Venda"), NULL, 0, 0, (void *) CHR('V','E','N',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Vietnamese"), NULL, 0, 0, (void *) CHR('V','I','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Wa"), NULL, 0, 0, (void *) CHR('W','A',' ',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Wagdi"), NULL, 0, 0, (void *) CHR('W','A','G',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("West-Cree"), NULL, 0, 0, (void *) CHR('W','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Welsh"), NULL, 0, 0, (void *) CHR('W','E','L',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Wolof"), NULL, 0, 0, (void *) CHR('W','L','F',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Xhosa"), NULL, 0, 0, (void *) CHR('X','H','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Yakut"), NULL, 0, 0, (void *) CHR('Y','A','K',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Yoruba"), NULL, 0, 0, (void *) CHR('Y','B','A',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Y-Cree"), NULL, 0, 0, (void *) CHR('Y','C','R',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Yi Classic"), NULL, 0, 0, (void *) CHR('Y','I','C',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Yi Modern"), NULL, 0, 0, (void *) CHR('Y','I','M',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chinese Phonetic"), NULL, 0, 0, (void *) CHR('Z','H','P',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chinese Simplified"), NULL, 0, 0, (void *) CHR('Z','H','S',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Chinese Traditional"), NULL, 0, 0, (void *) CHR('Z','H','T',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Zande"), NULL, 0, 0, (void *) CHR('Z','N','D',' '), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Zulu"), NULL, 0, 0, (void *) CHR('Z','U','L',' '), NULL, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo ligature_tags[] = {
    { (unichar_t *) N_("Ancient Ligatures"), NULL, 0, 0, (void *) CHR('a','l','i','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Discretionary Ligatures"), NULL, 0, 0, (void *) CHR('d','l','i','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Historic Ligatures"), NULL, 0, 0, (void *) CHR('h','l','i','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Required Ligatures"), NULL, 0, 0, (void *) CHR('r','l','i','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Standard Ligatures"), NULL, 0, 0, (void *) CHR('l','i','g','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Diagonal Fractions"), NULL, 0, 0, (void *) CHR('f','r','a','c'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Vertical Fractions"), NULL, 0, 0, (void *) CHR('a','f','r','c'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Above Base Substitutions"), NULL, 0, 0, (void *) CHR('a','b','v','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Below Base Forms"), NULL, 0, 0, (void *) CHR('b','l','w','f'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Below Base Substitutions"), NULL, 0, 0, (void *) CHR('b','l','w','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Akhand"), NULL, 0, 0, (void *) CHR('a','k','h','n'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Glyph Composition/Decomposition"), NULL, 0, 0, (void *) CHR('c','c','m','p'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Half Forms"), NULL, 0, 0, (void *) CHR('h','a','l','f'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Halant Forms"), NULL, 0, 0, (void *) CHR('h','a','l','n'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Leading Jamo Forms"), NULL, 0, 0, (void *) CHR('l','j','m','o'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Trailing Jamo Forms"), NULL, 0, 0, (void *) CHR('t','j','m','o'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Vowel Jamo Forms"), NULL, 0, 0, (void *) CHR('v','j','m','o'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Nukta Forms"), NULL, 0, 0, (void *) CHR('n','u','k','t'), NULL, false, false, false, false, false, false, true },	/* for numero */
    { (unichar_t *) N_("Ordinals"), NULL, 0, 0, (void *) CHR('o','r','d','n'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Pre Base Forms"), NULL, 0, 0, (void *) CHR('p','r','e','f'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Pre Base Substitutions"), NULL, 0, 0, (void *) CHR('p','r','e','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Post Base Forms"), NULL, 0, 0, (void *) CHR('p','s','t','f'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Post Base Substitutions"), NULL, 0, 0, (void *) CHR('p','s','t','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Reph Form"), NULL, 0, 0, (void *) CHR('r','p','h','f'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Vattu Variants"), NULL, 0, 0, (void *) CHR('v','a','t','u'), NULL, false, false, false, false, false, false, true },
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

GTextInfo simplepos_tags[] = {
    { (unichar_t *) N_("Case-Sensitive Forms"), NULL, 0, 0, (void *) CHR('c','a','s','e'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Capital Spacing"), NULL, 0, 0, (void *) CHR('c','p','s','p'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Full Widths"), NULL, 0, 0, (void *) CHR('f','w','i','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Alternative Half Widths"), NULL, 0, 0, (void *) CHR('h','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Half Widths"), NULL, 0, 0, (void *) CHR('h','w','i','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Left Bounds"), NULL, 0, 0, (void *) CHR('l','f','b','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Optical Bounds"), NULL, 0, 0, (void *) CHR('o','p','b','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Proportional Alternate Metrics"), NULL, 0, 0, (void *) CHR('p','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Quarter Widths"), NULL, 0, 0, (void *) CHR('q','w','i','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Right Bounds"), NULL, 0, 0, (void *) CHR('r','t','b','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Third Widths"), NULL, 0, 0, (void *) CHR('t','w','i','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Alternate Vertical Metrics"), NULL, 0, 0, (void *) CHR('v','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Alternate Vertical Half Metrics"), NULL, 0, 0, (void *) CHR('v','h','a','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Proportional Alternate Vertical Metrics"), NULL, 0, 0, (void *) CHR('v','p','a','l'), NULL, false, false, false, false, false, false, true },
/* Added by me so I can do round trip conversion of TeX tfm files */
    { (unichar_t *) N_("Italic Correction"), NULL, 0, 0, (void *) CHR('I','T','L','C'), NULL, false, false, false, false, false, false, true },
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

GTextInfo pairpos_tags[] = {
    { (unichar_t *) N_("Distance"), NULL, 0, 0, (void *) CHR('d','i','s','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Horizontal Kerning"), NULL, 0, 0, (void *) CHR('k','e','r','n'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Vertical Kerning"), NULL, 0, 0, (void *) CHR('v','k','r','n'), NULL, false, false, false, false, false, false, true },
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

GTextInfo simplesubs_tags[] = {
    { (unichar_t *) N_("Access All Alternates"), NULL, 0, 0, (void *) CHR('a','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Above Base Forms"), NULL, 0, 0, (void *) CHR('a','b','v','f'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Case-Sensitive Forms"), NULL, 0, 0, (void *) CHR('c','a','s','e'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Capitals to Petite Capitals"), NULL, 0, 0, (void *) CHR('c','2','p','c'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Capitals to Small Capitals"), NULL, 0, 0, (void *) CHR('c','2','s','c'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Denominators"), NULL, 0, 0, (void *) CHR('d','n','o','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Expert Forms"), NULL, 0, 0, (void *) CHR('e','x','p','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Terminal Forms"), NULL, 0, 0, (void *) CHR('f','i','n','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Diagonal Fractions"), NULL, 0, 0, (void *) CHR('f','r','a','c'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Full Widths"), NULL, 0, 0, (void *) CHR('f','w','i','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Historical Forms"), NULL, 0, 0, (void *) CHR('h','i','s','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Horizontal Kana Alternatives"), NULL, 0, 0, (void *) CHR('h','k','n','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hanja to Hangul"), NULL, 0, 0, (void *) CHR('h','n','g','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Half Widths"), NULL, 0, 0, (void *) CHR('h','w','i','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Initial Forms"), NULL, 0, 0, (void *) CHR('i','n','i','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Isolated Forms"), NULL, 0, 0, (void *) CHR('i','s','o','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Italics"), NULL, 0, 0, (void *) CHR('i','t','a','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("JIS78 Forms"), NULL, 0, 0, (void *) CHR('j','p','7','8'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("JIS83 Forms"), NULL, 0, 0, (void *) CHR('j','p','8','3'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("JIS90 Forms"), NULL, 0, 0, (void *) CHR('j','p','9','0'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lining Figures"), NULL, 0, 0, (void *) CHR('l','n','u','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Localized Forms"), NULL, 0, 0, (void *) CHR('l','o','c','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Medial Forms"), NULL, 0, 0, (void *) CHR('m','e','d','i'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mathematical Greek"), NULL, 0, 0, (void *) CHR('m','g','r','k'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Alternate Annotation Forms"), NULL, 0, 0, (void *) CHR('n','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Numerators"), NULL, 0, 0, (void *) CHR('n','u','m','r'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Oldstyle Figures"), NULL, 0, 0, (void *) CHR('o','n','u','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ornaments"), NULL, 0, 0, (void *) CHR('o','r','n','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lowercase to Petite Capitals"), NULL, 0, 0, (void *) CHR('p','c','a','p'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Proportional Numbers"), NULL, 0, 0, (void *) CHR('p','n','u','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Proportional Width"), NULL, 0, 0, (void *) CHR('p','w','i','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Quarter Widths"), NULL, 0, 0, (void *) CHR('q','w','i','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Right to Left Alternates"), NULL, 0, 0, (void *) CHR('r','t','l','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ruby Notational Forms"), NULL, 0, 0, (void *) CHR('r','u','b','y'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Stylistic Alternatives"), NULL, 0, 0, (void *) CHR('s','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Scientific Inferiors"), NULL, 0, 0, (void *) CHR('s','i','n','f'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Lowercase to Small Capitals"), NULL, 0, 0, (void *) CHR('s','m','c','p'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Simplified Forms"), NULL, 0, 0, (void *) CHR('s','m','p','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Style Set 1"), NULL, 0, 0, (void *) CHR('s','s','0','1'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Style Set 2"), NULL, 0, 0, (void *) CHR('s','s','0','2'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Style Set 3"), NULL, 0, 0, (void *) CHR('s','s','0','3'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Style Set 4"), NULL, 0, 0, (void *) CHR('s','s','0','4'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Style Set 5"), NULL, 0, 0, (void *) CHR('s','s','0','5'), NULL, false, false, false, false, false, false, true },
	/* These are documented to go up to Style Set 20, but I'm not going to bother will all of them until someone cares */
    { (unichar_t *) N_("Subscript"), NULL, 0, 0, (void *) CHR('s','u','b','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Superscript"), NULL, 0, 0, (void *) CHR('s','u','p','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Swash"), NULL, 0, 0, (void *) CHR('s','w','s','h'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Titling"), NULL, 0, 0, (void *) CHR('t','i','t','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Traditional Name Forms"), NULL, 0, 0, (void *) CHR('t','n','a','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Tabular Numbers"), NULL, 0, 0, (void *) CHR('t','n','u','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Traditional Forms"), NULL, 0, 0, (void *) CHR('t','r','a','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Third Widths"), NULL, 0, 0, (void *) CHR('t','w','i','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Unicase"), NULL, 0, 0, (void *) CHR('u','n','i','c'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Vertical Alternates (obs)"), NULL, 0, 0, (void *) CHR('v','e','r','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Vertical Rotation & Alternates"), NULL, 0, 0, (void *) CHR('v','r','t','2'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Vertical Kana Alternates"), NULL, 0, 0, (void *) CHR('v','k','n','a'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Slashed Zero"), NULL, 0, 0, (void *) CHR('z','e','r','o'), NULL, false, false, false, false, false, false, true },
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

GTextInfo alternatesubs_tags[] = {
    { (unichar_t *) N_("Access All Alternates"), NULL, 0, 0, (void *) CHR('a','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Final Glyph On Line"), NULL, 0, 0, (void *) CHR('f','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Hanja to Hangul"), NULL, 0, 0, (void *) CHR('h','n','g','l'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Justification Alternatives"), NULL, 0, 0, (void *) CHR('j','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("JIS78 Forms"), NULL, 0, 0, (void *) CHR('j','p','7','8'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Alternate Annotation Forms"), NULL, 0, 0, (void *) CHR('n','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ornaments"), NULL, 0, 0, (void *) CHR('o','r','n','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Randomize"), NULL, 0, 0, (void *) CHR('r','a','n','d'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Stylistic Alternatives"), NULL, 0, 0, (void *) CHR('s','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Swash"), NULL, 0, 0, (void *) CHR('s','w','s','h'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Traditional Forms"), NULL, 0, 0, (void *) CHR('t','r','a','d'), NULL, false, false, false, false, false, false, true },
/* My own invention, to provide data for tfm files for TeX's characters (like parens) which come in multiple sizes */
    { (unichar_t *) N_("TeX Glyphlist"), NULL, 0, 0, (void *) CHR('T','C','H','L'), NULL, false, false, false, false, false, false, true },
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo multiplesubs_tags[] = {
    { (unichar_t *) N_("Glyph Composition/Decomposition"), NULL, 0, 0, (void *) CHR('c','c','m','p'), NULL, false, false, false, false, false, false, true },
/* My own invention, to provide data for tfm files for TeX's characters (like parens) which can grow to an arbetrary size */
    { (unichar_t *) N_("TeX Extension List"), NULL, 0, 0, (void *) CHR('T','E','X','L'), NULL, false, false, false, false, false, false, true },
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo mark_tags[] = {
    { (unichar_t *) N_("Above Base Mark"), NULL, 0, 0, (void *) CHR('a','b','v','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Below Base Mark"), NULL, 0, 0, (void *) CHR('b','l','w','m'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mark Positioning"), NULL, 0, 0, (void *) CHR('m','a','r','k'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mark to Mark"), NULL, 0, 0, (void *) CHR('m','k','m','k'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Cursive Attachment"), NULL, 0, 0, (void *) CHR('c','u','r','s'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo contextualsubs_tags[] = {		/* 5 */
    { (unichar_t *) N_("Terminal Forms #2"), NULL, 0, 0, (void *) CHR('f','i','n','2'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Terminal Forms #3"), NULL, 0, 0, (void *) CHR('f','i','n','3'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Medial Forms 2"), NULL, 0, 0, (void *) CHR('m','e','d','2'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Mark Positioning via Substitution"), NULL, 0, 0, (void *) CHR('m','s','e','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Pre Base Substitutions"), NULL, 0, 0, (void *) CHR('p','r','e','s'), NULL, false, false, false, false, false, false, true },
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo contextualchainingsubs_tags[] = {	/* 6 */
    { (unichar_t *) N_("Contextual Alternates"), NULL, 0, 0, (void *) CHR('c','a','l','t'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ordinals"), NULL, 0, 0, (void *) CHR('o','r','d','n'), NULL, false, false, false, false, false, false, true },
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo reversechainingsubs_tags[] = {		/* 8 */
    { (unichar_t *) N_("Contextual Ligatures"), NULL, 0, 0, (void *) CHR('c','l','i','g'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Contextual Swash"), NULL, 0, 0, (void *) CHR('c','s','w','h'), NULL, false, false, false, false, false, false, true },
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

/* No recommended contextualpos_tags */
static GTextInfo contextualpos_tags[] = {		/* 7 */
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo contextualchainingpos_tags[] = {	/* 8 */
    { (unichar_t *) N_("Horizontal Kerning"), NULL, 0, 0, (void *) CHR('k','e','r','n'), NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Vertical Kerning"), NULL, 0, 0, (void *) CHR('v','k','r','n'), NULL, false, false, false, false, false, false, true },
/* My hack to identify required features */
    { (unichar_t *) N_("Required feature"), NULL, 0, 0, (void *) REQUIRED_FEATURE, NULL, false, false, false, false, false, false, true },
    { NULL }
};

static GTextInfo lcaret_tags[] = {		/* This really shouldn't be here, but it makes parsing the pst_tag array easier */
    { NULL }
};

GTextInfo *pst_tags[] = {
    simplepos_tags, pairpos_tags, simplesubs_tags,
    alternatesubs_tags, multiplesubs_tags,
    ligature_tags,
    lcaret_tags,	/* pst_lcaret */
    pairpos_tags,	/* pst_kerning */
    pairpos_tags,	/* pst_vkerning */
    mark_tags,		/* pst_anchor */
    contextualpos_tags, contextualsubs_tags,
    contextualchainingpos_tags,  contextualchainingsubs_tags,
    reversechainingsubs_tags, 
    NULL
};

static char *newstrings[] = { N_("New Position Variant"), N_("New Pair Position"),
	N_("New Substitution Variant"),
	N_("New Alternate List"), N_("New Multiple List"), N_("New Ligature"), NULL };
static char *editstrings[] = { N_("Edit Position Variant"), N_("Edit Pair Position"),
	N_("Edit Substitution Variant"),
	N_("Edit Alternate List"), N_("Edit Multiple List"), N_("Edit Ligature"), NULL };

static unichar_t monospace[] = { 'c','o','u','r','i','e','r',',','m', 'o', 'n', 'o', 's', 'p', 'a', 'c', 'e',',','c','a','s','l','o','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

int SFAddScriptLangIndex(SplineFont *sf,uint32 script,uint32 lang) {
    int i;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    if ( script==0 ) script=DEFAULT_SCRIPT;
    if ( lang==0 ) lang=DEFAULT_LANG;
    if ( sf->script_lang==NULL )
	sf->script_lang = gcalloc(2,sizeof(struct script_record *));
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	if ( sf->script_lang[i][0].script==script && sf->script_lang[i][1].script==0 &&
		sf->script_lang[i][0].langs[0]==lang &&
		sf->script_lang[i][0].langs[1]==0 )
return( i );
    }
    sf->script_lang = grealloc(sf->script_lang,(i+2)*sizeof(struct script_record *));
    sf->script_lang[i] = gcalloc(2,sizeof(struct script_record));
    sf->script_lang[i][0].script = script;
    sf->script_lang[i][0].langs = galloc(2*sizeof(uint32));
    sf->script_lang[i][0].langs[0] = lang;
    sf->script_lang[i][0].langs[1] = 0;
    sf->script_lang[i+1] = NULL;
    sf->sli_cnt = i+1;
return( i );
}

static void SFGuessScriptList(SplineFont *sf) {
    uint32 scripts[32], script;
    int i, scnt=0, j;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	script = SCScriptFromUnicode(sf->glyphs[i]);
	if ( script!=0 && script!=DEFAULT_SCRIPT ) {
	    for ( j=scnt-1; j>=0 ; --j )
		if ( scripts[j]==script )
	    break;
	    if ( j<0 ) {
		scripts[scnt++] = script;
		if ( scnt>=32 )
    break;
	    }
	}
    }
    if ( scnt==0 )
	scripts[scnt++] = CHR('l','a','t','n');

    /* order scripts */
    for ( i=0; i<scnt-1; ++i ) for ( j=i+1; j<scnt; ++j ) {
	if ( scripts[i]>scripts[j] ) {
	    script = scripts[i];
	    scripts[i] = scripts[j];
	    scripts[j] = script;
	}
    }

    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    if ( sf->script_lang!=NULL )
return;
    sf->script_lang = gcalloc(2,sizeof(struct script_record *));
    sf->script_lang[0] = gcalloc(scnt+1,sizeof(struct script_record));
    sf->sli_cnt = 1;
    for ( j=0; j<scnt; ++j ) {
	sf->script_lang[0][j].script = scripts[j];
	sf->script_lang[0][j].langs = galloc(2*sizeof(uint32));
	sf->script_lang[0][j].langs[0] = DEFAULT_LANG;
	sf->script_lang[0][j].langs[1] = 0;
    }
    sf->script_lang[1] = NULL;
}

static int SLContains(struct script_record *sr, uint32 script, uint32 lang) {
    int i, j;

    if ( script==DEFAULT_SCRIPT || script == 0 )
return( true );
    for ( i=0; sr[i].script!=0; ++i ) {
	if ( sr[i].script==script ) {
	    if ( lang==0 )
return( true );
	    for ( j=0; sr[i].langs[j]!=0; ++j )
		if ( sr[i].langs[j]==lang )
return( true );

return( false );	/* this script entry didn't contain the language. won't be any other scripts to check */
	}
    }
return( false );	/* Never found script */
}

static int SLCount(struct script_record *sr) {
    int sl_cnt = 0;
    int i,j;

    for ( i=0; sr[i].script!=0; ++i ) {
	for ( j=0; sr[i].langs[j]!=0; ++j )
	    ++sl_cnt;
    }
return( sl_cnt );
}

int SFFindBiggestScriptLangIndex(SplineFont *sf,uint32 script,uint32 lang) {
    int i, best_sli= -1, best_cnt= -1, cnt;

    if ( sf->script_lang==NULL )
	SFGuessScriptList(sf);
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	if ( SLContains(sf->script_lang[i],script,lang)) {
	    cnt = SLCount(sf->script_lang[i]);
	    if ( cnt>best_cnt ) {
		best_sli = i;
		best_cnt = cnt;
	    }
	}
    }
    if ( best_sli==-1 )
return( SFAddScriptLangIndex(sf,script,lang) );

return( best_sli );
}

int SRMatch(struct script_record *sr1,struct script_record *sr2) {
    int i, j;

    for ( i=0; sr1[i].script!=0 && sr2[i].script!=0 ; ++i ) {
	if ( sr1[i].script!=sr2[i].script )
return( false );
	for ( j=0 ; sr1[i].langs[j]!=0 && sr2[i].langs[j]!=0 ; ++j ) {
	    if ( sr1[i].langs[j]!= sr2[i].langs[j] )
return( false );
	}
	if ( sr1[i].langs[j]!=0 || sr2[i].langs[j]!=0 )
return( false );
    }
    if ( sr1[i].script!=0 || sr2[i].script!=0 )
return( false );

return( true );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int SRIsRightToLeft(struct script_record *sr) {
return( ScriptIsRightToLeft(sr[0].script) );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

int SFFindScriptLangRecord(SplineFont *sf,struct script_record *sr) {
    int i;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    if ( sf->script_lang==NULL )
return( -1 );
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	if ( SRMatch(sf->script_lang[i],sr))
return( i );
    }
return( -1 );
}

int SFAddScriptLangRecord(SplineFont *sf,struct script_record *sr) {
    int i;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    if ( sf->script_lang==NULL )
	sf->script_lang = gcalloc(2,sizeof(struct script_record *));
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	if ( SRMatch(sf->script_lang[i],sr)) {
	    ScriptRecordFree(sr);
return( i );
	}
    }
    sf->script_lang = grealloc(sf->script_lang,(i+2)*sizeof(struct script_record *));
    sf->script_lang[i] = sr;
    sf->script_lang[i+1] = NULL;
    sf->sli_cnt = i+1;
return( i );
}

int ScriptInSLI(SplineFont *sf,uint32 script,int sli) {
    struct script_record *sr;
    int i;

    if ( sli>=sf->sli_cnt || sli<0 )
return( false );
    /* some things, digits, marks of punctuation, etc. aren't really in any */
    /*  script because they are used by a lot of different scripts. So as long*/
    /*  as the sli is valid, assume they just work */
    if ( script==DEFAULT_SCRIPT )
return( true );

    sr = sf->script_lang[sli];
    for ( i=0; sr[i].script!=0; ++i )
	if ( sr[i].script==script )
return( true );

return( false );
}

int SCScriptInSLI(SplineFont *sf,SplineChar *sc,int sli) {
return( ScriptInSLI(sf,SCScriptFromUnicode(sc),sli));
}

unichar_t *ScriptLangLine(struct script_record *sr) {
    int i,j, tot=0;
    unichar_t *line, *pt;

    for ( i=0; sr[i].script!=0; ++i ) {
	for ( j=0; sr[i].langs[j]!=0; ++j );
	tot += j;
    }
    line = pt = galloc((7*i+tot*5+1)*sizeof(unichar_t));
    for ( i=0; sr[i].script!=0; ++i ) {
	*pt++ = sr[i].script>>24;
	*pt++ = (sr[i].script>>16)&0xff;
	*pt++ = (sr[i].script>>8)&0xff;
	*pt++ = sr[i].script&0xff;
	*pt++ = '{';
	for ( j=0; sr[i].langs[j]!=0; ++j ) {
	    *pt++ = sr[i].langs[j]>>24;
	    *pt++ = (sr[i].langs[j]>>16)&0xff;
	    *pt++ = (sr[i].langs[j]>>8)&0xff;
	    *pt++ = sr[i].langs[j]&0xff;
	    *pt++ = ',';
	}
	pt[-1] = '}';
	*pt++ = ' ';
    }
    *pt = '\0';
return( line );
}

int SCDefaultSLI(SplineFont *sf, SplineChar *default_script) {
    int def_sli;
    uint32 script;
    PST *pst;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    /* Try to guess a reasonable default */
    if ( default_script==(SplineChar *) -1 ) {
	default_script = NULL;
	def_sli = -1;
    } else {
	script = SCScriptFromUnicode(default_script);
	if ( sf->script_lang==NULL ) {
	    if ( script!=DEFAULT_SCRIPT && script!=0 )
		SFFindBiggestScriptLangIndex(sf,script,DEFAULT_LANG);
	    else
		SFGuessScriptList(sf);
	}
	def_sli = -1;
	if ( default_script!=NULL ) {
	    for ( pst=default_script->possub; pst!=NULL; pst=pst->next ) {
		if ( pst->type!=pst_lcaret && pst->script_lang_index!=SLI_NESTED ) {
		    def_sli = pst->script_lang_index;
	    break;
		}
	    }
	    if ( def_sli==-1 && default_script->kerns!=NULL )
		def_sli = default_script->kerns->sli;
	}
	if ( def_sli==-1 )
	    def_sli = SFFindBiggestScriptLangIndex(sf,script,DEFAULT_LANG);
    }
return( def_sli );
}

int SLICount(SplineFont *sf) {
    int i = 0;
    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    if ( sf->script_lang!=NULL )
	for ( i=0; sf->script_lang[i]!=NULL; ++i );
return( i );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static char *sli_names[] = { N_("Any Script"), N_(" -- Default --"), N_(" -- Nested --"), N_(" -- Edit Script & Lang List --"), NULL };
static int   sli_ud[] = { SLI_UNKNOWN, SLI_UNKNOWN, SLI_NESTED, -1 };

static GTextInfo *SFMarkClassList(SplineFont *sf,int class) {
    int i;
    GTextInfo *ti;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    i = sf->mark_class_cnt;
    ti = gcalloc(i+4,sizeof( GTextInfo ));
    ti[0].text = utf82u_copy( _("All"));
    ti[0].selected = class==0;
    for ( i=1; i<sf->mark_class_cnt; ++i ) {
	ti[i].text = (unichar_t *) copy(sf->mark_class_names[i]);
	ti[i].text_is_1byte = true;
	if ( i==class ) ti[i].selected = true;
    }
return( ti );
}

GTextInfo *SFLangList(SplineFont *sf,int addfinal,SplineChar *default_script) {
    int i,j,k,bit;
    GTextInfo *ti;
    int def_sli;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    def_sli = SCDefaultSLI(sf,default_script);

    i = 0;
    if ( sf->script_lang!=NULL )
	for ( i=0; sf->script_lang[i]!=NULL; ++i );
    ti = gcalloc(i+4,sizeof( GTextInfo ));
    if ( sf->script_lang!=NULL )
	for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	    ti[i].text = ScriptLangLine(sf->script_lang[i]);
	    ti[i].userdata = (void *) (intpt) i;
	}
    if ( def_sli!=-1 && def_sli<i )
	ti[def_sli].selected = true;
    for ( k=0, j=sizeof(sli_names)/sizeof(sli_names[0])-2, bit = 1<<j; j>=0; --j, ++k, bit>>=1 ) {
	if ( addfinal&bit ) {
	    ti[i].text = utf82u_copy(_(sli_names[k]));
	    ti[i].userdata = (void *) (intpt) sli_ud[k];
	    if ( sli_ud[k]==def_sli )
		ti[i].selected = true;
	    ++i;
	}
    }
return( ti );
}

GTextInfo **SFLangArray(SplineFont *sf,int addfinal) {
    int i, bit, j, k;
    GTextInfo **ti;

    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;
    for ( i=0; sf->script_lang[i]!=NULL; ++i );
    ti = gcalloc(i+4,sizeof( GTextInfo * ));
    for ( i=0; sf->script_lang[i]!=NULL; ++i ) {
	ti[i] = gcalloc(1,sizeof( GTextInfo));
	ti[i]->text = ScriptLangLine(sf->script_lang[i]);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	ti[i]->userdata = (void *) (intpt) i;
    }
    for ( k=0, j=sizeof(sli_names)/sizeof(sli_names[0])-2, bit = 1<<j; j>=0; --j, ++k, bit>>=1 ) {
	if ( addfinal&bit ) {
	    ti[i] = gcalloc(1,sizeof( GTextInfo));
	    ti[i]->text = utf82u_copy(sli_names[k]);
	    ti[i]->userdata = (void *) (intpt) sli_ud[k];
	    ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	    ++i;
	}
    }
    ti[i] = gcalloc(1,sizeof( GTextInfo));
return( ti );
}

struct sl_dlg {
    int done;
    int sel;
    SplineFont *sf;
    GGadget *list;
};
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

struct script_record *SRParse(const unichar_t *line) {
    int scnt, lcnt, i, j, k;
    const unichar_t *pt, *start, *lpt;
    struct script_record *sr, srtemp;
    unichar_t tag[6];

    if ( line==NULL )
return( NULL );

    for ( scnt=0, pt=line; *pt; ++pt )
	if ( *pt=='{' ) ++scnt;
    sr = gcalloc(scnt+1,sizeof(struct script_record));
    for ( i=0, pt=line; i<scnt && *pt; ++i ) {
	start = pt;
	while ( *pt!='{' && *pt!='\0' ) ++pt;
	if ( pt-start<4 ) {
	    tag[0] = tag[1] = tag[2] = tag[3] = ' ';
	    if ( start<pt ) tag[0] = *start;
	    if ( start+1<pt ) tag[1] = start[1];
	    if ( start+2<pt ) tag[2] = start[2];
	    if ( start+3<pt ) tag[3] = start[3];
	    start = tag;
	}
	if ( *pt!='{' ) {
	    ScriptRecordFree(sr);
	    gwwv_post_error(_("Cant Parse Scripts"),_("A script language series consists of a list of\n4 letter script tags each followed by a '{'\nand a sub-series of 4 letter language tags separated\nby commas and ended with '}'. As:\nlatn{DEU ,dflt} cyrl{dflt}"));
return( NULL );
	}
	sr[i].script = (start[0]<<24) | ((start[1]&0xff)<<16) | ((start[2]&0xff)<<8) | (start[3]&0xff);
	for ( lcnt=1, lpt=pt+1; *lpt!='}' && *lpt!='\0'; ++lpt )
	    if ( *lpt==',' ) ++lcnt;
	if ( *lpt=='\0' ) {
	    ScriptRecordFree(sr);
	    gwwv_post_error(_("Cant Parse Scripts"),_("A script language series consists of a list of\n4 letter script tags each followed by a '{'\nand a sub-series of 4 letter language tags separated\nby commas and ended with '}'. As:\nlatn{DEU ,dflt} cyrl{dflt}"));
return( NULL );
	}
	sr[i].langs = galloc((lcnt+1)*sizeof(uint32));
	for ( j=0, lpt=start=pt+1; j<lcnt ; ++lpt ) {
	    if ( *lpt==',' || *lpt=='}' ) {
		if ( lpt-start<4 ) {
		    tag[0] = tag[1] = tag[2] = tag[3] = ' ';
		    if ( start<lpt ) tag[0] = *start;
		    if ( start+1<lpt ) tag[1] = start[1];
		    if ( start+2<lpt ) tag[2] = start[2];
		    if ( start+3<lpt ) tag[3] = start[3];
		    start = tag;
		}
		sr[i].langs[j++] = (start[0]<<24) | ((start[1]&0xff)<<16) | ((start[2]&0xff)<<8) | (start[3]&0xff);
		start = lpt+1;
		if ( *lpt=='}' ) {
		    sr[i].langs[j] = 0;
	break;
		}
	    }
	}
	pt = lpt+1;
	while ( pt[0]==' ' ) ++pt;
    }
    sr[i].script = 0;

    if ( *pt!='\0' ) {
	ScriptRecordFree(sr);
	gwwv_post_error(_("Cant Parse Scripts"),_("A script language series consists of a list of\n4 letter script tags each followed by a '{'\nand a sub-series of 4 letter language tags separated\nby commas and ended with '}'. As:\nlatn{DEU ,dflt} cyrl{dflt}"));
return( NULL );
    }

    /* Order it properly */
    if ( sr[0].script!=0 ) 
    for ( i=0; sr[i+1].script!=0; ++i ) for ( j=i+1; sr[j].script!=0; ++j ) {
	if ( sr[i].script>sr[j].script ) {
	    srtemp = sr[i];
	    sr[i] = sr[j];
	    sr[j] = srtemp;
	}
    }
    for ( k=0; sr[k].script!=0; ++k ) {
	struct script_record *srpt = &sr[k];
	if ( srpt->langs[0]!=0 )
	for ( i=0; srpt->langs[i+1]!=0; ++i ) for ( j=i+1; srpt->langs[j]!=0; ++j ) {
	    if ( srpt->langs[i]>srpt->langs[j] ) {
		uint32 temp = srpt->langs[i];
		srpt->langs[i] = srpt->langs[j];
		srpt->langs[j] = temp;
	    }
	}
    }

return( sr );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static struct script_record **SRParseList(GGadget *list,int *sel) {
    int32 len, i;
    GTextInfo **ti = GGadgetGetList(list,&len);
    struct script_record **sr;

    *sel = -1;
    sr = gcalloc(len+1,sizeof(struct script_record *));
    for ( i=0; i<len; ++i ) {
	sr[i] = SRParse(ti[i]->text);
	if ( sr[i]==NULL ) {
	    ScriptRecordListFree(sr);
return( NULL );
	}
	if ( ti[i]->selected )
	    *sel = i;
    }
return( sr );
}

#define CID_Ok		1

static int sl_e_h(GWindow gw, GEvent *event) {
    int *done = GDrawGetUserData(gw);

    if ( event->type==et_close ) {
	*done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html#Language");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	switch ( GGadgetGetCid(event->u.control.g)) {
	  case CID_Ok:
	    *done = 2;
	  break;
	  case CID_Cancel:
	    *done = true;
	  break;
	}
    }
return( true );
}

static uint32 *ShowLanguages(uint32 *langs) {
    int i,j,done=0;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[5], *varray[5], *harray[7], boxes[3];
    GTextInfo label[5];
    GRect pos;
    GWindow gw;
    int32 len;
    GTextInfo **ti;
    const int width = 150;
    uint32 *ret, *pt;
    int warned=0;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Language List");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,width));
	pos.height = GDrawPointsToPixels(NULL,193);
	gw = GDrawCreateTopWindow(NULL,&pos,sl_e_h,&done,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));
	memset(&label,0,sizeof(label));

	for ( i=0; languages[i].text!=NULL; ++i )
	    languages[i].selected = false;
	for ( i=0; langs[i]!=0; ++i ) {
	    for ( j=0; languages[j].text!=NULL && (uint32) (intpt) languages[j].userdata!=langs[i]; ++j );
	    if ( languages[j].text!=NULL )
		languages[j].selected = true;
	    else if ( !warned ) {
		gwwv_post_error(_("Language Missing"),_("Could not find a language to match '%c%c%c%c'.\nIt has been omitted from this list.\nIf that is not desirable, press [Cancel] twice,\nand then hold down the control key and press [Edit]."),
			langs[i]>>24,
			(langs[i]>>16)&0xff,
			(langs[i]>>8)&0xff,
			langs[i]&0xff);
		warned = true;
	    }
	}

	i = 0;
	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 5;
	gcd[i].gd.pos.width = width-20;
	gcd[i].gd.pos.height = 12*12+6;
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_list_alphabetic|gg_list_multiplesel;
	gcd[i].gd.u.list = languages;
	gcd[i].gd.cid = 0;
	varray[0] = &gcd[i]; varray[1] = NULL;
	gcd[i++].creator = GListCreate;

	gcd[i].gd.pos.x = 15-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+gcd[i-1].gd.pos.height+5;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.mnemonic = 'O';
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = CID_Ok;
	harray[0] = GCD_Glue; harray[1] = &gcd[i]; harray[2] = GCD_Glue;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.mnemonic = 'C';
	gcd[i].gd.cid = CID_Cancel;
	harray[3] = GCD_Glue; harray[4] = &gcd[i]; harray[5] = GCD_Glue; harray[6] = NULL;
	gcd[i++].creator = GButtonCreate;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray;
	boxes[2].creator = GHBoxCreate;
	varray[2] = &boxes[2]; varray[3] = NULL; varray[4] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,0);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw,true);
 retry:
    while ( !done )
	GDrawProcessOneEvent(NULL);
    ret = NULL;
    ti = GGadgetGetList(gcd[0].ret,&len);
    if ( done==2 ) {
	int lcnt=0;
	for ( i=0; i<len; ++i ) {
	    if ( ti[i]->selected )
		++lcnt;
	}
	if ( lcnt==0 ) {
	    gwwv_post_error(_("Language Missing"),_("You must select at least one language.\nUse the \"Default\" language if nothing else fits."));
	    done = 0;
 goto retry;
	}
	ret = galloc((lcnt+1)*sizeof(uint32));
	*ret = '\0';
    }
    pt = ret;
    for ( i=0; i<len; ++i ) {
	if ( done==2 && ti[i]->selected )
	    *pt++ = (uint32) (intpt) (ti[i]->userdata);
    }
    if ( pt!=ret )
	*pt = 0;
    GDrawDestroyWindow(gw);
return( ret );
}

static int FromScriptToList(GTextInfo **ti,int sindex) {
    /* We've got an index in the script array. We want an index in the ti array*/
    /* everything in ti should be in script and vice-versa, but ti is in */
    /* alphabetic order (which may not be english ordering) */
    int i;
    unichar_t *match = utf82u_copy((char *) scripts[sindex].text);

    for ( i=0; ti[i]->text!=NULL; ++i ) {
	if ( u_strcmp(ti[i]->text,match)==0 ) {
	    free(match);
return( i );
	}
    }
    free(match);
    IError("Failed to find corresponding script S2L" );
return( -1 );
}

static int FromListToScript(GTextInfo **ti,int tindex) {
    /* We've got an index in the script array. We want an index in the ti array*/
    /* everything in ti should be in script and vice-versa, but ti is in */
    /* alphabetic order (which may not be english ordering) */
    int i;

    for ( i=0; scripts[i].text!=NULL; ++i ) {
	unichar_t *match = utf82u_copy((char *) scripts[i].text);
	if ( u_strcmp(ti[tindex]->text,match)==0 ) {
	    free(match);
return( i );
	}
	free(match);
    }
    IError("Failed to find corresponding script L2S" );
return( -1 );
}

static int ss_e_h(GWindow gw, GEvent *event) {
    int *done = GDrawGetUserData(gw);
    GTextInfo *ti;

    if ( event->type==et_close ) {
	*done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html#Script");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_listselected ) {
	if ( event->u.control.u.list.changed_index>=0 ) {
	    ti = GGadgetGetListItem(event->u.control.g,event->u.control.u.list.changed_index);
	    if ( ti!=NULL && ti->selected && ti->userdata == NULL ) {
		ti->userdata = gcalloc(2,sizeof(uint32));
		((uint32 *) (ti->userdata))[0] = DEFAULT_LANG;
	    }
	}
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_listdoubleclick ) {
	if ( event->u.control.u.list.changed_index>=0 ) {
	    ti = GGadgetGetListItem(event->u.control.g,event->u.control.u.list.changed_index);
	    if ( ti!=NULL && ti->userdata!=NULL ) {
		uint32 *ret =ShowLanguages(ti->userdata);
		if ( ret!=NULL ) {
		    free(ti->userdata);
		    ti->userdata = ret;
		}
	    }
	}
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	switch ( GGadgetGetCid(event->u.control.g)) {
	  case CID_Ok:
	    *done = 2;
	  break;
	  case CID_Cancel:
	    *done = true;
	  break;
	}
    }
return( true );
}

unichar_t *ShowScripts(unichar_t *usedef) {
    struct script_record *sr = SRParse(usedef);
    static struct script_record dummy = { 0 };
    int i,j,done=0, first;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[5], *varray[5], *harray[7], boxes[3];
    GTextInfo label[5];
    GRect pos;
    GWindow gw;
    int warned = false;
    int32 len;
    GTextInfo **ti;
    const int width = 150;
    unichar_t *ret, *pt;

    CharInfoInit();

    if ( sr==NULL ) sr = &dummy;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Script List");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,width));
	pos.height = GDrawPointsToPixels(NULL,193);
	gw = GDrawCreateTopWindow(NULL,&pos,ss_e_h,&done,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));
	memset(&label,0,sizeof(label));

	i = 0;
	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 5;
	gcd[i].gd.pos.width = width-20;
	gcd[i].gd.pos.height = 12*12+6;
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_list_alphabetic|gg_list_multiplesel;
	gcd[i].gd.u.list = scripts;
	gcd[i].gd.cid = 0;
	varray[0] = &gcd[i]; varray[1] = NULL;
	gcd[i++].creator = GListCreate;

	gcd[i].gd.pos.x = 15-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+gcd[i-1].gd.pos.height+5;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.mnemonic = 'O';
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = CID_Ok;
	harray[0] = GCD_Glue; harray[1] = &gcd[i]; harray[2] = GCD_Glue;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.mnemonic = 'C';
	gcd[i].gd.cid = CID_Cancel;
	harray[3] = GCD_Glue; harray[4] = &gcd[i]; harray[5] = GCD_Glue; harray[6] = NULL;
	gcd[i++].creator = GButtonCreate;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray;
	boxes[2].creator = GHBoxCreate;
	varray[2] = &boxes[2]; varray[3] = NULL; varray[4] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,0);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);

	ti = GGadgetGetList(gcd[0].ret,&len);
	for ( i=0; i<len; ++i ) {
	    ti[i]->selected = false;
	    ti[i]->userdata = NULL;
	}
	first = -1;
	for ( i=0; sr[i].script!=0; ++i ) {
	    for ( j=0; j<len && (uint32) (intpt) scripts[j].userdata!=sr[i].script; ++j );
	    if ( j<len ) {
		int jj = FromScriptToList(ti,j);
		if ( first==-1 || jj<first ) first = jj;
		ti[jj]->selected = true;
		ti[jj]->userdata = sr[i].langs;
		sr[i].langs = NULL;
	    } else if ( !warned ) {
		gwwv_post_error(_("Script Missing"),_("Could not find a script to match '%c%c%c%c'.\nIt has been omitted from this list.\nIf that is not desirable, press [Cancel],\nand then hold down the control key and press [Edit]."),
			sr[i].script>>24,
			(sr[i].script>>16)&0xff,
			(sr[i].script>>8)&0xff,
			sr[i].script&0xff);
		warned = true;
	    }
	}
	if ( first!=-1 )
	    GGadgetScrollListToPos(gcd[0].ret,first);
	if ( sr!=&dummy )
	    ScriptRecordFree(sr);

    GDrawSetVisible(gw,true);
 retry:
    while ( !done )
	GDrawProcessOneEvent(NULL);
    ret = NULL;
    ti = GGadgetGetList(gcd[0].ret,&len);
    if ( done==2 ) {
	int scnt=0, lcnt=0;
	for ( i=0; i<len; ++i ) {
	    if ( ti[i]->selected ) {
		++scnt;
		for ( j=0; ((uint32 *) (ti[i]->userdata))[j]!=0; ++j );
		lcnt += j;
	    }
	}
	if ( scnt==0 ) {
	    gwwv_post_error(_("Script Missing"),_("You must select at least one script"));
	    done = 0;
 goto retry;
	}
	ret = galloc((7*scnt+5*lcnt+3)*sizeof(unichar_t));
	*ret = '\0';
    }
    pt = ret;
    for ( i=0; i<len; ++i ) {
	if ( done==2 && ti[i]->selected ) {
	    int ii = FromListToScript(ti,i);
	    *pt++ = ((uint32) (intpt) (scripts[ii].userdata))>>24;
	    *pt++ = (((uint32) (intpt) (scripts[ii].userdata))>>16)&0xff;
	    *pt++ = (((uint32) (intpt) (scripts[ii].userdata))>>8)&0xff;
	    *pt++ = ((uint32) (intpt) (scripts[ii].userdata))&0xff;
	    *pt++ = '{';
	    for ( j=0; ((uint32 *) (ti[i]->userdata))[j]!=0; ++j ) {
		*pt++ = ((uint32 *) (ti[i]->userdata))[j]>>24;
		*pt++ = (((uint32 *) (ti[i]->userdata))[j]>>16)&0xff;
		*pt++ = (((uint32 *) (ti[i]->userdata))[j]>>8)&0xff;
		*pt++ = ((uint32 *) (ti[i]->userdata))[j]&0xff;
		*pt++ = ',';
	    }
	    pt[-1]= '}';
	    *pt++ = ' ';
	}
	free(ti[i]->userdata);
    }
    if ( pt!=ret )
	pt[-1] = '\0';
    GDrawDestroyWindow(gw);
return( ret );
}

static void SLL_DoChange(struct sl_dlg *sld,unichar_t *def,GEvent *e) {
    struct script_record *sr;
    char *cret;
    unichar_t *ret;
    unichar_t *usedef = def;

    forever {
	if ( e!=NULL && ( e->u.control.u.button.button!=1 ||
		(e->u.control.u.button.state&(ksm_control|ksm_meta|ksm_capslock|ksm_shift))) ) {
	    char *cdef = u2utf8_copy(usedef);
	    cret = gwwv_ask_string(_("Script & Language"),cdef,_("Enter a list of scripts each followed by a list of languages"));
	    ret = utf82u_copy(cret);
	    free(cdef);
	    free(cret);
	} else
	    ret = ShowScripts(usedef);
	if ( usedef!=def )
	    free(usedef);
	if ( ret==NULL )
return;
	sr = SRParse(ret);
	if ( sr!=NULL ) {
	    ScriptRecordFree(sr);
	    if ( def==NULL )
		GListAppendLine(sld->list,ret,true);
	    else
		GListChangeLine(sld->list,GGadgetGetFirstListSelectedItem(sld->list),ret);
	    free(ret);
return;
	}
	usedef = ret;
    }
}

static int sld_e_h(GWindow gw, GEvent *event) {
    struct sl_dlg *sld = GDrawGetUserData(gw);
    struct script_record **srl;
    GTextInfo *ti;
    int i;

    if ( event->type==et_close ) {
	sld->done = true;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html#ScriptLang");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_listselected ) {
	GGadgetSetEnabled(GWidgetGetControl(gw,CID_Edit),
		GGadgetGetFirstListSelectedItem(sld->list)!=-1);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_listdoubleclick ) {
	ti = GGadgetGetListItemSelected(sld->list);
	if ( ti!=NULL )
	    SLL_DoChange(sld,ti->text,NULL);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	switch ( GGadgetGetCid(event->u.control.g)) {
	  case CID_Ok:
	    if ( (srl = SRParseList(sld->list,&sld->sel))!=NULL ) {
		sld->done = true;
		ScriptRecordListFree(sld->sf->script_lang);
		sld->sf->script_lang = srl;
		if ( srl!=NULL ) {
		    for ( i=0; srl[i]!=NULL; ++i );
		    sld->sf->sli_cnt = i;
		} else
		    sld->sf->sli_cnt = 0;
	    }
	  break;
	  case CID_Cancel:
	    sld->done = true;
	    sld->sel = -2;
	  break;
	  case CID_New:
	    SLL_DoChange(sld,NULL,event);
	  break;
	  case CID_Edit:
	    ti = GGadgetGetListItemSelected(sld->list);
	    if ( ti!=NULL )
		SLL_DoChange(sld,ti->text,event);
	  break;
	}
    }
return( true );
}

int ScriptLangList(SplineFont *sf,GGadget *list,int sli) {
    int32 len;
    GTextInfo **ti = GGadgetGetList(list,&len);
    struct sl_dlg sld;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[8], *varray[9], *harray1[7], *harray2[8], boxes[4];
    GTextInfo label[8];
    GRect pos;
    GWindow gw;
    FontRequest rq;
    GFont *font;
    int i;
    const int width = 300;

    CharInfoInit();

    if ( len>=2 && ti[len-2]->selected && ti[len-2]->userdata==(void *) SLI_NESTED )
return(false);
    if ( len>=1 && !ti[len-1]->selected )
return(true);
    /* the last entry in the script/lang pulldown is the one that allows them */
    /*  to edit the script lang list. That's what the above check is for */

	memset(&sld,0,sizeof(sld));
	sld.sf = sf;
	sld.sel = -2;
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Script & Language List");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,width));
	pos.height = GDrawPointsToPixels(NULL,183);
	gw = GDrawCreateTopWindow(NULL,&pos,sld_e_h,&sld,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));
	memset(&label,0,sizeof(label));

	i = 0;
	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 5;
	gcd[i].gd.pos.width = width-20;
	gcd[i].gd.pos.height = 8*12+10;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.list = SFLangList(sf,false,(SplineChar *) -1);
	gcd[i].gd.cid = i+1;
	varray[0] = &gcd[i]; varray[1] = NULL;
	gcd[i++].creator = GListCreate;

	gcd[i].gd.pos.x = 30; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+gcd[i-1].gd.pos.height+5;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled;
	label[i].text = (unichar_t *) _("_New...");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = CID_New;
	harray1[0] = GCD_Glue; harray1[1] = &gcd[i]; harray1[2] = GCD_Glue;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -30; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible /*| gg_enabled*/;
	label[i].text = (unichar_t *) _("_Edit");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = CID_Edit;
	harray1[3] = GCD_Glue; harray1[4] = &gcd[i]; harray1[5] = GCD_Glue; harray1[6] = NULL;
	gcd[i++].creator = GButtonCreate;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray1;
	boxes[2].creator = GHBoxCreate;
	varray[2] = &boxes[2]; varray[3] = NULL;

	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+28;
	gcd[i].gd.pos.width = width-10; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible ;
	varray[4] = &gcd[i]; varray[5] = NULL;
	gcd[i++].creator = GLineCreate;

	gcd[i].gd.pos.x = 15-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+5;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.mnemonic = 'O';
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = CID_Ok;
	harray2[0] = GCD_Glue; harray2[1] = &gcd[i]; harray2[2] = GCD_Glue;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.mnemonic = 'C';
	gcd[i].gd.cid = CID_Cancel;
	harray2[3] = GCD_Glue; harray2[4] = GCD_Glue; harray2[5] = &gcd[i]; harray2[6] = GCD_Glue; harray2[7] = NULL;
	gcd[i++].creator = GButtonCreate;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = harray2;
	boxes[3].creator = GHBoxCreate;
	varray[6] = &boxes[3]; varray[7] = NULL; varray[8] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,0);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);

	GTextInfoListFree(gcd[0].gd.u.list);
	memset(&rq,0,sizeof(rq));
	rq.family_name = monospace;
	rq.point_size = 12;
	rq.weight = 400;
	font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(gw),&rq);
	GGadgetSetFont(gcd[0].ret,font);
	sld.list = gcd[0].ret;

    GDrawSetVisible(gw,true);
    while ( !sld.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);

    if ( sld.sel!=-2 ) {
	GTextInfo **ti = SFLangArray(sf,true);
	if ( sld.sel!=-1 ) {
	    for ( i=0; ti[i]->text!=NULL; ++i )
		ti[i]->selected = i==sld.sel;
	}
	GGadgetSetList(list,ti,false);
	if ( sld.sel>=0 )
	    GGadgetSetTitle(list,ti[sld.sel]->text);
	else
	    GGadgetSetTitle(list,ti[sli]->text);
    } else {
	int32 len;
	GTextInfo **old = GGadgetGetList(list,&len);
	GGadgetSetTitle(list,old[sli]->text);
    }
return( true );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

unichar_t *ClassName(const char *name,uint32 feature_tag,
	uint16 flags, int script_lang_index,int merge_with,int act_type,
	int macfeature,SplineFont *sf) {
    char *newname, *upt, *mark_class_name=NULL;
    unichar_t *u2;

    if ( (flags&0xff00)!=0 && ((flags&0xff00)>>8) < sf->mark_class_cnt )
	mark_class_name = sf->mark_class_names[((flags&0xff00)>>8)];

    newname = galloc((strlen(name)+30+(mark_class_name==NULL?0:strlen(mark_class_name)+2)) );
    if ( macfeature ) {
	sprintf(newname,"<%d,%d> ", (int) (feature_tag>>16), (int) (feature_tag&0xffff));
	upt = newname+strlen(newname);
    } else {
	if ( (newname[0] = feature_tag>>24)==0 ) newname[0] = ' ';
	if ( (newname[1] = (feature_tag>>16)&0xff)==0 ) newname[1] = ' ';
	if ( (newname[2] = (feature_tag>>8)&0xff)==0 ) newname[2] = ' ';
	if ( (newname[3] = feature_tag&0xff)==0 ) newname[3] = ' ';
	newname[4] = ' ';
	upt = newname+5;
    }

    *upt++ = flags&pst_r2l?'r':' ';
    *upt++ = flags&pst_ignorebaseglyphs?'b':' ';
    *upt++ = flags&pst_ignoreligatures?'l':' ';
    *upt++ = flags&pst_ignorecombiningmarks?'m':' ';

    if ( mark_class_name ) {
	*upt++ = '(';
	strcpy(upt,mark_class_name); upt += strlen(upt);
	*upt++ = ')';
    }
    *upt++ = ' ';

    sprintf( upt,"%3d ", script_lang_index );
    upt += strlen(upt);

    if ( act_type!=-1 ) {
	sprintf( upt,"%d ", act_type );
	upt += strlen(upt);
    }

    if ( merge_with!=-1 ) {
	sprintf( upt,"%3d ", merge_with );
	upt += strlen(upt);
    }

    strcpy(upt,name);
    u2 = utf82u_copy(newname);
    free(newname);
return( u2 );
}

unichar_t *DecomposeClassName(const unichar_t *clsnm, unichar_t **name,
	uint32 *feature_tag, int *macfeature,
	uint16 *flags, uint16 *script_lang_index,int *merge_with,int *act_type,
	SplineFont *sf) {
    int sli, type, mw, wasmac;
    uint32 tag;
    unichar_t *end;

    if ( clsnm[0]=='\0' ) {
	tag = CHR(' ',' ',' ',' ');
	wasmac = false;
    } else if ( clsnm[0]!='<' ) {
	tag = (clsnm[0]<<24) | (clsnm[1]<<16) | (clsnm[2]<<8) | clsnm[3];
	clsnm += 5;
	wasmac = false;
    } else {
	int temp = u_strtol(clsnm+1,&end,10);
	tag = (temp<<16)|u_strtol(end+1,&end,10);
	for ( clsnm=end; isspace(*clsnm); ++clsnm );
	if ( *clsnm=='>' ) ++clsnm;
	if ( *clsnm==' ' ) ++clsnm;
	wasmac = true;
    }
    if ( feature_tag!=NULL )
	*feature_tag = tag;
    if ( macfeature!=NULL )
	*macfeature = wasmac;
    if (( clsnm[0]=='r' || clsnm[0]==' ' ) &&
	    ( clsnm[1]=='b' || clsnm[1]==' ' ) &&
	    ( clsnm[2]=='l' || clsnm[2]==' ' ) &&
	    ( clsnm[3]=='m' || clsnm[3]==' ' ) &&
	    ( clsnm[4]==' ' || clsnm[4]=='(' )) {
	if ( flags!=NULL ) {
	    *flags = 0;
	    if ( clsnm[0]=='r' ) *flags |= pst_r2l;
	    if ( clsnm[1]=='b' ) *flags |= pst_ignorebaseglyphs;
	    if ( clsnm[2]=='l' ) *flags |= pst_ignoreligatures;
	    if ( clsnm[3]=='m' ) *flags |= pst_ignorecombiningmarks;
	}
	clsnm += 4;
	if ( *clsnm=='(' ) {
	    const unichar_t *end;
	    char *utf8;
	    int i;
	    for ( end = ++clsnm; *end && *end!=')'; ++end );
	    utf8 = u2utf8_copyn(clsnm,end-clsnm);
	    for ( i=sf->mark_class_cnt-1; i>0; --i )
		if ( strcmp(sf->mark_class_names[i],utf8)== 0 )
	    break;
	    free(utf8);
	    if ( flags!=NULL ) *flags |= (i<<8);
	    clsnm = end;
	}
	++clsnm;
    }
    sli = u_strtol(clsnm,&end,10);
    type = u_strtol(end,&end,10);
    mw = u_strtol(end,&end,10);
    if ( script_lang_index!=NULL ) *script_lang_index = sli;
    if ( act_type!=NULL ) *act_type = type;
    if ( merge_with!=NULL ) *merge_with = mw;
    while ( *end==' ' ) ++end;
    if ( name!=NULL )
	*name = u_copy(end);
return( end );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
GTextInfo *AddMacFeatures(GTextInfo *opentype,enum possub_type type,SplineFont *sf) {
    MacFeat *from_p, *from_f;
    struct macsetting *s;
    int i, feat, set, none, cnt;
    GTextInfo *res = NULL;

    if ( type!=pst_substitution && type!=pst_ligature && type != pst_max )
return( opentype );

    if ( sf->cidmaster ) sf = sf->cidmaster;
    else if ( sf->mm!=NULL ) sf=sf->mm->normal;

    cnt = 0;		/* Yes, I want it outside, look at the end of the loop */
    for ( i=0; i<2; ++i ) {
	for ( feat=1; ; ++feat ) {	/* I'm excluding "all typographic features", it isn't meaningful here */
	    none=true;
	    for ( from_f = sf->features; from_f!=NULL && from_f->feature!=feat; from_f=from_f->next )
		if ( from_f->feature>feat )
		    none = false;
	    for ( from_p = default_mac_feature_map; from_p!=NULL && from_p->feature!=feat; from_p=from_p->next )
		if ( from_p->feature>feat )
		    none = false;
	    if ( from_f==NULL && from_p==NULL && none )
	break;
	    for ( set=0; ; ++set ) {
		none = true;
		s = NULL;
		if ( from_f!=NULL ) {
		    for ( s = from_f->settings; s!=NULL && s->setting!=set; s=s->next )
			if ( s->setting>set )
			    none = false;
		}
		if ( s!=NULL ) none=false;	/* It might lack a name */
		if ( (s==NULL || s->setname==NULL ) && from_p!=NULL ) {
		    for ( s = from_p->settings; s!=NULL && s->setting!=set; s=s->next )
			if ( s->setting>set )
			    none = false;
		}
		if ( s==NULL && none )
	    break;
		if ( s!=NULL && s->setname!=NULL ) {
		    if ( res!=NULL ) {
			res[cnt].text = (unichar_t *) PickNameFromMacName(s->setname);
			res[cnt].text_is_1byte = true;
			res[cnt].image_precedes = true;	/* flag to say it's a mac thing */
			res[cnt].userdata = (void *) (intpt) ((feat<<16)|set);
		    }
		    ++cnt;
		}
	    }
	}
	if ( res==NULL ) {
	    int c;
	    if ( cnt==0 )
return( opentype );
	    if ( opentype!=NULL ) {
		for ( c=0; opentype[c].text!=NULL; ++c );
		res = gcalloc(c+3+cnt,sizeof(GTextInfo));
		memcpy(res,opentype,c*sizeof(GTextInfo));
		for ( c=0; opentype[c].text!=NULL; ++c )
		    if ( res[c].text_is_1byte )
			res[c].text = (unichar_t *) copy((char *) res[c].text);
		res[c].line = true;
		cnt = c+1;
	    } else {
		res = gcalloc(3+cnt,sizeof(GTextInfo));
		cnt = 0;
	    }
	}
    }
return( res );
}

#define CID_ACD_Tag	1001
#define CID_ACD_Sli	1002
#define CID_ACD_R2L	1003
#define CID_ACD_IgnBase	1004
#define CID_ACD_IgnLig	1005
#define CID_ACD_IgnMark	1006
#define CID_ACD_ProcessMark	1007
#define CID_ACD_Merge	1008

struct ac_dlg {
    int done;
    int ok;
    int sli;
    enum possub_type type;
    GTextInfo *tags, *mactags;
    GGadget *taglist;
    SplineFont *sf;
    GWindow gw;
    unichar_t *skipname;
    int was_normalsli;
};

static GTextInfo **ACD_FigureMerge(SplineFont *sf,uint32 tag, int flags,
	int sli, unichar_t *skipname, int select, int *spos) {
    int i, cnt, j;
    GTextInfo **ti = NULL;
    AnchorClass *ac;
    char *u8skipname = u2utf8_copy(skipname);

    for ( j=0; j<2; ++j ) {
	cnt = 0;
	for ( ac = sf->anchor; ac!=NULL ; ac=ac->next ) {
	    if ( strcmp(u8skipname,ac->name)!=0 ) {
		if ( ac->feature_tag==tag && ac->flags==flags &&
			ac->script_lang_index==sli ) {
		    if ( ti!=NULL ) {
			ti[cnt] = gcalloc(1,sizeof(GTextInfo));
			ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
			ti[cnt]->text = utf82u_copy(ac->name);
			ti[cnt]->userdata = (void *) (intpt) (ac->merge_with);
			if ( ac->merge_with == select ) {
			    ti[cnt]->selected = true;
			    *spos = cnt;
			    select = -2;
			}
		    }
		    ++cnt;
		}
	    }
	}
	if ( j==0 ) {
	    for ( i=1; i<0xfffff; ++i ) {
		for ( ac=sf->anchor; ac!=NULL; ac=ac->next )
		    if ( strcmp(ac->name,u8skipname)!=0 &&
			    ac->merge_with==i )
		break;
		if ( ac==NULL )
	    break;
	    }
	    ti = galloc((cnt+2)*sizeof(GTextInfo *));
	    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
	    ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
	    ti[cnt]->text = utf82u_copy(_("Itself"));
	    ti[cnt]->userdata = (void *) (intpt) (i);
	    ti[cnt+1] = gcalloc(1,sizeof(GTextInfo));
	} else if ( select!=-2 ) {
	    ti[cnt]->selected = true;
	    *spos = cnt;
	}
    }
    free(u8skipname);
return( ti );
}

static void ACD_RefigureMerge(struct ac_dlg *acd,int old) {
    uint32 tag, flags, sli; int spos;
    const unichar_t *utag;
    GGadget *merge = GWidgetGetControl(acd->gw,CID_ACD_Merge);
    unichar_t ubuf[8];
    GTextInfo **list;

    if ( merge==NULL )
return;

    if ( old==-1 )
	old = (intpt) (GGadgetGetListItemSelected(merge)->userdata);
    utag = _GGadgetGetTitle(GWidgetGetControl(acd->gw,CID_ACD_Tag));
    if ( (ubuf[0] = utag[0])==0 ) {
	ubuf[0] = ubuf[1] = ubuf[2] = ubuf[3] = ' ';
    } else {
	if ( (ubuf[1] = utag[1])==0 )
	    ubuf[1] = ubuf[2] = ubuf[3] = ' ';
	else if ( (ubuf[2] = utag[2])==0 )
	    ubuf[2] = ubuf[3] = ' ';
	else if ( (ubuf[3] = utag[3])==0 )
	    ubuf[3] = ' ';
    }
    tag = (ubuf[0]<<24) | (ubuf[1]<<16) | (ubuf[2]<<8) | ubuf[3];

    sli = GGadgetGetFirstListSelectedItem(GWidgetGetControl(acd->gw,CID_ACD_Sli));

    flags = 0;
    if ( GGadgetIsChecked(GWidgetGetControl(acd->gw,CID_ACD_R2L)) ) flags |= pst_r2l;
    if ( GGadgetIsChecked(GWidgetGetControl(acd->gw,CID_ACD_IgnBase)) ) flags |= pst_ignorebaseglyphs;
    if ( GGadgetIsChecked(GWidgetGetControl(acd->gw,CID_ACD_IgnLig)) ) flags |= pst_ignoreligatures;
    if ( GGadgetIsChecked(GWidgetGetControl(acd->gw,CID_ACD_IgnMark)) ) flags |= pst_ignorecombiningmarks;

    list = ACD_FigureMerge(acd->sf, tag, flags, sli, acd->skipname, old, &spos);
    GGadgetSetList(merge,list,false);
    GGadgetSetTitle(merge,list[spos]->text);
}

static void ACD_ToggleNest(struct ac_dlg *acd) {
    static unichar_t nullstr[] = { 0 };

    acd->was_normalsli = !acd->was_normalsli;
    if ( acd->was_normalsli ) {
	GGadgetSetList(acd->taglist,GTextInfoArrayFromList(acd->mactags,NULL),false);
    } else {
	GGadgetSetList(acd->taglist,SFGenTagListFromType(&acd->sf->gentags,acd->type),false);
    }
    GGadgetSetTitle(acd->taglist,nullstr);
}

static void ACD_SelectTag(struct ac_dlg *acd) {
    const unichar_t *utag;
    unichar_t ubuf[8], *end;
    uint32 tag;
    int macfeature;
    int i,j;
    int32 len;
    GTextInfo **ti;
    GGadget *list = GWidgetGetControl(acd->gw,CID_ACD_Tag);

    utag = _GGadgetGetTitle(list);
    if ( (ubuf[0] = utag[0])==0 )
return;
    else if (( utag[0]=='<' && utag[u_strlen(utag)-1]=='>' ) ||
	    ((u_strtol(utag,&end,10),*end==',') &&
	     (u_strtol(end+1,&end,10),*end=='\0')) ) {
	macfeature = true;
	if ( utag[0]=='<' ) ++utag;
	tag = u_strtol(utag,&end,10)<<16;
	tag |= u_strtol(end+1,&end,10);
    } else {
	macfeature = false;
	if ( utag[0]=='\'' && utag[5]=='\'' ) {
	    memcpy(ubuf,utag+1,4*sizeof(unichar_t));
	} else {
	    if ( (ubuf[1] = utag[1])==0 )
		ubuf[1] = ubuf[2] = ubuf[3] = ' ';
	    else if ( (ubuf[2] = utag[2])==0 )
		ubuf[2] = ubuf[3] = ' ';
	    else if ( (ubuf[3] = utag[3])==0 )
		ubuf[3] = ' ';
	    if ( u_strlen(utag)>4 || ubuf[0]>=0x7f || ubuf[1]>=0x7f || ubuf[2]>=0x7f || ubuf[3]>=0x7f )
return;
	}
	tag = (ubuf[0]<<24) | (ubuf[1]<<16) | (ubuf[2]<<8) | ubuf[3];
    }

    ti = GGadgetGetList(list,&len);
    for ( i=0; i<len; ++i ) {
	if ( ti[i]->userdata == (void *) (intpt) tag ) {
	    if ( ti[i]->selected )
return;
	    for ( j=0; j<len; ++j ) ti[j]->selected = false;
	    ti[i]->selected = true;
	}
    }
}

static void TagPopupMessage(GGadget *g,SplineFont *sf) {
    const unichar_t *ret = _GGadgetGetTitle(g);
    uint32 tag;
    int i,k;

    if ( ret[0]=='<' ) {
	unichar_t *end;
	int feat = u_strtol(ret+1,&end,10);
	int setting = u_strtol(end+1,&end,10);
	for ( ret=end; isspace(*ret); ++ret );
	if ( *ret=='>' ) {
	    char *fs = PickNameFromMacName(
		    FindMacSettingName(sf, feat, setting));
	    if ( fs!=NULL ) {
		unichar_t *temp = utf82u_copy(fs);
		GGadgetSetPopupMsg(g,temp);
		free(fs); free(temp);
	    }
	}
    } else if ( u_strlen(ret)==4 ) {
	tag = ((ret[0]&0xff)<<24) | ((ret[1]&0xff)<<16) | ((ret[2]&0xff)<<8) | (ret[3]&0xff);
	for ( k=0; pst_tags[k]!=NULL; ++k ) {
	    for ( i=0; pst_tags[k][i].text!=NULL; ++i ) {
		if ( pst_tags[k][i].userdata == (void *) (intpt) tag ) {
		    unichar_t *temp = utf82u_copy((char *) pst_tags[k][i].text);
		    GGadgetSetPopupMsg(g,temp);
		    free(temp);
return;
		}
	    }
	}
    }
}

static int acd_e_h(GWindow gw, GEvent *event) {
    struct ac_dlg *acd = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	acd->done = true;
	acd->ok = false;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html#Feature-Tag");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	acd->done = true;
	acd->ok = GGadgetGetCid(event->u.control.g);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_textchanged &&
	    event->u.control.g == acd->taglist && acd->was_normalsli) {
	if ( event->u.control.u.tf_changed.from_pulldown!=-1 ) {
	    uint32 tag = (uint32) (intpt) acd->mactags[event->u.control.u.tf_changed.from_pulldown].userdata;
	    int macfeat = acd->mactags[event->u.control.u.tf_changed.from_pulldown].image_precedes;
	    unichar_t ubuf[20];
	    char buf[20];
	    /* If they select something from the pulldown, don't show the human */
	    /*  readable form, instead show the 4 character tag */
	    if ( !macfeat ) {
		ubuf[0] = tag>>24;
		ubuf[1] = (tag>>16)&0xff;
		ubuf[2] = (tag>>8)&0xff;
		ubuf[3] = tag&0xff;
		ubuf[4] = 0;
	    } else {
		sprintf( buf,"<%d,%d>", (int) (tag>>16), (int) (tag&0xffff) );
		uc_strcpy(ubuf,buf);
	    }
	    GGadgetSetTitle(event->u.control.g,ubuf);
	} else
	    ACD_SelectTag(acd);
	ACD_RefigureMerge(acd,-1);
	TagPopupMessage(event->u.control.g,acd->sf);
    } else if ( event->type==et_controlevent &&
	    event->u.control.subtype == et_listselected &&
	    GGadgetGetCid(event->u.control.g)==CID_ACD_Sli ) {
	if ( ScriptLangList(acd->sf,event->u.control.g,acd->sli)!=acd->was_normalsli )
	    ACD_ToggleNest(acd);
	ACD_RefigureMerge(acd,-1);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_radiochanged ) {
	ACD_RefigureMerge(acd,-1);
    }
return( true );
}

static int GetSLI(GGadget *g) {
    int32 len, sel = GGadgetGetFirstListSelectedItem(g);
    GTextInfo **ti = GGadgetGetList(g,&len);
    if ( ti[sel]->userdata == (void *) SLI_NESTED )
return( SLI_NESTED );
    if ( ti[sel]->userdata == (void *) SLI_UNKNOWN )
return( SLI_UNKNOWN );

return( sel );
}

unichar_t *AskNameTag(char *title,unichar_t *def,uint32 def_tag, uint16 flags,
	int script_lang_index, enum possub_type type, SplineFont *sf,
	SplineChar *default_script, int merge_with, int act_type ) {
    static unichar_t nullstr[] = { 0 };
    struct ac_dlg acd;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[20], *varray[35], *harray[7], *harray2[10], boxes[4];
    GTextInfo label[20];
    GWindow gw;
    char buf[16];
    unichar_t ubuf[16];
    unichar_t *ret;
    char *name;
    const unichar_t *utag;
    unichar_t *end, *components=NULL;
    uint32 tag;
    int i, j, k, macfeature = false, class;
    GTextInfo *tags = pst_tags[type-1], *mactags = tags;

    CharInfoInit();

    if ( def==NULL ) def=nullstr;
    if ( def_tag==0 && *def!='\0' ) {
	uint16 sli;
	DecomposeClassName(def,&components,&def_tag,&macfeature,
		&flags, &sli,
		merge_with>=0 ? &merge_with : NULL,
		merge_with>=0 ? &act_type: NULL,
		sf);
	script_lang_index = sli;
    } else
	components = u_copy(def);

	memset(&acd,0,sizeof(acd));
	acd.tags = tags;
	acd.sf = sf;
	acd.sli = script_lang_index;
	acd.type = type;
	acd.skipname = def;
	acd.was_normalsli = true;
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = title;
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,160));
	pos.height = GDrawPointsToPixels(NULL,merge_with==-1?265:merge_with==-2?240:320);
	acd.gw = gw = GDrawCreateTopWindow(NULL,&pos,acd_e_h,&acd,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));
	memset(&label,0,sizeof(label));

	label[0].text = (unichar_t *) (strcmp(title,_("Edit anchor class"))==0||strcmp(title,_("New anchor class"))==0?
			_("Anchor Class Name:"):
		    strcmp(title,_("Suffix to Tag..."))==0?
			_("Suffix:"): _("Components"));
	label[0].text_is_1byte = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5;
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;
	varray[0] = &gcd[0]; varray[1] = NULL;

	label[1].text = components;
	gcd[1].gd.label = &label[1];
	gcd[1].gd.pos.x = 10; gcd[1].gd.pos.y = 5+13;
	gcd[1].gd.pos.width = 140;
	gcd[1].gd.flags = gg_enabled|gg_visible;
	gcd[1].creator = GTextFieldCreate;
	varray[2] = &gcd[1]; varray[3] = NULL;

	if ( merge_with==-2 ) {
	    gcd[0].gd.pos.y -= 39;
	    gcd[1].gd.pos.y -= 39;
	    gcd[0].gd.flags = 0;
	    gcd[1].gd.flags = 0;
	    merge_with = -1;
	}

	label[2].text = (unichar_t *) _("_Tag:");
	label[2].text_is_1byte = true;
	label[2].text_in_resource = true;
	gcd[2].gd.label = &label[2];
	gcd[2].gd.pos.x = 5; gcd[2].gd.pos.y = gcd[1].gd.pos.y+26; 
	gcd[2].gd.flags = gg_enabled|gg_visible;
	gcd[2].creator = GLabelCreate;
	varray[4] = &gcd[2]; varray[5] = NULL;

	if ( type==pst_substitution || type==pst_ligature )
	    mactags = AddMacFeatures(tags,type,sf);
	acd.mactags = mactags;
	if ( macfeature ) {
	    sprintf(buf,"<%d,%d>", (int) (def_tag>>16), (int) (def_tag&0xffff) );
	    uc_strcpy(ubuf,buf);
	} else {
	    ubuf[0] = def_tag>>24; ubuf[1] = (def_tag>>16)&0xff; ubuf[2] = (def_tag>>8)&0xff; ubuf[3] = def_tag&0xff; ubuf[4] = 0;
	}
	label[3].text = ubuf;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.pos.x = 10; gcd[3].gd.pos.y = gcd[2].gd.pos.y+14;
	gcd[3].gd.flags = gg_enabled|gg_visible;
	gcd[3].gd.u.list = mactags;
	gcd[3].gd.cid = CID_ACD_Tag;
	gcd[3].creator = GListFieldCreate;
	varray[6] = &gcd[3]; varray[7] = NULL;

	label[4].text = (unichar_t *) _("_Script & Languages:");
	label[4].text_is_1byte = true;
	label[4].text_in_resource = true;
	gcd[4].gd.label = &label[4];
	gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[3].gd.pos.y+26; 
	gcd[4].gd.flags = gg_enabled|gg_visible;
	gcd[4].creator = GLabelCreate;
	varray[8] = &gcd[4]; varray[9] = NULL;

	gcd[5].gd.pos.x = 10; gcd[5].gd.pos.y = gcd[4].gd.pos.y+14;
	gcd[5].gd.pos.width = 140;
	gcd[5].gd.flags = gg_enabled|gg_visible;
	gcd[5].gd.u.list = SFLangList(sf,strcmp(title,_("Suffix to Tag..."))==0?7:3,default_script);
	j = script_lang_index;
	if ( script_lang_index!=-1 && script_lang_index!=65535 ) {
	    for ( i=0; gcd[5].gd.u.list[i].text!=NULL; ++i )
		gcd[5].gd.u.list[i].selected = false;
	    if ( script_lang_index==SLI_NESTED ) {
		for ( j=0 ; gcd[5].gd.u.list[j].userdata!=(void *) SLI_NESTED; ++j );
		gcd[5].gd.u.list[j].selected = true;
	    } else
		gcd[5].gd.u.list[script_lang_index].selected = true;
	} else {
	    for ( script_lang_index=0; !gcd[5].gd.u.list[script_lang_index].selected &&
		    gcd[5].gd.u.list[script_lang_index].text!=NULL; ++script_lang_index );
	    if ( gcd[5].gd.u.list[script_lang_index].text!=NULL ) {
		acd.sli = script_lang_index;
		if ( flags==0 && SRIsRightToLeft(sf->script_lang[acd.sli]))
		    flags = pst_r2l;
	    }
	    j = script_lang_index;
	}
	if ( j<0 || j>65500 ) j=0;
	gcd[5].gd.label = &gcd[5].gd.u.list[j];
	gcd[5].gd.cid = CID_ACD_Sli;
	gcd[5].creator = GListButtonCreate;
	varray[10] = &gcd[5]; varray[11] = NULL;

	gcd[6].gd.pos.x = 5; gcd[6].gd.pos.y = gcd[5].gd.pos.y+28;
	gcd[6].gd.flags = gg_visible | gg_enabled | (flags&pst_r2l?gg_cb_on:0);
	label[6].text = (unichar_t *) _("Right To Left");
	label[6].text_is_1byte = true;
	gcd[6].gd.label = &label[6];
	gcd[6].gd.cid = CID_ACD_R2L;
	gcd[6].creator = GCheckBoxCreate;
	varray[12] = &gcd[6]; varray[13] = NULL;

	gcd[7].gd.pos.x = 5; gcd[7].gd.pos.y = gcd[6].gd.pos.y+15;
	gcd[7].gd.flags = gg_visible | gg_enabled | (flags&pst_ignorebaseglyphs?gg_cb_on:0);
	label[7].text = (unichar_t *) _("Ignore Base Glyphs");
	label[7].text_is_1byte = true;
	gcd[7].gd.label = &label[7];
	gcd[7].gd.cid = CID_ACD_IgnBase;
	gcd[7].creator = GCheckBoxCreate;
	varray[14] = &gcd[7]; varray[15] = NULL;

	gcd[8].gd.pos.x = 5; gcd[8].gd.pos.y = gcd[7].gd.pos.y+15;
	gcd[8].gd.flags = gg_visible | gg_enabled | (flags&pst_ignoreligatures?gg_cb_on:0);
	label[8].text = (unichar_t *) _("Ignore Ligatures");
	label[8].text_is_1byte = true;
	gcd[8].gd.label = &label[8];
	gcd[8].gd.cid = CID_ACD_IgnLig;
	gcd[8].creator = GCheckBoxCreate;
	varray[16] = &gcd[8]; varray[17] = NULL;

	gcd[9].gd.pos.x = 5; gcd[9].gd.pos.y = gcd[8].gd.pos.y+15;
	gcd[9].gd.flags = gg_visible | gg_enabled | (flags&pst_ignorecombiningmarks?gg_cb_on:0);
	label[9].text = (unichar_t *) _("Ignore Combining Marks");
	label[9].text_is_1byte = true;
	gcd[9].gd.label = &label[9];
	gcd[9].gd.cid = CID_ACD_IgnMark;
	gcd[9].creator = GCheckBoxCreate;
	varray[18] = &gcd[9]; varray[19] = NULL;

/* GT: Process is a verb here. Marks of the given mark class are to be processed */
	label[10].text = (unichar_t *) _("Process Marks:");
	label[10].text_is_1byte = true;
	gcd[10].gd.label = &label[10];
	gcd[10].gd.pos.x = 5; gcd[10].gd.pos.y = gcd[9].gd.pos.y+16; 
	gcd[10].gd.flags = sf->mark_class_cnt<=1 ? gg_visible : (gg_enabled|gg_visible);
	gcd[10].creator = GLabelCreate;
	varray[20] = &gcd[10]; varray[21] = NULL;

	gcd[11].gd.pos.x = 10; gcd[11].gd.pos.y = gcd[10].gd.pos.y+14;
	gcd[11].gd.pos.width = 140;
	gcd[11].gd.flags = gcd[10].gd.flags;
	if ( (class = ((flags&0xff00)>>8) )>= sf->mark_class_cnt )
	    class = 0;
	gcd[11].gd.u.list = SFMarkClassList(sf,class);
	gcd[11].gd.label = &gcd[11].gd.u.list[class];
	gcd[11].gd.cid = CID_ACD_ProcessMark;
	gcd[11].creator = GListButtonCreate;
	varray[22] = &gcd[11]; varray[23] = NULL;

	i = 12; k = 24;
	if ( merge_with!=-1 ) {
	    label[i].text = (unichar_t *) _("Default");
	    label[i].text_is_1byte = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[11].gd.pos.y+28;
	    gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    gcd[i].gd.popup_msg = (unichar_t *) _("Mark To Base, or\nMark To Ligature");
	    harray2[0] = GCD_Glue; harray2[1] = &gcd[i];
	    gcd[i++].creator = GRadioCreate;

	    label[i].text = (unichar_t *) "Mk-Mk";
	    label[i].text_is_1byte = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 55; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
	    gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    gcd[i].gd.popup_msg = (unichar_t *) _("Mark To Mark");
	    harray2[2] = GCD_Glue; harray2[3] = &gcd[i];
	    gcd[i++].creator = GRadioCreate;

	    label[i].text = (unichar_t *) _("Cursive");
	    label[i].text_is_1byte = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 103; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y;
	    gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    gcd[i].gd.popup_msg = (unichar_t *) _("Cursive Attachment");
	    harray2[4] = GCD_Glue; harray2[5] = &gcd[i]; harray2[6] = GCD_Glue; harray2[7] = NULL;
	    gcd[i++].creator = GRadioCreate;

	    boxes[2].gd.flags = gg_enabled|gg_visible;
	    boxes[2].gd.u.boxelements = harray2;
	    boxes[2].creator = GHBoxCreate;
	    varray[k++] = &boxes[2]; varray[k++] = NULL;

	    if ( act_type==act_mark ) gcd[12].gd.flags |= gg_cb_on;
	    else if ( act_type==act_mkmk ) gcd[13].gd.flags |= gg_cb_on;
	    else gcd[14].gd.flags |= gg_cb_on;

	    label[i].text = (unichar_t *) _("Merge With:");
	    label[i].text_is_1byte = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+17;
	    gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    gcd[i].gd.popup_msg = (unichar_t *) _("FontForge will attempt to merge all anchor classes with the same value for \"Merge With\" into one GPOS lookup");
	    varray[k++] = &gcd[i]; varray[k++] = NULL;
	    gcd[i++].creator = GLabelCreate;

	    gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+13;
	    gcd[i].gd.pos.width = 140;
	    gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	    gcd[i].gd.popup_msg = (unichar_t *) _("FontForge will attempt to merge all anchor classes with the same value for \"Merge With\" into one GPOS lookup");
	    gcd[i].gd.cid = CID_ACD_Merge;
	    varray[k++] = &gcd[i]; varray[k++] = NULL;
	    gcd[i++].creator = GListButtonCreate;

	    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26;
	} else 
	    gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+30;

	varray[k++] = GCD_Glue; varray[k++] = NULL;

	gcd[i].gd.pos.x = 15-3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = true;
	harray[0] = GCD_Glue; harray[1] = &gcd[i]; harray[2] = GCD_Glue;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = false;
	harray[3] = GCD_Glue; harray[4] = &gcd[i]; harray[5] = GCD_Glue; harray[6] = NULL;
	gcd[i++].creator = GButtonCreate;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = harray;
	boxes[3].creator = GHBoxCreate;
	varray[k++] = &boxes[3]; varray[k++] = NULL; varray[k++] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
	if ( boxes[2].ret!=NULL )
	    GHVBoxSetExpandableCol(boxes[2].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);
	GTextInfoListFree(gcd[5].gd.u.list);
	GTextInfoListFree(gcd[11].gd.u.list);
	free(components);
	acd.taglist = gcd[3].ret;
	if ( merge_with!=-1 )
	    ACD_RefigureMerge(&acd,merge_with);
	if ( acd.sli==SLI_NESTED )
	    ACD_ToggleNest(&acd);
	ACD_SelectTag(&acd);
	TagPopupMessage(acd.taglist,acd.sf);

    GDrawSetVisible(gw,true);
    GWidgetIndicateFocusGadget(gcd[1].ret);
 tryagain:
    acd.done = false;
    while ( !acd.done )
	GDrawProcessOneEvent(NULL);
    if ( acd.ok ) {
	name = GGadgetGetTitle8(gcd[1].ret);
	utag = _GGadgetGetTitle(gcd[3].ret);
	script_lang_index = GetSLI(gcd[5].ret);
	if ( (ubuf[0] = utag[0])==0 ) {
	    gwwv_post_error(_("Missing Feature Tag"),_("Missing Feature Tag"));
 goto tryagain;
	} else if ((( utag[0]=='<' && utag[u_strlen(utag)-1]=='>' ) ||
		((u_strtol(utag,&end,10),*end==',') &&
		 (u_strtol(end+1,&end,10),*end=='\0')) ) &&
		(type==pst_substitution || pst_ligature)) {
	    macfeature = true;
	    if ( utag[0]=='<' ) ++utag;
	    tag = u_strtol(utag,&end,10)<<16;
	    tag |= u_strtol(end+1,&end,10);
	} else {
	    macfeature = false;
	    if ( utag[0]=='\'' && utag[5]=='\'' ) {
		memcpy(ubuf,utag+1,4*sizeof(unichar_t));
	    } else {
		if ( (ubuf[1] = utag[1])==0 )
		    ubuf[1] = ubuf[2] = ubuf[3] = ' ';
		else if ( (ubuf[2] = utag[2])==0 )
		    ubuf[2] = ubuf[3] = ' ';
		else if ( (ubuf[3] = utag[3])==0 )
		    ubuf[3] = ' ';
		if ( u_strlen(utag)>4 || ubuf[0]>=0x7f || ubuf[1]>=0x7f || ubuf[2]>=0x7f || ubuf[3]>=0x7f ) {
		    gwwv_post_error(_("Tag too long"),_("Feature tags must be exactly 4 ASCII characters"));
 goto tryagain;
		}
	    }
	    tag = (ubuf[0]<<24) | (ubuf[1]<<16) | (ubuf[2]<<8) | ubuf[3];
	}
	if ( script_lang_index==SLI_NESTED ) {
	    enum possub_type pstype = SFGTagUsed(&sf->gentags,tag);
	    if ( pstype == pst_null ) {
		SFGenerateNewFeatureTag(&sf->gentags,type,tag);
	    } else {
		if ( pstype==type && type<=pst_ligature )
		    /* That's ok */;
		else {
		    gwwv_post_error(_("Tag Reuse"),_("You may not reuse a tag that has already been used for a nested feature"));
 goto tryagain;
		}
	    }
	}
	flags = 0;
	if ( GGadgetIsChecked(gcd[6].ret) ) flags |= pst_r2l;
	if ( GGadgetIsChecked(gcd[7].ret) ) flags |= pst_ignorebaseglyphs;
	if ( GGadgetIsChecked(gcd[8].ret) ) flags |= pst_ignoreligatures;
	if ( GGadgetIsChecked(gcd[9].ret) ) flags |= pst_ignorecombiningmarks;
	flags |= (GGadgetGetFirstListSelectedItem(gcd[11].ret))<<8;
	if ( merge_with!=-1 ) {
	    merge_with = (intpt) (GGadgetGetListItemSelected(gcd[16].ret)->userdata);
	    act_type = GGadgetIsChecked(gcd[12].ret) ? act_mark :
			GGadgetIsChecked(gcd[13].ret) ? act_mkmk : act_curs;
	}
	ret = ClassName(name,tag,flags,script_lang_index,merge_with,act_type,
		macfeature,sf);
    } else
	ret = NULL;
    GDrawDestroyWindow(gw);
    if ( mactags!=tags )
	GTextInfoListFree(mactags);
return( ret );
}

struct pt_dlg {
    int done;
    int ok;
    int sli;
    enum possub_type type;
    GTextInfo *tags;
    GGadget *taglist;
    SplineFont *sf;
    GWindow gw;
    int ispair;
    int was_normalsli;
};

static void PTD_ToggleNest(struct pt_dlg *ptd) {
    static unichar_t nullstr[] = { 0 };

    ptd->was_normalsli = !ptd->was_normalsli;
    if ( ptd->was_normalsli ) {
	GGadgetSetList(ptd->taglist,GTextInfoArrayFromList(ptd->tags,NULL),false);
    } else {
	GGadgetSetList(ptd->taglist,SFGenTagListFromType(&ptd->sf->gentags,ptd->type),false);
    }
    GGadgetSetTitle(ptd->taglist,nullstr);
}

static int ptd_e_h(GWindow gw, GEvent *event) {
    struct pt_dlg *ptd = GDrawGetUserData(gw);
    if ( event->type==et_close ) {
	ptd->done = true;
	ptd->ok = false;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html#Feature-Tag");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	ptd->done = true;
	ptd->ok = GGadgetGetCid(event->u.control.g);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_textchanged &&
	    event->u.control.g==ptd->taglist ) {
	if ( event->u.control.u.tf_changed.from_pulldown!=-1 && ptd->was_normalsli ) {
	    uint32 tag;
	    unichar_t ubuf[8];
	    /* If they select something from the pulldown, don't show the human */
	    /*  readable form, instead show the 4 character tag */
	    if ( ptd->ispair )
		tag = (uint32) (intpt) pairpos_tags[event->u.control.u.tf_changed.from_pulldown].userdata;
	    else
		tag = (uint32) (intpt) simplepos_tags[event->u.control.u.tf_changed.from_pulldown].userdata;
	    ubuf[0] = tag>>24;
	    ubuf[1] = (tag>>16)&0xff;
	    ubuf[2] = (tag>>8)&0xff;
	    ubuf[3] = tag&0xff;
	    ubuf[4] = 0;
	    GGadgetSetTitle(event->u.control.g,ubuf);
	}
	TagPopupMessage(event->u.control.g,ptd->sf);
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_listselected ) {
	if ( ScriptLangList(ptd->sf,event->u.control.g,ptd->sli)!=ptd->was_normalsli )
	    PTD_ToggleNest(ptd);
    }
return( true );
}

static unichar_t *AskPosTag(char *title,unichar_t *def,uint32 def_tag, uint16 flags,
	int script_lang_index, enum possub_type type,SplineFont *sf,
	SplineChar *default_script) {
    struct pt_dlg ptd;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[27], *varray[27], *harray[7], *hvarray[21], boxes[5];
    GTextInfo label[27];
    GWindow gw;
    unichar_t ubuf[8];
    unichar_t *ret, *pt, *end, *other=NULL;
    const unichar_t *utag;
    uint32 tag;
    int dx=0, dy=0, dxa=0, dya=0;
    int dx2=0, dy2=0, dxa2=0, dya2=0;
    char buf[250];
    unichar_t udx[12], udy[12], udxa[12], udya[12];
    unichar_t udx2[12], udy2[12], udxa2[12], udya2[12];
    int i, j, k, temp, tag_pos, sli_pos, class, mrk_pos;
    static unichar_t nullstr[] = { 0 };
    GTextInfo *tags = pst_tags[type-1];
#if defined(FONTFORGE_CONFIG_GDRAW)
    static char *buts[3];
    static int done = false;

    if ( !done ) {
	buts[0] = _("_Yes");
	buts[1] = _("_Cancel");
	done = true;
	CharInfoInit();
    }
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif

    if ( def==NULL ) def=nullstr;
    if ( def_tag==0 && u_strlen(def)>4 && def[4]==' ' && def[0]<0x7f && def[1]<0x7f && def[2]<0x7f && def[3]<0x7f ) {
	if ( def[0]!=' ' )
	    def_tag = (def[0]<<24) | (def[1]<<16) | (def[2]<<8) | def[3];
	def += 5;
	if (( def[0]=='r' || def[0]==' ' ) &&
		( def[1]=='b' || def[1]==' ' ) &&
		( def[2]=='l' || def[2]==' ' ) &&
		( def[3]=='m' || def[3]==' ' ) &&
		( def[4]==' ' || def[4]=='(' )) {
	    flags = 0;
	    if ( def[0]=='r' ) flags |= pst_r2l;
	    if ( def[1]=='b' ) flags |= pst_ignorebaseglyphs;
	    if ( def[2]=='l' ) flags |= pst_ignoreligatures;
	    if ( def[3]=='m' ) flags |= pst_ignorecombiningmarks;
	    def += 4;
	    if ( *def=='(' ) {
		for ( end = ++def; *end && *end!=')'; ++end );
		for ( i=sf->mark_class_cnt-1; i>0; --i )
		    if ( strlen(sf->mark_class_names[i])==end-def &&
			    uc_strncmp(def,sf->mark_class_names[i],
				    end-def)==0 )
		break;
		flags |= (i<<8);
		def = end;
	    }
	    ++def;
	}
	temp = u_strtol(def,&end,10);
	if ( end!=def ) {
	    script_lang_index = temp;
	    def = end;
	    if ( *def==' ' ) ++def;
	}
    }

    if ( type==pst_pair ) {
	for ( pt=def; *pt==' ' ; ++pt );
	other = pt;
	while ( *pt!=' ' && *pt!='\0' ) ++pt;
	other = u_copyn(other,pt-other);
    }
    for ( pt=def; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    dx = u_strtol(pt,&end,10);
    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    dy = u_strtol(pt,&end,10);
    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    dxa = u_strtol(pt,&end,10);
    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    dya = u_strtol(pt,&end,10);
    if ( type==pst_pair ) {
	for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
	dx2 = u_strtol(pt,&end,10);
	for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
	dy2 = u_strtol(pt,&end,10);
	for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
	dxa2 = u_strtol(pt,&end,10);
	for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
	dya2 = u_strtol(pt,&end,10);
    }
    sprintf(buf,"%d",dx);
    uc_strcpy(udx,buf);
    sprintf(buf,"%d",dy);
    uc_strcpy(udy,buf);
    sprintf(buf,"%d",dxa);
    uc_strcpy(udxa,buf);
    sprintf(buf,"%d",dya);
    uc_strcpy(udya,buf);
    if ( type==pst_pair ) {
	sprintf(buf,"%d",dx2);
	uc_strcpy(udx2,buf);
	sprintf(buf,"%d",dy2);
	uc_strcpy(udy2,buf);
	sprintf(buf,"%d",dxa2);
	uc_strcpy(udxa2,buf);
	sprintf(buf,"%d",dya2);
	uc_strcpy(udya2,buf);
    }
    

	memset(&ptd,0,sizeof(ptd));
	ptd.sf = sf;
	ptd.sli = script_lang_index;
	ptd.type = type;
	ptd.ispair = type==pst_pair;
	ptd.tags = tags;
	ptd.was_normalsli = true;
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = title;
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,ptd.ispair?190:160));
	pos.height = GDrawPointsToPixels(NULL,ptd.ispair?360:330);
	ptd.gw = gw = GDrawCreateTopWindow(NULL,&pos,ptd_e_h,&ptd,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&boxes,0,sizeof(boxes));
	memset(&label,0,sizeof(label));

	i=k=0;
	label[i].text = (unichar_t *) U_("∆X:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = 5+4;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	hvarray[k++] = &gcd[i];
	gcd[i++].creator = GLabelCreate;

	label[i].text = udx;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 70; gcd[i].gd.pos.y = 5; gcd[i].gd.pos.width = 50;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = i+1;
	hvarray[k++] = &gcd[i];
	gcd[i++].creator = GTextFieldCreate;

	if ( ptd.ispair ) {
	    label[i].text = udx2;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 130; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y; gcd[i].gd.pos.width = 50;
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i].gd.cid = i+1;
	    hvarray[k++] = &gcd[i];
	    gcd[i++].creator = GTextFieldCreate;
	}
	hvarray[k++] = NULL;

	label[i].text = (unichar_t *) U_("∆Y:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	hvarray[k++] = &gcd[i];
	gcd[i++].creator = GLabelCreate;

	label[i].text = udy;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26; gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = i+1;
	hvarray[k++] = &gcd[i];
	gcd[i++].creator = GTextFieldCreate;

	if ( ptd.ispair ) {
	    label[i].text = udy2;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = gcd[i-1].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y; gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i].gd.cid = i+1;
	    hvarray[k++] = &gcd[i];
	    gcd[i++].creator = GTextFieldCreate;
	}
	hvarray[k++] = NULL;

	label[i].text = (unichar_t *) U_("∆XAdvance:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	hvarray[k++] = &gcd[i];
	gcd[i++].creator = GLabelCreate;

	label[i].text = udxa;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26; gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = i+1;
	hvarray[k++] = &gcd[i];
	gcd[i++].creator = GTextFieldCreate;

	if ( ptd.ispair ) {
	    label[i].text = udxa2;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-5].gd.pos.y; gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i].gd.cid = i+1;
	    hvarray[k++] = &gcd[i];
	    gcd[i++].creator = GTextFieldCreate;
	}
	hvarray[k++] = NULL;

	label[i].text = (unichar_t *) U_("∆YAdvance:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	hvarray[k++] = &gcd[i];
	gcd[i++].creator = GLabelCreate;

	label[i].text = udya;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26; gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = i+1;
	hvarray[k++] = &gcd[i];
	gcd[i++].creator = GTextFieldCreate;

	if ( ptd.ispair ) {
	    label[i].text = udya2;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = gcd[i-3].gd.pos.x; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y; gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i].gd.cid = i+1;
	    hvarray[k++] = &gcd[i]; hvarray[k++] = NULL;
	    gcd[i++].creator = GTextFieldCreate;

	    label[i].text = (unichar_t *) _("Paired Glyph:");
	    label[i].text_is_1byte = true;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-3].gd.pos.y+26;
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    hvarray[k++] = &gcd[i];
	    gcd[i++].creator = GLabelCreate;

	    label[i].text = other;
	    gcd[i].gd.label = &label[i];
	    gcd[i].gd.pos.x = gcd[i-2].gd.pos.x; gcd[i].gd.pos.y = gcd[i-2].gd.pos.y+26; gcd[i].gd.pos.width = gcd[i-2].gd.pos.width;
	    gcd[i].gd.flags = gg_enabled|gg_visible;
	    gcd[i].gd.cid = i+1;
	    hvarray[k++] = &gcd[i]; hvarray[k++] = GCD_ColSpan; hvarray[k++] = NULL;
	    gcd[i++].creator = GTextFieldCreate;
	} else
	    hvarray[k++] = NULL;
	hvarray[k++] = NULL;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = hvarray;
	boxes[2].creator = GHVBoxCreate;
	k = 0;
	varray[k++] = &boxes[2]; varray[k++] = NULL;

	label[i].text = (unichar_t *) _("_Tag:");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	varray[k++] = &gcd[i]; varray[k++] = NULL;
	gcd[i++].creator = GLabelCreate;

	tag_pos = i;
	ubuf[0] = def_tag>>24; ubuf[1] = (def_tag>>16)&0xff; ubuf[2] = (def_tag>>8)&0xff; ubuf[3] = def_tag&0xff; ubuf[4] = 0;
	label[i].text = ubuf;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+14;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.list = tags;
	gcd[i].gd.cid = i+1;
	varray[k++] = &gcd[i]; varray[k++] = NULL;
	gcd[i++].creator = GListFieldCreate;

	label[i].text = (unichar_t *) _("_Script & Languages:");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	varray[k++] = &gcd[i]; varray[k++] = NULL;
	gcd[i++].creator = GLabelCreate;

	sli_pos = i;
	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+14;
	gcd[i].gd.pos.width = 140;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.list = SFLangList(sf,3,default_script);
	if ( script_lang_index!=-1 ) {
	    for ( j=0; gcd[i].gd.u.list[j].text!=NULL; ++j )
		gcd[i].gd.u.list[j].selected = false;
	    if ( script_lang_index==SLI_NESTED ) {
		for ( j=0 ; gcd[i].gd.u.list[j].userdata!=(void *) SLI_NESTED; ++j );
		gcd[i].gd.u.list[j].selected = true;
	    } else {
		gcd[i].gd.u.list[script_lang_index].selected = true;
		j = script_lang_index;
	    }
	} else {
	    for ( script_lang_index=0; !gcd[i].gd.u.list[script_lang_index].selected &&
		    gcd[i].gd.u.list[script_lang_index].text!=NULL; ++script_lang_index );
	    if ( gcd[i].gd.u.list[script_lang_index].text!=NULL ) {
		ptd.sli = script_lang_index;
		if ( flags==0 && SRIsRightToLeft(sf->script_lang[ptd.sli]))
		    flags = pst_r2l;
	    }
	    j = script_lang_index;
	}
	gcd[i].gd.label = &gcd[i].gd.u.list[j];
	gcd[i].gd.cid = i+1;
	varray[k++] = &gcd[i]; varray[k++] = NULL;
	gcd[i++].creator = GListButtonCreate;

	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+28;
	gcd[i].gd.flags = gg_visible | gg_enabled | (flags&pst_r2l?gg_cb_on:0);
	label[i].text = (unichar_t *) _("Right To Left");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	varray[k++] = &gcd[i]; varray[k++] = NULL;
	gcd[i++].creator = GCheckBoxCreate;

	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
	gcd[i].gd.flags = gg_visible | gg_enabled | (flags&pst_ignorebaseglyphs?gg_cb_on:0);
	label[i].text = (unichar_t *) _("Ignore Base Glyphs");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	varray[k++] = &gcd[i]; varray[k++] = NULL;
	gcd[i++].creator = GCheckBoxCreate;

	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
	gcd[i].gd.flags = gg_visible | gg_enabled | (flags&pst_ignoreligatures?gg_cb_on:0);
	label[i].text = (unichar_t *) _("Ignore Ligatures");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	varray[k++] = &gcd[i]; varray[k++] = NULL;
	gcd[i++].creator = GCheckBoxCreate;

	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15;
	gcd[i].gd.flags = gg_visible | gg_enabled | (flags&pst_ignorecombiningmarks?gg_cb_on:0);
	label[i].text = (unichar_t *) _("Ignore Combining Marks");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	varray[k++] = &gcd[i]; varray[k++] = NULL;
	gcd[i++].creator = GCheckBoxCreate;

	label[i].text = (unichar_t *) _("Process Marks:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+16;
	gcd[i].gd.flags = sf->mark_class_cnt<=1 ? gg_visible : (gg_enabled|gg_visible);
	varray[k++] = &gcd[i]; varray[k++] = NULL;
	gcd[i++].creator = GLabelCreate;

	mrk_pos = i;
	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+14;
	gcd[i].gd.pos.width = 140;
	gcd[i].gd.flags = gcd[i-1].gd.flags;
	if ( (class = ((flags&0xff00)>>8) )>= sf->mark_class_cnt )
	    class = 0;
	gcd[i].gd.u.list = SFMarkClassList(sf,class);
	gcd[i].gd.label = &gcd[i].gd.u.list[class];
	gcd[i].gd.cid = CID_ACD_ProcessMark;
	varray[k++] = &gcd[i]; varray[k++] = NULL;
	gcd[i++].creator = GListButtonCreate;


	gcd[i].gd.pos.x = 15-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+30;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.mnemonic = 'O';
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = true;
	harray[0] = GCD_Glue; harray[1] = &gcd[i]; harray[2] = GCD_Glue;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.mnemonic = 'C';
	gcd[i].gd.cid = false;
	harray[3] = GCD_Glue; harray[4] = &gcd[i]; harray[5] = GCD_Glue; harray[6] = NULL;
	gcd[i++].creator = GButtonCreate;

	boxes[3].gd.flags = gg_enabled|gg_visible;
	boxes[3].gd.u.boxelements = harray;
	boxes[3].creator = GHBoxCreate;
	varray[k++] = &boxes[3]; varray[k++] = NULL; 
	varray[k++] = GCD_Glue; varray[k++] = NULL; varray[k++] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(boxes[3].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);

	free(other);
	GTextInfoListFree(gcd[sli_pos].gd.u.list);
	GTextInfoListFree(gcd[mrk_pos].gd.u.list);
	ptd.taglist = gcd[tag_pos].ret;
	TagPopupMessage(ptd.taglist,ptd.sf);

    GDrawSetVisible(gw,true);
    GWidgetIndicateFocusGadget(gcd[1].ret);
 tryagain:
    while ( !ptd.done )
	GDrawProcessOneEvent(NULL);
    if ( ptd.ok ) {
	int err=false;
	char *other;
	dx = GetInt8(gw,2, U_("∆X:"),&err);
	dy = GetInt8(gw,4, U_("∆Y:"),&err);
	dxa = GetInt8(gw,6, U_("∆XAdvance:"),&err);
	dya = GetInt8(gw,8, U_("∆YAdvance:"),&err);
	if ( ptd.ispair ) {
	    dx2 = GetInt8(gw,9, U_("∆X:"),&err);
	    dy2 = GetInt8(gw,10, U_("∆Y:"),&err);
	    dxa2 = GetInt8(gw,11, U_("∆XAdvance:"),&err);
	    dya2 = GetInt8(gw,12, U_("∆YAdvance:"),&err);
	    other = cu_copy(_GGadgetGetTitle(GWidgetGetControl(gw,14)));
	    if ( *other=='\0' ) {
		gwwv_post_error(_("Missing paired glyph"),_("You must specify a name by which to identify the paired glyph"));
		err = true;
	    } else if ( SFGetChar(sf,-1,other)==NULL ) {
		if ( gwwv_ask(_("Missing paired glyph"),(const char **) buts,0,1,_("The paired glyph's name (%.50s) does not match any glyph in the font. Is that what you want?"),other)==1 )
		    err = true;
	    }
	}
	utag = _GGadgetGetTitle(gcd[tag_pos].ret);
	script_lang_index = GetSLI(gcd[tag_pos+2].ret);
	if ( err ) {
	    ptd.done = false;
 goto tryagain;
	}
	if ( (ubuf[0] = utag[0])==0 ) {
	    gwwv_post_error(_("Missing Feature Tag"),_("Missing Feature Tag"));
 goto tryagain;
	/* Can't get any mac features here */
	} else {
	    if ( utag[0]=='\'' && utag[5]=='\'' ) {
		memcpy(ubuf,utag+1,4*sizeof(unichar_t));
	    } else {
		if ( (ubuf[1] = utag[1])==0 )
		    ubuf[1] = ubuf[2] = ubuf[3] = ' ';
		else if ( (ubuf[2] = utag[2])==0 )
		    ubuf[2] = ubuf[3] = ' ';
		else if ( (ubuf[3] = utag[3])==0 )
		    ubuf[3] = ' ';
		if ( u_strlen(utag)>4 || ubuf[0]>=0x7f || ubuf[1]>=0x7f || ubuf[2]>=0x7f || ubuf[3]>=0x7f ) {
		    gwwv_post_error(_("Tag too long"),_("Feature tags must be exactly 4 ASCII characters"));
 goto tryagain;
		}
	    }
	    tag = (ubuf[0]<<24) | (ubuf[1]<<16) | (ubuf[2]<<8) | ubuf[3];
	}
	if ( script_lang_index==SLI_NESTED ) {
	    enum possub_type pstype = SFGTagUsed(&sf->gentags,tag);
	    if ( pstype == pst_null ) {
		SFGenerateNewFeatureTag(&sf->gentags,type,tag);
	    } else {
		if ( pstype==type && type<=pst_ligature )
		    /* That's ok */;
		else {
		    gwwv_post_error(_("Tag Reuse"),_("You may not reuse a tag that has already been used for a nested feature"));
 goto tryagain;
		}
	    }
	}
	flags = 0;
	if ( GGadgetIsChecked(gcd[i-8].ret) ) flags |= pst_r2l;
	if ( GGadgetIsChecked(gcd[i-7].ret) ) flags |= pst_ignorebaseglyphs;
	if ( GGadgetIsChecked(gcd[i-6].ret) ) flags |= pst_ignoreligatures;
	if ( GGadgetIsChecked(gcd[i-5].ret) ) flags |= pst_ignorecombiningmarks;
	flags |= (GGadgetGetFirstListSelectedItem(gcd[i-3].ret))<<8;
	if ( ptd.ispair ) {
#if defined( _NO_SNPRINTF ) || defined( __VMS )
	    sprintf(buf,"%c%c%c%c %c%c%c%c %d %.50s ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d | ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d",
#else
	    snprintf(buf,sizeof(buf), "%c%c%c%c %c%c%c%c %d %.50s ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d | ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d",
#endif
		    (int) (tag>>24), (int) (tag>>16), (int) (tag>>8), (int) tag,
		    flags&pst_r2l?'r':' ',
		    flags&pst_ignorebaseglyphs?'b':' ',
		    flags&pst_ignoreligatures?'l':' ',
		    flags&pst_ignorecombiningmarks?'m':' ',
		    script_lang_index,
		    other,
		    dx, dy, dxa, dya,
		    dx2, dy2, dxa2, dya2 );
	} else {
#if defined( _NO_SNPRINTF ) || defined( __VMS )
	    sprintf(buf,"%c%c%c%c %c%c%c%c %d ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d",
#else
	    snprintf(buf,sizeof(buf), "%c%c%c%c %c%c%c%c %d ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d",
#endif
		    (int) (tag>>24), (int) (tag>>16), (int) (tag>>8), (int) tag,
		    flags&pst_r2l?'r':' ',
		    flags&pst_ignorebaseglyphs?'b':' ',
		    flags&pst_ignoreligatures?'l':' ',
		    flags&pst_ignorecombiningmarks?'m':' ',
		    script_lang_index,
		    dx, dy, dxa, dya );
	}
	ret = uc_copy(buf);
    } else
	ret = NULL;
    GDrawDestroyWindow(gw);
return( ret );
}

static unichar_t *CounterMaskLine(SplineChar *sc, HintMask *hm) {
    unichar_t *textmask = NULL;
    int j,k,len;
    StemInfo *h;
    char buffer[100];

    for ( j=0; j<2; ++j ) {
	len = 0;
	for ( h=sc->hstem, k=0; h!=NULL && k<HntMax; h=h->next, ++k ) {
	    if ( (*hm)[k>>3]& (0x80>>(k&7)) ) {
		sprintf( buffer, "H<%g,%g>, ",
			rint(h->start*100)/100, rint(h->width*100)/100 );
		if ( textmask!=NULL )
		    uc_strcpy(textmask+len,buffer);
		len += strlen(buffer);
	    }
	}
	for ( h=sc->vstem; h!=NULL && k<HntMax; h=h->next, ++k ) {
	    if ( (*hm)[k>>3]& (0x80>>(k&7)) ) {
		sprintf( buffer, "V<%g,%g>, ",
			rint(h->start*100)/100, rint(h->width*100)/100 );
		if ( textmask!=NULL )
		    uc_strcpy(textmask+len,buffer);
		len += strlen(buffer);
	    }
	}
	if ( textmask==NULL ) {
	    textmask = galloc((len+1)*sizeof(unichar_t));
	    *textmask = '\0';
	}
    }
    if ( len>1 && textmask[len-2]==',' )
	textmask[len-2] = '\0';
return( textmask );
}

#define CID_HintMask	2020
#define HI_Width	200
#define HI_Height	260

struct hi_data {
    int done, ok, empty;
    GWindow gw;
    HintMask *cur;
    SplineChar *sc;
};

static int HI_Ok(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct hi_data *hi = GDrawGetUserData(GGadgetGetWindow(g));
	int32 i, len;
	GTextInfo **ti = GGadgetGetList(GWidgetGetControl(hi->gw,CID_HintMask),&len);

	for ( i=0; i<len; ++i )
	    if ( ti[i]->selected )
	break;

	memset(hi->cur,0,sizeof(HintMask));
	if ( i==len ) {
	    hi->empty = true;
	} else {
	    for ( i=0; i<len; ++i )
		if ( ti[i]->selected )
		    (*hi->cur)[i>>3] |= (0x80>>(i&7));
	}
	PI_ShowHints(hi->sc,GWidgetGetControl(hi->gw,CID_HintMask),false);

	hi->done = true;
	hi->ok = true;
    }
return( true );
}

static void HI_DoCancel(struct hi_data *hi) {
    hi->done = true;
    PI_ShowHints(hi->sc,GWidgetGetControl(hi->gw,CID_HintMask),false);
}

static int HI_HintSel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_listselected ) {
	struct hi_data *hi = GDrawGetUserData(GGadgetGetWindow(g));

	PI_ShowHints(hi->sc,g,true);
	/* Do I need to check for overlap here? */
    }
return( true );
}

static int HI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	HI_DoCancel( GDrawGetUserData(GGadgetGetWindow(g)));
    }
return( true );
}

static int hi_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	HI_DoCancel( GDrawGetUserData(gw));
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html#Counters");
return( true );
	}
return( false );
    }
return( true );
}

static void CI_AskCounters(CharInfo *ci,HintMask *old) {
    HintMask *cur = old != NULL ? old : chunkalloc(sizeof(HintMask));
    struct hi_data hi;
    GWindowAttrs wattrs;
    GGadgetCreateData hgcd[5], *varray[11], *harray[8], boxes[3];
    GTextInfo hlabel[5];
    GGadget *list = GWidgetGetControl(ci->gw,CID_List+600);
    int j,k;
    GRect pos;

    memset(&hi,0,sizeof(hi));
    hi.cur = cur;
    hi.sc = ci->sc;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = old==NULL?_("New Counter Mask"):_("Edit Counter Mask");
	wattrs.is_dlg = true;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,HI_Width));
	pos.height = GDrawPointsToPixels(NULL,HI_Height);
	hi.gw = GDrawCreateTopWindow(NULL,&pos,hi_e_h,&hi,&wattrs);


	memset(hgcd,0,sizeof(hgcd));
	memset(boxes,0,sizeof(boxes));
	memset(hlabel,0,sizeof(hlabel));

	j=k=0;

	hgcd[j].gd.pos.x = 20-3; hgcd[j].gd.pos.y = HI_Height-31-3;
	hgcd[j].gd.pos.width = -1; hgcd[j].gd.pos.height = 0;
	hgcd[j].gd.flags = gg_visible | gg_enabled;
	hlabel[j].text = (unichar_t *) _("Select hints between which counters are formed");
	hlabel[j].text_is_1byte = true;
	hlabel[j].text_in_resource = true;
	hgcd[j].gd.label = &hlabel[j];
	varray[k++] = &hgcd[j]; varray[k++] = NULL;
	hgcd[j++].creator = GLabelCreate;

	hgcd[j].gd.pos.x = 5; hgcd[j].gd.pos.y = 5;
	hgcd[j].gd.pos.width = HI_Width-10; hgcd[j].gd.pos.height = HI_Height-45;
	hgcd[j].gd.flags = gg_visible | gg_enabled | gg_list_multiplesel;
	hgcd[j].gd.cid = CID_HintMask;
	hgcd[j].gd.u.list = SCHintList(ci->sc,old);
	hgcd[j].gd.handle_controlevent = HI_HintSel;
	varray[k++] = &hgcd[j]; varray[k++] = NULL;
	varray[k++] = GCD_Glue; varray[k++] = NULL;
	hgcd[j++].creator = GListCreate;

	hgcd[j].gd.pos.x = 20-3; hgcd[j].gd.pos.y = HI_Height-31-3;
	hgcd[j].gd.pos.width = -1; hgcd[j].gd.pos.height = 0;
	hgcd[j].gd.flags = gg_visible | gg_enabled | gg_but_default;
	hlabel[j].text = (unichar_t *) _("_OK");
	hlabel[j].text_is_1byte = true;
	hlabel[j].text_in_resource = true;
	hgcd[j].gd.label = &hlabel[j];
	hgcd[j].gd.handle_controlevent = HI_Ok;
	harray[0] = GCD_Glue; harray[1] = &hgcd[j]; harray[2] = GCD_Glue; harray[3] = GCD_Glue;
	hgcd[j++].creator = GButtonCreate;

	hgcd[j].gd.pos.x = -20; hgcd[j].gd.pos.y = HI_Height-31;
	hgcd[j].gd.pos.width = -1; hgcd[j].gd.pos.height = 0;
	hgcd[j].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	hlabel[j].text = (unichar_t *) _("_Cancel");
	hlabel[j].text_is_1byte = true;
	hlabel[j].text_in_resource = true;
	hgcd[j].gd.label = &hlabel[j];
	hgcd[j].gd.handle_controlevent = HI_Cancel;
	harray[4] = GCD_Glue; harray[5] = &hgcd[j]; harray[6] = GCD_Glue; harray[7] = NULL;
	hgcd[j++].creator = GButtonCreate;

	boxes[2].gd.flags = gg_enabled|gg_visible;
	boxes[2].gd.u.boxelements = harray;
	boxes[2].creator = GHBoxCreate;
	varray[k++] = &boxes[2]; varray[k++] = NULL; 
	varray[k++] = GCD_Glue; varray[k++] = NULL;
	varray[k] = NULL;

	boxes[0].gd.pos.x = boxes[0].gd.pos.y = 2;
	boxes[0].gd.flags = gg_enabled|gg_visible;
	boxes[0].gd.u.boxelements = varray;
	boxes[0].creator = GHVGroupCreate;

	GGadgetsCreate(hi.gw,boxes);
	GHVBoxSetExpandableRow(boxes[0].ret,1);
	GHVBoxSetExpandableCol(boxes[2].ret,gb_expandgluesame);
	GHVBoxFitWindow(boxes[0].ret);
	GTextInfoListFree(hgcd[0].gd.u.list);

	PI_ShowHints(hi.sc,hgcd[0].ret,true);

    GDrawSetVisible(hi.gw,true);
    while ( !hi.done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(hi.gw);

    if ( !hi.ok ) {
	if ( old==NULL ) chunkfree(cur,sizeof(HintMask));
return;		/* Cancelled */
    } else if ( old==NULL && hi.empty ) {
	if ( old==NULL ) chunkfree(cur,sizeof(HintMask));
return;		/* Didn't add anything new */
    } else if ( old==NULL ) {
	GListAddStr(list,CounterMaskLine(hi.sc,cur),cur);
return;
    } else if ( !hi.empty ) {
	GListReplaceStr(list,GGadgetGetFirstListSelectedItem(list),
		CounterMaskLine(hi.sc,cur),cur);
return;
    } else {
	GListDelSelected(list);
	chunkfree(cur,sizeof(HintMask));
    }
}

static int LigCheck(SplineChar *sc,enum possub_type type,
	uint32 tag, char *components) {
    int i;
    char *pt, *start, ch;
    PST *pst;
    SplineFont *sf = sc->parent;
    SplineChar *found;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];

    buts[0] = _("_Yes");
    buts[1] = _("_Cancel");
    buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif

    if ( components==NULL || *components=='\0' )
return( true );

    if ( type==pst_ligature ) {
	for ( i=0; i<sf->glyphcnt; ++i )
	    if ( sf->glyphs[i]!=sc && sf->glyphs[i]!=NULL && sf->glyphs[i]->possub!=NULL ) {
		for ( pst=sf->glyphs[i]->possub; pst!=NULL; pst=pst->next )
			if ( pst->type==pst_ligature && pst->tag==tag &&
			    strcmp(components,pst->u.lig.components)==0 ) {
return( gwwv_ask(_("Multiple"),(const char **) buts,0,1,_("There is already a ligature made from these components\n(named %1$.40s, at local encoding %2$d).\nIs that what you want?"),sf->glyphs[i]->name,i)==0 );
		}
	    }
    }

    if ( type==pst_substitution && strcmp(components,MAC_DELETED_GLYPH_NAME)==0 )
return( true );

    start = components;
    while ( 1 ) {
	pt = strchr(start+1,' ');
	if ( pt==NULL ) pt = start+strlen(start);
	ch = *pt; *pt = '\0';
	if ( strcmp(start,sc->name)==0 && type == pst_ligature ) {
	    gwwv_post_error(_("A ligature may not be made up of itself"),_("This substitution is self-referential") );
	    *pt = ch;
return( false );
	}
	found = SFGetChar(sf,-1,start);
	if ( found==NULL ) {
	    int ret = gwwv_ask(_("Multiple"),(const char **) buts,0,1,_("The component \"%.20s\" is not in this font.\nIs that what you want?"),start);
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

static int UnicodeContainsCombiners(int uni) {
    const unichar_t *alt;

    if ( uni<0 || uni>=unicode4_size )
return( -1 );
    if ( iscombining(uni))
return( true );

    if ( !isdecompositionnormative(uni) || unicode_alternates[uni>>8]==NULL )
return( false );
    alt = unicode_alternates[uni>>8][uni&0xff];
    if ( alt==NULL )
return( false );
    while ( *alt ) {
	if ( UnicodeContainsCombiners(*alt))
return( true );
	++alt;
    }
return( false );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

uint16 PSTDefaultFlags(enum possub_type type,SplineChar *sc ) {
    uint16 flags = 0;

    if ( sc!=NULL ) {
	if ( SCRightToLeft(sc))
	    flags = pst_r2l;
#if 0		/* Better not. Yudit doesn't support the ligature substitution if the bit is set */
	if ( type==pst_ligature ) {
	    int script = SCScriptFromUnicode(sc);
	    if ( ScriptIsRightToLeft(script) ) {
		if ( !UnicodeContainsCombiners(sc->unicodeenc))
		    flags |= pst_ignorecombiningmarks;
	    }
	}
#endif
    }
return( flags );
}

static enum possub_type PSTGuess(char *data) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    enum possub_type type;
    int i;
    uint32 tag;

    if ( data[0]=='<' ) {
	type=pst_max;
    } else {
	tag = (((uint8 *) data)[0]<<24) | (((uint8 *) data)[1]<<16 ) |
		    (((uint8 *) data)[2]<<8) | ((uint8 *) data)[3];
	for ( type=pst_position; type<pst_max; ++type ) {
	    for ( i=0; pst_tags[type-1][i].text!=NULL; ++i ) {
		if ( (uint32) (intpt) pst_tags[type-1][i].userdata==tag )
	    break;
	    }
	    if ( pst_tags[type-1][i].text!=NULL )
	break;
	}
    }
    if ( type==pst_max )
return( pst_null );

return( type );
#else
return( pst_null );
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static unichar_t *SLICheck(SplineChar *sc,unichar_t *data,SplineFont *copied_from) {
    /* We've got a string. Don't know what font it came from, can't really */
    /*  make the sli right. Best we can do is insure against crashes and hope */
    /*  it was copied from us */
    int new;
    int merge, act, macfeat;
    uint32 tag;
    uint16 flags, sli;
    unichar_t *name, *ret;
    char *u8name;

    DecomposeClassName(data,&name,&tag,&macfeat,&flags,&sli,&merge,&act,sc->parent);

    new = SFConvertSLI(copied_from,sli,sc->parent,sc);

    if ( sli==new ) {
	free(name);
return( data );
    }

    u8name = u2utf8_copy(name);
    free(name);
    ret = ClassName(u8name,tag,flags,new,merge,act,macfeat,sc->parent);
    free(u8name); free(data);
return( ret );
}

static void CI_DoNew(CharInfo *ci, unichar_t *def) {
    int32 len; int i, sel;
    GTextInfo **old, **new;
    GGadget *list;
    unichar_t *newname, *upt;
    uint16 flags=0;

    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
    flags = PSTDefaultFlags(sel+1,ci->sc);

    if ( sel==7 ) {
	CI_AskCounters(ci,NULL);
return;
    }

    newname = sel<=1 
	    ? AskPosTag(newstrings[sel],def,0,flags,-1,sel+1,ci->sc->parent,ci->sc)
	    : AskNameTag(newstrings[sel],def,0,flags,-1,sel+1,ci->sc->parent,ci->sc,-1,-1);
    if ( newname!=NULL ) {
	if ( sel>1 ) {
	    uint32 tag; int macfeat;
	    unichar_t *comp;
	    char *components;
	    DecomposeClassName(newname,&comp,&tag,&macfeat,NULL,NULL,NULL,NULL,
		    ci->sc->parent);
	    components = u2utf8_copy(comp);
	    free(comp);
	    if ( !LigCheck(ci->sc,sel+1,tag,components)) {
		free( newname ); free(components);
return;
	    }
	    free(components);
	}
	list = GWidgetGetControl(ci->gw,CID_List+sel*100);
	old = GGadgetGetList(list,&len);
	upt = DecomposeClassName(newname,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		ci->sc->parent);
	for ( i=0; i<len; ++i ) {
	    if ( u_strncmp(old[i]->text,newname,upt-newname)==0 )
	break;
	}
	if ( i<len && sel+1!=pst_pair && sel+1!=pst_ligature ) {
/* GT: We have a tag that appears twice and that's bad. */
	    gwwv_post_error(_("Duplicate Tag"),_("Duplicate Tag"));
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
    if ( sel<=pst_pair || sel>=pst_lcaret ) {
	GDrawBeep(NULL);
return;
    }

    if ( !GDrawSelectionHasType(ci->gw,sn_drag_and_drop,"STRING"))
return;
    cnames = GDrawRequestSelection(ci->gw,sn_drag_and_drop,"STRING",&len);
    if ( cnames==NULL )
return;

    if ( sel==pst_substitution && strchr(cnames,' ')!=NULL ) {
	gwwv_post_error(_("Too Many Components"),_("A simple substitution takes exactly one component, but you have provided several"));
	free(cnames);
return;
    }

    unames = ClassName(cnames,CHR(' ',' ',' ',' '),PSTDefaultFlags(sel,ci->sc),
	    -1,-1,-1,false,ci->sc->parent);
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
    int32 len; int i,j, offset;
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
	if ( offset==600 ) {
	    for ( i=0; i<len; ++i ) if ( old[i]->selected )
		chunkfree(old[i]->userdata,sizeof(HintMask));
	}
	GGadgetSetList(list,new,false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Delete+offset),false);
	GGadgetSetEnabled(GWidgetGetControl(GGadgetGetWindow(g),CID_Edit+offset),false);
    }
return( true );
}

static int CI_Edit(GGadget *g, GEvent *e) {
    int32 len; int i;
    GTextInfo **old, **new, *ti;
    GGadget *list;
    CharInfo *ci;
    unichar_t *newname, *upt;
    int sel;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	ci = GDrawGetUserData(GGadgetGetWindow(g));
	sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
	if ( sel==7 ) sel=6;
	list = GWidgetGetControl(GGadgetGetWindow(g),CID_List+sel*100);
	if ( (ti = GGadgetGetListItemSelected(list))==NULL )
return( true );
	if ( sel==6 ) {
	    CI_AskCounters(ci,ti->userdata);
return(true);
	}
	newname = sel<=1 
		? AskPosTag(editstrings[sel],ti->text,0,0,0,sel+1,ci->sc->parent,ci->sc)
		: AskNameTag(editstrings[sel],ti->text,0,0,0,sel+1,ci->sc->parent,ci->sc,-1,-1);
	if ( newname!=NULL ) {
	    old = GGadgetGetList(list,&len);
	    upt = DecomposeClassName(newname,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		    ci->sc->parent);
	    for ( i=0; i<len; ++i ) if ( old[i]!=ti ) {
		if ( u_strncmp(old[i]->text,newname,upt-newname)==0 )
	    break;
	    }
	    if ( i<len && sel+1!=pst_ligature && sel+1!=pst_pair ) {
		gwwv_post_error(_("Duplicate Tag"),_("Duplicate Tag"));
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
    int sel, i, cnt;
    int32 len;
    GTextInfo **tis;
    char **data;

    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
    list = GWidgetGetControl(ci->gw,CID_List+sel*100);
    if ( list==NULL ) {
	GGadgetActiveGadgetEditCmd(ci->gw,ec_copy);
return;
    }
    tis = GGadgetGetList(list,&len);
    for ( i=cnt=0; i<len; ++i )
	if ( tis[i]->selected )
	    ++cnt;
    if ( cnt==0 )
return;
    data = gcalloc(cnt+1,sizeof(char *));
    for ( i=cnt=0; i<len; ++i )
	if ( tis[i]->selected )
	    data[cnt++] = cu_copy(tis[i]->text);
    PosSubCopy(sel+1,data,ci->sc->parent);
    CI_CanPaste(ci);
}

static int CI_Copy(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	CI_DoCopy( GDrawGetUserData(GGadgetGetWindow(g)) );
return( true );
}

static void CI_DoPaste(CharInfo *ci,char **data, enum possub_type type,SplineFont *copied_from) {
    GGadget *list;
    int sel, i,j,k, cnt;
    int32 len;
    uint32 tag;
    GTextInfo **tis, **newlist;
    char **tempdata, *paste;
    char *pt;
    int lcnt, pst_depth;

    pst_depth = data==NULL ? 0 : -1;
    forever {
	tempdata = NULL; paste = NULL;
	if ( data==NULL )
	    data = CopyGetPosSubData(&type,&copied_from,pst_depth);
	if ( data==NULL && pst_depth!=0 )
return;
	if ( data==NULL ) {
	    int32 plen;
	    sel = GTabSetGetSel(GWidgetGetControl(ci->gw,CID_Tabs))-2;
	    paste = GDrawRequestSelection(ci->gw,sn_clipboard,"STRING",&plen);
	    if ( paste==NULL || plen==0 )
return;
	    if ( paste[strlen(paste)-1]=='\n' ) paste[strlen(paste)-1] = '\0';
	    for ( pt=paste, lcnt=1; *pt; ++pt )
		if ( *pt=='\n' ) ++lcnt;
	    tempdata = gcalloc(lcnt+1,sizeof(char *));
	    tempdata[0] = paste;
	    for ( pt=paste, lcnt=1; *pt; ++pt )
		if ( *pt=='\n' ) {
		    tempdata[lcnt++] = pt+1;
		    *pt = '\0';
		}
	    data = tempdata;
	    if ( paste[0]=='<' )
		type = pst_null;
	    else {
		tag = (((uint8 *) paste)[0]<<24) | (((uint8 *) paste)[1]<<16 ) |
			    (((uint8 *) paste)[2]<<8) | ((uint8 *) paste)[3];
		type = pst_null;
		if ( sel+1>pst_null && sel+1<pst_max ) {
		    for ( i=0; pst_tags[sel][i].text!=NULL; ++i )
			if ( (uint32) (intpt) pst_tags[sel][i].userdata == tag ) {
			    type = sel+1;
		    break;
		    }
		}
	    }
	    if ( type==pst_null )
		type = PSTGuess(paste);
	    if ( type==pst_null ) {
		free(paste); free(tempdata);
		gwwv_post_error(_("Bad GPOS/GSUB"),_("Bad GPOS/GSUB"));
return;
	    }
	}

	list = GWidgetGetControl(ci->gw,CID_List+(type-1)*100);
	if ( list==NULL )
    continue;
	tis = GGadgetGetList(list,&len);

	for ( cnt=0; data[cnt]!=NULL; ++cnt );
	newlist = galloc((len+cnt+1)*sizeof(GTextInfo *));
	for ( i=0; i<len; ++i ) {
	    newlist[i] = galloc(sizeof(GTextInfo));
	    *newlist[i] = *tis[i];
	    newlist[i]->text = u_copy(tis[i]->text);
	}
	k = 0;
	for ( i=0; i<cnt; ++i ) {
	    unichar_t *udata = uc_copy(data[i]);
	    unichar_t *upt = DecomposeClassName(udata,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
		    ci->sc->parent);
	    for ( j=0; j<len; ++j )
		if ( u_strncmp(newlist[j]->text,udata,upt-udata)==0 )
	    break;
	    if ( j<len ) {
		free(newlist[j]->text);
		newlist[j]->text = SLICheck(ci->sc,udata,copied_from);
	    } else {
		newlist[len+k] = gcalloc(1,sizeof(GTextInfo));
		newlist[len+k]->fg = newlist[len+k]->bg = COLOR_DEFAULT;
		newlist[len+k]->text = SLICheck(ci->sc,udata,copied_from);
		++k;
	    }
	}
	newlist[len+k] = gcalloc(1,sizeof(GTextInfo));
	GGadgetSetList(list,newlist,false);
	free(paste); free(tempdata);
	data = NULL;
	if ( pst_depth==-1 )
return;
	++pst_depth;
    }
}

static int CI_Paste(GGadget *g, GEvent *e) {

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	CI_DoPaste( GDrawGetUserData(GGadgetGetWindow(g)),NULL,pst_null,NULL );
return( true );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static int MultipleValues(char *name, int local) {
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_No"); buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif
    if ( gwwv_ask(_("Multiple"),(const char **) buts,0,1,_("There is already a glyph with this Unicode encoding\n(named %1$.40s, at local encoding %2$d).\nIs that what you want?"),name,local)==0 )
return( true );

return( false );
}

static int MultipleNames(void) {
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_Cancel"); buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif
    if ( gwwv_ask(_("Multiple"),(const char **) buts,0,1,_("There is already a glyph with this name,\ndo you want to swap names?"))==0 )
return( true );

return( false );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
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
    if ( *end || val<0 || val>0x10ffff ) {
	Protest8( _("Unicode _Value:") );
return( -2 );
    }
return( val );
}

static void SetNameFromUnicode(GWindow gw,int cid,int val) {
    unichar_t *temp;
    char buf[100];
    CharInfo *ci = GDrawGetUserData(gw);

    temp = utf82u_copy(StdGlyphName(buf,val,ci->sc->parent->uni_interp,ci->sc->parent->for_new_glyphs));
    GGadgetSetTitle(GWidgetGetControl(gw,cid),temp);
    free(temp);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void SCInsertPST(SplineChar *sc,PST *new) {
#if 0
    PST *old, *prev;

    if ( new->type == pst_ligature || new->type==pst_pair ) {
#endif
	new->next = sc->possub;
	sc->possub = new;
#if 0
return;
    }
    for ( old=sc->possub, prev = NULL; old!=NULL; prev = old, old = old->next ) {
	if ( old->tag==new->tag && old->type==new->type &&
		old->script_lang_index == new->script_lang_index ) {
	    new->next = old->next;
	    PSTFree(old);
    break;
	}
    }
    if ( prev==NULL )
	sc->possub = new;
    else
	prev->next = new;
#endif
}

PST *SCFindPST(SplineChar *sc,int type,uint32 tag,int sli,int flags) {
    PST *old;

    for ( old=sc->possub; old!=NULL; old = old->next ) {
	if ( old->tag==tag && old->type==type &&
		(old->script_lang_index == sli || sli==-1) &&
		(old->flags == flags || flags==-1))
return( old );
    }
return( NULL );
}

static int ParseVR(unichar_t *end,struct vr *vr,unichar_t **done) {
    unichar_t *pt;

    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    vr->xoff = u_strtol(pt,&end,10);
    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    vr->yoff = u_strtol(pt,&end,10);
    for ( pt=end; *pt!='\0' && *pt!='='; ++pt ); if ( *pt=='=' ) ++pt;
    vr->h_adv_off = u_strtol(pt,&end,10);
    for ( pt=end; *pt!='\0' && *pt!='='; ++pt );
    if ( *pt=='=' )
	++pt;
    else {
	gwwv_post_error(_("Bad GPOS/GSUB"),_("Bad format for position data, expected four = characters with data"));
return(false);
    }
    vr->v_adv_off = u_strtol(pt,&end,10);
    *done = end;
return( true );
}

static void SCAppendKernPST(SplineChar *sc,char *data,enum possub_type type,SplineFont *copied_from) {
    SplineChar *first, *second, *other;
    SplineFont *sf = sc->parent;
    char name[400];
    KernPair **head, *kp;
    int sli, flags, offset;
    char *pt;

    pt = data+5;
    flags = 0;
    if ( pt[0]=='r' ) flags = pst_r2l;
    if ( pt[1]=='b' ) flags = pst_ignorebaseglyphs;
    if ( pt[2]=='l' ) flags = pst_ignoreligatures;
    if ( pt[3]=='m' ) flags = pst_ignorecombiningmarks;
    if ( sscanf( pt+5, "%d %s offset=%d", &sli, name, &offset )!=3 )
return;
    sli = SFConvertSLI(copied_from,sli,sc->parent,sc);
    other = SFGetChar(sf,-1,name);
    if ( other==NULL )
return;
    if ( type==pst_kerning || type==pst_vkerning ) {
	first = sc;
	second = other;
    } else {
	first = other;
	second = sc;
    }
    head = ( type==pst_kerning || type==pst_kernback ) ? &first->kerns : &first->vkerns;
    for ( kp=*head; kp!=NULL; kp=kp->next )
	if ( kp->sc == second )
    break;
    if ( kp==NULL ) {
	kp = chunkalloc(sizeof(KernPair));
	kp->next = *head;
	kp->sc = second;
	*head = kp;
    }
    kp->off = offset;
    kp->sli = sli;
    kp->flags = flags;
}

void SCAppendPosSub(SplineChar *sc,enum possub_type type, char **d,SplineFont *copied_from) {
    PST *new;
    char *data;
    unichar_t *pt, *end, *rest, *udata, *other, *spt, *tpt;
    char *cend;
    int i, macfeat;
    uint16 flags;

    if ( d==NULL )
return;

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( sc->charinfo!=NULL && type<pst_kerning ) {
	CI_DoPaste(sc->charinfo,d,type,copied_from);
	GDrawRaise(sc->charinfo->gw);
return;
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    for ( i=0; d[i]!=NULL; ++i ) {
	data = d[i];
	if ( data[0]=='<' && (strtol(data+1,&cend,10),*cend==',') &&
		(strtol(cend+1,&cend,10),*cend=='>') && cend[1]==' ' &&
		cend[6]==' ' )
	    /* Don't check any further */;
	else if ( strlen(data)<10 || data[4]!=' ' || (data[9]!=' ' && data[9]!='(') ) {
	    gwwv_post_error(_("Bad GPOS/GSUB"),_("The string must start with a 4 character type field, be followed by a space, and then contain the information"));
return;
	}
	if ( type==pst_null ) {
	    type = PSTGuess(data);
	    if ( type==pst_null ) {
		gwwv_post_error(_("Bad GPOS/GSUB"),_("Bad GPOS/GSUB"));
return;
	    }
	}
	if ( type==pst_kerning || type==pst_vkerning ||
		type==pst_kernback || type == pst_vkernback )
	    SCAppendKernPST(sc,data,type,copied_from);
	else {
	    new = chunkalloc(sizeof(PST));
	    new->type = type;
	    udata = uc_copy(data);
	    DecomposeClassName(udata,&rest,&new->tag,&macfeat,
		    &flags, &new->script_lang_index,
		    NULL,NULL,sc->parent);
	    new->script_lang_index = SFConvertSLI(copied_from,new->script_lang_index,sc->parent,sc);
	    new->flags = flags;
	    new->macfeature = macfeat;
	    free(udata);

	    if ( type==pst_position ) {
		if ( !ParseVR(rest,&new->u.pos,&end)) {
		    chunkfree(new,sizeof(PST));
		    free(rest);
return;
		}
	    } else if ( type==pst_pair ) {
		for ( pt=rest; *pt==' ' ; ++pt );
		other = pt;
		while ( *pt!=' ' && *pt!='\0' ) ++pt;
		new->u.pair.paired = cu_copyn(other,pt-other);
		new->u.pair.vr = chunkalloc(sizeof(struct vr [2]));
		if ( !ParseVR(pt,&new->u.pair.vr[0],&end)) {
		    free(new->u.pair.paired);
		    chunkfree(new->u.pair.vr,sizeof(struct vr [2]));
		    chunkfree(new,sizeof(PST));
		    free(rest);
return;
		}
		if ( !ParseVR(end,&new->u.pair.vr[1],&end)) {
		    free(new->u.pair.paired);
		    chunkfree(new->u.pair.vr,sizeof(struct vr [2]));
		    chunkfree(new,sizeof(PST));
		    free(rest);
return;
		}
	    } else {
		/* remove leading/training spaces */
		for ( pt=rest; *pt==' '; ++pt );
		for ( end=pt+u_strlen(pt)-1; *pt==' '; --pt )
		    *pt = '\0';
		if ( type==pst_substitution && u_strchr(pt,' ')!=NULL ) {
		    gwwv_post_error(_("Bad GPOS/GSUB"),_("A simple substitution must have exactly one component"));
		    free(rest);
return;
		}
		/* Remove multiple spaces */
		for ( spt=tpt=pt; *spt; ++spt ) {
		    *tpt++ = *spt;
		    if ( *spt==' ' ) {
			while ( *spt==' ' ) ++spt;
			--spt;
		    }
		}
		*tpt = '\0';
		new->u.subs.variant = cu_copy(pt);
		if ( type==pst_ligature )
		    new->u.lig.lig = sc;
	    }

	    SCInsertPST(sc,new);
	    free(rest);
	}
    }
    if ( i!=0 )
	sc->parent->changed = true;
}
		
int SCSetMetaData(SplineChar *sc,char *name,int unienc,const char *comment) {
    SplineFont *sf = sc->parent;
    int i, mv=0;
    int isnotdef, samename=false;
    struct altuni *alt;

    for ( alt=sc->altuni; alt!=NULL && alt->unienc!=unienc; alt=alt->next );
    if ( (sc->unicodeenc == unienc || alt!=NULL ) && strcmp(name,sc->name)==0 ) {
	samename = true;	/* No change, it must be good */
    }
    if ( alt!=NULL || !samename ) {
	isnotdef = strcmp(name,".notdef")==0;
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && sf->glyphs[i]!=sc ) {
	    if ( unienc!=-1 && sf->glyphs[i]->unicodeenc==unienc ) {
		if ( !mv && !MultipleValues(sf->glyphs[i]->name,i)) {
return( false );
		}
		mv = 1;
	    } else if ( !isnotdef && strcmp(name,sf->glyphs[i]->name)==0 ) {
		if ( !MultipleNames()) {
return( false );
		}
		free(sf->glyphs[i]->name);
		sf->glyphs[i]->namechanged = true;
		if ( strncmp(sc->name,"uni",3)==0 && sf->glyphs[i]->unicodeenc!=-1) {
		    char buffer[12];
		    if ( sf->glyphs[i]->unicodeenc<0x10000 )
			sprintf( buffer,"uni%04X", sf->glyphs[i]->unicodeenc);
		    else
			sprintf( buffer,"u%04X", sf->glyphs[i]->unicodeenc);
		    sf->glyphs[i]->name = copy(buffer);
		} else {
		    sf->glyphs[i]->name = sc->name;
		    sc->name = NULL;
		}
	    break;
	    }
	}
	if ( sc->unicodeenc!=unienc ) {
	    struct splinecharlist *scl;
	    int layer;
	    RefChar *ref;

	    for ( scl=sc->dependents; scl!=NULL; scl=scl->next ) {
		for ( layer=ly_fore; layer<scl->sc->layer_cnt; ++layer )
		    for ( ref = scl->sc->layers[layer].refs; ref!=NULL; ref=ref->next )
			if ( ref->sc==sc )
			    ref->unicode_enc = unienc;
	    }
	}
    }
    if ( alt!=NULL )
	alt->unienc = sc->unicodeenc;
    sc->unicodeenc = unienc;
    if ( sc->name==NULL || strcmp(name,sc->name)!=0 ) {
	free(sc->name);
	sc->name = copy(name);
	sc->namechanged = true;
	GlyphHashFree(sf);
    }
    sf->changed = true;
    if ( unienc>=0xe000 && unienc<=0xf8ff )
	/* Ok to name things in the private use area */;
    else if ( samename )
	/* Ok to name it itself */;
    else {
	FontView *fvs;
	for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	    int enc = fvs->map->backmap[sc->orig_pos];
	    if ( enc!=-1 && ((fvs->map->enc->only_1byte && enc<256) ||
			(fvs->map->enc->has_2byte && enc<65535 ))) {
		fvs->map->enc = &custom;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		FVSetTitle(fvs);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	    }
	}
    }
    free(sc->comment); sc->comment = NULL;
    if ( comment!=NULL && *comment!='\0' )
	sc->comment = copy(comment);

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    SCRefreshTitles(sc);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
return( true );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int CI_ProcessPosSubs(CharInfo *ci) {
    int i, j;
    int32 len;
    GTextInfo **tis;
    PST *old = ci->sc->possub, *prev, *lcaret;
    char **data;

    for ( prev=NULL, lcaret=old; lcaret!=NULL && lcaret->type!=pst_lcaret;
	    prev = lcaret, lcaret=lcaret->next );
    if ( lcaret!=NULL ) {
	if ( prev==NULL )
	    old = lcaret->next;
	else
	    prev->next = lcaret->next;
	lcaret->next = NULL;
    }

    ci->sc->possub = lcaret;
    ci->sc->charinfo = NULL;	/* Without this we put them back into the charinfo dlg */
    for ( i=0; i<6; ++i ) {
	tis = GGadgetGetList(GWidgetGetControl(ci->gw,CID_List+i*100),&len);
	if ( len!=0 ) {
	    data = galloc((len+1)*sizeof(char *));
	    for ( j=0; j<len; ++j )
		data[j] = cu_copy(tis[j]->text);
	    data[j] = NULL;
	    SCAppendPosSub(ci->sc,i+1,data,ci->sc->parent);
	    for ( j=0; j<len; ++j )
		free(data[j]);
	    free(data);
	}
    }
    ci->sc->charinfo = ci;
    PSTFree(old);
    SCLigCaretCheck(ci->sc,true);
return( true );
}

static int CI_NameCheck(const unichar_t *name) {
    int bad, questionable;
    extern int allow_utf8_glyphnames;
#if defined(FONTFORGE_CONFIG_GDRAW)
    char *buts[3];
    buts[0] = _("_Yes"); buts[1]=_("_No"); buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif

    if ( uc_strcmp(name,".notdef")==0 )		/* This name is a special case and doesn't follow conventions */
return( true );
    if ( u_strlen(name)>31 ) {
	gwwv_post_error(_("Bad Name"),_("Glyph names are limitted to 31 characters"));
return( false );
    } else if ( *name=='\0' ) {
	gwwv_post_error(_("Bad Name"),_("Bad Name"));
return( false );
    } else if ( isdigit(*name) || *name=='.' ) {
	gwwv_post_error(_("Bad Name"),_("A glyph name may not start with a digit nor a full stop (period)"));
return( false );
    }
    bad = questionable = false;
    while ( *name ) {
	if ( *name<=' ' || (!allow_utf8_glyphnames && *name>=0x7f) ||
		*name=='(' || *name=='[' || *name=='{' || *name=='<' ||
		*name==')' || *name==']' || *name=='}' || *name=='>' ||
		*name=='%' || *name=='/' )
	    bad=true;
	else if ( !isalnum(*name) && *name!='.' && *name!='_' )
	    questionable = true;
	++name;
    }
    if ( bad ) {
	gwwv_post_error(_("Bad Name"),_("A glyph name must be ASCII, without spaces and may not contain the characters \"([{<>}])/%%\", and should contain only alphanumerics, periods and underscores"));
return( false );
    } else if ( questionable ) {
	if ( gwwv_ask(_("Bad Name"),(const char **) buts,0,1,_("A glyph name should contain only alphanumerics, periods and underscores\nDo you want to use this name in spite of that?"))==1 )
return(false);
    }
return( true );
}

static void CI_ParseCounters(CharInfo *ci) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(ci->gw,CID_List+600),&len);
    SplineChar *sc = ci->sc;

    free(sc->countermasks);

    sc->countermask_cnt = len;
    if ( len==0 )
	sc->countermasks = NULL;
    else {
	sc->countermasks = galloc(len*sizeof(HintMask));
	for ( i=0; i<len; ++i ) {
	    memcpy(sc->countermasks[i],ti[i]->userdata,sizeof(HintMask));
	    chunkfree(ti[i]->userdata,sizeof(HintMask));
	    ti[i]->userdata = NULL;
	}
    }
}

static int gettex(GWindow gw,int cid,char *msg,int *err) {
    const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(gw,cid));

    if ( *ret=='\0' )
return( TEX_UNDEF );
return( GetInt8(gw,cid,msg,err));
}

static int _CI_OK(CharInfo *ci) {
    int val;
    int ret, refresh_fvdi=0;
    char *name, *comment;
    const unichar_t *nm;
    FontView *fvs;
    int err = false;
    int tex_height, tex_depth, tex_sub, tex_super;

    val = ParseUValue(ci->gw,CID_UValue,true,ci->sc->parent);
    if ( val==-2 )
return( false );
    tex_height = gettex(ci->gw,CID_TeX_Height,_("Height:"),&err);
    tex_depth  = gettex(ci->gw,CID_TeX_Depth ,_("Depth:") ,&err);
    tex_sub    = gettex(ci->gw,CID_TeX_Sub   ,_("Sub Pos:"),&err);
    tex_super  = gettex(ci->gw,CID_TeX_Super ,_("Sup Pos:"),&err);
    if ( err )
return( false );
    if ( !CI_ProcessPosSubs(ci))
return( false );
    nm = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName));
    if ( !CI_NameCheck(nm) )
return( false );
    name = u2utf8_copy( nm );
    if ( strcmp(name,ci->sc->name)!=0 || val!=ci->sc->unicodeenc )
	refresh_fvdi = 1;
    comment = GGadgetGetTitle8(GWidgetGetControl(ci->gw,CID_Comment));
    SCPreserveState(ci->sc,2);
    ret = SCSetMetaData(ci->sc,name,val,comment);
    free(name); free(comment);
    if ( refresh_fvdi ) {
	for ( fvs=ci->sc->parent->fv; fvs!=NULL; fvs=fvs->next ) {
	    GDrawRequestExpose(fvs->gw,NULL,false);	/* Redraw info area just in case this char is selected */
	    GDrawRequestExpose(fvs->v,NULL,false);	/* Redraw character area in case this char is on screen */
	}
    }
    if ( ret ) {
	ci->sc->glyph_class = GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_GClass));
	val = GGadgetGetFirstListSelectedItem(GWidgetGetControl(ci->gw,CID_Color));
	if ( val!=-1 ) {
	    if ( ci->sc->color != (int) (intpt) (std_colors[val].userdata) ) {
		ci->sc->color = (intpt) (std_colors[val].userdata);
		for ( fvs=ci->sc->parent->fv; fvs!=NULL; fvs=fvs->next )
		    GDrawRequestExpose(fvs->v,NULL,false);	/* Redraw info area just in case this char is selected */
	    }
	}
	CI_ParseCounters(ci);
	ci->sc->tex_height = tex_height;
	ci->sc->tex_depth  = tex_depth;
	ci->sc->tex_sub_pos   = tex_sub;
	ci->sc->tex_super_pos = tex_super;
    }
    if ( ret )
	ci->sc->parent->changed = true;
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static char *LigDefaultStr(int uni, char *name, int alt_lig ) {
    const unichar_t *alt=NULL, *pt;
    char *components = NULL;
    int len;
    const char *uname;
    unichar_t hack[30], *upt;
    char buffer[80];

    /* If it's not (bmp) unicode we have no info on it */
    /*  Unless it looks like one of adobe's special ligature names */
    if ( uni==-1 || uni>=0x10000 )
	/* Nope */;
    else if ( isdecompositionnormative(uni) &&
		unicode_alternates[uni>>8]!=NULL &&
		(alt = unicode_alternates[uni>>8][uni&0xff])!=NULL ) {
	if ( alt[1]=='\0' )
	    alt = NULL;		/* Single replacements aren't ligatures */
	else if ( iscombining(alt[1]) && ( alt[2]=='\0' || iscombining(alt[2]))) {
	    if ( alt_lig != -10 )	/* alt_lig = 10 => mac unicode decomp */
		alt = NULL;		/* Otherwise, don't treat accented letters as ligatures */
	} else if ( _UnicodeNameAnnot!=NULL &&
		(uname = _UnicodeNameAnnot[uni>>16][(uni>>8)&0xff][uni&0xff].name)!=NULL &&
		strstr(uname,"LIGATURE")==NULL &&
		strstr(uname,"VULGAR FRACTION")==NULL &&
		uni!=0x152 && uni!=0x153 &&	/* oe ligature should not be standard */
		uni!=0x132 && uni!=0x133 &&	/* nor ij */
		(uni<0xfb2a || uni>0xfb4f) &&	/* Allow hebrew precomposed chars */
		uni!=0x215f &&
		!((uni>=0x0958 && uni<=0x095f) || uni==0x929 || uni==0x931 || uni==0x934)) {
	    alt = NULL;
	} else if ( _UnicodeNameAnnot==NULL ) {
	    if ( (uni>=0xbc && uni<=0xbe ) ||		/* Latin1 fractions */
		    (uni>=0x2153 && uni<=0x215e ) ||	/* other fractions */
		    (uni>=0xfb00 && uni<=0xfb06 ) ||	/* latin ligatures */
		    (uni>=0xfb13 && uni<=0xfb17 ) ||	/* armenian ligatures */
		    uni==0xfb17 ||			/* hebrew ligature */
		    (uni>=0xfb2a && uni<=0xfb4f ) ||	/* hebrew precomposed chars */
		    (uni>=0xfbea && uni<=0xfdcf ) ||	/* arabic ligatures */
		    (uni>=0xfdf0 && uni<=0xfdfb ) ||	/* arabic ligatures */
		    (uni>=0xfef5 && uni<=0xfefc ))	/* arabic ligatures */
		;	/* These are good */
	    else
		alt = NULL;
	}
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
	if ( alt[1]==0x2044 && (alt[2]==0 || alt[3]==0) && alt_lig==1 ) {
	    u_strcpy(hack,alt);
	    hack[1] = '/';
	    alt = hack;
	} else if ( alt_lig>0 )
return( NULL );

	if ( isarabisolated(uni) || isarabinitial(uni) || isarabmedial(uni) || isarabfinal(uni) ) {
	    /* If it is arabic, then convert from the unformed version to the formed */
	    if ( u_strlen(alt)<sizeof(hack)/sizeof(hack[0])-1 ) {
		u_strcpy(hack,alt);
		for ( upt=hack ; *upt ; ++upt ) {
		    /* Make everything medial */
		    if ( *upt>=0x600 && *upt<=0x6ff )
			*upt = ArabicForms[*upt-0x600].medial;
		}
		if ( isarabisolated(uni) || isarabfinal(uni) ) {
		    int len = upt-hack-1;
		    if ( alt[len]>=0x600 && alt[len]<=0x6ff )
			hack[len] = ArabicForms[alt[len]-0x600].final;
		}
		if ( isarabisolated(uni) || isarabinitial(uni) ) {
		    if ( alt[0]>=0x600 && alt[0]<=0x6ff )
			hack[0] = ArabicForms[alt[0]-0x600].initial;
		}
		alt = hack;
	    }
	}

	components=NULL;
	while ( 1 ) {
	    len = 0;
	    for ( pt=alt; *pt; ++pt ) {
		if ( components==NULL ) {
		    len += strlen(StdGlyphName(buffer,*pt,ui_none,(NameList *)-1))+1;
		} else {
		    const char *temp = StdGlyphName(buffer,*pt,ui_none,(NameList *)-1);
		    strcpy(components+len,temp);
		    len += strlen( temp );
		    components[len++] = ' ';
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
	    if ( sscanf(pt,"%4x", (unsigned *) &uni )==0 ) {
		free(components); components = NULL;
	break;
	    }
	    next = StdGlyphName(buffer,uni,ui_none,(NameList *)-1);
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

    if (( uni>=0xbc && uni<=0xbe ) || (uni>=0x2153 && uni<=0x215f) )
	tag = CHR('f','r','a','c');	/* Fraction */
    /* hebrew precomposed characters */
    else if ( uni>=0xfb2a && uni<=0xfb4e )
	tag = CHR('c','c','m','p');
    else if ( uni==0xfb4f )
	tag = CHR('h','l','i','g');
    /* armenian */
    else if ( uni>=0xfb13 && uni<=0xfb17 )
	tag = CHR('l','i','g','a');
    /* devanagari ligatures */
    else if ( (uni>=0x0958 && uni<=0x095f) || uni==0x931 || uni==0x934 || uni==0x929 )
	tag = CHR('n','u','k','t');
    else switch ( uni ) {
      case 0xfb05:		/* long-s t */
	/* This should be 'liga' for long-s+t and 'hlig' for s+t */
	tag = CHR('l','i','g','a');
      break;
      case 0x00c6: case 0x00e6:		/* ae, AE */
      case 0x0152: case 0x0153:		/* oe, OE */
      case 0x0132: case 0x0133:		/* ij, IJ */
      case 0xfb06:			/* s t */
	tag = CHR('d','l','i','g');
      break;
      case 0xfefb: case 0xfefc:	/* Lam & Alef, required ligs */
	tag = CHR('r','l','i','g');
      break;
    }
#if 0
    if ( tag==CHR('l','i','g','a') && uni!=-1 && uni<0x10000 ) {
	const unichar_t *alt=NULL;
	if ( isdecompositionnormative(uni) &&
		    unicode_alternates[uni>>8]!=NULL &&
		(alt = unicode_alternates[uni>>8][uni&0xff])!=NULL ) {
	    if ( iscombining(alt[1]) && ( alt[2]=='\0' || iscombining(alt[2])))
		tag = ((27<<16)|1);
	}
    }
#endif
return( tag );
}

static PST *AddPos(PST *last,uint32 tag,int dx, int dy, int dxa, int dya, uint16 flags,
	SplineChar *sc) {
    PST *pos = chunkalloc(sizeof(PST));
    pos->tag = tag;
    pos->flags = flags;
    pos->type = pst_position;
    pos->next = last;
    pos->script_lang_index = SFFindBiggestScriptLangIndex(sc->parent,
			SCScriptFromUnicode(sc),DEFAULT_LANG);
    pos->u.pos.xoff = dx;
    pos->u.pos.yoff = dy;
    pos->u.pos.h_adv_off = dxa;
    pos->u.pos.v_adv_off = dya;
return( pos );
}

PST *AddSubs(PST *last,uint32 tag,char *name,uint16 flags,
	uint16 sli,SplineChar *sc) {
    PST *sub = chunkalloc(sizeof(PST));
    sub->tag = tag;
    if ( tag<CHR(' ',' ',' ',' ') || tag>0x7f000000 )
	sub->macfeature = true;
    sub->flags = flags;
    sub->type = pst_substitution;
    if ( sli==SLI_UNKNOWN )
	sub->script_lang_index = SFFindBiggestScriptLangIndex(sc->parent,
			    SCScriptFromUnicode(sc),DEFAULT_LANG);
    else
	sub->script_lang_index = sli;
    sub->next = last;
    sub->u.subs.variant = copy(name);
return( sub );
}

static SplineChar *SuffixCheck(SplineChar *sc,char *suffix) {
    SplineChar *alt = NULL;
    SplineFont *sf = sc->parent;
    char namebuf[100];

    if ( *suffix=='.' ) ++suffix;
    if ( sf->cidmaster!=NULL ) {
	sprintf( namebuf, "%.20s.%d.%s", sf->cidmaster->ordering, sc->orig_pos, suffix );
	alt = SFGetChar(sf,-1,namebuf);
	if ( alt==NULL ) {
	    sprintf( namebuf, "cid-%d.%s", sc->orig_pos, suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
    }
    if ( alt==NULL && sc->unicodeenc!=-1 ) {
	sprintf( namebuf, "uni%04X.%s", sc->unicodeenc, suffix );
	alt = SFGetChar(sf,-1,namebuf);
    }
    if ( alt==NULL ) {
	sprintf( namebuf, "glyph%d.%s", sc->orig_pos, suffix );
	alt = SFGetChar(sf,-1,namebuf);
    }
    if ( alt==NULL ) {
	sprintf( namebuf, "%s.%s", sc->name, suffix );
	alt = SFGetChar(sf,-1,namebuf);
    }
return( alt );
}

static SplineChar *SuffixCheckCase(SplineChar *sc,char *suffix, int cvt2lc ) {
    SplineChar *alt = NULL;
    SplineFont *sf = sc->parent;
    char namebuf[100];

    if ( *suffix=='.' ) ++suffix;
    if ( sf->cidmaster!=NULL )
return( NULL );

    /* Small cap characters are sometimes named "a.sc" */
    /*  and sometimes "A.small" */
    /* So if I want a 'smcp' feature I must convert "a" to "A.small" */
    /* And if I want a 'c2sc' feature I must convert "A" to "a.sc" */
    if ( cvt2lc ) {
	if ( alt==NULL && sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
		isupper(sc->unicodeenc)) {
	    sprintf( namebuf, "uni%04X.%s", tolower(sc->unicodeenc), suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
	if ( alt==NULL && isupper(*sc->name)) {
	    sprintf( namebuf, "%c%s.%s", tolower(*sc->name), sc->name+1, suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
    } else {
	if ( alt==NULL && sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 &&
		islower(sc->unicodeenc)) {
	    sprintf( namebuf, "uni%04X.%s", toupper(sc->unicodeenc), suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
	if ( alt==NULL && islower(*sc->name)) {
	    sprintf( namebuf, "%c%s.%s", toupper(*sc->name), sc->name+1, suffix );
	    alt = SFGetChar(sf,-1,namebuf);
	}
    }
return( alt );
}

/* Adam Twardoch says:
    That's right. In short:

    liga: longs t -> longs_t
    dlig: s t -> s_t, c t -> c_t
    hlig: s t -> longs_t
    hist: s -> longs

    BTW, I'm also putting a e -> ae and o e -> oe into hlig (same for 
    uppercase). While ae and oe are letters used Danish, French or 
    Norwegian, they were used as ligatures in Latin, English and many other 
    languages (including Polish). Therefore, I believe it's quite reasonable
John Hudson replies
    No, they're a perfect example of a discretionary ligature. :)
Adam:
    They are both
*/
static PST *LigDefaultList(SplineChar *sc, uint32 tag) {
    /* This fills in default ligatures as the name suggests */
    /* it also builds up various other default gpos/gsub tables */
    char *components;
    PST *lig, *last=NULL;
    int i, alt_index;
    SplineChar *alt;
    SplineFont *sf = sc->parent;
    const unichar_t *variant;
    static uint32 form_tags[] = { CHR('i','n','i','t'), CHR('m','e','d','i'), CHR('f','i','n','a'), CHR('i','s','o','l'), 0 };
    DBounds bb;

    if ( tag==0 || tag==0xffffffff || tag == LigTagFromUnicode(sc->unicodeenc) ) {
	for ( alt_index = 0; ; ++alt_index ) {
	    components = LigDefaultStr(sc->unicodeenc,sc->name,alt_index);
	    if ( components==NULL )
	break;
	    lig = chunkalloc(sizeof(PST));
	    lig->tag = LigTagFromUnicode(sc->unicodeenc);
	    lig->flags = PSTDefaultFlags(pst_ligature,sc);
	    lig->script_lang_index = SFFindBiggestScriptLangIndex(sc->parent,
			SCScriptFromUnicode(sc),DEFAULT_LANG);
	    lig->type = pst_ligature;
	    lig->next = last;
	    last = lig;
	    lig->u.lig.lig = sc;
	    lig->u.lig.components = components;
#if 0
	    if ( lig->tag==CHR('r','l','i','g') ) {
		lig2 = chunkalloc(sizeof(PST));
		*lig2 = *lig;
		lig2->tag = CHR('l','i','g','a');
		lig2->next = last;
		last = lig2;
	    }
#endif
	}
    }

    if ( tag==((27<<16)|1) && sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 ) {
	const unichar_t *alt=NULL;
	int uni = sc->unicodeenc;
	if ( isdecompositionnormative(uni) &&
		    unicode_alternates[uni>>8]!=NULL &&
		(alt = unicode_alternates[uni>>8][uni&0xff])!=NULL ) {
	    if ( iscombining(alt[1]) && ( alt[2]=='\0' || iscombining(alt[2]))) {
		components = LigDefaultStr(sc->unicodeenc,sc->name,-10);
		if ( components!=NULL ) {
		    lig = chunkalloc(sizeof(PST));
		    lig->tag = tag;
		    lig->macfeature = true;
		    lig->flags = PSTDefaultFlags(pst_ligature,sc);
		    lig->script_lang_index = SFFindBiggestScriptLangIndex(sc->parent,
				SCScriptFromUnicode(sc),DEFAULT_LANG);
		    lig->type = pst_ligature;
		    lig->next = last;
		    last = lig;
		    lig->u.lig.lig = sc;
		    lig->u.lig.components = components;
		}
	    }
	}
    }

	/* Look for left to right mirrored characters */
    if ( tag==0 || tag==CHR('r','t','l','a') ) {
	if ( sc->unicodeenc!=-1 && sc->unicodeenc<0x10000 && tomirror(sc->unicodeenc)!=0 ) {
	    alt = SFGetChar(sf,tomirror(sc->unicodeenc),NULL);
	    if ( alt!=NULL )
		last=AddSubs(last,CHR('r','t','l','a'),alt->name, 0x0,SLI_UNKNOWN,sc);
	}
    }

	/* Look for vertically rotated text */
    if ( tag==0 || tag==CHR('v','r','t','2') ) {
	alt = SuffixCheck(sc,"vert");
	if ( alt!=NULL )
	    last=AddSubs(last,CHR('v','r','t','2'),alt->name, 0x0,SLI_UNKNOWN,sc);
    }

	/* Look for small caps (Lower Case -> Small Caps)*/
    if ( tag==0 || tag==CHR('s','m','c','p') ) {
	alt = SuffixCheck(sc,"sc");
	if ( alt==NULL )
	    alt = SuffixCheckCase(sc,"small",false);
#if 0		/* Adobe says oldstyles can be included in smallcaps */
	if ( alt==NULL )
	    alt = SuffixCheck(sc,"oldstyle");
#endif
	if ( alt!=NULL )
	    last=AddSubs(last,CHR('s','m','c','p'),alt->name, 0x0,SLI_UNKNOWN,sc);
    }

	/* Look for small caps (UC->Small Caps) */
    if ( tag==0 || tag==CHR('c','2','s','c') ) {
	alt = SuffixCheck(sc,"small");
	if ( alt==NULL )
	    alt = SuffixCheckCase(sc,"sc",true);
	if ( alt!=NULL )
	    last=AddSubs(last,CHR('c','2','s','c'),alt->name, 0x0,SLI_UNKNOWN,sc);
    }

	/* And for oldstyle */
    if ( tag==0 || tag==CHR('o','n','u','m') ) {
	alt = SuffixCheck(sc,"oldstyle");
	if ( alt!=NULL )
	    last=AddSubs(last,CHR('o','n','u','m'),alt->name, 0x0,SLI_UNKNOWN,sc);
    }

	/* Look for superscripts */
    if ( tag==0 || tag==CHR('s','u','p','s') ) {
	alt = SuffixCheck(sc,"superior");
	if ( alt==NULL ) {
	    for ( i=0x2070; i<0x2080; ++i ) {
		if ( unicode_alternates[i>>8]!=NULL &&
			(variant = unicode_alternates[i>>8][i&0xff])!=NULL && variant[1]=='\0' &&
			*variant == sc->unicodeenc )
	    break;
	    }
	    if ( i==0x2080 ) {
		if ( sc->unicodeenc=='1' ) i = 0xb9;
		else if ( sc->unicodeenc=='2' ) i = 0xb2;
		else if ( sc->unicodeenc=='3' ) i = 0xb3;
	    }
	    if ( i!=0x2080 )
		alt = SFGetChar(sf,i,NULL);
	}
	if ( alt!=NULL )
	    last=AddSubs(last,CHR('s','u','p','s'),alt->name, 0x0,SLI_UNKNOWN,sc);
    }

	/* Look for subscripts */
    if ( tag==0 || tag==CHR('s','u','b','s') ) {
	alt = SuffixCheck(sc,"inferior");
	if ( alt==NULL ) {
	    for ( i=0x2080; i<0x2090; ++i ) {
		if ( unicode_alternates[i>>8]!=NULL &&
			(variant = unicode_alternates[i>>8][i&0xff])!=NULL && variant[1]=='\0' &&
			*variant == sc->unicodeenc )
	    break;
	    }
	    if ( i!=0x2090 )
		alt = SFGetChar(sf,i,NULL);
	}
	if ( alt!=NULL )
	    last=AddSubs(last,CHR('s','u','b','s'),alt->name, 0x0,SLI_UNKNOWN,sc);
    }

	/* Look for swash forms */
    if ( tag==0 || tag==CHR('s','w','s','h')) {
	alt = SuffixCheck(sc,"swash");
	if ( alt!=NULL ) {
	    last=AddSubs(last,CHR('s','w','s','h'),alt->name, 0x0,SLI_UNKNOWN,sc);
	    last->type = pst_alternate;
	}
    }

    if ( tag==0 || tag==CHR('p','w','i','d') ) {
	if (( sc->unicodeenc>=0xff01 && sc->unicodeenc<=0xff5e ) ||
		( sc->unicodeenc>=0xffe0 && sc->unicodeenc<0xffe6)) {
	    /* these are full width latin */
	    if ( unicode_alternates[sc->unicodeenc>>8]!=NULL &&
		    (variant = unicode_alternates[sc->unicodeenc>>8][sc->unicodeenc&0xff])!=NULL &&
		    variant[1]=='\0' ) {
		alt = SFGetChar(sf,variant[0],NULL);
		if ( alt!=NULL )
		    last=AddSubs(last,CHR('p','w','i','d'),alt->name, 0x0,SLI_UNKNOWN,sc);
	    }
	}
    }

    if ( tag==0 || tag==CHR('f','w','i','d') ) {
	alt = NULL;
	if ( sc->unicodeenc>=0xff61 && sc->unicodeenc<0xffdc ) {
	    /* These are halfwidth katakana and sung */
	    if ( unicode_alternates[sc->unicodeenc>>8]!=NULL &&
		    (variant = unicode_alternates[sc->unicodeenc>>8][sc->unicodeenc&0xff])!=NULL &&
		    variant[1]=='\0' ) {
		alt = SFGetChar(sf,variant[0],NULL);
		if ( alt!=NULL )
		    last=AddSubs(last,CHR('f','w','i','d'),alt->name, 0x0,SLI_UNKNOWN,sc);
	    }
	} else if ( sc->unicodeenc>=0x0021 && sc->unicodeenc<=0x100 ) {
	    for ( i=0xff01; i<0xffef; ++i ) {
		if ( unicode_alternates[i>>8]!=NULL &&
			(variant = unicode_alternates[i>>8][i&0xff])!=NULL &&
			variant[1]=='\0' && variant[0]==sc->unicodeenc )
	    break;
	    }
	    if ( i<0xffef ) {
		alt = SFGetChar(sf,i,NULL);
		if ( alt!=NULL )
		    last=AddSubs(last,CHR('f','w','i','d'),alt->name, 0x0,SLI_UNKNOWN,sc);
	    }
	}
	if ( alt==NULL ) {
	    alt = SuffixCheck(sc,"full");
	    if ( alt!=NULL )
		last=AddSubs(last,CHR('f','w','i','d'),alt->name, 0x0,SLI_UNKNOWN,sc);
	}
    }

    if ( tag==0 || tag==CHR('h','w','i','d') ) {
	alt = NULL;
	if ( sc->unicodeenc>=0x3000 && sc->unicodeenc<=0x31ff ) {
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
		    last=AddSubs(last,CHR('h','w','i','d'),alt->name, 0x0,SLI_UNKNOWN,sc);
	    }
	}
	if ( alt==NULL ) {
	    alt = SuffixCheck(sc,"hw");
	    if ( alt!=NULL )
		last=AddSubs(last,CHR('h','w','i','d'),alt->name, 0x0,SLI_UNKNOWN,sc);
	}
    }

    if ( sc->unicodeenc>=0x600 && sc->unicodeenc<0x700 ) {
	/* Arabic forms */
	for ( i=0; form_tags[i]!=0; ++i ) if ( tag==0 || form_tags[i]==tag ) {
	    if ( (&(ArabicForms[sc->unicodeenc-0x600].initial))[i]!=0 &&
		    (&(ArabicForms[sc->unicodeenc-0x600].initial))[i]!=sc->unicodeenc &&
		    (alt = SFGetChar(sf,(&(ArabicForms[sc->unicodeenc-0x600].initial))[i],NULL))!=NULL )
		last=AddSubs(last,form_tags[i],alt->name,0,SLI_UNKNOWN,sc);
	}
#if 0		/* Silvan Toledo tells me that Hebrew doesn't need (and shouldn't have) this tag */
    } else if ( sc->unicodeenc>=0x5db && sc->unicodeenc<=0x5e6 &&
	    (tag==0 || tag==CHR('f','i','n','a')) ) {
	/* Hebrew finals */
	alt = NULL;
	if ( sc->unicodeenc==0x5db )
	    alt = SFGetChar(sf,0x5da,NULL);
	else if ( sc->unicodeenc==0x5de )
	    alt = SFGetChar(sf,0x5dd,NULL);
	else if ( sc->unicodeenc==0x5e0 )
	    alt = SFGetChar(sf,0x5df,NULL);
	else if ( sc->unicodeenc==0x5e4 )
	    alt = SFGetChar(sf,0x5e3,NULL);
	else if ( sc->unicodeenc==0x5e6 )
	    alt = SFGetChar(sf,0x5e5,NULL);
	if ( alt!=NULL )
	    last=AddSubs(last,CHR('f','i','n','a'),alt->name,0,SLI_UNKNOWN,sc);
#endif
    } else if ( sc->unicodeenc>=0x3c3 &&
	    (tag==0 || tag==CHR('f','i','n','a')) ) {
	/* Greek final sigma */
	alt = SFGetChar(sf,0x3c2,NULL);
	if ( alt!=NULL )
	    last=AddSubs(last,CHR('f','i','n','a'),alt->name,0,SLI_UNKNOWN,sc);
/* I'd really like to add an entry for long-s (initial & medial) but it would */
/*  confuse most people. There's no historical initial entry, and although the*/
/*  open type docs suggest long-s as an example of the 'hist' tag, the */
/*  semantics are incorrect (ie. it would change final short-s to long-s and */
/*  that's not valid) so... we do nothing for latin here. */
    }

    if ( tag==0 || tag==CHR('l','f','b','d') ) {
	SplineCharFindBounds(sc,&bb);
	last = AddPos(last,CHR('l','f','b','d'),-bb.minx,0,-bb.minx,0, 0x0, sc);
    }

    if ( tag==0 || tag==CHR('r','t','b','d') ) {
	SplineCharFindBounds(sc,&bb);
	last = AddPos(last,CHR('r','t','b','d'),0,0,bb.maxx-sc->width,0, 0x0, sc);
    }
return( last );
}

static void SCMergePSList(SplineChar *sc,PST *list) {
    PST *test, *next, *prev;

    for ( ; list!=NULL; list=next ) {
	next = list->next;
	prev = NULL;
	for ( test=sc->possub; test!=NULL ; prev=test, test=test->next ) {
	    if ( test->tag==list->tag && (test->type!=pst_ligature ||
		    strcmp(test->u.lig.components,list->u.lig.components)==0 ))
	break;
	}
	if ( test!=NULL ) {
	    if ( prev==NULL )
		sc->possub = list;
	    else
		prev->next = list;
	    list->next = test->next;
	    test->next = NULL;
	    PSTFree(test);
	} else {
	    list->next = sc->possub;
	    sc->possub = list;
	}
    }
}

void SCLigDefault(SplineChar *sc) {
    PST *pst, *prev, *n;

    /* Free any ligatures */
    for ( prev=NULL, pst = sc->possub; pst!=NULL; pst = n ) {
	n = pst->next;
	if ( pst->type == pst_ligature ) {
	    if ( prev==NULL )
		sc->possub = n;
	    else
		prev->next = n;
	    pst->next = NULL;
	    PSTFree(pst);
	} else
	    prev = pst;
    }

    if ( LigTagFromUnicode(sc->unicodeenc)!=((27<<16)|1) ) {
	pst = LigDefaultList(sc,0xffffffff);
	if ( pst!=NULL ) {
	    for ( n=pst; n->next!=NULL ; n=n->next );
	    n->next = sc->possub;
	    sc->possub = pst;
	}
    }
}

void SCTagDefault(SplineChar *sc,uint32 tag) {

    SCMergePSList(sc,LigDefaultList(sc,tag));
}

void SCSuffixDefault(SplineChar *sc,uint32 tag,char *suffix,uint16 flags,uint16 sli) {
    SplineChar *alt;

    alt = SuffixCheck(sc,suffix);
    if ( alt!=NULL )
	SCMergePSList(sc,AddSubs(NULL,tag,alt->name,flags,sli,sc));
}

void SCLigCaretCheck(SplineChar *sc,int clean) {
    PST *pst, *carets=NULL, *prev_carets=NULL, *prev;
    int lig_comp_max=0, lc, i;
    char *pt;
    /* Check to see if this is a ligature character, and if so, does it have */
    /*  a ligature caret structure. If a lig but no lig caret structure then */
    /*  create a lig caret struct */

    for ( pst=sc->possub, prev=NULL; pst!=NULL; prev = pst, pst=pst->next ) {
	if ( pst->type == pst_lcaret ) {
	    if ( carets!=NULL )
		IError("Too many ligature caret structures" );
	    else {
		carets = pst;
		prev_carets = prev;
	    }
	} else if ( pst->type==pst_ligature ) {
	    for ( lc=0, pt=pst->u.lig.components; *pt; ++pt )
		if ( *pt==' ' ) ++lc;
	    if ( lc>lig_comp_max )
		lig_comp_max = lc;
	}
    }
    if ( lig_comp_max == 0 ) {
	if ( clean && carets!=NULL ) {
	    if ( prev_carets==NULL )
		sc->possub = carets->next;
	    else
		prev_carets->next = carets->next;
	    carets->next = NULL;
	    PSTFree(carets);
	}
return;
    }
    if ( carets==NULL ) {
	carets = chunkalloc(sizeof(PST));
	carets->type = pst_lcaret;
	carets->script_lang_index = -1;		/* Not really relevant here */
	carets->next = sc->possub;
	sc->possub = carets;
    }
    if ( carets->u.lcaret.cnt>=lig_comp_max ) {
	carets->u.lcaret.cnt = lig_comp_max;
return;
    }
    if ( carets->u.lcaret.carets==NULL )
	carets->u.lcaret.carets = (int16 *) gcalloc(lig_comp_max,sizeof(int16));
    else {
	carets->u.lcaret.carets = (int16 *) grealloc(carets->u.lcaret.carets,lig_comp_max*sizeof(int16));
	for ( i=carets->u.lcaret.cnt; i<lig_comp_max; ++i )
	    carets->u.lcaret.carets[i] = 0;
    }
    carets->u.lcaret.cnt = lig_comp_max;
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static int CI_SName(GGadget *g, GEvent *e) {	/* Set From Name */
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	const unichar_t *ret = _GGadgetGetTitle(GWidgetGetControl(ci->gw,CID_UName));
	int i;
	char buf[40], *ctemp; unichar_t ubuf[2], *temp;
	ctemp = u2utf8_copy(ret);
	i = UniFromName(ctemp,ui_none,&custom);
	free(ctemp);
	if ( i==-1 ) {
	    /* Adobe says names like uni00410042 represent a ligature (A&B) */
	    /*  (that is "uni" followed by two (or more) 4-digit codes). */
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

	sprintf(buf,"U+%04x", i);
	temp = uc_copy(i==-1?"-1":buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);

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

	ubuf[0] = val;
	if ( val==-1 )
	    ubuf[0] = '\0';
	ubuf[1] = '\0';
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
    }
return( true );
}

GTextInfo *TIFromName(const char *name) {
    GTextInfo *ti = gcalloc(1,sizeof(GTextInfo));
    ti->text = utf82u_copy(name);
    ti->fg = COLOR_DEFAULT;
    ti->bg = COLOR_DEFAULT;
return( ti );
}

static void CI_SetNameList(CharInfo *ci,int val) {
    GGadget *g = GWidgetGetControl(ci->gw,CID_UName);
    int cnt;

    if ( GGadgetGetUserData(g)==(void *) (intpt) val )
return;		/* Didn't change */
    {
	GTextInfo **list = NULL;
	char **names = AllGlyphNames(val,ci->sc->parent->for_new_glyphs,ci->sc);

	for ( cnt=0; names[cnt]!=NULL; ++cnt );
	list = galloc((cnt+1)*sizeof(GTextInfo*)); 
	for ( cnt=0; names[cnt]!=NULL; ++cnt ) {
	    list[cnt] = TIFromName(names[cnt]);
	    free(names[cnt]);
	}
	free(names);
	list[cnt] = TIFromName(NULL);
	GGadgetSetList(g,list,true);
    }
    GGadgetSetUserData(g,(void *) (intpt) val);
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
	unichar_t *temp, ubuf[2]; char buf[10];

	if ( ret[0]=='\0' )
return( true );
	else if ( ret[1]!='\0' ) {
	    gwwv_post_notice(_("Only a single character allowed"),_("Only a single character allowed"));
	    ubuf[0] = ret[0];
	    ubuf[1] = '\0';
	    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);
return( true );
	}

	SetNameFromUnicode(ci->gw,CID_UName,val);
	CI_SetNameList(ci,val);

	sprintf(buf,"U+%04x", val);
	temp = uc_copy(buf);
	GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
	free(temp);
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

char *PST2Text(PST *pst,SplineFont *sf) {
    char buffer[400];

    if ( pst->type==pst_position || pst->type==pst_pair ) {
	if ( pst->type==pst_position ) {
	    sprintf(buffer,"          %3d ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d",
		    pst->script_lang_index,
		    pst->u.pos.xoff, pst->u.pos.yoff,
		    pst->u.pos.h_adv_off, pst->u.pos.v_adv_off );
	} else if ( pst->type==pst_pair ) {
	    sprintf(buffer,"          %3d %s ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d | ∆x=%d ∆y=%d ∆x_adv=%d ∆y_adv=%d",
		    pst->script_lang_index,
		    pst->u.pair.paired,
		    pst->u.pair.vr[0].xoff, pst->u.pair.vr[0].yoff,
		    pst->u.pair.vr[0].h_adv_off, pst->u.pair.vr[0].v_adv_off,
		    pst->u.pair.vr[1].xoff, pst->u.pair.vr[1].yoff,
		    pst->u.pair.vr[1].h_adv_off, pst->u.pair.vr[1].v_adv_off );
	}
	buffer[0] = pst->tag>>24;
	buffer[1] = (pst->tag>>16)&0xff;
	buffer[2] = (pst->tag>>8)&0xff;
	buffer[3] = (pst->tag)&0xff;
	buffer[4] = ' ';
	buffer[5] = pst->flags&pst_r2l?'r':' ';
	buffer[6] = pst->flags&pst_ignorebaseglyphs?'b':' ';
	buffer[7] = pst->flags&pst_ignoreligatures?'l':' ';
	buffer[8] = pst->flags&pst_ignorecombiningmarks?'m':' ';
	buffer[9] = ' ';
return( copy( buffer ));
    } else {
	unichar_t *uret  = ClassName(pst->u.subs.variant,pst->tag,pst->flags,
		pst->script_lang_index,-1,-1,pst->macfeature,sf);
	char *ret = u2utf8_copy(uret);
	free(uret);
return( ret );
    }
}

char *Kern2Text(SplineChar *other,KernPair *kp,int isv) {
    char buffer[400];

    sprintf(buffer,"%s %c%c%c%c %3d %s offset=%d",
	    isv ? "vkrn" : "kern",
	    kp->flags&pst_r2l?'r':' ',
	    kp->flags&pst_ignorebaseglyphs?'b':' ',
	    kp->flags&pst_ignoreligatures?'l':' ',
	    kp->flags&pst_ignorecombiningmarks?'m':' ',
	    kp->sli,
	    other->name,
	    kp->off );
return( copy( buffer ));
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void CIFillup(CharInfo *ci) {
    SplineChar *sc = ci->sc;
    SplineFont *sf = sc->parent;
    unichar_t *temp;
    char buffer[400];
    char buf[200];
    const unichar_t *bits;
    int i,j,gid;
    GTextInfo **arrays[pst_max], **ti;
    int cnts[pst_max];
    PST *pst;
    unichar_t ubuf[4];

    sprintf(buf,_("Glyph Info for %.40s"),sc->name);
    GDrawSetWindowTitles8(ci->gw, buf, _("Glyph Info..."));

    if ( ci->oldsc!=NULL && ci->oldsc->charinfo==ci )
	ci->oldsc->charinfo = NULL;
    sc->charinfo = ci;
    ci->oldsc = sc;

    CI_CanPaste(ci);

    GGadgetSetEnabled(GWidgetGetControl(ci->gw,-1), ci->enc>0 &&
	    ((gid=ci->map->map[ci->enc-1])==-1 ||
	     sf->glyphs[gid]==NULL || sf->glyphs[gid]->charinfo==NULL ||
	     gid==sc->orig_pos));
    GGadgetSetEnabled(GWidgetGetControl(ci->gw,1), ci->enc<ci->map->enccount-1 &&
	    ((gid=ci->map->map[ci->enc+1])==-1 ||
	     sf->glyphs[gid]==NULL || sf->glyphs[gid]->charinfo==NULL ||
	     gid==sc->orig_pos));

    temp = utf82u_copy(sc->name);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UName),temp);
    free(temp);
    CI_SetNameList(ci,sc->unicodeenc);

    sprintf(buffer,"U+%04x", sc->unicodeenc);
    temp = utf82u_copy(sc->unicodeenc==-1?"-1":buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UValue),temp);
    free(temp);

    ubuf[0] = sc->unicodeenc;
    if ( sc->unicodeenc==-1 )
	ubuf[0] = '\0';
    ubuf[1] = '\0';
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_UChar),ubuf);

    memset(cnts,0,sizeof(cnts));
    for ( pst = sc->possub; pst!=NULL; pst=pst->next )
	++cnts[pst->type];
    for ( i=pst_null+1; i<pst_max; ++i )
	arrays[i] = gcalloc((cnts[i]+1),sizeof(GTextInfo *));
    memset(cnts,0,sizeof(cnts));
    for ( pst = sc->possub; pst!=NULL; pst=pst->next ) {
	char *t;
	j = cnts[pst->type]++;
	arrays[pst->type][j] = gcalloc(1,sizeof(GTextInfo));
	arrays[pst->type][j]->text = utf82u_copy(t= PST2Text(pst,ci->sc->parent));
	free(t);
	arrays[pst->type][j]->fg = arrays[pst->type][j]->bg = COLOR_DEFAULT;
    }
    for ( i=pst_null+1; i<pst_lcaret /* == pst_max-1 */; ++i ) {
	arrays[i][cnts[i]] = gcalloc(1,sizeof(GTextInfo));
	GGadgetSetList(GWidgetGetControl(ci->gw,CID_List+(i-1)*100),
		arrays[i],false);
    }

    bits = SFGetAlternate(sc->parent,sc->unicodeenc,sc,true);
    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_ComponentMsg),
	bits==NULL ? _("No components") :
	hascomposing(sc->parent,sc->unicodeenc,sc) ? _("Accented glyph composed of:") :
	    _("Glyph composed of:"));
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

    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),0);

    GGadgetSetTitle8(GWidgetGetControl(ci->gw,CID_Comment),
	    sc->comment?sc->comment:"");
    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_GClass),sc->glyph_class);
    for ( i=0; std_colors[i].image!=NULL; ++i ) {
	if ( std_colors[i].userdata == (void *) (intpt) sc->color )
	    GGadgetSelectOneListItem(GWidgetGetControl(ci->gw,CID_Color),i);
    }
    ci->first = sc->comment==NULL;

    ti = galloc((sc->countermask_cnt+1)*sizeof(GTextInfo *));
    ti[sc->countermask_cnt] = gcalloc(1,sizeof(GTextInfo));
    for ( i=0; i<sc->countermask_cnt; ++i ) {
	ti[i] = gcalloc(1,sizeof(GTextInfo));
	ti[i]->text = CounterMaskLine(sc,&sc->countermasks[i]);
	ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
	ti[i]->userdata = chunkalloc(sizeof(HintMask));
	memcpy(ti[i]->userdata,sc->countermasks[i],sizeof(HintMask));
    }
    GGadgetSetList(GWidgetGetControl(ci->gw,CID_List+600),ti,false);

    if ( sc->tex_height!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->tex_height);
    else
	buffer[0] = '\0';
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_TeX_Height),ubuf);

    if ( sc->tex_depth!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->tex_depth);
    else
	buffer[0] = '\0';
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_TeX_Depth),ubuf);

    if ( sc->tex_sub_pos!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->tex_sub_pos);
    else
	buffer[0] = '\0';
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_TeX_Sub),ubuf);

    if ( sc->tex_super_pos!=TEX_UNDEF )
	sprintf(buffer,"%d",sc->tex_super_pos);
    else
	buffer[0] = '\0';
    uc_strcpy(ubuf,buffer);
    GGadgetSetTitle(GWidgetGetControl(ci->gw,CID_TeX_Super),ubuf);
}

static int CI_NextPrev(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	int enc = ci->enc + GGadgetGetCid(g);	/* cid is 1 for next, -1 for prev */
	SplineChar *new;

	if ( enc<0 || enc>=ci->map->enccount ) {
	    GGadgetSetEnabled(g,false);
return( true );
	}
	if ( !_CI_OK(ci))
return( true );
	new = SFMakeChar(ci->sc->parent,ci->map,enc);
	if ( new->charinfo!=NULL && new->charinfo!=ci ) {
	    GGadgetSetEnabled(g,false);
return( true );
	}
	ci->sc = new;
	ci->enc = enc;
	CIFillup(ci);
    }
return( true );
}

static void CI_DoCancel(CharInfo *ci) {
    int32 i,len;
    GTextInfo **ti = GGadgetGetList(GWidgetGetControl(ci->gw,CID_List+600),&len);

    for ( i=0; i<len; ++i )
	chunkfree(ti[i]->userdata,sizeof(HintMask));
    CI_Finish(ci);
}

static int CI_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	CharInfo *ci = GDrawGetUserData(GGadgetGetWindow(g));
	CI_DoCancel(ci);
    }
return( true );
}

static int ci_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	CharInfo *ci = GDrawGetUserData(gw);
	CI_DoCancel(ci);
    } else if ( event->type==et_char ) {
	CharInfo *ci = GDrawGetUserData(gw);
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("charinfo.html");
return( true );
	} else if ( event->u.chr.keysym=='c' && (event->u.chr.state&ksm_control)) {
	    CI_DoCopy(ci);
return( true );
	} else if ( event->u.chr.keysym=='v' && (event->u.chr.state&ksm_control)) {
	    CI_DoPaste(ci,NULL,pst_null,NULL);
return( true );
	} else if ( event->u.chr.keysym=='q' && (event->u.chr.state&ksm_control)) {
	    if ( event->u.chr.state&ksm_shift )
		CI_DoCancel(ci);
	    else
		MenuExit(NULL,NULL,NULL);
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

void SCCharInfo(SplineChar *sc,EncMap *map,int enc) {
    CharInfo *ci;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData ugcd[12], cgcd[6], psgcd[7][7], cogcd[3], mgcd[9], tgcd[10];
    GTextInfo ulabel[12], clabel[6], pslabel[7][6], colabel[3], mlabel[9], tlabel[10];
    GGadgetCreateData mbox[4], *mvarray[7], *mharray1[7], *mharray2[8];
    GGadgetCreateData ubox[3], *uhvarray[19], *uharray[6];
    GGadgetCreateData cbox[3], *cvarray[5], *charray[4];
    GGadgetCreateData pstbox[7][4], *pstvarray[7][5], *pstharray1[7][8], *pstharray2[7][7];
    GGadgetCreateData cobox[2], *covarray[4];
    GGadgetCreateData tbox[2], *thvarray[16];
    int i;
    GTabInfo aspects[13];
    static GBox smallbox = { bt_raised, bs_rect, 2, 1, 0, 0, 0,0,0,0, COLOR_DEFAULT,COLOR_DEFAULT };
    static int boxset=0;
    FontRequest rq;
    GFont *font;

    CharInfoInit();

    if ( sc->charinfo!=NULL ) {
	GDrawSetVisible(sc->charinfo->gw,true);
	GDrawRaise(sc->charinfo->gw);
return;
    }

    ci = gcalloc(1,sizeof(CharInfo));
    ci->sc = sc;
    ci->done = false;
    ci->map = map;
    if ( enc==-1 )
	enc = map->backmap[sc->orig_pos];
    ci->enc = enc;

    if ( !boxset ) {
	extern GBox _ggadget_Default_Box;
	extern void GGadgetInit(void);
	GGadgetInit();
	smallbox = _ggadget_Default_Box;
	smallbox.padding = 1;
	boxset = 1;
    }

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = false;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Glyph Info...");
	wattrs.is_dlg = false;
	pos.x = pos.y = 0;
#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,CI_Width));
#else
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,CI_Width+65));
#endif
	pos.height = GDrawPointsToPixels(NULL,CI_Height);
	ci->gw = GDrawCreateTopWindow(NULL,&pos,ci_e_h,ci,&wattrs);

	memset(&ugcd,0,sizeof(ugcd));
	memset(&ubox,0,sizeof(ubox));
	memset(&ulabel,0,sizeof(ulabel));

	ulabel[0].text = (unichar_t *) _("U_nicode Name:");
	ulabel[0].text_is_1byte = true;
	ulabel[0].text_in_resource = true;
	ugcd[0].gd.label = &ulabel[0];
	ugcd[0].gd.pos.x = 5; ugcd[0].gd.pos.y = 5+4; 
	ugcd[0].gd.flags = gg_enabled|gg_visible;
	ugcd[0].gd.mnemonic = 'N';
	ugcd[0].creator = GLabelCreate;
	uhvarray[0] = &ugcd[0];

	ugcd[1].gd.pos.x = 85; ugcd[1].gd.pos.y = 5;
	ugcd[1].gd.flags = gg_enabled|gg_visible;
	ugcd[1].gd.mnemonic = 'N';
	ugcd[1].gd.cid = CID_UName;
	ugcd[1].creator = GListFieldCreate;
	ugcd[1].data = (void *) (-2);
	uhvarray[1] = &ugcd[1]; uhvarray[2] = NULL;

	ulabel[2].text = (unichar_t *) _("Unicode _Value:");
	ulabel[2].text_in_resource = true;
	ulabel[2].text_is_1byte = true;
	ugcd[2].gd.label = &ulabel[2];
	ugcd[2].gd.pos.x = 5; ugcd[2].gd.pos.y = 31+4; 
	ugcd[2].gd.flags = gg_enabled|gg_visible;
	ugcd[2].gd.mnemonic = 'V';
	ugcd[2].creator = GLabelCreate;
	uhvarray[3] = &ugcd[2];

	ugcd[3].gd.pos.x = 85; ugcd[3].gd.pos.y = 31;
	ugcd[3].gd.flags = gg_enabled|gg_visible;
	ugcd[3].gd.mnemonic = 'V';
	ugcd[3].gd.cid = CID_UValue;
	ugcd[3].gd.handle_controlevent = CI_UValChanged;
	ugcd[3].creator = GTextFieldCreate;
	uhvarray[4] = &ugcd[3]; uhvarray[5] = NULL;

	ulabel[4].text = (unichar_t *) _("Unicode C_har:");
	ulabel[4].text_in_resource = true;
	ulabel[4].text_is_1byte = true;
	ugcd[4].gd.label = &ulabel[4];
	ugcd[4].gd.pos.x = 5; ugcd[4].gd.pos.y = 57+4; 
	ugcd[4].gd.flags = gg_enabled|gg_visible;
	ugcd[4].gd.mnemonic = 'h';
	ugcd[4].creator = GLabelCreate;
	uhvarray[6] = &ugcd[4];

	ugcd[5].gd.pos.x = 85; ugcd[5].gd.pos.y = 57;
	ugcd[5].gd.flags = gg_enabled|gg_visible|gg_text_xim;
	ugcd[5].gd.mnemonic = 'h';
	ugcd[5].gd.cid = CID_UChar;
	ugcd[5].gd.handle_controlevent = CI_CharChanged;
	ugcd[5].creator = GTextFieldCreate;
	uhvarray[7] = &ugcd[5]; uhvarray[8] = NULL;

	ugcd[6].gd.pos.x = 5; ugcd[6].gd.pos.y = 83+4;
	ugcd[6].gd.flags = gg_visible | gg_enabled;
	ulabel[6].text = (unichar_t *) _("OT _Glyph Class:");
	ulabel[6].text_is_1byte = true;
	ulabel[6].text_in_resource = true;
	ugcd[6].gd.label = &ulabel[6];
	ugcd[6].creator = GLabelCreate;
	uhvarray[9] = &ugcd[6];

	ugcd[7].gd.pos.x = 85; ugcd[7].gd.pos.y = 83;
	ugcd[7].gd.flags = gg_visible | gg_enabled;
	ugcd[7].gd.cid = CID_GClass;
	ugcd[7].gd.u.list = glyphclasses;
	ugcd[7].creator = GListButtonCreate;
	uhvarray[10] = &ugcd[7]; uhvarray[11] = NULL;

	ugcd[8].gd.pos.x = 12; ugcd[8].gd.pos.y = 117;
	ugcd[8].gd.flags = gg_visible | gg_enabled;
	ulabel[8].text = (unichar_t *) _("Set From N_ame");
	ulabel[8].text_is_1byte = true;
	ulabel[8].text_in_resource = true;
	ugcd[8].gd.mnemonic = 'a';
	ugcd[8].gd.label = &ulabel[8];
	ugcd[8].gd.handle_controlevent = CI_SName;
	ugcd[8].creator = GButtonCreate;
	uharray[0] = GCD_Glue; uharray[1] = &ugcd[8];

	ugcd[9].gd.pos.x = 107; ugcd[9].gd.pos.y = 117;
	ugcd[9].gd.flags = gg_visible | gg_enabled;
	ulabel[9].text = (unichar_t *) _("Set From Val_ue");
	ulabel[9].text_is_1byte = true;
	ulabel[9].text_in_resource = true;
	ugcd[9].gd.mnemonic = 'l';
	ugcd[9].gd.label = &ulabel[9];
	ugcd[9].gd.handle_controlevent = CI_SValue;
	ugcd[9].creator = GButtonCreate;
	uharray[2] = GCD_Glue; uharray[3] = &ugcd[9]; uharray[4] = GCD_Glue; uharray[5] = NULL;

	ubox[2].gd.flags = gg_enabled|gg_visible;
	ubox[2].gd.u.boxelements = uharray;
	ubox[2].creator = GHBoxCreate;
	uhvarray[12] = &ubox[2]; uhvarray[13] = GCD_ColSpan; uhvarray[14] = NULL;
	uhvarray[15] = GCD_Glue; uhvarray[16] = GCD_Glue; uhvarray[17] = NULL;
	uhvarray[18] = NULL;

	ubox[0].gd.flags = gg_enabled|gg_visible;
	ubox[0].gd.u.boxelements = uhvarray;
	ubox[0].creator = GHVBoxCreate;


	memset(&cgcd,0,sizeof(cgcd));
	memset(&cbox,0,sizeof(cbox));
	memset(&clabel,0,sizeof(clabel));

	clabel[0].text = (unichar_t *) _("Comment");
	clabel[0].text_is_1byte = true;
	cgcd[0].gd.label = &clabel[0];
	cgcd[0].gd.pos.x = 5; cgcd[0].gd.pos.y = 5; 
	cgcd[0].gd.flags = gg_enabled|gg_visible;
	cgcd[0].creator = GLabelCreate;
	cvarray[0] = &cgcd[0];

	cgcd[1].gd.pos.x = 5; cgcd[1].gd.pos.y = cgcd[0].gd.pos.y+13;
	cgcd[1].gd.pos.width = CI_Width-20;
	cgcd[1].gd.pos.height = 7*12+6;
	cgcd[1].gd.flags = gg_enabled|gg_visible|gg_textarea_wrap|gg_text_xim;
	cgcd[1].gd.cid = CID_Comment;
	cgcd[1].gd.handle_controlevent = CI_CommentChanged;
	cgcd[1].creator = GTextAreaCreate;
	cvarray[1] = &cgcd[1]; cvarray[2] = GCD_Glue;

	clabel[2].text = (unichar_t *) _("Color:");
	clabel[2].text_is_1byte = true;
	cgcd[2].gd.label = &clabel[2];
	cgcd[2].gd.pos.x = 5; cgcd[2].gd.pos.y = cgcd[1].gd.pos.y+cgcd[1].gd.pos.height+5+6; 
	cgcd[2].gd.flags = gg_enabled|gg_visible;
	cgcd[2].creator = GLabelCreate;
	charray[0] = &cgcd[2];

	cgcd[3].gd.pos.x = cgcd[3].gd.pos.x; cgcd[3].gd.pos.y = cgcd[2].gd.pos.y-6;
	cgcd[3].gd.pos.width = cgcd[3].gd.pos.width;
	cgcd[3].gd.flags = gg_enabled|gg_visible;
	cgcd[3].gd.cid = CID_Color;
	cgcd[3].gd.u.list = std_colors;
	cgcd[3].creator = GListButtonCreate;
	charray[1] = &cgcd[3]; charray[2] = GCD_Glue; charray[3] = NULL;

	cbox[2].gd.flags = gg_enabled|gg_visible;
	cbox[2].gd.u.boxelements = charray;
	cbox[2].creator = GHBoxCreate;
	cvarray[3] = &cbox[2]; cvarray[4] = NULL;

	cbox[0].gd.flags = gg_enabled|gg_visible;
	cbox[0].gd.u.boxelements = cvarray;
	cbox[0].creator = GVBoxCreate;

	memset(&psgcd,0,sizeof(psgcd));
	memset(&pstbox,0,sizeof(pstbox));
	memset(&pslabel,0,sizeof(pslabel));

	for ( i=0; i<7; ++i ) {
	    psgcd[i][0].gd.pos.x = 5; psgcd[i][0].gd.pos.y = 5;
	    psgcd[i][0].gd.pos.width = CI_Width-28; psgcd[i][0].gd.pos.height = 7*12+10;
	    psgcd[i][0].gd.flags = gg_visible | gg_enabled | gg_list_alphabetic | gg_list_multiplesel;
	    psgcd[i][0].gd.cid = CID_List+i*100;
	    psgcd[i][0].gd.handle_controlevent = CI_SelChanged;
	    psgcd[i][0].gd.box = &smallbox;
	    psgcd[i][0].creator = GListCreate;
	    pstvarray[i][0] = &psgcd[i][0];

	    psgcd[i][1].gd.pos.x = 10; psgcd[i][1].gd.pos.y = psgcd[i][0].gd.pos.y+psgcd[i][0].gd.pos.height+4;
	    psgcd[i][1].gd.pos.width = -1;
	    psgcd[i][1].gd.flags = gg_visible | gg_enabled;
	    pslabel[i][1].text = (unichar_t *) _("_New...");
	    pslabel[i][1].text_is_1byte = true;
	    pslabel[i][1].text_in_resource = true;
	    psgcd[i][1].gd.label = &pslabel[i][1];
	    psgcd[i][1].gd.cid = CID_New+i*100;
	    psgcd[i][1].gd.handle_controlevent = CI_New;
	    psgcd[i][1].gd.box = &smallbox;
	    psgcd[i][1].creator = GButtonCreate;
	    pstharray1[i][0] = GCD_Glue; pstharray1[i][1] = &psgcd[i][1];

	    psgcd[i][2].gd.pos.x = 20+GIntGetResource(_NUM_Buttonsize)*100/GIntGetResource(_NUM_ScaleFactor); psgcd[i][2].gd.pos.y = psgcd[i][1].gd.pos.y;
	    psgcd[i][2].gd.pos.width = -1;
	    psgcd[i][2].gd.flags = gg_visible;
	    pslabel[i][2].text = (unichar_t *) _("_Delete");
	    pslabel[i][2].text_is_1byte = true;
	    pslabel[i][2].text_in_resource = true;
	    psgcd[i][2].gd.label = &pslabel[i][2];
	    psgcd[i][2].gd.cid = CID_Delete+i*100;
	    psgcd[i][2].gd.handle_controlevent = CI_Delete;
	    psgcd[i][2].gd.box = &smallbox;
	    psgcd[i][2].creator = GButtonCreate;
	    pstharray1[i][2] = GCD_Glue; pstharray1[i][3] = &psgcd[i][2];

	    psgcd[i][3].gd.pos.x = -10; psgcd[i][3].gd.pos.y = psgcd[i][1].gd.pos.y;
	    psgcd[i][3].gd.pos.width = -1;
	    psgcd[i][3].gd.flags = gg_visible;
	    pslabel[i][3].text = (unichar_t *) _("_Edit...");
	    pslabel[i][3].text_is_1byte = true;
	    pslabel[i][3].text_in_resource = true;
	    psgcd[i][3].gd.label = &pslabel[i][3];
	    psgcd[i][3].gd.cid = CID_Edit+i*100;
	    psgcd[i][3].gd.handle_controlevent = CI_Edit;
	    psgcd[i][3].gd.box = &smallbox;
	    psgcd[i][3].creator = GButtonCreate;
	    pstharray1[i][4] = GCD_Glue; pstharray1[i][5] = &psgcd[i][3]; pstharray1[i][6] = GCD_Glue; pstharray1[i][7] = NULL;

	    pstbox[i][2].gd.flags = gg_enabled|gg_visible;
	    pstbox[i][2].gd.u.boxelements = pstharray1[i];
	    pstbox[i][2].creator = GHBoxCreate;
	    pstvarray[i][1] = &pstbox[i][2];

	    psgcd[i][4].gd.pos.x = 20; psgcd[i][4].gd.pos.y = psgcd[i][3].gd.pos.y+22;
	    psgcd[i][4].gd.pos.width = -1;
	    psgcd[i][4].gd.flags = gg_visible;
	    pslabel[i][4].text = (unichar_t *) _("_Copy");
	    pslabel[i][4].text_is_1byte = true;
	    pslabel[i][4].text_in_resource = true;
	    psgcd[i][4].gd.label = &pslabel[i][4];
	    psgcd[i][4].gd.cid = CID_Copy+i*100;
	    psgcd[i][4].gd.handle_controlevent = CI_Copy;
	    psgcd[i][4].gd.box = &smallbox;
	    psgcd[i][4].creator = GButtonCreate;
	    pstharray2[i][0] = GCD_Glue; pstharray2[i][1] = &psgcd[i][4]; pstharray2[i][2] = GCD_Glue;

	    psgcd[i][5].gd.pos.x = -20; psgcd[i][5].gd.pos.y = psgcd[i][4].gd.pos.y;
	    psgcd[i][5].gd.pos.width = -1;
	    psgcd[i][5].gd.flags = gg_visible;
	    pslabel[i][5].text = (unichar_t *) _("_Paste");
	    pslabel[i][5].text_is_1byte = true;
	    pslabel[i][5].text_in_resource = true;
	    psgcd[i][5].gd.label = &pslabel[i][5];
	    psgcd[i][5].gd.cid = CID_Paste+i*100;
	    psgcd[i][5].gd.handle_controlevent = CI_Paste;
	    psgcd[i][5].gd.box = &smallbox;
	    psgcd[i][5].creator = GButtonCreate;
	    pstharray2[i][3] = GCD_Glue; pstharray2[i][4] = &psgcd[i][5]; pstharray2[i][5] = GCD_Glue; pstharray2[i][6] = NULL;

	    pstbox[i][3].gd.flags = gg_enabled|gg_visible;
	    pstbox[i][3].gd.u.boxelements = pstharray2[i];
	    pstbox[i][3].creator = GHBoxCreate;
	    pstvarray[i][2] = &pstbox[i][3]; pstvarray[i][3] = NULL;

	    pstbox[i][0].gd.flags = gg_enabled|gg_visible;
	    pstbox[i][0].gd.u.boxelements = pstvarray[i];
	    pstbox[i][0].creator = GVBoxCreate;
	}
	psgcd[6][4].gd.flags = psgcd[6][5].gd.flags = 0;	/* No copy, paste for hint masks */

	memset(&cogcd,0,sizeof(cogcd));
	memset(&cobox,0,sizeof(cobox));
	memset(&colabel,0,sizeof(colabel));

	colabel[0].text = (unichar_t *) _("Accented glyph composed of:");
	colabel[0].text_is_1byte = true;
	cogcd[0].gd.label = &colabel[0];
	cogcd[0].gd.pos.x = 5; cogcd[0].gd.pos.y = 5; 
	cogcd[0].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	cogcd[0].gd.cid = CID_ComponentMsg;
	cogcd[0].creator = GLabelCreate;

	cogcd[1].gd.pos.x = 5; cogcd[1].gd.pos.y = cogcd[0].gd.pos.y+12;
	cogcd[1].gd.pos.width = CI_Width-20;
	cogcd[1].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	cogcd[1].gd.cid = CID_Components;
	cogcd[1].creator = GLabelCreate;

	covarray[0] = &cogcd[0]; covarray[1] = &cogcd[1]; covarray[2] = GCD_Glue; covarray[3] = NULL;
	cobox[0].gd.flags = gg_enabled|gg_visible;
	cobox[0].gd.u.boxelements = covarray;
	cobox[0].creator = GVBoxCreate;
	


	memset(&tgcd,0,sizeof(tgcd));
	memset(&tbox,0,sizeof(tbox));
	memset(&tlabel,0,sizeof(tlabel));

	tlabel[0].text = (unichar_t *) _("Height:");
	tlabel[0].text_is_1byte = true;
	tgcd[0].gd.label = &tlabel[0];
	tgcd[0].gd.pos.x = 5; tgcd[0].gd.pos.y = 5+4; 
	tgcd[0].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[0].gd.popup_msg = (unichar_t *) _("These fields are the metrics fields used by TeX\nThe height and depth are pretty self-explanatory,\nexcept that they are corrected for optical distortion.\nSo 'x' and 'o' probably have the same height.\nSubscript and Superscript positions\nare only used in math fonts and should be left blank elsewhere");
	tgcd[0].creator = GLabelCreate;
	thvarray[0] = &tgcd[0];

	tgcd[1].gd.pos.x = 85; tgcd[1].gd.pos.y = 5;
	tgcd[1].gd.flags = gg_enabled|gg_visible;
	tgcd[1].gd.cid = CID_TeX_Height;
	tgcd[1].creator = GTextFieldCreate;
	thvarray[1] = &tgcd[1]; thvarray[2] = NULL;

	tlabel[2].text = (unichar_t *) _("Depth:");
	tlabel[2].text_is_1byte = true;
	tgcd[2].gd.label = &tlabel[2];
	tgcd[2].gd.pos.x = 5; tgcd[2].gd.pos.y = 31+4; 
	tgcd[2].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[2].gd.popup_msg = tgcd[0].gd.popup_msg;
	tgcd[2].creator = GLabelCreate;
	thvarray[3] = &tgcd[2];

	tgcd[3].gd.pos.x = 85; tgcd[3].gd.pos.y = 31;
	tgcd[3].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[3].gd.cid = CID_TeX_Depth;
	tgcd[3].creator = GTextFieldCreate;
	thvarray[4] = &tgcd[3]; thvarray[5] = NULL;

	tlabel[4].text = (unichar_t *) _("Sub Pos:");
	tlabel[4].text_is_1byte = true;
	tgcd[4].gd.label = &tlabel[4];
	tgcd[4].gd.pos.x = 5; tgcd[4].gd.pos.y = 57+4; 
	tgcd[4].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	tgcd[4].gd.popup_msg = tgcd[0].gd.popup_msg;
	tgcd[4].creator = GLabelCreate;
	thvarray[6] = &tgcd[4];

	tgcd[5].gd.pos.x = 85; tgcd[5].gd.pos.y = 57;
	tgcd[5].gd.flags = gg_enabled|gg_visible|gg_text_xim;
	tgcd[5].gd.cid = CID_TeX_Sub;
	tgcd[5].creator = GTextFieldCreate;
	thvarray[7] = &tgcd[5]; thvarray[8] = NULL;

	tgcd[6].gd.pos.x = 5; tgcd[6].gd.pos.y = 83+4;
	tgcd[6].gd.flags = gg_visible | gg_enabled|gg_utf8_popup;
	tlabel[6].text = (unichar_t *) _("Sup Pos:");
	tlabel[6].text_is_1byte = true;
	tgcd[6].gd.label = &tlabel[6];
	tgcd[6].gd.popup_msg = tgcd[0].gd.popup_msg;
	tgcd[6].creator = GLabelCreate;
	thvarray[9] = &tgcd[6];

	tgcd[7].gd.pos.x = 85; tgcd[7].gd.pos.y = 83;
	tgcd[7].gd.flags = gg_visible | gg_enabled;
	tgcd[7].gd.cid = CID_TeX_Super;
	tgcd[7].creator = GTextFieldCreate;
	thvarray[10] = &tgcd[7]; thvarray[11] = NULL;

	thvarray[12] = GCD_Glue; thvarray[13] = GCD_Glue; thvarray[14] = NULL;
	thvarray[15] = NULL;

	tbox[0].gd.flags = gg_enabled|gg_visible;
	tbox[0].gd.u.boxelements = thvarray;
	tbox[0].creator = GHVBoxCreate;

	memset(&mgcd,0,sizeof(mgcd));
	memset(&mbox,0,sizeof(mbox));
	memset(&mlabel,0,sizeof(mlabel));
	memset(&aspects,'\0',sizeof(aspects));

	i = 0;
	aspects[i].text = (unichar_t *) _("Unicode");
	aspects[i].text_is_1byte = true;
	aspects[i].selected = true;
	aspects[i++].gcd = ubox;

	aspects[i].text = (unichar_t *) _("Comment");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = cbox;

#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	aspects[i].text = (unichar_t *) _("Pos");
#else
	aspects[i].text = (unichar_t *) _("Positionings");
#endif
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[0];

#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	aspects[i].text = (unichar_t *) _("Pair");
#else
	aspects[i].text = (unichar_t *) _("Pairwise Pos");
#endif
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[1];

#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	aspects[i].text = (unichar_t *) _("Subs");
#else
	aspects[i].text = (unichar_t *) _("Substitutions");
#endif
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[2];

	aspects[i].text = (unichar_t *) _("Alt Subs");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[3];

	aspects[i].text = (unichar_t *) _("Mult Subs");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[4];

#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	aspects[i].text = (unichar_t *) _("Ligature");
#else
	aspects[i].text = (unichar_t *) _("Ligatures");
#endif
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[5];

	aspects[i].text = (unichar_t *) _("Components");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = cobox;

	aspects[i].text = (unichar_t *) _("Counters");
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = pstbox[6];

	aspects[i].text = (unichar_t *) U_("ΤεΧ");		/* TeX */
	aspects[i].text_is_1byte = true;
	aspects[i++].gcd = tbox;

	mgcd[0].gd.pos.x = 4; mgcd[0].gd.pos.y = 6;
	mgcd[0].gd.u.tabs = aspects;
#ifdef FONTFORGE_CONFIG_INFO_HORIZONTAL
	mgcd[0].gd.pos.width = CI_Width-10;
	mgcd[0].gd.pos.height = CI_Height-70;
	mgcd[0].gd.flags = gg_visible | gg_enabled;
#else
	mgcd[0].gd.pos.width = CI_Width-10+65;
	mgcd[0].gd.pos.height = CI_Height-70;
	mgcd[0].gd.flags = gg_visible | gg_enabled | gg_tabset_vert;
#endif
	mgcd[0].gd.cid = CID_Tabs;
	mgcd[0].creator = GTabSetCreate;
	mvarray[0] = &mgcd[0]; mvarray[1] = NULL;

	mgcd[1].gd.pos.x = 40; mgcd[1].gd.pos.y = mgcd[0].gd.pos.y+mgcd[0].gd.pos.height+3;
	mgcd[1].gd.pos.width = -1; mgcd[1].gd.pos.height = 0;
	mgcd[1].gd.flags = gg_visible | gg_enabled ;
	mlabel[1].text = (unichar_t *) _("< _Prev");
	mlabel[1].text_is_1byte = true;
	mlabel[1].text_in_resource = true;
	mgcd[1].gd.mnemonic = 'P';
	mgcd[1].gd.label = &mlabel[1];
	mgcd[1].gd.handle_controlevent = CI_NextPrev;
	mgcd[1].gd.cid = -1;
	mharray1[0] = GCD_Glue; mharray1[1] = &mgcd[1]; mharray1[2] = GCD_Glue;
	mgcd[1].creator = GButtonCreate;

	mgcd[2].gd.pos.x = -40; mgcd[2].gd.pos.y = mgcd[1].gd.pos.y;
	mgcd[2].gd.pos.width = -1; mgcd[2].gd.pos.height = 0;
	mgcd[2].gd.flags = gg_visible | gg_enabled ;
	mlabel[2].text = (unichar_t *) _("_Next >");
	mlabel[2].text_is_1byte = true;
	mlabel[2].text_in_resource = true;
	mgcd[2].gd.label = &mlabel[2];
	mgcd[2].gd.mnemonic = 'N';
	mgcd[2].gd.handle_controlevent = CI_NextPrev;
	mgcd[2].gd.cid = 1;
	mharray1[3] = GCD_Glue; mharray1[4] = &mgcd[2]; mharray1[5] = GCD_Glue; mharray1[6] = NULL;
	mgcd[2].creator = GButtonCreate;

	mbox[2].gd.flags = gg_enabled|gg_visible;
	mbox[2].gd.u.boxelements = mharray1;
	mbox[2].creator = GHBoxCreate;
	mvarray[2] = &mbox[2]; mvarray[3] = NULL;

	mgcd[3].gd.pos.x = 25-3; mgcd[3].gd.pos.y = CI_Height-31-3;
	mgcd[3].gd.pos.width = -1; mgcd[3].gd.pos.height = 0;
	mgcd[3].gd.flags = gg_visible | gg_enabled | gg_but_default;
	mlabel[3].text = (unichar_t *) _("_OK");
	mlabel[3].text_is_1byte = true;
	mlabel[3].text_in_resource = true;
	mgcd[3].gd.mnemonic = 'O';
	mgcd[3].gd.label = &mlabel[3];
	mgcd[3].gd.handle_controlevent = CI_OK;
	mharray2[0] = GCD_Glue; mharray2[1] = &mgcd[3]; mharray2[2] = GCD_Glue; mharray2[3] = GCD_Glue;
	mgcd[3].creator = GButtonCreate;

	mgcd[4].gd.pos.x = -25; mgcd[4].gd.pos.y = mgcd[3].gd.pos.y+3;
	mgcd[4].gd.pos.width = -1; mgcd[4].gd.pos.height = 0;
	mgcd[4].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	mlabel[4].text = (unichar_t *) _("_Done");
	mlabel[4].text_is_1byte = true;
	mlabel[4].text_in_resource = true;
	mgcd[4].gd.label = &mlabel[4];
	mgcd[4].gd.mnemonic = 'C';
	mgcd[4].gd.handle_controlevent = CI_Cancel;
	mgcd[4].gd.cid = CID_Cancel;
	mharray2[4] = GCD_Glue; mharray2[5] = &mgcd[4]; mharray2[6] = GCD_Glue; mharray2[7] = NULL;
	mgcd[4].creator = GButtonCreate;

	mbox[3].gd.flags = gg_enabled|gg_visible;
	mbox[3].gd.u.boxelements = mharray2;
	mbox[3].creator = GHBoxCreate;
	mvarray[4] = &mbox[3]; mvarray[5] = NULL;
	mvarray[6] = NULL;

	mbox[0].gd.pos.x = mbox[0].gd.pos.y = 2;
	mbox[0].gd.flags = gg_enabled|gg_visible;
	mbox[0].gd.u.boxelements = mvarray;
	mbox[0].creator = GHVGroupCreate;

	GGadgetsCreate(ci->gw,mbox);
	GHVBoxSetExpandableRow(mbox[0].ret,1);
	GHVBoxSetExpandableCol(mbox[2].ret,gb_expandgluesame);
	GHVBoxSetExpandableCol(mbox[3].ret,gb_expandgluesame);

	GHVBoxSetExpandableRow(ubox[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(ubox[0].ret,1);
	GHVBoxSetExpandableCol(ubox[2].ret,gb_expandgluesame);

	GHVBoxSetExpandableRow(cbox[0].ret,1);
	GHVBoxSetExpandableCol(cbox[2].ret,gb_expandglue);

	for ( i=0; i<7; ++i ) {
	    GHVBoxSetExpandableRow(pstbox[i][0].ret,0);
	    GHVBoxSetExpandableCol(pstbox[i][2].ret,gb_expandgluesame);
	    GHVBoxSetExpandableCol(pstbox[i][3].ret,gb_expandgluesame);
	}

	GHVBoxSetExpandableRow(cobox[0].ret,gb_expandglue);
	GHVBoxSetExpandableRow(tbox[0].ret,gb_expandglue);
	GHVBoxSetExpandableCol(tbox[0].ret,1);
	GHVBoxSetPadding(tbox[0].ret,6,4);

	GHVBoxFitWindow(mbox[0].ret);
	
	memset(&rq,0,sizeof(rq));
	rq.family_name = monospace;
	rq.point_size = 12;
	rq.weight = 400;
	font = GDrawInstanciateFont(GDrawGetDisplayOfWindow(ci->gw),&rq);
	for ( i=0; i<5; ++i )
	    GGadgetSetFont(psgcd[i][0].ret,font);

    CIFillup(ci);

    GWidgetHidePalettes();
    GDrawSetVisible(ci->gw,true);
}

void CharInfoDestroy(CharInfo *ci) {
    GDrawDestroyWindow(ci->gw);
}

struct sel_dlg {
    int done;
    int ok;
    FontView *fv;
};

GTextInfo pst_names[] = {
    { (unichar_t *) N_("Ligature"), NULL, 0, 0, (void *) pst_ligature, NULL, false, false, false, false, true, false, true },
    { (unichar_t *) N_("Simple Substitution"), NULL, 0, 0, (void *) pst_substitution, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Alternate Substitutions"), NULL, 0, 0, (void *) pst_alternate, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Multiple Substitution"), NULL, 0, 0, (void *) pst_multiple, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Simple Position"), NULL, 0, 0, (void *) pst_position, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Pairwise Position"), NULL, 0, 0, (void *) pst_pair, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Kerning"), NULL, 0, 0, (void *) pst_kerning, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("VKerning"), NULL, 0, 0, (void *) pst_vkerning, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Anchor Classes"), NULL, 0, 0, (void *) pst_anchors, NULL, false, false, false, false, false, false, true },
    { (unichar_t *) N_("Ligature Caret"), NULL, 0, 0, (void *) pst_lcaret, NULL, false, false, false, false, false, false, true },
    { NULL }
};
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

struct match_data {
    SplineFont *sf;
    enum possub_type type;
    int tagcnt;
    uint32 tags[10];
    char *contains;
    AnchorClass *ac;
    SplineChar *kernwith;
};

static int SCMatchAnchor(SplineChar *sc,struct match_data *md) {
    AnchorPoint *ap;

    if ( sc==NULL )
return( false );

    for ( ap=sc->anchor; ap!=NULL; ap=ap->next ) {
	if ( ap->anchor==md->ac )
return( true );
    }
return( false );
}

static int SCMatchPST(SplineChar *sc,struct match_data *md) {
    PST *pst;
    int j;

    if ( sc==NULL )
return( false );
    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == md->type ) {
	    if ( md->tagcnt!=0 ) {
		for ( j=md->tagcnt-1; j>=0; --j )
		    if ( pst->tag==md->tags[j] )
		break;
		if ( j==-1 )
    continue;
	    }
	    if ( md->type==pst_position || md->contains==NULL ||
		    PSTContains(pst->u.lig.components,md->contains))	/* pair names will match here too */
return( true );
	}
    }
return( false );
}

static int SCMatchLCaret(SplineChar *sc,struct match_data *md) {
    PST *pst;
    int j;

    if ( sc==NULL )
return( false );
    for ( pst=sc->possub; pst!=NULL; pst=pst->next ) {
	if ( pst->type == pst_lcaret ) {
	    for ( j = pst->u.lcaret.cnt-1; j>=0; --j ) {
		if ( pst->u.lcaret.carets[j]!=0 )
return( true );
	    }
return( false );
	}
    }
return( false );
}

static int _SCMatchKern(SplineChar *sc,struct match_data *md,int isv) {
    SplineFont *sf = md->sf;
    int i;
    KernPair *kp, *head;
    KernClass *kc;

    if ( sc==NULL )
return( false );
    head = isv ? sc->vkerns : sc->kerns;

    if ( md->kernwith==NULL ) {
	/* Is the current character involved in ANY kerning */
	if ( head!=NULL )
return( true );
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    for ( kp = isv ? sf->glyphs[i]->vkerns : sf->glyphs[i]->kerns; kp!=NULL; kp = kp->next ) {
		if ( kp->sc == sc )
return( true );
	    }
	}
	for ( kc = isv ? sf->vkerns : sf->kerns; kc!=NULL; kc=kc->next ) {
	    for ( i=1; i<kc->first_cnt; ++i )
		if ( PSTContains(kc->firsts[i],sc->name) )
return( true );
	    for ( i=1; i<kc->second_cnt; ++i )
		if ( PSTContains(kc->seconds[i],sc->name) )
return( true );
	}
    } else {
	for ( kp=head; kp!=NULL; kp=kp->next ) {
	    if ( kp->sc==md->kernwith )
return( true );
	}
	for ( kp=isv ? md->kernwith->vkerns : md->kernwith->kerns ; kp!=NULL; kp=kp->next ) {
	    if ( kp->sc==sc )
return( true );
	}
	for ( kc = isv ? sf->vkerns : sf->kerns; kc!=NULL; kc=kc->next ) {
	    if ( KernClassContains(kc,sc->name,md->kernwith->name,false)!=0 )
return( true );
	}
    }
return( false );
}

static int SCMatchKern(SplineChar *sc,struct match_data *md) {
return( _SCMatchKern(sc,md,false));
}

static int SCMatchVKern(SplineChar *sc,struct match_data *md) {
return( _SCMatchKern(sc,md,true));
}
    
int FVParseSelectByPST(FontView *fv,int type,
	const unichar_t *tags,const unichar_t *contents,
	int search_type) {
    struct match_data md;
    uint8 u[4];
    int i, j;
    int (*tester)(SplineChar *sc,struct match_data *md);
    SplineFont *sf;
    AnchorClass *ac;
    int first;
    int gid;

    memset(&md,0,sizeof(md));
    md.sf = fv->sf;
    md.type = type;
    if ( type!=pst_anchors || type!=pst_position || type!=pst_lcaret ) {
	md.contains = cu_copy( contents );
	if ( strcmp( md.contains,"" )==0 || strcmp( md.contains,"*" )==0 ) {
	    free( md.contains );
	    md.contains = NULL;
	}
	if (( type==pst_kerning || type==pst_vkerning ) &&
		md.contains!=NULL ) {
	    md.kernwith = SFGetChar(md.sf,-1,md.contains);
	    if ( md.kernwith==NULL )
		gwwv_post_error(_("Selec_t By ATT..."),_("Could not find the glyph: %.70s"),md.contains);
	    free(md.contains);
	    md.contains = NULL;
	    if ( md.kernwith==NULL )
return( false );
	}
    }
    if ( type==pst_anchors ) {
	char *u8tags = u2utf8_copy(tags);
	for ( ac = md.sf->anchor; ac!=NULL; ac=ac->next )
	    if ( strcmp(u8tags,ac->name)==0 )
	break;
	free(u8tags);
	md.ac = ac;
	if ( ac==NULL ) {
	    free( md.contains );
	    gwwv_post_error(_("Selec_t By ATT..."),_("Unknown Anchor Class: %.70s"),tags);
return( false );
	}
    } else if ( type!=pst_kerning && type!=pst_vkerning && type!=pst_lcaret ) {
	if ( uc_strcmp( tags,"" )==0 || uc_strcmp( tags,"*" )==0 )
	    md.tagcnt = 0;
	else {
	    for ( i=0; i<sizeof(md.tags)/sizeof(md.tags[0]) && *tags!='\0'; ++i ) {
		memset(u,' ',4);
		for ( j=0; j<4 && *tags!='\0' && *tags!=',' && *tags!=' '; ++j )
		    u[j] = *tags++;
		while ( *tags==',' || *tags==' ' ) ++tags;
		md.tags[i] = (u[0]<<24) | (u[1]<<16) | (u[2]<<8) | u[3];
	    }
	    if ( *tags!='\0' ) {
		gwwv_post_error(_("Selec_t By ATT..."),_("Too many tags specified"));
		free( md.contains );
return( false );
	    }
	    md.tagcnt = i;
	}
    }

    if ( type==pst_anchors )
	tester = SCMatchAnchor;
    else if ( type==pst_lcaret )
	tester = SCMatchLCaret;
    else if ( type==pst_kerning )
	tester = SCMatchKern;
    else if ( type==pst_vkerning )
	tester = SCMatchVKern;
    else
	tester = SCMatchPST;

    sf = fv->sf;
    first = -1;
    if ( search_type==1 ) {	/* Select results */
	for ( i=0; i<fv->map->enccount; ++i ) {
	    gid=fv->map->map[i];
	    if ( (fv->selected[i] = tester(gid==-1?NULL:sf->glyphs[gid],&md)) && first==-1 )
		first = i;
	}
    } else if ( search_type==2) {/* merge results */
	for ( i=0; i<fv->map->enccount; ++i ) if ( !fv->selected[i] ) {
	    gid=fv->map->map[i];
	    if ( (fv->selected[i] = tester(gid==-1?NULL:sf->glyphs[gid],&md)) && first==-1 )
		first = i;
	}
    } else {			/* restrict selection */
	for ( i=0; i<fv->map->enccount; ++i ) if ( fv->selected[i] ) {
	    gid=fv->map->map[i];
	    if ( (fv->selected[i] = tester(gid==-1?NULL:sf->glyphs[gid],&md)) && first==-1 )
		first = i;
	}
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( first!=-1 )
	FVScrollToChar(fv,first);
    else if ( !no_windowing_ui )
	gwwv_post_notice(_("Select By ATT..."),_("No glyphs matched"));
    if (  !no_windowing_ui )
	GDrawRequestExpose(fv->v,NULL,false);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
return( true );
}
    
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static GTextInfo **LListFromList(GTextInfo *array) {
    int cnt;
    GTextInfo **ti;

    for ( cnt=0; array[cnt].text!=NULL; ++cnt);
    ti = galloc((cnt+1)*sizeof(GTextInfo *));
    for ( cnt=0; array[cnt].text!=NULL; ++cnt) {
	ti[cnt] = gcalloc(1,sizeof(GTextInfo));
	*(ti[cnt]) = array[cnt];
	if ( ti[cnt]->text_is_1byte ) {
	    ti[cnt]->text_is_1byte = false;
	    ti[cnt]->text = utf82u_copy((char *) ti[cnt]->text);
	}
	ti[cnt]->fg = ti[cnt]->bg = COLOR_DEFAULT;
    }
    ti[cnt] = gcalloc(1,sizeof(GTextInfo));
return( ti );
}

static int SelectStuff(struct sel_dlg *sld,GWindow gw) {
    int type = (intpt) (GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PST))->userdata);
    int search_type = GGadgetIsChecked(GWidgetGetControl(gw,CID_SelectResults)) ? 1 :
	    GGadgetIsChecked(GWidgetGetControl(gw,CID_MergeResults)) ? 2 :
		3;
return( FVParseSelectByPST(sld->fv, type,
	_GGadgetGetTitle(GWidgetGetControl(gw,CID_Tag)),
	_GGadgetGetTitle(GWidgetGetControl(gw,CID_Contents)),
	search_type));
}

static int selpst_e_h(GWindow gw, GEvent *event) {
    struct sel_dlg *sld = GDrawGetUserData(gw);
    static unichar_t nullstr[] = { 0 };
    static GTextInfo *tags[] = { NULL,
	simplepos_tags,
	pairpos_tags,
	simplesubs_tags,
	alternatesubs_tags,
	multiplesubs_tags,
	ligature_tags,
	NULL,
	NULL,
	NULL };

    if ( event->type==et_close ) {
	sld->done = true;
	sld->ok = false;
    } else if ( event->type==et_char ) {
	if ( event->u.chr.keysym == GK_F1 || event->u.chr.keysym == GK_Help ) {
	    help("selectbyatt.html");
return( true );
	}
return( false );
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_buttonactivate ) {
	sld->ok = GGadgetGetCid(event->u.control.g);
	if ( !sld->ok || SelectStuff(sld,gw))
	    sld->done = true;
    } else if ( event->type==et_controlevent &&
	    event->u.control.subtype == et_listselected &&
	    GGadgetGetCid(event->u.control.g)==CID_PST ) {
	int type = (intpt) (GGadgetGetListItemSelected(event->u.control.g)->userdata);
	if ( type==pst_anchors ) {
	    GGadgetSetList(GWidgetGetControl(gw,CID_Tag),AnchorClassesSimpleLList(sld->fv->sf),false);
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_Tag),true);
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_Contents),false);
	} else if ( type==pst_kerning ) {
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_Tag),nullstr);
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_Tag),false);
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_Contents),true);
	} else if ( type==pst_lcaret ) {
	    GGadgetSetTitle(GWidgetGetControl(gw,CID_Tag),nullstr);
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_Tag),false);
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_Contents),false);
	} else if ( type==pst_position ) {
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_Contents),false);
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_Tag),true);
	    GGadgetSetList(GWidgetGetControl(gw,CID_Tag),LListFromList(simplepos_tags),false);
	} else {
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_Contents),true);
	    GGadgetSetEnabled(GWidgetGetControl(gw,CID_Tag),true);
	    GGadgetSetList(GWidgetGetControl(gw,CID_Tag),LListFromList(tags[type]),false);
	}
    } else if ( event->type==et_controlevent && event->u.control.subtype == et_textchanged &&
	    event->u.control.u.tf_changed.from_pulldown!=-1 &&
	    GGadgetGetCid(event->u.control.g)==CID_Tag ) {
	int type = (intpt) (GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PST))->userdata);
	if ( type!=pst_kerning && type!=pst_anchors && type!=pst_lcaret ) {
	    uint32 tag = (uint32) (intpt) tags[type][event->u.control.u.tf_changed.from_pulldown].userdata;
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
    }
return( true );
}

void FVSelectByPST(FontView *fv) {
    static struct sel_dlg sld;
    static GWindow gw;
    GRect pos;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[14];
    GTextInfo label[14];
    int i,j;

    CharInfoInit();

    memset(&sld,0,sizeof(sld));
    sld.fv = fv;
    if ( gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title =  _("Select By ATT...");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,160));
	pos.height = GDrawPointsToPixels(NULL,204);
	gw = GDrawCreateTopWindow(NULL,&pos,selpst_e_h,&sld,&wattrs);

	memset(&gcd,0,sizeof(gcd));
	memset(&label,0,sizeof(label));

	i=0;
	gcd[i].gd.label = &pst_names[0];
	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = 5+4;
	gcd[i].gd.flags = gg_enabled|gg_visible/*|gg_list_exactlyone*/;
	gcd[i].gd.u.list = pst_names;
	gcd[i].gd.cid = CID_PST;
	gcd[i++].creator = GListButtonCreate;
	if ( fv->sf->anchor==NULL )
	    for ( j=0; pst_names[j].text!=NULL; ++j )
		if ( pst_names[j].text == (void *) _("Anchor Classes") )
		    pst_names[j].disabled = true;

	label[i].text = (unichar_t *) _("_Tag:");
	label[i].text_in_resource = true;
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+14;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.u.list = ligature_tags;
	gcd[i].gd.cid = CID_Tag;
	gcd[i++].creator = GListFieldCreate;

	label[i].text = (unichar_t *) _("Containing:");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26; 
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i++].creator = GLabelCreate;

	gcd[i].gd.pos.x = 10; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+14;
	gcd[i].gd.pos.width = 140;
	gcd[i].gd.flags = gg_enabled|gg_visible;
	gcd[i].gd.cid = CID_Contents;
	gcd[i++].creator = GTextFieldCreate;

	label[i].text = (unichar_t *) _("Select Results");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+26; 
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_cb_on|gg_utf8_popup;
	gcd[i].gd.popup_msg = (unichar_t *) _("Set the selection of the font view to the characters\nfound by this search");
	gcd[i].gd.cid = CID_SelectResults;
	gcd[i++].creator = GRadioCreate;

	label[i].text = (unichar_t *) _("Merge Results");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15; 
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[i].gd.popup_msg = (unichar_t *) _("Expand the selection of the font view to include\nall the characters found by this search");
	gcd[i].gd.cid = CID_MergeResults;
	gcd[i++].creator = GRadioCreate;

	label[i].text = (unichar_t *) _("Restrict Selection");
	label[i].text_is_1byte = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.pos.x = 5; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+15; 
	gcd[i].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
	gcd[i].gd.popup_msg = (unichar_t *) _("Only search the selected characters, and unselect\nany characters which do not match this search");
	gcd[i].gd.cid = CID_RestrictSelection;
	gcd[i++].creator = GRadioCreate;

	gcd[i].gd.pos.x = 15-3; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+22;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[i].text = (unichar_t *) _("_OK");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.mnemonic = 'O';
	gcd[i].gd.label = &label[i];
	gcd[i].gd.cid = true;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = -15; gcd[i].gd.pos.y = gcd[i-1].gd.pos.y+3;
	gcd[i].gd.pos.width = -1; gcd[i].gd.pos.height = 0;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[i].text = (unichar_t *) _("_Cancel");
	label[i].text_is_1byte = true;
	label[i].text_in_resource = true;
	gcd[i].gd.label = &label[i];
	gcd[i].gd.mnemonic = 'C';
	gcd[i].gd.cid = false;
	gcd[i++].creator = GButtonCreate;

	gcd[i].gd.pos.x = 2; gcd[i].gd.pos.y = 2;
	gcd[i].gd.pos.width = pos.width-4; gcd[i].gd.pos.height = pos.height-4;
	gcd[i].gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels;
	gcd[i++].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);
    } else {
	if ( (intpt) (GGadgetGetListItemSelected(GWidgetGetControl(gw,CID_PST))->userdata) ==
		pst_anchors ) {
	    if ( fv->sf->anchor==NULL ) {
		GGadgetSelectOneListItem(GWidgetGetControl(gw,CID_PST),0);
		GGadgetSetList(GWidgetGetControl(gw,CID_Tag),LListFromList(ligature_tags),false);
		GGadgetSetEnabled(GWidgetGetControl(gw,CID_Contents),true);
	    } else
		GGadgetSetList(GWidgetGetControl(gw,CID_Tag),AnchorClassesSimpleLList(fv->sf),false);
	}
    }

    GDrawSetVisible(gw,true);
    while ( !sld.done )
	GDrawProcessOneEvent(NULL);
    if ( sld.ok ) {
    }
    GDrawSetVisible(gw,false);
}

int SCAnyFeatures(SplineChar *sc) {
    PST *pst;
    KernPair *kp;
    int i;
    SplineFont *sf;

    if ( sc==NULL )
return( false );

    for ( kp = sc->kerns; kp!=NULL; kp=kp->next )
	if ( kp->off!=0 )
return( true );
    for ( kp = sc->vkerns; kp!=NULL; kp=kp->next )
	if ( kp->off!=0 )
return( true );

    for ( pst = sc->possub; pst!=NULL; pst=pst->next )
	if ( pst->type!=pst_lcaret )
return( true );

    sf = sc->parent;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( kp = sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sc == sc && kp->off!=0 )
return( true );
	for ( kp = sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sc == sc && kp->off!=0 )
return( true );
    }

return( false );
}

static int compare_tag(const void *_n1, const void *_n2) {
    const uint32 *t1 = _n1, *t2 = _n2;
return( *t1 > *t2 ? 1 : *t1 == *t2 ? 0 : -1 );
}

void SCCopyFeatures(SplineChar *sc) {
    PST *pst;
    KernPair *kp;
    int i,j;
    SplineFont *sf;
    int cnt, haslk, haslv, hask, hasv;
    char *sel;
    char **choices;
    uint32 *tags;

    if ( sc==NULL )
return;

    cnt = 0;
    haslk = haslv = false;
    for ( kp = sc->kerns; kp!=NULL; kp=kp->next )
	if ( kp->off!=0 ) {
	    haslk = true;
    break;
	}
    for ( kp = sc->vkerns; kp!=NULL; kp=kp->next )
	if ( kp->off!=0 ) {
	    haslv = true;
    break;
	}
    cnt += haslk + haslv;

    sf = sc->parent;
    hask = hasv = false;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( kp = sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sc == sc && kp->off!=0 ) hask = true;
	for ( kp = sf->glyphs[i]->kerns; kp!=NULL; kp=kp->next )
	    if ( kp->sc == sc && kp->off!=0 ) hasv = true;
    }
    cnt += hask + hasv;

    for ( pst = sc->possub; pst!=NULL; pst=pst->next )
	if ( pst->type!=pst_lcaret )
	    ++cnt;

    choices = gcalloc(cnt+1,sizeof(char *));
    tags = gcalloc(cnt+1,sizeof(uint32));
    sel = gcalloc(cnt,sizeof(char));

    cnt = 0;
    for ( pst = sc->possub; pst!=NULL; pst=pst->next )
	if ( pst->type!=pst_lcaret ) {
	    for ( i=0; i<cnt; ++i )
		if ( pst->tag==tags[i] )
	    break;
	    if ( i==cnt )
		tags[cnt++] = pst->tag;
	}
    qsort(tags,cnt,sizeof(uint32),compare_tag);
    for ( i=0; i<cnt; ++i )
	choices[i] = TagFullName(sf,tags[i],-1);
    if ( haslk )
	choices[i++] = copy( _("Kern Pairs with this as the initial glyph"));
    if ( haslv )
	choices[i++] = copy( _("Vertical Kern Pairs with this as the initial glyph"));
    if ( hask )
	choices[i++] = copy( _("Kern Pairs with this as the final glyph"));
    if ( hasv )
	choices[i++] = copy( _("Vertical Kern Pairs with this as the final glyph"));
    if ( gwwv_choose_multiple(_("Copy Which Features?"),(const char **) choices,sel,i,NULL,
	    _("Copy Which Features?"))==-1 )
return;
    CopyPSTStart(sf);
    for ( i=0; i<cnt; ++i ) if ( sel[i] ) {
	for ( pst = sc->possub; pst!=NULL; pst=pst->next )
	    if ( pst->tag == tags[i] && pst->type!=pst_lcaret )
		CopyPSTAppend(pst->type,PST2Text(pst,sf));
    }
    if ( haslk ) {
	if ( sel[i] ) {
	    for ( kp=sc->kerns; kp!=NULL; kp=kp->next ) if ( kp->off!=0 )
		CopyPSTAppend(pst_kerning,Kern2Text(kp->sc,kp,false));
	}
	++i;
    }
    if ( haslv ) {
	if ( sel[i] ) {
	    for ( kp=sc->vkerns; kp!=NULL; kp=kp->next ) if ( kp->off!=0 )
		CopyPSTAppend(pst_vkerning,Kern2Text(kp->sc,kp,true));
	}
	++i;
    }
    if ( hask ) {
	if ( sel[i] ) {
	    for ( j=0; j<sf->glyphcnt; ++j ) if ( sf->glyphs[j]!=NULL )
		for ( kp=sf->glyphs[j]->kerns; kp!=NULL; kp=kp->next )
		    if ( kp->off!=0 && kp->sc==sc )
			CopyPSTAppend(pst_kernback,Kern2Text(sf->glyphs[j],kp,false));
	}
	++i;
    }
    if ( hasv ) {
	if ( sel[i] ) {
	    for ( j=0; j<sf->glyphcnt; ++j ) if ( sf->glyphs[j]!=NULL )
		for ( kp=sf->glyphs[j]->vkerns; kp!=NULL; kp=kp->next )
		    if ( kp->off!=0 && kp->sc==sc )
			CopyPSTAppend(pst_vkernback,Kern2Text(sf->glyphs[j],kp,true));
	}
	++i;
    }
    free(tags);
    free(sel);
    for ( j=0; j<i; ++j )
	free(choices[j]);
    free(choices);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

void CharInfoInit(void) {
    static GTextInfo *lists[] = { contextualchainingpos_tags,contextualpos_tags,
	reversechainingsubs_tags, contextualchainingsubs_tags,
	contextualsubs_tags, mark_tags, multiplesubs_tags, alternatesubs_tags,
	simplesubs_tags, pairpos_tags, simplepos_tags, ligature_tags,
	languages, scripts, glyphclasses, std_colors, pst_names, NULL };
    static int done = 0;
    int i, j;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    static char **cnames[] = { sli_names, newstrings, editstrings, NULL };
#endif

    if ( done )
return;
    done = true;
    for ( i=0; lists[i]!=NULL; ++i )
	for ( j=0; lists[i][j].text!=NULL; ++j )
	    lists[i][j].text = (unichar_t *) S_((char *) lists[i][j].text);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    for ( i=0; cnames[i]!=NULL; ++i )
	for ( j=0; cnames[i][j]!=NULL; ++j )
	    cnames[i][j] = _(cnames[i][j]);
#endif
}    
