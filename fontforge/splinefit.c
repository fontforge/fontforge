/* -*- coding: utf-8 -*- */
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

#include "fontforge.h"
#include "splinefit.h"
#include "splineorder2.h"
#include "splineutil.h"
#include "splineutil2.h"

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
    from->nonextcp = to->noprevcp = true;
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
	    from->nextcp = from->me; from->nonextcp = true;
	    to->prevcp = to->me; to->noprevcp = true;
	} else {
	    Spline temp;
	    memset(&temp,0,sizeof(temp));
	    temp.from = from; temp.to = to;
	    SplineRefigure(&temp);
	    if ( SplineIsLinear(&temp)) {
		from->nextcp = from->me; from->nonextcp = true;
		to->prevcp = to->me; to->noprevcp = true;
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
	    to->noprevcp = from->nonextcp = false;
return( SplineMake3(from,to));
	}
    }

    if ( (spline = IsLinearApprox(from,to,mid,cnt,order2))!=NULL )
return( spline );

    ret = _ApproximateSplineFromPoints(from,to,mid,cnt,&nextcp,&prevcp,order2);

    if ( ret&1 ) {
	from->nextcp = nextcp;
	from->nonextcp = false;
    } else {
	from->nextcp = from->me;
	from->nonextcp = true;
    }
    if ( ret&2 ) {
	to->prevcp = prevcp;
	to->noprevcp = false;
    } else {
	to->prevcp = to->me;
	to->noprevcp = true;
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
/* I used to do a least squares aproach adding two more to the above set of equations */
/*  which held the slopes constant. But that didn't work very well. So instead*/
/*  Then I tried doing the approximation, and then forcing the control points */
/*  to be in line (witht the original slopes), getting a better approximation */
/*  to "t" for each data point and then calculating an error array, approximating*/
/*  it, and using that to fix up the final result */
/* Then I tried checking various possible cp lengths in the desired directions*/
/*  finding the best one or two, and doing a 2D binary search using that as a */
/*  starting point. */
/* And sometimes a least squares approach will give us the right answer, so   */
/*  try that too. */
/* This still isn't as good as I'd like it... But I haven't been able to */
/*  improve it further yet */
#define TRY_CNT		2
#define DECIMATION	5
Spline *ApproximateSplineFromPointsSlopes(SplinePoint *from, SplinePoint *to,
	FitPoint *mid, int cnt, int order2) {
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
	from->nonextcp = to->noprevcp = false;
	fromunit.x = from->nextcp.x-from->me.x; fromunit.y = from->nextcp.y-from->me.y;
	tounit.x = to->prevcp.x-to->me.x; tounit.y = to->prevcp.y-to->me.y;

	if ( !IntersectLines(&nextcp,&from->nextcp,&from->me,&to->prevcp,&to->me) ||
		(nextcp.x-from->me.x)*fromunit.x + (nextcp.y-from->me.y)*fromunit.y < 0 ||
		(nextcp.x-to->me.x)*tounit.x + (nextcp.y-to->me.y)*tounit.y < 0 ) {
	    /* If the slopes don't intersect then use a line */
	    /*  (or if the intersection is patently absurd) */
	    from->nonextcp = to->noprevcp = true;
	    from->nextcp = from->me;
	    to->prevcp = to->me;
	    TestForLinear(from,to);
	} else {
	    from->nextcp = to->prevcp = nextcp;
	    from->nonextcp = to->noprevcp = false;
	}
return( SplineMake2(from,to));
    }
    /* From here down we are only working with cubic splines */

    if ( cnt==0 ) {
	/* Just use what we've got, no data to improve it */
	/* But we do sometimes get some cps which are too big */
	bigreal len = sqrt((to->me.x-from->me.x)*(to->me.x-from->me.x) + (to->me.y-from->me.y)*(to->me.y-from->me.y));
	if ( len==0 ) {
	    from->nonextcp = to->noprevcp = true;
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
    if ( tlen==0 || flen==0 ) {
	if ( from->next!=NULL )
	    temp = *from->next;
	else {
	    memset(&temp,0,sizeof(temp));
	    temp.from = from; temp.to = to;
	    SplineRefigure(&temp);
	    from->next = to->prev = NULL;
	}
    }
    if ( tlen==0 ) {
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
	from->nonextcp = to->noprevcp = true;
	from->nextcp = from->me; to->prevcp = to->me;
return( SplineMake3(from,to));
    }

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
	    from->nonextcp = rf==0;
	    to->noprevcp = rt==0;
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
    from->nonextcp = false;
    to->noprevcp = false;
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
    /*  linear). But in when stroking a path with an eliptical pen we transform*/
    /*  the coordinate system and our normal definitions of "close to linear" */
    /*  don't apply */
    /*TestForLinear(from,to);*/
    SplineRefigure(spline);

return( spline );
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
    ApproximateSplineFromPointsSlopes(from,to,fp+1,cnt-2,order2);

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
