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
#include "gwidget.h"
#include "ustring.h"
#include "gkeysym.h"

static GBox gtabset_box = GBOX_EMPTY; /* Don't initialize here */
static GBox gvtabset_box = GBOX_EMPTY; /* Don't initialize here */
static FontInstance *gtabset_font = NULL;
static int gtabset_inited = false;

static GResInfo gtabset_ri, gvtabset_ri;

static GResInfo gtabset_ri = {
    &gvtabset_ri, &ggadget_ri, &gvtabset_ri, NULL,
    &gtabset_box,
    NULL,
    NULL,
    NULL,
    N_("TabSet"),
    N_("Tab Set"),
    "GTabSet",
    "Gdraw",
    false,
    omf_border_width|omf_border_shape,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

/* Dummy gadget for styling vertical layout */
static GResInfo gvtabset_ri = {
    NULL, &gtabset_ri, &gtabset_ri, NULL,
    &gvtabset_box,
    NULL,
    NULL,
    NULL,
    N_("VerticalTabSet"),
    N_("Vertical Tab Set"),
    "GVTabSet",
    "Gdraw",
    false,
    0,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};
#define NEST_INDENT	4

static void GTabSetInit() {

    if ( gtabset_inited )
return;

    GGadgetInit();

    _GGadgetCopyDefaultBox(&gtabset_box);
    gtabset_box.border_width = 1; gtabset_box.border_shape = bs_rect;
    /*gtabset_box.flags = 0;*/
    gtabset_font = _GGadgetInitDefaultBox("GTabSet.",&gtabset_box,NULL);

    gvtabset_box = gtabset_box; /* needs this to figure inheritance */
    _GGadgetInitDefaultBox("GVTabSet.",&gvtabset_box,NULL);

    gtabset_inited = true;
}

static void GTabSetChanged(GTabSet *gts,int oldsel) {
    GEvent e;

    e.type = et_controlevent;
    e.w = gts->g.base;
    e.u.control.subtype = et_radiochanged;
    e.u.control.g = &gts->g;
    if ( gts->g.handle_controlevent != NULL )
	(gts->g.handle_controlevent)(&gts->g,&e);
    else
	GDrawPostEvent(&e);
}

static int DrawLeftArrowTab(GWindow pixmap, GTabSet *gts, int x, int y ) {
    Color fg = gts->g.box->main_foreground;
    GPoint pts[5];
    int retx = x + gts->arrow_width, cnt;

    if ( fg==COLOR_DEFAULT ) fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap));
    GBoxDrawTabOutline(pixmap,&gts->g,x,y,gts->arrow_width,gts->rowh,false);
    gts->haslarrow = true;
	y += (gts->rowh-gts->arrow_size)/2;
	x += (gts->arrow_width-gts->arrow_size/2)/2;
	cnt = 4;
	pts[0].y = (y+(gts->arrow_size-1)/2); 	pts[0].x = x;
	pts[1].y = y; 				pts[1].x = x + (gts->arrow_size-1)/2;
	pts[2].y = y+gts->arrow_size-1; 	pts[2].x = pts[1].x;
	pts[3] = pts[0];
	if ( !(gts->arrow_size&1 )) {
	    ++pts[3].y;
	    pts[4] = pts[0];
	    cnt = 5;
	}
	GDrawFillPoly(pixmap,pts,cnt,fg);
return( retx );
}

static int DrawRightArrowTab(GWindow pixmap, GTabSet *gts, int x, int y ) {
    Color fg = gts->g.box->main_foreground;
    GPoint pts[5];
    int retx = x + gts->arrow_width, cnt;

    if ( fg==COLOR_DEFAULT ) fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap));
    GBoxDrawTabOutline(pixmap,&gts->g,x,y,gts->arrow_width,gts->rowh,false);
    gts->hasrarrow = true;
	y += (gts->rowh-gts->arrow_size)/2;
	x += (gts->arrow_width-gts->arrow_size/2)/2;
	cnt = 4;
	pts[0].y = (y+(gts->arrow_size-1)/2); 	pts[0].x = x + (gts->arrow_size-1)/2;
	pts[1].y = y; 				pts[1].x = x;
	pts[2].y = y+gts->arrow_size-1; 	pts[2].x = pts[1].x;
	pts[3] = pts[0];
	if ( !(gts->arrow_size&1 )) {
	    ++pts[3].y;
	    pts[4] = pts[0];
	    cnt = 5;
	}
	GDrawFillPoly(pixmap,pts,cnt,fg);
return( retx );
}

static int DrawTab(GWindow pixmap, GTabSet *gts, int i, int x, int y ) {
    Color fg = gts->tabs[i].disabled?gts->g.box->disabled_foreground:gts->g.box->main_foreground;

    if ( fg==COLOR_DEFAULT ) fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap));
    GBoxDrawTabOutline(pixmap,&gts->g,x,y,gts->tabs[i].width,gts->rowh,i==gts->sel);

    if ( (i==gts->sel) && (gts->g.box->flags&box_active_border_inner) ) {
        GRect r;
        r.x = x+2;
        r.y = y+1;
        r.width = gts->tabs[i].width-4;
        r.height = gts->rowh-2;
        GDrawFillRect(pixmap,&r,gts->g.box->active_border);
    }

    GDrawDrawText(pixmap,x+(gts->tabs[i].width-gts->tabs[i].tw)/2,y+gts->rowh-gts->ds,
	    gts->tabs[i].name,-1,fg);
    gts->tabs[i].x = x;
    x += gts->tabs[i].width;
return( x );
}

static int gtabset_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GTabSet *gts = (GTabSet *) g;
    int x,y,i,rd, dsel;
    GRect old1, bounds;
    int bw = GBoxBorderWidth(pixmap,g->box);
    int yoff = ( gts->rcnt==1 ? bw : 0 );
    Color fg;
    int ni = GDrawPointsToPixels(pixmap,NEST_INDENT);

    if ( g->state == gs_invisible )
return( false );

    GDrawPushClip(pixmap,&g->r,&old1);

    GBoxDrawBackground(pixmap,&g->r,g->box,g->state,false);
    bounds = g->r;

    if ( !gts->vertical ) {
	/* make room for tabs */
	bounds.y += gts->rcnt*gts->rowh+yoff-1;
	bounds.height -= gts->rcnt*gts->rowh+yoff-1;
	/* draw border around horizontal tabs only */
	GBoxDrawBorder(pixmap,&bounds,g->box,g->state,false);
    }
    else if ( g->state==gs_enabled ) {
	/* background for labels */
	GRect old2, vertListRect = g->r;
	vertListRect.width = gts->vert_list_width+bw-1;
	GDrawPushClip(pixmap,&vertListRect,&old2);
	GBoxDrawBackground(pixmap,&g->r,g->box,gs_pressedactive,false);
	GDrawPopClip(pixmap,&old2);
    }

    GDrawSetFont(pixmap,gts->font);

    if ( gts->vertical ) {
	x = g->r.x + bw + 3;
	y = g->r.y + bw + 3;
	for ( i=gts->offtop; i<gts->tabcnt; ++i ) {
	    fg = gts->tabs[i].disabled?gts->g.box->disabled_foreground:gts->g.box->main_foreground;
	    if ( fg==COLOR_DEFAULT ) fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap));
	    if ( i==gts->sel ) {
		GRect r;
		r.x = x; r.y = y;
		r.width = gts->vert_list_width-10; r.height = gts->fh;
		GDrawFillRect(pixmap,&r,gts->g.box->active_border);
	    }
	    GDrawDrawText(pixmap,x+gts->tabs[i].nesting*ni,y + gts->as,gts->tabs[i].name,-1,fg);
	    y += gts->fh;
	}
	fg = gts->g.box->main_foreground;
	if ( fg==COLOR_DEFAULT ) fg = GDrawGetDefaultForeground(GDrawGetDisplayOfWindow(pixmap));
	GDrawDrawLine(pixmap,x + gts->vert_list_width-4, g->r.y + bw,
			     x + gts->vert_list_width-4, g->r.y + g->r.height - (bw+1),
			     fg);
	if ( gts->vsb != NULL )
	    gts->vsb->funcs->handle_expose(pixmap,gts->vsb,event);
    } else {
	gts->haslarrow = gts->hasrarrow = false;
	if ( gts->scrolled ) {
	    x = g->r.x + GBoxBorderWidth(pixmap,gts->g.box);
	    y = g->r.y+yoff;
	    dsel = 0;
	    if ( gts->toff!=0 )
		x = DrawLeftArrowTab(pixmap,gts,x,y);
	    for ( i=gts->toff;
		    (i==gts->tabcnt-1 && x+gts->tabs[i].width < g->r.width) ||
		    (i<gts->tabcnt-1 && x+gts->tabs[i].width < g->r.width-gts->arrow_width ) ;
		    ++i ) {
		if ( i!=gts->sel )
		    x = DrawTab(pixmap,gts,i,x,y);
		else {
		    gts->tabs[i].x = x;
		    x += gts->tabs[i].width;
		    dsel = 1;
		}
	    }
	    if ( i!=gts->tabcnt ) {
		int p = gts->g.inner.x+gts->g.inner.width - gts->arrow_width;
		if ( p>x ) x=p;
		x = DrawRightArrowTab(pixmap,gts,x,y);
		gts->tabs[i].x = 0x7fff;
	    }
	    /* This one draws on top of the others, must come last */
	    if ( dsel )
		DrawTab(pixmap,gts,gts->sel,gts->tabs[gts->sel].x, g->r.y + (gts->rcnt-1) * gts->rowh + yoff );
	} else {
	    /* r is real row, rd is drawn pos */
	    /* rd is 0 at the top of the ggadget */
	    /* r is 0 when it contains tabs[0], (the index in the rowstarts array) */
	    for ( rd = 0; rd<gts->rcnt; ++rd ) {
		int r = (gts->rcnt-1-rd+gts->active_row)%gts->rcnt;
		y = g->r.y + rd * gts->rowh + yoff;
		x = g->r.x + (gts->rcnt-1-rd) * gts->offset_per_row + GBoxBorderWidth(pixmap,gts->g.box);
		for ( i = gts->rowstarts[r]; i<gts->rowstarts[r+1]; ++i )
		    if ( i==gts->sel ) {
			gts->tabs[i].x = x;
			x += gts->tabs[i].width;
		    } else
			x = DrawTab(pixmap,gts,i,x,y);
	    }
	    /* This one draws on top of the others, must come last */
	    DrawTab(pixmap,gts,gts->sel,gts->tabs[gts->sel].x, g->r.y + (gts->rcnt-1) * gts->rowh + yoff );
	}
    }
    if ( gts->nested_expose )
	(gts->nested_expose)(pixmap,g,event);
    GDrawPopClip(pixmap,&old1);
return( true );
}

static int GTabSetRCnt(GTabSet *gts, int totwidth) {
    int i, off, r, width;
    int bp = GBoxBorderWidth(gts->g.base,gts->g.box) + GDrawPointsToPixels(gts->g.base,5);

    width = totwidth;
    for ( i = off = r = 0; i<gts->tabcnt; ++i ) {
	if ( off!=0 && width-(gts->tabs[i].tw+2*bp)< 0 ) {
	    off = 0; ++r;
	    width = totwidth;
	}
	width -= gts->tabs[i].width;
	gts->tabs[i].x = off;
	off ++;
    }
return( r+1 );
}

static int GTabSetGetLineWidth(GTabSet *gts,int r) {
    int i, width = 0;

    for ( i=gts->rowstarts[r]; i<gts->rowstarts[r+1]; ++i )
	width += gts->tabs[i].width;
return( width );
}

static void GTabSetDistributePixels(GTabSet *gts,int r, int widthdiff) {
    int diff, off, i;

    diff = widthdiff/(gts->rowstarts[r+1]-gts->rowstarts[r]);
    off = widthdiff-diff*(gts->rowstarts[r+1]-gts->rowstarts[r]);
    for ( i=gts->rowstarts[r]; i<gts->rowstarts[r+1]; ++i ) {
	gts->tabs[i].width += diff;
	if ( off ) {
	    ++gts->tabs[i].width;
	    --off;
	}
    }
}

/* We have rearranged the rows of tabs. We want to make sure that each row */
/*  row is at least as wide as the row above it */
static void GTabSetFigureWidths(GTabSet *gts) {
    int bp = GBoxBorderWidth(gts->g.base,gts->g.box) + GDrawPointsToPixels(gts->g.base,5);
    int i, rd;
    int oldwidth=0, width;

    /* set all row widths to default values */
    for ( i=0; i<gts->tabcnt; ++i ) {
	gts->tabs[i].width = gts->tabs[i].tw + 2*bp;
    }
    /* r is real row, rd is drawn pos */
    /* rd is 0 at the top of the ggadget */
    /* r is 0 when it contains tabs[0], (the index in the rowstarts array) */
    if ( ( gts->filllines && gts->rcnt>1 ) || (gts->fill1line && gts->rcnt==1) ) {
	for ( rd = 0; rd<gts->rcnt; ++rd ) {
	    int r = (rd+gts->rcnt-1-gts->active_row)%gts->rcnt;
	    int totwidth = gts->g.r.width-2*GBoxBorderWidth(gts->g.base,gts->g.box) -
		    (gts->rcnt-1-rd)*gts->offset_per_row;
	    width = GTabSetGetLineWidth(gts,r);
	    GTabSetDistributePixels(gts,r,totwidth-width);
	}
    } else {
	for ( rd = 0; rd<gts->rcnt; ++rd ) {		/* r is real row, rd is drawn pos */
	    int r = (rd+gts->rcnt-1-gts->active_row)%gts->rcnt;
	    width = GTabSetGetLineWidth(gts,r) + (gts->rcnt-1-rd)*gts->offset_per_row;
	    if ( rd==0 )
		oldwidth = width;
	    else if ( oldwidth>width )
		GTabSetDistributePixels(gts,r,oldwidth-width);
	    else
		oldwidth = width;
	}
    }
}

/* Something happened which would change how many things fit on a line */
/*  (initialization, change of font, addition or removal of tab, etc.) */
/* Figure out how many rows we need and then how best to divide the tabs */
/*  between those rows, and then the widths of each tab */
static void GTabSet_Remetric(GTabSet *gts) {
    int bbp = GBoxBorderWidth(gts->g.base,gts->g.box);
    int bp = bbp + GDrawPointsToPixels(gts->g.base,5);
    int r, r2, width, i;
    int as, ds, ld;
    int ni = GDrawPointsToPixels(gts->g.base,NEST_INDENT), in;

    GDrawSetFont(gts->g.base,gts->font);
    GDrawWindowFontMetrics(gts->g.base,gts->font,&as,&ds,&ld);
    gts->as = as; gts->fh = as+ds;
    gts->rowh = as+ds + bbp+GDrawPointsToPixels(gts->g.base,3);
    gts->ds = ds + bbp+GDrawPointsToPixels(gts->g.base,1);
    gts->arrow_size = as+ds;
    gts->arrow_width = gts->arrow_size + 2*GBoxBorderWidth(gts->g.base,gts->g.box);
    gts->vert_list_width = 0;

    for ( i=0; i<gts->tabcnt; ++i ) {
	gts->tabs[i].tw = GDrawGetTextWidth(gts->g.base,gts->tabs[i].name,-1);
	gts->tabs[i].width = gts->tabs[i].tw + 2*bp;
	in = gts->tabs[i].nesting*ni;
	if ( gts->tabs[i].tw+in > gts->vert_list_width )
	    gts->vert_list_width = gts->tabs[i].tw+in;
    }
    if ( gts->vsb ) {
	gts->vert_list_width += gts->vsb->r.width;
	if ( gts->g.inner.height>26 ) {
	    int bp = GBoxBorderWidth(gts->g.base,gts->g.box);
	    GScrollBarSetBounds(gts->vsb,0,gts->tabcnt,(gts->g.r.height-2*bp-6)/gts->fh);
	}
    }
    gts->vert_list_width += 8;

    if ( gts->vertical ) {
	/* Nothing much to do */
    } else if ( gts->scrolled ) {
	free(gts->rowstarts);
	gts->rowstarts = malloc(2*sizeof(int16));
	gts->rowstarts[0] = 0; gts->rowstarts[1] = gts->tabcnt;
	gts->rcnt = 1;
    } else {
	width = gts->g.r.width-2*GBoxBorderWidth(gts->g.base,gts->g.box);
	r = GTabSetRCnt(gts,width);
	if ( gts->offset_per_row!=0 && r>1 )
	    while ( (r2 = GTabSetRCnt(gts,width-(r-1)*gts->offset_per_row))!=r )
		r = r2;
	free(gts->rowstarts);
	gts->rowstarts = malloc((r+1)*sizeof(int16));
	gts->rcnt = r;
	gts->rowstarts[r] = gts->tabcnt;
	for ( i=r=0; i<gts->tabcnt; ++i ) {
	    if ( gts->tabs[i].x==0 ) 
		gts->rowstarts[r++] = i;
	}
	/* if there is a single tab on the last line and there are more on */
	/*  the previous line, then things look nicer if we move one of the */
	/*  tabs from the previous line onto the last line */
	/*  Providing it fits, of course */
	if ( gts->rowstarts[r]-gts->rowstarts[r-1]==1 && r>1 &&
		gts->rowstarts[r-1]-gts->rowstarts[r-2]>1 &&
		gts->tabs[i-1].width+gts->tabs[i-2].width < width-(r-1)*gts->offset_per_row )
	    --gts->rowstarts[r-1];

	GTabSetFigureWidths(gts);
    }
}

static void GTabSetChangeSel(GTabSet *gts, int sel,int sendevent) {
    int i, width;
    int oldsel = gts->sel;

    if ( sel==-2 )		/* left arrow */
	--gts->toff;
    else if ( sel==-3 )
	++gts->toff;
    else if ( sel<0 || sel>=gts->tabcnt || gts->tabs[sel].disabled )
return;
    else {
	if ( gts->vertical )
	    gts->sel = sel;
	else {
	    for ( i=0; i<gts->rcnt && sel>=gts->rowstarts[i+1]; ++i );
	    if ( gts->active_row != i ) {
		gts->active_row = i;
		if ( gts->rcnt>1 && (!gts->filllines || gts->offset_per_row!=0))
		    GTabSetFigureWidths(gts);
	    }
	    gts->sel = sel;
	    if ( sel<gts->toff )
		gts->toff = sel;
	    else if ( gts->scrolled ) {
		for ( i=gts->toff; i<sel && gts->tabs[i].x!=0x7fff; ++i );
		if ( gts->tabs[i].x==0x7fff ) {
		    width = gts->g.r.width-2*gts->arrow_width;	/* it will have a left arrow */
		    if ( sel!=gts->tabcnt )
			width -= gts->arrow_width;		/* it might have a right arrow */
		    for ( i=sel; i>=0 && width-gts->tabs[i].width>=0; --i )
			width -= gts->tabs[i].width;
		    if ( ++i>sel ) i = sel;
		    gts->toff = i;
		}
	    }
	}
	if ( oldsel!=sel ) {
	    if ( sendevent )
		GTabSetChanged(gts,oldsel);
	    if ( gts->tabs[oldsel].w!=NULL )
		GDrawSetVisible(gts->tabs[oldsel].w,false);
	    if ( gts->tabs[gts->sel].w!=NULL )
		GDrawSetVisible(gts->tabs[gts->sel].w,true);
	}
    }
    _ggadget_redraw(&gts->g);
}

static int gtabset_mouse(GGadget *g, GEvent *event) {
    GTabSet *gts = (GTabSet *) g;

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return( false );
    if ( gts->nested_mouse!=NULL )
	if ( (gts->nested_mouse)(g,event))
return( true );

    if ( event->type == et_crossing ) {
return( true );
    } else if ( event->type == et_mousemove ) {
return( true );
    } else {
	int i, sel= -1, l;
	if ( event->u.mouse.y < gts->g.r.y ||
		( !gts->vertical && event->u.mouse.y >= gts->g.inner.y ) ||
		( gts->vertical && event->u.mouse.x >= gts->g.inner.x ))
return( false );
	else if ( gts->vertical ) {
	    int y = g->r.y + GBoxBorderWidth(g->base,g->box) + 5;
	    sel = (event->u.mouse.y-y)/gts->fh + gts->offtop;
	    if ( sel<0 || sel>=gts->tabcnt )
return(false);
	} else if ( gts->scrolled ) {
	    if ( gts->haslarrow && event->u.mouse.x<gts->tabs[gts->toff].x )
		sel = -2;	/* left arrow */
	    else {
		for ( i=gts->toff;
			i<gts->tabcnt && event->u.mouse.x>=gts->tabs[i].x+gts->tabs[i].width;
			++i );
		if ( gts->hasrarrow && gts->tabs[i].x==0x7fff &&
			event->u.mouse.x>=gts->tabs[i-1].x+gts->tabs[i-1].width )
		    sel = -3;
		else
		    sel = i;
	    }
	} else {
	    l = (event->u.mouse.y-gts->g.r.y)/gts->rowh;	/* screen row */
	    if ( l>=gts->rcnt ) l = gts->rcnt-1;		/* can happen on single line tabsets (there's extra space then) */
	    l = (gts->rcnt-1-l+gts->active_row)%gts->rcnt;	/* internal row number */
	    if ( event->u.mouse.x<gts->tabs[gts->rowstarts[l]].x )
		sel = -1;
	    else if ( event->u.mouse.x>=gts->tabs[gts->rowstarts[l+1]-1].x+gts->tabs[gts->rowstarts[l+1]-1].width )
		sel = -1;
	    else {
		for ( i=gts->rowstarts[l]; i<gts->rowstarts[l+1] &&
			event->u.mouse.x>=gts->tabs[i].x+gts->tabs[i].width; ++i );
		sel = i;
	    }
	}
	if ( event->type==et_mousedown ) {
	    gts->pressed = true;
	    gts->pressed_sel = sel;
	} else {
	    if ( gts->pressed && gts->pressed_sel == sel )
		GTabSetChangeSel(gts,sel,true);
	    gts->pressed = false;
	    gts->pressed_sel = -1;
	}
    }
return( true );
}

static int gtabset_key(GGadget *g, GEvent *event) {
    GTabSet *gts = (GTabSet *) g;
    int i;

    if ( !g->takes_input || !g->takes_keyboard || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return(false);
    if ( event->type == et_charup )
return( true );

    if (event->u.chr.keysym == GK_Left || event->u.chr.keysym == GK_KP_Left ||
	    (event->u.chr.keysym == GK_Tab && (event->u.chr.state&ksm_shift)) ||
	    event->u.chr.keysym == GK_BackTab ||
	    event->u.chr.keysym == GK_Up || event->u.chr.keysym == GK_KP_Up ) {
	for ( i = gts->sel-1; i>0 && gts->tabs[i].disabled; --i );
	GTabSetChangeSel(gts,i,true);
return( true );
    } else if (event->u.chr.keysym == GK_Right || event->u.chr.keysym == GK_KP_Right ||
	    event->u.chr.keysym == GK_Tab ||
	    event->u.chr.keysym == GK_Down || event->u.chr.keysym == GK_KP_Down ) {
	for ( i = gts->sel+1; i<gts->tabcnt-1 && gts->tabs[i].disabled; ++i );
	GTabSetChangeSel(gts,i,true);
return( true );
    }
return( false );
}

static int gtabset_focus(GGadget *g, GEvent *UNUSED(event)) {

    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active ))
return(false);

return( true );
}

static void gtabset_destroy(GGadget *g) {
    GTabSet *gts = (GTabSet *) g;
    int i;

    if ( gts==NULL )
return;
    free(gts->rowstarts);
    for ( i=0; i<gts->tabcnt; ++i ) {
	free(gts->tabs[i].name);
/* This has already been done */
/*	if ( gts->tabs[i].w!=NULL ) */
/*	    GDrawDestroyWindow(gts->tabs[i].w); */
    }
    free(gts->tabs);
    _ggadget_destroy(g);
}

static void GTabSetSetFont(GGadget *g,FontInstance *new) {
    GTabSet *gts = (GTabSet *) g;
    gts->font = new;
    GTabSet_Remetric(gts);
}

static FontInstance *GTabSetGetFont(GGadget *g) {
    GTabSet *gts = (GTabSet *) g;
return( gts->font );
}

static void _gtabset_redraw(GGadget *g) {
    GTabSet *gts = (GTabSet *) g;
    int i;

    GDrawRequestExpose(g->base, &g->r, false);
    i = gts->sel;
    if ( gts->tabs[i].w!=NULL )
	GDrawRequestExpose(gts->tabs[i].w, NULL, false);
}

static void _gtabset_move(GGadget *g, int32 x, int32 y ) {
    GTabSet *gts = (GTabSet *) g;
    int i;
    int32 nx = x+g->inner.x-g->r.x, ny = y+g->inner.y-g->r.y;

    for ( i=0; i<gts->tabcnt; ++i ) if ( gts->tabs[i].w!=NULL )
	GDrawMove(gts->tabs[i].w,nx,ny);
    _ggadget_move(g,x,y);
    if ( gts->vsb!=NULL ) {
	int bp = GBoxBorderWidth(gts->g.base,gts->g.box);
	GGadgetMove(gts->vsb,g->r.x +bp+ gts->vert_list_width - gts->vsb->r.width,
		    gts->g.r.y+bp);
    }
}

static void _gtabset_resize(GGadget *g, int32 width, int32 height ) {
    GTabSet *gts = (GTabSet *) g;
    int i;

    _ggadget_resize(g,width,height);
    for ( i=0; i<gts->tabcnt; ++i ) if ( gts->tabs[i].w!=NULL )
	GDrawResize(gts->tabs[i].w,g->inner.width,g->inner.height);
    if ( gts->vsb!=NULL ) {
	int off = gts->offtop;
	int bp = GBoxBorderWidth(gts->g.base,gts->g.box);
	GGadgetResize(gts->vsb,gts->vsb->r.width, gts->g.r.height-2*bp);
	GScrollBarSetBounds(gts->vsb,0,gts->tabcnt,(gts->g.r.height-2*bp-6)/gts->fh);
	if ( gts->offtop + (gts->g.r.height-2*bp-6)/gts->fh > gts->tabcnt )
	    off = gts->tabcnt - (gts->g.r.height-2*bp-6)/gts->fh;
	if ( off<0 )
	    off = 0;
	if ( off!=gts->offtop ) {
	    gts->offtop = off;
	    GScrollBarSetPos(gts->vsb, off );
	    GGadgetRedraw(&gts->g);
	}
    }
}

static void _gtabset_setvisible(GGadget *g,int visible) {
    GTabSet *gts = (GTabSet *) g;

    _ggadget_setvisible(g,visible);
    if ( gts->tabs[gts->sel].w!=NULL )
	GDrawSetVisible(gts->tabs[gts->sel].w, visible);
    if ( gts->vsb!=NULL )
	GGadgetSetVisible(gts->vsb,visible);
}

static int gtabset_FillsWindow(GGadget *g) {
return( g->prev==NULL && _GWidgetGetGadgets(g->base)==g );
}

static void gtabset_GetDesiredSize(GGadget *g, GRect *outer, GRect *inner) {
    GTabSet *gts = (GTabSet *) g;
    int bp = GBoxBorderWidth(g->base,g->box);
    GRect nested, test;
    int i;

    memset(&nested,0,sizeof(nested));
    for ( i=0; i<gts->tabcnt; ++i ) {
	GGadget *last = _GWidgetGetGadgets(gts->tabs[i].w);
	if ( last!=NULL ) {
	    while ( last->prev!=NULL )
		last=last->prev;
	    GGadgetGetDesiredSize(last,&test,NULL);
	    if ( GGadgetFillsWindow(last)) {
		test.width += 2*last->r.x;
		test.height += 2*last->r.y;
	    }
	    if ( test.width>nested.width ) nested.width = test.width;
	    if ( test.height>nested.height ) nested.height = test.height;
	}
    }
    if ( gts->vertical ) {
	if ( gts->vsb==NULL && gts->rcnt*gts->fh+10 > nested.height )
	    nested.height = gts->tabcnt*gts->fh+10;
	else if ( gts->vsb!=NULL && 2*gts->vsb->r.width + 20 > nested.height )
	    nested.height = 2*gts->vsb->r.width + 20;	/* Minimum size for scrollbar arrows, vaguely */
    }

    if ( g->desired_width>=0 ) nested.width = g->desired_width - 2*bp;
    if ( g->desired_height>=0 ) nested.height = g->desired_height - 2*bp;
    if ( nested.width==0 ) nested.width = 100;
    if ( nested.height==0 ) nested.height = 100;

    if ( inner != NULL )
	*inner = nested;
    if ( gts->vertical ) {
	if ( outer != NULL ) {
	    *outer = nested;
	    outer->width += gts->vert_list_width + 2*bp;
	    outer->height += 2*bp;
	}
    } else {
	if ( outer != NULL ) {
	    *outer = nested;
	    outer->width += 2*bp;
	    outer->height += gts->rcnt*gts->rowh + bp;
	}
    }
}

struct gfuncs gtabset_funcs = {
    0,
    sizeof(struct gfuncs),

    gtabset_expose,
    gtabset_mouse,
    gtabset_key,
    NULL,
    gtabset_focus,
    NULL,
    NULL,

    _gtabset_redraw,
    _gtabset_move,
    _gtabset_resize,
    _gtabset_setvisible,
    _ggadget_setenabled,		/* Doesn't work right */
    _ggadget_getsize,
    _ggadget_getinnersize,

    gtabset_destroy,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    GTabSetSetFont,
    GTabSetGetFont,

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

    gtabset_GetDesiredSize,
    _ggadget_setDesiredSize,
    gtabset_FillsWindow,
    NULL
};

static int sendtoparent_eh(GWindow gw, GEvent *event) {
    switch ( event->type ) {
      case et_controlevent: case et_char: case et_drop:
	event->w = GDrawGetParentWindow(gw);
	GDrawPostEvent(event);
      break;
      case et_resize:
	GDrawRequestExpose(gw,NULL,false);
      break;
    }

return( true );
}

static int gtabset_vscroll(GGadget *g, GEvent *event) {
    enum sb sbt = event->u.control.u.sb.type;
    GTabSet *gts = (GTabSet *) (g->data);
    int loff = gts->offtop;
    int page = (g->inner.height-6)/gts->fh- ((g->inner.height-6)/gts->fh>2?1:0);

    if ( sbt==et_sb_top )
	loff = 0;
    else if ( sbt==et_sb_bottom ) {
	loff = gts->tabcnt - (gts->g.inner.height-6)/gts->fh;
    } else if ( sbt==et_sb_up ) {
	--loff;
    } else if ( sbt==et_sb_down ) {
	++loff;
    } else if ( sbt==et_sb_uppage ) {
	loff -= page;
    } else if ( sbt==et_sb_downpage ) {
	loff += page;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	loff = event->u.control.u.sb.pos;
    }
    if ( loff + (gts->g.inner.height-6)/gts->fh > gts->tabcnt )
	loff = gts->tabcnt - (gts->g.inner.height-6)/gts->fh;
    if ( loff<0 )
	loff = 0;
    if ( loff!=gts->offtop ) {
	gts->offtop = loff;
	GScrollBarSetPos(gts->vsb, loff );
	GGadgetRedraw(&gts->g);
    }
return( true );
}

GGadget *GTabSetCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GTabSet *gts = calloc(1,sizeof(GTabSet));
    int i, bp;
    GRect r;
    GWindowAttrs childattrs;

    memset(&childattrs,0,sizeof(childattrs));
    childattrs.mask = wam_events;
    childattrs.event_masks = -1;

    if ( !gtabset_inited )
	GTabSetInit();
    gts->g.funcs = &gtabset_funcs;
    _GGadget_Create(&gts->g,base,gd,data, gd->flags&gg_tabset_vert ? &gvtabset_box  : &gtabset_box);
    gts->font = gtabset_font;

    gts->g.takes_input = true; gts->g.takes_keyboard = true; gts->g.focusable = true;

    GDrawGetSize(base,&r);
    if ( gd->pos.x <= 0 )
	gts->g.r.x = GDrawPointsToPixels(base,2);
    if ( gd->pos.y <= 0 )
	gts->g.r.y = GDrawPointsToPixels(base,2);
    if ( gd->pos.width<=0 )
	gts->g.r.width = r.width - gts->g.r.x - GDrawPointsToPixels(base,2);
    if ( gd->pos.height<=0 ) {
	if ( gd->flags&gg_tabset_nowindow )
	    gts->g.r.height = GDrawPointsToPixels(base,20);
	else
	    gts->g.r.height = r.height - gts->g.r.y - GDrawPointsToPixels(base,26);
    }

    for ( i=0; gd->u.tabs[i].text!=NULL; ++i );
    gts->tabcnt = i;
    gts->tabs = malloc(i*sizeof(struct tabs));
    for ( i=0; gd->u.tabs[i].text!=NULL; ++i ) {
	if ( gd->u.tabs[i].text_in_resource )
	    gts->tabs[i].name = u_copy(GStringGetResource((intpt) (gd->u.tabs[i].text),NULL));
	else if ( gd->u.tabs[i].text_is_1byte )
	    gts->tabs[i].name = utf82u_copy((char *) (gd->u.tabs[i].text));
	else
	    gts->tabs[i].name = u_copy(gd->u.tabs[i].text);
	gts->tabs[i].disabled = gd->u.tabs[i].disabled;
	gts->tabs[i].nesting = gd->u.tabs[i].nesting;
	if ( gd->u.tabs[i].selected && !gts->tabs[i].disabled )
	    gts->sel = i;
    }
    if ( gd->flags&gg_tabset_scroll ) gts->scrolled = true;
    if ( gd->flags&gg_tabset_filllines ) gts->filllines = true;
    if ( gd->flags&gg_tabset_fill1line ) gts->fill1line = true;
    if ( gd->flags&gg_tabset_vert ) gts->vertical = true;
    gts->offset_per_row = GDrawPointsToPixels(base,2);
    if ( gts->vertical && gts->scrolled ) {
	GGadgetData gd;
	memset(&gd,'\0',sizeof(gd));
	gd.pos.y = gts->g.r.y; gd.pos.height = gts->g.inner.height;
	gd.pos.width = GDrawPointsToPixels(gts->g.base,_GScrollBar_Width);
	gd.pos.x = gts->g.inner.x;
	gd.flags = (gts->g.state==gs_invisible?0:gg_visible)|gg_enabled|gg_pos_in_pixels|gg_sb_vert;
	gd.handle_controlevent = gtabset_vscroll;
	gts->vsb = GScrollBarCreate(gts->g.base,&gd,gts);
	gts->vsb->contained = true;
    }
    GTabSet_Remetric(gts);
    _GGadget_FinalPosition(&gts->g,base,gd);

    bp = GBoxBorderWidth(base,gts->g.box);
    gts->g.inner = gts->g.r;
    if ( gts->vertical ) {
	gts->g.inner.x += bp+gts->vert_list_width; gts->g.inner.width -= 2*bp+gts->vert_list_width;
	gts->g.inner.y += bp; gts->g.inner.height -= 2*bp;
    } else {
	gts->g.inner.x += bp; gts->g.inner.width -= 2*bp;
	gts->g.inner.y += gts->rcnt*gts->rowh+bp; gts->g.inner.height -= 2*bp+gts->rcnt*gts->rowh;
    }
    if ( gts->rcnt==1 ) {
	gts->g.inner.y += bp; gts->g.inner.height -= bp;
    }
    if ( gts->vsb!=NULL ) {
	GGadgetMove(gts->vsb, gts->g.r.x + bp + gts->vert_list_width - gts->vsb->r.width, gts->g.r.y + bp);
	GGadgetResize(gts->vsb, gts->vsb->r.width, gts->g.r.height-2*bp);
	if ( gts->g.inner.height>26 )
	    GScrollBarSetBounds(gts->vsb,0,gts->tabcnt,(gts->g.r.height-2*bp-6)/gts->fh);
    }

    if ( gd->flags&gg_tabset_nowindow ) gts->nowindow = true;

    for ( i=0; gd->u.tabs[i].text!=NULL; ++i )
	if ( !(gd->flags&gg_tabset_nowindow)) {
	    gts->tabs[i].w = GDrawCreateSubWindow(base,&gts->g.inner,sendtoparent_eh,GDrawGetUserData(base),&childattrs);
	    if ( gd->u.tabs[i].gcd!=NULL )
		GGadgetsCreate(gts->tabs[i].w,gd->u.tabs[i].gcd);
	    if ( gts->sel==i && (gd->flags & gg_visible ))
		GDrawSetVisible(gts->tabs[i].w,true);
	} else
	    gts->tabs[i].w = NULL;

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&gts->g);

    for ( i=0; gd->u.tabs[i].text!=NULL && !gd->u.tabs[i].selected; ++i );
    if ( i!=0 && gd->u.tabs[i].text!=NULL )
	GTabSetChangeSel(gts,i,false);

return( &gts->g );
}

int GTabSetGetSel(GGadget *g) {
    GTabSet *gts = (GTabSet *) g;
return( gts->sel );
}

void GTabSetSetSel(GGadget *g,int sel) {
    GTabSet *gts = (GTabSet *) g;
    GTabSetChangeSel(gts,sel,false);
}

void GTabSetSetEnabled(GGadget *g,int pos,int enabled) {
    GTabSet *gts = (GTabSet *) g;

    if ( pos>=0 && pos<gts->tabcnt ) {
	gts->tabs[pos].disabled = !enabled;
    }
    GDrawRequestExpose(g->base, &g->r, false);
}

GWindow GTabSetGetSubwindow(GGadget *g,int pos) {
    GTabSet *gts = (GTabSet *) g;

    if ( pos>=0 && pos<gts->tabcnt )
return( gts->tabs[pos].w );

return( NULL );
}

int GTabSetGetTabLines(GGadget *g) {
    GTabSet *gts = (GTabSet *) g;
return( gts->rcnt );
}

void GTabSetSetNestedExpose(GGadget *g, void (*ne)(GWindow,GGadget *,GEvent *)) {
    GTabSet *gts = (GTabSet *) g;
    gts->nested_expose = ne;
}

void GTabSetSetNestedMouse(GGadget *g, int (*nm)(GGadget *,GEvent *)) {
    GTabSet *gts = (GTabSet *) g;
    gts->nested_mouse = nm;
}

void GTabSetChangeTabName(GGadget *g, const char *name, int pos) {
    GTabSet *gts = (GTabSet *) g;

    if ( pos==gts->tabcnt && gts->nowindow ) {
	gts->tabs = realloc(gts->tabs,(pos+1)*sizeof(struct tabs));
	memset(&gts->tabs[pos],0,sizeof(struct tabs));
	++gts->tabcnt;
    }
    if ( pos<gts->tabcnt ) {
	free(gts->tabs[pos].name);
	gts->tabs[pos].name = utf82u_copy(name);
    }
}

void GTabSetRemetric(GGadget *g) {
    GTabSet *gts = (GTabSet *) g;
    GTabSet_Remetric(gts);
}

void GTabSetRemoveTabByPos(GGadget *g, int pos) {
    GTabSet *gts = (GTabSet *) g;
    int i;

    if ( gts->nowindow && pos>=0 && pos<gts->tabcnt && gts->tabcnt>1 ) {
	free(gts->tabs[pos].name);
	for ( i=pos+1; i<gts->tabcnt; ++i )
	    gts->tabs[i-1] = gts->tabs[i];
	--gts->tabcnt;
	if ( gts->sel==pos ) {
	    if ( gts->sel==gts->tabcnt )
		--gts->sel;
	    GTabSetChanged(gts,pos);
	}
    }
}
	
void GTabSetRemoveTabByName(GGadget *g, char *name) {
    GTabSet *gts = (GTabSet *) g;
    int pos;
    unichar_t *uname = utf82u_copy(name);

    for ( pos=0; pos<gts->tabcnt; ++pos ) {
	if ( u_strcmp(uname,gts->tabs[pos].name)==0 ) {
	    GTabSetRemoveTabByPos(g,pos);
    break;
	}
    }

    free(uname);
}

GResInfo *_GTabSetRIHead(void) {
    if ( !gtabset_inited )
	GTabSetInit();
return( &gtabset_ri );
}
