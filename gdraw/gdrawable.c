/* Copyright (C) 2007-2012 by George Williams */
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
#include "gkeysym.h"
#include "gresource.h"
#include "gwidget.h"

static GBox gdrawable_box = GBOX_EMPTY; /* Don't initialize here */
static FontInstance *gdrawable_font = NULL;
static int gdrawable_inited = false;

static GResInfo gdrawable_ri = {
    NULL, &ggadget_ri, NULL,NULL,
    &gdrawable_box,
    NULL,
    NULL,
    NULL,
    N_("Drawing Area"),
    N_("A canvas (sub-window) wrapped up in a gadget, for drawing"),
    "GDrawable",
    "Gdraw",
    false,
    omf_border_width|omf_padding|omf_border_type,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

static void GDrawableInit() {

    GGadgetInit();
    _GGadgetCopyDefaultBox(&gdrawable_box);
    gdrawable_box.border_width = gdrawable_box.padding = 0;
    gdrawable_box.border_type = bt_none;
    gdrawable_font = _GGadgetInitDefaultBox("GDrawable.",&gdrawable_box,NULL);
    gdrawable_inited = true;
}

static int gdrawable_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GRect old;

    if ( g->state == gs_invisible )
return( false );

    GDrawPushClip(pixmap,&g->r,&old);
    GBoxDrawBorder(pixmap,&g->r,g->box,g->state,false);

    GDrawPopClip(pixmap,&old);
return( true );
}

static int gdrawable_mouse(GGadget *g, GEvent *event) {
return( event->type==et_mousedown || event->type==et_mouseup );
}

static int gdrawable_key(GGadget *g, GEvent *event) {
return(false);
}

static void gdrawable_redraw(GGadget *g ) {
    GDrawable *gd = (GDrawable *) g;
    if ( gd->gw!=NULL )
	GDrawRequestExpose(gd->gw,NULL,true);
    _ggadget_redraw(g);
}

static void gdrawable_move(GGadget *g, int32 x, int32 y) {
    GDrawable *gd = (GDrawable *) g;

    if ( gd->gw!=NULL )
	GDrawMove(gd->gw,x+(g->inner.x-g->r.x),y+(g->inner.y-g->r.y));
    _ggadget_move(g,x,y);
}

static void gdrawable_resize(GGadget *g, int32 width, int32 height ) {
    GDrawable *gd = (GDrawable *) g;
    int bp = GBoxBorderWidth(g->base,g->box);

    if ( gd->gw!=NULL )
	GDrawResize(gd->gw,width - 2*bp, height - 2*bp);
    _ggadget_resize(g,width, height);
}

static void gdrawable_setvisible(GGadget *g, int visible ) {
    GDrawable *gd = (GDrawable *) g;

    if ( gd->gw!=NULL )
	GDrawSetVisible(gd->gw,visible);
    _ggadget_setvisible(g,visible);
}

static void gdrawable_destroy(GGadget *g) {
    GDrawable *gd = (GDrawable *) g;

    if ( gd->gw!=NULL ) {
	GDrawSetUserData(gd->gw,NULL);
	GDrawDestroyWindow(gd->gw);
    }

    _ggadget_destroy(g);
}

static void GDrawableGetDesiredSize(GGadget *g, GRect *outer, GRect *inner) {
    int bp = GBoxBorderWidth(g->base,g->box);

    if ( outer!=NULL ) {
	*outer = g->r;
	outer->width = g->desired_width; outer->height = g->desired_height;
    }
    if ( inner!=NULL ) {
	*inner = g->inner;
	inner->width = g->desired_width-2*bp; inner->height = g->desired_height-2*bp;
    }
}

static int gdrawable_FillsWindow(GGadget *g) {
return( g->prev==NULL && _GWidgetGetGadgets(g->base)==g );
}

struct gfuncs gdrawable_funcs = {
    0,
    sizeof(struct gfuncs),

    gdrawable_expose,
    gdrawable_mouse,
    gdrawable_key,
    NULL,
    NULL,
    NULL,
    NULL,

    gdrawable_redraw,
    gdrawable_move,
    gdrawable_resize,
    gdrawable_setvisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    gdrawable_destroy,

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

    GDrawableGetDesiredSize,
    _ggadget_setDesiredSize,
    gdrawable_FillsWindow,
    NULL
};

static int drawable_e_h(GWindow pixmap, GEvent *event) {
    GWindow gw = event->type==et_expose ? event->w : pixmap;
    GGadget *g = _GWidgetGetGadgets(GDrawGetParentWindow(gw));
    GDrawable *gdr = NULL;

    /* Figure out our ggadget */
    while ( g!=NULL ) {
	if ( g->funcs == &gdrawable_funcs && ((GDrawable *) g)->gw==gw ) {
	    gdr = (GDrawable *) g;
    break;
	}
	g = g->prev;
    }
    if ( gdr==NULL )
return( false );
    if ( event->type == et_destroy ) {
	gdr->gw = NULL;
    }
    if ( gdr->e_h!=NULL )
return( (gdr->e_h)(pixmap,event));

return( false );
}

GGadget *GDrawableCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GDrawable *gdr = calloc(1,sizeof(GTabSet));
    int bp;
    GRect r;
    GWindowAttrs childattrs;

    if ( !gdrawable_inited )
	GDrawableInit();
    gdr->g.funcs = &gdrawable_funcs;
    _GGadget_Create(&gdr->g,base,gd,data,&gdrawable_box);

    gdr->g.takes_input = false; gdr->g.takes_keyboard = false; gdr->g.focusable = false;

    GDrawGetSize(base,&r);
    if ( gd->pos.x <= 0 )
	gdr->g.r.x = GDrawPointsToPixels(base,2);
    if ( gd->pos.y <= 0 )
	gdr->g.r.y = GDrawPointsToPixels(base,2);
    if ( gd->pos.width<=0 )
	gdr->g.r.width = r.width - gdr->g.r.x - GDrawPointsToPixels(base,2);
    if ( gd->pos.height<=0 ) {
	gdr->g.r.height = r.height - gdr->g.r.y - GDrawPointsToPixels(base,26);
    }

    bp = GBoxBorderWidth(base,gdr->g.box);
    gdr->g.inner = gdr->g.r;
    gdr->g.inner.x += bp; gdr->g.inner.width -= 2*bp;
    gdr->g.inner.y += bp; gdr->g.inner.height -= 2*bp;

    gdr->g.desired_width = gdr->g.r.width;
    gdr->g.desired_height = gdr->g.r.height;
    gdr->e_h = gd->u.drawable_e_h;

    memset(&childattrs,0,sizeof(childattrs));
    childattrs.mask = wam_events|wam_backcol;
    childattrs.event_masks = -1;
    childattrs.background_color = gdr->g.box->main_background;

    if ( !(gd->flags&gg_tabset_nowindow) ) {
	gdr->gw = GWidgetCreateSubWindow(base,&gdr->g.inner,drawable_e_h,GDrawGetUserData(base),&childattrs);
	if ( gd->flags&gg_visible )
	    GDrawSetVisible(gdr->gw,true);
    }

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&gdr->g);

return( &gdr->g );
}

GWindow GDrawableGetWindow(GGadget *g) {
    GDrawable *gd = (GDrawable *) g;
return( gd->gw );
}

GResInfo *_GDrawableRIHead(void) {

    if ( !gdrawable_inited )
	GDrawableInit();
return( &gdrawable_ri );
}
