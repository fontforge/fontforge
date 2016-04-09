/**
*  \file  ggdkdraw.c
*  \brief GDK drawing backend.
*  \author Jeremy Tan
*  \license MIT
*/

#include "ggdkdrawP.h"

#ifdef FONTFORGE_CAN_USE_GDK
#include "gresource.h"
#include "ustring.h"
#include <assert.h>
#include <string.h>
#include <math.h>

static void GGDKDrawSetCursor(GWindow w, GCursor gcursor);
static void GGDKDrawTranslateCoordinates(GWindow from, GWindow to, GPoint *pt);

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

static void _GGDKDraw_CallEHChecked(GGDKWindow gw, GEvent *event, int (*eh)(GWindow gw, GEvent *)) {
    if (eh) {
        (eh)((GWindow)gw, event);
        if (gw->cc != NULL) {
            cairo_destroy(gw->cc);
            gw->cc = NULL;
        }
    }
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
    }
    if ((wattrs->mask & wam_isdlg) && wattrs->is_dlg) {
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
        attribs.type_hint = nw->is_popup ? GDK_WINDOW_TYPE_HINT_UTILITY :
                            nw->is_dlg ? GDK_WINDOW_TYPE_HINT_DIALOG : GDK_WINDOW_TYPE_HINT_NORMAL;

        if (attribs.title != NULL) {
            attribs_mask |= GDK_WA_TITLE;
        }
        attribs_mask |= GDK_WA_TYPE_HINT;
    }

    // Event mask
    attribs.event_mask = GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK;
    if (attribs.window_type == GDK_WINDOW_TOPLEVEL) {
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
        //if ((wattrs->event_masks & (1 << et_mouseup)) && (wattrs->event_masks & (1 << et_mousedown)))
        //    attribs.event_mask |= OwnerGrabButtonMask;
        if (wattrs->event_masks & (1 << et_visibility)) {
            attribs.event_mask |= GDK_VISIBILITY_NOTIFY_MASK;
        }
    }

    // Window position and size
    // I hate it placing stuff under the cursor...
    if (gw == gdisp->groot &&
            ((((wattrs->mask & wam_centered) && wattrs->centered)) ||
             ((wattrs->mask & wam_undercursor) && wattrs->undercursor))) {
        pos->x = (gdisp->groot->pos.width - pos->width) / 2;
        pos->y = (gdisp->groot->pos.height - pos->height) / 2;
        if (wattrs->centered == 2) {
            pos->y = (gdisp->groot->pos.height - pos->height) / 3;
        }
        nw->pos = *pos;
    }

    attribs.x = pos->x;
    attribs.y = pos->y;
    attribs.width = pos->width;
    attribs.height = pos->height;
    attribs_mask |= GDK_WA_X | GDK_WA_Y;

    // Window class
    attribs.wclass = GDK_INPUT_OUTPUT; // No hidden windows

    // Just use default GDK visual
    // attribs.visual = NULL;

    // Window type already set
    // attribs.window_type = ...;

    // Window cursor
    // Set below.

    // Window manager name and class
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

    // Decor
    if (nw->is_popup) {
        gdk_window_set_decorations(nw->w, 0);
        gdk_window_move_resize(nw->w, attribs.x, attribs.y, attribs.width, attribs.height);
    }

    if (attribs.window_type != GDK_WINDOW_CHILD) {
        // Set icon
        GGDKWindow icon = gdisp->default_icon;
        if (((wattrs->mask & wam_icon) && wattrs->icon != NULL) && ((GGDKWindow)wattrs->icon)->is_pixmap) {
            icon = (GGDKWindow) wattrs->icon;
        }
        if (icon != NULL) {
            GList_Glib *icon_list = NULL;
            GdkPixbuf *pb = gdk_pixbuf_get_from_surface(icon->cs, 0, 0, icon->pos.width, icon->pos.height);
            if (pb != NULL) {
                icon_list = g_list_append(icon_list, pb);
                gdk_window_set_icon_list(nw->w, icon_list);
                g_list_free(icon_list);
                g_object_unref(pb);
            }
        }

        GdkGeometry geom = {0};
        GdkWindowHints hints = GDK_HINT_BASE_SIZE;
        if ((wattrs->mask & wam_noresize) && wattrs->noresize) {
            hints |= GDK_HINT_MIN_SIZE;
        }

        // Hmm does this seem right?
        geom.base_width = geom.min_width = geom.max_width = pos->width;
        geom.base_height = geom.min_height = geom.max_height = pos->height;

        if (((wattrs->mask & wam_positioned) && wattrs->positioned) ||
                ((wattrs->mask & wam_centered) && wattrs->centered) ||
                ((wattrs->mask & wam_undercursor) && wattrs->undercursor)) {
            hints |= GDK_HINT_POS;
            nw->was_positioned = true;
        }

        gdk_window_set_geometry_hints(nw->w, &geom, hints);

        if (wattrs->mask & wam_restrict) {
            nw->restrict_input_to_me = wattrs->restrict_input_to_me;
        }
        if (wattrs->mask & wam_redirect) {
            nw->redirect_chars_to_me = wattrs->redirect_chars_to_me;
            nw->redirect_from = wattrs->redirect_from;
        }
        if ((wattrs->mask & wam_transient) && wattrs->transient != NULL) {
            gdk_window_set_transient_for(nw->w, ((GGDKWindow)(wattrs->transient))->w);
            nw->istransient = true;
            nw->transient_owner = ((GGDKWindow)(wattrs->transient))->w;
            nw->is_dlg = true;
        } else if (!nw->is_dlg) {
            ++gdisp->top_window_count;
        } else if (nw->restrict_input_to_me && gdisp->last_nontransient_window != NULL) {
            gdk_window_set_transient_for(nw->w, gdisp->last_nontransient_window);
            nw->transient_owner = gdisp->last_nontransient_window;
            nw->istransient = true;
        }
        nw->isverytransient = (wattrs->mask & wam_verytransient) ? 1 : 0;
        nw->is_toplevel = true;
    }

    // Establish Pango/Cairo context
    if (!_GGDKDraw_InitPangoCairo(gw)) {
        gdk_window_destroy(nw->w);
        free(nw->window_title);
        free(nw->ggc);
        free(nw);
        return NULL;
    }

    if ((wattrs->mask & wam_cursor) && wattrs->cursor != ct_default) {
        GGDKDrawSetCursor((GWindow)nw, wattrs->cursor);
    }

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
    // This will be unreferenced in _GGDKDraw_OnWindowDestroyed
    g_object_ref(G_OBJECT(nw->w));
    return (GWindow)nw;
}

static void _GGDKDraw_InitFonts(GGDKDisplay *gdisp) {
    FState *fs = calloc(1, sizeof(FState));
    if (fs == NULL) {
        Log(LOGDEBUG, "GGDKDraw: FState alloc failed!");
        assert(false);
    }

    /* In inches, because that's how fonts are measured */
    gdisp->fontstate = fs;
    fs->res = gdisp->res;
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
    return (GWindow)gw;
}

static gboolean _GGDKDraw_OnWindowDestroyed(gpointer data) {
    GGDKWindow gw = (GGDKWindow)data;

    Log(LOGDEBUG, "OnWindowDestroyed!");
    if (!gw->is_pixmap) {
        struct gevent die = {0};

        Log(LOGDEBUG, "Attempting clean of window!");
        // If the window's not destroyed yet, then try again...
        if (!gdk_window_is_destroyed(gw->w)) {
            Log(LOGDEBUG, "IT'S NOT DEAD JIM");
            return true;
        }

        // Send death threat
        die.w = (GWindow)gw;
        die.native_window = gw->w;
        die.type = et_destroy;
        GDrawPostEvent(&die);
        GDrawProcessPendingEvents((GDisplay *)(gw->display));

        // Remove all relevant timers that haven't been cleaned up by the user
        // This must come after the death threat because users may try to cancel the timers themselves.
        GList_Glib *ent = gw->display->timers;
        while (ent != NULL) {
            GList_Glib *next = ent->next;
            GGDKTimer *timer = (GGDKTimer *)ent->data;
            if (timer->owner == (GWindow)gw) {
                //Since we update the timer list ourselves, don't all GDrawCancelTimer.
                Log(LOGDEBUG, "WARNING: We're cleaning up after you!!! %x -> %x", gw, timer);
                timer->active = false;
                g_source_remove(timer->glib_timeout_id);
                gw->display->timers = g_list_delete_link(gw->display->timers, ent);
                free(timer);
            }
            ent = next;
        }

        if (gw->display->groot == gw->parent && !gw->is_dlg) {
            gw->display->top_window_count--;
        }

        // Ensure an invalid value is returned if someone tries to get this value again.
        g_object_set_data(G_OBJECT(gw->w), "GGDKWindow", NULL);
        free(gw->window_title);
        // Unreference our reference to the window
        g_object_unref(G_OBJECT(gw->w));
    }

    g_object_unref(gw->pango_layout);

    if (gw->cc != NULL) {
        cairo_destroy(gw->cc);
    }
    if (gw->cs != NULL) {
        cairo_surface_destroy(gw->cs);
    }
    free(gw->ggc);
    //ffs for some reason gdk doesn't pop events of dead windows.
    //this means we could end up with an event that is dead...
    //memset(gw, 0, sizeof(struct ggdkwindow));
    //free(gw);
    return false; // Don't repeat timeout
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

int _GGDKDraw_WindowOrParentsDying(GGDKWindow gw) {
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

static gboolean _GGDKDraw_ProcessTimerEvent(gpointer user_data) {
    GGDKTimer *timer = (GGDKTimer *)user_data;
    GEvent e = {0};
    GGDKDisplay *gdisp;

    if (!timer->active) {
        return false;
    }

    gdisp = ((GGDKWindow)timer->owner)->display;
    e.type = et_timer;
    e.w = timer->owner;
    e.native_window = timer->owner->native_window;
    e.u.timer.timer = (GTimer *)timer;
    e.u.timer.userdata = timer->userdata;
    _GGDKDraw_CallEHChecked((GGDKWindow)timer->owner, &e, timer->owner->eh);

    if (timer->repeat_time == 0) {
        gdisp->timers = g_list_remove(gdisp->timers, timer);
        return false;
    } else if (timer->has_differing_repeat_time) {
        timer->has_differing_repeat_time = false;
        timer->glib_timeout_id = g_timeout_add(timer->repeat_time, _GGDKDraw_ProcessTimerEvent, timer);
        return false;
    }

    return true;
}

static void _GGDKDraw_DispatchEvent(GdkEvent *event, gpointer data) {
    static int request_id = 0;
    struct gevent gevent = {0};
    GGDKDisplay *gdisp = (GGDKDisplay *)data;
    GdkWindow *w = gdk_event_get_window(event);
    GGDKWindow gw;

    Log(LOGDEBUG, "[%d] Received event %d(%s) %x", request_id, event->type, GdkEventName(event->type), w);
    fflush(stderr);

    if (w == NULL) {
        return;
    } else if ((gw = g_object_get_data(G_OBJECT(w), "GGDKWindow")) == NULL) {
        Log(LOGDEBUG, "MISSING GW!");
        //assert(false);
        return;
    } else if (_GGDKDraw_WindowOrParentsDying(gw) || gdk_window_is_destroyed(w)) {
        Log(LOGDEBUG, "DYING!");
        return;
    }

    gevent.w = (GWindow)gw;
    gevent.native_window = (void *)gw->w;
    gevent.type = -1;
    if (event->type == GDK_KEY_PRESS || event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE) {
        if (gw->transient_owner != NULL && gw->isverytransient) {
            gdisp->last_nontransient_window = gw->transient_owner;
        } else {
            gdisp->last_nontransient_window = gw->w;
        }
    }

    switch (event->type) {
        /*
        case GDK_KEY_PRESS:
        case GDK_KEY_RELEASE:
            gdisp->last_event_time = gdk_event_get_time(event);
            gevent.type = event->type == GDK_KEY_PRESS ? et_char : et_charup;
            gevent.u.chr.time = gdisp->last_event_time;
            gevent.u.chr.state = _GGDKDraw_GdkModifierToKsm(((GdkEventKey*)event)->state); //event->xkey.state;
            gevent.u.chr.autorepeat = 0;
            // Mumble mumble Mac
            //if ((event->xkey.state & ksm_option) && gdisp->macosx_cmd) {
            //    gevent.u.chr.state |= ksm_meta;
            //}

            //TODO: GDK doesn't send x/y pos when key was pressed...
            //gevent.u.chr.x = event->xkey.x;
            //gevent.u.chr.y = event->xkey.y;

            if ((redirect = InputRedirection(gdisp->input, gw)) == (GWindow)(-1)) {
                len = XLookupString((XKeyEvent *) event, charbuf, sizeof(charbuf), &keysym, &gdisp->buildingkeys);
                if (event->type == KeyPress && len != 0) {
                    GXDrawBeep((GDisplay *) gdisp);
                }
                return;
            } else if (redirect != NULL) {
                GPoint pt;
                gevent.w = redirect;
                pt.x = event->xkey.x;
                pt.y = event->xkey.y;
                GXDrawTranslateCoordinates(gw, redirect, &pt);
                gevent.u.chr.x = pt.x;
                gevent.u.chr.y = pt.y;
                gw = redirect;
            }
            */
        /*
        if (gevent.type == et_char) {
            // The state may be modified in the gevent where a mac command key
            //  entry gets converted to control, etc.
            if (((GGDKWindow) gw)->gic == NULL) {
                len = XLookupString((XKeyEvent *) event, charbuf, sizeof(charbuf), &keysym, &gdisp->buildingkeys);
                charbuf[len] = '\0';
                gevent.u.chr.keysym = keysym;
                def2u_strncpy(gevent.u.chr.chars, charbuf,
                              sizeof(gevent.u.chr.chars) / sizeof(gevent.u.chr.chars[0]));
            } else {
        #ifdef X_HAVE_UTF8_STRING
                // I think there's a bug in SCIM. If I leave the meta(alt/option) modifier
                //  bit set, then scim returns no keysym and no characters. On the other hand,
                //  if I don't leave that bit set, then the default input method on the mac
                //  will not do the Option key transformations properly. What I pass should
                //  be IM independent. So I don't think I should have to do the next line
                event->xkey.state &= ~Mod2Mask;
                // But I do
                len = Xutf8LookupString(((GGDKWindow) gw)->gic->ic, (XKeyPressedEvent *)event,
                                        charbuf, sizeof(charbuf), &keysym, &status);
                pt = charbuf;
                if (status == XBufferOverflow) {
                    pt = malloc(len + 1);
                    len = Xutf8LookupString(((GGDKWindow) gw)->gic->ic, (XKeyPressedEvent *)&event,
                                            pt, len, &keysym, &status);
                }
                if (status != XLookupChars && status != XLookupBoth) {
                    len = 0;
                }
                if (status != XLookupKeySym && status != XLookupBoth) {
                    keysym = 0;
                }
                pt[len] = '\0';
                gevent.u.chr.keysym = keysym;
                utf82u_strncpy(gevent.u.chr.chars, pt,
                               sizeof(gevent.u.chr.chars) / sizeof(gevent.u.chr.chars[0]));
                if (pt != charbuf) {
                    free(pt);
                }
        #else
                gevent.u.chr.keysym = keysym = 0;
                gevent.u.chr.chars[0] = 0;
        #endif
            }
            // Convert X11 keysym values to unicode
            if (keysym >= XKeysym_Mask) {
                keysym -= XKeysym_Mask;
            } else if (keysym <= XKEYSYM_TOP && keysym >= 0) {
                keysym = gdraw_xkeysym_2_unicode[keysym];
            }
            gevent.u.chr.keysym = keysym;
            if (keysym == gdisp->mykey_keysym &&
                    (event->xkey.state & (ControlMask | Mod1Mask)) == gdisp->mykey_mask) {
                gdisp->mykeybuild = !gdisp->mykeybuild;
                gdisp->mykey_state = 0;
                gevent.u.chr.chars[0] = '\0';
                gevent.u.chr.keysym = '\0';
                if (!gdisp->mykeybuild && _GDraw_BuildCharHook != NULL) {
                    (_GDraw_BuildCharHook)((GDisplay *) gdisp);
                }
            } else if (gdisp->mykeybuild) {
                _GDraw_ComposeChars((GDisplay *) gdisp, &gevent);
            }
        } else {
            // XLookupKeysym doesn't do shifts for us (or I don't know how to use the index arg to make it)
            len = XLookupString((XKeyEvent *) event, charbuf, sizeof(charbuf), &keysym, &gdisp->buildingkeys);
            gevent.u.chr.keysym = keysym;
            gevent.u.chr.chars[0] = '\0';
        }

        //
        // If we are a charup, but the very next XEvent is a chardown
        // on the same key, then we are just an autorepeat XEvent which
        // other code might like to ignore
        //
        if (gevent.type == et_charup && XEventsQueued(gdisp->display, QueuedAfterReading)) {
            XEvent nev;
            XPeekEvent(gdisp->display, &nev);
            if (nev.type == KeyPress && nev.xkey.time == event->xkey.time &&
                    nev.xkey.keycode == event->xkey.keycode) {
                gevent.u.chr.autorepeat = 1;
            }
        }
        break;
        */
        case GDK_MOTION_NOTIFY: {
            GdkEventMotion *evt = (GdkEventMotion *)event;
            gevent.type = et_mousemove;
            gdisp->last_event_time = gdk_event_get_time(event);
            gevent.u.mouse.time = gdisp->last_event_time;
            gevent.u.mouse.state = _GGDKDraw_GdkModifierToKsm(evt->state);
            gevent.u.mouse.x = evt->x;
            gevent.u.mouse.y = evt->y;
        }
        break;
        case GDK_SCROLL: { //Synthesize a button press
            GdkEventScroll *evt = (GdkEventScroll *)event;
            gdisp->last_event_time = gdk_event_get_time(event);
            gevent.u.mouse.time = gdisp->last_event_time;
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
            gdisp->last_event_time = gdk_event_get_time(event);
            gevent.u.mouse.time = gdisp->last_event_time;
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
        case GDK_EXPOSE:
            gevent.type = et_expose;
            gevent.u.expose.rect.x = ((GdkEventExpose *)event)->area.x;
            gevent.u.expose.rect.y = ((GdkEventExpose *)event)->area.y;
            gevent.u.expose.rect.width = ((GdkEventExpose *)event)->area.width;
            gevent.u.expose.rect.height = ((GdkEventExpose *)event)->area.height;

            assert(gw->cc == NULL);
            gdk_window_begin_paint_region(w, ((GdkEventExpose *)event)->region);
            gw->cc = gdk_cairo_create(w);
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
        }
        break;
        case GDK_CONFIGURE: {
            GdkEventConfigure *configure = (GdkEventConfigure *)event;
            gevent.type = et_resize;
            gevent.u.resize.size.x = configure->x;
            gevent.u.resize.size.y = configure->y;
            gevent.u.resize.size.width = configure->width;
            gevent.u.resize.size.height = configure->height;
            //if (gw->is_toplevel) {
            //    GPoint p = {0};
            //    GGDKDrawTranslateCoordinates((GWindow)gw, (GWindow)(gdisp->groot), &p);
            //    gevent.u.resize.size.x = p.x;
            //    gevent.u.resize.size.y = p.y;
            //}
            gevent.u.resize.dx = gevent.u.resize.size.x - gw->pos.x;
            gevent.u.resize.dy = gevent.u.resize.size.y - gw->pos.y;
            gevent.u.resize.dwidth = gevent.u.resize.size.width - gw->pos.width;
            gevent.u.resize.dheight = gevent.u.resize.size.height - gw->pos.height;
            gevent.u.resize.moved = gevent.u.resize.sized = false;
            if (gevent.u.resize.dx != 0 || gevent.u.resize.dy != 0) {
                gevent.u.resize.moved = true;
            }
            if (gevent.u.resize.dwidth != 0 || gevent.u.resize.dheight != 0) {
                gevent.u.resize.sized = true;
            }

            if (!gevent.u.resize.sized && gevent.u.resize.moved) {
                gevent.type = et_noevent;
            }

            gw->pos = gevent.u.resize.size;
            if (!gdisp->top_offsets_set && ((GGDKWindow) gw)->was_positioned &&
                    gw->is_toplevel && !((GGDKWindow) gw)->is_popup &&
                    !((GGDKWindow) gw)->istransient) {
                // I don't know why I need a fudge factor here, but I do
                //gdisp->off_x = gevent.u.resize.dx - 2; //TODO: FIXME!
                //gdisp->off_y = gevent.u.resize.dy - 1;
                gdisp->top_offsets_set = true;
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
        case GDK_DELETE:
            gevent.type = et_close;
            break;
        case GDK_CLIENT_EVENT:
            /*
                if ((redirect = InputRedirection(gdisp->input, gw)) != NULL) {
                    GXDrawBeep((GDisplay *) gdisp);
                    return;
                }
                if (event->xclient.message_type == gdisp->atoms.wm_protocols &&
                        event->xclient.data.l[0] == gdisp->atoms.wm_del_window) {
                    gevent.type = et_close;
                } else if (event->xclient.message_type == gdisp->atoms.drag_and_drop) {
                    gevent.type = event->xclient.data.l[0];
                    gevent.u.drag_drop.x = event->xclient.data.l[1];
                    gevent.u.drag_drop.y = event->xclient.data.l[2];
                }*/
            break;
        case GDK_SELECTION_CLEAR: /*{
        int i;
        gdisp->last_event_time = event->xselectionclear.time;
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
                gdisp->last_event_time = event->xselectionrequest.time;
                GXDrawTransmitSelection(gdisp, event);*/
            break;
        case GDK_SELECTION_NOTIFY: // paste
            if (gw->received_selection != NULL) {
                Log(LOGDEBUG, "WARNING: Discarding unused selection!");
                gdk_event_free((GdkEvent *)(gw->received_selection));
            }
            gw->received_selection = (GdkEventSelection *)gdk_event_copy(event);
            break;
        case GDK_PROPERTY_NOTIFY:
            gdisp->last_event_time = gdk_event_get_time(event);
            break;
        default:
            Log(LOGDEBUG, "UNPROCESSED GDK EVENT %d", event->type);
            fflush(stderr);
            break;
    }

    if (gevent.type != et_noevent && gw != NULL && gw->eh != NULL) {
        _GGDKDraw_CallEHChecked(gw, &gevent, gw->eh);
        if (gevent.type == et_expose) {
            gdk_window_end_paint(w);
        }
    }
    Log(LOGDEBUG, "[%d] Finished processing %d(%s)", request_id++, event->type, GdkEventName(event->type));
}

static void GGDKDrawInit(GDisplay *gdisp) {
    _GGDKDraw_InitFonts((GGDKDisplay *) gdisp);
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

    return _GGDKDraw_NewPixmap(gdisp, width, height, CAIRO_FORMAT_A1, data);
}

static GCursor GGDKDrawCreateCursor(GWindow src, GWindow mask, Color fg, Color bg, int16 x, int16 y) {
    Log(LOGDEBUG, "");

    GGDKDisplay *gdisp = (GGDKDisplay *)(src->display);
    GdkDisplay *display = gdisp->display;
    cairo_surface_t *cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, src->pos.width, src->pos.height);
    cairo_t *cc = cairo_create(cs);

    // Masking
    cairo_mask_surface(cc, ((GGDKWindow)mask)->cs, 0, 0);
    //Background
    cairo_set_source_rgb(cc, COLOR_RED(bg) / 255., COLOR_GREEN(bg) / 255., COLOR_BLUE(bg) / 255.);
    cairo_paint(cc);
    //Foreground
    cairo_mask_surface(cc, ((GGDKWindow)src)->cs, 0, 0);
    cairo_set_source_rgb(cc, COLOR_RED(fg) / 255., COLOR_GREEN(bg) / 255., COLOR_BLUE(bg) / 255.);
    cairo_paint(cc);

    GdkCursor *cursor = gdk_cursor_new_from_surface(display, cs, x, y);
    cairo_destroy(cc);
    cairo_surface_destroy(cs);
}

static void GGDKDrawDestroyWindow(GWindow w) {
    Log(LOGDEBUG, "");
    GGDKWindow gw = (GGDKWindow) w;

    gw->is_dying = true;
    if (!gw->is_pixmap) {
        if (gw->display->last_nontransient_window == gw->w) {
            gw->display->last_nontransient_window = NULL;
        }
        gdk_window_destroy(gw->w);
        g_timeout_add(200, _GGDKDraw_OnWindowDestroyed, gw);
    } else {
        _GGDKDraw_OnWindowDestroyed(gw);
    }
}

static void GGDKDrawDestroyCursor(GDisplay *gdisp, GCursor gcursor) {
    Log(LOGDEBUG, "");
    assert(false);
}

static int GGDKDrawNativeWindowExists(GDisplay *gdisp, void *native_window) {
    Log(LOGDEBUG, ""); //assert(false);
    GdkWindow *w = (GdkWindow *)native_window;

    if (!gdk_window_is_destroyed(w) && gdk_window_is_visible(w)) {
        return true;
    } else {
        // So if the window is dying, the gdk window is already gone.
        // But gcontainer.c expects this to return true on et_destroy...
        GGDKWindow gw = g_object_get_data(G_OBJECT(w), "GGDKWindow");
        return gw != NULL && gw->is_dying;
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
    gdk_window_reparent(((GGDKWindow)child)->w, ((GGDKWindow)newparent)->w, x, y);
}

static void GGDKDrawSetVisible(GWindow w, int show) {
    Log(LOGDEBUG, ""); //assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    if (show) {
        if (gw->transient_owner != NULL) {
            gdk_window_show_unraised(gw->w);
        } else {
            gdk_window_show(gw->w);
        }
    } else {
        gdk_window_hide(gw->w);
    }
}

static void GGDKDrawMove(GWindow gw, int32 x, int32 y) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_window_move(((GGDKWindow)gw)->w, x, y);
}

static void GGDKDrawTrueMove(GWindow w, int32 x, int32 y) {
    Log(LOGDEBUG, "");
    GGDKDrawMove(w, x, y);
    //GGDKWindow gw = (GGDKWindow)w;

    //if (gw->is_toplevel && !gw->is_popup && !gw->istransient) {
    //    x -= gw->display->off_x;
    //    y -= gw->display->off_y;
    //}
}

static void GGDKDrawResize(GWindow gw, int32 w, int32 h) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_window_resize(((GGDKWindow)gw)->w, w, h);
}

static void GGDKDrawMoveResize(GWindow gw, int32 x, int32 y, int32 w, int32 h) {
    Log(LOGDEBUG, ""); //assert(false);
    gdk_window_move_resize(((GGDKWindow)gw)->w, x, y, w, h);
}

static void GGDKDrawRaise(GWindow w) {
    Log(LOGDEBUG, ""); //assert(false);
    GGDKWindow gw = (GGDKWindow) w;
    if (gw->transient_owner != NULL) {
        gdk_window_restack(gw->w, gw->transient_owner, true);
    } else {
        gdk_window_raise(((GGDKWindow)gw)->w);
    }
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
    GGDKWindow gw = (GGDKWindow) transient;
    GGDKDisplay *gdisp = gw->display;
    GdkWindow *ow;

    if (owner == (GWindow) - 1) {
        ow = gdisp->last_nontransient_window;
    } else if (owner == NULL) {
        ow = NULL; // Does this work with GDK?
    } else {
        ow = ((GGDKWindow)owner)->w;
    }

    gdk_window_set_transient_for(gw->w, ow);
    gw->transient_owner = ow;
    gw->istransient = (ow != NULL);
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

    // Do I need to unref this?
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
            cursor = gdk_cursor_new_from_name(gw->display->display, "default");
            break;
        case ct_pointer:
            cursor = gdk_cursor_new_from_name(gw->display->display, "pointer");
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
            cursor = gdk_cursor_new_from_name(gw->display->display, "context-menu");
            break;
        case ct_invisible:
            cursor = gdk_cursor_new_from_name(gw->display->display, "none");
            break;
        default:
            Log(LOGDEBUG, "UNSUPPORTED CURSOR!");
    }

    gdk_window_set_cursor(gw->w, cursor);
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
    GdkPoint from_root, to_root;

    gdk_window_get_origin(((GGDKWindow)from)->w, &from_root.x, &from_root.y);
    gdk_window_get_origin(((GGDKWindow)to)->w, &to_root.x, &to_root.y);

    pt->x = from_root.x - to_root.x  + pt->x;
    pt->y = from_root.y - to_root.y + pt->y;
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
    GdkAtom sel, received_type = 0;
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

    gdk_selection_convert(gw->w, sel, GDK_TARGET_STRING, GDK_CURRENT_TIME);
    while (gw->received_selection == NULL) {
        GDrawProcessOneEvent((GDisplay *)(gw->display));
    }
    // This is broken.
    gdk_selection_property_get(gw->w, &data, &received_type, &received_format);
    if (received_type == GDK_SELECTION_TYPE_STRING) {
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
                  GDK_SEAT_CAPABILITY_POINTER,
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
        expose.region = cairo_region_create_rectangle(&expose.area);
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
    return (GTimer *)timer;
}

static void GGDKDrawCancelTimer(GTimer *timer) {
    Log(LOGDEBUG, ""); //assert(false);
    GGDKTimer *gtimer = (GGDKTimer *)timer;
    // If it's not active, then something else is already trying to cancel this timer.
    if (gtimer->active) {
        GGDKDisplay *gdisp = ((GGDKWindow)(gtimer->owner))->display;

        gtimer->active = false;
        g_source_remove(gtimer->glib_timeout_id);
        gdisp->timers = g_list_remove(gdisp->timers, gtimer);
        free(timer);
    }
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

    display = gdk_display_open(displayname);
    if (display == NULL) {
        return NULL;
    }

    gdisp = (GGDKDisplay *)calloc(1, sizeof(GGDKDisplay));
    if (gdisp == NULL) {
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
    gdisp->scale_screen_by = 1;
    gdisp->bs.double_time = 200;
    gdisp->bs.double_wiggle = 3;

    gdisp->pangoc_context = gdk_pango_context_get_for_screen(gdisp->screen);

    groot = (GGDKWindow)calloc(1, sizeof(struct ggdkwindow));
    if (groot == NULL) {
        g_object_unref(gdisp->pangoc_context);
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

    //GGDKResourceInit(gdisp,programname);

    gdisp->def_background = GResourceFindColor("Background", COLOR_CREATE(0xf5, 0xff, 0xfa));
    gdisp->def_foreground = GResourceFindColor("Foreground", COLOR_CREATE(0x00, 0x00, 0x00));
    if (GResourceFindBool("Synchronize", false)) {
        gdk_display_sync(gdisp->display);
    }

    (gdisp->funcs->init)((GDisplay *) gdisp);

    gdk_event_handler_set(_GGDKDraw_DispatchEvent, (gpointer)gdisp, NULL);

    _GDraw_InitError((GDisplay *) gdisp);

    //DEBUG
    if (!getenv("GDK_DEBUG_OFF")) {
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
