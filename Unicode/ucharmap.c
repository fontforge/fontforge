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
#include <assert.h>

#include "gwwiconv.h"
#include "ustring.h"
#include "utype.h"

static iconv_t to_unicode=(iconv_t) (-1), from_unicode=(iconv_t) (-1);
static iconv_t to_utf8=(iconv_t) (-1), from_utf8=(iconv_t) (-1);
static bool is_local_encoding_utf8 = true;

bool SetupUCharMap(const char* unichar_name, const char* local_name, bool is_local_utf8) {
    if (to_unicode != (iconv_t)-1)
        iconv_close(to_unicode);
    if (from_unicode != (iconv_t)-1)
        iconv_close(from_unicode);
    if (to_utf8 != (iconv_t)-1)
        iconv_close(to_utf8);
    if (from_utf8 != (iconv_t)-1)
        iconv_close(from_utf8);

    is_local_encoding_utf8 = is_local_utf8;
    if (is_local_encoding_utf8) {
        return true;
    }

    if ((to_unicode = iconv_open(unichar_name, local_name)) == (iconv_t)-1)
        return false;
    if ((from_unicode = iconv_open(local_name, unichar_name)) == (iconv_t)-1)
        return false;
    if ((to_utf8 = iconv_open("UTF-8", local_name)) == (iconv_t)-1)
        return false;
    if ((from_utf8 = iconv_open(local_name, "UTF-8")) == (iconv_t)-1)
        return false;

    return true;
}

unichar_t *def2u_strncpy(unichar_t *uto, const char *from, size_t n) {
    size_t in_left = n, out_left = sizeof(unichar_t)*n;
    char *cto = (char *) uto;
    if (is_local_encoding_utf8) {
        return utf82u_strncpy(uto, from, n);
    } else if (iconv(to_unicode, (iconv_arg2_t) &from, &in_left, &cto, &out_left) == (size_t)-1) {
        return NULL;
    }
    if ( cto<((char *) uto)+2*n) *cto++ = '\0';
    if ( cto<((char *) uto)+2*n) *cto++ = '\0';
    if ( cto<((char *) uto)+4*n) *cto++ = '\0';
    if ( cto<((char *) uto)+4*n) *cto++ = '\0';
return( uto );
}

char *u2def_strncpy(char *to, const unichar_t *ufrom, size_t n) {
    size_t in_left = sizeof(unichar_t)*n, out_left = n;
    char *cfrom = (char *) ufrom, *cto=to;
    if (is_local_encoding_utf8) {
        return u2utf8_strncpy(to, ufrom, n);
    } else if (iconv(from_unicode, (iconv_arg2_t) &cfrom, &in_left, &cto, &out_left) == (size_t)-1) {
        return NULL;
    }
    if ( cto<to+n ) *cto++ = '\0';
    if ( cto<to+n ) *cto++ = '\0';
    if ( cto<to+n ) *cto++ = '\0';
    if ( cto<to+n ) *cto++ = '\0';
return( to );
}

unichar_t *def2u_copy(const char *from) {
    int len;
    unichar_t *uto;

    if (is_local_encoding_utf8)
        return utf82u_copy(from);

    if ( from==NULL ) return( NULL );
    len = strlen(from);
    uto = (unichar_t *) malloc((len+1)*sizeof(unichar_t));
    if ( uto==NULL ) return( NULL );

    size_t in_left = len, out_left = sizeof(unichar_t)*len;
    char *cto = (char *) uto;
    if (iconv(to_unicode, (iconv_arg2_t) &from, &in_left, &cto, &out_left) == (size_t)-1) {
        free(uto);
        return NULL;
    }
    *cto++ = '\0';
    *cto++ = '\0';
    *cto++ = '\0';
    *cto++ = '\0';
    return( uto );
}

char *u2def_copy(const unichar_t *ufrom) {
    int len;
    char *to;

    if (is_local_encoding_utf8)
        return u2utf8_copy(ufrom);

    if ( ufrom==NULL ) return( NULL );
    len = u_strlen(ufrom);

    size_t in_left = sizeof(unichar_t)*len, out_left = 3*len;
    char *cfrom = (char *) ufrom, *cto;
    cto = to = (char *) malloc(3*len+2);
    if ( cto==NULL ) return( NULL );
    if (iconv(from_unicode, (iconv_arg2_t) &cfrom, &in_left, &cto, &out_left) == (size_t)-1) {
        free(to);
        return NULL;
    }
    *cto++ = '\0';
    *cto++ = '\0';
    *cto++ = '\0';
    *cto++ = '\0';
    return( to );

}

char *def2utf8_copy(const char *from) {
    int len;

    if (is_local_encoding_utf8)
        return copy(from);
    if ( from==NULL ) return( NULL );
    len = strlen(from);

    size_t in_left = len, out_left = 3*(len+1);
    char *cto = (char *) malloc(3*(len+1)), *cret = cto;
    if ( cto==NULL ) return( NULL );
    if (iconv(to_utf8, (iconv_arg2_t) &from, &in_left, &cto, &out_left) == (size_t)-1) {
        free(cto);
        return NULL;
    }
    *cto++ = '\0';
    *cto++ = '\0';
    *cto++ = '\0';
    *cto++ = '\0';
    return( cret );
}

char *utf82def_copy(const char *ufrom) {
    int len;

    if (is_local_encoding_utf8)
        return copy(ufrom);
    if ( ufrom==NULL ) return( NULL );
    len = strlen(ufrom);

    size_t in_left = len, out_left = 3*len;
    char *cfrom = (char *) ufrom, *cto, *to;
    cto = to = (char *) malloc(3*len+2);
    if ( cto==NULL ) return( NULL );
    if (iconv(from_utf8, (iconv_arg2_t) &cfrom, &in_left, &cto, &out_left) == (size_t)-1) {
        free(to);
        return NULL;
    }
    *cto++ = '\0';
    *cto++ = '\0';
    *cto++ = '\0';
    *cto++ = '\0';
    return( to );
}
