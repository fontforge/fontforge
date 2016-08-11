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

// Private member functions (file-level)

static GGC *_GGDKDrawNewGGC() {
    GGC *ggc = calloc(1,sizeof(GGC));
    if (ggc == NULL) {
        fprintf(stderr, "GGC: Memory allocation failed!\n");
        return NULL;
    }

    ggc->clip.width = ggc->clip.height = 0x7fff;
    ggc->fg = 0;
    ggc->bg = 0xffffff;
    return ggc;
}

static GWindow _GGDKDraw_CreateWindow(GGDKDisplay *gdisp, GGDKWindow gw, GRect *pos,
    int (*eh)(GWindow, GEvent *), void *user_data, GWindowAttrs *wattrs) {

    GWindowAttrs temp = GWINDOWATTRS_EMPTY;
    GdkWindowAttr attribs = {0};
    gint attribs_mask = 0;
    GGDKWindow nw = (GGDKWindow)calloc(1, sizeof(struct ggdkwindow));

    if (nw == NULL) {
        fprintf(stderr, "_GFDKDraw_CreateWindow: GGDKWindow calloc failed.\n");
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

    nw->ggc = _GGDKDrawNewGGC();
    if (nw->ggc == NULL) {
        fprintf(stderr, "_GFDKDraw_CreateWindow: _GGDKDrawNewGGC returned NULL\n");
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
        char *title = NULL;
        // Icon titles are ignored.
        if ((wattrs->mask & wam_utf8_wtitle) && (wattrs->utf8_window_title != NULL)) {
            title = copy(wattrs->utf8_window_title);
        } else if ((wattrs->mask & wam_wtitle) && (wattrs->window_title != NULL)) {
            title = u2utf8_copy(wattrs->window_title);
        }

        attribs.title = title;
        attribs.type_hint = nw->is_dlg ? GDK_WINDOW_TYPE_HINT_DIALOG : GDK_WINDOW_TYPE_HINT_NORMAL;

        if (title != NULL) {
            attribs_mask |= GDK_WA_TITLE;
        }
        attribs_mask |= GDK_WA_TYPE_HINT;
    }

    // Event mask
    attribs.event_mask = GDK_EXPOSURE_MASK|GDK_STRUCTURE_MASK;
    if (attribs.window_type == GDK_WINDOW_TOPLEVEL) {
        attribs.event_mask |= GDK_FOCUS_CHANGE_MASK|GDK_ENTER_NOTIFY_MASK|GDK_LEAVE_NOTIFY_MASK;
    }
    if (wattrs->mask & wam_events) {
        if (wattrs->event_masks & (1 << et_char))
            attribs.event_mask |= GDK_KEY_PRESS_MASK;
        if (wattrs->event_masks & (1 << et_charup))
            attribs.event_mask |= GDK_KEY_RELEASE_MASK;
        if (wattrs->event_masks & (1 << et_mousemove))
            attribs.event_mask |= GDK_POINTER_MOTION_MASK;
        if (wattrs->event_masks & (1 << et_mousedown))
            attribs.event_mask |= GDK_BUTTON_PRESS_MASK;
        if (wattrs->event_masks & (1 << et_mouseup))
            attribs.event_mask |= GDK_BUTTON_RELEASE_MASK;
        //if ((wattrs->event_masks & (1 << et_mouseup)) && (wattrs->event_masks & (1 << et_mousedown)))
        //    attribs.event_mask |= OwnerGrabButtonMask;
        if (wattrs->event_masks & (1 << et_visibility))
            attribs.event_mask |= GDK_VISIBILITY_NOTIFY_MASK;
    }

    // Window position and size
    // I hate it placing stuff under the cursor...
    if (gw == gdisp->groot &&
        ((((wattrs->mask&wam_centered) && wattrs->centered)) ||
        ((wattrs->mask&wam_undercursor) && wattrs->undercursor) )) {
        pos->x = (gdisp->groot->pos.width-pos->width)/2;
        pos->y = (gdisp->groot->pos.height-pos->height)/2;
        if (wattrs->centered == 2) {
            pos->y = (gdisp->groot->pos.height-pos->height)/3;
        }
        nw->pos = *pos;
    }

    attribs.x = pos->x;
    attribs.y = pos->y;
    attribs.width = pos->width;
    attribs.height = pos->height;
    attribs_mask |= GDK_WA_X|GDK_WA_Y;

    // Window class
    attribs.wclass = GDK_INPUT_OUTPUT; // No hidden windows

    // Just use default GDK visual
    // attribs.visual = NULL;

    // Window type already set
    // attribs.window_type = ...;

    // Window cursor
    if ((wattrs->mask&wam_cursor) && wattrs->cursor != ct_default) {
        //SET CURSOR (UNHANDLED)
        //attribs_mask |= GDK_WA_CURSOR;
    }

    // Window manager name and class
    // GDK docs say to not use this. But that's because it's done by GTK...
    attribs.wmclass_name = GResourceProgramName;
    attribs.wmclass_class = GResourceProgramName;
    attribs_mask |= GDK_WA_WMCLASS;

    nw->w = gdk_window_new(gw->w, &attribs, attribs_mask);
    free(attribs.title);
    if (nw->w == NULL) {
        fprintf(stderr, "GGDKDraw: Failed to create window!\n");
        free(nw->ggc);
        free(nw);
        return NULL;
    }

    // Set background
    if (!(wattrs->mask&wam_backcol) || wattrs->background_color == COLOR_DEFAULT) {
        wattrs->background_color = gdisp->def_background;
    }
    nw->ggc->bg = wattrs->background_color;

    GdkRGBA col = {
        .red = COLOR_RED(wattrs->background_color)/255.,
        .green = COLOR_GREEN(wattrs->background_color)/255.,
        .blue = COLOR_BLUE(wattrs->background_color)/255.,
        .alpha = 1.
    };
    gdk_window_set_background_rgba(nw->w, &col);

    //TODO: Set window sizing/transient hints

    // Establish Cairo context
    gw->cc = gdk_cairo_create(nw->w);
    if (gw->cc == NULL) {
        fprintf(stderr, "GGDKDRAW: Cairo context creation failed!\n");
        gdk_window_destroy(nw->w);
        free(nw->ggc);
        free(nw);
        return NULL;
    }

    // Establish Pango layout context
    gw->pango_layout = pango_layout_new(gdisp->pangoc_context);
    if (gw->pango_layout == NULL) {
        fprintf(stderr, "GGDKDRAW: Pango layout creation failed!\n");
        cairo_destroy(gw->cc);
        gdk_window_destroy(nw->w);
        free(nw->ggc);
        free(nw);
        return NULL;
    }

    // Event handler
    if (eh != NULL) {
        GEvent e = {0};
        e.type = et_create;
        e.w = (GWindow) nw;
        e.native_window = (void *) (intpt) nw->w;
        (eh)((GWindow) nw, &e);
    }

    // Cairo...
    //_GXCDraw_NewWindow(nw);
    gdk_window_show(nw->w);
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

static GWindow _GGDKDraw_NewPixmap(GDisplay *gdisp, uint16 width, uint16 height, cairo_format_t format, unsigned char *data) {
    GGDKWindow gw = (GGDKWindow)calloc(1,sizeof(struct ggdkwindow));
    if (gw == NULL) {
        fprintf(stderr, "GGDKDRAW: GGDKWindow calloc failed!\n");
        return NULL;
    }

    gw->ggc = _GGDKDrawNewGGC();
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
    gw->pos.width = width; gw->pos.height = height;

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
    gw->cc = cairo_create(gw->cs);
    if (gw->cc == NULL) {
        fprintf(stderr, "GGDKDRAW: Cairo context creation failed!\n");
        cairo_surface_destroy(gw->cs);
        free(gw->ggc);
        free(gw);
        return NULL;
    }
    gw->pango_layout = pango_layout_new(((GGDKDisplay*)gdisp)->pangoc_context);
    if (gw->pango_layout == NULL) {
        fprintf(stderr, "GGDKDRAW: Pango layout context creation failed!\n");
        cairo_destroy(gw->cc);
        cairo_surface_destroy(gw->cs);
        free(gw->ggc);
        free(gw);
        return NULL;
    }
    return (GWindow)gw;
}

static void GGDKDrawInit(GDisplay *gdisp) {
    _GGDKDraw_InitFonts((GGDKDisplay *) gdisp);
}

//static void GGDKDrawTerm(GDisplay *gdisp){}
//static void* GGDKDrawNativeDisplay(GDisplay *gdisp){}

static void GGDKDrawSetDefaultIcon(GWindow icon) {
    // Closest is: gdk_window_set_icon_list
    // Store an icon list???
}

static GWindow GGDKDrawCreateTopWindow(GDisplay *gdisp, GRect *pos, int (*eh)(GWindow gw, GEvent *), void *user_data, GWindowAttrs *gattrs){
    fprintf(stderr, "GDKCALL: GGDKDrawCreateTopWindow\n");
    return _GGDKDraw_CreateWindow((GGDKDisplay *) gdisp, NULL, pos, eh, user_data, gattrs);
}

static GWindow GGDKDrawCreateSubWindow(GWindow gw, GRect *pos, int (*eh)(GWindow gw, GEvent *), void *user_data, GWindowAttrs *gattrs){
    fprintf(stderr, "GDKCALL: GGDKDrawCreateSubWindow\n");
    return _GGDKDraw_CreateWindow(((GGDKWindow) gw)->display, (GGDKWindow) gw, pos, eh, user_data, gattrs);
}

static GWindow GGDKDrawCreatePixmap(GDisplay *gdisp, uint16 width, uint16 height){
    fprintf(stderr, "GDKCALL: GGDKDrawCreatePixmap\n");

    //TODO: Check format?
    return _GGDKDraw_NewPixmap(gdisp, width, height, CAIRO_FORMAT_ARGB32, NULL);
}

static GWindow GGDKDrawCreateBitmap(GDisplay *gdisp, uint16 width, uint16 height, uint8 *data){
    fprintf(stderr, "GDKCALL: GGDKDrawCreateBitmap\n");

    return _GGDKDraw_NewPixmap(gdisp, width, height, CAIRO_FORMAT_A1, data);
}

static GCursor GGDKDrawCreateCursor(GWindow src, GWindow mask, Color fg, Color bg, int16 x, int16 y){
    fprintf(stderr, "GDKCALL: GGDKDrawCreateCursor\n");

    GGDKDisplay *gdisp = (GGDKDisplay *) (src->display);
    GdkDisplay *display = gdisp->display;
    cairo_surface_t *cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, src->pos.width, src->pos.height);
    cairo_t *cc = cairo_create(cs);

    // Masking
    cairo_mask_surface(cc, ((GGDKWindow)mask)->cs, 0, 0);
    //Background
    cairo_set_source_rgb(cc, COLOR_RED(bg)/255., COLOR_GREEN(bg)/255., COLOR_BLUE(bg)/255.);
    cairo_paint(cc);
    //Foreground
    cairo_mask_surface(cc, ((GGDKWindow)src)->cs, 0, 0);
    cairo_set_source_rgb(cc, COLOR_RED(fg)/255., COLOR_GREEN(bg)/255., COLOR_BLUE(bg)/255.);
    cairo_paint(cc);

    GdkCursor *cursor = gdk_cursor_new_from_surface(display, cs, x, y);
    cairo_destroy(cc);
    cairo_surface_destroy(cs);
}

static void GGDKDrawDestroyWindow(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawDestroyWindow\n");
    GGDKWindow gw = (GGDKWindow) w;

    g_object_unref(gw->pango_layout);
    cairo_destroy(gw->cc);
    gw->cc = NULL;
    if (gw->cs != NULL) {
        cairo_surface_destroy(gw->cs);
    }

    if (!gw->is_pixmap) {
        gw->is_dying = true;
        //if (gw->display->grab_window==w ) gw->display->grab_window = NULL;
        //XDestroyWindow(gw->display->display,gw->w);
        //Windows should be freed when we get the destroy event
        gdk_window_destroy(gw->w);
    }
}

static void GGDKDrawDestroyCursor(GDisplay *gdisp, GCursor gcursor){
    fprintf(stderr, "GDKCALL: GGDKDrawDestroyCursor\n"); assert(false);
}

static int GGDKDrawNativeWindowExists(GDisplay *gdisp, void *native_window){
    fprintf(stderr, "GDKCALL: GGDKDrawNativeWindowExists\n"); assert(false);
}

static void GGDKDrawSetZoom(GWindow gw, GRect *size, enum gzoom_flags flags){
    fprintf(stderr, "GDKCALL: GGDKDrawSetZoom\n"); assert(false);
}

static void GGDKDrawSetWindowBorder(GWindow gw, int width, Color gcol){
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowBorder\n"); assert(false);
}

static void GGDKDrawSetWindowBackground(GWindow gw, Color gcol){
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowBackground\n"); assert(false);
}

static int GGDKDrawSetDither(GDisplay *gdisp, int set){
    fprintf(stderr, "GDKCALL: GGDKDrawSetDither\n"); assert(false);
}


static void GGDKDrawReparentWindow(GWindow gw1, GWindow gw2, int x, int y){
    fprintf(stderr, "GDKCALL: GGDKDrawReparentWindow\n"); assert(false);
}

static void GGDKDrawSetVisible(GWindow gw, int set){
    fprintf(stderr, "GDKCALL: GGDKDrawSetVisible\n"); assert(false);
}

static void GGDKDrawMove(GWindow gw, int32 x, int32 y){
    fprintf(stderr, "GDKCALL: GGDKDrawMove\n"); assert(false);
}

static void GGDKDrawTrueMove(GWindow gw, int32 x, int32 y){
    fprintf(stderr, "GDKCALL: GGDKDrawTrueMove\n"); assert(false);
}

static void GGDKDrawResize(GWindow gw, int32 w, int32 h){
    fprintf(stderr, "GDKCALL: GGDKDrawResize\n"); assert(false);
}

static void GGDKDrawMoveResize(GWindow gw, int32 x, int32 y, int32 w, int32 h){
    fprintf(stderr, "GDKCALL: GGDKDrawMoveResize\n"); assert(false);
}

static void GGDKDrawRaise(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawRaise\n"); assert(false);
}

static void GGDKDrawRaiseAbove(GWindow gw1, GWindow gw2){
    fprintf(stderr, "GDKCALL: GGDKDrawRaiseAbove\n"); assert(false);
}

static int GGDKDrawIsAbove(GWindow gw1, GWindow gw2){
    fprintf(stderr, "GDKCALL: GGDKDrawIsAbove\n"); assert(false);
}

static void GGDKDrawLower(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawLower\n"); assert(false);
}

static void GGDKDrawSetWindowTitles(GWindow gw, const unichar_t *title, const unichar_t *icontitle){
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowTitles\n"); assert(false);
}

static void GGDKDrawSetWindowTitles8(GWindow gw, const char *title, const char *icontitle){
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowTitles8\n"); assert(false);
}

static unichar_t* GGDKDrawGetWindowTitle(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawGetWindowTitle\n"); assert(false);
}

static char* GGDKDrawGetWindowTitle8(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawGetWindowTitle8\n"); assert(false);
}

static void GGDKDrawSetTransientFor(GWindow gw1, GWindow gw2){
    fprintf(stderr, "GDKCALL: GGDKDrawSetTransientFor\n"); assert(false);
}

static void GGDKDrawGetPointerPosition(GWindow gw, GEvent *gevent){
    fprintf(stderr, "GDKCALL: GGDKDrawGetPointerPos\n"); assert(false);
}

static GWindow GGDKDrawGetPointerWindow(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawGetPointerWindow\n"); assert(false);
}

static void GGDKDrawSetCursor(GWindow gw, GCursor gcursor){
    fprintf(stderr, "GDKCALL: GGDKDrawSetCursor\n"); assert(false);
}

static GCursor GGDKDrawGetCursor(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawGetCursor\n"); assert(false);
}

static GWindow GGDKDrawGetRedirectWindow(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawGetRedirectWindow\n"); assert(false);
}

static void GGDKDrawTranslateCoordinates(GWindow from, GWindow to, GPoint *pt){
    fprintf(stderr, "GDKCALL: GGDKDrawTranslateCoordinates\n"); assert(false);
}


static void GGDKDrawBeep(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawBeep\n"); assert(false);
}

static void GGDKDrawFlush(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawFlush\n"); assert(false);
}


static void GGDKDrawPushClip(GWindow gw, GRect *rct, GRect *old){
    fprintf(stderr, "GDKCALL: GGDKDrawPushClip\n"); assert(false);
}

static void GGDKDrawPopClip(GWindow gw, GRect *old){
    fprintf(stderr, "GDKCALL: GGDKDrawPopClip\n"); assert(false);
}


static void GGDKDrawSetDifferenceMode(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawSetDifferenceMode\n"); assert(false);
}


static void GGDKDrawClear(GWindow gw, GRect *rect){
    fprintf(stderr, "GDKCALL: GGDKDrawClear\n"); assert(false);
}

static void GGDKDrawDrawLine(GWindow gw, int32 x, int32 y, int32 xend, int32 yend, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawDrawLine\n"); assert(false);
}

static void GGDKDrawDrawArrow(GWindow gw, int32 x, int32 y, int32 xend, int32 yend, int16 arrows, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawDrawArrow\n"); assert(false);
}

static void GGDKDrawDrawRect(GWindow gw, GRect *rect, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawDrawRect\n"); assert(false);
}

static void GGDKDrawFillRect(GWindow gw, GRect *rect, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawFillRect\n"); assert(false);
}

static void GGDKDrawFillRoundRect(GWindow gw, GRect *rect, int radius, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawFillRoundRect\n"); assert(false);
}

static void GGDKDrawDrawEllipse(GWindow gw, GRect *rect, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawDrawEllipse\n"); assert(false);
}

static void GGDKDrawFillEllipse(GWindow gw, GRect *rect, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawFillEllipse\n"); assert(false);
}

static void GGDKDrawDrawArc(GWindow gw, GRect *rect, int32 sangle, int32 eangle, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawDrawArc\n"); assert(false);
}

static void GGDKDrawDrawPoly(GWindow gw, GPoint *pts, int16 cnt, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawDrawPoly\n"); assert(false);
}

static void GGDKDrawFillPoly(GWindow gw, GPoint *pts, int16 cnt, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawFillPoly\n"); assert(false);
}

static void GGDKDrawScroll(GWindow gw, GRect *rect, int32 hor, int32 vert){
    fprintf(stderr, "GDKCALL: GGDKDrawScroll\n"); assert(false);
}


static void GGDKDrawDrawImage(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y){
    fprintf(stderr, "GDKCALL: GGDKDrawDrawImage\n"); assert(false);
}

static void GGDKDrawTileImage(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y){
    fprintf(stderr, "GDKCALL: GGDKDrawTileImage\n"); assert(false);
}

static void GGDKDrawDrawGlyph(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y){
    fprintf(stderr, "GDKCALL: GGDKDrawDrawGlyph\n"); assert(false);
}

static void GGDKDrawDrawImageMagnified(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y, int32 width, int32 height){
    fprintf(stderr, "GDKCALL: GGDKDrawDrawImageMag\n"); assert(false);
}

static GImage* GGDKDrawCopyScreenToImage(GWindow gw, GRect *rect){
    fprintf(stderr, "GDKCALL: GGDKDrawCopyScreenToImage\n"); assert(false);
}

static void GGDKDrawDrawPixmap(GWindow gw1, GWindow gw2, GRect *src, int32 x, int32 y){
    fprintf(stderr, "GDKCALL: GGDKDrawDrawPixmap\n"); assert(false);
}

static void GGDKDrawTilePixmap(GWindow gw1, GWindow gw2, GRect *src, int32 x, int32 y){
    fprintf(stderr, "GDKCALL: GGDKDrawTilePixmap\n"); assert(false);
}


static GIC* GGDKDrawCreateInputContext(GWindow gw, enum gic_style style){
    fprintf(stderr, "GDKCALL: GGDKDrawCreateInputContext\n"); assert(false);
}

static void GGDKDrawSetGIC(GWindow gw, GIC *gic, int x, int y){
    fprintf(stderr, "GDKCALL: GGDKDrawSetGIC\n"); assert(false);
}


static void GGDKDrawGrabSelection(GWindow w, enum selnames sel){
    fprintf(stderr, "GDKCALL: GGDKDrawGrabSelection\n"); assert(false);
}

static void GGDKDrawAddSelectionType(GWindow w, enum selnames sel, char *type, void *data, int32 cnt, int32 unitsize, void *gendata(void *, int32 *len), void freedata(void *)){
    fprintf(stderr, "GDKCALL: GGDKDrawAddSelectionType\n"); assert(false);
}

static void* GGDKDrawRequestSelection(GWindow w, enum selnames sn, char *typename, int32 *len){
    fprintf(stderr, "GDKCALL: GGDKDrawRequestSelection\n"); assert(false);
}

static int GGDKDrawSelectionHasType(GWindow w, enum selnames sn, char *typename){
    fprintf(stderr, "GDKCALL: GGDKDrawSelectionHasType\n"); assert(false);
}

static void GGDKDrawBindSelection(GDisplay *gdisp, enum selnames sn, char *atomname){
    fprintf(stderr, "GDKCALL: GGDKDrawBindSelection\n"); //assert(false); //TODO: Implement selections (clipboard)
}

static int GGDKDrawSelectionHasOwner(GDisplay *gdisp, enum selnames sn){
    fprintf(stderr, "GDKCALL: GGDKDrawSelectionHasOwner\n"); assert(false);
}


static void GGDKDrawPointerUngrab(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawPointerUngrab\n"); assert(false);
}

static void GGDKDrawPointerGrab(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawPointerGrab\n"); assert(false);
}

static void GGDKDrawRequestExpose(GWindow gw, GRect *gr, int set){
    fprintf(stderr, "GDKCALL: GGDKDrawRequestExpose\n"); assert(false);
}

static void GGDKDrawForceUpdate(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawForceUpdate\n"); assert(false);
}

static void GGDKDrawSync(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawSync\n"); assert(false);
}

static void GGDKDrawSkipMouseMoveEvents(GWindow gw, GEvent *gevent){
    fprintf(stderr, "GDKCALL: GGDKDrawSkipMouseMoveEvents\n"); assert(false);
}

static void GGDKDrawProcessPendingEvents(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawProcessPendingEvents\n"); assert(false);
}

static void GGDKDrawProcessWindowEvents(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawProcessWindowEvents\n"); assert(false);
}

static void GGDKDrawProcessOneEvent(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawProcessOneEvent\n"); assert(false);
}

static void GGDKDrawEventLoop(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawEventLoop\n"); assert(false);
}

static void GGDKDrawPostEvent(GEvent *e){
    fprintf(stderr, "GDKCALL: GGDKDrawPostEvent\n"); assert(false);
}

static void GGDKDrawPostDragEvent(GWindow w, GEvent *mouse, enum event_type et){
    fprintf(stderr, "GDKCALL: GGDKDrawPostDragEvent\n"); assert(false);
}

static int GGDKDrawRequestDeviceEvents(GWindow w, int devcnt, struct gdeveventmask *de){
    fprintf(stderr, "GDKCALL: GGDKDrawRequestDeviceEvents\n"); assert(false);
}


static GTimer* GGDKDrawRequestTimer(GWindow w, int32 time_from_now, int32 frequency, void *userdata){
    fprintf(stderr, "GDKCALL: GGDKDrawRequestTimer\n"); assert(false);
}

static void GGDKDrawCancelTimer(GTimer *timer){
    fprintf(stderr, "GDKCALL: GGDKDrawCancelTimer\n"); assert(false);
}


static void GGDKDrawSyncThread(GDisplay *gdisp, void (*func)(void *), void *data){
    fprintf(stderr, "GDKCALL: GGDKDrawSyncThread\n"); assert(false);
}


static GWindow GGDKDrawPrinterStartJob(GDisplay *gdisp, void *user_data, GPrinterAttrs *attrs){
    fprintf(stderr, "GDKCALL: GGDKDrawStartJob\n"); assert(false);
}

static void GGDKDrawPrinterNextPage(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawNextPage\n"); assert(false);
}

static int GGDKDrawPrinterEndJob(GWindow w, int cancel){
    fprintf(stderr, "GDKCALL: GGDKDrawEndJob\n"); assert(false);
}


static void GGDKDrawGetFontMetrics(GWindow gw, GFont *gfont, int *as, int *ds, int *ld){
    fprintf(stderr, "GDKCALL: GGDKDrawGetFontMetrics\n"); assert(false);
}


static enum gcairo_flags GGDKDrawHasCairo(GWindow w){
    fprintf(stderr, "GDKCALL: gcairo_flags GGDKDrawHasCairo\n"); assert(false);
}


static void GGDKDrawPathStartNew(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawStartNewPath\n"); assert(false);
}

static void GGDKDrawPathClose(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawClosePath\n"); assert(false);
}

static void GGDKDrawPathMoveTo(GWindow w, double x, double y){
    fprintf(stderr, "GDKCALL: GGDKDrawMoveto\n"); assert(false);
}

static void GGDKDrawPathLineTo(GWindow w, double x, double y){
    fprintf(stderr, "GDKCALL: GGDKDrawLineto\n"); assert(false);
}

static void GGDKDrawPathCurveTo(GWindow w, double cx1, double cy1, double cx2, double cy2, double x, double y){
    fprintf(stderr, "GDKCALL: GGDKDrawCurveto\n"); assert(false);
}

static void GGDKDrawPathStroke(GWindow w, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawStroke\n"); assert(false);
}

static void GGDKDrawPathFill(GWindow w, Color col){
    fprintf(stderr, "GDKCALL: GGDKDrawFill\n"); assert(false);
}

static void GGDKDrawPathFillAndStroke(GWindow w, Color fillcol, Color strokecol){
    fprintf(stderr, "GDKCALL: GGDKDrawFillAndStroke\n"); assert(false);
}


static void GGDKDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutInit\n"); assert(false);
}

static void GGDKDrawLayoutDraw(GWindow w, int32 x, int32 y, Color fg){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutDraw\n"); assert(false);
}

static void GGDKDrawLayoutIndexToPos(GWindow w, int index, GRect *pos){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutIndexToPos\n"); assert(false);
}

static int GGDKDrawLayoutXYToIndex(GWindow w, int x, int y){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutXYToIndex\n"); assert(false);
}

static void GGDKDrawLayoutExtents(GWindow w, GRect *size){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutExtents\n"); assert(false);
}

static void GGDKDrawLayoutSetWidth(GWindow w, int width){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutSetWidth\n"); assert(false);
}

static int GGDKDrawLayoutLineCount(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutLineCount\n"); assert(false);
}

static int GGDKDrawLayoutLineStart(GWindow w, int line){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutLineStart\n"); assert(false);
}

static void GGDKDrawStartNewSubPath(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawStartNewSubPath\n"); assert(false);
}

static int GGDKDrawFillRuleSetWinding(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawFillRuleSetWinding\n"); assert(false);
}


static void GGDKDrawPushClipOnly(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawPushClipOnly\n"); assert(false);
}

static void GGDKDrawClipPreserve(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawClipPreserve\n"); assert(false);
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

    gdisp = (GGDKDisplay*)calloc(1, sizeof(GGDKDisplay));
    if (gdisp == NULL) {
        gdk_display_close(display);
        return NULL;
    }

    gdisp->funcs = &gdkfuncs;
    gdisp->display = display;
    gdisp->screen = gdk_display_get_default_screen(display);
    gdisp->root = gdk_screen_get_root_window(gdisp->screen);
    gdisp->res = gdk_screen_get_resolution(gdisp->screen);
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
    groot->ggc = _GGDKDrawNewGGC();
    groot->display = gdisp;
    groot->w = gdisp->root;
    groot->pos.width = gdk_screen_get_width(gdisp->screen);
    groot->pos.height = gdk_screen_get_height(gdisp->screen);
    groot->is_toplevel = true;
    groot->is_visible = true;

    //GGDKResourceInit(gdisp,programname);

    gdisp->def_background = GResourceFindColor("Background", COLOR_CREATE(0xf5,0xff,0xfa));
    gdisp->def_foreground = GResourceFindColor("Foreground", COLOR_CREATE(0x00,0x00,0x00));
    if (GResourceFindBool("Synchronize", false)) {
        gdk_display_sync(gdisp->display);
    }

    (gdisp->funcs->init)((GDisplay *) gdisp);
    //gdisp->top_window_count = 0; //Reference counting toplevel windows?

    _GDraw_InitError((GDisplay *) gdisp);

    return (GDisplay *)gdisp;
}

void _GGDKDraw_DestroyDisplay(GDisplay *gdisp) {
    GGDKDisplay* gdispc = (GGDKDisplay*)(gdisp);

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
