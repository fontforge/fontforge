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
/*  into splinefont.h after (or instead of) the definition of chunkalloc()*/

#ifndef chunkalloc
#define ALLOC_CHUNK	100		/* Number of small chunks to malloc at a time */
#define CHUNK_MAX	40		/* Maximum size (in chunk units) that we are prepared to allocate */
					/* The size of our data structures */
# define CHUNK_UNIT	sizeof(void *)	/*  will vary with the word size of */
					/*  the machine. if pointers are 64 bits*/
					/*  we may need twice as much space as for 32 bits */

#ifdef FLAG
#undef FLAG
#define FLAG 0xbadcafe
#endif

#ifdef CHUNKDEBUG
static int chunkdebug = 0;	/* When this is set we never free anything, insuring that each chunk is unique */
#endif

#if ALLOC_CHUNK>1
struct chunk { struct chunk *next; };
struct chunk2 { struct chunk2 *next; int flag; };
static struct chunk *chunklists[CHUNK_MAX] = { 0 };
#endif

#if defined(FLAG) && ALLOC_CHUNK>1
void chunktest(void) {
    int i;
    struct chunk2 *c;

    for ( i=2; i<CHUNK_MAX; ++i )
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

    if ( (size&(CHUNK_UNIT-1)) || size>=CHUNK_MAX*CHUNK_UNIT || size<=sizeof(struct chunk)) {
	fprintf( stderr, "Attempt to allocate something of size %d\n", size );
return( gcalloc(1,size));
    }
#ifdef FLAG
    chunktest();
#endif
    index = (size+CHUNK_UNIT-1)/CHUNK_UNIT;
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
    int index = (size+CHUNK_UNIT-1)/CHUNK_UNIT;
#ifdef CHUNKDEBUG
    if ( chunkdebug )
return;
#endif
# if ALLOC_CHUNK<=1
    free(item);
# else
    if ( item==NULL )
return;
    if ( (size&(CHUNK_UNIT-1)) || size>=CHUNK_MAX*CHUNK_UNIT || size<=sizeof(struct chunk)) {
	fprintf( stderr, "Attempt to free something of size %d\n", size );
	free(item);
    } else {
#ifdef LOCAL_DEBUG
	if ( (char *) (chunklists[index]) == (char *) item ||
		( ((char *) (chunklists[index]))<(char *) item &&
		  ((char *) (chunklists[index]))+size>(char *) item) ||
		( ((char *) (chunklists[index]))>(char *) item &&
		  ((char *) (chunklists[index]))<((char *) item)+size))
	      GDrawIError( "Memory mixup. Chunk list is wrong!!!" );
#endif
	((struct chunk *) item)->next = chunklists[index];
#  ifdef FLAG
	if ( size>=sizeof(struct chunk2))
	    ((struct chunk2 *) item)->flag = FLAG;
#  endif
	chunklists[index] = (struct chunk *) item;
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

SplinePoint *SplinePointCreate(real x, real y) {
    SplinePoint *sp = chunkalloc(sizeof(SplinePoint));
    sp->me.x = x; sp->me.y = y;
    sp->nextcp = sp->prevcp = sp->me;
    sp->nonextcp = sp->noprevcp = true;
    sp->nextcpdef = sp->prevcpdef = false;
    sp->ttfindex = 0xfffe;
return( sp );
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
    int nonext;

    if ( spl==NULL )
return;
    nonext = spl->first->next==NULL;
    if ( spl->first!=NULL ) {
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = next ) {
	    next = spline->to->next;
	    SplinePointFree(spline->to);
	    SplineFree(spline);
	    if ( first==NULL ) first = spline;
	}
	if ( spl->last!=spl->first || nonext )
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
	chunkfree(imgs,sizeof(ImageList));
	imgs = inext;
    }
}

void SplineRefigure3(Spline *spline) {
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
    spline->isquadratic = false;
    if ( !spline->knownlinear && xsp->a==0 && ysp->a==0 )
	spline->isquadratic = true;	/* Only likely if we read in a TTF */
    spline->order2 = false;
}

Spline *SplineMake3(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    SplineRefigure3(spline);
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
    factor = 1000.0/(sf->ascent+sf->descent);
    bounds->maxx *= factor; bounds->minx *= factor; bounds->miny *= factor; bounds->maxy *= factor;
    for ( i=1; i<cidmaster->subfontcnt; ++i ) {
	sf = cidmaster->subfonts[i];
	SplineFontFindBounds(sf,&b);
	factor = 1000.0/(sf->ascent+sf->descent);
	b.maxx *= factor; b.minx *= factor; b.miny *= factor; b.maxy *= factor;
	if ( b.maxx>bounds->maxx ) bounds->maxx = b.maxx;
	if ( b.maxy>bounds->maxy ) bounds->maxy = b.maxy;
	if ( b.miny<bounds->miny ) bounds->miny = b.miny;
	if ( b.minx<bounds->minx ) bounds->minx = b.minx;
    }
}

static void SplineSetFindTop(SplineSet *ss,BasePoint *top) {
    SplinePoint *sp;

    top->y = -1e10;
    for ( ; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->me.y > top->y ) *top = sp->me;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    if ( top->y < -65536 ) top->y = top->x = 0;
}

void SplineSetQuickBounds(SplineSet *ss,DBounds *b) {
    SplinePoint *sp;

    b->minx = b->miny = 1e10;
    b->maxx = b->maxy = -1e10;
    for ( ; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->me.y < b->miny ) b->miny = sp->me.y;
	    if ( sp->me.x < b->minx ) b->minx = sp->me.x;
	    if ( sp->me.y > b->maxy ) b->maxy = sp->me.y;
	    if ( sp->me.x > b->maxx ) b->maxx = sp->me.x;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    if ( b->minx>65536 ) b->minx = 0;
    if ( b->miny>65536 ) b->miny = 0;
    if ( b->maxx<-65536 ) b->maxx = 0;
    if ( b->maxy<-65536 ) b->maxy = 0;
}

void SplineCharQuickBounds(SplineChar *sc, DBounds *b) {
    RefChar *ref;

    SplineSetQuickBounds(sc->splines,b);
    for ( ref = sc->refs; ref!=NULL; ref = ref->next ) {
	/*SplineSetQuickBounds(ref->splines,&temp);*/
	if ( b->minx==0 && b->maxx==0 && b->miny==0 && b->maxy == 0 )
	    *b = ref->bb;
	else if ( ref->bb.minx!=0 || ref->bb.maxx != 0 || ref->bb.maxy != 0 || ref->bb.miny!=0 ) {
	    if ( ref->bb.minx < b->minx ) b->minx = ref->bb.minx;
	    if ( ref->bb.miny < b->miny ) b->miny = ref->bb.miny;
	    if ( ref->bb.maxx > b->maxx ) b->maxx = ref->bb.maxx;
	    if ( ref->bb.maxy > b->maxy ) b->maxy = ref->bb.maxy;
	}
    }
}

void SplineSetQuickConservativeBounds(SplineSet *ss,DBounds *b) {
    SplinePoint *sp;

    b->minx = b->miny = 1e10;
    b->maxx = b->maxy = -1e10;
    for ( ; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    if ( sp->me.y < b->miny ) b->miny = sp->me.y;
	    if ( sp->me.x < b->minx ) b->minx = sp->me.x;
	    if ( sp->me.y > b->maxy ) b->maxy = sp->me.y;
	    if ( sp->me.x > b->maxx ) b->maxx = sp->me.x;
	    if ( sp->nextcp.y < b->miny ) b->miny = sp->nextcp.y;
	    if ( sp->nextcp.x < b->minx ) b->minx = sp->nextcp.x;
	    if ( sp->nextcp.y > b->maxy ) b->maxy = sp->nextcp.y;
	    if ( sp->nextcp.x > b->maxx ) b->maxx = sp->nextcp.x;
	    if ( sp->prevcp.y < b->miny ) b->miny = sp->prevcp.y;
	    if ( sp->prevcp.x < b->minx ) b->minx = sp->prevcp.x;
	    if ( sp->prevcp.y > b->maxy ) b->maxy = sp->prevcp.y;
	    if ( sp->prevcp.x > b->maxx ) b->maxx = sp->prevcp.x;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    if ( b->minx>65536 ) b->minx = 0;
    if ( b->miny>65536 ) b->miny = 0;
    if ( b->maxx<-65536 ) b->maxx = 0;
    if ( b->maxy<-65536 ) b->maxy = 0;
}

void SplineCharQuickConservativeBounds(SplineChar *sc, DBounds *b) {
    RefChar *ref;

    SplineSetQuickConservativeBounds(sc->splines,b);
    for ( ref = sc->refs; ref!=NULL; ref = ref->next ) {
	/*SplineSetQuickConservativeBounds(ref->splines,&temp);*/
	if ( b->minx==0 && b->maxx==0 && b->miny==0 && b->maxy == 0 )
	    *b = ref->bb;
	else if ( ref->bb.minx!=0 || ref->bb.maxx != 0 || ref->bb.maxy != 0 || ref->bb.miny!=0 ) {
	    if ( ref->bb.minx < b->minx ) b->minx = ref->bb.minx;
	    if ( ref->bb.miny < b->miny ) b->miny = ref->bb.miny;
	    if ( ref->bb.maxx > b->maxx ) b->maxx = ref->bb.maxx;
	    if ( ref->bb.maxy > b->maxy ) b->maxy = ref->bb.maxy;
	}
    }
}

void SplineFontQuickConservativeBounds(SplineFont *sf,DBounds *b) {
    DBounds bb;
    int i;

    b->minx = b->miny = 1e10;
    b->maxx = b->maxy = -1e10;
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	SplineCharQuickConservativeBounds(sf->chars[i],&bb);
	if ( bb.minx < b->minx ) b->minx = bb.minx;
	if ( bb.miny < b->miny ) b->miny = bb.miny;
	if ( bb.maxx > b->maxx ) b->maxx = bb.maxx;
	if ( bb.maxy > b->maxy ) b->maxy = bb.maxy;
    }
    if ( b->minx>65536 ) b->minx = 0;
    if ( b->miny>65536 ) b->miny = 0;
    if ( b->maxx<-65536 ) b->maxx = 0;
    if ( b->maxy<-65536 ) b->maxy = 0;
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

SplinePointList *SplinePointListCopy1(SplinePointList *spl) {
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
	    for ( last = cur; last->next ; last = last->next );
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
    int allsel, anysel, alldone=true;

    for ( spl = base; spl!=NULL; spl = spl->next ) {
	pfirst = NULL;
	allsel = true; anysel=false;
	for ( spt = spl->first ; spt!=pfirst; spt = spt->next->to ) {
	    if ( pfirst==NULL ) pfirst = spt;
	    if ( allpoints || spt->selected ) {
		TransformPoint(spt,transform);
		anysel = true;
	    } else
		allsel = alldone = false;
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
	if ( !allpoints && !allsel && spl->first->next!=NULL && !spl->first->next->order2 ) {
	    pfirst = NULL;
	    for ( spt = spl->first ; spt!=pfirst; spt = spt->next->to ) {
		if ( pfirst==NULL ) pfirst = spt;
		if ( spt->prev!=NULL && spt->prevcpdef )
		    SplineCharDefaultPrevCP(spt);
		if ( spt->next==NULL )
	    break;
		if ( spt->nextcpdef )
		    SplineCharDefaultNextCP(spt);
	    }
	}
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( !alldone ) SplineRefigureFixup(spline); else SplineRefigure(spline);
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

    if ( dependent->searcherdummy )
return;

    for ( dlist=base->dependents; dlist!=NULL && dlist->sc!=dependent; dlist = dlist->next);
    if ( dlist==NULL ) {
	dlist = chunkalloc(sizeof(struct splinecharlist));
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
	    fprintf( stderr, "Couldn't find referenced character \"%s\" in %s\n",
		    AdobeStandardEncoding[refs->adobe_enc], dsc->name);
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
    if ( sf->uniqueid==0 ) sf->uniqueid = fd->uniqueid;
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

SplineChar *MakeDupRef(SplineChar *base, int local_enc, int uni_enc) {
    SplineChar *sc = SplineCharCreate();

    sc->enc = local_enc;
    sc->unicodeenc = uni_enc;
    sc->width = base->width;
    sc->vwidth = base->vwidth;
    sc->name = copy(base->name);
    sc->parent = base->parent;

    /* Used not to bother for spaces, but SCDuplicate depends on the ref */
    /*if ( dup->sc->refs!=NULL || dup->sc->splines!=NULL )*/ {
	RefChar *ref = chunkalloc(sizeof(RefChar));
	sc->refs = ref;
	ref->sc = base;
	ref->local_enc = base->enc;
	ref->unicode_enc = base->unicodeenc;
	ref->adobe_enc = getAdobeEnc(base->name);
	ref->transform[0] = ref->transform[3] = 1;
	SCReinstanciateRefChar(sc,ref);
	SCMakeDependent(sc,base);
	ref->checked = true;
    }
return( sc );
}

static SplineChar *DuplicateNameReference(SplineFont *sf,char **encoding,int encindex) {
    SplineChar *sc = NULL;
    int i;

    for ( i=0; i<encindex; ++i )
	if ( i!=encindex && strcmp(encoding[i],encoding[encindex])==0 )
    break;

    if ( i<encindex )
	sc = MakeDupRef(sf->chars[i],encindex,-1);
return( sc );
}

static void TransByFontMatrix(SplineFont *sf,real fontmatrix[6]) {
    real trans[6];
    int em = sf->ascent+sf->descent, i;
    SplineChar *sc;
    RefChar *refs;

    if ( fontmatrix[0]==fontmatrix[3] &&
	    fontmatrix[1]==0 && fontmatrix[2]==0 &&
	    fontmatrix[4]==0 && fontmatrix[5]==0 )
return;		/* It's just the expected matrix */

    trans[0] = 1;
    if ( fontmatrix[0]==fontmatrix[3] ) trans[3] = 1;
    else trans[3] = rint(fontmatrix[3]*em);
    trans[1] = fontmatrix[1]*em;
    trans[2] = fontmatrix[2]*em;
    trans[4] = rint(fontmatrix[4]*em);
    trans[5] = rint(fontmatrix[5]*em);

    for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL ) {
	SplinePointListTransform(sc->splines,trans,true);
	for ( refs=sc->refs; refs!=NULL; refs=refs->next ) {
	    /* Just scale the offsets. we'll do all the base characters */
	    double temp = refs->transform[4]*trans[0] +
			refs->transform[5]*trans[2] +
			/*transform[4]*/0;
	    refs->transform[5] = refs->transform[4]*trans[1] +
			refs->transform[5]*trans[3] +
			/*transform[5]*/0;
	    refs->transform[4] = temp;
	}
	sc->changedsincelasthinted = true;
	sc->manualhints = false;
    }
    for ( i=0; i<sf->charcnt; ++i ) if ( (sc=sf->chars[i])!=NULL ) {
	for ( refs=sc->refs; refs!=NULL; refs=refs->next )
	    SCReinstanciateRefChar(sc,refs);
    }
}

static void SplineFontFromType1(SplineFont *sf, FontDict *fd) {
    int i, isnotdef;
    RefChar *refs, *next, *pr;
    char **encoding;
    int istype2 = fd->fonttype==2;		/* Easy enough to deal with even though it will never happen... */
    uint8 *used = gcalloc(fd->chars->next,sizeof(uint8));

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
	isnotdef = false;
	if ( k==-1 ) {
	    k = k2 = LookupCharString(encoding[i],(struct pschars *) (fd->charprocs));
	    isnotdef = (k==-1);
	    if ( k==-1 )
		k = LookupCharString(".notdef",fd->chars);
	    if ( k==-1 )
		k = k2 = LookupCharString(".notdef",(struct pschars *) (fd->charprocs));
	}
	if ( k==-1 ) {
	    /* 0 500 hsbw endchar */
	    sf->chars[i] = PSCharStringToSplines((uint8 *) "\213\370\210\015\016",5,false,fd->private->subrs,NULL,".notdef");
	    sf->chars[i]->width = sf->ascent+sf->descent;
	} else if ( used[k] && !isnotdef && strcmp(encoding[i],".notdef")!=0 ) {
	    sf->chars[i] = DuplicateNameReference(sf,encoding,i);
	} else if ( k2==-1 ) {
	    sf->chars[i] = PSCharStringToSplines(fd->chars->values[k],fd->chars->lens[k],
		    istype2,fd->private->subrs,NULL,encoding[i]);
	    used[k] = true;
	} else {
	    if ( fd->charprocs->values[k]->unicodeenc==-2 )
		sf->chars[i] = fd->charprocs->values[k];
	    else
		sf->chars[i] = SplineCharCopy(fd->charprocs->values[k],sf);
	    used[k] = true;
	}
	sf->chars[i]->ttf_glyph = k;
	sf->chars[i]->vwidth = sf->ascent+sf->descent;
	sf->chars[i]->enc = i;
	sf->chars[i]->unicodeenc = UniFromName(encoding[i]);
	sf->chars[i]->parent = sf;
	SCLigDefault(sf->chars[i]);		/* Also reads from AFM file, but it probably doesn't exist */
	GProgressNext();
    }
    for ( i=0; i<sf->charcnt; ++i ) for ( pr=NULL, refs = sf->chars[i]->refs; refs!=NULL; refs=next ) {
	next = refs->next;
	sf->chars[i]->ticked = true;
	InstanciateReference(sf, refs, refs, refs->transform,sf->chars[i]);
	if ( refs->sc!=NULL ) {
	    SplineSetFindBounds(refs->splines,&refs->bb);
	    sf->chars[i]->ticked = false;
	    pr = refs;
	} else {
	    /* In some mal-formed postscript fonts we can have a reference */
	    /*  to a character that is not actually in the font. I even */
	    /*  generated one by mistake once... */
	    if ( pr==NULL )
		sf->chars[i]->refs = next;
	    else
		pr->next = next;
	}
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
    free(used);
    /* sometimes (some apple oblique fonts) the fontmatrix is not just a */
    /*  formality, it acutally contains a skew. So be ready */
    if ( fd->fontmatrix[0]!=0 )
	TransByFontMatrix(sf,fd->fontmatrix);
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

    map = FindCidMap(sf->cidregistry,sf->ordering,sf->supplement,sf);

    chars = gcalloc(fd->cidcnt,sizeof(SplineChar *));
    for ( i=0; i<fd->cidcnt; ++i ) if ( fd->cidlens[i]>0 ) {
	j = fd->cidfds[i];		/* We get font indexes of 255 for non-existant chars */
	uni = CID2NameEnc(map,i,buffer,sizeof(buffer));
	chars[i] = PSCharStringToSplines(fd->cidstrs[i],fd->cidlens[i],
		    fd->fds[j]->fonttype==2,fd->fds[j]->private->subrs,
		    NULL,buffer);
	chars[i]->vwidth = sf->subfonts[j]->ascent+sf->subfonts[j]->descent;
	chars[i]->unicodeenc = uni;
	chars[i]->enc = i;
	chars[i]->ttf_glyph = i;
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
    if ( loaded_fonts_same_as_new && new_fonts_are_order2 )
	SFConvertToOrder2(sf);
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
    SplineSetFindTop(rf->splines,&rf->top);
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
	chunkfree(dlist,sizeof(struct splinecharlist));
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
    real t, low, high, test;

    temp = *sp;
    temp.d -= sought;
    CubicSolve(&temp,ts);
    if ( tmax<tmin ) { t = tmax; tmax = tmin; tmin = t; }
    for ( i=0; i<3; ++i )
	if ( ts[i]>=tmin && ts[i]<=tmax )
return( ts[i] );

    /* Now the CubicSolver can have rounding errors such that the answer it */
    /*  gives is actually outside of the allowed range. So use an iterative */
    /*  approach if it fails */
    low = ((temp.a*tmin+temp.b)*tmin+temp.c)*tmin+temp.d;
    high = ((temp.a*tmax+temp.b)*tmax+temp.c)*tmax+temp.d;
    if ( low==0 )
return(tmin);
    if ( high==0 )
return(tmax);
    if (( low<0 && high>0 ) ||
	    ( low>0 && high<0 )) {
	while ( tmax-tmin>=err ) {
	    t = (tmax+tmin)/2;
	    test = ((temp.a*t+temp.b)*t+temp.c)*t+temp.d;
	    if ( test==0 )
return( t );
	    if ( (low<0 && test<0) || (low>0 && test>0) )
		tmin=t;
	    else
		tmax = t;
	}
return( (tmax+tmin)/2 );	
    } else if ( low>0 && high<0 ) {
    }
return( -1 );
}

void SplineFindExtrema(Spline1D *sp, real *_t1, real *_t2 ) {
    real t1= -1, t2= -1;
    real b2_fourac;

    /* Find the extreme points on the curve */
    /*  Set to -1 if there are none or if they are outside the range [0,1] */
    /*  Order them so that t1<t2 */
    /*  If only one valid inflection it will be t1 */
    /*  (Does not check the end points unless they have derivative==0) */
    if ( sp->a!=0 ) {
	/* cubic, possibly 2 extrema (possibly none) */
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
	/* Quadratic, at most one extremum */
	t1 = -sp->c/(2.0*sp->b);
	if ( t1<=0 || t1>=1 ) t1 = -1;
    } else /*if ( sp->c!=0 )*/ {
	/* linear, no extrema */
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

    if ( s1->knownlinear ) {
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
	SplineFindExtrema(xspline,&t1,&t2);
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
	    SplineFindExtrema(yspline,&t1,&t2);
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
	chunkfree(kp,sizeof(KernPair));
    }
}

static AnchorPoint *AnchorPointsRemoveName(AnchorPoint *alist,AnchorClass *an) {
    AnchorPoint *prev=NULL, *ap, *next;

    for ( ap=alist; ap!=NULL; ap=next ) {
	next = ap->next;
	if ( ap->anchor == an ) {
	    if ( prev==NULL )
		alist = next;
	    else
		prev->next = next;
	    chunkfree(ap,sizeof(AnchorPoint));
	    if ( ap->type==at_mark || ap->type==at_basechar || ap->type==at_basemark )
		next = NULL;		/* Names only occur once, unless it's a ligature or cursive */
	} else
	    prev = ap;
    }
return( alist );
}

static void SCRemoveAnchorClass(SplineChar *sc,AnchorClass *an) {
    Undoes *test;

    if ( sc==NULL )
return;
    sc->anchor = AnchorPointsRemoveName(sc->anchor,an);
    for ( test = sc->undoes[0]; test!=NULL; test=test->next )
	if ( test->undotype==ut_state || test->undotype==ut_tstate ||
		test->undotype==ut_statehint || test->undotype==ut_statename )
	    test->u.state.anchor = AnchorPointsRemoveName(test->u.state.anchor,an);
    for ( test = sc->redoes[0]; test!=NULL; test=test->next )
	if ( test->undotype==ut_state || test->undotype==ut_tstate ||
		test->undotype==ut_statehint || test->undotype==ut_statename )
	    test->u.state.anchor = AnchorPointsRemoveName(test->u.state.anchor,an);
}

void SFRemoveAnchorClass(SplineFont *sf,AnchorClass *an) {
    int i;
    AnchorClass *prev, *test;

    PasteRemoveAnchorClass(sf,an);

    for ( i=0; i<sf->charcnt; ++i )
	SCRemoveAnchorClass(sf->chars[i],an);
    prev = NULL;
    for ( test=sf->anchor; test!=NULL; test=test->next ) {
	if ( test==an ) {
	    if ( prev==NULL )
		sf->anchor = test->next;
	    else
		prev->next = test->next;
	    chunkfree(test,sizeof(AnchorClass));
    break;
	} else
	    prev = test;
    }
}

AnchorPoint *APAnchorClassMerge(AnchorPoint *anchors,AnchorClass *into,AnchorClass *from) {
    AnchorPoint *api=NULL, *prev, *ap, *next;

    prev = NULL;
    for ( ap=anchors; ap!=NULL; ap=next ) {
	next = ap->next;
	if ( ap->anchor==from ) {
	    for ( api=anchors; api!=NULL; api=api->next ) {
		if ( api->anchor==into &&
			(api->type!=at_baselig || ap->type!=at_baselig || api->lig_index==ap->lig_index))
	    break;
	    }
	    if ( api==NULL && into!=NULL ) {
		ap->anchor = into;
		prev = ap;
	    } else {
		if ( prev==NULL )
		    anchors = next;
		else
		    prev->next = next;
		chunkfree(ap,sizeof(AnchorPoint));
	    }
	} else
	    prev = ap;
    }
return( anchors );
}

void AnchorClassMerge(SplineFont *sf,AnchorClass *into,AnchorClass *from) {
    int i;

    if ( into==from )
return;
    PasteAnchorClassMerge(sf,into,from);
    for ( i=0; i<sf->charcnt; ++i ) if ( sf->chars[i]!=NULL ) {
	SplineChar *sc = sf->chars[i];

	sc->anchor = APAnchorClassMerge(sc->anchor,into,from);
    }
}

AnchorPoint *AnchorPointsCopy(AnchorPoint *alist) {
    AnchorPoint *head=NULL, *last, *ap;

    while ( alist!=NULL ) {
	ap = chunkalloc(sizeof(AnchorPoint));
	*ap = *alist;
	if ( head==NULL )
	    head = ap;
	else
	    last->next = ap;
	last = ap;
	alist = alist->next;
    }
return( head );
}

void AnchorPointsFree(AnchorPoint *ap) {
    AnchorPoint *anext;
    for ( ; ap!=NULL; ap = anext ) {
	anext = ap->next;
	chunkfree(ap,sizeof(AnchorPoint));
    }
}

void PSTFree(PST *pst) {
    PST *pnext;
    for ( ; pst!=NULL; pst = pnext ) {
	pnext = pst->next;
	if ( pst->type==pst_lcaret )
	    free(pst->u.lcaret.carets);
	else if ( pst->type!=pst_position )
	    free(pst->u.subs.variant);
	chunkfree(pst,sizeof(PST));
    }
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

SplineChar *SplineCharCreate(void) {
    SplineChar *sc = chunkalloc(sizeof(SplineChar));
    sc->color = COLOR_DEFAULT;
    sc->ttf_glyph = -1;
return( sc );
}

void SplineCharListsFree(struct splinecharlist *dlist) {
    struct splinecharlist *dnext;
    for ( ; dlist!=NULL; dlist = dnext ) {
	dnext = dlist->next;
	chunkfree(dlist,sizeof(struct splinecharlist));
    }
}

void SplineCharFreeContents(SplineChar *sc) {

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
    AnchorPointsFree(sc->anchor);
    UndoesFree(sc->undoes[0]); UndoesFree(sc->undoes[1]);
    UndoesFree(sc->redoes[0]); UndoesFree(sc->redoes[1]);
    SplineCharListsFree(sc->dependents);
    PSTFree(sc->possub);
    free(sc->ttf_instrs);
}

void SplineCharFree(SplineChar *sc) {

    if ( sc==NULL )
return;
    SplineCharFreeContents(sc);
    chunkfree(sc,sizeof(SplineChar));
}

void AnchorClassesFree(AnchorClass *an) {
    AnchorClass *anext;
    for ( ; an!=NULL; an = anext ) {
	anext = an->next;
	chunkfree(an,sizeof(AnchorClass));
    }
}

void TableOrdersFree(struct table_ordering *ord) {
    struct table_ordering *onext;
    for ( ; ord!=NULL; ord = onext ) {
	onext = ord->next;
	free(ord->ordered_features);
	chunkfree(ord,sizeof(struct table_ordering));
    }
}

void TtfTablesFree(struct ttf_table *tab) {
    struct ttf_table *next;

    for ( ; tab!=NULL; tab = next ) {
	next = tab->next;
	free(tab->data);
	chunkfree(tab,sizeof(struct ttf_table));
    }
}

void ScriptRecordFree(struct script_record *sr) {
    int i;

    for ( i=0; sr[i].script!=0; ++i )
	free( sr[i].langs );
    free( sr );
}

void ScriptRecordListFree(struct script_record **script_lang) {
    int i;

    if ( script_lang==NULL )
return;
    for ( i=0; script_lang[i]!=NULL; ++i )
	ScriptRecordFree(script_lang[i]);
    free( script_lang );
}

void SplineFontFree(SplineFont *sf) {
    int i;
    BDFFont *bdf, *bnext;

    if ( sf==NULL )
return;
    PasteRemoveSFAnchors(sf);
    for ( bdf = sf->bitmaps; bdf!=NULL; bdf = bnext ) {
	bnext = bdf->next;
	BDFFontFree(bdf);
    }
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
    free(sf->xuid);
    free(sf->cidregistry);
    free(sf->ordering);
    SplinePointListsFree(sf->gridsplines);
    AnchorClassesFree(sf->anchor);
    TableOrdersFree(sf->orders);
    TtfTablesFree(sf->ttf_tables);
    UndoesFree(sf->gundoes);
    UndoesFree(sf->gredoes);
    PSDictFree(sf->private);
    TTFLangNamesFree(sf->names);
    for ( i=0; i<sf->subfontcnt; ++i )
	SplineFontFree(sf->subfonts[i]);
    free(sf->subfonts);
    free(sf->remap);
    GlyphHashFree(sf);
    ScriptRecordListFree(sf->script_lang);
    free(sf);
}
