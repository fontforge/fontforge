/* Copyright (C) 2001 by George Williams */
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

#include "ttfmod.h"
#include <string.h>
#include <math.h>

static int default_to_Apple = 0;	/* Normally we assume the MS interpretation */
		/* At the moment this only controls composit offsets */

#define _On_Curve	1
#define _X_Short	2
#define _Y_Short	4
#define _Repeat		8
#define _X_Same		0x10
#define _Y_Same		0x20

#ifndef chunkalloc
#define ALLOC_CHUNK	100
#define CHUNK_MAX	100

#if ALLOC_CHUNK>1
struct chunk { struct chunk *next; };
static struct chunk *chunklists[CHUNK_MAX/4] = { 0 };
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
    index = size>>2;
    if ( chunklists[index]==NULL ) {
	char *pt, *end;
	pt = galloc(ALLOC_CHUNK*size);
	chunklists[index] = (struct chunk *) pt;
	end = pt+(ALLOC_CHUNK-1)*size;
	while ( pt<end ) {
	    ((struct chunk *) pt)->next = (struct chunk *) (pt + size);
	    pt += size;
	}
	((struct chunk *) pt)->next = NULL;
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
	chunklists[size>>2] = (struct chunk *) item;
    }
# endif
}
#endif

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

void ConicFree(Conic *conic) {
    LinearApproxFree(conic->approx);
    chunkfree(conic,sizeof(Conic));
}

void ConicPointFree(ConicPoint *sp) {
    chunkfree(sp,sizeof(ConicPoint));
}

void ConicPointListFree(ConicPointList *spl) {
    Conic *first, *conic, *next;

    if ( spl==NULL )
return;
    first = NULL;
    for ( conic = spl->first->next; conic!=NULL && conic!=first; conic = next ) {
	next = conic->to->next;
	chunkfree(conic->to->prevcp,sizeof(BasePoint));
	ConicPointFree(conic->to);
	ConicFree(conic);
	if ( first==NULL ) first = conic;
    }
    if ( spl->last!=spl->first || spl->first->next==NULL )
	ConicPointFree(spl->first);
    chunkfree(spl,sizeof(ConicPointList));
}

void ConicPointListsFree(ConicPointList *head) {
    ConicPointList *spl, *next;

    for ( spl=head; spl!=NULL; spl=next ) {
	next = spl->next;
	ConicPointListFree(spl);
    }
}

void RefCharFree(RefChar *ref) {

    if ( ref==NULL )
return;
    ConicPointListsFree(ref->conics);
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

void ConicCharFree(ConicChar *cc) {
    struct coniccharlist *dlist, *dnext;

    if ( cc==NULL )
return;
    ConicPointListsFree(cc->conics);
    RefCharsFree(cc->refs);
    for ( dlist=cc->dependents; dlist!=NULL; dlist = dnext ) {
	dnext = dlist->next;
	chunkfree(dlist,sizeof(struct coniccharlist));
    }
    chunkfree(cc,sizeof(ConicChar));
}

void ConicRefigure(Conic *conic) {
    ConicPoint *from = conic->from, *to = conic->to;
    Conic1D *xsp = &conic->conics[0], *ysp = &conic->conics[1];
    real t, y;

#ifdef DEBUG
    if ( RealNear(from->me.x,to->me.x) && RealNear(from->me.y,to->me.y))
	GDrawIError("Zero length conic created");
#endif
    if ( from->nextcp!=to->prevcp )
	GDrawIError( "Inconsistancy in ConicRefigure" );
    xsp->c = from->me.x; ysp->c = from->me.y;
    conic->islinear = ( from->nextcp==NULL );
    if ( from->nextcp!=NULL ) {
	if ( from->me.x==to->me.x ) {
	    if ( from->me.x==from->nextcp->x &&
		    ((from->nextcp->y>=from->me.y && from->nextcp->y<=to->me.y) ||
		     (from->nextcp->y<=from->me.y && from->nextcp->y>=to->me.y)) )
		conic->islinear = true;
	} else if ( from->me.y==to->me.y ) {
	    if ( from->me.y==from->nextcp->y &&
		    ((from->nextcp->x>=from->me.x && from->nextcp->x<=to->me.x) ||
		     (from->nextcp->x<=from->me.x && from->nextcp->x>=to->me.x)) )
		conic->islinear = true;
	} else {
	    t = (from->nextcp->x-from->me.x)/(to->me.x-from->me.x);
	    y = t*(to->me.y-from->me.y) + from->me.y;
	    if ( rint(y)==rint(from->nextcp->y) )
		conic->islinear = true;
	}
    }
    if ( conic->islinear ) {
	xsp->a = ysp->a = 0;
	xsp->b = to->me.x-from->me.x; ysp->b = to->me.y-from->me.y;
    } else {
	xsp->b = 2*from->nextcp->x-2*from->me.x;
	xsp->a = to->me.x+from->me.x-2*from->nextcp->x;
	ysp->b = 2*from->nextcp->y-2*from->me.y;
	ysp->a = to->me.y+from->me.y-2*from->nextcp->y;
	if ( RealNear(xsp->b,0)) xsp->b=0;
	if ( RealNear(ysp->b,0)) ysp->b=0;
	if ( RealNear(xsp->a,0)) xsp->a=0;
	if ( RealNear(ysp->a,0)) ysp->a=0;
	if ( xsp->a==0 && ysp->a==0 )
	    conic->islinear = true;		/* Can this happen? */
    }
    if ( isnan(ysp->a) || isnan(xsp->a) )
	GDrawIError("NaN value in conic creation");
    LinearApproxFree(conic->approx);
    conic->approx = NULL;
}

Conic *ConicMake(ConicPoint *from, ConicPoint *to) {
    Conic *conic = chunkalloc(sizeof(Conic));

    conic->from = from; conic->to = to;
    from->next = to->prev = conic;
    ConicRefigure(conic);
return( conic );
}

static void TransformPoint(BasePoint *bp, real transform[6]) {
    BasePoint p;
    p.x = transform[0]*bp->x + transform[2]*bp->y + transform[4];
    p.y = transform[1]*bp->x + transform[3]*bp->y + transform[5];
    p.x = rint(1024*p.x)/1024;
    p.y = rint(1024*p.y)/1024;
    p.pnum = bp->pnum;
    *bp = p;
}

ConicPointList *ConicPointListCopy(ConicPointList *base) {
    ConicPointList *spl;
    ConicPointList *hspl, *lspl=NULL, *cspl;
    ConicPoint *spt, *pfirst, *csp, *last;
    /* Note that this preserves the pnum field in the conic/basepoints */

    for ( spl = base; spl!=NULL; spl = spl->next ) {
	cspl = chunkalloc(sizeof(ConicPointList));
	if ( lspl==NULL ) hspl = cspl;
	else lspl->next = cspl;
	lspl = cspl;

	pfirst = NULL; last = NULL;
	for ( spt = spl->first ; spt!=pfirst; spt = spt->next->to ) {
	    csp = chunkalloc(sizeof(ConicPoint));
	    *csp = *spt;
	    if ( csp->nextcp!=NULL ) {
		csp->nextcp = chunkalloc(sizeof(BasePoint));
		*csp->nextcp = *spt->nextcp;
	    }
	    if ( pfirst==NULL ) {
		pfirst = spt;
		cspl->first = cspl->last = csp;
		csp->prevcp = NULL;		/* for now */
	    } else {
		csp->prevcp = last->nextcp;
		ConicMake(last,csp);
	    }
	    last = csp;
	    if ( spt->next==NULL ) {
		cspl->last = csp;
	break;
	    }
	}
	if ( spt==pfirst ) {
	    cspl->first->prevcp = last->nextcp;
	    ConicMake(last,cspl->first);
	}
    }
return( hspl );
}

ConicPointList *ConicPointListTransform(ConicPointList *base, real transform[6]) {
    Conic *conic, *first;
    ConicPointList *spl;
    ConicPoint *spt, *pfirst;

    for ( spl = base; spl!=NULL; spl = spl->next ) {
	pfirst = NULL;
	for ( spt = spl->first ; spt!=pfirst; spt = spt->next->to ) {
	    if ( pfirst==NULL ) pfirst = spt;
	    TransformPoint(&spt->me,transform);
	    if ( spt->nextcp != NULL )
		TransformPoint(spt->nextcp,transform);
	    if ( spt->next==NULL )
	break;
	}
	first = NULL;
	for ( conic = spl->first->next; conic!=NULL && conic!=first; conic=conic->to->next ) {
	    ConicRefigure(conic);
	    if ( first==NULL ) first = conic;
	}
    }
return( base );
}

static BasePoint *CCGetTTFPoint(ConicChar *cc,int pnum) {
    ConicPointList *spl;
    ConicPoint *sp;
    RefChar *rf;

    if ( cc!=NULL )
return( NULL );

    for ( spl = cc->conics; spl!=NULL; spl = spl->next ) {
	for ( sp = spl->first; ; ) {
	    if ( sp->me.pnum==pnum )
return( &sp->me );
	    if ( sp->nextcp!=NULL && sp->nextcp->pnum==pnum )
return( sp->nextcp );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
    for ( rf=cc->refs; rf!=NULL; rf = rf->next ) {
	for ( spl = rf->conics; spl!=NULL; spl = spl->next ) {
	    for ( sp = spl->first; ; ) {
		if ( sp->me.pnum==pnum )
return( &sp->me );
		if ( sp->nextcp!=NULL && sp->nextcp->pnum==pnum )
return( sp->nextcp );
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    }
return( NULL );
}

void RefCharRefigure(RefChar *ref) {
    ConicSet *last = NULL, *temp;
    RefChar *rf;

    ConicPointListsFree(ref->conics);
    ref->conics = ConicPointListTransform(ConicPointListCopy(ref->cc->conics),ref->transform);
    for ( last=ref->conics; last!=NULL && last->next!=NULL; last=last->next );
    /* in correct truetype the following should never happen */
    for ( rf = ref->cc->refs; rf!=NULL; rf=rf->next ) {
	temp = ConicPointListTransform(rf->conics,ref->transform);
	if ( last==NULL )
	    ref->conics = temp;
	else
	    last->next = temp;
	if ( temp==NULL )
	    /* last doesn't change */;
	else {
	    while ( temp->next!=NULL )
		temp = temp->next;
	    last = temp;
	}
    }
    ConicSetFindBounds(ref->conics,&ref->bb);
}

void RefCharAddDependant(RefChar *ref,ConicChar *referer) {
    struct coniccharlist *list;

    for ( list=ref->cc->dependents; list!=NULL && list->cc!=referer; list=list->next );
    if ( list!=NULL )		/* Already there */
return;
    list = chunkalloc(sizeof(struct coniccharlist));
    list->next = ref->cc->dependents;
    list->cc = referer;
    ref->cc->dependents = list;
}

static void AttachControls(ConicPoint *from, ConicPoint *to, BasePoint *cp) {
    from->nextcp = to->prevcp = chunkalloc(sizeof(BasePoint));
    *from->nextcp = *cp;
}

/* This section copied out of parsettf.c */
static ConicSet *ttfbuildcontours(int path_cnt,uint16 *endpt, char *flags,
	BasePoint *pts) {
    ConicSet *head=NULL, *last=NULL, *cur;
    int i, path, start, last_off;
    ConicPoint *sp;

    for ( path=i=0; path<path_cnt; ++path ) {
	if ( endpt[path]<i )	/* Sigh. Yes there are fonts with bad endpt info */
    continue;
	cur = chunkalloc(sizeof(ConicSet));
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	last_off = false;
	start = i;
	while ( i<=endpt[path] ) {
	    if ( flags[i]&_On_Curve ) {
		sp = chunkalloc(sizeof(ConicPoint));
		sp->me = pts[i];
		sp->nextcp = sp->prevcp = NULL;
		if ( last_off && cur->last!=NULL )
		    AttachControls(cur->last,sp,&pts[i-1]);
		last_off = false;
	    } else if ( last_off ) {
		/* two off curve points get a third on curve point created */
		/* half-way between them. Now isn't that special */
		sp = chunkalloc(sizeof(ConicPoint));
		sp->me.x = (pts[i].x+pts[i-1].x)/2;
		sp->me.y = (pts[i].y+pts[i-1].y)/2;
		sp->me.pnum = -1;
		sp->nextcp = sp->prevcp = NULL;
		if ( last_off && cur->last!=NULL )
		    AttachControls(cur->last,sp,&pts[i-1]);
		/* last_off continues to be true */
	    } else {
		last_off = true;
		sp = NULL;
	    }
	    if ( sp!=NULL ) {
		if ( cur->first==NULL )
		    cur->first = sp;
		else
		    ConicMake(cur->last,sp);
		cur->last = sp;
	    }
	    ++i;
	}
	if ( start==i-1 ) {
	    /* MS chinese fonts have contours consisting of a single off curve*/
	    /*  point. What on earth do they think that means? */
	    sp = chunkalloc(sizeof(ConicPoint));
	    sp->me.x = pts[start].x;
	    sp->me.y = pts[start].y;
	    cur->first = cur->last = sp;
	} else if ( !(flags[start]&_On_Curve) && !(flags[i-1]&_On_Curve) ) {
	    sp = chunkalloc(sizeof(ConicPoint));
	    sp->me.x = (pts[start].x+pts[i-1].x)/2;
	    sp->me.y = (pts[start].y+pts[i-1].y)/2;
	    sp->me.pnum = -1;
	    sp->nextcp = sp->prevcp = NULL;
	    AttachControls(cur->last,sp,&pts[i-1]);
	    ConicMake(cur->last,sp);
	    cur->last = sp;
	    AttachControls(sp,cur->first,&pts[start]);
	} else if ( !(flags[i-1]&_On_Curve))
	    AttachControls(cur->last,cur->first,&pts[i-1]);
	else if ( !(flags[start]&_On_Curve) )
	    AttachControls(cur->last,cur->first,&pts[start]);
	ConicMake(cur->last,cur->first);
	cur->last = cur->first;
    }
return( head );
}

static void readttfsimpleglyph(FILE *ttf,ConicChar *cc, int path_cnt) {
    uint16 *endpt = galloc((path_cnt+1)*sizeof(uint16));
    char *flags;
    BasePoint *pts;
    int i, j, tot;
    int last_pos;

    for ( i=0; i<path_cnt; ++i )
	endpt[i] = getushort(ttf);
    tot = endpt[path_cnt-1]+1;
    pts = galloc(tot*sizeof(BasePoint));
    for ( i=0; i<tot; ++i )
	pts[i].pnum = i;

    cc->instrdata.in_composit = cc->refs!=NULL;
    cc->instrdata.instr_cnt = getushort(ttf);
    cc->instrdata.instrs = galloc(cc->instrdata.instr_cnt);
    for ( i=0; i<cc->instrdata.instr_cnt; ++i )
	cc->instrdata.instrs[i] = getc(ttf);

    flags = galloc(tot);
    for ( i=0; i<tot; ++i ) {
	flags[i] = getc(ttf);
	if ( flags[i]&_Repeat ) {
	    int cnt = getc(ttf);
	    if ( cnt==EOF ) {
		GDrawIError("Unexpected End of File when reading a simple glyph\n" );
    break;
	    }
	    if ( i+cnt>=tot )
		GDrawIError("Flag count is wrong (or total is): %d %d", i+cnt, tot );
	    for ( j=0; j<cnt; ++j )
		flags[i+j+1] = flags[i];
	    i += cnt;
	}
    }
    if ( i!=tot )
	GDrawIError("Flag count is wrong (or total is): %d %d", i, tot );

    last_pos = 0;
    for ( i=0; i<tot; ++i ) {
	if ( flags[i]&_X_Short ) {
	    int off = getc(ttf);
	    if ( !(flags[i]&_X_Same ) )
		off = -off;
	    pts[i].x = last_pos + off;
	} else if ( flags[i]&_X_Same )
	    pts[i].x = last_pos;
	else
	    pts[i].x = last_pos + (short) getushort(ttf);
	last_pos = pts[i].x;
    }

    last_pos = 0;
    for ( i=0; i<tot; ++i ) {
	if ( flags[i]&_Y_Short ) {
	    int off = getc(ttf);
	    if ( !(flags[i]&_Y_Same ) )
		off = -off;
	    pts[i].y = last_pos + off;
	} else if ( flags[i]&_Y_Same )
	    pts[i].y = last_pos;
	else
	    pts[i].y = last_pos + (short) getushort(ttf);
	last_pos = pts[i].y;
    }

    cc->conics = ttfbuildcontours(path_cnt,endpt,flags,pts);
    cc->point_cnt = tot;
    CCCatagorizePoints(cc);
    free(endpt);
    free(flags);
    free(pts);
}

#define _ARGS_ARE_WORDS	1
#define _ARGS_ARE_XY	2
#define _ROUND		4		/* offsets rounded to grid */
#define _SCALE		8
#define _MORE		0x20
#define _XY_SCALE	0x40
#define _MATRIX		0x80
#define _INSTR		0x100
#define _MY_METRICS	0x200
#define _OVERLAP_COMPOUND	0x400	/* Used in Apple GX fonts */
	    /* Means the componants overlap (which? this one and what other?) */
/* These two described in OpenType specs, not by Apple yet */
#define _SCALED_OFFSETS		0x800	/* Use Apple definition of offset interpretation */
#define _UNSCALED_OFFSETS	0x1000	/* Use MS definition */


static void readttfcompositglyph(FILE *ttf,ConicFont *cf,ConicChar *cc, int32 end) {
    RefChar *head=NULL, *last=NULL, *cur;
    int flags, arg1, arg2;
    int i, pnum;
    ConicPointList *cpl;
    ConicPoint *sp;

    do {
	if ( ftell(ttf)>=end ) {
	    fprintf( stderr, "Bad flags value, implied MORE components at end of glyph\n" );
    break;
	}
	cur = chunkalloc(sizeof(RefChar));
	flags = getushort(ttf);
	cur->glyph = getushort(ttf);
	if ( flags&_ARGS_ARE_WORDS ) {
	    arg1 = (short) getushort(ttf);
	    arg2 = (short) getushort(ttf);
	} else {
	    arg1 = (signed char) getc(ttf);
	    arg2 = (signed char) getc(ttf);
	}
	if ( flags & _ARGS_ARE_XY ) {
	    cur->transform[4] = arg1;
	    cur->transform[5] = arg2;
	    /* There is some very strange stuff (half-)documented on the apple*/
	    /*  site about how these should be interpretted when there are */
	    /*  scale factors, or rotations */
	    /* It isn't well enough described to be comprehensible */
	    /*  http://fonts.apple.com/TTRefMan/RM06/Chap6glyf.html */
	    /* Microsoft says nothing about this */
	    /* Adobe implies this is a difference between MS and Apple */
	    /*  MS doesn't do this, Apple does (GRRRGH!!!!) */
	    /* Adobe says that setting bit 12 means that this will not happen */
	    /* Adobe says that setting bit 11 means that this will happen */
	    /*  So if either bit is set we know when this happens, if neither */
	    /*  we guess... But I still don't know how to interpret the */
	    /*  apple mode under rotation... */
	    /* I notice that FreeType does nothing about rotation nor does it */
	    /*  interpret bits 11&12 */
	} else {
	    /* Somehow we can get offsets by looking at the points in the */
	    /*  points so far generated and comparing them to the points in */
	    /*  the current componant */
	    /* How exactly is not described on any of the Apple, MS, Adobe */
	    /* freetype looks up arg1 in the set of points we've got so far */
	    /*  looks up arg2 in the new component (before renumbering) */
	    /*  offset.x = arg1.x - arg2.x; offset.y = arg1.y - arg2.y; */
	    /* Sadly I don't retain the information I need to deal with this */
	    /*  I lose the point numbers and the control points... */
	    cur->point_match = true;
	    fprintf( stderr, "TTF IError: I don't understand matching points. Please send me a copy\n" );
	    fprintf( stderr, "  of whatever ttf file you are looking at. Thanks.  gww@silcom.com\n" );
	}
	cur->transform[0] = cur->transform[3] = 1.0;
	if ( flags & _SCALE )
	    cur->transform[0] = cur->transform[3] = get2dot14(ttf);
	else if ( flags & _XY_SCALE ) {
	    cur->transform[0] = get2dot14(ttf);
	    cur->transform[3] = get2dot14(ttf);
	} else if ( flags & _MATRIX ) {
	    cur->transform[0] = get2dot14(ttf);
	    cur->transform[1] = get2dot14(ttf);
	    cur->transform[2] = get2dot14(ttf);
	    cur->transform[3] = get2dot14(ttf);
	}
	/* If neither SCALED/UNSCALED specified I'll just assume MS interpretation */
	/*  because I at least understand that method */
	if ( (default_to_Apple || (flags & _SCALED_OFFSETS)) && (flags & _ARGS_ARE_XY)) {
	    cur->transform[4] *= cur->transform[0];
	    cur->transform[5] *= cur->transform[1];
	    if ( (RealNear(cur->transform[1],cur->transform[2]) ||
		    RealNear(cur->transform[1],-cur->transform[2])) &&
		    !RealNear(cur->transform[1],0)) {
		/* If it's rotated by a 45 degree angle then multiply by 2 */
		/*  (presumably not if it's rotated by a multiple of 90) */
		/*  God knows why, and God knows what we do if it's not a multiple of 45 */
		cur->transform[4] *= 2;
		cur->transform[5] *= 2;
	    }
	    fprintf( stderr, "TTF IError: I don't understand Apple's scaled offsets. Please send me a copy\n" );
	    fprintf( stderr, "  of whatever ttf file you are loading. Thanks.  gww@silcom.com\n" );
	}
	cur->use_my_metrics = (flags&_MY_METRICS)?1:0;
	cur->round = (flags&_ROUND)?1:0;
	if ( head==NULL )
	    head = cur;
	else
	    last->next = cur;
	last = cur;
	if ( feof(ttf)) {
	    fprintf(stderr, "Reached end of file when reading composit glyph\n" );
    break;
	}
    } while ( flags&_MORE );
    cc->refs = head;
    if ( flags&_INSTR ) {
	cc->instrdata.instr_cnt = getushort(ttf);
	cc->instrdata.instrs = galloc(cc->instrdata.instr_cnt);
	for ( i=0; i<cc->instrdata.instr_cnt; ++i )
	    cc->instrdata.instrs[i] = getc(ttf);
    }
    for ( cur = head; cur!=NULL; cur = cur->next ) {
	cur->cc = LoadGlyph(cf,cur->glyph);
	if ( cur->point_match ) {
	    BasePoint *p1, *p2;
	    p1 = CCGetTTFPoint(cc,cur->transform[4]);
	    p2 = CCGetTTFPoint(cur->cc,cur->transform[5]);
	    if ( p1==NULL || p2==NULL ) {
		fprintf( stderr, "Could not do a point match when !ARGS_ARE_XY\n" );
		cur->transform[4] = cur->transform[5] = 0;
	    } else {
		cur->transform[4] = p1->x-p2->x;
		cur->transform[5] = p1->y-p2->y;
	    }
	    cur->point_match = false;
	}
	RefCharRefigure(cur);
	RefCharAddDependant(cur,cc);
    }

    pnum = 0;
    for ( cur = head; cur!=NULL; cur = cur->next ) {
	for ( cpl = cur->conics; cpl!=NULL ; cpl = cpl->next ) {
	    sp = cpl->first;
	    while ( 1 ) {
		if ( sp->me.pnum!=-1 )
		    sp->me.pnum = pnum++;
		if ( sp->nextcp != NULL )
		    sp->nextcp->pnum = pnum++;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==cpl->first )
	    break;
	    }
	}
    }
    cc->point_cnt = pnum;
}
/* End section copied out of parsettf.c */

ConicChar *LoadGlyph(ConicFont *cf, int glyph) {
    int path_cnt;
    FILE *ttf;
    ConicChar *cc;

    if ( glyph<0 || glyph>=cf->glyph_cnt )
return( NULL );
    if ( (cc = cf->chars[glyph])->loaded )
return( cc );
    cc->loaded = true;
    if ( cc->glyph_len==0 ) {
	/* glyphs with no size are like space, empty, and don't even have a */
	/*  path count. Just mark it as read (loaded) */
return( cc );
    }
    ttf = cf->glyphs->container->file;

    fseek(ttf,cf->glyphs->start+cc->glyph_offset,SEEK_SET);
    path_cnt = (short) getushort(ttf);
    /* xmin = */ getushort(ttf);
    /* ymin = */ getushort(ttf);
    /* xmax = */ getushort(ttf);
    /* ymax = */ getushort(ttf);
    if ( path_cnt>=0 )
	readttfsimpleglyph(ttf,cc,path_cnt);
    else
	readttfcompositglyph(ttf,cf,cc,cf->glyphs->start+cc->glyph_offset+cc->glyph_len);
return( cc );
}

ConicFont *LoadConicFont(TtfFont *tfont) {
    int i, mcnt, start, next, lcnt;
    Table *glyphs=NULL, *loca=NULL, *hmetrics=NULL, *hheader=NULL, *head;
    ConicFont *cf;

    for ( i=0; i<tfont->tbl_cnt; ++i ) {
	if ( tfont->tbls[i]->name==CHR('l','o','c','a') )
	    loca = tfont->tbls[i];
	else if ( tfont->tbls[i]->name==CHR('g','l','y','f') )
	    glyphs = tfont->tbls[i];
    	else if ( tfont->tbls[i]->name==CHR('h','m','t','x') )
	    hmetrics = tfont->tbls[i];
    	else if ( tfont->tbls[i]->name==CHR('h','h','e','a') )
	    hheader = tfont->tbls[i];
    	else if ( tfont->tbls[i]->name==CHR('h','e','a','d') )
	    head = tfont->tbls[i];
    }

    if ( loca==NULL || glyphs==NULL || hmetrics==NULL || hheader==NULL || head==NULL )
return( NULL );
    TableFillup(hheader);
    TableFillup(hmetrics);
    TableFillup(loca);
    TableFillup(head);

    lcnt = loca->len;
    if ( ptgetushort(head->data+50))		/* Long format */
	lcnt /= sizeof(int32);
    else
	lcnt /= sizeof(int16);
    --lcnt;
    if ( lcnt!=tfont->glyph_cnt ) {
	GDrawError("TTF Font has bad glyph count field. maxp says: %d sizeof(loca)=>%d", tfont->glyph_cnt, lcnt);
	if ( lcnt<tfont->glyph_cnt )
	    tfont->glyph_cnt = lcnt;
    }

    cf = gcalloc(1,sizeof(ConicFont));
    cf->glyph_cnt = tfont->glyph_cnt;
    cf->tfont = tfont;
    cf->tfile = tfont->container;
    cf->glyphs = glyphs;
    cf->loca = loca;
    cf->chars = galloc(cf->glyph_cnt*sizeof(ConicChar*));

    for ( i=0; i<cf->glyph_cnt; ++i ) {
	cf->chars[i] = chunkalloc(sizeof(ConicChar));
	cf->chars[i]->parent = cf;
	cf->chars[i]->glyph = i;
    }

    mcnt = ptgetushort(hheader->data+34);
    for ( i=0; i<mcnt; ++i )
	cf->chars[i]->width = ptgetushort(hmetrics->data+4*i);
    for ( i=mcnt; i<cf->glyph_cnt; ++i )
	cf->chars[i]->width = cf->chars[mcnt-1]->width;

    cf->em = ptgetushort(head->data+18);	/* units per em */
    if ( ptgetushort(head->data+50)) {		/* Long format */
	start = ptgetlong(loca->data);
	for ( i=0; i<cf->glyph_cnt; ++i ) {
	    next = ptgetlong(loca->data+4*i+4);
	    cf->chars[i]->glyph_offset = start;
	    cf->chars[i]->glyph_len = next-start;
	    start = next;
	}
    } else {					/* Short format */
	/* since low bit is always 0, short format hides it, so must multiply by 2 */
	start = ptgetushort(loca->data);
	for ( i=0; i<cf->glyph_cnt; ++i ) {
	    next = ptgetushort(loca->data+2*i+2);
	    cf->chars[i]->glyph_offset = 2*start;
	    cf->chars[i]->glyph_len = 2*(next-start);
	    start = next;
	}
    }

    free(loca->data); loca->data = NULL;		/* loca table is never worth looking at */
return( cf );
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

static real SolveQuad(real a,real b,real c,real err, real from,int bigger) {
    real temp, best;
    real ts[4];
    int i;

    if ( bigger )
	ts[0]=ts[1]=ts[2]=ts[4] = -1;
    else
	ts[0]=ts[1]=ts[2]=ts[4] = 2;

    if ( a!=0 ) {
	temp = b*b-4*a*(c+err);
	if ( temp==0 )
	    ts[0] = -b/(2*a);
	else if ( temp>0 ) {
	    temp = sqrt(temp);
	    ts[0] = (-b+temp)/(2*a);
	    ts[1] = (-b-temp)/(2*a);
	}
	temp = b*b-4*a*(c-err);
	if ( temp==0 )
	    ts[2] = -b/(2*a);
	else if ( temp>0 ) {
	    temp = sqrt(temp);
	    ts[2] = (-b+temp)/(2*a);
	    ts[3] = (-b-temp)/(2*a);
	}
	if ( bigger ) {
	    best = 1;
	    for ( i=0; i<4; ++i )
		if ( ts[i]>from && ts[i]<best ) best = ts[i];
	} else {
	    best = 0;
	    for ( i=0; i<4; ++i )
		if ( ts[i]<from && ts[i]>best ) best = ts[i];
	}
return( best );
    }
return( bigger ? 1 : 0 );
}

LinearApprox *ConicApproximate(Conic *conic, real scale) {
    LinearApprox *test;
    LineList *cur, *last=NULL, *prev;
    real tx,ty,t;
    real slpx, slpy;
    real intx, inty;

    for ( test = conic->approx; test!=NULL && test->scale!=scale; test = test->next );
    if ( test!=NULL )
return( test );

    test = chunkalloc(sizeof(LinearApprox));
    test->scale = scale;
    test->next = conic->approx;
    conic->approx = test;

    cur = chunkalloc(sizeof(LineList) );
    cur->here.x = rint(conic->from->me.x*scale);
    cur->here.y = rint(conic->from->me.y*scale);
    test->lines = last = cur;
    if ( conic->islinear ) {
	cur = chunkalloc(sizeof(LineList) );
	cur->here.x = rint(conic->to->me.x*scale);
	cur->here.y = rint(conic->to->me.y*scale);
	last->next = cur;
    } else {
	/* find t so that (xt,yt) is a half pixel off from (cx*t+dx,cy*t+dy) */
	/* min t of scale*(ax*t^3+bx*t^2)==.5, scale*(ay*t^3+by*t^2)==.5 */
	/* I do this from both ends in. this is because I miss essential */
	/*  symmetry if I go from one end to the other. */
	/* first start at 0 and go to .5, the first linear approximation is easy */
	/*  it's just the function itself ignoring higher orders, so the error*/
	/*  is just the higher orders */
	if ( (tx = .5/(scale*conic->conics[0].a))<0 ) tx = -tx;
	tx = sqrt(tx);
	if ( (ty = .5/(scale*conic->conics[1].a))<0 ) ty = -ty;
	ty = sqrt(ty);
	t = (tx<ty)?tx:ty;
	if ( t>=1 ) t = 1;
	cur = chunkalloc(sizeof(LineList) );
	cur->here.x = rint( ((conic->conics[0].a*t+conic->conics[0].b)*t+conic->conics[0].c)*scale );
	cur->here.y = rint( ((conic->conics[1].a*t+conic->conics[1].b)*t+conic->conics[1].c)*scale );
	last->next = cur;
	last = cur;
	while ( t<.5 ) {
	    slpx = 2*conic->conics[0].a*t+conic->conics[0].b;
	    slpy = 2*conic->conics[1].a*t+conic->conics[1].b;
	    intx = (conic->conics[0].a*t+conic->conics[0].b-slpx)*t+conic->conics[0].c;
	    inty = (conic->conics[1].a*t+conic->conics[1].b-slpy)*t+conic->conics[1].c;
	    tx = SolveQuad(conic->conics[0].a,conic->conics[0].b-slpx,conic->conics[0].c-intx,.5/scale,t,true);
	    ty = SolveQuad(conic->conics[1].a,conic->conics[1].b-slpy,conic->conics[1].c-inty,.5/scale,t,true);
	    t = (tx<ty)?tx:ty;
	    if ( t>.5 ) t=.5;
	    cur = chunkalloc(sizeof(LineList));
	    cur->here.x = rint( ((conic->conics[0].a*t+conic->conics[0].b)*t+conic->conics[0].c)*scale );
	    cur->here.y = rint( ((conic->conics[1].a*t+conic->conics[1].b)*t+conic->conics[1].c)*scale );
	    last->next = cur;
	    last = cur;
	}

	/* Now start at t=1 and work back to t=.5 */
	prev = NULL;
	cur = chunkalloc(sizeof(LineList) );
	cur->here.x = rint(conic->to->me.x*scale);
	cur->here.y = rint(conic->to->me.y*scale);
	prev = cur;
	t=1.0;
	while ( 1 ) {
	    slpx = 2*conic->conics[0].a*t+conic->conics[0].b;
	    slpy = 2*conic->conics[1].a*t+conic->conics[1].b;
	    intx = (conic->conics[0].a*t+conic->conics[0].b-slpx)*t+conic->conics[0].c;
	    inty = (conic->conics[1].a*t+conic->conics[1].b-slpy)*t+conic->conics[1].c;
	    tx = SolveQuad(conic->conics[0].a,conic->conics[0].b-slpx,conic->conics[0].c-intx,.5/scale,t,false);
	    ty = SolveQuad(conic->conics[1].a,conic->conics[1].b-slpy,conic->conics[1].c-inty,.5/scale,t,false);
	    t = (tx>ty)?tx:ty;
	    if ( t<.5 ) t=.5;
	    cur = chunkalloc(sizeof(LineList) );
	    cur->here.x = rint( ((conic->conics[0].a*t+conic->conics[0].b)*t+conic->conics[0].c)*scale );
	    cur->here.y = rint( ((conic->conics[1].a*t+conic->conics[1].b)*t+conic->conics[1].c)*scale );
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

static void ConicFindBounds(Conic *sp, DBounds *bounds) {
    real t, v;
    real min, max;
    Conic1D *sp1;
    int i;

    /* first try the end points */
    for ( i=0; i<2; ++i ) {
	sp1 = &sp->conics[i];
	if ( i==0 ) {
	    if ( sp->to->me.x<bounds->minx ) bounds->minx = sp->to->me.x;
	    if ( sp->to->me.x>bounds->maxx ) bounds->maxx = sp->to->me.x;
	    min = bounds->minx; max = bounds->maxx;
	} else {
	    if ( sp->to->me.y<bounds->miny ) bounds->miny = sp->to->me.y;
	    if ( sp->to->me.y>bounds->maxy ) bounds->maxy = sp->to->me.y;
	    min = bounds->miny; max = bounds->maxy;
	}

	/* then try the extrema of the conic (assuming they are between t=(0,1) */
	if ( sp1->a!=0 ) {
	    t = -sp1->b/(2.0*sp1->a);
	    if ( t>0 && t<1 ) {
		v = (sp1->a*t+sp1->b)*t + sp1->c;
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

static void _ConicSetFindBounds(ConicPointList *spl, DBounds *bounds) {
    Conic *conic, *first;

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
	for ( conic = spl->first->next; conic!=NULL && conic!=first; conic=conic->to->next ) {
	    ConicFindBounds(conic,bounds);
	    if ( first==NULL ) first = conic;
	}
    }
}

void ConicSetFindBounds(ConicPointList *spl, DBounds *bounds) {
    memset(bounds,'\0',sizeof(*bounds));
    _ConicSetFindBounds(spl,bounds);
}

void ConicCharFindBounds(ConicChar *cc,DBounds *bounds) {
    RefChar *rf;

    /* a char with no conics (ie. a space) must have an lbearing of 0 */
    bounds->minx = bounds->maxx = 0;
    bounds->miny = bounds->maxy = 0;

    for ( rf=cc->refs; rf!=NULL; rf = rf->next )
	_ConicSetFindBounds(rf->conics,bounds);

    _ConicSetFindBounds(cc->conics,bounds);
}

void ConicFontFindBounds(ConicFont *sf,DBounds *bounds) {
    RefChar *rf;
    int i;

    bounds->minx = bounds->maxx = 0;
    bounds->miny = bounds->maxy = 0;

    for ( i = 0; i<sf->glyph_cnt; ++i ) {
	ConicChar *cc = sf->chars[i];
	if ( cc!=NULL ) {
	    for ( rf=cc->refs; rf!=NULL; rf = rf->next )
		_ConicSetFindBounds(rf->conics,bounds);

	    _ConicSetFindBounds(cc->conics,bounds);
	}
    }
}

void ConicPointCatagorize(ConicPoint *sp) {

    sp->pointtype = pt_corner;
    if ( sp->next==NULL && sp->prev==NULL )
	;
/* Empty segments */
    else if ( (sp->next!=NULL && sp->next->to->me.x==sp->me.x && sp->next->to->me.y==sp->me.y) ||
	    (sp->prev!=NULL && sp->prev->from->me.x==sp->me.x && sp->prev->from->me.y==sp->me.y ))
	;
    else if ( sp->next==NULL ) {
	sp->pointtype = sp->prevcp==NULL ? pt_corner : pt_curve;
    } else if ( sp->prev==NULL ) {
	sp->pointtype = sp->nextcp==NULL ? pt_corner : pt_curve;
    } else if ( sp->nextcp==NULL && sp->prevcp==NULL ) {
	;
    } else if ( sp->nextcp!=NULL && sp->prevcp!=NULL ) {
	if ( sp->nextcp->y==sp->prevcp->y && sp->nextcp->y==sp->me.y )
	    sp->pointtype = pt_curve;
	else if ( sp->nextcp->x!=sp->prevcp->x ) {
	    real slope = (sp->nextcp->y-sp->prevcp->y)/(sp->nextcp->x-sp->prevcp->x);
	    real y = slope*(sp->me.x-sp->prevcp->x) + sp->prevcp->y - sp->me.y;
	    if ( y<1 && y>-1 )
		sp->pointtype = pt_curve;
	    else if ( sp->nextcp->y!=sp->prevcp->y ) {
		real x;
		slope = (sp->nextcp->x-sp->prevcp->x)/(sp->nextcp->y-sp->prevcp->y);
		x = slope*(sp->me.y-sp->prevcp->y) + sp->prevcp->x - sp->me.x;
		if ( x<1 && x>-1 )
		    sp->pointtype = pt_curve;
	    } else if ( sp->me.y == sp->nextcp->y )
		sp->pointtype = pt_curve;
	} else if ( sp->me.x == sp->nextcp->x )
	    sp->pointtype = pt_curve;
    } else if ( sp->nextcp==NULL ) {
	if ( sp->next->to->me.x!=sp->prevcp->x ) {
	    real slope = (sp->next->to->me.y-sp->prevcp->y)/(sp->next->to->me.x-sp->prevcp->x);
	    real y = slope*(sp->me.x-sp->prevcp->x) + sp->prevcp->y - sp->me.y;
	    if ( y<1 && y>-1 )
		sp->pointtype = pt_tangent;
	} else if ( sp->me.x == sp->prevcp->x )
	    sp->pointtype = pt_tangent;
    } else {
	if ( sp->nextcp->x!=sp->prev->from->me.x ) {
	    real slope = (sp->nextcp->y-sp->prev->from->me.y)/(sp->nextcp->x-sp->prev->from->me.x);
	    real y = slope*(sp->me.x-sp->prev->from->me.x) + sp->prev->from->me.y - sp->me.y;
	    if ( y<1 && y>-1 )
		sp->pointtype = pt_tangent;
	} else if ( sp->me.x == sp->nextcp->x )
	    sp->pointtype = pt_tangent;
    }
}

int ConicPointIsACorner(ConicPoint *sp) {
    enum pointtype old = sp->pointtype, new;

    ConicPointCatagorize(sp);
    new = sp->pointtype;
    sp->pointtype = old;
return( new==pt_corner );
}

void CCCatagorizePoints(ConicChar *cc) {
    Conic *conic, *first, *last=NULL;
    ConicPointList *spl;

    for ( spl = cc->conics; spl!=NULL; spl = spl->next ) {
	first = NULL;
	for ( conic = spl->first->next; conic!=NULL && conic!=first; conic=conic->to->next ) {
	    ConicPointCatagorize(conic->from);
	    last = conic;
	    if ( first==NULL ) first = conic;
	}
	if ( conic==NULL && last!=NULL )
	    ConicPointCatagorize(last->to);
    }
}
