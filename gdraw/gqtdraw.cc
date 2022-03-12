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

#include <array>
#include <QtWidgets>
#include <type_traits>

/**
 * TODO
 * Window lifecycle - cleanup createwindow
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
static QFont GQtDrawGetFont(GFont *font);

static QColor ToQColor(Color col) {
    if (COLOR_ALPHA(col) == 0) {
        col |= 0xff000000;
    }
    return QColor(COLOR_RED(col), COLOR_GREEN(col), COLOR_BLUE(col), COLOR_ALPHA(col));
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
        state |= ksm_super;
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
            if (wattrs->mask & wam_restrict) {
                windowFlags |= Qt::Popup; // modal
            } else {
                windowFlags |= Qt::ToolTip; // modeless
            }
        } else if ((wattrs->mask & wam_isdlg) && wattrs->is_dlg) {
            if (wattrs->mask & wam_palette) {
                windowFlags |= Qt::Tool;
            } else {
                windowFlags |= Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint;
            }
            if ((wattrs->mask & wam_noresize) && wattrs->noresize) {
                windowFlags &= ~Qt::WindowMaximizeButtonHint;
            }
        }
    }

    auto* window = new GQtWidget(parent, windowFlags);
    GWindow ret = window->Base();
    ret->native_window = static_cast<GQtWindow*>(window);
    ret->is_toplevel = w == nullptr;
    ret->is_popup = (windowFlags & Qt::ToolTip) == Qt::ToolTip;
    ret->is_dlg = ret->is_popup || ((windowFlags & Qt::Dialog) == Qt::Dialog);
    ret->display = gdisp->Base();
    ret->eh = eh;
    ret->parent = w;
    ret->pos = *pos;
    ret->user_data = user_data;

    window->setFocusPolicy(Qt::StrongFocus);
    window->resize(pos->width, pos->height);
    // TODO: Center?
    if (!ret->is_toplevel || ((wattrs->mask & wam_positioned) && !(wattrs->mask & wam_centered))) {
        window->move(ret->pos.x, ret->pos.y);
    }

    if (wattrs->mask & wam_events) {
        if (wattrs->event_masks & (1 << et_mousemove)) {
            window->setMouseTracking(true);
        }
    }

    // Set background
    if (!(wattrs->mask & wam_backcol) || wattrs->background_color == COLOR_DEFAULT) {
        wattrs->background_color = _GDraw_res_bg;
    }
    ret->ggc->bg = wattrs->background_color;
    GQtDrawSetWindowBackground(ret, ret->ggc->bg);

    if (ret->is_toplevel) {
        // Set title; icon titles are ignored.
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
        }
        // Set cursor
        if ((wattrs->mask & wam_cursor) && wattrs->cursor != ct_default) {
            GQtDrawSetCursor(ret, wattrs->cursor);
        }

        if (wattrs->mask & wam_restrict) {
            ret->restrict_input_to_me = wattrs->restrict_input_to_me;
            window->setWindowModality(Qt::ApplicationModal);
        }
        if (wattrs->mask & wam_palette) {
            window->setBaseSize(window->size());
            window->setMinimumSize(window->size());
        }
        if ((wattrs->mask & wam_noresize) && wattrs->noresize) {
            window->setFixedSize(window->size());
        }
        if ((wattrs->mask & wam_transient) && wattrs->transient != nullptr) {
            GQtDrawSetTransientFor(ret, wattrs->transient);
            ret->is_dlg = true;
        } else if (!ret->is_dlg) {
            ++gdisp->top_window_count;
            Log(LOGWARN, "INCTP %d", gdisp->top_window_count);
        }
        ret->was_positioned = true;
        window->setAttribute(Qt::WA_QuitOnClose, !ret->is_dlg);
    } else {
        // Expectation for GDraw is that it is created hidden
        // Toplevels are not shown by default, but children are
        // shown with the toplevel by default, so hide it
        window->hide();
    }

    // Event handler
    window->DispatchEvent(window->InitEvent<>(et_create), nullptr);
    Log(LOGWARN, "Window created: %p[%p][%s][toplevel:%d]",
        ret, ret->native_window, window->Title(), ret->is_toplevel);

    return ret;
}

static GWindow _GQtDraw_NewPixmap(GDisplay *disp, GWindow similar, uint16_t width, uint16_t height, bool is_bitmap,
                                   const unsigned char *data) {
    auto* pixmap = new GQtPixmap(width, height);
    GWindow ret = pixmap->Base();
    QImage::Format format;
    int stride;

    width &= 0x7fff;
    ret->ggc->bg = _GDraw_res_bg;
    ret->native_window = static_cast<GQtWindow*>(pixmap);
    ret->display = disp;
    ret->is_pixmap = 1;
    ret->parent = nullptr;
    ret->pos.x = ret->pos.y = 0;
    ret->pos.width = width;
    ret->pos.height = height;

    if (data != nullptr) {
        if (is_bitmap) {
            QImage img(data, width, height, width / 8, QImage::Format_MonoLSB);
            // for some reason converting a mono bitmap directly
            // causes a crash with qt6
            img = img.convertToFormat(QImage::Format_ARGB32);
            img.invertPixels();
            QBitmap bm = QBitmap::fromImage(img);
            pixmap->fill(Qt::transparent);

            QPainter painter(pixmap);
            painter.setPen(Qt::black);
            painter.setBackgroundMode(Qt::TransparentMode);
            painter.drawPixmap(0, 0, bm);
        } else {
            pixmap->convertFromImage(QImage(data, width, height, QImage::Format_ARGB32));
        }
    }

    return ret;
}

GQtWidget::~GQtWidget()
{
    Log(LOGWARN, "CLEANUP [%p] [%p] [%s]", Base(), this, Title());
}

template<typename E>
void UpdateLastEventTime(GWindow w, E *event) {}
template<typename E, typename std::enable_if<std::is_integral<decltype(E::timestamp())>::value>::type* = nullptr>
void UpdateLastEventTime(GWindow w, E *event) {
    GQtD(w)->last_event_time = event->timestamp();
}

template<typename E>
GEvent GQtWidget::InitEvent(event_type et, E *event) {
    GEvent gevent = {};
    gevent.w = Base();
    gevent.native_window = static_cast<GQtWindow*>(this);
    gevent.type = et;
    UpdateLastEventTime(gevent.w, event);
    return gevent;
}

void GQtWidget::DispatchEvent(const GEvent& e, QEvent* trigger) {
    if (e.w->eh && e.type != et_noevent && (!e.w->is_dying || e.type == et_destroy)) {
        if ((e.w->eh)(e.w, const_cast<GEvent*>(&e)) && trigger) {
            trigger->accept();
        }
    }
}

QPainter* GQtWidget::Painter() {
    if (!m_painter) {
        static QPainter sPainter;
        Log(LOGDEBUG, "[%p] [%s] that's a paddlin", Base(), Title());
        return &sPainter;
    }
    return m_painter;
}

bool GQtWidget::event(QEvent *event) {
    static const QMetaEnum sEventType = QMetaEnum::fromType<QEvent::Type>();
    Log(LOGVERB, "Received %s", sEventType.valueToKey(event->type()));
    return QWidget::event(event);
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
    DispatchEvent(gevent, event);
}


void GQtWidget::mouseEvent(QMouseEvent* event, event_type et) {
    GEvent gevent = InitEvent(et, event);
    gevent.u.mouse.state = _GQtDraw_QtModifierToKsm(event->modifiers());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    gevent.u.mouse.x = event->position().x();
    gevent.u.mouse.y = event->position().y();
#else
    gevent.u.mouse.x = event->x();
    gevent.u.mouse.y = event->y();
#endif
    gevent.u.mouse.time = event->timestamp();
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

    DispatchEvent(gevent, event);
    // Log(LOGDEBUG, "Button %7s: [%f %f]", evt->type == GDK_BUTTON_PRESS ? "press" : "release", evt->x, evt->y);
}

void GQtWidget::wheelEvent(QWheelEvent *event) {
    GEvent gevent = InitEvent(et_mousedown, event);
    GQtDisplay *gdisp = GQtD(gevent.w);

    gevent.u.mouse.state = _GQtDraw_QtModifierToKsm(event->modifiers());
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    gevent.u.mouse.x = event->position().x();
    gevent.u.mouse.y = event->position().y();
#else
    gevent.u.mouse.x = event->x();
    gevent.u.mouse.y = event->y();
#endif
    gevent.u.mouse.clicks = gdisp->bs.cur_click = 1;

    QPoint angle = event->angleDelta();
    if (angle.y() != 0) {
        gevent.u.mouse.button = angle.y() < 0 ? 5 : 4;
    } else {
        gevent.u.mouse.button = angle.x() < 0 ? 6 : 7;
    }
    // We need to simulate two events... I think.
    DispatchEvent(gevent, event);
    gevent.type = et_mouseup;
    DispatchEvent(gevent, event);
}

void GQtWidget::mouseMoveEvent(QMouseEvent *event) {
    GEvent gevent = InitEvent(et_mousemove, event);
    gevent.u.mouse.state = _GQtDraw_QtModifierToKsm(event->modifiers());
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    gevent.u.mouse.x = event->position().x();
    gevent.u.mouse.y = event->position().y();
#else
    gevent.u.mouse.x = event->x();
    gevent.u.mouse.y = event->y();
#endif
    DispatchEvent(gevent, event);
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
    // painter.setRenderHint(QPainter::SmoothPixmapTransform);

    m_painter = &painter;
    DispatchEvent(gevent, event);
    m_painter = nullptr;
    Path()->clear();
}

void GQtWidget::configureEvent(QEvent* event) {
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

    DispatchEvent(gevent, event);
}

void GQtWidget::mapEvent(QEvent *event, bool visible) {
    GEvent gevent = InitEvent(et_map);
    gevent.u.map.is_visible = visible;
    gevent.w->is_visible = visible;
    DispatchEvent(gevent, event);
}

void GQtWidget::focusEvent(QFocusEvent* event, bool focusIn) {
    GEvent gevent = InitEvent(et_focus, event);
    gevent.u.focus.gained_focus = focusIn;
    gevent.u.focus.mnemonic_focus = false;

    while (!gevent.w->is_toplevel && gevent.w->parent) {
        gevent.w = gevent.w->parent;
        gevent.native_window = GQtW(gevent.w);
    }

    if (gevent.w != Base()) {
        // Redirect to toplevel
        GQtW(gevent.w)->Widget()->focusEvent(event, focusIn);
        return;
    } else if (m_has_focus == focusIn) {
        return;
    }

    switch (event->reason()) {
    case Qt::TabFocusReason:
        gevent.u.focus.mnemonic_focus = mf_tab;
        break;
    case Qt::ShortcutFocusReason:
        gevent.u.focus.mnemonic_focus = mf_shortcut;
    default:
        break;
    }

    Log(LOGWARN, "Focus change %s %p(%s)",
        gevent.u.focus.gained_focus ? "IN" : "OUT", Base(), Title());

    m_has_focus = focusIn;
    DispatchEvent(gevent, event);
}

void GQtWidget::crossingEvent(QEvent* event, bool enter) {
    // Why QEvent?!?
    GEvent gevent = InitEvent(et_crossing, event);
    QPoint pos = mapFromGlobal(QCursor::pos());
    GQtDisplay *gdisp = GQtD(gevent.w);
    gevent.u.crossing.x = pos.x();
    gevent.u.crossing.y = pos.y();
    gevent.u.crossing.state = _GQtDraw_QtModifierToKsm(gdisp->app->keyboardModifiers());
    gevent.u.crossing.entered = enter;
    gevent.u.crossing.device = NULL;
    gevent.u.crossing.time = gdisp->last_event_time;
    // Always set to false when crossing boundary...
    gdisp->is_space_pressed = false;
    DispatchEvent(gevent, event);
}

void GQtWidget::inputMethodEvent(QInputMethodEvent *event)
{
    Log(LOGWARN, "%p[%s]: %s", Base(), Title(), event->commitString().toStdString().c_str());
    if (!event->commitString().isEmpty())
    {
        GEvent gevent = InitEvent(et_char, event);
        gevent.u.chr.autorepeat = false;

        QString text = event->commitString();
        const QChar* uni = text.unicode();
        for (size_t i = 0, j = 0; i < text.size(); ++i) {
            if (uni[i].isHighSurrogate() && (i+1) < text.size() &&
                uni[i+1].isLowSurrogate()) {
                gevent.u.chr.chars[j++] = QChar::surrogateToUcs4(uni[i], uni[i+1]);
                ++i;
            } else {
                gevent.u.chr.chars[j++] = uni[i].unicode();
            }

            if (j >= (_GD_EVT_CHRLEN-1) && (i+1) < text.size()) {
                gevent.u.chr.keysym = gevent.u.chr.chars[0];
                DispatchEvent(gevent, event);
                memset(gevent.u.chr.chars, 0, _GD_EVT_CHRLEN);
                j = 0;
            }
        }
        QToolTip::hideText();
        gevent.u.chr.keysym = gevent.u.chr.chars[0];
        DispatchEvent(gevent, event);
    } else {
        if (!event->preeditString().isEmpty()) {
            QPoint pos = mapToGlobal(m_icpos);

            QFont fd = GQtDrawGetFont(Base()->ggc->fi);
            QFontMetrics fm(fd);
            pos.setY(pos.y() - fm.height() * 3);
            QToolTip::showText(pos, event->preeditString(), this);
        } else {
            QToolTip::hideText();
        }
        QWidget::inputMethodEvent(event);
    }
}

QVariant GQtWidget::inputMethodQuery(Qt::InputMethodQuery query) const
{
    Log(LOGWARN, "%p[%s] %x", Base(), Title(), query);
    switch (query)
    {
    case Qt::ImEnabled:
        return QVariant(true);
    case Qt::ImCursorPosition:
        return QVariant(m_icpos);
    case Qt::ImCursorRectangle:
        return QRect(m_icpos, m_icpos);
    default:
        return QWidget::inputMethodQuery(query);
    }
}

void GQtWidget::closeEvent(QCloseEvent *event) {
    if (Base()->is_dying || Base() == Base()->display->groot) {
        Log(LOGWARN, "ACCEPT CLOSe [%p][%s]", Base(), Title());
        event->accept();
        DispatchEvent(InitEvent<>(et_destroy), nullptr);
        deleteLater();

        GQtDisplay *gdisp = GQtD(this);
        if (Base()->parent == nullptr && !Base()->is_dlg) {
            gdisp->top_window_count--;
            Log(LOGWARN, "DECTP %d", gdisp->top_window_count);
            if (gdisp->top_window_count <= 0) {
                gdisp->app->quit();
            }
        }

        return;
    }

    event->ignore();
    Log(LOGWARN, "SEND CLOSe [%p][%s]", Base(), Title());
    GEvent gevent = InitEvent(et_close, event);
    DispatchEvent(gevent, nullptr);
}

void GQtWidget::SetICPos(int x, int y) {
    if (x == 10000 && y == 10000) {
        if (m_ime_enabled) {
            setAttribute(Qt::WA_InputMethodEnabled, false);
            m_ime_enabled = false;
        }
    } else {
        if (!m_ime_enabled) {
            setAttribute(Qt::WA_InputMethodEnabled, true);
            m_ime_enabled = true;
        }
        m_icpos.setX(x);
        m_icpos.setY(y * 1.05);
    }
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

static GWindow GQtDrawCreatePixmap(GDisplay *disp, GWindow similar, uint16_t width, uint16_t height) {
    Log(LOGDEBUG, " ");
    return _GQtDraw_NewPixmap(disp, similar, width, height, false, nullptr);
}

static GWindow GQtDrawCreateBitmap(GDisplay *disp, uint16_t width, uint16_t height, uint8_t *data) {
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
        painter.setPen(ToQColor(bg));
        painter.setBackgroundMode(Qt::TransparentMode);
        painter.drawPixmap(0, 0, GQtW(mask)->Pixmap()->mask());

        painter.setPen(ToQColor(fg));
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
                // Can happen e.g. for palette windows
                Log(LOGINFO, "Orphaned toplevel [%p] [%s]", child->Base(), child->Title());
                GQtDrawSetTransientFor(child->Base(), nullptr);
            } else {
                GQtDrawDestroyWindow(child->Base());
            }
        }

        gw->Widget()->setDisabled(true);
        gw->Widget()->hide();
        QMetaObject::invokeMethod(gw->Widget(), "close", Qt::QueuedConnection);
    }
}

static int GQtDrawNativeWindowExists(GDisplay *UNUSED(gdisp), void *native_window) {
    // Log(LOGDEBUG, " ");
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
    pal.setColor(QPalette::Window, ToQColor(gcol));
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

static void GQtDrawMove(GWindow w, int32_t x, int32_t y) {
    Log(LOGDEBUG, "%p:%s, %d %d", w, GQtW(w)->Title(), x, y);
    GQtW(w)->Widget()->move(x, y);
}

static void GQtDrawTrueMove(GWindow w, int32_t x, int32_t y) {
    Log(LOGDEBUG, " ");
    GQtW(w)->Widget()->move(x, y);
}

static void GQtDrawResize(GWindow w, int32_t width, int32_t height) {
    Log(LOGDEBUG, "%p:%s, %d %d", w, GQtW(w)->Title(), width, height);
    GQtW(w)->Widget()->resize(width, height);
}

static void GQtDrawMoveResize(GWindow w, int32_t x, int32_t y, int32_t width, int32_t height) {
    Log(LOGDEBUG, "%p:%s, %d %d %d %d", w, GQtW(w)->Title(), x, y, width, height);
    GQtW(w)->Widget()->setGeometry(x, y, width, height);
}

static void GQtDrawRaise(GWindow w) {
    Log(LOGDEBUG, "%p:%s", w, GQtW(w)->Title());
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
    assert(!owner || owner->is_toplevel);

    QWidget *trans = GQtW(transient)->Widget();
    QWidget *parent = owner ? GQtW(owner)->Widget() : nullptr;
    Qt::WindowFlags flags = trans->windowFlags();
    bool visible = trans->isVisible();

    trans->setParent(parent, flags);
    if (visible) {
        trans->show();
    }
}

static void GQtDrawGetPointerPosition(GWindow w, GEvent *ret) {
    Log(LOGDEBUG, " ");
    auto *gdisp = GQtD(w);
    Qt::KeyboardModifiers modifiers = gdisp->app->keyboardModifiers();
    QPoint pos = GQtW(w)->Widget()->mapFromGlobal(QCursor::pos());

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

static void GQtDrawScroll(GWindow w, GRect *rect, int32_t hor, int32_t vert) {
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


static GIC *GQtDrawCreateInputContext(GWindow w, enum gic_style UNUSED(style)) {
    Log(LOGWARN, "%p:%s", w, GQtW(w)->Widget()->Title());
    return (GIC*)-1;
}

static void GQtDrawSetGIC(GWindow w, GIC *UNUSED(gic), int x, int y) {
    Log(LOGWARN, "%p:%s %dx%d", w, GQtW(w)->Widget()->Title(), x, y);
    GQtW(w)->Widget()->SetICPos(x, y);
}

static int GQtDrawKeyState(GWindow w, int keysym) {
    Log(LOGDEBUG, " ");
    if (keysym != ' ') {
        Log(LOGWARN, "Cannot check state of unsupported character!");
        return 0;
    }
    // Since this function is only used to check the state of the space button
    // Don't bother with a full implementation...
    return GQtD(w)->is_space_pressed;
}

static void GQtDrawGrabSelection(GWindow w, enum selnames sn) {
    Log(LOGDEBUG, " ");

    if ((int)sn < 0 || sn >= sn_max) {
        return;
    }
}

static void GQtDrawAddSelectionType(GWindow w, enum selnames sel, char *type, void *data, int32_t cnt, int32_t unitsize,
                                     void *gendata(void *, int32_t *len), void freedata(void *)) {
    Log(LOGDEBUG, " ");
}

static void *GQtDrawRequestSelection(GWindow w, enum selnames sn, char *type_name, int32_t *len) {
    if ((int)sn < 0 || sn > sn_clipboard) {
        return nullptr;
    }

    QClipboard::Mode mode = (QClipboard::Mode)(1 - (int)sn);
    QClipboard *cb = GQtD(w)->app->clipboard();
    const QMimeData *data = cb->mimeData(mode);
    if (!data) {
        return nullptr;
    }
    QByteArray arr = data->data(QString::fromUtf8(type_name));
    if (arr.isEmpty()) {
        return nullptr;
    }

    char* ret = (char*)calloc(4 + arr.size(), 1);
    memcpy(ret, arr.data(), arr.size());
    *len = arr.size();
    return ret;
}

static int GQtDrawSelectionHasType(GWindow w, enum selnames sn, char *type_name) {
    Log(LOGDEBUG, " ");
    if ((int)sn < 0 || sn > sn_clipboard) {
        return false;
    }

    QClipboard::Mode mode = (QClipboard::Mode)(1 - (int)sn);
    QClipboard *cb = GQtD(w)->app->clipboard();
    const QMimeData *data = cb->mimeData(mode);
    return data && data->hasFormat(QString::fromUtf8(type_name));
}

static void GQtDrawBindSelection(GDisplay *disp, enum selnames sn, char *atomname) {
    Log(LOGDEBUG, " ");
}

static int GQtDrawSelectionHasOwner(GDisplay *disp, enum selnames sn) {
    Log(LOGDEBUG, " ");

    if ((int)sn < 0 || sn > sn_clipboard) {
        return false;
    }

    QClipboard::Mode mode = (QClipboard::Mode)(1 - (int)sn);
    QClipboard *cb = GQtD(disp)->app->clipboard();
    const QMimeData *data = cb->mimeData(mode);
    return data && !data->formats().empty();
}

static void GQtDrawPointerUngrab(GDisplay *disp) {
    Log(LOGDEBUG, "");
}

static void GQtDrawPointerGrab(GWindow w) {
    Log(LOGDEBUG, "%p [%s]", w, GQtW(w)->Widget()->Title());
    // Don't use this, just set wam_restrict properly...
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
    if (GQtD(disp)->top_window_count > 0) {
        GQtD(disp)->app->exec();
    }
}

static void GQtDrawPostEvent(GEvent *e) {
    //Log(LOGDEBUG, " ");
    e->native_window = GQtW(e->w);
    GQtW(e->w)->Widget()->DispatchEvent(*e, nullptr);
}

static void GQtDrawPostDragEvent(GWindow w, GEvent *mouse, enum event_type et) {
    Log(LOGDEBUG, " ");
}

static int GQtDrawRequestDeviceEvents(GWindow w, int devcnt, struct gdeveventmask *de) {
    Log(LOGDEBUG, " ");
    return 0; //Not sure how to handle... For tablets...
}

static int GQtDrawShortcutKeyMatches(const GEvent *e, unichar_t ch) {
    if ((e->type != et_char && e->type != et_charup) || ch == 0) {
        return false;
    }

    Log(LOGWARN, "KSM keysym(%d) ch(%d) tk(%d) tch(%d)",
        e->u.chr.keysym, ch, toupper(e->u.chr.keysym),
        toupper(ch));
    return toupper(e->u.chr.keysym) == toupper(ch) ||
        toupper(e->u.chr.chars[0]) == toupper(ch);
}

GQtTimer::GQtTimer(GQtWindow *parent, void *userdata)
    : QTimer(parent->Widget())
    , m_gtimer{parent->Base(), this, userdata}
{
}

static GTimer *GQtDrawRequestTimer(GWindow w, int32_t time_from_now, int32_t frequency, void *userdata) {
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
        gw->Widget()->DispatchEvent(gevent, nullptr);
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
    QBrush brush((ts == 2) ? Qt::Dense5Pattern : Qt::Dense4Pattern);
    brush.setColor(fg);
    return brush;
}

static QImage _GQtDraw_GImage2QImage(GImage *image, GRect *src) {
    const struct _GImage *base = (image->list_len == 0) ? image->u.image : image->u.images[0];
    QImage ret;

    if (base->image_type == it_rgba) {
        ret = QImage(base->data + (src->y * base->bytes_per_line) + src->x * 4, src->width, src->height, base->bytes_per_line, QImage::Format_ARGB32);
    } else if (base->image_type == it_true) {
        ret = QImage(base->data + (src->y * base->bytes_per_line) + src->x * 4, src->width, src->height, base->bytes_per_line, QImage::Format_RGB32);
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
        ret = QImage(base->data + (src->y * base->bytes_per_line) + src->x, src->width, src->height, base->bytes_per_line, QImage::Format_Indexed8);
        ret.setColorCount(base->clut->clut_len);
        for (int i = 0; i < base->clut->clut_len; ++i) {
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
            fg = 0xff000000 | base->clut->clut[1];
            bg = 0xff000000 | base->clut->clut[0];
            if (base->clut->trans_index != COLOR_UNKNOWN) {
                fg = base->clut->trans_index == 0 ? fg : 0x00000000;
                bg = base->clut->trans_index == 0 ? 0x00000000 : bg;
            }
        }

        // Can't use mono formats as this is potentially not byte aligned
        uint8_t *pt = base->data + src->y * base->bytes_per_line + (src->x >> 3);
#ifdef WORDS_BIGENDIAN
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
#else
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
#endif
    } else {
        assert(false);
    }
    return ret;
}

static GImage *_GQtDraw_GImageExtract(struct _GImage *base, GRect *size,
                                       double xscale, double yscale) {
    static GImage temp;
    static struct _GImage tbase;
    static std::vector<uint8_t> data;
    int r, c;

    memset(&temp, 0, sizeof(temp));
    tbase = *base;
    temp.u.image = &tbase;
    tbase.width = size->width;
    tbase.height = size->height;
    if (base->image_type == it_mono) {
        tbase.bytes_per_line = (size->width + 7) / 8;
    } else if (base->image_type == it_index) {
        tbase.bytes_per_line = size->width;
    } else {
        tbase.bytes_per_line = 4 * size->width;
    }
    if (tbase.bytes_per_line * size->height > data.size()) {
        data.resize(tbase.bytes_per_line * size->height);
    }
    tbase.data = data.data();

    // I used to use rint(x). Now I use floor(x). For normal images rint
    // might be better, but for text we need floor
    if (base->image_type == it_mono) {
        memset(tbase.data, 0, tbase.height * tbase.bytes_per_line);
        for (r = 0; r < size->height; ++r) {
            int org = ((int) floor((r + size->y) / yscale));
            uint8_t *pt = tbase.data + r * tbase.bytes_per_line;
            uint8_t *opt = base->data + org * base->bytes_per_line;
            for (c = 0; c < size->width; ++c) {
                int oc = ((int) floor((c + size->x) / xscale));
#ifdef WORDS_BIGENDIAN
                if (opt[oc >> 3] & ((1 << (oc & 0x7))) {
                    pt[c >> 3] |= ((1 << (oc & 0x7)));
                }
#else
                if (opt[oc >> 3] & (0x80 >> (oc & 7))) {
                    pt[c >> 3] |= (0x80 >> (c & 7));
                }
#endif
            }
        }
    } else if (base->image_type == it_index) {
        for (r = 0; r < size->height; ++r) {
            int org = ((int) floor((r + size->y) / yscale));
            uint8_t *pt = tbase.data + r * tbase.bytes_per_line;
            uint8_t *opt = base->data + org * base->bytes_per_line;
            for (c = 0; c < size->width; ++c) {
                int oc = ((int) floor((c + size->x) / xscale));
                *pt++ = opt[oc];
            }
        }
    } else {
        for (r = 0; r < size->height; ++r) {
            int org = ((int) floor((r + size->y) / yscale));
            uint32_t *pt = (uint32_t *)(tbase.data + r * tbase.bytes_per_line);
            uint32_t *opt = (uint32_t *)(base->data + org * base->bytes_per_line);
            for (c = 0; c < size->width; ++c) {
                int oc = ((int) floor((c + size->x) / xscale));
                *pt++ = opt[oc];
            }
        }
    }
    return &temp;
}

static void GQtDrawPushClip(GWindow w, GRect *rct, GRect *old) {
    // Log(LOGDEBUG, " ");

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
    // Log(LOGDEBUG, " ");
    if (old) {
        w->ggc->clip = *old;
    }
    QPainter *painter = GQtW(w)->Painter();
    painter->restore();
}

static QPen GQtDrawGetPen(GGC *mine) {
    Color fg = mine->fg;
    QPen pen;
    pen.setWidth(std::max(1, (int)mine->line_width));

    if (mine->dash_len != 0) {
        pen.setDashPattern({(qreal)mine->dash_len, (qreal)mine->skip_len});
    }

    // I don't use line join/cap. On a screen with small line_width they are irrelevant
    if (mine->ts != 0) {
        pen.setBrush(GQtDraw_StippleMePink(mine->ts, fg));
    } else {
        pen.setColor(ToQColor(fg));
    }
    return pen;
}

static QBrush GQtDrawGetBrush(GGC *mine) {
    Color fg = mine->fg;
    if (mine->ts != 0) {
        return GQtDraw_StippleMePink(mine->ts, fg);
    } else {
        return QBrush(ToQColor(fg));
    }
}

static QFont GQtDrawGetFont(GFont *font) {
    QFont fd;

    QString families = QString::fromUtf8(font->rq.utf8_family_name);
    QStringList familyList = families.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (auto& family : familyList) {
        family = family.trimmed();
        if (family == QLatin1String("system-ui")) {
            family = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
        } else if (family == QLatin1String("monospace")) {
            family = QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
        }
    }
    fd.setFamilies(familyList);
    fd.setStyle((font->rq.style & fs_italic) ?
                    QFont::StyleItalic : QFont::StyleNormal);

    if (font->rq.style & fs_smallcaps) {
        fd.setCapitalization(QFont::SmallCaps);
    }

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    fd.setWeight(static_cast<QFont::Weight>(font->rq.weight));
#else
    static std::array<std::pair<int, int>, 10> sWeightMap {{
        {100, 0},
        {200, 12},
        {300, 25},
        {400, 50},
        {500, 57},
        {600, 63},
        {700, 75},
        {800, 81},
        {900, 87},
        {1000, 99},
    }};
    auto it = std::lower_bound(sWeightMap.begin(), sWeightMap.end(), font->rq.weight,
        [](const std::pair<int, int>& e, int v) {
            return e.first < v;
        });
    fd.setWeight(it == sWeightMap.end() ? 99 :
        it->second + (((it+1)->second - it->second) *
                      (font->rq.weight - it->first)) /
                     ((it+1)->first - it->first));
#endif

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

    fd.setPointSize(font->rq.point_size);
    return fd;
}

static void GQtDrawDrawLine(GWindow w, int32_t x, int32_t y, int32_t xend, int32_t yend, Color col) {
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

static void GQtDrawDrawArrow(GWindow w, int32_t x, int32_t y, int32_t xend, int32_t yend, int16_t arrows, Color col) {
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
    // Log(LOGDEBUG, " ");

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
    // Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QBrush brush = GQtDrawGetBrush(w->ggc);
    path.addRect(rect->x, rect->y, rect->width, rect->height);

    GQtW(w)->Painter()->fillPath(path, brush);
}

static void GQtDrawFillRoundRect(GWindow w, GRect *rect, int radius, Color col) {
    // Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QBrush brush = GQtDrawGetBrush(w->ggc);
    path.addRoundedRect(rect->x, rect->y, rect->width, rect->height, radius, radius);

    GQtW(w)->Painter()->fillPath(path, brush);
}

static void GQtDrawDrawEllipse(GWindow w, GRect *rect, Color col) {
    // Log(LOGDEBUG, " ");

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
    // Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QBrush brush = GQtDrawGetBrush(w->ggc);
    path.addEllipse(rect->x, rect->y, rect->width, rect->height);

    GQtW(w)->Painter()->fillPath(path, brush);
}

static void GQtDrawDrawArc(GWindow w, GRect *rect, int32_t sangle, int32_t eangle, Color col) {
    // Log(LOGDEBUG, " ");

    w->ggc->fg = col;

    QPainterPath path;
    QPen pen = GQtDrawGetPen(w->ggc);
    double factor = (pen.width() & 1) ? 0.5 : 0;
    // Leftover from XDrawArc: sangle/eangle in degrees*64.
    double start = sangle / 64., end = eangle / 64.;

    QRectF rct(rect->x + factor, rect->y + factor, rect->width, rect->height);
    path.arcMoveTo(rct, start);
    path.arcTo(rct, start, end);
    GQtW(w)->Painter()->strokePath(path, pen);
}

static void GQtDrawDrawPoly(GWindow w, GPoint *pts, int16_t cnt, Color col) {
    // Log(LOGDEBUG, " ");

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
    // Log(LOGDEBUG, " ");

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

static void GQtDrawDrawImage(GWindow w, GImage *image, GRect *src, int32_t x, int32_t y) {
    // Log(LOGDEBUG, " ");

    QImage img = _GQtDraw_GImage2QImage(image, src);
    GQtW(w)->Painter()->drawImage(x, y, img);
}

static void GQtDrawDrawGlyph(GWindow w, GImage *image, GRect *src, int32_t x, int32_t y) {
    Log(LOGDEBUG, " ");

    const struct _GImage *base = (image->list_len) == 0 ? image->u.image : image->u.images[0];
    const uint8_t *data = base->data + (src->y * base->bytes_per_line) + src->x;

    if (base->image_type != it_index) {
        GQtDrawDrawImage(w, image, src, x, y);
    } else {
        // Use the grey levels as an alpha channel
        // TODO: Is this really needed? Seems to do alpha blending just fine without this...
        QImage img(src->width, src->height, QImage::Format_ARGB32);
        Color fg = 0x00ffffff & base->clut->clut[base->clut->clut_len - 1];
        int factor = base->clut->clut_len == 256 ? 1 :
                     base->clut->clut_len == 16 ? 17 :
                     base->clut->clut_len == 4 ? 85 : 255;

        for (int i = 0; i < src->height; ++i) {
            auto* line = reinterpret_cast<QRgb*>(img.scanLine(i));
            for (int j = 0; j < src->width; ++j) {
                int alpha = (uint8_t)(data[j] * factor);
                line[j] = (alpha << 24) | fg;
            }
            data += base->bytes_per_line;
        }
        GQtW(w)->Painter()->drawImage(x, y, img);
    }
}

static void GQtDrawDrawImageMagnified(GWindow w, GImage *image, GRect *src, int32_t x, int32_t y, int32_t width, int32_t height) {
    Log(LOGDEBUG, " ");

    struct _GImage *base = (image->list_len == 0) ? image->u.image : image->u.images[0];
    double xscale, yscale;
    GRect viewable;

    viewable = w->ggc->clip;
    if (viewable.width > w->pos.width - viewable.x) {
        viewable.width = w->pos.width - viewable.x;
    }
    if (viewable.height > w->pos.height - viewable.y) {
        viewable.height = w->pos.height - viewable.y;
    }

    xscale = (base->width >= 1) ? ((double)(width)) / (base->width) : 1;
    yscale = (base->height >= 1) ? ((double)(height)) / (base->height) : 1;
    // Intersect the clip rectangle with the scaled image to find the
    // portion of screen that we want to draw
    if (viewable.x < x) {
        viewable.width -= (x - viewable.x);
        viewable.x = x;
    }
    if (viewable.y < y) {
        viewable.height -= (y - viewable.y);
        viewable.y = y;
    }
    if (viewable.x + viewable.width > x + width) {
        viewable.width = x + width - viewable.x;
    }
    if (viewable.y + viewable.height > y + height) {
        viewable.height = y + height - viewable.y;
    }
    if (viewable.height < 0 || viewable.width < 0) {
        return;
    }

    // Now find that same rectangle in the coordinates of the unscaled image
    // (translation & scale)
    viewable.x -= x;
    viewable.y -= y;

    GImage *temp = _GQtDraw_GImageExtract(base, &viewable, xscale, yscale);
    GRect tsrc;
    tsrc.x = tsrc.y = 0;
    tsrc.width = viewable.width;
    tsrc.height = viewable.height;
    GQtDrawDrawImage(w, temp, &tsrc, x + viewable.x, y + viewable.y);
}

static void GQtDrawDrawPixmap(GWindow w, GWindow pixmap, GRect *src, int32_t x, int32_t y) {
    // Log(LOGDEBUG, " ");
    GQtW(w)->Painter()->drawPixmap(x, y, *GQtW(pixmap)->Pixmap(), src->x, src->y, src->width, src->height);
}

static enum gcairo_flags GQtDrawHasCairo(GWindow UNUSED(w)) {
    // Log(LOGDEBUG, " ");
    return gc_all;
}

static void GQtDrawPathClose(GWindow w) {
    Log(LOGDEBUG, " ");
    GQtW(w)->Path()->closeSubpath();
}

static int GQtDrawFillRuleSetWinding(GWindow w) {
    Log(LOGDEBUG, " ");
    GQtW(w)->Path()->setFillRule(Qt::WindingFill);
    return 1;
}

static void GQtDrawPathMoveTo(GWindow w, double x, double y) {
    Log(LOGDEBUG, " ");
    GQtW(w)->Path()->moveTo(x, y);
}

static void GQtDrawPathLineTo(GWindow w, double x, double y) {
    Log(LOGDEBUG, " ");
    GQtW(w)->Path()->lineTo(x, y);
}

static void GQtDrawPathCurveTo(GWindow w, double cx1, double cy1, double cx2, double cy2, double x, double y) {
    Log(LOGDEBUG, " ");
    GQtW(w)->Path()->cubicTo(cx1, cy1, cx2, cy2, x, y);
}

static void GQtDrawPathStroke(GWindow w, Color col) {
    Log(LOGDEBUG, " ");
    w->ggc->fg = col;

    QPen pen = GQtDrawGetPen(w->ggc);
    GQtW(w)->Painter()->strokePath(*GQtW(w)->Path(), pen);
    GQtW(w)->Path()->clear();
}

static void GQtDrawPathFill(GWindow w, Color col) {
    Log(LOGDEBUG, " ");

    QBrush brush(ToQColor(col));
    GQtW(w)->Painter()->fillPath(*GQtW(w)->Path(), brush);
    GQtW(w)->Path()->clear();
}

static void GQtDrawPathFillAndStroke(GWindow w, Color fillcol, Color strokecol) {
    Log(LOGDEBUG, " ");
    // This function is unused, so it's unclear if it's implemented correctly.

    w->ggc->fg = strokecol;
    QPen pen = GQtDrawGetPen(w->ggc);
    QBrush brush(ToQColor(fillcol));
    GQtW(w)->Painter()->fillPath(*GQtW(w)->Path(), brush);
    GQtW(w)->Painter()->strokePath(*GQtW(w)->Path(), pen);
    GQtW(w)->Path()->clear();
}

static void GQtDrawPushClipOnly(GWindow w) {
    Log(LOGDEBUG, " ");

    GQtW(w)->Painter()->save();
    // HMMMMM
}

static void GQtDrawPathClipOnly(GWindow w) {
    Log(LOGDEBUG, " ");
    GQtW(w)->Painter()->setClipPath(*GQtW(w)->Path(), Qt::IntersectClip);
    GQtW(w)->Path()->clear();
}

static int GQtDrawDoText8(GWindow w, int32_t x, int32_t y, const char *text, int32_t cnt, Color col, enum text_funcs drawit,
                    struct tf_arg *arg) {
    // Log(LOGDEBUG, " ");

    struct font_instance *fi = w->ggc->fi;
    if (fi == nullptr || !text[0]) {
        return 0;
    }

    QFont fd = GQtDrawGetFont(fi);
    QFontMetrics fm(fd);
    QString qtext = QString::fromUtf8(text, cnt);
    if (drawit == tf_drawit) {
        y -= fm.ascent();// + fm.descent();
        QRect rct(x, y, w->ggc->clip.x + w->ggc->clip.width - x, w->ggc->clip.y + w->ggc->clip.height - y);
        if (!rct.isValid()) {
            return 0;
        }
        QRect bounds;
        GQtW(w)->Painter()->setFont(fd);
        GQtW(w)->Painter()->setPen(ToQColor(col));
        GQtW(w)->Painter()->drawText(rct, Qt::AlignLeft|Qt::AlignTop, qtext, &bounds);
        return bounds.width();
    } else if (drawit == tf_rect) {
        QRect br = fm.tightBoundingRect(qtext);
        arg->size.width = fm.horizontalAdvance(qtext);
        arg->size.lbearing = -br.x();
        arg->size.rbearing = br.width() - br.x();
        arg->size.fas = fm.ascent();
        arg->size.fds = fm.descent();
        arg->size.as = fm.ascent();
        arg->size.ds = br.height() - fm.ascent();
        return arg->size.width;
    }
    return fm.horizontalAdvance(qtext);
}

// PANGO LAYOUT
static void GQtDrawGetFontMetrics(GWindow w, GFont *fi, int *as, int *ds, int *ld) {
    // Log(LOGDEBUG, " ");
    QFont fd = GQtDrawGetFont(fi);
    QFontMetrics fm(fd);

    *as = fm.ascent();
    *ds = fm.descent();
    *ld = fm.leading();
}

static GQtLayoutState* DoLayout(GQtLayoutState* state) {
    if (state->laid_out) {
        return state;
    }

    qreal height = 0;
    qreal max_width = state->line_width < 0 ? INT_MAX : state->line_width;
    int leading = state->metrics->leading();

    state->utf8_line_starts.clear();
    state->layout.setCacheEnabled(true);
    state->layout.beginLayout();

    auto text = state->layout.text();
    const auto *uitr = state->utf8_text.c_str();
    int pos = 0;

    while (true) {
        QTextLine line = state->layout.createLine();
        if (!line.isValid()) {
            break;
        }
        line.setLineWidth(max_width);
        line.setPosition(QPointF(0, height));
        height += line.height() + leading;

        auto start = line.textStart();
        for (; pos < start; ++pos) {
            int ch = utf8_ildb(&uitr);
            if (ch > 0x10000) {
                ++pos;
            }
        }
        assert(pos == start);
        start = pos;
        state->utf8_line_starts.push_back(uitr - state->utf8_text.c_str());
        Log(LOGDEBUG, "LS (%d) %d vs %d", line.lineNumber(), line.textStart(), state->utf8_line_starts.back());
    }
    state->layout.endLayout();
    assert(state->layout.lineCount() == state->utf8_line_starts.size());

    state->laid_out = true;
    return state;
}

static void GQtDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi) {
    Log(LOGDEBUG, " ");

    auto* state = GQtW(w)->Layout();
    if (fi == NULL) {
        fi = w->ggc->fi;
    }

    state->laid_out = false;
    if (cnt < 0) {
        state->utf8_text = text;
    } else {
        state->utf8_text.assign(text, cnt);
    }

    // [QTBUG-95853] This is a bit of a hack, QTextLayouts are meant to be
    // used per paragraph. But for the cases where multiline text edits
    // are used, this is good enough, and is easier to work with.
    QString qtext = QString::fromUtf8(state->utf8_text.c_str(), state->utf8_text.size());
    qtext.replace('\n', QChar::LineSeparator);

    state->layout.setFont(GQtDrawGetFont(fi));
    state->layout.setText(qtext);
    state->metrics.reset(new QFontMetrics(state->layout.font()));
    state->line_width = -1;
}

static void GQtDrawLayoutDraw(GWindow w, int32_t x, int32_t y, Color fg) {
    Log(LOGDEBUG, " ");
    GQtWindow *gw = GQtW(w);
    auto* state = DoLayout(gw->Layout());
    QPointF pt(x, y - state->metrics->ascent());
    gw->Painter()->setPen(ToQColor(fg));
    state->layout.draw(gw->Painter(), pt);
}

static void GQtDrawLayoutIndexToPos(GWindow w, int index, GRect *pos) {
    Log(LOGDEBUG, " ");
    auto* state = DoLayout(GQtW(w)->Layout());
    if (state->utf8_line_starts.empty()) {
        *pos = {};
        return;
    }

    QTextLine line;
    int upos;
    if (index >= state->utf8_text.size()) {
        upos = state->layout.text().size();
        line = state->layout.lineAt(state->layout.lineCount() - 1);
    } else {
        auto it = std::lower_bound(state->utf8_line_starts.begin(), state->utf8_line_starts.end(), index);
        if (it == state->utf8_line_starts.end() || *it > index) {
            assert(it != state->utf8_line_starts.begin());
            --it;
        }

        int i = std::distance(state->utf8_line_starts.begin(), it);
        line = state->layout.lineAt(i);
        upos = line.textStart();

        const char* pt = state->utf8_text.c_str() + *it;
        const char* end = state->utf8_text.c_str() + index;
        for (; pt < end && (i = utf8_ildb(&pt)) > 0; ++upos) {
            if (i >= 0x10000) {
                ++upos;
            }
        }
    }

    auto x1 = line.cursorToX(upos, QTextLine::Leading);
    auto x2 = line.cursorToX(upos, QTextLine::Trailing);
    pos->x = x1;
    pos->width = x2 - x1;
    pos->y = line.y();
    pos->height = line.height();

    Log(LOGDEBUG, "HMM idx=%d pos=%d x=%d y=%d w=%d h=%d",
        index, upos, pos->x, pos->y, pos->width, pos->height);
}

static int GQtDrawLayoutXYToIndex(GWindow w, int x, int y) {
    Log(LOGDEBUG, " ");
    auto* state = DoLayout(GQtW(w)->Layout());

    // May not always be right? But this fn is only used with
    // A low/0 y-value anyway...
    int lineNumber = y / state->metrics->lineSpacing();
    QTextLine line = state->layout.lineAt(lineNumber);
    if (line.isValid()) {
        int curpos = line.xToCursor(x);
        int upos = state->utf8_line_starts[line.lineNumber()];
        const char *uitr = state->utf8_text.c_str() + upos;
        for (int pos = line.textStart(); pos < curpos; ++pos) {
            int ch = utf8_ildb(&uitr);
            if (ch > 0x10000) {
                ++pos;
            }
        }
        Log(LOGWARN, "VVV cp(%d) lsp(%d) -> %d", curpos, upos, (int)(uitr - state->utf8_text.c_str()));
        return uitr - state->utf8_text.c_str();
    }
    Log(LOGWARN, "MOPE");
    return 0;
}

static void GQtDrawLayoutExtents(GWindow w, GRect *size) {
    Log(LOGDEBUG, " ");
    auto* state = DoLayout(GQtW(w)->Layout());
    auto br = state->layout.boundingRect();
    size->x = br.x();
    size->y = br.y();
    size->width = br.width();
    size->height = br.height();
}

static void GQtDrawLayoutSetWidth(GWindow w, int width) {
    Log(LOGDEBUG, " ");
    auto* state = GQtW(w)->Layout();
    state->line_width = width;
    state->laid_out = false;
}

static int GQtDrawLayoutLineCount(GWindow w) {
    Log(LOGDEBUG, " ");
    auto* state = DoLayout(GQtW(w)->Layout());
    return state->layout.lineCount();
}

static int GQtDrawLayoutLineStart(GWindow w, int l) {
    Log(LOGDEBUG, " ");
    auto* state = DoLayout(GQtW(w)->Layout());
    assert(l >= 0 && l < state->utf8_line_starts.size());
    return state->utf8_line_starts[l];
}
// END PANGO LAYOUT


// END DRAW RELATED

}} // fontforge::gdraw

using namespace fontforge::gdraw;

// Our function VTable
static struct displayfuncs gqtfuncs = {
    nullptr, // GQtDrawInit -> Not needed

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
    GQtDrawPathClose,
    GQtDrawFillRuleSetWinding,
    GQtDrawPathMoveTo,
    GQtDrawPathLineTo,
    GQtDrawPathCurveTo,
    GQtDrawPathStroke,
    GQtDrawPathFill,
    GQtDrawPathFillAndStroke, // Currently unused
    GQtDrawPushClipOnly,
    GQtDrawPathClipOnly,

    GQtDrawLayoutInit,
    GQtDrawLayoutDraw,
    GQtDrawLayoutIndexToPos,
    GQtDrawLayoutXYToIndex,
    GQtDrawLayoutExtents,
    GQtDrawLayoutSetWidth,
    GQtDrawLayoutLineCount,
    GQtDrawLayoutLineStart,

    GQtDrawDoText8,
};

extern "C" GDisplay *_GQtDraw_CreateDisplay(char *displayname, int *argc, char ***argv) {
    LogInit();

    auto* gdisp = new GQtDisplay();
    gdisp->app.reset(new QApplication(*argc, *argv));
    gdisp->groot_base.reset(new GQtWidget(nullptr, Qt::Widget));
    gdisp->groot_base->setAttribute(Qt::WA_QuitOnClose, false);
    gdisp->groot_base->SetTitle("GROOT");

    GDrawResourceFind();
    gdisp->bs.double_time = _GDraw_res_multiclicktime;
    gdisp->bs.double_wiggle = _GDraw_res_multiclickwiggle;

    QRect screenGeom = gdisp->app->primaryScreen()->geometry();
    GDisplay *ret = gdisp->Base();
    ret->impl = gdisp;
    ret->funcs = &gqtfuncs;
    ret->fontstate = &gdisp->fs;
    ret->res = gdisp->fs.res = gdisp->app->primaryScreen()->logicalDotsPerInch();
    ret->groot = gdisp->groot_base->Base();
    ret->groot->display = (GDisplay*)gdisp;
    ret->groot->native_window = static_cast<GQtWindow*>(gdisp->groot_base.get());
    ret->groot->pos.width = screenGeom.width();
    ret->groot->pos.height = screenGeom.height();
    ret->groot->is_toplevel = true;
    ret->groot->is_visible = true;

    _GDraw_InitError(ret);
    return ret;
}

extern "C" void _GQtDraw_DestroyDisplay(GDisplay *disp) {
    delete GQtD(disp);
}

#endif // FONTFORGE_CAN_USE_QT
