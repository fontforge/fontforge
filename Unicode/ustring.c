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
#include <stddef.h>
#include "ustring.h"
#include "utype.h"

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
    register unichar_t ch;
    while ( (ch = *from++) != '\0' )
	*(to++) = ch;
    *to = 0;
}

void uc_strcpy(unichar_t *to, const char *from) {
    register unichar_t ch;
    while ( (ch = *(unsigned char *) from++) != '\0' )
	*(to++) = ch;
    *to = 0;
}

void u_strcpy(unichar_t *to, const unichar_t *from) {
    register unichar_t ch;
    while ( (ch = *from++) != '\0' )
	*(to++) = ch;
    *to = 0;
}

void u_strncpy(register unichar_t *to, const unichar_t *from, int len) {
    register unichar_t ch;
    while ( (ch = *from++) != '\0' && --len>=0 )
	*(to++) = ch;
    *to = 0;
}

void cu_strncpy(register char *to, const unichar_t *from, int len) {
    register unichar_t ch;
    while ( (ch = *from++) != '\0' && --len>=0 )
	*(to++) = ch;
    *to = 0;
}

void uc_strncpy(register unichar_t *to, const char *from, int len) {
    register unichar_t ch;
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

int  u_strlen(register const unichar_t *str) {
    register int len = 0;

    while ( *str++!='\0' )
	++len;
return( len );
}

unichar_t *u_strchr(const unichar_t *str ,unichar_t ch) {
    register unichar_t test;

    while ( (test=*(str++))!='\0' )
	if ( test==ch )
return( (unichar_t *) str-1 );

return( NULL );
}

unichar_t *u_strrchr(const unichar_t *str ,unichar_t ch) {
    register unichar_t test, *last = NULL;

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
    res = galloc((n+1)*sizeof(unichar_t));
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
    pt = galloc((len1+len2+1)*sizeof(unichar_t));
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
    res = galloc((len+1)*sizeof(unichar_t));
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
    res = galloc((n+1)*sizeof(unichar_t));
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
    res = galloc(len+1);
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
    res = galloc(n+1);
    for ( rpt=res; --n>=0 ; *rpt++ = *pt++ );
    *rpt = '\0';
return(res);
}

double u_strtod(const unichar_t *str, unichar_t **ptr) {
    char buf[60], *pt, *ret;
    const unichar_t *upt;
    double val;
    extern double strtod();		/* Please don't delete this, not all of us have good ansi headers */

    for ( upt=str, pt=buf; *upt<128 && *upt!='\0'; )
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
    extern long strtol();		/* Please don't delete this, not all of us have good ansi headers */

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
    unichar_t *upt=ubuf, *uend=ubuf+len-1;
    const uint8 *pt = (const uint8 *) utf8buf;
    int w;

    while ( *pt!='\0' && upt<uend ) {
	if ( *pt<=127 )
	    *upt = *pt++;
	else if ( *pt<=0xdf ) {
	    *upt = ((*pt&0x1f)<<6) | (pt[1]&0x3f);
	    pt += 2;
	} else if ( *pt<=0xef ) {
	    *upt = ((*pt&0xf)<<12) | ((pt[1]&0x3f)<<6) | (pt[2]&0x3f);
	    pt += 3;
	} else if ( upt+1<uend ) {
	    /* Um... I don't support surrogates */
	    w = ( ((*pt&0x7)<<2) | ((pt[1]&0x30)>>4) )-1;
	    *upt++ = 0xd800 | (w<<6) | ((pt[1]&0xf)<<2) | ((pt[2]&0x30)>>4);
	    *upt   = 0xdc00 | ((pt[2]&0xf)<<6) | (pt[3]&0x3f);
	    pt += 4;
	} else {
	    /* no space for surrogate */
	    pt += 4;
	}
	++upt;
    }
    *upt = '\0';
return( ubuf );
}

int32 *utf82u32_strncpy(int32 *ubuf,const char *utf8buf,int len) {
    int32 *upt=ubuf, *uend=ubuf+len-1;
    const uint8 *pt = (const uint8 *) utf8buf;
    int w, w2;

    while ( *pt!='\0' && upt<uend ) {
	if ( *pt<=127 )
	    *upt = *pt++;
	else if ( *pt<=0xdf ) {
	    *upt = ((*pt&0x1f)<<6) | (pt[1]&0x3f);
	    pt += 2;
	} else if ( *pt<=0xef ) {
	    *upt = ((*pt&0xf)<<12) | ((pt[1]&0x3f)<<6) | (pt[2]&0x3f);
	    pt += 3;
	} else {
	    w = ( ((*pt&0x7)<<2) | ((pt[1]&0x30)>>4) )-1;
	    w = (w<<6) | ((pt[1]&0xf)<<2) | ((pt[2]&0x30)>>4);
	    w2 = ((pt[2]&0xf)<<6) | (pt[3]&0x3f);
	    *upt = w*0x400 + w2 + 0x10000;
	    pt += 4;
	}
	++upt;
    }
    *upt = '\0';
return( ubuf );
}

char *u322utf8_strncpy(char *utf8buf, const int32 *ubuf,int len) {
    uint8 *pt=(uint8 *) utf8buf, *end=(uint8 *) utf8buf+len-1;
    const int32 *upt = ubuf;

    while ( *upt!='\0' && pt<end ) {
	if ( *upt<=127 )
	    *pt = *upt++;
	else if ( *upt<=0x7ff ) {
	    if ( pt+1>=end )
    break;
	    *pt++ = 0xc0 | (*upt>>6);
	    *pt++ = 0x80 | (*upt&0x3f);
	} else if ( *upt<=0xffff ) {
	    if ( pt+2>=end )
    break;
	    *pt++ = 0xe0 | (*upt>>12);
	    *pt++ = 0x80 | ((*upt>>6)&0x3f);
	    *pt++ = 0x80 | (*upt&0x3f);
	} else {
	    uint32 val = *upt-0x10000;
	    int u = ((val&0xf0000)>>16)+1, z=(val&0x0f000)>>12, y=(val&0x00fc0)>>6, x=val&0x0003f;
	    if ( pt+3>=end )
    break;
	    *pt++ = 0xf0 | (u>>2);
	    *pt++ = 0x80 | ((u&3)<<4) | z;
	    *pt++ = 0x80 | y;
	    *pt++ = 0x80 | x;
	}
	++upt;
    }
    *pt = '\0';
return( utf8buf );
}

char *u322utf8_copy(const int32 *ubuf) {
    int i, len;
    char *buf;

    for ( i=len=0; ubuf[i]!=0; ++i )
	if ( ubuf[i]<0x80 )
	    ++len;
	else if ( ubuf[i]<0x800 )
	    len += 2;
	else if ( ubuf[i]<0x10000 )
	    len += 3;
	else
	    len += 4;
    buf = galloc(len+1);
return( u322utf8_strncpy(buf,ubuf,len+1));
}

unichar_t *utf82u_copyn(const char *utf8buf,int len) {
    unichar_t *ubuf = galloc((len+1)*sizeof(unichar_t));
return( utf82u_strncpy(ubuf,utf8buf,len+1));
}

unichar_t *utf82u_copy(const char *utf8buf) {
    int len;
    unichar_t *ubuf;

    if ( utf8buf==NULL )
return( NULL );

    len = strlen(utf8buf);
    ubuf = galloc((len+1)*sizeof(unichar_t));
return( utf82u_strncpy(ubuf,utf8buf,len+1));
}

int32 *utf82u32_copy(const char *utf8buf) {
    int len;
    int32 *ubuf;

    if ( utf8buf==NULL )
return( NULL );

    len = strlen(utf8buf);
    ubuf = galloc((len+1)*sizeof(int32));
return( utf82u32_strncpy(ubuf,utf8buf,len+1));
}

char *utf82u_strcpy(char *utf8buf,const unichar_t *ubuf) {
    char *pt = utf8buf;

    while ( *ubuf ) {
	if ( *ubuf<0x80 )
	    *pt++ = *ubuf;
	else if ( *ubuf<0x800 ) {
	    *pt++ = 0xc0 | (*ubuf>>6);
	    *pt++ = 0x80 | (*ubuf&0x3f);
	} else if ( *ubuf>=0xd800 && *ubuf<0xdc00 && ubuf[1]>=0xdc00 && ubuf[1]<0xe000 ) {
	    int u = ((*ubuf>>6)&0xf)+1, y = ((*ubuf&3)<<4) | ((ubuf[1]>>6)&0xf);
	    *pt++ = 0xf0 | (u>>2);
	    *pt++ = 0x80 | ((u&3)<<4) | ((*ubuf>>2)&0xf);
	    *pt++ = 0x80 | y;
	    *pt++ = 0x80 | (ubuf[1]&0x3f);
	} else {
	    *pt++ = 0xe0 | (*ubuf>>12);
	    *pt++ = 0x80 | ((*ubuf>>6)&0x3f);
	    *pt++ = 0x80 | (*ubuf&0x3f);
	}
	++ubuf;
    }
    *pt = '\0';
return( utf8buf );
}

char *u2utf8_copy(const unichar_t *ubuf) {
    int len;
    char *utf8buf;

    if ( ubuf==NULL )
return( NULL );

    len = u_strlen(ubuf);
    utf8buf = galloc((len+1)*3);
return( utf82u_strcpy(utf8buf,ubuf));
}
