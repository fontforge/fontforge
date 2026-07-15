#ifndef FONTFORGE_PLUGIN_H
#define FONTFORGE_PLUGIN_H

#ifndef _NO_PYTHON

#include "ffglib_compat.h"
#include "ffpython.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int use_plugins;
extern int attempted_plugin_load;

enum plugin_startup_mode_type { sm_ask, sm_off, sm_on };
extern enum plugin_startup_mode_type plugin_startup_mode;

typedef struct plugin_entry {
    char *name, *package_name, *module_name, *attrs;
    char *summary, *package_url;
    enum plugin_startup_mode_type startup_mode, new_mode;
    PyObject *pyobj, *entrypoint;
    int is_present, is_well_formed, has_prefs;
} PluginEntry;

extern FFList *plugin_data;

void FreePluginEntry(PluginEntry *pe);
const char *PluginStartupModeString(enum plugin_startup_mode_type sm, int global);
const char *PluginInfoString(PluginEntry *pe, int do_new, int *is_err);
void *GetPluginStartupMode(void);
void SetPluginStartupMode(void *);
void LoadPlugin(PluginEntry *pe);
void SavePluginConfig();
void PluginDoPreferences(PluginEntry *);

extern void PyFF_ImportPlugins(int do_import);
extern PyObject *PyFF_GetPluginInfo(PyObject *, PyObject *);
extern PyObject *PyFF_ConfigurePlugins(PyObject *, PyObject *);

#ifdef __cplusplus
}
#endif

#endif // _NO_PYTHON
#endif // FONTFORGE_PLUGIN_H
