/* Copyright (C) 2022 by Jeremy Tan */
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

#ifndef FONTFORGE_GQTDRAWP_H
#define FONTFORGE_GQTDRAWP_H

#include <fontforge-config.h>

#ifdef FONTFORGE_CAN_USE_QT

#include "fontP.h"
#include "gdrawlogger.h"
#include "gdrawP.h"
#include "gkeysym.h"

#include <QtWidgets>
#include <vector>

namespace fontforge { namespace gdraw {

struct GQtWindow;

typedef struct gqtbuttonstate {
    uint32_t last_press_time;
    GQtWindow *release_w;
    int16_t release_x, release_y;
    int16_t release_button;
    int16_t cur_click;
    int16_t double_time;		// max milliseconds between release & click
    int16_t double_wiggle;	// max pixel wiggle allowed between release&click
} GQtButtonState;

struct GQtWidget : QWidget
{
    explicit GQtWidget(GQtWindow *base, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags())
    : QWidget(parent, f)
    , gwindow(base)
    {}

    ~GQtWidget()
    {
        Log(LOGDEBUG, "CLEANUP %p", this);
    }

    template<typename E = void>
    GEvent InitEvent(event_type et, E* event = nullptr);
    void DispatchEvent(const GEvent& e);

    void keyPressEvent(QKeyEvent *event) override { keyEvent(event, et_char); }
    void keyReleaseEvent(QKeyEvent *event) override { keyEvent(event, et_charup); }
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override { configureEvent(); }
    void moveEvent(QMoveEvent *event) override { configureEvent(); }
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override { mouseEvent(event, et_mousedown); }
    void mouseReleaseEvent(QMouseEvent *event) override { mouseEvent(event, et_mouseup); }
    void showEvent(QShowEvent *event) override { mapEvent(true); }
    void hideEvent(QHideEvent *event) override { mapEvent(false); }
    void focusInEvent(QFocusEvent *event) override { focusEvent(event, true); }
    void focusOutEvent(QFocusEvent *event) override { focusEvent(event, false); }
    void enterEvent(QEvent *event) override { crossingEvent(event, true); }
    void leaveEvent(QEvent *event) override { crossingEvent(event, false); }
    void closeEvent(QCloseEvent *event) override;
    void configureEvent();
    void keyEvent(QKeyEvent *event, event_type et);
    void mouseEvent(QMouseEvent* event, event_type et);
    void mapEvent(bool visible);
    void focusEvent(QFocusEvent* event, bool focusIn);
    void crossingEvent(QEvent* event, bool enter);
    
    QPainter* Painter();

    std::unique_ptr<QPainter> painter;
    GQtWindow *gwindow = nullptr;
};

struct GQtPixmap : QPixmap
{
    explicit GQtPixmap(int w, int h)
        : QPixmap(w, h)
    {}

    QPainter* Painter() {
        if (!painter.isActive()) {
            painter.begin(this);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::HighQualityAntialiasing);
        }
        return &painter;
    }

    QPainter painter;
};

struct GQtTimer : QTimer
{
    explicit GQtTimer(GQtWindow *parent, void *userdata);
    GTimer* Base() const { return const_cast<GTimer*>(&gtimer); }

    GTimer gtimer;
};

struct GQtDisplay
{
    struct gdisplay base;
    std::unique_ptr<QApplication> app;
    std::vector<QCursor> custom_cursors;
    GQtWindow *default_icon = nullptr;

    bool is_space_pressed = false; // Used for GGDKDrawKeyState. We cheat!

    int top_window_count = 0; // The number of toplevel, non-dialogue windows. When this drops to 0, the event loop stops
    ulong last_event_time = 0;
    GQtWindow *grabbed_window = nullptr;

    GQtButtonState bs;

    inline GDisplay* Base() const { return const_cast<GDisplay*>(&base); }
};

struct GQtWindow
{
    struct LayoutState {
        QFont fd;
        QString text;
    };

    struct gwindow base;
    void *q_base = nullptr;

    unsigned int is_dlg: 1;
    unsigned int was_positioned: 1;
    unsigned int restrict_input_to_me: 1;/* for dialogs, no input outside of dlg */
    unsigned int istransient: 1;	/* has transient for hint set */
    unsigned int isverytransient: 1;
    unsigned int is_cleaning_up: 1; //Are we running cleanup?
    unsigned int is_waiting_for_selection: 1;
    unsigned int is_notified_of_selection: 1;
    unsigned int is_in_paint: 1; // Have we called gdk_window_begin_paint_region?

    std::string window_title;
    GCursor current_cursor = ct_default;

    std::unique_ptr<LayoutState> layout_state;

    inline GWindow Base() const { return const_cast<GWindow>(&base); }

    inline GQtPixmap* Pixmap() const {
        assert(base.is_pixmap);
        return static_cast<GQtPixmap*>(q_base);
    }

    inline QPainter* Painter() {
        if (base.is_pixmap) {
            return Pixmap()->Painter();
        } else {
            return Widget()->Painter();
        }
    }

    inline GQtWidget* Widget() const {
        assert(!base.is_pixmap);
        return static_cast<GQtWidget*>(q_base);
    }
};

static inline GQtDisplay* GQtD(GDisplay *d) {
    return static_cast<GQtDisplay*>(d->impl);
}

static inline GQtDisplay* GQtD(GWindow w) {
    return static_cast<GQtDisplay*>(w->display->impl);
}

static inline GQtDisplay* GQtD(GQtWindow* w) {
    return GQtD(w->Base());
}

static inline GQtWindow* GQtW(GWindow w) {
    return static_cast<GQtWindow*>(w->native_window);
}

}} // fontforge::gdraw

#endif // FONTFORGE_CAN_USE_QT

#endif /* FONTFORGE_GQTDRAWP_H */
