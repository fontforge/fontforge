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

#include <QtWidgets>
#include <vector>

namespace fontforge { namespace gdraw {

struct GQtWindow;

struct GQtWidget : QWidget
{
    GQtWindow *gwindow = nullptr;
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

    int top_window_count = 0; // The number of toplevel, non-dialogue windows. When this drops to 0, the event loop stops
    GQtWindow *grabbed_window = nullptr;

    inline GDisplay* Base() const { return const_cast<GDisplay*>(&base); }
};

struct GQtWindow
{
    struct gwindow base;
    void *q_base = nullptr;

    unsigned int is_dlg: 1;
    unsigned int was_positioned: 1;
    unsigned int restrict_input_to_me: 1;/* for dialogs, no input outside of dlg */
    unsigned int istransient: 1;	/* has transient for hint set */
    unsigned int isverytransient: 1;
    unsigned int is_cleaning_up: 1; //Are we running cleanup?
    unsigned int is_centered: 1;
    unsigned int is_waiting_for_selection: 1;
    unsigned int is_notified_of_selection: 1;
    unsigned int is_in_paint: 1; // Have we called gdk_window_begin_paint_region?

    std::string window_title;
    GCursor current_cursor = ct_default;

    inline GWindow Base() const { return const_cast<GWindow>(&base); }

    inline QPixmap* Pixmap() const {
        assert(base.is_pixmap);
        return static_cast<QPixmap*>(q_base);
    }

    inline GQtWidget* Widget() const {
        assert(!base.is_pixmap);
        return static_cast<GQtWidget*>(q_base);
    }

    inline GQtDisplay* Display() const
    {
        return static_cast<GQtDisplay*>(base.display->impl);
    }
};

static inline GQtDisplay* GQtD(GDisplay *d) {
    return static_cast<GQtDisplay*>(d->impl);
}

static inline GQtDisplay* GQtD(GWindow w) {
    return static_cast<GQtDisplay*>(w->display->impl);
}

static inline GQtWindow* GQtW(GWindow w) {
    return static_cast<GQtWindow*>(w->native_window);
}

}} // fontforge::gdraw

#endif // FONTFORGE_CAN_USE_QT

#endif /* FONTFORGE_GQTDRAWP_H */
