/* Copyright (C) 2000-2004 by George Williams */
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
#include "edgelist.h"
#include <math.h>

/*#define DEBUG 1*/

/* To deal with overlap:
    Ignore any open splinesets
    Find all intersections
    Break splines into bits so that intersections only happen at verteces
    Find all splines which are unneeded (if winding number>1 or <-1 inside em)
    	(have to do this both horizontally and vertically, else we miss
	 horizontal splines)
    If any splineset is entirely unneeded remove it
    	(for instance the inner loop of an O would be unneeded if it went
	 the same direction as the outer)
    If any splineset is entirely needed set it aside
    So now all remaining splineset must be partly needed and partly unneeded,
	which means that they all must intersect at least one other splineset.
    So run through the list of intersections
	At an intersection there should be only two needed splines emerging from it
		(exception a figure 8. Should be an even number then. Pick two)
	Tack these two together as the basis of a new splineset
	Trace them around until we get to the next intersection.
	Find another line at it and keep tracing until we get back to where we started.
	New splineset. Remove these guys from consideration
    Run through all intersections again
	This time we free unused splines
    Now free the old splineset headers (contents are already free)
    Now merge all the bits we care about and return 'em
*/
/* Beware of the self-intersecting spline (half a figure 8) */

typedef struct splinelist {
    struct splinelist *next;
    Spline *spline;
} SplineList;

typedef struct splinetlist {
    struct splinetlist *next;
    Spline *spline;
    double t;
    struct intersection *inter;
    unsigned int processed:1;
} SplineTList;

typedef struct intersection {
    struct intersection *next;
    unsigned int processed: 1;
    BasePoint intersection;
    SplineList *splines;		/* the splines intersecting here */
    SplineTList *oldsplines;
} IntersectionList;

static void AddOldSplines(Spline *spline,double t,IntersectionList *il) {
    SplineTList *new = gcalloc(1,sizeof(SplineTList));

    new->spline = spline;
    new->t = t;
    new->inter = il;
    new->next = il->oldsplines;
    il->oldsplines = new;
}

static IntersectionList *FindIntersectionAt(IntersectionList *old,double xpos,double ypos) {
    IntersectionList *il;
    const double err = 1.0 / 64.0;

    for ( il=old; il!=NULL; il=il->next ) {
	if ( il->intersection.x-xpos>-err && il->intersection.x-xpos<err &&
		il->intersection.y-ypos>-err && il->intersection.y-ypos<err ) {
return( il );
	}
    }
return( NULL );
}

static IntersectionList *AddIntersection(IntersectionList *old,
	Spline *lefts, Spline *rights, double lt, double rt,
	double xpos, double ypos ) {
    IntersectionList *il;
    SplineTList *sp;
    SplinePoint *match;
    const double err = 1.0 / 64.0;
    /* Our needed tester won't notice splines that are less than 1 unit long */
    /*  so any intersections close together should be merged */


    match = NULL;
    if ( lefts->from->me.x-xpos>-.1 && lefts->from->me.x-xpos<.1 &&
	    lefts->from->me.y-ypos>-.1 && lefts->from->me.y-ypos<.1 ) {
	match = lefts->from;
	lt = 0;
    } else if ( lefts->to->me.x-xpos>-.1 && lefts->to->me.x-xpos<.1 &&
	    lefts->to->me.y-ypos>-.1 && lefts->to->me.y-ypos<.1 ) {
	match = lefts->to;
	lt = 1;
    } else if ( rights->from->me.x-xpos>-.1 && rights->from->me.x-xpos<.1 &&
	    rights->from->me.y-ypos>-.1 && rights->from->me.y-ypos<.1 ) {
	match = rights->from;
	rt = 0;
    } else if ( rights->to->me.x-xpos>-.1 && rights->to->me.x-xpos<.1 &&
	    rights->to->me.y-ypos>-.1 && rights->to->me.y-ypos<.1 ) {
	match = rights->to;
	rt = 1;
    }

    /* fixup some rounding errors */
    if ( match!=NULL ) {
	if ( (match==lefts->from && match==rights->to) ||
		(match==lefts->to && match==rights->from))	/* Intersection is where it should be */
return( old );
	xpos = match->me.x;
	ypos = match->me.y;
    } else {
	if ( lefts->ishorvert ) {
	    if ( lefts->from->me.x==lefts->to->me.x )
		xpos = lefts->from->me.x;
	    if ( lefts->from->me.y==lefts->to->me.y )
		ypos = lefts->from->me.y;
	}
	if ( rights->ishorvert ) {
	    if ( rights->from->me.x==rights->to->me.x )
		xpos = rights->from->me.x;
	    if ( rights->from->me.y==rights->to->me.y )
		ypos = rights->from->me.y;
	}
    }

    for ( il=old; il!=NULL; il=il->next ) {
	if ( il->intersection.x-xpos>-err && il->intersection.x-xpos<err &&
		il->intersection.y-ypos>-err && il->intersection.y-ypos<err ) {
	    int lpresent=0, rpresent=0;
	    for ( sp = il->oldsplines; sp!=NULL; sp=sp->next ) {
		if ( sp->spline==lefts )
		    lpresent = 1;
		else if ( sp->spline==rights )
		    rpresent = 1;
	    }
	    if ( !lpresent )
		AddOldSplines(lefts,lt,il);
	    if ( !rpresent )
		AddOldSplines(rights,rt,il);
return( old );
	}
    }

#ifdef DEBUG
 printf( "Intersection at %g,%g\n", xpos, ypos);
#endif
    il = gcalloc(1,sizeof(IntersectionList));
    il->intersection.x = xpos; il->intersection.y = ypos;
    AddOldSplines(lefts,lt,il);
    AddOldSplines(rights,rt,il);
    if ( match!=NULL ) {
	if ( match->prev!=lefts && match->prev!=rights )
	    AddOldSplines(match->prev,1.0,il);
	if ( match->next!=lefts && match->next!=rights )
	    AddOldSplines(match->next,0,il);
    }
    il->next = old;
return( il );
}

static IntersectionList *IntersectionOf(Edge *wasleft,Edge *wasright,
	IntersectionList *old, double mpos, EdgeList *es, double lastincr) {
    /* first find the point of intersection. It's somewhere between oldt and t_cur */
    /*  major coordinate value is somewhere between mpos-1 and mpos */
    double mtop, mbottom, xpos, ypos;
    double ltop, lbottom, lt=-1, rtop, rbottom, rt=-1, locur, rocur=0, nrt=-1, nlt;
    Spline1D *lsp, *rsp, *losp, *rosp;

    lsp = &wasleft->spline->splines[es->major];
    rsp = &wasright->spline->splines[es->major];
    losp = &wasleft->spline->splines[es->other];
    rosp = &wasright->spline->splines[es->other];
    ltop = wasleft->t_cur; lbottom = wasleft->oldt;
    if ( wasleft->up ) {
	if ( (ltop+=.001)>wasleft->t_mmax ) ltop = wasleft->t_mmax;
	if ( (lbottom-=.001)<wasleft->t_mmin ) lbottom = wasleft->t_mmin;
    } else {
	if ( (ltop-=.001)<wasleft->t_mmax ) ltop = wasleft->t_mmax;
	if ( (lbottom+=.001)>wasleft->t_mmin ) lbottom = wasleft->t_mmin;
    }
    rtop = wasright->t_cur; rbottom = wasright->oldt;
    if ( wasright->up ) {
	if ( (rtop+=.001)>wasright->t_mmax ) rtop = wasright->t_mmax;
	if ( (rbottom-=.001)<wasright->t_mmin ) rbottom = wasright->t_mmin;
    } else {
	if ( (rtop-=.001)<wasright->t_mmax ) rtop = wasright->t_mmax;
	if ( (rbottom+=.001)>wasright->t_mmin ) rbottom = wasright->t_mmin;
    }
    mbottom = (mpos-lastincr+es->mmin)/es->scale; mtop = (mpos+es->mmin)/es->scale;
    locur = -1;
    while ( 1 ) {
	nlt = SplineSolve(lsp,lbottom,ltop,(mbottom+mtop)/2,.0001);
	nrt = SplineSolve(rsp,rbottom,rtop,(mbottom+mtop)/2,.0001);
	if ( nlt==-1 || nrt==-1 ) {
	    if ( locur!=-1 )
    break;
 nlt = SplineSolve(lsp,lbottom,ltop,(mbottom+mtop)/2,.0001);
 nrt = SplineSolve(rsp,rbottom,rtop,(mbottom+mtop)/2,.0001);	/* Debug */
	    IError("Request for an intersection out of range");
return( old );
	}
	lt = nlt; rt = nrt;
	locur =  ((losp->a*lt+losp->b)*lt+losp->c)*lt + losp->d ;
	rocur =  ((rosp->a*rt+rosp->b)*rt+rosp->c)*rt + rosp->d ;
#if 0
	if ( (oerr = locur/(1024*1024))<0 ) oerr= -oerr;
	if ( oerr<.0001 ) oerr = .0001;
	if ( ((locur-rocur)>-oerr && (locur-rocur)<oerr) ||
#endif
	if ( ((locur-rocur)>-.0001 && (locur-rocur)<.0001) ||
		mtop-mbottom<.0001 )
    break;
	if ( locur<rocur ) {
	    lbottom = lt;
	    rbottom = rt;
	    mbottom = (mbottom+mtop)/2;
	} else {
	    ltop = lt;
	    rtop = rt;
	    mtop = (mbottom+mtop)/2;
	}
    }
    if ( es->major==1 ) {
	ypos = (mtop+mbottom)/2;
	xpos = (locur+rocur)/2;
    } else {
	xpos = (mtop+mbottom)/2;
	ypos = (locur+rocur)/2;
    }

    if (( wasleft->spline->to->next==wasright->spline &&
	    RealNearish(xpos,wasleft->spline->to->me.x) &&
	    RealNearish(ypos,wasleft->spline->to->me.y)) ||
	( wasleft->spline->from->prev==wasright->spline &&
	    RealNearish(xpos,wasleft->spline->from->me.x) &&
	    RealNearish(ypos,wasleft->spline->from->me.y)) )
return( old );

return( AddIntersection(old,wasleft->spline,wasright->spline,lt,rt,xpos,ypos));
}

#if 0		/* Shouldn't need this, Linear intersections should get it better? */
static IntersectionList *MajorIntersectionOf(Edge *par_major,Edge *edge,
	IntersectionList *old, double mpos, EdgeList *es) {
    /* first find the point of intersection. It's somewhere between oldt and t_cur */
    /*  major coordinate value is major->mmax (or mmin) */
    double et, opos, mt;
    Spline1D *esp, *eosp;
    double ypos, xpos;

    esp = &edge->spline->splines[es->major];
    eosp = &edge->spline->splines[es->other];
    et = SplineSolve(esp,edge->oldt,edge->t_cur,par_major->mmin+es->mmin,.0001);
    opos = ( ((eosp->a*et + eosp->b)*et + eosp->c)*et + eosp->d ) * es->scale;
    if (( opos<par_major->o_mmin && opos<par_major->o_mmax ) ||
	    (opos>par_major->o_mmin && opos>par_major->o_mmax ) || et<0 || et>1 )
return( old );

    mt = (opos-par_major->o_mmin)/(par_major->o_mmax-par_major->o_mmin);
    if ( es->major==1 ) {
	ypos = par_major->mmin+es->mmin;
	xpos = opos;
    } else {
	xpos = par_major->mmin+es->mmin;
	ypos = opos;
    }
    xpos = rint(xpos*1024)/1024;
    ypos = rint(ypos*1024)/1024;

    if (( edge->spline->to->next==par_major->spline &&
	    RealNearish(xpos,edge->spline->to->me.x) &&
	    RealNearish(ypos,edge->spline->to->me.y)) ||
	( edge->spline->from->prev==par_major->spline &&
	    RealNearish(xpos,edge->spline->from->me.x) &&
	    RealNearish(ypos,edge->spline->from->me.y)) )
return( old );

return( AddIntersection(old,edge->spline,par_major->spline,et,mt,xpos,ypos));
}
#endif

/* This routine is almost exactly the same as ActiveEdgesRefigure, except */
/*  that we preserve the old value of t each time through and call */
static IntersectionList *_FindIntersections(EdgeList *es, IntersectionList *sofar) {
    Edge *active=NULL, *apt, *pr;
    double i, incr=1, lastincr,diff;
    int any;

    for ( i=0; i<es->cnt; i+=incr ) {
	if ( ( diff = i - floor(i+.00005))<0 ) diff=-diff;
	if ( diff < .0001 )
	    i = floor(i+.00005);
	lastincr = incr;
	incr = floor(i)+1-i;

	/* first remove any entry which doesn't intersect the new scan line */
	/*  (ie. stopped on last line) */
	for ( pr=NULL, apt=active; apt!=NULL; apt = apt->aenext ) {
	    if ( apt->mmax<i ) {
		if ( pr==NULL )
		    active = apt->aenext;
		else
		    pr->aenext = apt->aenext;
	    } else
		pr = apt;
	}
	/* then move the active list to the next line */
	for ( apt=active; apt!=NULL; apt = apt->aenext ) {
	    Spline1D *osp = &apt->spline->splines[es->other];
	    apt->oldt = apt->t_cur;
	    apt->t_cur = TOfNextMajor(apt,es,i);
	    apt->o_cur = ( ((osp->a*apt->t_cur+osp->b)*apt->t_cur+osp->c)*apt->t_cur + osp->d ) * es->scale;
	}
	/* reorder list */
	if ( active!=NULL ) {
	    any = true;
	    while ( any ) {
		any = false;
		for ( pr=NULL, apt=active; apt->aenext!=NULL; ) {
		    if ( apt->o_cur <= apt->aenext->o_cur ) {
			/* still ordered */;
			pr = apt;
			apt = apt->aenext;
		    } else if ( pr==NULL ) {
			sofar = IntersectionOf(apt,apt->aenext, sofar, i, es, lastincr);
			active = apt->aenext;
			apt->aenext = apt->aenext->aenext;
			active->aenext = apt;
			/* don't need to set any, since this reorder can't disorder the list */
			pr = active;
		    } else {
			sofar = IntersectionOf(apt,apt->aenext, sofar, i, es, lastincr);
			pr->aenext = apt->aenext;
			apt->aenext = apt->aenext->aenext;
			pr->aenext->aenext = apt;
			any = true;
			pr = pr->aenext;
		    }
		}
	    }
#if 0
	    while ( es->majors!=NULL && es->majors->mmax<=i ) {
		pr = es->majors;
		for ( apt=active; apt!=NULL; apt=apt->aenext )
		    sofar = MajorIntersectionOf(pr,apt,sofar,i,es);
		es->majors = pr->esnext;
		pr->esnext = es->majorhold;
		es->majorhold = pr;
	    }
#endif
	}
	/* Insert new nodes */
	if ( i==floor(i) )
	    active = ActiveEdgesInsertNew(es,active,i);

	/* If any spline ends soon, adjust the increment so we get one last look at it */
	for ( apt=active; apt!=NULL; apt = apt->aenext ) {
	    if ( apt->mmax<i+incr && apt->mmax>i+.0002)
		incr = apt->mmax-i-.0001;
	}
    }
return( sofar );
}

#if 0
static int IsHorVertSpline(Spline *sp) {
    int major;
    Spline1D *msp;
    double fm, tm;
    /* Check for splines that are very close to being horizontal or vertical */
    /*  we shall just pretend that they are lines.... */

    for ( major=0; major<2; ++major ) {
	fm = major==1?sp->from->me.y:sp->from->me.x;
	tm = major==1?sp->to->me.y:sp->to->me.x;
	msp = &sp->splines[major];
	if ( fm==tm && !RealNear(msp->a,0) ) {
	    double m1, m2, d1, d2, t1, t2;
	    double b2_fourac = 4*msp->b*msp->b - 12*msp->a*msp->c;
	    if ( b2_fourac>=0 ) {
		b2_fourac = sqrt(b2_fourac);
		t1 = (-2*msp->b - b2_fourac) / (6*msp->a);
		t2 = (-2*msp->b + b2_fourac) / (6*msp->a);
		if ( t1>t2 ) { double temp = t1; t1 = t2; t2 = temp; }
		else if ( t1==t2 ) t2 = 2.0;

		m1 = m2 = fm;
		if ( t1>0 && t1<1 )
		    m1 = ((msp->a*t1+msp->b)*t1+msp->c)*t1 + msp->d;
		if ( t2>0 && t2<1 )
		    m2 = ((msp->a*t2+msp->b)*t2+msp->c)*t2 + msp->d;
		d1 = (m1-fm);
		d2 = (m2-fm);
		if ( d1>-1 && d1<1 && d2>-1 && d2<1 )
return( true );
	    }
	}
    }
return( false );
}
#endif

static IntersectionList *FindLinearIntersections(SplineSet *spl, IntersectionList *sofar) {
    SplineSet *ss;
    Spline *first, *sp, *first2, *sp2;
    BasePoint pts[9];
    double t1s[10], t2s[10];
    int i;
    /* This used only to work if both splines were lines. Now it works if */
    /*  at least one of the splines is a line */

    for ( ; spl!=NULL; spl = spl->next ) if ( spl->first->prev!=NULL ) {
	first = NULL;
	for ( sp = spl->first->next; sp!=NULL && sp!=first; sp=sp->to->next ) {
	    if ( first==NULL ) first = sp;
	    for ( ss=spl; ss!=NULL; ss = ss->next ) {
		if ( ss==spl ) {
		    first2 = first;
		    sp2 = sp->to->next;
		} else {
		    first2 = NULL;
		    sp2 = ss->first->next;
		}
		for ( ; sp2!=NULL && sp2!=first2; sp2 = sp2->to->next ) {
		    if (( sp->islinear || sp2->islinear ) &&
			    SplinesIntersect(sp,sp2,pts,t1s,t2s)>0 ) {
			for ( i=0; i<4 && t1s[i]!=-1; ++i )
			    if ( (t1s[i]>.001 && t1s[i]<.999) || (t2s[i]>.001 && t2s[i]<.999))
				sofar = AddIntersection(sofar,sp,sp2,t1s[i],t2s[i],
					pts[i].x,pts[i].y);
		    }
		    if ( first2==NULL ) first2 = sp2;
		}
	    }
	}
    }
return( sofar );
}

static IntersectionList *FindThisVertextIntersections(SplineSet *spl,
	SplinePoint *sp, IntersectionList *ilist) {
    Spline *spline, *first;
    double t, xpos, ypos;

    for ( ; spl!=NULL ; spl=spl->next ) if ( spl->first->prev!=NULL ) {
	first = NULL;
	for ( spline=spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	    if ( spline->from!=sp && spline->to!=sp &&
		    !((sp->me.x<spline->from->me.x && sp->me.x<spline->from->nextcp.x &&
		       sp->me.x<spline->to->me.x && sp->me.x<spline->to->prevcp.x ) ||
		      (sp->me.x>spline->from->me.x && sp->me.x>spline->from->nextcp.x &&
		       sp->me.x>spline->to->me.x && sp->me.x>spline->to->prevcp.x ) ||
		      (sp->me.y<spline->from->me.y && sp->me.y<spline->from->nextcp.y &&
		       sp->me.y<spline->to->me.y && sp->me.y<spline->to->prevcp.y ) ||
		      (sp->me.y>spline->from->me.y && sp->me.y>spline->from->nextcp.y &&
		       sp->me.y>spline->to->me.y && sp->me.y>spline->to->prevcp.y ))) {
		/* If we get here then sp is within the convex hull of the spline */
		/*  Actually it's within a bounding box that includes the hull */
		if (( t = SplineNearPoint(spline,&sp->me,1./128.))!=-1 ) {
		    /* if we get here there's a real intersection */
		    xpos = ((spline->splines[0].a*t+spline->splines[0].b)*t+spline->splines[0].c)*t + spline->splines[0].d;
		    ypos = ((spline->splines[1].a*t+spline->splines[1].b)*t+spline->splines[1].c)*t + spline->splines[1].d;
		    ilist = AddIntersection(ilist,spline,sp->prev,
				t,1,xpos,ypos);
		}
	    }
	    if ( first==NULL ) first = spline;
	}
    }
return( ilist );
}

static IntersectionList *FindVertexIntersections(SplineSet *ss,
	IntersectionList *ilist) {
    SplinePoint *sp;
    SplineSet *spl;

    for ( spl=ss; spl!=NULL ; spl=spl->next ) if ( spl->first->prev!=NULL ) {
	for ( sp=spl->first; ; ) {
	    ilist = FindThisVertextIntersections(ss,sp,ilist);
	    sp = sp->next->to;			/* paths are all closed */
	    if ( sp==spl->first )
	break;
	}
    }
return( ilist );
}

struct tllist { SplineTList *tl; struct tllist *next; };
static struct tllist *AddTList(struct tllist *sofar, SplineTList *this) {
    struct tllist *cur, *test, *prev;

    cur = galloc(sizeof(struct tllist));
    cur->next = NULL;
    cur->tl = this;
    if ( sofar==NULL )
return( cur );
    prev = NULL;
    for ( test=sofar; test!=NULL && test->tl->t<this->t; prev=test, test = test->next );
    cur->next = test;
    if ( prev==NULL )
return( cur );
    prev->next = cur;
return( sofar );
}

static void AddSplines(IntersectionList *il,Spline *s1,Spline *s2) {
    SplineList *sl1, *sl2;
    sl1 = galloc(sizeof(SplineList));
    sl2 = galloc(sizeof(SplineList));
    sl1->spline = s1; sl2->spline = s2;
    sl1->next = sl2;
    sl2->next = il->splines;
    il->splines = sl1;
}

#if 0
static void AddSpline(IntersectionList *il,Spline *s) {
    SplineList *sl1;
    sl1 = galloc(sizeof(SplineList));
    sl1->spline = s;
    sl1->next = il->splines;
    il->splines = sl1;
}
#endif

static void ChangeSpline(IntersectionList *il,Spline *new,Spline *old) {
    /* Spline old just got broken into two pieces (one of which is new) */
    /*  need to change any reference to the old spline into ones to the new */
    SplineList *sl;

    if ( il==NULL )
return;

    for ( sl = il->splines; sl!=NULL; sl = sl->next )
	if ( sl->spline == old ) {
	    sl->spline = new;
    break;
	}
}

static void ChangeAndCleanSpline(IntersectionList *il,Spline *new,Spline *old) {
    /* Spline old just got broken into two pieces (one of which is new) */
    /*  need to change any reference to the old spline into ones to the new */
    SplineList *sl, *prev, *next;
    int sawnew;

    while ( il!=NULL ) {
	sawnew = false;
	prev = NULL;
	for ( sl = il->splines; sl!=NULL; sl = next ) {
	    next = sl->next;
	    if ( sawnew && (sl->spline==old || sl->spline==new) ) {
		prev->next = next;
		free(sl);
	    } else {
		if ( sl->spline == old ) {
		    sl->spline = new;
		    sawnew = true;
		} else if ( sl->spline==new )
		    sawnew = true;
		prev = sl;
	    }
	}
	il = il->next;
    }
}

static void DoIntersections(SplineChar *sc,SplineTList *me,IntersectionList *ilist) {
    struct tllist *base = NULL, *cur, *cnext;
    SplineTList *tsp;
    double tbase, t;
    SplinePoint *to, *sp;
    IntersectionList *prev;
    Spline *lastspline=NULL;
    BasePoint test;
    int atfrom, atto;

    while ( ilist!=NULL ) {
	for ( tsp=ilist->oldsplines; tsp!=NULL; tsp=tsp->next ) {
	    if ( tsp->spline == me->spline )
		base = AddTList(base,tsp);
	}
	ilist = ilist->next;
    }

    tbase = 0;
    lastspline = me->spline;
    to = me->spline->to;		/* The spline will change as we bisect*/
			    /* it, but the end point remains fixed and we can */
			    /* use that to figure out what spline we should be*/
			    /* splitting now */
    prev = NULL;
    for ( cur=base; cur!=NULL; cur=cnext ) {
	cnext = cur->next;
	if ( tbase==cur->tl->t )
	    /* That's ok, I guess */;
	    /* Seem to get a lot of zero length splines, especially at the */
	    /*  endpoints. Should be ok just to ignore them */
	else if ( tbase>=1 ) {
	    IError( "Bad value for tbase in %s", sc==NULL?"<nameless char>": sc->name );
    break;
	} else {
	    t = (cur->tl->t-tbase)/(1.0-tbase);
#if 0
	    test.x = ((to->prev->splines[0].a*t+to->prev->splines[0].b)*t+to->prev->splines[0].c)*t + to->prev->splines[0].d;
	    test.y = ((to->prev->splines[1].a*t+to->prev->splines[1].b)*t+to->prev->splines[1].c)*t + to->prev->splines[1].d;
#else
	    test = cur->tl->inter->intersection;
#endif
	    atfrom = ( to->prev->from->me.x>test.x-.005 && to->prev->from->me.x<test.x+.005 &&
		    to->prev->from->me.y>test.y-.005 && to->prev->from->me.y<test.y+.005 );
	    atto = ( to->me.x>test.x-.005 && to->me.x<test.x+.005 &&
		to->me.y>test.y-.005 && to->me.y<test.y+.005 );
	    if ( atfrom && ( !atto || cur->tl->t<.5 )) {
		/* if the intersection occurs at an end point then we don't need */
		/*  to cut the spline in two */
		sp = to->prev->from;
		t = 0;
	    } else if ( atto ) {
		sp = to;
		t = 1;
	    } else {
		SplineBisect(to->prev,t);
		sp = to->prev->from;
	    }
	    sp->isintersection = true;
	    sp->nextcp.x += cur->tl->inter->intersection.x-sp->me.x;
	    sp->prevcp.x += cur->tl->inter->intersection.x-sp->me.x;
	    sp->nextcp.y += cur->tl->inter->intersection.y-sp->me.y;
	    sp->prevcp.y += cur->tl->inter->intersection.y-sp->me.y;
	    sp->me = cur->tl->inter->intersection;	/* might be slightly different due to rounding. I want it exact */
	    SplineRefigure(sp->prev);		/* Again fix up any marginal diffs */
	    SplineRefigure(sp->next);
	    if ( t==0 || t==1 ) {
		AddSplines(cur->tl->inter,sp->prev,sp->next);
	    } else {
		AddSplines(cur->tl->inter,sp->prev,sp->next);
		ChangeSpline(prev,sp->prev,lastspline);
	    }
	    lastspline = sp->next;
	}
	tbase = cur->tl->t;
	cur->tl->processed = true;
	prev = cur->tl->inter;
	free(cur);
    }
}

static void CleanupSplines(IntersectionList *ilist,IntersectionList *ilbase) {
    SplineList *sl1, *sl2, *prev, *next;

    /* Suppose we have two splines which are essentially coincident */
    /* We will probably get some rounding errors when calculating the */
    /*  intersections. These will lead to more intersections, and so forth */
    /* So try to even out any rounding errors that might lead to problems */
    for ( sl1 = ilist->splines; sl1!=NULL; sl1=sl1->next ) {
	for ( sl2 = sl1->next; sl2!=NULL ; sl2 = sl2->next ) {
	    if ( sl2->spline->from->me.x==sl1->spline->from->me.x &&
		    sl2->spline->from->me.y==sl1->spline->from->me.y ) {
		if ( RealWithin(sl2->spline->from->nextcp.x,sl1->spline->from->nextcp.x,.1) &&
			RealWithin(sl2->spline->from->nextcp.y,sl1->spline->from->nextcp.y,.1)) {
		    sl2->spline->from->nextcp = sl1->spline->from->nextcp;
		    if ( sl2->spline->order2 ) sl2->spline->to->prevcp = sl2->spline->from->nextcp;
		    SplineRefigure(sl2->spline);
		}
	    } else if ( sl2->spline->from->me.x==sl1->spline->to->me.x &&
		    sl2->spline->from->me.y==sl1->spline->to->me.y ) {
		if ( RealWithin(sl2->spline->from->nextcp.x,sl1->spline->to->prevcp.x,.1) &&
			RealWithin(sl2->spline->from->nextcp.y,sl1->spline->to->prevcp.y,.1)) {
		    sl2->spline->from->nextcp = sl1->spline->to->prevcp;
		    if ( sl2->spline->order2 ) sl2->spline->to->prevcp = sl2->spline->from->nextcp;
		    SplineRefigure(sl2->spline);
		}
	    } else if ( sl2->spline->to->me.x==sl1->spline->from->me.x &&
		    sl2->spline->to->me.y==sl1->spline->from->me.y ) {
		if ( RealWithin(sl2->spline->to->prevcp.x,sl1->spline->from->nextcp.x,.1) &&
			RealWithin(sl2->spline->to->prevcp.y,sl1->spline->from->nextcp.y,.1)) {
		    sl2->spline->to->prevcp = sl1->spline->from->nextcp;
		    if ( sl2->spline->order2 ) sl2->spline->from->nextcp = sl2->spline->to->prevcp;
		    SplineRefigure(sl2->spline);
		}
	    } else if ( sl2->spline->to->me.x==sl1->spline->to->me.x &&
		    sl2->spline->to->me.y==sl1->spline->to->me.y ) {
		if ( RealWithin(sl2->spline->to->prevcp.x,sl1->spline->to->prevcp.x,.1) &&
			RealWithin(sl2->spline->to->prevcp.y,sl1->spline->to->prevcp.y,.1)) {
		    sl2->spline->to->prevcp = sl1->spline->to->prevcp;
		    if ( sl2->spline->order2 ) sl2->spline->from->nextcp = sl2->spline->to->prevcp;
		    SplineRefigure(sl2->spline);
		}
	    }
	}
    }

/* if we have a T shape and the ends of the vertical stem are on the horizontal*/
/*  then we will get two copies of the line that connects the end of the vert */
/*  stem (one from the vert stem, and one from the horizontal stem) we need to*/
/*  merge these two splines into one */
return;	/* But it seems we don't need it at all now */

    for ( sl1 = ilist->splines; sl1!=NULL; sl1=sl1->next ) {
	prev = sl1;
	for ( sl2 = sl1->next; sl2!=NULL ; sl2 = next ) {
	    next = sl2->next;
	    if (( sl2->spline->from->me.x==sl1->spline->from->me.x && sl2->spline->from->me.y==sl1->spline->from->me.y &&
		    sl2->spline->to->me.x==sl1->spline->to->me.x && sl2->spline->to->me.y==sl1->spline->to->me.y ) ||
		( sl2->spline->from->me.x==sl1->spline->to->me.x && sl2->spline->from->me.y==sl1->spline->to->me.y &&
		    sl2->spline->to->me.x==sl1->spline->from->me.x && sl2->spline->to->me.y==sl1->spline->from->me.y )) {
		prev->next = next;
		ChangeAndCleanSpline(ilbase,sl1->spline,sl2->spline);
		SplineFree(sl2->spline);
		free(sl2);
	    } else
		prev = sl2;
	}
    }
}

/* Put the intersections onto the splines. We couldn't do this earlier because*/
/*  that would have meant freeing the old splines while we were still using */
/*  them to find more intersections. */
/* Things are complex here because one spline may have several intersections */
/*  (think of a plus sign) and we must do them all at once... */
static void InsertIntersections(SplineChar *sc,IntersectionList *ilist) {
    SplineTList *tsp, *tnext;
    IntersectionList *ilbase = ilist;

    while ( ilist!=NULL ) {
#ifdef DEBUG
 printf( "Inter=(%g,%g)\n", ilist->intersection.x, ilist->intersection.y );
#endif
	for ( tsp=ilist->oldsplines; tsp!=NULL; tsp=tsp->next ) {
#ifdef DEBUG
 printf( "\t(%g,%g) -> (%g,%g) %g\n",
  tsp->spline->from->me.x, tsp->spline->from->me.y,
  tsp->spline->to->me.x, tsp->spline->to->me.y,
  tsp->t );
#endif
	    if ( !tsp->processed )
		DoIntersections(sc,tsp,ilist);
	}
#ifdef DEBUG
 { SplineList *sl; printf(" --\n" );
  for ( sl=ilist->splines; sl!=NULL; sl=sl->next )
   if ( sl->spline->from->me.x==ilist->intersection.x && sl->spline->from->me.y==ilist->intersection.y )
    printf( "\t-> (%g,%g)\n", sl->spline->to->me.x, sl->spline->to->me.y );
   else if ( sl->spline->to->me.x==ilist->intersection.x && sl->spline->to->me.y==ilist->intersection.y )
    printf( "\t-> (%g,%g)\n", sl->spline->from->me.x, sl->spline->from->me.y );
   else
    printf( "\t-> spline (%g,%g) -> (%g,%g) not at intersection, should be\n",
     sl->spline->from->me.x, sl->spline->from->me.y,
     sl->spline->to->me.x, sl->spline->to->me.y );
 }
#endif
	for ( tsp=ilist->oldsplines; tsp!=NULL; tsp=tnext ) {
	    tnext = tsp->next;
	    free(tsp);
	}
	ilist->oldsplines = NULL;
	CleanupSplines(ilist,ilbase);
	ilist = ilist->next;
    }
}

static IntersectionList *SplineSetFindIntersections(SplineChar *sc,SplineSet *base) {
    EdgeList es;
    DBounds b;
    IntersectionList *ilist= NULL;

    /* We miss intersections where the end-point of a spline is on */
    /*  another spline. Check for them first */
    ilist = FindVertexIntersections(base,ilist);

    /* We can find linear intersections exactly, so let's do so */
    /* Do this early so we'll store the exact position, otherwise it might */
    /*  be an approximation */
    ilist = FindLinearIntersections(base,ilist);

    SplineSetFindBounds(base,&b);
    memset(&es,'\0',sizeof(es));
    es.scale = 1.0;
    es.mmin = floor(b.miny*es.scale);
    es.mmax = ceil(b.maxy*es.scale);
    es.omin = b.minx*es.scale;
    es.omax = b.maxx*es.scale;
    es.cnt = (int) (es.mmax-es.mmin) + 1;
    es.edges = gcalloc(es.cnt,sizeof(Edge *));
    es.interesting = gcalloc(es.cnt,sizeof(char));
    es.sc = NULL;
    es.major = 1; es.other = 0;
    es.genmajoredges = true;
    es.is_overlap = true;
    FindEdgesSplineSet(base,&es);
    ilist = _FindIntersections(&es,ilist);
    FreeEdges(&es);

    /* Have to check both directions, else we lose horizontal line intersections */
    es.mmin = floor(b.minx*es.scale);
    es.mmax = ceil(b.maxx*es.scale);
    es.omin = b.miny*es.scale;
    es.omax = b.maxy*es.scale;
    es.cnt = (int) (es.mmax-es.mmin) + 1;
    es.edges = gcalloc(es.cnt,sizeof(Edge *));
    es.interesting = gcalloc(es.cnt,sizeof(char));
    es.major = 0; es.other = 1;
    es.majors = NULL;
    es.genmajoredges = true;
    FindEdgesSplineSet(base,&es);
    ilist = _FindIntersections(&es,ilist);
    FreeEdges(&es);

    InsertIntersections(sc,ilist);
return( ilist );
}

static int bottomcmp(const void *_e1, const void *_e2) {
    const EI *e1 = *(EI *const *) _e1, *e2 = *(EI *const *) _e2;
    const double min1 = e1->coordmin[e1->major], min2 = e2->coordmin[e1->major];

    if ( min1==min2 ) {
	double tmmin1, tmmax1, tmmin2, tmmax2;
	double o1, o2;
	int other = !e1->major;
	Spline1D *o;
	if ( e1->up ) { tmmin1 = e1->tmin; tmmax1 = e1->tmax; }
	else { tmmin1 = e1->tmax; tmmax1 = e1->tmin; }
	if ( e2->up ) { tmmin2 = e2->tmin; tmmax2 = e2->tmax; }
	else { tmmin2 = e2->tmax; tmmax2 = e2->tmin; }
	o = &e1->spline->splines[other];
	o1 = ((o->a*tmmin1+o->b)*tmmin1+o->c)*tmmin1+o->d;
	o = &e2->spline->splines[other];
	o2 = ((o->a*tmmin2+o->b)*tmmin2+o->c)*tmmin2+o->d;
	if ( o1>o2 )
return( 1 );
	else if ( o2<o1 )
return( -1 );
	o = &e1->spline->splines[other];
	o1 = ((o->a*tmmax1+o->b)*tmmax1+o->c)*tmmax1+o->d;
	o = &e2->spline->splines[other];
	o2 = ((o->a*tmmax2+o->b)*tmmax2+o->c)*tmmax2+o->d;
	if ( o1>o2 )
return( 1 );
	else if ( o2<o1 )
return( -1 );

return( 0 );
    } else if ( min1 > min2 )
return( 1 );
    else
return( -1 );
}

static int topcmp(const void *_e1, const void *_e2) {
    const EI *e1 = *(EI *const *) _e1, *e2 = *(EI *const *) _e2;
    const double max1 = e1->coordmax[e1->major], max2 = e2->coordmax[e1->major];

return( ( max1>max2 ) ? 1 : ( max1<max2 ) ? -1 : 0 );
}

static void ELCarefullOrder(EIList *el, int major) {
    int i, ecnt;
    EI *e;

    for ( i=ecnt=0; i<el->cnt; ++i ) {
	for ( e=el->ordered[i]; e!=NULL; e=e->ordered )
	    ++ecnt;
    }
    el->bottoms = galloc((ecnt+1)*sizeof(EI *));
    el->tops    = galloc((ecnt+1)*sizeof(EI *));
    for ( i=ecnt=0; i<el->cnt; ++i ) {
	for ( e=el->ordered[i]; e!=NULL; e=e->ordered ) {
	    if ( e->coordmin[major]!=e->coordmax[major] ) {
		el->bottoms[ecnt] = el->tops[ecnt] = e;
		++ecnt;
	    }
	    e->major = major;
	}
    }
    el->bottoms[ecnt] = el->tops[ecnt] = NULL;
    qsort(el->bottoms,ecnt,sizeof(EI *),bottomcmp);
    qsort(el->tops,ecnt,sizeof(EI *),topcmp);
}

static int ExactlySame(EI *e1,EI *e2) {
    if ( e1==e2 )
return( true );

    if ( e1->spline->from->me.x==e2->spline->from->me.x && e1->spline->from->me.y==e2->spline->from->me.y &&
	    e1->spline->to->me.x==e2->spline->to->me.x && e1->spline->to->me.y==e2->spline->to->me.y &&
	    e1->spline->from->nonextcp && e2->spline->from->nonextcp &&
	    e1->spline->to->noprevcp && e2->spline->to->noprevcp )
return( true );
    /* And in reverse... */
    if ( e1->spline->from->me.x==e2->spline->to->me.x && e1->spline->from->me.y==e2->spline->to->me.y &&
	    e1->spline->to->me.x==e2->spline->from->me.x && e1->spline->to->me.y==e2->spline->from->me.y &&
	    e1->spline->from->nonextcp && e2->spline->to->nonextcp &&
	    e1->spline->to->noprevcp && e2->spline->from->noprevcp )
return( true );

return( false );
}

static EI *CountDuplicates(SplineChar *sc,enum overlap_type ot,
	EI *apt,int *_cnt, int *_ecnt) {
    int cnt = *_cnt, tot, etot, c;
    EI *test, *needed=NULL, *notunneeded=NULL;

    /* If we have two (or more) tangent splines then they will add 2 to the */
    /*  count. But they need to be treated carefully. One of them will always */
    /*  have to go, but the other might be needed. And if we get the wrong one*/
    /*  ... */
    tot = c = etot = 0;
    for ( test=apt; test!=NULL && ExactlySame(apt,test); test=test->aenext ) {
	if ( test->spline->isneeded ) needed = test;
	if ( !test->spline->isunneeded ) notunneeded = test;
	if ( ot==over_exclude && test->spline->exclude )
	    etot += test->up?1:-1;
	else
	    tot += test->up?1:-1;
	++c;
    }
    if ( ot == over_remove ) {
	if (( cnt==0 && tot==0 ) ||		/* The two (or more) lines cancel out */
		(cnt!=0 && cnt+tot!=0)) {	/* Normal case of internal lines */
#if DEBUG
	    if ( needed!=NULL && !needed->spline->isunneeded )
		IError( c==1?
		    "A spline is both needed and unneeded in CountDuplicates#1 in %s":
		    "A set of tangent splines is both needed and unneeded in CountDuplicates#1 in %s",
		    sc!=NULL?sc->name:"<nameless char>");
#endif
	    for ( test=apt; test!=NULL && ExactlySame(apt,test); test=test->aenext )
		test->spline->isunneeded = true;
	} else /* if (( cnt==0 && tot!=0 ) || (cnt!=0 && cnt+tot==0))*/ {
#if DEBUG
	    if ( needed )
		/* Already done */;
	    else if ( notunneeded==NULL )
		IError( c==1?
		    "A spline is both needed and unneeded in CountDuplicates#2 in %s":
		    "A set of tangent splines is both needed and unneeded in CountDuplicates#2 in %s",
		    sc!=NULL?sc->name:"<nameless char>");
#endif
	    for ( test=apt; test!=NULL && ExactlySame(apt,test); test=test->aenext ) {
		test->spline->isunneeded = true;
		test->spline->isneeded = false;
	    }
	    apt->spline->isunneeded = false;
	    apt->spline->isneeded = true;
	}
    } else if ( ot == over_intersect ) {
	if (( cnt>-2 && cnt<2 && cnt+tot>-2 && cnt+tot<2 ) ||
		((cnt<=-2 || cnt>=2) && (cnt+tot<=-2 || cnt+tot>=2))) {
	    for ( test=apt; test!=NULL && ExactlySame(apt,test); test=test->aenext )
		test->spline->isunneeded = true;
	} else /* if ((cnt>-2 && cnt<2 && (cnt+tot<=-2 || cnt+tot>=2)) ||
		    ((cnt<=-2 || cnt>=2) && cnt+tot>-2 && cnt+tot<2 ))*/ {
	    for ( test=apt; test!=NULL && ExactlySame(apt,test); test=test->aenext ) {
		test->spline->isunneeded = true;
		test->spline->isneeded = false;
	    }
	    apt->spline->isunneeded = false;
	    apt->spline->isneeded = true;
	}
    } else if ( ot == over_exclude ) {
	int ecnt = *_ecnt;
	if ( ( ecnt!=0 && ecnt+etot!=0 ) ||
		( cnt==0 && tot==0 ) ||
		( cnt!=0 && cnt+tot!=0 && ecnt==0 && etot==0 ) ) {
#if DEBUG
	    if ( needed!=NULL && !needed->spline->isunneeded )
		IError( c==1?
		    "A spline is both needed and unneeded in CountDuplicates#3 in %s":
		    "A set of tangent splines is both needed and unneeded in CountDuplicates#3 in %s",
		    sc!=NULL?sc->name:"<nameless char>");
#endif
	    for ( test=apt; test!=NULL && ExactlySame(apt,test); test=test->aenext )
		test->spline->isunneeded = true;
	} else {
#if DEBUG
	    if ( needed )
		/* Already done */;
	    else if ( notunneeded==NULL )
		IError( c==1?
		    "A spline is both needed and unneeded in CountDuplicates#4 in %s":
		    "A set of tangent splines is both needed and unneeded in CountDuplicates#4 in %s",
		    sc!=NULL?sc->name:"<nameless char>");
#endif
	    for ( test=apt; test!=NULL && ExactlySame(apt,test); test=test->aenext ) {
		test->spline->isunneeded = true;
		test->spline->isneeded = false;
	    }
	    apt->spline->isunneeded = false;
	    apt->spline->isneeded = true;
	}
	*_ecnt += etot;
    }
    *_cnt += tot;
return( test );
}

static void _FindNeeded(SplineChar *sc,EIList *el, int major) {
    EI *active=NULL, *apt, *e, *npt, *pr;
    int bpos=0, tpos=0, cnt, ecnt;
    double pos, npos;
    int other = !major, subchange;

    el->major = major;
    ELOrder(el,major);
    ELCarefullOrder(el,major);

    while ( el->bottoms[bpos]!=NULL || el->tops[tpos]!=NULL ) {
	/* Figure out the next point of interest */
	if ( el->tops[tpos]==NULL )
	    pos = el->bottoms[bpos]->coordmin[major];
	else if ( el->bottoms[bpos]==NULL )
	    pos = el->tops[tpos]->coordmax[major];
	else
	    pos = el->tops[tpos]->coordmax[major]<=el->bottoms[bpos]->coordmin[major] ?
		    el->tops[tpos]->coordmax[major] :
		    el->bottoms[bpos]->coordmin[major];
/* Much of this loop is a reworking of EIActiveEdgesRefigure, changed */
/*  to support a non-integral approach. This works because there should be */
/*  no crossings */
	/* Then remove anything that ends here */
	if ( el->tops[tpos]!=NULL && pos==el->tops[tpos]->coordmax[major] ) {
	    for ( pr=NULL, apt=active; apt!=NULL; apt = apt->aenext ) {
		if ( apt->coordmax[major]==pos ) {
		    if ( pr==NULL )
			active = apt->aenext;
		    else
			pr->aenext = apt->aenext;
		} else
		    pr = apt;
	    }
	    while ( el->tops[tpos]!=NULL && pos==el->tops[tpos]->coordmax[major])
		++tpos;
	}
	/* then move the active list to the next line */
	for ( apt=active; apt!=NULL; apt = apt->aenext ) {
	    Spline1D *osp = &apt->spline->splines[other];
	    apt->tcur = EITOfNextMajor(apt,el,pos);
	    apt->ocur = ( ((osp->a*apt->tcur+osp->b)*apt->tcur+osp->c)*apt->tcur + osp->d );
	}
	active = EIActiveListReorder(active,&subchange);
	if ( subchange )
	    IError("There should be no crossovers in _FindNeeded in %s", sc!=NULL?sc->name:"<nameless char>");
	while ( el->bottoms[bpos]!=NULL && pos==el->bottoms[bpos]->coordmin[major] ) {
	    Spline1D *osp;
	    npt = el->bottoms[bpos++];
	    osp = &npt->spline->splines[other];
	    npt->tcur = npt->up?0:1;
	    npt->ocur = ( ((osp->a*npt->tcur+osp->b)*npt->tcur+osp->c)*npt->tcur + osp->d );
	    for ( pr=NULL, apt=active; apt!=NULL && npt!=NULL; ) {
		if ( npt->ocur<apt->ocur ) {
		    npt->aenext = apt;
		    if ( pr==NULL )
			active = npt;
		    else
			pr->aenext = npt;
		    npt = NULL;
	    break;
		} else {
		    pr = apt;
		    apt = apt->aenext;
		}
	    }
	    if ( npt!=NULL ) {
		if ( pr==NULL )
		    active = npt;
		else
		    pr->aenext = npt;
		npt->aenext = NULL;
	    }
	}
/* End of EIActiveEdgesRefigure */
/* Now for the part of the routine which actually does something... */
	if ( el->tops[tpos]==NULL && el->bottoms[bpos]==NULL)
    break;
	if ( el->tops[tpos]==NULL )
	    npos = el->bottoms[bpos]->coordmin[major];
	else if ( el->bottoms[bpos]==NULL )
	    npos = el->tops[tpos]->coordmax[major];
	else
	    npos = el->tops[tpos]->coordmax[major]<=el->bottoms[bpos]->coordmin[major] ?
		    el->tops[tpos]->coordmax[major] :
		    el->bottoms[bpos]->coordmin[major];
	pos = (npos+pos)/2;
	for ( apt=active; apt!=NULL; apt = apt->aenext ) {
	    Spline1D *osp = &apt->spline->splines[other];
	    apt->tcur = EITOfNextMajor(apt,el,pos);
	    apt->ocur = ( ((osp->a*apt->tcur+osp->b)*apt->tcur+osp->c)*apt->tcur + osp->d );
	}
	active = EIActiveListReorder(active,&subchange);
	for ( apt=active; apt!=NULL; apt = e ) {
	    ecnt = cnt = 0;
	    e = CountDuplicates(sc,el->ot,apt,&cnt,&ecnt);
	    for ( ; e!=NULL && (cnt!=0 || ecnt!=0); ) {
		e = CountDuplicates(sc,el->ot,e,&cnt,&ecnt);
	    }
	}
    }
    free(el->ordered);
    free(el->ends);
    free(el->bottoms);
    free(el->tops);
}

static void SSExcludeSet(SplineSet *base) {
    int selected;
    SplinePoint *sp;
    Spline *s, *first;

    while ( base!=NULL ) {
	selected = false;
	for ( sp=base->first ; ; ) {
	    if ( sp->selected ) {
		selected = true;
	break;
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==base->first )
	break;
	}
	first = NULL;
	for ( s = base->first->next; s!=NULL && s!=first ; s = s->to->next ) {
	    if ( first==NULL ) first = s;
	    s->exclude = selected;
	}
	base = base->next;
    }
}

static void SplineSetFindNeeded(SplineChar *realsc,SplineSet *base,enum overlap_type ot) {
    EIList el;
    SplineChar sc;
#ifdef FONTFORGE_CONFIG_TYPE3
    Layer layers[2];
#endif

    memset(&sc,'\0',sizeof(sc));
#ifdef FONTFORGE_CONFIG_TYPE3
    memset(layers,0,sizeof(layers));
    sc.layers = layers;
#endif
    sc.layer_cnt = 2;
    sc.layers[ly_fore].splines = base;
    sc.name = realsc->name;

    memset(&el,'\0',sizeof(el));
    el.leavetiny = true;
    ELFindEdges(&sc, &el);
    el.ot = ot;
    if ( ot==over_exclude )
	SSExcludeSet(base);

    _FindNeeded(realsc,&el,1);

    /* Have to check both directions, else we lose horizontal line intersections */
    _FindNeeded(realsc,&el,0);

    ElFreeEI(&el);
}

/* Ok. If we're going to refigure things we've got to make sure that each */
/*  spline segment at the intersection is disconnected from whatever spline */
/*  segment it was originally connected to. As we do that we have to create */
/*  new (temporary) end points for one of them (since the spline point is */
/*  where the connection happens) */
/* It's also possible to have a spline where both end points are at the */
/*  intersection (figure 8 again) */
static void ILDisconnect(IntersectionList *ilist) {
    SplineList *sl;
    SplinePoint *sp;

    while ( ilist!=NULL ) {
	for ( sl = ilist->splines; sl!=NULL; sl=sl->next ) {
	    if ( sl->spline->from->me.x == ilist->intersection.x &&
		    sl->spline->from->me.y == ilist->intersection.y ) {
		if ( sl->spline->from->prev!=sl->spline && sl->spline->from->prev != NULL ) {
		    sp = chunkalloc(sizeof(SplinePoint));
		    *sp = *sl->spline->from;
		    sp->hintmask = NULL;
		    sl->spline->from->prev = NULL;
		    sp->next = NULL;
		    sp->prev->to = sp;
		}
	    }
	    if ( sl->spline->to->me.x == ilist->intersection.x &&
		    sl->spline->to->me.y == ilist->intersection.y ) {
		if ( sl->spline->to->next!=sl->spline && sl->spline->to->next != NULL ) {
		    sp = chunkalloc(sizeof(SplinePoint));
		    *sp = *sl->spline->to;
		    sp->hintmask = NULL;
		    sl->spline->to->next = NULL;
		    sp->prev = NULL;
		    sp->next->from = sp;
		}
	    }
	}
	ilist = ilist->next;
    }
}

static void SplinesMergeLists(SplineChar *sc,Spline *before, Spline *after) {

    if ( !RealNearish(before->to->me.x,after->from->me.x) ||
	    !RealNearish(before->to->me.y,after->from->me.y) )
	IError("Attempt to merge two splines which don't meet in %s",
		sc!=NULL?sc->name:"<nameless char>");
    if ( before->to->next!=NULL || after->from->prev!=NULL )
	IError("Attempt to merge two splines which are already attached to stuff in %s",
		sc!=NULL?sc->name:"<nameless char>");
    before->to->nextcp = after->from->nextcp;
    before->to->nonextcp = after->from->nonextcp;
    before->to->nextcpdef = after->from->nextcpdef;
    before->to->next = after;
    SplinePointFree(after->from);
    after->from = before->to;
    SplinePointCatagorize(before->to);
}

static SplineSet *SplineSetCreate(SplineChar *sc,SplinePoint *from, SplinePoint *to) {
    SplineSet *spl;

    if ( from->prev!=NULL || to->next!=NULL )
	IError("Attempt to create a splineset from two points not at end in %s",
		sc!=NULL?sc->name:"<nameless char>");
    if ( from->me.x != to->me.x || from->me.y!=to->me.y )
	IError("Attempt to create a splineset from two points which aren't at the same place in %s",
		sc!=NULL?sc->name:"<nameless char>");
    if ( /*from->next->isticked ||*/ !from->next->isneeded || /*to->prev->isticked ||*/
	    !to->prev->isneeded )
	IError("Bad choice of splines in SplineSetCreate in %s",
		sc!=NULL?sc->name:"<nameless char>");
    from->next->isticked = to->prev->isticked = true;
    SplinesMergeLists(sc,to->prev,from->next);
    spl = chunkalloc(sizeof(SplineSet));
    spl->first = spl->last = to;
return( spl );
}

/* We're about to free this spline, but it connects to some intersection */
/*  so we've got to remove it from the intersection first */
static void ILRemoveSplineFrom(SplineChar *sc,IntersectionList *il,BasePoint *ival,Spline *spline) {
    SplineList *sl, *slprev;

    while ( il!=NULL && (il->intersection.x!=ival->x || il->intersection.y!=ival->y))
	il = il->next;
    if ( il==NULL )
	IError("Couldn't find intersection in ILRemoveSplineFrom in %s",
		sc!=NULL?sc->name:"<nameless char>");
    else {
	slprev = NULL;
	for ( sl = il->splines; sl!=NULL && sl->spline!=spline; slprev = sl, sl = sl->next );
	if ( sl==NULL )
	    IError("Couldn't find spline in ILRemoveSplineFrom in %s",
		    sc!=NULL?sc->name:"<nameless char>");
	else {
	    if ( slprev==NULL )
		il->splines = sl->next;
	    else
		slprev->next = sl->next;
	    free(sl);
	}
    }
}

static void SplineListFree(SplineChar *sc,SplineList *sl,IntersectionList *ilist) {
    Spline *spline, *snext;

    if ( sl->spline->from->me.x==ilist->intersection.x &&
	    sl->spline->from->me.y==ilist->intersection.y ) {
	SplinePointFree(sl->spline->from);
	for ( spline = sl->spline; spline !=NULL; spline = snext ) {
	    snext = spline->to->next;
	    if ( spline->isneeded || !spline->isunneeded )
		IError("Spline which is needed (or not unneeded) when about to be freed in %s",
			sc!=NULL?sc->name:"<nameless char>");
	    if ( spline->to->isintersection )
		ILRemoveSplineFrom(sc,ilist,&spline->to->me,spline);
	    SplinePointFree(spline->to);
	    SplineFree(spline);
	}
    } else if ( sl->spline->to->me.x==ilist->intersection.x &&
		sl->spline->to->me.y==ilist->intersection.y ) {
	SplinePointFree(sl->spline->to);
	for ( spline = sl->spline; spline !=NULL; spline = snext ) {
	    snext = spline->from->prev;
	    if ( spline->isneeded || !spline->isunneeded )
		IError("Spline which is needed (or not unneeded) when about to be freed in %s",
			sc!=NULL?sc->name:"<nameless char>");
	    if ( spline->from->isintersection )
		ILRemoveSplineFrom(sc,ilist,&spline->from->me,spline);
	    SplinePointFree(spline->from);
	    SplineFree(spline);
	}
    } else
	IError( "Couldn't identify intersection in SplineListFree in %s",
		sc!=NULL?sc->name:"<nameless char>");
    free(sl);
}

static void ILFreeUnusedSplines(SplineChar *sc,IntersectionList *ilist) {
    SplineList *sl, *prev, *snext;

    while ( ilist!=NULL ) {
	prev = NULL;
	for ( sl=ilist->splines; sl!=NULL; sl=snext ) {
	    snext = sl->next;
	    if ( sl->spline->isunneeded ) {
		if ( sl->spline->isneeded )
		    IError("Spline which is both needed and unneeded in ILFreeUnusedSplines in %s",
			    sc!=NULL?sc->name:"<nameless char>");
		if ( prev==NULL )
		    ilist->splines = snext;
		else
		    prev->next = snext;
		SplineListFree(sc,sl,ilist);	/* This might free snext */
		snext = (prev==NULL)?ilist->splines:prev->next;
	    } else {
		if ( !sl->spline->isneeded )
		    IError("Spline which is neither needed nor unneeded in ILFreeUnusedSplines in %s",
			    sc!=NULL?sc->name:"<nameless char>");
		prev = sl;
	    }
	}
	if ( ilist->splines==NULL ) ilist->processed = true;
	ilist = ilist->next;
    }
}

static void ReverseSplines(Spline *last) {
    Spline *first;
    SplinePointList spl;

    spl.last = last->to;
    for ( first=last; first->from->prev!=NULL ; first = first->from->prev );
    spl.first = first->from;
    SplineSetReverse(&spl);
}

static int SimpleReturn(Spline *spline, IntersectionList *curpos, IntersectionList *start) {
    if ( spline->from->me.x==curpos->intersection.x && spline->from->me.y==curpos->intersection.y ) {
	while ( spline->to->next!=NULL ) spline = spline->to->next;
return( spline->to->me.x==start->intersection.x && spline->to->me.y==start->intersection.y );
    } else {
	while ( spline->from->prev!=NULL ) spline = spline->from->prev;
return( spline->from->me.x==start->intersection.x && spline->from->me.y==start->intersection.y );
    }
}

static SplineSet *ILRemoveTwoTrack(SplineChar *sc,IntersectionList *inter, IntersectionList *ilist) {
    Spline *good1=NULL, *good2=NULL, *cur, *last;
    SplineList *sl;
    IntersectionList *il;

    for ( sl=inter->splines; sl!=NULL; sl=sl->next ) {
	if ( sl->spline->isneeded && !sl->spline->isticked ) {
	    if ( good1==NULL ) {
		good1 = sl->spline;
		if ( good1->from->me.x == good1->to->me.x && good1->from->me.y==good1->to->me.y ) {
		    /* this spline stops and ends at the intersection point */
		    /* so makes a complete loop by itself */
return( SplineSetCreate(sc,good1->from,good1->to));
		}
	    } else {
		good2 = sl->spline;
    break;
	    }
	}
    }
    if ( good1==NULL )
return( NULL );
    else if ( good2==NULL ) {
	IError( "Single needed spline at an intersection in %s",
		sc!=NULL?sc->name:"<nameless char>");
return( NULL );
    }
    good1->isticked = true; /*good2->isticked=true;*/
    if ( good1->to->me.x==inter->intersection.x && good1->to->me.y==inter->intersection.y )
	ReverseSplines(good1);
    cur = good1;
    while ( 1 ) {
	for ( last=cur; last->to->next!=NULL; last=last->to->next) {
	    if ( last->isunneeded || !last->isneeded )
		IError( "Spline unneeded (or not needed) when it should have been in %s",
			sc!=NULL?sc->name:"<nameless char>");
	}
	last->isticked = true;
	if ( last->to->me.x == inter->intersection.x && last->to->me.y == inter->intersection.y ) {
	    /* Done, we've looped back, might not be to good2 though. That doesn't */
	    /*  matter the point (before) was to make sure there was somewhere */
	    /*  return to */
return( SplineSetCreate(sc,good1->from,last->to));
	}
	for ( il = ilist; il!=NULL; il=il->next ) {
	    if ( last->to->me.x == il->intersection.x && last->to->me.y == il->intersection.y )
	break;
	}
	cur = NULL;
	if ( il!=NULL ) {
	    for ( sl=il->splines; sl!=NULL; sl=sl->next ) {
		/* if there is a splineset which returns to our start without */
		/*  going through other intersections then pick it */
		if ( sl->spline->isneeded && !sl->spline->isticked &&
			SimpleReturn(sl->spline,il,inter)) {
		    cur = sl->spline;
	    break;
		}
	    }
	    if ( cur==NULL ) {
		/* otherwise just pick anything. It'll return eventually */
		for ( sl=il->splines; sl!=NULL; sl=sl->next ) {
		    if ( sl->spline->isneeded && !sl->spline->isticked ) {
			cur = sl->spline;
		break;
		    }
		}
	    }
	}
	if ( cur==NULL ) {
	    SplinePointList *spl;
	    IError("Found an intersection with no exit in %s",
		    sc!=NULL?sc->name:"<nameless char>");
	    spl = chunkalloc(sizeof(SplinePointList));
	    spl->first = good1->from; spl->last = last->to;
return( spl );
	}
	cur->isticked = true;
	if ( RealNearish(cur->to->me.x,last->to->me.x) &&
		RealNearish(cur->to->me.y,last->to->me.y) )
	    ReverseSplines(cur);
	SplinesMergeLists(sc,last,cur);
    }
}

static SplineSet *SSRebuild(SplineChar *sc,IntersectionList *ilist) {
    SplineSet *head=NULL, *last=NULL, *cur;
    IntersectionList *il;
    SplineList *sl;

    /* Look for any simple closed loops first. Not really needed, but I think */
    /* results are better if we do this */
    for ( il=ilist; il!=NULL; il=il->next ) {
	for ( sl=il->splines; sl!=NULL; sl=sl->next ) {
	    if ( sl->spline->isneeded && !sl->spline->isticked ) {
		Spline *s = sl->spline;
		if ( s->from->me.x == s->to->me.x && s->from->me.y==s->to->me.y ) {
		    cur = SplineSetCreate(sc,s->from,s->to);
		    if ( head==NULL )
			head=cur;
		    else
			last->next = cur;
		    last = cur;
		}
	    }
	}
    }

    while ( ilist!=NULL ) {
	if ( !ilist->processed ) {
	    /* An intersection may have no needed splines at it (entirely internal) */
	    /* In almost all cases an intersection will only have two splines */
	    /* rarely we will get a self-intersecting spline (figure 8) where */
	    /*  we have more than 2 needed paths. There better be an even number */
	    /*  I don't think it matters which ones we pick */
	    while ( (cur=ILRemoveTwoTrack(sc,ilist,ilist->next))) {
		if ( head==NULL )
		    head = cur;
		else
		    last->next = cur;
		last = cur;
	    }
	    ilist->processed = true;
	}
	ilist = ilist->next;
    }
return( head );
}

static void IListCheck(SplinePoint *sp,IntersectionList *ilist) {
    /* We're freeing the splineset that this point is on */
    /*  but it happens to be one of our intersection points, so we need */
    /*  the intersection to know that it's just lost the two splines attached */
    /*  to the point */
    SplineList *sl, *snext, *prev;

    while ( ilist!=NULL ) {
	if ( ilist->intersection.x==sp->me.x && ilist->intersection.y==sp->me.y ) {
	    prev = NULL;
	    for ( sl = ilist->splines; sl!=NULL; sl = snext ) {
		snext = sl->next;
		if ( sl->spline==sp->prev || sl->spline==sp->next ) {
		    if ( prev==NULL )
			ilist->splines = snext;
		    else
			prev->next = snext;
		    free(sl);
		} else
		    prev = sl;
	    }
    break;
	}
	ilist = ilist->next;
    }
}

/* If there are any splineset where all the splines are marked unneeded then */
/*  we can just free them now and reduce complexity later */
static SplineSet *SSRemoveAllUnneeded(SplineChar *sc,
	SplineSet *base, IntersectionList *ilist) {
    SplineSet *spl, *prev, *snext;
    Spline *spline, *first;
    int allunneeded;
    SplinePoint *sp, *firstsp;

    for ( spl = base, prev=NULL; spl!=NULL; spl = snext ) if ( spl->first->prev!=NULL ) {
	snext = spl->next;
	first = NULL;
	allunneeded = true;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	    if ( !spline->isunneeded ) {
		allunneeded = false;
	break;
	    }
	    if ( spline->isneeded )
		IError("Spline both needed and unneeded in SSRemoveAllUnneeded in %s",
			sc!=NULL?sc->name:"<nameless char>");
	    if ( first==NULL ) first=spline;
	}
	if ( allunneeded ) {
	    firstsp = NULL;
	    for ( sp = spl->first; sp!=NULL && sp!=firstsp; sp=sp->next->to ) {
		if ( sp->isintersection )
		    IListCheck(sp,ilist);
		if ( firstsp==NULL ) firstsp = sp;
	    }
	    if ( prev==NULL )
		base = snext;
	    else
		prev->next = snext;
	    spl->next = NULL;
	    SplinePointListFree(spl);
	} else
	    prev = spl;
    }
return( base );
}

static SplineSet *SSRemoveAllNeeded(SplineChar *sc,SplineSet **base, IntersectionList *ilist) {
    SplineSet *spl, *prev, *snext, *head=NULL, *last=NULL;
    Spline *spline, *first;
    int allneeded;

    for ( spl = *base, prev=NULL; spl!=NULL; spl = snext ) {
	snext = spl->next;
	if ( spl->first->prev!=NULL ) {
	    first = NULL;
	    allneeded = !spl->first->isintersection;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
		if ( !spline->isneeded || spline->to->isintersection ) {
		    allneeded = false;
	    break;
		}
		if ( first==NULL ) first=spline;
	    }
	    if ( allneeded ) {
		if ( prev==NULL )
		    *base = snext;
		else
		    prev->next = snext;
		spl->next = NULL;
		if ( head==NULL )
		    head = spl;
		else
		    last->next = spl;
		last = spl;
		for ( spline = spl->first->next; spline!=NULL && !spline->isticked; spline = spline->to->next ) {
		    spline->isticked = true;
		    if ( spline->to->isintersection )
			IError("Spline in a fully-needed splineset has an intersection in SSRemoveAllNeeded in %s",
				sc!=NULL?sc->name:"<nameless char>");
		}
	    } else
		prev = spl;
	}
    }
return( head );
}

/* We rotated the character's splineset so that the current spline at the */
/*  current point is parallel to the x-axis. Then we are only interested in */
/*  splines which intersect the vertical line through the current point */

static int IsInteresting(Spline *s,double tmin,double tmax,BasePoint *me) {
    Spline1D *s1 = &s->splines[0];
    double xtmin = ((s1->a*tmin+s1->b)*tmin+s1->c)*tmin+s1->d;
    double xtmax = ((s1->a*tmax+s1->b)*tmax+s1->c)*tmax+s1->d;
    double t, test;

    if (( xtmin<me->x && xtmax<me->x ) || ( xtmin>me->x && xtmax>me->x ))
return( false );
    if ( xtmin!=me->x && xtmax!=me->x )
return( true );
    if ( xtmin==me->x ) {
	if ( tmin>0 ) {
	    if ( (t = tmin-.0001)<0 ) t=0;
	} else {
	    s1 = &s->from->prev->splines[0];
	    t = .9999;
	}
	test = ((s1->a*t+s1->b)*t+s1->c)*t+s1->d;
	if (( test<xtmin && xtmax<xtmin ) || ( test>xtmin && xtmax>xtmin ))
return( false );		/* Point of inflection */
    } else {
	if ( tmax<1 ) {
	    if ( (t = tmax+.0001)>1 ) t=1;
	} else {
	    s1 = &s->to->next->splines[0];
	    t = .0001;
	}
	if (( test<xtmax && xtmin<xtmax ) || ( test>xtmax && xtmin>xtmax ))
return( false );		/* Point of inflection */
    }
return( true );
}

static int SameSplines(Spline *s1,Spline *s2) {
    if ( s1==s2 )
return( true );
    if ( s1==NULL || s2==NULL )
return( false );
    if ( s1->from->me.x==s2->from->me.x && s1->from->me.y==s2->from->me.y &&
	    s1->from->nextcp.x==s2->from->nextcp.x && s1->from->nextcp.y==s2->from->nextcp.y &&
	    s1->to->prevcp.x==s2->to->prevcp.x && s1->to->prevcp.y==s2->to->prevcp.y &&
	    s1->to->me.x==s2->to->me.x && s1->to->me.y==s2->to->me.y )
return( true );

return( false );
}

static int CountCrossings(SplineSet *spl,BasePoint *pt,double me_t, Spline *exceptme) {
    Spline *s, *first;
    double ts[4], t, y, xmin, xmax;
    int i, bcnt;

    ts[0] = 0;
    bcnt = 0;
    while ( spl!=NULL ) {
	if ( spl->first->prev!=NULL ) {
	    first = NULL;
	    for ( s=spl->first->next ; s!=first ; s = s->to->next ) if ( !SameSplines(s,exceptme) ) {
		SplineFindExtrema(&s->splines[0],&ts[1],&ts[2]);
		if ( ts[1]==-1 ) ts[1] = 1;
		else if ( ts[2]==-1 ) ts[2] = 1;
		else ts[3] = 1;
		for ( i=0; ts[i]!=1; ++i ) {
		    if ( IsInteresting(s,ts[i], ts[i+1], pt)) {
			t = SplineSolve(&s->splines[0],ts[i],ts[i+1], pt->x, .0001);
			if ( t==-1 )		/* can't happen */
return( -1 );
			if ( t==ts[i] || t==ts[i+1] )	/* Things get too complicated at the ends of splines */
return( -1 );
			y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
			if ( y==pt->y )
			    /* Ignore anything coincident */;
			else if ( y<pt->y ) {
			    xmin = ((s->splines[0].a*ts[i]+s->splines[0].b)*ts[i]+s->splines[0].c)*ts[i]+s->splines[0].d;
			    xmax = ((s->splines[0].a*ts[i+1]+s->splines[0].b)*ts[i+1]+s->splines[0].c)*ts[i+1]+s->splines[0].d;
			    if ( xmax>xmin )
				++bcnt;
			    else
				--bcnt;
			}
		    }
		}
		if ( first==NULL )
		    first = s;
	    }
	}
	spl = spl->next;
    }
    if ( bcnt!=0 && exceptme!=NULL ) {
	ts[0] = me_t-.01; ts[1] = me_t+.01;
	s = exceptme;
	xmin = ((s->splines[0].a*ts[0]+s->splines[0].b)*ts[0]+s->splines[0].c)*ts[0]+s->splines[0].d;
	xmax = ((s->splines[0].a*ts[1]+s->splines[0].b)*ts[1]+s->splines[0].c)*ts[1]+s->splines[0].d;
	if ( xmax>xmin )
	    ++bcnt;
	else
	    --bcnt;
    }
return( bcnt<0 ? -bcnt : bcnt );
}

static Spline *FindMe(SplineSet *spl,real trans[6],Spline *s) {
    SplinePoint to, from;
    Spline *test, *first;

    to.me.x = rint(1024*(s->to->me.x*trans[0] + s->to->me.y*trans[2]))/1024;
    to.me.y = rint(1024*(s->to->me.x*trans[1] + s->to->me.y*trans[3]))/1024;
    to.prevcp.x = rint(1024*(s->to->prevcp.x*trans[0] + s->to->prevcp.y*trans[2]))/1024;
    to.prevcp.y = rint(1024*(s->to->prevcp.x*trans[1] + s->to->prevcp.y*trans[3]))/1024;
    from.me.x = rint(1024*(s->from->me.x*trans[0] + s->from->me.y*trans[2]))/1024;
    from.me.y = rint(1024*(s->from->me.x*trans[1] + s->from->me.y*trans[3]))/1024;
    from.nextcp.x = rint(1024*(s->from->nextcp.x*trans[0] + s->from->nextcp.y*trans[2]))/1024;
    from.nextcp.y = rint(1024*(s->from->nextcp.x*trans[1] + s->from->nextcp.y*trans[3]))/1024;

    while ( spl!=NULL ) {
	if ( spl->first->prev!=NULL ) {
	    first = NULL;
	    for ( test = spl->first->next; test!=first ; test=test->to->next ) {
		if ( first==NULL ) first = test;
		if ( RealNear(test->from->me.x,from.me.x) &&
			RealNear(test->from->me.y,from.me.y) &&
			RealNear(test->from->nextcp.x,from.nextcp.x) &&
			RealNear(test->from->nextcp.y,from.nextcp.y) &&
			RealNear(test->to->prevcp.x,to.prevcp.x) &&
			RealNear(test->to->prevcp.y,to.prevcp.y) &&
			RealNear(test->to->me.x,to.me.x) &&
			RealNear(test->to->me.y,to.me.y) )
return( test );
	    }
	}
	spl = spl->next;
    }
    IError("Failed to find transformed spline" );
return( NULL );
}

static int IsExternalEdge(Spline *s,double t,SplineSet *base ) {
    SplineSet *altered=NULL;
    double theta;
    double slopex, slopey;
    BasePoint here, rpt;
    int cnt;
    real trans[6];

    slopex = (3*s->splines[0].a*t+2*s->splines[0].b)*t+s->splines[0].c;
    slopey = (3*s->splines[1].a*t+2*s->splines[1].b)*t+s->splines[1].c;
    here.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
    here.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
    if ( slopex==0 && slopey==0 )
return( -1 );
    theta = atan2(slopey,slopex);

    trans[0] = trans[3] = cos(theta);
    trans[2] = sin(theta);
    trans[1] = -trans[2];
    trans[4] = trans[5] = 0;
    altered = SplinePointListTransform(SplinePointListCopy(base),trans,true);
    rpt.x = rint(1024*(here.x*trans[0] + here.y*trans[2]))/1024;
    rpt.y = rint(1024*(here.x*trans[1] + here.y*trans[3]))/1024;
    cnt = CountCrossings(altered,&rpt,t,FindMe(altered,trans,s));
    SplinePointListsFree(altered);
return( cnt==-1 ? -1 : cnt==0 );
}

static void ForceAllnu(SplinePoint *csp,SplinePoint *nsp,int isneeded) {
    Spline *s;

    for ( s=csp->next ; ; s = s->to->next ) {
	s->isneeded = isneeded;
	s->isunneeded = !isneeded;
	if ( s->to == nsp )
    break;
    }
}

static int PathIsGood(Spline *s) {
    /* Are all splines on this path either needed or unneeded, but never both? */
    int isn = s->isneeded;

    forever {
	if ( s->isneeded && s->isunneeded )
return( false );
	if ( !s->isneeded && !s->isunneeded )
return( false );
	if ( s->isneeded!=isn )
return( false );
	if ( s->to->isintersection )
return( true );
	s = s->to->next;
    }
}

static int CountGoodNeededPaths(IntersectionList *il,Spline *ignoreme) {
    int cnt=0;
    SplineList *l;

    for ( l=il->splines; l!=NULL; l=l->next ) if ( l->spline!=ignoreme ) {
	if ( !PathIsGood(l->spline))
return( -1 );
	else if ( l->spline->isneeded )
	    ++cnt;
    }
return( cnt );
}

static int AllIntersectionsEven(IntersectionList *ilist) {
    IntersectionList *il;
    int cnt;

    for ( il=ilist; il!=NULL; il=il->next ) {
	cnt = CountGoodNeededPaths(il,NULL);
	if ( cnt==-1 || (cnt&1))
return( false );
    }
return( true );
}

static void ForceAllSame(SplinePoint *csp,SplinePoint *nsp,SplineChar *sc,
	SplineSet * base, IntersectionList *ilist) {
    int ncnt, ucnt;
    int cntc, cntn;
    Spline *s;
    IntersectionList *cil, *nil;
    int i, div;

    ncnt = ucnt = 0;
    for ( s=csp->next ; ; s = s->to->next ) {
	if ( s->isneeded ) ++ncnt;
	if ( s->isunneeded ) ++ucnt;
	if ( s->to == nsp )
    break;
    }
    if ( (ncnt==0 || ucnt==0 ) && ncnt!=ucnt )
return;
    if ( csp->isintersection ) {	/* This might be false if we are dealing with an entire contour */
	cil = FindIntersectionAt(ilist,csp->me.x,csp->me.y);
	nil = FindIntersectionAt(ilist,nsp->me.x,nsp->me.y);
	if ( cil==NULL || nil==NULL )
	    IError("Failed to find intersection");
	else if ( cil==nil ) {
	    /* Can't guess whether this path is needed or unneeded. Since */
	    /*  it starts and ends at the same point, either way will work */
	} else {
	    cntc = CountGoodNeededPaths(cil,csp->next);
	    cntn = CountGoodNeededPaths(nil,nsp->prev);
	    if ( cntc==-1 || cntc==-1 )
		/* Can't tell */;
	    else if ( (cntc&1)==0 && (cntn&1)==0 )
		ForceAllnu(csp,nsp,0);		/* Both intersections have an */
		    /* even number of paths (ignoring this one) so adding this*/
		    /* one would make an uneven number, which would be wrong */
	    else if ( (cntc&1)==1 && (cntn&1)==1 )
		ForceAllnu(csp,nsp,1);
	}
    }

    for ( div=3; div<20; div += 2 ) {
	ncnt = ucnt = 0;
	for ( s=csp->next ; ; s = s->to->next ) {
	    for ( i=1; i<=div; ++i ) {
		int isee = IsExternalEdge(s,i/((double) (div+1)),base);
		if ( isee==-1 )
      goto tryagain;
		if ( isee )
		    ++ncnt;
		else
		    ++ucnt;
	    }
	    if ( s->to == nsp )
	break;
	}
	if ( ncnt>=2*ucnt ) {
	    ForceAllnu(csp,nsp,1);
return;
	} else if ( ucnt>=2*ncnt ) {
	    ForceAllnu(csp,nsp,0);
return;
	}
      tryagain:;
    }
    IError("Couldn't guess needed/unneeded in ForceAllSame");
}

static void RefigureNeeded(SplineSet *base) {
    int i;
    SplineSet *spl;
    Spline *s, *first;

    for ( spl=base; spl!=NULL; spl=spl->next ) if ( spl->first->prev!=NULL ) {
	first = NULL;
	for ( s = spl->first->next; s!=first; s = s->to->next ) {
	    if ( first==NULL ) first = s;
	    for ( i=1; i<5; ++i ) {
		int isee = IsExternalEdge(s,i/5.,base);
		if ( isee==1 )
		    s->isneeded = true;
		else if ( isee==0 )
		    s->isunneeded = true;
	    }
	}
    }
}

static int AllSame(SplinePoint *csp,SplinePoint *nsp) {
    int ncnt, ucnt;
    Spline *s;

    ncnt = ucnt = 0;
    for ( s=csp->next ; ; s = s->to->next ) {
	if ( !s->isneeded && !s->isunneeded )
return( false );
	if ( s->isneeded ) ++ncnt;
	if ( s->isunneeded ) ++ucnt;
	if ( s->to == nsp )
    break;
    }
return(ncnt==0 || ucnt==0 );
}

static void CleanupNeededUn(SplineChar *sc,SplineSet *base, IntersectionList *ilist) {
    SplineSet *spl;
    SplinePoint *sp, *csp, *nsp;
    int i, anyproblems;
    /* First check if we have any inconsistant paths (both needed & unneeded) */
    /*  If not, return */
    /*  If so, do a slower method of calculating the needed bits, and then */
    /*   check the conflicting paths more closely */
    /* (we refigure all the needed bits, not just the conflicting ones because*/
    /*  we sometimes miss a conflict) */

    for ( i=0; i<2; ++i ) {
	for ( spl=base; spl!=NULL; spl = spl->next ) if ( spl->first->prev!=NULL ) {
	    for ( sp = spl->first; !sp->isintersection ; ) {
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	    for ( csp = sp; ; ) {
		for ( nsp = csp->next->to; !nsp->isintersection && nsp!=sp; nsp = nsp->next->to );
		if ( i==0 ) {
		    anyproblems = !AllSame(csp,nsp);
		    if ( anyproblems )
      goto break_out;
		} else {
		    ForceAllSame(csp,nsp,sc,base,ilist);
		}
		if ( nsp==sp )
	    break;
		csp = nsp;
	    }
	}
      break_out:
	if ( i==0 ) {
	    if ( !anyproblems && AllIntersectionsEven(ilist))
return;
	    RefigureNeeded(base);
	}
    }
}

static void SSValidate(SplineChar *sc,SplineSet *spl) {
    Spline *spline, *first;
    
    for ( ;spl!=NULL; spl=spl->next ) if ( spl->first->prev!=NULL ) {
	first = NULL;
	for ( spline=spl->first->next; spline!=first && spline!=NULL; spline = spline->to->next ) {
	    if ( spline->isneeded ^ spline->isunneeded )
		/* Exactly one is set, that's good */;
	    else {
		if ( spline->isneeded )
		    IError( "Spline is both needed and unneeded in SSValidate\n(%g,%g)->(%g,%g) in %s",
			    spline->from->me.x, spline->from->me.y,
			    spline->to->me.x, spline->to->me.y,
			    sc!=NULL?sc->name:"<nameless char>");
		else
		    IError( "Spline is neither needed nor unneeded in SSValidate\n(%g,%g)->(%g,%g) in %s",
			    spline->from->me.x, spline->from->me.y,
			    spline->to->me.x, spline->to->me.y,
			    sc!=NULL?sc->name:"<nameless char>");
		/*spline->isunneeded = !spline->isunneeded;*/
	    }
	    if ( first==NULL ) first = spline;
	}
    }
}

static void ILFree(IntersectionList *il) {
    IntersectionList *inext;

    while ( il!=NULL ) {
	inext = il->next;
	free(il);
	il = inext;
    }
}

/* Now it is possible that we might have two splines that go between the same */
/*  two points (example: a horizontal bar and a vertical bar where the top of */
/*  the hbar is coincident with the top of the vbar. then that section will be*/
/*  represented by two splines). If this happens then findneeded can get all  */
/*  confused and may find one first when going horizontally, the other first  */
/*  when going vertically and mark both as needed and unneeded. That's bad. So*/
/*  here we go looking for duplicate splines and cleaning them up so that only*/
/*  one is needed and all others are unneeded */
static void RemoveDuplicates(IntersectionList *ilist) {
    SplineList *sl, *sl2;
    while ( ilist!=NULL ) {
	for ( sl=ilist->splines; sl!=NULL; sl=sl->next ) {
	    for ( sl2=sl->next; sl2!=NULL; sl2=sl2->next ) if ( sl2->spline!=sl->spline ) {
		if ( sl->spline->splines[0].a==sl2->spline->splines[0].a &&
			sl->spline->splines[0].b==sl2->spline->splines[0].b &&
			sl->spline->splines[0].c==sl2->spline->splines[0].c &&
			sl->spline->splines[0].d==sl2->spline->splines[0].d &&
			sl->spline->splines[1].a==sl2->spline->splines[1].a &&
			sl->spline->splines[1].b==sl2->spline->splines[1].b &&
			sl->spline->splines[1].c==sl2->spline->splines[1].c &&
			sl->spline->splines[1].d==sl2->spline->splines[1].d ) {
		    if ( sl->spline->isneeded || sl2->spline->isneeded ) {
			sl->spline->isneeded = true;
			sl2->spline->isneeded = false;
			sl->spline->isunneeded = false;
			sl2->spline->isunneeded = true;
		    }
		}
	    }
	}
	ilist = ilist->next;
    }
}

/* We will also, occasionally, get a little spur stiking out with 0 width */
/*  where a spline runs from an intersection, A, to point B, and then back */
/*  from B to A */
static SplineSet *RemoveBacktracks(SplineChar *sc,SplineSet *base, IntersectionList *ilist) {
    SplineSet *spl, *prev, *next;
    Spline *s, *first, *n;
    SplinePoint *f, *t, *m;

    prev = NULL;
    for ( spl=base, first=NULL; spl!=NULL; spl=next ) {
	next = spl->next;
	first = NULL;
	for ( s=spl->first->next; s!=NULL && s!=first; s=n ) {
	    if ( first==NULL ) first=s;
	    n = s->to->next;
	    f = s->from; t = n->to; m = s->to;
	    if ( n!=s && f->me.x==t->me.x && f->me.y==t->me.y &&
		    f->nextcp.x==t->nextcp.x && f->nextcp.y==t->prevcp.y &&
		    m->nextcp.x==m->prevcp.x && m->nextcp.y==m->prevcp.y ) {
		if ( m->isintersection ) {
		    ILRemoveSplineFrom(sc,ilist,&m->me,s);
		    ILRemoveSplineFrom(sc,ilist,&m->me,n);
		}
		if ( f->isintersection )
		    ILRemoveSplineFrom(sc,ilist,&f->me,s);
		if ( t->isintersection )
		    ILRemoveSplineFrom(sc,ilist,&t->me,n);
		if ( f==t ) {
		    SplinePointListFree(spl);
		    if ( prev==NULL )
			base = next;
		    else
			prev->next = next;
		    spl=NULL;
	break;
		}
		SplinePointFree(m);
		n=n->to->next;
		if ( first==s || first==s->to->next ) first = NULL;
		SplineFree(s->to->next);
		SplineFree(s);
		n->from = f;
		f->nextcp = t->nextcp;
		f->next = n;
		SplinePointFree(t);
		if ( spl->first==t || spl->first==m ) spl->first = f;
		if ( spl->last==t || spl->last==m ) spl->last = f;
		first = NULL;	/* If we have a spur of two segments, we might need to go through the list again */
	    } else if ( f->me.x==m->me.x && f->me.y==m->me.y &&
		    f->nonextcp && m->noprevcp ) {
		/* Remove zero length splines too */
		if ( s==first ) first = NULL;
		f->next = n;
		f->nextcp = m->nextcp;
		f->nonextcp = m->nonextcp;
		f->nextcpdef = m->nextcpdef;
		n->from = f;
		if ( f->isintersection )
		    ILRemoveSplineFrom(sc,ilist,&f->me,s);
		if ( f==m ) {
		    SplinePointListFree(spl);
		    if ( prev==NULL )
			base = next;
		    else
			prev->next = next;
		    spl=NULL;
	break;
		}
		SplineFree(s);
		SplinePointFree(m);
		if ( m==spl->first ) spl->first = f;
		if ( m==spl->last ) spl->last = f;
	    }
	}
	if ( spl!=NULL )
	    prev = spl;
    }
return(base);
}

#ifdef DEBUG
static void ShowIntersections(IntersectionList *ilist) {
 IntersectionList *il; for ( il=ilist; il!=NULL; il=il->next ) {
 printf( "Inter=(%g,%g)\n", il->intersection.x, il->intersection.y );
 { SplineList *sl; printf(" --\n" );
  for ( sl=il->splines; sl!=NULL; sl=sl->next ) {
   if ( sl->spline->from->me.x==ilist->intersection.x && sl->spline->from->me.y==ilist->intersection.y )
    printf( "\t-> (%g,%g)", sl->spline->to->me.x, sl->spline->to->me.y );
   else
    printf( "\t-> (%g,%g)", sl->spline->from->me.x, sl->spline->from->me.y );
   printf( "%s%s\n", sl->spline->isneeded?" needed":"", sl->spline->isunneeded?" un":"" );
  }
 }}}
#endif

/* Various operations that could be tried to make life easier:
	Round to int
	Insert extreme points
	bring points that are close together on top of one another
	?Correct Direction?	But that alters semantics
*/
SplineSet *SplineSetRemoveOverlap(SplineChar *sc, SplineSet *base,enum overlap_type ot) {
    SplineSet *needed, *tbase, *new, *next;
    IntersectionList *ilist;
    int changed = false;
    SplineSet *open;

    SplineSetsUntick(base);

    tbase = base;
    /*tbase = SplineCharSimplify(sc,tbase,sf_cleanup,1.0/64.0,0);*/
    open = SplineSetsExtractOpen(&tbase);
    base = tbase;
    if ( ot==over_findinter ) {
	ilist = SplineSetFindIntersections(sc,base);
	ILFree(ilist);
	needed = base;
    } else {
	SplineSetsRound2Sixtyfourths(base);
	base = SplineCharRemoveTiny(sc,base);
	ilist = SplineSetFindIntersections(sc,base);
	base = RemoveBacktracks(sc,base,ilist);
	SplineSetFindNeeded(sc,base,ot);
	RemoveDuplicates(ilist);
	CleanupNeededUn(sc,base,ilist);
#ifdef DEBUG
	ShowIntersections(ilist);
#endif
	SSValidate(sc,base);
	base = SSRemoveAllUnneeded(sc,base,ilist);
#ifdef DEBUG
	ShowIntersections(ilist);
#endif
	SSValidate(sc,base);
	tbase = base;
	needed = SSRemoveAllNeeded(sc,&tbase,ilist);
	base = tbase;
	/*SSValidate(sc,base);*/
	ILDisconnect(ilist);
	ILFreeUnusedSplines(sc,ilist);
	new = SSRebuild(sc,ilist);

	/* Here all splines will be either used or freed, but the old SS headers */
	/*  will still exist */
	while ( base!=NULL ) {
	    next = base->next;
	    chunkfree(base,sizeof(SplinePointList));
	    base = next;
	}
	ILFree(ilist);

	if ( needed==NULL )
	    needed=new;
	else if ( needed!=NULL ) {
	    for ( next=needed; next->next!=NULL; next = next->next );
	    next->next = new;
	}
	SplineSetsCorrect(needed,&changed);		/* Make sure it's all pointing the right way */
    }
    if ( open==NULL )
	open=needed;
    else if ( needed!=NULL ) {
	for ( next=open; next->next!=NULL; next = next->next );
	next->next = needed;
    }
return( open );
}
