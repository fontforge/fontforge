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
    if ( !si->caligraphic ) {
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
	if ( si->caligraphic ) {
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
    } else if ( si->caligraphic ) {
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
	    mid1 = chunkalloc(sizeof(SplinePoint));
	    mid1->me.x = plus->me.x+ sign*(plus->me.y-base->me.y);
	    mid1->me.y = plus->me.y- sign*(plus->me.x-base->me.x);
	    mid1->nextcp = mid1->prevcp = mid1->me;
	    mid1->nonextcp = mid1->noprevcp = true;
	    mid2 = chunkalloc(sizeof(SplinePoint));
	    mid2->me.x = minus->me.x+ sign*(plus->me.y-base->me.y);
	    mid2->me.y = minus->me.y- sign*(plus->me.x-base->me.x);
	    mid2->nextcp = mid2->prevcp = mid2->me;
	    mid2->nonextcp = mid2->noprevcp = true;
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
	    minus->prevcp.x = minus->me.x +c;
	    minus->prevcp.y = minus->me.y +s;
	    mid1->prevcp.x = mid1->me.x - sign*s;
	    mid1->prevcp.y = mid1->me.y + sign*c;
	    mid1->nextcp.x = mid1->me.x + sign*s;
	    mid1->nextcp.y = mid1->me.y - sign*c;
	    SplineMake3(plus,mid1);
	    SplineMake3(mid1,minus);
	}
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
    if ( si->caligraphic ) {
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
	CirclePoint(&approx[0],center,&ret->inter,factor*si->radius); approx[0].t = .5;
	temp.x = (ret->inter.x+from->me.x)/2; temp.y = (ret->inter.y+from->me.y)/2;
	CirclePoint(&approx[1],center,&temp,factor*si->radius); approx[1].t = .25;
	temp.x = (ret->inter.x+to->me.x)/2; temp.y = (ret->inter.y+to->me.y)/2;
	CirclePoint(&approx[2],center,&temp,factor*si->radius); approx[2].t = .75;
	ApproximateSplineFromPoints(from,to,approx,3);
    }
}

/* Plus joins run from prev to next, minus joins run from next to prev */
/* This makes plus joins clockwise and minus joins counter */
static void StrokeJoint(SplinePoint *base,StrokeInfo *si,JointPoint *plus,JointPoint *minus) {
    BasePoint nplus, nminus, pplus,pminus;
    double nangle, pangle;
    int pinner;
    real factor = si->factor==NULL ? 1.0 : (si->factor)(si->data,base->next,0);

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
	plus->from = plus->to = chunkalloc(sizeof(SplinePoint));
	plus->from->pointtype = pt_curve;
	plus->from->me.x = (pplus.x + nplus.x)/2;
	plus->from->me.y = (pplus.y + nplus.y)/2;
	minus->from = minus->to = chunkalloc(sizeof(SplinePoint));
	minus->from->me.x = (pminus.x + nminus.x)/2;
	minus->from->me.y = (pminus.y + nminus.y)/2;
	minus->from->pointtype = pt_curve;
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
	    MakeJoints(plus,si,&pplus,&nplus,&base->me,1,pangle,nangle,&base->me,factor);
	    junk.first = plus->from; junk.last = plus->to;
	    SplineSetReverse(&junk);
	    plus->from = junk.first; plus->to = junk.last;
	    plus->tprev = 1; plus->tnext = 0;
	    minus->from = minus->to = chunkalloc(sizeof(SplinePoint));
	    minus->from->me = minus->inter;
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
	    plus->from = plus->to = chunkalloc(sizeof(SplinePoint));
	    plus->from->me = plus->inter;
	    plus->from->pointtype = pt_corner;
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
    
SplineSet *SplineSetStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc) {
    JointPoint first_plus, first_minus, cur_plus, cur_minus;
    SplineSet *ssplus, *ssminus;
    SplinePoint *plus, *minus;		/* plus expects splines added on prev */
    					/* minus expects splines added on next*/
    SplinePoint *pto, *mto;
    SplinePoint *p_to, *m_to, *p_from, *m_from;
    BasePoint p,m,temp;
    TPoint pmids[4], mmids[4];
    real p_tlast, m_tlast, p_tcur, m_tcur;
    real t_start, t_end;
    int i,j;
    Spline *first, *spline;
    int reversed = false;
    double ts[10];
    int pinners[10];
    int cnt;

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
	p_tlast = first_plus.tprev;
	m_tlast = first_minus.tnext;
    } else if ( spl->first->next==NULL ) {
	/* Only one point in the SplineSet. */
	ssplus = chunkalloc(sizeof(SplineSet));
	StrokeEnd(spl->first,si,&ssplus->first,&ssplus->last);
return( ssplus );
    } else {
	StrokeEnd(spl->first,si,&plus,&minus);
	p_tlast = 1.0; m_tlast = 0;
    }

    first = NULL;
    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	if ( spline->to->next!=NULL ) {
	    if ( spline->to == spl->first ) {
		cur_plus = first_plus;
		cur_minus = first_minus;
	    } else
		StrokeJoint(spline->to,si,&cur_plus,&cur_minus);
	    p_tcur = cur_plus.tnext;
	    m_tcur = cur_minus.tprev;
	    pto = cur_plus.to;
	    mto = cur_minus.from;
	} else {
	    SplineSet junk;
	    StrokeEnd(spline->to,si,&junk.first,&junk.last);
	    SplineSetReverse(&junk);
	    pto = junk.last;
	    mto = junk.first;
	    p_tcur = 0.0; m_tcur = 1.0;
	}
	t_start = (p_tcur>m_tlast)?p_tcur:m_tlast;
	t_end = (p_tlast<m_tcur)?p_tlast:m_tcur;
	if ( p_tcur>=p_tlast || m_tcur<=m_tlast ) {
	    si->gottoobig = true;
	    if ( !si->toobigwarn ) {
		si->toobigwarn = true;
		GWidgetErrorR( _STR_BadStroke, _STR_StrokeWidthTooBig,
			sc==NULL?"<nameless char>": sc->name );
	    }
	}
	if ( spline->knownlinear ||
/* 0 and 1 are valid values. They happen on circles for example */
		p_tcur<0 || m_tcur>1 || m_tlast<0 || p_tlast>1 ||
		m_tcur<=m_tlast || p_tcur>=p_tlast ) {
	    pto->nonextcp = plus->noprevcp = true;
	    minus->nonextcp = mto->noprevcp = true;
	    SplineMake3(pto,plus);
	    SplineMake3(minus,mto);
	    if ( plus->nonextcp && plus->noprevcp ) plus->pointtype = pt_corner;
	    if ( minus->nonextcp && minus->noprevcp ) minus->pointtype = pt_corner;
	} else if ( si->caligraphic ) {
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
	    cnt = SplineSolveForPen(spline,si,ts,pinners,t_start,t_end);
	    p_to = m_to = NULL;
	    for ( j=1; j<cnt; ++j ) {
		for ( i=0; i<4; ++i ) {
		    real t = ts[j-1] + (i+1)*(ts[j]-ts[j-1])/5;
		    mmids[i].t = (i+1)/5.; pmids[i].t = 1-mmids[i].t;
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
		ApproximateSplineFromPoints(p_to,p_from,pmids,4);
		ApproximateSplineFromPoints(m_from,m_to,mmids,4);
		if ( m_from!=minus && m_from->pointtype!=pt_corner )
		    m_from->pointtype = pt_tangent;
	    }
	} else {
	    for ( i=0; i<4; ++i ) {
		real t = t_start + (i+1)*(t_end-t_start)/5;
		pmids[i].t = (t-p_tlast)/(p_tcur-p_tlast);
		mmids[i].t = (t-m_tlast)/(m_tcur-m_tlast);
		SplineExpand(spline,t,0,si,&p,&m);
		pmids[i].x = p.x; pmids[i].y = p.y;
		mmids[i].x = m.x; mmids[i].y = m.y;
	    }
	    ApproximateSplineFromPoints(pto,plus,pmids,4);
	    ApproximateSplineFromPoints(minus,mto,mmids,4);
	}
	if ( spline->to->next!=NULL ) {
	    plus = cur_plus.from;
	    minus = cur_minus.to;
	    p_tlast = cur_plus.tprev;
	    m_tlast = cur_minus.tnext;
	} else {
	    /* Done */
    break;
	}
	if ( first==NULL ) first = spline;
    }

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

    /* for angles between [penangle,penangle+90] use (-r,t/2) rotated by penangle */
    /*			  [penangle+90,penangle+180] use (-r,-t/2) */
    /*			  [penangle+180,penangle+270] use (r,-t/2) */
    /*			  [penangle+270,penangle] use (r,t/2) */

SplineSet *SSStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc) {
    SplineSet *head=NULL, *last=NULL, *cur;
    int was_clock = true;

    for ( ; spl!=NULL; spl = spl->next ) {
	if ( si->removeinternal || si->removeexternal )
	    was_clock = SplinePointListIsClockwise(spl);
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
