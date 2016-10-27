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
#include "cvundoes.h"
#include "dumppfa.h"
#include "encoding.h"
#include "fontforgevw.h"
#include "fvfonts.h"
#include "fvimportbdf.h"
#include "mm.h"
#include "namelist.h"
#include "parsepfa.h"
#include "parsettf.h"
#include <math.h>
#include "psfont.h"
#include "ustring.h"
#include "utype.h"
#include "views.h"		/* for FindSel structure */
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif
#include <locale.h>
#include "sfd1.h" // This has the extended SplineFont type SplineFont1 for old file versions.
#include "c-strtod.h"
#ifdef FF_UTHASH_GLIF_NAMES
# include "glif_name_hash.h"
#endif

/*#define DEBUG 1*/

typedef struct quartic {
    bigreal a,b,c,d,e;
} Quartic;

/* In an attempt to make allocation more efficient I just keep preallocated */
/*  lists of certain common sizes. It doesn't seem to make much difference */
/*  when allocating stuff, but does when freeing. If the extra complexity */
/*  is bad then put:							  */
/*	#define chunkalloc(size)	calloc(1,size)			  */
/*	#define chunkfree(item,size)	free(item)			  */
/*  into splinefont.h after (or instead of) the definition of chunkalloc()*/

#define ALLOC_CHUNK	100		/* Number of small chunks to malloc at a time */
#ifndef FONTFORGE_CONFIG_USE_DOUBLE
# define CHUNK_MAX	100		/* Maximum size (in chunk units) that we are prepared to allocate */
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

char *strconcat(const char *str1,const char *str2) {
    char *ret;
    int len1 = strlen(str1);
    if ( (ret=malloc(len1+strlen(str2)+1))!=NULL ) {
	strcpy(ret,str1);
	strcpy(ret+len1,str2);
    }
    return( ret );
}

char *strconcat3(const char *str1,const char *str2, const char *str3) {
    char *ret;
    int len1 = strlen(str1), len2 = strlen(str2);
    if ( (ret=malloc(len1+len2+strlen(str3)+1))!=NULL ) {
	strcpy(ret,str1);
	strcpy(ret+len1,str2);
	strcpy(ret+len1+len2,str3);
    }
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
    SplinePoint *sp;
    if ( (sp=chunkalloc(sizeof(SplinePoint)))!=NULL ) {
	sp->me.x = x; sp->me.y = y;
	sp->nextcp = sp->prevcp = sp->me;
	sp->nonextcp = sp->noprevcp = true;
	sp->nextcpdef = sp->prevcpdef = false;
	sp->ttfindex = sp->nextcpindex = 0xfffe;
	sp->name = NULL;
    }
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
	free(sp->name);
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
	free(sp->name);
    chunkfree(sp,sizeof(SplinePoint));
}

void SplinePointsFree(SplinePointList *spl) {
    Spline *first, *spline, *next;
    int nonext;

    if ( spl==NULL )
      return;
    if ( spl->first!=NULL ) {
	nonext = spl->first->next==NULL; // If there is no spline, we set a flag.
	first = NULL;
        // We start on the first spline if it exists.
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = next ) {
	    next = spline->to->next; // Cache the location of the next spline.
	    SplinePointFree(spline->to); // Free the destination point.
	    SplineFree(spline); // Free the spline.
	    if ( first==NULL ) first = spline; // We want to avoid repeating the circuit.
	}
        // If the path is open or has no splines, free the starting point.
	if ( spl->last!=spl->first || nonext )
	    SplinePointFree(spl->first);
    }
}

void SplineSetBeziersClear(SplinePointList *spl) {

    if ( spl==NULL ) return;
    SplinePointsFree(spl);
    spl->first = spl->last = NULL;
    spl->start_offset = 0;
}

void SplinePointListFree(SplinePointList *spl) {

    if ( spl==NULL ) return;
    SplinePointsFree(spl);
    free(spl->spiros);
    free(spl->contour_name);
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
    free(spl->spiros);
    free(spl->contour_name);
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

void SplinePointListsFree(SplinePointList *spl) {
    SplinePointList *next;

    while ( spl!=NULL ) {
	next = spl->next;
	SplinePointListFree(spl);
	spl = next;
    }
}

void SplineSetSpirosClear(SplineSet *spl) {
    free(spl->spiros);
    spl->spiros = NULL;
    spl->spiro_cnt = spl->spiro_max = 0;
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
    int i;

    if ( ref==NULL )
return;
    for ( i=0; i<ref->layer_cnt; ++i ) {
	SplinePointListsFree(ref->layers[i].splines);
	ImageListsFree(ref->layers[i].images);
	GradientFree(ref->layers[i].fill_brush.gradient);
	GradientFree(ref->layers[i].stroke_pen.brush.gradient);
	PatternFree(ref->layers[i].fill_brush.pattern);
	PatternFree(ref->layers[i].stroke_pen.brush.pattern);
    }
    free(ref->layers);
    chunkfree(ref,sizeof(RefChar));
}

RefChar *RefCharCreate(void) {
    RefChar *ref = chunkalloc(sizeof(RefChar));
    ref->layer_cnt = 1;
    ref->layers = calloc(1,sizeof(struct reflayer));
    ref->layers[0].fill_brush.opacity = ref->layers[0].stroke_pen.brush.opacity = 1.0;
    ref->layers[0].fill_brush.col = ref->layers[0].stroke_pen.brush.col = COLOR_INHERITED;
    ref->layers[0].stroke_pen.width = WIDTH_INHERITED;
    ref->layers[0].stroke_pen.linecap = lc_inherited;
    ref->layers[0].stroke_pen.linejoin = lj_inherited;
    ref->layers[0].dofill = true;
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
	    bigreal slope = (next->here.y-prev->here.y) / (bigreal) (next->here.x-prev->here.x);
	    bigreal inter = prev->here.y - slope*prev->here.x;
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

typedef struct spline1 {
    Spline1D sp;
    real s0, s1;
    real c0, c1;
} Spline1;

static void FigureSpline1(Spline1 *sp1,bigreal t0, bigreal t1, Spline1D *sp ) {
    bigreal s = (t1-t0);
    if ( sp->a==0 && sp->b==0 ) {
	sp1->sp.d = sp->d + t0*sp->c;
	sp1->sp.c = s*sp->c;
	sp1->sp.b = sp1->sp.a = 0;
    } else {
	sp1->sp.d = sp->d + t0*(sp->c + t0*(sp->b + t0*sp->a));
	sp1->sp.c = s*(sp->c + t0*(2*sp->b + 3*sp->a*t0));
	sp1->sp.b = s*s*(sp->b+3*sp->a*t0);
	sp1->sp.a = s*s*s*sp->a;
    }
    sp1->c0 = sp1->sp.c/3 + sp1->sp.d;
    sp1->c1 = sp1->c0 + (sp1->sp.b+sp1->sp.c)/3;
}

static LineList *SplineSegApprox(LineList *last, Spline *spline, bigreal start, bigreal end, real scale) {
    /* Divide into n equal segments */
    /* (first point is already on the line list) */
    /* what's a good value for n? Perhaps the normal distance of the control */
    /*  points to the line between the end points. */
    int i,n;
    bigreal t, diff, len;
    bigreal x,y;
    LineList *cur;
    BasePoint startp, endp, slope, off;
    bigreal temp;

    n = 6;
    if ( start==0 && end==1 ) {
	/* No different from the latter case, except we can optimize here */
	/*  and it's easier to understand what is happening in the simple */
	/*  case */
	slope.x = spline->to->me.x - spline->from->me.x;
	slope.y = spline->to->me.y - spline->from->me.y;
	len = slope.x*slope.x + slope.y*slope.y;
	if ( len==0 )
return( last );
	len = sqrt(len);
	slope.x /= len; slope.y /= len;
	off.x = spline->from->nextcp.x - spline->from->me.x;
	off.y = spline->from->nextcp.y - spline->from->me.y;
	temp = (off.x*slope.y - off.y*slope.x) * scale;
	if ( temp<0 ) temp = -temp;
	if ( temp>n ) n = temp;
	off.x = spline->to->prevcp.x - spline->from->me.x;
	off.y = spline->to->prevcp.y - spline->from->me.y;
	temp = (off.x*slope.y - off.y*slope.x) * scale;
	if ( temp<0 ) temp = -temp;
	if ( temp>n ) n = temp;
    } else {
	Spline1 xsp, ysp;
	startp.x = ((spline->splines[0].a*start+spline->splines[0].b)*start+spline->splines[0].c)*start + spline->splines[0].d;
	startp.y = ((spline->splines[1].a*start+spline->splines[1].b)*start+spline->splines[1].c)*start + spline->splines[1].d;
	endp.x = ((spline->splines[0].a*end+spline->splines[0].b)*end+spline->splines[0].c)*end + spline->splines[0].d;
	endp.y = ((spline->splines[1].a*end+spline->splines[1].b)*end+spline->splines[1].c)*end + spline->splines[1].d;
	slope.x = endp.x - startp.x;
	slope.y = endp.y - startp.y;
	FigureSpline1(&xsp,start,end,&spline->splines[0]);
	FigureSpline1(&ysp,start,end,&spline->splines[1]);
	len = slope.x*slope.x + slope.y*slope.y;
	if ( len==0 )
return( last );
	len = sqrt(len);
	slope.x /= len; slope.y /= len;
	off.x = xsp.c0 - startp.x;
	off.y = ysp.c0 - startp.y;
	temp = (off.x*slope.y - off.y*slope.x) * scale;
	if ( temp<0 ) temp = -temp;
	if ( temp>n ) n = temp;
	off.x = xsp.c1 - endp.x;
	off.y = ysp.c1 - endp.y;
	temp = (off.x*slope.y - off.y*slope.x) * scale;
	if ( temp<0 ) temp = -temp;
	if ( temp>n ) n = temp;
    }

    diff = (end-start)/n;
    for ( t=start+diff, i=1; i<=n; ++i, t+=diff ) {
	if ( i==n ) t = end;		/* Avoid rounding errors */
	cur = chunkalloc(sizeof(LineList) );
	x = ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d;
	y = ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d;
	cur->here.x = rint(x*scale);
	cur->here.y = rint(y*scale);
	last->next = cur;
	last = cur;
    }
return( last );
}

LinearApprox *SplineApproximate(Spline *spline, real scale) {
    LinearApprox *test;
    LineList *cur, *last=NULL;
    extended poi[2], lastt;
    int i,n;

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
    } else if ( spline->isquadratic ) {
	last = SplineSegApprox(last,spline,0,1,scale);
    } else {
	n = Spline2DFindPointsOfInflection(spline,poi);
	lastt=0;
	for ( i=0; i<n; ++i ) {
	    last = SplineSegApprox(last,spline,lastt,poi[i],scale);
	    lastt = poi[i];
	}
	last = SplineSegApprox(last,spline,lastt,1,scale);
    }
    SimplifyLineList(test->lines);
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

static void _SplineSetFindClippedBounds(const SplinePointList *spl, DBounds *bounds,DBounds *clipb) {
    Spline *spline, *first;
    /* Ignore contours consisting of a single point (used for hinting, anchors */
    /*  for mark to base, etc. */

    for ( ; spl!=NULL; spl = spl->next ) if ( spl->first->next!=NULL && spl->first->next->to != spl->first ) {
	first = NULL;
	if ( !spl->is_clip_path ) {
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
	} else {
	    if ( clipb->minx==0 && clipb->maxx==0 && clipb->miny==0 && clipb->maxy == 0 ) {
		clipb->minx = clipb->maxx = spl->first->me.x;
		clipb->miny = clipb->maxy = spl->first->me.y;
	    } else {
		if ( spl->first->me.x<clipb->minx ) clipb->minx = spl->first->me.x;
		if ( spl->first->me.x>clipb->maxx ) clipb->maxx = spl->first->me.x;
		if ( spl->first->me.y<clipb->miny ) clipb->miny = spl->first->me.y;
		if ( spl->first->me.y>clipb->maxy ) clipb->maxy = spl->first->me.y;
	    }
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		SplineFindBounds(spline,clipb);
		if ( first==NULL ) first = spline;
	    }
	}
    }
}

void SplineSetFindBounds(const SplinePointList *spl, DBounds *bounds) {
    DBounds clipb;
    memset(bounds,'\0',sizeof(*bounds));
    memset(&clipb,'\0',sizeof(clipb));
    _SplineSetFindClippedBounds(spl, bounds,&clipb);
    if ( clipb.minx!=0 || clipb.miny!=0 || clipb.maxx!=0 || clipb.maxy!=0 ) {
	if ( bounds->minx<clipb.minx ) bounds->minx = clipb.minx;
	if ( bounds->miny<clipb.miny ) bounds->miny = clipb.miny;
	if ( bounds->maxx>clipb.maxx ) bounds->maxx = clipb.maxx;
	if ( bounds->maxy>clipb.maxy ) bounds->maxy = clipb.maxy;
    }
}

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

static void _SplineCharLayerFindBounds(SplineChar *sc,int layer, DBounds *bounds) {
    RefChar *rf;
    ImageList *img;
    real e;
    DBounds b, clipb;

    for ( rf=sc->layers[layer].refs; rf!=NULL; rf = rf->next ) {
	if ( bounds->minx==0 && bounds->maxx==0 && bounds->miny==0 && bounds->maxy == 0 )
	    *bounds = rf->bb;
	else if ( rf->bb.minx!=0 || rf->bb.maxx != 0 || rf->bb.maxy != 0 || rf->bb.miny!=0 ) {
	    if ( rf->bb.minx < bounds->minx ) bounds->minx = rf->bb.minx;
	    if ( rf->bb.miny < bounds->miny ) bounds->miny = rf->bb.miny;
	    if ( rf->bb.maxx > bounds->maxx ) bounds->maxx = rf->bb.maxx;
	    if ( rf->bb.maxy > bounds->maxy ) bounds->maxy = rf->bb.maxy;
	}
    }
    memset(&b,0,sizeof(b));
    memset(&clipb,0,sizeof(clipb));
    _SplineSetFindClippedBounds(sc->layers[layer].splines,&b,&clipb);
    for ( img=sc->layers[layer].images; img!=NULL; img=img->next )
	_ImageFindBounds(img,bounds);
    if ( sc->layers[layer].dostroke ) {
	if ( sc->layers[layer].stroke_pen.width!=WIDTH_INHERITED )
	    e = sc->layers[layer].stroke_pen.width*sc->layers[layer].stroke_pen.trans[0];
	else
	    e = sc->layers[layer].stroke_pen.trans[0];
	b.minx -= e; b.maxx += e;
	b.miny -= e; b.maxy += e;
    }
    if ( clipb.minx!=0 || clipb.miny!=0 || clipb.maxx!=0 || clipb.maxy!=0 ) {
	if ( b.minx<clipb.minx ) b.minx = clipb.minx;
	if ( b.miny<clipb.miny ) b.miny = clipb.miny;
	if ( b.maxx>clipb.maxx ) b.maxx = clipb.maxx;
	if ( b.maxy>clipb.maxy ) b.maxy = clipb.maxy;
    }
    if ( bounds->minx==0 && bounds->maxx==0 && bounds->miny==0 && bounds->maxy == 0 )
	*bounds = b;
    else if ( b.minx!=0 || b.maxx != 0 || b.maxy != 0 || b.miny!=0 ) {
	if ( b.minx < bounds->minx ) bounds->minx = b.minx;
	if ( b.miny < bounds->miny ) bounds->miny = b.miny;
	if ( b.maxx > bounds->maxx ) bounds->maxx = b.maxx;
	if ( b.maxy > bounds->maxy ) bounds->maxy = b.maxy;
    }

    if ( sc->parent!=NULL && sc->parent->strokedfont &&
	    (bounds->minx!=bounds->maxx || bounds->miny!=bounds->maxy)) {
	real sw = sc->parent->strokewidth;
	bounds->minx -= sw; bounds->miny -= sw;
	bounds->maxx += sw; bounds->maxy += sw;
    }
}

void SplineCharLayerFindBounds(SplineChar *sc,int layer,DBounds *bounds) {

    if ( sc->parent!=NULL && sc->parent->multilayer ) {
	SplineCharFindBounds(sc,bounds);
return;
    }

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;
    bounds->miny = bounds->maxy = 0;

    _SplineCharLayerFindBounds(sc,layer,bounds);
}

void SplineCharFindBounds(SplineChar *sc,DBounds *bounds) {
    int i;
    int first,last;

    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;
    bounds->miny = bounds->maxy = 0;

    first = last = ly_fore;
    if ( sc->parent!=NULL && sc->parent->multilayer )
	last = sc->layer_cnt-1;
    for ( i=first; i<=last; ++i )
	_SplineCharLayerFindBounds(sc,i,bounds);
}

void SplineFontLayerFindBounds(SplineFont *sf,int layer,DBounds *bounds) {
    int i, k, first, last;

    if ( sf->multilayer ) {
	SplineFontFindBounds(sf,bounds);
return;
    }

    bounds->minx = bounds->maxx = 0;
    bounds->miny = bounds->maxy = 0;

    for ( i = 0; i<sf->glyphcnt; ++i ) {
	SplineChar *sc = sf->glyphs[i];
	if ( sc!=NULL ) {
	    first = last = ly_fore;
	    if ( sc->parent != NULL && sc->parent->multilayer )
		last = sc->layer_cnt-1;
	    for ( k=first; k<=last; ++k )
		_SplineCharLayerFindBounds(sc,k,bounds);
	}
    }
}

void SplineFontFindBounds(SplineFont *sf,DBounds *bounds) {
    int i, k, first, last;

    bounds->minx = bounds->maxx = 0;
    bounds->miny = bounds->maxy = 0;

    for ( i = 0; i<sf->glyphcnt; ++i ) {
	SplineChar *sc = sf->glyphs[i];
	if ( sc!=NULL ) {
	    first = last = ly_fore;
	    if ( sf->multilayer )
		last = sc->layer_cnt-1;
	    for ( k=first; k<=last; ++k )
		_SplineCharLayerFindBounds(sc,k,bounds);
	}
    }
}

void CIDLayerFindBounds(SplineFont *cidmaster,int layer,DBounds *bounds) {
    SplineFont *sf;
    int i;
    DBounds b;
    real factor;

    if ( cidmaster->cidmaster )
	cidmaster = cidmaster->cidmaster;
    if ( cidmaster->subfonts==NULL ) {
	SplineFontLayerFindBounds(cidmaster,layer,bounds);
return;
    }

    sf = cidmaster->subfonts[0];
    SplineFontLayerFindBounds(sf,layer,bounds);
    factor = 1000.0/(sf->ascent+sf->descent);
    bounds->maxx *= factor; bounds->minx *= factor; bounds->miny *= factor; bounds->maxy *= factor;
    for ( i=1; i<cidmaster->subfontcnt; ++i ) {
	sf = cidmaster->subfonts[i];
	SplineFontLayerFindBounds(sf,layer,&b);
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
	    // Frank added the control points to the calculation since,
	    // according to Adam Twardoch,
	    // the OpenType values that rely upon this function
	    // expect control points to be included.
	    if ( !sp->noprevcp ) {
	      if ( sp->prevcp.y < b->miny ) b->miny = sp->prevcp.y;
	      if ( sp->prevcp.x < b->minx ) b->minx = sp->prevcp.x;
	      if ( sp->prevcp.y > b->maxy ) b->maxy = sp->prevcp.y;
	      if ( sp->prevcp.x > b->maxx ) b->maxx = sp->prevcp.x;
	    }
	    if ( !sp->nonextcp ) {
	      if ( sp->nextcp.y < b->miny ) b->miny = sp->nextcp.y;
	      if ( sp->nextcp.x < b->minx ) b->minx = sp->nextcp.x;
	      if ( sp->nextcp.y > b->maxy ) b->maxy = sp->nextcp.y;
	      if ( sp->nextcp.x > b->maxx ) b->maxx = sp->nextcp.x;
	    }
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
    int i,first, last;
    DBounds temp;
    real e;
    ImageList *img;

    b->minx = b->miny = 1e10;
    b->maxx = b->maxy = -1e10;
    first = last = ly_fore;
    if ( sc->parent!=NULL && sc->parent->multilayer )
	last = sc->layer_cnt-1;
    for ( i=first; i<=last; ++i ) {
	SplineSetQuickBounds(sc->layers[i].splines,&temp);
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
    if ( sc->parent!=NULL && sc->parent->strokedfont &&
	    (b->minx!=b->maxx || b->miny!=b->maxy)) {
	real sw = sc->parent->strokewidth;
	b->minx -= sw; b->miny -= sw;
	b->maxx += sw; b->maxy += sw;
    }
    if ( b->minx>1e9 )
	memset(b,0,sizeof(*b));
}

void SplineCharLayerQuickBounds(SplineChar *sc,int layer,DBounds *bounds) {
    RefChar *ref;
    DBounds temp;

    if ( sc->parent!=NULL && sc->parent->multilayer ) {
	SplineCharQuickBounds(sc,bounds);
return;
    }

    bounds->minx = bounds->miny = 1e10;
    bounds->maxx = bounds->maxy = -1e10;

    SplineSetQuickBounds(sc->layers[layer].splines,bounds);

    for ( ref = sc->layers[layer].refs; ref!=NULL; ref = ref->next ) {
	SplineSetQuickBounds(ref->layers[0].splines,&temp);
	if ( bounds->minx==0 && bounds->maxx==0 && bounds->miny==0 && bounds->maxy == 0 )
	    *bounds = temp;
	else if ( temp.minx!=0 || temp.maxx != 0 || temp.maxy != 0 || temp.miny!=0 ) {
	    if ( temp.minx < bounds->minx ) bounds->minx = temp.minx;
	    if ( temp.miny < bounds->miny ) bounds->miny = temp.miny;
	    if ( temp.maxx > bounds->maxx ) bounds->maxx = temp.maxx;
	    if ( temp.maxy > bounds->maxy ) bounds->maxy = temp.maxy;
	}
    }
    /* a char with no splines (ie. a space) must have an lbearing of 0 */
    if ( bounds->minx>1e9 )
	memset(bounds,0,sizeof(*bounds));
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
    int i, first,last;
    DBounds temp;
    real e;
    ImageList *img;

    memset(b,0,sizeof(*b));
    first = last = ly_fore;
    if ( sc->parent!=NULL && sc->parent->multilayer )
	last = sc->layer_cnt-1;
    for ( i=first; i<=last; ++i ) {
	SplineSetQuickConservativeBounds(sc->layers[i].splines,&temp);
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

void SplinePointCategorize(SplinePoint *sp) {
    int oldpointtype = sp->pointtype;

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
	BasePoint ndir, ncdir, ncunit, pdir, pcdir, pcunit;
	bigreal nlen, nclen, plen, pclen;
	bigreal cross, bounds;

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
	ncunit = ncdir; pcunit = pcdir;
	if ( nclen!=0 ) { ncunit.x /= nclen; ncunit.y /= nclen; }
	if ( pclen!=0 ) { pcunit.x /= pclen; pcunit.y /= pclen; }
	if ( nlen!=0 ) { ndir.x /= nlen; ndir.y /= nlen; }
	if ( plen!=0 ) { pdir.x /= plen; pdir.y /= plen; }

	/* find out which side has the shorter control vector. Cross that vector */
	/*  with the normal of the unit vector on the other side. If the */
	/*  result is less than 1 em-unit then we've got colinear control points */
	/*  (within the resolution of the integer grid) */
	/* Not quite... they could point in the same direction */
        if ( oldpointtype==pt_curve )
            bounds = 4.0;
        else
            bounds = 1.0;
	if ( nclen!=0 && pclen!=0 &&
		((nclen>=pclen && (cross = pcdir.x*ncunit.y - pcdir.y*ncunit.x)<bounds && cross>-bounds ) ||
		 (pclen>nclen && (cross = ncdir.x*pcunit.y - ncdir.y*pcunit.x)<bounds && cross>-bounds )) &&
		 ncdir.x*pcdir.x + ncdir.y*pcdir.y < 0 )
	    sp->pointtype = pt_curve;
	/* Cross product of control point with unit vector normal to line in */
	/*  opposite direction should be less than an em-unit for a tangent */
	else if (( nclen==0 && pclen!=0 && (cross = pcdir.x*ndir.y-pcdir.y*ndir.x)<bounds && cross>-bounds ) ||
		( pclen==0 && nclen!=0 && (cross = ncdir.x*pdir.y-ncdir.y*pdir.x)<bounds && cross>-bounds ))
	    sp->pointtype = pt_tangent;

	/* If a point started out hv, and could still be hv, them make it so */
	/*  but don't make hv points de novo, Alexey doesn't like change */
	/*  (this only works because hv isn't a default setting, so if it's */
	/*   there it was done intentionally) */
	if ( sp->pointtype == pt_curve && oldpointtype == pt_hvcurve &&
		((sp->nextcp.x==sp->me.x && sp->prevcp.x==sp->me.x && sp->nextcp.y!=sp->me.y) ||
		 (sp->nextcp.y==sp->me.y && sp->prevcp.y==sp->me.y && sp->nextcp.x!=sp->me.x)))
	    sp->pointtype = pt_hvcurve;
    }
}

int SplinePointIsACorner(SplinePoint *sp) {
    enum pointtype old = sp->pointtype, new;

    SplinePointCategorize(sp);
    new = sp->pointtype;
    sp->pointtype = old;
return( new==pt_corner );
}

void SPLCategorizePoints(SplinePointList *spl) {
    Spline *spline, *first, *last=NULL;

    for ( ; spl!=NULL; spl = spl->next ) {
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    SplinePointCategorize(spline->from);
	    last = spline;
	    if ( first==NULL ) first = spline;
	}
	if ( spline==NULL && last!=NULL )
	    SplinePointCategorize(last->to);
    }
}

void SPLCategorizePointsKeepCorners(SplinePointList *spl) {
    // It's important when round-tripping U. F. O. data that we keep corners as corners and non-corners as non-corners.
    Spline *spline, *first, *last=NULL;
    int old_type;

    for ( ; spl!=NULL; spl = spl->next ) {
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    // If it is a corner, we leave it as a corner.
	    if ((old_type = spline->from->pointtype) != pt_corner) {
	      SplinePointCategorize(spline->from);
	      // If it was not a corner, we do not let it change to a corner.
	      if (spline->from->pointtype == pt_corner) spline->from->pointtype = old_type;
	    }
	    last = spline;
	    if ( first==NULL ) first = spline;
	}
	if ( spline==NULL && last!=NULL )
	    // If it is a corner, we leave it as a corner.
	    if ((old_type = last->to->pointtype) != pt_corner) {
	      SplinePointCategorize(last->to);
	      // If it was not a corner, we do not let it change to a corner.
	      if (last->to->pointtype == pt_corner) last->to->pointtype = old_type;
	    }
    }
}

void SCCategorizePoints(SplineChar *sc) {
    int i;
    for ( i=ly_fore; i<sc->layer_cnt; ++i )
	SPLCategorizePoints(sc->layers[i].splines);
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
    cur->is_clip_path = spl->is_clip_path;
    cur->spiro_cnt = cur->spiro_max = 0;
    cur->spiros = 0;
    if (spl->contour_name != NULL) cur->contour_name = copy(spl->contour_name);
    for ( pt=spl->first; ;  ) {
	cpt = SplinePointCreate( 0, 0 );
	*cpt = *pt;
	if ( pt->hintmask!=NULL ) {
	    cpt->hintmask = chunkalloc(sizeof(HintMask));
	    memcpy(cpt->hintmask,pt->hintmask,sizeof(HintMask));
	}
	if ( pt->name!=NULL ) {
		cpt->name = copy(pt->name);
	}
	cpt->next = cpt->prev = NULL;
	if ( cur->first==NULL ) {
	    cur->first = cur->last = cpt;
	    cur->start_offset = 0;
	} else {
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
    if ( spl->spiro_cnt!=0 ) {
	cur->spiro_cnt = cur->spiro_max = spl->spiro_cnt;
	cur->spiros = malloc(cur->spiro_cnt*sizeof(spiro_cp));
	memcpy(cur->spiros,spl->spiros,cur->spiro_cnt*sizeof(spiro_cp));
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
	/* If it's a closed contour and the start point is selected then we */
	/*  don't know where that selection began (and we have to keep it with */
	/*  the things that precede it when we make the new splinesets), so */
	/*  loop until we find something unselected */
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
		cpt->name = NULL;
	    cpt->next = cpt->prev = NULL;
	    if ( cur->first==NULL ) {
		cur->first = cur->last = cpt;
		cur->start_offset = 0;
	    } else {
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

/* If this routine is called we are guarenteed that:
    at least one point on the splineset is selected
    not all points on the splineset are selected
*/
static SplinePointList *SplinePointListCopySpiroSelected1(SplinePointList *spl) {
    SplinePointList *head=NULL, *last=NULL, *cur;
    int i,j;
    spiro_cp *list = spl->spiros, *freeme = NULL, *temp = NULL;

    if ( !SPIRO_SPL_OPEN(spl)) {
	/* If it's a closed contour and the start point is selected then we */
	/*  don't know where that selection began (and we have to keep it with */
	/*  the things that precede it when we make the new splinesets), so */
	/*  loop until we find something unselected */
	for ( i=0 ; i<spl->spiro_cnt-1; ++i )
	    if ( !(SPIRO_SELECTED(&list[i])) )
	break;
	if ( i!=0 ) {
	    freeme = malloc(spl->spiro_cnt*sizeof(spiro_cp));
	    memcpy(freeme,list+i,(spl->spiro_cnt-1-i)*sizeof(spiro_cp));
	    memcpy(freeme+(spl->spiro_cnt-1-i),list,i*sizeof(spiro_cp));
	    /* And copy the list terminator */
	    memcpy(freeme+spl->spiro_cnt-1,list+spl->spiro_cnt-1,sizeof(spiro_cp));
	    list = freeme;
	}
    }
    for ( i=0 ; i<spl->spiro_cnt-1; ) {
	/* Skip unselected things */
	for ( ; i<spl->spiro_cnt-1 && !SPIRO_SELECTED(&list[i]); ++i );
	if ( i==spl->spiro_cnt-1 )
    break;
	for ( j=i; j<spl->spiro_cnt-1 && SPIRO_SELECTED(&list[j]); ++j );
	temp = malloc((j-i+2)*sizeof(spiro_cp));
	memcpy(temp,list+i,(j-i)*sizeof(spiro_cp));
	temp[0].ty = SPIRO_OPEN_CONTOUR;
	memset(temp+(j-i),0,sizeof(spiro_cp));
	temp[j-i].ty = SPIRO_END;
	cur = SpiroCP2SplineSet( temp );
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	i = j;
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

SplinePointList *SplinePointListCopySpiroSelected(SplinePointList *base) {
    SplinePointList *head=NULL, *last=NULL, *cur=NULL;
    int anysel, allsel;
    int i;

    for ( ; base!=NULL; base = base->next ) {
	anysel = false; allsel = true;
	for ( i=0; i<base->spiro_cnt-1; ++i ) {
	    if ( SPIRO_SELECTED(&base->spiros[i]) )
		anysel = true;
	    else
		allsel = false;
	}
	if ( allsel )
	    cur = SplinePointListCopy1(base);
	else if ( anysel )
	    cur = SplinePointListCopySpiroSelected1(base);
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

static SplinePointList *SplinePointListSplitSpiros(SplineChar *sc,SplinePointList *spl) {
    SplinePointList *head=NULL, *last=NULL, *cur;
    int i;
    spiro_cp *list = spl->spiros, *freeme = NULL, *temp = NULL;

    if ( !SPIRO_SPL_OPEN(spl)) {
	/* If it's a closed contour and the start point is selected then we */
	/*  don't know where that selection began (and we have to keep it with */
	/*  the things that precede it when we make the new splinesets), so */
	/*  loop until we find something unselected */
	for ( i=0 ; i<spl->spiro_cnt-1; ++i )
	    if ( !(SPIRO_SELECTED(&list[i])) )
	break;
	if ( i!=0 ) {
	    freeme = malloc(spl->spiro_cnt*sizeof(spiro_cp));
	    memcpy(freeme,list+i,(spl->spiro_cnt-1-i)*sizeof(spiro_cp));
	    memcpy(freeme+(spl->spiro_cnt-1-i),list,i*sizeof(spiro_cp));
	    /* And copy the list terminator */
	    memcpy(freeme+spl->spiro_cnt-1,list+spl->spiro_cnt-1,sizeof(spiro_cp));
	    list = freeme;
	}
    }
    for ( i=0 ; i<spl->spiro_cnt-1; ) {
	int start = i;
	/* Retain unselected things */
	for ( ; i<spl->spiro_cnt-1 && !SPIRO_SELECTED(&list[i]); ++i );
	if ( i!=start ) {
	    temp = malloc((i-start+2)*sizeof(spiro_cp));
	    memcpy(temp,list+start,(i-start)*sizeof(spiro_cp));
	    temp[0].ty = SPIRO_OPEN_CONTOUR;
	    memset(temp+(i-start),0,sizeof(spiro_cp));
	    temp[i-start].ty = SPIRO_END;
	    cur = SpiroCP2SplineSet( temp );
	    if ( head==NULL )
		head = cur;
	    else
		last->next = cur;
	    last = cur;
	}
	for ( ; i<spl->spiro_cnt-1 && SPIRO_SELECTED(&list[i]); ++i );
    }
    SplinePointListFree(spl);
return( head );
}

static SplinePointList *SplinePointListSplit(SplineChar *sc,SplinePointList *spl) {
    SplinePointList *head=NULL, *last=NULL, *cur;
    SplinePoint *first, *start, *next;

    start = spl->first;
    if ( spl->first==spl->last ) {
	/* If it's a closed contour and the start point is selected then we */
	/*  don't know where that selection began (and we have to keep it with */
	/*  the things that precede it when we make the new splinesets), so */
	/*  loop until we find something unselected */
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
	    spl->start_offset = 0;
	} else {
	    cur = chunkalloc(sizeof(SplinePointList));
	    last->next = cur;
	}
	last = cur;

	while ( start!=NULL && !start->selected && start!=first ) {
	    if ( cur->first==NULL ) {
		cur->first = start;
		cur->start_offset = 0;
	    }
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
	if ( !sc->inspiro || !hasspiro()) {
	    first = NULL;
	    for ( pt=base->first; pt!=NULL && pt!=first; pt = pt->next->to ) {
		if ( pt->selected ) anysel = true;
		else allsel = false;
		if ( first==NULL ) first = pt;
		if ( pt->next==NULL )
	    break;
	    }
	} else {
	    int i;
	    for ( i=0; i<base->spiro_cnt; ++i ) {
		if ( SPIRO_SELECTED(&base->spiros[i]) )
		    anysel = true;
		else
		    allsel = false;
	    }
	}
	if ( allsel ) {
	    SplinePointListMDFree(sc,base);
    continue;
	}
	if ( !sc->inspiro || !anysel || !hasspiro()) {
	    if ( head==NULL )
		head = base;
	    else
		last->next = base;
	    last = base;
	    if ( anysel )
		last = SplinePointListSplit(sc,base);
	} else {
	    SplineSet *ret;
	    ret = SplinePointListSplitSpiros(sc,base);
	    if ( head==NULL )
		head = ret;
	    else
		last->next = ret;
	    if ( ret!=NULL )
		for ( last=ret; last->next!=NULL; last=last->next );
	}
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

ImageList *ImageListTransform(ImageList *img, real transform[6],int everything) {
    ImageList *head = img;

	/* Don't support rotating, flipping or skewing images */;
    if ( transform[0]!=0 && transform[3]!=0 ) {
	while ( img!=NULL ) {
	    if ( everything || (!everything && img->selected)) {
		bigreal x = img->xoff;
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
	    }
	    img = img->next;
	}
    }
return( head );
}

void BpTransform(BasePoint *to, BasePoint *from, real transform[6]) {
    BasePoint p;
    p.x = transform[0]*from->x + transform[2]*from->y + transform[4];
    p.y = transform[1]*from->x + transform[3]*from->y + transform[5];
    to->x = rint(1024*p.x)/1024;
    to->y = rint(1024*p.y)/1024;
}

void ApTransform(AnchorPoint *ap, real transform[6]) {
    BpTransform(&ap->me,&ap->me,transform);
}

static void TransformPointExtended(SplinePoint *sp, real transform[6], enum transformPointMask tpmask )
{
    /**
     * If we are to transform selected BCP instead of their base splinepoint
     * then lets do that.
     */
    if( tpmask & tpmask_operateOnSelectedBCP
	&& (sp->nextcpselected || sp->prevcpselected ))
    {
	if( sp->nextcpselected )
	{
	    int order2 = sp->next ? sp->next->order2 : 0;
	    BpTransform(&sp->nextcp,&sp->nextcp,transform);
	    SPTouchControl( sp, &sp->nextcp, order2 );
	}
	else if( sp->prevcpselected )
	{
	    int order2 = sp->next ? sp->next->order2 : 0;
	    BpTransform(&sp->prevcp,&sp->prevcp,transform);
	    SPTouchControl( sp, &sp->prevcp, order2 );
	}
    }
    else
    {
	/**
	 * Transform the base splinepoints.
	 */
	BpTransform(&sp->me,&sp->me,transform);

	if ( !sp->nonextcp )
	{
	    BpTransform(&sp->nextcp,&sp->nextcp,transform);
	}
	else
	{
	    sp->nextcp = sp->me;
	}

	if ( !sp->noprevcp )
	{
	    BpTransform(&sp->prevcp,&sp->prevcp,transform);
	}
	else
	{
	    sp->prevcp = sp->me;
	}
    }



    if ( sp->pointtype == pt_hvcurve )
    {
	if(
	    ((sp->nextcp.x==sp->me.x && sp->prevcp.x==sp->me.x && sp->nextcp.y!=sp->me.y) ||
	     (sp->nextcp.y==sp->me.y && sp->prevcp.y==sp->me.y && sp->nextcp.x!=sp->me.x)))
	{
	    /* Do Nothing */;
	}
	else
	{
	    sp->pointtype = pt_curve;
	}
    }
}

static void TransformPoint(SplinePoint *sp, real transform[6])
{
    TransformPointExtended( sp, transform, 0 );
}

static void TransformSpiro(spiro_cp *cp, real transform[6]) {
    bigreal x;

    x = transform[0]*cp->x + transform[2]*cp->y + transform[4];
    cp->y = transform[1]*cp->x + transform[3]*cp->y + transform[5];
    cp->x = x;
}

static void TransformPTsInterpolateCPs(BasePoint *fromorig,Spline *spline,
	BasePoint *toorig,real transform[6] ) {
    BasePoint totrans, temp;
    bigreal fraction;

    /* Normally the "from" point will already have been translated, and the "to" */
    /*  point will need to be. But if we have a closed contour then on the */
    /*  last spline both from and to will have been transform. We can detect */
    /*  this because toorig will be different from &spline->to->me */
    if ( spline->to->selected && toorig==&spline->to->me )
	BpTransform(&totrans,&spline->to->me,transform);
    else
	totrans = spline->to->me;

    /* None of the control points will have been transformed yet */
    if ( fromorig->x!=toorig->x ) {
	fraction = (spline->from->nextcp.x-fromorig->x)/( toorig->x-fromorig->x );
	spline->from->nextcp.x = spline->from->me.x + fraction*( totrans.x-spline->from->me.x );
	fraction = (spline->to->prevcp.x-fromorig->x)/( toorig->x-fromorig->x );
	spline->to->prevcp.x = spline->from->me.x + fraction*( totrans.x-spline->from->me.x );
    } else {
	BpTransform(&temp,&spline->from->nextcp,transform);
	spline->from->nextcp.x = temp.x;
	BpTransform(&temp,&spline->to->prevcp,transform);
	spline->to->prevcp.x = temp.x;
    }
    if ( fromorig->y!=toorig->y ) {
	fraction = (spline->from->nextcp.y-fromorig->y)/( toorig->y-fromorig->y );
	spline->from->nextcp.y = spline->from->me.y + fraction*( totrans.y-spline->from->me.y );
	fraction = (spline->to->prevcp.y-fromorig->y)/( toorig->y-fromorig->y );
	spline->to->prevcp.y = spline->from->me.y + fraction*( totrans.y-spline->from->me.y );
    } else {
	BpTransform(&temp,&spline->from->nextcp,transform);
	spline->from->nextcp.y = temp.y;
	BpTransform(&temp,&spline->to->prevcp,transform);
	spline->to->prevcp.y = temp.y;
    }

    if ( spline->to->selected )
	spline->to->me = totrans;
}


SplinePointList *SplinePointListTransformExtended(SplinePointList *base, real transform[6],
						  enum transformPointType tpt, enum transformPointMask tpmask ) {
    Spline *spline, *first;
    SplinePointList *spl;
    SplinePoint *spt, *pfirst;
    int allsel, anysel, alldone=true;
    BasePoint lastpointorig, firstpointorig, orig;

    for ( spl = base; spl!=NULL; spl = spl->next ) {
	pfirst = NULL; first = NULL;
	allsel = true; anysel=false;
	if ( tpt==tpt_OnlySelectedInterpCPs && spl->first->next!=NULL && !spl->first->next->order2 ) {
	    lastpointorig = firstpointorig = spl->first->me;
	    printf("SplinePointListTransformExtended() spl->first->selected %d\n", spl->first->selected );
	    if ( spl->first->selected ) {
		anysel = true;
		BpTransform(&spl->first->me,&spl->first->me,transform);
	    } else
		allsel = false;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( first==NULL ) first = spline;
		orig = spline->to->me;
		if ( spline->from->selected || spline->to->selected )
		{
		    TransformPTsInterpolateCPs( &lastpointorig, spline,
						spl->first==spline->to? &firstpointorig : &spline->to->me,
						transform );
		}
		lastpointorig = orig;
		if ( spline->to->selected ) anysel = true; else allsel = false;
	    }

	} else {
	    for ( spt = spl->first ; spt!=pfirst; spt = spt->next->to ) {
		if ( pfirst==NULL ) pfirst = spt;
		if ( tpt==tpt_AllPoints || spt->selected ) {
		    TransformPointExtended(spt,transform,tpmask);
		    if ( tpt!=tpt_AllPoints ) {
			if ( spt->next!=NULL && spt->next->order2 && !spt->next->to->selected && spt->next->to->ttfindex==0xffff ) {
			    SplinePoint *to = spt->next->to;
			    to->prevcp = spt->nextcp;
			    to->me.x = (to->prevcp.x+to->nextcp.x)/2;
			    to->me.y = (to->prevcp.y+to->nextcp.y)/2;
			}
			if ( spt->prev!=NULL && spt->prev->order2 && !spt->prev->from->selected && spt->prev->from->ttfindex==0xffff ) {
			    SplinePoint *from = spt->prev->from;
			    from->nextcp = spt->prevcp;
			    from->me.x = (from->prevcp.x+from->nextcp.x)/2;
			    from->me.y = (from->prevcp.y+from->nextcp.y)/2;
			}
		    }
		    anysel = true;
		} else
		    allsel = alldone = false;
		if ( spt->next==NULL )
	    break;
	    }
	}
	if ( !anysel )		/* This splineset had no selected points it's unchanged */
    continue;

	/* If we changed all the points, then transform the spiro version too */
	/*  otherwise if we just changed some points, throw away the spiro */
	if ( allsel ) {
	    int i;
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		TransformSpiro(&spl->spiros[i], transform);
	} else
	    SplineSetSpirosClear(spl);

	/* if we changed all the points then the control points are right */
	/*  otherwise those near the edges may be wonky, fix 'em up */
	/* Figuring out where the edges of the selection are is difficult */
	/*  so let's just tweak all points, it shouldn't matter */
	/* It does matter. Let's tweak all default points */
	if( !(tpmask & tpmask_dontFixControlPoints))
	{
	    if ( tpt!=tpt_AllPoints && !allsel && spl->first->next!=NULL && !spl->first->next->order2 )
	    {
		pfirst = NULL;
		for ( spt = spl->first ; spt!=pfirst; spt = spt->next->to )
		{
		    if ( pfirst==NULL ) pfirst = spt;
		    if ( spt->selected && spt->prev!=NULL && !spt->prev->from->selected &&
			 spt->prev->from->pointtype == pt_tangent )
			SplineCharTangentPrevCP(spt->prev->from);
		    if ( spt->selected && spt->next!=NULL && !spt->next->to->selected &&
			 spt->next->to->pointtype == pt_tangent )
			SplineCharTangentNextCP(spt->next->to);
		    if ( spt->prev!=NULL && spt->prevcpdef && tpt==tpt_OnlySelected )
			SplineCharDefaultPrevCP(spt);
		    if ( spt->next==NULL )
			break;
		    if ( spt->nextcpdef && tpt==tpt_OnlySelected )
			SplineCharDefaultNextCP(spt);
		}
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

SplinePointList *SplinePointListTransform( SplinePointList *base, real transform[6],
					   enum transformPointType tpt )
{
    enum transformPointMask tpmask = 0;
    return SplinePointListTransformExtended( base, transform, tpt, tpmask );
}

SplinePointList *SplinePointListSpiroTransform(SplinePointList *base, real transform[6], int allpoints ) {
    SplinePointList *spl;
    int allsel, anysel;
    int i;


    if ( allpoints )
return( SplinePointListTransform(base,transform,tpt_AllPoints));

    for ( spl = base; spl!=NULL; spl = spl->next ) {
	allsel = true; anysel=false;
	for ( i=0; i<spl->spiro_cnt-1; ++i )
	    if ( spl->spiros[i].ty & 0x80 )
		anysel = true;
	    else
		allsel = false;
	if ( !anysel )
    continue;
	if ( allsel ) {
	    SplinePointList *next = spl->next;
	    /* If we are transforming everything, then we can just transform */
	    /*  the beziers too */
	    spl->next = NULL;
	    SplinePointListTransform(spl,transform,tpt_AllPoints);
	    spl->next = next;
    continue;
	}
	/* If we are transformings some things, then we need to transform the */
	/*  selected spiros and then regenerate the beziers */
	for ( i=0; i<spl->spiro_cnt-1; ++i )
	    if ( spl->spiros[i].ty & 0x80 )
		TransformSpiro(&spl->spiros[i], transform);
	SSRegenerateFromSpiros(spl);
    }
return( base );
}

SplinePointList *SplinePointListShift(SplinePointList *base,real xoff,enum transformPointType allpoints ) {
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

static SplinePointList *_SPLCopyTransformedHintMasks(SplineChar *subsc,int layer,
	real transform[6], SplineChar *basesc ) {
    SplinePointList *spl, *spl2, *head, *last=NULL, *cur, *base;
    SplinePoint *spt, *spt2, *pfirst;
    Spline *s, *first;
    real trans[6];
    RefChar *rf;

    base = subsc->layers[layer].splines;
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
    for ( rf=subsc->layers[layer].refs; rf!=NULL; rf=rf->next ) {
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
	cur = _SPLCopyTransformedHintMasks(rf->sc,layer,trans,basesc);
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
	SplineChar *basesc, BasePoint *trans,int layer ) {
    real transform[6];

    memcpy(transform,r->transform,sizeof(transform));
    transform[4] += trans->x; transform[5] += trans->y;
return( _SPLCopyTransformedHintMasks(r->sc,layer,transform,basesc));
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
	real transform[6], SplineChar *dsc, int layer) {
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
	InstanciateReference(sf,topref,rf,trans,rsc,layer);
    }
    rsc->ticked = false;

    if ( sf->multilayer ) {
	int lbase = topref->layer_cnt;
	if ( topref->layer_cnt==0 ) {
	    topref->layers = calloc(rsc->layer_cnt-1,sizeof(struct reflayer));
	    topref->layer_cnt = rsc->layer_cnt-1;
	} else {
	    topref->layer_cnt += rsc->layer_cnt-1;
	    topref->layers = realloc(topref->layers,topref->layer_cnt*sizeof(struct reflayer));
	    memset(topref->layers+lbase,0,(rsc->layer_cnt-1)*sizeof(struct reflayer));
	}
	for ( i=ly_fore; i<rsc->layer_cnt; ++i ) {
	    topref->layers[i-ly_fore+lbase].splines = SplinePointListTransform(SplinePointListCopy(rsc->layers[i].splines),transform,tpt_AllPoints);
	    BrushCopy(&topref->layers[i-ly_fore+lbase].fill_brush, &rsc->layers[i].fill_brush,transform);
	    PenCopy(&topref->layers[i-ly_fore+lbase].stroke_pen, &rsc->layers[i].stroke_pen,transform);
	    topref->layers[i-ly_fore+lbase].dofill = rsc->layers[i].dofill;
	    topref->layers[i-ly_fore+lbase].dostroke = rsc->layers[i].dostroke;
	    topref->layers[i-ly_fore+lbase].fillfirst = rsc->layers[i].fillfirst;
	    /* ??? and images?  */
	}
    } else {
	if ( topref->layer_cnt==0 ) {
	    topref->layers = calloc(1,sizeof(struct reflayer));
	    topref->layer_cnt = 1;
	}
	new = SplinePointListTransform(SplinePointListCopy(rsc->layers[layer].splines),transform,tpt_AllPoints);
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

    rpt=ret=malloc(strlen(str)+1);
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
    if ( !AllAscii(ret)) {
	rpt = StripToASCII(ret);
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
	ret = malloc(2+20*(i+1));
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

    if ( fd->cidfontname || fd->fontname ) sf->fontname = utf8_verify_copy(fd->cidfontname?fd->cidfontname:fd->fontname);
    sf->display_size = -default_fv_font_size;
    sf->display_antialias = default_fv_antialias;
    if ( fd->fontinfo!=NULL ) {
	if ( sf->fontname==NULL && fd->fontinfo->fullname!=NULL ) {
	    sf->fontname = EnforcePostScriptName(fd->fontinfo->fullname);
	}
	if ( sf->fontname==NULL && fd->fontinfo->familyname!=NULL ){
		sf->fontname = EnforcePostScriptName(fd->fontinfo->familyname);
	}
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
	SplinePointListTransform(sc->layers[ly_fore].splines,trans,tpt_AllPoints);
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
	    SCReinstanciateRefChar(sc,refs,ly_fore);
    }
}

void SFInstanciateRefs(SplineFont *sf) {
    int i, layer;
    RefChar *refs, *next, *pr;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL )
	sf->glyphs[i]->ticked = false;

    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	SplineChar *sc = sf->glyphs[i];

	for ( layer=ly_back; layer<sc->layer_cnt; ++layer ) {
	    for ( pr=NULL, refs = sc->layers[layer].refs; refs!=NULL; refs=next ) {
		next = refs->next;
		sc->ticked = true;
		InstanciateReference(sf, refs, refs, refs->transform,sc,layer);
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
    sf->glyphs = calloc(map->backmax,sizeof(SplineChar *));
    if ( istype3 )	/* We read a type3 */
	sf->multilayer = true;
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
	ff_progress_next();
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
	    SCRefToSplines(sf->glyphs[i],refs,ly_fore);
	}
    }
    /* sometimes (some apple oblique fonts) the fontmatrix is not just a */
    /*  formality, it acutally contains a skew. So be ready */
    if ( fd->fontmatrix[0]!=0 )
	TransByFontMatrix(sf,fd->fontmatrix);
    AltUniFigure(sf,sf->map,true);
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
	ff_post_error(_("Bad Multiple Master Font"),_("Bad Multiple Master Font"));
	SplineFontFree(sf);
return( NULL );
    }

    mm = chunkalloc(sizeof(MMSet));

    pt = fd->weightvector;
    while ( *pt==' ' || *pt=='[' ) ++pt;
    while ( *pt!=']' && *pt!='\0' ) {
	pscontext->blend_values[ pscontext->instance_count ] =
		c_strtod(pt,&end);
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
    mm->instances = malloc(pscontext->instance_count*sizeof(SplineFont *));
    mm->defweights = malloc(mm->instance_count*sizeof(real));
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
	ff_post_error(_("Bad Multiple Master Font"),_("This multiple master font has %1$d instance fonts, but it needs at least %2$d master fonts for %3$d axes. FontForge will not be able to edit this correctly"),mm->instance_count,1<<mm->axis_count,mm->axis_count);
    else if ( mm->instance_count > (1<<mm->axis_count) )
	ff_post_error(_("Bad Multiple Master Font"),_("This multiple master font has %1$d instance fonts, but FontForge can only handle %2$d master fonts for %3$d axes. FontForge will not be able to edit this correctly"),mm->instance_count,1<<mm->axis_count,mm->axis_count);
    mm->positions = calloc(mm->axis_count*mm->instance_count,sizeof(real));
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
			c_strtod(pt,&end);
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

    mm->axismaps = calloc(mm->axis_count,sizeof(struct axismap));
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
		    designs[ppos] = c_strtod(pt,&end);
		    blends[ppos] = c_strtod(end,&end);
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
	    mm->axismaps[apos].blends = malloc(ppos*sizeof(real));
	    mm->axismaps[apos].designs = malloc(ppos*sizeof(real));
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
			bigreal val = c_strtod(pt,NULL);
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
    sf->subfonts = malloc((sf->subfontcnt+1)*sizeof(SplineFont *));
    for ( i=0; i<fd->fdcnt; ++i ) {
	if ( fd->fontmatrix[0]!=0 ) {
	    MatMultiply(fd->fontmatrix,fd->fds[i]->fontmatrix,fd->fds[i]->fontmatrix);
	}
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

    chars = calloc(fd->cidcnt,sizeof(SplineChar *));
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
	chars[i]->altuni = CIDSetAltUnis(map,i);
	/* There better not be any references (seac's) because we have no */
	/*  encoding on which to base any fixups */
	if ( chars[i]->layers[ly_fore].refs!=NULL )
	    IError( "Reference found in CID font. Can't fix it up");
	sf->subfonts[j]->glyphcnt = sf->subfonts[j]->glyphmax = i+1;
	ff_progress_next();
    }
    for ( i=0; i<fd->fdcnt; ++i )
	sf->subfonts[i]->glyphs = calloc(sf->subfonts[i]->glyphcnt,sizeof(SplineChar *));
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

static void LayerToRefLayer(struct reflayer *rl,Layer *layer, real transform[6]) {
    BrushCopy(&rl->fill_brush, &layer->fill_brush,transform);
    PenCopy(&rl->stroke_pen, &layer->stroke_pen,transform);
    rl->dofill = layer->dofill;
    rl->dostroke = layer->dostroke;
    rl->fillfirst = layer->fillfirst;
}

int RefLayerFindBaseLayerIndex(RefChar *rf, int layer) {
	// Note that most of the logic below is copied and lightly modified from SCReinstanciateRefChar.
	SplineChar *rsc = rf->sc;
	int i = 0, j = 0, cnt = 0;
	RefChar *subref;
	for ( i=ly_fore; i<rsc->layer_cnt; ++i ) {
	    if ( rsc->layers[i].splines!=NULL || rsc->layers[i].images!=NULL ) {
	        if (cnt == layer) return i;
		++cnt;
	    }
	    for ( subref=rsc->layers[i].refs; subref!=NULL; subref=subref->next ) {
		for ( j=0; j<subref->layer_cnt; ++j ) if ( subref->layers[j].images!=NULL || subref->layers[j].splines!=NULL ) {
		    if (cnt == layer) return i;
		    ++cnt;
		}
	    }
	}
	return -1;
}

void RefCharFindBounds(RefChar *rf) {
    int i;
    SplineChar *rsc = rf->sc;
    real extra=0,e;

    memset(&rf->bb,'\0',sizeof(rf->bb));
    rf->top.y = -1e10;
    for ( i=0; i<rf->layer_cnt; ++i ) {
	_SplineSetFindBounds(rf->layers[i].splines,&rf->bb);
	_SplineSetFindTop(rf->layers[i].splines,&rf->top);
	int baselayer = RefLayerFindBaseLayerIndex(rf, i);
	if ( baselayer >= 0 && rsc->layers[baselayer].dostroke ) {
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
}

void SCReinstanciateRefChar(SplineChar *sc,RefChar *rf,int layer) {
    SplinePointList *new, *last;
    RefChar *refs;
    int i,j;
    SplineChar *rsc = rf->sc;
    real extra=0,e;

    for ( i=0; i<rf->layer_cnt; ++i ) {
	SplinePointListsFree(rf->layers[i].splines);
	GradientFree(rf->layers[i].fill_brush.gradient);
	PatternFree(rf->layers[i].fill_brush.pattern);
	GradientFree(rf->layers[i].stroke_pen.brush.gradient);
	PatternFree(rf->layers[i].stroke_pen.brush.pattern);
    }
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
	rf->layers = calloc(cnt,sizeof(struct reflayer));
	cnt = 0;
	for ( i=ly_fore; i<rsc->layer_cnt; ++i ) {
	    if ( rsc->layers[i].splines!=NULL || rsc->layers[i].images!=NULL ) {
		rf->layers[cnt].splines =
			SplinePointListTransform(
			 SplinePointListCopy(rsc->layers[i].splines),rf->transform,tpt_AllPoints);
		rf->layers[cnt].images =
			ImageListTransform(
			 ImageListCopy(rsc->layers[i].images),rf->transform,true);
		LayerToRefLayer(&rf->layers[cnt],&rsc->layers[i],rf->transform);
		++cnt;
	    }
	    for ( subref=rsc->layers[i].refs; subref!=NULL; subref=subref->next ) {
		for ( j=0; j<subref->layer_cnt; ++j ) if ( subref->layers[j].images!=NULL || subref->layers[j].splines!=NULL ) {
		    rf->layers[cnt] = subref->layers[j];
		    rf->layers[cnt].splines =
			    SplinePointListTransform(
			     SplinePointListCopy(subref->layers[j].splines),rf->transform,tpt_AllPoints);
		    rf->layers[cnt].images =
			    ImageListTransform(
			     ImageListCopy(subref->layers[j].images),rf->transform,true);
		    ++cnt;
		}
	    }
	}

	memset(&rf->bb,'\0',sizeof(rf->bb));
	rf->top.y = -1e10;
	for ( i=0; i<rf->layer_cnt; ++i ) {
	    _SplineSetFindBounds(rf->layers[i].splines,&rf->bb);
	    _SplineSetFindTop(rf->layers[i].splines,&rf->top);
	    int baselayer = RefLayerFindBaseLayerIndex(rf, i);
	    if ( baselayer >= 0 && rsc->layers[baselayer].dostroke ) {
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
	if ( rf->layer_cnt>0 ) {
	    SplinePointListsFree(rf->layers[0].splines);
	    rf->layers[0].splines = NULL;
	}
	rf->layers = calloc(1,sizeof(struct reflayer));
	rf->layer_cnt = 1;
	rf->layers[0].dofill = true;
	new = SplinePointListTransform(SplinePointListCopy(rf->sc->layers[layer].splines),rf->transform,tpt_AllPoints);
	rf->layers[0].splines = new;
	last = NULL;
	if ( new!=NULL )
	    for ( last = new; last->next!=NULL; last = last->next );
	for ( refs = rf->sc->layers[layer].refs; refs!=NULL; refs = refs->next ) {
	    new = SplinePointListTransform(SplinePointListCopy(refs->layers[0].splines),rf->transform,tpt_AllPoints);
	    if ( last!=NULL )
		last->next = new;
	    else
		rf->layers[0].splines = new;
	    if ( new!=NULL )
		for ( last = new; last->next!=NULL; last = last->next );
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
	for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL && !sf->glyphs[i]->ticked ) {
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
			SCReinstanciateRefChar(sf->glyphs[i],ref,j);
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

void SCReinstanciateRef(SplineChar *sc,SplineChar *rsc,int layer) {
    RefChar *rf;

    for ( rf=sc->layers[layer].refs; rf!=NULL; rf=rf->next ) if ( rf->sc==rsc ) {
	SCReinstanciateRefChar(sc,rf,layer);
    }
}

void SCRemoveDependent(SplineChar *dependent,RefChar *rf,int layer) {
    struct splinecharlist *dlist, *pd;
    RefChar *prev;
    int i;

    if ( dependent->layers[layer].refs==rf )
	dependent->layers[layer].refs = rf->next;
    else {
	for ( prev = dependent->layers[layer].refs; prev->next!=rf; prev=prev->next );
	prev->next = rf->next;
    }
    /* Check for multiple dependencies (colon has two refs to period) */
    /* Also check other layers (they may include references to the same glyph as well */
    /*  if there are none, then remove dependent from ref->sc's dependents list */
    for ( i=0; i<dependent->layer_cnt; i++ ) {
	for ( prev = dependent->layers[i].refs; prev!=NULL && (prev==rf || prev->sc!=rf->sc); prev = prev->next );
    }
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
	SCRemoveDependent(dependent,rf,layer);
    }
    dependent->layers[layer].refs = NULL;
}

void SCRemoveDependents(SplineChar *dependent) {
    int layer;

    for ( layer=ly_fore; layer<dependent->layer_cnt; ++layer )
	SCRemoveLayerDependents(dependent,layer);
}

void SCRefToSplines(SplineChar *sc,RefChar *rf,int layer) {
    SplineSet *spl;
    int rlayer;

    if ( sc->parent->multilayer ) {
	Layer *old = sc->layers;
	sc->layers = realloc(sc->layers,(sc->layer_cnt+rf->layer_cnt)*sizeof(Layer));
	for ( rlayer = 0; rlayer<rf->layer_cnt; ++rlayer ) {
	    LayerDefault(&sc->layers[sc->layer_cnt+rlayer]);
	    sc->layers[sc->layer_cnt+rlayer].splines = rf->layers[rlayer].splines;
	    rf->layers[rlayer].splines = NULL;
	    sc->layers[sc->layer_cnt+rlayer].images = rf->layers[rlayer].images;
	    rf->layers[rlayer].images = NULL;
	    sc->layers[sc->layer_cnt+rlayer].refs = NULL;
	    sc->layers[sc->layer_cnt+rlayer].undoes = NULL;
	    sc->layers[sc->layer_cnt+rlayer].redoes = NULL;
	    BrushCopy(&sc->layers[sc->layer_cnt+rlayer].fill_brush, &rf->layers[rlayer].fill_brush,rf->transform);
	    PenCopy(&sc->layers[sc->layer_cnt+rlayer].stroke_pen, &rf->layers[rlayer].stroke_pen,rf->transform);
	    sc->layers[sc->layer_cnt+rlayer].dofill = rf->layers[rlayer].dofill;
	    sc->layers[sc->layer_cnt+rlayer].dostroke = rf->layers[rlayer].dostroke;
	    sc->layers[sc->layer_cnt+rlayer].fillfirst = rf->layers[rlayer].fillfirst;
	}
	sc->layer_cnt += rf->layer_cnt;
	SCMoreLayers(sc,old);
    } else {
	if ( (spl = rf->layers[0].splines)!=NULL ) {
	    while ( spl->next!=NULL )
		spl = spl->next;
	    spl->next = sc->layers[layer].splines;
	    sc->layers[layer].splines = rf->layers[0].splines;
	    rf->layers[0].splines = NULL;
	    if ( sc->layers[layer].order2 && !sc->layers[layer].background )
		SCClearInstrsOrMark(sc,layer,true);
	}
    }
    SCRemoveDependent(sc,rf,layer);
}

/* This returns all real solutions, even those out of bounds */
/* I use -999999 as an error flag, since we're really only interested in */
/*  solns near 0 and 1 that should be ok. -1 is perhaps a little too close */
/* Sigh. When solutions are near 0, the rounding errors are appalling. */
int _CubicSolve(const Spline1D *sp,bigreal sought, extended ts[3]) {
    extended d, xN, yN, delta2, temp, delta, h, t2, t3, theta;
    extended sa=sp->a, sb=sp->b, sc=sp->c, sd=sp->d-sought;
    int i=0;

    ts[0] = ts[1] = ts[2] = -999999;
    if ( sd==0 && sa!=0 ) {
	/* one of the roots is 0, the other two are the soln of a quadratic */
	ts[0] = 0;
	if ( sc==0 ) {
	    ts[1] = -sb/(extended) sa;	/* two zero roots */
	} else {
	    temp = sb*(extended) sb-4*(extended) sa*sc;
	    if ( RealNear(temp,0))
		ts[1] = -sb/(2*(extended) sa);
	    else if ( temp>=0 ) {
		temp = sqrt(temp);
		ts[1] = (-sb+temp)/(2*(extended) sa);
		ts[2] = (-sb-temp)/(2*(extended) sa);
	    }
	}
    } else if ( sa!=0 ) {
    /* http://www.m-a.org.uk/eb/mg/mg077ch.pdf */
    /* this nifty solution to the cubic neatly avoids complex arithmatic */
	xN = -sb/(3*(extended) sa);
	yN = ((sa*xN + sb)*xN+sc)*xN + sd;

	delta2 = (sb*(extended) sb-3*(extended) sa*sc)/(9*(extended) sa*sa);
	/*if ( RealWithin(delta2,0,.00000001) ) delta2 = 0;*/

	/* the descriminant is yN^2-h^2, but delta might be <0 so avoid using h */
	d = yN*yN - 4*sa*sa*delta2*delta2*delta2;
	if ( ((yN>.01 || yN<-.01) && RealNear(d/yN,0)) || ((yN<=.01 && yN>=-.01) && RealNear(d,0)) )
	    d = 0;
	if ( d>0 ) {
	    temp = sqrt(d);
	    t2 = (-yN-temp)/(2*sa);
	    t2 = (t2==0) ? 0 : (t2<0) ? -pow(-t2,1./3.) : pow(t2,1./3.);
	    t3 = (-yN+temp)/(2*sa);
	    t3 = t3==0 ? 0 : (t3<0) ? -pow(-t3,1./3.) : pow(t3,1./3.);
	    ts[0] = xN + t2 + t3;
	} else if ( d<0 ) {
	    if ( delta2>=0 ) {
		delta = sqrt(delta2);
		h = 2*sa*delta2*delta;
		temp = -yN/h;
		if ( temp>=-1.0001 && temp<=1.0001 ) {
		    if ( temp<-1 ) temp = -1; else if ( temp>1 ) temp = 1;
		    theta = acos(temp)/3;
		    ts[i++] = xN+2*delta*cos(theta);
		    ts[i++] = xN+2*delta*cos(2.0943951+theta);	/* 2*pi/3 */
		    ts[i++] = xN+2*delta*cos(4.1887902+theta);	/* 4*pi/3 */
		}
	    }
	} else if ( /* d==0 && */ delta2!=0 ) {
	    delta = yN/(2*sa);
	    delta = delta==0 ? 0 : delta>0 ? pow(delta,1./3.) : -pow(-delta,1./3.);
	    ts[i++] = xN + delta;	/* this root twice, but that's irrelevant to me */
	    ts[i++] = xN - 2*delta;
	} else if ( /* d==0 && */ delta2==0 ) {
	    if ( xN>=-0.0001 && xN<=1.0001 ) ts[0] = xN;
	}
    } else if ( sb!=0 ) {
	extended d = sc*(extended) sc-4*(extended) sb*sd;
	if ( d<0 && RealNear(d,0)) d=0;
	if ( d<0 )
return(false);		/* All roots imaginary */
	d = sqrt(d);
	ts[0] = (-sc-d)/(2*(extended) sb);
	ts[1] = (-sc+d)/(2*(extended) sb);
    } else if ( sc!=0 ) {
	ts[0] = -sd/(extended) sc;
    } else {
	/* If it's a point then either everything is a solution, or nothing */
    }
return( ts[0]!=-999999 );
}

int CubicSolve(const Spline1D *sp,bigreal sought, extended ts[3]) {
    extended t;
    extended ts2[3];
    int i,j;
    /* This routine gives us all solutions between [0,1] with -1 as an error flag */
    /* http://mathforum.org/dr.math/faq/faq.cubic.equations.html */

    ts[0] = ts[1] = ts[2] = -1;
    if ( !_CubicSolve(sp,sought,ts2)) {
return( false );
    }

    for ( i=j=0; i<3; ++i ) {
	if ( ts2[i]>-.0001 && ts2[i]<1.0001 ) {
	    if ( ts2[i]<0 ) ts[j++] = 0;
	    else if ( ts2[i]>1 ) ts[j++] = 1;
	    else
		ts[j++] = ts2[i];
	}
    }
    if ( j==0 )
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
	ts[3] = -999999;
return( _CubicSolve(&sp,0,ts));
    } else if ( q->e==0 ) {	/* we can factor out a zero root */
	sp.a = q->a;
	sp.b = q->b;
	sp.c = q->c;
	sp.d = q->d;
	ts[0] = 0;
return( _CubicSolve(&sp,0,ts+1)+1);
    }

    sp.a = 4*q->a;
    sp.b = 3*q->b;
    sp.c = 2*q->c;
    sp.d = q->d;
    if ( _CubicSolve(&sp,0,extrema)) {
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
	for (;;) {
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

extended SplineSolve(const Spline1D *sp, real tmin, real tmax, extended sought) {
    /* We want to find t so that spline(t) = sought */
    /*  the curve must be monotonic */
    /* returns t which is near sought or -1 */
    extended ts[3];
    int i;
    extended t;

    CubicSolve(sp,sought,ts);
    if ( tmax<tmin ) { t = tmax; tmax = tmin; tmin = t; }
    for ( i=0; i<3; ++i )
	if ( ts[i]>=tmin && ts[i]<=tmax )
return( ts[i] );

return( -1 );
}

/* An IEEE double has 52 bits of precision. So one unit of rounding error will be */
/*  the number divided by 2^51 */
# define D_RE_Factor	(1024.0*1024.0*1024.0*1024.0*1024.0*2.0)
/* But that's not going to work near 0, so, since the t values we care about */
/*  are [0,1], let's use 1.0/D_RE_Factor */

extended SplineSolveFixup(const Spline1D *sp, real tmin, real tmax, extended sought) {
    extended ts[3];
    int i;
    bigreal factor;
    extended t;
    extended val, valp, valm;

    CubicSolve(sp,sought,ts);
    if ( tmax<tmin ) { t = tmax; tmax = tmin; tmin = t; }
    for ( i=0; i<3; ++i )
	if ( ts[i]>=tmin && ts[i]<=tmax )
    break;
    if ( i==3 ) {
	/* nothing in range, but ... */
	/* did a rounding error take a solution just outside the bounds? */
	extended bestd = .0001; int besti = -1;
	extended off;
	for ( i=0; i<3 && ts[i]!=-1; ++i ) {
	    if ( ts[i]<tmin )
		off = tmin-ts[i];
	    else
		off = ts[i]-tmax;
	    if ( off<bestd ) {
		bestd = off;
		besti = i;
	    }
	}
	if ( besti==-1 )
return( -1 );
	i = besti;
    }
    t = ts[i];

    if ((val = (((sp->a*t+sp->b)*t+sp->c)*t+sp->d) - sought)<0 )
	val=-val;
    if ( val!=0 ) {
	for ( factor=1024.0*1024.0*1024.0*1024.0*1024.0; factor>.5; factor/=2.0 ) {
	    extended tp = t + (factor*t)/D_RE_Factor;
	    extended tm = t - (factor*t)/D_RE_Factor;
	    if ( (valp = (((sp->a*tp+sp->b)*tp+sp->c)*tp+sp->d) - sought)<0 )
		valp = -valp;
	    if ( (valm = (((sp->a*tm+sp->b)*tm+sp->c)*tm+sp->d) - sought)<0 )
		valm = -valm;
	    if ( valp<val && valp<valm ) {
		if ( factor==1024.0*1024.0*1024.0*1024*1024 ) {
		    bigreal it = IterateSplineSolve(sp,tmin,tmax,sought);
		    printf( "Used %g: orig-t: %g, new-t: %g iter-t: %g\n", (double) factor, (double) t, (double) tp, (double) it );
		}
		t = tp;
		val = valp;
	    } else if ( valm<val ) {
		if ( factor==1024.0*1024.0*1024.0*1024*1024 ) {
		    bigreal it = IterateSplineSolve(sp,tmin,tmax,sought);
		    printf( "Used -%g: orig-t: %g, new-t: %g iter-t: %g\n", (double) factor, (double) t, (double) tm, (double) it );
		}
		t = tm;
		val = valm;
	    }
	}
    }
    if ( t>=tmin && t<=tmax )
return( t );

return( -1 );
}

extended IterateSplineSolve(const Spline1D *sp, extended tmin, extended tmax,
	extended sought) {
    extended t, low, high, test;
    Spline1D temp;
    /* Now the closed form CubicSolver can have rounding errors so if we know */
    /*  the spline to be monotonic, an iterative approach is more accurate */

    if ( tmin>tmax ) {
	t=tmin; tmin=tmax; tmax=t;
    }

    temp = *sp;
    temp.d -= sought;

    if ( temp.a==0 && temp.b==0 && temp.c!=0 ) {
	t = -temp.d/(extended) temp.c;
	if ( t<tmin || t>tmax )
return( -1 );
return( t );
    }

    low = ((temp.a*tmin+temp.b)*tmin+temp.c)*tmin+temp.d;
    high = ((temp.a*tmax+temp.b)*tmax+temp.c)*tmax+temp.d;
    if ( low==0 )
return(tmin);
    if ( high==0 )
return(tmax);
    if (( low<0 && high>0 ) ||
	    ( low>0 && high<0 )) {

	for (;;) {
	    t = (tmax+tmin)/2;
	    if ( t==tmax || t==tmin )
return( t );
	    test = ((temp.a*t+temp.b)*t+temp.c)*t+temp.d;
	    if ( test==0 )	/* someone complained that this test relied on exact arithmetic. In fact this test will almost never be hit, the real exit test is the line above, when tmin/tmax are so close that there is no space between them in the floating representation */
return( t );
	    if ( (low<0 && test<0) || (low>0 && test>0) )
		tmin=t;
	    else
		tmax = t;
	}
    } else if ( low<.0001 && low>-.0001 )
return( tmin );			/* Rounding errors */
    else if ( high<.0001 && high>-.0001 )
return( tmax );

return( -1 );
}

extended IterateSplineSolveFixup(const Spline1D *sp, extended tmin, extended tmax,
	extended sought) {
    // Search between tmin and tmax for a t-value at which the spline outputs sought.
    extended t;
    bigreal factor;
    extended val, valp, valm;

    if ( tmin>tmax ) {
	t=tmin; tmin=tmax; tmax=t;
    }

    t = IterateSplineSolve(sp,tmin,tmax,sought);

    if ( t==-1 )
return( -1 );

    if ((val = (((sp->a*t+sp->b)*t+sp->c)*t+sp->d) - sought)<0 )
	val=-val;
    if ( val!=0 ) {
	for ( factor=1024.0*1024.0*1024.0*1024.0*1024.0; factor>.5; factor/=2.0 ) {
	    extended tp = t + (factor*t)/D_RE_Factor;
	    extended tm = t - (factor*t)/D_RE_Factor;
	    if ( tp>tmax ) tp=tmax;
	    if ( tm<tmin ) tm=tmin;
	    if ( (valp = (((sp->a*tp+sp->b)*tp+sp->c)*tp+sp->d) - sought)<0 )
		valp = -valp;
	    if ( (valm = (((sp->a*tm+sp->b)*tm+sp->c)*tm+sp->d) - sought)<0 )
		valm = -valm;
	    if ( valp<val && valp<valm ) {
		t = tp;
		val = valp;
	    } else if ( valm<val ) {
		t = tm;
		val = valm;
	    }
	}
    }
    if ( t==0 && !Within16RoundingErrors(sought,sought+val))
return( -1 );
    /* if t!=0 then we we get the chance of far worse rounding errors */
    else if ( t==tmax || t==tmin ) {
	if ( Within16RoundingErrors(sought,sought+val) ||
		Within16RoundingErrors(sp->a,sp->a+val) ||
		Within16RoundingErrors(sp->b,sp->b+val) ||
		Within16RoundingErrors(sp->c,sp->c+val) ||
		Within16RoundingErrors(sp->c,sp->c+val) ||
		Within16RoundingErrors(sp->d,sp->d+val))
return( t );
	else
return( -1 );
    }

    if ( t>=tmin && t<=tmax )
return( t );

    /* I don't think this can happen... */
return( -1 );
}

double CheckExtremaForSingleBitErrors(const Spline1D *sp, double t, double othert) {
    double u1, um1;
    double slope, slope1, slopem1;
    int err;
    double diff, factor;

    if ( t<0 || t>1 )
return( t );

    factor = t*0x40000/D_RE_Factor;
    if ( (diff = t-othert)<0 ) diff= -diff;
    if ( factor>diff/4 && diff!=0 )		/* This little check is to insure we don't skip beyond the well of this extremum into the next */
	factor = diff/4;

    slope = (3*(double) sp->a*t+2*sp->b)*t+sp->c;
    if ( slope<0 ) slope = -slope;

    for ( err = 0x40000; err!=0; err>>=1 ) {
	u1 = t+factor;
	slope1 = (3*(double) sp->a*u1+2*sp->b)*u1+sp->c;
	if ( slope1<0 ) slope1 = -slope1;

	um1 = t-factor;
	slopem1 = (3*(double) sp->a*um1+2*sp->b)*um1+sp->c;
	if ( slopem1<0 ) slopem1 = -slopem1;

	if ( slope1<slope && slope1<=slopem1 && u1<=1.0 ) {
	    t = u1;
	} else if ( slopem1<slope && slopem1<=slope1 && um1>=0.0 ) {
	    t = um1;
	}
	factor /= 2.0;
    }
    /* that seems as good as it gets */

return( t );
}

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
	    b2_fourac = sqrt(b2_fourac);
	    t1 = (-2*sp->b - b2_fourac) / (6*sp->a);
	    t2 = (-2*sp->b + b2_fourac) / (6*sp->a);
	    t1 = CheckExtremaForSingleBitErrors(sp,t1,t2);
	    t2 = CheckExtremaForSingleBitErrors(sp,t2,t1);
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
	    b2_fourac = sqrt(b2_fourac);
	    t1 = (-2*sp->b - b2_fourac) / (6*sp->a);
	    t2 = (-2*sp->b + b2_fourac) / (6*sp->a);
	    t1 = CheckExtremaForSingleBitErrors(sp,t1,t2);
	    t2 = CheckExtremaForSingleBitErrors(sp,t2,t1);
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

bigreal SplineCurvature(Spline *s, bigreal t) {
	/* Kappa = (x'y'' - y'x'') / (x'^2 + y'^2)^(3/2) */
    bigreal dxdt, dydt, d2xdt2, d2ydt2, denom, numer;

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

int SplineAtInflection(Spline1D *sp, bigreal t ) {
    /* It's a point of inflection if d sp/dt==0 and d2 sp/dt^2==0 */
return ( RealNear( (3*sp->a*t + 2*sp->b)*t + sp->c,0) &&
	    RealNear( 6*sp->a*t + 2*sp->b, 0));
}

int SplineAtMinMax(Spline1D *sp, bigreal t ) {
    /* It's a point of inflection if d sp/dt==0 and d2 sp/dt^2!=0 */
return ( RealNear( (3*sp->a*t + 2*sp->b)*t + sp->c,0) &&
	    !RealNear( 6*sp->a*t + 2*sp->b, 0));
}

int Spline2DFindExtrema(const Spline *sp, extended extrema[4] ) {
    int i,j;
    BasePoint last, cur, mid;

    /* If the control points are at the end-points then this (1D) spline is */
    /*  basically a line. But rounding errors can give us very faint extrema */
    /*  if we look for them */
    if ( !Spline1DCantExtremeX(sp) )
	SplineFindExtrema(&sp->splines[0],&extrema[0],&extrema[1]);
    else
	extrema[0] = extrema[1] = -1;
    if ( !Spline1DCantExtremeY(sp) )
	SplineFindExtrema(&sp->splines[1],&extrema[2],&extrema[3]);
    else
	extrema[2] = extrema[3] = -1;

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
	    for ( j=i; j<3; ++j )
		extrema[j] = extrema[j+1];
	    extrema[3] = -1;
	    --i;
	} else
	    last = cur;
    }
    if ( extrema[0]!=-1 ) {
	mid.x = (last.x+sp->to->me.x)/2; mid.y = (last.y+sp->to->me.y)/2;
	if ( (mid.x==last.x || mid.x==cur.x) &&
		(mid.y==last.y || mid.y==cur.y))
	    extrema[i-1] = -1;
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
	b2_fourac = sqrt( b2_fourac );
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
    if ( cnt<2 )
	poi[cnt] = -1;

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
	else {
	    /* Well we should just remove the whole spline? */
	    ;
	}
    }
    *_t1 = t1; *_t2 = t2;
}

int IntersectLines(BasePoint *inter,
	BasePoint *line1_1, BasePoint *line1_2,
	BasePoint *line2_1, BasePoint *line2_2) {
    // A lot of functions call this with the same address as an input and the output.
    // In order to avoid unexpected behavior, we delay writing to the output until the end.
    bigreal s1, s2;
    BasePoint _output;
    BasePoint * output = &_output;
    if ( line1_1->x == line1_2->x ) {
        // Line 1 is vertical.
	output->x = line1_1->x;
	if ( line2_1->x == line2_2->x ) {
            // Line 2 is vertical.
	    if ( line2_1->x!=line1_1->x )
              return( false );		/* Parallel vertical lines */
	    output->y = (line1_1->y+line2_1->y)/2;
	} else {
	    output->y = line2_1->y + (output->x-line2_1->x) * (line2_2->y - line2_1->y)/(line2_2->x - line2_1->x);
        }
        *inter = *output;
        return( true );
    } else if ( line2_1->x == line2_2->x ) {
        // Line 2 is vertical, but we know that line 1 is not.
	output->x = line2_1->x;
	output->y = line1_1->y + (output->x-line1_1->x) * (line1_2->y - line1_1->y)/(line1_2->x - line1_1->x);
        *inter = *output;
        return( true );
    } else {
        // Both lines are oblique.
	s1 = (line1_2->y - line1_1->y)/(line1_2->x - line1_1->x);
	s2 = (line2_2->y - line2_1->y)/(line2_2->x - line2_1->x);
	if ( RealNear(s1,s2)) {
	    if ( !RealNear(line1_1->y + (line2_1->x-line1_1->x) * s1,line2_1->y))
              return( false );
	    output->x = (line1_2->x+line2_2->x)/2;
	    output->y = (line1_2->y+line2_2->y)/2;
	} else {
	    output->x = (s1*line1_1->x - s2*line2_1->x - line1_1->y + line2_1->y)/(s1-s2);
	    output->y = line1_1->y + (output->x-line1_1->x) * s1;
	}
        *inter = *output;
        return( true );
    }
}

int IntersectLinesClip(BasePoint *inter,
	BasePoint *line1_1, BasePoint *line1_2,
	BasePoint *line2_1, BasePoint *line2_2) {
    BasePoint old = *inter, unit;
    bigreal len, val;

    if ( !IntersectLines(inter,line1_1,line1_2,line2_1,line2_2))
return( false );
    else {
	unit.x = line2_2->x-line1_2->x;
	unit.y = line2_2->y-line1_2->y;
	len = sqrt(unit.x*unit.x + unit.y*unit.y);
	if ( len==0 )
return( false );
	else {
	    unit.x /= len; unit.y /= len;
	    val = unit.x*(inter->x-line1_2->x) + unit.y*(inter->y-line1_2->y);
	    if ( val<=0 || val>=len ) {
		*inter = old;
return( false );
	    }
	}
    }
return( true );
}

int IntersectLinesSlopes(BasePoint *inter,
	BasePoint *line1, BasePoint *slope1,
	BasePoint *line2, BasePoint *slope2) {
    bigreal denom = slope1->y*(bigreal) slope2->x - slope1->x*(bigreal) slope2->y;
    bigreal x,y;

    if ( denom == 0 )
return( false );			/* Lines are colinear, no intersection */
    if ( line1->x==line2->x && line1->y==line2->y ) {
	*inter = *line1;
return( true );
    }

    x = (slope1->y*(bigreal) slope2->x*line1->x -
	    slope2->y*(bigreal) slope1->x*line2->x +
	    slope2->x*(bigreal) slope1->x*(line2->y - line1->y)) / denom;
    if ( slope1->x==0 )
	y = slope2->y*(x-line2->x)/slope2->x + line2->y;
    else
	y = slope1->y*(x-line1->x)/slope1->x + line1->y;
    if ( x<-16000 || x>16000 || y<-16000 || y>16000 )
return( false );			/* Effectively parallel */

    inter->x = x;
    inter->y = y;
return( true );
}

static int AddPoint(extended x,extended y,extended t,extended s,BasePoint *pts,
	extended t1s[3],extended t2s[3], int soln) {
    int i;

    for ( i=0; i<soln; ++i )
	if ( x==pts[i].x && y==pts[i].y )
return( soln );
    if ( soln>=9 )
	IError( "Too many solutions!\n" );
    t1s[soln] = t;
    t2s[soln] = s;
    pts[soln].x = x;
    pts[soln].y = y;
return( soln+1 );
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
	    ts[j] = IterateSplineSolve(sp,e[i],e[i+1],0);
	    if ( ts[j]!=-1 ) ++j;
	    if ( e[i+1]==1.0 )
	break;
	}
    } else if ( sp->b!=0 ) {
	extended b2_4ac = sp->c*(extended) sp->c - 4*sp->b*(extended) sp->d;
	if ( b2_4ac>=0 ) {
	    b2_4ac = sqrt(b2_4ac);
	    ts[0] = (-sp->c-b2_4ac)/(2*sp->b);
	    ts[1] = (-sp->c+b2_4ac)/(2*sp->b);
	    if ( ts[0]>ts[1] ) { bigreal t = ts[0]; ts[0] = ts[1]; ts[1] = t; }
	}
    } else if ( sp->c!=0 ) {
	ts[0] = -sp->d/(extended) sp->c;
    } else {
	/* No solutions, or all solutions */
	;
    }

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

static extended ISolveWithin(const Spline *spline,int major,
	extended val,extended tlow, extended thigh) {
    Spline1D temp;
    extended ts[3];
    const Spline1D *sp = &spline->splines[major];
    int i;

    /* Calculation for t=1 can yield rounding errors. Insist on the endpoints */
    /*  (the Spline1D is not a perfectly accurate description of the spline,  */
    /*   but the control points are right -- at least that's my defn.) */
    if ( tlow==0 && val==(&spline->from->me.x)[major] )
return( 0 );
    if ( thigh==1.0 && val==(&spline->to->me.x)[major] )
return( 1.0 );

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
	const Spline *s1,const Spline *s2,extended t1,extended t2, int maxcnt) {

    if ( cnt>=maxcnt )
return( cnt );

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
	const Spline *s1,const Spline *s2,extended t1low,extended t1high,extended t2low,extended t2high,
	int maxcnt) {
    int major;
    extended t1, t2;
    extended o1o, o2o, o1n, o2n, m;

    major = !other;
    o1o = ((s1->splines[other].a*t1low+s1->splines[other].b)*t1low+
		    s1->splines[other].c)*t1low+s1->splines[other].d;
    o2o = ((s2->splines[other].a*t2low+s2->splines[other].b)*t2low+
		    s2->splines[other].c)*t2low+s2->splines[other].d;
    for (;;) {
	t1 = (t1low+t1high)/2;
	m = ((s1->splines[major].a*t1+s1->splines[major].b)*t1+
			s1->splines[major].c)*t1+s1->splines[major].d;
	t2 = ISolveWithin(s2,major,m,t2low,t2high);
	if ( t2==-1 )
return( cnt );

	o1n = ((s1->splines[other].a*t1+s1->splines[other].b)*t1+
			s1->splines[other].c)*t1+s1->splines[other].d;
	o2n = ((s2->splines[other].a*t2+s2->splines[other].b)*t2+
			s2->splines[other].c)*t2+s2->splines[other].d;
	if (( o1n-o2n<.001 && o1n-o2n>-.001) ||
		(t1-t1low<.0001 && t1-t1low>-.0001))
return( ICAddInter(cnt,foundpos,foundt1,foundt2,s1,s2,t1,t2,maxcnt));
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
			    BasePoint *foundpos,extended *foundt1,extended *foundt2,
			    int maxcnt) {
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

    t1max = ISolveWithin(s1,major,(&max.x)[major],lowt1,hight1);
    t1min = ISolveWithin(s1,major,(&min.x)[major],lowt1,hight1);
    t2max = ISolveWithin(s2,major,(&max.x)[major],lowt2,hight2);
    t2min = ISolveWithin(s2,major,(&min.x)[major],lowt2,hight2);
    if ( t1max==-1 || t1min==-1 || t2max==-1 || t2min==-1 )
return( 0 );
    t1diff = (t1max-t1min)/64.0;
    if (RealNear(t1diff,0))
return( 0 );

    t1 = t1min; t2 = t2min;
    o1o = t1==0   ? (&s1->from->me.x)[other] :
	  t1==1.0 ? (&s1->to->me.x)[other] :
	    ((s1->splines[other].a*t1+s1->splines[other].b)*t1+
		    s1->splines[other].c)*t1+s1->splines[other].d;
    o2o = t2==0   ? (&s2->from->me.x)[other] :
	  t2==1.0 ? (&s2->to->me.x)[other] :
	    ((s2->splines[other].a*t2+s2->splines[other].b)*t2+
		    s2->splines[other].c)*t2+s2->splines[other].d;
    if ( o1o==o2o )
	cnt = ICAddInter(cnt,foundpos,foundt1,foundt2,s1,s2,t1,t2,maxcnt);
    for (;;) {
	if ( cnt>=maxcnt )
    break;
	t1 += t1diff;
	if (( t1max>t1min && t1>t1max ) || (t1max<t1min && t1<t1max) || cnt>3 )
    break;
	m =   t1==0   ? (&s1->from->me.x)[major] :
	      t1==1.0 ? (&s1->to->me.x)[major] :
		((s1->splines[major].a*t1+s1->splines[major].b)*t1+
			s1->splines[major].c)*t1+s1->splines[major].d;
	oldt2 = t2;
	t2 = ISolveWithin(s2,major,m,lowt2,hight2);
	if ( t2==-1 )
    continue;

	o1n = t1==0   ? (&s1->from->me.x)[other] :
	      t1==1.0 ? (&s1->to->me.x)[other] :
		((s1->splines[other].a*t1+s1->splines[other].b)*t1+
			s1->splines[other].c)*t1+s1->splines[other].d;
	o2n = t2==0   ? (&s2->from->me.x)[other] :
	      t2==1.0 ? (&s2->to->me.x)[other] :
		((s2->splines[other].a*t2+s2->splines[other].b)*t2+
			s2->splines[other].c)*t2+s2->splines[other].d;
	if ( o1n==o2n )
	    cnt = ICAddInter(cnt,foundpos,foundt1,foundt2,s1,s2,t1,t2,maxcnt);
	if ( (o1o>o2o && o1n<o2n) || (o1o<o2o && o1n>o2n))
	    cnt = ICBinarySearch(cnt,foundpos,foundt1,foundt2,other,
		    s1,s2,t1-t1diff,t1,oldt2,t2,maxcnt);
	o1o = o1n; o2o = o2n;
    }
return( cnt );
}

static int Closer(const Spline *s1,const Spline *s2,extended t1,extended t2,extended t1p,extended t2p) {
    bigreal x1 = ((s1->splines[0].a*t1+s1->splines[0].b)*t1+s1->splines[0].c)*t1+s1->splines[0].d;
    bigreal y1 = ((s1->splines[1].a*t1+s1->splines[1].b)*t1+s1->splines[1].c)*t1+s1->splines[1].d;
    bigreal x2 = ((s2->splines[0].a*t2+s2->splines[0].b)*t2+s2->splines[0].c)*t2+s2->splines[0].d;
    bigreal y2 = ((s2->splines[1].a*t2+s2->splines[1].b)*t2+s2->splines[1].c)*t2+s2->splines[1].d;
    bigreal diff = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
    bigreal x1p = ((s1->splines[0].a*t1p+s1->splines[0].b)*t1p+s1->splines[0].c)*t1p+s1->splines[0].d;
    bigreal y1p = ((s1->splines[1].a*t1p+s1->splines[1].b)*t1p+s1->splines[1].c)*t1p+s1->splines[1].d;
    bigreal x2p = ((s2->splines[0].a*t2p+s2->splines[0].b)*t2p+s2->splines[0].c)*t2p+s2->splines[0].d;
    bigreal y2p = ((s2->splines[1].a*t2p+s2->splines[1].b)*t2p+s2->splines[1].c)*t2p+s2->splines[1].d;
    bigreal diffp = (x1p-x2p)*(x1p-x2p) + (y1p-y2p)*(y1p-y2p);

    if ( diff<diffp )
return( false );

return( true );
}

/* returns 0=>no intersection, 1=>at least one, location in pts, t1s, t2s */
/*  -1 => We couldn't figure it out in a closed form, have to do a numerical */
/*  approximation */
int SplinesIntersect(const Spline *s1, const Spline *s2, BasePoint pts[9],
	extended t1s[10], extended t2s[10]) {	/* One extra for a trailing -1 */
    BasePoint min1, max1, min2, max2;
    int soln = 0;
    extended x,y,t, ac0, ac1;
    int i,j,found;
    Spline1D spline;
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

    if ( s1->knownlinear ) {
	spline.d = s1->splines[1].c*((bigreal) s2->splines[0].d-(bigreal) s1->splines[0].d)-
		s1->splines[0].c*((bigreal) s2->splines[1].d-(bigreal) s1->splines[1].d);
	spline.c = s1->splines[1].c*(bigreal) s2->splines[0].c - s1->splines[0].c*(bigreal) s2->splines[1].c;
	spline.b = s1->splines[1].c*(bigreal) s2->splines[0].b - s1->splines[0].c*(bigreal) s2->splines[1].b;
	spline.a = s1->splines[1].c*(bigreal) s2->splines[0].a - s1->splines[0].c*(bigreal) s2->splines[1].a;
	IterateSolve(&spline,tempts);
	if ( tempts[0]==-1 )
return( false );
	for ( i = 0; i<3 && tempts[i]!=-1; ++i ) {
	    x = ((s2->splines[0].a*tempts[i]+s2->splines[0].b)*tempts[i]+
		    s2->splines[0].c)*tempts[i]+s2->splines[0].d;
	    y = ((s2->splines[1].a*tempts[i]+s2->splines[1].b)*tempts[i]+
		    s2->splines[1].c)*tempts[i]+s2->splines[1].d;
	    if ( s1->splines[0].c==0 )
		x = s1->splines[0].d;
	    if ( s1->splines[1].c==0 )
		y = s1->splines[1].d;
	    if ( (ac0 = s1->splines[0].c)<0 ) ac0 = -ac0;
	    if ( (ac1 = s1->splines[1].c)<0 ) ac1 = -ac1;
	    if ( ac0>ac1 )
		t = (x-s1->splines[0].d)/s1->splines[0].c;
	    else
		t = (y-s1->splines[1].d)/s1->splines[1].c;
	    if ( tempts[i]>.99996 && Closer(s1,s2,t,tempts[i],t,1)) {
		tempts[i] = 1;
		x = s2->to->me.x; y = s2->to->me.y;
	    } else if ( tempts[i]<.00001 && Closer(s1,s2,t,tempts[i],t,0)) {
		tempts[i] = 0;
		x = s2->from->me.x; y = s2->from->me.y;
	    }
	    /* I know we just did this, but we might have changed x,y so redo */
	    if ( ac0>ac1 )
		t = (x-s1->splines[0].d)/s1->splines[0].c;
	    else
		t = (y-s1->splines[1].d)/s1->splines[1].c;
	    if ( t>.99996 && t<1.001 && Closer(s1,s2,t,tempts[i],1,tempts[i])) {
		t = 1;
		x = s1->to->me.x; y = s1->to->me.y;
	    } else if ( t<.00001 && t>-.001 && Closer(s1,s2,t,tempts[i],0,tempts[i])) {
		t = 0;
		x = s1->from->me.x; y = s1->from->me.y;
	    }
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
				    &pts[found],&t1s[found],&t2s[found],9-found);
	    else {
		int k,l;
		int cnt = CubicsIntersect(s1,extrema1[i],extrema1[i+1],&min1,&max1,
				    s2,extrema2[j],extrema2[j+1],&min2,&max2,
				    &pts[found],&t1s[found],&t2s[found],9-found);
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
	    if ( found>=8 ) {
		/* If the splines are colinear then we might get an unbounded */
		/*  number of intersections */
	break;
	    }
	}
    }
    t1s[found] = t2s[found] = -1;
return( found!=0 );
}

/* Puts all the contours of the glyph together. Next routine undoes this */
SplineSet *LayerAllSplines(Layer *layer) {
    SplineSet *head=NULL, *last=NULL;
    RefChar *r;

    head = layer->splines;
    if ( head!=NULL )
	for ( last=head; last->next!=NULL; last = last->next );
    for ( r=layer->refs; r!=NULL; r=r->next ) {
	if ( last!=NULL ) {
	    last->next = r->layers[0].splines;
	    for ( ; last->next!=NULL; last=last->next );
	} else {
	    head = r->layers[0].splines;
	    if ( head!=NULL )
		for ( last=head; last->next!=NULL; last = last->next );
	}
    }
return( head );
}

SplineSet *LayerUnAllSplines(Layer *layer) {
    SplineSet *spl = layer->splines;
    RefChar *r = layer->refs;

    if ( spl==NULL ) {
	while ( r!=NULL && r->layers[0].splines==NULL )
	    r = r->next;
	if ( r==NULL )
return( NULL );
	spl = r->layers[0].splines;
	do { r = r->next; } while ( r!=NULL && r->layers[0].splines==NULL );
    }
    while ( r!=NULL ) {
	while ( spl!=NULL && spl->next!=r->layers[0].splines )
	    spl = spl->next;
	spl->next = NULL;
	spl = r->layers[0].splines;
	do { r = r->next; } while ( r!=NULL && r->layers[0].splines==NULL );
    }
return( layer->splines );
}

int SplineSetIntersect(SplineSet *spl, Spline **_spline, Spline **_spline2) {
    BasePoint pts[9];
    extended t1s[10], t2s[10];
    int found = false,i;
    SplineSet *test, *test2;
    Spline *spline, *spline2, *first, *first2;

    for ( test=spl; test!=NULL ; test=test->next ) {
	first = NULL;
	for ( spline = test->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( first==NULL ) first = spline;
	    for ( test2=test; test2!=NULL; test2=test2->next ) {
		first2 = test2==test && first!=spline ? first : NULL;
		for ( spline2=(test2==test)?spline : test2->first->next;
			spline2!=NULL && spline2!=first2; spline2 = spline2->to->next ) {
		    if ( first2==NULL ) first2 = spline2;
		    if ( SplinesIntersect(spline,spline2,pts,t1s,t2s)) {
			if ( spline->to->next!=spline2 && spline->from->prev!=spline2 )
			    found = true;
			else for ( i=0; i<10 && t1s[i]!=-1; ++i ) {
			    if ( (t1s[i]<.999 && t1s[i]>.001) || (t2s[i]<.999 && t2s[i]>.001)) {
				found = true;
			break;
			    }
			}
			if ( found )
		break;
		    }
		}
		if ( found )
	    break;
	    }
	    if ( found )
	break;
	}
	if ( found )
    break;
    }
    if ( found ) {
	*_spline = spline;
	*_spline2 = spline2;
    }
return( found );
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

    if ( !isfinite(pt->x) || !isfinite(pt->y) ) {
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

    fs->p->t = t = SplineSolve(xspline,tmin,tmax,fs->p->cx);
    if ( t>=0 && t<=1 ) {
	y = ((yspline->a*t+yspline->b)*t+yspline->c)*t + yspline->d;
	if ( fs->yl<y && fs->yh>y )
return( true );
    }
    /* Although we know that globaly there's more x change, locally there */
    /*  maybe more y change */
    fs->p->t = t = SplineSolve(yspline,tmin,tmax,fs->p->cy);
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

    fs->p->t = t = SplineSolve(yspline,tmin,tmax,fs->p->cy);
    if ( t>=0 && t<=1 ) {
	x = ((xspline->a*t+xspline->b)*t+xspline->c)*t + xspline->d;
	if ( fs->xl<x && fs->xh>x )
return( true );
    }
    /* Although we know that globaly there's more y change, locally there */
    /*  maybe more x change */
    fs->p->t = t = SplineSolve(xspline,tmin,tmax,fs->p->cx);
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

int SplineT2SpiroIndex(Spline *spline,bigreal t,SplineSet *spl) {
    /* User clicked on a spline. Now, where in the spiro array was that? */
    /* I shall assume that the first time we hit a spiro point that corresponds */
    /*  to the point. In some really gnarly spiro tangles that might not be */
    /*  true, but in a well behaved contour I think (hope) it will be ok */

    /* It appears that each spiro cp has a corresponding splinepoint, but */
    /*  I don't want to rely on that because it won't be true after a simplify*/
    Spline *sp, *lastsp=spl->first->next;
    bigreal test;
    int i;
    BasePoint bp;

    for ( i=1; i<spl->spiro_cnt; ++i ) {	/* For once I do mean spiro count, that loops back to the start for close contours */
	if ( i<spl->spiro_cnt-1 ) {
	    bp.x = spl->spiros[i].x;
	    bp.y = spl->spiros[i].y;
	} else if ( SPIRO_SPL_OPEN(spl))
return( -1 );
	else {
	    bp.x = spl->spiros[0].x;
	    bp.y = spl->spiros[0].y;
	}
	for ( sp=lastsp; ; ) {
	    test = SplineNearPoint(sp,&bp,.001);
	    if ( test==-1 ) {
		if ( sp==spline )
return( i-1 );
	    } else {
		if ( sp==spline && t<test )
return( i-1 );
		lastsp = sp;
	break;
	    }
	    if ( sp->to->next==NULL || sp->to==spl->first )
return( -1 );
	    sp = sp->to->next;
	}
    }
return( -1 );
}

static int SplinePrevMinMax(Spline *s,int up) {
    const bigreal t = .9999;
    bigreal y;
    int pup;

    s = s->from->prev;
    while ( s->from->me.y==s->to->me.y && s->islinear )
	s = s->from->prev;
    y = ((s->splines[1].a*t + s->splines[1].b)*t + s->splines[1].c)*t + s->splines[1].d;
    pup = s->to->me.y > y;
return( pup!=up );
}

static int SplineNextMinMax(Spline *s,int up) {
    const bigreal t = .0001;
    bigreal y;
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
	t = IterateSplineSolve(&s->splines[1],ext[i],ext[i+1],pt->y);
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
    HintInstance *hi, *n;

    for ( hi=h->where; hi!=NULL; hi=n ) {
	n = hi->next;
	chunkfree(hi,sizeof(HintInstance));
    }
    chunkfree(h,sizeof(DStemInfo));
}

void DStemInfosFree(DStemInfo *h) {
    DStemInfo *hnext;
    HintInstance *hi, *n;

    for ( ; h!=NULL; h = hnext ) {
	for ( hi=h->where; hi!=NULL; hi=n ) {
	    n = hi->next;
	    chunkfree(hi,sizeof(HintInstance));
	}
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
    HintInstance *hilast, *hicur, *hi;

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

MinimumDistance *MinimumDistanceCopy(MinimumDistance *md) {
    MinimumDistance *head=NULL, *last=NULL, *cur;

    for ( ; md!=NULL; md = md->next ) {
	cur = chunkalloc(sizeof(MinimumDistance));
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
	if ( kp->adjust!=NULL ) {
	    free(kp->adjust->corrections);
	    chunkfree(kp->adjust,sizeof(DeviceTable));
	}
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
	    if ( an->type == act_mark || (an->type==act_mklg && ap->type==at_mark))
		next = NULL;	/* Only one instance of an anchor class in a glyph for mark to base anchors */
				/*  Or for the mark glyphs of ligature classes */
			        /*  Mark to mark & cursive will (probably) have 2 occurances */
			        /*  and ligatures may have lots */
	    AnchorPointsFree(ap);
	} else
	    prev = ap;
    }
return( alist );
}

/* Finds or adds an AnchorClass of the given name. Resets it to have the given subtable if not NULL */
AnchorClass *SFFindOrAddAnchorClass(SplineFont *sf,char *name,struct lookup_subtable *sub) {
    AnchorClass *ac;
    int actype = act_unknown;

    for ( ac=sf->anchor; ac!=NULL; ac=ac->next )
        if (strcmp(name,ac->name)==0)
    break;
    if ( ac!=NULL && ( sub==NULL || ac->subtable==sub ) )
return( ac );

    if ( sub!=NULL )
        actype = sub->lookup->lookup_type==gpos_cursive             ? act_curs :
                    sub->lookup->lookup_type==gpos_mark2base        ? act_mark :
                    sub->lookup->lookup_type==gpos_mark2ligature    ? act_mklg :
                    sub->lookup->lookup_type==gpos_mark2mark        ? act_mkmk :
                                                                      act_unknown;
    if ( ac==NULL ) {
        ac = chunkalloc(sizeof(AnchorClass));
        ac->subtable = sub;
        ac->type = actype;
        ac->name = copy( name );
        ac->next = sf->anchor;
        sf->anchor = ac;
    }
    else if ( sub!=NULL && ac->subtable!=sub ) {
        ac->subtable = sub;
        ac->type = actype;
    }
return( ac );
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
	if ( ap->xadjust.corrections!=NULL ) {
	    int len = ap->xadjust.last_pixel_size-ap->xadjust.first_pixel_size+1;
	    ap->xadjust.corrections = malloc(len);
	    memcpy(ap->xadjust.corrections,alist->xadjust.corrections,len);
	}
	if ( ap->yadjust.corrections!=NULL ) {
	    int len = ap->yadjust.last_pixel_size-ap->yadjust.first_pixel_size+1;
	    ap->yadjust.corrections = malloc(len);
	    memcpy(ap->yadjust.corrections,alist->yadjust.corrections,len);
	}
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
	free(ap->xadjust.corrections);
	free(ap->yadjust.corrections);
	chunkfree(ap,sizeof(AnchorPoint));
    }
}

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
	    (&new->xadjust)[i].corrections = malloc(len);
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
    new->corrections = malloc(len);
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
	    adjust->corrections = malloc(1);
	} else if ( size>=adjust->first_pixel_size &&
		size<=adjust->last_pixel_size ) {
	} else if ( size>adjust->last_pixel_size ) {
	    adjust->corrections = realloc(adjust->corrections,
		    size-adjust->first_pixel_size);
	    for ( i=len; i<size-adjust->first_pixel_size; ++i )
		adjust->corrections[i] = 0;
	    adjust->last_pixel_size = size;
	} else {
	    int8 *new = malloc(adjust->last_pixel_size-size+1);
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

void PSTFree(PST *pst) {
    PST *pnext;
    for ( ; pst!=NULL; pst = pnext ) {
	pnext = pst->next;
	if ( pst->type==pst_lcaret )
	    free(pst->u.lcaret.carets);
	else if ( pst->type==pst_pair ) {
	    free(pst->u.pair.paired);
	    ValDevFree(pst->u.pair.vr[0].adjust);
	    ValDevFree(pst->u.pair.vr[1].adjust);
	    chunkfree(pst->u.pair.vr,sizeof(struct vr [2]));
	} else if ( pst->type!=pst_position ) {
	    free(pst->u.subs.variant);
	} else if ( pst->type==pst_position ) {
	    ValDevFree(pst->u.pos.adjust);
	}
	chunkfree(pst,sizeof(PST));
    }
}

void FPSTRuleContentsFree(struct fpst_rule *r, enum fpossub_format format) {
    int j;

    switch ( format ) {
      case pst_glyphs:
	free(r->u.glyph.names);
	free(r->u.glyph.back);
	free(r->u.glyph.fore);
      break;
      case pst_class:
	free(r->u.class.nclasses);
	free(r->u.class.bclasses);
	free(r->u.class.fclasses);
      break;
      case pst_reversecoverage:
	free(r->u.rcoverage.replacements);
      case pst_coverage:
	for ( j=0 ; j<r->u.coverage.ncnt ; ++j )
	    free(r->u.coverage.ncovers[j]);
	free(r->u.coverage.ncovers);
	for ( j=0 ; j<r->u.coverage.bcnt ; ++j )
	    free(r->u.coverage.bcovers[j]);
	free(r->u.coverage.bcovers);
	for ( j=0 ; j<r->u.coverage.fcnt ; ++j )
	    free(r->u.coverage.fcovers[j]);
	free(r->u.coverage.fcovers);
      break;
    }
    free(r->lookups);
}

void FPSTRulesFree(struct fpst_rule *r, enum fpossub_format format, int rcnt) {
    int i;
    for ( i=0; i<rcnt; ++i )
	FPSTRuleContentsFree(&r[i],format);
    free(r);
}

static struct fpst_rule *RulesCopy(struct fpst_rule *from, int cnt,
	enum fpossub_format format ) {
    int i, j;
    struct fpst_rule *to, *f, *t;

    if ( cnt==0 )
return( NULL );

    to = calloc(cnt,sizeof(struct fpst_rule));
    for ( i=0; i<cnt; ++i ) {
	f = from+i; t = to+i;
	switch ( format ) {
	  case pst_glyphs:
	    t->u.glyph.names = copy(f->u.glyph.names);
	    t->u.glyph.back = copy(f->u.glyph.back);
	    t->u.glyph.fore = copy(f->u.glyph.fore);
	  break;
	  case pst_class:
	    t->u.class.ncnt = f->u.class.ncnt;
	    t->u.class.bcnt = f->u.class.bcnt;
	    t->u.class.fcnt = f->u.class.fcnt;
	    t->u.class.nclasses = malloc( f->u.class.ncnt*sizeof(uint16));
	    memcpy(t->u.class.nclasses,f->u.class.nclasses,
		    f->u.class.ncnt*sizeof(uint16));
	    if ( t->u.class.bcnt!=0 ) {
		t->u.class.bclasses = malloc( f->u.class.bcnt*sizeof(uint16));
		memcpy(t->u.class.bclasses,f->u.class.bclasses,
			f->u.class.bcnt*sizeof(uint16));
	    }
	    if ( t->u.class.fcnt!=0 ) {
		t->u.class.fclasses = malloc( f->u.class.fcnt*sizeof(uint16));
		memcpy(t->u.class.fclasses,f->u.class.fclasses,
			f->u.class.fcnt*sizeof(uint16));
	    }
	  break;
	  case pst_reversecoverage:
	    t->u.rcoverage.replacements = copy(f->u.rcoverage.replacements);
	  case pst_coverage:
	    t->u.coverage.ncnt = f->u.coverage.ncnt;
	    t->u.coverage.bcnt = f->u.coverage.bcnt;
	    t->u.coverage.fcnt = f->u.coverage.fcnt;
	    t->u.coverage.ncovers = malloc( f->u.coverage.ncnt*sizeof(char *));
	    for ( j=0; j<t->u.coverage.ncnt; ++j )
		t->u.coverage.ncovers[j] = copy(f->u.coverage.ncovers[j]);
	    if ( t->u.coverage.bcnt!=0 ) {
		t->u.coverage.bcovers = malloc( f->u.coverage.bcnt*sizeof(char *));
		for ( j=0; j<t->u.coverage.bcnt; ++j )
		    t->u.coverage.bcovers[j] = copy(f->u.coverage.bcovers[j]);
	    }
	    if ( t->u.coverage.fcnt!=0 ) {
		t->u.coverage.fcovers = malloc( f->u.coverage.fcnt*sizeof(char *));
		for ( j=0; j<t->u.coverage.fcnt; ++j )
		    t->u.coverage.fcovers[j] = copy(f->u.coverage.fcovers[j]);
	    }
	  break;
	}
	if ( f->lookup_cnt!=0 ) {
	    t->lookup_cnt = f->lookup_cnt;
	    t->lookups = malloc(t->lookup_cnt*sizeof(struct seqlookup));
	    memcpy(t->lookups,f->lookups,t->lookup_cnt*sizeof(struct seqlookup));
	}
    }
return( to );
}

FPST *FPSTCopy(FPST *fpst) {
    FPST *nfpst;
    int i;

    nfpst = chunkalloc(sizeof(FPST));
    *nfpst = *fpst;
    nfpst->next = NULL;
    if ( nfpst->nccnt!=0 ) {
	nfpst->nclass = malloc(nfpst->nccnt*sizeof(char *));
	nfpst->nclassnames = malloc(nfpst->nccnt*sizeof(char *));
	for ( i=0; i<nfpst->nccnt; ++i ) {
	    nfpst->nclass[i] = copy(fpst->nclass[i]);
	    nfpst->nclassnames[i] = copy(fpst->nclassnames[i]);
	}
    }
    if ( nfpst->bccnt!=0 ) {
	nfpst->bclass = malloc(nfpst->bccnt*sizeof(char *));
	nfpst->bclassnames = malloc(nfpst->bccnt*sizeof(char *));
	for ( i=0; i<nfpst->bccnt; ++i ) {
	    nfpst->bclass[i] = copy(fpst->bclass[i]);
	    nfpst->bclassnames[i] = copy(fpst->bclassnames[i]);
	}
    }
    if ( nfpst->fccnt!=0 ) {
	nfpst->fclass = malloc(nfpst->fccnt*sizeof(char *));
	nfpst->fclassnames = malloc(nfpst->fccnt*sizeof(char *));
	for ( i=0; i<nfpst->fccnt; ++i ) {
	    nfpst->fclass[i] = copy(fpst->fclass[i]);
	    nfpst->fclassnames[i] = copy(fpst->fclassnames[i]);
	}
    }
    nfpst->rules = RulesCopy(fpst->rules,fpst->rule_cnt,fpst->format);
return( nfpst );
}

void FPSTClassesFree(FPST *fpst) {
    int i;

    for ( i=0; i<fpst->nccnt; ++i ) {
	free(fpst->nclass[i]);
	free(fpst->nclassnames[i]);
    }
    for ( i=0; i<fpst->bccnt; ++i ) {
	free(fpst->bclass[i]);
	free(fpst->bclassnames[i]);
    }
    for ( i=0; i<fpst->fccnt; ++i ) {
	free(fpst->fclass[i]);
	free(fpst->fclassnames[i]);
    }
    free(fpst->nclass); free(fpst->bclass); free(fpst->fclass);
    free(fpst->nclassnames); free(fpst->bclassnames); free(fpst->fclassnames);

    fpst->nccnt = fpst->bccnt = fpst->fccnt = 0;
    fpst->nclass = fpst->bclass = fpst->fclass = NULL;
    fpst->nclassnames = fpst->bclassnames = fpst->fclassnames = NULL;
}

void FPSTFree(FPST *fpst) {
    FPST *next;
    int i;

    while ( fpst!=NULL ) {
	next = fpst->next;
	FPSTClassesFree(fpst);
	for ( i=0; i<fpst->rule_cnt; ++i ) {
	    FPSTRuleContentsFree( &fpst->rules[i],fpst->format );
	}
	free(fpst->rules);
	chunkfree(fpst,sizeof(FPST));
	fpst = next;
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
}

SplineChar *SplineCharCreate(int layer_cnt) {
    SplineChar *sc = chunkalloc(sizeof(SplineChar));
    int i;

    sc->color = COLOR_DEFAULT;
    sc->orig_pos = 0xffff;
    sc->unicodeenc = -1;
    sc->layer_cnt = layer_cnt;
    sc->layers = calloc(layer_cnt,sizeof(Layer));
    for ( i=0; i<layer_cnt; ++i )
	LayerDefault(&sc->layers[i]);
    sc->tex_height = sc->tex_depth = sc->italic_correction = sc->top_accent_horiz =
	    TEX_UNDEF;
return( sc );
}

SplineChar *SFSplineCharCreate(SplineFont *sf) {
    SplineChar *sc = SplineCharCreate(sf==NULL?2:sf->layer_cnt);
    int i;

    if ( sf==NULL ) {
	sc->layers[ly_back].background = true;
	sc->layers[ly_fore].background = false;
    } else {
	for ( i=0; i<sf->layer_cnt; ++i ) {
	    sc->layers[i].background = sf->layers[i].background;
	    sc->layers[i].order2     = sf->layers[i].order2;
	}
	sc->parent = sf;
    }
return( sc );
}

void GlyphVariantsFree(struct glyphvariants *gv) {
    int i;

    if ( gv==NULL )
return;
    free(gv->variants);
    DeviceTableFree(gv->italic_adjusts);
    for ( i=0; i<gv->part_cnt; ++i )
	free( gv->parts[i].component );
    free(gv->parts);
    chunkfree(gv,sizeof(*gv));
}

struct glyphvariants *GlyphVariantsCopy(struct glyphvariants *gv) {
    struct glyphvariants *newgv;
    int i;

    if ( gv==NULL )
return( NULL );
    newgv = chunkalloc(sizeof(struct glyphvariants));
    newgv->variants = copy(gv->variants);
    newgv->italic_adjusts = DeviceTableCopy(gv->italic_adjusts);
    newgv->part_cnt = gv->part_cnt;
    if ( gv->part_cnt!=0 ) {
	newgv->parts = calloc(gv->part_cnt,sizeof(struct gv_part));
	memcpy(newgv->parts,gv->parts,gv->part_cnt*sizeof(struct gv_part));
	for ( i=0; i<gv->part_cnt; ++i )
	    newgv->parts[i].component = copy(gv->parts[i].component);
    }
return( newgv );
}

struct mathkern *MathKernCopy(struct mathkern *mk) {
    int i,j;
    struct mathkern *mknew;

    if ( mk==NULL )
return( NULL );
    mknew = chunkalloc(sizeof(*mknew));
    for ( i=0; i<4; ++i ) {
	struct mathkernvertex *mkv = &(&mk->top_right)[i];
	struct mathkernvertex *mknewv = &(&mknew->top_right)[i];
	mknewv->cnt = mkv->cnt;
	if ( mknewv->cnt!=0 ) {
	    mknewv->mkd = calloc(mkv->cnt,sizeof(struct mathkerndata));
	    for ( j=0; j<mkv->cnt; ++j ) {
		mknewv->mkd[j].height = mkv->mkd[j].height;
		mknewv->mkd[j].kern   = mkv->mkd[j].kern;
		mknewv->mkd[j].height_adjusts = DeviceTableCopy( mkv->mkd[j].height_adjusts );
		mknewv->mkd[j].kern_adjusts   = DeviceTableCopy( mkv->mkd[j].kern_adjusts );
	    }
	}
    }
return( mknew );
}

void MathKernVContentsFree(struct mathkernvertex *mk) {
    int i;
    for ( i=0; i<mk->cnt; ++i ) {
	DeviceTableFree(mk->mkd[i].height_adjusts);
	DeviceTableFree(mk->mkd[i].kern_adjusts);
    }
    free(mk->mkd);
}

void MathKernFree(struct mathkern *mk) {
    int i;

    if ( mk==NULL )
return;
    for ( i=0; i<4; ++i )
	MathKernVContentsFree( &(&mk->top_right)[i] );
    chunkfree(mk,sizeof(*mk));
}

void SplineCharListsFree(struct splinecharlist *dlist) {
    struct splinecharlist *dnext;
    for ( ; dlist!=NULL; dlist = dnext ) {
	dnext = dlist->next;
	chunkfree(dlist,sizeof(struct splinecharlist));
    }
}

struct pattern *PatternCopy(struct pattern *old, real transform[6]) {
    struct pattern *pat;

    if ( old==NULL )
return( NULL );

    pat = chunkalloc(sizeof(struct pattern));

    *pat = *old;
    pat->pattern = copy( old->pattern );
    if ( transform!=NULL )
	MatMultiply(pat->transform,transform,pat->transform);
return( pat );
}

void PatternFree(struct pattern *pat) {
    if ( pat==NULL )
return;
    free(pat->pattern);
    chunkfree(pat,sizeof(struct pattern));
}

struct gradient *GradientCopy(struct gradient *old,real transform[6]) {
    struct gradient *grad;

    if ( old==NULL )
return( NULL );

    grad = chunkalloc(sizeof(struct gradient));

    *grad = *old;
    grad->grad_stops = malloc(old->stop_cnt*sizeof(struct grad_stops));
    memcpy(grad->grad_stops,old->grad_stops,old->stop_cnt*sizeof(struct grad_stops));
    if ( transform!=NULL ) {
	BpTransform(&grad->start,&grad->start,transform);
	BpTransform(&grad->stop,&grad->stop,transform);
    }
return( grad );
}

void GradientFree(struct gradient *grad) {
    if ( grad==NULL )
return;
    free(grad->grad_stops);
    chunkfree(grad,sizeof(struct gradient));
}

void BrushCopy(struct brush *into, struct brush *from, real transform[6]) {
    *into = *from;
    into->gradient = GradientCopy(from->gradient,transform);
    into->pattern = PatternCopy(from->pattern,transform);
}

void PenCopy(struct pen *into, struct pen *from,real transform[6]) {
    *into = *from;
    into->brush.gradient = GradientCopy(from->brush.gradient,transform);
    into->brush.pattern = PatternCopy(from->brush.pattern,transform);
}

void LayerFreeContents(SplineChar *sc,int layer) {
    SplinePointListsFree(sc->layers[layer].splines);
    GradientFree(sc->layers[layer].fill_brush.gradient);
    PatternFree(sc->layers[layer].fill_brush.pattern);
    GradientFree(sc->layers[layer].stroke_pen.brush.gradient);
    PatternFree(sc->layers[layer].stroke_pen.brush.pattern);
    RefCharsFree(sc->layers[layer].refs);
    ImageListsFree(sc->layers[layer].images);
    /* image garbage collection????!!!! */
    UndoesFree(sc->layers[layer].undoes);
    UndoesFree(sc->layers[layer].redoes);
}

void SplineCharFreeContents(SplineChar *sc) {
    int i;

    if ( sc==NULL )
return;
    if (sc->name != NULL) free(sc->name);
    if (sc->comment != NULL) free(sc->comment);
    for ( i=0; i<sc->layer_cnt; ++i ) {
#if defined(_NO_PYTHON)
        if (sc->layers[i].python_persistent != NULL) free( sc->layers[i].python_persistent );	/* It's a string of pickled data which we leave as a string */
#else
        PyFF_FreeSCLayer(sc, i);
#endif
	LayerFreeContents(sc,i);
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
    if (sc->ttf_instrs != NULL) free(sc->ttf_instrs);
    if (sc->countermasks != NULL) free(sc->countermasks);
    if (sc->layers != NULL) free(sc->layers);
    AltUniFree(sc->altuni);
    GlyphVariantsFree(sc->horiz_variants);
    GlyphVariantsFree(sc->vert_variants);
    DeviceTableFree(sc->italic_adjusts);
    DeviceTableFree(sc->top_accent_adjusts);
    MathKernFree(sc->mathkern);
    if (sc->glif_name != NULL) { free(sc->glif_name); sc->glif_name = NULL; }
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

void SFRemoveSavedTable(SplineFont *sf, uint32 tag) {
    struct ttf_table *tab, *prev;

    for ( prev=NULL, tab=sf->ttf_tables; tab!=NULL && tab->tag!=tag; prev=tab, tab=tab->next );
    if ( tab!=NULL ) {
	if ( prev==NULL )
	    sf->ttf_tables = tab->next;
	else
	    prev->next = tab->next;
    } else {
	for ( prev=NULL, tab=sf->ttf_tab_saved; tab!=NULL && tab->tag!=tag; prev=tab, tab=tab->next );
	if ( tab==NULL )
return;
	if ( prev==NULL )
	    sf->ttf_tab_saved = tab->next;
	else
	    prev->next = tab->next;
    }
    tab->next = NULL;
    TtfTablesFree(tab);
    if ( !sf->changed ) {
	sf->changed = true;
	FVSetTitles(sf);
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
    new->firsts = malloc(new->first_cnt*sizeof(char *));
    new->seconds = malloc(new->second_cnt*sizeof(char *));
    new->offsets = malloc(new->first_cnt*new->second_cnt*sizeof(int16));
    memcpy(new->offsets,kc->offsets, new->first_cnt*new->second_cnt*sizeof(int16));
    // Group kerning.
    if (kc->firsts_names) new->firsts_names = calloc(new->first_cnt,sizeof(char *));
    if (kc->seconds_names) new->seconds_names = calloc(new->second_cnt,sizeof(char *));
    if (kc->firsts_flags) {
	new->firsts_flags = calloc(new->first_cnt,sizeof(int));
	memcpy(new->firsts_flags, kc->firsts_flags, new->first_cnt*sizeof(int));
    }
    if (kc->seconds_flags) {
	new->seconds_flags = calloc(new->second_cnt,sizeof(int));
	memcpy(new->seconds_flags, kc->seconds_flags, new->second_cnt*sizeof(int));
    }
    if (kc->offsets_flags) {
	new->offsets_flags = calloc(new->first_cnt*new->second_cnt,sizeof(int));
	memcpy(new->offsets_flags, kc->offsets_flags, new->first_cnt*new->second_cnt*sizeof(int));
    }
    for ( i=0; i<new->first_cnt; ++i ) {
	new->firsts[i] = copy(kc->firsts[i]);
	if (kc->firsts_names && kc->firsts_names[i]) new->firsts_names[i] = copy(kc->firsts_names[i]);
    }
    for ( i=0; i<new->second_cnt; ++i ) {
	new->seconds[i] = copy(kc->seconds[i]);
	if (kc->seconds_names && kc->seconds_names[i]) new->seconds_names[i] = copy(kc->seconds_names[i]);
    }
    new->adjusts = calloc(new->first_cnt*new->second_cnt,sizeof(DeviceTable));
    memcpy(new->adjusts,kc->adjusts, new->first_cnt*new->second_cnt*sizeof(DeviceTable));
    for ( i=new->first_cnt*new->second_cnt-1; i>=0 ; --i ) {
	if ( new->adjusts[i].corrections!=NULL ) {
	    int8 *old = new->adjusts[i].corrections;
	    int len = new->adjusts[i].last_pixel_size - new->adjusts[i].first_pixel_size + 1;
	    new->adjusts[i].corrections = malloc(len);
	    memcpy(new->adjusts[i].corrections,old,len);
	}
    }



    new->next = NULL;
return( new );
}

void KernClassFreeContents(KernClass *kc) {
    int i;
    for ( i=1; i<kc->first_cnt; ++i )
	free(kc->firsts[i]);
    for ( i=1; i<kc->second_cnt; ++i )
	free(kc->seconds[i]);
    free(kc->firsts);
    free(kc->seconds);
    free(kc->offsets);
    for ( i=kc->first_cnt*kc->second_cnt-1; i>=0 ; --i )
	free(kc->adjusts[i].corrections);
    free(kc->adjusts);
    if (kc->firsts_flags) free(kc->firsts_flags);
    if (kc->seconds_flags) free(kc->seconds_flags);
    if (kc->offsets_flags) free(kc->offsets_flags);
    if (kc->firsts_names) {
      for ( i=kc->first_cnt-1; i>=0 ; --i )
	free(kc->firsts_names[i]);
      free(kc->firsts_names);
    }
    if (kc->seconds_names) {
      for ( i=kc->second_cnt-1; i>=0 ; --i )
	free(kc->seconds_names[i]);
      free(kc->seconds_names);
    }
}

void KernClassClearSpecialContents(KernClass *kc) {
    // This frees and zeros special data not handled by the FontForge GUI,
    // most of which comes from U. F. O..
    int i;
    if (kc->firsts_flags) { free(kc->firsts_flags); kc->firsts_flags = NULL; }
    if (kc->seconds_flags) { free(kc->seconds_flags); kc->seconds_flags = NULL; }
    if (kc->offsets_flags) { free(kc->offsets_flags); kc->offsets_flags = NULL; }
    if (kc->firsts_names) {
      for ( i=kc->first_cnt-1; i>=0 ; --i )
	free(kc->firsts_names[i]);
      free(kc->firsts_names);
      kc->firsts_names = NULL;
    }
    if (kc->seconds_names) {
      for ( i=kc->second_cnt-1; i>=0 ; --i )
	free(kc->seconds_names[i]);
      free(kc->seconds_names);
      kc->seconds_names = NULL;
    }
}

void KernClassListFree(KernClass *kc) {
    KernClass *n;

    while ( kc ) {
	KernClassFreeContents(kc);
	n = kc->next;
	chunkfree(kc,sizeof(KernClass));
	kc = n;
    }
}

void KernClassListClearSpecialContents(KernClass *kc) {
    KernClass *n;

    while ( kc ) {
	KernClassClearSpecialContents(kc);
	n = kc->next;
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

void OtfFeatNameListFree(struct otffeatname *fn) {
    struct otffeatname *fn_next;

    for ( ; fn!=NULL; fn = fn_next ) {
	fn_next = fn->next;
	OtfNameListFree(fn->names);
	chunkfree(fn,sizeof(*fn));
    }
}

EncMap *EncMapNew(int enccount,int backmax,Encoding *enc) {
/* NOTE: 'enccount' and 'backmax' can sometimes be different map sizes */
    EncMap *map;

    /* Ensure all memory available, otherwise cleanup and exit as NULL */
    if ( (map=chunkalloc(sizeof(EncMap)))!=NULL ) {
	if ( (map->map=malloc(enccount*sizeof(int32)))!=NULL ) {
	    if ( (map->backmap=malloc(backmax*sizeof(int32)))!=NULL ) {
		map->enccount = map->encmax = enccount;
		map->backmax = backmax;
		memset(map->map,-1,enccount*sizeof(int32));
		memset(map->backmap,-1,backmax*sizeof(int32));
		map->enc = enc;
		return( map );
	    }
	    free(map->map);
	}
	free(map);
    }
    return( NULL );
}

EncMap *EncMap1to1(int enccount) {
/* Used for CID fonts where CID is same as orig_pos */
/* NOTE: map-enc point to a global variable custom. */
/* TODO: avoid global custom and use passed pointer */
    EncMap *map;
    int i;

    if ( (map=EncMapNew(enccount,enccount,&custom))!=NULL ) {
	for ( i=0; i<enccount; ++i )
	    map->map[i] = map->backmap[i] = i;
    }
    return( map );
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
/* Make a duplicate 'new' copy of EncMap 'map', Return a NULL if error */
/* NOTE: new-enc also shares map->enc, so be careful if freeing either */
    EncMap *new;
    int n;

    /* Ensure all memory available, otherwise cleanup and exit as NULL */
    if ( (new=chunkalloc(sizeof(EncMap)))!=NULL ) {
	*new = *map;
	if ( (new->map=malloc(map->enccount*sizeof(int32)))!=NULL ) {
	    if ( (new->backmap=malloc(map->backmax*sizeof(int32)))!=NULL ) {
		memcpy(new->map,map->map,map->enccount*sizeof(int32));
		memcpy(new->backmap,map->backmap,map->backmax*sizeof(int32));
		/* NOTE: This new->enc 'also' points to same map->enc. */
		if ( map->remap==NULL )
		    return( new );
		for ( n=0; map->remap[n].infont!=-1; ++n );
		if ( (new->remap=malloc(n*sizeof(struct remap)))!=NULL ) {
		    memcpy(new->remap,map->remap,n*sizeof(struct remap));
		    return( new );
		}
		free(new->backmap);
	    }
	    free(new->map);
	}
	free(new);
     }
    return( NULL );
}

void MarkClassFree(int cnt,char **classes,char **names) {
    int i;

    for ( i=1; i<cnt; ++i ) {
	free( classes[i] );
	free( names[i] );
    }
    free( classes );
    free( names );
}

void MarkSetFree(int cnt,char **classes,char **names) {
    int i;

    for ( i=0; i<cnt; ++i ) {
	free( classes[i] );
	free( names[i] );
    }
    free( classes );
    free( names );
}

struct baselangextent *BaseLangCopy(struct baselangextent *extent) {
    struct baselangextent *head, *last, *cur;

    last = head = NULL;
    for ( ; extent!=NULL; extent = extent->next ) {
	cur = chunkalloc(sizeof(struct baselangextent));
	*cur = *extent;
	cur->features = BaseLangCopy(cur->features);
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
    }
return( head );
}

void BaseLangFree(struct baselangextent *extent) {
    struct baselangextent *next;

    while ( extent!=NULL ) {
	next = extent->next;
	BaseLangFree(extent->features);
	chunkfree(extent,sizeof(struct baselangextent));
	extent = next;
    }
}

void BaseScriptFree(struct basescript *bs) {
    struct basescript *next;

    while ( bs!=NULL ) {
	next = bs->next;
	if ( bs->baseline_pos )
	    free(bs->baseline_pos);
	BaseLangFree(bs->langs);
	chunkfree(bs,sizeof(struct basescript));
	bs = next;
    }
}

void BaseFree(struct Base *base) {
    if ( base==NULL )
return;

    free(base->baseline_tags);
    BaseScriptFree(base->scripts);
    chunkfree(base,sizeof(struct Base));
}

static OTLookup **OTLListCopy(OTLookup **str) {
    OTLookup **ret;
    int i;

    if ( str == NULL )
return( NULL );
    for ( i=0 ; str[i]!=NULL; ++i );
    ret = malloc((i+1)*sizeof( OTLookup *));
    for ( i=0 ; str[i]!=NULL; ++i )
	ret[i] = str[i];
    ret[i] = NULL;
return( ret );
}

struct jstf_lang *JstfLangsCopy(struct jstf_lang *jl) {
    struct jstf_lang *head=NULL, *last=NULL, *cur;
    int i;

    while ( jl!=NULL ) {
	cur = chunkalloc(sizeof(*cur));
	cur->lang = jl->lang;
	cur->cnt = jl->cnt;
	cur->prios = calloc(cur->cnt,sizeof(struct jstf_prio));
	for ( i=0; i<cur->cnt; ++i ) {
	    cur->prios[i].enableShrink = OTLListCopy( jl->prios[i].enableShrink );
	    cur->prios[i].disableShrink = OTLListCopy( jl->prios[i].disableShrink );
	    cur->prios[i].maxShrink = OTLListCopy( jl->prios[i].maxShrink );
	    cur->prios[i].enableExtend = OTLListCopy( jl->prios[i].enableExtend );
	    cur->prios[i].disableExtend = OTLListCopy( jl->prios[i].disableExtend );
	    cur->prios[i].maxExtend = OTLListCopy( jl->prios[i].maxExtend );
	}
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	jl = jl->next;
    }
return( head );
}

void JstfLangFree(struct jstf_lang *jl) {
    struct jstf_lang *next;
    int i;

    while ( jl!=NULL ) {
	next = jl->next;
	for ( i=0; i<jl->cnt; ++i ) {
	    struct jstf_prio *jp = &jl->prios[i];
	    free(jp->enableShrink);
	    free(jp->disableShrink);
	    free(jp->maxShrink);
	    free(jp->enableExtend);
	    free(jp->disableExtend);
	    free(jp->maxExtend);
	}
	free(jl->prios);
	chunkfree(jl,sizeof(*jl));
	jl = next;
    }
}

void JustifyFree(Justify *just) {
    Justify *next;

    while ( just!=NULL ) {
	next = just->next;
	free(just->extenders);
	JstfLangFree(just->langs);
	chunkfree(just,sizeof(*just));
	just = next;
    }
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
    if ( sf->sfd_version>0 && sf->sfd_version<2 ) {
      // Free special data.
      SplineFont1* oldsf = (SplineFont1*)sf;
      // First the script language lists.
      if (oldsf->script_lang != NULL) {
        int scripti;
        for (scripti = 0; oldsf->script_lang[scripti] != NULL; scripti ++) {
          int scriptj;
          for (scriptj = 0; oldsf->script_lang[scripti][scriptj].script != 0; scriptj ++) {
            if (oldsf->script_lang[scripti][scriptj].langs != NULL) free(oldsf->script_lang[scripti][scriptj].langs);
          }
          free(oldsf->script_lang[scripti]); oldsf->script_lang[scripti] = NULL;
        }
        free(oldsf->script_lang); oldsf->script_lang = NULL;
      }
      // Then the table orderings.
      {
        struct table_ordering *ord = oldsf->orders;
        while (ord != NULL) {
          struct table_ordering *ordtofree = ord;
          if (ord->ordered_features != NULL) free(ord->ordered_features);
          ord = ord->next;
          chunkfree(ordtofree, sizeof(struct table_ordering));
        }
        oldsf->orders = NULL;
      }
    }
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
    if ( sf->styleMapFamilyName && sf->styleMapFamilyName[0]!='\0' ) { free(sf->styleMapFamilyName); sf->styleMapFamilyName = NULL; }
    MacFeatListFree(sf->features);
    /* We don't free the EncMap. That field is only a temporary pointer. Let the FontViewBase free it, that's where it really lives */
    // TODO: But that doesn't always get freed. The statement below causes double-frees, so we need to come up with better conditions.
    #if 0
    if (sf->cidmaster == NULL || sf->cidmaster == sf)
      if (sf->map != NULL) { free(sf->map); sf->map = NULL; }
    #endif // 0
    SplinePointListsFree(sf->grid.splines);
    AnchorClassesFree(sf->anchor);
    TtfTablesFree(sf->ttf_tables);
    TtfTablesFree(sf->ttf_tab_saved);
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
    OtfFeatNameListFree(sf->feat_names);
    MarkClassFree(sf->mark_class_cnt,sf->mark_classes,sf->mark_class_names);
    MarkSetFree(sf->mark_set_cnt,sf->mark_sets,sf->mark_set_names);
    GlyphGroupsFree(sf->groups);
    GlyphGroupKernsFree(sf->groupkerns);
    GlyphGroupKernsFree(sf->groupvkerns);
    free( sf->gasp );
#if defined(_NO_PYTHON)
    free( sf->python_persistent );	/* It's a string of pickled data which we leave as a string */
#else
    PyFF_FreeSF(sf);
#endif
    BaseFree(sf->horiz_base);
    BaseFree(sf->vert_base);
    JustifyFree(sf->justify);
    if (sf->layers != NULL) {
      int layer;
      for (layer = 0; layer < sf->layer_cnt; layer ++) {
        if (sf->layers[layer].name != NULL) {
          free(sf->layers[layer].name);
          sf->layers[layer].name = NULL;
        }
        if (sf->layers[layer].ufo_path != NULL) {
          free(sf->layers[layer].ufo_path);
          sf->layers[layer].ufo_path = NULL;
        }
      }
      free(sf->layers); sf->layers = NULL;
    }
    free(sf);
}

void SplineFontClearSpecial(SplineFont *sf) {
    int i;

    if ( sf==NULL )
return;
    if ( sf->mm!=NULL ) {
	MMSetClearSpecial(sf->mm);
return;
    }
    for ( i=0; i<sf->glyphcnt; ++i ) if ( sf->glyphs[i]!=NULL ) {
	struct splinechar *sc = sf->glyphs[i];
	if (sc->glif_name != NULL) { free(sc->glif_name); sc->glif_name = NULL; }
    }
    for ( i=0; i<sf->subfontcnt; ++i )
	SplineFontClearSpecial(sf->subfonts[i]);
    KernClassListClearSpecialContents(sf->kerns);
    KernClassListClearSpecialContents(sf->vkerns);
    if (sf->groups) { GlyphGroupsFree(sf->groups); sf->groups = NULL; }
    if (sf->groupkerns) { GlyphGroupKernsFree(sf->groupkerns); sf->groupkerns = NULL; }
    if (sf->groupvkerns) { GlyphGroupKernsFree(sf->groupvkerns); sf->groupvkerns = NULL; }
    if (sf->python_persistent) {
#if defined(_NO_PYTHON)
      free( sf->python_persistent );	/* It's a string of pickled data which we leave as a string */
#else
      PyFF_FreeSF(sf);
#endif
      sf->python_persistent = NULL;
    }
    if (sf->layers != NULL) {
      int layer;
      for (layer = 0; layer < sf->layer_cnt; layer ++) {
        if (sf->layers[layer].ufo_path != NULL) {
          free(sf->layers[layer].ufo_path);
          sf->layers[layer].ufo_path = NULL;
        }
      }
    }
}

#if 0
// These are in splinefont.h.
#define GROUP_NAME_KERNING_UFO 1
#define GROUP_NAME_KERNING_FEATURE 2
#define GROUP_NAME_VERTICAL 4 // Otherwise horizontal.
#define GROUP_NAME_RIGHT 8 // Otherwise left (or above).
#endif // 0


void GlyphGroupFree(struct ff_glyphclasses* group) {
  if (group->classname != NULL) free(group->classname);
  if (group->glyphs != NULL) free(group->glyphs);
  free(group);
}

void GlyphGroupsFree(struct ff_glyphclasses* root) {
  struct ff_glyphclasses* current = root;
  struct ff_glyphclasses* next;
  while (current != NULL) {
    next = current->next;
    GlyphGroupFree(current);
    current = next;
  }
}

int GroupNameType(const char *input) {
  int kerning_type = 0; // 1 for U. F. O., 2 for feature file.
  int kerning_vert = 0;
  int kerning_side = 0; // 1 for left, 2 for right.
  if (strchr(input, ' ') || strchr(input, '\n')) return -1;
  if (strncmp(input, "public.kern", strlen("public.kern")) == 0) {
    int off1 = strlen("public.kern");
    char nextc = *(input+off1);
    if (nextc == '1') kerning_side = 1;
    if (nextc == '2') kerning_side = 2;
    if (kerning_side != 0 && *(input+off1+1) == '.' && *(input+off1+2) != '\0')
      kerning_type = 1;
    else return -1;
  } else if (strncmp(input, "public.vkern", strlen("public.vkern")) == 0) {
    kerning_vert = 1;
    int off1 = strlen("public.vkern");
    char nextc = *(input+off1);
    if (nextc == '1') kerning_side = 1;
    if (nextc == '2') kerning_side = 2;
    if (kerning_side != 0 && *(input+off1+1) == '.' && *(input+off1+2) != '\0')
      kerning_type = 1;
    else return -1;
  } else if (strncmp(input, "@MMK_", strlen("@MMK_")) == 0) {
    int off1 = strlen("@MMK_");
    char nextc = *(input+off1);
    if (nextc == 'L') kerning_side = 1;
    else if (nextc == 'R') kerning_side = 2;
    else if (nextc == 'A') { kerning_side = 1; kerning_vert = 1; }
    else if (nextc == 'B') { kerning_side = 2; kerning_vert = 1; }
    if (kerning_side != 0 && *(input+off1+1) == '_' && *(input+off1+2) != '\0')
      kerning_type = 2;
    else return -1;
  }
  return kerning_type | ((kerning_side == 2) ? GROUP_NAME_RIGHT : 0) | (kerning_vert * GROUP_NAME_VERTICAL);
}

void GlyphGroupKernFree(struct ff_rawoffsets* groupkern) {
  if (groupkern->left != NULL) free(groupkern->left);
  if (groupkern->right != NULL) free(groupkern->right);
  free(groupkern);
}

void GlyphGroupKernsFree(struct ff_rawoffsets* root) {
  struct ff_rawoffsets* current = root;
  struct ff_rawoffsets* next;
  while (current != NULL) {
    next = current->next;
    GlyphGroupKernFree(current);
    current = next;
  }
}

int CountKerningClasses(SplineFont *sf) {
    struct kernclass *current_kernclass;
    int isv;
    int isr;
    int i;
    int absolute_index = 0; // This gives us a unique index for each kerning class.
    // First we catch the existing names.
    absolute_index = 0;
    for (isv = 0; isv < 2; isv++)
    for (current_kernclass = (isv ? sf->vkerns : sf->kerns); current_kernclass != NULL; current_kernclass = current_kernclass->next)
    for (isr = 0; isr < 2; isr++) {
      // for ( i=0; i< (isr ? current_kernclass->second_cnt : current_kernclass->first_cnt); ++i );
      // absolute_index +=i;
      absolute_index += (isr ? current_kernclass->second_cnt : current_kernclass->first_cnt);
    }
    return absolute_index;
}

size_t count_caps(const char * input) {
  size_t count = 0;
  for (int i = 0; input[i] != '\0'; i++) {
    if ((input[i] >= 'A') && (input[i] <= 'Z')) count ++;
  }
  return count;
}

char * upper_case(const char * input) {
  size_t output_length = strlen(input);
  char * output = malloc(output_length + 1);
  off_t pos = 0;
  if (output == NULL) return NULL;
  while (pos < output_length) {
    if ((input[pos] >= 'a') && (input[pos] <= 'z')) {
      output[pos] = (char)(((unsigned char) input[pos]) - 0x20U);
    } else {
      output[pos] = input[pos];
    }
    pos++;
  }
  output[pos] = '\0';
  return output;
}

char * same_case(const char * input) {
  size_t output_length = strlen(input);
  char * output = malloc(output_length + 1);
  off_t pos = 0;
  if (output == NULL) return NULL;
  while (pos < output_length) {
    output[pos] = input[pos];
    pos++;
  }
  output[pos] = '\0';
  return output;
}

char * delimit_null(const char * input, char delimiter) {
  size_t output_length = strlen(input);
  char * output = malloc(output_length + 1);
  if (output == NULL) return NULL;
  off_t pos = 0;
  while (pos < output_length) {
    if (input[pos] == delimiter) {
      output[pos] = '\0';
    } else {
      output[pos] = input[pos];
    }
    pos++;
  }
  return output;
}

#ifdef FF_UTHASH_GLIF_NAMES
int HashKerningClassNamesFlex(SplineFont *sf, struct glif_name_index * class_name_hash, int capitalize) {
    struct kernclass *current_kernclass;
    int isv;
    int isr;
    int i;
    int absolute_index = 0; // This gives us a unique index for each kerning class.
    // First we catch the existing names.
    absolute_index = 0;
    for (isv = 0; isv < 2; isv++)
    for (current_kernclass = (isv ? sf->vkerns : sf->kerns); current_kernclass != NULL; current_kernclass = current_kernclass->next)
    for (isr = 0; isr < 2; isr++) if ( (isr ? current_kernclass->seconds_names : current_kernclass->firsts_names) != NULL ) {
    for ( i=0; i < (isr ? current_kernclass->second_cnt : current_kernclass->first_cnt); ++i )
    if ( (isr ? current_kernclass->seconds_names[i] : current_kernclass->firsts_names[i]) != NULL ) {
        // Add it to the hash table with its index.
	if (capitalize) {
          char * cap_name = upper_case(isr ? current_kernclass->seconds_names[i] : current_kernclass->firsts_names[i]);
          glif_name_track_new(class_name_hash, absolute_index + i, cap_name);
          free(cap_name); cap_name = NULL;
	} else {
          glif_name_track_new(class_name_hash, absolute_index + i, (isr ? current_kernclass->seconds_names[i] : current_kernclass->firsts_names[i]));
        }
    }
    absolute_index +=i;
    }
    return absolute_index;
}
int HashKerningClassNames(SplineFont *sf, struct glif_name_index * class_name_hash) {
  return HashKerningClassNamesFlex(sf, class_name_hash, 0);
}
int HashKerningClassNamesCaps(SplineFont *sf, struct glif_name_index * class_name_hash) {
  return HashKerningClassNamesFlex(sf, class_name_hash, 1);
}
#endif

int KerningClassSeekByAbsoluteIndex(const struct splinefont *sf, int seek_index, struct kernclass **okc, int *oisv, int *oisr, int *ooffset) {
    int current = 0;
    struct kernclass *current_kernclass;
    int isv;
    int isr;
    int absolute_index = 0; // This gives us a unique index for each kerning class.
    // First we catch the existing names.
    absolute_index = 0;
    for (isv = 0; isv < 2; isv++)
    for (current_kernclass = (isv ? sf->vkerns : sf->kerns); current_kernclass != NULL; current_kernclass = current_kernclass->next)
    for (isr = 0; isr < 2; isr++) {
      if (seek_index < absolute_index + (isr ? current_kernclass->second_cnt : current_kernclass->first_cnt)) {
        *okc = current_kernclass;
        *oisv = isv;
        *oisr = isr;
        *ooffset = seek_index - absolute_index;
        return 1;
      }
      absolute_index += (isr ? current_kernclass->second_cnt : current_kernclass->first_cnt);
    }
    return 0;
}

struct ff_glyphclasses *SFGetGroup(const struct splinefont *sf, int index, const char *name) {
  if (sf == NULL) return NULL;
  struct ff_glyphclasses *ret = sf->groups;
  while (ret != NULL && (ret->classname == NULL || strcmp(ret->classname, name) != 0)) ret = ret->next;
  return ret;
}

int StringInStrings(char const* const* space, int length, const char *target) {
  int pos;
  for (pos = 0; pos < length; pos++) if (strcmp(space[pos], target) == 0) break;
  return pos;
}

char **StringExplode(const char *input, char delimiter) {
  if (input == NULL) return NULL;
  const char *pstart = input;
  const char *pend = input;
  int entry_count = 0;
  while (*pend != '\0') {
    while (*pstart == delimiter) pstart++;
    pend = pstart;
    while (*pend != delimiter && *pend != '\0') pend++;
    if (pend > pstart) entry_count++;
    pstart = pend;
  }
  char **output = calloc(entry_count + 1, sizeof(char*));
  pstart = input;
  pend = input;
  entry_count = 0;
  while (*pend != '\0') {
    while (*pstart == delimiter) pstart++;
    pend = pstart;
    while (*pend != delimiter && *pend != '\0') pend++;
    if (pend > pstart) output[entry_count++] = strndup(pstart, pend-pstart);
    pstart = pend;
  }
  return output;
}

void ExplodedStringFree(char **input) {
  int index = 0;
  while (input[index] != NULL) free(input[index++]);
  free(input);
}

int SFKerningGroupExistsSpecific(const struct splinefont *sf, const char *groupname, int isv, int isr) {
  if (sf == NULL) return 0;
  if (isv) {
    if (sf->vkerns == NULL) return 0;
    if (isr) return (StringInStrings((const char * const *)sf->vkerns->seconds_names, sf->vkerns->second_cnt, groupname) < sf->vkerns->second_cnt);
    else return (StringInStrings((const char * const *)sf->vkerns->firsts_names, sf->vkerns->first_cnt, groupname) < sf->vkerns->first_cnt);
  } else {
    if (sf->kerns == NULL) return 0;
    if (isr) return (StringInStrings((const char * const *)sf->kerns->seconds_names, sf->kerns->second_cnt, groupname) < sf->kerns->second_cnt);
    else return (StringInStrings((const char * const *)sf->kerns->firsts_names, sf->kerns->first_cnt, groupname) < sf->kerns->first_cnt);
  }
  return 0;
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

void MMSetClearSpecial(MMSet *mm) {
    int i;

    for ( i=0; i<mm->instance_count; ++i ) {
	SplineFontClearSpecial(mm->instances[i]);
    }
    SplineFontClearSpecial(mm->normal);
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
	int ptcnt, int is_y, int i, bigreal within, bigreal max) {
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
	bigreal within, bigreal max ) {
    int i,j,best;
    bigreal low,high,cur;

    for ( i=0; i<ptcnt; ++i )
	cspace[i].cnt = 1;		/* Initialize to non-zero */
    for ( i=0; i<ptcnt; ++i )
	countcluster(ptspace,cspace,ptcnt,is_y,i,within,max);

    for (;;) {
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
	    if ( layer==ly_all )
		SCPreserveState(sc,dohints);
	    else if ( layer!=ly_grid )
		SCPreserveLayer(sc,layer,dohints);
	    changed = true;
	}
	for ( i=cspace[j].first; i<=cspace[j].last; ++i ) {
	    bigreal off = (&ptspace[i]->me.x)[is_y] - cur;
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

int SCRoundToCluster(SplineChar *sc,int layer,int sel,bigreal within,bigreal max) {
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
    /* if "layer"==ly_grid then use sf->grid. if layer==ly_all then all foreground layers*/
    /* if "layer"==ly_fore or ly_all then round hints that fall in our clusters too*/
    int ptcnt, selcnt;
    int l,k,changed;
    SplineSet *spl;
    SplinePoint *sp;
    SplinePoint **ptspace = NULL;
    struct cluster *cspace;
    Spline *s, *first;

    /* First figure out what points we will need */
    for ( k=0; k<2; ++k ) {
	ptcnt = selcnt = 0;
	if ( layer==ly_all ) {
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
		    SplineSetSpirosClear(spl);
		}
	    }
	} else {
	    if ( layer==ly_grid )
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
	if ( ptcnt<=1 ) {
	    free(ptspace);
return(false);				/* Can't be any clusters */
	}
	if ( k==0 )
	    ptspace = malloc((ptcnt+1)*sizeof(SplinePoint *));
	else
	    ptspace[ptcnt] = NULL;
    }

    cspace = malloc(ptcnt*sizeof(struct cluster));

    qsort(ptspace,ptcnt,sizeof(SplinePoint *),xcmp);
    changed = _SplineCharRoundToCluster(sc,ptspace,cspace,ptcnt,false,
	    (layer==ly_all || layer==ly_fore) && !sel,layer,false,within,max);

    qsort(ptspace,ptcnt,sizeof(SplinePoint *),ycmp);
    changed = _SplineCharRoundToCluster(sc,ptspace,cspace,ptcnt,true,
	    (layer==ly_all || layer==ly_fore) && !sel,layer,changed,within,max);

    free(ptspace);
    free(cspace);

    if ( changed ) {
	if ( layer==ly_all ) {
	    for ( l=ly_fore; l<sc->layer_cnt; ++l ) {
		for ( spl = sc->layers[l].splines; spl!=NULL; spl=spl->next ) {
		    first = NULL;
		    SplineSetSpirosClear(spl);
		    for ( s=spl->first->next; s!=NULL && s!=first; s=s->to->next ) {
			SplineRefigure(s);
			if ( first==NULL ) first = s;
		    }
		}
	    }
	} else {
	    if ( layer==ly_grid )
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
	SCCharChangedUpdate(sc,layer);
    }
return( changed );
}

static int SplineRemoveAnnoyingExtrema1(Spline *s,int which,bigreal err_sq) {
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

static int SplineRemoveAnnoyingExtrema(Spline *s,bigreal err_sq) {
    int changed;

    changed = SplineRemoveAnnoyingExtrema1(s,0,err_sq);
    if ( SplineRemoveAnnoyingExtrema1(s,1,err_sq) )
	changed = true;
return( changed );
}

int SplineSetsRemoveAnnoyingExtrema(SplineSet *ss,bigreal err) {
    int changed = false;
    bigreal err_sq = err*err;
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

SplinePoint *SplineBisect(Spline *spline, extended t) {
    Spline1 xstart, xend;
    Spline1 ystart, yend;
    Spline *spline1, *spline2;
    SplinePoint *mid;
    SplinePoint *old0, *old1;
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];
    int order2 = spline->order2;

#ifdef DEBUG
    if ( t<=1e-3 || t>=1-1e-3 )
	IError("Bisection to create a zero length spline");
#endif
    xstart.s0 = xsp->d; ystart.s0 = ysp->d;
    xend.s1 = (extended) xsp->a+xsp->b+xsp->c+xsp->d;
    yend.s1 = (extended) ysp->a+ysp->b+ysp->c+ysp->d;
    xstart.s1 = xend.s0 = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
    ystart.s1 = yend.s0 = ((ysp->a*t+ysp->b)*t+ysp->c)*t + ysp->d;
    FigureSpline1(&xstart,0,t,xsp);
    FigureSpline1(&xend,t,1,xsp);
    FigureSpline1(&ystart,0,t,ysp);
    FigureSpline1(&yend,t,1,ysp);

    mid = chunkalloc(sizeof(SplinePoint));
    mid->me.x = xstart.s1;	mid->me.y = ystart.s1;
    if ( order2 ) {
	mid->nextcp.x = xend.sp.d + xend.sp.c/2;
	mid->nextcp.y = yend.sp.d + yend.sp.c/2;
	mid->prevcp.x = xstart.sp.d + xstart.sp.c/2;
	mid->prevcp.y = ystart.sp.d + ystart.sp.c/2;
    } else {
	mid->nextcp.x = xend.c0;	mid->nextcp.y = yend.c0;
	mid->prevcp.x = xstart.c1;	mid->prevcp.y = ystart.c1;
    }
    if ( mid->me.x==mid->nextcp.x && mid->me.y==mid->nextcp.y )
	mid->nonextcp = true;
    if ( mid->me.x==mid->prevcp.x && mid->me.y==mid->prevcp.y )
	mid->noprevcp = true;

    old0 = spline->from; old1 = spline->to;
    if ( order2 ) {
	old0->nextcp = mid->prevcp;
	old1->prevcp = mid->nextcp;
    } else {
	old0->nextcp.x = xstart.c0;	old0->nextcp.y = ystart.c0;
	old1->prevcp.x = xend.c1;	old1->prevcp.y = yend.c1;
    }
    old0->nonextcp = (old0->nextcp.x==old0->me.x && old0->nextcp.y==old0->me.y);
    old1->noprevcp = (old1->prevcp.x==old1->me.x && old1->prevcp.y==old1->me.y);
    old0->nextcpdef = false;
    old1->prevcpdef = false;
    SplineFree(spline);

    spline1 = chunkalloc(sizeof(Spline));
    spline1->splines[0] = xstart.sp;	spline1->splines[1] = ystart.sp;
    spline1->from = old0;
    spline1->to = mid;
    spline1->order2 = order2;
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
    spline2->order2 = order2;
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

Spline *SplineSplit(Spline *spline, extended ts[3]) {
    /* Split the current spline in up to 3 places */
    Spline1 splines[2][4];
    int i,cnt;
    bigreal base;
    SplinePoint *last, *sp;
    Spline *new;
    int order2 = spline->order2;

    memset(splines,0,sizeof(splines));
    base=0;
    for ( i=cnt=0; i<3 && ts[i]!=-1; ++i ) {
	if ( base>1-1e-3 )			/* Avoid tiny splines */
    break;
	else if ( base<ts[i]-1e-3 ) {
	    FigureSpline1(&splines[0][cnt],base,ts[i],&spline->splines[0]);
	    FigureSpline1(&splines[1][cnt++],base,ts[i],&spline->splines[1]);
	    base = ts[i];
	}
    }
    if ( base==0 )
return( spline );

    FigureSpline1(&splines[0][cnt],base,1.0,&spline->splines[0]);
    FigureSpline1(&splines[1][cnt],base,1.0,&spline->splines[1]);

    last = spline->from;
    for ( i=0; i<=cnt; ++i ) {
	if ( order2 ) {
	    last->nextcp.x = splines[0][i].sp.d+splines[0][i].sp.c/2;
	    last->nextcp.y = splines[1][i].sp.d+splines[1][i].sp.c/2;
	} else {
	    last->nextcp.x = splines[0][i].c0;
	    last->nextcp.y = splines[1][i].c0;
	}
	if ( i==cnt )
	    sp = spline->to;
	else {
	    sp = chunkalloc(sizeof(SplinePoint));
	    sp->me.x = splines[0][i+1].sp.d;
	    sp->me.y = splines[1][i+1].sp.d;
	}
	if ( order2 ) {
	    sp->prevcp = last->nextcp;
	    SplineMake2(last,sp);
	} else {
	    sp->prevcp.x = splines[0][i].c1;
	    sp->prevcp.y = splines[1][i].c1;
	    SplineMake3(last,sp);
	}
	last = sp;
    }

    new = spline->from->next;
    SplineFree(spline);
return( new );
}

int SSExistsInLayer(SplineSet *ss,SplineSet *lots ) {
    /* In Find Problems we hold some state while we allow the user to go off */
    /*  and do stuff. It is perfectly possible for the user to delete the */
    /*  state we hold pointers to. Do a rough check that the thing hasn't */
    /*  been deleted */
    while ( lots!=NULL ) {
	if ( lots==ss )
return( true );
	lots = lots->next;
    }
return( false );
}

int SplineExistsInSS(Spline *s,SplineSet *ss) {
    /* In Find Problems we hold some state while we allow the user to go off */
    /*  and do stuff. It is perfectly possible for the user to delete the */
    /*  state we hold pointers to. Do a rough check that the thing hasn't */
    /*  been deleted */
    Spline *spline, *first;

    first = NULL;
    for ( spline = ss->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	if ( first==NULL ) first = spline;
	if ( spline==s )
return( true );
    }
return( false );
}

int SpExistsInSS(SplinePoint *sp,SplineSet *ss) {
    /* In Find Problems we hold some state while we allow the user to go off */
    /*  and do stuff. It is perfectly possible for the user to delete the */
    /*  state we hold pointers to. Do a rough check that the thing hasn't */
    /*  been deleted */
    SplinePoint *sp2;

    for ( sp2 = ss->first; ; ) {
	if ( sp==sp2 )
return( true );
	if ( sp2->next==NULL )
return( false );
	sp2 = sp2->next->to;
	if ( sp2==ss->first )
return( false );
    }
}

int SSHasClip(SplineSet *ss) {
    while ( ss!=NULL ) {
	if ( ss->is_clip_path )
return( true );
	ss = ss->next;
    }
return( false );
}

int SSHasDrawn(SplineSet *ss) {
    while ( ss!=NULL ) {
	if ( !ss->is_clip_path )
return( true );
	ss = ss->next;
    }
return( false );
}

int SSBoundsWithin(SplineSet *ss,bigreal z1, bigreal z2, bigreal *wmin, bigreal *wmax, int major ) {
    /* if major==0 then find y values when x between z1, z2 */
    /* if major==1 then find x values when y between z1, z2 */
    bigreal w0= 1e23, w1= -1e23;
    int any=0;
    Spline *s, *first;
    Spline1D *ws, *zs;
    extended ts[3];
    bigreal w, z;
    int i;
    int other = !major;

    if ( z1>z2 ) {
	bigreal temp = z1;
	z1 = z2;
	z2 = temp;
    }

    while ( ss!=NULL ) {
	first = NULL;
	for ( s=ss->first->next; s!=first; s=s->to->next ) {
	    if ( first==NULL ) first = s;
	    ws = &s->splines[other];
	    zs = &s->splines[major];
	    if ( major ) {
		if ( s->from->me.y<z1 && s->from->nextcp.y<z1 && s->to->prevcp.y<z1 && s->to->me.y<z1 )
	continue;
		else if ( s->from->me.y>z2 && s->from->nextcp.y>z2 && s->to->prevcp.y>z2 && s->to->me.y>z2 )
	continue;
	    } else {
		if ( s->from->me.x<z1 && s->from->nextcp.x<z1 && s->to->prevcp.x<z1 && s->to->me.x<z1 )
	continue;
		else if ( s->from->me.x>z2 && s->from->nextcp.x>z2 && s->to->prevcp.x>z2 && s->to->me.x>z2 )
	continue;
	    }
	    if ( CubicSolve(zs,z1,ts)) {
		for ( i=0; i<2 && ts[i]!=-1; ++i ) {
		    w = ((ws->a*ts[i]+ws->b)*ts[i]+ws->c)*ts[i]+ws->d;
		    if ( w<w0 ) w0=w;
		    if ( w>w1 ) w1=w;
		    any = true;
		}
	    }
	    if ( CubicSolve(zs,z2,ts)) {
		for ( i=0; i<2 && ts[i]!=-1; ++i ) {
		    w = ((ws->a*ts[i]+ws->b)*ts[i]+ws->c)*ts[i]+ws->d;
		    if ( w<w0 ) w0=w;
		    if ( w>w1 ) w1=w;
		    any = true;
		}
	    }
	    ts[0]=0; ts[1]=1.0;
	    for ( i=0; i<2; ++i ) {
		w = ((ws->a*ts[i]+ws->b)*ts[i]+ws->c)*ts[i]+ws->d;
		z = ((zs->a*ts[i]+zs->b)*ts[i]+zs->c)*ts[i]+zs->d;
		if ( z>=z1 && z<=z2 ) {
		    if ( w<w0 ) w0=w;
		    if ( w>w1 ) w1=w;
		    any = true;
		}
	    }
	    SplineFindExtrema(ws,&ts[0],&ts[1]);
	    for ( i=0; i<2 && ts[i]!=-1; ++i ) {
		w = ((ws->a*ts[i]+ws->b)*ts[i]+ws->c)*ts[i]+ws->d;
		z = ((zs->a*ts[i]+zs->b)*ts[i]+zs->c)*ts[i]+zs->d;
		if ( z>=z1 && z<=z2 ) {
		    if ( w<w0 ) w0=w;
		    if ( w>w1 ) w1=w;
		    any = true;
		}
	    }
	}
	ss = ss->next;
    }

    *wmin = w0; *wmax = w1;
return( any );
}

static bigreal FindZero5(bigreal w[7],bigreal tlow, bigreal thigh) {
    /* Somewhere between tlow and thigh there is a value of t where w(t)==0 */
    /*  It is conceiveable that there might be 3 such ts if there are some high frequency effects */
    /*  but I ignore that for now */
    bigreal t, test;
    int bot_negative;

    t = tlow;
    test = ((((w[5]*t+w[4])*t+w[3])*t+w[2])*t+w[1])*t + w[0];
    bot_negative = test<0;

    for (;;) {
	t = (thigh+tlow)/2;
	if ( thigh==t || tlow==t )
return( t );		/* close as we can get */
	test = ((((w[5]*t+w[4])*t+w[3])*t+w[2])*t+w[1])*t + w[0];
	if ( test==0 )
return( t );
	if ( bot_negative ) {
	    if ( test<0 )
		tlow = t;
	    else
		thigh = t;
	} else {
	    if ( test<0 )
		thigh = t;
	    else
		tlow = t;
	}
    }
}

static bigreal FindZero3(bigreal w[7],bigreal tlow, bigreal thigh) {
    /* Somewhere between tlow and thigh there is a value of t where w(t)==0 */
    /*  It is conceiveable that there might be 3 such ts if there are some high frequency effects */
    /*  but I ignore that for now */
    bigreal t, test;
    int bot_negative;

    t = tlow;
    test = ((w[3]*t+w[2])*t+w[1])*t + w[0];
    bot_negative = test<0;

    for (;;) {
	t = (thigh+tlow)/2;
	if ( thigh==t || tlow==t )
return( t );		/* close as we can get */
	test = ((w[3]*t+w[2])*t+w[1])*t + w[0];
	if ( test==0 )
return( t );
	if ( bot_negative ) {
	    if ( test<0 )
		tlow = t;
	    else
		thigh = t;
	} else {
	    if ( test<0 )
		thigh = t;
	    else
		tlow = t;
	}
    }
}

bigreal SplineMinDistanceToPoint(Spline *s, BasePoint *p) {
    /* So to find the minimum distance we want the sqrt( (sx(t)-px)^2 + (sy(t)-py)^2 ) */
    /*  Same minima as (sx(t)-px)^2 + (sy(t)-py)^2, which is easier to deal with */
    bigreal w[7];
    Spline1D *x = &s->splines[0], *y = &s->splines[1];
    bigreal off[2], best;

    off[0] = (x->d-p->x); off[1] = (y->d-p->y);

    w[6] = (x->a*x->a) + (y->a*y->a);
    w[5] = 2*(x->a*x->b + y->a*y->b);
    w[4] = (x->b*x->b) + 2*(x->a*x->c) + (y->b*y->b) + 2*(y->a*y->c);
    w[3] = 2* (x->b*x->c + x->a*off[0] + y->b*y->c + y->a*off[1]);
    w[2] = (x->c*x->c) + 2*(x->b*off[0]) + (y->c*y->c) + 2*y->b*off[1];
    w[1] = 2*(x->c*off[0] + y->c*off[1]);
    w[0] = off[0]*off[0] + off[1]*off[1];

    /* Take derivative */
    w[0] = w[1];
    w[1] = 2*w[2];
    w[2] = 3*w[3];
    w[3] = 4*w[4];
    w[4] = 5*w[5];
    w[5] = 6*w[6];
    w[6] = 0;

    if ( w[5]!=0 ) {
	bigreal tzeros[8], t, incr, test, lasttest, zerot;
	int i, zcnt=0;
	/* Well, we've got a 5th degree poly and no way to play cute tricks. */
	/* brute force it */
	incr = 1.0/1024;
	lasttest = w[0];
	for ( t = incr; t<=1.0; t += incr ) {
	    test = ((((w[5]*t+w[4])*t+w[3])*t+w[2])*t+w[1])*t + w[0];
	    if ( test==0 )
		tzeros[zcnt++] = t;
	    else {
		if ( lasttest!=0 && (test>0) != (lasttest>0) ) {
		    zerot = FindZero5(w,t-incr,t);
		    if ( zerot>0 )
			tzeros[zcnt++] = zerot;
		}
	    }
	    lasttest = test;
	}
	best = off[0]*off[0] + off[1]*off[1];		/* t==0 */
	test = (x->a+x->b+x->c+off[0])*(x->a+x->b+x->c+off[0]) +
		(y->a+y->b+y->c+off[1])*(y->a+y->b+y->c+off[1]); 	/* t==1 */
	if ( best>test ) best = test;
	for ( i=0; i<zcnt; ++i ) {
	    bigreal tx, ty;
	    tx = ((x->a*tzeros[i]+x->b)*tzeros[i]+x->c)*tzeros[i] + off[0];
	    ty = ((y->a*tzeros[i]+y->b)*tzeros[i]+y->c)*tzeros[i] + off[1];
	    test = tx*tx + ty*ty;
	    if ( best>test ) best = test;
	}
return( sqrt(best));
    } else if ( w[4]==0 && w[3]!=0 ) {
	/* Started with a quadratic -- now, find 0s of a cubic */
	/* We could find the extrema, so we have a bunch of monotonics */
	/* Or we could brute force it as above */
	bigreal tzeros[8], test, zerot;
	bigreal quad[3], disc, e[5], t1, t2;
	int i, zcnt=0, ecnt;

	quad[2] = 3*w[3]; quad[1] = 2*w[2]; quad[0] = w[1];
	disc = (-quad[1]*quad[1] - 4*quad[2]*quad[0]);
	e[0] = 0;
	if ( disc<0 ) {
	    e[1] = 1.0;
	    ecnt = 2;
	} else
	    disc = sqrt(disc);
	t1 = (-w[1] - disc) / (2*w[2]);
	t2 = (-w[1] + disc) / (2*w[2]);
	if ( t1>t2 ) {
	    bigreal temp = t1;
	    t1 = t2;
	    t2 = temp;
	}
	ecnt=1;
	if ( t1>0 && t1<1 )
	    e[ecnt++] = t1;
	if ( t2>0 && t2<1 && t1!=t2 )
	    e[ecnt++] = t2;
	e[ecnt++] = 1.0;
	for ( i=1; i<ecnt; ++i ) {
	    zerot = FindZero3(w,e[i-1],e[i]);
	    if ( zerot>0 )
		tzeros[zcnt++] = zerot;
	}
	best = off[0]*off[0] + off[1]*off[1];		/* t==0 */
	test = (x->b+x->c+off[0])*(x->b+x->c+off[0]) +
		(y->b+y->c+off[1])*(y->b+y->c+off[1]); 	/* t==1 */
	if ( best>test ) best = test;
	for ( i=0; i<zcnt; ++i ) {
	    bigreal tx, ty;
	    tx = (x->b*tzeros[i]+x->c)*tzeros[i] + off[0];
	    ty = (y->b*tzeros[i]+y->c)*tzeros[i] + off[1];
	    test = tx*tx + ty*ty;
	    if ( best>test ) best = test;
	}
return( sqrt(best));
    } else if ( w[2]==0 && w[1]!=0 ) {
	/* Started with a line */
	bigreal t = -w[0]/w[1], test, best;
	best = off[0]*off[0] + off[1]*off[1];		/* t==0 */
	test = (x->c+off[0])*(x->c+off[0]) + (y->c+off[1])*(y->c+off[1]); 	/* t==1 */
	if ( best>test ) best = test;
	if ( t>0 && t<1 ) {
	    test = (x->c*t+off[0])*(x->c*t+off[0]) + (y->c*t+off[1])*(y->c*t+off[1]);
	    if ( best>test ) best = test;
	}
return(sqrt(best));
    } else if ( w[4]!=0 && w[3]!=0 && w[2]!=0 && w[1]!=0 ) {
	IError( "Impossible condition in SplineMinDistanceToPoint");
    } else {
	/* It's a point, minimum distance is the only distance */
return( sqrt(off[0]*off[0] + off[1]*off[1]) );
    }
return( -1 );
}

void GrowBuffer(GrowBuf *gb) {
    if ( gb->base==NULL ) {
	gb->base = gb->pt = malloc(200);
	gb->end = gb->base + 200;
    } else {
	int len = (gb->end-gb->base) + 400;
	int off = gb->pt-gb->base;
	gb->base = realloc(gb->base,len);
	gb->end = gb->base + len;
	gb->pt = gb->base+off;
    }
}

void GrowBufferAdd(GrowBuf *gb,int ch) {
    if ( gb->base==NULL ) {
	gb->base = gb->pt = malloc(200);
	gb->end = gb->base + 200;
    } else if ( gb->pt>=gb->end ) {
	int len = (gb->end-gb->base) + 400;
	int off = gb->pt-gb->base;
	gb->base = realloc(gb->base,len);
	gb->end = gb->base + len;
	gb->pt = gb->base+off;
    }
    *gb->pt++ = ch;
}

void GrowBufferAddStr(GrowBuf *gb,char *str) {
    int n;

    if ( str==NULL )
return;
    n = strlen(str);

    if ( gb->base==NULL ) {
	gb->base = gb->pt = malloc(200+n);
	gb->end = gb->base + 200+n;
    } else if ( gb->pt+n+1>=gb->end ) {
	int len = (gb->end-gb->base) + n+200;
	int off = gb->pt-gb->base;
	gb->base = realloc(gb->base,len);
	gb->end = gb->base + len;
	gb->pt = gb->base+off;
    }
    strcpy((char *)gb->pt, str);
    gb->pt += n;
}

bigreal DistanceBetweenPoints( BasePoint *p1, BasePoint *p2 )
{
    bigreal t = pow(p1->x - p2->x,2) + pow(p1->y - p2->y,2);
    if( !t )
	return t;

    t = sqrt( t );
    return t;
}

