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

struct GQtDisplay
{
    struct gdisplay base;
    std::unique_ptr<QGuiApplication> app;
    std::vector<QCursor> custom_cursors;
    GQtWindow *default_icon = nullptr;

    inline GDisplay* operator()() const { return &base; }
};

struct GQtWindow
{
    struct gwindow base;
    void *q_base = nullptr;

    std::string window_title;
    GCursor current_cursor = ct_default;

    inline GWindow operator()() const { return &base; }

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

struct GQtWidget : QWidget
{
    GQtWindow *gwindow;
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
