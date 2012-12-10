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

#include "plugins.h"
#include "pluginloading.h"
#include <basics.h>

/* Load (or fake loading) a named plugin that is somewhere in the path
 * list for lt_dlopenext(). Returns 'true' on success; otherwise
 * returns 'false'. */
int LoadPlugin(const char *dynamic_lib_name) {
    lt_dlhandle plugin;
    plugin = load_plugin(dynamic_lib_name, LogError);
    return (plugin != NULL);
}

/* Callback function for LoadPluginDir(). */
static int plugin_loading_callback(const char *dynamic_lib_name, void *UNUSED(data)) {
    (void) LoadPlugin(dynamic_lib_name);
    return 0;
}

/* Load all the plugins in the given search_path, if search_path !=
   NULL.  Load plugins in the "user-defined" search path, if
   search_path == NULL. */
void LoadPluginDir(const char *search_path) {
    const char *path = (search_path == NULL) ? lt_dlgetsearchpath() : search_path;
    (void) lt_dlforeachfile(path, plugin_loading_callback, NULL);
}
