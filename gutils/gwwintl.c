/* Copyright (C) 2000-2008 by George Williams */
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <basics.h>
#include <intl.h>

char *sgettext(const char *msgid) {
    const char *msgval = _(msgid);
    char *found;
    if (msgval == msgid)
	if ( (found = strrchr (msgid, '|'))!=NULL )
	    msgval = found+1;
return (char *) msgval;
}

#if defined( HAVE_LIBINTL_H ) && !defined( NODYNAMIC ) && !defined ( _STATIC_LIBINTL )
#  include <dynamic.h>

static DL_CONST void *libintl = NULL;

static char *(*_bind_textdomain_codeset)(const char *, const char *);
static char *(*_bindtextdomain)(const char *, const char *);
static char *(*_textdomain)(const char *);
static char *(*_gettext)(const char *);
static char *(*_ngettext)(const char *, const char *, unsigned long int);
static char *(*_dgettext)(const char *, const char *);

static int init_gettext(void) {

    if ( libintl == (void *) -1 )
return( false );
    else if ( libintl !=NULL )
return( true );

    libintl = dlopen("libintl" SO_EXT,RTLD_LAZY);
    if ( libintl==NULL ) {
	libintl = (void *) -1;
return( false );
    }

    _bind_textdomain_codeset = (char *(*)(const char *, const char *)) dlsym(libintl,"bind_textdomain_codeset");
    _bindtextdomain = (char *(*)(const char *, const char *)) dlsym(libintl,"bindtextdomain");
    _textdomain = (char *(*)(const char *)) dlsym(libintl,"textdomain");
    _gettext = (char *(*)(const char *)) dlsym(libintl,"gettext");
    _ngettext = (char *(*)(const char *, const char *, unsigned long int)) dlsym(libintl,"ngettext");
    _dgettext = (char *(*)(const char *, const char *)) dlsym(libintl,"dgettext");

    if ( _bind_textdomain_codeset==NULL || _bindtextdomain==NULL ||
	    _textdomain==NULL || _gettext==NULL || _ngettext==NULL ) {
	libintl = (void *) -1;
	fprintf( stderr, "Found a copy of libintl but could not use it.\n" );
return( false );
    }
return( true );
}

char *gwwv_bind_textdomain_codeset(const char *domain, const char *dir) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 )
return( (_bind_textdomain_codeset)(domain,dir));

return( NULL );
}

char *gwwv_bindtextdomain(const char *domain, const char *dir) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 )
return( (_bindtextdomain)(domain,dir));

return( NULL );
}

char *gwwv_textdomain(const char *domain) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 )
return( (_textdomain)(domain));

return( NULL );
}

char *gwwv_gettext(const char *msg) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 )
return( (_gettext)(msg));

return( (char *) msg );
}

char *gwwv_ngettext(const char *msg, const char *pmsg,unsigned long int n) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 )
return( (_ngettext)(msg,pmsg,n));

return( (char *) (n==1?msg:pmsg) );
}

char *gwwv_dgettext(const char *domain, const char *msg) {
    if ( libintl==NULL )
	init_gettext();
    if ( libintl!=(void *) -1 && _dgettext!=NULL )
return( (_dgettext)(domain,msg));

return( (char *) msg );
}
#endif
