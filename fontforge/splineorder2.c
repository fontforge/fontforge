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

#include "splineorder2.h"

#include "fontforge.h"
#include "splinerefigure.h"
#include "splineutil.h"
#include "splineutil2.h"
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <utype.h>
#include <ustring.h>
#include <chardata.h>
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif

/* This file contains utility routines for second order bezier splines   */
/*  (ie. truetype)							 */
/* The most interesting thing						 */
/*  it does is to figure out a quadratic approximation to the cubic splines */
/*  that postscript uses. We do this by looking at each spline and running */
/*  from the end toward the beginning, checking approximately every emunit */
/*  There is only one quadratic spline possible for any given interval of the */
/*  cubic. The start and end points are the interval end points (obviously) */
/*  the control point is where the two slopes (at start and end) intersect. */
/* If this spline is a close approximation to the cubic spline (doesn't */
/*  deviate from it by more than an emunit or so), then we use this interval */
/*  as one of our quadratic splines. */
/* It may turn out that the "quadratic" spline above is actually linear. Well */
/*  that's ok. It may also turn out that we can't find a good approximation. */
/*  If that's true then just insert a linear segment for an emunit stretch. */
/*  (actually this failure mode may not be possible), but I'm not sure */
/* Then we play the same trick for the rest of the cubic spline (if any) */

/* Does the quadratic spline in ttf approximate the cubic spline in ps */
/*  within one pixel between tmin and tmax (on ps. presumably ttf between 0&1 */
/* dim is the dimension in which there is the greatest change */
static int comparespline(Spline *ps, Spline *ttf, real tmin, real tmax, real err) {
    int dim=0, other;
    real dx, dy, ddim, dt, t;
    real d, o;
    real ttf_t, sq, val;
    DBounds bb;
    extended ts[3];
    int i;

    /* Are all points on ttf near points on ps? */
    /* This doesn't answer that question, but rules out gross errors */
    bb.minx = bb.maxx = ps->from->me.x; bb.miny = bb.maxy = ps->from->me.y;
    if ( ps->from->nextcp.x>bb.maxx ) bb.maxx = ps->from->nextcp.x;
    else bb.minx = ps->from->nextcp.x;
    if ( ps->from->nextcp.y>bb.maxy ) bb.maxy = ps->from->nextcp.y;
    else bb.miny = ps->from->nextcp.y;
    if ( ps->to->prevcp.x>bb.maxx ) bb.maxx = ps->to->prevcp.x;
    else if ( ps->to->prevcp.x<bb.minx ) bb.minx = ps->to->prevcp.x;
    if ( ps->to->prevcp.y>bb.maxy ) bb.maxy = ps->to->prevcp.y;
    else if ( ps->to->prevcp.y<bb.miny ) bb.miny = ps->to->prevcp.y;
    if ( ps->to->me.x>bb.maxx ) bb.maxx = ps->to->me.x;
    else if ( ps->to->me.x<bb.minx ) bb.minx = ps->to->me.x;
    if ( ps->to->me.y>bb.maxy ) bb.maxy = ps->to->me.y;
    else if ( ps->to->me.y<bb.miny ) bb.miny = ps->to->me.y;
    for ( t=.1; t<1; t+= .1 ) {
	d = (ttf->splines[0].b*t+ttf->splines[0].c)*t+ttf->splines[0].d;
	o = (ttf->splines[1].b*t+ttf->splines[1].c)*t+ttf->splines[1].d;
	if ( d<bb.minx || d>bb.maxx || o<bb.miny || o>bb.maxy )
return( false );
    }

    /* Are all points on ps near points on ttf? */
    dx = ((ps->splines[0].a*tmax+ps->splines[0].b)*tmax+ps->splines[0].c)*tmax -
	 ((ps->splines[0].a*tmin+ps->splines[0].b)*tmin+ps->splines[0].c)*tmin ;
    dy = ((ps->splines[1].a*tmax+ps->splines[1].b)*tmax+ps->splines[1].c)*tmax -
	 ((ps->splines[1].a*tmin+ps->splines[1].b)*tmin+ps->splines[1].c)*tmin ;
    if ( dx<0 ) dx = -dx;
    if ( dy<0 ) dy = -dy;
    if ( dx>dy ) {
	dim = 0;
	ddim = dx;
    } else {
	dim = 1;
	ddim = dy;
    }
    other = !dim;

    t = tmin;
    dt = (tmax-tmin)/ddim;
    for ( t=tmin; t<=tmax; t+= dt ) {
	if ( t>tmax-dt/8. ) t = tmax;		/* Avoid rounding errors */
	d = ((ps->splines[dim].a*t+ps->splines[dim].b)*t+ps->splines[dim].c)*t+ps->splines[dim].d;
	o = ((ps->splines[other].a*t+ps->splines[other].b)*t+ps->splines[other].c)*t+ps->splines[other].d;
	if ( ttf->splines[dim].b == 0 ) {
	    ttf_t = (d-ttf->splines[dim].d)/ttf->splines[dim].c;
	} else {
	    sq = ttf->splines[dim].c*ttf->splines[dim].c -
		    4*ttf->splines[dim].b*(ttf->splines[dim].d-d);
	    if ( sq<0 )
return( false );
	    sq = sqrt(sq);
	    ttf_t = (-ttf->splines[dim].c-sq)/(2*ttf->splines[dim].b);
	    if ( ttf_t>=-0.1 && ttf_t<=1.1 ) {		/* Optimizer gives us rounding errors */
				    /* And tmin/tmax are no longer exact */
		val = (ttf->splines[other].b*ttf_t+ttf->splines[other].c)*ttf_t+
			    ttf->splines[other].d;
		if ( val>o-err && val<o+err )
    continue;
	    }
	    ttf_t = (-ttf->splines[dim].c+sq)/(2*ttf->splines[dim].b);
	}
	if ( ttf_t>=-0.1 && ttf_t<=1.1 ) {
	    val = (ttf->splines[other].b*ttf_t+ttf->splines[other].c)*ttf_t+
			ttf->splines[other].d;
	    if ( val>o-err && val<o+err )
    continue;
	}
return( false );
    }

    /* Are representative points on ttf near points on ps? */
    for ( t=.125; t<1; t+= .125 ) {
	d = (ttf->splines[dim].b*t+ttf->splines[dim].c)*t+ttf->splines[dim].d;
	o = (ttf->splines[other].b*t+ttf->splines[other].c)*t+ttf->splines[other].d;
	CubicSolve(&ps->splines[dim],d,ts);
	for ( i=0; i<3; ++i ) if ( ts[i]!=-1 ) {
	    val = ((ps->splines[other].a*ts[i]+ps->splines[other].b)*ts[i]+ps->splines[other].c)*ts[i]+ps->splines[other].d;
	    if ( val>o-err && val<o+err )
	break;
	}
	if ( i==3 )
return( false );
    }

return( true );
}

static SplinePoint *MakeQuadSpline(SplinePoint *start,Spline *ttf,real x,
	real y, real tmax,SplinePoint *oldend) {
    Spline *new = chunkalloc(sizeof(Spline));
    SplinePoint *end = chunkalloc(sizeof(SplinePoint));

    if ( tmax==1 ) {
	end->roundx = oldend->roundx; end->roundy = oldend->roundy; end->dontinterpolate = oldend->dontinterpolate;
	x = oldend->me.x; y = oldend->me.y;	/* Want it to compare exactly */
    }
    end->ttfindex = 0xfffe;
    end->nextcpindex = 0xfffe;
    end->me.x = end->nextcp.x = x;
    end->me.y = end->nextcp.y = y;
    end->nonextcp = true;

    *new = *ttf;
    new->from = start;		start->next = new;
    new->to = end;		end->prev = new;
    if ( new->splines[0].b==0 && new->splines[1].b==0 ) {
	end->noprevcp = true;
	end->prevcp.x = x; end->prevcp.y = y;
	new->islinear = new->knownlinear = true;
    } else {
	end->prevcp.x = start->nextcp.x = ttf->splines[0].c/2+ttf->splines[0].d;
	end->prevcp.y = start->nextcp.y = ttf->splines[1].c/2+ttf->splines[1].d;
	start->nonextcp = end->noprevcp = false;
	new->isquadratic = true;
    }
    new->order2 = true;
return( end );
}

static int buildtestquads(Spline *ttf,real xmin,real ymin,real cx,real cy,
	real x,real y,real tmin,real t,real err,Spline *ps, DBounds *psbb) {
    real fudge, normal, para;
    BasePoint segdir, cpdir;

    /* test the control points are reasonable */
    fudge = (psbb->maxx-psbb->minx) + (psbb->maxy-psbb->miny);
    if ( cx<psbb->minx-fudge || cx>psbb->maxx+fudge )
return( false );
    if ( cy<psbb->miny-fudge || cy>psbb->maxy+fudge )
return( false );

    segdir.x = x-xmin; segdir.y = y-ymin;
    cpdir.x = cx-xmin; cpdir.y = cy-ymin;
    para = segdir.x*cpdir.x + segdir.y*cpdir.y;
    if ( (normal = segdir.x*cpdir.y - segdir.y*cpdir.x)<0 )
	normal=-normal;
    if ( para<0 && -para >4*normal )
return( false );
    cpdir.x = x-cx; cpdir.y = y-cy;
    para = segdir.x*cpdir.x + segdir.y*cpdir.y;
    if ( (normal = segdir.x*cpdir.y - segdir.y*cpdir.x)<0 )
	normal=-normal;
    if ( para<0 && -para >4*normal )
return( false );

    ttf->splines[0].d = xmin;
    ttf->splines[0].c = 2*(cx-xmin);
    ttf->splines[0].b = xmin+x-2*cx;
    ttf->splines[1].d = ymin;
    ttf->splines[1].c = 2*(cy-ymin);
    ttf->splines[1].b = ymin+y-2*cy;
    if ( comparespline(ps,ttf,tmin,t,err) )
return( true );

return( false );
}

static SplinePoint *LinearSpline(Spline *ps,SplinePoint *start, real tmax) {
    real x,y;
    Spline *new = chunkalloc(sizeof(Spline));
    SplinePoint *end = chunkalloc(sizeof(SplinePoint));

    x = ((ps->splines[0].a*tmax+ps->splines[0].b)*tmax+ps->splines[0].c)*tmax+ps->splines[0].d;
    y = ((ps->splines[1].a*tmax+ps->splines[1].b)*tmax+ps->splines[1].c)*tmax+ps->splines[1].d;
    if ( tmax==1 ) {
	SplinePoint *oldend = ps->to;
	end->roundx = oldend->roundx; end->roundy = oldend->roundy; end->dontinterpolate = oldend->dontinterpolate;
	x = oldend->me.x; y = oldend->me.y;	/* Want it to compare exactly */
    }
    end->ttfindex = 0xfffe;
    end->nextcpindex = 0xfffe;
    end->me.x = end->nextcp.x = end->prevcp.x = x;
    end->me.y = end->nextcp.y = end->prevcp.y = y;
    end->nonextcp = end->noprevcp = start->nonextcp = true;
    new->from = start;		start->next = new;
    new->to = end;		end->prev = new;
    new->splines[0].d = start->me.x;
    new->splines[0].c = (x-start->me.x);
    new->splines[1].d = start->me.y;
    new->splines[1].c = (y-start->me.y);
    new->order2 = true;
    new->islinear = new->knownlinear = true;
return( end );
}

static SplinePoint *_ttfapprox(Spline *ps,real tmin, real tmax, SplinePoint *start) {
    int dim=0;
    real dx, dy, ddim, dt, t, err;
    real x,y, xmin, ymin;
    real dxdtmin, dydtmin, dxdt, dydt;
    SplinePoint *sp;
    real cx, cy;
    Spline ttf;
    int cnt = -1, forceit;
    BasePoint end, rend, dend;
    DBounds bb;

    rend.x = ((ps->splines[0].a*tmax+ps->splines[0].b)*tmax+ps->splines[0].c)*tmax + ps->splines[0].d;
    rend.y = ((ps->splines[1].a*tmax+ps->splines[1].b)*tmax+ps->splines[1].c)*tmax + ps->splines[1].d;
    end.x = rint( rend.x );
    end.y = rint( rend.y );
    dend.x = (3*ps->splines[0].a*tmax+2*ps->splines[0].b)*tmax+ps->splines[0].c;
    dend.y = (3*ps->splines[1].a*tmax+2*ps->splines[1].b)*tmax+ps->splines[1].c;
    memset(&ttf,'\0',sizeof(ttf));

    bb.minx = bb.maxx = ps->from->me.x;
    if ( ps->from->nextcp.x > bb.maxx ) bb.maxx = ps->from->nextcp.x;
    else if ( ps->from->nextcp.x < bb.minx ) bb.minx = ps->from->nextcp.x;
    if ( ps->to->prevcp.x > bb.maxx ) bb.maxx = ps->to->prevcp.x;
    else if ( ps->to->prevcp.x < bb.minx ) bb.minx = ps->to->prevcp.x;
    if ( ps->to->me.x > bb.maxx ) bb.maxx = ps->to->me.x;
    else if ( ps->to->me.x < bb.minx ) bb.minx = ps->to->me.x;
    bb.miny = bb.maxy = ps->from->me.y;
    if ( ps->from->nextcp.y > bb.maxy ) bb.maxy = ps->from->nextcp.y;
    else if ( ps->from->nextcp.y < bb.miny ) bb.miny = ps->from->nextcp.y;
    if ( ps->to->prevcp.y > bb.maxy ) bb.maxy = ps->to->prevcp.y;
    else if ( ps->to->prevcp.y < bb.miny ) bb.miny = ps->to->prevcp.y;
    if ( ps->to->me.y > bb.maxy ) bb.maxy = ps->to->me.y;
    else if ( ps->to->me.y < bb.miny ) bb.miny = ps->to->me.y;

  tail_recursion:
    ++cnt;

    xmin = start->me.x;
    ymin = start->me.y;
    dxdtmin = (3*ps->splines[0].a*tmin+2*ps->splines[0].b)*tmin + ps->splines[0].c;
    dydtmin = (3*ps->splines[1].a*tmin+2*ps->splines[1].b)*tmin + ps->splines[1].c;

    dx = ((ps->splines[0].a*tmax+ps->splines[0].b)*tmax+ps->splines[0].c)*tmax -
	 ((ps->splines[0].a*tmin+ps->splines[0].b)*tmin+ps->splines[0].c)*tmin ;
    dy = ((ps->splines[1].a*tmax+ps->splines[1].b)*tmax+ps->splines[1].c)*tmax -
	 ((ps->splines[1].a*tmin+ps->splines[1].b)*tmin+ps->splines[1].c)*tmin ;
    if ( dx<0 ) dx = -dx;
    if ( dy<0 ) dy = -dy;
    if ( dx>dy ) {
	dim = 0;
	ddim = dx;
    } else {
	dim = 1;
	ddim = dy;
    }
    if (( err = ddim/3000 )<1 ) err = 1;

    if ( ddim<2 ||
	    (dend.x==0 && rint(start->me.x)==end.x && dy<=10 && cnt!=0) ||
	    (dend.y==0 && rint(start->me.y)==end.y && dx<=10 && cnt!=0) ) {
	if ( cnt==0 || start->noprevcp )
return( LinearSpline(ps,start,tmax));
	/* If the end point is very close to where we want to be, then just */
	/*  pretend it's right */
	start->prev->splines[0].b += ps->to->me.x-start->me.x;
	start->prev->splines[1].b += ps->to->me.y-start->me.y;
	start->prevcp.x += rend.x-start->me.x;
	start->prevcp.y += rend.y-start->me.y;
	if ( start->prev!=NULL && !start->prev->from->nonextcp )
	    start->prev->from->nextcp = start->prevcp;
	start->me = rend;
return( start );
    }

    dt = (tmax-tmin)/ddim;
    forceit = false;
/* force_end: */
    for ( t=tmax; t>tmin+dt/128; t-= dt ) {		/* dt/128 is a hack to avoid rounding errors */
	x = ((ps->splines[0].a*t+ps->splines[0].b)*t+ps->splines[0].c)*t+ps->splines[0].d;
	y = ((ps->splines[1].a*t+ps->splines[1].b)*t+ps->splines[1].c)*t+ps->splines[1].d;
	dxdt = (3*ps->splines[0].a*t+2*ps->splines[0].b)*t + ps->splines[0].c;
	dydt = (3*ps->splines[1].a*t+2*ps->splines[1].b)*t + ps->splines[1].c;
	/* if the slopes are parallel at the ends there can be no bezier quadratic */
	/*  (control point is where the splines intersect. But if they are */
	/*  parallel and colinear then there is a line between 'em */
	if ( ( dxdtmin==0 && dxdt==0 ) || (dydtmin==0 && dydt==0) ||
		( dxdt!=0 && dxdtmin!=0 &&
		    RealNearish(dydt/dxdt,dydtmin/dxdtmin)) )
    continue;

	if ( dxdt==0 )
	    cx=x;
	else if ( dxdtmin==0 )
	    cx=xmin;
	else
	    cx = -(ymin-(dydtmin/dxdtmin)*xmin-y+(dydt/dxdt)*x)/(dydtmin/dxdtmin-dydt/dxdt);
	if ( dydt==0 )
	    cy=y;
	else if ( dydtmin==0 )
	    cy=ymin;
	else
	    cy = -(xmin-(dxdtmin/dydtmin)*ymin-x+(dxdt/dydt)*y)/(dxdtmin/dydtmin-dxdt/dydt);
	/* Make the quadratic spline from (xmin,ymin) through (cx,cy) to (x,y)*/
	if ( forceit || buildtestquads(&ttf,xmin,ymin,cx,cy,x,y,tmin,t,err,ps,&bb)) {
	    sp = MakeQuadSpline(start,&ttf,x,y,t,ps->to);
	    forceit = false;
	    if ( t==tmax )
return( sp );
	    tmin = t;
	    start = sp;
  goto tail_recursion;
	}
	ttf.splines[0].d = xmin;
	ttf.splines[0].c = x-xmin;
	ttf.splines[0].b = 0;
	ttf.splines[1].d = ymin;
	ttf.splines[1].c = y-ymin;
	ttf.splines[1].b = 0;
	if ( comparespline(ps,&ttf,tmin,t,err) ) {
	    sp = LinearSpline(ps,start,t);
	    if ( t==tmax )
return( sp );
	    tmin = t;
	    start = sp;
  goto tail_recursion;
	}
    }
    tmin += dt;
    start = LinearSpline(ps,start,tmin);
  goto tail_recursion;
}

static SplinePoint *__ttfApprox(Spline *ps,real tmin, real tmax, SplinePoint *start) {
    extended inflect[2];
    int i=0;
    SplinePoint *end;
    Spline *s, *next;

    end = _ttfapprox(ps,tmin,tmax,start);
    if ( ps->knownlinear )
return( end );
    for ( s=start->next; s!=NULL && !s->islinear; s=s->to->next );
    if ( s==NULL )
return( end );
    for ( s=start->next; s!=NULL ; s=next ) {
	next = s->to->next;
	SplinePointFree(s->to);
	SplineFree(s);
    }
/* Hmm. With my algorithem, checking for points of inflection actually makes */
/*  things worse. It uses more points and the splines don't join as nicely */
/* However if we get a bad match (a line) in the normal approx, then check */
/*  Err... I was computing POI incorrectly. Above statement might not be correct*/
    /* no points of inflection in quad splines */

    i = Spline2DFindPointsOfInflection(ps, inflect);
    if ( i==2 ) {
	if ( RealNearish(inflect[0],inflect[1]) )
	    --i;
	else if ( inflect[0]>inflect[1] ) {
	    real temp = inflect[0];
	    inflect[0] = inflect[1];
	    inflect[1] = temp;
	}
    }
    if ( i!=0 ) {
	start = _ttfapprox(ps,tmin,inflect[0],start);
	tmin = inflect[0];
	if ( i==2 ) {
	    start = _ttfapprox(ps,tmin,inflect[1],start);
	    tmin = inflect[1];
	}
    }
return( _ttfapprox(ps,tmin,tmax,start));
}

#if !defined(FONTFORGE_CONFIG_NON_SYMMETRIC_QUADRATIC_CONVERSION)
typedef struct qpoint {
    BasePoint bp;
    BasePoint cp;
    bigreal t;
} QPoint;

static int comparedata(Spline *ps,QPoint *data,int qfirst,int qlast,
	int round_to_int, int test_level ) {
    Spline ttf;
    int i;
    bigreal err = round_to_int ? 1.5 : 1;

    if ( qfirst==qlast )		/* happened (was a bug) */
return( false );

    err *= (test_level+1);

    /* Control points diametrically opposed */
    if ( (data[qlast-2].cp.x-ps->to->me.x)*(ps->to->prevcp.x-ps->to->me.x) +
	    (data[qlast-2].cp.y-ps->to->me.y)*(ps->to->prevcp.y-ps->to->me.y)<0 )
return( false );
    if ( (data[qfirst-1].cp.x-ps->from->me.x)*(ps->from->nextcp.x-ps->from->me.x) +
	    (data[qfirst-1].cp.y-ps->from->me.y)*(ps->from->nextcp.y-ps->from->me.y)<0 )
return( false );

    memset(&ttf,0,sizeof(ttf));
    for ( i=qfirst; i<qlast; ++i ) {
	ttf.splines[0].d = data[i-1].bp.x;
	ttf.splines[0].c = 2*(data[i-1].cp.x-data[i-1].bp.x);
	ttf.splines[0].b = data[i-1].bp.x+data[i].bp.x-2*data[i-1].cp.x;
	ttf.splines[1].d = data[i-1].bp.y;
	ttf.splines[1].c = 2*(data[i-1].cp.y-data[i-1].bp.y);
	ttf.splines[1].b = data[i-1].bp.y+data[i].bp.y-2*data[i-1].cp.y;
	if ( !comparespline(ps,&ttf,data[i-1].t,data[i].t,err) )
return( false );
    }
return( true );
}

static SplinePoint *CvtDataToSplines(QPoint *data,int qfirst,int qlast,SplinePoint *start) {
    SplinePoint *end;
    int i;

    for ( i=qfirst; i<qlast; ++i ) {
	end = SplinePointCreate(data[i].bp.x,data[i].bp.y);
	start->nextcp = end->prevcp = data[i-1].cp;
	start->nonextcp = end->noprevcp = false;
	if (( data[i-1].cp.x == data[i].bp.x && data[i-1].cp.y == data[i].bp.y ) ||
		( data[i-1].cp.x == start->me.x && data[i-1].cp.y == start->me.y ))
	    start->nonextcp = end->noprevcp = true;
	SplineMake2(start,end);
	start = end;
    }
return( start );
}

static int SplineWithWellBehavedControlPoints(Spline *ps) {
    BasePoint splineunit;
    bigreal splinelen, npos, ppos;

    splineunit.x = ps->to->me.x - ps->from->me.x;
    splineunit.y = ps->to->me.y - ps->from->me.y;
    splinelen = sqrt(splineunit.x*splineunit.x + splineunit.y*splineunit.y);
    if ( splinelen!=0 ) {
	splineunit.x /= splinelen;
	splineunit.y /= splinelen;
    }

    npos = (ps->from->nextcp.x-ps->from->me.x) * splineunit.x +
	    (ps->from->nextcp.y-ps->from->me.y) * splineunit.y;
    ppos = (ps->to->prevcp.x-ps->from->me.x) * splineunit.x +
	    (ps->to->prevcp.y-ps->from->me.y) * splineunit.y;
return( npos>=0 && /* npos<=ppos &&*/ ppos<=splinelen );
}

static int PrettyApprox(Spline *ps,bigreal tmin, bigreal tmax,
	QPoint *data, int qcnt, int round_to_int, int test_level ) {
    int ptcnt, q, i;
    bigreal distance, dx, dy, tstart;
    BasePoint end, mid, slopemin, slopemid, slopeend;
    BasePoint splineunit, start;
    bigreal splinelen, midpos, lastpos, lastpos2, cppos;
    int do_good_spline_check;
    QPoint data2[12];

    if ( qcnt==-1 )
return( -1 );

    slopemin.x = (3*ps->splines[0].a*tmin+2*ps->splines[0].b)*tmin+ps->splines[0].c;
    slopemin.y = (3*ps->splines[1].a*tmin+2*ps->splines[1].b)*tmin+ps->splines[1].c;
    if ( slopemin.x==0 && slopemin.y==0 ) {
	bigreal t = tmin + (tmax-tmin)/256;
	/* If there is no control point for this end point, then the slope is */
	/*  0/0 at the end point. Which isn't useful, it leads to a quadratic */
	/*  control point at the end point, but this one is real because it   */
	/*  is used to interpolate the next point, but we get all confused    */
	/*  because we don't expect a real cp to be on the base point.        */
	slopemin.x = (3*ps->splines[0].a*t+2*ps->splines[0].b)*t+ps->splines[0].c;
	slopemin.y = (3*ps->splines[1].a*t+2*ps->splines[1].b)*t+ps->splines[1].c;
    }

    end.x = ((ps->splines[0].a*tmax+ps->splines[0].b)*tmax+ps->splines[0].c)*tmax+ps->splines[0].d;
    end.y = ((ps->splines[1].a*tmax+ps->splines[1].b)*tmax+ps->splines[1].c)*tmax+ps->splines[1].d;
    slopeend.x = (3*ps->splines[0].a*tmax+2*ps->splines[0].b)*tmax+ps->splines[0].c;
    slopeend.y = (3*ps->splines[1].a*tmax+2*ps->splines[1].b)*tmax+ps->splines[1].c;
    if ( slopemin.x==0 && slopemin.y==0 ) {
	bigreal t = tmax - (tmax-tmin)/256;
	/* Same problem as above, except at the other end */
	slopeend.x = (3*ps->splines[0].a*t+2*ps->splines[0].b)*t+ps->splines[0].c;
	slopeend.y = (3*ps->splines[1].a*t+2*ps->splines[1].b)*t+ps->splines[1].c;
    }

    start.x = data[qcnt-1].bp.x;
    start.y = data[qcnt-1].bp.y;
    splineunit.x = end.x - start.x;
    splineunit.y = end.y - start.y;
    splinelen = sqrt(splineunit.x*splineunit.x + splineunit.y*splineunit.y);
    if ( splinelen!=0 ) {
	splineunit.x /= splinelen;
	splineunit.y /= splinelen;
    }
    do_good_spline_check = SplineWithWellBehavedControlPoints(ps);

    if ( round_to_int && tmax!=1 ) {
	end.x = rint( end.x );
	end.y = rint( end.y );
    }

    dx = end.x-data[qcnt-1].bp.x; dy = end.y-data[qcnt-1].bp.y;
    distance = dx*dx + dy*dy;

    if ( distance<.3 ) {
	/* This is meaningless in truetype, use a line */
	data[qcnt-1].cp = data[qcnt-1].bp;
	data[qcnt].bp = end;
	data[qcnt].t = 1;
return( qcnt+1 );
    }

    for ( ptcnt=0; ptcnt<10; ++ptcnt ) {
	if ( ptcnt>1 && distance/(ptcnt*ptcnt)<16 )
return( -1 );			/* Points too close for a good approx */
	q = qcnt;
	data2[ptcnt+1].bp = end;
	lastpos=0; lastpos2 = splinelen;
	for ( i=0; i<=ptcnt; ++i ) {
	    tstart = (tmin*(ptcnt-i) + tmax*(i+1))/(ptcnt+1);
	    mid.x = ((ps->splines[0].a*tstart+ps->splines[0].b)*tstart+ps->splines[0].c)*tstart+ps->splines[0].d;
	    mid.y = ((ps->splines[1].a*tstart+ps->splines[1].b)*tstart+ps->splines[1].c)*tstart+ps->splines[1].d;
	    if ( i==0 ) {
		slopemid.x = (3*ps->splines[0].a*tstart+2*ps->splines[0].b)*tstart+ps->splines[0].c;
		slopemid.y = (3*ps->splines[1].a*tstart+2*ps->splines[1].b)*tstart+ps->splines[1].c;
		if ( slopemid.x==0 )
		    data[q-1].cp.x=mid.x;
		else if ( slopemin.x==0 )
		    data[q-1].cp.x=data[q-1].bp.x;
		else if ( RealNear(slopemin.y/slopemin.x,slopemid.y/slopemid.x) )
	break;
		else
		    data[q-1].cp.x = -(data[q-1].bp.y-(slopemin.y/slopemin.x)*data[q-1].bp.x-mid.y+(slopemid.y/slopemid.x)*mid.x)/(slopemin.y/slopemin.x-slopemid.y/slopemid.x);
		if ( slopemid.y==0 )
		    data[q-1].cp.y=mid.y;
		else if ( slopemin.y==0 )
		    data[q-1].cp.y=data[q-1].bp.y;
		else if ( RealNear(slopemin.x/slopemin.y,slopemid.x/slopemid.y) )
	break;
		else
		    data[q-1].cp.y = -(data[q-1].bp.x-(slopemin.x/slopemin.y)*data[q-1].bp.y-mid.x+(slopemid.x/slopemid.y)*mid.y)/(slopemin.x/slopemin.y-slopemid.x/slopemid.y);
	    } else {
		data[q-1].cp.x = 2*data[q-1].bp.x - data[q-2].cp.x;
		data[q-1].cp.y = 2*data[q-1].bp.y - data[q-2].cp.y;
	    }

	    midpos = (mid.x-start.x)*splineunit.x + (mid.y-start.y)*splineunit.y;
	    cppos  = (data[q-1].cp.x-start.x)*splineunit.x + (data[q-1].cp.y-start.y)*splineunit.y;

 	    if ( ((do_good_spline_check || i!=0 ) &&  cppos<lastpos) || cppos>midpos ) {
		i = 0;		/* Means we failed */
	break;
	    }
	    lastpos = midpos;

	    data[q].bp = mid;
	    data[q++].t = tstart;

	    tstart = (tmax*(ptcnt-i) + tmin*(i+1))/(ptcnt+1);
	    mid.x = ((ps->splines[0].a*tstart+ps->splines[0].b)*tstart+ps->splines[0].c)*tstart+ps->splines[0].d;
	    mid.y = ((ps->splines[1].a*tstart+ps->splines[1].b)*tstart+ps->splines[1].c)*tstart+ps->splines[1].d;
	    if ( i==0 ) {
		slopemid.x = (3*ps->splines[0].a*tstart+2*ps->splines[0].b)*tstart+ps->splines[0].c;
		slopemid.y = (3*ps->splines[1].a*tstart+2*ps->splines[1].b)*tstart+ps->splines[1].c;
		if ( slopemid.x==0 )
		    data2[ptcnt-i].cp.x=mid.x;
		else if ( slopeend.x==0 )
		    data2[ptcnt-i].cp.x=data2[ptcnt-i+1].bp.x;
		else if ( RealNear(slopeend.y/slopeend.x,slopemid.y/slopemid.x) )
	break;
		else
		    data2[ptcnt-i].cp.x = -(data2[ptcnt-i+1].bp.y-(slopeend.y/slopeend.x)*data2[ptcnt-i+1].bp.x-mid.y+(slopemid.y/slopemid.x)*mid.x)/(slopeend.y/slopeend.x-slopemid.y/slopemid.x);
		if ( slopemid.y==0 )
		    data2[ptcnt-i].cp.y=mid.y;
		else if ( slopeend.y==0 )
		    data2[ptcnt-i].cp.y=data2[ptcnt-i+1].bp.y;
		else if ( RealNear(slopeend.x/slopeend.y,slopemid.x/slopemid.y) )
	break;
		else
		    data2[ptcnt-i].cp.y = -(data2[ptcnt-i+1].bp.x-(slopeend.x/slopeend.y)*data2[ptcnt-i+1].bp.y-mid.x+(slopemid.x/slopemid.y)*mid.y)/(slopeend.x/slopeend.y-slopemid.x/slopemid.y);
	    } else {
		data2[ptcnt-i].cp.x = 2*data2[ptcnt-i+1].bp.x - data2[ptcnt-i+1].cp.x;
		data2[ptcnt-i].cp.y = 2*data2[ptcnt-i+1].bp.y - data2[ptcnt-i+1].cp.y;
	    }
	    data2[ptcnt-i].bp = mid;

	    midpos = (mid.x-start.x)*splineunit.x + (mid.y-start.y)*splineunit.y;
	    cppos  = (data2[ptcnt-i].cp.x-start.x)*splineunit.x + (data2[ptcnt-i].cp.y-start.y)*splineunit.y;
	    if ( ((do_good_spline_check || i!=0 ) && cppos>lastpos2) || cppos<midpos ) {
		i = 0;		/* Means we failed */
	break;
	    }
	    lastpos2 = midpos;

	}
	if ( i==0 )
    continue;
	if ( (data2[ptcnt+1].bp.x-data2[ptcnt].bp.x)*(data2[ptcnt].cp.x-data2[ptcnt].bp.x)<0 ||
		(data2[ptcnt+1].bp.y-data2[ptcnt].bp.y)*(data2[ptcnt].cp.y-data2[ptcnt].bp.y)<0 ) {
	    /* data2 are bad ... don't use them */;
	} else if ( (data[qcnt-1].bp.x-data[qcnt].bp.x)*(data[qcnt-1].cp.x-data[qcnt].bp.x)<0 ||
		(data[qcnt-1].bp.y-data[qcnt].bp.y)*(data[qcnt-1].cp.y-data[qcnt].bp.y)<0 ) {
	    /* data are bad */;
	    for ( i=0; i<=ptcnt; ++i ) {
		data[qcnt+i-1].cp = data2[i].cp;
		data[qcnt+i-1].bp = data2[i].bp;
	    }
	} else {
	    for ( i=0; i<=ptcnt; ++i ) {
		if ( ptcnt!=0 ) {
		    data[qcnt+i-1].cp.x = (data[qcnt+i-1].cp.x*(ptcnt-i) + data2[i].cp.x*i)/ptcnt;
		    data[qcnt+i-1].cp.y = (data[qcnt+i-1].cp.y*(ptcnt-i) + data2[i].cp.y*i)/ptcnt;
		}
	    }
	}
	if ( round_to_int ) {
	    for ( i=0; i<=ptcnt; ++i ) {
		data[qcnt+i-1].cp.x = rint( data[qcnt+i-1].cp.x );
		data[qcnt+i-1].cp.y = rint( data[qcnt+i-1].cp.y );
	    }
	}
	for ( i=0; i<ptcnt; ++i ) {
	    data[qcnt+i].bp.x = (data[qcnt+i].cp.x + data[qcnt+i-1].cp.x)/2;
	    data[qcnt+i].bp.y = (data[qcnt+i].cp.y + data[qcnt+i-1].cp.y)/2;
	}
	if ( comparedata(ps,data,qcnt,q,round_to_int,test_level))
return( q );
    }
return( -1 );
}
#endif

static SplinePoint *AlreadyQuadraticCheck(Spline *ps, SplinePoint *start) {
    SplinePoint *sp;

    if ( (RealNearish(ps->splines[0].a,0) && RealNearish(ps->splines[1].a,0)) ||
	    ((ps->splines[0].b!=0 && RealNearish(ps->splines[0].a/ps->splines[0].b,0)) &&
	     (ps->splines[1].b!=0 && RealNearish(ps->splines[1].a/ps->splines[1].b,0))) ) {
	/* Already Quadratic, just need to find the control point */
	/* Or linear, in which case we don't need to do much of anything */
	Spline *spline;
	sp = chunkalloc(sizeof(SplinePoint));
	sp->me.x = ps->to->me.x; sp->me.y = ps->to->me.y;
	sp->roundx = ps->to->roundx; sp->roundy = ps->to->roundy; sp->dontinterpolate = ps->to->dontinterpolate;
	sp->ttfindex = 0xfffe;
	sp->nextcpindex = 0xfffe;
	sp->nonextcp = true;
	spline = chunkalloc(sizeof(Spline));
	spline->order2 = true;
	spline->from = start;
	spline->to = sp;
	spline->splines[0] = ps->splines[0]; spline->splines[1] = ps->splines[1];
	start->next = sp->prev = spline;
	if ( ps->knownlinear ) {
	    spline->islinear = spline->knownlinear = true;
	    start->nonextcp = sp->noprevcp = true;
	    start->nextcp = start->me;
	    sp->prevcp = sp->me;
	} else {
	    start->nonextcp = sp->noprevcp = false;
	    start->nextcp.x = sp->prevcp.x = (ps->splines[0].c+2*ps->splines[0].d)/2;
	    start->nextcp.y = sp->prevcp.y = (ps->splines[1].c+2*ps->splines[1].d)/2;
	}
return( sp );
    }
return( NULL );
}

static SplinePoint *ttfApprox(Spline *ps, SplinePoint *start) {
#if !defined(FONTFORGE_CONFIG_NON_SYMMETRIC_QUADRATIC_CONVERSION)
    extended magicpoints[6], last;
    int cnt, i, j, qcnt, test_level;
    QPoint data[8*10];
    int round_to_int =
    /* The end points are at integer points, or one coord is at half while */
    /*  the other is at an integer (ie. condition for ttf interpolated point)*/
	    ((ps->from->me.x==rint(ps->from->me.x) &&
	      ps->from->me.y==rint(ps->from->me.y)) ||
	     (ps->from->me.x==rint(ps->from->me.x) &&
	      ps->from->me.x==ps->from->nextcp.x &&
	      ps->from->me.y!=ps->from->nextcp.y &&
	      2*ps->from->me.y==rint(2*ps->from->me.y)) ||
	     (ps->from->me.y==rint(ps->from->me.y) &&
	      ps->from->me.y==ps->from->nextcp.y &&
	      ps->from->me.x!=ps->from->nextcp.x &&
	      2*ps->from->me.x==rint(2*ps->from->me.x)) ) &&
	    ((ps->to->me.x == rint(ps->to->me.x) &&
	      ps->to->me.y == rint(ps->to->me.y)) ||
	     (ps->to->me.x==rint(ps->to->me.x) &&
	      ps->to->me.x==ps->to->prevcp.x &&
	      ps->to->me.y!=ps->to->prevcp.y &&
	      2*ps->to->me.y==rint(2*ps->to->me.y)) ||
	     (ps->to->me.y==rint(ps->to->me.y) &&
	      ps->to->me.y==ps->to->prevcp.y &&
	      ps->to->me.x!=ps->to->prevcp.x &&
	      2*ps->to->me.x==rint(2*ps->to->me.x)) );
#endif
    SplinePoint *ret;
/* Divide the spline up at extrema and points of inflection. The first	*/
/*  because ttf splines should have points at their extrema, the second */
/*  because quadratic splines can't have points of inflection.		*/
/* Let's not do the first (extrema) AddExtrema does this better and we  */
/*  don't want unneeded extrema.					*/
/* And sometimes we don't want to look at the points of inflection either*/

    if (( ret = AlreadyQuadraticCheck(ps,start))!=NULL )
return( ret );

#if !defined(FONTFORGE_CONFIG_NON_SYMMETRIC_QUADRATIC_CONVERSION)
    qcnt = 1;
    data[0].bp = ps->from->me;
    data[0].t = 0;
    qcnt = PrettyApprox(ps,0,1,data,qcnt,round_to_int,0);
    if ( qcnt!=-1 )
return( CvtDataToSplines(data,1,qcnt,start));

    cnt = 0;
    /* cnt = Spline2DFindExtrema(ps,magicpoints);*/

    cnt += Spline2DFindPointsOfInflection(ps,magicpoints+cnt);

    /* remove points outside range */
    for ( i=0; i<cnt; ++i ) {
	if ( magicpoints[i]<=0 || magicpoints[i]>=1 ) {
	    for ( j=i+1; j<cnt; ++j )
		magicpoints[j-1] = magicpoints[j];
	    --cnt;
	    --i;
	}
    }
    /* sort points */
    for ( i=0; i<cnt; ++i ) for ( j=i+1; j<cnt; ++j ) {
	if ( magicpoints[i]>magicpoints[j] ) {
	    bigreal temp = magicpoints[i];
	    magicpoints[i] = magicpoints[j];
	    magicpoints[j] = temp;
	}
    }
    /* Remove duplicates */
    for ( i=1; i<cnt; ++i ) {
	while ( i<cnt && RealNear(magicpoints[i-1],magicpoints[i])) {
	    --cnt;
	    for ( j=i ; j<cnt; ++j )
		magicpoints[j] = magicpoints[j+1];
	    magicpoints[cnt] = -1;
	}
    }

    for ( test_level=0; test_level<3; ++test_level ) {
	qcnt = 1;
	last = 0;
	for ( i=0; i<cnt; ++i ) {
	    qcnt = PrettyApprox(ps,last,magicpoints[i],data,qcnt,round_to_int,test_level);
	    last = magicpoints[i];
	}
	qcnt = PrettyApprox(ps,last,1,data,qcnt,round_to_int,test_level);
	if ( qcnt!=-1 )
    return( CvtDataToSplines(data,1,qcnt,start));
    }
#endif

return( __ttfApprox(ps,0,1,start));
}

static void ttfCleanup(SplinePoint *from) {
    SplinePoint *test, *next;

    for ( test = from; test->next!=NULL; test = next ) {
	next = test->next->to;
	/* Too close together to be meaningful when output as ttf */
	if ( rint(test->me.x) == rint(next->me.x) &&
		rint(test->me.y) == rint(next->me.y) ) {
	    if ( next->next==NULL || next==from ) {
		if ( test==from )
    break;
		next->prevcp = test->prevcp;
		next->noprevcp = test->noprevcp;
		next->prev = test->prev;
		next->prev->to = next;
		SplineFree(test->next);
		SplinePointFree(test);
	    } else {
		test->nextcp = next->nextcp;
		test->nonextcp = next->nonextcp;
		test->next = next->next;
		test->next->from = test;
		SplineFree(next->prev);
		SplinePointFree(next);
		next = test->next->to;
	    }
	}
	if ( next==from )
    break;
    }
}

SplinePoint *SplineTtfApprox(Spline *ps) {
    SplinePoint *from;
    from = chunkalloc(sizeof(SplinePoint));
    *from = *ps->from;
    from->hintmask = NULL;
    ttfApprox(ps,from);
return( from );
}

SplineSet *SSttfApprox(SplineSet *ss) {
    SplineSet *ret = chunkalloc(sizeof(SplineSet));
    Spline *spline, *first;

    ret->first = chunkalloc(sizeof(SplinePoint));
    *ret->first = *ss->first;
    if ( ret->first->hintmask != NULL ) {
	ret->first->hintmask = chunkalloc(sizeof(HintMask));
	memcpy(ret->first->hintmask,ss->first->hintmask,sizeof(HintMask));
    }
    ret->last = ret->first;

    first = NULL;
    for ( spline=ss->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	ret->last = ttfApprox(spline,ret->last);
	ret->last->ptindex = spline->to->ptindex;
	ret->last->ttfindex = spline->to->ttfindex;
	ret->last->nextcpindex = spline->to->nextcpindex;
	if ( spline->to->hintmask != NULL ) {
	    ret->last->hintmask = chunkalloc(sizeof(HintMask));
	    memcpy(ret->last->hintmask,spline->to->hintmask,sizeof(HintMask));
	}
	if ( first==NULL ) first = spline;
    }
    if ( ss->first==ss->last ) {
	if ( ret->last!=ret->first ) {
	    ret->first->prevcp = ret->last->prevcp;
	    ret->first->noprevcp = ret->last->noprevcp;
	    ret->first->prev = ret->last->prev;
	    ret->last->prev->to = ret->first;
	    SplinePointFree(ret->last);
	    ret->last = ret->first;
	}
    }
    ttfCleanup(ret->first);
    SPLCategorizePoints(ret);
return( ret );
}

SplineSet *SplineSetsTTFApprox(SplineSet *ss) {
    SplineSet *head=NULL, *last, *cur;

    while ( ss!=NULL ) {
	cur = SSttfApprox(ss);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	ss = ss->next;
    }
return( head );
}

static void ImproveB3CPForQuadratic(real from,real *_ncp,real *_pcp,real to) {
    real ncp = *_ncp, pcp = *_pcp;
    real noff, poff;
    real c,b, best;
    int err, i, besti;
    real offs[9];

    if ( (noff=ncp/32768.0)<0 ) noff = -noff;
    if ( (poff=pcp/32768.0)<0 ) poff = -poff;
    if ( noff<1.0/32768.0 ) noff = 1.0/32768.0;
    if ( poff<1.0/32768.0 ) poff = 1.0/32768.0;

    c = 3*(ncp-from); b = 3*(pcp-ncp)-c; best = to-from-c-b;
    offs[4] = best;
    if ( best==0 )
return;

    for ( err=0; err<10; ++err, noff/=2.0, poff/=2.0 ) {
	c = 3*(ncp-noff-from); b = 3*(pcp-poff-(ncp-noff))-c; offs[0] = to-from-c-b;
	c = 3*(ncp-noff-from); b = 3*(pcp     -(ncp-noff))-c; offs[1] = to-from-c-b;
	c = 3*(ncp-noff-from); b = 3*(pcp+poff-(ncp-noff))-c; offs[2] = to-from-c-b;
	c = 3*(ncp     -from); b = 3*(pcp-poff-(ncp     ))-c; offs[3] = to-from-c-b;
	c = 3*(ncp     -from); b = 3*(pcp+poff-(ncp     ))-c; offs[5] = to-from-c-b;
	c = 3*(ncp+noff-from); b = 3*(pcp-poff-(ncp+noff))-c; offs[6] = to-from-c-b;
	c = 3*(ncp+noff-from); b = 3*(pcp     -(ncp+noff))-c; offs[7] = to-from-c-b;
	c = 3*(ncp+noff-from); b = 3*(pcp+poff-(ncp+noff))-c; offs[8] = to-from-c-b;
	besti=4;
	for ( i=0; i<9; ++i ) {
	    if ( offs[i]<0 ) offs[i]= - offs[i];
	    if ( offs[i]<best ) {
		besti = i;
		best = offs[i];
	    }
	}
	if ( besti!=4 ) {
	    if ( besti<3 ) ncp -= noff;
	    else if ( besti>=6 ) ncp += noff;
	    if ( besti%3==0 ) pcp -= poff;
	    else if ( besti%3==2 ) pcp += poff;
	    offs[4] = best;
	    if ( best==0 )
    break;
	}
    }
    *_ncp = ncp;
    *_pcp = pcp;
}
    
SplineSet *SSPSApprox(SplineSet *ss) {
    SplineSet *ret = chunkalloc(sizeof(SplineSet));
    Spline *spline, *first;
    SplinePoint *to;

    ret->first = chunkalloc(sizeof(SplinePoint));
    *ret->first = *ss->first;
    if ( ret->first->hintmask != NULL ) {
	ret->first->hintmask = chunkalloc(sizeof(HintMask));
	memcpy(ret->first->hintmask,ss->first->hintmask,sizeof(HintMask));
    }
    ret->last = ret->first;

    first = NULL;
    for ( spline=ss->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	to = chunkalloc(sizeof(SplinePoint));
	*to = *spline->to;
	if ( to->hintmask != NULL ) {
	    to->hintmask = chunkalloc(sizeof(HintMask));
	    memcpy(to->hintmask,spline->to->hintmask,sizeof(HintMask));
	}
	if ( !spline->knownlinear ) {
	    ret->last->nextcp.x = ret->last->me.x + 2*(ret->last->nextcp.x-ret->last->me.x)/3;
	    ret->last->nextcp.y = ret->last->me.y + 2*(ret->last->nextcp.y-ret->last->me.y)/3;
	    to->prevcp.x = to->me.x + 2*(to->prevcp.x-to->me.x)/3;
	    to->prevcp.y = to->me.y + 2*(to->prevcp.y-to->me.y)/3;
	    ImproveB3CPForQuadratic(ret->last->me.x,&ret->last->nextcp.x,&to->prevcp.x,to->me.x);
	    ImproveB3CPForQuadratic(ret->last->me.y,&ret->last->nextcp.y,&to->prevcp.y,to->me.y);
	}
	SplineMake3(ret->last,to);
	ret->last = to;
	if ( first==NULL ) first = spline;
    }
    if ( ss->first==ss->last ) {
	if ( ret->last!=ret->first ) {
	    ret->first->prevcp = ret->last->prevcp;
	    ret->first->noprevcp = ret->last->noprevcp;
	    ret->first->prev = ret->last->prev;
	    ret->last->prev->to = ret->first;
	    SplinePointFree(ret->last);
	    ret->last = ret->first;
	}
    }
    ret->is_clip_path = ss->is_clip_path;
return( ret );
}

SplineSet *SplineSetsPSApprox(SplineSet *ss) {
    SplineSet *head=NULL, *last, *cur;

    while ( ss!=NULL ) {
	cur = SSPSApprox(ss);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	ss = ss->next;
    }
return( head );
}

SplineSet *SplineSetsConvertOrder(SplineSet *ss, int to_order2) {
    SplineSet *new;

    if ( to_order2 )
	new = SplineSetsTTFApprox(ss);
    else
	new = SplineSetsPSApprox(ss);
    SplinePointListsFree(ss);
return( new );
}

void SCConvertLayerToOrder2(SplineChar *sc,int layer) {
    SplineSet *new;

    if ( sc==NULL )
return;

    new = SplineSetsTTFApprox(sc->layers[layer].splines);
    SplinePointListsFree(sc->layers[layer].splines);
    sc->layers[layer].splines = new;

    UndoesFree(sc->layers[layer].undoes);
    UndoesFree(sc->layers[layer].redoes);
    sc->layers[layer].undoes = NULL;
    sc->layers[layer].redoes = NULL;
    sc->layers[layer].order2 = true;

    MinimumDistancesFree(sc->md); sc->md = NULL;
}

void SCConvertToOrder2(SplineChar *sc) {
    int layer;

    if ( sc==NULL )
return;

    for ( layer=ly_back; layer<sc->layer_cnt; ++layer )
	SCConvertLayerToOrder2(sc,layer);
}

static void SCConvertRefs(SplineChar *sc,int layer) {
    RefChar *rf;

    sc->ticked = true;
    for ( rf=sc->layers[layer].refs; rf!=NULL; rf=rf->next ) {
	if ( !rf->sc->ticked )
	    SCConvertRefs(rf->sc,layer);
	SCReinstanciateRefChar(sc,rf,layer);	/* Conversion is done by reinstanciating */
		/* Since the base thing will have been converted, all we do is copy its data */
    }
}

void SFConvertLayerToOrder2(SplineFont *_sf,int layer) {
    int i, k;
    SplineFont *sf;

    if ( _sf->cidmaster!=NULL ) _sf=_sf->cidmaster;
    k = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    SCConvertLayerToOrder2(sf->glyphs[i],layer);
	    sf->glyphs[i]->ticked = false;
	    sf->glyphs[i]->changedsincelasthinted = false;
	}
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && !sf->glyphs[i]->ticked )
	    SCConvertRefs(sf->glyphs[i],layer);

	if ( layer!=ly_back )
	    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
		SCNumberPoints(sf->glyphs[i],layer);
	++k;
    } while ( k<_sf->subfontcnt );
    _sf->layers[layer].order2 = true;
}

void SFConvertGridToOrder2(SplineFont *_sf) {
    int k;
    SplineSet *new;
    SplineFont *sf;

    if ( _sf->cidmaster!=NULL ) _sf=_sf->cidmaster;
    k = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];

	new = SplineSetsTTFApprox(sf->grid.splines);
	SplinePointListsFree(sf->grid.splines);
	sf->grid.splines = new;

	UndoesFree(sf->grid.undoes); UndoesFree(sf->grid.redoes);
	sf->grid.undoes = sf->grid.redoes = NULL;
	sf->grid.order2 = true;
	++k;
    } while ( k<_sf->subfontcnt );
    _sf->grid.order2 = true;
}

void SFConvertToOrder2(SplineFont *_sf) {
    int layer;

    for ( layer=0; layer<_sf->layer_cnt; ++layer )
	SFConvertLayerToOrder2(_sf,layer);
    SFConvertGridToOrder2(_sf);
}

void SCConvertLayerToOrder3(SplineChar *sc,int layer) {
    SplineSet *new;
    RefChar *ref;
    AnchorPoint *ap;
    int has_order2_layer_still, i;

    new = SplineSetsPSApprox(sc->layers[layer].splines);
    SplinePointListsFree(sc->layers[layer].splines);
    sc->layers[layer].splines = new;

    UndoesFree(sc->layers[layer].undoes);
    UndoesFree(sc->layers[layer].redoes);
    sc->layers[layer].undoes = NULL;
    sc->layers[layer].redoes = NULL;
    sc->layers[layer].order2 = false;

    MinimumDistancesFree(sc->md); sc->md = NULL;

    /* OpenType/PostScript fonts don't support point matching to position */
    /*  references or anchors */
    for ( ref = sc->layers[layer].refs; ref!=NULL; ref=ref->next )
	ref->point_match = false;
    has_order2_layer_still = false;
    for ( i=ly_fore; i<sc->layer_cnt; ++i )
	if ( sc->layers[i].order2 ) {
	    has_order2_layer_still = true;
    break;
	}
    if ( !has_order2_layer_still ) {
	for ( ap = sc->anchor; ap!=NULL; ap=ap->next )
	    ap->has_ttf_pt = false;

	free(sc->ttf_instrs);
	sc->ttf_instrs = NULL; sc->ttf_instrs_len = 0;
	/* If this character has any cv's showing instructions then remove the instruction pane!!!!! */
    }
}

void SCConvertToOrder3(SplineChar *sc) {
    int layer;

    for ( layer=0; layer<sc->layer_cnt; ++layer )
	SCConvertLayerToOrder3(sc,layer);
}

void SCConvertOrder(SplineChar *sc, int to_order2) {
    if ( to_order2 )
	SCConvertToOrder2(sc);
    else
	SCConvertToOrder3(sc);
}

void SFConvertLayerToOrder3(SplineFont *_sf,int layer) {
    int i, k;
    SplineFont *sf;

    if ( _sf->cidmaster!=NULL ) _sf=_sf->cidmaster;
    k = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    SCConvertLayerToOrder3(sf->glyphs[i],layer);
	    sf->glyphs[i]->ticked = false;
	    sf->glyphs[i]->changedsincelasthinted = true;
	}
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && !sf->glyphs[i]->ticked )
	    SCConvertRefs(sf->glyphs[i],layer);

	sf->layers[layer].order2 = false;
	++k;
    } while ( k<_sf->subfontcnt );
    _sf->layers[layer].order2 = false;
}

void SFConvertGridToOrder3(SplineFont *_sf) {
    int k;
    SplineSet *new;
    SplineFont *sf;

    if ( _sf->cidmaster!=NULL ) _sf=_sf->cidmaster;
    k = 0;
    do {
	sf = _sf->subfonts==NULL ? _sf : _sf->subfonts[k];

	new = SplineSetsPSApprox(sf->grid.splines);
	SplinePointListsFree(sf->grid.splines);
	sf->grid.splines = new;

	UndoesFree(sf->grid.undoes); UndoesFree(sf->grid.redoes);
	sf->grid.undoes = sf->grid.redoes = NULL;
	sf->grid.order2 = false;
	++k;
    } while ( k<_sf->subfontcnt );
    _sf->grid.order2 = false;
}

void SFConvertToOrder3(SplineFont *_sf) {
    int layer;

    for ( layer=0; layer<_sf->layer_cnt; ++layer )
	SFConvertLayerToOrder3(_sf,layer);
    SFConvertGridToOrder3(_sf);
}

/* ************************************************************************** */

void SplineRefigure2(Spline *spline) {
    SplinePoint *from = spline->from, *to = spline->to;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];
    Spline old;

#ifdef DEBUG
    if ( RealNear(from->me.x,to->me.x) && RealNear(from->me.y,to->me.y))
	IError("Zero length spline created");
#endif
    if ( spline->acceptableextrema )
	old = *spline;

    if ( from->nonextcp || to->noprevcp ||
	    ( from->nextcp.x==from->me.x && from->nextcp.y == from->me.y && from->nextcpindex>=0xfffe ) ||
	    ( to->prevcp.x==to->me.x && to->prevcp.y == to->me.y && from->nextcpindex>=0xfffe )) {
	from->nonextcp = to->noprevcp = true;
	from->nextcp = from->me;
	to->prevcp = to->me;
    }

    if ( from->nonextcp && to->noprevcp )
	/* Ok */;
    else if ( from->nonextcp || to->noprevcp || from->nextcp.x!=to->prevcp.x ||
	    from->nextcp.y!=to->prevcp.y ) {
	if ( RealNear(from->nextcp.x,to->prevcp.x) &&
		RealNear(from->nextcp.y,to->prevcp.y)) {
	    from->nextcp.x = to->prevcp.x = (from->nextcp.x+to->prevcp.x)/2;
	    from->nextcp.y = to->prevcp.y = (from->nextcp.y+to->prevcp.y)/2;
	} else {
	    IError("Invalid 2nd order spline in SplineRefigure2" );
#ifndef GWW_TEST
	    /* I don't want these to go away when I'm debugging. I want to */
	    /*  know how I got them */
	    from->nextcp.x = to->prevcp.x = (from->nextcp.x+to->prevcp.x)/2;
	    from->nextcp.y = to->prevcp.y = (from->nextcp.y+to->prevcp.y)/2;
#endif
	}
    }

    xsp->d = from->me.x; ysp->d = from->me.y;
    if ( from->nonextcp && to->noprevcp ) {
	spline->islinear = true;
	xsp->c = to->me.x-from->me.x;
	ysp->c = to->me.y-from->me.y;
	xsp->a = xsp->b = 0;
	ysp->a = ysp->b = 0;
    } else {
	/* from p. 393 (Operator Details, curveto) PostScript Lang. Ref. Man. (Red book) */
	xsp->c = 2*(from->nextcp.x-from->me.x);
	ysp->c = 2*(from->nextcp.y-from->me.y);
	xsp->b = to->me.x-from->me.x-xsp->c;
	ysp->b = to->me.y-from->me.y-ysp->c;
	xsp->a = 0;
	ysp->a = 0;
	if ( RealNear(xsp->c,0)) xsp->c=0;
	if ( RealNear(ysp->c,0)) ysp->c=0;
	if ( RealNear(xsp->b,0)) xsp->b=0;
	if ( RealNear(ysp->b,0)) ysp->b=0;
	spline->islinear = false;
	if ( ysp->b==0 && xsp->b==0 )
	    spline->islinear = true;	/* This seems extremely unlikely... */
    }
    if ( isnan(ysp->b) || isnan(xsp->b) )
	IError("NaN value in spline creation");
    LinearApproxFree(spline->approx);
    spline->approx = NULL;
    spline->knowncurved = false;
    spline->knownlinear = spline->islinear;
    SplineIsLinear(spline);
    spline->isquadratic = !spline->knownlinear;
    spline->order2 = true;

    if ( spline->acceptableextrema ) {
	/* I don't check "d", because changes to that reflect simple */
	/*  translations which will not affect the shape of the spline */
	/* (I don't check "a" because it is always 0 in a quadratic spline) */
	if ( !RealNear(old.splines[0].b,spline->splines[0].b) ||
		!RealNear(old.splines[0].c,spline->splines[0].c) ||
		!RealNear(old.splines[1].b,spline->splines[1].b) ||
		!RealNear(old.splines[1].c,spline->splines[1].c) )
	    spline->acceptableextrema = false;
    }
}

void SplineRefigure(Spline *spline) {
    if ( spline==NULL )
return;
    if ( spline->order2 )
	SplineRefigure2(spline);
    else
	SplineRefigure3(spline);
}

static int IsHV(Spline *spline, int isfrom) {
    SplinePoint *sp;

    if ( spline==NULL )
return( false );

    if ( !isfrom ) {
	sp = spline->to;
	if ( sp->noprevcp )
return( false );
	if ( sp->me.x == sp->prevcp.x )
return( 2 );	/* Vertical */
	else if ( sp->me.y == sp->prevcp.y )
return( 1 );	/* Horizontal */
	else
return( 0 );	/* Neither */
    } else {
	sp = spline->from;
	if ( sp->nonextcp )
return( false );
	if ( sp->me.x == sp->nextcp.x )
return( 2 );	/* Vertical */
	else if ( sp->me.y == sp->nextcp.y )
return( 1 );	/* Horizontal */
	else
return( 0 );	/* Neither */
    }
}

void SplineRefigureFixup(Spline *spline) {
    SplinePoint *from, *to, *prev, *next;
    BasePoint foff, toff, unit, new;
    bigreal len;
    enum pointtype fpt, tpt;
    int done = false;
    extern int snaptoint;

    if ( !spline->order2 ) {
	SplineRefigure3(spline);
return;
    }
    from = spline->from; to = spline->to;
    if ( from->pointtype==pt_hvcurve && to->pointtype==pt_hvcurve ) {
	done = true;
	if ( !IsHV(from->prev,0) && !IsHV(to->next,1) ) {
	    if ( to->me.x == from->me.x ) {
		from->nextcp.x = to->prevcp.x = to->me.x;
		from->nextcp.y = to->prevcp.y = (from->me.y+from->me.y)/2;
	    } else if ( to->me.y==from->me.y ) {
		from->nextcp.y = to->prevcp.y = to->me.y;
		from->nextcp.x = to->prevcp.x = (from->me.x+from->me.x)/2;
	    /* Assume they are drawing clockwise */
	    } else if (( to->me.x>from->me.x && to->me.y>=from->me.y ) ||
			(to->me.x<from->me.x && to->me.y<=from->me.y )) {
		from->nextcp.x = to->prevcp.x = from->me.x;
		from->nextcp.y = to->prevcp.y = to->me.y;
	    } else {
		from->nextcp.x = to->prevcp.x = to->me.x;
		from->nextcp.y = to->prevcp.y = from->me.y;
	    }
	} else if ( !IsHV(to->next,1)) {
	    if ( IsHV(from->prev,0)==1 ) {
		from->nextcp.x = to->prevcp.x = to->me.x;
		from->nextcp.y = to->prevcp.y = from->me.y;
	    } else {
		from->nextcp.x = to->prevcp.x = from->me.x;
		from->nextcp.y = to->prevcp.y = to->me.y;
	    }
	} else if ( !IsHV(from->prev,0)) {
	    if ( IsHV(to->next,1)==1 ) {
		from->nextcp.x = to->prevcp.x = from->me.x;
		from->nextcp.y = to->prevcp.y = to->me.y;
	    } else {
		from->nextcp.x = to->prevcp.x = to->me.x;
		from->nextcp.y = to->prevcp.y = from->me.y;
	    }
	} else {
	    if ( IsHV(from->prev,0)==1 && IsHV(to->next,1)==2 ) {
		from->nextcp.x = to->prevcp.x = to->me.x;
		from->nextcp.y = to->prevcp.y = from->me.y;
	    } else if ( IsHV(from->prev,0)==2 && IsHV(to->next,1)==1 ) {
		from->nextcp.x = to->prevcp.x = from->me.x;
		from->nextcp.y = to->prevcp.y = to->me.y;
	    } else
		done = false;
	}
	if ( done )
	    to->noprevcp = from->nonextcp = false;
    }

  if ( !done ) {
    unit.x = from->nextcp.x-from->me.x;
    unit.y = from->nextcp.y-from->me.y;
    len = sqrt(unit.x*unit.x + unit.y*unit.y);
    if ( len!=0 ) {
        unit.x /= len;
        unit.y /= len;
    }

    if ( (fpt = from->pointtype)==pt_hvcurve ) fpt = pt_curve;
    if ( (tpt =   to->pointtype)==pt_hvcurve ) tpt = pt_curve;
    if ( from->nextcpdef && to->prevcpdef ) switch ( fpt*3+tpt ) {
      case pt_corner*3+pt_corner:
      case pt_corner*3+pt_tangent:
      case pt_tangent*3+pt_corner:
      case pt_tangent*3+pt_tangent:
	from->nonextcp = to->noprevcp = true;
	from->nextcp = from->me;
	to->prevcp = to->me;
      break;
      case pt_curve*3+pt_curve:
      case pt_curve*3+pt_corner:
      case pt_corner*3+pt_curve:
      case pt_tangent*3+pt_curve:
      case pt_curve*3+pt_tangent:
	if ( from->prev!=NULL && (from->pointtype==pt_tangent || from->pointtype==pt_hvcurve)) {
	    prev = from->prev->from;
	    foff.x = prev->me.x;
	    foff.y = prev->me.y;
	} else if ( from->prev!=NULL ) {
	    prev = from->prev->from;
	    foff.x = to->me.x-prev->me.x  + from->me.x;
	    foff.y = to->me.y-prev->me.y  + from->me.y;
	} else {
	    foff.x = from->me.x + (to->me.x-from->me.x)-(to->me.y-from->me.y);
	    foff.y = from->me.y + (to->me.x-from->me.x)+(to->me.y-from->me.y);
	    prev = NULL;
	}
	if ( to->next!=NULL && (to->pointtype==pt_tangent || to->pointtype==pt_hvcurve)) {
	    next = to->next->to;
	    toff.x = next->me.x;
	    toff.y = next->me.y;
	} else if ( to->next!=NULL ) {
	    next = to->next->to;
	    toff.x = next->me.x-from->me.x  + to->me.x;
	    toff.y = next->me.y-from->me.y  + to->me.y;
	} else {
	    toff.x = to->me.x + (to->me.x-from->me.x)+(to->me.y-from->me.y);
	    toff.y = to->me.y - (to->me.x-from->me.x)+(to->me.y-from->me.y);
	    next = NULL;
	}
	if (( from->pointtype==pt_hvcurve && foff.x!=from->me.x && foff.y!=from->me.y ) ||
		( to->pointtype==pt_hvcurve && toff.x!=to->me.x && toff.y!=to->me.y )) {
	    if ( from->me.x == to->me.x ) {
		if ( from->pointtype==pt_hvcurve )
		    foff.x = from->me.x;
		if ( to->pointtype==pt_hvcurve )
		    toff.x = to->me.x;
	    } else if ( from->me.y == to->me.y ) {
		if ( from->pointtype==pt_hvcurve )
		    foff.y = from->me.y;
		if ( to->pointtype==pt_hvcurve )
		    toff.y = to->me.y;
	    } else {
		if ( from->pointtype==pt_hvcurve && foff.x!=from->me.x && foff.y!=from->me.y ) {
		    if ( fabs(foff.x-from->me.x) > fabs(foff.y-from->me.y) )
			foff.y = from->me.y;
		    else
			foff.x = from->me.x;
		}
		if ( to->pointtype==pt_hvcurve && toff.x!=to->me.x && toff.y!=to->me.y ) {
		    if ( from->pointtype==pt_hvcurve ) {
			if ( from->me.x==foff.x )
			    toff.y = to->me.y;
			else
			    toff.x = to->me.x;
		    } else if ( fabs(toff.x-to->me.x) > fabs(toff.y-to->me.y) )
			toff.y = to->me.y;
		    else
			toff.x = to->me.x;
		}
	    }
	}
	if ( IntersectLinesClip(&from->nextcp,&foff,&from->me,&toff,&to->me)) {
	    from->nonextcp = to->noprevcp = false;
	    to->prevcp = from->nextcp;
	    if ( (from->pointtype==pt_curve || from->pointtype==pt_hvcurve ) &&
		    !from->noprevcp && from->prev!=NULL ) {
		prev = from->prev->from;
		if ( IntersectLinesClip(&from->prevcp,&from->nextcp,&from->me,&prev->nextcp,&prev->me)) {
		    prev->nextcp = from->prevcp;
		    SplineRefigure2(from->prev);
		}
	    }
	    if ( (to->pointtype==pt_curve || to->pointtype==pt_hvcurve) &&
		    !to->nonextcp && to->next!=NULL ) {
		next = to->next->to;
		if ( IntersectLinesClip(&to->nextcp,&to->prevcp,&to->me,&next->prevcp,&next->me)) {
		    next->prevcp = to->nextcp;
		    SplineRefigure(to->next);
		}
	    }
	}
      break;
    } else {
	/* Can't set things arbetrarily here, but make sure they are consistant */
	if ( (from->pointtype==pt_curve || from->pointtype==pt_hvcurve ) &&
		!from->noprevcp && !from->nonextcp ) {
	    unit.x = from->nextcp.x-from->me.x;
	    unit.y = from->nextcp.y-from->me.y;
	    len = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( len!=0 ) {
		unit.x /= len; unit.y /= len;
		len = sqrt((from->prevcp.x-from->me.x)*(from->prevcp.x-from->me.x) + (from->prevcp.y-from->me.y)*(from->prevcp.y-from->me.y));
		new.x = -len*unit.x + from->me.x; new.y = -len*unit.y + from->me.y;
		if ( new.x-from->prevcp.x<-1 || new.x-from->prevcp.x>1 ||
			 new.y-from->prevcp.y<-1 || new.y-from->prevcp.y>1 ) {
		    prev = NULL;
		    if ( from->prev!=NULL && (prev = from->prev->from)!=NULL &&
			    IntersectLinesClip(&from->prevcp,&new,&from->me,&prev->nextcp,&prev->me)) {
			prev->nextcp = from->prevcp;
			SplineRefigure2(from->prev);
		    } else {
			from->prevcp = new;
			if ( prev!=NULL )
			    prev->nextcp = new;
		    }
		}
	    }
	} else if ( from->pointtype==pt_tangent ) {
	    if ( from->prev!=NULL ) {
		prev = from->prev->from;
		if ( !from->noprevcp && !prev->nonextcp &&
			IntersectLinesClip(&from->prevcp,&to->me,&from->me,&prev->nextcp,&prev->me)) {
		    prev->nextcp = from->prevcp;
		    SplineRefigure2(from->prev);
		}
		if ( !from->nonextcp && !to->noprevcp &&
			IntersectLinesClip(&from->nextcp,&prev->me,&from->me,&to->prevcp,&to->me))
		    to->prevcp = from->nextcp;
	    }
	}
	if ( (to->pointtype==pt_curve || to->pointtype==pt_hvcurve ) &&
		!to->noprevcp && !to->nonextcp ) {
	    unit.x = to->prevcp.x-to->nextcp.x;
	    unit.y = to->prevcp.y-to->nextcp.y;
	    len = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( len!=0 ) {
		unit.x /= len; unit.y /= len;
		len = sqrt((to->nextcp.x-to->me.x)*(to->nextcp.x-to->me.x) + (to->nextcp.y-to->me.y)*(to->nextcp.y-to->me.y));
		new.x = -len*unit.x + to->me.x; new.y = -len*unit.y + to->me.y;
		if ( new.x-to->nextcp.x<-1 || new.x-to->nextcp.x>1 ||
			new.y-to->nextcp.y<-1 || new.y-to->nextcp.y>1 ) {
		    if ( to->next!=NULL && (next = to->next->to)!=NULL &&
			    IntersectLinesClip(&to->nextcp,&new,&to->me,&next->prevcp,&next->me)) {
			next->prevcp = to->nextcp;
			SplineRefigure2(to->next);
		    } else {
			to->nextcp = new;
			if ( to->next!=NULL ) {
			    to->next->to->prevcp = new;
			    SplineRefigure(to->next);
			}
		    }
		}
	    }
	} else if ( to->pointtype==pt_tangent ) {
	    if ( to->next!=NULL ) {
		next = to->next->to;
		if ( !to->nonextcp && !next->noprevcp &&
			IntersectLinesClip(&to->nextcp,&from->me,&to->me,&next->prevcp,&next->me)) {
		    next->prevcp = to->nextcp;
		    SplineRefigure2(to->next);
		}
		if ( !from->nonextcp && !to->noprevcp &&
			IntersectLinesClip(&from->nextcp,&next->me,&to->me,&from->nextcp,&from->me))
		    to->prevcp = from->nextcp;
	    }
	}
    }
    if ( from->nonextcp && to->noprevcp )
	/* Ok */;
    else if ( from->nonextcp || to->noprevcp ) {
	from->nonextcp = to->noprevcp = true;
    } else if (( from->nextcp.x==from->me.x && from->nextcp.y==from->me.y ) ||
		( to->prevcp.x==to->me.x && to->prevcp.y==to->me.y ) ) {
	from->nonextcp = to->noprevcp = true;
    } else if ( from->nonextcp || to->noprevcp || from->nextcp.x!=to->prevcp.x ||
	    from->nextcp.y!=to->prevcp.y ) {
	if ( !IntersectLinesClip(&from->nextcp,
		(from->pointtype==pt_tangent && from->prev!=NULL)?&from->prev->from->me:&from->nextcp, &from->me,
		(to->pointtype==pt_tangent && to->next!=NULL)?&to->next->to->me:&to->prevcp, &to->me)) {
	    from->nextcp.x = (from->me.x+to->me.x)/2;
	    from->nextcp.y = (from->me.y+to->me.y)/2;
	}
	to->prevcp = from->nextcp;
	if (( from->nextcp.x==from->me.x && from->nextcp.y==from->me.y ) ||
		( to->prevcp.x==to->me.x && to->prevcp.y==to->me.y ) ) {
	    from->nonextcp = to->noprevcp = true;
	    from->nextcp = from->me;
	    to->prevcp = to->me;
	}
    }
  }
    if ( snaptoint && !from->nonextcp ) {
	from->nextcp.x = to->prevcp.x = rint(from->nextcp.x);
	from->nextcp.y = to->prevcp.y = rint(from->nextcp.y);
    }
    SplineRefigure2(spline);

    /* Now in order2 splines it is possible to request combinations that are */
    /*  mathematically impossible -- two adjacent hv points often don't work */
    if ( to->pointtype==pt_hvcurve &&
		!(to->prevcp.x == to->me.x && to->prevcp.y != to->me.y ) &&
		!(to->prevcp.y == to->me.y && to->prevcp.x != to->me.x ) )
	to->pointtype = pt_curve;
    if ( from->pointtype==pt_hvcurve &&
		!(from->nextcp.x == from->me.x && from->nextcp.y != from->me.y ) &&
		!(from->nextcp.y == from->me.y && from->nextcp.x != from->me.x ) )
	from->pointtype = pt_curve;
}

Spline *SplineMake2(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    spline->order2 = true;
    SplineRefigure2(spline);
return( spline );
}

Spline *SplineMake(SplinePoint *from, SplinePoint *to, int order2) {
    if (order2 > 0)
return( SplineMake2(from,to));
    else
return( SplineMake3(from,to));
}
    
void SplinePointPrevCPChanged2(SplinePoint *sp) {
    SplinePoint *p, *pp;
    BasePoint p_pcp;

    if ( sp->prev!=NULL ) {
	p = sp->prev->from;
	if ( SPInterpolate(p) && !sp->noprevcp ) {
	    p->nextcp = sp->prevcp;
	    p->me.x = ( p->prevcp.x+p->nextcp.x)/2;
	    p->me.y = ( p->prevcp.y+p->nextcp.y)/2;
	    SplineRefigure2(sp->prev);
	    if (p->prev != NULL) SplineRefigure2(p->prev);
	} else {
	    p->nextcp = sp->prevcp;
	    p->nonextcp = sp->noprevcp;
	    if ( sp->noprevcp ) {
		p->nonextcp = true;
		p->nextcp = p->me;
		SplineRefigure2(sp->prev);
	    } else if (( p->pointtype==pt_curve || p->pointtype==pt_hvcurve ) &&
		    !p->noprevcp ) {
		SplineRefigure2(sp->prev);
		if ( p->prev==NULL ) {
		    bigreal len1, len2;
		    len1 = sqrt((p->nextcp.x-p->me.x)*(p->nextcp.x-p->me.x) +
				(p->nextcp.y-p->me.y)*(p->nextcp.y-p->me.y));
		    len2 = sqrt((p->prevcp.x-p->me.x)*(p->prevcp.x-p->me.x) +
				(p->prevcp.y-p->me.y)*(p->prevcp.y-p->me.y));
		    len2 /= len1;
		    p->prevcp.x = rint(len2 * (p->me.x-p->prevcp.x) + p->me.x);
		    p->prevcp.y = rint(len2 * (p->me.y-p->prevcp.y) + p->me.y);
		} else {
		    pp = p->prev->from;
		    /* Find the intersection (if any) of the lines between */
		    /*  pp->nextcp&pp->me with p->prevcp&p->me */
		    if ( IntersectLines(&p_pcp,&pp->nextcp,&pp->me,&p->nextcp,&p->me)) {
			bigreal len = (pp->me.x-p->me.x)*(pp->me.x-p->me.x) + (pp->me.y-p->me.y)*(pp->me.y-p->me.y);
			bigreal d1 = (p_pcp.x-p->me.x)*(pp->me.x-p->me.x) + (p_pcp.y-p->me.y)*(pp->me.y-p->me.y);
			bigreal d2 = (p_pcp.x-pp->me.x)*(p->me.x-pp->me.x) + (p_pcp.y-pp->me.y)*(p->me.y-pp->me.y);
			if ( d1>=0 && d1<=len && d2>=0 && d2<=len ) {
			    if ( rint(2*p->me.x)==2*p->me.x && rint(2*pp->me.x)==2*pp->me.x )
				p_pcp.x = rint( p_pcp.x );
			    if ( rint(2*p->me.y)==2*p->me.y && rint(2*pp->me.y)==2*pp->me.y )
				p_pcp.y = rint( p_pcp.y );
			    p->prevcp = pp->nextcp = p_pcp;
			    SplineRefigure2(p->prev);
			}
		    }
		}
	    }
	}
    }
}
    
void SplinePointNextCPChanged2(SplinePoint *sp) {
    SplinePoint *n, *nn;
    BasePoint n_ncp;

    if ( sp->next!=NULL ) {
	n = sp->next->to;
	if ( SPInterpolate(n) && !sp->nonextcp ) {
	    n->prevcp = sp->nextcp;
	    n->me.x = ( n->prevcp.x+n->nextcp.x)/2;
	    n->me.y = ( n->prevcp.y+n->nextcp.y)/2;
	    SplineRefigure2(sp->next);
	    if (n->next != NULL) SplineRefigure2(n->next);
	} else {
	    n->prevcp = sp->nextcp;
	    n->noprevcp = sp->nonextcp;
	    if ( sp->nonextcp ) {
		n->noprevcp = true;
		n->prevcp = n->me;
		SplineRefigure2(sp->next);
	    } else if (( n->pointtype==pt_curve || n->pointtype==pt_hvcurve ) &&
		    !n->nonextcp ) {
		SplineRefigure2(sp->next);
		if ( n->next==NULL ) {
		    bigreal len1, len2;
		    len1 = sqrt((n->prevcp.x-n->me.x)*(n->prevcp.x-n->me.x) +
				(n->prevcp.y-n->me.y)*(n->prevcp.y-n->me.y));
		    len2 = sqrt((n->nextcp.x-n->me.x)*(n->nextcp.x-n->me.x) +
				(n->nextcp.y-n->me.y)*(n->nextcp.y-n->me.y));
		    len2 /= len1;
		    n->nextcp.x = rint(len2 * (n->me.x-n->nextcp.x) + n->me.x);
		    n->nextcp.y = rint(len2 * (n->me.y-n->nextcp.y) + n->me.y);
		} else {
		    nn = n->next->to;
		    /* Find the intersection (if any) of the lines between */
		    /*  nn->prevcp&nn->me with n->nextcp&.->me */
		    if ( IntersectLines(&n_ncp,&nn->prevcp,&nn->me,&n->prevcp,&n->me)) {
			bigreal len = (nn->me.x-n->me.x)*(nn->me.x-n->me.x) + (nn->me.y-n->me.y)*(nn->me.y-n->me.y);
			bigreal d1 = (n_ncp.x-n->me.x)*(nn->me.x-n->me.x) + (n_ncp.y-n->me.y)*(nn->me.y-n->me.y);
			bigreal d2 = (n_ncp.x-nn->me.x)*(n->me.x-nn->me.x) + (n_ncp.y-nn->me.y)*(n->me.y-nn->me.y);
			if ( d1>=0 && d1<=len && d2>=0 && d2<=len ) {
			    if ( rint(2*n->me.x)==2*n->me.x && rint(2*nn->me.x)==2*nn->me.x )
				n_ncp.x = rint( n_ncp.x);
			    if ( rint(2*n->me.y)==2*n->me.y && rint(2*nn->me.y)==2*nn->me.y )
				n_ncp.y = rint( n_ncp.y);
			    n->nextcp = nn->prevcp = n_ncp;
			    SplineRefigure2(n->next);
			}
		    }
		}
	    }
	}
    }
}
