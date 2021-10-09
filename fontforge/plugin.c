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


#include <fontforge-config.h>

#ifndef _NO_PYTHON

#include "plugin.h"

#include "gfile.h"
#include "uiinterface.h"

int use_plugins = true; // Prefs variable
int attempted_plugin_load = false;

enum plugin_startup_mode_type plugin_startup_mode = sm_ask;
GList_Glib *plugin_data = NULL;

void FreePluginEntry(PluginEntry *pe) {
    free(pe->name);
    free(pe->package_name);
    free(pe->summary);
    free(pe->package_url);
    free(pe->module_name);
    free(pe->attrs);
    if (pe->pyobj != NULL) {
        Py_DECREF(pe->pyobj);
    }
    if (pe->entrypoint != NULL) {
        Py_DECREF(pe->entrypoint);
    }
    free(pe);
}

static PluginEntry *NewPluginEntry(const char *name, const char *pkgname,
                                   const char *modname, const char *url,
                                   enum plugin_startup_mode_type sm) {
    PluginEntry *pe = malloc(sizeof(PluginEntry));
    pe->name = copy(name);
    pe->package_name = copy(pkgname);
    pe->summary = NULL;
    pe->module_name = copy(modname);
    pe->attrs = NULL;
    pe->package_url = copy(url);
    pe->startup_mode = sm;
    pe->pyobj = NULL;
    pe->entrypoint = NULL;
    pe->is_present = false;
    pe->is_well_formed = true;
    pe->has_prefs = false;
    return pe;
}

static char *GetPluginDirName() {
    char *buf, *dir = getFontForgeUserDir(Config);

    if (dir == NULL) {
        return NULL;
    }

    buf = smprintf("%s/plugin", dir);
    free(dir);
    if (access(buf, F_OK) == -1)
        if (GFileMkDir(buf, 0755) == -1) {
            LogError(_("Could not create plugin directory '%s'\n"), buf);
            return (NULL);
        }
    return buf;
}

char *PluginInfoString(PluginEntry *pe, int do_new, int *is_err) {
    enum plugin_startup_mode_type sm = do_new ? pe->new_mode : pe->startup_mode;
    int err = true;
    char *r = NULL;
    if (!pe->is_present) {
        r = N_("Not Found");
    } else if (sm != sm_on) {
        err = false;
    } else if (pe->is_present && pe->pyobj == NULL && pe->entrypoint == NULL) {
        r = N_("Couldn't Load");
    } else if (pe->pyobj != NULL && !pe->is_well_formed) {
        r = N_("Couldn't Start");
    } else if (pe->entrypoint != NULL) {
        r = N_("Unloaded");
        err = false;
    } else {
        err = false;
    }
    if (is_err != NULL) {
        *is_err = err;
    }
    return r;
}

char *PluginStartupModeString(enum plugin_startup_mode_type sm, int global) {
    if (sm == sm_off) {
        return N_("Off");
    } else if (sm == sm_on) {
        return N_("On");
    } else {
        return global ? N_("Ask") : N_("New");
    }
}


static enum plugin_startup_mode_type PluginStartupModeFromString(const char *modestr) {
    if (modestr == NULL) {
        return sm_ask;
    } else if (strcasecmp(modestr, "off") == 0) {
        return sm_off;
    } else if (strcasecmp(modestr, "on") == 0) {
        return sm_on;
    } else {
        return sm_ask;
    }
}

void *GetPluginStartupMode() {
    return copy(PluginStartupModeString(plugin_startup_mode, true));
}

void SetPluginStartupMode(void *modevoid) {
    plugin_startup_mode = PluginStartupModeFromString((char *)modevoid);
}

void SavePluginConfig() {
    GKeyFile *conf = g_key_file_new();
    for (GList_Glib *i = plugin_data; i != NULL; i = i->next) {
        PluginEntry *pe = (PluginEntry *) i->data;
        if (pe->startup_mode == sm_ask) {
            continue;    // Don't save merely discovered plugin config
        }
        g_key_file_set_string(conf, pe->name, "Package name", pe->package_name);
        g_key_file_set_string(conf, pe->name, "Module name", pe->module_name);
        g_key_file_set_string(conf, pe->name, "Active", PluginStartupModeString(pe->startup_mode, false));
        if (pe->package_url != NULL) {
            g_key_file_set_string(conf, pe->name, "URL", pe->package_url);
        }
    }

    char *pdir = GetPluginDirName();
    if (pdir != NULL) {
        char *fname = smprintf("%s/plugin_config.ini", pdir);
        GError *gerror = NULL;
        int ok = g_key_file_save_to_file(conf, fname, &gerror);
        if (!ok && gerror != NULL) {
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

    char *pdir = GetPluginDirName();
    if (pdir != NULL) {
        char *fname = smprintf("%s/plugin_config.ini", pdir);
        GError *gerror = NULL;
        g_key_file_load_from_file(conf, fname, G_KEY_FILE_NONE, &gerror);
        if (gerror != NULL) {
            if (!g_error_matches(gerror, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
                LogError(_("Error reading plugin configuration file '%s': %s\n"), fname, gerror->message);
            }
            g_error_free(gerror);
            gerror = NULL;
        } else {
            groups = g_key_file_get_groups(conf, &glen);
            for (int i = 0; i < glen; ++i) {
                char *modname = g_key_file_get_string(conf, groups[i], "Module name", NULL);
                if (modname == NULL) {
                    LogError(_("No module name for '%s' in plugin config -- skipping.\n"), groups[i]);
                    continue;
                }
                char *pkgname = g_key_file_get_string(conf, groups[i], "Package name", NULL);
                char *sm_string = g_key_file_get_string(conf, groups[i], "Active", NULL);
                char *url = g_key_file_get_string(conf, groups[i], "URL", NULL);
                PluginEntry *pe = NewPluginEntry(groups[i], pkgname, modname, url, PluginStartupModeFromString(sm_string));
                g_free(pkgname);
                g_free(sm_string);
                g_free(url);
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
    if (!use_plugins || !pe->is_present || pe->pyobj != NULL || pe->entrypoint == NULL) {
        return;
    }
    str = PyUnicode_FromString("load");
    pe->pyobj = PyObject_CallMethodObjArgs(pe->entrypoint, str, NULL);
    Py_DECREF(str);
    if (pe->pyobj != NULL) {
        tmp = PyObject_GetAttrString(pe->pyobj, "fontforge_plugin_init");
        if (tmp != NULL && PyFunction_Check(tmp)) {
            PyObject *args = PyTuple_New(0);
            PyObject *kwargs = PyDict_New();
            char *b1 = GetPluginDirName(), *b2 = smprintf("%s/%s", b1, pe->name);
            PyObject *dstr = PyUnicode_FromString(b2);
            free(b1);
            free(b2);
            PyDict_SetItemString(kwargs, "preferences_path", dstr);
            tmp2 = PyObject_Call(tmp, args, kwargs);
            if (tmp2 == NULL) {
                LogError(_("Skipping plugin %s: module '%s': Error calling 'fontforge_plugin_init' function\n"), pe->name, pe->module_name);
                PyErr_Print();
            } else {
                pe->is_well_formed = true;
                Py_DECREF(tmp2);
            }
            Py_DECREF(dstr);
            Py_DECREF(kwargs);
            Py_DECREF(args);
            Py_DECREF(tmp);
        } else {
            LogError(_("Skipping plugin %s: module '%s': Lacks 'fontforge_plugin_init' function\n"), pe->name, pe->module_name);
            if (tmp != NULL) {
                Py_DECREF(tmp);
            } else {
                PyErr_Clear();
            }
        }
        tmp = PyObject_GetAttrString(pe->pyobj, "fontforge_plugin_config");
        pe->has_prefs = (tmp != NULL && PyFunction_Check(tmp));
        if (tmp != NULL) {
            Py_DECREF(tmp);
        } else {
            PyErr_Clear();
        }
    } else {
        LogError(_("Skipping plugin %s: module '%s': Could not load.\n"), pe->name, pe->module_name);
        PyErr_Print();
    }
    Py_DECREF(pe->entrypoint);
    pe->entrypoint = NULL;
}

static void ReimportPlugins() {
    GList_Glib *i;
    if (!use_plugins) {
        return;
    }
    for (i = plugin_data; i != NULL; i = i->next) {
        PluginEntry *pe = (PluginEntry *)i->data;
        if (pe->startup_mode == sm_on && pe->is_present && pe->pyobj == NULL) {
            LoadPlugin(pe);
        }
    }
}

static bool DiscoverPlugins(int do_import) {
    int do_ask = false;
    PluginEntry *pe;
    GList_Glib *i;
    PyObject *str, *str2, *iter, *tmp, *tmp2, *entrypoint;
    PyObject *pkgres = PyImport_ImportModule("pkg_resources");
    if (pkgres == NULL || !PyObject_HasAttrString(pkgres, "iter_entry_points")) {
        LogError(_("Core python package 'pkg_resources' not found: Cannot discover plugins\n"));
	PyErr_Clear();
        return false;
    }
    str = PyUnicode_FromString("iter_entry_points");
    str2 = PyUnicode_FromString("fontforge_plugin");
    iter = PyObject_CallMethodObjArgs(pkgres, str, str2, NULL);
    if (!PyIter_Check(iter)) {
        LogError(_("Could not iterate 'fontforge_plugin' entry points.\n"));
        return false;
    }
    Py_DECREF(str);
    Py_DECREF(str2);

    PyObject *getmetastr = PyUnicode_FromString("get_metadata_lines");
    while ((entrypoint = PyIter_Next(iter))) {
        // Find name and module_name
        str = PyObject_GetAttrString(entrypoint, "name");
        const char *name = PyUnicode_AsUTF8(str);
        if (name == NULL) {
            Py_XDECREF(str);
            PyErr_Clear();
            continue;
        }
        str2 = PyObject_GetAttrString(entrypoint, "module_name");
        const char *modname = PyUnicode_AsUTF8(str2);
        if (modname == NULL) {
            Py_XDECREF(str);
            Py_XDECREF(str2);
            PyErr_Clear();
            continue;
        }
        // Find entry in current list or add entry to end
        pe = NULL;
        for (i = plugin_data; i != NULL; i = i->next) {
            pe = (PluginEntry *)i->data;
            if (strcmp(name, pe->name) == 0 && strcmp(modname, pe->module_name) == 0) {
                break;
            }
        }
        if (i != NULL && pe->pyobj != NULL) { // Already loaded
            continue;
        }
        if (i == NULL) {
            pe = NewPluginEntry(name, NULL, modname, NULL, plugin_startup_mode);
            plugin_data = g_list_append(plugin_data, pe);
        }
        Py_DECREF(str);
        Py_DECREF(str2);
        str = PyObject_GetAttrString(entrypoint, "attrs");
        if (str == NULL) {
            PyErr_Clear();
        } else {
            if (pe->attrs) {
                free(pe->attrs);
            }
            const char *attrs = PyUnicode_AsUTF8(str);
            pe->attrs = attrs != NULL ? copy(attrs) : NULL;
        }
        Py_XDECREF(str);
        pe->is_present = true;
        Py_XDECREF(pe->entrypoint);
        pe->entrypoint = entrypoint;
        // Extract project URL from package data
        PyObject *dist = PyObject_GetAttrString(entrypoint, "dist");
        if (dist != NULL) {
            tmp = PyObject_GetAttrString(dist, "PKG_INFO");
            tmp2 = PyObject_CallMethodObjArgs(dist, getmetastr, tmp, NULL);
            Py_DECREF(tmp);
            if (PyIter_Check(tmp2)) {
                while ((str = PyIter_Next(tmp2))) {
                    const char *metaline = PyUnicode_AsUTF8(str);
                    // printf("%s\n", metaline);
                    if (strncmp(metaline, "Home-page: ", 11) == 0) {
                        if (pe->package_url != NULL) {
                            free(pe->package_url);
                        }
                        pe->package_url = copy(metaline + 11);
                    } else if (strncmp(metaline, "Name: ", 6) == 0) {
                        if (pe->package_name != NULL) {
                            free(pe->package_name);
                        }
                        pe->package_name = copy(metaline + 6);
                    } else if (strncmp(metaline, "Summary: ", 9) == 0) {
                        if (pe->summary != NULL) {
                            free(pe->summary);
                        }
                        pe->summary = copy(metaline + 9);
                    }
                    Py_DECREF(str);
                }
            }
            Py_DECREF(tmp2);
        }
        if (do_import && pe->startup_mode == sm_on) {
            LoadPlugin(pe);
        } else if (do_import && pe->startup_mode == sm_ask) {
            do_ask = true;
        }
        Py_XDECREF(dist);
    }
    if (PyErr_Occurred()) {
        PyErr_Print();
    }
    for (i = plugin_data; i != NULL; i = i->next) {
        pe = (PluginEntry *)i->data;
        if (!pe->is_present && pe->startup_mode == sm_on) {
            LogError(_("Warning: Enabled Plugin '%s' was not discovered\n"), pe->name);
        }
    }
    Py_DECREF(iter);
    Py_DECREF(getmetastr);
    Py_DECREF(pkgres);
    return do_ask;
}

void PluginDoPreferences(PluginEntry *pe) {
    if (!use_plugins || pe->pyobj == NULL || !pe->has_prefs) {
        return;
    }

    PyObject *tmp = PyObject_GetAttrString(pe->pyobj, "fontforge_plugin_config");
    if (tmp == NULL) {
        PyErr_Clear();
        return;
    }
    if (!PyFunction_Check(tmp)) {
        Py_DECREF(tmp);
        return;
    }
    PyObject_CallFunctionObjArgs(tmp, NULL);
    Py_DECREF(tmp);
}

void PyFF_ImportPlugins(int do_import) {

    if (!use_plugins) {
        return;
    }

    if (!attempted_plugin_load) {
        LoadPluginConfig();
        int ask = DiscoverPlugins(do_import);
        attempted_plugin_load = true;
        if (ask) {
            PluginDlg();
        }
    } else if (do_import) {
        ReimportPlugins();
    }

    SavePluginConfig();
}

extern PyObject *PyFF_GetPluginInfo(PyObject *UNUSED(noself), PyObject *UNUSED(args)) {
    PyObject *r, *d;
    GList_Glib *l;
    PluginEntry *pe;

    r = PyList_New(0);

    for (l = plugin_data; l != NULL; l = l->next) {
        pe = (PluginEntry *) l->data;
        d = Py_BuildValue("{s:s,s:s,s:s,s:s,s:s,s:s,s:O,s:s,s:s}", "name", pe->name,
                          "enabled", PluginStartupModeString(pe->startup_mode, false),
                          "status", PluginInfoString(pe, false, NULL),
                          "package_name", pe->package_name,
                          "module_name", pe->module_name,
                          "attrs", pe->attrs,
                          "prefs", pe->has_prefs ? Py_True : Py_False,
                          "package_url", pe->package_url,
                          "summary", pe->summary);
        PyList_Append(r, d);
    }
    return r;
}

extern PyObject *PyFF_ConfigurePlugins(PyObject *UNUSED(noself), PyObject *args) {
    PyObject *iter = NULL, *item;
    GList_Glib *l, *nl = NULL;
    PluginEntry *pe;
    int type_error = false;
    if (args != NULL || PyTuple_Check(args) || PyTuple_Size(args) == 1) {
        PyObject *l = PyTuple_GetItem(args, 0);
        if (l != NULL) {
            iter = PyObject_GetIter(l);
        }
    }

    if (iter != NULL) {
        for (l = plugin_data; l != NULL; l = l->next) {
            pe = (PluginEntry *) l->data;
            pe->new_mode = sm_ask;
        }
        while ((item = PyIter_Next(iter))) {
            if (!PyDict_Check(item)) {
                type_error = true;
                break;
            }
            PyObject *nameobj = PyDict_GetItemString(item, "name");
            if (nameobj == NULL || !PyUnicode_Check(nameobj)) {
                type_error = true;
                break;
            }
            const char *namestr = PyUnicode_AsUTF8(nameobj);
            for (l = plugin_data; l != NULL; l = l->next) {
                pe = (PluginEntry *) l->data;
                if (strcasecmp(namestr, pe->name) == 0) {
                    break;
                }
            }
            if (l == NULL) {
                PyErr_Format(PyExc_ValueError, _("'%s' is not the name of a currently known plugin"), namestr);
                g_list_free(g_steal_pointer(&nl));
                return NULL;
            }
            nl = g_list_append(nl, pe);
            PyObject *smobj = PyDict_GetItemString(item, "enabled");
            pe->new_mode = PluginStartupModeFromString(PyUnicode_AsUTF8(smobj));
            if (pe->new_mode == sm_ask) {
                PyErr_Format(PyExc_ValueError, _("Startup mode '%s' (for plugin '%s') must be 'on' or 'off'. (To set a discovered plugin to 'new' omit it from the list.)"), PyUnicode_AsUTF8(smobj), namestr);
                g_list_free(g_steal_pointer(&nl));
                return NULL;
            }
        }
    } else {
        type_error = true;
    }

    if (type_error) {
        PyErr_Format(PyExc_TypeError, _("The single parameter to this method must be an iterable object (such as a list) where each item is a tuple with a plugin name as its first element and 'on' or 'off' as its second element."));
        return NULL;
    }
    for (l = plugin_data; l != NULL; l = l->next) {
        pe = (PluginEntry *) l->data;
        pe->startup_mode = pe->new_mode;
        if (pe->new_mode == sm_ask) {
            if (pe->is_present) {
                nl = g_list_append(nl, pe);
            } else {
                FreePluginEntry(pe);
            }
        }
    }
    g_list_free(plugin_data);
    plugin_data = nl;
    SavePluginConfig();

    return Py_None;
}

#endif // _NO_PYTHON
