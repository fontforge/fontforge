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

typedef struct quartic {
    double a,b,c,d,e;
} Quartic;

/* In an attempt to make allocation more efficient I just keep preallocated */
/*  lists of certain common sizes. It doesn't seem to make much difference */
/*  when allocating stuff, but does when freeing. If the extra complexity */
/*  is bad then put:
/*	#define chunkalloc(size)	gcalloc(1,size)
/*	#define chunkfree(item,size)	free(item)
/*  into splinefont.h after the (or instead of) the definition of chunkalloc()*/

#ifndef chunkalloc
#define ALLOC_CHUNK	100
#define CHUNK_MAX	100

#ifdef FLAG
#undef FLAG
#define FLAG 0xbadcafe
#endif

#if ALLOC_CHUNK>1
struct chunk { struct chunk *next; };
struct chunk2 { struct chunk2 *next; int flag; };
static struct chunk *chunklists[CHUNK_MAX/4] = { 0 };
#endif

#if defined(FLAG) && ALLOC_CHUNK>1
void chunktest(void) {
    int i;
    struct chunk2 *c;

    for ( i=2; i<CHUNK_MAX/4; ++i )
	for ( c=(struct chunk2 *) chunklists[i]; c!=NULL; c=c->next )
	    if ( c->flag!=FLAG ) {
		fprintf( stderr, "Chunk memory list has been corrupted\n" );
		abort();
	    }
}
#endif

void *chunkalloc(int size) {
# if ALLOC_CHUNK<=1
return( gcalloc(1,size));
# else
    struct chunk *item;
    int index;

    if ( (size&0x3) || size>=CHUNK_MAX || size<=sizeof(struct chunk)) {
	fprintf( stderr, "Attempt to allocate something of size %d\n", size );
return( gcalloc(1,size));
    }
#ifdef FLAG
    chunktest();
#endif
    index = size>>2;
    if ( chunklists[index]==NULL ) {
	char *pt, *end;
	pt = galloc(ALLOC_CHUNK*size);
	chunklists[index] = (struct chunk *) pt;
	end = pt+(ALLOC_CHUNK-1)*size;
	while ( pt<end ) {
	    ((struct chunk *) pt)->next = (struct chunk *) (pt + size);
#ifdef FLAG
	    ((struct chunk2 *) pt)->flag = FLAG;
#endif
	    pt += size;
	}
	((struct chunk *) pt)->next = NULL;
#ifdef FLAG
	((struct chunk2 *) pt)->flag = FLAG;
#endif
    }
    item = chunklists[index];
    chunklists[index] = item->next;
    memset(item,'\0',size);
return( item );
# endif
}

void chunkfree(void *item,int size) {
# if ALLOC_CHUNK<=1
    free(item);
# else
    if ( item==NULL )
return;
    if ( (size&0x3) || size>=CHUNK_MAX || size<=sizeof(struct chunk)) {
	fprintf( stderr, "Attempt to free something of size %d\n", size );
	free(item);
    } else {
	((struct chunk *) item)->next = chunklists[size>>2];
#  ifdef FLAG
	if ( size>=sizeof(struct chunk2))
	    ((struct chunk2 *) item)->flag = FLAG;
#  endif
	chunklists[size>>2] = (struct chunk *) item;
    }
#  ifdef FLAG
    chunktest();
#  endif
# endif
}
#endif

void LineListFree(LineList *ll) {
    LineList *next;

    while ( ll!=NULL ) {
	next = ll->next;
	chunkfree(ll,sizeof(LineList));
	ll = next;
    }
}

void LinearApproxFree(LinearApprox *la) {
    LinearApprox *next;

    while ( la!=NULL ) {
	next = la->next;
	LineListFree(la->lines);
	chunkfree(la,sizeof(LinearApprox));
	la = next;
    }
}

void SplineFree(Spline *spline) {
    LinearApproxFree(spline->approx);
    chunkfree(spline,sizeof(Spline));
}

void SplinePointFree(SplinePoint *sp) {
    chunkfree(sp,sizeof(SplinePoint));
}

void SplinePointMDFree(SplineChar *sc, SplinePoint *sp) {
    MinimumDistance *md, *prev, *next;

    if ( sc!=NULL ) {
	prev = NULL;
	for ( md = sc->md; md!=NULL; md = next ) {
	    next = md->next;
	    if ( md->sp1==sp || md->sp2==sp ) {
		if ( prev==NULL )
		    sc->md = next;
		else
		    prev->next = next;
		chunkfree(md,sizeof(MinimumDistance));
	    } else
		prev = md;
	}
    }

    chunkfree(sp,sizeof(SplinePoint));
}

void SplinePointListFree(SplinePointList *spl) {
    Spline *first, *spline, *next;

    if ( spl==NULL )
return;
    if ( spl->first!=NULL ) {
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = next ) {
	    next = spline->to->next;
	    SplinePointFree(spline->to);
	    SplineFree(spline);
	    if ( first==NULL ) first = spline;
	}
	if ( spl->last!=spl->first || spl->first->next==NULL )
	    SplinePointFree(spl->first);
    }
    chunkfree(spl,sizeof(SplinePointList));
}

void SplinePointListMDFree(SplineChar *sc,SplinePointList *spl) {
    Spline *first, *spline, *next;

    if ( spl==NULL )
return;
    if ( spl->first!=NULL ) {
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = next ) {
	    next = spline->to->next;
	    SplinePointMDFree(sc,spline->to);
	    SplineFree(spline);
	    if ( first==NULL ) first = spline;
	}
	if ( spl->last!=spl->first || spl->first->next==NULL )
	    SplinePointMDFree(sc,spl->first);
    }
    chunkfree(spl,sizeof(SplinePointList));
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
    chunkfree(ref,sizeof(RefChar));
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
    if ( RealNear(from->me.x,to->me.x) && RealNear(from->me.y,to->me.y))
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
	if ( RealNear(xsp->c,0)) xsp->c=0;
	if ( RealNear(ysp->c,0)) ysp->c=0;
	if ( RealNear(xsp->b,0)) xsp->b=0;
	if ( RealNear(ysp->b,0)) ysp->b=0;
	if ( RealNear(xsp->a,0)) xsp->a=0;
	if ( RealNear(ysp->a,0)) ysp->a=0;
	spline->islinear = false;
	if ( ysp->a==0 && xsp->a==0 && ysp->b==0 && xsp->b==0 )
	    spline->islinear = true;	/* This seems extremely unlikely... */
    }
    if ( isnan(ysp->a) || isnan(xsp->a) )
	GDrawIError("NaN value in spline creation");
    LinearApproxFree(spline->approx);
    spline->approx = NULL;
    spline->knowncurved = false;
    spline->knownlinear = spline->islinear;
    SplineIsLinear(spline);
    if ( !spline->islinear && spline->knownlinear ) {
	spline->knownlinear = false;
    }
    spline->isquadratic = false;
    if ( !spline->islinear && xsp->a==0 && ysp->a==0 )
	spline->isquadratic = true;	/* Only likely if we read in a TTF */
}

Spline *SplineMake(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    SplineRefigure(spline);
return( spline );
}

static real SolveCubic(real a, real b, real c, real d, real err, real t0) {
    /* find t between t0 and .5 where at^3+bt^2+ct+d == +/- err */
    real t, val, offset;
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

static real SolveCubicBack(real a, real b, real c, real d, real err, real t0) {
    /* find t between .5 and t0 where at^3+bt^2+ct+d == +/- err */
    real t, val, offset;
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
	    chunkfree(next,sizeof(*next));
	} else {
	    prev = lines;
	    lines = next;
	}
    }
    if ( prev!=NULL &&
	    prev->here.x==lines->here.x && prev->here.y == lines->here.y ) {
	prev->next = NULL;
	chunkfree(lines,sizeof(*lines));
    }

    while ( (next = lines->next)!=NULL ) {
	if ( prev->here.x!=next->here.x ) {
	    real slope = (next->here.y-prev->here.y) / (real) (next->here.x-prev->here.x);
	    real inter = prev->here.y - slope*prev->here.x;
	    int y = rint(lines->here.x*slope + inter);
	    if ( y == lines->here.y ) {
		lines->here = next->here;
		lines->next = next->next;
		chunkfree(next,sizeof(*next));
	    } else
		lines = next;
	} else
	    lines = next;
    }
}

LinearApprox *SplineApproximate(Spline *spline, real scale) {
    LinearApprox *test;
    LineList *cur, *last=NULL, *prev;
    real tx,ty,t;
    real slpx, slpy;
    real intx, inty;

    for ( test = spline->approx; test!=NULL && test->scale!=scale; test = test->next );
    if ( test!=NULL )
return( test );

    test = chunkalloc(sizeof(LinearApprox));
    test->scale = scale;
    test->next = spline->approx;
    spline->approx = test;

    cur = chunkalloc(sizeof(LineList) );
    cur->here.x = rint(spline->from->me.x*scale);
    cur->here.y = rint(spline->from->me.y*scale);
    test->lines = last = cur;
    if ( spline->knownlinear ) {
	cur = chunkalloc(sizeof(LineList) );
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
	cur = chunkalloc(sizeof(LineList) );
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
	    cur = chunkalloc(sizeof(LineList));
	    cur->here.x = rint( (((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d)*scale );
	    cur->here.y = rint( (((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d)*scale );
	    last->next = cur;
	    last = cur;
	}

	/* Now start at t=1 and work back to t=.5 */
	prev = NULL;
	cur = chunkalloc(sizeof(LineList) );
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
	    cur = chunkalloc(sizeof(LineList) );
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
    real t, b2_fourac, v;
    real min, max;
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

void CIDFindBounds(SplineFont *cidmaster,DBounds *bounds) {
    SplineFont *sf;
    int i;
    DBounds b;
    real factor;

    sf = cidmaster->subfonts[0];
    SplineFontFindBounds(sf,bounds);
    factor = 1000/(sf->ascent+sf->descent);
    bounds->maxx *= factor; bounds->minx *= factor; bounds->miny *= factor; bounds->maxy *= factor;
    for ( i=1; i<cidmaster->subfontcnt; ++i ) {
	sf = cidmaster->subfonts[i];
	SplineFontFindBounds(sf,&b);
	factor = 1000/(sf->ascent+sf->descent);
	b.maxx *= factor; b.minx *= factor; b.miny *= factor; b.maxy *= factor;
	if ( b.maxx>bounds->maxx ) bounds->maxx = b.maxx;
	if ( b.maxy>bounds->maxy ) bounds->maxy = b.maxy;
	if ( b.miny<bounds->miny ) bounds->miny = b.miny;
	if ( b.minx<bounds->minx ) bounds->minx = b.minx;
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
	    real slope = (sp->nextcp.y-sp->prevcp.y)/(sp->nextcp.x-sp->prevcp.x);
	    real y = slope*(sp->me.x-sp->prevcp.x) + sp->prevcp.y - sp->me.y;
	    if ( y<1 && y>-1 )
		sp->pointtype = pt_curve;
	    else if ( sp->nextcp.y!=sp->prevcp.y ) {
		real x;
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
	    real slope = (sp->next->to->me.y-sp->prevcp.y)/(sp->next->to->me.x-sp->prevcp.x);
	    real y = slope*(sp->me.x-sp->prevcp.x) + sp->prevcp.y - sp->me.y;
	    if ( y<1 && y>-1 )
		sp->pointtype = pt_tangent;
	} else if ( sp->me.x == sp->prevcp.x )
	    sp->pointtype = pt_tangent;
    } else {
	if ( sp->nextcp.x!=sp->prev->from->me.x ) {
	    real slope = (sp->nextcp.y-sp->prev->from->me.y)/(sp->nextcp.x-sp->prev->from->me.x);
	    real y = slope*(sp->me.x-sp->prev->from->me.x) + sp->prev->from->me.y - sp->me.y;
	    if ( y<1 && y>-1 )
		sp->pointtype = pt_tangent;
	} else if ( sp->me.x == sp->nextcp.x )
	    sp->pointtype = pt_tangent;
    }
}

int SplinePointIsACorner(SplinePoint *sp) {
    enum pointtype old = sp->pointtype, new;

    SplinePointCatagorize(sp);
    new = sp->pointtype;
    sp->pointtype = old;
return( new==pt_corner );
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
    if ( fd->charprocs!=NULL ) for ( i=0; i<fd->charprocs->cnt; ++i ) {
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

    cur = chunkalloc(sizeof(SplinePointList));

    first = NULL;
    for ( pt=spl->first; pt!=NULL && pt!=first; pt = pt->next->to ) {
	cpt = chunkalloc(sizeof(SplinePoint));
	*cpt = *pt;
	cpt->next = cpt->prev = NULL;
	if ( cur->first==NULL )
	    cur->first = cur->last = cpt;
	else {
	    spline = chunkalloc(sizeof(Spline));
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
	spline = chunkalloc(sizeof(Spline));
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
	cur = chunkalloc(sizeof(SplinePointList));
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;

	while ( start!=NULL && start->selected && start!=first ) {
	    cpt = chunkalloc(sizeof(SplinePoint));
	    *cpt = *start;
	    cpt->next = cpt->prev = NULL;
	    if ( cur->first==NULL )
		cur->first = cur->last = cpt;
	    else {
		spline = chunkalloc(sizeof(Spline));
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

static SplinePointList *SplinePointListSplit(SplineChar *sc,SplinePointList *spl) {
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
	    SplinePointMDFree(sc,start);
	    start = next;
	}
	if ( start==NULL || start==first )
    break;
	if ( head==NULL ) {
	    head = cur = spl;
	    spl->first = spl->last = NULL;
	} else {
	    cur = chunkalloc(sizeof(SplinePointList));
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

SplinePointList *SplinePointListRemoveSelected(SplineChar *sc,SplinePointList *base) {
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
	    SplinePointListMDFree(sc,base);
    continue;
	}
	if ( head==NULL )
	    head = base;
	else
	    last->next = base;
	last = base;
	if ( anysel )
	    last = SplinePointListSplit(sc,base);
    }
    if ( last!=NULL ) last->next = NULL;
return( head );
}

static void TransformPoint(SplinePoint *sp, real transform[6]) {
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

SplinePointList *SplinePointListTransform(SplinePointList *base, real transform[6], int allpoints ) {
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
	/* It does matter. Let's tweak all default points */
	if ( !allpoints && !allsel ) {
	    pfirst = NULL;
	    for ( spt = spl->first ; spt!=pfirst; spt = spt->next->to ) {
		if ( pfirst==NULL ) pfirst = spt;
		if ( spt->prev!=NULL && spt->prevcpdef )
		    SplineCharDefaultPrevCP(spt,spt->prev->from);
		if ( spt->next==NULL )
	    break;
		if ( spt->nextcpdef )
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

SplinePointList *SplinePointListShift(SplinePointList *base,real xoff,int allpoints ) {
    real transform[6];
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
	real transform[6], SplineChar *dsc) {
    real trans[6];
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

static char *copyparse(char *str) {
    char *ret, *rpt;
    int ch,i;

    if ( str==NULL )
return( str );

    rpt=ret=galloc(strlen(str)+1);
    while ( *str ) {
	if ( *str=='\\' ) {
	    ++str;
	    if ( *str=='n' ) ch = '\n';
	    else if ( *str=='r' ) ch = '\r';
	    else if ( *str=='t' ) ch = '\t';
	    else if ( *str=='b' ) ch = '\b';
	    else if ( *str=='f' ) ch = '\f';
	    else if ( *str=='\\' ) ch = '\\';
	    else if ( *str=='(' ) ch = '(';
	    else if ( *str==')' ) ch = ')';
	    else if ( *str>='0' && *str<='7' ) {
		for ( i=ch = 0; i<3 && *str>='0' && *str<='7'; ++i )
		    ch = (ch<<3) + *str++-'0';
		--str;
	    } else
		ch = *str;
	    ++str;
	    *rpt++ = ch;
	} else
	    *rpt++ = *str++;
    }
    *rpt = '\0';
return(ret);
}

static void SplineFontMetaData(SplineFont *sf,struct fontdict *fd) {
    int em;
    int i;

    sf->fontname = copy(fd->cidfontname?fd->cidfontname:fd->fontname);
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
	sf->fullname = copyparse(fd->fontinfo->fullname);
	sf->familyname = copyparse(fd->fontinfo->familyname);
	sf->weight = copyparse(fd->fontinfo->weight);
	sf->version = copyparse(fd->fontinfo->version);
	sf->copyright = copyparse(fd->fontinfo->notice);
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
    sf->cidversion = fd->cidversion;
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

    sf->private = fd->private->private; fd->private->private = NULL;
    PSDictRemoveEntry(sf->private, "OtherSubrs");

    sf->cidregistry = copy(fd->registry);
    sf->ordering = copy(fd->ordering);
    sf->supplement = fd->supplement;
    sf->pfminfo.fstype = fd->fontinfo->fstype;
}

/* Adobe has (it seems to me) misnamed the greek letters so that "mu" actually*/
/*  refers to the micro sign. Similar problems for Delta and Omega. When I get*/
/*  a mu character I generate things named mu, uni00B5 and uni03BC. So here I */
/*  check for instances where both mu and uni00B5 (which both map to 0xb5) I */
/*  remove one of them */
static void CleanupGreekNames(FontDict *fd) {
    static char *namepairs[3][2] = {{ "mu", "uni00B5" /* 3bc */ },
				    { "Delta", "uni0394" /* 2206 */ },
				    { "Omega", "uni03A9" /* 2126 */ }};
    int i,j,k;
    struct pschars *chars = fd->chars;
    char *namei, *namej;

    for ( i=0; i<chars->cnt; ++i ) if ( (namei=chars->keys[i])!=NULL ) {
	for ( k=0; k<3; ++k )
	    if ( strcmp(namei,namepairs[k][0])==0 ) {
		for ( j=0; j<chars->cnt; ++j ) if ( (namej=chars->keys[j])!=NULL ) {
		    if ( strcmp(namej,namepairs[k][1])==0 ) {
			if ( i>j ) --i;
			free(chars->keys[j]);
			free(chars->values[j]);
			for ( k=j; k<chars->cnt-1; ++k ) {
			    chars->keys[k] = chars->keys[k+1];
			    chars->values[k] = chars->values[k+1];
			    chars->lens[k] = chars->lens[k+1];
			}
			chars->keys[k] = NULL; chars->values[k] = NULL; chars->lens[k] = 0;
		break;
		    }
		}
	    }
    }
}

static void SplineFontFromType1(SplineFont *sf, FontDict *fd) {
    int i;
    RefChar *refs, *next;
    char **encoding;
    int istype2 = fd->fonttype==2;		/* Easy enough to deal with even though it will never happen... */

    CleanupGreekNames(fd);
    if ( istype2 )
	fd->private->subrs->bias = fd->private->subrs->cnt<1240 ? 107 :
	    fd->private->subrs->cnt<33900 ? 1131 : 32768;
    sf->charcnt = 256+CharsNotInEncoding(fd);
    encoding = gcalloc(sf->charcnt,sizeof(char *));
    sf->chars = gcalloc(sf->charcnt,sizeof(SplineChar *));
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
		    istype2,fd->private->subrs,NULL,encoding[i]);
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
	    /*  real char links), so if we find a space link, get rid of*/
	    /*  it. It's an artifact */
	    SCRefToSplines(sf->chars[i],refs);
	}
    }
    free(encoding);
}

static SplineFont *SplineFontFromCIDType1(SplineFont *sf, FontDict *fd) {
    int i,j, bad, uni;
    SplineChar **chars;
    char buffer[100];
    struct cidmap *map;

    bad = 0x80000000;
    for ( i=0; i<fd->fdcnt; ++i )
	if ( fd->fds[i]->fonttype!=1 && fd->fds[i]->fonttype!=2 )
	    bad = fd->fds[i]->fonttype;
    if ( bad!=0x80000000 || fd->cidfonttype!=0 ) {
	fprintf(stderr,"Could not parse a CID font, " );
	if ( fd->cidfonttype!=0 )
	    fprintf( stderr, "unexpected CIDFontType %d ", fd->cidfonttype );
	if ( bad!=0x80000000 )
	    fprintf( stderr, "unexpected fonttype %d", bad );
	fprintf( stderr,"\n");
	SplineFontFree(sf);
return( NULL );
    }
    if ( fd->cidstrs==NULL || fd->cidcnt==0 ) {
	fprintf( stderr, "CID format doesn't contain what we expected it to.\n" );
	SplineFontFree(sf);
return( NULL );
    }

    sf->subfontcnt = fd->fdcnt;
    sf->subfonts = galloc((sf->subfontcnt+1)*sizeof(SplineFont *));
    for ( i=0; i<fd->fdcnt; ++i ) {
	sf->subfonts[i] = SplineFontEmpty();
	SplineFontMetaData(sf->subfonts[i],fd->fds[i]);
	sf->subfonts[i]->cidmaster = sf;
	if ( fd->fds[i]->fonttype==2 )
	    fd->fds[i]->private->subrs->bias =
		    fd->fds[i]->private->subrs->cnt<1240 ? 107 :
		    fd->fds[i]->private->subrs->cnt<33900 ? 1131 : 32768;
    }

    map = FindCidMap(sf->cidregistry,sf->ordering,sf->supplement);

    chars = gcalloc(fd->cidcnt,sizeof(SplineChar *));
    for ( i=0; i<fd->cidcnt; ++i ) if ( fd->cidlens[i]>0 ) {
	j = fd->cidfds[i];		/* We get font indexes of 255 for non-existant chars */
	uni = CID2NameEnc(map,i,buffer,sizeof(buffer));
	chars[i] = PSCharStringToSplines(fd->cidstrs[i],fd->cidlens[i],
		    fd->fds[j]->fonttype==2,fd->fds[j]->private->subrs,
		    NULL,buffer);
	chars[i]->unicodeenc = uni;
	chars[i]->enc = i;
	/* There better not be any references (seac's) because we have no */
	/*  encoding on which to base any fixups */
	if ( chars[i]->refs!=NULL )
	    GDrawIError( "Reference found in CID font. Can't fix it up");
	chars[i]->enc = i;
	sf->subfonts[j]->charcnt = i+1;
	GProgressNext();
    }
    for ( i=0; i<fd->fdcnt; ++i )
	sf->subfonts[i]->chars = gcalloc(sf->subfonts[i]->charcnt,sizeof(SplineChar *));
    for ( i=0; i<fd->cidcnt; ++i ) if ( chars[i]!=NULL ) {
	j = fd->cidfds[i];
	if ( j<sf->subfontcnt ) {
	    sf->subfonts[j]->chars[i] = chars[i];
	    chars[i]->parent = sf->subfonts[j];
	}
    }
    free(chars);
    /* Now we'd like to pull an adobe charset file (based on registry_ordering_supplement) */
    /*  out of the air, and use it to give ourselves... well at least real */
    /*  character names, possibly an encoding !!!! */
return( sf );
}

SplineFont *SplineFontFromPSFont(FontDict *fd) {
    SplineFont *sf = SplineFontEmpty();

    SplineFontMetaData(sf,fd);
    if ( fd->wascff ) {
	SplineFontFree(sf);
	sf = fd->sf;
    } else if ( fd->fdcnt==0 )
	SplineFontFromType1(sf,fd);
    else
	sf = SplineFontFromCIDType1(sf,fd);
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

/* This returns all real solutions, even those out of bounds */
/* I use -999999 as an error flag, since we're really only interested in */
/*  solns near 0 and 1 that should be ok. -1 is perhaps a little too close */
static int _CubicSolve(Spline1D *sp,real ts[3]) {
    double d, xN, yN, delta2, temp, delta, h, t2, t3, theta;
    int i=0;

    ts[0] = ts[1] = ts[2] = -999999;
    if ( sp->a!=0 ) {
    /* http://www.m-a.org.uk/eb/mg/mg077ch.pdf */
    /* this nifty solution to the cubic neatly avoids complex arithmatic */
	xN = -sp->b/(3*sp->a);
	yN = ((sp->a*xN + sp->b)*xN+sp->c)*xN + sp->d;

	delta2 = (sp->b*sp->b-3*sp->a*sp->c)/(9*sp->a*sp->a);
	if ( RealNear(delta2,0) ) delta2 = 0;

	/* the descriminant is yN^2-h^2, but delta might be <0 so avoid using h */
	d = yN*yN - 4*sp->a*sp->a*delta2*delta2*delta2;
	if ( ((yN>.01 || yN<-.01) && RealNear(d/yN,0)) || ((yN<=.01 && yN>=-.01) && RealNear(d,0)) )
	    d = 0;
	if ( d>0 ) {
	    temp = sqrt(d);
	    t2 = (-yN-temp)/(2*sp->a);
	    t2 = (t2==0) ? 0 : (t2<0) ? -pow(-t2,1./3.) : pow(t2,1./3.);
	    t3 = (-yN+temp)/(2*sp->a);
	    t3 = t3==0 ? 0 : (t3<0) ? -pow(-t3,1./3.) : pow(t3,1./3.);
	    ts[0] = xN + t2 + t3;
	} else if ( d<0 ) {
	    if ( delta2>=0 ) {
		delta = sqrt(delta2);
		h = 2*sp->a*delta2*delta;
		temp = -yN/h;
		if ( temp>=-1.0001 && temp<=1.0001 ) {
		    if ( temp<-1 ) temp = -1; else if ( temp>1 ) temp = 1;
		    theta = acos(temp)/3;
		    ts[i++] = xN+2*delta*cos(theta);
		    ts[i++] = xN+2*delta*cos(2.0943951+theta);
		    ts[i++] = xN+2*delta*cos(4.1887902+theta);
		}
	    }
	} else if ( /* d==0 && */ delta2!=0 ) {
	    delta = yN/(2*sp->a);
	    delta = delta==0 ? 0 : delta>0 ? pow(delta,1./3.) : -pow(-delta,1./3.);
	    ts[i++] = xN + delta;	/* this root twice, but that's irrelevant to me */
	    ts[i++] = xN - 2*delta;
	} else if ( /* d==0 && */ delta2==0 ) {
	    if ( xN>=-0.0001 && xN<=1.0001 ) ts[0] = xN;
	}
    } else if ( sp->b!=0 ) {
	double d = sp->c*sp->c-4*sp->b*sp->d;
	if ( RealNear(d,0)) d=0;
	if ( d<0 )
return(false);		/* All roots imaginary */
	d = sqrt(d);
	ts[0] = (-sp->c-d)/(2*sp->b);
	ts[1] = (-sp->c+d)/(2*sp->b);
    } else if ( sp->c!=0 ) {
	ts[0] = -sp->d/sp->c;
    } else {
	/* If it's a point then either everything is a solution, or nothing */
    }
return( ts[0]!=-1 );
}

int CubicSolve(Spline1D *sp,real ts[3]) {
    double t;
    /* This routine gives us all solutions between [0,1] with -1 as an error flag */

    if ( !_CubicSolve(sp,ts))
return( false );
    if (ts[0]>1.0001 || ts[0]<-.0001 ) ts[0] = -1;
    else if ( ts[0]<0 ) ts[0] = 0; else if ( ts[0]>1 ) ts[0] = 1;
    if (ts[1]>1.0001 || ts[1]<-.0001 ) ts[1] = -1;
    else if ( ts[1]<0 ) ts[1] = 0; else if ( ts[1]>1 ) ts[1] = 1;
    if (ts[2]>1.0001 || ts[2]<-.0001 ) ts[2] = -1;
    else if ( ts[2]<0 ) ts[2] = 0; else if ( ts[2]>1 ) ts[2] = 1;
    if ( ts[1]==-1 ) { ts[1] = ts[2]; ts[2] = -1;}
    if ( ts[0]==-1 ) { ts[0] = ts[1]; ts[1] = ts[2]; ts[2] = -1; }
    if ( ts[0]==-1 )
return( false );
    if ( ts[0]>ts[2] && ts[2]!=-1 ) {
	t = ts[0]; ts[0] = ts[2]; ts[2] = t;
    }
    if ( ts[0]>ts[1] && ts[1]!=-1 ) {
	t = ts[0]; ts[0] = ts[1]; ts[1] = t;
    }
    if ( ts[1]>ts[2] && ts[2]!=-1 ) {
	t = ts[1]; ts[1] = ts[2]; ts[2] = t;
    }
return( true );
}

static int QuarticSolve(Quartic *q,real ts[4]) {
    Quartic work;
    Spline1D sp;
    real zs[3];
    double sq, a, b, c, d, e, f;
    int i;

    work.a = 1;
    work.b = 0;
    work.c = (q->c-3*q->b*q->b/(8*q->a))/q->a;
    work.d = (q->b*q->b*q->b/(8*q->a*q->a) - q->b*q->c/(2*q->a) + q->d)/q->a;
    work.e = (-3*q->b*q->b*q->b*q->b/(256*q->a*q->a*q->a) + (q->b*q->b*q->c)/(16*q->a*q->a) -
	    q->b*q->d/(4*q->a) + q->e)/q->a;

    if ( RealNear(work.e,0))
	work.e = 0;
    if ( work.e<0 )		/* I hope this means no real roots, my cubic solver doesn't deal with complex coef */
return( -1 );
    sq = sqrt(work.e);
    sp.a = 8;
    sp.b = 24*sq-4*work.c;
    sp.c = 16*work.e-8*sq*work.c;
    sp.d = -work.d*work.d;
    if ( !_CubicSolve(&sp,zs) )
return( -1 );
    a=2*sq-work.c+2*zs[0];
    b= -work.d;
    c= zs[0]*(zs[0]+2*sq);
    if ( RealNear(c,0))
	c = 0;
    if ( c<0 )
return( -1 );
    if ( !RealNear(b*b-4*a*c,0) )
	GDrawIError("Failure in quartic");

    b = -sqrt(a);
    c = sq+zs[0]-sqrt(c);
    a = 1;
    i = 0;
    f = q->b/(4*q->a);
    if ( b*b-4*c>= 0 ) {
	sq = sqrt(b*b-4*c);
	ts[i++] = (-b+sq)/2-f;
	if ( sq!=0 )
	    ts[i++] = (-b-sq)/2-f; 
    }
    /* Now the a,b,c polynomial above must be offset by f before it can */
    /*  represent the roots of q */
    c += f*f + b*f;
    b += 2*f;

	/* Now divide the original spline by this quadratic, and we will get */
	/*  another quadratic with the other two roots */
    d = q->a;
    work.b = q->b-d*b;
    work.c = q->c-d*c;
    e = work.b;
    work.c -= e*b;
    work.d = q->d-e*c;
    f = work.c;
    if ( !RealNear(work.d-f*b,0) || !RealNear(q->e-f*c,0))
	GDrawIError("Polynomial division failed");
    if ( e*e-4*d*f >= 0 ) {
	sq = sqrt(e*e-4*d*f);
	ts[i++] = (-e+sq)/(2*d);
	if ( sq!=0 )
	    ts[i++] = (-e-sq)/(2*d);
    }
    while ( i<4 ) ts[i++] = -1;
    for ( i=0; i<4 && ts[i]!=-1; ++i ) {
	if ( ts[i]<-.001 || ts[i]>1.001 ) ts[i]=-1;
	else if ( ts[i]<0 ) ts[i] = 0; else if ( ts[i]>1 ) ts[i] = 1;
    }
    if ( ts[2]==-1 ) { ts[2]= ts[3]; ts[3] = -1; }
    if ( ts[1]==-1 ) { ts[1]= ts[2]; ts[2] = ts[3]; ts[3] = -1; }
    if ( ts[0]==-1 ) { ts[0]= ts[1]; ts[1]= ts[2]; ts[2] = ts[3]; ts[3] = -1; }
return( ts[0]!=-1 );
}

real SplineSolve(Spline1D *sp, real tmin, real tmax, real sought,real err) {
    /* We want to find t so that spline(t) = sought */
    /*  the curve must be monotonic */
    /* returns t which is near sought or -1 */
    Spline1D temp;
    real ts[3];
    int i;
    real t;

    temp = *sp;
    temp.d -= sought;
    CubicSolve(&temp,ts);
    if ( tmax<tmin ) { t = tmax; tmax = tmin; tmin = t; }
    for ( i=0; i<3; ++i )
	if ( ts[i]>=tmin && ts[i]<=tmax )
return( ts[i] );

return( -1 );
}

void SplineFindInflections(Spline1D *sp, real *_t1, real *_t2 ) {
    real t1= -1, t2= -1;
    real b2_fourac;

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
	    if ( t1<t2 ) { real temp = t1; t1 = t2; t2 = temp; }
	    else if ( t1==t2 ) t2 = -1;
	    if ( RealNear(t1,0)) t1=0; else if ( RealNear(t1,1)) t1=1;
	    if ( RealNear(t2,0)) t2=0; else if ( RealNear(t2,1)) t2=1;
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

/* Ok, if the above routine finds a point of inflection that less than 1 unit */
/*  from an endpoint or another point of inflection, then many things are */
/*  just going to skip over it, and other things will be confused by this */
/*  so just remove it. It should be so close the difference won't matter */
void SplineRemoveInflectionsTooClose(Spline1D *sp, real *_t1, real *_t2 ) {
    real last, test;
    real t1= *_t1, t2 = *_t2;

    if ( t1>t2 && t2!=-1 ) {
	t1 = t2;
	t2 = *_t1;
    }
    last = sp->d;
    if ( t1!=-1 ) {
	test = ((sp->a*t1+sp->b)*t1+sp->c)*t1+sp->d;
	if ( (test-last)*(test-last)<1 )
	    t1 = -1;
	else
	    last = test;
    }
    if ( t2!=-1 ) {
	test = ((sp->a*t2+sp->b)*t2+sp->c)*t2+sp->d;
	if ( (test-last)*(test-last)<1 )
	    t2 = -1;
	else
	    last = test;
    }
    test = sp->a+sp->b+sp->c+sp->d;
    if ( (test-last)*(test-last)<1 ) {
	if ( t2!=-1 )
	    t2 = -1;
	else if ( t1!=-1 )
	    t1 = -1;
	else
	    /* Well we should just remove the whole spline? */;
    }
    *_t1 = t1; *_t2 = t2;
}

int SplineSolveFull(Spline1D *sp,real val, real ts[3]) {
    Spline1D temp;

    temp = *sp;
    temp.d -= val;
    CubicSolve(&temp,ts);
return( ts[0]!=-1 );
}

#if 0
static int CheckEndpoint(BasePoint *end,Spline *s,int te,BasePoint *pts,
	real *tarray,real *ts,int soln) {
    double t;
    int i;

    for ( i=0; i<soln; ++i )
	if ( end->x==pts[i].x && end->y==pts[i].y )
return( soln );

    t = SplineNearPoint(s,end,.01);
    if ( t<=0 || t>=1 )		/* If both splines are at end points then it's a join not an intersection */
return( soln );
    tarray[soln] = te;
    ts[soln] = t;
    pts[soln] = *end;
return( soln+1 );
}
#endif

static int AddPoint(real x,real y,real t,real s,BasePoint *pts,
	real t1s[3],real t2s[3], int soln) {
    int i;

    for ( i=0; i<soln; ++i )
	if ( x==pts[i].x && y==pts[i].y )
return( soln );
    t1s[soln] = t;
    t2s[soln] = s;
    pts[soln].x = x;
    pts[soln].y = y;
return( soln+1 );
}

static int AddQuadraticSoln(real s,Spline *s1, Spline *s2, BasePoint pts[3],
	real t1s[3], real t2s[3], int soln ) {
    double t, x, y, d;
    int i;

    if ( s<-.0001 || s>1.0001 )
return( soln );
    if ( s<0 ) s=0; else if ( s>1 ) s=1;

    x = (s2->splines[0].b*s+s2->splines[0].c)*s+s2->splines[0].d;
    y = (s2->splines[1].b*s+s2->splines[1].c)*s+s2->splines[1].d;

    for ( i=0; i<soln; ++i )
	if ( x==pts[i].x && y==pts[i].y )
return( soln );

    d = s1->splines[0].c*s1->splines[0].c-4*s1->splines[0].a*(s1->splines[0].d-x);
    if ( RealNear(d,0)) d = 0;
    if ( d<0 )
return( soln );
    t = (-s1->splines[0].c-d)/(2*s1->splines[0].b);
    if ( t>-.0001 && t<1.0001 ) {
	if ( t<=0 ) {t=0; x=s1->from->me.x; y = s1->from->me.y; }
	else if ( t>=1 ) { t=1; x=s1->to->me.x; y = s1->to->me.y; }
	if ( RealNear(y, (s1->splines[1].b*t+s1->splines[1].c)*t+s1->splines[1].d ) )
return( AddPoint(x,y,t,s,pts,t1s,t2s,soln));
    }
    t = (-s1->splines[0].c+d)/(2*s1->splines[0].b);
    if ( t>-.0001 && t<1.0001 ) {
	if ( t<=0 ) {t=0; x=s1->from->me.x; y = s1->from->me.y; }
	else if ( t>=1 ) { t=1; x=s1->to->me.x; y = s1->to->me.y; }
	if ( RealNear(y, (s1->splines[1].b*t+s1->splines[1].c)*t+s1->splines[1].d) )
return( AddPoint(x,y,t,s,pts,t1s,t2s,soln));
    }
return( soln );
}

/* returns 0=>no intersection, 1=>at least one, location in pts, t1s, t2s */
/*  -1 => We couldn't figure it out in a closed form, have to do a numerical */
/*  approximation */
int SplinesIntersect(Spline *s1, Spline *s2, BasePoint pts[4], real t1s[4], real t2s[4]) {
    BasePoint min1, max1, min2, max2;
    int soln = 0;
    double x,y,s,t;
    double d;
    int i;
    Spline1D spline, temp;
    Quartic quad;
    real tempts[4];	/* 3 solns for cubics, 4 for quartics */

    t1s[0] = t1s[1] = t1s[2] = t1s[3] = -1;
    t2s[0] = t2s[1] = t2s[2] = t2s[3] = -1;

    min1 = s1->from->me; max1 = min1;
    min2 = s2->from->me; max2 = min2;
    if ( s1->from->nextcp.x>max1.x ) max1.x = s1->from->nextcp.x;
    else if ( s1->from->nextcp.x<min1.x ) min1.x = s1->from->nextcp.x;
    if ( s1->from->nextcp.y>max1.y ) max1.y = s1->from->nextcp.y;
    else if ( s1->from->nextcp.y<min1.y ) min1.y = s1->from->nextcp.y;
    if ( s1->to->prevcp.x>max1.x ) max1.x = s1->to->prevcp.x;
    else if ( s1->to->prevcp.x<min1.x ) min1.x = s1->to->prevcp.x;
    if ( s1->to->prevcp.y>max1.y ) max1.y = s1->to->prevcp.y;
    else if ( s1->to->prevcp.y<min1.y ) min1.y = s1->to->prevcp.y;
    if ( s1->to->me.x>max1.x ) max1.x = s1->to->me.x;
    else if ( s1->to->me.x<min1.x ) min1.x = s1->to->me.x;
    if ( s1->to->me.y>max1.y ) max1.y = s1->to->me.y;
    else if ( s1->to->me.y<min1.y ) min1.y = s1->to->me.y;

    if ( s2->from->nextcp.x>max2.x ) max2.x = s2->from->nextcp.x;
    else if ( s2->from->nextcp.x<min2.x ) min2.x = s2->from->nextcp.x;
    if ( s2->from->nextcp.y>max2.y ) max2.y = s2->from->nextcp.y;
    else if ( s2->from->nextcp.y<min2.y ) min2.y = s2->from->nextcp.y;
    if ( s2->to->prevcp.x>max2.x ) max2.x = s2->to->prevcp.x;
    else if ( s2->to->prevcp.x<min2.x ) min2.x = s2->to->prevcp.x;
    if ( s2->to->prevcp.y>max2.y ) max2.y = s2->to->prevcp.y;
    else if ( s2->to->prevcp.y<min2.y ) min2.y = s2->to->prevcp.y;
    if ( s2->to->me.x>max2.x ) max2.x = s2->to->me.x;
    else if ( s2->to->me.x<min2.x ) min2.x = s2->to->me.x;
    if ( s2->to->me.y>max2.y ) max2.y = s2->to->me.y;
    else if ( s2->to->me.y<min2.y ) min2.y = s2->to->me.y;
    if ( min1.x>max2.x || min2.x>max1.x || min1.y>max2.y || min2.y>max1.y )
return( false );		/* no intersection of bounding boxes */

    /* Ignore splines which are just a point */
    if ( s1->knownlinear && s1->splines[0].c==0 && s1->splines[1].c==0 )
return( false );
    if ( s2->knownlinear && s2->splines[0].c==0 && s2->splines[1].c==0 )
return( false );

    if ( s1->knownlinear )
	/* Do Nothing */;
    else if ( s2->knownlinear || (!s1->isquadratic && s2->isquadratic)) {
	Spline *stemp = s1;
	real *ts = t1s;
	t1s = t2s; t2s = ts;
	s1 = s2; s2 = stemp;
    }

#if 0
    soln = CheckEndpoint(&s1->from->me,s2,0,pts,t1s,t2s,soln);
    soln = CheckEndpoint(&s1->to->me,s2,1,pts,t1s,t2s,soln);
    soln = CheckEndpoint(&s2->from->me,s1,0,pts,t2s,t1s,soln);
    soln = CheckEndpoint(&s2->to->me,s1,1,pts,t2s,t1s,soln);
#endif

    if ( s1->islinear ) {
	spline.d = s1->splines[1].c*(s2->splines[0].d-s1->splines[0].d)-
		s1->splines[0].c*(s2->splines[1].d-s1->splines[1].d);
	spline.c = s1->splines[1].c*s2->splines[0].c - s1->splines[0].c*s2->splines[1].c;
	spline.b = s1->splines[1].c*s2->splines[0].b - s1->splines[0].c*s2->splines[1].b;
	spline.a = s1->splines[1].c*s2->splines[0].a - s1->splines[0].c*s2->splines[1].a;
	CubicSolve(&spline,tempts);
	if ( tempts[0]==-1 )
return( false );
	for ( i = 0; i<3 && tempts[i]!=-1; ++i ) {
	    x = ((s2->splines[0].a*tempts[i]+s2->splines[0].b)*tempts[i]+
		    s2->splines[0].c)*tempts[i]+s2->splines[0].d;
	    y = ((s2->splines[1].a*tempts[i]+s2->splines[1].b)*tempts[i]+
		    s2->splines[1].c)*tempts[i]+s2->splines[1].d;
	    if ( s1->splines[0].c!=0 )
		t = (x-s1->splines[0].d)/s1->splines[0].c;
	    else
		t = (y-s1->splines[1].d)/s1->splines[1].c;
	    if ( t<-.001 || t>1.001 )
	continue;
	    if ( t<=0 ) {t=0; x=s1->from->me.x; y = s1->from->me.y; }
	    else if ( t>=1 ) { t=1; x=s1->to->me.x; y = s1->to->me.y; }
	    soln = AddPoint(x,y,t,tempts[i],pts,t1s,t2s,soln);
	}
return( soln!=0 );
    } else if ( s1->isquadratic && s2->isquadratic ) {
	temp.a = 0;
	temp.b = s1->splines[1].b*s2->splines[0].b - s1->splines[0].b*s2->splines[1].b;
	temp.c = s1->splines[1].b*s2->splines[0].c - s1->splines[0].b*s2->splines[1].c;
	temp.d = s1->splines[1].b*(s2->splines[0].d-s1->splines[0].d) -
		 s1->splines[0].b*(s2->splines[1].d-s1->splines[1].d);
	d = s1->splines[1].b*s1->splines[0].c - s1->splines[0].b*s1->splines[1].c;
	if ( RealNear(d,0)) d=0;
	if ( d!=0 ) {
	    temp.b /= d; temp.c /= d; temp.d /= d;
	    /* At this point t= temp.b*s^2 + temp.c*s + temp.d */
	    /* We substitute this back into one of our equations and get a */
	    /*  quartic in s */
	    quad.a = s1->splines[0].b*temp.b*temp.b;
	    quad.b = s1->splines[0].b*2*temp.b*temp.c;
	    quad.c = s1->splines[0].b*(2*temp.b*temp.d+temp.c*temp.c);
	    quad.d = s1->splines[0].b*2*temp.d*temp.c;
	    quad.e = s1->splines[0].b*temp.d*temp.d;
	    quad.b+= s1->splines[0].c*temp.b;
	    quad.c+= s1->splines[0].c*temp.c;
	    quad.d+= s1->splines[0].c*temp.d;
	    quad.e+= s1->splines[0].d;
	    quad.e-= s2->splines[0].d;
	    quad.d-= s2->splines[0].c;
	    quad.c-= s2->splines[0].b;
	    if ( QuarticSolve(&quad,tempts)==-1 )
return( -1 );
	    for ( i=0; i<4 && tempts[i]!=-999999; ++i )
		soln = AddQuadraticSoln(tempts[i],s1,s2,pts,t1s,t2s,soln);
	} else {
	    d = temp.c*temp.c-4*temp.b*temp.c;
	    if ( RealNear(d,0)) d = 0;
	    if ( d<0 )
return( soln!=0 );
	    d = sqrt(d);
	    s = (-temp.c-d)/(2*temp.b);
	    soln = AddQuadraticSoln(s,s1,s2,pts,t1s,t2s,soln);
	    s = (-temp.c+d)/(2*temp.b);
	    soln = AddQuadraticSoln(s,s1,s2,pts,t1s,t2s,soln);
	}
return( soln!=0 );
    }
    /* if one of the splines is quadratic then we can get an expression */
    /*  relating c*t+d to poly(s^3), and substituting this back we get */
    /*  a poly of degree 6 in s which could be solved iteratively */
    /* however mixed quadratics and cubics are unlikely */

    /* but if both splines are degree 3, the t is expressed as the sqrt of */
    /*  a third degree poly, which must be substituted into a cubic, and */
    /*  then squared to get rid of the sqrts leaving us with an ?18? degree */
    /*  poly. Ick. */
return( -1 );
}

static int XSolve(Spline *spline,real tmin, real tmax,FindSel *fs) {
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];
    real t,x,y;

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

static int YSolve(Spline *spline,real tmin, real tmax,FindSel *fs) {
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];
    real t,x,y;

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
    real t,y;
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];

    if ( xspline->a!=0 ) {
	real t1, t2, tbase;
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
    } else if ( xspline->b!=0 ) {
	real root = xspline->c*xspline->c - 4*xspline->b*(xspline->d-fs->p->cx);
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
    } else /* xspline->c can't be 0 because dx>dy => dx!=0 => xspline->c!=0 */ {
	fs->p->t = t = (fs->p->cx-xspline->d)/xspline->c;
	y = ((yspline->a*t+yspline->b)*t+yspline->c)*t + yspline->d;
	if ( fs->yl<y && fs->yh>y )
return( true );
    }
return( false );
}

int NearSpline(FindSel *fs, Spline *spline) {
    real t,x,y;
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];
    real dx, dy;

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
	if ( xspline->c==0 && yspline->c==0 ) 	/* It's a point. */
return( true );
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
	    real root = yspline->c*yspline->c - 4*yspline->b*(yspline->d-fs->p->cy);
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
	    real t1, t2, tbase;
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

real SplineNearPoint(Spline *spline, BasePoint *bp, real fudge) {
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
	chunkfree(hi,sizeof(HintInstance));
    }
    chunkfree(h,sizeof(StemInfo));
}

void StemInfosFree(StemInfo *h) {
    StemInfo *hnext;
    HintInstance *hi, *n;

    for ( ; h!=NULL; h = hnext ) {
	for ( hi=h->where; hi!=NULL; hi=n ) {
	    n = hi->next;
	    chunkfree(hi,sizeof(HintInstance));
	}
	hnext = h->next;
	chunkfree(h,sizeof(StemInfo));
    }
}

void DStemInfoFree(DStemInfo *h) {

    free(h);
}

void DStemInfosFree(DStemInfo *h) {
    DStemInfo *hnext;

    for ( ; h!=NULL; h = hnext ) {
	hnext = h->next;
	chunkfree(h,sizeof(DStemInfo));
    }
}

StemInfo *StemInfoCopy(StemInfo *h) {
    StemInfo *head=NULL, *last=NULL, *cur;
    HintInstance *hilast, *hicur, *hi;

    for ( ; h!=NULL; h = h->next ) {
	cur = chunkalloc(sizeof(StemInfo));
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
	    hicur = chunkalloc(sizeof(StemInfo));
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

DStemInfo *DStemInfoCopy(DStemInfo *h) {
    DStemInfo *head=NULL, *last=NULL, *cur;

    for ( ; h!=NULL; h = h->next ) {
	cur = chunkalloc(sizeof(DStemInfo));
	*cur = *h;
	cur->next = NULL;
	if ( head==NULL )
	    head = last = cur;
	else {
	    last->next = cur;
	    last = cur;
	}
    }
return( head );
}

MinimumDistance *MinimumDistanceCopy(MinimumDistance *md) {
    MinimumDistance *head=NULL, *last=NULL, *cur;

    for ( ; md!=NULL; md = md->next ) {
	cur = chunkalloc(sizeof(DStemInfo));
	*cur = *md;
	cur->next = NULL;
	if ( head==NULL )
	    head = last = cur;
	else {
	    last->next = cur;
	    last = cur;
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
    free(lig->components);
    free(lig);
}

void MinimumDistancesFree(MinimumDistance *md) {
    MinimumDistance *next;

    while ( md!=NULL ) {
	next = md->next;
	chunkfree(md,sizeof(MinimumDistance));
	md = next;
    }
}

void TTFLangNamesFree(struct ttflangname *l) {
    struct ttflangname *next;
    int i;

    while ( l!=NULL ) {
	next = l->next;
	for ( i=0; i<ttf_namemax; ++i )
	    free(l->names[i]);
	free(l);
	l = next;
    }
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
    MinimumDistancesFree(sc->md);
    KernPairsFree(sc->kerns);
    UndoesFree(sc->undoes[0]); UndoesFree(sc->undoes[1]);
    UndoesFree(sc->redoes[0]); UndoesFree(sc->redoes[1]);
    for ( dlist=sc->dependents; dlist!=NULL; dlist = dnext ) {
	dnext = dlist->next;
	free(dlist);
    }
    LigatureFree(sc->lig);
    chunkfree(sc,sizeof(SplineChar));
}

void SplineFontFree(SplineFont *sf) {
    int i;

    if ( sf==NULL )
return;
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
    TTFLangNamesFree(sf->names);
    for ( i=0; i<sf->subfontcnt; ++i )
	SplineFontFree(sf->subfonts[i]);
    free(sf->subfonts);
    free(sf);
}
