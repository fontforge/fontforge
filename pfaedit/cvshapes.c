/* Copyright (C) 2000-2002 by George Williams */
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
#include "pfaeditui.h"
#include <math.h>

static SplinePoint *SPMake(BasePoint *base,int pt) {
    SplinePoint *new;

    new = chunkalloc(sizeof(SplinePoint));
    new->me = *base;
    new->nextcp = *base;
    new->prevcp = *base;
    new->nonextcp = new->noprevcp = true;
    new->nextcpdef = new->prevcpdef = false;
    new->pointtype = pt;
    new->selected = true;
return( new );
}

static SplinePoint *SPMakeTo(BasePoint *base,int pt, SplinePoint *from) {
    SplinePoint *to = SPMake(base,pt);
    SplineMake(from,to);
return( to );
}

void CVMouseDownShape(CharView *cv) {
    real radius = CVRoundRectRadius(); int points = CVPolyStarPoints();
    SplinePoint *last;
    int i;

    CVClearSel(cv);
    CVPreserveState(cv);
    CVSetCharChanged(cv,true);
    cv->active_shape = chunkalloc(sizeof(SplineSet));
    cv->active_shape->next = *(cv->heads[cv->drawmode]);
    *cv->heads[cv->drawmode] = cv->active_shape;
    cv->active_shape->first = last = SPMake(&cv->info,pt_corner);

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
	last->pointtype = pt_curve;
	last = SPMakeTo(&cv->info,pt_curve,last);
	last = SPMakeTo(&cv->info,pt_curve,last);
	last = SPMakeTo(&cv->info,pt_curve,last);
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
    SplineMake(last,cv->active_shape->first);
    cv->active_shape->last = cv->active_shape->first;
    SCUpdateAll(cv->sc);
}

static void SetCorner(SplinePoint *sp,real x, real y) {
    sp->me.x = x; sp->me.y = y;
    sp->nextcp = sp->me;
    sp->prevcp = sp->me;
}

static void SetCurve(SplinePoint *sp,real x, real y,real xrad, real yrad) {
    sp->me.x = x; sp->me.y = y;
    sp->nextcp = sp->me;
    sp->nextcp.x += .552*xrad;
    sp->nextcp.y += .552*yrad;
    sp->prevcp = sp->me;
    sp->prevcp.x -= .552*xrad;
    sp->prevcp.y -= .552*yrad;
    sp->nonextcp = sp->noprevcp = (xrad==0 && yrad==0);
}

static void SetPTangent(SplinePoint *sp,real x, real y,real xrad, real yrad) {
    sp->me.x = x; sp->me.y = y;
    sp->nextcp = sp->me;
    sp->prevcp = sp->me;
    sp->prevcp.x += .552*xrad;
    sp->prevcp.y += .552*yrad;
    sp->noprevcp = (xrad==0 && yrad==0);
}

static void SetNTangent(SplinePoint *sp,real x, real y,real xrad, real yrad) {
    sp->me.x = x; sp->me.y = y;
    sp->nextcp = sp->me;
    sp->nextcp.x += .552*xrad;
    sp->nextcp.y += .552*yrad;
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
    real xrad,yrad;
    real r2;
    SplinePoint *sp;
    real base, off;
    int i;

    if ( cv->active_shape==NULL )
return;
    switch ( cv->active_tool ) {
      case cvt_rect:
	if ( radius==0 ) {
	    sp = cv->active_shape->first->next->to;
	    SetCorner(sp,cv->p.cx,cv->info.y);
	    SetCorner(sp=sp->next->to, cv->info.x,cv->info.y);
	    SetCorner(sp=sp->next->to, cv->info.x,cv->p.cy);
	} else {
	    if (( xrad = (cv->p.cx-cv->info.x)/2 )<0 ) xrad = -xrad;
	    if (( yrad = (cv->p.cy-cv->info.y)/2 )<0 ) yrad = -yrad;
	    if ( xrad>radius ) xrad = radius;
	    if ( yrad>radius ) yrad = radius;
	    if ( cv->info.x<cv->p.cx ) xrad = -xrad;
	    if ( cv->info.y<cv->p.cy ) yrad = -yrad;
	    sp = cv->active_shape->first;
	    SetPTangent(sp,
		cv->p.cx+xrad,cv->p.cy, -xrad,0);
	    SetNTangent(sp=sp->next->to,
		cv->info.x-xrad,cv->p.cy, xrad,0);
	    SetPTangent(sp=sp->next->to,
		cv->info.x,cv->p.cy+yrad, 0,-yrad);
	    SetNTangent(sp=sp->next->to,
		cv->info.x,cv->info.y-yrad, 0,yrad);
	    SetPTangent(sp=sp->next->to,
		cv->info.x-xrad,cv->info.y, xrad,0);
	    SetNTangent(sp=sp->next->to,
		cv->p.cx+xrad,cv->info.y, -xrad,0);
	    SetPTangent(sp=sp->next->to,
		cv->p.cx,cv->info.y-yrad, 0,yrad);
	    SetNTangent(sp=sp->next->to,
		cv->p.cx,cv->p.cy+yrad, 0,-yrad);
	}
      break;
      case cvt_elipse:
	SetCurve(sp = cv->active_shape->first,
	    (cv->p.cx+cv->info.x)/2,cv->p.cy,-(cv->p.cx-cv->info.x)/2,0);
	SetCurve(sp = sp->next->to,
	    cv->info.x,(cv->p.cy+cv->info.y)/2,0,-(cv->p.cy-cv->info.y)/2);
	SetCurve(sp = sp->next->to,
	    (cv->p.cx+cv->info.x)/2,cv->info.y,(cv->p.cx-cv->info.x)/2,0);
	SetCurve(sp = sp->next->to,
	    cv->p.cx,(cv->p.cy+cv->info.y)/2,0,(cv->p.cy-cv->info.y)/2);
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
    SCUpdateAll(cv->sc);
}

void CVMouseUpShape(CharView *cv) {
    if ( cv->active_tool==cvt_rect || cv->active_tool==cvt_elipse ) {
	if ( !SplinePointListIsClockwise(cv->active_shape))
	    SplineSetReverse(cv->active_shape);
    }
    cv->active_shape = NULL;
}
