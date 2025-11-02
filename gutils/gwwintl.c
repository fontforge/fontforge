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

#include <fontforge-config.h>

 
#include "basics.h"
#include "intl.h"

#include <string.h>

char *sgettext(const char *msgid) {
    const char *msgval = _(msgid);
    char *found;
    if (msgval == msgid)
	if ( (found = strrchr (msgid, '|'))!=NULL )
	    msgval = found+1;
return (char *) msgval;
}

char* gettext_locale(char* locale_out) {
    const char *loc = getenv("LANGUAGE");

    if (loc == NULL) loc = getenv("LC_ALL");
    if (loc == NULL) loc = getenv("LC_MESSAGES");
    if (loc == NULL) loc = getenv("LANG");

    /* Fallback for empty environment */
    if (loc == NULL) {
        strcpy(locale_out, "en");
        return locale_out;
    }

    char* dot_position = strchr(loc, '.');
    char* colon_position = strchr(loc, ':');

    // If no dot or colon is found, copy the entire string
    if (dot_position == NULL && colon_position == NULL) {
        strcpy(locale_out, loc);
        return locale_out;
    }

    size_t substring_length = (dot_position == NULL) ? (colon_position - loc) :
        (colon_position == NULL) ? (dot_position - loc) :
        ((colon_position < dot_position) ? (colon_position - loc) : (dot_position - loc));

    strncpy(locale_out, loc, substring_length);
    locale_out[substring_length] = '\0';
    return locale_out;
}
