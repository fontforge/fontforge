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

    for ( upt=str, pt=buf; *upt<128 && *upt!='\0' && pt-buf<sizeof(buf)-1; )
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
    const uint8 *pt = (const uint8 *) utf8buf, *end = pt+strlen(utf8buf);
    int w, w2;

    while ( pt<end && *pt!='\0' && upt<uend ) {
	if ( *pt<=127 )
	    *upt = *pt++;
	else if ( *pt<=0xdf ) {
	    *upt = ((*pt&0x1f)<<6) | (pt[1]&0x3f);
	    pt += 2;
	} else if ( *pt<=0xef ) {
	    *upt = ((*pt&0xf)<<12) | ((pt[1]&0x3f)<<6) | (pt[2]&0x3f);
	    pt += 3;
#ifdef UNICHAR_16
	} else if ( upt+1<uend ) {
	    /* Um... I don't support surrogates */
	    w = ( ((*pt&0x7)<<2) | ((pt[1]&0x30)>>4) )-1;
	    *upt++ = 0xd800 | (w<<6) | ((pt[1]&0xf)<<2) | ((pt[2]&0x30)>>4);
	    *upt   = 0xdc00 | ((pt[2]&0xf)<<6) | (pt[3]&0x3f);
	    pt += 4;
	} else {
	    /* no space for surrogate */
	    pt += 4;
#else
	} else {
	    w = ( ((*pt&0x7)<<2) | ((pt[1]&0x30)>>4) )-1;
	    w = (w<<6) | ((pt[1]&0xf)<<2) | ((pt[2]&0x30)>>4);
	    w2 = ((pt[2]&0xf)<<6) | (pt[3]&0x3f);
	    *upt = w*0x400 + w2 + 0x10000;
	    pt += 4;
#endif
	}
	++upt;
    }
    *upt = '\0';
return( ubuf );
}

unichar_t *utf82u_strcpy(unichar_t *ubuf,const char *utf8buf) {
return( utf82u_strncpy(ubuf,utf8buf,strlen(utf8buf)+1));
}

# ifdef UNICHAR_16
uint32 *utf82u32_strncpy(uint32 *ubuf,const char *utf8buf,int len) {
    uint32 *upt=ubuf, *uend=ubuf+len-1;
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

char *u322utf8_strncpy(char *utf8buf, const uint32 *ubuf,int len) {
    uint8 *pt=(uint8 *) utf8buf, *end=(uint8 *) utf8buf+len-1;
    const uint32 *upt = ubuf;

    while ( *upt!='\0' && pt<end ) {
	if ( *upt<=127 )
	    *pt++ = *upt;
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

char *u322utf8_copy(const uint32 *ubuf) {
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
#endif

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

void utf82u_strcat(unichar_t *to,const char *from) {
    utf82u_strcpy(to+u_strlen(to),from);
}

#ifdef UNICHAR_16
uint32 *utf82u32_copy(const char *utf8buf) {
    int len;
    uint32 *ubuf;

    if ( utf8buf==NULL )
return( NULL );

    len = strlen(utf8buf);
    ubuf = galloc((len+1)*sizeof(uint32));
return( utf82u32_strncpy(ubuf,utf8buf,len+1));
}
#endif

char *u2utf8_strcpy(char *utf8buf,const unichar_t *ubuf) {
    char *pt = utf8buf;

    while ( *ubuf ) {
	if ( *ubuf<0x80 )
	    *pt++ = *ubuf;
	else if ( *ubuf<0x800 ) {
	    *pt++ = 0xc0 | (*ubuf>>6);
	    *pt++ = 0x80 | (*ubuf&0x3f);
#ifdef UNICHAR_16
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
#else
	} else if ( *ubuf < 0x10000 ) {
	    *pt++ = 0xe0 | (*ubuf>>12);
	    *pt++ = 0x80 | ((*ubuf>>6)&0x3f);
	    *pt++ = 0x80 | (*ubuf&0x3f);
	} else {
	    uint32 val = *ubuf-0x10000;
	    int u = ((val&0xf0000)>>16)+1, z=(val&0x0f000)>>12, y=(val&0x00fc0)>>6, x=val&0x0003f;
	    *pt++ = 0xf0 | (u>>2);
	    *pt++ = 0x80 | ((u&3)<<4) | z;
	    *pt++ = 0x80 | y;
	    *pt++ = 0x80 | x;
#endif
	}
	++ubuf;
    }
    *pt = '\0';
return( utf8buf );
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
    utf8buf = galloc(2*len+1);
return( latin1_2_utf8_strcpy(utf8buf,lbuf));
}

char *utf8_2_latin1_copy(const char *utf8buf) {
    int len;
    int ch;
    char *lbuf, *pt; const char *upt;

    if ( utf8buf==NULL )
return( NULL );

    len = strlen(utf8buf);
    pt = lbuf = galloc(len+1);
    for ( upt=utf8buf; (ch=utf8_ildb(&upt))!='\0'; )
	if ( ch>=0xff )
	    *pt++ = '?';
	else
	    *pt++ = ch;
    *pt = '\0';
return( lbuf );
}

char *u2utf8_copy(const unichar_t *ubuf) {
    int len;
    char *utf8buf;

    if ( ubuf==NULL )
return( NULL );

    len = u_strlen(ubuf);
    utf8buf = galloc((len+1)*4);
return( u2utf8_strcpy(utf8buf,ubuf));
}

char *u2utf8_copyn(const unichar_t *ubuf,int len) {
    int i;
    char *utf8buf, *pt;

    if ( ubuf==NULL )
return( NULL );

    utf8buf = pt = galloc((len+1)*4);
    for ( i=0; i<len && *ubuf!='\0'; ++i )
	pt = utf8_idpb(pt, *ubuf++);
    *pt = '\0';
return( utf8buf );
}

int32 utf8_ildb(const char **_text) {
    int32 val= -1;
    int ch;
    const uint8 *text = (const uint8 *) *_text;
    /* Increment and load character */

    if ( (ch = *text++)<0x80 ) {
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

char *utf8_idpb(char *utf8_text,uint32 ch) {
    /* Increment and deposit character */
    if ( ch<0 || ch>=17*65536 )
return( utf8_text );

    if ( ch<=127 )
	*utf8_text++ = ch;
    else if ( ch<=0x7ff ) {
	*utf8_text++ = 0xc0 | (ch>>6);
	*utf8_text++ = 0x80 | (ch&0x3f);
    } else if ( ch<=0xffff ) {
	*utf8_text++ = 0xe0 | (ch>>12);
	*utf8_text++ = 0x80 | ((ch>>6)&0x3f);
	*utf8_text++ = 0x80 | (ch&0x3f);
    } else {
	uint32 val = ch-0x10000;
	int u = ((val&0xf0000)>>16)+1, z=(val&0x0f000)>>12, y=(val&0x00fc0)>>6, x=val&0x0003f;
	*utf8_text++ = 0xf0 | (u>>2);
	*utf8_text++ = 0x80 | ((u&3)<<4) | z;
	*utf8_text++ = 0x80 | y;
	*utf8_text++ = 0x80 | x;
    }
return( utf8_text );
}


char *utf8_ib(char *utf8_text) {
    int ch;

    /* Increment character */
    if ( (ch = *utf8_text)=='\0' )
return( utf8_text );
    else if ( ch<=127 )
return( utf8_text+1 );
    else if ( ch<0xe0 )
return( utf8_text+2 );
    else if ( ch<0xf0 )
return( utf8_text+3 );
    else
return( utf8_text+4 );
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
    /* Decrement utf8 pointer */
    unsigned char *pt = (unsigned char *) utf8_text;

    --pt;
    if ( *pt>=0xc0 )
	/* This should never happen. The pointer was looking at an intermediate */
	/*  character. However, if it does happen then we are now properly */
	/*  positioned at the start of a new char */;
    else if ( *pt>=0x80 ) {
	--pt;
	if ( *pt>=0xc0 )
	    /* Done */;
	else if ( *pt>=0x80 ) {
	    --pt;
	    if ( *pt>=0xc0 )
		/* Done */;
	    else if ( *pt>=0x80 )
		--pt;
	}
    }
return( (char *) pt );
}

int utf8_strlen(const char *utf8_str) {
    /* how many characters in the string NOT bytes */
    int len = 0;

    while ( utf8_ildb(&utf8_str)>0 )
	++len;
return( len );
}

int utf82u_strlen(const char *utf8_str) {
    /* how many shorts needed to represent it in UCS2 */
    int ch;
    int len = 0;

    while ( (ch = utf8_ildb(&utf8_str))>0 )
	if ( ch>0x10000 )
	    len += 2;
	else
	    ++len;
return( len );
}

#include <chardata.h>
char *StripToASCII(const char *utf8_str) {
    /* Remove any non-ascii characters: Special case, convert the copyright symbol to (c) */
    char *newcr, *pt, *end;
    int len, ch;
    const unichar_t *alt;

    len = strlen(utf8_str);
    pt = newcr = galloc(len+1);
    end = pt+len;
    while ( (ch= utf8_ildb(&utf8_str))!='\0' ) {
	if ( pt>=end ) {
	    int off = pt-newcr;
	    newcr = grealloc(newcr,(off+10)+1);
	    pt = newcr+off;
	    end = pt+10;
	}
	if ( (ch>=' ' && ch<'\177' ) || ch=='\n' || ch=='\t' )
	    *pt++ = ch;
	else if ( ch=='\r' && *utf8_str!='\n' )
	    *pt++ = '\n';
	else if ( ch==0xa9 /* Copyright sign */ ) {
	    char *str = "(c)";
	    if ( pt+strlen(str)>=end ) {
		int off = pt-newcr;
		newcr = grealloc(newcr,(off+10+strlen(str))+1);
		pt = newcr+off;
		end = pt+10;
	    }
	    while ( *str )
		*pt++ = *str++;
	} else if ( unicode_alternates[ch>>8]!=NULL &&
		(alt = unicode_alternates[ch>>8][ch&0xff])!=NULL ) {
	    while ( *alt!='\0' ) {
		if ( pt>=end ) {
		    int off = pt-newcr;
		    newcr = grealloc(newcr,(off+10)+1);
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
    for ( ; *txt!='\0'; ++txt ) {
	if ( *txt=='\t' || *txt=='\n' || *txt=='\r' )
	    /* All right */;
	else if ( *txt<' ' || *txt>='\177' )
return( false );
    }
return( true );
}

int uAllAscii(const unichar_t *txt) {
    for ( ; *txt!='\0'; ++txt ) {
	if ( *txt=='\t' || *txt=='\n' || *txt=='\r' )
	    /* All right */;
	else if ( *txt<' ' || *txt>='\177' )
return( false );
    }
return( true );
}
