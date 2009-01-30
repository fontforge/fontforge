/* Copyright (C) 2000-2009 by George Williams */
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
#include "pfaedit.h"
#include "splinefont.h"
#include <math.h>
#include <gwidget.h>

#define PI	3.1415926535897932

typedef struct joininfo {
    SplinePoint *from, *to;
    real tprev;
    real tnext;
    BasePoint inter;
} JointPoint;


static real SplineAngle(Spline *spline,real t) {
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];
    real xslope = (3*xsp->a*t+2*xsp->b)*t + xsp->c;
    real yslope = (3*ysp->a*t+2*ysp->b)*t + ysp->c;

    if ( xslope==0 && yslope==0 ) {
	real faket = (t>.5) ? t-.01 : t+.01;
	xslope = (3*xsp->a*faket+2*xsp->b)*faket + xsp->c;
	yslope = (3*ysp->a*faket+2*ysp->b)*faket + ysp->c;
    }
    if ( spline->knownlinear || ( xslope==0 && yslope==0 )) {
	xslope = spline->to->me.x-spline->from->me.x;
	yslope = spline->to->me.y-spline->from->me.y;
    }
return( atan2(yslope,xslope) );
}

static int PenCorner(double lineangle,StrokeInfo *si) {

    if ( ( lineangle>=si->penangle && lineangle<=si->penangle+PI/2 ) ||
	 ( lineangle+2*PI>=si->penangle && lineangle+2*PI<=si->penangle+PI/2 ) ||
	 ( lineangle-2*PI>=si->penangle && lineangle-2*PI<=si->penangle+PI/2 ) ) {
return( 0 );
    } else if ( ( lineangle>=si->penangle+PI/2 && lineangle<=si->penangle+PI ) ||
		( lineangle+2*PI>=si->penangle+PI/2 && lineangle+2*PI<=si->penangle+PI ) ||
		( lineangle-2*PI>=si->penangle+PI/2 && lineangle-2*PI<=si->penangle+PI ) ) {
return( 1 );
    } else if ( ( lineangle>=si->penangle+PI && lineangle<=si->penangle+3*PI/2 ) ||
		( lineangle+2*PI>=si->penangle+PI && lineangle+2*PI<=si->penangle+3*PI/2 ) ||
		( lineangle-2*PI>=si->penangle+PI && lineangle-2*PI<=si->penangle+3*PI/2 ) ) {
return( 2 );
    } else {
return( 3 );
    }
}

/* the plus point is where we go when we rotate the line's direction by +90degrees */
/*  and then move radius in that direction. minus is when we rotate -90 and */
/*  then move */	/* counter-clockwise */
static double SplineExpand(Spline *spline,real t,real toff, StrokeInfo *si,
	BasePoint *plus, BasePoint *minus) {
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];
    BasePoint base;
    double lineangle, c,s, factor = 1.0;

    if ( si->factor!=NULL )
	factor = (si->factor)(si->data,spline,t);

    base.x = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
    base.y = ((ysp->a*t+ysp->b)*t+ysp->c)*t + ysp->d;

    lineangle = SplineAngle(spline,t+toff);
    if ( si->stroke_type != si_caligraphic ) {
	c = si->radius*factor*cos(lineangle+PI/2);
	s = si->radius*factor*sin(lineangle+PI/2);
	plus->y = base.y+s;
	plus->x = base.x+c;
	minus->y = base.y-s;
	minus->x = base.x-c;
    } else {
	int corner = PenCorner(lineangle,si);
	plus->x = base.x + factor*si->xoff[corner];
	plus->y = base.y + factor*si->yoff[corner];
	corner += 2;
	minus->x = base.x + factor*si->xoff[corner];
	minus->y = base.y + factor*si->yoff[corner];
    }
return( lineangle );
}

static SplinePoint *makequartercircle(real x, real y, real radius,
	real xmul, real ymul,SplinePoint *prev) {
    SplinePoint *here = SplinePointCreate(x,y);

    if ( xmul==0 ) {
	here->nextcp.x = here->prevcp.x = x;
	here->nextcp.y = y + .552*ymul*radius;
	here->prevcp.y = y - .552*ymul*radius;
    } else {
	here->nextcp.y = here->prevcp.y = y;
	here->nextcp.x = x + .552*xmul*radius;
	here->prevcp.x = x - .552*xmul*radius;
    }
    here->nonextcp = here->noprevcp = false;
    if ( prev!=NULL )
	SplineMake3(prev,here);
return( here );
}

static SplinePoint *makeline(SplinePoint *prev, real x, real y) {
    SplinePoint *here = SplinePointCreate(x,y);
    here->pointtype = pt_corner;
    if ( prev!=NULL )
	SplineMake3(prev,here);
return( here );
}

static void SinglePointStroke(SplinePoint *base, StrokeInfo *si, SplinePoint **_plus, SplinePoint **_minus) {
    SplinePoint *plus, *cur;

    /* A single point, is kind of dull.
	For a caligraphic pen, it's just a copy of the pen
	For a linecap of lc_butt it's still a point
	For a linecap of lc_round it's a circle
	For a linecap of lc_square it should be a square...
	    but how does one orient that square? probably a circle is best
	    here too
    */
    /* We don't have a spline, so don't try guessing factor */
    if ( si->stroke_type == si_caligraphic ) {
	plus = SplinePointCreate(base->me.x+si->xoff[0],base->me.y+si->yoff[0]);
	plus->pointtype = pt_corner;
	cur = makeline(plus,base->me.x+si->xoff[1],base->me.y+si->yoff[1]);
	cur = makeline(cur,base->me.x+si->xoff[2],base->me.y+si->yoff[2]);
	cur = makeline(cur,base->me.x+si->xoff[3],base->me.y+si->yoff[3]);
	SplineMake3(cur,plus);
	*_plus = *_minus = plus;
    } else if ( si->cap!=lc_butt ) {
	plus = makequartercircle(base->me.x-si->radius,base->me.y,si->radius,0,1,NULL);
	cur = makequartercircle(base->me.x,base->me.y+si->radius,si->radius,1,0,plus);
	cur = makequartercircle(base->me.x+si->radius,base->me.y,si->radius,0,-1,cur);
	cur = makequartercircle(base->me.x,base->me.y-si->radius,si->radius,-1,0,cur);
	SplineMake3(cur,plus);
	*_plus = *_minus = plus;
    } else {
	*_plus = *_minus = cur = chunkalloc(sizeof(SplinePoint));
	*cur = *base;
	cur->next = cur->prev = NULL;
	cur->hintmask = NULL;
    }
}

static SplinePoint *StrokeEnd(SplinePoint *base, StrokeInfo *si, int isstart,
	SplinePoint **_to) {
    BasePoint junk;
    SplinePoint *mid1, *mid2, *cur, *from, *to;
    real len;
    real c,s;
    real angle;
    real sign;
    real factor = si->factor==NULL ? 1.0 :
	    base->next!=NULL ? (si->factor)(si->data,base->next,0) :
	    base->prev!=NULL ? (si->factor)(si->data,base->prev,1) :
	    1.0;

    from = chunkalloc(sizeof(SplinePoint));
    to = chunkalloc(sizeof(SplinePoint));
    from->nonextcp = to->nonextcp = from->noprevcp = to->noprevcp = true;
    from->pointtype = pt_corner; to->pointtype = pt_corner;

    if ( isstart )
	angle = SplineExpand(base->next,0,0,si,&from->me,&to->me)+ PI;
    else
	angle = SplineExpand(base->prev,1,0,si,&to->me,&from->me);

    if ( (len = to->me.x-from->me.x)<0 )
	len = -len;
    len += ( to->me.y > from->me.y ) ? (to->me.y - from->me.y) : (from->me.y - to->me.y);

    if ( si->stroke_type == si_caligraphic ) {
	int corner;
	corner = PenCorner(angle,si);
	cur = makeline(from,base->me.x+factor*si->xoff[corner+1],base->me.y+factor*si->yoff[corner+1]);
	SplineMake3(cur,to);
    } else {
	if ( isstart ) {
	    SplineIsLinearMake(base->next);
	    angle = SplineExpand(base->next,0,0,si,&junk,&junk)+ PI;
	    sign = -1;
	} else {
	    SplineIsLinearMake(base->prev);
	    angle = SplineExpand(base->prev,1,0,si,&junk,&junk);
	    sign = -1;
	}
	if ( si->cap==lc_butt ) {
	    SplineMake3(from,to);		/* draw a line between */
	} else if ( si->cap==lc_square ) {
	    mid1 = SplinePointCreate(
		    from->me.x+ sign*(from->me.y-base->me.y),
		    from->me.y- sign*(from->me.x-base->me.x));
	    mid2 = SplinePointCreate(
		    to->me.x+ sign*(from->me.y-base->me.y),
		    to->me.y- sign*(from->me.x-base->me.x));
	    mid1->pointtype = pt_corner; mid2->pointtype = pt_corner;
	    SplineMake3(from,mid1);
	    SplineMake3(mid1,mid2);
	    SplineMake3(mid2,to);
	} else if ( si->cap==lc_round ) {
	    mid1 = chunkalloc(sizeof(SplinePoint));
	    mid1->me.x = base->me.x+ sign*(from->me.y-base->me.y);
	    mid1->me.y = base->me.y- sign*(from->me.x-base->me.x);
	    mid1->pointtype = pt_curve;
	    c = .552*si->radius*factor*cos(angle);
	    s = .552*si->radius*factor*sin(angle);
	    from->nextcp.x = from->me.x + c;
	    from->nextcp.y = from->me.y + s;
	    from->nonextcp = false;
	    to->prevcp.x = to->me.x +c;
	    to->prevcp.y = to->me.y +s;
	    to->noprevcp = false;
	    mid1->prevcp.x = mid1->me.x - sign*s;
	    mid1->prevcp.y = mid1->me.y + sign*c;
	    mid1->nextcp.x = mid1->me.x + sign*s;
	    mid1->nextcp.y = mid1->me.y - sign*c;
	    SplineMake3(from,mid1);
	    SplineMake3(mid1,to);
	}
    }
    SplinePointCatagorize(to);
    SplinePointCatagorize(from);
    *_to = to;
return( from );
}

/* Is this the inner intersection or the outer one (the inner one is on both splines) */
/*  the outer one is beyond both */
static int Intersect_Lines(BasePoint *inter,BasePoint *p1,real sx1, real sy1,
	BasePoint *p2, real sx2, real sy2, real radius) {
    real t1/*,t2*/;
    real denom;

    denom = (sx1*sy2-sx2*sy1);
    if ( denom>-.0001 && denom<.0001 ) {
	/* Lines are parallel. Might be coincident, might not */
	t1 = 10000;
    } else {
	/* t2 = (sy1*(p2->x-p1->x)-sx1*(p2->y-p1->y))/denom;*/
	t1 = (sy2*(p2->x-p1->x)-sx2*(p2->y-p1->y))/denom;
    }
    if ( t1>1000 || t1<-1000 ) {
	denom = sqrt(sx1*sx1 + sy1*sy1)/radius;
	if ( denom==0 ) {
	    inter->x = (p1->x+p2->x)/2;
	    inter->y = (p1->y+p2->y)/2;
	} else {
	    inter->x = (p1->x+p2->x)/2 + sx1/denom;
	    inter->y = (p1->y+p2->y)/2 + sy1/denom;
	}
return( -1 );
    } else {
	inter->x = p1->x + t1*sx1;
	inter->y = p1->y + t1*sy1;
return( t1<=0 );	/* if t1 < 0 then the intersection point is actually */
			/*  on both of the spline segments. if it isn't then */
			/*  it will be on the continuation of the spline */
			/*  but beyond its endpoint... */
    }
}

static double CircleCpDist(double angle) {
    /* To draw an arc of length angle on a unit circle, the control points */
    /*  should be this far from their base points. Determined empirically, */
    /*  fit by least squares */

    if ( angle<0 ) angle = -angle;
    while ( angle>2*PI ) angle -= 2*PI;
    if ( angle>PI ) angle = 2*PI-angle;
return( ((0.0115445*angle - 0.0111987)*angle + 0.357114)*angle );
}

static SplinePoint *ChordMid(double angle,BasePoint *center,BasePoint *from,
	double *_cpratio) {
    BasePoint off, new;
    double s,c,cpratio;
    SplinePoint *sp;

    if ( angle<0 ) angle = -angle;
    while ( angle>2*PI ) angle -= 2*PI;
    if ( angle>PI ) angle = 2*PI-angle;
    angle /= 2;

    off.x = from->x-center->x;
    off.y = from->y-center->y;
    s = sin(angle); c = cos(angle);
    new.x = c*off.x - s*off.y;
    new.y = s*off.x + c*off.y;
    sp = SplinePointCreate(new.x+center->x,new.y+center->y);

    *_cpratio = cpratio = CircleCpDist(angle);
    new.x *= cpratio; new.y *= cpratio;		/* new is a vector of length radius pointing perp to the direction of the cps */
		/* We need to multiply by cp ratio and rotate 90 degrees */
    sp->prevcp.x = sp->me.x + new.y;
    sp->prevcp.y = sp->me.y - new.x;
    sp->nextcp.x = sp->me.x - new.y;
    sp->nextcp.y = sp->me.y + new.x;
    sp->nonextcp = sp->noprevcp = false;
return( sp );
}

static int IntersectionTooFar(BasePoint *inter,SplinePoint *from,SplinePoint *to,StrokeInfo *si) {
    /* Things look really ugly when we try to miter acute angles -- we get */
    /* huge spikes. So if mitering is going to give bad results, just bevel */
    double len, xoff, yoff;

    xoff = inter->x-from->me.x; yoff = inter->y-from->me.y;
    len = xoff*xoff + yoff*yoff;
    if ( len > (5*si->radius * 5*si->radius) )
return( true );

    xoff = inter->x-to->me.x; yoff = inter->y-to->me.y;
    len = xoff*xoff + yoff*yoff;
    if ( len > (5*si->radius * 5*si->radius) )
return( true );

return( false );
}

static void MakeJoints(SplinePoint *from,SplinePoint *to,StrokeInfo *si,
	BasePoint *inter, BasePoint *center,
	int incr,double pangle, double nangle, real factor) {
    SplinePoint *mid;
    int cstart, cend, i;

    if ( si->stroke_type == si_caligraphic ) {
	cstart = PenCorner(pangle,si);
	cend = PenCorner(nangle,si);
	if ( cstart==cend ) {
	    /* same as a miter join */
	    mid = SplinePointCreate(inter->x,inter->y);
	    mid->pointtype = pt_corner;
	    SplineMake3(from,mid);
	    SplineMake3(mid,to);
	} else {
	    if ( incr<0 ) {
		if ((cstart += 2)>=4 ) cstart -= 4;
		if ((cend += 2)>=4 ) cend -= 4;
		incr = 1;	/* Why??? */
	    }
	    if ( incr>0 && cstart>cend )
		cend += 4;
	    else if ( incr<0 && cstart<cend )
		cstart += 4;
	    i = cstart + incr;		/* First one is from */
	    mid = from;
	    while ( i!=cend ) {
		mid = makeline(mid,center->x+factor*si->xoff[i],center->y+factor*si->yoff[i]);
		i += incr;
	    }
	    SplineMake3(mid,to);
	}
    } else if ( si->join == lj_miter && !IntersectionTooFar(inter,from,to,si)) {
	mid = SplinePointCreate(inter->x,inter->y);
	mid->pointtype = pt_corner;
	SplineMake3(from,mid);
	SplineMake3(mid,to);
	if ( from->ptindex == to->ptindex )
	    mid->ptindex = from->ptindex;
    } else if ( si->join==lj_bevel ) {
	SplineMake3(from,to);
    } else {
	double cplen = CircleCpDist(nangle-pangle);
	mid = NULL;
	if ( cplen>.6 ) {
	    /* If angle of the arc is more than about 90 degrees a cubic */
	    /*  spline is noticeably different from a circle's arc */
	    /* So add an extra point to help things out */
	    mid = ChordMid(nangle-pangle,center,&from->me,&cplen);
	}
	cplen *= si->radius*factor;
	from->pointtype = to->pointtype = pt_curve;
	from->nextcp.x = from->me.x-cplen*cos(nangle);
	from->nextcp.y = from->me.y-cplen*sin(nangle);
	to->prevcp.x = to->me.x+cplen*cos(pangle);
	to->prevcp.y = to->me.y+cplen*sin(pangle);
	from->nonextcp = false; to->noprevcp = false;
	if ( mid==NULL )
	    SplineMake3(from,to);
	else {
	    SplineMake3(from,mid);
	    SplineMake3(mid,to);
	}
    }
}

static int OnEdge(BasePoint *plus,BasePoint *minus,Spline *sp, double t,
	double heret, Spline *hsp,
	StrokeInfo *si, double *_ppt, double *_pmt, double *_mpt, double *_mmt) {
    double rsq = si->radius*si->radius;
    double tt, xdiff, ydiff, loopdiff;
    double pptval= -1, pmtval= -1, mptval= -1, mmtval = -1;
    BasePoint here, test;

    here.x = ((hsp->splines[0].a*heret+hsp->splines[0].b)*heret+hsp->splines[0].c)*heret+hsp->splines[0].d;
    here.y = ((hsp->splines[1].a*heret+hsp->splines[1].b)*heret+hsp->splines[1].c)*heret+hsp->splines[1].d;

    if ( (xdiff = sp->to->me.x-sp->from->me.x)<0 ) xdiff = -xdiff;
    if ( (ydiff = sp->to->me.y-sp->from->me.y)<0 ) ydiff = -ydiff;
    loopdiff = (xdiff+ydiff==0) ? 2 : 1.0/(4*(xdiff+ydiff)/si->radius);

    if ( _ppt!=NULL ) {
	for ( tt = t+loopdiff; tt<=1 ; tt += loopdiff ) {
	    test.x = ((sp->splines[0].a*tt+sp->splines[0].b)*tt+sp->splines[0].c)*tt+sp->splines[0].d;
	    test.y = ((sp->splines[1].a*tt+sp->splines[1].b)*tt+sp->splines[1].c)*tt+sp->splines[1].d;
	    if ( (test.x-here.x)*(test.x-here.x)+(test.y-here.y)*(test.y-here.y)> 2*rsq )
	break;
	    if ( (plus->x-test.x)*(plus->x-test.x)+(plus->y-test.y)*(plus->y-test.y)<= rsq )
		pptval = tt;
	    if ( (minus->x-test.x)*(minus->x-test.x)+(minus->y-test.y)*(minus->y-test.y)<= rsq )
		pmtval = tt;
	}
	*_ppt = pptval; *_pmt = pmtval;
    }

    if ( _mmt!=NULL ) {
	for ( tt = t-loopdiff; tt>=0 ; tt -= loopdiff ) {
	    test.x = ((sp->splines[0].a*tt+sp->splines[0].b)*tt+sp->splines[0].c)*tt+sp->splines[0].d;
	    test.y = ((sp->splines[1].a*tt+sp->splines[1].b)*tt+sp->splines[1].c)*tt+sp->splines[1].d;
	    if ( (test.x-here.x)*(test.x-here.x)+(test.y-here.y)*(test.y-here.y)> 2*rsq )
	break;
	    if ( (plus->x-test.x)*(plus->x-test.x)+(plus->y-test.y)*(plus->y-test.y)< rsq )
		mptval = tt;
	    if ( (minus->x-test.x)*(minus->x-test.x)+(minus->y-test.y)*(minus->y-test.y)< rsq )
		mmtval = tt;
	}
	*_mmt = mmtval; *_mpt = mptval;
    }

return( pptval!=-1 || mmtval!=-1 || pmtval!=-1 || mptval==-1 );
}

#define BasePtDistance(pt1, pt2)  sqrt(((pt1)->x-(pt2)->x)*((pt1)->x-(pt2)->x) + ((pt1)->y-(pt2)->y)*((pt1)->y-(pt2)->y))

#if 0
static int PointToSplineLessThan(BasePoint *test,Spline *against,real radius) {
    BasePoint here;
    double t;
    double xdiff,ydiff, delta;

    if ( BasePtDistance(test,&against->from->me)<=radius+.01 )
return( true );
    if ( BasePtDistance(test,&against->to->me)<=radius+.01 )
return( true );
    if ( (xdiff = against->from->me.x-against->to->me.x)<0 ) xdiff = -xdiff;
    if ( (ydiff = against->from->me.y-against->to->me.y)<0 ) ydiff = -ydiff;
    delta = radius/(2*(xdiff+ydiff));
    if ( delta<.0625 ) delta = .0625;
    for ( t=delta; t<.99999 ; t+=delta ) {
	here.x = ((against->splines[0].a*t+against->splines[0].b)*t+against->splines[0].c)*t+against->splines[0].d;
	here.y = ((against->splines[1].a*t+against->splines[1].b)*t+against->splines[1].c)*t+against->splines[1].d;
	if ( BasePtDistance(test,&here)<=radius )
return( true );
    }
return( false );
}

static int EntirelyWithin(SplinePoint *start,Spline *s,int dir,real radius) {
    /* Return whether every point along the splines starting at start is */
    /*  within radius of some point along the spline s */
    Spline *test = dir ? start->next : start->prev;
    BasePoint here;
    double t;

    while ( test!=NULL ) {
	if ( !PointToSplineLessThan(&test->from->me,s,radius) ||
		!PointToSplineLessThan(&test->to->me,s,radius))
return( false );
	for ( t=.125; t<.99999 ; t+=.125 ) {
	    here.x = ((test->splines[0].a*t+test->splines[0].b)*t+test->splines[0].c)*t+test->splines[0].d;
	    here.y = ((test->splines[1].a*t+test->splines[1].b)*t+test->splines[1].c)*t+test->splines[1].d;
	    if ( !PointToSplineLessThan(&test->to->me,s,radius))
return( false );
	}
	if ( dir ) test = test->to->next;
	else test = test->from->prev;
    }
return( true );
}
#endif

static SplinePoint *MergeSplinePoint(SplinePoint *sp1,SplinePoint *sp2) {
    /* sp1 and sp2 should be close together, use their average for the */
    /*  new position, get rid of one, and add its spline to the other */
    /* sp1->next==NULL, sp2->prev==NULL */
    double offx, offy;

    offx = (sp1->me.x-sp2->me.x)/2;
    offy = (sp1->me.y-sp2->me.y)/2;
    sp1->me.x -= offx; sp1->prevcp.x -= offx;
    sp1->me.y -= offy; sp1->prevcp.y -= offy;
    sp1->nextcp.x = sp2->nextcp.x + offx;
    sp1->nextcp.y = sp2->nextcp.y + offy;
    sp1->nonextcp = sp2->nonextcp;
    sp1->next = sp2->next;
    SplinePointFree(sp2);
    if ( sp1->next!=NULL )
	sp1->next->from = sp1;
    SplinePointCatagorize(sp1);
    if ( sp1->prev!=NULL )
	SplineRefigure(sp1->prev);
    if ( sp1->next!=NULL )
	SplineRefigure(sp1->next);
return( sp1 );
}

static void MSP(SplinePoint *sp1,SplinePoint **sp2, SplinePoint **sp2alt) {
    int same2 = *sp2==*sp2alt;

    *sp2 = MergeSplinePoint(sp1,*sp2);
    if ( same2 )
	*sp2alt = *sp2;
}

static SplinePoint *SplineMaybeBisect(Spline *s,double t) {
    /* Things get very confused if I have a splineset with just a single point */
    SplinePoint *temp, *sp;

    if ( t<.0001 ) {
	temp = chunkalloc(sizeof(SplinePoint));
	sp = s->from;
	*temp = *sp;
	temp->hintmask = NULL;
	temp->next->from = temp;
	sp->next = NULL;
	sp->nextcp = sp->me;
	sp->nonextcp = true;
	temp->prevcp = temp->me;
	temp->noprevcp = true;
	SplineMake3(sp,temp);
return( temp );
    } else if ( t>.9999 ) {
	temp = chunkalloc(sizeof(SplinePoint));
	sp = s->to;
	*temp = *sp;
	temp->hintmask = NULL;
	temp->prev->to = temp;
	sp->prev = NULL;
	sp->prevcp = sp->me;
	sp->noprevcp = true;
	temp->nextcp = temp->me;
	temp->nonextcp = true;
	SplineMake3(temp,sp);
return( temp );
    }

return( SplineBisect(s,t));
}

static void SplineFreeBetween(SplinePoint *from,SplinePoint *to,int freefrom,int freeto) {
    Spline *s;

    if ( from==to ) {
	if ( freefrom && freeto )
	    SplinePointFree(from);
return;
    }

    while ( from!=to && from!=NULL ) {
	s = from->next;
	if ( freefrom )
	    SplinePointFree(from);
	else
	    from->next = NULL;
	if ( s==NULL )
return;
	freefrom = true;
	from = s->to;
	SplineFree(s);
    }
    if ( freeto )
	SplinePointFree(to);
    else
	to->prev = NULL;
}

static void SplineFreeForeward(SplinePoint *from) {
    Spline *s;

    while ( from!=NULL ) {
	s = from->next;
	SplinePointFree(from);
	if ( s==NULL )
return;
	from = s->to;
	SplineFree(s);
    }
}

static void SplineFreeBackward(SplinePoint *to) {
    Spline *s;

    while ( to!=NULL ) {
	s = to->prev;
	SplinePointFree(to);
	if ( s==NULL )
return;
	to = s->from;
	SplineFree(s);
    }
}

static SplinePoint *SplineCopyAfter(SplinePoint *from,SplinePoint **end) {
    SplinePoint *head, *last;

    last = head = chunkalloc(sizeof(SplinePoint));
    *head = *from;
    head->hintmask = NULL;
    head->prev = NULL;
    while ( from->next!=NULL ) {
	last->next = chunkalloc(sizeof(Spline));
	*last->next = *from->next;
	last->next->from = last;
	last->next->to = chunkalloc(sizeof(SplinePoint));
	*last->next->to = *from->next->to;
	last->next->to->hintmask = NULL;
	last->next->to->prev = last->next;
	last = last->next->to;
	from = from->next->to;
    }
    *end = last;
return( head );
}

static SplinePoint *SplineCopyBefore(SplinePoint *to,SplinePoint **end) {
    SplinePoint *head, *last;

    last = head = chunkalloc(sizeof(SplinePoint));
    *head = *to;
    head->hintmask = NULL;
    head->next = NULL;
    while ( to->prev!=NULL ) {
	last->prev = chunkalloc(sizeof(Spline));
	*last->prev = *to->prev;
	last->prev->to = last;
	last->prev->from = chunkalloc(sizeof(SplinePoint));
	*last->prev->from = *to->prev->from;
	last->prev->from->hintmask = NULL;
	last->prev->from->next = last->prev;
	last = last->prev->from;
	to = to->prev->from;
    }
    *end = last;
return( head );
}

static SplinePoint *Intersect_Splines(SplinePoint *from,SplinePoint *to,
	SplinePoint **ret) {
    Spline *test1, *test2;
    BasePoint pts[9];
    extended t1s[9], t2s[9];

    for ( test1=from->next; test1!=NULL; test1=test1->to->next ) {
	for ( test2=to->prev; test2!=NULL; test2=test2->from->prev ) {
	    if ( SplinesIntersect(test1,test2,pts,t1s,t2s)>0 ) {
		*ret = SplineMaybeBisect(test2,t2s[0]);
return( SplineMaybeBisect(test1,t1s[0]));
	    }
	}
    }
    *ret = NULL;
return( NULL );
}

struct strokedspline {
    Spline *s;
    SplinePoint *plusfrom, *plusto, *origplusfrom;
    SplinePoint *minusfrom, *minusto, *origminusto;
    int8 plusskip, minusskip;		/* If this spline is so small that it is totally within the region stroked by an adjacent spline */
    int8 pinnerto, minnerto;		/* to and from as defined on original spline s */
    BasePoint minterto, pinterto;
    double nangle, pangle;
    struct strokedspline *next, *prev;
};

static void StrokeEndComplete(struct strokedspline *cur,StrokeInfo *si,int isstart) {
    SplinePoint *edgestart, *edgeend, *curat, *edgeat;
    struct strokedspline *lastp, *lastm;

    if ( isstart ) {
	edgestart = StrokeEnd(cur->s->from,si,true,&edgeend);
	for ( lastp=cur; lastp!=NULL && lastp->plusskip ; lastp=lastp->next );
	for ( lastm=cur; lastm!=NULL && lastm->minusskip ; lastm=lastm->next );
	if ( lastm==cur )
	    MSP(edgeend,&cur->minusfrom,&cur->minusto);
	else {
	    curat = Intersect_Splines(lastm->minusfrom,edgeend,&edgeat);
	    if ( curat!=NULL ) {
		SplineFreeBetween(lastm->minusfrom,curat,true,false);
		SplineFreeBetween(edgeat,edgeend,false,true);
	    } else
		MSP(edgeend,&lastm->minusfrom,&lastm->minusto);
	}
	if ( lastp==cur )
	    MergeSplinePoint(cur->plusto,edgestart);
	else {
	    edgeat = Intersect_Splines(edgestart,lastp->plusto,&curat);
	    if ( curat!=NULL ) {
		SplineFreeBetween(curat,lastp->plusto,false,true);
		SplineFreeBetween(edgestart,edgeat,true,false);
	    } else
		MergeSplinePoint(lastp->plusto,edgestart);
	}
    } else {
	edgestart = StrokeEnd(cur->s->to,si,false,&edgeend);
	for ( lastp=cur; lastp!=NULL && lastp->plusskip ; lastp=lastp->prev );
	for ( lastm=cur; lastm!=NULL && lastm->minusskip ; lastm=lastm->prev );
	if ( lastp==cur )
	    MSP(edgeend,&cur->plusfrom,&cur->plusto);
	else {
	    curat = Intersect_Splines(lastp->plusfrom,edgeend,&edgeat);
	    if ( curat!=NULL ) {
		SplineFreeBetween(lastp->plusfrom,curat,true,false);
		lastp->plusfrom = curat;
		SplineFreeBetween(edgeat,edgeend,false,true);
		lastp->plusfrom = MergeSplinePoint(edgeat,curat);
	    } else
		MSP(edgeend,&lastp->plusfrom,&lastp->plusto);
	}
	if ( lastm==cur )
	    MergeSplinePoint(cur->minusto,edgestart);
	else {
	    edgeat = Intersect_Splines(edgestart,lastm->minusto,&curat);
	    if ( curat!=NULL ) {
		SplineFreeBetween(curat,lastm->minusto,false,true);
		lastm->minusto = curat;
		SplineFreeBetween(edgestart,edgeat,true,false);
		MergeSplinePoint(lastm->minusto,edgeat);
	    } else
		MergeSplinePoint(lastm->minusto,edgestart);
	}
    }
}

static void StrokedSplineFree(struct strokedspline *head) {
    struct strokedspline *next, *cur=head;

    while ( cur!=NULL ) {
	next = cur->next;
	chunkfree(cur,sizeof(*cur));
	cur = next;
	if ( cur==head )
    break;
    }
}

static void FreeOrigStuff(struct strokedspline *before) {

    if ( before->origminusto!=NULL )
	SplineFreeBackward(before->origminusto);
    before->origminusto = NULL;
    if ( before->origplusfrom!=NULL )
	SplineFreeForeward(before->origplusfrom);
    before->origplusfrom = NULL;
}

static void SplineMakeRound(SplinePoint *from,SplinePoint *to, real radius) {
    /* I believe this only gets called when we have a line join where the */
    /*  contour makes a U-Turn (opposite of being colinear) */
    BasePoint dir;
    SplinePoint *center;

    dir.x = (to->me.y-from->me.y)/2;
    dir.y = -(to->me.x-from->me.x)/2;
    center = SplinePointCreate((to->me.x+from->me.x)/2+dir.x,
	    (to->me.y+from->me.y)/2+dir.y);
    from->nextcp.x = from->me.x + .552*dir.x;
    from->nextcp.y = from->me.y + .552*dir.y;
    to->prevcp.x = to->me.x + .552*dir.x;
    to->prevcp.y = to->me.y + .552*dir.y;
    from->nonextcp = to->noprevcp = false;
    center->prevcp.x = center->me.x + .552*dir.y;
    center->nextcp.x = center->me.x - .552*dir.y;
    center->prevcp.y = center->me.y - .552*dir.x;
    center->nextcp.y = center->me.y + .552*dir.x;
    center->nonextcp = center->noprevcp = false;
    SplineMake3(from,center);
    SplineMake3(center,to);
}

static int DoIntersect_Splines(struct strokedspline *before,
	struct strokedspline *after, int doplus,StrokeInfo *si,SplineChar *sc,
	int force_connect ) {
    SplinePoint *beforeat, *afterat;
    int ret = true;
    int toobig = false;

    if ( doplus ) {
	beforeat = Intersect_Splines(before->plusfrom,after->plusto,&afterat);
	if ( beforeat!=NULL ) {
	    after->origplusfrom = after->plusfrom;
	    after->plusto = SplineCopyBefore(afterat,&after->plusfrom);
	    SplineFreeBetween(before->plusfrom,beforeat,true/*free before->plusfrom*/,false/* keep beforeat */);
	    before->plusfrom = beforeat;
	} else if ( before->origplusfrom!=NULL &&
		(beforeat = Intersect_Splines(before->origplusfrom,after->plusto,&afterat))!=NULL ) {
	    toobig = true;
	    after->origplusfrom = after->plusfrom;
	    after->plusto = SplineCopyBefore(afterat,&after->plusfrom);
	    SplineFreeBetween(before->plusfrom,before->plusto,true/*free plusfrom*/,false);
	    before->plusfrom = SplinePointCreate(afterat->me.x,afterat->me.y);
	    before->plusfrom->nextcp = before->plusfrom->me;
	    before->plusfrom->nonextcp = true;
	    SplineMake3(before->plusfrom,before->plusto);	/* This line goes backwards */
#if 0		/* This introduces lots of bugs, it gets invoked when it */
		/*  shouldn't, and I can't figure out how to distinguish */
	} else if ( EntirelyWithin(before->plusfrom,after->s,true,si->radius) ) {
	    /* the splines at before are all within radius units of the original */
	    /*  after spline. This means that they will make no contribution */
	    /*  to the outline. */
	    if ( before->prev!=NULL && before->prev!=after )
		ret = DoIntersect_Splines(before->prev,after,doplus,si,sc);
	    before->plusskip = true;
	    toobig = ret;
	} else if ( EntirelyWithin(after->plusto,before->s,false,si->radius) ) {
	    /* the splines at after are entirely within radius units of the original */
	    if ( after->next!=NULL && after->next!=before )
		ret = DoIntersect_Splines(before,after->next,doplus,si,sc);
	    after->plusskip = true;
	    toobig = ret;
#endif
	} else {
	    /* No intersection everything can stay as it is */
	    if ( force_connect && BasePtDistance(&after->plusto->me,&before->plusfrom->me)>3 ) {
		beforeat = SplinePointCreate(after->plusto->me.x,after->plusto->me.y);
		if ( si->join==lj_round )
		    SplineMakeRound(beforeat,before->plusfrom,si->radius);
		else
		    SplineMake3(beforeat,before->plusfrom);
		before->plusfrom = beforeat;
		toobig = true;
	    }
	    ret = false;
	}
    } else {
	afterat = Intersect_Splines(after->minusfrom,before->minusto,&beforeat);
	if ( afterat!=NULL ) {
	    after->origminusto = after->minusto;
	    after->minusfrom = SplineCopyAfter(afterat,&after->minusto);
	    SplineFreeBetween(beforeat,before->minusto,false/*keep beforeat*/,true);
	    before->minusto = beforeat;
	} else if ( before->origminusto!=NULL &&
		(afterat = Intersect_Splines(after->minusfrom,before->origminusto,&beforeat))!=NULL ) {
	    toobig = true;
	    after->origminusto = after->minusto;
	    after->minusfrom = SplineCopyAfter(afterat,&after->minusto);
	    SplineFreeBetween(before->minusfrom,before->minusto,false/*keep minusfrom*/,true);
	    before->minusto = SplinePointCreate(afterat->me.x,afterat->me.y);
	    before->minusto->ptindex = afterat->ptindex;
	    before->minusfrom->nextcp = before->minusfrom->me;
	    before->minusfrom->nonextcp = true;
	    SplineMake3(before->minusfrom,before->minusto);	/* This line goes backwards */
#if 0		/* This introduces lots of bugs, it gets invoked when it */
		/*  shouldn't, and I can't figure out how to distinguish */
	} else if ( EntirelyWithin(before->minusto,after->s,false,si->radius) ) {
	    /* the splines at before are all within radius units of the original */
	    /*  after spline. This means that they will make no contribution */
	    /*  to the outline. */
	    ret = false;
	    if ( before->prev!=NULL && before->prev!=after && before->prev!=after->next )
		ret = DoIntersect_Splines(before->prev,after,doplus,si,sc);
	    before->minusskip = true;
	    toobig = ret;
	} else if ( EntirelyWithin(after->minusfrom,before->s,true,si->radius) ) {
	    /* the splines at after are entirely within radius units of the original */
	    ret = false;
	    if ( after->next!=NULL && after->next!=before && before->prev!=after->next )
		ret = DoIntersect_Splines(before,after->next,doplus,si,sc);
	    after->minusskip = true;
	    toobig = ret;
#endif
	} else {
	    /* No intersection everything can stay as it is */
	    if ( force_connect && BasePtDistance(&after->minusfrom->me,&before->minusto->me)>3 ) {
		beforeat = SplinePointCreate(after->minusfrom->me.x,after->minusfrom->me.y);
		beforeat->ptindex = after->minusfrom->ptindex;
		if ( si->join==lj_round )
		    SplineMakeRound(before->minusto,beforeat,si->radius);
		else
		    SplineMake3(before->minusto,beforeat);
		before->minusto = beforeat;
		toobig = true;
	    }
	    ret = false;
	}
    }

    if ( toobig ) {
	si->gottoobig = si->gottoobiglocal = true;
	if ( !si->toobigwarn ) {
	    si->toobigwarn = true;
	    ff_post_error( _("Bad Stroke"), _("The stroke width is so big that the generated path\nmay intersect itself in %.100s"),
		    sc==NULL?"<nameless char>": sc->name );
	}
    }
return( ret );
}

/* Plus joins run from prev to next, minus joins run from next to prev */
/* This makes plus joins clockwise and minus joins counter */
static void StrokeJoint(SplinePoint *base,StrokeInfo *si,
	struct strokedspline *before,struct strokedspline *after,
	SplineChar *sc) {
    BasePoint nplus, nminus, pplus,pminus;
    double nangle, pangle;
    int pinner, minner;
#if 0
    double pt, mt;
    double tt, xdiff, ydiff;
#endif

    before->pangle = pangle = SplineExpand(base->prev,1,0,si,&pplus,&pminus);
    before->nangle = nangle = SplineExpand(base->next,0,0,si,&nplus,&nminus);

    if ( RealWithin(pangle,nangle,.1) || RealWithin(pangle+2*PI,nangle,.1) ||
	    RealWithin(pangle,nangle+2*PI,.1)) {
	/* If the two splines are tangent at the base, then everything is */
	/*  simple, there is no join, things match up perfectly */
	/* Um. No. If there is a sharp bend or a corner nearby then it may */
	/*  have the same effect as a corner, in extreme cases the entire */
	/*  spline may be eaten up */
	/* Actually, that's probably done best in Remove Overlap. If we try */
	/*  to do it here, we unlease lots of potentials for bugs in other */
	/*  cases */

#if 0
	if ( (xdiff = base->me.x-base->prev->from->me.x)<0 ) xdiff = -xdiff;
	if ( (ydiff = base->me.y-base->prev->from->me.y)<0 ) ydiff = -ydiff;
	if ( xdiff+ydiff==0 ) xdiff = 1;
	tt = si->radius/(2*(xdiff+ydiff));
	if ( tt>.2 ) tt = .2;
	OnEdge(&pplus,&pminus,base->next,0,1.0-tt,base->prev,
		si,&pt,&mt,NULL,NULL);
	if ( pt!=-1 )
	    DoIntersect_Splines(before,after,true,si,sc,true);
	else {
	    if ( (xdiff = base->me.x-base->next->to->me.x)<0 ) xdiff = -xdiff;
	    if ( (ydiff = base->me.y-base->next->to->me.y)<0 ) ydiff = -ydiff;
	    tt = si->radius/(2*(xdiff+ydiff));
	    if ( tt>.2 ) tt = .2;
	    OnEdge(&nplus,&nminus,base->prev,1.,tt,base->next,
		    si,NULL,NULL,&pt,&mt);
	    if ( mt!=-1 )
		DoIntersect_Splines(before,after,false,si,sc,true);
	}
#endif
	before->pinnerto = before->minnerto = -1;
    } else {
	pinner = Intersect_Lines(&before->pinterto,&pplus,
		 3*base->prev->splines[0].a+2*base->prev->splines[0].b+base->prev->splines[0].c,
		 3*base->prev->splines[1].a+2*base->prev->splines[1].b+base->prev->splines[1].c,
		&nplus,
		 base->next->splines[0].c,
		 base->next->splines[1].c,si->radius);
	minner = Intersect_Lines(&before->minterto,&pminus,
		 3*base->prev->splines[0].a+2*base->prev->splines[0].b+base->prev->splines[0].c,
		 3*base->prev->splines[1].a+2*base->prev->splines[1].b+base->prev->splines[1].c,
		&nminus,
		 base->next->splines[0].c,
		 base->next->splines[1].c,si->radius);
	if ( pinner==-1 && minner!=-1 )
	    pinner = !minner;
	before->pinnerto = pinner; before->minnerto = (pinner!=-1?!pinner:-1);
	if ( pinner==1 ) {
	    DoIntersect_Splines(before,after,true,si,sc,true);
	} else if ( pinner==0 ) {
	    DoIntersect_Splines(before,after,false,si,sc,true);
	} else {	/* splines are parallel, but moving in same dir */
	    if ( DoIntersect_Splines(before,after,true,si,sc,false)) {
		before->pinnerto = 1;
		before->minnerto = 0;
	    } else {
		if ( DoIntersect_Splines(before,after,false,si,sc,true)) {
		    before->pinnerto = 0;
		    before->minnerto = 1;
		} else
		    DoIntersect_Splines(before,after,true,si,sc,true);
	    }
	}
    }
}

static int SplineSolveForPen(Spline *s,StrokeInfo *si,double *ts,int *pinners,
	double tstart,double tend) {
    /* Find all the places at which the spline has the same slope as one of the */
    /*  edges of the pen. There can be at most 8 (we get four quadratics) */
    double a, b, c, sq, t1, t2;
    int i, cnt=0, j;
    Spline1D *xsp = &s->splines[0], *ysp = &s->splines[1];
    BasePoint pp, pm, np, nm, testp, testm;

    ts[cnt++] = tstart;
    for ( i=0; i<2; ++i ) {
	if ( i==0 ) {
	    a = 3*(ysp->a*si->c-xsp->a*si->s);
	    b = 2*(ysp->b*si->c-xsp->b*si->s);
	    c = ysp->c*si->c-xsp->c*si->s;
	} else if ( i==1 ) {
	    a = 3*(-ysp->a*si->c-xsp->a*si->s);
	    b = 2*(-ysp->b*si->c-xsp->b*si->s);
	    c = -ysp->c*si->c-xsp->c*si->s;
#if 0	/* These two are just the negatives of the first two and as such have the same roots */
	} else if ( i==2 ) {
	    a = 3*(-ysp->a*si->c+xsp->a*si->s);
	    b = 2*(-ysp->b*si->c+xsp->b*si->s);
	    c = -ysp->c*si->c+xsp->c*si->s;
	} else {
	    a = 3*(ysp->a*si->c+xsp->a*si->s);
	    b = 2*(ysp->b*si->c+xsp->b*si->s);
	    c = ysp->c*si->c+xsp->c*si->s;
#endif
	}
	sq = b*b-4*a*c;
	if ( sq==0 ) {
	    t1 = -b/(2*a);
	    t2 = -1;
	} else if ( sq>0 ) {
	    sq = sqrt(sq);
	    t1 = (-b+sq)/(2*a);
	    t2 = (-b-sq)/(2*a);
	} else
	    t1 = t2 = -1;
	if ( t1>tstart && t1<tend )
	    ts[cnt++] = t1;
	if ( t2>tstart && t2<tend )
	    ts[cnt++] = t2;
    }
    ts[cnt++] = tend;
    if ( cnt<=2 )
return(cnt);
    /* Order them */
    for ( i=1; i<cnt-1; ++i ) for ( j=i+1; j<cnt; ++j )
	if ( ts[i]>ts[j] ) {
	    double temp = ts[i];
	    ts[i] = ts[j];
	    ts[j] = temp;
	}
    /* Figure which side is inner */
    for ( i=1; i<cnt-1; ++i ) {
	SplineExpand(s,ts[i],-(ts[i]-ts[i-1])/20.,si,&pp,&pm);
	SplineExpand(s,ts[i],(ts[i+1]-ts[i])/20.,si,&np,&nm);
	SplineExpand(s,ts[i]+(ts[i+1]-ts[i])/20.,0,si,&testp,&testm);
	pinners[i] = ( (testp.x-np.x)*(pp.x-np.x)+(testp.y-np.y)*(pp.y-np.y)> 0 );
    }
return( cnt );
}

#if 0
static void SplineSetFixRidiculous(SplineSet *ss) {
    /* Make sure we don't have any splines with ridiculous control points */
    /* No control point, when projected onto the vector between the two */
    /*  end points should be far beyond either of the end points... */
    Spline *s, *first;
    double vx, vy, test, end;
    int unreasonable;

    first = NULL;
    for ( s=ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	if ( first==NULL ) first = s;
	vx = s->to->me.x-s->from->me.x; vy = s->to->me.y-s->from->me.y;
	end = vx*vx + vy*vy;
	unreasonable = false;
	test = vx*(s->from->nextcp.x-s->from->me.x) +
		vy*(s->from->nextcp.y-s->from->me.y);
	if ( test<-2*end || test>2*end ) {
	    s->from->nextcp = s->from->me;
	    s->from->nonextcp = true;
	    unreasonable = true;
	}
	test = vx*(s->to->prevcp.x-s->from->me.x) +
		vy*(s->to->prevcp.y-s->from->me.y);
	if ( test<-2*end || test>2*end ) {
	    s->to->prevcp = s->to->me;
	    s->to->noprevcp = true;
	    unreasonable = true;
	}
	if ( unreasonable )
	    SplineRefigure(s);
    }
}
#endif

static void SplineSetFixCPs(SplineSet *ss) {
    SplinePoint *sp;

    for ( sp=ss->first; ; ) {
	SPWeightedAverageCps(sp);
	if ( sp->next==NULL )
    break;
	sp = sp->next->to;
	if ( sp==ss->first )
    break;
    }
    SPLCatagorizePoints(ss);
}

static SplinePoint *SPNew(SplinePoint *base,BasePoint *pos,BasePoint *cp,int isnext) {
    SplinePoint *sp = SplinePointCreate(pos->x,pos->y);

    sp->pointtype = base->pointtype;
    /* Embolden wants these three preserved */
    sp->ptindex = base->ptindex;
    sp->ttfindex = base->ttfindex;
    sp->nextcpindex = base->nextcpindex;
    if ( isnext ) {
	sp->nextcp.x = pos->x + (cp->x-base->me.x);
	sp->nextcp.y = pos->y + (cp->y-base->me.y);
	sp->nonextcp = (sp->nextcp.x==pos->x) && (sp->nextcp.y==pos->y);
    } else {
	sp->prevcp.x = pos->x + (cp->x-base->me.x);
	sp->prevcp.y = pos->y + (cp->y-base->me.y);
	sp->noprevcp = (sp->prevcp.x==pos->x) && (sp->prevcp.y==pos->y);
    }
return( sp );
}

static void NormalizeT(TPoint *mids,int cnt,double tbase,double tend) {
    int i;

    for ( i=0; i<cnt; ++i )
	mids[i].t = (mids[i].t - tbase)/(tend - tbase);
}

static void SPFigureCP(SplinePoint *sp,double t,Spline *spline,int isnext) {
    Spline temp;
    double tn;
    Spline1D *s1;
    BasePoint off;

    s1 = &spline->splines[0];
    off.x = sp->me.x - ( ((s1->a*t+s1->b)*t+s1->c)*t+s1->d );
    s1 = &spline->splines[1];
    off.y = sp->me.y - ( ((s1->a*t+s1->b)*t+s1->c)*t+s1->d );

    if ( isnext ) {
	double s = (1.0-t);
	/* We want to renormalize the spline so that it runs from [t,1] and */
	/*  then figure what the control point at t should be */
	s1 = &spline->splines[0];
	temp.splines[0].d = s1->d + t*(s1->c + t*(s1->b + t*s1->a));
	temp.splines[0].c = s*(s1->c + t*(2*s1->b + 3*s1->a*t));
	temp.splines[0].b = s*s*(s1->b+3*s1->a*t);
#if 0
	temp.splines[0].a = s*s*s*s1->a;
#endif
	s1 = &spline->splines[1];
	temp.splines[1].d = s1->d + t*(s1->c + t*(s1->b + t*s1->a));
	temp.splines[1].c = s*(s1->c + t*(2*s1->b + 3*s1->a*t));
	temp.splines[1].b = s*s*(s1->b+3*s1->a*t);
#if 0
	temp.splines[1].a = s*s*s*s1->a;
#endif
	if ( spline->order2 ) {
	    sp->nextcp.x = temp.splines[0].d + temp.splines[0].c/2 + off.x;
	    sp->nextcp.y = temp.splines[1].d + temp.splines[1].c/2 + off.y;
	} else {
	    sp->nextcp.x = temp.splines[0].d + temp.splines[0].c/3 + off.x;
	    sp->nextcp.y = temp.splines[1].d + temp.splines[1].c/3 + off.y;
	}
	sp->nonextcp = false;
    } else {
	/* We want to renormalize the spline so that it runs from [0,t] and */
	/*  then figure what the control point at t should be */
	temp = *spline;
	temp.splines[0].c *= t;		temp.splines[1].c *= t;
	tn = t*t;
	temp.splines[0].b *= tn;	temp.splines[1].b *= tn;
#if 0
	tn *= t;
	temp.splines[0].a *= tn;	temp.splines[1].a *= tn;
#endif
	if ( spline->order2 ) {
	    sp->prevcp.x = temp.splines[0].d + temp.splines[0].c/2 + off.x;
	    sp->prevcp.y = temp.splines[1].d + temp.splines[1].c/2 + off.y;
	} else {
	    sp->prevcp.x = temp.splines[0].d + (2*temp.splines[0].c+temp.splines[0].b)/3 + off.x;
	    sp->prevcp.y = temp.splines[1].d + (2*temp.splines[1].c+temp.splines[1].b)/3 + off.y;
	}
	sp->noprevcp = false;
    }
}

static void SPFigurePlusCP(SplinePoint *sp,double t,Spline *spline,int isnext) {
    SplinePoint dummy;

    /* Plus splines run in the oposite direction */
    dummy = *sp;
    SPFigureCP(&dummy,t,spline,!isnext);
    if ( isnext ) {
	sp->nextcp = dummy.prevcp;
	sp->nonextcp = false;
    } else {
	sp->prevcp = dummy.nextcp;
	sp->noprevcp = false;
    }
}

static int Overlaps(TPoint *expanded,TPoint *inner,double rsq) {
    double len;
    BasePoint dir;
    
    dir.x = (expanded->x-inner->x); dir.y = (expanded->y-inner->y);
    len = (dir.x*dir.x) + (dir.y*dir.y);
    if ( len>=rsq )
return( false );
    len = sqrt(rsq/len);
    expanded->x = inner->x + len*dir.x;
    expanded->y = inner->y + len*dir.y;
return( true );
}

#define Approx	10

static struct strokedspline *_SplineSetApprox(SplineSet *spl,StrokeInfo *si,SplineChar *sc) {
    struct strokedspline *head=NULL, *last=NULL, *cur;
    int max=Approx;
    TPoint *pmids=galloc(max*sizeof(TPoint)),
	    *mmids=galloc(max*sizeof(TPoint)),
	    *mids=galloc(max*sizeof(TPoint));
    uint8 *knots=galloc(max);
    BasePoint pto, mto, pfrom, mfrom;
    double approx, xdiff, ydiff, loopdiff;
    Spline *spline, *first;
    int i,j,k;
    SplinePoint *p_to, *m_to, *p_from, *m_from;
    int cnt, anyknots;
    double ts[9];
    BasePoint m,p,temp;
    double mt1, pt1, mt2, pt2, rsq;
    int pinners[10];
    int mwascovered, pwascovered;
    enum knot_type { kt_knot=1, kt_pgood=2, kt_mgood=4 };
    int toobig;

    first = NULL;
    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	cur = chunkalloc(sizeof(struct strokedspline));
	if ( last==NULL )
	    head = cur;
	else {
	    last->next = cur;
	    cur->prev = last;
	}
	last = cur;
	cur->s = spline;
	SplineIsLinearMake(spline);
	SplineExpand(spline,0,0,si,&pto,&mfrom);
	SplineExpand(spline,1,0,si,&pfrom,&mto);
	cur->minusfrom = SPNew(spline->from,&mfrom,&spline->from->nextcp,true);
	cur->plusto = SPNew(spline->from,&pto,&spline->from->nextcp,false);
	cur->minusto = SPNew(spline->to,&mto,&spline->to->prevcp,false);
	cur->plusfrom = SPNew(spline->to,&pfrom,&spline->to->prevcp,true);

	if ( si->stroke_type == si_caligraphic ) {
	    /* At each t where the spline is tangent to one of the pen-angles */
	    /*  we need to figure out which side is inner and which is outer */
	    /*  the outer side gets a copy of the appropriate pen side (with corner points tangent) */
	    /*  the inner side is going to be a single corner point at the */
	    /*  intersection of the splines from the two corners */
	    /* And if (god help us) we've got a point of inflection here then */
	    /*  we get half the pen on each side */
	    /* I ignore the case of a point of inflection, and I don't */
	    /*  find the real intersection point, I just guess that it is */
	    /*  near the mid point of the pen */
	    cnt = SplineSolveForPen(spline,si,ts,pinners+1,0,1);
	    p_to = m_to = NULL;
	    p_from = NULL;		/* Make gcc happy */
	    for ( j=1; j<cnt; ++j ) {
		for ( i=0; i<Approx; ++i ) {
		    real t = ts[j-1] + (i+1)*(ts[j]-ts[j-1])/(Approx+1);
		    mmids[i].t = (i+1)/(double) (Approx+1); pmids[i].t = 1-mmids[i].t;
		    SplineExpand(spline,t,0,si,&p,&m);
		    pmids[i].x = p.x; pmids[i].y = p.y;
		    mmids[i].x = m.x; mmids[i].y = m.y;
		}
		if ( j==1 ) {
		    p_to = cur->plusto; m_from = cur->minusfrom;
		} else if ( pinners[j-1] ) {
		    p_to = p_from;
		    SplineExpand(spline,ts[j-1],(ts[j-1]-ts[j-2])/20.,si,&p,&m);
		    m_from = SplinePointCreate(m.x,m.y);
		    m_from->pointtype = pt_tangent;
		    SplineMake3(m_to,m_from);
		} else {
		    m_from = m_to;
		    SplineExpand(spline,ts[j-1],(ts[j-1]-ts[j-2])/20.,si,&p,&m);
		    p_to = SplinePointCreate(p.x,p.y);
		    p_to->pointtype = pt_tangent;
		    SplineMake3(p_to,p_from);
		}
		if ( j==cnt-1 ) {
		    p_from = cur->plusfrom;
		    m_to = cur->minusto;
		} else if ( pinners[j] ) {
		    SplineExpand(spline,ts[j],(ts[j+1]-ts[j-1])/20.,si,&p,&m);
		    SplineExpand(spline,ts[j],-(ts[j+1]-ts[j-1])/20.,si,&temp,&m);
		    p_from = SplinePointCreate((p.x+temp.x)/2,(p.y+temp.y)/2);
		    p_from->pointtype = pt_corner;
		    m_to = SplinePointCreate(m.x,m.y);
		    m_to->pointtype = pt_tangent;
		} else {
		    SplineExpand(spline,ts[j],(ts[j+1]-ts[j-1])/20.,si,&p,&m);
		    SplineExpand(spline,ts[j],-(ts[j+1]-ts[j-1])/20.,si,&p,&temp);
		    p_from = SplinePointCreate(p.x,p.y);
		    p_from->pointtype = pt_tangent;
		    m_to = SplinePointCreate((m.x+temp.x)/2,(m.y+temp.y)/2);
		    m_to->pointtype = pt_corner;
		}
		ApproximateSplineFromPoints(p_from,p_to,pmids,Approx,false);
		ApproximateSplineFromPoints(m_from,m_to,mmids,Approx,false);
		if ( m_from!=cur->minusfrom && m_from->pointtype!=pt_corner )
		    m_from->pointtype = pt_tangent;
	    }
	} else {
	    /* Figure out where the curve starts to bend sharply, and add */
	    /* New points there. I used to strip out the curve where it */
	    /*  overlapped itself, but I think that's better done by remove */
	    /*  overlap rather than here */
	    if ( (xdiff = spline->to->me.x-spline->from->me.x)<0 ) xdiff = -xdiff;
	    if ( (ydiff = spline->to->me.y-spline->from->me.y)<0 ) ydiff = -ydiff;
	    loopdiff = (xdiff+ydiff==0) ? .1 : 1.0/(4*(xdiff+ydiff)/si->radius);
	    approx = rint(1.0/loopdiff);
	    if ( approx<0 || approx>3000 ) approx=3000;
	    if ( approx>max ) {
		max = approx+10;
		pmids = grealloc(pmids,max*sizeof(TPoint));
		mmids = grealloc(mmids,max*sizeof(TPoint));
		mids = grealloc(mids,max*sizeof(TPoint));
		knots = grealloc(knots,max);
	    }

	    mwascovered = pwascovered = false;
	    toobig = false;
	    for ( i=0; i<approx; ++i ) {
		real t = (i+1)/(approx+1);
		SplineExpand(spline,t,0,si,&p,&m);
		OnEdge(&p,&m,spline,t,t,spline,si,&pt1,&mt1,&pt2,&mt2);
		knots[i] = 0;
		if ( ((pt1!=-1 || pt2!=-1) && !pwascovered && i!=0) ||
			((mt1!=-1 || mt2!=-1) && !mwascovered && i!=0))
		    knots[i] = kt_knot;
		if ( ((pt1==-1 && pt2==-1) && pwascovered && i!=0 ) ||
			((mt1==-1 && mt2==-1) && mwascovered && i!=0 )) {
		    if ( knots[i-1]&kt_knot )
			knots[i] = kt_knot;
		    else
			knots[i-1] |= kt_knot;
		}
		pwascovered = pt1!=-1 || pt2!=-1;
		mwascovered = mt1!=-1 || mt2!=-1;
		pmids[i].t = 1-(i+1)/(approx+1);
		pmids[i].x = p.x; pmids[i].y = p.y;
		mmids[i].t = (i+1)/(approx+1);
		mmids[i].x = m.x; mmids[i].y = m.y;
		mids[i].x = (m.x+p.x)/2; mids[i].y = (m.y+p.y)/2;
		/*if ( !pwascovered )*/ knots[i] |= kt_pgood;
		/*if ( !mwascovered )*/ knots[i] |= kt_mgood;
		if ( pwascovered || mwascovered )
		    toobig = true;
	    }
	    rsq = si->radius*si->radius;
	    for ( i=0; i<approx; ++i ) {
		for ( j=1; j<approx/2; ++j ) {
		    if ( i+j<approx ) {
			Overlaps(&mmids[i],&mids[i+j],rsq);
			Overlaps(&pmids[i],&mids[i+j],rsq);
		    }
		    if ( i-j>0 ) {
			Overlaps(&mmids[i],&mids[i-j],rsq);
			Overlaps(&pmids[i],&mids[i-j],rsq);
		    }
		}
	    }
	    anyknots = false;
	    for ( i=0; i<approx; ++i ) if ( knots[i]&kt_knot ) { anyknots=true; break; }
	    if ( toobig ) {
		si->gottoobig = si->gottoobiglocal = true;
		if ( !si->toobigwarn ) {
		    si->toobigwarn = true;
		    ff_post_error( _("Bad Stroke"), _("The stroke width is so big that the generated path\nmay intersect itself in %.100s"),
			    sc==NULL?"<nameless char>": sc->name );
		}
	    }

	    /* Look for any sharp bends, they give us problems which are */
	    /*  eased by creating a new point. */
	    if ( !anyknots ) {
		double radius = si->radius;
		si->radius *= 2;
		mwascovered = pwascovered = false;
		for ( i=0; i<approx; ++i ) {
		    real t = (i+1)/(approx+1);
		    SplineExpand(spline,t,0,si,&p,&m);
		    OnEdge(&p,&m,spline,t,t,spline,si,&pt1,&mt1,&pt2,&mt2);
		    if ( ((pt1!=-1 || pt2!=-1) && !pwascovered && i!=0) ||
			    ((mt1!=-1 || mt2!=-1) && !mwascovered && i!=0))
			knots[i] |= kt_knot;
		    if ( ((pt1==-1 && pt2==-1) && pwascovered && i!=0 ) ||
			    ((mt1==-1 && mt2==-1) && mwascovered && i!=0 )) {
			if ( knots[i-1]&kt_knot )
			    knots[i] |= kt_knot;
			else
			    knots[i-1] |= kt_knot;
		    }
		    pwascovered = pt1!=-1 || pt2!=-1;
		    mwascovered = mt1!=-1 || mt2!=-1;
		}
		si->radius = radius;
	    }

	    p_to = cur->plusto;
	    m_from = cur->minusfrom;
	    for ( i=0, j=1; i<approx; ++i ) {
		if ( knots[i]&kt_knot ) {
		    for ( k=i+1; k<approx && !(knots[k]&kt_knot); ++k );
		    if ( i>0 && (knots[i-1]&kt_mgood) ) {
			if ( i+1<approx && !(knots[i+1]&kt_mgood) && k<approx )
			    m_to = SplinePointCreate((mmids[i].x+mmids[k].x)/2,(mmids[i].y+mmids[k].y)/2);
			else
			    m_to = SplinePointCreate(mmids[i].x,mmids[i].y);
			m_to->pointtype = pt_corner;
			SPFigureCP(m_from,(j)/(approx+1),spline,true);
			SPFigureCP(m_to,(i+1)/(approx+1),spline,false);
			NormalizeT(mmids+j,i-j,mmids[j-1].t,mmids[i].t);
			ApproximateSplineFromPointsSlopes(m_from,m_to,mmids+j,i-j,false);
			m_from = m_to;
		    }

		    if ( i>0 && (knots[i-1]&kt_pgood) ) {
			if ( i+1<approx && !(knots[i+1]&kt_pgood) && k<approx )
			    p_from = SplinePointCreate((pmids[i].x+pmids[k].x)/2,(pmids[i].y+pmids[k].y)/2);
			else
			    p_from = SplinePointCreate(pmids[i].x,pmids[i].y);
			p_from->pointtype = pt_corner;
			SPFigurePlusCP(p_to,j/(approx+1),spline,false);
			SPFigurePlusCP(p_from,(i+1)/(approx+1),spline,true);
			NormalizeT(pmids+j,i-j,pmids[i].t,pmids[j-1].t);
			ApproximateSplineFromPointsSlopes(p_from,p_to,pmids+j,i-j,false);
			p_to = p_from;
		    }

		    j=i+1;
		}
	    }

	    if ( j!=1 ) {
		NormalizeT(pmids+j,i-j,0.0,pmids[j-1].t);
		NormalizeT(mmids+j,i-j,mmids[j-1].t,1.0);
		SPFigureCP(m_from,(j)/(approx+1),spline,true);
		SPFigurePlusCP(p_to,(j)/(approx+1),spline,false);
	    }
	    ApproximateSplineFromPointsSlopes(cur->plusfrom,p_to,pmids+j,i-j,false);
	    ApproximateSplineFromPointsSlopes(m_from,cur->minusto,mmids+j,i-j,false);
	}
	if ( spline->to->next==NULL ) {
	    /* Done */
    break;
	}
	if ( first==NULL ) first = spline;
    }
    if ( spline==first ) {
	head->prev = last;
	last->next = head;
    }
    free(mmids); free(pmids); free(knots); free(mids);
return( head );
}

static void SPLCheckValidity(SplineSet *ss) {
    SplinePoint *sp, *nsp;

    for ( sp=ss->first; ; sp = nsp ) {
	if ( sp->next==NULL )
    break;
	nsp = sp->next->to;
	if ( nsp->prev != sp->next || sp->next->from!=sp )
	    IError("Bad SPL");
	if ( nsp==ss->first )
    break;
    }

    for ( sp=ss->last; ; sp = nsp ) {
	if ( sp->prev==NULL )
    break;
	nsp = sp->prev->from;
	if ( nsp->next != sp->prev || sp->prev->to!=sp )
	    IError("Bad SPL");
	if ( nsp==ss->last )
    break;
    }
}

static SplineSet *_SplineSetStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc) {
    SplineSet *ssplus, *ssminus;
    int reversed = false;
    struct strokedspline *head, *cur, *first, *lastp, *lastm;
    Spline *s1, *s2;

    si->gottoobiglocal = false;

    if ( spl->first->next==NULL || spl->first->next->to==spl->first ) {
	/* Only one point in the SplineSet. */
	ssplus = chunkalloc(sizeof(SplineSet));
	SinglePointStroke(spl->first,si,&ssplus->first,&ssplus->last);
return( ssplus );
    }

    SplineSetAddExtrema(NULL,spl,ae_all,1000/* Not used*/);

    if ( spl->first==spl->last && spl->first->next!=NULL ) {
	/* My routine gets screwed up by counter-clockwise triangles */
	if ( SplinePointListIsClockwise(spl)==0 ) {
	    reversed = true;
	    SplineSetReverse(spl);
	}
    }

    head = cur = _SplineSetApprox(spl,si,sc);

    first = NULL;
    for ( cur=head; cur!=NULL && cur!=first; cur=cur->next ) {
	if ( first==NULL ) first = cur;
	if ( cur->s->to->next!=NULL )
	    StrokeJoint(cur->s->to,si,cur,cur->next,sc);
	FreeOrigStuff(cur);
    }
    FreeOrigStuff(head);	/* normally gets freed when we look at the next item on list. But we did that for head first */

    /* Finish off intersections, before doing joins */
    if ( spl->first->prev==NULL ) {
	StrokeEndComplete(head,si,true);
	for ( cur=head; cur->next!=NULL; cur=cur->next );
	StrokeEndComplete(cur,si,false);
    }

    lastp = lastm = head;
    if ( lastp->plusskip ) lastp = NULL;
    if ( lastm->minusskip ) lastm = NULL;

    first = NULL;
    for ( cur=head; cur!=NULL && cur!=first; cur=cur->next ) {
	real factor = si->factor==NULL ? 1.0 : (si->factor)(si->data,cur->s,1.0);
	if ( first==NULL ) first = cur;
	
	if ( cur->s->to->next!=NULL ) {
	    if ( !cur->plusskip ) lastp = cur;
	    if ( lastp!=NULL && !cur->next->plusskip ) {
		if ( cur->pinnerto==-1 )
		    MSP(cur->next->plusto,&lastp->plusfrom,&lastp->plusto);
		else if ( cur->pinnerto )
		    MSP(cur->next->plusto,&lastp->plusfrom,&lastp->plusto);
		else if ( cur==lastp )
		    MakeJoints(cur->next->plusto,cur->plusfrom,si,&cur->pinterto,
			    &cur->s->to->me,-1,cur->pangle,cur->nangle,factor);
		else
		    IError("Lastp not cur" );
	    }
	    if ( !cur->minusskip ) lastm = cur;
	    if ( lastm!=NULL && !cur->next->minusskip ) {
		if ( cur->minnerto==-1 )
		    MSP(lastm->minusto,&cur->next->minusfrom,&cur->next->minusto);
		else if ( cur->minnerto )
		    MSP(lastm->minusto,&cur->next->minusfrom,&cur->next->minusto);
		else if ( cur==lastm )
		    MakeJoints(lastm->minusto,cur->next->minusfrom,si,&cur->minterto,
			    &cur->s->to->me,1,PI+cur->nangle,PI+cur->pangle,factor);
		else
		    IError("Lastm not cur");
	    }
	}
    }

    for ( cur=head; cur!=NULL && cur->plusskip; ) { cur=cur->next; if ( cur==head ) cur=NULL; }
    if ( cur!=NULL ) {
	ssplus = chunkalloc(sizeof(SplineSet));
	ssplus->first = ssplus->last = cur->plusfrom;
	SplineSetFixCPs(ssplus);
	SPLCheckValidity(ssplus);
    } else
	/* It is possible to have a contour completely swallowed by the pen */
	ssplus = NULL;
    for ( cur=head; cur!=NULL && cur->minusskip; ) { cur=cur->next; if ( cur==head ) cur=NULL; }
    if ( spl->first==spl->last && cur!=NULL ) {
	ssminus = chunkalloc(sizeof(SplineSet));
	ssminus->first = ssminus->last = cur->minusfrom;
	SPLCheckValidity(ssminus);
	/*SplineSetFixRidiculous(ssplus); SplineSetFixRidiculous(ssminus);*/
	SplineSetFixCPs(ssminus);
	if ( reversed ) {
	    SplineSet *temp = ssplus;
	    ssplus = ssminus;
	    ssminus = temp;
	}
	SplineSetReverse(ssminus);
	if ( ssplus != NULL )
	    SplineSetReverse(ssplus);
	if ( si->removeinternal && ssplus!=NULL ) {
	    SplinePointListFree(ssminus);
	} else if ( si->removeexternal ) {
	    SplinePointListFree(ssplus);
	    SplineSetReverse(ssminus);
	    ssplus = ssminus;
	} else {
	    if ( ssplus != NULL )
		ssplus->next = ssminus;
	    else
		ssplus = ssminus;
	    /* I used to do a splineset correct dir here on both, but */
	    /*  that doesn't work always if a contour self intersects */
	    /* I think it should always be correct */
	}
	/* I can't always detect an overlap, so let's always do the remove */
		/* Sigh, no. That is still too dangerous */
	if ( si->removeoverlapifneeded && ssplus!=NULL && SplineSetIntersect(ssplus,&s1,&s2))
	    ssplus = SplineSetRemoveOverlap(sc,ssplus,over_remove);
	if ( reversed )		/* restore original, just in case we want it */
	    SplineSetReverse(spl);
    } else if ( si->stroke_type==si_std || si->stroke_type==si_elipse )
	SplineSetReverse(ssplus);
    StrokedSplineFree(head);
return( ssplus );
}

#if 0
static void BisectTurners(SplineSet *spl) {
    Spline *first, *s, *next;
    double len,lenf,lent, dott,dotf;

    /* Also if we have a spline which turns through about 180 degrees */
    /*  our approximations degrade. So bisect any such splines */
    first = NULL;
    for ( s = spl->first->next; s!=NULL && s!=first; s=next ) {
	next = s->to->next;
	if ( first==NULL ) first = s;
	len = sqrt( (s->from->me.x-s->to->me.x)*(s->from->me.x-s->to->me.x) +
		    (s->from->me.y-s->to->me.y)*(s->from->me.y-s->to->me.y) );
	lenf= sqrt( (s->from->me.x-s->from->nextcp.x)*(s->from->me.x-s->from->nextcp.x) +
		    (s->from->me.y-s->from->nextcp.y)*(s->from->me.y-s->from->nextcp.y) );
	dotf = ((s->from->me.x-s->to->me.x)*(s->from->me.x-s->from->nextcp.x) +
		(s->from->me.y-s->to->me.y)*(s->from->me.y-s->from->nextcp.y))/
		(len*lenf);
	lent= sqrt( (s->to->prevcp.x-s->to->me.x)*(s->to->prevcp.x-s->to->me.x) +
		    (s->to->prevcp.y-s->to->me.y)*(s->to->prevcp.y-s->to->me.y) );
	dott = ((s->from->me.x-s->to->me.x)*(s->to->prevcp.x-s->to->me.x) +
		(s->from->me.y-s->to->me.y)*(s->to->prevcp.y-s->to->me.y))/
		(len*lent);
	dotf = acos(dotf); dott = acos(dott);
	if ( dotf+dott > PI/2 )
	    SplineBisect(s,.5);
    }
}

void SSBisectTurners(SplineSet *spl) {
    while ( spl!=NULL ) {
	BisectTurners(spl);
	spl = spl->next;
    }
}
#endif

#ifdef LOCAL_DEBUG
static void touchall(SplineSet *spl) {
    SplinePoint *sp;

    for ( sp=spl->last; sp!=NULL; ) {
	if ( sp->prev==NULL )
    break;
	sp = sp->prev->from;
	if ( sp==spl->last )
    break;
    }
    for ( sp=spl->first; sp!=NULL; ) {
	if ( sp->next==NULL )
    break;
	sp = sp->next->to;
	if ( sp==spl->first )
    break;
    }
}

/* I know there's no prototype. It's just a useful routine to make valgrind */
/*  check consistency */
void splstouchall(SplineSet *ss) {
    while ( ss!=NULL ) {
	touchall(ss);
	ss = ss->next;
    }
}
#endif

static SplineSet *SSRemoveUTurns(SplineSet *base, StrokeInfo *si) {
    /* All too often in MetaPost output splines have tiny cps which */
    /*  make the slope at the end-points irrelevant when looking at */
    /*  the curve.  Since we assume the slope at the end-points is */
    /*  similar to the slope at t=.01 this confuses us greatly and */
    /*  produces nasty results. In this case try to approximate a new */
    /*  spline with very different cps. Note: We break continuity! */
    /* A special case of this is the following: */
    /* My stroking algorithem gets confused by sharp turns. For example */
    /*  if we have a spline which is all in a line, but the control points */
    /*  are such that it doubles back on itself ( "* +   * +", ie. cps */
    /*  outside of the points) then things get very unhappy */
    SplineSet *spl= base;
    Spline *first, *s, *next, *snew;
    double dx,dy, offx,offy, diff, n,l, slen, len, bound;
    int linear, bad, i, cnt;
    SplinePoint fakefrom, faketo;
    TPoint *tps;

    bound = si->radius*si->radius;
    first = NULL;
  if ( spl->first->next!=NULL && !spl->first->next->order2 )
    for ( s = spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;

	bad = false;
	dx = s->to->me.x-s->from->me.x;
	dy = s->to->me.y-s->from->me.y;
	slen = dx*dx + dy*dy;

	offx = s->from->nextcp.x-s->from->me.x;
	offy = s->from->nextcp.y-s->from->me.y;
	l= offx*dx + offy*dy;
	if ( l<0 ) {
	    l = -l;
	    if ( (n= offx*dy - offy*dx)<0 ) n = -n;
	    len = offx*offx + offy*offy;
	    if ( (n/l>2*len/si->radius || (n>l/3 && s->from->prev==NULL )) && len<bound && len< slen/4 )
		bad = 1;
	}

	offx = s->to->me.x-s->to->prevcp.x;
	offy = s->to->me.y-s->to->prevcp.y;
	l= offx*dx + offy*dy;
	if ( l<0 ) {
	    l = -l;
	    if ( (n= offx*dy - offy*dx)<0 ) n = -n;
	    len = offx*offx + offy*offy;
	    if ( (n/l>2*len/si->radius || (n>l/3 && s->to->next==NULL)) && len<bound && len< slen/4 )
		bad |= 2;
	}

	if ( bad ) {
	    fakefrom = *s->from; fakefrom.next = fakefrom.prev = NULL;
	    faketo   = *s->to;   faketo.next   = faketo.prev   = NULL;

	    slen = sqrt(slen);
	    dx /= slen; dy/=slen;

	    if ( bad&1 ) {		/* from->nextcp is nasty */
		offx = s->from->nextcp.x-s->from->me.x;
		offy = s->from->nextcp.y-s->from->me.y;
		len = sqrt(offx*offx + offy*offy);
		offx /= len; offy/=len;

		n = offx*dy - offy*dx;
		fakefrom.nextcp.x = fakefrom.me.x + slen*dx + 3*len*dy;
		fakefrom.nextcp.y = fakefrom.me.y + slen*dy - 3*len*dx;
	    }

	    if ( bad&2 ) {		/* from->nextcp is nasty */
		offx = s->to->prevcp.x-s->to->me.x;
		offy = s->to->prevcp.y-s->to->me.y;
		len = sqrt(offx*offx + offy*offy);
		offx /= len; offy/=len;

		n = offx*dy - offy*dx;
		faketo.prevcp.x = faketo.me.x - slen*dx + 3*len*dy;
		faketo.prevcp.y = faketo.me.y - slen*dy - 3*len*dx;
	    }

	    if (( cnt = slen/2)<10 ) cnt = 10;
	    tps = galloc(cnt*sizeof(TPoint));
	    for ( i=0; i<cnt; ++i ) {
		double t = ((double) (i+1))/(cnt+1);
		tps[i].t = t;
		tps[i].x = ((s->splines[0].a*t + s->splines[0].b)*t + s->splines[0].c)*t + s->splines[0].d;
		tps[i].y = ((s->splines[1].a*t + s->splines[1].b)*t + s->splines[1].c)*t + s->splines[1].d;
	    }
	    snew = ApproximateSplineFromPointsSlopes(&fakefrom,&faketo,tps,cnt,false);
	    snew->from = s->from;
	    snew->to = s->to;
	    snew->from->next = snew;
	    snew->to->prev = snew;
	    snew->from->nextcp = fakefrom.nextcp;
	    snew->from->nonextcp = fakefrom.nonextcp;
	    if ( bad&1 ) snew->from->pointtype = pt_corner;
	    snew->to->prevcp = faketo.prevcp;
	    snew->to->noprevcp = faketo.noprevcp;
	    if ( bad&2 ) snew->to->pointtype = pt_corner;
	    if ( first==s ) first=snew;
	    SplineFree(s);
	    free(tps);
	    s = snew;
	}
    }

    first = NULL;
    for ( s = spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	dx = s->to->me.x-s->from->me.x;
	dy = s->to->me.y-s->from->me.y;
	offx = s->from->nextcp.x-s->from->me.x;
	offy = s->from->nextcp.y-s->from->me.y;
	if ( offx*dx + offy*dy<0 ) {
	    diff = offx*dy-offy*dx;
	    linear = ( diff<1 && diff>-1 );
	    if ( offx<0 ) offx = -offx;
	    if ( offy<0 ) offy = -offy;
	    if ( offx+offy<1 || linear ) {
		s->from->nextcp = s->from->me;
		s->from->nonextcp = true;
		if ( s->from->pointtype == pt_curve || s->from->pointtype == pt_hvcurve )
		    s->from->pointtype = pt_corner;
		if ( s->order2 ) {
		    s->to->prevcp = s->to->me;
		    s->to->noprevcp = true;
		    if ( s->to->pointtype==pt_curve || s->to->pointtype == pt_hvcurve )
			s->to->pointtype = pt_corner;
		}
		SplineRefigure(s);
	    }
	}
	offx = s->to->me.x-s->to->prevcp.x;
	offy = s->to->me.y-s->to->prevcp.y;
	if ( offx*dx + offy*dy<0 ) {
	    diff = offx*dy-offy*dx;
	    linear = ( diff<1 && diff>-1 );
	    if ( offx<0 ) offx = -offx;
	    if ( offy<0 ) offy = -offy;
	    if ( offx+offy<1 || linear ) {
		s->to->prevcp = s->to->me;
		s->to->noprevcp = true;
		if ( s->to->pointtype==pt_curve || s->to->pointtype == pt_hvcurve )
		    s->to->pointtype = pt_corner;
		if ( s->order2 ) {
		    s->from->nextcp = s->from->me;
		    s->from->nonextcp = true;
		    if ( s->from->pointtype == pt_curve || s->from->pointtype == pt_hvcurve )
			s->from->pointtype = pt_corner;
		}
		SplineRefigure(s);
	    }
	}
    }

    /* Zero length splines are bad too */
    /* As are splines of length .000003 */
    first = NULL;
    for ( s = spl->first->next; s!=NULL && s!=first; s=next ) {
	if ( first==NULL ) first = s;
	next = s->to->next;
	if ( s->from->nonextcp && s->to->noprevcp && s!=next &&
		s->from->me.x >= s->to->me.x-.1 && s->from->me.x <= s->to->me.x+.1 &&
		s->from->me.y >= s->to->me.y-.1 && s->from->me.y <= s->to->me.y+.1 ) {
	    s->from->next = next;
	    if ( next!=NULL ) {
		s->from->nextcp = next->from->nextcp;
		s->from->nonextcp = next->from->nonextcp;
		s->from->nextcpdef = next->from->nextcpdef;
		next->from = s->from;
	    }
	    SplinePointCatagorize(s->from);
	    if ( spl->last == s->to ) {
		if ( next==NULL )
		    spl->last = s->from;
		else
		    spl->first = spl->last = s->from;
	    }
	    if ( spl->first==s->to ) spl->first = s->from;
	    if ( spl->last==s->to ) spl->last = s->from;
	    SplinePointFree(s->to);
	    SplineFree(s);
	    if ( first==s ) first = NULL;
	}
    }

#if 0
    BisectTurners(spl);
#endif
return( base );
}

static void SSRemoveColinearPoints(SplineSet *ss) {
    SplinePoint *sp, *nsp, *nnsp;
    BasePoint dir, ndir;
    double len;
    int removed;

    sp = ss->first;
    if ( sp->prev==NULL )
return;
    nsp = sp->next->to;
    if ( nsp==sp )
return;
    dir.x = nsp->me.x - sp->me.x; dir.y = nsp->me.y - sp->me.y;
    len = dir.x*dir.x + dir.y*dir.y;
    if ( len!=0 ) {
	len = sqrt(len);
	dir.x /= len; dir.y /= len;
    }
    nnsp = nsp->next->to;
    if ( nnsp==sp )
return;
    memset(&ndir,0,sizeof(ndir));
    forever {
	removed = false;
	if ( nsp->next->islinear ) {
	    ndir.x = nnsp->me.x - nsp->me.x; ndir.y = nnsp->me.y - nsp->me.y;
	    len = ndir.x*ndir.x + ndir.y*ndir.y;
	    if ( len!=0 ) {
		len = sqrt(len);
		ndir.x /= len; ndir.y /= len;
	    }
	}
	if ( sp->next->islinear && nsp->next->islinear ) {
	    double dot =dir.x*ndir.y - dir.y*ndir.x;
	    if ( dot<.001 && dot>-.001 ) {
		sp->next->to = nnsp;
		nnsp->prev = sp->next;
		SplineRefigure(sp->next);
		SplineFree(nsp->next);
		SplinePointFree(nsp);
		if ( ss->first==nsp ) ss->first = sp;
		if ( ss->last ==nsp ) ss->last  = sp;
		removed = true;
	    } else
		sp = nsp;
	} else
	    sp = nsp;
	dir = ndir;
	nsp = nnsp;
	nnsp = nsp->next->to;
	if ( !removed && sp==ss->first )
    break;
    }
}

static void SSesRemoveColinearPoints(SplineSet *ss) {
    while ( ss!=NULL ) {
	SSRemoveColinearPoints(ss);
	ss = ss->next;
    }
}

SplineSet *SplineSetStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc) {
    SplineSet *ret, *temp, *temp2;
    SplineSet *order3 = NULL;

    if ( spl->first->next!=NULL && spl->first->next->order2 )
	order3 = spl = SSPSApprox(spl);
    if ( si->radius==0 )
	si->radius=1;
    temp2 = SSRemoveUTurns(SplinePointListCopy(spl),si);
    if ( si->stroke_type == si_elipse ) {
	real trans[6], factor;
	StrokeInfo si2;
	trans[0] = trans[3] = si->c;
	trans[1] = -si->s;
	trans[2] = si->s;
	trans[4] = trans[5] = 0;
	factor = si->radius/si->minorradius;
	trans[0] *= factor; trans[2] *= factor;
	temp = SplinePointListCopy(temp2);
#if 0 
	BisectTurners(temp);
#endif
	temp = SplinePointListTransform(temp,trans,true);
	si2 = *si;
	si2.stroke_type = si_std;
	ret = SplineSetStroke(temp,&si2,sc);
	SplinePointListFree(temp);
	trans[0] = trans[3] = si->c;
	trans[1] = si->s;
	trans[2] = -si->s;
	trans[4] = trans[5] = 0;
	factor = si->minorradius/si->radius;
	trans[0] *= factor; trans[1] *= factor;
	ret = SplinePointListTransform(ret,trans,true);
    } else
	ret = _SplineSetStroke(temp2,si,sc);
    SplinePointListFree(temp2);
    if ( order3!=NULL ) {
	temp = SplineSetsTTFApprox(ret);
	SplinePointListsFree(ret);
	SplinePointListFree(order3);
	ret = temp;
    }
    /* We tend to get (small) rounding errors */
    SplineSetsRound2Int(ret,1024.,false,false);
    /* If we use butt line caps or miter joins then we will likely have */
    /*  some spurious colinear points. If we do, remove them */
    SSesRemoveColinearPoints(ret);
return( ret );
}

    /* for angles between [penangle,penangle+90] use (-r,t/2) rotated by penangle */
    /*			  [penangle+90,penangle+180] use (-r,-t/2) */
    /*			  [penangle+180,penangle+270] use (r,-t/2) */
    /*			  [penangle+270,penangle] use (r,t/2) */

SplineSet *SSStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc) {
    SplineSet *head=NULL, *last=NULL, *cur;
    /*int was_clock = true;*/

    for ( ; spl!=NULL; spl = spl->next ) {
	cur = SplineSetStroke(spl,si,sc);
	if ( cur==NULL )		/* Can happen if stroke overlaps itself into nothing */
    continue;
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	while ( cur->next!=NULL ) cur = cur->next;
	last = cur;
    }
return( head );
}

#include "baseviews.h"

void FVStrokeItScript(void *_fv, StrokeInfo *si,int pointless_argument) {
    FontViewBase *fv = _fv;
    int layer = fv->active_layer;
    SplineSet *temp;
    int i, cnt=0, gid;
    SplineChar *sc;

    for ( i=0; i<fv->map->enccount; ++i ) if ( (gid=fv->map->map[i])!=-1 && fv->sf->glyphs[gid]!=NULL && fv->selected[i] )
	++cnt;
    ff_progress_start_indicator(10,_("Stroking..."),_("Stroking..."),0,cnt,1);

    SFUntickAll(fv->sf);
    for ( i=0; i<fv->map->enccount; ++i ) {
	if ( (gid=fv->map->map[i])!=-1 && (sc = fv->sf->glyphs[gid])!=NULL &&
		!sc->ticked && fv->selected[i] ) {
	    sc->ticked = true;
	    if ( sc->parent->multilayer ) {
		SCPreserveState(sc,false);
		for ( layer = ly_fore; layer<sc->layer_cnt; ++layer ) {
		    temp = SSStroke(sc->layers[layer].splines,si,sc);
		    SplinePointListsFree( sc->layers[layer].splines );
		    sc->layers[layer].splines = temp;
		}
		SCCharChangedUpdate(sc,ly_all);
	    } else {
		SCPreserveLayer(sc,layer,false);
		temp = SSStroke(sc->layers[layer].splines,si,sc);
		SplinePointListsFree( sc->layers[layer].splines );
		sc->layers[layer].splines = temp;
		SCCharChangedUpdate(sc,layer);
	    }
	    if ( !ff_progress_next())
    break;
	}
    }
    ff_progress_end_indicator();
}
