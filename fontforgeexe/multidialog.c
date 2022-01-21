/* -*- coding: utf-8 -*- */
/* Copyright (C) 2000-2012 by George Williams, 2021 by Skef Iterum */
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

#include "basics.h"

#include "ffglib.h"
#include "fontforgeui.h"
#include "multidialog.h"

#include <assert.h>

#define CID_BF_PAIR_START 0x10
#define CID_STR_START 0x1000
#define CID_LIST_START 0x4000

// Instead of a "done" boolean the window has a multi_done_state to
// track "how" it is done. It is pointed to by the window's user
// data value. The bottom buttons (OK, Cancel, and Apply)
// have a copy of the corresponding state in their user data and a
// generic "Multi_Button" handler to copy it into the window's
// user data.
enum multi_done_state { mds_not = 0, mds_ok = 1, mds_cancel = 2, mds_apply = 3 };

static int Multi_Button(GGadget *g, GEvent *e) {
    if (e->type == et_controlevent && e->u.control.subtype == et_buttonactivate) {
        enum multi_done_state *mdsp = GDrawGetUserData(GGadgetGetWindow(g));
        enum multi_done_state mds = (enum multi_done_state) GGadgetGetUserData(g);
        *mdsp = mds;
    }
    return (true);
}

static int md_e_h(GWindow gw, GEvent *event) {
    if (event->type == et_close) {
        enum multi_done_state *mdsp = GDrawGetUserData(gw);
        *mdsp = mds_cancel;
	return true;
    }
    return false;
}

// Opens a file chooser dialog when the handler-carrying button is pressed
static int Multi_BrowseFile(GGadget *g, GEvent *e) {
    if (e->type == et_controlevent && e->u.control.subtype == et_buttonactivate) {
        GWindow gw = GGadgetGetWindow(g);
        GGadget *tf = GWidgetGetControl(gw, GGadgetGetCid(g) - 1);
        char *ret, *cur = GGadgetGetTitle8(tf);
        MultiDlgQuestion *qstn = GGadgetGetUserData(tf);

        if (qstn->type == mdq_openpath)
            ret = gwwv_open_filename(qstn->label,
                                     *cur == '\0' ? NULL : cur,
                                     qstn->filter, NULL);
        else
            ret = gwwv_save_filename(qstn->label,
                                     *cur == '\0' ? NULL : cur,
                                     qstn->filter);
        free(cur);
        if (ret == NULL) {
            return true;
        }
        GGadgetSetTitle8(tf, ret);
        free(ret);
    }
    return true;
}

// On a radio or checkbox change event copies the new state into the
// MultiDlgAnswer is_checked value. (Each radio button and check box
// has its MultiDlgAnswer pointer as its user data.)
//
// MultiDlgAnswer->question points back to its "parent" so that
// corresponding radio button _is_checked values can be set to false.
static int Multi_DoRC(GGadget *g, GEvent *e) {
    if (e->type == et_controlevent && e->u.control.subtype == et_radiochanged) {
        MultiDlgAnswer *ans = GGadgetGetUserData(g);
        if (ans == NULL) {
            return false;
        }
        assert(ans->question->type == mdq_choice && ans->question->checks);
        ans->is_checked = GGadgetIsChecked(g);
        if (ans->is_checked && !ans->question->multiple)
            for (int i = 0; i < ans->question->answer_len; ++i) {
                if (ans != ans->question->answers + i) {
                    ans->question->answers[i].is_checked = false;
                }
            }
    }
    return true;
}

// Translates a changed list value into appropriate answer is_checked settings.
// Each list has a pointer to is MultiDlgQuestion as its user data
static int Multi_DoL(GGadget *g, GEvent *e) {
    if (e->type == et_controlevent && e->u.control.subtype == et_listselected) {
        MultiDlgQuestion *qstn = GGadgetGetUserData(g);
        if (qstn == NULL) {
            return false;
        }
        assert(qstn->type == mdq_choice && !qstn->checks);
        for (int i = 0; i < qstn->answer_len; ++i) {
            qstn->answers[i].is_checked = GGadgetIsListItemSelected(g, i);
        }
    }
    return true;
}

// A handler for the "check all" and "check none" buttons next to a
// multi-list with multiple selection options.
static int Multi_MLSelect(GGadget *g, GEvent *e, int sel) {
    if (e->type == et_controlevent && e->u.control.subtype == et_buttonactivate) {
        GWindow gw = GGadgetGetWindow(g);
        size_t cid = (size_t) GGadgetGetUserData(g);
        GGadget *l = GWidgetGetControl(gw, cid);
        GGadgetSelectListItem(l, -1, sel);
        e->u.control.subtype = et_listselected;
        Multi_DoL(l, e);
        e->u.control.subtype = et_buttonactivate;
    }
    return true;
}

static int Multi_MLSelectAll(GGadget *g, GEvent *e) {
    return Multi_MLSelect(g, e, true);
}

static int Multi_MLSelectNone(GGadget *g, GEvent *e) {
    return Multi_MLSelect(g, e, false);
}

struct multi_expand {
    GGadgetCreateData *gcd;
    int is_row;
    int loc;
};

struct multi_postproc {
    GList_Glib *memlist;
    GList_Glib *expands;
    int strcid;
    int listcid;
    int fbpair;
};

static GGadgetCreateData *LayoutMultiDlgQuestion(MultiDlgQuestion *qstn, struct multi_postproc *mpp) {
    int gcnt = 1, lcnt = 0, flcnt = 1, g = 0, l = 0, fl = 0, i;
    GGadgetCreateData *gcd, **flarray, *qgcd = NULL;
    GTextInfo *label, *glistarray;
    unichar_t qmne;

    if (qstn->label != NULL) {
        lcnt++;
        gcnt++;
    }
    if (qstn->type == mdq_string) {
        lcnt++;
        gcnt++;
        flcnt++;
    } else if (qstn->type == mdq_choice) {
        if (qstn->checks) {
            gcnt++;
            lcnt += qstn->answer_len;
            gcnt += qstn->answer_len + 1;
            flcnt += qstn->answer_len;
        } else {
            gcnt++;
            flcnt++;
            if (qstn->multiple) {
                lcnt += 2;
                gcnt += 3;
            }
        }
    } else if (qstn->type == mdq_openpath || qstn->type == mdq_savepath) {
        lcnt += 2;
        gcnt += 3;
        flcnt += 1;
    } else {
        ff_post_error(_("Unrecognized MultiAsk Question"), _("Cannot raise MultiAsk Dialog: Unrecognized type."));
        return NULL;
    }
    gcd = calloc(gcnt, sizeof(GGadgetCreateData));
    mpp->memlist = g_list_append(mpp->memlist, gcd);
    label = calloc(lcnt, sizeof(GTextInfo));
    mpp->memlist = g_list_append(mpp->memlist, label);
    flarray = calloc(flcnt, sizeof(GGadgetCreateData *));
    mpp->memlist = g_list_append(mpp->memlist, flarray);

    if (qstn->label != NULL) {
        label[l].text = (unichar_t *) utf82u_mncopy(qstn->label, &qmne);
        mpp->memlist = g_list_append(mpp->memlist, label[l].text);
        gcd[g].gd.label = &label[l++];
        gcd[g].gd.mnemonic = qmne;
        gcd[g].gd.flags = gg_enabled | gg_visible;
        gcd[g].creator = GLabelCreate;
        qgcd = &gcd[g++];
    }

    if (qstn->type == mdq_string) {
        label[l].text = (unichar_t *) qstn->dflt;
        label[l].text_is_1byte = true;
        gcd[g].gd.label = &label[l++];
        gcd[g].gd.mnemonic = qmne;
        gcd[g].gd.flags = gg_enabled | gg_visible;
        gcd[g].gd.cid = mpp->strcid + CID_STR_START;
        mpp->strcid++;
        gcd[g].data = qstn;
        gcd[g].creator = GTextFieldCreate;
        flarray[fl++] = &gcd[g++];
    } else if (qstn->type == mdq_choice) {
        if (qstn->checks) {
            for (i = 0; i < qstn->answer_len; ++i) {
                MultiDlgAnswer *ans = &qstn->answers[i];
                label[l].text = (unichar_t *) ans->name;
                label[l].text_in_resource = true;
                label[l].text_is_1byte = true;
                gcd[g].gd.label = &label[l++];
                gcd[g].gd.flags = gg_enabled | gg_visible;
                if (ans->is_default) {
                    gcd[g].gd.flags |= gg_cb_on;
                }
                gcd[g].data = ans;
                if (qstn->multiple) {
                    gcd[g].creator = GCheckBoxCreate;
                } else {
                    gcd[g].creator = GRadioCreate;
                    if (i != 0) {
                        gcd[g].gd.flags |= gg_rad_continueold;
                    }
                }
                gcd[g].gd.handle_controlevent = Multi_DoRC;
                flarray[fl++] = &gcd[g++];
            }
        } else {
            glistarray = calloc(qstn->answer_len + 1, sizeof(GTextInfo));
            mpp->memlist = g_list_append(mpp->memlist, glistarray);
            for (i = 0; i < qstn->answer_len; ++i) {
                MultiDlgAnswer *ans = &qstn->answers[i];
                glistarray[i].text = (unichar_t *) ans->name;
                glistarray[i].text_is_1byte = true;
                glistarray[i].selected = ans->is_default;
                glistarray[i].userdata = ans;
            }
            gcd[g].gd.u.list = glistarray;
            gcd[g].gd.flags = gg_visible | gg_enabled;
            gcd[g].gd.mnemonic = qmne;
            gcd[g].gd.cid = mpp->listcid + CID_LIST_START;
            gcd[g].creator = GListCreate;
            gcd[g].data = qstn;
            gcd[g].gd.handle_controlevent = Multi_DoL;
            if (qstn->multiple) {
                gcd[g].gd.flags |= gg_list_multiplesel;
                GGadgetCreateData *(*lbox)[3] = calloc(1, sizeof(GGadgetCreateData * [3][3]));
                mpp->memlist = g_list_append(mpp->memlist, lbox);
                lbox[0][0] = &gcd[g++];
                lbox[1][0] = GCD_RowSpan;

                label[l].text = (unichar_t *) _("All");
                label[l].text_is_1byte = true;
                gcd[g].gd.label = &label[l++];
                gcd[g].gd.flags = gg_enabled | gg_visible;
                gcd[g].creator = GButtonCreate;
                gcd[g].gd.handle_controlevent = Multi_MLSelectAll;
                gcd[g].data = (void *)(size_t) mpp->listcid + CID_LIST_START;
                lbox[0][1] = &gcd[g++];

                label[l].text = (unichar_t *) _("None");
                label[l].text_is_1byte = true;
                gcd[g].gd.label = &label[l++];
                gcd[g].gd.flags = gg_enabled | gg_visible;
                gcd[g].creator = GButtonCreate;
                gcd[g].gd.handle_controlevent = Multi_MLSelectNone;
                gcd[g].data = (void *)(size_t) mpp->listcid + CID_LIST_START;
                lbox[1][1] = &gcd[g++];

                gcd[g].gd.flags = gg_enabled | gg_visible;
                gcd[g].gd.u.boxelements = lbox[0];
                gcd[g].creator = GHVBoxCreate;
                struct multi_expand *me = calloc(1, sizeof(struct multi_expand));
                mpp->expands = g_list_append(mpp->expands, me);
                me->gcd = &gcd[g];
                me->is_row = false;
                me->loc = 0;
            } else {
                gcd[g].gd.flags |= gg_list_exactlyone;
            }
            mpp->listcid++;
            flarray[fl++] = &gcd[g++];
        }
    } else if (qstn->type == mdq_openpath || qstn->type == mdq_savepath) {
        int fb = 0;
        GGadgetCreateData **fbox = calloc(3, sizeof(GGadgetCreateData *));
        mpp->memlist = g_list_append(mpp->memlist, fbox);
        label[l].text = (unichar_t *) qstn->dflt;
        label[l].text_is_1byte = true;
        gcd[g].gd.label = &label[l++];
        gcd[g].gd.mnemonic = qmne;
        gcd[g].gd.flags = gg_enabled | gg_visible;
        gcd[g].gd.cid = mpp->fbpair + CID_BF_PAIR_START;
        gcd[g].data = qstn;
        gcd[g].creator = GTextFieldCreate;
        fbox[fb++] = &gcd[g++];

        label[l].text = (unichar_t *) _("...");
        label[l++].text_is_1byte = true;
        gcd[g].gd.label = &label[l - 1];
        gcd[g].gd.flags = gg_enabled | gg_visible;
        gcd[g].gd.cid = mpp->fbpair + 1 + CID_BF_PAIR_START;
        gcd[g].gd.handle_controlevent = Multi_BrowseFile;
        gcd[g].data = qstn->filter;
        mpp->fbpair += 2;
        gcd[g].creator = GButtonCreate;
        fbox[fb++] = &gcd[g++];

        gcd[g].gd.flags = gg_enabled | gg_visible;
        gcd[g].gd.u.boxelements = fbox;
        gcd[g].creator = GHBoxCreate;
        struct multi_expand *me = calloc(1, sizeof(struct multi_expand));
        mpp->expands = g_list_append(mpp->expands, me);
        me->gcd = &gcd[g];
        me->is_row = false;
        me->loc = 0;
        flarray[fl++] = &gcd[g++];
    }
    gcd[g].gd.flags = gg_enabled | gg_visible | gg_flow_ocenter | gg_flow_lvcenter;
    if (!(qstn->type == mdq_choice && qstn->checks)) {
        gcd[g].gd.flags |= gg_flow_expand;
    }
    if (!qstn->align) {
        gcd[g].gd.flags |= gg_flow_noalignlabel;
    }
    gcd[g].gd.u.boxelements = flarray;
    gcd[g].creator = GFlowBoxCreate;
    gcd[g].gd.label = (GTextInfo *) qgcd;

    return &gcd[g];
}

static GGadgetCreateData *LayoutMultiDlgCategoryBody(MultiDlgCategory *cat, struct multi_postproc *mpp) {
    int i;
    GGadgetCreateData *gcd, **s1barray;

    s1barray = calloc(cat->len + 1, sizeof(GGadgetCreateData *));
    mpp->memlist = g_list_append(mpp->memlist, s1barray);
    for (i = 0; i < cat->len; ++i) {
        s1barray[i] = LayoutMultiDlgQuestion(cat->questions + i, mpp);
        if (s1barray[i] == NULL) {
            return false;
        }
    }

    // Allocate 2 elements in case this is created at the top of a tabset,
    // which calls GGadgetsCreate()
    gcd = calloc(2, sizeof(GGadgetCreateData));
    mpp->memlist = g_list_append(mpp->memlist, gcd);
    gcd[0].gd.flags = gg_enabled | gg_visible | gg_s1_vert | gg_s1_flowalign | gg_s1_expand;
    gcd[0].gd.u.boxelements = s1barray;
    gcd[0].creator = GScroll1BoxCreate;

    return gcd;
}

static void MultiSetDefaultChoices(MultiDlgSpec *spec) {
    for (int c = 0; c < spec->len; ++c) {
        MultiDlgCategory *catspec = spec->categories + c;
        for (int q = 0; q < catspec->len; ++q) {
            MultiDlgQuestion *qstn = catspec->questions + q;
            if (qstn->type == mdq_choice) {
                for (int a = 0; a < qstn->answer_len; ++a) {
                    qstn->answers[a].is_checked = qstn->answers[a].is_default;
                }
            }
        }
    }
}

static void MultiSetStringsFromControls(GWindow gw, int strcid, int fbpair) {
    int i;
    GGadget *g;
    MultiDlgQuestion *qstn;

    for (i = 0; i < strcid; ++i) {
        g = GWidgetGetControl(gw, i + CID_STR_START);
        qstn = (MultiDlgQuestion *) GGadgetGetUserData(g);
        free(qstn->str_answer);
        qstn->str_answer = GGadgetGetTitle8(g);
        if (*qstn->str_answer == '\0') {
            free(qstn->str_answer);
            qstn->str_answer = NULL;
        }
    }
    for (i = 0; i < fbpair; i += 2) {
        g = GWidgetGetControl(gw, i + CID_BF_PAIR_START);
        qstn = (MultiDlgQuestion *) GGadgetGetUserData(g);
        free(qstn->str_answer);
        qstn->str_answer = GGadgetGetTitle8(g);
        if (*qstn->str_answer == '\0') {
            free(qstn->str_answer);
            qstn->str_answer = NULL;
        }
    }
}

int UI_Ask_Multi(const char *title, MultiDlgSpec *dlg) {
    GRect pos, wsize, scsize, s1bsize, s1bt;
    GWindow gw;
    struct multi_postproc mpp;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[4], boxes[4], *barray[9], *varray[3], *cat1;
    GTabInfo *categories;
    GTextInfo label[6];
    enum multi_done_state mds = mds_not;
    int b = 0, g = 0, l = 0, i, err = false;
    int maxwidth = 0, targetwidth;
    int maxheight = 0, targetheight;
    int hvborder;

    if (no_windowing_ui) {
        return false;
    }

    memset(&mpp, 0, sizeof(mpp));
    if (dlg->len > 1) {
        categories = calloc(dlg->len + 1, sizeof(GTabInfo));
        mpp.memlist = g_list_append(mpp.memlist, categories);
        for (i = 0; i < dlg->len; ++i) {
            categories[i].text = (unichar_t *) dlg->categories[i].label;
            categories[i].text_in_resource = true;
            categories[i].text_is_1byte = true;
            categories[i].gcd = LayoutMultiDlgCategoryBody(dlg->categories + i, &mpp);
            if (categories[i].gcd == NULL) {
                err = true;
                break;
            }
        }
        cat1 = categories[0].gcd;
    } else if (dlg->len == 1) {
        cat1 = LayoutMultiDlgCategoryBody(dlg->categories, &mpp);
        err = cat1 == NULL;
    } else {
        err = true;
    }

    if (err) {
        for (GList_Glib *e = mpp.expands; e != NULL; e = e->next) {
            free(e->data);
        }
        g_list_free(mpp.expands);
        for (GList_Glib *m = mpp.memlist; m != NULL; m = m->next) {
            free(m->data);
        }
        g_list_free(mpp.memlist);
        return false;
    }

    memset(&wattrs, 0, sizeof(wattrs));
    wattrs.mask = wam_events | wam_cursor | wam_utf8_wtitle | wam_undercursor | wam_isdlg | wam_restrict;
    wattrs.event_masks = ~(1 << et_charup);
    wattrs.restrict_input_to_me = 1;
    wattrs.undercursor = 1;
    wattrs.cursor = ct_pointer;
    wattrs.utf8_window_title = title;
    wattrs.is_dlg = true;
    pos.x = pos.y = 0;
    pos.width = GDrawPointsToPixels(NULL, 400);
    pos.height = GDrawPointsToPixels(NULL, 300);
    gw = GDrawCreateTopWindow(NULL, &pos, md_e_h, &mds, &wattrs);

    memset(&label, 0, sizeof(label));
    memset(&gcd, 0, sizeof(gcd));
    memset(&boxes, 0, sizeof(boxes));

    barray[b++] = GCD_Glue;
    label[l].text = (unichar_t *) _("_OK");
    label[l].text_is_1byte = true;
    label[l].text_in_resource = true;
    gcd[g].gd.flags = gg_visible | gg_enabled | gg_but_default;
    gcd[g].gd.label = &label[l++];
    gcd[g].creator = GButtonCreate;
    gcd[g].gd.handle_controlevent = Multi_Button;
    gcd[g].data = (void *) mds_ok;
    barray[b++] = &gcd[g++];
    barray[b++] = GCD_Glue;

    if (false) {
        label[l].text = (unichar_t *) _("_Apply");
        label[l].text_is_1byte = true;
        label[l].text_in_resource = true;
        gcd[g].gd.flags = gg_visible;
        gcd[g].gd.label = &label[l++];
        gcd[g].creator = GButtonCreate;
        gcd[g].gd.handle_controlevent = Multi_Button;
        gcd[g].data = (void *) mds_apply;
        barray[b++] = &gcd[g++];
        barray[b++] = GCD_Glue;
    }

    label[l].text = (unichar_t *) _("_Cancel");
    label[l].text_is_1byte = true;
    label[l].text_in_resource = true;
    gcd[g].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
    gcd[g].gd.label = &label[l++];
    gcd[g].creator = GButtonCreate;
    gcd[g].gd.handle_controlevent = Multi_Button;
    gcd[g].data = (void *) mds_cancel;
    barray[b++] = &gcd[g++];
    barray[b++] = GCD_Glue;
    barray[b++] = NULL;

    boxes[2].gd.flags = gg_enabled | gg_visible;
    boxes[2].gd.u.boxelements = barray;
    boxes[2].creator = GHBoxCreate;

    if (dlg->len == 1) {
        varray[0] = cat1;
    } else {
        boxes[3].gd.flags = gg_enabled | gg_visible | gg_tabset_vert | gg_tabset_scroll;
        boxes[3].gd.u.tabs = categories;
        boxes[3].creator = GTabSetCreate;
        varray[0] = &boxes[3];
    }
    varray[1] = &boxes[2];
    varray[2] = NULL;

    boxes[0].gd.flags = gg_enabled | gg_visible;
    boxes[0].gd.u.boxelements = varray;
    boxes[0].creator = GVBoxCreate;

    GGadgetsCreate(gw, boxes);

    GHVBoxSetExpandableRow(boxes[0].ret, 0);
    GHVBoxSetExpandableCol(boxes[2].ret, gb_expandglue);
    for (GList_Glib *e = mpp.expands; e != NULL; e = e->next) {
        struct multi_expand *me = (struct multi_expand *) e->data;
        if (me->is_row) {
            GHVBoxSetExpandableRow(me->gcd->ret, me->loc);
        } else {
            GHVBoxSetExpandableCol(me->gcd->ret, me->loc);
        }
        free(me);

    }
    g_list_free(mpp.expands);

    // GHVBoxFitWindow(boxes[0].ret);

    // Find and set more appropriate window dimensions

    GGadgetGetDesiredSize(boxes[0].ret, &wsize, NULL);
    hvborder = GGadgetGetX(boxes[0].ret);
    wsize.width += 2 * hvborder;
    wsize.height += 2 * hvborder;

    GDrawGetSize(GDrawGetRoot(NULL), &scsize);

    // Outer size of the just-fit GScroll1Box
    if (dlg->len > 1) {
        GGadgetGetDesiredSize(boxes[3].ret, NULL, &s1bsize);
    } else {
        GGadgetGetDesiredSize(cat1->ret, &s1bsize, NULL);
    }

    // Appropriate width
    _GScroll1BoxGetDesiredSize(cat1->ret, &s1bt, NULL, true);
    maxwidth = s1bt.width;
    for (i = 1; i < dlg->len; ++i) {
        _GScroll1BoxGetDesiredSize(categories[i].gcd->ret, &s1bt, NULL, true);
        if (s1bt.width > maxwidth) {
            maxwidth = s1bt.width;
        }
    }
    targetwidth = GDrawPointsToPixels(gw, 400);
    if (targetwidth > (int)(.666 * scsize.width)) {
        targetwidth = (int)(.666 * scsize.width);
    }
    // From window width to gadget width
    targetwidth -= wsize.width - s1bsize.width;
    if (targetwidth > maxwidth) {
        targetwidth = maxwidth;
    }
    targetwidth += wsize.width - s1bsize.width;
    // Need this to calculate the "min oppo size" (roughly the height
    // of the scrolling window corresponding to the resize width).
    GGadgetResize(cat1->ret, targetwidth, s1bsize.height);
    for (i = 1; i < dlg->len; ++i) {
        GGadgetResize(categories[i].gcd->ret, targetwidth, s1bsize.height);
    }
    // From gadget width back to window width
    targetwidth += wsize.width - s1bsize.width;

    // Appropriate height
    maxheight = GScroll1BoxMinOppoSize(cat1->ret);
    for (i = 1; i < dlg->len; ++i) {
        s1bt.height = GScroll1BoxMinOppoSize(categories[i].gcd->ret);
        if (s1bt.height > maxheight) {
            maxheight = s1bt.height;
        }
    }
    targetheight = (int)(.9 * scsize.height);
    targetheight -= wsize.height - s1bsize.height;
    if (targetheight > maxheight) {
        targetheight = maxheight;
    }
    targetheight += wsize.height - s1bsize.height;
    GDrawResize(gw, targetwidth, targetheight);
    GDrawMove(gw, (scsize.width - targetwidth) / 2, (scsize.height - targetheight) / 2);

    for (GList_Glib *m = mpp.memlist; m != NULL; m = m->next) {
        free(m->data);
    }
    g_list_free(mpp.memlist);

    MultiSetDefaultChoices(dlg);

    GDrawSetVisible(gw, true);

    while (mds != mds_ok && mds != mds_cancel) {
        GDrawProcessOneEvent(NULL);
    }

    if (mds == mds_cancel) {
        MultiSetDefaultChoices(dlg);
    } else {
        assert(mds == mds_ok);
        MultiSetStringsFromControls(gw, mpp.strcid, mpp.fbpair);
    }

    GDrawDestroyWindow(gw);

    return mds == mds_ok;
}
