/* Copyright (C) 2016 by Jeremy Tan */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

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

/**
*  \file  ggdkdraw.c
*  \brief GDK drawing backend.
*/

#include "ggdkdrawP.h"

#ifdef FONTFORGE_CAN_USE_GDK

#include "gkeysym.h"
#include "gresource.h"
#include "ustring.h"

#include <assert.h>
#include <math.h>

// HACK HACK HACK
#ifdef GDK_WINDOWING_WIN32
#  define GDK_COMPILATION
#  include <gdk/gdkwin32.h>
#  undef GDK_COMPILATION
#endif

// Forward declarations
static void GGDKDrawCancelTimer(GTimer *timer);
static void GGDKDrawDestroyWindow(GWindow w);
static void GGDKDrawPostEvent(GEvent *e);
static void GGDKDrawProcessPendingEvents(GDisplay *gdisp);
static void GGDKDrawSetCursor(GWindow w, GCursor gcursor);
static void GGDKDrawSetTransientFor(GWindow transient, GWindow owner);
static void GGDKDrawSetWindowBackground(GWindow w, Color gcol);

// Private member functions (file-level)
static void _GGDKDraw_InitiateWindowDestroy(GGDKWindow gw);

static GGC *_GGDKDraw_NewGGC(void) {
    GGC *ggc = calloc(1, sizeof(GGC));
    if (ggc == NULL) {
        Log(LOGDEBUG, "GGC: Memory allocation failed!");
        return NULL;
    }

    ggc->clip.width = ggc->clip.height = 0x7fff;
    ggc->fg = 0;
    ggc->bg = 0xffffff;
    return ggc;
}

static void _GGDKDraw_SetOpaqueRegion(GGDKWindow gw) {
    cairo_rectangle_int_t r = {
        gw->pos.x, gw->pos.y, gw->pos.width, gw->pos.height
    };
    cairo_region_t *cr = cairo_region_create_rectangle(&r);
    if (cairo_region_status(cr) == CAIRO_STATUS_SUCCESS) {
        gdk_window_set_opaque_region(gw->w, cr);
        cairo_region_destroy(cr);
    }
}

static void _GGDKDraw_ClearSelData(GGDKDisplay *gdisp, enum selnames sn) {
    GList_Glib *ptr = gdisp->selinfo[sn].datalist;
    while (ptr != NULL) {
        GGDKSelectionData *data = (GGDKSelectionData *)ptr->data;
        if (data->data) {
            if (data->freedata) {
                (data->freedata)(data->data);
            } else {
                free(data->data);
            }
        }
        free(data);
        ptr = g_list_delete_link(ptr, ptr);
    }
    gdisp->selinfo[sn].datalist = NULL;
    gdisp->selinfo[sn].owner = NULL;
}

static bool _GGDKDraw_TransmitSelection(GGDKDisplay *gdisp, GdkEventSelection *e) {
    // Default location to store data if none specified
    if (e->property == GDK_NONE) {
        e->property = e->target;
    }

    // Check that we own the selection request
    enum selnames sn;
    for (sn = 0; sn < sn_max; sn++) {
        if (e->selection == gdisp->selinfo[sn].sel_atom) {
            break;
        }
    }
    if (sn == sn_max) {
        return false;
    }

    GGDKSelectionInfo *sel = &gdisp->selinfo[sn];
    GdkWindow *requestor = (GdkWindow *)g_object_ref(e->requestor);

    if (e->target == gdk_atom_intern_static_string("TARGETS")) {
        guint i = 0, dlen = g_list_length(sel->datalist);
        GdkAtom *targets = calloc(2 + dlen, sizeof(GdkAtom));
        GList_Glib *ptr = sel->datalist;

        targets[i++] = gdk_atom_intern_static_string("TIMESTAMP");
        targets[i++] = gdk_atom_intern_static_string("TARGETS");

        while (ptr != NULL) {
            targets[i++] = ((GGDKSelectionData *)ptr->data)->type_atom;
            ptr = ptr->next;
        }

#ifdef GDK_WINDOWING_WIN32
        gdk_win32_selection_clear_targets(gdk_window_get_display(e->window), e->selection);
        gdk_win32_selection_add_targets(e->window, e->selection, i, targets);
#endif

        gdk_property_change(requestor, e->property, gdk_atom_intern_static_string("ATOM"),
                            32, GDK_PROP_MODE_REPLACE, (const guchar *)targets, i);
    } else if (e->target == gdk_atom_intern_static_string("TIMESTAMP")) {
        gdk_property_change(requestor, e->property, gdk_atom_intern_static_string("INTEGER"),
                            32, GDK_PROP_MODE_REPLACE, (const guchar *)&sel->timestamp, 1);
    } else {
        GList_Glib *ptr = sel->datalist;
        GGDKSelectionData *data = NULL;
        while (ptr != NULL) {
            if (((GGDKSelectionData *)ptr->data)->type_atom == e->target) {
                data = (GGDKSelectionData *)ptr->data;
                break;
            }
            ptr = ptr->next;
        }
        if (data == NULL) { // Unknown selection
            g_object_unref(requestor);
            return false;
        }

        void *tmp = data->data;
        int len = data->cnt;
        if (data->gendata) {
            tmp = data->gendata(data->data, &len);
            if (tmp == NULL) {
                g_object_unref(requestor);
                return false;
            }
        }

        gdk_property_change(requestor, e->property, e->target,
                            data->unit_size * 8, GDK_PROP_MODE_REPLACE, (const guchar *)tmp, len);
        if (data->gendata) {
            free(tmp);
        }
    }

    g_object_unref(requestor);
    return true;
}

static gboolean _GGDKDraw_OnWindowDestroyed(gpointer data) {
    GGDKWindow gw = (GGDKWindow)data;
    Log(LOGDEBUG, "Window: %p", gw);
    if (gw->is_cleaning_up) {
        return false;
    }
    gw->is_cleaning_up = true; // We're in the process of destroying it.

    if (gw->cc != NULL) {
        cairo_destroy(gw->cc);
        gw->cc = NULL;
    }

    if (gw->resize_timeout != 0) {
        g_source_remove(gw->resize_timeout);
        gw->resize_timeout = 0;
    }

    if (!gw->is_pixmap) {
        if (gw != gw->display->groot) {
            if (!gdk_window_is_destroyed(gw->w)) {
                gdk_window_destroy(gw->w);
                // Wait for it to die
                while (!gw->display->is_dying && !gdk_window_is_destroyed(gw->w)) {
                    GGDKDrawProcessPendingEvents((GDisplay *)gw->display);
                }
            }
        }

        // Signal that it has been destroyed - only if we're not cleaning up the display
        if (!gw->display->is_dying) {
            struct gevent die = {0};
            die.w = (GWindow)gw;
            die.native_window = gw->w;
            die.type = et_destroy;
            GGDKDrawPostEvent(&die);
        }

        // Remove all relevant timers that haven't been cleaned up by the user
        // Note: We do not free the GTimer struct as the user may then call DestroyTimer themselves...
        GList_Glib *ent = gw->display->timers;
        while (ent != NULL) {
            GList_Glib *next = ent->next;
            GGDKTimer *timer = (GGDKTimer *)ent->data;
            if (timer->owner == (GWindow)gw) {
                //Since we update the timer list ourselves, don't all GGDKDrawCancelTimer.
                Log(LOGDEBUG, "WARNING: Unstopped timer on window destroy!!! %p -> %p", gw, timer);
                timer->active = false;
                timer->stopped = true;
                g_source_remove(timer->glib_timeout_id);
                gw->display->timers = g_list_delete_link(gw->display->timers, ent);
            }
            ent = next;
        }

        // If it owns any of the selections, clear it
        for (int i = 0; i < sn_max; i++) {
            if (gw->display->selinfo[i].owner == gw) {
                _GGDKDraw_ClearSelData(gw->display, i);
            }
        }

        // Decrement the toplevel window count
        if (gw->display->groot == gw->parent && !gw->is_dlg) {
            gw->display->top_window_count--;
        }

        assert(gw->display->dirty_window != gw);

        Log(LOGDEBUG, "Window destroyed: %p[%p][%s][toplevel:%d][pixmap:%d]",
            gw, gw->w, gw->window_title, gw->is_toplevel, gw->is_pixmap);
        free(gw->window_title);
        if (gw != gw->display->groot) {
            // Unreference our reference to the window
            g_object_unref(G_OBJECT(gw->w));
        }
    }

    if (gw->pango_layout != NULL) {
        g_object_unref(gw->pango_layout);
    }

    if (gw->cs != NULL) {
        cairo_surface_destroy(gw->cs);
    }

    g_hash_table_remove(gw->display->windows, gw);

    // Remove our reference on the parent
    if (gw->parent != NULL && gw->parent != gw->display->groot) {
        GGDKDRAW_DECREF(gw->parent, _GGDKDraw_InitiateWindowDestroy);
    }

    free(gw->ggc);
    free(gw);
    return false;
}

// FF expects the destroy call to happen asynchronously to
// the actual GGDKDrawDestroyWindow call. So we add it to the queue...
static void _GGDKDraw_InitiateWindowDestroy(GGDKWindow gw) {
    if (gw->is_pixmap) {
        _GGDKDraw_OnWindowDestroyed(gw);
    } else if (!gw->is_cleaning_up) { // Check for nested call - if we're already being destroyed.
        // Note: This *MUST* be a 0-length timer so it always gets picked up on the next
        //       call to GDrawProcessPendingEvents. There are assumptions made that the destroy
        //       event will be invoked when that function is called after calling GDrawDestroyWindow.
        //       Ideally fix wherever the GDrawSync/GDrawProcessPendingEvents pattern is used
        //       to have a more concrete check that it's actually gone.
        g_timeout_add(0, _GGDKDraw_OnWindowDestroyed, gw);
    }
}

static void _GGDKDraw_OnTimerDestroyed(GGDKTimer *timer) {
    GGDKDisplay *gdisp = ((GGDKWindow)(timer->owner))->display;

    // We may stop the timer without destroying it -
    // e.g. if the user doesn't cancel the timer before a window is destroyed.
    if (!timer->stopped) {
        g_source_remove(timer->glib_timeout_id);
    }
    gdisp->timers = g_list_remove(gdisp->timers, timer);
    free(timer);
}

static gboolean _GGDKDraw_OnFakedConfigure(gpointer user_data) {
    GGDKWindow gw = (GGDKWindow)user_data;
    if (!gw->is_dying) {
        GdkEventConfigure evt = {0};

        evt.type       = GDK_CONFIGURE;
        evt.window     = gw->w;
        evt.send_event = true;
        evt.width      = gdk_window_get_width(gw->w);
        evt.height     = gdk_window_get_height(gw->w);
        gdk_window_get_position(gw->w, &evt.x, &evt.y);

        gdk_event_put((GdkEvent *)&evt);
    }
    gw->resize_timeout = 0;
    return false;
}

// In their infinite wisdom, GDK does not send configure events for child windows.
static void _GGDKDraw_FakeConfigureEvent(GGDKWindow gw) {
    if (gw->resize_timeout != 0) {
        g_source_remove(gw->resize_timeout);
        gw->resize_timeout = 0;
    }

    // Always fire the first faked configure event immediately
    if (!gw->has_had_faked_configure) {
        _GGDKDraw_OnFakedConfigure(gw);
        gw->has_had_faked_configure = true;
    } else {
        gw->resize_timeout = g_timeout_add(150, _GGDKDraw_OnFakedConfigure, gw);
    }
}

static void _GGDKDraw_CallEHChecked(GGDKWindow gw, GEvent *event, int (*eh)(GWindow gw, GEvent *)) {
    if (eh) {
        // Increment reference counter
        GGDKDRAW_ADDREF(gw);
        (eh)((GWindow)gw, event);

        // Cleanup after our user...
        _GGDKDraw_CleanupAutoPaint(gw->display);
        // Decrement reference counter
        GGDKDRAW_DECREF(gw, _GGDKDraw_InitiateWindowDestroy);
    }
}

static GdkDevice *_GGDKDraw_GetPointer(GGDKDisplay *gdisp) {
#ifdef GGDKDRAW_GDK_3_20
    GdkSeat *seat = gdk_display_get_default_seat(gdisp->display);
    if (seat == NULL) {
        return NULL;
    }

    return gdk_seat_get_pointer(seat);
#else
    GdkDeviceManager *manager = gdk_display_get_device_manager(gdisp->display);
    if (manager == NULL) {
        return NULL;
    }

    return gdk_device_manager_get_client_pointer(manager);
#endif
}

static void _GGDKDraw_CenterWindowOnScreen(GGDKWindow gw) {
    GGDKDisplay *gdisp = gw->display;
    GdkRectangle work_area, window_size;
    int x, y;

#ifdef GGDKDRAW_GDK_3_22
    GdkMonitor *monitor;

    gdk_device_get_position(_GGDKDraw_GetPointer(gdisp), NULL, &x, &y);
    monitor = gdk_display_get_monitor_at_point(gdisp->display, x, y);
    gdk_monitor_get_workarea(monitor, &work_area);
#else
    GdkScreen *pointer_screen;
    int monitor = 0;

    gdk_device_get_position(_GGDKDraw_GetPointer(gdisp), &pointer_screen, &x, &y);

    if (pointer_screen == gdisp->screen) { // Ensure it's on the same screen
        monitor = gdk_screen_get_monitor_at_point(gdisp->screen, x, y);
    }

    gdk_screen_get_monitor_workarea(pointer_screen, monitor, &work_area);
#endif // GGDKDRAW_GDK_3_22

    gdk_window_get_frame_extents(gw->w, &window_size);
    gw->pos.x = (work_area.width - window_size.width) / 2 + work_area.x;
    gw->pos.y = (work_area.height - window_size.height) / 2 + work_area.y;

    if (gw->pos.x < work_area.x) {
        gw->pos.x = work_area.x;
    }

    if (gw->pos.y < work_area.y) {
        gw->pos.y = work_area.y;
    }
    gdk_window_move(gw->w, gw->pos.x, gw->pos.y);
}

static GWindow _GGDKDraw_CreateWindow(GGDKDisplay *gdisp, GGDKWindow gw, GRect *pos,
                                      int (*eh)(GWindow, GEvent *), void *user_data, GWindowAttrs *wattrs) {

    GWindowAttrs temp = GWINDOWATTRS_EMPTY;
    GdkWindowAttr attribs = {0};
    gint attribs_mask = 0;
    GGDKWindow nw = (GGDKWindow)calloc(1, sizeof(struct ggdkwindow));

    if (nw == NULL) {
        Log(LOGDEBUG, "_GGDKDraw_CreateWindow: GGDKWindow calloc failed.");
        return NULL;
    }
    if (wattrs == NULL) {
        wattrs = &temp;
    }

    if (gw == NULL) { // Creating a top-level window. Set parent as default root.
        gw = gdisp->groot;
        attribs.window_type = GDK_WINDOW_TOPLEVEL;
    } else {
        attribs.window_type = GDK_WINDOW_CHILD;
    }

    // Now check window type
    if ((wattrs->mask & wam_nodecor) && wattrs->nodecoration) {
        // Is a modeless dialogue
        nw->is_popup = true;
        nw->is_dlg = true;
        nw->not_restricted = true;
        attribs.window_type = GDK_WINDOW_TEMP;
    } else if ((wattrs->mask & wam_isdlg) && wattrs->is_dlg) {
        nw->is_dlg = true;
    }
    if ((wattrs->mask & wam_notrestricted) && wattrs->not_restricted) {
        nw->not_restricted = true;
    }
    nw->is_toplevel = attribs.window_type != GDK_WINDOW_CHILD;

    // Drawing context
    nw->ggc = _GGDKDraw_NewGGC();
    if (nw->ggc == NULL) {
        Log(LOGDEBUG, "_GGDKDraw_CreateWindow: _GGDKDraw_NewGGC returned NULL");
        free(nw);
        return NULL;
    }

    // Base fields
    nw->display = gdisp;
    nw->eh = eh;
    nw->parent = gw;
    nw->pos = *pos;
    nw->user_data = user_data;

    // Window title, hints and event mask
    attribs.event_mask = GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK;
    if (nw->is_toplevel) {
        // Default event mask for toplevel windows
        attribs.event_mask |= GDK_FOCUS_CHANGE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;

        // Icon titles are ignored.
        if ((wattrs->mask & wam_utf8_wtitle) && (wattrs->utf8_window_title != NULL)) {
            nw->window_title = copy(wattrs->utf8_window_title);
        } else if ((wattrs->mask & wam_wtitle) && (wattrs->window_title != NULL)) {
            nw->window_title = u2utf8_copy(wattrs->window_title);
        }

        attribs.title = nw->window_title;
        attribs.type_hint = (nw->is_popup || (wattrs->mask & wam_palette)) ?
                            GDK_WINDOW_TYPE_HINT_UTILITY : GDK_WINDOW_TYPE_HINT_NORMAL;

        if (attribs.title != NULL) {
            attribs_mask |= GDK_WA_TITLE;
        }
        attribs_mask |= GDK_WA_TYPE_HINT;
    }

    // Further event mask flags
    if (wattrs->mask & wam_events) {
        if (wattrs->event_masks & (1 << et_char)) {
            attribs.event_mask |= GDK_KEY_PRESS_MASK;
        }
        if (wattrs->event_masks & (1 << et_charup)) {
            attribs.event_mask |= GDK_KEY_RELEASE_MASK;
        }
        if (wattrs->event_masks & (1 << et_mousemove)) {
            attribs.event_mask |= GDK_POINTER_MOTION_MASK;
        }
        if (wattrs->event_masks & (1 << et_mousedown)) {
            attribs.event_mask |= GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK;
        }
        if (wattrs->event_masks & (1 << et_mouseup)) {
            attribs.event_mask |= GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK;
        }
        if (wattrs->event_masks & (1 << et_visibility)) {
            attribs.event_mask |= GDK_VISIBILITY_NOTIFY_MASK;
        }
    }

    if (wattrs->mask & wam_restrict) {
        nw->restrict_input_to_me = wattrs->restrict_input_to_me;
    }
    if (wattrs->mask & wam_redirect) {
        nw->redirect_chars_to_me = wattrs->redirect_chars_to_me;
        nw->redirect_from = wattrs->redirect_from;
    }

    attribs.width = pos->width;
    attribs.height = pos->height;
    attribs.wclass = GDK_INPUT_OUTPUT;
    // GDK docs say to not use this. But that's because it's done by GTK...
    attribs.wmclass_name = GResourceProgramName;
    attribs.wmclass_class = GResourceProgramName;
    attribs_mask |= GDK_WA_WMCLASS;

    nw->w = gdk_window_new(gw->w, &attribs, attribs_mask);
    if (nw->w == NULL) {
        Log(LOGDEBUG, "GGDKDraw: Failed to create window!");
        free(nw->window_title);
        free(nw->ggc);
        free(nw);
        return NULL;
    }

    // We center windows here because we need to know the window size+decor
    // There is a bug on Windows (all versions < 3.21.1, <= 2.24.30) so don't use GDK_WA_X/GDK_WA_Y
    // https://bugzilla.gnome.org/show_bug.cgi?id=764996
    if (nw->is_toplevel && (!(wattrs->mask & wam_positioned) || (wattrs->mask & wam_centered))) {
        nw->is_centered = true;
        _GGDKDraw_CenterWindowOnScreen(nw);
    } else {
        gdk_window_move(nw->w, nw->pos.x, nw->pos.y);
    }

    // Set background
    if (!(wattrs->mask & wam_backcol) || wattrs->background_color == COLOR_DEFAULT) {
        wattrs->background_color = gdisp->def_background;
    }
    nw->ggc->bg = wattrs->background_color;
    GGDKDrawSetWindowBackground((GWindow)nw, wattrs->background_color);

    if (nw->is_toplevel) {
        // Set opaque region
        _GGDKDraw_SetOpaqueRegion(nw);

        // Set icon
        GGDKWindow icon = gdisp->default_icon;
        if (((wattrs->mask & wam_icon) && wattrs->icon != NULL) && ((GGDKWindow)wattrs->icon)->is_pixmap) {
            icon = (GGDKWindow) wattrs->icon;
        }
        if (icon != NULL) {
            GdkPixbuf *pb = gdk_pixbuf_get_from_surface(icon->cs, 0, 0, icon->pos.width, icon->pos.height);
            if (pb != NULL) {
                GList_Glib ent = {.data = pb};
                gdk_window_set_icon_list(nw->w, &ent);
                g_object_unref(pb);
            }
        } else {
            gdk_window_set_decorations(nw->w, GDK_DECOR_ALL | GDK_DECOR_MENU);
        }

        GdkGeometry geom = {0};
        GdkWindowHints hints = 0;
        if (wattrs->mask & wam_palette) {
            hints |= GDK_HINT_MIN_SIZE | GDK_HINT_BASE_SIZE;
        }
        if ((wattrs->mask & wam_noresize) && wattrs->noresize) {
            hints |= GDK_HINT_MIN_SIZE | GDK_HINT_MAX_SIZE | GDK_HINT_BASE_SIZE;
        }

        // Hmm does this seem right?
        geom.base_width = geom.min_width = geom.max_width = pos->width;
        geom.base_height = geom.min_height = geom.max_height = pos->height;

        hints |= GDK_HINT_POS;
        nw->was_positioned = true;

        gdk_window_set_geometry_hints(nw->w, &geom, hints);

        if ((wattrs->mask & wam_transient) && wattrs->transient != NULL) {
            GGDKDrawSetTransientFor((GWindow)nw, wattrs->transient);
            nw->is_dlg = true;
        } else if (!nw->is_dlg) {
            ++gdisp->top_window_count;
        } else if (nw->restrict_input_to_me && gdisp->mru_windows->length > 0) {
            GGDKDrawSetTransientFor((GWindow)nw, (GWindow) - 1);
        }
        nw->isverytransient = (wattrs->mask & wam_verytransient) ? 1 : 0;

        // Don't allow popups or 'very transient' windows from entering this list
        if (!nw->is_popup && !nw->isverytransient) {
            // This creates a single link with the data being the window, and next/prev NULL
            nw->mru_link = g_list_append(NULL, (gpointer)nw);
            if (nw->mru_link == NULL) {
                Log(LOGDEBUG, "Failed to create mru_link");
                gdk_window_destroy(nw->w);
                free(nw->window_title);
                free(nw->ggc);
                free(nw);
                return NULL;
            }
        }
    }

    // Establish Pango/Cairo context
    if (!_GGDKDraw_InitPangoCairo(nw)) {
        gdk_window_destroy(nw->w);
        free(nw->window_title);
        free(nw->ggc);
        free(nw->mru_link);
        free(nw);
        return NULL;
    }

    if ((wattrs->mask & wam_cursor) && wattrs->cursor != ct_default) {
        GGDKDrawSetCursor((GWindow)nw, wattrs->cursor);
    }

    // Add a reference to our own structure.
    GGDKDRAW_ADDREF(nw);

    // Add a reference on the parent window
    if (nw->parent != gdisp->groot) {
        GGDKDRAW_ADDREF(nw->parent);
    }

    if (nw->mru_link) {
        // Add it into the mru list
        Log(LOGDEBUG, "Adding %p(%s) to the mru list", nw, nw->window_title);
        g_queue_push_tail_link(gdisp->mru_windows, nw->mru_link);
    }

    // Event handler
    if (eh != NULL) {
        GEvent e = {0};
        e.type = et_create;
        e.w = (GWindow) nw;
        e.native_window = nw->w;
        _GGDKDraw_CallEHChecked(nw, &e, eh);
    }

    // Set the user data to the GWindow
    // Although there is gdk_window_set_user_data, if non-NULL,
    // GTK assumes it's a GTKWindow.
    g_object_set_data(G_OBJECT(nw->w), "GGDKWindow", nw);
    // Add our reference to the window
    // This will be unreferenced when the window is destroyed.
    g_object_ref(G_OBJECT(nw->w));
    // Add our window to the display's hashmap
    g_hash_table_add(gdisp->windows, nw);
    //gdk_window_move_resize(nw->w, nw->pos.x, nw->pos.y, nw->pos.width, nw->pos.height);

    Log(LOGDEBUG, "Window created: %p[%p][%s][toplevel:%d]", nw, nw->w, nw->window_title, nw->is_toplevel);
    return (GWindow)nw;
}

static GWindow _GGDKDraw_NewPixmap(GDisplay *disp, GWindow similar, uint16 width, uint16 height, bool is_bitmap,
                                   unsigned char *data) {
    GGDKDisplay *gdisp = (GGDKDisplay *)disp;
    GGDKWindow gw = (GGDKWindow)calloc(1, sizeof(struct ggdkwindow));
    if (gw == NULL) {
        Log(LOGDEBUG, "GGDKDRAW: GGDKWindow calloc failed!");
        return NULL;
    }

    gw->ggc = _GGDKDraw_NewGGC();
    if (gw->ggc == NULL) {
        Log(LOGDEBUG, "GGDKDRAW: GGC alloc failed!");
        free(gw);
        return NULL;
    }
    gw->ggc->bg = gdisp->def_background;
    width &= 0x7fff; // We're always using a cairo surface...

    gw->display = gdisp;
    gw->is_pixmap = 1;
    gw->parent = NULL;
    gw->pos.x = gw->pos.y = 0;
    gw->pos.width = width;
    gw->pos.height = height;

    if (data == NULL) {
        if (similar == NULL) {
            gw->cs = cairo_image_surface_create(is_bitmap ? CAIRO_FORMAT_A1 : CAIRO_FORMAT_ARGB32, width, height);
        } else {
            gw->cs = gdk_window_create_similar_surface(((GGDKWindow)similar)->w, CAIRO_CONTENT_COLOR, width, height);
        }
    } else {
        cairo_format_t format = is_bitmap ? CAIRO_FORMAT_A1 : CAIRO_FORMAT_ARGB32;
        gw->cs = cairo_image_surface_create_for_data(data, format, width, height, cairo_format_stride_for_width(format, width));
    }

    if (gw->cs == NULL) {
        Log(LOGDEBUG, "GGDKDRAW: Cairo image surface creation failed!");
        free(gw->ggc);
        free(gw);
        return NULL;
    }

    if (!_GGDKDraw_InitPangoCairo(gw)) {
        cairo_surface_destroy(gw->cs);
        free(gw->ggc);
        free(gw);
        return NULL;
    }
    // Add a reference to ourselves
    GGDKDRAW_ADDREF(gw);
    return (GWindow)gw;
}

static int16 _GGDKDraw_GdkModifierToKsm(GdkModifierType mask) {
    int16 state = 0;
    //Translate from mask to X11 state
    if (mask & GDK_SHIFT_MASK) {
        state |= ksm_shift;
    }
    if (mask & GDK_LOCK_MASK) {
        state |= ksm_capslock;
    }
    if (mask & GDK_CONTROL_MASK) {
        state |= ksm_control;
    }
    if (mask & GDK_MOD1_MASK) { //Guess
        state |= ksm_meta;
    }
    if (mask & GDK_MOD2_MASK) { //Guess
        state |= ksm_cmdmacosx;
    }
    if (mask & GDK_SUPER_MASK) {
        state |= ksm_super;
    }
    if (mask & GDK_HYPER_MASK) {
        state |= ksm_hyper;
    }
    //ksm_option?
    if (mask & GDK_BUTTON1_MASK) {
        state |= ksm_button1;
    }
    if (mask & GDK_BUTTON2_MASK) {
        state |= ksm_button2;
    }
    if (mask & GDK_BUTTON3_MASK) {
        state |= ksm_button3;
    }
    if (mask & GDK_BUTTON4_MASK) {
        state |= ksm_button4;
    }
    if (mask & GDK_BUTTON5_MASK) {
        state |= ksm_button5;
    }

    return state;
}

static VisibilityState _GGDKDraw_GdkVisibilityStateToVS(GdkVisibilityState state) {
    switch (state) {
        case GDK_VISIBILITY_FULLY_OBSCURED:
            return vs_obscured;
            break;
        case GDK_VISIBILITY_PARTIAL:
            return vs_partially;
            break;
        case GDK_VISIBILITY_UNOBSCURED:
        default:
            return vs_unobscured;
            break;
    }
}

static int _GGDKDraw_WindowOrParentsDying(GGDKWindow gw) {
    while (gw != NULL) {
        if (gw->is_dying) {
            return true;
        }
        if (gw->is_toplevel) {
            return false;
        }
        gw = gw->parent;
    }
    return false;
}

static bool _GGDKDraw_FilterByModal(GdkEvent *event, GGDKWindow gw) {
    switch (event->type) {
        case GDK_KEY_PRESS:
        case GDK_KEY_RELEASE:
        case GDK_BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
        case GDK_SCROLL:
        case GDK_MOTION_NOTIFY:
        case GDK_DELETE:
        case GDK_FOCUS_CHANGE:
            break;
        default:
            return false;
            break;
    }

    GGDKWindow gww = gw;
    GPtrArray *stack = gw->display->transient_stack;

    if (gww && gw->display->restrict_count == 0) {
        return false;
    }

    GGDKWindow last_modal = NULL;
    while (gww != NULL) {
        if (gww->is_toplevel) {
            for (int i = ((int)stack->len) - 1; i >= 0; --i) {
                GGDKWindow ow = (GGDKWindow)stack->pdata[i];
                if (ow == gww || ow->restrict_input_to_me) { // fudged
                    last_modal = ow;
                    break;
                }
            }

            if (last_modal == NULL || last_modal == gww) {
                return false;
            }
        }

        gww = gww->parent;
    }

    if (event->type != GDK_MOTION_NOTIFY && event->type != GDK_BUTTON_RELEASE && event->type != GDK_FOCUS_CHANGE) {
        gdk_window_beep(gw->w);
        if (last_modal != NULL) {
            GDrawRaise((GWindow)last_modal);
        }
    }

    return true;
}

static gboolean _GGDKDraw_ProcessTimerEvent(gpointer user_data) {
    GGDKTimer *timer = (GGDKTimer *)user_data;
    GEvent e = {0};
    bool ret = true;

    if (!timer->active || _GGDKDraw_WindowOrParentsDying((GGDKWindow)timer->owner)) {
        timer->active = false;
        timer->stopped = true;
        return false;
    }

    e.type = et_timer;
    e.w = timer->owner;
    e.native_window = timer->owner->native_window;
    e.u.timer.timer = (GTimer *)timer;
    e.u.timer.userdata = timer->userdata;

    GGDKDRAW_ADDREF(timer);
    _GGDKDraw_CallEHChecked((GGDKWindow)timer->owner, &e, timer->owner->eh);
    if (timer->active) {
        if (timer->repeat_time == 0) {
            timer->stopped = true; // Since we return false, this timer is no longer valid.
            GGDKDrawCancelTimer((GTimer *)timer);
            ret = false;
        } else if (timer->has_differing_repeat_time) {
            timer->has_differing_repeat_time = false;
            timer->glib_timeout_id = g_timeout_add(timer->repeat_time, _GGDKDraw_ProcessTimerEvent, timer);
            ret = false;
        }
    }

    GGDKDRAW_DECREF(timer, _GGDKDraw_OnTimerDestroyed);
    return ret;
}

static void _GGDKDraw_DispatchEvent(GdkEvent *event, gpointer data) {
    //static int request_id = 0;
    struct gevent gevent = {0};
    GGDKDisplay *gdisp = (GGDKDisplay *)data;
    GdkWindow *w = ((GdkEventAny *)event)->window;
    GGDKWindow gw;
    guint32 event_time = gdk_event_get_time(event);
    if (event_time != GDK_CURRENT_TIME) {
        gdisp->last_event_time = event_time;
    }

    //Log(LOGDEBUG, "[%d] Received event %d(%s) %p", request_id++, event->type, GdkEventName(event->type), w);
    //fflush(stderr);

    if (w == NULL) {
        return;
    } else if ((gw = g_object_get_data(G_OBJECT(w), "GGDKWindow")) == NULL) {
        //Log(LOGDEBUG, "MISSING GW!");
        return;
    } else if (_GGDKDraw_WindowOrParentsDying(gw) || gdk_window_is_destroyed(w)) {
        Log(LOGDEBUG, "DYING! %p", w);
        return;
    } else if (_GGDKDraw_FilterByModal(event, gw)) {
        Log(LOGDEBUG, "Discarding event - has modals!");
        return;
    }

    gevent.w = (GWindow)gw;
    gevent.native_window = (void *)gw->w;
    gevent.type = et_noevent;

    switch (event->type) {
        case GDK_KEY_PRESS:
        case GDK_KEY_RELEASE: {
            GdkEventKey *key = (GdkEventKey *)event;
            gevent.type = event->type == GDK_KEY_PRESS ? et_char : et_charup;
            gevent.u.chr.state = _GGDKDraw_GdkModifierToKsm(((GdkEventKey *)event)->state);

#ifdef GDK_WINDOWING_QUARTZ
            // On Mac, the Alt/Option key is used for alternate input.
            // We want accelerators, so translate ourselves, forcing the group to 0.
            if ((gevent.u.chr.state & ksm_meta) && key->group != 0) {
                GdkKeymap *km = gdk_keymap_get_for_display(gdisp->display);
                guint keyval;

                gdk_keymap_translate_keyboard_state(km, key->hardware_keycode,
                    key->state, 0, &keyval, NULL, NULL, NULL);

                //Log(LOGDEBUG, "Fixed keyval from 0x%x(%s) -> 0x%x(%s)",
                //	key->keyval, gdk_keyval_name(key->keyval),
                //	keyval, gdk_keyval_name(keyval));

                key->keyval = keyval;
            }
#endif

            gevent.u.chr.autorepeat =
                event->type    == GDK_KEY_PRESS &&
                gdisp->ks.type == GDK_KEY_PRESS &&
                key->keyval    == gdisp->ks.keyval &&
                key->state     == gdisp->ks.state;
            // Mumble mumble Mac
            //if ((event->xkey.state & ksm_option) && gdisp->macosx_cmd) {
            //    gevent.u.chr.state |= ksm_meta;
            //}

            if (gevent.u.chr.autorepeat && GKeysymIsModifier(key->keyval)) {
                gevent.type = et_noevent;
            } else {
                if (key->keyval == GDK_KEY_space) {
                    gw->display->is_space_pressed = event->type == GDK_KEY_PRESS;
                }

                gevent.u.chr.keysym = key->keyval;
                gevent.u.chr.chars[0] = gdk_keyval_to_unicode(key->keyval);
            }

            gdisp->ks.type   = key->type;
            gdisp->ks.keyval = key->keyval;
            gdisp->ks.state  = key->state;
        }
        break;
        case GDK_MOTION_NOTIFY: {
            GdkEventMotion *evt = (GdkEventMotion *)event;
            gevent.type = et_mousemove;
            gevent.u.mouse.state = _GGDKDraw_GdkModifierToKsm(evt->state);
            gevent.u.mouse.x = evt->x;
            gevent.u.mouse.y = evt->y;
            //Log(LOGDEBUG, "Motion: [%f %f]", evt->x, evt->y);
        }
        break;
        case GDK_SCROLL: { //Synthesize a button press
            GdkEventScroll *evt = (GdkEventScroll *)event;
            gevent.u.mouse.state = _GGDKDraw_GdkModifierToKsm(evt->state);
            gevent.u.mouse.x = evt->x;
            gevent.u.mouse.y = evt->y;
            gevent.u.mouse.clicks = gdisp->bs.cur_click = 1;
            gevent.type = et_mousedown;

            switch (evt->direction) {
                case GDK_SCROLL_UP:
                    gevent.u.mouse.button = 4;
                    break;
                case GDK_SCROLL_DOWN:
                    gevent.u.mouse.button = 5;
                    break;
                case GDK_SCROLL_LEFT:
                    gevent.u.mouse.button = 6;
                    break;
                case GDK_SCROLL_RIGHT:
                    gevent.u.mouse.button = 7;
                    break;
                default: // Ignore GDK_SCROLL_SMOOTH
                    gevent.type = et_noevent;
            }
            // We need to simulate two events... I think.
            if (gevent.type != et_noevent) {
                GGDKDrawPostEvent(&gevent);
                gevent.type = et_mouseup;
            }
        }
        break;
        case GDK_BUTTON_PRESS:
        case GDK_BUTTON_RELEASE: {
            GdkEventButton *evt = (GdkEventButton *)event;
            gevent.u.mouse.state = _GGDKDraw_GdkModifierToKsm(evt->state);
            gevent.u.mouse.x = evt->x;
            gevent.u.mouse.y = evt->y;
            gevent.u.mouse.button = evt->button;
            gevent.u.mouse.time = evt->time;

            Log(LOGDEBUG, "Button %7s: [%f %f]", evt->type == GDK_BUTTON_PRESS ? "press" : "release", evt->x, evt->y);

#ifdef GDK_WINDOWING_QUARTZ
            // Quartz backend fails to give a button press event
            // https://bugzilla.gnome.org/show_bug.cgi?id=769961
            if (!evt->send_event && evt->type == GDK_BUTTON_RELEASE &&
                    gdisp->bs.release_w == gw &&
                    (evt->x < 0 || evt->x > gw->pos.width ||
                     evt->y < 0 || evt->y > gw->pos.height)) {
                evt->send_event = true;
                gdk_event_put(event);
                event->type = GDK_BUTTON_PRESS;
            }
#endif

            if (event->type == GDK_BUTTON_PRESS) {
                int xdiff, ydiff;
                gevent.type = et_mousedown;

                xdiff = abs(((int)evt->x) - gdisp->bs.release_x);
                ydiff = abs(((int)evt->y) - gdisp->bs.release_y);

                if (xdiff + ydiff < gdisp->bs.double_wiggle &&
                        gw == gdisp->bs.release_w &&
                        gevent.u.mouse.button == gdisp->bs.release_button &&
                        (int32_t)(gevent.u.mouse.time - gdisp->bs.last_press_time) < gdisp->bs.double_time &&
                        gevent.u.mouse.time >= gdisp->bs.last_press_time) {  // Time can wrap

                    gdisp->bs.cur_click++;
                } else {
                    gdisp->bs.cur_click = 1;
                }
                gdisp->bs.last_press_time = gevent.u.mouse.time;
            } else {
                gevent.type = et_mouseup;
                gdisp->bs.release_w = gw;
                gdisp->bs.release_x = evt->x;
                gdisp->bs.release_y = evt->y;
                gdisp->bs.release_button = evt->button;
            }
            gevent.u.mouse.clicks = gdisp->bs.cur_click;
        }
        break;
        case GDK_EXPOSE: {
            GdkEventExpose *expose = (GdkEventExpose *)event;
            // Okay. So on GDK3, we must exclude child window regions to prevent flicker.
            // However, we cannot modify the expose event's region directly, as this is
            // used by GDK to determine what child windows to invalidate. Hence, we must
            // make a copy of the region, sans any child window areas.
            cairo_rectangle_int_t extents;
            cairo_region_t *reg = _GGDKDraw_ExcludeChildRegions(gw, expose->region, true);
            cairo_region_get_extents(reg, &extents);

            gevent.type = et_expose;
            gevent.u.expose.rect.x = extents.x;
            gevent.u.expose.rect.y = extents.y;
            gevent.u.expose.rect.width = extents.width;
            gevent.u.expose.rect.height = extents.height;

            // This can happen if say gdk_window_raise is called, which then runs the
            // event loop itself, which is outside of our checked event handler
            // I've added autopaint cleanups before such calls, so this should
            // never happen any more.
            assert(gw->cc == NULL);

#ifdef GGDKDRAW_GDK_3_22
            gw->drawing_ctx = gdk_window_begin_draw_frame(w, reg);
#else
            gdk_window_begin_paint_region(w, reg);
#endif
            gw->is_in_paint = true;
            gdisp->dirty_window = gw;

            // But wait, there's more! On GDK3, if the window isn't native,
            // gdk_window_begin_paint_region does nothing.
            // So we have to (a) ensure we draw to an offscreen surface,
            // and (b) clip the region appropriately. But on Windows
            // (and maybe Quartz), it seems to double buffer anyway.
            if (!gdk_window_has_native(gw->w)) {
#if !defined(GDK_WINDOWING_WIN32) && !defined(GDK_WINDOWING_QUARTZ)
                double sx = 1, sy = 1;

                gw->cs = gdk_window_create_similar_surface(gw->w, CAIRO_CONTENT_COLOR, extents.width, extents.height);
#if (CAIRO_VERSION_MAJOR >= 2) || (CAIRO_VERSION_MAJOR >= 1 && CAIRO_VERSION_MINOR >= 14)
                cairo_surface_get_device_scale(gw->cs, &sx, &sy);
#endif
                cairo_surface_set_device_offset(gw->cs, -extents.x * sx, -extents.y * sy);
                gw->cc = cairo_create(gw->cs);
                gw->expose_region = cairo_region_reference(reg);
#else
#ifdef GGDKDRAW_GDK_3_22
                gw->cc = cairo_reference(gdk_drawing_context_get_cairo_context(gw->drawing_ctx));
#else
                gw->cc = gdk_cairo_create(gw->w);
#endif
#endif
                gdk_cairo_region(gw->cc, reg);
                cairo_clip(gw->cc);
            }
            cairo_region_destroy(reg);
        }
        break;
        case GDK_VISIBILITY_NOTIFY:
            gevent.type = et_visibility;
            gevent.u.visibility.state = _GGDKDraw_GdkVisibilityStateToVS(((GdkEventVisibility *)event)->state);
            break;
        case GDK_FOCUS_CHANGE:
            gevent.type = et_focus;
            gevent.u.focus.gained_focus = ((GdkEventFocus *)event)->in;
            gevent.u.focus.mnemonic_focus = false;

            Log(LOGDEBUG, "Focus change %s %p(%s %d %d)",
                gevent.u.focus.gained_focus ? "IN" : "OUT",
                gw, gw->window_title, gw->is_toplevel, gw->istransient);

            if (gw->mru_link && gevent.u.focus.gained_focus) {
                GGDKWindow cur_toplevel = (GGDKWindow)g_queue_peek_head(gdisp->mru_windows);
                if (cur_toplevel != gw) {
                    Log(LOGDEBUG, "Last active toplevel updated: %p(%s)", gw, gw->window_title);
                    g_queue_unlink(gdisp->mru_windows, gw->mru_link);
                    g_queue_push_head_link(gdisp->mru_windows, gw->mru_link);
                }
            }

            break;
        case GDK_ENTER_NOTIFY:
        case GDK_LEAVE_NOTIFY: { // Should only get this on top level
            GdkEventCrossing *crossing = (GdkEventCrossing *)event;
            if (crossing->focus) { //Focus or inferior
                break;
            }
            if (gdisp->focusfollowsmouse && gw != NULL && gw->eh != NULL) {
                gevent.type = et_focus;
                gevent.u.focus.gained_focus = crossing->type == GDK_ENTER_NOTIFY;
                gevent.u.focus.mnemonic_focus = false;
                _GGDKDraw_CallEHChecked(gw, &gevent, gw->eh);
            }
            gevent.type = et_crossing;
            gevent.u.crossing.x = crossing->x;
            gevent.u.crossing.y = crossing->y;
            gevent.u.crossing.state = _GGDKDraw_GdkModifierToKsm(crossing->state);
            gevent.u.crossing.entered = crossing->type == GDK_ENTER_NOTIFY;
            gevent.u.crossing.device = NULL;
            gevent.u.crossing.time = crossing->time;

            // Always set to false when crossing boundary...
            gw->display->is_space_pressed = false;
        }
        break;
        case GDK_CONFIGURE: {
            GdkEventConfigure *configure = (GdkEventConfigure *)event;
            gevent.type = et_resize;
            gevent.u.resize.size.x      = configure->x;
            gevent.u.resize.size.y      = configure->y;
            gevent.u.resize.size.width  = configure->width;
            gevent.u.resize.size.height = configure->height;
            gevent.u.resize.dx          = configure->x - gw->pos.x;
            gevent.u.resize.dy          = configure->y - gw->pos.y;
            gevent.u.resize.dwidth      = configure->width - gw->pos.width;
            gevent.u.resize.dheight     = configure->height - gw->pos.height;
            gevent.u.resize.moved       = gevent.u.resize.sized = false;
            if (gevent.u.resize.dx != 0 || gevent.u.resize.dy != 0) {
                gevent.u.resize.moved = true;
                gw->is_centered = false;
            }
            if (gevent.u.resize.dwidth != 0 || gevent.u.resize.dheight != 0) {
                gevent.u.resize.sized = true;
            }

            // I could make this Windows specific... But it doesn't seem necessary on other platforms too.
            // On Windows, repeated configure messages are sent if we move the window around.
            // This causes CPU usage to go up because mouse handlers of this message just redraw the whole window.
            if (gw->is_toplevel && !gevent.u.resize.sized && gevent.u.resize.moved) {
                gevent.type = et_noevent;
                Log(LOGDEBUG, "Configure DISCARDED: %p:%s, %d %d %d %d", gw, gw->window_title, gw->pos.x, gw->pos.y, gw->pos.width, gw->pos.height);
            } else {
                Log(LOGDEBUG, "CONFIGURED: %p:%s, %d %d %d %d", gw, gw->window_title, gw->pos.x, gw->pos.y, gw->pos.width, gw->pos.height);
            }
            gw->pos = gevent.u.resize.size;

            // Update the opaque region (we're always completely opaque)
            // Although I don't actually know if this is completely necessary
            if (gw->is_toplevel && gevent.u.resize.sized) {
                _GGDKDraw_SetOpaqueRegion(gw);
            }
        }
        break;
        case GDK_MAP:
            gevent.type = et_map;
            gevent.u.map.is_visible = true;
            gw->is_visible = true;
            break;
        case GDK_UNMAP:
            gevent.type = et_map;
            gevent.u.map.is_visible = false;
            gw->is_visible = false;
            break;
        case GDK_DESTROY:
            GGDKDrawDestroyWindow((GWindow)gw); //Note: If we get here, something's probably wrong.
            break;
        case GDK_DELETE:
            gw->is_waiting_for_selection = false;
            gevent.type = et_close;
            break;
        case GDK_SELECTION_CLEAR: {
            gevent.type = et_selclear;
            gevent.u.selclear.sel = sn_primary;
            for (int i = 0; i < sn_max; i++) {
                GdkEventSelection *se = (GdkEventSelection *)event;
                if (se->selection == gdisp->selinfo[i].sel_atom) {
                    gevent.u.selclear.sel = i;
                    break;
                }
            }
            _GGDKDraw_ClearSelData(gdisp, gevent.u.selclear.sel);
        }
        break;
        case GDK_SELECTION_REQUEST: {
            GdkEventSelection *e = (GdkEventSelection *)event;
            gdk_selection_send_notify(
                e->requestor, e->selection, e->target,
                _GGDKDraw_TransmitSelection(gdisp, e) ? e->property : GDK_NONE,
                e->time);
        }
        break;
        case GDK_SELECTION_NOTIFY: // paste
            if (gw->is_waiting_for_selection) {
                gw->is_waiting_for_selection = false;
                gw->is_notified_of_selection = ((GdkEventSelection *)event)->property != GDK_NONE;
            }
            break;
        case GDK_PROPERTY_NOTIFY:
            break;
        default:
            Log(LOGDEBUG, "UNPROCESSED GDK EVENT %d %s", event->type, GdkEventName(event->type));
            break;
    }

    if (gevent.type != et_noevent && gw != NULL && gw->eh != NULL) {
        _GGDKDraw_CallEHChecked(gw, &gevent, gw->eh);
    }
    //Log(LOGDEBUG, "[%d] Finished processing %d(%s)", request_id++, event->type, GdkEventName(event->type));
}

static void GGDKDrawInit(GDisplay *gdisp) {
    FState *fs = calloc(1, sizeof(FState));
    if (fs == NULL) {
        Log(LOGDEBUG, "GGDKDraw: FState alloc failed!");
        assert(false);
    }

    // In inches, because that's how fonts are measured
    gdisp->fontstate = fs;
    fs->res = gdisp->res;
}

static void GGDKDrawSetDefaultIcon(GWindow icon) {
    GGDKWindow gicon = (GGDKWindow)icon;
    if (gicon->is_pixmap) {
        gicon->display->default_icon = gicon;
    }
}

static GWindow GGDKDrawCreateTopWindow(GDisplay *gdisp, GRect *pos, int (*eh)(GWindow gw, GEvent *), void *user_data,
                                       GWindowAttrs *gattrs) {
    Log(LOGDEBUG, " ");
    return _GGDKDraw_CreateWindow((GGDKDisplay *) gdisp, NULL, pos, eh, user_data, gattrs);
}

static GWindow GGDKDrawCreateSubWindow(GWindow gw, GRect *pos, int (*eh)(GWindow gw, GEvent *), void *user_data,
                                       GWindowAttrs *gattrs) {
    Log(LOGDEBUG, " ");
    return _GGDKDraw_CreateWindow(((GGDKWindow) gw)->display, (GGDKWindow) gw, pos, eh, user_data, gattrs);
}

static GWindow GGDKDrawCreatePixmap(GDisplay *gdisp, GWindow similar, uint16 width, uint16 height) {
    Log(LOGDEBUG, " ");

    //TODO: Check format?
    return _GGDKDraw_NewPixmap(gdisp, similar, width, height, false, NULL);
}

static GWindow GGDKDrawCreateBitmap(GDisplay *gdisp, uint16 width, uint16 height, uint8 *data) {
    Log(LOGDEBUG, " ");
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A1, width);
    int actual = (width & 0x7fff) / 8;

    if (actual != stride) {
        GWindow ret = _GGDKDraw_NewPixmap(gdisp, NULL, width, height, true, NULL);
        if (ret == NULL) {
            return NULL;
        }

        cairo_surface_flush(((GGDKWindow)ret)->cs);
        uint8 *buf = cairo_image_surface_get_data(((GGDKWindow)ret)->cs);
        for (int j = 0; j < height; j++) {
            memcpy(buf + stride * j, data + actual * j, actual);
        }
        cairo_surface_mark_dirty(((GGDKWindow)ret)->cs);
        return ret;
    }

    return _GGDKDraw_NewPixmap(gdisp, NULL, width, height, true, data);
}

static GCursor GGDKDrawCreateCursor(GWindow src, GWindow mask, Color fg, Color bg, int16 x, int16 y) {
    Log(LOGDEBUG, " ");

    GGDKDisplay *gdisp = (GGDKDisplay *)(src->display);
    GdkCursor *cursor = NULL;
    if (mask == NULL) { // Use src directly
        assert(src != NULL);
        assert(src->is_pixmap);
        cursor = gdk_cursor_new_from_surface(gdisp->display, ((GGDKWindow)src)->cs, x, y);
    } else { // Assume it's an X11-style cursor
        cairo_surface_t *cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, src->pos.width, src->pos.height);
        cairo_t *cc = cairo_create(cs);

        // Masking
        //Background
        cairo_set_source_rgb(cc, COLOR_RED(bg) / 255., COLOR_GREEN(bg) / 255., COLOR_BLUE(bg) / 255.);
        cairo_mask_surface(cc, ((GGDKWindow)mask)->cs, 0, 0);
        //Foreground
        cairo_set_source_rgb(cc, COLOR_RED(fg) / 255., COLOR_GREEN(fg) / 255., COLOR_BLUE(fg) / 255.);
        cairo_mask_surface(cc, ((GGDKWindow)src)->cs, 0, 0);

        cursor = gdk_cursor_new_from_surface(gdisp->display, cs, x, y);

        cairo_destroy(cc);
        cairo_surface_destroy(cs);
    }

    g_ptr_array_add(gdisp->cursors, cursor);
    return ct_user + (gdisp->cursors->len - 1);
}

static void GGDKDrawDestroyCursor(GDisplay *disp, GCursor gcursor) {
    Log(LOGDEBUG, " ");

    GGDKDisplay *gdisp = (GGDKDisplay *)disp;
    gcursor -= ct_user;
    if ((int)gcursor >= 0 && gcursor < gdisp->cursors->len) {
        g_object_unref(gdisp->cursors->pdata[gcursor]);
        gdisp->cursors->pdata[gcursor] = NULL;
    }
}

static void GGDKDrawDestroyWindow(GWindow w) {
    GGDKWindow gw = (GGDKWindow) w;
    Log(LOGDEBUG, "%p[%p][%s][toplevel:%d][pixmap:%d]",
        gw, gw->w, gw->window_title, gw->is_toplevel, gw->is_pixmap);

    if (gw->is_dying) {
        return;
    }

    gw->is_dying = true;
    gw->is_waiting_for_selection = false;
    gw->is_notified_of_selection = false;

    if (!gw->is_pixmap && gw != gw->display->groot) {
        GList_Glib *list = gdk_window_get_children(gw->w);
        GList_Glib *child = list;
        while (child != NULL) {
            GWindow c = g_object_get_data(child->data, "GGDKWindow");
            assert(c != NULL);
            GGDKDrawDestroyWindow(c);
            child = child->next;
        }
        g_list_free(list);

        if (gw->is_toplevel) {
            // Remove itself from the transient list, if present
            GGDKDrawSetTransientFor((GWindow)gw, NULL);

            // Remove it from the mru window list
            if (gw->mru_link != NULL) {
                Log(LOGDEBUG, "Removing %p(%s) from mru window list", gw, gw->window_title);
                g_queue_delete_link(gw->display->mru_windows, gw->mru_link);
                gw->mru_link = NULL;
            }
            // This is so that e.g. popup menus get hidden immediately
            // On Quartz, this is also necessary for toplevel windows,
            // as focus change events are not received when a window
            // is destroyed, but they *are* when it's first hidden...
            GDrawSetVisible((GWindow)gw, false);

            // HACK! Reparent all windows transient to this
            // If they were truly transient in the normal sense, they would just be
            // destroyed when this window goes away
            for (int i = ((int)gw->display->transient_stack->len) - 1; i >= 0; --i) {
                GGDKWindow tw = (GGDKWindow)gw->display->transient_stack->pdata[i];
                if (tw->transient_owner == gw) {
                    Log(LOGWARN, "Resetting transient owner on %p(%s) as %p(%s) is dying",
                        tw, tw->window_title, gw, gw->window_title);
                    GGDKDrawSetTransientFor((GWindow)tw, (GWindow)-1);
                }
            }
        }

        if (gw->display->last_dd.w == gw) {
            gw->display->last_dd.w = NULL;
        }
        // Ensure an invalid value is returned if someone tries to get this value again.
        g_object_set_data(G_OBJECT(gw->w), "GGDKWindow", NULL);
    }
    GGDKDRAW_DECREF(gw, _GGDKDraw_InitiateWindowDestroy);
}

static int GGDKDrawNativeWindowExists(GDisplay *UNUSED(gdisp), void *native_window) {
    //Log(LOGDEBUG, " ");
    GdkWindow *w = (GdkWindow *)native_window;

    if (w != NULL) {
        GGDKWindow gw = g_object_get_data(G_OBJECT(w), "GGDKWindow");
        // So if the window is dying, the gdk window is already gone.
        // But gcontainer.c expects this to return true on et_destroy...
        if (gw == NULL || gw->is_dying) {
            return true;
        } else {
            return !gdk_window_is_destroyed(w);
        }
    }
    return false;
}

static void GGDKDrawSetZoom(GWindow UNUSED(gw), GRect *UNUSED(size), enum gzoom_flags UNUSED(flags)) {
    //Log(LOGDEBUG, " ");
    // Not implemented.
}

static void GGDKDrawSetWindowBackground(GWindow w, Color gcol) {
    Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    GdkRGBA col = {
        .red = COLOR_RED(gcol) / 255.,
        .green = COLOR_GREEN(gcol) / 255.,
        .blue = COLOR_BLUE(gcol) / 255.,
        .alpha = 1.
    };
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gdk_window_set_background_rgba(gw->w, &col);
G_GNUC_END_IGNORE_DEPRECATIONS
}

static int GGDKDrawSetDither(GDisplay *UNUSED(gdisp), int UNUSED(set)) {
    // Not implemented; does nothing.
    return false;
}

static void GGDKDrawSetVisible(GWindow w, int show) {
    Log(LOGDEBUG, "0x%p %d", w, show);
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CleanupAutoPaint(gw->display);
    if (show) {
        if (gdk_window_is_visible(gw->w)) {
            Log(LOGDEBUG, "0x%p %d DISCARDED", w, show);
            return;
        }
#ifdef GDK_WINDOWING_QUARTZ
        // Quartz backend fails to send a configure event after showing a window
        // But FF expects one.
        _GGDKDraw_OnFakedConfigure(gw);
#endif
        gdk_window_show(gw->w);
        if (gw->restrict_input_to_me && gw->transient_owner == NULL && gw->display->mru_windows->length > 0) {
            GGDKDrawSetTransientFor((GWindow)gw, (GWindow) - 1);
        }
    } else {
        GGDKDrawSetTransientFor((GWindow)gw, NULL);
        gdk_window_hide(gw->w);
    }
}

static void GGDKDrawMove(GWindow gw, int32 x, int32 y) {
    Log(LOGDEBUG, "%p:%s, %d %d", gw, ((GGDKWindow)gw)->window_title, x, y);
    _GGDKDraw_CleanupAutoPaint(((GGDKWindow)gw)->display);
    gdk_window_move(((GGDKWindow)gw)->w, x, y);
    ((GGDKWindow)gw)->is_centered = false;
    if (!gw->is_toplevel) {
        _GGDKDraw_FakeConfigureEvent((GGDKWindow)gw);
    }
}

static void GGDKDrawTrueMove(GWindow w, int32 x, int32 y) {
    Log(LOGDEBUG, " ");
    GGDKDrawMove(w, x, y);
}

static void GGDKDrawResize(GWindow gw, int32 w, int32 h) {
    Log(LOGDEBUG, "%p:%s, %d %d", gw, ((GGDKWindow)gw)->window_title, w, h);
    _GGDKDraw_CleanupAutoPaint(((GGDKWindow)gw)->display);
    gdk_window_resize(((GGDKWindow)gw)->w, w, h);
    if (gw->is_toplevel && ((GGDKWindow)gw)->is_centered) {
        _GGDKDraw_CenterWindowOnScreen((GGDKWindow)gw);
    }
    if (!gw->is_toplevel) {
        _GGDKDraw_FakeConfigureEvent((GGDKWindow)gw);
    }
}

static void GGDKDrawMoveResize(GWindow gw, int32 x, int32 y, int32 w, int32 h) {
    Log(LOGDEBUG, "%p:%s, %d %d %d %d", gw, ((GGDKWindow)gw)->window_title, x, y, w, h);
    _GGDKDraw_CleanupAutoPaint(((GGDKWindow)gw)->display);
    gdk_window_move_resize(((GGDKWindow)gw)->w, x, y, w, h);
    if (!gw->is_toplevel) {
        _GGDKDraw_FakeConfigureEvent((GGDKWindow)gw);
    }
}

static void GGDKDrawRaise(GWindow w) {
    GGDKWindow gw = (GGDKWindow) w;
    Log(LOGDEBUG, "%p[%p][%s]", gw, gw->w, gw->window_title);
    if (!gw->is_visible) {
        Log(LOGINFO, "Discarding raise on hidden window: %p[%p][%s]",
            gw, gw->w, gw->window_title);
        return;
    }

    _GGDKDraw_CleanupAutoPaint(gw->display);
    gdk_window_raise(gw->w);
    if (!w->is_toplevel) {
        _GGDKDraw_FakeConfigureEvent(gw);
    }
}

static void GGDKDrawRaiseAbove(GWindow gw1, GWindow gw2) {
    Log(LOGDEBUG, " ");
    _GGDKDraw_CleanupAutoPaint(((GGDKWindow)gw1)->display);
    gdk_window_restack(((GGDKWindow)gw1)->w, ((GGDKWindow)gw2)->w, true);
    if (!gw1->is_toplevel) {
        _GGDKDraw_FakeConfigureEvent((GGDKWindow)gw1);
    }
    if (!gw2->is_toplevel) {
        _GGDKDraw_FakeConfigureEvent((GGDKWindow)gw2);
    }
}

// Only used once in gcontainer - force it to call GDrawRaiseAbove
static int GGDKDrawIsAbove(GWindow UNUSED(gw1), GWindow UNUSED(gw2)) {
    Log(LOGDEBUG, " ");
    return false;
}

static void GGDKDrawLower(GWindow gw) {
    Log(LOGDEBUG, " ");
    _GGDKDraw_CleanupAutoPaint(((GGDKWindow)gw)->display);
    gdk_window_lower(((GGDKWindow)gw)->w);
    if (!gw->is_toplevel) {
        _GGDKDraw_FakeConfigureEvent((GGDKWindow)gw);
    }
}

// Icon title is ignored.
static void GGDKDrawSetWindowTitles8(GWindow w, const char *title, const char *UNUSED(icontitle)) {
    Log(LOGDEBUG, " ");// assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    free(gw->window_title);
    gw->window_title = copy(title);

    if (title != NULL && gw->is_toplevel) {
        gdk_window_set_title(gw->w, title);
    }
}

static void GGDKDrawSetWindowTitles(GWindow gw, const unichar_t *title, const unichar_t *UNUSED(icontitle)) {
    Log(LOGDEBUG, " ");
    char *str = u2utf8_copy(title);
    if (str != NULL) {
        GGDKDrawSetWindowTitles8(gw, str, NULL);
        free(str);
    }
}

// Sigh. GDK doesn't provide a way to get the window title...
static unichar_t *GGDKDrawGetWindowTitle(GWindow gw) {
    Log(LOGDEBUG, " "); // assert(false);
    return utf82u_copy(((GGDKWindow)gw)->window_title);
}

static char *GGDKDrawGetWindowTitle8(GWindow gw) {
    Log(LOGDEBUG, " ");
    return copy(((GGDKWindow)gw)->window_title);
}

static void GGDKDrawSetTransientFor(GWindow transient, GWindow owner) {
    Log(LOGDEBUG, "transient=%p, owner=%p", transient, owner);
    GGDKWindow gw = (GGDKWindow) transient, ow = NULL;
    GGDKDisplay *gdisp = gw->display;
    assert(owner == NULL || gw->is_toplevel);

    if (!gw->is_toplevel) {
        return; // Transient child windows?!
    }

    if (owner == (GWindow) - 1) {
        for (GList_Glib *pw = gdisp->mru_windows->head; pw != NULL; pw = pw->next) {
            GGDKWindow tw = (GGDKWindow)pw->data;
            if (tw != gw && tw->is_visible) {
                ow = tw;
                break;
            }
        }
    } else if (owner == NULL) {
        ow = NULL; // Does this work with GDK?
    } else {
        ow = (GGDKWindow)owner;
    }

    if (gw->transient_owner != NULL) {
        for (int i = ((int)gdisp->transient_stack->len) - 1; i >= 0; --i) {
            if (gw == (GGDKWindow)gdisp->transient_stack->pdata[i]) {
                g_ptr_array_remove_index(gw->display->transient_stack, i);
                if (gw->restrict_input_to_me) {
                    gdisp->restrict_count--;
                }
                break;
            }
        }
    }

    if (ow != NULL) {
        gdk_window_set_transient_for(gw->w, ow->w);
        gdk_window_set_modal_hint(gw->w, true);
        gw->istransient = true;
        g_ptr_array_add(gdisp->transient_stack, gw);
        if (gw->restrict_input_to_me) {
            gdisp->restrict_count++;
        }
    } else {
        gdk_window_set_modal_hint(gw->w, false);
        gdk_window_set_transient_for(gw->w, gw->display->groot->w);
        gw->istransient = false;
    }

    gw->transient_owner = ow;
}

static void GGDKDrawGetPointerPosition(GWindow w, GEvent *ret) {
    Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    GdkModifierType mask;
    int x, y;

    GdkDevice *pointer = _GGDKDraw_GetPointer(gw->display);
    if (pointer == NULL) {
        ret->u.mouse.x = 0;
        ret->u.mouse.y = 0;
        ret->u.mouse.state = 0;
        return;
    }

    gdk_window_get_device_position(gw->w, pointer, &x, &y, &mask);
    ret->u.mouse.x = x;
    ret->u.mouse.y = y;
    ret->u.mouse.state = _GGDKDraw_GdkModifierToKsm(mask);
}

static GWindow GGDKDrawGetPointerWindow(GWindow gw) {
    Log(LOGDEBUG, " ");
    GdkDevice *pointer = _GGDKDraw_GetPointer(((GGDKWindow)gw)->display);
    GdkWindow *window = gdk_device_get_window_at_position(pointer, NULL, NULL);

    if (window != NULL) {
        return (GWindow)g_object_get_data(G_OBJECT(window), "GGDKWindow");
    }
    return NULL;
}

static void GGDKDrawSetCursor(GWindow w, GCursor gcursor) {
    Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    GdkCursor *cursor = NULL;

    switch (gcursor) {
        case ct_default:
        case ct_backpointer:
        case ct_pointer:
            cursor = gdk_cursor_new_from_name(gw->display->display, "default");
            break;
        case ct_hand:
            cursor = gdk_cursor_new_from_name(gw->display->display, "hand");
            break;
        case ct_question:
            cursor = gdk_cursor_new_from_name(gw->display->display, "help");
            break;
        case ct_cross:
            cursor = gdk_cursor_new_from_name(gw->display->display, "crosshair");
            break;
        case ct_4way:
            cursor = gdk_cursor_new_from_name(gw->display->display, "move");
            break;
        case ct_text:
            cursor = gdk_cursor_new_from_name(gw->display->display, "text");
            break;
        case ct_watch:
            cursor = gdk_cursor_new_from_name(gw->display->display, "wait");
            break;
        case ct_draganddrop:
            cursor = gdk_cursor_new_from_name(gw->display->display, "pointer");
            break;
        case ct_invisible:
            return; // There is no *good* reason to make the cursor invisible
            break;
        default:
            Log(LOGDEBUG, "CUSTOM CURSOR! %d", gcursor);
    }

    if (gcursor >= ct_user) {
        GGDKDisplay *gdisp = gw->display;
        gcursor -= ct_user;
        if (gcursor < gdisp->cursors->len && gdisp->cursors->pdata[gcursor] != NULL) {
            gdk_window_set_cursor(gw->w, (GdkCursor *)gdisp->cursors->pdata[gcursor]);
            gw->current_cursor = gcursor;
        } else {
            Log(LOGWARN, "Invalid cursor value passed: %d", gcursor);
        }
    } else {
        gdk_window_set_cursor(gw->w, cursor);
        gw->current_cursor = gcursor;
        if (cursor != NULL) {
            g_object_unref(cursor);
        }
    }
}

static GCursor GGDKDrawGetCursor(GWindow gw) {
    Log(LOGDEBUG, " ");
    return ((GGDKWindow)gw)->current_cursor;
}

static GWindow GGDKDrawGetRedirectWindow(GDisplay *UNUSED(gdisp)) {
    //Log(LOGDEBUG, " ");
    // Not implemented.
    return NULL;
}

static void GGDKDrawTranslateCoordinates(GWindow from, GWindow to, GPoint *pt) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gfrom = (GGDKWindow)from, gto = (GGDKWindow)to;
    GGDKDisplay *gdisp = gfrom->display;


    if (gto == gdisp->groot) {
        // The actual meaning of this command...
        int x, y;
        gdk_window_get_root_coords(gfrom->w, pt->x, pt->y, &x, &y);
        pt->x = x;
        pt->y = y;
    } else {
        GdkPoint from_root, to_root;
        gdk_window_get_origin(gfrom->w, &from_root.x, &from_root.y);
        gdk_window_get_origin(gto->w, &to_root.x, &to_root.y);
        pt->x = from_root.x - to_root.x  + pt->x;
        pt->y = from_root.y - to_root.y + pt->y;
    }
}


static void GGDKDrawBeep(GDisplay *gdisp) {
    //Log(LOGDEBUG, " ");
    gdk_display_beep(((GGDKDisplay *)gdisp)->display);
}

static void GGDKDrawScroll(GWindow w, GRect *rect, int32 hor, int32 vert) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow) w;
    GRect temp;

    vert = -vert;
    if (rect == NULL) {
        temp.x = temp.y = 0;
        temp.width = gw->pos.width;
        temp.height = gw->pos.height;
        rect = &temp;
    }

    GDrawRequestExpose(w, rect, false);
}


static GIC *GGDKDrawCreateInputContext(GWindow UNUSED(gw), enum gic_style UNUSED(style)) {
    Log(LOGDEBUG, " ");
    return NULL;
}

static void GGDKDrawSetGIC(GWindow UNUSED(gw), GIC *UNUSED(gic), int UNUSED(x), int UNUSED(y)) {
    Log(LOGDEBUG, " ");
}

static int GGDKDrawKeyState(GWindow w, int keysym) {
    //Log(LOGDEBUG, " ");
    if (keysym != ' ') {
        Log(LOGWARN, "Cannot check state of unsupported character!");
        return 0;
    }
    // Since this function is only used to check the state of the space button
    // Dont't bother with a full implementation...
    return ((GGDKWindow)w)->display->is_space_pressed;
}

static void GGDKDrawGrabSelection(GWindow w, enum selnames sn) {
    Log(LOGDEBUG, " ");

    if ((int)sn < 0 || sn >= sn_max) {
        return;
    }

    GGDKWindow gw = (GGDKWindow)w;
    GGDKDisplay *gdisp = gw->display;
    GGDKSelectionInfo *sel = &gdisp->selinfo[sn];

    // Send a SelectionClear event to the current owner of the selection
    if (sel->owner != NULL && sel->datalist != NULL) {
        GEvent e = {0};
        e.w = (GWindow)sel->owner;
        e.type = et_selclear;
        e.u.selclear.sel = sn;
        e.native_window = sel->owner->w;
        if (sel->owner->eh != NULL) {
            _GGDKDraw_CallEHChecked(sel->owner, &e, sel->owner->eh);
        }
    }
    _GGDKDraw_ClearSelData(gdisp, sn);

    gdk_selection_owner_set(gw->w, sel->sel_atom, gdisp->last_event_time, false);
    sel->owner = gw;
    sel->timestamp = gdisp->last_event_time;
}

static void GGDKDrawAddSelectionType(GWindow w, enum selnames sel, char *type, void *data, int32 cnt, int32 unitsize,
                                     void *gendata(void *, int32 *len), void freedata(void *)) {
    Log(LOGDEBUG, " ");

    GGDKWindow gw = (GGDKWindow)w;
    GGDKDisplay *gdisp = gw->display;
    GdkAtom type_atom = gdk_atom_intern(type, false);
    GGDKSelectionData *sd = NULL;
    GList_Glib *dl = gdisp->selinfo[sel].datalist;

    if (unitsize != 1 && unitsize != 2 && unitsize != 4) {
        GDrawIError("Bad unitsize to GGDKDrawAddSelectionType");
        return;
    } else if ((int)sel < 0 || sel >= sn_max) {
        GDrawIError("Bad selname value");
        return;
    }

    while (dl != NULL && (sd = ((GGDKSelectionData *)dl->data))->type_atom != type_atom) {
        dl = dl->next;
    }

    if (dl == NULL) {
        sd = calloc(1, sizeof(GGDKSelectionData));
        sd->type_atom = type_atom;
        gdisp->selinfo[sel].datalist = g_list_append(gdisp->selinfo[sel].datalist, sd);
    }

    sd->cnt = cnt;
    sd->data = data;
    sd->unit_size = unitsize;
    sd->gendata = gendata;
    sd->freedata = freedata;

#ifdef GDK_WINDOWING_QUARTZ
    if (sel == sn_clipboard && type_atom == gdk_atom_intern_static_string("UTF8_STRING")) {
        char *data = sd->data;
        if (sd->gendata) {
            int len;
            data = sd->gendata(sd->data, &len);
        }
        _GGDKDrawCocoa_SetClipboardText(data);
        if (sd->gendata) {
            free(data);
        }
    }
#endif
}

static void *GGDKDrawRequestSelection(GWindow w, enum selnames sn, char *typename, int32 *len) {
    GGDKWindow gw = (GGDKWindow)w;
    GGDKDisplay *gdisp = gw->display;
    GdkAtom type_atom = gdk_atom_intern(typename, false);
    void *ret = NULL;

    if (len != NULL) {
        *len = 0;
    }

    if ((int)sn < 0 || sn >= sn_max || gw->is_waiting_for_selection || gw->is_dying) {
        return NULL;
    }

#ifdef GDK_WINDOWING_QUARTZ
    if (sn == sn_clipboard && type_atom == gdk_atom_intern_static_string("UTF8_STRING")) {
        ret = _GGDKDrawCocoa_GetClipboardText();
        if (ret && len) {
            *len = strlen(ret);
        }
        return ret;
    }
#endif

    // If we own the selection, get the data ourselves...
    if (gdisp->selinfo[sn].owner != NULL) {
        GList_Glib *ptr = gdisp->selinfo[sn].datalist;
        while (ptr != NULL) {
            GGDKSelectionData *sd = (GGDKSelectionData *)ptr->data;
            if (sd->type_atom == type_atom) {
                if (sd->gendata != NULL) {
                    ret = (sd->gendata)(sd->data, len);
                    if (len != NULL) {
                        *len *= sd->unit_size;
                    }
                } else {
                    int sz = sd->unit_size * sd->cnt;
                    ret = calloc(sz + 4, 1);
                    if (ret) {
                        memcpy(ret, sd->data, sz);
                        if (len != NULL) {
                            *len = sz;
                        }
                    }
                }
                return ret;
            }
            ptr = ptr->next;
        }
    }

#ifndef GDK_WINDOWING_QUARTZ
    // Otherwise we have to ask the owner for the data.
    gdk_selection_convert(gw->w, gdisp->selinfo[sn].sel_atom, type_atom, gdisp->last_event_time);

    // On Windows, gdk_selection_convert is synchronous. Avoid the fluff of waiting
    // which also can cause segfault if a window gets destroyed in the meantime...
#ifdef GDK_WINDOWING_X11
    gw->is_waiting_for_selection = true;
    gw->is_notified_of_selection = false;

    GTimer_GTK *timer = g_timer_new();
    g_timer_start(timer);

    while (gw->is_waiting_for_selection) {
        if (g_timer_elapsed(timer, NULL) >= (double)gdisp->sel_notify_timeout) {
            break;
        }
        GGDKDrawProcessPendingEvents((GDisplay *)gdisp);
    }
    g_timer_destroy(timer);

    if (gw->is_notified_of_selection) {
#endif
        guchar *data;
        GdkAtom received_type;
        gint received_format;
        gint rlen = gdk_selection_property_get(gw->w, &data, &received_type, &received_format);
        if (data != NULL) {
            ret = calloc(rlen + 4, 1);
            if (ret) {
                memcpy(ret, data, rlen);
                if (len) {
                    *len = rlen;
                }
            }
            g_free(data);
        }
#ifdef GDK_WINDOWING_X11
    }

    gw->is_waiting_for_selection = false;
    gw->is_notified_of_selection = false;
#endif // GDK_WINDOWING_X11
#endif // GDK_WINDOWING_QUARTZ

    return ret;
}

static int GGDKDrawSelectionHasType(GWindow w, enum selnames sn, char *typename) {
    Log(LOGDEBUG, " ");

    GGDKWindow gw = (GGDKWindow)w;
    if (gw->is_dying || gw->is_waiting_for_selection || (int)sn < 0 || sn >= sn_max) {
        return false;
    }

    GGDKDisplay *gdisp = gw->display;
    GdkAtom sel_type = gdk_atom_intern(typename, false);

    // Check if we own it
    if (gdisp->selinfo[sn].owner != NULL) {
        GList_Glib *ptr = gdisp->selinfo[sn].datalist;
        while (ptr != NULL) {
            if (((GGDKSelectionData *)ptr->data)->type_atom == sel_type) {
                return true;
            }
            ptr = ptr->next;
        }
        return false;
    }

#ifndef GDK_WINDOWING_QUARTZ
    // Else query
    if (gdisp->seltypes.timestamp != gdisp->last_event_time || gdisp->seltypes.sel_atom != gdisp->selinfo[sn].sel_atom) {
        free(gdisp->seltypes.types);
        gdisp->seltypes.types = NULL;
        gdisp->seltypes.sel_atom = gdisp->selinfo[sn].sel_atom;
        gdisp->seltypes.types = (GdkAtom *) GGDKDrawRequestSelection(w, sn, "TARGETS", &gdisp->seltypes.len);
        gdisp->seltypes.len /= sizeof(GdkAtom *);
    }

    if (gdisp->seltypes.types != NULL) {
        for (int i = 0; i < gdisp->seltypes.len; i++) {
            if (gdisp->seltypes.types[i] == sel_type) {
                return true;
            }
        }
    }
    return false;
#else
    return sel_type == gdk_atom_intern_static_string("UTF8_STRING");
#endif
}

static void GGDKDrawBindSelection(GDisplay *disp, enum selnames sn, char *atomname) {
    Log(LOGDEBUG, " ");
    GGDKDisplay *gdisp = (GGDKDisplay *) disp;
    if ((int)sn >= 0 && sn < sn_max) {
        gdisp->selinfo[sn].sel_atom = gdk_atom_intern(atomname, false);
    }
}

static int GGDKDrawSelectionHasOwner(GDisplay *disp, enum selnames sn) {
    Log(LOGDEBUG, " ");

    if ((int)sn < 0 || sn >= sn_max) {
        return false;
    }

    GGDKDisplay *gdisp = (GGDKDisplay *)disp;
    // Check if we own it
    if (gdisp->selinfo[sn].owner != NULL) {
        return true;
    }

    // Else query. Note, we cannot use gdk_selection_owner_get, as this only works for
    // windows that GDK created. This code is untested.
    void *data = GGDKDrawRequestSelection((GWindow)gdisp->groot, sn, "TIMESTAMP", NULL);
    if (data != NULL) {
        free(data);
        return true;
    }
    return false;
}

static void GGDKDrawPointerUngrab(GDisplay *gdisp) {
    Log(LOGDEBUG, " ");
#ifndef GGDKDRAW_GDK_3_20
    GdkDevice *pointer = _GGDKDraw_GetPointer((GGDKDisplay *)gdisp);
    if (pointer == NULL) {
        return;
    }

    gdk_device_ungrab(pointer, GDK_CURRENT_TIME);
#else
    GdkSeat *seat = gdk_display_get_default_seat(((GGDKDisplay *)gdisp)->display);
    if (seat == NULL) {
        return;
    }

    gdk_seat_ungrab(seat);
#endif
}

static void GGDKDrawPointerGrab(GWindow w) {
    Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
#ifndef GGDKDRAW_GDK_3_20
    GdkDevice *pointer = _GGDKDraw_GetPointer(gw->display);
    if (pointer == NULL) {
        return;
    }

    gdk_device_grab(pointer, gw->w,
                    GDK_OWNERSHIP_NONE,
                    false,
                    GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_SCROLL_MASK,
                    NULL, GDK_CURRENT_TIME);
#else
    GdkSeat *seat = gdk_display_get_default_seat(gw->display->display);
    if (seat == NULL) {
        return;
    }

    gdk_seat_grab(seat, gw->w,
                  GDK_SEAT_CAPABILITY_ALL_POINTING,
                  false, NULL, NULL, NULL, NULL);
#endif
}

static void GGDKDrawRequestExpose(GWindow w, GRect *rect, int UNUSED(doclear)) {
    Log(LOGDEBUG, "%p [%s]", w, ((GGDKWindow)w)->window_title);

    GGDKWindow gw = (GGDKWindow) w;
    GdkRectangle clip;

    if (!gw->is_visible || _GGDKDraw_WindowOrParentsDying(gw)) {
        return;
    }
    if (rect == NULL) {
        clip.x = clip.y = 0;
        clip.width = gw->pos.width;
        clip.height = gw->pos.height;
    } else {
        clip.x = rect->x;
        clip.y = rect->y;
        clip.width = rect->width;
        clip.height = rect->height;

        if (rect->x < 0 || rect->y < 0 || rect->x + rect->width > gw->pos.width ||
                rect->y + rect->height > gw->pos.height) {

            if (clip.x < 0) {
                clip.width += clip.x;
                clip.x = 0;
            }
            if (clip.y < 0) {
                clip.height += clip.y;
                clip.y = 0;
            }
            if (clip.x + clip.width > gw->pos.width) {
                clip.width = gw->pos.width - clip.x;
            }
            if (clip.y + clip.height > gw->pos.height) {
                clip.height = gw->pos.height - clip.y;
            }
            if (clip.height <= 0 || clip.width <= 0) {
                return;
            }
        }
    }

    gdk_window_invalidate_rect(gw->w, &clip, false);
}

static void GGDKDrawForceUpdate(GWindow gw) {
    //Log(LOGDEBUG, " ");
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    gdk_window_process_updates(((GGDKWindow)gw)->w, true);
G_GNUC_END_IGNORE_DEPRECATIONS
}

static void GGDKDrawSync(GDisplay *gdisp) {
    //Log(LOGDEBUG, " ");
    gdk_display_sync(((GGDKDisplay *)gdisp)->display);
}

static void GGDKDrawSkipMouseMoveEvents(GWindow UNUSED(gw), GEvent *UNUSED(gevent)) {
    //Log(LOGDEBUG, " ");
    // Not implemented, not needed.
}

static void GGDKDrawProcessPendingEvents(GDisplay *gdisp) {
    //Log(LOGDEBUG, " ");
    GMainContext *ctx = g_main_loop_get_context(((GGDKDisplay *)gdisp)->main_loop);
    if (ctx != NULL) {
        _GGDKDraw_CleanupAutoPaint((GGDKDisplay *)gdisp);
        while (g_main_context_iteration(ctx, false));
    }
}

static void GGDKDrawProcessWindowEvents(GWindow w) {
    Log(LOGWARN, "This function SHOULD NOT BE CALLED! Will likely not do as expected! Window: %p", w);

    if (w != NULL)  {
        GGDKDrawProcessPendingEvents(w->display);
    }
}

static void GGDKDrawProcessOneEvent(GDisplay *gdisp) {
    //Log(LOGDEBUG, " ");
    GMainContext *ctx = g_main_loop_get_context(((GGDKDisplay *)gdisp)->main_loop);
    if (ctx != NULL) {
        _GGDKDraw_CleanupAutoPaint((GGDKDisplay *)gdisp);
        g_main_context_iteration(ctx, true);
    }
}

static void GGDKDrawEventLoop(GDisplay *gdisp) {
    Log(LOGDEBUG, " ");
    GMainContext *ctx = g_main_loop_get_context(((GGDKDisplay *)gdisp)->main_loop);
    if (ctx != NULL) {
        _GGDKDraw_CleanupAutoPaint((GGDKDisplay *)gdisp);
        do {
            while (((GGDKDisplay *)gdisp)->top_window_count > 0) {
                g_main_context_iteration(ctx, true);
                if ((gdisp->err_flag) && (gdisp->err_report)) {
                    GDrawIErrorRun("%s", gdisp->err_report);
                }
                if (gdisp->err_report) {
                    free(gdisp->err_report);
                    gdisp->err_report = NULL;
                }
            }
            GGDKDrawSync(gdisp);
            GGDKDrawProcessPendingEvents(gdisp);
            GGDKDrawSync(gdisp);
        } while (((GGDKDisplay *)gdisp)->top_window_count > 0 || g_main_context_pending(ctx));
    }
}

static void GGDKDrawPostEvent(GEvent *e) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)(e->w);
    e->native_window = gw->w;
    _GGDKDraw_CallEHChecked(gw, e, gw->eh);
}

static void GGDKDrawPostDragEvent(GWindow w, GEvent *mouse, enum event_type et) {
    Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w, cw;
    GGDKDisplay *gdisp = gw->display;
    int x, y;

    // If the cursor hasn't moved much, don't bother to send a drag event
    x = abs(mouse->u.mouse.x - gdisp->last_dd.x);
    y = abs(mouse->u.mouse.y - gdisp->last_dd.y);
    if (x + y < 4 && et == et_drag) {
        return;
    }

    cw = (GGDKWindow)GGDKDrawGetPointerWindow(w);

    if (gdisp->last_dd.w != cw) {
        if (gdisp->last_dd.w != NULL) {
            GEvent e = {0};
            e.type = et_dragout;
            e.u.drag_drop.x = gdisp->last_dd.rx;
            e.u.drag_drop.y = gdisp->last_dd.ry;
            e.w = (GWindow)gdisp->last_dd.w;
            e.native_window = e.w->native_window;

            _GGDKDraw_CallEHChecked(gdisp->last_dd.w, &e, gdisp->last_dd.w->eh);
        } else {
            /* Send foreign dragout message */
        }
        gdisp->last_dd.w = NULL;
    }

    GEvent e = {0};
    // Are we still within the original window?
    if (cw == gw) {
        e.type = et;
        e.w = w;
        e.native_window = w->native_window;
        x = e.u.drag_drop.x = mouse->u.mouse.x;
        y = e.u.drag_drop.y = mouse->u.mouse.y;
        _GGDKDraw_CallEHChecked(gw, &e, gw->eh);
    } else if (cw != NULL) {
        GPoint pt = {.x = mouse->u.mouse.x, .y = mouse->u.mouse.y};
        GGDKDrawTranslateCoordinates(w, (GWindow)cw, &pt);
        x = pt.x;
        y = pt.y;

        e.type = et;
        e.u.drag_drop.x = x;
        e.u.drag_drop.y = y;
        e.w = (GWindow)cw;
        e.native_window = cw->w;
        _GGDKDraw_CallEHChecked(cw, &e, cw->eh);
    } else {
        // Foreign window
    }

    if (et != et_drop) {
        gdisp->last_dd.w = cw;
        gdisp->last_dd.x = mouse->u.mouse.x;
        gdisp->last_dd.y = mouse->u.mouse.y;
        gdisp->last_dd.rx = x;
        gdisp->last_dd.ry = y;
    } else {
        gdisp->last_dd.w = NULL;
    }
}

static int GGDKDrawRequestDeviceEvents(GWindow w, int devcnt, struct gdeveventmask *de) {
    Log(LOGDEBUG, " ");
    return 0; //Not sure how to handle... For tablets...
}

static GTimer *GGDKDrawRequestTimer(GWindow w, int32 time_from_now, int32 frequency, void *userdata) {
    //Log(LOGDEBUG, " ");
    GGDKTimer *timer = calloc(1, sizeof(GGDKTimer));
    GGDKWindow gw = (GGDKWindow)w;
    if (timer == NULL)  {
        return NULL;
    }

    gw->display->timers = g_list_append(gw->display->timers, timer);

    timer->owner = w;
    timer->repeat_time = frequency;
    timer->userdata = userdata;
    timer->active = true;
    timer->has_differing_repeat_time = (time_from_now != frequency);
    timer->glib_timeout_id = g_timeout_add(time_from_now, _GGDKDraw_ProcessTimerEvent, timer);

    GGDKDRAW_ADDREF(timer);
    return (GTimer *)timer;
}

static void GGDKDrawCancelTimer(GTimer *timer) {
    //Log(LOGDEBUG, " ");
    GGDKTimer *gtimer = (GGDKTimer *)timer;
    gtimer->active = false;
    GGDKDRAW_DECREF(gtimer, _GGDKDraw_OnTimerDestroyed);
}

// Our function VTable
static struct displayfuncs gdkfuncs = {
    GGDKDrawInit,

    GGDKDrawSetDefaultIcon,

    GGDKDrawCreateTopWindow,
    GGDKDrawCreateSubWindow,
    GGDKDrawCreatePixmap,
    GGDKDrawCreateBitmap,
    GGDKDrawCreateCursor,
    GGDKDrawDestroyWindow,
    GGDKDrawDestroyCursor,
    GGDKDrawNativeWindowExists, //Not sure what this is meant to do...
    GGDKDrawSetZoom,
    GGDKDrawSetWindowBackground,
    GGDKDrawSetDither,

    GGDKDrawSetVisible,
    GGDKDrawMove,
    GGDKDrawTrueMove,
    GGDKDrawResize,
    GGDKDrawMoveResize,
    GGDKDrawRaise,
    GGDKDrawRaiseAbove,
    GGDKDrawIsAbove,
    GGDKDrawLower,
    GGDKDrawSetWindowTitles,
    GGDKDrawSetWindowTitles8,
    GGDKDrawGetWindowTitle,
    GGDKDrawGetWindowTitle8,
    GGDKDrawSetTransientFor,
    GGDKDrawGetPointerPosition,
    GGDKDrawGetPointerWindow,
    GGDKDrawSetCursor,
    GGDKDrawGetCursor,
    GGDKDrawGetRedirectWindow,
    GGDKDrawTranslateCoordinates,

    GGDKDrawBeep,

    GGDKDrawPushClip,
    GGDKDrawPopClip,

    GGDKDrawSetDifferenceMode,

    GGDKDrawClear,
    GGDKDrawDrawLine,
    GGDKDrawDrawArrow,
    GGDKDrawDrawRect,
    GGDKDrawFillRect,
    GGDKDrawFillRoundRect,
    GGDKDrawDrawEllipse,
    GGDKDrawFillEllipse,
    GGDKDrawDrawArc,
    GGDKDrawDrawPoly,
    GGDKDrawFillPoly,
    GGDKDrawScroll,

    GGDKDrawDrawImage,
    GGDKDrawDrawGlyph,
    GGDKDrawDrawImageMagnified,
    GGDKDrawDrawPixmap,

    GGDKDrawCreateInputContext,
    GGDKDrawSetGIC,
    GGDKDrawKeyState,

    GGDKDrawGrabSelection,
    GGDKDrawAddSelectionType,
    GGDKDrawRequestSelection,
    GGDKDrawSelectionHasType,
    GGDKDrawBindSelection,
    GGDKDrawSelectionHasOwner,

    GGDKDrawPointerUngrab,
    GGDKDrawPointerGrab,
    GGDKDrawRequestExpose,
    GGDKDrawForceUpdate,
    GGDKDrawSync,
    GGDKDrawSkipMouseMoveEvents,
    GGDKDrawProcessPendingEvents,
    GGDKDrawProcessWindowEvents,
    GGDKDrawProcessOneEvent,
    GGDKDrawEventLoop,
    GGDKDrawPostEvent,
    GGDKDrawPostDragEvent,
    GGDKDrawRequestDeviceEvents,

    GGDKDrawRequestTimer,
    GGDKDrawCancelTimer,

    GGDKDrawGetFontMetrics,

    GGDKDrawHasCairo,
    GGDKDrawPathStartNew,
    GGDKDrawPathClose,
    GGDKDrawPathMoveTo,
    GGDKDrawPathLineTo,
    GGDKDrawPathCurveTo,
    GGDKDrawPathStroke,
    GGDKDrawPathFill,
    GGDKDrawPathFillAndStroke, // Currently unused

    GGDKDrawLayoutInit,
    GGDKDrawLayoutDraw,
    GGDKDrawLayoutIndexToPos,
    GGDKDrawLayoutXYToIndex,
    GGDKDrawLayoutExtents,
    GGDKDrawLayoutSetWidth,
    GGDKDrawLayoutLineCount,
    GGDKDrawLayoutLineStart,
    GGDKDrawStartNewSubPath,
    GGDKDrawFillRuleSetWinding,

    GGDKDrawDoText8,

    GGDKDrawPushClipOnly,
    GGDKDrawClipPreserve
};

// Protected member functions (package-level)

GDisplay *_GGDKDraw_CreateDisplay(char *displayname, char *UNUSED(programname)) {
    GGDKDisplay *gdisp;
    GdkDisplay *display;
    GGDKWindow groot;

    LogInit();

    if (displayname == NULL) {
        display = gdk_display_get_default();
    } else {
        display = gdk_display_open(displayname);
    }

    if (display == NULL) {
        return NULL;
    }

    gdisp = (GGDKDisplay *)calloc(1, sizeof(GGDKDisplay));
    if (gdisp == NULL) {
        return NULL;
    }

    // cursors.c creates ~41.
    gdisp->cursors = g_ptr_array_sized_new(50);
    gdisp->windows = g_hash_table_new(g_direct_hash, g_direct_equal);
    gdisp->mru_windows = g_queue_new();
    if (gdisp->windows == NULL || gdisp->mru_windows == NULL) {
        if (gdisp->windows) {
            g_hash_table_destroy(gdisp->windows);
        }
        free(gdisp);
        return NULL;
    }
    gdisp->transient_stack = g_ptr_array_sized_new(5);

    gdisp->funcs = &gdkfuncs;
    gdisp->display = display;
    gdisp->screen = gdk_display_get_default_screen(display);
    gdisp->root = gdk_screen_get_root_window(gdisp->screen);
    gdisp->res = gdk_screen_get_resolution(gdisp->screen);
    gdisp->pangoc_context = gdk_pango_context_get_for_screen(gdisp->screen);
    if (gdisp->res <= 0) {
        gdisp->res = 96;
    }
#ifdef GDK_WINDOWING_QUARTZ
    if (gdisp->res <= 72) {
        gdisp->res = 96;
    }
#endif

    gdisp->main_loop = g_main_loop_new(NULL, true);
    gdisp->bs.double_time = 200;
    gdisp->bs.double_wiggle = 3;
    gdisp->sel_notify_timeout = 2; // 2 second timeout

    //Initialise selection atoms
    gdisp->selinfo[sn_primary].sel_atom = gdk_atom_intern_static_string("PRIMARY");
    gdisp->selinfo[sn_clipboard].sel_atom = gdk_atom_intern_static_string("CLIPBOARD");
    gdisp->selinfo[sn_drag_and_drop].sel_atom = gdk_atom_intern_static_string("DRAG_AND_DROP");
    gdisp->selinfo[sn_user1].sel_atom = gdk_atom_intern_static_string("PRIMARY");
    gdisp->selinfo[sn_user2].sel_atom = gdk_atom_intern_static_string("PRIMARY");

    bool tbf = false, mxc = false;
    int user_res = 0;
    GResStruct res[] = {
        {.resname = "MultiClickTime", .type = rt_int, .val = &gdisp->bs.double_time},
        {.resname = "MultiClickWiggle", .type = rt_int, .val = &gdisp->bs.double_wiggle},
        {.resname = "SelectionNotifyTimeout", .type = rt_int, .val = &gdisp->sel_notify_timeout},
        {.resname = "TwoButtonFixup", .type = rt_bool, .val = &tbf},
        {.resname = "MacOSXCmd", .type = rt_bool, .val = &mxc},
        {.resname = "ScreenResolution", .type = rt_int, .val = &user_res},
        {.resname = NULL},
    };
    GResourceFind(res, NULL);
    gdisp->twobmouse_win = tbf;
    gdisp->macosx_cmd = mxc;

    // Now finalise the resolution
    if (user_res > 0) {
        gdisp->res = user_res;
    }
    pango_cairo_context_set_resolution(gdisp->pangoc_context, gdisp->res);

    groot = (GGDKWindow)calloc(1, sizeof(struct ggdkwindow));
    if (groot == NULL) {
        g_object_unref(gdisp->pangoc_context);
        g_queue_free(gdisp->mru_windows);
        g_hash_table_destroy(gdisp->windows);
        free(gdisp);
        return NULL;
    }

    gdisp->groot = groot;
    groot->ggc = _GGDKDraw_NewGGC();
    groot->display = gdisp;
    groot->w = gdisp->root;
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    groot->pos.width = gdk_screen_get_width(gdisp->screen);
    groot->pos.height = gdk_screen_get_height(gdisp->screen);
G_GNUC_END_IGNORE_DEPRECATIONS
    groot->is_toplevel = true;
    groot->is_visible = true;
    g_object_set_data(G_OBJECT(gdisp->root), "GGDKWindow", groot);

    gdisp->def_background = GResourceFindColor("Background", COLOR_CREATE(0xf5, 0xff, 0xfa));
    gdisp->def_foreground = GResourceFindColor("Foreground", COLOR_CREATE(0x00, 0x00, 0x00));
    if (GResourceFindBool("Synchronize", false)) {
        gdk_display_sync(gdisp->display);
    }

    (gdisp->funcs->init)((GDisplay *) gdisp);

    gdk_event_handler_set(_GGDKDraw_DispatchEvent, (gpointer)gdisp, NULL);

    _GDraw_InitError((GDisplay *) gdisp);

    //DEBUG
    if (getenv("GGDK_DEBUG")) {
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        gdk_window_set_debug_updates(true);
G_GNUC_END_IGNORE_DEPRECATIONS
    }
    return (GDisplay *)gdisp;
}

void _GGDKDraw_DestroyDisplay(GDisplay *disp) {
    GGDKDisplay *gdisp = (GGDKDisplay *)(disp);

    // Indicate we're dying...
    gdisp->is_dying = true;

    // Destroy remaining windows
    if (g_hash_table_size(gdisp->windows) > 0) {
        Log(LOGINFO, "Windows left allocated - forcibly freeing!");
        while (g_hash_table_size(gdisp->windows) > 0) {
            GHashTableIter iter;
            GGDKWindow gw;
            g_hash_table_iter_init(&iter, gdisp->windows);
            if (g_hash_table_iter_next(&iter, (void **)&gw, NULL)) {
                Log(LOGINFO, "Forcibly destroying window (%p:%s)", gw, gw->window_title);
                gw->reference_count = 2;
                GGDKDrawDestroyWindow((GWindow)gw);
                _GGDKDraw_OnWindowDestroyed(gw);
            }
        }
    }

    // Destroy root window
    _GGDKDraw_OnWindowDestroyed(gdisp->groot);
    gdisp->groot = NULL;

    // Destroy the mru window list
    g_queue_free(gdisp->mru_windows);
    gdisp->mru_windows = NULL;

    // Finally destroy the window table
    g_hash_table_destroy(gdisp->windows);
    gdisp->windows = NULL;

    g_ptr_array_free(gdisp->transient_stack, true);

    // Destroy held selection types, if any
    free(gdisp->seltypes.types);
    gdisp->seltypes.types = NULL;

    // Destroy cursors
    for (guint i = 0; i < gdisp->cursors->len; i++) {
        if (gdisp->cursors->pdata[i] != NULL) {
            GGDKDrawDestroyCursor((GDisplay *)gdisp, (GCursor)(ct_user + i));
        }
    }
    g_ptr_array_free(gdisp->cursors, true);
    gdisp->cursors = NULL;

    // Destroy any orphaned timers (???)
    if (gdisp->timers != NULL) {
        Log(LOGWARN, "Orphaned timers present - forcibly freeing!");
        while (gdisp->timers != NULL) {
            GGDKTimer *timer = (GGDKTimer *)gdisp->timers->data;
            timer->reference_count = 1;
            GGDKDrawCancelTimer((GTimer *)timer);
        }
    }

    // Get rid of our pango context
    g_object_unref(gdisp->pangoc_context);
    gdisp->pangoc_context = NULL;

    // Destroy the fontstate
    free(gdisp->fontstate);
    gdisp->fontstate = NULL;

    // Close the display
    if (gdisp->display != NULL) {
        gdisp->display = NULL;
    }

    // Unreference the main loop
    g_main_loop_unref(gdisp->main_loop);
    gdisp->main_loop = NULL;

    // Free the data structure
    free(gdisp);
    return;
}

#endif // FONTFORGE_CAN_USE_GDK
