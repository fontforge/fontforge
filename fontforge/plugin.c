/* Copyright (C) 2000-2012 by George Williams, 2021 Skef Iterum */
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

#ifndef _NO_PYTHON

#include <fontforge-config.h>

#include "plugin.h"

#include "ffglib.h"
#include "ffpython.h"
#include "gfile.h"

int do_plugins = true;

static enum plugin_startup_mode_type { sm_ask, sm_off, sm_on } plugin_startup_mode = sm_ask;

typedef struct plugin_entry {
    char *name, *package_url, *module_name;
    PyObject *pyobj;
    int is_present, is_well_formed, has_prefs;
} PluginEntry;

static GList_Glib *plugin_data = NULL;

void FreePluginEntry(gpointer data) {
    PluginEntry *pe = (PluginEntry *) data;
    free(pe->name);
    free(pe->package_url);
    free(pe->module_name);
    if (pe->pyobj != NULL)
	Py_DECREF(pe->pyobj);
    free(pe);
}

PluginEntry *NewPluginEntry(char *name, char *modname, char *url) {
    PluginEntry *pe = malloc(sizeof(PluginEntry));
    pe->name = name;
    pe->module_name = modname;
    pe->package_url = url;
    pe->pyobj = NULL;
    pe->is_present = false;
    pe->is_well_formed = true;
    pe->has_prefs = false;
    return pe;
}

static char *getPluginDirName() {
    char *buf, *dir=getFontForgeUserDir(Config);

    if ( dir==NULL )
	return NULL;

    buf = smprintf("%s/plugin", dir);
    free(dir);
    if ( access(buf, F_OK)==-1 )
	if ( GFileMkDir(buf, 0755)==-1 ) {
	    fprintf(stderr, "Could not create plugin directory '%s'\n", buf);
	    return( NULL );
	}
    return buf;
}

void *getPluginStartupMode() {
    if ( plugin_startup_mode==sm_off ) {
	return (void *) copy("Off");
    } else if ( plugin_startup_mode==sm_on ) {
	return (void *) copy("On");
    } else {
	return (void *) copy("Ask");
    }
}

void setPluginStartupMode(void *modevoid) {
    char *modestr = (char *)modevoid;
    if ( strcasecmp(modestr, "off")==0 )
	plugin_startup_mode = sm_off;
    else if ( strcasecmp(modestr, "on")==0 )
	plugin_startup_mode = sm_on;
    else
	plugin_startup_mode = sm_ask;
}

static void SavePluginConfig() {
    GKeyFile *conf = g_key_file_new();
    for (GList_Glib *i = plugin_data; i!=NULL; i=i->next) {
	PluginEntry *pe = (PluginEntry *) i->data;
	g_key_file_set_string(conf, pe->name, "Module name", pe->module_name);
	if ( pe->package_url!= NULL )
	    g_key_file_set_string(conf, pe->name, "URL", pe->package_url);
    }

    char *pdir = getPluginDirName();
    if ( pdir!=NULL ) {
	char *fname = smprintf("%s/plugin_config.ini", pdir);
	GError *gerror = NULL;
    	g_key_file_save_to_file(conf, fname, &gerror);
	if ( gerror!=NULL ) {
	    fprintf(stderr, "Error saving plugin configuration file '%s': %s\n", fname, gerror->message);
	    g_error_free(gerror);
	}
	free(fname);
	free(pdir);
    }
    g_key_file_free(conf);
}

static void LoadPluginConfig() {
    GKeyFile *conf = g_key_file_new();
    gchar **groups;
    gsize glen;

    char *pdir = getPluginDirName();
    if ( pdir!=NULL ) {
	char *fname = smprintf("%s/plugin_config.ini", pdir);
	GError *gerror = NULL;
	g_key_file_load_from_file(conf, fname, G_KEY_FILE_NONE, &gerror);
	if ( gerror!=NULL ) {
	    if ( !g_error_matches(gerror, G_FILE_ERROR, G_FILE_ERROR_NOENT) )
		fprintf(stderr, "Error reading plugin configuration file '%s': %s\n", fname, gerror->message);
	    g_error_free(gerror);
	    gerror = NULL;
	} else {
	    groups = g_key_file_get_groups(conf, &glen);
	    for (int i=0; i<glen; ++i) {
		char *modname = g_key_file_get_string(conf, groups[i], "Module name", NULL);
		if ( modname==NULL ) {
		    fprintf(stderr, "No module name for '%s' in plugin config -- skipping.\n", groups[i]);
		    continue;
		}
		char *url = g_key_file_get_string(conf, groups[i], "URL", NULL);
		PluginEntry *pe = NewPluginEntry(copy(groups[i]), modname, url);
		plugin_data = g_list_append(plugin_data, pe);
	    }
	    g_strfreev(groups);
	}
	free(fname);
	free(pdir);
    }
    g_key_file_free(conf);
}

void DiscoverPlugins(int no_import) {
    PyObject *str, *str2, *str3, *iter, *tmp, *tmp2, *entrypoint;
    PyObject *pkgres = PyImport_ImportModule("pkg_resources");
    if ( pkgres==NULL || !PyObject_HasAttrString(pkgres, "iter_entry_points") ) {
	fprintf(stderr, "Core python package 'pkg_resources' not found: Cannot discover plugins\n");
	return;
    }
    str = PyUnicode_FromString("iter_entry_points");
    str2 = PyUnicode_FromString("fontforge_plugin");
    iter = PyObject_CallMethodOneArg(pkgres, str, str2);
    if ( !PyIter_Check(iter) ) {
	fprintf(stderr, "Could not iterate 'fontforge_plugin' entry points.\n");
	return;
    }
    Py_DECREF(str);
    Py_DECREF(str2);

    PyObject *getmetastr = PyUnicode_FromString("get_metadata_lines");
    while ( (entrypoint = PyIter_Next(iter)) ) {
	// Find name and module_name
	str = PyObject_GetAttrString(entrypoint, "name");
	if ( str==NULL )
	    continue;
	str2 = PyObject_GetAttrString(entrypoint, "module_name");
	if ( str2==NULL ) {
	    Py_DECREF(str);
	    continue;
	}
	tmp = PyUnicode_AsASCIIString(str);
	char *name = PyBytes_AsString(tmp);
	tmp2 = PyUnicode_AsASCIIString(str2);
	char *modname = PyBytes_AsString(tmp2);
	// Find entry in current list or add entry to end
	GList_Glib *i;
	PluginEntry *pe = NULL;
	for (i=plugin_data; i!=NULL; i=i->next) {
	    pe = (PluginEntry *)i->data;
	    if (strcmp(name, pe->name)==0 && strcmp(modname, pe->module_name)==0 )
		break;
	}
	if ( i==NULL ) {
	    pe = NewPluginEntry(copy(name), copy(modname), NULL);
	    plugin_data = g_list_append(plugin_data, pe);
	}
	Py_DECREF(str);
	Py_DECREF(str2);
	Py_DECREF(tmp);
	Py_DECREF(tmp2);
	pe->is_present = true;
	// Extract project URL from package data
	PyObject *dist = PyObject_GetAttrString(entrypoint, "dist");
	if ( dist!=NULL ) {
	    tmp = PyObject_GetAttrString(dist, "PKG_INFO");
	    tmp2 = PyObject_CallMethodOneArg(dist, getmetastr, tmp);
	    Py_DECREF(tmp);
	    if ( PyIter_Check(tmp2) ) {
		while ( (str = PyIter_Next(tmp2)) ) {
		    tmp = PyUnicode_AsASCIIString(str);
		    char *metaline = PyBytes_AsString(tmp);
		    if ( strncmp(metaline, "Home-page: ", 11)==0 )
			pe->package_url = copy(metaline+11);
		    Py_DECREF(str);
		    Py_DECREF(tmp);
		}
	    }
	    Py_DECREF(tmp2);
	}
	int do_load = false;
	if ( !no_import && plugin_startup_mode==sm_on )
	    do_load = true;
	else if ( !no_import && plugin_startup_mode==sm_ask )
	    // XXX implement ask code
	    ;
	if ( do_load ) {
	    str = PyUnicode_FromString("load");
	    pe->pyobj = PyObject_CallMethodNoArgs(entrypoint, str);
	    Py_DECREF(str);
	    tmp = PyObject_GetAttrString(pe->pyobj, "init_fontforge_plugin");
	    if ( tmp!=NULL && PyFunction_Check(tmp) ) {
		pe->is_well_formed = true;
		PyObject_CallNoArgs(tmp);
		Py_DECREF(tmp);
	    } else if ( tmp!=NULL ) {
		Py_DECREF(tmp);
	    }
	    tmp = PyObject_GetAttrString(pe->pyobj, "fontforge_plugin_configure");
	    pe->has_prefs = ( tmp!=NULL && PyFunction_Check(tmp) );
	    if ( tmp!=NULL )
		Py_DECREF(tmp);
	}
	Py_DECREF(dist);
	Py_DECREF(entrypoint);
    }
    Py_DECREF(iter);
    Py_DECREF(getmetastr);
    Py_DECREF(pkgres);
}

void PyFF_ImportPlugins(int no_import) {
    static int done = 0;

    if ( done==1 )
	return;

    if ( done==0 ) {
	LoadPluginConfig();
    }
    if ( done==0 || (done==-1 && !no_import) ) {
	DiscoverPlugins(no_import);
	SavePluginConfig();
    }
    done = no_import ? -1 : 1;
}

#endif // _NO_PYTHON
