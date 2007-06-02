/* Copyright (C) 2000-2007 by George Williams */
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
#include <utype.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <gfile.h>
#include <gio.h>
#include <gresource.h>
#include "plugins.h"

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
/*  leave them out. I doubt they get used.				     */
static Encoding texbase = { "TeX-Base-Encoding", 256, tex_base_encoding, NULL, NULL, 1, 1, 1, 1 };
       Encoding custom = { "Custom", 0, NULL, NULL, &texbase,			  1, 1, 0, 0, 0, 0, 0, 1, 0, 0 };
static Encoding original = { "Original", 0, NULL, NULL, &custom,		  1, 1, 0, 0, 0, 0, 0, 0, 1, 0 };
static Encoding unicodebmp = { "UnicodeBmp", 65536, NULL, NULL, &original, 	  1, 1, 0, 0, 1, 1, 0, 0, 0, 0 };
static Encoding unicodefull = { "UnicodeFull", 17*65536, NULL, NULL, &unicodebmp, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0 };
static Encoding adobestd = { "AdobeStandard", 256, unicode_from_adobestd, AdobeStandardEncoding, &unicodefull,
										  1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };
static Encoding symbol = { "Symbol", 256, unicode_from_MacSymbol, NULL, &adobestd,1, 1, 1, 1, 0, 0, 0, 0, 0, 0 };

Encoding *enclist = &symbol;

const char *FindUnicharName(void) {
    /* Iconv and libiconv use different names for UCS2. Just great. Perhaps */
    /*  different versions of each use still different names? */
    /* Even worse, both accept UCS-2, but under iconv it means native byte */
    /*  ordering and under libiconv it means big-endian */
    iconv_t test;
    static char *goodname = NULL;
#ifdef UNICHAR_16
    static char *names[] = { "UCS-2", "UCS-2-INTERNAL", "UCS2", "ISO-10646/UCS2", "UNICODE", NULL };
    static char *namesle[] = { "UCS-2LE", "UNICODELITTLE", NULL };
    static char *namesbe[] = { "UCS-2BE", "UNICODEBIG", NULL };
#else
    static char *names[] = { "UCS-4", "UCS-4-INTERNAL", "UCS4", "ISO-10646-UCS-4", "UTF-32", NULL };
    static char *namesle[] = { "UCS-4LE", "UTF-32LE", NULL };
    static char *namesbe[] = { "UCS-4BE", "UTF-32BE", NULL };
#endif
    char **testnames;
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
#ifdef UNICHAR_16
	IError( "I can't figure out your version of iconv(). I need a name for the UCS-2 encoding and I can't find one. Reconfigure --without-iconv. Bye.");
#else
	IError( "I can't figure out your version of iconv(). I need a name for the UCS-4 encoding and I can't find one. Reconfigure --without-iconv. Bye.");
#endif
	exit( 1 );
    }

    test = iconv_open(names[i],"Mac");
    if ( test==(iconv_t) -1 || test==NULL ) {
	IError( "Your version of iconv does not support the \"Mac Roman\" encoding.\nIf this causes problems, reconfigure --without-iconv." );
    } else
	iconv_close(test);

    /* I really should check for ISO-2022-JP, KR, CN, and all the other encodings */
    /*  I might find in a ttf 'name' table. But those tables take too long to build */
return( goodname );
}

static int TryEscape( Encoding *enc,char *escape_sequence ) {
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
	*strchr(buffer,'_') = '-';
	name = buffer;
    } else if ( strcasecmp(name,"iso-8859")==0 ) {
	/* Fixup for old naming conventions */
	strncpy(buffer,name,3);
	strncpy(buffer+3,name+4,sizeof(buffer)-3);
	name = buffer;
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
		    TryEscape( &temp,"\33$(O" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP2")!=NULL &&
		    TryEscape( &temp,"\33$(D" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"JP")!=NULL &&
		    TryEscape( &temp,"\33$B" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"KR")!=NULL &&
		    TryEscape( &temp,"\33$)C\16" ))
		;
	    else if ( strstr(iconv_name,"2022")!=NULL &&
		    strstr(iconv_name,"CN")!=NULL &&
		    TryEscape( &temp,"\33$)A\16" ))
		;
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

    if ( encfile!=NULL )
return( encfile );
    if ( getPfaEditDir(buffer)==NULL )
return( NULL );
    sprintf(buffer,"%s/Encodings.ps", getPfaEditDir(buffer));
    encfile = copy(buffer);
return( encfile );
}

static void EncodingFree(Encoding *item) {
    int i;

    free(item->enc_name);
    if ( item->psnames!=NULL ) for ( i=0; i<item->char_cnt; ++i )
	free(item->psnames[i]);
    free(item->psnames);
    free(item->unicode);
    free(item);
}

/* Parse a TXT file from the unicode consortium */
    /* Unicode Consortium Format A */
    /* List of lines with several fields, */
	/* first is the encoding value (in hex), second the Unicode value (in hex) */
    /* # is a comment character (to eol) */
static Encoding *ParseConsortiumEncodingFile(FILE *file) {
    char buffer[200];
    int32 encs[1024];
    int enc, unienc, max, i;
    Encoding *item;

    for ( i=0; i<1024; ++i )
	encs[i] = 0;
    for ( i=0; i<32; ++i )
	encs[i] = i;
    for ( i=127; i<0xa0; ++i )
	encs[i] = i;
    max = -1;

    while ( fgets(buffer,sizeof(buffer),file)!=NULL ) {
	if ( ishexdigit(buffer[0]) ) {
	    if ( sscanf(buffer, "%x %x", (unsigned *) &enc, (unsigned *) &unienc)==2 &&
		    enc<1024 && enc>=0 ) {
		encs[enc] = unienc;
		if ( enc>max ) max = enc;
	    }
	}
    }

    if ( max==-1 )
return( NULL );

    ++max;
    if ( max<256 ) max = 256;
    item = gcalloc(1,sizeof(Encoding));
    item->only_1byte = item->has_1byte = true;
    item->char_cnt = max;
    item->unicode = galloc(max*sizeof(int32));
    memcpy(item->unicode,encs,max*sizeof(int32));
return( item );
}

static void DeleteEncoding(Encoding *me);

static void RemoveMultiples(Encoding *item) {
    Encoding *test;

    for ( test=enclist; test!=NULL; test = test->next ) {
	if ( strcmp(test->enc_name,item->enc_name)==0 )
    break;
    }
    if ( test!=NULL )
	DeleteEncoding(test);
}

void ParseEncodingFile(char *filename) {
    FILE *file;
    char *orig = filename;
    Encoding *head, *item, *prev, *next;
    char buf[300];
    char *name;
    int i,ch;

    if ( filename==NULL ) filename = getPfaEditEncodings();
    file = fopen(filename,"r");
    if ( file==NULL ) {
	if ( orig!=NULL )
	    gwwv_post_error(_("Couldn't open file"), _("Couldn't open file %.200s"), orig);
return;
    }
    ch = getc(file);
    if ( ch==EOF ) {
	fclose(file);
return;
    }
    ungetc(ch,file);
    if ( ch=='#' || ch=='0' )
	head = ParseConsortiumEncodingFile(file);
    else
	head = PSSlurpEncodings(file);
    fclose(file);
    if ( head==NULL ) {
	gwwv_post_error(_("Bad encoding file format"),_("Bad encoding file format") );
return;
    }

    for ( i=0, prev=NULL, item=head; item!=NULL; prev = item, item=next, ++i ) {
	next = item->next;
	if ( item->enc_name==NULL ) {
	    if ( no_windowing_ui ) {
		gwwv_post_error(_("Bad encoding file format"),_("This file contains an unnamed encoding, which cannot be named in a script"));
return;
	    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	    if ( item==head && item->next==NULL )
		strcpy(buf,_("Please name this encoding"));
	    else {
# if defined( _NO_SNPRINTF ) || defined( __VMS )
		if ( i<=3 )
		    sprintf(buf,
			    _("Please name the %s encoding in this file"),
			    i==1 ? _("First") :
			    i==2 ? _("Second") :
				    _("Third") );
		else
		    sprintf(buf, _("Please name the %dth encoding in this file"),
			    i );
# else
		if ( i<=3 )
		    snprintf(buf,sizeof(buf),
			    _("Please name the %s encoding in this file"),
			    i==1 ? _("_First") : i==2 ? _("Second") : _("Third") );
		else
		    snprintf(buf,sizeof(buf),
			    _("Please name the %dth encoding in this file"),
			    i );
# endif
	    }
	    name = gwwv_ask_string(buf,NULL,buf);
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
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
}

void LoadPfaEditEncodings(void) {
    ParseEncodingFile(NULL);
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

static void DeleteEncoding(Encoding *me) {
    FontView *fv;
    Encoding *prev;

    if ( me->builtin )
return;

    for ( fv = fv_list; fv!=NULL; fv = fv->next ) {
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

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static GTextInfo *EncodingList(void) {
    GTextInfo *ti;
    int i;
    Encoding *item;

    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ++i;
    ti = gcalloc(i+1,sizeof(GTextInfo));
    i = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->builtin )
	    ti[i++].text = uc_copy(item->enc_name);
    if ( i!=0 )
	ti[0].selected = true;
return( ti );
}

#define CID_Encodings	1001

static int DE_Delete(GGadget *g, GEvent *e) {
    GWindow gw;
    int *done;
    GGadget *list;
    int sel,i;
    Encoding *item;

    if ( e->type==et_controlevent &&
	    (e->u.control.subtype == et_buttonactivate ||
	     e->u.control.subtype == et_listdoubleclick )) {
	gw = GGadgetGetWindow(g);
	done = GDrawGetUserData(gw);
	list = GWidgetGetControl(gw,CID_Encodings);
	sel = GGadgetGetFirstListSelectedItem(list);
	i=0;
	for ( item=enclist; item!=NULL; item=item->next ) {
	    if ( item->builtin )
		/* Do Nothing */;
	    else if ( i==sel )
	break;
	    else
		++i;
	}
	if ( item!=NULL )
	    DeleteEncoding(item);
	*done = true;
    }
return( true );
}

static int DE_Cancel(GGadget *g, GEvent *e) {
    GWindow gw;
    int *done;

    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	gw = GGadgetGetWindow(g);
	done = GDrawGetUserData(gw);
	*done = true;
    }
return( true );
}

static int de_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	int *done = GDrawGetUserData(gw);
	*done = true;
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void RemoveEncoding(void) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[5];
    GTextInfo label[5];
    Encoding *item;
    int done = 0;

    for ( item=enclist; item!=NULL && item->builtin; item=item->next );
    if ( item==NULL )
return;

    memset(&gcd,0,sizeof(gcd));
    memset(&label,0,sizeof(label));
    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_restrict;
    wattrs.event_masks = ~(1<<et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Remove Encoding");
    pos.x = pos.y = 0;
    pos.width = GGadgetScale(GDrawPointsToPixels(NULL,150));
    pos.height = GDrawPointsToPixels(NULL,110);
    gw = GDrawCreateTopWindow(NULL,&pos,de_e_h,&done,&wattrs);

    gcd[0].gd.pos.x = 10; gcd[0].gd.pos.y = 6;
    gcd[0].gd.pos.width = 130; gcd[0].gd.pos.height = 5*12+10;
    gcd[0].gd.flags = gg_visible | gg_enabled;
    gcd[0].gd.cid = CID_Encodings;
    gcd[0].gd.u.list = EncodingList();
    gcd[0].gd.handle_controlevent = DE_Delete;
    gcd[0].creator = GListCreate;

    gcd[2].gd.pos.x = -10; gcd[2].gd.pos.y = gcd[0].gd.pos.y+gcd[0].gd.pos.height+5;
    gcd[2].gd.pos.width = -1; gcd[2].gd.pos.height = 0;
    gcd[2].gd.flags = gg_visible | gg_enabled | gg_but_cancel ;
    label[2].text = (unichar_t *) _("_Cancel");
    label[2].text_is_1byte = true;
    label[2].text_in_resource = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.mnemonic = 'C';
    gcd[2].gd.handle_controlevent = DE_Cancel;
    gcd[2].creator = GButtonCreate;

    gcd[1].gd.pos.x = 10-3; gcd[1].gd.pos.y = gcd[2].gd.pos.y-3;
    gcd[1].gd.pos.width = -1; gcd[1].gd.pos.height = 0;
    gcd[1].gd.flags = gg_visible | gg_enabled | gg_but_default ;
    label[1].text = (unichar_t *) _("_Delete");
    label[1].text_is_1byte = true;
    label[1].text_in_resource = true;
    gcd[1].gd.mnemonic = 'D';
    gcd[1].gd.label = &label[1];
    gcd[1].gd.handle_controlevent = DE_Delete;
    gcd[1].creator = GButtonCreate;

    gcd[3].gd.pos.x = 2; gcd[3].gd.pos.y = 2;
    gcd[3].gd.pos.width = pos.width-4; gcd[3].gd.pos.height = pos.height-2;
    gcd[3].gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    gcd[3].creator = GGroupCreate;

    GGadgetsCreate(gw,gcd);
    GTextInfoListFree(gcd[0].gd.u.list);

    GWidgetHidePalettes();
    GDrawSetVisible(gw,true);
    while ( !done )
	GDrawProcessOneEvent(NULL);
    GDrawDestroyWindow(gw);
}

Encoding *MakeEncoding(SplineFont *sf,EncMap *map) {
    char *name;
    int i, gid;
    Encoding *item, *temp;
    SplineChar *sc;

    if ( map->enc!=&custom )
return(NULL);

    name = gwwv_ask_string(_("Please name this encoding"),NULL,_("Please name this encoding"));
    if ( name==NULL )
return(NULL);
    item = gcalloc(1,sizeof(Encoding));
    item->enc_name = name;
    item->only_1byte = item->has_1byte = true;
    item->char_cnt = map->enccount;
    item->unicode = gcalloc(map->enccount,sizeof(int32));
    for ( i=0; i<map->enccount; ++i ) if ( (gid = map->map[i])!=-1 && (sc=sf->glyphs[gid])!=NULL ) {
	if ( sc->unicodeenc!=-1 )
	    item->unicode[i] = sc->unicodeenc;
	else if ( strcmp(sc->name,".notdef")!=0 ) {
	    if ( item->psnames==NULL )
		item->psnames = gcalloc(map->enccount,sizeof(unichar_t *));
	    item->psnames[i] = copy(sc->name);
	}
    }
    RemoveMultiples(item);

    if ( enclist == NULL )
	enclist = item;
    else {
	for ( temp=enclist; temp->next!=NULL; temp=temp->next );
	temp->next = item;
    }
    DumpPfaEditEncodings();
return( item );
}

void LoadEncodingFile(void) {
    static char filter[] = "*.{ps,PS,txt,TXT,enc,ENC}";
    char *fn;
    char *filename;

    fn = gwwv_open_filename(_("Load Encoding"), NULL, filter, NULL);
#endif
    if ( fn==NULL )
return;
    filename = utf82def_copy(fn);
    ParseEncodingFile(filename);
    free(fn); free(filename);
    DumpPfaEditEncodings();
}

void SFFindNearTop(SplineFont *sf) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontView *fv;
    EncMap *map;
    int i,k,gid;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    if ( sf->subfontcnt==0 ) {
	for ( fv=sf->fv; fv!=NULL; fv=fv->nextsame ) {
	    map = fv->map;
	    fv->sc_near_top = NULL;
	    for ( i=fv->rowoff*fv->colcnt; i<map->enccount && i<(fv->rowoff+fv->rowcnt)*fv->colcnt; ++i )
		if ( (gid=map->map[i])!=-1 && sf->glyphs[gid]!=NULL ) {
		    fv->sc_near_top = sf->glyphs[gid];
	    break;
		}
	}
    } else {
	for ( fv=sf->fv; fv!=NULL; fv=fv->nextsame ) {
	    map = fv->map;
	    fv->sc_near_top = NULL;
	    for ( i=fv->rowoff*fv->colcnt; i<map->enccount && i<(fv->rowoff+fv->rowcnt)*fv->colcnt; ++i ) {
		for ( k=0; k<sf->subfontcnt; ++k )
		    if ( (gid=map->map[i])!=-1 &&
			    gid<sf->subfonts[k]->glyphcnt &&
			    sf->subfonts[k]->glyphs[gid]!=NULL )
			fv->sc_near_top = sf->subfonts[k]->glyphs[gid];
	    }
	}
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}

void SFRestoreNearTop(SplineFont *sf) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontView *fv;

    for ( fv=sf->fv; fv!=NULL; fv=fv->nextsame ) if ( fv->sc_near_top!=NULL ) {
	/* Note: For CID keyed fonts, we don't care if sc is in the currenly */
	/*  displayed font, all we care about is the CID */
	int enc = fv->map->backmap[fv->sc_near_top->orig_pos];
	if ( enc!=-1 ) {
	    fv->rowoff = enc/fv->colcnt;
	    GScrollBarSetPos(fv->vsb,fv->rowoff);
	    /* Don't ask for an expose event yet. We'll get one soon enough */
	}
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}

/* ************************************************************************** */
/* ****************************** CID Encodings ***************************** */
/* ************************************************************************** */
struct cidmap {
    char *registry, *ordering;
    int supplement, maxsupple;
    int cidmax;			/* Max cid found in the charset */
    int namemax;		/* Max cid with useful info */
    uint32 *unicode;
    char **name;
    struct cidmap *next;
};

static struct cidmap *cidmaps = NULL;

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
    }
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

#if defined( _NO_SNPRINTF ) || defined( __VMS )
    if ( map==NULL )
	sprintf(buffer,"cid-%d", cid);
    else if ( cid<map->namemax && map->name[cid]!=NULL )
	strncpy(buffer,map->name[cid],len);
    else if ( cid==0 || (cid<map->namemax && map->unicode[cid]!=0 )) {
	if ( map->unicode==NULL || map->namemax==0 )
	    enc = 0;
	else
	    enc = map->unicode[cid];
	temp = StdGlyphName(buffer,enc,ui_none,(NameList *) -1);
	if ( temp!=buffer )
	    strcpy(buffer,temp);
    } else
	sprintf(buffer,"%s.%d", map->ordering, cid);
#else
    if ( map==NULL )
	snprintf(buffer,len,"cid-%d", cid);
    else if ( cid<map->namemax && map->name[cid]!=NULL )
	strncpy(buffer,map->name[cid],len);
    else if ( cid==0 )
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
#endif
return( enc );
}

int NameUni2CID(struct cidmap *map,int uni, const char *name) {
    int i;

    if ( map==NULL )
return( -1 );
    if ( uni!=-1 ) {
	for ( i=0; i<map->namemax; ++i )
	    if ( map->unicode[i]==uni )
return( i );
    } else {
	for ( i=0; i<map->namemax; ++i )
	    if ( map->name[i]!=NULL && strcmp(map->name[i],name)==0 )
return( i );
    }
return( -1 );
}

int MaxCID(struct cidmap *map) {
return( map->cidmax );
}

static char *SearchDirForCidMap(char *dir,char *registry,char *ordering,
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
	    ret = galloc(strlen(dir)+1+len+1);
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
	ret = galloc(strlen(dir)+1+strlen(maybe)+1);
	strcpy(ret,dir);
	strcat(ret,"/");
	strcat(ret,maybe);
	*maybefile = ret;
    }
return( NULL );
}

static char *SearchNoLibsDirForCidMap(char *dir,char *registry,char *ordering,
	int supplement,char **maybefile) {
    char *ret;

    if ( dir==NULL || strstr(dir,"/.libs")==NULL )
return( NULL );

    dir = copy(dir);
    *strstr(dir,"/.libs") = '\0';

    ret = SearchDirForCidMap(dir,registry,ordering,supplement,maybefile);
    free(dir);
return( ret );
}

static struct cidmap *MakeDummyMap(char *registry,char *ordering,int supplement) {
    struct cidmap *ret = galloc(sizeof(struct cidmap));

    ret->registry = copy(registry);
    ret->ordering = copy(ordering);
    ret->supplement = ret->maxsupple = supplement;
    ret->cidmax = ret->namemax = 0;
    ret->unicode = NULL; ret->name = NULL;
    ret->next = cidmaps;
    cidmaps = ret;
return( ret );
}

struct cidmap *LoadMapFromFile(char *file,char *registry,char *ordering,
	int supplement) {
    struct cidmap *ret = galloc(sizeof(struct cidmap));
    char *pt = strrchr(file,'.');
    FILE *f;
    int cid1, cid2, uni, cnt, i;
    char name[100];

    while ( pt>file && isdigit(pt[-1]))
	--pt;
    ret->supplement = ret->maxsupple = strtol(pt,NULL,10);
    if ( supplement>ret->maxsupple )
	ret->maxsupple = supplement;
    ret->registry = copy(registry);
    ret->ordering = copy(ordering);
    ret->next = cidmaps;
    cidmaps = ret;

    f = fopen( file,"r" );
    if ( f==NULL ) {
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Missing cidmap file"),_("Couldn't open cidmap file: %s"), file );
#else
	gwwv_post_error(_("Couldn't open file"), _("Couldn't open file %.200s"), file);
#endif
	ret->cidmax = ret->namemax = 0;
	ret->unicode = NULL; ret->name = NULL;
    } else if ( fscanf( f, "%d %d", &ret->cidmax, &ret->namemax )!=2 ) {
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad cidmap file"),_("%s is not a cidmap file, please download\nhttp://fontforge.sourceforge.net/cidmaps.tgz"), file );
	fprintf( stderr, _("%s is not a cidmap file, please download\nhttp://fontforge.sourceforge.net/cidmaps.tgz"), file );
#else
	gwwv_post_error(_("Bad Cidmap File"), _("%s is not a cidmap file, please download\nhttp://fontforge.sourceforge.net/cidmaps.tgz"), file);
	fprintf( stderr, "%s is not a cidmap file, please download\nhttp://fontforge.sourceforge.net/cidmaps.tgz", file );
#endif
	ret->cidmax = ret->namemax = 0;
	ret->unicode = NULL; ret->name = NULL;
    } else {
	ret->unicode = gcalloc(ret->namemax+1,sizeof(uint32));
	ret->name = gcalloc(ret->namemax+1,sizeof(char *));
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
		if ( fscanf(f,"%x", (unsigned *) &uni )==1 )
		    ret->unicode[cid1] = uni;
		else if ( fscanf(f," /%s", name )==1 )
		    ret->name[cid1] = copy(name);
	    }
	}
	fclose(f);
    }
    free(file);
return( ret );
}

struct cidmap *FindCidMap(char *registry,char *ordering,int supplement,SplineFont *sf) {
    struct cidmap *map, *maybe=NULL;
    char *file, *maybefile=NULL;
    int maybe_sup = -1;
    char *buts[3], *buts2[3], *buts3[3];
    char buf[100];
    int ret;

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
	file = SearchDirForCidMap(GResourceProgramDir,registry,ordering,supplement,&maybefile);
    if ( file==NULL )
	file = SearchNoLibsDirForCidMap(GResourceProgramDir,registry,ordering,supplement,&maybefile);
#ifdef SHAREDIR
    if ( file==NULL )
	file = SearchDirForCidMap(SHAREDIR,registry,ordering,supplement,&maybefile);
#endif
    if ( file==NULL )
	file = SearchDirForCidMap(getPfaEditShareDir(),registry,ordering,supplement,&maybefile);
    if ( file==NULL )
	file = SearchDirForCidMap("/usr/share/fontforge",registry,ordering,supplement,&maybefile);

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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	buts[0] = _("_Use It"); buts[1] = _("_Search"); buts[2] = NULL;
	ret = gwwv_ask(_("Use CID Map"),(const char **) buts,0,1,_("This font is based on the charset %1$.20s-%2$.20s-%3$d, but the best I've been able to find is %1$.20s-%2$.20s-%4$d.\nShall I use that or let you search?"),
		registry,ordering,supplement,maybe_sup);
#else
	ret = 0;		/* In a script, default to using an old version of the file */
				/* We'll get most cids that way */
#endif
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

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( file==NULL ) {
	char *uret;
#if defined( _NO_SNPRINTF ) || defined( __VMS )
	sprintf(buf,"%s-%s-*.cidmap", registry, ordering );
#else
	snprintf(buf,sizeof(buf),"%s-%s-*.cidmap", registry, ordering );
#endif
	if ( maybe==NULL && maybefile==NULL ) {
	    buts3[0] = _("_Browse"); buts3[1] = _("_Give Up"); buts3[2] = NULL;
	    ret = gwwv_ask(_("No cidmap file..."),(const char **)buts3,0,1,_("FontForge was unable to find a cidmap file for this font. It is not essential to have one, but some things will work better if you do. If you have not done so you might want to download the cidmaps from:\n   http://FontForge.sourceforge.net/cidmaps.tgz\nand then gunzip and untar them and move them to:\n  %.80s\n\nWould you like to search your local disk for an appropriate file?"),
#ifdef SHAREDIR
		    SHAREDIR
#else
		    getPfaEditShareDir()==NULL?"/usr/share/fontforge":getPfaEditShareDir()
#endif
		    );
	    if ( ret==1 || no_windowing_ui )
		buf[0] = '\0';
	}
	uret = NULL;
	if ( buf[0]!='\0' && !no_windowing_ui ) {
	    if ( sf!=NULL ) sf->loading_cid_map = true;
	    uret = gwwv_open_filename(_("Find a cidmap file..."),NULL,buf,NULL);
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
	    } else if ( gwwv_ask(_("Use CID Map"),(const char **)buts2,0,1,_("Are you sure you don't want to use the cidmap I found?"))==0 ) {
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
#else		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    if ( maybe==NULL && maybefile==NULL )
	/* No luck */;
    else if ( maybe!=NULL ) {
	maybe->maxsupple = supplement;
return( maybe );
    } else {
	file = maybefile;
	maybefile = NULL;
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    free(maybefile);
    if ( file!=NULL )
return( LoadMapFromFile(file,registry,ordering,supplement));

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

    glyphs = gcalloc(glyphcnt+1,sizeof(SplineChar *));
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

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( anyextras ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	char *buttons[3];
	buttons[0] = _("_Delete");
	buttons[1] = _("_Add");
	buttons[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
	static char *buttons[] = { GTK_STOCK_DELETE, GTK_STOCK_ADD, NULL };
#endif
	if ( gwwv_ask(_("Extraneous glyphs"),(const char **) buttons,0,1,_("The current encoding contains glyphs which I cannot map to CIDs.\nShould I delete them or add them to the end (where they may conflict with future ros definitions)?"))==1 ) {
	    if ( map!=NULL && max<map->cidmax ) max = map->cidmax;
	    anyextras = 0;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( SCWorthOutputting(sc = sf->glyphs[i]) ) {
		if ( sc->orig_pos == -1 ) sc->orig_pos = max + anyextras++;
	    }
	    max += anyextras;
	}
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    SFApplyOrdering(sf, max+1);
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
struct block {
    int cur, tot;
    char **maps;
    char **dirs;
};

static void AddToBlock(struct block *block,char *mapname, char *dir) {
    int i, val, j;
    int len = strlen(mapname);

    if ( mapname[len-7]=='.' ) len -= 7;
    for ( i=0; i<block->cur; ++i ) {
	if ( (val=strncmp(block->maps[i],mapname,len))==0 )
return;		/* Duplicate */
	else if ( val>0 )
    break;
    }
    if ( block->tot==0 ) {
	block->tot = 10;
	block->maps = galloc(10*sizeof(char *));
	block->dirs = galloc(10*sizeof(char *));
    } else if ( block->cur>=block->tot ) {
	block->tot += 10;
	block->maps = grealloc(block->maps,block->tot*sizeof(char *));
	block->dirs = grealloc(block->dirs,block->tot*sizeof(char *));
    }
    for ( j=block->cur; j>=i; --j ) {
	block->maps[j+1] = block->maps[j];
	block->dirs[j+1] = block->dirs[j];
    }
    block->maps[i] = copyn(mapname,len);
    block->dirs[i] = dir;
    ++block->cur;
}

static void FindMapsInDir(struct block *block,char *dir) {
    struct dirent *ent;
    DIR *d;
    int len;
    char *pt, *pt2;

    if ( dir==NULL )
return;
    /* format of cidmap filename "?*-?*-[0-9]*.cidmap" */
    d = opendir(dir);
    if ( d==NULL )
return;
    while ( (ent = readdir(d))!=NULL ) {
	if ( (len = strlen(ent->d_name))<8 )
    continue;
	if ( strcmp(ent->d_name+len-7,".cidmap")!=0 )
    continue;
	pt = strchr(ent->d_name, '-');
	if ( pt==NULL || pt==ent->d_name )
    continue;
	pt2 = strchr(pt+1, '-' );
	if ( pt2==NULL || pt2==pt+1 || !isdigit(pt2[1]))
    continue;
	AddToBlock(block,ent->d_name,dir);
    }
}

static void FindMapsInNoLibsDir(struct block *block,char *dir) {

    if ( dir==NULL || strstr(dir,"/.libs")==NULL )
return;

    dir = copy(dir);
    *strstr(dir,"/.libs") = '\0';

    FindMapsInDir(block,dir);
    free(dir);
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
struct cidmap *AskUserForCIDMap(SplineFont *sf) {
    struct block block;
    struct cidmap *map = NULL;
    char buffer[200];
    char **choices;
    int i,ret;
    char *filename=NULL;
    char *reg, *ord, *pt;
    int supplement;

    memset(&block,'\0',sizeof(block));
    for ( map = cidmaps; map!=NULL; map = map->next ) {
	sprintf(buffer,"%s-%s-%d", map->registry, map->ordering, map->supplement);
	AddToBlock(&block,buffer,NULL);
    }
    FindMapsInDir(&block,".");
    FindMapsInDir(&block,GResourceProgramDir);
    FindMapsInNoLibsDir(&block,GResourceProgramDir);
#ifdef SHAREDIR
    FindMapsInDir(&block,SHAREDIR);
#endif
    FindMapsInDir(&block,getPfaEditShareDir());
    FindMapsInDir(&block,"/usr/share/fontforge");

    choices = gcalloc(block.cur+2,sizeof(unichar_t *));
    choices[0] = copy(_("Browse..."));
    for ( i=0; i<block.cur; ++i )
	choices[i+1] = copy(block.maps[i]);
    ret = gwwv_choose(_("Find a cidmap file..."),(const char **) choices,i+1,0,_("Please select a CID ordering"));
    for ( i=0; i<=block.cur; ++i )
	free( choices[i] );
    free(choices);
    if ( ret==0 ) {
	filename = gwwv_open_filename(_("Find a cidmap file..."),NULL,
		"?*-?*-[0-9]*.cidmap",NULL);
	if ( filename==NULL )
	    ret = -1;
    }
    if ( ret!=-1 ) {
	if ( filename!=NULL )
	    /* Do nothing for now */;
	else if ( block.dirs[ret-1]!=NULL ) {
	    filename = galloc(strlen(block.dirs[ret-1])+strlen(block.maps[ret-1])+3+8);
	    strcpy(filename,block.dirs[ret-1]);
	    strcat(filename,"/");
	    strcat(filename,block.maps[ret-1]);
	    strcat(filename,".cidmap");
	}
	if ( ret!=0 )
	    reg = block.maps[ret-1];
	else {
	    reg = strrchr(filename,'/');
	    if ( reg==NULL ) reg = filename;
	    else ++reg;
	    reg = copy(reg);
	}
	pt = strchr(reg,'-');
	if ( pt==NULL )
	    ret = -1;
	else {
	    *pt = '\0';
	    ord = pt+1;
	    pt = strchr(ord,'-');
	    if ( pt==NULL )
		ret = -1;
	    else {
		*pt = '\0';
		supplement = strtol(pt+1,NULL,10);
	    }
	}
	if ( ret == -1 )
	    /* No map */;
	else if ( filename==NULL )
	    map = FindCidMap(reg,ord,supplement,sf);
	else
	    map = LoadMapFromFile(filename,reg,ord,supplement);
	if ( ret!=0 && reg!=block.maps[ret-1] )
	    free(reg);
	/*free(filename);*/	/* Freed by loadmap */
    }
    for ( i=0; i<block.cur; ++i )
	free( block.maps[i]);
    free(block.maps);
    free(block.dirs);
    if ( map!=NULL ) {
	sf->cidregistry = copy(map->registry);
	sf->ordering = copy(map->ordering);
	sf->supplement = map->supplement;
    }
return( map );
}

static enum fchooserret CMapFilter(GGadget *g,GDirEntry *ent,
	const unichar_t *dir) {
    enum fchooserret ret = GFileChooserDefFilter(g,ent,dir);
    char buf2[256];
    FILE *file;
    static char *cmapflag = "%!PS-Adobe-3.0 Resource-CMap";

    if ( ret==fc_show && !ent->isdir ) {
	int len = 3*(u_strlen(dir)+u_strlen(ent->name)+5);
	char *filename = galloc(len);
	u2def_strncpy(filename,dir,len);
	strcat(filename,"/");
	u2def_strncpy(buf2,ent->name,sizeof(buf2));
	strcat(filename,buf2);
	file = fopen(filename,"r");
	if ( file==NULL )
	    ret = fc_hide;
	else {
	    if ( fgets(buf2,sizeof(buf2),file)==NULL ||
		    strncmp(buf2,cmapflag,strlen(cmapflag))!=0 )
		ret = fc_hide;
	    fclose(file);
	}
	free(filename);
    }
return( ret );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

enum cmaptype { cmt_out=-1, cmt_coderange, cmt_notdefs, cmt_cid, cmt_max };
struct cmap {
    struct {
	int n;
	struct coderange { uint32 first, last, cid; } *ranges;
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
	ranges = gcalloc(val,sizeof(struct coderange));
    else {
	ranges = grealloc(ranges,(*n+val)*sizeof(struct coderange));
	memset(ranges+*n,0,val*sizeof(struct coderange));
    }
    *n += val;
return( ranges );
}

static char *readpsstr(char *str) {
    char *eos;

    while ( isspace(*str)) ++str;
    if ( *str=='(' ) ++str;
    /* Postscript strings can be more complicated than this (hex, nested parens, Enc85...) */
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
    static const char *bcsr = "begincodespacerange", *bndr = "beginnotdefrange", *bcr = "begincidrange";
    static const char *reg = "/Registry", *ord = "/Ordering", *sup="/Supplement";

    file = fopen(filename,"r");
    if ( file==NULL )
return( NULL );

    cmap = gcalloc(1,sizeof(struct cmap));
    in = cmt_out;
    while ( fgets(buf2,sizeof(buf2),file)!=NULL ) {
	for ( pt=buf2; isspace(*pt); ++pt);
	if ( in==cmt_out ) {
	    if ( *pt=='/' ) {
		if ( strncmp(pt,reg,strlen(reg))==0 )
		    cmap->registry = readpsstr(pt+strlen(reg));
		else if ( strncmp(pt,ord,strlen(ord))==0 )
		    cmap->ordering = readpsstr(pt+strlen(ord));
		else if ( strncmp(pt,ord,strlen(ord))==0 ) {
		    for ( pt += strlen(sup); isspace(*pt); ++pt );
		    cmap->supplement = strtol(pt,NULL,10);
		}
    continue;
	    } else if ( !isdigit(*pt) )
    continue;
	    val = strtol(pt,&end,10);
	    while ( isspace(*end)) ++end;
	    if ( strncmp(end,bcsr,strlen(bcsr))==0 )
		in = cmt_coderange;
	    else if ( strncmp(end,bndr,strlen(bndr))==0 )
		in = cmt_notdefs;
	    else if ( strncmp(end,bcr,strlen(bcr))==0 )
		in = cmt_cid;
	    if ( in!=cmt_out ) {
		pos = cmap->groups[in].n;
		cmap->groups[in].ranges = ExtendArray(cmap->groups[in].ranges,&cmap->groups[in].n,val);
	    }
	} else if ( strncmp(pt,"end",3)== 0 )
	    in = cmt_out;
	else {
	    if ( *pt!='<' )
	continue;
	    cmap->groups[in].ranges[pos].first = strtoul(pt+1,&end,16);
	    if ( *end=='>' ) ++end;
	    while ( isspace(*end)) ++end;
	    if ( *end=='<' ) ++end;
	    cmap->groups[in].ranges[pos].last = strtoul(end,&end,16);
	    if ( in!=cmt_coderange ) {
		if ( *end=='>' ) ++end;
		while ( isspace(*end)) ++end;
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

    cmap->remap = gcalloc(cmap->groups[cmt_coderange].n+1,sizeof(struct remap));
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
    FontView *fvs;
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
    sprintf(buffer,"%g", cidmaster->cidversion);
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
	    fvs->selected = gcalloc(new->glyphcnt,sizeof(char));
	    if ( fvs->map->encmax < new->glyphcnt )
		fvs->map->map = grealloc(fvs->map->map,(fvs->map->encmax = new->glyphcnt)*sizeof(int));
	    fvs->map->enccount = new->glyphcnt;
	    if ( fvs->map->backmax < new->glyphcnt )
		fvs->map->backmap = grealloc(fvs->map->backmap,(fvs->map->backmax = new->glyphcnt)*sizeof(int));
	    for ( j=0; j<new->glyphcnt; ++j )
		fvs->map->map[j] = fvs->map->backmap[j] = j;
	}
	fvs->sf = new;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	FVSetTitle(fvs);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontViewReformatAll(new);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    SplineFontFree(cidmaster);
return( new );
}

void SFFlatten(SplineFont *cidmaster) {
    SplineChar **glyphs;
    int i,j,max;

    if ( cidmaster==NULL )
return;
    if ( cidmaster->cidmaster!=NULL )
	cidmaster = cidmaster->cidmaster;
    /* This doesn't change the ordering, so no need for special tricks to */
    /*  preserve scrolling location. */
    for ( i=max=0; i<cidmaster->subfontcnt; ++i ) {
	if ( max<cidmaster->subfonts[i]->glyphcnt )
	    max = cidmaster->subfonts[i]->glyphcnt;
    }
    glyphs = gcalloc(max,sizeof(SplineChar *));
    for ( j=0; j<max; ++j ) {
	for ( i=0; i<cidmaster->subfontcnt; ++i ) {
	    if ( j<cidmaster->subfonts[i]->glyphcnt && cidmaster->subfonts[i]->glyphs[j]!=NULL ) {
		glyphs[j] = cidmaster->subfonts[i]->glyphs[j];
		cidmaster->subfonts[i]->glyphs[j] = NULL;
	break;
	    }
	}
    }
    CIDFlatten(cidmaster,glyphs,max);
}

int SFFlattenByCMap(SplineFont *sf,char *cmapname) {
    struct cmap *cmap;
    int i,j,k,l,m, extras, max, curmax, warned;
    int found[4];
    SplineChar **glyphs = NULL, *sc;
    FontView *fvs;

    if ( sf->cidmaster!=NULL )
	sf = sf->cidmaster;
    if ( sf->subfontcnt==0 ) {
	gwwv_post_error(_("Not a CID-keyed font"),_("Not a CID-keyed font"));
return( false );
    }
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( cmapname==NULL ) {
	cmapname = gwwv_open_filename(_("Find an adobe CMap file..."),NULL,NULL,CMapFilter);
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
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
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_post_error(_("Encoding Too Large"),_("Encoding Too Large"));
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_error(_("Encoding Too Large"),_("Encoding Too Large"));
#endif
	    cmapfree(cmap);
return( false );
	}
    }

    SFFindNearTop(sf);
    curmax = 0;
    for ( k=0; k<sf->subfontcnt; ++k ) {
	if ( curmax < sf->subfonts[k]->glyphcnt )
	    curmax = sf->subfonts[k]->glyphcnt;
    }

    glyphs = gcalloc(curmax,sizeof(SplineChar *));
    for ( i=0; i<curmax; ++i ) {
	for ( k=0; k<sf->subfontcnt; ++k )
	    if ( i<sf->subfonts[k]->glyphcnt && sf->subfonts[k]->glyphs[i]!=NULL ) {
		glyphs[i] = sf->subfonts[k]->glyphs[i];
		sf->subfonts[k]->glyphs[i] = NULL;
	break;
	    }
    }
    sf = CIDFlatten(sf,glyphs,curmax);

    warned = true;
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
#if defined(FONTFORGE_CONFIG_GDRAW)
				ff_post_notice(_("MultipleEncodingIgnored"),
					_("The glyph at CID %d is mapped to more than %d encodings. Only the first %d are handled."), i,
					sizeof(found)/sizeof(found[0]),
					sizeof(found)/sizeof(found[0]));
#elif defined(FONTFORGE_CONFIG_GTK)
				ff_post_notice(_("MultipleEncodingIgnored"),
					_("The glyph at CID %d is mapped to more than %d encodings. Only the first %d are handled."), i,
					sizeof(found)/sizeof(found[0]),
					sizeof(found)/sizeof(found[0]));
#endif
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
		map->map = grealloc(map->map,(map->encmax = map->enccount = max+extras)*sizeof(int));
		memset(map->map,-1,map->enccount*sizeof(int));
		memset(map->backmap,-1,sf->glyphcnt*sizeof(int));
		map->remap = cmap->remap; cmap->remap = NULL;
	    }
	    warned = true;
	}
    }
    cmapfree(cmap);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontViewReformatAll(sf);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    SFRestoreNearTop(sf);
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
    SplineChar *sc;
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
	    if ( sc->orig_pos==0 ) sc->orig_pos = -1;
	}
	if ( sc->orig_pos>max ) max = sc->orig_pos;
	else if ( sc->orig_pos==-1 ) ++anyextras;
    }

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( anyextras ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
	char *buttons[3];
	buttons[0] = _("_Delete");
	buttons[1] = _("_Add");
	buttons[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
	static char *buttons[] = { GTK_STOCK_DELETE, GTK_STOCK_ADD, NULL };
#endif
	if ( gwwv_ask(_("Extraneous glyphs"),(const char **) buttons,0,1,_("The current encoding contains glyphs which I cannot map to CIDs.\nShould I delete them or add them to the end (where they may conflict with future ros definitions)?"))==1 ) {
	    if ( cmap!=NULL && max<cmap->total ) max = cmap->total;
	    anyextras = 0;
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc = sf->glyphs[i])!=NULL ) {
		if ( sc->orig_pos == -1 ) sc->orig_pos = max + anyextras++;
	    }
	    max += anyextras;
	}
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    SFApplyOrdering(sf, max);
}

SplineFont *MakeCIDMaster(SplineFont *sf,EncMap *oldmap,int bycmap,char *cmapfilename, struct cidmap *cidmap) {
    SplineFont *cidmaster;
    struct cidmap *map;
    struct cmap *cmap;
    FontView *fvs;
    int freeme;

    cidmaster = SplineFontEmpty();
    SFFindNearTop(sf);
    if ( bycmap ) {
	freeme = false;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	if ( cmapfilename==NULL ) {
	    cmapfilename = gwwv_open_filename(_("Find an adobe CMap file..."),NULL,NULL,CMapFilter);
	    freeme = true;
	}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	if ( cmapfilename==NULL ) {
	    SplineFontFree(cidmaster);
return(NULL);
	}
	cmap = ParseCMap(cmapfilename);
	if ( freeme )
	    free(cmapfilename);
	if ( cmap==NULL ) {
	    SplineFontFree(cidmaster);
return(NULL);
	}
	CompressCMap(cmap);
	SFEncodeToCMap(cidmaster,sf,oldmap,cmap);
	cmapfree(cmap);
    } else {
	map = cidmap;
	if (map == NULL) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	    map = AskUserForCIDMap(cidmaster);		/* Sets the ROS fields */
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	} else {
	    cidmaster->cidregistry = copy(map->registry);
	    cidmaster->ordering = copy(map->ordering);
	    cidmaster->supplement = map->supplement;
	}
	if ( map==NULL ) {
	    SplineFontFree(cidmaster);
return(NULL);
	}
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
    cidmaster->subfonts = gcalloc(2,sizeof(SplineFont *));
    cidmaster->subfonts[0] = sf;
    cidmaster->gpos_lookups = sf->gpos_lookups; sf->gpos_lookups = NULL;
    cidmaster->gsub_lookups = sf->gsub_lookups; sf->gsub_lookups = NULL;
    cidmaster->possub = sf->possub; sf->possub = NULL;
    cidmaster->kerns = sf->kerns; sf->kerns = NULL;
    cidmaster->vkerns = sf->vkerns; sf->vkerns = NULL;
    if ( sf->private==NULL )
	sf->private = gcalloc(1,sizeof(struct psdict));
    if ( !PSDictHasEntry(sf->private,"lenIV"))
	PSDictChangeEntry(sf->private,"lenIV","1");		/* It's 4 by default, in CIDs the convention seems to be 1 */
    for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	free(fvs->selected);
	fvs->selected = gcalloc(fvs->sf->glyphcnt,sizeof(char));
	EncMapFree(fvs->map);
	fvs->map = EncMap1to1(fvs->sf->glyphcnt);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	FVSetTitle(fvs);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
    SFRestoreNearTop(sf);
    CIDMasterAsDes(sf);
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    FontViewReformatAll(sf);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
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

static void BDFOrigFixup(BDFFont *bdf,int orig_cnt,SplineFont *sf) {
    BDFChar **glyphs = gcalloc(orig_cnt,sizeof(BDFChar *));
    int i;

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
    FontView *fvs;

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
		for ( layer=ly_fore; layer<scl->sc->layer_cnt; ++layer )
		    for ( ref = scl->sc->layers[layer].refs; ref!=NULL; ref=ref->next )
			ref->orig_pos = ref->sc->orig_pos;
	    }
	}
	for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) {
	    fvs->map->ticked = false;
	    if ( fvs->filled!=NULL ) fvs->filled->ticked = false;
	}
	for ( fvs=sf->fv; fvs!=NULL; fvs=fvs->nextsame ) if ( !fvs->map->ticked ) {
	    EncMap *map = fvs->map;
	    for ( i=0; i<map->enccount; ++i ) if ( map->map[i]!=-1 )
		map->map[i] = sf->glyphs[map->map[i]]->orig_pos;
	    if ( enc_cnt>map->backmax ) {
		free(map->backmap);
		map->backmax = enc_cnt;
		map->backmap = galloc(enc_cnt*sizeof(int));
	    }
	    memset(map->backmap,-1,enc_cnt*sizeof(int));
	    for ( i=0; i<map->enccount; ++i ) if ( map->map[i]!=-1 )
		if ( map->backmap[map->map[i]]==-1 )
		    map->backmap[map->map[i]] = i;
	    map->ticked = true;
	}
	if ( !old->ticked )
	    IError( "Unticked encmap" );
	for ( bdf=sf->bitmaps; bdf!=NULL; bdf=bdf->next )
	    BDFOrigFixup(bdf,enc_cnt,sf);
	for ( fvs=sf->fv; fvs!=NULL ; fvs=fvs->nextsame ) {
	    if ( fvs->filled!=NULL && !fvs->filled->ticked )
		BDFOrigFixup(sf->fv->filled,enc_cnt,sf);
	}
	glyphs = gcalloc(enc_cnt,sizeof(SplineChar *));
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
	    old->map = grealloc(old->map,enc_cnt*sizeof(int));
	    old->encmax = enc_cnt;
	}
	memset(old->map+old->enccount,-1,(enc_cnt-old->enccount)*sizeof(int));
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
	    for ( layer=ly_fore; layer<scl->sc->layer_cnt; ++layer )
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
    int *encoded, *unencoded;
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
    encoded = galloc(base*sizeof(int));
    memset(encoded,-1,base*sizeof(int));
    unencoded = galloc(sf->glyphcnt*sizeof(int));
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
		     sc->unicodeenc<unicode4_size &&
		     (j = EncFromUni(sc->unicodeenc,enc))!= -1 )
		encoded[j] = i;
	    else {
		/* I don't think extras can surpass unmax now, but it doesn't */
		/*  hurt to leave the code (it's from when we encoded duplicates see below) */
		if ( extras>=unmax ) unencoded = grealloc(unencoded,(unmax+=300)*sizeof(int));
		unencoded[extras++] = i;
	    }
	    for ( altuni=sc->altuni; altuni!=NULL; altuni=altuni->next ) {
		if ( altuni->unienc!=-1 &&
			 altuni->unienc<unicode4_size &&
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
	extern const int cns14pua[], amspua[];
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
    map->map = galloc(map->enccount*sizeof(int));
    memcpy(map->map,encoded,base*sizeof(int));
    memcpy(map->map+base,unencoded,extras*sizeof(int));
    map->backmax = sf->glyphcnt;
    map->backmap = galloc(sf->glyphcnt*sizeof(int));
    memset(map->backmap,-1,sf->glyphcnt*sizeof(int));	/* Just in case there are some unencoded glyphs (duplicates perhaps) */
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
    newmap = galloc(inuse*sizeof(int32));
    for ( i=inuse=0; i<map->enccount ; ++i )
	if ( (gid = map->map[i])!=-1 && SCWorthOutputting(sf->glyphs[gid]))
	    newmap[inuse++] = gid;
    free(map->map);
    map->map = newmap;
    map->enccount = inuse;
    map->encmax = inuse;
    map->enc = &custom;
    memset(map->backmap,-1,sf->glyphcnt*sizeof(int));
    for ( i=inuse-1; i>=0; --i )
	if ( (gid=map->map[i])!=-1 )
	    map->backmap[gid] = i;
return( map );
}

void SFRemoveGlyph(SplineFont *sf,SplineChar *sc, int *flags) {
    struct splinecharlist *dep, *dnext;
    BDFFont *bdf;
    BDFChar *bfc;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    CharView *cv, *next;
    BitmapView *bv, *bvnext;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    RefChar *refs, *rnext;
    FontView *fvs;
    KernPair *kp, *kprev;
    int i;

    if ( sc==NULL )
return;

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    /* Close any open windows */
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    for ( bdf=sf->bitmaps; bdf!=NULL; bdf = bdf->next ) {
	if ( sc->orig_pos<bdf->glyphcnt && (bfc = bdf->glyphs[sc->orig_pos])!= NULL ) {
	    bdf->glyphs[sc->orig_pos] = NULL;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	    BDFCharFree(bfc);
	}
    }

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    /* Turn any searcher references to this glyph into inline copies of it */
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
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

    /* Turn any references to this glyph into inline copies of it */
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

    for ( refs=sc->layers[ly_fore].refs; refs!=NULL; refs = rnext ) {
	rnext = refs->next;
	SCRemoveDependent(sc,refs);
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
    SplineCharFree(sc);
}

void FVAddEncodingSlot(FontView *fv,int gid) {
    EncMap *map = fv->map;
    int enc;

    if ( map->enccount>=map->encmax )
	map->map = grealloc(map->map,(map->encmax+=10)*sizeof(int));
    enc = map->enccount++;
    map->map[enc] = gid;
    map->backmap[gid] = enc;

    fv->selected = grealloc(fv->selected,map->enccount);
    fv->selected[enc] = 0;
    if ( fv->colcnt!=0 ) {		/* Ie. scripting vs. UI */
	fv->rowltot = (enc+1+fv->colcnt-1)/fv->colcnt;
	GScrollBarSetBounds(fv->vsb,0,fv->rowltot,fv->rowcnt);
    }
}

void SFAddEncodingSlot(SplineFont *sf,int gid) {
    FontView *fv;
    for ( fv=sf->fv; fv!=NULL; fv = fv->nextsame )
	FVAddEncodingSlot(fv,gid);
}

static int MapAddEnc(SplineFont *sf,SplineChar *sc,EncMap *basemap, EncMap *map,int baseenc, int gid, FontView *fv) {
    int any = false, enc;

    if ( gid>=map->backmax ) {
	map->backmap = grealloc(map->backmap,(map->backmax+=10)*sizeof(int));
	memset(map->backmap+map->backmax-10,-1,10*sizeof(int));
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
	if ( baseenc>=map->enccount )
	    FVAddEncodingSlot(fv,gid);
	else {
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
    FontView *fv;
    BDFFont *bdf;

    if ( sf->cidmaster==NULL ) {
	if ( sf->glyphcnt+1>=sf->glyphmax )
	    sf->glyphs = grealloc(sf->glyphs,(sf->glyphmax+=10)*sizeof(SplineChar *));
	gid = sf->glyphcnt++;
	for ( bdf = sf->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	    if ( sf->glyphcnt+1>=bdf->glyphmax )
		bdf->glyphs = grealloc(bdf->glyphs,(bdf->glyphmax=sf->glyphmax)*sizeof(BDFChar *));
	    if ( sf->glyphcnt>bdf->glyphcnt ) {
		memset(bdf->glyphs+bdf->glyphcnt,0,(sf->glyphcnt-bdf->glyphcnt)*sizeof(BDFChar *));
		bdf->glyphcnt = sf->glyphcnt;
	    }
	}
	for ( fv=sf->fv; fv!=NULL; fv = fv->nextsame ) {
	    EncMap *map = fv->map;
	    if ( gid>=map->backmax )
		map->backmap = grealloc(map->backmap,(map->backmax=gid+10)*sizeof(int));
	    map->backmap[gid] = -1;
	}
    } else {
	gid = baseenc;
	if ( baseenc+1>=sf->glyphmax )
	    sf->glyphs = grealloc(sf->glyphs,(sf->glyphmax = baseenc+10)*sizeof(SplineChar *));
	if ( baseenc>=sf->glyphcnt ) {
	    memset(sf->glyphs+sf->glyphcnt,0,(baseenc+1-sf->glyphcnt)*sizeof(SplineChar *));
	    sf->glyphcnt = baseenc+1;
	    for ( bdf = sf->cidmaster->bitmaps; bdf!=NULL; bdf=bdf->next ) {
		if ( baseenc+1>=bdf->glyphmax )
		    bdf->glyphs = grealloc(bdf->glyphs,(bdf->glyphmax=baseenc+10)*sizeof(BDFChar *));
		if ( baseenc+1>bdf->glyphcnt ) {
		    memset(bdf->glyphs+bdf->glyphcnt,0,(baseenc+1-bdf->glyphcnt)*sizeof(BDFChar *));
		    bdf->glyphcnt = baseenc+1;
		}
	    }
	    for ( fv=sf->fv; fv!=NULL; fv = fv->nextsame ) if ( fv->sf==sf ) {
		EncMap *map = fv->map;
		if ( gid>=map->backmax )
		    map->backmap = grealloc(map->backmap,(map->backmax=gid+10)*sizeof(int));
		map->backmap[gid] = -1;
	    }
	}
    }
    sf->glyphs[gid] = NULL;
    for ( fv=sf->fv; fv!=NULL; fv = fv->nextsame ) {
	EncMap *map = fv->map;
	BDFFont *bdf = fv->filled;

	if ( bdf!=NULL ) {
	    if ( gid>=bdf->glyphmax )
		bdf->glyphs = grealloc(bdf->glyphs,sf->glyphmax*sizeof(BDFChar *));
	    if ( gid>=bdf->glyphcnt ) {
		memset(bdf->glyphs+bdf->glyphcnt,0,(gid+1-bdf->glyphcnt)*sizeof(BDFChar *));
		bdf->glyphcnt = gid+1;
	    }
	}
	if ( !MapAddEnc(sf,sc,basemap,map,baseenc,gid,fv) )
	    FVAddEncodingSlot(fv,gid);
	if ( map==basemap ) mapfound = true;
    }
    if ( !mapfound && basemap!=NULL )
	MapAddEnc(sf,sc,basemap,basemap,baseenc,gid,fv);
    sf->glyphs[gid] = sc;
    sc->orig_pos = gid;
    sc->parent = sf;
    SFHashGlyph(sf,sc);
}

static SplineChar *SplineCharMatch(SplineFont *parent,SplineChar *sc) {
    SplineChar *scnew = SplineCharCreate();

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
    glyphs = gcalloc(cnt,sizeof(SplineChar *));
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
	glyphs = grealloc(glyphs,(target->glyphcnt+cnt2)*sizeof(SplineChar *));
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
	glyphs = gcalloc(sf->glyphcnt,sizeof(BDFChar *));
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
			base->glyphs = grealloc(base->glyphs,(base->glyphmax+=20)*sizeof(SplineChar *));
		    if ( lasthole>=base->glyphcnt )
			base->glyphcnt = lasthole+1;
		}
		base->glyphs[index] = scnew = SplineCharMatch(base,sc);
		scnew->orig_pos = index;
	    }
	}
    }

    /* Now force all other instances to match */
    for ( i = 0; i<mm->instance_count; ++i ) if ( (sf=mm->instances[i])!=NULL && sf!=NULL )
	SFMatchGlyphs(sf,base,true);
    if ( mm->normal!=NULL )
	SFMatchGlyphs(mm->normal,base,true);
}

GTextInfo encodingtypes[] = {
    { (unichar_t *) N_("Custom"), NULL, 0, 0, (void *) "Custom", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Encoding|Glyph Order"), NULL, 0, 0, (void *) "Original", NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) N_("ISO 8859-1  (Latin1)"), NULL, 0, 0, (void *) "iso8859-1", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-15  (Latin0)"), NULL, 0, 0, (void *) "iso8859-15", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-2  (Latin2)"), NULL, 0, 0, (void *) "iso8859-2", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-3  (Latin3)"), NULL, 0, 0, (void *) "iso8859-3", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-4  (Latin4)"), NULL, 0, 0, (void *) "iso8859-4", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-9  (Latin5)"), NULL, 0, 0, (void *) "iso8859-9", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-10  (Latin6)"), NULL, 0, 0, (void *) "iso8859-10", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-13  (Latin7)"), NULL, 0, 0, (void *) "iso8859-13", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-14  (Latin8)"), NULL, 0, 0, (void *) "iso8859-14", NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) N_("ISO 8859-5 (Cyrillic)"), NULL, 0, 0, (void *) "iso8859-5", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("KOI8-R (Cyrillic)"), NULL, 0, 0, (void *) "koi8-r", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-6 (Arabic)"), NULL, 0, 0, (void *) "iso8859-6", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-7 (Greek)"), NULL, 0, 0, (void *) "iso8859-7", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-8 (Hebrew)"), NULL, 0, 0, (void *) "iso8859-8", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 8859-11 (Thai)"), NULL, 0, 0, (void *) "iso8859-11", NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) N_("Macintosh Latin"), NULL, 0, 0, (void *) "mac", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Windows Latin (\"ANSI\")"), NULL, 0, 0, (void *) "win", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Adobe Standard"), NULL, 0, 0, (void *) "AdobeStandard", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Symbol"), NULL, 0, 0, (void *) "Symbol", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) NU_("ΤεΧ Base (8r)"), NULL, 0, 0, (void *) "TeX-Base-Encoding", NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) N_("ISO 10646-1 (Unicode, BMP)"), NULL, 0, 0, (void *) "UnicodeBmp", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("ISO 10646-1 (Unicode, Full)"), NULL, 0, 0, (void *) "UnicodeFull", NULL, 0, 0, 0, 0, 0, 0, 1},
    /*{ (unichar_t *) N_("ISO 10646-? (by plane) ..."), NULL, 0, 0, (void *) em_unicodeplanes, NULL, 0, 0, 0, 0, 0, 0, 1}*/
    { NULL, NULL, 0, 0, NULL, NULL, 1, 0, 0, 0, 0, 1, 0 },
    { (unichar_t *) N_("SJIS (Kanji)"), NULL, 0, 0, (void *) "sjis", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("JIS 208 (Kanji)"), NULL, 0, 0, (void *) "jis208", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("JIS 212 (Kanji)"), NULL, 0, 0, (void *) "jis212", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Wansung (Korean)"), NULL, 0, 0, (void *) "wansung", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("KSC 5601-1987 (Korean)"), NULL, 0, 0, (void *) "ksc5601", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Johab (Korean)"), NULL, 0, 0, (void *) "johab", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("GB 2312 (Simp. Chinese)"), NULL, 0, 0, (void *) "gb2312", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("EUC GB 2312 (Chinese)"), NULL, 0, 0, (void *) "gb2312pk", NULL, 0, 0, 0, 0, 0, 0, 1},
#ifdef FONTFORGE_CONFIG_GB12345
    { (unichar_t *) N_("EUC-GB12345"), NULL, 0, 0, (void *) "GB12345-EUC", NULL, 0, 0, 0, 0, 0, 0, 1, 0},
#endif
    { (unichar_t *) N_("Big5 (Trad. Chinese)"), NULL, 0, 0, (void *) "big5", NULL, 0, 0, 0, 0, 0, 0, 1},
    { (unichar_t *) N_("Big5 HKSCS (Trad. Chinese)"), NULL, 0, 0, (void *) "big5hkscs", NULL, 0, 0, 0, 0, 0, 0, 1},
    { NULL }};

static void EncodingInit(void) {
    int i;
    static int done = false;

    if ( done )
return;
    done = true;
    for ( i=0; i<sizeof(encodingtypes)/sizeof(encodingtypes[0])-1; ++i ) {
	if ( !encodingtypes[i].line )
	    encodingtypes[i].text = (unichar_t *) S_((char *) encodingtypes[i].text);
    }
}

GMenuItem *GetEncodingMenu(void (*func)(GWindow,GMenuItem *,GEvent *),
	Encoding *current) {
    GMenuItem *mi;
    int i, cnt;
    Encoding *item;

    EncodingInit();

    cnt = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->hidden )
	    ++cnt;
    i = cnt+1;
    i += sizeof(encodingtypes)/sizeof(encodingtypes[0]);
    mi = gcalloc(i+1,sizeof(GMenuItem));
    for ( i=0; i<sizeof(encodingtypes)/sizeof(encodingtypes[0])-1; ++i ) {
	mi[i].ti = encodingtypes[i];
	if ( !mi[i].ti.line ) {
	    mi[i].ti.text = utf82u_copy((char *) (mi[i].ti.text));
	    mi[i].ti.checkable = true;
	    if ( strmatch(mi[i].ti.userdata,current->enc_name)==0 ||
		    (current->iconv_name!=NULL && strmatch(mi[i].ti.userdata,current->iconv_name)==0))
		mi[i].ti.checked = true;
	}
	mi[i].ti.text_is_1byte = false;
	mi[i].ti.fg = mi[i].ti.bg = COLOR_DEFAULT;
	mi[i].invoke = func;
    }
    if ( cnt!=0 ) {
	mi[i].ti.fg = mi[i].ti.bg = COLOR_DEFAULT;
	mi[i++].ti.line = true;
	for ( item=enclist; item!=NULL ; item=item->next )
	    if ( !item->hidden ) {
		mi[i].ti.text = utf82u_copy(item->enc_name);
		mi[i].ti.userdata = (void *) item->enc_name;
		mi[i].ti.fg = mi[i].ti.bg = COLOR_DEFAULT;
		mi[i].ti.checkable = true;
		if ( item==current )
		    mi[i].ti.checked = true;
		mi[i++].invoke = func;
	    }
    }
return( mi );
}

GTextInfo *GetEncodingTypes(void) {
    GTextInfo *ti;
    int i, cnt;
    Encoding *item;

    EncodingInit();

    cnt = 0;
    for ( item=enclist; item!=NULL ; item=item->next )
	if ( !item->hidden )
	    ++cnt;
    i = cnt + sizeof(encodingtypes)/sizeof(encodingtypes[0]);
    ti = gcalloc(i+1,sizeof(GTextInfo));
    memcpy(ti,encodingtypes,sizeof(encodingtypes)-sizeof(encodingtypes[0]));
    for ( i=0; i<sizeof(encodingtypes)/sizeof(encodingtypes[0])-1; ++i ) {
	ti[i].text = (unichar_t *) copy((char *) ti[i].text);
    }
    if ( cnt!=0 ) {
	ti[i++].line = true;
	for ( item=enclist; item!=NULL ; item=item->next )
	    if ( !item->hidden ) {
		ti[i].text = uc_copy(item->enc_name);
		ti[i++].userdata = (void *) item->enc_name;
	    }
    }
return( ti );
}

GTextInfo *EncodingTypesFindEnc(GTextInfo *encodingtypes, Encoding *enc) {
    int i;
    char *name;
    Encoding *new_enc;

    for ( i=0; encodingtypes[i].text!=NULL || encodingtypes[i].line; ++i ) {
	if ( encodingtypes[i].text==NULL )
    continue;
	name = encodingtypes[i].userdata;
	new_enc = FindOrMakeEncoding(name);
	if ( new_enc==NULL )
    continue;
	if ( enc==new_enc )
return( &encodingtypes[i] );
    }
return( NULL );
}

Encoding *ParseEncodingNameFromList(GGadget *listfield) {
    const unichar_t *name = _GGadgetGetTitle(listfield);
    int32 len;
    GTextInfo **ti = GGadgetGetList(listfield,&len);
    int i;
    Encoding *enc = NULL;

    for ( i=0; i<len; ++i ) if ( ti[i]->text!=NULL ) {
	if ( u_strcmp(name,ti[i]->text)==0 ) {
	    enc = FindOrMakeEncoding(ti[i]->userdata);
    break;
	}
    }

    if ( enc == NULL ) {
	char *temp = u2utf8_copy(name);
	enc = FindOrMakeEncoding(temp);
	free(temp);
    }
    if ( enc==NULL )
	gwwv_post_error(_("Bad Encoding"),_("Bad Encoding"));
return( enc );
}

void SFExpandGlyphCount(SplineFont *sf, int newcnt) {
    int old = sf->glyphcnt;
    FontView *fv;

    if ( old>=newcnt )
return;
    if ( sf->glyphmax<newcnt ) {
	sf->glyphs = grealloc(sf->glyphs,newcnt*sizeof(SplineChar *));
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
		fv->map->backmap = grealloc(fv->map->backmap,(fv->map->backmax = newcnt+5)*sizeof(int32));
	    memset(fv->map->backmap+old,-1,(newcnt-old)*sizeof(int32));
	}
    }
}
	    

    
    
