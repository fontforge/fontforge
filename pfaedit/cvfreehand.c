/* Copyright (C) 2002 by George Williams */
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
    TraceData *next;

    while ( td!=NULL ) {
	next = td->next;
	chunkfree(td,sizeof(TraceData));
	td = next;
    }
}

static void TraceDataFromEvent(CharView *cv, GEvent *event) {
    TraceData *new;

    if ( cv->freehand.head!=NULL &&
	    cv->freehand.last->here.x==(event->u.mouse.x-cv->xoff)/cv->scale &&
	    cv->freehand.last->here.y==(cv->height-event->u.mouse.y-cv->yoff)/cv->scale ) {
	/* Has not moved */
	int constrained = (event->u.mouse.state&ksm_shift)?1:0;
	if ( constrained != cv->freehand.last->wasconstrained )
	    cv->freehand.last->constrained_corner = true;
return;
    }

    new = chunkalloc(sizeof(TraceData));

    if ( cv->freehand.head==NULL )
	cv->freehand.head = cv->freehand.last = new;
    else {
	cv->freehand.last->next = new;
	new->prev = cv->freehand.last;
	cv->freehand.last = new;
    }

    new->here.x = (event->u.mouse.x-cv->xoff)/cv->scale;
    new->here.y = (cv->height-event->u.mouse.y-cv->yoff)/cv->scale;
    new->time = event->u.mouse.time;
    new->pressure = event->u.mouse.pressure;
    new->xtilt = event->u.mouse.xtilt;
    new->ytilt = event->u.mouse.ytilt;
    new->wasconstrained = (event->u.mouse.state&ksm_shift)?1:0;
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
	forever {
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
	forever {
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
			forever {
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
		if ( pangle<0 && nangle>0 && nangle-pangle>=3.1415926 )
		    pangle += 2*3.1415926535897932;
		else if ( pangle>0 && nangle<0 && pangle-nangle>=3.1415926 )
		    nangle += 2*3.1415926535897932;
		if ( nangle-pangle<1 && nangle-pangle>-1 ) {
		    forever {
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

static void MakeExtremum(SplinePoint *cur,enum extreme extremum) {
    double len;
    /* If we decided this point should be an exteme point, then make sure */
    /*  the control points agree with that (ie. must be horizontal or vertical)*/

    if ( cur->pointtype==pt_tangent )
return;
    if ( extremum>=e_ymin ) {
	if ( cur->me.y!=cur->nextcp.y ) {
	    len = sqrt((cur->nextcp.y-cur->me.y)*(cur->nextcp.y-cur->me.y) +
			(cur->nextcp.x-cur->me.x)*(cur->nextcp.x-cur->me.x) );
	    cur->nextcp.y = cur->me.y;
	    if ( cur->me.x>cur->nextcp.x )
		cur->nextcp.x = cur->me.x-len;
	    else
		cur->nextcp.x = cur->me.x+len;
	}
	if ( cur->me.y!=cur->prevcp.y ) {
	    len = sqrt((cur->prevcp.y-cur->me.y)*(cur->prevcp.y-cur->me.y) +
			(cur->prevcp.x-cur->me.x)*(cur->prevcp.x-cur->me.x) );
	    cur->prevcp.y = cur->me.y;
	    if ( cur->me.x>cur->prevcp.x )
		cur->prevcp.x = cur->me.x-len;
	    else
		cur->prevcp.x = cur->me.x+len;
	}
    } else {
	if ( cur->me.x!=cur->nextcp.x ) {
	    len = sqrt((cur->nextcp.x-cur->me.x)*(cur->nextcp.x-cur->me.x) +
			(cur->nextcp.y-cur->me.y)*(cur->nextcp.y-cur->me.y) );
	    cur->nextcp.x = cur->me.x;
	    if ( cur->me.y>cur->nextcp.y )
		cur->nextcp.y = cur->me.y-len;
	    else
		cur->nextcp.y = cur->me.y+len;
	}
	if ( cur->me.x!=cur->prevcp.x ) {
	    len = sqrt((cur->prevcp.x-cur->me.x)*(cur->prevcp.x-cur->me.x) +
			(cur->prevcp.y-cur->me.y)*(cur->prevcp.y-cur->me.y) );
	    cur->prevcp.x = cur->me.x;
	    if ( cur->me.y>cur->prevcp.y )
		cur->prevcp.y = cur->me.y-len;
	    else
		cur->prevcp.y = cur->me.y+len;
	}
    }
}

static SplineSet *TraceCurve(CharView *cv) {
    TraceData *head = cv->freehand.head, *pt, *base, *e;
    SplineSet *spl;
    SplinePoint *last, *cur;
    int cnt, i, tot;
    TPoint *mids;
    double len,sofar;

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
	pt->x =  cv->xoff + rint(pt->here.x*cv->scale);
	pt->y = -cv->yoff + cv->height - rint(pt->here.y*cv->scale);
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
	    tot = 0;
	    if ( base->extremum>=e_ymin ) {
		for ( pt=base->next ; pt->extremum>=e_ymin ; pt = pt->next ) ++tot;
	    } else {
		for ( pt=base->next ; pt->extremum>=e_xmin && pt->extremum<e_ymin ; pt = pt->next ) ++tot;
	    }
	    tot /= 2;
	    e = pt;
	    for ( pt=base->next, i=0 ; i<tot ; pt = pt->next, ++i );
	    pt->use_as_pt = true;
	    base = e;
	}
    }

    TraceMassage(head,cv->freehand.last);

    /* Calculate the mids array */
    mids = galloc(cnt*sizeof(TPoint));
    for ( base=head; base!=NULL && base->next!=NULL; base = pt ) {
	mids[base->num].x = base->here.x;
	mids[base->num].y = base->here.y;
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
	    mids[pt->num].x = pt->here.x;
	    mids[pt->num].y = pt->here.y;
	    mids[pt->num].t = sofar/len;
	    if ( pt->use_as_pt )
	break;
	}
    }

    /* Splice things together */
    spl = chunkalloc(sizeof(SplineSet));
    spl->first = last = SplinePointCreate(head->here.x,head->here.y);
    last->ptindex = 0;

    for ( base=head; base!=NULL && base->next!=NULL; base = pt ) {
	for ( pt=base->next; !pt->use_as_pt ; pt=pt->next );
	cur = SplinePointCreate(pt->here.x,pt->here.y);
	cur->ptindex = pt->num;
	if ( base->next->online || base->next==pt )
	    SplineMake(last,cur);
	else
	    ApproximateSplineFromPoints(last,cur,mids+base->num+1,pt->num-base->num-1);
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
    if ( spl->first->next!=NULL ) {
	pt = head;
	for ( cur=spl->first->next->to; cur->next!=NULL ; cur = cur->next->to ) {
	    if ( cur->nonextcp && cur->noprevcp )
		cur->pointtype = pt_corner;
	    else {
		if ( !cur->nonextcp && !cur->noprevcp )
		    cur->pointtype = pt_curve;
		else
		    cur->pointtype = pt_tangent;
		SPAverageCps(cur);
		while ( pt!=NULL && pt->num<cur->ptindex ) pt=pt->next;
		if ( pt!=NULL && pt->extremum!=e_none )
		    MakeExtremum(cur,pt->extremum);
		if ( !cur->noprevcp )
		    ApproximateSplineFromPointsSlopes(cur->prev->from,cur,
			    mids+cur->prev->from->ptindex+1,
			    cur->ptindex-cur->prev->from->ptindex-1);
	    }
	}
	if ( !cur->nonextcp )
	    ApproximateSplineFromPointsSlopes(cur,cur->next->to,
		    mids+cur->ptindex+1,
		    cur->next->to->ptindex-cur->ptindex-1);
    }

    free(mids);
return( spl );
}

void CVMouseDownFreeHand(CharView *cv, GEvent *event) {
    TraceDataFree(cv->freehand.head);
    cv->freehand.head = cv->freehand.last = NULL;
    cv->freehand.current_trace = NULL;
    TraceDataFromEvent(cv,event);
}

void CVMouseMoveFreeHand(CharView *cv, GEvent *event) {
    TraceDataFromEvent(cv,event);
    SplinePointListFree(cv->freehand.current_trace);
    cv->freehand.current_trace = TraceCurve(cv);
    GDrawRequestExpose(cv->v,NULL,false);
}

void CVMouseUpFreeHand(CharView *cv) {

    if ( cv->freehand.current_trace!=NULL ) {
	SplineCharAddExtrema(cv->freehand.current_trace,false);
	SplinePointListSimplify(cv->sc,cv->freehand.current_trace,false);
	cv->freehand.current_trace->next = *cv->heads[cv->drawmode];
	*cv->heads[cv->drawmode] = cv->freehand.current_trace;
	cv->freehand.current_trace = NULL;
    }
    TraceDataFree(cv->freehand.head);
    cv->freehand.head = cv->freehand.last = NULL;
    GDrawRequestExpose(cv->v,NULL,false);
}
