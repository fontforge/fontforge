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

#include "encoding.h"

#include "bitmapchar.h"
#include "bvedit.h"
#include "dumppfa.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "namelist.h"
#include "psread.h"
#include "pua.h"
#include "splinefont.h"
#include "splinefill.h"
#include "splinesaveafm.h"
#include "splineutil.h"
#include "splineutil2.h"
#include <ustring.h>
#include <utype.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <gfile.h>
#include "encoding.h"
#include "psfont.h"
#include <ffglib.h>
#include <glib/gprintf.h>
#include "xvasprintf.h"

Encoding *default_encoding = NULL;

static int32 tex_base_encoding[] = {
    0x0000, 0x02d9, 0xfb01, 0xfb02, 0x2044, 0x02dd, 0x0141, 0x0142,
    0x02db, 0x02da, 0x000a, 0x02d8, 0x2212, 0x000d, 0x017d, 0x017e,
    0x02c7, 0x0131, 0xf6be, 0xfb00, 0xfb03, 0xfb04, 0x2260, 0x221e,
    0x2264, 0x2265, 0x2202, 0x2211, 0x220f, 0x03c0, 0x0060, 0x0027,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x2019,
    0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
    0x2018, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x007f,
    0x20ac, 0x222b, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
    0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0x2126, 0x221a, 0x2248,
    0x0090, 0x0091, 0x0092, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
    0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x2206, 0x25ca, 0x0178,
    0x0000, 0x00a1, 0x00a2, 0x00a3, 0x00a4, 0x00a5, 0x00a6, 0x00a7,
    0x00a8, 0x00a9, 0x00aa, 0x00ab, 0x00ac, 0x002d, 0x00ae, 0x00af,
    0x00b0, 0x00b1, 0x00b2, 0x00b3, 0x00b4, 0x00b5, 0x00b6, 0x00b7,
    0x00b8, 0x00b9, 0x00ba, 0x00bb, 0x00bc, 0x00bd, 0x00be, 0x00bf,
    0x00c0, 0x00c1, 0x00c2, 0x00c3, 0x00c4, 0x00c5, 0x00c6, 0x00c7,
    0x00c8, 0x00c9, 0x00ca, 0x00cb, 0x00cc, 0x00cd, 0x00ce, 0x00cf,
    0x00d0, 0x00d1, 0x00d2, 0x00d3, 0x00d4, 0x00d5, 0x00d6, 0x00d7,
    0x00d8, 0x00d9, 0x00da, 0x00db, 0x00dc, 0x00dd, 0x00de, 0x00df,
    0x00e0, 0x00e1, 0x00e2, 0x00e3, 0x00e4, 0x00e5, 0x00e6, 0x00e7,
    0x00e8, 0x00e9, 0x00ea, 0x00eb, 0x00ec, 0x00ed, 0x00ee, 0x00ef,
    0x00f0, 0x00f1, 0x00f2, 0x00f3, 0x00f4, 0x00f5, 0x00f6, 0x00f7,
    0x00f8, 0x00f9, 0x00fa, 0x00fb, 0x00fc, 0x00fd, 0x00fe, 0x00ff
};

static int32 unicode_from_MacSymbol[] = {
  0x0000, 0x0001, 0x0002, 0x0003, 0x0004, 0x0005, 0x0006, 0x0007,
  0x0008, 0x0009, 0x000a, 0x000b, 0x000c, 0x000d, 0x000e, 0x000f,
  0x0010, 0x0011, 0x0012, 0x0013, 0x0014, 0x0015, 0x0016, 0x0017,
  0x0018, 0x0019, 0x001a, 0x001b, 0x001c, 0x001d, 0x001e, 0x001f,
  0x0020, 0x0021, 0x2200, 0x0023, 0x2203, 0x0025, 0x0026, 0x220d,
  0x0028, 0x0029, 0x2217, 0x002b, 0x002c, 0x2212, 0x002e, 0x002f,
  0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
  0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
  0x2245, 0x0391, 0x0392, 0x03a7, 0x0394, 0x0395, 0x03a6, 0x0393,
  0x0397, 0x0399, 0x03d1, 0x039a, 0x039b, 0x039c, 0x039d, 0x039f,
  0x03a0, 0x0398, 0x03a1, 0x03a3, 0x03a4, 0x03a5, 0x03c2, 0x03a9,
  0x039e, 0x03a8, 0x0396, 0x005b, 0x2234, 0x005d, 0x22a5, 0x005f,
  0xf8e5, 0x03b1, 0x03b2, 0x03c7, 0x03b4, 0x03b5, 0x03c6, 0x03b3,
  0x03b7, 0x03b9, 0x03d5, 0x03ba, 0x03bb, 0x03bc, 0x03bd, 0x03bf,
  0x03c0, 0x03b8, 0x03c1, 0x03c3, 0x03c4, 0x03c5, 0x03d6, 0x03c9,
  0x03be, 0x03c8, 0x03b6, 0x007b, 0x007c, 0x007d, 0x223c, 0x007f,
  0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087,
  0x0088, 0x0089, 0x008a, 0x008b, 0x008c, 0x008d, 0x008e, 0x008f,
  0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095, 0x0096, 0x0097,
  0x0098, 0x0099, 0x009a, 0x009b, 0x009c, 0x009d, 0x009e, 0x009f,
  0x0000, 0x03d2, 0x2032, 0x2264, 0x2044, 0x221e, 0x0192, 0x2663,
  0x2666, 0x2665, 0x2660, 0x2194, 0x2190, 0x2191, 0x2192, 0x2193,
  0x00b0, 0x00b1, 0x2033, 0x2265, 0x00d7, 0x221d, 0x2202, 0x2022,
  0x00f7, 0x2260, 0x2261, 0x2248, 0x2026, 0xf8e6, 0xf8e7, 0x21b5,
  0x2135, 0x2111, 0x211c, 0x2118, 0x2297, 0x2295, 0x2205, 0x2229,
  0x222a, 0x2283, 0x2287, 0x2284, 0x2282, 0x2286, 0x2208, 0x2209,
  0x2220, 0x2207, 0x00ae, 0x00a9, 0x2122, 0x220f, 0x221a, 0x22c5,
  0x00ac, 0x2227, 0x2228, 0x21d4, 0x21d0, 0x21d1, 0x21d2, 0x21d3,
  0x22c4, 0x2329, 0xf8e8, 0xf8e9, 0xf8ea, 0x2211, 0xf8eb, 0xf8ec,
  0xf8ed, 0xf8ee, 0xf8ef, 0xf8f0, 0xf8f1, 0xf8f2, 0xf8f3, 0xf8f4,
  0xf8ff, 0x232a, 0x222b, 0x2320, 0xf8f5, 0x2321, 0xf8f6, 0xf8f7,
  0xf8f8, 0xf8f9, 0xf8fa, 0xf8fb, 0xf8fc, 0xf8fd, 0xf8fe, 0x02c7
};

/* I don't think iconv provides encodings for zapfdingbats nor jis201 */
/*  Perhaps I should list them here for compatability, but I think I'll just */
/*  leave them out. I doubt they get used. */
static Encoding texbase = { "TeX-Base-Encoding", 256, tex_base_encoding, NULL, NULL, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", 0, 0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0 };
       Encoding custom = { "Custom", 0, NULL, NULL, &texbase,                        1, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, "", 0, 0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0 };
static Encoding original = { "Original", 0, NULL, NULL, &custom,                     1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, "", 0, 0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0 };
static Encoding unicodebmp = { "UnicodeBmp", 65536, NULL, NULL, &original,           1, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, "", 0, 0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0 };
static Encoding unicodefull = { "UnicodeFull", 17*65536, NULL, NULL, &unicodebmp,    1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, "", 0, 0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0 };
static Encoding adobestd = { "AdobeStandard", 256, unicode_from_adobestd, (char**)AdobeStandardEncoding, &unicodefull,
                                                                                     1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", 0, 0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0 };
static Encoding symbol = { "Symbol", 256, unicode_from_MacSymbol, NULL, &adobestd,   1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "", 0, 0, 0, NULL, NULL, NULL, NULL, NULL, 0, 0 };

Encoding *enclist = &symbol;

const char *FindUnicharName(void) {
    /* Iconv and libiconv use different names for UCS2. Just great. Perhaps */
    /*  different versions of each use still different names? */
    /* Even worse, both accept UCS-2, but under iconv it means native byte */
    /*  ordering and under libiconv it means big-endian */
    iconv_t test;
    static const char *goodname = NULL;
    static const char *names[] = { "UCS-4-INTERNAL", "UCS-4", "UCS4", "ISO-10646-UCS-4", "UTF-32", NULL };
    static const char *namesle[] = { "UCS-4LE", "UTF-32LE", NULL };
    static const char *namesbe[] = { "UCS-4BE", "UTF-32BE", NULL };
    const char **testnames;
    int i;
    union {
	short s;
	char c[2];
    } u;

    if ( goodname!=NULL )
return( goodname );

    u.c[0] = 0x1; u.c[1] = 0x2;
    if ( u.s==0x201 ) {		/* Little endian */
	testnames = namesle;
    } else {
	testnames = namesbe;
    }
    for ( i=0; testnames[i]!=NULL; ++i ) {
	test = iconv_open(testnames[i],"ISO-8859-1");
	if ( test!=(iconv_t) -1 && test!=NULL ) {
	    iconv_close(test);
	    goodname = testnames[i];
    break;
	}
    }

    if ( goodname==NULL ) {
	for ( i=0; names[i]!=NULL; ++i ) {
	    test = iconv_open(names[i],"ISO-8859-1");
	    if ( test!=(iconv_t) -1 && test!=NULL ) {
		iconv_close(test);
		goodname = names[i];
	break;
	    }
	}
    }

    if ( goodname==NULL ) {
	IError( "I can't figure out your version of iconv(). I need a name for the UCS-4 encoding and I can't find one. Reconfigure --without-iconv. Bye.");
	exit( 1 );
    }

    test = iconv_open(goodname,"Mac");
    if ( test==(iconv_t) -1 || test==NULL ) {
	IError( "Your version of iconv does not support the \"Mac Roman\" encoding.\nIf this causes problems, reconfigure --without-iconv." );
    } else
	iconv_close(test);

    /* I really should check for ISO-2022-JP, KR, CN, and all the other encodings */
    /*  I might find in a ttf 'name' table. But those tables take too long to build */
return( goodname );
}

static int TryEscape( Encoding *enc, const char *escape_sequence ) {
    char from[20], ucs[20];
    size_t fromlen, tolen;
    ICONV_CONST char *fpt;
    char *upt;
    int i, j, low;
    int esc_len = strlen(escape_sequence);

    strcpy(from,escape_sequence);

    enc->has_2byte = false;
    low = -1;
    for ( i=0; i<256; ++i ) if ( i!=escape_sequence[0] ) {
	for ( j=0; j<256; ++j ) {
	    from[esc_len] = i; from[esc_len+1] = j; from[esc_len+2] = 0;
	    fromlen = esc_len+2;
	    fpt = from;
	    upt = ucs;
	    tolen = sizeof(ucs);
	    if ( iconv( enc->tounicode , &fpt, &fromlen, &upt, &tolen )!= (size_t) (-1) &&
		    upt-ucs==sizeof(unichar_t) /* Exactly one character */ ) {
		if ( low==-1 ) {
		    enc->low_page = low = i;
		    enc->has_2byte = true;
		}
		enc->high_page = i;
	break;
	    }
	}
    }
    if ( enc->low_page==enc->high_page )
	enc->has_2byte = false;
    if ( enc->has_2byte ) {
	strcpy(enc->iso_2022_escape, escape_sequence);
	enc->iso_2022_escape_len = esc_len;
    }
return( enc->has_2byte );
}

Encoding *_FindOrMakeEncoding(const char *name,int make_it) {
    Encoding *enc;
    char buffer[20];
    const char *iconv_name;
    Encoding temp;
    uint8 good[256];
    int i, j, any, all;
    char from[8], ucs[20];
    size_t fromlen, tolen;
    ICONV_CONST char *fpt;
    char *upt;
    /* iconv is not case sensitive */

    if ( strncasecmp(name,"iso8859_",8)==0 || strncasecmp(name,"koi8_",5)==0 ) {
	    /* Fixup for old naming conventions */
	    strncpy(buffer,name,sizeof(buffer));
        buffer[sizeof(buffer)-1] = '\0';
	    *strchr(buffer,'_') = '-';
	    name = buffer;
    } else if ( strcasecmp(name,"iso-8859")==0 ) {
	    /* Fixup for old naming conventions */
	    strncpy(buffer,name,3);
	    strncpy(buffer+3,name+4,sizeof(buffer)-3);
        buffer[sizeof(buffer)-1] = '\0';
	    name = buffer;
    } else if ( strcasecmp(name,"isolatin10")==0 || strcasecmp(name,"latin10")==0 ) {
        name = "iso8859-16";	/* try 10 before trying 1 */
    } else if ( strcasecmp(name,"isolatin1")==0 ) {
        name = "iso8859-1";
    } else if ( strcasecmp(name,"isocyrillic")==0 ) {
        name = "iso8859-5";
    } else if ( strcasecmp(name,"isoarabic")==0 ) {
        name = "iso8859-6";
    } else if ( strcasecmp(name,"isogreek")==0 ) {
        name = "iso8859-7";
    } else if ( strcasecmp(name,"isohebrew")==0 ) {
        name = "iso8859-8";
    } else if ( strcasecmp(name,"isothai")==0 ) {
	name = "tis-620";	/* TIS doesn't define non-breaking space in 0xA0 */
    } else if ( strcasecmp(name,"latin0")==0 || strcasecmp(name,"latin9")==0 ) {
	name = "iso8859-15";	/* "latin-9" is supported (libiconv bug?) */
    } else if ( strcasecmp(name,"koi8r")==0 ) {
        name = "koi8-r";
    } else if ( strncasecmp(name,"jis201",6)==0 || strncasecmp(name,"jisx0201",8)==0 ) {
        name = "jis_x0201";
    } else if ( strcasecmp(name,"AdobeStandardEncoding")==0 || strcasecmp(name,"Adobe")==0 )
	name = "AdobeStandard";
    for ( enc=enclist; enc!=NULL; enc=enc->next )
	if ( strmatch(name,enc->enc_name)==0 ||
		(enc->iconv_name!=NULL && strmatch(name,enc->iconv_name)==0))
return( enc );
    if ( strmatch(name,"unicode")==0 || strmatch(name,"iso10646")==0 || strmatch(name,"iso10646-1")==0 )
return( &unicodebmp );
    if ( strmatch(name,"unicode4")==0 || strmatch(name,"ucs4")==0 )
return( &unicodefull );

    iconv_name = name;
    /* Mac seems to work ok */
    if ( strcasecmp(name,"win")==0 || strcasecmp(name,"ansi")==0 )
	iconv_name = "MS-ANSI";		/* "WINDOWS-1252";*/
    else if ( strncasecmp(name,"jis208",6)==0 || strncasecmp(name,"jisx0208",8)==0 )
	iconv_name = "ISO-2022-JP";
    else if ( strncasecmp(name,"jis212",6)==0 || strncasecmp(name,"jisx0212",8)==0 )
	iconv_name = "ISO-2022-JP-2";
    else if ( strncasecmp(name,"ksc5601",7)==0 )
	iconv_name = "ISO-2022-KR";
    else if ( strcasecmp(name,"gb2312pk")==0 || strcasecmp(name,"gb2312packed")==0 )
	iconv_name = "EUC-CN";
    else if ( strncasecmp(name,"gb2312",6)==0 )
	iconv_name = "ISO-2022-CN";
    else if ( strcasecmp(name,"wansung")==0 )
	iconv_name = "EUC-KR";
    else if ( strcasecmp(name,"EUC-CN")==0 ) {
	iconv_name = name;
	name = "gb2312pk";
    } else if ( strcasecmp(name,"EUC-KR")==0 ) {
	iconv_name = name;
	name = "wansung";
    }

/* Escape sequences:					*/
/*	ISO-2022-CN:     \e $ ) A ^N			*/
/*	ISO-2022-KR:     \e $ ) C ^N			*/
/*	ISO-2022-JP:     \e $ B				*/
/*	ISO-2022-JP-2:   \e $ ( D			*/
/*	ISO-2022-JP-3:   \e $ ( O			*/ /* Capital "O", not zero */
/*	ISO-2022-CN-EXT: \e $ ) E ^N			*/ /* Not sure about this, also uses CN escape */

    memset(&temp,0,sizeof(temp));
    temp.builtin = true;
    temp.tounicode = iconv_open(FindUnicharName(),iconv_name);
    if ( temp.tounicode==(iconv_t) -1 || temp.tounicode==NULL )
return( NULL );			/* Iconv doesn't recognize this name */
    temp.fromunicode = iconv_open(iconv_name,FindUnicharName());
    if ( temp.fromunicode==(iconv_t) -1 || temp.fromunicode==NULL ) {
	/* This should never happen, but if it does... */
	iconv_close(temp.tounicode);
return( NULL );
    }

    memset(good,0,sizeof(good));
    any = false; all = true;
    for ( i=1; i<256; ++i ) {
	from[0] = i; from[1] = 0;
	fromlen = 1;
	fpt = from;
	upt = ucs;
	tolen = sizeof(ucs);
	if ( iconv( temp.tounicode , &fpt, &fromlen, &upt, &tolen )!= (size_t) (-1)) {
	    good[i] = true;
	    any = true;
	} else
	    all = false;
    }
    if ( any )
	temp.has_1byte = true;
    if ( all )
	temp.only_1byte = true;

    if ( !all ) {
	if ( strstr(iconv_name,"2022")==NULL ) {
	    for ( i=temp.has_1byte; i<256; ++i ) if ( !good[i] ) {
		for ( j=0; j<256; ++j ) {
		    from[0] = i; from[1] = j; from[2] = 0;
		    fromlen = 2;
		    fpt = from;
		    upt = ucs;
		    tolen = sizeof(ucs);
		    if ( iconv( temp.tounicode , &fpt, &fromlen, &upt, &tolen )!= (size_t) (-1) &&
			    upt-ucs==sizeof(unichar_t) /* Exactly one character */ ) {
			if ( temp.low_page==-1 )
			    temp.low_page = i;
			temp.high_page = i;
			temp.has_2byte = true;
		break;
		    }
		}
	    }
	    if ( temp.low_page==temp.high_page ) {
		temp.has_2byte = false;
		temp.low_page = temp.high_page = -1;
	    }
	}
	if ( !temp.has_2byte && !good[033]/* escape */ ) {
	    if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP3")!=NULL &&
		    TryEscape( &temp,"\33$(O" )) {
		;
	    } else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP2")!=NULL &&
		    TryEscape( &temp,"\33$(D" )) {
		;
	    } else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP")!=NULL &&
		    TryEscape( &temp,"\33$B" )) {
		;
	    } else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"KR")!=NULL &&
		    TryEscape( &temp,"\33$)C\16" )) {
		;
	    } else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"CN")!=NULL &&
		    TryEscape( &temp,"\33$)A\16" )) {
		;
	    }
	}
    }
    if ( !temp.has_1byte && !temp.has_2byte )
return( NULL );
    if ( !make_it )
return( NULL );

    enc = chunkalloc(sizeof(Encoding));
    *enc = temp;
    enc->enc_name = copy(name);
    if ( iconv_name!=name )
	enc->iconv_name = copy(iconv_name);
    enc->next = enclist;
    enc->builtin = true;
    enclist = enc;
    if ( enc->has_2byte )
	enc->char_cnt = (enc->high_page<<8) + 256;
    else {
	enc->char_cnt = 256;
	enc->only_1byte = true;
    }
    if ( strstrmatch(iconv_name,"JP")!=NULL ||
	    strstrmatch(iconv_name,"sjis")!=NULL ||
	    strstrmatch(iconv_name,"cp932")!=NULL )
	enc->is_japanese = true;
    else if ( strstrmatch(iconv_name,"KR")!=NULL )
	enc->is_korean = true;
    else if ( strstrmatch(iconv_name,"CN")!=NULL )
	enc->is_simplechinese = true;
    else if ( strstrmatch(iconv_name,"BIG")!=NULL && strstrmatch(iconv_name,"5")!=NULL )
	enc->is_tradchinese = true;

    if ( strstrmatch(name,"ISO8859")!=NULL &&
	    strtol(name+strlen(name)-2,NULL,10)>=16 )
	/* Not in our menu, don't hide */;
    else if ( iconv_name!=name || strmatch(name,"mac")==0 || strstrmatch(name,"ISO8859")!=NULL ||
	    strmatch(name,"koi8-r")==0 || strmatch(name,"sjis")==0 ||
	    strmatch(name,"big5")==0 || strmatch(name,"big5hkscs")==0 )
	enc->hidden = true;

return( enc );
}

Encoding *FindOrMakeEncoding(const char *name) {
return( _FindOrMakeEncoding(name,true));
}

/* Plugin API */
int AddEncoding(char *name,EncFunc enc_to_uni,EncFunc uni_to_enc,int max) {
    Encoding *enc;
    int i;

    for ( enc=enclist; enc!=NULL; enc=enc->next ) {
	if ( strmatch(name,enc->enc_name)==0 ||
		(enc->iconv_name!=NULL && strmatch(name,enc->iconv_name)==0)) {
	    if ( enc->tounicode_func==NULL )
return( 0 );			/* Failure */
	    else {
		enc->tounicode_func   = enc_to_uni;
		enc->fromunicode_func = uni_to_enc;
		enc->char_cnt	      = max;
return( 2 );
	    }
	}
    }

    if ( strmatch(name,"unicode")==0 || strmatch(name,"iso10646")==0 || strmatch(name,"iso10646-1")==0 )
return( 0 );			/* Failure */
    if ( strmatch(name,"unicode4")==0 || strmatch(name,"ucs4")==0 )
return( 0 );			/* Failure */

    enc = chunkalloc(sizeof(Encoding));
    enc->enc_name = copy(name);
    enc->next = enclist;
    enclist = enc;
    enc->tounicode_func   = enc_to_uni;
    enc->fromunicode_func = uni_to_enc;
    enc->char_cnt	      = max;
    for ( i=0; i<256 && i<max; ++i )
	if ( enc_to_uni(i)!=-1 )
    break;

    if ( i<256 && i<max )
	enc->has_1byte = true;
    if ( max<256 )
	enc->only_1byte = true;
    else
	enc->has_2byte = true;
return( 1 );
}

static char *getPfaEditEncodings(void) {
    static char *encfile=NULL;
    char buffer[1025];
    char *ffdir;

    if ( encfile!=NULL )
        return encfile;
    ffdir = getFontForgeUserDir(Config);
    if ( ffdir==NULL )
        return NULL;
    sprintf(buffer,"%s/Encodings.ps", ffdir);
    free(ffdir);
    encfile = copy(buffer);
    return encfile;
}

void EncodingFree(Encoding *item) {
    int i;

    if ( item==NULL )
	return;

    free(item->enc_name);
    if ( item->psnames!=NULL ) {
	for ( i=0; i<item->char_cnt; ++i )
	    free(item->psnames[i]);
	free(item->psnames);
    }
    free(item->unicode);
    free(item);
}

void DeleteEncoding(Encoding *me) {
    FontViewBase *fv;
    Encoding *prev;

    if ( me->builtin )
return;

    for ( fv = FontViewFirst(); fv!=NULL; fv = fv->next ) {
	if ( fv->map->enc==me )
	    fv->map->enc = &custom;
    }
    if ( me==enclist )
	enclist = me->next;
    else {
	for ( prev = enclist; prev!=NULL && prev->next!=me; prev=prev->next );
	if ( prev!=NULL ) prev->next = me->next;
    }
    EncodingFree(me);
    if ( default_encoding == me )
	default_encoding = FindOrMakeEncoding("ISO8859-1");
    if ( default_encoding == NULL )
	default_encoding = &custom;
    DumpPfaEditEncodings();
}

/* Parse a TXT file from the unicode consortium */
    /* Unicode Consortium Format A */
    /* List of lines with several fields, */
	/* first is the encoding value (in hex), second the Unicode value (in hex) */
    /* # is a comment character (to eol) */
static Encoding *ParseConsortiumEncodingFile(FILE *file) {
    char buffer[200];
    int32 encs[0x10000];
    int enc, unienc, max;
    Encoding *item;

    memset(encs, 0, sizeof(encs));
    max = -1;

    while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	if ( ishexdigit(buffer[0]) ) {
	    if ( sscanf(buffer, "%x %x", (unsigned *) &enc, (unsigned *) &unienc)==2 &&
		    enc<0x10000 && enc>=0 ) {
		encs[enc] = unienc;
		if ( enc>max ) max = enc;
	    }
	}
    }

    if ( max==-1 )
return( NULL );

    ++max;
    if ( max<256 ) max = 256;
    item = calloc(1,sizeof(Encoding));
    item->only_1byte = item->has_1byte = true;
    item->char_cnt = max;
    item->unicode = malloc(max*sizeof(int32));
    memcpy(item->unicode,encs,max*sizeof(int32));
return( item );
}

/**
 *  \brief Parses a GlyphOrderAndAliasDB encoding file
 *
 *  \param [in] file The file handle to the encoding file
 *  \return The encoding, or NULL if none could be made
 *
 *  \details Each line in the file contains one glyph name (equating to one slot).
 *           It is assumed that values are tab separated. There
 *           are at least two columns, with a third column being optional.
 *           The first column is the glyph name to be used on output. This
 *           is what will normally be used to determine the unicode value for
 *           this slot.
 *           The second column is the 'friendly' name - this is what will
 *           be used for the slot's name.
 *           The third, optional column specifies the unicode value that the
 *           slot should map to. When available, this will be used in preference
 *           to the first column.
 */
static Encoding *ParseGlyphOrderAndAliasDB(FILE *file) {
    GArray *enc_arr = g_array_sized_new(FALSE, TRUE, sizeof(int32), 256);
    GArray *names_arr = g_array_sized_new(FALSE, TRUE, sizeof(char *), 256);
    Encoding *item = NULL;
    char buffer[BUFSIZ];
    int enc, any = FALSE, has_1byte = FALSE;

    while (fgets(buffer, sizeof(buffer), file)) {
        char *split = strchr(buffer, '\t'), *split2, *enc_name = NULL;
        if (split == NULL) {
            // Skip entries that do not contain at least two (tab separated) values
            LogError(_("ParseGlyphOrderAndAliasDB: Invalid (non-tab separated entry) at index %d: %s\n"), enc_arr->len, g_strstrip(buffer));
            //enc = -1;
            //g_array_append_val(enc_arr, enc);
            //g_array_append_val(names_arr, enc_name);
            continue;
        }
        *split++ = '\0';
        // Buffer now contains the first column
        g_strstrip(buffer);
        // split contains the second, and possibly the third column
        g_strstrip(split);

        // Optional third column
        split2 = strchr(split, '\t');
        if (split2 != NULL) {
            *split2++ = '\0';
            g_strstrip(split2);
        }

        // Use the third column in preference to the first
        enc = UniFromName((split2 != NULL) ? split2 : buffer, ui_none, &custom);
        if (split[0] != '\0') {
            /* Used not to do this, but there are several legal names */
            /*  for some slots and people get unhappy (rightly) if we */
            /*  use the wrong one */
            enc_name = copy(split);
            any = TRUE;
        }

        if (enc != -1 && enc_arr->len < 256) {
            // We have a valid encoding within the first 256 entries
            has_1byte = TRUE;
        }

        // Append entry to our arrays
        g_array_append_val(enc_arr, enc);
        g_array_append_val(names_arr, enc_name);
    }

    if (enc_arr->len > 0) {
        // If we have mappings, we make an encoding.
        char *tmp_name = ff_ask_string(_("Encoding name"), "GlyphOrderAndAliasDB", _("Please name this encoding"));
        if (tmp_name != NULL) {
            if (tmp_name[0] == '\0') {
                // Encodings must be named.
                free(tmp_name);
                tmp_name = NULL;
            } else {
                item = calloc(1, sizeof(Encoding));
                item->enc_name = tmp_name;
                // We pad the map in accordance with existing code in FindOrMakeEncoding and elsewhere.
                // Nobody knows why.
                item->char_cnt = (enc_arr->len < 256) ? 256 : enc_arr->len;
                item->unicode = malloc(item->char_cnt * sizeof(int32));
                memcpy(item->unicode, enc_arr->data, enc_arr->len * sizeof(int32));
                if (item->char_cnt > enc_arr->len) {
                    // Pad the unfilled entries with -1
                    memset(item->unicode + enc_arr->len, -1, sizeof(int32) * (enc_arr->len - item->char_cnt));
                }
                if (any) {
                    item->psnames = calloc(item->char_cnt, sizeof(char *));
                    memcpy(item->psnames, names_arr->data, names_arr->len * sizeof(char *));
                }

                item->is_custom = TRUE;
                item->has_1byte = has_1byte;
                if (enc_arr->len < 256) {
                    item->only_1byte = TRUE;
                } else {
                    item->has_2byte = TRUE;
                }
            }
        }
    }

    g_array_free(enc_arr, TRUE);
    g_array_free(names_arr, TRUE);
    return item;
}

void RemoveMultiples(Encoding *item) {
    Encoding *test;

    for ( test=enclist; test!=NULL; test = test->next ) {
	if ( strcmp(test->enc_name,item->enc_name)==0 )
    break;
    }
    if ( test!=NULL )
	DeleteEncoding(test);
}

char *ParseEncodingFile(char *filename, char *encodingname) {
    FILE *file;
    char *orig = filename;
    Encoding *head, *item, *prev, *next;
    char *buf, *name;
    int i,ch;

    if ( filename==NULL ) filename = getPfaEditEncodings();
    file = fopen(filename,"r");
    if ( file==NULL ) {
	if ( orig!=NULL )
	    ff_post_error(_("Couldn't open file"), _("Couldn't open file %.200s"), orig);
return( NULL );
    }
    ch = getc(file);
    if ( ch==EOF ) {
	fclose(file);
return( NULL );
    }

    /* Here we detect file format and decide which format
       parsing routine to actually use.
    */
    ungetc(ch, file);


    if(strlen(filename) >= 20
       && !strcmp(filename + strlen(filename) - 20, "GlyphOrderAndAliasDB")){
        head = ParseGlyphOrderAndAliasDB(file);
    } else if (ch=='#' || ch=='0') {
        head = ParseConsortiumEncodingFile(file);
        if(encodingname)
            head->enc_name = copy(encodingname);
    } else {
        head = PSSlurpEncodings(file);
    }
    fclose(file);
    if ( head==NULL ) {
	ff_post_error(_("Bad encoding file format"),_("Bad encoding file format") );
	return( NULL );
    }

    for ( i=0, prev=NULL, item=head; item!=NULL; prev = item, item=next, ++i ) {
	next = item->next;
	if ( item->enc_name==NULL ) {
	    if ( no_windowing_ui ) {
		ff_post_error(_("Bad encoding file format"),_("This file contains an unnamed encoding, which cannot be named in a script"));
                EncodingFree(head);
		return( NULL );
	    }
	    if ( item==head && item->next==NULL )
		buf = strdup(_( "Please name this encoding" ));
	    else
		buf = xasprintf(_( "Please name encoding %d in this file" ), i );

	    name = ff_ask_string( buf, NULL, buf );

	    if ( name!=NULL ) {
		item->enc_name = copy(name);
		free(name);
	    } else {
		if ( prev==NULL )
		    head = item->next;
		else
		    prev->next = item->next;
		EncodingFree(item);
	    }
	}
    }
    for ( item=head; item!=NULL; item=item->next )
	RemoveMultiples(item);

    if ( enclist == NULL )
	enclist = head;
    else {
	for ( item=enclist; item->next!=NULL; item=item->next );
	item->next = head;
    }
return( copy( head->enc_name ) );
}

void LoadPfaEditEncodings(void) {
    free(ParseEncodingFile(NULL, NULL));
}

void DumpPfaEditEncodings(void) {
    FILE *file;
    Encoding *item;
    int i;
    char buffer[80];

    for ( item=enclist; item!=NULL && item->builtin; item=item->next );
    if ( item==NULL ) {
	unlink(getPfaEditEncodings());
return;
    }

    file = fopen( getPfaEditEncodings(), "w");
    if ( file==NULL ) {
	LogError( _("couldn't write encodings file\n") );
return;
    }

    for ( item=enclist; item!=NULL; item = item->next ) if ( !item->builtin && item->tounicode_func==NULL ) {
	fprintf( file, "/%s [\n", item->enc_name );
	if ( item->psnames==NULL )
	    fprintf( file, "%% Use codepoints.\n" );
	for ( i=0; i<item->char_cnt; ++i ) {
	    if ( item->psnames!=NULL && item->psnames[i]!=NULL )
		fprintf( file, " /%s", item->psnames[i]);
	    else if ( item->unicode[i]<' ' || (item->unicode[i]>=0x7f && item->unicode[i]<0xa0))
		fprintf( file, " /.notdef" );
	    else
		fprintf( file, " /%s", StdGlyphName(buffer,item->unicode[i],ui_none,(NameList *) -1));
	    if ( (i&0xf)==0 )
		fprintf( file, "\t\t%% 0x%02x\n", i );
	    else
		putc('\n',file);
	}
	fprintf( file, "] def\n\n" );
    }
    fclose(file);
}

/* ************************************************************************** */
/* ****************************** CID Encodings ***************************** */
/* ************************************************************************** */
struct cidmap *cidmaps = NULL;

int CIDFromName(char *name,SplineFont *cidmaster) {
    /* We've had various conventions for encoding a cid inside a name */
    /* I'm primarily interested in this when the name is something like */
    /*  Japan1.504.vert */
    /* which tells me that the current glyph is the rotated version of */
    /*  cid 504 */
    /* Other convention "cid-504.vert" */
    int len = strlen( cidmaster->ordering );
    int cid;
    char *end;

    if ( strncmp(name,cidmaster->ordering,len)==0 ) {
	if ( name[len]=='.' ) ++len;
    } else if ( strncmp(name,"cid-",4)==0 ) {
	len = 4;
    } else
	len = 0;
    cid = strtol(name+len,&end,10);
    if ( end==name+len )
return( -1 );
    if ( *end!='.' && *end!='\0' )
return( -1 );

return ( cid );
}

int CID2Uni(struct cidmap *map,int cid) {
    unsigned int uni;

    if ( map==NULL )
return( -1 );
    else if ( cid==0 )
return( 0 );
    else if ( cid<map->namemax && map->unicode[cid]!=0 )
return( map->unicode[cid] );
    else if ( cid<map->namemax && map->name[cid]!=NULL ) {
	if ( sscanf(map->name[cid],"uni%x", &uni )==1 )
return( uni );
    }

return( -1 );
}

int CID2NameUni(struct cidmap *map,int cid, char *buffer, int len) {
    int enc = -1;
    const char *temp;

    if ( map==NULL )
	snprintf(buffer,len,"cid-%d", cid);
    else if ( cid<map->namemax && map->name[cid]!=NULL ) {
	strncpy(buffer,map->name[cid],len);
	buffer[len-1] = '\0';
    } else if ( cid==0 )
	strcpy(buffer,".notdef");
    else if ( cid<map->namemax && map->unicode[cid]!=0 ) {
	if ( map->unicode==NULL || map->namemax==0 )
	    enc = 0;
	else
	    enc = map->unicode[cid];
	temp = StdGlyphName(buffer,enc,ui_none,(NameList *) -1);
	if ( temp!=buffer )
	    strcpy(buffer,temp);
    } else
	snprintf(buffer,len,"%s.%d", map->ordering, cid);
return( enc );
}

int NameUni2CID(struct cidmap *map, int uni, const char *name) {
    int i;
    struct cidaltuni *alts;

    if ( map==NULL )
		return( -1 );
    if ( uni!=-1 ) {
		// Search for a matching code.
		for ( i=0; i<map->namemax; ++i )
		    if ( map->unicode[i]==(uint32)uni )
				return( i );
		for ( alts=map->alts; alts!=NULL; alts=alts->next )
		    if ( alts->uni==uni )
				return( alts->cid );
    } else {
	// Search for a matching name.
	if ( name!=NULL )
	    for ( i=0; i<map->namemax; ++i )
		if ( map->name[i]!=NULL && strcmp(map->name[i],name)==0 )
		    return( i );
    }
	return( -1 );
}

struct altuni *CIDSetAltUnis(struct cidmap *map,int cid) {
    /* Some CIDs are mapped to several unicode code points, damn it */
    struct altuni *sofar = NULL, *alt;
    struct cidaltuni *alts;

    for ( alts=map->alts; alts!=NULL; alts=alts->next ) {
	if ( alts->cid==cid ) {
	    alt = chunkalloc(sizeof(struct altuni));
	    alt->next = sofar;
	    sofar = alt;
	    alt->unienc = alts->uni;
	    alt->vs = -1;
	}
    }
return( sofar );
}

int MaxCID(struct cidmap *map) {
return( map->cidmax );
}

static char *SearchDirForCidMap(const char *dir,char *registry,char *ordering,
	int supplement,char **maybefile) {
    char maybe[FILENAME_MAX+1];
    struct dirent *ent;
    DIR *d;
    int len, rlen = strlen(registry), olen=strlen(ordering);
    char *pt, *end, *ret;
    int test, best = -1;

    if ( dir==NULL )
return( NULL );

    if ( *maybefile!=NULL ) {
	char *pt = strrchr(*maybefile,'.');
	while ( pt>*maybefile && isdigit(pt[-1]))
	    --pt;
	best = strtol(pt,NULL,10);
    }

    d = opendir(dir);
    if ( d==NULL )
return( NULL );
    while ( (ent = readdir(d))!=NULL ) {
	if ( (len = strlen(ent->d_name))<8 )
    continue;
	if ( strcmp(ent->d_name+len-7,".cidmap")!=0 )
    continue;
	if ( strncmp(ent->d_name,registry,rlen)!=0 || ent->d_name[rlen]!='-' )
    continue;
	pt = ent->d_name+rlen+1;
	if ( strncmp(pt,ordering,olen)!=0 || pt[olen]!='-' )
    continue;
	pt += olen+1;
	if ( !isdigit(*pt))
    continue;
	test = strtol(pt,&end,10);
	if ( *end!='.' )
    continue;
	if ( test>=supplement ) {
	    ret = malloc(strlen(dir)+1+len+1);
	    strcpy(ret,dir);
	    strcat(ret,"/");
	    strcat(ret,ent->d_name);
	    closedir(d);
return( ret );
	} else if ( test>best ) {
	    best = test;
	    strcpy(maybe,ent->d_name);
	}
    }
    closedir(d);
    if ( best>-1 ) {
	ret = malloc(strlen(dir)+1+strlen(maybe)+1);
	strcpy(ret,dir);
	strcat(ret,"/");
	strcat(ret,maybe);
	*maybefile = ret;
    }
return( NULL );
}

static struct cidmap *MakeDummyMap(char *registry,char *ordering,int supplement) {
    struct cidmap *ret = malloc(sizeof(struct cidmap));

    ret->registry = copy(registry);
    ret->ordering = copy(ordering);
    ret->supplement = ret->maxsupple = supplement;
    ret->cidmax = ret->namemax = 0;
    ret->unicode = NULL; ret->name = NULL;
    ret->alts = NULL;
    ret->next = cidmaps;
    cidmaps = ret;
return( ret );
}

struct cidmap *LoadMapFromFile(char *file,char *registry,char *ordering,
	int supplement) {
    struct cidmap *ret = malloc(sizeof(struct cidmap));
    char *pt = strrchr(file,'.');
    FILE *f;
    int cid1, cid2, uni, cnt, i, ch;
    char name[100];

    while ( pt>file && isdigit(pt[-1]))
	--pt;
    ret->supplement = ret->maxsupple = strtol(pt,NULL,10);
    if ( supplement>ret->maxsupple )
	ret->maxsupple = supplement;
    ret->registry = copy(registry);
    ret->ordering = copy(ordering);
    ret->alts = NULL;
    ret->cidmax = ret->namemax = 0;
    ret->unicode = NULL; ret->name = NULL;
    ret->next = cidmaps;
    cidmaps = ret;

    f = fopen( file,"r" );
    if ( f==NULL ) {
	ff_post_error(_("Missing cidmap file"),_("Couldn't open cidmap file: %s"), file );
    } else if ( fscanf( f, "%d %d", &ret->cidmax, &ret->namemax )!=2 ) {
	ff_post_error(_("Bad cidmap file"),_("%s is not a cidmap file, please download\nhttp://fontforge.sourceforge.net/cidmaps.tgz"), file );
	fprintf( stderr, _("%s is not a cidmap file, please download\nhttp://fontforge.sourceforge.net/cidmaps.tgz"), file );
	fclose(f);
    } else {
	ret->unicode = calloc(ret->namemax+1,sizeof(uint32));
	ret->name = calloc(ret->namemax+1,sizeof(char *));
	while ( 1 ) {
	    cnt=fscanf( f, "%d..%d %x", &cid1, &cid2, (unsigned *) &uni );
	    if ( cnt<=0 )
	break;
	    if ( cid1>ret->namemax )
	continue;
	    if ( cnt==3 ) {
		if ( cid2>ret->namemax ) cid2 = ret->namemax;
		for ( i=cid1; i<=cid2; ++i )
		    ret->unicode[i] = uni++;
	    } else if ( cnt==1 ) {
		if ( fscanf(f,"%x", (unsigned *) &uni )==1 ) {
		    ret->unicode[cid1] = uni;
		    ch = getc(f);
		    while ( ch==',' ) {
			if ( fscanf(f,"%x", (unsigned *) &uni )==1 ) {
			    struct cidaltuni *alt = chunkalloc(sizeof(struct cidaltuni));
			    alt->next = ret->alts;
			    ret->alts = alt;
			    alt->uni = uni;
			    alt->cid = cid1;
			}
			ch = getc(f);
		    }
		    ungetc(ch,f);
		} else if ( fscanf(f," /%s", name )==1 )
		    ret->name[cid1] = copy(name);
	    }
	}
	fclose(f);
    }
return( ret );
}

struct cidmap *FindCidMap(char *registry,char *ordering,int supplement,SplineFont *sf) {
    struct cidmap *map, *maybe=NULL;
    char *file;
    char *maybefile=NULL;
    int maybe_sup = -1;
    const char *buts[3], *buts2[3], *buts3[3];
    char *buf = NULL;
    int ret;

    if ( sf!=NULL && sf->cidmaster ) sf = sf->cidmaster;
    if ( sf!=NULL && sf->loading_cid_map )
return( NULL );

    for ( map = cidmaps; map!=NULL; map = map->next ) {
	if ( strcmp(map->registry,registry)==0 && strcmp(map->ordering,ordering)==0 ) {
	    if ( supplement<=map->supplement )
return( map );
	    else if ( maybe==NULL || maybe->supplement<map->supplement )
		maybe = map;
	}
    }
    if ( maybe!=NULL && supplement<=maybe->maxsupple )
return( maybe );	/* User has said it's ok to use maybe at this supplement level */

    file = SearchDirForCidMap(".",registry,ordering,supplement,&maybefile);
    if ( file==NULL )
	file = SearchDirForCidMap(getFontForgeShareDir(),registry,ordering,supplement,&maybefile);

    if ( file==NULL && (maybe!=NULL || maybefile!=NULL)) {
	if ( maybefile!=NULL ) {
	    char *pt = strrchr(maybefile,'.');
	    while ( pt>maybefile && isdigit(pt[-1]))
		--pt;
	    maybe_sup = strtol(pt,NULL,10);
	    if ( maybe!=NULL && maybe->supplement >= maybe_sup ) {
		free(maybefile); maybefile = NULL;
		maybe_sup = maybe->supplement;
	    } else
		maybe = NULL;
	}
	if ( maybe!=NULL )
	    maybe_sup = maybe->supplement;
	if ( sf!=NULL ) sf->loading_cid_map = true;
	buts[0] = _("_Use It"); buts[1] = _("_Search"); buts[2] = NULL;
	ret = ff_ask(_("Use CID Map"),(const char **) buts,0,1,_("This font is based on the charset %1$.20s-%2$.20s-%3$d, but the best I've been able to find is %1$.20s-%2$.20s-%4$d.\nShall I use that or let you search?"),
		registry,ordering,supplement,maybe_sup);
	if ( sf!=NULL ) sf->loading_cid_map = false;
	if ( ret==0 ) {
	    if ( maybe!=NULL ) {
		maybe->maxsupple = supplement;
return( maybe );
	    } else {
		file = maybefile;
		maybefile = NULL;
	    }
	}
    }

    if ( file==NULL ) {
	char *uret;
	buf = xasprintf( "%s-%s-*.cidmap", registry, ordering );
	if ( maybe==NULL && maybefile==NULL ) {
	    buts3[0] = _("_Browse"); buts3[1] = _("_Give Up"); buts3[2] = NULL;
	    ret = ff_ask(_("No cidmap file..."),(const char **)buts3,0,1,_("FontForge was unable to find a cidmap file for this font. It is not essential to have one, but some things will work better if you do. If you have not done so you might want to download the cidmaps from:\n   http://FontForge.sourceforge.net/cidmaps.tgz\nand then gunzip and untar them and move them to:\n  %.80s\n\nWould you like to search your local disk for an appropriate file?"),
		    getFontForgeShareDir()==NULL?"/usr/share/fontforge":getFontForgeShareDir()
		    );
	    if ( ret==1 || no_windowing_ui )
		buf = NULL;
	}
	uret = NULL;
	if ( ( buf != NULL ) && !no_windowing_ui ) {
	    if ( sf!=NULL ) sf->loading_cid_map = true;
	    uret = ff_open_filename(_("Find a cidmap file..."), NULL, (char *) buf );
	    if ( sf!=NULL ) sf->loading_cid_map = false;
	}
	if ( uret==NULL ) {
	    buts2[0] = "_Use It"; buts2[1] = "_Search"; buts2[2] = NULL;
	    if ( maybe==NULL && maybefile==NULL )
		/* No luck */;
	    else if ( no_windowing_ui && maybe!=NULL ) {
		maybe->maxsupple = supplement;
return( maybe );
	    } else if ( no_windowing_ui ) {
		file = maybefile;
		maybefile = NULL;
	    } else if ( ff_ask(_("Use CID Map"),(const char **)buts2,0,1,_("Are you sure you don't want to use the cidmap I found?"))==0 ) {
		if ( maybe!=NULL ) {
		    maybe->maxsupple = supplement;
return( maybe );
		} else {
		    file = maybefile;
		    maybefile = NULL;
		}
	    }
	} else {
	    file = utf82def_copy(uret);
	    free(uret);
	}
    }

    free(maybefile);
    if ( file!=NULL ) {
	map = LoadMapFromFile(file,registry,ordering,supplement);
	free(file);
return( map );
    }

return( MakeDummyMap(registry,ordering,supplement));
}

static void SFApplyOrdering(SplineFont *sf, int glyphcnt) {
    SplineChar **glyphs, *sc;
    int i;
    RefChar *refs, *rnext, *rprev;
    SplineSet *new, *spl;

    /* Remove references to characters which aren't in the new map (if any) */
    /* Don't need to fix up dependencies, because we throw the char away */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	for ( rprev = NULL, refs=sc->layers[ly_fore].refs; refs!=NULL; refs=rnext ) {
	    rnext = refs->next;
	    if ( refs->sc->orig_pos==-1 ) {
		new = refs->layers[0].splines;
		if ( new!=NULL ) {
		    for ( spl = new; spl->next!=NULL; spl = spl->next );
		    spl->next = sc->layers[ly_fore].splines;
		    sc->layers[ly_fore].splines = new;
		}
		refs->layers[0].splines=NULL;
		RefCharFree(refs);
		if ( rprev==NULL )
		    sc->layers[ly_fore].refs = rnext;
		else
		    rprev->next = rnext;
	    } else
		rprev = refs;
	}
    }

    glyphs = calloc(glyphcnt+1,sizeof(SplineChar *));
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	if ( sc->orig_pos==-1 )
	    SplineCharFree(sc);
	else
	    glyphs[sc->orig_pos] = sc;
    }

    free(sf->glyphs);
    sf->glyphcnt = sf->glyphmax = glyphcnt;
    sf->glyphs = glyphs;
}

/* Convert a normal font to a cid font, rearranging glyphs into cid order */
void SFEncodeToMap(SplineFont *sf,struct cidmap *map) {
    SplineChar *sc;
    int i,max=0, anyextras=0;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc = sf->glyphs[i]) ) {
	sc->orig_pos = NameUni2CID(map,sc->unicodeenc,sc->name);
	if ( sc->orig_pos>max ) max = sc->orig_pos;
	else if ( sc->orig_pos==-1 ) ++anyextras;
    } else if ( sc!=NULL )
	sc->orig_pos = -1;

    if ( anyextras ) {
	char *buttons[3];
	buttons[0] = _("_Delete"); buttons[1] = _("_Add"); buttons[2] = NULL;
	if ( ff_ask(_("Extraneous glyphs"),(const char **) buttons,0,1,_("The current encoding contains glyphs which I cannot map to CIDs.\nShould I delete them or add them to the end (where they may conflict with future ros definitions)?"))==1 ) {
	    if ( map!=NULL && max<map->cidmax ) max = map->cidmax;
	    anyextras = 0;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc = sf->glyphs[i]) ) {
		if ( sc->orig_pos == -1 ) sc->orig_pos = max + anyextras++;
	    }
	    max += anyextras;
	}
    }
    SFApplyOrdering(sf, max+1);
}

enum cmaptype { cmt_out=-1, cmt_coderange, cmt_notdefs, cmt_cid, cmt_max };
struct coderange { uint32 first, last, cid; };
struct cmap {
    struct {
	int n;
	struct coderange *ranges;
    } groups[cmt_max];
    char *registry;
    char *ordering;
    int supplement;
    struct remap *remap;
    int total;
};

static void cmapfree(struct cmap *cmap) {
    free(cmap->registry);
    free(cmap->ordering);
    free(cmap->groups[cmt_coderange].ranges);
    free(cmap->groups[cmt_notdefs].ranges);
    free(cmap->groups[cmt_cid].ranges);
    free(cmap->remap);
    free(cmap);
}

static struct coderange *ExtendArray(struct coderange *ranges,int *n, int val) {
    if ( *n == 0 )
	ranges = calloc(val,sizeof(struct coderange));
    else {
	ranges = realloc(ranges,(*n+val)*sizeof(struct coderange));
	memset(ranges+*n,0,val*sizeof(struct coderange));
    }
    *n += val;
return( ranges );
}

static char *readpsstr(char *str) {
    char *eos;

    while ( isspace(*str)) ++str;
    if ( *str=='(' ) ++str;
    /* PostScript strings can be more complicated than this (hex, nested parens, Enc85...) */
    /*  but none of those should show up here */
    for ( eos = str; *eos!=')' && *eos!='\0'; ++eos );
return( copyn(str,eos-str));
}

static struct cmap *ParseCMap(char *filename) {
    char buf2[200];
    FILE *file;
    struct cmap *cmap;
    char *end, *pt;
    int val, pos;
    enum cmaptype in;
    int in_is_single; // We set this if we are to parse cidchars into cidranges.
    static const char *bcsr = "begincodespacerange", *bndr = "beginnotdefrange", *bcr = "begincidrange", *bcc = "begincidchar";
    static const char *reg = "/Registry", *ord = "/Ordering", *sup="/Supplement";

    file = fopen(filename,"r");
    if ( file==NULL )
return( NULL );

    cmap = calloc(1,sizeof(struct cmap));
    in = cmt_out;
    while ( fgets(buf2,sizeof(buf2),file)!=NULL ) {
	for ( pt=buf2; isspace(*pt); ++pt);
	if ( in==cmt_out ) {
	    if ( *pt=='/' ) {
		if ( strncmp(pt,reg,strlen(reg))==0 )
		    cmap->registry = readpsstr(pt+strlen(reg));
		else if ( strncmp(pt,ord,strlen(ord))==0 )
		    cmap->ordering = readpsstr(pt+strlen(ord));
		else if ( strncmp(pt,sup,strlen(sup))==0 ) {
		    for ( pt += strlen(sup); isspace(*pt); ++pt );
		    cmap->supplement = strtol(pt,NULL,10);
		}
    continue;
	    } else if ( !isdigit(*pt) )
    continue;
	    val = strtol(pt,&end,10);
	    while ( isspace(*end)) ++end;
	    in_is_single = 0;
	    if ( strncmp(end,bcsr,strlen(bcsr))==0 ) {
		in = cmt_coderange;
	    } else if ( strncmp(end,bndr,strlen(bndr))==0 ) {
		in = cmt_notdefs;
	    } else if ( strncmp(end,bcr,strlen(bcr))==0 ) {
		in = cmt_cid;
	    } else if ( strncmp(end,bcc,strlen(bcc))==0 ) {
		in = cmt_cid;
		in_is_single = 1;
	    }
	    if ( in!=cmt_out ) {
		pos = cmap->groups[in].n;
		cmap->groups[in].ranges = ExtendArray(cmap->groups[in].ranges,&cmap->groups[in].n,val);
	    }
	} else if ( strncmp(pt,"end",3)== 0 )
	    in = cmt_out;
    else if (pos >= cmap->groups[in].n) {
        LogError(_("cidmap entry out of bounds: %s"), buf2);
    }
	else {
	    // Read the first bracketed code.
	    if ( *pt!='<' )
	continue;
	    cmap->groups[in].ranges[pos].first = strtoul(pt+1,&end,16);
	    if ( *end=='>' ) ++end;
	    while ( isspace(*end)) ++end;
	    if (in_is_single) {
	      cmap->groups[in].ranges[pos].last = cmap->groups[in].ranges[pos].first;
	    } else {
	      // Read the second bracketed code.
	      if ( *end=='<' ) ++end;
	      cmap->groups[in].ranges[pos].last = strtoul(end,&end,16);
	      if ( *end=='>' ) ++end;
	    }
	    if ( in!=cmt_coderange ) {
		while ( isspace(*end)) ++end;
	        // Read the unbracketed argument.
		cmap->groups[in].ranges[pos].cid = strtol(end,&end,10);
	    }
	    ++pos;
	}
    }
    fclose(file);
return( cmap );
}

static void CompressCMap(struct cmap *cmap) {
    int32 i,j,k, pos, base;
    uint32 min, oldmax;
    /* we can't really deal with three and four byte encodings */
    /*  so if we get one arrange for the sf itself to do a remap */

    cmap->total = 0x10000;
    for ( i=0; i<cmap->groups[cmt_coderange].n; ++i )
	if ( cmap->groups[cmt_coderange].ranges[i].last>0xfffff )
    break;
    if ( i==cmap->groups[cmt_coderange].n )	/* No need to remap */
return;

    cmap->remap = calloc(cmap->groups[cmt_coderange].n+1,sizeof(struct remap));
    base = 0;
    for ( i=0; i<cmap->groups[cmt_coderange].n; ++i )
	if ( cmap->groups[cmt_coderange].ranges[i].last<0xffff ) {
	    base = 0x10000;
    break;
	}

    pos=0;
    oldmax = base==0?0:0xffff;
    for ( i=0; i<cmap->groups[cmt_coderange].n; ++i ) {
	min = 0xffffffff; k=-1;
	for ( j=0; j<cmap->groups[cmt_coderange].n; ++j )
	    if ( cmap->groups[cmt_coderange].ranges[j].first>oldmax &&
		    cmap->groups[cmt_coderange].ranges[j].first<min ) {
		min = cmap->groups[cmt_coderange].ranges[j].first;
		k = j;
	    }
	if ( k==-1 )
    break;
	cmap->remap[pos].firstenc = cmap->groups[cmt_coderange].ranges[k].first&~0xff;
	cmap->remap[pos].lastenc = cmap->groups[cmt_coderange].ranges[k].last|0xff;
	cmap->remap[pos].infont = base;
	base += cmap->remap[pos].lastenc-cmap->remap[pos].firstenc+1;
	oldmax = cmap->remap[pos].lastenc;
	++pos;
    }
    cmap->remap[pos].infont = -1;	/* Marks end */
    cmap->total = base;
    /* so cmap->remap will map sf indeces into the encoding in the cmap */

    /* And now we want to change the groups[cmt_cid].ranges so that they will */
    /*  map into sf indeces rather than into the encoding of the cmap */
    for ( i=0; i<cmap->groups[cmt_cid].n; ++i ) {
	for ( k=0; cmap->remap[k].infont!=-1; ++k )
	    if ( cmap->groups[cmt_cid].ranges[i].first>=cmap->remap[k].firstenc &&
		    cmap->groups[cmt_cid].ranges[i].first<=cmap->remap[k].lastenc )
	break;
	if ( cmap->remap[k].infont==-1 )
    continue;
	cmap->groups[cmt_cid].ranges[i].first += cmap->remap[k].infont-cmap->remap[k].firstenc;
	cmap->groups[cmt_cid].ranges[i].last += cmap->remap[k].infont-cmap->remap[k].firstenc;
    }
}

SplineFont *CIDFlatten(SplineFont *cidmaster,SplineChar **glyphs,int charcnt) {
    FontViewBase *fvs;
    SplineFont *new;
    char buffer[20];
    BDFFont *bdf;
    int j;

    if ( cidmaster==NULL )
return(NULL);
    new = SplineFontEmpty();
    new->fontname = copy(cidmaster->fontname);
    new->fullname = copy(cidmaster->fullname);
    new->familyname = copy(cidmaster->familyname);
    new->weight = copy(cidmaster->weight);
    new->copyright = copy(cidmaster->copyright);
    sprintf(buffer,"%g", (double)cidmaster->cidversion);
    new->version = copy(buffer);
    new->italicangle = cidmaster->italicangle;
    new->upos = cidmaster->upos;
    new->uwidth = cidmaster->uwidth;
    new->ascent = cidmaster->ascent;
    new->descent = cidmaster->descent;
    new->changed = new->changed_since_autosave = true;
    new->display_antialias = cidmaster->display_antialias;
    new->hasvmetrics = cidmaster->hasvmetrics;
    new->fv = cidmaster->fv;
    /* Don't copy the grid splines, there won't be anything meaningfull at top level */
    /*  and won't know which font to copy from below */
    new->bitmaps = cidmaster->bitmaps;		/* should already be flattened */
    cidmaster->bitmaps = NULL;			/* don't free 'em */
    for ( bdf=new->bitmaps; bdf!=NULL; bdf=bdf->next )
	bdf->sf = new;
    new->gpos_lookups = cidmaster->gpos_lookups;
    cidmaster->gpos_lookups = NULL;
    new->gsub_lookups = cidmaster->gsub_lookups;
    cidmaster->gsub_lookups = NULL;
    new->kerns = cidmaster->kerns; new->vkerns = cidmaster->vkerns;
    cidmaster->kerns = cidmaster->vkerns = NULL;
    new->names = cidmaster->names; cidmaster->names = NULL;
    new->horiz_base = cidmaster->horiz_base; cidmaster->horiz_base = NULL;
    new->vert_base = cidmaster->vert_base; cidmaster->vert_base = NULL;
    new->pfminfo = cidmaster->pfminfo;
    new->texdata = cidmaster->texdata;
    new->possub = cidmaster->possub; cidmaster->possub = NULL;
    new->sm = cidmaster->sm; cidmaster->sm = NULL;
    new->features = cidmaster->features; cidmaster->features = NULL;
    new->macstyle = cidmaster->macstyle;
    new->origname = copy( cidmaster->origname );
    new->display_size = cidmaster->display_size;
    /* Don't copy private */
    new->xuid = copy(cidmaster->xuid);
    new->glyphs = glyphs;
    new->glyphcnt = new->glyphmax = charcnt;
    for ( j=0; j<charcnt; ++j ) if ( glyphs[j]!=NULL ) {
	glyphs[j]->parent = new;
	glyphs[j]->orig_pos = j;
    }
    for ( fvs=new->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	fvs->cidmaster = NULL;
	if ( fvs->sf->glyphcnt!=new->glyphcnt ) {
	    free(fvs->selected);
	    fvs->selected = calloc(new->glyphcnt,sizeof(char));
	    if ( fvs->map->encmax < new->glyphcnt )
		fvs->map->map = realloc(fvs->map->map,(fvs->map->encmax = new->glyphcnt)*sizeof(int32));
	    fvs->map->enccount = new->glyphcnt;
	    if ( fvs->map->backmax < new->glyphcnt )
		fvs->map->backmap = realloc(fvs->map->backmap,(fvs->map->backmax = new->glyphcnt)*sizeof(int32));
	    for ( j=0; j<new->glyphcnt; ++j )
		fvs->map->map[j] = fvs->map->backmap[j] = j;
	}
	fvs->sf = new;
	FVSetTitle(fvs);
    }
    FontViewReformatAll(new);
    SplineFontFree(cidmaster);
return( new );
}

void SFFlatten(SplineFont **cidmasterpp) {
    SplineChar **glyphs;
    SplineFont *cidmaster = *cidmasterpp;
    int i,j,max;

    if ( cidmaster==NULL )
	return;

    if ( cidmaster->cidmaster!=NULL ) {
	cidmaster = cidmaster->cidmaster;
	cidmasterpp = &cidmaster;
    }
    /* This doesn't change the ordering, so no need for special tricks to */
    /*  preserve scrolling location. */
    for ( i=max=0; i<cidmaster->subfontcnt; ++i ) {
	if ( max<cidmaster->subfonts[i]->glyphcnt )
	    max = cidmaster->subfonts[i]->glyphcnt;
    }
    glyphs = calloc(max,sizeof(SplineChar *));
    for ( j=0; j<max; ++j ) {
	for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	    if ( j<cidmaster->subfonts[i]->glyphcnt && cidmaster->subfonts[i]->glyphs[j]!=NULL ) {
		glyphs[j] = cidmaster->subfonts[i]->glyphs[j];
		cidmaster->subfonts[i]->glyphs[j] = NULL;
	break;
	    }
	}
    }
    *cidmasterpp = CIDFlatten(cidmaster,glyphs,max);
}

int SFFlattenByCMap(SplineFont **sfpp,char *cmapname) {
    struct cmap *cmap;
    int i,j,k,l,m, extras, max, curmax, warned;
    int found[4];
    SplineChar **glyphs = NULL, *sc;
    SplineFont *sf = *sfpp;
    FontViewBase *fvs;

    if ( sf->cidmaster!=NULL ) {
	sf = sf->cidmaster;
	sfpp = &sf;
    }
    if ( sf->subfontcnt==0 ) {
	ff_post_error(_("Not a CID-keyed font"),_("Not a CID-keyed font"));
return( false );
    }
    if ( cmapname==NULL )
return( false );
    cmap = ParseCMap(cmapname);
    if ( cmap==NULL )
return( false );
    CompressCMap(cmap);
    max = 0;
    for ( i=0; i<cmap->groups[cmt_cid].n; ++i ) {
	if ( max<cmap->groups[cmt_cid].ranges[i].last )
	    max = cmap->groups[cmt_cid].ranges[i].last;
	if ( cmap->groups[cmt_cid].ranges[i].last>0x100000 ) {
	    ff_post_error(_("Encoding Too Large"),_("Encoding Too Large"));
	    cmapfree(cmap);
return( false );
	}
    }

    curmax = 0;
    for ( k=0; k<sf->subfontcnt; ++k ) {
	if ( curmax < sf->subfonts[k]->glyphcnt )
	    curmax = sf->subfonts[k]->glyphcnt;
    }

    glyphs = calloc(curmax,sizeof(SplineChar *));
    for ( i=0; i<curmax; ++i ) {
	for ( k=0; k<sf->subfontcnt; ++k )
	    if ( i<sf->subfonts[k]->glyphcnt && sf->subfonts[k]->glyphs[i]!=NULL ) {
		glyphs[i] = sf->subfonts[k]->glyphs[i];
		sf->subfonts[k]->glyphs[i] = NULL;
	break;
	    }
    }
    *sfpp = sf = CIDFlatten(sf,glyphs,curmax);

    warned = false;
    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	EncMap *map = fvs->map;
	for ( j=0; j<2; ++j ) {
	    extras = 0;
	    for ( i=0; i<curmax; ++i ) {
		sc = glyphs[i];
		if ( sc!=NULL ) {
		    m = 0;
		    for ( l=0; l<cmap->groups[cmt_cid].n; ++l ) {
			if ( i>=cmap->groups[cmt_cid].ranges[l].cid &&
				i<=cmap->groups[cmt_cid].ranges[l].cid +
				   cmap->groups[cmt_cid].ranges[l].last -
				    cmap->groups[cmt_cid].ranges[l].first ) {
			    if ( m<sizeof(found)/sizeof(found[0]) )
				found[m++] = l;
			    else if ( !warned ) {
				ff_post_notice(_("MultipleEncodingIgnored"),
					_("The glyph at CID %d is mapped to more than %d encodings. Only the first %d are handled."), i,
					sizeof(found)/sizeof(found[0]),
					sizeof(found)/sizeof(found[0]));
				warned = true;
			    }
			}
		    }
		    if ( m==0 ) {
			if ( j ) {
			    map->map[max+extras] = sc->orig_pos;
			    map->backmap[sc->orig_pos] = max+extras;
			}
			++extras;
		    } else {
			if ( j ) {
			    int p = cmap->groups[cmt_cid].ranges[found[0]].first +
				    i-cmap->groups[cmt_cid].ranges[found[0]].cid;
			    map->map[p] = sc->orig_pos;
			    map->backmap[sc->orig_pos] = p;
			    for ( l=1; l<m; ++l ) {
				int pos = cmap->groups[cmt_cid].ranges[found[l]].first +
					i-cmap->groups[cmt_cid].ranges[found[l]].cid;
				map->map[pos] = sc->orig_pos;
			    }
			}
		    }
		}
	    }
	    if ( !j ) {
		map->map = realloc(map->map,(map->encmax = map->enccount = max+extras)*sizeof(int32));
		memset(map->map,-1,map->enccount*sizeof(int32));
		memset(map->backmap,-1,sf->glyphcnt*sizeof(int32));
		fvs->selected = realloc(fvs->selected, map->enccount*sizeof(char));
		if (map->enccount > sf->glyphcnt) {
		    memset(fvs->selected+sf->glyphcnt, 0, map->enccount-sf->glyphcnt);
		}
		map->remap = cmap->remap; cmap->remap = NULL;
	    }
	    warned = true;
	}
    }
    cmapfree(cmap);
    FontViewReformatAll(sf);
return( true );
}

static int Enc2CMap(struct cmap *cmap,int enc) {
    int i;

    for ( i=0; i<cmap->groups[cmt_cid].n; ++i )
	if ( enc>=cmap->groups[cmt_cid].ranges[i].first &&
		enc<=cmap->groups[cmt_cid].ranges[i].last )
return( enc-cmap->groups[cmt_cid].ranges[i].first+
	    cmap->groups[cmt_cid].ranges[i].cid );

return( -1 );
}

static void SFEncodeToCMap(SplineFont *cidmaster,SplineFont *sf,EncMap *oldmap, struct cmap *cmap) {
    SplineChar *sc, *GID0=NULL;
    int i,max=0, anyextras=0;

    cidmaster->cidregistry = cmap->registry; cmap->registry = NULL;
    cidmaster->ordering = cmap->ordering; cmap->ordering = NULL;
    cidmaster->supplement = cmap->supplement;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
	if ( strcmp(sc->name,".notdef")==0 )
	    sc->orig_pos = 0;
	else if ( oldmap->backmap[i]==-1 )
	    sc->orig_pos = -1;
	else {
	    sc->orig_pos = Enc2CMap(cmap,oldmap->backmap[i]);
	    if ( sc->orig_pos==0 ) {
		if ( GID0==NULL )
		    GID0 = sc;
		else
		    sc->orig_pos = -1;
	    }
	}
	if ( sc->orig_pos>max ) max = sc->orig_pos;
	else if ( sc->orig_pos==-1 ) ++anyextras;
    }
    if ( GID0!=NULL )
	GID0->orig_pos = ++max;

    if ( anyextras ) {
	char *buttons[3];
	buttons[0] = _("_Delete");
	buttons[1] = _("_Add");
	buttons[2] = NULL;
	if ( ff_ask(_("Extraneous glyphs"),(const char **) buttons,0,1,_("The current encoding contains glyphs which I cannot map to CIDs.\nShould I delete them or add them to the end (where they may conflict with future ros definitions)?"))==1 ) {
	    if ( cmap!=NULL && max<cmap->total ) max = cmap->total;
	    anyextras = 0;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
		if ( sc->orig_pos == -1 ) sc->orig_pos = max + anyextras++;
	    }
	    max += anyextras;
	}
    }
    SFApplyOrdering(sf, max+1);
}

/* If we change the ascent/descent of a sub font then consider changing the */
/*  as/ds of the master font. I used to think this irrelevant, but as the */
/*  typoAscent/Descent is based on the master's ascent/descent it actually */
/*  is meaningful. Set the master to the subfont with the most glyphs */
void CIDMasterAsDes(SplineFont *sf) {
    SplineFont *cidmaster = sf->cidmaster;
    SplineFont *best;
    int i, cid, cnt, bcnt;

    if ( cidmaster==NULL )
return;
    best = NULL; bcnt = 0;
    for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	sf = cidmaster->subfonts[i];
	for ( cid=cnt=0; cid<sf->glyphcnt; ++cid )
	    if ( sf->glyphs[cid]!=NULL )
		++cnt;
	if ( cnt>bcnt ) {
	    best = sf;
	    bcnt = cnt;
	}
    }
    if ( best==NULL && cidmaster->subfontcnt>0 )
	best = cidmaster->subfonts[0];
    if ( best!=NULL ) {
	double ratio = 1000.0/(best->ascent+best->descent);
	int ascent = rint(best->ascent*ratio);
	if ( cidmaster->ascent!=ascent || cidmaster->descent!=1000-ascent ) {
	    cidmaster->ascent = ascent;
	    cidmaster->descent = 1000-ascent;
	}
    }
}

SplineFont *MakeCIDMaster(SplineFont *sf,EncMap *oldmap,int bycmap,char *cmapfilename, struct cidmap *cidmap) {
    SplineFont *cidmaster;
    struct cidmap *map;
    struct cmap *cmap;
    FontViewBase *fvs;

    cidmaster = SplineFontEmpty();
    if ( bycmap ) {
	if ( cmapfilename==NULL ) {
	    SplineFontFree(cidmaster);
return(NULL);
	}
	cmap = ParseCMap(cmapfilename);
	if ( cmap==NULL ) {
	    SplineFontFree(cidmaster);
return(NULL);
	}
	CompressCMap(cmap);
	SFEncodeToCMap(cidmaster,sf,oldmap,cmap);
	cmapfree(cmap);
    } else {
	map = cidmap;
	if ( map==NULL ) {
	    SplineFontFree(cidmaster);
return(NULL);
	}
	cidmaster->cidregistry = copy(map->registry);
	cidmaster->ordering = copy(map->ordering);
	cidmaster->supplement = map->supplement;
	SFEncodeToMap(sf,map);
    }
    if ( sf->uni_interp!=ui_none && sf->uni_interp!=ui_unset )
	cidmaster->uni_interp = sf->uni_interp;
    else if ( strstrmatch(cidmaster->ordering,"japan")!=NULL )
	cidmaster->uni_interp = ui_japanese;
    else if ( strstrmatch(cidmaster->ordering,"CNS")!=NULL )
	cidmaster->uni_interp = ui_trad_chinese;
    else if ( strstrmatch(cidmaster->ordering,"GB")!=NULL )
	cidmaster->uni_interp = ui_simp_chinese;
    else if ( strstrmatch(cidmaster->ordering,"Korea")!=NULL )
	cidmaster->uni_interp = ui_korean;
    sf->uni_interp = cidmaster->uni_interp;
    cidmaster->fontname = copy(sf->fontname);
    cidmaster->fullname = copy(sf->fullname);
    cidmaster->familyname = copy(sf->familyname);
    cidmaster->weight = copy(sf->weight);
    cidmaster->copyright = copy(sf->copyright);
    cidmaster->cidversion = 1.0;
    cidmaster->display_antialias = sf->display_antialias;
    cidmaster->display_size = sf->display_size;
    cidmaster->ascent = sf->ascent /*880*/;
    cidmaster->descent = sf->descent /*120*/;
    cidmaster->changed = cidmaster->changed_since_autosave = true;
    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame )
	fvs->cidmaster = cidmaster;
    cidmaster->fv = sf->fv;
    sf->cidmaster = cidmaster;
    cidmaster->subfontcnt = 1;
    cidmaster->subfonts = calloc(2,sizeof(SplineFont *));
    cidmaster->subfonts[0] = sf;
    cidmaster->gpos_lookups = sf->gpos_lookups; sf->gpos_lookups = NULL;
    cidmaster->gsub_lookups = sf->gsub_lookups; sf->gsub_lookups = NULL;
    cidmaster->horiz_base = sf->horiz_base; sf->horiz_base = NULL;
    cidmaster->vert_base = sf->vert_base; sf->vert_base = NULL;
    cidmaster->possub = sf->possub; sf->possub = NULL;
    cidmaster->kerns = sf->kerns; sf->kerns = NULL;
    cidmaster->vkerns = sf->vkerns; sf->vkerns = NULL;
    if ( sf->private==NULL )
	sf->private = calloc(1,sizeof(struct psdict));
    if ( !PSDictHasEntry(sf->private,"lenIV"))
	PSDictChangeEntry(sf->private,"lenIV","1");		/* It's 4 by default, in CIDs the convention seems to be 1 */
    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	free(fvs->selected);
	fvs->selected = calloc(fvs->sf->glyphcnt,sizeof(char));
	EncMapFree(fvs->map);
	fvs->map = EncMap1to1(fvs->sf->glyphcnt);
	FVSetTitle(fvs);
    }
    CIDMasterAsDes(sf);
    FontViewReformatAll(sf);
return( cidmaster );
}

int CountOfEncoding(Encoding *encoding_name) {
return( encoding_name->char_cnt );
}

char *SFEncodingName(SplineFont *sf,EncMap *map) {
    char buffer[130];

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    if ( sf->subfontcnt!=0 ) {
	sprintf( buffer, "%.50s-%.50s-%d", sf->cidregistry, sf->ordering, sf->supplement );
return( copy( buffer ));
    }
return( copy( map->enc->enc_name ));
}

/* ************************** Reencoding  routines ************************** */

void BDFOrigFixup(BDFFont *bdf,int orig_cnt,SplineFont *sf) {
    BDFChar **glyphs;
    int i;

    if ( bdf->glyphmax>=orig_cnt ) {
	if ( bdf->glyphcnt<orig_cnt ) {
	    for ( i=bdf->glyphcnt; i<orig_cnt; ++i )
		bdf->glyphs[i] = NULL;
	    bdf->glyphcnt = orig_cnt;
	}
return;
    }

    glyphs = calloc(orig_cnt,sizeof(BDFChar *));
    for ( i=0; i<bdf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	glyphs[sf->glyphs[i]->orig_pos] = bdf->glyphs[i];
	if ( bdf->glyphs[i]!=NULL )	/* Not all glyphs exist in a piecemeal font */
	    bdf->glyphs[i]->orig_pos = sf->glyphs[i]->orig_pos;
    }
    free(bdf->glyphs);
    bdf->glyphs = glyphs;
    bdf->glyphcnt = bdf->glyphmax = orig_cnt;
    bdf->ticked = true;
}

static int _SFForceEncoding(SplineFont *sf,EncMap *old, Encoding *new_enc) {
    int enc_cnt,i;
    BDFFont *bdf;
    FontViewBase *fvs;

    /* Normally we base our encoding process on unicode code points. */
    /*  but encodings like AdobeStandard are more interested in names than */
    /*  code points. It is perfectly possible to have a font properly */
    /*  encoded by code point which is not properly encoded by name */
    /*  (might have f_i where it should have fi). So even if it's got */
    /*  the right encoding, we still may want to force the names */
    if ( new_enc->is_custom )
return(false);			/* Custom, it's whatever's there */

    if ( new_enc->is_original ) {
	SplineChar **glyphs;
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	    sf->glyphs[i]->orig_pos = -1;
	for ( i=enc_cnt=0; i<old->enccount; ++i )
	    if ( old->map[i]!=-1 && sf->glyphs[old->map[i]]!=NULL &&
		    sf->glyphs[old->map[i]]->orig_pos == -1 )
		sf->glyphs[old->map[i]]->orig_pos = enc_cnt++;
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	    if ( sf->glyphs[i]->orig_pos==-1 )
		sf->glyphs[i]->orig_pos = enc_cnt++;
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    struct splinecharlist *scl;
	    int layer;
	    RefChar *ref;

	    for ( scl=sf->glyphs[i]->dependents; scl!=NULL; scl=scl->next ) {
		for ( layer=0; layer<scl->sc->layer_cnt; ++layer )
		    for ( ref = scl->sc->layers[layer].refs; ref!=NULL; ref=ref->next )
			ref->orig_pos = ref->sc->orig_pos;
	    }
	}
	for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	    fvs->map->ticked = false;
	    /*if ( fvs->filled!=NULL ) fvs->filled->ticked = false;*/
	}
	for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) if ( !fvs->map->ticked ) {
	    EncMap *map = fvs->map;
	    for ( i=0; i<map->enccount; ++i ) if ( map->map[i]!=-1 )
		map->map[i] = sf->glyphs[map->map[i]]->orig_pos;
	    if ( enc_cnt>map->backmax ) {
		free(map->backmap);
		map->backmax = enc_cnt;
		map->backmap = malloc(enc_cnt*sizeof(int32));
	    }
	    memset(map->backmap,-1,enc_cnt*sizeof(int32));
	    for ( i=0; i<map->enccount; ++i ) if ( map->map[i]!=-1 )
		if ( map->backmap[map->map[i]]==-1 )
		    map->backmap[map->map[i]] = i;
	    map->ticked = true;
	}
	if ( !old->ticked )
	    IError( "Unticked encmap" );
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	    BDFOrigFixup(bdf,enc_cnt,sf);
	for ( fvs=sf->fv; fvs!=NULL ; fvs=fvs->nextsame )
	    FVBiggerGlyphCache(fvs,enc_cnt);
	glyphs = calloc(enc_cnt,sizeof(SplineChar *));
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	    glyphs[sf->glyphs[i]->orig_pos] = sf->glyphs[i];
	free(sf->glyphs);
	sf->glyphs = glyphs;
	sf->glyphcnt = sf->glyphmax = enc_cnt;
return( true );
    }

    enc_cnt = new_enc->char_cnt;

    if ( old->enccount<enc_cnt ) {
	if ( old->encmax<enc_cnt ) {
	    old->map = realloc(old->map,enc_cnt*sizeof(int32));
	    old->encmax = enc_cnt;
	}
	memset(old->map+old->enccount,-1,(enc_cnt-old->enccount)*sizeof(int32));
	old->enccount = enc_cnt;
    }
    old->enc = new_enc;
    for ( i=0; i<old->enccount && i<enc_cnt; ++i ) if ( old->map[i]!=-1 && sf->glyphs[old->map[i]]!=NULL ) {
	SplineChar dummy;
	int j = old->map[i];
	SCBuildDummy(&dummy,sf,old,i);
	sf->glyphs[j]->unicodeenc = dummy.unicodeenc;
	free(sf->glyphs[j]->name);
	sf->glyphs[j]->name = copy(dummy.name);
    }
    /* We just changed the unicode values for most glyphs */
    /* but any references to them will have the old values, and that's bad, so fix 'em up */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	struct splinecharlist *scl;
	int layer;
	RefChar *ref;

	for ( scl=sf->glyphs[i]->dependents; scl!=NULL; scl=scl->next ) {
	    for ( layer=0; layer<scl->sc->layer_cnt; ++layer )
		for ( ref = scl->sc->layers[layer].refs; ref!=NULL; ref=ref->next )
		    ref->unicode_enc = ref->sc->unicodeenc;
	}
    }
return( true );
}

int SFForceEncoding(SplineFont *sf,EncMap *old, Encoding *new_enc) {
    if ( sf->mm!=NULL ) {
	MMSet *mm = sf->mm;
	int i;
	for ( i=0; i<mm->instance_count; ++i )
	    _SFForceEncoding(mm->instances[i],old,new_enc);
	_SFForceEncoding(mm->normal,old,new_enc);
    } else
return( _SFForceEncoding(sf,old,new_enc));

return( true );
}

EncMap *EncMapFromEncoding(SplineFont *sf,Encoding *enc) {
    int i,j, extras, found, base, unmax;
    int32 *encoded, *unencoded;
    EncMap *map;
    struct altuni *altuni;
    SplineChar *sc;

    if ( enc==NULL )
return( NULL );

    base = enc->char_cnt;
    if ( enc->is_original )
	base = 0;
    else if ( enc->char_cnt<=256 )
	base = 256;
    else if ( enc->char_cnt<=0x10000 )
	base = 0x10000;
    encoded = malloc(base*sizeof(int32));
    memset(encoded,-1,base*sizeof(int32));
    unencoded = malloc(sf->glyphcnt*sizeof(int32));
    unmax = sf->glyphcnt;

    for ( i=extras=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	found = false;
	if ( enc->psnames!=NULL ) {
	    for ( j=enc->char_cnt-1; j>=0; --j ) {
		if ( enc->psnames[j]!=NULL &&
			strcmp(enc->psnames[j],sc->name)==0 ) {
		    found = true;
		    encoded[j] = i;
		}
	    }
	}
	if ( !found ) {
	    if ( sc->unicodeenc!=-1 &&
                 sc->unicodeenc < (int)unicode4_size &&
		     (j = EncFromUni(sc->unicodeenc,enc))!= -1 )
		encoded[j] = i;
	    else {
		/* I don't think extras can surpass unmax now, but it doesn't */
		/*  hurt to leave the code (it's from when we encoded duplicates see below) */
		if ( extras>=unmax ) unencoded = realloc(unencoded,(unmax+=300)*sizeof(int32));
		unencoded[extras++] = i;
	    }
	    for ( altuni=sc->altuni; altuni!=NULL; altuni=altuni->next ) {
		if ( altuni->unienc!=-1 &&
                     (uint32)altuni->unienc<unicode4_size &&
			 altuni->vs==-1 &&
			 altuni->fid==0 &&
			 (j = EncFromUni(altuni->unienc,enc))!= -1 )
		    encoded[j] = i;
		/* I used to have code here to add these unencoded duplicates */
		/*  but I don't really see any reason to do so. The main unicode */
		/*  will occur, and any encoded duplicates so the glyph won't */
		/*  vanish */
	    }
	}
    }

    /* Some glyphs have both a pua encoding and an encoding in a non-bmp */
    /*  plane. Big5HK does and the AMS glyphs do */
    if ( enc->is_unicodefull && (sf->uni_interp == ui_trad_chinese ||
				 sf->uni_interp == ui_ams )) {
	const int *pua = sf->uni_interp == ui_ams? amspua : cns14pua;
	for ( i=0xe000; i<0xf8ff; ++i ) {
	    if ( pua[i-0xe000]!=0 )
		encoded[pua[i-0xe000]] = encoded[i];
	}
    }

    if ( enc->psnames != NULL ) {
	/* Names are more important than unicode code points for some encodings */
	/*  AdobeStandard for instance which won't work if you have a glyph  */
	/*  named "f_i" (must be "fi") even though the code point is correct */
	/* The code above would match f_i where AS requires fi, so force the */
	/*  names to be correct. */
	for ( j=0; j<enc->char_cnt; ++j ) {
	    if ( encoded[j]!=-1 && enc->psnames[j]!=NULL &&
		    strcmp(sf->glyphs[encoded[j]]->name,enc->psnames[j])!=0 ) {
		free(sf->glyphs[encoded[j]]->name);
		sf->glyphs[encoded[j]]->name = copy(enc->psnames[j]);
	    }
	}
    }

    map = chunkalloc(sizeof(EncMap));
    map->enccount = map->encmax = base + extras;
    map->map = malloc(map->enccount*sizeof(int32));
    memcpy(map->map,encoded,base*sizeof(int32));
    memcpy(map->map+base,unencoded,extras*sizeof(int32));
    map->backmax = sf->glyphcnt;
    map->backmap = malloc(sf->glyphcnt*sizeof(int32));
    memset(map->backmap,-1,sf->glyphcnt*sizeof(int32));	/* Just in case there are some unencoded glyphs (duplicates perhaps) */
    for ( i = map->enccount-1; i>=0; --i ) if ( map->map[i]!=-1 )
	map->backmap[map->map[i]] = i;
    map->enc = enc;

    free(encoded);
    free(unencoded);

return( map );
}

EncMap *CompactEncMap(EncMap *map, SplineFont *sf) {
    int i, inuse, gid;
    int32 *newmap;

    for ( i=inuse=0; i<map->enccount ; ++i )
	if ( (gid = map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid]))
	    ++inuse;
    newmap = malloc(inuse*sizeof(int32));
    for ( i=inuse=0; i<map->enccount ; ++i )
	if ( (gid = map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid]))
	    newmap[inuse++] = gid;
    free(map->map);
    map->map = newmap;
    map->enccount = inuse;
    map->encmax = inuse;
    map->enc = &custom;
    memset(map->backmap,-1,sf->glyphcnt*sizeof(int32));
    for ( i=inuse-1; i>=0; --i )
	if ( (gid=map->map[i])!=-1 )
	    map->backmap[gid] = i;
return( map );
}

static void BCProtectUndoes( Undoes *undo,BDFChar *bc ) {
    BDFRefChar *brhead, *brprev=NULL, *brnext;

    for ( ; undo!=NULL; undo=undo->next ) {
	switch ( undo->undotype ) {
	  case ut_bitmap:
	    for ( brhead=undo->u.bmpstate.refs; brhead != NULL; brhead=brnext ) {
		brnext = brhead->next;
		if ( brhead->bdfc == bc ) {
		    BCPasteInto( &undo->u.bmpstate,bc,brhead->xoff,brhead->yoff,false,false );
		    if ( brprev == NULL )
			undo->u.bmpstate.refs = brnext;
		    else
			brprev->next = brnext;
		    free( brhead );
		} else
		    brprev = brhead;
	    }
	  break;
	  case ut_multiple:
	    BCProtectUndoes( undo->u.multiple.mult,bc );
	  break;
	  case ut_composit:
	    BCProtectUndoes( undo->u.composit.bitmaps,bc );
	  break;
	}
    }
}

int SFReencode(SplineFont *sf, const char *encname, int force) {
    Encoding *new_enc;
    FontViewBase *fv = sf->fv;

    if ( strmatch(encname,"compacted")==0 ) {
	fv->normal = EncMapCopy(fv->map);
	CompactEncMap(fv->map,sf);
    } else {
	new_enc = FindOrMakeEncoding(encname);
	if ( new_enc==NULL )
return -1;
	if ( force )
	    SFForceEncoding(sf,fv->map,new_enc);
	else if ( new_enc==&custom )
	    fv->map->enc = &custom;
	else {
	    EncMap *map = EncMapFromEncoding(sf,new_enc);
	    EncMapFree(fv->map);
	    if (fv->sf != NULL && fv->map == fv->sf->map) { fv->sf->map = map; }
	    fv->map = map;
	    if ( !no_windowing_ui )
		FVSetTitle(fv);
	}
	if ( fv->normal!=NULL ) {
	    EncMapFree(fv->normal);
	    if (fv->sf != NULL && fv->map == fv->sf->map) { fv->sf->map = NULL; }
	    fv->normal = NULL;
	}
	SFReplaceEncodingBDFProps(sf,fv->map);
    }
    free(fv->selected);
    fv->selected = calloc(fv->map->enccount,sizeof(char));
    if ( !no_windowing_ui )
	FontViewReformatAll(sf);

return 0;
}

void SFRemoveGlyph( SplineFont *sf,SplineChar *sc ) {
    struct splinecharlist *dep, *dnext;
    struct bdfcharlist *bdep, *bdnext;
    RefChar *rf, *refs, *rnext;
    BDFRefChar *bref, *brnext, *brprev;
    KernPair *kp, *kprev;
    int i;
    BDFFont *bdf;
    BDFChar *bfc, *dbc;
    int layer;

    if ( sc==NULL )
return;

    /* Close any open windows */
    SCCloseAllViews(sc);

    /* Turn any references to this glyph into inline copies of it */
    for ( dep=sc->dependents; dep!=NULL; dep=dnext ) {
	SplineChar *dsc = dep->sc;
	dnext = dep->next;
	/* May be more than one reference to us, colon has two refs to period */
	/*  but only one dlist entry */
	for ( layer=0; layer<dsc->layer_cnt; ++layer ) {
	    for ( rf = dsc->layers[layer].refs; rf!=NULL; rf=rnext ) {
		rnext = rf->next;
		if ( rf->sc==sc )
		    SCRefToSplines(dsc,rf,layer);
	    }
	}
    }

    for ( layer=0; layer<sc->layer_cnt; ++layer ) {
	for ( refs=sc->layers[layer].refs; refs!=NULL; refs = rnext ) {
	    rnext = refs->next;
	    SCRemoveDependent(sc,refs,layer);
	}
    }

    /* Remove any kerning pairs that look at this character */
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	for ( kprev=NULL, kp=sf->glyphs[i]->kerns; kp!=NULL; kprev = kp, kp=kp->next ) {
	    if ( kp->sc==sc ) {
		if ( kprev==NULL )
		    sf->glyphs[i]->kerns = kp->next;
		else
		    kprev->next = kp->next;
		kp->next = NULL;
		KernPairsFree(kp);
	break;
	    }
	}
    }

    sf->glyphs[sc->orig_pos] = NULL;

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	if ( sc->orig_pos<bdf->glyphcnt && (bfc = bdf->glyphs[sc->orig_pos])!= NULL ) {
	    /* Turn any references to this glyph into inline copies of it */
	    for ( bdep=bfc->dependents; bdep!=NULL; bdep=bdnext ) {
		dbc = bdep->bc;
		bdnext = bdep->next;
		brprev = NULL;
		/* May be more than one reference to us, colon has two refs to period */
		/*  but only one dlist entry */
		for ( bref = dbc->refs; bref!=NULL; bref=brnext ) {
		    brnext = bref->next;
		    if ( bref->bdfc==bfc ) {
			BCPasteInto( dbc,bref->bdfc,bref->xoff,bref->yoff,false,false );
			if ( brprev == NULL ) dbc->refs = brnext;
			else brprev->next = brnext;
			free( bref );
		    } else
			brprev = bref;
		}
	    }
	    /* Suppose we have deleted a reference from a composite glyph and than
	     * going to remove the previously referenced glyph from the font. The
	     * void reference still remains in the undoes stack, so that executing Undo/Redo
	     * on the first glyph may lead to unpredictable effects. It is also
	     * impossible to detect such problematic undoes checking just our
	     * going-to-be-deleted glyph's dependents, because the composite character
	     * no longer contains the problematic reference and so is not listed
	     * in the dependents. Thus the only solution seems to be checking
	     * every single glyph in the font.
	     */
	    for ( i=0; i<bdf->glyphcnt; i++ ) if (( dbc = bdf->glyphs[i] ) != NULL ) {
		BCProtectUndoes( dbc->undoes,bfc );
		BCProtectUndoes( dbc->redoes,bfc );
	    }
	    for ( bref=bfc->refs; refs!=NULL; bref = brnext ) {
		brnext = bref->next;
		BCRemoveDependent( bfc,bref );
	    }
	    bdf->glyphs[sc->orig_pos] = NULL;
	    BDFCharFree(bfc);
	}
    }

    SplineCharFree(sc);
    GlyphHashFree(sf);
}

static int MapAddEncodingSlot(EncMap *map,int gid) {
    int enc;

    if ( map->enccount>=map->encmax )
	map->map = realloc(map->map,(map->encmax+=10)*sizeof(int32));
    enc = map->enccount++;
    map->map[enc] = gid;
    map->backmap[gid] = enc;
return( enc );
}

void FVAddEncodingSlot(FontViewBase *fv,int gid) {
    EncMap *map = fv->map;
    int enc;

    enc = MapAddEncodingSlot(map,gid);

    fv->selected = realloc(fv->selected,map->enccount);
    fv->selected[enc] = 0;
    FVAdjustScrollBarRows(fv,enc);
}

void SFAddEncodingSlot(SplineFont *sf,int gid) {
    FontViewBase *fv;
    for ( fv=sf->fv; fv!=NULL; fv = fv->nextsame )
	FVAddEncodingSlot(fv,gid);
}

static int MapAddEnc(SplineFont *sf,SplineChar *sc,EncMap *basemap, EncMap *map,int baseenc, int gid, FontViewBase *fv) {
    int any = false, enc;

    if ( gid>=map->backmax ) {
	map->backmap = realloc(map->backmap,(map->backmax+=10)*sizeof(int32));
	memset(map->backmap+map->backmax-10,-1,10*sizeof(int32));
    }
    if ( map->enc->psnames!=NULL ) {
	/* Check for multiple encodings */
	for ( enc = map->enc->char_cnt-1; enc>=0; --enc ) {
	    if ( map->enc->psnames[enc]!=NULL && strcmp(sc->name,map->enc->psnames[enc])==0 ) {
		if ( !any ) {
		    map->backmap[gid] = enc;
		    any = true;
		}
		map->map[enc] = gid;
	    }
	}
    } else {
	enc = SFFindSlot(sf,map,sc->unicodeenc,sc->name);
	if ( enc!=-1 ) {
	    map->map[enc] = gid;
	    map->backmap[gid] = enc;
	    any = true;
	}
    }
    if ( basemap!=NULL && map->enc==basemap->enc && baseenc!=-1 ) {
	if ( baseenc>=map->enccount ) {
	    if ( fv && map==fv->map )
		FVAddEncodingSlot(fv,gid);
	    else
		MapAddEncodingSlot(map,gid);
	} else {
	    map->map[baseenc] = gid;
	    if ( map->backmap[gid]==-1 )
		map->backmap[gid] = baseenc;
	}
	any = true;
    }
return( any );
}

void SFAddGlyphAndEncode(SplineFont *sf,SplineChar *sc,EncMap *basemap, int baseenc) {
    int gid, mapfound = false;
    FontViewBase *fv;
    BDFFont *bdf;

    if ( sf->cidmaster==NULL ) {
	if ( sf->glyphcnt+1>=sf->glyphmax )
	    sf->glyphs = realloc(sf->glyphs,(sf->glyphmax+=10)*sizeof(SplineChar *));
	gid = sf->glyphcnt++;
	for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( sf->glyphcnt+1>=bdf->glyphmax )
		bdf->glyphs = realloc(bdf->glyphs,(bdf->glyphmax=sf->glyphmax)*sizeof(BDFChar *));
	    if ( sf->glyphcnt>bdf->glyphcnt ) {
		memset(bdf->glyphs+bdf->glyphcnt,0,(sf->glyphcnt-bdf->glyphcnt)*sizeof(BDFChar *));
		bdf->glyphcnt = sf->glyphcnt;
	    }
	}
	for ( fv=sf->fv; fv!=NULL; fv = fv->nextsame ) {
	    EncMap *map = fv->map;
	    if ( gid>=map->backmax )
		map->backmap = realloc(map->backmap,(map->backmax=gid+10)*sizeof(int32));
	    map->backmap[gid] = -1;
	}
    } else {
	gid = baseenc;
	if ( baseenc+1>=sf->glyphmax )
	    sf->glyphs = realloc(sf->glyphs,(sf->glyphmax = baseenc+10)*sizeof(SplineChar *));
	if ( baseenc>=sf->glyphcnt ) {
	    memset(sf->glyphs+sf->glyphcnt,0,(baseenc+1-sf->glyphcnt)*sizeof(SplineChar *));
	    sf->glyphcnt = baseenc+1;
	    for ( bdf = sf->cidmaster->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( baseenc+1>=bdf->glyphmax )
		    bdf->glyphs = realloc(bdf->glyphs,(bdf->glyphmax=baseenc+10)*sizeof(BDFChar *));
		if ( baseenc+1>bdf->glyphcnt ) {
		    memset(bdf->glyphs+bdf->glyphcnt,0,(baseenc+1-bdf->glyphcnt)*sizeof(BDFChar *));
		    bdf->glyphcnt = baseenc+1;
		}
	    }
	    for ( fv=sf->fv; fv!=NULL; fv = fv->nextsame ) if ( fv->sf==sf ) {
		EncMap *map = fv->map;
		if ( gid>=map->backmax )
		    map->backmap = realloc(map->backmap,(map->backmax=gid+10)*sizeof(int32));
		map->backmap[gid] = -1;
	    }
	}
    }
    sf->glyphs[gid] = NULL;
    for ( fv=sf->fv; fv!=NULL; fv = fv->nextsame ) {
	EncMap *map = fv->map;

	FVBiggerGlyphCache(fv,gid);

	if ( !MapAddEnc(sf,sc,basemap,map,baseenc,gid,fv) )
	    FVAddEncodingSlot(fv,gid);
	if ( map==basemap ) mapfound = true;
	if ( fv->normal!=NULL ) {
	    if ( !MapAddEnc(sf,sc,basemap,fv->normal,baseenc,gid,fv))
		MapAddEncodingSlot(fv->normal,gid);
	}
    }
    if ( !mapfound && basemap!=NULL )
	MapAddEnc(sf,sc,basemap,basemap,baseenc,gid,fv);
    sf->glyphs[gid] = sc;
    sc->orig_pos = gid;
    sc->parent = sf;
    SFHashGlyph(sf,sc);
}

static SplineChar *SplineCharMatch(SplineFont *parent,SplineChar *sc) {
    SplineChar *scnew = SFSplineCharCreate(parent);

    scnew->parent = parent;
    scnew->orig_pos = sc->orig_pos;
    scnew->name = copy(sc->name);
    scnew->unicodeenc = sc->unicodeenc;
    scnew->width = sc->width;
    scnew->vwidth = sc->vwidth;
    scnew->widthset = true;
return( scnew );
}

void SFMatchGlyphs(SplineFont *sf,SplineFont *target,int addempties) {
    /* reorder sf so that its glyphs array is the same as that in target */
    int i, j, cnt, cnt2;
    SplineChar **glyphs;
    BDFFont *bdf;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->ticked = false;
    if (( cnt = target->glyphcnt )<sf->glyphcnt ) cnt = sf->glyphcnt;
    glyphs = calloc(cnt,sizeof(SplineChar *));
    for ( i=0; i<target->glyphcnt; ++i ) if ( target->glyphs[i]!=NULL ) {
	SplineChar *sc = SFGetChar(sf,target->glyphs[i]->unicodeenc,target->glyphs[i]->name );
	if ( sc==NULL && addempties )
	    sc = SplineCharMatch(sf,target->glyphs[i]);
	if ( sc!=NULL ) {
	    glyphs[i] = sc;
	    sc->ticked = true;
	}
    }
    for ( i=cnt2=0; i<sf->glyphcnt; ++i )
	if ( sf->glyphs[i]!=NULL && !sf->glyphs[i]->ticked )
	    ++cnt2;
    if ( target->glyphcnt+cnt2>cnt ) {
	glyphs = realloc(glyphs,(target->glyphcnt+cnt2)*sizeof(SplineChar *));
	memset(glyphs+cnt,0,(target->glyphcnt+cnt2-cnt)*sizeof(SplineChar *));
	cnt = target->glyphcnt+cnt2;
    }
    j = target->glyphcnt;
    for ( i=0; i<sf->glyphcnt; ++i )
	if ( sf->glyphs[i]!=NULL && !sf->glyphs[i]->ticked )
	    glyphs[j++] = sf->glyphs[i];
    free(sf->glyphs);
    sf->glyphs = glyphs;
    sf->glyphcnt = sf->glyphmax = cnt;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->orig_pos = i;
    for ( bdf = sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	BDFChar **glyphs;
	glyphs = calloc(sf->glyphcnt,sizeof(BDFChar *));
	for ( i=0; i<bdf->glyphcnt; ++i ) if ( bdf->glyphs[i]!=NULL )
	    glyphs[bdf->glyphs[i]->sc->orig_pos] = bdf->glyphs[i];
	free(bdf->glyphs);
	bdf->glyphs = glyphs;
	bdf->glyphcnt = bdf->glyphmax = sf->glyphcnt;
    }
}

void MMMatchGlyphs(MMSet *mm) {
    /* reorder all instances so that they have the same orig_pos */
    int i, j, index, lasthole;
    SplineFont *sf, *base = NULL;
    SplineChar *sc, *scnew, *sc2;

    for ( i = 0; i<mm->instance_count; ++i ) if ( mm->instances[i]!=NULL ) {
	base = mm->instances[i];
    break;
    }
    if ( base==NULL )
return;

    /* First build up an ordering that uses all glyphs found in any of the */
    /*  sub-fonts, "base" will be the start of it. We will add glyphs to */
    /*  "base" as needed */
    lasthole = -1;
    for ( i = 0; i<mm->instance_count; ++i ) if ( (sf=mm->instances[i])!=NULL && sf!=NULL ) {
	for ( j=0; j<sf->glyphcnt; ++j ) if ( (sc=sf->glyphs[j])!=NULL ) {
	    if ( j<base->glyphcnt && base->glyphs[j]!=NULL &&
		    base->glyphs[j]->unicodeenc==sc->unicodeenc &&
		    strcmp(base->glyphs[j]->name,sc->name)==0 )
	continue;	/* It's good, and in the same place */
	    else if ( (sc2=SFGetChar(base,sc->unicodeenc,sc->name))!=NULL &&
		    sc2->unicodeenc==sc->unicodeenc &&
		    strcmp(sc2->name,sc->name)==0 )
	continue;	/* Well, it's in there somewhere */
	    else {
		/* We need to add it */
		if ( j<base->glyphcnt && base->glyphs[j]==NULL )
		    index = j;
		else {
		    for ( ++lasthole ; lasthole<base->glyphcnt && base->glyphs[lasthole]!=NULL; ++lasthole );
		    index = lasthole;
		    if ( lasthole>=base->glyphmax )
			base->glyphs = realloc(base->glyphs,(base->glyphmax+=20)*sizeof(SplineChar *));
		    if ( lasthole>=base->glyphcnt )
			base->glyphcnt = lasthole+1;
		}
		base->glyphs[index] = scnew = SplineCharMatch(base,sc);
		scnew->orig_pos = index;
	    }
	}
    }

    /* Now force all other instances to match */
    for ( i = 0; i<mm->instance_count; ++i ) if ( (sf=mm->instances[i])!=NULL && sf!=base )
	SFMatchGlyphs(sf,base,true);
    if ( mm->normal!=NULL )
	SFMatchGlyphs(mm->normal,base,true);
}

int32 UniFromEnc(int enc, Encoding *encname) {
    char from[20];
    unichar_t to[20];
    ICONV_CONST char *fpt;
    char *tpt;
    size_t fromlen, tolen;

    if ( encname->is_custom || encname->is_original )
return( -1 );
    if ( enc>=encname->char_cnt )
return( -1 );
    if ( encname->is_unicodebmp || encname->is_unicodefull )
return( enc );
    if ( encname->unicode!=NULL )
return( encname->unicode[enc] );
    else if ( encname->tounicode ) {
	/* To my surprise, on RH9, doing a reset on conversion of CP1258->UCS2 */
	/* causes subsequent calls to return garbage */
	if ( encname->iso_2022_escape_len ) {
	    tolen = sizeof(to); fromlen = 0;
	    iconv(encname->tounicode,NULL,&fromlen,NULL,&tolen);	/* Reset state */
	}
	fpt = from; tpt = (char *) to; tolen = sizeof(to);
	if ( encname->has_1byte && enc<256 ) {
	    *(char *) fpt = enc;
	    fromlen = 1;
	} else if ( encname->has_2byte ) {
	    if ( encname->iso_2022_escape_len )
		strncpy(from,encname->iso_2022_escape,encname->iso_2022_escape_len );
	    fromlen = encname->iso_2022_escape_len;
	    from[fromlen++] = enc>>8;
	    from[fromlen++] = enc&0xff;
	}
	if ( iconv(encname->tounicode,&fpt,&fromlen,&tpt,&tolen)==(size_t) -1 )
return( -1 );
	if ( tpt-(char *) to == 0 ) {
	    /* This strange call appears to be what we need to make CP1258->UCS2 */
	    /*  work.  It's supposed to reset the state and give us the shift */
	    /*  out. As there is no state, and no shift out I have no idea why*/
	    /*  this works, but it does. */
	    if ( iconv(encname->tounicode,NULL,&fromlen,&tpt,&tolen)==(size_t) -1 )
return( -1 );
	}
	if ( tpt-(char *) to == sizeof(unichar_t) )
	{
	    return( to[0] );
	}
    } else if ( encname->tounicode_func!=NULL ) {
return( (encname->tounicode_func)(enc) );
    }
return( -1 );
}

int32 EncFromUni(int32 uni, Encoding *enc) {
    unichar_t from[20];
    unsigned char to[20];
    ICONV_CONST char *fpt;
    char *tpt;
    size_t fromlen, tolen;
    int i;

    if ( enc->is_custom || enc->is_original || enc->is_compact || uni==-1 )
return( -1 );
    if ( enc->is_unicodebmp || enc->is_unicodefull )
return( uni<enc->char_cnt ? uni : -1 );

    if ( enc->unicode!=NULL ) {
	for ( i=0; i<enc->char_cnt; ++i ) {
	    if ( enc->unicode[i]==uni )
return( i );
	}
return( -1 );
    } else if ( enc->fromunicode!=NULL ) {
	/* I don't see how there can be any state to reset in this direction */
	/*  So I don't reset it */
	from[0] = uni;
	fromlen = sizeof(unichar_t);
	fpt = (char *) from; tpt = (char *) to; tolen = sizeof(to);
	iconv(enc->fromunicode,NULL,NULL,NULL,NULL);	/* reset shift in/out, etc. */
	if ( iconv(enc->fromunicode,&fpt,&fromlen,&tpt,&tolen)==(size_t) -1 )
return( -1 );
	if ( tpt-(char *) to == 1 )
return( to[0] );
	if ( enc->iso_2022_escape_len!=0 ) {
	    if ( tpt-(char *) to == enc->iso_2022_escape_len+2 &&
		    strncmp((char *) to,enc->iso_2022_escape,enc->iso_2022_escape_len)==0 )
return( (to[enc->iso_2022_escape_len]<<8) | to[enc->iso_2022_escape_len+1] );
	} else {
	    if ( tpt-(char *) to == sizeof(unichar_t) )
return( (to[0]<<8) | to[1] );
	}
    } else if ( enc->fromunicode_func!=NULL ) {
return( (enc->fromunicode_func)(uni) );
    }
return( -1 );
}

int32 EncFromName(const char *name,enum uni_interp interp,Encoding *encname) {
    int i;
    if ( encname->psnames!=NULL ) {
	for ( i=0; i<encname->char_cnt; ++i )
	    if ( encname->psnames[i]!=NULL && strcmp(name,encname->psnames[i])==0 )
return( i );
    }
    i = UniFromName(name,interp,encname);
    if ( i==-1 && strlen(name)==4 ) {
	/* MS says use this kind of name, Adobe says use the one above */
	char *end;
	i = strtol(name,&end,16);
	if ( i<0 || i>0xffff || *end!='\0' )
return( -1 );
    }
return( EncFromUni(i,encname));
}

void SFExpandGlyphCount(SplineFont *sf, int newcnt) {
    int old = sf->glyphcnt;
    FontViewBase *fv;

    if ( old>=newcnt )
return;
    if ( sf->glyphmax<newcnt ) {
	sf->glyphs = realloc(sf->glyphs,newcnt*sizeof(SplineChar *));
	sf->glyphmax = newcnt;
    }
    memset(sf->glyphs+sf->glyphcnt,0,(newcnt-sf->glyphcnt)*sizeof(SplineChar *));
    sf->glyphcnt = newcnt;

    for ( fv=sf->fv; fv!=NULL; fv=fv->nextsame ) {
	if ( fv->sf==sf ) {	/* Beware of cid keyed fonts which might look at a different subfont */
	    if ( fv->normal!=NULL )
    continue;			/* If compacted then we haven't added any glyphs so haven't changed anything */
	    /* Don't display any of these guys, so not mapped. */
	    /*  No change to selection, or to map->map, but change to backmap */
	    if ( newcnt>fv->map->backmax )
		fv->map->backmap = realloc(fv->map->backmap,(fv->map->backmax = newcnt+5)*sizeof(int32));
	    memset(fv->map->backmap+old,-1,(newcnt-old)*sizeof(int32));
	}
    }
}
