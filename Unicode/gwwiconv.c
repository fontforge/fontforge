/* Copyright (C) 2004-2006 by George Williams */
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

#include <basics.h>
#include <gwwiconv.h>
#include <charset.h>
#include <chardata.h>
#include <string.h>
#include <ustring.h>
#include <stdio.h>

#ifndef HAVE_ICONV_H

/* I have written an limited iconv which will convert either to or from UCS2 */
/*  it will not convert latin1 to latin2, but latin1->UCS2, UCS2->latin2 */
/*  it uses the encodings built into libgunicode for systems with no iconv */
/*  (ie. macs before 10.3, perhaps others) */

struct gww_iconv_t {
    enum encoding from;
    enum encoding to;
};

enum extended_encoding { e_jisgbpk = e_encodingmax };

static enum endian { end_big, end_little, end_unknown } endian = end_unknown;

static void endian_detector(void) {
    union { short s; char c[2]; } u;

    u.s = 0x0102;
    if ( u.c[0]==0x1 )
	endian = end_big;
    else
	endian = end_little;
}

static enum encoding name_to_enc(const char *encname) {
    struct { const char *name; enum encoding enc; } map[] = {
	{ "UCS2", e_unicode },
	{ "UCS-2", e_unicode },
	{ "UCS-2-INTERNAL", e_unicode },
	{ "ISO-10646/UCS2", e_unicode },
	{ "ISO-10646/USC2", e_unicode },		/* Old typo */
	{ "iso8859-1", e_iso8859_1 },
	{ "iso8859-2", e_iso8859_2 },
	{ "iso8859-3", e_iso8859_3 },
	{ "iso8859-4", e_iso8859_4 },
	{ "iso8859-5", e_iso8859_5 },
	{ "iso8859-6", e_iso8859_6 },
	{ "iso8859-7", e_iso8859_7 },
	{ "iso8859-8", e_iso8859_8 },
	{ "iso8859-9", e_iso8859_9 },
	{ "iso8859-10", e_iso8859_10 },
	{ "iso8859-11", e_iso8859_11 },
	{ "iso8859-13", e_iso8859_13 },
	{ "iso8859-14", e_iso8859_14 },
	{ "iso8859-15", e_iso8859_15 },
	{ "iso-8859-1", e_iso8859_1 },
	{ "iso-8859-2", e_iso8859_2 },
	{ "iso-8859-3", e_iso8859_3 },
	{ "iso-8859-4", e_iso8859_4 },
	{ "iso-8859-5", e_iso8859_5 },
	{ "iso-8859-6", e_iso8859_6 },
	{ "iso-8859-7", e_iso8859_7 },
	{ "iso-8859-8", e_iso8859_8 },
	{ "iso-8859-9", e_iso8859_9 },
	{ "iso-8859-10", e_iso8859_10 },
	{ "iso-8859-11", e_iso8859_11 },
	{ "iso-8859-13", e_iso8859_13 },
	{ "iso-8859-14", e_iso8859_14 },
	{ "iso-8859-15", e_iso8859_15 },
	{ "koi8-r", e_koi8_r },
	{ "jis201", e_jis201 },
	{ "mac", e_mac },
	{ "Macintosh", e_mac },
	{ "MS-ANSI", e_win },
	{ "EUC-KR", e_wansung },
	{ "johab", e_johab },
	{ "ISO-2022-KR", e_jiskorean },
	{ "ISO-2022-CN", e_jisgb },
	{ "EUC-CN", e_jisgbpk },
	{ "big5", e_big5 },
	{ "big5hkscs", e_big5hkscs },
	{ "ISO-2022-JP", e_jis },
	{ "ISO-2022-JP-2", e_jis2 },
	{ "Sjis", e_sjis },
	{ "UTF-8", e_utf8 },
	{ "UTF8", e_utf8 },
	{ NULL }};
    int i;

    for ( i=0; map[i].name!=NULL; ++i )
	if ( strmatch(map[i].name,encname)==0 )
return( map[i].enc );

return( -1 );
}

gww_iconv_t gww_iconv_open(const char *toenc,const char *fromenc) {
    struct gww_iconv_t stuff, *ret;

    if ( endian==end_unknown )
	endian_detector();

    stuff.from = name_to_enc(fromenc);
    stuff.to = name_to_enc(toenc);
    if ( stuff.from==(enum encoding) -1 || stuff.to==(enum encoding) -1 ) {
	/*fprintf( stderr, "Unknown encoding\n" );*/
return( (iconv_t)(-1) );
    } else if ( stuff.from!=e_unicode && stuff.to!=e_unicode ) {
	fprintf( stderr, "Bad call to gww_iconv_open, neither arg is UCS2\n" );
return( (iconv_t)(-1) );
    }

    ret = galloc(sizeof(struct gww_iconv_t));
    *ret = stuff;
return( ret );
}

void gww_iconv_close( gww_iconv_t cd) {
    free(cd);
}

size_t gww_iconv( gww_iconv_t _cd,
	char **inbuf, size_t *inlen,
	char **outbuf, size_t *outlen) {
    struct gww_iconv_t *cd = _cd;
    int char_cnt = 0;
    unsigned char *plane;
    int ch;

    if ( inbuf==NULL || outbuf==NULL || inlen==NULL || outlen==NULL ||
	    *inbuf==NULL || *outbuf==NULL )
return( 0 );	/* Legal, used to reset the state. As we don't do states, irrelevant */

    if ( cd->from<0 || cd->from>e_encodingmax || cd->to<0 || cd->to>e_encodingmax ) {
	fprintf( stderr, "Garbage encoding passed to gww_iconv()\n" );
return( (size_t) -1 );
    }

    if ( cd->from==e_unicode ) {
	if ( cd->to==e_unicode ) {
	    int min = *inlen < *outlen ? *inlen : *outlen;
	    min &= ~1;
	    memcpy(*inbuf,*outbuf,min);
	    char_cnt = min/2;
	    *inbuf += min; *outbuf += min;
	    *inlen -= min; *outlen -= min;
	    if ( *inlen==1 && *outlen>0 )
return( (size_t) -1 );			/* Incomplete multi-byte sequence */
	} else if ( cd->to<e_first2byte ) {
	    struct charmap *table = NULL;
	    table = alphabets_from_unicode[cd->to];
	    while ( *inlen>1 && *outlen>0 ) {
		int highch, lowch;
		if ( endian == end_little ) {
		    highch = ((unsigned char *) *inbuf)[1], lowch = *(unsigned char *) *inbuf;
		} else {
		    highch = *(unsigned char *) *inbuf, lowch = ((unsigned char *) *inbuf)[1];
		}
		if ( highch>=table->first && highch<=table->last &&
			(plane = table->table[highch])!=NULL &&
			(ch=plane[lowch])!=0 ) {
		    *((*outbuf)++) = ch;
		    -- *outlen;
		    *inlen -= 2;
		    *inbuf += 2;
		    ++char_cnt;
		} else
return( (size_t) -1 );
	    }
	} else if ( cd->to==e_johab || cd->to==e_big5 || cd->to==e_big5hkscs ) {
	    struct charmap2 *table = cd->to==e_johab ? &johab_from_unicode :
				     cd->to==e_big5  ? &big5_from_unicode :
					&big5hkscs_from_unicode;
	    unichar_t *plane;
	    while ( *inlen>1 && *outlen>0 ) {
		int highch, lowch;
		if ( endian == end_little ) {
		    highch = ((unsigned char *) *inbuf)[1], lowch = *(unsigned char *) *inbuf;
		} else {
		    highch = *(unsigned char *) *inbuf, lowch = ((unsigned char *) *inbuf)[1];
		}
		if ( highch==0 && lowch<=0x80 ) {
		    *((*outbuf)++) = highch;
		    --*outlen;
		    *inlen-=2;
		    *inbuf+=2;
		    ++char_cnt;
		} else if ( *outlen==1 )
return( (size_t) -1 );
		else if ( highch>=table->first && highch<=table->last &&
			(plane = table->table[highch])!=NULL &&
			(ch=plane[lowch])!=0 ) {
		    *((*outbuf)++) = (ch>>8);
		    *((*outbuf)++) = (ch&0xff);
		    *outlen -= 2;
		    *inlen -= 2;
		    *inbuf += 2;
		    ++char_cnt;
		} else
return( (size_t) -1 );
	    }
	} else if ( cd->to==e_wansung || cd->to==e_jisgbpk ) {
	    struct charmap2 *table = cd->to==e_wansung ? &ksc5601_from_unicode :
			&gb2312_from_unicode;
	    unichar_t *plane;
	    while ( *inlen>1 && *outlen>0 ) {
		int highch, lowch;
		if ( endian == end_little ) {
		    highch = ((unsigned char *) *inbuf)[1], lowch = *(unsigned char *) *inbuf;
		} else {
		    highch = *(unsigned char *) *inbuf, lowch = ((unsigned char *) *inbuf)[1];
		}
		if ( highch==0 && lowch<=0x80 ) {
		    *((*outbuf)++) = lowch;
		    --*outlen;
		    *inlen-=2;
		    *inbuf+=2;
		    ++char_cnt;
		} else if ( *outlen==1 )
return( (size_t) -1 );
		else if ( highch>=table->first && highch<=table->last &&
			(plane = table->table[highch])!=NULL &&
			(ch=plane[lowch])!=0 ) {
		    *((*outbuf)++) = (ch>>8)+0x80;
		    *((*outbuf)++) = (ch&0xff)+0x80;
		    *outlen -= 2;
		    *inlen -= 2;
		    *inbuf += 2;
		    ++char_cnt;
		} else
return( (size_t) -1 );
	    }
	} else if ( cd->to==e_sjis ) {
	    unsigned char *plane1;
	    unichar_t *plane;
	    while ( *inlen>1 && *outlen>0 ) {
		int highch, lowch;
		if ( endian == end_little ) {
		    highch = ((unsigned char *) *inbuf)[1], lowch = *(unsigned char *) *inbuf;
		} else {
		    highch = *(unsigned char *) *inbuf, lowch = ((unsigned char *) *inbuf)[1];
		}
		if (( highch>=jis201_from_unicode.first && highch<=jis201_from_unicode.last &&
			(plane1 = jis201_from_unicode.table[highch-jis201_from_unicode.first])!=NULL &&
			(ch=plane1[lowch])!=0 ) ||
		       ( highch==0 && (ch=lowch)<' ' )) { /* control chars not mapped in jis201 */
		    *((*outbuf)++) = ch;
		    --*outlen;
		    *inlen-=2;
		    *inbuf+=2;
		    ++char_cnt;
		} else if ( *outlen==1 )
return( (size_t) -1 );
		else if ( highch>=jis_from_unicode.first && highch<=jis_from_unicode.last &&
			(plane = jis_from_unicode.table[highch-jis_from_unicode.first])!=NULL &&
			(ch=plane[lowch])!=0 && ch<0x8000 ) {	/* no jis212 */
		    int j1 = ch>>8, j2 = ch&0xff;
		    int ro = j1<95 ? 112 : 176;
		    int co = (j1&1) ? (j2>95?32:31) : 126;
		    *((*outbuf)++) = ((j1+1)>>1)+ro;
		    *((*outbuf)++) = j2+co;
		    *outlen -= 2;
		    *inlen -= 2;
		    *inbuf += 2;
		    ++char_cnt;
		} else
return( (size_t) -1 );
	    }
	} else if ( cd->to==e_jis || cd->to==e_jis2 ||
		cd->to==e_jiskorean || cd->to==e_jisgb ) {
	    struct charmap2 *table = cd->to==e_jisgb     ? &gb2312_from_unicode :
				     cd->to==e_jiskorean ? &ksc5601_from_unicode :
					&jis_from_unicode;
	    unichar_t *plane;
	    while ( *inlen>1 && *outlen>1 ) {
		int highch, lowch;
		if ( endian == end_little ) {
		    highch = ((unsigned char *) *inbuf)[1], lowch = *(unsigned char *) *inbuf;
		} else {
		    highch = *(unsigned char *) *inbuf, lowch = ((unsigned char *) *inbuf)[1];
		}
		if ( highch>=table->first && highch<=table->last &&
			(plane = table->table[highch])!=NULL &&
			(ch=plane[lowch])!=0 ) {
		    if ( ch>=0x8000 ) {
			if ( cd->to!=e_jis2 )
return( (size_t) -1 );
			ch -= 0x8000;
		    } else {
			if ( cd->to==e_jis2 )
return( (size_t) -1 );
		    }
		    *((*outbuf)++) = (ch>>8);
		    *((*outbuf)++) = (ch&0xff);
		    *outlen -= 2;
		    *inlen -= 2;
		    *inbuf += 2;
		    ++char_cnt;
		} else
return( (size_t) -1 );
	    }
	} else if ( cd->to==e_utf8 ) {
	    while ( *inlen>1 && *outlen>0 ) {
		unichar_t uch;
		if ( endian == end_little ) {
		    uch = (((unsigned char *) *inbuf)[1]<<8) | (*((unsigned char *) *inbuf));
		} else {
		    uch = (*((unsigned char *) *inbuf)<<8) | (((unsigned char *) *inbuf)[1]);
		}
		if ( uch < 0x80 ) {
		    *((*outbuf)++) = uch;
		    --*outlen;
		} else if ( uch<0x800 ) {
		    if ( *outlen==1 )
return( (size_t) -1 );
		    *((*outbuf)++) = 0xc0 | (uch>>6);
		    *((*outbuf)++) = 0x80 | (uch&0x3f);
		    *outlen-=2;
		} else {	/* I'm not dealing with */
		    if ( *outlen<=2 )
return( (size_t) -1 );
		    *((*outbuf)++) = 0xe0 | (uch>>12);
		    *((*outbuf)++) = 0x80 | ((uch>>6)&0x3f);
		    *((*outbuf)++) = 0x80 | (uch&0x3f);
		    *outlen-=3;
		}
		*inbuf += 2;
		*inlen -= 2;
		++char_cnt;
	    }
	} else {
	    fprintf( stderr, "Unexpected encoding\n" );
return( (size_t) -1 );
	}
    } else if ( cd->to==e_unicode ) {
	const unichar_t *table;
	if ( cd->from<e_first2byte ) {
	    table = unicode_from_alphabets[cd->from];
	    while ( *inlen>0 && *outlen>1 ) {
		unichar_t ch = table[ *(unsigned char *) ((*inbuf)++)];
		--*inlen;
		if ( endian==end_little ) {
		    *((*outbuf)++) = ch&0xff;
		    *((*outbuf)++) = ch>>8;
		} else {
		    *((*outbuf)++) = ch>>8;
		    *((*outbuf)++) = ch&0xff;
		}
		*outlen -= 2;
		++char_cnt;
	    }
	} else if ( cd->from==e_jis || cd->from==e_jis2 ||
		cd->from==e_jiskorean || cd->from==e_jisgb ) {
	    table  = cd->from==e_jisgb     ? unicode_from_gb2312 :
		     cd->from==e_jiskorean ? unicode_from_ksc5601 :
		     cd->from==e_jis       ? unicode_from_jis208 :
		        unicode_from_jis212;
	    while ( *inlen>1 && *outlen>1 ) {
		unsigned char *ipt = (unsigned char *) *inbuf;
		int ch;
		if ( *ipt<0x21 || *ipt>0x7e || ipt[1]<0x21 || ipt[1]>0x7e )
return( (size_t) -1 );
		ch = (*ipt-0x21)*94 + (ipt[1]-0x21);
		ch = table[ch];
		*inlen -= 2;
		*inbuf = (char *) ipt+2;
		if ( endian==end_little ) {
		    *((*outbuf)++) = ch&0xff;
		    *((*outbuf)++) = ch>>8;
		} else {
		    *((*outbuf)++) = ch>>8;
		    *((*outbuf)++) = ch&0xff;
		}
		*outlen -= 2;
		++char_cnt;
	    }
	    if ( *inlen==1 && *outlen>0 )
return( (size_t) -1 );			/* Incomplete multi-byte sequence */
	} else if ( cd->from==e_wansung || cd->from==e_jisgbpk ) {
	    table  = cd->from==e_jisgbpk   ? unicode_from_gb2312 :
		      unicode_from_ksc5601 ;
	    while ( *inlen>0 && *outlen>1 ) {
		unsigned char *ipt = (unsigned char *) *inbuf;
		int ch;
		if ( *ipt<0x7f ) {
		    ch = *ipt;
		    --*inlen;
		    *inbuf = (char *) ipt+1;
		} else {
		    if ( *ipt<0xa1 || *ipt>0xfe || ipt[1]<0xa1 || ipt[1]>0xfe ||
			    *inlen==1 )
return( (size_t) -1 );
		    ch = (*ipt-0xa1)*94 + (ipt[1]-0xa1);
		    ch = table[ch];
		    *inlen -= 2;;
		    *inbuf = (char *) ipt+2;
		}
		if ( endian==end_little ) {
		    *((*outbuf)++) = ch&0xff;
		    *((*outbuf)++) = ch>>8;
		} else {
		    *((*outbuf)++) = ch>>8;
		    *((*outbuf)++) = ch&0xff;
		}
		*outlen -= 2;
		++char_cnt;
	    }
	} else if ( cd->from==e_johab || cd->from==e_big5 || cd->from==e_big5hkscs ) {
	    int offset;
	    if ( cd->from==e_big5 ) {
		offset = 0xa100;
		table = unicode_from_big5;
	    } else if ( cd->from==e_big5hkscs ) {
		offset = 0x8100;
		table = unicode_from_big5hkscs;
	    } else {
		offset = 0x8400;
		table = unicode_from_johab;
	    }
	    while ( *inlen>0 && *outlen>1 ) {
		unsigned char *ipt = (unsigned char *) *inbuf;
		int ch;
		if ( *ipt<0x7f ) {
		    ch = *ipt;
		    --*inlen;
		    *inbuf = ipt+1;
		} else {
		    if ( *inlen==1 )
return( (size_t) -1 );
		    ch = (*ipt<<8) | ipt[1];
		    if ( ch<offset )
return( (size_t) -1 );
		    ch -= offset;
		    ch = table[ch];
		    *inlen -= 2;
		    *inbuf = (char *) ipt+2;
		}
		if ( endian==end_little ) {
		    *((*outbuf)++) = ch&0xff;
		    *((*outbuf)++) = ch>>8;
		} else {
		    *((*outbuf)++) = ch>>8;
		    *((*outbuf)++) = ch&0xff;
		}
		*outlen -= 2;
		++char_cnt;
	    }
	} else if ( cd->from==e_sjis ) {
	    while ( *inlen>0 && *outlen>1 ) {
		unsigned char *ipt = (unsigned char *) *inbuf;
		int ch1 = *ipt;
		if ( ch1<127 || ( ch1>=161 && ch1<=223 )) {
		    ch = unicode_from_jis201[ch1];
		    *inbuf = (char *) ipt+1;
		    --*inlen;
		} else if ( *inlen==1 )
return( (size_t) -1 );
		else {
		    int ch2 = ipt[1];
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
		    if ( ch1-0x21>=94 || ch2-0x21>=94 )
return( (size_t) -1 );
		    ch = unicode_from_jis208[(ch1-0x21)*94+(ch2-0x21)];
		    *inlen -= 2;
		    *inbuf = (char *) ipt+2;
		}
		if ( endian==end_little ) {
		    *((*outbuf)++) = ch&0xff;
		    *((*outbuf)++) = ch>>8;
		} else {
		    *((*outbuf)++) = ch>>8;
		    *((*outbuf)++) = ch&0xff;
		}
		*outlen -= 2;
		++char_cnt;
	    }
	} else if ( cd->from==e_utf8 ) {
	    while ( *inlen>0 && *outlen>1 ) {
		unsigned char *ipt = (unsigned char *) *inbuf;
		int ch = *ipt;
		if ( ch <= 127 ) {
		    *inbuf = (char *) ipt+1;
		    --*inlen;
		} else if ( ch<=0xdf ) {
		    if ( *inlen<2 || ipt[1]<0x80 )
return( (size_t) -1 );
		    ch = ((ch&0x1f)<<6) | (ipt[1] &0x3f);
		    *inlen -= 2;
		    *inbuf = (char *) ipt+2;
		} else if ( ch<=0xef ) {
		    if ( *inlen<3 || ipt[1]<0x80 || ipt[2]<0x80 )
return( (size_t) -1 );
		    ch = ((ch&0x1f)<<12) | ((ipt[1] &0x3f)<<6) | (ipt[2]&0x3f);
		    *inlen -= 3;
		    *inbuf = (char *) ipt+3;
		} else {
		    int w;
		    if ( *inlen<4 || *outlen<4 || ipt[1]<0x80 || ipt[2]<0x80 || ipt[3]<0x80 )
return( (size_t) -1 );
		    w = ( ((ch&0x7)<<2) | ((ipt[1]&0x30)>>4) )-1;
		    ch = 0xd800 | (w<<6) | ((ipt[1]&0xf)<<2) | ((ipt[2]&0x30)>>4);
		    if ( endian==end_little ) {
			*((*outbuf)++) = ch&0xff;
			*((*outbuf)++) = ch>>8;
		    } else {
			*((*outbuf)++) = ch>>8;
			*((*outbuf)++) = ch&0xff;
		    }
		    *outlen -= 2;
		    ch = 0xdc00 | ((ipt[2]&0xf)<<6) | (ipt[3]&0x3f);
		}
		if ( endian==end_little ) {
		    *((*outbuf)++) = ch&0xff;
		    *((*outbuf)++) = ch>>8;
		} else {
		    *((*outbuf)++) = ch>>8;
		    *((*outbuf)++) = ch&0xff;
		}
		*outlen -= 2;
		++char_cnt;
	    }
	} else {
	    fprintf( stderr, "Unexpected encoding\n" );
return( (size_t) -1 );
	}
    } else {
	fprintf( stderr, "One of the two encodings must be UCS2 in gww_iconv()\n" );
return( (size_t) -1 );
    }

    if ( *outlen>=1 ) {
	**outbuf = '\0';
	if ( *outlen>1 )
	    (*outbuf)[1] = '\0';
    }
return( char_cnt );
}
#else
static const int a_file_must_define_something=1;
#endif 	/* HAVE_ICONV_H */
