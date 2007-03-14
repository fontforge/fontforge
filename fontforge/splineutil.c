/* Copyright (C) 2000-2007 by George Williams */
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
#include "pfaeditui.h"
#include <math.h>
#include "psfont.h"
#include "ustring.h"
#include "utype.h"
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#include "views.h"		/* for FindSel structure */
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif

/*#define DEBUG 1*/

typedef struct quartic {
    double a,b,c,d,e;
} Quartic;

/* In an attempt to make allocation more efficient I just keep preallocated */
/*  lists of certain common sizes. It doesn't seem to make much difference */
/*  when allocating stuff, but does when freeing. If the extra complexity */
/*  is bad then put:							  */
/*	#define chunkalloc(size)	gcalloc(1,size)			  */
/*	#define chunkfree(item,size)	free(item)			  */
/*  into splinefont.h after (or instead of) the definition of chunkalloc()*/

#ifndef chunkalloc
#define ALLOC_CHUNK	100		/* Number of small chunks to malloc at a time */
#if !defined(FONTFORGE_CONFIG_USE_LONGDOUBLE) && !defined(FONTFORGE_CONFIG_USE_DOUBLE)
# define CHUNK_MAX	65		/* Maximum size (in chunk units) that we are prepared to allocate */
					/* The size of our data structures */
#else
# define CHUNK_MAX	129
#endif
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

    if ( size&(CHUNK_UNIT-1) )
	size = (size+CHUNK_UNIT-1)&~(CHUNK_UNIT-1);

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

    if ( size&(CHUNK_UNIT-1) )
	size = (size+CHUNK_UNIT-1)&~(CHUNK_UNIT-1);

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
	      IError( "Memory mixup. Chunk list is wrong!!!" );
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

char *strconcat(const char *str1,const char *str2) {
    int len1 = strlen(str1);
    char *ret = galloc(len1+strlen(str2)+1);
    strcpy(ret,str1);
    strcpy(ret+len1,str2);
return( ret );
}

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
    sp->ttfindex = sp->nextcpindex = 0xfffe;
return( sp );
}

Spline *SplineMake3(SplinePoint *from, SplinePoint *to) {
    Spline *spline = chunkalloc(sizeof(Spline));

    spline->from = from; spline->to = to;
    from->next = to->prev = spline;
    SplineRefigure3(spline);
return( spline );
}

void SplinePointFree(SplinePoint *sp) {
    chunkfree(sp->hintmask,sizeof(HintMask));
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

    chunkfree(sp->hintmask,sizeof(HintMask));
    chunkfree(sp,sizeof(SplinePoint));
}

void SplinePointsFree(SplinePointList *spl) {
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
}

void SplinePointListFree(SplinePointList *spl) {
    Spline *first, *spline, *next;
    int nonext;

    if ( spl==NULL )
return;
    if ( spl->first!=NULL ) {
	nonext = spl->first->next==NULL;
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
    int freefirst;

    if ( spl==NULL )
return;
    if ( spl->first!=NULL ) {
	first = NULL;
	freefirst = ( spl->last!=spl->first || spl->first->next==NULL );
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = next ) {
	    next = spline->to->next;
	    SplinePointMDFree(sc,spline->to);
	    SplineFree(spline);
	    if ( first==NULL ) first = spline;
	}
	if ( freefirst )
	    SplinePointMDFree(sc,spl->first);
    }
    chunkfree(spl,sizeof(SplinePointList));
}

void SplinePointListsMDFree(SplineChar *sc,SplinePointList *spl) {
    SplinePointList *next;

    while ( spl!=NULL ) {
	next = spl->next;
	SplinePointListMDFree(sc,spl);
	spl = next;
    }
}

void SplinePointListsFree(SplinePointList *head) {
    SplinePointList *spl, *next;

    for ( spl=head; spl!=NULL; spl=next ) {
	next = spl->next;
	SplinePointListFree(spl);
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

void RefCharFree(RefChar *ref) {
#ifdef FONTFORGE_CONFIG_TYPE3
    int i;

    if ( ref==NULL )
return;
    for ( i=0; i<ref->layer_cnt; ++i ) {
	SplinePointListsFree(ref->layers[i].splines);
	ImageListsFree(ref->layers[i].images);
    }
    free(ref->layers);
#else
    if ( ref==NULL )
return;
    SplinePointListsFree(ref->layers[0].splines);
#endif
    chunkfree(ref,sizeof(RefChar));
}

RefChar *RefCharCreate(void) {
    RefChar *ref = chunkalloc(sizeof(RefChar));
#ifdef FONTFORGE_CONFIG_TYPE3
    ref->layers = gcalloc(1,sizeof(struct reflayer));
    ref->layers[0].fill_brush.opacity = ref->layers[0].stroke_pen.brush.opacity = 1.0;
    ref->layers[0].fill_brush.col = ref->layers[0].stroke_pen.brush.col = COLOR_INHERITED;
    ref->layers[0].stroke_pen.width = WIDTH_INHERITED;
    ref->layers[0].stroke_pen.linecap = lc_inherited;
    ref->layers[0].stroke_pen.linejoin = lj_inherited;
    ref->layers[0].dofill = true;
#endif
    ref->layer_cnt = 1;
    ref->round_translation_to_grid = true;
return( ref );
}

void RefCharsFree(RefChar *ref) {
    RefChar *rnext;

    while ( ref!=NULL ) {
	rnext = ref->next;
	RefCharFree(ref);
	ref = rnext;
    }
}

static extended SolveCubic(extended a, extended b, extended c, extended d, double err, extended t0) {
    /* find t between t0 and .5 where at^3+bt^2+ct+d == +/- err */
    extended t, val, offset;
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

static extended SolveCubicBack(extended a, extended b, extended c, extended d, double err, extended t0) {
    /* find t between .5 and t0 where at^3+bt^2+ct+d == +/- err */
    extended t, val, offset;
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
	prev->next = lines->next;
	chunkfree(lines,sizeof(*lines));
	lines = prev->next;
    }

    if ( lines!=NULL ) while ( (next = lines->next)!=NULL ) {
	if ( prev->here.x!=next->here.x ) {
	    double slope = (next->here.y-prev->here.y) / (double) (next->here.x-prev->here.x);
	    double inter = prev->here.y - slope*prev->here.x;
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
    double tx,ty,t;
    double slpx, slpy;
    double intx, inty;

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

static void SplineFindBounds(const Spline *sp, DBounds *bounds) {
    real t, b2_fourac, v;
    real min, max;
    const Spline1D *sp1;
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
	/* (I don't bother fixing up for tiny rounding errors here. they don't matter */
	/* But we could call CheckExtremaForSingleBitErrors */
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

static void _SplineSetFindBounds(const SplinePointList *spl, DBounds *bounds) {
    Spline *spline, *first;
    /* Ignore contours consisting of a single point (used for hinting, anchors */
    /*  for mark to base, etc. */

    for ( ; spl!=NULL; spl = spl->next ) if ( spl->first->next!=NULL && spl->first->next->to != spl->first ) {
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

void SplineSetFindBounds(const SplinePointList *spl, DBounds *bounds) {
    memset(bounds,'\0',sizeof(*bounds));
    _SplineSetFindBounds(spl,bounds);
}

#ifdef FONTFORGE_CONFIG_TYPE3
static void _ImageFindBounds(ImageList *img,DBounds *bounds) {
    if ( bounds->minx==0 && bounds->maxx==0 && bounds->miny==0 && bounds->maxy == 0 )
	*bounds = img->bb;
    else if ( img->bb.minx!=0 || img->bb.maxx != 0 || img->bb.maxy != 0 || img->bb.miny!=0 ) {
	if ( img->bb.minx < bounds->minx ) bounds->minx = img->bb.minx;
	if ( img->bb.miny < bounds->miny ) bounds->miny = img->bb.miny;
	if ( img->bb.maxx > bounds->maxx ) bounds->maxx = img->bb.maxx;
	if ( img->bb.maxy > bounds->maxy ) bounds->maxy = img->bb.maxy;
    }
}
#endif

void SplineCharFindBounds(SplineChar *sc,DBounds *bounds) {
    RefChar *rf;
    int i;
#ifdef FONTFORGE_CONFIG_TYPE3
    ImageList *img;
    real e;
    DBounds b;
#endif

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;
    bounds->miny = bounds->maxy = 0;

    for ( i=ly_fore; i<sc->layer_cnt; ++i ) {
	for ( rf=sc->layers[i].refs; rf!=NULL; rf = rf->next ) {
	    if ( bounds->minx==0 && bounds->maxx==0 && bounds->miny==0 && bounds->maxy == 0 )
		*bounds = rf->bb;
	    else if ( rf->bb.minx!=0 || rf->bb.maxx != 0 || rf->bb.maxy != 0 || rf->bb.miny!=0 ) {
		if ( rf->bb.minx < bounds->minx ) bounds->minx = rf->bb.minx;
		if ( rf->bb.miny < bounds->miny ) bounds->miny = rf->bb.miny;
		if ( rf->bb.maxx > bounds->maxx ) bounds->maxx = rf->bb.maxx;
		if ( rf->bb.maxy > bounds->maxy ) bounds->maxy = rf->bb.maxy;
	    }
	}
#ifdef FONTFORGE_CONFIG_TYPE3
	memset(&b,0,sizeof(b));
	_SplineSetFindBounds(sc->layers[i].splines,&b);
	for ( img=sc->layers[i].images; img!=NULL; img=img->next )
	    _ImageFindBounds(img,bounds);
	if ( sc->layers[i].dostroke ) {
	    if ( sc->layers[i].stroke_pen.width!=WIDTH_INHERITED )
		e = sc->layers[i].stroke_pen.width*sc->layers[i].stroke_pen.trans[0];
	    else
		e = sc->layers[i].stroke_pen.trans[0];
	    b.minx -= e; b.maxx += e;
	    b.miny -= e; b.maxy += e;
	}
    if ( bounds->minx==0 && bounds->maxx==0 && bounds->miny==0 && bounds->maxy == 0 )
	*bounds = b;
    else if ( b.minx!=0 || b.maxx != 0 || b.maxy != 0 || b.miny!=0 ) {
	if ( b.minx < bounds->minx ) bounds->minx = b.minx;
	if ( b.miny < bounds->miny ) bounds->miny = b.miny;
	if ( b.maxx > bounds->maxx ) bounds->maxx = b.maxx;
	if ( b.maxy > bounds->maxy ) bounds->maxy = b.maxy;
    }
#else
	_SplineSetFindBounds(sc->layers[i].splines,bounds);
#endif
    }
    if ( sc->parent!=NULL && sc->parent->strokedfont &&
	    (bounds->minx!=bounds->maxx || bounds->miny!=bounds->maxy)) {
	real sw = sc->parent->strokewidth;
	bounds->minx -= sw; bounds->miny -= sw;
	bounds->maxx += sw; bounds->maxy += sw;
    }
}

void SplineFontFindBounds(SplineFont *sf,DBounds *bounds) {
    RefChar *rf;
    int i, k;
#ifdef FONTFORGE_CONFIG_TYPE3
    real extra=0,e;
    ImageList *img;
#endif

    bounds->minx = bounds->maxx = 0;
    bounds->miny = bounds->maxy = 0;

    for ( i = 0; i<sf->glyphcnt; ++i ) {
	SplineChar *sc = sf->glyphs[i];
	if ( sc!=NULL ) {
	    for ( k=ly_fore; k<sc->layer_cnt; ++k ) {
		for ( rf=sc->layers[k].refs; rf!=NULL; rf = rf->next ) {
		    if ( bounds->minx==0 && bounds->maxx==0 && bounds->miny==0 && bounds->maxy == 0 )
			*bounds = rf->bb;
		    else if ( rf->bb.minx!=0 || rf->bb.maxx != 0 || rf->bb.maxy != 0 || rf->bb.miny!=0 ) {
			if ( rf->bb.minx < bounds->minx ) bounds->minx = rf->bb.minx;
			if ( rf->bb.miny < bounds->miny ) bounds->miny = rf->bb.miny;
			if ( rf->bb.maxx > bounds->maxx ) bounds->maxx = rf->bb.maxx;
			if ( rf->bb.maxy > bounds->maxy ) bounds->maxy = rf->bb.maxy;
		    }
		}

		_SplineSetFindBounds(sc->layers[k].splines,bounds);
#ifdef FONTFORGE_CONFIG_TYPE3
		for ( img=sc->layers[k].images; img!=NULL; img=img->next )
		    _ImageFindBounds(img,bounds);
		if ( sc->layers[k].dostroke ) {
		    if ( sc->layers[k].stroke_pen.width!=WIDTH_INHERITED )
			e = sc->layers[k].stroke_pen.width*sc->layers[k].stroke_pen.trans[0];
		    else
			e = sc->layers[k].stroke_pen.trans[0];
		    if ( e>extra ) extra = e;
		}
#endif
	    }
	}
    }
#ifdef FONTFORGE_CONFIG_TYPE3
    bounds->minx -= extra; bounds->miny -= extra;
    bounds->maxx += extra; bounds->maxy += extra;
#endif
}

void CIDFindBounds(SplineFont *cidmaster,DBounds *bounds) {
    SplineFont *sf;
    int i;
    DBounds b;
    real factor;

    if ( cidmaster->cidmaster )
	cidmaster = cidmaster->cidmaster;
    if ( cidmaster->subfonts==NULL ) {
	SplineFontFindBounds(cidmaster,bounds);
return;
    }

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

static void _SplineSetFindTop(SplineSet *ss,BasePoint *top) {
    SplinePoint *sp;

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
}

#ifndef FONTFORGE_CONFIG_TYPE3
static void SplineSetFindTop(SplineSet *ss,BasePoint *top) {

    top->y = -1e10;
    _SplineSetFindTop(ss,top);
    if ( top->y < -65536 ) top->y = top->x = 0;
}
#endif

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
    int i;
    DBounds temp;
#ifdef FONTFORGE_CONFIG_TYPE3
    real e;
    ImageList *img;
#endif

    memset(b,0,sizeof(*b));
    for ( i=ly_fore; i<sc->layer_cnt; ++i ) {
	SplineSetQuickBounds(sc->layers[i].splines,&temp);
#ifdef FONTFORGE_CONFIG_TYPE3
	for ( img=sc->layers[i].images; img!=NULL; img=img->next )
	    _ImageFindBounds(img,b);
	if ( sc->layers[i].dostroke && sc->layers[i].splines!=NULL ) {
	    if ( sc->layers[i].stroke_pen.width!=WIDTH_INHERITED )
		e = sc->layers[i].stroke_pen.width*sc->layers[i].stroke_pen.trans[0];
	    else
		e = sc->layers[i].stroke_pen.trans[0];
	    temp.minx -= e; temp.maxx += e;
	    temp.miny -= e; temp.maxy += e;
	}
#endif
	if ( temp.minx!=0 || temp.maxx != 0 || temp.maxy != 0 || temp.miny!=0 ) {
	    if ( temp.minx < b->minx ) b->minx = temp.minx;
	    if ( temp.miny < b->miny ) b->miny = temp.miny;
	    if ( temp.maxx > b->maxx ) b->maxx = temp.maxx;
	    if ( temp.maxy > b->maxy ) b->maxy = temp.maxy;
	}
	for ( ref = sc->layers[i].refs; ref!=NULL; ref = ref->next ) {
	    /*SplineSetQuickBounds(ref->layers[0].splines,&temp);*/
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
    if ( sc->parent->strokedfont && (b->minx!=b->maxx || b->miny!=b->maxy)) {
	real sw = sc->parent->strokewidth;
	b->minx -= sw; b->miny -= sw;
	b->maxx += sw; b->maxy += sw;
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
    int i;
    DBounds temp;
#ifdef FONTFORGE_CONFIG_TYPE3
    real e;
    ImageList *img;
#endif

    memset(b,0,sizeof(*b));
    for ( i=ly_fore; i<sc->layer_cnt; ++i ) {
	SplineSetQuickConservativeBounds(sc->layers[i].splines,&temp);
#ifdef FONTFORGE_CONFIG_TYPE3
	for ( img=sc->layers[i].images; img!=NULL; img=img->next )
	    _ImageFindBounds(img,b);
	if ( sc->layers[i].dostroke && sc->layers[i].splines!=NULL ) {
	    if ( sc->layers[i].stroke_pen.width!=WIDTH_INHERITED )
		e = sc->layers[i].stroke_pen.width*sc->layers[i].stroke_pen.trans[0];
	    else
		e = sc->layers[i].stroke_pen.trans[0];
	    temp.minx -= e; temp.maxx += e;
	    temp.miny -= e; temp.maxy += e;
	}
#endif
	if ( temp.minx!=0 || temp.maxx != 0 || temp.maxy != 0 || temp.miny!=0 ) {
	    if ( temp.minx < b->minx ) b->minx = temp.minx;
	    if ( temp.miny < b->miny ) b->miny = temp.miny;
	    if ( temp.maxx > b->maxx ) b->maxx = temp.maxx;
	    if ( temp.maxy > b->maxy ) b->maxy = temp.maxy;
	}
	for ( ref = sc->layers[i].refs; ref!=NULL; ref = ref->next ) {
	    /*SplineSetQuickConservativeBounds(ref->layers[0].splines,&temp);*/
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
    if ( sc->parent->strokedfont && (b->minx!=b->maxx || b->miny!=b->maxy)) {
	real sw = sc->parent->strokewidth;
	b->minx -= sw; b->miny -= sw;
	b->maxx += sw; b->maxy += sw;
    }
}

void SplineFontQuickConservativeBounds(SplineFont *sf,DBounds *b) {
    DBounds bb;
    int i;

    b->minx = b->miny = 1e10;
    b->maxx = b->maxy = -1e10;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineCharQuickConservativeBounds(sf->glyphs[i],&bb);
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
    } else {
	BasePoint ndir, ncdir, pdir, pcdir;
	double nlen, nclen, plen, pclen;
	double slop;
	double minlen;

	ncdir.x = sp->nextcp.x - sp->me.x; ncdir.y = sp->nextcp.y - sp->me.y;
	pcdir.x = sp->prevcp.x - sp->me.x; pcdir.y = sp->prevcp.y - sp->me.y;
	ndir.x = ndir.y = pdir.x = pdir.y = 0;
	if ( sp->next!=NULL ) {
	    ndir.x = sp->next->to->me.x - sp->me.x; ndir.y = sp->next->to->me.y - sp->me.y;
	}
	if ( sp->prev!=NULL ) {
	    pdir.x = sp->prev->from->me.x - sp->me.x; pdir.y = sp->prev->from->me.y - sp->me.y;
	}
	nclen = sqrt(ncdir.x*ncdir.x + ncdir.y*ncdir.y);
	pclen = sqrt(pcdir.x*pcdir.x + pcdir.y*pcdir.y);
	nlen = sqrt(ndir.x*ndir.x + ndir.y*ndir.y);
	plen = sqrt(pdir.x*pdir.x + pdir.y*pdir.y);
	if ( nclen!=0 ) { ncdir.x /= nclen; ncdir.y /= nclen; }
	if ( pclen!=0 ) { pcdir.x /= pclen; pcdir.y /= pclen; }
	if ( nlen!=0 ) { ndir.x /= nlen; ndir.y /= nlen; }
	if ( plen!=0 ) { pdir.x /= plen; pdir.y /= plen; }
	/* as the cp gets closer to the base point (being bound to integer */
	/* coordinates in many cases) we need to be less precise in our defn */
	/* of colinear. */
	if ( pclen>=1 && nclen>=1 )
	    minlen = pclen<nclen ? pclen : nclen;
	else if ( pclen>=1 )
	    minlen = pclen;
	else
	    minlen = nclen;
	if ( minlen<2 )
	    slop = -.95;
	else if ( minlen<5 )
	    slop = -.98;
	else
	    slop = -.99;
	if ( nclen!=0 && pclen!=0 && ncdir.x*pcdir.x+ncdir.y*pcdir.y<slop )
	    sp->pointtype = pt_curve;
	else if (( nclen!=0 || plen!=0 ) &&
		(nclen==0 || ncdir.x*pdir.x+ncdir.y*pdir.y<slop ) &&
		(pclen==0 || ndir.x*pcdir.x+ndir.y*pcdir.y<slop ))
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

void SPLCatagorizePoints(SplinePointList *spl) {
    Spline *spline, *first, *last=NULL;

    for ( ; spl!=NULL; spl = spl->next ) {
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

void SCCatagorizePoints(SplineChar *sc) {
#ifdef FONTFORGE_CONFIG_TYPE3
    int i;
    for ( i=ly_fore; i<sc->layer_cnt; ++i )
	SPLCatagorizePoints(sc->layers[i].splines);
#else
    SPLCatagorizePoints(sc->layers[ly_fore].splines);
#endif
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

    if ( encname==NULL ) encname = ".notdef";	/* In case of an incomplete encoding array */

    for ( k=0; k<chars->cnt; ++k ) {
	if ( chars->keys[k]!=NULL )
	    if ( strcmp(encname,chars->keys[k])==0 )
return( k );
    }
return( -1 );
}

SplinePointList *SplinePointListCopy1(const SplinePointList *spl) {
    SplinePointList *cur;
    const SplinePoint *pt; SplinePoint *cpt;
    Spline *spline;

    cur = chunkalloc(sizeof(SplinePointList));

    for ( pt=spl->first; ;  ) {
	cpt = chunkalloc(sizeof(SplinePoint));
	*cpt = *pt;
	if ( pt->hintmask!=NULL ) {
	    cpt->hintmask = chunkalloc(sizeof(HintMask));
	    memcpy(cpt->hintmask,pt->hintmask,sizeof(HintMask));
	}
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
	pt = pt->next->to;
	if ( pt==spl->first )
    break;
    }
    if ( spl->first->prev!=NULL ) {
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
	    cpt->hintmask = NULL;
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

SplinePointList *SplinePointListCopy(const SplinePointList *base) {
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

ImageList *ImageListCopy(ImageList *cimg) {
    ImageList *head=NULL, *last=NULL, *new;

    for ( ; cimg!=NULL; cimg=cimg->next ) {
	new = chunkalloc(sizeof(ImageList));
	*new = *cimg;
	new->next = NULL;
	if ( last==NULL )
	    head = last = new;
	else {
	    last->next = new;
	    last = new;
	}
    }
return( head );
}

ImageList *ImageListTransform(ImageList *img, real transform[6]) {
    ImageList *head = img;

	/* Don't support rotating, flipping or skewing images */;
    if ( transform[0]!=0 && transform[3]!=0 ) {
	while ( img!=NULL ) {
	    double x = img->xoff;
	    img->xoff = transform[0]*x + transform[2]*img->yoff + transform[4];
	    img->yoff = transform[1]*x + transform[3]*img->yoff + transform[5];
	    if (( img->xscale *= transform[0])<0 ) {
		img->xoff += img->xscale *
		    (img->image->list_len==0?img->image->u.image:img->image->u.images[0])->width;
		img->xscale = -img->xscale;
	    }
	    if (( img->yscale *= transform[3])<0 ) {
		img->yoff += img->yscale *
		    (img->image->list_len==0?img->image->u.image:img->image->u.images[0])->height;
		img->yscale = -img->yscale;
	    }
	    img->bb.minx = img->xoff; img->bb.maxy = img->yoff;
	    img->bb.maxx = img->xoff + GImageGetWidth(img->image)*img->xscale;
	    img->bb.miny = img->yoff - GImageGetHeight(img->image)*img->yscale;
	    img = img->next;
	}
    }
return( head );
}

void ApTransform(AnchorPoint *ap, real transform[6]) {
    BasePoint p;
    p.x = transform[0]*ap->me.x + transform[2]*ap->me.y + transform[4];
    p.y = transform[1]*ap->me.x + transform[3]*ap->me.y + transform[5];
    ap->me.x = rint(1024*p.x)/1024;
    ap->me.y = rint(1024*p.y)/1024;
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

HintMask *HintMaskFromTransformedRef(RefChar *ref,BasePoint *trans,
	SplineChar *basesc,HintMask *hm) {
    StemInfo *st, *st2;
    int hst_cnt, bcnt;
    real start, width;
    int i;

    if ( ref->transform[1]!=0 || ref->transform[2]!=0 )
return(NULL);

    memset(hm,0,sizeof(HintMask));
    for ( st = ref->sc->hstem; st!=NULL; st=st->next ) {
	start = st->start*ref->transform[3] + ref->transform[5] + trans->y;
	width = st->width*ref->transform[3];
	for ( st2=basesc->hstem,bcnt=0; st2!=NULL; st2=st2->next, bcnt++ )
	    if ( st2->start == start && st2->width == width )
	break;
	if ( st2!=NULL )
	    (*hm)[bcnt>>3] |= (0x80>>(bcnt&7));
    }
    for ( st2=basesc->hstem,hst_cnt=0; st2!=NULL; st2=st2->next, hst_cnt++ );

    for ( st = ref->sc->vstem; st!=NULL; st=st->next ) {
	start = st->start*ref->transform[0] + ref->transform[4] + trans->x;
	width = st->width*ref->transform[0];
	for ( st2=basesc->vstem,bcnt=hst_cnt; st2!=NULL; st2=st2->next, bcnt++ )
	    if ( st2->start == start && st2->width == width )
	break;
	if ( st2!=NULL )
	    (*hm)[bcnt>>3] |= (0x80>>(bcnt&7));
    }
    for ( i=0; i<HntMax/8; ++i )
	if ( (*hm)[i]!=0 )
return( hm );

return( NULL );
}

static HintMask *HintMaskTransform(HintMask *oldhm,real transform[6],
	SplineChar *basesc,SplineChar *subsc) {
    HintMask *newhm;
    StemInfo *st, *st2;
    int cnt, hst_cnt, bcnt;
    real start, width;

    if ( transform[1]!=0 || transform[2]!=0 )
return( NULL );

    newhm = chunkalloc(sizeof(HintMask));
    for ( st = subsc->hstem,cnt = 0; st!=NULL; st=st->next, cnt++ ) {
	if ( (*oldhm)[cnt>>3]&(0x80>>(cnt&7)) ) {
	    start = st->start*transform[3] + transform[5];
	    width = st->width*transform[3];
	    for ( st2=basesc->hstem,bcnt=0; st2!=NULL; st2=st2->next, bcnt++ )
		if ( st2->start == start && st2->width == width )
	    break;
	    if ( st2!=NULL )
		(*newhm)[bcnt>>3] |= (0x80>>(bcnt&7));
	}
    }
    for ( st2=basesc->hstem,hst_cnt=0; st2!=NULL; st2=st2->next, hst_cnt++ );

    for ( st = subsc->vstem; st!=NULL; st=st->next, cnt++ ) {
	if ( (*oldhm)[cnt>>3]&(0x80>>(cnt&7)) ) {
	    start = st->start*transform[0] + transform[4];
	    width = st->width*transform[0];
	    for ( st2=basesc->vstem,bcnt=hst_cnt; st2!=NULL; st2=st2->next, bcnt++ )
		if ( st2->start == start && st2->width == width )
	    break;
	    if ( st2!=NULL )
		(*newhm)[bcnt>>3] |= (0x80>>(bcnt&7));
	}
    }
return( newhm );
}

SplinePointList *SPLCopyTranslatedHintMasks(SplinePointList *base,
	SplineChar *basesc, SplineChar *subsc, BasePoint *trans ) {
    SplinePointList *spl, *spl2, *head;
    SplinePoint *spt, *spt2, *pfirst;
    real transform[6];
    Spline *s, *first;

    head = SplinePointListCopy(base);

    transform[0] = transform[3] = 1; transform[1] = transform[2] = 0;
    transform[4] = trans->x; transform[5] = trans->y;

    for ( spl = head, spl2=base; spl!=NULL; spl = spl->next, spl2 = spl2->next ) {
	pfirst = NULL;
	for ( spt = spl->first, spt2 = spl2->first ; spt!=pfirst; spt = spt->next->to, spt2 = spt2->next->to ) {
	    if ( pfirst==NULL ) pfirst = spt;
	    TransformPoint(spt,transform);
	    if ( spt2->hintmask ) {
		chunkfree(spt->hintmask,sizeof(HintMask));
		spt->hintmask = HintMaskTransform(spt2->hintmask,transform,basesc,subsc);
	    }
	    if ( spt->next==NULL )
	break;
	}
	first = NULL;
	for ( s = spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
	    SplineRefigure(s);
	    if ( first==NULL ) first = s;
	}
    }
return( head );
}

static SplinePointList *_SPLCopyTransformedHintMasks(SplineChar *subsc,real transform[6],
	SplineChar *basesc ) {
    SplinePointList *spl, *spl2, *head, *last=NULL, *cur, *base;
    SplinePoint *spt, *spt2, *pfirst;
    Spline *s, *first;
    real trans[6];
    RefChar *rf;

    base = subsc->layers[ly_fore].splines;
    head = SplinePointListCopy(base);
    if ( head!=NULL )
	for ( last = head; last->next!=NULL; last=last->next );

    for ( spl = head, spl2=base; spl!=NULL; spl = spl->next, spl2=spl2->next ) {
	pfirst = NULL;
	for ( spt = spl->first, spt2 = spl2->first ; spt!=pfirst; spt = spt->next->to, spt2 = spt2->next->to ) {
	    if ( pfirst==NULL ) pfirst = spt;
	    TransformPoint(spt,transform);
	    if ( spt2->hintmask ) {
		chunkfree(spt->hintmask,sizeof(HintMask));
		spt->hintmask = HintMaskTransform(spt2->hintmask,transform,basesc,subsc);
	    }
	    if ( spt->next==NULL )
	break;
	}
	first = NULL;
	for ( s = spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
	    SplineRefigure(s);
	    if ( first==NULL ) first = s;
	}
    }
    for ( rf=subsc->layers[ly_fore].refs; rf!=NULL; rf=rf->next ) {
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
	cur = _SPLCopyTransformedHintMasks(rf->sc,trans,basesc);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	if ( cur!=NULL ) {
	    while ( cur->next!=NULL ) cur = cur->next;
	    last = cur;
	}
    }
return( head );
}

SplinePointList *SPLCopyTransformedHintMasks(RefChar *r,
	SplineChar *basesc, BasePoint *trans ) {
    real transform[6];

    memcpy(transform,r->transform,sizeof(transform));
    transform[4] += trans->x; transform[5] += trans->y;
return( _SPLCopyTransformedHintMasks(r->sc,transform,basesc));
}

void SplinePointListSelect(SplinePointList *spl,int sel) {
    Spline *spline, *first;

    for ( ; spl!=NULL; spl = spl->next ) {
	first = NULL;
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
	if ( refs->sc!=NULL )
	    i = refs->sc->orig_pos;		/* Can happen in type3 fonts */
	else for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	    if ( strcmp(sf->glyphs[i]->name,AdobeStandardEncoding[refs->adobe_enc])==0 )
	break;
	if ( i!=sf->glyphcnt && !sf->glyphs[i]->ticked ) {
	    refs->checked = true;
	    refs->sc = rsc = sf->glyphs[i];
	    refs->orig_pos = rsc->orig_pos;
	    refs->unicode_enc = rsc->unicodeenc;
	    SCMakeDependent(dsc,rsc);
	} else {
	    LogError( _("Couldn't find referenced character \"%s\" in %s\n"),
		    AdobeStandardEncoding[refs->adobe_enc], dsc->name);
return;
	}
    } else if ( refs->sc->ticked )
return;

    rsc = refs->sc;
    rsc->ticked = true;

    for ( rf=rsc->layers[ly_fore].refs; rf!=NULL; rf = rf->next ) {
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

#ifdef FONTFORGE_CONFIG_TYPE3
    if ( sf->multilayer ) {
	int lbase = topref->layer_cnt;
	if ( topref->layer_cnt==0 ) {
	    topref->layers = gcalloc(rsc->layer_cnt-1,sizeof(struct reflayer));
	    topref->layer_cnt = rsc->layer_cnt-1;
	} else {
	    topref->layer_cnt += rsc->layer_cnt-1;
	    topref->layers = grealloc(topref->layers,topref->layer_cnt*sizeof(struct reflayer));
	    memset(topref->layers+lbase,0,(rsc->layer_cnt-1)*sizeof(struct reflayer));
	}
	for ( i=ly_fore; i<rsc->layer_cnt; ++i ) {
	    topref->layers[i-ly_fore+lbase].splines = SplinePointListTransform(SplinePointListCopy(rsc->layers[i].splines),transform,true);
	    topref->layers[i-ly_fore+lbase].fill_brush = rsc->layers[i].fill_brush;
	    topref->layers[i-ly_fore+lbase].stroke_pen = rsc->layers[i].stroke_pen;
	    topref->layers[i-ly_fore+lbase].dofill = rsc->layers[i].dofill;
	    topref->layers[i-ly_fore+lbase].dostroke = rsc->layers[i].dostroke;
	    topref->layers[i-ly_fore+lbase].fillfirst = rsc->layers[i].fillfirst;
	    /* ??? and images? Can't read images yet, so not a problem yet */
	}
    } else {
	if ( topref->layer_cnt==0 ) {
	    topref->layers = gcalloc(1,sizeof(struct reflayer));
	    topref->layer_cnt = 1;
	}
#else
    {
#endif
	new = SplinePointListTransform(SplinePointListCopy(rsc->layers[ly_fore].splines),transform,true);
	if ( new!=NULL ) {
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
	    spl->next = topref->layers[0].splines;
	    topref->layers[0].splines = new;
	}
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
    if ( !utf8_valid(ret)) {
	/* Assume latin1, convert to utf8 */
	rpt = latin1_2_utf8_copy(ret);
	free(ret);
	ret = rpt;
    }
return(ret);
}

char *XUIDFromFD(int xuid[20]) {
    int i;
    char *ret=NULL;

    for ( i=19; i>=0 && xuid[i]==0; --i );
    if ( i>=0 ) {
	int j; char *pt;
	ret = galloc(2+20*(i+1));
	pt = ret;
	*pt++ = '[';
	for ( j=0; j<=i; ++j ) {
	    sprintf(pt,"%d ", xuid[j]);
	    pt += strlen(pt);
	}
	pt[-1] = ']';
    }
return( ret );
}

static void SplineFontMetaData(SplineFont *sf,struct fontdict *fd) {
    int em;

    sf->fontname = utf8_verify_copy(fd->cidfontname?fd->cidfontname:fd->fontname);
    sf->display_size = -default_fv_font_size;
    sf->display_antialias = default_fv_antialias;
    if ( fd->fontinfo!=NULL ) {
	if ( sf->fontname==NULL && fd->fontinfo->fullname!=NULL ) {
	    sf->fontname = EnforcePostScriptName(fd->fontinfo->fullname);
	}
	if ( sf->fontname==NULL ) sf->fontname = EnforcePostScriptName(fd->fontinfo->familyname);
	sf->fullname = copyparse(fd->fontinfo->fullname);
	sf->familyname = copyparse(fd->fontinfo->familyname);
	sf->weight = copyparse(fd->fontinfo->weight);
	sf->version = copyparse(fd->fontinfo->version);
	sf->copyright = copyparse(fd->fontinfo->notice);
	sf->italicangle = fd->fontinfo->italicangle;
	sf->upos = fd->fontinfo->underlineposition;
	sf->uwidth = fd->fontinfo->underlinethickness;
	sf->strokedfont = fd->painttype==2;
	sf->strokewidth = fd->strokewidth;
	sf->ascent = fd->fontinfo->ascent;
	sf->descent = fd->fontinfo->descent;
    }
    if ( sf->uniqueid==0 ) sf->uniqueid = fd->uniqueid;
    if ( sf->fontname==NULL ) sf->fontname = GetNextUntitledName();
    if ( sf->fullname==NULL ) sf->fullname = copy(sf->fontname);
    if ( sf->familyname==NULL ) sf->familyname = copy(sf->fontname);
    if ( sf->weight==NULL ) sf->weight = copy("");
    if ( fd->modificationtime!=0 ) {
	sf->modificationtime = fd->modificationtime;
	sf->creationtime = fd->creationtime;
    }
    sf->cidversion = fd->cidversion;
    sf->xuid = XUIDFromFD(fd->xuid);
    /*sf->wasbinary = fd->wasbinary;*/
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
    if ( sf->ordering!=NULL ) {
	if ( strnmatch(sf->ordering,"Japan",5)==0 )
	    sf->uni_interp = ui_japanese;
	else if ( strnmatch(sf->ordering,"Korea",5)==0 )
	    sf->uni_interp = ui_korean;
	else if ( strnmatch(sf->ordering,"CNS",3)==0 )
	    sf->uni_interp = ui_trad_chinese;
	else if ( strnmatch(sf->ordering,"GB",2)==0 )
	    sf->uni_interp = ui_simp_chinese;
    }
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

    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	SplinePointListTransform(sc->layers[ly_fore].splines,trans,true);
	for ( refs=sc->layers[ly_fore].refs; refs!=NULL; refs=refs->next ) {
	    /* Just scale the offsets. we'll do all the base characters */
	    real temp = refs->transform[4]*trans[0] +
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
    for ( i=0; i<sf->glyphcnt; ++i ) if ( (sc=sf->glyphs[i])!=NULL ) {
	for ( refs=sc->layers[ly_fore].refs; refs!=NULL; refs=refs->next )
	    SCReinstanciateRefChar(sc,refs);
    }
}

void SFInstanciateRefs(SplineFont *sf) {
    int i, layer;
    RefChar *refs, *next, *pr;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->ticked = false;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];

	for ( layer=ly_fore; layer<sc->layer_cnt; ++layer ) {
	    for ( pr=NULL, refs = sc->layers[layer].refs; refs!=NULL; refs=next ) {
		next = refs->next;
		sc->ticked = true;
		InstanciateReference(sf, refs, refs, refs->transform,sc);
		if ( refs->sc!=NULL ) {
		    SplineSetFindBounds(refs->layers[0].splines,&refs->bb);
		    sc->ticked = false;
		    pr = refs;
		} else {
		    /* In some mal-formed postscript fonts we can have a reference */
		    /*  to a character that is not actually in the font. I even */
		    /*  generated one by mistake once... */
		    if ( pr==NULL )
			sc->layers[layer].refs = next;
		    else
			pr->next = next;
		    refs->next = NULL;
		    RefCharsFree(refs);
		}
	    }
	}
    }
}

/* Also handles type3s */
static void _SplineFontFromType1(SplineFont *sf, FontDict *fd, struct pscontext *pscontext) {
    int i, j, notdefpos;
    RefChar *refs, *next;
    int istype2 = fd->fonttype==2;		/* Easy enough to deal with even though it will never happen... */
    int istype3 = fd->charprocs->next!=0;
    EncMap *map;

    if ( istype2 )
	fd->private->subrs->bias = fd->private->subrs->cnt<1240 ? 107 :
	    fd->private->subrs->cnt<33900 ? 1131 : 32768;
    sf->glyphmax = sf->glyphcnt = istype3 ? fd->charprocs->next : fd->chars->next;
    if ( sf->map==NULL ) {
	sf->map = map = EncMapNew(256+CharsNotInEncoding(fd),sf->glyphcnt,fd->encoding_name);
    } else
	map = sf->map;
    sf->glyphs = gcalloc(map->backmax,sizeof(SplineChar *));
#ifdef FONTFORGE_CONFIG_TYPE3
    if ( istype3 )	/* We read a type3 */
	sf->multilayer = true;
#endif
    if ( istype3 )
	notdefpos = LookupCharString(".notdef",(struct pschars *) (fd->charprocs));
    else
	notdefpos = LookupCharString(".notdef",fd->chars);
    for ( i=0; i<256; ++i ) {
	int k;
	if ( istype3 ) {
	    k = LookupCharString(fd->encoding[i],(struct pschars *) (fd->charprocs));
	} else {
	    k = LookupCharString(fd->encoding[i],fd->chars);
	}
	if ( k==-1 ) k = notdefpos;
	map->map[i] = k;
	if ( k!=-1 && map->backmap[k]==-1 )
	    map->backmap[k] = i;
    }
    if ( map->enccount>256 ) {
	int k, j;
	for ( k=0; k<fd->chars->cnt; ++k ) {
	    if ( fd->chars->keys[k]!=NULL ) {
		for ( j=0; j<256; ++j )
		    if ( fd->encoding[j]!=NULL &&
			    strcmp(fd->encoding[j],fd->chars->keys[k])==0 )
		break;
		if ( j==256 ) {
		    map->map[i] = k;
		    if ( map->backmap[k]==-1 )
			map->backmap[k] = i;
		    ++i;
		}
	    }
	}
	/* And for type3s */
	for ( k=0; k<fd->charprocs->cnt; ++k ) {
	    if ( fd->charprocs->keys[k]!=NULL ) {
		for ( j=0; j<256; ++j )
		    if ( fd->encoding[j]!=NULL &&
			    strcmp(fd->encoding[j],fd->charprocs->keys[k])==0 )
		break;
		if ( j==256 ) {
		    map->map[i] = k;
		    if ( map->backmap[k]==-1 )
			map->backmap[k] = i;
		    ++i;
		}
	    }
	}
    }
    for ( i=0; i<map->enccount; ++i ) if ( map->map[i]==-1 )
	map->map[i] = notdefpos;

    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( !istype3 )
	    sf->glyphs[i] = PSCharStringToSplines(fd->chars->values[i],fd->chars->lens[i],
		    pscontext,fd->private->subrs,NULL,fd->chars->keys[i]);
	else
	    sf->glyphs[i] = fd->charprocs->values[i];
	if ( sf->glyphs[i]!=NULL ) {
	    sf->glyphs[i]->orig_pos = i;
	    sf->glyphs[i]->vwidth = sf->ascent+sf->descent;
	    sf->glyphs[i]->unicodeenc = UniFromName(sf->glyphs[i]->name,sf->uni_interp,map->enc);
	    sf->glyphs[i]->parent = sf;
	    /* SCLigDefault(sf->glyphs[i]);*/		/* Also reads from AFM file, but it probably doesn't exist */
	}
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_progress_next();
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_next();
#endif
    }
    SFInstanciateRefs(sf);
    if ( fd->metrics!=NULL ) {
	for ( i=0; i<fd->metrics->next; ++i ) {
	    int width = strtol(fd->metrics->values[i],NULL,10);
	    for ( j=sf->glyphcnt-1; j>=0; --j ) {
		if ( sf->glyphs[j]!=NULL && sf->glyphs[j]->name!=NULL &&
			strcmp(fd->metrics->keys[i],sf->glyphs[j]->name)==0 ) {
		    sf->glyphs[j]->width = width;
	    break;
		}
	    }
	}
    }
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	    for ( refs = sf->glyphs[i]->layers[ly_fore].refs; refs!=NULL; refs=next ) {
	next = refs->next;
	if ( refs->adobe_enc==' ' && refs->layers[0].splines==NULL ) {
	    /* When I have a link to a single character I will save out a */
	    /*  seac to that character and a space (since I can only make */
	    /*  real char links), so if we find a space link, get rid of*/
	    /*  it. It's an artifact */
	    SCRefToSplines(sf->glyphs[i],refs);
	}
    }
    /* sometimes (some apple oblique fonts) the fontmatrix is not just a */
    /*  formality, it acutally contains a skew. So be ready */
    if ( fd->fontmatrix[0]!=0 )
	TransByFontMatrix(sf,fd->fontmatrix);
    AltUniFigure(sf,sf->map);
}

static void SplineFontFromType1(SplineFont *sf, FontDict *fd, struct pscontext *pscontext) {
    int i;
    SplineChar *sc;

    _SplineFontFromType1(sf,fd,pscontext);

    /* Clean up the hint masks, We create an initial hintmask whether we need */
    /*  it or not */
    for ( i=0; i<sf->glyphcnt; ++i ) {
	if ( (sc = sf->glyphs[i])!=NULL && !sc->hconflicts && !sc->vconflicts &&
		sc->layers[ly_fore].splines!=NULL ) {
	    chunkfree( sc->layers[ly_fore].splines->first->hintmask,sizeof(HintMask) );
	    sc->layers[ly_fore].splines->first->hintmask = NULL;
	}
    }
}

static SplineFont *SplineFontFromMMType1(SplineFont *sf, FontDict *fd, struct pscontext *pscontext) {
    char *pt, *end, *origweight;
    MMSet *mm;
    int ipos, apos, ppos, item, i;
    real blends[12];	/* At most twelve points/axis in a blenddesignmap */
    real designs[12];
    EncMap *map;

    if ( fd->weightvector==NULL || fd->fontinfo->blenddesignpositions==NULL ||
	    fd->fontinfo->blenddesignmap==NULL || fd->fontinfo->blendaxistypes==NULL ) {
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Multiple Master Font"),_("Bad Multiple Master Font"));
#else
	gwwv_post_error(_("Bad Multiple Master Font"),_("Bad Multiple Master Font"));
#endif
	SplineFontFree(sf);
return( NULL );
    }

    mm = chunkalloc(sizeof(MMSet));

    pt = fd->weightvector;
    while ( *pt==' ' || *pt=='[' ) ++pt;
    while ( *pt!=']' && *pt!='\0' ) {
	pscontext->blend_values[ pscontext->instance_count ] =
		strtod(pt,&end);
	if ( pt==end )
    break;
	++(pscontext->instance_count);
	if ( pscontext->instance_count>=sizeof(pscontext->blend_values)/sizeof(pscontext->blend_values[0])) {
	    LogError( _("Multiple master font with more than 16 instances\n") );
    break;
	}
	for ( pt = end; *pt==' '; ++pt );
    }

    mm->instance_count = pscontext->instance_count;
    mm->instances = galloc(pscontext->instance_count*sizeof(SplineFont *));
    mm->defweights = galloc(mm->instance_count*sizeof(real));
    memcpy(mm->defweights,pscontext->blend_values,mm->instance_count*sizeof(real));
    mm->normal = sf;
    _SplineFontFromType1(mm->normal,fd,pscontext);
    map = sf->map;
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->blended = true;
    sf->mm = mm;

    pt = fd->fontinfo->blendaxistypes;
    while ( *pt==' ' || *pt=='[' ) ++pt;
    while ( *pt!=']' && *pt!='\0' ) {
	if ( *pt=='/' ) ++pt;
	for ( end=pt; *end!=' ' && *end!=']' && *end!='\0'; ++end );
	if ( pt==end )
    break;
	if ( mm->axis_count>=sizeof(mm->axes)/sizeof(mm->axes[0])) {
	    LogError( _("Multiple master font with more than 4 axes\n") );
    break;
	}
	mm->axes[ mm->axis_count++ ] = copyn( pt,end-pt );
	for ( pt = end; *pt==' '; ++pt );
    }

    if ( mm->instance_count < (1<<mm->axis_count) )
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Multiple Master Font"),_("This multiple master font has %1$d instance fonts, but it needs at least %2$d master fonts for %3$d axes. FontForge will not be able to edit this correctly"),mm->instance_count,1<<mm->axis_count,mm->axis_count);
#else
	gwwv_post_error(_("Bad Multiple Master Font"),_("This multiple master font has %1$d instance fonts, but it needs at least %2$d master fonts for %3$d axes. FontForge will not be able to edit this correctly"),mm->instance_count,1<<mm->axis_count,mm->axis_count);
#endif
    else if ( mm->instance_count > (1<<mm->axis_count) )
#if defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_error(_("Bad Multiple Master Font"),_("This multiple master font has %1$d instance fonts, but FontForge can only handle %2$d master fonts for %3$d axes. FontForge will not be able to edit this correctly"),mm->instance_count,1<<mm->axis_count,mm->axis_count);
#else
	gwwv_post_error(_("Bad Multiple Master Font"),_("This multiple master font has %1$d instance fonts, but FontForge can only handle %2$d master fonts for %3$d axes. FontForge will not be able to edit this correctly"),mm->instance_count,1<<mm->axis_count,mm->axis_count);
#endif
    mm->positions = gcalloc(mm->axis_count*mm->instance_count,sizeof(real));
    pt = fd->fontinfo->blenddesignpositions;
    while ( *pt==' ' ) ++pt;
    if ( *pt=='[' ) ++pt;
    ipos = 0;
    while ( *pt!=']' && *pt!='\0' ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt==']' || *pt=='\0' )
    break;
	if ( ipos>=mm->instance_count )
    break;
	if ( *pt=='[' ) {
	    ++pt;
	    apos=0;
	    while ( *pt!=']' && *pt!='\0' ) {
		if ( apos>=mm->axis_count ) {
		    LogError( _("Too many axis positions specified in /BlendDesignPositions.\n") );
	    break;
		}
		mm->positions[ipos*mm->axis_count+apos] =
			strtod(pt,&end);
		if ( pt==end )
	    break;
		++apos;
		for ( pt = end; *pt==' '; ++pt );
	    }
	    if ( *pt==']' ) ++pt;
	    ++ipos;
	} else
	    ++pt;
    }
    
    mm->axismaps = gcalloc(mm->axis_count,sizeof(struct axismap));
    pt = fd->fontinfo->blenddesignmap;
    while ( *pt==' ' ) ++pt;
    if ( *pt=='[' ) ++pt;
    apos = 0;
    while ( *pt!=']' && *pt!='\0' ) {
	while ( *pt==' ' ) ++pt;
	if ( *pt==']' || *pt=='\0' )
    break;
	if ( apos>=mm->axis_count )
    break;
	if ( *pt=='[' ) {
	    ++pt;
	    ppos=0;
	    while ( *pt!=']' && *pt!='\0' ) {
		if ( ppos>=12 ) {
		    LogError( _("Too many mapping data points specified in /BlendDesignMap for axis %s.\n"), mm->axes[apos] );
	    break;
		}
		while ( *pt==' ' ) ++pt;
		if ( *pt=='[' ) {
		    ++pt;
		    designs[ppos] = strtod(pt,&end);
		    blends[ppos] = strtod(end,&end);
		    if ( blends[ppos]<0 || blends[ppos]>1 ) {
			LogError( _("Bad value for blend in /BlendDesignMap for axis %s.\n"), mm->axes[apos] );
			if ( blends[ppos]<0 ) blends[ppos] = 0;
			if ( blends[ppos]>1 ) blends[ppos] = 1;
		    }
		    pt = end;
		    while ( *pt!=']' && *pt!='\0' ) ++pt;
		    ppos ++;
		}
		++pt;
		while ( *pt==' ' ) ++pt;
	    }
	    if ( *pt==']' ) ++pt;
	    if ( ppos<2 )
		LogError( _("Bad few values in /BlendDesignMap for axis %s.\n"), mm->axes[apos] );
	    mm->axismaps[apos].points = ppos;
	    mm->axismaps[apos].blends = galloc(ppos*sizeof(real));
	    mm->axismaps[apos].designs = galloc(ppos*sizeof(real));
	    memcpy(mm->axismaps[apos].blends,blends,ppos*sizeof(real));
	    memcpy(mm->axismaps[apos].designs,designs,ppos*sizeof(real));
	    ++apos;
	} else
	    ++pt;
    }

    mm->cdv = copy(fd->cdv);
    mm->ndv = copy(fd->ndv);

    origweight = fd->fontinfo->weight;

    /* Now figure out the master designs, being careful to interpolate */
    /* BlueValues, ForceBold, UnderlinePosition etc. We need to copy private */
    /* generate a font name */
    for ( ipos = 0; ipos<mm->instance_count; ++ipos ) {
	free(fd->fontname);
	free(fd->fontinfo->fullname);
	fd->fontname = MMMakeMasterFontname(mm,ipos,&fd->fontinfo->fullname);
	fd->fontinfo->weight = MMGuessWeight(mm,ipos,copy(origweight));
	if ( fd->blendfontinfo!=NULL ) {
	    for ( item=0; item<3; ++item ) {
		static char *names[] = { "ItalicAngle", "UnderlinePosition", "UnderlineThickness" };
		pt = PSDictHasEntry(fd->blendfontinfo,names[item]);
		if ( pt!=NULL ) {
		    pt = MMExtractNth(pt,ipos);
		    if ( pt!=NULL ) {
			double val = strtod(pt,NULL);
			free(pt);
			switch ( item ) {
			  case 0: fd->fontinfo->italicangle = val; break;
			  case 1: fd->fontinfo->underlineposition = val; break;
			  case 2: fd->fontinfo->underlinethickness = val; break;
			}
		    }
		}
	    }
	}
	fd->private->private = PSDictCopy(sf->private);
	if ( fd->blendprivate!=NULL ) {
	    static char *arrnames[] = { "BlueValues", "OtherBlues", "FamilyBlues", "FamilyOtherBlues", "StdHW", "StdVW", "StemSnapH", "StemSnapV", NULL };
	    static char *scalarnames[] = { "ForceBold", "BlueFuzz", "BlueScale", "BlueShift", NULL };
	    for ( item=0; scalarnames[item]!=NULL; ++item ) {
		pt = PSDictHasEntry(fd->blendprivate,scalarnames[item]);
		if ( pt!=NULL ) {
		    pt = MMExtractNth(pt,ipos);
		    PSDictChangeEntry(fd->private->private,scalarnames[item],pt);
		    free(pt);
		}
	    }
	    for ( item=0; arrnames[item]!=NULL; ++item ) {
		pt = PSDictHasEntry(fd->blendprivate,arrnames[item]);
		if ( pt!=NULL ) {
		    pt = MMExtractArrayNth(pt,ipos);
		    PSDictChangeEntry(fd->private->private,arrnames[item],pt);
		    free(pt);
		}
	    }
	}
	for ( item=0; item<mm->instance_count; ++item )
	    pscontext->blend_values[item] = 0;
	pscontext->blend_values[ipos] = 1;

	mm->instances[ipos] = SplineFontEmpty();
	SplineFontMetaData(mm->instances[ipos],fd);
	free(fd->fontinfo->weight);
	mm->instances[ipos]->map = map;
	_SplineFontFromType1(mm->instances[ipos],fd,pscontext);
	mm->instances[ipos]->mm = mm;
    }
    fd->fontinfo->weight = origweight;

    /* Clean up hintmasks. We always create a hintmask on the first point */
    /*  only keep them if we actually have conflicts.			  */
    for ( i=0; i<mm->normal->glyphcnt; ++i )
	    if ( mm->normal->glyphs[i]!=NULL &&
		    mm->normal->glyphs[i]->layers[ly_fore].splines != NULL ) {
	for ( item=0; item<mm->instance_count; ++item )
	    if ( mm->instances[item]->glyphs[i]->vconflicts ||
		    mm->instances[item]->glyphs[i]->hconflicts )
	break;
	if ( item==mm->instance_count ) {	/* No conflicts */
	    for ( item=0; item<mm->instance_count; ++item ) {
		chunkfree( mm->instances[item]->glyphs[i]->layers[ly_fore].splines->first->hintmask, sizeof(HintMask) );
		mm->instances[item]->glyphs[i]->layers[ly_fore].splines->first->hintmask = NULL;
	    }
	    chunkfree( mm->normal->glyphs[i]->layers[ly_fore].splines->first->hintmask, sizeof(HintMask) );
	    mm->normal->glyphs[i]->layers[ly_fore].splines->first->hintmask = NULL;
	}
    }

return( sf );
}

static SplineFont *SplineFontFromCIDType1(SplineFont *sf, FontDict *fd,
	struct pscontext *pscontext) {
    int i,j,k, bad, uni;
    SplineChar **chars;
    char buffer[100];
    struct cidmap *map;
    SplineFont *_sf;
    SplineChar *sc;
    EncMap *encmap;

    bad = 0x80000000;
    for ( i=0; i<fd->fdcnt; ++i )
	if ( fd->fds[i]->fonttype!=1 && fd->fds[i]->fonttype!=2 )
	    bad = fd->fds[i]->fonttype;
    if ( bad!=0x80000000 || fd->cidfonttype!=0 ) {
	LogError( _("Could not parse a CID font, %sCIDFontType %d, %sfonttype %d\n"),
		( fd->cidfonttype!=0 ) ? "unexpected " : "",
		( bad!=0x80000000 ) ? "unexpected " : "",
		fd->cidfonttype, bad );
	SplineFontFree(sf);
return( NULL );
    }
    if ( fd->cidstrs==NULL || fd->cidcnt==0 ) {
	LogError( _("CID format doesn't contain what we expected it to.\n") );
	SplineFontFree(sf);
return( NULL );
    }

    encmap = EncMap1to1(fd->cidcnt);

    sf->subfontcnt = fd->fdcnt;
    sf->subfonts = galloc((sf->subfontcnt+1)*sizeof(SplineFont *));
    for ( i=0; i<fd->fdcnt; ++i ) {
	sf->subfonts[i] = SplineFontEmpty();
	SplineFontMetaData(sf->subfonts[i],fd->fds[i]);
	sf->subfonts[i]->cidmaster = sf;
	sf->subfonts[i]->uni_interp = sf->uni_interp;
	sf->subfonts[i]->map = encmap;
	if ( fd->fds[i]->fonttype==2 )
	    fd->fds[i]->private->subrs->bias =
		    fd->fds[i]->private->subrs->cnt<1240 ? 107 :
		    fd->fds[i]->private->subrs->cnt<33900 ? 1131 : 32768;
    }

    map = FindCidMap(sf->cidregistry,sf->ordering,sf->supplement,sf);

    chars = gcalloc(fd->cidcnt,sizeof(SplineChar *));
    for ( i=0; i<fd->cidcnt; ++i ) if ( fd->cidlens[i]>0 ) {
	j = fd->cidfds[i];		/* We get font indexes of 255 for non-existant chars */
	uni = CID2NameUni(map,i,buffer,sizeof(buffer));
	pscontext->is_type2 = fd->fds[j]->fonttype==2;
	chars[i] = PSCharStringToSplines(fd->cidstrs[i],fd->cidlens[i],
		    pscontext,fd->fds[j]->private->subrs,
		    NULL,buffer);
	chars[i]->vwidth = sf->subfonts[j]->ascent+sf->subfonts[j]->descent;
	chars[i]->unicodeenc = uni;
	chars[i]->orig_pos = i;
	/* There better not be any references (seac's) because we have no */
	/*  encoding on which to base any fixups */
	if ( chars[i]->layers[ly_fore].refs!=NULL )
	    IError( "Reference found in CID font. Can't fix it up");
	sf->subfonts[j]->glyphcnt = sf->subfonts[j]->glyphmax = i+1;
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_progress_next();
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_progress_next();
#endif
    }
    for ( i=0; i<fd->fdcnt; ++i )
	sf->subfonts[i]->glyphs = gcalloc(sf->subfonts[i]->glyphcnt,sizeof(SplineChar *));
    for ( i=0; i<fd->cidcnt; ++i ) if ( chars[i]!=NULL ) {
	j = fd->cidfds[i];
	if ( j<sf->subfontcnt ) {
	    sf->subfonts[j]->glyphs[i] = chars[i];
	    chars[i]->parent = sf->subfonts[j];
	}
    }
    free(chars);

    /* Clean up the hint masks, We create an initial hintmask whether we */
    /*  need it or not */
    k=0;
    do {
	_sf = k<sf->subfontcnt?sf->subfonts[k]:sf;
	for ( i=0; i<_sf->glyphcnt; ++i ) {
	    if ( (sc = _sf->glyphs[i])!=NULL && !sc->hconflicts && !sc->vconflicts &&
		    sc->layers[ly_fore].splines!=NULL ) {
		chunkfree( sc->layers[ly_fore].splines->first->hintmask,sizeof(HintMask) );
		sc->layers[ly_fore].splines->first->hintmask = NULL;
	    }
	}
	++k;
    } while ( k<sf->subfontcnt );
return( sf );
}

SplineFont *SplineFontFromPSFont(FontDict *fd) {
    SplineFont *sf;
    struct pscontext pscontext;

    if ( fd->sf!=NULL )
	sf = fd->sf;
    else {
	memset(&pscontext,0,sizeof(pscontext));
	pscontext.is_type2 = fd->fonttype==2;
	pscontext.painttype = fd->painttype;

	sf = SplineFontEmpty();
	SplineFontMetaData(sf,fd);
	if ( fd->wascff ) {
	    SplineFontFree(sf);
	    sf = fd->sf;
	} else if ( fd->fdcnt!=0 )
	    sf = SplineFontFromCIDType1(sf,fd,&pscontext);
	else if ( fd->weightvector!=NULL )
	    SplineFontFromMMType1(sf,fd,&pscontext);
	else
	    SplineFontFromType1(sf,fd,&pscontext);
	if ( loaded_fonts_same_as_new && new_fonts_are_order2 &&
		fd->weightvector==NULL )
	    SFConvertToOrder2(sf);
    }
    if ( sf->multilayer )
	SFCheckPSBitmap(sf);
return( sf );
}

#ifdef FONTFORGE_CONFIG_TYPE3
static void LayerToRefLayer(struct reflayer *rl,Layer *layer) {
    rl->fill_brush = layer->fill_brush;
    rl->stroke_pen = layer->stroke_pen;
    rl->dofill = layer->dofill;
    rl->dostroke = layer->dostroke;
    rl->fillfirst = layer->fillfirst;
}
#endif

void RefCharFindBounds(RefChar *rf) {
#ifdef FONTFORGE_CONFIG_TYPE3
    int i;
    SplineChar *rsc = rf->sc;
    real extra=0,e;

    memset(&rf->bb,'\0',sizeof(rf->bb));
    rf->top.y = -1e10;
    for ( i=0; i<rf->layer_cnt; ++i ) {
	_SplineSetFindBounds(rf->layers[i].splines,&rf->bb);
	_SplineSetFindTop(rf->layers[i].splines,&rf->top);
	if ( rsc->layers[i].dostroke ) {
	    if ( rf->layers[i].stroke_pen.width!=WIDTH_INHERITED )
		e = rf->layers[i].stroke_pen.width*rf->layers[i].stroke_pen.trans[0];
	    else
		e = rf->layers[i].stroke_pen.trans[0];
	    if ( e>extra ) extra = e;
	}
    }
    if ( rf->top.y < -65536 ) rf->top.y = rf->top.x = 0;
    rf->bb.minx -= extra; rf->bb.miny -= extra;
    rf->bb.maxx += extra; rf->bb.maxy += extra;
#else
    SplineSetFindBounds(rf->layers[0].splines,&rf->bb);
    SplineSetFindTop(rf->layers[0].splines,&rf->top);
#endif
}

void SCReinstanciateRefChar(SplineChar *sc,RefChar *rf) {
    SplinePointList *spl, *new;
    RefChar *refs;
#ifdef FONTFORGE_CONFIG_TYPE3
    int i,j;
    SplineChar *rsc = rf->sc;
    real extra=0,e;

    for ( i=0; i<rf->layer_cnt; ++i )
	SplinePointListsFree(rf->layers[i].splines);
    free( rf->layers );
    rf->layers = NULL;
    rf->layer_cnt = 0;
    if ( rsc==NULL )
return;
    /* Can be called before sc->parent is set, but only when reading a ttf */
    /*  file which won't be multilayer */
    if ( sc->parent!=NULL && sc->parent->multilayer ) {
	int cnt = 0;
	RefChar *subref;
	for ( i=ly_fore; i<rsc->layer_cnt; ++i ) {
	    if ( rsc->layers[i].splines!=NULL || rsc->layers[i].images!=NULL )
		++cnt;
	    for ( subref=rsc->layers[i].refs; subref!=NULL; subref=subref->next )
		cnt += subref->layer_cnt;
	}
    
	rf->layer_cnt = cnt;
	rf->layers = gcalloc(cnt,sizeof(struct reflayer));
	cnt = 0;
	for ( i=ly_fore; i<rsc->layer_cnt; ++i ) {
	    if ( rsc->layers[i].splines!=NULL || rsc->layers[i].images!=NULL ) {
		rf->layers[cnt].splines =
			SplinePointListTransform(
			 SplinePointListCopy(rsc->layers[i].splines),rf->transform,true);
		rf->layers[cnt].images =
			ImageListTransform(
			 ImageListCopy(rsc->layers[i].images),rf->transform);
		LayerToRefLayer(&rf->layers[cnt],&rsc->layers[i]);
		++cnt;
	    }
	    for ( subref=rsc->layers[i].refs; subref!=NULL; subref=subref->next ) {
		for ( j=0; j<subref->layer_cnt; ++j ) if ( subref->layers[j].images!=NULL || subref->layers[j].splines!=NULL ) {
		    rf->layers[cnt] = subref->layers[j];
		    rf->layers[cnt].splines =
			    SplinePointListTransform(
			     SplinePointListCopy(subref->layers[j].splines),rf->transform,true);
		    rf->layers[cnt].images =
			    ImageListTransform(
			     ImageListCopy(subref->layers[j].images),rf->transform);
		    ++cnt;
		}
	    }
	}

	memset(&rf->bb,'\0',sizeof(rf->bb));
	rf->top.y = -1e10;
	for ( i=0; i<rf->layer_cnt; ++i ) {
	    _SplineSetFindBounds(rf->layers[i].splines,&rf->bb);
	    _SplineSetFindTop(rf->layers[i].splines,&rf->top);
	    if ( rsc->layers[i].dostroke ) {
		if ( rf->layers[i].stroke_pen.width!=WIDTH_INHERITED )
		    e = rf->layers[i].stroke_pen.width*rf->layers[i].stroke_pen.trans[0];
		else
		    e = rf->layers[i].stroke_pen.trans[0];
		if ( e>extra ) extra = e;
	    }
	}
	if ( rf->top.y < -65536 ) rf->top.y = rf->top.x = 0;
	rf->bb.minx -= extra; rf->bb.miny -= extra;
	rf->bb.maxx += extra; rf->bb.maxy += extra;
    } else {
	rf->layers = gcalloc(1,sizeof(struct reflayer));
	rf->layer_cnt = 1;
	rf->layers[0].dofill = true;
#else
    SplinePointListsFree(rf->layers[0].splines);
    rf->layers[0].splines = NULL;
    if ( rf->sc==NULL )
return;
    {
#endif
	new = SplinePointListTransform(SplinePointListCopy(rf->sc->layers[ly_fore].splines),rf->transform,true);
	if ( new!=NULL ) {
	    for ( spl = new; spl->next!=NULL; spl = spl->next );
	    spl->next = rf->layers[0].splines;
	    rf->layers[0].splines = new;
	}
	for ( refs = rf->sc->layers[ly_fore].refs; refs!=NULL; refs = refs->next ) {
	    new = SplinePointListTransform(SplinePointListCopy(refs->layers[0].splines),rf->transform,true);
	    if ( new!=NULL ) {
		for ( spl = new; spl->next!=NULL; spl = spl->next );
		spl->next = rf->layers[0].splines;
		rf->layers[0].splines = new;
	    }
	}
    }
    RefCharFindBounds(rf);
}

static void _SFReinstanciateRefs(SplineFont *sf) {
    int i, undone, undoable, j, cnt;
    RefChar *ref;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->ticked = false;

    undone = true;
    cnt = 0;
    while ( undone && cnt<200) {
	undone = false;
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	    undoable = false;
	    for ( j=0; j<sf->glyphs[i]->layer_cnt; ++j ) {
		for ( ref=sf->glyphs[i]->layers[j].refs; ref!=NULL; ref=ref->next ) {
		    if ( !ref->sc->ticked )
			undoable = true;
		}
	    }
	    if ( undoable )
		undone = true;
	    else {
		for ( j=0; j<sf->glyphs[i]->layer_cnt; ++j ) {
		    for ( ref=sf->glyphs[i]->layers[j].refs; ref!=NULL; ref=ref->next )
			SCReinstanciateRefChar(sf->glyphs[i],ref);
		}
		sf->glyphs[i]->ticked = true;
	    }
	}
	++cnt;
    }
}
	    
void SFReinstanciateRefs(SplineFont *sf) {
    int i;

    if ( sf->cidmaster!=NULL || sf->subfontcnt!=0 ) {
	if ( sf->cidmaster!=NULL ) sf = sf->cidmaster;
	for ( i=0; i<sf->subfontcnt; ++i )
	    _SFReinstanciateRefs(sf->subfonts[i]);
    } else
	_SFReinstanciateRefs(sf);
}

void SCReinstanciateRef(SplineChar *sc,SplineChar *rsc) {
    RefChar *rf;

    for ( rf=sc->layers[ly_fore].refs; rf!=NULL; rf=rf->next ) if ( rf->sc==rsc ) {
	SCReinstanciateRefChar(sc,rf);
    }
}

void SCRemoveDependent(SplineChar *dependent,RefChar *rf) {
    struct splinecharlist *dlist, *pd;
    RefChar *prev;

    if ( dependent->layers[ly_fore].refs==rf )
	dependent->layers[ly_fore].refs = rf->next;
    else {
	for ( prev = dependent->layers[ly_fore].refs; prev->next!=rf; prev=prev->next );
	prev->next = rf->next;
    }
    /* Check for multiple dependencies (colon has two refs to period) */
    /*  if there are none, then remove dependent from ref->sc's dependents list */
    for ( prev = dependent->layers[ly_fore].refs; prev!=NULL && (prev==rf || prev->sc!=rf->sc); prev = prev->next );
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

void SCRemoveLayerDependents(SplineChar *dependent,int layer) {
    RefChar *rf, *next;

    for ( rf=dependent->layers[layer].refs; rf!=NULL; rf=next ) {
	next = rf->next;
	SCRemoveDependent(dependent,rf);
    }
    dependent->layers[layer].refs = NULL;
}

void SCRemoveDependents(SplineChar *dependent) {
    int layer;

    for ( layer=ly_fore; layer<dependent->layer_cnt; ++layer )
	SCRemoveLayerDependents(dependent,layer);
}

void SCRefToSplines(SplineChar *sc,RefChar *rf) {
    SplineSet *spl;
#ifdef FONTFORGE_CONFIG_TYPE3
    int layer;

    if ( sc->parent->multilayer ) {
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	Layer *old = sc->layers;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
	sc->layers = grealloc(sc->layers,(sc->layer_cnt+rf->layer_cnt)*sizeof(Layer));
	for ( layer = 0; layer<rf->layer_cnt; ++layer ) {
	    LayerDefault(&sc->layers[sc->layer_cnt+layer]);
	    sc->layers[sc->layer_cnt+layer].splines = rf->layers[layer].splines;
	    rf->layers[layer].splines = NULL;
	    sc->layers[sc->layer_cnt+layer].images = rf->layers[layer].images;
	    rf->layers[layer].images = NULL;
	    sc->layers[sc->layer_cnt+layer].refs = NULL;
	    sc->layers[sc->layer_cnt+layer].undoes = NULL;
	    sc->layers[sc->layer_cnt+layer].redoes = NULL;
	    sc->layers[sc->layer_cnt+layer].fill_brush = rf->layers[layer].fill_brush;
	    sc->layers[sc->layer_cnt+layer].stroke_pen = rf->layers[layer].stroke_pen;
	    sc->layers[sc->layer_cnt+layer].dofill = rf->layers[layer].dofill;
	    sc->layers[sc->layer_cnt+layer].dostroke = rf->layers[layer].dostroke;
	    sc->layers[sc->layer_cnt+layer].fillfirst = rf->layers[layer].fillfirst;
	}
	sc->layer_cnt += rf->layer_cnt;
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
	SCMoreLayers(sc,old);
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    } else {
#else
    {
#endif
	if ( (spl = rf->layers[0].splines)!=NULL ) {
	    while ( spl->next!=NULL )
		spl = spl->next;
	    spl->next = sc->layers[ly_fore].splines;
	    sc->layers[ly_fore].splines = rf->layers[0].splines;
	    rf->layers[0].splines = NULL;
	}
    }
    SCRemoveDependent(sc,rf);
}

/* This returns all real solutions, even those out of bounds */
/* I use -999999 as an error flag, since we're really only interested in */
/*  solns near 0 and 1 that should be ok. -1 is perhaps a little too close */
static int _CubicSolve(const Spline1D *sp,extended ts[3]) {
    extended d, xN, yN, delta2, temp, delta, h, t2, t3, theta;
    int i=0;

    ts[0] = ts[1] = ts[2] = -999999;
    if ( sp->d==0 && sp->a!=0 ) {
	/* one of the roots is 0, the other two are the soln of a quadratic */
	ts[0] = 0;
	if ( sp->c==0 ) {
	    ts[1] = -sp->b/(extended) sp->a;	/* two zero roots */
	} else {
	    temp = sp->b*(extended) sp->b-4*(extended) sp->a*sp->c;
	    if ( RealNear(temp,0))
		ts[1] = -sp->b/(2*(extended) sp->a);
	    else if ( temp>=0 ) {
		temp = sqrt(temp);
		ts[1] = (-sp->b+temp)/(2*(extended) sp->a);
		ts[2] = (-sp->b-temp)/(2*(extended) sp->a);
	    }
	}
    } else if ( sp->a!=0 ) {
    /* http://www.m-a.org.uk/eb/mg/mg077ch.pdf */
    /* this nifty solution to the cubic neatly avoids complex arithmatic */
	xN = -sp->b/(3*(extended) sp->a);
	yN = ((sp->a*xN + sp->b)*xN+sp->c)*xN + sp->d;

	delta2 = (sp->b*(extended) sp->b-3*(extended) sp->a*sp->c)/(9*(extended) sp->a*sp->a);
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
	extended d = sp->c*(extended) sp->c-4*(extended) sp->b*sp->d;
	if ( RealNear(d,0)) d=0;
	if ( d<0 )
return(false);		/* All roots imaginary */
	d = sqrt(d);
	ts[0] = (-sp->c-d)/(2*(extended) sp->b);
	ts[1] = (-sp->c+d)/(2*(extended) sp->b);
    } else if ( sp->c!=0 ) {
	ts[0] = -sp->d/(extended) sp->c;
    } else {
	/* If it's a point then either everything is a solution, or nothing */
    }
return( ts[0]!=-999999 );
}

int CubicSolve(const Spline1D *sp,extended ts[3]) {
    extended t;
    int i;
    /* This routine gives us all solutions between [0,1] with -1 as an error flag */
    /* http://mathforum.org/dr.math/faq/faq.cubic.equations.html */

    if ( !_CubicSolve(sp,ts)) {
	ts[0] = ts[1] = ts[2] = -1;
return( false );
    }

    for ( i=0; i<3; ++i )
	if ( ts[i]==-999999 ) ts[i] = -1;
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

#if 0
/* These methods purport to solve all quartics. But they don't solve mine */
/*  so I may have miscopied or something, but I'll give up on algebraic solns*/
static int QuarticAlternate(Quartic *q,double ts[4],double offset) {
    Spline1D sp;
    double zs[3], h, j, temp;
    int i;

    sp.a = 1; sp.b = 2*q->c; sp.c = q->c*q->c-4*q->e;
     sp.d = q->d*q->d;
    if ( !_CubicSolve(&sp,zs))
return(-1);
    for ( i=0; i<3; ++i )
	if ( zs[i]>0 )
    break;
    if ( i>=3 )
return( -1 );
    h = sqrt(zs[i]);
    j = (q->c+zs[i]-q->d/h)/2;

    /* Now the roots of q are the roots of y^2+hy+j and y^2-hy+(q->e)/j */
    i = 0;
    temp = h*h-4*j;
    if ( temp>=0 ) {
	temp = sqrt(temp);
	ts[i++] = (-h+temp)/2 - offset;
	ts[i++] = (-h-temp)/2 - offset;
    }
    temp = h*h-4*q->e/j;
    if ( temp>=0 ) {
	temp = sqrt(temp);
	ts[i++] = (h+temp)/2 - offset;
	ts[i++] = (h-temp)/2 - offset;
    }
return( i );
}

static int _QuarticSolve(Quartic *q,double ts[4]) {
    Quartic work;
    double zs[3], pq[2], r;
    double f,offset;
    Spline1D sp;
    int i,j;

    ts[0] = ts[1] = ts[2] = ts[3] = -1;
    if ( RealNear(q->a,0) ) {
	/* I got a quadratic here once so this can happen ... */
	i = 0;
	if ( RealNear(q->b,0)) {
	    if ( RealNear(q->c,0)) {
		if ( !RealNear(q->d,0))
		    ts[i++] = -q->e/q->d;
	    } else {
		f = q->d*q->d - 4*q->c*q->e;
		if ( f>=0 ) {
		    f=sqrt(f);
		    ts[i++] = (-q->d + f)/(2*q->c);
		    ts[i++] = (-q->d - f)/(2*q->c);
		}
	    }
	} else {
	    sp.a = q->b; sp.b = q->c; sp.c = q->d; sp.d = q->e;
	    ts[3] = -1;
	    if ( !CubicSolve(&sp,ts))
return( -1 );
	    for ( i=0; i<3 && ts[i]!=-1; ++i );
	}
    } else if ( RealNear(q->e,0)) {
	sp.a = q->a; sp.b = q->b; sp.c = q->c; sp.d = q->d;
	CubicSolve(&sp,ts);
	for ( i=0; i<3 && ts[i]!=-1; ++i );
	ts[i++] = 0;
    } else {
	/* http://mathforum.org/dr.math/faq/faq.cubic.equations.html */
	/* divide by q->a, substitute t=s - q->b/4 */
	offset = q->b/(4*q->a);

	work.a = 1;
	work.b = 0;
	work.c = (q->c-3*q->b*q->b/(8*q->a))/q->a;
	work.d = (q->b*q->b*q->b/(8*q->a*q->a) - q->b*q->c/(2*q->a) + q->d)/q->a;
	work.e = (-3*q->b*q->b*q->b*q->b/(256*q->a*q->a*q->a) + (q->b*q->b*q->c)/(16*q->a*q->a) -
		q->b*q->d/(4*q->a) + q->e)/q->a;

	if ( RealNear(work.e,0)) {
	    sp.a = work.a; sp.b = work.b; sp.c = work.c; sp.d = work.d;
	    CubicSolve(&sp,ts);
	    for ( i=0; i<3 && ts[i]!=-1; ++i );
	    ts[i++] = 0;
	    for ( j=0; j<i; ++j )
		ts[j] -= offset;
	} else if ( RealNear(work.d,0)) {
	    /* now we have a quadratic in s^2 */
	    double b24ac = work.c*work.c - 4*work.e, t1, t2;
	    if ( b24ac<0 && b24ac>-.0001 ) b24ac = 0;
	    if ( b24ac < 0 )	/* All roots imaginary */
return( -1 );
	    b24ac = sqrt(b24ac);
	    t1 = (-work.c + b24ac)/2;
	    t2 = (-work.c - b24ac)/2;
	    i = 0;
	    if ( t1>0 ) {
		ts[0] = sqrt(t1);
		ts[1] = -ts[0];
		i=2;
	    } else if ( t1>-.0001 )
		ts[i++] = 0;
	    if ( t2>0 ) {
		ts[i++] = sqrt(t2);
		ts[i] = -ts[i-1];
		++i;
	    } else if ( t2>-.0001 )
		ts[i++] = 0;
	    for ( j=0; j<i ; ++j )
		ts[j] -= offset;
	    if ( i==0 )
return( -1 );
	} else {
	    sp.a = 1;
	    sp.b = work.c/2;
	    sp.c = (work.c*work.c-4*work.e)/16;
	    sp.d = work.d*work.d/64;
	    if ( !_CubicSolve(&sp,zs) )
return( QuarticAlternate(&work,ts,offset));

	    j = 0;
	    for ( i=0; zs[i]!=-999999; ++i )
		if ( zs[i]>0 ) {
		    pq[j++] = zs[i];
		    if ( j==2 )
	    break;
		}
	    if ( j!=2 )
return( QuarticAlternate(&work,ts,offset));
	    pq[0] = sqrt(pq[0]);
	    pq[1] = sqrt(pq[1]);
	    r = -work.d/(8 * pq[0] *pq[1]);
	    ts[0] = pq[0]+pq[1]+r - offset;
	    ts[1] = pq[0]-pq[1]-r - offset;
	    ts[2] = -pq[0]+pq[1]-r - offset;
	    ts[3] = -pq[0]-pq[1]+r - offset;
	    i = 4;
	}
    }
return( i );
}
#else
static int _QuarticSolve(Quartic *q,extended ts[4]) {
    extended extrema[5];
    Spline1D sp;
    int ecnt = 0, i, zcnt;

    /* Two special cases */
    if ( q->a==0 ) {	/* It's really a cubic */
	sp.a = q->b;
	sp.b = q->c;
	sp.c = q->d;
	sp.d = q->e;
	ts[4] = -999999;
return( _CubicSolve(&sp,ts));
    } else if ( q->e==0 ) {	/* we can factor out a zero root */
	sp.a = q->a;
	sp.b = q->b;
	sp.c = q->c;
	sp.d = q->d;
	ts[0] = 0;
return( _CubicSolve(&sp,ts+1)+1);
    }

    sp.a = 4*q->a;
    sp.b = 3*q->b;
    sp.c = 2*q->c;
    sp.d = q->d;
    if ( _CubicSolve(&sp,extrema)) {
	ecnt = 1;
	if ( extrema[1]!=-999999 ) {
	    ecnt = 2;
	    if ( extrema[1]<extrema[0] ) {
		extended temp = extrema[1]; extrema[1] = extrema[0]; extrema[0]=temp;
	    }
	    if ( extrema[2]!=-999999 ) {
		ecnt = 3;
		if ( extrema[2]<extrema[0] ) {
		    extended temp = extrema[2]; extrema[2] = extrema[0]; extrema[0]=temp;
		}
		if ( extrema[2]<extrema[1] ) {
		    extended temp = extrema[2]; extrema[2] = extrema[1]; extrema[1]=temp;
		}
	    }
	}
    }
    for ( i=ecnt-1; i>=0 ; --i )
	extrema[i+1] = extrema[i];
    /* Upper and lower bounds within which we'll search */
    extrema[0] = -999;
    extrema[ecnt+1] = 999;
    ecnt += 2;
    /* divide into monotonic sections & use binary search to find zeroes */
    for ( i=zcnt=0; i<ecnt-1; ++i ) {
	extended top, bottom, val;
	extended topt, bottomt, t;
	topt = extrema[i+1];
	bottomt = extrema[i];
	top = (((q->a*topt+q->b)*topt+q->c)*topt+q->d)*topt+q->e;
	bottom = (((q->a*bottomt+q->b)*bottomt+q->c)*bottomt+q->d)*bottomt+q->e;
	if ( top<bottom ) {
	    extended temp = top; top = bottom; bottom = temp;
	    temp = topt; topt = bottomt; bottomt = temp;
	}
	if ( bottom>.001 )	/* this monotonic is all above 0 */
    continue;
	if ( top<-.001 )	/* this monotonic is all below 0 */
    continue;
	if ( bottom>0 ) {
	    ts[zcnt++] = bottomt;
    continue;
	}
	if ( top<0 ) {
	    ts[zcnt++] = topt;
    continue;
	}
	forever {
	    t = (topt+bottomt)/2;
	    if ( t==topt || t==bottomt ) {
		ts[zcnt++] = t;
	break;
	    }

	    val = (((q->a*t+q->b)*t+q->c)*t+q->d)*t+q->e;
	    if ( val>-.0001 && val<.0001 ) {
		ts[zcnt++] = t;
	break;
	    } else if ( val>0 ) {
		top = val;
		topt = t;
	    } else {
		bottom = val;
		bottomt = t;
	    }
	}
    }
    for ( i=zcnt; i<4; ++i )
	ts[i] = -999999;
return( zcnt );
}
#endif

static int QuarticSolve(Quartic *q,extended ts[4]) {
    int i,j,k;

    if ( _QuarticSolve(q,ts)==-1 )
return( -1 );

    for ( i=0; i<4 && ts[i]!=-999999; ++i ) {
	if ( ts[i]>-.0001 && ts[i]<0 ) ts[i] = 0;
	if ( ts[i]>1.0 && ts[i]<1.0001 ) ts[i] = 1;
	if ( ts[i]>1.0 || ts[i]<0 ) ts[i] = -999999;
    }
    for ( i=3; i>=0 && ts[i]==-999999; --i );
    if ( i==-1 )
return( -1 );
    for ( j=i ; j>=0 ; --j ) {
	if ( ts[j]<0 ) {
	    for ( k=j+1; k<=i; ++k )
		ts[k-1] = ts[k];
	    ts[i--] = -999999;
	}
    }
return(i+1);
}

extended SplineSolve(const Spline1D *sp, real tmin, real tmax, extended sought,real err) {
    /* We want to find t so that spline(t) = sought */
    /*  the curve must be monotonic */
    /* returns t which is near sought or -1 */
    Spline1D temp;
    extended ts[3];
    int i;
    extended t;

    temp = *sp;
    temp.d -= sought;
    CubicSolve(&temp,ts);
    if ( tmax<tmin ) { t = tmax; tmax = tmin; tmin = t; }
    for ( i=0; i<3; ++i )
	if ( ts[i]>=tmin && ts[i]<=tmax )
return( ts[i] );

return( -1 );
}

#ifndef EXTENDED_IS_LONG_DOUBLE
double CheckExtremaForSingleBitErrors(const Spline1D *sp, double t) {
    union { double dval; int32 ival[2]; } u1, um1, temp;
    double slope, slope1, slopem1;
#ifdef WORDS_BIGENDIAN
    const int index = 1;
#else
    const int index = 0;
#endif

    slope = (3*(double) sp->a*t+2*sp->b)*t+sp->c;

    u1.dval = t;
    u1.ival[index] += 1;
    slope1 = (3*(double) sp->a*u1.dval+2*sp->b)*u1.dval+sp->c;

    um1.dval = t;
    um1.ival[index] -= 1;
    slopem1 = (3*(double) sp->a*um1.dval+2*sp->b)*um1.dval+sp->c;

    if ( slope<0 ) slope = -slope;
    if ( slope1<0 ) slope1 = -slope1;
    if ( slopem1<0 ) slopem1 = -slopem1;

    if ( slope1<slope && slope1<=slopem1 ) {
	 /* Ok, things got better when we added 1. */
	 /*  Do they improve further if we add 1 more? */
	temp = u1;
	temp.ival[index] += 1;
	slope = (3*(double) sp->a*temp.dval+2*sp->b)*temp.dval+sp->c;
	if ( slope<0 ) slope = -slope;
	if ( slope<slope1 )
return( temp.dval );
	else
return( u1.dval );
    } else if ( slopem1<slope && slopem1<=slope ) {
	 /* Ok, things got better when we subtracted 1. */
	 /*  Do they improve further if we subtract 1 more? */
	temp = um1;
	temp.ival[index] -= 1;
	slope = (3*(double) sp->a*temp.dval+2*sp->b)*temp.dval+sp->c;
	if ( slope<0 ) slope = -slope;
	if ( slope<slopem1 )
return( temp.dval );
	else
return( um1.dval );
    }
    /* that seems as good as it gets */

return( t );
}
#else
extended esqrt(extended e) {
    extended rt, temp;

    rt = sqrt( (double) e );
    if ( e<=0 )
return( rt );

    temp = e/rt;
    rt = (rt+temp)/2;
    temp = e/rt;
    rt = (rt+temp)/2;
return( rt );
}
#endif

static void _SplineFindExtrema(const Spline1D *sp, extended *_t1, extended *_t2 ) {
    extended t1= -1, t2= -1;
    extended b2_fourac;

    /* Find the extreme points on the curve */
    /*  Set to -1 if there are none or if they are outside the range [0,1] */
    /*  Order them so that t1<t2 */
    /*  If only one valid extremum it will be t1 */
    /*  (Does not check the end points unless they have derivative==0) */
    /*  (Does not check to see if d/dt==0 points are inflection points (rather than extrema) */
    if ( sp->a!=0 ) {
	/* cubic, possibly 2 extrema (possibly none) */
	b2_fourac = 4*(extended)sp->b*sp->b - 12*(extended)sp->a*sp->c;
	if ( b2_fourac>=0 ) {
	    b2_fourac = esqrt(b2_fourac);
	    t1 = (-2*sp->b - b2_fourac) / (6*sp->a);
	    t2 = (-2*sp->b + b2_fourac) / (6*sp->a);
	    t1 = CheckExtremaForSingleBitErrors(sp,t1);
	    t2 = CheckExtremaForSingleBitErrors(sp,t2);
	    if ( t1>t2 ) { extended temp = t1; t1 = t2; t2 = temp; }
	    else if ( t1==t2 ) t2 = -1;
	    if ( RealNear(t1,0)) t1=0; else if ( RealNear(t1,1)) t1=1;
	    if ( RealNear(t2,0)) t2=0; else if ( RealNear(t2,1)) t2=1;
	}
    } else if ( sp->b!=0 ) {
	/* Quadratic, at most one extremum */
	t1 = -sp->c/(2.0*(extended) sp->b);
    } else /*if ( sp->c!=0 )*/ {
	/* linear, no extrema */
    }
    *_t1 = t1; *_t2 = t2;
}

void SplineFindExtrema(const Spline1D *sp, extended *_t1, extended *_t2 ) {
    extended t1= -1, t2= -1;
    extended b2_fourac;

    /* Find the extreme points on the curve */
    /*  Set to -1 if there are none or if they are outside the range [0,1] */
    /*  Order them so that t1<t2 */
    /*  If only one valid extremum it will be t1 */
    /*  (Does not check the end points unless they have derivative==0) */
    /*  (Does not check to see if d/dt==0 points are inflection points (rather than extrema) */
    if ( sp->a!=0 ) {
	/* cubic, possibly 2 extrema (possibly none) */
	b2_fourac = 4*(extended) sp->b*sp->b - 12*(extended) sp->a*sp->c;
	if ( b2_fourac>=0 ) {
	    b2_fourac = esqrt(b2_fourac);
	    t1 = (-2*sp->b - b2_fourac) / (6*sp->a);
	    t2 = (-2*sp->b + b2_fourac) / (6*sp->a);
	    t1 = CheckExtremaForSingleBitErrors(sp,t1);
	    t2 = CheckExtremaForSingleBitErrors(sp,t2);
	    if ( t1>t2 ) { extended temp = t1; t1 = t2; t2 = temp; }
	    else if ( t1==t2 ) t2 = -1;
	    if ( RealNear(t1,0)) t1=0; else if ( RealNear(t1,1)) t1=1;
	    if ( RealNear(t2,0)) t2=0; else if ( RealNear(t2,1)) t2=1;
	    if ( t2<=0 || t2>=1 ) t2 = -1;
	    if ( t1<=0 || t1>=1 ) { t1 = t2; t2 = -1; }
	}
    } else if ( sp->b!=0 ) {
	/* Quadratic, at most one extremum */
	t1 = -sp->c/(2.0*(extended) sp->b);
	if ( t1<=0 || t1>=1 ) t1 = -1;
    } else /*if ( sp->c!=0 )*/ {
	/* linear, no extrema */
    }
    *_t1 = t1; *_t2 = t2;
}

double SplineCurvature(Spline *s, double t) {
	/* Kappa = (x'y'' - y'x'') / (x'^2 + y'^2)^(3/2) */
    double dxdt, dydt, d2xdt2, d2ydt2, denom, numer;

    if ( s==NULL )
return( CURVATURE_ERROR );

    dxdt = (3*s->splines[0].a*t+2*s->splines[0].b)*t+s->splines[0].c;
    dydt = (3*s->splines[1].a*t+2*s->splines[1].b)*t+s->splines[1].c;
    d2xdt2 = 6*s->splines[0].a*t + 2*s->splines[0].b;
    d2ydt2 = 6*s->splines[1].a*t + 2*s->splines[1].b;
    denom = pow( dxdt*dxdt + dydt*dydt, 3.0/2.0 );
    numer = dxdt*d2ydt2 - dydt*d2xdt2;

    if ( numer==0 )
return( 0 );
    if ( denom==0 )
return( CURVATURE_ERROR );

return( numer/denom );
}

int SplineAtInflection(Spline1D *sp, double t ) {
    /* It's a point of inflection if d sp/dt==0 and d2 sp/dt^2==0 */
return ( RealNear( (3*sp->a*t + 2*sp->b)*t + sp->c,0) &&
	    RealNear( 6*sp->a*t + 2*sp->b, 0));
}

int SplineAtMinMax(Spline1D *sp, double t ) {
    /* It's a point of inflection if d sp/dt==0 and d2 sp/dt^2!=0 */
return ( RealNear( (3*sp->a*t + 2*sp->b)*t + sp->c,0) &&
	    !RealNear( 6*sp->a*t + 2*sp->b, 0));
}

int Spline2DFindExtrema(const Spline *sp, extended extrema[4] ) {
    int i,j;
    BasePoint last, cur, mid;

    SplineFindExtrema(&sp->splines[0],&extrema[0],&extrema[1]);
    SplineFindExtrema(&sp->splines[1],&extrema[2],&extrema[3]);

    for ( i=0; i<3; ++i ) for ( j=i+1; j<4; ++j ) {
	if ( (extrema[i]==-1 && extrema[j]!=-1) || (extrema[i]>extrema[j] && extrema[j]!=-1) ) {
	    extended temp = extrema[i];
	    extrema[i] = extrema[j];
	    extrema[j] = temp;
	}
    }
    for ( i=j=0; i<3 && extrema[i]!=-1; ++i ) {
	if ( extrema[i]==extrema[i+1] ) {
	    for ( j=i+1; j<3; ++j )
		extrema[j] = extrema[j+1];
	    extrema[3] = -1;
	}
    }

    /* Extrema which are too close together are not interesting */
    last = sp->from->me;
    for ( i=0; i<4 && extrema[i]!=-1; ++i ) {
	cur.x = ((sp->splines[0].a*extrema[i]+sp->splines[0].b)*extrema[i]+
		sp->splines[0].c)*extrema[i]+sp->splines[0].d;
	cur.y = ((sp->splines[1].a*extrema[i]+sp->splines[1].b)*extrema[i]+
		sp->splines[1].c)*extrema[i]+sp->splines[1].d;
	mid.x = (last.x+cur.x)/2; mid.y = (last.y+cur.y)/2;
	if ( (mid.x==last.x || mid.x==cur.x) &&
		(mid.y==last.y || mid.y==cur.y)) {
	    for ( j=i+1; j<3; ++j )
		extrema[j] = extrema[j+1]; 
	} else
	    last = cur;
    }
    for ( i=0; i<4 && extrema[i]!=-1; ++i );
    if ( i!=0 ) {
	cur = sp->to->me;
	mid.x = (last.x+cur.x)/2; mid.y = (last.y+cur.y)/2;
	if ( (mid.x==last.x || mid.x==cur.x) &&
		(mid.y==last.y || mid.y==cur.y))
	    extrema[--i] = -1;
    }

return( i );
}

int Spline2DFindPointsOfInflection(const Spline *sp, extended poi[2] ) {
    int cnt=0;
    extended a, b, c, b2_fourac, t;
    /* A POI happens when d2 y/dx2 is zero. This is not the same as d2y/dt2 / d2x/dt2 */
    /* d2 y/dx^2 = d/dt ( dy/dt / dx/dt ) / dx/dt */
    /*		 = ( (dx/dt) * d2 y/dt2 - ((dy/dt) * d2 x/dt2) )/ (dx/dt)^3 */
    /* (3ax*t^2+2bx*t+cx) * (6ay*t+2by) - (3ay*t^2+2by*t+cy) * (6ax*t+2bx) == 0 */
    /* (3ax*t^2+2bx*t+cx) * (3ay*t+by) - (3ay*t^2+2by*t+cy) * (3ax*t+bx) == 0 */
    /*   9*ax*ay*t^3 + (3ax*by+6bx*ay)*t^2 + (2bx*by+3cx*ay)*t + cx*by	   */
    /* -(9*ax*ay*t^3 + (3ay*bx+6by*ax)*t^2 + (2by*bx+3cy*ax)*t + cy*bx)==0 */
    /* 3*(ax*by-ay*bx)*t^2 + 3*(cx*ay-cy*ax)*t+ (cx*by-cy*bx) == 0	   */

    a = 3*((extended) sp->splines[1].a*sp->splines[0].b-(extended) sp->splines[0].a*sp->splines[1].b);
    b = 3*((extended) sp->splines[0].c*sp->splines[1].a - (extended) sp->splines[1].c*sp->splines[0].a);
    c = (extended) sp->splines[0].c*sp->splines[1].b-(extended) sp->splines[1].c*sp->splines[0].b;
    if ( !RealNear(a,0) ) {
	b2_fourac = b*b - 4*a*c;
	poi[0] = poi[1] = -1;
	if ( b2_fourac<0 )
return( 0 );
	b2_fourac = esqrt( b2_fourac );
	t = (-b+b2_fourac)/(2*a);
	if ( t>=0 && t<=1.0 )
	    poi[cnt++] = t;
	t = (-b-b2_fourac)/(2*a);
	if ( t>=0 && t<=1.0 ) {
	    if ( cnt==1 && poi[0]>t ) {
		poi[1] = poi[0];
		poi[0] = t;
		++cnt;
	    } else
		poi[cnt++] = t;
	}
    } else if ( !RealNear(b,0) ) {
	t = -c/b;
	if ( t>=0 && t<=1.0 )
	    poi[cnt++] = t;
    }

return( cnt );
}

/* Ok, if the above routine finds an extremum that less than 1 unit */
/*  from an endpoint or another extremum, then many things are */
/*  just going to skip over it, and other things will be confused by this */
/*  so just remove it. It should be so close the difference won't matter */
void SplineRemoveExtremaTooClose(Spline1D *sp, extended *_t1, extended *_t2 ) {
    extended last, test;
    extended t1= *_t1, t2 = *_t2;

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

int SplineSolveFull(const Spline1D *sp,extended val, extended ts[3]) {
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

static int AddPoint(extended x,extended y,extended t,extended s,BasePoint *pts,
	extended t1s[3],extended t2s[3], int soln) {
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

static int AddQuadraticSoln(extended s,const Spline *s1, const Spline *s2, BasePoint pts[3],
	extended t1s[3], extended t2s[3], int soln ) {
    extended t, x, y, d;
    int i;

    if ( s<-.0001 || s>1.0001 )
return( soln );
    if ( s<0 ) s=0; else if ( s>1 ) s=1;

    x = (s2->splines[0].b*s+s2->splines[0].c)*s+s2->splines[0].d;
    y = (s2->splines[1].b*s+s2->splines[1].c)*s+s2->splines[1].d;

    for ( i=0; i<soln; ++i )
	if ( x==pts[i].x && y==pts[i].y )
return( soln );

    d = s1->splines[0].c*(extended) s1->splines[0].c-4*(extended) s1->splines[0].a*(s1->splines[0].d-x);
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

static void IterateSolve(const Spline1D *sp,extended ts[3]) {
    /* The closed form solution has too many rounding errors for my taste... */
    int i,j;

    ts[0] = ts[1] = ts[2] = -1;

    if ( sp->a!=0 ) {
	extended e[4];
	e[0] = 0; e[1] = e[2] = e[3] = 1.0;
	SplineFindExtrema(sp,&e[1],&e[2]);
	if ( e[1]==-1 ) e[1] = 1;
	if ( e[2]==-1 ) e[2] = 1;
	for ( i=j=0; i<3; ++i ) {
	    ts[j] = IterateSplineSolve(sp,e[i],e[i+1],0,.0001);
	    if ( ts[j]!=-1 ) ++j;
	    if ( e[i+1]==1.0 )
	break;
	}
    } else if ( sp->b!=0 ) {
	extended b2_4ac = sp->c*(extended) sp->c - 4*sp->b*(extended) sp->d;
	if ( b2_4ac>=0 ) {
	    b2_4ac = esqrt(b2_4ac);
	    ts[0] = (-sp->c-b2_4ac)/(2*sp->b);
	    ts[1] = (-sp->c+b2_4ac)/(2*sp->b);
	    if ( ts[0]>ts[1] ) { bigreal t = ts[0]; ts[0] = ts[1]; ts[1] = t; }
	}
    } else if ( sp->c!=0 ) {
	ts[0] = -sp->d/(extended) sp->c;
    } else
	/* No solutions, or all solutions */;

    for ( i=j=0; i<3; ++i )
	if ( ts[i]>=0 && ts[i]<=1 )
	    ts[j++] = ts[i];
    for ( i=0; i<j-1; ++i )
	if ( ts[i]+.0000001>ts[i+1]) {
	    ts[i] = (ts[i]+ts[i+1])/2;
	    --j;
	    for ( ++i; i<j; ++i )
		ts[i] = ts[i+1];
	}
    if ( j!=0 ) {
	if ( ts[0]!=0 ) {
	    extended d0 = sp->d;
	    extended dt = ((sp->a*ts[0]+sp->b)*ts[0]+sp->c)*ts[0]+sp->d;
	    if ( d0<0 ) d0=-d0;
	    if ( dt<0 ) dt=-dt;
	    if ( d0<dt )
		ts[0] = 0;
	}
	if ( ts[j-1]!=1.0 ) {
	    extended d1 = sp->a+(extended) sp->b+sp->c+sp->d;
	    extended dt = ((sp->a*ts[j-1]+sp->b)*ts[j-1]+sp->c)*ts[j-1]+sp->d;
	    if ( d1<0 ) d1=-d1;
	    if ( dt<0 ) dt=-dt;
	    if ( d1<dt )
		ts[j-1] = 1;
	}
    }
    for ( ; j<3; ++j )
	ts[j] = -1;
}

static extended ISolveWithin(const Spline1D *sp,extended val,extended tlow, extended thigh) {
    Spline1D temp;
    extended ts[3];
    int i;

    temp = *sp;
    temp.d -= val;
    IterateSolve(&temp,ts);
    if ( tlow<thigh ) {
	for ( i=0; i<3; ++i )
	    if ( ts[i]>=tlow && ts[i]<=thigh )
return( ts[i] );
	for ( i=0; i<3; ++i ) {
	    if ( ts[i]>=tlow-1./1024. && ts[i]<=tlow )
return( tlow );
	    if ( ts[i]>=thigh && ts[i]<=thigh+1./1024 )
return( thigh );
	}
    } else {
	for ( i=0; i<3; ++i )
	    if ( ts[i]>=thigh && ts[i]<=tlow )
return( ts[i] );
	for ( i=0; i<3; ++i ) {
	    if ( ts[i]>=thigh-1./1024. && ts[i]<=thigh )
return( thigh );
	    if ( ts[i]>=tlow && ts[i]<=tlow+1./1024 )
return( tlow );
	}
    }
return( -1 );
}

static int ICAddInter(int cnt,BasePoint *foundpos,extended *foundt1,extended *foundt2,
	const Spline *s1,const Spline *s2,extended t1,extended t2) {
    foundt1[cnt] = t1;
    foundt2[cnt] = t2;
    foundpos[cnt].x = ((s1->splines[0].a*t1+s1->splines[0].b)*t1+
			s1->splines[0].c)*t1+s1->splines[0].d;
    foundpos[cnt].y = ((s1->splines[1].a*t1+s1->splines[1].b)*t1+
			s1->splines[1].c)*t1+s1->splines[1].d;
return( cnt+1 );
}

static int ICBinarySearch(int cnt,BasePoint *foundpos,extended *foundt1,extended *foundt2,
	int other,
	const Spline *s1,const Spline *s2,extended t1low,extended t1high,extended t2low,extended t2high) {
    int major;
    extended t1, t2;
    extended o1o, o2o, o1n, o2n, m;

    major = !other;
    o1o = ((s1->splines[other].a*t1low+s1->splines[other].b)*t1low+
		    s1->splines[other].c)*t1low+s1->splines[other].d;
    o2o = ((s2->splines[other].a*t2low+s2->splines[other].b)*t2low+
		    s2->splines[other].c)*t2low+s2->splines[other].d;
    forever {
	t1 = (t1low+t1high)/2;
	m = ((s1->splines[major].a*t1+s1->splines[major].b)*t1+
			s1->splines[major].c)*t1+s1->splines[major].d;
	t2 = ISolveWithin(&s2->splines[major],m,t2low,t2high);
	if ( t2==-1 )
return( cnt );

	o1n = ((s1->splines[other].a*t1+s1->splines[other].b)*t1+
			s1->splines[other].c)*t1+s1->splines[other].d;
	o2n = ((s2->splines[other].a*t2+s2->splines[other].b)*t2+
			s2->splines[other].c)*t2+s2->splines[other].d;
	if (( o1n-o2n<.001 && o1n-o2n>-.001) ||
		(t1-t1low<.0001 && t1-t1low>-.0001))
return( ICAddInter(cnt,foundpos,foundt1,foundt2,s1,s2,t1,t2));
	if ( (o1o>o2o && o1n<o2n) || (o1o<o2o && o1n>o2n)) {
	    t1high = t1;
	    t2high = t2;
	} else {
	    t1low = t1;
	    t2low = t2;
	}
    }
}

static int CubicsIntersect(const Spline *s1,extended lowt1,extended hight1,BasePoint *min1,BasePoint *max1,
			    const Spline *s2,extended lowt2,extended hight2,BasePoint *min2,BasePoint *max2,
			    BasePoint *foundpos,extended *foundt1,extended *foundt2) {
    int major, other;
    BasePoint max, min;
    extended t1max, t1min, t2max, t2min, t1, t2, t1diff, oldt2;
    extended o1o, o2o, o1n, o2n, m;
    int cnt=0;

    if ( (min.x = min1->x)<min2->x ) min.x = min2->x;
    if ( (min.y = min1->y)<min2->y ) min.y = min2->y;
    if ( (max.x = max1->x)>max2->x ) max.x = max2->x;
    if ( (max.y = max1->y)>max2->y ) max.y = max2->y;
    if ( max.x<min.x || max.y<min.y )
return( 0 );
    if ( max.x-min.x > max.y-min.y )
	major = 0;
    else
	major = 1;
    other = 1-major;

    t1max = ISolveWithin(&s1->splines[major],(&max.x)[major],lowt1,hight1);
    t1min = ISolveWithin(&s1->splines[major],(&min.x)[major],lowt1,hight1);
    t2max = ISolveWithin(&s2->splines[major],(&max.x)[major],lowt2,hight2);
    t2min = ISolveWithin(&s2->splines[major],(&min.x)[major],lowt2,hight2);
    if ( t1max==-1 || t1min==-1 || t2max==-1 || t1min==-1 )
return( 0 );
    t1diff = (t1max-t1min)/64.0;
    if ( t1diff==0 )
return( 0 );

    t1 = t1min; t2 = t2min;
    o1o = ((s1->splines[other].a*t1+s1->splines[other].b)*t1+
		    s1->splines[other].c)*t1+s1->splines[other].d;
    o2o = ((s2->splines[other].a*t2+s2->splines[other].b)*t2+
		    s2->splines[other].c)*t2+s2->splines[other].d;
    if ( o1o==o2o )
	cnt = ICAddInter(cnt,foundpos,foundt1,foundt2,s1,s2,t1,t2);
    forever {
	t1 += t1diff;
	if (( t1max>t1min && t1>t1max ) || (t1max<t1min && t1<t1max) || cnt>3 )
    break;
	m = ((s1->splines[major].a*t1+s1->splines[major].b)*t1+
			s1->splines[major].c)*t1+s1->splines[major].d;
	oldt2 = t2;
	t2 = ISolveWithin(&s2->splines[major],m,lowt2,hight2);
	if ( t2==-1 )
    continue;

	o1n = ((s1->splines[other].a*t1+s1->splines[other].b)*t1+
			s1->splines[other].c)*t1+s1->splines[other].d;
	o2n = ((s2->splines[other].a*t2+s2->splines[other].b)*t2+
			s2->splines[other].c)*t2+s2->splines[other].d;
	if ( o1n==o2n )
	    cnt = ICAddInter(cnt,foundpos,foundt1,foundt2,s1,s2,t1,t2);
	if ( (o1o>o2o && o1n<o2n) || (o1o<o2o && o1n>o2n))
	    cnt = ICBinarySearch(cnt,foundpos,foundt1,foundt2,other,
		    s1,s2,t1-t1diff,t1,oldt2,t2);
	o1o = o1n; o2o = o2n;
    }
return( cnt );
}

/* returns 0=>no intersection, 1=>at least one, location in pts, t1s, t2s */
/*  -1 => We couldn't figure it out in a closed form, have to do a numerical */
/*  approximation */
int SplinesIntersect(const Spline *s1, const Spline *s2, BasePoint pts[9],
	extended t1s[10], extended t2s[10]) {	/* One extra for a trailing -1 */
    BasePoint min1, max1, min2, max2;
    int soln = 0;
    extended x,y,s,t, ac0, ac1;
    extended d;
    int i,j,found;
    Spline1D spline, temp;
    Quartic quad;
    extended tempts[4];	/* 3 solns for cubics, 4 for quartics */
    extended extrema1[6], extrema2[6];
    int ecnt1, ecnt2;

    t1s[0] = t1s[1] = t1s[2] = t1s[3] = -1;
    t2s[0] = t2s[1] = t2s[2] = t2s[3] = -1;

    if ( s1==s2 && !s1->knownlinear && !s1->isquadratic )
	/* Special case see if it doubles back on itself anywhere */;
    else if ( s1==s2 )
return( 0 );			/* Linear and quadratics can't double back, can't self-intersect */
    else if ( s1->splines[0].a == s2->splines[0].a &&
	    s1->splines[0].b == s2->splines[0].b &&
	    s1->splines[0].c == s2->splines[0].c &&
	    s1->splines[0].d == s2->splines[0].d &&
	    s1->splines[1].a == s2->splines[1].a &&
	    s1->splines[1].b == s2->splines[1].b &&
	    s1->splines[1].c == s2->splines[1].c &&
	    s1->splines[1].d == s2->splines[1].d )
return( -1 );			/* Same spline. Intersects everywhere */

    /* Ignore splines which are just a point */
    if ( s1->knownlinear && s1->splines[0].c==0 && s1->splines[1].c==0 )
return( 0 );
    if ( s2->knownlinear && s2->splines[0].c==0 && s2->splines[1].c==0 )
return( 0 );

    if ( s1->knownlinear )
	/* Do Nothing */;
    else if ( s2->knownlinear || (!s1->isquadratic && s2->isquadratic)) {
	const Spline *stemp = s1;
	extended *ts = t1s;
	t1s = t2s; t2s = ts;
	s1 = s2; s2 = stemp;
    }

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
	IterateSolve(&spline,tempts);
	if ( tempts[0]==-1 )
return( false );
	for ( i = 0; i<3 && tempts[i]!=-1; ++i ) {
	    x = ((s2->splines[0].a*tempts[i]+s2->splines[0].b)*tempts[i]+
		    s2->splines[0].c)*tempts[i]+s2->splines[0].d;
	    y = ((s2->splines[1].a*tempts[i]+s2->splines[1].b)*tempts[i]+
		    s2->splines[1].c)*tempts[i]+s2->splines[1].d;
	    if ( (ac0 = s1->splines[0].c)<0 ) ac0 = -ac0;
	    if ( (ac1 = s1->splines[1].c)<0 ) ac1 = -ac1;
	    if ( ac0>ac1 )
		t = (x-s1->splines[0].d)/s1->splines[0].c;
	    else
		t = (y-s1->splines[1].d)/s1->splines[1].c;
	    if ( t<-.001 || t>1.001 || x<min1.x-.01 || y<min1.y-.01 || x>max1.x+.01 || y>max1.y+.01 )
	continue;
	    if ( t<=0 ) {t=0; x=s1->from->me.x; y = s1->from->me.y; }
	    else if ( t>=1 ) { t=1; x=s1->to->me.x; y = s1->to->me.y; }
	    if ( s1->from->me.x==s1->to->me.x )		/* Avoid rounding errors */
		x = s1->from->me.x;			/* on hor/vert lines */
	    else if ( s1->from->me.y==s1->to->me.y )
		y = s1->from->me.y;
	    if ( s2->knownlinear ) {
		if ( s2->from->me.x==s2->to->me.x )
		    x = s2->from->me.x;
		else if ( s2->from->me.y==s2->to->me.y )
		    y = s2->from->me.y;
	    }
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

    /* So let's do it the hard way... we break the splines into little bits */
    /*  where they are monotonic in both dimensions, then check these for */
    /*  possible intersections */
    extrema1[0] = extrema2[0] = 0;
    ecnt1 = Spline2DFindExtrema(s1,extrema1+1);
    ecnt2 = Spline2DFindExtrema(s2,extrema2+1);
    extrema1[++ecnt1] = 1.0;
    extrema2[++ecnt2] = 1.0;
    found=0;
    for ( i=0; i<ecnt1; ++i ) {
	min1.x = ((s1->splines[0].a*extrema1[i]+s1->splines[0].b)*extrema1[i]+
		s1->splines[0].c)*extrema1[i]+s1->splines[0].d;
	min1.y = ((s1->splines[1].a*extrema1[i]+s1->splines[1].b)*extrema1[i]+
		s1->splines[1].c)*extrema1[i]+s1->splines[1].d;
	max1.x = ((s1->splines[0].a*extrema1[i+1]+s1->splines[0].b)*extrema1[i+1]+
		s1->splines[0].c)*extrema1[i+1]+s1->splines[0].d;
	max1.y = ((s1->splines[1].a*extrema1[i+1]+s1->splines[1].b)*extrema1[i+1]+
		s1->splines[1].c)*extrema1[i+1]+s1->splines[1].d;
	if ( max1.x<min1.x ) { extended temp = max1.x; max1.x = min1.x; min1.x = temp; }
	if ( max1.y<min1.y ) { extended temp = max1.y; max1.y = min1.y; min1.y = temp; }
	for ( j=(s1==s2)?i+1:0; j<ecnt2; ++j ) {
	    min2.x = ((s2->splines[0].a*extrema2[j]+s2->splines[0].b)*extrema2[j]+
		    s2->splines[0].c)*extrema2[j]+s2->splines[0].d;
	    min2.y = ((s2->splines[1].a*extrema2[j]+s2->splines[1].b)*extrema2[j]+
		    s2->splines[1].c)*extrema2[j]+s2->splines[1].d;
	    max2.x = ((s2->splines[0].a*extrema2[j+1]+s2->splines[0].b)*extrema2[j+1]+
		    s2->splines[0].c)*extrema2[j+1]+s2->splines[0].d;
	    max2.y = ((s2->splines[1].a*extrema2[j+1]+s2->splines[1].b)*extrema2[j+1]+
		    s2->splines[1].c)*extrema2[j+1]+s2->splines[1].d;
	    if ( max2.x<min2.x ) { extended temp = max2.x; max2.x = min2.x; min2.x = temp; }
	    if ( max2.y<min2.y ) { extended temp = max2.y; max2.y = min2.y; min2.y = temp; }
	    if ( min1.x>max2.x || min2.x>max1.x || min1.y>max2.y || min2.y>max1.y )
		/* No possible intersection */;
	    else if ( s1!=s2 )
		found += CubicsIntersect(s1,extrema1[i],extrema1[i+1],&min1,&max1,
				    s2,extrema2[j],extrema2[j+1],&min2,&max2,
				    &pts[found],&t1s[found],&t2s[found]);
	    else {
		int k,l;
		int cnt = CubicsIntersect(s1,extrema1[i],extrema1[i+1],&min1,&max1,
				    s2,extrema2[j],extrema2[j+1],&min2,&max2,
				    &pts[found],&t1s[found],&t2s[found]);
		for ( k=0; k<cnt; ++k ) {
		    if ( RealNear(t1s[found+k],t2s[found+k]) ) {
			for ( l=k+1; l<cnt; ++l ) {
			    pts[found+l-1] = pts[found+l];
			    t1s[found+l-1] = t1s[found+l];
			    t2s[found+l-1] = t2s[found+l];
			}
			--cnt; --k;
		    }
		}
		found += cnt;
	    }
	}
    }
    t1s[found] = t2s[found] = -1;
return( found!=0 );
}

int LineTangentToSplineThroughPt(Spline *s, BasePoint *pt, extended ts[4],
	extended tmin, extended tmax) {
    /* attempt to find a line though the point pt which is tangent to the spline */
    /*  we return t of the tangent point on the spline (if any)		  */
    /* So the slope of the line through pt&tangent point must match slope */
    /*  at the tangent point on the spline. Or...			  */
    /* (pt.x-xt)/(pt.y-yt) = (dx/dt)/(dy/dt)				  */
    /* (pt.x-xt)*(dy/dt) = (pt.y-yt)*(dx/dt)				  */
    /* (pt.x - ax*t^3 - bx*t^2 - cx*t - dx)(3ay*t^2 + 2by*t + cy) =	  */
    /*      (pt.y - ay*t^3 - by*t^2 - cy*t^2 - dy)(3ax*t^2 + 2bx*t + cx)  */
    /* (-ax*3ay + ay*3ax) * t^5 +					  */
    /*   (-ax*2by - bx*3ay + ay*2bx + by*3ax) * t^4 +			  */
    /*   (-ax*cy - bx*2by - cx*3ay + ay*cx + by*2bx + cy*3ax) * t^3 +	  */
    /*   (pt.x*3ay - bx*cy - cx*2by - dx*3ay - pt.y*3ax + by*cx + cy*2bx + dy*3ax) * t^2 +	*/
    /*   (pt.x*2by - cx*cy - dx*2by - pt.y*2bx + cy*cx + dy*2bx) * t + 	  */
    /*   (pt.x*cy - dx*cy - pt.y*cx + dy*cx) = 0 			  */
    /* Yeah! The order 5 terms cancel out 				  */
    /* (ax*by - bx*ay) * t^4 +						  */
    /*   (2*ax*cy - 2*cx*ay) * t^3 +					  */
    /*   (3*pt.x*ay + bx*cy - cx*by - 3*dx*ay - 3*pt.y*ax + 3*dy*ax) * t^2 +	*/
    /*   (2*pt.x*by - 2*dx*by - 2*pt.y*bx + 2*dy*bx) * t +		  */
    /*   (pt.x*cy - dx*cy - pt.y*cx + dy*cx) = 0 			  */
    Quartic quad;
    int i,j,k;

    if ( !finite(pt->x) || !finite(pt->y) ) {
	IError( "Non-finite arguments passed to LineTangentToSplineThroughPt");
return( -1 );
    }

    quad.a = s->splines[0].a*s->splines[1].b - s->splines[0].b*s->splines[1].a;
    quad.b = 2*s->splines[0].a*s->splines[1].c - 2*s->splines[0].c*s->splines[1].a;
    quad.c = 3*pt->x*s->splines[1].a + s->splines[0].b*s->splines[1].c - s->splines[0].c*s->splines[1].b - 3*s->splines[0].d*s->splines[1].a - 3*pt->y*s->splines[0].a + 3*s->splines[1].d*s->splines[0].a;
    quad.d = 2*pt->x*s->splines[1].b - 2*s->splines[0].d*s->splines[1].b - 2*pt->y*s->splines[0].b + 2*s->splines[1].d*s->splines[0].b;
    quad.e = pt->x*s->splines[1].c - s->splines[0].d*s->splines[1].c - pt->y*s->splines[0].c + s->splines[1].d*s->splines[0].c;

    if ( _QuarticSolve(&quad,ts)==-1 )
return( -1 );

    for ( i=0; i<4 && ts[i]!=-999999; ++i ) {
	if ( ts[i]>tmin-.0001 && ts[i]<tmin ) ts[i] = tmin;
	if ( ts[i]>tmax && ts[i]<tmax+.001 ) ts[i] = tmax;
	if ( ts[i]>tmax || ts[i]<tmin ) ts[i] = -999999;
    }
    for ( i=3; i>=0 && ts[i]==-999999; --i );
    if ( i==-1 )
return( -1 );
    for ( j=i ; j>=0 ; --j ) {
	if ( ts[j]<0 ) {
	    for ( k=j+1; k<=i; ++k )
		ts[k-1] = ts[k];
	    ts[i--] = -999999;
	}
    }
return(i+1);
}

static int XSolve(Spline *spline,real tmin, real tmax,FindSel *fs) {
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];
    bigreal t,x,y;

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
    bigreal t,x,y;

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
    bigreal t,y;
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];

    if ( xspline->a!=0 ) {
	extended t1, t2, tbase;
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
	bigreal root = xspline->c*xspline->c - 4*xspline->b*(xspline->d-fs->p->cx);
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
    bigreal t,x,y;
    Spline1D *yspline = &spline->splines[1], *xspline = &spline->splines[0];
    bigreal dx, dy;

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
	    bigreal root = yspline->c*yspline->c - 4*yspline->b*(yspline->d-fs->p->cy);
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
	    extended t1, t2, tbase;
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

static int SplinePrevMinMax(Spline *s,int up) {
    const double t = .9999;
    double y;
    int pup;

    s = s->from->prev;
    while ( s->from->me.y==s->to->me.y && s->islinear )
	s = s->from->prev;
    y = ((s->splines[1].a*t + s->splines[1].b)*t + s->splines[1].c)*t + s->splines[1].d;
    pup = s->to->me.y > y;
return( pup!=up );
}

static int SplineNextMinMax(Spline *s,int up) {
    const double t = .0001;
    double y;
    int nup;

    s = s->to->next;
    while ( s->from->me.y==s->to->me.y && s->islinear )
	s = s->to->next;
    y = ((s->splines[1].a*t + s->splines[1].b)*t + s->splines[1].c)*t + s->splines[1].d;
    nup = y > s->from->me.y;
return( nup!=up );
}

static int Crossings(Spline *s,BasePoint *pt) {
    extended ext[4];
    int i, cnt=0;
    bigreal yi, yi1, t, x;

    ext[0] = 0; ext[3] = 1.0;
    SplineFindExtrema(&s->splines[1],&ext[1],&ext[2]);
    if ( ext[2]!=-1 && SplineAtInflection(&s->splines[1],ext[2])) ext[2]=-1;
    if ( ext[1]!=-1 && SplineAtInflection(&s->splines[1],ext[1])) {ext[1] = ext[2]; ext[2]=-1;}
    if ( ext[1]==-1 ) ext[1] = 1.0;
    else if ( ext[2]==-1 ) ext[2] = 1.0;
    yi = s->splines[1].d;
    for ( i=0; ext[i]!=1.0; ++i, yi=yi1 ) {
	yi1 = ((s->splines[1].a*ext[i+1]+s->splines[1].b)*ext[i+1]+s->splines[1].c)*ext[i+1]+s->splines[1].d;
	if ( yi==yi1 )
    continue;		/* Ignore horizontal lines */
	if ( (yi>yi1 && (pt->y<yi1 || pt->y>yi)) ||
		(yi<yi1 && (pt->y<yi || pt->y>yi1)) )
    continue;
	t = IterateSplineSolve(&s->splines[1],ext[i],ext[i+1],pt->y,.0001);
	if ( t==-1 )
    continue;
	x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	if ( x>=pt->x )		/* Things on the edge are not inside */
    continue;
	if (( ext[i]!=0 && RealApprox(t,ext[i]) && SplineAtMinMax(&s->splines[1],ext[i]) ) ||
		( ext[i+1]!=1 && RealApprox(t,ext[i+1]) && SplineAtMinMax(&s->splines[1],ext[i+1]) ))
    continue;			/* Min/Max points don't add to count */
	if (( RealApprox(t,0) && SplinePrevMinMax(s,yi1>yi) ) ||
		( RealApprox(t,1) && SplineNextMinMax(s,yi1>yi) ))
    continue;			/* ditto */
	if ( pt->y==yi1 )	/* Not a min/max. prev/next spline continues same direction */
    continue;			/* If it's at an endpoint we don't want to count it twice */
				/* So only add to count at start endpoint, never at final */
	if ( yi1>yi )
	    ++cnt;
	else
	    --cnt;
    }
return( cnt );
}

/* Return whether this point is within the set of contours in spl */
/* Draw a line from [-infinity,pt.y] to [pt.x,pt.y] and count contour crossings */
int SSPointWithin(SplineSet *spl,BasePoint *pt) {
    int cnt=0;
    Spline *s, *first;

    while ( spl!=NULL ) {
	if ( spl->first->prev!=NULL ) {
	    first = NULL;
	    for ( s = spl->first->next; s!=first; s=s->to->next ) {
		if ( first==NULL ) first = s;
		if (( s->from->me.x>pt->x && s->from->nextcp.x>pt->x &&
			s->to->me.x>pt->x && s->to->prevcp.x>pt->x) ||
		    ( s->from->me.y>pt->y && s->from->nextcp.y>pt->y &&
			s->to->me.y>pt->y && s->to->prevcp.y>pt->y) ||
		    ( s->from->me.y<pt->y && s->from->nextcp.y<pt->y &&
			s->to->me.y<pt->y && s->to->prevcp.y<pt->y))
	    continue;
		cnt += Crossings(s,pt);
	    }
	}
	spl = spl->next;
    }
return( cnt!=0 );	
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

    chunkfree(h,sizeof(DStemInfo));
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( kp->adjust!=NULL ) {
	    free(kp->adjust->corrections);
	    chunkfree(kp->adjust,sizeof(DeviceTable));
	}
#endif
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
	    ap->next = NULL;
	    AnchorPointsFree(ap);
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
    for ( test = sc->layers[ly_fore].undoes; test!=NULL; test=test->next )
	if ( test->undotype==ut_state || test->undotype==ut_tstate ||
		test->undotype==ut_statehint || test->undotype==ut_statename )
	    test->u.state.anchor = AnchorPointsRemoveName(test->u.state.anchor,an);
    for ( test = sc->layers[ly_fore].redoes; test!=NULL; test=test->next )
	if ( test->undotype==ut_state || test->undotype==ut_tstate ||
		test->undotype==ut_statehint || test->undotype==ut_statename )
	    test->u.state.anchor = AnchorPointsRemoveName(test->u.state.anchor,an);
}

void SFRemoveAnchorClass(SplineFont *sf,AnchorClass *an) {
    int i;
    AnchorClass *prev, *test;

    PasteRemoveAnchorClass(sf,an);

    for ( i=0; i<sf->glyphcnt; ++i )
	SCRemoveAnchorClass(sf->glyphs[i],an);
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
		ap->next = NULL;
		AnchorPointsFree(ap);
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
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];

	sc->anchor = APAnchorClassMerge(sc->anchor,into,from);
    }
}

AnchorPoint *AnchorPointsCopy(AnchorPoint *alist) {
    AnchorPoint *head=NULL, *last, *ap;

    while ( alist!=NULL ) {
	ap = chunkalloc(sizeof(AnchorPoint));
	*ap = *alist;
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	if ( ap->xadjust.corrections!=NULL ) {
	    int len = ap->xadjust.last_pixel_size-ap->xadjust.first_pixel_size+1;
	    ap->xadjust.corrections = galloc(len);
	    memcpy(ap->xadjust.corrections,alist->xadjust.corrections,len);
	}
	if ( ap->yadjust.corrections!=NULL ) {
	    int len = ap->yadjust.last_pixel_size-ap->yadjust.first_pixel_size+1;
	    ap->yadjust.corrections = galloc(len);
	    memcpy(ap->yadjust.corrections,alist->yadjust.corrections,len);
	}
#endif
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
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	free(ap->xadjust.corrections);
	free(ap->yadjust.corrections);
#endif
	chunkfree(ap,sizeof(AnchorPoint));
    }
}

#ifdef FONTFORGE_CONFIG_DEVICETABLES
void ValDevFree(ValDevTab *adjust) {
    if ( adjust==NULL )
return;
    free( adjust->xadjust.corrections );
    free( adjust->yadjust.corrections );
    free( adjust->xadv.corrections );
    free( adjust->yadv.corrections );
    chunkfree(adjust,sizeof(ValDevTab));
}

ValDevTab *ValDevTabCopy(ValDevTab *orig) {
    ValDevTab *new;
    int i;

    if ( orig==NULL )
return( NULL );
    new = chunkalloc(sizeof(ValDevTab));
    for ( i=0; i<4; ++i ) {
	if ( (&orig->xadjust)[i].corrections!=NULL ) {
	    int len = (&orig->xadjust)[i].last_pixel_size - (&orig->xadjust)[i].first_pixel_size + 1;
	    (&new->xadjust)[i] = (&orig->xadjust)[i];
	    (&new->xadjust)[i].corrections = galloc(len);
	    memcpy((&new->xadjust)[i].corrections,(&orig->xadjust)[i].corrections,len);
	}
    }
return( new );
}

void DeviceTableFree(DeviceTable *dt) {

    if ( dt==NULL )
return;

    free(dt->corrections);
    chunkfree(dt,sizeof(DeviceTable));
}

DeviceTable *DeviceTableCopy(DeviceTable *orig) {
    DeviceTable *new;
    int len;

    if ( orig==NULL )
return( NULL );
    new = chunkalloc(sizeof(DeviceTable));
    *new = *orig;
    len = orig->last_pixel_size - orig->first_pixel_size + 1;
    new->corrections = galloc(len);
    memcpy(new->corrections,orig->corrections,len);
return( new );
}

void DeviceTableSet(DeviceTable *adjust, int size, int correction) {
    int len, i, j;

    len = adjust->last_pixel_size-adjust->first_pixel_size + 1;
    if ( correction==0 ) {
	if ( adjust->corrections==NULL ||
		size<adjust->first_pixel_size ||
		size>adjust->last_pixel_size )
return;
	adjust->corrections[size-adjust->first_pixel_size] = 0;
	for ( i=0; i<len; ++i )
	    if ( adjust->corrections[i]!=0 )
	break;
	if ( i==len ) {
	    free(adjust->corrections);
	    memset(adjust,0,sizeof(DeviceTable));
	} else {
	    if ( i!=0 ) {
		for ( j=0; j<len-i; ++j )
		    adjust->corrections[j] = adjust->corrections[j+i];
		adjust->first_pixel_size += i;
		len -= i;
	    }
	    for ( i=len-1; i>=0; --i )
		if ( adjust->corrections[i]!=0 )
	    break;
	    adjust->last_pixel_size = adjust->first_pixel_size+i;
	}
    } else {
	if ( adjust->corrections==NULL ) {
	    adjust->first_pixel_size = adjust->last_pixel_size = size;
	    adjust->corrections = galloc(1);
	} else if ( size>=adjust->first_pixel_size &&
		size<=adjust->last_pixel_size ) {
	} else if ( size>adjust->last_pixel_size ) {
	    adjust->corrections = grealloc(adjust->corrections,
		    size-adjust->first_pixel_size);
	    for ( i=len; i<size-adjust->first_pixel_size; ++i )
		adjust->corrections[i] = 0;
	    adjust->last_pixel_size = size;
	} else {
	    int8 *new = galloc(adjust->last_pixel_size-size+1);
	    memset(new,0,adjust->first_pixel_size-size);
	    memcpy(new+adjust->first_pixel_size-size,
		    adjust->corrections, len);
	    adjust->first_pixel_size = size;
	    free(adjust->corrections);
	    adjust->corrections = new;
	}
	adjust->corrections[size-adjust->first_pixel_size] = correction;
    }
}
#endif

void PSTFree(PST *pst) {
    PST *pnext;
    for ( ; pst!=NULL; pst = pnext ) {
	pnext = pst->next;
	if ( pst->type==pst_lcaret )
	    free(pst->u.lcaret.carets);
	else if ( pst->type==pst_pair ) {
	    free(pst->u.pair.paired);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    ValDevFree(pst->u.pair.vr[0].adjust);
	    ValDevFree(pst->u.pair.vr[1].adjust);
#endif
	    chunkfree(pst->u.pair.vr,sizeof(struct vr [2]));
	} else if ( pst->type!=pst_position ) {
	    free(pst->u.subs.variant);
	} else if ( pst->type==pst_position ) {
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	    ValDevFree(pst->u.pos.adjust);
#endif
	}
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
	chunkfree(l,sizeof(*l));
	l = next;
    }
}

void AltUniFree(struct altuni *altuni) {
    struct altuni *next;

    while ( altuni ) {
	next = altuni->next;
	chunkfree(altuni,sizeof(struct altuni));
	altuni = next;
    }
}

void LayerDefault(Layer *layer) {
    memset(layer,0,sizeof(Layer));
#ifdef FONTFORGE_CONFIG_TYPE3
    layer->fill_brush.opacity = layer->stroke_pen.brush.opacity = 1.0;
    layer->fill_brush.col = layer->stroke_pen.brush.col = COLOR_INHERITED;
    layer->stroke_pen.width = 10;
    layer->stroke_pen.linecap = lc_round;
    layer->stroke_pen.linejoin = lj_round;
    layer->dofill = true;
    layer->fillfirst = true;
    layer->stroke_pen.trans[0] = layer->stroke_pen.trans[3] = 1.0;
    layer->stroke_pen.trans[1] = layer->stroke_pen.trans[2] = 0.0;
    /* Dashes default to an unbroken line */
#endif
}

SplineChar *SplineCharCreate(void) {
    SplineChar *sc = chunkalloc(sizeof(SplineChar));
    sc->color = COLOR_DEFAULT;
    sc->orig_pos = 0xffff;
    sc->unicodeenc = -1;
    sc->layer_cnt = 2;
#ifdef FONTFORGE_CONFIG_TYPE3
    sc->layers = gcalloc(2,sizeof(Layer));
    LayerDefault(&sc->layers[0]);
    LayerDefault(&sc->layers[1]);
#endif
    sc->tex_height = sc->tex_depth = sc->tex_sub_pos = sc->tex_super_pos =
	    TEX_UNDEF;
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
    int i;

    if ( sc==NULL )
return;
    free(sc->name);
    free(sc->comment);
    for ( i=0; i<sc->layer_cnt; ++i ) {
	SplinePointListsFree(sc->layers[i].splines);
	RefCharsFree(sc->layers[i].refs);
	ImageListsFree(sc->layers[i].images);
	/* image garbage collection????!!!! */
	UndoesFree(sc->layers[i].undoes);
	UndoesFree(sc->layers[i].redoes);
    }
    StemInfosFree(sc->hstem);
    StemInfosFree(sc->vstem);
    DStemInfosFree(sc->dstem);
    MinimumDistancesFree(sc->md);
    KernPairsFree(sc->kerns);
    KernPairsFree(sc->vkerns);
    AnchorPointsFree(sc->anchor);
    SplineCharListsFree(sc->dependents);
    PSTFree(sc->possub);
    free(sc->ttf_instrs);
    free(sc->countermasks);
#ifdef FONTFORGE_CONFIG_TYPE3
    free(sc->layers);
#endif
    AltUniFree(sc->altuni);
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
	free(an->name);
	chunkfree(an,sizeof(AnchorClass));
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

void ScriptLangListFree(struct scriptlanglist *sl) {
    struct scriptlanglist *next;

    while ( sl!=NULL ) {
	next = sl->next;
	free(sl->morelangs);
	chunkfree(sl,sizeof(*sl));
	sl = next;
    }
}

void FeatureScriptLangListFree(FeatureScriptLangList *fl) {
    FeatureScriptLangList *next;

    while ( fl!=NULL ) {
	next = fl->next;
	ScriptLangListFree(fl->scripts);
	chunkfree(fl,sizeof(*fl));
	fl = next;
    }
}

void OTLookupFree(OTLookup *lookup) {
    struct lookup_subtable *st, *stnext;

    free(lookup->lookup_name);
    FeatureScriptLangListFree(lookup->features);
    for ( st=lookup->subtables; st!=NULL; st=stnext ) {
	stnext = st->next;
	free(st->subtable_name);
	free(st->suffix);
	chunkfree(st,sizeof(struct lookup_subtable));
    }
    chunkfree( lookup,sizeof(OTLookup) );
}

void OTLookupListFree(OTLookup *lookup ) {
    OTLookup *next;

    for ( ; lookup!=NULL; lookup = next ) {
	next = lookup->next;
	OTLookupFree(lookup);
    }
}

KernClass *KernClassCopy(KernClass *kc) {
    KernClass *new;
    int i;

    if ( kc==NULL )
return( NULL );
    new = chunkalloc(sizeof(KernClass));
    *new = *kc;
    new->firsts = galloc(new->first_cnt*sizeof(char *));
    new->seconds = galloc(new->second_cnt*sizeof(char *));
    new->offsets = galloc(new->first_cnt*new->second_cnt*sizeof(int16));
    memcpy(new->offsets,kc->offsets, new->first_cnt*new->second_cnt*sizeof(int16));
    for ( i=0; i<new->first_cnt; ++i )
	new->firsts[i] = copy(kc->firsts[i]);
    for ( i=0; i<new->second_cnt; ++i )
	new->seconds[i] = copy(kc->seconds[i]);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
    new->adjusts = gcalloc(new->first_cnt*new->second_cnt,sizeof(DeviceTable));
    memcpy(new->adjusts,kc->adjusts, new->first_cnt*new->second_cnt*sizeof(DeviceTable));
    for ( i=new->first_cnt*new->second_cnt-1; i>=0 ; --i ) {
	if ( new->adjusts[i].corrections!=NULL ) {
	    int8 *old = new->adjusts[i].corrections;
	    int len = new->adjusts[i].last_pixel_size - new->adjusts[i].first_pixel_size + 1;
	    new->adjusts[i].corrections = galloc(len);
	    memcpy(new->adjusts[i].corrections,old,len);
	}
    }
#endif
    new->next = NULL;
return( new );
}

void KernClassListFree(KernClass *kc) {
    int i;
    KernClass *n;

    while ( kc ) {
	for ( i=1; i<kc->first_cnt; ++i )
	    free(kc->firsts[i]);
	for ( i=1; i<kc->second_cnt; ++i )
	    free(kc->seconds[i]);
	free(kc->firsts);
	free(kc->seconds);
	free(kc->offsets);
#ifdef FONTFORGE_CONFIG_DEVICETABLES
	for ( i=kc->first_cnt*kc->second_cnt-1; i>=0 ; --i )
	    free(kc->adjusts[i].corrections);
	free(kc->adjusts);
#endif
	n = kc->next;
	chunkfree(kc,sizeof(KernClass));
	kc = n;
    }
}

void MacNameListFree(struct macname *mn) {
    struct macname *next;

    while ( mn!=NULL ) {
	next = mn->next;
	free(mn->name);
	chunkfree(mn,sizeof(struct macname));
	mn = next;
    }
}

void MacSettingListFree(struct macsetting *ms) {
    struct macsetting *next;

    while ( ms!=NULL ) {
	next = ms->next;
	MacNameListFree(ms->setname);
	chunkfree(ms,sizeof(struct macsetting));
	ms = next;
    }
}

void MacFeatListFree(MacFeat *mf) {
    MacFeat *next;

    while ( mf!=NULL ) {
	next = mf->next;
	MacNameListFree(mf->featname);
	MacSettingListFree(mf->settings);
	chunkfree(mf,sizeof(MacFeat));
	mf = next;
    }
}

void ASMFree(ASM *sm) {
    ASM *next;
    int i;

    while ( sm!=NULL ) {
	next = sm->next;
	if ( sm->type==asm_insert ) {
	    for ( i=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
		free( sm->state[i].u.insert.mark_ins );
		free( sm->state[i].u.insert.cur_ins );
	    }
	} else if ( sm->type==asm_kern ) {
	    for ( i=0; i<sm->class_cnt*sm->state_cnt; ++i ) {
		free( sm->state[i].u.kern.kerns );
	    }
	}
	for ( i=4; i<sm->class_cnt; ++i )
	    free(sm->classes[i]);
	free(sm->state);
	free(sm->classes);
	chunkfree(sm,sizeof(ASM));
	sm = next;
    }
}

void OtfNameListFree(struct otfname *on) {
    struct otfname *on_next;

    for ( ; on!=NULL; on = on_next ) {
	on_next = on->next;
	free(on->name);
	chunkfree(on,sizeof(*on));
    }
}

EncMap *EncMapNew(int enccount,int backmax,Encoding *enc) {
    EncMap *map = chunkalloc(sizeof(EncMap));
    
    map->enccount = map->encmax = enccount;
    map->backmax = backmax;
    map->map = galloc(enccount*sizeof(int));
    memset(map->map,-1,enccount*sizeof(int));
    map->backmap = galloc(backmax*sizeof(int));
    memset(map->backmap,-1,backmax*sizeof(int));
    map->enc = enc;
return(map);
}

EncMap *EncMap1to1(int enccount) {
    EncMap *map = chunkalloc(sizeof(EncMap));
    /* Used for CID fonts where CID is same as orig_pos */
    int i;
    
    map->enccount = map->encmax = map->backmax = enccount;
    map->map = galloc(enccount*sizeof(int));
    map->backmap = galloc(enccount*sizeof(int));
    for ( i=0; i<enccount; ++i )
	map->map[i] = map->backmap[i] = i;
    map->enc = &custom;
return(map);
}

static void EncodingFree(Encoding *enc) {
    int i;

    if ( enc==NULL )
return;
    free(enc->enc_name);
    free(enc->unicode);
    if ( enc->psnames!=NULL ) {
	for ( i=0; i<enc->char_cnt; ++i )
	    free(enc->psnames[i]);
	free(enc->psnames);
    }
    free(enc);
}

void EncMapFree(EncMap *map) {
    if ( map==NULL )
return;

    if ( map->enc->is_temporary )
	EncodingFree(map->enc);
    free(map->map);
    free(map->backmap);
    free(map->remap);
    chunkfree(map,sizeof(EncMap));
}

EncMap *EncMapCopy(EncMap *map) {
    EncMap *new;

    new = chunkalloc(sizeof(EncMap));
    *new = *map;
    new->map = galloc(new->encmax*sizeof(int));
    new->backmap = galloc(new->backmax*sizeof(int));
    memcpy(new->map,map->map,new->enccount*sizeof(int));
    memcpy(new->backmap,map->backmap,new->backmax*sizeof(int));
    if ( map->remap ) {
	int n;
	for ( n=0; map->remap[n].infont!=-1; ++n );
	new->remap = galloc(n*sizeof(struct remap));
	memcpy(new->remap,map->remap,n*sizeof(struct remap));
    }
return( new );
}

void SplineFontFree(SplineFont *sf) {
    int i;
    BDFFont *bdf, *bnext;

    if ( sf==NULL )
return;
    if ( sf->mm!=NULL ) {
	MMSetFree(sf->mm);
return;
    }
    CopyBufferClearCopiedFrom(sf);
    PasteRemoveSFAnchors(sf);
    for ( bdf = sf->bitmaps; bdf!=NULL; bdf = bnext ) {
	bnext = bdf->next;
	BDFFontFree(bdf);
    }
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	SplineCharFree(sf->glyphs[i]);
    free(sf->glyphs);
    free(sf->fontname);
    free(sf->fullname);
    free(sf->familyname);
    free(sf->weight);
    free(sf->copyright);
    free(sf->comments);
    free(sf->filename);
    free(sf->origname);
    free(sf->autosavename);
    free(sf->version);
    free(sf->xuid);
    free(sf->cidregistry);
    free(sf->ordering);
    MacFeatListFree(sf->features);
    /* We don't free the EncMap. That field is only a temporary pointer. Let the FontView free it, that's where it really lives */
    SplinePointListsFree(sf->grid.splines);
    AnchorClassesFree(sf->anchor);
    TtfTablesFree(sf->ttf_tables);
    UndoesFree(sf->grid.undoes);
    UndoesFree(sf->grid.redoes);
    PSDictFree(sf->private);
    TTFLangNamesFree(sf->names);
    for ( i=0; i<sf->subfontcnt; ++i )
	SplineFontFree(sf->subfonts[i]);
    free(sf->subfonts);
    GlyphHashFree(sf);
    OTLookupListFree(sf->gpos_lookups);
    OTLookupListFree(sf->gsub_lookups);
    KernClassListFree(sf->kerns);
    KernClassListFree(sf->vkerns);
    FPSTFree(sf->possub);
    ASMFree(sf->sm);
    OtfNameListFree(sf->fontstyle_name);
    for ( i=1; i<sf->mark_class_cnt; ++i ) {
	free( sf->mark_classes[i] );
	free( sf->mark_class_names[i] );
    }
    free( sf->mark_classes );
    free( sf->mark_class_names );
    free( sf->gasp );
    free(sf);
}

void MMSetFreeContents(MMSet *mm) {
    int i;

    free(mm->instances);

    free(mm->positions);
    free(mm->defweights);

    for ( i=0; i<mm->axis_count; ++i ) {
	free(mm->axes[i]);
	free(mm->axismaps[i].blends);
	free(mm->axismaps[i].designs);
	MacNameListFree(mm->axismaps[i].axisnames);
    }
    free(mm->axismaps);
    free(mm->cdv);
    free(mm->ndv);
    for ( i=0; i<mm->named_instance_count; ++i ) {
	free(mm->named_instances[i].coords);
	MacNameListFree(mm->named_instances[i].names);
    }
    free(mm->named_instances);
}

void MMSetFree(MMSet *mm) {
    int i;

    for ( i=0; i<mm->instance_count; ++i ) {
	mm->instances[i]->mm = NULL;
	mm->instances[i]->map = NULL;
	SplineFontFree(mm->instances[i]);
    }
    mm->normal->mm = NULL;
    SplineFontFree(mm->normal);		/* EncMap gets freed here */
    MMSetFreeContents(mm);

    chunkfree(mm,sizeof(*mm));
}

static int xcmp(const void *_p1, const void *_p2) {
    const SplinePoint * const *_spt1 = _p1, * const *_spt2 = _p2;
    const SplinePoint *sp1 = *_spt1, *sp2 = *_spt2;

    if ( sp1->me.x>sp2->me.x )
return( 1 );
    else if ( sp1->me.x<sp2->me.x )
return( -1 );

return( 0 );
}

static int ycmp(const void *_p1, const void *_p2) {
    const SplinePoint * const *_spt1 = _p1, * const *_spt2 = _p2;
    const SplinePoint *sp1 = *_spt1, *sp2 = *_spt2;

    if ( sp1->me.y>sp2->me.y )
return( 1 );
    else if ( sp1->me.y<sp2->me.y )
return( -1 );

return( 0 );
}

struct cluster {
    int cnt;
    int first, last;
};

static void countcluster(SplinePoint **ptspace, struct cluster *cspace,
	int ptcnt, int is_y, int i, double within, double max) {
    int j;

    cspace[i].cnt = 1;	/* current point is always within its own cluster */
    cspace[i].first = cspace[i].last = i;
    for ( j=i-1; j>=0; --j ) {
	if ( cspace[j].cnt==0 )		/* Already allocated to a different cluster */
    break;
	if ( (&ptspace[j+1]->me.x)[is_y]-(&ptspace[j]->me.x)[is_y]<within &&
		(&ptspace[i]->me.x)[is_y]-(&ptspace[j]->me.x)[is_y]<max ) {
	    ++cspace[i].cnt;
	    cspace[i].first = j;
	} else
    break;
    }
    for ( j=i+1; j<ptcnt; ++j ) {
	if ( cspace[j].cnt==0 )		/* Already allocated to a different cluster */
    break;
	if ( (&ptspace[j]->me.x)[is_y]-(&ptspace[j-1]->me.x)[is_y]<within &&
		(&ptspace[j]->me.x)[is_y]-(&ptspace[i]->me.x)[is_y]<max ) {
	    ++cspace[i].cnt;
	    cspace[i].last = j;
	} else
    break;
    }
}
	
static int _SplineCharRoundToCluster(SplineChar *sc,SplinePoint **ptspace,
	struct cluster *cspace,int ptcnt,int is_y,int dohints,
	int layer, int changed,
	double within, double max ) {
    int i,j,best;
    double low,high,cur;

    for ( i=0; i<ptcnt; ++i )
	cspace[i].cnt = 1;		/* Initialize to non-zero */
    for ( i=0; i<ptcnt; ++i )
	countcluster(ptspace,cspace,ptcnt,is_y,i,within,max);

    forever {
	j=0; best = cspace[0].cnt;
	for ( i=1; i<ptcnt; ++i ) {
	    if ( cspace[i].cnt>best ) {
		j=i;
		best = cspace[i].cnt;
	    }
	}
	if ( best<=1 )		/* No more clusters */
return( changed );
	for ( i=j+1; i<=cspace[j].last && cspace[i].cnt==cspace[j].cnt; ++i );
	j = (j+i-1)/2;		/* There are probably several points with the */
				/* same values for cspace. Pick one in the */
			        /* middle, so that when round we round to the */
			        /* middle rather than to an edge */
	low = (&ptspace[cspace[j].first]->me.x)[is_y];
	high = (&ptspace[cspace[j].last]->me.x)[is_y];
	cur = (&ptspace[j]->me.x)[is_y];
	if ( low==high ) {
	    for ( i=cspace[j].first; i<=cspace[j].last; ++i )
		cspace[i].cnt = 0;
    continue;
	}
	if ( !changed ) {
	    if ( layer==-2 )
		SCPreserveState(sc,dohints);
	    else if ( layer!=-1 )
		SCPreserveLayer(sc,layer,dohints);
	    changed = true;
	}
	for ( i=cspace[j].first; i<=cspace[j].last; ++i ) {
	    double off = (&ptspace[i]->me.x)[is_y] - cur;
	    (&ptspace[i]->nextcp.x)[is_y] -= off;
	    (&ptspace[i]->prevcp.x)[is_y] -= off;
	    (&ptspace[i]->me.x)[is_y] -= off;
	    if ( (&ptspace[i]->prevcp.x)[is_y]-cur>-within &&
		    (&ptspace[i]->prevcp.x)[is_y]-cur<within ) {
		(&ptspace[i]->prevcp.x)[is_y] = cur;
		if ( (&ptspace[i]->prevcp.x)[!is_y]==(&ptspace[i]->me.x)[!is_y] )
		    ptspace[i]->noprevcp = true;
	    }
	    if ( (&ptspace[i]->nextcp.x)[is_y]-cur>-within &&
		    (&ptspace[i]->nextcp.x)[is_y]-cur<within ) {
		(&ptspace[i]->nextcp.x)[is_y] = cur;
		if ( (&ptspace[i]->nextcp.x)[!is_y]==(&ptspace[i]->me.x)[!is_y] )
		    ptspace[i]->nonextcp = true;
	    }
	    cspace[i].cnt = 0;
	}
	if ( dohints ) {
	    StemInfo *h = is_y ? sc->hstem : sc->vstem;
	    while ( h!=NULL && h->start<=high ) {
		if ( h->start>=low ) {
		    h->width -= (h->start-cur);
		    h->start = cur;
		}
		if ( h->start+h->width>=low && h->start+h->width<=high )
		    h->width = cur-h->start;
		h = h->next;
	    }
	}
	/* Ok, surrounding points can no longer use the ones allocated to */
	/*  this cluster. So refigure them */
	for ( i=cspace[j].first-1; i>=0 &&
		( (&ptspace[i]->me.x)[is_y]-cur>-max && (&ptspace[i]->me.x)[is_y]-cur<max ); --i )
	    countcluster(ptspace,cspace,ptcnt,is_y,i,within,max);
	for ( i=cspace[j].last+1; i<ptcnt &&
		( (&ptspace[i]->me.x)[is_y]-cur>-max && (&ptspace[i]->me.x)[is_y]-cur<max ); ++i )
	    countcluster(ptspace,cspace,ptcnt,is_y,i,within,max);
    }
}

int SCRoundToCluster(SplineChar *sc,int layer,int sel,double within,double max) {
    /* Do a cluster analysis on the points in this character. Look at each */
    /* axis in turn. Order all points in char along this axis. For each pt: */
    /*  look at the two points before & after it. If those to pts are within */
    /*  the error bound (within) then continue until we reach a point that */
    /*  is further from its nearest point than "within" or is further from */
    /*  the center point than "max". Count all the points we have found. */
    /*  These points make up the cluster centered on this point. */
    /* Then find the largest cluster & round everything to the center point */
    /*  (shift each point & its cps to center point. Then if cps are "within" */
    /*  set the cps to the center point too) */
    /* Refigure all clusters within "max" of this one, Find the next largest */
    /* cluster, and continue. We stop when the largest cluster has only one */
    /* point in it */
    /* if "sel" is true then we are only interested in selected points */
    /* (if there are no selected points then all points in the current layer) */
    /* if "layer"==-1 then use sf->grid. if layer==-2 then all foreground layers*/
    /* if "layer"==ly_fore or -2 then round hints that fall in our clusters too*/
    int ptcnt, selcnt;
    int l,k,changed;
    SplineSet *spl;
    SplinePoint *sp;
    SplinePoint **ptspace;
    struct cluster *cspace;
    Spline *s, *first;

    /* First figure out what points we will need */
    for ( k=0; k<2; ++k ) {
	ptcnt = selcnt = 0;
	if ( layer==-2 ) {
	    for ( l=ly_fore; l<sc->layer_cnt; ++l ) {
		for ( spl = sc->layers[l].splines; spl!=NULL; spl=spl->next ) {
		    for ( sp = spl->first; ; ) {
			if ( k && (!sel || (sel && sp->selected)) )
			    ptspace[ptcnt++] = sp;
			else if ( !k )
			    ++ptcnt;
			if ( sp->selected ) ++selcnt;
			if ( sp->next==NULL )
		    break;
			sp = sp->next->to;
			if ( sp==spl->first )
		    break;
		    }
		}
	    }
	} else {
	    if ( layer==-1 )
		spl = sc->parent->grid.splines;
	    else
		spl = sc->layers[layer].splines;
	    for ( ; spl!=NULL; spl=spl->next ) {
		for ( sp = spl->first; ; ) {
		    if ( k && (!sel || (sel && sp->selected)) )
			ptspace[ptcnt++] = sp;
		    else if ( !k )
			++ptcnt;
		    if ( sp->selected ) ++selcnt;
		    if ( sp->next==NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==spl->first )
		break;
		}
	    }
	}
	if ( sel && selcnt==0 )
	    sel = false;
	if ( sel ) ptcnt = selcnt;
	if ( ptcnt<=1 )
return(false);				/* Can't be any clusters */
	if ( k==0 )
	    ptspace = galloc((ptcnt+1)*sizeof(SplinePoint *));
	else
	    ptspace[ptcnt] = NULL;
    }

    cspace = galloc(ptcnt*sizeof(struct cluster));

    qsort(ptspace,ptcnt,sizeof(SplinePoint *),xcmp);
    changed = _SplineCharRoundToCluster(sc,ptspace,cspace,ptcnt,false,
	    (layer==-2 || layer==ly_fore) && !sel,layer,false,within,max);

    qsort(ptspace,ptcnt,sizeof(SplinePoint *),ycmp);
    changed = _SplineCharRoundToCluster(sc,ptspace,cspace,ptcnt,true,
	    (layer==-2 || layer==ly_fore) && !sel,layer,changed,within,max);

    free(ptspace);
    free(cspace);

    if ( changed ) {
	if ( layer==-2 ) {
	    for ( l=ly_fore; l<sc->layer_cnt; ++l ) {
		for ( spl = sc->layers[l].splines; spl!=NULL; spl=spl->next ) {
		    first = NULL;
		    for ( s=spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
			SplineRefigure(s);
			if ( first==NULL ) first = s;
		    }
		}
	    }
	} else {
	    if ( layer==-1 )
		spl = sc->parent->grid.splines;
	    else
		spl = sc->layers[layer].splines;
	    for ( ; spl!=NULL; spl=spl->next ) {
		first = NULL;
		for ( s=spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
		    SplineRefigure(s);
		    if ( first==NULL ) first = s;
		}
	    }
	}
	SCCharChangedUpdate(sc);
    }
return( changed );
}

static int SplineRemoveAnnoyingExtrema1(Spline *s,int which,double err_sq) {
    /* Remove extrema which are very close to one of the spline end-points */
    /*  and which are in the oposite direction (along the normal of the */
    /*  close end-point's cp) from the other end-point */
    extended ts[2], t1, t2;
    bigreal df, dt;
    bigreal dp, d_o;
    int i;
    BasePoint pos, norm;
    SplinePoint *close, *other;
    BasePoint *ccp;
    bigreal c_, b_, nextcp, prevcp, prop;
    int changed = false;

    SplineFindExtrema(&s->splines[which],&ts[0],&ts[1]);

    for ( i=0; i<2; ++i ) if ( ts[i]!=-1 && ts[i]!=0 && ts[i]!=1 ) {
	pos.x = ((s->splines[0].a*ts[i]+s->splines[0].b)*ts[i]+s->splines[0].c)*ts[i]+s->splines[0].d;
	pos.y = ((s->splines[1].a*ts[i]+s->splines[1].b)*ts[i]+s->splines[1].c)*ts[i]+s->splines[1].d;
	df = (pos.x-s->from->me.x)*(pos.x-s->from->me.x) + (pos.y-s->from->me.y)*(pos.y-s->from->me.y);
	dt = (pos.x-s->to->me.x)*(pos.x-s->to->me.x) + (pos.y-s->to->me.y)*(pos.y-s->to->me.y);
	if ( df<dt && df<err_sq ) {
	    close = s->from;
	    ccp = &s->from->nextcp;
	    other = s->to;
	} else if ( dt<df && dt<err_sq ) {
	    close = s->to;
	    ccp = &s->to->prevcp;
	    other = s->from;
	} else
    continue;
	if ( ccp->x==close->me.x && ccp->y==close->me.y )
    continue;

	norm.x = (ccp->y-close->me.y);
	norm.y = -(ccp->x-close->me.x);
	dp = (pos.x-close->me.x)*norm.x + (pos.y-close->me.y)*norm.y;
	d_o = (other->me.x-close->me.x)*norm.x + (other->me.y-close->me.y)*norm.y;
	if ( dp!=0 && dp*d_o>=0 )
    continue;

	_SplineFindExtrema(&s->splines[which],&t1,&t2);
	if ( t1==ts[i] ) {
	    if ( close==s->from ) t1=0;
	    else t1 = 1;
	} else if ( t2==ts[i] ) {
	    if ( close==s->from ) t2=0;
	    else t2 = 1;
	} else
    continue;

	if ( t2==-1 )		/* quadratic */
    continue;			/* Can't happen in a quadratic */

	/* The roots of the "slope" quadratic were t1, t2. We have shifted one*/
	/*  root so that that extremum is exactly on an end point */
	/*  Figure out the new slope quadratic, from that what the cubic must */
	/*  be, and from that what the control points must be */
	/* Quad = 3at^2 + 2bt + c */
	/* New quad = 3a * (t^2 -(t1+t2)t + t1*t2) */
	/* a' = a, b' = -(t1+t2)/6a, c' = t1*t2/3a, d' = d */
	/* nextcp = from + c'/3, prevcp = nextcp + (b' + c')/3 */
	/* Then for each cp figure what percentage of the original cp vector */
	/*  (or at least this one dimension of that vector) this represents */
	/*  and scale both dimens by this amount */
	b_ = -(t1+t2)*3*s->splines[which].a/2;
	c_ = (t1*t2)*3*s->splines[which].a;
	nextcp = (&s->from->me.x)[which] + c_/3;
	prevcp = nextcp + (b_ + c_)/3;

	if ( (&s->from->nextcp.x)[which] != (&s->from->me.x)[which] ) {
	    prop = (c_/3) / ( (&s->from->nextcp.x)[which] - (&s->from->me.x)[which] );
	    if ( prop<0 && (c_/3 < .1 && c_/3 > -.1))
		(&s->to->prevcp.x)[which] = nextcp;
	    else if ( prop>=0 && prop<=10 ) {
		s->from->nextcp.x = s->from->me.x + prop*(s->from->nextcp.x-s->from->me.x);
		s->from->nextcp.y = s->from->me.y + prop*(s->from->nextcp.y-s->from->me.y);
		s->from->nonextcp = (prop == 0);
	    }
	}

	if ( (&s->to->prevcp.x)[which] != (&s->to->me.x)[which] ) {
	    prop = ( prevcp - (&s->to->me.x)[which]) /
			( (&s->to->prevcp.x)[which] - (&s->to->me.x)[which] );
	    if ( prop<0 && (prevcp - (&s->to->me.x)[which] < .1 && prevcp - (&s->to->me.x)[which] > -.1))
		(&s->to->prevcp.x)[which] = prevcp;
	    else if ( prop>=0 && prop<=10 ) {
		s->to->prevcp.x = s->to->me.x + prop*(s->to->prevcp.x-s->to->me.x);
		s->to->prevcp.y = s->to->me.y + prop*(s->to->prevcp.y-s->to->me.y);
		s->to->noprevcp = (prop == 0);
	    }
	}
	SplineRefigure(s);
	changed = true;
    }
return( changed );
}

static int SplineRemoveAnnoyingExtrema(Spline *s,double err_sq) {
    int changed;

    changed = SplineRemoveAnnoyingExtrema1(s,0,err_sq);
    if ( SplineRemoveAnnoyingExtrema1(s,1,err_sq) )
	changed = true;
return( changed );
}

int SplineSetsRemoveAnnoyingExtrema(SplineSet *ss,double err) {
    int changed = false;
    double err_sq = err*err;
    Spline *s, *first;


    while ( ss!=NULL ) {
	first = NULL;
	for ( s = ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	    if ( first == NULL ) first = s;
	    if ( SplineRemoveAnnoyingExtrema(s,err_sq))
		changed = true;
	}
	ss = ss->next;
    }
return( changed );
}
