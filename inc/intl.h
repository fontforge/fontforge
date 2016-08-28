/* Copyright (C) 2005-2012 by George Williams */
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

#ifndef _INTL_H
#define _INTL_H

#include "fontforge-config.h"

#if !defined( HAVE_LIBINTL_H )

# define _(str)			(str)
# define P_(str1,str_non1,n)	((n)==1?str1:str_non1)
# define U_(str)		(str)

# ifdef bindtextdomain
#  undef bindtextdomain
# endif
# ifdef bind_textdomain_codeset
#  undef bind_textdomain_codeset
# endif
# ifdef textdomain
#  undef textdomain
# endif

# define bindtextdomain(domain,dir)
# define bind_textdomain_codeset(domain,enc)
# define textdomain(domain)

# define dgettext(domain,str)	(str)

#else /* HAVE_LIBINTL_H */

# include <libintl.h>
# define _(str)			gettext(str)
# define P_(str1,str_non1,n)	ngettext(str1,str_non1,n)
/* For messages including utf8 characters. old xgettexts won't handle them */
/*  so we must do something special. */
# define U_(str)		gettext(str)

#endif /* HAVE_LIBINTL_H */

/* For messages including utf8 sequences that need gettext_noop treatment */
#define NU_(str)	(str)
#define N_(str)		(str)
#define S_(str) sgettext(str)
/* For messages in the shortcuts domain */
#define H_(str)		(str)

extern void GResourceUseGetText(void);
char *sgettext(const char *msgid);

#endif	/* _INTL_H */
