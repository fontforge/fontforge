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

#include "ustring.h"
#include "utype.h"

#include "ffglib_compat.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

long uc_strcmp(const unichar_t *str1,const char *str2) {
    long ch1, ch2;
    for (;;) {
	ch1 = *str1++; ch2 = *(unsigned char *) str2++ ;
	if ( ch1!=ch2 || ch1=='\0' )
return(ch1-ch2);
    }
}

long uc_strncmp(const unichar_t *str1,const char *str2,int n) {
    long ch1, ch2;
    while ( --n>=0 ) {
	ch1 = *str1++; ch2 = *(unsigned char *) str2++ ;
	if ( ch1!=ch2 || ch1=='\0' )
return(ch1-ch2);
    }
return( 0 );
}

long uc_strmatch(const unichar_t *str1, const char *str2) {
    long ch1, ch2;
    for (;;) {
	ch1 = *str1++; ch2 = *(unsigned char *) str2++ ;
	ch1 = tolower(ch1);
	ch2 = tolower(ch2);
	if ( ch1!=ch2 || ch1=='\0' )
return(ch1-ch2);
    }
}

long uc_strnmatch(const unichar_t *str1, const char *str2, int len) {
    long ch1, ch2;
    for (;--len>=0;) {
	ch1 = *str1++; ch2 = *(unsigned char *) str2++ ;
	ch1 = tolower(ch1);
	ch2 = tolower(ch2);
	if ( ch1!=ch2 || ch1=='\0' || len<=0 )
return(ch1-ch2);
    }
return( 0 );
}

long u_strnmatch(const unichar_t *str1, const unichar_t *str2, int len) {
    long ch1, ch2;
    for (;--len>=0;) {
	ch1 = *str1++; ch2 = *str2++ ;
	ch1 = tolower(ch1);
	ch2 = tolower(ch2);
	if ( ch1!=ch2 || ch1=='\0' || len<=0 )
return(ch1-ch2);
    }
return( 0 );
}

long u_strcmp(const unichar_t *str1,const unichar_t *str2) {
    long ch1, ch2;
    for (;;) {
	ch1 = *str1++; ch2 = *str2++ ;
	if ( ch1!=ch2 || ch1=='\0' )
return(ch1-ch2);
    }
}

long u_strncmp(const unichar_t *str1,const unichar_t *str2,int n) {
    long ch1, ch2;
    while ( --n>=0 ) {
	ch1 = *str1++; ch2 = *str2++ ;
	if ( ch1!=ch2 || ch1=='\0' )
return(ch1-ch2);
    }
return( 0 );
}

long u_strmatch(const unichar_t *str1, const unichar_t *str2) {
    long ch1, ch2;
    for (;;) {
	ch1 = *str1++; ch2 = *str2++ ;
	ch1 = tolower(ch1);
	ch2 = tolower(ch2);
	if ( ch1!=ch2 || ch1=='\0' )
return(ch1-ch2);
    }
}

void cu_strcpy(char *to, const unichar_t *from) {
    unichar_t ch;
    while ( (ch = *from++) != '\0' )
	*(to++) = ch;
    *to = 0;
}

void uc_strcpy(unichar_t *to, const char *from) {
    unichar_t ch;
    while ( (ch = *(unsigned char *) from++) != '\0' )
	*(to++) = ch;
    *to = 0;
}

void u_strcpy(unichar_t *to, const unichar_t *from) {
    unichar_t ch;
    while ( (ch = *from++) != '\0' )
	*(to++) = ch;
    *to = 0;
}

char *cc_strncpy(char *to, const char *from, int len) {
    if( !from ) {
	to[0] = '\0';
	return to;
    }
    strncpy( to, from, len );
    return to;
}


void u_strncpy(unichar_t *to, const unichar_t *from, int len) {
    unichar_t ch;
    while ( (ch = *from++) != '\0' && --len>=0 )
	*(to++) = ch;
    *to = 0;
}

void cu_strncpy(char *to, const unichar_t *from, int len) {
    unichar_t ch;
    while ( (ch = *from++) != '\0' && --len>=0 )
	*(to++) = ch;
    *to = 0;
}

void uc_strncpy(unichar_t *to, const char *from, int len) {
    unichar_t ch;
    while ( (ch = *(unsigned char *) from++) != '\0' && --len>=0 )
	*(to++) = ch;
    *to = 0;
}

void uc_strcat(unichar_t *to, const char *from) {
    uc_strcpy(to+u_strlen(to),from);
}

void uc_strncat(unichar_t *to, const char *from,int len) {
    uc_strncpy(to+u_strlen(to),from,len);
}

void cu_strcat(char *to, const unichar_t *from) {
    cu_strcpy(to+strlen(to),from);
}

void cu_strncat(char *to, const unichar_t *from, int len) {
    cu_strncpy(to+strlen(to),from,len);
}

void u_strcat(unichar_t *to, const unichar_t *from) {
    u_strcpy(to+u_strlen(to),from);
}

void u_strncat(unichar_t *to, const unichar_t *from, int len) {
    u_strncpy(to+u_strlen(to),from,len);
}


int u_strlen(const unichar_t *str) {
    int len = 0;

    while ( *str++!='\0' )
	++len;
return( len );
}

int c_strlen( const char * p )
{
    if(!p)
	return 0;
    return strlen(p);
}


unichar_t *u_strchr(const unichar_t *str ,unichar_t ch) {
    unichar_t test;

    while ( (test=*(str++))!='\0' )
	if ( test==ch )
return( (unichar_t *) str-1 );

return( NULL );
}

unichar_t *u_strrchr(const unichar_t *str ,unichar_t ch) {
    unichar_t test;
    unichar_t *last = NULL;

    while ( (test=*(str++))!='\0' )
	if ( test==ch )
	    last = (unichar_t *) str-1;

return( last );
}

unichar_t *uc_strstr(const unichar_t *longer, const char *substr) {
    long ch1, ch2;
    const unichar_t *lpt, *str1; const char *str2;

    for ( lpt=longer; *lpt!='\0'; ++lpt ) {
	str1 = lpt; str2 = substr;
	for (;;) {
	    ch1 = *str1++; ch2 = *(unsigned char *) str2++ ;
	    if ( ch2=='\0' )
return((unichar_t *) lpt);
	    if ( ch1!=ch2 )
	break;
	}
    }
return( NULL );
}

unichar_t *u_strstr(const unichar_t *longer, const unichar_t *substr) {
    long ch1, ch2;
    const unichar_t *lpt, *str1, *str2;

    for ( lpt=longer; *lpt!='\0'; ++lpt ) {
	str1 = lpt; str2 = substr;
	for (;;) {
	    ch1 = *str1++; ch2 = *str2++ ;
	    if ( ch2=='\0' )
return((unichar_t *) lpt);
	    if ( ch1!=ch2 )
	break;
	}
    }
return( NULL );
}

unichar_t *uc_strstrmatch(const unichar_t *longer, const char *substr) {
    long ch1, ch2;
    const unichar_t *lpt, *str1; const unsigned char *str2;

    for ( lpt=longer; *lpt!='\0'; ++lpt ) {
	str1 = lpt; str2 = (unsigned char *) substr;
	for (;;) {
	    ch1 = *str1++; ch2 = *str2++ ;
	    ch1 = tolower(ch1);
	    ch2 = tolower(ch2);
	    if ( ch2=='\0' )
return((unichar_t *) lpt);
	    if ( ch1!=ch2 )
	break;
	}
    }
return( NULL );
}

unichar_t *u_strstrmatch(const unichar_t *longer, const unichar_t *substr) {
    long ch1, ch2;
    const unichar_t *lpt, *str1, *str2;

    for ( lpt=longer; *lpt!='\0'; ++lpt ) {
	str1 = lpt; str2 = substr;
	for (;;) {
	    ch1 = *str1++; ch2 = *str2++ ;
	    ch1 = tolower(ch1);
	    ch2 = tolower(ch2);
	    if ( ch2=='\0' )
return((unichar_t *) lpt);
	    if ( ch1!=ch2 )
	break;
	}
    }
return( NULL );
}

unichar_t *u_copyn(const unichar_t *pt, long n) {
    unichar_t *res;
#ifdef MEMORY_MASK
    if ( n*sizeof(unichar_t)>=MEMORY_MASK )
	n = MEMORY_MASK/sizeof(unichar_t)-1;
#endif
    res = (unichar_t *) malloc((n+1)*sizeof(unichar_t));
    memcpy(res,pt,n*sizeof(unichar_t));
    res[n]='\0';
return(res);
}

unichar_t *u_copynallocm(const unichar_t *pt, long n, long m) {
    unichar_t *res;
#ifdef MEMORY_MASK
    if ( n*sizeof(unichar_t)>=MEMORY_MASK )
	n = MEMORY_MASK/sizeof(unichar_t)-1;
#endif
    res = (unichar_t *) malloc((m+1)*sizeof(unichar_t));
    memcpy(res,pt,n*sizeof(unichar_t));
    res[n]='\0';
return(res);
}

unichar_t *u_copy(const unichar_t *pt) {
    if(pt)
return u_copyn(pt,u_strlen(pt));

return((unichar_t *)0);
}

unichar_t *u_concat(const unichar_t *s1, const unichar_t *s2) {
    long len1, len2;
    unichar_t *pt;

    if ( s1==NULL )
return( u_copy( s2 ));
    else if ( s2==NULL )
return( u_copy( s1 ));
    len1 = u_strlen(s1); len2 = u_strlen(s2);
    pt = (unichar_t *) malloc((len1+len2+1)*sizeof(unichar_t));
    u_strcpy(pt,s1);
    u_strcpy(pt+len1,s2);
return( pt );
}

unichar_t *uc_copyn(const char *pt,int len) {
    unichar_t *res, *rpt;

    if(!pt)
return((unichar_t *)0);

#ifdef MEMORY_MASK
    if ( (len+1)*sizeof(unichar_t)>=MEMORY_MASK )
	len = MEMORY_MASK/sizeof(unichar_t)-1;
#endif
    res = (unichar_t *) malloc((len+1)*sizeof(unichar_t));
    for ( rpt=res; --len>=0 ; *rpt++ = *(unsigned char *) pt++ );
    *rpt = '\0';
return(res);
}

unichar_t *uc_copy(const char *pt) {
    unichar_t *res, *rpt;
    int n;

    if(!pt)
return((unichar_t *)0);

    n = strlen(pt);
#ifdef MEMORY_MASK
    if ( (n+1)*sizeof(unichar_t)>=MEMORY_MASK )
	n = MEMORY_MASK/sizeof(unichar_t)-1;
#endif
    res = (unichar_t *) malloc((n+1)*sizeof(unichar_t));
    for ( rpt=res; --n>=0 ; *rpt++ = *(unsigned char *) pt++ );
    *rpt = '\0';
return(res);
}

char *cu_copyn(const unichar_t *pt,int len) {
    char *res, *rpt;

    if(!pt)
return(NULL);

#ifdef MEMORY_MASK
    if ( (len+1)>=MEMORY_MASK )
	len = MEMORY_MASK-1;
#endif
    res = (char *) malloc(len+1);
    for ( rpt=res; --len>=0 ; *rpt++ = *pt++ );
    *rpt = '\0';
return(res);
}

char *cu_copy(const unichar_t *pt) {
    char *res, *rpt;
    int n;

    if(!pt)
return((char *)0);

    n = u_strlen(pt);
#ifdef MEMORY_MASK
    if ( (n+1)>=MEMORY_MASK )
	n = MEMORY_MASK/sizeof(unichar_t)-1;
#endif
    res = (char *) malloc(n+1);
    for ( rpt=res; --n>=0 ; *rpt++ = *pt++ );
    *rpt = '\0';
return(res);
}

double u_strtod(const unichar_t *str, unichar_t **ptr) {
    char buf[60], *pt, *ret;
    const unichar_t *upt;
    double val;

    for ( upt=str, pt=buf; *upt<128 && *upt!='\0' && pt-buf<(ptrdiff_t)(sizeof(buf)-1); )
	*pt++ = *upt++;
    *pt = '\0';
    val = strtod(buf,&ret);
    if ( ptr!=NULL ) {
	if ( pt==ret )
	    *ptr = (unichar_t *) upt;
	else
	    *ptr = (unichar_t *) (str + (ret-buf));
    }
return( val );
}

long u_strtol(const unichar_t *str, unichar_t **ptr, int base) {
    char buf[60], *pt, *ret;
    const unichar_t *upt;
    long val;

    for ( upt=str, pt=buf; *upt<128 && *upt!='\0' && pt<buf+sizeof(buf)-1; )
	*pt++ = *upt++;
    *pt = '\0';
    val = strtol(buf,&ret,base);
    if ( ptr!=NULL ) {
	if ( pt==ret )
	    *ptr = (unichar_t *) upt;
	else
	    *ptr = (unichar_t *) (str + (ret-buf));
    }
return( val );
}

unsigned long u_strtoul(const unichar_t *str, unichar_t **ptr, int base) {
    char buf[60], *pt, *ret;
    const unichar_t *upt;
    unsigned long val;

    for ( upt=str, pt=buf; *upt<128 && *upt!='\0' && pt<buf+sizeof(buf)-1; )
	*pt++ = *upt++;
    *pt = '\0';
    val = strtoul(buf,&ret,base);
    if ( ptr!=NULL ) {
	if ( pt==ret )
	    *ptr = (unichar_t *) upt;
	else
	    *ptr = (unichar_t *) (str + (ret-buf));
    }
return( val );
}

unichar_t *cu_strstartmatch(const char *key,const unichar_t *str) {
    if ( key && str ) {
	while( *key ) {
	    if(tolower(*key) != tolower(*str))
return 0;
	    key++;
	    str++;
	}
    }
return (unichar_t *)str;
}

unichar_t *u_strstartmatch(const unichar_t *initial, const unichar_t *full) {
    int ch1, ch2;
    for (;;) {
	ch1 = *initial++; ch2 = *full++ ;
	if ( ch1=='\0' )
return( (unichar_t *) full );
	ch1 = tolower(ch1);
	ch2 = tolower(ch2);
	if ( ch1!=ch2 || ch1=='\0' )
return(NULL);
    }
}

char *u_to_c(const unichar_t *ubuf) {
    if( !ubuf )
	return 0;
    static char buf[400];
    cu_strncpy(buf,ubuf,sizeof(buf));
return( buf );
}

unichar_t *c_to_u(const char *buf) {
    static unichar_t ubuf[400];
    uc_strncpy(ubuf,buf,sizeof(ubuf));
return( ubuf );
}

unichar_t *utf82u_strncpy(unichar_t *ubuf,const char *utf8buf,int len) {
    unichar_t *upt = ubuf;
    int32_t ch;

    if (!ubuf || !utf8buf || len <= 0) {
        return ubuf;
    }

    while (len > 1 && (ch = utf8_ildb(&utf8buf)) != 0) {
        if (ch > 0) {
            *upt++ = ch;
            --len;
        } else {
            TRACE("Invalid UTF-8 sequence detected %s\n", utf8buf);
            do {
                ++utf8buf;
            } while ((*utf8buf & 0xc0) == 0x80);
        }
    }
    *upt = '\0';

    return ubuf;
}

unichar_t *utf82u_strcpy(unichar_t *ubuf,const char *utf8buf) {
return( utf82u_strncpy(ubuf,utf8buf,c_strlen(utf8buf)+1));
}

unichar_t *utf82u_copyn(const char *utf8buf,int len) {
    unichar_t *ubuf = (unichar_t *) malloc((len+1)*sizeof(unichar_t));
return( utf82u_strncpy(ubuf,utf8buf,len+1));
}

unichar_t *utf82u_copy(const char *utf8buf) {
    int len;
    unichar_t *ubuf;

    if ( utf8buf==NULL )
return( NULL );

    len = strlen(utf8buf);
    ubuf = (unichar_t *) malloc((len+1)*sizeof(unichar_t));
return( utf82u_strncpy(ubuf,utf8buf,len+1));
}

void utf82u_strcat(unichar_t *to,const char *from) {
    utf82u_strcpy(to+u_strlen(to),from);
}

char *u2utf8_strncpy(char *utf8buf,const unichar_t *ubuf,int len) {
/* Copy unichar string 'ubuf' into utf8 buffer string 'utf8buf' */
    char *pt = utf8buf;

    assert(utf8buf != NULL);

    if ( ubuf!=NULL ) {
        while ( *ubuf && (--len != 0) ) {
            pt=utf8_idpb(pt,*ubuf++,0);
        }
        *pt = '\0';
        return( utf8buf );
    }

    return( NULL );
}

char *u2utf8_strcpy(char *utf8buf,const unichar_t *ubuf) {
    return u2utf8_strncpy(utf8buf, ubuf, -1);
}

char *utf8_strchr(const char *str, int search) {
    int ch;
    const char *old = str;

    while ( (ch = utf8_ildb(&str))!=0 ) {
	if ( ch==search )
return( (char *) old );
	old = str;
    }
return( NULL );
}

char *latin1_2_utf8_strcpy(char *utf8buf,const char *lbuf) {
    char *pt = utf8buf;
    const unsigned char *lpt = (const unsigned char *) lbuf;

    while ( *lpt ) {
	if ( *lpt<0x80 )
	    *pt++ = *lpt;
	else {
	    *pt++ = 0xc0 | (*lpt>>6);
	    *pt++ = 0x80 | (*lpt&0x3f);
	}
	++lpt;
    }
    *pt = '\0';
return( utf8buf );
}

char *latin1_2_utf8_copy(const char *lbuf) {
    int len;
    char *utf8buf;

    if ( lbuf==NULL )
return( NULL );

    len = strlen(lbuf);
    utf8buf = (char *) malloc(2*len+1);
return( latin1_2_utf8_strcpy(utf8buf,lbuf));
}

char *utf8_2_latin1_copy(const char *utf8buf) {
    int len;
    int ch;
    char *lbuf, *pt; const char *upt;

    if ( utf8buf==NULL )
return( NULL );

    len = strlen(utf8buf);
    pt = lbuf = (char *) malloc(len+1);
    for ( upt=utf8buf; (ch=utf8_ildb(&upt))!='\0'; )
	if ( ch>=0xff )
	    *pt++ = '?';
	else
	    *pt++ = ch;
    *pt = '\0';
return( lbuf );
}

char *u2utf8_copy(const unichar_t *ubuf) {
/* Make a utf8 string copy of unichar string ubuf */

    if ( ubuf==NULL )
	return( NULL );

    return( u2utf8_copyn(ubuf,u_strlen(ubuf)+1) );
}

char *u2utf8_copyn(const unichar_t *ubuf,int len) {
/* Make a utf8 string copy of unichar string ubuf[0..len] */
    char *utf8buf, *pt, *pt2;

    if ( ubuf==NULL || len<=0 || (utf8buf=pt=(char *)malloc(len*6+1))==NULL )
	return( NULL );

    while ( (pt2=utf8_idpb(pt,*ubuf++,0)) && --len )
	pt = pt2;

    if ( pt2 )
	pt = pt2;
    else
	TRACE("u2utf8_copyn: truncated on invalid char 0x%x\n", ubuf[-1]);

    *pt = '\0';
    return( utf8buf );
}

int32_t utf8_ildb(const char **_text) {
    int32_t val= -1;
    int ch;
    const uint8_t *text = (const uint8_t *) *_text;
    /* Increment and load character */

    if ( text==NULL )
	return( val );
    else if ( (ch = *text++)<0x80 ) {
	val = ch;
    } else if ( ch<=0xbf ) {
	/* error */
    } else if ( ch<=0xdf ) {
	if ( *text>=0x80 && *text<0xc0 )
	    val = ((ch&0x1f)<<6) | (*text++&0x3f);
    } else if ( ch<=0xef ) {
	if ( *text>=0x80 && *text<0xc0 && text[1]>=0x80 && text[1]<0xc0 ) {
	    val = ((ch&0xf)<<12) | ((text[0]&0x3f)<<6) | (text[1]&0x3f);
	    text += 2;
	}
    } else {
	int w = ( ((ch&0x7)<<2) | ((text[0]&0x30)>>4) )-1, w2;
	w = (w<<6) | ((text[0]&0xf)<<2) | ((text[1]&0x30)>>4);
	w2 = ((text[1]&0xf)<<6) | (text[2]&0x3f);
	val = w*0x400 + w2 + 0x10000;
	if ( *text<0x80 || text[1]<0x80 || text[2]<0x80 ||
		*text>=0xc0 || text[1]>=0xc0 || text[2]>=0xc0 )
	    val = -1;
	else
	    text += 3;
    }
    *_text = (const char *) text;
return( val );
}

char *utf8_idpb(char *utf8_text,uint32_t ch,int flags) {
/* Increment and deposit character, no '\0' appended */
/* NOTE: Unicode only needs range of 17x65535 values */
/* and strings must be long enough to hold +4 chars. */
/* ISO/IEC 10646 description of UTF8 allows encoding */
/* character values up to U+7FFFFFFF before RFC3629. */

    if ( ch>0x7fffffff || \
	 (!(flags&UTF8IDPB_OLDLIMIT) && ((ch>=0xd800 && ch<=0xdfff) || ch>=17*65536)) )
	return( 0 ); /* Error, ch is out of range */

    if ( (flags&(UTF8IDPB_UCS2|UTF8IDPB_UTF16|UTF8IDPB_UTF32)) ) {
	if ( (flags&UTF8IDPB_UCS2) && ch>0xffff )
	    return( 0 ); /* Error, ch is out of range */
	if ( (flags&UTF8IDPB_UTF32) ) {
	    *utf8_text++ = ((ch>>24)&0xff);
	    *utf8_text++ = ((ch>>16)&0xff);
	    ch &= 0xffff;
	}
	if ( ch>0xffff ) {
	    /* ...here if a utf16 encoded value */
	    unsigned long us;
	    ch -= 0x10000;
	    us = (ch>>10)+0xd800;
	    *utf8_text++ = us>>8;
	    *utf8_text++ = us&0xff;
	    ch = (ch&0x3ff)+0xdc00;
	}
	*utf8_text++ = ch>>8;
	ch &= 0xff;
    } else if ( ch>127 || (ch==0 && (flags&UTF8IDPB_NOZERO)) ) {
	if ( ch<=0x7ff )
	    /* ch>=0x80 && ch<=0x7ff */
	    *utf8_text++ = 0xc0 | (ch>>6);
	else {
	    if ( ch<=0xffff )
		/* ch>=0x800 && ch<=0xffff */
		*utf8_text++ = 0xe0 | (ch>>12);
	    else {
		if ( ch<=0x1fffff )
		    /* ch>=0x10000 && ch<=0x1fffff */
		    *utf8_text++ = 0xf0 | (ch>>18);
		else {
		    if ( ch<=0x3ffffff )
			/* ch>=0x200000 && ch<=0x3ffffff */
			*utf8_text++ = 0xf8 | (ch>>24);
		    else {
			/* ch>=0x4000000 && ch<=0x7fffffff */
			*utf8_text++ = 0xfc | (ch>>30);
			*utf8_text++ = 0x80 | ((ch>>24)&0x3f);
		    }
		    *utf8_text++ = 0x80 | ((ch>>18)&0x3f);
		}
		*utf8_text++ = 0x80 | ((ch>>12)&0x3f);
	    }
	    *utf8_text++ = 0x80 | ((ch>>6)&0x3f);
	}
	ch = 0x80 | (ch&0x3f);
    }
    *utf8_text++ = ch;
    return( utf8_text );
}

char *utf8_ib(char *utf8_text) {
/* Increment to next utf8 character */
    unsigned char ch;

    if ( (ch = (unsigned char) *utf8_text)=='\0' )
	return( utf8_text );
    else if ( ch<=127 )
	return( utf8_text+1 );
    else if ( ch<0xe0 )
	return( utf8_text+2 );
    else if ( ch<0xf0 )
	return( utf8_text+3 );
    else if ( ch<0xf8 )
	return( utf8_text+4 );
    else if ( ch<0xfc )
	return( utf8_text+5 );
    else
	return( utf8_text+6 );
}

int utf8_valid(const char *str) {
    /* Is this a valid utf8 string? */
    int ch;

    while ( (ch=utf8_ildb(&str))!='\0' )
	if ( ch==-1 )
return( false );

return( true );
}

void utf8_truncatevalid(char *str) {
    /* There are certain cases where we have a fixed amount of space to display */
    /*  something, and if it doesn't fit in that, then we truncate it. But... */
    /*  that can leave us with a half completed utf8 byte sequence. So truncate*/
    /*  again, right before the start of the bad sequence */
    int ch;
    char *old;

    old = str;
    while ( (ch=utf8_ildb((const char **) &str))!='\0' ) {
	if ( ch==-1 ) {
	    *old = '\0';
return;
	}
	old = str;
    }
}

char *utf8_db(char *utf8_text) {
/* Decrement utf8 pointer to previous utf8 character.*/
/* NOTE: This should never happen but if the pointer */
/* was looking at an intermediate character, it will */
/* be properly positioned at the start of a new char */
/* and not the previous character.		     */
    unsigned char *pt = (unsigned char *) utf8_text;

    --pt;
    if ( *pt>=0x80 && *pt<0xc0 ) {
	--pt;
	if ( *pt>=0x80 && *pt<0xc0 ) {
	    --pt;
	    if ( *pt>=0x80 && *pt<0xc0 ) {
		--pt;
		if ( *pt>=0x80 && *pt<0xc0 ) {
		    --pt;
		    if ( *pt>=0x80 && *pt<0xc0 )
			--pt;
		}
	    }
	}
    }
    return( (char *) pt );
}

long utf8_strlen(const char *utf8_str) {
/* Count how many characters in the string NOT bytes */
    long len = 0;

    while ( utf8_ildb(&utf8_str)>0 && ++len>0 );
    return( len );
}

long utf82u_strlen(const char *utf8_str) {
/* Count how many shorts needed to represent in UCS2 */
    int32_t ch;
    long len = 0;

    while ( (ch = utf8_ildb(&utf8_str))>0 && ++len>0 )
	if ( ch>=0x10000 )
	    ++len;
    return( len );
}

void utf8_strncpy(char *to, const char *from, int len) {
    /* copy n characters NOT bytes */
    const char *old = from;
    while ( len && *old ) {
	utf8_ildb(&old);
	len--;
    }
    strncpy(to, from, old-from);
    to[old-from] = 0;
}

uint16_t *u2utf16_strcpy(uint16_t *utf16buf,const unichar_t *ubuf) {
/* Copy unichar string 'ubuf' into utf16 buffer string 'utf16buf' */
    uint16_t *pt = utf16buf;
    unichar_t ch;

    if (utf16buf == NULL || ubuf == NULL)
        return NULL;

    while ( *ubuf ) {
        ch = *ubuf++;
        if (ch <= 0xFFFF)
            *pt++ = ch;
        else {
            /* Encode with surrogate pair */
            *pt++ = ((ch - 0x10000)>>10) + 0xD800;    /* high surrogate */
            *pt++ = ((ch - 0x10000)&0x3FF) + 0xDC00;  /* low surrogate */
        }
    }
    *pt = '\0';
    return( utf16buf );
}

extern unichar_t *utf162u_strcpy(unichar_t*ubuf, const uint16_t *utf16buf) {
    unichar_t uch = 0x0, uch2 = 0x0;
    unichar_t *pt = ubuf;

    if (utf16buf == NULL || ubuf == NULL)
        return NULL;

    while ( *utf16buf ) {
        uch = *utf16buf++;
	if ( uch>=0xd800 && uch<0xdc00 ) {
	    /* Is this a possible utf16 high surrogate value? */
	    uch2 = *utf16buf++;
	    if ( uch2>=0xdc00 && uch2<0xe000 ) /* low surrogate */
		uch = ((uch-0xd800)<<10 | (uch2&0x3ff)) + 0x10000;
	    else {
		*pt++ = uch;
                if (uch2==0)
                    break;
		uch = uch2;
	    }
	}
        *pt++ = uch;
    }
    *pt = '\0';
    return( ubuf );
}

uint16_t *utf82utf16_copy(const char* utf8buf) {
    unichar_t *utf32_str = utf82u_copy(utf8buf); /* Convert from utf8 to utf32 */
    uint16_t *utf16buf = (uint16_t *) malloc(2*(u_strlen(utf32_str)+1)*sizeof(uint16_t));
    u2utf16_strcpy(utf16buf, utf32_str);
    free(utf32_str);

    return utf16buf;
}

char *StripToASCII(const char *utf8_str) {
    /* Remove any non-ascii characters: Special case, convert the copyright symbol to (c) */
    char *newcr, *pt, *end;
    int len, ch;
    const unichar_t *alt;

    len = strlen(utf8_str);
    pt = newcr = (char *) malloc(len+1);
    end = pt+len;
    while ( (ch= utf8_ildb(&utf8_str))!='\0' ) {
	if ( pt>=end ) {
	    int off = pt-newcr;
	    newcr = (char *) realloc(newcr,(off+10)+1);
	    pt = newcr+off;
	    end = pt+10;
	}
	if ( (ch>=' ' && ch<'\177' ) || ch=='\n' || ch=='\t' )
	    *pt++ = ch;
	else if ( ch=='\r' && *utf8_str!='\n' )
	    *pt++ = '\n';
	else if ( ch==0xa9 /* Copyright sign */ ) {
	    const char *str = "(c)";
	    if ( pt+strlen(str)>=end ) {
		int off = pt-newcr;
		newcr = (char *) realloc(newcr,(off+10+strlen(str))+1);
		pt = newcr+off;
		end = pt+10;
	    }
	    while ( *str )
		*pt++ = *str++;
	} else if ( (alt = unialt(ch))!=NULL ) {
	    while ( *alt!='\0' ) {
		if ( pt>=end ) {
		    int off = pt-newcr;
		    newcr = (char *) realloc(newcr,(off+10)+1);
		    pt = newcr+off;
		    end = pt+10;
		}
		if ( *alt>=' ' && *alt<'\177' )
		    *pt++ = *alt;
		else if ( *alt==0x300 )
		    *pt++ = '`';
		else if ( *alt==0x301 )
		    *pt++ = '\'';
		else if ( *alt==0x302 )
		    *pt++ = '^';
		else if ( *alt==0x303 )
		    *pt++ = '~';
		else if ( *alt==0x308 )
		    *pt++ = ':';
		++alt;
	    }
	}
    }
    *pt = '\0';
return( newcr );
}

int AllAscii(const char *txt) {
/* Verify string only All ASCII printable characters */
    unsigned char ch;

    if ( txt==NULL )
	return( false );

    for ( ; (ch=(unsigned char) *txt)!='\0'; ++txt )
	if ( (ch>=' ' && ch<'\177' ) || \
	     ch=='\t' || ch=='\n' || ch=='\r' )
	    /* All right */;
	else
	    return( false );

    return( true );
}

int uAllAscii(const unichar_t *txt) {
/* Verify string only All ASCII printable unichar_t. */

    if ( txt==NULL )
	return( false );

    for ( ; *txt!='\0'; ++txt )
	if ( (*txt>=' ' && *txt<'\177' ) || \
	     *txt=='\t' || *txt=='\n' || *txt=='\r' )
	    /* All right */;
	else
	    return( false );

    return( true );
}

char *chomp( char *line ) {
/* Chomp the last '\r' and '\n' in character string. */
/* \r = CR used as a newline char in MAC OS before X */
/* \n = LF used as a newline char in MAC OSX & Linux */
/* \r\n = CR + LF used as a newline char in Windows. */
    int x;

    if( line==NULL || (x=strlen(line)-1)<0 )
	return( line );

    if ( line[x]=='\n' ) {
	line[x] = '\0';
	--x;
    }
    if ( x>=0 && line[x]=='\r' )
	line[x] = '\0';
    return( line );
}

int endswith(const char *haystack,const char *needle) {
    int haylen = strlen( haystack );
    int nedlen = strlen( needle );
    if( haylen < nedlen )
	return 0;
    const char* p = strstr( haystack + haylen - nedlen, needle );
    return p == ( haystack + haylen - nedlen );
}

int endswithi(const char *haystackZ,const char *needleZ) {
    char* haystack = ff_ascii_strdown(haystackZ,-1);
    char* needle   = ff_ascii_strdown(needleZ,-1);
    int ret = endswith( haystack, needle );
    free( haystack );
    free( needle );
    return ret;
}

int endswithi_partialExtension( const char *haystackZ,const char *needleZ) {
    int nedlen = strlen(needleZ);
    if( nedlen == 0 ) {
	return 0;
    }
    char* haystack = ff_ascii_strdown(haystackZ,-1);
    char* needle   = ff_ascii_strdown(needleZ,-1);
    int ret = 0;
    int i = nedlen-1;
    ret |= endswith( haystack, needle );
    for( ; i>=0 && !ret ; --i ) {
	needle[i] = '\0';
	ret |= endswith( haystack, needle );
    }
    free( haystack );
    free( needle );
    return ret;
}

int u_endswith(const unichar_t *haystack,const unichar_t *needle) {
    int haylen = u_strlen( haystack );
    int nedlen = u_strlen( needle );
    if( haylen < nedlen )
	return 0;
    unichar_t* p = u_strstr( haystack + haylen - nedlen, needle );
    return p == ( haystack + haylen - nedlen );
}

int u_startswith(const unichar_t *haystack,const unichar_t *needle) {

    if( !haystack || !needle )
	return 0;

    unichar_t* p = u_strstr( haystack, needle );
    return p == ( haystack );
}

int uc_startswith(const unichar_t *haystack,const char* needle)
{
    return u_startswith( haystack, c_to_u(needle));
}


char* c_itostr( int v )
{
    static char ret[100+1];
    snprintf(ret,100,"%d",v );
    return ret;
}

char* str_replace_all( char* s, char* orig, char* replacement, int free_s )
{
    char* p = strstr( s, orig );
    if( !p )
    {
	if( free_s )
	    return s;
	return copy( s );
    }

    int count = 0;
    p = s;
    while( p )
    {
	p = strstr( p, orig );
	if( !p )
	    break;
	p++;
	count++;
    }
    count++;

    // more than strictly needed, but always enough RAM.
    int retsz = strlen(s) + count*strlen(replacement) + 1;
    char* ret = (char *) malloc( retsz );
    memset( ret, '\0', retsz );
    char* output = ret;
    char* remains = s;
    p = remains;
    while( p )
    {
	p = strstr( remains, orig );
	if( !p )
	{
	    strcpy( output, remains );
	    break;
	}
	if( p > remains )
	    strncpy( output, remains, p-remains );
	strcat( output, replacement );
	output += strlen(output);
	remains = p + strlen(orig);
    }

    if( free_s )
	free(s);
    return ret;
}

int toint( char* v )
{
    if( !v )
        return 0;
    return atoi(v);
}
char* tostr( int v )
{
    const int bufsz = 100;
    static char buf[101];
    snprintf(buf,bufsz,"%d",v);
    return buf;
}

void realloc_tail(char** p_buf, size_t size_delta, char** p_tail,
                  char** p_proc) {
    size_t new_size = size_delta + (*p_tail - *p_buf);
    char* new_buf = (char*) realloc(*p_buf, new_size);
    *p_tail = new_buf + new_size;
    if (p_proc) *p_proc = new_buf + (*p_proc - *p_buf);
    *p_buf = new_buf;
}
