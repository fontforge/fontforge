/* Copyright (C) 2000,2001 by George Williams */
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
#include <ustring.h>
#include <utype.h>
#include <charset.h>
#include <chardata.h>

/* Does not handle conversions to 2 byte character sets!!!!! */

unichar_t *encoding2u_strncpy(unichar_t *uto, const char *from, int n, enum encoding cs) {
    unichar_t *upt=uto;
    unsigned short *table;

    if ( cs<e_first2byte ) {
	table = unicode_from_alphabets[cs];
	while ( *from && n>0 ) {
	    *upt ++ = table[*(unsigned char *) (from++)];
	    --n;
	}
    } else if ( cs<e_unicode ) {
	*uto = '\0';
return( NULL );
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
    } else
return( NULL );

    if ( n>0 )
	*upt = '\0';

return( uto );
}

char *u2encoding_strncpy(char *to, const unichar_t *ufrom, int n, enum encoding cs) {

    /* we just ignore anything that doesn't fit in the encoding we look at */
    if ( cs<e_first2byte ) {
	struct charmap *table = NULL;
	char *pt = to;
	unsigned char *plane;
	table = alphabets_from_unicode[cs];
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
	if ( n>0 )
	    *pt = '\0';
    } else if ( cs<e_unicode ) {
	*to = '\0';
return( NULL );
    } else if ( cs==e_unicode ) {
	unichar_t *uto = (unichar_t *) to;
	while ( *ufrom && n>1 ) {
	    *uto++ = *ufrom++;
	    n-=2;
	}
	if ( n>1 )
	    *uto = '\0';
    } else if ( cs==e_unicode_backwards ) {
	unichar_t *uto = (unichar_t *) to;
	while ( *ufrom && n>1 ) {
	    unichar_t ch = (*ufrom>>8)||((*ufrom&0xff)<<8);
	    *uto++ = ch;
	    ++ufrom;
	    n-=2;
	}
	if ( n>1 )
	    *uto = '\0';
    } else
return( NULL );

return( to );
}

unichar_t *def2u_strncpy(unichar_t *uto, const char *from, int n) {
return( encoding2u_strncpy(uto,from,n,local_encoding));
}

char *u2def_strncpy(char *to, const unichar_t *ufrom, int n) {
return( u2encoding_strncpy(to,ufrom,n,local_encoding));
}

unichar_t *def2u_copy(const char *from) {
    int len;
    unichar_t *uto, *ret;

    if ( from==NULL )
return( NULL );
    len = sizeof(unichar_t)*strlen(from);
    uto = galloc((len+1)*sizeof(unichar_t));
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

    if ( ufrom==NULL )
return( NULL );
    len = u_strlen(ufrom);
    if ( local_encoding>=e_first2byte )
	len = 2*len;
    to = galloc(len+2);
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
