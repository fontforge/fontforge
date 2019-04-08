/* Copyright (C) 2004-2012 by George Williams */
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
#ifndef _GWWICONV_H
#define _GWWICONV_H

# ifndef HAVE_ICONV_H
#  define __need_size_t
#  include <stdlib.h>		/* For size_t */

typedef void *gww_iconv_t;

extern gww_iconv_t gww_iconv_open(const char *toenc,const char *fromenc);
extern void gww_iconv_close( gww_iconv_t cd);
extern size_t gww_iconv( gww_iconv_t cd,
        char **inbuf, size_t *inlen,
        char **outbuf, size_t *outlen);

#  define iconv_t		gww_iconv_t
#  define iconv_open	gww_iconv_open
#  define iconv_close	gww_iconv_close
#  define iconv			gww_iconv

#  define iconv_arg2_t	char **

# else		/* HAVE_ICONV_H */

#  include <iconv.h>
#  ifdef __Mac
#   include <xlocale.h>
#  endif

/* libiconv.h defines iconv as taking a const pointer for inbuf. iconv doesn't*/
/* OH, JOY! A new version of libiconv does not use the const! Even better, the man page says it does */
#  ifdef _LIBICONV_VERSION
#   if _LIBICONV_VERSION >= 0x10B
#    define ICONV_CONST
#   else
#    define ICONV_CONST	const
#   endif
#  else		/* _LIBICONV_VERSION*/
#   define ICONV_CONST
#  endif		/* _LIBICONV_VERSION */

#  define iconv_arg2_t ICONV_CONST char **
# endif		/* HAVE_ICONV_H */

#endif		/* _GWWICONV_H */
