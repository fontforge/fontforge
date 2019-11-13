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

#include <fontforge-config.h>

#include "cvundoes.h"
#include "fontforgeui.h"
#include "splinefit.h"
#include "splineorder2.h"
#include "splinestroke.h"
#include "splineutil.h"
#include "splineutil2.h"

#include <math.h>

#undef DEBUG_FREEHAND

const int min_line_cnt = 10;		/* line segments must be at least this many datapoints to be distinguished */
const int min_line_len = 20;		/* line segments must be at least this many pixels to be distinguished */
const double line_wobble = 1.0;		/* data must not stray more than this many pixels from the true line */
const int extremum_locale = 7;		/* for something to be an extremum in x it must be an extremum for at least this many y pixels. Or vice versa */
const int extremum_cnt = 4;		/* for something to be an extremum in x it must be an extremum for at least this data samples. Or for y */

enum extreme { e_none=0, e_xmin, e_xmax, e_xflat, e_ymin, e_ymax, e_yflat };

typedef struct tracedata {
  /* First the data */
    int x,y;
    BasePoint here;
    uint32 time;
    int32 pressure, xtilt, ytilt, separation;

  /* Then overhead */
    struct tracedata *next, *prev;

    unsigned int extremum: 3;
    unsigned int use_as_pt: 1;
    unsigned int online: 1;
    unsigned int wasconstrained: 1;
    unsigned int constrained_corner: 1;
    uint16 num;
} TraceData;

static void TraceDataFree(TraceData *td) {
    TraceData *next, *first=td;

    while ( td!=NULL ) {
	next = td->next;
	chunkfree(td,sizeof(TraceData));
	td = next;
	if ( td==first )
    break;
    }
}

static void TraceDataFromEvent(CharView *cv, GEvent *event) {
    CharViewTab* tab = CVGetActiveTab(cv);
    TraceData *new;
    int skiplast;

    if ( cv->freehand.head!=NULL &&
	    cv->freehand.last->here.x==(event->u.mouse.x-tab->xoff)/tab->scale &&
	    cv->freehand.last->here.y==(cv->height-event->u.mouse.y-tab->yoff)/tab->scale ) {
	/* Has not moved */
	int constrained = (event->u.mouse.state&ksm_shift)?1:0;
	if ( constrained != cv->freehand.last->wasconstrained )
	    cv->freehand.last->constrained_corner = true;
return;
    }

    /* I sometimes seem to get events out of order on the wacom */
    skiplast = false;
    if ( cv->freehand.last!=NULL && cv->freehand.last->prev!=NULL ) {
	int x = (event->u.mouse.x-tab->xoff)/tab->scale;
	int y = (cv->height-event->u.mouse.y-tab->yoff)/tab->scale;
	TraceData *bad = cv->freehand.last, *base = bad->prev;

	if ( ((bad->here.x < x-15 && bad->here.x < base->here.x-15) ||
		(bad->here.x > x+15 && bad->here.x > base->here.x+15)) &&
	     ((bad->here.y < y-15 && bad->here.y < base->here.y-15) ||
		(bad->here.y > y+15 && bad->here.y > base->here.y+15)) )
	    skiplast = true;
    }

    if ( skiplast )
	new = cv->freehand.last;
    else {
	new = chunkalloc(sizeof(TraceData));

	if ( cv->freehand.head==NULL )
	    cv->freehand.head = cv->freehand.last = new;
	else {
	    cv->freehand.last->next = new;
	    new->prev = cv->freehand.last;
	    cv->freehand.last = new;
	}
    }

    new->here.x = (event->u.mouse.x-tab->xoff)/tab->scale;
    new->here.y = (cv->height-event->u.mouse.y-tab->yoff)/tab->scale;
    new->time = event->u.mouse.time;
    new->pressure = event->u.mouse.pressure;
    new->xtilt = event->u.mouse.xtilt;
    new->ytilt = event->u.mouse.ytilt;
    new->wasconstrained = (event->u.mouse.state&ksm_shift)?1:0;
#ifdef DEBUG_FREEHAND
 printf( "(%d,%d) (%g,%g) %d %d\n", event->u.mouse.x, event->u.mouse.y,
  new->here.x, new->here.y, new->pressure, new->time );
 if ( new->prev!=NULL && new->time<new->prev->time )
   printf( "\tAh ha!\n" );
#endif
    if ( new->wasconstrained &&
	    ( new->prev==NULL || !new->prev->wasconstrained))
	new->constrained_corner = true;
    else if ( new->prev!=NULL && !new->wasconstrained && new->prev->wasconstrained )
	new->prev->constrained_corner = true;
}

static int TraceGoodLine(TraceData *base,TraceData *end) {
    /* Make sure that we don't stray more than line_wobble pixels */
    int dx, dy;
    double diff;
    TraceData *pt;

    if (( dx = end->x-base->x )<0 ) dx = -dx;
    if (( dy = end->y-base->y )<0 ) dy = -dy;

    if ( dy>dx ) {
	dx = end->x-base->x; dy = end->y-base->y;
	for ( pt=base->next; pt!=end; pt=pt->next ) {
	    diff = (pt->y-base->y)*dx/(double) dy + base->x - pt->x;
	    if ( diff>line_wobble || diff<-line_wobble )
return( false );
	}
    } else {
	dx = end->x-base->x; dy = end->y-base->y;
	for ( pt=base->next; pt!=end; pt=pt->next ) {
	    diff = (pt->x-base->x)*dy/(double) dx + base->y - pt->y;
	    if ( diff>line_wobble || diff<-line_wobble )
return( false );
	}
    }
return( true );
}

static TraceData *TraceLineCheck(TraceData *base) {
    /* Look for a line. To be a line it must be at least min_line_len pixels */
    /*  long (or reach the end of data with no real bends) */
    /*  and we can't have strayed more than line_wobble pixels from the center */
    /*  of the line */
    TraceData *pt, *end, *last_good;
    int cnt;

    if ( base->next==NULL || base->next->next==NULL || base->next->next->next==NULL ||
	    base->wasconstrained )
return( base );

    for ( end=base->next, cnt=0; end->next!=NULL; end=end->next, ++cnt ) {
	if ( (end->x-base->x)*(end->x-base->x) + (end->y-base->y)*(end->y-base->y)>=
		min_line_len*min_line_len  &&
		cnt>=min_line_cnt )
    break;
	if ( end->wasconstrained )
return( base );
    }

    last_good = NULL;
    while ( TraceGoodLine(base,end) ) {
	last_good = end;
	if ( end->wasconstrained )
    break;
	end = end->next;
	if ( end==NULL )
    break;
    }
    if ( last_good==NULL )
return( base );			/* No good line */
    base->use_as_pt = last_good->use_as_pt = true;
    for ( pt = base; pt!=last_good; pt=pt->next )
	pt->online = true;
    last_good->online = true;
return( last_good );
}

static enum extreme TraceIsExtremum(TraceData *base) {
    TraceData *pt;
    enum extreme type;
    int i;

    if ( base->online || base->use_as_pt )
return( e_none );

    type = e_xflat;
    for ( pt = base->next, i=0; pt!=NULL && type!=e_none; pt=pt->next, ++i ) {
	if ( i>=extremum_cnt && ( pt->y-base->y > extremum_locale || pt->y-base->y < -extremum_locale ))
    break;
	if ( pt->x>base->x ) {
	    if ( type==e_xmax )
		type = e_none;
	    else
		type = e_xmin;
	} else if ( pt->x<base->x ) {
	    if ( type==e_xmin )
		type = e_none;
	    else
		type = e_xmax;
	}
    }
    for ( pt = base->prev, i=0; pt!=NULL && type!=e_none; pt=pt->prev, ++i ) {
	if ( i>=extremum_cnt && ( pt->y-base->y > extremum_locale || pt->y-base->y < -extremum_locale ))
    break;
	if ( pt->x>base->x ) {
	    if ( type==e_xmax )
		type = e_none;
	    else
		type = e_xmin;
	} else if ( pt->x<base->x ) {
	    if ( type==e_xmin )
		type = e_none;
	    else
		type = e_xmax;
	}
    }
    if ( type!=e_none )
return( type );

    type = e_yflat;
    for ( pt = base->next, i=0; pt!=NULL && type!=e_none; pt=pt->next, ++i ) {
	if ( i>=extremum_cnt && ( pt->x-base->x > extremum_locale || pt->x-base->x < -extremum_locale ))
    break;
	if ( pt->y>base->y ) {
	    if ( type==e_ymax )
		type = e_none;
	    else
		type = e_ymin;
	} else if ( pt->y<base->y ) {
	    if ( type==e_ymin )
		type = e_none;
	    else
		type = e_ymax;
	}
    }
    for ( pt = base->prev, i=0; pt!=NULL && type!=e_none; pt=pt->prev, ++i ) {
	if ( i>=extremum_cnt && ( pt->x-base->x > extremum_locale || pt->x-base->x < -extremum_locale ))
    break;
	if ( pt->y>base->y ) {
	    if ( type==e_ymax )
		type = e_none;
	    else
		type = e_ymin;
	} else if ( pt->y<base->y ) {
	    if ( type==e_ymin )
		type = e_none;
	    else
		type = e_ymax;
	}
    }

return( type );
}

static TraceData *TraceNextPoint(TraceData *pt) {
    if ( pt==NULL )
return( NULL );
    for ( pt=pt->next; pt!=NULL && !pt->use_as_pt ; pt=pt->next );
return( pt );
}

static void TraceMassage(TraceData *head, TraceData *end) {
    TraceData *pt, *npt, *last, *nnpt;
    double nangle, pangle;
    /* Look for corners that are too close together */
    /* Paired tangents that might better be a single curve */
    /* tangent corner tangent that might better be a single curve */

    for ( pt=head ; pt!=NULL ; pt=npt ) {
	npt = TraceNextPoint(pt);
	for (;;) {
	    nnpt = TraceNextPoint(npt);
	    if ( nnpt!=NULL &&
		    ((pt->x==npt->x && npt->x==nnpt->x) ||
		     (pt->y==npt->y && npt->y==nnpt->y)) &&
		    (pt->next == npt || pt->next->online) ) {
		/*npt->online = true;*/
		npt->use_as_pt = false;
	    } else if ( nnpt==NULL && npt!=NULL && (pt->x==npt->x || pt->y==npt->y) &&
		    (pt->next == npt || pt->next->online) ) {
		pt->use_as_pt = false;
	    } else
	break;
	    npt = nnpt;
	}
    }

    /* Remove very short segments */
    if ( head && head->next ) {
	for ( pt=head->next; pt!=NULL && pt->use_as_pt && !pt->constrained_corner; pt = pt->next )
	    pt->use_as_pt = false;
	for ( pt=head->next ; pt->next!=NULL ; pt=pt->next ) {
	    if ( pt->use_as_pt && pt->next->use_as_pt && !pt->constrained_corner )
		pt->use_as_pt = false;
	}
    }
    for ( pt=head ; pt!=NULL ; pt=npt ) {
	npt = pt;
	for (;;) {
	    npt = TraceNextPoint(pt);
	    if ( npt == NULL || npt->constrained_corner )
	break;
	    if ( (npt->x-pt->x)*(npt->x-pt->x) + (npt->y-pt->y)*(npt->y-pt->y)<4 )
		npt->use_as_pt = false;
	    else
	break;
	}
    }

    last = NULL;
    for ( pt=head ; pt!=NULL ; pt=pt->next ) {
	if ( pt->use_as_pt && pt->next!=NULL && pt->next->online && !pt->next->wasconstrained &&
		pt->prev!=NULL && !pt->prev->online ) {
	    npt = TraceNextPoint(pt);
	    if ( npt==NULL )
    break;
	    if ( npt->wasconstrained )
		/* Leave it */;
	    else if ( npt->next!=NULL && !npt->next->online ) {
		/* We've got two adjacent tangents */
		nnpt = TraceNextPoint(npt);
		if ( last!=NULL && nnpt!=NULL ) {
		    int lencur = (npt->x-pt->x)*(npt->x-pt->x) + (npt->y-pt->y)*(npt->y-pt->y);
		    int lennext = (nnpt->x-npt->x)*(nnpt->x-npt->x) + (nnpt->y-npt->y)*(nnpt->y-npt->y);
		    int lenprev = (npt->x-pt->x)*(npt->x-pt->x) + (npt->y-pt->y)*(npt->y-pt->y);
		    if ( lencur<16*lennext || lencur<16*lenprev ) {
			/* Probably better done as one point */
			int which = (pt->num+npt->num)/2;
			for (;;) {
			    pt->online = pt->use_as_pt = false;
			    if ( pt->num==which )
				pt->use_as_pt = true;
			    if ( pt==npt )
			break;
			    pt = pt->next;
			}
		    }
		}
	    } else if ( (nnpt = TraceNextPoint(npt))!=NULL &&
		    nnpt->next && !nnpt->next->online ) {
		/* We've got tangent corner tangent */
		pangle = atan2(npt->y-pt->y,npt->x-pt->x);
		nangle = atan2(nnpt->y-npt->y,nnpt->x-npt->x);
		if ( pangle<0 && nangle>0 && nangle-pangle>=FF_PI )
		    pangle += 2*FF_PI;
		else if ( pangle>0 && nangle<0 && pangle-nangle>=FF_PI )
		    nangle += 2*FF_PI;
		if ( nangle-pangle<1 && nangle-pangle>-1 ) {
		    for (;;) {
			pt->online = pt->use_as_pt = false;
			if ( pt==nnpt )
		    break;
			pt = pt->next;
		    }
		    pt = npt;
		    pt->use_as_pt = true;
		}
	    }
	}
	if ( pt->use_as_pt )
	    last = pt;
    }
    head->use_as_pt = end->use_as_pt = true;
}

static bigreal Trace_Factor(void *_cv,Spline *spline, real t) {
    CharView *cv = (CharView *) _cv;
    TraceData *head = cv->freehand.head, *pt, *from=NULL, *to=NULL;
    int fromnum = spline->from->ptindex, tonum = spline->to->ptindex;
    StrokeInfo *si = CVFreeHandInfo();
    int p;

    if ( si->width<=0 || si->pressure1==si->pressure2 )
return( 1.0 );

    for ( pt = head; pt!=NULL; pt=pt->next ) {
	if ( fromnum == pt->num ) {
	    from = pt;
	    if ( to!=NULL )
    break;
	}
	if ( tonum == pt->num ) {
	    to = pt;
	    if ( from!=NULL )
    break;
	}
    }
    if ( from==NULL || to==NULL ) {
	fprintf( stderr, "Not found\n" );
return( 1.0 );
    }
    p = from->pressure*t + to->pressure*(1-t);
    
    if ( p<=si->pressure1 && p<=si->pressure2 ) {
	if ( si->pressure1<si->pressure2 )
return( 1.0 );
	else
return( si->radius2/(si->width/2) );
    } else if ( p>=si->pressure1 && p>=si->pressure2 ) {
	if ( si->pressure1<si->pressure2 )
return( si->radius2/(si->width/2) );
	else
return( 1.0 );
    } else
return( ((p-si->pressure1)*si->radius2 + (si->pressure2-p)*(si->width/2))/
		((si->width/2)*(si->pressure2-si->pressure1)) );
}

static void TraceFigureCPs(SplinePoint *last,SplinePoint *cur,TraceData *tlast,TraceData *tcur) {
    TraceData *pt;

    last->nonextcp = false; cur->noprevcp=false;

    for ( pt=tlast; !pt->use_as_pt ; pt=pt->prev );
    if ( pt->online ) {
	last->nextcp.x = last->me.x + ( tlast->x-pt->x );
	last->nextcp.y = last->me.y + ( tlast->y-pt->y );
    } else if ( tlast->extremum==e_xmin || tlast->extremum==e_xmax || tlast->extremum==e_xflat ) {
	last->nextcp.x = last->me.x;
	last->nextcp.y = cur->me.y>last->me.y ? last->me.y+1 : last->me.y-1;
    } else if ( tlast->extremum==e_ymin || tlast->extremum==e_ymax || tlast->extremum==e_yflat ) {
	last->nextcp.y = last->me.y;
	last->nextcp.x = cur->me.x>last->me.x ? last->me.x+1 : last->me.x-1;
    } else if ( !last->noprevcp && !tlast->constrained_corner ) {
	last->nextcp.x = last->me.x + (last->me.x - last->prevcp.x);
	last->nextcp.y = last->me.y + (last->me.y - last->prevcp.y);
    } else
	last->nonextcp = true;

    if ( tcur->online ) {
	for ( pt=tcur; !pt->use_as_pt ; pt=pt->next );
	cur->prevcp.x = cur->me.x + ( tcur->x-pt->x );
	cur->prevcp.y = cur->me.y + ( tcur->y-pt->y );
    } else if ( tcur->extremum==e_xmin || tcur->extremum==e_xmax || tcur->extremum==e_xflat ) {
	cur->prevcp.x = cur->me.x;
	cur->prevcp.y = last->me.y>cur->me.y ? cur->me.y+1 : cur->me.y-1;
    } else if ( tcur->extremum==e_ymin || tcur->extremum==e_ymax || tcur->extremum==e_yflat ) {
	cur->prevcp.y = cur->me.y;
	cur->prevcp.x = last->me.x>cur->me.x ? cur->me.x+1 : cur->me.x-1;
    } else if ( !cur->nonextcp && !tcur->constrained_corner ) {	/* Only happens at end of a closed contour */
	cur->prevcp.x = cur->me.x + (cur->me.x - cur->nextcp.x);
	cur->prevcp.y = cur->me.y + (cur->me.y - cur->nextcp.y);
    } else
	cur->noprevcp = true;
}

static SplineSet *TraceCurve(CharView *cv) {
    CharViewTab* tab = CVGetActiveTab(cv);
    TraceData *head = cv->freehand.head, *pt, *base, *e;
    SplineSet *spl;
    SplinePoint *last, *cur;
    int cnt, i, tot;
    FitPoint *mids;
    double len,sofar;
    StrokeInfo *si;

    /* First we look for straight lines in the data. We will put SplinePoints */
    /*  at their endpoints */
    /* Then we find places that are local extrema, or just flat in x,y */
    /*  SplinePoints here too. */
    /* Then approximate splines between */

    cnt = 0;
    for ( pt=head; pt!=NULL; pt = pt->next ) {
	pt->extremum = e_none;
	pt->online = pt->wasconstrained;
	pt->use_as_pt = pt->constrained_corner;
	/* We recalculate x,y because we might have autoscrolled the window */
	pt->x =  tab->xoff + rint(pt->here.x*tab->scale);
	pt->y = -tab->yoff + cv->height - rint(pt->here.y*tab->scale);
	pt->num = cnt++;
    }
    head->use_as_pt = cv->freehand.last->use_as_pt = true;

    /* Look for lines */
    for ( pt=head->next ; pt!=NULL; pt=pt->next )
	pt = TraceLineCheck(pt);

    /* Look for extremum */
    for ( pt=head->next ; pt!=NULL; pt=pt->next )
	pt->extremum = TraceIsExtremum(pt);
    /* Find the middle of a range of extremum points, that'll be the one we want */
    for ( base=head->next; base!=NULL; base = base->next ) {
	if ( base->extremum>=e_xmin ) {
	    tot = 1;
	    if ( base->extremum>=e_ymin ) {
		for ( pt=base->next ; pt->extremum>=e_ymin ; pt = pt->next ) ++tot;
	    } else {
		for ( pt=base->next ; pt->extremum>=e_xmin && pt->extremum<e_ymin ; pt = pt->next ) ++tot;
	    }
	    tot /= 2;
	    e = pt;
	    for ( pt=base, i=0 ; i<tot ; pt = pt->next, ++i );
	    pt->use_as_pt = true;
	    base = e;
	}
    }

    TraceMassage(head,cv->freehand.last);

    /* Calculate the mids array */
    mids = malloc(cnt*sizeof(FitPoint));
    for ( base=head; base!=NULL && base->next!=NULL; base = pt ) {
	mids[base->num].p.x = base->here.x;
	mids[base->num].p.y = base->here.y;
	mids[base->num].t = 0;
	len = 0;
	if ( base->next->online ) {
	    pt = base->next;	/* Don't bother to calculate the length, we won't use the data */
    continue;
	}
	for ( pt=base->next; ; pt=pt->next ) {
	    len += sqrt((double) (
		    (pt->x-pt->prev->x)*(pt->x-pt->prev->x) +
		    (pt->y-pt->prev->y)*(pt->y-pt->prev->y) ));
	    if ( pt->use_as_pt )
	break;
	}
	sofar = 0;
	for ( pt=base->next; ; pt=pt->next ) {
	    sofar += sqrt((double) (
		    (pt->x-pt->prev->x)*(pt->x-pt->prev->x) +
		    (pt->y-pt->prev->y)*(pt->y-pt->prev->y) ));
	    mids[pt->num].p.x = pt->here.x;
	    mids[pt->num].p.y = pt->here.y;
	    mids[pt->num].t = sofar/len;
	    if ( pt->use_as_pt )
	break;
	}
    }

    /* Splice things together */
    spl = chunkalloc(sizeof(SplineSet));
    spl->first = last = SplinePointCreate(rint(head->here.x),rint(head->here.y));
    last->ptindex = 0;

    for ( base=head; base!=NULL && base->next!=NULL; base = pt ) {
	for ( pt=base->next; !pt->use_as_pt ; pt=pt->next );
	cur = SplinePointCreate(rint(pt->here.x),rint(pt->here.y));
	cur->ptindex = pt->num;
	/* even if we are order2, do everything in order3, and then convert */
	/*  when they raise the pen */
	if ( base->next->online || base->next==pt )
	    SplineMake(last,cur,false);
	else {
	    TraceFigureCPs(last,cur,base,pt);
	    if ( !last->nonextcp && !cur->noprevcp )
		ApproximateSplineFromPointsSlopes(last,cur,mids+base->num+1,pt->num-base->num-1,false);
	    else {
		last->nonextcp = false; cur->noprevcp=false;
		ApproximateSplineFromPoints(last,cur,mids+base->num+1,pt->num-base->num-1,false);
	    }
	}
	last = cur;
    }
    spl->last = last;

    /* Now we've got a rough approximation to the contour, but the joins are */
    /*  probably not smooth. Clean things up a bit... */
    if ( spl->first->nonextcp )
	spl->first->pointtype = pt_corner;
    else
	spl->first->pointtype = pt_curve;
    if ( spl->last->noprevcp )
	spl->last->pointtype = pt_corner;
    else
	spl->last->pointtype = pt_curve;

    free(mids);

    si = CVFreeHandInfo();
    if ( si->stroke_type!=si_centerline ) {
	si->factor = ( si->pressure1==si->pressure2 ) ? NULL : Trace_Factor;
	si->data = cv;
	spl->next = SplineSetStroke(spl,si,false);
    }
return( spl );
}

static void TraceDataClose(CharView *cv,GEvent *event) {
    TraceData *new;
    SplineSet *trace;
    double langle, hangle, llen, hlen;
    double dx,dy;
    BasePoint oldp, oldn;

    if ( cv->freehand.head==NULL )
return; /* Eh? No points? How did that happen? */
    if ( cv->freehand.head->here.x!=cv->freehand.last->here.x ||
	     cv->freehand.head->here.y!=cv->freehand.last->here.y ) {
	new = chunkalloc(sizeof(TraceData));
	*new = *cv->freehand.head;
	new->time = event->u.mouse.time;
	new->wasconstrained = (event->u.mouse.state&ksm_shift)?1:0;

	new->prev = cv->freehand.last;
	new->next = NULL;
	cv->freehand.last->next = new;
	cv->freehand.last = new;
	SplinePointListsFree(cv->freehand.current_trace);
	cv->freehand.current_trace = TraceCurve(cv);
    } else if ( cv->freehand.head == cv->freehand.last )
return;			/* Only one point, no good way to close it */

    /* Now the first and last points are at the same location. We need to: */
    /*  average their control points (so that they form a smooth curve) */
    /*  merge the two SplinePoints into one */

    trace = cv->freehand.current_trace;
    trace->first->prevcp = trace->last->prevcp;
    trace->first->noprevcp = trace->last->noprevcp;
    trace->first->prevcpdef = trace->last->prevcpdef;
    trace->first->prev = trace->last->prev;
    trace->first->prev->to = trace->first;
    SplinePointFree(trace->last);
    trace->last = trace->first;
    
    if ( cv->freehand.head->wasconstrained  || cv->freehand.last->wasconstrained )
return;

    trace->first->pointtype = pt_curve;

    if ( trace->first->nonextcp && trace->first->noprevcp ) {
	SplineCharDefaultPrevCP(trace->first);
	SplineCharDefaultNextCP(trace->first);
return;
    }

    dx = trace->first->nextcp.x-trace->first->me.x;
    dy = trace->first->nextcp.y-trace->first->me.y;
    hlen = sqrt(dx*dx+dy*dy);
    hangle = atan2(dy,dx);
    oldn = trace->first->nextcp;
    dx = trace->first->me.x-trace->first->prevcp.x;
    dy = trace->first->me.y-trace->first->prevcp.y;
    llen = sqrt(dx*dx+dy*dy);
    langle = atan2(dy,dx);
    oldp = trace->first->prevcp;

    if ( trace->first->nonextcp ) {
	SplineCharDefaultPrevCP(trace->first);
	dx = trace->first->me.x-trace->first->prevcp.x;
	dy = trace->first->me.y-trace->first->prevcp.y;
	llen = sqrt(dx*dx+dy*dy);
	trace->first->prevcp.x = trace->first->me.x -
		cos(hangle)*llen;
	trace->first->prevcp.y = trace->first->me.y -
		sin(hangle)*llen;
	trace->first->nextcp = oldn;
    } else if ( trace->first->noprevcp ) {
	SplineCharDefaultPrevCP(trace->first);
	dx = trace->first->me.x-trace->first->nextcp.x;
	dy = trace->first->me.y-trace->first->nextcp.y;
	hlen = sqrt(dx*dx+dy*dy);
	trace->first->nextcp.x = trace->first->me.x +
		cos(langle)*hlen;
	trace->first->nextcp.y = trace->first->me.y +
		sin(langle)*hlen;
	trace->first->prevcp = oldp;
    } else {
	if ( hangle>FF_PI/2 && langle<-FF_PI/2 )
	    langle += 2*FF_PI;
	if ( hangle<-FF_PI/2 && langle>FF_PI/2 )
	    hangle += 2*FF_PI;
	hangle = (hangle+langle)/2;
	dx = cos(hangle);
	dy = sin(hangle);
	trace->first->prevcp.x = trace->first->me.x - dx*llen;
	trace->first->prevcp.y = trace->first->me.y - dy*llen;
	trace->first->nextcp.x = trace->first->me.x + dx*hlen;
	trace->first->nextcp.y = trace->first->me.y + dy*hlen;
    }
    SplineRefigure(trace->first->next);
    SplineRefigure(trace->first->prev);
}

static int TraceDataCleanup(CharView *cv) {
    TraceData *mid, *next;
    int cnt=0;

    for ( mid = cv->freehand.head; mid!=NULL; mid=next ) {
	++cnt;
	next = mid->next;
    }
return( cnt );
}

void CVMouseDownFreeHand(CharView *cv, GEvent *event) {

    TraceDataFree(cv->freehand.head);
    cv->freehand.head = cv->freehand.last = NULL;
    cv->freehand.current_trace = NULL;
    TraceDataFromEvent(cv,event);

    cv->freehand.current_trace = chunkalloc(sizeof(SplinePointList));
    cv->freehand.current_trace->first = cv->freehand.current_trace->last =
	    SplinePointCreate(rint(cv->freehand.head->here.x),rint(cv->freehand.head->here.y));
}

void CVMouseMoveFreeHand(CharView *cv, GEvent *event) {
    CharViewTab* tab = CVGetActiveTab(cv);
    double dx, dy;
    SplinePoint *last;
    BasePoint *here;

    TraceDataFromEvent(cv,event);
    /* I used to do the full processing here to get a path. But that took */
    /*  too long to process them and we appeared to get events out of order */
    /*  from the wacom tablet */
    last = cv->freehand.current_trace->last;
    here = &cv->freehand.last->here;
    if ( (dx=here->x-last->me.x)<0 ) dx = -dx;
    if ( (dy=here->y-last->me.y)<0 ) dy = -dy;
    if ( (dx+dy)*tab->scale > 4 ) {
	SplineMake3(last,SplinePointCreate(rint(here->x),rint(here->y)));
	cv->freehand.current_trace->last = last->next->to;
	GDrawRequestExpose(cv->v,NULL,false);
    }
}

#ifdef DEBUG_FREEHAND
static void RepeatFromFile(CharView *cv) {
    FILE *foo = fopen("mousemove","r");
    /*char buffer[100];*/
    GEvent e;
    int x,y,p,t;

    if ( foo==NULL )
return;

    memset(&e,0,sizeof(e));

    e.w = cv->v;
    e.type = et_mousedown;
    fscanf(foo,"(%d,%d) (%*g,%*g) %d %d\n", &x, &y, &p, &t);
    e.u.mouse.x = x; e.u.mouse.y = y; e.u.mouse.pressure = p; e.u.mouse.time = t;
    CVMouseDownFreeHand(cv,&e);

    e.type = et_mousemove;
    while ( fscanf(foo,"(%d,%d) (%*g,%*g) %d %d\n", &x, &y, &p, &t)==4 ) {
	e.u.mouse.x = x; e.u.mouse.y = y; e.u.mouse.pressure = p; e.u.mouse.time = t;
	CVMouseMoveFreeHand(cv,&e);
    }

    e.type = et_mouseup;
    CVMouseUpFreeHand(cv,&e);
    fclose(foo);
}
#endif

void CVMouseUpFreeHand(CharView *cv, GEvent *event) {
    CharViewTab* tab = CVGetActiveTab(cv);
    TraceData *head = cv->freehand.head;
    TraceData *last;
    double dx, dy;

#ifdef DEBUG_FREEHAND
    if ( event->u.mouse.clicks>1 ) {
	TraceDataFree(cv->freehand.head);
	cv->freehand.head = cv->freehand.last = NULL;
	RepeatFromFile(cv);
return;
    }
#endif
    if ( head==NULL )
return;

    if ( TraceDataCleanup(cv)>=4 ) {
	/* If there are fewer than 4 points then assume they made a mistake */
	/*  (or intended a double click) and just ignore the path */
	last = cv->freehand.last;
	if ( (dx=head->x-last->x)<0 ) dx = -dx;
	if ( (dy=head->y-last->y)<0 ) dy = -dy;

	if (( event->u.chr.state&ksm_meta ) || (dx+dy)*tab->scale > 4 )
	    TraceDataClose(cv,event);
	else {
	    SplinePointListsFree(cv->freehand.current_trace);
	    cv->freehand.current_trace = TraceCurve(cv);
	}
	if ( cv->freehand.current_trace!=NULL ) {
	    CVPreserveState((CharViewBase *) cv);
	    if ( cv->b.layerheads[cv->b.drawmode]->order2 )
		cv->freehand.current_trace = SplineSetsTTFApprox(cv->freehand.current_trace);
	    if ( CVFreeHandInfo()->stroke_type==si_centerline ) {
		cv->freehand.current_trace->next = cv->b.layerheads[cv->b.drawmode]->splines;
		cv->b.layerheads[cv->b.drawmode]->splines = cv->freehand.current_trace;
	    } else {
		SplineSet *ss = cv->freehand.current_trace;
		while ( ss->next!=NULL )
		    ss = ss->next;
		ss->next = cv->b.layerheads[cv->b.drawmode]->splines;
		cv->b.layerheads[cv->b.drawmode]->splines = cv->freehand.current_trace->next;
		cv->freehand.current_trace->next = NULL;
		/*SplinePointListsFree(cv->freehand.current_trace);*/
	    }
	    cv->freehand.current_trace = NULL;
	}
    } else {
	SplinePointListsFree(cv->freehand.current_trace);
	cv->freehand.current_trace = NULL;
    }
    TraceDataFree(cv->freehand.head);
    cv->freehand.head = cv->freehand.last = NULL;
    CVCharChangedUpdate(&cv->b);
#ifdef DEBUG_FREEHAND
    fflush( stdout );
#endif
}
