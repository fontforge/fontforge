/* Copyright (C) 2000-2003 by George Williams */
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

    base.x = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
    base.y = ((ysp->a*t+ysp->b)*t+ysp->c)*t + ysp->d;

    if ( si->factor!=NULL )
	factor = (si->factor)(si->data,spline,t);

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

static void StrokeEnd(SplinePoint *base, StrokeInfo *si, SplinePoint **_plus, SplinePoint **_minus) {
    SplinePoint *plus, *minus, *cur, *mid1, *mid2;
    BasePoint *dir;
    real c,s;
    real angle;
    real sign;
    real factor = base->next==NULL || si->factor==NULL ? 1.0 :
	    (si->factor)(si->data,base->next,0);

    if ( base->next==NULL && base->prev==NULL ) {
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
	}
    } else if ( si->stroke_type == si_caligraphic ) {
	double lineangle;
	int corner, incr;
	if ( base->next==NULL ) {	/* prev spine */
	    lineangle = SplineAngle(base->prev,1);
	    corner = PenCorner(lineangle,si);
	    incr = 1;
	} else {
	    lineangle = SplineAngle(base->next,0);
	    corner = PenCorner(lineangle,si)+4;
	    incr = -1;
	}
	plus = SplinePointCreate(base->me.x+factor*si->xoff[corner],base->me.y+factor*si->yoff[corner]);
	cur = makeline(plus,base->me.x+factor*si->xoff[corner+incr],base->me.y+factor*si->yoff[corner+incr]);
	*_minus = makeline(cur,base->me.x+factor*si->xoff[corner+2*incr],base->me.y+factor*si->yoff[corner+2*incr]);
	plus->pointtype = pt_corner;
	*_plus = plus;
    } else {
	plus = chunkalloc(sizeof(SplinePoint));
	minus = chunkalloc(sizeof(SplinePoint));
	plus->nonextcp = minus->nonextcp = plus->noprevcp = minus->noprevcp = true;
	plus->pointtype = pt_corner; minus->pointtype = pt_corner;
	if ( base->next==NULL ) {	/* the prev spline moves toward base */
	    SplineIsLinearMake(base->prev);
	    angle = SplineExpand(base->prev,1,0,si,&plus->me,&minus->me);
	    sign = 1;
	} else {
	    SplineIsLinearMake(base->next);
	    angle = SplineExpand(base->next,0,0,si,&plus->me,&minus->me)+ PI;
	    sign = -1;
	}
	if ( si->cap!=lc_round ) {
	    plus->nextcp = plus->me;
	    minus->prevcp = minus->me;
	    plus->nonextcp = minus->nonextcp = true;
	}
	if ( si->cap==lc_butt ) {
	    SplineMake3(plus,minus);		/* draw a line between */
	} else if ( si->cap==lc_square ) {
	    mid1 = SplinePointCreate(
		    plus->me.x+ sign*(plus->me.y-base->me.y),
		    plus->me.y- sign*(plus->me.x-base->me.x));
	    mid2 = SplinePointCreate(
		    minus->me.x+ sign*(plus->me.y-base->me.y),
		    minus->me.y- sign*(plus->me.x-base->me.x));
	    mid1->pointtype = pt_corner; mid2->pointtype = pt_corner;
	    SplineMake3(plus,mid1);
	    SplineMake3(mid1,mid2);
	    SplineMake3(mid2,minus);
	} else if ( si->cap==lc_round ) {
	    mid1 = chunkalloc(sizeof(SplinePoint));
	    mid1->me.x = base->me.x+ sign*(plus->me.y-base->me.y);
	    mid1->me.y = base->me.y- sign*(plus->me.x-base->me.x);
	    mid1->pointtype = pt_curve;
	    c = .552*si->radius*factor*cos(angle);
	    s = .552*si->radius*factor*sin(angle);
	    plus->nextcp.x = plus->me.x + c;
	    plus->nextcp.y = plus->me.y + s;
	    plus->nonextcp = false;
	    minus->prevcp.x = minus->me.x +c;
	    minus->prevcp.y = minus->me.y +s;
	    minus->noprevcp = false;
	    mid1->prevcp.x = mid1->me.x - sign*s;
	    mid1->prevcp.y = mid1->me.y + sign*c;
	    mid1->nextcp.x = mid1->me.x + sign*s;
	    mid1->nextcp.y = mid1->me.y - sign*c;
	    SplineMake3(plus,mid1);
	    SplineMake3(mid1,minus);
	}
	if ( base->next!=NULL ) {
	    plus->noprevcp = minus->nonextcp = base->nonextcp;
	    dir = &base->nextcp;
	} else {
	    plus->noprevcp = minus->nonextcp = base->noprevcp;
	    dir = &base->prevcp;
	}
	plus->prevcp.x = dir->x - base->me.x + plus->me.x;
	plus->prevcp.y = dir->y - base->me.y + plus->me.y;
	minus->nextcp.x = dir->x - base->me.x + minus->me.x;
	minus->nextcp.y = dir->y - base->me.y + minus->me.y;
	*_plus = plus;
	*_minus = minus;
    }
}

/* Is this the inner intersection or the outer one (the inner one is on both splines) */
/*  the outer one is beyond both */
static int Intersect_Lines(JointPoint *inter,BasePoint *p1,real sx1, real sy1,
	BasePoint *p2, real sx2, real sy2) {
    real t1,t2;
    real denom;

    denom = (sx1*sy2-sx2*sy1);
    if ( denom==0 ) {
	/* Lines are parallel. Might be coincident, might not */
	inter->tprev = 1.0;
	inter->tnext = 0;
	inter->inter.x = (p1->x+p2->x)/2;
	inter->inter.y = (p1->y+p2->y)/2;
	if ( sy1==0 && p1->y==p2->y )
	    t1 = 0;
	else if ( sy1!=0 ) {
	    t2 = p2->y/sy2;
	    t1 = p1->y/sy1;
	    t1 = RealNear(p2->x-t2*sx2,p1->x-t1*sx1)?0: 1;
	} else
	    t1 = 1;
    } else {
	t2 = (sy1*(p2->x-p1->x)-sx1*(p2->y-p1->y))/denom;
	t1 = (sy2*(p2->x-p1->x)-sx2*(p2->y-p1->y))/denom;
	inter->inter.x = p1->x + t1*sx1;
	inter->inter.y = p1->y + t1*sy1;
	inter->tprev = 1+t1;
	inter->tnext = t2;
    }
return( t1<=0 );	/* if t1 < 0 then the intersection point is actually */
			/*  on both of the spline segments. if it isn't then */
			/*  it will be on the continuation of the spline */
			/*  but beyond its endpoint... */
}

static void CirclePoint(TPoint *tp,BasePoint *center,BasePoint *dir,real radius) {
    BasePoint off;
    off.x = dir->x-center->x;
    off.y = dir->y-center->y;
    radius /= sqrt(off.x*off.x+off.y*off.y);
    off.x *= radius;
    off.y *= radius;
    tp->x = center->x + off.x;
    tp->y = center->y + off.y;
}

static void MakeJoints(JointPoint *ret,StrokeInfo *si,
	BasePoint *_from,BasePoint *_to, BasePoint *center,
	int incr,double pangle, double nangle, BasePoint *base,
	real factor) {
    SplinePoint *from, *to, *mid;
    BasePoint temp;
    TPoint approx[4];
    int cstart, cend, i;

    from = SplinePointCreate(_from->x,_from->y); from->pointtype = pt_corner;
    to = SplinePointCreate(_to->x,_to->y); to->pointtype = pt_corner;
    ret->from = from; ret->to = to;
    if ( si->stroke_type == si_caligraphic ) {
	cstart = PenCorner(pangle,si);
	cend = PenCorner(nangle,si);
	if ( cstart==cend ) {
	    /* same as a miter join */
	    mid = SplinePointCreate(ret->inter.x,ret->inter.y);
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
		mid = makeline(mid,base->x+factor*si->xoff[i],base->y+factor*si->yoff[i]);
		i += incr;
	    }
	    SplineMake3(mid,to);
	}
    } else if ( si->join==lj_bevel ) {
	SplineMake3(from,to);
    } else if ( si->join == lj_miter ) {
	mid = SplinePointCreate(ret->inter.x,ret->inter.y);
	mid->pointtype = pt_corner;
	SplineMake3(from,mid);
	SplineMake3(mid,to);
    } else {
	from->pointtype = to->pointtype = pt_curve;
	from->nextcp.x = from->me.x - si->radius*cos(nangle);
	from->nextcp.y = from->me.y - si->radius*sin(nangle);
	to->prevcp.x = to->me.x + si->radius*cos(pangle);
	to->prevcp.y = to->me.y + si->radius*sin(pangle);
	CirclePoint(&approx[0],center,&ret->inter,factor*si->radius); approx[0].t = .5;
	temp.x = (ret->inter.x+from->me.x)/2; temp.y = (ret->inter.y+from->me.y)/2;
	CirclePoint(&approx[1],center,&temp,factor*si->radius); approx[1].t = .25;
	temp.x = (ret->inter.x+to->me.x)/2; temp.y = (ret->inter.y+to->me.y)/2;
	CirclePoint(&approx[2],center,&temp,factor*si->radius); approx[2].t = .75;
	ApproximateSplineFromPointsSlopes(from,to,approx,3,false);
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

static void SPFigurePrevCP(SplinePoint *sp,Spline *s,double t) {
    /* All I really care about is the slope, not the length */

    if ( t==1 ) {
	sp->prevcp.x = s->to->prevcp.x - s->to->me.x + sp->me.x;
	sp->prevcp.y = s->to->prevcp.y - s->to->me.y + sp->me.y;
	sp->noprevcp = s->to->noprevcp;
    } else {
	sp->prevcp.x = sp->me.x - ((3*s->splines[0].a*t+2*s->splines[0].b)*t+s->splines[0].c)/3;
	sp->prevcp.y = sp->me.y - ((3*s->splines[1].a*t+2*s->splines[1].b)*t+s->splines[1].c)/3;
	sp->noprevcp = false;
    }
}

static void SPFigureNextCP(SplinePoint *sp,Spline *s,double t) {
    /* All I really care about is the slope, not the length */

    if ( t==0 ) {
	sp->nextcp.x = s->from->nextcp.x - s->from->me.x + sp->me.x;
	sp->nextcp.y = s->from->nextcp.y - s->from->me.y + sp->me.y;
	sp->nonextcp = s->from->nonextcp;
    } else {
	sp->nextcp.x = sp->me.x + ((3*s->splines[0].a*t+2*s->splines[0].b)*t+s->splines[0].c)/3;
	sp->nextcp.y = sp->me.y + ((3*s->splines[1].a*t+2*s->splines[1].b)*t+s->splines[1].c)/3;
	sp->nonextcp = false;
    }
}

/* Plus joins run from prev to next, minus joins run from next to prev */
/* This makes plus joins clockwise and minus joins counter */
static void StrokeJoint(SplinePoint *base,StrokeInfo *si,JointPoint *plus,JointPoint *minus) {
    BasePoint nplus, nminus, pplus,pminus, temp;
    double nangle, pangle;
    int pinner;
    real factor = si->factor==NULL ? 1.0 : (si->factor)(si->data,base->next,0);
    double pt, mt, lastpt, lastmt;
    double oldtt, tt, xdiff, ydiff, loopdiff;
    int mfound, pfound;
    int hold;

    SplineIsLinearMake(base->prev);
    SplineIsLinearMake(base->next);

    pangle = SplineExpand(base->prev,1,0,si,&pplus,&pminus);
    nangle = SplineExpand(base->next,0,0,si,&nplus,&nminus);

    plus->tprev = minus->tprev = 1; plus->tnext = minus->tnext = 0;
    if (( base->pointtype==pt_curve && !base->nonextcp && !base->noprevcp ) ||
	    ( base->pointtype==pt_tangent && (!base->nonextcp || !base->noprevcp)) ||
	    RealNearish(pangle,nangle) ) {
	/* If the two splines are tangent at the base, then everything is */
	/*  simple, there is no join, things match up perfectly */
	/* Um. No. If there is a sharp bend or a corner nearby then it may */
	/*  have the same effect as a corner, in extreme cases the entire */
	/*  spline may be eaten up */
	plus->from = plus->to = SplinePointCreate((pplus.x + nplus.x)/2,(pplus.y + nplus.y)/2);
	plus->from->pointtype = pt_curve;
	nplus = pplus = plus->from->me;
	minus->from = minus->to = SplinePointCreate((pminus.x + nminus.x)/2,(pminus.y + nminus.y)/2);
	nminus = pminus = minus->from->me;
	minus->from->pointtype = pt_curve;

	if ( (xdiff = base->me.x-base->prev->from->me.x)<0 ) xdiff = -xdiff;
	if ( (ydiff = base->me.y-base->prev->from->me.y)<0 ) ydiff = -ydiff;
	loopdiff = (xdiff+ydiff==0) ? 2 : 1.0/(xdiff+ydiff);

	mfound = pfound = false;
	lastpt = lastmt = 0;
	if ( si->stroke_type != si_caligraphic ) for ( tt=1; ; ) {
	    OnEdge(&pplus,&pminus,base->next,0,tt,base->prev,
		    si,&pt,&mt,NULL,NULL);
	    oldtt = tt;
	    tt -= loopdiff;
	    if ( !pfound && (pt==-1 || tt<0)) {
		pfound = true;
		plus->tprev = 1-lastpt;
		plus->tnext = 1-oldtt;
		plus->from->me = pplus;
	    }
	    if ( !mfound && (mt==-1 || tt<0)) {
		mfound = true;
		minus->tprev = oldtt;
		minus->tnext = lastmt;
		minus->from->me = pminus;
	    }
	    if ( pfound && mfound )
	break;
	    SplineExpand(base->prev,tt,0,si,&pplus,&pminus);
	    lastpt = pt; lastmt = mt;
	}

	if ( (xdiff = base->next->to->me.x-base->me.x)<0 ) xdiff = -xdiff;
	if ( (ydiff = base->next->to->me.y-base->me.y)<0 ) ydiff = -ydiff;
	loopdiff = (xdiff+ydiff==0) ? 2 : 1.0/(xdiff+ydiff);

	if ( plus->tnext==0 ) pfound = false;
	if ( minus->tnext==0 ) mfound = false;
	lastpt = lastmt = 1;
	if ( si->stroke_type != si_caligraphic ) for ( tt=0; ; ) {
	    OnEdge(&nplus,&nminus,base->prev,1.,tt,base->next,
		    si,NULL,NULL,&pt,&mt);
	    oldtt = tt;
	    tt += loopdiff;
	    if ( !pfound && (pt==-1 || tt>1.0)) {
		pfound = true;
		plus->tnext = 1-lastpt;
		plus->tprev = 1-oldtt;
		plus->from->me = nplus;
	    }
	    if ( !mfound && (mt==-1 || tt>1.0)) {
		mfound = true;
		minus->tnext = oldtt;
		minus->tprev = lastmt;
		minus->from->me = nminus;
	    }
	    if ( pfound && mfound )
	break;
	    SplineExpand(base->next,tt,0,si,&nplus,&nminus);
	    lastpt = pt; lastmt = mt;
	}

	SPFigurePrevCP(plus->from,base->prev,plus->tprev);
	SPFigureNextCP(plus->from,base->next,plus->tnext);
	SPFigurePrevCP(minus->from,base->prev,minus->tprev);
	SPFigureNextCP(minus->from,base->next,minus->tnext);
	temp = plus->from->nextcp; plus->from->nextcp = plus->from->prevcp; plus->from->prevcp = temp;
	hold = plus->from->nonextcp; plus->from->nonextcp = plus->from->noprevcp; plus->from->noprevcp = hold;
	

	if ( plus->tnext!=0 || plus->tprev!=1 ) plus->from->pointtype = pt_corner;
	if ( minus->tnext!=0 || minus->tprev!=1 ) minus->from->pointtype = pt_corner;
    } else {
	pinner = Intersect_Lines(plus,&pplus,
		 3*base->prev->splines[0].a+2*base->prev->splines[0].b+base->prev->splines[0].c,
		 3*base->prev->splines[1].a+2*base->prev->splines[1].b+base->prev->splines[1].c,
		&nplus,
		 base->next->splines[0].c,
		 base->next->splines[1].c);
	Intersect_Lines(minus,&pminus,
		 3*base->prev->splines[0].a+2*base->prev->splines[0].b+base->prev->splines[0].c,
		 3*base->prev->splines[1].a+2*base->prev->splines[1].b+base->prev->splines[1].c,
		&nminus,
		 base->next->splines[0].c,
		 base->next->splines[1].c);
	if ( !pinner ) {
	    SplineSet junk;
	    MakeJoints(plus,si,&pplus,&nplus,&base->me,1,nangle,pangle,&base->me,factor);
	    junk.first = plus->from; junk.last = plus->to;
	    SplineSetReverse(&junk);
	    plus->from = junk.first; plus->to = junk.last;
	    plus->tprev = 1; plus->tnext = 0;
	    minus->from = minus->to = SplinePointCreate(minus->inter.x,minus->inter.y);
	    minus->from->pointtype = pt_corner;
	} else {
	    SplineSet junk;
	    if (( pangle += PI )>PI ) pangle -= 2*PI;
	    if (( nangle += PI )>PI ) nangle -= 2*PI;
	    MakeJoints(minus,si,&nminus,&pminus,&base->me,-1,pangle,nangle,&base->me,factor);
	    junk.first = minus->from; junk.last = minus->to;
	    SplineSetReverse(&junk);
	    minus->from = junk.first; minus->to = junk.last;
	    minus->tprev = 1; minus->tnext = 0;
	    plus->from = plus->to = SplinePointCreate(plus->inter.x,plus->inter.y);
	    plus->from->pointtype = pt_corner;
	}
	SPFigureNextCP(plus->to,base->prev,plus->tprev);
	SPFigurePrevCP(plus->from,base->next,plus->tnext);
	SPFigurePrevCP(minus->from,base->prev,minus->tprev);
	SPFigureNextCP(minus->to,base->next,minus->tnext);
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
}

static SplineSet *SSFixupOverlap(StrokeInfo *si,SplineChar *sc,
	SplineSet *ssplus,SplineSet *ssminus) {
    ssplus->next = ssminus;
    ssplus = SplineSetRemoveOverlap(sc,ssplus,over_remove);
    if ( si->removeinternal || si->removeexternal ) {
	SplineSet *prev, *spl, *next;
	prev = NULL;
	for ( spl=ssplus; spl!=NULL; spl = next ) {
	    int clock = SplinePointListIsClockwise(spl);
	    next = spl->next;
	    if (( !clock && si->removeinternal ) || ( clock && si->removeexternal )) {
		SplinePointListFree(spl);
		if ( prev==NULL )
		    ssplus = next;
		else
		    prev->next = next;
	    } else
		prev = spl;
	}
    }
return( ssplus );
}

#define Approx	10

static SplineSet *_SplineSetStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc) {
    JointPoint first_plus, first_minus, cur_plus, cur_minus;
    SplineSet *ssplus, *ssminus;
    SplinePoint *plus, *minus;		/* plus expects splines added on prev */
    					/* minus expects splines added on next*/
    SplinePoint *pto, *mto;
    SplinePoint *p_to, *m_to, *p_from, *m_from;
    BasePoint p,m,temp;
    int max=Approx;
    TPoint *pmids=galloc(max*sizeof(TPoint)), *mmids=galloc(max*sizeof(TPoint));
    real p_tlast, m_tlast, p_tcur, m_tcur;
    real t_start, t_end;
    int i,j;
    Spline *first, *spline;
    int reversed = false;
    double ts[10];
    int pinners[10];
    int cnt;
    int pcnt, mcnt;
    double pt1,pt2, mt1,mt2;
    double approx, xdiff, ydiff, loopdiff;

    if ( spl->first==spl->last && spl->first->next!=NULL ) {
	/* My routine gets screwed up by counter-clockwise triangles */
	if ( !SplinePointListIsClockwise(spl)) {
	    reversed = true;
	    SplineSetReverse(spl);
	}
	/* It's a loop, we'll return two SplineSets */
	StrokeJoint(spl->first,si,&first_plus,&first_minus);
	plus = first_plus.from;
	minus = first_minus.to;
	p_tlast = first_plus.tnext;
	m_tlast = first_minus.tnext;
    } else if ( spl->first->next==NULL ) {
	/* Only one point in the SplineSet. */
	ssplus = chunkalloc(sizeof(SplineSet));
	StrokeEnd(spl->first,si,&ssplus->first,&ssplus->last);
return( ssplus );
    } else {
	StrokeEnd(spl->first,si,&plus,&minus);
	p_tlast = 0.0; m_tlast = 0;
    }

    first = NULL;
    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	if ( spline->to->next!=NULL ) {
	    if ( spline->to == spl->first ) {
		cur_plus = first_plus;
		cur_minus = first_minus;
	    } else
		StrokeJoint(spline->to,si,&cur_plus,&cur_minus);
	    p_tcur = cur_plus.tprev;
	    m_tcur = cur_minus.tprev;
	    pto = cur_plus.to;
	    mto = cur_minus.from;
	} else {
	    SplineSet junk;
	    StrokeEnd(spline->to,si,&junk.first,&junk.last);
	    SplineSetReverse(&junk);
	    pto = junk.last;
	    mto = junk.first;
	    p_tcur = 1.0; m_tcur = 1.0;
	}
	t_start = (p_tcur>m_tlast)?p_tcur:m_tlast;
	t_end = (p_tlast<m_tcur)?p_tlast:m_tcur;
	if ( p_tcur<=p_tlast || m_tcur<=m_tlast ) {
	    si->gottoobig = true;
	    if ( !si->toobigwarn ) {
		si->toobigwarn = true;
		GWidgetErrorR( _STR_BadStroke, _STR_StrokeWidthTooBig,
			sc==NULL?"<nameless char>": sc->name );
	    }
	}

	if ( si->stroke_type == si_caligraphic ) {
	    if ( spline->knownlinear ||
    /* 0 and 1 are valid values. They happen on circles for example */
		    p_tcur>1 || m_tcur>1 || m_tlast<0 || p_tlast<0 ||
		    m_tcur<=m_tlast || p_tcur<=p_tlast ) {
		/* this isn't really the right thing to do when m_tcur<=m_tlast*/
		/*  but I'm not sure what is */
		pto->nonextcp = plus->noprevcp = true;
		minus->nonextcp = mto->noprevcp = true;
		SplineMake3(pto,plus);
		SplineMake3(minus,mto);
		if ( plus->nonextcp && plus->noprevcp ) plus->pointtype = pt_corner;
		if ( minus->nonextcp && minus->noprevcp ) minus->pointtype = pt_corner;
	    } else {
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
	    cnt = SplineSolveForPen(spline,si,ts,pinners+1,t_start,t_end);
	    p_to = m_to = NULL;
	    for ( j=1; j<cnt; ++j ) {
		for ( i=0; i<Approx; ++i ) {
		    real t = ts[j-1] + (i+1)*(ts[j]-ts[j-1])/(Approx+1);
		    mmids[i].t = (i+1)/(double) (Approx+1); pmids[i].t = 1-mmids[i].t;
		    SplineExpand(spline,t,0,si,&p,&m);
		    pmids[i].x = p.x; pmids[i].y = p.y;
		    mmids[i].x = m.x; mmids[i].y = m.y;
		}
		if ( j==1 ) {
		    p_from = plus; m_from = minus;
		} else if ( pinners[j-1] ) {
		    p_from = p_to;
		    SplineExpand(spline,ts[j-1],(ts[j-1]-ts[j-2])/20.,si,&p,&m);
		    m_from = SplinePointCreate(m.x,m.y);
		    m_from->pointtype = pt_tangent;
		    SplineMake3(m_to,m_from);
		} else {
		    m_from = m_to;
		    SplineExpand(spline,ts[j-1],(ts[j-1]-ts[j-2])/20.,si,&p,&m);
		    p_from = SplinePointCreate(p.x,p.y);
		    p_from->pointtype = pt_tangent;
		    SplineMake3(p_from,p_to);
		}
		if ( j==cnt-1 ) {
		    p_to = pto;
		    m_to = mto;
		} else if ( pinners[j] ) {
		    SplineExpand(spline,ts[j],(ts[j+1]-ts[j-1])/20.,si,&p,&m);
		    SplineExpand(spline,ts[j],-(ts[j+1]-ts[j-1])/20.,si,&temp,&m);
		    p_to = SplinePointCreate((p.x+temp.x)/2,(p.y+temp.y)/2);
		    p_to->pointtype = pt_corner;
		    m_to = SplinePointCreate(m.x,m.y);
		    m_to->pointtype = pt_tangent;
		} else {
		    SplineExpand(spline,ts[j],(ts[j+1]-ts[j-1])/20.,si,&p,&m);
		    SplineExpand(spline,ts[j],-(ts[j+1]-ts[j-1])/20.,si,&p,&temp);
		    p_to = SplinePointCreate(p.x,p.y);
		    p_to->pointtype = pt_tangent;
		    m_to = SplinePointCreate((m.x+temp.x)/2,(m.y+temp.y)/2);
		    m_to->pointtype = pt_corner;
		}
		ApproximateSplineFromPoints(p_to,p_from,pmids,Approx,false);
		ApproximateSplineFromPoints(m_from,m_to,mmids,Approx,false);
		if ( m_from!=minus && m_from->pointtype!=pt_corner )
		    m_from->pointtype = pt_tangent;
	    }
	    }
	} else {
	    if ( (xdiff = spline->to->me.x-spline->from->me.x)<0 ) xdiff = -xdiff;
	    if ( (ydiff = spline->to->me.y-spline->from->me.y)<0 ) ydiff = -ydiff;
	    loopdiff = (xdiff+ydiff==0) ? .1 : 1.0/(4*(xdiff+ydiff)/si->radius);
	    approx = rint(1.0/loopdiff);
	    if ( approx>max ) {
		max = approx+10;
		pmids = grealloc(pmids,max*sizeof(TPoint));
		mmids = grealloc(mmids,max*sizeof(TPoint));
	    }

	    pcnt = mcnt = 0;
	    /* I used just to calculate points in the area where both curves */
	    /*  were defined, and that works fine most of the time, but some */
	    /*  times we turn sharp corners and we don't get a good sampling */
	    /*  of the curve which doesn't turn a corner */
	    if ( spline->knownlinear || p_tcur>1 || p_tlast<0 || p_tcur<=p_tlast ) {
		pto->nonextcp = plus->noprevcp = true;
		SplineMake3(pto,plus);
		if ( plus->nonextcp && plus->noprevcp ) plus->pointtype = pt_corner;
	    } else {
		for ( i=0; i<approx; ++i ) {
		    real t = p_tlast + (i+1)*(p_tcur-p_tlast)/(approx+1);
		    pmids[pcnt].t = 1-(i+1)/(approx+1);
		    SplineExpand(spline,t,0,si,&p,&m);
		    pmids[pcnt].x = p.x; pmids[pcnt].y = p.y;
		    OnEdge(&p,&m,spline,t,t,spline,si,&pt1,&mt1,&pt2,&mt2);
		    /* If one of these guys isn't -1 then we should stop here */
		    /*  create a corner point, approximate a spline up to it */
		    /*  ignore points until we're not OnEdge, and then start */
		    /*  accumulating points again. But I'm not sure about figuring*/
		    /*  the control points on the corner point yet */
		    if ( pt1==-1 && pt2==-1 ) ++pcnt;
		}
		ApproximateSplineFromPointsSlopes(pto,plus,pmids,pcnt,false);
	    }

	    if ( spline->knownlinear || m_tcur>1 || m_tlast<0 || m_tcur<=m_tlast ) {
		minus->nonextcp = mto->noprevcp = true;
		SplineMake3(minus,mto);
		if ( minus->nonextcp && minus->noprevcp ) minus->pointtype = pt_corner;
	    } else {
		for ( i=0; i<approx; ++i ) {
		    real t = m_tlast + (i+1)*(m_tcur-m_tlast)/(approx+1);
		    mmids[mcnt].t = (i+1)*(m_tcur-m_tlast)/(approx+1);
		    SplineExpand(spline,t,0,si,&p,&m);
		    mmids[mcnt].x = m.x; mmids[mcnt].y = m.y;
		    OnEdge(&p,&m,spline,t,t,spline,si,&pt1,&mt1,&pt2,&mt2);
		    if ( mt1==-1 && mt2==-1 ) ++mcnt;
		}
		ApproximateSplineFromPointsSlopes(minus,mto,mmids,mcnt,false);
	    }
	}
	if ( spline->to->next!=NULL ) {
	    plus = cur_plus.from;
	    minus = cur_minus.to;
	    p_tlast = cur_plus.tnext;
	    m_tlast = cur_minus.tnext;
	} else {
	    /* Done */
    break;
	}
	if ( first==NULL ) first = spline;
    }
    free(mmids); free(pmids);

    ssplus = chunkalloc(sizeof(SplineSet));
    ssplus->first = ssplus->last = plus;
    if ( spl->first==spl->last ) {
	ssminus = chunkalloc(sizeof(SplineSet));
	ssminus->first = ssminus->last = minus;
	/*SplineSetFixRidiculous(ssplus); SplineSetFixRidiculous(ssminus);*/
	SplineSetFixCPs(ssplus); SplineSetFixCPs(ssminus);
	if ( reversed ) {
	    SplineSet *temp = ssplus;
	    ssplus = ssminus;
	    ssminus = temp;
	}
	if ( si->removeoverlapifneeded && si->gottoobig )
	    ssplus = SSFixupOverlap(si,sc,ssplus,ssminus);
	else if ( si->removeinternal ) {
	    SplinePointListFree(ssminus);
	    SplineSetReverse(ssplus);
	} else if ( si->removeexternal ) {
	    SplinePointListFree(ssplus);
	    ssplus = ssminus;
	} else {
	    SplineSetReverse(ssplus);
	    SplineSetReverse(ssminus);
	    ssplus->next = ssminus;
	    /* I used to do a splineset correct dir here on both, but */
	    /*  that doesn't work always if a contour self intersects */
	    /* I think it should always be correct */
#if 0
	    /* probably best to leave it clockwise even if it started counter */
	    if ( reversed ) {
		SplineSetReverse(ssplus);
		SplineSetReverse(ssminus);
	    }
#endif
	}
	if ( reversed )		/* restore original, just in case we want it */
	    SplineSetReverse(spl);
    } else {
	/*SplineSetFixRidiculous(ssplus);*/
	SplineSetFixCPs(ssplus);
	if ( !SplinePointListIsClockwise(ssplus))
	    SplineSetReverse(ssplus);
    }
return( ssplus );
}

static SplineSet *SSRemoveUTurns(SplineSet *base) {
    /* My stroking algorithem gets confused by sharp turns. For example */
    /*  if we have a spline which is all in a line, but the control points */
    /*  are such that it doubles back on itself ( "* +   * +", ie. cps */
    /*  outside of the points) then things get very unhappy */
    SplineSet *spl;
    Spline *first, *s;
    double dx,dy, offx,offy, diff;
    int linear;

    /*for ( spl=base; spl!=NULL; spl=spl->next )*/ spl = base; {
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
		    if ( s->from->pointtype == pt_curve )
			s->from->pointtype = pt_corner;
		    if ( s->order2 ) {
			s->to->prevcp = s->to->me;
			s->to->noprevcp = true;
			if ( s->to->pointtype==pt_curve )
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
		    if ( s->to->pointtype==pt_curve )
			s->to->pointtype = pt_corner;
		    if ( s->order2 ) {
			s->from->nextcp = s->from->me;
			s->from->nonextcp = true;
			if ( s->from->pointtype == pt_curve )
			    s->from->pointtype = pt_corner;
		    }
		    SplineRefigure(s);
		}
	    }
	}
    }
return( base );
}

SplineSet *SplineSetStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc) {
    SplineSet *ret, *temp;

    if ( si->stroke_type == si_elipse ) {
	real trans[6], factor;
	StrokeInfo si2;
	trans[0] = trans[3] = si->c;
	trans[1] = -si->s;
	trans[2] = si->s;
	trans[4] = trans[5] = 0;
	factor = si->radius/si->minorradius;
	trans[0] *= factor; trans[1] *= factor;
	temp = SplinePointListTransform(SplinePointListCopy(spl),trans,true);
	si2 = *si;
	si2.stroke_type = si_std;
	ret = SplineSetStroke(temp,&si2,sc);
	SplinePointListFree(temp);
	trans[0] = trans[3] = si->c;
	trans[1] = si->s;
	trans[2] = -si->s;
	trans[4] = trans[5] = 0;
	factor = si->minorradius/si->radius;
	trans[0] *= factor; trans[2] *= factor;
	ret = SplinePointListTransform(ret,trans,true);
    } else
	ret = _SplineSetStroke(SSRemoveUTurns(spl),si,sc);
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
#if 0
	if ( si->removeinternal || si->removeexternal )
	    was_clock = SplinePointListIsClockwise(spl);
#endif
	cur = SplineSetStroke(spl,si,sc);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
#if 0
	if ( was_clock==0 ) 	/* there'd better be only one spl in cur */
	    SplineSetReverse(cur);
#endif
	while ( cur->next!=NULL ) cur = cur->next;
	last = cur;
    }
return( head );
}
