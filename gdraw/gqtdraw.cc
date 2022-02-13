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

#include <QtWidgets>
#include <type_traits>

static GGC *_GQtDraw_NewGGC(void) {
    GGC *ggc = (GGC *)calloc(1, sizeof(GGC));
    if (ggc == NULL) {
        Log(LOGDEBUG, "GGC: Memory allocation failed!");
        return NULL;
    }

    ggc->clip.width = ggc->clip.height = 0x7fff;
    ggc->fg = 0;
    ggc->bg = 0xffffff;
    return ggc;
}

static void GQtDrawInit(GDisplay *gdisp) {
    FState *fs = (FState *)calloc(1, sizeof(FState));
    if (fs == NULL) {
        Log(LOGDEBUG, "GQtDraw: FState alloc failed!");
        assert(false);
    }

    // In inches, because that's how fonts are measured
    gdisp->fontstate = fs;
    fs->res = gdisp->res;
}

static void GQtDrawSetDefaultIcon(GWindow icon) {
}

static GWindow GQtDrawCreateTopWindow(GDisplay *gdisp, GRect *pos, int (*eh)(GWindow gw, GEvent *), void *user_data,
                                       GWindowAttrs *gattrs) {
    Log(LOGDEBUG, " ");
    return nullptr;
}

static GWindow GQtDrawCreateSubWindow(GWindow gw, GRect *pos, int (*eh)(GWindow gw, GEvent *), void *user_data,
                                       GWindowAttrs *gattrs) {
    Log(LOGDEBUG, " ");
    return nullptr;
}

static GWindow GQtDrawCreatePixmap(GDisplay *gdisp, GWindow similar, uint16 width, uint16 height) {
    Log(LOGDEBUG, " ");

    return nullptr;
}

static GWindow GQtDrawCreateBitmap(GDisplay *gdisp, uint16 width, uint16 height, uint8 *data) {
    Log(LOGDEBUG, " ");
    return nullptr;
}

static GCursor GQtDrawCreateCursor(GWindow src, GWindow mask, Color fg, Color bg, int16 x, int16 y) {
    Log(LOGDEBUG, " ");
    return ct_default;
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
}

static void GQtDrawMove(GWindow gw, int32 x, int32 y) {
    Log(LOGDEBUG, "%p:%s, %d %d", gw, /* ((GQtWindow)gw)->window_title */ nullptr, x, y);
}

static void GQtDrawTrueMove(GWindow w, int32 x, int32 y) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawResize(GWindow gw, int32 w, int32 h) {
    // Log(LOGDEBUG, "%p:%s, %d %d", gw, /* ((GQtWindow)gw)->window_title */ nullptr, w, h);
}

static void GQtDrawMoveResize(GWindow gw, int32 x, int32 y, int32 w, int32 h) {
    // Log(LOGDEBUG, "%p:%s, %d %d %d %d", gw, /* ((GQtWindow)gw)->window_title */ nullptr, x, y, w, h);
}

static void GQtDrawRaise(GWindow w) {
    GQtWindow gw = (GQtWindow) w;
    Log(LOGDEBUG, "%p[%p][%s]", gw, w->native_window, /* gw->window_title */ nullptr);
}

// Icon title is ignored.
static void GQtDrawSetWindowTitles8(GWindow w, const char *title, const char *UNUSED(icontitle)) {
    Log(LOGDEBUG, " ");// assert(false);
}

// Sigh. GDK doesn't provide a way to get the window title...
static char *GQtDrawGetWindowTitle8(GWindow gw) {
    Log(LOGDEBUG, " ");
    return nullptr;
}

static void GQtDrawSetTransientFor(GWindow transient, GWindow owner) {
    Log(LOGDEBUG, "transient=%p, owner=%p", transient, owner);
}

static void GQtDrawGetPointerPosition(GWindow w, GEvent *ret) {
    Log(LOGDEBUG, " ");

}

static GWindow GQtDrawGetPointerWindow(GWindow gw) {
    Log(LOGDEBUG, " ");
    return nullptr;
}

static void GQtDrawSetCursor(GWindow w, GCursor gcursor) {
    Log(LOGDEBUG, " ");
}

static GCursor GQtDrawGetCursor(GWindow gw) {
    Log(LOGDEBUG, " ");
    return ct_default;
}

static void GQtDrawTranslateCoordinates(GWindow from, GWindow to, GPoint *pt) {
    Log(LOGDEBUG, " ");
    *pt = {0};
}


static void GQtDrawBeep(GDisplay *gdisp) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawScroll(GWindow w, GRect *rect, int32 hor, int32 vert) {
    Log(LOGDEBUG, " ");
    GRect temp;

    vert = -vert;
    if (rect == NULL) {
        temp.x = temp.y = 0;
        temp.width = w->pos.width;
        temp.height = w->pos.height;
        rect = &temp;
    }

    GDrawRequestExpose(w, rect, false);
}


static GIC *GQtDrawCreateInputContext(GWindow UNUSED(gw), enum gic_style UNUSED(style)) {
    Log(LOGDEBUG, " ");
    return NULL;
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

static void GQtDrawPointerUngrab(GDisplay *gdisp) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawPointerGrab(GWindow w) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawRequestExpose(GWindow w, GRect *rect, int UNUSED(doclear)) {
    Log(LOGDEBUG, "%p [%s]", w, /* ((GQtWindow)w)->window_title */ nullptr);

    // GQtWindow gw = (GQtWindow) w;
    // GdkRectangle clip;

    // if (!gw->is_visible || _GQtDraw_WindowOrParentsDying(gw)) {
    //     return;
    // }
    // if (rect == NULL) {
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

static void GQtDrawForceUpdate(GWindow gw) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawSync(GDisplay *gdisp) {
    // Log(LOGDEBUG, " ");
}

static void GQtDrawSkipMouseMoveEvents(GWindow UNUSED(gw), GEvent *UNUSED(gevent)) {
    //Log(LOGDEBUG, " ");
    // Not implemented, not needed.
}

static void GQtDrawProcessPendingEvents(GDisplay *gdisp) {
    //Log(LOGDEBUG, " ");
}

static void GQtDrawProcessOneEvent(GDisplay *gdisp) {
    //Log(LOGDEBUG, " ");
}

static void GQtDrawEventLoop(GDisplay *gdisp) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawPostEvent(GEvent *e) {
    //Log(LOGDEBUG, " ");
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

static GTimer *GQtDrawRequestTimer(GWindow w, int32 time_from_now, int32 frequency, void *userdata) {
    //Log(LOGDEBUG, " ");
    return nullptr;
}

static void GQtDrawCancelTimer(GTimer *timer) {
    //Log(LOGDEBUG, " ");
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

static void GQtDrawDrawArrow(GWindow w, int32 x, int32 y, int32 xend, int32 yend, int16 arrows, Color col) {
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

static void GQtDrawDrawPoly(GWindow w, GPoint *pts, int16 cnt, Color col) {
    Log(LOGDEBUG, " ");
}

static void GQtDrawFillPoly(GWindow w, GPoint *pts, int16 cnt, Color col) {
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
static void GQtDrawGetFontMetrics(GWindow gw, GFont *fi, int *as, int *ds, int *ld) {
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

GDisplay *_GQtDraw_CreateDisplay(char *displayname, char *UNUSED(programname)) {
    static_assert(std::is_standard_layout<GQtDisplay>::value, "Is std layout");


    static int argc = 1;
    static char *argv[] = {"a"};
    std::unique_ptr<QGuiApplication> app(new QGuiApplication(argc, argv));
    std::unique_ptr<GQtDisplay>gdisp(new GQtDisplay());

    gdisp->base.funcs = &gqtfuncs;
    gdisp->app = app.get();

    GQtWindow groot = (GQtWindow)calloc(1, sizeof(struct gqtwindow));
    if (groot == NULL) {
        return nullptr;
    }

    QRect screenGeom = app->primaryScreen()->geometry();

    gdisp->base.groot = (GWindow)groot;
    groot->base.ggc = _GQtDraw_NewGGC();
    groot->base.display = (GDisplay*)gdisp.get();
    groot->base.native_window = groot;
    groot->base.pos.width = screenGeom.width();
    groot->base.pos.height = screenGeom.height();
    groot->base.is_toplevel = true;
    groot->base.is_visible = true;

    (gdisp->base.funcs->init)((GDisplay *) gdisp.get());
    _GDraw_InitError((GDisplay *) gdisp.get());

    app.release();
    return reinterpret_cast<GDisplay*>(gdisp.release());
}

void _GQtDraw_DestroyDisplay(GDisplay *disp) {
    auto* gdisp = reinterpret_cast<GQtDisplay*>(disp);
    std::unique_ptr<QGuiApplication> app(gdisp->app);
    delete gdisp;
}

#endif // FONTFORGE_CAN_USE_QT
