/* Copyright (C) 2000-2009 by George Williams */
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
#include "splinefont.h"
#include "edgelist2.h"
#include <math.h>
#ifdef HAVE_IEEEFP_H
# include <ieeefp.h>		/* Solaris defines isnan in ieeefp rather than math.h */
#endif
#include <stdarg.h>

#include <gwidget.h>		/* For PostNotice */

/* First thing we do is divide each spline into a set of sub-splines each of */
/*  which is monotonic in both x and y (always increasing or decreasing)     */
/* Then we compare each monotonic spline with every other one and see if they*/
/*  intersect.  If they do, split each up into sub-sub-segments and create an*/
/*  intersection point (note we need to be a little careful if an intersec-  */
/*  tion happens at an end point. We don't need to create a intersection for */
/*  two adjacent splines, there isn't a real intersection... but if a third  */
/*  spline crosses that point (or ends there) then all three (four) splines  */
/*  need to be joined into an intersection point)                            */
/* Nasty things happen if splines are coincident. They will almost never be  */
/*  perfectly coincident and will keep crossing and recrossing as rounding   */
/*  errors suggest one is before the other. Look for coincident splines and  */
/*  treat the places they start and stop being coincident as intersections   */
/*  then when we find needed splines below look for these guys and ignore    */
/*  recrossings of splines which are close together                          */
/* Figure out if each monotonic sub-spline is needed or not		     */
/*  (Note: It was tempting to split the bits up into real splines rather     */
/*   than keeping them as sub-sections of the original. Unfortunately this   */
/*   splitting introduced rounding errors which meant that we got more       */
/*   intersections, which meant that splines could be both needed and un.    */
/*   so I don't do that until later)					     */
/*  if the spline hasn't been tagged yet:				     */
/*   does the spline change greater in x or y?				     */
/*   draw a line parallel to the OTHER axis which hits our spline and doesn't*/
/*    hit any endpoints (or intersections, which are end points too now)     */
/*   count the winding number (as we do this we can mark other splines as    */
/*    needed or not) and figure out if our spline is needed		     */
/*  So run through the list of intersections				     */
/*	At an intersection there should be an even number of needed monos.   */
/*	Use this as the basis of a new splineset, trace it around until	     */
/*	  we get back to the start intersection (should happen)		     */
/*	  (Note: We may need to reverse a monotonic sub-spline or two)	     */
/*	  As we go, mark each monotonic as having been used		     */
/*  Keep doing this until all needed exits from all intersections have been  */
/*	used.								     */
/*  The free up our temporary data structures, merge in any open splinesets  */
/*	free the old closed splinesets					     */

typedef struct mlist {
    Spline *s;
    Monotonic *m;			/* May get slightly munched but will */
			/* always have right spline. we fix when we need it */
    extended t;
    int isend;
    BasePoint unit;
    struct mlist *next;
} MList;

typedef struct intersection {
    MList *monos;
    BasePoint inter;
    struct intersection *next;
} Intersection;

static char *glyphname=NULL;

static void SOError(char *format,...) {
    va_list ap;
    va_start(ap,format);
    if ( glyphname==NULL )
	fprintf(stderr, "Internal Error: " );
    else
	fprintf(stderr, "Internal Error in %s: ", glyphname );
    vfprintf(stderr,format,ap);
}

static Monotonic *SplineToMonotonic(Spline *s,extended startt,extended endt,
	Monotonic *last,int exclude) {
    Monotonic *m;
    BasePoint start, end;

    start.x = ((s->splines[0].a*startt+s->splines[0].b)*startt+s->splines[0].c)*startt
		+ s->splines[0].d;
    start.y = ((s->splines[1].a*startt+s->splines[1].b)*startt+s->splines[1].c)*startt
		+ s->splines[1].d;
    end.x = ((s->splines[0].a*endt+s->splines[0].b)*endt+s->splines[0].c)*endt
		+ s->splines[0].d;
    end.y = ((s->splines[1].a*endt+s->splines[1].b)*endt+s->splines[1].c)*endt
		+ s->splines[1].d;
    if ( (real) (((start.x+end.x)/2)==start.x || (real) ((start.x+end.x)/2)==end.x) &&
	    (real) (((start.y+end.y)/2)==start.y || (real) ((start.y+end.y)/2)==end.y) ) {
	/* The distance between the two extrema is so small */
	/*  as to be unobservable. In other words we'd end up with a zero*/
	/*  length spline */
	if ( endt==1.0 && last!=NULL && last->s==s )
	    last->tend = endt;
return( last );
    }

    m = chunkalloc(sizeof(Monotonic));
    m->s = s;
    m->tstart = startt;
    m->tend = endt;
    m->exclude = exclude;

    if ( end.x>start.x ) {
	m->xup = true;
	m->b.minx = start.x;
	m->b.maxx = end.x;
    } else {
	m->b.minx = end.x;
	m->b.maxx = start.x;
    }
    if ( end.y>start.y ) {
	m->yup = true;
	m->b.miny = start.y;
	m->b.maxy = end.y;
    } else {
	m->b.miny = end.y;
	m->b.maxy = start.y;
    }

    if ( last!=NULL ) {
	last->next = m;
	last->linked = m;
	m->prev = last;
    }
return( m );
}

static int SSIsSelected(SplineSet *spl) {
    SplinePoint *sp;

    for ( sp=spl->first; ; ) {
	if ( sp->selected )
return( true );
	if ( sp->next==NULL )
return( false );
	sp = sp->next->to;
	if ( sp==spl->first )
return( false );
    }
}

static int BpSame(BasePoint *bp1, BasePoint *bp2) {
    BasePoint mid;

    mid.x = (bp1->x+bp2->x)/2; mid.y = (bp1->y+bp2->y)/2;
    if ( (bp1->x==mid.x || bp2->x==mid.x) &&
	    (bp1->y==mid.y || bp2->y==mid.y))
return( true );

return( false );
}

static int SSRmNullSplines(SplineSet *spl) {
    Spline *s, *first, *next;

    first = NULL;
    for ( s=spl->first->next ; s!=first; s=next ) {
	next = s->to->next;
	if ( ((s->splines[0].a>-.01 && s->splines[0].a<.01 &&
		s->splines[0].b>-.01 && s->splines[0].b<.01 &&
		s->splines[1].a>-.01 && s->splines[1].a<.01 &&
		s->splines[1].b>-.01 && s->splines[1].b<.01) ||
		/* That describes a null spline (a line between the same end-point) */
	       RealNear((s->from->nextcp.x-s->from->me.x)*(s->to->me.y-s->to->prevcp.y)-
			(s->from->nextcp.y-s->from->me.y)*(s->to->me.x-s->to->prevcp.x),0)) &&
		/* And the above describes a point with a spline between it */
		/*  and itself where the spline covers no area (the two cps */
		/*  point in the same direction) */
		BpSame(&s->from->me,&s->to->me)) {
	    if ( next==s )
return( true );
	    if ( next->from->selected ) s->from->selected = true;
	    s->from->next = next;
	    s->from->nextcp = next->from->nextcp;
	    s->from->nonextcp = next->from->nonextcp;
	    s->from->nextcpdef = next->from->nextcpdef;
	    SplinePointFree(next->from);
	    if ( spl->first==next->from )
		spl->last = spl->first = s->from;
	    next->from = s->from;
	    SplineFree(s);
	} else {
	    if ( first==NULL )
		first = s;
	}
    }
return( false );
}

static Monotonic *SSToMContour(SplineSet *spl, Monotonic *start,
	Monotonic **end, enum overlap_type ot) {
    extended ts[4];
    Spline *first, *s;
    Monotonic *head=NULL, *last=NULL;
    int cnt, i, selected = false;
    extended lastt;

    if ( spl->first->prev==NULL )
return( start );		/* Open contours have no interior, ignore 'em */
    if ( spl->first->prev->from==spl->first &&
	    spl->first->noprevcp && spl->first->nonextcp )
return( start );		/* Let's just remove single points */

    if ( ot==over_rmselected || ot==over_intersel || ot==over_fisel || ot==over_exclude ) {
	selected = SSIsSelected(spl);
	if ( ot==over_rmselected || ot==over_intersel || ot==over_fisel ) {
	    if ( !selected )
return( start );
	    selected = false;
	}
    }

    /* We blow up on zero length splines. And a zero length contour is nasty */
    if ( SSRmNullSplines(spl))
return( start );

    first = NULL;
    for ( s=spl->first->next; s!=first; s=s->to->next ) {
	if ( first==NULL ) first = s;
	cnt = Spline2DFindExtrema(s,ts);
	lastt = 0;
	for ( i=0; i<cnt; ++i ) {
	    last = SplineToMonotonic(s,lastt,ts[i],last,selected);
	    if ( head==NULL ) head = last;
	    lastt=ts[i];
	}
	if ( lastt!=1.0 ) {
	    last = SplineToMonotonic(s,lastt,1.0,last,selected);
	    if ( head==NULL ) head = last;
	}
    }
    head->prev = last;
    last->next = head;
    if ( start==NULL )
	start = head;
    else
	(*end)->linked = head;
    *end = last;
return( start );
}

Monotonic *SSsToMContours(SplineSet *spl, enum overlap_type ot) {
    Monotonic *head=NULL, *last = NULL;

    while ( spl!=NULL ) {
	if ( spl->first->prev!=NULL )
	    head = SSToMContour(spl,head,&last,ot);
	spl = spl->next;
    }
return( head );
}

static void _AddSpline(Intersection *il,Monotonic *m,extended t,int isend) {
    MList *ml;

    for ( ml=il->monos; ml!=NULL; ml=ml->next ) {
	if ( ml->s==m->s && RealNear( ml->t,t ) && ml->isend==isend )
return;
    }

    ml = chunkalloc(sizeof(MList));
    ml->next = il->monos;
    il->monos = ml;
    ml->s = m->s;
    ml->m = m;			/* This may change. We'll fix it up later */
    ml->t = t;
    ml->isend = isend;
    if ( isend ) {
	if ( m->end!=NULL && m->end!=il )
	    SOError("Resetting end.\n");
	m->end = il;
    } else {
	if ( m->start!=NULL && m->start!=il )
	    SOError("Resetting start.\n");
	m->start = il;
    }
return;
}

static void AddSpline(Intersection *il,Monotonic *m,extended t) {
    MList *ml;

    if ( m->start==il || m->end==il )
return;

    for ( ml=il->monos; ml!=NULL; ml=ml->next ) {
	if ( ml->s==m->s && RealWithin( ml->t,t,.0001 ))
return;
    }

    ml = chunkalloc(sizeof(MList));
    ml->next = il->monos;
    il->monos = ml;
    ml->s = m->s;
    ml->m = m;			/* This may change. We'll fix it up later */
    ml->t = t;
    ml->isend = true;
    if ( t-m->tstart < m->tend-t && RealNear(m->tstart,t) ) {
	if ( m->start!=NULL && m->start!=il )
	    SOError("Resetting start.\n");
	m->start = il;
	ml->t = m->tstart;
	ml->isend = false;
	_AddSpline(il,m->prev,m->prev->tend,true);
    } else if ( RealNear(m->tend,t)) {
	if ( m->end!=NULL && m->end!=il )
	    SOError("Resetting end.\n");
	m->end = il;
	ml->t = m->tend;
	_AddSpline(il,m->next,m->next->tstart,false);
    } else {
	/* Ok, if we've got a new intersection on this spline then break up */
	/*  the monotonic into two bits which end and start at this inter */
	if ( t<m->tstart || t>m->tend )
	    SOError( "Attempt to subset monotonic rejoin inappropriately: %g should be [%g,%g]\n",
		    t, m->tstart, m->tend );
	else {
	    /* It is monotonic, so a subset of it must also be */
	    Monotonic *m2 = chunkalloc(sizeof(Monotonic));
	    BasePoint pt;
	    *m2 = *m;
	    m->next = m2;
	    m2->prev = m;
	    m2->next->prev = m2;
	    m->linked = m2;
	    m->tend = t;
	    m->end = il;
	    m2->start = il;
	    m2->tstart = t;
	    pt.x = ((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+
		    m->s->splines[0].c)*m->tstart+m->s->splines[0].d;
	    pt.y = ((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+
		    m->s->splines[1].c)*m->tstart+m->s->splines[1].d;
	    if ( pt.x>il->inter.x ) {
		m->b.minx = il->inter.x;
		m->b.maxx = pt.x;
	    } else {
		m->b.minx = pt.x;
		m->b.maxx = il->inter.x;
	    }
	    if ( pt.y>il->inter.y ) {
		m->b.miny = il->inter.y;
		m->b.maxy = pt.y;
	    } else {
		m->b.miny = pt.y;
		m->b.maxy = il->inter.y;
	    }
	    pt.x = ((m2->s->splines[0].a*m2->tend+m2->s->splines[0].b)*m2->tend+
		    m2->s->splines[0].c)*m2->tend+m2->s->splines[0].d;
	    pt.y = ((m2->s->splines[1].a*m2->tend+m2->s->splines[1].b)*m2->tend+
		    m2->s->splines[1].c)*m2->tend+m2->s->splines[1].d;
	    if ( pt.x>il->inter.x ) {
		m2->b.minx = il->inter.x;
		m2->b.maxx = pt.x;
	    } else {
		m2->b.minx = pt.x;
		m2->b.maxx = il->inter.x;
	    }
	    if ( pt.y>il->inter.y ) {
		m2->b.miny = il->inter.y;
		m2->b.maxy = pt.y;
	    } else {
		m2->b.miny = pt.y;
		m2->b.maxy = il->inter.y;
	    }
	    _AddSpline(il,m2,t,false);
	}
    }
}

static void SetStartPoint(BasePoint *pt,Monotonic *m) {
    if ( m->tstart==0 )
	*pt = m->s->from->me;
    else if ( m->start!=NULL )
	*pt = m->start->inter;
    else {
	pt->x = ((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart +
		m->s->splines[0].c)*m->tstart + m->s->splines[0].d;
	pt->y = ((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart +
		m->s->splines[1].c)*m->tstart + m->s->splines[1].d;
    }
}

static void SetEndPoint(BasePoint *pt,Monotonic *m) {
    if ( m->tend==1.0 )
	*pt = m->s->to->me;
    else if ( m->end!=NULL )
	*pt = m->end->inter;
    else {
	pt->x = ((m->s->splines[0].a*m->tend+m->s->splines[0].b)*m->tend +
		m->s->splines[0].c)*m->tend + m->s->splines[0].d;
	pt->y = ((m->s->splines[1].a*m->tend+m->s->splines[1].b)*m->tend +
		m->s->splines[1].c)*m->tend + m->s->splines[1].d;
    }
}

static extended RoundToEndpoints(Monotonic *m,extended t,BasePoint *inter) {
    BasePoint end;
    extended bound;

    if ( t==0 || t==1 ) {
	if ( t==0 )
	    *inter = m->s->from->me;
	else
	    *inter = m->s->to->me;
return( t );
    }

    if ( t-m->tstart < m->tend-t ) {
	bound = m->tstart;
	SetStartPoint(&end,m);
    } else {
	bound = m->tend;
	SetEndPoint(&end,m);
    }
    if ( BpSame(&end,inter) || RealWithin(t,bound,.00001)) {
	*inter = end;
return( bound );
    }

return( t );
}

static extended Grad1(Spline1D *s1, Spline1D *s2,
	extended t1,extended t2 ) {
    /* d/dt[12] (m1(t1).x-m2(t2).x)^2 + (m1(t1).y-m2(t2).y)^2 */
    /* d/dt[12] (m1(t1).x^2 -2m1(t1).x*m2(t2).x + m2(t2).x^2) + (m1(t1).y^2 -2m1(t1).y*m2(t2).y + m2(t2).y^2) */
    extended val2 = ((s2->a*t2+s2->b)*t2+s2->c)*t2+s2->d;

return( ((((6*(extended)s1->a*s1->a*t1 +
	    5*2*(extended)s1->a*s1->b)*t1 +
	    4*(s1->b*(extended)s1->b+2*s1->a*(extended)s1->c))*t1 +
	    3*2*(s1->a*(extended)s1->d+s1->b*(extended)s1->c))*t1 +
	    2*(s1->c*(extended)s1->c+2*s1->b*(extended)s1->d))*t1 +
	    2*s1->c*(extended)s1->d  -
	     2*val2 * ((3*s1->a*t1 + 2*s1->b)*t1 + s1->c) );
}

static void GradImproveInter(Monotonic *m1, Monotonic *m2,
	extended *_t1,extended *_t2,BasePoint *inter) {
    Spline *s1 = m1->s, *s2 = m2->s;
    extended x1, x2, y1, y2;
    extended gt1=0, gt2=0, glen=1;
    extended error, olderr=1e10;
    extended factor = 4096;
    extended t1=*_t1, t2=*_t2;
    extended off, off2, yoff;
    int cnt=0;
    /* We want to find (t1,t2) so that (m1(t1)-m2(t2))^2==0 */
    /* Find the gradiant and move in the reverse direction */
    /* We know that the current values of (t1,t2) are close to an intersection*/
    /* so the grad should point correctly */
    /* d/dt[12] (m1(t1).x-m2(t2).x)^2 + (m1(t1).y-m2(t2).y)^2 */
    /* d/dt[12] (m1(t1).x^2 -2m1(t1).x*m2(t2).x + m2(t2).x^2) + (m1(t1).y^2 -2m1(t1).y*m2(t2).y + m2(t2).y^2) */

    forever {
	x1 = ((s1->splines[0].a*t1 + s1->splines[0].b)*t1 + s1->splines[0].c)*t1 + s1->splines[0].d;
	x2 = ((s2->splines[0].a*t2 + s2->splines[0].b)*t2 + s2->splines[0].c)*t2 + s2->splines[0].d;
	y1 = ((s1->splines[1].a*t1 + s1->splines[1].b)*t1 + s1->splines[1].c)*t1 + s1->splines[1].d;
	y2 = ((s2->splines[1].a*t2 + s2->splines[1].b)*t2 + s2->splines[1].c)*t2 + s2->splines[1].d;
	error = (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2);
	if ( error>olderr ) {
	    if ( olderr==1e10 )
    break;
	    factor *= 2;
	    if ( factor>4096*4096 )
    break;
	    glen *= 2;
	    t1 += gt1/glen;
	    t2 += gt2/glen;
    continue;
	} else
	    factor /= 1.4;
	if ( error<1e-11 )	/* Error is actually the square of the error */
    break;			/* So this isn't as constraining as it looks */

	gt1 = Grad1(&s1->splines[0],&s2->splines[0],t1,t2) + Grad1(&s1->splines[1],&s2->splines[1],t1,t2);
	gt2 = Grad1(&s2->splines[0],&s1->splines[0],t2,t1) + Grad1(&s2->splines[1],&s1->splines[1],t2,t1);
	glen = esqrt(gt1*gt1 + gt2*gt2) * factor;
	if ( glen==0 )
    break;
	*_t1 = t1; *_t2 = t2;
	t1 -= gt1/glen;
	t2 -= gt2/glen;
	if ( isnan(t1) || isnan(t2)) {
	    IError( "Nan in grad" );
    break;
	}
	olderr = error;
	++cnt;
	if ( cnt>1000 )
    break;
    }
#if 0
    if ( cnt<=1 && error>=1e-11 )
	fprintf(stderr,"No Improvement\n" );
    else if ( cnt>1 )
	fprintf(stderr,"Improvement\n" );
#endif
    t1 = *_t1; t2 = *_t2;
    if ( t1<0 && t1>-.00001 ) *_t1 = t1 = 0;
    else if ( t1>1 && t1<1.00001 ) *_t1 = t1 = 1.0;
    if ( t2<0 && t2>-.00001 ) *_t2 = t2 = 0;
    else if ( t2>1 && t2<1.00001 ) *_t2 = t2 = 1.0;
    x1 = ((s1->splines[0].a*t1 + s1->splines[0].b)*t1 + s1->splines[0].c)*t1 + s1->splines[0].d;
    x2 = ((s2->splines[0].a*t2 + s2->splines[0].b)*t2 + s2->splines[0].c)*t2 + s2->splines[0].d;
    y1 = ((s1->splines[1].a*t1 + s1->splines[1].b)*t1 + s1->splines[1].c)*t1 + s1->splines[1].d;
    y2 = ((s2->splines[1].a*t2 + s2->splines[1].b)*t2 + s2->splines[1].c)*t2 + s2->splines[1].d;
    inter->x = (x1+x2)/2; inter->y = (y1+y2)/2;

    if ( (off=x1-x2)<0 ) off = -off;
    if ( (yoff=y1-y2)<0 ) yoff = -yoff;
    off += yoff;

    if ( t1<.0001 ) {
	t1 = 0;
	x1 = s1->splines[0].d;
	y1 = s1->splines[1].d;
    } else if ( t1>.9999 ) {
	t1 = 1.0;
	x1 = s1->splines[0].a+s1->splines[0].b+s1->splines[0].c+s1->splines[0].d;
	y1 = s1->splines[1].a+s1->splines[1].b+s1->splines[1].c+s1->splines[1].d;
    }
    if ( t2<.0001 ) {
	t2=0;
	x2 = s2->splines[0].d;
	y2 = s2->splines[1].d;
    } else if ( t2>.9999 ) {
	t2=1.0;
	x2 = s2->splines[0].a+s2->splines[0].b+s2->splines[0].c+s2->splines[0].d;
	y2 = s2->splines[1].a+s2->splines[1].b+s2->splines[1].c+s2->splines[1].d;
    }
    if ( (off2=x1-x2)<0 ) off2 = -off2;
    if ( (yoff=y1-y2)<0 ) yoff = -yoff;
    off2 += yoff;
    if ( off2<=off ) {
	*_t1 = t1; *_t2 = t2;
	inter->x = (x1+x2)/2; inter->y = (y1+y2)/2;
    }
}

static Intersection *AddIntersection(Intersection *ilist,Monotonic *m1,
	Monotonic *m2,extended t1,extended t2,BasePoint *inter) {
    Intersection *il;
    extended ot1 = t1, ot2 = t2;

    /* Fixup some rounding errors */
    GradImproveInter(m1,m2,&t1,&t2,inter);
    if ( t1<m1->tstart || t1>m1->tend || t2<m2->tstart || t2>m2->tend )
return( ilist );

#if 0
    if (( m1->start==m2->start && m1->start!=NULL && RealNear(t1,m1->tstart) && RealNear(t2,m2->tstart)) ||
	    ( m1->start==m2->end && m1->start!=NULL && RealNear(t1,m1->tstart) && RealNear(t2,m2->tend)) ||
	    ( m1->end==m2->start && m1->end!=NULL && RealNear(t1,m1->tend) && RealNear(t2,m2->tstart)) ||
	    ( m1->end==m2->end && m1->end!=NULL && RealNear(t1,m1->tend) && RealNear(t2,m2->tend)) )
return( ilist );
    else
    if ( RealWithin(t1,1.0,.01) && RealWithin(t2,0.0,.01) && BpSame(&m1->s->to->me,&m2->s->from->me)) {
	t1 = 1.0;
	t2 = 0.0;
    } else if ( RealWithin(t2,1.0,.01) && RealWithin(t1,0.0,.01) && BpSame(&m2->s->to->me,&m1->s->from->me)) {
	t1 = 0.0;
	t2 = 1.0;
    } else {
#endif
	t1 = RoundToEndpoints(m1,t1,inter);
	t2 = RoundToEndpoints(m2,t2,inter);
	t1 = RoundToEndpoints(m1,t1,inter);	/* Do it twice. rounding t2 can mean we now need to round t1 */
#if 0
    }
#endif

    if (( m1->s->to == m2->s->from && RealWithin(t1,1.0,.01) && RealWithin(t2,0,.01)) ||
	    ( m1->s->from == m2->s->to && RealWithin(t1,0,.01) && RealWithin(t2,1.0,.01)))
return( ilist );

    if (( t1==m1->tstart && m1->start!=NULL &&
	    (inter->x!=m1->start->inter.x || inter->y!=m1->start->inter.y)) ||
	( t1==m1->tend && m1->end!=NULL &&
	    (inter->x!=m1->end->inter.x || inter->y!=m1->end->inter.y)))
	t1 = ot1;
    if (( t2==m2->tstart && m2->start!=NULL &&
	    (inter->x!=m2->start->inter.x || inter->y!=m2->start->inter.y)) ||
	( t2==m2->tend && m2->end!=NULL &&
	    (inter->x!=m2->end->inter.x || inter->y!=m2->end->inter.y)))
	t2 = ot2;

    /* The ordinary join of one spline to the next doesn't really count */
    /*  Or one monotonic sub-spline to the next either */
    if (( m1->next==m2 && RealNear(t1,m1->tend) && RealNear(t2,m2->tstart)) ||
	    (m2->next==m1 && RealNear(t1,m1->tstart) && RealNear(t2,m2->tend)) )
return( ilist );

    if ( RealWithin(m1->tstart,t1,.01) )
	il = m1->start;
    else if ( RealWithin(m1->tend,t1,.01) )
	il = m1->end;
    else
	il = NULL;
    if ( il!=NULL &&
	    ((RealWithin(m2->tstart,t2,.01) && m2->start==il) ||
	     (RealWithin(m2->tend,t2,.01) && m2->end==il)) )
return( ilist );

    for ( il = ilist; il!=NULL; il=il->next ) {
	if ( RealWithin(il->inter.x,inter->x,.01) && RealWithin(il->inter.y,inter->y,.01)) {
	    AddSpline(il,m1,t1);
	    AddSpline(il,m2,t2);
return( ilist );
	}
    }

    il = chunkalloc(sizeof(Intersection));
    il->inter = *inter;
    il->next = ilist;
    AddSpline(il,m1,t1);
    AddSpline(il,m2,t2);
return( il );
}

static extended BoundIterateSplineSolve(Spline1D *sp, extended tmin, extended tmax,
	extended sought,double err) {
    extended t = IterateSplineSolve(sp,tmin,tmax,sought,err);
    if ( t<tmin || t>tmax )
return( -1 );

return( t );
}

static Intersection *FindMonotonicIntersection(Intersection *ilist,Monotonic *m1,Monotonic *m2) {
    /* I believe that two monotonic cubics can still intersect in two points */
    /*  so we can't just check if the splines are on oposite sides of each */
    /*  other at top and bottom */
    DBounds b;
    const double error = .0001;
    BasePoint pt;
    extended t1,t2;

    b.minx = m1->b.minx>m2->b.minx ? m1->b.minx : m2->b.minx;
    b.maxx = m1->b.maxx<m2->b.maxx ? m1->b.maxx : m2->b.maxx;
    b.miny = m1->b.miny>m2->b.miny ? m1->b.miny : m2->b.miny;
    b.maxy = m1->b.maxy<m2->b.maxy ? m1->b.maxy : m2->b.maxy;

    if ( b.maxy==b.miny && b.minx==b.maxx ) {
	extended x1,y1, x2,y2;
	if ( m1->next==m2 || m2->next==m1 )
return( ilist );		/* Not interesting. Only intersection is at an endpoint */
	if ( ((m1->start==m2->start || m1->end==m2->start) && m2->start!=NULL) ||
		((m1->start==m2->end || m1->end==m2->end ) && m2->end!=NULL ))
return( ilist );
	pt.x = b.minx; pt.y = b.miny;
	if ( m1->b.maxx-m1->b.minx > m1->b.maxy-m1->b.miny )
	    t1 = BoundIterateSplineSolve(&m1->s->splines[0],m1->tstart,m1->tend,b.minx,error);
	else
	    t1 = BoundIterateSplineSolve(&m1->s->splines[1],m1->tstart,m1->tend,b.miny,error);
	if ( m2->b.maxx-m2->b.minx > m2->b.maxy-m2->b.miny )
	    t2 = BoundIterateSplineSolve(&m2->s->splines[0],m2->tstart,m2->tend,b.minx,error);
	else
	    t2 = BoundIterateSplineSolve(&m2->s->splines[1],m2->tstart,m2->tend,b.miny,error);
	if ( t1!=-1 && t2!=-1 ) {
	    x1 = ((m1->s->splines[0].a*t1+m1->s->splines[0].b)*t1+m1->s->splines[0].c)*t1+m1->s->splines[0].d;
	    y1 = ((m1->s->splines[1].a*t1+m1->s->splines[1].b)*t1+m1->s->splines[1].c)*t1+m1->s->splines[1].d;
	    x2 = ((m2->s->splines[0].a*t2+m2->s->splines[0].b)*t2+m2->s->splines[0].c)*t2+m2->s->splines[0].d;
	    y2 = ((m2->s->splines[1].a*t2+m2->s->splines[1].b)*t2+m2->s->splines[1].c)*t2+m2->s->splines[1].d;
	    if ( x1-x2>-.01 && x1-x2<.01 && y1-y2>-.01 && y1-y2<.01 )
		ilist = AddIntersection(ilist,m1,m2,t1,t2,&pt);
	}
    } else if ( b.maxy==b.miny ) {
	extended x1,x2;
	if ( m1->next==m2 || m2->next==m1 )
return( ilist );		/* Not interesting. Only intersection is at an endpoint */
	t1 = BoundIterateSplineSolve(&m1->s->splines[1],m1->tstart,m1->tend,b.miny,error);
	t2 = BoundIterateSplineSolve(&m2->s->splines[1],m2->tstart,m2->tend,b.miny,error);
	if ( t1!=-1 && t2!=-1 ) {
	    x1 = ((m1->s->splines[0].a*t1+m1->s->splines[0].b)*t1+m1->s->splines[0].c)*t1+m1->s->splines[0].d;
	    x2 = ((m2->s->splines[0].a*t2+m2->s->splines[0].b)*t2+m2->s->splines[0].c)*t2+m2->s->splines[0].d;
	    if ( x1-x2>-.01 && x1-x2<.01 ) {
		pt.x = (x1+x2)/2; pt.y = b.miny;
		ilist = AddIntersection(ilist,m1,m2,t1,t2,&pt);
	    }
	}
    } else if ( b.maxx==b.minx ) {
	extended y1,y2;
	if ( m1->next==m2 || m2->next==m1 )
return( ilist );		/* Not interesting. Only intersection is at an endpoint */
	t1 = BoundIterateSplineSolve(&m1->s->splines[0],m1->tstart,m1->tend,b.minx,error);
	t2 = BoundIterateSplineSolve(&m2->s->splines[0],m2->tstart,m2->tend,b.minx,error);
	if ( t1!=-1 && t2!=-1 ) {
	    y1 = ((m1->s->splines[1].a*t1+m1->s->splines[1].b)*t1+m1->s->splines[1].c)*t1+m1->s->splines[1].d;
	    y2 = ((m2->s->splines[1].a*t2+m2->s->splines[1].b)*t2+m2->s->splines[1].c)*t2+m2->s->splines[1].d;
	    if ( y1-y2>-.01 && y1-y2<.01 ) {
		pt.x = b.minx; pt.y = (y1+y2)/2;
		ilist = AddIntersection(ilist,m1,m2,t1,t2,&pt);
	    }
	}
    } else if ( b.maxy-b.miny > b.maxx-b.minx ) {
	extended diff, y, x1,x2, x1o,x2o;
	extended t1,t2, t1o,t2o ;

	diff = (b.maxy-b.miny)/32;
	y = b.miny;
	x1o = x2o = 0;
	while ( y<b.maxy ) {
	    while ( y<b.maxy ) {
		t1o = BoundIterateSplineSolve(&m1->s->splines[1],m1->tstart,m1->tend,b.miny,error);
		t2o = BoundIterateSplineSolve(&m2->s->splines[1],m2->tstart,m2->tend,b.miny,error);
		if ( t1o!=-1 && t2o!=-1 )
	    break;
		y += diff;
	    }
	    x1o = ((m1->s->splines[0].a*t1o+m1->s->splines[0].b)*t1o+m1->s->splines[0].c)*t1o+m1->s->splines[0].d;
	    x2o = ((m2->s->splines[0].a*t2o+m2->s->splines[0].b)*t2o+m2->s->splines[0].c)*t2o+m2->s->splines[0].d;
	    if ( x1o!=x2o )
	break;
	    y += diff;
	}
#if 0
	if ( x1o==x2o ) {
	    pt.x = x1o; pt.y = y;
	    ilist = AddIntersection(ilist,m1,m2,t1o,t2o,&pt);
	}
#endif
	for ( y+=diff; ; y += diff ) {
	    /* I used to say y<=b.maxy in the above for statement. */
	    /*  that seemed to get rounding errors on the mac, so we do it */
	    /*  like this now: */
	    if ( y>b.maxy ) {
		if ( y<b.maxy+diff/4 ) y = b.maxy;
		else
	break;
	    }
	    t1 = BoundIterateSplineSolve(&m1->s->splines[1],m1->tstart,m1->tend,y,error);
	    t2 = BoundIterateSplineSolve(&m2->s->splines[1],m2->tstart,m2->tend,y,error);
	    if ( t1==-1 || t2==-1 )
	continue;
	    x1 = ((m1->s->splines[0].a*t1+m1->s->splines[0].b)*t1+m1->s->splines[0].c)*t1+m1->s->splines[0].d;
	    x2 = ((m2->s->splines[0].a*t2+m2->s->splines[0].b)*t2+m2->s->splines[0].c)*t2+m2->s->splines[0].d;
#if 0
	    if ( x1==x2 && x1o!=x2o ) {
		pt.x = x1; pt.y = y;
		ilist = AddIntersection(ilist,m1,m2,t1,t2,&pt);
		if ( m1->tend==t1 && m1->s==m1->next->s ) m1 = m1->next;
		if ( m2->tend==t2 && m2->s==m2->next->s ) m2 = m2->next;
	    } else
#endif
	    if ( x1o!=x2o && (x1o>x2o) != ( x1>x2 ) ) {
		/* A cross over has occured. (assume we have a small enough */
		/*  region that three cross-overs can't have occurred) */
		/* Use a binary search to track it down */
		extended ytop, ybot;
		ytop = y;
		ybot = y-diff;
		while ( ytop!=ybot ) {
		    extended ytest = (ytop+ybot)/2;
		    extended t1t, t2t;
		    t1t = BoundIterateSplineSolve(&m1->s->splines[1],m1->tstart,m1->tend,ytest,error);
		    t2t = BoundIterateSplineSolve(&m2->s->splines[1],m2->tstart,m2->tend,ytest,error);
		    x1 = ((m1->s->splines[0].a*t1t+m1->s->splines[0].b)*t1t+m1->s->splines[0].c)*t1t+m1->s->splines[0].d;
		    x2 = ((m2->s->splines[0].a*t2t+m2->s->splines[0].b)*t2t+m2->s->splines[0].c)*t2t+m2->s->splines[0].d;
		    if ( t1t==-1 || t2t==-1 ) {
			SOError( "Can't find something in range.\n" );
		break;
		    } else if (( x1-x2<error && x1-x2>-error ) || ytop==ytest || ybot==ytest ) {
			pt.y = ytest; pt.x = (x1+x2)/2;
			ilist = AddIntersection(ilist,m1,m2,t1t,t2t,&pt);
			b.maxy = m1->b.maxy<m2->b.maxy ? m1->b.maxy : m2->b.maxy;
		break;
		    } else if ( (x1o>x2o) != ( x1>x2 ) ) {
			ytop = ytest;
		    } else {
			ybot = ytest;
		    }
		}
		x1 = x1o; x1o = x2o; x2o = x1;
	    } else {
		x1o = x1; x2o = x2;
	    }
	}
    } else {
	extended diff, x, y1,y2, y1o,y2o;
	extended t1,t2, t1o,t2o ;

	diff = (b.maxx-b.minx)/32;
	x = b.minx;
	y1o = y2o = 0;
	while ( x<b.maxx ) {
	    while ( x<b.maxx ) {
		t1o = BoundIterateSplineSolve(&m1->s->splines[0],m1->tstart,m1->tend,b.minx,error);
		t2o = BoundIterateSplineSolve(&m2->s->splines[0],m2->tstart,m2->tend,b.minx,error);
		if ( t1o!=-1 && t2o!=-1 )
	    break;
		x += diff;
	    }
	    y1o = ((m1->s->splines[1].a*t1o+m1->s->splines[1].b)*t1o+m1->s->splines[1].c)*t1o+m1->s->splines[1].d;
	    y2o = ((m2->s->splines[1].a*t2o+m2->s->splines[1].b)*t2o+m2->s->splines[1].c)*t2o+m2->s->splines[1].d;
	    if ( y1o!=y2o )
	break;
	    x += diff;
	}
#if 0
	if ( y1o==y2o ) {
	    pt.y = y1o; pt.x = x;
	    ilist = AddIntersection(ilist,m1,m2,t1o,t2o,&pt);
	}
#endif
	y1 = y2 = 0;
	for ( x+=diff; ; x += diff ) {
	    if ( x>b.maxx ) {
		if ( x<b.maxx+diff/4 ) x = b.maxx;
		else
	break;
	    }
	    t1 = BoundIterateSplineSolve(&m1->s->splines[0],m1->tstart,m1->tend,x,error);
	    t2 = BoundIterateSplineSolve(&m2->s->splines[0],m2->tstart,m2->tend,x,error);
	    if ( t1==-1 || t2==-1 )
	continue;
	    y1 = ((m1->s->splines[1].a*t1+m1->s->splines[1].b)*t1+m1->s->splines[1].c)*t1+m1->s->splines[1].d;
	    y2 = ((m2->s->splines[1].a*t2+m2->s->splines[1].b)*t2+m2->s->splines[1].c)*t2+m2->s->splines[1].d;
#if 0
	    if ( y1==y2 && y1o!=y2o ) {
		pt.y = y1; pt.x = x;
		ilist = AddIntersection(ilist,m1,m2,t1,t2,&pt);
		if ( m1->tend==t1 && m1->s==m1->next->s ) m1 = m1->next;
		if ( m2->tend==t2 && m2->s==m2->next->s ) m2 = m2->next;
	    } else
#endif
	    if ( (y1o>y2o) != ( y1>y2 ) ) {
		/* A cross over has occured. (assume we have a small enough */
		/*  region that three cross-overs can't have occurred) */
		/* Use a binary search to track it down */
		extended xtop, xbot;
		xtop = x;
		xbot = x-diff;
		while ( xtop!=xbot ) {
		    extended xtest = (xtop+xbot)/2;
		    extended t1t, t2t;
		    t1t = BoundIterateSplineSolve(&m1->s->splines[0],m1->tstart,m1->tend,xtest,error);
		    t2t = BoundIterateSplineSolve(&m2->s->splines[0],m2->tstart,m2->tend,xtest,error);
		    y1 = ((m1->s->splines[1].a*t1t+m1->s->splines[1].b)*t1t+m1->s->splines[1].c)*t1t+m1->s->splines[1].d;
		    y2 = ((m2->s->splines[1].a*t2t+m2->s->splines[1].b)*t2t+m2->s->splines[1].c)*t2t+m2->s->splines[1].d;
		    if ( t1t==-1 || t2t==-1 ) {
			SOError( "Can't find something in range.\n" );
		break;
		    } else if (( y1-y2<error && y1-y2>-error ) || xtop==xtest || xbot==xtest ) {
			pt.x = xtest; pt.y = (y1+y2)/2;
			ilist = AddIntersection(ilist,m1,m2,t1t,t2t,&pt);
			b.maxx = m1->b.maxx<m2->b.maxx ? m1->b.maxx : m2->b.maxx;
		break;
		    } else if ( (y1o>y2o) != ( y1>y2 ) ) {
			xtop = xtest;
		    } else {
			xbot = xtest;
		    }
		}
		y1 = y1o; y1o = y2o; y2o = y1;
	    } else {
		y1o = y1; y2o = y2;
	    }
	}
    }
return( ilist );
}

#if 0
static void SubsetSpline1(Spline1D *subset,Spline1D *orig,
	extended tstart,extended tend,extended d) {
    extended f = tend-tstart;

    subset->a = orig->a*f*f*f;
    subset->b = (orig->b + 3*orig->a*tstart) *f*f;
    subset->c = (orig->c + (2*orig->b + 3*orig->a*tstart)*tstart) * f;
    subset->d = d;
}
#endif

static extended SplineContainsPoint(Monotonic *m,BasePoint *pt) {
    int which, nw;
    extended t;
    BasePoint slope;
    const double error = .0001;

    which = ( m->b.maxx-m->b.minx > m->b.maxy-m->b.miny )? 0 : 1;
    nw = !which;
    t = BoundIterateSplineSolve(&m->s->splines[which],m->tstart,m->tend,(&pt->x)[which],error);
    if ( (slope.x = (3*m->s->splines[0].a*t+2*m->s->splines[0].b)*t+m->s->splines[0].c)<0 )
	slope.x = -slope.x;
    if ( (slope.y = (3*m->s->splines[1].a*t+2*m->s->splines[1].b)*t+m->s->splines[1].c)<0 )
	slope.y = -slope.y;
    if ( t==-1 || (slope.y>slope.x)!=which ) {
	nw = which;
	which = 1-which;
	t = BoundIterateSplineSolve(&m->s->splines[which],m->tstart,m->tend,(&pt->x)[which],error);
    }
    if ( t!=-1 && RealWithin((&pt->x)[nw],
	   ((m->s->splines[nw].a*t+m->s->splines[nw].b)*t +
		m->s->splines[nw].c)*t + m->s->splines[nw].d,.1 ))
return( t );

return( -1 );
}

/* If two splines are coincident, then pretend they intersect at both */
/*  end-points and nowhere else */
static int CoincidentIntersect(Monotonic *m1,Monotonic *m2,BasePoint *pts,
	extended *t1s,extended *t2s) {
    const double error = .0001;
    int cnt=0;
    extended t, t2, diff;

    if ( m1==m2 || m1->next==m2 || m1->prev==m2 )
return( false );		/* Can't be coincident. Adjacent */
    /* Actually adjacent splines can double back on themselves */

    if ( (m1->xup==m2->xup && m1->yup==m2->yup) ||
	    ((m1->xup!=m2->xup || (m1->b.minx==m1->b.maxx && m2->b.minx==m2->b.maxx)) ||
	     (m1->yup!=m2->yup || (m1->b.miny==m1->b.maxy && m2->b.miny==m2->b.maxy))))
	/* A match is possible */;
    else
return( false );

    SetStartPoint(&pts[cnt],m1);
    t1s[cnt] = m1->tstart;
    if ( (t2s[cnt] = SplineContainsPoint(m2,&pts[cnt]))!=-1 )
	++cnt;

    SetEndPoint(&pts[cnt],m1);
    t1s[cnt] = m1->tend;
    if ( (t2s[cnt] = SplineContainsPoint(m2,&pts[cnt]))!=-1 )
	++cnt;

    if ( cnt!=2 ) {
	SetStartPoint(&pts[cnt],m2);
	t2s[cnt] = m2->tstart;
	if ( (t1s[cnt] = SplineContainsPoint(m1,&pts[cnt]))!=-1 )
	    ++cnt;
    }

    if ( cnt!=2 ) {
	SetEndPoint(&pts[cnt],m2);
	t2s[cnt] = m2->tend;
	if ( (t1s[cnt] = SplineContainsPoint(m1,&pts[cnt]))!=-1 )
	    ++cnt;
    }

    if ( cnt!=2 )
return( false );

    if ( RealWithin(t1s[0],t1s[1],.01) )
return( false );

    /* Ok, if we've gotten this far we know that two of the end points are  */
    /*  on both splines.                                                    */
#if 0
    /* Interesting. The following algorithem does not produce a unique      */
    /*  representation of the subset spline.  Must test each point          */
    /* Then subset each spline so that [0,1] runs along the range we just   */
    /*  found for it. Compare these two subsets, if they are almost equal   */
    /*  then assume they are coincident, and return the two endpoints as    */
    /*  the intersection points.                                            */
    SubsetSpline1(&subs1,&m1->s->splines[0],t1s[0],t1s[1],pts[0].x);
    SubsetSpline1(&subs2,&m2->s->splines[0],t2s[0],t2s[1],pts[0].x);
    if ( !RealWithin(subs1.a,subs2.a,.1) || !RealWithin(subs1.b,subs2.b,.1) ||
	    !RealWithin(subs1.c,subs2.c,.2) /* Don't need to check d, it's always pts[0].x */)
return( false );

    SubsetSpline1(&subs1,&m1->s->splines[1],t1s[0],t1s[1],pts[0].y);
    SubsetSpline1(&subs2,&m2->s->splines[1],t2s[0],t2s[1],pts[0].y);
    if ( !RealWithin(subs1.a,subs2.a,.1) || !RealWithin(subs1.b,subs2.b,.1) ||
	    !RealWithin(subs1.c,subs2.c,.2) /* Don't need to check d, it's always pts[0].x */)
return( false );

    t1s[2] = t2s[2] = -1;
return( true );
#else
    t1s[2] = t2s[2] = -1;
    if ( !m1->s->knownlinear || !m1->s->knownlinear ) {
	if ( t1s[1]<t1s[0] ) {
	    extended temp = t1s[1]; t1s[1] = t1s[0]; t1s[0] = temp;
	    temp = t2s[1]; t2s[1] = t2s[0]; t2s[0] = temp;
	}
	diff = (t1s[1]-t1s[0])/16;
	for ( t=t1s[0]+diff; t<t1s[1]-diff/4; t += diff ) {
	    BasePoint here, slope;
	    here.x = ((m1->s->splines[0].a*t+m1->s->splines[0].b)*t+m1->s->splines[0].c)*t+m1->s->splines[0].d;
	    here.y = ((m1->s->splines[1].a*t+m1->s->splines[1].b)*t+m1->s->splines[1].c)*t+m1->s->splines[1].d;
	    if ( (slope.x = (3*m1->s->splines[0].a*t+2*m1->s->splines[0].b)*t+m1->s->splines[0].c)<0 )
		slope.x = -slope.x;
	    if ( (slope.y = (3*m1->s->splines[1].a*t+2*m1->s->splines[1].b)*t+m1->s->splines[1].c)<0 )
		slope.y = -slope.y;
	    if ( slope.y>slope.x ) {
		t2 = BoundIterateSplineSolve(&m2->s->splines[1],t2s[0],t2s[1],here.y,error);
		if ( t2==-1 || !RealWithin(here.x,((m2->s->splines[0].a*t2+m2->s->splines[0].b)*t2+m2->s->splines[0].c)*t2+m2->s->splines[0].d,.1))
return( false );
	    } else {
		t2 = BoundIterateSplineSolve(&m2->s->splines[0],t2s[0],t2s[1],here.x,error);
		if ( t2==-1 || !RealWithin(here.y,((m2->s->splines[1].a*t2+m2->s->splines[1].b)*t2+m2->s->splines[1].c)*t2+m2->s->splines[1].d,.1))
return( false );
	    }
	}
    }

return( true );
#endif
}

static void FigureProperMonotonicsAtIntersections(Intersection *ilist) {
    MList *ml, *ml2, *mlnext, *prev, *p2;

    while ( ilist!=NULL ) {
	for ( ml=ilist->monos; ml!=NULL; ml=ml->next ) {
	    if ( (ml->t==ml->m->tstart && !ml->isend) ||
		    (ml->t==ml->m->tend && ml->isend))
		/* It's right */;
	    else if ( ml->t>ml->m->tstart ) {
		while ( ml->t>ml->m->tend ) {
		    ml->m = ml->m->next;
		    if ( ml->m->s!=ml->s ) {
			SOError("we could not find a matching monotonic\n" );
		break;
		    }
		}
	    } else {
		while ( ml->t<ml->m->tstart ) {
		    ml->m = ml->m->prev;
		    if ( ml->m->s!=ml->s ) {
			SOError( "we could not find a matching monotonic\n" );
		break;
		    }
		}
	    }
	    if ( ml->t==ml->m->tstart && ml->isend )
		ml->m = ml->m->prev;
	    else if ( ml->t==ml->m->tend && !ml->isend )
		ml->m = ml->m->next;
	    if ( ml->t!=ml->m->tstart && ml->t!=ml->m->tend )
		SOError( "we could not find a matching monotonic time\n" );
	}
	for ( prev=NULL, ml=ilist->monos; ml!=NULL; ml = mlnext ) {
	    mlnext = ml->next;
	    if ( ml->m->start==ml->m->end ) {
		for ( p2 = ml, ml2=ml->next; ml2!=NULL; p2=ml2, ml2 = ml2->next ) {
		    if ( ml2->m==ml->m )
		break;
		}
		if ( ml2!=NULL ) {
		    if ( ml2==mlnext ) mlnext = ml2->next;
		    p2->next = ml2->next;
		    chunkfree(ml2,sizeof(*ml2));
		}
		if ( prev==NULL )
		    ilist->monos = mlnext;
		else
		    prev->next = mlnext;
		chunkfree(ml,sizeof(*ml));
	    }
	}
#if 0
	for ( ml=ilist->monos; ml!=NULL; ml=ml->next ) {
	    Monotonic *search;
	    MList *ml2;
	    extended t;
	    if ( ml->m->start == ilist ) {
		search = ml->m->prev;
		t = ( ml->m->tstart==0 ) ? 1.0 : ml->m->tstart;
	    } else {
		search = ml->m->next;
		t = ( ml->m->tend==1.0 ) ? 0.0 : ml->m->tend;
	    }
	    for ( ml2=ilist->monos; ml2!=NULL && ml2->m!=search; ml2=ml2->next );
	    if ( ml2==NULL ) {
		ml2 = chunkalloc(sizeof(MList));
		ml2->m = search;
		ml2->s = search->s;
		ml2->t = t;
		ml2->next = ml->next;
		ml->next = ml2;
		ml = ml2;
	    }
	}
#endif
	ilist = ilist->next;
    }
}

static void Validate(Monotonic *ms, Intersection *ilist) {
    MList *ml;
    int mcnt;

    while ( ilist!=NULL ) {
	for ( mcnt=0, ml=ilist->monos; ml!=NULL; ml=ml->next ) {
	    if ( ml->m->isneeded ) ++mcnt;
	    if ( ml->m->start!=ilist && ml->m->end!=ilist )
		SOError( "Intersection (%g,%g) not on a monotonic which should contain it.\n",
			ilist->inter.x, ilist->inter.y );
	}
	if ( mcnt&1 )
	    SOError( "Odd number of needed monotonic sections at intersection. (%g,%g)\n",
		    ilist->inter.x,ilist->inter.y );
	ilist = ilist->next;
    }

    while ( ms!=NULL ) {
	if ( ms->prev->end!=ms->start )
	    SOError( "Mismatched intersection.\n");
	ms = ms->linked;
    }
}

static Intersection *FindIntersections(Monotonic *ms, enum overlap_type ot) {
    Monotonic *m1, *m2;
    BasePoint pts[9];
    extended t1s[10], t2s[10];
    Intersection *ilist=NULL;
    int i;
    int wasc;

    for ( m1=ms; m1!=NULL; m1=m1->linked ) {
	for ( m2=m1->linked; m2!=NULL; m2=m2->linked ) {
	    if ( m2->b.minx > m1->b.maxx ||
		    m2->b.maxx < m1->b.minx ||
		    m2->b.miny > m1->b.maxy ||
		    m2->b.maxy < m1->b.miny )
	continue;		/* Can't intersect */;
	    wasc = CoincidentIntersect(m1,m2,pts,t1s,t2s);
	    if ( wasc || m1->s->knownlinear || m2->s->knownlinear ||
		    (m1->s->splines[0].a==0 && m1->s->splines[1].a==0 &&
		     m2->s->splines[0].a==0 && m2->s->splines[1].a==0 )) {
		if ( !wasc && SplinesIntersect(m1->s,m2->s,pts,t1s,t2s)<=0 )
	continue;
		for ( i=0; i<4 && t1s[i]!=-1; ++i ) {
		    if ( t1s[i]>=m1->tstart && t1s[i]<=m1->tend &&
			    t2s[i]>=m2->tstart && t2s[i]<=m2->tend ) {
			ilist = AddIntersection(ilist,m1,m2,t1s[i],t2s[i],&pts[i]);
		    }
		}
	continue;
	    }
	    ilist = FindMonotonicIntersection(ilist,m1,m2);
	}
    }

    FigureProperMonotonicsAtIntersections(ilist);

    /* Now suppose we have a contour which intersects nothing? */
    /* with no intersections we lose track of it and it will vanish */
    /* That's not a good idea. Make sure each contour has at least one inter */
    if ( ot!=over_findinter && ot!=over_fisel ) {
	for ( m1=ms; m1!=NULL; m1=m2->linked ) {
	    if ( m1->start==NULL && m1->end==NULL ) {
		Intersection *il;
		il = chunkalloc(sizeof(Intersection));
		il->inter = m1->s->from->me;
		il->next = ilist;
		AddSpline(il,m1,0);
		AddSpline(il,m1->prev,1.0);
		ilist = il;
	    }
	    for ( m2=m1; m2->linked==m2->next; m2=m2->linked );
	}
    }

return( ilist );
}

static int dcmp(const void *_p1, const void *_p2) {
    const extended *dpt1 = _p1, *dpt2 = _p2;
    if ( *dpt1>*dpt2 )
return( 1 );
    else if ( *dpt1<*dpt2 )
return( -1 );

return( 0 );
}

static extended *FindOrderedEndpoints(Monotonic *ms,int which) {
    int cnt;
    Monotonic *m;
    extended *ends;
    int i,j,k;

    for ( m=ms, cnt=0; m!=NULL; m=m->linked, ++cnt );
    ends = galloc((2*cnt+1)*sizeof(extended));
    for ( m=ms, cnt=0; m!=NULL; m=m->linked, cnt+=2 ) {
	if ( m->start!=NULL )
	    ends[cnt] = (&m->start->inter.x)[which];
	else if ( m->tstart==0 )
	    ends[cnt] = (&m->s->from->me.x)[which];
	else
	    ends[cnt] = ((m->s->splines[which].a*m->tstart+m->s->splines[which].b)*m->tstart+
		    m->s->splines[which].c)*m->tstart+m->s->splines[which].d;
	if ( m->end!=NULL )
	    ends[cnt+1] = (&m->end->inter.x)[which];
	else if ( m->tend==1.0 )
	    ends[cnt+1] = (&m->s->to->me.x)[which];
	else
	    ends[cnt+1] = ((m->s->splines[which].a*m->tend+m->s->splines[which].b)*m->tend+
		    m->s->splines[which].c)*m->tend+m->s->splines[which].d;
    }

    qsort(ends,cnt,sizeof(extended),dcmp);
    for ( i=0; i<cnt; ++i ) {
	for ( j=i; j<cnt && ends[i]==ends[j]; ++j );
	if ( j>i+1 ) {
	    for ( k=i+1; j<cnt; ends[k++] = ends[j++]);
	    cnt-= j-k;
	}
    }
    ends[cnt] = 1e10;
return( ends );
}

static int mcmp(const void *_p1, const void *_p2) {
    const Monotonic * const *mpt1 = _p1, * const *mpt2 = _p2;
    if ( (*mpt1)->other>(*mpt2)->other )
return( 1 );
    else if ( (*mpt1)->other<(*mpt2)->other )
return( -1 );

return( 0 );
}

int MonotonicFindAt(Monotonic *ms,int which, extended test, Monotonic **space ) {
    /* Find all monotonic sections which intersect the line (x,y)[which] == test */
    /*  find the value of the other coord on that line */
    /*  Order them (by the other coord) */
    /*  then run along that line figuring out which monotonics are needed */
    extended t;
    Monotonic *m, *mm;
    int i, j, k, cnt;
    const double error = .0001;
    int nw = !which;

    for ( m=ms, i=0; m!=NULL; m=m->linked ) {
	if (( which==0 && test >= m->b.minx && test <= m->b.maxx ) ||
		( which==1 && test >= m->b.miny && test <= m->b.maxy )) {
	    /* Lines parallel to the direction we are testing just get in the */
	    /*  way and don't add any useful info */
	    if ( m->s->knownlinear &&
		    (( which==1 && m->s->from->me.y==m->s->to->me.y ) ||
			(which==0 && m->s->from->me.x==m->s->to->me.x)))
    continue;
	    t = BoundIterateSplineSolve(&m->s->splines[which],m->tstart,m->tend,test,error);
	    if ( t==-1 )
    continue;
	    m->t = t;
	    if ( t==m->tend ) t -= (m->tend-m->tstart)/100;
	    else if ( t==m->tstart ) t += (m->tend-m->tstart)/100;
	    m->other = ((m->s->splines[nw].a*t+m->s->splines[nw].b)*t+
		    m->s->splines[nw].c)*t+m->s->splines[nw].d;
	    space[i++] = m;
	}
    }
    cnt = i;

    /* Things get a little tricky at end-points */
    for ( i=0; i<cnt; ++i ) {
	m = space[i];
	if ( m->t==m->tend ) {
	    /* Ignore horizontal/vertical lines (as appropriate) */
	    for ( mm=m->next; mm!=m; mm=mm->next ) {
		if ( !mm->s->knownlinear )
	    break;
		if (( which==1 && mm->s->from->me.y!=m->s->to->me.y ) ||
			(which==0 && mm->s->from->me.x!=m->s->to->me.x))
	    break;
	    }
	} else if ( m->t==m->tstart ) {
	    for ( mm=m->prev; mm!=m; mm=mm->prev ) {
		if ( !mm->s->knownlinear )
	    break;
		if (( which==1 && mm->s->from->me.y!=m->s->to->me.y ) ||
			(which==0 && mm->s->from->me.x!=m->s->to->me.x))
	    break;
	    }
	} else
    break;
	/* If the next monotonic continues in the same direction, and we found*/
	/*  it too, then don't count both. They represent the same intersect */
	/* If they are in oposite directions then they cancel each other out */
	/*  and that is correct */
	if ( mm!=m &&	/* Should always be true */
		(&mm->xup)[which]==(&m->xup)[which] ) {
	    for ( j=cnt-1; j>=0; --j )
		if ( space[j]==mm )
	    break;
	    if ( j!=-1 ) {
		/* remove mm */
		for ( k=j+1; k<cnt; ++k )
		    space[k-1] = space[k];
		--cnt;
		if ( i>j ) --i;
	    }
	}
    }

    space[cnt] = NULL; space[cnt+1] = NULL;
    qsort(space,cnt,sizeof(Monotonic *),mcmp);
return(cnt);
}

static void FigureNeeds(Monotonic *ms,int which, extended test, Monotonic **space,
	enum overlap_type ot, int ignore_close) {
    /* Find all monotonic sections which intersect the line (x,y)[which] == test */
    /*  find the value of the other coord on that line */
    /*  Order them (by the other coord) */
    /*  then run along that line figuring out which monotonics are needed */
    int i, j, winding, ew, was_close, close;

    MonotonicFindAt(ms,which,test,space);

#if 0		/* Really slow, and it fixes some problems at the expense of causing others */
    for ( i=0; space[i+1]!=NULL; ++i ) {
	/* If two splines are very close to each other, we may miss an */
	/*  intersection. If that has happened, reorder the splines */
	if ( space[i+1]->other - space[i]->other < .1 ) {
	    extended oi, oi1, ti, ti1, test2;
	    if ( which==1 ) {
		if ( space[i+1]->b.miny > space[i]->b.miny )
		    test2 = space[i]->b.miny;
		else
		    test2 = space[i+1]->b.miny;
	    } else {
		if ( space[i+1]->b.minx > space[i]->b.minx )
		    test2 = space[i]->b.minx;
		else
		    test2 = space[i+1]->b.minx;
	    }
	    ti = BoundIterateSplineSolve(&space[i]->s->splines[which],
		    space[i]->tstart,space[i]->tend,test2,error);
	    ti1= BoundIterateSplineSolve(&space[i+1]->s->splines[which],
		    space[i+1]->tstart,space[i+1]->tend,test2,error);
	    oi = ((space[i]->s->splines[nw].a*ti+space[i]->s->splines[nw].b)*ti+
		    space[i]->s->splines[nw].c)*ti+space[i]->s->splines[nw].d;
	    oi1= ((space[i+1]->s->splines[nw].a*ti1+space[i+1]->s->splines[nw].b)*ti1+
		    space[i+1]->s->splines[nw].c)*ti1+space[i+1]->s->splines[nw].d;
	    if ( oi1<oi ) {
		m = space[i];
		space[i] = space[i+1];
		space[i+1] = m;
 fprintf( stderr, "Flipped\n" );
	    }
	}
    }
#endif

    winding = 0; ew = 0; was_close = false;
    for ( i=0; space[i]!=NULL; ++i ) {
	int needed, unneeded, inverted=false;
	Monotonic *m;
	int new;
	int nwinding;
      retry:
	needed = false, unneeded = false;
	nwinding=winding;
	new=ew;
	m = space[i];
	if ( m->exclude )
	    new += ( (&m->xup)[which] ? 1 : -1 );
	else
	    nwinding += ( (&m->xup)[which] ? 1 : -1 );
	if ( ot==over_remove || ot==over_rmselected ) {
	    if ( winding==0 || nwinding==0 )
		needed = true;
	    else
		unneeded = true;
	} else if ( ot==over_intersect || ot==over_intersel ) {
	    if ( (winding>-2 && winding<2 && nwinding>-2 && nwinding<2) ||
		    ((winding<=-2 || winding>=2) && (nwinding<=-2 && nwinding>=2)))
		unneeded = true;
	    else
		needed = true;
	} else if ( ot == over_exclude ) {
	    if ( (( winding==0 || nwinding==0 ) && ew==0 && new==0 ) ||
		    (winding!=0 && (( ew!=0 && new==0 ) || ( ew==0 && new!=0))) )
		needed = true;
	    else
		unneeded = true;
	}
	if ( space[i+1]!=NULL )
	    close = space[i+1]->other-space[i]->other < 1;
	else
	    close = false;
	if (( !close && !was_close ) || ignore_close ) {
	    if (( m->isneeded || m->isunneeded ) && m->isneeded!=needed ) {
		for ( j=i+1; space[j]!=NULL && space[j]->other-m->other<.5; ++j ) {
		    if ( space[j]->start==m->start && space[j]->end==m->end &&
			    (space[j]->isneeded == needed ||
			     (!space[j]->isneeded && !space[j]->isunneeded))) {
			space[i] = space[j];
			space[j] = m;
			m = space[i];
		break;
		    } else if ( !inverted && space[j]->other-m->other<.001 &&
			    (((&space[j]->xup)[which] == (&m->xup)[which] &&
			      (space[j]->isneeded == needed ||
			       (!space[j]->isneeded && !space[j]->isunneeded))) ||
			     ((&space[j]->xup)[which] != (&m->xup)[which] &&
			      (space[j]->isneeded != needed ||
			       (!space[j]->isneeded && !space[j]->isunneeded)))) ) {
			space[i] = space[j];
			space[j] = m;
			inverted = true;
		  goto retry;
		    }
		}
	    }
	    if ( !m->isneeded && !m->isunneeded ) {
		m->isneeded = needed; m->isunneeded = unneeded;
		m->when_set = test;		/* Debugging */
	    } else if ( m->isneeded!=needed || m->isunneeded!=unneeded )
		SOError( "monotonic is both needed and unneeded.\n" );
	}
	winding = nwinding;
	ew = new;
	was_close = close;
    }
    if ( winding!=0 )
	SOError( "Winding number did not return to 0 when %s=%g\n",
		which ? "y" : "x", test );
}

struct gaps { extended test, len; int which; };

static int gcmp(const void *_p1, const void *_p2) {
    const struct gaps *gpt1 = _p1, *gpt2 = _p2;
    if ( gpt1->len > gpt2->len )
return( 1 );
    else if ( gpt1->len < gpt2->len )
return( -1 );

return( 0 );
}

static void FindNeeded(Monotonic *ms,enum overlap_type ot) {
    extended *ends[2];
    Monotonic *m, **space;
    extended top, bottom, test;
    int t,b,i,j,k,cnt,which;
    struct gaps *gaps;
    extended min_gap;

    if ( ms==NULL )
return;

    ends[0] = FindOrderedEndpoints(ms,0);
    ends[1] = FindOrderedEndpoints(ms,1);

    for ( m=ms, cnt=0; m!=NULL; m=m->linked, ++cnt );
    space = galloc((cnt+2)*sizeof(Monotonic*));
    gaps = galloc(2*cnt*sizeof(struct gaps));

    /* Look for the longest splines without interruptions first. These are */
    /* least likely to cause problems and will give us a good basis from which*/
    /* to make guesses should rounding errors occur later */
    for ( j=k=0; j<2; ++j )
	for ( i=0; ends[j][i+1]!=1e10; ++i ) {
	    gaps[k].which = j;
	    gaps[k].len = (ends[j][i+1]-ends[j][i]);
	    gaps[k++].test = (ends[j][i+1]+ends[j][i])/2;
	}
    qsort(gaps,k,sizeof(struct gaps),gcmp);
    min_gap = 1e10;
    for ( m=ms; m!=NULL; m=m->linked ) {
	if ( m->b.maxx-m->b.minx > m->b.maxy-m->b.miny ) {
	    if ( min_gap > m->b.maxx-m->b.minx ) min_gap = m->b.maxx-m->b.minx;
	} else {
	    if ( m->b.maxy-m->b.miny==0 )
		fprintf( stderr, "Foo\n");
	    if ( min_gap > m->b.maxy-m->b.miny ) min_gap = m->b.maxy-m->b.miny;
	}
    }
    if ( min_gap<.5 ) min_gap = .5;
    for ( i=0; i<k && gaps[i].len>=min_gap; ++i )
	FigureNeeds(ms,gaps[i].which,gaps[i].test,space,ot,0);

    for ( m=ms; m!=NULL; m=m->linked ) if ( !m->isneeded && !m->isunneeded ) {
	if ( m->b.maxx-m->b.minx > m->b.maxy-m->b.miny ) {
	    top = m->b.maxx;
	    bottom = m->b.minx;
	    which = 0;
	} else {
	    top = m->b.maxy;
	    bottom = m->b.miny;
	    which = 1;
	}
	for ( b=0; ends[which][b]<=bottom; ++b );
	for ( t=b; ends[which][t]<top; ++t );
	--t;
	/* b points to an endpoint which is greater than bottom */
	/* t points to an endpoint which is less than top */
	test = (top+bottom)/2;
	for ( i=b; i<=t; ++i ) {
	    if ( RealNearish(test,ends[which][i]) ) {
		if ( i==b )
		    test = (bottom+ends[which][i])/2;
		else
		    test = (ends[which][i-1]+ends[which][i])/2;
	break;
	    }
	}
	FigureNeeds(ms,which,test,space,ot,1);
    }
    free(ends[0]);
    free(ends[1]);
    free(space);
    free(gaps);
}

static void FindUnitVectors(Intersection *ilist) {
    MList *ml;
    Intersection *il;
    BasePoint u;
    double len;

    for ( il=ilist; il!=NULL; il=il->next ) {
	for ( ml=il->monos; ml!=NULL; ml=ml->next ) {
	    if ( ml->m->isneeded ) {
		Spline *s = ml->m->s;
		double t1, t2;
		t1 = ml->t;
		if ( ml->isend )
		    t2 = ml->t - (ml->t-ml->m->tstart)/20.0;
		else
		    t2 = ml->t + (ml->m->tend-ml->t)/20.0;
		u.x = ((s->splines[0].a*t1 + s->splines[0].b)*t1 + s->splines[0].c)*t1 -
			((s->splines[0].a*t2 + s->splines[0].b)*t2 + s->splines[0].c)*t2;
		u.y = ((s->splines[1].a*t1 + s->splines[1].b)*t1 + s->splines[1].c)*t1 -
			((s->splines[1].a*t2 + s->splines[1].b)*t2 + s->splines[1].c)*t2;
		len = u.x*u.x + u.y*u.y;
		if ( len!=0 ) {
		    len = sqrt(len);
		    u.x /= len;
		    u.y /= len;
		}
		ml->unit = u;
	    }
	}
    }
}

static void TestForBadDirections(Intersection *ilist) {
    /* If we have a glyph with at least two contours one drawn clockwise, */
    /*  one counter, and these two intersect, then our algorithm will */
    /*  not remove what appears to the user to be an overlap. Warn about */
    /*  this. */
    /* I think it happens iff all exits from an intersection are needed */
    MList *ml, *ml2;
    int cnt, ncnt;
    Intersection *il;

    /* If we have two splines one going from a->b and the other from b->a */
    /*  tracing exactly the same route, then they should cancel each other */
    /*  out. But depending on the order we hit them they may both be marked */
    /*  needed */  /* OverlapBugs.sfd: asciicircumflex */
    for ( il=ilist; il!=NULL; il=il->next ) {
	for ( ml=il->monos; ml!=NULL; ml=ml->next ) {
	    if ( ml->m->isneeded && ml->m->s->knownlinear &&
		    ml->m->start!=NULL && ml->m->end!=NULL ) {
		for ( ml2 = ml->next; ml2!=NULL; ml2=ml2->next ) {
		    if ( ml2->m->isneeded && ml2->m->s->knownlinear &&
			    ml2->m->start == ml->m->end &&
			    ml2->m->end == ml->m->start ) {
			ml2->m->isneeded = false;
			ml->m->isneeded = false;
			ml2->m->isunneeded = true;
			ml->m->isunneeded = true;
		break;
		    }
		}
	    }
	}
    }

    while ( ilist!=NULL ) {
	cnt = ncnt = 0;
	for ( ml = ilist->monos; ml!=NULL; ml=ml->next ) {
	    ++cnt;
	    if ( ml->m->isneeded ) ++ncnt;
	}
#if 0
	if ( cnt>=4 && ncnt==cnt ) {
	    ff_post_notice(_("Warning"),_("Glyph %.40s contains an overlapped region where two contours with opposite orientations intersect. This will not be removed. In many cases doing Element->Correct Direction before Remove Overlap will improve matters."),
		    glyphname );
	    fprintf( stderr, "%s: Fully needed intersection at (%g,%g)\n",
		    glyphname,
		    ilist->inter.x, ilist->inter.y );
    break;
	}
#endif
	ilist = ilist->next;
    }
}

static void MonoFigure(Spline *s,extended firstt,extended endt, SplinePoint *first,
	SplinePoint *end) {
    extended f;
    Spline1D temp;

    f = endt - firstt;
    /*temp.d = first->me.x;*/
    /*temp.a = s->splines[0].a*f*f*f;*/
    temp.b = (s->splines[0].b + 3*s->splines[0].a*firstt) *f*f;
    temp.c = (s->splines[0].c + 2*s->splines[0].b*firstt + 3*s->splines[0].a*firstt*firstt) * f;
    first->nextcp.x = first->me.x + temp.c/3;
    end->prevcp.x = first->nextcp.x + (temp.b+temp.c)/3;
    if ( temp.c>-.01 && temp.c<.01 ) first->nextcp.x = first->me.x;
    if ( (temp.b+temp.c)>-.01 && (temp.b+temp.c)<.01 ) end->prevcp.x = end->me.x;

    temp.b = (s->splines[1].b + 3*s->splines[1].a*firstt) *f*f;
    temp.c = (s->splines[1].c + 2*s->splines[1].b*firstt + 3*s->splines[1].a*firstt*firstt) * f;
    first->nextcp.y = first->me.y + temp.c/3;
    end->prevcp.y = first->nextcp.y + (temp.b+temp.c)/3;
    if ( temp.c>-.01 && temp.c<.01 ) first->nextcp.y = first->me.y;
    if ( (temp.b+temp.c)>-.01 && (temp.b+temp.c)<.01 ) end->prevcp.y = end->me.y;
    first->nonextcp = false; end->noprevcp = false;
    SplineMake3(first,end);
    if ( SplineIsLinear(first->next)) {
	first->nextcp = first->me;
	end->prevcp = end->me;
	first->nonextcp = end->noprevcp = true;
	SplineRefigure(first->next);
    }
}

static Intersection *MonoFollow(Intersection *curil, Monotonic *m) {
    Monotonic *mstart=m;

    if ( m->start==curil ) {
	while ( m!=NULL && m->end==NULL ) {
	    m=m->next;
	    if ( m==mstart )
	break;
	}
	if ( m==NULL )
return( NULL );

return( m->end );
    } else {
	while ( m!=NULL && m->start==NULL ) {
	    m=m->prev;
	    if ( m==mstart )
	break;
	}
	if ( m==NULL )
return( NULL );

return( m->start );
    }
}

static int MonoGoesSomewhereUseful(Intersection *curil, Monotonic *m) {
    Intersection *nextil = MonoFollow(curil,m);
    MList *ml;
    int cnt;

    if ( nextil==NULL )
return( false );
    cnt = 0;
    for ( ml=nextil->monos; ml!=NULL ; ml=ml->next )
	if ( ml->m->isneeded )
	    ++cnt;
    if ( cnt>=2 )	/* One for the mono that one in, one for another going out... */
return( true );

return( false );
}

static MList *FindMLOfM(Intersection *curil,Monotonic *finalm) {
    MList *ml;

    for ( ml=curil->monos; ml!=NULL; ml=ml->next ) {
	if ( ml->m==finalm )
return( ml );
    }
#if 0
	if ( ml->isend ) {
	    for ( m=ml->m; m!=NULL ; m=m->prev ) {
		if ( m==finalm )
return( ml );
		if ( m->start != NULL )
	    break;
	    }
	} else {
	    for ( m=ml->m; m!=NULL; m=m->next ) {
		if ( m==finalm )
return( ml );
		if ( m->end!=NULL )
	    break;
	    }
	}
    }
#endif
return( NULL );
}

static SplinePoint *MonoFollowForward(Intersection **curil, MList *ml,
	SplinePoint *last, Monotonic **finalm) {
    SplinePoint *mid;
    Monotonic *m = ml->m, *mstart;

    forever {
	for ( mstart = m; m->s==mstart->s; m=m->next) {
	    if ( !m->isneeded )
		SOError( "Expected needed monotonic.\n" );
	    m->isneeded = false;		/* Mark as used */
	    if ( m->end!=NULL )
	break;
	}
	if ( m->s==mstart->s ) {
	    if ( m->end==NULL ) SOError( "Invariant condition does not hold.\n" );
	    mid = SplinePointCreate(m->end->inter.x,m->end->inter.y);
	} else {
	    m = m->prev;
	    mid = SplinePointCreate(m->s->to->me.x,m->s->to->me.y);
	}
	if ( mstart->tstart==0 && m->tend==1.0 ) {
	    /* I check for this special case to avoid rounding errors */
	    last->nextcp = m->s->from->nextcp;
	    last->nonextcp = m->s->from->nonextcp;
	    mid->prevcp = m->s->to->prevcp;
	    mid->noprevcp = m->s->to->noprevcp;
	    SplineMake3(last,mid);
	} else {
	    MonoFigure(m->s,mstart->tstart,m->tend,last,mid);
	}
	last = mid;
	if ( m->end!=NULL ) {
	    *curil = m->end;
	    *finalm = m;
return( last );
	}
	m = m->next;
    }
}

static SplinePoint *MonoFollowBackward(Intersection **curil, MList *ml,
	SplinePoint *last, Monotonic **finalm) {
    SplinePoint *mid;
    Monotonic *m = ml->m, *mstart;

    forever {
	for ( mstart=m; m->s==mstart->s; m=m->prev) {
	    if ( !m->isneeded )
		SOError( "Expected needed monotonic.\n" );
	    m->isneeded = false;		/* Mark as used */
	    if ( m->start!=NULL )
	break;
	}
	if ( m->s==mstart->s ) {
	    if ( m->start==NULL ) SOError( "Invariant condition does not hold.\n" );
	    mid = SplinePointCreate(m->start->inter.x,m->start->inter.y);
	} else {
	    m = m->next;
	    mid = SplinePointCreate(m->s->from->me.x,m->s->from->me.y);
	}
	if ( m->s->knownlinear ) mid->pointtype = pt_corner;
	if ( mstart->tend==1.0 && m->tstart==0 ) {
	    /* I check for this special case to avoid rounding errors */
	    last->nextcp = m->s->to->prevcp;
	    last->nonextcp = m->s->to->noprevcp;
	    mid->prevcp = m->s->from->nextcp;
	    mid->noprevcp = m->s->from->nonextcp;
	    SplineMake3(last,mid);
	} else {
	    MonoFigure(m->s,mstart->tend,m->tstart,last,mid);
	}
	last = mid;
	if ( m->start!=NULL ) {
	    *curil = m->start;
	    *finalm = m;
return( last );
	}
	m = m->prev;
    }
}

static SplineSet *JoinAContour(Intersection *startil,MList *ml) {
    SplineSet *ss = chunkalloc(sizeof(SplineSet));
    SplinePoint *last;
    Intersection *curil;
    int allexclude = ml->m->exclude;
    Monotonic *finalm;
    MList *lastml;

    ss->first = last = SplinePointCreate(startil->inter.x,startil->inter.y);
    curil = startil;
    forever {
	if ( allexclude && !ml->m->exclude ) allexclude = false;
	finalm = NULL;
	if ( ml->m->start==curil ) {
	    last = MonoFollowForward(&curil,ml,last,&finalm);
	} else if ( ml->m->end==curil ) {
	    last = MonoFollowBackward(&curil,ml,last,&finalm);
	} else {
	    SOError( "Couldn't find endpoint (%g,%g).\n",
		    curil->inter.x, curil->inter.y );
	    ml->m->isneeded = false;		/* Prevent infinite loops */
	    ss->last = last;
    break;
	}
	if ( curil==startil ) {
	    ss->first->prev = last->prev;
	    ss->first->prevcp = last->prevcp;
	    ss->first->noprevcp = last->noprevcp;
	    last->prev->to = ss->first;
	    SplinePointFree(last);
	    ss->last = ss->first;
    break;
	}
	lastml = FindMLOfM(curil,finalm);
	if ( lastml==NULL ) {
	    IError("Could not find finalm");
	    /* Try to preserve direction */
	    for ( ml=curil->monos; ml!=NULL && (!ml->m->isneeded || ml->m->end==curil); ml=ml->next );
	    if ( ml==NULL )
		for ( ml=curil->monos; ml!=NULL && !ml->m->isneeded; ml=ml->next );
	} else {
	    int k; MList *bestml; double bestdot;
	    for ( k=0; k<2; ++k ) {
		bestml = NULL; bestdot = -2;
		for ( ml=curil->monos; ml!=NULL ; ml=ml->next ) {
		    if ( ml->m->isneeded && (ml->m->start==curil || k) ) {
			double dot = lastml->unit.x*ml->unit.x + lastml->unit.y*ml->unit.y;
			if ( dot>bestdot ) {
			    bestml = ml;
			    bestdot = dot;
			}
		    }
		}
		if ( bestml!=NULL )
	    break;
	    }
	    ml = bestml;
	}
	if ( ml==NULL ) {
	    for ( ml=curil->monos; ml!=NULL ; ml=ml->next )
		if ( ml->m->isunneeded && ml->m->start==curil &&
			MonoFollow(curil,ml->m)==startil )
	    break;
	    if ( ml==NULL )
		for ( ml=curil->monos; ml!=NULL ; ml=ml->next )
		    if ( ml->m->isunneeded && ml->m->end==curil &&
			    MonoFollow(curil,ml->m)==startil )
		break;
	    if ( ml!=NULL ) {
		SOError("Closing contour with unneeded path\n" );
		ml->m->isneeded = true;
	    }
	}
	if ( ml==NULL ) {
	    SOError( "couldn't find a needed exit from an intersection\n" );
	    ss->last = last;
    break;
	}
    }
    SPLCatagorizePoints(ss);
    if ( allexclude && SplinePointListIsClockwise(ss)==1 )
	SplineSetReverse(ss);
return( ss );
}

static SplineSet *FindMatchingContour(SplineSet *head,SplineSet *cur) {
    SplineSet *test;

    for ( test=head; test!=NULL; test=test->next ) {
	if ( test->first->prev==NULL &&
		test->first->me.x==cur->last->me.x && test->first->me.y==cur->last->me.y &&
		test->last->me.x==cur->first->me.x && test->last->me.y==cur->first->me.y )
    break;
    }
    if ( test==NULL ) {
	for ( test=head; test!=NULL; test=test->next ) {
	    if ( test->first->prev==NULL &&
		    test->last->me.x==cur->last->me.x && test->last->me.y==cur->last->me.y &&
		    test->first->me.x==cur->first->me.x && test->first->me.y==cur->first->me.y ) {
		SplineSetReverse(cur);
	break;
	    }
	}
    }
    if ( test==NULL ) {
	for ( test=head; test!=NULL; test=test->next ) {
	    if ( test->first->prev==NULL &&
		    ((test->first->me.x==cur->last->me.x && test->first->me.y==cur->last->me.y) ||
		     (test->last->me.x==cur->first->me.x && test->last->me.y==cur->first->me.y )))
	break;
	}
    }
    if ( test==NULL ) {
	for ( test=head; test!=NULL; test=test->next ) {
	    if ( test->first->prev==NULL &&
		    ((test->last->me.x==cur->last->me.x && test->last->me.y==cur->last->me.y) ||
		     (test->first->me.x==cur->first->me.x && test->first->me.y==cur->first->me.y ))) {
		SplineSetReverse(cur);
	break;
	    }
	}
    }
return( test );
}

static SplineSet *JoinAllNeeded(Intersection *ilist) {
    Intersection *il;
    SplineSet *head=NULL, *last=NULL, *cur, *test;
    MList *ml;

    for ( il=ilist; il!=NULL; il=il->next ) {
	/* Try to preserve direction */
	forever {
	    for ( ml=il->monos; ml!=NULL && (!ml->m->isneeded || ml->m->end==il); ml=ml->next );
	    if ( ml==NULL )
		for ( ml=il->monos; ml!=NULL && !ml->m->isneeded; ml=ml->next );
	    if ( ml==NULL )
	break;
	    if ( !MonoGoesSomewhereUseful(il,ml->m)) {
		SOError("Humph. This monotonic leads nowhere.\n" );
	/* break; */
	    }
	    cur = JoinAContour(il,ml);
	    if ( head==NULL )
		head = cur;
	    else {
		if ( cur->first->prev==NULL ) {
		    /* Open contours are errors. See if we had an earlier error */
		    /*  to which we can join this */
		    test = FindMatchingContour(head,cur);
		    if ( test!=NULL ) {
			if ( test->first->me.x==cur->last->me.x && test->first->me.y==cur->last->me.y ) {
			    test->first->prev = cur->last->prev;
			    cur->last->prev->to = test->first;
			    SplinePointFree(cur->last);
			    if ( test->last->me.x==cur->first->me.x && test->last->me.y==cur->first->me.y ) {
				test->last->next = cur->first->next;
			        cur->first->next->from = test->last;
			        SplinePointFree(cur->first);
			        test->last = test->first;
			    } else
				test->first = cur->first;
			} else {
			    if ( test->last->me.x!=cur->first->me.x || test->last->me.y!=cur->first->me.y )
				SOError( "Join failed");
			    else {
				test->last->next = cur->first->next;
			        cur->first->next->from = test->last;
			        SplinePointFree(cur->first);
			        test->last = test->first;
			    }
			}
			cur->first = cur->last = NULL;
			SplinePointListFree(cur);
			cur=NULL;
		    }
		}
		if ( cur!=NULL )
		    last->next = cur;
	    }
	    if ( cur!=NULL )
		last = cur;
	}
    }
return( head );
}

static SplineSet *MergeOpenAndFreeClosed(SplineSet *new,SplineSet *old,
	enum overlap_type ot) {
    SplineSet *next;

    while ( old!=NULL ) {
	next = old->next;
	if ( old->first->prev==NULL ||
		(( ot==over_rmselected || ot==over_intersel || ot==over_fisel) &&
		  !SSIsSelected(old)) ) {
	    old->next = new;
	    new = old;
	} else {
	    old->next = NULL;
	    SplinePointListFree(old);
	}
	old = next;
    }
return(new);
}

void FreeMonotonics(Monotonic *m) {
    Monotonic *next;

    while ( m!=NULL ) {
	next = m->linked;
	chunkfree(m,sizeof(*m));
	m = next;
    }
}

static void FreeMList(MList *ml) {
    MList *next;

    while ( ml!=NULL ) {
	next = ml->next;
	chunkfree(ml,sizeof(*ml));
	ml = next;
    }
}

static void FreeIntersections(Intersection *ilist) {
    Intersection *next;

    while ( ilist!=NULL ) {
	next = ilist->next;
	FreeMList(ilist->monos);
	chunkfree(ilist,sizeof(*ilist));
	ilist = next;
    }
}

static void MonoSplit(Monotonic *m) {
    Spline *s = m->s;
    SplinePoint *last = s->from;
    SplinePoint *final = s->to;
    extended lastt = 0;

    last->next = NULL;
    final->prev = NULL;
    while ( m!=NULL && m->s==s && m->tend<1 ) {
	if ( m->end!=NULL ) {
	    SplinePoint *mid = SplinePointCreate(m->end->inter.x,m->end->inter.y);
	    if ( m->s->knownlinear ) mid->pointtype = pt_corner;
	    MonoFigure(s,lastt,m->tend,last,mid);
	    lastt = m->tend;
	    last = mid;
	}
	m = m->linked;
    }
    MonoFigure(s,lastt,1.0,last,final);
    SplineFree(s);
}

static void FixupIntersectedSplines(Monotonic *ms) {
    /* If all we want is intersections, then the contours are already correct */
    /*  all we need to do is run through the Monotonic list and when we find */
    /*  an intersection, make sure it has real splines around it */
    Monotonic *m;
    int needs_split;

    while ( ms!=NULL ) {
	needs_split = false;
	for ( m=ms; m!=NULL && m->s==ms->s; m=m->linked ) {
	    if ( (m->tstart!=0 && m->start!=NULL) || (m->tend!=1 && m->end!=NULL))
		needs_split = true;
	}
	if ( needs_split )
	    MonoSplit(ms);
	ms = m;
    }
}

static int BpClose(BasePoint *here, BasePoint *there, double error) {
    extended dx, dy;

    if ( (dx = here->x-there->x)<0 ) dx= -dx;
    if ( (dy = here->y-there->y)<0 ) dy= -dy;
    if ( dx<error && dy<error )
return( true );

return( false );
}

static SplineSet *SSRemoveTiny(SplineSet *base) {
    DBounds b;
    double error;
    extended test, dx, dy;
    SplineSet *prev = NULL, *head = base, *ssnext;
    SplinePoint *sp, *nsp;

    SplineSetQuickBounds(base,&b);
    error = b.maxy-b.miny;
    test = b.maxx-b.minx;
    if ( test>error ) error = test;
    if ( (test = b.maxy)<0 ) test = -test;
    if ( test>error ) error = test;
    if ( (test = b.maxx)<0 ) test = -test;
    if ( test>error ) error = test;
    error /= 30000;

    while ( base!=NULL ) {
	ssnext = base->next;
	for ( sp=base->first; ; ) {
	    if ( sp->next==NULL )
	break;
	    nsp = sp->next->to;
	    if ( BpClose(&sp->me,&nsp->me,error) ) {
		if ( BpClose(&sp->me,&sp->nextcp,2*error) &&
			BpClose(&nsp->me,&nsp->prevcp,2*error)) {
		    /* Remove the spline */
		    if ( nsp==sp ) {
			/* Only this spline in the contour, so remove the contour */
			base->next = NULL;
			SplinePointListFree(base);
			if ( prev==NULL )
			    head = ssnext;
			else
			    prev->next = ssnext;
			base = NULL;
	break;
		    }
		    SplineFree(sp->next);
		    if ( nsp->nonextcp ) {
			sp->nextcp = sp->me;
			sp->nonextcp = true;
		    } else {
			sp->nextcp = nsp->nextcp;
			sp->nonextcp = false;
		    }
		    sp->nextcpdef = nsp->nextcpdef;
		    sp->next = nsp->next;
		    if ( nsp->next!=NULL ) {
			nsp->next->from = sp;
			SplineRefigure(sp->next);
		    }
		    if ( nsp==base->last )
			base->last = sp;
		    if ( nsp==base->first )
			base->first = sp;
		    SplinePointFree(nsp);
		    if ( sp->next==NULL )
	break;
		    nsp = sp->next->to;
		} else {
		    /* Leave the spline, but move the two points together */
		    BasePoint new;
		    new.x = (sp->me.x+nsp->me.x)/2;
		    new.y = (sp->me.y+nsp->me.y)/2;
		    dx = new.x-sp->me.x; dy = new.y-sp->me.y;
		    sp->me = new;
		    sp->nextcp.x += dx; sp->nextcp.y += dy;
		    sp->prevcp.x += dx; sp->prevcp.y += dy;
		    dx = new.x-nsp->me.x; dy = new.y-nsp->me.y;
		    nsp->me = new;
		    nsp->nextcp.x += dx; nsp->nextcp.y += dy;
		    nsp->prevcp.x += dx; nsp->prevcp.y += dy;
		    SplineRefigure(sp->next);
		    if ( sp->prev ) SplineRefigure(sp->prev);
		    if ( nsp->next ) SplineRefigure(nsp->next);
		}
	    }
	    sp = nsp;
	    if ( sp==base->first )
	break;
	}
	if ( sp->prev!=NULL && !sp->noprevcp ) {
	    int refigure = false;
	    if ( sp->me.x-sp->prevcp.x>-error && sp->me.x-sp->prevcp.x<error ) {
		sp->prevcp.x = sp->me.x;
		refigure = true;
	    }
	    if ( sp->me.y-sp->prevcp.y>-error && sp->me.y-sp->prevcp.y<error ) {
		sp->prevcp.y = sp->me.y;
		refigure = true;
	    }
	    if ( sp->me.x==sp->prevcp.x && sp->me.y==sp->prevcp.y )
		sp->noprevcp = true;
	    if ( refigure )
		SplineRefigure(sp->prev);
	}
	if ( sp->next!=NULL && !sp->nonextcp ) {
	    int refigure = false;
	    if ( sp->me.x-sp->nextcp.x>-error && sp->me.x-sp->nextcp.x<error ) {
		sp->nextcp.x = sp->me.x;
		refigure = true;
	    }
	    if ( sp->me.y-sp->nextcp.y>-error && sp->me.y-sp->nextcp.y<error ) {
		sp->nextcp.y = sp->me.y;
		refigure = true;
	    }
	    if ( sp->me.x==sp->nextcp.x && sp->me.y==sp->nextcp.y )
		sp->nonextcp = true;
	    if ( refigure )
		SplineRefigure(sp->next);
	}
	if ( base!=NULL )
	    prev = base;
	base = ssnext;
    }

return( head );
}

static void RemoveNextSP(SplinePoint *psp,SplinePoint *sp,SplinePoint *nsp,
	SplineSet *base) {
    if ( psp==nsp ) {
	SplineFree(psp->next);
	psp->next = psp->prev;
	psp->next->from = psp;
	SplinePointFree(sp);
	SplineRefigure(psp->prev);
    } else {
	psp->next = nsp->next;
	psp->next->from = psp;
	psp->nextcp = nsp->nextcp;
	psp->nonextcp = nsp->nonextcp;
	psp->nextcpdef = nsp->nextcpdef;
	SplineFree(sp->prev);
	SplineFree(sp->next);
	SplinePointFree(sp);
	SplinePointFree(nsp);
	SplineRefigure(psp->next);
    }
    if ( base->first==sp || base->first==nsp )
	base->first = psp;
    if ( base->last==sp || base->last==nsp )
	base->last = psp;
}

static void RemovePrevSP(SplinePoint *psp,SplinePoint *sp,SplinePoint *nsp,
	SplineSet *base) {
    if ( psp==nsp ) {
	SplineFree(nsp->prev);
	nsp->prev = nsp->next;
	nsp->prev->to = nsp;
	SplinePointFree(sp);
	SplineRefigure(nsp->next);
    } else {
	nsp->prev = psp->prev;
	nsp->prev->to = nsp;
	nsp->prevcp = nsp->me;
	nsp->noprevcp = true;
	nsp->prevcpdef = psp->prevcpdef;
	SplineFree(sp->prev);
	SplineFree(sp->next);
	SplinePointFree(sp);
	SplinePointFree(psp);
	SplineRefigure(nsp->prev);
    }
    if ( base->first==sp || base->first==psp )
	base->first = nsp;
    if ( base->last==sp || base->last==psp )
	base->last = nsp;
}

static SplinePoint *SameLine(SplinePoint *psp, SplinePoint *sp, SplinePoint *nsp) {
    BasePoint noff, poff;
    real nlen, plen, normal;

    noff.x = nsp->me.x-sp->me.x; noff.y = nsp->me.y-sp->me.y;
    poff.x = psp->me.x-sp->me.x; poff.y = psp->me.y-sp->me.y;
    nlen = esqrt(noff.x*noff.x + noff.y*noff.y);
    plen = esqrt(poff.x*poff.x + poff.y*poff.y);
    if ( nlen==0 )
return( nsp );
    if ( plen==0 )
return( psp );
    normal = (noff.x*poff.y - noff.y*poff.x)/nlen/plen;
    if ( normal<-.0001 || normal>.0001 )
return( NULL );

    if ( noff.x*poff.x < 0 || noff.y*poff.y < 0 )
return( NULL );		/* Same line, but different directions */
    if ( (noff.x>0 && noff.x>poff.x) ||
	    (noff.x<0 && noff.x<poff.x) ||
	    (noff.y>0 && noff.y>poff.y) ||
	    (noff.y<0 && noff.y<poff.y))
return( nsp );

return( psp );
}

static double AdjacentSplinesMatch(Spline *s1,Spline *s2,int s2forward) {
    /* Is every point on s2 close to a point on s1 */
    double t, tdiff, t1 = -1;
    double xoff, yoff;
    double t1start, t1end;
    extended ts[2];
    int i;

    if ( (xoff = s2->to->me.x-s2->from->me.x)<0 ) xoff = -xoff;
    if ( (yoff = s2->to->me.y-s2->from->me.y)<0 ) yoff = -yoff;
    if ( xoff>yoff )
	SplineFindExtrema(&s1->splines[0],&ts[0],&ts[1]);
    else
	SplineFindExtrema(&s1->splines[1],&ts[0],&ts[1]);
    if ( s2forward ) {
	t = 0;
	tdiff = 1/16.0;
	t1end = 1;
	for ( i=1; i>=0 && ts[i]==-1; --i );
	t1start = i<0 ? 0 : ts[i];
    } else {
	t = 1;
	tdiff = -1/16.0;
	t1start = 0;
	t1end = ( ts[0]==-1 ) ? 1.0 : ts[0];
    }

    for ( ; (s2forward && t<=1) || (!s2forward && t>=0 ); t += tdiff ) {
	double x1, y1, xo, yo;
	double x = ((s2->splines[0].a*t+s2->splines[0].b)*t+s2->splines[0].c)*t+s2->splines[0].d;
	double y = ((s2->splines[1].a*t+s2->splines[1].b)*t+s2->splines[1].c)*t+s2->splines[1].d;
	if ( xoff>yoff )
	    t1 = IterateSplineSolve(&s1->splines[0],t1start,t1end,x,.001);
	else
	    t1 = IterateSplineSolve(&s1->splines[1],t1start,t1end,y,.001);
	if ( t1<0 || t1>1 )
return( -1 );
	x1 = ((s1->splines[0].a*t1+s1->splines[0].b)*t1+s1->splines[0].c)*t1+s1->splines[0].d;
	y1 = ((s1->splines[1].a*t1+s1->splines[1].b)*t1+s1->splines[1].c)*t1+s1->splines[1].d;
	if ( (xo = (x-x1))<0 ) xo = -xo;
	if ( (yo = (y-y1))<0 ) yo = -yo;
	if ( xo+yo>.5 )
return( -1 );
    }
return( t1 );
}
	
void SSRemoveBacktracks(SplineSet *ss) {
    SplinePoint *sp;

    if ( ss==NULL )
return;
    for ( sp=ss->first; ; ) {
	if ( sp->next!=NULL && sp->prev!=NULL ) {
	    SplinePoint *nsp = sp->next->to, *psp = sp->prev->from, *isp;
	    BasePoint ndir, pdir;
	    double dot, pdot, nlen, plen, t = -1;

	    ndir.x = (nsp->me.x - sp->me.x); ndir.y = (nsp->me.y - sp->me.y);
	    pdir.x = (psp->me.x - sp->me.x); pdir.y = (psp->me.y - sp->me.y);
	    nlen = ndir.x*ndir.x + ndir.y*ndir.y; plen = pdir.x*pdir.x + pdir.y*pdir.y;
	    dot = ndir.x*pdir.x + ndir.y*pdir.y;
	    if ( (pdot = ndir.x*pdir.y - ndir.y*pdir.x)<0 ) pdot = -pdot;
	    if ( dot>0 && dot>pdot ) {
		if ( nlen>plen && (t=AdjacentSplinesMatch(sp->next,sp->prev,false))!=-1 ) {
		    isp = SplineBisect(sp->next,t);
		    psp->nextcp.x = psp->me.x + (isp->nextcp.x-isp->me.x);
		    psp->nextcp.y = psp->me.y + (isp->nextcp.y-isp->me.y);
		    psp->nonextcp = isp->nonextcp;
		    psp->next = isp->next;
		    isp->next->from = psp;
		    SplineFree(isp->prev);
		    SplineFree(sp->prev);
		    SplinePointFree(isp);
		    SplinePointFree(sp);
		    if ( psp->next->order2 ) {
			psp->nextcp.x = nsp->prevcp.x = (psp->nextcp.x+nsp->prevcp.x)/2;
			psp->nextcp.y = nsp->prevcp.y = (psp->nextcp.y+nsp->prevcp.y)/2;
			if ( psp->nonextcp || nsp->noprevcp )
			    psp->nonextcp = nsp->noprevcp = true;
		    }
		    SplineRefigure(psp->next);
		    if ( ss->first==sp )
			ss->first = psp;
		    if ( ss->last==sp )
			ss->last = psp;
		    sp=psp;
		} else if ( nlen<plen && (t=AdjacentSplinesMatch(sp->prev,sp->next,true))!=-1 ) {
		    isp = SplineBisect(sp->prev,t);
		    nsp->prevcp.x = nsp->me.x + (isp->prevcp.x-isp->me.x);
		    nsp->prevcp.y = nsp->me.y + (isp->prevcp.y-isp->me.y);
		    nsp->noprevcp = isp->noprevcp;
		    nsp->prev = isp->prev;
		    isp->prev->to = nsp;
		    SplineFree(isp->next);
		    SplineFree(sp->next);
		    SplinePointFree(isp);
		    SplinePointFree(sp);
		    if ( psp->next->order2 ) {
			psp->nextcp.x = nsp->prevcp.x = (psp->nextcp.x+nsp->prevcp.x)/2;
			psp->nextcp.y = nsp->prevcp.y = (psp->nextcp.y+nsp->prevcp.y)/2;
			if ( psp->nonextcp || nsp->noprevcp )
			    psp->nonextcp = nsp->noprevcp = true;
		    }
		    SplineRefigure(nsp->prev);
		    if ( ss->first==sp )
			ss->first = psp;
		    if ( ss->last==sp )
			ss->last = psp;
		    sp=psp;
		}
	    }
	}
	if ( sp->next==NULL )
    break;
	sp=sp->next->to;
	if ( sp==ss->first )
    break;
    }
}

/* If we have a contour with no width, say a line from A to B and then from B */
/*  to A, then it will be ambiguous, depending on how we hit the contour, as  */
/*  to whether it is needed or not. Which will cause us to complain. Since    */
/*  they contain no area, they achieve nothing, so we might as well say they  */
/*  overlap themselves and remove them here */
static SplineSet *SSRemoveReversals(SplineSet *base) {
    SplineSet *head = base, *prev=NULL, *next;
    SplinePoint *sp;
    int changed;

    while ( base!=NULL ) {
	next = base->next;
	changed = true;
	while ( changed ) {
	    changed = false;
	    if ( base->first->next==NULL ||
		    (base->first->next->to==base->first &&
		     base->first->nextcp.x==base->first->prevcp.x &&
		     base->first->nextcp.y==base->first->prevcp.y)) {
		/* remove single points */
		if ( prev==NULL )
		    head = next;
		else
		    prev->next = next;
		base->next = NULL;
		SplinePointListFree(base);
		base = prev;
	break;
	    }
	    for ( sp=base->first; ; ) {
		if ( sp->next!=NULL && sp->prev!=NULL &&
			sp->nextcp.x==sp->prevcp.x && sp->nextcp.y==sp->prevcp.y ) {
		    SplinePoint *nsp = sp->next->to, *psp = sp->prev->from, *isp;
		    if ( psp->me.x==nsp->me.x && psp->me.y==nsp->me.y &&
			    psp->nextcp.x==nsp->prevcp.x && psp->nextcp.y==nsp->prevcp.y ) {
			/* We wish to remove sp, sp->next, sp->prev & one of nsp/psp */
			RemoveNextSP(psp,sp,nsp,base);
			changed = true;
	    break;
		    } else if ( sp->nonextcp /* which implies sp->noprevcp */ &&
			    psp->nonextcp && nsp->noprevcp &&
			    (isp = SameLine(psp,sp,nsp))!=NULL ) {
			/* We have a line that backtracks, but doesn't cover */
			/*  the entire spline, so we intersect */
			/* We want to remove sp, the shorter of sp->next, sp->prev */
			/*  and a bit of the other one. Also reomve one of nsp,psp */
			if ( isp==psp ) {
			    RemoveNextSP(psp,sp,nsp,base);
			    psp->nextcp = psp->me;
			    psp->nonextcp = true;
			} else {
			    RemovePrevSP(psp,sp,nsp,base);
			}
			changed = true;
	    break;
		    }
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==base->first )
	    break;
	    }
	}
	SSRemoveBacktracks(base);
	prev = base;
	base = next;
    }
return( head );
}

SplineSet *SplineSetRemoveOverlap(SplineChar *sc, SplineSet *base,enum overlap_type ot) {
    Monotonic *ms;
    Intersection *ilist;
    SplineSet *ret;
    SplineSet *order3 = NULL;
    int is_o2 = false;
    SplineSet *ss;

    for ( ss=base; ss!=NULL; ss=ss->next )
	if ( ss->first->next!=NULL ) {
	    is_o2 = ss->first->next->order2;
    break;
	}
    if ( is_o2 ) {
	order3 = SplineSetsPSApprox(base);
	SplinePointListsFree(base);
	base = order3;
    }

    if ( sc!=NULL )
	glyphname = sc->name;

    base = SSRemoveTiny(base);
    SSRemoveStupidControlPoints(base);
    SplineSetsRemoveAnnoyingExtrema(base,.3);
    SSOverlapClusterCpAngles(base,.01);
    base = SSRemoveReversals(base);
    ms = SSsToMContours(base,ot);
    ilist = FindIntersections(ms,ot);
    Validate(ms,ilist);
    if ( ot==over_findinter || ot==over_fisel ) {
	FixupIntersectedSplines(ms);
	ret = base;
    } else {
	FindNeeded(ms,ot);
	FindUnitVectors(ilist);
	if ( ot==over_remove || ot == over_rmselected )
	    TestForBadDirections(ilist);
	ret = JoinAllNeeded(ilist);
	ret = MergeOpenAndFreeClosed(ret,base,ot);
    }
    FreeMonotonics(ms);
    FreeIntersections(ilist);
    glyphname = NULL;
    if ( order3!=NULL ) {
	ss = SplineSetsTTFApprox(ret);
	SplinePointListsFree(ret);
	ret = ss;
    }
return( ret );
}
