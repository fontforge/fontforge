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
#include <memory>
#include <vector>

namespace fontforge { namespace gdraw {

class GQtWindow;
class GQtPixmap;
class GQtWidget;

struct GQtButtonState {
    uint32_t last_press_time;
    GQtWindow *release_w;
    int16_t release_x, release_y;
    int16_t release_button;
    int16_t cur_click;
    int16_t double_time;		// max milliseconds between release & click
    int16_t double_wiggle;	// max pixel wiggle allowed between release&click
};

struct GQtLayoutState {
    std::unique_ptr<QFontMetrics> metrics;
    QTextLayout layout;
    std::vector<int> utf8_line_starts;
    std::string utf8_text;
    int line_width = -1;
    bool laid_out = false;
};

struct GQtSelectionData {
    ~GQtSelectionData() {
        Release();
    }
    void Release() {
        if (data) {
            if (freedata) {
                (freedata)(data);
            } else {
                free(data);
            }
        }
    }
    QVariant Generate() const {
        void *ret = data;
        int32_t len = cnt;
        if (gendata) {
            ret = (gendata)(data, &len);
        }
        len *= unit_size;
        if (ret) {
            QByteArray rv((char*)ret, len);
            if (gendata) {
                free(ret);
            }
            return rv;
        }
        return QVariant();
    }

    // The identifier of the type of data held e.g. UTF8_STRING
    QString type;
    // Number of elements held
    int32_t cnt = 0;
    // Size per element held
    int32_t unit_size = 0;
    // Data of selection, or context data if generator is provided
    void *data = nullptr;
    // Function to generate selection data, or NULL if provided already
    void *(*gendata)(void *, int32_t *len) = nullptr;
    // Method by which to free the data element
    void (*freedata)(void *) = nullptr;
};

struct GQtSelectionInfo {
    GQtWindow *owner = nullptr;
    std::vector<std::unique_ptr<GQtSelectionData>> data;
};

class GQtMimeData : public QMimeData {
public:
    GQtMimeData(std::shared_ptr<GQtSelectionInfo> selinfo)
        : m_selinfo(std::move(selinfo))
    {}

    // QMimeData
    QStringList formats() const override;
    bool hasFormat(const QString& mimeType) const override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QVariant retrieveData(const QString &mimeType, QMetaType type) const override;
#else
    QVariant retrieveData(const QString &mimeType, QVariant::Type type) const override;
#endif

    void setSelInfo(std::shared_ptr<GQtSelectionInfo> selinfo) {
        m_selinfo = std::move(selinfo);
    }
private:
    std::shared_ptr<GQtSelectionInfo> m_selinfo;
};

class GQtWindow
{
public:
    GQtWindow(bool is_pixmap)
    {
        m_ggc.clip.width = m_ggc.clip.height = 0x7fff;
        m_ggc.fg = 0;
        m_ggc.bg = 0xffffff;
        m_base.is_pixmap = is_pixmap;
        m_base.ggc = &m_ggc;
    }
    virtual ~GQtWindow() = default;

    inline GWindow Base() const { return const_cast<GWindow>(&m_base); }

    virtual GQtPixmap* Pixmap() { throw std::runtime_error("Not a pixmap"); }
    virtual GQtWidget* Widget() { throw std::runtime_error("Not a widget"); }
    virtual QPainter* Painter() = 0;
    virtual void InitiateDestroy() = 0;

    inline void AddRef() { ++m_reference_count; }
    inline void DecRef() {
        --m_reference_count;
        if (m_reference_count <= 0) {
            assert(m_reference_count == 0);
            InitiateDestroy();
        }
    }

    inline const char* Title() const { return m_window_title.c_str(); }
    inline void SetTitle(const char *title) { m_window_title = title; }
    GCursor Cursor() const { return m_current_cursor; }
    void SetCursor(GCursor c) { m_current_cursor = c; }
    GQtLayoutState* Layout() { return &m_layout_state; }
    QPainterPath* Path() { return &m_path; }
    void ClearPath() {
#if QT_VERSION >= QT_VERSION_CHECK(5, 13, 0)
        m_path.clear();
#else
        m_path = QPainterPath();
#endif
    }

private:
    struct gwindow m_base = {};
    GGC m_ggc = {};

    int m_reference_count = 1;
    std::string m_window_title;
    GCursor m_current_cursor = ct_default;
    GQtLayoutState m_layout_state;
    QPainterPath m_path;
};

class GQtWidget : public QWidget, public GQtWindow
{
public:
    explicit GQtWidget(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags())
        : QWidget(parent, f)
        , GQtWindow(false)
    {
    }

    ~GQtWidget() override;

    template<typename E = void>
    GEvent InitEvent(event_type et, E* event = nullptr);
    void DispatchEvent(const GEvent& e, QEvent* trigger);

    bool event(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override {
        // QWidget::mousePressEvent(event);
        // if (!event->isAccepted()) {
            mouseEvent(event, et_mousedown);
        // }
    }
    void mouseReleaseEvent(QMouseEvent *event) override { mouseEvent(event, et_mouseup); }
    void wheelEvent(QWheelEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override {
        // QWidget::keyPressEvent(event);
        // if (!event->isAccepted()) {
            keyEvent(event, et_char);
        // }
    }
    void keyReleaseEvent(QKeyEvent *event) override { keyEvent(event, et_charup); }
    void paintEvent(QPaintEvent *event) override;
    void moveEvent(QMoveEvent *event) override { configureEvent(event); }
    void resizeEvent(QResizeEvent *event) override { configureEvent(event); }
    void showEvent(QShowEvent *event) override { mapEvent(event, true); }
    void hideEvent(QHideEvent *event) override { mapEvent(event, false); }
    void focusInEvent(QFocusEvent *event) override {
        QWidget::focusInEvent(event);
        focusEvent(event, true);
    }
    void focusOutEvent(QFocusEvent *event) override {
        QWidget::focusOutEvent(event);
        focusEvent(event, false);
    }
    void inputMethodEvent(QInputMethodEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent *event) override { crossingEvent(event, true); }
#else
    void enterEvent(QEvent *event) override { crossingEvent(event, true); }
#endif
    void leaveEvent(QEvent *event) override { crossingEvent(event, false); }
    void closeEvent(QCloseEvent *event) override;

    void keyEvent(QKeyEvent *event, event_type et);
    void mouseEvent(QMouseEvent* event, event_type et);
    void configureEvent(QEvent* event);
    void mapEvent(QEvent* event, bool visible);
    void focusEvent(QFocusEvent* event, bool focusIn);
    void crossingEvent(QEvent* event, bool enter);

    void SetICPos(int x, int y);
    GQtWidget* Widget() override { return this; }
    QPainter* Painter() override;
    void InitiateDestroy() override;

private:
    QPainter* m_painter = nullptr;
    int m_dispatch_depth = 0;
    bool m_destroying = false;
    bool m_ime_enabled = false;
    QPoint m_icpos = {};

    bool ShouldDispatch(const GEvent& e, QEvent* trigger);
};

class GQtPixmap : public QPixmap, public GQtWindow
{
public:
    explicit GQtPixmap(int w, int h)
        : QPixmap(w, h)
        , GQtWindow(true)
    {
    }

    GQtPixmap* Pixmap() override { return this; }
    QPainter* Painter() override {
        if (!m_painter.isActive()) {
            m_painter.begin(this);
            m_painter.setRenderHint(QPainter::Antialiasing);
            // m_painter.setRenderHint(QPainter::SmoothPixmapTransform);
        }
        return &m_painter;
    }
    void InitiateDestroy() override {
        delete this;
    }

private:
    QPainter m_painter;
};

class GQtTimer : public QTimer
{
public:
    explicit GQtTimer(GQtWindow *parent, void *userdata);
    GTimer* Base() const { return const_cast<GTimer*>(&m_gtimer); }
private:
    GTimer m_gtimer;
};

struct GQtDisplay
{
    struct gdisplay base;
    std::unique_ptr<QApplication> app;
    std::unique_ptr<GQtWidget> groot_base;
    std::vector<QCursor> custom_cursors;
    GQtWindow *default_icon = nullptr;

    bool is_space_pressed = false; // Used for GGDKDrawKeyState. We cheat!

    int top_window_count = 0; // The number of toplevel, non-dialogue windows. When this drops to 0, the event loop stops
    ulong last_event_time = 0;

    std::shared_ptr<GQtSelectionInfo> selinfo[sn_max];
    struct {
        GQtWindow *gw;
        int x, y;
        int rx, ry;
    } last_dd;

    FState fs = {};
    GQtButtonState bs = {};

    inline GDisplay* Base() const { return const_cast<GDisplay*>(&base); }
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
