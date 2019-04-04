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
#include "gdraw.h"
#include "gresource.h"
#include "ggadgetP.h"
#include "ustring.h"

static GBox scrollbar_box = GBOX_EMPTY; /* Don't initialize here */
static GBox thumb_box = GBOX_EMPTY; /* Don't initialize here */
int _GScrollBar_Width = 13;			/* in points */
int _GScrollBar_StartTime=300, _GScrollBar_RepeatTime=200;
static int gscrollbar_inited = false;

static GGadget *GScrollBarCreateInitialized(struct gwindow *base, GGadgetData *gd,void *data);
static struct scrollbarinit sbinit = { 0, 40, 20, 10 };
static GGadgetCreateData scrollbar_gcd[] = {
    { GScrollBarCreateInitialized, { { 0, 0, 100, 13 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) &sbinit }, gg_visible, NULL, NULL }, NULL, NULL },
    { GScrollBarCreateInitialized, { { 0, 0, 100, 13 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) &sbinit }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *sarray[] = { GCD_Glue, &scrollbar_gcd[0], GCD_Glue, &scrollbar_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData scrollbarbox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) sarray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static GResInfo gthumb_ri;
static GResInfo gscrollbar_ri = {
    &gthumb_ri, &ggadget_ri,&gthumb_ri, NULL,
    &scrollbar_box,
    NULL,
    &scrollbarbox,
    NULL,
    N_("ScrollBar"),
    N_("Scroll Bar"),
    "GScrollBar",
    "Gdraw",
    false,
    box_foreground_border_outer|omf_border_type|omf_border_width|
	omf_padding|omf_main_background,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
static GResInfo gthumb_ri = {
    NULL, &ggadget_ri,&gscrollbar_ri, NULL,
    &thumb_box,
    NULL,
    &scrollbarbox,
    NULL,
    N_("SB Thumb"),
    N_("Scroll Bar Thumb"),
    "GScrollBarThumb",
    "Gdraw",
    true,
    omf_main_background|omf_border_width|omf_padding,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

static void GScrollBarChanged(GScrollBar *gsb, enum sb sbtype, int32 pos) {
    GEvent e;
    int active_len;

    if ( gsb->g.vert ) {
	active_len = gsb->g.inner.height;
    } else {
	active_len = gsb->g.inner.width;
    }
    if ( active_len<=0 )
	active_len = 1;

    e.type = et_controlevent;
    e.w = gsb->g.base;
    e.u.control.subtype = et_scrollbarchange;
    e.u.control.g = &gsb->g;
    e.u.control.u.sb.type = sbtype;
    e.u.control.u.sb.pos = (pos-gsb->thumboff)*(gsb->sb_max-gsb->sb_min)/active_len +
	    gsb->sb_min;
    if ( e.u.control.u.sb.pos > gsb->sb_max-gsb->sb_mustshow )
	e.u.control.u.sb.pos = gsb->sb_max-gsb->sb_mustshow;
    if ( e.u.control.u.sb.pos < gsb->sb_min )
	e.u.control.u.sb.pos = gsb->sb_min;

    if ( gsb->g.handle_controlevent!=NULL )
	(gsb->g.handle_controlevent)(&gsb->g,&e);
    else
	GDrawPostEvent(&e);
}

static void draw_thumb(GWindow pixmap, GScrollBar *gsb) {
    GRect thumbrect, thumbinner, old;
    int lw, skip, i;

    GDrawPushClip(pixmap,&gsb->g.inner,&old);
    thumbrect = gsb->g.inner;
    if ( gsb->g.vert ) {
	thumbrect.y = gsb->g.inner.y+gsb->thumbpos;
	thumbrect.height = gsb->thumbsize;
    } else {
	thumbrect.x = gsb->g.inner.x+gsb->thumbpos;
	thumbrect.width = gsb->thumbsize;
    }
    thumbinner.x = thumbrect.x+gsb->thumbborder;
    thumbinner.y = thumbrect.y+gsb->thumbborder;
    thumbinner.width = thumbrect.width-2*gsb->thumbborder;
    thumbinner.height = thumbrect.height-2*gsb->thumbborder;

    GBoxDrawBackground(pixmap,&thumbrect,gsb->thumbbox, gsb->g.state,false);
    GBoxDrawBorder(pixmap,&thumbrect,gsb->thumbbox,gsb->g.state,false);

    lw = GDrawPointsToPixels(gsb->g.base,1);
    skip = GDrawPointsToPixels(gsb->g.base,3);
    GDrawSetLineWidth(pixmap,lw);
    if ( gsb->g.vert ) {
	for ( i = thumbinner.y + skip; i<thumbinner.y+thumbinner.height-skip;
		i += skip+2*lw ) {
	    GDrawDrawLine(pixmap,thumbinner.x+lw, i, thumbinner.x+thumbinner.width-2*lw, i,
		    gsb->thumbbox->border_brightest );
	    GDrawDrawLine(pixmap,thumbinner.x+lw, i+lw, thumbinner.x+thumbinner.width-2*lw, i+lw,
		    gsb->thumbbox->border_darkest );
	}
    } else {
	for ( i = thumbinner.x + skip; i<thumbinner.x+thumbinner.width-skip;
		i += skip+2*lw ) {
	    GDrawDrawLine(pixmap,i, thumbinner.y+lw, i, thumbinner.y+thumbinner.height-2*lw,
		    gsb->thumbbox->border_brightest );
	    GDrawDrawLine(pixmap,i+lw, thumbinner.y+lw, i+lw, thumbinner.y+thumbinner.height-2*lw, 
		    gsb->thumbbox->border_darkest );
	}
    }
    GDrawPopClip(pixmap,&old);
}

static void draw_arrow(GWindow pixmap, GScrollBar *gsb, int which) {
    GPoint pts[5];
    int point = GDrawPointsToPixels(gsb->g.base,1);
    int cnt = 4;
    Color fill = gsb->thumbbox->main_foreground;

    if ( fill == COLOR_DEFAULT )
	fill = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap));

    switch ( which ) {
      case 0:		/* Horizontal left arrow */
	pts[0].y = (gsb->g.r.y+(gsb->g.r.height-1)/2);
		pts[0].x = gsb->g.r.x + 2*point;
	pts[1].y = gsb->g.r.y + point;
		pts[1].x = pts[0].x + (gsb->g.r.height-1)/2-point ;
	pts[2].y = gsb->g.r.y+gsb->g.r.height-1 - point;
		pts[2].x = pts[1].x;
	pts[3] = pts[0];
	if ( !(gsb->g.inner.height&1 )) {
	    ++pts[3].y;
	    pts[4] = pts[0];
	    cnt = 5;
	}
	GDrawFillPoly(pixmap,pts,cnt,fill);
	GDrawDrawLine(pixmap,pts[0].x,pts[0].y,pts[1].x,pts[1].y,
		gsb->thumbbox->border_brightest);
	GDrawDrawLine(pixmap,pts[2].x,pts[2].y,pts[3].x,pts[3].y,
		gsb->thumbbox->border_darker);
	GDrawDrawLine(pixmap,pts[1].x,pts[1].y,pts[2].x,pts[2].y,
		gsb->thumbbox->border_darkest);
      break;
      case 1:		/* Vertical up arrow */
	pts[0].x = (gsb->g.r.x+(gsb->g.r.width-1)/2);
		pts[0].y = gsb->g.r.y + 2*point;
	pts[1].x = gsb->g.r.x + point;
		pts[1].y = pts[0].y + (gsb->g.r.width-1)/2-point ;
	pts[2].x = gsb->g.r.x+gsb->g.r.width-1 - point;
		pts[2].y = pts[1].y;
	pts[3] = pts[0];
	if ( !(gsb->g.inner.width&1 )) {
	    ++pts[3].x;
	    pts[4] = pts[0];
	    cnt = 5;
	}
	GDrawFillPoly(pixmap,pts,cnt,fill);
	GDrawDrawLine(pixmap,pts[0].x,pts[0].y,pts[1].x,pts[1].y,
		gsb->thumbbox->border_brightest);
	GDrawDrawLine(pixmap,pts[2].x,pts[2].y,pts[3].x,pts[3].y,
		gsb->thumbbox->border_darker);
	GDrawDrawLine(pixmap,pts[1].x,pts[1].y,pts[2].x,pts[2].y,
		gsb->thumbbox->border_darkest);
      break;
      case 2:		/* Horizontal right arrow */
	pts[0].y = (gsb->g.r.y+(gsb->g.r.height-1)/2);
		pts[0].x = gsb->g.r.x + gsb->g.r.width-1 - 2*point;
	pts[1].y = gsb->g.r.y + point;
		pts[1].x = pts[0].x - ((gsb->g.r.height-1)/2-point);
	pts[2].y = gsb->g.r.y+gsb->g.r.height-1 - point;
		pts[2].x = pts[1].x;
	pts[3] = pts[0];
	if ( !(gsb->g.inner.height&1 )) {
	    ++pts[3].y;
	    pts[4] = pts[0];
	    cnt = 5;
	}
	GDrawFillPoly(pixmap,pts,cnt,fill);
	GDrawDrawLine(pixmap,pts[0].x,pts[0].y,pts[1].x,pts[1].y,
		gsb->thumbbox->border_darkest);
	GDrawDrawLine(pixmap,pts[2].x,pts[2].y,pts[3].x,pts[3].y,
		gsb->thumbbox->border_darker);
	GDrawDrawLine(pixmap,pts[1].x,pts[1].y,pts[2].x,pts[2].y,
		gsb->thumbbox->border_brightest);
      break;
      case 3:		/* Vertical down arrow */
	pts[0].x = (gsb->g.r.x+(gsb->g.r.width-1)/2);
		pts[0].y = gsb->g.r.y + gsb->g.r.height-1 - 2*point;
	pts[1].x = gsb->g.r.x + point;
		pts[1].y = pts[0].y - ((gsb->g.r.width-1)/2-point);
	pts[2].x = gsb->g.r.x+gsb->g.r.width-1 - point;
		pts[2].y = pts[1].y;
	pts[3] = pts[0];
	if ( !(gsb->g.inner.width&1 )) {
	    ++pts[3].x;
	    pts[4] = pts[0];
	    cnt = 5;
	}
	GDrawFillPoly(pixmap,pts,cnt,fill);
	GDrawDrawLine(pixmap,pts[0].x,pts[0].y,pts[1].x,pts[1].y,
		gsb->thumbbox->border_darkest);
	GDrawDrawLine(pixmap,pts[2].x,pts[2].y,pts[3].x,pts[3].y,
		gsb->thumbbox->border_darker);
	GDrawDrawLine(pixmap,pts[1].x,pts[1].y,pts[2].x,pts[2].y,
		gsb->thumbbox->border_brightest);
      break;
    }
}

static void GScrollBarFit(GScrollBar *gsb);

static int gscrollbar_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GScrollBar *gsb = (GScrollBar *) g;
    GBox box = *(g->box);
    GRect old1;
    GRect r;
    int ar;

    if ( g->state == gs_invisible )
return( false );

    /* In case border was changed in resource editor, */
    /* the scrollbar thumb inside must be refitted. */
    GScrollBarFit(gsb);

    GDrawPushClip(pixmap,&g->r,&old1);

    r = g->r;
    ar = gsb->arrowsize - gsb->sbborder;
    if ( gsb->g.vert ) { r.y += ar ; r.height -= 2*ar; }
    else { r.x += ar; r.width -= 2*ar; }

    /* Mimick old border behavior to retain compatibility with older themes, */
    /* but match border shape with that of background. */
    box.flags = box_foreground_border_outer;
    box.border_width = 0;
    GBoxDrawBackground(pixmap,&g->r,g->box,g->state,false);
    GBoxDrawBackground(pixmap,&r,g->box,gs_pressedactive,false);
    GBoxDrawBorder(pixmap,&g->r,&box,g->state,false);
    GBoxDrawBorder(pixmap,&r,g->box,g->state,false);

    draw_thumb(pixmap,gsb); /* sets line width for arrows too */
    draw_arrow(pixmap,gsb,gsb->g.vert);
    draw_arrow(pixmap,gsb,gsb->g.vert|2);

    GDrawPopClip(pixmap,&old1);
return( true );
}

static int gscrollbar_mouse(GGadget *g, GEvent *event) {
    GScrollBar *gsb = (GScrollBar *) g;
    int active_pos, active_len;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return( false );
    if ( event->type == et_crossing )
return( false );

    if ( gsb->g.vert ) {
	active_pos = event->u.mouse.y-g->inner.y;
	active_len = g->inner.height;
    } else {
	active_pos = event->u.mouse.x-g->inner.x;
	active_len = g->inner.width;
    }

    if ( (event->type==et_mouseup || event->type==et_mousedown) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7) ) {
	/* X treats scroll wheels as though they send events from buttons 4 and 5 */
	/* Badly configured wacom mice send "p5 r5 p4 r4" or "p4 r4 p5 r5" */
	/*  properly configured mice just send "p4 r4" or "p5 r5" */
	/* And apple's mouse with a scrollwheel sends buttons 6&7 for horizontal*/
	/*  scrolling */
	/* Convention is that shift-vertical scroll=horizontal scroll */
	/*                    control-vertical scroll=minimize/maximize */
	if ( event->type==et_mousedown ) {
	    GDrawCancelTimer(gsb->pressed); gsb->pressed = NULL;
	    int isv = event->u.mouse.button<=5;
	    if ( event->u.mouse.state&ksm_shift ) isv = !isv;
	    if ( !isv && g->vert )
return( false );	/* Allow horizontal scrolling with normal scroll but not vice versa */
	    else if ( event->u.mouse.state&ksm_control )
return( false );
	    if ( event->u.mouse.button==5 || event->u.mouse.button==7 ) {
		GScrollBarChanged(gsb,et_sb_down,0);
	    } else if ( event->u.mouse.button==4 || event->u.mouse.button==6 ) {
		GScrollBarChanged(gsb,et_sb_up,0);
	    }
	}
return( true );
    }

    if ( event->type == et_mousedown && GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	GDrawCancelTimer(gsb->pressed); gsb->pressed = NULL;
	if ( event->u.mouse.button!=1 ) {
	    gsb->thumbpressed = true;
	    gsb->thumboff = 0;
	    active_pos = event->u.mouse.y-g->inner.y;
	    GScrollBarChanged(gsb,et_sb_thumb,active_pos);
	} else if ( active_pos >= gsb->thumbpos &&
		active_pos < gsb->thumbpos+gsb->thumbsize ) {
	    gsb->thumbpressed = true;
	    gsb->thumboff = active_pos-gsb->thumbpos;
	} else if ( active_pos < gsb->thumbpos &&
		(event->u.mouse.state&(ksm_control|ksm_meta)) ) {
	    gsb->thumbpressed = true;
	    gsb->thumboff = active_pos;
	    GScrollBarChanged(gsb,et_sb_top,0);
	} else if ( active_pos >= gsb->thumbpos+gsb->thumbsize &&
		(event->u.mouse.state&(ksm_control|ksm_meta)) ) {
	    gsb->thumbpressed = true;
	    gsb->thumboff = active_pos-active_len+gsb->thumbsize;
	    GScrollBarChanged(gsb,et_sb_bottom,0);
	} else {
	    if ( active_pos<0 )
		gsb->repeatcmd = et_sb_up;
	    else if ( active_pos >= active_len )
		gsb->repeatcmd = et_sb_down;
	    else if ( active_pos < gsb->thumbpos )
		gsb->repeatcmd = et_sb_uppage;
	    else /* if ( active_pos > gsb->thumbpos+gsb->thumbsize )*/
		gsb->repeatcmd = et_sb_downpage;
	    GScrollBarChanged(gsb,gsb->repeatcmd,0);
	    gsb->pressed = GDrawRequestTimer(g->base,_GScrollBar_StartTime,_GScrollBar_RepeatTime,NULL);
	}
    } else if ( event->type == et_mousemove && gsb->thumbpressed ) {
	GDrawSkipMouseMoveEvents(gsb->g.base,event);
	if ( gsb->g.vert ) {
	    active_pos = event->u.mouse.y-g->inner.y;
	} else {
	    active_pos = event->u.mouse.x-g->inner.x;
	}
	GScrollBarChanged(gsb,et_sb_thumb,active_pos);
    } else if ( event->type == et_mouseup && (gsb->thumbpressed || gsb->pressed)) {
	if ( gsb->thumbpressed )
	    GScrollBarChanged(gsb,et_sb_thumbrelease,active_pos);
	GDrawCancelTimer(gsb->pressed); gsb->pressed = NULL;
	gsb->thumbpressed = false;
    } else if ( event->type == et_mousemove && !gsb->pressed &&
	    g->popup_msg!=NULL && GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y)) {
	GGadgetPreparePopup(g->base,g->popup_msg);
return( true );
    } else
return( false );

return( true );
}

static int gscrollbar_timer(GGadget *g, GEvent *event) {
    GScrollBar *gsb = (GScrollBar *) g;

    if ( event->u.timer.timer == gsb->pressed ) {
	GScrollBarChanged(gsb,gsb->repeatcmd,0);
return( true );
    }
return( false );
}

static void gscrollbar_destroy(GGadget *g) {
    GScrollBar *gsb = (GScrollBar *) g;

    if ( gsb==NULL )
return;
    GDrawCancelTimer(gsb->pressed);
    _ggadget_destroy(g);
}

static void gscrollbar_get_desired_size(GGadget *g, GRect *outer, GRect *inner) {
    int bp = GBoxBorderWidth(g->base,g->box);
    GScrollBar *gsb = (GScrollBar *) g;
    int width, height;
    int minheight, sbw;

    sbw = GDrawPointsToPixels(gsb->g.base,_GScrollBar_Width);
    minheight = 2*(gsb->thumbborder+gsb->arrowsize) + GDrawPointsToPixels(gsb->g.base,2);
    if ( g->vert ) {
	width = sbw;
	height = minheight;
    } else {
	width = minheight;
	height = sbw;
    }

    if ( inner!=NULL ) {
	inner->x = inner->y = 0;
	inner->width = width; inner->height = height;
    }
    if ( outer!=NULL ) {
	outer->x = outer->y = 0;
	outer->width = width+2*bp; outer->height = height+2*bp;
    }
}

struct gfuncs gscrollbar_funcs = {
    0,
    sizeof(struct gfuncs),

    gscrollbar_expose,
    gscrollbar_mouse,
    NULL,
    NULL,
    NULL,
    gscrollbar_timer,
    NULL,

    _ggadget_redraw,
    _ggadget_move,
    _ggadget_resize,
    _ggadget_setvisible,
    _ggadget_setenabled,
    _ggadget_getsize,
    _ggadget_getinnersize,

    gscrollbar_destroy,

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

    gscrollbar_get_desired_size,
    NULL,
    NULL,
    NULL
};

static void GScrollBarInit() {
    _GGadgetCopyDefaultBox(&scrollbar_box);
    _GGadgetCopyDefaultBox(&thumb_box);
    scrollbar_box.border_type = bt_lowered;
    scrollbar_box.border_width = 1;
    scrollbar_box.padding = 0;
    scrollbar_box.flags |= box_foreground_border_outer;
    scrollbar_box.main_background = GDrawColorBrighten(scrollbar_box.main_background, 0x10);
    thumb_box.main_background = GDrawColorDarken(thumb_box.main_background,0x8);
    thumb_box.border_width = 1;
    thumb_box.padding = 0;
    _GGadgetInitDefaultBox("GScrollBar.",&scrollbar_box,NULL);
    _GGadgetInitDefaultBox("GScrollBarThumb.",&thumb_box,NULL);
    _GScrollBar_Width = GResourceFindInt("GScrollBar.Width",_GScrollBar_Width);
    _GScrollBar_StartTime = GResourceFindInt("GScrollBar.StartupTime",_GScrollBar_StartTime);
    _GScrollBar_RepeatTime = GResourceFindInt("GScrollBar.RepeatTime",_GScrollBar_RepeatTime);
    gscrollbar_inited = true;
}

static void GScrollBarFit(GScrollBar *gsb) {
    int minheight;

    gsb->sbborder = GBoxBorderWidth(gsb->g.base,gsb->g.box);
    gsb->thumbborder = GBoxBorderWidth(gsb->g.base,gsb->thumbbox);
    /* FIXME: workaround for incorrect calculation. */
    if ( gsb->thumbborder > 5 ) gsb->thumbborder = 5;
    gsb->arrowsize = gsb->sbborder +
	    2*GDrawPointsToPixels(gsb->g.base,2) +
	    GDrawPointsToPixels(gsb->g.base,_GScrollBar_Width)/2-
	    2*GDrawPointsToPixels(gsb->g.base,1);
    minheight = 2*(gsb->thumbborder+gsb->arrowsize) + GDrawPointsToPixels(gsb->g.base,2);

    if ( gsb->g.vert ) {
	if ( gsb->g.r.width==0 )
	    gsb->g.r.width = GDrawPointsToPixels(gsb->g.base,_GScrollBar_Width);
	if ( gsb->g.r.height< minheight )
	    gsb->g.r.height = minheight;
	gsb->g.inner.x = gsb->g.r.x+gsb->sbborder;
	gsb->g.inner.width = gsb->g.r.width - 2*gsb->sbborder;
	gsb->g.inner.y = gsb->g.r.y+gsb->arrowsize;
	gsb->g.inner.height = gsb->g.r.height - 2*gsb->arrowsize;
    } else {
	if ( gsb->g.r.height==0 )
	    gsb->g.r.height = GDrawPointsToPixels(gsb->g.base,_GScrollBar_Width);
	if ( gsb->g.r.width< minheight )
	    gsb->g.r.width = minheight;
	gsb->g.inner.x = gsb->g.r.x+gsb->arrowsize;
	gsb->g.inner.width = gsb->g.r.width - 2*gsb->arrowsize;
	gsb->g.inner.y = gsb->g.r.y+gsb->sbborder;
	gsb->g.inner.height = gsb->g.r.height - 2*gsb->sbborder;
    }
}

static GScrollBar *_GScrollBarCreate(GScrollBar *gsb, struct gwindow *base, GGadgetData *gd,void *data, GBox *def) {

    if ( !gscrollbar_inited )
	GScrollBarInit();
    gsb->g.funcs = &gscrollbar_funcs;
    gd->flags |= gg_pos_use0;
    _GGadget_Create(&gsb->g,base,gd,data,def);

    gsb->g.takes_input = true;
    if ( gd->flags & gg_sb_vert )
	gsb->g.vert = true;
    gsb->thumbbox = &thumb_box;

    GScrollBarFit(gsb);
    if ( gd->u.sbinit!=NULL )
	GScrollBarSetMustShow(&gsb->g,
		gd->u.sbinit->sb_min,
		gd->u.sbinit->sb_max,
		gd->u.sbinit->sb_pagesize,
		gd->u.sbinit->sb_pos);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&gsb->g);
return( gsb );
}

static GGadget *GScrollBarCreateInitialized(struct gwindow *base, GGadgetData *gd,void *data) {
    GScrollBar *gsb = _GScrollBarCreate(calloc(1,sizeof(GScrollBar)),base,gd,data,&scrollbar_box);

return( &gsb->g );
}

GGadget *GScrollBarCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GScrollBar *gsb;
    struct scrollbarinit *hold = gd->u.sbinit;

    gd->u.sbinit = NULL;
    gsb = _GScrollBarCreate(calloc(1,sizeof(GScrollBar)),base,gd,data,&scrollbar_box);
    gd->u.sbinit = hold;

return( &gsb->g );
}

int32 GScrollBarGetPos(GGadget *g) {
return( ((GScrollBar *) g)->sb_pos );
}

int32 GScrollBarAddToPos(GGadget *g,int32 pos) {
    return GScrollBarSetPos( g, GScrollBarGetPos(g) + pos );
}


int32 GScrollBarSetPos(GGadget *g,int32 pos) {
    GScrollBar *gsb = (GScrollBar *) g;

    if ( pos>gsb->sb_max-gsb->sb_mustshow )
	pos = gsb->sb_max-gsb->sb_mustshow;
    if ( pos<gsb->sb_min )
	pos = gsb->sb_min;
    gsb->sb_pos = pos;

    if ( pos==gsb->sb_min || gsb->sb_min==gsb->sb_max )
	gsb->thumbpos = 0;
    else
	gsb->thumbpos =
	    ((gsb->g.vert?gsb->g.inner.height:gsb->g.inner.width)-gsb->size_offset)
	      *(pos-gsb->sb_min)/(gsb->sb_max-gsb->sb_min);
    _ggadget_redraw(g);
return( pos );
}

void GScrollBarSetMustShow(GGadget *g, int32 sb_min, int32 sb_max, int32 sb_pagesize,
	int32 sb_mustshow ) {
    GScrollBar *gsb = (GScrollBar *) g;

    if ( sb_min>sb_max || sb_pagesize<=0 ) {
	GDrawIError("Invalid scrollbar bounds min=%d max=%d, pagesize=%d",
		sb_min, sb_max, sb_pagesize );
return;
    }
    gsb->sb_min = sb_min;
    gsb->sb_max = sb_max;
    gsb->sb_pagesize = sb_pagesize;
    gsb->sb_mustshow = sb_mustshow;
    gsb->size_offset = 0;
    gsb->thumbsize = (gsb->g.vert?gsb->g.inner.height:gsb->g.inner.width);
    if ( sb_max-sb_min > sb_pagesize )
	gsb->thumbsize = (gsb->thumbsize*gsb->sb_pagesize)/(sb_max-sb_min);
    if ( gsb->thumbsize<2*gsb->thumbborder+10 ) {
        gsb->size_offset = 2*gsb->thumbborder+10 - gsb->thumbsize;
	gsb->thumbsize = 2*gsb->thumbborder+10;
	if ( gsb->thumbsize>(gsb->g.vert?gsb->g.inner.height:gsb->g.inner.width) ) {
	    gsb->size_offset = 0;
	    gsb->thumbsize = (gsb->g.vert?gsb->g.inner.height:gsb->g.inner.width);
        }
    }
    GScrollBarSetPos(g,gsb->sb_pos);
}

void GScrollBarSetBounds(GGadget *g, int32 sb_min, int32 sb_max, int32 sb_pagesize ) {
    GScrollBarSetMustShow(g,sb_min,sb_max,sb_pagesize,sb_pagesize);
}

void GScrollBarGetBounds(GGadget *g, int32 *sb_min, int32 *sb_max, int32 *sb_pagesize ) {
    GScrollBar *gsb = (GScrollBar *) g;
    *sb_min = gsb->sb_min;
    *sb_max = gsb->sb_max;
    *sb_pagesize = gsb->sb_pagesize;
}

GResInfo *_GScrollBarRIHead(void) {
    if ( !gscrollbar_inited )
	GScrollBarInit();
return( &gscrollbar_ri );
}
