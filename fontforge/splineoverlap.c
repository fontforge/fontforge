/* Copyright (C) 2000-2010 by George Williams */
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
	fprintf(stderr, "Internal Error (overlap): " );
    else
	fprintf(stderr, "Internal Error (overlap) in %s: ", glyphname );
    vfprintf(stderr,format,ap);
}

#if defined( FONTFORGE_CONFIG_USE_DOUBLE ) || defined( FONTFORGE_CONFIG_USE_LONGDOUBLE )
# define RE_NearZero	.00001
# define RE_Factor	(1024.0*1024.0*1024.0*1024.0*1024.0*2.0) /* 52 bits => divide by 2^51 */
#else
# define RE_NearZero	.001
# define RE_Factor	(1024.0*1024.0*4.0)	/* 23 bits => divide by 2^22 */
#endif

static int Within4RoundingErrors(double v1, double v2) {
    double temp=v1*v2;
    double re;

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

static int Within16RoundingErrors(double v1, double v2) {
    double temp=v1*v2;
    double re;

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
	*_t1 = t1; *_t2 = t2;
	if ( error<1e-11 )	/* Error is actually the square of the error */
    break;			/* So this isn't as constraining as it looks */

	gt1 = Grad1(&s1->splines[0],&s2->splines[0],t1,t2) + Grad1(&s1->splines[1],&s2->splines[1],t1,t2);
	gt2 = Grad1(&s2->splines[0],&s1->splines[0],t2,t1) + Grad1(&s2->splines[1],&s1->splines[1],t2,t1);
	glen = esqrt(gt1*gt1 + gt2*gt2) * factor;
	if ( glen==0 )
    break;
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

static Intersection *_AddIntersection(Intersection *ilist,Monotonic *m1,
	Monotonic *m2,extended t1,extended t2,BasePoint *inter) {
    Intersection *il, *closest=NULL;
    double dist, dx, dy, bestd=9e10;

    for ( il = ilist; il!=NULL; il=il->next ) {
	if ( RealWithin(il->inter.x,inter->x,.01) && RealWithin(il->inter.y,inter->y,.01)) {
	    if ( (dx = il->inter.x-inter->x)<0 ) dx = -dx;
	    if ( (dy = il->inter.y-inter->y)<0 ) dy = -dy;
	    dist = dx+dy;
	    if ( dist<bestd ) {
		bestd = dist;
		closest = il;
		if ( dist==0 )
    break;
	    }
	}
    }

    if ( closest==NULL ) {
	closest = chunkalloc(sizeof(Intersection));
	closest->inter = *inter;
	closest->next = ilist;
    }
    AddSpline(closest,m1,t1);
    AddSpline(closest,m2,t2);
return( closest );
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

return( _AddIntersection(ilist,m1,m2,t1,t2,inter));
}

static Intersection *FindMonotonicIntersection(Intersection *ilist,Monotonic *m1,Monotonic *m2) {
    /* Note that two monotonic cubics can still intersect in multiple points */
    /*  so we can't just check if the splines are on opposite sides of each */
    /*  other at top and bottom */
    DBounds b;
    const double error = .00001;
    BasePoint pt;
    extended t1,t2;
    extended t1end = m1->tend, t2end = m2->tend;

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
	    t1 = IterateSplineSolve(&m1->s->splines[0],m1->tstart,m1->tend,b.minx);
	else
	    t1 = IterateSplineSolve(&m1->s->splines[1],m1->tstart,m1->tend,b.miny);
	if ( m2->b.maxx-m2->b.minx > m2->b.maxy-m2->b.miny )
	    t2 = IterateSplineSolve(&m2->s->splines[0],m2->tstart,m2->tend,b.minx);
	else
	    t2 = IterateSplineSolve(&m2->s->splines[1],m2->tstart,m2->tend,b.miny);
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
	t1 = IterateSplineSolve(&m1->s->splines[1],m1->tstart,m1->tend,b.miny);
	t2 = IterateSplineSolve(&m2->s->splines[1],m2->tstart,m2->tend,b.miny);
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
	t1 = IterateSplineSolve(&m1->s->splines[0],m1->tstart,m1->tend,b.minx);
	t2 = IterateSplineSolve(&m2->s->splines[0],m2->tstart,m2->tend,b.minx);
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
	    t1o = IterateSplineSolve(&m1->s->splines[1],m1->tstart,m1->tend,y);
	    if ( t1o==-1 )
		t1o = IterateSplineSolve(&m1->s->splines[1],m1->tstart-m1->tstart/32,m1->tend+m1->tend/32,y);
	    t2o = IterateSplineSolve(&m2->s->splines[1],m2->tstart,m2->tend,y);
	    if ( t2o==-1 )
		t2o = IterateSplineSolve(&m2->s->splines[1],m2->tstart-m2->tstart/32,m2->tend+m2->tend/32,y);
	    if ( t1o!=-1 && t2o!=-1 )
	break;
	    y += diff;
	}
	x1o = ((m1->s->splines[0].a*t1o+m1->s->splines[0].b)*t1o+m1->s->splines[0].c)*t1o+m1->s->splines[0].d;
	x2o = ((m2->s->splines[0].a*t2o+m2->s->splines[0].b)*t2o+m2->s->splines[0].c)*t2o+m2->s->splines[0].d;
	if ( x1o==x2o ) {	/* Unlikely... but just in case */
	    pt.x = x1o; pt.y = y;
	    ilist = AddIntersection(ilist,m1,m2,t1o,t2o,&pt);
	    /* If pt is not one of the end points then AddIntersection will*/
	    /*  split the monotonic in two at that point. m1/m2 will be the*/
	    /*  section of the the monotonic with lower t values. We need  */
	    /*  to keep testing the section with higher y values. So if yup*/
	    /*  then m1/m2 have lower y values and m?->next will be what we*/
	    /*  need */ /* Unless pt is the start point. Then there will be*/
	    /*  no new monotonic and m?->tend won't have changed */
	    if ( m1->yup && m1->tend<t1end ) m1 = m1->next;
	    if ( m2->yup && m2->tend<t2end ) m2 = m2->next;
	}
	for ( y+=diff; ; y += diff ) {
	    /* I used to say y<=b.maxy in the above for statement. */
	    /*  that seemed to get rounding errors on the mac, so we do it */
	    /*  like this now: */
	    if ( y>b.maxy ) {
		if ( y<b.maxy+(63*diff/64) ) y = b.maxy;
		else
	break;
	    }
	    t1 = IterateSplineSolve(&m1->s->splines[1],m1->tstart,m1->tend,y);
	    if ( t1==-1 )
		t1 = IterateSplineSolve(&m1->s->splines[1],m1->tstart-m1->tstart/32,m1->tend+m1->tend/32,y);
	    t2 = IterateSplineSolve(&m2->s->splines[1],m2->tstart,m2->tend,y);
	    if ( t2==-1 )
		t2 = IterateSplineSolve(&m2->s->splines[1],m2->tstart-m2->tstart/32,m2->tend+m2->tend/32,y);
	    if ( t1==-1 || t2==-1 )
	continue;
	    x1 = ((m1->s->splines[0].a*t1+m1->s->splines[0].b)*t1+m1->s->splines[0].c)*t1+m1->s->splines[0].d;
	    x2 = ((m2->s->splines[0].a*t2+m2->s->splines[0].b)*t2+m2->s->splines[0].c)*t2+m2->s->splines[0].d;
	    if ( x1==x2 && x1o!=x2o ) {
		pt.x = x1; pt.y = y;
		ilist = AddIntersection(ilist,m1,m2,t1,t2,&pt);
		/* see comment above for these next lines */
		if ( m1->yup && m1->tend<t1end ) m1 = m1->next;
		if ( m2->yup && m2->tend<t2end ) m2 = m2->next;
		x1o = x1; x2o = x2;
	    } else if ( x1o!=x2o && (x1o>x2o) != ( x1>x2 ) ) {
		/* A cross over has occured. (assume we have a small enough */
		/*  region that three cross-overs can't have occurred) */
		/* Use a binary search to track it down */
		extended ytop, ybot, ytest, oldy;
		oldy = ytop = y;
		ybot = y-diff;
		x1o = x1; x2o = x2;
		while ( ytop!=ybot ) {
		    extended t1t, t2t;
		    ytest = (ytop+ybot)/2;
		    t1t = IterateSplineSolve(&m1->s->splines[1],m1->tstart,m1->tend,ytest);
		    t2t = IterateSplineSolve(&m2->s->splines[1],m2->tstart,m2->tend,ytest);
		    x1 = ((m1->s->splines[0].a*t1t+m1->s->splines[0].b)*t1t+m1->s->splines[0].c)*t1t+m1->s->splines[0].d;
		    x2 = ((m2->s->splines[0].a*t2t+m2->s->splines[0].b)*t2t+m2->s->splines[0].c)*t2t+m2->s->splines[0].d;
		    if ( t1t==-1 || t2t==-1 ) {
			if ( t1t==-1 && (RealNear(ytest,m1->b.miny) || RealNear(ytest,m1->b.maxy)))
			    /* OK */;
			else if ( t2t==-1 && (RealNear(ytest,m2->b.miny) || RealNear(ytest,m2->b.maxy)))
			    /* OK */;
			else
			    SOError( "Can't find something in range. y=%g\n", ytest );
		break;
		    } else if (( x1-x2<error && x1-x2>-error ) || ytop==ytest || ybot==ytest ) {
			pt.y = ytest; pt.x = (x1+x2)/2;
			ilist = AddIntersection(ilist,m1,m2,t1t,t2t,&pt);
			/* see comment above for these next lines */
			if ( m1->yup && m1->tend<t1end ) m1 = m1->next;
			if ( m2->yup && m2->tend<t2end ) m2 = m2->next;
		break;
		    } else if ( (x1o>x2o) != ( x1>x2 ) ) {
			ybot = ytest;
		    } else {
			ytop = ytest;
		    }
		}
		y = oldy;		/* Might be more than one intersection, keep going */
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
	    t1o = IterateSplineSolve(&m1->s->splines[0],m1->tstart,m1->tend,x);
	    if ( t1o==-1 )
		t1o = IterateSplineSolve(&m1->s->splines[0],m1->tstart-m1->tstart/32,m1->tend+m1->tend/32,x);
	    t2o = IterateSplineSolve(&m2->s->splines[0],m2->tstart,m2->tend,x);
	    if ( t2o==-1 )
		t2o = IterateSplineSolve(&m2->s->splines[0],m2->tstart-m2->tstart/32,m2->tend+m2->tend/32,x);
	    if ( t1o!=-1 && t2o!=-1 )
	break;
	    x += diff;
	}
	y1o = ((m1->s->splines[1].a*t1o+m1->s->splines[1].b)*t1o+m1->s->splines[1].c)*t1o+m1->s->splines[1].d;
	y2o = ((m2->s->splines[1].a*t2o+m2->s->splines[1].b)*t2o+m2->s->splines[1].c)*t2o+m2->s->splines[1].d;
	if ( y1o==y2o ) {
	    pt.y = y1o; pt.x = x;
	    ilist = AddIntersection(ilist,m1,m2,t1o,t2o,&pt);
	    /* If pt is not one of the end points then AddIntersection will*/
	    /*  split the monotonic in two at that point. m1/m2 will be the*/
	    /*  section of the the monotonic with lower t values. We need  */
	    /*  to keep testing the section with higher x values. So if xup*/
	    /*  then m1/m2 have lower x values and m?->next will be what we*/
	    /*  need */ /* Unless pt is the start point. Then there will be*/
	    /*  no new monotonic and m?->tend won't have changed */
	    if ( m1->xup && m1->tend<t1end ) m1 = m1->next;
	    if ( m2->xup && m2->tend<t2end ) m2 = m2->next;
	}
	y1 = y2 = 0;
	for ( x+=diff; ; x += diff ) {
	    if ( x>b.maxx ) {
		if ( x<b.maxx+(63*diff/64) ) x = b.maxx;
		else
	break;
	    }
	    t1 = IterateSplineSolve(&m1->s->splines[0],m1->tstart,m1->tend,x);
	    if ( t1==-1 )
		t1 = IterateSplineSolve(&m1->s->splines[0],m1->tstart-m1->tstart/32,m1->tend+m1->tend/32,x);
	    t2 = IterateSplineSolve(&m2->s->splines[0],m2->tstart,m2->tend,x);
	    if ( t2==-1 )
		t2 = IterateSplineSolve(&m2->s->splines[0],m2->tstart-m2->tstart/32,m2->tend+m2->tend/32,x);
	    if ( t1==-1 || t2==-1 )
	continue;
	    y1 = ((m1->s->splines[1].a*t1+m1->s->splines[1].b)*t1+m1->s->splines[1].c)*t1+m1->s->splines[1].d;
	    y2 = ((m2->s->splines[1].a*t2+m2->s->splines[1].b)*t2+m2->s->splines[1].c)*t2+m2->s->splines[1].d;
	    if ( y1==y2 && y1o!=y2o ) {
		pt.y = y1; pt.x = x;
		ilist = AddIntersection(ilist,m1,m2,t1,t2,&pt);
		/* see comment above */
		if ( m1->xup && m1->tend<t1end ) m1 = m1->next;
		if ( m2->xup && m2->tend<t2end ) m2 = m2->next;
		y1o = y1; y2o = y2;
	    } else if ( y1o!=y2o && (y1o>y2o) != ( y1>y2 ) ) {
		/* A cross over has occured. (assume we have a small enough */
		/*  region that three cross-overs can't have occurred) */
		/* Use a binary search to track it down */
		extended xtop, xbot, xtest, oldx;
		oldx = xtop = x;
		xbot = x-diff;
		y1o = y1; y2o = y2;
		while ( xtop!=xbot ) {
		    extended t1t, t2t;
		    xtest = (xtop+xbot)/2;
		    t1t = IterateSplineSolve(&m1->s->splines[0],m1->tstart,m1->tend,xtest);
		    t2t = IterateSplineSolve(&m2->s->splines[0],m2->tstart,m2->tend,xtest);
		    y1 = ((m1->s->splines[1].a*t1t+m1->s->splines[1].b)*t1t+m1->s->splines[1].c)*t1t+m1->s->splines[1].d;
		    y2 = ((m2->s->splines[1].a*t2t+m2->s->splines[1].b)*t2t+m2->s->splines[1].c)*t2t+m2->s->splines[1].d;
		    if ( t1t==-1 || t2t==-1 ) {
			if ( t1t==-1 && (RealNear(xtest,m1->b.minx) || RealNear(xtest,m1->b.maxx)))
			    /* OK */;
			else if ( t2t==-1 && (RealNear(xtest,m2->b.minx) || RealNear(xtest,m2->b.maxx)))
			    /* OK */;
			else
			    SOError( "Can't find something in range. x=%g\n", (double) xtest );
		break;
		    } else if (( y1-y2<error && y1-y2>-error ) || xtop==xtest || xbot==xtest ) {
			pt.x = xtest; pt.y = (y1+y2)/2;
			ilist = AddIntersection(ilist,m1,m2,t1t,t2t,&pt);
			/* see comment above */
			if ( m1->xup && m1->tend<t1end ) m1 = m1->next;
			if ( m2->xup && m2->tend<t2end ) m2 = m2->next;
		break;
		    } else if ( (y1o>y2o) != ( y1>y2 ) ) {
			xbot = xtest;
		    } else {
			xtop = xtest;
		    }
		}
		x = oldx;
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

    which = ( m->b.maxx-m->b.minx > m->b.maxy-m->b.miny )? 0 : 1;
    nw = !which;
    t = IterateSplineSolve(&m->s->splines[which],m->tstart,m->tend,(&pt->x)[which]);
    if ( (slope.x = (3*m->s->splines[0].a*t+2*m->s->splines[0].b)*t+m->s->splines[0].c)<0 )
	slope.x = -slope.x;
    if ( (slope.y = (3*m->s->splines[1].a*t+2*m->s->splines[1].b)*t+m->s->splines[1].c)<0 )
	slope.y = -slope.y;
    if ( t==-1 || (slope.y>slope.x)!=which ) {
	nw = which;
	which = 1-which;
	t = IterateSplineSolve(&m->s->splines[which],m->tstart,m->tend,(&pt->x)[which]);
    }
    if ( t!=-1 && Within4RoundingErrors((&pt->x)[nw],
	   ((m->s->splines[nw].a*t+m->s->splines[nw].b)*t +
		m->s->splines[nw].c)*t + m->s->splines[nw].d ))
return( t );

return( -1 );
}

/* If two splines are coincident, then pretend they intersect at both */
/*  end-points and nowhere else */
static int CoincidentIntersect(Monotonic *m1,Monotonic *m2,BasePoint *pts,
	extended *t1s,extended *t2s) {
    int cnt=0;
    extended t, t2, diff;

    if ( m1==m2 )
return( false );		/* Can't be coincident. Adjacent */
    /* Adjacent splines can double back on themselves */
    if ( m1->next==m2 || m1->prev==m2 ) {
	/* But normally they'll only intersect in one point, where they join */
	/* and that doesn't count */
	if ( (m1->b.minx>m2->b.minx ? m1->b.minx : m2->b.minx) == (m1->b.maxx<m2->b.maxx ? m1->b.maxx : m2->b.maxx) &&
		(m1->b.miny>m2->b.miny ? m1->b.miny : m2->b.miny) == (m1->b.maxy<m2->b.maxy ? m1->b.maxy : m2->b.maxy) )
return( false );
    }

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
		t2 = IterateSplineSolve(&m2->s->splines[1],t2s[0],t2s[1],here.y);
		if ( t2==-1 || !RealWithin(here.x,((m2->s->splines[0].a*t2+m2->s->splines[0].b)*t2+m2->s->splines[0].c)*t2+m2->s->splines[0].d,.1))
return( false );
	    } else {
		t2 = IterateSplineSolve(&m2->s->splines[0],t2s[0],t2s[1],here.x);
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
	    /* skip to next contour */
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
	    t = IterateSplineSolve(&m->s->splines[which],m->tstart,m->tend,test);
	    if ( t==-1 ) {
		if ( which==0 ) {
		    if (( test-m->b.minx > m->b.maxx-test && m->xup ) ||
			    ( test-m->b.minx < m->b.maxx-test && !m->xup ))
			t = m->tstart;
		    else
			t = m->tend;
		} else {
		    if (( test-m->b.miny > m->b.maxy-test && m->yup ) ||
			    ( test-m->b.miny < m->b.maxy-test && !m->yup ))
			t = m->tstart;
		    else
			t = m->tend;
		}
	    }
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

static void ILReplaceMono(Intersection *il,Monotonic *m,Monotonic *otherm) {
    MList *ml;

    for ( ml=il->monos; ml!=NULL; ml=ml->next ) {
	if ( ml->m==m ) {
	    ml->m = otherm;
    break;
	}
    }
}

struct inter_data {
    Monotonic *m, *otherm;
    double t, othert;
    BasePoint inter;
    int new;
};

static void SplitMonotonicAt(Monotonic *m,int which,double coord,
	struct inter_data *id) {
    Monotonic *otherm = NULL;
    double t;
    double othert;
    int low=0, high=0;
    double cx,cy;
    Spline1D *sx, *sy;

    if (( which==0 && coord==m->b.minx ) || (which==1 && coord==m->b.miny))
	low = true;
    else if ( (which==0 && coord==m->b.maxx) || (which==1 && coord==m->b.maxy) )
	high = true;

    if ( low || high ) {
	if ( (low && (&m->xup)[which]) || (high && !(&m->xup)[which]) ) {
	    t = m->tstart;
	    otherm = m->prev;
	    othert = m->prev->tend;
	} else if ( (low && !(&m->xup)[which]) || (high && (&m->xup)[which]) ) {
	    t = m->tend;
	    otherm = m->next;
	    othert = m->next->tstart;
	}
	sx = &m->s->splines[0]; sy = &m->s->splines[1];
	cx = ((sx->a*t+sx->b)*t+sx->c)*t+sx->d;
	cy = ((sy->a*t+sy->b)*t+sy->c)*t+sy->d;
	if ( which ) cy = coord; else cx = coord;	/* Correct for rounding errors */
	id->new = false;
    } else {
	t = IterateSplineSolve(&m->s->splines[which],m->tstart,m->tend,coord);
#if 0
	if ( t==-1 && m->next->s==m->s &&
		(t = IterateSplineSolve(&m->next->s->splines[which],m->next->tstart,m->next->tend,coord))!=-1 )
	    m=m->next;
	else if ( t==-1 && m->prev->s==m->s &&
		(t = IterateSplineSolve(&m->prev->s->splines[which],m->prev->tstart,m->prev->tend,coord))!=-1 )
	    m=m->prev;
#endif
	if ( t==-1 )
	    SOError("Intersection failed!\n");
	othert = t;
	otherm = chunkalloc(sizeof(Monotonic));
	*otherm = *m;
	m->next = otherm;
	m->linked = otherm;
	otherm->prev = m;
	otherm->next->prev = otherm;
	m->tend = t;
	if ( otherm->end!=NULL ) {
	    m->end = NULL;
	    ILReplaceMono(otherm->end,m,otherm);
	}
	otherm->tstart = t; otherm->start = NULL;
	sx = &m->s->splines[0]; sy = &m->s->splines[1];
	cx = ((sx->a*t+sx->b)*t+sx->c)*t+sx->d;
	cy = ((sy->a*t+sy->b)*t+sy->c)*t+sy->d;
	if ( which ) cy = coord; else cx = coord;	/* Correct for rounding errors */
	if ( m->xup ) {
	    m->b.maxx = otherm->b.minx = cx;
	} else {
	    m->b.minx = otherm->b.maxx = cx;
	}
	if ( m->yup ) {
	    m->b.maxy = otherm->b.miny = cy;
	} else {
	    m->b.miny = otherm->b.maxy = cy;
	}
	id->new = true;
    }
    id->m = m; id->otherm = otherm;
    id->t = t; id->othert = othert;
    id->inter.x = cx; id->inter.y = cy;
}

static Intersection *SplitMonotonicsAt(Monotonic *m1,Monotonic *m2,
	int which,double coord,Intersection *ilist) {
    struct inter_data id1, id2;
    Intersection *check;

    SplitMonotonicAt(m1,which,coord,&id1);
    SplitMonotonicAt(m2,which,coord,&id2);
    if ( !id1.new && !id2.new )
return( ilist );
    ilist = check = _AddIntersection(ilist,id1.m,id1.otherm,id1.t,id1.othert,&id2.inter);
    ilist = _AddIntersection(ilist,id2.m,id2.otherm,id2.t,id2.othert,&id2.inter);	/* Use id1.inter to avoid rounding errors */
    if ( check!=ilist )
	IError("Added too many intersections.");
return( ilist );
}

static Intersection *TryHarderWhenClose(int which, double tried_value, Monotonic **space,int cnt,
	Intersection *ilist) {
    /* If splines are very close together at a certain point then we can't */
    /*  tell the proper ordering due to rounding errors. */
    int i, j;
    double low, high, test, t1, t2, c1, c2, incr;
    int neg_cnt, pos_cnt, pc, nc;
    int other = !which;

    for ( i=cnt-2; i>=0; --i ) {
	Monotonic *m1 = space[i], *m2 = space[i+1];
	if ( Within4RoundingErrors( m1->other,m2->other )) {
	    /* Now, we know that these two monotonics do not intersect */
	    /*  (except possibly at the end points, because we found all */
	    /*  intersections earlier) so we can compare them anywhere */
	    /*  along their mutual span, and any ordering (not encumbered */
	    /*  by rounding errors) should be valid */
	    if ( which==0 ) {
		low = m1->b.minx>m2->b.minx ? m1->b.minx : m2->b.minx;
		high = m1->b.maxx<m2->b.maxx ? m1->b.maxx : m2->b.maxx;
	    } else {
		low = m1->b.miny>m2->b.miny ? m1->b.miny : m2->b.miny;
		high = m1->b.maxy<m2->b.maxy ? m1->b.maxy : m2->b.maxy;
	    }
#define DECIMATE	32
	    incr = (high-low)/DECIMATE;
	    neg_cnt = pos_cnt=0;
	    pc = nc = 0;
	    for ( j=0, test=low+incr; j<=DECIMATE; ++j, test += incr ) {
		if ( test>high ) test=high;
#undef DECIMATE
		t1 = IterateSplineSolve(&m1->s->splines[which],m1->tstart,m1->tend,test);
		t2 = IterateSplineSolve(&m2->s->splines[which],m2->tstart,m2->tend,test);
		if ( t1==-1 || t2==-1 )
	    continue;
		c1 = ((m1->s->splines[other].a*t1+m1->s->splines[other].b)*t1+m1->s->splines[other].c)*t1+m1->s->splines[other].d;
		c2 = ((m2->s->splines[other].a*t2+m2->s->splines[other].b)*t2+m2->s->splines[other].c)*t2+m2->s->splines[other].d;
		if ( !Within16RoundingErrors(c1,c2)) {
		    if ( c1>c2 ) { pos_cnt=1; neg_cnt=0; }
		    else { pos_cnt=0; neg_cnt=1; }
	    break;
		} else if ( !Within4RoundingErrors(c1,c2) ) {
		    if ( c1>c2 )
			++pos_cnt;
		    else
			++neg_cnt;
		} else {
		    /* Here the diff might be 0, which doesn't count as either +/- */
		    /*  earlier diff was bigger than error so that couldn't happen */
		    if ( c1>c2 )
			++pc;
		    else if ( c1!=c2 )
			++nc;
		}
	    }
	    if ( pos_cnt>neg_cnt ) {
		/* Out of order */
		space[i+1] = m1;
		space[i] = m2;
	    } else if ( pos_cnt==0 && neg_cnt==0 ) {
		/* the two monotonics are never far from one another over */
		/*  this range. So let's add intersections at the end of */
		/*  the range so we don't get confused */
		if ( ilist!=NULL ) {
		    real rh = (real) high;
		    if ( (which==0 && (m1->b.minx!=m2->b.minx || m1->b.maxx!=m2->b.maxx)) ||
			    (which==1 && (m1->b.miny!=m2->b.miny || m1->b.maxy!=m2->b.maxy)) ) {
			ilist = SplitMonotonicsAt(m1,m2,which,low,ilist);
			if ( (which==0 && rh>m1->b.maxx && rh<=m1->next->b.maxx) ||
				(which==1 && rh>m1->b.maxy && rh<=m1->next->b.maxy))
			    m1 = m1->next;
			if ( (which==0 && rh>m2->b.maxx && rh<=m2->next->b.maxx) ||
				(which==1 && rh>m2->b.maxy && rh<=m2->next->b.maxy))
			    m2 = m2->next;
			ilist = SplitMonotonicsAt(m1,m2,which,high,ilist);
		    }
		    if ( (&m1->xup)[which]!=(&m2->xup)[which] ) {
			/* the two monotonics cancel each other out */
			/* (they are close together, and go in opposite directions) */
			if ( (which==0 && rh<m1->b.maxx && Within4RoundingErrors(rh,m1->next->b.maxx)) ||
				(which==1 && rh<m1->b.maxy && Within4RoundingErrors(rh,m1->next->b.maxy)))
			    m1 = m1->next;
			if ( (which==0 && rh<m2->b.maxx && Within4RoundingErrors(rh,m2->next->b.maxx)) ||
				(which==1 && rh<m2->b.maxy && Within4RoundingErrors(rh,m2->next->b.maxy)))
			    m2 = m2->next;
			m1->mutual_collapse = m1->isunneeded = true;
			m2->mutual_collapse = m2->isunneeded = true;
		    }
		}
		if ( pc>nc ) {
		    space[i+1] = m1;
		    space[i] = m2;
		}
	    }
	}
    }
return( ilist );
}

static int IsNeeded(enum overlap_type ot,int winding, int nwinding, int ew, int new) {
    if ( ot==over_remove || ot==over_rmselected ) {
return( winding==0 || nwinding==0 );
    } else if ( ot==over_intersect || ot==over_intersel ) {
return( !( (winding>-2 && winding<2 && nwinding>-2 && nwinding<2) ||
		    ((winding<=-2 || winding>=2) && (nwinding<=-2 && nwinding>=2))));
    } else if ( ot == over_exclude ) {
return( !( (( winding==0 || nwinding==0 ) && ew==0 && new==0 ) ||
		    (winding!=0 && (( ew!=0 && new==0 ) || ( ew==0 && new!=0))) ));
    }
return( false );
}

static void FigureNeeds(Monotonic *ms,int which, extended test, Monotonic **space,
	enum overlap_type ot, double close_level) {
    /* Find all monotonic sections which intersect the line (x,y)[which] == test */
    /*  find the value of the other coord on that line */
    /*  Order them (by the other coord) */
    /*  then run along that line figuring out which monotonics are needed */
    int i, winding, ew, close, n;

    TryHarderWhenClose(which,test,space,MonotonicFindAt(ms,which,test,space),NULL);

    winding = 0; ew = 0;
    for ( i=0; space[i]!=NULL; ++i ) {
	int needed;
	Monotonic *m, *nm;
	int new;
	int nwinding, nnwinding, nneeded, nnew, niwinding, niew, nineeded, inneeded, inwinding, inew;
      /* retry: */
	needed = false;
	nwinding=winding;
	new=ew;
	m = space[i];
	if ( m->mutual_collapse )
    continue;
	n=0;
	do {
	    ++n;
	    nm = space[i+n];
	} while ( nm!=NULL && nm->mutual_collapse );
	if ( m->exclude )
	    new += ( (&m->xup)[which] ? 1 : -1 );
	else
	    nwinding += ( (&m->xup)[which] ? 1 : -1 );
	/* We do some look ahead and figure out the neededness of the next */
	/*  monotonic on the list. This is because we may need to reorder them*/
	/*  (if the two are close together we might get rounding errors). */
	/*  So not only do we figure out the neededness of both this and the */
	/*  next mono using the current ordering, but we also do it as things*/
	/*  would appear after reversing the two. So... */
	/* needed -- means the current mono is needed with the current order */
	/* nneeded -- next mono is needed with the current order */
	/* nineeded -- next mono is needed with reveresed order */
	/* inneeded -- cur mono is needed with reversed order */
	niwinding = winding; niew = ew;
	nnwinding = nwinding; nnew = new;
	if ( nm!=NULL ) {
	    if ( nm->exclude ) {
		nnew += ( (&nm->xup)[which] ? 1 : -1 );
		niew += ( (&nm->xup)[which] ? 1 : -1 );
	    } else {
		nnwinding += ( (&nm->xup)[which] ? 1 : -1 );
		niwinding += ( (&nm->xup)[which] ? 1 : -1 );
	    }
	}
	inwinding = niwinding; inew = niew;
	if ( m->exclude )
	    inew += ( (&m->xup)[which] ? 1 : -1 );
	else
	    inwinding += ( (&m->xup)[which] ? 1 : -1 );
	needed = IsNeeded(ot,winding,nwinding,ew,new);
	nneeded = IsNeeded(ot,nwinding,nnwinding,new,nnew);
	nineeded = IsNeeded(ot,winding,niwinding,ew,niew);
	inneeded = IsNeeded(ot,niwinding,inwinding,niew,inew);
	if ( nm!=NULL )
	    close = nm->other-m->other < close_level;
	else
	    close = false;
	if ( i>0 && m->other-space[i-1]->other < close_level &&
		m->other-space[i-1]->other > -close_level )	/* In case we reversed things */
	    close = true;
	/* On our first pass through the list, don't set needed/unneeded */
	/*  when to monotonics are close together. (We get rounding errors */
	/*  when things are too close and get confused about the order */
	if ( !close ) {
	    if ( nm!=NULL && nm->other-m->other < .01 ) {
		if ((( m->isneeded || m->isunneeded ) && m->isneeded!=needed &&
			(nm->isneeded==nineeded ||
			 (!nm->isneeded && !nm->isunneeded)) ) ||
		      ( (nm->isneeded || nm->isunneeded) && nm->isneeded!=nneeded &&
			(m->isneeded == inneeded ||
			 (!m->isneeded && !m->isunneeded)) )) {
		    space[i] = nm;
		    space[i+1] = m;
		    needed = nineeded;
		    m = nm;
		}
	    }
	    if ( !m->isneeded && !m->isunneeded ) {
		m->isneeded = needed; m->isunneeded = !needed;
		m->when_set = test;		/* Debugging */
	    } else if ( m->isneeded!=needed || m->isunneeded!=!needed ) {
		SOError( "monotonic is both needed and unneeded (%g,%g)->(%g,%g). %s=%g (prev=%g)\n",
		    ((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+m->s->splines[0].c)*m->tstart+m->s->splines[0].d,
		    ((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+m->s->splines[1].c)*m->tstart+m->s->splines[1].d,
		    ((m->s->splines[0].a*m->tend  +m->s->splines[0].b)*m->tend  +m->s->splines[0].c)*m->tend  +m->s->splines[0].d,
		    ((m->s->splines[1].a*m->tend  +m->s->splines[1].b)*m->tend  +m->s->splines[1].c)*m->tend  +m->s->splines[1].d,
		    which ? "y" : "x", (double) test, m->when_set );
	    }
	}
	winding = nwinding;
	ew = new;
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

static Intersection *FindNeeded(Monotonic *ms,enum overlap_type ot,Intersection *ilist) {
    extended *ends[2];
    Monotonic *m, **space;
    extended top, bottom, test, last, gap_len;
    int i,j,k,l, cnt,which;
    struct gaps *gaps;
    extended min_gap;
    static const double closeness_level[] = { .1, .01, 0, -1 };

    if ( ms==NULL )
return(ilist);

    for ( m=ms, cnt=0; m!=NULL; m=m->linked, ++cnt );
    space = galloc(4*(cnt+2)*sizeof(Monotonic*));	/* We need at most cnt, but we will be adding more monotonics... */

    /* Check (again) for coincident spline segments */
    for ( m=ms; m!=NULL; m=m->linked ) {
	if ( m->b.maxx-m->b.minx > m->b.maxy-m->b.miny ) {
	    top = m->b.maxx;
	    bottom = m->b.minx;
	    which = 0;
	} else {
	    top = m->b.maxy;
	    bottom = m->b.miny;
	    which = 1;
	}
	test=(top+bottom)/2;
	ilist = TryHarderWhenClose(which,test,space,MonotonicFindAt(ms,which,test,space),ilist);
    }

    ends[0] = FindOrderedEndpoints(ms,0);
    ends[1] = FindOrderedEndpoints(ms,1);

    for ( m=ms, cnt=0; m!=NULL; m=m->linked, ++cnt );
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
	FigureNeeds(ms,gaps[i].which,gaps[i].test,space,ot,1.0);

    for ( l=0; closeness_level[l]>=0; ++l ) {
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
	    /* If we try a test which is at one of the endpoints of any monotonic */
	    /*  then bad things happen (because two monotonics will appear to be  */
	    /*  active when only one is. This can be corrected for, but it can    */
	    /*  become very complex and it is easiest just to insure that we never*/
	    /*  test at such a point. So look for the biggest gap along this mono */
	    /*  and test in the middle of that */
	    last = bottom; gap_len = 0; test=-1;
	    for ( i=0; ends[which][i]<top; ++i ) {
		if ( ends[which][i]>bottom ) {
		    if ( ends[which][i]-last > gap_len ) {
			gap_len = ends[which][i]-last;
			test = last + gap_len/2;
		    }
		    last = ends[which][i];
		}
	    }
	    if ( top-last > gap_len ) {
		gap_len = top-last;
		test = last + gap_len/2;
	    }
	    FigureNeeds(ms,which,test,space,ot,closeness_level[l]);
	}
    }
    for ( m=ms; m!=NULL; m=m->linked ) if ( !m->isneeded && !m->isunneeded ) {
	SOError( "Neither needed nor unneeded (%g,%g)->(%g,%g)\n",
		    ((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+m->s->splines[0].c)*m->tstart+m->s->splines[0].d,
		    ((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+m->s->splines[1].c)*m->tstart+m->s->splines[1].d,
		    ((m->s->splines[0].a*m->tend  +m->s->splines[0].b)*m->tend  +m->s->splines[0].c)*m->tend  +m->s->splines[0].d,
		    ((m->s->splines[1].a*m->tend  +m->s->splines[1].b)*m->tend  +m->s->splines[1].c)*m->tend  +m->s->splines[1].d);
    }
    free(ends[0]);
    free(ends[1]);
    free(space);
    free(gaps);
return( ilist );
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
		SOError( "Expected needed monotonic @(%g,%g).\n", (*curil)->inter.x, (*curil)->inter.y );
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
		SOError( "Expected needed monotonic (back) @(%g,%g).\n", (*curil)->inter.x, (*curil)->inter.y );
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
		Monotonic *m = ml->m;
		SOError("Humph. This monotonic leads nowhere (%g,%g)->(%g,%g).\n",
		    ((m->s->splines[0].a*m->tstart+m->s->splines[0].b)*m->tstart+m->s->splines[0].c)*m->tstart+m->s->splines[0].d,
		    ((m->s->splines[1].a*m->tstart+m->s->splines[1].b)*m->tstart+m->s->splines[1].c)*m->tstart+m->s->splines[1].d,
		    ((m->s->splines[0].a*m->tend  +m->s->splines[0].b)*m->tend  +m->s->splines[0].c)*m->tend  +m->s->splines[0].d,
		    ((m->s->splines[1].a*m->tend  +m->s->splines[1].b)*m->tend  +m->s->splines[1].c)*m->tend  +m->s->splines[1].d );
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
	    t1 = IterateSplineSolve(&s1->splines[0],t1start,t1end,x);
	else
	    t1 = IterateSplineSolve(&s1->splines[1],t1start,t1end,y);
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
	ilist = FindNeeded(ms,ot,ilist);
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
