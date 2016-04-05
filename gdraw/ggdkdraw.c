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

static bool _GGDKDraw_InitPangoCairo(GGDKWindow gw) {
    if (gw->is_pixmap) {
        gw->cc = cairo_create(gw->cs);
    } else {
        gw->cc = gdk_cairo_create(gw->w);
    }

    if (gw->cc == NULL) {
        fprintf(stderr, "GGDKDRAW: Cairo context creation failed!\n");
        return false;
    }

    // Establish Pango layout context
    gw->pango_layout = pango_layout_new(gw->display->pangoc_context);
    if (gw->pango_layout == NULL) {
        fprintf(stderr, "GGDKDRAW: Pango layout creation failed!\n");
        cairo_destroy(gw->cc);
        return false;
    }

    return true;
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

    // Establish Pango/Cairo context
    if (!_GGDKDraw_InitPangoCairo(gw)) {
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

    // This should not be here. Debugging purposes only.
    gdk_window_show(nw->w);

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

    if (!_GGDKDraw_InitPangoCairo(gw)) {
        cairo_surface_destroy(gw->cs);
        free(gw->ggc);
        free(gw);
        return NULL;

    }
    return (GWindow)gw;
}

// Pango text
PangoFontDescription *_GGDKDraw_configfont(GWindow w, GFont *font) {
    GGDKWindow gw = (GGDKWindow) w;
    PangoFontDescription *fd;

    // Initialize Cairo and Pango if not initialized, e.g. root window
    if (gw->pango_layout == NULL && !_GGDKDraw_InitPangoCairo(gw)){
        return NULL;
    }

    PangoFontDescription **fdbase = &font->pangoc_fd;
    if (*fdbase != NULL) {
        return *fdbase;
    }
    *fdbase = fd = pango_font_description_new();
    if (*fdbase == NULL) {
        return NULL;
    }

    if (font->rq.utf8_family_name != NULL) {
        pango_font_description_set_family(fd, font->rq.utf8_family_name);
    } else {
        char *temp = u2utf8_copy(font->rq.family_name);
        pango_font_description_set_family(fd, temp);
        free(temp);
    }

    pango_font_description_set_style(fd, (font->rq.style & fs_italic)?
        PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
    pango_font_description_set_variant(fd, (font->rq.style & fs_smallcaps)?
        PANGO_VARIANT_SMALL_CAPS : PANGO_VARIANT_NORMAL);
    pango_font_description_set_weight(fd, font->rq.weight);
    pango_font_description_set_stretch(fd,
        (font->rq.style&fs_condensed)? PANGO_STRETCH_CONDENSED :
        (font->rq.style&fs_extended )? PANGO_STRETCH_EXPANDED  :
            PANGO_STRETCH_NORMAL);

    if (font->rq.style & fs_vertical) {
        //FIXME: not sure this is the right thing
        pango_font_description_set_gravity(fd, PANGO_GRAVITY_WEST);
    }

    if (font->rq.point_size <= 0) {
        // Any negative (pixel) values should be converted when font opened
        GDrawIError("Bad point size for Pango");
    }

    // Pango doesn't give me any control over the resolution on X, so I do my
    //  own conversion from points to pixels
    // But under pangocairo I can set the resolution, so behavior is different
    pango_font_description_set_absolute_size(fd,
        GDrawPointsToPixels(NULL, font->rq.point_size * PANGO_SCALE));
    return fd;
}

// Strangely the equivalent routine was not part of the pangocairo library
// Oh there's pango_cairo_layout_path but that's more restrictive and probably
// less efficient
static void _GGDKDraw_MyCairoRenderLayout(cairo_t *cc, Color fg, PangoLayout *layout,int x,int y) {
    PangoRectangle rect, r2;
    PangoLayoutIter *iter;

    iter = pango_layout_get_iter(layout);
    do {
        PangoLayoutRun *run = pango_layout_iter_get_run(iter);
        if (run != NULL) { // NULL runs mark end of line
            pango_layout_iter_get_run_extents(iter, &r2, &rect);
            cairo_move_to(cc, x + (rect.x + PANGO_SCALE/2)/PANGO_SCALE, y + (rect.y + PANGO_SCALE/2)/PANGO_SCALE);
            if (COLOR_ALPHA(fg) == 0) {
                cairo_set_source_rgba(cc, COLOR_RED(fg)/255.0,
                    COLOR_GREEN(fg)/255.0, COLOR_BLUE(fg)/255.0, 1.0);
            } else {
                cairo_set_source_rgba(cc, COLOR_RED(fg)/255.0,
                    COLOR_GREEN(fg)/255.0, COLOR_BLUE(fg)/255.0, COLOR_ALPHA(fg)/255.);
            }
            pango_cairo_show_glyph_string(cc, run->item->analysis.font, run->glyphs);
        }
    } while (pango_layout_iter_next_run(iter));
    pango_layout_iter_free(iter);
}

static int16 _GGDKDraw_GdkModifierToKsm(GdkModifierType mask) {
    int16 state = 0;
    //Translate from mask to X11 state
    if (mask & GDK_SHIFT_MASK)
        state |= ksm_shift;
    if (mask & GDK_LOCK_MASK)
        state |= ksm_capslock;
    if (mask & GDK_CONTROL_MASK)
        state |= ksm_control;
    if (mask & GDK_MOD1_MASK) //Guess
        state |= ksm_meta;
    if (mask & GDK_MOD2_MASK) //Guess
        state |= ksm_cmdmacosx;
    if (mask & GDK_SUPER_MASK)
        state |= ksm_super;
    if (mask * GDK_HYPER_MASK)
        state |= ksm_hyper;
    //ksm_option?
    if (mask & GDK_BUTTON1_MASK)
        state |= ksm_button1;
    if (mask & GDK_BUTTON2_MASK)
        state |= ksm_button2;
    if (mask & GDK_BUTTON3_MASK)
        state |= ksm_button3;
    if (mask & GDK_BUTTON4_MASK)
        state |= ksm_button4;
    if (mask & GDK_BUTTON5_MASK)
        state |= ksm_button5;

    return state;
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

static void GGDKDrawDestroyCursor(GDisplay *gdisp, GCursor gcursor){
    fprintf(stderr, "GDKCALL: GGDKDrawDestroyCursor\n"); assert(false);
}

// Some hack to see if the window has been created(???)
static int GGDKDrawNativeWindowExists(GDisplay *gdisp, void *native_window){
    fprintf(stderr, "GDKCALL: GGDKDrawNativeWindowExists\n"); //assert(false);
    return true;
}

static void GGDKDrawSetZoom(GWindow gw, GRect *size, enum gzoom_flags flags){
    fprintf(stderr, "GDKCALL: GGDKDrawSetZoom\n"); assert(false);
}

// Not possible?
static void GGDKDrawSetWindowBorder(GWindow gw, int width, Color gcol){
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowBorder\n"); //assert(false);
}

static void GGDKDrawSetWindowBackground(GWindow gw, Color gcol){
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowBackground\n"); //assert(false);
    GdkRGBA col = {
        .red = COLOR_RED(gcol)/255.,
        .green = COLOR_GREEN(gcol)/255.,
        .blue = COLOR_BLUE(gcol)/255.,
        .alpha = 1.
    };
    gdk_window_set_background_rgba(((GGDKWindow)gw)->w, &col);
}

// How about NO
static int GGDKDrawSetDither(GDisplay *gdisp, int set){
    fprintf(stderr, "GDKCALL: GGDKDrawSetDither\n"); //assert(false);
    return false;
}


static void GGDKDrawReparentWindow(GWindow child, GWindow newparent, int x, int y){
    fprintf(stderr, "GDKCALL: GGDKDrawReparentWindow\n");
    gdk_window_reparent(((GGDKWindow)child)->w, ((GGDKWindow)newparent)->w, x, y);
}

static void GGDKDrawSetVisible(GWindow gw, int show){
    fprintf(stderr, "GDKCALL: GGDKDrawSetVisible\n"); //assert(false);
    if (show) {
        gdk_window_show(((GGDKWindow)gw)->w);
    } else {
        gdk_window_hide(((GGDKWindow)gw)->w);
    }
}

static void GGDKDrawMove(GWindow gw, int32 x, int32 y){
    fprintf(stderr, "GDKCALL: GGDKDrawMove\n"); //assert(false);
    gdk_window_move(((GGDKWindow)gw)->w, x, y);
}

static void GGDKDrawTrueMove(GWindow w, int32 x, int32 y){
    fprintf(stderr, "GDKCALL: GGDKDrawTrueMove\n");
    GGDKDrawMove(w, x, y);
    //GGDKWindow gw = (GGDKWindow)w;

    //if (gw->is_toplevel && !gw->is_popup && !gw->istransient) {
    //    x -= gw->display->off_x;
    //    y -= gw->display->off_y;
    //}
}

static void GGDKDrawResize(GWindow gw, int32 w, int32 h){
    fprintf(stderr, "GDKCALL: GGDKDrawResize\n"); //assert(false);
    gdk_window_resize(((GGDKWindow)gw)->w, w, h);
}

static void GGDKDrawMoveResize(GWindow gw, int32 x, int32 y, int32 w, int32 h){
    fprintf(stderr, "GDKCALL: GGDKDrawMoveResize\n"); //assert(false);
    gdk_window_move_resize(((GGDKWindow)gw)->w, x, y, w, h);
}

static void GGDKDrawRaise(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawRaise\n"); //assert(false);
    gdk_window_raise(((GGDKWindow)gw)->w);
}

static void GGDKDrawRaiseAbove(GWindow gw1, GWindow gw2){
    fprintf(stderr, "GDKCALL: GGDKDrawRaiseAbove\n"); //assert(false);
    gdk_window_restack(((GGDKWindow)gw1)->w, ((GGDKWindow)gw2)->w, true);
}

// Only used once in gcontainer - force it to call GDrawRaiseAbove
static int GGDKDrawIsAbove(GWindow gw1, GWindow gw2){
    fprintf(stderr, "GDKCALL: GGDKDrawIsAbove\n"); //assert(false);
    return false;
}

static void GGDKDrawLower(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawLower\n"); //assert(false);
    gdk_window_lower(((GGDKWindow)gw)->w);
}

// Icon title is ignored.
static void GGDKDrawSetWindowTitles8(GWindow w, const char *title, const char *icontitle){
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowTitles8\n");// assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    free(gw->window_title);
    gw->window_title = copy(title);

    if (title != NULL && gw->is_toplevel) {
        gdk_window_set_title(gw->w, title);
    }
}

static void GGDKDrawSetWindowTitles(GWindow gw, const unichar_t *title, const unichar_t *icontitle){
    fprintf(stderr, "GDKCALL: GGDKDrawSetWindowTitles\n"); //assert(false);
    char *str = u2utf8_copy(title);
    if (str != NULL) {
        GGDKDrawSetWindowTitles8(gw, str, NULL);
        free(str);
    }
}

// Sigh. GDK doesn't provide a way to get the window title...
static unichar_t* GGDKDrawGetWindowTitle(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawGetWindowTitle\n"); // assert(false);
    return utf82u_copy(((GGDKWindow)gw)->window_title);
}

static char* GGDKDrawGetWindowTitle8(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawGetWindowTitle8\n"); //assert(false);
    return copy(((GGDKWindow)gw)->window_title);
}

static void GGDKDrawSetTransientFor(GWindow transient, GWindow owner){
    fprintf(stderr, "GDKCALL: GGDKDrawSetTransientFor\n"); //assert(false);
    /*GGDKWindow gw = (GGDKWindow) transient;
    GGDKDisplay *gdisp = gw->display;
    GdkWindow *ow;

    if (owner == (GWindow)-1) {
        ow = gdisp->last_nontransient_window;
    } else if (owner == NULL) {
        ow = NULL; // Does this work with GDK?
    } else {
        ow = ((GGDKWindow)owner)->w;
    }

    gdk_window_set_transient_for(gdisp->display, gw->w, ow);
    gw->transient_owner = ow;
    gw->istransient = (ow != NULL);*/
}

static void GGDKDrawGetPointerPosition(GWindow w, GEvent *ret){
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

static GWindow GGDKDrawGetPointerWindow(GWindow gw){
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

static void GGDKDrawSetCursor(GWindow gw, GCursor gcursor){
    fprintf(stderr, "GDKCALL: GGDKDrawSetCursor\n"); assert(false);
}

static GCursor GGDKDrawGetCursor(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawGetCursor\n"); assert(false);
}

static GWindow GGDKDrawGetRedirectWindow(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawGetRedirectWindow\n"); assert(false);
    // Sigh... I don't know...
    /*
    GGDKDisplay *gdisp = (GGDKDisplay *) gd;
    if (gdisp->input == NULL) {
        return NULL;
    }
    return gdisp->input->cur_dlg;
    */
}

static void GGDKDrawTranslateCoordinates(GWindow from, GWindow to, GPoint *pt){
    fprintf(stderr, "GDKCALL: GGDKDrawTranslateCoordinates\n"); assert(false);
    // Looks like i need to walk up the stack to find the parent and translate along the way...
    /*
    GXDisplay *gd = (GXDisplay *) ((_from!=NULL)?_from->display:_to->display);
    Window from = (_from==NULL)?gd->root:((GXWindow) _from)->w;
    Window to = (_to==NULL)?gd->root:((GXWindow) _to)->w;
    int x,y;
    Window child;

    XTranslateCoordinates(gd->display,from,to,pt->x,pt->y,&x,&y,&child);
    pt->x = x; pt->y = y;
    */
}


static void GGDKDrawBeep(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawBeep\n"); //assert(false);
    gdk_display_beep(((GGDKDisplay*)gdisp)->display);
}

static void GGDKDrawFlush(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawFlush\n"); //assert(false);
    gdk_display_flush(((GGDKDisplay*)gdisp)->display);
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
    fprintf(stderr, "GDKCALL: GGDKDrawGrabSelection\n"); //assert(false);
}

static void GGDKDrawAddSelectionType(GWindow w, enum selnames sel, char *type, void *data, int32 cnt, int32 unitsize, void *gendata(void *, int32 *len), void freedata(void *)){
    fprintf(stderr, "GDKCALL: GGDKDrawAddSelectionType\n"); //assert(false);
}

static void* GGDKDrawRequestSelection(GWindow w, enum selnames sn, char *typename, int32 *len){
    fprintf(stderr, "GDKCALL: GGDKDrawRequestSelection\n"); //assert(false);
}

static int GGDKDrawSelectionHasType(GWindow w, enum selnames sn, char *typename){
    fprintf(stderr, "GDKCALL: GGDKDrawSelectionHasType\n"); //assert(false);
}

static void GGDKDrawBindSelection(GDisplay *gdisp, enum selnames sn, char *atomname){
    fprintf(stderr, "GDKCALL: GGDKDrawBindSelection\n"); //assert(false); //TODO: Implement selections (clipboard)
}

static int GGDKDrawSelectionHasOwner(GDisplay *gdisp, enum selnames sn){
    fprintf(stderr, "GDKCALL: GGDKDrawSelectionHasOwner\n"); assert(false);
}


static void GGDKDrawPointerUngrab(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawPointerUngrab\n"); //assert(false);
    //Supposedly deprecated but I don't care
    //gdk_display_pointer_ungrab(((GGDKDisplay*)gdisp)->display, GDK_CURRENT_TIME);
}

static void GGDKDrawPointerGrab(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawPointerGrab\n"); //assert(false);
    //Supposedly deprecated but I don't care
    //WTF? don't exist? sigh. Have to use the seat/device/...
    //gdk_display_pointer_grab(((GGDKDisplay*)gdisp)->display, GDK_CURRENT_TIME);
}

static void GGDKDrawRequestExpose(GWindow gw, GRect *gr, int set){
    fprintf(stderr, "GDKCALL: GGDKDrawRequestExpose\n"); assert(false);
}

static void GGDKDrawForceUpdate(GWindow gw){
    fprintf(stderr, "GDKCALL: GGDKDrawForceUpdate\n"); assert(false);
}

static void GGDKDrawSync(GDisplay *gdisp){
    fprintf(stderr, "GDKCALL: GGDKDrawSync\n");
    gdk_display_sync(((GGDKDisplay*)gdisp)->display);
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
    fprintf(stderr, "GDKCALL: GGDKDrawRequestTimer\n"); //assert(false);
    return NULL;
}

static void GGDKDrawCancelTimer(GTimer *timer){
    fprintf(stderr, "GDKCALL: GGDKDrawCancelTimer\n"); //assert(false);
}


static void GGDKDrawSyncThread(GDisplay *gdisp, void (*func)(void *), void *data){
    fprintf(stderr, "GDKCALL: GGDKDrawSyncThread\n"); //assert(false); // For some shitty gio impl. Ignore ignore ignore!
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


static void GGDKDrawGetFontMetrics(GWindow gw, GFont *fi, int *as, int *ds, int *ld){
    fprintf(stderr, "GDKCALL: GGDKDrawGetFontMetrics\n");

    GGDKDisplay *gdisp = ((GGDKWindow) gw)->display;
    PangoFont *pfont;
    PangoFontMetrics *fm;
    PangoFontMap *pfm;

    _GGDKDraw_configfont(gw, fi);
    pfm = pango_context_get_font_map(gdisp->pangoc_context);
    pfont = pango_font_map_load_font(pfm, gdisp->pangoc_context, fi->pangoc_fd);
    fm = pango_font_get_metrics(pfont, NULL);
    *as = pango_font_metrics_get_ascent(fm)/PANGO_SCALE;
    *ds = pango_font_metrics_get_descent(fm)/PANGO_SCALE;
    *ld = 0;
    pango_font_metrics_unref(fm);
    //g_object_unref(pfont);
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
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutInit\n");
    GGDKWindow gw = (GGDKWindow) w;
    PangoFontDescription *fd;

    if (fi == NULL) {
        fi = gw->ggc->fi;
    }

    fd = _GGDKDraw_configfont(w, fi);
    pango_layout_set_font_description(gw->pango_layout, fd);
    pango_layout_set_text(gw->pango_layout, text, cnt);
}

static void GGDKDrawLayoutDraw(GWindow w, int32 x, int32 y, Color fg){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutDraw\n");
    GGDKWindow gw = (GGDKWindow)w;

    _GGDKDraw_MyCairoRenderLayout(gw->cc, fg, gw->pango_layout, x, y);
}

static void GGDKDrawLayoutIndexToPos(GWindow w, int index, GRect *pos){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutIndexToPos\n");
    GGDKWindow gw = (GGDKWindow)w;
    PangoRectangle rect;

    pango_layout_index_to_pos(gw->pango_layout, index, &rect);
    pos->x = rect.x/PANGO_SCALE;
    pos->y = rect.y/PANGO_SCALE;
    pos->width  = rect.width/PANGO_SCALE;
    pos->height = rect.height/PANGO_SCALE;
}

static int GGDKDrawLayoutXYToIndex(GWindow w, int x, int y){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutXYToIndex\n");
    GGDKWindow gw = (GGDKWindow)w;
    int trailing, index;

    // Pango retuns the last character if x is negative, not the first.
    if (x < 0) {
        x = 0;
    }
    pango_layout_xy_to_index(gw->pango_layout, x * PANGO_SCALE, y * PANGO_SCALE, &index, &trailing);

    // If I give Pango a position after the last character on a line, it
    // returns to me the first character. Strange. And annoying -- you click
    // at the end of a line and the cursor moves to the start
    // Of course in right to left text an initial position is correct...
    if ((index + trailing) == 0 && x > 0 ) {
        PangoRectangle rect;
        pango_layout_get_pixel_extents(gw->pango_layout, &rect, NULL);
        if (x >= rect.width) {
            x = rect.width - 1;
            pango_layout_xy_to_index(gw->pango_layout, x * PANGO_SCALE, y * PANGO_SCALE, &index, &trailing);
        }
    }
    return index + trailing;
}

static void GGDKDrawLayoutExtents(GWindow w, GRect *size){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutExtents\n");
    GGDKWindow gw = (GGDKWindow)w;
    PangoRectangle rect;

    pango_layout_get_pixel_extents(gw->pango_layout,NULL,&rect);
    size->x = rect.x;
    size->y = rect.y;
    size->width  = rect.width;
    size->height = rect.height;
}

static void GGDKDrawLayoutSetWidth(GWindow w, int width){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutSetWidth\n");
    GGDKWindow gw = (GGDKWindow)w;

    pango_layout_set_width(gw->pango_layout, (width == -1) ? -1 : width * PANGO_SCALE);
}

static int GGDKDrawLayoutLineCount(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutLineCount\n");
    GGDKWindow gw = (GGDKWindow)w;

    return pango_layout_get_line_count(gw->pango_layout);
}

static int GGDKDrawLayoutLineStart(GWindow w, int l){
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutLineStart\n");
    GGDKWindow gw = (GGDKWindow)w;
    PangoLayoutLine *line = pango_layout_get_line(gw->pango_layout, l);

    if (line == NULL) {
        return -1;
    }

    return line->start_index;
}

static void GGDKDrawStartNewSubPath(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawStartNewSubPath\n"); assert(false);
}

static int GGDKDrawFillRuleSetWinding(GWindow w){
    fprintf(stderr, "GDKCALL: GGDKDrawFillRuleSetWinding\n"); assert(false);
}

static int GGDKDrawDoText8(GWindow w, int32 x, int32 y, const char *text, int32 cnt, Color col, enum text_funcs drawit, struct tf_arg *arg) {
    fprintf(stderr, "GDKCALL: GGDKDrawDoText8\n");

    GGDKWindow gw = (GGDKWindow) w;
    GGDKDisplay *gdisp = gw->display;
    struct font_instance *fi = gw->ggc->fi;
    PangoRectangle rect, ink;
    PangoFontDescription *fd;

    if (fi == NULL) {
        return 0;
    }

    fd = _GGDKDraw_configfont(w, fi);
    if (fd == NULL) {
        return 0;
    }

    pango_layout_set_font_description(gw->pango_layout, fd);
    pango_layout_set_text(gw->pango_layout, (char *)text, cnt);
    pango_layout_get_pixel_extents(gw->pango_layout, NULL, &rect);

    if (drawit == tf_drawit) {
        _GGDKDraw_MyCairoRenderLayout(gw->cc, col, gw->pango_layout, x, y);
    } else if (drawit == tf_rect) {
        PangoLayoutIter *iter;
        PangoLayoutRun *run;
        PangoFontMetrics *fm;

        pango_layout_get_pixel_extents(gw->pango_layout, &ink, &rect);
        arg->size.lbearing = ink.x - rect.x;
        arg->size.rbearing = ink.x+ink.width - rect.x;
        arg->size.width = rect.width;
        if ( *text=='\0' ) {
            // There are no runs if there are no characters
            memset(&arg->size,0,sizeof(arg->size));
        } else {
            iter = pango_layout_get_iter(gw->pango_layout);
            run = pango_layout_iter_get_run(iter);
            if (run == NULL) {
                // Pango doesn't give us runs in a couple of other places
                // surrogates, not unicode (0xfffe, 0xffff), etc.
                memset(&arg->size, 0, sizeof(arg->size));
            } else {
                fm = pango_font_get_metrics(run->item->analysis.font, NULL);
                arg->size.fas = pango_font_metrics_get_ascent(fm)/PANGO_SCALE;
                arg->size.fds = pango_font_metrics_get_descent(fm)/PANGO_SCALE;
                arg->size.as = ink.y + ink.height - arg->size.fds;
                arg->size.ds = arg->size.fds - ink.y;
                if (arg->size.ds < 0) {
                    --arg->size.as;
                    arg->size.ds = 0;
                }
                // In the one case I've looked at fds is one pixel off from rect.y
                //  I don't know what to make of that
                pango_font_metrics_unref(fm);
            }
            pango_layout_iter_free(iter);
        }
    }
    return rect.width;
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
    g_object_set_data(G_OBJECT(gdisp->root), "GGDKWindow", groot);

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
