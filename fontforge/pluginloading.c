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

#include "intl.h"
#include "basics.h"
#include "pluginloading.h"

static int inited = false;

void init_plugins(void) {
    int err;

    if (!inited) {
        err = lt_dlinit();
        if (1 < err) {
            fprintf(stderr, "%d errors encountered during libltdl startup.\n", err);
            abort();
        } else if (1 == err) {
            fprintf(stderr, "1 error encountered during libltdl startup.\n");
            abort();
        }

#ifdef PLUGINDIR
        lt_dladdsearchdir(PLUGINDIR);
#endif /* PLUGINDIR */

        inited = true;
    }
}

int plugins_are_initialized(void) {
    return inited;
}

/* Load (or fake loading) a named plugin that is somewhere in the path
 * list for lt_dlopenext(). Optionally logs a message. Returns the
 * plugin handle, or NULL. */
lt_dlhandle load_plugin(const char *dynamic_lib_name, void (*logger)(const char *fmt,...)) {
    lt_dlhandle plugin;
    int (*init)(void);

    plugin = lt_dlopenext(dynamic_lib_name);
    if ( plugin == NULL ) {
        if (logger != NULL)
            logger(_("Failed to dlopen: %s\n%s"), dynamic_lib_name, lt_dlerror());
    } else {
        init = (int (*)(void)) lt_dlsym(plugin,"FontForgeInit");
        if ( init == NULL ) {
            if (logger != NULL)
                logger(_("Failed to find init function in %s"), dynamic_lib_name);
            lt_dlclose(plugin);
            plugin = NULL;
        } else if ( (*init)()==0 ) { /* Init routine in charge of any
                                      * error messages */
            lt_dlclose(plugin);
            plugin = NULL;
        }
    }
    return plugin;
}
