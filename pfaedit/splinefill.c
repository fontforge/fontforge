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
#include <string.h>
#include <ustring.h>
#include <math.h>
#include "gdraw.h"
#include "splinefont.h"
#include "edgelist.h"

static void HintsFree(Hints *h) {
    Hints *hnext;
    for ( ; h!=NULL; h = hnext ) {
	hnext = h->next;
	free(h);
    }
}

void FreeEdges(EdgeList *es) {
    int i;

    /* edges will be NULL if the user tries to make an enormous bitmap */
    /*  if the linear size is bigger than several thousand, we just */
    /*  ignore the request */
    if ( es->edges!=NULL ) {
	for ( i=0; i<es->cnt; ++i ) {
	    Edge *e, *next;
	    for ( e = es->edges[i]; e!=NULL; e = next ) {
		next = e->esnext;
		free(e);
	    }
	}
    }
    free(es->edges);
    free(es->interesting);
    HintsFree(es->hhints);
    HintsFree(es->vhints);
}

real TOfNextMajor(Edge *e, EdgeList *es, real sought_m ) {
    /* We want to find t so that Mspline(t) = sought_m */
    /*  the curve is monotonic */
    Spline *sp = e->spline;
    Spline1D *msp = &e->spline->splines[es->major];
    real slope = (3.0*msp->a*e->t_cur+2.0*msp->b)*e->t_cur + msp->c;
    real new_t, old_t, newer_t;
    real found_m;
    real t_mmax, t_mmin;
    real mmax, mmin;

    if ( sp->islinear ) {
	new_t = e->t_cur + (sought_m-e->m_cur)/(es->scale * msp->c);
	e->m_cur = (msp->c*new_t + msp->d)*es->scale - es->mmin;
return( new_t );
    }
    /* if we have a spline that is nearly horizontal at its max. endpoint */
    /*  then finding A value of t for which y has the right value isn't good */
    /*  enough (at least not when finding intersections) */
    if ( sought_m+1>e->mmax ) {
	e->m_cur = e->mmax;
return( e->t_mmax );
    }

    /* if we've adjusted the height then we won't be able to find it restricting */
    /*  t between [0,1] as we do. So it's a special case. (this is to handle */
    /*  hstem hints) */
    if ( e->max_adjusted && sought_m==e->mmax ) {
	e->m_cur = sought_m;
return( e->up?1.0:0.0 );
    }

    old_t = e->t_cur;
    slope *= es->scale;
    mmax = e->mmax; t_mmax = e->t_mmax;
    mmin = e->m_cur; t_mmin = old_t;

    if ( slope==0 )	/* we often start at a point of inflection */
	new_t = old_t + (t_mmax-t_mmin)* (sought_m-mmin)/(mmax-mmin);
    else {
	new_t = old_t + 1/slope;
	if (( new_t>t_mmax && e->up ) ||
		(new_t<t_mmax && !e->up) ||
		(new_t<t_mmin && e->up ) ||
		(new_t>t_mmin && !e->up ))
	    new_t = old_t + (t_mmax-t_mmin)* (sought_m-mmin)/(mmax-mmin);
    }

    while ( 1 ) {
	found_m = ( ((msp->a*new_t+msp->b)*new_t+msp->c)*new_t + msp->d )
		* es->scale - es->mmin ;
	if ( found_m>sought_m-.001 && found_m<sought_m+.001 ) {
	    e->m_cur = found_m;
return( new_t );
	}
	newer_t = (t_mmax+t_mmin)/2 /* * (sought_m-found_m)/(mmax-mmin) */;
	if ( found_m > sought_m ) {
	    mmax = found_m;
	    t_mmax = new_t;
	} else {
	    mmin = found_m;
	    t_mmin = new_t;
	}
	if ( t_mmax==t_mmin ) {
	    GDrawIError("TOfNextMajor failed! on %s", es->sc!=NULL?es->sc->name:"Unknown" );
	    e->m_cur = sought_m;
return( new_t );
	}
	new_t = newer_t;
#if 0
	if (( new_t>t_mmax && e->up ) ||
		(new_t<t_mmax && !e->up) ||
		(new_t<t_mmin && e->up ) ||
		(new_t>t_mmin && !e->up ))
	    new_t = (t_mmax+t_mmin)/2;
#endif
    }
}

static int SlopeLess(Edge *e, Edge *p, int other) {
    Spline1D *osp = &e->spline->splines[other];
    Spline1D *psp = &p->spline->splines[other];
    Spline1D *msp = &e->spline->splines[!other];
    Spline1D *qsp = &p->spline->splines[!other];
    real os = (3*osp->a*e->t_cur+2*osp->b)*e->t_cur+osp->c,
	   ps = (3*psp->a*p->t_cur+2*psp->b)*p->t_cur+psp->c;
    real ms = (3*msp->a*e->t_cur+2*msp->b)*e->t_cur+msp->c,
	   qs = (3*qsp->a*p->t_cur+2*qsp->b)*p->t_cur+qsp->c;
    if ( e->t_cur-e->tmin > e->tmax-e->t_cur ) { os = -os; ms = -ms; }
    if ( p->t_cur-p->tmin > p->tmax-p->t_cur ) { ps = -ps; qs = -qs; }
    if ( ms<.0001 && ms>-.0001 ) ms = 0;
    if ( qs<.0001 && qs>-.0001 ) qs = 0;
    if ( ms!=0 && qs!=0 ) { os /= ms; ps /= qs; }
    else if ( ms==0 && qs==0 ) /* Do Nothing */;
    else if ( ms==0 )
return( false );
    else if ( qs==0 )
return( true );

    if ( os==ps )
return( e->o_mmax<p->o_mmax );

return( os<ps );
}

static void AddEdge(EdgeList *es, Spline *sp, real tmin, real tmax ) {
    Edge *e, *pr;
    real m1, m2;
    int mpos;
    Hints *hint;
    Spline1D *msp = &sp->splines[es->major], *osp = &sp->splines[es->other];

    e = gcalloc(1,sizeof(Edge));
    e->spline = sp;

    m1 = ( ((msp->a*tmin+msp->b)*tmin+msp->c)*tmin + msp->d ) * es->scale;
    m2 = ( ((msp->a*tmax+msp->b)*tmax+msp->c)*tmax + msp->d ) * es->scale;
    if ( m1>m2 ) {
	e->mmin = m2;
	e->t_mmin = tmax;
	e->mmax = m1;
	e->t_mmax = tmin;
	e->up = false;
    } else {
	e->mmax = m2;
	e->t_mmax = tmax;
	e->mmin = m1;
	e->t_mmin = tmin;
	e->up = true;
    }
    if ( RealNear(e->mmin,es->mmin)) e->mmin = es->mmin;
    e->o_mmin = ( ((osp->a*e->t_mmin+osp->b)*e->t_mmin+osp->c)*e->t_mmin + osp->d ) * es->scale;
    e->o_mmax = ( ((osp->a*e->t_mmax+osp->b)*e->t_mmax+osp->c)*e->t_mmax + osp->d ) * es->scale;
    e->mmin -= es->mmin; e->mmax -= es->mmin;
    e->t_cur = e->t_mmin;
    e->o_cur = e->o_mmin;
    e->m_cur = e->mmin;
    e->last_opos = e->last_mpos = -2;
    e->tmin = tmin; e->tmax = tmax;

    if ( e->mmin<0 || e->mmin>=e->mmax ) {
	/*GDrawIError("Probably not serious, but we've got a zero length spline in AddEdge in %s",es->sc==NULL?<nameless>:es->sc->name);*/
	free(e);
return;
    }

    if ( es->sc!=NULL ) for ( hint=es->hhints; hint!=NULL; hint=hint->next ) {
	if ( hint->adjustb ) {
	    if ( e->m_cur>hint->b1 && e->m_cur<hint->b2 ) {
		e->m_cur = e->mmin = hint->ab;
		e->min_adjusted = true;
	    } else if ( e->mmax>hint->b1 && e->mmax<hint->b2 ) {
		e->mmax = hint->ab;
		e->max_adjusted = true;
	    }
	} else if ( hint->adjuste ) {
	    if ( e->m_cur>hint->e1 && e->m_cur<hint->e2 ) {
		e->m_cur = e->mmin = hint->ae;
		e->min_adjusted = true;
	    } else if ( e->mmax>hint->e1 && e->mmax<hint->e2 ) {
		e->mmax = hint->ae;
		e->max_adjusted = true;
	    }
	}
    }

    mpos = (int) ceil(e->m_cur);
    if ( mpos>e->mmax ) {
	free(e);
return;
    }

    if ( e->m_cur!=ceil(e->m_cur) ) {
	/* bring the new edge up to its first scan line */
	e->t_cur = TOfNextMajor(e,es,ceil(e->m_cur));
	e->o_cur = ( ((osp->a*e->t_cur+osp->b)*e->t_cur+osp->c)*e->t_cur + osp->d ) * es->scale;
    }

    e->before = es->last;
    if ( es->last!=NULL )
	es->last->after = e;
    if ( es->last==NULL )
	es->splinesetfirst = e;
    es->last = e;

    if ( es->edges[mpos]==NULL || e->o_cur<es->edges[mpos]->o_cur ||
	    (e->o_cur==es->edges[mpos]->o_cur && SlopeLess(e,es->edges[mpos],es->other))) {
	e->esnext = es->edges[mpos];
	es->edges[mpos] = e;
    } else {
	for ( pr=es->edges[mpos]; pr->esnext!=NULL && pr->esnext->o_cur<e->o_cur ;
		pr = pr->esnext );
	/* When two splines share a vertex which is a local minimum, then */
	/*  o_cur will be equal for both (to the vertex's o value) and so */
	/*  the above code randomly picked one to go first. That screws up */
	/*  the overlap code, which wants them properly ordered from the */
	/*  start. so look at the end point, nope the end point isn't always */
	/*  meaningful, look at the slope... */
	if ( pr->esnext!=NULL && pr->esnext->o_cur==e->o_cur &&
		SlopeLess(e,pr->esnext,es->other)) {
	    pr = pr->esnext;
	}
	e->esnext = pr->esnext;
	pr->esnext = e;
    }
    if ( es->interesting ) {
	/* Mark the other end of the spline as interesting */
	es->interesting[(int) ceil(e->mmax)]=1;
    }
}

static void AddMajorEdge(EdgeList *es, Spline *sp) {
    Edge *e, *pr;
    real m1;
    Spline1D *msp = &sp->splines[es->major], *osp = &sp->splines[es->other];

    e = gcalloc(1,sizeof(Edge));
    e->spline = sp;

    e->mmin = e->mmax = m1 = msp->d * es->scale - es->mmin;
    e->t_mmin = 0;
    e->t_mmax = 1;
    e->up = false;
    e->o_mmin = osp->d * es->scale;
    e->o_mmax = ( osp->a + osp->b + osp->c + osp->d ) * es->scale;
    if ( e->o_mmin == e->o_mmax ) {	/* Just a point? */
	free(e);
return;
    }
    if ( e->mmin<0 )
	GDrawIError("Grg!");

    if ( ceil(e->m_cur)>e->mmax ) {
	free(e);
return;
    }

    if ( es->majors==NULL || es->majors->mmin>=m1 ) {
	e->esnext = es->majors;
	es->majors = e;
    } else {
	for ( pr=es->majors; pr->esnext!=NULL && pr->esnext->mmin<m1; pr = pr->esnext );
	e->esnext = pr->esnext;
	pr->esnext = e;
    }
}

static void AddSpline(EdgeList *es, Spline *sp ) {
    real t1=2, t2=2, t;
    real b2_fourac;
    real fm, tm;
    Spline1D *msp = &sp->splines[es->major], *osp = &sp->splines[es->other];

    /* Find the points of inflection on the curve discribing y behavior */
    if ( !RealNear(msp->a,0) ) {
	/* cubic, possibly 2 inflections (possibly none) */
	b2_fourac = 4*msp->b*msp->b - 12*msp->a*msp->c;
	if ( b2_fourac>=0 ) {
	    b2_fourac = sqrt(b2_fourac);
	    t1 = (-2*msp->b - b2_fourac) / (6*msp->a);
	    t2 = (-2*msp->b + b2_fourac) / (6*msp->a);
	    if ( t1>t2 ) { real temp = t1; t1 = t2; t2 = temp; }
	    else if ( t1==t2 ) t2 = 2.0;

	    /* check for curves which have such a small slope they might */
	    /*  as well be horizontal */
	    fm = es->major==1?sp->from->me.y:sp->from->me.x;
	    tm = es->major==1?sp->to->me.y:sp->to->me.x;
	    if ( fm==tm ) {
		real m1, m2, d1, d2;
		m1 = m2 = fm;
		if ( t1>0 && t1<1 )
		    m1 = ((msp->a*t1+msp->b)*t1+msp->c)*t1 + msp->d;
		if ( t2>0 && t2<1 )
		    m2 = ((msp->a*t2+msp->b)*t2+msp->c)*t2 + msp->d;
		d1 = (m1-fm)*es->scale;
		d2 = (m2-fm)*es->scale;
		if ( d1>-.5 && d1<.5 && d2>-.5 && d2<.5 ) {
		    sp->ishorvert = true;
		    if ( es->genmajoredges )
			AddMajorEdge(es,sp);
return;		/* Pretend it's horizontal, ignore it */
		}
	    }
	}
    } else if ( !RealNear(msp->b,0) ) {
	/* Quadratic, at most one inflection */
	t1 = -msp->c/(2.0*msp->b);
    } else if ( !RealNear(msp->c,0) ) {
	/* linear, no points of inflection */
    } else {
	sp->ishorvert = true;
	if ( es->genmajoredges )
	    AddMajorEdge(es,sp);
return;		/* Horizontal line, ignore it */
    }

    if ( RealNear(t1,0)) t1=0;
    if ( RealNear(t1,1)) t1=1;
    if ( RealNear(t2,0)) t2=0;
    if ( RealNear(t2,1)) t2=1;
    if ( RealNear(t1,t2)) t2=2;
    t=0;
    if ( t1>0 && t1<1 ) {
	AddEdge(es,sp,0,t1);
	t = t1;
    }
    if ( t2>0 && t2<1 ) {
	AddEdge(es,sp,t,t2);
	t = t2;
    }
    AddEdge(es,sp,t,1.0);
    if ( es->interesting ) {
	/* Also store up points of inflection in X as interesting (we got the endpoints, just internals now)*/
	real ot1, ot2;
	int mpos;
	SplineFindExtrema(osp,&ot1,&ot2);
	if ( ot1>0 && ot1<1 ) {
	    mpos = (int) ceil( ((msp->a*ot1+msp->b)*ot1+msp->c)*ot1+msp->d-es->mmin );
	    es->interesting[mpos] = 1;
	}
	if ( ot2>0 && ot2<1 ) {
	    mpos = (int) ceil( ((msp->a*ot2+msp->b)*ot2+msp->c)*ot2+msp->d-es->mmin );
	    es->interesting[mpos] = 1;
	}
    }
}

void FindEdgesSplineSet(SplinePointList *spl, EdgeList *es) {
    Spline *spline, *first;

    for ( ; spl!=NULL; spl = spl->next ) {
	first = NULL;
	es->last = es->splinesetfirst = NULL;
	/* Set so there is no previous point!!! */
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    AddSpline(es,spline);
	    if ( first==NULL ) first = spline;
	}
	if ( es->last!=NULL ) {
	    es->splinesetfirst->before = es->last;
	    es->last->after = es->splinesetfirst;
	}
    }
}

static void FindEdges(SplineChar *sc, EdgeList *es) {
    RefChar *rf;

    for ( rf=sc->refs; rf!=NULL; rf = rf->next )
	FindEdgesSplineSet(rf->splines,es);

    FindEdgesSplineSet(sc->splines,es);
}

Edge *ActiveEdgesInsertNew(EdgeList *es, Edge *active,int i) {
    Edge *apt, *pr, *npt;

    for ( pr=NULL, apt=active, npt=es->edges[(int) i]; apt!=NULL && npt!=NULL; ) {
	if ( npt->o_cur<apt->o_cur ) {
	    npt->aenext = apt;
	    if ( pr==NULL )
		active = npt;
	    else
		pr->aenext = npt;
	    pr = npt;
	    npt = npt->esnext;
	} else {
	    pr = apt;
	    apt = apt->aenext;
	}
    }
    while ( npt!=NULL ) {
	npt->aenext = NULL;
	if ( pr==NULL )
	    active = npt;
	else
	    pr->aenext = npt;
	pr = npt;
	npt = npt->esnext;
    }
return( active );
}

Edge *ActiveEdgesRefigure(EdgeList *es, Edge *active,real i) {
    Edge *apt, *pr;
    int any;

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
		    active = apt->aenext;
		    apt->aenext = apt->aenext->aenext;
		    active->aenext = apt;
		    /* don't need to set any, since this reorder can't disorder the list */
		    pr = active;
		} else {
		    pr->aenext = apt->aenext;
		    apt->aenext = apt->aenext->aenext;
		    pr->aenext->aenext = apt;
		    any = true;
		    pr = pr->aenext;
		}
	    }
	}
    }
    /* Insert new nodes */
    active = ActiveEdgesInsertNew(es,active,i);
return( active );
}

Edge *ActiveEdgesFindStem(Edge *apt, Edge **prev, real i) {
    int cnt=apt->up?1:-1;
    Edge *pr, *e;

    for ( pr=apt, e=apt->aenext; e!=NULL && cnt!=0; pr=e, e=e->aenext ) {
	if ( pr->up!=e->up )
	    cnt += (e->up?1:-1);
	else if ( (pr->before==e || pr->after==e ) &&
		(( pr->mmax==i && e->mmin==i ) ||
		 ( pr->mmin==i && e->mmax==i )) )
	    /* This just continues the line and doesn't change count */;
	else
	    cnt += (e->up?1:-1);
    }
    /* color a horizontal line that comes out of the last vertex */
    if ( e!=NULL && (e->before==pr || e->after==pr) &&
		(( pr->mmax==i && e->mmin==i ) ||
		 ( pr->mmin==i && e->mmax==i )) ) {
	pr = e;
	e = e->aenext;
    } else if ( e!=NULL && ((pr->up && !e->up) || (!pr->up && e->up)) &&
		    pr->spline!=e->spline &&
		    ((pr->after==e && pr->spline->to->next!=NULL &&
				pr->spline->to->next!=e->spline &&
				pr->spline->to->next->to->next==e->spline ) ||
			    (pr->before==e && pr->spline->from->prev!=NULL &&
				pr->spline->from->prev!=e->spline &&
				pr->spline->from->prev->from->prev!=e->spline )) &&
		    ((pr->mmax == i && e->mmax==i ) ||
		     (pr->mmin == i && e->mmin==i )) ) {
	pr = e;
    }
    *prev = pr;
return( e );
}

static int isvstem(EdgeList *es,real stem,int *vval) {
    Hints *hint;

    for ( hint=es->vhints; hint!=NULL ; hint=hint->next ) {
	if ( stem>=hint->b1 && stem<=hint->b2 ) {
	    *vval = hint->ab;
return( true );
	} else if ( stem>=hint->e1 && stem<=hint->e2 ) {
	    *vval = hint->ae;
return( true );
	}
    }
return( false );
}

static void FillChar(EdgeList *es) {
    Edge *active=NULL, *apt, *pr, *e, *prev;
    int i, k, end, width, oldk;
    uint8 *bpt;

    for ( i=0; i<es->cnt; ++i ) {
	active = ActiveEdgesRefigure(es,active,i);

	/* process scanline */
	bpt = es->bitmap+((es->cnt-1-i)*es->bytes_per_line);
	for ( apt=active; apt!=NULL; ) {
	    e = ActiveEdgesFindStem(apt,&prev,i);
	    pr = prev;
	    width = rint(pr->o_cur-apt->o_cur);
	    if ( width<=0 ) width=1;
	    k=rint(apt->o_cur-es->omin);
	    end =rint(pr->o_cur-es->omin);
	    if ( end-k > width-1 ) {
		int kval = -999999, eval= -999999;
		if ( isvstem(es,apt->o_cur,&kval) )
		    k = kval;
		if ( isvstem(es,pr->o_cur,&eval))
		    end = eval;
		if ( end-k > width-1 ) {
		    if ( k!=kval && (end==eval || (apt->o_cur-es->omin)-k > end-(pr->o_cur-es->omin) ))
			++k;
		    else if ( end!=eval )
			--end;
		}
	    }
	    oldk = k;
	    if ( apt->last_mpos==i-1 || pr->last_mpos==i-1 ) {
		int lx1=apt->last_opos, lx2=pr->last_opos;
		if ( apt->last_mpos!=i-1 ) lx1 = lx2;
		else if ( pr->last_mpos!=i-1 ) lx2 = lx1;
		if ( lx1>lx2 ) { int temp = lx1; lx1=lx2; lx2=temp; }
		if ( lx2<k-1 ) {
		    int mid = (lx2+k)/2;
		    for ( ; lx2<mid; ++lx2 )
			bpt[(lx2>>3)+es->bytes_per_line] |= (1<<(7-(lx2&7)));
		    for ( ; lx2<k; ++lx2 )
			bpt[(lx2>>3)] |= (1<<(7-(lx2&7)));
		} else if ( lx1>end+1 ) {
		    int mid = (lx1+end)/2;
		    for ( ; lx1<mid; --lx1 )
			bpt[(lx1>>3)+es->bytes_per_line] |= (1<<(7-(lx1&7)));
		    for ( ; lx1<end; --lx1 )
			bpt[(lx1>>3)] |= (1<<(7-(lx1&7)));
		}
	    }
	    for ( ; k<=end; ++k )
		bpt[k>>3] |= (1<<(7-(k&7)));
	    apt->last_mpos = pr->last_mpos = i;
	    apt->last_opos = oldk;
	    pr->last_opos = end;
	    apt = e;
	}
    }
}

static void InitializeHints(SplineChar *sc, EdgeList *es) {
    Hints *hint, *last;
    StemInfo *s;
    real t1, t2;
    int k,end,width;

    /* we only care about hstem hints, and only if they fail to cross a */
    /*  vertical pixel boundary. If that happens, adjust either the top */
    /*  or bottom position so that a boundary forcing is crossed.   Any */
    /*  vertexes at those points will be similarly adjusted later... */

    last = NULL; es->hhints = NULL;
    for ( s=sc->hstem; s!=NULL; s=s->next ) {
	hint = gcalloc(1,sizeof(Hints));
	hint->base = s->start; hint->width = s->width;
	if ( last==NULL ) es->hhints = hint;
	else last->next = hint;
	last = hint;
	hint->adjuste = hint->adjustb = false;
	t1 = hint->base*es->scale	-es->mmin;
	t2 = (hint->base+hint->width)*es->scale	-es->mmin;
	if ( floor(t1)==floor(t2) && t1!=floor(t1) && t2!=floor(t2) ) {
	    if ( t1<t2 ) {
		if ( t1-floor(t1) > ceil(t2)-t2 ) {
		    hint->adjuste = true;
		    hint->ae = ceil(t2);
		} else {
		    hint->adjustb = true;
		    hint->ab = floor(t1);
		}
	    } else {
		if ( t2-floor(t2) > ceil(t1)-t1 ) {
		    hint->adjustb = true;
		    hint->ab = ceil(t1);
		} else {
		    hint->adjuste = true;
		    hint->ae = floor(t2);
		}
	    }
	}
	hint->b1 = t1-.2; hint->b2 = t1 + .2;
	hint->e1 = t2-.2; hint->e2 = t2 + .2;
    }

    /* Nope. We care about vstems too now, but in a different case */
    last = NULL; es->vhints = NULL;
    for ( s=sc->vstem; s!=NULL; s=s->next ) {
	hint = gcalloc(1,sizeof(Hints));
	hint->base = s->start; hint->width = s->width;
	if ( last==NULL ) es->vhints = hint;
	else last->next = hint;
	last = hint;
	if ( hint->width<0 ) {
	    width = -rint(-hint->width*es->scale);
	    t1 = (hint->base+hint->width)*es->scale;
	    t2 = hint->base*es->scale;
	} else {
	    width = rint(hint->width*es->scale);
	    t1 = hint->base*es->scale;
	    t2 = (hint->base+hint->width)*es->scale;
	}
	k = rint(t1-es->omin); end = rint(t2-es->omin);
	if ( end-k > width-1 )
	    if ( end-k > width-1 ) {
		if ( (t1-es->omin)-k > end-(t2-es->omin) )
		    ++k;
		else
		    --end;
	    }
	hint->ab = k; hint->ae = end;
	hint->b1 = t1-.2; hint->b2 = t1 + .2;
	hint->e1 = t2-.2; hint->e2 = t2 + .2;
    }
}

/* After a bitmap has been compressed, it's sizes may not comply with the */
/*  expectations for saving images */
void BCRegularizeBitmap(BDFChar *bdfc) {
    int bpl =(bdfc->xmax-bdfc->xmin)/8+1;
    int i;

    if ( bdfc->bytes_per_line!=bpl ) {
	uint8 *bitmap = galloc(bpl*(bdfc->ymax-bdfc->ymin+1));
	for ( i=0; i<=(bdfc->ymax-bdfc->ymin); ++i )
	    memcpy(bitmap+i*bpl,bdfc->bitmap+i*bdfc->bytes_per_line,bpl);
	free(bdfc->bitmap);
	bdfc->bitmap= bitmap;
	bdfc->bytes_per_line = bpl;
    }
}

void BCRegularizeGreymap(BDFChar *bdfc) {
    int bpl = bdfc->xmax-bdfc->xmin+1;
    int i;

    if ( bdfc->bytes_per_line!=bpl ) {
	uint8 *bitmap = galloc(bpl*(bdfc->ymax-bdfc->ymin+1));
	for ( i=0; i<=(bdfc->ymax-bdfc->ymin); ++i )
	    memcpy(bitmap+i*bpl,bdfc->bitmap+i*bdfc->bytes_per_line,bpl);
	free(bdfc->bitmap);
	bdfc->bitmap= bitmap;
	bdfc->bytes_per_line = bpl;
    }
}

void BCCompressBitmap(BDFChar *bdfc) {
    /* Now we may have allocated a bit more than we need to the bitmap */
    /*  check to see if there are any unused rows or columns.... */
    int i,j,any, off, last;

    /* we can use the same code to lop off rows wether we deal with bytes or bits */
    for ( i=0; i<bdfc->ymax-bdfc->ymin; ++i ) {
	any = 0;
	for ( j=0; j<bdfc->bytes_per_line; ++j )
	    if ( bdfc->bitmap[i*bdfc->bytes_per_line+j]!=0 ) any = 1;
	if ( any )
    break;
    }
    if ( i!=0 ) {
	bdfc->ymax -= i;
	memcpy(bdfc->bitmap,bdfc->bitmap+i*bdfc->bytes_per_line,(bdfc->ymax-bdfc->ymin+1)*bdfc->bytes_per_line );
    }

    for ( i=bdfc->ymax-bdfc->ymin; i>0; --i ) {
	any = 0;
	for ( j=0; j<bdfc->bytes_per_line; ++j )
	    if ( bdfc->bitmap[i*bdfc->bytes_per_line+j]!=0 ) any = 1;
	if ( any )
    break;
    }
    if ( i!=bdfc->ymax-bdfc->ymin ) {
	bdfc->ymin += bdfc->ymax-bdfc->ymin-i;
    }

    if ( !bdfc->byte_data ) {
	for ( j=0; j<bdfc->xmax-bdfc->xmin; ++j ) {
	    any = 0;
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i )
		if ( bdfc->bitmap[i*bdfc->bytes_per_line+(j>>3)] & (1<<(7-(j&7))) )
		    any = 1;
	    if ( any )
	break;
	}
	if ( j!=0 ) {
	    off = j;
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i ) {
		last = 0;
		for ( j=bdfc->bytes_per_line-1; j>=0; --j ) {
		    int index = i*bdfc->bytes_per_line+j;
		    int temp = bdfc->bitmap[index]>>(8-off);
		    bdfc->bitmap[index] = (bdfc->bitmap[index]<<off)|last;
		    last = temp;
		}
		if ( last!=0 )
		    GDrawIError("Sigh");
	    }
	    bdfc->xmin += off;
	}

	for ( j=bdfc->xmax-bdfc->xmin; j>0; --j ) {
	    any = 0;
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i )
		if ( bdfc->bitmap[i*bdfc->bytes_per_line+(j>>3)] & (1<<(7-(j&7))) )
		    any = 1;
	    if ( any )
	break;
	}
	if ( j!=bdfc->xmax+bdfc->xmin ) {
	    bdfc->xmax -= bdfc->xmax-bdfc->xmin-j;
	}
	BCRegularizeBitmap(bdfc);
    } else {
	for ( j=0; j<bdfc->xmax-bdfc->xmin; ++j ) {
	    any = 0;
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i )
		if ( bdfc->bitmap[i*bdfc->bytes_per_line+j] != 0 )
		    any = 1;
	    if ( any )
	break;
	}
	if ( j!=0 ) {
	    off = j;
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i ) {
		memcpy(bdfc->bitmap+i*bdfc->bytes_per_line,
			bdfc->bitmap+i*bdfc->bytes_per_line+off,
			bdfc->bytes_per_line-off);
		memset(bdfc->bitmap+i*bdfc->bytes_per_line+bdfc->bytes_per_line-off,
			0, off);
	    }
	    bdfc->xmin += off;
	}

	for ( j=bdfc->xmax-bdfc->xmin; j>0; --j ) {
	    any = 0;
	    for ( i=0; i<bdfc->ymax-bdfc->ymin+1; ++i )
		if ( bdfc->bitmap[i*bdfc->bytes_per_line+j] != 0 )
		    any = 1;
	    if ( any )
	break;
	}
	if ( j!=bdfc->xmax+bdfc->xmin ) {
	    bdfc->xmax -= bdfc->xmax-bdfc->xmin-j;
	}
	BCRegularizeGreymap(bdfc);
    }
}

BDFChar *SplineCharRasterize(SplineChar *sc, int pixelsize) {
    EdgeList es;
    DBounds b;
    BDFChar *bdfc;
    SplineSet *open;

    if ( sc==NULL )
return( NULL );
    memset(&es,'\0',sizeof(es));
    if ( sc==NULL ) {
	es.mmin = es.mmax = es.omin = es.omax = 0;
	es.bitmap = gcalloc(1,1);
	es.bytes_per_line = 1;
    } else {
#if 1
	/* Some truetype fonts have open paths that should be ignored */
	open = SplineSetsExtractOpen(&sc->splines);
#endif
	SplineCharFindBounds(sc,&b);
	es.scale = (pixelsize-.3) / (real) (sc->parent->ascent+sc->parent->descent);
	es.mmin = floor(b.miny*es.scale);
	es.mmax = ceil(b.maxy*es.scale);
	es.omin = b.minx*es.scale;
	es.omax = b.maxx*es.scale;
	es.cnt = (int) (es.mmax-es.mmin) + 1;
	if ( es.cnt<4000 && es.omax-es.omin<4000 && es.cnt>1 ) {
	    es.edges = gcalloc(es.cnt,sizeof(Edge *));
	    es.sc = sc;
	    es.major = 1; es.other = 0;

	    InitializeHints(sc,&es);
	    FindEdges(sc,&es);

	    es.bytes_per_line = ((int) ceil(es.omax-es.omin) + 8)/8;
	    es.bitmap = gcalloc(es.cnt*es.bytes_per_line,1);
	    FillChar(&es);
	} else {
	    /* If they want a bitmap so enormous it threatens our memory */
	    /*  then just give 'em a blank. It's probably by mistake anyway */
	    es.mmin = es.mmax = es.omin = es.omax = 0;
	    es.bitmap = gcalloc(1,1);
	    es.bytes_per_line = 1;
	}
#if 1
	if ( open!=NULL ) {
	    SplineSet *e;
	    for ( e=open; e->next!=NULL; e = e->next );
	    e->next = sc->splines;
	    sc->splines = open;
	}
#endif
    }

    bdfc = chunkalloc(sizeof(BDFChar));
    bdfc->sc = sc;
    bdfc->xmin = rint(es.omin);
    bdfc->ymin = es.mmin;
    bdfc->xmax = (int) ceil(es.omax-es.omin) + bdfc->xmin;
    bdfc->ymax = es.mmax;
    if ( sc!=NULL ) {
	bdfc->width = rint(sc->width*pixelsize / (real) (sc->parent->ascent+sc->parent->descent));
	bdfc->enc = sc->enc;
    }
    bdfc->bitmap = es.bitmap;
    bdfc->bytes_per_line = es.bytes_per_line;
    BCCompressBitmap(bdfc);
    FreeEdges(&es);
return( bdfc );
}

BDFFont *SplineFontToBDFHeader(SplineFont *_sf, int pixelsize, int indicate) {
    BDFFont *bdf = gcalloc(1,sizeof(BDFFont));
    int i;
    real scale;
    char csize[10]; unichar_t size[30];
    unichar_t aa[200];
    int max;
    SplineFont *sf;	/* The complexity here is to pick the appropriate subfont of a CID font */

    sf = _sf;
    max = sf->charcnt;
    for ( i=0; i<_sf->subfontcnt; ++i ) {
	sf = _sf->subfonts[i];
	if ( sf->charcnt>max ) max = sf->charcnt;
    }
    scale = pixelsize / (real) (sf->ascent+sf->descent);

    if ( indicate ) {
	sprintf(csize,"%d", pixelsize );
	uc_strcpy(size,csize);
	u_strcat(size,GStringGetResource(_STR_Pixels,NULL));
	u_strcpy(aa,GStringGetResource(_STR_GenBitmap,NULL));
	if ( sf->fontname!=NULL ) {
	    uc_strcat(aa,": ");
	    uc_strncat(aa,sf->fontname,sizeof(aa)/sizeof(aa[0])-u_strlen(aa));
	    aa[sizeof(aa)/sizeof(aa[0])-1] = '\0';
	}
	GProgressStartIndicator(10,GStringGetResource(_STR_Rasterizing,NULL),
		aa,size,sf->charcnt,1);
	GProgressEnableStop(0);
    }
    bdf->sf = _sf;
    bdf->charcnt = max;
    bdf->pixelsize = pixelsize;
    bdf->chars = galloc(max*sizeof(BDFChar *));
    bdf->ascent = rint(sf->ascent*scale);
    bdf->descent = pixelsize-bdf->ascent;
    bdf->encoding_name = sf->encoding_name;
    bdf->res = -1;
return( bdf );
}

#if 0
/* This code was an attempt to do better at rasterizing by making a big bitmap*/
/*  and shrinking it down. It did make curved edges look better, but it made */
/*  straight edges worse. So I don't think it's worth it. */
static int countsquare(BDFChar *bc, int i, int j, int linear_scale) {
    int ii,jj,jjj, cnt;
    uint8 *bpt;

    cnt = 0;
    for ( ii=0; ii<linear_scale; ++ii ) {
	if ( i*linear_scale+ii>bc->ymax-bc->ymin )
    break;
	bpt = bc->bitmap + (i*linear_scale+ii)*bc->bytes_per_line;
	for ( jj=0; jj<linear_scale; ++jj ) {
	    jjj = j*linear_scale+jj;
	    if ( jjj>bc->xmax-bc->xmin )
	break;
	    if ( bpt[jjj>>3]& (1<<(7-(jjj&7))) )
		++cnt;
	}
    }
return( cnt );
}

static int makesleftedge(BDFChar *bc, int i, int j, int linear_scale) {
    int ii,jjj;
    uint8 *bpt;

    for ( ii=0; ii<linear_scale; ++ii ) {
	if ( i*linear_scale+ii>bc->ymax-bc->ymin )
return( false );
	bpt = bc->bitmap + (i*linear_scale+ii)*bc->bytes_per_line;
	jjj = j*linear_scale;
	if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
return( false );
    }
return( true );
}

static int makesbottomedge(BDFChar *bc, int i, int j, int linear_scale) {
    int jj,jjj;
    uint8 *bpt;

    for ( jj=0; jj<linear_scale; ++jj ) {
	jjj = j*linear_scale+jj;
	if ( jjj>bc->xmax-bc->xmin )
return( false );
	bpt = bc->bitmap + (i*linear_scale)*bc->bytes_per_line;
	if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
return( false );
    }
return( true );
}

static int makesline(BDFChar *bc, int i, int j, int linear_scale) {
    int ii,jj,jjj;
    int across, any, alltop, allbottom;	/* alltop is sometimes allleft, and allbottom allright */
    uint8 *bpt;
    /* If we have a line that goes all the way across a square then we want */
    /*  to turn on the pixel that corresponds to that square. Exception: */
    /*  if that line is on an edge of the square, and the square adjacent to */
    /*  it has more pixels, then let the adjacent square get the pixel */
    /* if two squares are essentially equal, choose the top one */

    across = alltop = allbottom = true;
    for ( ii=0; ii<linear_scale; ++ii ) {
	if ( i*linear_scale+ii>bc->ymax-bc->ymin ) {
	    across = alltop = allbottom = false;
    break;
	}
	bpt = bc->bitmap + (i*linear_scale+ii)*bc->bytes_per_line;
	jjj = j*linear_scale;
	if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
	    allbottom = 0;
	jjj += linear_scale-1;
	if ( jjj>bc->xmax-bc->xmin )
	    alltop = false;
	else if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
	    alltop = 0;
	any = false;
	for ( jj=0; jj<linear_scale; ++jj ) {
	    jjj = j*linear_scale+jj;
	    if ( jjj>bc->xmax-bc->xmin )
	break;
	    if ( bpt[jjj>>3]& (1<<(7-(jjj&7))) )
		any = true;
	}
	if ( !any )
	    across = false;
    }
    if ( across ) {
	if ( (!alltop && !allbottom ) || (alltop && allbottom))
return( true );
	if ( allbottom ) {
	    if ( j==0 )
return( true );
	    if ( countsquare(bc,i,j-1,linear_scale)>=linear_scale*linear_scale/2 )
return( false );
return( true );
	}
	if ( alltop ) {
	    if ( j==(bc->xmax-bc->xmin)/linear_scale )
return( true );
	    if ( countsquare(bc,i,j+1,linear_scale)>=linear_scale*linear_scale/2 )
return( false );
	    if ( makesleftedge(bc,i,j+1,linear_scale))
return( false );
return( true );
	}
    }

    /* now the other dimension */
    across = alltop = allbottom = true;
    for ( jj=0; jj<linear_scale; ++jj ) {
	jjj = j*linear_scale+jj;
	if ( jjj>bc->xmax-bc->xmin ) {
	    across = alltop = allbottom = false;
    break;
	}
	bpt = bc->bitmap + (i*linear_scale)*bc->bytes_per_line;
	if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
	    allbottom = 0;
	if ( i*linear_scale + linear_scale-1>bc->ymax-bc->ymin )
	    alltop = 0;
	else {
	    bpt = bc->bitmap + (i*linear_scale+linear_scale-1)*bc->bytes_per_line;
	    if ( !( bpt[jjj>>3]& (1<<(7-(jjj&7))) ))
		alltop = 0;
	}
	any = false;
	for ( ii=0; ii<linear_scale; ++ii ) {
	    if ( (i*linear_scale+ii)>bc->xmax-bc->xmin )
	break;
	    bpt = bc->bitmap + (i*linear_scale+ii)*bc->bytes_per_line;
	    if ( bpt[jjj>>3]& (1<<(7-(jjj&7))) )
		any = true;
	}
	if ( !any )
	    across = false;
    }
    if ( across ) {
	if ( (!alltop && !allbottom ) || (alltop && allbottom))
return( true );
	if ( allbottom ) {
	    if ( i==0 )
return( true );
	    if ( countsquare(bc,i-1,j,linear_scale)>=linear_scale*linear_scale/2 )
return( false );
return( true );
	}
	if ( alltop ) {
	    if ( i==(bc->ymax-bc->ymin)/linear_scale )
return( true );
	    if ( countsquare(bc,i+1,j,linear_scale)>=linear_scale*linear_scale/2 )
return( false );
	    if ( makesbottomedge(bc,i+1,j,linear_scale))
return( false );
return( true );
	}
    }

return( false );		/* No line */
}

/* Make a much larger bitmap than we need, and then shrink it */
static void BDFCShrinkBitmap(BDFChar *bc, int linear_scale) {
    BDFChar new;
    int i,j, mid = linear_scale*linear_scale/2;
    int cnt;
    uint8 *pt;

    if ( bc==NULL )
return;

    memset(&new,'\0',sizeof(new));
    new.xmin = floor( ((real) bc->xmin)/linear_scale );
    new.ymin = floor( ((real) bc->ymin)/linear_scale );
    new.xmax = new.xmin + (bc->xmax-bc->xmin+linear_scale-1)/linear_scale;
    new.ymax = new.ymin + (bc->ymax-bc->ymin+linear_scale-1)/linear_scale;
    new.width = rint( ((real) bc->width)/linear_scale );

    new.bytes_per_line = (new.xmax-new.xmin+1);
    new.enc = bc->enc;
    new.sc = bc->sc;
    new.byte_data = true;
    new.bitmap = gcalloc( (new.ymax-new.ymin+1) * new.bytes_per_line, sizeof(uint8));
    for ( i=0; i<=new.ymax-new.ymin; ++i ) {
	pt = new.bitmap + i*new.bytes_per_line;
	for ( j=0; j<=new.xmax-new.xmin; ++j ) {
	    cnt = countsquare(bc,i,j,linear_scale);
	    if ( cnt>=mid )
		pt[j>>3] |= (1<<(7-(j&7)));
	    else if ( cnt>=linear_scale && makesline(bc,i,j,linear_scale))
		pt[j>>3] |= (1<<(7-(j&7)));
	}
    }
    free(bc->bitmap);
    *bc = new;
}

BDFChar *SplineCharSlowerRasterize(SplineChar *sc, int pixelsize) {
    BDFChar *bc;
    int linear_scale = 1;

    if ( pixelsize<=30 )
	linear_scale = 4;
    else if ( pixelsize<=40 )
	linear_scale = 3;
    else if ( pixelsize<=60 )
	linear_scale = 2;

    bc = SplineCharRasterize(sc,pixelsize*linear_scale);
    if ( linear_scale==1 )
return( bc );
    BDFCShrinkBitmap(bc,linear_scale);
return( bc );
}

BDFFont *SplineFontRasterize(SplineFont *_sf, int pixelsize, int indicate, int slower) {
#else
BDFFont *SplineFontRasterize(SplineFont *_sf, int pixelsize, int indicate) {
#endif
    BDFFont *bdf = SplineFontToBDFHeader(_sf,pixelsize,indicate);
    int i,k;
    real scale;
    SplineFont *sf=_sf;	/* The complexity here is to pick the appropriate subfont of a CID font */

    for ( i=0; i<bdf->charcnt; ++i ) {
	if ( _sf->subfontcnt!=0 ) {
	    for ( k=0; k<_sf->subfontcnt; ++k ) if ( _sf->subfonts[k]->charcnt>i ) {
		sf = _sf->subfonts[k];
		if ( SCWorthOutputting(sf->chars[i]))
	    break;
	    }
	    scale = pixelsize / (real) (sf->ascent+sf->descent);
	}
#if 0
	bdf->chars[i] = slower ? SplineCharSlowerRasterize(sf->chars[i],pixelsize):
				SplineCharRasterize(sf->chars[i],pixelsize);
#else
	bdf->chars[i] = SplineCharRasterize(sf->chars[i],pixelsize);
#endif
	if ( indicate ) GProgressNext();
    }
    if ( indicate ) GProgressEndIndicator();
return( bdf );
}

static void BDFCAntiAlias(BDFChar *bc, int linear_scale) {
    BDFChar new;
    int i,j, max = linear_scale*linear_scale-1;
    uint8 *bpt, *pt;

    if ( bc==NULL )
return;

    memset(&new,'\0',sizeof(new));
    new.xmin = floor( ((real) bc->xmin)/linear_scale );
    new.ymin = floor( ((real) bc->ymin)/linear_scale );
    new.xmax = new.xmin + (bc->xmax-bc->xmin+linear_scale-1)/linear_scale;
    new.ymax = new.ymin + (bc->ymax-bc->ymin+linear_scale-1)/linear_scale;
    new.width = rint( ((real) bc->width)/linear_scale );

    new.bytes_per_line = (new.xmax-new.xmin+1);
    new.enc = bc->enc;
    new.sc = bc->sc;
    new.byte_data = true;
    new.depth = max==3 ? 2 : max==15 ? 4 : 8;
    new.bitmap = gcalloc( (new.ymax-new.ymin+1) * new.bytes_per_line, sizeof(uint8));
    for ( i=0; i<=bc->ymax-bc->ymin; ++i ) {
	bpt = bc->bitmap + i*bc->bytes_per_line;
	pt = new.bitmap + (i/linear_scale)*new.bytes_per_line;
	for ( j=0; j<=bc->xmax-bc->xmin; ++j ) {
	    if ( bpt[(j>>3)] & (1<<(7-(j&7))) )
		/* we can get values between 0..2^n we want values between 0..(2^n-1) */
		if ( pt[ j/linear_scale ]!=max )
		    ++pt[ j/linear_scale ];
	}
    }
    free(bc->bitmap);
    *bc = new;
}

void BDFClut(BDFFont *bdf, int linear_scale) {
    int scale = linear_scale*linear_scale, i;
    Color bg = screen_display==NULL?0xffffff:GDrawGetDefaultBackground(NULL);
    int bgr=COLOR_RED(bg), bgg=COLOR_GREEN(bg), bgb=COLOR_BLUE(bg);
    GClut *clut;

    bdf->clut = clut = gcalloc(1,sizeof(GClut));
    clut->clut_len = scale;
    clut->is_grey = (bgr==bgg && bgb==bgr);
    clut->trans_index = -1;
    for ( i=0; i<scale; ++i ) {
	clut->clut[i] =
		COLOR_CREATE( bgr- (i*(bgr))/(scale-1),
				bgg- (i*(bgg))/(scale-1),
				bgb- (i*(bgb))/(scale-1));
    }
    clut->clut[scale-1] = 0;	/* avoid rounding errors */
}

int BDFDepth(BDFFont *bdf) {
    if ( bdf->clut==NULL )
return( 1 );

return( bdf->clut->clut_len==256 ? 8 :
	bdf->clut->clut_len==16 ? 4 : 2);
}

BDFChar *SplineCharAntiAlias(SplineChar *sc, int pixelsize, int linear_scale) {
    BDFChar *bc;

    bc = SplineCharRasterize(sc,pixelsize*linear_scale);
    if ( linear_scale!=1 )
	BDFCAntiAlias(bc,linear_scale);
    BCCompressBitmap(bc);
return( bc );
}

BDFFont *SplineFontAntiAlias(SplineFont *_sf, int pixelsize, int linear_scale) {
    BDFFont *bdf;
    int i,k;
    real scale;
    char csize[10]; unichar_t size[30];
    unichar_t aa[200];
    int max;
    SplineFont *sf;	/* The complexity here is to pick the appropriate subfont of a CID font */

    if ( linear_scale==1 )
return( SplineFontRasterize(_sf,pixelsize,true));

    bdf = gcalloc(1,sizeof(BDFFont));
    sf = _sf;
    max = sf->charcnt;
    for ( i=0; i<_sf->subfontcnt; ++i ) {
	sf = _sf->subfonts[i];
	if ( sf->charcnt>max ) max = sf->charcnt;
    }
    scale = pixelsize / (real) (sf->ascent+sf->descent);

    sprintf(csize,"%d", pixelsize );
    uc_strcpy(size,csize);
    u_strcat(size,GStringGetResource(_STR_Pixels,NULL));
    u_strcpy(aa,GStringGetResource(_STR_GenAntiAlias,NULL));
    if ( sf->fontname!=NULL ) {
	uc_strcat(aa,": ");
	uc_strncat(aa,sf->fontname,sizeof(aa)/sizeof(aa[0])-u_strlen(aa));
	aa[sizeof(aa)/sizeof(aa[0])-1] = '\0';
    }
    GProgressStartIndicator(10,GStringGetResource(_STR_Rasterizing,NULL),
	    aa,size,sf->charcnt,1);
    GProgressEnableStop(0);

    if ( linear_scale>16 ) linear_scale = 16;	/* can't deal with more than 256 levels of grey */
    if ( linear_scale<=1 ) linear_scale = 2;
    bdf->sf = _sf;
    bdf->charcnt = max;
    bdf->pixelsize = pixelsize;
    bdf->chars = galloc(max*sizeof(BDFChar *));
    bdf->ascent = rint(sf->ascent*scale);
    bdf->descent = pixelsize-bdf->ascent;
    bdf->encoding_name = sf->encoding_name;
    bdf->res = -1;
    for ( i=0; i<max; ++i ) {
	if ( _sf->subfontcnt!=0 ) {
	    for ( k=0; k<_sf->subfontcnt; ++k ) if ( _sf->subfonts[k]->charcnt>i ) {
		sf = _sf->subfonts[k];
		if ( SCWorthOutputting(sf->chars[i]))
	    break;
	    }
	    scale = pixelsize / (real) (sf->ascent+sf->descent);
	}
	bdf->chars[i] = SplineCharRasterize(sf->chars[i],pixelsize*linear_scale);
	BDFCAntiAlias(bdf->chars[i],linear_scale);
	GProgressNext();
    }
    BDFClut(bdf,linear_scale);
    GProgressEndIndicator();
return( bdf );
}

BDFChar *BDFPieceMeal(BDFFont *bdf, int index) {
    SplineChar *sc = bdf->sf->chars[index];

    if ( sc==NULL )
return(NULL);
    if ( bdf->freetype_context )
	bdf->chars[index] = SplineCharFreeTypeRasterize(bdf->freetype_context,
		sc->enc,bdf->truesize,bdf->clut?8:1);
    else if ( bdf->clut )
	bdf->chars[index] = SplineCharAntiAlias(sc,bdf->truesize,4);
    else
	bdf->chars[index] = SplineCharRasterize(sc,bdf->truesize);
return( bdf->chars[index] );
}

/* Piecemeal fonts are only used as the display font in the fontview */
/*  as such they are simple fonts (ie. we only display the current cid subfont) */
BDFFont *SplineFontPieceMeal(SplineFont *sf,int pixelsize,int flags,void *ftc) {
    BDFFont *bdf = gcalloc(1,sizeof(BDFFont));
    real scale;
    int truesize = pixelsize;

    if ( flags&pf_bbsized ) {
	DBounds bb;
	SplineFontQuickConservativeBounds(sf,&bb);
	if ( bb.maxy<sf->ascent ) bb.maxy = sf->ascent;
	if ( bb.miny>-sf->descent ) bb.miny = -sf->descent;
	/* Ignore absurd values */
	if ( bb.maxy>10*(sf->ascent+sf->descent) ) bb.maxy = 2*(sf->ascent+sf->descent);
	if ( bb.maxx>10*(sf->ascent+sf->descent) ) bb.maxx = 2*(sf->ascent+sf->descent);
	if ( bb.miny<-10*(sf->ascent+sf->descent) ) bb.miny = -2*(sf->ascent+sf->descent);
	if ( bb.minx<-10*(sf->ascent+sf->descent) ) bb.minx = -2*(sf->ascent+sf->descent);
	scale = pixelsize/ (real) (bb.maxy-bb.miny);
	bdf->ascent = rint(bb.maxy*scale);
	truesize = rint( (sf->ascent+sf->descent)*scale );
    } else {
	scale = pixelsize / (real) (sf->ascent+sf->descent);
	bdf->ascent = rint(sf->ascent*scale);
    }

    bdf->sf = sf;
    bdf->charcnt = sf->charcnt;
    bdf->pixelsize = pixelsize;
    bdf->chars = gcalloc(sf->charcnt,sizeof(BDFChar *));
    bdf->descent = pixelsize-bdf->ascent;
    bdf->encoding_name = sf->encoding_name;
    bdf->piecemeal = true;
    bdf->bbsized = (flags&pf_bbsized)?1:0;
    bdf->res = -1;
    bdf->truesize = truesize;
    bdf->freetype_context = ftc;
    if ( ftc && (flags&pf_antialias) )
	BDFClut(bdf,16);
    else if ( flags&pf_antialias )
	BDFClut(bdf,4);
return( bdf );
}

void BDFCharFree(BDFChar *bdfc) {
    if ( bdfc==NULL )
return;
    free(bdfc->bitmap);
    chunkfree(bdfc,sizeof(BDFChar));
}

void BDFFontFree(BDFFont *bdf) {
    int i;

    if ( bdf==NULL )
return;
    for ( i=0; i<bdf->charcnt; ++i )
	BDFCharFree(bdf->chars[i]);
    free(bdf->chars);
    if ( bdf->clut!=NULL )
	free(bdf->clut);
    if ( bdf->freetype_context!=NULL )
	FreeTypeFreeContext(bdf->freetype_context);
    free( bdf->foundry );
    free(bdf);
}
