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
#include <errno.h>

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
    if (from == NULL || uto == NULL || n == 0) {
        return uto;
    } else if (is_local_encoding_utf8) {
        return utf82u_strncpy(uto, from, n);
    } else {
        size_t in_left = sizeof(from[0]) * strlen(from), out_left = sizeof(uto[0])*(n-1);
        char *cto = (char *) uto;
        iconv(to_unicode, (iconv_arg2_t) &from, &in_left, &cto, &out_left);
        uto[n - (out_left/sizeof(uto[0])) - 1] = '\0';
    }
    return uto;
}

char *u2def_strncpy(char *to, const unichar_t *ufrom, size_t n) {
    if (ufrom == NULL || to == NULL || n == 0) {
        return to;
    } else if (is_local_encoding_utf8) {
        return u2utf8_strncpy(to, ufrom, n);
    } else {
        size_t in_left = sizeof(ufrom[0]) * u_strlen(ufrom), out_left = sizeof(to[0])*(n-1);
        char *cto = (char *) to;
        iconv(from_unicode, (iconv_arg2_t) &ufrom, &in_left, &cto, &out_left);
        to[n - (out_left/sizeof(to[0])) - 1] = '\0';
    }
    return to;
}

static void* do_iconv(iconv_t cd, const void* inbuf, size_t incount, size_t inunitsize, size_t outunitsize) {
    size_t outbytes = (incount + 1) * outunitsize, outremain = outbytes;
    incount *= inunitsize;
    char *buf = malloc(outbytes), *dst = buf;

    while (incount > 0 && buf) {
        if (iconv(cd, (iconv_arg2_t)&inbuf, &incount, &dst, &outremain) == (size_t)-1) {
            if (errno == E2BIG) {
                buf = realloc(buf, outbytes*2);
                if (buf) {
                    dst = buf + outbytes;
                    outremain += outbytes;
                    outbytes += outbytes;
                }
            } else {
                free(buf);
                return NULL;
            }
        }
    }

    if (buf) {
        if (outremain < outunitsize) {
            outbytes += outunitsize - outremain;
            outremain = outunitsize;
            buf = realloc(buf, outbytes);
        }
        if (buf) {
            memset(buf + (outbytes - outremain), 0, outremain);
        }
    }

    return buf;
}

unichar_t *def2u_copy(const char *from) {
    if (from == NULL)
        return NULL;
    else if (is_local_encoding_utf8)
        return utf82u_copy(from);
    return do_iconv(to_unicode, from, strlen(from), sizeof(from[0]), sizeof(unichar_t));
}

char *u2def_copy(const unichar_t *ufrom) {
    if (ufrom == NULL)
        return NULL;
    else if (is_local_encoding_utf8)
        return u2utf8_copy(ufrom);
    return do_iconv(from_unicode, ufrom, u_strlen(ufrom), sizeof(ufrom[0]), sizeof(char));
}

char *def2utf8_copy(const char *from) {
    if (from == NULL)
        return NULL;
    else if (is_local_encoding_utf8)
        return copy(from);
    return do_iconv(to_utf8, from, strlen(from), sizeof(from[0]), sizeof(char));
}

char *utf82def_copy(const char *ufrom) {
    if (ufrom == NULL)
        return NULL;
    else if (is_local_encoding_utf8)
        return copy(ufrom);
    return do_iconv(from_utf8, ufrom, strlen(ufrom), sizeof(ufrom[0]), sizeof(char));
}
