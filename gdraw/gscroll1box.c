/* Copyright (C) 2000-2012 by George Williams, 2021 by Skef Iterum */
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

#include "gdraw.h"
#include "gdrawP.h"
#include "ggadgetP.h"
#include "gkeysym.h"
#include "gwidget.h"
#include "ustring.h"

static GBox scroll1box_box = GBOX_EMPTY; /* Don't initialize here */;

extern GResInfo gmenubar_ri;
GResInfo gscroll1box_ri = {
    &gmenubar_ri, &ggadget_ri, NULL, NULL,
    &scroll1box_box,
    NULL,
    NULL,
    NULL,
    N_("Scroll Box"),
    N_("A box used to scroll widgets vertically or horizontally"),
    "GScroll1Box",
    "Gdraw",
    false,
    false,
    omf_border_type | omf_border_width | omf_padding,
    { bt_none, 0, 1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

static void GScroll1BoxDestroy(GGadget *g) {
    GScroll1Box *s1b = (GScroll1Box *) g;

    if (s1b == NULL) {
        return;
    }

    if (s1b->sb != NULL) {
        GGadgetDestroy(s1b->sb);
    }
    if (s1b->nested != NULL) {
        GDrawSetUserData(s1b->nested, NULL);
        GDrawDestroyWindow(s1b->nested);
    }
    free(s1b->children);
    _ggadget_destroy(g);
}

static void GScroll1Box_SetScroll(GScroll1Box *s1b) {
    int need_sb, pagewidth = s1b->g.inner.width, pageheight = s1b->g.inner.height;

    if (s1b->vertical) {
        need_sb = pageheight < s1b->subheight;
        GGadgetMove(s1b->sb, s1b->g.inner.x + s1b->g.inner.width - s1b->sbsize, s1b->g.inner.y);
        GGadgetResize(s1b->sb, s1b->sbsize, s1b->g.inner.height);
        if (need_sb || s1b->always_show_sb) {
            pagewidth -= s1b->sbsize;
            GScrollBarSetBounds(s1b->sb, 0, s1b->subheight, pageheight);
        }
    } else {
        if (s1b->sized_for_sb) {
            need_sb = pagewidth < s1b->subwidth;
        }
        GGadgetMove(s1b->sb, s1b->g.inner.x, s1b->g.inner.y + s1b->g.inner.height - s1b->sbsize);
        GGadgetResize(s1b->sb, s1b->g.inner.width, s1b->sbsize);
        if (need_sb || s1b->always_show_sb) {
            pageheight -= s1b->sbsize;
            GScrollBarSetBounds(s1b->sb, 0, s1b->subwidth, pagewidth);
        }
    }
    GGadgetSetVisible(s1b->sb, s1b->always_show_sb || need_sb);
    GGadgetSetEnabled(s1b->sb, need_sb);
    // Subwindow
    GDrawResize(s1b->nested, pagewidth, pageheight);
    GDrawRequestExpose(s1b->nested, NULL, false);
}

static void GScroll1BoxMove(GGadget *g, int32_t x, int32_t y) {
    GScroll1Box *s1b = (GScroll1Box *) g;
    int offx = x - g->r.x, offy = y - g->r.y;

    GDrawMove(s1b->nested, g->inner.x + offx, g->inner.y + offy);
    GGadgetMove(s1b->sb, s1b->sb->r.x + offx, s1b->sb->r.y + offy);
    _ggadget_move(g, x, y);
}

static void GScroll1BoxMoveChildren(GScroll1Box *s1b, int32_t diff) {
    for (int c = 0; c < s1b->count; ++c) {
        GGadget *g = s1b->children[c];
        if (s1b->vertical) {
            GGadgetMove(g, g->r.x, g->r.y - diff);
        } else {
            GGadgetMove(g, g->r.x - diff, g->r.y);
        }
    }
}

static int GScroll1Box_Scroll(GGadget *g, GEvent *e) {
    if (e->type != et_controlevent || e->u.control.subtype != et_scrollbarchange) {
        return true;
    }

    struct sbevent *sb = &e->u.control.u.sb;
    GScroll1Box *s1b = (GScroll1Box *) g->data;
    int newpos = s1b->offset, winsize, subsize;
    GRect rect;

    GDrawGetSize(s1b->nested, &rect);
    if (s1b->vertical) {
        winsize = rect.height;
        subsize = s1b->subheight;
    } else {
        winsize = rect.width;
        subsize = s1b->subwidth;
    }

    switch (sb->type) {
    case et_sb_top:
        newpos = 0;
        break;
    case et_sb_uppage:
        newpos -= subsize - winsize;
        break;
    case et_sb_up:
        newpos -= s1b->scrollchange;
        break;
    case et_sb_down:
        newpos += s1b->scrollchange;
        break;
    case et_sb_downpage:
        newpos += subsize - winsize;
        break;
    case et_sb_bottom:
        newpos = subsize - winsize;
        break;
    case et_sb_thumb:
    case et_sb_thumbrelease:
        newpos = sb->pos;
        break;
    case et_sb_halfup: case et_sb_halfdown: break;
    }
    if (newpos < 0) {
        newpos = 0;
    } else if (newpos > subsize - winsize) {
        newpos = subsize - winsize;
    }

    if (newpos != s1b->offset) {
        int diff = newpos - s1b->offset;
        s1b->offset = newpos;
        GScrollBarSetPos(s1b->sb, newpos);
        GRect clip = rect;
        clip.x = clip.y = 0;
        GScroll1BoxMoveChildren(s1b, diff);
        GDrawRequestExpose(s1b->nested, &clip, false);
    }
    return true;
}

/* A Scroll1Box will often be embedded in a GHVBox, in which case the desired
 * size will translate in the miminum size of the field. Therefore the desired
 * size should be as small as practical.
 *
 * That means the DesiredSize in that dimension should be the maximum of the
 * DesiredSize of each non-gflowbox child in that dimension and the "squashed
 * size" of each gflowbox child in that dimension.
 *
 * The scrolling direction should be min scroll size (so that the scrollbar can
 * be operational or the combined height of the children, which ever is
 * smaller.
 */

void _GScroll1BoxGetDesiredSize(GGadget *g, GRect *outer, GRect *inner, int big) {
    GScroll1Box *s1b = (GScroll1Box *) g;
    int bp = GBoxBorderWidth(g->base, g->box);
    int sbwidth, sbheight;
    int width = 0, height = 0, c, flowlabelsize = 0, t;
    GRect rect;

    if (s1b->align_flow_labels) {
        for (c = 0; c < s1b->count; ++c)
            if (GGadgetIsGFlowBox(s1b->children[c])) {
		t = GFlowBoxGetLabelSize(s1b->children[c]);
		if (flowlabelsize < t)
		    flowlabelsize = t;
	    }
        for (c = 0; c < s1b->count; ++c)
            if (GGadgetIsGFlowBox(s1b->children[c]))
                GFlowBoxSetLabelSize(s1b->children[c], flowlabelsize);
    }

    for (c = 0; c < s1b->count; ++c) {
        if (GGadgetIsGFlowBox(s1b->children[c])) {
            _GFlowBoxGetDesiredSize(s1b->children[c], &rect, NULL, !big, true);
        } else {
            GGadgetGetDesiredSize(s1b->children[c], &rect, NULL);
        }
        if (s1b->vertical) {
            if (rect.width > width) {
                width = rect.width;
            }
            height += rect.height;
            if (c > 0) {
                height += s1b->pad;
            }
        } else {
            if (rect.height > height) {
                rect.height = height;
            }
            width += rect.width;
            if (c > 0) {
                width += s1b->pad;
            }
        }
    }

    if (!big) {
        GGadgetGetDesiredSize(s1b->sb, &rect, NULL);
        sbwidth = rect.width;
        sbheight = rect.height;
        s1b->sbsize = s1b->vertical ? sbwidth : sbheight;
    }

    if (s1b->vertical) {
        if (!big) {
            s1b->minsize = width;
            s1b->oppoatmin = height;
        }
        width += s1b->sbsize;
        if (height < g->desired_height) {
            height = g->desired_height;
        } else if (!big && height > sbheight) {
            height = sbheight;
        }
    } else {
        if (!big) {
            s1b->minsize = height;
            s1b->oppoatmin = width;
        }
        height += s1b->sbsize;
        if (width < g->desired_width) {
            width = g->desired_width;
        } else if (!big && width > sbwidth) {
            width = sbwidth;
        }
    }

    if (inner != NULL) {
        inner->x = inner->y = bp;
        inner->width = width;
        inner->height = height;
    }
    if (outer != NULL) {
        outer->x = outer->y = 0;
        outer->width = width + 2 * bp;
        outer->height = height + 2 * bp;
    }
}

static void GScroll1BoxGetDesiredSize(GGadget *g, GRect *outer, GRect *inner) {
    _GScroll1BoxGetDesiredSize(g, outer, inner, false);
}

static int GScroll1BoxExpose(GWindow pixmap, GGadget *g, GEvent *event) {
    if (g->state != gs_invisible) {
        GBoxDrawBorder(pixmap, &g->r, g->box, g->state, false);
    }
    return true;
}

static void GScroll1BoxRedraw(GGadget *g) {
    GScroll1Box *s1b = (GScroll1Box *) g;
    if (s1b->sb != NULL) {
        GGadgetRedraw(s1b->sb);
    }
    GDrawRequestExpose(s1b->nested, NULL, false);
    _ggadget_redraw(g);
}

struct childdata {
    int size;
    int offset;
};

static void GScroll1BoxResize(GGadget *g, int32_t width, int32_t height) {
    GScroll1Box *s1b = (GScroll1Box *) g;
    int c, move, exp, has_just;
    int old_enabled = GDrawEnableExposeRequests(g->base, false);
    struct childdata *cd = calloc(s1b->count, sizeof(struct childdata));
    GRect rect;

    // This gadget
    _ggadget_resize(g, width, height);

    width = g->inner.width;
    height = g->inner.height;

    has_just = s1b->just & (gg_s1_right | gg_s1_bottom | gg_s1_center | gg_s1_expand);

    // Children
    if (s1b->vertical) {
        if (width < s1b->minsize + s1b->sbsize) {
            width = s1b->minsize + s1b->sbsize;
        }
        if ((!(s1b->subwidth == width             && !s1b->sized_for_sb)
                && !(s1b->subwidth == width - s1b->sbsize && s1b->sized_for_sb))
                || s1b->subheight != height) {
            s1b->sized_for_sb = false;
            for (int i = 0; i < 2; ++i) {
                s1b->subwidth = width;
                s1b->subheight = 0;
                for (c = 0; c < s1b->count; ++c) {
                    rect.height = -1;
                    rect.width = width;
                    if (GGadgetIsGFlowBox(s1b->children[c])) {
                        GGadgetSetDesiredSize(s1b->children[c], &rect, NULL);
                    }
                    GGadgetGetDesiredSize(s1b->children[c], &rect, NULL);
                    if (c > 0) {
                        s1b->subheight += s1b->pad;
                    }
                    cd[c].size = rect.height;
                    cd[c].offset = s1b->subheight;
                    s1b->subheight += rect.height;
                }
                if (i == 0) {
                    s1b->oppoatcur = s1b->subheight;
                }
                if (s1b->subheight <= height) {
                    break;
                }
                if (i == 0) {
                    width -= s1b->sbsize;
                    s1b->sized_for_sb = true;
                }
            }
            move = height - s1b->subheight;
            exp = move / s1b->count;
            if (move < 0 && s1b->offset > -move) {
                s1b->offset = -move;
            }
            for (c = 0; c < s1b->count; ++c) {
                if (move > 0) {
                    if (s1b->just & gg_s1_bottom) {
                        cd[c].offset += move;
                    } else if (s1b->just & gg_s1_center) {
                        cd[c].offset += move / 2;
                    } else if (s1b->just & gg_s1_expand) {
                        cd[c].size += exp;
                        cd[c].offset += c * exp;
                    }
                }
                GGadgetResize(s1b->children[c], width, cd[c].size);
                GGadgetMove(s1b->children[c], 0, cd[c].offset - s1b->offset);
            }
            if (has_just && move > 0) {
                s1b->subheight += move;
            }
        }
    } else {
        if (height < s1b->minsize) {
            height = s1b->minsize;
        }
        if ((!(s1b->subheight == height             && !s1b->sized_for_sb)
                && !(s1b->subheight == height - s1b->sbsize && s1b->sized_for_sb))
                || s1b->subwidth != width) {
            s1b->sized_for_sb = false;
            for (int i = 0; i < 2; ++i) {
                s1b->subheight = height;
                s1b->subwidth = 0;
                for (c = 0; c < s1b->count; ++c) {
                    rect.width = -1;
                    rect.height = height;
                    if (GGadgetIsGFlowBox(s1b->children[c])) {
                        GGadgetSetDesiredSize(s1b->children[c], &rect, NULL);
                    }
                    GGadgetGetDesiredSize(s1b->children[c], &rect, NULL);
                    if (c > 0) {
                        s1b->subwidth += s1b->pad;
                    }
                    cd[c].size = rect.width;
                    cd[c].offset = s1b->subwidth;
                    s1b->subwidth += rect.width;
                }
                if (i == 0) {
                    s1b->oppoatcur = s1b->subwidth;
                }
                if (s1b->subwidth <= width) {
                    break;
                }
                if (i == 0) {
                    height -= s1b->sbsize;
                    s1b->sized_for_sb = true;
                }
            }
            move = width - s1b->subwidth;
            exp = move / s1b->count;
            if (move < 0 && s1b->offset > -move) {
                s1b->offset = -move;
            }
            for (c = 0; c < s1b->count; ++c) {
                if (move > 0) {
                    if (s1b->just & gg_s1_right) {
                        cd[c].offset += move;
                    } else if (s1b->just & gg_s1_center) {
                        cd[c].offset += move / 2;
                    } else if (s1b->just & gg_s1_expand) {
                        cd[c].size += exp;
                        cd[c].offset += c * exp;
                    }
                }
                GGadgetResize(s1b->children[c], cd[c].size, height);
                GGadgetMove(s1b->children[c], cd[c].offset - s1b->offset, 0);
            }
            if (has_just && move > 0) {
                s1b->subwidth += move;
            }
        }
    }

    free(cd);

    GScroll1Box_SetScroll(s1b);

    GDrawEnableExposeRequests(g->base, old_enabled);
    GDrawRequestExpose(g->base, &g->r, false);
}

int GScroll1BoxMinOppoSize(GGadget *g) {
    GScroll1Box *s1b = (GScroll1Box *) g;

    return s1b->oppoatcur + g->r.height - g->inner.height;
}

int GScroll1BoxSBSize(GGadget *g) {
    GScroll1Box *s1b = (GScroll1Box *) g;
    return s1b->sbsize;
}

static void GScroll1BoxSetVisible(GGadget *g, int visible) {
    GScroll1Box *s1b = (GScroll1Box *) g;
    if (s1b->sb != NULL) {
        if (visible) {
            GScroll1Box_SetScroll(s1b);
        } else {
            GGadgetSetVisible(s1b->sb, false);
        }
    }
    if (s1b->nested != NULL) {
        GDrawSetVisible(s1b->nested, visible);
    }
    _ggadget_setvisible(g, visible);
}

static int GScroll1BoxFillsWindow(GGadget *g) {
    return true;
}

static int gs1bsub_e_h(GWindow gw, GEvent *event) {
    GScroll1Box *s1b = (GScroll1Box *) GDrawGetUserData(gw);

    if (s1b == NULL) {
        return false;
    }

    if ((event->type == et_mouseup || event->type == et_mousedown) &&
            (event->u.mouse.button >= 4 && event->u.mouse.button <= 7)) {
        return GGadgetDispatchEvent((GGadget *) s1b->sb, event);
    }

    switch (event->type) {
    case et_mousedown:
    case et_mouseup:
        if (s1b->g.state == gs_disabled) {
            return false;
        }
        break;
    case et_destroy:
        s1b->nested = NULL;
        break;
    default:
        break;
    }
    return true;
}

struct gfuncs gscroll1box_funcs = {
    0,
    sizeof(struct gfuncs),

    GScroll1BoxExpose,
    _ggadget_noop,
    _ggadget_noop,
    NULL,
    NULL,
    NULL,
    NULL,

    GScroll1BoxRedraw,
    GScroll1BoxMove,
    GScroll1BoxResize,
    GScroll1BoxSetVisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    GScroll1BoxDestroy,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,

    GScroll1BoxGetDesiredSize,
    _ggadget_setDesiredSize,
    GScroll1BoxFillsWindow,
    NULL
};

void GScroll1BoxSetPadding(GGadget *g, int pad) {
    GScroll1Box *s1b = (GScroll1Box *) g;
    if (pad >= 0) {
        s1b->pad = pad;
    }
}

void GScroll1BoxSetSBAlwaysVisible(GGadget *g, int always) {
    ((GScroll1Box *) g)->always_show_sb = always;
}

GGadget *GScroll1BoxCreate(struct gwindow *base, GGadgetData *gd, void *data) {
    GWindowAttrs wattrs;
    GRect pos;
    GScroll1Box *s1b = calloc(1, sizeof(GScroll1Box));
    GGadgetData sub_gd;

    GResEditDoInit(&gscroll1box_ri);

    for (s1b->count = 0; gd->u.boxelements[s1b->count] != NULL; ++s1b->count);

    s1b->g.funcs = &gscroll1box_funcs;
    _GGadget_Create(&s1b->g, base, gd, data, &scroll1box_box);
    int bp = GBoxBorderWidth(base, s1b->g.box);
    s1b->pad = GDrawPointsToPixels(base, 4);
    s1b->scrollchange = GDrawPointsToPixels(base, 20);
    s1b->offset = 0;

    // temporary values before Resize
    s1b->g.r = (GRect) {
        0, 0, 100, 100
    };
    s1b->g.inner = (GRect) {
        bp, bp, 100 - 2 * bp, 100 - 2 * bp
    };

    if (gd->flags & gg_s1_vert) {
        s1b->vertical = true;
    }
    if (gd->flags & gg_s1_flowalign) {
        s1b->align_flow_labels = true;
    }
    s1b->just = gd->flags;

    memset(&wattrs, 0, sizeof(wattrs));
    wattrs.mask = wam_events | wam_cursor;
    if (s1b->g.box->main_background != COLOR_TRANSPARENT) {
        wattrs.mask |= wam_backcol;
    }
    wattrs.event_masks = ~(1 << et_charup);
    wattrs.cursor = ct_pointer;
    wattrs.background_color = s1b->g.box->main_background;
    pos = s1b->g.inner;
    s1b->nested = GWidgetCreateSubWindow(base, &pos, gs1bsub_e_h, s1b, &wattrs);

    s1b->g.takes_input = false;
    s1b->g.takes_keyboard = false;
    s1b->g.focusable = false;

    memset(&sub_gd, 0, sizeof(sub_gd));
    // Will redo in GetDesiredSize
    s1b->sbsize = GDrawPointsToPixels(base, _GScrollBar_Width);
    if (s1b->vertical) {
        sub_gd.pos = (GRect) {
            100 - bp - s1b->sbsize, bp, s1b->sbsize, 100 - 2 * bp
        };
        sub_gd.flags = gg_enabled | gg_visible | gg_sb_vert | gg_pos_in_pixels;
    } else {
        sub_gd.pos = (GRect) {
            bp, 100 - bp - s1b->sbsize, 100 - 2 * bp, s1b->sbsize
        };
        sub_gd.flags = gg_enabled | gg_visible | gg_pos_in_pixels;
    }
    sub_gd.handle_controlevent = GScroll1Box_Scroll;
    s1b->sb = GScrollBarCreate(base, &sub_gd, s1b);
    s1b->sb->contained = true;

    s1b->children = malloc(s1b->count * sizeof(GGadget *));
    for (int c = 0; c < s1b->count; ++c) {
        GGadgetCreateData *gcd = gd->u.boxelements[c];
        if (gcd == GCD_HPad10) {
            s1b->children[c] = (GGadget *) gcd;
        } else {
            gcd->gd.pos.x = gcd->gd.pos.y = 0;
            s1b->children[c] = gcd->ret = (gcd->creator)(s1b->nested, &gcd->gd, gcd->data);
        }
    }

    if (s1b->g.state != gs_invisible) {
        GDrawSetVisible(s1b->nested, true);
    }

    return &s1b->g;
}

/*
void GScroll1BoxFitWindow(GGadget *g) {
    GRect outer, cur, screen;

    GScroll1BoxGetDesiredSize(g, &outer, NULL);
    GDrawGetSize(GDrawGetRoot(NULL),&screen);
    if ( outer.width > screen.width-20 ) outer.width = screen.width-20;
    if ( outer.height > screen.height-40 ) outer.height = screen.height-40;
    GDrawGetSize(g->base,&cur);
    outer.width += 2*g->r.x;
    outer.height += 2*g->r.y;
    if ( cur.width!=outer.width || cur.height!=outer.height ) {
	GDrawResize(g->base, outer.width, outer.height );
        // We want to get the resize before we set the window visible
        //  and window managers make synchronizing an issue...
	GDrawSync(GDrawGetDisplayOfWindow(g->base));
	GDrawProcessPendingEvents(GDrawGetDisplayOfWindow(g->base));
	GDrawSync(GDrawGetDisplayOfWindow(g->base));
	GDrawProcessPendingEvents(GDrawGetDisplayOfWindow(g->base));
    } else
	GGadgetResize(g, outer.width-2*g->r.x, outer.height-2*g->r.y );
}
*/
