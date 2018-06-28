/* Copyright (C) 2000-2012 by George Williams */
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
#include <stdlib.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <gdraw.h>
#include <gwidget.h>
#include <ggadget.h>
#include <charset.h>
#include <chardata.h>
#include <gresource.h>
#include "ggadgetP.h"		/* For the font family names */

#include "gutils/unicodelibinfo.h"
//#include "../fontforge/unicodelibinfo.c"

#define INSCHR_CharSet	1
#define INSCHR_Char	2
#define INSCHR_Hex	3
#define INSCHR_Dec	4
#define INSCHR_Unicode	5
#define INSCHR_KuTen	6
#define INSCHR_Prev	7
#define INSCHR_Next	8
#define INSCHR_Insert	9
#define INSCHR_Close	10
#define INSCHR_Show	11

enum dsp_mode { d_hex, d_dec, d_unicode, d_kuten };

static struct inschr {
    GWindow icw;
    int width, height;
    int spacing, ybase;
    long sel_char;
    enum charset map;
    int page;
    enum dsp_mode dsp_mode;
    unsigned int hidden: 1;
    unsigned int show_enabled: 1;
    unsigned int mouse_down: 1;
    unsigned int mouse_in: 1;
    unsigned int pageable: 1;
    unsigned int flash: 1;
    int as,sas;
    short x,y;
    GTimer *flash_time;
    GFont *font;
    GFont *smallfont;
} inschr = {
     NULL, 0, 0, 0, 0, EOF, em_iso8859_1, 0, d_hex,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, NULL, NULL, NULL
};

static struct unicode_subranges { unichar_t first; int len; const char *name; } unicode_ranges[] = {
	{ 0x100, 0x80, "Latin Extended A" },
	{ 0x180, 0x98, "Latin Extended B" },
	{ 0x250, 0x60, "IPA Extensions" },
	{ 0x1e00, 0x100, "Latin Additional Extensions" },
	{ 0xfb00, 0x010, "Latin Ligatures" },
	{ 0xff00, 0x060, "Latin Fullwidth" },
	{ 0x02b0, 0x040, "Spacing Modifier Letters" },
	{ 0x0300, 0x070, "Combining Diacritics" },
	{ 0x0370, 0x90, "Greek & Coptic" },
	{ 0x1f00, 0x100, "Greek Additional Extensions" },
	{ 0x0400, 0x100, "Cyrillic" },
	{ 0x0530, 0x060, "Armenian" },
	{ 0xfb10, 0x010, "Armenian Ligatures" },
	{ 0x0590, 0x070, "Hebrew" },
	{ 0xfb20, 0x030, "Hebrew Ligatures" },
	{ 0x0600, 0x100, "Arabic" },
	{ 0xfb50, 0x100, "Arabic Ligatures A1" },
	{ 0xfc50, 0x100, "Arabic Ligatures A2" },
	{ 0xfd50, 0x0b0, "Arabic Ligatures A3" },
	{ 0xfe70, 0x090, "Arabic Ligatures B" },
	{ 0x0700, 0x080, "Syriac" },
	{ 0x0780, 0x080, "Thaana" },
	{ 0x0900, 0x080, "Devanagari" },
	{ 0x0980, 0x080, "Bengali" },
	{ 0x0a00, 0x080, "Gurmukhi" },
	{ 0x0a80, 0x080, "Gujarati" },
	{ 0x0b00, 0x080, "Oriya" },
	{ 0x0b80, 0x080, "Tamil" },
	{ 0x0c00, 0x080, "Telugu" },
	{ 0x0c80, 0x080, "Kannada" },
	{ 0x0d00, 0x080, "Malayalam" },
	{ 0x0d80, 0x080, "Sinhala" },
	{ 0x0e00, 0x080, "Thai" },
	{ 0x0e80, 0x080, "Lao" },
	{ 0x0f00, 0x100, "Tibetan" },
	{ 0x1000, 0x080, "Myanmar" },
	{ 0x10A0, 0x080, "Georgian" },
	{ 0x1100, 0x100, "Hangul Jamo" },
	{ 0xffa0, 0x040, "Hangul Jamo Halfwidth" },
	{ 0x1200, 0x100, "Ethiopic A" },
	{ 0x1300, 0x080, "Ethiopic B" },
	{ 0x1380, 0x080, "Cherokee" },
	{ 0x1400, 0x100, "Canadian Syllabics A" },
	{ 0x1500, 0x100, "Canadian Syllabics B" },
	{ 0x1600, 0x080, "Canadian Syllabics C" },
	{ 0x1680, 0x020, "Ogham" },
	{ 0x16A0, 0x060, "Runic" },
	{ 0x1780, 0x080, "Khmer" },
	{ 0x1800, 0x100, "Mongolian" },
	{ 0x2000, 0x070, "General Punctuation" },
	{ 0x2070, 0x030, "Super & Sub scripts" },
	{ 0x20a0, 0x030, "Currency Symbols" },
	{ 0x20D0, 0x030, "Combining Symbol Diacritics" },
	{ 0x2100, 0x050, "Letterlike Symbols" },
	{ 0x2150, 0x040, "Number Forms" },
	{ 0x2190, 0x070, "Arrows" },
	{ 0x2200, 0x100, "Mathematical Operators" },
	{ 0x2300, 0x080, "Miscellaneous Technical" },
	{ 0x2400, 0x040, "Control Pictures" },
	{ 0x2440, 0x020, "OCR" },
	{ 0x2460, 0x0a0, "Enclosed Alphanumerics" },
	{ 0x2500, 0x080, "Box Drawing" },
	{ 0x2580, 0x020, "Block Elements" },
	{ 0x25a0, 0x060, "Geometric Shapes" },
	{ 0x2600, 0x080, "Miscellaneous Symbols" },
	{ 0x2700, 0x0c0, "Dingbats" },
	{ 0x3000, 0x040, "CJK Symbols and Punctuation" },
	{ 0x3040, 0x060, "Hiragana" },
	{ 0x30a0, 0x060, "Katakana" },
	{ 0xff60, 0x040, "Halfwidth Katakana" },
	{ 0xfe30, 0x020, "CJK Compatibility Forms" },
	{ 0xfe50, 0x020, "Small Form Variants" },
	{ 0xffe0, 0x010, "Fullwidth Symbol Variants" },
	{ 0xfff0, 0x010, "Specials" },
	{ 0, 0, NULL }
};

struct namemap encodingnames[] = {
    {"Latin1 (iso8859-1)", em_iso8859_1 },
    {"Latin0 (iso8859-15)", em_iso8859_15 },
    {"Latin2 (iso8859-2)", em_iso8859_2 },
    {"Latin3 (iso8859-3)", em_iso8859_3 },
    {"Latin4 (iso8859-4)", em_iso8859_4 },
    {"Latin5 (iso8859-9)", em_iso8859_9 },
    {"Latin6 (iso8859-10)", em_iso8859_10 },
    {"Latin7 (iso8859-13)", em_iso8859_13 },
    {"Latin8 (iso8859-14)", em_iso8859_14 },
    {"Latin10 (iso8859-16)", em_iso8859_16 },
    {"Cyrillic (iso8859-5)", em_iso8859_5 },
    {"Cyrillic (koi8-r)", em_koi8_r },
    {"Arabic (iso8859-6)", em_iso8859_6 },
    {"Greek (iso8859-7)", em_iso8859_7 },
    {"Hebrew (iso8859-8)", em_iso8859_8 },
    {"Thai (iso8859-11)", em_iso8859_11 },
    {"KataKana (jis201)", em_jis201 },
    {"Windows Latin1 extended", em_win },
    {"Macintosh Latin", em_mac },
    {"Kanji (jis208)", em_jis208 },
    {"Kanji (jis212)", em_jis212 },
    {"Hangul (ksc5601)", em_ksc5601 },
    {"Han (gb2312)", em_gb2312 },
    {"Big5 (Taiwan)", em_big5 },
    {"Unicode", em_unicode },
    {"Symbol", em_symbol },
    {"Zapf Dingbats", em_zapfding },
    {"Adobe Standard", em_user },
    {"-", -1 },
    {"Arabic", em_max+16 },
    {"Arabic Ligatures A1", em_max+17 },
    {"Arabic Ligatures A2", em_max+18 },
    {"Arabic Ligatures A3", em_max+19 },
    {"Arabic Ligatures B", em_max+20 },
    {"Armenian", em_max+12 },
    {"Armenian Ligatures", em_max+13 },
    {"Arrows", em_max+56 },
    {"Bengali", em_max+24 },
    {"Block Elements", em_max+63 },
    {"Box Drawing", em_max+62 },
    {"Canadian Syllabics A", em_max+43 },
    {"Canadian Syllabics B", em_max+44 },
    {"Canadian Syllabics C", em_max+45 },
    {"Cherokee", em_max+42 },
    {"CJK Compatibility Forms", em_max+71 },
    {"CJK Symbols and Punctuation", em_max+67 },
    {"Combining Diacritics", em_max+8 },
    {"Combining Symbol Diacritics", em_max+53 },
    {"Control Pictures", em_max+59 },
    {"Currency Symbols", em_max+52 },
    {"Cyrillic", em_max+11 },
    {"Devanagari", em_max+23 },
    {"Dingbats", em_max+66 },
    {"Enclosed Alphanumerics", em_max+61 },
    {"Ethiopic A", em_max+40 },
    {"Ethiopic B", em_max+41 },
    {"Fullwidth Symbol Variants", em_max+73 },
    {"General Punctuation", em_max+50 },
    {"Geometric Shapes", em_max+64 },
    {"Georgian", em_max+37 },
    {"Greek & Coptic", em_max+9 },
    {"Greek Additional Extensions", em_max+10 },
    {"Gujarati", em_max+26 },
    {"Gurmukhi", em_max+25 },
    {"Halfwidth Katakana", em_max+70 },
    {"Hangul Jamo", em_max+38 },
    {"Hangul Jamo Halfwidth", em_max+39 },
    {"Hebrew", em_max+14 },
    {"Hebrew Ligatures", em_max+15 },
    {"Hiragana", em_max+68 },
    {"IPA Extensions", em_max+3 },
    {"Kannada", em_max+30 },
    {"Katakana", em_max+69 },
    {"Khmer", em_max+48 },
    {"Latin Additional Extensions", em_max+4 },
    {"Latin Fullwidth", em_max+6 },
    {"Latin Extended A", em_max+1 },
    {"Latin Extended B", em_max+2 },
    {"Latin Ligatures", em_max+5 },
    {"Letterlike Symbols", em_max+54 },
    {"Lao", em_max+34 },
    {"Malayalam", em_max+31 },
    {"Mathematical Operators", em_max+57 },
    {"Miscellaneous Symbols", em_max+65 },
    {"Miscellaneous Technical", em_max+58 },
    {"Mongolian", em_max+49 },
    {"Myanmar", em_max+36 },
    {"Number Forms", em_max+55 },
    {"OCR", em_max+60 },
    {"Ogham", em_max+46 },
    {"Oriya", em_max+27 },
    {"Punctuation", em_max+50 },
    {"Runic", em_max+47 },
    {"Sinhala", em_max+32 },
    {"Small Form Variants", em_max+72 },
    {"Spacing Modifier Letters", em_max+7 },
    {"Specials", em_max+74 },
    {"Super & Sub scripts", em_max+51 },
    {"Syriac", em_max+21 },
    {"Tamil", em_max+28 },
    {"Telugu", em_max+29 },
    {"Thaana", em_max+22 },
    {"Thai", em_max+33 },
    {"Tibetan", em_max+35 },
    /* {"Latin Extended A", em_max+1 }, */
    /* {"Latin Extended B", em_max+2 }, */
    /* {"IPA Extensions", em_max+3 }, */
    /* {"Latin Additional Extensions", em_max+4 }, */
    /* {"Latin Ligatures", em_max+5 }, */
    /* {"Latin Fullwidth", em_max+6 }, */
    /* {"Spacing Modifier Letters", em_max+7 }, */
    /* {"Combining Diacritics", em_max+8 }, */
    /* {"Greek & Coptic", em_max+9 }, */
    /* {"Greek Additional Extensions", em_max+10 }, */
    /* {"Cyrillic", em_max+11 }, */
    /* {"Armenian", em_max+12 }, */
    /* {"Armenian Ligatures", em_max+13 }, */
    /* {"Hebrew", em_max+14 }, */
    /* {"Hebrew Ligatures", em_max+15 }, */
    /* {"Arabic", em_max+16 }, */
    /* {"Arabic Ligatures A1", em_max+17 }, */
    /* {"Arabic Ligatures A2", em_max+18 }, */
    /* {"Arabic Ligatures A3", em_max+19 }, */
    /* {"Arabic Ligatures B", em_max+20 }, */
    /* {"Syriac", em_max+21 }, */
    /* {"Thaana", em_max+22 }, */
    /* {"Devanagari", em_max+23 }, */
    /* {"Bengali", em_max+24 }, */
    /* {"Gurmukhi", em_max+25 }, */
    /* {"Gujarati", em_max+26 }, */
    /* {"Oriya", em_max+27 }, */
    /* {"Tamil", em_max+28 }, */
    /* {"Telugu", em_max+29 }, */
    /* {"Kannada", em_max+30 }, */
    /* {"Malayalam", em_max+31 }, */
    /* {"Sinhala", em_max+32 }, */
    /* {"Thai", em_max+33 }, */
    /* {"Lao", em_max+34 }, */
    /* {"Tibetan", em_max+35 }, */
    /* {"Myanmar", em_max+36 }, */
    /* {"Georgian", em_max+37 }, */
    /* {"Hangul Jamo", em_max+38 }, */
    /* {"Hangul Jamo Halfwidth", em_max+39 }, */
    /* {"Ethiopic A", em_max+40 }, */
    /* {"Ethiopic B", em_max+41 }, */
    /* {"Cherokee", em_max+42 }, */
    /* {"Canadian Syllabics A", em_max+43 }, */
    /* {"Canadian Syllabics B", em_max+44 }, */
    /* {"Canadian Syllabics C", em_max+45 }, */
    /* {"Ogham", em_max+46 }, */
    /* {"Runic", em_max+47 }, */
    /* {"Khmer", em_max+48 }, */
    /* {"Mongolian", em_max+49 }, */
    /* {"General Punctuation", em_max+50 }, */
    /* {"Super & Sub scripts", em_max+51 }, */
    /* {"Currency Symbols", em_max+52 }, */
    /* {"Combining Symbol Diacritics", em_max+53 }, */
    /* {"Letterlike Symbols", em_max+54 }, */
    /* {"Number Forms", em_max+55 }, */
    /* {"Arrows", em_max+56 }, */
    /* {"Mathematical Operators", em_max+57 }, */
    /* {"Miscellaneous Technical", em_max+58 }, */
    /* {"Control Pictures", em_max+59 }, */
    /* {"OCR", em_max+60 }, */
    /* {"Enclosed Alphanumerics", em_max+61 }, */
    /* {"Box Drawing", em_max+62 }, */
    /* {"Block Elements", em_max+63 }, */
    /* {"Geometric Shapes", em_max+64 }, */
    /* {"Miscellaneous Symbols", em_max+65 }, */
    /* {"Dingbats", em_max+66 }, */
    /* {"CJK Symbols and Punctuation", em_max+67 }, */
    /* {"Hiragana", em_max+68 }, */
    /* {"Katakana", em_max+69 }, */
    /* {"Halfwidth Katakana", em_max+70 }, */
    /* {"CJK Compatibility Forms", em_max+71 }, */
    /* {"Small Form Variants", em_max+72 }, */
    /* {"Fullwidth Symbol Variants", em_max+73 }, */
    /* {"Specials", em_max+74 }, */
    { NULL, 0 }};

static int mapFromIndex(int i) {
return( encodingnames[i].map );
}

static void InsChrRedraw(void) {
    GRect r;

    r.x = 0; r.width = inschr.width;
    /* it would be ybase, except we added the Page: field which needs to be updated */
    r.y = GDrawPointsToPixels(inschr.icw,90); r.height = inschr.height-r.y;
    GDrawRequestExpose(inschr.icw,&r,false);
}

static void InsChrXorChar(GWindow pixmap, int x, int y) {
    GRect rct, old;

    rct.x = x*inschr.spacing+1; rct.width = inschr.spacing-1;
    rct.y = inschr.ybase+y*inschr.spacing+1; rct.height = inschr.spacing-1;
    GDrawPushClip(pixmap, &rct, &old);
    GDrawSetDifferenceMode(pixmap);
    GDrawFillRect(pixmap, &rct, COLOR_WHITE);
    GDrawPopClip(pixmap, &old);
}

static void InsChrSetNextPrev() {

    if ( inschr.icw==NULL )
return;
    if ( inschr.map<em_first2byte || inschr.map>em_max ) {
	inschr.pageable = false;
	GGadgetSetEnabled(GWidgetGetControl(inschr.icw,INSCHR_Next),false);
	GGadgetSetEnabled(GWidgetGetControl(inschr.icw,INSCHR_Prev),false);
    } else if ( inschr.map==em_unicode ) {
	inschr.pageable = true;
	GGadgetSetEnabled(GWidgetGetControl(inschr.icw,INSCHR_Next),inschr.page<255);
	GGadgetSetEnabled(GWidgetGetControl(inschr.icw,INSCHR_Prev),inschr.page>0);
    } else if ( inschr.map==em_big5 ) {
	inschr.pageable = true;
	GGadgetSetEnabled(GWidgetGetControl(inschr.icw,INSCHR_Next),inschr.page<0xf9);
	GGadgetSetEnabled(GWidgetGetControl(inschr.icw,INSCHR_Prev),inschr.page>0xa1);
    } else {
	inschr.pageable = true;
	GGadgetSetEnabled(GWidgetGetControl(inschr.icw,INSCHR_Next),inschr.page<0x7e);
	GGadgetSetEnabled(GWidgetGetControl(inschr.icw,INSCHR_Prev),inschr.page>0x21);
    }
}

static long InsChrToUni(long val) {

    if ( inschr.map==em_unicode )
return( val );
    if ( inschr.map==em_iso8859_1 ) {	/* Our latin 1 map is really windows */
	if ( val>=0 && val<=255 )	/*  so do the real latin1->unicode */
return( val );
    } else if ( inschr.map<em_first2byte ) {
	if ( val>=0 && val<=255 )
return( unicode_from_alphabets[inschr.map+3][val] );
    } else if ( inschr.map>em_max ) {
	if ( val>=0 && val<unicode_ranges[inschr.map-em_max-1].len )
return( unicode_ranges[inschr.map-em_max-1].first+val );
    } else if ( inschr.map==em_big5 ) {
	if ( val<0xa100 || val>0xFfff )
return( -1 );
return( unicode_from_big5[val-0xa100] );
    } else {
	if ( (val>>8)<0x21 || (val>>8)>0x7e || (val&0xff)<0x21 || (val&0xff)>0x7e )
return( -1 );
	val = ((val>>8)-0x21)*94 + ((val&0xff)-0x21);
	if ( inschr.map==em_jis208 )
return( unicode_from_jis208[val]);
	else if ( inschr.map==em_jis212 )
return( unicode_from_jis212[val]);
	else if ( inschr.map==em_gb2312 )
return( unicode_from_gb2312[val]);
	else /*if ( inschr.map==em_ksc5601 )*/
return( unicode_from_ksc5601[val]);
    }
return( -1 );
}

static unichar_t InsChrMapChar(unichar_t ch) {
    if ( inschr.map==em_iso8859_1 ) {	/* Our latin 1 map is really windows */
return( ch );				/*  so do the real latin1->unicode */
    } else if ( inschr.map<em_first2byte ) {
return( unicode_from_alphabets[inschr.map+3][ch] );
    } else if ( inschr.map>em_max ) {
return( unicode_ranges[inschr.map-em_max-1].first+ch );
    } else if ( inschr.map==em_unicode ) {
return( (inschr.page<<8) + ch );
    } else if ( inschr.map==em_jis208 ) {
return(  unicode_from_jis208[(inschr.page-0x21)*94-0x21+ch]);
    } else if ( inschr.map==em_jis212 ) {
return(  unicode_from_jis212[(inschr.page-0x21)*94-0x21+ch]);
    } else if ( inschr.map==em_gb2312 ) {
return( unicode_from_gb2312[(inschr.page-0x21)*94-0x21+ch]);
    } else if ( inschr.map==em_ksc5601 ) {
return( unicode_from_ksc5601[(inschr.page-0x21)*94-0x21+ch]);
    } else if ( inschr.map==em_big5 ) {
return( unicode_from_big5[inschr.page*256-0xa100+ch] );
    }
return( 0x20 );
}

static long InsChrUniVal(void) {
    const unichar_t *str, *pt; unichar_t *pos;
    long val, val2;

    str = _GGadgetGetTitle(GWidgetGetControl(inschr.icw,INSCHR_Char));
    for ( pt = str; isspace(*pt); ++pt );
    if ( *pt=='\0' )
return( -1 );
    if ( *pt=='u' || *pt=='U' ) {
	++pt;
	if ( *pt=='+' ) ++pt;
	val = u_strtol(pt,&pos,16);
	if ( *pos!='\0' )
return( -1 );
return( val );
    } else if ( u_strchr(pt,',')!=NULL && inschr.map!=em_big5 &&
	    inschr.map>=em_first2byte && inschr.map<em_max ) {
	val = u_strtol(pt,&pos,10);
	while ( isspace(*pos)) ++pos;
	if ( *pos!=',' )
return( -1 );
	val2 = u_strtol(pos+1,&pos,10);
	if ( *pos!='\0' )
return( -1 );
	if ( inschr.map==em_unicode )
return( 256*val+val2 );
	val = 256*(val+0x20) + (val2+0x20);
    } else {
	if ( inschr.dsp_mode!=d_dec || (val=u_strtol(pt,&pos,10))<0 || *pos!='\0' )
	    val = u_strtol(pt,&pos,16);
	if ( *pos!='\0' )
return( -1 );
    }
return( InsChrToUni(val));
}

static int InsChrInCurrentEncoding(void) {
    int enable = false;
    long ch;
    long resch;

    if ( inschr.icw==NULL )
return(0);
    if ( (ch = InsChrUniVal())<=0 ) {
	enable = false;
	if ( inschr.map==em_unicode && ch==0 ) enable = true;
    } else if ( inschr.map>em_max ) {
	resch = ch - unicode_ranges[inschr.map-em_max-1].first;
	enable = (resch>=0 && resch<unicode_ranges[inschr.map-em_max-1].len);
    } else if ( inschr.map>=em_first2byte ) {
	int highch = (ch>>8);
	struct charmap2 *table2=NULL;
	unsigned short *plane;
	if ( inschr.map<=em_jis212 )
	    table2 = &jis_from_unicode;
	else if ( inschr.map==em_gb2312 )
	    table2 = &gb2312_from_unicode;
	else if ( inschr.map==em_ksc5601 )
	    table2 = &ksc5601_from_unicode;
	else if ( inschr.map==em_big5 )
	    table2 = &big5_from_unicode;

	if ( inschr.map==em_unicode )
	    enable = /*( (ch>>8)!=inschr.page )*/true;
	else if ( highch>=table2->first && highch<=table2->last &&
		 (plane = table2->table[highch])!=NULL &&
		 (resch = plane[ch&0xff])!=0 &&
		 ((inschr.map==em_jis212 && (resch&0x8000) && ((resch&~0x8000)>>8)!=inschr.page) ||
		  (inschr.map!=em_jis212 && !(resch&0x8000) && (resch!=inschr.page))) )
	    enable = true;
    } else {
	int highch = (ch>>8);
	struct charmap *table = NULL;
	unsigned char *plane;
	table = alphabets_from_unicode[inschr.map+3];	/* Skip the 3 asciis */
	if ( (highch>=table->first && highch<=table->last &&
		 (plane = table->table[highch])!=NULL &&
		 (resch=plane[ch&0xff])!=0 ))
	    enable = true;;
    }
return( enable );
}

static int InsChrFigureShow() {
    long ch;
    const unichar_t *str;
    int enable = true;

    if ( inschr.icw==NULL )
return(false);
    if ( InsChrInCurrentEncoding() )
	enable = true;
    else {
	str = _GGadgetGetTitle(GWidgetGetControl(inschr.icw,INSCHR_Char));
	if ( *str!='u' && *str!='U' )
	    enable = false;
	else if ( str[1]!='+' )
	    enable = false;
	else if ((ch = InsChrUniVal())<=0 || ch>=0x10000 )
	    enable = false;
    }

    if ( enable!=inschr.show_enabled ) {
	inschr.show_enabled = enable;
	GGadgetSetEnabled(GWidgetGetControl(inschr.icw,INSCHR_Show),enable);
    }
return( enable );
}

static int _InsChrSetSelChar(unichar_t ch, int refresh_page) {
    int inmap=0;
    int highch = (ch>>8);
    struct charmap *table = NULL;
    unsigned char *plane;
    struct charmap2 *table2 = NULL;
    unsigned short *plane2;
    unichar_t resch=0;
    char buffer[30];
    unichar_t ubuffer[30];

    if ( inschr.icw==NULL || inschr.hidden )
	inschr.sel_char = ch;
    else {
	/* Is it in the current map? */
	if ( inschr.map<em_first2byte )
	    table = alphabets_from_unicode[inschr.map+3];	/* Skip the 3 asciis */
	else if ( inschr.map<=em_jis212 )
	    table2 = &jis_from_unicode;
	else if ( inschr.map==em_gb2312 )
	    table2 = &gb2312_from_unicode;
	else if ( inschr.map==em_ksc5601 )
	    table2 = &ksc5601_from_unicode;
	else if ( inschr.map==em_big5 )
	    table2 = &big5_from_unicode;

	inmap = true;
	if ( inschr.map == em_unicode )
	    resch = ch;
	else if ( inschr.map>em_max ) {
	    resch = ch-unicode_ranges[inschr.map-em_max-1].first;
	    /* resch is unsigned so the <0 check is also >len */
	    if ( resch>unicode_ranges[inschr.map-em_max-1].len )
		inmap = false;
	} else if ( inschr.map<em_first2byte &&
		(highch<table->first || highch>table->last ||
		 (plane = table->table[highch])==NULL ||
		 (resch=plane[ch&0xff])==0 ))
	    inmap = false;
	else if ( inschr.map>=em_first2byte &&
		(highch<table2->first || highch>table2->last ||
		 (plane2 = table2->table[highch])==NULL ||
		 (resch=plane2[ch&0xff])==0 ))
	    inmap = false;
	if ( inmap && inschr.map==em_jis208 && (resch&0x8000))
	    inmap = false;
	else if ( inmap && inschr.map==em_jis212 && !(resch&0x8000))
	    inmap = false;
	if ( inschr.map==em_jis212 ) resch &= ~0x8000;
	if ( !inmap || inschr.dsp_mode==d_unicode || inschr.map>em_max )
	    sprintf( buffer,"U+0x%04x", ch );
	else if ( inschr.dsp_mode==d_dec )
	    sprintf( buffer,"%d", resch );
	else if ( inschr.dsp_mode==d_hex )
	    sprintf( buffer,inschr.map<em_first2byte?"0x%02x":"0x%04x", resch );
	else if ( inschr.map==em_unicode )
	    sprintf( buffer,"%d,%d", highch, ch&0xff );
	else
	    sprintf( buffer,"%d,%d", (resch>>8)-0x20, (resch&0xff)-0x20 );
	uc_strcpy(ubuffer, buffer);
	GGadgetSetTitle(GWidgetGetControl(inschr.icw,INSCHR_Char),ubuffer);
	if ( inschr.flash ) {
	    GDrawCancelTimer(inschr.flash_time);
	    InsChrXorChar(inschr.icw,inschr.x,inschr.y);
	    inschr.flash = false;
	}
	if ( inmap && refresh_page ) {
	    inschr.x = resch&0xf; inschr.y = (resch>>4)&0xf;
	    inschr.flash = true;
	    if ( inschr.map>=em_first2byte && inschr.map<em_max &&
		    inschr.page!=(resch>>8) ) {
		/* Set the page correctly */
		inschr.page = resch>>8;
		InsChrSetNextPrev();
		InsChrRedraw();
		InsChrSetNextPrev();
		InsChrFigureShow();
	    } else {
		InsChrXorChar(inschr.icw,inschr.x,inschr.y);
	    }
	    inschr.flash_time = GDrawRequestTimer(inschr.icw,500,0,NULL);
	}
	inschr.sel_char = EOF;
    }
return( inmap );
}

static void InsChrSetFormat(enum dsp_mode format) {
    if ( inschr.dsp_mode!=format ) {
	unichar_t ch = InsChrUniVal();
	inschr.dsp_mode = format;
	if ( ch>0 )
	    _InsChrSetSelChar(ch,false);
    }
}

static void InsChrSetCharset(int map) {
    int enabled;
    long ch;

    if ( map!=inschr.map ) {
	ch = InsChrUniVal();
	inschr.map = map;
	if ( inschr.map<em_first2byte ) {
	    inschr.page = 0;
	    enabled = false;
	} else if ( inschr.map>em_max ) {
	    inschr.page = 0;
	    enabled = false;
	} else if ( inschr.map==em_unicode ) {
	    inschr.page = 0;
	    enabled = true;
	} else if ( inschr.map==em_big5 ) {
	    inschr.page = 0xa1;
	    enabled = false;
	} else {
	    inschr.page = 0x21;
	    enabled = true;
	}
	if ( !enabled && inschr.dsp_mode==d_kuten ) {
	    GGadgetSetChecked(GWidgetGetControl(inschr.icw,INSCHR_Hex),true);
	    inschr.dsp_mode = d_hex;
	}
	GGadgetSetEnabled(GWidgetGetControl(inschr.icw,INSCHR_KuTen),enabled);
	InsChrSetNextPrev();
	InsChrRedraw();
	if ( ch>0 )		/* Must happen after we set display mode */
	    _InsChrSetSelChar(ch,false);
	InsChrFigureShow();
    }
}

static void InsChrCharset() {
    int map = mapFromIndex(GGadgetGetFirstListSelectedItem(GWidgetGetControl(inschr.icw,INSCHR_CharSet)));
    if ( map!=-1 )
	InsChrSetCharset(map);
    else {
	int i;
	for ( i=0; encodingnames[i].name!=NULL && encodingnames[i].map!=inschr.map; ++i );
	if ( encodingnames[i].name!=NULL )
	    GGadgetSelectOneListItem(GWidgetGetControl(inschr.icw,INSCHR_CharSet),i);
    }
}

static void InsChrShow(void) {
    long ch = InsChrUniVal();
    int i;

    if ( ch>0 ) {
	if ( !InsChrInCurrentEncoding() ) {
	    InsChrSetCharset(em_unicode);
	    for ( i=0; encodingnames[i].name!=NULL && strcmp(encodingnames[i].name,"Unicode")!=0; ++i );
	    if ( encodingnames[i].name!=NULL )
		GGadgetSelectOneListItem(GWidgetGetControl(inschr.icw,INSCHR_CharSet),i);
	}
	_InsChrSetSelChar(ch, true);
    }
}

static void InsChrInsert(void) {
    long ch = InsChrUniVal();
    GEvent e;

    e.type = et_char;
    e.w = GWidgetGetPreviousFocusTopWindow();
    if ( e.w==NULL || e.w==inschr.icw ) {
	GDrawBeep(NULL);
return;
    }
    e.u.chr.state = 0;
    e.u.chr.x = e.u.chr.y = -1;
    e.u.chr.keysym = 0;
    e.u.chr.chars[0] = ch;
    e.u.chr.chars[1] = '\0';
    GDrawPostEvent(&e);
}

static void InsChrExpose( GWindow pixmap, GRect *rect) {
    GRect old, r;
    int i, j;
    int highi, lowi, is94x94;

    if ( inschr.pageable ) {
	char buffer[20]; unichar_t ubuf[20];
	GDrawPushClip(pixmap,rect,&old);
	GDrawSetFont(pixmap,inschr.smallfont);
	if ( inschr.dsp_mode==d_hex || inschr.dsp_mode == d_unicode )
	    sprintf( buffer, "Page: 0x%02X", inschr.page );
	else
	    sprintf( buffer, "Page: %d", inschr.page );
	uc_strcpy(ubuf,buffer);
	GDrawDrawText(pixmap,GDrawPointsToPixels(pixmap,6),
		GDrawPointsToPixels(pixmap,90)+inschr.sas,
		ubuf, -1, 0x000000 );
	GDrawPopClip(pixmap,&old);
    }
    if ( rect->y+rect->height < inschr.ybase )
return;
    if ( rect->y < inschr.ybase ) {
	r = *rect; rect = &r;
	rect->height -= (inschr.ybase-rect->y);
	rect->y = inschr.ybase;
    }
    GDrawPushClip(pixmap,rect,&old);
    for ( i=0; i<17; ++i ) {
	GDrawDrawLine(pixmap,0,inschr.ybase+i*inschr.spacing,
		inschr.width,inschr.ybase+i*inschr.spacing, 0x000000);
	GDrawDrawLine(pixmap,i*inschr.spacing,inschr.ybase,
		i*inschr.spacing,inschr.height, 0x000000);
    }
    GDrawSetFont(pixmap,inschr.font);

    lowi = 0; highi = 16; is94x94 = false;
    if ( inschr.map>em_max ) {
	highi = (unicode_ranges[inschr.map-em_max-1].len+15)/16;
    } else if ( inschr.map==em_jis208 || inschr.map==em_jis212 ||
	    inschr.map==em_gb2312 || inschr.map==em_ksc5601 ) {
	lowi=2; highi = 8;
	is94x94 = true;
    } else if ( inschr.map==em_big5 ) {
	lowi=4; highi = 16;
    }
    for ( i=lowi; i<highi; ++i ) for ( j=0; j<16; ++j ) {
	unichar_t buf[1];
	int width;
	if ( j==15 && i==7 && is94x94 )
    break;
	if ( j==0 && i==2 && is94x94 )
    continue;
	if (( i==8 || i==9 ) &&
		(inschr.map<=em_iso8859_15 ||
		 (inschr.map==em_unicode && inschr.page==0 )))
    continue;
	buf[0] = InsChrMapChar(i*16+j);
	if ( buf[0]==0xad ) buf[0] = '-';	/* 0xad usually doesn't print */
	width = GDrawGetTextWidth(pixmap,buf,1);
	GDrawDrawText(pixmap,
		j*inschr.spacing+(inschr.spacing-width)/2,
		i*inschr.spacing+inschr.ybase+inschr.as+4,
		buf,1,0x000000);
    }
    if ( inschr.flash )
	InsChrXorChar(pixmap,inschr.x,inschr.y);
    GDrawPopClip(pixmap,&old);
}

static void InsChrTimer() {
    GDrawCancelTimer(inschr.flash_time);
    if ( inschr.flash ) {
	InsChrXorChar(inschr.icw,inschr.x,inschr.y);
	inschr.flash = false;
    }
}

static void InsChrMouseDown(GWindow gw, GEvent *event) {
    int x,y;
    int ch;
    char buffer[20]; unichar_t ubuffer[20];

    x= event->u.mouse.x/inschr.spacing;
    y= (event->u.mouse.y-inschr.ybase)/inschr.spacing;
    ch = 256*inschr.page + (y*16)+x;
    /* Is it on a border line? */
    if ( y<0 || x*inschr.spacing==event->u.mouse.x ||
	    y*inschr.spacing==event->u.mouse.y-inschr.ybase )
return;
    /* Is it a valid character in the current map? */
    if ( inschr.map>em_max ) {
	if ( ch>unicode_ranges[inschr.map-em_max-1].len )
return;
    } else if ( (ch<32 && inschr.map!=em_mac) || ch==127 ||
	    (inschr.map<em_first2byte && inschr.map!=em_mac &&
		inschr.map!=em_win && inschr.map!=em_symbol &&
		inschr.map!=em_user &&
		ch>=128 && ch<160) ||
	    (inschr.map>=em_first2byte && inschr.map<=em_gb2312 &&
		((ch&0xff)<0x21 || (ch&0xff)>0x7e)) )
return;
    inschr.mouse_down = true;
    inschr.mouse_in = true;
    inschr.x = x;
    inschr.y = y;

    InsChrXorChar(inschr.icw,x,y);

    if ( inschr.dsp_mode==d_unicode || inschr.map>em_max )
	sprintf( buffer, "U+%04lx", InsChrToUni(ch) );
    else if ( inschr.dsp_mode==d_hex )
	sprintf( buffer, inschr.map<em_first2byte?"0x%02x":"0x%04x", ch );
    else if ( inschr.dsp_mode==d_dec )
	sprintf( buffer, "%d", ch );
    else if ( inschr.map==em_unicode )
	sprintf( buffer, "%d,%d", ((ch&0xff)>>8), (ch&0xff) );
    else
	sprintf( buffer, "%d,%d", ((ch&0xff)>>8)-0x21, (ch&0xff)-0x21 );
    uc_strcpy(ubuffer,buffer);
    GGadgetSetTitle(GWidgetGetControl(inschr.icw,INSCHR_Char),ubuffer);
    InsChrFigureShow();
}

static void uc_annot_strncat(unichar_t *to, const char *from, int len) {
    register unichar_t ch;

    to += u_strlen(to);
    while ( (ch = utf8_ildb(&from)) != '\0' && --len>=0 )
	*(to++) = ch;
    *to = 0;
}

static void InsChrMouseMove(GWindow gw, GEvent *event) {
    int x, y;

    x= event->u.mouse.x/inschr.spacing;
    y= (event->u.mouse.y-inschr.ybase)/inschr.spacing;
    if ( !inschr.mouse_down && event->u.mouse.y>inschr.ybase ) {
	int uch = InsChrMapChar(16*y + x);
	static unichar_t space[650];
	char cspace[40];
	char *uniname;
	char *uniannot;

	if ( (uniname=unicode_name(uch))!=NULL ) {
	    uc_strncpy(space, uniname, 550);
	    if ( uch<=0xffff )
		sprintf( cspace, " U+%04X", uch );
	    else if ( uch<=0xfffff )
		sprintf( cspace, " 0x%05X", uch );
	    else
		sprintf( cspace, " 0x%06X", uch );
	    uc_strcpy(space+u_strlen(space),cspace);
	    free(uniname);
	} else {
	    if ( uch<160 )
		sprintf(cspace, "Control Char U+%04X ", uch);
	    else if ( uch>=0x3400 && uch<=0x4db5 )
		sprintf(cspace, "CJK Ideograph Extension A U+%04X ", uch);
	    else if ( uch>=0x4e00 && uch<=0x9fef )
		sprintf(cspace, "CJK Ideograph U+%04X ", uch);
	    else if ( uch>=0xAC00 && uch<=0xD7A3 )
		sprintf(cspace, "Hangul Syllable U+%04X ", uch);
	    else if ( uch>=0xD800 && uch<=0xDB7F )
 		sprintf(cspace, "Non Private Use High Surrogate U+%04X ", uch);
	    else if ( uch>=0xDB80 && uch<=0xDBFF )
		sprintf(cspace, "Private Use High Surrogate U+%04X ", uch);
	    else if ( uch>=0xDC00 && uch<=0xDFFF )
		sprintf(cspace, "Low Surrogate U+%04X ", uch);
	    else if ( uch>=0xE000 && uch<=0xF8FF )
		sprintf(cspace, "Private Use U+%04X ", uch);
	    else if ( uch>=0x20000 && uch<=0x2a6d6 )
		sprintf(cspace, "CJK Ideograph Extension B 0x%05X ", uch);
	    else if ( uch>=0x2a700 && uch<=0x2b734 )
		sprintf(cspace, "CJK Ideograph Extension C 0x%05X ", uch);
	    else if ( uch>=0x2b740 && uch<=0x2b81d )
		sprintf(cspace, "CJK Ideograph Extension D 0x%05X ", uch);
	    else if ( uch>=0x2b820 && uch<=0x2ceaf )
		sprintf(cspace, "CJK Ideograph Extension E 0x%05X ", uch);
	    else if ( uch>=0x2ceb0 && uch<=0x2ebe0 )
		sprintf(cspace, "CJK Ideograph Extension F 0x%05X ", uch);
	    else if ( uch>=0xf0000 && uch<=0xfffff )
		sprintf(cspace, "Supplementary Private Use Area-A 0x%05X ", uch);
	    else if ( uch>=0x100000 && uch<=0x10fffd )
		sprintf(cspace, "Supplementary Private Use Area-B 0x%06X ", uch);
	    else
		if ( uch<=0xffff )
		    sprintf(cspace, "Unencoded Unicode U+%04X ", uch);
		else if ( uch<=0xfffff )
		    sprintf(cspace, "Unencoded Unicode 0x%05X ", uch);
		else
		    sprintf(cspace, "Unencoded Unicode 0x%06X ", uch);
	    uc_strcpy(space,cspace);
	}
	if ( (uniannot=unicode_annot(uch))!=NULL ) {
	    int left = sizeof(space)/sizeof(space[0]) - u_strlen(space)-1;
	    if ( left>4 ) {
		uc_strcat(space,"\n");
		uc_annot_strncat(space, uniannot, left-2);
	    }
	    free(uniannot);
	}
	GGadgetPreparePopup(gw,space);
    } else if ( inschr.mouse_down ) {
	int in = true;
	if ( y<0 || x*inschr.spacing==event->u.mouse.x ||
		y*inschr.spacing==event->u.mouse.y-inschr.ybase ||
		x!=inschr.x || y!=inschr.y )
	    in = false;
	if ( in!=inschr.mouse_in ) {
	    InsChrXorChar(inschr.icw,inschr.x,inschr.y);
	    inschr.mouse_in = in;
	}
    }
}

static void InsChrMouseUp(GWindow gw, GEvent *event) {
    if ( !inschr.mouse_down )
return;
    InsChrMouseMove(gw,event);
    inschr.mouse_down = false;
    if ( inschr.mouse_in ) {
	InsChrXorChar(inschr.icw,inschr.x,inschr.y);

	if ( !(event->u.mouse.state&ksm_control) )
	    InsChrInsert();
    }
}

static int inschr_e_h(GWindow gw, GEvent *event) {
    GGadgetPopupExternalEvent(event);
    switch ( event->type ) {
      case et_close:
	inschr.hidden = true;
	GDrawSetVisible(gw,false);
      break;
      case et_expose:
	InsChrExpose(gw,&event->u.expose.rect);
      break;
      case et_mousedown:
	  InsChrMouseDown(gw,event);
      break;
      case et_mousemove:
	  InsChrMouseMove(gw,event);
      break;
      case et_mouseup:
	  InsChrMouseUp(gw,event);
      break;
      case et_char:
	  if ( event->u.chr.chars[0]=='\r' )
		InsChrShow();
      break;
      case et_timer:
	  InsChrTimer();
      break;
      case et_controlevent:
	if ( event->u.control.subtype == et_buttonactivate ) {
	    switch ( GGadgetGetCid(event->u.control.g)) {
	      case INSCHR_Close:
		inschr.hidden = true;
		GDrawSetVisible(gw,false);
	      break;
	      case INSCHR_Next:
		++inschr.page;
		InsChrSetNextPrev();
		InsChrRedraw();
	      break;
	      case INSCHR_Prev:
		--inschr.page;
		InsChrSetNextPrev();
		InsChrRedraw();
	      break;
	      case INSCHR_Show:
		InsChrShow();
	      break;
	      case INSCHR_Insert:
		InsChrInsert();
	      break;
	    }
	} else if ( event->u.control.subtype == et_radiochanged ) {
	    int cid = GGadgetGetCid(event->u.control.g);
	    InsChrSetFormat(cid==INSCHR_Hex?d_hex:
			    cid==INSCHR_Dec?d_dec:
			    cid==INSCHR_Unicode?d_unicode:
				    d_kuten);
	} else if ( event->u.control.subtype == et_textchanged ) {
	    if ( !InsChrFigureShow()) {
		/*GDrawBeep(NULL)*/
		;
            }
	} else if ( event->u.control.subtype == et_listselected ) {
	    InsChrCharset();
	}
      break;
    }
return( true );
}

static unichar_t inschar[] = { 'I', 'n', 's', 'e', 'r', 't', ' ', 'C', 'h', 'a', 'r', 'a', 'c', 't', 'e', 'r', '\0' };
void GWidgetCreateInsChar(void) {
    GTextInfo charsetnames[sizeof(encodingnames)/sizeof(struct namemap)];
    static GTextInfo labels[11] = {
        { (unichar_t *) "Character Set", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
        { (unichar_t *) "Character", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0,0,0,0,0,0, 1, 0, 0, '\0' },
        { (unichar_t *) "Hex", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0,0,0,0,0,0, 1, 0, 0, '\0' },
        { (unichar_t *) "Dec", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0,0,0,0,0,0, 1, 0, 0, '\0' },
        { (unichar_t *) "Unicode", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0,0,0,0,0,0, 1, 0, 0, '\0' },
        { (unichar_t *) "Ku Ten", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0,0,0,0,0,0, 1, 0, 0, '\0' },
        { (unichar_t *) "< Prev", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0,0,0,0,0,0, 1, 0, 0, '\0' },
        { (unichar_t *) "Next >", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0,0,0,0,0,0, 1, 0, 0, '\0' },
        { (unichar_t *) "Insert", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0,0,0,0,0,0, 1, 0, 0, '\0' },
        { (unichar_t *) "Close", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0,0,0,0,0,0, 1, 0, 0, '\0' },
        { (unichar_t *) "Show", NULL, COLOR_UNKNOWN, COLOR_UNKNOWN, NULL, NULL, 0,0,0,0,0,0, 1, 0, 0, '\0' },
    };
    static GGadgetCreateData gcd[] = {
        { GLabelCreate, { { 6, 6, 0, 0 }, NULL, 'e', 0, 0, 0, 0, &labels[0], { NULL }, gg_visible | gg_enabled | gg_pos_use0, NULL, NULL }, NULL, NULL },
        { GListButtonCreate, { { 6, 21, 168, 0 }, NULL, 'e', 0, 0, 0, INSCHR_CharSet, NULL, { NULL }, gg_visible | gg_enabled | gg_pos_use0, NULL, NULL }, NULL, NULL },
        { GLabelCreate, { { 6, 50, 0, 0 }, NULL, 'C', 0, 0, 0, 0, &labels[1], { NULL }, gg_visible | gg_enabled | gg_pos_use0, NULL, NULL }, NULL, NULL },
        /* gg_textarea_wrap means (here) that we should not invoke the InsChar Hook for selections */
        { GTextFieldCreate, { { 6, 64, 65, 0 }, NULL, 'C', 0, 0, 0, INSCHR_Char, NULL, { NULL }, gg_visible | gg_enabled | gg_pos_use0 | gg_textarea_wrap, NULL, NULL }, NULL, NULL },
        { GRadioCreate, { { 85, 48, 0, 0 }, NULL, 'H', 0, 0, 0, INSCHR_Hex, &labels[2], { NULL }, gg_visible | gg_enabled | gg_cb_on | gg_pos_use0, NULL, NULL }, NULL, NULL },
        { GRadioCreate, { { 85, 68, 0, 0 }, NULL, 'D', 0, 0, 0, INSCHR_Dec, &labels[3], { NULL }, gg_visible | gg_enabled | gg_pos_use0, NULL, NULL }, NULL, NULL },
        { GRadioCreate, { { 127, 48, 0, 0 }, NULL, 'U', 0, 0, 0, INSCHR_Unicode, &labels[4], { NULL }, gg_visible | gg_enabled | gg_pos_use0, NULL, NULL }, NULL, NULL },
        { GRadioCreate, { { 127, 68, 0, 0 }, NULL, 'K', 0, 0, 0, INSCHR_KuTen, &labels[5], { NULL }, gg_visible | gg_pos_use0, NULL, NULL }, NULL, NULL },
        { GButtonCreate, { { 73, 93, 50, 0 }, NULL, 'P', 0, 0, 0, INSCHR_Prev, &labels[6], { NULL }, gg_visible | gg_pos_use0, NULL, NULL }, NULL, NULL },
        { GButtonCreate, { { 137, 93, 50, 0 }, NULL, 'N', 0, 0, 0, INSCHR_Next, &labels[7], { NULL }, gg_visible | gg_pos_use0, NULL, NULL }, NULL, NULL },
        { GButtonCreate, { { 196-3, 6-3, 50+6, 0 }, NULL, 'I', 0, 0, 0, INSCHR_Insert, &labels[8], { NULL }, gg_visible | gg_enabled | gg_but_default | gg_pos_use0, NULL, NULL }, NULL, NULL },
        { GButtonCreate, { { 196, 36, 50, 0 }, NULL, 'l', 0, 0, 0, INSCHR_Close, &labels[9], { NULL }, gg_visible | gg_enabled | gg_but_cancel | gg_pos_use0, NULL, NULL }, NULL, NULL },
        { GButtonCreate, { { 196, 64, 50, 0 }, NULL, 'S', 0, 0, 0, INSCHR_Show, &labels[10], { NULL }, gg_visible | gg_pos_use0, NULL, NULL }, NULL, NULL },
        GGADGETCREATEDATA_EMPTY
    };
#define keyboard_width 15
#define keyboard_height 9
    static unsigned char keyboard_bits[] = {
       0xff, 0x7f, 0x01, 0x40, 0x55, 0x55, 0x01, 0x40, 0xad, 0x5a, 0x01, 0x40,
       0xd5, 0x55, 0x01, 0x40, 0xff, 0x7f};
    GRect pos;
    GWindowAttrs wattrs;
    int i;
    FontRequest rq;
    int as, ds, ld;
    static int inited= false;

    if ( !inited ) {
	inituninameannot();
	inited = true;
    }
    if ( inschr.icw!=NULL ) {
	inschr.hidden = false;
	GDrawSetVisible(inschr.icw,true);
	GDrawRaise(inschr.icw);
    } else {
	memset(charsetnames,'\0',sizeof(charsetnames));
	for ( i=0; encodingnames[i].name!=NULL; ++i ) {
	    if ( *encodingnames[i].name=='-' )
		charsetnames[i].line = true;
	    else {
		charsetnames[i].text = (unichar_t *) (encodingnames[i].name);
		charsetnames[i].text_is_1byte = true;
	    }
	}
	gcd[1].gd.u.list = charsetnames;

	inschr.spacing = GDrawPointsToPixels(NULL,16);
	inschr.ybase = GDrawPointsToPixels(NULL,123);
	pos.x = pos.y = 0;
	inschr.width = pos.width = 16*inschr.spacing+1;
	inschr.height = pos.height = inschr.ybase + pos.width;
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_isdlg|wam_notrestricted|wam_icon;
	wattrs.event_masks = 0xffffffff;
	wattrs.cursor = ct_pointer;
	wattrs.window_title = inschar;
	wattrs.is_dlg = true;
	wattrs.not_restricted = true;
	wattrs.icon = GDrawCreateBitmap(NULL,keyboard_width,keyboard_height,keyboard_bits);
	inschr.icw = GDrawCreateTopWindow(NULL,&pos,inschr_e_h,&inschr,&wattrs);
	GGadgetsCreate(inschr.icw,gcd);

	memset(&rq,0,sizeof(rq));
	rq.utf8_family_name = copy(GResourceFindString("InsChar.Family"));
	if ( rq.utf8_family_name==NULL )
	    rq.utf8_family_name = SANS_UI_FAMILIES;
	rq.point_size = /*15*/12;
	rq.weight = 400;
	rq.style = 0;
	inschr.font = GDrawInstanciateFont(inschr.icw,&rq);
	GDrawWindowFontMetrics(inschr.icw,inschr.font,&as, &ds, &ld);
	inschr.as = as;
	rq.point_size = 8;
	inschr.smallfont = GDrawInstanciateFont(inschr.icw,&rq);
	GDrawWindowFontMetrics(inschr.icw,inschr.smallfont,&as, &ds, &ld);
	inschr.sas = as;

	GDrawSetVisible(inschr.icw,true);
    }
    if ( inschr.sel_char > 0 )
	_InsChrSetSelChar(inschr.sel_char,true);
    else
	InsChrFigureShow();
}

void GInsCharSetChar(unichar_t ch) {
    _InsChrSetSelChar(ch,true);
}
