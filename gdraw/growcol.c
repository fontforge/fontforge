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
#include "gkeysym.h"
#include "ggadgetP.h"
#include "gwidget.h"
#include "ustring.h"

static Color GRowColLabelBg = 0xc0c0c0;

static void GRowColSelected(GRowCol *l) {
    GEvent e;

    e.type = et_controlevent;
    e.w = l->g.base;
    e.u.control.subtype = et_listselected;
    e.u.control.g = &l->g;
    e.u.control.u.list.from_mouse = false;
    if ( l->g.handle_controlevent != NULL )
	(l->g.handle_controlevent)(&l->g,&e);
    else
	GDrawPostEvent(&e);
}

static void GRowColDoubleClick(GRowCol *l) {
    GEvent e;

    e.type = et_controlevent;
    e.w = l->g.base;
    e.u.control.subtype = et_listdoubleclick;
    e.u.control.g = &l->g;
    if ( l->g.handle_controlevent != NULL )
	(l->g.handle_controlevent)(&l->g,&e);
    else
	GDrawPostEvent(&e);
}

static int GRowColAlphaCompare(const void *v1, const void *v2) {
    GTextInfo * const *pt1 = v1, * const *pt2 = v2;
return( GTextInfoCompare(*pt1,*pt2));
}

static void GRowColOrderIt(GRowCol *grc) {
    if ( grc->order_entries ) {
	/* Sort each entry left to right then top to bottom (1 2 3/4 5 6/...) */
	qsort(grc->ti,grc->rows*grc->cols,sizeof(GTextInfo *),grc->orderer);
	if ( grc->backwards ) {
	    int i;
	    GTextInfo *ti;
	    for ( i=0; i<grc->ltot/2; ++i ) {
		ti = grc->ti[i];
		grc->ti[i] = grc->ti[grc->ltot-1-i];
		grc->ti[grc->ltot-1-i] = ti;
	    }
	}
    } else {
	/* Sort each row, top to bottom (1 8 5/2 7 3/...) */
	qsort(grc->ti,grc->rows,sizeof(GTextInfo *)*grc->cols,grc->orderer);
	if ( grc->backwards ) {
	    int i;
	    GTextInfo **ti;
	    ti = gmalloc(grc->cols*sizeof(GTextInfo *));
	    for ( i=0; i<grc->rows/2; ++i ) {
		memcpy(ti,grc->ti+(i*grc->cols),grc->cols*sizeof(GTextInfo *));
		memcpy(grc->ti+(i*grc->cols),grc->ti+((grc->rows-1-i)*grc->cols),grc->cols*sizeof(GTextInfo *));
		memcpy(grc->ti+((grc->rows-1-i)*grc->cols),ti,grc->cols*sizeof(GTextInfo *));
	    }
	    free(ti);
	}
    }
}

static void GRowColClearSel(GRowCol *grc) {
    int i;

    for ( i=0; i<grc->ltot; ++i )
	grc->ti[i]->selected = false;
}

static int GRowColAnyOtherSels(GRowCol *grc, int pos) {
    int i;

    for ( i=0; i<grc->ltot; ++i )
	if ( grc->ti[i]->selected && i!=pos )
return( true );

return( false );
}

static int32 GRowColGetFirstSelPos(GGadget *g) {
    int i;
    GRowCol *grc = (GRowCol *) g;

    for ( i=0; i<grc->ltot; ++i )
	if ( grc->ti[i]->selected )
return( i );

return( -1 );
}

static void GRowColSelect(GGadget *g, int32 pos, int32 sel) {
    GRowCol *grc = (GRowCol *) g;

    GRowColClearSel(grc);
    if ( pos>=grc->ltot || pos<0 )
return;
    if ( grc->ltot>0 ) {
	grc->ti[pos]->selected = sel;
	_ggadget_redraw(g);
    }
}

static void GRowColSelectOne(GGadget *g, int32 pos) {
    GRowCol *grc = (GRowCol *) g;

    GRowColClearSel(grc);
    if ( pos>=grc->ltot ) pos = grc->ltot-1;
    if ( pos<0 ) pos = 0;
    if ( grc->ltot>0 ) {
	grc->ti[pos]->selected = true;
	_ggadget_redraw(g);
    }
}

static int32 GRowColIsItemSelected(GGadget *g, int32 pos) {
    GRowCol *grc = (GRowCol *) g;

    if ( pos>=grc->ltot )
return( false );
    if ( pos<0 )
return( false );
    if ( grc->ltot>0 )
return( grc->ti[pos]->selected );

return( false );
}

static int GRowColIndexFromPos(GRowCol *grc,int y) {
    int i, height;

    y -= grc->g.inner.y;
    if ( y<0 ) y=0;
    if ( y>=grc->g.inner.height ) y = grc->g.inner.height-1;
    for ( i=grc->loff, height=0; i<grc->ltot; ++i ) {
	int temp = GTextInfoGetHeight(grc->g.base,grc->ti[i],grc->font);
	if ( height+temp>y )
    break;
	height += temp;
    }
    if ( i==grc->ltot )
return( -1 );
    if ( grc->ti[i]->disabled )
return( -1 );
return( i );
}

static void GRowColScrollBy(GRowCol *grc,int loff,int xoff) {
    int top = GRowColTopInWindow(grc,grc->ltot-1);
    int ydiff, i;

    if ( grc->loff + loff < 0 )
	loff = -grc->loff;
    else if ( grc->loff + loff > top )
	loff = top-grc->loff;
    if ( xoff+grc->xoff<0 )
	xoff = -grc->xoff;
    else if ( xoff+grc->xoff+grc->g.inner.width > grc->xmax ) {
	xoff = grc->xmax-grc->g.inner.width-grc->xoff;
	if ( xoff<0 ) xoff = 0;
    }
    if ( loff == 0 && xoff==0 )
return;

    ydiff = 0;
    if ( loff>0 ) {
	for ( i=0; i<loff && ydiff<grc->g.inner.height; ++i )
	    ydiff += GTextInfoGetHeight(grc->g.base,grc->ti[i+grc->loff],grc->font);
    } else if ( loff<0 ) {
	for ( i=loff; i<0 && -ydiff<grc->g.inner.height; ++i )
	    ydiff -= GTextInfoGetHeight(grc->g.base,grc->ti[i+grc->loff],grc->font);
    }
    if ( !GDrawIsVisible(grc->g.base))
return;
    GDrawForceUpdate(grc->g.base);
    grc->loff += loff; grc->xoff += xoff;
    if ( ydiff>=grc->g.inner.height || -ydiff >= grc->g.inner.height )
	_ggadget_redraw(&grc->g);
    else if ( ydiff!=0 || xoff!=0 )
	GDrawScroll(grc->g.base,&grc->g.inner,xoff,ydiff);
    if ( loff!=0 )
	GScrollBarSetPos(&grc->vsb->g,grc->loff);
}

static int GRowColFindPosition(GRowCol *grc,unichar_t *text) {
    GTextInfo temp, *ptemp=&temp;
    int i, order;

    if ( grc->orderer!=NULL ) {
	memset(&temp,'\0',sizeof(temp));
	temp.text = text;

	/* I don't think I need to write a binary search here... */
	for ( i=0; i<grc->ltot; ++i ) {
	    order = (grc->orderer)(&ptemp,&grc->ti[i]);
	    if (( order<= 0 && !grc->backwards ) || ( order>=0 && grc->backwards ))
return( i );
	}
return( 0 );
    } else {
	for ( i=0; i<grc->ltot; ++i ) {
	    if (u_strmatch(text,grc->ti[i]->text)==0 )
return( i );
	}
    }
return( 0 );
}

static int GRowColAdjustPos(GGadget *g,int pos) {
    GRowCol *grc = (GRowCol *) g;
    int newoff = grc->loff;

    if ( pos<grc->loff ) {
	if (( newoff = pos-1)<0 ) newoff = 0;
	if ( GRowColLinesInWindow(grc,newoff)<2 )
	    newoff = pos;
    } else if ( pos >= grc->loff + GRowColLinesInWindow(grc,grc->loff) ) {
	newoff = GRowColTopInWindow(grc,pos);
	if ( pos!=grc->ltot-1 && GRowColLinesInWindow(grc,newoff+1)>=2 )
	    ++newoff;
    }
return( newoff );
}

static void GRowColShowPos(GGadget *g,int pos) {
    GRowCol *grc = (GRowCol *) g;
    int newoff = GRowColAdjustPos(g,pos);
    if ( newoff!=grc->loff )
	GRowColScrollBy(grc,newoff-grc->loff,0);
}

static void GRowColScrollToText(GGadget *g,unichar_t *text,int sel) {
    GRowCol *grc = (GRowCol *) g;
    int pos;

    pos = GRowColFindPosition(grc,text);
    if ( sel && pos<grc->ltot ) {
	GRowColClearSel(grc);
	if ( grc->exactly_one || u_strmatch(text,grc->ti[pos]->text)==0 )
	    grc->ti[pos]->selected = true;
    }
    grc->loff = GRowColAdjustPos(g,pos);
    _ggadget_redraw(g);
}

static void GRowColSetOrderer(GGadget *g,int (*orderer)(const void *, const void *)) {
    GRowCol *grc = (GRowCol *) g;

    grc->orderer = orderer;
    if ( orderer!=NULL ) {
	GRowColOrderIt(grc);
	GRowColScrollBy(grc,-grc->loff,-grc->xoff);
	_ggadget_redraw(&grc->g);
    }
}

static int growcol_scroll(GGadget *g, GEvent *event);
static void GRowColCheckSB(GRowCol *grc) {
    if ( GRowColLinesInWindow(grc,0)<grc->ltot ) {
	if ( grc->vsb==NULL ) {
	    GGadgetData gd;
	    memset(&gd,'\0',sizeof(gd));
	    gd.pos.y = grc->g.r.y; gd.pos.height = grc->g.r.height;
	    gd.pos.width = GDrawPointsToPixels(grc->g.base,_GScrollBar_Width);
	    gd.pos.x = grc->g.r.x+grc->g.r.width - gd.pos.width;
	    gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert|gg_pos_use0;
	    gd.handle_controlevent = growcol_scroll;
	    grc->vsb = (GScrollBar *) GScrollBarCreate(grc->g.base,&gd,grc);
	    grc->vsb->g.contained = true;

	    gd.pos.width += GDrawPointsToPixels(grc->g.base,1);
	    grc->g.r.width -= gd.pos.width;
	    grc->g.inner.width -= gd.pos.width;
	}
	GScrollBarSetBounds(&grc->vsb->g,0,grc->ltot-1,GRowColLinesInWindow(grc,0));
	GScrollBarSetPos(&grc->vsb->g,grc->loff);
    } else {
	if ( grc->vsb!=NULL ) {
	    int wid = grc->vsb->g.r.width + GDrawPointsToPixels(grc->g.base,1);
	    (grc->vsb->g.funcs->destroy)(&grc->vsb->g);
	    grc->vsb = NULL;
	    grc->g.r.width += wid;
	    grc->g.inner.width += wid;
	}
    }
}

static int GRowColFindXMax(GRowCol *grc) {
    int i, width=0, temp;

    for ( i=0; i<grc->ltot; ++i ) {
	temp = GTextInfoGetWidth(grc->g.base,grc->ti[i],grc->font);
	if ( temp>width ) width=temp;
    }
    grc->xmax = width;
return( width );
}

static void GRowColSetList(GGadget *g,GTextInfo **ti,int docopy) {
    GRowCol *grc = (GRowCol *) g;
    int same;

    GTextInfoArrayFree(grc->ti);
    if ( docopy || ti==NULL )
	ti = GTextInfoArrayCopy(ti);
    grc->ti = ti;
    grc->ltot = GTextInfoArrayCount(ti);
    if ( grc->orderer!=NULL )
	GRowColOrderIt(grc);
    grc->loff = grc->xoff = 0;
    grc->hmax = GTextInfoGetMaxHeight(g->base,ti,grc->font,&same);
    grc->sameheight = same;
    GRowColCheckSB(grc);
    _ggadget_redraw(&grc->g);
}

static void GRowColClear(GGadget *g) {
    GRowColSetList(g,NULL,true);
}

static GTextInfo **GRowColGetList(GGadget *g,int32 *len) {
    GRowCol *grc = (GRowCol *) g;
    if ( len!=NULL ) *len = grc->ltot;
return( grc->ti );
}

static GTextInfo *GRowColGetListItem(GGadget *g,int32 pos) {
    GRowCol *grc = (GRowCol *) g;
    if ( pos<0 || pos>=grc->ltot )
return( NULL );

return(grc->ti[pos]);
}

static int growcol_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GRowCol *grc = (GRowCol *) g;
    GRect old1, old2;
    Color fg, dfg;
    int y, l, ymax, i;

    if ( g->state == gs_invisible )
return( false );

    GDrawPushClip(pixmap,&g->r,&old1);

    GBoxDrawBackground(pixmap,&g->r,g->box, g->state,false);
    if ( g->box->border_type!=bt_none ||
	    (g->box->flags&(box_foreground_border_inner|box_foreground_border_outer|box_active_border_inner))!=0 ) {
	GBoxDrawBorder(pixmap,&g->r,g->box,g->state,false);

	GDrawPushClip(pixmap,&g->inner,&old2);
    }

    fg = g->state==gs_disabled?g->box->disabled_foreground:g->box->main_foreground;
    dfg = g->box->disabled_foreground;
    y = g->inner.y;
    ymax = g->inner.y+g->inner.height;
    if ( ymax>event->u.expose.rect.y+event->u.expose.rect.height )
	ymax = event->u.expose.rect.y+event->u.expose.rect.height;
    if ( grc->labels!=NULL ) {
	if ( y+grc->fh > event->u.expose.rect.y ) {
	    GRect r;
	    r = g->inner; r.height = fh;
	    GDrawFillRect(pixmap,&r,GRowColLabelBg);
	    for ( i=0; i<grc->cols; ++i ) {
		GTextInfoDraw(pixmap,g->inner.x+grc->colx[i]+grc->hpad-grc->xoff,y,grc->labels[i],
			grc->font,dfg,g->box->active_border
			g->inner.y+g->inner.height);
	    }
	}
	y += grc->fh;
	if ( grc->hrules )
	    GDrawDrawLine(pixmap,g->inner.x,y-1,g->inner.x+g->inner.width,y-1,fg);
    }
    for ( l = grc->loff; y<ymax && l<grc->rows; ++l ) {
	if ( y+grc->fh > event->u.expose.rect.y ) {
	    for ( i=0; i<grc->cols; ++i ) {
		if ( l!=grc->tfr || i!=grc->tfc ) {
		    GTextInfo *ti = grc->ti[l*grc->cols+i];
		    GTextInfoDraw(pixmap,g->inner.x+grc->colx[i]-grc->xoff+grc->hpad,y,ti,
			    grc->font,dfg,g->box->active_border,
			    g->inner.y+g->inner.height);
		} else {
		    /* now we can draw the text field */
		    grc->tf->dontdraw = false;
		    (grc->tf->g.gfuncs->handle_expose)(pixmap,&grc->tf->g,event);
		    grc->tf->dontdraw = true;
		}
	    }
	}
	y += grc->fh;
	if ( grc->hrules )
	    GDrawDrawLine(pixmap,g->inner.x,y-1,g->inner.x+g->inner.width,y-1,fg);
    }
    if ( grc->vrules ) {
	for ( i=1; i<grc->cols; ++i ) 
	    GDrawDrawLine(pixmap,grc->colx[i]-grc->xoff,g->inner.y,
		    grc->colx[i]-grc->xoff,g->inner.y+g->inner.height,fg);
    }
    if ( g->box->border_type!=bt_none ||
	    (g->box->flags&(box_foreground_border_inner|box_foreground_border_outer|box_active_border_inner))!=0 )
	GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
return( true );
}

static void growcol_scroll_selbymouse(GGadget *g, GEvent *event) {
    int loff=0, xoff=0, pos;
    GRowCol *grc = (GRowCol *) (g->data);

    if ( event->u.mouse.y<grc->g.inner.y ) {
	if ( grc->loff>0 ) loff = -1;
    } else if ( event->u.mouse.y >= grc->g.inner.y+grc->g.inner.height ) {
	int top = GRowColTopInWindow(grc,grc->ltot-1);
	if ( grc->loff<top ) loff = 1;
    }
    if ( event->u.mouse.x<grc->g.inner.x ) {
	xoff = -GDrawPointsToPixels(grc->g.base,6);
    } else if ( event->u.mouse.x >= grc->g.inner.x+grc->g.inner.width ) {
	xoff = GDrawPointsToPixels(grc->g.base,6);
    }
    GRowColScrollBy(grc,loff,xoff);
    pos = GRowColIndexFromPos(grc,event->u.mouse.y);
    if ( pos==-1 || pos == grc->end )
	/* Do Nothing, nothing selectable */;
    else if ( !grc->multiple_sel ) {
	GRowColClearSel(grc);
	grc->ti[pos]->selected = true;
	grc->start = grc->end = pos;
	_ggadget_redraw(&grc->g);
    } else {
	GRowColExpandSelection(grc,pos);
	grc->end = pos;
	_ggadget_redraw(&grc->g);
    }
}

static int growcol_mouse(GGadget *g, GEvent *event) {
    GRowCol *grc = (GRowCol *) g;
    int pos;

    if ( !g->takes_input || (g->state!=gs_active && g->state!=gs_enabled && g->state!=gs_focused))
return( false );

    if ( event->type == et_crossing )
return( false );
    if ( event->type==et_mousemove && !grc->pressed && !grc->parentpressed ) {
	if ( GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y) && g->popup_msg )
	    GGadgetPreparePopup(g->base,g->popup_msg);
return( true );
    } else if ( event->type==et_mouseup && grc->parentpressed &&
	    !GGadgetInnerWithin(&grc->g,event->u.mouse.x,event->u.mouse.y)) {
	grc->parentpressed = false;
	GDrawPointerUngrab(GDrawGetDisplayOfWindow(grc->g.base));
    } else if ( event->type==et_mousemove && grc->parentpressed &&
	    GGadgetInnerWithin(&grc->g,event->u.mouse.x,event->u.mouse.y)) {
	if ( grc->pressed == NULL )
	    grc->pressed = GDrawRequestTimer(g->base,GRowColScrollTime,GRowColScrollTime,NULL);
	GDrawPointerUngrab(GDrawGetDisplayOfWindow(grc->g.base));
	grc->parentpressed = false;
	growcol_scroll_selbymouse(grc,event);
return( true );
    } else if ( event->type==et_mousemove && grc->pressed ) {
	growcol_scroll_selbymouse(grc,event);
return( true );
    } else if ( event->type==et_mousedown ) {
	if ( grc->pressed == NULL )
	    grc->pressed = GDrawRequestTimer(g->base,GRowColScrollTime,GRowColScrollTime,NULL);
	pos = GRowColIndexFromPos(grc,event->u.mouse.y);
	if ( pos==-1 )
return( true ); /* Do Nothing, nothing selectable */
	else if ( !grc->exactly_one && grc->ti[pos]->selected &&
		(event->u.mouse.state&(ksm_control|ksm_shift))) {
	    grc->ti[pos]->selected = false;
	} else if ( !grc->multiple_sel ||
		!(event->u.mouse.state&(ksm_control|ksm_shift))) {
	    GRowColClearSel(grc);
	    grc->ti[pos]->selected = true;
	    grc->start = grc->end = pos;
	} else if ( event->u.mouse.state&ksm_control ) {
	    grc->ti[pos]->selected = !grc->ti[pos]->selected;
	    grc->start = grc->end = pos;
	} else if ( event->u.mouse.state&ksm_shift ) {
	    GRowColExpandSelection(grc,pos);
	}
	_ggadget_redraw(&grc->g);
    } else if ( event->type==et_mouseup && grc->pressed ) {
	GDrawCancelTimer(grc->pressed); grc->pressed = NULL;
	if ( GGadgetInnerWithin(&grc->g,event->u.mouse.x,event->u.mouse.y) ) {
	    growcol_scroll_selbymouse(grc,event);
	    if ( event->u.mouse.clicks==2 )
		GRowColDoubleClick(grc);
	    else
		GRowColSelected(grc);
	}
    } else
return( false );

return( true );
}

static int growcol_key(GGadget *g, GEvent *event) {
    GRowCol *grc = (GRowCol *) g;
    uint16 keysym = event->u.chr.keysym;
    int sofar_pos = grc->sofar_pos;
    int loff, xoff, sel=-1;
    int refresh = false;

    if ( event->type == et_charup )
return( false );
    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return(false );

    if ( grc->ispopup && event->u.chr.keysym == GK_Return ) {
	GRowColDoubleClick(grc);
return( true );
    }

    if ( event->u.chr.keysym == GK_Return || event->u.chr.keysym == GK_Tab ||
	    event->u.chr.keysym == GK_BackTab || event->u.chr.keysym == GK_Escape )
return( false );

    GDrawCancelTimer(grc->enduser); grc->enduser = NULL; grc->sofar_pos = 0;

    loff = 0x80000000; xoff = 0x80000000; sel = -1;
    if ( keysym == GK_Home || keysym == GK_KP_Home || keysym == GK_Begin || keysym == GK_KP_Begin ) {
	loff = -grc->loff;
	xoff = -grc->xoff;
	sel = 0;
    } else if ( keysym == GK_End || keysym == GK_KP_End ) {
	loff = GRowColTopInWindow(grc,grc->ltot-1)-grc->loff;
	xoff = -grc->xoff;
	sel = grc->ltot-1;
    } else if ( keysym == GK_Up || keysym == GK_KP_Up ) {
	if (( sel = GRowColGetFirstSelPos(&grc->g)-1 )<0 ) {
	    /*if ( grc->loff!=0 ) loff = -1; else loff = 0;*/
	    sel = 0;
	}
    } else if ( keysym == GK_Down || keysym == GK_KP_Down ) {
	if (( sel = GRowColGetFirstSelPos(&grc->g))!= -1 )
	    ++sel;
	else
	    /*if ( grc->loff + GRowColLinesInWindow(grc,grc->loff)<grc->ltot ) loff = 1; else loff = 0;*/
	    sel = 0;
    } else if ( keysym == GK_Left || keysym == GK_KP_Left ) {
	xoff = -GDrawPointsToPixels(grc->g.base,6);
    } else if ( keysym == GK_Right || keysym == GK_KP_Right ) {
	xoff = GDrawPointsToPixels(grc->g.base,6);
    } else if ( keysym == GK_Page_Up || keysym == GK_KP_Page_Up ) {
	loff = GRowColTopInWindow(grc,grc->loff);
	if ( loff == grc->loff )		/* Normally we leave one line in window from before, except if only one line fits */
	    loff = GRowColTopInWindow(grc,grc->loff-1);
	loff -= grc->loff;
	if (( sel = GRowColGetFirstSelPos(&grc->g))!= -1 ) {
	    if (( sel += loff )<0 ) sel = 0;
	}
    } else if ( keysym == GK_Page_Down || keysym == GK_KP_Page_Down ) {
	loff = GRowColLinesInWindow(grc,grc->loff)-1;
	if ( loff<=0 ) loff = 1;
	if ( loff + grc->loff >= grc->ltot )
	    loff = GRowColTopInWindow(grc,grc->ltot-1)-grc->loff;
	if (( sel = GRowColGetFirstSelPos(&grc->g))!= -1 ) {
	    if (( sel += loff )>=grc->ltot ) sel = grc->ltot-1;
	}
    } else if ( keysym == GK_BackSpace && grc->orderer ) {
	/* ordered lists may be reversed by typing backspace */
	grc->backwards = !grc->backwards;
	GRowColOrderIt(grc);
	sel = GRowColGetFirstSelPos(&grc->g);
	if ( sel!=-1 ) {
	    int top = GRowColTopInWindow(grc,grc->ltot-1);
	    grc->loff = sel-1;
	    if ( grc->loff > top )
		grc->loff = top;
	    if ( sel-1<0 )
		grc->loff = 0;
	}
	GScrollBarSetPos(&grc->vsb->g,grc->loff);
	_ggadget_redraw(&grc->g);
return( true );
    } else if ( event->u.chr.chars[0]!='\0' && grc->orderer ) {
	int len = u_strlen(event->u.chr.chars);
	if ( sofar_pos+len >= grc->sofar_max ) {
	    if ( grc->sofar_max == 0 )
		grc->sofar = malloc((grc->sofar_max = len+10) * sizeof(unichar_t));
	    else
		grc->sofar = realloc(grc->sofar,(grc->sofar_max = sofar_pos+len+10)*sizeof(unichar_t));
	}
	u_strcpy(grc->sofar+sofar_pos,event->u.chr.chars);
	grc->sofar_pos = sofar_pos + len;
	sel = GRowColFindPosition(grc,grc->sofar);
	grc->enduser = GDrawRequestTimer(grc->g.base,GRowColTypeTime,0,NULL);
    }

    if ( loff==0x80000000 && sel>=0 ) {
	if ( sel>=grc->ltot ) sel = grc->ltot-1;
	if ( sel<grc->loff ) loff = sel-grc->loff;
	else if ( sel>=grc->loff+GRowColLinesInWindow(grc,grc->loff) )
	    loff = sel-(grc->loff+GRowColLinesInWindow(grc,grc->loff)-1);
    } else
	sel = -1;
    if ( sel!=-1 ) {
	int wassel = grc->ti[sel]->selected;
	refresh = GRowColAnyOtherSels(grc,sel) || !wassel;
	GRowColSelectOne(&grc->g,sel);
	if ( refresh )
	    GRowColSelected(grc);
    }
    if ( loff!=0x80000000 || xoff!=0x80000000 ) {
	if ( loff==0x80000000 ) loff = 0;
	if ( xoff==0x80000000 ) xoff = 0;
	GRowColScrollBy(grc,loff,xoff);
    }
    if ( refresh )
	_ggadget_redraw(g);
    if ( loff!=0x80000000 || xoff!=0x80000000 || sel!=-1 )
return( true );

return( false );
}

static int growcol_timer(GGadget *g, GEvent *event) {
    GRowCol *grc = (GRowCol *) g;

    if ( event->u.timer.timer == grc->enduser ) {
	grc->enduser = NULL;
	grc->sofar_pos = 0;
return( true );
    } else if ( event->u.timer.timer == grc->pressed ) {
	GEvent e;
	e.type = et_mousemove;
	GDrawGetPointerPosition(g->base,&e);
	if ( e.u.mouse.x<g->inner.x || e.u.mouse.y <g->inner.y ||
		e.u.mouse.x >= g->inner.x + g->inner.width ||
		e.u.mouse.y >= g->inner.y + g->inner.height )
	    growcol_scroll_selbymouse(grc,&e);
return( true );
    }
return( false );
}

static int growcol_scroll(GGadget *g, GEvent *event) {
    int loff = 0;
    enum sb sbt = event->u.control.u.sb.type;
    GRowCol *grc = (GRowCol *) g;

    if ( sbt==et_sb_top )
	loff = -grc->loff;
    else if ( sbt==et_sb_bottom )
	loff = GRowColTopInWindow(grc,grc->ltot-1)-grc->loff;
    else if ( sbt==et_sb_up ) {
	if ( grc->loff!=0 ) loff = -1; else loff = 0;
    } else if ( sbt==et_sb_down ) {
	if ( grc->loff + GRowColLinesInWindow(grc,grc->loff)<grc->ltot ) loff = 1; else loff = 0;
    } else if ( sbt==et_sb_uppage ) {
	loff = GRowColTopInWindow(grc,grc->loff);
	if ( loff == grc->loff )		/* Normally we leave one line in window from before, except if only one line fits */
	    loff = GRowColTopInWindow(grc,grc->loff-1);
	loff -= grc->loff;
    } else if ( sbt==et_sb_downpage ) {
	loff = GRowColLinesInWindow(grc,grc->loff)-1;
	if ( loff<=0 ) loff = 1;
	if ( loff + grc->loff >= grc->ltot )
	    loff = GRowColTopInWindow(grc,grc->ltot-1)-grc->loff;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	loff = event->u.control.u.sb.pos - grc->loff;
    }
    GRowColScrollBy(grc,loff,0);
return( true );
}

static void GRowCol_destroy(GGadget *g) {
    GRowCol *grc = (GRowCol *) g;

    if ( grc==NULL )
return;
    GDrawCancelTimer(grc->enduser);
    GDrawCancelTimer(grc->pressed);
    if ( grc->freeti )
	GTextInfoArrayFree(grc->ti);
    free(grc->sofar);
    if ( grc->vsb!=NULL )
	(grc->vsb->g.funcs->destroy)(&grc->vsb->g);
    _ggadget_destroy(g);
}

static void GRowColSetFont(GGadget *g,FontInstance *new) {
    GRowCol *b = (GRowCol *) g;
    b->font = new;
}

static FontInstance *GRowColGetFont(GGadget *g) {
    GRowCol *b = (GRowCol *) g;
return( b->font );
}

static void growcol_redraw(GGadget *g) {
    GRowCol *grc = (GRowCol *) g;
    if ( grc->vsb!=NULL )
	_ggadget_redraw((GGadget *) (grc->vsb));
    _ggadget_redraw(g);
}

static void growcol_move(GGadget *g, int32 x, int32 y ) {
    GRowCol *grc = (GRowCol *) g;
    if ( grc->vsb!=NULL )
	_ggadget_move((GGadget *) (grc->vsb),x+(grc->vsb->g.r.x-g->r.x),y);
    _ggadget_move(g,x,y);
}

static void growcol_resize(GGadget *g, int32 width, int32 height ) {
    GRowCol *grc = (GRowCol *) g;
    if ( grc->vsb!=NULL ) {
	int oldwidth = grc->vsb->g.r.x+grc->vsb->g.r.width-g->r.x;
	_ggadget_move((GGadget *) (grc->vsb),grc->vsb->g.r.x+width-oldwidth,grc->vsb->g.r.y);
	_ggadget_resize(g,width-(oldwidth-g->r.width), height);
	_ggadget_resize((GGadget *) (grc->vsb),grc->vsb->g.r.width,height);
    } else
	_ggadget_resize(g,width,height);
}

static GRect *growcol_getsize(GGadget *g, GRect *r ) {
    GRowCol *grc = (GRowCol *) g;
    _ggadget_getsize(g,r);
    if ( grc->vsb!=NULL )
	r->width =  grc->vsb->g.r.x+grc->vsb->g.r.width-g->r.x;
return( r );
}

static void growcol_setvisible(GGadget *g, int visible ) {
    GRowCol *grc = (GRowCol *) g;
    if ( grc->vsb!=NULL ) _ggadget_setvisible(&grc->vsb->g,visible);
    _ggadget_setvisible(g,visible);
}

static void growcol_setenabled(GGadget *g, int enabled ) {
    GRowCol *grc = (GRowCol *) g;
    if ( grc->vsb!=NULL ) _ggadget_setenabled(&grc->vsb->g,enabled);
    _ggadget_setenabled(g,enabled);
}

struct gfuncs GRowCol_funcs = {
    0,
    sizeof(struct gfuncs),

    growcol_expose,
    growcol_mouse,
    growcol_key,
    NULL,
    NULL,
    growcol_timer,
    NULL,

    growcol_redraw,
    growcol_move,
    growcol_resize,
    growcol_setvisible,
    growcol_setenabled,
    growcol_getsize,
    _ggadget_getinnersize,

    GRowCol_destroy,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    GRowColSetFont,
    GRowColGetFont,

    GRowColClear,
    GRowColSetList,
    GRowColGetList,
    GRowColGetListItem,
    GRowColSelect,
    GRowColSelectOne,
    GRowColIsItemSelected,
    GRowColGetFirstSelPos,
    GRowColShowPos,
    GRowColScrollToText,
    GRowColSetOrderer,

    NULL,
    NULL,
    NULL,
    NULL
};

static GBox list_box = { bt_lowered, bs_rect, 2, 2, 3, box_foreground_border_outer, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static FontInstance *list_font = NULL;
static int growcol_inited = false;

static void GRowColInit() {
    list_box.main_background = 0xc0c0c0;
    list_font = _GGadgetInitDefaultBox("GRowCol.",&list_box,NULL);
    growcol_inited = true;
}

static void GRowColFit(GRowCol *grc) {
    int width=0, height=0;
    int bp = GBoxBorderWidth(grc->g.base,grc->g.box);
    int i;

    /* can't deal with eliptical scrolling lists nor diamond ones. Just rects and roundrects */
    GRowColFindXMax(grc);
    if ( grc->g.r.width==0 ) {
	if ( width==0 ) width = GDrawPointsToPixels(grc->g.base,100);
	width += GDrawPointsToPixels(grc->g.base,_GScrollBar_Width) +
		GDrawPointsToPixels(grc->g.base,1);
	grc->g.r.width = width + 2*bp;
    }
    if ( grc->g.r.height==0 ) {
	for ( i=0; i<grc->ltot && i<5; ++i ) {
	    height += GTextInfoGetHeight(grc->g.base,grc->ti[i],grc->font);
	}
	if ( i<5 ) {
	    int as, ds, ld;
	    GDrawWindowFontMetrics(grc->g.base,grc->font,&as, &ds, &ld);
	    height += (5-i)*(as+ds);
	}
	grc->g.r.height = height + 2*bp;
    }
    grc->g.inner = grc->g.r;
    grc->g.inner.x += bp; grc->g.inner.y += bp;
    grc->g.inner.width -= 2*bp; grc->g.inner.height -= 2*bp;
    GRowColCheckSB(grc);
}

static GRowCol *_GRowColCreate(GRowCol *grc, struct gwindow *base, GGadgetData *gd,void *data, GBox *def) {
    int same;

    if ( !growcol_inited )
	GRowColInit();
    grc->g.funcs = &GRowCol_funcs;
    _GGadget_Create(&grc->g,base,gd,data,def);
    grc->font = list_font;
    grc->g.takes_input = grc->g.takes_keyboard = true; grc->g.focusable = true;

    if ( !(gd->flags & gg_list_internal ) ) {
	grc->ti = GTextInfoArrayFromList(gd->u.list,&grc->ltot);
	grc->freeti = true;
    } else {
	grc->ti = (GTextInfo **) (gd->u.list);
	grc->ltot = GTextInfoArrayCount(grc->ti);
    }
    grc->hmax = GTextInfoGetMaxHeight(grc->g.base,grc->ti,grc->font,&same);
    grc->sameheight = same;
    if ( gd->flags & gg_list_alphabetic ) {
	grc->orderer = GRowColAlphaCompare;
	GRowColOrderIt(grc);
    }
    grc->start = grc->end = -1;
    if ( gd->flags & gg_list_multiplesel )
	grc->multiple_sel = true;
    else if ( gd->flags & gg_list_exactlyone ) {
	int sel = GRowColGetFirstSelPos(&grc->g);
	grc->exactly_one = true;
	if ( sel==-1 ) sel = 0;
	GRowColClearSel(grc);
	if ( grc->ltot>0 ) grc->ti[sel]->selected = true;
    }

    GRowColFit(grc);
    _GGadget_FinalPosition(&grc->g,base,gd);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&grc->g);
    GWidgetIndicateFocusGadget(&grc->g);
return( grc );
}

GGadget *GRowColCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GRowCol *grc = _GRowColCreate(calloc(1,sizeof(GRowCol)),base,gd,data,&list_box);

return( &grc->g );
}
