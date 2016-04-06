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

static void GGDKDrawSetCursor(GWindow w, GCursor gcursor);
static void GGDKDrawTranslateCoordinates(GWindow from, GWindow to, GPoint *pt);

// Private member functions (file-level)

static GGC *_GGDKDraw_NewGGC() {
    GGC *ggc = calloc(1, sizeof(GGC));
    if (ggc == NULL) {
        fprintf(stderr, "GGC: Memory allocation failed!\n");
        return NULL;
    }

    ggc->clip.width = ggc->clip.height = 0x7fff;
    ggc->fg = 0;
    ggc->bg = 0xffffff;
    return ggc;
}

static cairo_surface_t *_GGDKDraw_OnGdkCreateSurface(GdkWindow *w, gint width, gint height, gpointer user_data) {
    GGDKWindow gw = g_object_get_data(G_OBJECT(w), "GGDKWindow");
    fprintf(stderr, "GDKCALL _GGDKDraw_OnGdkCreateSurface\n");
    if (gw == NULL) {
        fprintf(stderr, "NO GGDKWindow attached to GdkWindow!\n");
    }

    cairo_destroy(gw->cc);
    gw->cc = NULL;
    return NULL;
}

static GWindow _GGDKDraw_CreateWindow(GGDKDisplay *gdisp, GGDKWindow gw, GRect *pos,
                                      int (*eh)(GWindow, GEvent *), void *user_data, GWindowAttrs *wattrs) {

    GWindowAttrs temp = GWINDOWATTRS_EMPTY;
    GdkWindowAttr attribs = {0};
    gint attribs_mask = 0;
    GGDKWindow nw = (GGDKWindow)calloc(1, sizeof(struct ggdkwindow));

    if (nw == NULL) {
        fprintf(stderr, "_GGDKDraw_CreateWindow: GGDKWindow calloc failed.\n");
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
        fprintf(stderr, "_GGDKDraw_CreateWindow: _GGDKDraw_NewGGC returned NULL\n");
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
    }
    if ((wattrs->mask & wam_isdlg) && wattrs->is_dlg) {
        nw->is_dlg = true;
    }
    if ((wattrs->mask & wam_notrestricted) && wattrs->not_restricted) {
        nw->not_restricted = true;
    }

    // Window title and hints
    if (attribs.window_type == GDK_WINDOW_TOPLEVEL) {
        // Icon titles are ignored.
        if ((wattrs->mask & wam_utf8_wtitle) && (wattrs->utf8_window_title != NULL)) {
            nw->window_title = copy(wattrs->utf8_window_title);
        } else if ((wattrs->mask & wam_wtitle) && (wattrs->window_title != NULL)) {
            nw->window_title = u2utf8_copy(wattrs->window_title);
        }

        attribs.title = nw->window_title;
        attribs.type_hint = nw->is_dlg ? GDK_WINDOW_TYPE_HINT_DIALOG : GDK_WINDOW_TYPE_HINT_NORMAL;

        if (attribs.title != NULL) {
            attribs_mask |= GDK_WA_TITLE;
        }
        attribs_mask |= GDK_WA_TYPE_HINT;
    }

    // Event mask
    attribs.event_mask = GDK_ALL_EVENTS_MASK | GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK;
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
            attribs.event_mask |= GDK_BUTTON_PRESS_MASK;
        }
        if (wattrs->event_masks & (1 << et_mouseup)) {
            attribs.event_mask |= GDK_BUTTON_RELEASE_MASK;
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
        fprintf(stderr, "GGDKDraw: Failed to create window!\n");
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

    if (attribs.window_type == GDK_WINDOW_TOPLEVEL) {
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
        GdkWindowHints hints = GDK_HINT_MIN_SIZE /* | GDK_HINT_MAX_SIZE*/ | GDK_HINT_BASE_SIZE;

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
            //++gdisp->top_window_count;
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

    // Determine when we need to refresh the Cairo context
    //g_signal_connect(nw->w, "create-surface",
    //                G_CALLBACK (_GGDKDraw_OnGdkCreateSurface), NULL);

    // Event handler
    if (eh != NULL) {
        GEvent e = {0};
        e.type = et_create;
        e.w = (GWindow) nw;
        e.native_window = (void *)(intpt) nw->w;
        (eh)((GWindow) nw, &e);
    }

    // This should not be here. Debugging purposes only.
    // gdk_window_show(nw->w);

    // Set the user data to the GWindow
    // Although there is gdk_window_set_user_data, if non-NULL,
    // GTK assumes it's a GTKWindow.
    g_object_set_data(G_OBJECT(nw->w), "GGDKWindow", nw);
    return (GWindow)nw;
}

static void _GGDKDraw_InitFonts(GGDKDisplay *gdisp) {
    FState *fs = calloc(1, sizeof(FState));
    if (fs == NULL) {
        fprintf(stderr, "GGDKDraw: FState alloc failed!\n");
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
        fprintf(stderr, "GGDKDRAW: GGDKWindow calloc failed!\n");
        return NULL;
    }

    gw->ggc = _GGDKDraw_NewGGC();
    if (gw->ggc == NULL) {
        fprintf(stderr, "GGDKDRAW: GGC alloc failed!\n");
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
        fprintf(stderr, "GGDKDRAW: Cairo image surface creation failed!\n");
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

static enum visibility_state _GGDKDraw_GdkVisibilityStateToVS(GdkVisibilityState state) {
    switch (state) {
    case GDK_VISIBILITY_FULLY_OBSCURED:
                return vs_obscured;
        case GDK_VISIBILITY_PARTIAL:
            return vs_partially;
        case GDK_VISIBILITY_UNOBSCURED:
            return vs_unobscured;
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

static void _GGDKDraw_CleanUpWindow(GGDKWindow gw) {

}

static void _GGDKDraw_DispatchEvent(GdkEvent *event, gpointer data) {
    struct gevent gevent = {0};
    GGDKDisplay *gdisp = (GGDKDisplay *)data;
    GdkWindow *w = gdk_event_get_window(event);
    GGDKWindow gw;

    fprintf(stderr, "RECEIVED GDK EVENT %d\n", event->type);
    fflush(stderr);

    if (event->type == GDK_DELETE) {
        event->type = GDK_DESTROY;
    }

    if (w == NULL) {
        return;
    } else if ((gw = g_object_get_data(G_OBJECT(w), "GGDKWindow")) == NULL) {
        fprintf(stderr, "MISSING GW!\n");
        assert(false);
        return;
    } else if (_GGDKDraw_WindowOrParentsDying(gw) && event->type != GDK_DESTROY) {
        fprintf(stderr, "DYING!\n");
        return;
    }

    gevent.w = (GWindow)gw;
    gevent.native_window = (void *)gw->w;
    gevent.type = -1;
    if (event->type == GDK_KEY_PRESS || event->type == GDK_BUTTON_PRESS || event->type == GDK_BUTTON_RELEASE) {
        if (gw->transient_owner != 0 && gw->isverytransient) {
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
    case ButtonPress:
    case ButtonRelease:
    case MotionNotify:
    if (event->type == ButtonPress) {
        gdisp->grab_window = gw;
    } else if (gdisp->grab_window != NULL) {
        if (gw != gdisp->grab_window) {
            Window wjunk;
            gevent.w = gw = gdisp->grab_window;
            XTranslateCoordinates(gdisp->display,
                                  event->xbutton.window, ((GGDKWindow) gw)->w,
                                  event->xbutton.x, event->xbutton.y,
                                  &event->xbutton.x, &event->xbutton.y,
                                  &wjunk);
        }
        if (event->type == ButtonRelease) {
            gdisp->grab_window = NULL;
        }
    }

    gdisp->last_event_time = event->xbutton.time;
    gevent.u.mouse.time = event->xbutton.time;
    if (event->type == MotionNotify && gdisp->grab_window == NULL)
        // Allow simple motion events to go through
    else if ((redirect = InputRedirection(gdisp->input, gw)) != NULL) {
        if (event->type == ButtonPress) {
            GXDrawBeep((GDisplay *) gdisp);
        }
        return;
    }
    gevent.u.mouse.state = event->xbutton.state;
    gevent.u.mouse.x = event->xbutton.x;
    gevent.u.mouse.y = event->xbutton.y;
    gevent.u.mouse.button = event->xbutton.button;
    gevent.u.mouse.device = NULL;
    gevent.u.mouse.pressure = gevent.u.mouse.xtilt = gevent.u.mouse.ytilt = gevent.u.mouse.separation = 0;
    if ((event->xbutton.state & 0x40) && gdisp->twobmouse_win) {
        gevent.u.mouse.button = 2;
    }
    if (event->type == MotionNotify) {
    #if defined (__MINGW32__) || __CygWin
        //For some reason, a mouse move event is triggered even if it hasn't moved.
        if (gdisp->mousemove_last_x == event->xbutton.x &&
                gdisp->mousemove_last_y == event->xbutton.y) {
            return;
        }
        gdisp->mousemove_last_x = event->xbutton.x;
        gdisp->mousemove_last_y = event->xbutton.y;
    #endif
        gevent.type = et_mousemove;
        gevent.u.mouse.button = 0;
        gevent.u.mouse.clicks = 0;
    } else if (event->type == ButtonPress) {
        int diff, temp;
        gevent.type = et_mousedown;
        if ((diff = event->xbutton.x - gdisp->bs.release_x) < 0) {
            diff = -diff;
        }
        if ((temp = event->xbutton.y - gdisp->bs.release_y) < 0) {
            temp = -temp;
        }
        if (diff + temp < gdisp->bs.double_wiggle &&
                event->xbutton.window == gdisp->bs.release_w &&
                event->xbutton.button == gdisp->bs.release_button &&
                event->xbutton.time - gdisp->bs.release_time < gdisp->bs.double_time &&
                event->xbutton.time >= gdisp->bs.release_time) {	// Time can wrap
            ++ gdisp->bs.cur_click;
        } else {
            gdisp->bs.cur_click = 1;
        }
        gevent.u.mouse.clicks = gdisp->bs.cur_click;
    } else {
        gevent.type = et_mouseup;
        gdisp->bs.release_time = event->xbutton.time;
        gdisp->bs.release_w = event->xbutton.window;
        gdisp->bs.release_x = event->xbutton.x;
        gdisp->bs.release_y = event->xbutton.y;
        gdisp->bs.release_button = event->xbutton.button;
        gevent.u.mouse.clicks = gdisp->bs.cur_click;
    }
    break;
    */
    case GDK_EXPOSE:
        cairo_destroy(gw->cc);
        gw->cc = gdk_cairo_create(gw->w);
        gevent.type = et_expose;
        gevent.u.expose.rect.x = ((GdkEventExpose *)event)->area.x;
        gevent.u.expose.rect.y = ((GdkEventExpose *)event)->area.y;
        gevent.u.expose.rect.width = ((GdkEventExpose *)event)->area.width;
        gevent.u.expose.rect.height = ((GdkEventExpose *)event)->area.height;
        /* X11 does this automatically. but cairo won't get the event */
        GGDKDrawClear((GWindow)gw, &gevent.u.expose.rect);
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
            (gw->eh)((GWindow) gw, &gevent);
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
        // We need to recreate our Cairo context here.
        cairo_destroy(gw->cc);
        gw->cc = gdk_cairo_create(gw->w);

        GdkEventConfigure *configure = (GdkEventConfigure *)event;
        gevent.type = et_resize;
        gevent.u.resize.size.x = configure->x;
        gevent.u.resize.size.y = configure->y;
        gevent.u.resize.size.width = configure->width;
        gevent.u.resize.size.height = configure->height;
        if (gw->is_toplevel) {
            GPoint p = {0};
            GGDKDrawTranslateCoordinates((GWindow)gw, (GWindow)(gdisp->groot), &p);
            gevent.u.resize.size.x = p.x;
            gevent.u.resize.size.y = p.y;
        }
        gevent.u.resize.dx = gevent.u.resize.size.x - gw->pos.x;
        gevent.u.resize.dy = gevent.u.resize.size.y - gw->pos.y;
        gevent.u.resize.dwidth = gevent.u.resize.size.width - gw->pos.width;
        gevent.u.resize.dheight = gevent.u.resize.size.height - gw->pos.height;
        gevent.u.resize.moved = gevent.u.resize.sized = false;
        if (gevent.u.resize.dx != 0 || gevent.u.resize.dy != 0) {
            gevent.u.resize.moved = true;
        }

        gw->pos = gevent.u.resize.size;
        if (!gdisp->top_offsets_set && ((GGDKWindow) gw)->was_positioned &&
                gw->is_toplevel && !((GGDKWindow) gw)->is_popup &&
                !((GGDKWindow) gw)->istransient) {
            /* I don't know why I need a fudge factor here, but I do */
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
        gevent.type = et_destroy;
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
    case GDK_SELECTION_NOTIFY:		// Paste
        /*gdisp->last_event_time = event->xselection.time;*/ /* it's the request's time not the current? */
        break;
    case GDK_PROPERTY_NOTIFY:
        gdisp->last_event_time = gdk_event_get_time(event);
        break;
    default:
        fprintf(stderr, "UNPROCESSED GDK EVENT %d\n", event->type);
        fflush(stderr);
        break;
    }

    if (gevent.type != et_noevent && gw != NULL && gw->eh != NULL) {
        (gw->eh)((GWindow) gw, &gevent);
    }
    if (event->type == DestroyNotify && gw != NULL) {
        _GGDKDraw_CleanUpWindow(gw);
    }
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
    fprintf(stderr, "GDKCALL: GGDKDrawCreateTopWindow\n");
    return _GGDKDraw_CreateWindow((GGDKDisplay *) gdisp, NULL, pos, eh, user_data, gattrs);
}

static GWindow GGDKDrawCreateSubWindow(GWindow gw, GRect *pos, int (*eh)(GWindow gw, GEvent *), void *user_data,
                                       GWindowAttrs *gattrs) {
    fprintf(stderr, "GDKCALL: GGDKDrawCreateSubWindow\n");
    return _GGDKDraw_CreateWindow(((GGDKWindow) gw)->display, (GGDKWindow) gw, pos, eh, user_data, gattrs);
}

static GWindow GGDKDrawCreatePixmap(GDisplay *gdisp, uint16 width, uint16 height) {
    fprintf(stderr, "GDKCALL: GGDKDrawCreatePixmap\n");

    //TODO: Check format?
    return _GGDKDraw_NewPixmap(gdisp, width, height, CAIRO_FORMAT_ARGB32, NULL);
}

static GWindow GGDKDrawCreateBitmap(GDisplay *gdisp, uint16 width, uint16 height, uint8 *data) {
    fprintf(stderr, "GDKCALL: GGDKDrawCreateBitmap\n");

    return _GGDKDraw_NewPixmap(gdisp, width, height, CAIRO_FORMAT_A1, data);
}

static GCursor GGDKDrawCreateCursor(GWindow src, GWindow mask, Color fg, Color bg, int16 x, int16 y) {
    fprintf(stderr, "GDKCALL: GGDKDrawCreateCursor\n");

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
    fprintf(stderr, "GDKCALL: GGDKDrawDestroyWindow\n");
    GGDKWindow gw = (GGDKWindow) w;

    g_object_unref(gw->pango_layout);
    cairo_destroy(gw->cc);
    free(gw->window_title);
    gw->window_title = NULL;

    gw->cc = NULL;
    if (gw->cs != NULL) {
        cairo_surface_destroy(gw->cs);
        gw->cs = NULL;
    }

    if (!gw->is_pixmap) {
        gw->is_dying = true;
        //if (gw->display->grab_window==w ) gw->display->grab_window = NULL;
        //XDestroyWindow(gw->display->display,gw->w);
        //Windows should be freed when we get the destroy event
        gdk_window_destroy(gw->w);
    }
}

static void GGDKDrawDestroyCursor(GDisplay *gdisp, GCursor gcursor) {
    fprintf(stderr, "GDKCALL: GGDKDrawDestroyCursor\n");
    assert(false);
}

// Some hack to see if the window has been created(???)
static int GGDKDrawNativeWindowExists(GDisplay *gdisp, void *native_window) {
    fprintf(stderr, "GDKCALL: GGDKDrawNativeWindowExists\n"); //assert(false);
    //gdk_window_is_viewable((GdkWindow*)native_window);
    return true;
}

static void GGDKDrawSetZoom(GWindow gw, GRect *size, enum gzoom_flags flags) {
    fprintf(stderr, "GDKCALL: GGDKDrawSetZoom\n");
    assert(false);
}

// Not possible?
static void GGDKDrawSetWindowBorder(GWindow gw, int width, Color gcol) {
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowBorder\n"); //assert(false);
}

static void GGDKDrawSetWindowBackground(GWindow gw, Color gcol) {
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowBackground\n"); //assert(false);
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
    fprintf(stderr, "GDKCALL: GGDKDrawSetDither\n"); //assert(false);
    return false;
}


static void GGDKDrawReparentWindow(GWindow child, GWindow newparent, int x, int y) {
    fprintf(stderr, "GDKCALL: GGDKDrawReparentWindow\n");
    gdk_window_reparent(((GGDKWindow)child)->w, ((GGDKWindow)newparent)->w, x, y);
}

static void GGDKDrawSetVisible(GWindow gw, int show) {
    fprintf(stderr, "GDKCALL: GGDKDrawSetVisible\n"); //assert(false);
    if (show) {
        gdk_window_show(((GGDKWindow)gw)->w);
    } else {
        gdk_window_hide(((GGDKWindow)gw)->w);
    }
}

static void GGDKDrawMove(GWindow gw, int32 x, int32 y) {
    fprintf(stderr, "GDKCALL: GGDKDrawMove\n"); //assert(false);
    gdk_window_move(((GGDKWindow)gw)->w, x, y);
}

static void GGDKDrawTrueMove(GWindow w, int32 x, int32 y) {
    fprintf(stderr, "GDKCALL: GGDKDrawTrueMove\n");
    GGDKDrawMove(w, x, y);
    //GGDKWindow gw = (GGDKWindow)w;

    //if (gw->is_toplevel && !gw->is_popup && !gw->istransient) {
    //    x -= gw->display->off_x;
    //    y -= gw->display->off_y;
    //}
}

static void GGDKDrawResize(GWindow gw, int32 w, int32 h) {
    fprintf(stderr, "GDKCALL: GGDKDrawResize\n"); //assert(false);
    gdk_window_resize(((GGDKWindow)gw)->w, w, h);
}

static void GGDKDrawMoveResize(GWindow gw, int32 x, int32 y, int32 w, int32 h) {
    fprintf(stderr, "GDKCALL: GGDKDrawMoveResize\n"); //assert(false);
    gdk_window_move_resize(((GGDKWindow)gw)->w, x, y, w, h);
}

static void GGDKDrawRaise(GWindow gw) {
    fprintf(stderr, "GDKCALL: GGDKDrawRaise\n"); //assert(false);
    gdk_window_raise(((GGDKWindow)gw)->w);
}

static void GGDKDrawRaiseAbove(GWindow gw1, GWindow gw2) {
    fprintf(stderr, "GDKCALL: GGDKDrawRaiseAbove\n"); //assert(false);
    gdk_window_restack(((GGDKWindow)gw1)->w, ((GGDKWindow)gw2)->w, true);
}

// Only used once in gcontainer - force it to call GDrawRaiseAbove
static int GGDKDrawIsAbove(GWindow gw1, GWindow gw2) {
    fprintf(stderr, "GDKCALL: GGDKDrawIsAbove\n"); //assert(false);
    return false;
}

static void GGDKDrawLower(GWindow gw) {
    fprintf(stderr, "GDKCALL: GGDKDrawLower\n"); //assert(false);
    gdk_window_lower(((GGDKWindow)gw)->w);
}

// Icon title is ignored.
static void GGDKDrawSetWindowTitles8(GWindow w, const char *title, const char *icontitle) {
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowTitles8\n");// assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    free(gw->window_title);
    gw->window_title = copy(title);

    if (title != NULL && gw->is_toplevel) {
        gdk_window_set_title(gw->w, title);
    }
}

static void GGDKDrawSetWindowTitles(GWindow gw, const unichar_t *title, const unichar_t *icontitle) {
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowTitles\n"); //assert(false);
    char *str = u2utf8_copy(title);
    if (str != NULL) {
        GGDKDrawSetWindowTitles8(gw, str, NULL);
        free(str);
    }
}

// Sigh. GDK doesn't provide a way to get the window title...
static unichar_t *GGDKDrawGetWindowTitle(GWindow gw) {
    fprintf(stderr, "GDKCALL: GGDKDrawGetWindowTitle\n"); // assert(false);
    return utf82u_copy(((GGDKWindow)gw)->window_title);
}

static char *GGDKDrawGetWindowTitle8(GWindow gw) {
    fprintf(stderr, "GDKCALL: GGDKDrawGetWindowTitle8\n"); //assert(false);
    return copy(((GGDKWindow)gw)->window_title);
}

static void GGDKDrawSetTransientFor(GWindow transient, GWindow owner) {
    fprintf(stderr, "GDKCALL: GGDKDrawSetTransientFor\n"); //assert(false);
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
    fprintf(stderr, "GDKCALL: GGDKDrawGetPointerPos\n"); //assert(false);
    GGDKWindow gw = (GGDKWindow)w;

    // Well GDK likes deprecating everything...
    // This is the latest 'non-deprecated' version.
    // But it's only available in GDK 3.2...
    // Might need to write a version using 'deprecated' functions...
    GdkSeat *seat = gdk_display_get_default_seat(((GGDKWindow)gw)->display->display);
    if (seat == NULL) {
        return;
    }

    GdkDevice *pointer = gdk_seat_get_pointer(seat);
    if (pointer == NULL) {
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
    fprintf(stderr, "GDKCALL: GGDKDrawGetPointerWindow\n");  //assert(false);

    GdkSeat *seat = gdk_display_get_default_seat(((GGDKWindow)gw)->display->display);
    if (seat == NULL) {
        return NULL;
    }

    GdkDevice *pointer = gdk_seat_get_pointer(seat);
    if (pointer == NULL) {
        return NULL;
    }

    // Do I need to unref this?
    GdkWindow *window = gdk_device_get_window_at_position(pointer, NULL, NULL);
    if (window != NULL) {
        return (GWindow)g_object_get_data(G_OBJECT(window), "GGDKWindow");
    }
    return NULL;
}

static void GGDKDrawSetCursor(GWindow w, GCursor gcursor) {
    fprintf(stderr, "GDKCALL: GGDKDrawSetCursor\n"); //assert(false);
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
        fprintf(stderr, "UNSUPPORTED CURSOR!\n");
    }

    gdk_window_set_cursor(gw->w, cursor);
}

static GCursor GGDKDrawGetCursor(GWindow gw) {
    fprintf(stderr, "GDKCALL: GGDKDrawGetCursor\n");
    //assert(false);
    return ct_default;
}

static GWindow GGDKDrawGetRedirectWindow(GDisplay *gdisp) {
    fprintf(stderr, "GDKCALL: GGDKDrawGetRedirectWindow\n");
    assert(false);
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
    fprintf(stderr, "GDKCALL: GGDKDrawTranslateCoordinates\n");
    GdkPoint from_root, to_root;

    gdk_window_get_root_origin(((GGDKWindow)from)->w, &from_root.x, &from_root.y);
    gdk_window_get_root_origin(((GGDKWindow)to)->w, &to_root.x, &to_root.y);

    pt->x = to_root.x - from_root.x + pt->x;
    pt->y = to_root.y - from_root.y + pt->y;
}


static void GGDKDrawBeep(GDisplay *gdisp) {
    fprintf(stderr, "GDKCALL: GGDKDrawBeep\n"); //assert(false);
    gdk_display_beep(((GGDKDisplay *)gdisp)->display);
}

static void GGDKDrawFlush(GDisplay *gdisp) {
    fprintf(stderr, "GDKCALL: GGDKDrawFlush\n"); //assert(false);
    gdk_display_flush(((GGDKDisplay *)gdisp)->display);
}

static void GGDKDrawScroll(GWindow w, GRect *rect, int32 hor, int32 vert) {
    fprintf(stderr, "GDKCALL: GGDKDrawScroll\n");
    assert(false);
    /*
    GGDKWindow gw = (GGDKWindow) w;
    GGDKDisplay *gdisp = gw->display;
    GRect temp, old;

    vert = -vert;

    if (rect == NULL) {
        temp.x = temp.y = 0;
        temp.width = gw->pos.width;
        temp.height = gw->pos.height;
        rect = &temp;
    }

    GDrawPushClip(w, rect, &old);

    // Cairo can happily scroll the window -- except it doesn't know about
    //  child windows, and so we don't get the requisit events to redraw
    //  areas covered by children. Rats.
    GXDrawSendExpose(gw, rect->x, rect->y, rect->x + rect->width, rect->y + rect->height);
    GDrawPopClip(w, &old);
    */
}


static GIC *GGDKDrawCreateInputContext(GWindow gw, enum gic_style style) {
    fprintf(stderr, "GDKCALL: GGDKDrawCreateInputContext\n");
    assert(false);
}

static void GGDKDrawSetGIC(GWindow gw, GIC *gic, int x, int y) {
    fprintf(stderr, "GDKCALL: GGDKDrawSetGIC\n");
    assert(false);
}


static void GGDKDrawGrabSelection(GWindow w, enum selnames sel) {
    fprintf(stderr, "GDKCALL: GGDKDrawGrabSelection\n"); //assert(false);
}

static void GGDKDrawAddSelectionType(GWindow w, enum selnames sel, char *type, void *data, int32 cnt, int32 unitsize,
                                     void *gendata(void *, int32 *len), void freedata(void *)) {
    fprintf(stderr, "GDKCALL: GGDKDrawAddSelectionType\n"); //assert(false);
}

static void *GGDKDrawRequestSelection(GWindow w, enum selnames sn, char *typename, int32 *len) {
    fprintf(stderr, "GDKCALL: GGDKDrawRequestSelection\n"); //assert(false);
}

static int GGDKDrawSelectionHasType(GWindow w, enum selnames sn, char *typename) {
    fprintf(stderr, "GDKCALL: GGDKDrawSelectionHasType\n"); //assert(false);
}

static void GGDKDrawBindSelection(GDisplay *gdisp, enum selnames sn, char *atomname) {
    fprintf(stderr, "GDKCALL: GGDKDrawBindSelection\n"); //assert(false); //TODO: Implement selections (clipboard)
}

static int GGDKDrawSelectionHasOwner(GDisplay *gdisp, enum selnames sn) {
    fprintf(stderr, "GDKCALL: GGDKDrawSelectionHasOwner\n");
    assert(false);
}


static void GGDKDrawPointerUngrab(GDisplay *gdisp) {
    fprintf(stderr, "GDKCALL: GGDKDrawPointerUngrab\n"); //assert(false);
    //Supposedly deprecated but I don't care
    //gdk_display_pointer_ungrab(((GGDKDisplay*)gdisp)->display, GDK_CURRENT_TIME);
}

static void GGDKDrawPointerGrab(GWindow gw) {
    fprintf(stderr, "GDKCALL: GGDKDrawPointerGrab\n"); //assert(false);
    //Supposedly deprecated but I don't care
    //WTF? don't exist? sigh. Have to use the seat/device/...
    //gdk_display_pointer_grab(((GGDKDisplay*)gdisp)->display, GDK_CURRENT_TIME);
}

static void GGDKDrawRequestExpose(GWindow w, GRect *rect, int doclear) {
    fprintf(stderr, "GDKCALL: GGDKDrawRequestExpose\n"); // assert(false);

    GGDKWindow gw = (GGDKWindow) w;
    GGDKDisplay *display = gw->display;
    GRect temp;

    if (!gw->is_visible || _GGDKDraw_WindowOrParentsDying(gw)) {
        return;
    }
    if (rect == NULL) {
        temp.x = temp.y = 0;
        temp.width = gw->pos.width;
        temp.height = gw->pos.height;
        rect = &temp;
    } else if (rect->x < 0 || rect->y < 0 || rect->x + rect->width > gw->pos.width ||
               rect->y + rect->height > gw->pos.height) {
        temp = *rect;
        if (temp.x < 0) {
            temp.width += temp.x;
            temp.x = 0;
        }
        if (temp.y < 0) {
            temp.height += temp.y;
            temp.y = 0;
        }
        if (temp.x + temp.width > gw->pos.width) {
            temp.width = gw->pos.width - temp.x;
        }
        if (temp.y + temp.height > gw->pos.height) {
            temp.height = gw->pos.height - temp.y;
        }
        if (temp.height <= 0 || temp.width <= 0) {
            return;
        }
        rect = &temp;
    }

    GdkRectangle rct = {.x = rect->x, .y = rect->y, .width = rect->width, .height = rect->height};
    gdk_window_begin_paint_rect(gw->w, &rct);

    if (doclear) {
        GDrawClear(w, rect);
    }

    if (gw->eh != NULL) {
        struct gevent event;
        memset(&event, 0, sizeof(event));
        event.type = et_expose;
        event.u.expose.rect = *rect;
        event.w = w;
        event.native_window = gw->w;
        (gw->eh)(w, &event);
    }
    gdk_window_end_paint(gw->w);
}

static void GGDKDrawForceUpdate(GWindow gw) {
    fprintf(stderr, "GDKCALL: GGDKDrawForceUpdate\n");
    assert(false);
}

static void GGDKDrawSync(GDisplay *gdisp) {
    fprintf(stderr, "GDKCALL: GGDKDrawSync\n"); //assert(false)
    gdk_display_sync(((GGDKDisplay *)gdisp)->display);
}

static void GGDKDrawSkipMouseMoveEvents(GWindow gw, GEvent *gevent) {
    fprintf(stderr, "GDKCALL: GGDKDrawSkipMouseMoveEvents\n");
    assert(false);
}

static void GGDKDrawProcessPendingEvents(GDisplay *gdisp) {
    fprintf(stderr, "GDKCALL: GGDKDrawProcessPendingEvents\n");
    //assert(false);
    GMainContext *ctx = g_main_loop_get_context(((GGDKDisplay *)gdisp)->main_loop);
    if (ctx != NULL) {
        while (g_main_context_iteration(ctx, false));
    }
}

static void GGDKDrawProcessWindowEvents(GWindow gw) {
    fprintf(stderr, "GDKCALL: GGDKDrawProcessWindowEvents\n"); //assert(false);
    GGDKDrawProcessPendingEvents((GDisplay *)((GGDKWindow)gw)->display);
}

static void GGDKDrawProcessOneEvent(GDisplay *gdisp) {
    //fprintf(stderr, "GDKCALL: GGDKDrawProcessOneEvent\n"); //assert(false);
    GMainContext *ctx = g_main_loop_get_context(((GGDKDisplay *)gdisp)->main_loop);
    if (ctx != NULL) {
        g_main_context_iteration(ctx, true);
    }
}

static void GGDKDrawEventLoop(GDisplay *gdisp) {
    fprintf(stderr, "GDKCALL: GGDKDrawEventLoop\n");
    //assert(false);
    GMainContext *ctx = g_main_loop_get_context(((GGDKDisplay *)gdisp)->main_loop);
    if (ctx != NULL) {
        while (g_main_context_iteration(ctx, true))
            ;
    }
}

static void GGDKDrawPostEvent(GEvent *e) {
    fprintf(stderr, "GDKCALL: GGDKDrawPostEvent\n");
    assert(false);
}

static void GGDKDrawPostDragEvent(GWindow w, GEvent *mouse, enum event_type et) {
    fprintf(stderr, "GDKCALL: GGDKDrawPostDragEvent\n");
    assert(false);
}

static int GGDKDrawRequestDeviceEvents(GWindow w, int devcnt, struct gdeveventmask *de) {
    fprintf(stderr, "GDKCALL: GGDKDrawRequestDeviceEvents\n");
    assert(false);
}


static GTimer *GGDKDrawRequestTimer(GWindow w, int32 time_from_now, int32 frequency, void *userdata) {
    fprintf(stderr, "GDKCALL: GGDKDrawRequestTimer\n"); //assert(false);
    return NULL;
}

static void GGDKDrawCancelTimer(GTimer *timer) {
    fprintf(stderr, "GDKCALL: GGDKDrawCancelTimer\n"); //assert(false);
}


static void GGDKDrawSyncThread(GDisplay *gdisp, void (*func)(void *), void *data) {
    fprintf(stderr, "GDKCALL: GGDKDrawSyncThread\n"); //assert(false); // For some shitty gio impl. Ignore ignore ignore!
}


static GWindow GGDKDrawPrinterStartJob(GDisplay *gdisp, void *user_data, GPrinterAttrs *attrs) {
    fprintf(stderr, "GDKCALL: GGDKDrawStartJob\n");
    assert(false);
}

static void GGDKDrawPrinterNextPage(GWindow w) {
    fprintf(stderr, "GDKCALL: GGDKDrawNextPage\n");
    assert(false);
}

static int GGDKDrawPrinterEndJob(GWindow w, int cancel) {
    fprintf(stderr, "GDKCALL: GGDKDrawEndJob\n");
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
    gdisp->main_loop = g_main_loop_new(NULL, true);
    gdisp->scale_screen_by = 1;

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
    //gdisp->top_window_count = 0; //Reference counting toplevel windows?

    gdk_event_handler_set(_GGDKDraw_DispatchEvent, (gpointer)gdisp, NULL);

    _GDraw_InitError((GDisplay *) gdisp);

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
