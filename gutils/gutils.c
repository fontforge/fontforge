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

#include "basics.h"
#include "gutils.h"

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ustring.h>
#include <utype.h>
#include <unistd.h>
#include <locale.h>
#if !defined(__MINGW32__)
# include <pwd.h>
#endif
#include <stdarg.h>
#include <time.h>
#include <gdraw.h>		/* For image defn */

#ifdef __CygWin
 #include <sys/types.h>
 #include <sys/stat.h>
 #include <unistd.h>
#endif


const char *GetAuthor(void) {
#if defined(__MINGW32__)
    static char author[200] = { '\0' };
    if ( author[0] == '\0' ){
	char* name = getenv("USER");
	if(!name) return NULL;
	strncpy(author, name, sizeof(author));
	author[sizeof(author)-1] = '\0';
    }
    return author;
#else
    struct passwd *pwd;
    static char author[200] = { '\0' };
    const char *ret = NULL, *pt;

    if ( author[0]!='\0' )
return( author );

/* Can all be commented out if no pwd routines */
    pwd = getpwuid(getuid());
    if ( pwd!=NULL && pwd->pw_gecos!=NULL && *pwd->pw_gecos!='\0' ) {
	strncpy(author,pwd->pw_gecos,sizeof(author));
	author[sizeof(author)-1] = '\0';
	ret = author;
    } else if ( pwd!=NULL && pwd->pw_name!=NULL && *pwd->pw_name!='\0' ) {
	strncpy(author,pwd->pw_name,sizeof(author));
	author[sizeof(author)-1] = '\0';
	ret = author;
    } else if ( (pt=getenv("USER"))!=NULL ) {
	strncpy(author,pt,sizeof(author));
	author[sizeof(author)-1] = '\0';
	ret = author;
    }
    endpwent();
/* End comment */
return( ret );
#endif
}

time_t GetTime(void) {
	time_t now;
	if (getenv("SOURCE_DATE_EPOCH")) {
		now = atol(getenv("SOURCE_DATE_EPOCH"));
	} else {
		now = time(NULL);
	}

	return now;
}
