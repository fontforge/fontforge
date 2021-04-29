/* Copyright (C) 2000-2012 by George Williams */
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
#include "ggadgetP.h"

GBox _GGroup_LineBox = GBOX_EMPTY; /* Don't initialize here */
static GBox group_box = GBOX_EMPTY; /* Don't initialize here */

static GGadgetCreateData gline_gcd[] = {
    { GLineCreate, { { 0, 0, 100, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { NULL }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *larray[] = { GCD_Glue, &gline_gcd[0], GCD_Glue, NULL, NULL };
static GGadgetCreateData linebox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) larray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
extern GResInfo ghvbox_ri;
static GResInfo gline_ri = {
    &ghvbox_ri, &ggadget_ri, NULL, NULL,
    &_GGroup_LineBox,
    NULL,
    &linebox,
    NULL,
    N_("Line"),
    N_("A separator line drawn across a dialog or in a menu"),
    "GLine",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_border_shape|omf_padding,
    { bt_engraved, bs_rect, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
GResInfo ggroup_ri = {
    &gline_ri, &ggadget_ri, NULL, NULL,
    &group_box,
    NULL,
    NULL,
    NULL,
    N_("Group"),
    N_("A grouping of elements"),
    "GGroup",
    "Gdraw",
    false,
    false,
    omf_border_type|omf_border_shape|omf_padding|omf_main_background|omf_disabled_background,
    { bt_engraved, bs_rect, 0, 0, 0, 0, 0, 0, 0, 0, COLOR_TRANSPARENT, 0, COLOR_TRANSPARENT, 0, 0 ,0 ,0 ,0 ,0 },
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

void _GGroup_Init(void) {
    GResEditDoInit(&ggroup_ri);
    GResEditDoInit(&gline_ri);
}

static int ggroup_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GRect old1, border;

    if ( g->state == gs_invisible )
return( false );

    GDrawPushClip(pixmap,&g->r,&old1);
    /* Background should be transpearant */
    /*GBoxDrawBackground(pixmap,&g->r,g->box, g->state,false);*/
    border = g->r;
    if ( g->prevlabel ) {
	int off = (g->prev->r.height-GBoxBorderWidth(g->base,g->box))/2;
	border.y += off; border.height -= off;
    }
    GBoxDrawBorder(pixmap,&border,g->box,g->state,false);

    GDrawPopClip(pixmap,&old1);
return( true );
}

static int gline_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GRect old1;

    if ( g->state == gs_invisible )
return( false );

    GDrawPushClip(pixmap,&g->r,&old1);
    if ( g->vert )
	GBoxDrawVLine(pixmap,&g->r,g->box);
    else
	GBoxDrawHLine(pixmap,&g->r,g->box);
    GDrawPopClip(pixmap,&old1);
return( true );
}

static void GGroupGetDesiredSize(GGadget *g, GRect *outer, GRect *inner) {
    if ( outer!=NULL ) {
	int bp = GBoxBorderWidth(g->base,g->box);
	outer->x = outer->y = 0;
	outer->width = outer->height = 2*bp+2;
	if ( g->desired_width>0 ) outer->width = g->desired_width;
	if ( g->desired_height>0 ) outer->height = g->desired_height;
    }
    if ( inner!=NULL ) {
	inner->x = inner->y = 0;
	inner->width = inner->height = 1;
    }
}

struct gfuncs gline_funcs = {
    0,
    sizeof(struct gfuncs),

    gline_expose,
    _ggadget_noop,
    _ggadget_noop,
    NULL,
    NULL,
    NULL,
    NULL,

    _ggadget_redraw,
    _ggadget_move,
    _ggadget_resize,
    _ggadget_setvisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    _ggadget_destroy,

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

    GGroupGetDesiredSize,
    _ggadget_setDesiredSize,
    NULL,
    NULL
};

struct gfuncs ggroup_funcs = {
    0,
    sizeof(struct gfuncs),

    ggroup_expose,
    _ggadget_noop,
    _ggadget_noop,
    NULL,
    NULL,
    NULL,
    NULL,

    _ggadget_redraw,
    _ggadget_move,
    _ggadget_resize,
    _ggadget_setvisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    _ggadget_destroy,

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

    GGroupGetDesiredSize,
    _ggadget_setDesiredSize,
    NULL,
    NULL
};

static void GLineFit(GGadget *g) {
    int bp = GBoxBorderWidth(g->base,g->box);

    if ( g->r.width==0 && !g->vert ) {
	GRect size;
	GDrawGetSize(g->base,&size);
	g->r.width = size.width - g->r.x - GDrawPointsToPixels(g->base,_GGadget_Skip);
    }
    if ( g->r.height==0 && !g->vert )
	g->r.height = bp;
    if ( g->r.width==0 && g->vert )
	g->r.width = bp;
    g->inner = g->r;
    g->inner.width = g->inner.height = 0;
}

static void GGroupFit(GGadget *g) {
    int bp = GBoxBorderWidth(g->base,g->box);

    if ( g->r.width==0 || g->r.height==0 )
	g->opengroup = true;
    g->inner = g->r;
    g->inner.x += bp;
    if ( g->prevlabel )
	g->inner.y += (g->prev->r.height-bp)/2 + bp;
    else
	g->inner.y += bp;
    if ( g->r.width != 0 )
	g->inner.width = g->r.width - 2*bp;
    if ( g->r.height != 0 )
	g->inner.height = g->r.y + g->r.height - bp - g->inner.y;
}

GGadget *GLineCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GGadget *g = calloc(1,sizeof(GLine));

    _GGroup_Init();
    g->funcs = &gline_funcs;
    _GGadget_Create(g,base,gd,data,&_GGroup_LineBox);
    if ( gd->flags & gg_line_vert )
	g->vert = true;

    GLineFit(g);
    _GGadget_FinalPosition(g,base,gd);
return( g );
}

GGadget *GGroupCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GGadget *g = calloc(1,sizeof(GGroup));

    _GGroup_Init();
    g->funcs = &ggroup_funcs;
    _GGadget_Create(g,base,gd,data,&group_box);

    if ( (gd->flags&gg_group_prevlabel) && g->prev!=NULL )
	g->prevlabel = true;
    if ( g->prevlabel && gd->pos.x==0 )
	g->r.x = g->prev->r.x - GDrawPointsToPixels(base,_GGadget_Skip);
    GGroupFit(g);
    _GGadget_FinalPosition(g,base,gd);
return( g );
}
