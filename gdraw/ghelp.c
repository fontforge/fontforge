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

#include "gfile.h"
#include "ustring.h"

#include <gio/gio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

static char* SupportedLocale(const char *locale, const char *fullspec, const char *filename) {
/* If there's additional help files written for other languages, then check */
/* to see if this local matches the additional help message language. If so */
/* then report back that there's another language available to use for help */
/* NOTE: If Docs are not maintained very well, maybe comment-out lang here. */
    int i;
    /* list languages in specific to generic order, ie: en_CA, en_GB, en... */
    static const char *supported[] = { "de","ja", NULL }; /* other html lang list */

    for ( i=0; supported[i]!=NULL; ++i ) {
        if ( strcmp(locale,supported[i])==0 ) {
            const char *pt = strrchr(filename, '/');
            return smprintf("%s/old/%s/%s", fullspec, supported[i], pt ? pt : filename);
        }
    }
    return NULL;
}

static char* CheckSupportedLocale(const char *fullspec, const char *filename) {
/* Add Browser HELP for this local if there's more html docs for this local */

    /* KANOU has provided a japanese translation of the docs */
    /* Edward Lee is working on traditional chinese docs */
    const char *loc = getenv("LC_ALL");
    char buffer[40], *pt;

    if ( loc==NULL ) loc = getenv("LC_CTYPE");
    if ( loc==NULL ) loc = getenv("LANG");
    if ( loc==NULL ) loc = getenv("LC_MESSAGES");
    if ( loc==NULL )
        return NULL;

    /* first, try checking entire string */
    strncpy(buffer,loc,sizeof(buffer));
    buffer[sizeof(buffer)-1] = '\0';
    pt = SupportedLocale(buffer, fullspec, filename);
    if (pt) {
        return pt;
    }

    /* parse possible suffixes, such as .UTF-8, then try again */
    if ( (pt=strchr(buffer,'.'))!=NULL ) {
        *pt = '\0';
        pt = SupportedLocale(buffer, fullspec, filename);
        if (pt) {
            return pt;
        }
    }

    /* parse possible suffixes such as _CA, _GB, and try again */
    if ( (pt=strchr(buffer,'_'))!=NULL ) {
        *pt = '\0';
        return SupportedLocale(buffer, fullspec, filename);
    }

    return NULL;
}

void help(const char *file, const char *section) {
    if (!file) {
        return;
    } else if (strstr(file, "://")) {
        g_app_info_launch_default_for_uri(file, NULL, NULL);
        return;
    } else if (!section) {
        section = "";
    }

    bool launched = false;
    const char *helpdir = getHelpDir();
    if (helpdir) {
        char *path = CheckSupportedLocale(helpdir, file);
        if (!path) {
            path = smprintf("%s/%s", helpdir, file);
            if (!path) {
                return;
            }
        }

        GFile *gfile = g_file_new_for_path(path);
        free(path);

        if (g_file_query_exists(gfile, NULL)) {
            gchar* uri = g_file_get_uri(gfile);
            path = smprintf("%s%s", uri, section);
            launched = g_app_info_launch_default_for_uri(path, NULL, NULL);
            g_free(uri);
            free(path);
        }
        g_object_unref(gfile);
    }

    if (!launched) {
        char *path = smprintf("https://fontforge.org/docs/%s%s", file, section);
        g_app_info_launch_default_for_uri(path, NULL, NULL);
        free(path);
    }
}
