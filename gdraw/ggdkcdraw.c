/* Copyright (C) 2000-2012 by George Williams */
/* Copyright (C) 2016 by Jeremy Tan */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

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

/**
 *  \file ggdkcdraw.c
 *  \brief Cairo drawing functionality
 *  This is based on gxcdraw.c, but modified to suit GDK.
 */

#include "ggdkdrawP.h"

#ifdef FONTFORGE_CAN_USE_GDK

#include "ustring.h"

#include <assert.h>
#include <math.h>

// Private member functions

static void _GGDKDraw_CheckAutoPaint(GGDKWindow gw) {
    if (gw->cc == NULL) {
        assert(!gw->is_pixmap);

        //Log(LOGWARN, "Dirty dirty window! 0x%p", gw);
        if (gw->display->dirty_window != gw) {
            _GGDKDraw_CleanupAutoPaint(gw->display);
            gw->display->dirty_window = gw;
        }

        // Shut up! I know what I'm doing.
G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        gw->cc = gdk_cairo_create(gw->w);
G_GNUC_END_IGNORE_DEPRECATIONS

        // Unlike GDK2, it turns out you can draw over child windows
        // But we don't want that. Must be something to do with alpha transparency.
        if (!gdk_window_has_native(gw->w)) {
            cairo_region_t *r = _GGDKDraw_ExcludeChildRegions(gw, NULL, false);
            if (r != NULL) {
                gdk_cairo_region(gw->cc, r);
                cairo_clip(gw->cc);
                cairo_region_destroy(r);
            }
        }
    }
}

static void GGDKDraw_StippleMePink(GGDKWindow gw, int ts, Color fg) {
    static unsigned char grey_init[8] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
    static unsigned char fence_init[8] = {0x55, 0x22, 0x55, 0x88, 0x55, 0x22, 0x55, 0x88};
    uint8 *spt;
    int bit, i, j;
    uint32 *data;
    static uint32 space[8 * 8];
    static cairo_pattern_t *pat = NULL;

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
    if (pat == NULL) {
        cairo_surface_t *is = cairo_image_surface_create_for_data((uint8 *) space,
                              CAIRO_FORMAT_ARGB32, 8, 8, 8 * 4);
        pat = cairo_pattern_create_for_surface(is);
        cairo_surface_destroy(is);
        cairo_pattern_set_extend(pat, CAIRO_EXTEND_REPEAT);
    }
    cairo_set_source(gw->cc, pat);
}

static int GGDKDrawSetcolfunc(GGDKWindow gw, GGC *mine) {
    Color fg = mine->fg;

    if ((fg >> 24) == 0) {
        fg |= 0xff000000;
    }

    if (mine->ts != 0) {
        GGDKDraw_StippleMePink(gw, mine->ts, fg);
    } else {
        cairo_set_source_rgba(gw->cc, COLOR_RED(fg) / 255.0,
                              COLOR_GREEN(fg) / 255.0, COLOR_BLUE(fg) / 255.0, (fg >> 24) / 255.);
    }
    return true;
}

static int GGDKDrawSetline(GGDKWindow gw, GGC *mine) {
    Color fg = mine->fg;
    double dashes[2] = {mine->dash_len, mine->skip_len};

    if ((fg >> 24) == 0) {
        fg |= 0xff000000;
    }

    if (mine->line_width <= 0) {
        mine->line_width = 1;
    }
    cairo_set_line_width(gw->cc, mine->line_width);
    cairo_set_dash(gw->cc, dashes, (mine->dash_len == 0) ? 0 : 2, mine->dash_offset);

    // I don't use line join/cap. On a screen with small line_width they are irrelevant

    if (mine->ts != 0) {
        GGDKDraw_StippleMePink(gw, mine->ts, fg);
    } else {
        cairo_set_source_rgba(gw->cc, COLOR_RED(fg) / 255.0, COLOR_GREEN(fg) / 255.0, COLOR_BLUE(fg) / 255.0,
                              (fg >> 24) / 255.0);
    }
    return mine->line_width;
}

// Pango text
static PangoFontDescription *_GGDKDraw_configfont(GWindow w, GFont *font) {
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
            GDrawPointsToPixels(w, font->rq.point_size * PANGO_SCALE));
    return fd;
}

// Strangely the equivalent routine was not part of the pangocairo library
// Oh there's pango_cairo_layout_path but that's more restrictive and probably
// less efficient
static void _GGDKDraw_MyCairoRenderLayout(cairo_t *cc, Color fg, PangoLayout *layout, int x, int y) {
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

static void _GGDKDraw_EllipsePath(cairo_t *cc, double cx, double cy, double width, double height) {
    cairo_new_path(cc);
    cairo_move_to(cc, cx, cy + height);
    cairo_curve_to(cc,
                   cx + .552 * width, cy + height,
                   cx + width, cy + .552 * height,
                   cx + width, cy);
    cairo_curve_to(cc,
                   cx + width, cy - .552 * height,
                   cx + .552 * width, cy - height,
                   cx, cy - height);
    cairo_curve_to(cc,
                   cx - .552 * width, cy - height,
                   cx - width, cy - .552 * height,
                   cx - width, cy);
    cairo_curve_to(cc,
                   cx - width, cy + .552 * height,
                   cx - .552 * width, cy + height,
                   cx, cy + height);
    cairo_close_path(cc);
}

static cairo_surface_t *_GGDKDraw_GImage2Surface(GImage *image, GRect *src) {
    struct _GImage *base = (image->list_len == 0) ? image->u.image : image->u.images[0];
    cairo_format_t type;
    uint8 *pt;
    uint32 *idata, *ipt, *ito;
    int i, j, jj, tjj, stride;
    int bit, tobit;
    cairo_surface_t *cs;

    if (base->image_type == it_rgba) {
        type = CAIRO_FORMAT_ARGB32;
    } else if (base->image_type == it_true && base->trans != COLOR_UNKNOWN) {
        type = CAIRO_FORMAT_ARGB32;
    } else if (base->image_type == it_index && base->clut->trans_index != COLOR_UNKNOWN) {
        type = CAIRO_FORMAT_ARGB32;
    } else if (base->image_type == it_true) {
        type = CAIRO_FORMAT_RGB24;
    } else if (base->image_type == it_index) {
        type = CAIRO_FORMAT_RGB24;
    } else if (base->image_type == it_mono && base->clut != NULL &&
               base->clut->trans_index != COLOR_UNKNOWN) {
        type = CAIRO_FORMAT_A1;
    } else {
        type = CAIRO_FORMAT_RGB24;
    }

    /* We can't reuse the image's data for alpha images because we must */
    /*  premultiply each channel by alpha. We can reuse it for non-transparent*/
    /*  rgb images */
    if (base->image_type == it_true && type == CAIRO_FORMAT_RGB24) {
        idata = ((uint32 *)(base->data + src->y * base->bytes_per_line)) + src->x;
        return cairo_image_surface_create_for_data((uint8 *) idata, type,
                src->width, src->height,
                base->bytes_per_line);
    }

    cs = cairo_image_surface_create(type, src->width, src->height);
    stride = cairo_image_surface_get_stride(cs);
    cairo_surface_flush(cs);
    idata = (uint32 *)cairo_image_surface_get_data(cs);

    if (base->image_type == it_rgba) {
        ipt = ((uint32 *)(base->data + src->y * base->bytes_per_line)) + src->x;
        ito = idata;
        for (i = 0; i < src->height; ++i) {
            for (j = 0; j < src->width; ++j) {
                uint32 orig = ipt[j];
                int alpha = orig >> 24;
                if (alpha == 0xff) {
                    ito[j] = orig;
                } else if (alpha == 0) {
                    ito[j] = 0x00000000;
                } else
                    ito[j] = (alpha << 24) |
                             ((COLOR_RED(orig) * alpha / 255) << 16) |
                             ((COLOR_GREEN(orig) * alpha / 255) << 8) |
                             ((COLOR_BLUE(orig) * alpha / 255));
            }
            ipt = (uint32 *)(((uint8 *) ipt) + base->bytes_per_line);
            ito = (uint32 *)(((uint8 *) ito) + stride);
        }
    } else if (base->image_type == it_true && base->trans != COLOR_UNKNOWN) {
        Color trans = base->trans;
        ipt = ((uint32 *)(base->data + src->y * base->bytes_per_line)) + src->x;
        ito = idata;
        for (i = 0; i < src->height; ++i) {
            for (j = 0; j < src->width; ++j) {
                if (ipt[j] == trans) {
                    ito[j] = 0x00000000;
                } else {
                    ito[j] = ipt[j] | 0xff000000;
                }
            }
            ipt = (uint32 *)(((uint8 *) ipt) + base->bytes_per_line);
            ito = (uint32 *)(((uint8 *) ito) + stride);
        }
    } else if (base->image_type == it_true) {
        ipt = ((uint32 *)(base->data + src->y * base->bytes_per_line)) + src->x;
        ito = idata;
        for (i = 0; i < src->height; ++i) {
            for (j = 0; j < src->width; ++j) {
                ito[j] = ipt[j] | 0xff000000;
            }
            ipt = (uint32 *)(((uint8 *) ipt) + base->bytes_per_line);
            ito = (uint32 *)(((uint8 *) ito) + stride);
        }
    } else if (base->image_type == it_index && base->clut->trans_index != COLOR_UNKNOWN) {
        int trans = base->clut->trans_index;
        Color *clut = base->clut->clut;
        pt = base->data + src->y * base->bytes_per_line + src->x;
        ito = idata;
        for (i = 0; i < src->height; ++i) {
            for (j = 0; j < src->width; ++j) {
                int index = pt[j];
                if (index == trans) {
                    ito[j] = 0x00000000;
                } else
                    /* In theory RGB24 images don't need the alpha channel set*/
                    /*  but there is a bug in Cairo 1.2, and they do. */
                {
                    ito[j] = clut[index] | 0xff000000;
                }
            }
            pt += base->bytes_per_line;
            ito = (uint32 *)(((uint8 *) ito) + stride);
        }
    } else if (base->image_type == it_index) {
        Color *clut = base->clut->clut;
        pt = base->data + src->y * base->bytes_per_line + src->x;
        ito = idata;
        for (i = 0; i < src->height; ++i) {
            for (j = 0; j < src->width; ++j) {
                int index = pt[j];
                ito[j] = clut[index] | 0xff000000;
            }
            pt += base->bytes_per_line;
            ito = (uint32 *)(((uint8 *) ito) + stride);
        }
#ifdef WORDS_BIGENDIAN
    } else if (base->image_type == it_mono && base->clut != NULL &&
               base->clut->trans_index != COLOR_UNKNOWN) {
        pt = base->data + src->y * base->bytes_per_line + (src->x >> 3);
        ito = idata;
        if (base->clut->trans_index == 0) {
            for (i = 0; i < src->height; ++i) {
                bit = (0x80 >> (src->x & 0x7));
                tobit = 0x80000000;
                for (j = jj = tjj = 0; j < src->width; ++j) {
                    if (pt[jj]&bit) {
                        ito[tjj] |= tobit;
                    }
                    if ((bit >>= 1) == 0) {
                        bit = 0x80;
                        ++jj;
                    }
                    if ((tobit >>= 1) == 0) {
                        tobit = 0x80000000;
                        ++tjj;
                    }
                }
                pt += base->bytes_per_line;
                ito = (uint32 *)(((uint8 *) ito) + stride);
            }
        } else {
            for (i = 0; i < src->height; ++i) {
                bit = (0x80 >> (src->x & 0x7));
                tobit = 0x80000000;
                for (j = jj = tjj = 0; j < src->width; ++j) {
                    if (!(pt[jj]&bit)) {
                        ito[tjj] |= tobit;
                    }
                    if ((bit >>= 1) == 0) {
                        bit = 0x80;
                        ++jj;
                    }
                    if ((tobit >>= 1) == 0) {
                        tobit = 0x80000000;
                        ++tjj;
                    }
                }
                pt += base->bytes_per_line;
                ito = (uint32 *)(((uint8 *) ito) + stride);
            }
        }
#else
    } else if (base->image_type == it_mono && base->clut != NULL &&
               base->clut->trans_index != COLOR_UNKNOWN) {
        pt = base->data + src->y * base->bytes_per_line + (src->x >> 3);
        ito = idata;
        if (base->clut->trans_index == 0) {
            for (i = 0; i < src->height; ++i) {
                bit = (0x80 >> (src->x & 0x7));
                tobit = 1;
                for (j = jj = tjj = 0; j < src->width; ++j) {
                    if (pt[jj]&bit) {
                        ito[tjj] |= tobit;
                    }
                    if ((bit >>= 1) == 0) {
                        bit = 0x80;
                        ++jj;
                    }
                    if ((tobit <<= 1) == 0) {
                        tobit = 0x1;
                        ++tjj;
                    }
                }
                pt += base->bytes_per_line;
                ito = (uint32 *)(((uint8 *) ito) + stride);
            }
        } else {
            for (i = 0; i < src->height; ++i) {
                bit = (0x80 >> (src->x & 0x7));
                tobit = 1;
                for (j = jj = tjj = 0; j < src->width; ++j) {
                    if (!(pt[jj]&bit)) {
                        ito[tjj] |= tobit;
                    }
                    if ((bit >>= 1) == 0) {
                        bit = 0x80;
                        ++jj;
                    }
                    if ((tobit <<= 1) == 0) {
                        tobit = 0x1;
                        ++tjj;
                    }
                }
                pt += base->bytes_per_line;
                ito = (uint32 *)(((uint8 *) ito) + stride);
            }
        }
#endif
    } else {
        Color fg = base->clut == NULL ? 0xffffff : base->clut->clut[1];
        Color bg = base->clut == NULL ? 0x000000 : base->clut->clut[0];
        /* In theory RGB24 images don't need the alpha channel set*/
        /*  but there is a bug in Cairo 1.2, and they do. */
        fg |= 0xff000000;
        bg |= 0xff000000;
        pt = base->data + src->y * base->bytes_per_line + (src->x >> 3);
        ito = idata;
        for (i = 0; i < src->height; ++i) {
            bit = (0x80 >> (src->x & 0x7));
            for (j = jj = 0; j < src->width; ++j) {
                ito[j] = (pt[jj] & bit) ? fg : bg;
                if ((bit >>= 1) == 0) {
                    bit = 0x80;
                    ++jj;
                }
            }
            pt += base->bytes_per_line;
            ito = (uint32 *)(((uint8 *) ito) + stride);
        }
    }
    cairo_surface_mark_dirty(cs);
    return cs;
}

static GImage *_GGDKDraw_GImageExtract(struct _GImage *base, GRect *src, GRect *size,
                                       double xscale, double yscale) {
    static GImage temp;
    static struct _GImage tbase;
    static uint8 *data;
    static int dlen;
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
    if (tbase.bytes_per_line * size->height > dlen) {
        data = realloc(data, dlen = tbase.bytes_per_line * size->height);
    }
    tbase.data = data;

    /* I used to use rint(x). Now I use floor(x). For normal images rint */
    /*  might be better, but for text we need floor */

    if (base->image_type == it_mono) {
        memset(data, 0, tbase.height * tbase.bytes_per_line);
        for (r = 0; r < size->height; ++r) {
            int or = ((int) floor((r + size->y) / yscale));
            uint8 *pt = data + r * tbase.bytes_per_line;
            uint8 *opt = base->data + or * base->bytes_per_line;
            for (c = 0; c < size->width; ++c) {
                int oc = ((int) floor((c + size->x) / xscale));
                if (opt[oc >> 3] & (0x80 >> (oc & 7))) {
                    pt[c >> 3] |= (0x80 >> (c & 7));
                }
            }
        }
    } else if (base->image_type == it_index) {
        for (r = 0; r < size->height; ++r) {
            int or = ((int) floor((r + size->y) / yscale));
            uint8 *pt = data + r * tbase.bytes_per_line;
            uint8 *opt = base->data + or * base->bytes_per_line;
            for (c = 0; c < size->width; ++c) {
                int oc = ((int) floor((c + size->x) / xscale));
                *pt++ = opt[oc];
            }
        }
    } else {
        for (r = 0; r < size->height; ++r) {
            int or = ((int) floor((r + size->y) / yscale));
            uint32 *pt = (uint32 *)(data + r * tbase.bytes_per_line);
            uint32 *opt = (uint32 *)(base->data + or * base->bytes_per_line);
            for (c = 0; c < size->width; ++c) {
                int oc = ((int) floor((c + size->x) / xscale));
                *pt++ = opt[oc];
            }
        }
    }
    return (&temp);
}

// Protected member functions

bool _GGDKDraw_InitPangoCairo(GGDKWindow gw) {
    if (gw->is_pixmap) {
        gw->cc = cairo_create(gw->cs);
        if (gw->cc == NULL) {
            Log(LOGDEBUG, "GGDKDRAW: Cairo context creation failed!");
            return false;
        }
    }

    // Establish Pango layout context
    gw->pango_layout = pango_layout_new(gw->display->pangoc_context);
    if (gw->pango_layout == NULL) {
        Log(LOGDEBUG, "GGDKDRAW: Pango layout creation failed!");
        if (gw->cc != NULL) {
            cairo_destroy(gw->cc);
            gw->cc = NULL;
        }
        return false;
    }

    return true;
}

cairo_region_t *_GGDKDraw_ExcludeChildRegions(GGDKWindow gw, cairo_region_t *r, bool force) {
    GList_Glib *children = gdk_window_peek_children(gw->w);
    cairo_region_t *reg = NULL;

    if (children) {
        if (r == NULL) {
            reg = gdk_window_get_clip_region(gw->w);
        } else {
            reg = cairo_region_copy(r);
        }

        while (children != NULL) {
            cairo_region_t *chr = gdk_window_get_clip_region((GdkWindow *)children->data);
            int dx, dy;

            gdk_window_get_position((GdkWindow *)children->data, &dx, &dy);
            cairo_region_translate(chr, dx, dy);
            cairo_region_subtract(reg, chr);
            cairo_region_destroy(chr);
            children = children->next;
        }
    } else if (force) {
        reg = gdk_window_get_clip_region(gw->w);
    }

    return reg;
}

void _GGDKDraw_CleanupAutoPaint(GGDKDisplay *gdisp) {
    if (gdisp->dirty_window != NULL) {
        GGDKWindow gw = gdisp->dirty_window;

        if (gw->cc != NULL) {
            cairo_destroy(gw->cc);
            gw->cc = NULL;
        }
        if (gw->is_in_paint) {
            //Log(LOGDEBUG, "ENDED PAINT %p", gw);
#if !defined(GDK_WINDOWING_WIN32) && !defined(GDK_WINDOWING_QUARTZ)
            if (gw->cs != NULL) {
                assert(gw->expose_region != NULL);
#ifdef GGDKDRAW_GDK_3_22
                cairo_t *cc = cairo_reference(gdk_drawing_context_get_cairo_context(gw->drawing_ctx));
#else
                cairo_t *cc = gdk_cairo_create(gw->w);
#endif

                gdk_cairo_region(cc, gw->expose_region);
                cairo_clip(cc);

                cairo_set_source_surface(cc, gw->cs, 0, 0);
                cairo_set_operator(cc, CAIRO_OPERATOR_SOURCE);
                cairo_paint(cc);

                cairo_destroy(cc);
                cairo_region_destroy(gw->expose_region);
                gw->expose_region = NULL;
                cairo_surface_destroy(gw->cs);
                gw->cs = NULL;
            }
#endif
#ifdef GGDKDRAW_GDK_3_22
            gdk_window_end_draw_frame(gw->w, gw->drawing_ctx);
#else
            gdk_window_end_paint(gw->w);
#endif
            gw->is_in_paint = false;
        }
        gdisp->dirty_window = NULL;
    }
}

void GGDKDrawPushClip(GWindow w, GRect *rct, GRect *old) {
    //Log(LOGDEBUG, " ");

    // Return the current clip, and intersect the current clip with the desired
    // clip to get the new clip.
    _GGDKDraw_CheckAutoPaint((GGDKWindow)w);

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

    GGDKWindow gw = (GGDKWindow)w;

    cairo_save(gw->cc);
    cairo_new_path(gw->cc);
    cairo_rectangle(gw->cc, gw->ggc->clip.x, gw->ggc->clip.y,
                    gw->ggc->clip.width, gw->ggc->clip.height);
    cairo_clip(gw->cc);
}

void GGDKDrawPopClip(GWindow w, GRect *old) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    if (old) {
        gw->ggc->clip = *old;
    }
    // cc can be NULL if the autopaint cleanup had to run because
    // a raise/lower/move/resize function was called.
    if (gw->cc != NULL) {
        cairo_restore(gw->cc);
    }
}


void GGDKDrawSetDifferenceMode(GWindow w) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);
    cairo_set_operator(gw->cc, CAIRO_OPERATOR_DIFFERENCE);
    cairo_set_antialias(gw->cc, CAIRO_ANTIALIAS_NONE);
}


void GGDKDrawClear(GWindow w, GRect *rect) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);
    GRect temp, *r = rect, old;
    if (r == NULL) {
        temp = gw->pos;
        temp.x = temp.y = 0;
        r = &temp;
    }
    GGDKDrawPushClip((GWindow)gw, r, &old);
    cairo_set_source_rgba(gw->cc, COLOR_RED(gw->ggc->bg) / 255.,
                          COLOR_GREEN(gw->ggc->bg) / 255.,
                          COLOR_BLUE(gw->ggc->bg) / 255., 1.0);
    cairo_paint(gw->cc);
    GGDKDrawPopClip((GWindow)gw, &old);
}

void GGDKDrawDrawLine(GWindow w, int32 x, int32 y, int32 xend, int32 yend, Color col) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    w->ggc->fg = col;

    int width = GGDKDrawSetline(gw, gw->ggc);
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

void GGDKDrawDrawArrow(GWindow w, int32 x, int32 y, int32 xend, int32 yend, int16 arrows, Color col) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    gw->ggc->fg = col;

    int width = GGDKDrawSetline(gw, gw->ggc);
    if (width & 1) {
        x += .5;
        y += .5;
        xend += .5;
        yend += .5;
    }

    const double head_angle = 0.5;
    double angle = atan2(yend - y, xend - x) + FF_PI;
    double length = sqrt((x - xend) * (x - xend) + (y - yend) * (y - yend));

    cairo_new_path(gw->cc);
    cairo_move_to(gw->cc, x, y);
    cairo_line_to(gw->cc, xend, yend);
    cairo_stroke(gw->cc);

    if (length < 2) { //No point arrowing something so small

        return;
    } else if (length > 20) {
        length = 10;
    } else {
        length *= 2. / 3.;
    }
    cairo_new_path(gw->cc);
    cairo_move_to(gw->cc, xend, yend);
    cairo_line_to(gw->cc, xend + length * cos(angle - head_angle), yend + length * sin(angle - head_angle));
    cairo_line_to(gw->cc, xend + length * cos(angle + head_angle), yend + length * sin(angle + head_angle));
    cairo_close_path(gw->cc);
    cairo_fill(gw->cc);

}

void GGDKDrawDrawRect(GWindow w, GRect *rect, Color col) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    gw->ggc->fg = col;

    int width = GGDKDrawSetline(gw, gw->ggc);
    cairo_new_path(gw->cc);
    if (width & 1) {
        cairo_rectangle(gw->cc, rect->x + .5, rect->y + .5, rect->width, rect->height);
    } else {
        cairo_rectangle(gw->cc, rect->x, rect->y, rect->width, rect->height);
    }
    cairo_stroke(gw->cc);

}

void GGDKDrawFillRect(GWindow w, GRect *rect, Color col) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    gw->ggc->fg = col;

    GGDKDrawSetcolfunc(gw, gw->ggc);

    cairo_new_path(gw->cc);
    cairo_rectangle(gw->cc, rect->x, rect->y, rect->width, rect->height);
    cairo_fill(gw->cc);

}

void GGDKDrawFillRoundRect(GWindow w, GRect *rect, int radius, Color col) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    gw->ggc->fg = col;

    GGDKDrawSetcolfunc(gw, gw->ggc);

    double degrees = FF_PI / 180.0;
    int rr = (radius <= (rect->height + 1) / 2) ? (radius > 0 ? radius : 0) : (rect->height + 1) / 2;
    cairo_new_path(gw->cc);
    cairo_arc(gw->cc, rect->x + rect->width - rr, rect->y + rr, rr, -90 * degrees, 0 * degrees);
    cairo_arc(gw->cc, rect->x + rect->width - rr, rect->y + rect->height - rr, rr, 0 * degrees, 90 * degrees);
    cairo_arc(gw->cc, rect->x + rr, rect->y + rect->height - rr, rr, 90 * degrees, 180 * degrees);
    cairo_arc(gw->cc, rect->x + rr, rect->y + rr, rr, 180 * degrees, 270 * degrees);
    cairo_close_path(gw->cc);
    cairo_fill(gw->cc);

}

void GGDKDrawDrawEllipse(GWindow w, GRect *rect, Color col) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    gw->ggc->fg = col;

    // It is tempting to use the cairo arc command and scale the
    //  coordinates to get an ellipse, but that distorts the stroke width.
    int lwidth = GGDKDrawSetline(gw, gw->ggc);
    double cx, cy, width, height;

    width = rect->width / 2.0;
    height = rect->height / 2.0;
    cx = rect->x + width;
    cy = rect->y + height;
    if (lwidth & 1) {
        if (rint(width) == width) {
            cx += .5;
        }
        if (rint(height) == height) {
            cy += .5;
        }
    }
    _GGDKDraw_EllipsePath(gw->cc, cx, cy, width, height);
    cairo_stroke(gw->cc);

}

void GGDKDrawFillEllipse(GWindow w, GRect *rect, Color col) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);


    gw->ggc->fg = col;
    GGDKDrawSetcolfunc(gw, gw->ggc);

    // It is tempting to use the cairo arc command and scale the
    //  coordinates to get an ellipse, but that distorts the stroke width.
    double cx, cy, width, height;
    width = rect->width / 2.0;
    height = rect->height / 2.0;
    cx = rect->x + width;
    cy = rect->y + height;
    _GGDKDraw_EllipsePath(gw->cc, cx, cy, width, height);
    cairo_fill(gw->cc);

}

/**
 *  \brief Draws an arc (circular and elliptical).
 *
 *  \param [in] gw The window to draw on.
 *  \param [in] rect The bounding box of the arc. If width!=height, then
 *                   an elliptical arc will be drawn.
 *  \param [in] sangle The start angle in degrees * 64 (Cartesian)
 *  \param [in] eangle The angle offset from the start in degrees * 64 (positive CCW)
 */
void GGDKDrawDrawArc(GWindow w, GRect *rect, int32 sangle, int32 eangle, Color col) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    gw->ggc->fg = col;

    // Leftover from XDrawArc: sangle/eangle in degrees*64.
    double start = -(sangle + eangle) * FF_PI / 11520., end = -sangle * FF_PI / 11520.;
    int width = GGDKDrawSetline(gw, gw->ggc);

    cairo_new_path(gw->cc);
    cairo_save(gw->cc);
    if (width & 1) {
        cairo_translate(gw->cc, rect->x + .5 + rect->width / 2., rect->y + .5 + rect->height / 2.);
    } else {
        cairo_translate(gw->cc, rect->x + rect->width / 2., rect->y + rect->height / 2.);
    }
    cairo_scale(gw->cc, rect->width / 2., rect->height / 2.);
    cairo_arc(gw->cc, 0., 0., 1., start, end);
    cairo_restore(gw->cc);
    cairo_stroke(gw->cc);

}

void GGDKDrawDrawPoly(GWindow w, GPoint *pts, int16 cnt, Color col) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    gw->ggc->fg = col;

    int width = GGDKDrawSetline(gw, gw->ggc);
    double off = (width & 1) ? .5 : 0;

    cairo_new_path(gw->cc);
    cairo_move_to(gw->cc, pts[0].x + off, pts[0].y + off);
    for (int i = 1; i < cnt; ++i) {
        cairo_line_to(gw->cc, pts[i].x + off, pts[i].y + off);
    }
    cairo_stroke(gw->cc);

}

void GGDKDrawFillPoly(GWindow w, GPoint *pts, int16 cnt, Color col) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    gw->ggc->fg = col;

    GGDKDrawSetcolfunc(gw, gw->ggc);

    cairo_new_path(gw->cc);
    cairo_move_to(gw->cc, pts[0].x, pts[0].y);
    for (int i = 1; i < cnt; ++i) {
        cairo_line_to(gw->cc, pts[i].x, pts[i].y);
    }
    cairo_close_path(gw->cc);
    cairo_fill(gw->cc);

    cairo_set_line_width(gw->cc, 1);
    cairo_new_path(gw->cc);
    cairo_move_to(gw->cc, pts[0].x + .5, pts[0].y + .5);
    for (int i = 1; i < cnt; ++i) {
        cairo_line_to(gw->cc, pts[i].x + .5, pts[i].y + .5);
    }
    cairo_close_path(gw->cc);
    cairo_stroke(gw->cc);

}

void GGDKDrawDrawImage(GWindow w, GImage *image, GRect *src, int32 x, int32 y) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    cairo_surface_t *is = _GGDKDraw_GImage2Surface(image, src), *cs = is;
    struct _GImage *base = (image->list_len == 0) ? image->u.image : image->u.images[0];

    if (cairo_image_surface_get_format(is) == CAIRO_FORMAT_A1) {
        /* No color info, just alpha channel */
        Color fg = base->clut->trans_index == 0 ? base->clut->clut[1] : base->clut->clut[0];
#ifdef GDK_WINDOWING_QUARTZ
        // The quartz backend cannot mask/render A1 surfaces directly
        // So render to intermediate ARGB32 surface first, then render that to screen
        cs = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, src->width, src->height);
        cairo_t *cc = cairo_create(cs);
        cairo_set_source_rgba(cc, COLOR_RED(fg) / 255.0, COLOR_GREEN(fg) / 255.0, COLOR_BLUE(fg) / 255.0, 1.0);
        cairo_mask_surface(cc, is, 0, 0);
        cairo_destroy(cc);
#else
        cairo_set_source_rgba(gw->cc, COLOR_RED(fg) / 255.0, COLOR_GREEN(fg) / 255.0, COLOR_BLUE(fg) / 255.0, 1.0);
        cairo_mask_surface(gw->cc, cs, x, y);
        cs = NULL;
#endif
    }

    if (cs != NULL) {
        cairo_set_source_surface(gw->cc, cs, x, y);
        cairo_rectangle(gw->cc, x, y, src->width, src->height);
        cairo_fill(gw->cc);

        if (cs != is) {
            cairo_surface_destroy(cs);
        }
    }
    /* Clear source and mask, in case we need to */
    cairo_new_path(gw->cc);
    cairo_set_source_rgba(gw->cc, 0, 0, 0, 0);

    cairo_surface_destroy(is);
}

// What we really want to do is use the grey levels as an alpha channel
void GGDKDrawDrawGlyph(GWindow w, GImage *image, GRect *src, int32 x, int32 y) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    struct _GImage *base = (image->list_len) == 0 ? image->u.image : image->u.images[0];
    cairo_surface_t *is;

    if (base->image_type != it_index) {
        GGDKDrawDrawImage(w, image, src, x, y);
    } else {
        int stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, src->width);
        uint8 *basedata = malloc(stride * src->height),
               *data = basedata,
                *srcd = base->data + src->y * base->bytes_per_line + src->x;
        int factor = base->clut->clut_len == 256 ? 1 :
                     base->clut->clut_len == 16 ? 17 :
                     base->clut->clut_len == 4 ? 85 : 255;
        int i, j;
        Color fg = base->clut->clut[base->clut->clut_len - 1];

        for (i = 0; i < src->height; ++i) {
            for (j = 0; j < src->width; ++j) {
                data[j] = factor * srcd[j];
            }
            srcd += base->bytes_per_line;
            data += stride;
        }
        is = cairo_image_surface_create_for_data(basedata, CAIRO_FORMAT_A8,
                src->width, src->height, stride);
        cairo_set_source_rgba(gw->cc, COLOR_RED(fg) / 255.0, COLOR_GREEN(fg) / 255.0, COLOR_BLUE(fg) / 255.0, 1.0);
        cairo_mask_surface(gw->cc, is, x, y);
        /* I think the mask is sufficient, setting a rectangle would provide */
        /*  a new mask? */
        /*cairo_rectangle(gw->cc,x,y,src->width,src->height);*/
        /* I think setting the mask also draws... at least so the tutorial implies */
        /* cairo_fill(gw->cc);*/
        /* Presumably that doesn't leave the mask surface pattern lying around */
        /* but dereferences it so we can free it */
        cairo_surface_destroy(is);
        free(basedata);
    }

}

void GGDKDrawDrawImageMagnified(GWindow w, GImage *image, GRect *src, int32 x, int32 y, int32 width, int32 height) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    _GGDKDraw_CheckAutoPaint(gw);

    struct _GImage *base = (image->list_len == 0) ? image->u.image : image->u.images[0];
    GRect full;
    double xscale, yscale;
    GRect viewable;

    viewable = gw->ggc->clip;
    if (viewable.width > gw->pos.width - viewable.x) {
        viewable.width = gw->pos.width - viewable.x;
    }
    if (viewable.height > gw->pos.height - viewable.y) {
        viewable.height = gw->pos.height - viewable.y;
    }

    xscale = (base->width >= 1) ? ((double)(width)) / (base->width) : 1;
    yscale = (base->height >= 1) ? ((double)(height)) / (base->height) : 1;
    /* Intersect the clip rectangle with the scaled image to find the */
    /*  portion of screen that we want to draw */
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

    /* Now find that same rectangle in the coordinates of the unscaled image */
    /* (translation & scale) */
    viewable.x -= x;
    viewable.y -= y;
    full.x = viewable.x / xscale;
    full.y = viewable.y / yscale;
    full.width = viewable.width / xscale;
    full.height = viewable.height / yscale;
    if (full.x + full.width > base->width) {
        full.width = base->width - full.x;    /* Rounding errors */
    }
    if (full.y + full.height > base->height) {
        full.height = base->height - full.y;    /* Rounding errors */
    }
    /* Rounding errors */
    {
        GImage *temp = _GGDKDraw_GImageExtract(base, &full, &viewable, xscale, yscale);
        GRect src;
        src.x = src.y = 0;
        src.width = viewable.width;
        src.height = viewable.height;
        GGDKDrawDrawImage(w, temp, &src, x + viewable.x, y + viewable.y);
    }

}

void GGDKDrawDrawPixmap(GWindow w, GWindow pixmap, GRect *src, int32 x, int32 y) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w, gpixmap = (GGDKWindow)pixmap;
    _GGDKDraw_CheckAutoPaint(gw);

    if (!gpixmap->is_pixmap) {
        return;
    }

    cairo_set_source_surface(gw->cc, gpixmap->cs, x - src->x, y - src->y);
    cairo_rectangle(gw->cc, x, y, src->width, src->height);
    cairo_fill(gw->cc);
}

enum gcairo_flags GGDKDrawHasCairo(GWindow UNUSED(w)) {
    //Log(LOGDEBUG, " ");
    return gc_all;
}

void GGDKDrawPathStartNew(GWindow w) {
    //Log(LOGDEBUG, " ");
    _GGDKDraw_CheckAutoPaint((GGDKWindow)w);
    cairo_new_path(((GGDKWindow)w)->cc);
}

void GGDKDrawPathClose(GWindow w) {
    //Log(LOGDEBUG, " ");
    cairo_close_path(((GGDKWindow)w)->cc);
}

void GGDKDrawPathMoveTo(GWindow w, double x, double y) {
    //Log(LOGDEBUG, " ");
    _GGDKDraw_CheckAutoPaint((GGDKWindow)w);
    cairo_move_to(((GGDKWindow)w)->cc, x, y);
}

void GGDKDrawPathLineTo(GWindow w, double x, double y) {
    //Log(LOGDEBUG, " ");
    cairo_line_to(((GGDKWindow)w)->cc, x, y);
}

void GGDKDrawPathCurveTo(GWindow w, double cx1, double cy1, double cx2, double cy2, double x, double y) {
    //Log(LOGDEBUG, " ");
    cairo_curve_to(((GGDKWindow)w)->cc, cx1, cy1, cx2, cy2, x, y);
}

void GGDKDrawPathStroke(GWindow w, Color col) {
    //Log(LOGDEBUG, " ");
    w->ggc->fg = col;
    GGDKDrawSetline((GGDKWindow) w, w->ggc);
    cairo_stroke(((GGDKWindow)w)->cc);
}

void GGDKDrawPathFill(GWindow w, Color col) {
    //Log(LOGDEBUG, " ");
    cairo_set_source_rgba(((GGDKWindow) w)->cc, COLOR_RED(col) / 255.0, COLOR_GREEN(col) / 255.0, COLOR_BLUE(col) / 255.0,
                          (col >> 24) / 255.0);
    cairo_fill(((GGDKWindow) w)->cc);
}

void GGDKDrawPathFillAndStroke(GWindow w, Color fillcol, Color strokecol) {
    //Log(LOGDEBUG, " ");
    // This function is unused, so it's unclear if it's implemented correctly.
    GGDKWindow gw = (GGDKWindow) w;

    cairo_save(gw->cc);
    cairo_set_source_rgba(gw->cc, COLOR_RED(fillcol) / 255.0, COLOR_GREEN(fillcol) / 255.0, COLOR_BLUE(fillcol) / 255.0,
                          (fillcol >> 24) / 255.0);
    cairo_fill(gw->cc);
    cairo_restore(gw->cc);
    w->ggc->fg = strokecol;
    GGDKDrawSetline(gw, gw->ggc);
    cairo_fill_preserve(gw->cc);
    cairo_stroke(gw->cc);
}

void GGDKDrawStartNewSubPath(GWindow w) {
    //Log(LOGDEBUG, " ");
    _GGDKDraw_CheckAutoPaint((GGDKWindow)w);
    cairo_new_sub_path(((GGDKWindow)w)->cc);
}

int GGDKDrawFillRuleSetWinding(GWindow w) {
    //Log(LOGDEBUG, " ");
    _GGDKDraw_CheckAutoPaint((GGDKWindow)w);
    cairo_set_fill_rule(((GGDKWindow)w)->cc, CAIRO_FILL_RULE_WINDING);
    return 1;
}

int GGDKDrawDoText8(GWindow w, int32 x, int32 y, const char *text, int32 cnt, Color col, enum text_funcs drawit,
                    struct tf_arg *arg) {
    //Log(LOGDEBUG, " ");

    GGDKWindow gw = (GGDKWindow) w;
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
    pango_layout_get_pixel_extents(gw->pango_layout, &ink, &rect);

    if (drawit == tf_drawit) {
        _GGDKDraw_CheckAutoPaint(gw);
        _GGDKDraw_MyCairoRenderLayout(gw->cc, col, gw->pango_layout, x, y);
    } else if (drawit == tf_rect) {
        PangoLayoutIter *iter;
        PangoLayoutRun *run;
        PangoFontMetrics *fm;

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
    //Log(LOGDEBUG, " ");
    _GGDKDraw_CheckAutoPaint((GGDKWindow)w);
    cairo_save(((GGDKWindow)w)->cc);
}

void GGDKDrawClipPreserve(GWindow w) {
    //Log(LOGDEBUG, " ");
    _GGDKDraw_CheckAutoPaint((GGDKWindow)w);
    cairo_clip_preserve(((GGDKWindow)w)->cc);
}

// PANGO LAYOUT

void GGDKDrawGetFontMetrics(GWindow gw, GFont *fi, int *as, int *ds, int *ld) {
    //Log(LOGDEBUG, " ");

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
    if (pfont != NULL) {
        g_object_unref(pfont);
    }
}

void GGDKDrawLayoutInit(GWindow w, char *text, int cnt, GFont *fi) {
    //Log(LOGDEBUG, " ");
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
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;

    _GGDKDraw_MyCairoRenderLayout(gw->cc, fg, gw->pango_layout, x, y);
}

void GGDKDrawLayoutIndexToPos(GWindow w, int index, GRect *pos) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    PangoRectangle rect;

    pango_layout_index_to_pos(gw->pango_layout, index, &rect);
    pos->x = rect.x / PANGO_SCALE;
    pos->y = rect.y / PANGO_SCALE;
    pos->width  = rect.width / PANGO_SCALE;
    pos->height = rect.height / PANGO_SCALE;
}

int GGDKDrawLayoutXYToIndex(GWindow w, int x, int y) {
    //Log(LOGDEBUG, " ");
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
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    PangoRectangle rect;

    pango_layout_get_pixel_extents(gw->pango_layout, NULL, &rect);
    size->x = rect.x;
    size->y = rect.y;
    size->width  = rect.width;
    size->height = rect.height;
}

void GGDKDrawLayoutSetWidth(GWindow w, int width) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;

    pango_layout_set_width(gw->pango_layout, (width == -1) ? -1 : width * PANGO_SCALE);
}

int GGDKDrawLayoutLineCount(GWindow w) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;

    return pango_layout_get_line_count(gw->pango_layout);
}

int GGDKDrawLayoutLineStart(GWindow w, int l) {
    //Log(LOGDEBUG, " ");
    GGDKWindow gw = (GGDKWindow)w;
    PangoLayoutLine *line = pango_layout_get_line(gw->pango_layout, l);

    if (line == NULL) {
        return -1;
    }

    return line->start_index;
}

#endif // FONTFORGE_CAN_USE_GDK
