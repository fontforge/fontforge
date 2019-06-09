/* Copyright (C) 2000-2002 by George Williams */
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
#include "utype.h"

char *strstart(const char *initial, const char *full) {
    int ch1, ch2;
    for (;;) {
	ch1 = *initial++; ch2 = *full++ ;
	if ( ch1=='\0' )
return( (char *) full );
	if ( ch1!=ch2 || ch1=='\0' )
return(NULL);
    }
}

char *strstartmatch(const char *initial, const char *full) {
    int ch1, ch2;
    for (;;) {
	ch1 = *initial++; ch2 = *full++ ;
	if ( ch1=='\0' )
return( (char *) full );
	ch1 = tolower(ch1);
	ch2 = tolower(ch2);
	if ( ch1!=ch2 || ch1=='\0' )
return(NULL);
    }
}

int strmatch(const char *str1, const char *str2) {
    int ch1, ch2;
    for (;;) {
	ch1 = *str1++; ch2 = *str2++ ;
	ch1 = tolower(ch1);
	ch2 = tolower(ch2);
	if ( ch1!=ch2 || ch1=='\0' )
return(ch1-ch2);
    }
}

int strnmatch(const char *str1, const char *str2, int n) {
    int ch1, ch2;
    for (;n-->0;) {
	ch1 = *str1++; ch2 = *str2++ ;
	ch1 = tolower(ch1);
	ch2 = tolower(ch2);
	if ( ch1!=ch2 || ch1=='\0' )
return(ch1-ch2);
    }
return(0);
}

char *strstrmatch(const char *longer, const char *substr) {
    int ch1, ch2;
    const char *lpt, *str1, *str2;

    for ( lpt=longer; *lpt!='\0'; ++lpt ) {
	str1 = lpt; str2 = substr;
	for (;;) {
	    ch1 = *str1++; ch2 = *str2++ ;
	    ch1 = tolower(ch1);
	    ch2 = tolower(ch2);
	    if ( ch2=='\0' )
return((char *) lpt);
	    if ( ch1!=ch2 )
	break;
	}
    }
return( NULL );
}
