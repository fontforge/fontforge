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
#include "intl.h"
#include "uiinterface.h"

#include <string>  /* for std::string */
#include <ini.h>   /* mINI library for INI file parsing */

int use_plugins = true; // Prefs variable
int attempted_plugin_load = false;

enum plugin_startup_mode_type plugin_startup_mode = sm_ask;
FFList *plugin_data = NULL;

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
    PluginEntry *pe = (PluginEntry *)malloc(sizeof(PluginEntry));
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
    if (ff_access(buf, F_OK) == -1)
        if (GFileMkDir(buf, 0755) == -1) {
            LogError(_("Could not create plugin directory '%s'"), buf);
            return (NULL);
        }
    return buf;
}

const char *PluginInfoString(PluginEntry *pe, int do_new, int *is_err) {
    enum plugin_startup_mode_type sm = do_new ? pe->new_mode : pe->startup_mode;
    int err = true;
    const char *r = NULL;
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

const char *PluginStartupModeString(enum plugin_startup_mode_type sm, int global) {
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
    mINI::INIStructure ini;

    for (FFList *i = plugin_data; i != NULL; i = i->next) {
        PluginEntry *pe = (PluginEntry *) i->data;
        if (pe->startup_mode == sm_ask) {
            continue;    // Don't save merely discovered plugin config
        }
        std::string section(pe->name);
        if (pe->package_name != NULL) {
            ini[section]["Package name"] = pe->package_name;
        }
        ini[section]["Module name"] = pe->module_name;
        ini[section]["Active"] = PluginStartupModeString(pe->startup_mode, false);
        if (pe->package_url != NULL) {
            ini[section]["URL"] = pe->package_url;
        }
    }

    char *pdir = GetPluginDirName();
    if (pdir != NULL) {
        char *fname = smprintf("%s/plugin_config.ini", pdir);
        mINI::INIFile file(fname);
        if (!file.write(ini)) {
            LogError(_("Error saving plugin configuration file '%s'"), fname);
        }
        free(fname);
        free(pdir);
    }
}

static void LoadPluginConfig() {
    char *pdir = GetPluginDirName();
    if (pdir == NULL) {
        return;
    }

    char *fname = smprintf("%s/plugin_config.ini", pdir);
    mINI::INIFile file(fname);
    mINI::INIStructure ini;

    if (!file.read(ini)) {
        /* File doesn't exist or can't be read - not an error for new installs */
        free(fname);
        free(pdir);
        return;
    }

    for (auto const& section : ini) {
        const std::string& name = section.first;
        const auto& values = section.second;

        if (!values.has("Module name")) {
            LogError(_("No module name for '%s' in plugin config -- skipping."), name.c_str());
            continue;
        }
        std::string modname = values.get("Module name");
        if (modname.empty()) {
            LogError(_("No module name for '%s' in plugin config -- skipping."), name.c_str());
            continue;
        }

        std::string pkgname = values.get("Package name");
        std::string sm_string = values.get("Active");
        std::string url = values.get("URL");

        PluginEntry *pe = NewPluginEntry(
            name.c_str(),
            pkgname.empty() ? NULL : pkgname.c_str(),
            modname.c_str(),
            url.empty() ? NULL : url.c_str(),
            PluginStartupModeFromString(sm_string.empty() ? NULL : sm_string.c_str())
        );
        plugin_data = ff_list_append(plugin_data, pe);
    }

    free(fname);
    free(pdir);
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
                LogError(_("Skipping plugin %s: module '%s': Error calling 'fontforge_plugin_init' function"), pe->name, pe->module_name);
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
            LogError(_("Skipping plugin %s: module '%s': Lacks 'fontforge_plugin_init' function"), pe->name, pe->module_name);
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
        LogError(_("Skipping plugin %s: module '%s': Could not load."), pe->name, pe->module_name);
        PyErr_Print();
    }
    Py_DECREF(pe->entrypoint);
    pe->entrypoint = NULL;
}

static void ReimportPlugins() {
    FFList *i;
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

static PyObject *GetPluginEntryPoints() {
    PyObject *iter = NULL, *all_eps = NULL;
    PyObject *entry_points = NULL;

    PyObject *importlib = PyImport_ImportModule("importlib.metadata");
    if (!importlib) {
        PyErr_Clear();
        LogError(_("Core python package 'importlib.metadata' not found: Cannot discover plugins"));
        return NULL;
    }
    if (!PyObject_HasAttrString(importlib, "entry_points")) {
        LogError(_("Method 'entry_points()' not found in module 'importlib.metadata'"));
	PyErr_Clear();
        Py_DECREF(importlib);
        return NULL;
    }
    all_eps = PyObject_CallMethod(importlib, "entry_points", NULL);

    /* The object returned from importlib.metadata.entry_points() varies between versions*/
    if (PyDict_Check(all_eps)) {
	/* entry_points is a dictionary of entry point groups */
	entry_points = PyDict_GetItemString(all_eps, "fontforge_plugin");
	if (!entry_points) {
	    /* No FontForge plugins found */
	    PyErr_Clear();
            return NULL;
	}
    } else {
        /* all_eps is an EntryPoints object. Call all_eps.select(group="fontforge_plugin") */
        PyObject *select_method = PyObject_GetAttrString(all_eps, "select");
        PyObject *kw_args = Py_BuildValue("{s,s}", "group", "fontforge_plugin");
        entry_points = PyObject_Call(select_method, PyTuple_New(0), kw_args);
        Py_DECREF(select_method);
        Py_DECREF(kw_args);
        if (!entry_points) {
            LogError(_("Failed to retrieve plugin entry points"));
	    PyErr_Print();
            return NULL;
        }
    }

    Py_DECREF(importlib);

    iter = PyObject_GetIter(entry_points);
    if (!iter || !PyIter_Check(iter)) {
	PyErr_Clear();
        LogError(_("Could not iterate 'fontforge_plugin' entry points."));
        Py_XDECREF(iter);
        return NULL;
    }

    return iter;
}

static void RetrieveStringItem(PyObject* obj, const char* key, char** p_value) {
    PyObject* str = PyUnicode_FromString(key);
    PyObject* val = PyObject_GetItem(obj, str);
    if (val) {
        free(*p_value);
        *p_value = copy(PyUnicode_AsUTF8(val));
    }
    Py_XDECREF(str);
    Py_XDECREF(val);
}

/* Retrieve name, URL, and summary of the plugin */
static void LoadPluginMetadata(PluginEntry* pe) {
    PyObject *globals = PyDict_New(), *locals = PyDict_New();
    PyObject *dist = NULL, *function_args = NULL;

    /* The `EntryPoint.dist` attribute is available starting with Python 3.10.
     * To support Python 3.8+ we resort to the ugly but more uniform method of
     * retrieving all `Distribution` objects and traversing them until we find
     * one which points to our `EntryPoint` object. */
    const char* function_string =
        "def load_plugin_metadata(entrypoint):\n"
        "    import importlib.metadata\n"
        "    all_dists = importlib.metadata.distributions()\n"
        "    for dist in all_dists:\n"
        "        dist_eps = dist.entry_points\n"
        "        for ep in dist_eps:\n"
        "            if ep == entrypoint:\n"
        "                return dist\n"
        "    return None\n";

    /* Execute the function definition */
    if (PyRun_String(function_string, Py_file_input, globals, locals) == NULL) {
        Py_DECREF(globals);
        Py_DECREF(locals);
        return;
    }

    /* Retrieve the function object */
    PyObject* func = PyDict_GetItemString(locals, "load_plugin_metadata");
    if (func == NULL || !PyCallable_Check(func)) {
        PyErr_SetString(PyExc_RuntimeError, "Could not find the function");
        Py_DECREF(globals);
        Py_DECREF(locals);
        return;
    }

    /* Call the function */
    function_args = PyTuple_Pack(1, pe->entrypoint);
    if (function_args != NULL) {
        dist = PyObject_Call(func, function_args, NULL);
        Py_DECREF(function_args);
    }
    /* Note: func is a borrowed reference from PyDict_GetItemString, don't DECREF */

    Py_DECREF(globals);
    Py_DECREF(locals);

    if (dist == NULL || dist == Py_None) {
        if (dist == NULL) {
            PyErr_Print();
        }
        PyErr_Format(PyExc_RuntimeError,
            "Could not retrieve distribution for plugin '%s'", pe->name);
        Py_XDECREF(dist);
        return;
    }

    if (PyObject_HasAttrString(dist, "metadata")) {
        PyObject* metadata = PyObject_GetAttrString(dist, "metadata");

	RetrieveStringItem(metadata, "Home-page", &pe->package_url);
	RetrieveStringItem(metadata, "Name", &pe->package_name);
	RetrieveStringItem(metadata, "Summary", &pe->summary);
        Py_XDECREF(metadata);
    }

    Py_DECREF(dist);
}

static bool DiscoverPlugins(int do_import) {
    int do_ask = false;
    PluginEntry *pe = NULL;
    FFList *i;
    PyObject *str, *str2, *iter, *entrypoint;

    iter = GetPluginEntryPoints();
    if (iter == NULL) {
	return false;
    }

    while ((entrypoint = PyIter_Next(iter))) {
        // Find name and module_name
        str = PyObject_GetAttrString(entrypoint, "name");
        const char *name = PyUnicode_AsUTF8(str);
        if (name == NULL) {
            Py_XDECREF(str);
            PyErr_Clear();
            continue;
        }
        str2 = PyObject_GetAttrString(entrypoint, "value");
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
            plugin_data = ff_list_append(plugin_data, pe);
        }
        Py_DECREF(str);
        Py_DECREF(str2);
        str = PyObject_GetAttrString(entrypoint, "attr");
        if (str == NULL || str == Py_None) {
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

	LoadPluginMetadata(pe);
        if (do_import && pe->startup_mode == sm_on) {
            LoadPlugin(pe);
        } else if (do_import && pe->startup_mode == sm_ask) {
            do_ask = true;
        }
    }
    if (PyErr_Occurred()) {
        PyErr_Print();
    }
    for (i = plugin_data; i != NULL; i = i->next) {
        pe = (PluginEntry *)i->data;
        if (!pe->is_present && pe->startup_mode == sm_on) {
            LogError(_("Warning: Enabled Plugin '%s' was not discovered"), pe->name);
        }
    }
    Py_DECREF(iter);
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
    FFList *l;
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
    FFList *l, *nl = NULL;
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
                ff_list_free(ff_steal_pointer(&nl));
                return NULL;
            }
            nl = ff_list_append(nl, pe);
            PyObject *smobj = PyDict_GetItemString(item, "enabled");
            pe->new_mode = PluginStartupModeFromString(PyUnicode_AsUTF8(smobj));
            if (pe->new_mode == sm_ask) {
                PyErr_Format(PyExc_ValueError, _("Startup mode '%s' (for plugin '%s') must be 'on' or 'off'. (To set a discovered plugin to 'new' omit it from the list.)"), PyUnicode_AsUTF8(smobj), namestr);
                ff_list_free(ff_steal_pointer(&nl));
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
                nl = ff_list_append(nl, pe);
            } else {
                FreePluginEntry(pe);
            }
        }
    }
    ff_list_free(plugin_data);
    plugin_data = nl;
    SavePluginConfig();

    return Py_None;
}

#endif // _NO_PYTHON
