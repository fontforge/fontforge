/* Copyright (C) 2007-2012 by George Williams */
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

#include "cvundoes.h"
#include "fontforgeui.h"
#include "usermenu.h"
#include "ustring.h"

typedef struct menu_info {
    menu_info_func func;
    menu_info_check check_enabled;
    menu_info_data data;
} MenuInfo;

static MenuInfo *cv_menu_data = NULL;
static MenuInfo *fv_menu_data = NULL;
static int cv_menu_cnt = 0;
static int cv_menu_max = 0;
static int fv_menu_cnt = 0;
static int fv_menu_max = 0;

GMenuItem2 *cv_menu;
GMenuItem2 *fv_menu;

static void
tl2listcheck(struct gmenuitem *mi,
             void *owner,
             struct menu_info *menu_data,
             int menu_cnt)
{
    int result;

    if (menu_data==NULL)
        return;

    for (mi = mi->sub; mi->ti.text != NULL || mi->ti.line; ++mi) {
        if (mi->mid == -1)		/* Submenu */
            continue;
        if (mi->mid < 0 || mi->mid >= menu_cnt) {
            fprintf(stderr, "Bad Menu ID in python menu %d\n", mi->mid);
            mi->ti.disabled = true;
            continue;
        }
        if (menu_data[mi->mid].check_enabled == NULL) {
            mi->ti.disabled = false;
            continue;
        }
        result = (*menu_data[mi->mid].check_enabled)(menu_data[mi->mid].data, owner);
        mi->ti.disabled = (result == 0);
    }
}

void
cv_tl2listcheck(GWindow gw, struct gmenuitem *mi, GEvent *e)
{
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if (cv_menu_data != NULL) {
        sc_active_in_ui = cv->b.sc;
        layer_active_in_ui = CVLayer((CharViewBase *) cv);
        tl2listcheck(mi, cv->b.sc, cv_menu_data, cv_menu_cnt);
        sc_active_in_ui = NULL;
        layer_active_in_ui = ly_fore;
    }
}

void
fv_tl2listcheck(GWindow gw, struct gmenuitem *mi, GEvent *e)
{
    FontViewBase *fv = (FontViewBase *) GDrawGetUserData(gw);

    if (fv_menu_data != NULL) { 
        fv_active_in_ui = fv;
        layer_active_in_ui = fv->active_layer;
        tl2listcheck(mi, fv, fv_menu_data, fv_menu_cnt);
        fv_active_in_ui = NULL;
    }
}

static void
menuactivate(struct gmenuitem *mi,
             void *owner,
             MenuInfo *menu_data,
             int menu_cnt)
{
    if (mi->mid == -1)          /* Submenu */
        return;
    if (mi->mid < 0 || mi->mid >= menu_cnt) {
        fprintf(stderr, "Bad Menu ID in python menu %d\n", mi->mid);
        return;
    }
    if (menu_data[mi->mid].func == NULL) {
        return;
    }
    (*menu_data[mi->mid].func)(menu_data[mi->mid].data, owner);
}

static void
cv_menuactivate(GWindow gw, struct gmenuitem *mi, GEvent *e)
{
    CharView *cv = (CharView *) GDrawGetUserData(gw);

    if (cv_menu_data != NULL) {
        sc_active_in_ui = cv->b.sc;
        layer_active_in_ui = CVLayer((CharViewBase *) cv);
        menuactivate(mi, cv->b.sc, cv_menu_data, cv_menu_cnt);
        sc_active_in_ui = NULL;
        layer_active_in_ui = ly_fore;
    }
}

static void
fv_menuactivate(GWindow gw, struct gmenuitem *mi, GEvent *e)
{
    FontViewBase *fv = (FontViewBase *) GDrawGetUserData(gw);

    if (fv_menu_data!=NULL) {
        fv_active_in_ui = fv;
        layer_active_in_ui = fv->active_layer;
        menuactivate(mi, fv, fv_menu_data, fv_menu_cnt);
        fv_active_in_ui = NULL;
    }
}

static int
MenuDataAdd(menu_info_func func, menu_info_check check, menu_info_data data, int is_cv)
{
    int index;

    if (is_cv) {
        if (cv_menu_cnt >= cv_menu_max)
            cv_menu_data = realloc(cv_menu_data,(cv_menu_max+=10)*sizeof(struct menu_info));
        cv_menu_data[cv_menu_cnt].func = func;
        cv_menu_data[cv_menu_cnt].check_enabled = check;
        cv_menu_data[cv_menu_cnt].data = data;
        index = cv_menu_cnt;
        cv_menu_cnt++;
    } else {
        if (fv_menu_cnt >= fv_menu_max)
            fv_menu_data = realloc(fv_menu_data,(fv_menu_max+=10)*sizeof(struct menu_info));
        fv_menu_data[fv_menu_cnt].func = func;
        fv_menu_data[fv_menu_cnt].check_enabled = check;
        fv_menu_data[fv_menu_cnt].data = data;
        index = fv_menu_cnt;
        fv_menu_cnt++;
    }
    return index;
}

static void
InsertSubMenus(menu_info_func func,
               menu_info_check check,
               menu_info_data data,
               char *shortcut_str,
               char **submenu_names,
               GMenuItem2 **mn,
               int is_cv)
{
    int i;
    int j;
    GMenuItem2 *mmn;

    for (i = 0; submenu_names[i] != NULL; ++i) {
        unichar_t *submenuu = utf82u_copy(submenu_names[i]);

        j = 0;
        if (*mn != NULL) {
            for (j = 0; (*mn)[j].ti.text != NULL || (*mn)[j].ti.line; ++j) {
                if ((*mn)[j].ti.text == NULL)
                    continue;
                if (u_strcmp((*mn)[j].ti.text, submenuu) == 0)
                    break;
            }
        }
        
        if (*mn == NULL || (*mn)[j].ti.text == NULL) {
            *mn = realloc(*mn,(j+2)*sizeof(GMenuItem2));
            memset(*mn+j,0,2*sizeof(GMenuItem2));
        }
        mmn = *mn;
        if (mmn[j].ti.text == NULL) {
            mmn[j].ti.text = submenuu;
            mmn[j].ti.fg = mmn[j].ti.bg = COLOR_DEFAULT;
            if (submenu_names[i + 1] != NULL) {
                mmn[j].mid = -1;
                mmn[j].moveto = is_cv ? cv_tl2listcheck : fv_tl2listcheck;
                mn = &mmn[j].sub;
            } else {
                mmn[j].shortcut = copy(shortcut_str);
                mmn[j].invoke = is_cv ? cv_menuactivate : fv_menuactivate;
                mmn[j].mid = MenuDataAdd(func, check, data, is_cv);
            }
        } else {
            if (submenu_names[i + 1] != NULL)
                mn = &mmn[j].sub;
            else {
                mmn[j].shortcut = copy(shortcut_str);
                mmn[j].invoke = is_cv ? cv_menuactivate : fv_menuactivate;
                mmn[j].mid = MenuDataAdd(func, check, data, is_cv);
                fprintf(stderr, "Redefining menu item %s\n", submenu_names[i]);
                free(submenuu);
            }
        }
    }
}

void
RegisterMenuItem(menu_info_func func,
                 menu_info_check check,
                 menu_info_data data,
                 int flags,
                 char *shortcut_str,
                 char **submenu_names)
{
    if (!no_windowing_ui) {
        if (flags & menu_fv)
            InsertSubMenus(func, check, data, shortcut_str,
                           submenu_names, &fv_menu, false);
        if (flags & menu_cv)
            InsertSubMenus(func, check, data, shortcut_str,
                           submenu_names, &cv_menu, true);
    }
}
