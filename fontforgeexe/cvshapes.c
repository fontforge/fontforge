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
#include "fontforgeui.h"
#include "cvundoes.h"
#include <math.h>

static struct shapedescrip {
    BasePoint me, prevcp, nextcp; int nocp;
} ellipse3[] = {
    { { -1, 0 }, { -1, -0.552 }, { -1, 0.552 }, false },
    { { 0, 1 }, { -0.552, 1 }, { 0.552, 1 }, false },
    { { 1, 0 }, { 1, 0.552 }, { 1, -0.552 }, false },
    { { 0, -1 }, { 0.552, -1 }, { -0.552, -1 }, false },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, 0 }
};

static SplinePoint *SPMake(BasePoint *base,int pt) {
    SplinePoint *new;

    new = SplinePointCreate(base->x,base->y);
    new->pointtype = pt;
    new->selected = true;
return( new );
}

static SplinePoint *SPMakeTo(BasePoint *base,int pt, SplinePoint *from) {
    SplinePoint *to = SPMake(base,pt);
    SplineMake(from,to,false);		/* Always make a cubic, then convert to quadratic when done */
return( to );
}

void CVMouseDownShape(CharView *cv,GEvent *event) {
    real radius = CVRoundRectRadius(); int points = CVPolyStarPoints();
    SplinePoint *last;
    int i;
    struct shapedescrip *ellipse;

    if ( event->u.mouse.clicks==2 &&
	    (cv->active_tool == cvt_rect || cv->active_tool == cvt_elipse)) {
	CVRectEllipsePosDlg(cv);
return;
    }

    CVClearSel(cv);
    CVPreserveState(&cv->b);
    CVSetCharChanged(cv,true);
    cv->active_shape = chunkalloc(sizeof(SplineSet));
    cv->active_shape->next = cv->b.layerheads[cv->b.drawmode]->splines;
    cv->b.layerheads[cv->b.drawmode]->splines = cv->active_shape;
    cv->active_shape->first = last = SPMake(&cv->info,pt_corner);

    /* always make a cubic shape. Then convert to order2 when done */
    /*  the conversion routine already knows about rounding to int */
    switch ( cv->active_tool ) {
      case cvt_rect:
	if ( radius==0 ) {
	    last = SPMakeTo(&cv->info,pt_corner,last);
	    last = SPMakeTo(&cv->info,pt_corner,last);
	    last = SPMakeTo(&cv->info,pt_corner,last);
	} else {
	    last->pointtype = pt_tangent;
	    last = SPMakeTo(&cv->info,pt_tangent,last);
	    last = SPMakeTo(&cv->info,pt_tangent,last);
	    last = SPMakeTo(&cv->info,pt_tangent,last);
	    last = SPMakeTo(&cv->info,pt_tangent,last);
	    last = SPMakeTo(&cv->info,pt_tangent,last);
	    last = SPMakeTo(&cv->info,pt_tangent,last);
	    last = SPMakeTo(&cv->info,pt_tangent,last);
	}
      break;
      case cvt_elipse:
	ellipse = /*order2 ? ellipse2 :*/ ellipse3;
	last->pointtype = pt_curve;
	for ( i=1; ellipse[i].me.x!=0 || ellipse[i].me.y!=0 ; ++i ) {
	    last = SPMakeTo(&cv->info,pt_curve,last);
	}
      break;
      case cvt_poly:
	for ( i=1; i<points; ++i )
	    last = SPMakeTo(&cv->info,pt_corner,last);
      break;
      case cvt_star:
	for ( i=1; i<2*points; ++i )
	    last = SPMakeTo(&cv->info,pt_corner,last);
      break;
    }
    SplineMake(last,cv->active_shape->first,false);
    cv->active_shape->last = cv->active_shape->first;
    cv->b.sc->suspendMetricsViewEventPropagation = 1;
    SCUpdateAll(cv->b.sc);
}

static void SetCorner(SplinePoint *sp,real x, real y) {
    sp->me.x = x; sp->me.y = y;
    sp->nextcp = sp->me;
    sp->prevcp = sp->me;
}

static void SetCurve(SplinePoint *sp,BasePoint *center,real xrad, real yrad,
	struct shapedescrip *pt_d) {
    sp->me.x = center->x+xrad*pt_d->me.x; sp->me.y = center->y+yrad*pt_d->me.y;
    sp->nextcp.x = center->x+xrad*pt_d->nextcp.x; sp->nextcp.y = center->y+yrad*pt_d->nextcp.y;
    sp->prevcp.x = center->x+xrad*pt_d->prevcp.x; sp->prevcp.y = center->y+yrad*pt_d->prevcp.y;
    sp->nonextcp = sp->noprevcp = (xrad==0 && yrad==0);
}

static void SetPTangent(SplinePoint *sp,real x, real y,real xrad, real yrad) {
    sp->me.x = x; sp->me.y = y;
    sp->nextcp = sp->me;
    sp->prevcp = sp->me;
    if ( !sp->next->order2 ) {
	xrad *= .552;
	yrad *= .552;
    }
    sp->prevcp.x += xrad;
    sp->prevcp.y += yrad;
    sp->noprevcp = (xrad==0 && yrad==0);
}

static void SetNTangent(SplinePoint *sp,real x, real y,real xrad, real yrad) {
    sp->me.x = x; sp->me.y = y;
    sp->nextcp = sp->me;
    if ( !sp->next->order2 ) {
	xrad *= .552;
	yrad *= .552;
    }
    sp->nextcp.x += xrad;
    sp->nextcp.y += yrad;
    sp->prevcp = sp->me;
    sp->nonextcp = (xrad==0 && yrad==0);
}

static void RedoActiveSplineSet(SplineSet *ss) {
    Spline *spline, *first;

    first = NULL;
    for ( spline = ss->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	SplineRefigure(spline);
	if ( first == NULL ) first = spline;
    }
}

void CVMouseMoveShape(CharView *cv) {
    real radius = CVRoundRectRadius(); int points = CVPolyStarPoints();
    int center_out = CVRectElipseCenter();
    real xrad,yrad, xrr, yrr;
    real r2;
    SplinePoint *sp;
    real base, off;
    int i;
    struct shapedescrip *ellipse;
    BasePoint center;

    if ( cv->active_shape==NULL )
return;
    switch ( cv->active_tool ) {
      case cvt_rect:
	if ( radius==0 ) {
	    sp = cv->active_shape->first->next->to;
	    if ( !center_out ) {
		SetCorner(sp,cv->p.cx,cv->info.y);
		SetCorner(sp=sp->next->to, cv->info.x,cv->info.y);
		SetCorner(sp=sp->next->to, cv->info.x,cv->p.cy);
	    } else {
		if (( xrad = (cv->p.cx-cv->info.x) )<0 ) xrad = -xrad;
		if (( yrad = (cv->p.cy-cv->info.y) )<0 ) yrad = -yrad;
		SetCorner(sp,cv->p.cx-xrad,cv->p.cy+yrad);
		SetCorner(sp=sp->next->to, cv->p.cx+xrad,cv->p.cy+yrad);
		SetCorner(sp=sp->next->to, cv->p.cx+xrad,cv->p.cy-yrad);
		SetCorner(sp=sp->next->to, cv->p.cx-xrad,cv->p.cy-yrad);
	    }
	} else {
	    if ( !center_out ) {
		if (( xrad = (cv->p.cx-cv->info.x)/2 )<0 ) xrad = -xrad;
		if (( yrad = (cv->p.cy-cv->info.y)/2 )<0 ) yrad = -yrad;
		if ( xrad>radius ) xrr = radius; else xrr = xrad;
		if ( yrad>radius ) yrr = radius; else yrr = yrad;
		if ( cv->info.x<cv->p.cx ) xrr = -xrr;
		if ( cv->info.y<cv->p.cy ) yrr = -yrr;
		sp = cv->active_shape->first;
		SetPTangent(sp,
		    cv->p.cx+xrr,cv->p.cy, -xrr,0);
		SetNTangent(sp=sp->next->to,
		    cv->info.x-xrr,cv->p.cy, xrr,0);
		SetPTangent(sp=sp->next->to,
		    cv->info.x,cv->p.cy+yrr, 0,-yrr);
		SetNTangent(sp=sp->next->to,
		    cv->info.x,cv->info.y-yrr, 0,yrr);
		SetPTangent(sp=sp->next->to,
		    cv->info.x-xrr,cv->info.y, xrr,0);
		SetNTangent(sp=sp->next->to,
		    cv->p.cx+xrr,cv->info.y, -xrr,0);
		SetPTangent(sp=sp->next->to,
		    cv->p.cx,cv->info.y-yrr, 0,yrr);
		SetNTangent(sp=sp->next->to,
		    cv->p.cx,cv->p.cy+yrr, 0,-yrr);
	    } else {
		if (( xrad = (cv->p.cx-cv->info.x) )<0 ) xrad = -xrad;
		if (( yrad = (cv->p.cy-cv->info.y) )<0 ) yrad = -yrad;
		if ( xrad>radius ) xrr = radius; else xrr = xrad;
		if ( yrad>radius ) yrr = radius; else yrr = yrad;
		sp = cv->active_shape->first;
		SetPTangent(sp,
		    cv->p.cx-xrad+xrr,cv->p.cy-yrad, -xrr,0);
		SetNTangent(sp=sp->next->to,
		    cv->p.cx+xrad-xrr,cv->p.cy-yrad, xrr,0);
		SetPTangent(sp=sp->next->to,
		    cv->p.cx+xrad,cv->p.cy-yrad+yrr, 0,-yrr);
		SetNTangent(sp=sp->next->to,
		    cv->p.cx+xrad,cv->p.cy+yrad-yrr, 0,yrr);
		SetPTangent(sp=sp->next->to,
		    cv->p.cx+xrad-xrr,cv->p.cy+yrad, xrr,0);
		SetNTangent(sp=sp->next->to,
		    cv->p.cx-xrad+xrr,cv->p.cy+yrad, -xrr,0);
		SetPTangent(sp=sp->next->to,
		    cv->p.cx-xrad,cv->p.cy+yrad-yrr, 0,yrr);
		SetNTangent(sp=sp->next->to,
		    cv->p.cx-xrad,cv->p.cy-yrad+yrr, 0,-yrr);
	    }
	}
      break;
      case cvt_elipse:
	if ( !center_out ) {
	    center.x = (cv->p.cx+cv->info.x)/2; center.y = (cv->p.cy+cv->info.y)/2;
	    if (( xrad = (cv->p.cx-cv->info.x)/2 )<0 ) xrad = -xrad;
	    if (( yrad = (cv->p.cy-cv->info.y)/2 )<0 ) yrad = -yrad;
	} else {
	    center.x = cv->p.cx; center.y = cv->p.cy;
	    if (( xrad = (cv->p.cx-cv->info.x) )<0 ) xrad = -xrad;
	    if (( yrad = (cv->p.cy-cv->info.y) )<0 ) yrad = -yrad;
	}
	if ( cv->b.layerheads[cv->b.drawmode]->order2 ) {
	    xrad = rint(xrad);
	    yrad = rint(yrad);
	}
	ellipse = /*cv->b.sc->parent->order2 ? ellipse2 :*/ ellipse3;
	for ( i=0, sp=cv->active_shape->first; ellipse[i].me.x!=0 || ellipse[i].me.y!=0 ; ++i, sp=sp->next->to )
	    SetCurve(sp,&center,xrad,yrad,&ellipse[i]);
      break;
      case cvt_poly:
	base = atan2(cv->p.cy-cv->info.y,cv->p.cx-cv->info.x);
	radius = sqrt((cv->p.cy-cv->info.y)*(cv->p.cy-cv->info.y) +
		(cv->p.cx-cv->info.x)*(cv->p.cx-cv->info.x));
	off = -2*3.1415926535897932/points;
	sp = cv->active_shape->last->prev->from;
	for ( i=0; i<points; ++i )
	    SetCorner(sp=sp->next->to, cv->p.cx+radius*cos(base+i*off),
		    cv->p.cy+radius*sin(base+i*off));
      break;
      case cvt_star:
	base = atan2(cv->p.cy-cv->info.y,cv->p.cx-cv->info.x)-3.1415926535897932;
	if ( base<-3.1416 )
	    base += 2*3.1415926535897932;
	radius = sqrt((cv->p.cy-cv->info.y)*(cv->p.cy-cv->info.y) +
		(cv->p.cx-cv->info.x)*(cv->p.cx-cv->info.x));
	off = -2*3.1415926535897932/(2*points);
	sp = cv->active_shape->last->prev->from;
	r2 = radius/CVStarRatio();
	for ( i=0; i<2*points; ++i ) {
	    if ( i&1 ) {
		SetCorner(sp=sp->next->to, cv->p.cx+r2*cos(base+i*off),
			cv->p.cy+r2*sin(base+i*off));
	    } else {
		SetCorner(sp=sp->next->to, cv->p.cx+radius*cos(base+i*off),
			cv->p.cy+radius*sin(base+i*off));
	    }
	}
      break;
    }
    RedoActiveSplineSet(cv->active_shape);
    cv->b.sc->suspendMetricsViewEventPropagation = 1;
    SCUpdateAll(cv->b.sc);
}

void CVMouseUpShape(CharView *cv) {
    SplinePoint *first, *second, *sp;
    extern int snaptoint;

    if ( cv->active_shape==NULL )
return;

    if ( cv->b.layerheads[cv->b.drawmode]->order2 ) {
	SplineSet *prev, *new, *ss;
	new = SplineSetsTTFApprox(cv->active_shape);
	for ( ss=cv->b.layerheads[cv->b.drawmode]->splines, prev=NULL;
		ss!=NULL && ss!=cv->active_shape;
		prev = ss, ss=ss->next );
	if ( ss==NULL )
	    IError("Couldn't find shape");
	else {
	    if ( prev==NULL )
		cv->b.layerheads[cv->b.drawmode]->splines = new;
	    else
		prev->next = new;
	    SplinePointListsFree(cv->active_shape);
	    cv->active_shape = new;
	}
    }
    first = cv->active_shape->first; second = first->next->to;
    if ( first->me.x == second->me.x && first->me.y == second->me.y ) {
	/* Remove this shape, it will be selected */
	cv->b.layerheads[cv->b.drawmode]->splines = SplinePointListRemoveSelected(cv->b.sc,
		cv->b.layerheads[cv->b.drawmode]->splines);
    } else if ( cv->active_tool==cvt_rect || cv->active_tool==cvt_elipse ) {
	if ( SplinePointListIsClockwise(cv->active_shape)==0 )
	    SplineSetReverse(cv->active_shape);
	if ( snaptoint ) {
	    for ( sp= cv->active_shape->first; ; ) {
		SplinePointRound(sp,1.0);
		sp = sp->next->to;
		if ( sp==cv->active_shape->first )
	    break;
	    }
	    for ( sp= cv->active_shape->first; ; ) {
		SplineRefigure(sp->next);
		sp = sp->next->to;
		if ( sp==cv->active_shape->first )
	    break;
	    }
	}
    }
    if ( cv->b.sc->inspiro && hasspiro()) {
	free(cv->active_shape->spiros);
	cv->active_shape->spiros = SplineSet2SpiroCP(cv->active_shape,&cv->active_shape->spiro_cnt);
	cv->active_shape->spiro_max = cv->active_shape->spiro_cnt;
    }
    cv->active_shape = NULL;

    cv->b.sc->suspendMetricsViewEventPropagation = 0;
    SCUpdateAll(cv->b.sc);
}
