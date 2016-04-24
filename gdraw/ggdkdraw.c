/**
*  \file  ggdkdraw.c
*  \brief GDK drawing backend.
*/

#include "ggdkdrawP.h"

#ifdef FONTFORGE_CAN_USE_GDK
#include "gresource.h"
#include "ustring.h"
#include <assert.h>
#include <string.h>
#include <math.h>

// Private member functions (file-level)

static GGC *_GGDKDraw_NewGGC() {
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

static gboolean _GGDKDraw_OnWindowDestroyed(gpointer data) {
    GGDKWindow gw = (GGDKWindow)data;
    gw->is_cleaning_up = true; // We're in the process of destroying it.

    Log(LOGDEBUG, "OnWindowDestroyed!");
    if (gw->cc != NULL) {
        cairo_destroy(gw->cc);
        gw->cc = NULL;
    }

    if (!gw->is_pixmap) {
        while (gw->transient_childs->len > 0) {
            GGDKWindow tw = (GGDKWindow)g_ptr_array_remove_index_fast(gw->transient_childs, gw->transient_childs->len - 1);
            gdk_window_set_transient_for(tw->w, gw->display->groot->w);
            tw->transient_owner = NULL;
            tw->istransient = false;
        }

        if (!gdk_window_is_destroyed(gw->w)) {
            gdk_window_destroy(gw->w);
            // Wait for it to die
            while (!gdk_window_is_destroyed(gw->w)) {
                GDrawProcessPendingEvents((GDisplay *)gw->display);
            }
        }

        // Signal that it has been destroyed
        struct gevent die = {0};
        die.w = (GWindow)gw;
        die.native_window = gw->w;
        die.type = et_destroy;
        GDrawPostEvent(&die);

        // Remove all relevant timers that haven't been cleaned up by the user
        // Note: We do not free the GTimer struct as the user may then call DestroyTimer themselves...
        GList_Glib *ent = gw->display->timers;
        while (ent != NULL) {
            GList_Glib *next = ent->next;
            GGDKTimer *timer = (GGDKTimer *)ent->data;
            if (timer->owner == (GWindow)gw) {
                //Since we update the timer list ourselves, don't all GDrawCancelTimer.
                Log(LOGDEBUG, "WARNING: Unstopped timer on window destroy!!! %x -> %x", gw, timer);
                timer->active = false;
                timer->stopped = true;
                g_source_remove(timer->glib_timeout_id);
                gw->display->timers = g_list_delete_link(gw->display->timers, ent);
            }
            ent = next;
        }

        // Decrement the toplevel window count
        if (gw->display->groot == gw->parent && !gw->is_dlg) {
            gw->display->top_window_count--;
        }

        Log(LOGDEBUG, "Window destroyed: %p[%p][%s][%d]", gw, gw->w, gw->window_title, gw->is_toplevel);
        free(gw->window_title);
        g_ptr_array_free(gw->transient_childs, false);
        // Unreference our reference to the window
        g_object_unref(G_OBJECT(gw->w));
    }

    g_object_unref(gw->pango_layout);

    if (gw->cs != NULL) {
        cairo_surface_destroy(gw->cs);
    }

    free(gw->ggc);
    free(gw);
    return false;
}

// FF expects the destroy call to happen asynchronously to
// the actual GDrawDestroyWindow call. So we add it to the queue...
static void _GGDKDraw_InitiateWindowDestroy(GGDKWindow gw) {
    if (gw->is_pixmap) {
        _GGDKDraw_OnWindowDestroyed(gw);
    } else if (!gw->is_cleaning_up) { // Check for nested call - if we're already being destroyed.
        g_timeout_add(10, _GGDKDraw_OnWindowDestroyed, gw);
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

static void _GGDKDraw_CallEHChecked(GGDKWindow gw, GEvent *event, int (*eh)(GWindow gw, GEvent *)) {
    if (eh) {
        // Increment reference counter
        GGDKDRAW_ADDREF(gw);
        (eh)((GWindow)gw, event);

        // Cleanup after our user...
        GPtrArray *dirty = gw->display->dirty_windows;
        while (dirty->len > 0) {
            GGDKWindow dw = (GGDKWindow)g_ptr_array_remove_index_fast(dirty, dirty->len - 1);
            assert(dw != NULL);
            if (dw->cc != NULL && !dw->is_dying) {
                cairo_destroy(dw->cc);
                dw->cc = NULL;
            }
        }
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

    GdkDevice *pointer = gdk_seat_get_pointer(seat);
    if (pointer == NULL) {
        return NULL;
    }
#else
    GdkDeviceManager *manager = gdk_display_get_device_manager(gdisp->display);
    if (manager == NULL) {
        return NULL;
    }

    GdkDevice *pointer = gdk_device_manager_get_client_pointer(manager);
    if (pointer == NULL) {
        return NULL;
    }
#endif
    return pointer;
}

static void _GGDKDraw_CenterWindowOnScreen(GGDKWindow gw) {
    GGDKDisplay *gdisp = gw->display;
    GdkRectangle work_area, window_size;
    GdkDevice *pointer = _GGDKDraw_GetPointer(gdisp);
    GdkScreen *pointer_screen;
    int x, y, monitor = 0;

    gdk_window_get_frame_extents(gw->w, &window_size);
    gdk_device_get_position(pointer, &pointer_screen, &x, &y);
    if (pointer_screen == gdisp->screen) { // Ensure it's on the same screen
        monitor = gdk_screen_get_monitor_at_point(gdisp->screen, x, y);
    }

    gdk_screen_get_monitor_workarea(gdisp->screen, monitor, &work_area);
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

    nw->ggc = _GGDKDraw_NewGGC();
    if (nw->ggc == NULL) {
        Log(LOGDEBUG, "_GGDKDraw_CreateWindow: _GGDKDraw_NewGGC returned NULL");
        free(nw);
        return NULL;
    }

    nw->display = gdisp;
    nw->eh = eh;
    nw->parent = gw;
    nw->pos = *pos;
    nw->user_data = user_data;

    // Window type
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

    // Window title and hints
    if (attribs.window_type != GDK_WINDOW_CHILD) {
        // Icon titles are ignored.
        if ((wattrs->mask & wam_utf8_wtitle) && (wattrs->utf8_window_title != NULL)) {
            nw->window_title = copy(wattrs->utf8_window_title);
        } else if ((wattrs->mask & wam_wtitle) && (wattrs->window_title != NULL)) {
            nw->window_title = u2utf8_copy(wattrs->window_title);
        }

        attribs.title = nw->window_title;
        attribs.type_hint = nw->is_popup ? GDK_WINDOW_TYPE_HINT_UTILITY : GDK_WINDOW_TYPE_HINT_NORMAL;

        if (attribs.title != NULL) {
            attribs_mask |= GDK_WA_TITLE;
        }
        attribs_mask |= GDK_WA_TYPE_HINT;
    }

    // Event mask
    attribs.event_mask = GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK;
    if (attribs.window_type != GDK_WINDOW_CHILD) {
        attribs.event_mask |= GDK_FOCUS_CHANGE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK;
    }
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

    attribs.x = pos->x;
    attribs.y = pos->y;
    attribs.width = pos->width;
    attribs.height = pos->height;
    attribs_mask |= GDK_WA_X | GDK_WA_Y;

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
    if (attribs.window_type == GDK_WINDOW_TOPLEVEL) {
        nw->is_centered = true;
        _GGDKDraw_CenterWindowOnScreen(nw);
    }

    // Set background
    if (!(wattrs->mask & wam_backcol) || wattrs->background_color == COLOR_DEFAULT) {
        wattrs->background_color = gdisp->def_background;
    }
    nw->ggc->bg = wattrs->background_color;

    GdkRGBA col = {
        .red = COLOR_RED(wattrs->background_color) / 255.,
        .green = COLOR_GREEN(wattrs->background_color) / 255.,
        .blue = COLOR_BLUE(wattrs->background_color) / 255.,
        .alpha = 1.
    };
    gdk_window_set_background_rgba(nw->w, &col);

    if (attribs.window_type != GDK_WINDOW_CHILD) {
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
        GdkWindowHints hints = GDK_HINT_BASE_SIZE;
        if ((wattrs->mask & wam_noresize) && wattrs->noresize) {
            hints |= GDK_HINT_MIN_SIZE;
        }

        // Hmm does this seem right?
        geom.base_width = geom.min_width = geom.max_width = pos->width;
        geom.base_height = geom.min_height = geom.max_height = pos->height;

        hints |= GDK_HINT_POS;
        nw->was_positioned = true;

        gdk_window_set_geometry_hints(nw->w, &geom, hints);

        if ((wattrs->mask & wam_transient) && wattrs->transient != NULL) {
            GDrawSetTransientFor((GWindow)nw, wattrs->transient);
            nw->is_dlg = true;
        } else if (!nw->is_dlg) {
            ++gdisp->top_window_count;
        } else if (nw->restrict_input_to_me && gdisp->last_nontransient_window != NULL) {
            GDrawSetTransientFor((GWindow)nw, (GWindow) - 1);
        }
        nw->isverytransient = (wattrs->mask & wam_verytransient) ? 1 : 0;
        nw->is_toplevel = true;
    }

    // Establish Pango/Cairo context
    if (!_GGDKDraw_InitPangoCairo(nw)) {
        gdk_window_destroy(nw->w);
        free(nw->window_title);
        free(nw->ggc);
        free(nw);
        return NULL;
    }

    if ((wattrs->mask & wam_cursor) && wattrs->cursor != ct_default) {
        GDrawSetCursor((GWindow)nw, wattrs->cursor);
    }

    // TODO: Add error checking?
    nw->transient_childs = g_ptr_array_sized_new(5);

    // Add a reference to our own structure.
    GGDKDRAW_ADDREF(nw);

    // Event handler
    if (eh != NULL) {
        GEvent e = {0};
        e.type = et_create;
        e.w = (GWindow) nw;
        e.native_window = (void *)(intpt) nw->w;
        _GGDKDraw_CallEHChecked(nw, &e, eh);
    }

    // This should not be here. Debugging purposes only.
    // gdk_window_show(nw->w);

    // Set the user data to the GWindow
    // Although there is gdk_window_set_user_data, if non-NULL,
    // GTK assumes it's a GTKWindow.
    g_object_set_data(G_OBJECT(nw->w), "GGDKWindow", nw);
    // Add our reference to the window
    // This will be unreferenced when the window is destroyed.
    g_object_ref(G_OBJECT(nw->w));
    //gdk_window_move_resize(nw->w, nw->pos.x, nw->pos.y, nw->pos.width, nw->pos.height);
    Log(LOGDEBUG, "Window created: %p[%p][%s][%d]", nw, nw->w, nw->window_title, nw->is_toplevel);
    return (GWindow)nw;
}

static GWindow _GGDKDraw_NewPixmap(GDisplay *gdisp, uint16 width, uint16 height, cairo_format_t format,
                                   unsigned char *data) {
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
    gw->ggc->bg = ((GGDKDisplay *) gdisp)->def_background;
    width &= 0x7fff; // We're always using a cairo surface...

    gw->display = (GGDKDisplay *) gdisp;
    gw->is_pixmap = 1;
    gw->parent = NULL;
    gw->pos.x = gw->pos.y = 0;
    gw->pos.width = width;
    gw->pos.height = height;

    if (data == NULL) {
        gw->cs = cairo_image_surface_create(format, width, height);
    } else {
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
    if (mask * GDK_HYPER_MASK) {
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
            return vs_unobscured;
            break;
    }
    return vs_unobscured;
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
    GGDKDisplay *gdisp = gw->display;
    GGDKWindow gww = gw;

    switch (event->type) {
        case GDK_KEY_PRESS:
        case GDK_KEY_RELEASE:
        case GDK_BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
        case GDK_SCROLL:
        case GDK_MOTION_NOTIFY:
        case GDK_DELETE:
            break;
        default:
            return false;
            break;
    }

    while (gww != gw->display->groot) {
        if (gww->transient_childs->len > 0) {
            bool has_modal = false;
            for (int i = 0; i < gww->transient_childs->len; i++) {
                GGDKWindow ow = (GGDKWindow)gww->transient_childs->pdata[i];
                if (ow->restrict_input_to_me) {
                    has_modal = true;
                    break;
                }
            }
            if (has_modal) {
                break;
            }
        }
        gww = gww->parent;
    }

    if (gww == gw->display->groot) {
        return false;
    }

    if (event->type != GDK_MOTION_NOTIFY && event->type != GDK_BUTTON_RELEASE) {
        gdk_window_beep(gw->w);
        //GDrawBeep((GDisplay *)gdisp);
    }
    return true;
}

static gboolean _GGDKDraw_ProcessTimerEvent(gpointer user_data) {
    GGDKTimer *timer = (GGDKTimer *)user_data;
    GEvent e = {0};
    GGDKDisplay *gdisp;
    bool ret = true;

    if (!timer->active || _GGDKDraw_WindowOrParentsDying((GGDKWindow)timer->owner)) {
        timer->active = false;
        return false;
    }

    gdisp = ((GGDKWindow)timer->owner)->display;
    e.type = et_timer;
    e.w = timer->owner;
    e.native_window = timer->owner->native_window;
    e.u.timer.timer = (GTimer *)timer;
    e.u.timer.userdata = timer->userdata;

    GGDKDRAW_ADDREF(timer);
    _GGDKDraw_CallEHChecked((GGDKWindow)timer->owner, &e, timer->owner->eh);
    if (timer->active) {
        if (timer->repeat_time == 0) {
            GDrawCancelTimer((GTimer *)timer);
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

static gboolean _GGDKDraw_OnFakedResize(gpointer user_data) {
    GGDKWindow gw = (GGDKWindow)user_data;
    GdkEventConfigure evt = {0};

    evt.type       = GDK_CONFIGURE;
    evt.window     = gw->w;
    evt.send_event = true;
    evt.width      = gdk_window_get_width(gw->w);
    evt.height     = gdk_window_get_height(gw->w);
    gdk_window_get_position(gw->w, &evt.x, &evt.y);

    gdk_event_put((GdkEvent *)&evt);
    gw->resize_timeout = 0;
    return false;
}

// In their infinite wisdom, GDK does not send configure events for child windows.
static void _GGDKDraw_FakeConfigureEvent(GGDKWindow gw) {
    if (gw->resize_timeout != 0) {
        g_source_remove(gw->resize_timeout);
        gw->resize_timeout = 0;
    }
    gw->resize_timeout = g_timeout_add(150, _GGDKDraw_OnFakedResize, gw);
}

static void _GGDKDraw_DispatchEvent(GdkEvent *event, gpointer data) {
    static int request_id = 0;
    struct gevent gevent = {0};
    GGDKDisplay *gdisp = (GGDKDisplay *)data;
    GdkWindow *w = gdk_event_get_window(event);
    GGDKWindow gw;
    guint32 event_time = gdk_event_get_time(event);
    if (event_time != GDK_CURRENT_TIME) {
        gdisp->last_event_time = event_time;
    }

    Log(LOGDEBUG, "[%d] Received event %d(%s) %x", request_id, event->type, GdkEventName(event->type), w);
    fflush(stderr);

    if (w == NULL) {
        return;
    } else if ((gw = g_object_get_data(G_OBJECT(w), "GGDKWindow")) == NULL) {
        Log(LOGDEBUG, "MISSING GW!");
        //assert(false);
        return;
    } else if (_GGDKDraw_WindowOrParentsDying(gw) || gdk_window_is_destroyed(w)) {
        Log(LOGDEBUG, "DYING! %x", w);
        return;
    } else if (_GGDKDraw_FilterByModal(event, gw)) {
        Log(LOGDEBUG, "Discarding event - has modals!");
        return;
    }

    gevent.w = (GWindow)gw;
    gevent.native_window = (void *)gw->w;
    gevent.type = et_noevent;
    if (event->type == GDK_KEY_PRESS || event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE) {
        if (gw->transient_owner != NULL && gw->isverytransient && !gw->transient_owner->is_dying) {
            gdisp->last_nontransient_window = gw->transient_owner;
        } else {
            gdisp->last_nontransient_window = gw;
        }
    }

    switch (event->type) {
        case GDK_KEY_PRESS:
        case GDK_KEY_RELEASE: {
            GdkEventKey *key = (GdkEventKey *)event;
            gevent.type = event->type == GDK_KEY_PRESS ? et_char : et_charup;
            gevent.u.chr.state = _GGDKDraw_GdkModifierToKsm(((GdkEventKey *)event)->state);
            gevent.u.chr.autorepeat =
                event->type    == GDK_KEY_PRESS &&
                gdisp->ks.type == GDK_KEY_PRESS &&
                key->keyval    == gdisp->ks.keyval &&
                key->state     == gdisp->ks.state;
            // Mumble mumble Mac
            //if ((event->xkey.state & ksm_option) && gdisp->macosx_cmd) {
            //    gevent.u.chr.state |= ksm_meta;
            //}

            if (key->keyval == GDK_KEY_space) {
                gw->display->is_space_pressed = event->type == GDK_KEY_PRESS;
            }

            gevent.u.chr.keysym = key->keyval;
            gevent.u.chr.chars[0] = gdk_keyval_to_unicode(key->keyval);

            if (gevent.u.chr.chars[0] == '\0') {
                gevent.u.chr.chars[1] = '\0';
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
                GDrawPostEvent(&gevent);
                gevent.type = et_mouseup;
            }
        }
        break;
        case GDK_BUTTON_PRESS:
        case GDK_BUTTON_RELEASE: {
            GdkEventButton *evt = (GdkEventButton *)event;
            /*if ((redirect = InputRedirection(gdisp->input, gw)) != NULL) {
                if (event->type == ButtonPress) {
                    GXDrawBeep((GDisplay *) gdisp);
                }
                return;
            }*/
            gevent.u.mouse.state = _GGDKDraw_GdkModifierToKsm(evt->state);
            gevent.u.mouse.x = evt->x;
            gevent.u.mouse.y = evt->y;
            gevent.u.mouse.button = evt->button;

            if (event->type == GDK_BUTTON_PRESS) {
                int xdiff, ydiff;
                gevent.type = et_mousedown;

                xdiff = abs(evt->x - gdisp->bs.release_x);
                ydiff = abs(evt->y - gdisp->bs.release_y);

                if (xdiff + ydiff < gdisp->bs.double_wiggle &&
                        gw == gdisp->bs.release_w &&
                        evt->button == gdisp->bs.release_button &&
                        gevent.u.mouse.time - gdisp->bs.release_time < gdisp->bs.double_time &&
                        gevent.u.mouse.time >= gdisp->bs.release_time) {	// Time can wrap

                    gdisp->bs.cur_click++;
                } else {
                    gdisp->bs.cur_click = 1;
                }
            } else {
                gevent.type = et_mouseup;
                gdisp->bs.release_time = gevent.u.mouse.time;
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

            gevent.type = et_expose;
            gevent.u.expose.rect.x = expose->area.x;
            gevent.u.expose.rect.y = expose->area.y;
            gevent.u.expose.rect.width = expose->area.width;
            gevent.u.expose.rect.height = expose->area.height;

            if (gw->cc != NULL) {
                cairo_destroy(gw->cc);
                gw->cc = NULL;
            }

            gdk_window_begin_paint_region(w, expose->region);

            // So if this was a requested expose (send_event), and this
            // is a child window, then mask out the expose region.
            // This is necessary because gdk_window_begin_paint_region does
            // nothing if it's a child window...
            if (!gw->is_toplevel && expose->send_event) {
                int nr = cairo_region_num_rectangles(expose->region);
                gw->cc = gdk_cairo_create(w);

                for (int i = 0; i < nr; i++) {
                    cairo_rectangle_int_t rect;
                    cairo_region_get_rectangle(expose->region, i, &rect);
                    cairo_rectangle(gw->cc, rect.x, rect.y, rect.width, rect.height);
                }
                cairo_clip(gw->cc);
            }
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
            }
            if (gevent.u.resize.dwidth != 0 || gevent.u.resize.dheight != 0) {
                gevent.u.resize.sized = true;
            }

            if (gw->is_toplevel && !gevent.u.resize.sized && gevent.u.resize.moved) {
                gevent.type = et_noevent;
            }

            gw->pos = gevent.u.resize.size;
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
            GDrawDestroyWindow((GWindow)gw);
            break;
        case GDK_DELETE:
            gevent.type = et_close;
            break;
        case GDK_SELECTION_CLEAR: /*{
        int i;
        gevent.type = et_selclear;
        gevent.u.selclear.sel = sn_primary;
        for (i = 0; i < sn_max; ++i) {
            if (event->xselectionclear.selection == gdisp->selinfo[i].sel_atom) {
                gevent.u.selclear.sel = i;
                break;
            }
        }
        GXDrawClearSelData(gdisp, gevent.u.selclear.sel);
    }*/
            break;
        case GDK_SELECTION_REQUEST:
            /*
                GXDrawTransmitSelection(gdisp, event);*/
            break;
        case GDK_SELECTION_NOTIFY: // paste
            break;
        case GDK_PROPERTY_NOTIFY:
            break;
        default:
            Log(LOGDEBUG, "UNPROCESSED GDK EVENT %d", event->type);
            break;
    }

    if (gevent.type != et_noevent && gw != NULL && gw->eh != NULL) {
        _GGDKDraw_CallEHChecked(gw, &gevent, gw->eh);
        if (gevent.type == et_expose) {
            gdk_window_end_paint(w);
            if (gw->cc != NULL) {
                cairo_destroy(gw->cc);
                gw->cc = NULL;
            }
        }
    }
    Log(LOGDEBUG, "[%d] Finished processing %d(%s)", request_id++, event->type, GdkEventName(event->type));
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

//static void GGDKDrawTerm(GDisplay *gdisp){}
//static void* GGDKDrawNativeDisplay(GDisplay *gdisp){}

static void GGDKDrawSetDefaultIcon(GWindow icon) {
    GGDKWindow gicon = (GGDKWindow)icon;
    if (gicon->is_pixmap) {
        gicon->display->default_icon = gicon;
    }
}

static GWindow GGDKDrawCreateTopWindow(GDisplay *gdisp, GRect *pos, int (*eh)(GWindow gw, GEvent *), void *user_data,
                                       GWindowAttrs *gattrs) {
    Log(LOGDEBUG, "");
    return _GGDKDraw_CreateWindow((GGDKDisplay *) gdisp, NULL, pos, eh, user_data, gattrs);
}

static GWindow GGDKDrawCreateSubWindow(GWindow gw, GRect *pos, int (*eh)(GWindow gw, GEvent *), void *user_data,
                                       GWindowAttrs *gattrs) {
    Log(LOGDEBUG, "");
    return _GGDKDraw_CreateWindow(((GGDKWindow) gw)->display, (GGDKWindow) gw, pos, eh, user_data, gattrs);
}

static GWindow GGDKDrawCreatePixmap(GDisplay *gdisp, uint16 width, uint16 height) {
    Log(LOGDEBUG, "");

    //TODO: Check format?
    return _GGDKDraw_NewPixmap(gdisp, width, height, CAIRO_FORMAT_ARGB32, NULL);
}

static GWindow GGDKDrawCreateBitmap(GDisplay *gdisp, uint16 width, uint16 height, uint8 *data) {
    Log(LOGDEBUG, "");
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A1, width);
    int actual = (width & 0x7fff) / 8;

    if (actual != stride) {
        GWindow ret = _GGDKDraw_NewPixmap(gdisp, width, height, CAIRO_FORMAT_A1, NULL);
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

    return _GGDKDraw_NewPixmap(gdisp, width, height, CAIRO_FORMAT_A1, data);
}

static GCursor GGDKDrawCreateCursor(GWindow src, GWindow mask, Color fg, Color bg, int16 x, int16 y) {
    Log(LOGDEBUG, "");

    GGDKDisplay *gdisp = (GGDKDisplay *)(src->display);
    GdkDisplay *display = gdisp->display;
    cairo_surface_t *cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, src->pos.width, src->pos.height);
    cairo_t *cc = cairo_create(cs);

    // Masking
    //Background
    cairo_set_source_rgb(cc, COLOR_RED(bg) / 255., COLOR_GREEN(bg) / 255., COLOR_BLUE(bg) / 255.);
    cairo_mask_surface(cc, ((GGDKWindow)mask)->cs, 0, 0);
    //Foreground
    cairo_set_source_rgb(cc, COLOR_RED(fg) / 255., COLOR_GREEN(fg) / 255., COLOR_BLUE(fg) / 255.);
    cairo_mask_surface(cc, ((GGDKWindow)src)->cs, 0, 0);

    GdkCursor *cursor = gdk_cursor_new_from_surface(display, cs, x, y);
    cairo_destroy(cc);
    cairo_surface_destroy(cs);

    g_ptr_array_add(gdisp->cursors, cursor);
    return ct_user + (gdisp->cursors->len - 1);
}

static void GGDKDrawDestroyWindow(GWindow w) {
    Log(LOGDEBUG, "");
    GGDKWindow gw = (GGDKWindow) w;

    if (gw->is_dying) {
        return;
    }

    gw->is_dying = true;
    if (!gw->is_pixmap) {
        GList_Glib *list = gdk_window_get_children(gw->w);
        GList_Glib *child = list;
        while (child != NULL) {
            GWindow c = g_object_get_data(child->data, "GGDKWindow");
            assert(c != NULL);
            GGDKDrawDestroyWindow(c);
            child = child->next;
        }
        g_list_free(list);

        if (gw->display->last_nontransient_window == gw) {
            gw->display->last_nontransient_window = NULL;
        }
        GDrawSetTransientFor(w, NULL);
        // Ensure an invalid value is returned if someone tries to get this value again.
        g_object_set_data(G_OBJECT(gw->w), "GGDKWindow", NULL);
    }
    GGDKDRAW_DECREF(gw, _GGDKDraw_InitiateWindowDestroy);
}

static void GGDKDrawDestroyCursor(GDisplay *gdisp, GCursor gcursor) {
    Log(LOGDEBUG, "");
    assert(false);
    // Well. No code calls GDrawDestroyCursor.
}

static int GGDKDrawNativeWindowExists(GDisplay *gdisp, void *native_window) {
    Log(LOGDEBUG, ""); //assert(false);
    GdkWindow *w = (GdkWindow *)native_window;
    GGDKWindow gw = g_object_get_data(G_OBJECT(w), "GGDKWindow");

    // So if the window is dying, the gdk window is already gone.
    // But gcontainer.c expects this to return true on et_destroy...
    if (gw == NULL || gw->is_dying) {
        return true;
    } else {
        return !gdk_window_is_destroyed(w) && gdk_window_is_visible(w);
    }
}

static void GGDKDrawSetZoom(GWindow gw, GRect *size, enum gzoom_flags flags) {
    Log(LOGDEBUG, "");
    //assert(false);
}

// Not possible?
static void GGDKDrawSetWindowBorder(GWindow gw, int width, Color gcol) {
    Log(LOGDEBUG, ""); //assert(false);
}

static void GGDKDrawSetWindowBackground(GWindow gw, Color gcol) {
    Log(LOGDEBUG, ""); //assert(false);
    GdkRGBA col = {
        .red = COLOR_RED(gcol) / 255.,
        .green = COLOR_GREEN(gcol) / 255.,
        .blue = COLOR_BLUE(gcol) / 255.,
        .alpha = 1.
    };
    gdk_window_set_background_rgba(((GGDKWindow)gw)->w, &col);
}

// How about NO
static int GGDKDrawSetDither(GDisplay *gdisp, int set) {
    Log(LOGDEBUG, ""); //assert(false);
    return false;
}


static void GGDKDrawReparentWindow(GWindow child, GWindow newparent, int x, int y) {
    Log(LOGDEBUG, "");
    GGDKWindow gchild = (GGDKWindow)child, gparent = (GGDKWindow)newparent;
    gchild->parent = gparent;
    gchild->is_toplevel = gchild->display->groot == gparent;
    gdk_window_reparent(gchild->w, gparent->w, x, y);
    // Hack to position it correctly on Windows
    // https://bugzilla.gnome.org/show_bug.cgi?id=765100
    gdk_window_move(gchild->w, x, y);
}

static void GGDKDrawSetVisible(GWindow w, int show) {
    Log(LOGDEBUG, ""); //assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    if (show) {
        gdk_window_show(gw->w);
    } else {
        GDrawSetTransientFor((GWindow)gw, NULL);
        gdk_window_hide(gw->w);
    }
}

static void GGDKDrawMove(GWindow gw, int32 x, int32 y) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_window_move(((GGDKWindow)gw)->w, x, y);
    if (!gw->is_toplevel) {
        _GGDKDraw_FakeConfigureEvent((GGDKWindow)gw);
    }
    ((GGDKWindow)gw)->is_centered = false;
}

static void GGDKDrawTrueMove(GWindow w, int32 x, int32 y) {
    Log(LOGDEBUG, "");
    GGDKDrawMove(w, x, y);
}

static void GGDKDrawResize(GWindow gw, int32 w, int32 h) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_window_resize(((GGDKWindow)gw)->w, w, h);
    if (gw->is_toplevel && ((GGDKWindow)gw)->is_centered) {
        _GGDKDraw_CenterWindowOnScreen((GGDKWindow)gw);
    }
    if (!gw->is_toplevel) {
        _GGDKDraw_FakeConfigureEvent((GGDKWindow)gw);
    }
}

static void GGDKDrawMoveResize(GWindow gw, int32 x, int32 y, int32 w, int32 h) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_window_move_resize(((GGDKWindow)gw)->w, x, y, w, h);
    if (!gw->is_toplevel) {
        _GGDKDraw_FakeConfigureEvent((GGDKWindow)gw);
    }
}

static void GGDKDrawRaise(GWindow w) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_window_raise(((GGDKWindow)w)->w);
}

static void GGDKDrawRaiseAbove(GWindow gw1, GWindow gw2) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_window_restack(((GGDKWindow)gw1)->w, ((GGDKWindow)gw2)->w, true);
}

// Only used once in gcontainer - force it to call GDrawRaiseAbove
static int GGDKDrawIsAbove(GWindow gw1, GWindow gw2) {
    Log(LOGDEBUG, ""); //assert(false);
    return false;
}

static void GGDKDrawLower(GWindow gw) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_window_lower(((GGDKWindow)gw)->w);
}

// Icon title is ignored.
static void GGDKDrawSetWindowTitles8(GWindow w, const char *title, const char *icontitle) {
    Log(LOGDEBUG, "");// assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    free(gw->window_title);
    gw->window_title = copy(title);

    if (title != NULL && gw->is_toplevel) {
        gdk_window_set_title(gw->w, title);
    }
}

static void GGDKDrawSetWindowTitles(GWindow gw, const unichar_t *title, const unichar_t *icontitle) {
    Log(LOGDEBUG, ""); //assert(false);
    char *str = u2utf8_copy(title);
    if (str != NULL) {
        GGDKDrawSetWindowTitles8(gw, str, NULL);
        free(str);
    }
}

// Sigh. GDK doesn't provide a way to get the window title...
static unichar_t *GGDKDrawGetWindowTitle(GWindow gw) {
    Log(LOGDEBUG, ""); // assert(false);
    return utf82u_copy(((GGDKWindow)gw)->window_title);
}

static char *GGDKDrawGetWindowTitle8(GWindow gw) {
    Log(LOGDEBUG, ""); //assert(false);
    return copy(((GGDKWindow)gw)->window_title);
}

static void GGDKDrawSetTransientFor(GWindow transient, GWindow owner) {
    Log(LOGDEBUG, ""); //assert(false);
    GGDKWindow gw = (GGDKWindow) transient, ow;
    GGDKDisplay *gdisp = gw->display;

    if (owner == (GWindow) - 1) {
        ow = gdisp->last_nontransient_window;
    } else if (owner == NULL) {
        ow = NULL; // Does this work with GDK?
    } else {
        ow = (GGDKWindow)owner;
    }

    if (gw->transient_owner != NULL) {
        g_ptr_array_remove_fast(gw->transient_owner->transient_childs, gw);
    }

    if (ow != NULL) {
        gdk_window_set_transient_for(gw->w, ow->w);
        gdk_window_set_modal_hint(gw->w, true);
        gw->istransient = true;

        g_ptr_array_add(ow->transient_childs, gw);
    } else {
        gdk_window_set_modal_hint(gw->w, false);
        gdk_window_set_transient_for(gw->w, gw->display->groot->w);
        gw->istransient = false;
    }

    gw->transient_owner = ow;
}

static void GGDKDrawGetPointerPosition(GWindow w, GEvent *ret) {
    Log(LOGDEBUG, ""); //assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    GdkDevice *pointer = _GGDKDraw_GetPointer(gw->display);
    if (pointer == NULL) {
        ret->u.mouse.x = 0;
        ret->u.mouse.y = 0;
        ret->u.mouse.state = 0;
        return;
    }

    int x, y;
    GdkModifierType mask;
    gdk_window_get_device_position(gw->w, pointer, &x, &y, &mask);
    ret->u.mouse.x = x;
    ret->u.mouse.y = y;
    ret->u.mouse.state = _GGDKDraw_GdkModifierToKsm(mask);
}

static GWindow GGDKDrawGetPointerWindow(GWindow gw) {
    Log(LOGDEBUG, "");  //assert(false);
    GdkDevice *pointer = _GGDKDraw_GetPointer(((GGDKWindow)gw)->display);
    GdkWindow *window = gdk_device_get_window_at_position(pointer, NULL, NULL);

    if (window != NULL) {
        return (GWindow)g_object_get_data(G_OBJECT(window), "GGDKWindow");
    }
    return NULL;
}

static void GGDKDrawSetCursor(GWindow w, GCursor gcursor) {
    Log(LOGDEBUG, ""); //assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    GdkCursor *cursor = NULL;

    switch (gcursor) {
        case ct_default:
        case ct_backpointer:
        case ct_pointer:
            cursor = gdk_cursor_new_from_name(gw->display->display, "default");
            break;
        //cursor = gdk_cursor_new_from_name(gw->display->display, "pointer");
        //break;
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
            cursor = gdk_cursor_new_from_name(gw->display->display, "context-menu");
            break;
        case ct_invisible:
            cursor = gdk_cursor_new_from_name(gw->display->display, "none");
            break;
        default:
            Log(LOGDEBUG, "CUSTOM CURSOR!");
    }

    if (gcursor >= ct_user) {
        GGDKDisplay *gdisp = gw->display;
        gcursor -= ct_user;
        if (gcursor < gdisp->cursors->len) {
            gdk_window_set_cursor(gw->w, (GdkCursor *)gdisp->cursors->pdata[gcursor]);
        }
    } else {
        gdk_window_set_cursor(gw->w, cursor);
        if (cursor != NULL) {
            g_object_unref(cursor);
        }
    }
}

static GCursor GGDKDrawGetCursor(GWindow gw) {
    Log(LOGDEBUG, "");
    //assert(false);
    return ct_default;
}

static GWindow GGDKDrawGetRedirectWindow(GDisplay *gdisp) {
    Log(LOGDEBUG, ""); //assert(false);
    return NULL;
    // Sigh... I don't know...
    /*
    GGDKDisplay *gdisp = (GGDKDisplay *) gd;
    if (gdisp->input == NULL) {
        return NULL;
    }
    return gdisp->input->cur_dlg;
    */
}

static void GGDKDrawTranslateCoordinates(GWindow from, GWindow to, GPoint *pt) {
    Log(LOGDEBUG, "");
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
    Log(LOGDEBUG, ""); //assert(false);
    gdk_display_beep(((GGDKDisplay *)gdisp)->display);
}

static void GGDKDrawFlush(GDisplay *gdisp) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_display_flush(((GGDKDisplay *)gdisp)->display);
}

static void GGDKDrawScroll(GWindow w, GRect *rect, int32 hor, int32 vert) {
    Log(LOGDEBUG, "");
    GGDKWindow gw = (GGDKWindow) w;
    GRect temp, old;

    vert = -vert;
    if (rect == NULL) {
        temp.x = temp.y = 0;
        temp.width = gw->pos.width;
        temp.height = gw->pos.height;
        rect = &temp;
    }

    GDrawRequestExpose(w, rect, false);
}


static GIC *GGDKDrawCreateInputContext(GWindow gw, enum gic_style style) {
    Log(LOGDEBUG, ""); //assert(false);
}

static void GGDKDrawSetGIC(GWindow gw, GIC *gic, int x, int y) {
    Log(LOGDEBUG, ""); //assert(false);
}

static int GGDKDrawKeyState(GWindow w, int keysym) {
    Log(LOGDEBUG, "");
    if (keysym != ' ') {
        Log(LOGWARN, "Cannot check state of unsupported character!");
        return 0;
    }
    // Since this function is only used to check the state of the space button
    // Dont't bother with a full implementation...
    return ((GGDKWindow)w)->display->is_space_pressed;
}

static void GGDKDrawGrabSelection(GWindow w, enum selnames sel) {
    Log(LOGDEBUG, ""); //assert(false);
}

static void GGDKDrawAddSelectionType(GWindow w, enum selnames sel, char *type, void *data, int32 cnt, int32 unitsize,
                                     void *gendata(void *, int32 *len), void freedata(void *)) {
    Log(LOGDEBUG, ""); //assert(false);
}

static void *GGDKDrawRequestSelection(GWindow w, enum selnames sn, char *typename, int32 *len) {
    Log(LOGDEBUG, ""); //assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    GdkAtom sel, type = gdk_atom_intern_static_string(typename), received_type = 0;
    guchar *data;
    gint received_format;

    switch (sn) {
        case sn_primary:
            sel = GDK_SELECTION_PRIMARY;
            break;
        case sn_clipboard:
            sel = GDK_SELECTION_CLIPBOARD;
            break;
        default:
            return NULL;
    }

#ifdef GDK_WINDOWING_WIN32
    sel = GDK_SELECTION_CLIPBOARD;
#endif

    gdk_selection_convert(gw->w, sel, type, gw->display->last_event_time);
    gdk_display_sync(gw->display->display);
    // This is broken.
    *len = gdk_selection_property_get(gw->w, &data, &received_type, &received_format);
    if (received_type == type) {
        char *ret = copy(data);
        g_free(data);
        return ret;
    }
    return NULL;
}

static int GGDKDrawSelectionHasType(GWindow w, enum selnames sn, char *typename) {
    Log(LOGDEBUG, ""); //assert(false);
    switch (sn) {
        case sn_primary:
        case sn_clipboard:
            if (!strcmp(typename, "UTF8_STRING")) {
                return true;
            }
    }
    return false;
}

static void GGDKDrawBindSelection(GDisplay *gdisp, enum selnames sn, char *atomname) {
    Log(LOGDEBUG, ""); //assert(false); //TODO: Implement selections (clipboard)
}

static int GGDKDrawSelectionHasOwner(GDisplay *gdisp, enum selnames sn) {
    Log(LOGDEBUG, "");
    assert(false);
}

static void GGDKDrawPointerUngrab(GDisplay *gdisp) {
    Log(LOGDEBUG, ""); //assert(false);
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
    Log(LOGDEBUG, ""); //assert(false);
    GGDKWindow gw = (GGDKWindow)w;
#ifndef GGDKDRAW_GDK_3_20
    GdkDevice *pointer = _GGDKDraw_GetPointer(gw->display);
    if (pointer == NULL) {
        return;
    }

    gdk_device_grab(pointer, gw->w,
                    GDK_OWNERSHIP_NONE,
                    false,
                    GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK,
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

static void GGDKDrawRequestExpose(GWindow w, GRect *rect, int doclear) {
    Log(LOGDEBUG, ""); // assert(false);

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

    if (!gw->is_toplevel) {
        //Eugh
        // So if you try to invalidate a child window,
        // GDK will also invalidate the parent window over the child window's area.
        // This causes noticable flickering. So synthesise the expose event ourselves...
        GdkEventExpose expose = {0};
        expose.type = GDK_EXPOSE;
        expose.window = gw->w;
        expose.send_event = true;
        expose.area.x = clip.x;
        expose.area.y = clip.y;
        expose.area.width = clip.width;
        expose.area.height = clip.height;
        expose.region = gdk_window_get_visible_region(gw->w);
        cairo_region_intersect_rectangle(expose.region, &expose.area);

        // Mask out child window areas
        GList_Glib *children = gdk_window_peek_children(gw->w);
        while (children != NULL) {
            cairo_region_t *chr = gdk_window_get_clip_region((GdkWindow *)children->data);
            int dx, dy;

            gdk_window_get_position((GdkWindow *)children->data, &dx, &dy);
            cairo_region_translate(chr, dx, dy);
            cairo_region_subtract(expose.region, chr);
            cairo_region_destroy(chr);
            children = children->next;
        }

        // Don't send unnecessarily...
        if (cairo_region_is_empty(expose.region)) {
            cairo_region_destroy(expose.region);
            return;
        }

        cairo_region_get_extents(expose.region, &expose.area);
        gdk_event_put((GdkEvent *)&expose);
        cairo_region_destroy(expose.region);
    } else {
        gdk_window_invalidate_rect(gw->w, &clip, false);
    }

}

static void GGDKDrawForceUpdate(GWindow gw) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_window_process_updates(((GGDKWindow)gw)->w, true);
}

static void GGDKDrawSync(GDisplay *gdisp) {
    Log(LOGDEBUG, ""); //assert(false)
    gdk_display_sync(((GGDKDisplay *)gdisp)->display);
}

static void GGDKDrawSkipMouseMoveEvents(GWindow gw, GEvent *gevent) {
    Log(LOGDEBUG, "");

    //assert(false);
}

static void GGDKDrawProcessPendingEvents(GDisplay *gdisp) {
    Log(LOGDEBUG, "");
    //assert(false);
    GMainContext *ctx = g_main_loop_get_context(((GGDKDisplay *)gdisp)->main_loop);
    if (ctx != NULL) {
        while (g_main_context_iteration(ctx, false));
    }
}

static void GGDKDrawProcessWindowEvents(GWindow gw) {
    Log(LOGDEBUG, ""); //assert(false);
    GGDKDrawProcessPendingEvents((GDisplay *)((GGDKWindow)gw)->display);
}

static void GGDKDrawProcessOneEvent(GDisplay *gdisp) {
    //Log(LOGDEBUG, ""); //assert(false);
    GMainContext *ctx = g_main_loop_get_context(((GGDKDisplay *)gdisp)->main_loop);
    if (ctx != NULL) {
        g_main_context_iteration(ctx, true);
    }
}

static void GGDKDrawEventLoop(GDisplay *gdisp) {
    Log(LOGDEBUG, "");
    //assert(false);
    GMainContext *ctx = g_main_loop_get_context(((GGDKDisplay *)gdisp)->main_loop);
    if (ctx != NULL) {
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
    Log(LOGDEBUG, ""); //assert(false);
    GGDKWindow gw = (GGDKWindow)(e->w);
    e->native_window = ((GWindow) gw)->native_window;
    _GGDKDraw_CallEHChecked(gw, e, gw->eh);
}

static void GGDKDrawPostDragEvent(GWindow w, GEvent *mouse, enum event_type et) {
    Log(LOGDEBUG, "");
    assert(false);
}

static int GGDKDrawRequestDeviceEvents(GWindow w, int devcnt, struct gdeveventmask *de) {
    Log(LOGDEBUG, ""); //assert(false);
    return 0; //Not sure how to handle... For tablets...
}

static GTimer *GGDKDrawRequestTimer(GWindow w, int32 time_from_now, int32 frequency, void *userdata) {
    Log(LOGDEBUG, ""); //assert(false);
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
    Log(LOGDEBUG, ""); //assert(false);
    GGDKTimer *gtimer = (GGDKTimer *)timer;
    gtimer->active = false;
    GGDKDRAW_DECREF(gtimer, _GGDKDraw_OnTimerDestroyed);
}


static void GGDKDrawSyncThread(GDisplay *gdisp, void (*func)(void *), void *data) {
    Log(LOGDEBUG, ""); //assert(false); // For some shitty gio impl. Ignore ignore ignore!
}


static GWindow GGDKDrawPrinterStartJob(GDisplay *gdisp, void *user_data, GPrinterAttrs *attrs) {
    Log(LOGDEBUG, "");
    assert(false);
}

static void GGDKDrawPrinterNextPage(GWindow w) {
    Log(LOGDEBUG, "");
    assert(false);
}

static int GGDKDrawPrinterEndJob(GWindow w, int cancel) {
    Log(LOGDEBUG, "");
    assert(false);
}


// Our function VTable
static struct displayfuncs gdkfuncs = {
    GGDKDrawInit,
    NULL, // GGDKDrawTerm,
    NULL, // GGDKDrawNativeDisplay,

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
    GGDKDrawSetWindowBorder,
    GGDKDrawSetWindowBackground,
    GGDKDrawSetDither,

    GGDKDrawReparentWindow,
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
    GGDKDrawFlush,

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
    GGDKDrawTileImage,
    GGDKDrawDrawGlyph,
    GGDKDrawDrawImageMagnified,
    GGDKDrawCopyScreenToImage,
    GGDKDrawDrawPixmap,
    GGDKDrawTilePixmap,

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

    GGDKDrawSyncThread,

    GGDKDrawPrinterStartJob,
    GGDKDrawPrinterNextPage,
    GGDKDrawPrinterEndJob,

    GGDKDrawGetFontMetrics,

    GGDKDrawHasCairo,
    GGDKDrawPathStartNew,
    GGDKDrawPathClose,
    GGDKDrawPathMoveTo,
    GGDKDrawPathLineTo,
    GGDKDrawPathCurveTo,
    GGDKDrawPathStroke,
    GGDKDrawPathFill,
    GGDKDrawPathFillAndStroke,

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

GDisplay *_GGDKDraw_CreateDisplay(char *displayname, char *programname) {
    GGDKDisplay *gdisp;
    GdkDisplay *display;
    GGDKWindow groot;

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
        gdk_display_close(display);
        return NULL;
    }

    // cursors.c creates ~41.
    gdisp->cursors = g_ptr_array_sized_new(50);
    // 15 Dirty windows/eh call?
    gdisp->dirty_windows = g_ptr_array_sized_new(15);
    if (gdisp->dirty_windows == NULL) {
        free(gdisp);
        gdk_display_close(display);
        return NULL;
    }

    gdisp->funcs = &gdkfuncs;
    gdisp->display = display;
    gdisp->screen = gdk_display_get_default_screen(display);
    gdisp->root = gdk_screen_get_root_window(gdisp->screen);
    gdisp->res = gdk_screen_get_resolution(gdisp->screen);
    if (gdisp->res <= 0) {
        Log(LOGWARN, "Unknown resolution, assuming 96DPI!");
        gdisp->res = 96;
    }
    gdisp->main_loop = g_main_loop_new(NULL, true);
    gdisp->scale_screen_by = 1; //Does nothing
    gdisp->bs.double_time = 200;
    gdisp->bs.double_wiggle = 3;

    bool tbf = false, mxc = false;
    GResStruct res[] = {
        {.resname = "MultiClickTime", .type = rt_int, .val = &gdisp->bs.double_time},
        {.resname = "MultiClickWiggle", .type = rt_int, .val = &gdisp->bs.double_wiggle},
        //{.resname = "SelectionNotifyTimeout", .type = rt_int, .val = &gdisp->SelNotifyTimeout},
        {.resname = "TwoButtonFixup", .type = rt_bool, .val = &tbf},
        {.resname = "MacOSXCmd", .type = rt_bool, .val = &mxc},
        NULL
    };
    GResourceFind(res, NULL);
    gdisp->twobmouse_win = tbf;
    gdisp->macosx_cmd = mxc;
    gdisp->pangoc_context = gdk_pango_context_get_for_screen(gdisp->screen);

    groot = (GGDKWindow)calloc(1, sizeof(struct ggdkwindow));
    if (groot == NULL) {
        g_object_unref(gdisp->pangoc_context);
        g_ptr_array_free(gdisp->dirty_windows, false);
        free(gdisp);
        gdk_display_close(display);
        return NULL;
    }

    gdisp->groot = groot;
    groot->ggc = _GGDKDraw_NewGGC();
    groot->display = gdisp;
    groot->w = gdisp->root;
    groot->pos.width = gdk_screen_get_width(gdisp->screen);
    groot->pos.height = gdk_screen_get_height(gdisp->screen);
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
        gdk_window_set_debug_updates(true);
    }
    return (GDisplay *)gdisp;
}

void _GGDKDraw_DestroyDisplay(GDisplay *gdisp) {
    GGDKDisplay *gdispc = (GGDKDisplay *)(gdisp);

    if (gdispc->groot != NULL) {
        if (gdispc->groot->ggc != NULL) {
            free(gdispc->groot->ggc);
            gdispc->groot->ggc = NULL;
        }
        free(gdispc->groot);
        gdispc->groot = NULL;
    }

    if (gdispc->display != NULL) {
        gdk_display_close(gdispc->display);
        gdispc->display = NULL;
    }
    return;
}

#endif // FONTFORGE_CAN_USE_GDK
