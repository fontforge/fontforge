/* Copyright (C) 2000-2012 by George Williams */
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

#include "fontforgeui.h"
#include "gkeysym.h"
#include "plugin.h"

#include "assert.h"

extern GTextInfo *GTextInfoCopy(GTextInfo *ti);

#define CID_SmOn         1000
#define CID_SmOff        1001
#define CID_SmAsk        1002
#define CID_Top          1003
#define CID_Up           1004
#define CID_Down         1005
#define CID_Bottom       1006
#define CID_Enable       1007
#define CID_Disable      1008
#define CID_Delete       1009
#define CID_MoreInfo     1010
#define CID_Load         1011
#define CID_Web          1012
#define CID_Conf         1013
#define CID_Revert       1014
#define CID_PluginList   1015
#define CID_OK           1016
#define CID_Cancel       1017

static int pluginfo_e_h(GWindow gw, GEvent *e) {
    int *done = (int *) GDrawGetUserData(gw);
    if (e->type == et_close) {
        *done = true;
    } else if (e->type == et_char && e->u.chr.keysym == GK_Return) {
        *done = true;
    }
    return true;
}

static int PLUG_Info_OK(GGadget *g, GEvent *e) {
    int *done = (int *) GDrawGetUserData(GGadgetGetWindow(g));
    if (e->type == et_controlevent && e->u.control.subtype == et_buttonactivate) {
        *done = true;
    }
    return true;
}

static void PluginInfoDlg(PluginEntry *pe) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[14], boxes[4], *hvgrid[7][4], *okgrid[7], *mgrid[3];
    GTextInfo label[14];
    int done = false, k;

    if (no_windowing_ui) {
        return;
    }

    memset(&wattrs, 0, sizeof(wattrs));
    wattrs.mask = wam_events | wam_cursor | wam_utf8_wtitle | wam_undercursor | wam_isdlg | wam_restrict;
    wattrs.event_masks = ~(1 << et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Plugin Configuration");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL, GGadgetScale(400));
    pos.height = 0;
    gw = GDrawCreateTopWindow(NULL, &pos, pluginfo_e_h, &done, &wattrs);

    memset(&gcd, 0, sizeof(gcd));
    memset(&boxes, 0, sizeof(boxes));
    memset(&hvgrid, 0, sizeof(hvgrid));
    memset(&label, 0, sizeof(label));

    k = 0;
    label[k].text = (unichar_t *) _("Name:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[0][0] = &gcd[k - 1];

    label[k].text = (unichar_t *) pe->name;
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[0][1] = &gcd[k - 1];
    hvgrid[0][2] = GCD_ColSpan;
    hvgrid[0][3] = NULL;

    label[k].text = (unichar_t *) _("Package Name:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[1][0] = &gcd[k - 1];

    label[k].text = (unichar_t *)(pe->package_name == NULL ? "[Unknown]" : pe->package_name);
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[1][1] = &gcd[k - 1];
    hvgrid[1][2] = GCD_ColSpan;
    hvgrid[1][3] = NULL;

    label[k].text = (unichar_t *) _("Module Name:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[2][0] = &gcd[k - 1];

    label[k].text = (unichar_t *)(pe->module_name == NULL ? "[Unknown]" : pe->module_name);
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[2][1] = &gcd[k - 1];
    hvgrid[2][2] = GCD_ColSpan;
    hvgrid[2][3] = NULL;

    label[k].text = (unichar_t *) _("'attrs' (if any):");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[3][0] = &gcd[k - 1];

    label[k].text = (unichar_t *)(pe->attrs == NULL ? "" : pe->attrs);
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[3][1] = &gcd[k - 1];
    hvgrid[3][2] = GCD_ColSpan;
    hvgrid[3][3] = NULL;

    label[k].text = (unichar_t *) _("Package URL:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[4][0] = &gcd[k - 1];

    label[k].text = (unichar_t *)(pe->package_url == NULL ? "[Unknown]" : pe->package_url);
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[4][1] = &gcd[k - 1];
    hvgrid[4][2] = GCD_ColSpan;
    hvgrid[4][3] = NULL;

    label[k].text = (unichar_t *) _("Summary:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[5][0] = &gcd[k - 1];

    label[k].text = (unichar_t *)(pe->summary == NULL ? "[Unknown]" : pe->summary);
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    hvgrid[5][1] = &gcd[k - 1];
    hvgrid[5][2] = GCD_ColSpan;
    hvgrid[5][3] = NULL;

    hvgrid[6][0] = NULL;

    boxes[2].gd.flags = gg_enabled | gg_visible;
    boxes[2].gd.u.boxelements = hvgrid[0];
    boxes[2].creator = GHVBoxCreate;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_OK;
    gcd[k].gd.handle_controlevent = PLUG_Info_OK;
    gcd[k++].creator = GButtonCreate;
    okgrid[0] = GCD_Glue;
    okgrid[1] = GCD_Glue;
    okgrid[2] = &gcd[k - 1];
    okgrid[3] = GCD_Glue;
    okgrid[4] = GCD_Glue;
    okgrid[5] = GCD_Glue;
    okgrid[6] = NULL;

    boxes[3].gd.flags = gg_enabled | gg_visible;
    boxes[3].gd.u.boxelements = okgrid;
    boxes[3].creator = GHBoxCreate;

    mgrid[0] = &boxes[2];
    mgrid[1] = &boxes[3];
    mgrid[2] = NULL;

    boxes[0].gd.flags = gg_enabled | gg_visible;
    boxes[0].gd.u.boxelements = mgrid;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw, boxes);
    GHVBoxSetPadding(boxes[2].ret, 5, 6);
    GHVBoxFitWindow(boxes[0].ret);

    GDrawSetVisible(gw, true);

    while (!done) {
        GDrawProcessOneEvent(NULL);
    }

    GDrawDestroyWindow(gw);
}

struct plg_data {
    GWindow gw;
    int done;
};

static int PLUG_OK(GGadget *g, GEvent *e) {
    int len, i;
    FFList *l;
    struct plg_data *d = (struct plg_data *) GDrawGetUserData(GGadgetGetWindow(g));

    if (e->type != et_controlevent || e->u.control.subtype != et_buttonactivate) {
        return true;
    }

    for (l = plugin_data; l != NULL; l = l->next) {
        PluginEntry *pe = (PluginEntry *) l->data;
        if (!pe->is_present && pe->new_mode == sm_ask) {
            FreePluginEntry(pe);
        }
    }

    ff_list_free(ff_steal_pointer(&plugin_data)); // Won't free ->data elements
    GGadget *list = GWidgetGetControl(d->gw, CID_PluginList);
    GTextInfo **ti = GGadgetGetList(list, &len);
    for (i = 0; i < len; ++i) {
        PluginEntry *pe = (PluginEntry *) ti[i]->userdata;
        pe->startup_mode = pe->new_mode;
        plugin_data = ff_list_append(plugin_data, pe);
    }
    SavePluginConfig();

    enum plugin_startup_mode_type sm = sm_ask;
    if (GGadgetIsChecked(GWidgetGetControl(d->gw, CID_SmOn))) {
        sm = sm_on;
    } else if (GGadgetIsChecked(GWidgetGetControl(d->gw, CID_SmOff))) {
        sm = sm_off;
    }
    if (sm != plugin_startup_mode) {
        plugin_startup_mode = sm;
        SavePrefs(true);
    }

    d->done = true;
    return true;
}

static int PLUG_Cancel(GGadget *g, GEvent *e) {
    struct plg_data *d = (struct plg_data *) GDrawGetUserData(GGadgetGetWindow(g));
    if (e->type == et_controlevent && e->u.control.subtype == et_buttonactivate) {
        d->done = true;
    }
    return true;
}

static int plug_e_h(GWindow gw, GEvent *e) {
    struct plg_data *d = (struct plg_data *) GDrawGetUserData(gw);
    if (e->type == et_close) {
        d->done = true;
    } else if (e->type == et_char) {
        if ( e->u.chr.keysym == GK_F1 || e->u.chr.keysym == GK_Help ) {
            help("techref/plugin.html", NULL);
	    return( true );
	}
    }
    return true;
}

static unichar_t *pluginDescString(PluginEntry *pe, int *has_err) {
    char *str, *csm, *nsm = NULL, *info = NULL;

    csm = _(PluginStartupModeString(pe->startup_mode, false));
    if (pe->startup_mode != pe->new_mode) {
        nsm = _(PluginStartupModeString(pe->new_mode, false));
    }
    info = _(PluginInfoString(pe, true, has_err));

    if (info != NULL && nsm != NULL) {
        str = smprintf("%s (%s -> %s) [%s]", pe->name, csm, nsm, info);
    } else if (info != NULL && nsm == NULL) {
        str = smprintf("%s (%s) [%s]", pe->name, csm, info);
    } else if (info == NULL && nsm != NULL) {
        str = smprintf("%s (%s -> %s)", pe->name, csm, nsm);
    } else {
        str = smprintf("%s (%s)", pe->name, csm);
    }
    unichar_t *r = utf82u_copy(str);
    free(str);
    return r;
}

static void FigurePluginList(struct plg_data *d) {
    GGadget *list = GWidgetGetControl(d->gw, CID_PluginList);
    FFList *p;
    int l = ff_list_length(plugin_data), i, has_err;

    GGadgetClearList(list);

    if (l == 0) {
        return;
    }

    GTextInfo **ti = malloc((l + 1) * sizeof(GTextInfo *));
    for (i = 0, p = plugin_data; p != NULL; ++i, p = p->next) {
        PluginEntry *pe = (PluginEntry *) p->data;
        pe->new_mode = pe->startup_mode;
        ti[i] = calloc(1, sizeof(GTextInfo));
        ti[i]->text = pluginDescString(pe, &has_err);
        ti[i]->fg = ti[i]->bg = COLOR_DEFAULT;
        if (has_err) {
            ti[i]->fg = GDrawGetWarningForeground(GDrawGetDisplayOfWindow(d->gw));
        }
        ti[i]->userdata = pe;
    }
    ti[i] = calloc(1, sizeof(GTextInfo));
    GGadgetSetList(list, ti, false);
}

static void PLUG_EnableButtons(struct plg_data *d) {
    GGadget *list = GWidgetGetControl(d->gw, CID_PluginList);
    int pos = GGadgetGetFirstListSelectedItem(list), len;
    GTextInfo **ti = GGadgetGetList(list, &len);
    PluginEntry *pe = pos == -1 ? NULL : (PluginEntry *) ti[pos]->userdata;

    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_Top), pe && pos > 0);
    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_Up), pe && pos > 0);
    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_Down), pe && pos < len - 1);
    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_Bottom), pe && pos < len - 1);
    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_Enable), pe && pe->new_mode != sm_on);
    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_Disable), pe && pe->new_mode != sm_off);
    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_Delete), pe && !pe->is_present);
    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_MoreInfo), pe != NULL);
    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_Load), pe && pe->entrypoint != NULL);
    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_Web), pe && pe->package_url != NULL);
    GGadgetSetEnabled(GWidgetGetControl(d->gw, CID_Conf), pe && pe->has_prefs);
}

static int PLUG_PluginListChange(GGadget *g, GEvent *e) {
    struct plg_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    PLUG_EnableButtons(d);
    return true;
}

void GListMoveOneSelected(GGadget *list, int offset) {
    int32_t len;
    int i, pos = -1;
    GTextInfo **old, **new, *tmp;

    old = GGadgetGetList(list, &len);
    new = calloc(len + 1, sizeof(GTextInfo *));
    for (i = 0; i < len; i++) {
        if (old[i]->selected) {
            pos = i;
        }
        new[i] = GTextInfoCopy(old[i]);
    }
    new[len] = calloc(1, sizeof(GTextInfo));
    if (pos == -1 || pos == offset) {
        GGadgetSetList(list, new, false);
        return;
    }
    if (offset > pos) {
        tmp = new[pos];
        for (i = pos; i < offset; ++i) {
            new[i] = new[i + 1];
        }
        new[offset] = tmp;
    } else {
        tmp = new[pos];
        for (i = pos; i > offset; --i) {
            new[i] = new[i - 1];
        }
        new[offset] = tmp;
    }
    GGadgetSetList(list, new, false);
}

static int PLUG_PluginListOrder(GGadget *g, GEvent *e) {
    if ( e->type!=et_controlevent || e->u.control.subtype!=et_buttonactivate)
        return true;

    struct plg_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    GGadget *list = GWidgetGetControl(d->gw, CID_PluginList);
    int cid = GGadgetGetCid(g);
    int pos = GGadgetGetFirstListSelectedItem(list), newpos, len;
    GGadgetGetList(list, &len);
    (void)len; //only used in asserts

    if (pos == -1) {
        return true;    // Shouldn't happen
    }
    assert(pos >= 0 && pos < len);

    if (cid == CID_Top) {
        assert(pos > 0);
        GListMoveOneSelected(list, newpos = 0);
    } else if (cid == CID_Up) {
        assert(pos > 0);
        GListMoveOneSelected(list, newpos = pos - 1);
    } else if (cid == CID_Down) {
        assert(pos < len - 1);
        GListMoveOneSelected(list, newpos = pos + 1);
    } else if (cid == CID_Bottom) {
        assert(pos < len - 1);
        GListMoveOneSelected(list, newpos = len - 1);
    }
    GGadgetSelectOneListItem(list, newpos);
    PLUG_EnableButtons(d);
    return true;
}

static void ResetPluginText(GTextInfo *i) {
    free(i->text);
    i->text = pluginDescString((PluginEntry *) i->userdata, NULL);
}

static int PLUG_PluginOp(GGadget *g, GEvent *e) {
    if (e->type != et_controlevent || e->u.control.subtype != et_buttonactivate) {
        return true;
    }

    struct plg_data *d = GDrawGetUserData(GGadgetGetWindow(g));
    GGadget *list = GWidgetGetControl(d->gw, CID_PluginList);
    GTextInfo *i =  GGadgetGetListItemSelected(list);
    if (i == NULL) {
        return true;
    }
    PluginEntry *pe = (PluginEntry *) i->userdata;
    int cid = GGadgetGetCid(g);

    if (cid == CID_Enable) {
	static char *buts[4];
	buts[0] = _("_Yes");
	buts[1] = _("_No");
	buts[2] = _("_Cancel");
	buts[3] = NULL;
	int ans = ff_ask(_("Load Plugin?"), (const char **) buts, 0, 2, _("The plugin will be loaded in the order at the next restart\nof FontForge. You can also load it now. Would you like to?"));
	if ( ans==0 || ans==1)
            pe->new_mode = sm_on;
	if ( ans==0 )
            LoadPlugin(pe);
        ResetPluginText(i);
        PLUG_EnableButtons(d);
    } else if (cid == CID_Disable) {
        pe->new_mode = sm_off;
        ResetPluginText(i);
        PLUG_EnableButtons(d);
    } else if (cid == CID_Delete) {
        pe->new_mode = sm_ask;
        if (!pe->is_present)
            GListDelSelected(list);
        PLUG_EnableButtons(d);
    } else if (cid == CID_MoreInfo) {
        PluginInfoDlg(pe);
    } else if (cid == CID_Load) {
        if (pe->entrypoint != NULL) {
            LoadPlugin(pe);
            ResetPluginText(i);
            PLUG_EnableButtons(d);
        }
    } else if (cid == CID_Web) {
        if (pe->package_url != NULL) {
            help(pe->package_url, NULL);
        }
    } else if (cid == CID_Conf) {
        PluginDoPreferences(pe);
    } else if (cid == CID_Revert) {
        FigurePluginList(d);
    }
    GGadgetRedraw(list);
    return true;
}

void _PluginDlg(void) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[24], boxes[6], *vert[5], *horiz[3], *tradio[6], *sbuttons[17],
                      *bbuttons[9];
    GTextInfo label[24];
    int k, msg;
    struct plg_data d = { NULL, false };

    if (no_windowing_ui) {
        return;
    }

    if (!attempted_plugin_load) {
        static char *buts[3];
        if (!use_plugins) {
            buts[0] = _("_Ok");
	    buts[1] = NULL;
            ff_ask(_("Plugins off in preferences"), (const char **) buts, 0, 0, _("The UsePlugins preferences option is currently off.\nFontForge will not load plugin configuration or\nattempt discovery unless that option is on."));
        } else {
            buts[0] = _("_Yes");
            buts[1] = _("_No");
            buts[2] = NULL;
            int ans = ff_ask(_("Initialize Plugins?"), (const char **) buts, 0, 1, _("FontForge has not yet attempted to load Plugins\nDo you want it to do that now?"));
            if (ans == 0) {
                PyFF_ProcessInitFiles(false, true);
            }
        }
    }

    memset(&wattrs, 0, sizeof(wattrs));
    wattrs.mask = wam_events | wam_cursor | wam_utf8_wtitle | wam_undercursor | wam_isdlg | wam_restrict;
    wattrs.event_masks = ~(1 << et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = _("Plugin Configuration");
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = pos.height = 0;
    d.gw = gw = GDrawCreateTopWindow(NULL, &pos, plug_e_h, &d, &wattrs);

    memset(&label, 0, sizeof(label));
    memset(&gcd, 0, sizeof(gcd));
    memset(&vert, 0, sizeof(vert));
    memset(&tradio, 0, sizeof(tradio));
    memset(&horiz, 0, sizeof(horiz));
    memset(&sbuttons, 0, sizeof(sbuttons));
    memset(&bbuttons, 0, sizeof(bbuttons));
    memset(&boxes, 0, sizeof(boxes));

    k = 0;
    label[k].text = (unichar_t *) _("PluginStartupMode:");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10;
    gcd[k].gd.pos.y = 6;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;
    tradio[0] = &gcd[k - 1];
    tradio[1] = GCD_Glue;

    label[k].text = (unichar_t *) _("O_n");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled | (plugin_startup_mode == sm_on ? gg_cb_on : 0);
    gcd[k].gd.u.radiogroup = 1;
    gcd[k].gd.popup_msg = _("When a new plugin is discovered it is recorded and activated");
    gcd[k].gd.cid = CID_SmOn;
    gcd[k++].creator = GRadioCreate;
    tradio[2] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("O_ff");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled | (plugin_startup_mode == sm_off ? gg_cb_on : 0);
    gcd[k].gd.u.radiogroup = 1;
    gcd[k].gd.popup_msg = _("When a new plugin is discovered it is recorded but not activated");
    gcd[k].gd.cid = CID_SmOff;
    gcd[k++].creator = GRadioCreate;
    tradio[3] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("_Ask");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled | (plugin_startup_mode == sm_ask ? gg_cb_on : 0);
    gcd[k].gd.u.radiogroup = 1;
    gcd[k].gd.popup_msg = _("When a new plugin is discovered it is left unrecorded\nuntil configured in this dialog.");
    gcd[k].gd.cid = CID_SmAsk;
    gcd[k++].creator = GRadioCreate;
    tradio[4] = &gcd[k - 1];
    tradio[5] = NULL;

    boxes[2].gd.flags = gg_enabled | gg_visible;
    boxes[2].gd.u.boxelements = tradio;
    boxes[2].creator = GHBoxCreate;

    msg = k;
    label[k].text = (unichar_t *) _("Plugins can be Loaded and Configured now.\nOther changes will take effect at next restart.");
    label[k].text_is_1byte = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.pos.x = 10;
    gcd[k].gd.pos.y = 6;
    gcd[k].gd.flags = gg_enabled | gg_visible;
    gcd[k++].creator = GLabelCreate;

    label[k].text = (unichar_t *) _("_Top");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Top;
    gcd[k].gd.handle_controlevent = PLUG_PluginListOrder;
    gcd[k++].creator = GButtonCreate;
    sbuttons[0] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("_Up");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Up;
    gcd[k].gd.handle_controlevent = PLUG_PluginListOrder;
    gcd[k++].creator = GButtonCreate;
    sbuttons[1] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("_Down");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Down;
    gcd[k].gd.handle_controlevent = PLUG_PluginListOrder;
    gcd[k++].creator = GButtonCreate;
    sbuttons[2] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("_Bottom");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Bottom;
    gcd[k].gd.handle_controlevent = PLUG_PluginListOrder;
    gcd[k++].creator = GButtonCreate;
    sbuttons[3] = &gcd[k - 1];

    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLineCreate;
    sbuttons[4] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("_Enable");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Enable;
    gcd[k].gd.handle_controlevent = PLUG_PluginOp;
    gcd[k++].creator = GButtonCreate;
    sbuttons[5] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("_Disable");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Disable;
    gcd[k].gd.handle_controlevent = PLUG_PluginOp;
    gcd[k++].creator = GButtonCreate;
    sbuttons[6] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("_Delete");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Delete;
    gcd[k].gd.handle_controlevent = PLUG_PluginOp;
    gcd[k++].creator = GButtonCreate;
    sbuttons[7] = &gcd[k - 1];

    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLineCreate;
    sbuttons[8] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("_More Info");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_MoreInfo;
    gcd[k].gd.handle_controlevent = PLUG_PluginOp;
    gcd[k++].creator = GButtonCreate;
    sbuttons[9] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("_Load");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Load;
    gcd[k].gd.handle_controlevent = PLUG_PluginOp;
    gcd[k++].creator = GButtonCreate;
    sbuttons[10] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("Open _Web Page");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Web;
    gcd[k].gd.handle_controlevent = PLUG_PluginOp;
    gcd[k++].creator = GButtonCreate;
    sbuttons[11] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("_Configure");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Conf;
    gcd[k].gd.handle_controlevent = PLUG_PluginOp;
    gcd[k++].creator = GButtonCreate;
    sbuttons[12] = &gcd[k - 1];

    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k++].creator = GLineCreate;
    sbuttons[13] = &gcd[k - 1];

    label[k].text = (unichar_t *) _("Re_vert List");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Revert;
    gcd[k].gd.handle_controlevent = PLUG_PluginOp;
    gcd[k++].creator = GButtonCreate;
    sbuttons[14] = &gcd[k - 1];

    sbuttons[15] = GCD_Glue;
    sbuttons[16] = NULL;

    boxes[3].gd.flags = gg_enabled | gg_visible;
    boxes[3].gd.u.boxelements = sbuttons;
    boxes[3].creator = GVBoxCreate;

    gcd[k].gd.flags = gg_visible | gg_enabled | gg_list_exactlyone;
    gcd[k].gd.handle_controlevent = PLUG_PluginListChange;
    gcd[k].gd.cid = CID_PluginList;
    gcd[k].gd.pos.width = 200;
    gcd[k++].creator = GListCreate;

    horiz[0] = &gcd[k - 1];
    horiz[1] = &boxes[3];
    horiz[2] = NULL;

    boxes[4].gd.flags = gg_enabled | gg_visible;
    boxes[4].gd.u.boxelements = horiz;
    boxes[4].creator = GHBoxCreate;

    label[k].text = (unichar_t *) _("_OK");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_OK;
    gcd[k].gd.handle_controlevent = PLUG_OK;
    gcd[k++].creator = GButtonCreate;
    bbuttons[0] = GCD_Glue;
    bbuttons[1] = &gcd[k - 1];
    bbuttons[2] = GCD_Glue;
    bbuttons[3] = GCD_Glue;

    label[k].text = (unichar_t *) _("_Cancel");
    label[k].text_is_1byte = true;
    label[k].text_in_resource = true;
    gcd[k].gd.label = &label[k];
    gcd[k].gd.flags = gg_visible | gg_enabled;
    gcd[k].gd.cid = CID_Cancel;
    gcd[k].gd.handle_controlevent = PLUG_Cancel;
    gcd[k++].creator = GButtonCreate;
    bbuttons[4] = GCD_Glue;
    bbuttons[5] = &gcd[k - 1];
    bbuttons[6] = GCD_Glue;
    bbuttons[7] = GCD_Glue;
    bbuttons[8] = NULL;

    boxes[5].gd.flags = gg_enabled | gg_visible;
    boxes[5].gd.u.boxelements = bbuttons;
    boxes[5].creator = GHBoxCreate;

    vert[0] = &boxes[2];
    vert[1] = &gcd[msg];
    vert[2] = &boxes[4];
    vert[3] = &boxes[5];
    vert[4] = NULL;

    boxes[0].gd.flags = gg_enabled | gg_visible;
    boxes[0].gd.u.boxelements = vert;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw, boxes);
    GHVBoxSetExpandableRow(boxes[0].ret, 2);
    GHVBoxSetExpandableRow(boxes[3].ret, 15);
    GHVBoxSetExpandableCol(boxes[4].ret, 0);
    GHVBoxSetPadding(boxes[0].ret, 0, 10);
    GHVBoxSetPadding(boxes[4].ret, 10, 0);
    GHVBoxFitWindow(boxes[0].ret);

    FigurePluginList(&d);
    PLUG_EnableButtons(&d);

    GDrawSetVisible(gw, true);

    while (!d.done) {
        GDrawProcessOneEvent(NULL);
    }

    GDrawDestroyWindow(gw);
}

void MenuPlug(GWindow UNUSED(base), struct gmenuitem *UNUSED(mi), GEvent *UNUSED(e)) {
    PluginDlg();
}

#endif // _NO_PYTHON
