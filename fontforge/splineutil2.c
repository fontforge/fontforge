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

#include "splineutil2.h"

#include "autohint.h"
#include "cvundoes.h"
#include "edgelist.h"
#include "fontforge.h"
#include "gfile.h"
#include "gutils.h"
#include "namelist.h"
#include "psread.h"
#include "spiro.h"
#include "splinefill.h"
#include "splinefit.h"
#include "splineorder2.h"
#include "splineoverlap.h"
#include "splineutil.h"
#include "utanvec.h"
#include "ustring.h"
#include "views.h"		/* For SCCharChangedUpdate */

#include <math.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

int new_em_size = 1000;
int new_fonts_are_order2 = false;
int loaded_fonts_same_as_new = false;
int default_fv_row_count = 4;
int default_fv_col_count = 16;
int default_fv_font_size = 48;
int default_fv_antialias=true;
int default_fv_bbsized=false;
int snaptoint=0;

/*#define DEBUG	1*/

#if defined( FONTFORGE_CONFIG_USE_DOUBLE )
# define RE_NearZero	.00000001
# define RE_Factor	(1024.0*1024.0*1024.0*1024.0*1024.0*2.0) /* 52 bits => divide by 2^51 */
#else
# define RE_NearZero	.00001
# define RE_Factor	(1024.0*1024.0*4.0)	/* 23 bits => divide by 2^22 */
#endif

int Within4RoundingErrors(bigreal v1, bigreal v2) {
    bigreal temp=v1*v2;
    bigreal re;

    if ( temp<0 ) /* Ok, if the two values are on different sides of 0 there */
return( false );	/* is no way they can be within a rounding error of each other */
    else if ( temp==0 ) {
	if ( v1==0 )
return( v2<RE_NearZero && v2>-RE_NearZero );
	else
return( v1<RE_NearZero && v1>-RE_NearZero );
    } else if ( v1>0 ) {
	if ( v1>v2 ) {		/* Rounding error from the biggest absolute value */
	    re = v1/ (RE_Factor/4);
return( v1-v2 < re );
	} else {
	    re = v2/ (RE_Factor/4);
return( v2-v1 < re );
	}
    } else {
	if ( v1<v2 ) {
	    re = v1/ (RE_Factor/4);	/* This will be a negative number */
return( v1-v2 > re );
	} else {
	    re = v2/ (RE_Factor/4);
return( v2-v1 > re );
	}
    }
}

int Within16RoundingErrors(bigreal v1, bigreal v2) {
    bigreal temp=v1*v2;
    bigreal re;

    if ( temp<0 ) /* Ok, if the two values are on different sides of 0 there */
return( false );	/* is no way they can be within a rounding error of each other */
    else if ( temp==0 ) {
	if ( v1==0 )
return( v2<RE_NearZero && v2>-RE_NearZero );
	else
return( v1<RE_NearZero && v1>-RE_NearZero );
    } else if ( v1>0 ) {
	if ( v1>v2 ) {		/* Rounding error from the biggest absolute value */
	    re = v1/ (RE_Factor/16);
return( v1-v2 < re );
	} else {
	    re = v2/ (RE_Factor/16);
return( v2-v1 < re );
	}
    } else {
	if ( v1<v2 ) {
	    re = v1/ (RE_Factor/16);	/* This will be a negative number */
return( v1-v2 > re );
	} else {
	    re = v2/ (RE_Factor/16);
return( v2-v1 > re );
	}
    }
}

int Within64RoundingErrors(bigreal v1, bigreal v2) {
    bigreal temp=v1*v2;
    bigreal re;

    if ( temp<0 ) /* Ok, if the two values are on different sides of 0 there */
return( false );	/* is no way they can be within a rounding error of each other */
    else if ( temp==0 ) {
	if ( v1==0 )
return( v2<RE_NearZero && v2>-RE_NearZero );
	else
return( v1<RE_NearZero && v1>-RE_NearZero );
    } else if ( v1>0 ) {
	if ( v1>v2 ) {		/* Rounding error from the biggest absolute value */
	    re = v1/ (RE_Factor/64);
return( v1-v2 < re );
	} else {
	    re = v2/ (RE_Factor/64);
return( v2-v1 < re );
	}
    } else {
	if ( v1<v2 ) {
	    re = v1/ (RE_Factor/64);	/* This will be a negative number */
return( v1-v2 > re );
	} else {
	    re = v2/ (RE_Factor/64);
return( v2-v1 > re );
	}
    }
}

bool RealNear(real a,real b) {
    real d = a-b;
#ifdef FONTFORGE_CONFIG_USE_DOUBLE
    // These tighter equals-zero tests are retained for code tuned when
    // passing zero as a constant
    if ( a==0 )
	return b>-1e-8 && b<1e-8;
    if ( b==0 )
	return a>-1e-8 && a<1e-8;

    return d>-1e-6 && d<1e-6;
#else		/* For floats */
    return d>-1e-5 && d<1e-5
#endif
}

int RealNearish(real a,real b) {

    a-=b;
return( a<.001 && a>-.001 );
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

int RealRatio(real a,real b,real fudge) {

    if ( b==0 )
return( RealWithin(a,b,fudge));

return( RealWithin(a/b,1.0,fudge));
}

static int MinMaxWithin(Spline *spline) {
    extended dx, dy;
    int which;
    extended t1, t2;
    extended w;
    /* We know that this "spline" is basically one dimensional. As long as its*/
    /*  extrema are between the start and end points on that line then we can */
    /*  treat it as a line. If the extrema are way outside the line segment */
    /*  then it's a line that backtracks on itself */

    if ( (dx = spline->to->me.x - spline->from->me.x)<0 ) dx = -dx;
    if ( (dy = spline->to->me.y - spline->from->me.y)<0 ) dy = -dy;
    which = dx<dy;
    SplineFindExtrema(&spline->splines[which],&t1,&t2);
    if ( t1==-1 )
return( true );
    w = ((spline->splines[which].a*t1 + spline->splines[which].b)*t1
	     + spline->splines[which].c)*t1 + spline->splines[which].d;
    if ( RealNear(w, (&spline->to->me.x)[which]) || RealNear(w, (&spline->from->me.x)[which]) )
	/* Close enough */;
    else if ( (w<(&spline->to->me.x)[which] && w<(&spline->from->me.x)[which]) ||
	    (w>(&spline->to->me.x)[which] && w>(&spline->from->me.x)[which]) )
return( false );		/* Outside */

    w = ((spline->splines[which].a*t2 + spline->splines[which].b)*t2
	     + spline->splines[which].c)*t2 + spline->splines[which].d;
    if ( RealNear(w, (&spline->to->me.x)[which]) || RealNear(w, (&spline->from->me.x)[which]) )
	/* Close enough */;
    else if ( (w<(&spline->to->me.x)[which] && w<(&spline->from->me.x)[which]) ||
	    (w>(&spline->to->me.x)[which] && w>(&spline->from->me.x)[which]) )
return( false );		/* Outside */

return( true );
}

int SplineIsLinear(Spline *spline) {
    bigreal t1,t2, t3,t4;
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
	    RealNear(spline->from->me.x,spline->to->prevcp.x);
	if ( ret && ! ((spline->from->nextcp.y >= spline->from->me.y &&
		        spline->from->nextcp.y <= spline->to->me.y &&
		        spline->to->prevcp.y >= spline->from->me.y &&
		        spline->to->prevcp.y <= spline->to->me.y ) ||
		       (spline->from->nextcp.y <= spline->from->me.y &&
		        spline->from->nextcp.y >= spline->to->me.y &&
		        spline->to->prevcp.y <= spline->from->me.y &&
		        spline->to->prevcp.y >= spline->to->me.y )) )
	    ret = MinMaxWithin(spline);
    /* Horizontal lines */
    } else if ( RealNear(spline->from->me.y,spline->to->me.y) ) {
	ret = RealNear(spline->from->me.y,spline->from->nextcp.y) &&
	    RealNear(spline->from->me.y,spline->to->prevcp.y);
	if ( ret && ! ((spline->from->nextcp.x >= spline->from->me.x &&
		       spline->from->nextcp.x <= spline->to->me.x &&
		       spline->to->prevcp.x >= spline->from->me.x &&
		       spline->to->prevcp.x <= spline->to->me.x) ||
		      (spline->from->nextcp.x <= spline->from->me.x &&
		       spline->from->nextcp.x >= spline->to->me.x &&
		       spline->to->prevcp.x <= spline->from->me.x &&
		       spline->to->prevcp.x >= spline->to->me.x)) )
	    ret = MinMaxWithin(spline);
    } else {
	ret = true;
	t1 = (spline->from->nextcp.y-spline->from->me.y)/(spline->to->me.y-spline->from->me.y);
	t2 = (spline->from->nextcp.x-spline->from->me.x)/(spline->to->me.x-spline->from->me.x);
	t3 = (spline->to->me.y-spline->to->prevcp.y)/(spline->to->me.y-spline->from->me.y);
	t4 = (spline->to->me.x-spline->to->prevcp.x)/(spline->to->me.x-spline->from->me.x);
	ret = (Within16RoundingErrors(t1,t2) || (RealApprox(t1,0) && RealApprox(t2,0))) &&
		(Within16RoundingErrors(t3,t4) || (RealApprox(t3,0) && RealApprox(t4,0)));
	if ( ret ) {
	    if ( t1<0 || t2<0 || t3<0 || t4<0 ||
		    t1>1 || t2>1 || t3>1 || t4>1 )
		ret = MinMaxWithin(spline);
	}
    }
    spline->knowncurved = !ret;
    spline->knownlinear = ret;
    if ( ret ) {
	/* A few places that if the spline is knownlinear then its splines[?] */
	/*  are linear. So give the linear version and not that suggested by */
	/*  the control points */
	spline->splines[0].a = spline->splines[0].b = 0;
	spline->splines[0].d = spline->from->me.x;
	spline->splines[0].c = spline->to->me.x-spline->from->me.x;
	spline->splines[1].a = spline->splines[1].b = 0;
	spline->splines[1].d = spline->from->me.y;
	spline->splines[1].c = spline->to->me.y-spline->from->me.y;
    }
return( ret );
}

int SplineIsLinearMake(Spline *spline) {
    if ( SplineIsLinear(spline)) {
	spline->islinear = true;
	spline->from->nextcp = spline->from->me;
	if ( spline->from->nonextcp && spline->from->noprevcp )
	    spline->from->pointtype = pt_corner;
	else if ( spline->from->pointtype == pt_curve || spline->from->pointtype == pt_hvcurve )
	    spline->from->pointtype = pt_tangent;
	spline->to->prevcp = spline->to->me;
	if ( spline->to->nonextcp && spline->to->noprevcp )
	    spline->to->pointtype = pt_corner;
	else if ( spline->to->pointtype == pt_curve || spline->to->pointtype == pt_hvcurve )
	    spline->to->pointtype = pt_tangent;
	SplineRefigure(spline);
    }
return( spline->islinear );
}

int GoodCurve(SplinePoint *sp, int check_prev ) {
    bigreal dx, dy, lenx, leny;

    if ( sp->pointtype!=pt_curve && sp->pointtype!=pt_hvcurve )
return( false );
    if ( check_prev ) {
	dx = sp->me.x - sp->prevcp.x;
	dy = sp->me.y - sp->prevcp.y;
    } else {
	dx = sp->me.x - sp->nextcp.x;
	dy = sp->me.y - sp->nextcp.y;
    }
    /* If the cp is very close to the base point the point might as well be a corner */
    if ( dx<0 ) dx = -dx;
    if ( dy<0 ) dy = -dy;
    if ( dx+dy<1 )
return( false );

    if ( check_prev ) {
	if ( sp->prev==NULL )
return( true );
	lenx = sp->me.x - sp->prev->from->me.x;
	leny = sp->me.y - sp->prev->from->me.y;
    } else {
	if ( sp->next==NULL )
return( true );
	lenx = sp->me.x - sp->next->to->me.x;
	leny = sp->me.y - sp->next->to->me.y;
    }
    if ( lenx<0 ) lenx = -lenx;
    if ( leny<0 ) leny = -leny;
    if ( 50*(dx+dy) < lenx+leny )
return( false );

return( true );
}

/* calculating the actual length of a spline is hard, this gives a very */
/*  rough (but quick) approximation */
static bigreal SplineLenApprox(Spline *spline) {
    bigreal len, slen, temp;

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

bigreal SplineLength(Spline *spline) {
    /* I ignore the constant term. It's just an unneeded addition */
    bigreal len, t;
    bigreal lastx = 0, lasty = 0;
    bigreal curx, cury;

    len = 0;
    for ( t=1.0/128; t<=1.0001 ; t+=1.0/128 ) {
	curx = ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t;
	cury = ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t;
	len += sqrt( (curx-lastx)*(curx-lastx) + (cury-lasty)*(cury-lasty) );
	lastx = curx; lasty = cury;
    }
return( len );
}

bigreal SplineLengthRange(Spline *spline,real from_t, real to_t) {
    /* I ignore the constant term. It's just an unneeded addition */
    bigreal len, t;
    bigreal lastx = 0, lasty = 0;
    bigreal curx, cury;

    if ( from_t>to_t ) {
	real fubble = to_t;
	to_t = from_t;
	from_t = fubble;
    }

    lastx = ((spline->splines[0].a*from_t+spline->splines[0].b)*from_t+spline->splines[0].c)*from_t;
    lasty = ((spline->splines[1].a*from_t+spline->splines[1].b)*from_t+spline->splines[1].c)*from_t;
    len = 0;
    for ( t=from_t; t<to_t + 1.0/128 ; t+=1.0/128 ) {
	if ( t>to_t ) t = to_t;
	curx = ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t;
	cury = ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t;
	len += sqrt( (curx-lastx)*(curx-lastx) + (cury-lasty)*(cury-lasty) );
	lastx = curx; lasty = cury;
	if ( t==to_t )
    break;
    }
return( len );
}

bigreal PathLength(SplineSet *ss) {
    Spline *s, *first=NULL;
    bigreal len=0;

    for ( s=ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	len += SplineLength(s);
	if ( first==NULL )
	    first = s;
    }
return( len );
}

Spline *PathFindDistance(SplineSet *path,bigreal d,bigreal *_t) {
    Spline *s, *first=NULL, *last=NULL;
    bigreal len=0, diff;
    bigreal curx, cury;
    bigreal t;

    for ( s=path->first->next; s!=NULL && s!=first; s=s->to->next ) {
	bigreal lastx = 0, lasty = 0;
	for ( t=1.0/128; t<=1.0001 ; t+=1.0/128 ) {
	    curx = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t;
	    cury = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t;
	    diff = sqrt( (curx-lastx)*(curx-lastx) + (cury-lasty)*(cury-lasty) );
	    lastx = curx; lasty = cury;
	    if ( len+diff>=d ) {
		d -= len;
		*_t = t - (diff-d)/diff * (1.0/128);
		if ( *_t<0 ) *_t=0;
		if ( *_t>1 ) *_t = 1;
return( s );
	    }
	    len += diff;
	}
	if ( first==NULL )
	    first = s;
	last = s;
    }
    *_t = 1;
return( last );
}

static void SplinePointBindToPath(SplinePoint *sp,SplineSet *path) {
    Spline *s;
    bigreal t;
    BasePoint pos, slope, ntemp, ptemp;
    bigreal len;

    s = PathFindDistance(path,sp->me.x,&t);
    pos.x = ((s->splines[0].a*t + s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
    pos.y = ((s->splines[1].a*t + s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
    slope.x = (3*s->splines[0].a*t + 2*s->splines[0].b)*t+s->splines[0].c;
    slope.y = (3*s->splines[1].a*t + 2*s->splines[1].b)*t+s->splines[1].c;
    len = sqrt(slope.x*slope.x + slope.y*slope.y);
    if ( len!=0 ) {
	slope.x /= len;
	slope.y /= len;
    }

    /* Now I could find a separate transformation matrix for the control points*/
    /* but I think that would look odd (formerly smooth joints could become */
    /* discontiguous) so I use the same transformation for all */
    /* Except that doesn't work for order2 (I'll fix that later) */

    ntemp.x = (sp->nextcp.x-sp->me.x)*slope.x - (sp->nextcp.y-sp->me.y)*slope.y;
    ntemp.y = (sp->nextcp.x-sp->me.x)*slope.y + (sp->nextcp.y-sp->me.y)*slope.x;
    ptemp.x = (sp->prevcp.x-sp->me.x)*slope.x - (sp->prevcp.y-sp->me.y)*slope.y;
    ptemp.y = (sp->prevcp.x-sp->me.x)*slope.y + (sp->prevcp.y-sp->me.y)*slope.x;
    sp->me.x = pos.x - sp->me.y*slope.y;
    sp->me.y = pos.y + sp->me.y*slope.x;
    sp->nextcp.x = sp->me.x + ntemp.x;
    sp->nextcp.y = sp->me.y + ntemp.y;
    sp->prevcp.x = sp->me.x + ptemp.x;
    sp->prevcp.y = sp->me.y + ptemp.y;
}

static Spline *SplineBindToPath(Spline *s,SplineSet *path) {
    /* OK. The endpoints and the control points have already been moved. */
    /*  But the transformation is potentially non-linear, so figure some */
    /*  intermediate values, and then approximate a new spline based on them */
    FitPoint mids[3];
    bigreal t, pt,len;
    int i;
    BasePoint spos, pos;
    Spline *ps, *ret;

    for ( i=0, t=.25; i<3 ; t += .25, ++i ) {
	spos.x = ((s->splines[0].a*t + s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	spos.y = ((s->splines[1].a*t + s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
	ps = PathFindDistance(path,spos.x,&pt);
	pos.x = ((ps->splines[0].a*pt + ps->splines[0].b)*pt+ps->splines[0].c)*pt+ps->splines[0].d;
	pos.y = ((ps->splines[1].a*pt + ps->splines[1].b)*pt+ps->splines[1].c)*pt+ps->splines[1].d;
	mids[i].ut.x = (3*ps->splines[0].a*pt + 2*ps->splines[0].b)*pt+ps->splines[0].c;
	mids[i].ut.y = (3*ps->splines[1].a*pt + 2*ps->splines[1].b)*pt+ps->splines[1].c;
	len = sqrt(mids[i].ut.x*mids[i].ut.x + mids[i].ut.y*mids[i].ut.y);
	if ( len!=0 ) {
	    mids[i].ut.x /= len;
	    mids[i].ut.y /= len;
	}
	mids[i].t = t;
	mids[i].p.x = pos.x - spos.y*mids[i].ut.y;
	mids[i].p.y = pos.y + spos.y*mids[i].ut.x;
    }
    ret = ApproximateSplineFromPointsSlopes(s->from,s->to,mids,i,false,mt_matrix);
    SplineFree(s);
return( ret );
}

static void GlyphBindToPath(SplineSet *glyph,SplineSet *path) {
    /* Find the transformation for the middle of the glyph, and then rotate */
    /*  the entire thing by that */
    bigreal pt,len;
    BasePoint pos, slope;
    Spline *ps;
    DBounds b;
    real transform[6], offset[6], mid;

    SplineSetFindBounds(glyph,&b);
    mid = (b.minx+b.maxx)/2;
    ps = PathFindDistance(path,mid,&pt);
    pos.x = ((ps->splines[0].a*pt + ps->splines[0].b)*pt+ps->splines[0].c)*pt+ps->splines[0].d;
    pos.y = ((ps->splines[1].a*pt + ps->splines[1].b)*pt+ps->splines[1].c)*pt+ps->splines[1].d;
    slope.x = (3*ps->splines[0].a*pt + 2*ps->splines[0].b)*pt+ps->splines[0].c;
    slope.y = (3*ps->splines[1].a*pt + 2*ps->splines[1].b)*pt+ps->splines[1].c;
    len = sqrt(slope.x*slope.x + slope.y*slope.y);
    if ( len!=0 ) {
	slope.x /= len;
	slope.y /= len;
    }

    memset(offset,0,sizeof(offset));
    offset[0] = offset[3] = 1;
    offset[4] = -mid;

    transform[0] = transform[3] = slope.x;
    transform[1] = slope.y; transform[2] = -slope.y;
    transform[4] = pos.x;
    transform[5] = pos.y;
    MatMultiply(offset,transform,transform);
    SplinePointListTransform(glyph,transform,tpt_AllPoints);
}
    

SplineSet *SplineSetBindToPath(SplineSet *ss,int doscale, int glyph_as_unit,
	int align,real offset, SplineSet *path) {
    DBounds b;
    real transform[6];
    bigreal pathlength = PathLength(path);
    SplineSet *spl, *eog, *nextglyph;
    SplinePoint *sp;
    Spline *s, *first;
    int order2 = -1;
    BasePoint cp;

    memset(transform,0,sizeof(transform));
    transform[0] = transform[3] = 1;
    SplineSetFindBounds(ss,&b);
    
    if ( doscale && b.maxx-b.minx!=0 ) {
	transform[0] = transform[3] = pathlength/(b.maxx-b.minx);
	transform[4] = -b.minx;
    } else if ( align == 0 ) {	/* At start */
	transform[4] = -b.minx;
    } else if ( align == 1 ) {	/* Centered */
	transform[4] = (pathlength-(b.maxx-b.minx))/2 - b.minx;
    } else {			/* At end */
	transform[4] = pathlength - b.maxx;
    }
    if ( pathlength==0 ) {
	transform[4] += path->first->me.x;
	transform[5] += path->first->me.y;
    }
    transform[5] += offset;
    SplinePointListTransform(ss,transform,tpt_AllPoints);
    if ( pathlength==0 )
return( ss );

    if ( glyph_as_unit ) {
	for ( spl = ss; spl!=NULL ; spl=nextglyph ) {
	    for ( eog=spl; eog!=NULL && !eog->ticked; eog=eog->next );
	    if ( eog==NULL )
		nextglyph = NULL;
	    else {
		nextglyph = eog->next;
		eog->next = NULL;
	    }
	    GlyphBindToPath(spl,path);
	    if ( eog!=NULL )
		eog->next = nextglyph;
	}
    } else {
	for ( spl = ss; spl!=NULL ; spl=spl->next ) {
	    for ( sp = spl->first; ; ) {
		SplinePointBindToPath(sp,path);
		if ( sp->next==NULL )
	    break;
		order2 = sp->next->order2;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
	if ( order2==1 ) {
	    for ( spl = ss; spl!=NULL ; spl=spl->next ) {
		for ( sp = spl->first; ; ) {
		    if ( !sp->noprevcp && sp->prev!=NULL ) {
			if ( !IntersectLines(&cp,&sp->me,&sp->prevcp,&sp->prev->from->nextcp,&sp->prev->from->me)) {
			    cp.x = (sp->prevcp.x+sp->prev->from->nextcp.x)/2;
			    cp.y = (sp->prevcp.y+sp->prev->from->nextcp.y)/2;
			}
			sp->prevcp = sp->prev->from->nextcp = cp;
		    }
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==spl->first )
		break;
		}
	    }
	}

	for ( spl = ss; spl!=NULL ; spl=spl->next ) {
	    first = NULL;
	    for ( s=spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
		if ( s->order2 )
		    SplineRefigure2(s);
		else
		    s = SplineBindToPath(s,path);
		if ( first==NULL )
		    first = s;
	    }
	}
    }
return( ss );
}

static FitPoint *SplinesFigureFPsBetween(SplinePoint *from, SplinePoint *to,
	int *tot) {
    int cnt, i, j, pcnt;
    bigreal len, slen, lbase;
    SplinePoint *np;
    FitPoint *fp;
    bigreal _lens[10], *lens = _lens;
    int _cnts[10], *cnts = _cnts;
    /* I used just to give every spline 10 points. But that gave much more */
    /*  weight to small splines than to big ones */

    cnt = 0;
    for ( np = from->next->to; ; np = np->next->to ) {
	++cnt;
	if ( np==to )
    break;
    }
    if ( cnt>10 ) {
	lens = malloc(cnt*sizeof(bigreal));
	cnts = malloc(cnt*sizeof(int));
    }
    cnt = 0; len = 0;
    for ( np = from->next->to; ; np = np->next->to ) {
	lens[cnt] = SplineLenApprox(np->prev);
	len += lens[cnt];
	++cnt;
	if ( np==to )
    break;
    }
    if ( len!=0 ) {
	pcnt = 0;
	for ( i=0; i<cnt; ++i ) {
	    int pnts = rint( (10*cnt*lens[i])/len );
	    if ( pnts<2 ) pnts = 2;
	    cnts[i] = pnts;
	    pcnt += pnts;
	}
    } else
	pcnt = 2*cnt;

    fp = malloc((pcnt+1)*sizeof(FitPoint)); i = 0;
    if ( len==0 ) {
	for ( i=0; i<=pcnt; ++i ) {
	    fp[i].t = i/(pcnt);
	    fp[i].p.x = from->me.x;
	    fp[i].p.y = from->me.y;
	}
    } else {
	lbase = 0;
	for ( i=cnt=0, np = from->next->to; ; np = np->next->to, ++cnt ) {
	    slen = SplineLenApprox(np->prev);
	    for ( j=0; j<cnts[cnt]; ++j ) {
		bigreal t = j/(bigreal) cnts[cnt];
		fp[i].t = (lbase+ t*slen)/len;
		fp[i].p.x = ((np->prev->splines[0].a*t+np->prev->splines[0].b)*t+np->prev->splines[0].c)*t + np->prev->splines[0].d;
		fp[i++].p.y = ((np->prev->splines[1].a*t+np->prev->splines[1].b)*t+np->prev->splines[1].c)*t + np->prev->splines[1].d;
	    }
	    lbase += slen;
	    if ( np==to )
	break;
	}
    }
    if ( cnts!=_cnts ) free(cnts);
    if ( lens!=_lens ) free(lens);

    *tot = i;
	
return( fp );
}

static void SplinePointReCategorize(SplinePoint *sp,int oldpt) {
    SplinePointCategorize(sp);
    if ( sp->pointtype!=oldpt ) {
	if ( sp->pointtype==pt_curve && oldpt==pt_hvcurve &&
		((sp->nextcp.x == sp->me.x && sp->nextcp.y != sp->me.y ) ||
		 (sp->nextcp.y == sp->me.y && sp->nextcp.x != sp->me.x )))
	    sp->pointtype = pt_hvcurve;
    }
}

void SplinesRemoveBetween(SplineChar *sc, SplinePoint *from, SplinePoint *to,int type) {
    int tot;
    FitPoint *fp;
    SplinePoint *np, oldfrom;
    int oldfpt = from->pointtype, oldtpt = to->pointtype;
    Spline *sp;
    int order2 = from->next->order2;

    oldfrom = *from;
    fp = SplinesFigureFPsBetween(from,to,&tot);

    if ( type==1 )
	ApproximateSplineFromPointsSlopes(from,to,fp,tot-1,order2,mt_levien);
    else
	ApproximateSplineFromPoints(from,to,fp,tot-1,order2);

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
    
    free(fp);

    SplinePointReCategorize(from,oldfpt);
    SplinePointReCategorize(to,oldtpt);
}

static void RemoveZeroLengthSplines(SplineSet *spl, int onlyselected, bigreal bound) {
    SplinePoint *curp, *next, *prev;
    bigreal plen, nlen;

    bound *= bound;

    for ( curp = spl->first, prev=NULL; curp!=NULL ; curp=next ) {
	next = NULL;
	if ( curp->next!=NULL )
	    next = curp->next->to;
	if ( curp==next )	/* Once we've worked a contour down to a single point we can't do anything more here. Someone else will have to free the contour */
return;
	/* Zero length splines give us NaNs */
	if ( curp!=NULL && (curp->selected || !onlyselected) ) {
	    plen = nlen = 1e10;
	    if ( curp->prev!=NULL ) {
		plen = (curp->me.x-curp->prev->from->me.x)*(curp->me.x-curp->prev->from->me.x) +
			(curp->me.y-curp->prev->from->me.y)*(curp->me.y-curp->prev->from->me.y);
		if ( plen<=bound ) {
		    plen =
			sqrt( (curp->me.x-curp->prevcp.x)*(curp->me.x-curp->prevcp.x) +
				(curp->me.y-curp->prevcp.y)*(curp->me.y-curp->prevcp.y)) +
			sqrt( (curp->prevcp.x-curp->prev->from->nextcp.x)*(curp->prevcp.x-curp->prev->from->nextcp.x) +
				(curp->prevcp.y-curp->prev->from->nextcp.y)*(curp->prevcp.y-curp->prev->from->nextcp.y)) +
			sqrt( (curp->prev->from->nextcp.x-curp->prev->from->me.x)*(curp->prev->from->nextcp.x-curp->prev->from->me.x) +
				(curp->prev->from->nextcp.y-curp->prev->from->me.y)*(curp->prev->from->nextcp.y-curp->prev->from->me.y));
		    plen *= plen;
		}
	    }
	    if ( curp->next!=NULL ) {
		nlen = (curp->me.x-next->me.x)*(curp->me.x-next->me.x) +
			(curp->me.y-next->me.y)*(curp->me.y-next->me.y);
		if ( nlen<=bound ) {
		    nlen =
			sqrt( (curp->me.x-curp->nextcp.x)*(curp->me.x-curp->nextcp.x) +
				(curp->me.y-curp->nextcp.y)*(curp->me.y-curp->nextcp.y)) +
			sqrt( (curp->nextcp.x-curp->next->to->prevcp.x)*(curp->nextcp.x-curp->next->to->prevcp.x) +
				(curp->nextcp.y-curp->next->to->prevcp.y)*(curp->nextcp.y-curp->next->to->prevcp.y)) +
			sqrt( (curp->next->to->prevcp.x-curp->next->to->me.x)*(curp->next->to->prevcp.x-curp->next->to->me.x) +
				(curp->next->to->prevcp.y-curp->next->to->me.y)*(curp->next->to->prevcp.y-curp->next->to->me.y));
		    nlen *= nlen;
		}
	    }
	    if (( curp->prev!=NULL && plen<=bound && plen<nlen ) ||
		    (curp->next!=NULL && nlen<=bound && nlen<=plen )) {
		if ( curp->prev!=NULL && plen<=bound && plen<nlen ) {
		    SplinePoint *other = curp->prev->from;
		    other->nextcp = curp->nextcp;
		    other->nextcpdef = curp->nextcpdef;
		    other->next = curp->next;
		    if ( curp->next!=NULL ) other->next->from = other;
		    SplineFree(curp->prev);
		    SplineRefigure(other->next); // To set nonextcp correctly
		} else {
		    SplinePoint *other = next;
		    other->prevcp = curp->prevcp;
		    other->prevcpdef = curp->prevcpdef;
		    other->prev = curp->prev;
		    if ( curp->prev!=NULL ) other->prev->to = other;
		    SplineFree(curp->next);
		    SplineRefigure(other->prev); // To set noprevcp correctly
		}
		SplinePointFree(curp);
		if ( spl->first==curp ) {
		    spl->first = next;
		    spl->start_offset = 0;
		    if ( spl->last==curp )
			spl->last = next;
		} else if ( spl->last==curp )
		    spl->last = prev;
	    } else
		prev = curp;
	} else
	    prev = curp;
	if ( next==spl->first )
    break;
    }
}

SplineSet *SSRemoveZeroLengthSplines(SplineSet *base) {
    SplineSet *spl;

    for ( spl=base; spl!=NULL; spl=spl->next ) {
	RemoveZeroLengthSplines(spl,false,0);
	if ( spl->first->next!=NULL && spl->first->next->to==spl->first &&
		spl->first->nonextcp && spl->first->noprevcp ) {
	    /* Turn it into a single point, rather than a zero length contour */
	    chunkfree(spl->first->next,sizeof(Spline));
	    spl->first->next = spl->first->prev = NULL;
	}
    }
return( base );
}

static void RemoveStupidControlPoints(SplineSet *spl) {
    bigreal len, normal, dir;
    Spline *s, *first;
    BasePoint unit, off;

    /* Also remove really stupid control points: Tiny offsets pointing in */
    /*  totally the wrong direction. Some of the TeX fonts we get have these */
    /* We get equally bad results with a control point that points beyond the */
    /*  other end point */
    first = NULL;
    for ( s = spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
	unit.x = s->to->me.x-s->from->me.x;
	unit.y = s->to->me.y-s->from->me.y;
	len = sqrt(unit.x*unit.x+unit.y*unit.y);
	if ( len!=0 ) {
	    int refigure = false;
	    unit.x /= len; unit.y /= len;
	    if ( !s->from->nonextcp ) {
		off.x = s->from->nextcp.x-s->from->me.x;
		off.y = s->from->nextcp.y-s->from->me.y;
		if ((normal = off.x*unit.y - off.y*unit.x)<0 ) normal = -normal;
		dir = off.x*unit.x + off.y*unit.y;
		if (( normal<dir && normal<1 && dir<0 ) || (normal<.5 && dir<-.5) ||
			(normal<.1 && dir>len)) {
		    s->from->nextcp = s->from->me;
		    refigure = true;
		}
	    }
	    if ( !s->to->noprevcp ) {
		off.x = s->to->me.x - s->to->prevcp.x;
		off.y = s->to->me.y - s->to->prevcp.y;
		if ((normal = off.x*unit.y - off.y*unit.x)<0 ) normal = -normal;
		dir = off.x*unit.x + off.y*unit.y;
		if (( normal<-dir && normal<1 && dir<0 ) || (normal<.5 && dir>-.5 && dir<0) ||
			(normal<.1 && dir>len)) {
		    s->to->prevcp = s->to->me;
		    refigure = true;
		}
	    }
	    if ( refigure )
		SplineRefigure(s);
	}
	if ( first==NULL ) first = s;
    }
}

void SSRemoveStupidControlPoints(SplineSet *base) {
    SplineSet *spl;

    for (spl=base; spl!=NULL; spl=spl->next )
	RemoveStupidControlPoints(spl);
}

static void OverlapClusterCpAngles(SplineSet *spl,bigreal within) {
    bigreal len, nlen, plen;
    bigreal startoff, endoff;
    SplinePoint *sp, *nsp, *psp;
    BasePoint *nbp, *pbp;
    BasePoint pdir, ndir, fpdir, fndir;
    int nbad, pbad;

    /* Does this even work? Why do we dot ndir with the normal of pdir? !!!! */

    /* If we have a junction point (right mid of 3) where the two splines */
    /*  are almost, but not quite moving in the same direction as they go */
    /*  away from the point, and if there is a tiny overlap because of this */
    /*  "almost" same, then we will probably get a bit confused in remove */
    /*  overlap */

    if ( spl->first->next!=NULL && spl->first->next->order2 )
return;

    for ( sp = spl->first; ; ) {
	if ( sp->next==NULL )
    break;
	nsp = sp->next->to;
	if (( !sp->nonextcp || !sp->noprevcp ) && sp->prev!=NULL ) {
	    psp = sp->prev->from; 
	    nbp = !sp->nonextcp ? &sp->nextcp : !nsp->noprevcp ? &nsp->prevcp : &nsp->me;
	    pbp = !sp->noprevcp ? &sp->prevcp : !psp->nonextcp ? &psp->nextcp : &psp->me;

	    pdir.x = pbp->x-sp->me.x; pdir.y = pbp->y-sp->me.y;
	    ndir.x = nbp->x-sp->me.x; ndir.y = nbp->y-sp->me.y;
	    fpdir.x = psp->me.x-sp->me.x; fpdir.y = psp->me.y-sp->me.y;
	    fndir.x = nsp->me.x-sp->me.x; fndir.y = nsp->me.y-sp->me.y;

	    plen = sqrt(pdir.x*pdir.x+pdir.y*pdir.y);
	    if ( plen!=0 ) {
		pdir.x /= plen; pdir.y /= plen;
	    }

	    nlen = sqrt(ndir.x*ndir.x+ndir.y*ndir.y);
	    if ( nlen!=0 ) {
		ndir.x /= nlen; ndir.y /= nlen;
	    }

	    nbad = pbad = false;
	    if ( !sp->nonextcp && plen!=0 && nlen!=0 ) {
		len = sqrt(fndir.x*fndir.x+fndir.y*fndir.y);
		if ( len!=0 ) {
		    fndir.x /= len; fndir.y /= len;
		    startoff = ndir.x*pdir.y - ndir.y*pdir.x;
		    endoff = fndir.x*pdir.y - fndir.y*pdir.x;
		    if (( (startoff<0 && endoff>0) || (startoff>0 && endoff<0)) &&
			    startoff > -within && startoff < within )
			nbad = true;
		}
	    }
	    if ( !sp->noprevcp && plen!=0 && nlen!=0 ) {
		len = sqrt(fpdir.x*fpdir.x+fpdir.y*fpdir.y);
		if ( len!=0 ) {
		    fpdir.x /= len; fpdir.y /= len;
		    startoff = pdir.x*ndir.y - pdir.y*ndir.x;
		    endoff = fpdir.x*ndir.y - fpdir.y*ndir.x;
		    if (( (startoff<0 && endoff>0) || (startoff>0 && endoff<0)) &&
			    startoff > -within && startoff < within )
			pbad = true;
		}
	    }
	    if ( nbad && pbad ) {
		if ( ndir.x==0 || ndir.y==0 )
		    nbad = false;
		else if ( pdir.x==0 || pdir.y==0 )
		    pbad = false;
	    }
	    if ( nbad && pbad ) {
		if ( ndir.x*pdir.x + ndir.y*pdir.y > 0 ) {
		    ndir.x = pdir.x = (ndir.x + pdir.x)/2;
		    ndir.y = pdir.y = (ndir.x + pdir.x)/2;
		} else {
		    ndir.x = (ndir.x - pdir.x)/2;
		    ndir.y = (ndir.y - pdir.y)/2;
		    pdir.x = -ndir.x; pdir.y = -ndir.y;
		}
		sp->nextcp.x = sp->me.x + nlen*ndir.x;
		sp->nextcp.y = sp->me.y + nlen*ndir.y;
		sp->prevcp.x = sp->me.x + plen*pdir.x;
		sp->prevcp.y = sp->me.y + plen*pdir.y;
		SplineRefigure(sp->next); SplineRefigure(sp->prev);
	    } else if ( nbad ) {
		if ( ndir.x*pdir.x + ndir.y*pdir.y < 0 ) {
		    pdir.x = -pdir.x;
		    pdir.y = -pdir.y;
		}
		sp->nextcp.x = sp->me.x + nlen*pdir.x;
		sp->nextcp.y = sp->me.y + nlen*pdir.y;
		SplineRefigure(sp->next);
	    } else if ( pbad ) {
		if ( ndir.x*pdir.x + ndir.y*pdir.y < 0 ) {
		    ndir.x = -ndir.x;
		    ndir.y = -ndir.y;
		}
		sp->prevcp.x = sp->me.x + plen*ndir.x;
		sp->prevcp.y = sp->me.y + plen*ndir.y;
		SplineRefigure(sp->prev);
	    }
	}
	if ( nsp==spl->first )
    break;
	sp = nsp;
    }
}

void SSOverlapClusterCpAngles(SplineSet *base,bigreal within) {
    SplineSet *spl;

    for (spl=base; spl!=NULL; spl=spl->next )
	OverlapClusterCpAngles(spl,within);
}

static SplinePointList *SplinePointListMerge(SplineChar *sc, SplinePointList *spl,int type) {
    Spline *spline, *first;
    SplinePoint *nextp, *curp, *selectme;
    int all, any;


    /* If the entire splineset is selected, it should merge into oblivion */
    first = NULL;
    /* The python contour merge code calls with NULL, but that interface
       currently only works for bezier contours */
    if ( sc!=NULL && sc->inspiro && hasspiro() ) {
	int i,j;
	any = false; all = true;
	for ( i=0; i<spl->spiro_cnt-1; ++i )
	    if ( SPIRO_SELECTED(&spl->spiros[i]))
		any = true;
	    else
		all = false;
	if ( all )
return( NULL );
	else if ( any ) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		if ( SPIRO_SELECTED(&spl->spiros[i])) {
		    for ( j=i+1; j<spl->spiro_cnt ; ++j )
			spl->spiros[j-1] = spl->spiros[j];
		    --spl->spiro_cnt;
		}
	    SSRegenerateFromSpiros(spl);
	}
return( spl );
    }

    any = all = spl->first->selected;
    for ( spline = spl->first->next; spline!=NULL && spline!=first && all; spline=spline->to->next ) {
	if ( spline->to->selected ) any = true;
	else all = false;
	if ( first==NULL ) first = spline;
    }
    if ( spl->first->next!=NULL && spl->first->next->to==spl->first &&
	    spl->first->nonextcp && spl->first->noprevcp )
	all = true;		/* Merge away any splines which are just dots */
    if ( all )
return( NULL );			/* Some one else should free it and reorder the spline set list */
    RemoveZeroLengthSplines(spl,true,.3);

    if ( spl->first!=spl->last ) {
	/* If the spline isn't closed, then any selected points at the ends */
	/*  get deleted */
	while ( spl->first->selected ) {
	    nextp = spl->first->next->to;
	    SplineFree(spl->first->next);
	    SplinePointMDFree(sc,spl->first);
	    spl->first = nextp;
	    spl->start_offset = 0;
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
	    spl->start_offset = 0;
	    spl->last = spl->first;
	}
    }

    /* when we get here spl->first is not selected */
    if ( spl->first->selected ) IError( "spl->first is selected in SplinePointListMerge");
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
    if ( any )
	SplineSetSpirosClear(spl);
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

static int SPisExtremum(SplinePoint *sp) {
    BasePoint *prev, *next;
    SplinePoint *psp, *nsp;

    if ( sp->prev==NULL || sp->next==NULL )
return( true );

    nsp = sp->next->to;
    psp = sp->prev->from;

    /* A point that changes from curved to straight horizontal/vertical should*/
    /*  be treated as an extremum */
    if (( !sp->next->knownlinear && sp->prev->knownlinear &&
		(RealWithin(sp->me.x,sp->prev->from->me.x,.02) ||
		 RealWithin(sp->me.y,sp->prev->from->me.y,.02))) ||
	    ( !sp->prev->knownlinear && sp->next->knownlinear &&
		(RealWithin(sp->me.x,sp->next->to->me.x,.02) ||
		 RealWithin(sp->me.y,sp->next->to->me.y,.02))))
return( true );

    if ( sp->prev->knownlinear )
	prev = &psp->me;
    else if ( !sp->noprevcp )
	prev = &sp->prevcp;
    else
	prev = &psp->nextcp;
    if ( sp->next->knownlinear )
	next = &nsp->me;
    else if ( !sp->nonextcp )
	next = &sp->nextcp;
    else
	next = &nsp->prevcp;

    if ( sp->next->knownlinear && sp->prev->knownlinear &&
	    ((sp->me.x==nsp->me.x && sp->me.x==psp->me.x &&
	      ((sp->me.y<=nsp->me.y && psp->me.y<=sp->me.y) ||
	       (sp->me.y>=nsp->me.y && psp->me.y>=sp->me.y))) ||
	     (sp->me.y==nsp->me.y && sp->me.y==psp->me.y &&
	      ((sp->me.x<=nsp->me.x && psp->me.x<=sp->me.x) ||
	       (sp->me.x>=nsp->me.x && psp->me.x>=sp->me.x)))) )
return( false );	/* A point in the middle of a horizontal/vertical line */
			/*  is not an extrema and can be removed */

    if ( prev->x==sp->me.x && next->x==sp->me.x ) {
	if ( prev->y==sp->me.y && next->y==sp->me.y )
return( false );		/* this should be caught above */
	/* no matter what the control points look like this guy is either an */
	/*  an extremum or a point of inflection, so we don't need to check */
return( true );
    } else if ( prev->y==sp->me.y && next->y==sp->me.y ) {
return( true );
    } else if (( prev->x<=sp->me.x && next->x<=sp->me.x ) ||
	    (prev->x>=sp->me.x && next->x>=sp->me.x ))
return( true );
    else if (( prev->y<=sp->me.y && next->y<=sp->me.y ) ||
	    (prev->y>=sp->me.y && next->y>=sp->me.y ))
return( true );

return( false );
}

static bigreal SecondDerivative(Spline *s,bigreal t) {
    /* That is d2y/dx2, not d2y/dt2 */

    /* dy/dx = (dy/dt) / (dx/dt) */
    /* d2 y/dx2 = d(dy/dx)/dt / (dx/dt) */
    /* d2 y/dx2 = ((d2y/dt2)*(dx/dt) - (dy/dt)*(d2x/dt2))/ (dx/dt)^2 */

    /* dy/dt = 3 ay *t^2 + 2 by * t + cy */
    /* dx/dt = 3 ax *t^2 + 2 bx * t + cx */
    /* d2y/dt2 = 6 ay *t + 2 by */
    /* d2x/dt2 = 6 ax *t + 2 bx */
    bigreal dydt = (3*s->splines[1].a*t + 2*s->splines[1].b)*t + s->splines[1].c;
    bigreal dxdt = (3*s->splines[0].a*t + 2*s->splines[0].b)*t + s->splines[0].c;
    bigreal d2ydt2 = 6*s->splines[1].a*t + 2*s->splines[1].b;
    bigreal d2xdt2 = 6*s->splines[0].a*t + 2*s->splines[0].b;
    bigreal top = (d2ydt2*dxdt - dydt*d2xdt2);
    if ( dxdt==0 ) {
	if ( top==0 )
return( 0 );
	if ( top>0 )
return( 1e10 );
return( -1e10 );
    }

return( top/(dxdt*dxdt) );
}
    

/* Does the second derivative change sign around this point? If so we should */
/*  retain it for truetype */
static int SPisD2Change( SplinePoint *sp ) {
    bigreal d2next = SecondDerivative(sp->next,0);
    bigreal d2prev = SecondDerivative(sp->prev,1);

    if ( d2next>=0 && d2prev>=0 )
return( false );
    if ( d2next<=0 && d2prev<=0 )
return( false );

return( true );
}

/* Almost exactly the same as SplinesRemoveBetween, but this one is conditional */
/*  the intermediate points/splines are removed only if we have a good match */
/*  used for simplify */
static int SplinesRemoveBetweenMaybe(SplineChar *sc,
	SplinePoint *from, SplinePoint *to, int flags, bigreal err) {
    int i,tot;
    SplinePoint *afterfrom, *sp, *next;
    FitPoint *fp, *fp2;
    BasePoint test;
    int good;
    BasePoint fncp, tpcp;
    int fpt, tpt, fnoncp, tnopcp;
    int order2 = from->next->order2;

    afterfrom = from->next->to;
    fncp = from->nextcp; tpcp = to->prevcp;
    fnoncp = from->nonextcp; tnopcp = to->noprevcp;
    fpt = from->pointtype; tpt = to->pointtype;

    if ( afterfrom==to || from==to )
return( false );

    fp = SplinesFigureFPsBetween(from,to,&tot);
    fp2 = malloc((tot+1)*sizeof(FitPoint));
    memcpy(fp2,fp,tot*sizeof(FitPoint));

    if ( !(flags&sf_ignoreslopes) )
	ApproximateSplineFromPointsSlopes(from,to,fp,tot-1,order2,mt_levien);
    else {
	ApproximateSplineFromPoints(from,to,fp,tot-1,order2);
    }

    i = tot;

    good = true;
    while ( --i>0 && good ) {
	/* fp[0] is the same as from (easier to include it), but the SplineNear*/
	/*  routine will sometimes reject the end-points of the spline */
	/*  so just don't check it */
	test.x = fp2[i].p.x; test.y = fp2[i].p.y;
	good = SplineNearPoint(from->next,&test,err)!= -1;
    }

    free(fp);
    free(fp2);
    if ( good ) {
	SplineFree(afterfrom->prev);
	for ( sp=afterfrom; sp!=to; sp=next ) {
	    next = sp->next->to;
	    SplineFree(sp->next);
	    SplinePointMDFree(sc,sp);
	}
	if ( !SplineIsLinearMake(from->next) ) {
	    SplinePointCategorize(from);
	    SplinePointCategorize(to);
	}
    } else {
	SplineFree(from->next);
	from->next = afterfrom->prev;
	from->nextcp = fncp;
	from->nonextcp = fnoncp;
	from->pointtype = fpt;
	for ( sp=afterfrom; sp->next->to!=to; sp=sp->next->to );
	to->prev = sp->next;
	to->prevcp = tpcp;
	to->noprevcp = tnopcp;
	to->pointtype = tpt;
    }
return( good );
}

/* In truetype we can interpolate away an on curve point. Try this */
static int Spline2Interpolate(SplinePoint *mid, bigreal err) {
    SplinePoint *from, *to;
    BasePoint midme, test;
    int i,tot, good;
    FitPoint *fp;

    midme = mid->me;
    from = mid->prev->from; to = mid->next->to;
    fp = SplinesFigureFPsBetween(from,to,&tot);

    mid->me.x = (mid->prevcp.x + mid->nextcp.x)/2;
    mid->me.y = (mid->prevcp.y + mid->nextcp.y)/2;
    SplineRefigure(mid->next);
    SplineRefigure(mid->prev);

    i = tot;
    good = true;
    while ( --i>0 && good ) {
	/* fp[0] is the same as from (easier to include it), but the SplineNear*/
	/*  routine will sometimes reject the end-points of the spline */
	/*  so just don't check it */
	test.x = fp[i].p.x; test.y = fp[i].p.y;
	if ( i>tot/2 )
	    good = SplineNearPoint(mid->next,&test,err)!= -1 ||
		    SplineNearPoint(mid->prev,&test,err)!= -1;
	else
	    good = SplineNearPoint(mid->prev,&test,err)!= -1 ||
		    SplineNearPoint(mid->next,&test,err)!= -1;
    }
    if ( !good ) {
	mid->me = midme;
	SplineRefigure(mid->next);
	SplineRefigure(mid->prev);
    }
    free(fp);
return( good );
}

int SPInterpolate(const SplinePoint *sp) {
    /* Using truetype rules, can we interpolate this point? */
return( !sp->dontinterpolate && !sp->nonextcp && !sp->noprevcp &&
	    !sp->roundx && !sp->roundy &&
	    (RealWithin(sp->me.x,(sp->nextcp.x+sp->prevcp.x)/2,.1) &&
	     RealWithin(sp->me.y,(sp->nextcp.y+sp->prevcp.y)/2,.1)) );
}

static int _SplinesRemoveMidMaybe(SplineChar *sc,SplinePoint *mid, int flags,
	bigreal err, bigreal lenmax2) {
    SplinePoint *from, *to;

    if ( mid->prev==NULL || mid->next==NULL )
return( false );

    from = mid->prev->from; to = mid->next->to;

    /* Retain points which are horizontal or vertical, because postscript says*/
    /*  type1 fonts should always have a point at the extrema (except for small*/
    /*  things like serifs), and the extrema occur at horizontal/vertical points*/
    /* tt says something similar */
    if ( !(flags&sf_ignoreextremum) && SPisExtremum(mid) )
return( false );

    /* In truetype fonts we also want to retain points where the second */
    /*  derivative changes sign */
    if ( !(flags&sf_ignoreextremum) && mid->prev->order2 && SPisD2Change(mid) )
return( false );

    if ( !(flags&sf_mergelines) && (mid->pointtype==pt_corner ||
	    mid->prev->knownlinear || mid->next->knownlinear) ) {
	/* Be very careful about merging straight lines. Generally they should*/
	/*  remain straight... */
	/* Actually it's not that the lines are straight, the significant thing */
	/*  is that if there is a abrupt change in direction at this point */
	/*  (a corner) we don't want to get rid of it */
	BasePoint prevu, nextu;
	bigreal plen, nlen;

	if ( mid->next->knownlinear || mid->nonextcp ) {
	    nextu.x = to->me.x-mid->me.x;
	    nextu.y = to->me.y-mid->me.y;
	} else {
	    nextu.x = mid->nextcp.x-mid->me.x;
	    nextu.y = mid->nextcp.y-mid->me.y;
	}
	if ( mid->prev->knownlinear || mid->noprevcp ) {
	    prevu.x = from->me.x-mid->me.x;
	    prevu.y = from->me.y-mid->me.y;
	} else {
	    prevu.x = mid->prevcp.x-mid->me.x;
	    prevu.y = mid->prevcp.y-mid->me.y;
	}
	nlen = sqrt(nextu.x*nextu.x + nextu.y*nextu.y);
	plen = sqrt(prevu.x*prevu.x + prevu.y*prevu.y);
	if ( nlen==0 || plen==0 )
	    /* Not a real corner */;
	else if ( (nextu.x*prevu.x + nextu.y*prevu.y)/(nlen*plen)>((nlen+plen>20)?-.98:-.95) ) {
	    /* If the cos if the angle between the two segments is too far */
	    /*  from parallel then don't try to smooth the point into oblivion */
	    bigreal flen, tlen;
	    flen = (from->me.x-mid->me.x)*(from->me.x-mid->me.x) +
		    (from->me.y-mid->me.y)*(from->me.y-mid->me.y);
	    tlen = (to->me.x-mid->me.x)*(to->me.x-mid->me.x) +
		    (to->me.y-mid->me.y)*(to->me.y-mid->me.y);
	    if ( (flen<.7 && tlen<.7) || flen<.25 || tlen<.25 )
		/* Too short to matter */;
	    else
return( false );
	}

	if ( mid->prev->knownlinear && mid->next->knownlinear ) {
	    /* Special checks for horizontal/vertical lines */
	    /* don't let the smoothing distort them */
	    if ( from->me.x==to->me.x ) {
		if ( mid->me.x!=to->me.x )
return( false );
	    } else if ( from->me.y==to->me.y ) {
		if ( mid->me.y!=to->me.y )
return( false );
	    } else if ( !RealRatio((from->me.y-to->me.y)/(from->me.x-to->me.x),
				(mid->me.y-to->me.y)/(mid->me.x-to->me.x),
			        .05) ) {
return( false );
	    }
	} else if ( mid->prev->knownlinear ) {
	    if ( (mid->me.x-from->me.x)*(mid->me.x-from->me.x) + (mid->me.y-from->me.y)*(mid->me.y-from->me.y)
		    > lenmax2 )
return( false );
	} else {
	    if ( (mid->me.x-to->me.x)*(mid->me.x-to->me.x) + (mid->me.y-to->me.y)*(mid->me.y-to->me.y)
		    > lenmax2 )
return( false );
	}
    }

    if ( mid->next->order2 ) {
	if ( SPInterpolate(from) && SPInterpolate(to) && SPInterpolate(mid))
return( false );
    }

return ( SplinesRemoveBetweenMaybe(sc,from,to,flags,err));
}

/* A wrapper to SplinesRemoveBetweenMaybe to handle some extra checking for a */
/*  common case */
static int SplinesRemoveMidMaybe(SplineChar *sc,SplinePoint *mid, int flags,
	bigreal err, bigreal lenmax2) {
    int changed1 = false;
    if ( mid->next->order2 ) {
	if ( !mid->dontinterpolate && !mid->nonextcp && !mid->noprevcp &&
		!(RealWithin(mid->me.x,(mid->nextcp.x+mid->prevcp.x)/2,.1) &&
		  RealWithin(mid->me.y,(mid->nextcp.y+mid->prevcp.y)/2,.1)) )
	  changed1 = Spline2Interpolate(mid,err);
    }
return( _SplinesRemoveMidMaybe(sc,mid,flags,err,lenmax2) || changed1 );
}

void SPLNearlyHvCps(SplineChar *sc,SplineSet *ss,bigreal err) {
    Spline *s, *first=NULL;
    int refresh;
    SplinePoint *from, *to;

    for ( s = ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	refresh = false;
	from = s->from; to = s->to;
	if ( !from->nonextcp && from->nextcp.x-from->me.x<err && from->nextcp.x-from->me.x>-err ) {
	    from->nextcp.x = from->me.x;
	    if ( s->order2 ) to->prevcp = from->nextcp;
	    refresh = true;
	} else if ( !from->nonextcp && from->nextcp.y-from->me.y<err && from->nextcp.y-from->me.y>-err ) {
	    from->nextcp.y = from->me.y;
	    if ( s->order2 ) to->prevcp = from->nextcp;
	    refresh = true;
	}
	if ( !to->noprevcp && to->prevcp.x-to->me.x<err && to->prevcp.x-to->me.x>-err ) {
	    to->prevcp.x = to->me.x;
	    if ( s->order2 ) from->nextcp = to->prevcp;
	    refresh = true;
	} else if ( !to->noprevcp && to->prevcp.y-to->me.y<err && to->prevcp.y-to->me.y>-err ) {
	    to->prevcp.y = to->me.y;
	    if ( s->order2 ) from->nextcp = to->prevcp;
	    refresh = true;
	}
	if ( refresh )
	    SplineRefigure(s);
    }
}

void SPLNearlyHvLines(SplineChar *sc,SplineSet *ss,bigreal err) {
    Spline *s, *first=NULL;

    for ( s = ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	if ( s->knownlinear ) {
	    if ( s->to->me.x-s->from->me.x<err && s->to->me.x-s->from->me.x>-err ) {
		s->to->nextcp.x += (s->from->me.x-s->to->me.x);
		if ( s->order2 && s->to->next!=NULL )
		    s->to->next->to->prevcp.x = s->to->nextcp.x;
		s->to->me.x = s->from->me.x;
		s->to->prevcp = s->to->me;
		s->from->nextcp = s->from->me;
		SplineRefigure(s);
		if ( s->to->next != NULL )
		    SplineRefigure(s->to->next);
	    } else if ( s->to->me.y-s->from->me.y<err && s->to->me.y-s->from->me.y>-err ) {
		s->to->nextcp.y += (s->from->me.y-s->to->me.y);
		if ( s->order2 && s->to->next!=NULL )
		    s->to->next->to->prevcp.y = s->to->nextcp.y;
		s->to->me.y = s->from->me.y;
		s->to->prevcp = s->to->me;
		s->from->nextcp = s->from->me;
		SplineRefigure(s);
		if ( s->to->next != NULL )
		    SplineRefigure(s->to->next);
	    }
	}
    }
}

/* The older check below is based on an absolute distance
 * and is therefore magnification-relative. This one is
 * (more or less) hull-based with a ratio of 1000 visually
 * verified at multiple magnifications.
 */
int SplineIsLinearish(Spline *spline) {
    int i;
    real dmax = 0, dtmp, ldx, ldy, ln;
    BasePoint *sp, *ep, *cp;

    if ( SplineIsLinear(spline) )
	return 1;

    sp = &spline->from->me;
    ep = &spline->to->me;
    ldx = ep->x - sp->x;
    ldy = ep->y - sp->y;
    ln = sqrt(ldy*ldy + ldx*ldx);

    for (i = 0; i < 2; ++i) {
	if ( i==0 )
	    cp = &spline->from->nextcp;
	else
	    cp = &spline->to->prevcp;
	dtmp = fabs(ldy*cp->x - ldx*cp->y + ep->x*sp->y - ep->y*sp->x)/ln;
	if (dtmp > dmax)
	    dmax = dtmp;
    }
    return ( ln/dmax >= 1000 );
}

/* Does this spline deviate from a straight line between its endpoints by more*/
/*  than err?								      */
/* Rotate the spline so that the line between the endpoints is horizontal,    */
/*  then find the maxima/minima of the y spline (this is the deviation)	      */
/*  check that that is less than err					      */
static int SplineCloseToLinear(Spline *s, bigreal err) {
    bigreal angle;
    extended co,si, t1, t2, y;
    SplinePoint from, to;
    Spline sp;
    BasePoint bp;

    from = *s->from; to = *s->to;
    to.me.x -= from.me.x; to.me.y -= from.me.y;
    to.prevcp.x -= from.me.x; to.prevcp.y -= from.me.y;
    from.nextcp.x -= from.me.x ; from.nextcp.y -= from.me.y;
    from.me.x = from.me.y = 0;
    angle = atan2(to.me.y, to.me.x);

    si = sin(angle); co = cos(angle);
    bp.x =  to.me.x*co + to.me.y*si;
    bp.y = -to.me.x*si + to.me.y*co;
    to.me = bp;

    bp.x =  to.prevcp.x*co + to.prevcp.y*si;
    bp.y = -to.prevcp.x*si + to.prevcp.y*co;
    to.prevcp = bp;

    bp.x =  from.nextcp.x*co + from.nextcp.y*si;
    bp.y = -from.nextcp.x*si + from.nextcp.y*co;
    from.nextcp = bp;

    memset(&sp,0,sizeof(Spline));
    sp.from = &from; sp.to = &to;
    sp.order2 = s->order2;
    SplineRefigure(&sp);
    SplineFindExtrema(&sp.splines[1],&t1,&t2);

    if ( t1==-1 )
return( true );

    y = ((sp.splines[1].a*t1 + sp.splines[1].b)*t1 + sp.splines[1].c)*t1 + sp.splines[1].d;
    if ( y>err || y<-err )
return( false );

    if ( t2==-1 )
return( true );
    y = ((sp.splines[1].a*t2 + sp.splines[1].b)*t2 + sp.splines[1].c)*t2 + sp.splines[1].d;
return( y<=err && y>=-err );
}

int SPLNearlyLines(SplineChar *sc,SplineSet *ss,bigreal err) {
    Spline *s, *first=NULL;
    int changed = false;

    for ( s = ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	if ( s->islinear )
	    /* Nothing to be done */;
	else if ( s->knownlinear || SplineCloseToLinear(s,err)) {
	    s->from->nextcp = s->from->me;
	    s->to->prevcp = s->to->me;
	    SplineRefigure(s);
	    changed = true;
	}
    }
return( changed );
}

static void SPLForceLines(SplineChar *sc,SplineSet *ss,bigreal bump_size) {
    Spline *s, *first=NULL;
    SplinePoint *sp;
    int any;
    BasePoint unit;
    bigreal len, minlen = sc==NULL ? 50.0 : (sc->parent->ascent+sc->parent->descent)/20.0;
    bigreal diff, xoff, yoff, len2;
    int order2=false;

    if ( ss->first->next!=NULL && ss->first->next->order2 )
	order2 = true;

    for ( s = ss->first->next; s!=NULL && s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	if ( s->knownlinear ) {
	    unit.x = s->to->me.x-s->from->me.x;
	    unit.y = s->to->me.y-s->from->me.y;
	    len = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( len<minlen )
    continue;
	    unit.x /= len; unit.y /= len;
	    do {
		any = false;
		if ( s->from->prev!=NULL && s->from->prev!=s ) {
		    sp = s->from->prev->from;
		    len2 = sqrt((sp->me.x-s->from->me.x)*(sp->me.x-s->from->me.x) + (sp->me.y-s->from->me.y)*(sp->me.y-s->from->me.y));
		    diff = (sp->me.x-s->from->me.x)*unit.y - (sp->me.y-s->from->me.y)*unit.x;
		    if ( len2<len && fabs(diff)<=bump_size ) {
			xoff = diff*unit.y; yoff = -diff*unit.x;
			sp->me.x -= xoff; sp->me.y -= yoff;
			sp->prevcp.x -= xoff; sp->prevcp.y -= yoff;
			if ( order2 && sp->prev!=NULL && !sp->noprevcp )
			    sp->prev->from->nextcp = sp->prevcp;
			sp->nextcp = sp->me;
			if ( sp->next==first ) first = NULL;
			SplineFree(sp->next);
			if ( s->from==ss->first ) {
			    if ( ss->first==ss->last ) ss->last = sp;
			    ss->first = sp;
			    ss->start_offset = 0;
			}
			SplinePointMDFree(sc,s->from);
			sp->next = s; s->from = sp;
			SplineRefigure(s);
			if ( sp->prev!=NULL )
			    SplineRefigure(sp->prev);
			sp->pointtype = pt_corner;
			any = true;

			/* We must recalculate the length each time, we */
			/*  might have shortened it. */
			unit.x = s->to->me.x-s->from->me.x;
			unit.y = s->to->me.y-s->from->me.y;
			len = sqrt(unit.x*unit.x + unit.y*unit.y);
			if ( len<minlen )
	    break;
			unit.x /= len; unit.y /= len;
		    }
		}
		if ( s->to->next!=NULL && s->to->next!=s ) {
		    sp = s->to->next->to;
		    /* If the next spline is a longer line than we are, then don't */
		    /*  merge it to us, rather merge us into it next time through the loop */
		    /* Hmm. Don't merge out the bump in general if the "bump" is longer than we are */
		    len2 = sqrt((sp->me.x-s->to->me.x)*(sp->me.x-s->to->me.x) + (sp->me.y-s->to->me.y)*(sp->me.y-s->to->me.y));
		    diff = (sp->me.x-s->to->me.x)*unit.y - (sp->me.y-s->to->me.y)*unit.x;
		    if ( len2<len && fabs(diff)<=bump_size ) {
			xoff = diff*unit.y; yoff = -diff*unit.x;
			sp->me.x -= xoff; sp->me.y -= yoff;
			sp->nextcp.x -= xoff; sp->nextcp.y -= yoff;
			if ( order2 && sp->next!=NULL && !sp->nonextcp )
			    sp->next->to->prevcp = sp->nextcp;
			sp->prevcp = sp->me;
			if ( sp->prev==first ) first = NULL;
			SplineFree(sp->prev);
			if ( s->to==ss->last ) {
			    if ( ss->first==ss->last ) {
			      ss->first = sp;
			      ss->start_offset = 0;
			    }
			    ss->last = sp;
			}
			SplinePointMDFree(sc,s->to);
			sp->prev = s; s->to = sp;
			SplineRefigure(s);
			if ( sp->next!=NULL )
			    SplineRefigure(sp->next);
			sp->pointtype = pt_corner;
			any = true;

			unit.x = s->to->me.x-s->from->me.x;
			unit.y = s->to->me.y-s->from->me.y;
			len = sqrt(unit.x*unit.x + unit.y*unit.y);
			if ( len<minlen )
	    break;
			unit.x /= len; unit.y /= len;
		    }
		}
	    } while ( any );
	}
    }
}

static int SPLSmoothControlPoints(SplineSet *ss,bigreal tan_bounds,int vert_check) {
    SplinePoint *sp;
    /* If a point has control points, and if those cps are in nearly the same */
    /*  direction (within tan_bounds) then adjust them so that they are in the*/
    /*  same direction */
    BasePoint unit, unit2;
    bigreal len, len2, para, norm, tn;
    int changed=false, found;

    if ( ss->first->next!=NULL && ss->first->next->order2 )
return( false );

    for ( sp = ss->first; ; ) {
	if (( !sp->nonextcp && !sp->noprevcp && sp->pointtype==pt_corner ) ||
		((sp->pointtype==pt_corner || sp->pointtype==pt_curve || sp->pointtype==pt_hvcurve) &&
		 (( !sp->nonextcp && sp->noprevcp && sp->prev!=NULL && sp->prev->knownlinear ) ||
		  ( !sp->noprevcp && sp->nonextcp && sp->next!=NULL && sp->next->knownlinear )))) {
	    BasePoint *next = sp->nonextcp ? &sp->next->to->me : &sp->nextcp;
	    BasePoint *prev = sp->noprevcp ? &sp->prev->to->me : &sp->prevcp;
	    unit.x = next->x-sp->me.x;
	    unit.y = next->y-sp->me.y;
	    len = sqrt(unit.x*unit.x + unit.y*unit.y);
	    unit.x /= len; unit.y /= len;
	    para = (sp->me.x-prev->x)*unit.x + (sp->me.y-prev->y)*unit.y;
	    norm = (sp->me.x-prev->x)*unit.y - (sp->me.y-prev->y)*unit.x;
	    if ( para==0 )
		tn = 1000;
	    else
		tn = norm/para;
	    if ( tn<0 ) tn = -tn;
	    if ( tn<tan_bounds && para>0 ) {
		found = 0;
		unit2.x = sp->me.x-sp->prevcp.x;
		unit2.y = sp->me.y-sp->prevcp.y;
		len2 = sqrt(unit2.x*unit2.x + unit2.y*unit2.y);
		unit2.x /= len2; unit2.y /= len2;
		if ( vert_check ) {
		    if ( fabs(unit.x)>fabs(unit.y) ) {
			/* Closer to horizontal */
			if ( (unit.y<=0 && unit2.y>=0) || (unit.y>=0 && unit2.y<=0) ) {
			    unit2.x = unit2.x<0 ? -1 : 1; unit2.y = 0;
			    found = 1;
			}
		    } else {
			if ( (unit.x<=0 && unit2.x>=0) || (unit.x>=0 && unit2.x<=0) ) {
			    unit2.y = unit2.y<0 ? -1 : 1; unit2.x = 0;
			    found = 1;
			}
		    }
		}
		/* If we're next to a line, we must extend the line. No choice */
		if ( sp->nonextcp ) {
		    if ( len<len2 )
    goto nextpt;
		    found = true;
		    unit2 = unit;
		} else if ( sp->noprevcp ) {
		    if ( len2<len )
    goto nextpt;
		    found = true;
		} else if ( !found ) {
		    unit2.x = (unit.x*len + unit2.x*len2)/(len+len2);
		    unit2.y = (unit.y*len + unit2.y*len2)/(len+len2);
		}
		sp->nextcp.x = sp->me.x + len*unit2.x;
		sp->nextcp.y = sp->me.y + len*unit2.y;
		sp->prevcp.x = sp->me.x - len2*unit2.x;
		sp->prevcp.y = sp->me.y - len2*unit2.y;
		sp->pointtype = pt_curve;
		if ( sp->prev )
		    SplineRefigure(sp->prev);
		if ( sp->next )
		    SplineRefigure(sp->next);
		changed = true;
	    }
	}
    nextpt:
	if ( sp->next==NULL )
    break;
	sp = sp->next->to;
	if ( sp==ss->first )
    break;
    }
return( changed );
}

static void GetNextUnitVector(SplinePoint *sp,BasePoint *uv) {
    bigreal len;

    if ( sp->next==NULL ) {
	uv->x = uv->y = 0;
    } else if ( sp->next->knownlinear ) {
	uv->x = sp->next->to->me.x - sp->me.x;
	uv->y = sp->next->to->me.y - sp->me.y;
    } else if ( sp->nonextcp ) {
	uv->x = sp->next->to->prevcp.x - sp->me.x;
	uv->y = sp->next->to->prevcp.y - sp->me.y;
    } else {
	uv->x = sp->nextcp.x - sp->me.x;
	uv->y = sp->nextcp.y - sp->me.y;
    }
    len = sqrt(uv->x*uv->x + uv->y*uv->y );
    if ( len!= 0 ) {
	uv->x /= len;
	uv->y /= len;
    }
}

static int isExtreme(SplinePoint *sp) {
    if ( sp->next==NULL || sp->prev==NULL )
return( 2 );		/* End point. Always extreme */
    if ( sp->nonextcp ) {
	if ( !sp->next->knownlinear )
return( false );
	if ( sp->next->to->me.x == sp->me.x || sp->next->to->me.y == sp->me.y )
	    /* Ok, horizontal or vertical line, that'll do */;
	else
return( false );
    } else {
	if ( sp->nextcp.x != sp->me.x && sp->nextcp.y != sp->me.y )
return( false );
    }
    if ( sp->noprevcp ) {
	if ( !sp->prev->knownlinear )
return( false );
	if ( sp->prev->from->me.x == sp->me.x || sp->prev->from->me.y == sp->me.y )
	    /* Ok, horizontal or vertical line, that'll do */;
	else
return( false );
    } else {
	if ( sp->prevcp.x != sp->me.x && sp->prevcp.y != sp->me.y )
return( false );
    }
return( true );
}

/* If the start point of a contour is not at an extremum, and the contour has */
/*  a point which is at an extremum, then make the start point be that point  */
/* leave it unchanged if start point is already extreme, or no extreme point  */
/*  could be found							      */
static void SPLStartToExtremum(SplineChar *sc,SplinePointList *spl) {
    SplinePoint *sp;

    while ( spl!=NULL ) {
	if ( spl->first==spl->last ) {		/* It's closed */
	    for ( sp=spl->first; !isExtreme(sp); ) {
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	    if ( sp!=spl->first ) {
		spl->first = spl->last = sp;
		spl->start_offset = 0;
	    }
	}
	spl = spl->next;
    }
}

/* Cleanup just turns splines with control points which happen to trace out */
/*  lines into simple lines */
/* it also checks for really nasty control points which point in the wrong */
/*  direction but are very close to the base point. We get these from some */
/*  TeX fonts. I assume they are due to rounding errors (or just errors) in*/
/*  some autotracer */
void SplinePointListSimplify(SplineChar *sc,SplinePointList *spl,
	struct simplifyinfo *smpl) {
    SplinePoint *first, *next, *sp, *nsp;
    BasePoint suv, nuv;
    bigreal lenmax2 = smpl->linelenmax*smpl->linelenmax;

    if ( spl==NULL )
return;

    RemoveZeroLengthSplines(spl,false,0.1);
    RemoveStupidControlPoints(spl);
    SSRemoveBacktracks(spl);
    if ( smpl->flags!=sf_cleanup && (smpl->flags&sf_setstart2extremum))
	SPLStartToExtremum(sc,spl);
    if ( spl->first->next!=NULL && spl->first->next->to==spl->first &&
	    spl->first->nonextcp && spl->first->noprevcp )
return;		/* Ignore any splines which are just dots */

    if ( smpl->flags!=sf_cleanup && (smpl->flags&sf_forcelines)) {
	SPLNearlyHvLines(sc,spl,smpl->linefixup);
	SPLForceLines(sc,spl,smpl->linefixup);
    }

    if ( smpl->flags!=sf_cleanup && spl->first->prev!=NULL && spl->first->prev!=spl->first->next ) {
	/* first thing to try is to remove everything between two extrema */
	/* We do this even if they checked ignore extrema. After this pass */
	/*  we'll come back and check every point individually */
	/* However, if we turn through more than 90 degrees we can't approximate */
	/*  a good match, and it takes us forever to make the attempt and fail*/
	/*  We take a dot product to prevent that */
	for ( sp = spl->first; ; ) {
	    if ( sp->next==NULL )
	break;
	    if ( SPisExtremum(sp) ) {
		GetNextUnitVector(sp,&suv);
		for ( nsp=sp->next->to; nsp!=sp; nsp = nsp->next->to ) {
		    if ( nsp->next==NULL )
		break;
		    if ( nsp->prev->knownlinear &&
			    (nsp->me.x-nsp->prev->from->me.x)*(nsp->me.x-nsp->prev->from->me.x) +
			    (nsp->me.y-nsp->prev->from->me.y)*(nsp->me.y-nsp->prev->from->me.y)
			    >= lenmax2 )
	      goto nogood;
		    GetNextUnitVector(nsp,&nuv);
		    if ( suv.x*nuv.x + suv.y*nuv.y < 0 ) {
			if ( suv.x*nuv.x + suv.y*nuv.y > -.1 )
		break;
	      goto nogood;
		    }
		    if ( SPisExtremum(nsp) || nsp==spl->first)
		break;
		}
		/* nsp is something we don't want to remove */
		if ( nsp==sp )
	break;
		else if ( sp->next->to == nsp )
	      goto nogood;		/* Nothing to remove */
		if ( SplinesRemoveBetweenMaybe(sc,sp,nsp,smpl->flags,smpl->err)) {
		    if ( spl->last==spl->first ) {
			spl->last = spl->first = sp;	/* We know this point didn't get removed */
			spl->start_offset = 0;
		    }
		}
	      nogood:
		sp = nsp;
	    } else
		sp = sp->next->to;
	    if ( sp == spl->first )
	break;
	}

	while ( 1 ) {
	    first = spl->first->prev->from;
	    if ( first->prev == first->next )
return;
	    if ( !SplinesRemoveMidMaybe(sc,spl->first,smpl->flags,smpl->err,lenmax2))
	break;
	    if ( spl->first==spl->last )
		spl->last = first;
	    spl->first = first;
	    spl->start_offset = 0;
	}
    }

	/* Special case checks for paths containing only one point */
	/*  else we get lots of nans (or only two points) */
    if ( spl->first->next == NULL )
return;
    for ( sp = spl->first->next->to; sp->next!=NULL; sp = next ) {
	SplineIsLinearMake(sp->prev);		/* First see if we can turn it*/
				/* into a line, then try to merge two splines */
	next = sp->next->to;
	if ( sp->prev == sp->next ||
		(sp->next!=NULL && sp->next->to->next!=NULL &&
		    sp->next->to->next->to == sp ))
return;
	if ( smpl->flags!=sf_cleanup ) {
	    if ( SplinesRemoveMidMaybe(sc,sp,smpl->flags,smpl->err,lenmax2) ) {
		if ( spl->first==sp ) {
		    spl->first = next;
		    spl->start_offset = 0;
		}
		if ( spl->last==sp )
		    spl->last = next;
    continue;
	    }
	} else {
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
		sp->nextcpdef = next->nextcpdef;
		SplinePointMDFree(sc,next);
		if ( sp->next!=NULL ) {
		    SplineRefigure(sp->next); // To set nonextcp accurately
		    next = sp->next->to;
		} else {
		    next = NULL;
	    break;
		}
	    }
	    if ( next==NULL )
    break;
	}
	if ( next->prev!=NULL && next->prev->from==spl->last )
    break;
    }
    if ( smpl->flags!=sf_cleanup && (smpl->flags&sf_smoothcurves))
	SPLSmoothControlPoints(spl,smpl->tan_bounds,smpl->flags&sf_choosehv);
}

/* cleanup may be: -1 => lines become lines, 0 => simplify & retain slopes, 1=> simplify and discard slopes, 2=>discard extrema */
SplineSet *SplineCharSimplify(SplineChar *sc,SplineSet *head,
	struct simplifyinfo *smpl) {
    SplineSet *spl, *prev, *snext;
    int anysel=0, wassingleton;

    if ( smpl->check_selected_contours ) {
	for ( spl = head; spl!=NULL && !anysel; spl = spl->next ) {
	    anysel = PointListIsSelected(spl);
	}
    }

    prev = NULL;
    for ( spl = head; spl!=NULL; spl = snext ) {
	snext = spl->next;
	if ( !anysel || PointListIsSelected(spl)) {
	    wassingleton = spl->first->prev==spl->first->next &&
		    (spl->first->prev==NULL ||
		     (spl->first->noprevcp && spl->first->nonextcp));
	    SplinePointListSimplify(sc,spl,smpl);
	    /* remove any singleton points */
	    if (( !wassingleton ||
		    (smpl->flags!=sf_cleanup && (smpl->flags&sf_rmsingletonpoints))) &&
		   spl->first->prev==spl->first->next &&
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
    SplineSetsRemoveAnnoyingExtrema(head,.3);
    SPLCategorizePoints(head);
    /* printf( "nocnt=%d totcnt=%d curdif=%d incr=%d\n", nocnt_cnt, totcnt_cnt, curdiff_cnt, incr_cnt ); */ /* Debug!!! */
return( head );
}

/* If the start point of a contour to be the left most point on it.  If there */
/*  are several points with that same value, use the one closest to the base */
/*  line */
void SPLStartToLeftmost(SplineChar *sc,SplinePointList *spl, int *changed) {
    SplinePoint *sp;
    SplinePoint *best;

    if ( spl->first==spl->last ) {		/* It's closed */
	best = spl->first;
	for ( sp=spl->first; ; ) {
	    if ( sp->me.x < best->me.x ||
		    (sp->me.x==best->me.x && (fabs(sp->me.y)<fabs(best->me.y))) )
		best = sp;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	if ( best!=spl->first ) {
	    if ( !*changed ) {
		SCPreserveState(sc,false);
		*changed = true;
	    }
	    SplineSetSpirosClear(spl);
	    spl->first = spl->last = best;
	    spl->start_offset = 0;
	}
    }
}

void SPLsStartToLeftmost(SplineChar *sc,int layer) {
    int changed = 0;
    SplineSet *ss;

    if ( sc==NULL )
return;

    if ( sc->parent->multilayer ) {
	for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	    for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next )
		SPLStartToLeftmost(sc,ss,&changed);
	}
	if ( changed )
	    SCCharChangedUpdate(sc,ly_all);
    } else {
	for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next )
	    SPLStartToLeftmost(sc,ss,&changed);
	if ( changed )
	    SCCharChangedUpdate(sc,layer);
    }
}

struct contourinfo {
    SplineSet *ss;
    BasePoint *min;
};

static int order_contours(const void *_c1, const void *_c2) {
    const struct contourinfo *c1 = _c1, *c2 = _c2;

    if ( c1->min->x<c2->min->x )
return( -1 );
    else if ( c1->min->x>c2->min->x )
return( 1 );
    else if ( fabs(c1->min->y)<fabs(c2->min->y) )
return( -1 );
    else if ( fabs(c1->min->y)>fabs(c2->min->y) )
return( 1 );
    else
return( 0 );
}

void CanonicalContours(SplineChar *sc,int layer) {
    int changed;
    SplineSet *ss;
    SplinePoint *sp, *best;
    int contour_cnt, contour_max=0, i, diff;
    struct contourinfo *ci;

    if ( sc==NULL )
return;

    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	contour_cnt = 0;
	for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next )
	    ++contour_cnt;
	if ( contour_cnt>contour_max )
	    contour_max = contour_cnt;
    }

    if ( contour_max<=1 )	/* Can't be out of order */
return;

    changed = 0;
    ci = calloc(contour_max,sizeof(struct contourinfo));
    for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	contour_cnt = 0;
	for ( ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	    best = ss->first;
	    for ( sp=ss->first; ; ) {
		if ( sp->me.x < best->me.x ||
			(sp->me.x==best->me.x && (fabs(sp->me.y)<fabs(best->me.y))) )
		    best = sp;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	    ci[contour_cnt].ss = ss;
	    ci[contour_cnt++].min = &best->me;
	}
	qsort(ci,contour_cnt,sizeof(struct contourinfo),order_contours);
	diff = 0;
	for ( i=0, ss = sc->layers[layer].splines; ss!=NULL; ss=ss->next ) {
	    if ( ci[i].ss != ss ) {
		diff = true;
	break;
	    }
	}
	if ( diff && !changed ) {
	    SCPreserveLayer(sc,layer,false);
	    changed = true;
	}
	if ( diff ) {
	    sc->layers[layer].splines = ci[0].ss;
	    for ( i=1; i<contour_cnt; ++i )
		ci[i-1].ss->next = ci[i].ss;
	    ci[contour_cnt-1].ss->next = NULL;
	}
    }
    free(ci);
    if ( changed )
	SCCharChangedUpdate(sc,ly_all);
}

void SplineSetJoinCpFixup(SplinePoint *sp) {
    BasePoint ndir, pdir;
    bigreal nlen, plen;
    int fixprev=0, fixnext=0;

    if ( sp->pointtype == pt_corner )
	/* Leave control points as they are */;
    else if ( sp->pointtype == pt_tangent ) {
	SplineCharTangentNextCP(sp);
	SplineCharTangentPrevCP(sp);
	fixprev = fixnext = 1;
    } else if ( !BpColinear(&sp->prevcp,&sp->me,&sp->nextcp)) {
	ndir.x = sp->nextcp.x - sp->me.x;
	ndir.y = sp->nextcp.y - sp->me.y;
	nlen = sqrt( ndir.x*ndir.x + ndir.y*ndir.y );
	if ( nlen!=0 ) { ndir.x /= nlen; ndir.y/=nlen; }
	pdir.x = sp->prevcp.x - sp->me.x;
	pdir.y = sp->prevcp.y - sp->me.y;
	plen = sqrt( pdir.x*pdir.x + pdir.y*pdir.y );
	if ( plen!=0 ) { pdir.x /= plen; pdir.y/=plen; }
	if ( !sp->nextcpdef && sp->prevcpdef ) {
	    sp->prevcp.x = sp->me.x - plen * ndir.x;
	    sp->prevcp.y = sp->me.y - plen * ndir.y;
	    fixprev = true;
	} else if ( sp->nextcpdef && !sp->prevcpdef ) {
	    sp->nextcp.x = sp->me.x - nlen * pdir.x;
	    sp->nextcp.y = sp->me.y - nlen * pdir.y;
	    fixnext = true;
	} else {
	    SplineCharDefaultNextCP(sp);
	    SplineCharDefaultPrevCP(sp);
	    fixprev = fixnext = 1;
	}
    }
    if ( sp->next!=NULL && sp->next->to->pointtype==pt_tangent && sp->next->to->next!=NULL ) {
	SplineCharTangentNextCP(sp->next->to);
	SplineRefigure(sp->next->to->next);
    }
    if ( sp->prev!=NULL && sp->prev->from->pointtype==pt_tangent && sp->prev->from->prev!=NULL ) {
	SplineCharTangentPrevCP(sp->prev->from);
	SplineRefigure(sp->prev->from->prev);
    }
    if ( fixprev && sp->prev!=NULL )
	SplineRefigure(sp->prev);
    if ( fixnext && sp->next!=NULL )
	SplineRefigure(sp->next);
}

static int SplineSetMakeLoop(SplineSet *spl,real fudge) {
    if ( spl->first!=spl->last &&
	    (spl->first->me.x >= spl->last->me.x-fudge &&
		spl->first->me.x <= spl->last->me.x+fudge &&
		spl->first->me.y >= spl->last->me.y-fudge &&
		spl->first->me.y <= spl->last->me.y+fudge )) {
	spl->first->prev = spl->last->prev;
	spl->first->prev->to = spl->first;
	spl->first->prevcp = spl->last->prevcp;
	spl->first->prevcpdef = spl->last->prevcpdef;
	SplinePointFree(spl->last);
	spl->last = spl->first;
	if ( spl->spiros!=NULL ) {
	    spl->spiros[0].ty = spl->spiros[spl->spiro_cnt-2].ty;
	    spl->spiros[spl->spiro_cnt-2] = spl->spiros[spl->spiro_cnt-1];
	    --spl->spiro_cnt;
	}
	SplineSetJoinCpFixup(spl->first);
return( true );
    }
return( false );
}

SplineSet *SplineSetJoin(SplineSet *start,int doall,real fudge,int *changed,
                         int doloops) {
    SplineSet *spl, *spl2, *prev;
    /* Few special cases for spiros here because the start and end points */
    /*  will be the same for spiros and beziers. We just need to fixup spiros */
    /*  at the end */

    *changed = false;
    for ( spl=start; spl!=NULL; spl=spl->next ) {
	if ( spl->first->prev==NULL &&
		(doall || PointListIsSelected(spl)) ) {
	    if ( doloops && SplineSetMakeLoop(spl,fudge) ) {
		*changed = true;
	    } else {
		prev = NULL;
		for ( spl2=start ; spl2!=NULL; prev = spl2, spl2=spl2->next ) if ( spl2!=spl ) {
		    if ( spl2->first->prev!=NULL || !(doall || PointListIsSelected(spl2)) )
			continue;
		    if (!( spl->first->me.x >= spl2->last->me.x-fudge &&
			    spl->first->me.x <= spl2->last->me.x+fudge &&
			    spl->first->me.y >= spl2->last->me.y-fudge &&
			    spl->first->me.y <= spl2->last->me.y+fudge )) {
			if (( spl->last->me.x >= spl2->last->me.x-fudge &&
				spl->last->me.x <= spl2->last->me.x+fudge &&
				spl->last->me.y >= spl2->last->me.y-fudge &&
				spl->last->me.y <= spl2->last->me.y+fudge ) ||
			    ( spl->last->me.x >= spl2->first->me.x-fudge &&
				spl->last->me.x <= spl2->first->me.x+fudge &&
				spl->last->me.y >= spl2->first->me.y-fudge &&
				spl->last->me.y <= spl2->first->me.y+fudge ))
			    SplineSetReverse(spl);
		    }
		    if ( spl->first->me.x >= spl2->first->me.x-fudge &&
			    spl->first->me.x <= spl2->first->me.x+fudge &&
			    spl->first->me.y >= spl2->first->me.y-fudge &&
			    spl->first->me.y <= spl2->first->me.y+fudge )
			SplineSetReverse(spl2);
		    if ( spl->first->me.x >= spl2->last->me.x-fudge &&
			    spl->first->me.x <= spl2->last->me.x+fudge &&
			    spl->first->me.y >= spl2->last->me.y-fudge &&
			    spl->first->me.y <= spl2->last->me.y+fudge ) {
			spl->first->prev = spl2->last->prev;
			spl->first->prev->to = spl->first;
			spl->first->prevcp = spl2->last->prevcp;
			spl->first->prevcpdef = spl2->last->prevcpdef;
			SplinePointFree(spl2->last);
			SplineSetJoinCpFixup(spl->first);
			spl->first = spl2->first;
			spl2->first = spl2->last = NULL;
			spl->start_offset = 0;
			spl2->start_offset = 0;
			if ( prev!=NULL )
			    prev->next = spl2->next;
			else
			    start = spl2->next;
			if ( spl->spiros && spl2->spiros ) {
			    if ( spl->spiro_cnt+spl2->spiro_cnt > spl->spiro_max )
				spl->spiros = realloc(spl->spiros,
					(spl->spiro_max = spl->spiro_cnt+spl2->spiro_cnt)*sizeof(spiro_cp));
			    memcpy(spl->spiros+spl->spiro_cnt-1,
				    spl2->spiros+1, (spl2->spiro_cnt-1)*sizeof(spiro_cp));
			    spl->spiro_cnt += spl2->spiro_cnt-2;
			} else
			    SplineSetSpirosClear(spl);
			spl2->last = spl2->first = NULL;
			spl2->start_offset = 0;
			SplinePointListFree(spl2);
			SplineSetMakeLoop(spl,fudge);
			*changed = true;
		break;
		    }
		}
	    }
	}
    }
return(start);
}

SplineSet *SplineCharRemoveTiny(SplineChar *sc,SplineSet *head) {
    SplineSet *spl, *snext, *pr;
    Spline *spline, *next, *first;
    const bigreal err = 1.0/64.0;

    for ( spl = head, pr=NULL; spl!=NULL; spl = snext ) {
	first = NULL;
	for ( spline=spl->first->next; spline!=NULL && spline!=first; spline=next ) {
	    next = spline->to->next;
	    if ( spline->from->me.x-spline->to->me.x>-err && spline->from->me.x-spline->to->me.x<err &&
		    spline->from->me.y-spline->to->me.y>-err && spline->from->me.y-spline->to->me.y<err &&
		    (spline->from->nonextcp || spline->to->noprevcp) &&
		    spline->from->prev!=NULL ) {
		if ( spline->from==spline->to )
	    break;
		if ( spl->last==spline->from ) spl->last = NULL;
		if ( spl->first==spline->from ) {
		  spl->first = NULL;
		  spl->start_offset = 0;
		}
		if ( first==spline->from->prev ) first=NULL;
		/*SplinesRemoveBetween(sc,spline->from->prev->from,spline->to);*/
		spline->to->prevcp = spline->from->prevcp;
		spline->to->prevcpdef = spline->from->prevcpdef;
		spline->from->prev->to = spline->to;
		spline->to->prev = spline->from->prev;
		SplineRefigure(spline->from->prev);
		SplinePointFree(spline->from);
		SplineFree(spline);
		if ( first==NULL ) first = next->from->prev;
		if ( spl->first==NULL ) {
		  spl->first = next->from;
		  spl->start_offset = 0;
		}
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

int SpIsExtremum(SplinePoint *sp) {
    BasePoint *ncp, *pcp;
    BasePoint *nncp, *ppcp;
    if ( sp->next==NULL || sp->prev==NULL )
return( true );
    nncp = &sp->next->to->me;
    if ( !sp->nonextcp ) {
	ncp = &sp->nextcp;
	if ( !sp->next->to->noprevcp )
	    nncp = &sp->next->to->prevcp;
    } else if ( !sp->next->to->noprevcp )
	ncp = &sp->next->to->prevcp;
    else
	ncp = nncp;
    ppcp = &sp->prev->from->me;
    if ( !sp->noprevcp ) {
	pcp = &sp->prevcp;
	if ( !sp->prev->from->nonextcp )
	    ppcp = &sp->prev->from->nextcp;
    } else if ( !sp->prev->from->nonextcp )
	pcp = &sp->prev->from->nextcp;
    else
	pcp = ppcp;
    if ((( ncp->x<sp->me.x || (ncp->x==sp->me.x && nncp->x<sp->me.x)) &&
		(pcp->x<sp->me.x || (pcp->x==sp->me.x && ppcp->x<sp->me.x))) ||
	    ((ncp->x>sp->me.x || (ncp->x==sp->me.x && nncp->x>sp->me.x)) &&
		(pcp->x>sp->me.x || (pcp->x==sp->me.x && ppcp->x>sp->me.x))) ||
	(( ncp->y<sp->me.y || (ncp->y==sp->me.y && nncp->y<sp->me.y)) &&
		(pcp->y<sp->me.y || (pcp->y==sp->me.y && ppcp->y<sp->me.y))) ||
	    ((ncp->y>sp->me.y || (ncp->y==sp->me.y && nncp->y>sp->me.y)) &&
		(pcp->y>sp->me.y || (pcp->y==sp->me.y && ppcp->y>sp->me.y))))
return( true );

    /* These aren't true points of extrema, but they probably should be treated */
    /*  as if they were */
    if ( !sp->nonextcp && !sp->noprevcp &&
	    ((sp->me.x==sp->nextcp.x && sp->me.x==sp->prevcp.x) ||
	     (sp->me.y==sp->nextcp.y && sp->me.y==sp->prevcp.y)) )
return( true );

return( false );
}

/* An extremum is very close to the end-point. So close that we don't want */
/*  to add a new point. Instead try moving the control points around */
/*  Options: */
/*    o  if the control point is very close to the base point then remove it */
/*    o  if the slope at the endpoint is in the opposite direction from */
/*           what we expect, then subtract off the components we don't like */
/*    o  make the slope at the end point horizontal/vertical */
static int ForceEndPointExtrema(Spline *s,int isto) {
    SplinePoint *end;
    BasePoint *cp, oldcp, to, unitslope, othercpunit, myslope;
    bigreal xdiff, ydiff, mylen, cplen, mydot, cpdot, len;
    /* To get here we know that the extremum is extremely close to the end */
    /*  point, and adjusting the slope at the end-point may be all we need */
    /*  to do. We won't need to adjust it by much, because it is so close. */

    if ( isto ) {
	end = s->to; cp = &end->prevcp;
	othercpunit.x = s->from->nextcp.x - s->from->me.x;
	othercpunit.y = s->from->nextcp.y - s->from->me.y;
    } else {
	end = s->from; cp = &end->nextcp;
	othercpunit.x = s->to->prevcp.x-s->to->me.x;
	othercpunit.y = s->to->prevcp.y-s->to->me.y;
    }
    cplen = othercpunit.x*othercpunit.x + othercpunit.y*othercpunit.y;
    cplen = sqrt(cplen);
    myslope.x = cp->x - end->me.x;
    myslope.y = cp->y - end->me.y;
    mylen = sqrt(myslope.x*myslope.x + myslope.y*myslope.y);

    unitslope.x = s->to->me.x - s->from->me.x;
    unitslope.y = s->to->me.y - s->from->me.y;
    len = unitslope.x*unitslope.x + unitslope.y*unitslope.y;
    if ( len==0 )
return( -1 );
    len = sqrt(len);
    if ( mylen<30*len && mylen<cplen && mylen<1 ) {
	if ( mylen==0 )
	    return -1;
	if ( isto ) {
	    s->to->prevcp = s->to->me;
	} else {
	    s->from->nextcp = s->from->me;
	}
	end->pointtype = pt_corner;
	SplineRefigure(s);
return( true );	/* We changed the slope */
    }
    unitslope.x /= len; unitslope.y /= len;

    mydot = myslope.x*unitslope.y - myslope.y*unitslope.x;
    cpdot = othercpunit.x*unitslope.y - othercpunit.y*unitslope.y;
    if ( mydot*cpdot<0 && mylen<cplen ) {
	/* The two control points are in opposite directions with respect to */
	/*  the main spline, and ours isn't very big, so make it point along */
	/*  the spline */
	end->pointtype = pt_corner;
	if ( isto ) {
	    s->to->prevcp.x = s->to->me.x - mydot*unitslope.x;
	    s->to->prevcp.y = s->to->me.y - mydot*unitslope.y;
	} else {
	    s->from->nextcp.x = s->from->me.x + mydot*unitslope.x;
	    s->from->nextcp.y = s->from->me.y + mydot*unitslope.y;
	}
	SplineRefigure(s);
return( true );	/* We changed the slope */
    }
    
    if ( (xdiff = cp->x - end->me.x)<0 ) xdiff = -xdiff;
    if ( (ydiff = cp->y - end->me.y)<0 ) ydiff = -ydiff;

    oldcp = to = *cp;
    if ( xdiff<ydiff/10.0 && xdiff>0 ) {
	to.x = end->me.x;
	end->pointtype = pt_corner;
	SPAdjustControl(end,cp,&to,s->order2);
	return( (cp->x!=oldcp.x || cp->y!=oldcp.y) ? 1 : -1 );
    } else if ( ydiff<xdiff/10 && ydiff>0 ) {
	to.y = end->me.y;
	end->pointtype = pt_corner;
	SPAdjustControl(end,cp,&to,s->order2);
	return( (cp->x!=oldcp.x || cp->y!=oldcp.y) ? 1 : -1 );
    }

return( -1 );		/* Didn't do anything */
}

int Spline1DCantExtremeX(const Spline *s) {
    /* Sometimes we get rounding errors when converting from control points */
    /*  to spline coordinates. These rounding errors can give us false */
    /*  extrema. So do a sanity check to make sure it is possible to get */
    /*  any extrema before actually looking for them */

    if ( s->from->me.x>=s->from->nextcp.x &&
	    s->from->nextcp.x>=s->to->prevcp.x &&
	    s->to->prevcp.x>=s->to->me.x )
return( true );
    if ( s->from->me.x<=s->from->nextcp.x &&
	    s->from->nextcp.x<=s->to->prevcp.x &&
	    s->to->prevcp.x<=s->to->me.x )
return( true );

return( false );
}

int Spline1DCantExtremeY(const Spline *s) {
    /* Sometimes we get rounding errors when converting from control points */
    /*  to spline coordinates. These rounding errors can give us false */
    /*  extrema. So do a sanity check to make sure it is possible to get */
    /*  any extrema before actually looking for them */

    if ( s->from->me.y>=s->from->nextcp.y &&
	    s->from->nextcp.y>=s->to->prevcp.y &&
	    s->to->prevcp.y>=s->to->me.y )
return( true );
    if ( s->from->me.y<=s->from->nextcp.y &&
	    s->from->nextcp.y<=s->to->prevcp.y &&
	    s->to->prevcp.y<=s->to->me.y )
return( true );

return( false );
}

Spline *SplineAddExtrema(Spline *s,int always,real lenbound, real offsetbound,
	DBounds *b) {
    /* First find the extrema, if any */
    bigreal t[4], min;
    uint8_t rmfrom[4], rmto[4];
    int p, i,j, p_s, mini, restart, forced;
    SplinePoint *sp;
    real len;

    if ( !always ) {
	real xlen, ylen;
	xlen = (s->from->me.x-s->to->me.x);
	ylen = (s->from->me.y-s->to->me.y);
	len = xlen*xlen + ylen*ylen;
	lenbound *= lenbound;
	if ( len < lenbound ) {
	    len = SplineLength(s);
	    len *= len;
	}
    }

    memset(rmfrom,0,sizeof(rmfrom));
    memset(rmto,0,sizeof(rmto));

    for (;;) {
	if ( s->knownlinear )
return(s);
	p = 0;
	if ( Spline1DCantExtremeX(s) ) {
	    /* If the control points are at the end-points then this (1D) spline is */
	    /*  basically a line. But rounding errors can give us very faint extrema */
	    /*  if we look for them */
	} else if ( s->splines[0].a!=0 ) {
	    bigreal d = 4*s->splines[0].b*s->splines[0].b-4*3*s->splines[0].a*s->splines[0].c;
	    if ( d>0 ) {
		extended t1, t2;
		d = sqrt(d);
		t1 = (-2*s->splines[0].b+d)/(2*3*s->splines[0].a);
		t2 = (-2*s->splines[0].b-d)/(2*3*s->splines[0].a);
		t[p++] = CheckExtremaForSingleBitErrors(&s->splines[0],t1,t2);
		t[p++] = CheckExtremaForSingleBitErrors(&s->splines[0],t2,t1);
	    }
	} else if ( s->splines[0].b!=0 )
	    t[p++] = -s->splines[0].c/(2*s->splines[0].b);
	if ( !always ) {
	    /* Generally we are only interested in extrema on long splines, or */
	    /* extrema which are extrema for the entire contour, not just this */
	    /* spline */
	    /* Also extrema which are very close to one of the end-points can */
	    /*  be ignored. */
	    /* No they can't. But we need to remove the original point in this*/
	    /*  case */
	    for ( i=0; i<p; ++i ) {
		real x = ((s->splines[0].a*t[i]+s->splines[0].b)*t[i]+s->splines[0].c)*t[i]+s->splines[0].d;
		real y = ((s->splines[1].a*t[i]+s->splines[1].b)*t[i]+s->splines[1].c)*t[i]+s->splines[1].d;
		int close_from = ( x-s->from->me.x<offsetbound && x-s->from->me.x>-offsetbound) &&
				( y-s->from->me.y<10*offsetbound && y-s->from->me.y>-10*offsetbound );
		int close_to = ( x-s->to->me.x<offsetbound && x-s->to->me.x>-offsetbound) &&
				( y-s->to->me.y<10*offsetbound && y-s->to->me.y>-10*offsetbound );
		int remove_from = close_from  && GoodCurve(s->from,true) && !SpIsExtremum(s->from);
		int remove_to = close_to  && GoodCurve(s->to,false) && !SpIsExtremum(s->to);
		if (( x>b->minx && x<b->maxx  && len<lenbound ) ||
			(close_from && !remove_from) || (close_to && !remove_to) ) {
		    --p;
		    for ( j=i; j<p; ++j )
			t[j] = t[j+1];
		    --i;
		} else {
		    rmfrom[i] = remove_from;
		    rmto[i] = remove_to;
		}
	    }
	}

	p_s = p;
	if ( Spline1DCantExtremeY(s) ) {
	    /* If the control points are at the end-points then this (1D) spline is */
	    /*  basically a line. But rounding errors can give us very faint extrema */
	    /*  if we look for them */
	} else if ( s->splines[1].a!=0 ) {
	    bigreal d = 4*s->splines[1].b*s->splines[1].b-4*3*s->splines[1].a*s->splines[1].c;
	    if ( d>0 ) {
		extended t1,t2;
		d = sqrt(d);
		t1 = (-2*s->splines[1].b+d)/(2*3*s->splines[1].a);
		t2 = (-2*s->splines[1].b-d)/(2*3*s->splines[1].a);
		t[p++] = CheckExtremaForSingleBitErrors(&s->splines[1],t1,t2);
		t[p++] = CheckExtremaForSingleBitErrors(&s->splines[1],t2,t1);
	    }
	} else if ( s->splines[1].b!=0 )
	    t[p++] = -s->splines[1].c/(2*s->splines[1].b);
	if ( !always ) {
	    for ( i=p_s; i<p; ++i ) {
		real x = ((s->splines[0].a*t[i]+s->splines[0].b)*t[i]+s->splines[0].c)*t[i]+s->splines[0].d;
		real y = ((s->splines[1].a*t[i]+s->splines[1].b)*t[i]+s->splines[1].c)*t[i]+s->splines[1].d;
		int close_from =( y-s->from->me.y<offsetbound && y-s->from->me.y>-offsetbound ) &&
			( x-s->from->me.x<offsetbound && x-s->from->me.x>-offsetbound);
		int close_to = ( y-s->to->me.y<offsetbound && y-s->to->me.y>-offsetbound ) &&
			( x-s->to->me.x<offsetbound && x-s->to->me.x>-offsetbound);
		int remove_from = close_from  && GoodCurve(s->from,true) && !SpIsExtremum(s->from);
		int remove_to = close_to  && GoodCurve(s->to,false) && !SpIsExtremum(s->to);
		if (( y>b->miny && y<b->maxy && len<lenbound ) ||
			(close_from && !remove_from) || (close_to && !remove_to) ) {
		    --p;
		    for ( j=i; j<p; ++j )
			t[j] = t[j+1];
		    --i;
		} else {
		    rmfrom[i] = remove_from;
		    rmto[i] = remove_to;
		}
	    }
	}

	/* Throw out any t values which are not between 0 and 1 */
	/*  (we do a little fudging near the endpoints so we don't get confused */
	/*   by rounding errors) */
	restart = false;
	for ( i=0; i<p; ++i ) {
	    if ( t[i]>0 && t[i]<.05 ) {
		BasePoint test;
		/* Expand stroke gets very confused on zero-length splines so */
		/*  don't let that happen */
		test.x = ((s->splines[0].a*t[i]+s->splines[0].b)*t[i]+s->splines[0].c)*t[i]+s->splines[0].d - s->from->me.x;
		test.y = ((s->splines[1].a*t[i]+s->splines[1].b)*t[i]+s->splines[1].c)*t[i]+s->splines[1].d - s->from->me.y;
		if (( test.x*test.x + test.y*test.y<1e-7 ) && ( test.x*test.x + test.y*test.y>0.0 )) {
		    if ( (forced = ForceEndPointExtrema(s,0))>=0 ) {
			if ( forced && s->from->prev!=NULL )
			    SplineAddExtrema(s->from->prev,always,lenbound,offsetbound,b);
			restart = true;
	break;
		    }
		}
	    }
	    if ( t[i]<1 && t[i]>.95 ) {
		BasePoint test;
		test.x = ((s->splines[0].a*t[i]+s->splines[0].b)*t[i]+s->splines[0].c)*t[i]+s->splines[0].d - s->to->me.x;
		test.y = ((s->splines[1].a*t[i]+s->splines[1].b)*t[i]+s->splines[1].c)*t[i]+s->splines[1].d - s->to->me.y;
		if (( test.x*test.x + test.y*test.y < 1e-7 ) && ( test.x*test.x + test.y*test.y>0.0 )) {
		    if ( ForceEndPointExtrema(s,1)>=0 ) {
			/* don't need to fix up next, because splinesetaddextrema will do that soon */
			restart = true;
	break;
		    }
		}
	    }
		
	    if ( t[i]<=0 || t[i]>=1.0 ) {
		--p;
		for ( j=i; j<p; ++j ) {
		    t[j] = t[j+1];
		    rmfrom[j] = rmfrom[j+1];
		    rmto[j] = rmto[j+1];
		}
		--i;
	    }
	}
	if ( restart )
    continue;

	if ( p==0 )
return(s);

	/* Find the smallest of all the interesting points */
	min = t[0]; mini = 0;
	for ( i=1; i<p; ++i ) {
	    if ( t[i]<min ) {
		min=t[i];
		mini = i;
	    }
	}
	sp = SplineBisect(s,min);
/* On the mac we get rounding errors in the bisect routine */
	{ bigreal dx, dy;
	    if ( (dx = sp->me.x - sp->prevcp.x)<0 ) dx=-dx;
	    if ( (dy = sp->me.y - sp->prevcp.y)<0 ) dy=-dy;
	    if ( dx!=0 && dy!=0 ) {
		if ( dx<dy )
		    sp->prevcp.x = sp->me.x;
		else
		    sp->prevcp.y = sp->me.y;
	    }
	    if ( (dx = sp->me.x - sp->nextcp.x)<0 ) dx=-dx;
	    if ( (dy = sp->me.y - sp->nextcp.y)<0 ) dy=-dy;
	    if ( dx!=0 && dy!=0 ) {
		if ( dx<dy )
		    sp->nextcp.x = sp->me.x;
		else
		    sp->nextcp.y = sp->me.y;
	    }
	}

	if ( rmfrom[mini] ) sp->prev->from->ticked = true;
	if ( rmto[mini] ) sp->next->to->ticked = true;
	s = sp->next;
	if ( p==1 )
return( s );
	/* Don't try to use any other computed t values, it is easier to */
	/*  recompute them than to try and figure out what they map to on the */
	/*  new spline */
    }
}

void SplineSetAddExtrema(SplineChar *sc, SplineSet *ss,enum ae_type between_selected, int emsize) {
    Spline *s, *first;
    DBounds b;
    int always = true;
    real lenbound = 0;
    real offsetbound=0;
    SplinePoint *sp, *nextp;

    if ( between_selected==ae_only_good ) {
	SplineSetQuickBounds(ss,&b);
	lenbound = (emsize)/32.0;
	always = false;
	offsetbound = .5;
	between_selected = ae_only_good_rm_later;
	for ( sp = ss->first; ; ) {
	    sp->ticked = false;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }

    first = NULL;
    for ( s = ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	if ( between_selected!=ae_between_selected ||
		(s->from->selected && s->to->selected))
	    s = SplineAddExtrema(s,always,lenbound,offsetbound,&b);
	if ( first==NULL ) first = s;
    }
    if ( between_selected == ae_only_good_rm_later ) {
	for ( sp = ss->first; ; ) {
	    if ( sp->next==NULL )
	break;
	    nextp = sp->next->to;
	    if ( sp->ticked ) {
		if ( sp==ss->first ) {
		    ss->first = ss->last = nextp;
		    ss->start_offset = 0;
		}
		SplinesRemoveBetween(sc,sp->prev->from,nextp,1);
	    }
	    sp = nextp;
	    if ( sp==ss->first )
	break;
	}
    }
}

void SplineCharAddExtrema(SplineChar *sc, SplineSet *head,enum ae_type between_selected,int emsize) {
    SplineSet *ss;

    for ( ss=head; ss!=NULL; ss=ss->next )
	    SplineSetAddExtrema(sc,ss,between_selected,emsize);
}

Spline *SplineAddInflections(Spline *s) { 
    if ( s->knownlinear )
return(s);
    /* First find the inflections, if any */
    extended inflect[2];
    int i = Spline2DFindPointsOfInflection(s, inflect);
    if ( i==2 ) {
	if ( RealNearish(inflect[0],inflect[1]) )
	    --i;
	else if ( inflect[0]>inflect[1] ) {
	    real temp = inflect[0];
	    inflect[0] = inflect[1];
	    inflect[1] = temp;
	}
    }   
    /* Now split the spline at 1 or 2 times */
    extended splittimes[3] = {-1,-1,-1}; /* 3 slots due to SplineSplit() */
    for (int j = 0; j < i; j++) {
		if ( inflect[j] > .001 && inflect[j] < .999 ) /* avoid rounding issues */
			splittimes[j] = inflect[j];
	}
    if ( splittimes[0] == -1)
return(s);
    s = SplineSplit(s,splittimes);
return(s);	
}

void SplineSetAddInflections(SplineChar *sc, SplineSet *ss, int anysel) {
    Spline *s, *first; 
    first = NULL;
    for ( s = ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	    if ( !anysel || (s->from->selected && s->to->selected) )
			s = SplineAddInflections(s);
	    if ( first==NULL ) first = s;
    }
}

void SplineCharAddInflections(SplineChar *sc, SplineSet *head, int anysel) { 
    SplineSet *ss;
    for ( ss=head; ss!=NULL; ss=ss->next )
	    SplineSetAddInflections(sc,ss,anysel);
}

/* Make the line between the control points parallel to the chord */
/* such that the area is preserved (kind of an improved "tunnify") */
Spline *SplineBalance(Spline *s) { 
    if ( s->knownlinear || s->order2 )
return(s);
	bigreal flen,tlen,ftlen;
	BasePoint fromhandle = BPSub(s->from->nextcp, s->from->me);
	BasePoint tohandle = BPSub(s->to->prevcp, s->to->me);
	BasePoint ft = BPSub(s->to->me, s->from->me);
	flen = BPNorm(fromhandle);
	tlen = BPNorm(tohandle);
	ftlen = BPNorm(ft);
	if ( ( flen == 0 && tlen == 0 ) || ftlen == 0) 
return(s); /* line or closed path*/
	if ( flen == 0 ) 
		fromhandle = BPSub(s->to->prevcp, s->from->me);
	if ( tlen == 0 ) 
		tohandle = BPSub(s->from->nextcp, s->to->me);
	BasePoint fromunit = NormVec(fromhandle); /* do not divide by flen (could be 0) */
	BasePoint tounit = NormVec(tohandle); /* do not divide by tlen (could be 0) */
	BasePoint ftunit = BPScale(ft,1/ftlen);
	BasePoint aunit = (BasePoint) { BPDot(ftunit, fromunit), BPCross(ftunit, fromunit) }; 
    BasePoint bunit = (BasePoint) { BPDot(BPRev(ftunit), tounit),BPCross(ftunit, tounit) }; 
    if ( aunit.y < 0 ) { /* normalize aunit.y to >= 0: */
		aunit.y = -aunit.y;
		bunit.y = -bunit.y;
	}
	bigreal sab = aunit.y * bunit.x + aunit.x * bunit.y; /* sin(alpha+beta) */
	if (sab == 0) { /* if handles are parallel */
		bigreal len = (flen + tlen) / 2;
		s->from->nextcp = BPAdd(s->from->me, BPScale(fromunit, len));
		s->to->prevcp = BPAdd(s->to->me, BPScale(tounit, len));
		SplineRefigure(s);
return(s);
	}
	if ( bunit.y <= 0 || aunit.y == 0)
return(s); /* impossible */
	/* generic case: */
	flen /= ftlen;
	tlen /= ftlen;
	bigreal asa = flen * aunit.y; /* a*sin(alpha) */
	bigreal bsb = tlen * bunit.y; /* b*sin(beta) */
	bigreal area = 2*(asa+bsb)-flen*tlen*sab;	
	bigreal c = aunit.x/aunit.y + bunit.x/bunit.y;
	bigreal discriminant = 4-c*area;
	if ( discriminant < 0 ) /* occurs sometimes for splines with inflections */
return(s); 
	bigreal h = (2-sqrt(discriminant))/c; /* take the smaller solution as the larger could have loops */
	if ( h < 0 ) 
		h = (2+sqrt(discriminant))/c;
	s->from->nextcp = BPAdd(s->from->me, BPScale(fromunit, h/aunit.y*ftlen));
	s->to->prevcp = BPAdd(s->to->me, BPScale(tounit, h/bunit.y*ftlen));
	SplineRefigure(s);
return(s);
}

void SplineSetBalance(SplineChar *sc, SplineSet *ss, int anysel) {
    Spline *s, *first; 
    first = NULL;
    for ( s = ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	    if ( !anysel || (s->from->selected && s->to->selected) )
			s = SplineBalance(s);
	    if ( first==NULL ) first = s;
    }
}

void SplineCharBalance(SplineChar *sc, SplineSet *head, int anysel) { 
    SplineSet *ss;
    for ( ss=head; ss!=NULL; ss=ss->next )
	    SplineSetBalance(sc,ss,anysel);
}

/* Make the splines adjacent to the SplinePoint sp G2-continuous */
/* by moving sp between its adjacent control points */
void SplinePointHarmonize(SplinePoint *sp) {
    if ( sp->prev!=NULL && sp->next!=NULL && !BPEq(sp->prevcp, sp->nextcp) 
    && ( sp->pointtype==pt_curve || sp->pointtype == pt_hvcurve ) ) {
		BasePoint tangentunit = NormVec(BPSub(sp->nextcp,sp->prevcp));
		bigreal p; /* distance of previous point to handle line prevcp--nextcp */
		bigreal n; /* distance of next point to handle line prevcp--nextcp */
		if ( sp->prev->order2 ) 
			p = fabs(BPCross(tangentunit,BPSub(sp->prev->from->me,sp->me)));
		else
			p = fabs(BPCross(tangentunit,BPSub(sp->prev->from->nextcp,sp->me)));
		if ( sp->next->order2 ) 
			n = fabs(BPCross(tangentunit,BPSub(sp->next->to->me,sp->me)));
		else
			n = fabs(BPCross(tangentunit,BPSub(sp->next->to->prevcp,sp->me)));
		if ( p == n ) sp->me = BPAvg(sp->nextcp,sp->prevcp);
		else {
			bigreal t = (p-sqrt(p*n))/(p-n);
			sp->me = BPAdd(BPScale(sp->prevcp,1-t),BPScale(sp->nextcp,t));
		}
		SplineRefigure(sp->prev);
		SplineRefigure(sp->next);
	}	
}

void SplineSetHarmonize(SplineChar *sc, SplineSet *ss, int anysel) {
    Spline *s, *first; 
    first = NULL;
    for ( s = ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	    if ( !anysel || s->from->selected )
			SplinePointHarmonize(s->from);
	    if ( first==NULL ) first = s;
    }
}

void SplineCharHarmonize(SplineChar *sc, SplineSet *head, int anysel) { 
    SplineSet *ss;
    for ( ss=head; ss!=NULL; ss=ss->next )
	    SplineSetHarmonize(sc,ss,anysel);
}

char *GetNextUntitledName(void) {
    static int untitled_cnt=1;
    char buffer[80];

    sprintf( buffer, "Untitled%d", untitled_cnt++ );
return( copy(buffer));
}

SplineFont *SplineFontEmpty(void) {
    extern int default_fv_row_count, default_fv_col_count;
    SplineFont *sf;

    sf = calloc(1,sizeof(SplineFont));
    sf->pfminfo.fstype = -1;
    sf->pfminfo.stylemap = -1;
    sf->top_enc = -1;
    sf->macstyle = -1;
    sf->desired_row_cnt = default_fv_row_count; sf->desired_col_cnt = default_fv_col_count;
    sf->display_antialias = default_fv_antialias;
    sf->display_bbsized = default_fv_bbsized;
    sf->display_size = -default_fv_font_size;
    sf->display_layer = ly_fore;
    sf->sfntRevision = sfntRevisionUnset;
    sf->woffMajor = woffUnset;
    sf->woffMinor = woffUnset;
    sf->pfminfo.winascent_add = sf->pfminfo.windescent_add = true;
    sf->pfminfo.hheadascent_add = sf->pfminfo.hheaddescent_add = true;
    sf->pfminfo.typoascent_add = sf->pfminfo.typodescent_add = true;
    if ( TTFFoundry!=NULL )
	strncpy(sf->pfminfo.os2_vendor,TTFFoundry,4);
    else
	memcpy(sf->pfminfo.os2_vendor,"PfEd",4);
    sf->for_new_glyphs = DefaultNameListForNewFonts();
    sf->creationtime = sf->modificationtime = GetTime();

    sf->layer_cnt = 2;
    sf->layers = calloc(2,sizeof(LayerInfo));
    sf->layers[ly_back].name = copy(_("Back"));
    sf->layers[ly_back].background = true;
    sf->layers[ly_fore].name = copy(_("Fore"));
    sf->layers[ly_fore].background = false;
    sf->grid.background = true;

return( sf );
}

SplineFont *SplineFontBlank(int charcnt) {
    SplineFont *sf;
    char buffer[200];
    time_t now;
    struct tm *tm;
    const char *author = GetAuthor();

    sf = SplineFontEmpty();
    sf->fontname = GetNextUntitledName();
    sf->fullname = copy(sf->fontname);
    sf->familyname = copy(sf->fontname);
    sprintf( buffer, "%s.sfd", sf->fontname);
    sf->origname = GFileGetAbsoluteName(buffer);
    sf->weight = copy("Regular");
    now = GetTime();
    if (!getenv("SOURCE_DATE_EPOCH")) {
	tm = localtime(&now);
    } else {
	tm = gmtime(&now);
    }
    if ( author!=NULL )
	sprintf( buffer, "Copyright (c) %d, %.50s", tm->tm_year+1900, author );
    else
	sprintf( buffer, "Copyright (c) %d, Anonymous", tm->tm_year+1900 );
    sf->copyright = copy(buffer);
    if ( xuid!=NULL ) {
	sf->xuid = malloc(strlen(xuid)+20);
	sprintf(sf->xuid,"[%s %d]", xuid, (rand()&0xffffff));
    }
    sprintf( buffer, "%d-%d-%d: Created with FontForge (http://fontforge.org)", tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday );
    sf->comments = copy(buffer);
    sf->version = copy("001.000");
    sf->ascent = rint(new_em_size*.8); sf->descent = new_em_size-sf->ascent;
    sf->upos = -rint(new_em_size*.1); sf->uwidth = rint(new_em_size*.05);		/* defaults for cff */
    sf->glyphcnt = 0;
    sf->glyphmax = charcnt;
    sf->glyphs = calloc(charcnt,sizeof(SplineChar *));
    sf->pfminfo.fstype = -1;
    sf->pfminfo.stylemap = -1;
    sf->use_typo_metrics = true;
return( sf );
}

SplineFont *SplineFontNew(void) {
    SplineFont *sf;
    int enclen = default_encoding->char_cnt;

    sf = SplineFontBlank(enclen);
    sf->onlybitmaps = true;
    sf->new = true;
    sf->layers[ly_back].order2 = new_fonts_are_order2;
    sf->layers[ly_fore].order2 = new_fonts_are_order2;
    sf->grid.order2 = new_fonts_are_order2;

    sf->map = EncMapNew(enclen,enclen,default_encoding);
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

    new = malloc(pt-sf->xuid+12);
    strncpy(new,sf->xuid,pt-sf->xuid);
    npt = new + (pt-sf->xuid);
    if ( npt==new ) *npt++ = '[';
    sprintf(npt, "%d]", val );
    free(sf->xuid); sf->xuid = new;
    sf->changed = true;
    sf->changed_since_xuidchanged = false;
}

void SFIncrementXUID(SplineFont *sf) {
    SFChangeXUID(sf,false);
}

void SFRandomChangeXUID(SplineFont *sf) {
    SFChangeXUID(sf,true);
}

void SPWeightedAverageCps(SplinePoint *sp) {
    bigreal pangle, nangle, angle, plen, nlen, c, s;
    if ( sp->noprevcp || sp->nonextcp )
	/*SPAverageCps(sp)*/;		/* Expand Stroke wants this case to hold still */
    else if (( sp->pointtype==pt_curve || sp->pointtype==pt_hvcurve) &&
	    sp->prev && sp->next ) {
	pangle = atan2(sp->me.y-sp->prevcp.y,sp->me.x-sp->prevcp.x);
	nangle = atan2(sp->nextcp.y-sp->me.y,sp->nextcp.x-sp->me.x);
	if ( pangle<0 && nangle>0 && nangle-pangle>=(FF_PI-6e-8) )
	    pangle += 2*FF_PI;
	else if ( pangle>0 && nangle<0 && pangle-nangle>=(FF_PI-6e-8) )
	    nangle += 2*FF_PI;
	plen = sqrt((sp->me.y-sp->prevcp.y)*(sp->me.y-sp->prevcp.y) +
		(sp->me.x-sp->prevcp.x)*(sp->me.x-sp->prevcp.x));
	nlen = sqrt((sp->nextcp.y-sp->me.y)*(sp->nextcp.y-sp->me.y) +
		(sp->nextcp.x-sp->me.x)*(sp->nextcp.x-sp->me.x));
	if ( plen+nlen==0 )
	    angle = (nangle+pangle)/2;
	else
	    angle = (plen*pangle + nlen*nangle)/(plen+nlen);
	plen = -plen;
	c = cos(angle); s=sin(angle);
	sp->nextcp.x = c*nlen + sp->me.x;
	sp->nextcp.y = s*nlen + sp->me.y;
	sp->prevcp.x = c*plen + sp->me.x;
	sp->prevcp.y = s*plen + sp->me.y;
	SplineRefigure(sp->prev);
	SplineRefigure(sp->next);
    } else
	SPAverageCps(sp);
}

void SPAverageCps(SplinePoint *sp) {
    bigreal pangle, nangle, angle, plen, nlen, c, s;
    if (( sp->pointtype==pt_curve || sp->pointtype==pt_hvcurve) &&
	    sp->prev && sp->next ) {
	if ( sp->noprevcp )
	    pangle = atan2(sp->me.y-sp->prev->from->me.y,sp->me.x-sp->prev->from->me.x);
	else
	    pangle = atan2(sp->me.y-sp->prevcp.y,sp->me.x-sp->prevcp.x);
	if ( sp->nonextcp )
	    nangle = atan2(sp->next->to->me.y-sp->me.y,sp->next->to->me.x-sp->me.x);
	else
	    nangle = atan2(sp->nextcp.y-sp->me.y,sp->nextcp.x-sp->me.x);
	if ( pangle<0 && nangle>0 && nangle-pangle>=(FF_PI-6e-8) )
	    pangle += 2*FF_PI;
	else if ( pangle>0 && nangle<0 && pangle-nangle>=(FF_PI-6e-8) )
	    nangle += 2*FF_PI;
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
    } else if ( sp->pointtype==pt_tangent && sp->prev && sp->next ) {
	if ( !sp->noprevcp ) {
	    nangle = atan2(sp->next->to->me.y-sp->me.y,sp->next->to->me.x-sp->me.x);
	    plen = -sqrt((sp->me.y-sp->prevcp.y)*(sp->me.y-sp->prevcp.y) +
		    (sp->me.x-sp->prevcp.x)*(sp->me.x-sp->prevcp.x));
	    c = cos(nangle); s=sin(nangle);
	    sp->prevcp.x = c*plen + sp->me.x;
	    sp->prevcp.y = s*plen + sp->me.y;
	SplineRefigure(sp->prev);
	}
	if ( !sp->nonextcp ) {
	    pangle = atan2(sp->me.y-sp->prev->from->me.y,sp->me.x-sp->prev->from->me.x);
	    nlen = sqrt((sp->nextcp.y-sp->me.y)*(sp->nextcp.y-sp->me.y) +
		    (sp->nextcp.x-sp->me.x)*(sp->nextcp.x-sp->me.x));
	    c = cos(pangle); s=sin(pangle);
	    sp->nextcp.x = c*nlen + sp->me.x;
	    sp->nextcp.y = s*nlen + sp->me.y;
	    SplineRefigure(sp->next);
	}
    }
}

void SPLAverageCps(SplinePointList *spl) {
    SplinePoint *sp;

    while ( spl!=NULL ) {
	for ( sp=spl->first ; ; ) {
	    SPAverageCps(sp);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	spl = spl->next;
    }
}

void SplineCharTangentNextCP(SplinePoint *sp) {
    bigreal len;
    BasePoint *bp, unit;
    extern int snaptoint;

    if ( sp->prev==NULL )
return;
    bp = &sp->prev->from->me;

    unit.y = sp->me.y-bp->y; unit.x = sp->me.x-bp->x;
    len = sqrt( unit.x*unit.x + unit.y*unit.y );
    if ( len!=0 ) {
	unit.x /= len;
	unit.y /= len;
    }
    len = sqrt((sp->nextcp.y-sp->me.y)*(sp->nextcp.y-sp->me.y) + (sp->nextcp.x-sp->me.x)*(sp->nextcp.x-sp->me.x));
    sp->nextcp.x = sp->me.x + len*unit.x;
    sp->nextcp.y = sp->me.y + len*unit.y;
    if ( snaptoint ) {
	sp->nextcp.x = rint(sp->nextcp.x);
	sp->nextcp.y = rint(sp->nextcp.y);
    } else {
	sp->nextcp.x = rint(sp->nextcp.x*1024)/1024;
	sp->nextcp.y = rint(sp->nextcp.y*1024)/1024;
    }
    if ( sp->next!=NULL && sp->next->order2 )
	sp->next->to->prevcp = sp->nextcp;
}

void SplineCharTangentPrevCP(SplinePoint *sp) {
    bigreal len;
    BasePoint *bp, unit;
    extern int snaptoint;

    if ( sp->next==NULL )
return;
    bp = &sp->next->to->me;

    unit.y = sp->me.y-bp->y; unit.x = sp->me.x-bp->x;
    len = sqrt( unit.x*unit.x + unit.y*unit.y );
    if ( len!=0 ) {
	unit.x /= len;
	unit.y /= len;
    }
    len = sqrt((sp->prevcp.y-sp->me.y)*(sp->prevcp.y-sp->me.y) + (sp->prevcp.x-sp->me.x)*(sp->prevcp.x-sp->me.x));
    sp->prevcp.x = sp->me.x + len*unit.x;
    sp->prevcp.y = sp->me.y + len*unit.y;
    if ( snaptoint ) {
	sp->prevcp.x = rint(sp->prevcp.x);
	sp->prevcp.y = rint(sp->prevcp.y);
    } else {
	sp->prevcp.x = rint(sp->prevcp.x*1024)/1024;
	sp->prevcp.y = rint(sp->prevcp.y*1024)/1024;
    }
    if ( sp->prev!=NULL && sp->prev->order2 )
	sp->prev->from->nextcp = sp->prevcp;
}

void BP_HVForce(BasePoint *vector) {
    /* Force vector to be horizontal/vertical */
    bigreal dx, dy, len;

    if ( (dx= vector->x)<0 ) dx = -dx;
    if ( (dy= vector->y)<0 ) dy = -dy;
    if ( dx==0 || dy==0 )
return;
    len = sqrt(dx*dx + dy*dy);
    if ( dx>dy ) {
	vector->x = vector->x<0 ? -len : len;
	vector->y = 0;
    } else {
	vector->y = vector->y<0 ? -len : len;
	vector->x = 0;
    }
}
    
void SplineCharDefaultNextCP(SplinePoint *base) {
    SplinePoint *prev=NULL, *next;
    bigreal len, plen, ulen;
    BasePoint unit;
    extern int snaptoint;

    if ( base->next==NULL )
return;
    if ( base->next->order2 ) {
	SplineRefigureFixup(base->next);
return;
    }
    if ( !base->nextcpdef ) {
	if ( base->pointtype==pt_tangent )
	    SplineCharTangentNextCP(base);
return;
    }
    next = base->next->to;
    if ( base->prev!=NULL )
	prev = base->prev->from;

    len = NICE_PROPORTION * sqrt((base->me.x-next->me.x)*(base->me.x-next->me.x) +
	    (base->me.y-next->me.y)*(base->me.y-next->me.y));
    unit.x = next->me.x - base->me.x;
    unit.y = next->me.y - base->me.y;
    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
    if ( ulen!=0 )
	unit.x /= ulen, unit.y /= ulen;
    // Now that nonextcp is set automatically based on equality of
    // nextcp and me, we force unequal values here to track
    // the need for a nextcp. It is recalculated below with "len".
    if ( base->nextcp.x==base->me.x && base->nextcp.y==base->me.y )
	base->nextcp.x += 1;

    if ( base->pointtype == pt_curve || base->pointtype == pt_hvcurve ) {
	if ( prev!=NULL && (base->prevcpdef || base->noprevcp)) {
	    unit.x = next->me.x - prev->me.x;
	    unit.y = next->me.y - prev->me.y;
	    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( ulen!=0 )
		unit.x /= ulen, unit.y /= ulen;
	    if ( base->pointtype == pt_hvcurve )
		BP_HVForce(&unit);
	    plen = sqrt((base->prevcp.x-base->me.x)*(base->prevcp.x-base->me.x) +
		    (base->prevcp.y-base->me.y)*(base->prevcp.y-base->me.y));
	    base->prevcp.x = base->me.x - plen*unit.x;
	    base->prevcp.y = base->me.y - plen*unit.y;
	    if ( snaptoint ) {
		base->prevcp.x = rint(base->prevcp.x);
		base->prevcp.y = rint(base->prevcp.y);
	    }
	    SplineRefigureFixup(base->prev);
	} else if ( prev!=NULL ) {
	    /* The prev control point is fixed. So we've got to use the same */
	    /*  angle it uses */
	    unit.x = base->me.x-base->prevcp.x;
	    unit.y = base->me.y-base->prevcp.y;
	    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( ulen!=0 )
		unit.x /= ulen, unit.y /= ulen;
	} else {
	    base->prevcp = base->me;
	    base->prevcpdef = true;
	}
	if ( base->pointtype == pt_hvcurve )
	    BP_HVForce(&unit);
    } else if ( base->pointtype == pt_corner ) {
	if ( next->pointtype != pt_curve && next->pointtype != pt_hvcurve ) {
	    base->nextcp = base->me;
	}
    } else /* tangent */ {
	if ( next->pointtype != pt_curve ) {
	    base->nextcp = base->me;
	} else {
	    if ( prev!=NULL ) {
		if ( !base->noprevcp ) {
		    plen = sqrt((base->prevcp.x-base->me.x)*(base->prevcp.x-base->me.x) +
			    (base->prevcp.y-base->me.y)*(base->prevcp.y-base->me.y));
		    base->prevcp.x = base->me.x - plen*unit.x;
		    base->prevcp.y = base->me.y - plen*unit.y;
		    SplineRefigureFixup(base->prev);
		}
		unit.x = base->me.x-prev->me.x;
		unit.y = base->me.y-prev->me.y;
		ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
		if ( ulen!=0 )
		    unit.x /= ulen, unit.y /= ulen;
	    }
	}
    }
    if ( base->nextcp.x!=base->me.x || base->nextcp.y!=base->me.y ) {
	base->nextcp.x = base->me.x + len*unit.x;
	base->nextcp.y = base->me.y + len*unit.y;
	if ( snaptoint ) {
	    base->nextcp.x = rint(base->nextcp.x);
	    base->nextcp.y = rint(base->nextcp.y);
	} else {
	    base->nextcp.x = rint(base->nextcp.x*1024)/1024;
	    base->nextcp.y = rint(base->nextcp.y*1024)/1024;
	}
	if ( base->next != NULL )
	    SplineRefigureFixup(base->next);
    }
}

void SplineCharDefaultPrevCP(SplinePoint *base) {
    SplinePoint *next=NULL, *prev;
    bigreal len, nlen, ulen;
    BasePoint unit;
    extern int snaptoint;

    if ( base->prev==NULL )
return;
    if ( base->prev->order2 ) {
	SplineRefigureFixup(base->prev);
return;
    }
    if ( !base->prevcpdef ) {
	if ( base->pointtype==pt_tangent )
	    SplineCharTangentPrevCP(base);
return;
    }
    prev = base->prev->from;
    if ( base->next!=NULL )
	next = base->next->to;

    len = NICE_PROPORTION * sqrt((base->me.x-prev->me.x)*(base->me.x-prev->me.x) +
	    (base->me.y-prev->me.y)*(base->me.y-prev->me.y));
    unit.x = prev->me.x - base->me.x;
    unit.y = prev->me.y - base->me.y;
    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
    if ( ulen!=0 )
	unit.x /= ulen, unit.y /= ulen;
    // Now that noprevcp is set automatically based on equality of
    // prevcp and me, we force unequal values here to track
    // the need for a nextcp. It is recalculated below with "len".
    if ( base->prevcp.x==base->me.x && base->prevcp.y==base->me.y )
	base->prevcp.x += 1;

    if ( base->pointtype == pt_curve || base->pointtype == pt_hvcurve ) {
	if ( next!=NULL && (base->nextcpdef || base->nonextcp)) {
	    unit.x = prev->me.x - next->me.x;
	    unit.y = prev->me.y - next->me.y;
	    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( ulen!=0 ) 
		unit.x /= ulen, unit.y /= ulen;
	    if ( base->pointtype == pt_hvcurve )
		BP_HVForce(&unit);
	    nlen = sqrt((base->nextcp.x-base->me.x)*(base->nextcp.x-base->me.x) +
		    (base->nextcp.y-base->me.y)*(base->nextcp.y-base->me.y));
	    base->nextcp.x = base->me.x - nlen*unit.x;
	    base->nextcp.y = base->me.y - nlen*unit.y;
	    if ( snaptoint ) {
		base->nextcp.x = rint(base->nextcp.x);
		base->nextcp.y = rint(base->nextcp.y);
	    }
	    SplineRefigureFixup(base->next);
	} else if ( next!=NULL ) {
	    /* The next control point is fixed. So we got to use the same */
	    /*  angle it uses */
	    unit.x = base->me.x-base->nextcp.x;
	    unit.y = base->me.y-base->nextcp.y;
	    ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
	    if ( ulen!=0 )
		unit.x /= ulen, unit.y /= ulen;
	} else {
	    base->nextcp = base->me;
	    base->nextcpdef = true;
	}
	if ( base->pointtype == pt_hvcurve )
	    BP_HVForce(&unit);
    } else if ( base->pointtype == pt_corner ) {
	if ( prev->pointtype != pt_curve && prev->pointtype != pt_hvcurve ) {
	    base->prevcp = base->me;
	}
    } else /* tangent */ {
	if ( prev->pointtype != pt_curve ) {
	    base->prevcp = base->me;
	} else {
	    if ( next!=NULL ) {
		if ( !base->nonextcp ) {
		    nlen = sqrt((base->nextcp.x-base->me.x)*(base->nextcp.x-base->me.x) +
			    (base->nextcp.y-base->me.y)*(base->nextcp.y-base->me.y));
		    base->nextcp.x = base->me.x - nlen*unit.x;
		    base->nextcp.y = base->me.y - nlen*unit.y;
		    SplineRefigureFixup(base->next);
		}
		unit.x = base->me.x-next->me.x;
		unit.y = base->me.y-next->me.y;
		ulen = sqrt(unit.x*unit.x + unit.y*unit.y);
		if ( ulen!=0 )
		    unit.x /= ulen, unit.y /= ulen;
	    }
	}
    }
    if ( base->prevcp.x!=base->me.x || base->prevcp.y!=base->me.y ) {
	base->prevcp.x = base->me.x + len*unit.x;
	base->prevcp.y = base->me.y + len*unit.y;
	if ( snaptoint ) {
	    base->prevcp.x = rint(base->prevcp.x);
	    base->prevcp.y = rint(base->prevcp.y);
	} else {
	    base->prevcp.x = rint(base->prevcp.x*1024)/1024;
	    base->prevcp.y = rint(base->prevcp.y*1024)/1024;
	}
	if ( base->prev!=NULL )
	    SplineRefigureFixup(base->prev);
    }
}

void SPHVCurveForce(SplinePoint *sp) {
    BasePoint unit;
    bigreal len, dot;
    if ( sp->prev==NULL || sp->next==NULL || sp->pointtype==pt_corner )
return;

    if ( sp->pointtype==pt_hvcurve &&
	    !sp->nonextcp && !sp->noprevcp ) {
	if ( sp->prev!=NULL && sp->prev->order2 ) {
	    SplineRefigureFixup(sp->prev);
	    SplineRefigureFixup(sp->next);
return;
	}

	unit.x = sp->nextcp.x-sp->prevcp.x;
	unit.y = sp->nextcp.y-sp->prevcp.y;
	len = sqrt(unit.x*unit.x + unit.y*unit.y);
	if ( len==0 )
return;
	unit.x /= len; unit.y /= len;
	BP_HVForce(&unit);
	dot = (sp->nextcp.x-sp->me.x)*unit.x + (sp->nextcp.y-sp->me.y)*unit.y;
	sp->nextcp.x = dot * unit.x + sp->me.x;
	sp->nextcp.y = dot * unit.y + sp->me.y;
	dot = (sp->prevcp.x-sp->me.x)*unit.x + (sp->prevcp.y-sp->me.y)*unit.y;
	sp->prevcp.x = dot * unit.x + sp->me.x;
	sp->prevcp.y = dot * unit.y + sp->me.y;
	SplineRefigure(sp->prev); SplineRefigure(sp->next);
    }
}

void SPSmoothJoint(SplinePoint *sp) {
    BasePoint unitn, unitp;
    bigreal len, dot, dotn, dotp;
    if ( sp->prev==NULL || sp->next==NULL || sp->pointtype==pt_corner )
return;

    if ( (sp->pointtype==pt_curve || sp->pointtype==pt_hvcurve ) &&
	    !sp->nonextcp && !sp->noprevcp ) {
	unitn.x = sp->nextcp.x-sp->me.x;
	unitn.y = sp->nextcp.y-sp->me.y;
	len = sqrt(unitn.x*unitn.x + unitn.y*unitn.y);
	if ( len==0 )
return;
	unitn.x /= len; unitn.y /= len;
	unitp.x = sp->me.x - sp->prevcp.x;
	unitp.y = sp->me.y - sp->prevcp.y;
	len = sqrt(unitp.x*unitp.x + unitp.y*unitp.y);
	if ( len==0 )
return;
	unitp.x /= len; unitp.y /= len;
	dotn = unitp.y*(sp->nextcp.x-sp->me.x) - unitp.x*(sp->nextcp.y-sp->me.y);
	dotp = unitn.y*(sp->me.x - sp->prevcp.x) - unitn.x*(sp->me.y - sp->prevcp.y);
	sp->nextcp.x -= dotn*unitp.y/2;
	sp->nextcp.y -= -dotn*unitp.x/2;
	sp->prevcp.x += dotp*unitn.y/2;
	sp->prevcp.y += -dotp*unitn.x/2;
	SplineRefigure(sp->prev); SplineRefigure(sp->next);
    }
    if ( sp->pointtype==pt_tangent && !sp->nonextcp ) {
	unitp.x = sp->me.x - sp->prev->from->me.x;
	unitp.y = sp->me.y - sp->prev->from->me.y;
	len = sqrt(unitp.x*unitp.x + unitp.y*unitp.y);
	if ( len!=0 ) {
	    unitp.x /= len; unitp.y /= len;
	    dot = unitp.y*(sp->nextcp.x-sp->me.x) - unitp.x*(sp->nextcp.y-sp->me.y);
	    sp->nextcp.x -= dot*unitp.y;
	    sp->nextcp.y -= -dot*unitp.x;
	    SplineRefigure(sp->next);
	}
    }
    if ( sp->pointtype==pt_tangent && !sp->noprevcp ) {
	unitn.x = sp->nextcp.x-sp->me.x;
	unitn.y = sp->nextcp.y-sp->me.y;
	len = sqrt(unitn.x*unitn.x + unitn.y*unitn.y);
	if ( len!=0 ) {
	    unitn.x /= len; unitn.y /= len;
	    dot = unitn.y*(sp->me.x-sp->prevcp.x) - unitn.x*(sp->me.y-sp->prevcp.y);
	    sp->prevcp.x += dot*unitn.y;
	    sp->prevcp.y += -dot*unitn.x;
	    SplineRefigure(sp->prev);
	}
    }
}

void SPTouchControl(SplinePoint *sp,BasePoint *which, int order2)
{
    BasePoint to = *which;
    SPAdjustControl( sp, which, &to, order2 );
}


void SPAdjustControl(SplinePoint *sp,BasePoint *cp, BasePoint *to,int order2) {
    BasePoint *othercp = cp==&sp->nextcp?&sp->prevcp:&sp->nextcp;
    int refig = false, otherchanged = false;

    if ( sp->ttfindex==0xffff && order2 ) {
	/* If the point itself is implied, then it's the control points that */
	/*  are fixed. Moving a CP should move the implied point so that it */
	/*  continues to be in the right place */
	sp->me.x = (to->x+othercp->x)/2;
	sp->me.y = (to->y+othercp->y)/2;
	*cp = *to;
	refig = true;
    } else if ( sp->pointtype==pt_corner ) {
	*cp = *to;
    } else if ( sp->pointtype==pt_curve || sp->pointtype==pt_hvcurve ) {
	if ( sp->pointtype==pt_hvcurve ) {
	    BasePoint diff;
	    diff.x = to->x - sp->me.x;
	    diff.y = to->y - sp->me.y;
	    BP_HVForce(&diff);
	    cp->x = sp->me.x + diff.x;
	    cp->y = sp->me.y + diff.y;
	} else {
	    *cp = *to;
	}
	if (( cp->x!=sp->me.x || cp->y!=sp->me.y ) &&
		(!order2 ||
		 (cp==&sp->nextcp && sp->next!=NULL && sp->next->to->ttfindex==0xffff) ||
		 (cp==&sp->prevcp && sp->prev!=NULL && sp->prev->from->ttfindex==0xffff)) ) {
	    bigreal len1, len2;
	    len1 = sqrt((cp->x-sp->me.x)*(cp->x-sp->me.x) +
			(cp->y-sp->me.y)*(cp->y-sp->me.y));
	    len2 = sqrt((othercp->x-sp->me.x)*(othercp->x-sp->me.x) +
			(othercp->y-sp->me.y)*(othercp->y-sp->me.y));
	    len2 /= len1;
	    othercp->x = len2 * (sp->me.x-cp->x) + sp->me.x;
	    othercp->y = len2 * (sp->me.y-cp->y) + sp->me.y;
	    otherchanged = true;
	    if ( sp->next!=NULL && othercp==&sp->nextcp ) {
		if ( order2 ) sp->next->to->prevcp = *othercp;
		SplineRefigure(sp->next);
	    } else if ( sp->prev!=NULL && othercp==&sp->prevcp ) {
		if ( order2 ) sp->prev->from->nextcp = *othercp;
		SplineRefigure(sp->prev);
	    }
	}
	if ( cp==&sp->nextcp ) sp->prevcpdef = false;
	else sp->nextcpdef = false;
    } else {
	BasePoint *bp;
	if ( cp==&sp->prevcp && sp->next!=NULL )
	    bp = &sp->next->to->me;
	else if ( cp==&sp->nextcp && sp->prev!=NULL )
	    bp = &sp->prev->from->me;
	else
	    bp = NULL;
	if ( bp!=NULL ) {
	    real angle = atan2(bp->y-sp->me.y,bp->x-sp->me.x);
	    real len = sqrt((bp->x-sp->me.x)*(bp->x-sp->me.x) + (bp->y-sp->me.y)*(bp->y-sp->me.y));
	    real dotprod =
		    ((to->x-sp->me.x)*(bp->x-sp->me.x) +
		     (to->y-sp->me.y)*(bp->y-sp->me.y));
	    if ( len!=0 ) {
		dotprod /= len;
		if ( dotprod>0 ) dotprod = 0;
		cp->x = sp->me.x + dotprod*cos(angle);
		cp->y = sp->me.y + dotprod*sin(angle);
	    }
	}
    }

    if ( order2 ) {
	if ( (cp==&sp->nextcp || otherchanged) && sp->next!=NULL ) {
	    SplinePoint *osp = sp->next->to;
	    if ( osp->ttfindex==0xffff ) {
		osp->prevcp = sp->nextcp;
		osp->me.x = (osp->prevcp.x+osp->nextcp.x)/2;
		osp->me.y = (osp->prevcp.y+osp->nextcp.y)/2;
		SplineRefigure(osp->next);
	    }
	}
	if ( (cp==&sp->prevcp || otherchanged) && sp->prev!=NULL ) {
	    SplinePoint *osp = sp->prev->from;
	    if ( osp->ttfindex==0xffff ) {
		osp->nextcp = sp->prevcp;
		osp->me.x = (osp->prevcp.x+osp->nextcp.x)/2;
		osp->me.y = (osp->prevcp.y+osp->nextcp.y)/2;
		SplineRefigure(osp->prev);
	    }
	}
    }

    if ( cp==&sp->nextcp ) sp->nextcpdef = false;
    else sp->prevcpdef = false;

    if ( sp->next!=NULL && cp==&sp->nextcp ) {
	if ( order2 && !sp->nonextcp ) {
	    sp->next->to->prevcp = *cp;
	    sp->next->to->noprevcp = false;
	}
	SplineRefigureFixup(sp->next);
    }
    if ( sp->prev!=NULL && cp==&sp->prevcp ) {
	if ( order2 && !sp->noprevcp ) {
	    sp->prev->from->nextcp = *cp;
	}
	SplineRefigureFixup(sp->prev);
    }
    if ( refig ) {
	SplineRefigure(sp->prev);
	SplineRefigure(sp->next);
    }
}

int PointListIsSelected(SplinePointList *spl) {
    int anypoints = 0;
    Spline *spline, *first;
    int i;

    first = NULL;
    if ( spl->first->selected ) anypoints = true;
    for ( spline=spl->first->next; spline!=NULL && spline!=first && !anypoints; spline = spline->to->next ) {
	if ( spline->to->selected ) anypoints = true;
	if ( first == NULL ) first = spline;
    }
    if ( !anypoints && spl->spiro_cnt!=0 ) {
	for ( i=0; i<spl->spiro_cnt-1; ++i )
	    if ( SPIRO_SELECTED(&spl->spiros[i]))
return( true );
    }
return( anypoints );
}

SplineSet *SplineSetReverse(SplineSet *spl) {
    Spline *spline, *first, *next;
    BasePoint tp;
    SplinePoint *temp;
    int flag;
    int i;
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
    flag = spline->from->nonextcp;
    spline->from->nonextcp = spline->from->noprevcp;
    spline->from->noprevcp = flag;
    flag = spline->from->nextcpdef;
    spline->from->nextcpdef = spline->from->prevcpdef;
    spline->from->prevcpdef = flag;
    flag = spline->from->nextcpselected;
    spline->from->nextcpselected = spline->from->prevcpselected;
    spline->from->prevcpselected = flag;

    for ( ; spline!=NULL && spline!=first; spline=next ) {
	next = spline->to->next;

	if ( spline->to!=spl->first ) {		/* On a closed spline don't want to reverse the first point twice */
	    tp = spline->to->nextcp;
	    spline->to->nextcp = spline->to->prevcp;
	    spline->to->prevcp = tp;
	    flag = spline->to->nonextcp;
	    spline->to->nonextcp = spline->to->noprevcp;
	    spline->to->noprevcp = flag;
	    flag = spline->to->nextcpdef;
	    spline->to->nextcpdef = spline->to->prevcpdef;
	    spline->to->prevcpdef = flag;
	    flag = spline->to->nextcpselected;
	    spline->to->nextcpselected = spline->to->prevcpselected;
	    spline->to->prevcpselected = flag;
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
	spl->start_offset = 0;
	spl->last = temp;
	spl->first->prev = NULL;
	spl->last->next = NULL;
    }

    if ( spl->spiro_cnt>2 ) {
	for ( i=(spl->spiro_cnt-1)/2-1; i>=0; --i ) {
	    spiro_cp temp_cp = spl->spiros[i];
	    spl->spiros[i] = spl->spiros[spl->spiro_cnt-2-i];
	    spl->spiros[spl->spiro_cnt-2-i] = temp_cp;
	}
	if ( (spl->spiros[spl->spiro_cnt-2].ty&0x7f)==SPIRO_OPEN_CONTOUR ) {
	    spl->spiros[spl->spiro_cnt-2].ty = (spl->spiros[0].ty&0x7f) | (spl->spiros[spl->spiro_cnt-2].ty&0x80);
	    spl->spiros[0].ty = SPIRO_OPEN_CONTOUR | (spl->spiros[0].ty&0x80);
	}
	for ( i=spl->spiro_cnt-2; i>=0; --i ) {
	    if ( (spl->spiros[i].ty&0x7f) == SPIRO_LEFT )
		spl->spiros[i].ty = SPIRO_RIGHT | (spl->spiros[i].ty&0x80);
	    else if ( (spl->spiros[i].ty&0x7f) == SPIRO_RIGHT )
		spl->spiros[i].ty = SPIRO_LEFT | (spl->spiros[i].ty&0x80);
	}
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

static int SSCheck(SplineSet *base,Edge *active, int up, EdgeList *es,int *changed) {
    SplineSet *spl;
    if ( active->spline->isticked )
return( 0 );
    spl = SplineSetOfSpline(base,active->spline);
    if ( active->up!=up ) {
	SplineSetReverse(spl);
	*changed = true;
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

void SplineSetsInsertOpen(SplineSet **tbase,SplineSet *open) {
    SplineSet *e, *p, *spl, *next;

    for ( p=NULL, spl=*tbase, e=open; e!=NULL; e = next ) {
	next = e->next;
	while ( spl!=NULL && spl->first->ttfindex<e->first->ttfindex ) {
	    p = spl;
	    spl = spl->next;
	}
	if ( p==NULL )
	    *tbase = e;
	else
	    p->next = e;
	e->next = spl;
	p = e;
    }
}

/* The idea behind SplineSetsCorrect is simple. However there are many splinesets */
/*  where it is impossible, so bear in mind that this only works for nice */
/*  splines. Figure 8's, intersecting splines all cause problems */
/* The outermost spline should be clockwise (up), the next splineset we find */
/*  should be down, if it isn't reverse it (if it's already been dealt with */
/*  then ignore it) */
SplineSet *SplineSetsCorrect(SplineSet *base,int *changed) {
    SplineSet *spl;
    int sscnt, check_cnt;
    EdgeList es;
    DBounds b;
    Edge *active=NULL, *apt, *pr, *e;
    int i, winding;

    *changed = false;

    SplineSetsUntick(base);
    for (sscnt=0,spl=base; spl!=NULL; spl=spl->next, ++sscnt );

    SplineSetFindBounds(base,&b);
    memset(&es,'\0',sizeof(es));
    es.scale = 1.0;
    es.mmin = floor(b.miny*es.scale);
    es.mmax = ceil(b.maxy*es.scale);
    es.omin = b.minx*es.scale;
    es.omax = b.maxx*es.scale;
    es.layer = ly_fore;		/* Not meaningful */

/* Give up if we are given unreasonable values (ie. if rounding errors might screw us up) */
    if ( es.mmin<1e5 && es.mmax>-1e5 && es.omin<1e5 && es.omax>-1e5 ) {
	es.cnt = (int)ceil(es.mmax-es.mmin) + 1;
	es.edges = calloc(es.cnt,sizeof(Edge *));
	es.interesting = calloc(es.cnt,sizeof(char));
	es.sc = NULL;
	es.major = 1; es.other = 0;
	FindEdgesSplineSet(base,&es,false);

	check_cnt = 0;
	for ( i=0; i<es.cnt && check_cnt<sscnt; ++i ) {
	    active = ActiveEdgesRefigure(&es,active,i);
	    if ( es.edges[i]!=NULL )
	continue;			/* Just too hard to get the edges sorted when we are at a start vertex */
	    if ( /*es.edges[i]==NULL &&*/ !es.interesting[i] &&
		    !(i>0 && es.interesting[i-1]) && !(i>0 && es.edges[i-1]!=NULL) &&
		    !(i<es.cnt-1 && es.edges[i+1]!=NULL) &&
		    !(i<es.cnt-1 && es.interesting[i+1]))	/* interesting things happen when we add (or remove) entries */
	continue;			/* and where we have extrema */
	    for ( apt=active; apt!=NULL; apt = e) {
		check_cnt += SSCheck(base,apt,true,&es,changed);
		winding = apt->up?1:-1;
		for ( pr=apt, e=apt->aenext; e!=NULL && winding!=0; pr=e, e=e->aenext ) {
		    if ( !e->spline->isticked )
			check_cnt += SSCheck(base,e,winding<0,&es,changed);
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
return( base );
}

SplineSet *SplineSetsAntiCorrect(SplineSet *base) {
    int changed;
    SplineSet *spl;

    SplineSetsCorrect(base,&changed);
    for ( spl = base; spl!=NULL; spl = spl->next )
	SplineSetReverse(spl);
return( base );
}

/* This is exactly the same as SplineSetsCorrect, but instead of correcting */
/*  problems we merely search for them and if we find any return the first */
SplineSet *SplineSetsDetectDir(SplineSet **_base,int *_lastscan) {
    SplineSet *ret, *base;
    EIList el;
    EI *active=NULL, *apt, *pr, *e;
    int i, winding,change,waschange;
    int lastscan = *_lastscan;
    SplineChar dummy;
    Layer layers[2];
    int cnt;

    base = *_base;

    memset(&el,'\0',sizeof(el));
    memset(&dummy,'\0',sizeof(dummy));
    memset(layers,0,sizeof(layers));
    el.layer = ly_fore;
    dummy.layers = layers;
    dummy.layer_cnt = 2;
    dummy.layers[ly_fore].splines = base;
    ELFindEdges(&dummy,&el);
    if ( el.coordmax[1]-el.coordmin[1] > 1.e6 ) {
	LogError( _("Warning: Unreasonably big splines. They will be ignored.\n") );
return( NULL );
    }
    el.major = 1;
    ELOrder(&el,el.major);

    ret = NULL;
    waschange = false;
    for ( i=0; i<el.cnt && ret==NULL; ++i ) {
	active = EIActiveEdgesRefigure(&el,active,i,1,&change);
	if ( i<=lastscan )
    continue;
	for ( apt=active, cnt=0; apt!=NULL; apt = apt->aenext , ++cnt );
	if ( el.ordered[i]!=NULL || el.ends[i] || cnt&1 ) {
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
		ret = SplineSetOfSpline(base,apt->spline);
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
    *_base = base;
    *_lastscan = i;
return( ret );
}

static int _SplinePointListIsClockwise(const SplineSet *spl, int max_depth) {
    EIList el;
    EI *active=NULL, *apt, *pr, *e;
    int i, winding,change,waschange, cnt;
    SplineChar dummy;
    SplineSet *next;
    Layer layers[2];
    int cw_cnt=0, ccw_cnt=0, l_cw_cnt, l_ccw_cnt, lines_processed = 0;

    memset(&el,'\0',sizeof(el));
    memset(&dummy,'\0',sizeof(dummy));
    memset(layers,0,sizeof(layers));
    el.layer = ly_fore;
    dummy.layers = layers;
    dummy.layer_cnt = 2;
    dummy.layers[ly_fore].splines = (SplineSet *) spl;
    dummy.name = "Clockwise Test";
    next = spl->next; ((SplineSet *) spl)->next = NULL;
    ELFindEdges(&dummy,&el);
    if ( el.coordmax[1]-el.coordmin[1] > 1.e6 ) {
	LogError( _("Warning: Unreasonably big splines. They will be ignored.\n") );
	((SplineSet *) spl)->next = next;
return( -1 );
    }
    el.major = 1;
    ELOrder(&el,el.major);

    waschange = false;
    for ( i=0; i<el.cnt ; ++i ) {
	l_cw_cnt = l_ccw_cnt = 0;
	active = EIActiveEdgesRefigure(&el,active,i,1,&change);
	for ( apt=active, cnt=0; apt!=NULL; apt = apt->aenext , ++cnt );
	// "Scan line" skip conditions:
	//   Edge starts or ends on this line
	//   Edge starts or ends on the following line
	//   Odd number of edges on this line
	//   On or immediately after a line marked "change" by EIAER
	if ( el.ordered[i]!=NULL || el.ends[i] || cnt&1 ||
		waschange || change ||
		(i!=el.cnt-1 && (el.ends[i+1] || el.ordered[i+1])) ) {
	    waschange = change;
	    continue;
	}
	waschange = change;
	for ( apt=active; apt!=NULL; apt = e) {
	    if ( EISkipExtremum(apt,i+el.low,1)) {
		e = apt->aenext->aenext;
		continue;
	    }
	    if ( apt->up )
		++l_cw_cnt;
	    else
		++l_ccw_cnt;
	    if ( (cw_cnt + l_cw_cnt)!=0 && (ccw_cnt + l_ccw_cnt)!=0 ) {
		((SplineSet *) spl)->next = next;
		free(el.ordered);
		free(el.ends);
		ElFreeEI(&el);
		return( -1 );
	    }
	    winding = apt->up?1:-1;
	    for ( pr=apt, e=apt->aenext; e!=NULL && winding!=0; pr=e, e=e->aenext ) {
		if ( EISkipExtremum(e,i+el.low,1)) {
		    e = e->aenext;
	    continue;
		}
		if ( pr->up!=e->up ) {
		    if ( (winding<=0 && !e->up) || (winding>0 && e->up )) {
			// This is an erroneous condition... but I don't think
			// it can actually happen with a single contour. I
			// think it is more likely this means a rounding error
			// and a problem in my algorithm
			l_cw_cnt = l_ccw_cnt = 0;
			break;
		    }
		    winding += (e->up?1:-1);
		} else if ( EISameLine(pr,e,i+el.low,1) )
		    // This just continues the line and doesn't change count
		    ;
		else {
		    if ( (winding<=0 && !e->up) || (winding>0 && e->up )) {
			l_cw_cnt = l_ccw_cnt = 0;
			break;
		    }
		    winding += (e->up?1:-1);
		}
	    }
	}
	cw_cnt += l_cw_cnt;
	ccw_cnt += l_ccw_cnt;
	if ( l_cw_cnt!=0 || l_ccw_cnt!=0 )
	    ++lines_processed;
    }
    free(el.ordered);
    free(el.ends);
    ElFreeEI(&el);

    ((SplineSet *) spl)->next = next;

    if (    ( lines_processed > 4 && ((float) lines_processed / el.cnt) > .33 )
         || ( max_depth && lines_processed > 0 ) ) {
	if ( cw_cnt!=0 && ccw_cnt==0 )
	    return true;
	else if ( cw_cnt==0 && ccw_cnt!=0 )
	    return false;
	else
	    return -1;
    }

    return -2;
}

int SplinePointListIsClockwise(const SplineSet *spl) {
    SplineSet *cpy;
    const SplineSet *pass;
    SplinePoint *sp;
    int r, depth=0, mag=1, y, ymin=INT_MAX, ymax=INT_MIN, pt_cnt=0;

    while ( depth<3 ) {
	if ( mag!=1 ) {
	    cpy = SplinePointListCopy1(spl);
	    real trans[6] = { mag, 0.0, 0.0, mag, 0.0, 0.0 };
	    SplinePointListTransformExtended(cpy,trans,tpt_AllPoints,
	                                     tpmask_dontTrimValues);
	    pass = cpy;
	} else {
	    cpy = NULL;
	    pass = spl;
	}
	r = _SplinePointListIsClockwise(pass, depth==2);
	if ( cpy!=NULL )
	    SplinePointListFree(cpy);
	if ( r>=-1 )
	    return r;
	// Bad run, prepare for next
	if ( depth==0 ) {
	    // Check for open or single-point splines and
	    // further magnify small splines
	    for ( sp = spl->first; ; ) {
		if ( sp->next==NULL )
		    return -1; // Open Spline
		++pt_cnt;
		y = floor(sp->me.y);
		if ( y < ymin )
		    ymin = y;
		y = ceil(sp->me.y);
		if ( y > ymax )
		    ymax = y;
		sp = sp->next->to;
		if ( sp==spl->first )
		    break;
	    }
	    if ( pt_cnt==1 )
		return -1; // Single point spline
	    y = ymax - ymin + 1;
	    if ( y < pt_cnt + 7 )
		mag = (int) (7 + pt_cnt)/y;
	}
	mag *= 3;
	++depth;
    }
    mag /= 3;
    LogError( _("Warning: SplinePointListIsClockwise found no usable line even at %dx magnification.\n"), mag );
    return -1;
}

/* Since this function now deals with 4 arbitrarily selected points, */
/* it has to try to combine them by different ways in order to see */
/* if they actually can specify a diagonal stem. The reordered points */
/* are placed back to array passed to the function.*/
int PointsDiagonalable( SplineFont *sf,BasePoint **bp,BasePoint *unit ) {
    BasePoint *line1[2], *line2[2], *temp, *base;
    BasePoint unit1, unit2;
    int i, j, k;
    bigreal dist_error_diag, len1, len2, width, dot;
    bigreal off1, off2;

    for ( i=0; i<4; i++ ) {
        if ( bp[i] == NULL )
return( false );
    }

    dist_error_diag = 0.0065 * ( sf->ascent + sf->descent );
    /* Assume that the first point passed to the function is the starting */
    /* point of the first of two vectors. Then try all possible combinations */
    /* (there are 3), ensure the vectors are consistently ordered, and */
    /* check if they are parallel.*/
    base = bp[0];
    for ( i=1; i<4; i++ ) {
        line1[0] = base; line1[1] = bp[i];
        
        k=0;
        for ( j=1; j<4; j++ ) {
            if ( j != i )
                line2[k++] = bp[j];
        }
        unit1.x = line1[1]->x - line1[0]->x;
        unit1.y = line1[1]->y - line1[0]->y;
        unit2.x = line2[1]->x - line2[0]->x;
        unit2.y = line2[1]->y - line2[0]->y;
        /* No horizontal, vertical edges */
        if ( unit1.x == 0 || unit1.y == 0 || unit2.x == 0 || unit2.y == 0 )
    continue;
        len1 = sqrt( pow( unit1.x,2 ) + pow( unit1.y,2 ));
        len2 = sqrt( pow( unit2.x,2 ) + pow( unit2.y,2 ));
        unit1.x /= len1; unit1.y /= len1;
        unit2.x /= len2; unit2.y /= len2;
        dot = unit1.x * unit2.y - unit1.y * unit2.x;
        /* Units parallel */
        if ( dot <= -.05 || dot >= .05 )
    continue;
        /* Ensure vectors point by such a way that the angle is between 90 and 270 degrees */
        if ( unit1.x <  0 ) {
            temp = line1[0]; line1[0] = line1[1]; line1[1] = temp;
            unit1.x = -unit1.x; unit1.y = -unit1.y;
        }
        if ( unit2.x <  0 ) {
            temp = line2[0]; line2[0] = line2[1]; line2[1] = temp;
            unit2.x = -unit2.x; unit2.y = -unit2.y;
        }
        off1 =  ( line1[1]->x - line1[0]->x ) * unit2.y -
                ( line1[1]->y - line1[0]->y ) * unit2.x;
        off2 =  ( line2[1]->x - line2[0]->x ) * unit1.y -
                ( line2[1]->y - line2[0]->y ) * unit1.x;
        if ( len1 > len2 && fabs( off2 ) < 2*dist_error_diag ) *unit = unit1;
        else if ( fabs( off1 ) < 2*dist_error_diag ) *unit = unit2;
        else
    continue;
        width = ( line2[0]->x - line1[0]->x ) * unit->y -
                ( line2[0]->y - line1[0]->y ) * unit->x;
        /* Make sure this is a real line, rather than just two */
        /* short spline segments which occasionally have happened to be */
        /* parallel. This is necessary to correctly handle things which may */
        /* be "diagonalable" in 2 different directions (like slash in some */
        /* designs). */
        if ( fabs( width ) > len1 || fabs( width ) > len2 )
    continue;
	/* Make sure line2 is further right than line1 */
        if ( width < 0 ) {
	    temp = line1[0]; line1[0] = line2[0]; line2[0] = temp;
	    temp = line1[1]; line1[1] = line2[1]; line2[1] = temp;
        }
        bp[0] = line1[0];
        bp[1] = line2[0];
        bp[2] = line1[1];
        bp[3] = line2[1];
return( true );
    }
return( false );
}

int SplineTurningCCWAt(Spline *s, bigreal t) {
    bigreal tmp = SecondDerivative(s, t);
    // For missing curvatures take a very small step away and try again.
    if ( RealWithin(tmp,0,1e-9) ) {
	tmp = SecondDerivative(s, (t+1e-8 <= 1.0) ? t+1e-8 : t-1e-8 );
    }
    return tmp > 0;
}
