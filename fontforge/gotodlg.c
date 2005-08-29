/* Copyright (C) 2000-2005 by George Williams */
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
#include <utype.h>
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include <ustring.h>

struct unicoderange {
    char *name;		/* The range's name */
    int32 first, last, defined;
    			/* The first codepoint, last codepoint in the range */
			/*  and a codepoint which actually has a character */
			/*  associated with it */
} unicoderange[] = {
    /* { "Unicode", 0, 0x10ffff, ' ' }, */
    { "Unicode Basic Multilingual Plane", 0, 0xffff, ' ' },
    { "Basic Multilingual Plane", 0, 0xffff, ' ' },
    { "Alphabetic", 0, 0x1fff, 'A' },
    { "C0 Control Character", 0, ' '-1, 0 },
    { "NUL, Default Character", 0, 0, 0 },
    { "Basic Latin", ' ', 0x7e, 'A' },
    { "Delete Character", 0x7f, 0x7f, 0x7f },
    { "C1 Control Character", 0x80, 0x9f, 0x80 },
    { "Latin-1 Supplement", 0xa0, 0xff, 0xc0 },
    { "Latin Extended-A", 0x100, 0x17f, 0x100 },
    { "Latin Extended-B", 0x180, 0x24f, 0x180 },
    { "IPA Extensions", 0x250, 0x2af, 0x250 },
    { "Spacing Modifier Letters", 0x2b0, 0x2ff, 0x2b0 },
    { "Combining Diacritical Marks", 0x300, 0x36f, 0x300 },
    { "Greek", 0x370, 0x3ff, 0x391 },
    { "Cyrillic", 0x400, 0x4ff, 0x410 },
    { "Cyrillic Supplement", 0x500, 0x52f, 0x500 },
    { "Armenian", 0x530, 0x58f, 0x531 },
    { "Hebrew", 0x590, 0x5ff, 0x5d0 },
    { "Arabic", 0x600, 0x6ff, 0x627 },
    { "Syriac", 0x700, 0x74f, 0x710 },
    { "N'ko", 0x750, 0x77f, -1 },	/* Don't know defined point */
    { "Thaana", 0x780, 0x7bf, -1 },
    { "Avestan", 0x7c0, 0x7ff, -1 },
    { "Phoenician", 0x800, 0x81f, -1 },
    { "Aramaic", 0x820, 0x83f, -1 },
    { "Pahlavi", 0x840, 0x87f, -1 },
    { "Tifinagh", 0x880, 0x8af, -1 },
    { "Samaritan", 0x8b0, 0x8cf, -1 },
    { "Mandaic", 0x8d0, 0x8ff, -1 },
    { "Devangari", 0x900, 0x97f, 0x905 },
    { "Bengali", 0x980, 0x9ff, 0x985 },
    { "Gurmukhi", 0xa00, 0xa7f, 0xa05 },
    { "Gujarati", 0xa80, 0xaff, 0xa85 },
    { "Oriya", 0xb00, 0xb7f, 0xb05 },
    { "Tamil", 0xb80, 0xbff, 0xb85 },
    { "Telugu", 0xc00, 0xc7f, 0xc05 },
    { "Kannada", 0xc80, 0xcff, 0xc85 },
    { "Malayalam", 0xd00, 0xd7f, 0xd05 },
    { "Sinhala", 0xd80, 0xdff, 0xd85 },
    { "Thai", 0xe00, 0xe7f, 0xe01 },
    { "Lao", 0xe80, 0xeff, 0xe81 },
    { "Tibetan", 0xf00, 0xfff, 0xf00 },
    { "Myanmar", 0x1000, 0x109f, 0x1000 },
    { "Georgian", 0x10a0, 0x10ff, 0x10d0 },
    { "Hangul Jamo, Choseong", 0x1100, 0x115f, 0x1100 },
    { "Hangul Jamo, Jungseong", 0x1160, 0x11a7, 0x1161 },
    { "Hangul Jamo, Jongseong", 0x11a8, 0x11ff, 0x11a8 },
    { "Ethiopic", 0x1200, 0x137f, -1 },
    { "Ethiopic Extended", 0x1380, 0x139f, -1 },
    { "Cherokee", 0x13a0, 0x13ff, 0x13a0 },
    { "Unified Canadian Aboriginal Syllabics", 0x1400, 0x167f, -1 },
    { "Ogham", 0x1680, 0x139f, 0x1680 },
    { "Runic", 0x16a0, 0x13ff, 0x16a0 },
    { "Tagalog", 0x1700, 0x171f, -1 },
    { "Hanunóo", 0x1720, 0x173f, -1 },
    { "Buhid", 0x1740, 0x175f, -1 },
    { "Tagbanwa", 0x1760, 0x177f, -1 },
    { "Khmer", 0x1780, 0x17af, -1 },
    { "Mongolian", 0x17b0, 0x17ff, -1 },
    { "Cham", 0x1800, 0x18ff, -1 },
    { "Limbu", 0x1900, 0x194f, -1 },
    { "Tai Le", 0x1950, 0x197f, -1 },
    { "Viêt Thái", 0x1980, 0x19cf, -1 },
    { "New Tai Lu", 0x1a00, 0x1a5f, -1 },
    { "Lanna", 0x1a80, 0x1adf, -1 },
    { "Batak", 0x1b00, 0x1b3f, -1 },
    { "Buginese", 0x1b40, 0x1b5f, -1 },
    { "Javanese", 0x1b80, 0x1bcf, -1 },
    { "Meithei/Manipuri", 0x1c00, 0x1c5f, -1 },
    { "Lepcha", 0x1c80, 0x1cbf, -1 },
    { "Kayah Li", 0x1cc0, 0x1cff, -1 },
    { "Phonetic Extensions", 0x1d00, 0x1dff, 0x1d02 },
    { "Latin Extended Additional", 0x1e00, 0x1eff, 0x1e00 },
    { "Greek Extended", 0x1f00, 0x1fff, 0x1f00 },
    { "Symbols", 0x2000, 0x2bff, 0x2000 },
    { "General Punctuation", 0x2000, 0x206f, 0x2000 },
    { "Super and Sub scripts", 0x2070, 0x209f, 0x2070 },
    { "Currency Symbols", 0x20a0, 0x20cf, 0x20ac },
    { "Combining marks for Symbols", 0x20d0, 0x20ff, 0x20d0 },
    { "Letterlike Symbols", 0x2100, 0x214f, 0x2100 },
    { "Number Forms", 0x2150, 0x218f, 0x2153 },
    { "Arrows", 0x2190, 0x21ff, 0x2190 },
    { "Mathematical Symbols", 0x2200, 0x22ff, 0x2200 },
    { "Technical Symbols", 0x2300, 0x23ff, 0x2300 },
    { "Control Pictures", 0x2400, 0x243f, 0x2400 },
    { "OCR", 0x2440, 0x245f, 0x2440 },
    { "Enclosed Alphanumerics", 0x2460, 0x24ff, 0x2460 },
    { "Box Drawing", 0x2500, 0x257f, 0x2500 },
    { "Block Elements", 0x2580, 0x259f, 0x2580 },
    { "Geometric Shapes", 0x25a0, 0x25ff, 0x25a0 },
    { "Miscellaneous Symbols", 0x2600, 0x267f, 0x2600 },
    { "Dingbats", 0x2700, 0x27bf, 0x2701 },
    {  "Zapf Dingbats", 0x2700, 0x27bf, 0x2701 },		/* Synonym */
    { "Miscellaneous Math Symbols-A", 0x27c0, 0x27df, -1 },
    {  "Math Miscellaneous Symbols-A", 0x27c0, 0x27df, -1 },
    { "Supplemental Arrows-A", 0x27e0, 0x27ff, -1 },
    {  "Arrows Supplemental-A", 0x27e0, 0x27ff, -1 },
    { "Braille Patterns", 0x2800, 0x28ff, 0x2800 },
    { "Supplemental Arrows-B", 0x2900, 0x297f, -1 },
    {  "Arrows Supplemental-B", 0x2900, 0x297f, -1 },
    { "Miscellaneous Math Symbols-B", 0x2980, 0x29ff, -1 },
    {  "Math Miscellaneous Symbols-B", 0x2980, 0x29ff, -1 },
    { "Supplemental Math Operators", 0x2a00, 0x2aff, -1 },
    {  "Math Supplemental Operators", 0x2a00, 0x2aff, -1 },
    { "Supplemental Symbols", 0x2b00, 0x2bff, -1 },
    {  "Symbols Supplemental", 0x2b00, 0x2bff, -1 },
    { "Alphabetic Extended", 0x2c00, 0x2fff, -1 },
    { "Glagolitic", 0x2c00, 0x2c5f, -1 },
    { "Coptic", 0x2c80, 0x2cff, -1 },
    { "Ethiopic Extended", 0x2d00, 0x2d5f, -1 },
    { "Ol Cemet'", 0x2d80, 0x2daf, -1 },
    { "Sorang Sng.", 0x2db0, 0x2ddf, -1 },
    { "Siloti Nagri", 0x2e00, 0x2e3f, -1 },
    { "Varang Kshiti", 0x2e40, 0x2e7f, -1 },
    { "CJK Radicals", 0x2e80, 0x2eff, -1 },
    { "Kanqxi Radicals", 0x2f00, 0x2fdf, -1 },
    { "IDC", 0x2fe0, 0x2fef, -1 },
    { "CJK Phonetics and Symbols", 0x3000, 0x33ff, 0x3000 },
    { "CJK Symbols and Punctuation", 0x3000, 0x303f, 0x3000 },
    { "Hiragana", 0x3040, 0x309f, 0x3041 },
    { "Katakana", 0x30a0, 0x30ff, 0x30a1 },
    { "Bopomofo", 0x3100, 0x312f, 0x3105 },
    { "Hangul Compatibility Jamo", 0x3130, 0x318f, 0x3131 },
    { "Kanbun", 0x3190, 0x319f, 0x3190 },
    { "Enclosed CJK Letters and Months", 0x3200, 0x32ff, 0x3200 },
    {  "CJK Enclosed Letters and Months", 0x3200, 0x32ff, 0x3200 },
    { "CJK Compatibility", 0x3300, 0x33ff, 0x3300 },
    { "CJK Unified Ideographs Extension A", 0x3400, 0x4dff, -1 },
    { "Yijing Hexagram Symbols", 0x4db0, 0x4dff, -1 },
    { "CJK Unified Ideographs", 0x4e00, 0x9fff, 0x4e00 },
    { "Yi", 0xa000, 0xabff, -1 },
    { "Yi Radicals", 0xa490, 0xa4af, -1 },
    { "Yi Extensions", 0xa500, 0xa93f, -1 },
    { "Pahawh Hmong", 0xa900, 0xa93f, -1 },
    { "Chakma", 0xa980, 0xa9cf, -1 },
    { "Newari", 0xaa00, 0xaa5f, -1 },
    { "Siddham", 0xaa80, 0xaacf, -1 },
    { "Phags-pa", 0xab00, 0xab5f, -1 },
    { "Hangul Syllables", 0xac00, 0xd7a3, 0xac00 },
    { "High Surrogate", 0xd800, 0xdbff, -1 },		/* No characters defined */
    {  "Surrogate High", 0xd800, 0xdbff, -1 },
    { "Surrogate High, Non Private Use", 0xd800, 0xdb7f, -1 },
    { "Surrogate High, Private Use", 0xdb80, 0xdbff, -1 },
    { "Low Surrogate", 0xdc00, 0xdfff, -1 },
    { "Private/Corporate Use", 0xe000, 0xf8ff, 0xe000 },
    { "Private Use", 0xe000, 0xe0ff, 0xe000 },
	/* Boundary between private and corporate use is not fixed */
	/*  these should be safe... */
    { "Corporate Use", 0xf500, 0xf8ff, 0xf730 },
    { "MicroSoft Symbol Area", 0xf000, 0xf0ff, 0xf020 },
    { "CJK Compatibility Ideographs", 0xf900, 0xfaff, 0xf900 },
    { "Alphabetic Presentation Forms", 0xfb00, 0xfb4f, 0xfb00 },
    { "Latin Ligatures", 0xfb00, 0xfb05, 0xfb00 },
    { "Armenian Ligatures", 0xfb13, 0xfb17, 0xfb13 },
    { "Hebrew Ligatures/Pointed Letters", 0xfb1e, 0xfb4f, 0xfb2a },
    { "Arabic Presentation Forms A", 0xfb50, 0xfdff, 0xfb50 },
    { "Combining half marks", 0xfe20, 0xfe2f, 0xfe20 },
    { "CJK Compatibility Forms", 0xfe30, 0xfe4f, 0xfe30 },
    { "Small Form Variants", 0xfe50, 0xfe6f, 0xfe50 },
    { "Arabic Presentation Forms B", 0xfe70, 0xfefe, 0xfe70 },
    { "Byte Order Mark", 0xfeff, 0xfeff, 0xfeff },
    { "Half and Full Width Forms", 0xff00, 0xffef, 0xff01 },
    { "Latin Full Width Forms", 0xff01, 0xff5e, 0xff01 },
    { "KataKana Half Width Forms", 0xff61, 0xff9f, 0xff61 },
    { "Hangul Jamo Half Width Forms", 0xffa0, 0xffdc, 0xffa0 },
    { "Specials", 0xfff0, 0xfffd, 0xfffd },
    { "Not a Unicode Character", 0xfffe, 0xfffe, 0xfffe },
    { "Signature Mark", 0xffff, 0xffff, 0xffff },
/* End of BMP, Start of SMP */
    { "Unicode Supplementary Multilingual Plane", 0x10000, 0x1ffff, -1 },
    { "Supplementary Multilingual Plane", 0x10000, 0x1ffff, -1 },
    { "Aegean scripts", 0x10000, 0x102ff, 0x10000 },
    { "Linear B Syllabary", 0x10000, 0x1007f, 0x10000 },
    { "Linear B Ideograms", 0x10080, 0x100df, 0x10080 },
    { "Aegean numbers", 0x10100, 0x1003f, 0x10100 },
    { "Linear A", 0x10180, 0x102cf, 0x10180 },
    { "Alphabetic and syllabic LTR scripts", 0x10300, 0x107ff, -1 },
    { "Old Italic", 0x10300, 0x1032f, 0x10300 },
    { "Gothic", 0x10330, 0x1034f, 0x10330 },
    { "Old Permic", 0x10350, 0x1037f, 0x10350 },
    { "Ugaritic", 0x10380, 0x1039f, -1 },
    { "Deseret", 0x10400, 0x1044f, -1 },
    { "Shavian", 0x10450, 0x1047f, -1 },
    { "Osmanya", 0x10480, 0x104a9, -1 },
    { "Pollard", 0x104b0, 0x104d9, -1 },
    { "Alphabetic and syllabic RTL scripts", 0x10800, 0x10fff, -1 },
    { "Cypriot", 0x10800, 0x1083f, 0x10800 },
    { "Meriotic", 0x10840, 0x1085f, 0x10840 },
    { "Balti", 0x10860, 0x1087f, -1 },
    { "Kharosthi", 0x10a00, 0x10a5f, -1 },
    { "Brahmic scripts", 0x11000, 0x117ff, -1 },
    { "African and other syllabic scripts", 0x11800, 0x11fff, -1 },
    { "Scripts for invented languages", 0x12000, 0x127ff, -1 },
    { "Tengwar (Tolkien Elvish)", 0x12000, 0x1207f, 0x12000 },
    { "Cirth (Tolkien Runic)", 0x12080, 0x120ff, 0x12080 },
    { "Blissymbols", 0x12200, 0x124ff, -1 },
    { "Cuneiform and other Near Eastern Scripts", 0x12800, 0x12fff, -1 },
    { "Undeciphered scripts", 0x13000, 0x137ff, -1 },
    { "North American ideographs and pictograms", 0x13800, 0x13fff, -1 },
    { "Egyptian and Mayan hieroglyphs", 0x14000, 0x16bff, -1 },
    { "Sumerian pictograms", 0x16c00, 0x16fff, -1 },
    { "Large Asian Scripts", 0x17000, 0x1b5ff, -1 },
    { "Notational systems", 0x1d000, 0x1ffd, -1 },
    { "Byzantine Musical Symbols", 0x1d000, 0x1d0ff },
    { "Musical Symbols", 0x1d100, 0x1d1ff },
    { "Mathematical Alphanumeric Symbols", 0x1d400, 0x1d7ff },
/* End of SMP, Start of SIP */
    { "Unicode Supplementary Ideographic Plane", 0x20000, 0x2ffff, -1 },
    { "Supplementary Ideographic Plane", 0x20000, 0x2ffff, -1 },
    { "CJK Unified Ideographs Extension B", 0x20000, 0x2a6d6, -1 },
    { "CJK Compatibility Ideographs Supplement", 0x2f800, 0x2fa1f, -1 },
/* End of SIP, Start of SSP */
    { "Unicode Supplementary Special-purpose Plane", 0xe0000, 0xeffff, -1 },
    { "Supplementary Special-purpose Plane", 0xe0000, 0xeffff, -1 },
    { "Tag characters", 0xe0000, 0xe007f, -1 },
    { "Variation Selectors", 0xe0110, 0xe01ff, -1 },
/* End of SSP */
    { "Supplementary Private Use Area-A", 0xf0000, 0xfffff, -1 },
    { "Supplementary Private Use Area-B", 0x100000, 0x10ffff, -1 },
    { NULL }
};

struct unicoderange specialnames[] = {
    { NULL }
};

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

static int alpha(const void *_t1, const void *_t2) {
    const GTextInfo *t1 = _t1, *t2 = _t2;

return( strcmp((char *) (t1->text),(char *) (t2->text)));
}

static GTextInfo *AvailableRanges(SplineFont *sf,EncMap *map) {
    GTextInfo *ret = gcalloc(sizeof(unicoderange)/sizeof(unicoderange[0])+2,sizeof(GTextInfo));
    int i, cnt, ch, pos;

    for ( i=cnt=0; unicoderange[i].name!=NULL; ++i ) {
	ch = unicoderange[i].defined==-1 ? unicoderange[i].first : unicoderange[i].defined;
	pos = SFFindSlot(sf,map,ch,NULL);
	if ( pos!=-1 ) {
	    ret[cnt].text = (unichar_t *) unicoderange[i].name;
	    ret[cnt].text_is_1byte = true;
	    ret[cnt++].userdata = (void *) pos;
	}
    }
    qsort(ret,cnt,sizeof(GTextInfo),alpha);
return( ret );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

int NameToEncoding(SplineFont *sf,EncMap *map,const unichar_t *uname) {
    int enc, uni, i;
    char *end, *dot=NULL, *freeme=NULL;
    char *name;

    if ( uname[1]==0 && uname[0]>=256 )
return( SFFindSlot(sf,map,uname[0],NULL));

    name = cu_copy(uname);
    enc = uni = -1;
	
    while ( 1 ) {
	enc = SFFindSlot(sf,map,-1,name);
	if ( enc!=-1 ) {
	    free(name);
return( enc );
	}
	if ( (*name=='U' || *name=='u') && name[1]=='+' ) {
	    uni = strtol(name+2,&end,16);
	    if ( *end!='\0' )
		uni = -1;
	} else if ( name[0]=='u' && name[1]=='n' && name[2]=='i' ) {
	    uni = strtol(name+3,&end,16);
	    if ( *end!='\0' )
		uni = -1;
	} else if ( name[0]=='g' && name[1]=='l' && name[2]=='y' && name[3]=='p' && name[4]=='h' ) {
	    int orig = strtol(name+5,&end,10);
	    if ( *end!='\0' )
		orig = -1;
	    if ( orig!=-1 )
		enc = map->backmap[orig];
	} else if ( isdigit(*name)) {
	    enc = strtoul(name,&end,0);
	    if ( *end!='\0' )
		enc = -1;
	    if ( map->remap!=NULL && enc!=-1 ) {
		struct remap *remap = map->remap;
		while ( remap->infont!=-1 ) {
		    if ( enc>=remap->firstenc && enc<=remap->lastenc ) {
			enc += remap->infont-remap->firstenc;
		break;
		    }
		    ++remap;
		}
	    }
	} else {
	    if ( enc==-1 ) {
		uni = UniFromName(name,sf->uni_interp,map->enc);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
		if ( uni<0 ) {
		    for ( i=0; specialnames[i].name!=NULL; ++i )
			if ( strcmp(name,specialnames[i].name)==0 ) {
			    uni = specialnames[i].first;
		    break;
			}
		}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
		if ( uni<0 && name[1]=='\0' )
		    uni = name[0];
	    }
	}
	if ( enc>=map->enccount || enc<0 )
	    enc = -1;
	if ( enc==-1 && uni!=-1 )
	    enc = SFFindSlot(sf,map,uni,NULL);
	if ( enc!=-1 && uni==-1 ) {
	    int gid = map->map[enc];
	    if ( gid!=-1 && sf->glyphs[gid]!=NULL )
		uni = sf->glyphs[gid]->unicodeenc;
	    else if ( map->enc->is_unicodebmp || map->enc->is_unicodefull )
		uni = enc;
	}
	if ( dot!=NULL ) {
	    free(name);
	    if ( uni==-1 )
return( -1 );
	    if ( uni<0x600 || uni>0x6ff )
return( -1 );
	    if ( strmatch(dot,".begin")==0 || strmatch(dot,".initial")==0 )
		uni = ArabicForms[uni-0x600].initial;
	    else if ( strmatch(dot,".end")==0 || strmatch(dot,".final")==0 )
		uni = ArabicForms[uni-0x600].final;
	    else if ( strmatch(dot,".medial")==0 )
		uni = ArabicForms[uni-0x600].medial;
	    else if ( strmatch(dot,".isolated")==0 )
		uni = ArabicForms[uni-0x600].isolated;
	    else
return( -1 );
return( SFFindSlot(sf,map,uni,NULL) );
	} else 	if ( enc!=-1 ) {
	    free(name);
return( enc );
	}
	dot = strrchr(name,'.');
	if ( dot==NULL )
return( -1 );
	freeme = copyn(name,dot-name);
	free(name);
	name = freeme;
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
typedef struct gotodata {
    SplineFont *sf;
    EncMap *map;
    GWindow gw;
    int ret, done;
    GTextInfo *ranges;
} GotoData;

#define CID_Name 1000

static int Goto_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    GotoData *d;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	d->done = true;
    }
return( true );
}

static int Goto_OK(GGadget *g, GEvent *e) {
    GWindow gw;
    GotoData *d;
    const unichar_t *ret;
    int i;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	d = GDrawGetUserData(gw);
	ret = _GGadgetGetTitle(GWidgetGetControl(gw,CID_Name));
	for ( i=0; d->ranges[i].text!=NULL; ++i ) {
	    if ( uc_strcmp(ret,(char *) d->ranges[i].text)==0 ) {
		d->ret = (int) d->ranges[i].userdata;
	break;
	    }
	}
	if ( d->ret==-1 ) {
	    d->ret = NameToEncoding(d->sf,d->map,ret);
	    if ( d->ret<0 || d->ret>=d->map->enccount )
		d->ret = -1;
	    if ( d->ret==-1 ) {
		char *temp=cu_copy(ret);
#if defined(FONTFORGE_CONFIG_GDRAW)
		GWidgetPostNoticeR(_STR_Goto,_STR_CouldntfindGlyph,temp);
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_notice(_("Goto"),_("Could not find the glyph: %.70s"),temp);
#endif
		free(temp);
	    } else
		d->done = true;
	} else
	    d->done = true;
    }
return( true );
}

static int goto_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	GotoData *d = GDrawGetUserData(gw);
	d->done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

int GotoChar(SplineFont *sf,EncMap *map) {
    int enc = -1;
    unichar_t *ret;

    if ( map->enc->only_1byte ) {
	/* In one byte encodings don't bother with the range list. It won't */
	/*  have enough entries to be useful */
	ret = GWidgetAskStringR(_STR_Goto,NULL,_STR_EnternameofGlyph);
	if ( ret==NULL )
return(-1);
	enc = NameToEncoding(sf,map,ret);
	if ( enc<0 || enc>=map->enccount )
	    enc = -1;
	if ( enc==-1 )
#if defined(FONTFORGE_CONFIG_GDRAW)
	    GWidgetPostNoticeR(_STR_Goto,_STR_CouldntfindGlyphU,ret);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_notice(_("Goto"),_("Could not find the glyph: %.70s"),ret);
#endif
	free(ret);
return( enc );
    } else {
	GRect pos;
	GWindow gw;
	GWindowAttrs wattrs;
	GGadgetCreateData gcd[9];
	GTextInfo label[9];
	static GotoData gd;
	GTextInfo *ranges = AvailableRanges(sf,map);
	int i, wid, temp;
	unichar_t ubuf[100];

	memset(&gd,0,sizeof(gd));
	gd.sf = sf;
	gd.map = map;
	gd.ret = -1;
	gd.ranges = ranges;

	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
#if defined(FONTFORGE_CONFIG_GDRAW)
	wattrs.window_title = GStringGetResource(_STR_Goto,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	wattrs.window_title = _("Goto");
#endif
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,170));
	pos.height = GDrawPointsToPixels(NULL,90);
	gd.gw = gw = GDrawCreateTopWindow(NULL,&pos,goto_e_h,&gd,&wattrs);

	GDrawSetFont(gw,GGadgetGetFont(NULL));		/* Default gadget font */
#if defined(FONTFORGE_CONFIG_GDRAW)
	wid = GDrawGetTextWidth(gw,GStringGetResource(_STR_EnternameofGlyph,NULL),-1,NULL);
#elif defined(FONTFORGE_CONFIG_GTK)
	wid = GDrawGetTextWidth(gw,_("Enter the name of a glyph in the font"),-1,NULL);
#endif
	for ( i=0; ranges[i].text!=NULL; ++i ) {
	    uc_strncpy(ubuf,(char *) (ranges[i].text),sizeof(ubuf)/sizeof(ubuf[0])-1);
	    temp = GDrawGetTextWidth(gw,ubuf,-1,NULL);
	    if ( temp>wid )
		wid = temp;
	}
	if ( pos.width<wid+20 ) {
	    pos.width = wid+20;
	    GDrawResize(gw,pos.width,pos.height);
	}

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	label[0].text = (unichar_t *) _STR_EnternameofGlyph;
	label[0].text_in_resource = true;
	gcd[0].gd.label = &label[0];
	gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = 5; 
	gcd[0].gd.flags = gg_enabled|gg_visible;
	gcd[0].creator = GLabelCreate;

	gcd[1].gd.pos.x = 5; gcd[1].gd.pos.y = 17;  gcd[1].gd.pos.width = GDrawPixelsToPoints(NULL,wid);
	gcd[1].gd.flags = gg_enabled|gg_visible|gg_text_xim;
	gcd[1].gd.cid = CID_Name;
	gcd[1].gd.u.list = ranges;
	gcd[1].creator = GListFieldCreate;

	gcd[2].gd.pos.x = 20-3; gcd[2].gd.pos.y = 90-35-3;
	gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
	gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[2].text = (unichar_t *) _STR_OK;
	label[2].text_in_resource = true;
	gcd[2].gd.mnemonic = 'O';
	gcd[2].gd.label = &label[2];
	gcd[2].gd.handle_controlevent = Goto_OK;
	gcd[2].creator = GButtonCreate;

	gcd[3].gd.pos.x = -20; gcd[3].gd.pos.y = 90-35;
	gcd[3].gd.pos.width = -1; gcd[3].gd.pos.height = 0;
	gcd[3].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[3].text = (unichar_t *) _STR_Cancel;
	label[3].text_in_resource = true;
	gcd[3].gd.label = &label[3];
	gcd[3].gd.mnemonic = 'C';
	gcd[3].gd.handle_controlevent = Goto_Cancel;
	gcd[3].creator = GButtonCreate;

	gcd[4].gd.pos.x = 2; gcd[4].gd.pos.y = 2;
	gcd[4].gd.pos.width = pos.width-4; gcd[4].gd.pos.height = pos.height-2;
	gcd[4].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
	gcd[4].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);
	GDrawSetVisible(gw,true);
	while ( !gd.done )
	    GDrawProcessOneEvent(NULL);
	GDrawDestroyWindow(gw);
	free(ranges);
return( gd.ret );
    }
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
