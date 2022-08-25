/* -*- coding: utf-8 -*- */
/* Copyright (C) 2000-2012 by George Williams, 2021 by Linus Romer */
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

#include "fontforge.h"
#include "splinefit.h"
#include "splineorder2.h"
#include "splineutil.h"
#include "splineutil2.h"
#include "utanvec.h"

#include <math.h>

static Spline *IsLinearApprox(SplinePoint *from, SplinePoint *to,
	FitPoint *mid, int cnt, int order2) {
    bigreal vx, vy, slope;
    int i;

    vx = to->me.x-from->me.x; vy = to->me.y-from->me.y;
    if ( vx==0 && vy==0 ) {
	for ( i=0; i<cnt; ++i )
	    if ( mid[i].p.x != from->me.x || mid[i].p.y != from->me.y )
return( NULL );
    } else if ( fabs(vx)>fabs(vy) ) {
	slope = vy/vx;
	for ( i=0; i<cnt; ++i )
	    if ( !RealWithin(mid[i].p.y,from->me.y+slope*(mid[i].p.x-from->me.x),.7) )
return( NULL );
    } else {
	slope = vx/vy;
	for ( i=0; i<cnt; ++i )
	    if ( !RealWithin(mid[i].p.x,from->me.x+slope*(mid[i].p.y-from->me.y),.7) )
return( NULL );
    }
    from->nextcp = from->me;
    to->prevcp = to->me;
return( SplineMake(from,to,order2) );
}

/* This routine should almost never be called now. It uses a flawed algorithm */
/*  which won't produce the best results. It gets called only when the better */
/*  approach doesn't work (singular matrices, etc.) */
/* Old comment, back when I was confused... */
/* Least squares tells us that:
	| S(xi*ti^3) |	 | S(ti^6) S(ti^5) S(ti^4) S(ti^3) |   | a |
	| S(xi*ti^2) | = | S(ti^5) S(ti^4) S(ti^3) S(ti^2) | * | b |
	| S(xi*ti)   |	 | S(ti^4) S(ti^3) S(ti^2) S(ti)   |   | c |
	| S(xi)	     |   | S(ti^3) S(ti^2) S(ti)   n       |   | d |
 and the definition of a spline tells us:
	| x1         | = |   1        1       1       1    | * (a b c d)
	| x0         | = |   0        0       0       1    | * (a b c d)
So we're a bit over specified. Let's use the last two lines of least squares
and the 2 from the spline defn. So d==x0. Now we've got three unknowns
and only three equations...

For order2 splines we've got
	| S(xi*ti^2) |	 | S(ti^4) S(ti^3) S(ti^2) |   | b |
	| S(xi*ti)   | = | S(ti^3) S(ti^2) S(ti)   | * | c |
	| S(xi)	     |   | S(ti^2) S(ti)   n       |   | d |
 and the definition of a spline tells us:
	| x1         | = |   1       1       1    | * (b c d)
	| x0         | = |   0       0       1    | * (b c d)
=>
    d = x0
    b+c = x1-x0
    S(ti^2)*b + S(ti)*c = S(xi)-n*x0
    S(ti^2)*b + S(ti)*(x1-x0-b) = S(xi)-n*x0
    [ S(ti^2)-S(ti) ]*b = S(xi)-S(ti)*(x1-x0) - n*x0
*/
static int _ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
	FitPoint *mid, int cnt, BasePoint *nextcp, BasePoint *prevcp,
	int order2) {
    bigreal tt, ttn;
    int i, j, ret;
    bigreal vx[3], vy[3], m[3][3];
    bigreal ts[7], xts[4], yts[4];
    BasePoint nres, pres;
    int nrescnt=0, prescnt=0;
    bigreal nmin, nmax, pmin, pmax, test, ptest;
    bigreal bx, by, cx, cy;
	
    memset(&nres,0,sizeof(nres)); memset(&pres,0,sizeof(pres));

    /* Add the initial and end points */
    ts[0] = 2; for ( i=1; i<7; ++i ) ts[i] = 1;
    xts[0] = from->me.x+to->me.x; yts[0] = from->me.y+to->me.y;
    xts[3] = xts[2] = xts[1] = to->me.x; yts[3] = yts[2] = yts[1] = to->me.y;
    nmin = pmin = 0; nmax = pmax = (to->me.x-from->me.x)*(to->me.x-from->me.x)+(to->me.y-from->me.y)*(to->me.y-from->me.y);
    for ( i=0; i<cnt; ++i ) {
	xts[0] += mid[i].p.x;
	yts[0] += mid[i].p.y;
	++ts[0];
	tt = mid[i].t;
	xts[1] += tt*mid[i].p.x;
	yts[1] += tt*mid[i].p.y;
	ts[1] += tt;
	ts[2] += (ttn=tt*tt);
	xts[2] += ttn*mid[i].p.x;
	yts[2] += ttn*mid[i].p.y;
	ts[3] += (ttn*=tt);
	xts[3] += ttn*mid[i].p.x;
	yts[3] += ttn*mid[i].p.y;
	ts[4] += (ttn*=tt);
	ts[5] += (ttn*=tt);
	ts[6] += (ttn*=tt);

	test = (mid[i].p.x-from->me.x)*(to->me.x-from->me.x) + (mid[i].p.y-from->me.y)*(to->me.y-from->me.y);
	if ( test<nmin ) nmin=test;
	if ( test>nmax ) nmax=test;
	test = (mid[i].p.x-to->me.x)*(from->me.x-to->me.x) + (mid[i].p.y-to->me.y)*(from->me.y-to->me.y);
	if ( test<pmin ) pmin=test;
	if ( test>pmax ) pmax=test;
    }
    pmin *= 1.2; pmax *= 1.2; nmin *= 1.2; nmax *= 1.2;

    for ( j=0; j<3; ++j ) {
	if ( order2 ) {
	    if ( RealNear(ts[j+2],ts[j+1]) )
    continue;
	    /* This produces really bad results!!!! But I don't see what I can do to improve it */
	    bx = (xts[j]-ts[j+1]*(to->me.x-from->me.x) - ts[j]*from->me.x) / (ts[j+2]-ts[j+1]);
	    by = (yts[j]-ts[j+1]*(to->me.y-from->me.y) - ts[j]*from->me.y) / (ts[j+2]-ts[j+1]);
	    cx = to->me.x-from->me.x-bx;
	    cy = to->me.y-from->me.y-by;

	    nextcp->x = from->me.x + cx/2;
	    nextcp->y = from->me.y + cy/2;
	    *prevcp = *nextcp;
	} else {
	    vx[0] = xts[j+1]-ts[j+1]*from->me.x;
	    vx[1] = xts[j]-ts[j]*from->me.x;
	    vx[2] = to->me.x-from->me.x;		/* always use the defn of spline */

	    vy[0] = yts[j+1]-ts[j+1]*from->me.y;
	    vy[1] = yts[j]-ts[j]*from->me.y;
	    vy[2] = to->me.y-from->me.y;

	    m[0][0] = ts[j+4]; m[0][1] = ts[j+3]; m[0][2] = ts[j+2];
	    m[1][0] = ts[j+3]; m[1][1] = ts[j+2]; m[1][2] = ts[j+1];
	    m[2][0] = 1;  m[2][1] = 1;  m[2][2] = 1;

	    /* Remove a terms from rows 0 and 1 */
	    vx[0] -= ts[j+4]*vx[2];
	    vy[0] -= ts[j+4]*vy[2];
	    m[0][0] = 0; m[0][1] -= ts[j+4]; m[0][2] -= ts[j+4];
	    vx[1] -= ts[j+3]*vx[2];
	    vy[1] -= ts[j+3]*vy[2];
	    m[1][0] = 0; m[1][1] -= ts[j+3]; m[1][2] -= ts[j+3];

	    if ( fabs(m[1][1])<fabs(m[0][1]) ) {
		bigreal temp;
		temp = vx[1]; vx[1] = vx[0]; vx[0] = temp;
		temp = vy[1]; vy[1] = vy[0]; vy[0] = temp;
		temp = m[1][1]; m[1][1] = m[0][1]; m[0][1] = temp;
		temp = m[1][2]; m[1][2] = m[0][2]; m[0][2] = temp;
	    }
	    /* remove b terms from rows 0 and 2 (first normalize row 1 so m[1][1] is 1*/
	    vx[1] /= m[1][1];
	    vy[1] /= m[1][1];
	    m[1][2] /= m[1][1];
	    m[1][1] = 1;
	    vx[0] -= m[0][1]*vx[1];
	    vy[0] -= m[0][1]*vy[1];
	    m[0][2] -= m[0][1]*m[1][2]; m[0][1] = 0;
	    vx[2] -= m[2][1]*vx[1];
	    vy[2] -= m[2][1]*vy[1];
	    m[2][2] -= m[2][1]*m[1][2]; m[2][1] = 0;

	    vx[0] /= m[0][2];			/* This is cx */
	    vy[0] /= m[0][2];			/* This is cy */
	    /*m[0][2] = 1;*/

	    vx[1] -= m[1][2]*vx[0];		/* This is bx */
	    vy[1] -= m[1][2]*vy[0];		/* This is by */
	    /* m[1][2] = 0; */
	    vx[2] -= m[2][2]*vx[0];		/* This is ax */
	    vy[2] -= m[2][2]*vy[0];		/* This is ay */
	    /* m[2][2] = 0; */

	    nextcp->x = from->me.x + vx[0]/3;
	    nextcp->y = from->me.y + vy[0]/3;
	    prevcp->x = nextcp->x + (vx[0]+vx[1])/3;
	    prevcp->y = nextcp->y + (vy[0]+vy[1])/3;
	}

	test = (nextcp->x-from->me.x)*(to->me.x-from->me.x) +
		(nextcp->y-from->me.y)*(to->me.y-from->me.y);
	ptest = (prevcp->x-to->me.x)*(from->me.x-to->me.x) +
		(prevcp->y-to->me.y)*(from->me.y-to->me.y);
	if ( order2 &&
		(test<nmin || test>nmax || ptest<pmin || ptest>pmax))
    continue;
	if ( test>=nmin && test<=nmax ) {
	    nres.x += nextcp->x; nres.y += nextcp->y;
	    ++nrescnt;
	}
	if ( test>=pmin && test<=pmax ) {
	    pres.x += prevcp->x; pres.y += prevcp->y;
	    ++prescnt;
	}
	if ( nrescnt==1 && prescnt==1 )
    break;
    }

    ret = 0;
    if ( nrescnt>0 ) {
	ret |= 1;
	nextcp->x = nres.x/nrescnt;
	nextcp->y = nres.y/nrescnt;
    } else
	*nextcp = from->nextcp;
    if ( prescnt>0 ) {
	ret |= 2;
	prevcp->x = pres.x/prescnt;
	prevcp->y = pres.y/prescnt;
    } else
	*prevcp = to->prevcp;
    if ( order2 && ret!=3 ) {
	nextcp->x = (nextcp->x + prevcp->x)/2;
	nextcp->y = (nextcp->y + prevcp->y)/2;
    }
    if ( order2 )
	*prevcp = *nextcp;
return( ret );
}

static void TestForLinear(SplinePoint *from,SplinePoint *to) {
    BasePoint off, cpoff, cpoff2;
    bigreal len, co, co2;

    /* Did we make a line? */
    off.x = to->me.x-from->me.x; off.y = to->me.y-from->me.y;
    len = sqrt(off.x*off.x + off.y*off.y);
    if ( len!=0 ) {
	off.x /= len; off.y /= len;
	cpoff.x = from->nextcp.x-from->me.x; cpoff.y = from->nextcp.y-from->me.y;
	len = sqrt(cpoff.x*cpoff.x + cpoff.y*cpoff.y);
	if ( len!=0 ) {
	    cpoff.x /= len; cpoff.y /= len;
	}
	cpoff2.x = to->prevcp.x-from->me.x; cpoff2.y = to->prevcp.y-from->me.y;
	len = sqrt(cpoff2.x*cpoff2.x + cpoff2.y*cpoff2.y);
	if ( len!=0 ) {
	    cpoff2.x /= len; cpoff2.y /= len;
	}
	co = cpoff.x*off.y - cpoff.y*off.x; co2 = cpoff2.x*off.y - cpoff2.y*off.x;
	if ( co<.05 && co>-.05 && co2<.05 && co2>-.05 ) {
	    from->nextcp = from->me;
	    to->prevcp = to->me;
	} else {
	    Spline temp;
	    memset(&temp,0,sizeof(temp));
	    temp.from = from; temp.to = to;
	    SplineRefigure(&temp);
	    if ( SplineIsLinear(&temp)) {
		from->nextcp = from->me;
		to->prevcp = to->me;
	    }
	}
    }
}

/* Find a spline which best approximates the list of intermediate points we */
/*  are given. No attempt is made to use fixed slope angles */
/* given a set of points (x,y,t) */
/* find the bezier spline which best fits those points */

/* OK, we know the end points, so all we really need are the control points */
  /*    For cubics.... */
/* Pf = point from */
/* CPf = control point, from nextcp */
/* CPt = control point, to prevcp */
/* Pt = point to */
/* S(t) = Pf + 3*(CPf-Pf)*t + 3*(CPt-2*CPf+Pf)*t^2 + (Pt-3*CPt+3*CPf-Pf)*t^3 */
/* S(t) = Pf - 3*Pf*t + 3*Pf*t^2 - Pf*t^3 + Pt*t^3 +                         */
/*           3*(t-2*t^2+t^3)*CPf +                                           */
/*           3*(t^2-t^3)*CPt                                                 */
/* We want to minimize Σ [S(ti)-Pi]^2 */
/* There are four variables CPf.x, CPf.y, CPt.x, CPt.y */
/* When we take the derivative of the error term above with each of these */
/*  variables, we find that the two coordinates are separate. So I shall only */
/*  work through the equations once, leaving off the coordinate */
/* d error/dCPf = Σ 2*3*(t-2*t^2+t^3) * [S(ti)-Pi] = 0 */
/* d error/dCPt = Σ 2*3*(t^2-t^3)     * [S(ti)-Pi] = 0 */
  /*    For quadratics.... */
/* CP = control point, there's only one */
/* S(t) = Pf + 2*(CP-Pf)*t + (Pt-2*CP+Pf)*t^2 */
/* S(t) = Pf - 2*Pf*t + Pf*t^2 + Pt*t^2 +     */
/*           2*(t-2*t^2)*CP                   */
/* We want to minimize Σ [S(ti)-Pi]^2 */
/* There are two variables CP.x, CP.y */
/* d error/dCP = Σ 2*2*(t-2*t^2) * [S(ti)-Pi] = 0 */
/* Σ (t-2*t^2) * [Pf - 2*Pf*t + Pf*t^2 + Pt*t^2 - Pi +     */
/*           2*(t-2*t^2)*CP] = 0               */
/* CP * (Σ 2*(t-2*t^2)*(t-2*t^2)) = Σ (t-2*t^2) * [Pf - 2*Pf*t + Pf*t^2 + Pt*t^2 - Pi] */

/*        Σ (t-2*t^2) * [Pf - 2*Pf*t + Pf*t^2 + Pt*t^2 - Pi] */
/* CP = ----------------------------------------------------- */
/*                    Σ 2*(t-2*t^2)*(t-2*t^2)                */
Spline *ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
	FitPoint *mid, int cnt, int order2) {
    int ret;
    Spline *spline;
    BasePoint nextcp, prevcp;
    int i;

    if ( order2 ) {
	bigreal xconst, yconst, term /* Same for x and y */;
	xconst = yconst = term = 0;
	for ( i=0; i<cnt; ++i ) {
	    bigreal t = mid[i].t, t2 = t*t;
	    bigreal tfactor = (t-2*t2);
	    term += 2*tfactor*tfactor;
	    xconst += tfactor*(from->me.x*(1-2*t+t2) + to->me.x*t2 - mid[i].p.x);
	    yconst += tfactor*(from->me.y*(1-2*t+t2) + to->me.y*t2 - mid[i].p.y);
	}
	if ( term!=0 ) {
	    BasePoint cp;
	    cp.x = xconst/term; cp.y = yconst/term;
	    from->nextcp = to->prevcp = cp;
return( SplineMake2(from,to));
	}
    } else {
	bigreal xconst[2], yconst[2], f_term[2], t_term[2] /* Same for x and y */;
	bigreal tfactor[2], determinant;
	xconst[0] = xconst[1] = yconst[0] = yconst[1] =
	    f_term[0] = f_term[1] = t_term[0] = t_term[1] =  0;
	for ( i=0; i<cnt; ++i ) {
	    bigreal t = mid[i].t, t2 = t*t, t3=t*t2;
	    bigreal xc = (from->me.x*(1-3*t+3*t2-t3) + to->me.x*t3 - mid[i].p.x);
	    bigreal yc = (from->me.y*(1-3*t+3*t2-t3) + to->me.y*t3 - mid[i].p.y);
	    tfactor[0] = (t-2*t2+t3); tfactor[1]=(t2-t3);
	    xconst[0] += tfactor[0]*xc;
	    xconst[1] += tfactor[1]*xc;
	    yconst[0] += tfactor[0]*yc;
	    yconst[1] += tfactor[1]*yc;
	    f_term[0] += 3*tfactor[0]*tfactor[0];
	    f_term[1] += 3*tfactor[0]*tfactor[1];
	    /*t_term[0] += 3*tfactor[1]*tfactor[0];*/
	    t_term[1] += 3*tfactor[1]*tfactor[1];
	}
	t_term[0] = f_term[1];
	determinant = f_term[1]*t_term[0] - f_term[0]*t_term[1];
	if ( determinant!=0 ) {
	    to->prevcp.x = -(xconst[0]*f_term[1]-xconst[1]*f_term[0])/determinant;
	    to->prevcp.y = -(yconst[0]*f_term[1]-yconst[1]*f_term[0])/determinant;
	    if ( f_term[0]!=0 ) {
		from->nextcp.x = (-xconst[0]-t_term[0]*to->prevcp.x)/f_term[0];
		from->nextcp.y = (-yconst[0]-t_term[0]*to->prevcp.y)/f_term[0];
	    } else {
		from->nextcp.x = (-xconst[1]-t_term[1]*to->prevcp.x)/f_term[1];
		from->nextcp.y = (-yconst[1]-t_term[1]*to->prevcp.y)/f_term[1];
	    }
return( SplineMake3(from,to));
	}
    }

    if ( (spline = IsLinearApprox(from,to,mid,cnt,order2))!=NULL )
return( spline );

    ret = _ApproximateSplineFromPoints(from,to,mid,cnt,&nextcp,&prevcp,order2);

    if ( ret&1 ) {
	from->nextcp = nextcp;
    } else {
	from->nextcp = from->me;
    }
    if ( ret&2 ) {
	to->prevcp = prevcp;
    } else {
	to->prevcp = to->me;
    }
    TestForLinear(from,to);
    spline = SplineMake(from,to,order2);
return( spline );
}

static bigreal ClosestSplineSolve(Spline1D *sp,bigreal sought,bigreal close_to_t) {
    /* We want to find t so that spline(t) = sought */
    /*  find the value which is closest to close_to_t */
    /* on error return closetot */
    extended ts[3];
    int i;
    bigreal t, best, test;

    _CubicSolve(sp,sought,ts);
    best = 9e20; t= close_to_t;
    for ( i=0; i<3; ++i ) if ( ts[i]>-.0001 && ts[i]<1.0001 ) {
	if ( (test=ts[i]-close_to_t)<0 ) test = -test;
	if ( test<best ) {
	    best = test;
	    t = ts[i];
	}
    }

return( t );
}

struct dotbounds {
    BasePoint unit;
    BasePoint base;
    bigreal len;
    bigreal min,max;		/* If min<0 || max>len the spline extends beyond its endpoints */
};

static bigreal SigmaDeltas(Spline *spline, FitPoint *mid, int cnt, DBounds *b, struct dotbounds *db) {
    int i;
    bigreal xdiff, ydiff, sum, temp, t;
    SplinePoint *to = spline->to, *from = spline->from;
    extended ts[2], x,y;
    struct dotbounds db2;
    bigreal dot;
    int near_vert, near_horiz;

    if ( (xdiff = to->me.x-from->me.x)<0 ) xdiff = -xdiff;
    if ( (ydiff = to->me.y-from->me.y)<0 ) ydiff = -ydiff;
    near_vert = ydiff>2*xdiff;
    near_horiz = xdiff>2*ydiff;

    sum = 0;
    for ( i=0; i<cnt; ++i ) {
	if ( near_vert ) {
	    t = ClosestSplineSolve(&spline->splines[1],mid[i].p.y,mid[i].t);
	} else if ( near_horiz ) {
	    t = ClosestSplineSolve(&spline->splines[0],mid[i].p.x,mid[i].t);
	} else {
	    t = (ClosestSplineSolve(&spline->splines[1],mid[i].p.y,mid[i].t) +
		    ClosestSplineSolve(&spline->splines[0],mid[i].p.x,mid[i].t))/2;
	}
	temp = mid[i].p.x - ( ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d );
	sum += temp*temp;
	temp = mid[i].p.y - ( ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d );
	sum += temp*temp;
    }

    /* Ok, we've got distances from a set of points on the old spline to the */
    /*  new one. Let's do the reverse: find the extrema of the new and see how*/
    /*  close they get to the bounding box of the old */
    /* And get really unhappy if the spline extends beyond the end-points */
    db2.min = 0; db2.max = db->len;
    SplineFindExtrema(&spline->splines[0],&ts[0],&ts[1]);
    for ( i=0; i<2; ++i ) {
	if ( ts[i]!=-1 ) {
	    x = ((spline->splines[0].a*ts[i]+spline->splines[0].b)*ts[i]+spline->splines[0].c)*ts[i] + spline->splines[0].d;
	    y = ((spline->splines[1].a*ts[i]+spline->splines[1].b)*ts[i]+spline->splines[1].c)*ts[i] + spline->splines[1].d;
	    if ( x<b->minx )
		sum += (x-b->minx)*(x-b->minx);
	    else if ( x>b->maxx )
		sum += (x-b->maxx)*(x-b->maxx);
	    dot = (x-db->base.x)*db->unit.x + (y-db->base.y)*db->unit.y;
	    if ( dot<db2.min ) db2.min = dot;
	    if ( dot>db2.max ) db2.max = dot;
	}
    }
    SplineFindExtrema(&spline->splines[1],&ts[0],&ts[1]);
    for ( i=0; i<2; ++i ) {
	if ( ts[i]!=-1 ) {
	    x = ((spline->splines[0].a*ts[i]+spline->splines[0].b)*ts[i]+spline->splines[0].c)*ts[i] + spline->splines[0].d;
	    y = ((spline->splines[1].a*ts[i]+spline->splines[1].b)*ts[i]+spline->splines[1].c)*ts[i] + spline->splines[1].d;
	    if ( y<b->miny )
		sum += (y-b->miny)*(y-b->miny);
	    else if ( y>b->maxy )
		sum += (y-b->maxy)*(y-b->maxy);
	    dot = (x-db->base.x)*db->unit.x + (y-db->base.y)*db->unit.y;
	    if ( dot<db2.min ) db2.min = dot;
	    if ( dot>db2.max ) db2.max = dot;
	}
    }

    /* Big penalty for going beyond the range of the desired spline */
    if ( db->min==0 && db2.min<0 )
	sum += 10000 + db2.min*db2.min;
    else if ( db2.min<db->min )
	sum += 100 + (db2.min-db->min)*(db2.min-db->min);
    if ( db->max==db->len && db2.max>db->len )
	sum += 10000 + (db2.max-db->max)*(db2.max-db->max);
    else if ( db2.max>db->max )
	sum += 100 + (db2.max-db->max)*(db2.max-db->max);

return( sum );
}

static void ApproxBounds(DBounds *b, FitPoint *mid, int cnt, struct dotbounds *db) {
    int i;
    bigreal dot;

    b->minx = b->maxx = mid[0].p.x;
    b->miny = b->maxy = mid[0].p.y;
    db->min = 0; db->max = db->len;
    for ( i=1; i<cnt; ++i ) {
	if ( mid[i].p.x>b->maxx ) b->maxx = mid[i].p.x;
	if ( mid[i].p.x<b->minx ) b->minx = mid[i].p.x;
	if ( mid[i].p.y>b->maxy ) b->maxy = mid[i].p.y;
	if ( mid[i].p.y<b->miny ) b->miny = mid[i].p.y;
	dot = (mid[i].p.x-db->base.x)*db->unit.x + (mid[i].p.y-db->base.y)*db->unit.y;
	if ( dot<db->min ) db->min = dot;
	if ( dot>db->max ) db->max = dot;
    }
}

/* pf == point from (start point) */
/* Δf == slope from (cp(from) - from) */
/* pt == point to (end point, t==1) */
/* Δt == slope to (cp(to) - to) */

/* A spline from pf to pt with slope vectors rf*Δf, rt*Δt is: */
/* p(t) = pf +  [ 3*rf*Δf ]*t  +  3*[pt-pf+rt*Δt-2*rf*Δf] *t^2 +			*/
/*		[2*pf-2*pt+3*rf*Δf-3*rt*Δt]*t^3 */

/* So I want */
/*   d  Σ (p(t(i))-p(i))^2/ d rf  == 0 */
/*   d  Σ (p(t(i))-p(i))^2/ d rt  == 0 */
/* now... */
/*   d  Σ (p(t(i))-p(i))^2/ d rf  == 0 */
/* => Σ 3*t*Δf*(1-2*t+t^2)*
 *			[pf-pi+ 3*(pt-pf)*t^2 + 2*(pf-pt)*t^3]   +
 *			3*[t - 2*t^2 + t^3]*Δf*rf   +
 *			3*[t^2-t^3]*Δt*rt   */
/* and... */
/*   d  Σ (p(t(i))-p(i))^2/ d rt  == 0 */
/* => Σ 3*t^2*Δt*(1-t)*
 *			[pf-pi+ 3*(pt-pf)*t^2 + 2*(pf-pt)*t^3]   +
 *			3*[t - 2*t^2 + t^3]*Δf*rf   +
 *			3*[t^2-t^3]*Δt*rt   */

/* Now for a long time I looked at that and saw four equations and two unknowns*/
/*  That was I was trying to solve for x and y separately, and that doesn't work. */
/*  There are really just two equations and each sums over both x and y components */

/* Old comment: */
/* I used to do a least squares approach adding two more to the above set of equations */
/*  which held the slopes constant. But that didn't work very well. So instead*/
/*  Then I tried doing the approximation, and then forcing the control points */
/*  to be in line (with the original slopes), getting a better approximation */
/*  to "t" for each data point and then calculating an error array, approximating*/
/*  it, and using that to fix up the final result */
/* Then I tried checking various possible cp lengths in the desired directions*/
/*  finding the best one or two, and doing a 2D binary search using that as a */
/*  starting point. */
/* And sometimes a least squares approach will give us the right answer, so   */
/*  try that too. */
/* This still isn't as good as I'd like it... But I haven't been able to */
/*  improve it further yet */
/* The mergetype mt is either of: */
/*  mt_matrix; original, fast, all-purpose (relies on matrix calculations) */
/*  mt_levien; by Raph Levien (implemented by Linus Romer), fast, accurate, use only if mid is on spline */
/*  mt_bruteforce; slow, all-purpose, normally more accurate than mt_matrix.*/
/* The mt_levien algorithm is explained here: */
/* raphlinus.github.io/curves/2021/03/11/bezier-fitting.html */
/* The notation used here is a bit different: Instead of theta1, theta2, */
/* delta1, delta2, momentx, area we use alpha,beta,a,b,m,f: */
/* Here is to complete math that we are using: */
/* Signed area of the cubic bezier spline a .. controls b and c .. d to the x-axis  */
/* f = ((xb-xa)*(10*ya+6*yb+3*yc+yd)+(xc-xb)*(4*ya+6*yb+6*yc+4*yd)+(xd-xc)*(ya+3*yb+6*yc+10*yd))/20; */
/* simplified for the normed case */
/* f = 3/20*(2*a*sin(alpha)+2*b*sin(beta)-a*b*sin(alpha+beta)); */
/* solved for b */
/* b = (20*f-6*a*sin(alpha))/(6*sin(beta)-3*a*sin(alpha+beta)). */
/* Signed area of the cubic bezier spline a .. controls b and c .. d to the x-axis  */
/* from point a up to the bezier point at time t */
/* f(t) = ((((1-t)*xa+xb*t)-xa)*(10*ya+6*((1-t)*ya+yb*t)+3*((1-t)^2*ya+2*(1-t)*t*yb+t^2*yc) */
/* +((1-t)^3*ya+3*(1-t)^2*t*yb+3*(1-t)*t^2*yc+t^3*yd))+(((1-t)^2*xa+2*(1-t)*t*xb+t^2*xc) */
/* -((1-t)*xa+xb*t))*(4*ya+6*((1-t)*ya+yb*t)+6*((1-t)^2*ya+2*(1-t)*t*yb+t^2*yc) */
/* +4*((1-t)^3*ya+3*(1-t)^2*t*yb+3*(1-t)*t^2*yc+t^3*yd))+(((1-t)^3*xa+3*(1-t)^2*t*xb */
/* +3*(1-t)*t^2*xc+t^3*xd)-((1-t)^2*xa+2*(1-t)*t*xb+t^2*xc))*(ya+3*((1-t)*ya+yb*t) */
/* +6*((1-t)^2*ya+2*(1-t)*t*yb+t^2*yc)+10*((1-t)^3*ya+3*(1-t)^2*t*yb+3*(1-t)*t^2*yc+t^3*yd)))/20; */
/* simplified for the normed case: */
/* f(t) = -(3*(30*a*b*sin(beta-alpha)*t^6+15*b^2*sin(2*beta)*t^6-20*b*sin(beta)*t^6 */
/* -15*a^2*sin(2*alpha)*t^6+20*a*sin(alpha)*t^6+6*a*b*sin(beta+alpha)*t^5 */
/* -90*a*b*sin(beta-alpha)*t^5-30*b^2*sin(2*beta)*t^5+48*b*sin(beta)*t^5 */
/* +60*a^2*sin(2*alpha)*t^5-72*a*sin(alpha)*t^5-15*a*b*sin(beta+alpha)*t^4 */
/* +90*a*b*sin(beta-alpha)*t^4+15*b^2*sin(2*beta)*t^4-30*b*sin(beta)*t^4 */
/* -90*a^2*sin(2*alpha)*t^4+90*a*sin(alpha)*t^4+10*a*b*sin(beta+alpha)*t^3 */
/* -30*a*b*sin(beta-alpha)*t^3+60*a^2*sin(2*alpha)*t^3-40*a*sin(alpha)*t^3 */
/* -15*a^2*sin(2*alpha)*t^2))/20. */
/* First moment about y-axis = \int x dA = \int x dA/dt dt for a cubic bezier  */
/* path a .. controls b and c .. d */
/* m = (280*xd^2*yd-105*xc*xd*yd-30*xb*xd*yd-5*xa*xd*yd-45*xc^2*yd-45*xb*xc*yd */
/* -12*xa*xc*yd-18*xb^2*yd-15*xa*xb*yd-5*xa^2*yd+105*xd^2*yc+45*xc*xd*yc */
/* -3*xa*xd*yc-27*xb*xc*yc-18*xa*xc*yc-27*xb^2*yc-45*xa*xb*yc-30*xa^2*yc */
/* +30*xd^2*yb+45*xc*xd*yb+18*xb*xd*yb+3*xa*xd*yb+27*xc^2*yb+27*xb*xc*yb */
/* -45*xa*xb*yb-105*xa^2*yb+5*xd^2*ya+15*xc*xd*ya+12*xb*xd*ya+5*xa*xd*ya */
/* +18*xc^2*ya+45*xb*xc*ya+30*xa*xc*ya+45*xb^2*ya+105*xa*xb*ya-280*xa^2*ya)/840; */
/* simplified for the normed case */
/* m = (9*a*cos(alpha)*b^2*cos(beta)*sin(beta)-15*b^2*cos(beta)*sin(beta) */
/* -9*a^2*cos(alpha)^2*b*sin(beta)-9*a*cos(alpha)*b*sin(beta)+50*b*sin(beta) */
/* +9*a*sin(alpha)*b^2*cos(beta)^2-9*a^2*cos(alpha)*sin(alpha)*b*cos(beta) */
/* -33*a*sin(alpha)*b*cos(beta)+15*a^2*cos(alpha)*sin(alpha)+34*a*sin(alpha))/280; */
/* normed case combined with the formula for b depending on the area (see above): */
/* m = (34*a*sin(alpha)+50*(20*f-6*a*sin(alpha))/(6*sin(beta)-3*a*sin(beta+alpha))*sin(beta) */
/* +15*a^2*sin(alpha)*cos(alpha)-15*(20*f-6*a*sin(alpha))/(6*sin(beta) */
/* -3*a*sin(beta+alpha))^2*sin(beta)*cos(beta)-a*(20*f-6*a*sin(alpha))/(6*sin(beta) */
/* -3*a*sin(beta+alpha))*(33*sin(alpha)*cos(beta)+9*cos(alpha)*sin(beta)) */
/* -9*a^2*(20*f-6*a*sin(alpha))/(6*sin(beta)-3*a*sin(beta+alpha))*sin(alpha+beta)*cos(alpha) */
/* +9*a*(20*f-6*a*sin(alpha))/(6*sin(beta)-3*a*sin(beta+alpha))^2*sin(alpha+beta)*cos(beta))/280; */
/* and reduced to a quartic equation with sa = sin(alpha), sb = sin(beta), ca = cos(alpha), cb = cos(beta) */
/* 0 = -9*ca*(((2*sb*cb*ca+sa*(2*cb*cb-1))*ca-2*sb*cb)*ca-cb*cb*sa) * a^4 */
/* + 12*((((cb*(30*f*cb-sb)-15*f)*ca+2*sa-cb*sa*(cb+30*f*sb))*ca+cb*(sb-15*f*cb))*ca-sa*cb*cb) * a^3 */
/* + 12*((((70*m+15*f)*sb^2+cb*(9*sb-70*cb*m-5*cb*f))*ca-5*sa*sb*(3*sb-4*cb*(7*m+f)))*ca-cb*(9*sb-70*cb*m-5*cb*f)) * a^2 */
/* + 16*(((12*sa-5*ca*(42*m-17*f))*sb-70*cb*(3*m-f)*sa-75*ca*cb*f*f)*sb-75*cb^2*f^2*sa) * a */
/* + 80*sb*(42*sb*m-25*f*(sb-cb*f)); */
/* this quartic equation reduces to a quadratic for the special case beta = pi - alpha or beta = -alpha */
/* 0 = -9*ca*sa^2 * a^3  */
/* + 6*sa*(4*sa+5*ca*f) * a^2 */
/* + 10*((42*m-25*f)*sa-25*ca*f^2). */
/* The derivative of the first moment (not the quartic) = 0 results in a quartic as well: */
/* 0 = -9*ca*sa*sab^3 * a^4 */
/* -3*sab^2*(9*ca*sa*sb-(17*sa+30*ca*f)*sab+15*cb*sa^2) * a^3 */
/* +18*sab*sb*(21*ca*sa*sb-(17*sa+30*ca*f)*sab+15*cb*sa^2) * a^2 */
/* -4*(144*ca*sa*sb^3+((-78*sa-135*ca*f)*sab+108*cb*sa^2)*sb^2+(-125*f*sab^2-45*cb*f*sa*sab)*sb+150*cb*f^2*sab^2) * a */
/* +8*sb*((24*sa+45*ca*f)*sb^2+(15*cb*f*sa-125*f*sab)*sb+100*cb*f^2*sab) */
/* this quartic equation reduces to a linear for the special case beta = pi - alpha or beta = -alpha */
/* 0 = -3*ca*sa * a */
/* +4*sa+5*ca*f */
#define TRY_CNT		2
#define DECIMATION	5
Spline *ApproximateSplineFromPointsSlopes(SplinePoint *from, SplinePoint *to,
	FitPoint *mid, int cnt, int order2, enum mergetype mt) {
    BasePoint tounit, fromunit, ftunit;
    bigreal flen,tlen,ftlen,dot;
    Spline *spline, temp;
    BasePoint nextcp;
    int bettern, betterp;
    bigreal offn, offp, incrn, incrp, trylen;
    int nocnt = 0, totcnt;
    bigreal curdiff, bestdiff[TRY_CNT];
    int i,j,besti[TRY_CNT],bestj[TRY_CNT],k,l;
    bigreal fdiff, tdiff, fmax, tmax, fdotft, tdotft;
    DBounds b;
    struct dotbounds db;
    bigreal offn_, offp_, finaldiff;
    bigreal pt_pf_x, pt_pf_y, determinant;
    bigreal consts[2], rt_terms[2], rf_terms[2];

    /* If all the selected points are at the same spot, and one of the */
    /*  end-points is also at that spot, then just copy the control point */
    /* But our caller seems to have done that for us */

    /* If the two end-points are corner points then allow the slope to vary */
    /* Or if one end-point is a tangent but the point defining the tangent's */
    /*  slope is being removed then allow the slope to vary */
    /* Except if the slope is horizontal or vertical then keep it fixed */
    if ( ( !from->nonextcp && ( from->nextcp.x==from->me.x || from->nextcp.y==from->me.y)) ||
	    (!to->noprevcp && ( to->prevcp.x==to->me.x || to->prevcp.y==to->me.y)) )
	/* Preserve the slope */;
    else if ( ((from->pointtype == pt_corner && from->nonextcp) ||
		(from->pointtype == pt_tangent &&
			((from->nonextcp && from->noprevcp) || !from->noprevcp))) &&
	    ((to->pointtype == pt_corner && to->noprevcp) ||
		(to->pointtype == pt_tangent &&
			((to->nonextcp && to->noprevcp) || !to->nonextcp))) ) {
	from->pointtype = to->pointtype = pt_corner;
return( ApproximateSplineFromPoints(from,to,mid,cnt,order2) );
    }

    /* If we are going to honour the slopes of a quadratic spline, there is */
    /*  only one possibility */
    if ( order2 ) {
	if ( from->nonextcp )
	    from->nextcp = from->next->to->me;
	if ( to->noprevcp )
	    to->prevcp = to->prev->from->me;
	fromunit.x = from->nextcp.x-from->me.x; fromunit.y = from->nextcp.y-from->me.y;
	tounit.x = to->prevcp.x-to->me.x; tounit.y = to->prevcp.y-to->me.y;

	if ( !IntersectLines(&nextcp,&from->nextcp,&from->me,&to->prevcp,&to->me) ||
		(nextcp.x-from->me.x)*fromunit.x + (nextcp.y-from->me.y)*fromunit.y < 0 ||
		(nextcp.x-to->me.x)*tounit.x + (nextcp.y-to->me.y)*tounit.y < 0 ) {
	    /* If the slopes don't intersect then use a line */
	    /*  (or if the intersection is patently absurd) */
	    from->nextcp = from->me;
	    to->prevcp = to->me;
	    TestForLinear(from,to);
	} else {
	    from->nextcp = to->prevcp = nextcp;
	}
return( SplineMake2(from,to));
    }
    /* From here down we are only working with cubic splines */

    if ( cnt==0 ) {
	/* Just use what we've got, no data to improve it */
	/* But we do sometimes get some cps which are too big */
	bigreal len = sqrt((to->me.x-from->me.x)*(to->me.x-from->me.x) + (to->me.y-from->me.y)*(to->me.y-from->me.y));
	if ( len==0 ) {
	    from->nextcp = from->me;
	    to->prevcp = to->me;
	} else {
	    BasePoint noff, poff;
	    bigreal nlen, plen;
	    noff.x = from->nextcp.x-from->me.x; noff.y = from->nextcp.y-from->me.y;
	    poff.x = to->me.x-to->prevcp.x; poff.y = to->me.y-to->prevcp.y;
	    nlen = sqrt(noff.x*noff.x + noff.y+noff.y);
	    plen = sqrt(poff.x*poff.x + poff.y+poff.y);
	    if ( nlen>len/3 ) {
		noff.x *= len/3/nlen; noff.y *= len/3/nlen;
		from->nextcp.x = from->me.x + noff.x;
		from->nextcp.y = from->me.y + noff.y;
	    }
	    if ( plen>len/3 ) {
		poff.x *= len/3/plen; poff.y *= len/3/plen;
		to->prevcp.x = to->me.x + poff.x;
		to->prevcp.y = to->me.y + poff.y;
	    }
	}
return( SplineMake3(from,to));
    }

    if ( to->prev!=NULL && (( to->noprevcp && to->nonextcp ) || to->prev->knownlinear )) {
	tounit.x = to->prev->from->me.x-to->me.x; tounit.y = to->prev->from->me.y-to->me.y;
    } else if ( !to->noprevcp || to->pointtype == pt_corner ) {
	tounit.x = to->prevcp.x-to->me.x; tounit.y = to->prevcp.y-to->me.y;
    } else {
	tounit.x = to->me.x-to->nextcp.x; tounit.y = to->me.y-to->nextcp.y;
    }
    tlen = sqrt(tounit.x*tounit.x + tounit.y*tounit.y);
    if ( from->next!=NULL && (( from->noprevcp && from->nonextcp ) || from->next->knownlinear) ) {
	fromunit.x = from->next->to->me.x-from->me.x; fromunit.y = from->next->to->me.y-from->me.y;
    } else if ( !from->nonextcp || from->pointtype == pt_corner ) {
	fromunit.x = from->nextcp.x-from->me.x; fromunit.y = from->nextcp.y-from->me.y;
    } else {
	fromunit.x = from->me.x-from->prevcp.x; fromunit.y = from->me.y-from->prevcp.y;
    }
    flen = sqrt(fromunit.x*fromunit.x + fromunit.y*fromunit.y);
    if ( (tlen==0 || flen==0) && (from->next==NULL || to->prev==NULL) ) {
	memset(&temp,0,sizeof(temp));
	temp.from = from; temp.to = to;
	SplineRefigure(&temp);
	from->next = to->prev = NULL;
    }
    if ( tlen==0 ) {
	if ( to->prev!=NULL ) {
	    temp = *to->prev;
	}
	if ( (to->pointtype==pt_curve || to->pointtype==pt_hvcurve) &&
		to->next && !to->nonextcp ) {
	    tounit.x = to->me.x-to->nextcp.x; tounit.y = to->me.y-to->nextcp.y;
	} else {
	    tounit.x = -( (3*temp.splines[0].a*.9999+2*temp.splines[0].b)*.9999+temp.splines[0].c );
	    tounit.y = -( (3*temp.splines[1].a*.9999+2*temp.splines[1].b)*.9999+temp.splines[1].c );
	}
	tlen = sqrt(tounit.x*tounit.x + tounit.y*tounit.y);
    }
    tounit.x /= tlen; tounit.y /= tlen;

    if ( flen==0 ) {
	if ( from->next!=NULL ) {
	    temp = *from->next;
	}
	if ( (from->pointtype==pt_curve || from->pointtype==pt_hvcurve) &&
		from->prev && !from->noprevcp ) {
	    fromunit.x = from->me.x-from->prevcp.x; fromunit.y = from->me.y-from->prevcp.y;
	} else {
	    fromunit.x = ( (3*temp.splines[0].a*.0001+2*temp.splines[0].b)*.0001+temp.splines[0].c );
	    fromunit.y = ( (3*temp.splines[1].a*.0001+2*temp.splines[1].b)*.0001+temp.splines[1].c );
	}
	flen = sqrt(fromunit.x*fromunit.x + fromunit.y*fromunit.y);
    }
    fromunit.x /= flen; fromunit.y /= flen;

    ftunit.x = (to->me.x-from->me.x); ftunit.y = (to->me.y-from->me.y);
    ftlen = sqrt(ftunit.x*ftunit.x + ftunit.y*ftunit.y);
    if ( ftlen!=0 ) {
	ftunit.x /= ftlen; ftunit.y /= ftlen;
    }

    if ( (dot=fromunit.x*tounit.y - fromunit.y*tounit.x)<.0001 && dot>-.0001 &&
	    (dot=ftunit.x*tounit.y - ftunit.y*tounit.x)<.0001 && dot>-.0001 ) {
	/* It's a line. Slopes are parallel, and parallel to vector between (from,to) */
	from->nextcp = from->me; to->prevcp = to->me;
return( SplineMake3(from,to));
    }
    /* This is the generic case, where a generic part is approximated by a cubic */
    /* bezier spline. */
    if ( ( ftlen == 0 ) && ( mt != mt_matrix ) )
		mt = mt_matrix; 
    if ( mt == mt_levien ) { 
	bigreal f,m,xa,ya,xb,yb,xc,yc,xd,yd,sasa,sab;
	int numberOfSolutions;
    SplinePoint *frompoint,*topoint;
    f = 0; /* area */
    m = 0; /* first area moment about y (along x) */
    frompoint = from;
    if ( from->next==NULL )
		topoint=to;
	else
		topoint=from->next->to;
	for ( ; ; frompoint = topoint->next->from, topoint = topoint->next->to ) {
		/* normalizing transformation (chord to x axis and length 1) */
		xa = ((frompoint->me.x-from->me.x)*ftunit.x+(frompoint->me.y-from->me.y)*ftunit.y)/ftlen;
		ya = (-(frompoint->me.x-from->me.x)*ftunit.y+(frompoint->me.y-from->me.y)*ftunit.x)/ftlen;
		xb = ((frompoint->nextcp.x-from->me.x)*ftunit.x+(frompoint->nextcp.y-from->me.y)*ftunit.y)/ftlen;
		yb = (-(frompoint->nextcp.x-from->me.x)*ftunit.y+(frompoint->nextcp.y-from->me.y)*ftunit.x)/ftlen;
		xc = ((topoint->prevcp.x-from->me.x)*ftunit.x+(topoint->prevcp.y-from->me.y)*ftunit.y)/ftlen;
		yc = (-(topoint->prevcp.x-from->me.x)*ftunit.y+(topoint->prevcp.y-from->me.y)*ftunit.x)/ftlen;
		xd = ((topoint->me.x-from->me.x)*ftunit.x+(topoint->me.y-from->me.y)*ftunit.y)/ftlen;
		yd = (-(topoint->me.x-from->me.x)*ftunit.y+(topoint->me.y-from->me.y)*ftunit.x)/ftlen;
		f += ((xb-xa)*(10*ya+6*yb+3*yc+yd)+(xc-xb)*(4*ya+6*yb+6*yc+4*yd)+(xd-xc)*(ya+3*yb+6*yc+10*yd))/20;
		m += (280*xd*xd*yd-105*xc*xd*yd-30*xb*xd*yd-5*xa*xd*yd-45*xc*xc*yd-45*xb*xc*yd-12*xa*xc*yd-18*xb*xb*yd
		-15*xa*xb*yd-5*xa*xa*yd+105*xd*xd*yc+45*xc*xd*yc-3*xa*xd*yc-27*xb*xc*yc-18*xa*xc*yc-27*xb*xb*yc
		-45*xa*xb*yc-30*xa*xa*yc+30*xd*xd*yb+45*xc*xd*yb+18*xb*xd*yb+3*xa*xd*yb+27*xc*xc*yb+27*xb*xc*yb
		-45*xa*xb*yb-105*xa*xa*yb+5*xd*xd*ya+15*xc*xd*ya+12*xb*xd*ya+5*xa*xd*ya+18*xc*xc*ya+45*xb*xc*ya
		+30*xa*xc*ya+45*xb*xb*ya+105*xa*xb*ya-280*xa*xa*ya)/840;
		if ( topoint==to )
			break;
    }
    BasePoint aunit = (BasePoint) { BPDot(ftunit, fromunit), BPCross(ftunit, fromunit) }; /* normed direction at "from" */
    BasePoint bunit = (BasePoint) { BPDot(BPRev(ftunit), tounit),BPCross(ftunit, tounit) }; /* normed direction at "to" */
    if ( aunit.y < 0 ) { /* normalize aunit.y to >= 0: */
		aunit.y = -aunit.y;
		bunit.y = -bunit.y;
		m = -m;
		f = -f;
	}
	/* calculate the Tunni point (where the tangents at "from" and "to" intersect) */
	bigreal aMax = 100; /* maximal value that the handle a can reach up to the Tunni point, 100 is really long */
	bigreal bMax = 100; /* maximal value that the handle b can reach up to the Tunni point, 100 is really long */
	sab = aunit.y*bunit.x+aunit.x*bunit.y; 
	if (sab != 0) { /* if handles not parallel */
		aMax = bunit.y/sab;
		bMax = aunit.y/sab;
		if ( aMax < 0 ) {
				aMax = 100;
		}
		if ( bMax < 0 ) {
				bMax = 100;
		}
	}
	/* start approximation by solving the quartic equation */
	sasa = aunit.y*aunit.y; /* reducing the multiplications */
	Quartic quad;
	if ( (aunit.x == -bunit.x && aunit.y == bunit.y) || (aunit.x == bunit.x && aunit.y == -bunit.y) ) { /* handles are parallel */
		quad.a = 0;
		quad.b = 0;
		quad.c = -9*aunit.x*sasa;
		quad.d = 6*aunit.y*(4*aunit.y+5*aunit.x*f);	
		quad.e = 10*((42*m-25*f)*aunit.y-25*aunit.x*f*f);
	} else { /* generic situation */
		quad.a = -9*aunit.x*(((2*bunit.y*bunit.x*aunit.x+aunit.y
				*(2*bunit.x*bunit.x-1))*aunit.x-2*bunit.y*bunit.x)
				*aunit.x-bunit.x*bunit.x*aunit.y);
		quad.b = 12*((((bunit.x*(30*f*bunit.x-bunit.y)-15*f)
				*aunit.x+2*aunit.y-bunit.x*aunit.y*(bunit.x+30*f*bunit.y))
				*aunit.x+bunit.x*(bunit.y-15*f*bunit.x))
				*aunit.x-aunit.y*bunit.x*bunit.x);	
		quad.c = 12*((((70*m+15*f)*bunit.y*bunit.y+bunit.x
				*(9*bunit.y-70*bunit.x*m-5*bunit.x*f))
				*aunit.x-5*aunit.y*bunit.y*(3*bunit.y-4*bunit.x
				*(7*m+f)))*aunit.x-bunit.x*(9*bunit.y-70*bunit.x*m-5*bunit.x*f));
		quad.d = 16*(((12*aunit.y-5*aunit.x*(42*m-17*f))*bunit.y
				-70*bunit.x*(3*m-f)*aunit.y-75*aunit.x*bunit.x*f*f)
				*bunit.y-75*bunit.x*bunit.x*f*f*aunit.y);
		quad.e = 80*bunit.y*(42*bunit.y*m-25*f*(bunit.y-bunit.x*f));
	}
	extended solutions[4] = {-999999,-999999,-999999,-999999};
	_QuarticSolve(&quad,solutions);
	extended abSolutions[10][2]; /* there are at most 4+4+1+1=10 solutions of pairs of a and b (quartic=0,derivative=0,b=0.01,a=0.01) */
	numberOfSolutions = 0;
	extended a,b;
	for( int i = 0; i < 4; i++ ){
		a = solutions[i];
		if ( a >= 0 && a < aMax ) {
			b = (20*f-6*a*aunit.y)/(3*(2*bunit.y-a*sab));
			if ( b >= 0 && b < bMax ) {
				abSolutions[numberOfSolutions][0] = a;
				abSolutions[numberOfSolutions++][1] = b;
			}
		}
	}
	/* and now again for the derivative (of m not of the upper quartic): */
	if ( (aunit.x == -bunit.x && aunit.y == bunit.y) || (aunit.x == bunit.x && aunit.y == -bunit.y) ) { /* handles are parallel */
		quad.a = 0;
		quad.b = 0;
		quad.c = 0;
		quad.d = -3*aunit.x*aunit.y;
		quad.e = 4*aunit.y+5*aunit.x*f;
	} else { /* generic situation */
		bigreal sbsb = bunit.y*bunit.y;
		bigreal sabsab = sab*sab;
		quad.a = -9*aunit.x*aunit.y*sabsab*sab;
		quad.b = -3*sabsab*(9*aunit.x*aunit.y*bunit.y-(17*aunit.y
				+30*aunit.x*f)*sab+15*bunit.x*sasa);	
		quad.c = 18*sab*bunit.y*(21*aunit.x*aunit.y*bunit.y-(17*aunit.y
				+30*aunit.x*f)*sab+15*bunit.x*sasa);
		quad.d = -4*(144*aunit.x*aunit.y*sbsb*bunit.y+((-78*aunit.y-
				135*aunit.x*f)*sab+108*bunit.x*sasa)*sbsb+(-125*f*sabsab
				-45*bunit.x*f*aunit.y*sab)*bunit.y+150*bunit.x*f*f*sabsab);
		quad.e = 8*bunit.y*((24*aunit.y+45*aunit.x*f)*sbsb
				+(15*bunit.x*f*aunit.y-125*f*sab)*bunit.y+100*bunit.x*f*f*sab);				
	}
	for( int i = 0; i < 4; i++ ) /* overwriting (reusing) */
		solutions[i] = -999999;
	_QuarticSolve(&quad,solutions); 
	for( int i = 0; i < 4; i++ ){
		a = solutions[i];
		if ( a >= 0 && a < aMax ) {
			b = (20*f-6*a*aunit.y)/(3*(2*bunit.y-a*sab));
			if ( b >= 0 && b < bMax ) {
				abSolutions[numberOfSolutions][0] = a;
				abSolutions[numberOfSolutions++][1] = b;
			} 
		}
	}
	/* Add the solution of b = 0.01 (approximately 0 but above because of direction). */
	/* This solution is not part of the original algorithm by Raph Levien. */
	a = (2000*f-6*bunit.y)/(600*aunit.y-3*sab);
	if ( a >= 0 && a < aMax ) {
		abSolutions[numberOfSolutions][0] = a;
		abSolutions[numberOfSolutions++][1] = 0.01;
	}
	/* Add the solution of a = 0.01 (approximately 0 but above because of direction). */
	/* This solution is not part of the original algorithm by Raph Levien. */
	b = (2000*f-6*aunit.y)/(600*bunit.y-3*sab);
	if ( b >= 0 && b < bMax ) {
		abSolutions[numberOfSolutions][0] = 0.01;
		abSolutions[numberOfSolutions++][1] = b;
	}	
	if ( numberOfSolutions == 0) { /* add solutions that extend up to the Tunni point */
		/* try solution with a = aMax and b area-equal*/
		b = (20*f-6*aMax*aunit.y)/(3*(2*bunit.y-aMax*sab));
		if ( b >= 0 && b < bMax ) {
			abSolutions[numberOfSolutions][0] = aMax;
			abSolutions[numberOfSolutions++][1] = b;
		}
		/* try solution with b = bMax and a area-equal*/
		a = (20*f-6*bMax*bunit.y)/(3*(2*aunit.y-bMax*sab));
		if ( a >= 0 && a < aMax ) {
			abSolutions[numberOfSolutions][0] = a;
			abSolutions[numberOfSolutions++][1] = bMax;
		}
	}
	if ( numberOfSolutions == 0) {
		/* solution with a = aMax and b = bMax*/
		abSolutions[numberOfSolutions][0] = aMax;
		abSolutions[numberOfSolutions++][1] = bMax;
	}
	if ( numberOfSolutions == 1) { 
		from->nextcp.x = from->me.x+ftlen*fromunit.x*abSolutions[0][0];
		from->nextcp.y = from->me.y+ftlen*fromunit.y*abSolutions[0][0];
		to->prevcp.x = to->me.x+ftlen*tounit.x*abSolutions[0][1];
		to->prevcp.y = to->me.y+ftlen*tounit.y*abSolutions[0][1];
	} else { /* compare L2 errors to choose the best solution */
		bigreal bestError = 1e30;
		bigreal t,error,errorsum,dist;
		BasePoint prevcp,nextcp,coeff1,coeff2,coeff3;	
		int last_best_j;
		for (int k=0; k<numberOfSolutions; k++) {
			nextcp.x = from->me.x+ftlen*fromunit.x*abSolutions[k][0];
			nextcp.y = from->me.y+ftlen*fromunit.y*abSolutions[k][0];
			prevcp.x = to->me.x+ftlen*tounit.x*abSolutions[k][1];
			prevcp.y = to->me.y+ftlen*tounit.y*abSolutions[k][1];
			/* Calculate the error of the cubic bezier path from,nextcp,prevcp,to: */
			/* In order to do that we calculate 99 points on the bezier path. */
			coeff3.x = -from->me.x+3*nextcp.x-3*prevcp.x+to->me.x;
			coeff3.y = -from->me.y+3*nextcp.y-3*prevcp.y+to->me.y;
			coeff2.x = 3*from->me.x-6*nextcp.x+3*prevcp.x;
			coeff2.y = 3*from->me.y-6*nextcp.y+3*prevcp.y;
			coeff1.x = -3*from->me.x+3*nextcp.x;
			coeff1.y = -3*from->me.y+3*nextcp.y;
			BasePoint approx[99];
			for (int i=0; i<99; i++) {
				t = (i+1)/100.0;
				approx[i].x = from->me.x+t*(coeff1.x+t*(coeff2.x+t*coeff3.x));
				approx[i].y = from->me.y+t*(coeff1.y+t*(coeff2.y+t*coeff3.y));
			}
			/* Now we calculate the error by determining the minimal quadratic distance to the mid points. */
			errorsum = 0.0;
			last_best_j = 0;
			for (int i=0; i<cnt; i++) { /* Going through the mid points */
				error = 1e30;
				/* For this mid point, find the distance to the closest one of the */
				/* 99 points on the approximate cubic bezier. */
				/* To not favour approximations which trace the original multiple times */
				/* by going back and forth, only consider monotonic mappings. */
				/* I.e., start from the point that was closest to the previous mid point: */
				for (int j=last_best_j; j<99; j++) {
					dist = (mid[i].p.x-approx[j].x)*(mid[i].p.x-approx[j].x)
					+(mid[i].p.y-approx[j].y)*(mid[i].p.y-approx[j].y);
					if (dist < error) {
						error = dist;
						last_best_j = j;
					}
				}
				errorsum += error;
				if (errorsum > bestError)
					break;
			}
			if (errorsum < bestError) {
				bestError = errorsum;
				from->nextcp = nextcp;
				to->prevcp = prevcp;
			}
		}
    } 
    return( SplineMake3(from,to));
	} else if ( mt == mt_bruteforce ) { 
	bigreal best_error = 1e30;
	bigreal t,error,errorsum,dist;
	BasePoint prevcp,coeff1,coeff2,coeff3;
	bigreal best_fromhandle = 0.0;
	bigreal best_tohandle = 0.0;
	BasePoint approx[99]; /* The 99 points on the approximate cubic bezier */
	/* We make 2 runs: The first run to narrow the variation range, the second run to finetune */
	/* The optimal length of the two handles are determined by brute force. */
	for (int run=0; run<2; ++run) {
		for (int fromhandle=((run==0)?1:-29); fromhandle<=((run==0)?60:29); ++fromhandle) {
			for (int tohandle=((run==0)?1:-29); tohandle<=((run==0)?60:29); ++tohandle) {
				nextcp.x = from->me.x+ftlen*fromunit.x*( (run==0)?fromhandle:best_fromhandle+fromhandle/30.0 )/60.0;
				nextcp.y = from->me.y+ftlen*fromunit.y*( (run==0)?fromhandle:best_fromhandle+fromhandle/30.0 )/60.0;
				prevcp.x = to->me.x+ftlen*tounit.x*( (run==0)?tohandle:best_tohandle+tohandle/30.0 )/60.0;
				prevcp.y = to->me.y+ftlen*tounit.y*( (run==0)?tohandle:best_tohandle+tohandle/30.0 )/60.0;
				/* Calculate the error of the cubic bezier path from,nextcp,prevcp,to: */
				/* In order to do that we calculate 99 points on the bezier path. */
				coeff3.x = -from->me.x+3*nextcp.x-3*prevcp.x+to->me.x;
				coeff3.y = -from->me.y+3*nextcp.y-3*prevcp.y+to->me.y;
				coeff2.x = 3*from->me.x-6*nextcp.x+3*prevcp.x;
				coeff2.y = 3*from->me.y-6*nextcp.y+3*prevcp.y;
				coeff1.x = -3*from->me.x+3*nextcp.x;
				coeff1.y = -3*from->me.y+3*nextcp.y;
				for (int i=0; i<99; ++i) {
					t = (i+1)/100.0;
					approx[i].x = from->me.x+t*(coeff1.x+t*(coeff2.x+t*coeff3.x));
					approx[i].y = from->me.y+t*(coeff1.y+t*(coeff2.y+t*coeff3.y));
				}
				/* Now we calculate the error by determining the minimal quadratic distance to the mid points. */
				errorsum = 0.0;
				for (int i=0; i<cnt; ++i) { /* Going through the mid points */
					error = (mid[i].p.x-approx[0].x)*(mid[i].p.x-approx[0].x)
					+(mid[i].p.y-approx[0].y)*(mid[i].p.y-approx[0].y);
					/* Above we have just initialized the error and */
					/* now we are going through the remaining 98 of */
					/* 99 points on the approximate cubic bezier: */
					for (int j=1; j<99; ++j) {
						dist = (mid[i].p.x-approx[j].x)*(mid[i].p.x-approx[j].x)
						+(mid[i].p.y-approx[j].y)*(mid[i].p.y-approx[j].y);
						if (dist < error)
							error = dist;
					}
					errorsum += error;
					if (errorsum > best_error)
						break;
				}
				if (errorsum < best_error) {
					best_error = errorsum;
					if (run == 0) {
						best_fromhandle = fromhandle;
						best_tohandle = tohandle;
					}
					from->nextcp = nextcp;
					to->prevcp = prevcp;
				}
			}
		}
	}
	return( SplineMake3(from,to));
    }
	else { /* mergetype mt_matrix (original algorithm) */
    pt_pf_x = to->me.x - from->me.x;
    pt_pf_y = to->me.y - from->me.y;
    consts[0] = consts[1] = rt_terms[0] = rt_terms[1] = rf_terms[0] = rf_terms[1] = 0;
    for ( i=0; i<cnt; ++i ) {
	bigreal t = mid[i].t, t2 = t*t, t3=t2*t;
	bigreal factor_from = t-2*t2+t3;
	bigreal factor_to = t2-t3;
	bigreal const_x = from->me.x-mid[i].p.x + 3*pt_pf_x*t2 - 2*pt_pf_x*t3;
	bigreal const_y = from->me.y-mid[i].p.y + 3*pt_pf_y*t2 - 2*pt_pf_y*t3;
	bigreal temp1 = 3*(t-2*t2+t3);
	bigreal rf_term_x = temp1*fromunit.x;
	bigreal rf_term_y = temp1*fromunit.y;
	bigreal temp2 = 3*(t2-t3);
	bigreal rt_term_x = -temp2*tounit.x;
	bigreal rt_term_y = -temp2*tounit.y;

	consts[0] += factor_from*( fromunit.x*const_x + fromunit.y*const_y );
	consts[1] += factor_to *( -tounit.x*const_x + -tounit.y*const_y);
	rf_terms[0] += factor_from*( fromunit.x*rf_term_x + fromunit.y*rf_term_y);
	rf_terms[1] += factor_to*( -tounit.x*rf_term_x + -tounit.y*rf_term_y);
	rt_terms[0] += factor_from*( fromunit.x*rt_term_x + fromunit.y*rt_term_y);
	rt_terms[1] += factor_to*( -tounit.x*rt_term_x + -tounit.y*rt_term_y);
    }

 /* I've only seen singular matrices (determinant==0) when cnt==1 */
 /* but even with cnt==1 the determinant is usually non-0 (16 times out of 17)*/
    determinant = (rt_terms[0]*rf_terms[1]-rt_terms[1]*rf_terms[0]);
    if ( determinant!=0 ) {
	bigreal rt, rf;
	rt = (consts[1]*rf_terms[0]-consts[0]*rf_terms[1])/determinant;
	if ( rf_terms[0]!=0 )
	    rf = -(consts[0]+rt*rt_terms[0])/rf_terms[0];
	else /* if ( rf_terms[1]!=0 ) This can't happen, otherwise the determinant would be 0 */
	    rf = -(consts[1]+rt*rt_terms[1])/rf_terms[1];
	/* If we get bad values (ones that point diametrically opposed to what*/
	/*  we need), then fix that factor at 0, and see what we get for the */
	/*  other */
	if ( rf>=0 && rt>0 && rf_terms[0]!=0 &&
		(rf = -consts[0]/rf_terms[0])>0 ) {
	    rt = 0;
	} else if ( rf<0 && rt<=0 && rt_terms[1]!=0 &&
		(rt = -consts[1]/rt_terms[1])<0 ) {
	    rf = 0;
	}
	if ( rt<=0 && rf>=0 ) {
	    from->nextcp.x = from->me.x + rf*fromunit.x;
	    from->nextcp.y = from->me.y + rf*fromunit.y;
	    to->prevcp.x = to->me.x - rt*tounit.x;
	    to->prevcp.y = to->me.y - rt*tounit.y;
return( SplineMake3(from,to));
	}
    }

    trylen = (to->me.x-from->me.x)*fromunit.x + (to->me.y-from->me.y)*fromunit.y;
    if ( trylen>flen ) flen = trylen;

    trylen = (from->me.x-to->me.x)*tounit.x + (from->me.y-to->me.y)*tounit.y;
    if ( trylen>tlen ) tlen = trylen;

    for ( i=0; i<cnt; ++i ) {
	trylen = (mid[i].p.x-from->me.x)*fromunit.x + (mid[i].p.y-from->me.y)*fromunit.y;
	if ( trylen>flen ) flen = trylen;
	trylen = (mid[i].p.x-to->me.x)*tounit.x + (mid[i].p.y-to->me.y)*tounit.y;
	if ( trylen>tlen ) tlen = trylen;
    }

    fdotft = fromunit.x*ftunit.x + fromunit.y*ftunit.y;
    fmax = fdotft>0 ? ftlen/fdotft : 1e10;
    tdotft = -tounit.x*ftunit.x - tounit.y*ftunit.y;
    tmax = tdotft>0 ? ftlen/tdotft : 1e10;
    /* At fmax, tmax the control points will stretch beyond the other endpoint*/
    /*  when projected along the line between the two endpoints */

    db.base = from->me;
    db.unit = ftunit;
    db.len = ftlen;
    ApproxBounds(&b,mid,cnt,&db);

    for ( k=0; k<TRY_CNT; ++k ) {
	bestdiff[k] = 1e20;
	besti[k] = -1; bestj[k] = -1;
    }
    fdiff = flen/DECIMATION;
    tdiff = tlen/DECIMATION;
    from->nextcp = from->me;
    memset(&temp,0,sizeof(Spline));
    temp.from = from; temp.to = to;
    for ( i=1; i<DECIMATION; ++i ) {
	from->nextcp.x += fdiff*fromunit.x; from->nextcp.y += fdiff*fromunit.y;
	to->prevcp = to->me;
	for ( j=1; j<DECIMATION; ++j ) {
	    to->prevcp.x += tdiff*tounit.x; to->prevcp.y += tdiff*tounit.y;
	    SplineRefigure(&temp);
	    curdiff = SigmaDeltas(&temp,mid,cnt,&b,&db);
	    for ( k=0; k<TRY_CNT; ++k ) {
		if ( curdiff<bestdiff[k] ) {
		    for ( l=TRY_CNT-1; l>k; --l ) {
			bestdiff[l] = bestdiff[l-1];
			besti[l] = besti[l-1];
			bestj[l] = bestj[l-1];
		    }
		    bestdiff[k] = curdiff;
		    besti[k] = i; bestj[k]=j;
	    break;
		}
	    }
	}
    }

    finaldiff = 1e20;
    offn_ = offp_ = -1;
    spline = SplineMake(from,to,false);
    for ( k=-1; k<TRY_CNT; ++k ) {
	if ( k<0 ) {
	    BasePoint nextcp, prevcp;
	    bigreal temp1, temp2;
	    int ret = _ApproximateSplineFromPoints(from,to,mid,cnt,&nextcp,&prevcp,false);
	    /* sometimes least squares gives us the right answer */
	    if ( !(ret&1) || !(ret&2))
    continue;
	    temp1 = (prevcp.x-to->me.x)*tounit.x + (prevcp.y-to->me.y)*tounit.y;
	    temp2 = (nextcp.x-from->me.x)*fromunit.x + (nextcp.y-from->me.y)*fromunit.y;
	    if ( temp1<=0 || temp2<=0 )		/* A nice solution... but the control points are diametrically opposed to what they should be */
    continue;
	    tlen = temp1; flen = temp2;
	} else {
	    if ( bestj[k]<0 || besti[k]<0 )
    continue;
	    tlen = bestj[k]*tdiff; flen = besti[k]*fdiff;
	}
	to->prevcp.x = to->me.x + tlen*tounit.x; to->prevcp.y = to->me.y + tlen*tounit.y;
	from->nextcp.x = from->me.x + flen*fromunit.x; from->nextcp.y = from->me.y + flen*fromunit.y;
	SplineRefigure(spline);

	bettern = betterp = false;
	incrn = tdiff/2.0; incrp = fdiff/2.0;
	offn = flen; offp = tlen;
	nocnt = 0;
	curdiff = SigmaDeltas(spline,mid,cnt,&b,&db);
	totcnt = 0;
	for (;;) {
	    bigreal fadiff, fsdiff;
	    bigreal tadiff, tsdiff;

	    from->nextcp.x = from->me.x + (offn+incrn)*fromunit.x; from->nextcp.y = from->me.y + (offn+incrn)*fromunit.y;
	    to->prevcp.x = to->me.x + offp*tounit.x; to->prevcp.y = to->me.y + offp*tounit.y;
	    SplineRefigure(spline);
	    fadiff = SigmaDeltas(spline,mid,cnt,&b,&db);
	    from->nextcp.x = from->me.x + (offn-incrn)*fromunit.x; from->nextcp.y = from->me.y + (offn-incrn)*fromunit.y;
	    SplineRefigure(spline);
	    fsdiff = SigmaDeltas(spline,mid,cnt,&b,&db);
	    from->nextcp.x = from->me.x + offn*fromunit.x; from->nextcp.y = from->me.y + offn*fromunit.y;
	    if ( offn-incrn<=0 )
		fsdiff += 1e10;

	    to->prevcp.x = to->me.x + (offp+incrp)*tounit.x; to->prevcp.y = to->me.y + (offp+incrp)*tounit.y;
	    SplineRefigure(spline);
	    tadiff = SigmaDeltas(spline,mid,cnt,&b,&db);
	    to->prevcp.x = to->me.x + (offp-incrp)*tounit.x; to->prevcp.y = to->me.y + (offp-incrp)*tounit.y;
	    SplineRefigure(spline);
	    tsdiff = SigmaDeltas(spline,mid,cnt,&b,&db);
	    to->prevcp.x = to->me.x + offp*tounit.x; to->prevcp.y = to->me.y + offp*tounit.y;
	    if ( offp-incrp<=0 )
		tsdiff += 1e10;

	    if ( offn>=incrn && fsdiff<curdiff &&
		    (fsdiff<fadiff && fsdiff<tsdiff && fsdiff<tadiff)) {
		offn -= incrn;
		if ( bettern>0 )
		    incrn /= 2;
		bettern = -1;
		nocnt = 0;
		curdiff = fsdiff;
	    } else if ( offn+incrn<fmax && fadiff<curdiff &&
		    (fadiff<=fsdiff && fadiff<tsdiff && fadiff<tadiff)) {
		offn += incrn;
		if ( bettern<0 )
		    incrn /= 2;
		bettern = 1;
		nocnt = 0;
		curdiff = fadiff;
	    } else if ( offp>=incrp && tsdiff<curdiff &&
		    (tsdiff<=fsdiff && tsdiff<=fadiff && tsdiff<tadiff)) {
		offp -= incrp;
		if ( betterp>0 )
		    incrp /= 2;
		betterp = -1;
		nocnt = 0;
		curdiff = tsdiff;
	    } else if ( offp+incrp<tmax && tadiff<curdiff &&
		    (tadiff<=fsdiff && tadiff<=fadiff && tadiff<=tsdiff)) {
		offp += incrp;
		if ( betterp<0 )
		    incrp /= 2;
		betterp = 1;
		nocnt = 0;
		curdiff = tadiff;
	    } else {
		if ( ++nocnt > 6 )
	break;
		incrn /= 2;
		incrp /= 2;
	    }
	    if ( curdiff<1 )
	break;
	    if ( incrp<tdiff/1024 || incrn<fdiff/1024 )
	break;
	    if ( ++totcnt>200 )
	break;
	    if ( offn<0 || offp<0 ) {
		IError("Approximation got inverse control points");
	break;
	    }
	}
	if ( curdiff<finaldiff ) {
	    finaldiff = curdiff;
	    offn_ = offn;
	    offp_ = offp;
	}
    }

    to->noprevcp = offp_==0;
    from->nonextcp = offn_==0;
    to->prevcp.x = to->me.x + offp_*tounit.x; to->prevcp.y = to->me.y + offp_*tounit.y;
    from->nextcp.x = from->me.x + offn_*fromunit.x; from->nextcp.y = from->me.y + offn_*fromunit.y;
    /* I used to check for a spline very close to linear (and if so, make it */
    /*  linear). But in when stroking a path with an elliptical pen we transform*/
    /*  the coordinate system and our normal definitions of "close to linear" */
    /*  don't apply */
    /*TestForLinear(from,to);*/
    SplineRefigure(spline);

return( spline );
    }
}
#undef TRY_CNT
#undef DECIMATION

SplinePoint *_ApproximateSplineSetFromGen(SplinePoint *from, SplinePoint *to,
                                          bigreal start_t, bigreal end_t,
                                          bigreal toler, int toler_is_sumsq,
                                          GenPointsP genp, void *tok,
                                          int order2, int depth) {
    int cnt, i, maxerri=0, created = false;
    bigreal errsum=0, maxerr=0, d, mid_t;
    FitPoint *fp;
    SplinePoint *mid, *r;

    cnt = (*genp)(tok, start_t, end_t, &fp);
    if ( cnt < 2 )
	return NULL;

    // Rescale zero to one
    for ( i=1; i<(cnt-1); ++i )
	fp[i].t = (fp[i].t-fp[0].t)/(fp[cnt-1].t-fp[0].t);
    fp[0].t = 0.0;
    fp[cnt-1].t = 1.0;

    from->nextcp.x = from->me.x + fp[0].ut.x;
    from->nextcp.y = from->me.y + fp[0].ut.y;
    from->nonextcp = false;
    if ( to!=NULL )
	to->me = fp[cnt-1].p;
    else  {
	to = SplinePointCreate(fp[cnt-1].p.x, fp[cnt-1].p.y);
	created = true;
    }
    to->prevcp.x = to->me.x - fp[cnt-1].ut.x;
    to->prevcp.y = to->me.y - fp[cnt-1].ut.y;
    to->noprevcp = false;
    ApproximateSplineFromPointsSlopes(from,to,fp+1,cnt-2,order2,mt_matrix);

    for ( i=0; i<cnt; ++i ) {
	d = SplineMinDistanceToPoint(from->next, &fp[i].p);
	errsum += d*d;
	if ( d>maxerr ) {
	    maxerr = d;
	    maxerri = i;
	}
    }
    // printf("   Error sum %lf, max error %lf at depth %d\n", errsum, maxerr, depth);

    if ( (toler_is_sumsq ? errsum : maxerr) > toler && depth < 6 ) {
	mid_t = fp[maxerri].t * (end_t-start_t) + start_t;
	free(fp);
	SplineFree(from->next);
	from->next = NULL;
	to->prev = NULL;
	mid = _ApproximateSplineSetFromGen(from, NULL, start_t, mid_t, toler,
	                                   toler_is_sumsq, genp, tok, order2,
	                                   depth+1);
	if ( mid ) {
	    r = _ApproximateSplineSetFromGen(mid, to, mid_t, end_t, toler,
	                                     toler_is_sumsq, genp, tok,
	                                     order2, depth+1);
	    if ( r )
		return r;
	    else {
		if ( created )
		    SplinePointFree(to);
		else
		    to->prev = NULL;
		SplinePointFree(mid);
		SplineFree(from->next);
		from->next = NULL;
		return NULL;
	    }
	} else {
	    if ( created )
		SplinePointFree(to);
	    return NULL;
	}
    } else if ( (toler_is_sumsq ? errsum : maxerr) > toler ) {
	TRACE("%s %lf exceeds %lf at maximum depth %d\n",
	      toler_is_sumsq ? "Sum of squared errors" : "Maximum error length",
	      toler_is_sumsq ? errsum : maxerr, toler, depth);
    }
    free(fp);
    return to;
}

SplinePoint *ApproximateSplineSetFromGen(SplinePoint *from, SplinePoint *to,
                                          bigreal start_t, bigreal end_t,
                                          bigreal toler, int toler_is_sumsq,
                                          GenPointsP genp, void *tok,
                                          int order2) {
    return _ApproximateSplineSetFromGen(from, to, start_t, end_t, toler,
                                        toler_is_sumsq, genp, tok, order2, 0);
}
