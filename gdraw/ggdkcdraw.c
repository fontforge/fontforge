/**
 *  \file ggdkcdraw.c
 *  \brief Cairo drawing functionality
 */

#include "ggdkdrawP.h"
#include "ustring.h"
#include <assert.h>
#include <string.h>

// Private member functions

static void GXCDraw_StippleMePink(GXWindow gw, int ts, Color fg) {
    static unsigned char grey_init[8] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
    static unsigned char fence_init[8] = {0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55, 0x88};
    uint8 *spt;
    int bit, i, j;
    uint32 *data;
    static uint32 space[8 * 8];
    static cairo_surface_t *is = NULL;
    static cairo_pattern_t *pat = NULL;

    if ((fg >> 24) != 0xff) {
        int alpha = fg >> 24, r = COLOR_RED(fg), g = COLOR_GREEN(fg), b = COLOR_BLUE(fg);
        r = (alpha * r + 128) / 255;
        g = (alpha * g + 128) / 255;
        b = (alpha * b + 128) / 255;
        fg = (alpha << 24) | (r << 16) | (g << 8) | b;
    }

    spt = ts == 2 ? fence_init : grey_init;
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
    if (is == NULL) {
        is = cairo_image_surface_create_for_data((uint8 *) space, CAIRO_FORMAT_ARGB32,
                8, 8, 8 * 4);
        pat = cairo_pattern_create_for_surface(is);
        cairo_pattern_set_extend(pat, CAIRO_EXTEND_REPEAT);
    }
    cairo_set_source(gw->cc, pat);
}

static int GXCDrawSetcolfunc(GXWindow gw, GGC *mine) {
    /*GCState *gcs = &gw->cairo_state;*/
    Color fg = mine->fg;

    if ((fg >> 24) == 0) {
        fg |= 0xff000000;
    }

    if (mine->ts != 0) {
        GXCDraw_StippleMePink(gw, mine->ts, fg);
    } else {
        cairo_set_source_rgba(gw->cc, COLOR_RED(fg) / 255.0, COLOR_GREEN(fg) / 255.0, COLOR_BLUE(fg) / 255.0,
                              (fg >> 24) / 255.);
    }
    return true;
}

static int GXCDrawSetline(GXWindow gw, GGC *mine) {
    GCState *gcs = &gw->cairo_state;
    Color fg = mine->fg;

    if ((fg >> 24) == 0) {
        fg |= 0xff000000;
    }

    if (mine->line_width <= 0) {
        mine->line_width = 1;
    }
    if (mine->line_width != gcs->line_width || mine->line_width != 2) {
        cairo_set_line_width(gw->cc, mine->line_width);
        gcs->line_width = mine->line_width;
    }
    if (mine->dash_len != gcs->dash_len || mine->skip_len != gcs->skip_len ||
            mine->dash_offset != gcs->dash_offset) {
        double dashes[2];
        dashes[0] = mine->dash_len;
        dashes[1] = mine->skip_len;
        cairo_set_dash(gw->cc, dashes, 0, mine->dash_offset);
        gcs->dash_offset = mine->dash_offset;
        gcs->dash_len = mine->dash_len;
        gcs->skip_len = mine->skip_len;
    }
    /* I don't use line join/cap. On a screen with small line_width they are irrelevant */

    if (mine->ts != 0) {
        GXCDraw_StippleMePink(gw, mine->ts, fg);
    } else {
        cairo_set_source_rgba(gw->cc, COLOR_RED(fg) / 255.0, COLOR_GREEN(fg) / 255.0, COLOR_BLUE(fg) / 255.0,
                              (fg >> 24) / 255.0);
    }
    return mine->line_width;
}

// Protected member functions

bool _GGDKDraw_InitPangoCairo(GGDKWindow gw) {
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

// Pango text
PangoFontDescription *_GGDKDraw_configfont(GWindow w, GFont *font) {
    GGDKWindow gw = (GGDKWindow) w;
    PangoFontDescription *fd;

    // Initialize Cairo and Pango if not initialized, e.g. root window
    if (gw->pango_layout == NULL && !_GGDKDraw_InitPangoCairo(gw)) {
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

    pango_font_description_set_style(fd, (font->rq.style & fs_italic) ?
                                     PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
    pango_font_description_set_variant(fd, (font->rq.style & fs_smallcaps) ?
                                       PANGO_VARIANT_SMALL_CAPS : PANGO_VARIANT_NORMAL);
    pango_font_description_set_weight(fd, font->rq.weight);
    pango_font_description_set_stretch(fd,
                                       (font->rq.style & fs_condensed) ? PANGO_STRETCH_CONDENSED :
                                       (font->rq.style & fs_extended) ? PANGO_STRETCH_EXPANDED  :
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
void _GGDKDraw_MyCairoRenderLayout(cairo_t *cc, Color fg, PangoLayout *layout, int x, int y) {
    PangoRectangle rect, r2;
    PangoLayoutIter *iter;

    iter = pango_layout_get_iter(layout);
    do {
        PangoLayoutRun *run = pango_layout_iter_get_run(iter);
        if (run != NULL) { // NULL runs mark end of line
            pango_layout_iter_get_run_extents(iter, &r2, &rect);
            cairo_move_to(cc, x + (rect.x + PANGO_SCALE / 2) / PANGO_SCALE, y + (rect.y + PANGO_SCALE / 2) / PANGO_SCALE);
            if (COLOR_ALPHA(fg) == 0) {
                cairo_set_source_rgba(cc, COLOR_RED(fg) / 255.0,
                                      COLOR_GREEN(fg) / 255.0, COLOR_BLUE(fg) / 255.0, 1.0);
            } else {
                cairo_set_source_rgba(cc, COLOR_RED(fg) / 255.0,
                                      COLOR_GREEN(fg) / 255.0, COLOR_BLUE(fg) / 255.0, COLOR_ALPHA(fg) / 255.);
            }
            pango_cairo_show_glyph_string(cc, run->item->analysis.font, run->glyphs);
        }
    } while (pango_layout_iter_next_run(iter));
    pango_layout_iter_free(iter);
}


void _GGDKDraw_PushClip(GGDKWindow gw) {
    cairo_save(gw->cc);
    cairo_new_path(gw->cc);
    cairo_rectangle(gw->cc, gw->ggc->clip.x, gw->ggc->clip.y,
                    gw->ggc->clip.width, gw->ggc->clip.height);
    cairo_clip(gw->cc);
}

void _GGDKDraw_PopClip(GGDKWindow gw) {
    cairo_restore(gw->cc);
}


void GGDKDrawPushClip(GWindow w, GRect *rct, GRect *old) {
    fprintf(stderr, "GDKCALL: GGDKDrawPushClip\n"); //assert(false);

    // Return the current clip, and intersect the current clip with the desired
    // clip to get the new clip.

    *old = w->ggc->clip;
    w->ggc->clip = *rct;
    if (w->ggc->clip.x + w->ggc->clip.width > old->x + old->width) {
        w->ggc->clip.width = old->x + old->width - w->ggc->clip.x;
    }
    if (w->ggc->clip.y + w->ggc->clip.height > old->y + old->height) {
        w->ggc->clip.height = old->y + old->height - w->ggc->clip.y;
    }
    if (w->ggc->clip.x < old->x) {
        if (w->ggc->clip.width > (old->x - w->ggc->clip.x)) {
            w->ggc->clip.width -= (old->x - w->ggc->clip.x);
        } else {
            w->ggc->clip.width = 0;
        }
        w->ggc->clip.x = old->x;
    }
    if (w->ggc->clip.y < old->y) {
        if (w->ggc->clip.height > (old->y - w->ggc->clip.y)) {
            w->ggc->clip.height -= (old->y - w->ggc->clip.y);
        } else {
            w->ggc->clip.height = 0;
        }
        w->ggc->clip.y = old->y;
    }
    if (w->ggc->clip.height < 0 || w->ggc->clip.width < 0) {
        // Negative values mean large positive values, so if we want to clip
        //  to nothing force clip outside window
        w->ggc->clip.x = w->ggc->clip.y = -100;
        w->ggc->clip.height = w->ggc->clip.width = 1;
    }
    _GGDKDraw_PushClip((GGDKWindow)w);
}

void GGDKDrawPopClip(GWindow gw, GRect *old) {
    fprintf(stderr, "GDKCALL: GGDKDrawPopClip\n"); //assert(false);
    gw->ggc->clip = *old;
    _GGDKDraw_PopClip((GGDKWindow)gw);
}


void GGDKDrawSetDifferenceMode(GWindow w) {
    fprintf(stderr, "GDKCALL: GGDKDrawSetDifferenceMode\n"); //assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    cairo_set_operator(gw->cc, CAIRO_OPERATOR_DIFFERENCE);
    cairo_set_antialias(gw->cc, CAIRO_ANTIALIAS_NONE);
}


void GGDKDrawClear(GWindow w, GRect *rect) {
    fprintf(stderr, "GDKCALL: GGDKDrawClear\n"); //assert(false);
    GGDKWindow gw = (GGDKWindow)w;
    GRect temp, *r = rect;
    if (r == NULL) {
        temp = gw->pos;
        temp.x = temp.y = 0;
        r = &temp;
    }
    cairo_new_path(gw->cc);
    cairo_rectangle(gw->cc, r->x, r->y, r->width, r->height);
    cairo_set_source_rgba(gw->cc, GColorToGDK(gw->ggc->bg), 1.0);
    cairo_fill(gw->cc);
}

void GGDKDrawDrawLine(GWindow w, int32 x, int32 y, int32 xend, int32 yend, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawDrawLine\n"); //assert(false);
    GGDKWindow gw = (GGDKWindow)w;

    w->ggc->fg = col;

    int width = GXCDrawSetline(gw, gw->ggc);
    cairo_new_path(gw->cc);
    if (width & 1) {
        cairo_move_to(gw->cc, x + .5, y + .5);
        cairo_line_to(gw->cc, xend + .5, yend + .5);
    } else {
        cairo_move_to(gw->cc, x, y);
        cairo_line_to(gw->cc, xend, yend);
    }
    cairo_stroke(gw->cc);
}

void GGDKDrawDrawArrow(GWindow gw, int32 x, int32 y, int32 xend, int32 yend, int16 arrows, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawDrawArrow\n");
    assert(false);
}

void GGDKDrawDrawRect(GWindow gw, GRect *rect, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawDrawRect\n");
    assert(false);
}

void GGDKDrawFillRect(GWindow gw, GRect *rect, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawFillRect\n");
    assert(false);
}

void GGDKDrawFillRoundRect(GWindow gw, GRect *rect, int radius, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawFillRoundRect\n");
    assert(false);
}

void GGDKDrawDrawEllipse(GWindow gw, GRect *rect, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawDrawEllipse\n");
    assert(false);
}

void GGDKDrawFillEllipse(GWindow gw, GRect *rect, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawFillEllipse\n");
    assert(false);
}

void GGDKDrawDrawArc(GWindow gw, GRect *rect, int32 sangle, int32 eangle, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawDrawArc\n");
    assert(false);
}

void GGDKDrawDrawPoly(GWindow gw, GPoint *pts, int16 cnt, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawDrawPoly\n");
    assert(false);
}

void GGDKDrawFillPoly(GWindow gw, GPoint *pts, int16 cnt, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawFillPoly\n");
    assert(false);
}

void GGDKDrawScroll(GWindow gw, GRect *rect, int32 hor, int32 vert) {
    fprintf(stderr, "GDKCALL: GGDKDrawScroll\n");
    assert(false);
}


void GGDKDrawDrawImage(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y) {
    fprintf(stderr, "GDKCALL: GGDKDrawDrawImage\n");
    assert(false);
}

void GGDKDrawTileImage(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y) {
    fprintf(stderr, "GDKCALL: GGDKDrawTileImage\n");
    assert(false);
}

void GGDKDrawDrawGlyph(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y) {
    fprintf(stderr, "GDKCALL: GGDKDrawDrawGlyph\n");
    assert(false);
}

void GGDKDrawDrawImageMagnified(GWindow gw, GImage *gimg, GRect *src, int32 x, int32 y, int32 width, int32 height) {
    fprintf(stderr, "GDKCALL: GGDKDrawDrawImageMag\n");
    assert(false);
}

GImage *GGDKDrawCopyScreenToImage(GWindow gw, GRect *rect) {
    fprintf(stderr, "GDKCALL: GGDKDrawCopyScreenToImage\n");
    assert(false);
}

void GGDKDrawDrawPixmap(GWindow gw1, GWindow gw2, GRect *src, int32 x, int32 y) {
    fprintf(stderr, "GDKCALL: GGDKDrawDrawPixmap\n");
    assert(false);
}

void GGDKDrawTilePixmap(GWindow gw1, GWindow gw2, GRect *src, int32 x, int32 y) {
    fprintf(stderr, "GDKCALL: GGDKDrawTilePixmap\n");
    assert(false);
}

enum gcairo_flags GGDKDrawHasCairo(GWindow w) {
    fprintf(stderr, "GDKCALL: gcairo_flags GGDKDrawHasCairo\n");
    assert(false);
}


void GGDKDrawPathStartNew(GWindow w) {
    fprintf(stderr, "GDKCALL: GGDKDrawStartNewPath\n");
    assert(false);
}

void GGDKDrawPathClose(GWindow w) {
    fprintf(stderr, "GDKCALL: GGDKDrawClosePath\n");
    assert(false);
}

void GGDKDrawPathMoveTo(GWindow w, double x, double y) {
    fprintf(stderr, "GDKCALL: GGDKDrawMoveto\n");
    assert(false);
}

void GGDKDrawPathLineTo(GWindow w, double x, double y) {
    fprintf(stderr, "GDKCALL: GGDKDrawLineto\n");
    assert(false);
}

void GGDKDrawPathCurveTo(GWindow w, double cx1, double cy1, double cx2, double cy2, double x, double y) {
    fprintf(stderr, "GDKCALL: GGDKDrawCurveto\n");
    assert(false);
}

void GGDKDrawPathStroke(GWindow w, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawStroke\n");
    assert(false);
}

void GGDKDrawPathFill(GWindow w, Color col) {
    fprintf(stderr, "GDKCALL: GGDKDrawFill\n");
    assert(false);
}

void GGDKDrawPathFillAndStroke(GWindow w, Color fillcol, Color strokecol) {
    fprintf(stderr, "GDKCALL: GGDKDrawFillAndStroke\n");
    assert(false);
}

void GGDKDrawStartNewSubPath(GWindow w) {
    fprintf(stderr, "GDKCALL: GGDKDrawStartNewSubPath\n");
    assert(false);
}

int GGDKDrawFillRuleSetWinding(GWindow w) {
    fprintf(stderr, "GDKCALL: GGDKDrawFillRuleSetWinding\n");
    assert(false);
}

int GGDKDrawDoText8(GWindow w, int32 x, int32 y, const char *text, int32 cnt, Color col, enum text_funcs drawit,
                    struct tf_arg *arg) {
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
        arg->size.rbearing = ink.x + ink.width - rect.x;
        arg->size.width = rect.width;
        if (*text == '\0') {
            // There are no runs if there are no characters
            memset(&arg->size, 0, sizeof(arg->size));
        } else {
            iter = pango_layout_get_iter(gw->pango_layout);
            run = pango_layout_iter_get_run(iter);
            if (run == NULL) {
                // Pango doesn't give us runs in a couple of other places
                // surrogates, not unicode (0xfffe, 0xffff), etc.
                memset(&arg->size, 0, sizeof(arg->size));
            } else {
                fm = pango_font_get_metrics(run->item->analysis.font, NULL);
                arg->size.fas = pango_font_metrics_get_ascent(fm) / PANGO_SCALE;
                arg->size.fds = pango_font_metrics_get_descent(fm) / PANGO_SCALE;
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

void GGDKDrawPushClipOnly(GWindow w) {
    fprintf(stderr, "GDKCALL: GGDKDrawPushClipOnly\n");
    assert(false);
}

void GGDKDrawClipPreserve(GWindow w) {
    fprintf(stderr, "GDKCALL: GGDKDrawClipPreserve\n");
    assert(false);
}




// PANGO LAYOUT

void GGDKDrawGetFontMetrics(GWindow gw, GFont *fi, int *as, int *ds, int *ld) {
    fprintf(stderr, "GDKCALL: GGDKDrawGetFontMetrics\n");

    GGDKDisplay *gdisp = ((GGDKWindow) gw)->display;
    PangoFont *pfont;
    PangoFontMetrics *fm;
    PangoFontMap *pfm;

    _GGDKDraw_configfont(gw, fi);
    pfm = pango_context_get_font_map(gdisp->pangoc_context);
    pfont = pango_font_map_load_font(pfm, gdisp->pangoc_context, fi->pangoc_fd);
    fm = pango_font_get_metrics(pfont, NULL);
    *as = pango_font_metrics_get_ascent(fm) / PANGO_SCALE;
    *ds = pango_font_metrics_get_descent(fm) / PANGO_SCALE;
    *ld = 0;
    pango_font_metrics_unref(fm);
    //g_object_unref(pfont);
}

void GGDKDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi) {
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

void GGDKDrawLayoutDraw(GWindow w, int32 x, int32 y, Color fg) {
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutDraw\n");
    GGDKWindow gw = (GGDKWindow)w;

    _GGDKDraw_MyCairoRenderLayout(gw->cc, fg, gw->pango_layout, x, y);
}

void GGDKDrawLayoutIndexToPos(GWindow w, int index, GRect *pos) {
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutIndexToPos\n");
    GGDKWindow gw = (GGDKWindow)w;
    PangoRectangle rect;

    pango_layout_index_to_pos(gw->pango_layout, index, &rect);
    pos->x = rect.x / PANGO_SCALE;
    pos->y = rect.y / PANGO_SCALE;
    pos->width  = rect.width / PANGO_SCALE;
    pos->height = rect.height / PANGO_SCALE;
}

int GGDKDrawLayoutXYToIndex(GWindow w, int x, int y) {
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
    if ((index + trailing) == 0 && x > 0) {
        PangoRectangle rect;
        pango_layout_get_pixel_extents(gw->pango_layout, &rect, NULL);
        if (x >= rect.width) {
            x = rect.width - 1;
            pango_layout_xy_to_index(gw->pango_layout, x * PANGO_SCALE, y * PANGO_SCALE, &index, &trailing);
        }
    }
    return index + trailing;
}

void GGDKDrawLayoutExtents(GWindow w, GRect *size) {
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutExtents\n");
    GGDKWindow gw = (GGDKWindow)w;
    PangoRectangle rect;

    pango_layout_get_pixel_extents(gw->pango_layout, NULL, &rect);
    size->x = rect.x;
    size->y = rect.y;
    size->width  = rect.width;
    size->height = rect.height;
}

void GGDKDrawLayoutSetWidth(GWindow w, int width) {
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutSetWidth\n");
    GGDKWindow gw = (GGDKWindow)w;

    pango_layout_set_width(gw->pango_layout, (width == -1) ? -1 : width * PANGO_SCALE);
}

int GGDKDrawLayoutLineCount(GWindow w) {
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutLineCount\n");
    GGDKWindow gw = (GGDKWindow)w;

    return pango_layout_get_line_count(gw->pango_layout);
}

int GGDKDrawLayoutLineStart(GWindow w, int l) {
    fprintf(stderr, "GDKCALL: GGDKDrawLayoutLineStart\n");
    GGDKWindow gw = (GGDKWindow)w;
    PangoLayoutLine *line = pango_layout_get_line(gw->pango_layout, l);

    if (line == NULL) {
        return -1;
    }

    return line->start_index;
}


