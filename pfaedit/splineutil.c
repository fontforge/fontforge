/* Copyright (C) 2000,2001 by George Williams */
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
#include "splinefont.h"
#include "psfont.h"
#include "ustring.h"
#include "string.h"
#include "utype.h"
#include "views.h"		/* for FindSel structure */

/*#define DEBUG 1*/

void LineListFree(LineList *ll) {
    LineList *next;

    while ( ll!=NULL ) {
	next = ll->next;
	free(ll);
	ll = next;
    }
}

void LinearApproxFree(LinearApprox *la) {
    LinearApprox *next;

    while ( la!=NULL ) {
	next = la->next;
	LineListFree(la->lines);
	free(la);
	la = next;
    }
}

void SplineFree(Spline *spline) {
    LinearApproxFree(spline->approx);
    free(spline);
}

void SplinePointFree(SplinePoint *sp) {
    free(sp);
}

void SplinePointListFree(SplinePointList *spl) {
    Spline *first, *spline, *next;

    if ( spl==NULL )
return;
    first = NULL;
    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = next ) {
	next = spline->to->next;
	SplinePointFree(spline->to);
	SplineFree(spline);
	if ( first==NULL ) first = spline;
    }
    if ( spl->last!=spl->first || spl->first->next==NULL )
	SplinePointFree(spl->first);
    free(spl);
}

void SplinePointListsFree(SplinePointList *head) {
    SplinePointList *spl, *next;

    for ( spl=head; spl!=NULL; spl=next ) {
	next = spl->next;
	SplinePointListFree(spl);
    }
}

void RefCharFree(RefChar *ref) {

    if ( ref==NULL )
return;
    SplinePointListsFree(ref->splines);
    free(ref);
}

void RefCharsFree(RefChar *ref) {
    RefChar *rnext;

    while ( ref!=NULL ) {
	rnext = ref->next;
	RefCharFree(ref);
	ref = rnext;
    }
}

void ImageListsFree(ImageList *imgs) {
    ImageList *inext;

    while ( imgs!=NULL ) {
	inext = imgs->next;
	free(imgs);
	imgs = inext;
    }
}

void SplineRefigure(Spline *spline) {
    SplinePoint *from = spline->from, *to = spline->to;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];

#ifdef DEBUG
    if ( DoubleNear(from->me.x,to->me.x) && DoubleNear(from->me.y,to->me.y))
	GDrawIError("Zero length spline created");
#endif
    xsp->d = from->me.x; ysp->d = from->me.y;
    if ( from->nonextcp ) from->nextcp = from->me;
    else if ( from->nextcp.x==from->me.x && from->nextcp.y == from->me.y ) from->nonextcp = true;
    if ( to->noprevcp ) to->prevcp = to->me;
    else if ( to->prevcp.x==to->me.x && to->prevcp.y == to->me.y ) to->noprevcp = true;
    if ( from->nonextcp && to->noprevcp ) {
	spline->islinear = true;
	xsp->c = to->me.x-from->me.x;
	ysp->c = to->me.y-from->me.y;
	xsp->a = xsp->b = 0;
	ysp->a = ysp->b = 0;
    } else {
	/* from p. 393 (Operator Details, curveto) Postscript Lang. Ref. Man. (Red book) */
	xsp->c = 3*(from->nextcp.x-from->me.x);
	ysp->c = 3*(from->nextcp.y-from->me.y);
	xsp->b = 3*(to->prevcp.x-from->nextcp.x)-xsp->c;
	ysp->b = 3*(to->prevcp.y-from->nextcp.y)-ysp->c;
	xsp->a = to->me.x-from->me.x-xsp->c-xsp->b;
	ysp->a = to->me.y-from->me.y-ysp->c-ysp->b;
	if ( DoubleNear(xsp->c,0)) xsp->c=0;
	if ( DoubleNear(ysp->c,0)) ysp->c=0;
	if ( DoubleNear(xsp->b,0)) xsp->b=0;
	if ( DoubleNear(ysp->b,0)) ysp->b=0;
	if ( DoubleNear(xsp->a,0)) xsp->a=0;
	if ( DoubleNear(ysp->a,0)) ysp->a=0;
	spline->islinear = false;
	if ( ysp->a==0 && xsp->a==0 && ysp->b==0 && xsp->b==0 )
	    spline->islinear = true;	/* I'm not sure if this can happen */
    }
    LinearApproxFree(spline->approx);
    spline->approx = NULL;
    spline->knowncurved = false;
    spline->knownlinear = spline->islinear;
    SplineIsLinear(spline);
    if ( !spline->islinear && spline->knownlinear ) {
	spline->knownlinear = false;
	SplineIsLinear(spline);		/* Debug */
    }
}

Spline *SplineMake(SplinePoint *from, SplinePoint *to) {
    Spline *spline = calloc(1,sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    SplineRefigure(spline);
return( spline );
}

static double SolveCubic(double a, double b, double c, double d, double err, double t0) {
    /* find t between t0 and .5 where at^3+bt^2+ct+d == +/- err */
    double t, val, offset;
    int first;

    offset=a;
    if ( a<0 ) offset = -a;
    if ( b<0 ) offset -= b; else offset += b;
    if ( c<0 ) offset -= c; else offset += c;
    offset += 1;		/* Make sure it isn't 0 */
    offset = err/(10.*offset);
    if ( offset<.00001 ) offset = .00001; else if ( offset>.01 ) offset = .01;

    first = 1;
    for ( t=t0+offset; t<.5; t += offset ) {
	val = ((a*t+b)*t+c)*t+d;
	if ( val>=err || val<=-err )
    break;
	first = 0;
    }
    if ( !first )
	t -= offset;
    if ( t>.5 ) t=.5;

return( t );
}

static double SolveCubicBack(double a, double b, double c, double d, double err, double t0) {
    /* find t between .5 and t0 where at^3+bt^2+ct+d == +/- err */
    double t, val, offset;
    int first;

    offset=a;
    if ( a<0 ) offset = -a;
    if ( b<0 ) offset -= b; else offset += b;
    if ( c<0 ) offset -= c; else offset += c;
    offset += 1;		/* Make sure it isn't 0 */
    offset = err/(10.*offset);
    if ( offset<.00001 ) offset = .00001; else if ( offset>.01 ) offset = .01;

    first = 1;
    for ( t=t0-offset; t>.5; t -= offset ) {
	val = ((a*t+b)*t+c)*t+d;
	if ( val>=err || val<=-err )
    break;
	first = 0;
    }
    if ( !first )
	t -= offset;
    if ( t<.5 ) t=.5;

return( t );
}
    
/* Remove line segments which are just one point long */
/* Merge colinear line segments */
/* Merge two segments each of which involves a single pixel change in one dimension (cut corners) */ 
static void SimplifyLineList(LineList *prev) {
    LineList *next, *lines;

    if ( prev->next==NULL )
return;
    lines = prev->next;
    while ( (next = lines->next)!=NULL ) {
	if ( (prev->here.x==lines->here.x && prev->here.y == lines->here.y ) ||
		( prev->here.x==lines->here.x && lines->here.x==next->here.x ) ||
		( prev->here.y==lines->here.y && lines->here.y==next->here.y ) ||
		(( prev->here.x==next->here.x+1 || prev->here.x==next->here.x-1 ) &&
		 ( prev->here.y==next->here.y+1 || prev->here.y==next->here.y-1 )) ) {
	    lines->here = next->here;
	    lines->next = next->next;
	    free(next);
	} else {
	    prev = lines;
	    lines = next;
	}
    }
    if ( prev!=NULL &&
	    prev->here.x==lines->here.x && prev->here.y == lines->here.y ) {
	prev->next = NULL;
	free(lines);
    }

    while ( (next = lines->next)!=NULL ) {
	if ( prev->here.x!=next->here.x ) {
	    double slope = (next->here.y-prev->here.y) / (double) (next->here.x-prev->here.x);
	    double inter = prev->here.y - slope*prev->here.x;
	    int y = rint(lines->here.x*slope + inter);
	    if ( y == lines->here.y ) {
		lines->here = next->here;
		lines->next = next->next;
		free(next);
	    } else
		lines = next;
	} else
	    lines = next;
    }
}

LinearApprox *SplineApproximate(Spline *spline, double scale) {
    LinearApprox *test;
    LineList *cur, *last=NULL, *prev;
    double tx,ty,t;
    double slpx, slpy;
    double intx, inty;

    for ( test = spline->approx; test!=NULL && test->scale!=scale; test = test->next );
    if ( test!=NULL )
return( test );

    test = calloc( 1,sizeof(LinearApprox));
    test->scale = scale;
    test->next = spline->approx;
    spline->approx = test;

    cur = calloc( 1,sizeof(LineList) );
    cur->here.x = rint(spline->from->me.x*scale);
    cur->here.y = rint(spline->from->me.y*scale);
    test->lines = last = cur;
    if ( spline->knownlinear ) {
	cur = calloc( 1,sizeof(LineList) );
	cur->here.x = rint(spline->to->me.x*scale);
	cur->here.y = rint(spline->to->me.y*scale);
	last->next = cur;
    } else {
	/* find t so that (xt,yt) is a half pixel off from (cx*t+dx,cy*t+dy) */
	/* min t of scale*(ax*t^3+bx*t^2)==.5, scale*(ay*t^3+by*t^2)==.5 */
	/* I do this from both ends in. this is because I miss essential */
	/*  symmetry if I go from one end to the other. */
	/* first start at 0 and go to .5, the first linear approximation is easy */
	/*  it's just the function itself ignoring higher orders, so the error*/
	/*  is just the higher orders */
	tx = SolveCubic(spline->splines[0].a,spline->splines[0].b,0,0,.5/scale,0);
	ty = SolveCubic(spline->splines[1].a,spline->splines[1].b,0,0,.5/scale,0);
	t = (tx<ty)?tx:ty;
	cur = calloc( 1,sizeof(LineList) );
	cur->here.x = rint( (((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d)*scale );
	cur->here.y = rint( (((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d)*scale );
	last->next = cur;
	last = cur;
	while ( t<.5 ) {
	    slpx = (3*spline->splines[0].a*t+2*spline->splines[0].b)*t+spline->splines[0].c;
	    slpy = (3*spline->splines[1].a*t+2*spline->splines[1].b)*t+spline->splines[1].c;
	    intx = ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c-slpx)*t + spline->splines[0].d;
	    inty = ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c-slpy)*t + spline->splines[1].d;
	    tx = SolveCubic(spline->splines[0].a,spline->splines[0].b,spline->splines[0].c-slpx,spline->splines[0].d-intx,.5/scale,t);
	    ty = SolveCubic(spline->splines[1].a,spline->splines[1].b,spline->splines[1].c-slpy,spline->splines[1].d-inty,.5/scale,t);
	    t = (tx<ty)?tx:ty;
	    cur = calloc(1,sizeof(LineList));
	    cur->here.x = rint( (((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d)*scale );
	    cur->here.y = rint( (((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d)*scale );
	    last->next = cur;
	    last = cur;
	}

	/* Now start at t=1 and work back to t=.5 */
	prev = NULL;
	cur = calloc( 1,sizeof(LineList) );
	cur->here.x = rint(spline->to->me.x*scale);
	cur->here.y = rint(spline->to->me.y*scale);
	prev = cur;
	t=1.0;
	while ( 1 ) {
	    slpx = (3*spline->splines[0].a*t+2*spline->splines[0].b)*t+spline->splines[0].c;
	    slpy = (3*spline->splines[1].a*t+2*spline->splines[1].b)*t+spline->splines[1].c;
	    intx = ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c-slpx)*t + spline->splines[0].d;
	    inty = ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c-slpy)*t + spline->splines[1].d;
	    tx = SolveCubicBack(spline->splines[0].a,spline->splines[0].b,spline->splines[0].c-slpx,spline->splines[0].d-intx,.5/scale,t);
	    ty = SolveCubicBack(spline->splines[1].a,spline->splines[1].b,spline->splines[1].c-slpy,spline->splines[1].d-inty,.5/scale,t);
	    t = (tx>ty)?tx:ty;
	    cur = calloc( 1,sizeof(LineList) );
	    cur->here.x = rint( (((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d)*scale );
	    cur->here.y = rint( (((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d)*scale );
	    cur->next = prev;
	    prev = cur;
	    if ( t<=.5 )
	break;
	}
	last->next = cur;
	SimplifyLineList(test->lines);
    }
    if ( test->lines->next==NULL ) {
	test->oneline = 1;
	test->onepoint = 1;
    } else if ( test->lines->next->next == NULL ) {
	test->oneline = 1;
    }
return( test );
}

static void SplineFindBounds(Spline *sp, DBounds *bounds) {
    double t, b2_fourac, v;
    double min, max;
    Spline1D *sp1;
    int i;

    /* first try the end points */
    for ( i=0; i<2; ++i ) {
	sp1 = &sp->splines[i];
	if ( i==0 ) {
	    if ( sp->to->me.x<bounds->minx ) bounds->minx = sp->to->me.x;
	    if ( sp->to->me.x>bounds->maxx ) bounds->maxx = sp->to->me.x;
	    min = bounds->minx; max = bounds->maxx;
	} else {
	    if ( sp->to->me.y<bounds->miny ) bounds->miny = sp->to->me.y;
	    if ( sp->to->me.y>bounds->maxy ) bounds->maxy = sp->to->me.y;
	    min = bounds->miny; max = bounds->maxy;
	}

	/* then try the extrema of the spline (assuming they are between t=(0,1) */
	if ( sp1->a!=0 ) {
	    b2_fourac = 4*sp1->b*sp1->b - 12*sp1->a*sp1->c;
	    if ( b2_fourac>=0 ) {
		b2_fourac = sqrt(b2_fourac);
		t = (-2*sp1->b + b2_fourac) / (6*sp1->a);
		if ( t>0 && t<1 ) {
		    v = ((sp1->a*t+sp1->b)*t+sp1->c)*t + sp1->d;
		    if ( v<min ) min = v;
		    if ( v>max ) max = v;
		}
		t = (-2*sp1->b - b2_fourac) / (6*sp1->a);
		if ( t>0 && t<1 ) {
		    v = ((sp1->a*t+sp1->b)*t+sp1->c)*t + sp1->d;
		    if ( v<min ) min = v;
		    if ( v>max ) max = v;
		}
	    }
	} else if ( sp1->b!=0 ) {
	    t = -sp1->c/(2.0*sp1->b);
	    if ( t>0 && t<1 ) {
		v = (sp1->b*t+sp1->c)*t + sp1->d;
		if ( v<min ) min = v;
		if ( v>max ) max = v;
	    }
	}
	if ( i==0 ) {
	    bounds->minx = min; bounds->maxx = max;
	} else {
	    bounds->miny = min; bounds->maxy = max;
	}
    }
}

static void _SplineSetFindBounds(SplinePointList *spl, DBounds *bounds) {
    Spline *spline, *first;

    for ( ; spl!=NULL; spl = spl->next ) {
	first = NULL;
	if ( bounds->minx==0 && bounds->maxx==0 && bounds->miny==0 && bounds->maxy == 0 ) {
	    bounds->minx = bounds->maxx = spl->first->me.x;
	    bounds->miny = bounds->maxy = spl->first->me.y;
	} else {
	    if ( spl->first->me.x<bounds->minx ) bounds->minx = spl->first->me.x;
	    if ( spl->first->me.x>bounds->maxx ) bounds->maxx = spl->first->me.x;
	    if ( spl->first->me.y<bounds->miny ) bounds->miny = spl->first->me.y;
	    if ( spl->first->me.y>bounds->maxy ) bounds->maxy = spl->first->me.y;
	}
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    SplineFindBounds(spline,bounds);
	    if ( first==NULL ) first = spline;
	}
    }
}

void SplineSetFindBounds(SplinePointList *spl, DBounds *bounds) {
    memset(bounds,'\0',sizeof(*bounds));
    _SplineSetFindBounds(spl,bounds);
}

int SplinePointListIsClockwise(SplineSet *spl) {
    Spline *leftmost = NULL, *spline, *first;
    double xmin;
    DBounds bounds;

    if ( spl->first!=spl->last || spl->first->next == NULL )
return( -1 );		/* Open paths, (open paths with only one point are a special case) */

    first = NULL;
    leftmost = spl->first->next;
    while ( leftmost!=first && leftmost->from->me.y==leftmost->to->me.y ) {
	if ( first==NULL ) first = leftmost;
	leftmost = leftmost->to->next;
    }
    xmin = bounds.minx = bounds.maxx = leftmost->from->me.x;
    bounds.miny = bounds.maxy = leftmost->from->me.y;
    
    first = NULL;
    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	if ( spline->from->me.y!=spline->to->me.y ) {
	    bounds.minx = spline->from->me.x;	/* only bound we care about (bounds of this spline) */
	    SplineFindBounds(spline,&bounds);
	    if ( bounds.minx<xmin ) {
		leftmost = spline;
		xmin = bounds.minx;
	    }
	}
	if ( first==NULL ) first = spline;
    }
    if ( leftmost==NULL )
return( -1 );

    if ( xmin==leftmost->from->me.x ) {
return( leftmost->to->me.y>leftmost->from->prev->from->me.y );
    } else if ( xmin==leftmost->to->me.x ) {
return( leftmost->from->me.y<leftmost->to->next->to->me.y );
    } else
return( leftmost->from->me.y<leftmost->to->me.y );
}

void SplineCharFindBounds(SplineChar *sc,DBounds *bounds) {
    RefChar *rf;

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;
    bounds->miny = bounds->maxy = 0;

    for ( rf=sc->refs; rf!=NULL; rf = rf->next )
	_SplineSetFindBounds(rf->splines,bounds);

    _SplineSetFindBounds(sc->splines,bounds);
}

void SplineFontFindBounds(SplineFont *sf,DBounds *bounds) {
    RefChar *rf;
    int i;

    bounds->minx = bounds->maxx = 0;
    bounds->miny = bounds->maxy = 0;

    for ( i = 0; i<sf->charcnt; ++i ) {
	SplineChar *sc = sf->chars[i];
	if ( sc!=NULL ) {
	    for ( rf=sc->refs; rf!=NULL; rf = rf->next )
		_SplineSetFindBounds(rf->splines,bounds);

	    _SplineSetFindBounds(sc->splines,bounds);
	}
    }
}

void SplinePointCatagorize(SplinePoint *sp) {

    sp->pointtype = pt_corner;
    if ( sp->next==NULL && sp->prev==NULL )
	;
    else if ( (sp->next!=NULL && sp->next->to->me.x==sp->me.x && sp->next->to->me.y==sp->me.y) ||
	    (sp->prev!=NULL && sp->prev->from->me.x==sp->me.x && sp->prev->from->me.y==sp->me.y ))
	;
    else if ( sp->next==NULL ) {
	sp->pointtype = sp->noprevcp ? pt_corner : pt_curve;
    } else if ( sp->prev==NULL ) {
	sp->pointtype = sp->nonextcp ? pt_corner : pt_curve;
    } else if ( sp->nonextcp && sp->noprevcp ) {
	;
    } else if ( !sp->nonextcp && !sp->noprevcp ) {
	if ( sp->nextcp.y==sp->prevcp.y && sp->nextcp.y==sp->me.y )
	    sp->pointtype = pt_curve;
	else if ( sp->nextcp.x!=sp->prevcp.x ) {
	    double slope = (sp->nextcp.y-sp->prevcp.y)/(sp->nextcp.x-sp->prevcp.x);
	    double y = slope*(sp->me.x-sp->prevcp.x) + sp->prevcp.y - sp->me.y;
	    if ( y<1 && y>-1 )
		sp->pointtype = pt_curve;
	    else if ( sp->nextcp.y!=sp->prevcp.y ) {
		double x;
		slope = (sp->nextcp.x-sp->prevcp.x)/(sp->nextcp.y-sp->prevcp.y);
		x = slope*(sp->me.y-sp->prevcp.y) + sp->prevcp.x - sp->me.x;
		if ( x<1 && x>-1 )
		    sp->pointtype = pt_curve;
	    } else if ( sp->me.y == sp->nextcp.y )
		sp->pointtype = pt_curve;
	} else if ( sp->me.x == sp->nextcp.x )
	    sp->pointtype = pt_curve;
    } else if ( sp->nonextcp ) {
	if ( sp->next->to->me.x!=sp->prevcp.x ) {
	    double slope = (sp->next->to->me.y-sp->prevcp.y)/(sp->next->to->me.x-sp->prevcp.x);
	    double y = slope*(sp->me.x-sp->prevcp.x) + sp->prevcp.y - sp->me.y;
	    if ( y<1 && y>-1 )
		sp->pointtype = pt_tangent;
	} else if ( sp->me.x == sp->prevcp.x )
	    sp->pointtype = pt_tangent;
    } else {
	if ( sp->nextcp.x!=sp->prev->from->me.x ) {
	    double slope = (sp->nextcp.y-sp->prev->from->me.y)/(sp->nextcp.x-sp->prev->from->me.x);
	    double y = slope*(sp->me.x-sp->prev->from->me.x) + sp->prev->from->me.y - sp->me.y;
	    if ( y<1 && y>-1 )
		sp->pointtype = pt_tangent;
	} else if ( sp->me.x == sp->nextcp.x )
	    sp->pointtype = pt_tangent;
    }
}

void SCCatagorizePoints(SplineChar *sc) {
    Spline *spline, *first, *last=NULL;
    SplinePointList *spl;

    for ( spl = sc->splines; spl!=NULL; spl = spl->next ) {
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    SplinePointCatagorize(spline->from);
	    last = spline;
	    if ( first==NULL ) first = spline;
	}
	if ( spline==NULL && last!=NULL )
	    SplinePointCatagorize(last->to);
    }
}

static int CharsNotInEncoding(FontDict *fd) {
    int i, cnt, j;
    for ( i=cnt=0; i<fd->chars->cnt; ++i ) {
	if ( fd->chars->keys[i]!=NULL ) {
	    for ( j=0; j<256; ++j )
		if ( fd->encoding[j]!=NULL &&
			strcmp(fd->encoding[j],fd->chars->keys[i])==0 )
	    break;
	    if ( j==256 )
		++cnt;
	}
    }
    /* And for type 3 fonts... */
    for ( i=0; i<fd->charprocs->cnt; ++i ) {
	if ( fd->charprocs->keys[i]!=NULL ) {
	    for ( j=0; j<256; ++j )
		if ( fd->encoding[j]!=NULL &&
			strcmp(fd->encoding[j],fd->charprocs->keys[i])==0 )
	    break;
	    if ( j==256 )
		++cnt;
	}
    }
return( cnt );
}

static int LookupCharString(char *encname,struct pschars *chars) {
    int k;

    for ( k=0; k<chars->cnt; ++k ) {
	if ( chars->keys[k]!=NULL )
	    if ( strcmp(encname,chars->keys[k])==0 )
return( k );
    }
return( -1 );
}

int UnicodeNameLookup(char *name) {
    int i;

    if ( name==NULL )
return( -1 );

    if ( *name=='u' && name[1]=='n' && name[2]=='i' &&
	    (isdigit(name[3]) || (name[3]>='A' && name[3]<='F')) &&
	    (isdigit(name[4]) || (name[4]>='A' && name[4]<='F')) &&
	    (isdigit(name[5]) || (name[5]>='A' && name[5]<='F')) &&
	    (isdigit(name[6]) || (name[6]>='A' && name[6]<='F')) &&
	    name[7]=='\0' ) {
return( strtol(name+3,NULL,16));
    }
    for ( i=0; i<psunicodenames_cnt; ++i ) if ( psunicodenames[i]!=NULL )
	if ( strcmp(name,psunicodenames[i])==0 )
return( i );
    if ( strcmp(name,"nonbreakingspace")==0 )
return( 0xa0 );

return( -1 );
}

static SplinePointList *SplinePointListCopy1(SplinePointList *spl) {
    SplinePointList *cur;
    SplinePoint *pt, *cpt, *first;
    Spline *spline;

    cur = calloc(1,sizeof(SplinePointList));

    first = NULL;
    for ( pt=spl->first; pt!=NULL && pt!=first; pt = pt->next->to ) {
	cpt = malloc(sizeof(SplinePoint));
	*cpt = *pt;
	cpt->next = cpt->prev = NULL;
	if ( cur->first==NULL )
	    cur->first = cur->last = cpt;
	else {
	    spline = malloc(sizeof(Spline));
	    *spline = *pt->prev;
	    spline->from = cur->last;
	    cur->last->next = spline;
	    cpt->prev = spline;
	    spline->to = cpt;
	    spline->approx = NULL;
	    cur->last = cpt;
	}
	if ( pt->next==NULL )
    break;
	if ( first==NULL ) first = pt;
    }
    if ( pt==first ) {
	cpt = cur->first;
	spline = malloc(sizeof(Spline));
	*spline = *pt->prev;
	spline->from = cur->last;
	cur->last->next = spline;
	cpt->prev = spline;
	spline->to = cpt;
	spline->approx = NULL;
	cur->last = cpt;
    }
return( cur );
}

/* If this routine is called we are guarenteed that:
    at least one point on the splineset is selected
    not all points on the splineset are selected
*/
static SplinePointList *SplinePointListCopySelected1(SplinePointList *spl) {
    SplinePointList *head=NULL, *last=NULL, *cur;
    SplinePoint *cpt, *first, *start;
    Spline *spline;

    start = spl->first;
    if ( spl->first==spl->last ) {
	/* If it's a circle and the start point is selected then we don't know*/
	/*  where that selection began (and we have to keep it with the things*/
	/*  that precede it when we make the new splinesets), so loop until we*/
	/*  find something unselected */
	while ( start->selected )
	    start = start->next->to;
    }
    first = NULL;
    while ( start != NULL && start!=first ) {
	while ( start!=NULL && start!=first && !start->selected ) {
	    if ( first==NULL ) first = start;
	    if ( start->next==NULL ) {
		start = NULL;
	break;
	    }
	    start = start->next->to;
	}
	if ( start==NULL || start==first )
    break;
	cur = calloc(1,sizeof(SplinePointList));
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;

	while ( start!=NULL && start->selected && start!=first ) {
	    cpt = malloc(sizeof(SplinePoint));
	    *cpt = *start;
	    cpt->next = cpt->prev = NULL;
	    if ( cur->first==NULL )
		cur->first = cur->last = cpt;
	    else {
		spline = malloc(sizeof(Spline));
		*spline = *start->prev;
		spline->from = cur->last;
		cur->last->next = spline;
		cpt->prev = spline;
		spline->to = cpt;
		spline->approx = NULL;
		cur->last = cpt;
	    }
	    if ( first==NULL ) first = start;
	    if ( start->next==NULL ) {
		start = NULL;
	break;
	    }
	    start = start->next->to;
	}
    }
return( head );
}

SplinePointList *SplinePointListCopy(SplinePointList *base) {
    SplinePointList *head=NULL, *last=NULL, *cur;

    for ( ; base!=NULL; base = base->next ) {
	cur = SplinePointListCopy1(base);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
return( head );
}

SplinePointList *SplinePointListCopySelected(SplinePointList *base) {
    SplinePointList *head=NULL, *last=NULL, *cur=NULL;
    SplinePoint *pt, *first;
    int anysel, allsel;

    for ( ; base!=NULL; base = base->next ) {
	anysel = false; allsel = true;
	first = NULL;
	for ( pt=base->first; pt!=NULL && pt!=first; pt = pt->next->to ) {
	    if ( pt->selected ) anysel = true;
	    else allsel = false;
	    if ( first==NULL ) first = pt;
	    if ( pt->next==NULL )
	break;
	}
	if ( allsel )
	    cur = SplinePointListCopy1(base);
	else if ( anysel )
	    cur = SplinePointListCopySelected1(base);
	if ( anysel ) {
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
    }
return( head );
}

static SplinePointList *SplinePointListSplit(SplinePointList *spl) {
    SplinePointList *head=NULL, *last=NULL, *cur;
    SplinePoint *first, *start, *next;

    start = spl->first;
    if ( spl->first==spl->last ) {
	/* If it's a circle and the start point isnt selected then we don't know*/
	/*  where that selection began (and we have to keep it with the things*/
	/*  that precede it when we make the new splinesets), so loop until we*/
	/*  find something selected */
	while ( !start->selected )
	    start = start->next->to;
    }
    first = NULL;
    while ( start != NULL && start!=first ) {
	while ( start!=NULL && start!=first && start->selected ) {
	    if ( first==NULL ) first = start;
	    if ( start->prev!=NULL ) {
		start->prev->from->next = NULL;
		SplineFree(start->prev);
	    }
	    if ( start->next!=NULL ) {
		next = start->next->to;
		next->prev = NULL;
		SplineFree(start->next);
	    } else
		next = NULL;
	    SplinePointFree(start);
	    start = next;
	}
	if ( start==NULL || start==first )
    break;
	if ( head==NULL ) {
	    head = cur = spl;
	    spl->first = spl->last = NULL;
	} else {
	    cur = calloc(1,sizeof(SplinePointList));
	    last->next = cur;
	}
	last = cur;

	while ( start!=NULL && !start->selected && start!=first ) {
	    if ( cur->first==NULL )
		cur->first = start;
	    cur->last = start;
	    if ( start->next!=NULL ) {
		next = start->next->to;
		if ( next->selected ) {
		    SplineFree(start->next);
		    start->next = NULL;
		    next->prev = NULL;
		}
	    } else
		next = NULL;
	    if ( first==NULL ) first = start;
	    start = next;
	}
    }
return( last );
}

SplinePointList *SplinePointListRemoveSelected(SplinePointList *base) {
    SplinePointList *head=NULL, *last=NULL, *next;
    SplinePoint *pt, *first;
    int anysel, allsel;

    for ( ; base!=NULL; base = next ) {
	next = base->next;
	anysel = false; allsel = true;
	first = NULL;
	for ( pt=base->first; pt!=NULL && pt!=first; pt = pt->next->to ) {
	    if ( pt->selected ) anysel = true;
	    else allsel = false;
	    if ( first==NULL ) first = pt;
	    if ( pt->next==NULL )
	break;
	}
	if ( allsel ) {
	    SplinePointListFree(base);
    continue;
	}
	if ( head==NULL )
	    head = base;
	else
	    last->next = base;
	last = base;
	if ( anysel )
	    last = SplinePointListSplit(base);
    }
    if ( last!=NULL ) last->next = NULL;
return( head );
}

static void TransformPoint(SplinePoint *sp, double transform[6]) {
    BasePoint p;
    p.x = transform[0]*sp->me.x + transform[2]*sp->me.y + transform[4];
    p.y = transform[1]*sp->me.x + transform[3]*sp->me.y + transform[5];
    p.x = rint(1024*p.x)/1024;
    p.y = rint(1024*p.y)/1024;
    sp->me = p;
    if ( !sp->nonextcp ) {
	p.x = transform[0]*sp->nextcp.x + transform[2]*sp->nextcp.y + transform[4];
	p.y = transform[1]*sp->nextcp.x + transform[3]*sp->nextcp.y + transform[5];
	p.x = rint(1024*p.x)/1024;
	p.y = rint(1024*p.y)/1024;
	sp->nextcp = p;
    } else
	sp->nextcp = sp->me;
    if ( !sp->noprevcp ) {
	p.x = transform[0]*sp->prevcp.x + transform[2]*sp->prevcp.y + transform[4];
	p.y = transform[1]*sp->prevcp.x + transform[3]*sp->prevcp.y + transform[5];
	p.x = rint(1024*p.x)/1024;
	p.y = rint(1024*p.y)/1024;
	sp->prevcp = p;
    } else
	sp->prevcp = sp->me;
}

SplinePointList *SplinePointListTransform(SplinePointList *base, double transform[6], int allpoints ) {
    Spline *spline, *first;
    SplinePointList *spl;
    SplinePoint *spt, *pfirst;
    int allsel, anysel;

    for ( spl = base; spl!=NULL; spl = spl->next ) {
	pfirst = NULL;
	allsel = true; anysel=false;
	for ( spt = spl->first ; spt!=pfirst; spt = spt->next->to ) {
	    if ( pfirst==NULL ) pfirst = spt;
	    if ( allpoints || spt->selected ) {
		TransformPoint(spt,transform);
		anysel = true;
	    } else
		allsel = false;
	    if ( spt->next==NULL )
	break;
	}
	if ( !anysel )		/* This splineset had no selected points it's unchanged */
    continue;
	/* if we changed all the points then the control points are right */
	/*  otherwise those near the edges may be wonky, fix 'em up */
	/* Figuring out where the edges of the selection are is difficult */
	/*  so let's just tweak all points, it shouldn't matter */
	if ( !allpoints && !allsel ) {
	    pfirst = NULL;
	    for ( spt = spl->first ; spt!=pfirst; spt = spt->next->to ) {
		if ( pfirst==NULL ) pfirst = spt;
		if ( spt->prev!=NULL )
		    SplineCharDefaultPrevCP(spt,spt->prev->from);
		if ( spt->next==NULL )
	    break;
		SplineCharDefaultNextCP(spt,spt->next->to);
	    }
	}
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    SplineRefigure(spline);
	    if ( first==NULL ) first = spline;
	}
    }
return( base );
}

SplinePointList *SplinePointListShift(SplinePointList *base,double xoff,int allpoints ) {
    double transform[6];
    if ( xoff==0 )
return( base );
    transform[0] = transform[3] = 1;
    transform[1] = transform[2] = transform[5] = 0;
    transform[4] = xoff;
return( SplinePointListTransform(base,transform,allpoints));
}

void SplinePointListSelect(SplinePointList *spl,int sel) {
    Spline *spline, *first, *last;

    for ( ; spl!=NULL; spl = spl->next ) {
	first = NULL; last = NULL;
	spl->first->selected = sel;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    spline->to->selected = sel;
	    if ( first==NULL ) first = spline;
	}
    }
}

void SCMakeDependent(SplineChar *dependent,SplineChar *base) {
    struct splinecharlist *dlist;

    for ( dlist=base->dependents; dlist!=NULL && dlist->sc!=dependent; dlist = dlist->next);
    if ( dlist==NULL ) {
	dlist = calloc(1,sizeof(struct splinecharlist));
	dlist->sc = dependent;
	dlist->next = base->dependents;
	base->dependents = dlist;
    }
}

static void InstanciateReference(SplineFont *sf, RefChar *topref, RefChar *refs,
	double transform[6], SplineChar *dsc) {
    double trans[6];
    RefChar *rf;
    SplineChar *rsc;
    SplinePointList *spl, *new;
    int i;

    if ( !refs->checked ) {
	for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	    if ( strcmp(sf->chars[i]->name,AdobeStandardEncoding[refs->adobe_enc])==0 )
	break;
	if ( i!=sf->charcnt && !sf->chars[i]->ticked ) {
	    refs->checked = true;
	    refs->sc = rsc = sf->chars[i];
	    refs->local_enc = rsc->enc;
	    SCMakeDependent(dsc,rsc);
	} else {
	    fprintf( stderr, "Couldn't find referenced character \"%s\"\n", AdobeStandardEncoding[refs->adobe_enc]);
return;
	}
    } else if ( refs->sc->ticked )
return;

    rsc = refs->sc;
    rsc->ticked = true;

    for ( rf=rsc->refs; rf!=NULL; rf = rf->next ) {
	trans[0] = rf->transform[0]*transform[0] +
		    rf->transform[1]*transform[2];
	trans[1] = rf->transform[0]*transform[1] +
		    rf->transform[1]*transform[3];
	trans[2] = rf->transform[2]*transform[0] +
		    rf->transform[3]*transform[2];
	trans[3] = rf->transform[2]*transform[1] +
		    rf->transform[3]*transform[3];
	trans[4] = rf->transform[4]*transform[0] +
		    rf->transform[5]*transform[2] +
		    transform[4];
	trans[5] = rf->transform[4]*transform[1] +
		    rf->transform[5]*transform[3] +
		    transform[5];
	InstanciateReference(sf,topref,rf,trans,rsc);
    }
    rsc->ticked = false;

    new = SplinePointListTransform(SplinePointListCopy(rsc->splines),transform,true);
    if ( new!=NULL ) {
	for ( spl = new; spl->next!=NULL; spl = spl->next );
	spl->next = topref->splines;
	topref->splines = new;
    }
}

SplineFont *SplineFontFromPSFont(FontDict *fd) {
    SplineFont *sf = calloc(1,sizeof(SplineFont));
    int em;
    int i;
    RefChar *refs, *next;
    char **encoding;

    sf->fontname = copy(fd->fontname);
    sf->display_size = -default_fv_font_size;
    sf->display_antialias = default_fv_antialias;
    if ( fd->fontinfo!=NULL ) {
	if ( sf->fontname==NULL && fd->fontinfo->fullname!=NULL ) { char *from, *to;
	    sf->fontname = copy(fd->fontinfo->fullname);
	    for ( from=to=sf->fontname; *from; ++from )
		if ( *from!=' ' ) *to++ = *from;
	    *to ='\0';
	}
	if ( sf->fontname==NULL ) sf->fontname = copy(fd->fontinfo->familyname);
	sf->fullname = copy(fd->fontinfo->fullname);
	sf->familyname = copy(fd->fontinfo->familyname);
	sf->weight = copy(fd->fontinfo->weight);
	sf->version = copy(fd->fontinfo->version);
	sf->copyright = copy(fd->fontinfo->notice);
	sf->italicangle = fd->fontinfo->italicangle;
	sf->upos = fd->fontinfo->underlineposition;
	sf->uwidth = fd->fontinfo->underlinethickness;
	sf->ascent = fd->fontinfo->ascent;
	sf->descent = fd->fontinfo->descent;
    }
    if ( sf->fontname==NULL ) sf->fontname = GetNextUntitledName();
    if ( sf->fullname==NULL ) sf->fullname = copy(sf->fontname);
    if ( sf->familyname==NULL ) sf->familyname = copy(sf->fontname);
    if ( sf->weight==NULL ) sf->weight = copy("Medium");
    for ( i=19; i>=0 && fd->xuid[i]==0; --i );
    if ( i>=0 ) {
	int j; char *pt;
	sf->xuid = galloc(2+20*(i+1));
	pt = sf->xuid;
	*pt++ = '[';
	for ( j=0; j<=i; ++j ) {
	    sprintf(pt,"%d ", fd->xuid[j]);
	    pt += strlen(pt);
	}
	pt[-1] = ']';
    }
    /*sf->wasbinary = fd->wasbinary;*/
    sf->encoding_name = fd->encoding_name;
    if ( fd->fontmatrix[0]==0 )
	em = 1000;
    else
	em = rint(1/fd->fontmatrix[0]);
    if ( sf->ascent==0 && sf->descent!=0 )
	sf->ascent = em-sf->descent;
    else if ( fd->fontbb[3]-fd->fontbb[1]==em ) {
	if ( sf->ascent==0 ) sf->ascent = fd->fontbb[3];
	if ( sf->descent==0 ) sf->descent = fd->fontbb[1];
    } else if ( sf->ascent==0 )
	sf->ascent = 8*em/10;
    sf->descent = em-sf->ascent;
    sf->charcnt = 256+CharsNotInEncoding(fd);

    encoding = calloc(sf->charcnt,sizeof(char *));
    sf->chars = calloc(sf->charcnt,sizeof(SplineChar *));
    for ( i=0; i<256; ++i )
	encoding[i] = copy(fd->encoding[i]);
    if ( sf->charcnt>256 ) {
	int k, j;
	for ( k=0; k<fd->chars->cnt; ++k ) {
	    if ( fd->chars->keys[k]!=NULL ) {
		for ( j=0; j<256; ++j )
		    if ( fd->encoding[j]!=NULL &&
			    strcmp(fd->encoding[j],fd->chars->keys[k])==0 )
		break;
		if ( j==256 )
		    encoding[i++] = copy(fd->chars->keys[k]);
	    }
	}
	/* And for type3s */
	for ( k=0; k<fd->charprocs->cnt; ++k ) {
	    if ( fd->charprocs->keys[k]!=NULL ) {
		for ( j=0; j<256; ++j )
		    if ( fd->encoding[j]!=NULL &&
			    strcmp(fd->encoding[j],fd->charprocs->keys[k])==0 )
		break;
		if ( j==256 )
		    encoding[i++] = copy(fd->charprocs->keys[k]);
	    }
	}
    }
    for ( i=0; i<sf->charcnt; ++i ) if ( encoding[i]==NULL )
	encoding[i] = copy(".notdef");
    for ( i=0; i<sf->charcnt; ++i ) {
	int k= -1, k2=-1;
	k = LookupCharString(encoding[i],fd->chars);
	if ( k==-1 ) {
	    k = k2 = LookupCharString(encoding[i],(struct pschars *) (fd->charprocs));
	    if ( k==-1 )
		k = LookupCharString(".notdef",fd->chars);
	    if ( k==-1 )
		k = k2 = LookupCharString(".notdef",(struct pschars *) (fd->charprocs));
	}
	if ( k==-1 ) {
	    /* 0 500 hsbw endchar */
	    sf->chars[i] = PSCharStringToSplines((uint8 *) "\213\370\210\015\016",5,false,fd->private->subrs,NULL,".notdef");
	    sf->chars[i]->width = sf->ascent+sf->descent;
	} else if ( k2==-1 ) {
	    sf->chars[i] = PSCharStringToSplines(fd->chars->values[k],fd->chars->lens[k],
		    false,fd->private->subrs,NULL,encoding[i]);
#if 0
	    sf->chars[i]->origtype1 = (uint8 *) copyn((char *)fd->chars->values[k],fd->chars->lens[k]);
	    sf->chars[i]->origlen = fd->chars->lens[k];
#endif
	} else {
	    if ( fd->charprocs->values[k]->unicodeenc==-2 )
		sf->chars[i] = fd->charprocs->values[k];
	    else
		sf->chars[i] = SplineCharCopy(fd->charprocs->values[k]);
	}
	sf->chars[i]->enc = i;
	sf->chars[i]->unicodeenc = UnicodeNameLookup(encoding[i]);
	sf->chars[i]->parent = sf;
	sf->chars[i]->lig = SCLigDefault(sf->chars[i]);		/* Should read from AFM file, but that's way too much work */
	GProgressNext();
    }
    for ( i=0; i<sf->charcnt; ++i ) for ( refs = sf->chars[i]->refs; refs!=NULL; refs=refs->next ) {
	sf->chars[i]->ticked = true;
	InstanciateReference(sf, refs, refs, refs->transform,sf->chars[i]);
	SplineSetFindBounds(refs->splines,&refs->bb);
	sf->chars[i]->ticked = false;
    }
    for ( i=0; i<sf->charcnt; ++i ) for ( refs = sf->chars[i]->refs; refs!=NULL; refs=next ) {
	next = refs->next;
	if ( refs->adobe_enc==' ' && refs->splines==NULL ) {
	    /* When I have a link to a single character I will save out a */
	    /*  seac to that character and a space (since I can only make */
	    /*  double char links), so if we find a space link, get rid of*/
	    /*  it. It's an artifact */
	    SCRefToSplines(sf->chars[i],refs);
	}
    }
    free(encoding);
    sf->private = fd->private->private; fd->private->private = NULL;
    PSDictRemoveEntry(sf->private, "OtherSubrs");
    if ( fd->private->subrs->cnt!=0 ) {
#if 0
	sf->subrs = fd->private->subrs; fd->private->subrs = NULL;
#endif
    }
return( sf );
}

void SCReinstanciateRefChar(SplineChar *sc,RefChar *rf) {
    SplinePointList *spl, *new;
    RefChar *refs;

    SplinePointListsFree(rf->splines);
    rf->splines = NULL;
    if ( rf->sc==NULL )
return;
    new = SplinePointListTransform(SplinePointListCopy(rf->sc->splines),rf->transform,true);
    if ( new!=NULL ) {
	for ( spl = new; spl->next!=NULL; spl = spl->next );
	spl->next = rf->splines;
	rf->splines = new;
    }
    for ( refs = rf->sc->refs; refs!=NULL; refs = refs->next ) {
	new = SplinePointListTransform(SplinePointListCopy(refs->splines),rf->transform,true);
	if ( new!=NULL ) {
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
	    spl->next = rf->splines;
	    rf->splines = new;
	}
    }
    SplineSetFindBounds(rf->splines,&rf->bb);
}

void SCReinstanciateRef(SplineChar *sc,SplineChar *rsc) {
    RefChar *rf;

    for ( rf=sc->refs; rf!=NULL; rf=rf->next ) if ( rf->sc==rsc ) {
	SCReinstanciateRefChar(sc,rf);
    }
}

void SCRemoveDependent(SplineChar *dependent,RefChar *rf) {
    struct splinecharlist *dlist, *pd;
    RefChar *prev;

    if ( dependent->refs==rf )
	dependent->refs = rf->next;
    else {
	for ( prev = dependent->refs; prev->next!=rf; prev=prev->next );
	prev->next = rf->next;
    }
    /* Check for multiple dependencies (colon has two refs to period) */
    /*  if there are none, then remove dependent from ref->sc's dependents list */
    for ( prev = dependent->refs; prev!=NULL && (prev==rf || prev->sc!=rf->sc); prev = prev->next );
    if ( prev==NULL ) {
	dlist = rf->sc->dependents;
	if ( dlist==NULL )
	    /* Do nothing */;
	else if ( dlist->sc==dependent ) {
	    rf->sc->dependents = dlist->next;
	} else {
	    for ( pd=dlist, dlist = pd->next; dlist!=NULL && dlist->sc!=dependent; pd=dlist, dlist = pd->next );
	    if ( dlist!=NULL )
		pd->next = dlist->next;
	}
	free(dlist);
    }
    RefCharFree(rf);
}

void SCRemoveDependents(SplineChar *dependent) {
    RefChar *rf, *next;

    for ( rf=dependent->refs; rf!=NULL; rf=next ) {
	next = rf->next;
	SCRemoveDependent(dependent,rf);
    }
    dependent->refs = NULL;
}

void SCRefToSplines(SplineChar *sc,RefChar *rf) {
    SplineSet *spl;

    if ( (spl = rf->splines)!=NULL ) {
	while ( spl->next!=NULL )
	    spl = spl->next;
	spl->next = sc->splines;
	sc->splines = rf->splines;
	rf->splines = NULL;
    }
    SCRemoveDependent(sc,rf);
}

double SplineSolve(Spline1D *sp, double tmin, double tmax, double sought,double err) {
    /* We want to find t so that spline(t) = sought */
    /*  the curve must be monotonic */
    /* returns t which is near sought or -1 */
    double slope;
    double new_t, newer_t;
    double found_y;
    double t_ymax, t_ymin;
    double ymax, ymin;
    int up;

    if ( sp->a==0 && sp->b==0 ) {
	if ( sp->c==0 )
return( sought==sp->d ? 0 : -1 );
	new_t = (sought-sp->d)/sp->c;
	if ( new_t >= 0 && new_t<=1 )
return( new_t );
return( -1 );
    }

    ymax = ((sp->a*tmax + sp->b)*tmax + sp->c)*tmax + sp->d;
    ymin = ((sp->a*tmin + sp->b)*tmin + sp->c)*tmin + sp->d;
    if ( ymax<ymin ) {
	double temp = ymax;
	ymax = ymin;
	ymin = temp;
	t_ymax = tmin;
	t_ymin = tmax;
	up = false;
    } else {
	t_ymax = tmax;
	t_ymin = tmin;
	up = true;
    }
    if ( sought<ymin || sought>ymax )
return( -1 );

    slope = (3.0*sp->a*t_ymin+2.0*sp->b)*t_ymin + sp->c;
    if ( slope==0 )	/* we often start at a point of inflection */
	new_t = t_ymin + (t_ymax-t_ymin)* (sought-ymin)/(ymax-ymin);
    else {
	new_t = t_ymin + (sought-ymin)/slope;
	if (( new_t>t_ymax && up ) ||
		(new_t<t_ymax && !up) ||
		(new_t<t_ymin && up ) ||
		(new_t>t_ymin && !up ))
	    new_t = t_ymin + (t_ymax-t_ymin)* (sought-ymin)/(ymax-ymin);
    }

    while ( 1 ) {
	found_y = ((sp->a*new_t+sp->b)*new_t+sp->c)*new_t + sp->d ;
	if ( found_y>sought-err && found_y<sought+err ) {
return( new_t );
	}
	newer_t = (t_ymax-t_ymin) * (sought-found_y)/(ymax-ymin);
	if ( found_y > sought ) {
	    ymax = found_y;
	    t_ymax = new_t;
	} else {
	    ymin = found_y;
	    t_ymin = new_t;
	}
	if ( t_ymax==t_ymin )
return( -1 );
	new_t = newer_t;
	if (( new_t>t_ymax && up ) ||
		(new_t<t_ymax && !up) ||
		(new_t<t_ymin && up ) ||
		(new_t>t_ymin && !up ))
	    new_t = (t_ymax+t_ymin)/2;
    }
}

void SplineFindInflections(Spline1D *sp, double *_t1, double *_t2 ) {
    double t1= -1, t2= -1;
    double b2_fourac;

    /* Find the points of inflection on the curve discribing x behavior */
    /*  Set to -1 if there are none or if they are outside the range [0,1] */
    /*  Order them so that t1<t2 */
    /*  If only one valid inflection it will be t1 */
    if ( sp->a!=0 ) {
	/* cubic, possibly 2 inflections (possibly none) */
	b2_fourac = 4*sp->b*sp->b - 12*sp->a*sp->c;
	if ( b2_fourac>=0 ) {
	    b2_fourac = sqrt(b2_fourac);
	    t1 = (-2*sp->b - b2_fourac) / (6*sp->a);
	    t2 = (-2*sp->b + b2_fourac) / (6*sp->a);
	    if ( t1<t2 ) { double temp = t1; t1 = t2; t2 = temp; }
	    else if ( t1==t2 ) t2 = -1;
	    if ( DoubleNear(t1,0)) t1=0; else if ( DoubleNear(t1,1)) t1=1;
	    if ( DoubleNear(t2,0)) t2=0; else if ( DoubleNear(t2,1)) t2=1;
	    if ( t2<=0 || t2>=1 ) t2 = -1;
	    if ( t1<=0 || t1>=1 ) { t1 = t2; t2 = -1; }
	}
    } else if ( sp->b!=0 ) {
	/* Quadratic, at most one inflection */
	t1 = -sp->c/(2.0*sp->b);
	if ( t1<=0 || t1>=1 ) t1 = -1;
    } else /*if ( sp->c!=0 )*/ {
	/* linear, no points of inflection */
    }
    *_t1 = t1; *_t2 = t2;
}

static int XSolve(Spline *spline,double tmin, double tmax,FindSel *fs) {
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];
    double t,x,y;

    fs->p->t = t = SplineSolve(xspline,tmin,tmax,fs->p->cx,.001);
    if ( t>=0 && t<=1 ) {
	y = ((yspline->a*t+yspline->b)*t+yspline->c)*t + yspline->d;
	if ( fs->yl<y && fs->yh>y )
return( true );
    }
    /* Although we know that globaly there's more x change, locally there */
    /*  maybe more y change */
    fs->p->t = t = SplineSolve(yspline,tmin,tmax,fs->p->cy,.001);
    if ( t>=0 && t<=1 ) {
	x = ((xspline->a*t+xspline->b)*t+xspline->c)*t + xspline->d;
	if ( fs->xl<x && fs->xh>x )
return( true );
    }
return( false );
}

static int YSolve(Spline *spline,double tmin, double tmax,FindSel *fs) {
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];
    double t,x,y;

    fs->p->t = t = SplineSolve(yspline,tmin,tmax,fs->p->cy,.001);
    if ( t>=0 && t<=1 ) {
	x = ((xspline->a*t+xspline->b)*t+xspline->c)*t + xspline->d;
	if ( fs->xl<x && fs->xh>x )
return( true );
    }
    /* Although we know that globaly there's more y change, locally there */
    /*  maybe more x change */
    fs->p->t = t = SplineSolve(xspline,tmin,tmax,fs->p->cx,.001);
    if ( t>=0 && t<=1 ) {
	y = ((yspline->a*t+yspline->b)*t+yspline->c)*t + yspline->d;
	if ( fs->yl<y && fs->yh>y )
return( true );
    }
return( false );
}

static int NearXSpline(FindSel *fs, Spline *spline) {
    /* If we get here we've checked the bounding box and we're in it */
    /*  the y spline is a horizontal line */
    /*  the x spline is not linear */
    double t,y;
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];

    if ( xspline->a==0 ) {
	double root = xspline->c*xspline->c - 4*xspline->b*(xspline->d-fs->p->cx);
	if ( root < 0 )
return( false );
	root = sqrt(root);
	fs->p->t = t = (-xspline->c + root)/(2*xspline->b);
	if ( t>=0 && t<=1 ) {
	    y = ((yspline->a*t+yspline->b)*t+yspline->c)*t + yspline->d;
	    if ( fs->yl<y && fs->yh>y )
return( true );
	}
	fs->p->t = t = (-xspline->c - root)/(2*xspline->b);
	if ( t>=0 && t<=1 ) {
	    y = ((yspline->a*t+yspline->b)*t+yspline->c)*t + yspline->d;
	    if ( fs->yl<y && fs->yh>y )
return( true );
	}
    } else {
	double t1, t2, tbase;
	SplineFindInflections(xspline,&t1,&t2);
	tbase = 0;
	if ( t1!=-1 ) {
	    if ( XSolve(spline,0,t1,fs))
return( true );
	    tbase = t1;
	}
	if ( t2!=-1 ) {
	    if ( XSolve(spline,tbase,t2,fs))
return( true );
	    tbase = t2;
	}
	if ( XSolve(spline,tbase,1.0,fs))
return( true );
    }
return( false );
}

int NearSpline(FindSel *fs, Spline *spline) {
    double t,x,y;
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];
    double dx, dy;

    if (( dx = spline->to->me.x-spline->from->me.x)<0 ) dx = -dx;
    if (( dy = spline->to->me.y-spline->from->me.y)<0 ) dy = -dy;

    if ( spline->knownlinear ) {
	if ( fs->xl > spline->from->me.x && fs->xl > spline->to->me.x )
return( false );
	if ( fs->xh < spline->from->me.x && fs->xh < spline->to->me.x )
return( false );
	if ( fs->yl > spline->from->me.y && fs->yl > spline->to->me.y )
return( false );
	if ( fs->yh < spline->from->me.y && fs->yh < spline->to->me.y )
return( false );
	if ( xspline->c==0 && yspline->c==0 )	/* It's a point. */
return( false );	/* if we didn't find it when we checked points we won't now */
	if ( dy>dx ) {
	    t = (fs->p->cy-yspline->d)/yspline->c;
	    fs->p->t = t;
	    x = xspline->c*t + xspline->d;
	    if ( fs->xl<x && fs->xh>x && t>=0 && t<=1 )
return( true );
	} else {
	    t = (fs->p->cx-xspline->d)/xspline->c;
	    fs->p->t = t;
	    y = yspline->c*t + yspline->d;
	    if ( fs->yl<y && fs->yh>y && t>=0 && t<=1 )
return( true );
	}
return( false );
    } else {
	if ( fs->xl > spline->from->me.x && fs->xl > spline->to->me.x &&
		fs->xl > spline->from->nextcp.x && fs->xl > spline->to->prevcp.x )
return( false );
	if ( fs->xh < spline->from->me.x && fs->xh < spline->to->me.x &&
		fs->xh < spline->from->nextcp.x && fs->xh < spline->to->prevcp.x )
return( false );
	if ( fs->yl > spline->from->me.y && fs->yl > spline->to->me.y &&
		fs->yl > spline->from->nextcp.y && fs->yl > spline->to->prevcp.y )
return( false );
	if ( fs->yh < spline->from->me.y && fs->yh < spline->to->me.y &&
		fs->yh < spline->from->nextcp.y && fs->yh < spline->to->prevcp.y )
return( false );

	if ( dx>dy )
return( NearXSpline(fs,spline));
	else if ( yspline->a == 0 && yspline->b == 0 ) {
	    fs->p->t = t = (fs->p->cy-yspline->d)/yspline->c;
	    x = ((xspline->a*t+xspline->b)*t+xspline->c)*t + xspline->d;
	    if ( fs->xl<x && fs->xh>x && t>=0 && t<=1 )
return( true );
	} else if ( yspline->a==0 ) {
	    double root = yspline->c*yspline->c - 4*yspline->b*(yspline->d-fs->p->cy);
	    if ( root < 0 )
return( false );
	    root = sqrt(root);
	    fs->p->t = t = (-yspline->c + root)/(2*yspline->b);
	    x = ((xspline->a*t+xspline->b)*t+xspline->c)*t + xspline->d;
	    if ( fs->xl<x && fs->xh>x && t>0 && t<1 )
return( true );
	    fs->p->t = t = (-yspline->c - root)/(2*yspline->b);
	    x = ((xspline->a*t+xspline->b)*t+xspline->c)*t + xspline->d;
	    if ( fs->xl<x && fs->xh>x && t>=0 && t<=1 )
return( true );
	} else {
	    double t1, t2, tbase;
	    SplineFindInflections(yspline,&t1,&t2);
	    tbase = 0;
	    if ( t1!=-1 ) {
		if ( YSolve(spline,0,t1,fs))
return( true );
		tbase = t1;
	    }
	    if ( t2!=-1 ) {
		if ( YSolve(spline,tbase,t2,fs))
return( true );
		tbase = t2;
	    }
	    if ( YSolve(spline,tbase,1.0,fs))
return( true );
	}
    }
return( false );
}

double SplineNearPoint(Spline *spline, BasePoint *bp, double fudge) {
    PressedOn p;
    FindSel fs;

    memset(&fs,'\0',sizeof(fs));
    memset(&p,'\0',sizeof(p));
    fs.p = &p;
    p.cx = bp->x;
    p.cy = bp->y;
    fs.fudge = fudge;
    fs.xl = p.cx-fudge;
    fs.xh = p.cx+fudge;
    fs.yl = p.cy-fudge;
    fs.yh = p.cy+fudge;
    if ( !NearSpline(&fs,spline))
return( -1 );

return( p.t );
}

void StemInfoFree(StemInfo *h) {
    HintInstance *hi, *n;

    for ( hi=h->where; hi!=NULL; hi=n ) {
	n = hi->next;
	free(hi);
    }
    free(h);
}

void StemInfosFree(StemInfo *h) {
    StemInfo *hnext;
    HintInstance *hi, *n;

    for ( ; h!=NULL; h = hnext ) {
	for ( hi=h->where; hi!=NULL; hi=n ) {
	    n = hi->next;
	    free(hi);
	}
	hnext = h->next;
	free(h);
    }
}

void DStemInfoFree(DStemInfo *h) {

    free(h);
}

void DStemInfosFree(DStemInfo *h) {
    DStemInfo *hnext;

    for ( ; h!=NULL; h = hnext ) {
	hnext = h->next;
	free(h);
    }
}

StemInfo *StemInfoCopy(StemInfo *h) {
    StemInfo *head=NULL, *last=NULL, *cur;
    HintInstance *hilast, *hicur, *hi;

    for ( ; h!=NULL; h = h->next ) {
	cur = galloc(sizeof(StemInfo));
	*cur = *h;
	cur->next = NULL;
	if ( head==NULL )
	    head = last = cur;
	else {
	    last->next = cur;
	    last = cur;
	}
	cur->where = hilast = NULL;
	for ( hi=h->where; hi!=NULL; hi=hi->next ) {
	    hicur = galloc(sizeof(StemInfo));
	    *hicur = *hi;
	    hicur->next = NULL;
	    if ( hilast==NULL )
		cur->where = hilast = hicur;
	    else {
		hilast->next = hicur;
		hilast = hicur;
	    }
	}
    }
return( head );
}

void KernPairsFree(KernPair *kp) {
    KernPair *knext;
    for ( ; kp!=NULL; kp = knext ) {
	knext = kp->next;
	free(kp);
    }
}

void LigatureFree(Ligature *lig) {
    if ( lig==NULL )
return;
    free(lig->componants);
    free(lig);
}

void SplineCharFree(SplineChar *sc) {
    struct splinecharlist *dlist, *dnext;

    if ( sc==NULL )
return;
    free(sc->name);
    SplinePointListsFree(sc->splines);
    SplinePointListsFree(sc->backgroundsplines);
    RefCharsFree(sc->refs);
    ImageListsFree(sc->backimages);
    /* image garbage collection????!!!! */
    StemInfosFree(sc->hstem);
    StemInfosFree(sc->vstem);
    DStemInfosFree(sc->dstem);
    KernPairsFree(sc->kerns);
    UndoesFree(sc->undoes[0]); UndoesFree(sc->undoes[1]);
    UndoesFree(sc->redoes[0]); UndoesFree(sc->redoes[1]);
    for ( dlist=sc->dependents; dlist!=NULL; dlist = dnext ) {
	dnext = dlist->next;
	free(dlist);
    }
#if 0
    free(sc->origtype1);
#endif
    LigatureFree(sc->lig);
    free(sc);
}

void SplineFontFree(SplineFont *sf) {
    int i;

    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL )
	SplineCharFree(sf->chars[i]);
    free(sf->chars);
    free(sf->fontname);
    free(sf->fullname);
    free(sf->familyname);
    free(sf->weight);
    free(sf->copyright);
    free(sf->filename);
    free(sf->origname);
    free(sf->autosavename);
    free(sf->version);
    free(sf->hsnaps);
    free(sf->vsnaps);
    SplinePointListFree(sf->gridsplines);
    UndoesFree(sf->gundoes);
    UndoesFree(sf->gredoes);
    PSDictFree(sf->private);
#if 0
    PSCharsFree(sf->subrs);
#endif
    free(sf);
}
