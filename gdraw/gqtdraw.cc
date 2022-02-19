/* Copyright (C) 2016-2022 by Jeremy Tan */
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

/**
 * TODO
 * Unbreak cursors
 * Window lifecycle
 * Cairo/Pango
 * Text layout
 * Clipboard
 * GIC/input context
 */


namespace fontforge { namespace gdraw {

// Forward declarations
static void GQtDrawCancelTimer(GTimer *timer);
static void GQtDrawDestroyWindow(GWindow w);
static void GQtDrawPostEvent(GEvent *e);
static void GQtDrawProcessPendingEvents(GDisplay *disp);
static void GQtDrawSetCursor(GWindow w, GCursor gcursor);
static void GQtDrawSetTransientFor(GWindow transient, GWindow owner);
static void GQtDrawSetWindowBackground(GWindow w, Color gcol);
static void GQtDrawSetWindowTitles8(GWindow w, const char *title, const char *UNUSED(icontitle));

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

static int _GQtDraw_WindowOrParentsDying(GWindow w) {
    while (w != nullptr) {
        if (w->is_dying) {
            return true;
        }
        if (w->is_toplevel) {
            return false;
        }
        w = w->parent;
    }
    return false;
}

static GWindow _GQtDraw_CreateWindow(GQtDisplay *gdisp, GWindow w, GRect *pos,
                                      int (*eh)(GWindow, GEvent *), void *user_data, GWindowAttrs *wattrs) {

    Qt::WindowFlags windowFlags = Qt::Widget;
    QWidget *parent = w ? GQtW(w)->Widget() : nullptr;

    if (wattrs == nullptr) {
        static GWindowAttrs temp = GWINDOWATTRS_EMPTY;
        wattrs = &temp;
    }
    if (w == nullptr) { // Creating a top-level window. Set parent as default root.
        windowFlags |= Qt::Window;
        if ((wattrs->mask & wam_nodecor) && wattrs->nodecoration) {
            // Is a modeless dialogue
            windowFlags |= Qt::Popup; // hmm
        } else if ((wattrs->mask & wam_isdlg) && wattrs->is_dlg) {
            windowFlags |= Qt::Dialog;
        }
        // if (ret->is_popup || (wattrs->mask & wam_palette)) {
        //     // windowFlags |= Qt::ToolTip;
        // }
    }

    std::unique_ptr<GQtWidget> window(new GQtWidget(parent, windowFlags));
    GWindow ret = window->Base();
    ret->native_window = static_cast<GQtWindow*>(window.get());
    ret->is_toplevel = w == nullptr;
    ret->is_popup = windowFlags & Qt::Popup;
    ret->is_dlg = windowFlags & (Qt::Popup | Qt::Dialog);
    ret->display = gdisp->Base();
    ret->eh = eh;
    ret->parent = w;
    ret->pos = *pos;
    ret->user_data = user_data;

    if (wattrs->mask & wam_restrict) {
        ret->restrict_input_to_me = wattrs->restrict_input_to_me;
        window->setWindowModality(Qt::ApplicationModal);
    }
    if (ret->is_toplevel) {
        // Icon titles are ignored.
        if ((wattrs->mask & wam_utf8_wtitle) && (wattrs->utf8_window_title != nullptr)) {
            window->setWindowTitle(QString::fromUtf8(wattrs->utf8_window_title));
            window->SetTitle(wattrs->utf8_window_title);
        }
    }

    window->resize(pos->width, pos->height);
    window->setFocusPolicy(Qt::StrongFocus);
    if (wattrs->mask & wam_events) {
        if (wattrs->event_masks & (1 << et_mousemove)) {
            window->setMouseTracking(true);
        }
    }

    if (!ret->is_toplevel || ((wattrs->mask & wam_positioned) && !(wattrs->mask & wam_centered))) {
        window->move(ret->pos.x, ret->pos.y);
    }

    // Set background
    if (!(wattrs->mask & wam_backcol) || wattrs->background_color == COLOR_DEFAULT) {
        wattrs->background_color = _GDraw_res_bg;
    }
    ret->ggc->bg = wattrs->background_color;
    GQtDrawSetWindowBackground(ret, ret->ggc->bg);

    if (ret->is_toplevel) {
        // Icon titles are ignored.
        if ((wattrs->mask & wam_utf8_wtitle) && (wattrs->utf8_window_title != nullptr)) {
            GQtDrawSetWindowTitles8(ret, wattrs->utf8_window_title, nullptr);
        }

        // Set icon
        GQtWindow *icon = gdisp->default_icon;
        if (((wattrs->mask & wam_icon) && wattrs->icon != nullptr) && wattrs->icon->is_pixmap) {
            icon = GQtW(wattrs->icon);
        }
        if (icon != nullptr) {
            window->setWindowIcon(QIcon(*icon->Pixmap()));
        } else {
            // Qt_window_set_decorations(nw->w, Qt_DECOR_ALL | Qt_DECOR_MENU);
        }

        if (wattrs->mask & wam_palette) {
            window->setBaseSize(window->size());
            window->setMinimumSize(window->size());
        }
        if ((wattrs->mask & wam_noresize) && wattrs->noresize) {
            window->setFixedSize(window->size());
        }
        ret->was_positioned = true;

        if ((wattrs->mask & wam_transient) && wattrs->transient != nullptr) {
            GQtDrawSetTransientFor(ret, wattrs->transient);
            ret->is_dlg = true;
        } else if (!ret->is_dlg) {
            ++gdisp->top_window_count;
        }
        // else if (ret->restrict_input_to_me) {
        //     GQtDrawSetTransientFor(ret, (GWindow) - 1);
        // }
    }

    if ((wattrs->mask & wam_cursor) && wattrs->cursor != ct_default) {
        GQtDrawSetCursor(ret, wattrs->cursor);
    }

    // Event handler
    if (eh != nullptr) {
        window->DispatchEvent(window->InitEvent<>(et_create));
    }

    Log(LOGDEBUG, "Window created: %p[%p][%s][toplevel:%d]",
        ret, ret->native_window, window->Title(), ret->is_toplevel);
    window.release();
    return ret;
}

static GWindow _GQtDraw_NewPixmap(GDisplay *disp, GWindow similar, uint16 width, uint16 height, bool is_bitmap,
                                   const unsigned char *data) {
    std::unique_ptr<GQtPixmap> pixmap(new GQtPixmap(width, height));
    GWindow ret = pixmap->Base();
    QImage::Format format;
    int stride;

    width &= 0x7fff;
    ret->ggc->bg = _GDraw_res_bg;
    ret->native_window = static_cast<GQtWindow*>(pixmap.get());
    ret->display = disp;
    ret->is_pixmap = 1;
    ret->parent = nullptr;
    ret->pos.x = ret->pos.y = 0;
    ret->pos.width = width;
    ret->pos.height = height;

    if (data != nullptr) {
        if (is_bitmap) {
            QImage img(data, width, height, width / 8, QImage::Format_MonoLSB);
            QBitmap bm = QBitmap::fromImage(img);
            pixmap->fill(Qt::transparent);

            QPainter painter(pixmap.get());
            painter.setPen(Qt::black);
            painter.setBackgroundMode(Qt::TransparentMode);
            painter.drawPixmap(0, 0, bm);
        } else {
            pixmap->convertFromImage(QImage(data, width, height, QImage::Format_ARGB32));
        }
    }

    pixmap.release();
    return ret;
}

GQtWidget::~GQtWidget()
{
    Log(LOGWARN, "CLEANUP [%p] [%p] [%s]", Base(), this, Title());
    DispatchEvent(InitEvent<>(et_destroy));
}

template<typename E>
void UpdateLastEventTime(GWindow w, E *event) {}
template<typename E, typename std::enable_if<std::is_integral<decltype(E::timestamp())>::value>::type* = nullptr>
void UpdateLastEventTime(GWindow w, E *event) {
    GQtD(w)->last_event_time = event->timestamp();
}

template<typename E = void>
GEvent GQtWidget::InitEvent(event_type et, E *event) {
    GEvent gevent = {};
    gevent.w = Base();
    gevent.native_window = static_cast<GQtWindow*>(this);
    gevent.type = et;
    UpdateLastEventTime(gevent.w, event);
    return gevent;
}

void GQtWidget::DispatchEvent(const GEvent& e) {
    if (e.w->eh && e.type != et_noevent && (!e.w->is_dying || e.type == et_destroy)) {
        (e.w->eh)(e.w, const_cast<GEvent*>(&e)); //yuck
    }
}

QPainter* GQtWidget::Painter() {
    if (!m_painter) {
        static QPainter sPainter;
        Log(LOGWARN, "[%p] [%s] that's a paddlin", Base(), Title());
        return &sPainter;
    }
    return m_painter;
}

void GQtWidget::keyEvent(QKeyEvent *event, event_type et) {
    GEvent gevent = InitEvent(et, event);
    gevent.u.chr.state = _GQtDraw_QtModifierToKsm(event->modifiers());
    gevent.u.chr.autorepeat = event->isAutoRepeat();

    if (gevent.u.chr.autorepeat && GKeysymIsModifier(event->key())) {
        return;
    } else {
        if (event->key() == Qt::Key_Space) {
            GQtD(gevent.w)->is_space_pressed = et == et_char;
        }

        gevent.u.chr.keysym = event->key();
        QString text = event->text();
        const QChar* uni = text.unicode();
        for (size_t i = 0, j = 0; i < text.size() && j< (_GD_EVT_CHRLEN-1); ++i) {
            if (uni[i].isHighSurrogate() && (i+1) < text.size() &&
                uni[i+1].isLowSurrogate()) {
                gevent.u.chr.chars[j++] = QChar::surrogateToUcs4(uni[i], uni[i+1]);
                ++i;
            } else {
                gevent.u.chr.chars[j++] = uni[i].unicode();
            }
        }
    }
    DispatchEvent(gevent);
}

void GQtWidget::paintEvent(QPaintEvent *event) {
    Log(LOGDEBUG, "PAINTING %p %s", Base(), Title());

    const QRect& rect = event->rect();
    GEvent gevent = InitEvent(et_expose, event);
    gevent.u.expose.rect.x = rect.x();
    gevent.u.expose.rect.y = rect.y();
    gevent.u.expose.rect.width = rect.width();
    gevent.u.expose.rect.height = rect.height();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::HighQualityAntialiasing);

    m_painter = &painter;
    DispatchEvent(gevent);
    m_painter = nullptr;
}

void GQtWidget::configureEvent() {
    GEvent gevent = InitEvent<>(et_resize);
    auto geom = geometry();

    gevent.u.resize.size.x      = geom.x();
    gevent.u.resize.size.y      = geom.y();
    gevent.u.resize.size.width  = geom.width();
    gevent.u.resize.size.height = geom.height();
    gevent.u.resize.dx          = geom.x() - gevent.w->pos.x;
    gevent.u.resize.dy          = geom.y() - gevent.w->pos.y;
    gevent.u.resize.dwidth      = geom.width() - gevent.w->pos.width;
    gevent.u.resize.dheight     = geom.height() - gevent.w->pos.height;
    gevent.u.resize.moved       = gevent.u.resize.sized = false;
    if (gevent.u.resize.dx != 0 || gevent.u.resize.dy != 0) {
        gevent.u.resize.moved = true;
    }
    if (gevent.u.resize.dwidth != 0 || gevent.u.resize.dheight != 0) {
        gevent.u.resize.sized = true;
    }

    gevent.w->pos = gevent.u.resize.size;

    // I could make this Windows specific... But it doesn't seem necessary on other platforms too.
    // On Windows, repeated configure messages are sent if we move the window around.
    // This causes CPU usage to go up because mouse handlers of this message just redraw the whole window.
    if (gevent.w->is_toplevel && !gevent.u.resize.sized && gevent.u.resize.moved) {
        Log(LOGDEBUG, "Configure DISCARDED: %p:%s, %d %d %d %d", gevent.w, Title(), gevent.w->pos.x, gevent.w->pos.y, gevent.w->pos.width, gevent.w->pos.height);
        return;
    } else {
        Log(LOGDEBUG, "CONFIGURED: %p:%s, %d %d %d %d", gevent.w, Title(), gevent.w->pos.x, gevent.w->pos.y, gevent.w->pos.width, gevent.w->pos.height);
    }

    DispatchEvent(gevent);
}

void GQtWidget::mouseEvent(QMouseEvent* event, event_type et) {
    GEvent gevent = InitEvent(et, event);
    gevent.u.mouse.state = _GQtDraw_QtModifierToKsm(event->modifiers());
    gevent.u.mouse.x = event->x();
    gevent.u.mouse.y = event->y();
    switch (event->button())
    {
    case Qt::LeftButton:
        gevent.u.mouse.button = 1;
        break;
    case Qt::MiddleButton:
        gevent.u.mouse.button = 2;
        break;
    case Qt::RightButton:
        gevent.u.mouse.button = 3;
        break;
    default:
        return;
    }

    GQtDisplay *gdisp = GQtD(gevent.w);
    if (gevent.type == et_mousedown) {
        int xdiff = abs(gevent.u.mouse.x - gdisp->bs.release_x);
        int ydiff = abs(gevent.u.mouse.y - gdisp->bs.release_y);

        if (xdiff + ydiff < gdisp->bs.double_wiggle &&
                this == gdisp->bs.release_w &&
                gevent.u.mouse.button == gdisp->bs.release_button &&
                (int32_t)(gevent.u.mouse.time - gdisp->bs.last_press_time) < gdisp->bs.double_time &&
                gevent.u.mouse.time >= gdisp->bs.last_press_time) {  // Time can wrap

            gdisp->bs.cur_click++;
        } else {
            gdisp->bs.cur_click = 1;
        }
        gdisp->bs.last_press_time = gevent.u.mouse.time;
    } else {
        gdisp->bs.release_w = this;
        gdisp->bs.release_x = gevent.u.mouse.x;
        gdisp->bs.release_y = gevent.u.mouse.y;
        gdisp->bs.release_button = gevent.u.mouse.button;
    }

    gevent.u.mouse.clicks = gdisp->bs.cur_click;
    gevent.u.mouse.time = event->timestamp();

    DispatchEvent(gevent);
    // Log(LOGDEBUG, "Button %7s: [%f %f]", evt->type == GDK_BUTTON_PRESS ? "press" : "release", evt->x, evt->y);
}

void GQtWidget::wheelEvent(QWheelEvent *event) {
    GEvent gevent = InitEvent(et_mousedown, event);
    GQtDisplay *gdisp = GQtD(gevent.w);

    gevent.u.mouse.state = _GQtDraw_QtModifierToKsm(event->modifiers());
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    gevent.u.mouse.x = event->x();
    gevent.u.mouse.y = event->y();
#else
    gevent.u.mouse.x = event->position().x();
    gevent.u.mouse.y = event->position().y();
#endif
    gevent.u.mouse.clicks = gdisp->bs.cur_click = 1;

    QPoint angle = event->angleDelta();
    if (angle.y() != 0) {
        gevent.u.mouse.button = angle.y() < 0 ? 5 : 4;
    } else {
        gevent.u.mouse.button = angle.x() < 0 ? 6 : 7;
    }
    // We need to simulate two events... I think.
    DispatchEvent(gevent);
    gevent.type = et_mouseup;
    DispatchEvent(gevent);
}

void GQtWidget::mouseMoveEvent(QMouseEvent *event) {
    GEvent gevent = InitEvent(et_mousemove, event);
    gevent.u.mouse.state = _GQtDraw_QtModifierToKsm(event->modifiers());
    gevent.u.mouse.x = event->x();
    gevent.u.mouse.y = event->y();
    DispatchEvent(gevent);
}

void GQtWidget::mapEvent(bool visible) {
    GEvent gevent = InitEvent(et_map);
    gevent.u.map.is_visible = visible;
    gevent.w->is_visible = visible;
    DispatchEvent(gevent);
}

void GQtWidget::focusEvent(QFocusEvent* event, bool focusIn) {
    GEvent gevent = InitEvent(et_focus, event);
    gevent.u.focus.gained_focus = focusIn;
    gevent.u.focus.mnemonic_focus = false;

    switch (event->reason()) {
    case Qt::TabFocusReason:
        gevent.u.focus.mnemonic_focus = mf_tab;
        break;
    case Qt::ShortcutFocusReason:
        gevent.u.focus.mnemonic_focus = mf_shortcut;
    default:
        break;
    }

    Log(LOGDEBUG, "Focus change %s %p(%s %d)",
        gevent.u.focus.gained_focus ? "IN" : "OUT",
        Base(), Title(), gevent.w->is_toplevel);

    DispatchEvent(gevent);
}

void GQtWidget::crossingEvent(QEvent* event, bool enter) {
    // Why QEvent?!?
    GEvent gevent = InitEvent(et_crossing, event);
    QPoint pos = QCursor::pos();
    GQtDisplay *gdisp = GQtD(gevent.w);
    gevent.u.crossing.x = pos.x();
    gevent.u.crossing.y = pos.y();
    gevent.u.crossing.state = _GQtDraw_QtModifierToKsm(gdisp->app->keyboardModifiers());
    gevent.u.crossing.entered = enter;
    gevent.u.crossing.device = NULL;
    gevent.u.crossing.time = gdisp->last_event_time;
    // Always set to false when crossing boundary...
    gdisp->is_space_pressed = false;
    DispatchEvent(gevent);
}

void GQtWidget::closeEvent(QCloseEvent *event) {
    GEvent gevent = InitEvent(et_close, event);
    event->ignore();
    DispatchEvent(gevent);
}

static void GQtDrawInit(GDisplay *disp) {
    // Log(LOGDEBUG, " ");
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
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setPen(QColor(0xff000000 | bg));
        painter.setBackgroundMode(Qt::TransparentMode);
        painter.drawPixmap(0, 0, GQtW(mask)->Pixmap()->mask());

        painter.setPen(QColor(0xff000000 | fg));
        painter.drawPixmap(0, 0, GQtW(src)->Pixmap()->mask());
        painter.end();

        gdisp->custom_cursors.emplace_back(pixmap, x, y);
    }

    return (GCursor)(ct_user + (gdisp->custom_cursors.size() - 1));
}

static void GQtDrawDestroyCursor(GDisplay *disp, GCursor gcursor) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawDestroyWindow(GWindow w) {
    Log(LOGWARN, " ");
    GQtWindow *gw = GQtW(w);
    w->is_dying = true;
    if (w->is_pixmap) {
        delete gw;
    } else {
        GQtDisplay *gdisp = GQtD(w);
        auto children = gw->Widget()->findChildren<GQtWidget*>();
        for (auto* child : children) {
            if (child->Base()->is_toplevel) {
                Log(LOGWARN, "Orphaned toplevel [%p] [%s]",
                    child->Base(), child->Title());
                // child->setVisible(false);
                child->setParent(nullptr);
                child->Widget()->DispatchEvent(child->Widget()->InitEvent(et_close));
            } else {
                GQtDrawDestroyWindow(child->Base());
            }
        }

        gw->Widget()->setDisabled(true);
        gw->Widget()->deleteLater();
        // if (gdisp->grabbed_window == gw) {
        //     gw->Widget()->releaseMouse();
        //     gdisp->grabbed_window = nullptr;
        // }
    }
}

static int GQtDrawNativeWindowExists(GDisplay *UNUSED(gdisp), void *native_window) {
    Log(LOGDEBUG, " ");
    return native_window != nullptr;
}

static void GQtDrawSetZoom(GWindow UNUSED(gw), GRect *UNUSED(size), enum gzoom_flags UNUSED(flags)) {
    //Log(LOGDEBUG, " ");
    // Not implemented.
}

static void GQtDrawSetWindowBackground(GWindow w, Color gcol) {
    Log(LOGDEBUG, " ");
    GQtWindow *gw = GQtW(w);
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, QColor(gcol));
    gw->Widget()->setAutoFillBackground(true);
    gw->Widget()->setPalette(pal);
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
    Log(LOGDEBUG, "%p:%s, %d %d", w, GQtW(w)->Title(), x, y);
    GQtW(w)->Widget()->move(x, y);
}

static void GQtDrawTrueMove(GWindow w, int32 x, int32 y) {
    Log(LOGDEBUG, " ");
    GQtW(w)->Widget()->move(x, y);
}

static void GQtDrawResize(GWindow w, int32 width, int32 height) {
    Log(LOGDEBUG, "%p:%s, %d %d", w, GQtW(w)->Title(), width, height);
    GQtW(w)->Widget()->resize(width, height);
}

static void GQtDrawMoveResize(GWindow w, int32 x, int32 y, int32 width, int32 height) {
    Log(LOGDEBUG, "%p:%s, %d %d %d %d", w, GQtW(w)->Title(), x, y, width, height);
    GQtW(w)->Widget()->setGeometry(x, y, width, height);
}

static void GQtDrawRaise(GWindow w) {
    Log(LOGDEBUG, "%p", w, GQtW(w)->Title());
    GQtW(w)->Widget()->raise();
}

// Icon title is ignored.
static void GQtDrawSetWindowTitles8(GWindow w, const char *title, const char *UNUSED(icontitle)) {
    Log(LOGDEBUG, " ");// assert(false);
    auto* gw = GQtW(w);
    gw->Widget()->setWindowTitle(QString::fromUtf8(title));
    gw->SetTitle(title);
}

static char *GQtDrawGetWindowTitle8(GWindow w) {
    Log(LOGDEBUG, " ");
    return copy(GQtW(w)->Title());
}

static void GQtDrawSetTransientFor(GWindow transient, GWindow owner) {
    Log(LOGDEBUG, "transient=%p, owner=%p", transient, owner);
    if (owner == (GWindow)-1) {
        auto* aw = dynamic_cast<GQtWidget*>(GQtD(transient)->app->activeWindow());
        while (aw && (!aw->Base()->is_toplevel || aw->Base()->istransient)) {
            aw = dynamic_cast<GQtWidget*>(aw->parentWidget());
        }
        if (!aw || aw->Base() == GQtD(transient)->Base()->groot) {
            return;
        }
        owner = aw->Base();
    }
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
        return widget->Base();
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
            gw->SetCursor((GCursor)(gcursor + ct_user));
        } else {
            Log(LOGWARN, "Invalid cursor value passed: %d", gcursor);
        }
    } else {
        gw->Widget()->setCursor(cursor);
        gw->SetCursor(gcursor);
    }
}

static GCursor GQtDrawGetCursor(GWindow w) {
    Log(LOGDEBUG, " ");
    return GQtW(w)->Cursor();
}

static void GQtDrawTranslateCoordinates(GWindow from, GWindow to, GPoint *pt) {
    Log(LOGDEBUG, " ");

    GQtWindow *gfrom = GQtW(from), *gto = GQtW(to);
    QPoint res;

    if (to == from->display->groot) {
        // The actual meaning of this command...
        res = gfrom->Widget()->mapToGlobal(QPoint(pt->x, pt->y));
    } else {
        res = gfrom->Widget()->mapToGlobal(QPoint(pt->x, pt->y));
        res = gto->Widget()->mapFromGlobal(res);
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
    // GQtDisplay *gdisp = GQtD(disp);
    // if (gdisp->grabbed_window != nullptr) {
    //     gdisp->grabbed_window->Widget()->releaseMouse();
    // }
}

static void GQtDrawPointerGrab(GWindow w) {
    Log(LOGDEBUG, " ");
    // GQtDisplay *gdisp = GQtD(w);
    // GQtDrawPointerUngrab(gdisp->Base());
    // gdisp->grabbed_window = GQtW(w);
    // gdisp->grabbed_window->Widget()->grabMouse();
}

static void GQtDrawRequestExpose(GWindow w, GRect *rect, int UNUSED(doclear)) {
    Log(LOGDEBUG, "%p [%s]", w, GQtW(w)->Title());

    GQtWindow *gw = GQtW(w);

    if (!w->is_visible || _GQtDraw_WindowOrParentsDying(gw->Base())) {
        return;
    }
    if (rect == nullptr) {
        gw->Widget()->update();
    } else {
        const GRect &pos = gw->Base()->pos;
        QRect clip;
        clip.setX(rect->x);
        clip.setY(rect->y);
        clip.setWidth(rect->width);
        clip.setHeight(rect->height);

        if (rect->x < 0 || rect->y < 0 || rect->x + rect->width > pos.width ||
                rect->y + rect->height > pos.height) {

            if (clip.x() < 0) {
                clip.setWidth(clip.width() + clip.x());
                clip.setX(0);
            }
            if (clip.y() < 0) {
                clip.setHeight(clip.height() + clip.y());
                clip.setY(0);
            }
            if (clip.x() + clip.width() > pos.width) {
                clip.setWidth(pos.width - clip.x());
            }
            if (clip.y() + clip.height() > pos.height) {
                clip.setHeight(pos.height - clip.y());
            }
            if (clip.height() <= 0 || clip.width() <= 0) {
                return;
            }
        }
        gw->Widget()->update(clip);
    }
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
    e->native_window = GQtW(e->w);
    GQtW(e->w)->Widget()->DispatchEvent(*e);
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
    , m_gtimer{parent->Base(), this, userdata}
{
}

static GTimer *GQtDrawRequestTimer(GWindow w, int32 time_from_now, int32 frequency, void *userdata) {
    Log(LOGDEBUG, " ");
    GQtTimer *timer = new GQtTimer(GQtW(w), userdata);
    if (frequency == 0) {
        timer->setSingleShot(true);
    }

    QObject::connect(timer, &QTimer::timeout, [timer, frequency]{
        GQtWindow *gw = GQtW(timer->Base()->owner);

        Log(LOGDEBUG, "Timer fired [%p][%s]", gw->Base(), gw->Title());
        if (_GQtDraw_WindowOrParentsDying(gw->Base())) {
            return;
        }

        GEvent gevent = gw->Widget()->InitEvent<>(et_timer);
        gevent.u.timer.timer = timer->Base();
        gevent.u.timer.userdata = timer->Base()->userdata;
        gw->Widget()->DispatchEvent(gevent);
        if (frequency) {
            if (timer->interval() != frequency) {
                timer->setInterval(frequency);
            }
        } else {
            assert(timer->isSingleShot());
            timer->deleteLater();
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


static QBrush GQtDraw_StippleMePink(int ts, Color fg) {
    static const unsigned char grey_init[8] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
    static const unsigned char fence_init[8] = {0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55, 0x88};
    const uint8 *spt;
    int bit, i, j;
    uint32 *data;
    uint32 space[8 * 8];

    if ((fg >> 24) != 0xff) {
        int alpha = fg >> 24, r = COLOR_RED(fg), g = COLOR_GREEN(fg), b = COLOR_BLUE(fg);
        r = (alpha * r + 128) / 255;
        g = (alpha * g + 128) / 255;
        b = (alpha * b + 128) / 255;
        fg = (alpha << 24) | (r << 16) | (g << 8) | b;
    }

    spt = (ts == 2) ? fence_init : grey_init;
    for (i = 0; i < 8; ++i) {
        data = space + 8 * i;
        for (j = 0, bit = 0x80; bit != 0; ++j, bit >>= 1) {
            if (spt[i]&bit) {
                data[j] = fg;
            } else {
                data[j] = 0;
            }
        }
    }

    QImage pattern((const unsigned char*)space, 8, 8, QImage::Format_ARGB32_Premultiplied);
    // QBrush brush(pattern);
    QBrush brush((ts == 2) ? Qt::Dense5Pattern : Qt::Dense4Pattern);
    brush.setColor(fg);
    return brush;
}

static QImage _GQtDraw_GImage2QImage(GImage *image, GRect *src) {
    const struct _GImage *base = (image->list_len == 0) ? image->u.image : image->u.images[0];
    QImage ret;

    if (base->image_type == it_rgba) {
        ret = QImage(base->data + (src->y * base->bytes_per_line), src->width, src->height, base->bytes_per_line, QImage::Format_ARGB32);
    } else if (base->image_type == it_true) {
        ret = QImage(base->data + (src->y * base->bytes_per_line), src->width, src->height, base->bytes_per_line, QImage::Format_RGB32);
        if (base->trans != COLOR_UNKNOWN) {
            ret = ret.convertToFormat(QImage::Format_ARGB32);

            QRgb trans = base->trans;
            for (int i = 0; i < src->height; ++i) {
                auto* line = reinterpret_cast<QRgb*>(ret.scanLine(i));
                for (int j = 0; j < src->width; ++j) {
                    if (line[j] == trans) {
                        line[j] = 0x00000000;
                    }
                }
            }
        }
    } else if (base->image_type == it_index) {
        ret = QImage(base->data + (src->y * base->bytes_per_line), src->width, src->height, base->bytes_per_line, QImage::Format_Indexed8);
        ret.setColorCount(256);
        for (int i = 0; i < 256; ++i) {
            ret.setColor(i, 0xff000000 | base->clut->clut[i]);
        }
        if (base->clut->trans_index != COLOR_UNKNOWN) {
            ret.setColor(base->clut->trans_index, 0);
        }
    } else if (base->image_type == it_mono) {
        ret = QImage(src->width, src->height, QImage::Format_ARGB32);
        QRgb fg = 0xffffffff;
        QRgb bg = 0xff000000;

        if (base->clut) {
            if (base->clut->trans_index != COLOR_UNKNOWN) {
                bg = 0x0000000;
                if (base->clut->trans_index == 1) {
                    fg = 0x00000000;
                    bg = 0xffffffff;
                }
            } else {
                fg = 0xff000000 | base->clut->clut[1];
                bg = 0xff000000 | base->clut->clut[1];
            }
        }

        uint8_t *pt = base->data + src->y * base->bytes_per_line + (src->x >> 3);
#ifdef WORDS_BIGENDIAN
        for (int i = 0; i < src->height; ++i) {
            auto* line = reinterpret_cast<QRgb*>(ret.scanLine(i));
            int bit = (0x80 >> (src->x & 0x7));
            for (int j = 0, jj = 0; j < src->width; ++j) {
                line[j] = (pt[jj] & bit) ? fg : bg;
                if ((bit >>= 1) == 0) {
                    bit = 0x80;
                    ++jj;
                }
            }
            pt += base->bytes_per_line;
        }
#else
        for (int i = 0; i < src->height; ++i) {
            auto* line = reinterpret_cast<QRgb*>(ret.scanLine(i));
            int bit = (1 << (src->x & 0x7));
            for (int j = 0, jj = 0; j < src->width; ++j) {
                line[j] = (pt[jj] & bit) ? fg : bg;
                if ((bit = (bit << 1) & 0xff) == 0) {
                    bit = 0x1;
                    ++jj;
                }
            }
            pt += base->bytes_per_line;
        }
#endif
    } else {
        assert(false);
    }
    return ret;
}

static void GQtDrawPushClip(GWindow w, GRect *rct, GRect *old) {
    Log(LOGDEBUG, " ");

    // Return the current clip, and intersect the current clip with the desired
    // clip to get the new clip.
    auto& clip = w->ggc->clip;

    *old = clip;
    clip = *rct;
    if (clip.x + clip.width > old->x + old->width) {
        clip.width = old->x + old->width - clip.x;
    }
    if (clip.y + clip.height > old->y + old->height) {
        clip.height = old->y + old->height - clip.y;
    }
    if (clip.x < old->x) {
        if (clip.width > (old->x - clip.x)) {
            clip.width -= (old->x - clip.x);
        } else {
            clip.width = 0;
        }
        clip.x = old->x;
    }
    if (clip.y < old->y) {
        if (clip.height > (old->y - clip.y)) {
            clip.height -= (old->y - clip.y);
        } else {
            clip.height = 0;
        }
        clip.y = old->y;
    }
    if (clip.height < 0 || clip.width < 0) {
        // Negative values mean large positive values, so if we want to clip
        //  to nothing force clip outside window
        clip.x = clip.y = -100;
        clip.height = clip.width = 1;
    }

    QPainter *painter = GQtW(w)->Painter();
    painter->save();
    painter->setClipRect(QRect(clip.x, clip.y, clip.width, clip.height), Qt::IntersectClip);
}

static void GQtDrawPopClip(GWindow w, GRect *old) {
    Log(LOGDEBUG, " ");
    if (old) {
        w->ggc->clip = *old;
    }
    QPainter *painter = GQtW(w)->Painter();
    painter->restore();
}

static QPen GQtDrawGetPen(GGC *mine) {
    Color fg = mine->fg;
    if ((fg >> 24) == 0) {
        fg |= 0xff000000;
    }

    QPen pen;
    pen.setWidth(std::max(1, (int)mine->line_width));

    if (mine->dash_len != 0) {
        pen.setDashPattern({(qreal)mine->dash_len, (qreal)mine->skip_len});
    }

    // I don't use line join/cap. On a screen with small line_width they are irrelevant
    if (mine->ts != 0) {
        pen.setBrush(GQtDraw_StippleMePink(mine->ts, fg));
    } else {
        pen.setColor(QColor(fg));
    }
    return pen;
}

static QBrush GQtDrawGetBrush(GGC *mine) {
    Color fg = mine->fg;
    if ((fg >> 24) == 0) {
        fg |= 0xff000000;
    }
    if (mine->ts != 0) {
        return GQtDraw_StippleMePink(mine->ts, fg);
    } else {
        return QBrush(QColor(fg));
    }
}

static QFont GQtDrawGetFont(GFont *font) {
    QFont fd;

    // fd.setStyleHint(QFont::System);
    // fd.setFamily("Courier");
    fd.setFamily(QString::fromUtf8(font->rq.utf8_family_name));
    fd.setStyle((font->rq.style & fs_italic) ?
                    QFont::StyleItalic : QFont::StyleNormal);

    if (font->rq.style & fs_smallcaps) {
        fd.setCapitalization(QFont::SmallCaps);
    }
    fd.setWeight(font->rq.weight);
    fd.setStretch((font->rq.style & fs_condensed) ? QFont::Condensed :
                    (font->rq.style & fs_extended) ? QFont::Expanded  :
                    QFont::Unstretched);

    if (font->rq.style & fs_vertical) {
        //FIXME: not sure this is the right thing
        fd.setHintingPreference(QFont::PreferVerticalHinting);
    }

    if (font->rq.point_size <= 0) {
        // Any negative (pixel) values should be converted when font opened
        GDrawIError("Bad point size for Pango");
    }

    // Or set pixel size??
    fd.setPointSize(font->rq.point_size);
    return fd;
}

static void GQtDrawDrawLine(GWindow w, int32 x, int32 y, int32 xend, int32 yend, Color col) {
    // Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QPen pen = GQtDrawGetPen(w->ggc);
    if (pen.width() & 1) {
        path.moveTo(x + .5, y + .5);
        path.lineTo(xend + .5, yend + .5);
    } else {
        path.moveTo(x, y);
        path.lineTo(xend, yend);
    }

    GQtW(w)->Painter()->strokePath(path, pen);
}

static void GQtDrawDrawArrow(GWindow w, int32 x, int32 y, int32 xend, int32 yend, int16_t arrows, Color col) {
    Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QPen pen = GQtDrawGetPen(w->ggc);
    if (pen.width() & 1) {
        x += .5;
        y += .5;
        xend += .5;
        yend += .5;
    }

    const double head_angle = 0.5;
    double angle = atan2(yend - y, xend - x) + FF_PI;
    double length = sqrt((x - xend) * (x - xend) + (y - yend) * (y - yend));

    path.moveTo(x, y);
    path.lineTo(xend, yend);
    GQtW(w)->Painter()->strokePath(path, pen);

    if (length < 2) { //No point arrowing something so small
        return;
    } else if (length > 20) {
        length = 10;
    } else {
        length *= 2. / 3.;
    }

    QBrush brush = GQtDrawGetBrush(w->ggc);
    path.clear();
    path.moveTo(xend, yend);
    path.lineTo(xend + length * cos(angle - head_angle), yend + length * sin(angle - head_angle));
    path.lineTo(xend + length * cos(angle + head_angle), yend + length * sin(angle + head_angle));
    path.closeSubpath();
    GQtW(w)->Painter()->fillPath(path, brush);
}

static void GQtDrawDrawRect(GWindow w, GRect *rect, Color col) {
    Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QPen pen = GQtDrawGetPen(w->ggc);
    if (pen.width() & 1) {
        path.addRect(rect->x + .5, rect->y + .5, rect->width, rect->height);
    } else {
        path.addRect(rect->x, rect->y, rect->width, rect->height);
    }

    GQtW(w)->Painter()->strokePath(path, pen);
}

static void GQtDrawFillRect(GWindow w, GRect *rect, Color col) {
    Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QBrush brush = GQtDrawGetBrush(w->ggc);
    path.addRect(rect->x, rect->y, rect->width, rect->height);

    GQtW(w)->Painter()->fillPath(path, brush);
}

static void GQtDrawFillRoundRect(GWindow w, GRect *rect, int radius, Color col) {
    Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QBrush brush = GQtDrawGetBrush(w->ggc);
    path.addRoundedRect(rect->x, rect->y, rect->width, rect->height, radius, radius);

    GQtW(w)->Painter()->fillPath(path, brush);
}

static void GQtDrawDrawEllipse(GWindow w, GRect *rect, Color col) {
    Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QPen pen = GQtDrawGetPen(w->ggc);
    if (pen.width() & 1) {
        path.addEllipse(rect->x + .5, rect->y + .5, rect->width, rect->height);
    } else {
        path.addEllipse(rect->x, rect->y, rect->width, rect->height);
    }

    GQtW(w)->Painter()->strokePath(path, pen);
}

static void GQtDrawFillEllipse(GWindow w, GRect *rect, Color col) {
    Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QBrush brush = GQtDrawGetBrush(w->ggc);
    path.addEllipse(rect->x, rect->y, rect->width, rect->height);

    GQtW(w)->Painter()->fillPath(path, brush);
}

static void GQtDrawDrawArc(GWindow w, GRect *rect, int32 sangle, int32 eangle, Color col) {
    // Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    // Leftover from XDrawArc: sangle/eangle in degrees*64.
    double start = sangle / 64., end = eangle / 64.;

    QPainterPath path;
    QPen pen = GQtDrawGetPen(w->ggc);
    if (pen.width() & 1) {
        path.arcMoveTo(rect->x + .5, rect->y + .5, rect->width, rect->height, start);
        path.arcTo(rect->x + .5, rect->y + .5, rect->width, rect->height, start, end);
    } else {
        path.arcMoveTo(rect->x, rect->y, rect->width, rect->height, start);
        path.arcTo(rect->x, rect->y, rect->width, rect->height, start, end);
    }

    GQtW(w)->Painter()->strokePath(path, pen);
}

static void GQtDrawDrawPoly(GWindow w, GPoint *pts, int16_t cnt, Color col) {
    Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QPen pen = GQtDrawGetPen(w->ggc);
    double off = (pen.width() & 1) ? .5 : 0;

    path.moveTo(pts[0].x + off, pts[0].y + off);
    for (int i = 1; i < cnt; ++i) {
        path.lineTo(pts[i].x + off, pts[i].y + off);
    }

    GQtW(w)->Painter()->strokePath(path, pen);
}

static void GQtDrawFillPoly(GWindow w, GPoint *pts, int16_t cnt, Color col) {
    Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QBrush brush = GQtDrawGetBrush(w->ggc);
    QPen pen = GQtDrawGetPen(w->ggc);

    path.moveTo(pts[0].x, pts[0].y);
    for (int i = 1; i < cnt; ++i) {
        path.lineTo(pts[i].x, pts[i].y);
    }
    path.closeSubpath();
    GQtW(w)->Painter()->fillPath(path, brush);

    pen.setWidth(1); // hmm
    path.clear();
    path.moveTo(pts[0].x + .5, pts[0].y + .5);
    for (int i = 1; i < cnt; ++i) {
        path.lineTo(pts[i].x + .5, pts[i].y + .5);
    }
    path.closeSubpath();
    GQtW(w)->Painter()->strokePath(path, pen);
}

static void GQtDrawDrawImage(GWindow w, GImage *image, GRect *src, int32 x, int32 y) {
    Log(LOGDEBUG, " ");

    QImage img = _GQtDraw_GImage2QImage(image, src);
    GQtW(w)->Painter()->drawImage(x, y, img);
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

    // GQtW(pixmap)->Painter()->end(); //hm
    GQtW(w)->Painter()->drawPixmap(x, y, *GQtW(pixmap)->Pixmap(), src->x, src->y, src->width, src->height);
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
    // Log(LOGDEBUG, " ");

    struct font_instance *fi = w->ggc->fi;
    if (fi == nullptr || !text[0]) {
        return 0;
    }

    QFont fd = GQtDrawGetFont(fi);
    QString qtext = QString::fromUtf8(text);
    if (drawit == tf_drawit) {
        // QFontMetrics metrics(fd);
        // y -= metrics.ascent() + metrics.descent();
        // QRect rct(x, y, w->ggc->clip.x + w->ggc->clip.width - x, w->ggc->clip.y + w->ggc->clip.height - y);
        // if (!rct.isValid()) {
        //     return 0;
        // }
        QRect bounds;
        GQtW(w)->Painter()->setFont(fd);
        GQtW(w)->Painter()->drawText(x, y, qtext);
        return 0;
        // GQtW(w)->Painter()->drawText(rct, Qt::AlignLeft|Qt::AlignBottom, qtext, &bounds);
        // return bounds.width();
    } else if (drawit == tf_rect) {
        QFontMetrics metrics(fd);
        QRect br = metrics.tightBoundingRect(qtext);
        arg->size.width = metrics.horizontalAdvance(qtext);
        arg->size.lbearing = -br.x();
        arg->size.rbearing = br.width() - br.x();
        arg->size.fas = metrics.ascent();
        arg->size.fds = metrics.descent();
        arg->size.as = metrics.ascent();
        arg->size.ds = metrics.descent();

        QTextLayout layout; // qt 5.13 supports these relative to the paint device...
        layout.setText(qtext);
        layout.setFont(fd);
        layout.beginLayout();

        QTextLine line = layout.createLine();
        if (!line.isValid()) {
            memset(&arg->size, 0, sizeof(arg->size));
            return 0;
        } else {
            line.setLineWidth(w->ggc->clip.width - x);
            arg->size.as = line.ascent();
            arg->size.ds = line.descent();
        }
        return arg->size.width;
    }

    QFontMetrics metrics(fd);
    return metrics.horizontalAdvance(qtext);
}

static void GQtDrawPushClipOnly(GWindow w) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawClipPreserve(GWindow w) {
    Log(LOGDEBUG, " ");
}

// PANGO LAYOUT
static void GQtDrawGetFontMetrics(GWindow w, GFont *fi, int *as, int *ds, int *ld) {
    // Log(LOGDEBUG, " ");

    QFont fd = GQtDrawGetFont(fi);
    QFontMetrics fm(fd);

    *as = fm.ascent();
    *ds = fm.descent();
    *ld = 0;
}

static void GQtDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi) {
    Log(LOGDEBUG, " ");

    auto* state = GQtW(w)->InitLayout();
    if (fi == NULL) {
        fi = w->ggc->fi;
    }

    state->fd = GQtDrawGetFont(fi);
    state->text = QString::fromUtf8(text, cnt);
}

static void GQtDrawLayoutDraw(GWindow w, int32 x, int32 y, Color fg) {
    Log(LOGDEBUG, " ");
    GQtWindow *gw = GQtW(w);
    auto* state = gw->Layout();
    if (!state) {
        assert(false);
        return;
    }
    gw->Painter()->setPen(QColor(fg));
    gw->Painter()->setFont(state->fd);
    gw->Painter()->drawText(x, y, state->text);
}

static void GQtDrawLayoutIndexToPos(GWindow w, int index, GRect *pos) {
    Log(LOGDEBUG, " ");
    // GGDKWindow gw = (GGDKWindow)w;
    // PangoRectangle rect;

    // pango_layout_index_to_pos(gw->pango_layout, index, &rect);
    // pos->x = rect.x / PANGO_SCALE;
    // pos->y = rect.y / PANGO_SCALE;
    // pos->width  = rect.width / PANGO_SCALE;
    // pos->height = rect.height / PANGO_SCALE;
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
    ret->fontstate = &gdisp->fs;

    std::unique_ptr<GQtWidget> groot(new GQtWidget(nullptr, Qt::Widget));
    QRect screenGeom = gdisp->app->primaryScreen()->geometry();

    ret->res = gdisp->fs.res = gdisp->app->primaryScreen()->logicalDotsPerInch();
    ret->groot = groot->Base();
    ret->groot->display = (GDisplay*)gdisp.get();
    ret->groot->native_window = static_cast<GQtWindow*>(groot.get());
    ret->groot->pos.width = screenGeom.width();
    ret->groot->pos.height = screenGeom.height();
    ret->groot->is_toplevel = true;
    ret->groot->is_visible = true;

    gdisp->bs.double_time = 200;
    gdisp->bs.double_wiggle = 3;
    GDrawResourceFind();
    gdisp->bs.double_time = _GDraw_res_multiclicktime;
    gdisp->bs.double_wiggle = _GDraw_res_multiclickwiggle;

    (ret->funcs->init)(ret);
    _GDraw_InitError(ret);

    groot.release();
    gdisp.release();
    return ret;
}

extern "C" void _GQtDraw_DestroyDisplay(GDisplay *disp) {
    delete GQtW(disp->groot);
    delete GQtD(disp);
}

#endif // FONTFORGE_CAN_USE_QT
