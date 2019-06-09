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
#include "gkeysym.h"
#include "gwidget.h"
#include "ustring.h"

static int GListTypeTime = 500;			/* half a second between keystrokes */
static int GListScrollTime = 500;		/* half a second between scrolls when mouse out of listbox */

static void GListSelected(GList *l,int frommouse,int index) {
    GEvent e;

    e.type = et_controlevent;
    e.w = l->g.base;
    e.u.control.subtype = et_listselected;
    e.u.control.g = &l->g;
    e.u.control.u.list.from_mouse = frommouse;
    e.u.control.u.list.changed_index = index;
    if ( l->g.handle_controlevent != NULL )
	(l->g.handle_controlevent)(&l->g,&e);
    else
	GDrawPostEvent(&e);
}

static void GListDoubleClick(GList *l,int frommouse,int index) {
    GEvent e;

    e.type = et_controlevent;
    e.w = l->g.base;
    e.u.control.subtype = et_listdoubleclick;
    e.u.control.g = &l->g;
    e.u.control.u.list.from_mouse = frommouse;
    e.u.control.u.list.changed_index = index;
    if ( l->g.handle_controlevent != NULL )
	(l->g.handle_controlevent)(&l->g,&e);
    else
	GDrawPostEvent(&e);
}

static void GListClose(GList *l) {
    GEvent e;

    e.type = et_close;
    e.w = l->g.base;
    if ( l->g.handle_controlevent != NULL )
	(l->g.handle_controlevent)(&l->g,&e);
    else
	GDrawPostEvent(&e);
}

static int GListTopInWindow(GList *gl,int last) {
    /* If we want to display last at the bottom of our list, then what line */
    /*  do we display at the top? */
    int32 height = gl->g.inner.height, temp;
    int l;

    for ( l=last; l>=0; --l ) {
	temp = GTextInfoGetHeight(gl->g.base,gl->ti[l],gl->font);
	if ( height<temp )
return( l==last?last:l+1 );		/* if we can't even fit one line on, pretend it fits */
	height -= temp;
    }
return( 0 );
}

static int GListLinesInWindow(GList *gl,int first) {
    /* Ok, the first line displayed is "first", how many others do we have */
    /*  room for? */
    int32 height = gl->g.inner.height, temp;
    int l, lcnt=0;

    for ( l=first; l<gl->ltot; ++l ) {
	temp = GTextInfoGetHeight(gl->g.base,gl->ti[l],gl->font);
	if ( height<temp )
return( l==first?1:lcnt );		/* if we can't even fit one line on, pretend it fits */
	height -= temp;
	++lcnt;
    }
    if ( height>0 ) {
	if ( gl->fh==0 ) {
	    int as, ds, ld;
	    GDrawWindowFontMetrics(gl->g.base,gl->font,&as, &ds, &ld);
	    gl->fh = as+ds;
	    gl->as = as;
	}
	lcnt += height/gl->fh;
    }
    if ( lcnt==0 )
	lcnt=1;
return( lcnt );
}

static int GListAlphaCompare(const void *v1, const void *v2) {
    GTextInfo * const *pt1 = v1, * const *pt2 = v2;
return( GTextInfoCompare(*pt1,*pt2));
}

static void GListOrderIt(GList *gl) {
    qsort(gl->ti,gl->ltot,sizeof(GTextInfo *),gl->orderer);
    if ( gl->backwards ) {
	int i;
	GTextInfo *ti;
	for ( i=0; i<gl->ltot/2; ++i ) {
	    ti = gl->ti[i];
	    gl->ti[i] = gl->ti[gl->ltot-1-i];
	    gl->ti[gl->ltot-1-i] = ti;
	}
    }
}

static void GListClearSel(GList *gl) {
    int i;

    for ( i=0; (i<gl->ltot && gl->ti[i]!=NULL); ++i )
	gl->ti[i]->selected = false;
}

static int GListAnyOtherSels(GList *gl, int pos) {
    int i;

    for ( i=0; i<gl->ltot; ++i )
	if ( gl->ti[i]->selected && i!=pos )
return( true );

return( false );
}

static int32 GListGetFirstSelPos(GGadget *g) {
    int i;
    GList *gl = (GList *) g;

    for ( i=0; i<gl->ltot; ++i )
	if ( gl->ti[i]->selected )
return( i );

return( -1 );
}

static void GListSelect(GGadget *g, int32 pos, int32 sel) {
    GList *gl = (GList *) g;
    int i;

    if ( pos==-1 && (gl->multiple_sel || (!sel && !gl->exactly_one)) ) {
	/* Select/deselect all */
	for ( i=0; i<gl->ltot; ++i )
	    gl->ti[i]->selected = sel;
	_ggadget_redraw(g);
return;
    }

    if ( pos>=gl->ltot || pos<0 )
return;
    if ( gl->exactly_one && !sel )
return;
    if ( !gl->multiple_sel && sel )
	GListClearSel(gl);
    if ( gl->ltot>0 ) {
	gl->ti[pos]->selected = sel;
	_ggadget_redraw(g);
    }
}

static void GListSelectOne(GGadget *g, int32 pos) {
    GList *gl = (GList *) g;

    GListClearSel(gl);
    if ( pos>=gl->ltot ) pos = gl->ltot-1;
    if ( pos<0 ) pos = 0;
    if ( gl->ltot>0 ) {
	gl->ti[pos]->selected = true;
	_ggadget_redraw(g);
    }
}

static int32 GListIsItemSelected(GGadget *g, int32 pos) {
    GList *gl = (GList *) g;

    if ( pos>=gl->ltot )
return( false );
    if ( pos<0 )
return( false );
    if ( gl->ltot>0 )
return( gl->ti[pos]->selected );

return( false );
}

static void GListExpandSelection(GList *gl,int pos) {
    int i;

    if ( gl->start!=65535 ) {
	if ( gl->start<gl->end )
	    for ( i=gl->start; i<=gl->end; ++i )
		gl->ti[i]->selected = false;
	else
	    for ( i=gl->start; i>=gl->end; --i )
		gl->ti[i]->selected = false;
    } else
	gl->start = pos;
    gl->end = pos;
    if ( gl->start<gl->end )
	for ( i=gl->start; i<=gl->end; ++i )
	    gl->ti[i]->selected = true;
    else
	for ( i=gl->start; i>=gl->end; --i )
	    gl->ti[i]->selected = true;
}

static int GListIndexFromPos(GList *gl,int y) {
    int i, height;

    y -= gl->g.inner.y;
    if ( y<0 ) y=0;
    if ( y>=gl->g.inner.height ) y = gl->g.inner.height-1;
    for ( i=gl->loff, height=0; i<gl->ltot; ++i ) {
	int temp = GTextInfoGetHeight(gl->g.base,gl->ti[i],gl->font);
	if ( height+temp>y )
    break;
	height += temp;
    }
    if ( i==gl->ltot )
return( -1 );
    if ( gl->ti[i]->disabled )
return( -1 );
return( i );
}

static void GListScrollBy(GList *gl,int loff,int xoff) {
    int top = GListTopInWindow(gl,gl->ltot-1);
    int ydiff, i;

    if ( gl->loff + loff < 0 )
	loff = -gl->loff;
    else if ( gl->loff + loff > top )
	loff = top-gl->loff;
    if ( xoff+gl->xoff<0 )
	xoff = -gl->xoff;
    else if ( xoff+gl->xoff+gl->g.inner.width > gl->xmax ) {
	xoff = gl->xmax-gl->g.inner.width-gl->xoff;
	if ( xoff<0 ) xoff = 0;
    }
    if ( loff == 0 && xoff==0 )
return;

    ydiff = 0;
    if ( loff>0 ) {
	for ( i=0; i<loff && ydiff<gl->g.inner.height; ++i )
	    ydiff += GTextInfoGetHeight(gl->g.base,gl->ti[i+gl->loff],gl->font);
    } else if ( loff<0 ) {
	for ( i=loff; i<0 && -ydiff<gl->g.inner.height; ++i )
	    ydiff -= GTextInfoGetHeight(gl->g.base,gl->ti[i+gl->loff],gl->font);
    }
    if ( !GDrawIsVisible(gl->g.base))
return;
    GDrawForceUpdate(gl->g.base);
    gl->loff += loff; gl->xoff += xoff;
    if ( ydiff>=gl->g.inner.height || -ydiff >= gl->g.inner.height )
	_ggadget_redraw(&gl->g);
    else if ( ydiff!=0 || xoff!=0 )
	GDrawScroll(gl->g.base,&gl->g.inner,xoff,ydiff);
    if ( loff!=0 && gl->vsb!=NULL )
	GScrollBarSetPos(&gl->vsb->g,gl->loff);
}

static int GListFindPosition(GList *gl,unichar_t *text) {
    GTextInfo temp, *ptemp=&temp;
    int i, order;

    if ( gl->orderer!=NULL ) {
	memset(&temp,'\0',sizeof(temp));
	temp.text = text;

	/* I don't think I need to write a binary search here... */
	for ( i=0; i<gl->ltot; ++i ) {
	    order = (gl->orderer)(&ptemp,&gl->ti[i]);
	    if (( order<= 0 && !gl->backwards ) || ( order>=0 && gl->backwards ))
return( i );
	}
return( 0 );
    } else {
	for ( i=0; i<gl->ltot; ++i ) {
	    if (u_strmatch(text,gl->ti[i]->text)==0 )
return( i );
	}
    }
return( 0 );
}

static int GListAdjustPos(GGadget *g,int pos) {
    GList *gl = (GList *) g;
    int newoff = gl->loff;

    if ( pos<gl->loff ) {
	if (( newoff = pos-1)<0 ) newoff = 0;
	if ( GListLinesInWindow(gl,newoff)<2 )
	    newoff = pos;
    } else if ( pos >= gl->loff + GListLinesInWindow(gl,gl->loff) ) {
	newoff = GListTopInWindow(gl,pos);
	if ( pos!=gl->ltot-1 && GListLinesInWindow(gl,newoff+1)>=2 )
	    ++newoff;
    }
return( newoff );
}

static void GListShowPos(GGadget *g,int32 pos) {
    GList *gl = (GList *) g;
    int newoff = GListAdjustPos(g,pos);
    if ( newoff!=gl->loff )
	GListScrollBy(gl,newoff-gl->loff,0);
}

static void GListScrollToText(GGadget *g,const unichar_t *text,int32 sel) {
    GList *gl = (GList *) g;
    int pos;

    pos = GListFindPosition(gl,(unichar_t *) text);
    if ( sel && pos<gl->ltot ) {
	GListClearSel(gl);
	if ( gl->exactly_one || u_strmatch(text,gl->ti[pos]->text)==0 )
	    gl->ti[pos]->selected = true;
    }
    gl->loff = GListAdjustPos(g,pos);
    if ( gl->vsb!=NULL )
	GScrollBarSetPos(&gl->vsb->g,gl->loff);
    _ggadget_redraw(g);
}

static void GListSetOrderer(GGadget *g,int (*orderer)(const void *, const void *)) {
    GList *gl = (GList *) g;

    gl->orderer = orderer;
    if ( orderer!=NULL ) {
	GListOrderIt(gl);
	GListScrollBy(gl,-gl->loff,-gl->xoff);
	_ggadget_redraw(&gl->g);
    }
}

static int glist_scroll(GGadget *g, GEvent *event);
static void GListCheckSB(GList *gl) {
    if ( gl->vsb==NULL ) {
	GGadgetData gd;
	memset(&gd,'\0',sizeof(gd));
	gd.pos.y = gl->g.r.y; gd.pos.height = gl->g.r.height;
	gd.pos.width = GDrawPointsToPixels(gl->g.base,_GScrollBar_Width);
	gd.pos.x = gl->g.r.x+gl->g.r.width - gd.pos.width;
	gd.flags = gg_visible|gg_enabled|gg_pos_in_pixels|gg_sb_vert|gg_pos_use0;
	gd.handle_controlevent = glist_scroll;
	gl->vsb = (GScrollBar *) GScrollBarCreate(gl->g.base,&gd,gl);
	gl->vsb->g.contained = true;

	gd.pos.width += GDrawPointsToPixels(gl->g.base,1);
	gl->g.r.width -= gd.pos.width;
	gl->g.inner.width -= gd.pos.width;
    }
    if ( gl->always_show_sb || GListLinesInWindow(gl,0)<gl->ltot ) {
	if ( gl->vsb->g.state == gs_invisible ) {
	    int wid = gl->vsb->g.r.width + GDrawPointsToPixels(gl->g.base,1);
	    gl->vsb->g.state = gs_enabled;
	    gl->g.r.width -= wid;
	    gl->g.inner.width -= wid;
	}
	GScrollBarSetBounds(&gl->vsb->g,0,gl->ltot,GListLinesInWindow(gl,0));
	GScrollBarSetPos(&gl->vsb->g,gl->loff);
    } else {
	if ( gl->vsb->g.state != gs_invisible ) {
	    int wid = gl->vsb->g.r.width + GDrawPointsToPixels(gl->g.base,1);
	    gl->vsb->g.state = gs_invisible;
	    gl->g.r.width += wid;
	    gl->g.inner.width += wid;
	}
    }
}

static int GListFindXMax(GList *gl) {
    int i, width=0, temp;

    for ( i=0; i<gl->ltot; ++i ) {
	temp = GTextInfoGetWidth(gl->g.base,gl->ti[i],gl->font);
	if ( temp>width ) width=temp;
    }
    gl->xmax = width;
return( width );
}

static void GListSetList(GGadget *g,GTextInfo **ti,int32 docopy) {
    GList *gl = (GList *) g;
    int same;

    GTextInfoArrayFree(gl->ti);
    if ( docopy || ti==NULL )
	ti = GTextInfoArrayCopy(ti);
    gl->ti = ti;
    gl->ltot = GTextInfoArrayCount(ti);
    if ( gl->orderer!=NULL )
	GListOrderIt(gl);
    gl->loff = gl->xoff = 0;
    gl->hmax = GTextInfoGetMaxHeight(g->base,ti,gl->font,&same);
    gl->sameheight = same;
    GListCheckSB(gl);
    _ggadget_redraw(&gl->g);
}

static void GListClear(GGadget *g) {
    GListSetList(g,NULL,true);
}

static GTextInfo **GListGetList(GGadget *g,int32 *len) {
    GList *gl = (GList *) g;
    if ( len!=NULL ) *len = gl->ltot;
return( gl->ti );
}

static GTextInfo *GListGetListItem(GGadget *g,int32 pos) {
    GList *gl = (GList *) g;
    if ( pos<0 || pos>=gl->ltot )
return( NULL );

return(gl->ti[pos]);
}

static int glist_expose(GWindow pixmap, GGadget *g, GEvent *event) {
    GList *gl = (GList *) g;
    GRect old0, old1, old2;
    Color fg, dfg;
    int y, l, ymax;

    if ( g->state == gs_invisible )
return( false );

    GDrawPushClip(pixmap,&event->u.expose.rect,&old0);
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
    for ( l = gl->loff; y<ymax && l<gl->ltot; ++l ) {
	if ( y+gl->hmax > event->u.expose.rect.y )
	    y += GTextInfoDraw(pixmap,g->inner.x-gl->xoff,y,gl->ti[l],
		    gl->font,gl->ti[l]->disabled?dfg:fg,g->box->active_border,
		    ymax);
	else if ( gl->sameheight )
	    y += gl->hmax;
	else
	    y += GTextInfoGetHeight(pixmap,gl->ti[l],gl->font);
    }
    if ( g->box->border_type!=bt_none ||
	    (g->box->flags&(box_foreground_border_inner|box_foreground_border_outer|box_active_border_inner))!=0 )
	GDrawPopClip(pixmap,&old2);
    GDrawPopClip(pixmap,&old1);
    GDrawPopClip(pixmap,&old0);
return( true );
}

static void glist_scroll_selbymouse(GList *gl, GEvent *event) {
    int loff=0, xoff=0, pos;

    if ( event->u.mouse.y<gl->g.inner.y ) {
	if ( gl->loff>0 ) loff = -1;
    } else if ( event->u.mouse.y >= gl->g.inner.y+gl->g.inner.height ) {
	int top = GListTopInWindow(gl,gl->ltot-1);
	if ( gl->loff<top ) loff = 1;
    }
    if ( event->u.mouse.x<gl->g.inner.x ) {
	xoff = -GDrawPointsToPixels(gl->g.base,6);
    } else if ( event->u.mouse.x >= gl->g.inner.x+gl->g.inner.width ) {
	xoff = GDrawPointsToPixels(gl->g.base,6);
    }
    GListScrollBy(gl,loff,xoff);
    pos = GListIndexFromPos(gl,event->u.mouse.y);
    if ( pos==-1 || pos == gl->end )
	/* Do Nothing, nothing selectable */;
    else if ( !gl->multiple_sel ) {
	GListClearSel(gl);
	gl->ti[pos]->selected = true;
	gl->start = gl->end = pos;
	_ggadget_redraw(&gl->g);
    } else {
	GListExpandSelection(gl,pos);
	gl->end = pos;
	_ggadget_redraw(&gl->g);
    }
}

static int glist_mouse(GGadget *g, GEvent *event) {
    GList *gl = (GList *) g;
    int pos;

    if ( !g->takes_input || (g->state!=gs_active && g->state!=gs_enabled && g->state!=gs_focused))
return( false );

    if ( event->type == et_crossing )
return( false );
    if (( event->type==et_mouseup || event->type==et_mousedown ) &&
	    (event->u.mouse.button>=4 && event->u.mouse.button<=7)) {
	if ( gl->vsb!=NULL )
return( GGadgetDispatchEvent(&gl->vsb->g,event));
	else
return( true );
    }
    if ( event->type==et_mousemove && !gl->pressed && !gl->parentpressed ) {
	if ( GGadgetWithin(g,event->u.mouse.x,event->u.mouse.y) ) {
	    if ( gl->popup_callback!=NULL )
		(gl->popup_callback)(g,GListIndexFromPos(gl,event->u.mouse.y));
	    else if ( g->popup_msg )
		GGadgetPreparePopup(g->base,g->popup_msg);
	}
return( true );
    } else if ( event->type==et_mouseup && gl->parentpressed /* &&
	    !GGadgetInnerWithin(&gl->g,event->u.mouse.x,event->u.mouse.y)*/ ) {
	gl->parentpressed = false;
	GDrawPointerUngrab(GDrawGetDisplayOfWindow(gl->g.base));
    } else if ( event->type==et_mousemove && gl->parentpressed &&
	    GGadgetInnerWithin(&gl->g,event->u.mouse.x,event->u.mouse.y)) {
	if ( gl->pressed == NULL )
	    gl->pressed = GDrawRequestTimer(g->base,GListScrollTime,GListScrollTime,NULL);
	GDrawPointerUngrab(GDrawGetDisplayOfWindow(gl->g.base));
	gl->parentpressed = false;
	glist_scroll_selbymouse(gl,event);
return( true );
    } else if ( event->type==et_mousemove && gl->pressed ) {
	glist_scroll_selbymouse(gl,event);
return( true );
    } else if ( event->type==et_mousedown ) {
	if ( gl->pressed == NULL )
	    gl->pressed = GDrawRequestTimer(g->base,GListScrollTime,GListScrollTime,NULL);
	pos = GListIndexFromPos(gl,event->u.mouse.y);
	if ( pos==-1 )
return( true ); /* Do Nothing, nothing selectable */
	else if ( !gl->exactly_one && gl->ti[pos]->selected &&
		(event->u.mouse.state&(ksm_control|ksm_shift))) {
	    gl->ti[pos]->selected = false;
	    gl->start = gl->end = 0xffff;
	} else if ( !gl->multiple_sel ||
		(!gl->ti[pos]->selected && !(event->u.mouse.state&(ksm_control|ksm_shift)))) {
	    GListClearSel(gl);
	    gl->ti[pos]->selected = true;
	    gl->start = gl->end = pos;
	} else if ( event->u.mouse.state&ksm_control ||
		((event->u.mouse.state&ksm_shift) && gl->ti[pos]->selected)) {
	    gl->ti[pos]->selected = !gl->ti[pos]->selected;
	    gl->start = gl->end = pos;
	} else if ( event->u.mouse.state&ksm_shift ) {
	    GListExpandSelection(gl,pos);
	} else {
	    gl->ti[pos]->selected = true;
	    gl->start = gl->end = pos;
	}
	_ggadget_redraw(&gl->g);
    } else if ( event->type==et_mouseup && gl->pressed ) {
	GDrawCancelTimer(gl->pressed); gl->pressed = NULL;
	if ( GGadgetInnerWithin(&gl->g,event->u.mouse.x,event->u.mouse.y) ) {
	    pos = GListIndexFromPos(gl,event->u.mouse.y);
	    if ( !(event->u.mouse.state&(ksm_control|ksm_shift)) || gl->start!=0xffff )
		glist_scroll_selbymouse(gl,event);
	    if ( event->u.mouse.clicks==2 )
		GListDoubleClick(gl,true,pos);
	    else
		GListSelected(gl,true,pos);
	}
    } else
return( false );

return( true );
}

static int glist_key(GGadget *g, GEvent *event) {
    GList *gl = (GList *) g;
    uint16 keysym = event->u.chr.keysym;
    int sofar_pos = gl->sofar_pos;
    int loff, xoff, sel=-1;
    int refresh = false;

    if ( event->type == et_charup )
return( false );
    if ( !g->takes_input || (g->state!=gs_enabled && g->state!=gs_active && g->state!=gs_focused ))
return(false );

    if ( gl->ispopup && event->u.chr.keysym == GK_Return ) {
	GListDoubleClick(gl,false,-1);
return( true );
    } else if ( gl->ispopup && event->u.chr.keysym == GK_Escape ) {
	GListClose(gl);
return( true );
    }

    if ( event->u.chr.keysym == GK_Return || event->u.chr.keysym == GK_Tab ||
	    event->u.chr.keysym == GK_BackTab || event->u.chr.keysym == GK_Escape )
return( false );

    GDrawCancelTimer(gl->enduser); gl->enduser = NULL; gl->sofar_pos = 0;

    loff = 0x80000000; xoff = 0x80000000; sel = -1;
    if ( keysym == GK_Home || keysym == GK_KP_Home || keysym == GK_Begin || keysym == GK_KP_Begin ) {
	loff = -gl->loff;
	xoff = -gl->xoff;
	sel = 0;
    } else if ( keysym == GK_End || keysym == GK_KP_End ) {
	loff = GListTopInWindow(gl,gl->ltot-1)-gl->loff;
	xoff = -gl->xoff;
	sel = gl->ltot-1;
    } else if ( keysym == GK_Up || keysym == GK_KP_Up ) {
	if (( sel = GListGetFirstSelPos(&gl->g)-1 )<0 ) {
	    /*if ( gl->loff!=0 ) loff = -1; else loff = 0;*/
	    sel = 0;
	}
    } else if ( keysym == GK_Down || keysym == GK_KP_Down ) {
	if (( sel = GListGetFirstSelPos(&gl->g))!= -1 )
	    ++sel;
	else
	    /*if ( gl->loff + GListLinesInWindow(gl,gl->loff)<gl->ltot ) loff = 1; else loff = 0;*/
	    sel = 0;
    } else if ( keysym == GK_Left || keysym == GK_KP_Left ) {
	xoff = -GDrawPointsToPixels(gl->g.base,6);
    } else if ( keysym == GK_Right || keysym == GK_KP_Right ) {
	xoff = GDrawPointsToPixels(gl->g.base,6);
    } else if ( keysym == GK_Page_Up || keysym == GK_KP_Page_Up ) {
	loff = GListTopInWindow(gl,gl->loff);
	if ( loff == gl->loff )		/* Normally we leave one line in window from before, except if only one line fits */
	    loff = GListTopInWindow(gl,gl->loff-1);
	loff -= gl->loff;
	if (( sel = GListGetFirstSelPos(&gl->g))!= -1 ) {
	    if (( sel += loff )<0 ) sel = 0;
	}
    } else if ( keysym == GK_Page_Down || keysym == GK_KP_Page_Down ) {
	loff = GListLinesInWindow(gl,gl->loff)-1;
	if ( loff<=0 ) loff = 1;
	if ( loff + gl->loff >= gl->ltot )
	    loff = GListTopInWindow(gl,gl->ltot-1)-gl->loff;
	if (( sel = GListGetFirstSelPos(&gl->g))!= -1 ) {
	    if (( sel += loff )>=gl->ltot ) sel = gl->ltot-1;
	}
    } else if ( keysym == GK_BackSpace && gl->orderer ) {
	/* ordered lists may be reversed by typing backspace */
	gl->backwards = !gl->backwards;
	GListOrderIt(gl);
	sel = GListGetFirstSelPos(&gl->g);
	if ( sel!=-1 ) {
	    int top = GListTopInWindow(gl,gl->ltot-1);
	    gl->loff = sel-1;
	    if ( gl->loff > top )
		gl->loff = top;
	    if ( sel-1<0 )
		gl->loff = 0;
	}
	GScrollBarSetPos(&gl->vsb->g,gl->loff);
	_ggadget_redraw(&gl->g);
return( true );
    } else if ( event->u.chr.chars[0]!='\0' && gl->orderer ) {
	int len = u_strlen(event->u.chr.chars);
	if ( sofar_pos+len >= gl->sofar_max ) {
	    if ( gl->sofar_max == 0 )
		gl->sofar = malloc((gl->sofar_max = len+10) * sizeof(unichar_t));
	    else
		gl->sofar = realloc(gl->sofar,(gl->sofar_max = sofar_pos+len+10)*sizeof(unichar_t));
	}
	u_strcpy(gl->sofar+sofar_pos,event->u.chr.chars);
	gl->sofar_pos = sofar_pos + len;
	sel = GListFindPosition(gl,gl->sofar);
	gl->enduser = GDrawRequestTimer(gl->g.base,GListTypeTime,0,NULL);
    }

    if ( loff==0x80000000 && sel>=0 ) {
	if ( sel>=gl->ltot ) sel = gl->ltot-1;
	if ( sel<gl->loff ) loff = sel-gl->loff;
	else if ( sel>=gl->loff+GListLinesInWindow(gl,gl->loff) )
	    loff = sel-(gl->loff+GListLinesInWindow(gl,gl->loff)-1);
    } else
	sel = -1;
    if ( sel!=-1 ) {
	int wassel = gl->ti[sel]->selected;
	refresh = GListAnyOtherSels(gl,sel) || !wassel;
	GListSelectOne(&gl->g,sel);
	if ( refresh )
	    GListSelected(gl,false,sel);
    }
    if ( loff!=0x80000000 || xoff!=0x80000000 ) {
	if ( loff==0x80000000 ) loff = 0;
	if ( xoff==0x80000000 ) xoff = 0;
	GListScrollBy(gl,loff,xoff);
    }
    if ( refresh )
	_ggadget_redraw(g);
    if ( loff!=0x80000000 || xoff!=0x80000000 || sel!=-1 )
return( true );

return( false );
}

static int glist_timer(GGadget *g, GEvent *event) {
    GList *gl = (GList *) g;

    if ( event->u.timer.timer == gl->enduser ) {
	gl->enduser = NULL;
	gl->sofar_pos = 0;
return( true );
    } else if ( event->u.timer.timer == gl->pressed ) {
	GEvent e;
	e.type = et_mousemove;
	GDrawGetPointerPosition(g->base,&e);
	if ( e.u.mouse.x<g->inner.x || e.u.mouse.y <g->inner.y ||
		e.u.mouse.x >= g->inner.x + g->inner.width ||
		e.u.mouse.y >= g->inner.y + g->inner.height )
	    glist_scroll_selbymouse(gl,&e);
return( true );
    }
return( false );
}

static int glist_scroll(GGadget *g, GEvent *event) {
    int loff = 0;
    enum sb sbt = event->u.control.u.sb.type;
    GList *gl = (GList *) (g->data);

    g = (GGadget *) gl;

    if ( sbt==et_sb_top )
	loff = -gl->loff;
    else if ( sbt==et_sb_bottom )
	loff = GListTopInWindow(gl,gl->ltot-1)-gl->loff;
    else if ( sbt==et_sb_up ) {
	if ( gl->loff!=0 ) loff = -1; else loff = 0;
    } else if ( sbt==et_sb_down ) {
	if ( gl->loff + GListLinesInWindow(gl,gl->loff)<gl->ltot ) loff = 1; else loff = 0;
    } else if ( sbt==et_sb_uppage ) {
	loff = GListTopInWindow(gl,gl->loff);
	if ( loff == gl->loff )		/* Normally we leave one line in window from before, except if only one line fits */
	    loff = GListTopInWindow(gl,gl->loff-1);
	loff -= gl->loff;
    } else if ( sbt==et_sb_downpage ) {
	loff = GListLinesInWindow(gl,gl->loff)-1;
	if ( loff<=0 ) loff = 1;
	if ( loff + gl->loff >= gl->ltot )
	    loff = GListTopInWindow(gl,gl->ltot-1)-gl->loff;
    } else /* if ( sbt==et_sb_thumb || sbt==et_sb_thumbrelease ) */ {
	loff = event->u.control.u.sb.pos - gl->loff;
    }
    GListScrollBy(gl,loff,0);
return( true );
}

static void GList_destroy(GGadget *g) {
    GList *gl = (GList *) g;

    if ( gl==NULL )
return;
    GDrawCancelTimer(gl->enduser);
    GDrawCancelTimer(gl->pressed);
    if ( gl->freeti )
	GTextInfoArrayFree(gl->ti);
    free(gl->sofar);
    if ( gl->vsb!=NULL )
	(gl->vsb->g.funcs->destroy)(&gl->vsb->g);
    _ggadget_destroy(g);
}

static void GListSetFont(GGadget *g,FontInstance *new) {
    GList *gl = (GList *) g;
    int same;

    gl->font = new;
    gl->hmax = GTextInfoGetMaxHeight(gl->g.base,gl->ti,gl->font,&same);
    gl->sameheight = same;
}

static FontInstance *GListGetFont(GGadget *g) {
    GList *b = (GList *) g;
return( b->font );
}

static void glist_redraw(GGadget *g) {
    GList *gl = (GList *) g;
    if ( gl->vsb!=NULL )
	_ggadget_redraw((GGadget *) (gl->vsb));
    _ggadget_redraw(g);
}

static void glist_move(GGadget *g, int32 x, int32 y ) {
    GList *gl = (GList *) g;
    if ( gl->vsb!=NULL )
	_ggadget_move((GGadget *) (gl->vsb),x+(gl->vsb->g.r.x-g->r.x),y);
    _ggadget_move(g,x,y);
}

static void glist_resize(GGadget *g, int32 width, int32 height ) {
    GList *gl = (GList *) g;
    if ( gl->vsb!=NULL ) {
	int oldwidth = gl->vsb->g.r.x+gl->vsb->g.r.width-g->r.x;
	_ggadget_move((GGadget *) (gl->vsb),gl->vsb->g.r.x+width-oldwidth,gl->vsb->g.r.y);
	_ggadget_resize(g,width-(oldwidth-g->r.width), height);
	_ggadget_resize((GGadget *) (gl->vsb),gl->vsb->g.r.width,height);
	GListCheckSB(gl);
    } else
	_ggadget_resize(g,width,height);
}

static GRect *glist_getsize(GGadget *g, GRect *r ) {
    GList *gl = (GList *) g;
    _ggadget_getsize(g,r);
    if ( gl->vsb!=NULL )
	r->width =  gl->vsb->g.r.x+gl->vsb->g.r.width-g->r.x;
return( r );
}

static void glist_setvisible(GGadget *g, int visible ) {
    GList *gl = (GList *) g;
    if ( gl->vsb!=NULL ) _ggadget_setvisible(&gl->vsb->g,visible);
    _ggadget_setvisible(g,visible);
}

static void glist_setenabled(GGadget *g, int enabled ) {
    GList *gl = (GList *) g;
    if ( gl->vsb!=NULL ) _ggadget_setenabled(&gl->vsb->g,enabled);
    _ggadget_setenabled(g,enabled);
}

static void GListGetDesiredSize(GGadget *g,GRect *outer, GRect *inner) {
    GList *gl = (GList *) g;
    int width=0, height=0, temp;
    int bp = GBoxBorderWidth(gl->g.base,gl->g.box);
    int i;

    /* can't deal with eliptical scrolling lists nor diamond ones. Just rects and roundrects */
    if ( g->desired_width<=0 ) {
	GListFindXMax(gl);

	width = gl->xmax;
	temp = GDrawPointsToPixels(gl->g.base,50);
	if ( width<temp ) width = temp;
	width += GDrawPointsToPixels(gl->g.base,_GScrollBar_Width) +
		GDrawPointsToPixels(gl->g.base,1);
    } else
	width = g->desired_width - 2*bp;

    if ( g->desired_height<=0 ) {
	for ( i=0; i<gl->ltot && i<8; ++i ) {
	    height += GTextInfoGetHeight(gl->g.base,gl->ti[i],gl->font);
	}
	if ( i<4 ) {
	    int as, ds, ld;
	    GDrawWindowFontMetrics(g->base,gl->font,&as, &ds, &ld);
	    height += (4-i)*(as+ds);
	}
    } else
	height = g->desired_height - 2*bp;
    if ( inner!=NULL ) {
	inner->x = inner->y = 0;
	inner->width = width;
	inner->height = height;
    }
    if ( outer!=NULL ) {
	outer->x = outer->y = 0;
	outer->width = width + 2*bp;
	outer->height = height + 2*bp;
    }
}

static int glist_FillsWindow(GGadget *g) {
return( g->prev==NULL &&
	(_GWidgetGetGadgets(g->base)==g ||
	 _GWidgetGetGadgets(g->base)==(GGadget *) ((GList *) g)->vsb));
}

struct gfuncs GList_funcs = {
    0,
    sizeof(struct gfuncs),

    glist_expose,
    glist_mouse,
    glist_key,
    NULL,
    NULL,
    glist_timer,
    NULL,

    glist_redraw,
    glist_move,
    glist_resize,
    glist_setvisible,
    glist_setenabled,
    glist_getsize,
    _ggadget_getinnersize,

    GList_destroy,

    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    GListSetFont,
    GListGetFont,

    GListClear,
    GListSetList,
    GListGetList,
    GListGetListItem,
    GListSelect,
    GListSelectOne,
    GListIsItemSelected,
    GListGetFirstSelPos,
    GListShowPos,
    GListScrollToText,
    GListSetOrderer,

    GListGetDesiredSize,
    _ggadget_setDesiredSize,
    glist_FillsWindow,
    NULL
};

static GBox list_box = GBOX_EMPTY; /* Don't initialize here */;
static FontInstance *list_font = NULL;
static int glist_inited = false;

static GTextInfo list_choices[] = {
    { (unichar_t *) "1", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "2", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    { (unichar_t *) "3", NULL, 0, 0, NULL, NULL, 0, 0, 0, 0, 0, 0, 1, 0, 0, '\0' },
    GTEXTINFO_EMPTY
};
static GGadgetCreateData list_gcd[] = {
    { GListCreate, { { 0, 0, 0, 36 }, NULL, 0, 0, 0, 0, 0, &list_choices[0], { list_choices }, gg_visible, NULL, NULL }, NULL, NULL },
    { GListCreate, { { 0, 0, 0, 36 }, NULL, 0, 0, 0, 0, 0, &list_choices[0], { list_choices }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL }
};
static GGadgetCreateData *tarray[] = { GCD_Glue, &list_gcd[0], GCD_Glue, &list_gcd[1], GCD_Glue, NULL, NULL };
static GGadgetCreateData listhvbox =
    { GHVGroupCreate, { { 2, 2, 0, 0 }, NULL, 0, 0, 0, 0, 0, NULL, { (GTextInfo *) tarray }, gg_visible|gg_enabled, NULL, NULL }, NULL, NULL };
static GResInfo glist_ri = {
    NULL, &ggadget_ri,NULL, NULL,
    &list_box,
    &list_font,
    &listhvbox,
    NULL,
    N_("List"),
    N_("List"),
    "GList",
    "Gdraw",
    false,
    box_foreground_border_outer,
    NULL,
    GBOX_EMPTY,
    NULL,
    NULL,
    NULL
};

static void GListInit() {
    _GGadgetCopyDefaultBox(&list_box);
    list_box.flags |= box_foreground_border_outer;
    list_font = _GGadgetInitDefaultBox("GList.",&list_box,NULL);
    glist_inited = true;
}

static void GListFit(GList *gl) {
    int bp = GBoxBorderWidth(gl->g.base,gl->g.box);
    GRect inner, outer;

    GListGetDesiredSize(&gl->g,&outer,&inner);
    if ( gl->g.r.width==0 )
	gl->g.r.width = outer.width;
    if ( gl->g.r.height==0 )
	gl->g.r.height = outer.height;
    gl->g.inner = gl->g.r;
    gl->g.inner.x += bp; gl->g.inner.y += bp;
    gl->g.inner.width -= 2*bp; gl->g.inner.height -= 2*bp;
    GListCheckSB(gl);
}

static GList *_GListCreate(GList *gl, struct gwindow *base, GGadgetData *gd,void *data, GBox *def) {
    int same;

    if ( !glist_inited )
	GListInit();
    gl->g.funcs = &GList_funcs;
    _GGadget_Create(&gl->g,base,gd,data,def);
    gl->font = list_font;
    gl->g.takes_input = gl->g.takes_keyboard = true; gl->g.focusable = true;

    if ( !(gd->flags & gg_list_internal ) ) {
	gl->ti = GTextInfoArrayFromList(gd->u.list,&gl->ltot);
	gl->freeti = true;
    } else {
	gl->ti = (GTextInfo **) (gd->u.list);
	gl->ltot = GTextInfoArrayCount(gl->ti);
    }
    gl->hmax = GTextInfoGetMaxHeight(gl->g.base,gl->ti,gl->font,&same);
    gl->sameheight = same;
    if ( gd->flags & gg_list_alphabetic ) {
	gl->orderer = GListAlphaCompare;
	GListOrderIt(gl);
    }
    gl->start = gl->end = -1;
    if ( gd->flags & gg_list_multiplesel )
	gl->multiple_sel = true;
    else if ( gd->flags & gg_list_exactlyone ) {
	int sel = GListGetFirstSelPos(&gl->g);
	gl->exactly_one = true;
	if ( sel==-1 ) sel = 0;
	GListClearSel(gl);
	if ( gl->ltot>0 ) gl->ti[sel]->selected = true;
    }

    GListFit(gl);
    _GGadget_FinalPosition(&gl->g,base,gd);

    if ( gd->flags & gg_group_end )
	_GGadgetCloseGroup(&gl->g);
    GWidgetIndicateFocusGadget(&gl->g);
return( gl );
}

GGadget *GListCreate(struct gwindow *base, GGadgetData *gd,void *data) {
    GList *gl = _GListCreate(calloc(1,sizeof(GList)),base,gd,data,&list_box);

return( &gl->g );
}

static int popup_eh(GWindow popup,GEvent *event) {
    GGadget *owner = GDrawGetUserData(popup);
    if (owner == NULL) {
        return true;
    }

    if ( event->type == et_controlevent ) {
	GList *gl = (GList *) (event->u.control.g);
	void (*inform)(GGadget *,int) = (void (*) (GGadget *,int)) GGadgetGetUserData(&gl->g);
	int i;
	for ( i=0; i<gl->ltot; ++i )
	    if ( gl->ti[i]->selected )
	break;
	if ( i>=gl->ltot ) i = -1;
	GDrawDestroyWindow(popup);
	(inform)(owner,i);
    } else if ( event->type == et_close ) {
	GGadget *g = GWindowGetFocusGadgetOfWindow(popup);
	void (*inform)(GGadget *,int) = (void (*) (GGadget *,int)) GGadgetGetUserData(g);
	GDrawSetUserData(popup, NULL);
	GDrawDestroyWindow(popup);
	_GWidget_ClearPopupOwner(owner);
	_GWidget_ClearGrabGadget(owner);
	(inform)(owner,-1);
    } else if ( event->type == et_destroy ) {
	_GWidget_ClearPopupOwner(owner);
	_GWidget_ClearGrabGadget(owner);
    }
return( true );
}
	
static void GListPopupFigurePos(GGadget *owner,GTextInfo **ti,GRect *pos) {
    int width, height, width1, maxh;
    int i;
    GWindow root = GDrawGetRoot(GDrawGetDisplayOfWindow(owner->base));
    GRect rsize, rootsize;
    int bp;
    GPoint pt;

    if ( !glist_inited )
	GListInit();
    GDrawGetSize(GDrawGetRoot(GDrawGetDisplayOfWindow(owner->base)),&rootsize);
    maxh = 2*rootsize.height/3;
    width = GTextInfoGetMaxWidth(owner->base,ti,list_font);
    height = 0;
    for ( i=0; height<maxh && (ti[i]->text!=NULL || ti[i]->image!=NULL || ti[i]->line); ++i )
	height += GTextInfoGetHeight(owner->base,ti[i],list_font);
    if ( ti[i]->text!=NULL || ti[i]->image!=NULL || ti[i]->line )	/* Need a scroll bar if more */
	width += GDrawPointsToPixels(owner->base,_GScrollBar_Width) +
		GDrawPointsToPixels(owner->base,1);
    bp = GBoxBorderWidth(owner->base,&list_box);
    width += 2*bp; height += 2*bp;
    if ( (width1 = width)<owner->r.width ) width = owner->r.width;

    GDrawGetSize(root,&rsize);
    if ( width>rsize.width ) width = rsize.width;
    if ( height>rsize.height ) height = rsize.height;
    pt.x = owner->r.x; pt.y = owner->r.y+owner->r.height;
    GDrawTranslateCoordinates(owner->base,root,&pt);
    if ( pt.y+height > rsize.height ) {
	pt.x = owner->r.x; pt.y = owner->r.y-height;
	GDrawTranslateCoordinates(owner->base,root,&pt);
	if ( pt.y<0 ) {
	    pt.y = 0;
	    /* Ok, it will overlap the base widget. not that good an idea */
	    if ( pt.x+owner->r.width+width+3<rsize.width )
		pt.x += owner->r.width+3;
	    else if ( pt.x-width-3>=0 )
		pt.x -= width+3;
	    else {
		/* But there doesn't seem much we can do about it if we get here */
		;
            }
	}
    }
    pos->y = pt.y;

    if ( pt.x+width > rsize.width ) width = width1;
    if ( pt.x+width > rsize.width ) {
	pt.x = owner->r.x+owner->r.width-width; pt.y = 0;
	GDrawTranslateCoordinates(owner->base,root,&pt);
	if ( pt.x<0 )
	    pt.x = 0;
    }
    pos->x = pt.x;
    pos->width = width;
    pos->height = height;
}

GWindow GListPopupCreate(GGadget *owner,void (*inform)(GGadget *,int), GTextInfo **ti) {
    GWindow popup;
    GWindowAttrs pattrs;
    GDisplay *disp = GDrawGetDisplayOfWindow(owner->base);
    GRect pos;
    GGadgetData gd;
    GList *gl;
    int i;
    GEvent e;

    if ( ti==NULL )
return(NULL);

    GDrawPointerUngrab(disp);
    GDrawGetPointerPosition(owner->base,&e);

    pattrs.mask = wam_events|wam_nodecor|wam_positioned|wam_cursor|wam_transient|wam_verytransient;
    pattrs.event_masks = -1;
    pattrs.nodecoration = true;
    pattrs.positioned = true;
    pattrs.cursor = ct_pointer;
    pattrs.transient = GWidgetGetTopWidget(owner->base);

    GListPopupFigurePos(owner,ti,&pos);
    popup = GDrawCreateTopWindow(disp,&pos,popup_eh,owner,&pattrs);

    memset(&gd,'\0',sizeof(gd));
    gd.pos.x = gd.pos.y = 0;
    gd.pos.width = pos.width; gd.pos.height = pos.height;
    gd.u.list = (GTextInfo *) ti;
    gd.flags = gg_visible | gg_enabled | gg_pos_in_pixels | gg_list_internal |
	    gg_pos_use0;
    gl = (GList *) GListCreate(popup,&gd,(void *) inform);
    for ( i=0; ti[i]->text!=NULL || ti[i]->image!=NULL || ti[i]->line ; ++i )
	if ( ti[i]->selected ) {
	    GListScrollBy(gl,i,0);
    break;
	}
    GDrawSetVisible(popup,true);
    GDrawPointerGrab(popup);
    _GWidget_SetGrabGadget(&gl->g);
    if ( e.u.mouse.state&(ksm_button1|ksm_button2|ksm_button3) )
	gl->parentpressed = true;
    gl->ispopup = true;
    _GWidget_SetPopupOwner(owner);
return( popup );
}

int GListIndexFromY(GGadget *g,int y) {
return( GListIndexFromPos( (GList *) g, y ));
}

void GListSetSBAlwaysVisible(GGadget *g,int always) {
    ((GList *) g)->always_show_sb = always;
}

void GListSetPopupCallback(GGadget *g,void (*callback)(GGadget *,int)) {
    ((GList *) g)->popup_callback = callback;
}

GResInfo *_GListRIHead(void) {
    int as,ds,ld;

    if ( !glist_inited )
	GListInit();
    /* bp = GBoxBorderWidth(GDrawGetRoot(NULL),&list_box);*/	/* This gives bizarre values */
    GDrawWindowFontMetrics(GDrawGetRoot(NULL),list_font,&as, &ds, &ld);	/* I don't have a window yet... */
    list_gcd[0].gd.pos.height = list_gcd[1].gd.pos.height = 2*(as+ds)+4;
return( &glist_ri );
}
