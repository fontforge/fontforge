#ifndef FONTFORGE_PLUGIN_H
#define FONTFORGE_PLUGIN_H

void *getPluginStartupMode(void);
void setPluginStartupMode(void *);

extern void PyFF_ImportPlugins(int no_import);

#endif /* FONTFORGE_PLUGIN_H */
