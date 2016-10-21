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
#include "cvundoes.h"
#include "encoding.h"
#include "fontforge.h"
#include "fvimportbdf.h"
#include "lookups.h"
#include "splinefont.h"
#include "macenc.h"
#include "mm.h"
#include "namelist.h"
#include <chardata.h>
#include <utype.h>
#include <ustring.h>
#include <math.h>
#include <locale.h>
#include <gwidget.h>
#include "ttf.h"
#include "scripting.h"

char *SaveTablesPref;
int ask_user_for_cmap = false;

/* True Type is a really icky format. Nothing is together. It's badly described */
/*  much of the description is misleading */
/* Apple's version: */
/*  http://fonts.apple.com/TTRefMan/index.html */
/* MS's version: */
/*  http://www.microsoft.com/typography/tt/tt.htm */
/* An helpful but incomplete description is given at */
/*  http://www.truetype.demon.co.uk/ttoutln.htm */
/* For some things I looked at freetype's code to see how they did it */
/*  (I think only for what happens if !ARGS_ARE_XY) */
/*  http://freetype.sourceforge.net/ */
/* It grows on you though... now that I understand it better it seems better designed */
/*  but the docs remain in conflict. Sometimes badly so */

int prefer_cjk_encodings=false;

/* ************************************************************************** */
static struct ms_2_locales { const char *loc_name; int local_id; } ms_2_locals[] = {
    { "af", 0x436 },
    { "sq_AL", 0x41c },
    { "am", 0x45e },
    { "ar_SA", 0x401 },
    { "ar_IQ", 0x801 },
    { "ar_EG", 0xc01 },
    { "ar_LY", 0x1001 },
    { "ar_DZ", 0x1401 },
    { "ar_MA", 0x1801 },
    { "ar_TN", 0x1C01 },
    { "ar_OM", 0x2001 },
    { "ar_YE", 0x2401 },
    { "ar_SY", 0x2801 },
    { "ar_JO", 0x2c01 },
    { "ar_LB", 0x3001 },
    { "ar_KW", 0x3401 },
    { "ar_AE", 0x3801 },
    { "ar_BH", 0x3c01 },
    { "ar_QA", 0x4001 },
    { "hy", 0x42b },
    { "as", 0x44d },
    { "az", 0x42c },
    { "az", 0x82c },
    { "eu", 0x42d },
    { "be_BY", 0x423 },
    { "bn_IN", 0x445 },
    { "bn_BD", 0x845 },
    { "bg_BG", 0x402 },
    { "my", 0x455 },
    { "ca", 0x403 },
    { "km", 0x453 },
    { "zh_TW", 0x404 },		/* Trad */
    { "zh_CN", 0x804 },		/* Simp */
    { "zh_HK", 0xc04 },		/* Trad */
    { "zh_SG", 0x1004 },	/* Simp */
    { "zh_MO", 0x1404 },	/* Trad */
    { "hr", 0x41a },
    { "hr_BA", 0x101a },
    { "cs_CZ", 0x405 },
    { "da_DK", 0x406 },
    { "div", 0x465 },
    { "nl_NL", 0x413 },
    { "nl_BE", 0x813 },
    { "en_UK", 0x809 },
    { "en_US", 0x409 },
    { "en_CA", 0x1009 },
    { "en_AU", 0xc09 },
    { "en_NZ", 0x1409 },
    { "en_IE", 0x1809 },
    { "en_ZA", 0x1c09 },
    { "en_JM", 0x2009 },
    { "en", 0x2409 },
    { "en_BZ", 0x2809 },
    { "en_TT", 0x2c09 },
    { "en_ZW", 0x3009 },
    { "en_PH", 0x3409 },
    { "en_ID", 0x3809 },
    { "en_HK", 0x3c09 },
    { "en_IN", 0x4009 },
    { "en_MY", 0x4409 },
    { "et_EE", 0x425 },
    { "fo", 0x438 },
/* No language code for filipino */
    { "fa", 0x429 },
    { "fi_FI", 0x40b },
    { "fr_FR", 0x40c },
    { "fr_BE", 0x80c },
    { "fr_CA", 0xc0c },
    { "fr_CH", 0x100c },
    { "fr_LU", 0x140c },
    { "fr_MC", 0x180c },
    { "fr", 0x1c0c },		/* West Indes */
    { "fr_RE", 0x200c },
    { "fr_CD", 0x240c },
    { "fr_SN", 0x280c },
    { "fr_CM", 0x2c0c },
    { "fr_CI", 0x300c },
    { "fr_ML", 0x340c },
    { "fr_MA", 0x380c },
    { "fr_HT", 0x3c0c },
    { "fr_DZ", 0xe40c },	/* North African is most likely to be Algeria, possibly Tunisia */
    { "fy", 0x462 },
    { "gl", 0x456 },
    { "ka", 0x437 },
    { "de_DE", 0x407 },
    { "de_CH", 0x807 },
    { "de_AT", 0xc07 },
    { "de_LU", 0x1007 },
    { "de_LI", 0x1407 },
    { "el_GR", 0x408 },
    { "ga", 0x83c },
    { "gd", 0x43c },
    { "gn", 0x474 },
    { "gu", 0x447 },
    { "ha", 0x468 },
    { "he_IL", 0x40d },
    { "iw", 0x40d },		/* Obsolete name for Hebrew */
    { "hi", 0x439 },
    { "hu_HU", 0x40e },
    { "is_IS", 0x40f },
    { "id", 0x421 },
    { "in", 0x421 },		/* Obsolete name for Indonesean */
    { "iu", 0x45d },
    { "it_IT", 0x410 },
    { "it_CH", 0x810 },
    { "ja_JP", 0x411 },
    { "kn", 0x44b },
    { "ks_IN", 0x860 },
    { "kk", 0x43f },
    { "ky", 0x440 },
    { "km", 0x453 },
    { "kok", 0x457 },
    { "ko", 0x412 },
    { "ko", 0x812 },	/*Johab */
    { "lo", 0x454 },
    { "la", 0x476 },
    { "lv_LV", 0x426 },
    { "lt_LT", 0x427 },
    { "lt", 0x827 },	/* Classic */
    { "mk", 0x42f },
    { "ms", 0x43e },
    { "ms", 0x83e },
    { "ml", 0x44c },
    { "mt", 0x43a },
    { "mr", 0x44e },
    { "mn", 0x450 },
    { "ne_NP", 0x461 },
    { "ne_IN", 0x861 },
    { "no_NO", 0x414 },	/* Bokmal */
    { "no_NO", 0x814 },	/* Nynorsk */
    { "or", 0x448 },
    { "om", 0x472 },
    { "ps", 0x463 },
    { "pl_PL", 0x415 },
    { "pt_PT", 0x416 },
    { "pt_BR", 0x816 },
    { "pa_IN", 0x446 },
    { "pa_PK", 0x846 },
    { "qu_BO", 0x46b },
    { "qu_EC", 0x86b },
    { "qu_PE", 0xc6b },
    { "rm", 0x417 },
    { "ro_RO", 0x418 },
    { "ro_MD", 0x818 },
    { "ru_RU", 0x419 },
    { "ru_MD", 0x819 },
    { "smi", 0x43b },
    { "sa", 0x43b },
/* No language code for Sepedi */
    { "sr", 0xc1a },	/* Cyrillic */
    { "sr", 0x81a },	/* Latin */
    { "sd_IN", 0x459 },
    { "sd_PK", 0x859 },
    { "si", 0x45b },
    { "sk_SK", 0x41b },
    { "sl_SI", 0x424 },
    { "wen", 0x42e },
    { "es_ES", 0x40a },	/* traditional spanish */
    { "es_MX", 0x80a },
    { "es_ES", 0xc0a },	/* Modern spanish */
    { "es_GT", 0x100a },
    { "es_CR", 0x140a },
    { "es_PA", 0x180a },
    { "es_DO", 0x1c0a },
    { "es_VE", 0x200a },
    { "es_CO", 0x240a },
    { "es_PE", 0x280a },
    { "es_AR", 0x2c0a },
    { "es_EC", 0x300a },
    { "es_CL", 0x340a },
    { "es_UY", 0x380a },
    { "es_PY", 0x3c0a },
    { "es_BO", 0x400a },
    { "es_SV", 0x440a },
    { "es_HN", 0x480a },
    { "es_NI", 0x4c0a },
    { "es_PR", 0x500a },
    { "es_US", 0x540a },
    { "sutu", 0x430 },
    { "sw_KE", 0x441 },
    { "sv_SE", 0x41d },
    { "sv_FI", 0x81d },
    { "tl", 0x464 },
    { "tg", 0x464 },
    { "ta", 0x449 },
    { "tt", 0x444 },
    { "te", 0x44a },
    { "th", 0x41e },
    { "bo_CN", 0x451 },
    { "bo_BT", 0x451 },
    { "ti_ET", 0x473 },
    { "ti_ER", 0x873 },
    { "ts", 0x431 },
    { "tn", 0x432 },
    { "tr_TR", 0x41f },
    { "tk", 0x442 },
    { "uk_UA", 0x422 },
    { "ug", 0x480 },
    { "ur_PK", 0x420 },
    { "ur_IN", 0x820 },
    { "uz", 0x443 },	/* Latin */
    { "uz", 0x843 },	/* Cyrillic */
    { "ven", 0x433 },
    { "vi", 0x42a },
    { "cy", 0x452 },
    { "xh", 0x434 },
    { "yi", 0x43d },
    { "ji", 0x43d },	/* Obsolete Yiddish */
    { "yo", 0x46a },
    { "zu", 0x435 },
    { NULL, 0 }
};

int MSLanguageFromLocale(void) {
    const char *lang=NULL;
    int i, langlen;
    static const char *envs[] = { "LC_ALL", "LC_MESSAGES", "LANG", NULL };
    char langcountry[8], language[4];
    int langcode, langlocalecode;

    for ( i=0; envs[i]!=NULL; ++i ) {
	lang = getenv(envs[i]);
	if ( lang!=NULL ) {
	    langlen = strlen(lang);
	    if (( langlen>5 && lang[5]=='.' && lang[2]=='_' ) ||
		    (langlen==5 && lang[2]=='_' ) ||
		    (langlen==2) ||
		    (langlen==3))	/* Some obscure languages have a 3 letter code */
		/* I understand this language */
    break;
	}
    }
    if ( lang==NULL )
	lang = "en_US";
    strncpy(langcountry,lang,5); langcountry[5] = '\0';
    strncpy(language,lang,3); language[3] = '\0';
    if ( language[2]=='_' ) language[2] = '\0';
    langlen = strlen(language);

    langcode = langlocalecode = -1;
    for ( i=0; ms_2_locals[i].loc_name!=NULL; ++i ) {
	if ( strmatch(langcountry,ms_2_locals[i].loc_name)==0 ) {
	    langlocalecode = ms_2_locals[i].local_id;
	    langcode = langlocalecode&0x3ff;
    break;
	} else if ( strncmp(language,ms_2_locals[i].loc_name,langlen)==0 )
	    langcode = ms_2_locals[i].local_id&0x3ff;
    }
    if ( langcode==-1 )		/* Default to English */
	langcode = 0x9;
return( langlocalecode==-1 ? (langcode|0x400) : langlocalecode );
}
/* ************************************************************************** */

int getushort(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    if ( ch2==EOF )
return( EOF );
return( (ch1<<8)|ch2 );
}

int get3byte(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    if ( ch3==EOF )
return( EOF );
return( (ch1<<16)|(ch2<<8)|ch3 );
}

int32 getlong(FILE *ttf) {
    int ch1 = getc(ttf);
    int ch2 = getc(ttf);
    int ch3 = getc(ttf);
    int ch4 = getc(ttf);
    if ( ch4==EOF )
return( EOF );
return( (ch1<<24)|(ch2<<16)|(ch3<<8)|ch4 );
}

static int32 getoffset(FILE *ttf, int offsize) {
    if ( offsize==1 )
return( getc(ttf));
    else if ( offsize==2 )
return( getushort(ttf));
    else if ( offsize==3 )
return( get3byte(ttf));
    else
return( getlong(ttf));
}

real getfixed(FILE *ttf) {
    int32 val = getlong(ttf);
    int mant = val&0xffff;
    /* This oddity may be needed to deal with the first 16 bits being signed */
    /*  and the low-order bits unsigned */
return( (real) (val>>16) + (mant/65536.0) );
}

real get2dot14(FILE *ttf) {
    int32 val = getushort(ttf);
    int mant = val&0x3fff;
    /* This oddity may be needed to deal with the first 2 bits being signed */
    /*  and the low-order bits unsigned */
return( (real) ((val<<16)>>(16+14)) + (mant/16384.0) );
}

static Encoding *enc_from_platspec(int platform,int specific) {
    const char *enc;
    Encoding *e;

    enc = "Custom";
    if ( platform==0 ) {
	enc = "Unicode";
	if ( specific==4 )
	    enc = "UnicodeFull";
    } else if ( platform==1 ) {
	if ( specific==0 )
	    enc = "Mac";
	else if ( specific==1 )
	    enc = "Sjis";
	else if ( specific==2 )
	    enc = "Big5hkscs";		/* Or should we just guess big5? Both are wrong sometimes */
	else if ( specific==3 )
	    enc = "EUC-KR";
	else if ( specific==25 )
	    enc = "EUC-CN";
    } else if ( platform==2 ) {		/* obselete */
	if ( specific==0 )
	    enc = "ASCII";
	else if ( specific==1 )
	    enc = "Unicode";
	else if ( specific==2 )
	    enc = "ISO8859-1";
    } else if ( platform==3 ) {
	if ( specific==1 || specific==0 )	/* symbol (sp=0) is just unicode (PUA) */
	    enc = "Unicode";
	else if ( specific==2 )
	    enc = "Sjis";
	else if ( specific==3 )
	    enc = "EUC-CN";
	else if ( specific==4 )
	    enc = "Big5hkscs";
	else if ( specific==5 )
	    enc = "EUC-KR";
	else if ( specific==6 )
	    enc = "Johab";
	else if ( specific==10 )
	    enc = "UnicodeFull";
    } else if ( platform==7 ) {		/* Used internally in freetype, but */
	if ( specific==0 ) {		/*  there's no harm in looking for it */
	    enc = "AdobeStandard";	/*  even if it never happens */
	} else if ( specific==1 ) {
	    /* adobe_expert */
	    ;
	} else if ( specific==2 ) {
	    /* adobe_custom */
	    ;
	}
    }
    e = FindOrMakeEncoding(enc);
    if ( e==NULL ) {
	static int p = -1,s = -1;
	if ( p!=platform || s!=specific ) {
	    LogError( _("The truetype encoding specified by platform=%d specific=%d (which we map to %s) is not supported by your version of iconv(3).\n"),
		    platform, specific, enc );
	    p = platform; s = specific;
	}
    }
return( e );
}

static char *_readencstring(FILE *ttf,int offset,int len,
	int platform,int specific,int language) {
    long pos = ftell(ttf);
    unichar_t *str, *pt;
    char *ret;
    int i, ch;
    Encoding *enc;

    fseek(ttf,offset,SEEK_SET);

    if ( platform==1 ) {
	/* Mac is screwy, there are several different varients of MacRoman */
	/*  depending on the language, they didn't get it right when they  */
	/*  invented their script system */
	char *cstr, *cpt;
	cstr = cpt = malloc(len+1);
	for ( i=0; i<len; ++i )
	    *cpt++ = getc(ttf);
	*cpt = '\0';
	ret = MacStrToUtf8(cstr,specific,language);
	free(cstr);
    } else {
	enc = enc_from_platspec(platform,specific);
	if ( enc==NULL ) {
	  fseek(ttf, pos, SEEK_SET);
	  return( NULL );
	}
	if ( enc->is_unicodebmp ) {
	    str = pt = malloc((sizeof(unichar_t)/2)*len+sizeof(unichar_t));
	    for ( i=0; i<len/2; ++i ) {
		ch = getc(ttf)<<8;
		*pt++ = ch | getc(ttf);
	    }
	    *pt = 0;
	} else if ( enc->unicode!=NULL ) {
	    str = pt = malloc(sizeof(unichar_t)*len+sizeof(unichar_t));
	    for ( i=0; i<len; ++i )
		*pt++ = enc->unicode[getc(ttf)];
	    *pt = 0;
	} else if ( enc->tounicode!=NULL ) {
	    size_t inlen = len+1, outlen = sizeof(unichar_t)*(len+1);
	    char *cstr = malloc(inlen), *cpt;
	    ICONV_CONST char *in = cstr;
	    char *out;
	    for ( cpt=cstr, i=0; i<len; ++i )
		*cpt++ = getc(ttf);
	    str = malloc(outlen+sizeof(unichar_t));
	    out = (char *) str;
	    iconv(enc->tounicode,&in,&inlen,&out,&outlen);
	    out[0] = '\0'; out[1] = '\0';
	    out[2] = '\0'; out[3] = '\0';
	    free(cstr);
	} else {
	    str = uc_copy("");
	}
	ret = u2utf8_copy(str);
	free(str);
    }
    fseek(ttf,pos,SEEK_SET);
return( ret );
}

char *TTFGetFontName(FILE *ttf,int32 offset,int32 off2) {
    int i,num;
    int32 tag, nameoffset, namelength, stringoffset;
    int plat, spec, lang, name, len, off, val;
    int fullval, fullstr, fulllen, famval, famstr, famlen;
    Encoding *enc;
    int fullplat, fullspec, fulllang, famplat, famspec, famlang;
    int locale = MSLanguageFromLocale();
    int maclang = WinLangToMac(locale);
    long ttfFileSize;

    /* Determine file size to check table offset bounds */
    fseek(ttf,0,SEEK_END);
    ttfFileSize = ftell(ttf);

    fseek(ttf,offset,SEEK_SET);
    /* version = */ getlong(ttf);
    num = getushort(ttf);
    /* srange = */ getushort(ttf);
    /* esel = */ getushort(ttf);
    /* rshift = */ getushort(ttf);
    if ( num == EOF || feof(ttf) || num < 0 || num >= 0xFFFF)
        return( NULL );
    for ( i=0; i<num; ++i ) {
        tag = getlong(ttf);
        /* checksum = */ getlong(ttf);
        nameoffset = off2+getlong(ttf);
        namelength = getlong(ttf);
        if ( feof(ttf) )
            return( NULL );
        if ( tag==CHR('n','a','m','e'))
            break;
    }
    if ( i==num )
        return( NULL );
    if ( nameoffset+namelength > ttfFileSize )
        return( NULL );

    fseek(ttf,nameoffset,SEEK_SET);
    /* format = */ getushort(ttf);
    num = getushort(ttf);
    stringoffset = nameoffset+getushort(ttf);
    fullval = famval = 0;
    for ( i=0; i<num; ++i ) {
        plat = getushort(ttf);
        spec = getushort(ttf);
        lang = getushort(ttf);
        name = getushort(ttf);
        len = getushort(ttf);
        off = getushort(ttf);
        enc = enc_from_platspec(plat,spec);
        if ( enc==NULL )
            continue;
        val = 0;
        if ( plat==3 && !enc->is_custom && lang==locale )
            val = 15;
        else if ( plat==3 && !enc->is_custom && (lang&0xff)==(locale&0xff) )
            val = 14;
        else if ( (plat==0 || plat==1) && !enc->is_custom && lang==maclang )
            val = 13;
        /* Ok, that didn't work, how about an english name? */
        else if ( plat==3 && !enc->is_custom && lang==0x409 )
            val = 12;
        else if ( plat==3 && !enc->is_custom && (lang&0xff)==0x09 )
            val = 11;
        else if ( (plat==0 || plat==1) && !enc->is_custom && lang==0 )
            val = 10;
        /* failing that I'll take what I can get */
        else if ( !enc->is_custom )
            val = 1;
        if ( name==4 && val>fullval ) {
            fullval = val;
            fullstr = off;
            fulllen = len;
            fullplat = plat;
            fullspec = spec;
            fulllang = lang;
            if ( val==12 )
                break;
        } else if ( name==1 && val>famval ) {
            famval = val;
            famstr = off;
            famlen = len;
            famplat = plat;
            famspec = spec;
            famlang = lang;
        }
    }
    if ( fullval==0 ) {
        if ( famval==0 )
            return( NULL );
        fullstr = famstr;
        fulllen = famlen;
        fullplat = famplat;
        fullspec = famspec;
        fulllang = famlang;
    }
    return( _readencstring(ttf,stringoffset+fullstr,fulllen,fullplat,fullspec,fulllang));
}

/* Chooses which font to open from a TTC TrueType Collection font file.      */
/*                                                                           */
/* There are five ways that one enclosed font is selected:                   */
/*   1)  there is only one font enclosed, so we force defaulting to that one.*/
/*   2a) the filename has a font index appended, we choose that N'th font.   */
/*   2b) the filename has a font name appended, we try to match that name    */
/*           in list of discovered font names and select that named font.    */
/*   3)  the user is prompted with a list of all discovered font names, and  */
/*           asked to select one, and then that N'th font is chosen.         */
/*   4)  when there is no UI, then font index zero is used.                  */
/*                                                                           */
/* On failure and no font is chosen, returns false.                          */
/*                                                                           */
/* On success, true is returned.  The chosen font name (allocated) pointer   */
/*   is returned via 'chosenname'. Additionally, the file position is set    */
/*   pointing to the chosen TTF font offset table, ready for reading the     */
/*   TTF header.                                                             */
/*                                                                           */
/* Example filename strings with appended font selector:                     */
/*     ./tests/fonts/mingliu.windows.ttc(PMingLiU)                           */
/*     ./tests/fonts/mingliu.windows.ttc(1)                                  */
/*                                                                           */
/* 'offsets' is a list of file offsets to each enclosed TTF offset table.    */
/* 'names' is a list of font names as found in each enclosed name table.     */
/* 'names' is used to search for a matching font name, or to present as a    */
/*    list to the user via ff_choose() to select from.                       */
/*  Once the chosen font index is determined, offsets[choice] is used to     */
/*    call fseek() to position to the chosen TTF header offset table. Then   */
/*    the chosen font name is copied into 'chosenname'.                      */

static int PickTTFFont(FILE *ttf,char *filename,char **chosenname) {
    int32 *offsets, cnt, i, choice;
    char **names;
    char *pt, *lparen, *rparen;

    /* TTCF version = */ getlong(ttf);
    cnt = getlong(ttf);
    if ( cnt==1 ) {
	/* This is easy, don't bother to ask the user, there's no choice */
	int32 offset = getlong(ttf);
	fseek(ttf,offset,SEEK_SET);
        return( true );
    }

    offsets = malloc(cnt*sizeof(int32));
    for ( i=0; i<cnt; ++i )
	offsets[i] = getlong(ttf);
    names = malloc(cnt*sizeof(char *));
    for ( i=0; i<cnt; ++i ) {
	names[i] = TTFGetFontName(ttf,offsets[i],0);
        if ( names[i]==NULL ) 
            asprintf(&names[i], "<Unknown font name %d>", i+1);
    }
    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt = filename;
    /* Someone gave me a font "Nafees Nastaleeq(Updated).ttf" and complained */
    /*  that ff wouldn't open it */
    /* Now someone will complain about "Nafees(Updated).ttc(fo(ob)ar)" */
    if ( (lparen = strrchr(pt,'('))!=NULL &&
	    (rparen = strrchr(lparen,')'))!=NULL &&
	    rparen[1]=='\0' ) {
        char *find = copyn(lparen+1, rparen-lparen-1);
	for ( choice=cnt-1; choice>=0; --choice )
            if ( names[choice]!=NULL )
	        if ( strcmp(names[choice],find)==0 )
	            break;
	if ( choice==-1 ) {
	    char *end;
	    choice = strtol(find,&end,10);
	    if ( *end!='\0' )
		choice = -1;
            else if ( choice < 0 || choice >= cnt )
		choice = -1;
	}
	if ( choice==-1 ) {
	    char *fn = copy(filename);
	    fn[lparen-filename] = '\0';
	    ff_post_error(_("Not in Collection"),
/* GT: The user is trying to open a font file which contains multiple fonts and */
/* GT: has asked for a font which is not in that file. */
/* GT: The string will look like: <fontname> is not in <filename> */
		    _("%1$s is not in %2$.100s"),find,fn);
	    free(fn);
	}
	free(find);
    } else if ( no_windowing_ui )
	choice = 0;
    else
	choice = ff_choose(_("Pick a font, any font..."),(const char **) names,cnt,0,_("There are multiple fonts in this file, pick one"));
    if ( choice < -1 || choice >= cnt )
        choice = -1;
    if ( choice!=-1 ) {
        /* position file to start of the chosen TTF font header */
	fseek(ttf,offsets[choice],SEEK_SET);
	*chosenname = names[choice];
	names[choice] = NULL;
    }
    for ( i=0; i<cnt; ++i )
	free(names[i]);
    free(names);
    free(offsets);
return( choice!=-1);
}

static int PickCFFFont(char **fontnames) {
    unichar_t **names;
    int cnt, i, choice;

    for ( cnt=0; fontnames[cnt]!=NULL; ++cnt);
    names = calloc(cnt+1,sizeof(unichar_t *));
    for ( i=0; i<cnt; ++i )
	names[i] = uc_copy(fontnames[i]);
    if ( no_windowing_ui )
	choice = 0;
    else
	choice = ff_choose(_("Pick a font, any font..."),
	    (const char **) names,cnt,0,_("There are multiple fonts in this file, pick one"));
    for ( i=0; i<cnt; ++i )
	free(names[i]);
    free(names);
return( choice );
}

static void ParseSaveTablesPref(struct ttfinfo *info) {
    char *pt, *spt;
    int cnt;

    if (info->openflags & of_all_tables) {
        info->savecnt = info->numtables;
        info->savetab = calloc(info->savecnt,sizeof(struct savetab));
    } else {
        info->savecnt = 0;
        info->savetab = NULL;
        if ( SaveTablesPref==NULL || *SaveTablesPref=='\0' )
    return;
        for ( pt=SaveTablesPref, cnt=0; *pt; ++pt )
            if ( *pt==',' )
                ++cnt;
        info->savecnt = cnt+1;
        info->savetab = calloc(cnt+1,sizeof(struct savetab));
        for ( pt=spt=SaveTablesPref, cnt=0; ; ++pt ) {
            if ( *pt==',' || *pt=='\0' ) {
                uint32 tag;
                tag  = ( ( spt  <pt )? spt[0] : ' ' )<<24;
                tag |= ( ( spt+1<pt )? spt[1] : ' ' )<<16;
                tag |= ( ( spt+2<pt )? spt[2] : ' ' )<<8 ;
                tag |= ( ( spt+3<pt )? spt[3] : ' ' )    ;
                info->savetab[cnt++].tag = tag;
                if ( *pt )
                    spt = pt+1;
                else
        break;
            }
        }
    }
}

static uint32 regionchecksum(FILE *file, int start, int len) {
    uint32 sum = 0, chunk;

    fseek(file,start,SEEK_SET);
    if ( len!=-1 ) len=(len+3)>>2;
    while ( len==-1 || --len>=0 ) {
	chunk = getlong(file);
	if ( feof(file))
    break;
	sum += chunk;
    }
return( sum );
}

static void ValidateTTFHead(FILE *ttf,struct ttfinfo *info) {
    /* When doing font lint we want to check the ttf header and make */
    /*  sure all the offsets and lengths are valid, and the checksums */
    /*  match. Most of the time this is just extra work and we don't */
    /*  bather */
    uint32 restore_this_pos = ftell(ttf);
    struct tt_tables {
	uint32 tag;
	uint32 checksum;
	uint32 offset;
	uint32 length;
    } *tabs, temp;
    int i,j;
    uint32 file_len;
    int sr, es, rs, e_sr, e_es, e_rs;
    int hashead, hashhea, hasmaxp, masos2, haspost, hasname, hasos2;
    int hasloca, hascff, hasglyf;

    info->numtables = getushort(ttf);
    sr = getushort(ttf);
    es = getushort(ttf);
    rs = getushort(ttf);
    e_sr = (info->numtables<8?4:info->numtables<16?8:info->numtables<32?16:info->numtables<64?32:64)*16;
    e_es = (info->numtables<8?2:info->numtables<16?3:info->numtables<32?4:info->numtables<64?5:6);
    e_rs = info->numtables*16-e_sr;
    if ( e_sr!=sr || e_es!=es || e_rs!=rs ) {
	LogError( _("Unexpected values for binsearch header. Based on the number of tables I\n expect searchRange=%d (not %d), entrySel=%d (not %d) rangeShift=%d (not %d)\n"),
		e_sr, sr, e_es, es, e_rs, rs );
	info->bad_sfnt_header = true;
    }

    if ( info->numtables<=0 ) {
	LogError(_("An sfnt file must contain SOME tables, but this one does not."));
	info->bad_sfnt_header = true;
	fseek(ttf,restore_this_pos,SEEK_SET);
return;
    } else if ( info->numtables>1000 ) {
	LogError(_("An sfnt file may contain a large number of tables, but this one has over 1000\n and that seems like too many\n"));
	info->bad_sfnt_header = true;
	fseek(ttf,restore_this_pos,SEEK_SET);
return;
    }

    tabs = malloc(info->numtables*sizeof(struct tt_tables));

    for ( i=0; i<info->numtables; ++i ) {
	tabs[i].tag = getlong(ttf);
	tabs[i].checksum = getlong(ttf);
	tabs[i].offset = getlong(ttf);
	tabs[i].length = getlong(ttf);
	if ( i!=0 && tabs[i].tag<tabs[i-1].tag && !info->bad_sfnt_header ) {
	    LogError(_("Table tags should be in alphabetic order in the font header\n but '%c%c%c%c', appears after '%c%c%c%c'."),
		    tabs[i-1].tag>>24, tabs[i-1].tag>>16, tabs[i-1].tag>>8, tabs[i-1].tag,
		    tabs[i].tag>>24, tabs[i].tag>>16, tabs[i].tag>>8, tabs[i].tag );
	    info->bad_sfnt_header = true;
	}
    }
    fseek(ttf,0,SEEK_END);
    file_len = ftell(ttf);

    for ( i=0; i<info->numtables; ++i ) for ( j=i+1; j<info->numtables; ++j ) {
	if ( tabs[i].offset>tabs[j].offset ) {
	    temp = tabs[i];
	    tabs[i] = tabs[j];
	    tabs[j] = temp;
	}
    }
    for ( i=0; i<info->numtables-1; ++i ) {
	for ( j=i+1; j<info->numtables; ++j ) {
	    if ( tabs[i].tag==tabs[j].tag ) {
		LogError(_("Same table tag, '%c%c%c%c', appears twice in sfnt header"),
			tabs[i].tag>>24, tabs[i].tag>>16, tabs[i].tag>>8, tabs[i].tag );
		info->bad_sfnt_header = true;
	    }
	}
	if ( tabs[i].offset+tabs[i].length > tabs[i+1].offset ) {
	    LogError(_("Tables '%c%c%c%c' and '%c%c%c%c' overlap"),
		    tabs[i].tag>>24, tabs[i].tag>>16, tabs[i].tag>>8, tabs[i].tag,
		    tabs[j].tag>>24, tabs[j].tag>>16, tabs[j].tag>>8, tabs[j].tag );
	}
    }
    if ( tabs[i].offset+tabs[i].length > file_len ) {
	LogError(_("Table '%c%c%c%c' extends beyond end of file."),
		tabs[i].tag>>24, tabs[i].tag>>16, tabs[i].tag>>8, tabs[i].tag );
	info->bad_sfnt_header = true;
    }

    /* Checksums. First file as a whole, then each table */
    if ( regionchecksum(ttf,0,-1)!=0xb1b0afba ) {
	LogError(_("File checksum is incorrect."));
	info->bad_sfnt_header = true;
    }
    for ( i=0; i<info->numtables-1; ++i ) if ( tabs[i].tag!=CHR('h','e','a','d')) {
	if ( regionchecksum(ttf,tabs[i].offset,tabs[i].length)!=tabs[i].checksum ) {
	    LogError(_("Table '%c%c%c%c' has a bad checksum."),
		    tabs[i].tag>>24, tabs[i].tag>>16, tabs[i].tag>>8, tabs[i].tag );
	    info->bad_sfnt_header = true;
	}
    }

    hashead = hashhea = hasmaxp = masos2 = haspost = hasname = hasos2 = false;
    hasloca = hascff = hasglyf = false;
    for ( i=0; i<info->numtables-1; ++i ) {
	switch ( tabs[i].tag ) {
	  case CHR('c','v','t',' '):
	    if ( tabs[i].length&1 )
		LogError(_("Table '%c%c%c%c' has a bad length, must be even."),
			tabs[i].tag>>24, tabs[i].tag>>16, tabs[i].tag>>8, tabs[i].tag );
	  break;
	  case CHR('b','h','e','d'):	/* Fonts with bitmaps but no outlines get bhea */
	  case CHR('h','e','a','d'):
	    if ( tabs[i].length!=54 )
		LogError(_("Table '%c%c%c%c' has a bad length, must be 54 but is %d."),
			tabs[i].tag>>24, tabs[i].tag>>16, tabs[i].tag>>8, tabs[i].tag,
			tabs[i].length );
	    hashead = true;
	  break;
	  case CHR('h','h','e','a'):
	    hashhea = true;
	  case CHR('v','h','e','a'):
	    if ( tabs[i].length!=36 )
		LogError(_("Table '%c%c%c%c' has a bad length, must be 36 but is %d."),
			tabs[i].tag>>24, tabs[i].tag>>16, tabs[i].tag>>8, tabs[i].tag,
			tabs[i].length );
	  break;
	  case CHR('m','a','x','p'):
	    hasmaxp = true;
	    if ( tabs[i].length!=32 && tabs[i].length!=6 )
		LogError(_("Table '%c%c%c%c' has a bad length, must be 32 or 6 but is %d."),
			tabs[i].tag>>24, tabs[i].tag>>16, tabs[i].tag>>8, tabs[i].tag,
			tabs[i].length );
	  break;
	  case CHR('O','S','/','2'):
	    hasos2 = true;
	    if ( tabs[i].length!=78 && tabs[i].length!=86 && tabs[i].length!=96 )
		LogError(_("Table '%c%c%c%c' has a bad length, must be 78, 86 or 96 but is %d."),
			tabs[i].tag>>24, tabs[i].tag>>16, tabs[i].tag>>8, tabs[i].tag,
			tabs[i].length );
	  break;
	  case CHR('p','o','s','t'):
	    haspost = true;
	  break;
	  case CHR('n','a','m','e'):
	    hasname = true;
	  break;
	  case CHR('l','o','c','a'):
	    hasloca = true;
	  break;
	  case CHR('g','l','y','f'):
	    hasglyf = true;
	  break;
	  case CHR('C','F','F',' '):
	    hascff = true;
	  break;
          default:
          break;
	}
    }
    if ( !hashead )
	LogError(_("Missing required table: \"head\""));
    if ( !hashhea )
	LogError(_("Missing required table: \"hhea\""));
    if ( !hasmaxp )
	LogError(_("Missing required table: \"maxp\""));
    if ( !haspost )
	LogError(_("Missing required table: \"post\""));
    if ( !hasname )
	LogError(_("Missing required table: \"name\""));
    if ( hasglyf && !hasloca )
	LogError(_("Missing required table: \"loca\""));
    if ( !hasos2 )
	LogError(_("Missing \"OS/2\" table"));
    if ( !hasglyf && hasloca )
	LogError(_("Missing required table: \"glyf\""));
    if ( !hasglyf && !hascff )
	LogError(_("This font contains neither \"CFF \" nor \"glyf\"/\"loca\" tables"));

    free(tabs);
    fseek(ttf,restore_this_pos,SEEK_SET);
}
	    
static struct tablenames { uint32 tag; const char *name; } stdtables[] = {
    { CHR('a','c','n','t'), N_("accent attachment table") },
    { CHR('a','v','a','r'), N_("axis variation table") },
    { CHR('B','A','S','E'), N_("Baseline table (OT version)") },
    { CHR('b','d','a','t'), N_("bitmap data table (AAT version)") },
    { CHR('B','D','F',' '), N_("BDF bitmap properties table") },
    { CHR('b','h','e','d'), N_("bitmap font header table") },
    { CHR('b','l','o','c'), N_("bitmap location table (AAT version)") },
    { CHR('b','s','l','n'), N_("baseline table (AAT version)") },
    { CHR('C','F','F',' '), N_("PostScript font program (Compact Font Format)") },
    { CHR('C','I','D',' '), N_("Obsolete table for a type1 CID keyed font") },
    { CHR('c','m','a','p'), N_("character code mapping table") },
    { CHR('c','v','a','r'), N_("CVT variation table") },
    { CHR('c','v','t',' '), N_("control value table") },
    { CHR('D','S','I','G'), N_("digital signature table") },
    { CHR('E','B','D','T'), N_("bitmap data table (OT version)") },
    { CHR('E','B','L','C'), N_("bitmap location table (OT version)") },
    { CHR('E','B','S','C'), N_("embedded bitmap scaling control table") },
    { CHR('E','L','U','A'), N_("electronic end user license table") },
    { CHR('f','d','s','c'), N_("font descriptor table") },
    { CHR('f','e','a','t'), N_("layout feature table") },
    { CHR('F','e','a','t'), N_("SIL Graphite layout feature table") },
    { CHR('F','F','T','M'), N_("FontForge time stamp table") },
    { CHR('f','m','t','x'), N_("font metrics table") },
    { CHR('f','p','g','m'), N_("font program table") },
    { CHR('f','v','a','r'), N_("font variation table") },
    { CHR('g','a','s','p'), N_("grid-fitting and scan-conversion procedure table") },
    { CHR('G','D','E','F'), N_("glyph definition table") },
    { CHR('G','l','a','t'), N_("Graphite glyph attribute table") },
    { CHR('G','l','o','c'), N_("Graphite glyph location in Glat table") },
    { CHR('g','l','y','f'), N_("glyph outline table") },
    { CHR('G','P','O','S'), N_("glyph positioning table") },
    { CHR('g','v','a','r'), N_("glyph variation table") },
    { CHR('G','S','U','B'), N_("glyph substitution table") },
    { CHR('h','d','m','x'), N_("horizontal device metrics table") },
    { CHR('h','e','a','d'), N_("font header table") },
    { CHR('h','h','e','a'), N_("horizontal header table") },
    { CHR('h','m','t','x'), N_("horizontal metrics table") },
    { CHR('h','s','t','y'), N_("horizontal style table") },
    { CHR('j','u','s','t'), N_("justification table (AAT version)") },
    { CHR('J','S','T','F'), N_("justification table (OT version)") },
    { CHR('k','e','r','n'), N_("kerning table") },
    { CHR('l','c','a','r'), N_("ligature caret table") },
    { CHR('l','o','c','a'), N_("glyph location table") },
    { CHR('L','T','S','H'), N_("linear threshold table") },
    { CHR('M','A','T','H'), N_("math table") },
    { CHR('m','a','x','p'), N_("maximum profile table") },
    { CHR('M','M','S','D'), N_("Multi-Master table, obsolete") },
    { CHR('M','M','F','X'), N_("Multi-Master table, obsolete") },
    { CHR('m','o','r','t'), N_("metamorphosis table") },
    { CHR('m','o','r','x'), N_("extended metamorphosis table") },
    { CHR('n','a','m','e'), N_("name table") },
    { CHR('o','p','b','d'), N_("optical bounds table") },
    { CHR('O','S','/','2'), N_("OS/2 and Windows specific metrics table") },
    { CHR('P','C','L','T'), N_("PCL 5 data table") },
    { CHR('P','f','E','d'), N_("FontForge font debugging table") },
    { CHR('p','o','s','t'), N_("glyph name and PostScript compatibility table") },
    { CHR('p','r','e','p'), N_("control value program table") },
    { CHR('p','r','o','p'), N_("properties table") },
    { CHR('S','i','l','f'), N_("SIL Graphite rule table") },
    { CHR('S','i','l','l'), N_("(unspecified) SIL Graphite table") },
    { CHR('S','i','l','t'), N_("unknown SIL table") },
    { CHR('T','e','X',' '), N_("TeX table") },
    { CHR('t','r','a','k'), N_("tracking table") },
    { CHR('T','Y','P','1'), N_("Obsolete table for a type1 font") },
    { CHR('V','D','M','X'), N_("vertical device metrics table") },
    { CHR('v','h','e','a'), N_("vertical header table") },
    { CHR('v','m','t','x'), N_("vertical metrics table") },
    { CHR('V','O','R','G'), N_("vertical origin table") },
    { CHR('Z','a','p','f'), N_("glyph reference table") },
    { 0, NULL }
};

static int readttfheader(FILE *ttf, struct ttfinfo *info,char *filename,
	char **choosenname) {
    int i, j, k, offset, length, version;
    uint32 tag;
    int first = true;

    version=getlong(ttf);
    if ( version==CHR('t','t','c','f')) {
	/* TrueType font collection */
	info->is_ttc = true;
	if ( !PickTTFFont(ttf,filename,choosenname))
return( 0 );
	/* If they picked a font, then we should be left pointing at the */
	/*  start of the Table Directory for that font */
	info->one_of_many = true;
	version = getlong(ttf);
    }

    /* Apple says that 'typ1' is a valid code for a type1 font wrapped up in */
    /*  a truetype table structure, but gives no docs on what tables get used */
    /*  or how */ /* Turns out to be pretty simple */
    /* typ1 is used for both type1 fonts and CID type1 fonts, I don't think a version of 'CID ' is actually used */
    if ( version==CHR('t','y','p','1') || version==CHR('C','I','D',' ')) {
	LogError( _("Nifty, you've got one of the old Apple/Adobe type1 sfnts here\n") );
    } else if ( version!=0x00010000 && version!=CHR('t','r','u','e') &&
	    version!=0x00020000 &&	/* Windows 3.1 Chinese version used this version for some arphic fonts */
					/* See discussion on freetype list, july 2004 */
	    version!=CHR('O','T','T','O'))
return( 0 );			/* Not version 1 of true type, nor Open Type */

    if ( info->openflags & of_fontlint )
	ValidateTTFHead(ttf,info);

    info->numtables = getushort(ttf);
    /* searchRange = */ getushort(ttf);
    /* entrySelector = */ getushort(ttf);
    /* rangeshift = */ getushort(ttf);

    ParseSaveTablesPref(info);

    for ( i=0; i<info->numtables; ++i ) {
	tag = getlong(ttf);
	/* checksum */ getlong(ttf);
	offset = getlong(ttf);
	length = getlong(ttf);
        if ( offset+length > info->ttfFileSize ) {
	    LogError(_("Table '%c%c%c%c' extends beyond end of file and must be ignored."),
	    	            tag>>24, tag>>16, tag>>8, tag );
	    info->bad_sfnt_header = true;
            continue;
        }
#ifdef DEBUG
 printf( "%c%c%c%c\n", tag>>24, (tag>>16)&0xff, (tag>>8)&0xff, tag&0xff );
#endif
	switch ( tag ) {
	  case CHR('B','A','S','E'):
	    info->base_start = offset;
	  break;
	  case CHR('b','s','l','n'):
	    info->bsln_start = offset;
	  break;
	  case CHR('C','F','F',' '):
	    info->cff_start = offset;
	    info->cff_length = length;
	  break;
	  case CHR('c','m','a','p'):
	    info->encoding_start = offset;
	  break;
	  case CHR('g','a','s','p'):
	    info->gasp_start = offset;
	  break;
	  case CHR('g','l','y','f'):
	    info->glyph_start = offset;
	    info->glyph_length = length;
	  break;
	  case CHR('G','D','E','F'):
	    info->gdef_start = offset;
	    info->gdef_length = length;
	  break;
	  case CHR('G','P','O','S'):
	    info->gpos_start = offset;
	    info->gpos_length = length;
	  break;
	  case CHR('G','S','U','B'):
	    info->gsub_start = offset;
	    info->gsub_length = length;
	  break;
	  case CHR('b','d','a','t'):		/* Apple/MS use a different tag, but the same format. Great. */
	  case CHR('E','B','D','T'):
	    info->bitmapdata_start = offset;
	    info->bitmapdata_length = length;
	  break;
	  case CHR('b','l','o','c'):		/* Apple/MS use a different tag. Great. */
	  case CHR('E','B','L','C'):
	    info->bitmaploc_start = offset;
	    info->bitmaploc_length = length;
	  break;
	  case CHR('b','h','e','d'):		/* Apple uses bhed for fonts with only bitmaps */
	  case CHR('h','e','a','d'):
	    info->head_start = offset;
	  break;
	  case CHR('h','h','e','a'):
	    info->hhea_start = offset;
	  break;
	  case CHR('h','m','t','x'):
	    info->hmetrics_start = offset;
	  break;
	  case CHR('J','S','T','F'):
	    info->jstf_start = offset;
	    info->jstf_length = length;
	  break;
	  case CHR('k','e','r','n'):
	    info->kern_start = offset;
	  break;
	  case CHR('l','o','c','a'):
	    info->glyphlocations_start = offset;
	    info->loca_length = length;
	    info->glyph_cnt = length/2-1;	/* the minus one is because there is one extra entry to give the length of the last glyph */
	    if ( info->glyph_cnt<0 ) info->glyph_cnt = 0;
	  break;
	  case CHR('m','a','x','p'):
	    info->maxp_start = offset;
	    info->maxp_len = length;
	  break;
	  case CHR('n','a','m','e'):
	    info->copyright_start = offset;
	  break;
	  case CHR('p','o','s','t'):
	    info->postscript_start = offset;
	  break;
	  case CHR('O','S','/','2'):
	    info->os2_start = offset;
	  break;
	  case CHR('C','I','D',' '):
	  case CHR('T','Y','P','1'):
	    info->typ1_start = offset;
	    info->typ1_length = length;
	  break;
	  case CHR('v','h','e','a'):
	    info->vhea_start = offset;
	  break;
	  case CHR('v','m','t','x'):
	    info->vmetrics_start = offset;
	  break;
	  case CHR('M','A','T','H'):
	    info->math_start = offset;
	    info->math_length = length;
	  break;
	      /* Apple stuff */
	  case CHR('f','e','a','t'):
	    info->feat_start = offset;
	  break;
	  case CHR('l','c','a','r'):
	    info->lcar_start = offset;
	  break;
	  case CHR('m','o','r','t'):
	    info->mort_start = offset;
	  break;
	  case CHR('m','o','r','x'):
	    info->morx_start = offset;
	  break;
	  case CHR('o','p','b','d'):
	    info->opbd_start = offset;
	  break;
	  case CHR('p','r','o','p'):
	    info->prop_start = offset;
	  break;
	      /* to make sense of instrs */
	  case CHR('c','v','t',' '):
	    info->cvt_start = offset;
	    info->cvt_len = length;
	  break;
	  case CHR('p','r','e','p'):
	    info->prep_start = offset;
	    info->prep_len = length;
	  break;
	  case CHR('f','p','g','m'):
	    info->fpgm_start = offset;
	    info->fpgm_len = length;
	  break;

	    /* non-standard tables I've added */
	  case CHR('P','f','E','d'):
	    info->pfed_start = offset;
	  break;
	  case CHR('F','F','T','M'):
	    info->fftm_start = offset;
	  break;
	  case CHR('T','e','X',' '):
	    info->tex_start = offset;
	  break;
	  case CHR('B','D','F',' '):
	    info->bdf_start = offset;
	  break;

	    /* Apple's mm fonts */
	  case CHR('g','v','a','r'):
	    info->gvar_start = offset;
	    info->gvar_len = length;
	  break;
	  case CHR('f','v','a','r'):
	    info->fvar_start = offset;
	    info->fvar_len = length;
	  break;
	  case CHR('a','v','a','r'):
	    info->avar_start = offset;
	    info->avar_len = length;
	  break;
	  case CHR('c','v','a','r'):
	    info->cvar_start = offset;
	    info->cvar_len = length;
	  break;

	  default:
            if (info->openflags & of_all_tables) {
                info->savetab[i].offset = offset;
                info->savetab[i].tag = tag;
                info->savetab[i].len = length;
            }
            else {
                for ( j=0; j<info->savecnt; ++j ) if ( info->savetab[j].tag == tag ) {
                    info->savetab[j].offset = offset;
                    info->savetab[j].len = length;
                break;
                }
                if ( j==info->savecnt ) {
                    if ( first ) {
                        LogError( _("The following table(s) in the font have been ignored by FontForge\n") );
                        first = false;
                    }
                    for ( k=0; stdtables[k].tag!=0; ++k )
                        if ( stdtables[k].tag == tag )
                    break;
                    if ( stdtables[k].tag==0 ) {
                        LogError( _("  Ignoring '%c%c%c%c'\n"), tag>>24, tag>>16, tag>>8, tag);
                    } else {
                        LogError( _("  Ignoring '%c%c%c%c' %s\n"), tag>>24, tag>>16, tag>>8, tag,
                                _(stdtables[k].name));
                    }
                }
            }
	}
    }
    if ( info->glyphlocations_start!=0 && info->cff_start!=0 )
	LogError( _("This font contains both truetype and PostScript glyph descriptions\n  only one will be used.\n"));
    else if ( (info->glyphlocations_start!=0) +
		(info->cff_start!=0) +
		(info->typ1_start!=0)>1 )
	LogError( _("This font contains multiple glyph descriptions\n  only one will be used.\n"));
    if ( info->gpos_start!=0 && info->kern_start!=0 )
	LogError( _("This font contains both a 'kern' table and a 'GPOS' table.\n  The 'kern' table will only be read if there is no 'kern' feature in 'GPOS'.\n"));
    if ( (info->mort_start!=0 || info->morx_start!=0) && info->gsub_start!=0 )
	LogError( _("This font contains both a 'mor[tx]' table and a 'GSUB' table.\n  FF will only read feature/settings in 'morx' which do not match features\n  found in 'GSUB'.\n"));
    if ( info->base_start!=0 && info->bsln_start!=0 )
	LogError( _("This font contains both a 'BASE' table and a 'bsln' table.\n  FontForge will only read one of them ('BASE').\n"));
return( true );
}

static void readdate(FILE *ttf,struct ttfinfo *info,int ismod) {
    int i, date[4];
    /* TTFs have creation and modification timestamps in the 'head' table.  */
    /* These are 64 bit values in network order / big-endian and denoted    */
    /* as 'LONGDATETIME'.                                                   */
    /* These timestamps are in "number of seconds since 00:00 1904-01-01",  */
    /* noted some places as a Mac OS epoch time value.  We use Unix epoch   */
    /* timestamps which are "number of seconds since 00:00 1970-01-01".     */
    /* The difference between these two epoch values is a constant number   */ 
    /* of seconds, and so we convert from Mac to Unix time by simple        */
    /* subtraction of that constant difference.                             */

    /*      (31781 * 65536) + 45184 = 2082844800 secs is 24107 days */
    int date1970[4] = {45184, 31781, 0, 0}; 

    /* As there was not (nor still is?) a portable way to do 64-bit math aka*/
    /* "long long" the code below works on 16-bit slices of the full value. */
    /* The lowest 16 bits is operated on, then the next 16 bits, and so on. */

    date[3] = getushort(ttf);
    date[2] = getushort(ttf);
    date[1] = getushort(ttf);
    date[0] = getushort(ttf);

    for ( i=0; i<3; ++i ) {
	date[i] -= date1970[i];
	date[i+1] += date[i]>>16;
	date[i] &= 0xffff;
    }
    date[3] -= date1970[3];

    *(ismod ? &info->modificationtime : &info->creationtime) =
	    (((long long) date[3])<<48) |
	    (((long long) date[2])<<32) |
	    (             date[1] <<16) |
			  date[0];
}

static void readttfhead(FILE *ttf,struct ttfinfo *info) {
    /* Here I want units per em, and size of loca entries */
    /* oh... also creation/modification times */
    int i, flags;

    fseek(ttf,info->head_start+4,SEEK_SET);		/* skip over the version number */
    info->sfntRevision = getlong(ttf);
    (void) getlong(ttf);
    (void) getlong(ttf);
    flags = getushort(ttf);
    info->optimized_for_cleartype = (flags&(1<<13))?1:0;
    info->apply_lsb = !(flags&(1<<1));
    info->emsize = getushort(ttf);

    info->ascent = .8*info->emsize;
    info->descent = info->emsize-info->ascent;

    for ( i=0; i<8; ++i )
	getushort(ttf);
    for ( i=0; i<4; ++i )
	info->fbb[i] = (short) getushort(ttf);
    info->macstyle = getushort(ttf);
    for ( i=0; i<2; ++i )
	getushort(ttf);
    info->index_to_loc_is_long = getushort(ttf);
    if ( info->index_to_loc_is_long )
	info->glyph_cnt = info->loca_length/4-1;
    if ( info->glyph_cnt<0 ) info->glyph_cnt = 0;

    if ( info->fftm_start!=0 ) {
	fseek(ttf,info->fftm_start+3*4,SEEK_SET);
    } else {
	fseek(ttf,info->head_start+4*4+2+2,SEEK_SET);
    }
    readdate(ttf,info,0);
    readdate(ttf,info,1);
}

static void readttfhhea(FILE *ttf,struct ttfinfo *info) {
    /* Here I want ascent, descent and the number of horizontal metrics */
    int i;

    fseek(ttf,info->hhea_start+4,SEEK_SET);		/* skip over the version number */
    info->pfminfo.hhead_ascent = getushort(ttf);
    info->pfminfo.hhead_descent = (short) getushort(ttf);
    info->pfminfo.hheadascent_add = info->pfminfo.hheaddescent_add = false;
    info->pfminfo.linegap = (short) getushort(ttf);
    info->advanceWidthMax = getushort(ttf);
    info->pfminfo.hheadset = true;
    /*info->ascent = info->pfminfo.hhead_ascent;*/

    for ( i=0; i<11; ++i )
	getushort(ttf);
    info->width_cnt = getushort(ttf);
}

static void readttfmaxp(FILE *ttf,struct ttfinfo *info) {
    /* All I want here is the number of glyphs */
    int cnt;
    fseek(ttf,info->maxp_start+4,SEEK_SET);		/* skip over the version number */
    cnt = getushort(ttf);
    if ( info->glyph_cnt==0 && info->glyph_length==0 && info->loca_length<=4 &&
	    info->cff_length==0 && info->typ1_start==0 ) {
	/* X11 OpenType bitmap format */;
	info->onlystrikes = true;
    } else if ( cnt!=info->glyph_cnt && info->loca_length!=0 ) {
	ff_post_notice(_("Bad Glyph Count"), _("Font file has bad glyph count field. maxp says: %d sizeof(loca)=>%d"), cnt, info->glyph_cnt);
	info->bad_glyph_data = true;
	if ( cnt>info->glyph_cnt )
	    cnt = info->glyph_cnt;		/* Use the smaller of the two values */
    }
    /* Open Type fonts have no loca table, so we can't calculate the glyph */
    /*  count from it */
    info->glyph_cnt = cnt;
    if ( cnt<0 ) info->glyph_cnt = 0;
}

static char *stripspaces(char *str) {
    char *str2 = str, *base = str;

    if ( str==NULL )
return( NULL );

    while ( *str ) {
	if ( *str==' ' )
	    ++str;
	else
	    *str2++ = *str++;
    }
    *str2 = '\0';
return( base );
}

static struct macname *AddMacName(FILE *ttf,
	int strlen, int stroff,int spec,int language, struct macname *last) {
    struct macname *new = chunkalloc(sizeof(struct macname));
    long pos = ftell(ttf);
    char *pt;
    int i;

    new->next = last;
    new->enc = spec;
    new->lang = language;
    new->name = pt = malloc(strlen+1);

    fseek(ttf,stroff,SEEK_SET);

    for ( i=0; i<strlen; ++i )
	*pt++ = getc(ttf);
    *pt = '\0';

    fseek(ttf,pos,SEEK_SET);
return( new );
}

static void MacFeatureAdd(FILE *ttf, struct ttfinfo *info, int id,
	int strlen, int stroff,int spec,int language) {
    MacFeat *f;
    struct macsetting *s;

    for ( f=info->features; f!=NULL; f=f->next ) {
	if ( f->strid==id ) {
	    f->featname = AddMacName(ttf,strlen,stroff,spec,language,f->featname);
return;
	} else {
	    for ( s=f->settings; s!=NULL; s=s->next ) {
		if ( s->strid==id ) {
		    s->setname = AddMacName(ttf,strlen,stroff,spec,language,s->setname);
return;
		}
	    }
	}
    }
    /* Well, there are some things in the name table other than feature/setting*/
    /*  names. Let's keep track of everything just in case.... */
    if ( info->fvar_start!=0 ) {
	struct macidname *mi, *p;
	for ( p=NULL, mi=info->macstrids; mi!=NULL && mi->id!=id; p = mi, mi=mi->next );
	if ( mi==NULL ) {
	    mi = chunkalloc(sizeof(struct macidname));
	    mi->id = id;
	    mi->last = mi->head = AddMacName(ttf,strlen,stroff,spec,language,NULL);
	    if ( p==NULL )
		info->macstrids = mi;
	    else
		p->next = mi;
	} else {
	    mi->last->next = AddMacName(ttf,strlen,stroff,spec,language,NULL);
	    mi->last = mi->last->next;
	}
    }
}

static void ValidatePostScriptFontName(struct ttfinfo *info, char *str) {
    char *end, *pt, *npt;
    int complained = false;

    /* someone gave me a font where the fontname started with the utf8 byte */
    /*  order mark.  PLRM says only ASCII encoding is supported. CFF says */
    /*  only printable ASCII should be used */
    if ( ((uint8 *) str)[0] == 0xef && ((uint8 *) str)[1]==0xbb && ((uint8 *) str)[2] == 0xbf ) {
	LogError(_("The fontname begins with the utf8 byte order sequence. This is illegal. %s"), str+3 );
	info->bad_ps_fontname = true;
	for ( pt=str+3; *pt; ++pt )
	    pt[-3] = *pt;		/* ANSI says we can't strcpy overlapping strings */
    }
    strtod(str,&end);
    if ( (*end=='\0' || (isdigit(str[0]) && strchr(str,'#')!=NULL)) &&
	    *str!='\0' ) {
	ff_post_error(_("Bad Font Name"),_("A PostScript name may not be a number"));
	info->bad_ps_fontname = true;
	*str = 'a';
	complained = true;
    }
    for ( pt=str; *pt; ++pt ) {
	if ( *pt<=' ' || *pt>=0x7f ||
		*pt=='(' || *pt=='[' || *pt=='{' || *pt=='<' ||
		*pt==')' || *pt==']' || *pt=='}' || *pt=='>' ||
		*pt=='%' || *pt=='/' ) {
	    if ( !complained ) {
		ff_post_error(_("Bad Font Name"),_("The PostScript font name \"%.63s\" is invalid.\nIt should be printable ASCII,\nmust not contain (){}[]<>%%/ or space\nand must be shorter than 63 characters"),str);
		info->bad_ps_fontname = true;
	    }
	    complained = true;
	    for ( npt=pt; npt[1]; ++npt )
		*npt = npt[1];
	    *npt = '\0';
	    --pt;
	}
    }
    if ( strlen(str)>63 ) {
	ff_post_error(_("Bad Font Name"),_("The PostScript font name \"%.63s\" is invalid.\nIt should be printable ASCII,\nmust not contain (){}[]<>%%/ or space\nand must be shorter than 63 characters"),str);
	info->bad_ps_fontname = true;
	str[63] = '\0';
    }
}

char *EnforcePostScriptName(char *old) {
    char *end, *pt, *npt, *str = copy(old);

    if ( old==NULL )
return( old );

    strtod(str,&end);
    if ( (*end=='\0' || (isdigit(str[0]) && strchr(str,'#')!=NULL)) &&
	    *str!='\0' ) {
	free(str);
	str=malloc(strlen(old)+2);
	*str = 'a';
	strcpy(str+1,old);
    }
    for ( pt=str; *pt; ++pt ) {
	if ( *pt<=' ' || *pt>=0x7f ||
		*pt=='(' || *pt=='[' || *pt=='{' || *pt=='<' ||
		*pt==')' || *pt==']' || *pt=='}' || *pt=='>' ||
		*pt=='%' || *pt=='/' ) {
	    for ( npt=pt; npt[1]; ++npt )
		*npt = npt[1];
	    *npt = '\0';
	}
    }
    if ( strlen(str)>63 )
	str[63] = '\0';
return( str );
}

static int IsSubSetOf(const char *substr,const char *fullstr ) {
    /* The mac string is often a subset of the unicode string. Certain */
    /*  characters can't be expressed in the mac encoding and are omitted */
    /*  or turned to question marks or some such */
    const char *pt1, *pt2;
    uint32 ch1, ch2;

    for ( pt1=substr, pt2=fullstr, ch1=utf8_ildb(&pt1); ch1!=0 ; ) {
	if ( *pt2=='\0' )
    break;
	ch2 = utf8_ildb(&pt2);
	if ( ch1==ch2 )
	    ch1 = utf8_ildb(&pt1);
    }
    if ( ch1=='\0' )
return( true );

    for ( pt1=substr, pt2=fullstr, ch1=utf8_ildb(&pt1); ch1!=0 ; ) {
	if ( *pt2=='\0' )
    break;
	ch2 = utf8_ildb(&pt2);
	if ( ch1==ch2 || ch1=='?' )
	    ch1 = utf8_ildb(&pt1);
    }
return( ch1=='\0' );
}

static void TTFAddLangStr(FILE *ttf, struct ttfinfo *info, int id,
	int strlength, int stroff,int plat,int spec,int language) {
    struct ttflangname *cur, *prev;
    char *str;

    if ( plat==1 && id>=256 && (info->features!=NULL || info->fvar_start!=0)) {
	MacFeatureAdd(ttf,info,id,strlength,stroff,spec,language);
return;
    } else if ( id<0 || id>=ttf_namemax )
return;

    str = _readencstring(ttf,stroff,strlength,plat,spec,language);
    if ( str==NULL )		/* we didn't understand the encoding */
return;
    if ( id==ttf_postscriptname )
	ValidatePostScriptFontName(info,str);
    if ( *str=='\0' ) {
	free(str);
return;
    }

    if ( plat==1 || plat==0 )
	language = WinLangFromMac(language);
    if ( (language&0xff00)==0 ) language |= 0x400;

    for ( prev=NULL, cur=info->names; cur!=NULL && cur->lang!=language; prev = cur, cur=cur->next );
    if ( cur==NULL ) {
	cur = chunkalloc(sizeof(struct ttflangname));
	cur->lang = language;
	if ( prev==NULL )
	    info->names = cur;
	else
	    prev->next = cur;
    }
    if ( cur->names[id]==NULL ) {
	cur->names[id] = str;
	if ( plat==1 || plat==0 )
	    cur->frommac[id/32] |= (1<<(id&0x1f));
    } else if ( strcmp(str,cur->names[id])==0 ) {
	free(str);
	if ( plat==3 )
	    cur->frommac[id/32] &= ~(1<<(id&0x1f));
    } else if ( plat==1 ) {
	/* Mac string doesn't match mac unicode string */
	if ( !IsSubSetOf(str,cur->names[id]) )
	    LogError( _("Warning: Mac and Unicode entries in the 'name' table differ for the\n %s string in the language %s\n Mac String: %s\nMac Unicode String: %s\n"),
		    TTFNameIds(id),MSLangString(language),
		    str,cur->names[id]);
	else
	    LogError( _("Warning: Mac string is a subset of the Unicode string in the 'name' table\n for the %s string in the %s language.\n"),
		    TTFNameIds(id),MSLangString(language));
	free(str);
    } else if ( plat==3 && (cur->frommac[id/32] & (1<<(id&0x1f))) ) {
	if ( !IsSubSetOf(cur->names[id],str) )
	    LogError( _("Warning: Mac and Windows entries in the 'name' table differ for the\n %s string in the language %s\n Mac String: %s\nWindows String: %s\n"),
		    TTFNameIds(id),MSLangString(language),
		    cur->names[id],str);
	else
	    LogError( _("Warning: Mac string is a subset of the Windows string in the 'name' table\n for the %s string in the %s language.\n"),
		    TTFNameIds(id),MSLangString(language));
	free(cur->names[id]);
	cur->names[id] = str;
	cur->frommac[id/32] &= ~(1<<(id&0x1f));
    } else {
	int ret;
	if ( info->dupnamestate!=0 )
	    ret = info->dupnamestate;
	else if ( running_script )
	    ret = 3;
	else {
	    char *buts[5];
	    buts[0] = _("Use _First");
	    buts[1] = _("First to _All");
	    buts[2] = _("Second _to All");
	    buts[3] = _("Use _Second");
	    buts[4] = NULL;
	    ret = ff_ask(_("Multiple names for language"),(const char **)buts,0,3,
		    _("The 'name' table contains (at least) two strings for the %s in language %s, the first '%.12s...' the second '%.12s...'.\nWhich do you prefer?"),
		    TTFNameIds(id),MSLangString(language),
		    cur->names[id],str);
	    if ( ret==1 || ret==2 )
		info->dupnamestate = ret;
	}
	if ( ret==0 || ret==1 )
	    free(str);
	else {
	    free(cur->names[id]);
	    cur->names[id] = str;
	}
    }
}

static int is_ascii(char *str) {	/* isascii is in ctype */
    if ( str==NULL )
return( false );
    while ( *str && *str<127 && *str>=' ' )
	++str;
return( *str=='\0' );
}

static char *FindLangEntry(struct ttfinfo *info, int id ) {
    /* Look for an entry with string id */
    /* we prefer english, if we can't find english look for something in ascii */
    struct ttflangname *cur;
    char *ret;

    for ( cur=info->names; cur!=NULL && cur->lang!=0x409; cur=cur->next );
    if ( cur!=NULL && cur->names[id]==NULL ) cur = NULL;
    if ( cur==NULL )
	for ( cur=info->names; cur!=NULL && (cur->lang&0xf)!=0x09; cur=cur->next );
    if ( cur!=NULL && cur->names[id]==NULL ) cur = NULL;
    if ( cur==NULL )
	for ( cur=info->names; cur!=NULL && !is_ascii(cur->names[id]); cur=cur->next );
    if ( cur==NULL )
	for ( cur=info->names; cur!=NULL && cur->names[id]==NULL; cur=cur->next );
    if ( cur==NULL )
return( NULL );
    ret = copy(cur->names[id]);	
return( ret );
}

struct otfname *FindAllLangEntries(FILE *ttf, struct ttfinfo *info, int id ) {
    /* Look for all entries with string id under windows platform */
    int32 here = ftell(ttf);
    int i, cnt, tableoff;
    int platform, specific, language, name, str_len, stroff;
    struct otfname *head=NULL, *cur;

    if ( info->copyright_start!=0 && id!=0 ) {
	fseek(ttf,info->copyright_start,SEEK_SET);
	/* format selector = */ getushort(ttf);
	cnt = getushort(ttf);
	tableoff = info->copyright_start+getushort(ttf);
	for ( i=0; i<cnt; ++i ) {
	    platform = getushort(ttf);
	    specific = getushort(ttf);
	    language = getushort(ttf);
	    name = getushort(ttf);
	    str_len = getushort(ttf);
	    stroff = getushort(ttf);

	    if ( platform==3 && name==id ) {
		char *temp = _readencstring(ttf,tableoff+stroff,str_len,platform,specific,language);
		if ( temp!=NULL ) {
		    cur = chunkalloc(sizeof(struct otfname));
		    cur->next = head;
		    head = cur;
		    cur->lang = language;
		    cur->name = temp;
		}
	    }
	}
	fseek(ttf,here,SEEK_SET);
    }
return( head );
}

static struct macname *reversemacnames(struct macname *mn) {
    struct macname *next, *prev=NULL;

    if ( mn==NULL )
return( NULL );

    next = mn->next;
    while ( next!=NULL ) {
	mn->next = prev;
	prev = mn;
	mn = next;
	next = mn->next;
    }
    mn->next = prev;
return( mn );
}

static void readttfcopyrights(FILE *ttf,struct ttfinfo *info) {
    int i, cnt, tableoff;
    int platform, specific, language, name, str_len, stroff;

    if ( info->feat_start!=0 )
	readmacfeaturemap(ttf,info);
    if ( info->copyright_start!=0 ) {
	fseek(ttf,info->copyright_start,SEEK_SET);
	/* format selector = */ getushort(ttf);
	cnt = getushort(ttf);
	tableoff = info->copyright_start+getushort(ttf);
	for ( i=0; i<cnt; ++i ) {
	    platform = getushort(ttf);
	    specific = getushort(ttf);
	    language = getushort(ttf);
	    name = getushort(ttf);
	    str_len = getushort(ttf);
	    stroff = getushort(ttf);
    
	    TTFAddLangStr(ttf,info,name,str_len,tableoff+stroff,
		    platform,specific,language);
	}
    }

    if ( info->copyright==NULL )
	info->copyright = FindLangEntry(info,ttf_copyright);
    if ( info->familyname==NULL )
	info->familyname = FindLangEntry(info,ttf_family);
    if ( info->fullname==NULL )
	info->fullname = FindLangEntry(info,ttf_fullname);
    if ( info->version==NULL )
	info->version = FindLangEntry(info,ttf_version);
    if ( info->fontname==NULL )
	info->fontname = FindLangEntry(info,ttf_postscriptname);

    if ( info->fontname != NULL && *info->fontname=='\0' ) {
	free(info->fontname);
	info->fontname = NULL;
    }
    if ( info->familyname != NULL && *info->familyname=='\0' ) {
	free(info->familyname);
	info->familyname = NULL;
    }
    if ( info->fullname != NULL && *info->fullname=='\0' ) {
	free(info->fullname);
	info->fullname = NULL;
    }

    /* OpenType spec says the version string should begin with "Version " and */
    /*  end with a space and have a number in between */
    if ( info->version==NULL ) info->version = copy("1.0");
    else if ( strnmatch(info->version,"Version ",8)==0 ) {
	char *temp = copy(info->version+8);
	if ( temp[strlen(temp)-1]==' ' )
	    temp[strlen(temp)-1] = '\0';
	free(info->version);
	info->version = temp;
    }
    if ( info->fontname==NULL ) {
	if ( info->fullname!=NULL )
	    info->fontname = stripspaces(copy(info->fullname));
	if ( info->fontname==NULL && info->familyname!=NULL )
	    info->fontname = stripspaces(copy(info->familyname));
	if ( info->fontname!=NULL )
	    ValidatePostScriptFontName(info,info->fontname);
    }

    if ( info->features ) {
	MacFeat *mf;
	struct macsetting *ms;
	for ( mf=info->features; mf!=NULL; mf = mf->next ) {
	    mf->featname = reversemacnames(mf->featname);
	    for ( ms=mf->settings; ms!=NULL; ms=ms->next )
		ms->setname = reversemacnames(ms->setname);
	}
    }
}

static void readttfpreglyph(FILE *ttf,struct ttfinfo *info) {
    if ( info->head_start!=0 )
	readttfhead(ttf,info);
    if ( info->hhea_start!=0 )
	readttfhhea(ttf,info);
    if ( info->maxp_start!=0 )
	readttfmaxp(ttf,info);
    readttfcopyrights(ttf,info);	/* This one has internal checks */
}

#define _On_Curve	1
#define _X_Short	2
#define _Y_Short	4
#define _Repeat		8
#define _X_Same		0x10
#define _Y_Same		0x20

static void FigureControls(SplinePoint *from, SplinePoint *to, BasePoint *cp,
	int is_order2) {
    /* What are the control points for 2 cp bezier which will provide the same*/
    /*  curve as that for the 1 cp bezier specified above */
    real b, c, d;

    if ( is_order2 ) {
	from->nextcp = to->prevcp = *cp;
    } else {
	d = from->me.x;
	c = 2*cp->x - 2*from->me.x;
	b = to->me.x+from->me.x-2*cp->x;
	from->nextcp.x = d+c/3;
	to->prevcp.x = from->nextcp.x + (c+b)/3;

	d = from->me.y;
	c = 2*cp->y - 2*from->me.y;
	b = to->me.y+from->me.y-2*cp->y;
	from->nextcp.y = d+c/3;
	to->prevcp.y = from->nextcp.y + (c+b)/3;
    }

    if ( from->me.x!=from->nextcp.x || from->me.y!=from->nextcp.y || from->nextcpindex<0xfffe )
	from->nonextcp = false;
    if ( to->me.x!=to->prevcp.x || to->me.y!=to->prevcp.y || from->nextcpindex<0xfffe )
	to->noprevcp = false;
    if ( is_order2 && (to->noprevcp || from->nonextcp)) {
	to->noprevcp = from->nonextcp = true;
	from->nextcp = from->me;
	to->prevcp = to->me;
    }
}

static SplineSet *ttfbuildcontours(int path_cnt,uint16 *endpt, char *flags,
	BasePoint *pts, int is_order2) {
    SplineSet *head=NULL, *last=NULL, *cur;
    int i, path, start, last_off;
    SplinePoint *sp;

    for ( path=i=0; path<path_cnt; ++path ) {
	if ( endpt[path]<i )	/* Sigh. Yes there are fonts with bad endpt info */
    continue;
	cur = chunkalloc(sizeof(SplineSet));
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	last_off = false;
	start = i;
	sp = NULL;
	while ( i<=endpt[path] ) {
	    if ( flags[i]&_On_Curve ) {
		sp = chunkalloc(sizeof(SplinePoint));
		sp->me = sp->nextcp = sp->prevcp = pts[i];
		sp->nonextcp = sp->noprevcp = true;
		sp->ttfindex = i;
		sp->nextcpindex = 0xffff;
		if ( last_off ) {
		  sp->noprevcp = false;
		  if ( cur->last!=NULL ) {
		    cur->last->nonextcp = false;
		    FigureControls(cur->last,sp,&pts[i-1],is_order2);
		    cur->last->nonextcp = false; // FigureControls reads and changes, so we do this twice.
		  }
		}
		last_off = false;
	    } else if ( last_off ) {
		/* two off curve points get a third on curve point created */
		/* half-way between them. Now isn't that special */
		sp = chunkalloc(sizeof(SplinePoint));
		sp->me.x = (pts[i].x+pts[i-1].x)/2;
		sp->me.y = (pts[i].y+pts[i-1].y)/2;
		sp->nextcp = sp->prevcp = sp->me;
		sp->nonextcp = true;
		sp->ttfindex = 0xffff;
		sp->nextcpindex = i;
		if ( last_off && cur->last!=NULL )
		    FigureControls(cur->last,sp,&pts[i-1],is_order2);
		/* last_off continues to be true */
	    } else {
		if ( cur->first!=NULL )
		    cur->last->nextcpindex = i;
		last_off = true;
		sp = NULL;
	    }
	    if ( sp!=NULL ) {
		if ( cur->first==NULL )
		    cur->first = sp;
		else
		    SplineMake(cur->last,sp,is_order2);
		cur->last = sp;
	    }
	    ++i;
	}
	if ( start==i-1 ) {
	    /* MS chinese fonts have contours consisting of a single off curve*/
	    /*  point. What on earth do they think that means? */
	    /* Oh. I see. It's used to possition marks and such */
	    if ( cur->first==NULL ) {
		sp = chunkalloc(sizeof(SplinePoint));
		sp->me.x = pts[start].x;
		sp->me.y = pts[start].y;
		sp->nextcp = sp->prevcp = sp->me;
		sp->nonextcp = sp->noprevcp = true;
		sp->ttfindex = i-1;
		sp->nextcpindex = 0xffff;
		cur->first = cur->last = sp;
	    }
	} else if ( !(flags[start]&_On_Curve) && !(flags[i-1]&_On_Curve) ) {
	    sp = chunkalloc(sizeof(SplinePoint));
	    sp->me.x = (pts[start].x+pts[i-1].x)/2;
	    sp->me.y = (pts[start].y+pts[i-1].y)/2;
	    sp->nextcp = sp->prevcp = sp->me;
	    sp->nonextcp = true;
	    sp->ttfindex = 0xffff;
	    sp->nextcpindex = start;
	    FigureControls(cur->last,sp,&pts[i-1],is_order2);
	    SplineMake(cur->last,sp,is_order2);
	    cur->last = sp;
	    FigureControls(sp,cur->first,&pts[start],is_order2);
	} else if ( !(flags[i-1]&_On_Curve)) {
	    FigureControls(cur->last,cur->first,&pts[i-1],is_order2);
	    cur->last->nextcpindex = i-1;
	} else if ( !(flags[start]&_On_Curve) ) {
	    FigureControls(cur->last,cur->first,&pts[start],is_order2);
	    sp->nextcpindex = start;
	}
	if ( cur->last!=cur->first ) {
	    SplineMake(cur->last,cur->first,is_order2);
	    cur->last = cur->first;
	}
	for ( sp=cur->first; ; ) {
	    if ( sp->ttfindex!=0xffff && SPInterpolate(sp) )
		sp->dontinterpolate = true;
	    if ( sp->next==NULL )
	break;
	    sp=sp->next->to;
	    if ( sp==cur->first )
	break;
	}
    }
return( head );
}

static void readttfsimpleglyph(FILE *ttf,struct ttfinfo *info,SplineChar *sc, int path_cnt, int gbb[4]) {
    uint16 *endpt = malloc((path_cnt+1)*sizeof(uint16));
    uint8 *instructions;
    char *flags;
    BasePoint *pts;
    int i, j, tot, len;
    int last_pos;

    for ( i=0; i<path_cnt; ++i ) {
	endpt[i] = getushort(ttf);
	if ( i!=0 && endpt[i]<endpt[i-1] ) {
	    info->bad_glyph_data = true;
	    LogError( _("Bad tt font: contour ends make no sense in glyph %d.\n"),
		    sc->orig_pos );
return;
	}
    }
    if ( path_cnt==0 ) {
	tot = 0;
	pts = malloc(sizeof(BasePoint));
    } else {
	tot = endpt[path_cnt-1]+1;
	pts = malloc(tot*sizeof(BasePoint));
    }

    len = getushort(ttf);
    instructions = malloc(len);
    for ( i=0; i<len; ++i )
	instructions[i] = getc(ttf);

    flags = malloc(tot);
    for ( i=0; i<tot; ++i ) {
	flags[i] = getc(ttf);
	if ( flags[i]&_Repeat ) {
	    int cnt = getc(ttf);
	    if ( i+cnt>=tot ) {
		IError("Flag count is wrong (or total is): %d %d", i+cnt, tot );
		cnt = tot-i-1;
	    }
	    for ( j=0; j<cnt; ++j )
		flags[i+j+1] = flags[i];
	    i += cnt;
	}
	if ( feof(ttf))
    break;
    }
    if ( i!=tot )
	IError("Flag count is wrong (or total is): %d %d in glyph %d", i, tot, sc->orig_pos );

    last_pos = 0;
    for ( i=0; i<tot; ++i ) {
	if ( flags[i]&_X_Short ) {
	    int off = getc(ttf);
	    if ( !(flags[i]&_X_Same ) )
		off = -off;
	    pts[i].x = last_pos + off;
	} else if ( flags[i]&_X_Same )
	    pts[i].x = last_pos;
	else
	    pts[i].x = last_pos + (short) getushort(ttf);
	last_pos = pts[i].x;
	if ( (last_pos<gbb[0] || last_pos>gbb[2]) && ( flags[i]&_On_Curve )) {
	    if ( !info->gbbcomplain || (info->openflags&of_fontlint)) {
		LogError(_("A point in GID %d is outside the glyph bounding box\n"), sc->orig_pos );
		info->bad_glyph_data = true;
		if ( !(info->openflags&of_fontlint) )
		    LogError(_("  Subsequent errors will not be reported.\n") );
		info->gbbcomplain = true;
	    }
	}
    }

    last_pos = 0;
    for ( i=0; i<tot; ++i ) {
	if ( flags[i]&_Y_Short ) {
	    int off = getc(ttf);
	    if ( !(flags[i]&_Y_Same ) )
		off = -off;
	    pts[i].y = last_pos + off;
	} else if ( flags[i]&_Y_Same )
	    pts[i].y = last_pos;
	else
	    pts[i].y = last_pos + (short) getushort(ttf);
	last_pos = pts[i].y;
	if (( last_pos<gbb[1] || last_pos>gbb[3]) && ( flags[i]&_On_Curve ) ) {
	    if ( !info->gbbcomplain || (info->openflags&of_fontlint)) {
		LogError(_("A point in GID %d is outside the glyph bounding box\n"), sc->orig_pos );
		info->bad_glyph_data = true;
		if ( !(info->openflags&of_fontlint) )
		    LogError(_("  Subsequent errors will not be reported.\n") );
		info->gbbcomplain = true;
	    }
	}
    }

    sc->layers[ly_fore].splines = ttfbuildcontours(path_cnt,endpt,flags,pts,info->to_order2);
    if ( info->to_order2 && len!=0 ) {
	sc->ttf_instrs_len = len;
	sc->ttf_instrs = instructions;
    } else
	free(instructions);
    SCCategorizePoints(sc);
    free(endpt);
    free(flags);
    free(pts);
    if ( feof(ttf)) {
	LogError( _("Reached end of file when reading simple glyph\n") );
	info->bad_glyph_data = true;
    }
}

static void readttfcompositglyph(FILE *ttf,struct ttfinfo *info,SplineChar *sc, int32 end) {
    RefChar *head=NULL, *last=NULL, *cur;
    int flags=0, arg1, arg2;
    int use_my_metrics=0;

    if ( ftell(ttf)>=end ) {
	LogError( _("Empty composite %d\n"), sc->orig_pos );
	info->bad_glyph_data = true;
return;
    }

    do {
	if ( ftell(ttf)>=end ) {
	    LogError( _("Bad flags value, implied MORE components at end of glyph %d\n"), sc->orig_pos );
	    info->bad_glyph_data = true;
    break;
	}
	cur = RefCharCreate();
	flags = getushort(ttf);
	cur->orig_pos = getushort(ttf);
	if ( feof(ttf) || cur->orig_pos>=info->glyph_cnt ) {
	    LogError(_("Reference to glyph %d out of bounds when parsing 'glyf' table.\n"), cur->orig_pos );
	    info->bad_glyph_data = true;
	    cur->orig_pos = 0;
	}
	if ( info->inuse!=NULL )
	    info->inuse[cur->orig_pos] = true;
	if ( flags&_ARGS_ARE_WORDS ) {
	    arg1 = (short) getushort(ttf);
	    arg2 = (short) getushort(ttf);
	} else {
	    arg1 = (signed char) getc(ttf);
	    arg2 = (signed char) getc(ttf);
	}
	cur->use_my_metrics =		 (flags & _USE_MY_METRICS) ? 1 : 0;
	if ( cur->use_my_metrics ) {
	    if ( use_my_metrics ) {
		LogError( _("Use-my-metrics flag set on at least two components in glyph %d\n"), sc->orig_pos );
		info->bad_glyph_data = true;
	    } else
		use_my_metrics = true;
	}
	cur->round_translation_to_grid = (flags & _ROUND) ? 1 : 0;
	if ( flags & _ARGS_ARE_XY ) {
	    /* There is some very strange stuff (half-)documented on the apple*/
	    /*  site about how these should be interpretted when there are */
	    /*  scale factors, or rotations */
	    /* It isn't well enough described to be comprehensible */
	    /*  http://fonts.apple.com/TTRefMan/RM06/Chap6glyf.html */
	    /* Microsoft says nothing about this */
	    /* Adobe implies this is a difference between MS and Apple */
	    /*  MS doesn't do this, Apple does (GRRRGH!!!!) */
	    /* Adobe says that setting bit 12 means that this will not happen */
	    /* Adobe says that setting bit 11 means that this will happen */
	    /*  So if either bit is set we know when this happens, if neither */
	    /*  we guess... But I still don't know how to interpret the */
	    /*  apple mode under rotation... */
	    /* I notice that FreeType does nothing about rotation nor does it */
	    /*  interpret bits 11&12 */
	    /* Ah. It turns out that even Apple does not do what Apple's docs */
	    /*  claim it does. I think I've worked it out (see below), but... */
	    /*  Bleah! */
	    cur->transform[4] = arg1;
	    cur->transform[5] = arg2;
	} else {
	    /* Somehow we can get offsets by looking at the points in the */
	    /*  points so far generated and comparing them to the points in */
	    /*  the current componant */
	    /* How exactly is not described on any of the Apple, MS, Adobe */
	    /* freetype looks up arg1 in the set of points we've got so far */
	    /*  looks up arg2 in the new component (before renumbering) */
	    /*  offset.x = arg1.x - arg2.x; offset.y = arg1.y - arg2.y; */
	    /* This fixup needs to be done later though (after all glyphs */
	    /*  have been loaded) */
	    cur->match_pt_base = arg1;
	    cur->match_pt_ref = arg2;
	    cur->point_match = true;
	}
	cur->transform[0] = cur->transform[3] = 1.0;
	if ( flags & _SCALE )
	    cur->transform[0] = cur->transform[3] = get2dot14(ttf);
	else if ( flags & _XY_SCALE ) {
	    cur->transform[0] = get2dot14(ttf);
	    cur->transform[3] = get2dot14(ttf);
	} else if ( flags & _MATRIX ) {
	    cur->transform[0] = get2dot14(ttf);
	    cur->transform[1] = get2dot14(ttf);
	    cur->transform[2] = get2dot14(ttf);
	    cur->transform[3] = get2dot14(ttf);
	}
	if ( flags & _ARGS_ARE_XY ) {	/* Only muck with these guys if they are real offsets and not point matching */
#ifdef __Mac
	/* On mac assume scaled offsets unless told unscaled explicitly */
	if ( !(flags&_UNSCALED_OFFSETS) &&
#else
	/* everywhere else assume unscaled offsets unless told scaled explicitly */
	if ( (flags & _SCALED_OFFSETS) &&
#endif
		(flags & _ARGS_ARE_XY) && (flags&(_SCALE|_XY_SCALE|_MATRIX))) {
	    /*static int asked = 0;*/
	    /* This is not what Apple documents on their website. But it is */
	    /*  what appears to match the behavior of their rasterizer */
	    /* Apple has changed their documentation (without updating their */
	    /*  changelog), but I believe they are still incorrect */
	    cur->transform[4] *= sqrt(cur->transform[0]*cur->transform[0]+
		    cur->transform[1]*cur->transform[1]);
	    cur->transform[5] *= sqrt(cur->transform[2]*cur->transform[2]+
		    cur->transform[3]*cur->transform[3]);
	}
	}
	if ( cur->orig_pos>=info->glyph_cnt ) {
	    LogError(_("Glyph %d attempts to reference glyph %d which is outside the font\n"), sc->orig_pos, cur->orig_pos );
	    chunkfree(cur,sizeof(*cur));
	} else {
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
	if ( feof(ttf)) {
	    LogError(_("Reached end of file when reading composite glyph\n") );
	    info->bad_glyph_data = true;
    break;
	}
    } while ( flags&_MORE );
    if ( (flags & _INSTR ) && info->to_order2 && ftell(ttf)<end ) {
	sc->ttf_instrs_len = getushort(ttf);
	if ( sc->ttf_instrs_len > 0 && ftell(ttf)+sc->ttf_instrs_len<=end ) {
	    uint8 *instructions = malloc(sc->ttf_instrs_len);
	    int i;
	    for ( i=0; i<sc->ttf_instrs_len; ++i )
		instructions[i] = getc(ttf);
	    sc->ttf_instrs = instructions;
	} else
	    sc->ttf_instrs_len = 0;
    }
    sc->layers[ly_fore].refs = head;
}

static SplineChar *readttfglyph(FILE *ttf,struct ttfinfo *info,uint32 start, uint32 end,int gid) {
    int path_cnt;
    SplineChar *sc = SplineCharCreate(2);
    int gbb[4];

    sc->layers[ly_fore].background = 0;
    sc->layers[ly_back].background = 1;
    sc->unicodeenc = -1;
    sc->vwidth = info->emsize;
    sc->orig_pos = gid;

    if ( end>info->glyph_length ) {
	if ( !info->complainedbeyondglyfend )
	    LogError(_("Bad glyph (%d), its definition extends beyond the end of the glyf table\n"), gid );
	info->bad_glyph_data = true;
	info->complainedbeyondglyfend = true;
	SplineCharFree(sc);
return( NULL );
    } else if ( end<start ) {
	LogError(_("Bad glyph (%d), its data length is negative\n"), gid );
	SplineCharFree(sc);
return( NULL );
    }

    if ( start==end ) {
	/* This isn't mentioned, but we seem to get some glyphs with no size,*/
	/*  not even a path cnt. They appear to be empty glyphs */
return( sc );
    }
    fseek(ttf,info->glyph_start+start,SEEK_SET);
    path_cnt = (short) getushort(ttf);
    gbb[0] = sc->lsidebearing = (short) getushort(ttf);
    gbb[1] = (short) getushort(ttf);
    gbb[2] = (short) getushort(ttf);
    gbb[3] = (short) getushort(ttf);
    if ( info->head_start!=0 && ( gbb[0]<info->fbb[0] || gbb[1]<info->fbb[1] ||
				  gbb[2]>info->fbb[2] || gbb[3]>info->fbb[3])) {
	if ( !info->bbcomplain || (info->openflags&of_fontlint)) {
	    LogError(_("Glyph bounding box data exceeds font bounding box data for GID %d\n"), gid );
	    info->bad_glyph_data = true;
	    if ( !(info->openflags&of_fontlint) )
		LogError(_("  Subsequent errors will not be reported.\n") );
	    info->bbcomplain = true;
	}
    }
    if ( path_cnt>=0 )
	readttfsimpleglyph(ttf,info,sc,path_cnt,gbb);
    else
	readttfcompositglyph(ttf,info,sc,info->glyph_start+end);
	/* I don't check that composite glyphs fit in the bounding box */
	/* because the components may not have been read in yet */
	/* I'll check against the font bb later, if validation mode */
    if ( start>end ) {
	LogError(_("Bad glyph (%d), disordered 'loca' table (start comes after end)\n"), gid );
	info->bad_glyph_data = true;
    } else if ( ftell(ttf)>info->glyph_start+end ) {
	LogError(_("Bad glyph (%d), its definition extends beyond the space allowed for it\n"), gid );
	info->bad_glyph_data = true;
    }
return( sc );
}

static void readttfencodings(FILE *ttf,struct ttfinfo *info, int justinuse);

static void readttfglyphs(FILE *ttf,struct ttfinfo *info) {
    int i, anyread;
    uint32 *goffsets = malloc((info->glyph_cnt+1)*sizeof(uint32));

    /* First we read all the locations. This might not be needed, they may */
    /*  just follow one another, but nothing I've noticed says that so let's */
    /*  be careful */
    fseek(ttf,info->glyphlocations_start,SEEK_SET);
    if ( info->index_to_loc_is_long ) {
	for ( i=0; i<=info->glyph_cnt ; ++i )
	    goffsets[i] = getlong(ttf);
    } else {
	for ( i=0; i<=info->glyph_cnt ; ++i )
	    goffsets[i] = 2*getushort(ttf);
    }

    info->chars = calloc(info->glyph_cnt,sizeof(SplineChar *));
    if ( !info->is_ttc || (info->openflags&of_all_glyphs_in_ttc)) {
	/* read all the glyphs */
	for ( i=0; i<info->glyph_cnt ; ++i ) {
	    info->chars[i] = readttfglyph(ttf,info,goffsets[i],goffsets[i+1],i);
	    ff_progress_next();
	}
    } else {
	/* only read the glyphs we actually use in this font */
	/* this is complicated by references (and substitutions), */
	/* we can't just rely on the encoding to tell us what is used */
	info->inuse = calloc(info->glyph_cnt,sizeof(char));
	readttfencodings(ttf,info,git_justinuse);
	if ( info->gsub_start!=0 )		/* Some glyphs may appear in substitutions and not in the encoding... */
	    readttfgsubUsed(ttf,info);
	if ( info->math_start!=0 )
	    otf_read_math_used(ttf,info);
	if ( info->morx_start!=0 || info->mort_start!=0 )
	    readttfmort_glyphsused(ttf,info);
	anyread = true;
	while ( anyread ) {
	    anyread = false;
	    for ( i=0; i<info->glyph_cnt ; ++i ) {
		if ( info->inuse[i] && info->chars[i]==NULL ) {
		    info->chars[i] = readttfglyph(ttf,info,goffsets[i],goffsets[i+1],i);
		    ff_progress_next();
		    anyread = info->chars[i]!=NULL;
		}
	    }
	}
	free(info->inuse); info->inuse = NULL;
    }
    free(goffsets);
    for ( i=0; i<info->glyph_cnt ; ++i )
	if ( info->chars[i]!=NULL )
	    info->chars[i]->orig_pos = i;
    ff_progress_next_stage();
}

/* Standard names for cff */
const char *cffnames[] = {
 ".notdef",
 "space",
 "exclam",
 "quotedbl",
 "numbersign",
 "dollar",
 "percent",
 "ampersand",
 "quoteright",
 "parenleft",
 "parenright",
 "asterisk",
 "plus",
 "comma",
 "hyphen",
 "period",
 "slash",
 "zero",
 "one",
 "two",
 "three",
 "four",
 "five",
 "six",
 "seven",
 "eight",
 "nine",
 "colon",
 "semicolon",
 "less",
 "equal",
 "greater",
 "question",
 "at",
 "A",
 "B",
 "C",
 "D",
 "E",
 "F",
 "G",
 "H",
 "I",
 "J",
 "K",
 "L",
 "M",
 "N",
 "O",
 "P",
 "Q",
 "R",
 "S",
 "T",
 "U",
 "V",
 "W",
 "X",
 "Y",
 "Z",
 "bracketleft",
 "backslash",
 "bracketright",
 "asciicircum",
 "underscore",
 "quoteleft",
 "a",
 "b",
 "c",
 "d",
 "e",
 "f",
 "g",
 "h",
 "i",
 "j",
 "k",
 "l",
 "m",
 "n",
 "o",
 "p",
 "q",
 "r",
 "s",
 "t",
 "u",
 "v",
 "w",
 "x",
 "y",
 "z",
 "braceleft",
 "bar",
 "braceright",
 "asciitilde",
 "exclamdown",
 "cent",
 "sterling",
 "fraction",
 "yen",
 "florin",
 "section",
 "currency",
 "quotesingle",
 "quotedblleft",
 "guillemotleft",
 "guilsinglleft",
 "guilsinglright",
 "fi",
 "fl",
 "endash",
 "dagger",
 "daggerdbl",
 "periodcentered",
 "paragraph",
 "bullet",
 "quotesinglbase",
 "quotedblbase",
 "quotedblright",
 "guillemotright",
 "ellipsis",
 "perthousand",
 "questiondown",
 "grave",
 "acute",
 "circumflex",
 "tilde",
 "macron",
 "breve",
 "dotaccent",
 "dieresis",
 "ring",
 "cedilla",
 "hungarumlaut",
 "ogonek",
 "caron",
 "emdash",
 "AE",
 "ordfeminine",
 "Lslash",
 "Oslash",
 "OE",
 "ordmasculine",
 "ae",
 "dotlessi",
 "lslash",
 "oslash",
 "oe",
 "germandbls",
 "onesuperior",
 "logicalnot",
 "mu",
 "trademark",
 "Eth",
 "onehalf",
 "plusminus",
 "Thorn",
 "onequarter",
 "divide",
 "brokenbar",
 "degree",
 "thorn",
 "threequarters",
 "twosuperior",
 "registered",
 "minus",
 "eth",
 "multiply",
 "threesuperior",
 "copyright",
 "Aacute",
 "Acircumflex",
 "Adieresis",
 "Agrave",
 "Aring",
 "Atilde",
 "Ccedilla",
 "Eacute",
 "Ecircumflex",
 "Edieresis",
 "Egrave",
 "Iacute",
 "Icircumflex",
 "Idieresis",
 "Igrave",
 "Ntilde",
 "Oacute",
 "Ocircumflex",
 "Odieresis",
 "Ograve",
 "Otilde",
 "Scaron",
 "Uacute",
 "Ucircumflex",
 "Udieresis",
 "Ugrave",
 "Yacute",
 "Ydieresis",
 "Zcaron",
 "aacute",
 "acircumflex",
 "adieresis",
 "agrave",
 "aring",
 "atilde",
 "ccedilla",
 "eacute",
 "ecircumflex",
 "edieresis",
 "egrave",
 "iacute",
 "icircumflex",
 "idieresis",
 "igrave",
 "ntilde",
 "oacute",
 "ocircumflex",
 "odieresis",
 "ograve",
 "otilde",
 "scaron",
 "uacute",
 "ucircumflex",
 "udieresis",
 "ugrave",
 "yacute",
 "ydieresis",
 "zcaron",
 "exclamsmall",
 "Hungarumlautsmall",
 "dollaroldstyle",
 "dollarsuperior",
 "ampersandsmall",
 "Acutesmall",
 "parenleftsuperior",
 "parenrightsuperior",
 "twodotenleader",
 "onedotenleader",
 "zerooldstyle",
 "oneoldstyle",
 "twooldstyle",
 "threeoldstyle",
 "fouroldstyle",
 "fiveoldstyle",
 "sixoldstyle",
 "sevenoldstyle",
 "eightoldstyle",
 "nineoldstyle",
 "commasuperior",
 "threequartersemdash",
 "periodsuperior",
 "questionsmall",
 "asuperior",
 "bsuperior",
 "centsuperior",
 "dsuperior",
 "esuperior",
 "isuperior",
 "lsuperior",
 "msuperior",
 "nsuperior",
 "osuperior",
 "rsuperior",
 "ssuperior",
 "tsuperior",
 "ff",
 "ffi",
 "ffl",
 "parenleftinferior",
 "parenrightinferior",
 "Circumflexsmall",
 "hyphensuperior",
 "Gravesmall",
 "Asmall",
 "Bsmall",
 "Csmall",
 "Dsmall",
 "Esmall",
 "Fsmall",
 "Gsmall",
 "Hsmall",
 "Ismall",
 "Jsmall",
 "Ksmall",
 "Lsmall",
 "Msmall",
 "Nsmall",
 "Osmall",
 "Psmall",
 "Qsmall",
 "Rsmall",
 "Ssmall",
 "Tsmall",
 "Usmall",
 "Vsmall",
 "Wsmall",
 "Xsmall",
 "Ysmall",
 "Zsmall",
 "colonmonetary",
 "onefitted",
 "rupiah",
 "Tildesmall",
 "exclamdownsmall",
 "centoldstyle",
 "Lslashsmall",
 "Scaronsmall",
 "Zcaronsmall",
 "Dieresissmall",
 "Brevesmall",
 "Caronsmall",
 "Dotaccentsmall",
 "Macronsmall",
 "figuredash",
 "hypheninferior",
 "Ogoneksmall",
 "Ringsmall",
 "Cedillasmall",
 "questiondownsmall",
 "oneeighth",
 "threeeighths",
 "fiveeighths",
 "seveneighths",
 "onethird",
 "twothirds",
 "zerosuperior",
 "foursuperior",
 "fivesuperior",
 "sixsuperior",
 "sevensuperior",
 "eightsuperior",
 "ninesuperior",
 "zeroinferior",
 "oneinferior",
 "twoinferior",
 "threeinferior",
 "fourinferior",
 "fiveinferior",
 "sixinferior",
 "seveninferior",
 "eightinferior",
 "nineinferior",
 "centinferior",
 "dollarinferior",
 "periodinferior",
 "commainferior",
 "Agravesmall",
 "Aacutesmall",
 "Acircumflexsmall",
 "Atildesmall",
 "Adieresissmall",
 "Aringsmall",
 "AEsmall",
 "Ccedillasmall",
 "Egravesmall",
 "Eacutesmall",
 "Ecircumflexsmall",
 "Edieresissmall",
 "Igravesmall",
 "Iacutesmall",
 "Icircumflexsmall",
 "Idieresissmall",
 "Ethsmall",
 "Ntildesmall",
 "Ogravesmall",
 "Oacutesmall",
 "Ocircumflexsmall",
 "Otildesmall",
 "Odieresissmall",
 "OEsmall",
 "Oslashsmall",
 "Ugravesmall",
 "Uacutesmall",
 "Ucircumflexsmall",
 "Udieresissmall",
 "Yacutesmall",
 "Thornsmall",
 "Ydieresissmall",
 "001.000",
 "001.001",
 "001.002",
 "001.003",
 "Black",
 "Bold",
 "Book",
 "Light",
 "Medium",
 "Regular",
 "Roman",
 "Semibold",
 NULL
};
const int nStdStrings = sizeof(cffnames)/sizeof(cffnames[0])-1;

static char **readcfffontnames(FILE *ttf,int *cnt,struct ttfinfo *info) {
    uint16 count = getushort(ttf);
    int offsize;
    uint32 *offsets;
    char **names;
    uint32 i,j;

    if ( cnt!=NULL ) *cnt = count;

    if ( count==0 )
return( NULL );
    offsets = malloc((count+1)*sizeof(uint32));
    offsize = getc(ttf);
    for ( i=0; i<=count; ++i )
	offsets[i] = getoffset(ttf,offsize);
    names = malloc((count+1)*sizeof(char *));
    for ( i=0; i<count; ++i ) {
	if ( offsets[i+1]<offsets[i] ) {
/* GT: The CFF font type contains a thing called a name INDEX, and that INDEX */
/* GT: is bad. It is an index of many of the names used in the CFF font. */
/* GT: We hope the user will never see this. */
	    LogError( _("Bad CFF name INDEX\n") );
	    if ( info!=NULL ) info->bad_cff = true;
	    while ( i<count ) {
		names[i] = copy("");
		++i;
	    }
	    --i;
	} else {
	    names[i] = malloc(offsets[i+1]-offsets[i]+1);
	    for ( j=0; j<offsets[i+1]-offsets[i]; ++j )
		names[i][j] = getc(ttf);
	    names[i][j] = '\0';
	}
    }
    names[i] = NULL;
    free(offsets);
return( names );
}

static char *addnibble(char *pt, int nib) {
    if ( nib<=9 )
	*pt++ = nib+'0';
    else if ( nib==10 )
	*pt++ = '.';
    else if ( nib==11 )
	*pt++ = 'E';
    else if ( nib==12 ) {
	*pt++ = 'E';
	*pt++ = '-';
    } else if ( nib==14 )
	*pt++ = '-';
    else if ( nib==15 )
	*pt++ = '\0';
return( pt );
}

static int readcffthing(FILE *ttf,int *_ival,real *dval,int *operand,struct ttfinfo *info) {
    char buffer[50], *pt;
    int ch, ival;

    ch = getc(ttf);
    if ( ch==12 ) {
	*operand = (12<<8) | getc(ttf);
return( 3 );
    } else if ( ch<=21 ) {
	*operand = ch;
return( 3 );
    } else if ( ch==30 ) {
	/* fixed format doesn't exist in dict data but does in type2 strings */
	pt = buffer;
	do {
	    ch = getc(ttf);
	    if ( pt<buffer+44 || (ch&0xf)==0xf || (ch&0xf0)==0xf0 ) {
		pt = addnibble(pt,ch>>4);
		pt = addnibble(pt,ch&0xf);
	    }
	} while ( pt[-1]!='\0' );
	*dval = strtod(buffer,NULL);
return( 2 );
    } else if ( ch>=32 && ch<=246 ) {
	*_ival = ch-139;
return( 1 );
    } else if ( ch>=247 && ch<=250 ) {
	*_ival = ((ch-247)<<8) + getc(ttf)+108;
return( 1 );
    } else if ( ch>=251 && ch<=254 ) {
	*_ival = -((ch-251)<<8) - getc(ttf)-108;
return( 1 );
    } else if ( ch==28 ) {
	ival = getc(ttf)<<8;
	*_ival = (short) (ival | getc(ttf));
return( 1 );
    } else if ( ch==29 ) {
	/* 4 byte integers exist in dict data but not in type2 strings */
	ival = getc(ttf)<<24;
	ival = ival | getc(ttf)<<16;
	ival = ival | getc(ttf)<<8;
	*_ival = (int) (ival | getc(ttf));
return( 1 );
    }
    LogError(_("Unexpected value in dictionary %d\n"), ch );
    info->bad_cff = true;
    *_ival = 0;
return( 0 );
}

static void skipcfft2thing(FILE *ttf) {
    /* The old CFF spec allows little type2 programs to live in the CFF dict */
    /*  indices. These are designed to allow interpolation of values for mm */
    /*  fonts. */
    /* The Type2 program is terminated by an "endchar" operator */
    /* I don't support this, but I shall try to skip over them properly */
    /* There's no discussion about how values move from the t2 stack to the */
    /*  cff stack, as there are no examples of this, it's hard to guess */
    int ch;

/* GT: DICT is a magic term inside CFF fonts, as is INDEX, and I guess CFF and type2 */
    LogError( _("FontForge does not support type2 programs embedded in CFF DICT INDICES.\n") );
    for (;;) {
	ch = getc(ttf);
	if ( ch>=247 && ch<=254 )
	    getc(ttf);		/* Two byte number */
	else if ( ch==255 ) {
	    getc(ttf); getc(ttf); getc(ttf); getc(ttf);
	    /* 16.16 number */
	} else if ( ch==28 ) {
	    getc(ttf);
	    getc(ttf);
	} else if ( ch==12 ) {
	    getc(ttf);		/* Two byte operator */
	} else if ( ch==14 ) {
return;
	}
    }
}

struct topdicts {
    int32 cff_start;

    char *fontname;	/* From Name Index */

    int version;	/* SID */
    int notice;		/* SID */
    int copyright;	/* SID */
    int fullname;	/* SID */
    int familyname;	/* SID */
    int weight;		/* SID */
    int isfixedpitch;
    real italicangle;
    real underlinepos;
    real underlinewidth;
    int painttype;
    int charstringtype;
    real fontmatrix[6];
    int fontmatrix_set;
    int uniqueid;
    real fontbb[4];
    real strokewidth;
    int xuid[20];
    int charsetoff;	/* from start of file */
    int encodingoff;	/* from start of file */
    int charstringsoff;	/* from start of file */
    int private_size;
    int private_offset;	/* from start of file */
    int synthetic_base;	/* font index */
    int postscript_code;	/* SID */
 /* synthetic fonts only (whatever they are) */
    int basefontname;		/* SID */
 /* Multiple master/synthetic fonts */
    real basefontblend[16];	/* delta */	/* No description of why this is relevant for mm fonts */
 /* Multiple master fonts only */
    int blendaxistypes[17];	/* SID */
    int nMasters;
    int nAxes;
    real weightvector[17];
    int lenBuildCharArray;	/* No description of what this means */
    int NormalizeDesignVector;	/* SID */	/* No description of what this does */
    int ConvertDesignVector;	/* SID */	/* No description of what this does */
 /* CID fonts only */
    int ros_registry;		/* SID */
    int ros_ordering;		/* SID */
    int ros_supplement;
    real cidfontversion;
    int cidfontrevision;
    int cidfonttype;
    int cidcount;
    int uidbase;
    int fdarrayoff;	/* from start of file */
    int fdselectoff;	/* from start of file */
    int sid_fontname;	/* SID */
/* Private stuff */
    real bluevalues[14];
    real otherblues[10];
    real familyblues[14];
    real familyotherblues[10];
    real bluescale;
    real blueshift;
    real bluefuzz;
    int stdhw;
    int stdvw;
    real stemsnaph[10];
    real stemsnapv[10];
    int forcebold;
    real forceboldthreshold;
    int languagegroup;
    real expansionfactor;
    int initialRandomSeed;
    int subrsoff;	/* from start of this private table */
    int defaultwidthx;
    int nominalwidthx;

    struct pschars glyphs;
    struct pschars local_subrs;
    uint16 *charset;
};

static void TopDictFree(struct topdicts *dict) {
    int i;

    free(dict->charset);
    for ( i=0; i<dict->glyphs.cnt; ++i )
	free(dict->glyphs.values[i]);
    free(dict->glyphs.values);
    free(dict->glyphs.lens);
    for ( i=0; i<dict->local_subrs.cnt; ++i )
	free(dict->local_subrs.values[i]);
    free(dict->local_subrs.values);
    free(dict->local_subrs.lens);
    free(dict);
}

static void readcffsubrs(FILE *ttf, struct pschars *subs, struct ttfinfo *info) {
    uint16 count = getushort(ttf);
    int offsize;
    uint32 *offsets;
    int i,j, base;
    int err = false;

    memset(subs,'\0',sizeof(struct pschars));
    if ( count==0 )
return;
    subs->cnt = count;
    subs->lens = malloc(count*sizeof(int));
    subs->values = malloc(count*sizeof(uint8 *));
    offsets = malloc((count+1)*sizeof(uint32));
    offsize = getc(ttf);
    for ( i=0; i<=count; ++i )
	offsets[i] = getoffset(ttf,offsize);
    base = ftell(ttf)-1;
    for ( i=0; i<count; ++i ) {
	if ( offsets[i+1]>offsets[i] && offsets[i+1]-offsets[i]<0x10000 ) {
	    subs->lens[i] = offsets[i+1]-offsets[i];
	    subs->values[i] = malloc(offsets[i+1]-offsets[i]+1);
	    for ( j=0; j+offsets[i]<offsets[i+1]; ++j )
		subs->values[i][j] = getc(ttf);
	    subs->values[i][j] = '\0';
	} else {
	    if ( !err )
		LogError( _("Bad subroutine INDEX in cff font.\n" ));
	    info->bad_cff = true;
	    err = true;
	    subs->lens[i] = 1;
	    subs->values[i] = malloc(2);
	    subs->values[i][0] = 11;		/* return */
	    subs->values[i][1] = '\0';
	    fseek(ttf,base+offsets[i+1],SEEK_SET);
	}
    }
    free(offsets);
}

static struct topdicts *readcfftopdict(FILE *ttf, char *fontname, int len,
	struct ttfinfo *info) {
    struct topdicts *td = calloc(1,sizeof(struct topdicts));
    long base = ftell(ttf);
    int ival, oval, sp, ret, i;
    real stack[50];

    if ( fontname!=NULL )
	ValidatePostScriptFontName(info,fontname);

    td->fontname = fontname;
    td->underlinepos = -100;
    td->underlinewidth = 50;
    td->charstringtype = 2;
    td->fontmatrix[0] = td->fontmatrix[3] = .001;

    td->notice = td->copyright = td->fullname = td->familyname = td->weight = td->version = -1;
    td->postscript_code = td->basefontname = -1;
    td->synthetic_base = td->ros_registry = -1;
    td->fdarrayoff = td->fdselectoff = td->sid_fontname = -1;
    td->blendaxistypes[0] = -1;

    /* Multiple master fonts can have Type2 operators here, particularly */
    /*  blend operators. We're ignoring that */
    while ( ftell(ttf)<base+len ) {
	sp = 0;
	while ( (ret=readcffthing(ttf,&ival,&stack[sp],&oval,info))!=3 && ftell(ttf)<base+len ) {
	    if ( ret==1 )
		stack[sp]=ival;
	    if ( ret!=0 && sp<45 )
		++sp;
	}
	if ( ret==3 && oval==31 /* "T2" operator, can have 0 arguments */ ) {
	    skipcfft2thing(ttf);
	} else if ( sp==0 ) {
	    LogError( _("No argument to operator\n") );
	    info->bad_cff = true;
	} else if ( ret==3 ) switch( oval ) {
	  case 0:
	    td->version = stack[sp-1];
	  break;
	  case 1:
	    td->notice = stack[sp-1];
	  break;
	  case (12<<8)+0:
	    td->copyright = stack[sp-1];
	  break;
	  case 2:
	    td->fullname = stack[sp-1];
	  break;
	  case 3:
	    td->familyname = stack[sp-1];
	  break;
	  case 4:
	    td->weight = stack[sp-1];
	  break;
	  case (12<<8)+1:
	    td->isfixedpitch = stack[sp-1];
	  break;
	  case (12<<8)+2:
	    td->italicangle = stack[sp-1];
	  break;
	  case (12<<8)+3:
	    td->underlinepos = stack[sp-1];
	  break;
	  case (12<<8)+4:
	    td->underlinewidth = stack[sp-1];
	  break;
	  case (12<<8)+5:
	    td->painttype = stack[sp-1];
	  break;
	  case (12<<8)+6:
	    td->charstringtype = stack[sp-1];
	  break;
	  case (12<<8)+7:
	    memcpy(td->fontmatrix,stack,(sp>=6?6:sp)*sizeof(real));
	    td->fontmatrix_set = 1;
	  break;
	  case 13:
	    td->uniqueid = stack[sp-1];
	  break;
	  case 5:
	    memcpy(td->fontbb,stack,(sp>=4?4:sp)*sizeof(real));
	  break;
	  case (12<<8)+8:
	    td->strokewidth = stack[sp-1];
	  break;
	  case 14:
	    for ( i=0; i<sp && i<20; ++i )
		td->xuid[i] = stack[i];
	  break;
	  case 15:
	    td->charsetoff = stack[sp-1];
	  break;
	  case 16:
	    td->encodingoff = stack[sp-1];
	  break;
	  case 17:
	    td->charstringsoff = stack[sp-1];
	  break;
	  case 18:
	    td->private_size = stack[0];
	    td->private_offset = stack[1];
	  break;
	  case (12<<8)+20:
	    LogError( _("FontForge does not support synthetic fonts\n") );
	    td->synthetic_base = stack[sp-1];
	  break;
	  case (12<<8)+21:
	    td->postscript_code = stack[sp-1];
	  break;
	  case (12<<8)+22:
	    td->basefontname = stack[sp-1];
	  break;
	  case (12<<8)+23:
	    for ( i=0; i<sp && i<16; ++i )
		td->basefontblend[i] = stack[i];
	  break;
	  case (12<<8)+24:
	    LogError( _("FontForge does not support type2 multiple master fonts\n") );
	    info->bad_cff = true;
	    td->nMasters = stack[0];
	    td->nAxes = sp-4;
	    memcpy(td->weightvector,stack+1,(sp-4)*sizeof(real));
	    td->lenBuildCharArray = stack[sp-3];
	    td->NormalizeDesignVector = stack[sp-2];	/* These are type2 charstrings, even in type1 fonts */
	    td->ConvertDesignVector = stack[sp-1];
	  break;
	  case (12<<8)+26:
	    for ( i=0; i<sp && i<16; ++i )
		td->blendaxistypes[i] = stack[i];
	    td->blendaxistypes[i] = -1;
	  break;
	  case (12<<8)+30:
	    td->ros_registry = stack[0];
	    td->ros_ordering = stack[1];
	    td->ros_supplement = stack[2];
	  break;
	  case (12<<8)+31:
	    td->cidfontversion = stack[sp-1];
	  break;
	  case (12<<8)+32:
	    td->cidfontrevision = stack[sp-1];
	  break;
	  case (12<<8)+33:
	    td->cidfonttype = stack[sp-1];
	  break;
	  case (12<<8)+34:
	    td->cidcount = stack[sp-1];
	  break;
	  case (12<<8)+35:
	    td->uidbase = stack[sp-1];
	  break;
	  case (12<<8)+36:
	    td->fdarrayoff = stack[sp-1];
	  break;
	  case (12<<8)+37:
	    td->fdselectoff = stack[sp-1];
	  break;
	  case (12<<8)+38:
	    td->sid_fontname = stack[sp-1];
	  break;
	  case (12<<8)+39:
	    LogError(_("FontForge does not support Chameleon fonts\n"));;
	  break;
	  default:
	    LogError(_("Unknown operator in %s: %x\n"), fontname, oval );
	    info->bad_cff = true;
	  break;
	}
    }
return( td );
}

static void readcffprivate(FILE *ttf, struct topdicts *td, struct ttfinfo *info) {
    int ival, oval, sp, ret, i;
    real stack[50];
    int32 end = td->cff_start+td->private_offset+td->private_size;

    fseek(ttf,td->cff_start+td->private_offset,SEEK_SET);

    td->subrsoff = -1;
    td->expansionfactor = .06;
    td->bluefuzz = 1;
    td->blueshift = 7;
    td->bluescale = .039625;

    while ( ftell(ttf)<end ) {
	if ( feof(ttf) ) {
	    LogError(_("End of file found when reading private dictionary.\n") );
    break;
	}
	sp = 0;
	while ( (ret=readcffthing(ttf,&ival,&stack[sp],&oval,info))!=3 && ftell(ttf)<end ) {
	    if ( ret==1 )
		stack[sp]=ival;
	    if ( ret!=0 && sp<45 )
		++sp;
	}
	if ( ret==3 && oval==31 /* "T2" operator, can have 0 arguments */ ) {
	    skipcfft2thing(ttf);
	} else if ( sp==0 && oval!=6 && oval!=7 && oval!=8 && oval!=9 && oval !=(12<<8)+12 && oval !=(12<<8)+13) {
	    LogError( _("No argument to operator %d in private dict\n"), oval );
	    info->bad_cff = true;
	} else if ( ret==3 ) switch( oval ) {
	  case 6:
	    for ( i=0; i<sp && i<14; ++i ) {
		td->bluevalues[i] = stack[i];
		if ( i!=0 )
		    td->bluevalues[i] += td->bluevalues[i-1];
	    }
	    if ( i==0 ) td->bluevalues[0] = 1234567;	/* Marker for an empty arry, which is legal, and different from no array */
	  break;
	  case 7:
	    for ( i=0; i<sp && i<10; ++i ) {
		td->otherblues[i] = stack[i];
		if ( i!=0 )
		    td->otherblues[i] += td->otherblues[i-1];
	    }
	    if ( i==0 ) td->otherblues[0] = 1234567;
	  break;
	  case 8:
	    for ( i=0; i<sp && i<14; ++i ) {
		td->familyblues[i] = stack[i];
		if ( i!=0 )
		    td->familyblues[i] += td->familyblues[i-1];
	    }
	    if ( i==0 ) td->familyblues[0] = 1234567;
	  break;
	  case 9:
	    for ( i=0; i<sp && i<10; ++i ) {
		td->familyotherblues[i] = stack[i];
		if ( i!=0 )
		    td->familyotherblues[i] += td->familyotherblues[i-1];
	    }
	    if ( i==0 ) td->familyotherblues[0] = 1234567;
	  break;
	  case (12<<8)+9:
	    td->bluescale = stack[sp-1];
	  break;
	  case (12<<8)+10:
	    td->blueshift = stack[sp-1];
	  break;
	  case (12<<8)+11:
	    td->bluefuzz = stack[sp-1];
	  break;
	  case 10:
	    td->stdhw = stack[sp-1];
	  break;
	  case 11:
	    td->stdvw = stack[sp-1];
	  break;
	  case (12<<8)+12:
	    for ( i=0; i<sp && i<10; ++i ) {
		td->stemsnaph[i] = stack[i];
		if ( i!=0 )
		    td->stemsnaph[i] += td->stemsnaph[i-1];
	    }
	    if ( i==0 ) td->stemsnaph[0] = 1234567;
	  break;
	  case (12<<8)+13:
	    for ( i=0; i<sp && i<10; ++i ) {
		td->stemsnapv[i] = stack[i];
		if ( i!=0 )
		    td->stemsnapv[i] += td->stemsnapv[i-1];
	    }
	    if ( i==0 ) td->stemsnapv[0] = 1234567;
	  break;
	  case (12<<8)+14:
	    td->forcebold = stack[sp-1];
	  break;
	  case (12<<8)+15:		/* obsolete */
	    td->forceboldthreshold = stack[sp-1];
	  break;
	  case (12<<8)+16:
	    /* lenIV. -1 => unencrypted charstrings */
	    /* obsolete */
	  break;
	  case (12<<8)+17:
	    td->languagegroup = stack[sp-1];
	  break;
	  case (12<<8)+18:
	    td->expansionfactor = stack[sp-1];
	  break;
	  case (12<<8)+19:
	    td->initialRandomSeed = stack[sp-1];
	  break;
	  case 19:
	    td->subrsoff = stack[sp-1];
	  break;
	  case 20:
	    td->defaultwidthx = stack[sp-1];
	  break;
	  case 21:
	    td->nominalwidthx = stack[sp-1];
	  break;
	  default:
	    LogError(_("Unknown operator in %s: %x\n"), td->fontname, oval );
	    info->bad_cff = true;
	  break;
	}
    }

    if ( td->subrsoff!=-1 ) {
	fseek(ttf,td->cff_start+td->private_offset+td->subrsoff,SEEK_SET);
	readcffsubrs(ttf,&td->local_subrs,info);
    }
}

static struct topdicts **readcfftopdicts(FILE *ttf, char **fontnames, int32 cff_start,
	struct ttfinfo *info, struct topdicts *parent_dict) {
    uint16 count = getushort(ttf);
    int offsize;
    uint32 *offsets;
    struct topdicts **dicts;
    int i;

    if ( count==0 )
return( NULL );
    offsets = malloc((count+1)*sizeof(uint32));
    offsize = getc(ttf);
    for ( i=0; i<=count; ++i )
	offsets[i] = getoffset(ttf,offsize);
    dicts = malloc((count+1)*sizeof(struct topdicts *));
    for ( i=0; i<count; ++i ) {
	dicts[i] = readcfftopdict(ttf,fontnames!=NULL?fontnames[i]:NULL,
		offsets[i+1]-offsets[i], info);
	if ( parent_dict!=NULL && parent_dict->fontmatrix_set ) {
	    MatMultiply(parent_dict->fontmatrix,dicts[i]->fontmatrix,dicts[i]->fontmatrix);
	}
	dicts[i]->cff_start = cff_start;
    }
    dicts[i] = NULL;
    free(offsets);
return( dicts );
}

static const char *getsid(int sid,char **strings,int scnt,struct ttfinfo *info) {
    if ( sid==-1 )
return( NULL );
    else if ( sid<nStdStrings )
return( cffnames[sid] );
    else if ( sid-nStdStrings>scnt ) {
	LogError( _("Bad sid %d (must be less than %d)\n"), sid, scnt+nStdStrings );
	if ( info!=NULL ) info->bad_cff = true;
return( NULL );
    } else
return( strings[sid-nStdStrings]);
}

/* I really expect to deal with encodings in ttf cmap, but ocasionally we */
/*  get a bare cff */
static void readcffenc(FILE *ttf,struct topdicts *dict,struct ttfinfo *info,
	char **strings, int scnt) {
    int format, cnt, i, j, pos, first, last, dupenc, sid;
    const char *name;
    EncMap *map;

    if ( info->encoding_start!=0 )		/* Use the cmap instead */
return;
    if ( info->subfontcnt!=0 )
return;						/* Use cids instead */

    for ( i=0; i<info->glyph_cnt; ++i ) {
	if ( info->chars[i]->unicodeenc==-1 )
	    info->chars[i]->unicodeenc = UniFromName(info->chars[i]->name,ui_none,&custom);
    }

    map = EncMapNew(256,256,&custom);
    if ( dict->encodingoff==0 || dict->encodingoff==1 ) {
	/* Standard Encodings */
	char **enc = dict->encodingoff==0 ? (char **)AdobeStandardEncoding : (char **)AdobeExpertEncoding;
	map->enc = FindOrMakeEncoding( dict->encodingoff==0 ?
		"AdobeStandard" : "Custom" );
	if ( map->enc==NULL )
	    map->enc = &custom;
	for ( i=0; i<info->glyph_cnt; ++i ) {
	    for ( pos=0; pos<256; ++pos )
		if ( strcmp(info->chars[i]->name,enc[pos])==0 )
	    break;
	    if ( pos<256 )
		map->map[pos] = i;
	}
    } else {
	fseek(ttf,dict->cff_start+dict->encodingoff,SEEK_SET);
	format = getc(ttf);
        /* Mask off high (additional encoding bit) and check format type */
	if ( (format&0x7f)==0 ) {
            /* format 0 is a 1-1 map of glyph_id to code, starting with id 1 */
	    cnt = getc(ttf);
	    for ( i=1; i<=cnt && i<info->glyph_cnt; ++i )
		map->map[getc(ttf)] = i;
	} else if ( (format&0x7f)==1 ) {
	    cnt = getc(ttf);
            /* CFF encodings start with glyph_id 1 since 0 is always .notdef */
	    pos = 1;
            /* Parse format 1 code ranges */
	    for ( i=0; i<cnt ; ++i ) {
                /* next byte is code of first character in range */
		first = getc(ttf);
                /* next byte is the number of additional characters in range */
		last = first + getc(ttf);
		while ( first<=last && first<256 ) {
		    if ( pos<info->glyph_cnt )
			map->map[first] = pos;
		    ++pos;
		    ++first;
		}
	    }
	} else {
	    LogError( _("Unexpected encoding format in cff: %d\n"), format );
	    if ( info!=NULL ) info->bad_cff = true;
	}
        /* if additional encoding bit set, add all additional encodings */
	if ( format&0x80 ) {
	    cnt = getc(ttf);
	    for ( i=0; i<cnt; ++i ) {
		dupenc = getc(ttf);
		sid = getushort(ttf);
		name = getsid(sid,strings,scnt,info);
		if ( name==NULL )	/* Table is erroneous */
	    break;
		for ( j=0; j<info->glyph_cnt; ++j )
		    if ( strcmp(name,info->chars[j]->name)==0 )
		break;
		if ( j!=info->glyph_cnt )
		    map->map[dupenc] = j;
	    }
	}
    }
    info->map = map;
}

static void readcffset(FILE *ttf,struct topdicts *dict,struct ttfinfo *info) {
    int len = dict->glyphs.cnt;
    int i;
    int format, cnt, j, first;

    i = 0;
    if ( dict->charsetoff==0 ) {
	/* ISO Adobe charset */
	dict->charset = malloc(len*sizeof(uint16));
	for ( i=0; i<len && i<=228; ++i )
	    dict->charset[i] = i;
    } else if ( dict->charsetoff==1 ) {
	/* Expert charset */
	dict->charset = malloc((len<162?162:len)*sizeof(uint16));
	dict->charset[0] = 0;		/* .notdef */
	dict->charset[1] = 1;
	for ( i=2; i<len && i<=238-227; ++i )
	    dict->charset[i] = i+227;
	dict->charset[12] = 13;
	dict->charset[13] = 14;
	dict->charset[14] = 15;
	dict->charset[15] = 99;
	for ( i=16; i<len && i<=248-223; ++i )
	    dict->charset[i] = i+223;
	dict->charset[25] = 27;
	dict->charset[26] = 28;
	for ( i=27; i<len && i<=266-222; ++i )
	    dict->charset[i] = i+222;
	dict->charset[44] = 109;
	dict->charset[45] = 110;
	for ( i=46; i<len && i<=318-221; ++i )
	    dict->charset[i] = i+221;
	dict->charset[96] = 158;
	dict->charset[97] = 155;
	dict->charset[98] = 163;
	for ( i=99; i<len && i<=326-220; ++i )
	    dict->charset[i] = i+220;
	dict->charset[107] = 150;
	dict->charset[108] = 164;
	dict->charset[109] = 169;
	for ( i=110; i<len && i<=378-217; ++i )
	    dict->charset[i] = i+217;
    } else if ( dict->charsetoff==2 ) {
	/* Expert subset charset */
	dict->charset = malloc((len<130?130:len)*sizeof(uint16));
	dict->charset[0] = 0;		/* .notdef */
	dict->charset[1] = 1;
	for ( i=2; i<len && i<=238-227; ++i )
	    dict->charset[i] = i+227;
	dict->charset[12] = 13;
	dict->charset[13] = 14;
	dict->charset[14] = 15;
	dict->charset[15] = 99;
	for ( i=16; i<len && i<=248-223; ++i )
	    dict->charset[i] = i+223;
	dict->charset[25] = 27;
	dict->charset[26] = 28;
	for ( i=27; i<len && i<=266-222; ++i )
	    dict->charset[i] = i+222;
	dict->charset[44] = 109;
	dict->charset[45] = 110;
	for ( i=46; i<len && i<=272-221; ++i )
	    dict->charset[i] = i+221;
	dict->charset[51] = 300;
	dict->charset[52] = 301;
	dict->charset[53] = 302;
	dict->charset[54] = 305;
	dict->charset[55] = 314;
	dict->charset[56] = 315;
	dict->charset[57] = 158;
	dict->charset[58] = 155;
	dict->charset[59] = 163;
	for ( i=60; i<len && i<=326-260; ++i )
	    dict->charset[i] = i+260;
	dict->charset[67] = 150;
	dict->charset[68] = 164;
	dict->charset[69] = 169;
	for ( i=110; i<len && i<=346-217; ++i )
	    dict->charset[i] = i+217;
    } else {
	dict->charset = malloc(len*sizeof(uint16));
	dict->charset[0] = 0;		/* .notdef */
	fseek(ttf,dict->cff_start+dict->charsetoff,SEEK_SET);
	format = getc(ttf);
	if ( format==0 ) {
	    for ( i=1; i<len; ++i )
		dict->charset[i] = getushort(ttf);
	} else if ( format==1 ) {
	    for ( i = 1; i<len; ) {
		first = dict->charset[i++] = getushort(ttf);
		cnt = getc(ttf);
		for ( j=0; j<cnt; ++j )
		    dict->charset[i++] = ++first;
	    }
	} else if ( format==2 ) {
	    for ( i = 1; i<len; ) {
		first = dict->charset[i++] = getushort(ttf);
		cnt = getushort(ttf);
		for ( j=0; j<cnt; ++j )
		    dict->charset[i++] = ++first;
	    }
	} else {
	    LogError( _("Unexpected charset format in cff: %d\n"), format );
	    if ( info!=NULL ) info->bad_cff = true;
	}
    }
    while ( i<len ) dict->charset[i++] = 0;
}

static uint8 *readfdselect(FILE *ttf,int numglyphs,struct ttfinfo *info) {
    uint8 *fdselect = calloc(numglyphs,sizeof(uint8));
    int i, j, format, nr, first, end, fd;

    format = getc(ttf);
    if ( format==0 ) {
	for ( i=0; i<numglyphs; ++i )
	    fdselect[i] = getc(ttf);
    } else if ( format==3 ) {
	nr = getushort(ttf);
	first = getushort(ttf);
	for ( i=0; i<nr; ++i ) {
	    fd = getc(ttf);
	    end = getushort(ttf);
	    for ( j=first; j<end; ++j ) {
		if ( j>=numglyphs ) {
		    LogError( _("Bad fdselect\n") );
		    if ( info!=NULL ) info->bad_cff = true;
		} else
		    fdselect[j] = fd;
	    }
	    first = end;
	}
    } else {
	LogError( _("Didn't understand format for fdselect %d\n"), format );
	if ( info!=NULL ) info->bad_cff = true;
    }
return( fdselect );
}


static char *intarray2str(int *array, int size) {
    int i,j;
    char *pt, *ret;

    for ( i=size-1; i>=0 && array[i]==0; --i );
    if ( i==-1 )
return( NULL );
    ret = pt = malloc((i+1)*12+12);
    *pt++ = '[';
    for ( j=0; j<=i; ++j ) {
	sprintf( pt, "%d ", array[j]);
	pt += strlen(pt);
    }
    pt[-1]=']';
return( ret );
}

static char *realarray2str(real *array, int size, int must_be_even) {
    int i,j;
    char *pt, *ret;

    for ( i=size-1; i>=0 && array[i]==0; --i );
    if ( i==-1 )
return( NULL );
    if ( i==0 && array[0]==1234567 ) /* Special marker for a null array */
return( copy( "[]" ));
    if ( must_be_even && !(i&1) && array[i]<0 )
	++i;			/* Someone gave us a bluevalues of [-20 0] and we reported [-20] */
    ret = pt = malloc((i+1)*20+12);
    *pt++ = '[';
    for ( j=0; j<=i; ++j ) {
	sprintf( pt, "%g ", (double) array[j]);
	pt += strlen(pt);
    }
    pt[-1]=']';
return( ret );
}

static void privateadd(struct psdict *private,char *key,char *value) {
    if ( value==NULL )
return;
    private->keys[private->next] = copy(key);
    private->values[private->next++] = value;
}

static void privateaddint(struct psdict *private,char *key,int val) {
    char buf[20];
    if ( val==0 )
return;
    sprintf( buf,"%d", val );
    privateadd(private,key,copy(buf));
}

static void privateaddintarray(struct psdict *private,char *key,int val) {
    char buf[20];
    if ( val==0 )
return;
    sprintf( buf,"[%d]", val );
    privateadd(private,key,copy(buf));
}

static void privateaddreal(struct psdict *private,char *key,double val,double def) {
    char buf[40];
    if ( val==def )
return;
    sprintf( buf,"%g", val );
    privateadd(private,key,copy(buf));
}

static void cffprivatefillup(struct psdict *private, struct topdicts *dict) {
    private->cnt = 14;
    private->keys = malloc(14*sizeof(char *));
    private->values = malloc(14*sizeof(char *));
    privateadd(private,"BlueValues",
	    realarray2str(dict->bluevalues,sizeof(dict->bluevalues)/sizeof(dict->bluevalues[0]),true));
    privateadd(private,"OtherBlues",
	    realarray2str(dict->otherblues,sizeof(dict->otherblues)/sizeof(dict->otherblues[0]),true));
    privateadd(private,"FamilyBlues",
	    realarray2str(dict->familyblues,sizeof(dict->familyblues)/sizeof(dict->familyblues[0]),true));
    privateadd(private,"FamilyOtherBlues",
	    realarray2str(dict->familyotherblues,sizeof(dict->familyotherblues)/sizeof(dict->familyotherblues[0]),true));
    privateaddreal(private,"BlueScale",dict->bluescale,0.039625);
    privateaddreal(private,"BlueShift",dict->blueshift,7);
    privateaddreal(private,"BlueFuzz",dict->bluefuzz,1);
    privateaddintarray(private,"StdHW",dict->stdhw);
    privateaddintarray(private,"StdVW",dict->stdvw);
    privateadd(private,"StemSnapH",
	    realarray2str(dict->stemsnaph,sizeof(dict->stemsnaph)/sizeof(dict->stemsnaph[0]),false));
    privateadd(private,"StemSnapV",
	    realarray2str(dict->stemsnapv,sizeof(dict->stemsnapv)/sizeof(dict->stemsnapv[0]),false));
    if ( dict->forcebold )
	privateadd(private,"ForceBold",copy("true"));
    if ( dict->forceboldthreshold!=0 )
	privateaddreal(private,"ForceBoldThreshold",dict->forceboldthreshold,0);
    privateaddint(private,"LanguageGroup",dict->languagegroup);
    privateaddreal(private,"ExpansionFactor",dict->expansionfactor,0.06);
}

static SplineFont *cffsffillup(struct topdicts *subdict, char **strings,
	int scnt, struct ttfinfo *info) {
    SplineFont *sf = SplineFontEmpty();
    int emsize;
    static int nameless;

    sf->fontname = utf8_verify_copy(getsid(subdict->sid_fontname,strings,scnt,info));
    if ( sf->fontname==NULL ) {
	char buffer[40];
	sprintf(buffer,"UntitledSubFont_%d", ++nameless );
	sf->fontname = copy(buffer);
    }

    if ( subdict->fontmatrix[0]==0 )
	emsize = 1000;
    else
	emsize = rint( 1/subdict->fontmatrix[0] );
    sf->ascent = .8*emsize;
    sf->descent = emsize - sf->ascent;
    if ( subdict->copyright!=-1 )
	sf->copyright = utf8_verify_copy(getsid(subdict->copyright,strings,scnt,info));
    else
	sf->copyright = utf8_verify_copy(getsid(subdict->notice,strings,scnt,info));
    sf->familyname = utf8_verify_copy(getsid(subdict->familyname,strings,scnt,info));
    sf->fullname = utf8_verify_copy(getsid(subdict->fullname,strings,scnt,info));
    sf->weight = utf8_verify_copy(getsid(subdict->weight,strings,scnt,info));
    sf->version = utf8_verify_copy(getsid(subdict->version,strings,scnt,info));
    sf->italicangle = subdict->italicangle;
    sf->upos = subdict->underlinepos;
    sf->uwidth = subdict->underlinewidth;
    sf->xuid = intarray2str(subdict->xuid,sizeof(subdict->xuid)/sizeof(subdict->xuid[0]));
    sf->uniqueid = subdict->uniqueid;
    sf->strokewidth = subdict->strokewidth;
    sf->strokedfont = subdict->painttype==2;

    if ( subdict->private_size>0 ) {
	sf->private = calloc(1,sizeof(struct psdict));
	cffprivatefillup(sf->private,subdict);
    }
return( sf );
}

static void cffinfofillup(struct ttfinfo *info, struct topdicts *dict,
	char **strings, int scnt ) {

    info->glyph_cnt = dict->glyphs.cnt;
    if ( info->glyph_cnt<0 ) info->glyph_cnt = 0;

    if ( dict->fontmatrix[0]==0 )
	info->emsize = 1000;
    else
	info->emsize = rint( 1/dict->fontmatrix[0] );
    info->ascent = .8*info->emsize;
    info->descent = info->emsize - info->ascent;
    if ( dict->copyright!=-1 || dict->notice!=-1 )
	free( info->copyright );
    if ( dict->copyright!=-1 )
	info->copyright = utf8_verify_copy(getsid(dict->copyright,strings,scnt,info));
    else if ( dict->notice!=-1 )
	info->copyright = utf8_verify_copy(getsid(dict->notice,strings,scnt,info));
    if ( dict->familyname!=-1 ) {
	free(info->familyname);
	info->familyname = utf8_verify_copy(getsid(dict->familyname,strings,scnt,info));
    }
    if ( dict->fullname!=-1 ) {
	free(info->fullname);
	info->fullname = utf8_verify_copy(getsid(dict->fullname,strings,scnt,info));
    }
    if ( dict->weight!=-1 ) {
	free(info->weight);
	info->weight = utf8_verify_copy(getsid(dict->weight,strings,scnt,info));
    }
    if ( dict->version!=-1 ) {
	free(info->version);
	info->version = utf8_verify_copy(getsid(dict->version,strings,scnt,info));
    }
    if ( dict->fontname!=NULL ) {
	free(info->fontname);
	info->fontname = utf8_verify_copy(dict->fontname);
    }
    info->italicAngle = dict->italicangle;
    info->upos = dict->underlinepos;
    info->uwidth = dict->underlinewidth;
    info->xuid = intarray2str(dict->xuid,sizeof(dict->xuid)/sizeof(dict->xuid[0]));
    info->uniqueid = dict->uniqueid;
    info->strokewidth = dict->strokewidth;
    info->strokedfont = dict->painttype==2;

    if ( dict->private_size>0 ) {
	info->private = calloc(1,sizeof(struct psdict));
	cffprivatefillup(info->private,dict);
    }
    if ( dict->ros_registry!=-1 ) {
	info->cidregistry = copy(getsid(dict->ros_registry,strings,scnt,info));
	info->ordering = copy(getsid(dict->ros_ordering,strings,scnt,info));
	info->supplement = dict->ros_supplement;
	info->cidfontversion = dict->cidfontversion;
    }
}

static void cfffigure(struct ttfinfo *info, struct topdicts *dict,
	char **strings, int scnt, struct pschars *gsubrs) {
    int i, cstype;
    struct pschars *subrs;
    struct pscontext pscontext;

    memset(&pscontext,0,sizeof(pscontext));

    cffinfofillup(info, dict, strings, scnt );

/* The format allows for some dicts that are type1 strings and others that */
/*  are type2s. Which means that the global subrs will have a different bias */
/*  as we flip from font to font. So we can't set the bias when we read in */
/*  the subrs but must wait until we know which font we're working on. */
    cstype = dict->charstringtype;
    pscontext.is_type2 = cstype-1;
    pscontext.painttype = dict->painttype;
    gsubrs->bias = cstype==1 ? 0 :
	    gsubrs->cnt < 1240 ? 107 :
	    gsubrs->cnt <33900 ? 1131 : 32768;
    subrs = &dict->local_subrs;
    subrs->bias = cstype==1 ? 0 :
	    subrs->cnt < 1240 ? 107 :
	    subrs->cnt <33900 ? 1131 : 32768;

    info->chars = calloc(info->glyph_cnt,sizeof(SplineChar *));
    for ( i=0; i<info->glyph_cnt; ++i ) {
	info->chars[i] = PSCharStringToSplines(
		dict->glyphs.values[i], dict->glyphs.lens[i],&pscontext,
		subrs,gsubrs,getsid(dict->charset[i],strings,scnt,info));
	info->chars[i]->vwidth = info->emsize;
	if ( cstype==2 ) {
	    if ( info->chars[i]->width == (int16) 0x8000 )
		info->chars[i]->width = dict->defaultwidthx;
	    else
		info->chars[i]->width += dict->nominalwidthx;
	}
    }
    /* Need to do a reference fixup here !!!!! just in case some idiot */
    /*  used type1 char strings -- or used the deprecated meaning of */
    /*  endchar (==seac) */
}

static void cidfigure(struct ttfinfo *info, struct topdicts *dict,
	char **strings, int scnt, struct pschars *gsubrs, struct topdicts **subdicts,
	uint8 *fdselect) {
    int i, j, cstype, uni, cid;
    struct pschars *subrs;
    SplineFont *sf;
    struct cidmap *map;
    char buffer[100];
    struct pscontext pscontext;
    EncMap *encmap = NULL;

    memset(&pscontext,0,sizeof(pscontext));

    cffinfofillup(info, dict, strings, scnt );

    /* We'll set the encmap later */
    /*info->map = encmap = EncMapNew(info->glyph_cnt,info->glyph_cnt,&custom);*/

    for ( j=0; subdicts[j]!=NULL; ++j );
    info->subfontcnt = j;
    info->subfonts = calloc(j+1,sizeof(SplineFont *));
    for ( j=0; subdicts[j]!=NULL; ++j )  {
	info->subfonts[j] = cffsffillup(subdicts[j],strings,scnt,info);
	info->subfonts[j]->map = encmap;
    }
    for ( i=0; i<info->glyph_cnt; ++i ) {
	sf = info->subfonts[ fdselect[i] ];
	cid = dict->charset[i];
	if ( cid>=sf->glyphcnt ) sf->glyphcnt = sf->glyphmax = cid+1;
	/*if ( cid>=encmap->enccount ) encmap->enccount = cid+1;*/
    }
    for ( j=0; subdicts[j]!=NULL; ++j )
	info->subfonts[j]->glyphs = calloc(info->subfonts[j]->glyphcnt,sizeof(SplineChar *));
    /*encmap->encmax = encmap->enccount;*/
    /*encmap->map = malloc(encmap->enccount*sizeof(int));*/
    /*memset(encmap->map,-1,encmap->enccount*sizeof(int));*/

    info->chars = calloc(info->glyph_cnt,sizeof(SplineChar *));

    /* info->chars provides access to the chars ordered by glyph, which the */
    /*  ttf routines care about */
    /* sf->glyphs provides access to the chars ordered by CID. Not sure what */
    /*  would happen to a kern from one font to another... */

    map = FindCidMap(info->cidregistry,info->ordering,info->supplement,NULL);

    for ( i=0; i<info->glyph_cnt; ++i ) {
	j = fdselect[i];
	sf = info->subfonts[ j ];
/* The format allows for some dicts that are type1 strings and others that */
/*  are type2s. Which means that the global subrs will have a different bias */
/*  as we flip from font to font. So we can't set the bias when we read in */
/*  the subrs but must wait until we know which font we're working on. */
	cstype = subdicts[j]->charstringtype;
	pscontext.is_type2 = cstype-1;
	pscontext.painttype = subdicts[j]->painttype;
	gsubrs->bias = cstype==1 ? 0 :
		gsubrs->cnt < 1240 ? 107 :
		gsubrs->cnt <33900 ? 1131 : 32768;
	subrs = &subdicts[j]->local_subrs;
	subrs->bias = cstype==1 ? 0 :
		subrs->cnt < 1240 ? 107 :
		subrs->cnt <33900 ? 1131 : 32768;

	cid = dict->charset[i];
	/*encmap->map[cid] = cid;*/
	uni = CID2NameUni(map,cid,buffer,sizeof(buffer));
	info->chars[i] = PSCharStringToSplines(
		dict->glyphs.values[i], dict->glyphs.lens[i],&pscontext,
		subrs,gsubrs,buffer);
	info->chars[i]->vwidth = sf->ascent+sf->descent;
	info->chars[i]->unicodeenc = uni;
	info->chars[i]->altuni = CIDSetAltUnis(map,cid);
	sf->glyphs[cid] = info->chars[i];
	sf->glyphs[cid]->parent = sf;
	sf->glyphs[cid]->orig_pos = cid;		/* Bug! should be i, but I assume sf->chars[orig_pos]->orig_pos==orig_pos */
	if ( sf->glyphs[cid]->layers[ly_fore].refs!=NULL )
	    IError( "Reference found in CID font. Can't fix it up");
	if ( cstype==2 ) {
	    if ( sf->glyphs[cid]->width == (int16) 0x8000 )
		sf->glyphs[cid]->width = subdicts[j]->defaultwidthx;
	    else
		sf->glyphs[cid]->width += subdicts[j]->nominalwidthx;
	}
	ff_progress_next();
    }
    /* No need to do a reference fixup here-- the chars aren't associated */
    /*  with any encoding as is required for seac */
}

static int readcffglyphs(FILE *ttf,struct ttfinfo *info) {
    int offsize;
    int hdrsize;
    char **fontnames, **strings;
    struct topdicts **dicts, **subdicts;
    int i, j, which;
    struct pschars gsubs;
    uint8 *fdselect;
    int scnt;

    fseek(ttf,info->cff_start,SEEK_SET);
    if ( getc(ttf)!='\1' ) {		/* Major version */
	LogError( _("CFF version mismatch\n" ));
	info->bad_cff = true;
return( 0 );
    }
    getc(ttf);				/* Minor version */
    hdrsize = getc(ttf);
    offsize = getc(ttf);
    if ( hdrsize!=4 )
	fseek(ttf,info->cff_start+hdrsize,SEEK_SET);
    fontnames = readcfffontnames(ttf,NULL,info);
    which = 0;
    if ( fontnames[1]!=NULL ) {		/* More than one? Can that even happen in OpenType? */
	which = PickCFFFont(fontnames);
	if ( which==-1 ) {
	    for ( i=0; fontnames[i]!=NULL; ++i )
		free(fontnames[i]);
	    free(fontnames);
return( 0 );
	}
    }
    dicts = readcfftopdicts(ttf,fontnames,info->cff_start,info, NULL);
	/* String index is just the same as fontname index */
    strings = readcfffontnames(ttf,&scnt,info);
    readcffsubrs(ttf,&gsubs,info );
    /* Can be many fonts here. Only decompose the one */
    if ( dicts[which]->charstringsoff!=-1 ) {
	fseek(ttf,info->cff_start+dicts[which]->charstringsoff,SEEK_SET);
	readcffsubrs(ttf,&dicts[which]->glyphs,info);
    }
    if ( dicts[which]->private_offset!=-1 )
	readcffprivate(ttf,dicts[which],info);
    if ( dicts[which]->charsetoff!=-1 )
	readcffset(ttf,dicts[which],info);
    if ( dicts[which]->fdarrayoff==-1 )
	cfffigure(info,dicts[which],strings,scnt,&gsubs);
    else {
	fseek(ttf,info->cff_start+dicts[which]->fdarrayoff,SEEK_SET);
	subdicts = readcfftopdicts(ttf,NULL,info->cff_start,info,dicts[which]);
	fseek(ttf,info->cff_start+dicts[which]->fdselectoff,SEEK_SET);
	fdselect = readfdselect(ttf,dicts[which]->glyphs.cnt,info);
	for ( j=0; subdicts[j]!=NULL; ++j ) {
	    if ( subdicts[j]->private_offset!=-1 )
		readcffprivate(ttf,subdicts[j],info);
	    if ( subdicts[j]->charsetoff!=-1 )
		readcffset(ttf,subdicts[j],info);
	}
	cidfigure(info,dicts[which],strings,scnt,&gsubs,subdicts,fdselect);
	for ( j=0; subdicts[j]!=NULL; ++j )
	    TopDictFree(subdicts[j]);
	free(subdicts); free(fdselect);
    }
    if ( dicts[which]->encodingoff!=-1 )
	readcffenc(ttf,dicts[which],info,strings,scnt);

    if ( dicts[which]->fdarrayoff==-1 ) {
	for ( i=0; i<info->glyph_cnt ; ++i )
	    if ( info->chars[i]!=NULL )
		info->chars[i]->orig_pos = i;
    }

    if ( info->to_order2 ) {
	for ( i=0; i<info->glyph_cnt; ++i )
	    SCConvertToOrder2(info->chars[i]);
    }

    if (fontnames[0] != NULL) {
	free(fontnames[0]);
	TopDictFree(dicts[0]);
    }

    free(fontnames); free(dicts);
    if ( strings!=NULL ) {
	for ( i=0; strings[i]!=NULL; ++i )
	    free(strings[i]);
	free(strings);
    }
    for ( i=0; i<gsubs.cnt; ++i )
	free(gsubs.values[i]);
    free(gsubs.values); free(gsubs.lens);

return( 1 );
}

static int readtyp1glyphs(FILE *ttf,struct ttfinfo *info) {
    FontDict *fd;
    FILE *tmp;
    int i;
    SplineChar *sc;

    fseek(ttf,info->typ1_start,SEEK_SET);
/* There appear to be about 20 bytes of garbage (well, I don't know what they */
/*  mean, so they are garbage to me) before the start of the PostScript. But */
/*  it's not exactly 20. I've seen 22 and 24. So see if we can find "%!PS-Adobe" */
/*  in the first few bytes of the file, and skip to there if found */
    { char buffer[41];
	fread(buffer,1,sizeof(buffer),ttf);
	buffer[40] = '\0';
	for ( i=39; i>=0; --i )
	    if ( buffer[i]=='%' && buffer[i+1]=='!' )
	break;
	if ( i<0 )
	    i = 0;
	fseek(ttf,info->typ1_start+i,SEEK_SET);
    }
    
    tmp = tmpfile();
    for ( i=0; i<info->typ1_length; ++i )
	putc(getc(ttf),tmp);
    rewind(tmp);
    fd = _ReadPSFont(tmp);
    fclose(tmp);
    if ( fd!=NULL ) {
	SplineFont *sf = SplineFontFromPSFont(fd);
	PSFontFree(fd);
	info->emsize = (sf->ascent+sf->descent);
	info->ascent = sf->ascent;
	info->descent = sf->descent;
	if ( sf->subfontcnt!=0 ) {
	    info->subfontcnt = sf->subfontcnt;
	    info->subfonts = sf->subfonts;
	    info->cidregistry = copy(sf->cidregistry);
	    info->ordering = copy(sf->ordering);
	    info->supplement = sf->supplement;
	    info->cidfontversion = sf->cidversion;
	    sf->subfonts = NULL;
	    sf->subfontcnt = 0;
	} else {
	    info->chars = sf->glyphs;
	    info->glyph_cnt = sf->glyphcnt;
	    for ( i=sf->glyphcnt-1; i>=0; --i ) if ( (sc=sf->glyphs[i])!=NULL )
		sc->parent = NULL;
	    sf->glyphs = NULL;
	    sf->glyphcnt = 0;
	}
	SplineFontFree(sf);
return( true );
    }
return( false );
}

static void readttfwidths(FILE *ttf,struct ttfinfo *info) {
    int i,j;
    int lastwidth = info->emsize, lsb;
    /* I'm not interested in the lsb, I'm not sure what it means if it differs*/
    /*  from that is specified in the outline. Do we move the outline? */
    /* Ah... I am interested in it if bit 1 of 'head'.flags is set, then we */
    /*  do move the outline */
    int check_width_consistency = info->cff_start!=0 && info->glyph_start==0;
    SplineChar *sc;
    real trans[6];

    memset(trans,0,sizeof(trans));
    trans[0] = trans[3] = 1;

    fseek(ttf,info->hmetrics_start,SEEK_SET);
    for ( i=0; i<info->width_cnt && i<info->glyph_cnt; ++i ) {
	lastwidth = getushort(ttf);
	lsb = (short) getushort(ttf);
	if ( (sc = info->chars[i])!=NULL ) {	/* can happen in ttc files */
	    if ( lastwidth>info->advanceWidthMax && info->hhea_start!=0 ) {
		if ( !info->wdthcomplain || (info->openflags&of_fontlint)) {
		    if ( info->fontname!=NULL && sc->name!=NULL )
			LogError(_("In %s, the advance width (%d) for glyph %s is greater than the maximum (%d)\n"),
				info->fontname, lastwidth, sc->name, info->advanceWidthMax );
		    else
			LogError(_("In GID %d the advance width (%d) is greater than the stated maximum (%d)\n"),
				i, lastwidth, info->advanceWidthMax );
		    if ( !(info->openflags&of_fontlint) )
			LogError(_("  Subsequent errors will not be reported.\n") );
		    info->wdthcomplain = true;
		}
	    }
	    if ( check_width_consistency && sc->width!=lastwidth ) {
		if ( info->fontname!=NULL && sc->name!=NULL )
		    LogError(_("In %s, in glyph %s, 'CFF ' advance width (%d) and\n  'hmtx' width (%d) do not match. (Subsequent mismatches will not be reported)\n"),
			    info->fontname, sc->name, sc->width, lastwidth );
		else
		    LogError(_("In GID %d, 'CFF ' advance width (%d) and 'hmtx' width (%d) do not match.\n  (Subsequent mismatches will not be reported)\n"),
			    i, sc->width, lastwidth );
		info->bad_metrics = true;
		check_width_consistency = false;
	    }
	    sc->width = lastwidth;
	    sc->widthset = true;
	    if ( info->apply_lsb ) {
		if ( sc->lsidebearing!=lsb ) {
		    trans[4] = lsb-sc->lsidebearing;
		    SplinePointListTransform(sc->layers[ly_fore].splines,trans,tpt_AllPoints);
		}
	    }
	}
    }
    if ( i==0 ) {
	LogError( _("Invalid ttf hmtx table (or hhea), numOfLongMetrics is 0\n") );
	info->bad_metrics = true;
    }
	
    for ( j=i; j<info->glyph_cnt; ++j ) {
	if ( (sc = info->chars[j])!=NULL ) {	/* In a ttc file we may skip some */
	    sc->width = lastwidth;
	    sc->widthset = true;
	    if ( info->apply_lsb ) {
		lsb = (short) getushort(ttf);
		if ( sc->lsidebearing!=lsb ) {
		    trans[4] = lsb-sc->lsidebearing;
		    SplinePointListTransform(sc->layers[ly_fore].splines,trans,tpt_AllPoints);
		}
	    }
	}
    }
}

static void dummywidthsfromstrike(FILE *ttf,struct ttfinfo *info) {
    BDFFont *bdf;
    int i, cnt;
    double scaled_sum;

    if ( info->bitmaps==NULL )
return;
    for ( i=0; i<info->glyph_cnt; ++i ) if ( info->chars[i]!=NULL ) {
	cnt = 0; scaled_sum = 0;
	for ( bdf=info->bitmaps; bdf->next!=NULL; bdf=bdf->next ) {
	    if ( i<bdf->glyphcnt && bdf->glyphs[i]!=NULL ) {
		scaled_sum += ((double) (info->emsize*bdf->glyphs[i]->width))/bdf->pixelsize;
		++cnt;
	    }
	}
	if ( cnt!=0 ) {
	    info->chars[i]->width = scaled_sum/cnt;
	    info->chars[i]->widthset = true;
	}
    }
}

static void readttfvwidths(FILE *ttf,struct ttfinfo *info) {
    int i,j;
    int lastvwidth = info->emsize, vwidth_cnt, tsb/*, cnt=0*/;
    /* int32 voff=0; */

    fseek(ttf,info->vhea_start+4+4,SEEK_SET);		/* skip over the version number & typo right/left */
    info->pfminfo.vlinegap = getushort(ttf);
    info->pfminfo.vheadset = true;

    for ( i=0; i<12; ++i )
	getushort(ttf);
    vwidth_cnt = getushort(ttf);

    fseek(ttf,info->vmetrics_start,SEEK_SET);
    for ( i=0; i<vwidth_cnt && i<info->glyph_cnt; ++i ) {
	lastvwidth = getushort(ttf);
	tsb = getushort(ttf);
	if ( info->chars[i]!=NULL )		/* can happen in ttc files */
	    info->chars[i]->vwidth = lastvwidth;
    }
    if ( i==0 ) {
	LogError( _("Invalid ttf vmtx table (or vhea), numOfLongVerMetrics is 0\n") );
	info->bad_metrics = true;
    }

    for ( j=i; j<info->glyph_cnt; ++j ) {
	if ( info->chars[j]!=NULL )		/* In a ttc file we may skip some */
	    info->chars[j]->vwidth = lastvwidth;
    }

}

static int modenc(int enc,int modtype) {
return( enc );
}

static int badencoding(struct ttfinfo *info) {
    if ( !info->bad_cmap ) {
	LogError(_("Bad encoding information in 'cmap' table."));
	info->bad_cmap = true;
    }
return( -1 );
}

static int umodenc(int enc,int modtype, struct ttfinfo *info) {
    if ( modtype==-1 )
return( -1 );
    if ( modtype<=1 /* Unicode */ ) {
	/* No conversion needed, already unicode */;
    } else if ( modtype==2 /* SJIS */ ) {
	if ( enc<=127 ) {
	    /* Latin */
	    if ( enc=='\\' ) enc = 0xa5;	/* Yen */
	} else if ( enc>=161 && enc<=223 ) {
	    /* Katakana */
	    enc = unicode_from_jis201[enc];
	} else if ( enc<255 ) {
	    /* This is erroneous as I understand SJIS */
	    enc = badencoding(info);
	} else if (enc >= 0xeaa5) {
        /* Encoded value is outside SJIS range */
        /* If this happens, it's likely that it's actually CP932 encoded */
        /* Todo: Detect CP932 encoding earlier and apply that instead of SJIS */
        enc = badencoding(info);
	} else {
	    int ch1 = enc>>8, ch2 = enc&0xff;
	    if ( ch1 >= 129 && ch1<= 159 )
		ch1 -= 112;
	    else
		ch1 -= 176;
	    ch1 <<= 1;
	    if ( ch2>=159 )
		ch2-= 126;
	    else if ( ch2>127 ) {
		--ch1;
		ch2 -= 32;
	    } else {
		--ch1;
		ch2 -= 31;
	    }
	    if ( ch1<0x21 || ch2<0x21 || ch1>0x7e || ch2>0x7e )
		enc = badencoding(info);
	    else
		enc = unicode_from_jis208[(ch1-0x21)*94+(ch2-0x21)];
	}
    } else if ( modtype==3 /* GB2312 offset by 0x8080, parse just like wansung */ ) {
	if ( enc>0xa1a1 ) {
	    enc -= 0xa1a1;
	    enc = (enc>>8)*94 + (enc&0xff);
	    enc = unicode_from_gb2312[enc];
	    if ( enc==0 ) enc = -1;
	} else if ( enc>0x100 )
	    enc = badencoding(info);
    } else if ( modtype==4 /* BIG5 */ ) {	/* old ms docs say big5 is modtype==3, but new ones say 4 */
	if ( enc>0x8100 )
	    enc = unicode_from_big5hkscs[enc-0x8100];
	else if ( enc>0x100 )
	    enc = badencoding(info);
    } else if ( modtype==5 /* Wansung == KSC 5601-1987, I hope */ ) {
	if ( enc>0xa1a1 ) {
	    enc -= 0xa1a1;
	    enc = (enc>>8)*94 + (enc&0xff);
	    enc = unicode_from_ksc5601[enc];
	    if ( enc==0 ) enc = -1;
	} else if ( enc>0x100 )
	    enc = badencoding(info);
    } else if ( modtype==6 /* Johab */ ) {
	if ( enc>0x8400 )
	    enc = unicode_from_johab[enc-0x8400];
	else if ( enc>0x100 )
	    enc = badencoding(info);
    }
    if ( enc==0 )
	enc = -1;
return( enc );
}

struct cmap_encs {
    int platform;
    int specific;
    int offset;
    int lang;
    int format;
    Encoding *enc;
};

static int SubtableIsntSupported(FILE *ttf,uint32 offset,struct cmap_encs *cmap_enc, struct ttfinfo *info) {
    uint32 here = ftell(ttf);
    int format, len, ret=false;

    fseek(ttf,offset,SEEK_SET);

    cmap_enc->format = format = getushort(ttf);
    if ( format<0 || (format&1) || format>12 ) {
	LogError( _("Encoding subtable for platform=%d, specific=%d has an unsupported format %d.\n"),
		cmap_enc->platform, cmap_enc->specific, format );
	info->bad_cmap = true;
	ret = true;
    }

    if ( format!=12 && format!=10 && format!=8 ) {
	len = getushort(ttf);
	cmap_enc->lang = getushort(ttf);
    } else {
	/* padding */ getushort(ttf);
	len = getlong(ttf);
	cmap_enc->lang = getlong(ttf);
    }
    if ( len==0 ) {
	LogError( _("Encoding subtable for platform=%d, specific=%d has a 0 length subtable.\n"),
		cmap_enc->platform, cmap_enc->specific );
	info->bad_cmap = true;
	ret = true;
    }
    fseek(ttf,here,SEEK_SET);
return( ret );
}

static int SubtableMustBe14(FILE *ttf,uint32 offset,struct ttfinfo *info) {
    uint32 here = ftell(ttf);
    int format, ret=true;

    fseek(ttf,offset,SEEK_SET);

    format = getushort(ttf);
    if ( format!=14 ) {
	LogError( _("Encoding subtable for platform=%d, specific=%d (which must be 14)\nhas an unsupported format %d.\n"),
		0, 5, format );
	info->bad_cmap = true;
	ret = false;
    }
    fseek(ttf,here,SEEK_SET);
return( ret );
}

static void ApplyVariationSequenceSubtable(FILE *ttf,uint32 vs_map,
	struct ttfinfo *info,int justinuse) {
    int sub_table_len, vs_cnt, i, j, rcnt, gid, cur_gid;
    struct vs_data { int vs; uint32 def, non_def; } *vs_data;
    SplineChar *sc;

    fseek(ttf,vs_map,SEEK_SET);
    /* We/ve already checked the format is 14 */ getushort(ttf);
    sub_table_len = getlong(ttf);
    vs_cnt = getlong(ttf);
    vs_data = malloc(vs_cnt*sizeof(struct vs_data));
    for ( i=0; i<vs_cnt; ++i ) {
	vs_data[i].vs = get3byte(ttf);
	vs_data[i].def = getlong(ttf);
	vs_data[i].non_def = getlong(ttf);
    }

    for ( i=0; i<vs_cnt; ++i ) {
	if ( vs_data[i].def!=0 && justinuse==git_normal ) {
	    fseek(ttf,vs_map+vs_data[i].def,SEEK_SET);
	    rcnt = getlong(ttf);
	    for ( j=0; j<rcnt; ++j ) {
		int start_uni = get3byte(ttf);
		int cnt = getc(ttf);
		int uni;
		for ( uni=start_uni; uni<=start_uni+cnt; ++uni ) {
		    SplineChar *sc;
		    struct altuni *altuni;
		    for ( gid = 0; gid<info->glyph_cnt; ++gid ) {
			if ( (sc = info->chars[gid])!=NULL ) {
			    if ( sc->unicodeenc==uni )
		    break;
			    for ( altuni = sc->altuni; altuni!=NULL; altuni=altuni->next )
				if ( altuni->unienc==uni && altuni->vs == -1 && altuni->fid==0 )
			    break;
			    if ( altuni!=NULL )
		    break;
			}
		    }
		    if ( gid==info->glyph_cnt ) {
			LogError( _("No glyph with unicode U+%05x in font\n"),
				uni );
			info->bad_cmap = true;
		    } else {
			altuni = chunkalloc(sizeof(struct altuni));
			altuni->unienc = uni;
			altuni->vs = vs_data[i].vs;
			altuni->fid = 0;
			altuni->next = sc->altuni;
			sc->altuni = altuni;
		    }
		}
	    }
	}
	if ( vs_data[i].non_def!=0 ) {
	    fseek(ttf,vs_map+vs_data[i].non_def,SEEK_SET);
	    rcnt = getlong(ttf);
	    for ( j=0; j<rcnt; ++j ) {
		int uni = get3byte(ttf);
		int curgid = getushort(ttf);
		if ( justinuse==git_justinuse ) {
		    if ( curgid<info->glyph_cnt && curgid>=0)
			info->inuse[curgid] = 1;
		} else if ( justinuse==git_justinuse ) {
		    if ( curgid<info->glyph_cnt && curgid>=0 &&
			    (sc=info->chars[curgid])!=NULL && sc->name==NULL ) {
			char buffer[32];
			sprintf(buffer, "u%04X.vs%04X", uni, vs_data[i].vs );
			sc->name = copy(buffer);
		    }
		} else {
		    if ( curgid>=info->glyph_cnt || curgid<0 ||
			    info->chars[curgid]==NULL ) {
			LogError( _("GID out of range (%d) in format 14 'cmap' subtable\n"),
				cur_gid );
			info->bad_cmap = true;
		    } else {
			SplineChar *sc = info->chars[curgid];
			struct altuni *altuni = chunkalloc(sizeof(struct altuni));
			altuni->unienc = uni;
			altuni->vs = vs_data[i].vs;
			altuni->fid = 0;
			altuni->next = sc->altuni;
			sc->altuni = altuni;
		    }
		}
	    }
	}
    }
    free(vs_data);
}

static enum uni_interp amscheck(struct ttfinfo *info, EncMap *map) {
    int cnt = 0;
    /* Try to guess if the font uses the AMS math PUA assignments */

    if ( map==NULL )
return( ui_none );

    if ( 0xe668<map->enccount && map->map[0xe668]!=-1 &&
	    info->chars[map->map[0xe668]]->unicodeenc=='b' )
	++cnt;
    if ( 0xe3c8<map->enccount && map->map[0xe626]!=-1 &&
	    info->chars[map->map[0xe626]]->unicodeenc==0xe626 )
	++cnt;
    if ( 0xe3c8<map->enccount && map->map[0xe3c8]!=-1 &&
	    info->chars[map->map[0xe3c8]]->unicodeenc==0x29e1 )
	++cnt;
    if ( 0x2A7C<map->enccount && map->map[0x2A7C]!=-1 &&
	    info->chars[map->map[0x2A7C]]->unicodeenc==0xE32A )
	++cnt;
    if ( 0x2920<map->enccount && map->map[0x2920]!=-1 &&
	    info->chars[map->map[0x2920]]->unicodeenc==0xE221 )
	++cnt;
return( cnt>=2 ? ui_ams : ui_none );
}

static int PickCMap(struct cmap_encs *cmap_encs,int enccnt,int def) {
    char buffer[500];
    char **choices, *encname;
    int i, ret;
    static char *macscripts[]= { N_("Script|Roman"), N_("Script|Japanese"), N_("Script|Traditional Chinese"), N_("Script|Korean"),
	N_("Script|Arabic"), N_("Script|Hebrew"),  N_("Script|Greek"),
/* GT: Don't ask me what RSymbol means, I don't know either. It's in apple's */
/* GT:  docs though */
	N_("Script|Cyrillic"), N_("Script|RSymbol"), N_("Script|Devanagari"),
/* 10*/ N_("Script|Gurmukhi"), N_("Script|Gujarati"), NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,
/* 20*/	NULL, N_("Script|Thai"), NULL, NULL, NULL, N_("Script|Simplified Chinese"),
	NULL, NULL, NULL, N_("Script|Central European"),
/* 30*/ NULL, NULL, NULL };

    choices = malloc(enccnt*sizeof(char *));
    for ( i=0; i<enccnt; ++i ) {
	encname = NULL;
	if ( cmap_encs[i].platform==1 && cmap_encs[i].specific<32 ) {
	    encname = macscripts[cmap_encs[i].specific];
	    if ( encname!=NULL )
		encname = S_(encname);
	} else if ( cmap_encs[i].platform==0 ) {
	    switch ( cmap_encs[i].specific ) {
	      case 0:
		encname = N_("Unicode 1.0");
	      break;
	      case 1:
		encname = N_("Unicode 1.1");
	      break;
	      case 2:
		encname = N_("ISO 10646:1993");
	      break;
	      case 3:
		encname = N_("Unicode 2.0+, BMP only");
	      break;
	      case 4:
		encname = N_("Unicode 2.0+, all planes");
	      break;
	    }
	} else if ( cmap_encs[i].platform==3 && cmap_encs[i].specific==0 )
	    encname = N_("\"Symbol\"");
	if ( encname==NULL )
	    encname = cmap_encs[i].enc->enc_name;

	sprintf(buffer,"%d (%s) %d %s %s  %s",
		cmap_encs[i].platform,
		    cmap_encs[i].platform==0 ? _("Unicode") :
		    cmap_encs[i].platform==1 ? _("Apple") :
		    cmap_encs[i].platform==2 ? _("ISO (Deprecated)") :
		    cmap_encs[i].platform==3 ? _("MicroSoft") :
		    cmap_encs[i].platform==4 ? _("Custom") :
		    cmap_encs[i].platform==7 ? _("FreeType internals") :
					       _("Unknown"),
		cmap_encs[i].specific,
		encname,
		cmap_encs[i].platform==1 && cmap_encs[i].lang!=0? MacLanguageFromCode(cmap_encs[i].lang-1) : "",
		cmap_encs[i].format == 0 ? "Byte encoding table" :
		cmap_encs[i].format == 2 ? "High-byte mapping through table" :
		cmap_encs[i].format == 4 ? "Segment mapping to delta values" :
		cmap_encs[i].format == 6 ? "Trimmed table mapping" :
		cmap_encs[i].format == 8 ? "mixed 16-bit and 32-bit coverage" :
		cmap_encs[i].format == 10 ? "Trimmed array" :
		cmap_encs[i].format == 12 ? "Segmented coverage" :
		    "Unknown format" );
	choices[i] = copy(buffer);
    }
    ret = ff_choose(_("Pick a CMap subtable"),(const char **) choices,enccnt,def,
	    _("Pick a CMap subtable"));
    for ( i=0; i<enccnt; ++i )
	free(choices[i]);
    free(choices);
return( ret );
}

/* 'cmap' table: readttfcmap */
static void readttfencodings(FILE *ttf,struct ttfinfo *info, int justinuse) {
    int i,j, def, unicode_cmap, unicode4_cmap, dcnt, dcmap_cnt, dc;
    int nencs, version, usable_encs;
    Encoding *enc = &custom;
    const int32 *trans=NULL;
    enum uni_interp interp = ui_none;
    int platform, specific;
    int offset, encoff=0;
    int format, len;
    uint32 vs_map=0;
    uint16 table[256];
    int segCount;
    uint16 *endchars, *startchars, *delta, *rangeOffset, *glyphs;
    int index, last;
    int mod = 0;
    SplineChar *sc;
    uint8 *used;
    int badencwarned=false;
    int glyph_tot;
    Encoding *temp;
    EncMap *map;
    struct cmap_encs *cmap_encs, desired_cmaps[2], *dcmap;
    extern int ask_user_for_cmap;

    fseek(ttf,info->encoding_start,SEEK_SET);
    version = getushort(ttf);
    nencs = getushort(ttf);
    if ( version!=0 && nencs==0 )
	nencs = version;		/* Sometimes they are backwards */ /* Or was I just confused early on? */
    cmap_encs = malloc(nencs*sizeof(struct cmap_encs));
    for ( i=usable_encs=0; i<nencs; ++i ) {
	cmap_encs[usable_encs].platform =  getushort(ttf);
	cmap_encs[usable_encs].specific = getushort(ttf);
	cmap_encs[usable_encs].offset = getlong(ttf);
	if ( cmap_encs[usable_encs].platform == 0 && cmap_encs[usable_encs].specific == 5 ) {
	    /* This isn't a true encoding. */
	    /* It's an optional set of encoding modifications (sort of) */
	    /*  applied to a format 4/10 encoding (unicode BMP/Full) */
	    if ( SubtableMustBe14(ttf,info->encoding_start+cmap_encs[usable_encs].offset,info) )
		vs_map = info->encoding_start+cmap_encs[usable_encs].offset;
    continue;
	}
	temp = enc_from_platspec(cmap_encs[usable_encs].platform,cmap_encs[usable_encs].specific);
	if ( temp==NULL )	/* iconv doesn't support this. Some sun iconvs seem limited */
	    temp = FindOrMakeEncoding("Custom");
	cmap_encs[usable_encs].enc = temp;
	if ( SubtableIsntSupported(ttf,info->encoding_start+cmap_encs[usable_encs].offset,
		&cmap_encs[usable_encs],info))
    continue;
	++usable_encs;
    }
    if ( usable_encs==0 ) {
	LogError( _("Could not find any valid encoding tables" ));
	free(cmap_encs);
return;
    }
    def = -1;
    enc = &custom;
    unicode_cmap = unicode4_cmap = -1;
    for ( i=0; i<usable_encs; ++i ) {
	temp = cmap_encs[i].enc;
	platform = cmap_encs[i].platform;
	specific = cmap_encs[i].specific;
	offset = cmap_encs[i].offset;

	if ( (platform==3 && specific==10) || (platform==0 && specific==4) ) { /* MS Unicode 4 byte */
	    enc = temp;
	    def = i;
	    unicode4_cmap = i;
	} else if ( !enc->is_unicodefull && (!prefer_cjk_encodings ||
		(!enc->is_japanese && !enc->is_korean && !enc->is_tradchinese &&
		    !enc->is_simplechinese)) &&
		(( platform==3 && specific==1 ) || /* MS Unicode */
/* Well I should only deal with apple unicode specific==0 (default) and 3 (U2.0 semantics) */
/*  but apple ships dfonts with specific==1 (Unicode 1.1 semantics) */
/*  which is stupid of them */
		( platform==0 /*&& (specific==0 || specific==3)*/ ))) {	/* Apple Unicode */
	    enc = temp;
	    def = i;
	} else if ( platform==3 && specific==0 && enc->is_custom ) {
	    /* Only select symbol if we don't have something better */
	    enc = temp;
	    def = i;
	    /* Now I had assumed this would be a 1 byte encoding, but it turns*/
	    /*  out to map into the unicode private use area at U+f000-U+F0FF */
	    /*  so it's a 2 byte enc */
/* Mac platform specific encodings are script numbers. 0=>roman, 1=>jap, 2=>big5, 3=>korean, 4=>arab, 5=>hebrew, 6=>greek, 7=>cyrillic, ... 25=>simplified chinese */
	} else if ( platform==1 && specific==0 && enc->is_custom ) {
	    enc = temp;
	    def = i;
	} else if ( platform==1 && (specific==2 ||specific==1||specific==3||specific==25) &&
		!enc->is_unicodefull &&
		(prefer_cjk_encodings || !enc->is_unicodebmp) ) {
	    enc = temp;
	    def = i;
	} else if ( platform==3 && (specific>=2 && specific<=6 ) &&
		!enc->is_unicodefull &&
		(prefer_cjk_encodings || !enc->is_unicodebmp) ) {
	    /* Old ms docs say that specific==3 => big 5, new docs say specific==4 => big5 */
	    /*  Ain't that jus' great? */
	    enc = temp;
	    def = i;
	}
	if ( (platform==3 && specific==1) ||
		(platform==0 && specific==3))
	    unicode_cmap = i;
    }

    if ( justinuse==git_justinuse || !ask_user_for_cmap || (i = PickCMap(cmap_encs,usable_encs,def))==-1 )
	i = def;

    if ( i==-1 ) {
	if ( justinuse==git_normal )
	    LogError( _("Could not find a usable encoding table" ));
	free(cmap_encs);
return;
    }

    info->platform = cmap_encs[i].platform;
    info->specific = cmap_encs[i].specific;

    desired_cmaps[0] = cmap_encs[i]; dcnt = 1;
    if ( unicode4_cmap!=-1 ) {
	if ( i!=unicode4_cmap ) {
	    desired_cmaps[1] = cmap_encs[unicode4_cmap];
	    ++dcnt;
	}
    } else if ( unicode_cmap!=-1 ) {
	if ( i!=unicode_cmap ) {
	    desired_cmaps[1] = cmap_encs[unicode_cmap];
	    ++dcnt;
	}
    } else {
	if ( i!=def && def!=-1 ) {
	    desired_cmaps[1] = cmap_encs[def];
	    ++dcnt;
	}
    }

    map = NULL;
    if ( justinuse==git_justinuse ) {
	dcmap_cnt = usable_encs;
	dcmap = cmap_encs;
    } else {
	dcmap_cnt = dcnt;
	dcmap = desired_cmaps;
    }
    for ( dc=dcmap_cnt-1; dc>=0; --dc ) {
	/* if justinuse then look at all cmaps and tick the glyphs they use */
	/* otherwise dcmap_cnt will be either 1 or 2. If 1 then this subtable */
	/* contains both the encoding and the source for unicode encodings */
	/* if dcmap_cnt==2 then when dc==0 we are setting up the encoding */
	/*  and when dc==1 we are setting up the unicode code points */
	int dounicode = (dc==dcmap_cnt-1);
	enc = dcmap[dc].enc;
	encoff = dcmap[dc].offset;

	mod = 0;
	if ( dcmap[dc].platform==3 && (dcmap[dc].specific>=2 && dcmap[dc].specific<=6 ))
	    mod = dcmap[dc].specific;
	else if ( dcmap[dc].platform==1 && (dcmap[dc].specific==2 ||dcmap[dc].specific==1||dcmap[dc].specific==3||dcmap[dc].specific==25))
	    mod = dcmap[dc].specific==1?2:dcmap[dc].specific==2?4:dcmap[dc].specific==3?5:3;		/* convert to ms specific */
	if ( dc==0 && justinuse==git_normal ) {
	    interp = interp_from_encoding(enc,ui_none);
	    info->map = map = EncMapNew(enc->char_cnt,info->glyph_cnt,enc);
	    info->uni_interp = interp;
	}

	fseek(ttf,info->encoding_start+encoff,SEEK_SET);
	format = getushort(ttf);
	if ( format!=12 && format!=10 && format!=8 ) {
	    len = getushort(ttf);
	    /* version/language = */ getushort(ttf);
	} else {
	    /* padding */ getushort(ttf);
	    len = getlong(ttf);
	    /* language = */ getlong(ttf);
	}
	if ( enc->is_unicodebmp && (format==8 || format==10 || format==12))
	    enc = FindOrMakeEncoding("UnicodeFull");

	if ( format==0 ) {
	    if ( justinuse==git_normal && map!=NULL && map->enccount<256 ) {
		map->map = realloc(map->map,256*sizeof(int));
		memset(map->map,-1,(256-map->enccount)*sizeof(int));
		map->enccount = map->encmax = 256;
	    }
	    for ( i=0; i<len-6; ++i )
		table[i] = getc(ttf);
	    trans = enc->unicode;
	    if ( trans==NULL && dcmap[dc].platform==1 )
		trans = MacEncToUnicode(dcmap[dc].specific,dcmap[dc].lang-1);
	    for ( i=0; i<256 && i<len-6; ++i )
		if ( justinuse==git_normal ) {
		    if ( table[i]<info->glyph_cnt && info->chars[table[i]]!=NULL ) {
			if ( map!=NULL )
			    map->map[i] = table[i];
			if ( dounicode && trans!=NULL )
			    info->chars[table[i]]->unicodeenc = trans[i];
		    }
		} else if ( table[i]<info->glyph_cnt && info->chars[table[i]]!=NULL )
		    info->inuse[table[i]] = 1;
	} else if ( format==4 ) {
	    segCount = getushort(ttf)/2;
	    /* searchRange = */ getushort(ttf);
	    /* entrySelector = */ getushort(ttf);
	    /* rangeShift = */ getushort(ttf);
	    endchars = malloc(segCount*sizeof(uint16));
	    used = calloc(65536,sizeof(uint8));
	    for ( i=0; i<segCount; ++i )
		endchars[i] = getushort(ttf);
	    if ( getushort(ttf)!=0 )
		IError("Expected 0 in 'cmap' format 4 subtable");
	    startchars = malloc(segCount*sizeof(uint16));
	    for ( i=0; i<segCount; ++i )
		startchars[i] = getushort(ttf);
	    delta = malloc(segCount*sizeof(uint16));
	    for ( i=0; i<segCount; ++i )
		delta[i] = getushort(ttf);
	    rangeOffset = malloc(segCount*sizeof(uint16));
	    for ( i=0; i<segCount; ++i )
		rangeOffset[i] = getushort(ttf);
	    len -= 8*sizeof(uint16) +
		    4*segCount*sizeof(uint16);
	    /* that's the amount of space left in the subtable and it must */
	    /*  be filled with glyphIDs */
	    if ( len<0 ) {
		IError("This font has an illegal format 4 subtable with too little space for all the segments.\nThis error is not recoverable.\nBye" );
		exit(1);
	    }
	    glyphs = malloc(len);
	    glyph_tot = len/2;
	    for ( i=0; i<glyph_tot; ++i )
		glyphs[i] = getushort(ttf);
	    for ( i=0; i<segCount; ++i ) {
		if ( rangeOffset[i]==0 && startchars[i]==0xffff )
		    /* Done */;
		else if ( rangeOffset[i]==0 ) {
		    for ( j=startchars[i]; j<=endchars[i]; ++j ) {
			if ( justinuse==git_justinuse && (uint16) (j+delta[i])<info->glyph_cnt )
			    info->inuse[(uint16) (j+delta[i])] = true;
			else if ( (uint16) (j+delta[i])>=info->glyph_cnt || info->chars[(uint16) (j+delta[i])]==NULL ) {
			    LogError( _("Attempt to encode missing glyph %d to %d (0x%x)\n"),
				    (uint16) (j+delta[i]), modenc(j,mod), modenc(j,mod));
			    info->bad_cmap = true;
			} else {
			    int uenc = umodenc(j,mod,info);
			    int lenc = modenc(j,mod);
			    if ( uenc!=-1 && used[uenc] ) {
				if ( !badencwarned ) {
				    LogError( _("Multiple glyphs map to the same unicode encoding U+%04X, only one will be used\n"), uenc );
			            info->bad_cmap = true;
			            badencwarned = true;
				}
			    } else {
				if ( uenc!=-1 && dounicode ) used[uenc] = true;
				if ( dounicode && info->chars[(uint16) (j+delta[i])]->unicodeenc==-1 )
				    info->chars[(uint16) (j+delta[i])]->unicodeenc = uenc;
			        if ( map!=NULL && lenc<map->enccount )
				    map->map[lenc] = (uint16) (j+delta[i]);
			    }
			}
		    }
		} else if ( rangeOffset[i]!=0xffff ) {
		    /* Apple says a rangeOffset of 0xffff means no glyph */
		    /*  OpenType doesn't mention this */
		    for ( j=startchars[i]; j<=endchars[i]; ++j ) {
			int temp = (i-segCount+rangeOffset[i]/2) + j-startchars[i];
			if ( temp<glyph_tot )
			    index = glyphs[ temp ];
			else {
			    /* This happened in mingliu.ttc(PMingLiU) */
			    if ( justinuse==git_normal ) {
				LogError( _("Glyph index out of bounds. Was %d, must be less than %d.\n In attempt to associate a glyph with encoding %x in segment %d\n with platform=%d, specific=%d (in 'cmap')\n"),
					temp, glyph_tot, j, i, dcmap[dc].platform, dcmap[dc].specific );
				info->bad_cmap = true;
			    }
			    index = 0;
			}
			if ( index!=0 ) {
			    index = (unsigned short) (index+delta[i]);
			    if ( index>=info->glyph_cnt ) {
				/* This isn't mentioned either, but in some */
			        /*  MS Chinese fonts (kaiu.ttf) the index */
			        /*  goes out of bounds. and MS's ttf dump */
			        /*  program says it is treated as 0 */
				LogError( _("Attempt to encode missing glyph %d to %d (0x%x)\n"),
					index, modenc(j,mod), modenc(j,mod));
				info->bad_cmap = true;
			    } else if ( justinuse==git_justinuse )
				info->inuse[index] = 1;
			    else if ( info->chars[index]==NULL ) {
				LogError( _("Attempt to encode missing glyph %d to %d (0x%x)\n"),
					index, modenc(j,mod), modenc(j,mod));
				info->bad_cmap = true;
			    } else {
				int uenc = umodenc(j,mod,info);
				int lenc = modenc(j,mod);
				if ( uenc!=-1 && used[uenc] ) {
				    if ( !badencwarned ) {
					LogError( _("Multiple glyphs map to the same unicode encoding U+%04X, only one will be used\n"), uenc );
			                info->bad_cmap = true;
					badencwarned = true;
				    }
				} else {
				    if ( uenc!=-1 && dounicode ) used[uenc] = true;
				    if ( dounicode && info->chars[index]->unicodeenc==-1 )
					info->chars[index]->unicodeenc = uenc;
				    if ( map!=NULL && lenc<map->enccount )
					map->map[lenc] = index;
				}
			    }
			}
		    }
		} else {
		    LogError( _("Use of a range offset of 0xffff to mean a missing glyph in cmap table\n") );
		    info->bad_cmap = true;
		}
	    }
	    free(glyphs);
	    free(rangeOffset);
	    free(delta);
	    free(startchars);
	    free(endchars);
	    free(used);
	} else if ( format==6 ) {
	    /* trimmed array format */
	    /* Well, the docs say it's for 2byte encodings, but Apple actually*/
	    /*  uses it for 1 byte encodings which don't fit into the require-*/
	    /*  ments for a format 0 sub-table. See Zapfino.dfont */
	    int first, count;
	    first = getushort(ttf);
	    count = getushort(ttf);
	    trans = enc->unicode;
	    if ( trans==NULL && dcmap[dc].platform==1 && first+count<=256 )
		trans = MacEncToUnicode(dcmap[dc].specific,dcmap[dc].lang-1);
	    if ( justinuse==git_justinuse )
		for ( i=0; i<count; ++i )
		    info->inuse[getushort(ttf)]= 1;
	    else {
		for ( i=0; i<count; ++i ) {
		    int gid = getushort(ttf);
		    if ( dounicode )
			info->chars[gid]->unicodeenc = trans!=NULL ? trans[first+1] : first+i;
		    if ( map!=NULL && first+i < map->enccount )
			map->map[first+i] = gid;
		}
	    }
	} else if ( format==2 ) {
	    int max_sub_head_key = 0, cnt, max_pos= -1;
	    struct subhead *subheads;
	    
	    for ( i=0; i<256; ++i ) {
		table[i] = getushort(ttf)/8;	/* Sub-header keys */
		if ( table[i]>max_sub_head_key ) {
		    max_sub_head_key = table[i];	/* The entry is a byte pointer, I want a pointer in units of struct subheader */
		    max_pos = i;
		}
	    }
	    subheads = malloc((max_sub_head_key+1)*sizeof(struct subhead));
	    for ( i=0; i<=max_sub_head_key; ++i ) {
		subheads[i].first = getushort(ttf);
		subheads[i].cnt = getushort(ttf);
		subheads[i].delta = getushort(ttf);
		subheads[i].rangeoff = (getushort(ttf)-
				(max_sub_head_key-i)*sizeof(struct subhead)-
				sizeof(short))/sizeof(short);
	    }
	    cnt = (len-(ftell(ttf)-(info->encoding_start+encoff)))/sizeof(short);
	    /* The count is the number of glyph indexes to read. it is the */
	    /*  length of the entire subtable minus that bit we've read so far */
	    glyphs = malloc(cnt*sizeof(short));
	    for ( i=0; i<cnt; ++i )
		glyphs[i] = getushort(ttf);
	    last = -1;
	    for ( i=0; i<256; ++i ) {
		if ( table[i]==0 ) {
		    /* Special case, single byte encoding entry, look it up in */
		    /*  subhead */
		    /* In the one example I've got of this encoding (wcl-02.ttf) the chars */
		    /* 0xfd, 0xfe, 0xff are said to exist but there is no mapping */
		    /* for them. */
		    if ( i>=max_pos )
			index = 0;	/* the subhead says there are 256 entries, but in fact there are only 193, so attempting to find these guys should give an error */
		    else if ( i<subheads[0].first || i>=subheads[0].first+subheads[0].cnt ||
			    subheads[0].rangeoff+(i-subheads[0].first)>=cnt )
			index = 0;
		    else if ( (index = glyphs[subheads[0].rangeoff+(i-subheads[0].first)])!= 0 )
			index = (uint32) (index+subheads[0].delta);
		    /* I assume the single byte codes are just ascii or latin1*/
		    if ( index!=0 && index<info->glyph_cnt ) {
			if ( justinuse==git_justinuse )
			    info->inuse[index] = 1;
			else if ( info->chars[index]==NULL )
			    /* Do Nothing */;
			else {
			    int lenc = modenc(i,mod);
			    if ( dounicode && info->chars[index]->unicodeenc==-1 )
				info->chars[index]->unicodeenc = i;
			    if ( map!=NULL && lenc<map->enccount )
				map->map[lenc] = index;
			}
		    }
		} else {
		    int k = table[i];
		    for ( j=0; j<subheads[k].cnt; ++j ) {
			int enc, lenc;
			if ( subheads[k].rangeoff+j>=cnt )
			    index = 0;
			else if ( (index = glyphs[subheads[k].rangeoff+j])!= 0 )
			    index = (uint16) (index+subheads[k].delta);
			if ( index!=0 && index<info->glyph_cnt ) {
			    enc = (i<<8)|(j+subheads[k].first);
			    lenc = modenc(enc,mod);
			    if ( justinuse==git_justinuse )
				info->inuse[index] = 1;
			    else if ( info->chars[index]==NULL )
				/* Do Nothing */;
			    else {
				if ( dounicode && info->chars[index]->unicodeenc==-1 )
				    info->chars[index]->unicodeenc = umodenc(enc,mod,info);
				if ( map!=NULL && lenc<map->enccount )
				    map->map[lenc] = index;
			    }
			}
		    }
		    /*if ( last==-1 ) last = i;*/
		}
	    }
	    free(subheads);
	    free(glyphs);
	} else if ( format==8 ) {
	    uint32 ngroups, start, end, startglyph;
	    if ( !enc->is_unicodefull ) {
		IError("I don't support 32 bit characters except for the UCS-4 (MS platform, specific=10)" );
		enc = FindOrMakeEncoding("UnicodeFull");
	    }
	    /* I'm now assuming unicode surrogate encoding, so I just ignore */
	    /*  the is32 table (it will be set for the surrogates and not for */
	    /*  anything else */
	    fseek(ttf,8192,SEEK_CUR);
	    ngroups = getlong(ttf);
	    for ( j=0; j<ngroups; ++j ) {
		start = getlong(ttf);
		end = getlong(ttf);
		startglyph = getlong(ttf);
		if ( justinuse==git_justinuse )
		    for ( i=start; i<=end; ++i )
			info->inuse[startglyph+i-start]= 1;
		else
		    for ( i=start; i<=end; ++i ) {
			int uenc = ((i>>16)-0xd800)*0x400 + (i&0xffff)-0xdc00 + 0x10000;
			sc = info->chars[startglyph+i-start];
			if ( dounicode && sc->unicodeenc==-1 )
			    sc->unicodeenc = uenc;
			if ( map!=NULL && sc->unicodeenc < map->enccount )
			    map->map[uenc] = startglyph+i-start;
		    }
	    }
	} else if ( format==10 ) {
	    /* same as format 6, except for 4byte chars */
	    int first, count;
	    if ( !enc->is_unicodefull ) {
		IError("I don't support 32 bit characters except for the UCS-4 (MS platform, specific=10)" );
		enc = FindOrMakeEncoding("UnicodeFull");
	    }
	    first = getlong(ttf);
	    count = getlong(ttf);
	    if ( justinuse==git_justinuse )
		for ( i=0; i<count; ++i )
		    info->inuse[getushort(ttf)]= 1;
	    else
		for ( i=0; i<count; ++i ) {
		    int gid = getushort(ttf);
		    if ( dounicode )
			info->chars[gid]->unicodeenc = first+i;
		    if ( map!=NULL && first+i < map->enccount )
			map->map[first+i] = gid;
		}
	} else if ( format==12 ) {
	    uint32 ngroups, start, end, startglyph;
	    if ( !enc->is_unicodefull ) {
		IError("I don't support 32 bit characters except for the UCS-4 (MS platform, specific=10)" );
		enc = FindOrMakeEncoding("UnicodeFull");
	    }
	    ngroups = getlong(ttf);
	    for ( j=0; j<ngroups; ++j ) {
		start = getlong(ttf);
		end = getlong(ttf);
		startglyph = getlong(ttf);
		if ( justinuse==git_justinuse ) {
		    for ( i=start; i<=end; ++i )
			if ( startglyph+i-start < info->glyph_cnt )
			    info->inuse[startglyph+i-start]= 1;
			else
		    break;
		} else
		    for ( i=start; i<=end; ++i ) {
			if ( startglyph+i-start >= info->glyph_cnt ||
				info->chars[startglyph+i-start]==NULL ) {
			    LogError( _("Bad font: Encoding data out of range.\n") );
			    info->bad_cmap = true;
		    break;
			} else {
			    if ( dounicode )
				info->chars[startglyph+i-start]->unicodeenc = i;
			    if ( map!=NULL && i < map->enccount )
				map->map[i] = startglyph+i-start;
			}
		    }
	    }
	}
    }
    free(cmap_encs);
    if ( info->chars!=NULL )
	for ( i=0; i<info->glyph_cnt; ++i )
	    if ( info->chars[i]!=NULL && info->chars[i]->unicodeenc==0xffff )
		info->chars[i]->unicodeenc = -1;
    info->vs_start = vs_map;
    if ( vs_map!=0 )
	ApplyVariationSequenceSubtable(ttf,vs_map,info,justinuse);
    if ( justinuse==git_normal ) {
	if ( interp==ui_none )
	    info->uni_interp = amscheck(info,map);
	map->enc = enc;		/* This can be changed from the initial value */
    }
    info->map = map;
}

static void readttfos2metrics(FILE *ttf,struct ttfinfo *info) {
    int i, sel;

    fseek(ttf,info->os2_start,SEEK_SET);
    info->os2_version = getushort(ttf);
    /* avgWidth */ getushort(ttf);
    info->pfminfo.weight = getushort(ttf);
    info->pfminfo.width = getushort(ttf);
    info->pfminfo.fstype = getushort(ttf);
    info->pfminfo.os2_subxsize = getushort(ttf);
    info->pfminfo.os2_subysize = getushort(ttf);
    info->pfminfo.os2_subxoff = getushort(ttf);
    info->pfminfo.os2_subyoff = getushort(ttf);
    info->pfminfo.os2_supxsize = getushort(ttf);
    info->pfminfo.os2_supysize = getushort(ttf);
    info->pfminfo.os2_supxoff = getushort(ttf);
    info->pfminfo.os2_supyoff = getushort(ttf);
    info->pfminfo.os2_strikeysize = getushort(ttf);
    info->pfminfo.os2_strikeypos = getushort(ttf);
    info->pfminfo.os2_family_class = getushort(ttf);
    for ( i=0; i<10; ++i )
	info->pfminfo.panose[i] = getc(ttf);
    info->pfminfo.pfmfamily = info->pfminfo.panose[0]==2 ? 0x11 :	/* might be 0x21 */ /* Text & Display maps to either serif 0x11 or sans 0x21 or monospace 0x31 */
		      info->pfminfo.panose[0]==3 ? 0x41 :	/* Script */
		      info->pfminfo.panose[0]==4 ? 0x51 :	/* Decorative */
		      0x51;					/* And pictorial doesn't fit into pfm */
    info->pfminfo.unicoderanges[0] = getlong(ttf);
    info->pfminfo.unicoderanges[1] = getlong(ttf);
    info->pfminfo.unicoderanges[2] = getlong(ttf);
    info->pfminfo.unicoderanges[3] = getlong(ttf);
    info->pfminfo.hasunicoderanges = true;
    info->pfminfo.os2_vendor[0] = getc(ttf);
    info->pfminfo.os2_vendor[1] = getc(ttf);
    info->pfminfo.os2_vendor[2] = getc(ttf);
    info->pfminfo.os2_vendor[3] = getc(ttf);
    sel = getushort(ttf);
    if ( info->os2_version>=4 ) {
	info->use_typo_metrics = (sel&128)?1:0;
	info->weight_width_slope_only = (sel&256)?1:0;
    }
    /* Clear the bits we don't support in stylemap and set it. */
    info->pfminfo.stylemap = sel & 0x61;
    /* firstchar */ getushort(ttf);
    /* lastchar */ getushort(ttf);
    info->pfminfo.os2_typoascent = getushort(ttf);
    info->pfminfo.os2_typodescent = (short) getushort(ttf);
    if ( info->pfminfo.os2_typoascent-info->pfminfo.os2_typodescent == info->emsize ) {
	info->ascent = info->pfminfo.os2_typoascent;
	info->descent = -info->pfminfo.os2_typodescent;
    }
    info->pfminfo.os2_typolinegap = getushort(ttf);
    info->pfminfo.os2_winascent = getushort(ttf);
    info->pfminfo.os2_windescent = getushort(ttf);
    info->pfminfo.winascent_add = info->pfminfo.windescent_add = false;
    info->pfminfo.typoascent_add = info->pfminfo.typodescent_add = false;
    info->pfminfo.pfmset = true;
    info->pfminfo.panose_set = true;
    info->pfminfo.subsuper_set = true;
    if ( info->os2_version>=1 ) {
	info->pfminfo.codepages[0] = getlong(ttf);
	info->pfminfo.codepages[1] = getlong(ttf);
	info->pfminfo.hascodepages = true;
	if ( info->os2_version>=2 ) {
	info->pfminfo.os2_xheight = (short) getushort(ttf);
	info->pfminfo.os2_capheight = (short) getushort(ttf);
	}
    }

    if ( info->os2_version==0 ) {
	LogError(_("Windows will reject fonts with an OS/2 version number of 0\n"));
	info->bad_os2_version = true;
    } else if ( info->os2_version==1 && info->cff_start!=0 ) {
	LogError(_("Windows will reject otf (cff) fonts with an OS/2 version number of 1\n"));
	info->bad_os2_version = true;
    }
}

static void readttfpostnames(FILE *ttf,struct ttfinfo *info) {
    int i,j;
    int format, len, gc, gcbig, val;
    const char *name;
    char buffer[30];
    uint16 *indexes;
    extern const char *ttfstandardnames[];
    int notdefwarned = false;
    int anynames = false;

    ff_progress_change_line2(_("Reading Names"));

    /* Give ourselves an xuid, just in case they want to convert to PostScript*/
    /*  (even type42)							      */
    if ( xuid!=NULL && info->fd==NULL && info->xuid==NULL ) {
	info->xuid = malloc(strlen(xuid)+20);
	sprintf(info->xuid,"[%s %d]", xuid, (rand()&0xffffff));
    }

    if ( info->postscript_start!=0 ) {
	fseek(ttf,info->postscript_start,SEEK_SET);
	format = getlong(ttf);
	info->italicAngle = getfixed(ttf);
    /*
     * Due to the legacy of two formats, there are two underlinePosition
     * attributes in an OpenType CFF font, one being stored in the CFF table.
     * FontForge due to its pfa heritage will only keep the PostScript/CFF
     * underlinePosition in the SplineFont so we'll calculate that here if we
     * are indeed working on a TTF.
     * If we have a CFF font, cffinfofillup() has already read the appropriate
     * data and so we don't rewind it (if info->uwidth is odd we are possibly
     * introducing a rounding error).
     */
	if (info->cff_start==0) {
	info->upos = (short) getushort(ttf);
	info->uwidth = (short) getushort(ttf);
	info->upos -= info->uwidth/2;		/* 'post' defn of this field is different from FontInfo defn and I didn't notice */
	}
	info->isFixedPitch = getlong(ttf);
	/* mem1 = */ getlong(ttf);
	/* mem2 = */ getlong(ttf);
	/* mem3 = */ getlong(ttf);
	/* mem4 = */ getlong(ttf);
	if ( format==0x00020000 ) {
	    gc = getushort(ttf);
	    indexes = calloc(65536,sizeof(uint16));
	    /* the index table is backwards from the way I want to use it */
	    gcbig = 0;
	    for ( i=0; i<gc; ++i ) {
		val = getushort(ttf);
		if ( val<0 )		/* Don't crash on EOF */
	    break;
		indexes[val] = i;
		if ( val>=258 ) ++gcbig;
	    }

	    /* if we are only loading bitmaps, we can get holes in our data */
	    for ( i=0; i<258; ++i ) if ( indexes[i]!=0 || i==0 ) if ( indexes[i]<info->glyph_cnt && info->chars[indexes[i]]!=NULL )
		info->chars[indexes[i]]->name = copy(ttfstandardnames[i]); /* Too many fonts have badly named glyphs to deduce encoding from name */
	    gcbig += 258;
	    for ( i=258; i<gcbig; ++i ) {
		char *nm;
		len = getc(ttf);
		if ( len<0 )		/* Don't crash on EOF */
	    break;
		nm = malloc(len+1);
		for ( j=0; j<len; ++j )
		    nm[j] = getc(ttf);
		nm[j] = '\0';
		if ( indexes[i]<info->glyph_cnt && info->chars[indexes[i]]!=NULL )
		    info->chars[indexes[i]]->name = nm; /* Too many fonts have badly named glyphs to deduce encoding from name */
	    }
	    free(indexes);
	    anynames = true;
	}
    }

    if ( info->fd!=NULL && info->fd->chars!=NULL) {
	EncMap *map = NULL;
	struct pschars *chars = info->fd->chars;
	if ( info->map==NULL )
	    info->map = map = EncMapNew(65536,65536,FindOrMakeEncoding("UnicodeBmp"));
	/* In type42 fonts the names are stored in a postscript /CharStrings dictionary */
	for ( i=0; i<chars->next; ++i ) {
	    int gid = (intpt) (chars->values[i]);
	    if ( gid>=0 && gid<info->glyph_cnt && chars->keys[i]!=NULL ) {
		free(info->chars[gid]->name);
		info->chars[gid]->name = chars->keys[i];
		info->chars[gid]->unicodeenc = UniFromName(chars->keys[i],info->uni_interp,info->map->enc);
		if ( map!=NULL && info->chars[gid]->unicodeenc!=-1 &&
			info->chars[gid]->unicodeenc<map->enccount)
		    map->map[ info->chars[gid]->unicodeenc ] = gid;
		chars->keys[i] = NULL;
		chars->values[i] = NULL;
	    } else
		chars->values[i] = NULL;
	}
    }

    for ( i=0; i<info->glyph_cnt; ++i ) if ( info->chars[i]!=NULL ) {
	/* info->chars[i] can be null in some TTC files */
	if ( i!=0 && info->chars[i]->name!=NULL &&
		strcmp(info->chars[i]->name,".notdef")==0 ) {
	    /* for some reason MS puts out fonts where several characters */
	    /* are called .notdef (and only one is a real notdef). So if we */
	    /* find a glyph other than 0 called ".notdef" then pretend it had */
	    /* no name */
	    if ( !notdefwarned ) {
		notdefwarned = true;
		LogError( _("Glyph %d is called \".notdef\", a singularly inept choice of name (only glyph 0\n may be called .notdef)\nFontForge will rename it.\n"), i );
	    }
	    free(info->chars[i]->name);
	    info->chars[i]->name = NULL;
	/* I used to check for glyphs with bad names (ie. names indicative of */
	/*  another unicode code point than the one applied to the glyph) but */
	/*  this proves too early for that check, as we don't have the altunis*/
	/*  figured out yet. So I've moved that into its own routine later */
	}
	/* And some volt files actually assign nul strings to the name */
	if ( (info->chars[i]->name!=NULL && *info->chars[i]->name!='\0' ))
    continue;
	free(info->chars[i]->name);	/* If it's a null string get rid of it */
	if ( i==0 )
	    name = ".notdef";
	else if ( info->chars[i]->unicodeenc==-1 ) {
	    /* Do this later */;
	    name = NULL;
	} else {
	    name = StdGlyphName(buffer,info->chars[i]->unicodeenc,info->uni_interp,NULL);
	    if ( anynames ) {
		for ( j=0; j<info->glyph_cnt; ++j ) {
		    if ( info->chars[j]!=NULL && j!=i && info->chars[j]->name!=NULL ) {
			if ( strcmp(info->chars[j]->name,name)==0 ) {
			    name = NULL;
		break;
			}
		    }
		}
	    }
	}
	ff_progress_next();
	info->chars[i]->name = copy(name);
    }

    /* If we have a GSUB table we can give some unencoded glyphs names */
    /*  for example if we have a vrt2 substitution of A to <unencoded> */
    /*  we could name the unencoded "A.vrt2" (though in this case we might */
    /*  try A.vert instead */ /* Werner suggested this */
    /* We could try this from morx too, except that apple features don't */
    /*  use meaningful ids. That is A.15,3 isn't very readable */
    for ( i=info->glyph_cnt-1; i>=0 ; --i )
	if ( info->chars[i]!=NULL && info->chars[i]->name==NULL )
    break;
    if ( i>=0 && info->vs_start!=0 )
	ApplyVariationSequenceSubtable(ttf,info->vs_start,info,git_findnames);
    if ( i>=0 && info->gsub_start!=0 )
	GuessNamesFromGSUB(ttf,info);
    if ( i>=0 && info->math_start!=0 )
	GuessNamesFromMATH(ttf,info);

    for ( i=0; i<info->glyph_cnt; ++i ) {
	/* info->chars[i] can be null in some TTC files */
	if ( info->chars[i]==NULL )
    continue;
	if ( info->chars[i]->name!=NULL )
    continue;
	if ( info->ordering!=NULL )
	    sprintf(buffer, "%.20s-%d", info->ordering, i );
	else if ( info->map!=NULL && info->map->backmap[i]!=-1 )
	    sprintf(buffer, "nounicode.%d.%d.%x", info->platform, info->specific,
		    (int) info->map->backmap[i] );
	else
	    sprintf( buffer, "glyph%d", i );
	info->chars[i]->name = copy(buffer);
	ff_progress_next();
    }
    ff_progress_next_stage();
}

static void readttfgasp(FILE *ttf,struct ttfinfo *info) {
    int i, cnt;

    if ( info->gasp_start==0 )
return;

    fseek(ttf,info->gasp_start,SEEK_SET);
    info->gasp_version = getushort(ttf);
    if ( info->gasp_version!=0 && info->gasp_version!=1 )
return;			/* We only support 'gasp' versions 0&1 (no other versions currently) */
    info->gasp_cnt = cnt = getushort(ttf);
    if ( cnt==0 )
return;
    info->gasp = malloc(cnt*sizeof(struct gasp));
    for ( i=0; i<cnt; ++i ) {
	info->gasp[i].ppem = getushort(ttf);
	info->gasp[i].flags = getushort(ttf);
    }
}

static void UnfigureControls(Spline *spline,BasePoint *pos) {
    pos->x = rint( (spline->splines[0].c+2*spline->splines[0].d)/2 );
    pos->y = rint( (spline->splines[1].c+2*spline->splines[1].d)/2 );
}

int ttfFindPointInSC(SplineChar *sc,int layer,int pnum,BasePoint *pos,
	RefChar *bound) {
    SplineSet *ss;
    SplinePoint *sp;
    int last=0, ret;
    RefChar *refs;

    for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->ttfindex==pnum ) {
		*pos = sp->me;
return(-1);
	    } else if ( sp->nextcpindex==pnum ) {
		if ( sp->next!=NULL && sp->next->order2 )
		    *pos = sp->nextcp;
		else {
		    /* fix this up to be 2 degree bezier control point */
		    UnfigureControls(sp->next,pos);
		}
return( -1 );
	    }
	    if ( !sp->nonextcp && last<=sp->nextcpindex )
		last = sp->nextcpindex+1;
	    else if ( sp->ttfindex!=0xffff )
		last = sp->ttfindex+1;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    for ( refs=sc->layers[layer].refs; refs!=NULL; refs=refs->next ) {
	if ( refs==bound ) {
	    LogError( _("Invalid point match. Point would be after this reference.\n") );
return( 0x800000 );
	}
	ret = ttfFindPointInSC(refs->sc,ly_fore,pnum-last,pos,NULL);
	if ( ret==-1 ) {
	    BasePoint p;
	    p.x = refs->transform[0]*pos->x + refs->transform[2]*pos->y + refs->transform[4];
	    p.y = refs->transform[1]*pos->x + refs->transform[3]*pos->y + refs->transform[5];
	    *pos = p;
return( -1 );
	}
	last += ret;
	if ( last>pnum ) {
	    IError("Point match failure last=%d, pnum=%d", last, pnum );
return( 0x800000 );
	}
    }
return( last );		/* Count of number of points in the character */
}

static void ttfPointMatch(SplineChar *sc,RefChar *rf) {
    BasePoint sofar, inref;

    if ( ttfFindPointInSC(sc,ly_fore,rf->match_pt_base,&sofar,rf)!=-1 ||
	    ttfFindPointInSC(rf->sc,ly_fore,rf->match_pt_ref,&inref,NULL)!=-1 ) {
	LogError( _("Could not match points in composite glyph (%d to %d) when adding %s to %s\n"),
		rf->match_pt_base, rf->match_pt_ref, rf->sc->name, sc->name);
return;
    }
    rf->transform[4] = sofar.x-inref.x;
    rf->transform[5] = sofar.y-inref.y;
}

int ttfFixupRef(SplineChar **chars,int i) {
    RefChar *ref, *prev, *next;

    if ( chars[i]==NULL )		/* Can happen in ttc files */
return( false );
    if ( chars[i]->ticked )
return( false );
    chars[i]->ticked = true;
    prev = NULL;
    for ( ref=chars[i]->layers[ly_fore].refs; ref!=NULL; ref=next ) {
	if ( ref->sc!=NULL )
    break;				/* Already done */
	next = ref->next;
	if ( !ttfFixupRef(chars,ref->orig_pos)) {
	    if ( prev==NULL )
		chars[i]->layers[ly_fore].refs = next;
	    else
		prev->next = next;
	    chunkfree(ref,sizeof(RefChar));
	} else {
	    ref->sc = chars[ref->orig_pos];
	    ref->adobe_enc = getAdobeEnc(ref->sc->name);
	    if ( ref->point_match )
		ttfPointMatch(chars[i],ref);
	    SCReinstanciateRefChar(chars[i],ref,ly_fore);
	    SCMakeDependent(chars[i],ref->sc);
	    prev = ref;
	}
    }
    chars[i]->ticked = false;
return( true );
}

static void ttfFixupReferences(struct ttfinfo *info) {
    int i;

    ff_progress_change_line2(_("Fixing up References"));
    for ( i=0; i<info->glyph_cnt; ++i ) if ( info->chars[i]!=NULL )
	info->chars[i]->ticked = false;
    for ( i=0; i<info->glyph_cnt; ++i ) {
	ttfFixupRef(info->chars,i);
	ff_progress_next();
    }
    ff_progress_next_stage();
}

static void TtfCopyTableBlindly(struct ttfinfo *info,FILE *ttf,
	uint32 start,uint32 len,uint32 tag) {
    struct ttf_table *tab;

    if ( start==0 || len==0 )
return;
    if ( len>0x1000000 ) {
	LogError( _("Unlikely length for table, so I'm ignoring it. %u\n"), len );
return;
    }

    tab = chunkalloc(sizeof(struct ttf_table));
    tab->tag = tag;
    tab->len = len;
    tab->data = malloc(len);
    fseek(ttf,start,SEEK_SET);
    fread(tab->data,1,len,ttf);
    tab->next = info->tabs;
    info->tabs = tab;
}

static int LookupListHasFeature(OTLookup *otl,uint32 tag) {
    FeatureScriptLangList *feat;

    while ( otl!=NULL ) {
	for ( feat = otl->features; feat!=NULL; feat=feat->next )
	    if ( feat->featuretag == tag )
return( true );
	otl = otl->next;
    }
return( false );
}

static int readttf(FILE *ttf, struct ttfinfo *info, char *filename) {
    int i;

    /* Determine file size to check table offset bounds */
    fseek(ttf,0,SEEK_END);
    info->ttfFileSize = ftell(ttf);
    fseek(ttf,0,SEEK_SET);

    ff_progress_change_stages(3);
    if ( !readttfheader(ttf,info,filename,&info->chosenname)) {
return( 0 );
    }
    /* TrueType doesn't need this but opentype dictionaries do */
    locale_t tmplocale; locale_t oldlocale; // Declare temporary locale storage.
    switch_to_c_locale(&tmplocale, &oldlocale); // Switch to the C locale temporarily and cache the old locale.
    readttfpreglyph(ttf,info);
    ff_progress_change_total(info->glyph_cnt);

    /* If font only contains bitmaps, then only read bitmaps */
    if ( (info->glyphlocations_start==0 || info->glyph_length==0) &&
	    info->cff_start==0 && info->typ1_start==0 &&
	    info->bitmapdata_start!=0 && info->bitmaploc_start!=0 )
	info->onlystrikes = true;

    if ( !info->onlystrikes &&
	    info->glyphlocations_start!=0 && info->glyph_start!=0 &&
	    info->cff_start!=0 ) {
	char *buts[4];
	int choice;
	buts[0] = _("TTF 'glyf'");
	buts[1] = _("OTF 'CFF '");
	buts[2] = _("_Cancel");
	buts[3] = NULL;
	choice = ff_ask(_("Pick a font, any font..."),(const char **) buts,0,2,_("This font contains both a TrueType 'glyf' table and an OpenType 'CFF ' table. FontForge can only deal with one at a time, please pick which one you want to use"));
	if ( choice==2 ) {
          switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( 0 );
	} else if ( choice==0 )
	    info->cff_start=0;
	else
	    info->glyph_start = info->glyphlocations_start = 0;
    }
    if ( info->onlystrikes ) {
	info->chars = calloc(info->glyph_cnt+1,sizeof(SplineChar *));
	info->to_order2 = new_fonts_are_order2;
    } else if ( info->glyphlocations_start!=0 && info->glyph_start!=0 ) {
	info->to_order2 = (!loaded_fonts_same_as_new ||
		(loaded_fonts_same_as_new && new_fonts_are_order2));
	/* If it's an apple mm font, then we don't want to change the order */
	/*  This messes up the point count */
	if ( info->gvar_start!=0 && info->fvar_start!=0 )
	    info->to_order2 = true;
	readttfglyphs(ttf,info);
    } else if ( info->cff_start!=0 ) {
	info->to_order2 = (loaded_fonts_same_as_new && new_fonts_are_order2);
	if ( !readcffglyphs(ttf,info) ) {
	    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( 0 );
	}
    } else if ( info->typ1_start!=0 ) {
	if ( !readtyp1glyphs(ttf,info) ) {
	    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( 0 );
	}
    } else {
	switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( 0 );
    }
    if ( info->bitmapdata_start!=0 && info->bitmaploc_start!=0 )
	TTFLoadBitmaps(ttf,info,info->onlyonestrike);
    else if ( info->onlystrikes )
	ff_post_error( _("No Bitmap Strikes"), _("No (useable) bitmap strikes in this TTF font: %s"), filename==NULL ? "<unknown>" : filename );
    if ( info->onlystrikes && info->bitmaps==NULL ) {
	free(info->chars);
	switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
return( 0 );
    }
    if ( info->hmetrics_start!=0 )
	readttfwidths(ttf,info);
    else if ( info->bitmapdata_start!=0 && info->bitmaploc_start!=0 )
	dummywidthsfromstrike(ttf,info);
    if ( info->vmetrics_start!=0 && info->vhea_start!=0 )
	readttfvwidths(ttf,info);
    /* 'cmap' is not meaningful for cid keyed fonts, and not supplied for */
    /*  type42 fonts */
    /* Oops. It is meaningful for cid fonts. It just seemed redundant to me */
    /*  but that was my ignorance. Adobe complains that FF doesn't read it */
    /* (We've already (probably) set the unicodeencs of the glyphs according */
    /*  to the cidmap files, but we can override that here. Mmm. what about a*/
    /*  glyph in cidmap but not in cmap???? */
    if ( /*info->cidregistry==NULL &&*/ info->encoding_start!=0 )
	readttfencodings(ttf,info,git_normal);
    if ( info->os2_start!=0 )
	readttfos2metrics(ttf,info);
    readttfpostnames(ttf,info);		/* If no postscript table we'll guess at names */
    if ( info->gdef_start!=0 )		/* ligature caret positioning info */
	readttfgdef(ttf,info);
    else {
	if ( info->prop_start!=0 )
	    readttfprop(ttf,info);
	if ( info->lcar_start!=0 )
	    readttflcar(ttf,info);
    }
    if ( info->base_start!=0 )
	readttfbase(ttf,info);
    else if ( info->bsln_start!=0 )
	readttfbsln(ttf,info);
    if ( info->gasp_start!=0 )
	readttfgasp(ttf,info);
    /* read the cvt table before reading variation data */
    if ( info->to_order2 ) {
	    /* Yes, even though we've looked at maxp already, let's make a blind */
	    /*  copy too for those fields we can't compute on our own */
	    /* Like size of twilight zone, etc. */
	TtfCopyTableBlindly(info,ttf,info->maxp_start,info->maxp_len,CHR('m','a','x','p'));
	TtfCopyTableBlindly(info,ttf,info->cvt_start,info->cvt_len,CHR('c','v','t',' '));
	TtfCopyTableBlindly(info,ttf,info->fpgm_start,info->fpgm_len,CHR('f','p','g','m'));
	TtfCopyTableBlindly(info,ttf,info->prep_start,info->prep_len,CHR('p','r','e','p'));
    }
    for ( i=0; i<info->savecnt; ++i ) if ( info->savetab[i].offset!=0 )
	TtfCopyTableBlindly(info,ttf,info->savetab[i].offset,info->savetab[i].len,info->savetab[i].tag);
    /* Do this before reading kerning info */
    if ( info->to_order2 && info->gvar_start!=0 && info->fvar_start!=0 )
	readttfvariations(info,ttf);
    if ( info->gpos_start!=0 )		/* kerning info may live in the gpos table too */
	readttfgpossub(ttf,info,true);
    /* Load the 'kern' table if the GPOS table either didn't exist or didn't */
    /*  contain any kerning info */
    if ( info->kern_start!=0 && !LookupListHasFeature(info->gpos_lookups,CHR('k','e','r','n')))
	readttfkerns(ttf,info);
    if ( info->opbd_start!=0 && !LookupListHasFeature(info->gpos_lookups,CHR('l','f','b','d')))
	readttfopbd(ttf,info);
    if ( info->gsub_start!=0 )
	readttfgpossub(ttf,info,false);
    if ( info->morx_start!=0 || info->mort_start!=0 )
	readttfmort(ttf,info);
    if ( info->jstf_start!=0 )
	readttfjstf(ttf,info);

    if ( info->pfed_start!=0 )
	pfed_read(ttf,info);
    if ( info->tex_start!=0 )
	tex_read(ttf,info);
    if ( info->math_start!=0 )
	otf_read_math(ttf,info);
    switch_to_old_locale(&tmplocale, &oldlocale); // Switch to the cached locale.
    if ( !info->onlystrikes && info->glyphlocations_start!=0 && info->glyph_start!=0 )
	ttfFixupReferences(info);
    /* Can't fix up any postscript references until we create a SplineFont */
    /*  so the check for cff is delayed. Generally there aren't any cff refs */
    /*  anyway */
return( true );
}

static void SymbolFixup(struct ttfinfo *info) {
    /* convert a two-byte symbol encoding (one using PUA) into expected */
    /*  one-byte encoding. */
    int i, max;
    EncMap *map = info->map;

    max = -1;
    for ( i=map->enccount-1; i>=0; --i ) {
	if ( map->map[i]==-1 )
    continue;
	if ( i>=0xf000 && i<=0xf0ff ) {
	    map->map[i-0xf000] = map->map[i];
	    map->map[i] = -1;
    continue;
	}
	if ( i>max ) max = i;
    }
    map->enccount = max;
}

void AltUniFigure(SplineFont *sf,EncMap *map,int check_dups) {
    int i,gid;

    if ( map->enc!=&custom ) {
	for ( i=0; i<map->enccount; ++i ) if ( (gid = map->map[i])!=-1 ) {
	    int uni = UniFromEnc(i,map->enc);
	    if (check_dups)
		AltUniAdd(sf->glyphs[gid],uni);
	    else
		AltUniAdd_DontCheckDups(sf->glyphs[gid],uni);
	}
    }
}

static void NameConsistancyCheck(SplineFont *sf,EncMap *map) {
    /* Many fonts seem to have glyph names which mean something other than */
    /*  what the encoding says of the glyph */
    /* I used to ask about fixing the names up, but people didn't like that */
    /*  so now I just produce warnings */
    int gid, uni;
    SplineChar *sc;

    for ( gid = 0 ; gid<sf->glyphcnt; ++gid ) if ( (sc=sf->glyphs[gid])!=NULL ) {
	if ( sc->name!=NULL &&
		strcmp(sc->name,".null")!=0 &&
		strcmp(sc->name,"nonmarkingreturn")!=0 &&
		(uni = UniFromName(sc->name,sf->uni_interp,map==NULL ? &custom : map->enc))!= -1 &&
		sc->unicodeenc != uni ) {
	    if ( uni>=0xe000 && uni<=0xf8ff )
		/* Don't complain about adobe's old PUA assignments for things like "eight.oldstyle" */;
	    else if ( uni<0x20 )
		/* Nor about control characters */;
	    else if ( sc->unicodeenc==-1 ) {
	    } else {
		/* Ah, but suppose there's an altuni? */
		struct altuni *alt;
		for ( alt = sc->altuni; alt!=NULL && alt->unienc!=uni; alt=alt->next );
		if ( alt==NULL )
		{
                   if ( strcmp(sc->name,"alefmaksurainitialarabic")==0 ||
                        strcmp(sc->name,"alefmaksuramedialarabic")==0 )
                   {
                      LogError( _("The names 'alefmaksurainitialarabic' and 'alefmaksuramedialarabic' in the Adobe Glyph List disagree with Unicode.  The use of these glyph names is therefore discouraged.\n") );
                   } else {
		      LogError( _("The glyph named %.30s is mapped to U+%04X.\nBut its name indicates it should be mapped to U+%04X.\n"),
			    sc->name,sc->unicodeenc, uni);
		   }
		}
		else if ( alt->vs==0 ) {
		    alt->unienc = sc->unicodeenc;
		    sc->unicodeenc = uni;
		}
	    }
	}
    }
}

static void UseGivenEncoding(SplineFont *sf,struct ttfinfo *info) {
    int i;
    RefChar *rf, *prev, *next;
    SplineChar *sc;

    sf->glyphs = info->chars;
    sf->glyphcnt = sf->glyphmax = info->glyph_cnt;
    for ( i=0; i<sf->glyphcnt; ++i )
	if ( (sc = sf->glyphs[i])!=NULL ) {
	    sc->layers[ly_fore].order2 = sc->layers[ly_back].order2 = info->to_order2;
	    sc->parent = sf;
	}

    /* A CFF font could contain type1 charstrings, or a type2 font could use */
    /*  the deprecated convention that endchar =~ seac */
    if ( info->cff_length!=0 )
	SFInstanciateRefs(sf);

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( rf = sf->glyphs[i]->layers[ly_fore].refs, prev=NULL; rf!=NULL; rf = next ) {
	    next = rf->next;
	    if ( rf->sc==NULL ) {
		if ( prev==NULL ) sf->glyphs[i]->layers[ly_fore].refs = next;
		else prev->next = next;
		RefCharFree(rf);
	    } else {
		rf->orig_pos = rf->sc->orig_pos;
		rf->unicode_enc = rf->sc->unicodeenc;
		prev = rf;
	    }
	}
    }
    sf->map = info->map;
    sf->uni_interp = info->uni_interp;
    AltUniFigure(sf,sf->map,false);
    NameConsistancyCheck(sf, sf->map);
}

static char *AxisNameConvert(uint32 tag) {
    char buffer[8];

    if ( tag==CHR('w','g','h','t'))
return( copy("Weight"));
    if ( tag==CHR('w','d','t','h'))
return( copy("Width"));
    if ( tag==CHR('o','p','s','z'))
return( copy("OpticalSize"));
    if ( tag==CHR('s','l','n','t'))
return( copy("Slant"));

    buffer[0] = tag>>24;
    buffer[1] = tag>>16;
    buffer[2] = tag>>8;
    buffer[3] = tag&0xff;
    buffer[4] = 0;
return( copy(buffer ));
}

static struct macname *FindMacName(struct ttfinfo *info, int strid) {
    struct macidname *sid;

    for ( sid=info->macstrids; sid!=NULL; sid=sid->next ) {
	if ( sid->id == strid )
return( sid->head );
    }
return( NULL );
}

static SplineFont *SFFromTuple(SplineFont *basesf,struct variations *v,int tuple,
	MMSet *mm, struct ttfinfo *info) {
    SplineFont *sf;
    int i;
    RefChar *r;

    sf = SplineFontEmpty();
    sf->display_size = basesf->display_size;
    sf->display_antialias = basesf->display_antialias;

    sf->fontname = MMMakeMasterFontname(mm,tuple,&sf->fullname);
    sf->familyname = copy(basesf->familyname);
    sf->weight = copy("All");
    sf->italicangle = basesf->italicangle;
    sf->strokewidth = basesf->strokewidth;
    sf->strokedfont = basesf->strokedfont;
    sf->upos = basesf->upos;
    sf->uwidth = basesf->uwidth;
    sf->ascent = basesf->ascent;
    sf->hasvmetrics = basesf->hasvmetrics;
    sf->descent = basesf->descent;
    sf->kerns = v->tuples[tuple].khead;
    sf->vkerns = v->tuples[tuple].vkhead;
    sf->map = basesf->map;
    sf->mm = mm;
    sf->glyphmax = sf->glyphcnt = basesf->glyphcnt;
    sf->glyphs = v->tuples[tuple].chars;
    sf->layers[ly_fore].order2 = sf->layers[ly_back].order2 = true;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( basesf->glyphs[i]!=NULL && sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];
	sc->orig_pos = i;
	sc->parent = sf;
	sc->layers[ly_fore].order2 = sc->layers[ly_back].order2 = true;
    }
    sf->grid.order2 = true;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( r=sf->glyphs[i]->layers[ly_fore].refs; r!=NULL; r=r->next )
	    SCReinstanciateRefChar(sf->glyphs[i],r,ly_fore);
    }

    sf->ttf_tables = v->tuples[tuple].cvt;

    v->tuples[tuple].chars = NULL;
    v->tuples[tuple].khead = NULL;
    v->tuples[tuple].vkhead = NULL;
    v->tuples[tuple].cvt = NULL;
return( sf );
}

static void MMFillFromVAR(SplineFont *sf, struct ttfinfo *info) {
    MMSet *mm = chunkalloc(sizeof(MMSet));
    struct variations *v = info->variations;
    int i,j;

    sf->mm = mm;
    mm->normal = sf;
    mm->apple = true;
    mm->axis_count = v->axis_count;
    mm->instance_count = v->tuple_count;
    mm->instances = malloc(v->tuple_count*sizeof(SplineFont *));
    mm->positions = malloc(v->tuple_count*v->axis_count*sizeof(real));
    for ( i=0; i<v->tuple_count; ++i ) for ( j=0; j<v->axis_count; ++j )
	mm->positions[i*v->axis_count+j] = v->tuples[i].coords[j];
    mm->defweights = calloc(v->tuple_count,sizeof(real));	/* Doesn't apply */
    mm->axismaps = calloc(v->axis_count,sizeof(struct axismap));
    for ( i=0; i<v->axis_count; ++i ) {
	mm->axes[i] = AxisNameConvert(v->axes[i].tag);
	mm->axismaps[i].min = v->axes[i].min;
	mm->axismaps[i].def = v->axes[i].def;
	mm->axismaps[i].max = v->axes[i].max;
	if ( v->axes[i].paircount==0 ) {
	    mm->axismaps[i].points = 3;
	    mm->axismaps[i].blends = malloc(3*sizeof(real));
	    mm->axismaps[i].designs = malloc(3*sizeof(real));
	    mm->axismaps[i].blends[0] = -1; mm->axismaps[i].designs[0] = mm->axismaps[i].min;
	    mm->axismaps[i].blends[1] =  0; mm->axismaps[i].designs[1] = mm->axismaps[i].def;
	    mm->axismaps[i].blends[2] =  1; mm->axismaps[i].designs[2] = mm->axismaps[i].max;
	} else {
	    mm->axismaps[i].points = v->axes[i].paircount;
	    mm->axismaps[i].blends = malloc(v->axes[i].paircount*sizeof(real));
	    mm->axismaps[i].designs = malloc(v->axes[i].paircount*sizeof(real));
	    for ( j=0; j<v->axes[i].paircount; ++j ) {
		if ( v->axes[i].mapfrom[j]<=0 ) {
		    mm->axismaps[i].designs[j] = mm->axismaps[i].def +
			    v->axes[i].mapfrom[j]*(mm->axismaps[i].def-mm->axismaps[i].min);
		} else {
		    mm->axismaps[i].designs[j] = mm->axismaps[i].def +
			    v->axes[i].mapfrom[j]*(mm->axismaps[i].max-mm->axismaps[i].def);
		}
		mm->axismaps[i].blends[j] = v->axes[i].mapto[j];
	    }
	}
	mm->axismaps[i].axisnames = MacNameCopy(FindMacName(info, v->axes[i].nameid));
    }
    mm->named_instance_count = v->instance_count;
    mm->named_instances = malloc(v->instance_count*sizeof(struct named_instance));
    for ( i=0; i<v->instance_count; ++i ) {
	mm->named_instances[i].coords = v->instances[i].coords;
	v->instances[i].coords = NULL;
	mm->named_instances[i].names = MacNameCopy(FindMacName(info, v->instances[i].nameid));
    }
    for ( i=0; i<mm->instance_count; ++i )
	mm->instances[i] = SFFromTuple(sf,v,i,mm,info);
    VariationFree(info);
}

static void PsuedoEncodeUnencoded(EncMap *map,struct ttfinfo *info) {
    int extras, base;
    int i;

    for ( i=0; i<info->glyph_cnt; ++i )
	if ( info->chars[i]!=NULL )
	    info->chars[i]->ticked = false;
    for ( i=0; i<map->enccount; ++i )
	if ( map->map[i]!=-1 )
	    info->chars[map->map[i]]->ticked = true;
    extras = 0;
    for ( i=0; i<info->glyph_cnt; ++i )
	if ( info->chars[i]!=NULL && !info->chars[i]->ticked )
	    ++extras;
    if ( extras!=0 ) {
	if ( map->enccount<=256 )
	    base = 256;
	else if ( map->enccount<=65536 )
	    base = 65536;
	else if ( map->enccount<=17*65536 )
	    base = 17*65536;
	else
	    base = map->enccount;
	if ( base+extras>map->encmax ) {
	    map->map = realloc(map->map,(base+extras)*sizeof(int));
	    memset(map->map+map->enccount,-1,(base+extras-map->enccount)*sizeof(int));
	    map->encmax = base+extras;
	}
	map->enccount = base+extras;
	extras = 0;
	for ( i=0; i<info->glyph_cnt; ++i )
	    if ( info->chars[i]!=NULL && !info->chars[i]->ticked )
		map->map[base+extras++] = i;
    }
}

static void MapDoBack(EncMap *map,struct ttfinfo *info) {
    int i;

    if ( map==NULL )		/* CID fonts */
return;
    free(map->backmap);		/* CFF files have this */
    map->backmax = info->glyph_cnt;
    map->backmap = malloc(info->glyph_cnt*sizeof(int));
    memset(map->backmap,-1,info->glyph_cnt*sizeof(int));
    for ( i = map->enccount-1; i>=0; --i )
	if ( map->map[i]>=0 && map->map[i]<info->glyph_cnt )
	    if ( map->backmap[map->map[i]]==-1 )
		map->backmap[map->map[i]] = i;
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
	    strcmp(english->names[ttf_family],sf->familyname)==0 ) {
	free(english->names[ttf_family]);
	english->names[ttf_family]=NULL;
    }
    if ( english->names[ttf_copyright]!=NULL &&
	    strcmp(english->names[ttf_copyright],sf->copyright)==0 ) {
	free(english->names[ttf_copyright]);
	english->names[ttf_copyright]=NULL;
    }
    if ( english->names[ttf_fullname]!=NULL &&
	    strcmp(english->names[ttf_fullname],sf->fullname)==0 ) {
	free(english->names[ttf_fullname]);
	english->names[ttf_fullname]=NULL;
    }
    if ( sf->subfontcnt!=0 || sf->version!=NULL ) {
	if ( sf->subfontcnt!=0 )
	    sprintf( versionbuf, "Version %f", sf->cidversion );
	else
	    sprintf(versionbuf,"Version %.20s ", sf->version);
	if ( english->names[ttf_version]!=NULL &&
		strcmp(english->names[ttf_version],versionbuf)==0 ) {
	    free(english->names[ttf_version]);
	    english->names[ttf_version]=NULL;
	}
    }
    if ( english->names[ttf_subfamily]!=NULL &&
	    strcmp(english->names[ttf_subfamily],SFGetModifiers(sf))==0 ) {
	free(english->names[ttf_subfamily]);
	english->names[ttf_subfamily]=NULL;
    }

    /* User should not be allowed any access to this one, not ever */
    free(english->names[ttf_postscriptname]);
    english->names[ttf_postscriptname]=NULL;
}

static void ASCIIcheck(char **str) {

    if ( *str!=NULL && !AllAscii(*str)) {
	char *temp = StripToASCII(*str);
	free(*str);
	*str = temp;
    }
}

static SplineFont *SFFillFromTTF(struct ttfinfo *info) {
    SplineFont *sf, *_sf;
    int i,k;
    BDFFont *bdf;
    SplineChar *sc;
    struct ttf_table *last[2], *tab, *next;

    sf = SplineFontEmpty();
    sf->display_size = -default_fv_font_size;
    sf->display_antialias = default_fv_antialias;
    sf->fontname = info->fontname;
    sf->fullname = info->fullname;
    sf->familyname = info->familyname;
    sf->chosenname = info->chosenname;
    sf->onlybitmaps = info->onlystrikes;
    sf->layers[ly_fore].order2 = info->to_order2;
    sf->layers[ly_back].order2 = info->to_order2;
    sf->grid.order2 = info->to_order2;
    sf->comments = info->fontcomments;
    sf->fontlog = info->fontlog;
    sf->cvt_names = info->cvt_names;

    sf->creationtime = info->creationtime;
    sf->modificationtime = info->modificationtime;

    sf->design_size = info->design_size;
    sf->design_range_bottom = info->design_range_bottom;
    sf->design_range_top = info->design_range_top;
    sf->fontstyle_id = info->fontstyle_id;
    sf->fontstyle_name = info->fontstyle_name;
    sf->feat_names = info->feat_names;

    sf->gasp_cnt = info->gasp_cnt;
    sf->gasp = info->gasp;
    sf->MATH = info->math;

    sf->texdata = info->texdata;

    sf->mark_class_cnt = info->mark_class_cnt;
    sf->mark_classes = info->mark_classes;
    sf->mark_class_names = info->mark_class_names;

    sf->mark_set_cnt = info->mark_set_cnt;
    sf->mark_sets = info->mark_sets;
    sf->mark_set_names = info->mark_set_names;

    if ( info->fd!=NULL ) {		/* Special hack for type42 fonts */
	sf->fontname = copy(info->fd->fontname);
	sf->uniqueid = info->fd->uniqueid;
	sf->xuid = XUIDFromFD(info->fd->xuid);
	if ( info->fd->fontinfo!=NULL ) {
	    sf->familyname = utf8_verify_copy(info->fd->fontinfo->familyname);
	    sf->fullname = utf8_verify_copy(info->fd->fontinfo->fullname);
	    sf->copyright = utf8_verify_copy(info->fd->fontinfo->notice);
	    sf->weight = utf8_verify_copy(info->fd->fontinfo->weight);
	    sf->version = utf8_verify_copy(info->fd->fontinfo->version);
	    sf->italicangle = info->fd->fontinfo->italicangle;
	    sf->upos = info->fd->fontinfo->underlineposition*(sf->ascent+sf->descent);
	    sf->uwidth = info->fd->fontinfo->underlinethickness*(sf->ascent+sf->descent);
	}
    }

    if ( sf->fontname==NULL ) {
	sf->fontname = EnforcePostScriptName(sf->fullname);
	if ( sf->fontname==NULL )
	    sf->fontname = EnforcePostScriptName(sf->familyname);
	if ( sf->fontname==NULL ) sf->fontname = EnforcePostScriptName("UntitledTTF");
    }
    if ( sf->fullname==NULL ) sf->fullname = copy( sf->fontname );
    if ( sf->familyname==NULL ) sf->familyname = copy( sf->fontname );
    if ( sf->weight==NULL ) {
	if ( info->weight != NULL )
	    sf->weight = info->weight;
	else if ( info->pfminfo.pfmset )
	    sf->weight = copy( info->pfminfo.weight <= 100 ? "Thin" :
				info->pfminfo.weight <= 200 ? "Extra-Light" :
				info->pfminfo.weight <= 300 ? "Light" :
				info->pfminfo.weight <= 400 ? "Book" :
				info->pfminfo.weight <= 500 ? "Medium" :
				info->pfminfo.weight <= 600 ? "Demi" :
				info->pfminfo.weight <= 700 ? "Bold" :
				info->pfminfo.weight <= 800 ? "Heavy" :
				    "Black" );
	else
	    sf->weight = copy("");
    } else
	free( info->weight );
    if ( sf->copyright==NULL )
	sf->copyright = info->copyright;
    else
	free( info->copyright );
    sf->version = info->version;
    sf->italicangle = info->italicAngle;
    sf->strokewidth = info->strokewidth;
    sf->strokedfont = info->strokedfont;
    sf->upos = info->upos;
    sf->uwidth = info->uwidth;
    sf->ascent = info->ascent;
    if ( info->vhea_start!=0 && info->vmetrics_start!=0 )
	sf->hasvmetrics = true;
    sf->descent = info->descent;
    sf->private = info->private;
    sf->xuid = info->xuid;
    sf->uniqueid = info->uniqueid;
    sf->pfminfo = info->pfminfo;
    sf->os2_version = info->os2_version;
    sf->sfntRevision = info->sfntRevision;
    sf->use_typo_metrics = info->use_typo_metrics;
    sf->weight_width_slope_only = info->weight_width_slope_only;
    sf->head_optimized_for_cleartype = info->optimized_for_cleartype;
    sf->gasp_version = info->gasp_version;
    sf->names = info->names;
    sf->anchor = info->ahead;
    sf->kerns = info->khead;
    sf->vkerns = info->vkhead;
    sf->possub = info->possub;
    sf->sm = info->sm;
    sf->features = info->features;
    sf->gpos_lookups = info->gpos_lookups;
    sf->gsub_lookups = info->gsub_lookups;

    last[0] = sf->ttf_tables;
    last[1] = NULL;
    for ( tab=info->tabs; tab!=NULL; tab = next ) {
	next = tab->next;
	if ( tab->tag==CHR('f','p','g','m') || tab->tag==CHR('p','r','e','p') ||
		tab->tag==CHR('c','v','t',' ') || tab->tag==CHR('m','a','x','p')) {
	    if ( last[0]==NULL )
		sf->ttf_tables = tab;
	    else
		last[0]->next = tab;
	    last[0] = tab;
	} else {
	    if ( last[1]==NULL )
		sf->ttf_tab_saved = tab;
	    else
		last[1]->next = tab;
	    last[1] = tab;
	}
	tab->next = NULL;
    }

    if ( info->twobytesymbol )
	/* rework ms symbol encodings */
	SymbolFixup(info);
    if ( info->map==NULL && info->subfonts==NULL )		/* Can happen when reading a ttf from a pdf */
	info->map = EncMapFromEncoding(sf,FindOrMakeEncoding("original"));
    if ( info->subfontcnt==0 )
	PsuedoEncodeUnencoded(info->map,info);
    MapDoBack(info->map,info);
    sf->map = info->map;
    sf->cidregistry = info->cidregistry;
    sf->ordering = info->ordering;
    sf->supplement = info->supplement;
    sf->cidversion = info->cidfontversion;
    sf->bitmaps = info->bitmaps;
    sf->grid = info->guidelines;
    sf->horiz_base = info->horiz_base;
    sf->vert_base = info->vert_base;
    sf->justify = info->justify;
    for ( bdf = info->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	bdf->sf = sf;
    }
    SFDefaultAscent(sf);
    if ( info->layers!=NULL ) {
	info->layers[ly_fore] = sf->layers[ly_fore];
	sf->layers[ly_fore].name = NULL;
	if ( info->layers[ly_back].name==NULL )
	    info->layers[ly_back].name = sf->layers[ly_back].name;
	else
	    free( sf->layers[ly_back].name );
	free( sf->layers );
	sf->layers = info->layers;
	sf->layer_cnt = info->layer_cnt;
    }
	

    for ( i=0; i<info->glyph_cnt; ++i ) if ( info->chars[i]!=NULL ) {
	SCOrderAP(info->chars[i]);
    }

    if ( info->subfontcnt == 0 ) {
	UseGivenEncoding(sf,info);
    } else {
	sf->subfontcnt = info->subfontcnt;
	sf->subfonts = info->subfonts;
	free(info->chars);		/* This is the GID->char index, don't need it now */
	for ( i=0; i<sf->subfontcnt; ++i ) {
	    sf->subfonts[i]->cidmaster = sf;
	    sf->subfonts[i]->hasvmetrics = sf->hasvmetrics;
	}
    }
    ASCIIcheck(&sf->copyright);
    ASCIIcheck(&sf->familyname);
    ASCIIcheck(&sf->weight);
    ASCIIcheck(&sf->version);
    
    TTF_PSDupsDefault(sf);

    /* I thought the languages were supposed to be ordered, but it seems */
    /*  that is not always the case. Order everything, just in case */
    { int isgpos; OTLookup *otl;
    for ( isgpos=0; isgpos<2; ++isgpos )
	for ( otl= isgpos? sf->gpos_lookups:sf->gsub_lookups; otl!=NULL; otl=otl->next )
	    otl->features = FLOrder(otl->features);
    }

    if ( info->variations!=NULL )
	MMFillFromVAR(sf,info);

    if ( info->cff_length!=0 && !sf->layers[ly_fore].order2 ) {
	/* Clean up the hint masks, We create an initial hintmask whether we */
	/*  need it or not */
	k=0;
	do {
	    _sf = k<sf->subfontcnt?sf->subfonts[k]:sf;
	    for ( i=0; i<sf->glyphcnt; ++i ) {
		if ( (sc = _sf->glyphs[i])!=NULL && !sc->hconflicts && !sc->vconflicts &&
			sc->layers[ly_fore].splines!=NULL ) {
		    chunkfree( sc->layers[ly_fore].splines->first->hintmask,sizeof(HintMask) );
		    sc->layers[ly_fore].splines->first->hintmask = NULL;
		}
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
    }
    /* should not be default as it confuses users */
    /* SFRelativeWinAsDs(sf); */
    free(info->savetab);

    if ( info->openflags & of_fontlint ) {
	k=0;
	do {
	    _sf = k<sf->subfontcnt?sf->subfonts[k]:sf;
	    for ( i=0; i<sf->glyphcnt; ++i ) {
		if ( (sc = _sf->glyphs[i])!=NULL ) {
		    DBounds b;
		    SplineCharQuickBounds(sc,&b);
		    if ( b.minx==0 && b.maxx==0 )
			/* Skip it, no points */;
		    else if ( b.minx < info->fbb[0] || b.miny < info->fbb[1] ||
			    b.maxx > info->fbb[2] || b.maxy > info->fbb[3] ) {
			LogError(_("A point in %s is outside the font bounding box data.\n"), sc->name );
			info->bad_cff = true;
		    }
		    if ( info->isFixedPitch && i>2 && sc->width!=info->advanceWidthMax )
			LogError(_("The advance width of %s (%d) does not match the font's advanceWidthMax (%d) and this is a fixed pitch font\n"),
				sc->name, sc->width, info->advanceWidthMax );
		}
	    }
	    ++k;
	} while ( k<sf->subfontcnt );
    }

    sf->loadvalidation_state =
	    (info->bad_ps_fontname	?lvs_bad_ps_fontname:0) |
	    (info->bad_glyph_data	?lvs_bad_glyph_table:0) |
	    (info->bad_cff		?lvs_bad_cff_table:0) |
	    (info->bad_metrics		?lvs_bad_metrics_table:0) |
	    (info->bad_cmap		?lvs_bad_cmap_table:0) |
	    (info->bad_embedded_bitmap	?lvs_bad_bitmaps_table:0) |
	    (info->bad_gx		?lvs_bad_gx_table:0) |
	    (info->bad_ot		?lvs_bad_ot_table:0) |
	    (info->bad_os2_version	?lvs_bad_os2_version:0)|
	    (info->bad_sfnt_header	?lvs_bad_sfnt_header:0);
return( sf );
}

SplineFont *_SFReadTTF(FILE *ttf, int flags,enum openflags openflags, char *filename,struct fontdict *fd) {
    struct ttfinfo info;
    int ret;

    memset(&info,'\0',sizeof(struct ttfinfo));
    info.onlystrikes = (flags&ttf_onlystrikes)?1:0;
    info.onlyonestrike = (flags&ttf_onlyonestrike)?1:0;
    info.use_typo_metrics = false;
    info.weight_width_slope_only = false;
    info.openflags = openflags;
    info.fd = fd;
    ret = readttf(ttf,&info,filename);
    if ( !ret )
return( NULL );
return( SFFillFromTTF(&info));
}

SplineFont *SFReadTTF(char *filename, int flags, enum openflags openflags) {
    FILE *ttf;
    SplineFont *sf;
    char *temp=filename, *pt, *lparen, *rparen;

    pt = strrchr(filename,'/');
    if ( pt==NULL ) pt = filename;
    if ( (lparen = strrchr(pt,'('))!=NULL &&
	    (rparen = strrchr(lparen,')'))!=NULL &&
	    rparen[1]=='\0' ) {
	temp = copy(filename);
	pt = temp + (lparen-filename);
	*pt = '\0';
    }
    ttf = fopen(temp,"rb");
    if ( temp!=filename ) free(temp);
    if ( ttf==NULL )
return( NULL );

    sf = _SFReadTTF(ttf,flags,openflags,filename,NULL);
    fclose(ttf);
return( sf );
}

SplineFont *_CFFParse(FILE *temp,int len, char *fontsetname) {
    struct ttfinfo info;

    memset(&info,'\0',sizeof(info));
    info.cff_start = 0;
    info.cff_length = len;
    info.barecff = true;
    if ( !readcffglyphs(temp,&info) )
return( NULL );
return( SFFillFromTTF(&info));
}

SplineFont *CFFParse(char *filename) {
    FILE *cff = fopen(filename,"r");
    SplineFont *sf;
    long len;

    if ( cff == NULL )
return( NULL );
    fseek(cff,0,SEEK_END);
    len = ftell(cff);
    fseek(cff,0,SEEK_SET);
    sf = _CFFParse(cff,len,NULL);
    fclose(cff);
return( sf );
}

char **NamesReadCFF(char *filename) {
    FILE *cff = fopen(filename,"rb");
    int32 hdrsize, offsize;
    char **fontnames;

    if ( cff==NULL )
return( NULL );
    if ( getc(cff)!='\1' ) {		/* Major version */
	LogError( _("CFF version mismatch\n") );
	fclose(cff);
return( NULL );
    }
    getc(cff);				/* Minor version */
    hdrsize = getc(cff);
    offsize = getc(cff);
    if ( hdrsize!=4 )
	fseek(cff,hdrsize,SEEK_SET);
    fontnames = readcfffontnames(cff,NULL,NULL);
    fclose(cff);
return( fontnames );
}

char **NamesReadTTF(char *filename) {
    FILE *ttf = fopen(filename,"rb");
    int32 version, cnt, *offsets;
    int i,j;
    char **ret = NULL;
    char *temp;

    if ( ttf==NULL )
return( NULL );
    version=getlong(ttf);
    if ( version==CHR('t','t','c','f')) {
	/* TTCF version = */ getlong(ttf);
	cnt = getlong(ttf);
	if (cnt != EOF && cnt >= 0 && cnt < 0xFFFF) {
		offsets = malloc(cnt*sizeof(int32));
		for ( i=0; i<cnt; ++i )
		    offsets[i] = getlong(ttf);
		ret = malloc((cnt+1)*sizeof(char *));
		for ( i=j=0; i<cnt; ++i ) {
		    temp = TTFGetFontName(ttf,offsets[i],0);
		    if ( temp!=NULL )
			ret[j++] = temp;
		}
		ret[j] = NULL;
		free(offsets);
	} else {
		LogError(_("Invalid font count in TTC %s."), filename);
	}
    } else {
	temp = TTFGetFontName(ttf,0,0);
	if ( temp!=NULL ) {
	    ret = malloc(2*sizeof(char *));
	    ret[0] = temp;
	    ret[1] = NULL;
	}
    }
    fclose(ttf);
return(ret);
}
