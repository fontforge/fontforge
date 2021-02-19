#ifndef FONTFORGE_PLUGIN_H
#define FONTFORGE_PLUGIN_H

#ifndef _NO_PYTHON

#include "ffglib.h"
#include "ffpython.h"

extern int use_plugins;

enum plugin_startup_mode_type { sm_ask, sm_off, sm_on };
extern enum plugin_startup_mode_type plugin_startup_mode;

typedef struct plugin_entry {
    char *name, *package_name, *module_name;
    char *summary, *package_url;
    enum plugin_startup_mode_type startup_mode, new_mode;
    PyObject *pyobj, *entrypoint;
    int is_present, is_well_formed, has_prefs;
} PluginEntry;

extern GList_Glib *plugin_data;

void FreePluginEntry(PluginEntry *pe);
char *pluginStartupModeString(enum plugin_startup_mode_type sm, int global);
char *pluginErrorString(PluginEntry *);
void *getPluginStartupMode(void);
void setPluginStartupMode(void *);
void LoadPlugin(PluginEntry *pe);
void SavePluginConfig();
void pluginDoPreferences(PluginEntry *);

extern void PyFF_ImportPlugins(int no_import);

#endif // _NO_PYTHON
#endif // FONTFORGE_PLUGIN_H
