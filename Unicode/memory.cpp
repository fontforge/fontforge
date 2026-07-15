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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>


void NoMoreMemMessage(void) {
/* Output an 'Out of memory' message, then continue */
    fprintf(stderr, "Out of memory\n" );
}

void ExpandBuffer(void** p_buf, size_t elem_size, size_t increment,
                  int* p_maxalloc) {
    *p_buf = realloc(*p_buf, (*p_maxalloc + increment) * elem_size);
    memset((char*)*p_buf + (*p_maxalloc) * elem_size, 0, increment * elem_size);
    *p_maxalloc += increment;
}

char *copy(const char *str) {
    return str ? strdup(str) : NULL;
}

char *copyn(const char *str,long n) {
    /**
     * MIQ: Note that there is at least one site that relies on
     *      copyn copying up to n bytes including embedded nulls.
     *      So using strndup() doesn't provide the same outcomes
     *      to that code.
     *      https://github.com/fontforge/fontforge/issues/1239
     */
    char *ret;
    if ( str==NULL )
    	return( NULL );

    ret = (char *) malloc(n+1);
    memcpy(ret,str,n);
    ret[n]='\0';
    return( ret );
}

/**
 *  Format a string, allocating a buffer as necessary.
 *  If memory allocation fails, the application is aborted.
 *  \param [in] fmt The printf-like format string
 *  \param [in] args Input arguments to the format string
 *  \return The buffer containing the formatted string.
 *          Must be freed by the caller.
 */
char *vsmprintf(const char *fmt, va_list args) {
    va_list args2;
    char* ret;
    int len;

    va_copy(args2, args);
    len = vsnprintf(NULL, 0, fmt, args2);
    va_end(args2);

    if (len < 0) {
        return NULL;
    }

    ret = (char*)malloc(len + 1);
    if (ret == NULL) {
        perror("smprintf");
        abort();
        return NULL;
    }

    len = vsnprintf(ret, len + 1, fmt, args);

    if (len < 0) {
        free(ret);
        return NULL;
    }

    return ret;
}

/**
 *  Format a string, allocating a buffer as necessary.
 *  See vsmprintf for more details.
 */
char *smprintf(const char* fmt, ...) {
    va_list args;
    char *ret;

    va_start(args, fmt);
    ret = vsmprintf(fmt, args);
    va_end(args);

    return ret;
}

