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

#include "gfile.h"
#include "uiinterface.h"

int do_plugins = true;

enum plugin_startup_mode_type plugin_startup_mode = sm_ask;
GList_Glib *plugin_data = NULL;

void FreePluginEntry(PluginEntry *pe) {
    free(pe->name);
    free(pe->package_name);
    free(pe->summary);
    free(pe->package_url);
    free(pe->module_name);
    if (pe->pyobj != NULL)
	Py_DECREF(pe->pyobj);
    free(pe);
}

PluginEntry *NewPluginEntry(char *name, char *modname, char *url,
                            enum plugin_startup_mode_type sm) {
    PluginEntry *pe = malloc(sizeof(PluginEntry));
    pe->name = name;
    pe->package_name = NULL;
    pe->summary = NULL;
    pe->module_name = modname;
    pe->package_url = url;
    pe->startup_mode = sm;
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
	    LogError( _("Could not create plugin directory '%s'\n"), buf);
	    return( NULL );
	}
    return buf;
}

char *pluginErrorString(PluginEntry *pe) {
   if ( !pe->is_present )
	return _("Not Found");
   else if ( pe->startup_mode!=sm_on )
	return NULL;
   else if ( pe->is_present && pe->pyobj==NULL )
	return _("Couldn't Load");
   else if ( pe->pyobj!=NULL && !pe->is_well_formed )
	return _("Couldn't Start");
   else
	return NULL;
}

char *pluginStartupModeString(enum plugin_startup_mode_type sm, int global) {
    if ( global ) {
	if ( sm==sm_off )
	    return "Off";
	else if ( sm==sm_on )
	    return "On";
	else if ( global ) 
	    return "Ask";
    } else {
	if ( sm==sm_off )
	    return _("Off");
	else if ( sm==sm_on )
	    return _("On");
	else if ( global ) 
	    return _("Ask");
    }
}


static enum plugin_startup_mode_type pluginStartupModeFromString(char *modestr) {
    if ( modestr==NULL )
	return sm_ask;
    else if ( strcasecmp(modestr, "off")==0 )
	return sm_off;
    else if ( strcasecmp(modestr, "on")==0 )
	return sm_on;
    else
	return sm_ask;
}

void *getPluginStartupMode() {
    return copy(pluginStartupModeString(plugin_startup_mode, true));
}

void setPluginStartupMode(void *modevoid) {
    plugin_startup_mode = pluginStartupModeFromString((char *)modevoid);
}

void SavePluginConfig() {
    GKeyFile *conf = g_key_file_new();
    for (GList_Glib *i = plugin_data; i!=NULL; i=i->next) {
	PluginEntry *pe = (PluginEntry *) i->data;
	if ( pe->startup_mode==sm_ask )
	    continue; // Don't save merely discovered plugin config
	g_key_file_set_string(conf, pe->name, "Module name", pe->module_name);
	g_key_file_set_string(conf, pe->name, "Active", pluginStartupModeString(pe->startup_mode, true));
	if ( pe->package_url!= NULL )
	    g_key_file_set_string(conf, pe->name, "URL", pe->package_url);
    }

    char *pdir = getPluginDirName();
    if ( pdir!=NULL ) {
	char *fname = smprintf("%s/plugin_config.ini", pdir);
	GError *gerror = NULL;
    	g_key_file_save_to_file(conf, fname, &gerror);
	if ( gerror!=NULL ) {
	    LogError(_("Error saving plugin configuration file '%s': %s\n"), fname, gerror->message);
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
		LogError(_("Error reading plugin configuration file '%s': %s\n"), fname, gerror->message);
	    g_error_free(gerror);
	    gerror = NULL;
	} else {
	    groups = g_key_file_get_groups(conf, &glen);
	    for (int i=0; i<glen; ++i) {
		char *modname = g_key_file_get_string(conf, groups[i], "Module name", NULL);
		if ( modname==NULL ) {
		    LogError(_("No module name for '%s' in plugin config -- skipping.\n"), groups[i]);
		    continue;
		}
		char *sm_string = g_key_file_get_string(conf, groups[i], "Active", NULL);
		char *url = g_key_file_get_string(conf, groups[i], "URL", NULL);
		PluginEntry *pe = NewPluginEntry(copy(groups[i]), modname, url, pluginStartupModeFromString(sm_string));
		free(sm_string);
		plugin_data = g_list_append(plugin_data, pe);
	    }
	    g_strfreev(groups);
	}
	free(fname);
	free(pdir);
    }
    g_key_file_free(conf);
}

void LoadPlugin(PluginEntry *pe) {
    PyObject *str, *tmp, *tmp2;
    if ( !pe->is_present || pe->pyobj!=NULL || pe->entrypoint==NULL )
	return;
    str = PyUnicode_FromString("load");
    pe->pyobj = PyObject_CallMethodNoArgs(pe->entrypoint, str);
    Py_DECREF(str);
    if ( pe->pyobj!=NULL ) {
	tmp = PyObject_GetAttrString(pe->pyobj, "fontforge_plugin_init");
	if ( tmp!=NULL && PyFunction_Check(tmp) ) {
	    tmp2 = PyObject_CallNoArgs(tmp);
	    if ( tmp2==NULL ) {
		LogError( _("Skipping plugin %s: module '%s': Error calling 'fontforge_plugin_init' function\n"), pe->name, pe->module_name);
		PyErr_Print();
	    } else {
		pe->is_well_formed = true;
		Py_DECREF(tmp2);
	    }
	    Py_DECREF(tmp);
	} else {
	    LogError( _("Skipping plugin %s: module '%s': Lacks 'fontforge_plugin_init' function\n"), pe->name, pe->module_name);
	    if ( tmp!=NULL )
		Py_DECREF(tmp);
	    else
		PyErr_Clear();
	}
	tmp = PyObject_GetAttrString(pe->pyobj, "fontforge_plugin_config");
	pe->has_prefs = ( tmp!=NULL && PyFunction_Check(tmp) );
	if ( tmp!=NULL )
	    Py_DECREF(tmp);
	else
	    PyErr_Clear();
    } else {
	LogError( _("Skipping plugin %s: module '%s': Could not load.\n"), pe->name, pe->module_name);
	PyErr_Print();
    }
    Py_DECREF(pe->entrypoint);
    pe->entrypoint=NULL;
}

static bool DiscoverPlugins(int no_import) {
    int do_ask = false;
    PyObject *str, *str2, *str3, *iter, *tmp, *tmp2, *entrypoint;
    PyObject *pkgres = PyImport_ImportModule("pkg_resources");
    if ( pkgres==NULL || !PyObject_HasAttrString(pkgres, "iter_entry_points") ) {
	LogError(_("Core python package 'pkg_resources' not found: Cannot discover plugins\n"));
	return false;
    }
    str = PyUnicode_FromString("iter_entry_points");
    str2 = PyUnicode_FromString("fontforge_plugin");
    iter = PyObject_CallMethodOneArg(pkgres, str, str2);
    if ( !PyIter_Check(iter) ) {
	LogError(_("Could not iterate 'fontforge_plugin' entry points.\n"));
	return false;
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
	if ( i!=NULL && pe->pyobj!=NULL ) // Already loaded
	    continue;
	if ( i==NULL ) {
	    pe = NewPluginEntry(copy(name), copy(modname), NULL, plugin_startup_mode);
	    plugin_data = g_list_append(plugin_data, pe);
	}
	Py_DECREF(str);
	Py_DECREF(str2);
	Py_DECREF(tmp);
	Py_DECREF(tmp2);
	pe->is_present = true;
	pe->entrypoint = entrypoint;
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
		    // printf("%s\n", metaline);
		    if ( strncmp(metaline, "Home-page: ", 11)==0 )
			pe->package_url = copy(metaline+11);
		    else if ( strncmp(metaline, "Name: ", 6)==0 )
			pe->package_name = copy(metaline+6);
		    else if ( strncmp(metaline, "Summary: ", 9)==0 )
			pe->summary = copy(metaline+9);
		    Py_DECREF(str);
		    Py_DECREF(tmp);
		}
	    }
	    Py_DECREF(tmp2);
	}
	if ( !no_import && pe->startup_mode==sm_on ) {
	    LoadPlugin(pe);
	} else if ( !no_import && pe->startup_mode==sm_ask )
	    do_ask = true;
	Py_DECREF(dist);
    }
    if (PyErr_Occurred())
	PyErr_Print();
    Py_DECREF(iter);
    Py_DECREF(getmetastr);
    Py_DECREF(pkgres);
    return do_ask;
}

void PyFF_ImportPlugins(int no_import) {
    static int done = 0;

    if ( done==1 )
	return;

    if ( done==0 ) {
	LoadPluginConfig();
    }
    if ( done==0 || (done==-1 && !no_import) ) {
	if ( DiscoverPlugins(no_import) )
	    PluginDlg();
	SavePluginConfig();
    }
    done = no_import ? -1 : 1;
}

void pluginDoPreferences(PluginEntry *pe) {
    if ( pe->pyobj==NULL || !pe->has_prefs )
	return;

    PyObject *tmp = PyObject_GetAttrString(pe->pyobj, "fontforge_plugin_config");
    if ( tmp==NULL ) {
	PyErr_Clear();
	return;
    }
    if ( !PyFunction_Check(tmp) ) {
	Py_DECREF(tmp);
	return;
    }
    PyObject_CallNoArgs(tmp);
    Py_DECREF(tmp);
}

#endif // _NO_PYTHON
