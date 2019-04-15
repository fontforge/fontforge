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

#include <fontforge-config.h>		
#include <gwwiconv.h>
#include <stddef.h>
#include <ustring.h>
#include <utype.h>
#include <charset.h>
#include "chardata.h"

int local_encoding = e_iso8859_1;
#if HAVE_ICONV_H
char *iconv_local_encoding_name = NULL;
#endif

static int bad_enc_warn = false;

/* Does not handle conversions to Extended unix */

unichar_t *encoding2u_strncpy(unichar_t *uto, const char *_from, int n, enum encoding cs) {
    unichar_t *upt=uto;
    const unichar_t *table;
    int offset;
    const unsigned char *from = (const unsigned char *) _from;

    if ( cs<e_first2byte ) {
	table = unicode_from_alphabets[cs];
	if ( table==NULL ) {
	    while ( *from && n>0 ) {
		*upt++ = *(unsigned char *) (from++);
		--n;
	    }
	} else {
	    while ( *from && n>0 ) {
		*upt ++ = table[*(unsigned char *) (from++)];
		--n;
	    }
	}
    } else if ( cs<e_unicode ) {
	*uto = '\0';
	switch ( cs ) {
	  default:
	    if ( !bad_enc_warn ) {
		bad_enc_warn = true;
		fprintf( stderr, "Unexpected encoding %d, I'll pretend it's latin1\n", cs );
	    }
return( encoding2u_strncpy(uto,_from,n,e_iso8859_1));
	  case e_johab: case e_big5: case e_big5hkscs:
	    if ( cs==e_big5 ) {
		offset = 0xa100;
		table = unicode_from_big5;
	    } else if ( cs==e_big5hkscs ) {
		offset = 0x8100;
		table = unicode_from_big5hkscs;
	    } else {
		offset = 0x8400;
		table = unicode_from_johab;
	    }
	    while ( *from && n>0 ) {
		if ( *from>=(offset>>8) && from[1]!='\0' ) {
		    *upt++ = table[ ((*from<<8) | from[1]) - offset ];
		    from += 2;
		} else
		    *upt++ = *from++;
		--n;
	    }
	  break;
	  case e_wansung:
	    while ( *from && n>0 ) {
		if ( *from>=0xa1 && from[1]>=0xa1 ) {
		    *upt++ = unicode_from_ksc5601[ (*from-0xa1)*94+(from[1]-0xa1) ];
		    from += 2;
		} else
		    *upt++ = *from++;
		--n;
	    }
	  break;
	  case e_jisgb:
	    while ( *from && n>0 ) {
		if ( *from>=0xa1 && from[1]>=0xa1 ) {
		    *upt++ = unicode_from_gb2312[ (*from-0xa1)*94+(from[1]-0xa1) ];
		    from += 2;
		} else
		    *upt++ = *from++;
		--n;
	    }
	  break;
	  case e_sjis:
	    while ( *from && n>0 ) {
		if ( *from<127 || ( *from>=161 && *from<=223 )) {
		    *upt++ = unicode_from_jis201[*from++];
		} else {
		    int ch1 = *from++;
		    int ch2 = *from++;
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
		    *upt++ = unicode_from_jis208[(ch1-0x21)*94+(ch2-0x21)];
		}
		--n;
	    }
	  break;
	}
    } else if ( cs==e_unicode ) {
	unichar_t *ufrom = (unichar_t *) from;
	while ( *ufrom && n>0 ) {
	    *upt++ = *ufrom++;
	    --n;
	}
    } else if ( cs==e_unicode_backwards ) {
	unichar_t *ufrom = (unichar_t *) from;
	while ( *ufrom && n>0 ) {
	    unichar_t ch = (*ufrom>>8)||((*ufrom&0xff)<<8);
	    *upt++ = ch;
	    ++ufrom;
	    --n;
	}
    } else if ( cs==e_utf8 ) {
	while ( *from && n>0 ) {
	    if ( *from<=127 )
		*upt = *from++;
	    else if ( *from<=0xdf ) {
		if ( from[1]>=0x80 ) {
		    *upt = ((*from&0x1f)<<6) | (from[1]&0x3f);
		    from += 2;
		} else {
		    ++from;	/* Badly formed utf */
		    *upt = 0xfffd;
		}
	    } else if ( *from<=0xef ) {
		if ( from[1]>=0x80 && from[2]>=0x80 ) {
		    *upt = ((*from&0xf)<<12) | ((from[1]&0x3f)<<6) | (from[2]&0x3f);
		    from += 3;
		} else {
		    ++from;	/* Badly formed utf */
		    *upt = 0xfffd;
		}
	    } else if ( n>2 ) {
		if ( from[1]>=0x80 && from[2]>=0x80 && from[3]>=0x80 ) {
		    int w = ( ((*from&0x7)<<2) | ((from[1]&0x30)>>4) )-1;
		    *upt++ = 0xd800 | (w<<6) | ((from[1]&0xf)<<2) | ((from[2]&0x30)>>4);
		    *upt   = 0xdc00 | ((from[2]&0xf)<<6) | (from[3]&0x3f);
		    from += 4;
		} else {
		    ++from;	/* Badly formed utf */
		    *upt = 0xfffd;
		}
	    } else {
		/* no space for surrogate */
		from += 4;
	    }
	    ++upt;
	}
    } else {
	if ( !bad_enc_warn ) {
	    bad_enc_warn = true;
	    fprintf( stderr, "Unexpected encoding %d, I'll pretend it's latin1\n", cs );
	}
return( encoding2u_strncpy(uto,_from,n,e_iso8859_1));
    }

    if ( n>0 )
	*upt = '\0';

return( uto );
}

char *u2encoding_strncpy(char *to, const unichar_t *ufrom, size_t n, enum encoding cs) {
    char *pt = to;

    /* we just ignore anything that doesn't fit in the encoding we look at */
    if ( cs<e_first2byte ) {
	struct charmap *table = NULL;
	unsigned char *plane;
	table = alphabets_from_unicode[cs];
	if ( table==NULL ) {	/* ASCII */
	    while ( *ufrom && n>0 ) {
		int ch = *ufrom;
		if ( ch<127 ) {
		    *pt++ = ch;
		    --n;
		}
		++ufrom;
	    }
	} else {
	    while ( *ufrom && n>0 ) {
		int highch = *ufrom>>8, ch;
		if ( highch>=table->first && highch<=table->last &&
			    (plane = table->table[highch])!=NULL &&
			    (ch=plane[*ufrom&0xff])!=0 ) {
		    *pt++ = ch;
		    --n;
		}
		++ufrom;
	    }
	}
	if ( n>0 )
	    *pt = '\0';
    } else if ( cs<e_unicode ) {
	struct charmap2 *table;
	unsigned short *plane;
	unsigned char *plane1;

	*to = '\0';
	switch ( cs ) {
	  default:
	    if ( !bad_enc_warn ) {
		bad_enc_warn = true;
		fprintf( stderr, "Unexpected encoding %d, I'll pretend it's latin1\n", cs );
	    }
return( u2encoding_strncpy(to,ufrom,n,e_iso8859_1));
	  case e_johab: case e_big5: case e_big5hkscs:
	    table = cs==e_big5 ? &big5_from_unicode :
		    cs==e_big5hkscs ? &big5hkscs_from_unicode :
			    &johab_from_unicode;
	    while ( *ufrom && n>0 ) {
		int highch = *ufrom>>8, ch;
		if ( *ufrom<0x80 ) {
		    *pt++ = *ufrom;
		    --n;
		} else if ( highch>=table->first && highch<=table->last &&
			(plane = table->table[highch-table->first])!=NULL &&
			(ch=plane[*ufrom&0xff])!=0 ) {
		    *pt++ = ch>>8;
		    *pt++ = ch&0xff;
		    n -= 2;
		}
		ufrom ++;
	    }
	  break;
	  case e_wansung:
	    while ( *ufrom && n>0 ) {
		int highch = *ufrom>>8, ch;
		if ( *ufrom<0x80 ) {
		    *pt++ = *ufrom;
		    --n;
		} else if ( highch>=ksc5601_from_unicode.first && highch<=ksc5601_from_unicode.last &&
			(plane = ksc5601_from_unicode.table[highch-ksc5601_from_unicode.first])!=NULL &&
			(ch=plane[*ufrom&0xff])!=0 ) {
		    *pt++ = (ch>>8) + 0x80;
		    *pt++ = (ch&0xff) + 0x80;
		    n -= 2;
		}
		ufrom ++;
	    }
	  break;
	  case e_jisgb:
	    while ( *ufrom && n>0 ) {
		int highch = *ufrom>>8, ch;
		if ( *ufrom<0x80 ) {
		    *pt++ = *ufrom;
		    --n;
		} else if ( highch>=gb2312_from_unicode.first && highch<=gb2312_from_unicode.last &&
			(plane = gb2312_from_unicode.table[highch-gb2312_from_unicode.first])!=NULL &&
			(ch=plane[*ufrom&0xff])!=0 ) {
		    *pt++ = (ch>>8) + 0x80;
		    *pt++ = (ch&0xff) + 0x80;
		    n -= 2;
		}
		ufrom ++;
	    }
	  break;
	  case e_sjis:
	    while ( *ufrom && n>0 ) {
		int highch = *ufrom>>8, ch;
		if ( highch>=jis201_from_unicode.first && highch<=jis201_from_unicode.last &&
			(plane1 = jis201_from_unicode.table[highch-jis201_from_unicode.first])!=NULL &&
			(ch=plane1[*ufrom&0xff])!=0 ) {
		    *pt++ = ch;
		    --n;
		} else if ( *ufrom<' ' ) {	/* control chars */
		    *pt++ = *ufrom;
		    --n;
		} else if ( highch>=jis_from_unicode.first && highch<=jis_from_unicode.last &&
			(plane = jis_from_unicode.table[highch-jis_from_unicode.first])!=NULL &&
			(ch=plane[*ufrom&0xff])!=0 && ch<0x8000 ) {	/* no jis212 */
		    int j1 = ch>>8, j2 = ch&0xff;
		    int ro = j1<95 ? 112 : 176;
		    int co = (j1&1) ? (j2>95?32:31) : 126;
		    *pt++ = ((j1+1)>>1)+ro;
		    *pt++ = j2+co;
		    n -= 2;
		}
		++ufrom;
	    }
	  break;
	}
	if ( n>0 )
	    *pt = '\0';
    } else if ( cs==e_unicode ) {
	unichar_t *uto = (unichar_t *) to;
	while ( *ufrom && n>1 ) {
	    *uto++ = *ufrom++;
	    n-=sizeof(unichar_t);
	}
	if ( n>1 )
	    *uto = '\0';
    } else if ( cs==e_unicode_backwards ) {
	unichar_t *uto = (unichar_t *) to;
	while ( *ufrom && n>sizeof(unichar_t)-1 ) {
	    unichar_t ch = (*ufrom>>24)|((*ufrom>>8)&0xff00)|
		    ((*ufrom<<8)&0xff0000)|(*ufrom<<24);
	    *uto++ = ch;
	    ++ufrom;
	    n-=sizeof(unichar_t);
	}
	if ( n>1 )
	    *uto = '\0';
    } else if ( cs==e_utf8 ) {
	while ( *ufrom ) {
	    if ( *ufrom<0x80 ) {
		if ( n<=1 )
	break;
		*pt++ = *ufrom;
		--n;
	    } else if ( *ufrom<0x800 ) {
		if ( n<=2 )
	break;
		*pt++ = 0xc0 | (*ufrom>>6);
		*pt++ = 0x80 | (*ufrom&0x3f);
		n -= 2;
	    } else if ( *ufrom>=0xd800 && *ufrom<0xdc00 && ufrom[1]>=0xdc00 && ufrom[1]<0xe000 ) {
		int u = ((*ufrom>>6)&0xf)+1, y = ((*ufrom&3)<<4) | ((ufrom[1]>>6)&0xf);
		if ( n<=4 )
	    break;
		*pt++ = 0xf0 | (u>>2);
		*pt++ = 0x80 | ((u&3)<<4) | ((*ufrom>>2)&0xf);
		*pt++ = 0x80 | y;
		*pt++ = 0x80 | (ufrom[1]&0x3f);
		n -= 4;
	    } else {
		if ( n<=3 )
	    break;
		*pt++ = 0xe0 | (*ufrom>>12);
		*pt++ = 0x80 | ((*ufrom>>6)&0x3f);
		*pt++ = 0x80 | (*ufrom&0x3f);
	    }
	    ++ufrom;
	}
	if ( n>1 )
	    *pt = '\0';
    } else {
	if ( !bad_enc_warn ) {
	    bad_enc_warn = true;
	    fprintf( stderr, "Unexpected encoding %d, I'll pretend it's latin1\n", cs );
	}
return( u2encoding_strncpy(to,ufrom,n,e_iso8859_1));
    }

return( to );
}

#if HAVE_ICONV_H
static char *old_local_name=NULL;
static iconv_t to_unicode=(iconv_t) (-1), from_unicode=(iconv_t) (-1);
static iconv_t to_utf8=(iconv_t) (-1), from_utf8=(iconv_t) (-1);
static const char (*names[]) = { "UCS-4-INTERNAL", "UCS-4", "UCS4", "ISO-10646-UCS-4", "UTF-32", NULL };
static const char (*namesle[]) = { "UCS-4LE", "UTF-32LE", NULL };
static const char (*namesbe[]) = { "UCS-4BE", "UTF-32BE", NULL };
static const char *unicode_name = NULL;
static int byteswapped = false;

static int BytesNormal(iconv_t latin1_2_unicode) {
    union {
	int32 s;
	char c[4];
    } u[8];
    const char *from = "A";
    char *to = &u[0].c[0];
    size_t in_left = 1, out_left = sizeof(u);
    memset(u,0,sizeof(u));
    iconv( latin1_2_unicode, (iconv_arg2_t) &from, &in_left, &to, &out_left);
    if ( u[0].s=='A' )
return( true );

return( false );
}

static int my_iconv_setup(void) {
    const char **testnames;
    int i;
    union {
	short s;
	char c[2];
    } u;
    iconv_t test;

    if ( iconv_local_encoding_name==NULL ) {
	if ( to_unicode!=(iconv_t) (-1) ) {
	    iconv_close(to_unicode);
	    iconv_close(from_unicode);
	    to_unicode = from_unicode = (iconv_t) (-1);
	}
return(false);
    }
    if ( old_local_name!=NULL && strcmp(old_local_name,iconv_local_encoding_name)==0 )
return( to_unicode!=(iconv_t) (-1) );

    free(old_local_name);
    old_local_name = copy(iconv_local_encoding_name);
    to_utf8 = iconv_open("UTF-8",iconv_local_encoding_name);
    from_utf8 = iconv_open(iconv_local_encoding_name,"UTF-8");

    if ( unicode_name==NULL ) {
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
		unicode_name = testnames[i];
	break;
	    }
	}
	if ( unicode_name==NULL ) {
	    for ( i=0; names[i]!=NULL; ++i ) {
		test = iconv_open(names[i],"ISO-8859-1");
		if ( test!=(iconv_t) -1 && test!=NULL ) {
		    byteswapped = !BytesNormal(test);
		    iconv_close(test);
		    unicode_name = names[i];
	    break;
		}
	    }
	}
    }
    if ( unicode_name == NULL ) {
	fprintf( stderr, "Could not find a name for Unicode which iconv could understand.\n" );
return( false );
    } else if ( byteswapped ) {
	fprintf( stderr, "The only name for Unicode that iconv understood produced unexpected results.\nPerhaps %s was byte swapped.\n", unicode_name );
return( false );
    }
	
    to_unicode = iconv_open(unicode_name,iconv_local_encoding_name);
    from_unicode = iconv_open(iconv_local_encoding_name,unicode_name);
    if ( to_unicode == (iconv_t) (-1) || to_utf8 == (iconv_t) (-1) ) {
	fprintf( stderr, "iconv failed to understand encoding %s\n",
		iconv_local_encoding_name);
return( false );
    }
return( true );
}
#endif

unichar_t *def2u_strncpy(unichar_t *uto, const char *from, size_t n) {
#if HAVE_ICONV_H
    if ( my_iconv_setup() ) {
	size_t in_left = n, out_left = sizeof(unichar_t)*n;
	char *cto = (char *) uto;
	iconv(to_unicode, (iconv_arg2_t) &from, &in_left, &cto, &out_left);
	if ( cto<((char *) uto)+2*n) *cto++ = '\0';
	if ( cto<((char *) uto)+2*n) *cto++ = '\0';
	if ( cto<((char *) uto)+4*n) *cto++ = '\0';
	if ( cto<((char *) uto)+4*n) *cto++ = '\0';
return( uto );
    }
#endif
return( encoding2u_strncpy(uto,from,n,local_encoding));
}

char *u2def_strncpy(char *to, const unichar_t *ufrom, size_t n) {
#if HAVE_ICONV_H
    if ( my_iconv_setup() ) {
	size_t in_left = sizeof(unichar_t)*n, out_left = n;
	char *cfrom = (char *) ufrom, *cto=to;
	iconv(from_unicode, (iconv_arg2_t) &cfrom, &in_left, &cto, &out_left);
	if ( cto<to+n ) *cto++ = '\0';
	if ( cto<to+n ) *cto++ = '\0';
	if ( cto<to+n ) *cto++ = '\0';
	if ( cto<to+n ) *cto++ = '\0';
return( to );
    }
#endif
return( u2encoding_strncpy(to,ufrom,n,local_encoding));
}

unichar_t *def2u_copy(const char *from) {
    int len;
    unichar_t *uto, *ret;

    if ( from==NULL ) return( NULL );
    len = strlen(from);
    uto = (unichar_t *) malloc((len+1)*sizeof(unichar_t));
    if ( uto==NULL ) return( NULL );
#if HAVE_ICONV_H
    if ( my_iconv_setup() ) {
	size_t in_left = len, out_left = sizeof(unichar_t)*len;
	char *cto = (char *) uto;
	iconv(to_unicode, (iconv_arg2_t) &from, &in_left, &cto, &out_left);
	*cto++ = '\0';
	*cto++ = '\0';
	*cto++ = '\0';
	*cto++ = '\0';
	return( uto );
    }
#endif
    ret = encoding2u_strncpy(uto,from,len,local_encoding);
    if ( ret==NULL )
	free( uto );
    else
	uto[len] = '\0';
    return( ret );
}

char *u2def_copy(const unichar_t *ufrom) {
    int len;
    char *to, *ret;

    if ( ufrom==NULL ) return( NULL );
    len = u_strlen(ufrom);
#if HAVE_ICONV_H
    if ( my_iconv_setup() ) {
	size_t in_left = sizeof(unichar_t)*len, out_left = 3*len;
	char *cfrom = (char *) ufrom, *cto;
	cto = to = (char *) malloc(3*len+2);
	if ( cto==NULL ) return( NULL );
	iconv(from_unicode, (iconv_arg2_t) &cfrom, &in_left, &cto, &out_left);
	*cto++ = '\0';
	*cto++ = '\0';
	*cto++ = '\0';
	*cto++ = '\0';
	return( to );
    }
#endif
    if ( local_encoding==e_utf8 )
	len *= 3;
    if ( local_encoding>=e_first2byte )
	len *= 2;
    to = (char *) malloc(len+sizeof(unichar_t));
    if ( to==NULL ) return( NULL );
    ret = u2encoding_strncpy(to,ufrom,len,local_encoding);
    if ( ret==NULL )
	free( to );
    else if ( local_encoding<e_first2byte )
	to[len] = '\0';
    else {
	to[len] = '\0';
	to[len+1] = '\0';
    }
    return( ret );
}

char *def2utf8_copy(const char *from) {
    int len;
    char *ret;
    unichar_t *temp, *uto;

    if ( from==NULL ) return( NULL );
    len = strlen(from);
#if HAVE_ICONV_H
    if ( my_iconv_setup() ) {
	size_t in_left = len, out_left = 3*(len+1);
	char *cto = (char *) malloc(3*(len+1)), *cret = cto;
	if ( cto==NULL ) return( NULL );
	iconv(to_utf8, (iconv_arg2_t) &from, &in_left, &cto, &out_left);
	*cto++ = '\0';
	*cto++ = '\0';
	*cto++ = '\0';
	*cto++ = '\0';
	return( cret );
    }
#endif
    uto = (unichar_t *) malloc(sizeof(unichar_t)*(len+1));
    if ( uto==NULL ) return( NULL );
    temp = encoding2u_strncpy(uto,from,len,local_encoding);
    if ( temp==NULL ) {
	free( uto );
	return( NULL );
    }
    uto[len] = '\0';
    ret = u2utf8_copy(uto);
    free(uto);
    return( ret );
}

char *utf82def_copy(const char *ufrom) {
    int len;
    char *ret;
    unichar_t *u2from;

    if ( ufrom==NULL ) return( NULL );
    len = strlen(ufrom);
#if HAVE_ICONV_H
    if ( my_iconv_setup() ) {
	size_t in_left = len, out_left = 3*len;
	char *cfrom = (char *) ufrom, *cto, *to;
	cto = to = (char *) malloc(3*len+2);
	if ( cto==NULL ) return( NULL );
	iconv(from_utf8, (iconv_arg2_t) &cfrom, &in_left, &cto, &out_left);
	*cto++ = '\0';
	*cto++ = '\0';
	*cto++ = '\0';
	*cto++ = '\0';
	return( to );
    }
#endif
    if ( local_encoding==e_utf8 ) return( copy( ufrom )); /* Well, that's easy */
    u2from = utf82u_copy(ufrom);
    ret = u2def_copy(u2from);
    free(u2from);
    return( ret );
}
