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

#ifdef FONTFORGE_CAN_USE_QT

#include "gqtdrawP.h"
#include "ustring.h"

#include <QtWidgets>
#include <type_traits>

namespace fontforge { namespace gdraw {

// Forward declarations
static void GQtDrawCancelTimer(GTimer *timer);
static void GQtDrawDestroyWindow(GWindow w);
static void GQtDrawPostEvent(GEvent *e);
static void GQtDrawProcessPendingEvents(GDisplay *disp);
static void GQtDrawSetCursor(GWindow w, GCursor gcursor);
static void GQtDrawSetTransientFor(GWindow transient, GWindow owner);
static void GQtDrawSetWindowBackground(GWindow w, Color gcol);


static GGC *_GQtDraw_NewGGC(void) {
    GGC *ggc = new GGC();
    ggc->clip.width = ggc->clip.height = 0x7fff;
    ggc->fg = 0;
    ggc->bg = 0xffffff;
    return ggc;
}

static int16_t _GQtDraw_QtModifierToKsm(Qt::KeyboardModifiers mask) {
    int16_t state = 0;
    if (mask & Qt::ShiftModifier) {
        state |= ksm_shift;
    }
    if (mask & Qt::ControlModifier) {
        state |= ksm_control;
    }
    if (mask & Qt::AltModifier) {
        state |= ksm_meta;
    }
    if (mask & Qt::MetaModifier) {
        state |= ksm_meta;
    }
    return state;
}

static void _GQtDraw_CallEHChecked(GQtWindow *gw, GEvent *event, int (*eh)(GWindow w, GEvent *)) {
    if (eh) {
        (eh)(gw->Base(), event);
    }
}


static GWindow _GQtDraw_CreateWindow(GQtDisplay *gdisp, GWindow w, GRect *pos,
                                      int (*eh)(GWindow, GEvent *), void *user_data, GWindowAttrs *wattrs) {

    Qt::WindowFlags windowFlags = Qt::Widget;
    std::unique_ptr<GQtWindow> nw(new GQtWindow());
    GWindow ret = nw->Base();
    ret->native_window = nw.get();

    if (wattrs == nullptr) {
        static GWindowAttrs temp = GWINDOWATTRS_EMPTY;
        wattrs = &temp;
    }

    if (w == nullptr) { // Creating a top-level window. Set parent as default root.
        windowFlags |= Qt::Window;
    }

    // Now check window type
    if ((wattrs->mask & wam_nodecor) && wattrs->nodecoration) {
        // Is a modeless dialogue
        ret->is_popup = true;
        nw->is_dlg = true;
        windowFlags |= Qt::Popup; // hmm
    } else if ((wattrs->mask & wam_isdlg) && wattrs->is_dlg) {
        nw->is_dlg = true;
        windowFlags |= Qt::Dialog;
    }
    ret->is_toplevel = (windowFlags & Qt::Window) != 0;

    // Drawing context
    ret->ggc = _GQtDraw_NewGGC();

    // Base fields
    ret->display = gdisp->Base();
    ret->eh = eh;
    ret->parent = w;
    ret->pos = *pos;
    ret->user_data = user_data;

    QString title;
    QWidget *parent = nullptr;

    // Window title, hints
    if (ret->is_toplevel) {
        // Icon titles are ignored.
        if ((wattrs->mask & wam_utf8_wtitle) && (wattrs->utf8_window_title != nullptr)) {
            title = QString::fromUtf8(wattrs->utf8_window_title);
            nw->window_title = wattrs->utf8_window_title;
        }
        if (ret->is_popup || (wattrs->mask & wam_palette)) {
            windowFlags |= Qt::ToolTip;
        }
    } else {
        parent = GQtW(w)->Widget();
    }

    if (wattrs->mask & wam_restrict) {
        nw->restrict_input_to_me = wattrs->restrict_input_to_me;
    }

    std::unique_ptr<QWidget> window(new QWidget(parent, windowFlags));
    nw->q_base = window.get();

    window->resize(pos->width, pos->height);

    // We center windows here because we need to know the window size+decor
    // There is a bug on Windows (all versions < 3.21.1, <= 2.24.30) so don't use GDK_WA_X/GDK_WA_Y
    // https://bugzilla.gnome.org/show_bug.cgi?id=764996
    if (ret->is_toplevel && (!(wattrs->mask & wam_positioned) || (wattrs->mask & wam_centered))) {
        nw->is_centered = true;
        // _GQtDraw_CenterWindowOnScreen(nw);
    } else {
        window->move(ret->pos.x, ret->pos.y);
    }

    // Set background
    if (!(wattrs->mask & wam_backcol) || wattrs->background_color == COLOR_DEFAULT) {
        wattrs->background_color = _GDraw_res_bg;
    }
    ret->ggc->bg = wattrs->background_color;
    GQtDrawSetWindowBackground(ret, wattrs->background_color);

    if (ret->is_toplevel) {
        // Set icon
        GQtWindow *icon = gdisp->default_icon;
        if (((wattrs->mask & wam_icon) && wattrs->icon != nullptr) && wattrs->icon->is_pixmap) {
            icon = GQtW(wattrs->icon);
        }
        if (icon != nullptr) {
            window->setWindowIcon(QIcon(*icon->Pixmap()));
        } else {
            // gdk_window_set_decorations(nw->w, GDK_DECOR_ALL | GDK_DECOR_MENU);
        }

        if (wattrs->mask & wam_palette) {
            window->setBaseSize(window->size());
            window->setMinimumSize(window->size());
        }
        if ((wattrs->mask & wam_noresize) && wattrs->noresize) {
            window->setFixedSize(window->size());
        }
        nw->was_positioned = true;

        if ((wattrs->mask & wam_transient) && wattrs->transient != nullptr) {
            GQtDrawSetTransientFor(ret, wattrs->transient);
            nw->is_dlg = true;
        } else if (!nw->is_dlg) {
            ++gdisp->top_window_count;
        }
        // else if (nw->restrict_input_to_me && gdisp->mru_windows->length > 0) {
        //     GQtDrawSetTransientFor(ret, (GWindow) - 1);
        // }
        nw->isverytransient = (wattrs->mask & wam_verytransient) ? 1 : 0;
    }

    if ((wattrs->mask & wam_cursor) && wattrs->cursor != ct_default) {
        GQtDrawSetCursor(ret, wattrs->cursor);
    }

    // Event handler
    if (eh != nullptr) {
        GEvent e = {};
        e.type = et_create;
        e.w = ret;
        e.native_window = nw.get();
        _GQtDraw_CallEHChecked(nw.get(), &e, eh);
    }

    Log(LOGDEBUG, "Window created: %p[%p][%s][toplevel:%d]", nw.get(), nw->Widget(), nw->window_title, ret->is_toplevel);
    window.release();
    nw.release();
    return ret;
}

static GWindow _GQtDraw_NewPixmap(GDisplay *disp, GWindow similar, uint16 width, uint16 height, bool is_bitmap,
                                   const unsigned char *data) {
    std::unique_ptr<GQtWindow> gw(new GQtWindow());
    std::unique_ptr<QPixmap> pixmap(new QPixmap(width, height));
    GWindow ret = gw->Base();
    QImage::Format format;
    int stride;

    ret->ggc = _GQtDraw_NewGGC();
    ret->ggc->bg = _GDraw_res_bg;
    width &= 0x7fff;

    ret->native_window = gw.get();
    ret->display = disp;
    ret->is_pixmap = 1;
    ret->parent = nullptr;
    ret->pos.x = ret->pos.y = 0;
    ret->pos.width = width;
    ret->pos.height = height;
    gw->q_base = pixmap.get();

    if (data != nullptr) {
        if (is_bitmap) {
            QImage img(data, width, height, width / 8, QImage::Format_MonoLSB);
            QBitmap bm = QBitmap::fromImage(img);

            img.invertPixels();
            gw->Pixmap()->convertFromImage(img);
            gw->Pixmap()->setMask(bm);
        } else {
            gw->Pixmap()->convertFromImage(QImage(data, width, height, QImage::Format_ARGB32));
        }
    }

    pixmap.release();
    gw.release();
    return ret;
}

static void GQtDrawInit(GDisplay *disp) {
    disp->fontstate = new FState();
    // In inches, because that's how fonts are measured
    disp->fontstate->res = disp->res;
}

static void GQtDrawSetDefaultIcon(GWindow icon) {
    Log(LOGDEBUG, " ");
    assert(icon->is_pixmap);
    GQtD(icon)->default_icon = GQtW(icon);
}

static GWindow GQtDrawCreateTopWindow(GDisplay *disp, GRect *pos, int (*eh)(GWindow w, GEvent *), void *user_data,
                                       GWindowAttrs *gattrs) {
    Log(LOGDEBUG, " ");
    return _GQtDraw_CreateWindow(GQtD(disp), nullptr, pos, eh, user_data, gattrs);
}

static GWindow GQtDrawCreateSubWindow(GWindow w, GRect *pos, int (*eh)(GWindow w, GEvent *), void *user_data,
                                       GWindowAttrs *gattrs) {
    Log(LOGDEBUG, " ");
    return _GQtDraw_CreateWindow(GQtD(w), w, pos, eh, user_data, gattrs);
}

static GWindow GQtDrawCreatePixmap(GDisplay *disp, GWindow similar, uint16 width, uint16 height) {
    Log(LOGDEBUG, " ");
    return _GQtDraw_NewPixmap(disp, similar, width, height, false, nullptr);
}

static GWindow GQtDrawCreateBitmap(GDisplay *disp, uint16 width, uint16 height, uint8 *data) {
    Log(LOGDEBUG, " ");
    return _GQtDraw_NewPixmap(disp, nullptr, width, height, true, data);
}

static GCursor GQtDrawCreateCursor(GWindow src, GWindow mask, Color fg, Color bg, int16_t x, int16_t y) {
    Log(LOGDEBUG, " ");

    GQtDisplay *gdisp = GQtD(src);
    if (mask == nullptr) { // Use src directly
        assert(src != nullptr);
        assert(src->is_pixmap);
        gdisp->custom_cursors.emplace_back(*GQtW(src)->Pixmap(), x, y);
    } else { // Assume it's an X11-style cursor
        QPixmap pixmap(src->pos.width, src->pos.height);

        // Masking
        //Background
        QBitmap bgMask(*GQtW(mask)->Pixmap());
        QBitmap fgMask(*GQtW(src)->Pixmap());
        pixmap.setMask(bgMask);

        QPainter painter(&pixmap);
        painter.fillRect(pixmap.rect(), QBrush(QColor(bg)));
        painter.end();

        pixmap.setMask(QBitmap());
        pixmap.setMask(fgMask);
        painter.begin(&pixmap);
        painter.fillRect(pixmap.rect(), QBrush(QColor(fg)));
        painter.end();

        gdisp->custom_cursors.emplace_back(pixmap, x, y);
    }

    return (GCursor)(ct_user + (gdisp->custom_cursors.size() - 1));
}

static void GQtDrawDestroyCursor(GDisplay *disp, GCursor gcursor) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDestroyWindow(GWindow w) {
    Log(LOGDEBUG, " ");
}

static int GQtDrawNativeWindowExists(GDisplay *UNUSED(gdisp), void *native_window) {
    Log(LOGDEBUG, " ");
    return false;
}

static void GQtDrawSetZoom(GWindow UNUSED(gw), GRect *UNUSED(size), enum gzoom_flags UNUSED(flags)) {
    //Log(LOGDEBUG, " ");
    // Not implemented.
}

static void GQtDrawSetWindowBackground(GWindow w, Color gcol) {
    Log(LOGDEBUG, " ");
}

static int GQtDrawSetDither(GDisplay *UNUSED(gdisp), int UNUSED(set)) {
    // Not implemented; does nothing.
    return false;
}

static void GQtDrawSetVisible(GWindow w, int show) {
    Log(LOGDEBUG, "0x%p %d", w, show);
    GQtW(w)->Widget()->setVisible((bool)show);
}

static void GQtDrawMove(GWindow w, int32 x, int32 y) {
    Log(LOGDEBUG, "%p:%s, %d %d", w, GQtW(w)->window_title, x, y);
    GQtW(w)->Widget()->move(x, y);
}

static void GQtDrawTrueMove(GWindow w, int32 x, int32 y) {
    Log(LOGDEBUG, " ");
    GQtW(w)->Widget()->move(x, y);
}

static void GQtDrawResize(GWindow w, int32 width, int32 height) {
    Log(LOGDEBUG, "%p:%s, %d %d", w, GQtW(w)->window_title, width, height);
    GQtW(w)->Widget()->resize(width, height);
}

static void GQtDrawMoveResize(GWindow w, int32 x, int32 y, int32 width, int32 height) {
    Log(LOGDEBUG, "%p:%s, %d %d %d %d", w, GQtW(w)->window_title, x, y, width, height);
    GQtW(w)->Widget()->setGeometry(x, y, width, height);
}

static void GQtDrawRaise(GWindow w) {
    Log(LOGDEBUG, "%p", w, GQtW(w)->window_title);
    GQtW(w)->Widget()->raise();
}

// Icon title is ignored.
static void GQtDrawSetWindowTitles8(GWindow w, const char *title, const char *UNUSED(icontitle)) {
    Log(LOGDEBUG, " ");// assert(false);
    auto* gw = GQtW(w);
    gw->Widget()->setWindowTitle(QString::fromUtf8(title));
    gw->window_title = title;
}

static char *GQtDrawGetWindowTitle8(GWindow w) {
    Log(LOGDEBUG, " ");
    return copy(GQtW(w)->window_title.c_str());
}

static void GQtDrawSetTransientFor(GWindow transient, GWindow owner) {
    Log(LOGDEBUG, "transient=%p, owner=%p", transient, owner);
    assert(transient->is_toplevel);
    assert(owner->is_toplevel);

    QWidget *trans = GQtW(transient)->Widget();
    QWidget *parent = GQtW(owner)->Widget();
    Qt::WindowFlags flags = trans->windowFlags();
    bool visible = trans->isVisible();

    trans->setParent(parent);
    trans->setWindowFlags(flags);
    if (visible) {
        trans->show();
    }
}

static void GQtDrawGetPointerPosition(GWindow w, GEvent *ret) {
    Log(LOGDEBUG, " ");
    auto *gdisp = GQtD(w);
    Qt::KeyboardModifiers modifiers = gdisp->app->keyboardModifiers();
    QPoint pos = QCursor::pos();

    ret->u.mouse.x = pos.x();
    ret->u.mouse.y = pos.y();
    ret->u.mouse.state = _GQtDraw_QtModifierToKsm(modifiers);
}

static GWindow GQtDrawGetPointerWindow(GWindow w) {
    Log(LOGDEBUG, " ");
    auto *gdisp = GQtD(w);
    GQtWidget *widget = dynamic_cast<GQtWidget*>(gdisp->app->widgetAt(QCursor::pos()));
    if (widget) {
        return widget->gwindow->Base();
    }
    return nullptr;
}

static void GQtDrawSetCursor(GWindow w, GCursor gcursor) {
    Log(LOGDEBUG, " ");

    QCursor cursor;
    switch (gcursor) {
        case ct_default:
        case ct_backpointer:
        case ct_pointer:
            break;
        case ct_hand:
            cursor = QCursor(Qt::OpenHandCursor);
            break;
        case ct_question:
            cursor = QCursor(Qt::WhatsThisCursor);
            break;
        case ct_cross:
            cursor = QCursor(Qt::CrossCursor);
            break;
        case ct_4way:
            cursor = QCursor(Qt::SizeAllCursor);
            break;
        case ct_text:
            cursor = QCursor(Qt::IBeamCursor);
            break;
        case ct_watch:
            cursor = QCursor(Qt::WaitCursor);
            break;
        case ct_draganddrop:
            cursor = QCursor(Qt::DragMoveCursor);
            break;
        case ct_invisible:
            return; // There is no *good* reason to make the cursor invisible
            break;
        default:
            Log(LOGDEBUG, "CUSTOM CURSOR! %d", gcursor);
    }

    auto *gw = GQtW(w);
    if (gcursor >= ct_user) {
        GQtDisplay *gdisp = GQtD(w);
        gcursor = (GCursor)(gcursor - ct_user);
        if ((size_t)gcursor < gdisp->custom_cursors.size()) {
            gw->Widget()->setCursor(gdisp->custom_cursors[gcursor]);
            gw->current_cursor = (GCursor)(gcursor + ct_user);
        } else {
            Log(LOGWARN, "Invalid cursor value passed: %d", gcursor);
        }
    } else {
        gw->Widget()->setCursor(cursor);
        gw->current_cursor = gcursor;
    }
}

static GCursor GQtDrawGetCursor(GWindow w) {
    Log(LOGDEBUG, " ");
    return GQtW(w)->current_cursor;
}

static void GQtDrawTranslateCoordinates(GWindow from, GWindow to, GPoint *pt) {
    Log(LOGDEBUG, " ");

    GQtWindow *gfrom = GQtW(from), *gto = GQtW(to);
    QPoint res;

    if (to == from->display->groot) {
        // The actual meaning of this command...
        res = gfrom->Widget()->mapToGlobal(QPoint(pt->x, pt->y));
    } else {
        res = gfrom->Widget()->mapTo(gto->Widget(), QPoint(pt->x, pt->y));
    }

    pt->x = res.x();
    pt->y = res.y();
}

static void GQtDrawBeep(GDisplay *disp) {
    Log(LOGDEBUG, " ");
    GQtD(disp)->app->beep();
}

static void GQtDrawScroll(GWindow w, GRect *rect, int32 hor, int32 vert) {
    Log(LOGDEBUG, " ");
    GRect temp;

    vert = -vert;
    if (rect == nullptr) {
        temp.x = temp.y = 0;
        temp.width = w->pos.width;
        temp.height = w->pos.height;
        rect = &temp;
    }

    GDrawRequestExpose(w, rect, false);
}


static GIC *GQtDrawCreateInputContext(GWindow UNUSED(gw), enum gic_style UNUSED(style)) {
    Log(LOGDEBUG, " ");
    return nullptr;
}

static void GQtDrawSetGIC(GWindow UNUSED(gw), GIC *UNUSED(gic), int UNUSED(x), int UNUSED(y)) {
    Log(LOGDEBUG, " ");
}

static int GQtDrawKeyState(GWindow w, int keysym) {
    Log(LOGDEBUG, " ");
    if (keysym != ' ') {
        Log(LOGWARN, "Cannot check state of unsupported character!");
        return 0;
    }
    return 0;
    // Since this function is only used to check the state of the space button
    // Don't bother with a full implementation...
    // return ((GQtWindow)w)->display->is_space_pressed;
}

static void GQtDrawGrabSelection(GWindow w, enum selnames sn) {
    Log(LOGDEBUG, " ");

    if ((int)sn < 0 || sn >= sn_max) {
        return;
    }
}

static void GQtDrawAddSelectionType(GWindow w, enum selnames sel, char *type, void *data, int32 cnt, int32 unitsize,
                                     void *gendata(void *, int32 *len), void freedata(void *)) {
    Log(LOGDEBUG, " ");
}

static void *GQtDrawRequestSelection(GWindow w, enum selnames sn, char *type_name, int32 *len) {
    return nullptr;
}

static int GQtDrawSelectionHasType(GWindow w, enum selnames sn, char *type_name) {
    Log(LOGDEBUG, " ");
    return false;
}

static void GQtDrawBindSelection(GDisplay *disp, enum selnames sn, char *atomname) {
    Log(LOGDEBUG, " ");
}

static int GQtDrawSelectionHasOwner(GDisplay *disp, enum selnames sn) {
    Log(LOGDEBUG, " ");

    if ((int)sn < 0 || sn >= sn_max) {
        return false;
    }

    return false;
}

static void GQtDrawPointerUngrab(GDisplay *disp) {
    Log(LOGDEBUG, " ");

}

static void GQtDrawPointerGrab(GWindow w) {
    Log(LOGDEBUG, " ");
    // ((QWidget *)w->native_window)->grabMouse();
}

static void GQtDrawRequestExpose(GWindow w, GRect *rect, int UNUSED(doclear)) {
    Log(LOGDEBUG, "%p [%s]", w, GQtW(w)->window_title);

    // GQtWindow gw = (GQtWindow) w;
    // GdkRectangle clip;

    // if (!gw->is_visible || _GQtDraw_WindowOrParentsDying(gw)) {
    //     return;
    // }
    // if (rect == nullptr) {
    //     clip.x = clip.y = 0;
    //     clip.width = gw->pos.width;
    //     clip.height = gw->pos.height;
    // } else {
    //     clip.x = rect->x;
    //     clip.y = rect->y;
    //     clip.width = rect->width;
    //     clip.height = rect->height;

    //     if (rect->x < 0 || rect->y < 0 || rect->x + rect->width > gw->pos.width ||
    //             rect->y + rect->height > gw->pos.height) {

    //         if (clip.x < 0) {
    //             clip.width += clip.x;
    //             clip.x = 0;
    //         }
    //         if (clip.y < 0) {
    //             clip.height += clip.y;
    //             clip.y = 0;
    //         }
    //         if (clip.x + clip.width > gw->pos.width) {
    //             clip.width = gw->pos.width - clip.x;
    //         }
    //         if (clip.y + clip.height > gw->pos.height) {
    //             clip.height = gw->pos.height - clip.y;
    //         }
    //         if (clip.height <= 0 || clip.width <= 0) {
    //             return;
    //         }
    //     }
    // }

    // gdk_window_invalidate_rect(gw->w, &clip, false);
}

static void GQtDrawForceUpdate(GWindow w) {
    Log(LOGDEBUG, " ");
    GQtD(w)->app->processEvents();
}

static void GQtDrawSync(GDisplay *disp) {
    // Log(LOGDEBUG, " ");
}

static void GQtDrawSkipMouseMoveEvents(GWindow UNUSED(gw), GEvent *UNUSED(gevent)) {
    //Log(LOGDEBUG, " ");
    // Not implemented, not needed.
}

static void GQtDrawProcessPendingEvents(GDisplay *disp) {
    //Log(LOGDEBUG, " ");
    GQtD(disp)->app->processEvents();
}

static void GQtDrawProcessOneEvent(GDisplay *disp) {
    //Log(LOGDEBUG, " ");
    GQtD(disp)->app->processEvents(QEventLoop::WaitForMoreEvents);
}

static void GQtDrawEventLoop(GDisplay *disp) {
    Log(LOGDEBUG, " ");
    GQtD(disp)->app->exec();
}

static void GQtDrawPostEvent(GEvent *e) {
    //Log(LOGDEBUG, " ");
    GQtWindow *gw = GQtW(e->w);
    e->native_window = gw;
    _GQtDraw_CallEHChecked(gw, e, e->w->eh);
}

static void GQtDrawPostDragEvent(GWindow w, GEvent *mouse, enum event_type et) {
    Log(LOGDEBUG, " ");
}

static int GQtDrawRequestDeviceEvents(GWindow w, int devcnt, struct gdeveventmask *de) {
    Log(LOGDEBUG, " ");
    return 0; //Not sure how to handle... For tablets...
}

static int GQtDrawShortcutKeyMatches(const GEvent *e, unichar_t ch) {
    return false;
}

GQtTimer::GQtTimer(GQtWindow *parent, void *userdata)
    : QTimer(parent->Widget())
    , gtimer{parent->Base(), this, userdata}
{
}

static GTimer *GQtDrawRequestTimer(GWindow w, int32 time_from_now, int32 frequency, void *userdata) {
    Log(LOGDEBUG, " ");
    GQtTimer *timer = new GQtTimer(GQtW(w), userdata);
    if (frequency == 0) {
        timer->setSingleShot(true);
    }

    QObject::connect(timer, &QTimer::timeout, [timer, frequency]{
        GEvent e = {};

        // if (_GGDKDraw_WindowOrParentsDying((GGDKWindow)timer->owner)) {
        //     return;
        // }

        e.type = et_timer;
        e.w = timer->Base()->owner;
        e.native_window = GQtW(e.w);
        e.u.timer.timer = timer->Base();
        e.u.timer.userdata = timer->Base()->userdata;

        _GQtDraw_CallEHChecked(GQtW(e.w), &e, e.w->eh);
        if (frequency) {
            timer->setInterval(frequency);
        }
    });

    timer->setInterval(time_from_now);
    timer->start();
    return timer->Base();
}

static void GQtDrawCancelTimer(GTimer *timer) {
    Log(LOGDEBUG, " ");
    GQtTimer *gtimer = (GQtTimer*)timer->impl;
    gtimer->stop();
    gtimer->deleteLater();
}


// DRAW RELATED

static void GQtDrawPushClip(GWindow w, GRect *rct, GRect *old) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawPopClip(GWindow w, GRect *old) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDrawLine(GWindow w, int32 x, int32 y, int32 xend, int32 yend, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDrawArrow(GWindow w, int32 x, int32 y, int32 xend, int32 yend, int16_t arrows, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDrawRect(GWindow w, GRect *rect, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawFillRect(GWindow w, GRect *rect, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawFillRoundRect(GWindow w, GRect *rect, int radius, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDrawEllipse(GWindow w, GRect *rect, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawFillEllipse(GWindow w, GRect *rect, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDrawArc(GWindow w, GRect *rect, int32 sangle, int32 eangle, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDrawPoly(GWindow w, GPoint *pts, int16_t cnt, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawFillPoly(GWindow w, GPoint *pts, int16_t cnt, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDrawImage(GWindow w, GImage *image, GRect *src, int32 x, int32 y) {
    Log(LOGDEBUG, " ");
}

// What we really want to do is use the grey levels as an alpha channel
static void GQtDrawDrawGlyph(GWindow w, GImage *image, GRect *src, int32 x, int32 y) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDrawImageMagnified(GWindow w, GImage *image, GRect *src, int32 x, int32 y, int32 width, int32 height) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDrawPixmap(GWindow w, GWindow pixmap, GRect *src, int32 x, int32 y) {
    Log(LOGDEBUG, " ");
}

static enum gcairo_flags GQtDrawHasCairo(GWindow UNUSED(w)) {
    Log(LOGDEBUG, " ");
    return gc_all;
}

static void GQtDrawPathStartNew(GWindow w) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawPathClose(GWindow w) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawPathMoveTo(GWindow w, double x, double y) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawPathLineTo(GWindow w, double x, double y) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawPathCurveTo(GWindow w, double cx1, double cy1, double cx2, double cy2, double x, double y) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawPathStroke(GWindow w, Color col) {
    Log(LOGDEBUG, " ");
    w->ggc->fg = col;
}

static void GQtDrawPathFill(GWindow w, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawPathFillAndStroke(GWindow w, Color fillcol, Color strokecol) {
    Log(LOGDEBUG, " ");
    // This function is unused, so it's unclear if it's implemented correctly.
}

static void GQtDrawStartNewSubPath(GWindow w) {
    Log(LOGDEBUG, " ");
}

static int GQtDrawFillRuleSetWinding(GWindow w) {
    Log(LOGDEBUG, " ");
    return 1;
}

static int GQtDrawDoText8(GWindow w, int32 x, int32 y, const char *text, int32 cnt, Color col, enum text_funcs drawit,
                    struct tf_arg *arg) {
    Log(LOGDEBUG, " ");
    return 0;
}

static void GQtDrawPushClipOnly(GWindow w) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawClipPreserve(GWindow w) {
    Log(LOGDEBUG, " ");
}

// PANGO LAYOUT
static void GQtDrawGetFontMetrics(GWindow w, GFont *fi, int *as, int *ds, int *ld) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawLayoutDraw(GWindow w, int32 x, int32 y, Color fg) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawLayoutIndexToPos(GWindow w, int index, GRect *pos) {
    Log(LOGDEBUG, " ");
    *pos = {0};
}

static int GQtDrawLayoutXYToIndex(GWindow w, int x, int y) {
    Log(LOGDEBUG, " ");
    return 0;
}

static void GQtDrawLayoutExtents(GWindow w, GRect *size) {
    Log(LOGDEBUG, " ");
    *size = {0};
}

static void GQtDrawLayoutSetWidth(GWindow w, int width) {
    Log(LOGDEBUG, " ");
}

static int GQtDrawLayoutLineCount(GWindow w) {
    Log(LOGDEBUG, " ");
    return 0;
}

static int GQtDrawLayoutLineStart(GWindow w, int l) {
    Log(LOGDEBUG, " ");
    return 0;
}
// END PANGO LAYOUT


// END DRAW RELATED

}} // fontforge::gdraw

using namespace fontforge::gdraw;

// Our function VTable
static struct displayfuncs gqtfuncs = {
    GQtDrawInit,

    GQtDrawSetDefaultIcon,

    GQtDrawCreateTopWindow,
    GQtDrawCreateSubWindow,
    GQtDrawCreatePixmap,
    GQtDrawCreateBitmap,
    GQtDrawCreateCursor,
    GQtDrawDestroyWindow,
    GQtDrawDestroyCursor,
    GQtDrawNativeWindowExists, //Not sure what this is meant to do...
    GQtDrawSetZoom,
    GQtDrawSetWindowBackground,
    GQtDrawSetDither,

    GQtDrawSetVisible,
    GQtDrawMove,
    GQtDrawTrueMove,
    GQtDrawResize,
    GQtDrawMoveResize,
    GQtDrawRaise,
    GQtDrawSetWindowTitles8,
    GQtDrawGetWindowTitle8,
    GQtDrawSetTransientFor,
    GQtDrawGetPointerPosition,
    GQtDrawGetPointerWindow,
    GQtDrawSetCursor,
    GQtDrawGetCursor,
    GQtDrawTranslateCoordinates,

    GQtDrawBeep,

    GQtDrawPushClip,
    GQtDrawPopClip,

    GQtDrawDrawLine,
    GQtDrawDrawArrow,
    GQtDrawDrawRect,
    GQtDrawFillRect,
    GQtDrawFillRoundRect,
    GQtDrawDrawEllipse,
    GQtDrawFillEllipse,
    GQtDrawDrawArc,
    GQtDrawDrawPoly,
    GQtDrawFillPoly,
    GQtDrawScroll,

    GQtDrawDrawImage,
    GQtDrawDrawGlyph,
    GQtDrawDrawImageMagnified,
    GQtDrawDrawPixmap,

    GQtDrawCreateInputContext,
    GQtDrawSetGIC,
    GQtDrawKeyState,

    GQtDrawGrabSelection,
    GQtDrawAddSelectionType,
    GQtDrawRequestSelection,
    GQtDrawSelectionHasType,
    GQtDrawBindSelection,
    GQtDrawSelectionHasOwner,

    GQtDrawPointerUngrab,
    GQtDrawPointerGrab,
    GQtDrawRequestExpose,
    GQtDrawForceUpdate,
    GQtDrawSync,
    GQtDrawSkipMouseMoveEvents,
    GQtDrawProcessPendingEvents,
    GQtDrawProcessOneEvent,
    GQtDrawEventLoop,
    GQtDrawPostEvent,
    GQtDrawPostDragEvent,
    GQtDrawRequestDeviceEvents,
    GQtDrawShortcutKeyMatches,

    GQtDrawRequestTimer,
    GQtDrawCancelTimer,

    GQtDrawGetFontMetrics,

    GQtDrawHasCairo,
    GQtDrawPathStartNew,
    GQtDrawPathClose,
    GQtDrawPathMoveTo,
    GQtDrawPathLineTo,
    GQtDrawPathCurveTo,
    GQtDrawPathStroke,
    GQtDrawPathFill,
    GQtDrawPathFillAndStroke, // Currently unused

    GQtDrawLayoutInit,
    GQtDrawLayoutDraw,
    GQtDrawLayoutIndexToPos,
    GQtDrawLayoutXYToIndex,
    GQtDrawLayoutExtents,
    GQtDrawLayoutSetWidth,
    GQtDrawLayoutLineCount,
    GQtDrawLayoutLineStart,
    GQtDrawStartNewSubPath,
    GQtDrawFillRuleSetWinding,

    GQtDrawDoText8,

    GQtDrawPushClipOnly,
    GQtDrawClipPreserve
};

extern "C" GDisplay *_GQtDraw_CreateDisplay(char *displayname, int *argc, char ***argv) {
    LogInit();

    std::unique_ptr<GQtDisplay> gdisp(new GQtDisplay());
    gdisp->app.reset(new QApplication(*argc, *argv));

    GDisplay *ret = gdisp->Base();
    ret->impl = gdisp.get();
    ret->funcs = &gqtfuncs;

    std::unique_ptr<GQtWindow> groot(new GQtWindow());
    QRect screenGeom = gdisp->app->primaryScreen()->geometry();

    ret->res = gdisp->app->primaryScreen()->logicalDotsPerInch();

    ret->groot = groot->Base();
    ret->groot->ggc = _GQtDraw_NewGGC();
    ret->groot->display = (GDisplay*)gdisp.get();
    ret->groot->native_window = groot.get();
    ret->groot->pos.width = screenGeom.width();
    ret->groot->pos.height = screenGeom.height();
    ret->groot->is_toplevel = true;
    ret->groot->is_visible = true;

    (ret->funcs->init)(ret);
    _GDraw_InitError(ret);

    groot.release();
    gdisp.release();
    return ret;
}

extern "C" void _GQtDraw_DestroyDisplay(GDisplay *disp) {
    delete GQtD(disp);
}

#endif // FONTFORGE_CAN_USE_QT
