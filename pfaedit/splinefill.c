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
#include <string.h>
#include <ustring.h>
#include <math.h>
#include "gdraw.h"
#include "splinefont.h"
#include "edgelist.h"

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
}

double TOfNextMajor(Edge *e, EdgeList *es, double sought_m ) {
    /* We want to find t so that Mspline(t) = sought_m */
    /*  the curve is monotonic */
    Spline *sp = e->spline;
    Spline1D *msp = &e->spline->splines[es->major];
    double slope = (3.0*msp->a*e->t_cur+2.0*msp->b)*e->t_cur + msp->c;
    double new_t, old_t, newer_t;
    double found_m;
    double t_mmax, t_mmin;
    double mmax, mmin;

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

static void AddEdge(EdgeList *es, Spline *sp, double tmin, double tmax ) {
    Edge *e, *pr;
    double m1, m2;
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
    if ( DoubleNear(e->mmin,es->mmin)) e->mmin = es->mmin;
    e->o_mmin = ( ((osp->a*e->t_mmin+osp->b)*e->t_mmin+osp->c)*e->t_mmin + osp->d ) * es->scale;
    e->mmin -= es->mmin; e->mmax -= es->mmin;
    e->t_cur = e->t_mmin;
    e->o_cur = e->o_mmin;
    e->m_cur = e->mmin;
    e->last_opos = e->last_mpos = -2;

    if ( e->mmin<0 || e->mmin>=e->mmax )
	GDrawIError("Grg!");

    if ( es->sc!=NULL ) for ( hint=es->sc->hstem; hint!=NULL; hint=hint->next ) {
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
	    (e->o_cur==es->edges[mpos]->o_cur &&
	     e->o_cur>( ((osp->a*e->t_mmax+osp->b)*e->t_mmax+osp->c)*e->t_mmax + osp->d ) * es->scale )) {
	e->esnext = es->edges[mpos];
	es->edges[mpos] = e;
    } else {
	for ( pr=es->edges[mpos]; pr->esnext!=NULL && pr->esnext->o_cur<e->o_cur ;
		pr = pr->esnext );
	/* When two splines share a vertex which is a local minimum, then */
	/*  o_cur will be equal for both (to the vertex's o value) and so */
	/*  the above code randomly picked one to go first. That screws up */
	/*  the overlap code, which wants them properly ordered from the */
	/*  start. so look at the end point */
	if ( pr->esnext!=NULL && pr->esnext->o_cur==e->o_cur ) {
	    if ( e->o_cur<( ((osp->a*e->t_mmax+osp->b)*e->t_mmax+osp->c)*e->t_mmax + osp->d ) * es->scale )
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
    double m1;
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
    double t1=2, t2=2, t;
    double b2_fourac;
    double fm, tm;
    Spline1D *msp = &sp->splines[es->major], *osp = &sp->splines[es->other];

    /* Find the points of inflection on the curve discribing y behavior */
    if ( !DoubleNear(msp->a,0) ) {
	/* cubic, possibly 2 inflections (possibly none) */
	b2_fourac = 4*msp->b*msp->b - 12*msp->a*msp->c;
	if ( b2_fourac>=0 ) {
	    b2_fourac = sqrt(b2_fourac);
	    t1 = (-2*msp->b - b2_fourac) / (6*msp->a);
	    t2 = (-2*msp->b + b2_fourac) / (6*msp->a);
	    if ( t1>t2 ) { double temp = t1; t1 = t2; t2 = temp; }
	    else if ( t1==t2 ) t2 = 2.0;

	    /* check for curves which have such a small slope they might */
	    /*  as well be horizontal */
	    fm = es->major==1?sp->from->me.y:sp->from->me.x;
	    tm = es->major==1?sp->to->me.y:sp->to->me.x;
	    if ( fm==tm ) {
		double m1, m2, d1, d2;
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
    } else if ( !DoubleNear(msp->b,0) ) {
	/* Quadratic, at most one inflection */
	t1 = -msp->c/(2.0*msp->b);
    } else if ( !DoubleNear(msp->c,0) ) {
	/* linear, no points of inflection */
    } else {
	sp->ishorvert = true;
	if ( es->genmajoredges )
	    AddMajorEdge(es,sp);
return;		/* Horizontal line, ignore it */
    }

    if ( DoubleNear(t1,0)) t1=0;
    if ( DoubleNear(t1,1)) t1=1;
    if ( DoubleNear(t2,0)) t2=0;
    if ( DoubleNear(t2,1)) t2=1;
    if ( DoubleNear(t1,t2)) t2=2;
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
	double ot1, ot2;
	int mpos;
	SplineFindInflections(osp,&ot1,&ot2);
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

Edge *ActiveEdgesRefigure(EdgeList *es, Edge *active,double i) {
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

Edge *ActiveEdgesFindStem(Edge *apt, Edge **prev, double i) {
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

static int isvstem(EdgeList *es,double stem,int *vval) {
    Hints *hint;

    for ( hint=es->sc->vstem; hint!=NULL ; hint=hint->next ) {
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
    Hints *hint;
    double t1, t2;
    int k,end,width;

    /* we only care about hstem hints, and only if they fail to cross a */
    /*  vertical pixel boundary. If that happens, adjust either the top */
    /*  or bottom position so that a boundary forcing is crossed.   Any */
    /*  vertexes at those points will be similarly adjusted later... */

    for ( hint=sc->hstem; hint!=NULL; hint=hint->next ) {
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
    for ( hint=sc->vstem; hint!=NULL; hint=hint->next ) {
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

void BCCompressBitmap(BDFChar *bdfc) {
    /* Now we may have allocated a bit more than we need to the bitmap */
    /*  check to see if there are any unused rows or columns.... */
    int i,j,any, off, last;

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
}

/* After a bitmap has been compressed, it's sizes may not complie with the */
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

BDFChar *SplineCharRasterize(SplineChar *sc, int pixelsize) {
    EdgeList es;
    DBounds b;
    BDFChar *bdfc;

    if ( sc==NULL )
return( NULL );
    memset(&es,'\0',sizeof(es));
    if ( sc==NULL ) {
	es.mmin = es.mmax = es.omin = es.omax = 0;
	es.bitmap = gcalloc(1,1);
	es.bytes_per_line = 1;
    } else {
	SplineCharFindBounds(sc,&b);
	es.scale = (pixelsize-.3) / (double) (sc->parent->ascent+sc->parent->descent);
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
    }

    bdfc = gcalloc(1,sizeof(BDFChar));
    bdfc->sc = sc;
    bdfc->xmin = rint(es.omin);
    bdfc->ymin = es.mmin;
    bdfc->xmax = (int) ceil(es.omax-es.omin) + bdfc->xmin;
    bdfc->ymax = es.mmax;
    if ( sc!=NULL ) {
	bdfc->width = rint(sc->width*pixelsize / (double) (sc->parent->ascent+sc->parent->descent));
	bdfc->enc = sc->enc;
    }
    bdfc->bitmap = es.bitmap;
    bdfc->bytes_per_line = es.bytes_per_line;
    BCCompressBitmap(bdfc);
    FreeEdges(&es);
return( bdfc );
}

static unichar_t rasterizing[] = { 'R','a','s','t','e','r','i','z','i','n','g','.','.','.',  '\0' };
static unichar_t bitmap[] = { 'G','e','n','e','r','a','t','i','n','g',' ','b','i','t','m','a','p',' ','f','o','n','t',  '\0' };
static unichar_t aamap[] = { 'G','e','n','e','r','a','t','i','n','g',' ','a','n','t','i','-','a','l','i','a','s','e','d',' ','f','o','n','t',  '\0' };
BDFFont *SplineFontRasterize(SplineFont *sf, int pixelsize) {
    BDFFont *bdf = gcalloc(1,sizeof(BDFFont));
    int i;
    double scale = pixelsize / (double) (sf->ascent+sf->descent);
    char csize[10]; unichar_t size[10];

    sprintf(csize,"%d pixels", pixelsize );
    uc_strcpy(size,csize);
    GProgressStartIndicator(10,rasterizing,bitmap,size,sf->charcnt,1);
    GProgressEnableStop(0);
    bdf->sf = sf;
    bdf->charcnt = sf->charcnt;
    bdf->pixelsize = pixelsize;
    bdf->chars = galloc(sf->charcnt*sizeof(BDFChar *));
    bdf->ascent = rint(sf->ascent*scale);
    bdf->descent = pixelsize-bdf->ascent;
    bdf->encoding_name = sf->encoding_name;
    for ( i=0; i<sf->charcnt; ++i ) {
	bdf->chars[i] = SplineCharRasterize(sf->chars[i],pixelsize);
	GProgressNext();
    }
    GProgressEndIndicator();
return( bdf );
}

static void BDFCAntiAlias(BDFChar *bc, int linear_scale) {
    BDFChar new;
    int i,j, max = linear_scale*linear_scale-1;
    uint8 *bpt, *pt;

    if ( bc==NULL )
return;

#if 1
    memset(&new,'\0',sizeof(new));
    new.xmin = floor( ((double) bc->xmin)/linear_scale );
    new.ymin = floor( ((double) bc->ymin)/linear_scale );
    new.xmax = new.xmin + (bc->xmax-bc->xmin+linear_scale-1)/linear_scale;
    new.ymax = new.ymin + (bc->ymax-bc->ymin+linear_scale-1)/linear_scale;
    new.width = rint( ((double) bc->width)/linear_scale );

    new.bytes_per_line = (new.xmax-new.xmin+1);
    new.enc = bc->enc;
    new.sc = bc->sc;
    new.byte_data = true;
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
#else
    memset(&new,'\0',sizeof(new));
    new.xmin = floor( ((double) bc->xmin)/linear_scale );
    new.ymin = floor( ((double) bc->ymin)/linear_scale );
    new.xmax = ceil( ((double) bc->xmax)/linear_scale );
    new.ymax = ceil( ((double) bc->ymax)/linear_scale );
    new.width = rint( ((double) bc->width)/linear_scale );

    new.bytes_per_line = (new.xmax-new.xmin+1);
    new.enc = bc->enc;
    new.sc = bc->sc;
    new.byte_data = true;
    new.bitmap = gcalloc( (new.ymax-new.ymin+1) * new.bytes_per_line, sizeof(uint8));
    for ( i=bc->ymin; i<=bc->ymax; ++i ) {
	bpt = bc->bitmap + (i-bc->ymin)*bc->bytes_per_line;
	pt = new.bitmap + (int) (floor( ((double) i)/linear_scale )-new.ymin)*new.bytes_per_line;
	for ( j=bc->xmin; j<bc->xmax; ++j ) {
	    if ( bpt[(j>>3)] & (1<<(7-(j&7))) )
		++pt[ (int) ((floor( ((double) j)/linear_scale )-new.xmin)) ];
	}
    }
#endif
    free(bc->bitmap);
    *bc = new;
}

static void BDFClut(BDFFont *bdf, int linear_scale) {
    int scale = linear_scale*linear_scale, i;
    Color bg = GDrawGetDefaultBackground(NULL);
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

BDFChar *SplineCharAntiAlias(SplineChar *sc, int pixelsize, int linear_scale) {
    BDFChar *bc;

    bc = SplineCharRasterize(sc,pixelsize*linear_scale);
    BDFCAntiAlias(bc,linear_scale);
return( bc );
}

BDFFont *SplineFontAntiAlias(SplineFont *sf, int pixelsize, int linear_scale) {
    BDFFont *bdf = gcalloc(1,sizeof(BDFFont));
    int i;
    double scale = pixelsize / (double) (sf->ascent+sf->descent);
    char csize[10]; unichar_t size[10];

    sprintf(csize,"%d pixels", pixelsize );
    uc_strcpy(size,csize);
    GProgressStartIndicator(10,rasterizing,aamap,size,sf->charcnt,1);

    if ( linear_scale>16 ) linear_scale = 16;	/* can't deal with more than 256 levels of grey */
    if ( linear_scale<=1 ) linear_scale = 2;
    bdf->sf = sf;
    bdf->charcnt = sf->charcnt;
    bdf->pixelsize = pixelsize;
    bdf->chars = galloc(sf->charcnt*sizeof(BDFChar *));
    bdf->ascent = rint(sf->ascent*scale);
    bdf->descent = pixelsize-bdf->ascent;
    bdf->encoding_name = sf->encoding_name;
    for ( i=0; i<sf->charcnt; ++i ) {
	bdf->chars[i] = SplineCharRasterize(sf->chars[i],pixelsize*linear_scale);
	BDFCAntiAlias(bdf->chars[i],linear_scale);
	GProgressNext();
    }
    BDFClut(bdf,linear_scale);
    GProgressEndIndicator();
return( bdf );
}

void BDFCharFree(BDFChar *bdfc) {
    if ( bdfc==NULL )
return;
    free(bdfc->bitmap);
    free(bdfc);
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
    free(bdf);
}
