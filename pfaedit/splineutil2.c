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
#include "pfaedit.h"
#include <stdio.h>
#include <math.h>
#include "ustring.h"
#include "splinefont.h"
#include "chardata.h"
#include <unistd.h>
#include <pwd.h>
#include <time.h>

/*#define DEBUG	1*/

int RealNear(real a,real b) {
    real d;

#ifdef USE_DOUBLE
    if ( a==0 )
return( b>-1e-8 && b<1e-8 );
    if ( b==0 )
return( a>-1e-8 && a<1e-8 );

    d = a/(1024*1024.);
    if ( d<0 ) d = -d;
return( b>a-d && b<a+d );
#else		/* For floats */
    if ( a==0 )
return( b>-1e-5 && b<1e-5 );
    if ( b==0 )
return( a>-1e-5 && a<1e-5 );

    d = a/(1024*64.);
    if ( d<0 ) d = -d;
return( b>a-d && b<a+d );
#endif
}

int RealNearish(real a,real b) {

    if ( a-b<.001 && a-b>-.001 )
return( true );
return( false );
}

int RealApprox(real a,real b) {

    if ( a==0 ) {
	if ( b<.0001 && b>-.0001 )
return( true );
    } else if ( b==0 ) {
	if ( a<.0001 && a>-.0001 )
return( true );
    } else {
	a /= b;
	if ( a>=.95 && a<=1.05 )
return( true );
    }
return( false );
}

int RealWithin(real a,real b,real fudge) {

return( b>=a-fudge && b<=a+fudge );
}

int SplineIsLinear(Spline *spline) {
    real t1,t2;
    int ret;

    if ( spline->knownlinear )
return( true );
    if ( spline->knowncurved )
return( false );

    if ( spline->splines[0].a==0 && spline->splines[0].b==0 &&
	    spline->splines[1].a==0 && spline->splines[1].b==0 )
return( true );

    /* Something is linear if the control points lie on the line between the */
    /*  two base points */

    /* Vertical lines */
    if ( RealNear(spline->from->me.x,spline->to->me.x) ) {
	ret = RealNear(spline->from->me.x,spline->from->nextcp.x) &&
	    RealNear(spline->from->me.x,spline->to->prevcp.x) &&
	    ((spline->from->nextcp.y >= spline->from->me.y &&
	      spline->from->nextcp.y <= spline->to->prevcp.y &&
	      spline->to->prevcp.y <= spline->to->me.y ) ||
	     (spline->from->nextcp.y <= spline->from->me.y &&
	      spline->from->nextcp.y >= spline->to->prevcp.y &&
	      spline->to->prevcp.y >= spline->to->me.y ));
    /* Horizontal lines */
    } else if ( RealNear(spline->from->me.y,spline->to->me.y) ) {
	ret = RealNear(spline->from->me.y,spline->from->nextcp.y) &&
	    RealNear(spline->from->me.y,spline->to->prevcp.y) &&
	    ((spline->from->nextcp.x >= spline->from->me.x &&
	      spline->from->nextcp.x <= spline->to->prevcp.x &&
	      spline->to->prevcp.x <= spline->to->me.x) ||
	     (spline->from->nextcp.x <= spline->from->me.x &&
	      spline->from->nextcp.x >= spline->to->prevcp.x &&
	      spline->to->prevcp.x >= spline->to->me.x));
    } else {
	ret = true;
	t1 = (spline->from->nextcp.y-spline->from->me.y)/(spline->to->me.y-spline->from->me.y);
	if ( t1<0 || t1>1.0 )
	    ret = false;
	else {
	    t2 = (spline->from->nextcp.x-spline->from->me.x)/(spline->to->me.x-spline->from->me.x);
	    if ( t2<0 || t2>1.0 )
		ret = false;
	    ret = RealApprox(t1,t2);
	}
	if ( ret ) {
	    t1 = (spline->to->me.y-spline->to->prevcp.y)/(spline->to->me.y-spline->from->me.y);
	    if ( t1<0 || t1>1.0 )
		ret = false;
	    else {
		t2 = (spline->to->me.x-spline->to->prevcp.x)/(spline->to->me.x-spline->from->me.x);
		if ( t2<0 || t2>1.0 )
		    ret = false;
		else
		    ret = RealApprox(t1,t2);
	    }
	}
    }
    spline->knowncurved = !ret;
    spline->knownlinear = ret;
return( ret );
}

int SplineIsLinearMake(Spline *spline) {

    if ( spline->islinear )
return( true );
    if ( SplineIsLinear(spline)) {
	spline->islinear = spline->from->nonextcp = spline->to->noprevcp = true;
	spline->from->nextcp = spline->from->me;
	if ( spline->from->nonextcp && spline->from->noprevcp )
	    spline->from->pointtype = pt_corner;
	else if ( spline->from->pointtype == pt_curve )
	    spline->from->pointtype = pt_tangent;
	spline->to->prevcp = spline->to->me;
	if ( spline->to->nonextcp && spline->to->noprevcp )
	    spline->to->pointtype = pt_corner;
	else if ( spline->to->pointtype == pt_curve )
	    spline->to->pointtype = pt_tangent;
	SplineRefigure(spline);
    }
return( spline->islinear );
}

typedef struct spline1 {
    Spline1D sp;
    real s0, s1;
    real c0, c1;
} Spline1;

static void FigureSpline1(Spline1 *sp1,double t0, double t1, Spline1D *sp ) {
    double s = (t1-t0);
    if ( sp->a==0 && sp->b==0 ) {
	sp1->sp.d = sp->d + t0*sp->c;
	sp1->sp.c = s*sp->c;
	sp1->sp.b = sp1->sp.a = 0;
	sp1->c0 = sp1->s0; sp1->c1 = sp1->s1;
    } else {
	sp1->sp.d = sp->d + t0*(sp->c + t0*(sp->b + t0*sp->a));
	sp1->sp.c = s*(sp->c + t0*(2*sp->b + 3*sp->a*t0));
	sp1->sp.b = s*s*(sp->b+3*sp->a*t0);
	sp1->sp.a = s*s*s*sp->a;
#if 0		/* Got invoked once on a perfectly good spline */
	if ( ((sp1->s1>.001 || sp1->s1<-.001) && !RealNear((double) sp1->sp.a+sp1->sp.b+sp1->sp.c+sp1->sp.d,sp1->s1)) ||
		!RealNear(sp1->sp.d,sp1->s0))
	    GDrawIError( "Created spline does not work in FigureSpline1");
#endif
	sp1->c0 = sp1->sp.c/3 + sp1->s0;
	sp1->c1 = sp1->c0 + (sp1->sp.b+sp1->sp.c)/3;
    }
}

SplinePoint *SplineBisect(Spline *spline, double t) {
    Spline1 xstart, xend;
    Spline1 ystart, yend;
    Spline *spline1, *spline2;
    SplinePoint *mid;
    SplinePoint *old0, *old1;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];

#ifdef DEBUG
    if ( t<=1e-3 || t>=1-1e-3 )
	GDrawIError("Bisection to create a zero length spline");
#endif
    xstart.s0 = xsp->d; ystart.s0 = ysp->d;
    xend.s1 = (double) xsp->a+xsp->b+xsp->c+xsp->d;
    yend.s1 = (double) ysp->a+ysp->b+ysp->c+ysp->d;
    xstart.s1 = xend.s0 = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
    ystart.s1 = yend.s0 = ((ysp->a*t+ysp->b)*t+ysp->c)*t + ysp->d;
    FigureSpline1(&xstart,0,t,xsp);
    FigureSpline1(&xend,t,1,xsp);
    FigureSpline1(&ystart,0,t,ysp);
    FigureSpline1(&yend,t,1,ysp);

    mid = chunkalloc(sizeof(SplinePoint));
    mid->me.x = xstart.s1;	mid->me.y = ystart.s1;
    mid->nextcp.x = xend.c0;	mid->nextcp.y = yend.c0;
    mid->prevcp.x = xstart.c1;	mid->prevcp.y = ystart.c1;
    if ( mid->me.x==mid->nextcp.x && mid->me.y==mid->nextcp.y )
	mid->nonextcp = true;
    if ( mid->me.x==mid->prevcp.x && mid->me.y==mid->prevcp.y )
	mid->noprevcp = true;

    old0 = spline->from; old1 = spline->to;
    old0->nextcp.x = xstart.c0;	old0->nextcp.y = ystart.c0;
    old0->nonextcp = (old0->nextcp.x==old0->me.x && old0->nextcp.y==old0->me.y);
    old0->nextcpdef = false;
    old1->prevcp.x = xend.c1;	old1->prevcp.y = yend.c1;
    old1->noprevcp = (old1->prevcp.x==old1->me.x && old1->prevcp.y==old1->me.y);
    old1->prevcpdef = false;
    SplineFree(spline);

    spline1 = chunkalloc(sizeof(Spline));
    spline1->splines[0] = xstart.sp;	spline1->splines[1] = ystart.sp;
    spline1->from = old0;
    spline1->to = mid;
    old0->next = spline1;
    mid->prev = spline1;
    if ( SplineIsLinear(spline1)) {
	spline1->islinear = spline1->from->nonextcp = spline1->to->noprevcp = true;
	spline1->from->nextcp = spline1->from->me;
	spline1->to->prevcp = spline1->to->me;
    }
    SplineRefigure(spline1);

    spline2 = chunkalloc(sizeof(Spline));
    spline2->splines[0] = xend.sp;	spline2->splines[1] = xend.sp;
    spline2->from = mid;
    spline2->to = old1;
    mid->next = spline2;
    old1->prev = spline2;
    if ( SplineIsLinear(spline2)) {
	spline2->islinear = spline2->from->nonextcp = spline2->to->noprevcp = true;
	spline2->from->nextcp = spline2->from->me;
	spline2->to->prevcp = spline2->to->me;
    }
    SplineRefigure(spline2);
return( mid );
}

static Spline *IsLinearApprox(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt) {
    double vx, vy, slope;
    int i;

    vx = to->me.x-from->me.x; vy = to->me.y-from->me.y;
    if ( vx==0 && vy==0 ) {
	for ( i=0; i<cnt; ++i )
	    if ( mid[i].x != from->me.x || mid[i].y != from->me.y )
return( NULL );
    } else if ( fabs(vx)>fabs(vy) ) {
	slope = vy/vx;
	for ( i=0; i<cnt; ++i )
	    if ( !RealNear(mid[i].y,from->me.y+slope*(mid[i].x-from->me.x)) )
return( NULL );
    } else {
	slope = vx/vy; 
	for ( i=0; i<cnt; ++i )
	    if ( !RealNear(mid[i].x,from->me.x+slope*(mid[i].y-from->me.y)) )
return( NULL );
    }
    from->nonextcp = to->noprevcp = true;
return( SplineMake(from,to) );
}

/* Least squares tells us that:
	| S(xi*ti^3) |	 | S(ti^6) S(ti^5) S(ti^4) S(ti^3) |   | a |
	| S(xi*ti^2) | = | S(ti^5) S(ti^4) S(ti^3) S(ti^2) | * | b |
	| S(xi*ti)   |	 | S(ti^4) S(ti^3) S(ti^2) S(ti)   |   | c |
	| S(xi)	     |   | S(ti^3) S(ti^2) S(ti)   n       |   | d |
 and the definition of a spline tells us:
	| x1         | = |   1        1       1       1    | * (a b c d)
	| x0         | = |   0        0       0       1    | * (a b c d)
So we're a bit over specified. Let's use the second from last line of least squares
and the 2 from the spline defn. So d==x0. Now we've only got three unknowns
and only two equations, but we want to insure that the slope of the spline
at the end points remains unchanged
*/
/*
	| d y1         | = |   slope1*d x1 
	| d y0         | = |   slope0*d x0
=>
	| cy	       | = |   (orig c y)/(orig c x) * cx
	| 3*ay+2*by+cy | = |   (orig 3*ay + 2*by + cy)/(orig 3*ax+2*bx+cx) * (3*ax+2*bx+cx)
*/
/* cnt is not quite n because we add in two more points, first and last */
Spline *ApproximateSplineFromPointsSlopes(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt) {
    real t, t2, t3, t4, t5, x, y, xt, xt2, yt, yt2, tt, ttn;
    real sx, sy;
    int i;
    double v[6], m[6][6];	/* Yes! rounding errors cause problems here */
    Spline *spline;
    BasePoint prevcp, *p, *n;
    int err;

    /* If the two end-points are corner points then allow the slope to vary */
    /* Or if one end-point is a tangent but the point defining the tangent's */
    /*  slope is being removed then allow the slope to vary */
    /* Except if the slope is horizontal or vertical then keep it fixed */
    if ( ( !from->nonextcp && ( from->nextcp.x==from->me.x || from->nextcp.y==from->me.y)) ||
	    (!to->noprevcp && ( to->prevcp.x==to->me.x || to->prevcp.y==to->me.y)) )
	/* Preserve the slope */;
    else if ( (from->pointtype == pt_corner ||
		(from->pointtype == pt_tangent &&
			((from->nonextcp && from->noprevcp) || !from->noprevcp))) &&
	    (to->pointtype == pt_corner ||
		(to->pointtype == pt_tangent &&
			((to->nonextcp && to->noprevcp) || !to->nonextcp))) ) {
	from->pointtype = to->pointtype = pt_corner;
return( ApproximateSplineFromPoints(from,to,mid,cnt) );
    }

    t = t2 = t3 = t4 = t5 = 1;
    x = from->me.x+to->me.x; y = from->me.y+to->me.y;
    xt = xt2 = to->me.x; yt = yt2 = to->me.y;
    for ( i=0; i<cnt; ++i ) {
	x += mid[i].x;
	y += mid[i].y;
	tt = mid[i].t;
	xt += tt*mid[i].x;
	yt += tt*mid[i].y;
	xt2 += tt*tt*mid[i].x;
	yt2 += tt*tt*mid[i].y;
	t += tt;
	t2 += (ttn=tt*tt);
	t3 += (ttn*=tt);
	t4 += (ttn*=tt);
	t5 += (ttn*=tt);
    }

    v[0] = to->me.x-from->me.x;		/* ax */
    v[1] = xt-t*from->me.x;		/* bx */
    v[2] = 0;				/* cx */
    v[3] = to->me.y-from->me.y;		/* ay */
    v[4] = yt-t*from->me.y;		/* by */
    v[5] = 0;				/* cy */

    memset(m,0,sizeof(m));
    m[0][0] = 1;  m[0][1] = 1;  m[0][2] = 1;
    m[1][0] = t4; m[1][1] = t3; m[1][2] = t2;
    m[2][2] = from->next->splines[1].c; m[2][5] = -from->next->splines[0].c;
    m[3][3] = 1;  m[3][4] = 1;  m[3][5] = 1;
    m[4][3] = t4; m[4][4] = t3; m[4][5] = t2;
    sx = 3*to->prev->splines[0].a+2*to->prev->splines[0].b+to->prev->splines[0].c;
    sy = 3*to->prev->splines[1].a+2*to->prev->splines[1].b+to->prev->splines[1].c;
    m[5][0] = 3*sy; m[5][1] = 2*sy; m[5][2] = sy;
	m[5][3] = -3*sx; m[5][4] = -2*sx; m[5][5] = -sx;
    if ( t3-t4==0 ) {
	m[1][0] = m[4][3] = t3;
	m[1][1] = m[4][4] = t2;
	m[1][2] = m[4][5] = t;
	v[1] = x-(cnt+2)*from->me.x;
	v[4] = y-(cnt+2)*from->me.y;
    }

    /* Remove a terms from rows 1, 4, and 5 (are none in row 2) */
    v[1] -= m[1][0]*v[0];
    v[4] -= m[1][0]*v[3];
    m[1][1] -= m[1][0]; m[1][2] -= m[1][0];
    m[4][4] -= m[1][0]; m[4][5] -= m[1][0];
    m[1][0] = m[4][3] = 0;
    v[5] -= m[5][0]*v[0]+m[5][3]*v[3];
    m[5][1] -= m[5][0]; m[5][2] -= m[5][0];
	m[5][4] -= m[5][3]; m[5][5] -= m[5][3];
    m[5][0] = m[5][3] = 0;

    /* At this point the matrix looks like:
     1 ? ? 0 0 0
     0 ? ? 0 0 0
     0 0 ? 0 0 ?
     0 0 0 1 ? ?
     0 0 0 0 ? ?
     0 ? ? 0 ? ?
    */
    if ( m[1][1]>.0001 || m[1][1]<-.0001 ) { /* m[4][4] is same as m[1][1] */
	v[1] /= m[1][1];
	v[4] /= m[1][1];
	m[1][2] = m[4][5] /= m[1][1];

	v[0] -= m[0][1]*v[1];
	v[3] -= m[0][1]*v[4];
	m[0][2] -= m[0][1]*m[1][2]; m[0][1] = 0;
	m[3][5] -= m[3][4]*m[4][5]; m[3][4] = 0;
	v[5] -= m[5][1]*v[1] + m[5][4]*v[4];
	m[5][2] -= m[5][1]*m[1][2];
	m[5][5] -= m[5][4]*m[4][5];
	m[5][1] = m[5][4] = 0;
	m[1][1] = m[4][4] = 1;
    }

    /* At this point the matrix looks like:
     1 0 ? 0 0 0
     0 1 ? 0 0 0
     0 0 ? 0 0 ?
     0 0 0 1 0 ?
     0 0 0 0 1 ?
     0 0 ? 0 0 ?
    */
    if ( m[2][2]<.0001 && m[2][2]>-.0001 ) {
	real temp;
	m[2][2] = m[5][2];
	m[5][2] = 0;
	temp = m[2][5];
	m[2][5] = m[5][5];
	m[5][5] = temp;
	temp = v[2];
	v[2] = v[5];
	v[5] = temp;
    }
    if ( m[2][2]>=.0001 || m[2][2]<=-.0001 ) {
	v[2] /= m[2][2];
	m[2][5] /= m[2][2];

	if ( RealApprox(m[5][5],m[5][2]*m[2][5]) )
	    m[5][5] = 0;
	else
	    m[5][5] -= m[5][2]*m[2][5];
	v[5] -= m[5][2]*v[2];
	m[5][2] = 0;
	m[2][2] = 1;
    }
    if ( m[5][5]<.0001 && m[5][5]>-.0001 ) {	/* I have seen this failure mode */
	m[5][3] = t5;
	m[5][4] = t4;
	m[5][5] = t3;
	v[5] = xt2-t2*from->me.y;
	err = false;
	if ( t5!=0 ) {
	    m[5][5] -= t5*m[3][5];
	    v[5] -= t5*v[3];
	    m[5][3] = 0;
	} else err = true;
	if ( m[5][4]!=0 ) {
	    m[5][5] -= m[5][4]*m[4][5];
	    v[5] -= m[5][4]*v[4];
	    m[5][4] = 0;
	} else err = true;
	if ( err || (m[5][5]<.2 && m[5][5]>-.2 )) {
	    m[5][3] = t3;
	    m[5][4] = t2;
	    m[5][5] = t;
	    v[5] = y-(cnt+2)*from->me.y;
	    err = false;
	    if ( t3!=0 ) {
		m[5][5] -= t3*m[3][5];
		v[5] -= t3*v[3];
		m[5][3] = 0;
	    }
	    if ( m[5][4]!=0 ) {
		m[5][5] -= m[5][4]*m[4][5];
		v[5] -= m[5][4]*v[4];
		m[5][4] = 0;
	    }
	}
    }
    if ( m[5][5]>=.0001 || m[5][5]<=-.0001 ) {
	v[5] /= m[5][5];
	v[4] -= m[4][5]*v[5];
	v[3] -= m[3][5]*v[5];
	v[2] -= m[2][5]*v[5];
	m[4][5] = m[3][5] = m[2][5] = 0;
    }

    /* At this point the matrix looks like:
     1 0 ? 0 0 0
     0 1 ? 0 0 0
     0 0 1 0 0 0
     0 0 0 1 0 0
     0 0 0 0 1 0
     0 0 0 0 0 1
    */
    v[0] -= m[0][2]*v[2];
    v[1] -= m[1][2]*v[2];
    m[0][2] = m[1][2] = 0;

    /* Make sure the slopes we've found are in the same direction as they */
    /*  were before (might be diametrically opposed). If not then leave */
    /*  control points as they are (not a good soln, but it prevents an even */
    /*  worse) */
    prevcp.x = from->me.x + v[2]/3 + (v[2]+v[1])/3;
    prevcp.y = from->me.y + v[5]/3 + (v[5]+v[4])/3;
    n = from->nonextcp ? &to->me : &from->nextcp;
    p = to->noprevcp ? &from->me : &to->prevcp;
    if ( prevcp.x > 65536 || prevcp.x < -65536 || prevcp.y > 65536 || prevcp.y < -65536 ||
	    v[2] > 185536 || v[2] < -185536 || v[5] > 185536 || v[5] < -185536 ) {
	/* Well these aren't reasonable values. I assume a rounding error */
	/*  caused us to miss a divide by zero check. Just throw out the */
	/*  results and leave the control points as they were */
	fprintf( stderr, "Possible divide by zero problem, please send a copy of the current font\n" );
	fprintf( stderr, " the name of the character you were working on to gww@silcom.com\n" );
	fprintf( stderr, "From: (%g,%g) nextcp: (%g,%g) prevcp: (%g,%g) To: (%g,%g)\nVia: ",
	    from->me.x,from->me.y,
	    from->nextcp.x,from->nextcp.y,
	    to->prevcp.x, to->prevcp.y,
	    to->me.x, to->me.y);
	for ( i=0; i<cnt; ++i )
	    fprintf( stderr, "(%g,%g,%g) ", mid[i].x, mid[i].y, mid[i].t );
	fprintf( stderr, "\n");
    } else if ( v[2]*(n->x-from->me.x) + v[5]*(n->y-from->me.y)>=0 &&
	    (prevcp.x-to->me.x)*(p->x-to->me.x) + (prevcp.y-to->me.y)*(p->y-to->me.y)>=0 ) {
	from->nextcp.x = from->me.x + v[2]/3;
	from->nextcp.y = from->me.y + v[5]/3;
	from->nonextcp = v[2]==0 && v[5]==0;
	to->prevcp = prevcp;
	to->noprevcp = prevcp.x==to->me.x && prevcp.y==to->me.y;
    }
    spline = SplineMake(from,to);
    if ( SplineIsLinear(spline)) {
	spline->islinear = from->nonextcp = to->noprevcp = true;
	spline->from->nextcp = spline->from->me;
	spline->to->prevcp = spline->to->me;
	SplineRefigure(spline);
    }
return( spline );
}

/* Similar to the above except we don't try to preserve the slopes */
Spline *ApproximateSplineFromPoints(SplinePoint *from, SplinePoint *to,
	TPoint *mid, int cnt) {
    real t, t2, t3, t4, x, y, xt, yt, tt, ttn;
    int i;
    double vx[3], vy[3], m[3][3];
    Spline *spline;
    BasePoint nextcp, prevcp;

    if ( (spline = IsLinearApprox(from,to,mid,cnt))!=NULL )
return( spline );

    t = t2 = t3 = t4 = 1;
    x = from->me.x+to->me.x; y = from->me.y+to->me.y;
    xt = to->me.x; yt = to->me.y;
    for ( i=0; i<cnt; ++i ) {
	x += mid[i].x;
	y += mid[i].y;
	tt = mid[i].t;
	xt += tt*mid[i].x;
	yt += tt*mid[i].y;
	t += tt;
	t2 += (ttn=tt*tt);
	t3 += (ttn*=tt);
	t4 += (ttn*=tt);
    }

    vx[0] = xt-t*from->me.x;
    vx[1] = x-(cnt+2)*from->me.x;
    vx[2] = to->me.x-from->me.x;

    vy[0] = yt-t*from->me.y;
    vy[1] = y-(cnt+2)*from->me.y;
    vy[2] = to->me.y-from->me.y;

    m[0][0] = t4; m[0][1] = t3; m[0][2] = t2;
    m[1][0] = t3; m[1][1] = t2; m[1][2] = t;
    m[2][0] = 1;  m[2][1] = 1;  m[2][2] = 1;

    /* Remove a terms from rows 0 and 1 */
    vx[0] -= t4*vx[2];
    vy[0] -= t4*vy[2];
    m[0][0] = 0; m[0][1] -= t4; m[0][2] -= t4;
    vx[1] -= t3*vx[2];
    vy[1] -= t3*vy[2];
    m[1][0] = 0; m[1][1] -= t3; m[1][2] -= t3;

    if ( fabs(m[1][1])<fabs(m[0][1]) ) {
	real temp;
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

    nextcp.x = from->me.x + vx[0]/3;
    nextcp.y = from->me.y + vy[0]/3;
    prevcp.x = nextcp.x + (vx[0]+vx[1])/3;
    prevcp.y = nextcp.y + (vy[0]+vy[1])/3;
    if ( vx[0]*(to->me.x-from->me.x) + vy[0]*(to->me.y-from->me.y)>=0 ) {
	from->nextcp = nextcp;
	from->nonextcp = false;
    } else {
	from->nextcp = from->me;
	from->nonextcp = true;
    }
    if ( (prevcp.x-to->me.x)*(from->me.x-to->me.x) + (prevcp.y-to->me.y)*(from->me.y-to->me.y)>=0 ) {
	to->prevcp = prevcp;
	to->noprevcp = false;
    } else {
	to->prevcp = to->me;
	to->noprevcp = true;
    }
    spline = SplineMake(from,to);
    if ( SplineIsLinear(spline)) {
	spline->islinear = from->nonextcp = to->noprevcp = true;
	spline->from->nextcp = spline->from->me;
	spline->to->prevcp = spline->to->me;
	SplineRefigure(spline);
    }
return( spline );
}

    /* calculating the actual length of a spline is hard, this gives a very */
    /*  rough (but quick) approximation */
static double SplineLenApprox(Spline *spline) {
    double len, slen, temp;

    if ( (temp = spline->to->me.x-spline->from->me.x)<0 ) temp = -temp;
    len = temp;
    if ( (temp = spline->to->me.y-spline->from->me.y)<0 ) temp = -temp;
    len += temp;
    if ( !spline->to->noprevcp || !spline->from->nonextcp ) {
	if ( (temp = spline->from->nextcp.x-spline->from->me.x)<0 ) temp = -temp;
	slen = temp;
	if ( (temp = spline->from->nextcp.y-spline->from->me.y)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->prevcp.x-spline->from->nextcp.x)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->prevcp.y-spline->from->nextcp.y)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->me.x-spline->to->prevcp.x)<0 ) temp = -temp;
	slen += temp;
	if ( (temp = spline->to->me.y-spline->to->prevcp.y)<0 ) temp = -temp;
	slen += temp;
	len = (len + slen)/2;
    }
return( len );
}

static TPoint *SplinesFigureTPsBetween(SplinePoint *from, SplinePoint *to,
	int *tot) {
    int cnt, i, j;
    real len, slen, lbase;
    SplinePoint *np;
    TPoint *tp;

    cnt = 0; len = 0;
    for ( np = from->next->to; ; np = np->next->to ) {
	++cnt;
	len += SplineLenApprox(np->prev);
	if ( np==to )
    break;
    }

#if 0
    extras = fcp = tcp = 0;
    if ( from->pointtype!=pt_corner && !from->nonextcp ) {
	extras += 4*cnt;
	fcp = 1;
    }
    if ( to->pointtype!=pt_corner && !to->noprevcp ) {
	extras += 4*cnt;
	tcp = 1;
    }
    tp = galloc((4*(cnt+1)+extras)*sizeof(TPoint)); i = 0;
#endif

    tp = galloc(10*(cnt+1)*sizeof(TPoint)); i = 0;
    if ( len==0 ) {
	for ( ; i<=10*cnt; ++i ) {
	    tp[i].t = i/(10.0*cnt);
	    tp[i].x = from->me.x;
	    tp[i].y = from->me.y;
	}
    } else {
	lbase = 0;
	for ( np = from->next->to; ; np = np->next->to ) {
	    slen = SplineLenApprox(np->prev);
	    for ( j=0; j<10; ++j ) {
		real t = j/10.0;
		tp[i].t = (lbase+ t*slen)/len;
		tp[i].x = ((np->prev->splines[0].a*t+np->prev->splines[0].b)*t+np->prev->splines[0].c)*t + np->prev->splines[0].d;
		tp[i++].y = ((np->prev->splines[1].a*t+np->prev->splines[1].b)*t+np->prev->splines[1].c)*t + np->prev->splines[1].d;
	    }
	    lbase += slen;
	    if ( np==to )
	break;
	}
    }

#if 0
    /* for curved and tangent points we need the slope at the end points to */
    /*  be the same as it was before. So we add a point near the start of the */
    /*  from spline and weight it heavily */
    if ( fcp ) {
	np = from->next->to;
	temp = (np->me.x-np->prev->from->me.x);
	if ( temp<0 ) temp = -temp;
	slen = temp;
	temp = (np->me.y-np->prev->from->me.y);
	if ( temp<0 ) temp = -temp;
	slen += temp;
	t = .05;
	tp[i].t = (t*slen)/len;
	tp[i].x = ((np->prev->splines[0].a*t+np->prev->splines[0].b)*t+np->prev->splines[0].c)*t + np->prev->splines[0].d;
	tp[i].y = ((np->prev->splines[1].a*t+np->prev->splines[1].b)*t+np->prev->splines[1].c)*t + np->prev->splines[1].d;
	for ( j=1; j<4*cnt; ++j )
	    tp[i+j] = tp[i];
	i += j;
    }
    if ( tcp ) {
	np = to;
	temp = (np->me.x-np->prev->from->me.x);
	if ( temp<0 ) temp = -temp;
	slen = temp;
	temp = (np->me.y-np->prev->from->me.y);
	if ( temp<0 ) temp = -temp;
	slen += temp;
	t = .95;
	tp[i].t = (t*slen)/len;
	tp[i].x = ((np->prev->splines[0].a*t+np->prev->splines[0].b)*t+np->prev->splines[0].c)*t + np->prev->splines[0].d;
	tp[i].y = ((np->prev->splines[1].a*t+np->prev->splines[1].b)*t+np->prev->splines[1].c)*t + np->prev->splines[1].d;
	for ( j=1; j<4*cnt; ++j )
	    tp[i+j] = tp[i];
	i += j;
    }
#endif
    *tot = i;
	
return( tp );
}

static void FixupCurveTanPoints(SplinePoint *from,SplinePoint *to,
        BasePoint *fncp, BasePoint *tpcp) {

    /* if the control points don't match the point types then change the */
    /*  point types. I used to try changing the point types, but the results */
    /*  weren't good */
    if ( from->pointtype!=pt_corner && !from->nonextcp ) {
        fncp->x -= from->me.x; fncp->y -= from->me.y;
        if ( fncp->x==0 && fncp->y==0 ) {
            if ( from->pointtype == pt_tangent && from->prev!=NULL ) {
                fncp->x = from->me.x-from->prev->from->me.x;
                fncp->y = from->me.y-from->prev->from->me.y;
            } else if ( from->pointtype == pt_curve && !from->noprevcp ) {
                fncp->x = from->me.x-from->prevcp.x;
                fncp->y = from->me.y-from->prevcp.y;
            }
        }
        if ( fncp->x!=0 || fncp->y!=0 ) {
            if ( !RealNear(atan2(fncp->y,fncp->x),
                    atan2(from->nextcp.y-from->me.y,from->nextcp.x-from->me.x)) )
                from->pointtype = pt_corner;
        }
    }
    if ( to->pointtype!=pt_corner && !to->noprevcp ) {
        tpcp->x -= to->me.x; tpcp->y -= to->me.y;
        if ( tpcp->x==0 && tpcp->y==0 ) {
            if ( to->pointtype == pt_tangent && to->next!=NULL ) {
                tpcp->x = to->me.x-to->next->to->me.x;
                tpcp->y = to->me.y-to->next->to->me.y;
            } else if ( to->pointtype == pt_curve && !to->nonextcp ) {
                tpcp->x = to->me.x-to->nextcp.x;
                tpcp->y = to->me.y-to->nextcp.y;
            }
        }
        if ( tpcp->x!=0 || tpcp->y!=0 ) {
            if ( !RealNear(atan2(tpcp->y,tpcp->x),
                    atan2(to->prevcp.y-to->me.y,to->prevcp.x-to->me.x)) )
                to->pointtype = pt_corner;
        }
    }
    if ( from->pointtype==pt_tangent )
        SplineCharTangentPrevCP(from);
    if ( to->pointtype==pt_tangent )
        SplineCharTangentNextCP(to);
}

static void SplinesRemoveBetween(SplineChar *sc, SplinePoint *from, SplinePoint *to,int type) {
    int tot;
    TPoint *tp;
    SplinePoint *np, oldfrom;
    Spline *sp;
    BasePoint fncp, tpcp;

    tp = SplinesFigureTPsBetween(from,to,&tot);
    fncp = from->nextcp; tpcp = to->prevcp;

    oldfrom = *from;
    if ( type==1 )
	ApproximateSplineFromPointsSlopes(from,to,tp,tot-1);
    else
	ApproximateSplineFromPoints(from,to,tp,tot-1);
    /* Have to do the frees after the approximation because the approx */
    /*  uses the splines to determine slopes */
    for ( sp = oldfrom.next; ; ) {
	np = sp->to;
	SplineFree(sp);
	if ( np==to )
    break;
	sp = np->next;
	SplinePointMDFree(sc,np);
    }
    if ( type==0 )
	FixupCurveTanPoints(from,to,&fncp,&tpcp);
    free(tp);
}

static SplinePointList *SplinePointListMerge(SplineChar *sc, SplinePointList *spl,int type) {
    Spline *spline, *first;
    SplinePoint *nextp, *curp, *selectme;
    int all;

    /* If the entire splineset is selected, it should merge into oblivion */
    first = NULL;
    all = spl->first->selected;
    for ( spline = spl->first->next; spline!=NULL && spline!=first && all; spline=spline->to->next ) {
	if ( !spline->to->selected ) all = false;
	if ( first==NULL ) first = spline;
    }
    if ( all )
return( NULL );			/* Some one else should free it and reorder the spline set list */

    if ( spl->first!=spl->last ) {
	/* If the spline isn't closed, then any selected points at the ends */
	/*  get deleted */
	while ( spl->first->selected ) {
	    nextp = spl->first->next->to;
	    SplineFree(spl->first->next);
	    SplinePointMDFree(sc,spl->first);
	    spl->first = nextp;
	    nextp->prev = NULL;
	}
	while ( spl->last->selected ) {
	    nextp = spl->last->prev->from;
	    SplineFree(spl->last->prev);
	    SplinePointMDFree(sc,spl->last);
	    spl->last = nextp;
	    nextp->next = NULL;
	}
    } else {
	while ( spl->first->selected ) {
	    spl->first = spl->first->next->to;
	    spl->last = spl->first;
	}
    }

    /* when we get here spl->first is not selected */
    if ( spl->first->selected ) GDrawIError( "spl->first is selected in SplinePointListMerge");
    curp = spl->first;
    selectme = NULL;
    while ( 1 ) {
	while ( !curp->selected ) {
	    if ( curp->next==NULL )
		curp = NULL;
	    else
		curp = curp->next->to;
	    if ( curp==spl->first || curp==NULL )
	break;
	}
	if ( curp==NULL || !curp->selected )
    break;
	for ( nextp=curp->next->to; nextp->selected; nextp = nextp->next->to );
	/* we don't need to check for the end of the splineset here because */
	/*  we know that spl->last is not selected */
	SplinesRemoveBetween(sc,curp->prev->from,nextp,type);
	curp = nextp;
	selectme = nextp;
    }
    if ( selectme!=NULL ) selectme->selected = true;
return( spl );
}

void SplineCharMerge(SplineChar *sc,SplineSet **head,int type) {
    SplineSet *spl, *prev=NULL, *next;

    for ( spl = *head; spl!=NULL; spl = next ) {
	next = spl->next;
	if ( SplinePointListMerge(sc,spl,type)==NULL ) {
	    if ( prev==NULL )
		*head = next;
	    else
		prev->next = next;
	    chunkfree(spl,sizeof(*spl));
	} else
	    prev = spl;
    }
}

/* Almost exactly the same as SplinesRemoveBetween, but this one is conditional */
/*  the point is only removed if it doesn't perturb the spline much and if */
/*  it isn't an extrema.
/*  used for simplify */
static int SplinesRemoveMidMaybe(SplineChar *sc,SplinePoint *mid, int type) {
    int i,tot;
    SplinePoint *from, *to;
    TPoint *tp;
    BasePoint test;
    int good;
    BasePoint fncp, tpcp, fncp2, tpcp2;
    int fpt, tpt;

    if ( mid->prev==NULL || mid->next==NULL )
return( false );

    from = mid->prev->from; to = mid->next->to;

    /* Retain points which are horizontal or vertical, because postscript says*/
    /*  type1 fonts should always have a point at the extrema (except for small*/
    /*  things like serifs), and the extrema occur at horizontal/vertical points*/
    if ( (!mid->nonextcp && (mid->nextcp.x==mid->me.x || mid->nextcp.y==mid->me.y)) ||
	    (!mid->noprevcp && (mid->prevcp.x==mid->me.x || mid->prevcp.y==mid->me.y)) ) {
	real x=mid->me.x, y=mid->me.y;
	if (( mid->nextcp.x==x && mid->prevcp.x==x &&
		 to->me.x==x && to->prevcp.x==x &&
		 from->me.x==x && from->nextcp.x==x ) ||
		( mid->nextcp.y==y && mid->prevcp.y==y &&
		 to->me.y==y && to->prevcp.y==y &&
		 from->me.y==y && from->nextcp.y==y ))
	    /* On the other hand a point in the middle of a straight */
	    /* horizontal/vertical line may still be removed with impunity */
	    ;
	else
return( false );
    }

    fncp2 = fncp = from->nextcp; tpcp2 = tpcp = to->prevcp;
    fpt = from->pointtype; tpt = to->pointtype;

    /* if from or to is a tangent then we can only remove mid if it's on the */
    /*  line between them */
    if (( from->pointtype==pt_tangent && !from->noprevcp) ||
	    ( to->pointtype==pt_tangent && !to->nonextcp)) {
	if ( from->me.x==to->me.x ) {
	    if ( mid->me.x!=to->me.x )
return( false );
	} else if ( !RealNear((from->me.y-to->me.y)/(from->me.x-to->me.x),
			    (mid->me.y-to->me.y)/(mid->me.x-to->me.x)) )
return( false );
    }

    tp = SplinesFigureTPsBetween(from,to,&tot);

    if ( !type )
	ApproximateSplineFromPointsSlopes(from,to,tp,tot-1);
    else {
	ApproximateSplineFromPoints(from,to,tp,tot-1);
	FixupCurveTanPoints(from,to,&fncp2,&tpcp2);
    }

    i = tot;

    good = true;
    while ( --i>0 && good ) {
	/* tp[0] is the same as from (easier to include it), but the SplineNear*/
	/*  routine will sometimes reject the end-points of the spline */
	/*  so just don't check it */
	test.x = tp[i].x; test.y = tp[i].y;
	good = SplineNearPoint(from->next,&test,.75)!= -1;
    }

    free(tp);
    if ( good ) {
	SplineFree(mid->prev);
	SplineFree(mid->next);
	SplinePointMDFree(sc,mid);
    } else {
	SplineFree(from->next);
	from->next = mid->prev;
	from->nextcp = fncp;
	from->nonextcp = ( fncp.x==from->me.x && fncp.y==from->me.y);
	from->pointtype = fpt;
	to->prev = mid->next;
	to->prevcp = tpcp;
	to->noprevcp = ( tpcp.x==to->me.x && tpcp.y==to->me.y);
	to->pointtype = tpt;
    }
return( good );
}

/* Cleanup just turns splines with control points which happen to trace out */
/*  lines into simple lines */
void SplinePointListSimplify(SplineChar *sc,SplinePointList *spl,int type) {
    SplinePoint *first, *next, *sp;

	/* Special case checks for paths containing only one point */
	/*  else we get lots of nans (or only two) */

    if ( type!=-1 && spl->first->prev!=NULL ) {
	while ( 1 ) {
	    first = spl->first->prev->from;
	    if ( first->prev == first->next )
return;
	    if ( !SplinesRemoveMidMaybe(sc,spl->first,type))
	break;
	    if ( spl->first==spl->last )
		spl->last = first;
	    spl->first = first;
	}
    }
    if ( spl->first->next == NULL )
return;
    for ( sp = spl->first->next->to; sp!=spl->last && sp->next!=NULL; sp = next ) {
	SplineIsLinearMake(sp->prev);		/* First see if we can turn it*/
				/* into a line, then try to merge two splines */
	next = sp->next->to;
	if ( sp->prev == sp->next ||
		(sp->next!=NULL && sp->next->to->next!=NULL &&
		    sp->next->to->next->to == sp ))
return;
	if ( type!=-1 )
	    SplinesRemoveMidMaybe(sc,sp,type);
	else {
	    while ( sp->me.x==next->me.x && sp->me.y==next->me.y &&
		    sp->nextcp.x>sp->me.x-1 && sp->nextcp.x<sp->me.x+1 &&
		    sp->nextcp.y>sp->me.y-1 && sp->nextcp.y<sp->me.y+1 &&
		    next->prevcp.x>next->me.x-1 && next->prevcp.x<next->me.x+1 &&
		    next->prevcp.y>next->me.y-1 && next->prevcp.y<next->me.y+1 ) {
		SplineFree(sp->next);
		sp->next = next->next;
		if ( sp->next!=NULL )
		    sp->next->from = sp;
		sp->nextcp = next->nextcp;
		sp->nonextcp = next->nonextcp;
		sp->nextcpdef = next->nextcpdef;
		SplinePointMDFree(sc,next);
		if ( sp->next!=NULL )
		    next = sp->next->to;
		else {
		    next = NULL;
	    break;
		}
	    }
	    if ( next==NULL )
    break;
	}
    }
}

SplineSet *SplineCharSimplify(SplineChar *sc,SplineSet *head,int cleanup) {
    SplineSet *spl, *prev, *snext;
    int anysel=0;

    for ( spl = head; spl!=NULL && !anysel; spl = spl->next ) {
	anysel = PointListIsSelected(spl);
    }

    prev = NULL;
    for ( spl = head; spl!=NULL; spl = snext ) {
	snext = spl->next;
	if ( !anysel || PointListIsSelected(spl)) {
	    SplinePointListSimplify(sc,spl,cleanup);
	    /* remove any singleton points */
	    if ( spl->first->prev==spl->first->next &&
		    (spl->first->prev==NULL ||
		     (spl->first->noprevcp && spl->first->nonextcp))) {
		if ( prev==NULL )
		    head = snext;
		else
		    prev->next = snext;
		spl->next = NULL;
		SplinePointListMDFree(sc,spl);
	    } else
		prev = spl;
	}
    }
return( head );
}

SplineSet *SplineCharRemoveTiny(SplineChar *sc,SplineSet *head) {
    SplineSet *spl, *snext, *pr;
    Spline *spline, *next, *first;

    for ( spl = head, pr=NULL; spl!=NULL; spl = snext ) {
	first = NULL;
	for ( spline=spl->first->next; spline!=NULL && spline!=first; spline=next ) {
	    next = spline->to->next;
	    if ( spline->from->me.x-spline->to->me.x>-1 && spline->from->me.x-spline->to->me.x<1 &&
		    spline->from->me.y-spline->to->me.y>-1 && spline->from->me.y-spline->to->me.y<1 &&
		    (spline->from->nonextcp || spline->to->noprevcp) &&
		    spline->from->prev!=NULL ) {
		if ( spline->from==spline->to )
	    break;
		if ( spl->last==spline->from ) spl->last = NULL;
		if ( spl->first==spline->from ) spl->first = NULL;
		if ( first==spline->from->prev ) first=NULL;
		/*SplinesRemoveBetween(sc,spline->from->prev->from,spline->to);*/
		spline->to->prevcp = spline->from->prevcp;
		spline->to->noprevcp = spline->from->noprevcp;
		spline->to->prevcpdef = spline->from->prevcpdef;
		spline->from->prev->to = spline->to;
		spline->to->prev = spline->from->prev;
		SplineRefigure(spline->from->prev);
		SplinePointFree(spline->from);
		SplineFree(spline);
		if ( first==NULL ) first = next->from->prev;
		if ( spl->first==NULL ) spl->first = next->from;
		if ( spl->last==NULL ) spl->last = next->from;
	    } else {
		if ( first==NULL ) first = spline;
	    }
	}
	snext = spl->next;
	if ( spl->first->next==spl->first->prev ) {
	    spl->next = NULL;
	    SplinePointListMDFree(sc,spl);
	    if ( pr==NULL )
		head = snext;
	    else
		pr->next = snext;
	} else
	    pr = spl;
    }
return( head );
}

static Spline *SplineAddExtrema(Spline *s) {
    /* First find the extrema, if any */
    double t[4], min;
    int p, i,j;
    SplinePoint *sp;

    forever {
	if ( s->islinear )
return(s);
	p = 0;
	if ( s->splines[0].a!=0 ) {
	    double d = 4*s->splines[0].b*s->splines[0].b-4*3*s->splines[0].a*s->splines[0].c;
	    if ( d>0 ) {
		d = sqrt(d);
		t[p++] = (-2*s->splines[0].b+d)/(2*3*s->splines[0].a);
		t[p++] = (-2*s->splines[0].b-d)/(2*3*s->splines[0].a);
	    }
	} else if ( s->splines[0].b!=0 )
	    t[p++] = -s->splines[0].c/(2*s->splines[0].b);
	if ( s->splines[1].a!=0 ) {
	    double d = 4*s->splines[1].b*s->splines[1].b-4*3*s->splines[1].a*s->splines[1].c;
	    if ( d>0 ) {
		d = sqrt(d);
		t[p++] = (-2*s->splines[1].b+d)/(2*3*s->splines[1].a);
		t[p++] = (-2*s->splines[1].b-d)/(2*3*s->splines[1].a);
	    }
	} else if ( s->splines[1].b!=0 )
	    t[p++] = -s->splines[1].c/(2*s->splines[1].b);

	/* Throw out any t values which are not between 0 and 1 */
	/*  (we do a little fudging near the endpoints so we don't get confused */
	/*   by rounding errors) */
	for ( i=0; i<p; ++i ) {
	    if ( t[i]<.0001 || t[i]>.9999 ) {
		--p;
		for ( j=i; j<p; ++j )
		    t[j] = t[j+1];
		--i;
	    }
	}
	if ( p==0 )
return(s);

	/* Find the smallest of all the interesting points */
	min = t[0];
	for ( i=1; i<p; ++i ) {
	    if ( t[i]<min )
		min=t[i];
	}
	sp = SplineBisect(s,min);
	s = sp->next;
	/* Don't try to use any other computed t values, it is easier to */
	/*  recompute them than to try and figure out what they map to on the */
	/*  new spline */
    }
}

void SplineCharAddExtrema(SplineSet *head,int between_selected) {
    SplineSet *ss;
    Spline *s, *first;

    for ( ss=head; ss!=NULL; ss=ss->next ) {
	first = NULL;
	for ( s = ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	    if ( first==NULL ) first = s;
	    if ( !between_selected || (s->from->selected && s->to->selected))
		s = SplineAddExtrema(s);
	}
    }
}

char *GetNextUntitledName(void) {
    static int untitled_cnt=1;
    char buffer[80];

    sprintf( buffer, "Untitled%d", untitled_cnt++ );
return( copy(buffer));
}

SplineFont *SplineFontEmpty(void) {
    SplineFont *sf;
    sf = gcalloc(1,sizeof(SplineFont));
    sf->pfminfo.fstype = -1;
    sf->encoding_name = em_none;
return( sf );
}

SplineFont *SplineFontBlank(int encoding_name,int charcnt) {
    SplineFont *sf;
    char buffer[200], *pt;
    struct passwd *pwd;
    time_t now;
    struct tm *tm;

    sf = SplineFontEmpty();
    sf->fontname = GetNextUntitledName();
    sf->fullname = copy(sf->fontname);
    sf->familyname = copy(sf->fontname);
    sprintf( buffer, "%s.sfd", sf->fontname);
    sf->origname = copy(buffer);
    sf->weight = copy("Medium");
/* Can all be commented out if no pwd routines */
    pwd = getpwuid(getuid());
    if ( pwd!=NULL && pwd->pw_gecos!=NULL && *pwd->pw_gecos!='\0' )
	sprintf( buffer, "Created by %.50s with PfaEdit 1.0 (http://pfaedit.sf.net)", pwd->pw_gecos );
    else if ( pwd!=NULL && pwd->pw_name!=NULL && *pwd->pw_name!='\0' )
	sprintf( buffer, "Created by %.50s with PfaEdit 1.0 (http://pfaedit.sf.net)", pwd->pw_name );
    else if ( (pt=getenv("USER"))!=NULL )
	sprintf( buffer, "Created by %.50s with PfaEdit 1.0 (http://pfaedit.sf.net)", pt );
    else
	strcpy( buffer, "Created with PfaEdit 1.0 (http://pfaedit.sf.net)" );
    endpwent();
/* End comment */
    sf->copyright = copy(buffer);
    if ( xuid!=NULL ) {
	sf->xuid = galloc(strlen(xuid)+20);
	sprintf(sf->xuid,"[%s %d]", xuid, (rand()&0xffffff));
    }
    time(&now);
    tm = localtime(&now);
    sprintf( buffer, "%d-%d-%d: Created.", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday );
    sf->comments = copy(buffer);
    sf->version = copy("001.000");
    sf->upos = -100; sf->uwidth = 50;		/* defaults for cff */
    sf->ascent = 800; sf->descent = 200;
    sf->charcnt = charcnt;
    sf->chars = gcalloc(charcnt,sizeof(SplineChar *));
    sf->encoding_name = encoding_name;
    sf->pfminfo.fstype = -1;
return( sf );
}

/* see also SFReencodeFont in fontinfo.c */
SplineFont *SplineFontNew(void) {
    SplineFont *sf;
    const char *name;
    int i,uenc;
    /* Create an ISO 8859-1 (Latin1) font, actually whatever default_encoding is */
    const unsigned short *table;
    int tlen=256, enclen=256;
    char buf[10];
    extern unsigned short unicode_from_adobestd[256];
    Encoding *item=NULL;

    if ( default_encoding>=em_base ) {
	for ( item=enclist; item!=NULL && item->enc_num!=default_encoding; item=item->next );
	if ( item==NULL )
	    default_encoding = em_iso8859_1;
    }
    if ( default_encoding==em_adobestandard ) {
	table = unicode_from_adobestd;
    } else if ( default_encoding==em_iso8859_1 )
	table = unicode_from_i8859_1;
    else if ( default_encoding==em_unicode ) {
	table = NULL;
	enclen = 65536;
    } else if ( default_encoding==em_unicode4 ) {
	table = NULL;
	enclen = unicode4_size;
    } else if ( default_encoding>=em_unicodeplanes && default_encoding<=em_unicodeplanesmax ) {
	table = NULL;
	enclen = 65536;
    } else if ( default_encoding==em_jis208 ) {
	table = unicode_from_jis208;
	tlen = 94*94;
	enclen = 94*96;
    } else if ( default_encoding==em_jis212 ) {
	table = unicode_from_jis212;
	tlen = 94*94;
	enclen = 94*96;
    } else if ( default_encoding==em_ksc5601 ) {
	table = unicode_from_ksc5601;
	tlen = 94*94;
	enclen = 94*96;
    } else if ( default_encoding==em_gb2312 ) {
	table = unicode_from_gb2312;
	tlen = 94*94;
	enclen = 94*96;
    } else if ( default_encoding==em_big5 ) {
	table = unicode_from_big5;
	tlen = 0x10000-0xa100;
	enclen = 65536;
    } else if ( default_encoding==em_johab ) {
	table = unicode_from_johab;
	tlen = 0x10000-0x8400;
	enclen = 65536;
    } else if ( default_encoding==em_wansung ) {
	table = unicode_from_ksc5601;
	tlen = 0x10000-0x8400;
	enclen = 65536;
    } else if ( default_encoding==em_sjis ) {
	table = unicode_from_jis208;
	tlen = 257;
	enclen = 65536;
    } else if ( item!=NULL ) {
	table = item->unicode;
	tlen = enclen = item->char_cnt;
    } else
	table = unicode_from_alphabets[default_encoding+3];

    sf = SplineFontBlank(default_encoding,enclen);
    sf->display_antialias = default_fv_antialias;
    sf->display_size = -default_fv_font_size;
    sf->onlybitmaps = true;
    for ( i=0; i<enclen && i<256; ++i ) {
	SplineChar *sc = sf->chars[i] = chunkalloc(sizeof(SplineChar));
	sc->vwidth = sf->ascent+sf->descent;
	if ( default_encoding>=em_unicodeplanes && default_encoding<=em_unicodeplanesmax )
	    uenc = i+ ((default_encoding-em_unicodeplanes)<<16);
	else if ( table==NULL )
	    uenc = i;
	else if ( tlen==94*94 ) {
	    if ( i%96==0 || i%96==95 )
		uenc = -1;
	    else
		uenc = table[(i/96)*94+(i%96-1)];
	} else if ( tlen==0x10000-0xa100 || tlen == 0x10000-0x8400 ) {
	    uenc = ( i<160 )?i : -1;	/* deal with single byte encoding of big5 */
	} else if ( tlen==257 ) {		/* sjis */
	    if ( i<0x80 ) uenc = i;
	    else if ( i>=0xa1 && i<=0xdf )
		uenc = unicode_from_jis201[i];
	    else uenc = -1;
	} else
	    uenc = table[i];
	sc->enc = i;
	sc->unicodeenc = uenc;
	name = uenc<=0?".notdef":psunicodenames[uenc];
	if ( uenc==-1 && item!=NULL && item->psnames!=NULL && item->psnames[i]!=NULL )
	    name = item->psnames[i];
	if ( name==NULL ) {
	    if ( uenc==0xa0 )
		name = "nonbreakingspace";
	    else if ( uenc==0x2d )
		name = "hyphen-minus";
	    else if ( uenc==0xad )
		name = "softhyphen";
	    else if ( uenc<32 || (uenc>0x7e && uenc<0xa0))
		name = ".notdef";
	    else {
		sprintf(buf,"uni%04X", uenc );
		name = buf;
	    }
	}
	sc->name = copy(name);
	sc->width = sf->ascent+sf->descent;
	sc->lsidebearing = 0;
	sc->parent = sf;
	sc->lig = SCLigDefault(sc);
    }
return( sf );
}

static void SFChangeXUID(SplineFont *sf, int random) {
    char *pt, *new, *npt;
    int val;

    if ( sf->xuid==NULL )
return;
    pt = strrchr(sf->xuid,' ');
    if ( pt==NULL )
	pt = strchr(sf->xuid,'[');
    if ( pt==NULL )
	pt = sf->xuid;
    else
	++pt;
    if ( random )
	val = rand()&0xffffff;
    else {
	val = strtol(pt,NULL,10);
	val = (val+1)&0xffffff;
    }

    new = galloc(pt-sf->xuid+12);
    strncpy(new,sf->xuid,pt-sf->xuid);
    npt = new + (pt-sf->xuid);
    if ( npt==new ) *npt++ = '[';
    sprintf(npt, "%d]", val );
    free(sf->xuid); sf->xuid = new;
    sf->changed = true;
}

void SFIncrementXUID(SplineFont *sf) {
    SFChangeXUID(sf,false);
}

void SFRandomChangeXUID(SplineFont *sf) {
    SFChangeXUID(sf,true);
}

void SPAverageCps(SplinePoint *sp) {
    real pangle, nangle, angle, plen, nlen, c, s;
    if ( sp->pointtype==pt_curve && sp->prev && sp->next ) {
	pangle = atan2(sp->me.y-sp->prevcp.y,sp->me.x-sp->prevcp.x);
	nangle = atan2(sp->nextcp.y-sp->me.y,sp->nextcp.x-sp->me.x);
	if ( pangle<0 && nangle>0 && nangle-pangle>=3.1415926 )
	    pangle += 2*3.1415926535897932;
	else if ( pangle>0 && nangle<0 && pangle-nangle>=3.1415926 )
	    nangle += 2*3.1415926535897932;
	angle = (nangle+pangle)/2;
	plen = -sqrt((sp->me.y-sp->prevcp.y)*(sp->me.y-sp->prevcp.y) +
		(sp->me.x-sp->prevcp.x)*(sp->me.x-sp->prevcp.x));
	nlen = sqrt((sp->nextcp.y-sp->me.y)*(sp->nextcp.y-sp->me.y) +
		(sp->nextcp.x-sp->me.x)*(sp->nextcp.x-sp->me.x));
	c = cos(angle); s=sin(angle);
	sp->nextcp.x = c*nlen + sp->me.x;
	sp->nextcp.y = s*nlen + sp->me.y;
	sp->prevcp.x = c*plen + sp->me.x;
	sp->prevcp.y = s*plen + sp->me.y;
	SplineRefigure(sp->prev);
	SplineRefigure(sp->next);
    }
}

void SplineCharTangentNextCP(SplinePoint *sp) {
    real angle, len;
    BasePoint *bp;

    if ( sp->prev==NULL )
return;
    bp = &sp->prev->from->me;

    angle = atan2(sp->me.y-bp->y,sp->me.x-bp->x);
    len = sqrt((sp->nextcp.y-sp->me.y)*(sp->nextcp.y-sp->me.y) + (sp->nextcp.x-sp->me.x)*(sp->nextcp.x-sp->me.x));
    sp->nextcp.x = sp->me.x + len*cos(angle);
    sp->nextcp.y = sp->me.y + len*sin(angle);
    sp->nextcp.x = rint(sp->nextcp.x*1024)/1024;
    sp->nextcp.y = rint(sp->nextcp.y*1024)/1024;
}

void SplineCharTangentPrevCP(SplinePoint *sp) {
    real angle, len;
    BasePoint *bp;

    if ( sp->next==NULL )
return;
    bp = &sp->next->to->me;

    angle = atan2(sp->me.y-bp->y,sp->me.x-bp->x);
    len = sqrt((sp->prevcp.y-sp->me.y)*(sp->prevcp.y-sp->me.y) + (sp->prevcp.x-sp->me.x)*(sp->prevcp.x-sp->me.x));
    sp->prevcp.x = sp->me.x + len*cos(angle);
    sp->prevcp.y = sp->me.y + len*sin(angle);
    sp->prevcp.x = rint(sp->prevcp.x*1024)/1024;
    sp->prevcp.y = rint(sp->prevcp.y*1024)/1024;
}

#define NICE_PROPORTION	.4
void SplineCharDefaultNextCP(SplinePoint *base, SplinePoint *next) {
    SplinePoint *prev=NULL;
    real len;
    real angle, pangle, plen, ca;

    if ( next==NULL )
return;
    if ( !base->nextcpdef ) {
	if ( base->pointtype==pt_tangent )
	    SplineCharTangentNextCP(base);
return;
    }
    if ( base->prev!=NULL )
	prev = base->prev->from;

    len = NICE_PROPORTION * sqrt((base->me.x-next->me.x)*(base->me.x-next->me.x) +
	    (base->me.y-next->me.y)*(base->me.y-next->me.y));
    angle = atan2( base->me.y-next->me.y , base->me.x-next->me.x );
    base->nonextcp = false;

    if ( base->pointtype == pt_curve ) {
	if ( prev!=NULL && (base->prevcpdef || base->noprevcp)) {
	    pangle = atan2( prev->me.y-base->me.y , prev->me.x-base->me.x );
	    angle = (angle+pangle)/2;
	    plen = sqrt((base->prevcp.x-base->me.x)*(base->prevcp.x-base->me.x) +
		    (base->prevcp.y-base->me.y)*(base->prevcp.y-base->me.y));
	    ca = cos(angle);
	    if ( fabs(ca) > fabs(sin(angle)) )
		if (( ca>0 && base->me.x>prev->me.x ) || (ca<0 && base->me.x<prev->me.x))
		    angle += 3.1415926535897932;
	    base->prevcp.x = base->me.x + plen*cos(angle);
	    base->prevcp.y = base->me.y + plen*sin(angle);
	    SplineRefigure(base->prev);
	} else if ( prev!=NULL ) {
	    /* The prev control point is fixed. So we got to use the same */
	    /*  angle it uses */
	    angle = atan2( base->prevcp.y-base->me.y , base->prevcp.x-base->me.x );
	}
    } else if ( base->pointtype == pt_corner ) {
	if ( next->pointtype != pt_curve ) {
	    base->nonextcp = true;
	}
    } else /* tangent */ {
	if ( next->pointtype == pt_corner ) {
	    base->nonextcp = true;
	} else {
	    if ( prev!=NULL ) {
		if ( !base->noprevcp ) {
		    plen = sqrt((base->prevcp.x-base->me.x)*(base->prevcp.x-base->me.x) +
			    (base->prevcp.y-base->me.y)*(base->prevcp.y-base->me.y));
		    base->prevcp.x = base->me.x + plen*cos(angle);
		    base->prevcp.y = base->me.y + plen*sin(angle);
		    SplineRefigure(base->prev);
		}
		angle = atan2( prev->me.y-base->me.y , prev->me.x-base->me.x );
	    }
	}
    }
    if ( base->nonextcp )
	base->nextcp = base->me;
    else {
	base->nextcp.x = base->me.x - len*cos(angle);
	base->nextcp.y = base->me.y - len*sin(angle);
	base->nextcp.x = rint(base->nextcp.x*1024)/1024;
	base->nextcp.y = rint(base->nextcp.y*1024)/1024;
	if ( base->next != NULL )
	    SplineRefigure(base->next);
    }
}

void SplineCharDefaultPrevCP(SplinePoint *base, SplinePoint *prev) {
    SplinePoint *next=NULL;
    real len, nlen;
    real angle, nangle, ca;

    if ( prev==NULL )
return;
    if ( !base->prevcpdef ) {
	if ( base->pointtype==pt_tangent )
	    SplineCharTangentPrevCP(base);
return;
    }
    if ( base->next!=NULL )
	next = base->next->to;

    len = NICE_PROPORTION * sqrt((base->me.x-prev->me.x)*(base->me.x-prev->me.x) +
	    (base->me.y-prev->me.y)*(base->me.y-prev->me.y));
    angle = atan2( base->me.y-prev->me.y , base->me.x-prev->me.x );
    base->noprevcp = false;

    if ( base->pointtype == pt_curve ) {
	if ( next!=NULL && (base->nextcpdef || base->nonextcp)) {
	    nangle = atan2( next->me.y-base->me.y , next->me.x-base->me.x );
	    angle = (angle+nangle)/2;
	    nlen = sqrt((base->nextcp.x-base->me.x)*(base->nextcp.x-base->me.x) +
		    (base->nextcp.y-base->me.y)*(base->nextcp.y-base->me.y));
	    ca = cos(angle);
	    if ( fabs(ca) > fabs(sin(angle)) )
		if (( ca>0 && base->me.x>next->me.x ) || (ca<0 && base->me.x<next->me.x))
		    angle += 3.1415926535897932;
	    base->nextcp.x = base->me.x + nlen*cos(angle);
	    base->nextcp.y = base->me.y + nlen*sin(angle);
	    SplineRefigure(base->next);
	} else if ( next!=NULL ) {
	    /* The next control point is fixed. So we got to use the same */
	    /*  angle it uses */
	    angle = atan2( base->nextcp.y-base->me.y , base->nextcp.x-base->me.x );
	}
    } else if ( base->pointtype == pt_corner ) {
	if ( prev->pointtype != pt_curve ) {
	    base->noprevcp = true;
	}
    } else /* tangent */ {
	if ( prev->pointtype == pt_corner ) {
	    base->noprevcp = true;
	} else {
	    if ( next!=NULL ) {
		if ( !base->nonextcp ) {
		    nlen = sqrt((base->nextcp.x-base->me.x)*(base->nextcp.x-base->me.x) +
			    (base->nextcp.y-base->me.y)*(base->nextcp.y-base->me.y));
		    base->nextcp.x = base->me.x + nlen*cos(angle);
		    base->nextcp.y = base->me.y + nlen*sin(angle);
		    SplineRefigure(base->next);
		}
		angle = atan2( next->me.y-base->me.y , next->me.x-base->me.x );
	    }
	}
    }
    if ( base->noprevcp )
	base->prevcp = base->me;
    else {
	base->prevcp.x = base->me.x - len*cos(angle);
	base->prevcp.y = base->me.y - len*sin(angle);
	base->prevcp.x = rint(base->prevcp.x*1024)/1024;
	base->prevcp.y = rint(base->prevcp.y*1024)/1024;
	if ( base->prev!=NULL )
	    SplineRefigure(base->prev);
    }
}

void SplinePointListSet(SplinePointList *tobase, SplinePointList *frombase) {
    SplinePoint *tsp, *first, *fsp;
    Spline *tspline, *firstsp, *fspline;

    for ( ; tobase!=NULL && frombase!=NULL; tobase = tobase->next , frombase=frombase->next ) {
	first = NULL;
	for ( tsp = tobase->first, fsp=frombase->first; tsp!=first; tsp=tsp->next->to, fsp=fsp->next->to ) {
	    tsp->me = fsp->me;
	    tsp->nextcp = fsp->nextcp;
	    tsp->prevcp = fsp->prevcp;
	    tsp->noprevcp = fsp->noprevcp;
	    tsp->nonextcp = fsp->nonextcp;
	    if ( first==NULL ) first = tsp;
	    if ( tsp->next==NULL )
	break;
	}
	firstsp = NULL;
	for ( tspline = tobase->first->next, fspline=frombase->first->next;
		tspline!=NULL && tspline!=firstsp;
		tspline=tspline->to->next, fspline=fspline->to->next ) {
	    tspline->splines[0] = fspline->splines[0];
	    tspline->splines[1] = fspline->splines[1];
	    LinearApproxFree(tspline->approx);
	    tspline->approx = NULL;
	    if ( firstsp==NULL ) firstsp = tspline;
	}
    }
}

int PointListIsSelected(SplinePointList *spl) {
    int anypoints = 0;
    Spline *spline, *first;

    first = NULL;
    if ( spl->first->selected ) anypoints = true;
    for ( spline=spl->first->next; spline!=NULL && spline!=first && !anypoints; spline = spline->to->next ) {
	if ( spline->to->selected ) anypoints = true;
	if ( first == NULL ) first = spline;
    }
return( anypoints );
}

SplineSet *SplineSetReverse(SplineSet *spl) {
    Spline *spline, *first, *next;
    BasePoint tp;
    SplinePoint *temp;
    int bool;
    /* reverse the splineset so that what was the start point becomes the end */
    /*  and vice versa. This entails reversing every individual spline, and */
    /*  each point */

    first = NULL;
    spline = spl->first->next;
    if ( spline==NULL )
return( spl );			/* Only one point, reversal is meaningless */

    tp = spline->from->nextcp;
    spline->from->nextcp = spline->from->prevcp;
    spline->from->prevcp = tp;
    bool = spline->from->nonextcp;
    spline->from->nonextcp = spline->from->noprevcp;
    spline->from->noprevcp = bool;
    bool = spline->from->nextcpdef;
    spline->from->nextcpdef = spline->from->prevcpdef;
    spline->from->prevcpdef = bool;

    for ( ; spline!=NULL && spline!=first; spline=next ) {
	next = spline->to->next;

	if ( spline->to!=spl->first ) {		/* On a closed spline don't want to reverse the first point twice */
	    tp = spline->to->nextcp;
	    spline->to->nextcp = spline->to->prevcp;
	    spline->to->prevcp = tp;
	    bool = spline->to->nonextcp;
	    spline->to->nonextcp = spline->to->noprevcp;
	    spline->to->noprevcp = bool;
	    bool = spline->to->nextcpdef;
	    spline->to->nextcpdef = spline->to->prevcpdef;
	    spline->to->prevcpdef = bool;
	}

	temp = spline->to;
	spline->to = spline->from;
	spline->from = temp;
	spline->from->next = spline;
	spline->to->prev = spline;
	SplineRefigure(spline);
	if ( first==NULL ) first = spline;
    }

    if ( spl->first!=spl->last ) {
	temp = spl->first;
	spl->first = spl->last;
	spl->last = temp;
	spl->first->prev = NULL;
	spl->last->next = NULL;
    }
return( spl );
}

void SplineSetsUntick(SplineSet *spl) {
    Spline *spline, *first;
    
    while ( spl!=NULL ) {
	first = NULL;
	spl->first->isintersection = false;
	for ( spline=spl->first->next; spline!=first && spline!=NULL; spline = spline->to->next ) {
	    spline->isticked = false;
	    spline->isneeded = false;
	    spline->isunneeded = false;
	    spline->ishorvert = false;
	    spline->to->isintersection = false;
	    if ( first==NULL ) first = spline;
	}
	spl = spl->next;
    }
}

static void SplineSetTick(SplineSet *spl) {
    Spline *spline, *first;
    
    first = NULL;
    for ( spline=spl->first->next; spline!=first && spline!=NULL; spline = spline->to->next ) {
	spline->isticked = true;
	if ( first==NULL ) first = spline;
    }
}

static SplineSet *SplineSetOfSpline(SplineSet *spl,Spline *search) {
    Spline *spline, *first;
    
    while ( spl!=NULL ) {
	first = NULL;
	for ( spline=spl->first->next; spline!=first && spline!=NULL; spline = spline->to->next ) {
	    if ( spline==search )
return( spl );
	    if ( first==NULL ) first = spline;
	}
	spl = spl->next;
    }
return( NULL );
}

int SplineInSplineSet(Spline *spline, SplineSet *spl) {
    Spline *first, *s;

    first = NULL;
    for ( s = spl->first->next; s!=NULL && s!=first; s = s->to->next ) {
	if ( s==spline )
return( true );
	if ( first==NULL ) first = s;
    }
return( false );
}

#include "edgelist.h"

static void EdgeListReverse(EdgeList *es, SplineSet *spl) {
    int i;

    if ( es->edges!=NULL ) {
	for ( i=0; i<es->cnt; ++i ) {
	    Edge *e;
	    for ( e = es->edges[i]; e!=NULL; e = e->esnext ) {
		if ( SplineInSplineSet(e->spline,spl)) {
		    e->up = !e->up;
		    e->t_mmin = 1-e->t_mmin;
		    e->t_mmax = 1-e->t_mmax;
		    e->t_cur = 1-e->t_cur;
		}
	    }
	}
    }
}

static int SSCheck(SplineSet *base,Edge *active, int up, EdgeList *es) {
    SplineSet *spl;
    if ( active->spline->isticked )
return( 0 );
    spl = SplineSetOfSpline(base,active->spline);
    if ( active->up!=up ) {
	SplineSetReverse(spl);
	EdgeListReverse(es,spl);
    }
    SplineSetTick(spl);
return( 1 );
}

SplineSet *SplineSetsExtractOpen(SplineSet **tbase) {
    SplineSet *spl, *openhead=NULL, *openlast=NULL, *prev=NULL, *snext;

    for ( spl= *tbase; spl!=NULL; spl = snext ) {
	snext = spl->next;
	if ( spl->first->prev==NULL ) {
	    if ( prev==NULL )
		*tbase = snext;
	    else
		prev->next = snext;
	    if ( openhead==NULL )
		openhead = spl;
	    else
		openlast->next = spl;
	    openlast = spl;
	    spl->next = NULL;
	} else
	    prev = spl;
    }
return( openhead );
}

/* The idea behind SplineSetsCorrect is simple. However there are many splinesets */
/*  where it is impossible, so bear in mind that this only works for nice */
/*  splines. Figure 8's, interesecting splines all cause problems */
/* The outermost spline should be clockwise (up), the next splineset we find */
/*  should be down, if it isn't reverse it (if it's already been dealt with */
/*  then ignore it) */
SplineSet *SplineSetsCorrect(SplineSet *base) {
    SplineSet *spl;
    int sscnt, check_cnt;
    EdgeList es;
    DBounds b;
    Edge *active=NULL, *apt, *pr, *e;
    int i, winding;
    SplineSet *open, *tbase;

    tbase = base;
    open = SplineSetsExtractOpen(&tbase);
    base = tbase;

    SplineSetsUntick(base);
    for (sscnt=0,spl=base; spl!=NULL; spl=spl->next, ++sscnt );

    SplineSetFindBounds(base,&b);
    memset(&es,'\0',sizeof(es));
    es.scale = 1.0;
    es.mmin = floor(b.miny*es.scale);
    es.mmax = ceil(b.maxy*es.scale);
    es.omin = b.minx*es.scale;
    es.omax = b.maxx*es.scale;

/* Give up if we are given unreasonable values (ie. if rounding errors might screw us up) */
    if ( es.mmin<1e5 && es.mmax>-1e5 && es.omin<1e5 && es.omax>-1e5 ) {
	es.cnt = (int) (es.mmax-es.mmin) + 1;
	es.edges = gcalloc(es.cnt,sizeof(Edge *));
	es.interesting = gcalloc(es.cnt,sizeof(char));
	es.sc = NULL;
	es.major = 1; es.other = 0;
	FindEdgesSplineSet(base,&es);

	check_cnt = 0;
	for ( i=0; i<es.cnt && check_cnt<sscnt; ++i ) {
	    active = ActiveEdgesRefigure(&es,active,i);
	    if ( es.edges[i]!=NULL )
	continue;			/* Just too hard to get the edges sorted when we are at a start vertex */
	    if ( /*es.edges[i]==NULL &&*/ !es.interesting[i] &&
		    !(i>0 && es.interesting[i-1]) && !(i>0 && es.edges[i-1]!=NULL) &&
		    !(i<es.cnt-1 && es.edges[i+1]!=NULL) &&
		    !(i<es.cnt-1 && es.interesting[i+1]))	/* interesting things happen when we add (or remove) entries */
	continue;			/* and where we have points of inflection */
	    for ( apt=active; apt!=NULL; apt = e) {
		check_cnt += SSCheck(base,apt,true,&es);
		winding = apt->up?1:-1;
		for ( pr=apt, e=apt->aenext; e!=NULL && winding!=0; pr=e, e=e->aenext ) {
		    if ( !e->spline->isticked )
			check_cnt += SSCheck(base,e,winding<0,&es);
		    if ( pr->up!=e->up )
			winding += (e->up?1:-1);
		    else if ( (pr->before==e || pr->after==e ) &&
			    (( pr->mmax==i && e->mmin==i ) ||
			     ( pr->mmin==i && e->mmax==i )) )
			/* This just continues the line and doesn't change count */;
		    else
			winding += (e->up?1:-1);
		}
		/* color a horizontal line that comes out of the last vertex */
		if ( e!=NULL && (e->before==pr || e->after==pr) &&
			    (( pr->mmax==i && e->mmin==i ) ||
			     ( pr->mmin==i && e->mmax==i )) ) {
		    pr = e;
		    e = e->aenext;
		}
	    }
	}
	FreeEdges(&es);
    }
    if ( open==NULL )
	open = base;
    else {
	for ( spl=open; spl->next!=NULL; spl = spl->next );
	spl->next = base;
    }
return( open );
}

/* This is exactly the same as SplineSetsCorrect, but instead of correcting */
/*  problems we merely search for them and if we find any return the first */
SplineSet *SplineSetsDetectDir(SplineSet **_base,int *_lastscan) {
    SplineSet *spl, *ret, *base;
    EIList el;
    EI *active=NULL, *apt, *pr, *e;
    int i, winding,change,waschange;
    SplineSet *open;
    int lastscan = *_lastscan;
    SplineChar dummy;

    open = SplineSetsExtractOpen(_base);
    base = *_base;

    memset(&el,'\0',sizeof(el));
    memset(&dummy,'\0',sizeof(dummy));
    dummy.splines = base;
    ELFindEdges(&dummy,&el);
    el.major = 1;
    ELOrder(&el,el.major);

    ret = NULL;
    waschange = false;
    for ( i=0; i<el.cnt && ret==NULL; ++i ) {
	active = EIActiveEdgesRefigure(&el,active,i,1,&change);
	if ( i<=lastscan )
    continue;
	if ( el.ordered[i]!=NULL || el.ends[i] ) {
	    waschange = change;
    continue;			/* Just too hard to get the edges sorted when we are at a start vertex */
	}
	if ( !( waschange || change || el.ends[i] || el.ordered[i]!=NULL ||
		(i!=el.cnt-1 && (el.ends[i+1] || el.ordered[i+1]!=NULL)) ))
    continue;
	waschange = change;
	for ( apt=active; apt!=NULL && ret==NULL; apt = e) {
	    if ( EISkipExtremum(apt,i+el.low,1)) {
		e = apt->aenext->aenext;
	continue;
	    }
	    if ( !apt->up ) {
		ret = SplineSetOfSpline(base,active->spline);
	break;
	    }
	    winding = apt->up?1:-1;
	    for ( pr=apt, e=apt->aenext; e!=NULL && winding!=0; pr=e, e=e->aenext ) {
		if ( EISkipExtremum(e,i+el.low,1)) {
		    e = e->aenext;
	    continue;
		}
		if ( pr->up!=e->up ) {
		    if ( (winding<=0 && !e->up) || (winding>0 && e->up )) {
			ret = SplineSetOfSpline(base,active->spline);
		break;
		    }
		    winding += (e->up?1:-1);
		} else if ( EISameLine(pr,e,i+el.low,1) )
		    /* This just continues the line and doesn't change count */;
		else {
		    if ( (winding<=0 && !e->up) || (winding>0 && e->up )) {
			ret = SplineSetOfSpline(base,active->spline);
		break;
		    }
		    winding += (e->up?1:-1);
		}
	    }
	}
    }
    free(el.ordered);
    free(el.ends);
    ElFreeEI(&el);
    if ( open==NULL )
	open = base;
    else {
	for ( spl=open; spl->next!=NULL; spl = spl->next );
	spl->next = base;
    }
    *_base = open;
    *_lastscan = i;
return( ret );
}

int SplinePointListIsClockwise(SplineSet *spl) {
    EIList el;
    EI *active=NULL, *apt, *e;
    int i, change,waschange;
    SplineChar dummy;
    SplineSet *next;
    int ret = -1;

    if ( spl->first!=spl->last || spl->first->next == NULL )
return( -1 );		/* Open paths, (open paths with only one point are a special case) */

    memset(&el,'\0',sizeof(el));
    memset(&dummy,'\0',sizeof(dummy));
    dummy.splines = spl;
    next = spl->next; spl->next = NULL;
    ELFindEdges(&dummy,&el);
    el.major = 1;
    ELOrder(&el,el.major);

    waschange = false;
    for ( i=0; i<el.cnt && ret==-1; ++i ) {
	active = EIActiveEdgesRefigure(&el,active,i,1,&change);
	if ( el.ordered[i]!=NULL || el.ends[i] ) {
	    waschange = change;
    continue;			/* Just too hard to get the edges sorted when we are at a start vertex */
	}
	if ( !( waschange || change || el.ends[i] || el.ordered[i]!=NULL ||
		(i!=el.cnt-1 && (el.ends[i+1] || el.ordered[i+1]!=NULL)) ))
    continue;
	waschange = change;
	for ( apt=active; apt!=NULL && ret==-1; apt = e) {
	    if ( EISkipExtremum(apt,i+el.low,1)) {
		e = apt->aenext->aenext;
	continue;
	    }
	    ret = apt->up;
	break;
	}
    }
    free(el.ordered);
    free(el.ends);
    ElFreeEI(&el);
    spl->next = next;
return( ret );
}

#if 0
void SFFigureGrid(SplineFont *sf) {
    /* Look for any horizontal/vertical lines in the grid splineset */
    int hsnaps[40], hcnt=0, vsnaps[40], vcnt=0, i;
    SplineSet *ss;
    Spline *s, *first;

    for ( ss = sf->gridsplines; ss!=NULL; ss=ss->next ) {
	first = NULL;
	for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	    if ( s->knownlinear ) {
		if ( s->from->me.x==s->to->me.x && hcnt<40 )
		    hsnaps[hcnt++] = s->from->me.x;
		if ( s->from->me.y==s->to->me.y && vcnt<40 )
		    vsnaps[vcnt++] = s->from->me.y;
	    }
	    if ( first==NULL ) first = s;
	}
    }

    if ( sf->hsnaps!=NULL ) {
	for ( i=0; i<hcnt && sf->hsnaps[i]==hsnaps[i]; ++i );
	if ( i!=hcnt || sf->hsnaps[i]!=0x80000000 ) {
	    free( sf->hsnaps );
	    sf->hsnaps = NULL;
	}
    }
    if ( sf->vsnaps!=NULL ) {
	for ( i=0; i<vcnt && sf->vsnaps[i]==vsnaps[i]; ++i );
	if ( i!=vcnt || sf->vsnaps[i]!=0x80000000 ) {
	    free( sf->vsnaps );
	    sf->vsnaps = NULL;
	}
    }

    if ( hcnt!=0 && sf->hsnaps==NULL ) {
	sf->hsnaps = galloc((hcnt+1)*sizeof(int));
	memcpy(sf->hsnaps,hsnaps,hcnt*sizeof(int));
	sf->hsnaps[hcnt] = 0x80000000;
    }
    if ( vcnt!=0 && sf->vsnaps==NULL ) {
	sf->vsnaps = galloc((vcnt+1)*sizeof(int));
	memcpy(sf->vsnaps,vsnaps,vcnt*sizeof(int));
	sf->vsnaps[vcnt] = 0x80000000;
    }
}
#endif
