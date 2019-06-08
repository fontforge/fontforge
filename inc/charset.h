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

#ifndef FONTFORGE_CHARSET_H
#define FONTFORGE_CHARSET_H

#if HAVE_ICONV_H
# include <iconv.h>
extern char *iconv_local_encoding_name;
#else
# include "gwwiconv.h"		/* My fake iconv based on encodings in gdraw/gunicode */
#endif

/* ASCII is ISO 646, except the ISO version admits national alternatives */
enum encoding { e_usascii, e_iso646_no, e_iso646_se, e_iso8859_1,
    e_iso8859_2, e_iso8859_3, e_iso8859_4, e_iso8859_5, e_iso8859_6,
    e_iso8859_7, e_iso8859_8, e_iso8859_9, e_iso8859_10,
    e_iso8859_11/* same as TIS */, e_iso8859_13, e_iso8859_14, e_iso8859_15,
    e_iso8859_16,
    e_koi8_r,	/* RFC 1489 */
    e_jis201,	/* 8 bit, ascii & katakana */
    e_win, e_mac,
    e_user,
/* korean appears to fit into the jis/euc encoding schemes */
/* the difference between jis & jis2 is what the output encoding should be (presence of '(') */
    e_jis, e_jis2, e_jiskorean, e_jisgb, e_sjis,	/* multi-byte */
    e_euc, e_euckorean, e_eucgb,
    e_wansung, e_johab,
    e_big5,
    e_big5hkscs,
    e_unicode, e_unicode_backwards,			/* wide chars */
    e_utf7, e_utf8,					/* unicode encodings */
    e_ucs4,						/* 4 byte chars */
    e_notrans,					/* _inch returns 16bits */
    e_encodingmax, e_unknown=-1, e_first2byte=e_jis };

enum charset { em_none = -1,
    em_iso8859_1, em_iso8859_2, em_iso8859_3, em_iso8859_4, em_iso8859_5,
    em_iso8859_6, em_iso8859_7, em_iso8859_8, em_iso8859_9, em_iso8859_10,
    em_iso8859_11/* same as TIS */, em_iso8859_13, em_iso8859_14, em_iso8859_15,
    em_iso8859_16,
    em_koi8_r,
    em_jis201,
    em_win, em_mac, em_symbol, em_zapfding, em_user, em_adobestandard=em_user,
    em_jis208, em_jis212, em_ksc5601, em_gb2312, em_big5, em_big5hkscs,
    em_johab /* Korean*/,
/* 28 */
    em_unicode, em_unicode4, em_gb18030 , em_max, em_first2byte=em_jis208, em_last94x94=em_gb2312 };

extern int /*enum charset*/ local_encoding;

extern struct namemap { const char *name; int map; } encodingnames[];

#endif /* FONTFORGE_CHARSET_H */
