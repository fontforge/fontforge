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

/* FIXME: Get rid of NOPLUGIN. */
#if !defined(NOPLUGIN)

#include "plugins.h"
#include "basics.h"
#include <ltdl.h>

/* Load (or fake loading) a named plugin that is somewhere in the path
 * list for lt_dlopenext(). */
void LoadPlugin(const char *dynamic_lib_name) {
    lt_dlhandle plugin;
    int (*init)(void);

    plugin = lt_dlopenext(dynamic_lib_name);
    if ( plugin == NULL ) {
        LogError(_("Failed to dlopen: %s\n%s"), dynamic_lib_name, lt_dlerror());
    } else {
        init = (int (*)(void)) lt_dlsym(plugin,"FontForgeInit");
        if ( init == NULL ) {
            LogError(_("Failed to find init function in %s"), dynamic_lib_name);
            lt_dlclose(plugin);
        } else if ( (*init)()==0 ) { /* Init routine in charge of any
                                      * error messages */
            lt_dlclose(plugin);
        }
    }
}

/* Callback function for LoadPluginDir(). */
static int load_plugin(const char *dynamic_lib_name, void *UNUSED(data)) {
    (void) LoadPlugin(dynamic_lib_name);
    return 0;
}

/* Load all the plugins in the given search_path, if search_path !=
   NULL.  Load plugins in the "user-defined" search path, if
   search_path == NULL. */
void LoadPluginDir(const char *search_path) {
    const char *path = (search_path == NULL) ? lt_dlgetsearchpath() : search_path;
    (void) lt_dlforeachfile(path, load_plugin, NULL);
}

#endif /* !defined(NOPLUGIN) */
