#ifndef FONTFORGE_USERMENU_H
#define FONTFORGE_USERMENU_H

#include "ggadget.h"

extern SplineChar *sc_active_in_ui;
extern FontViewBase *fv_active_in_ui;
extern int layer_active_in_ui;
typedef struct flaglist {
    char *name;
    int flag;
} FlagList;

typedef void (*menu_info_func)(void *, void *);
typedef int (*menu_info_check)(void *, void *);
typedef void *menu_info_data;

enum {
    menu_fv = 0x01,
    menu_cv = 0x02
};

void cv_tl2listcheck(GWindow gw, struct gmenuitem *mi, GEvent *e);
void fv_tl2listcheck(GWindow gw, struct gmenuitem *mi, GEvent *e);
void RegisterMenuItem(menu_info_func func, menu_info_check check,
                      menu_info_data data, int flags,
                      char *shortcut_str, char **submenu_names);

#endif /* FONTFORGE_USERMENU_H */
