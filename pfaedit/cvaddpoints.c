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
#include "pfaeditui.h"
#include <math.h>
#include "splinefont.h"
#include "ustring.h"

int CVOneThingSel(CharView *cv, SplinePoint **sp, SplinePointList **_spl,
	RefChar **ref, ImageList **img) {
    /* if there is exactly one thing selected return it */
    SplinePointList *spl, *found=NULL;
    Spline *spline;
    SplinePoint *foundsp=NULL;
    RefChar *refs, *foundref=NULL;
    ImageList *imgs, *foundimg=NULL;

    *sp = NULL; *_spl=NULL; *ref=NULL; *img = NULL;
    for ( spl= *cv->heads[cv->drawmode]; spl!=NULL; spl=spl->next ) {
	if ( spl->first->selected ) {
	    if ( found!=NULL )
return( 0 );			/* At least two points */
	    found = spl; foundsp = spl->first;
	}
	for ( spline = spl->first->next; spline!=NULL ; spline=spline->to->next ) {
	    if ( spline->to==spl->first )
	break;
	    if ( spline->to->selected ) {
		if ( found!=NULL )
return( 0 );
		found = spl; foundsp = spline->to;
	    }
	}
    }
    *sp = foundsp; *_spl = found;

    if ( cv->drawmode==dm_fore ) {
	for ( refs=cv->sc->refs; refs!=NULL; refs = refs->next ) {
	    if ( refs->selected ) {
		if ( found!=NULL || foundref!=NULL )
return( 0 );
		foundref = refs;
	    }
	}
	*ref = foundref;
    }

    if ( cv->drawmode==dm_back ) {
	for ( imgs=cv->sc->backimages; imgs!=NULL; imgs = imgs->next ) {
	    if ( imgs->selected ) {
		if ( found!=NULL || foundimg!=NULL )
return( 0 );
		foundimg = imgs;
	    }
	}
	*img = foundimg;
    }
    if ( found )
return( foundimg==NULL && foundref==NULL );
    else if ( foundref || foundimg )
return( true );

return( false );
}

SplinePointList *CVAnySelPointList(CharView *cv) {
    /* if there is exactly one point selected and it is on an open splineset */
    /*  and it is one of the endpoints of the splineset, then return that */
    /*  splineset */
    SplinePointList *spl, *found=NULL;
    Spline *spline, *first;

    for ( spl= *cv->heads[cv->drawmode]; spl!=NULL; spl=spl->next ) {
	if ( spl->first->selected ) {
	    if ( found!=NULL )
return( NULL );			/* At least two points */
	    if ( spl->first->prev!=NULL )
return( NULL );			/* Not an open splineset */
	    found = spl;
	}
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( spline->to->selected ) {
		if ( found!=NULL )
return( NULL );
		if ( spline->to->next!=NULL )
return( NULL );			/* Selected point is not at end of a splineset */
		found = spl;
	    }
	    if ( first==NULL ) first = spline;
	}
    }
return( found );
}

SplinePoint *CVAnySelPoint(CharView *cv) {
    /* if there is exactly one point selected */
    SplinePointList *spl;
    Spline *spline, *first; SplinePoint *found = NULL;

    for ( spl= *cv->heads[cv->drawmode]; spl!=NULL; spl=spl->next ) {
	if ( spl->first->selected ) {
	    if ( found!=NULL )
return( NULL );			/* At least two points */
	    found = spl->first;
	}
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( spline->to->selected ) {
		if ( found!=NULL )
return( NULL );
		found = spline->to;
	    }
	    if ( first==NULL ) first = spline;
	}
    }
return( found );
}

/* When the user tries to add a point (by doing a mouse down with a point tool
  selected) there are several cases to be looked at:
	If there is a single point selected and it is at the begining/end of an open spline set
	    if we clicked on an other point which is the begining/end of an open splineset
		draw a spline connecting the two spline sets and merge them
			(or if it's the same spline set, then close it)
	    else
		create a new point where we clicked
		draw a spline between the selected point and the new one
		deselect the old point
		select the new one
	    endif
	else if they clicked on a spline
	    split the spline into two bits at the point where they clicked
	else
	    create a new point where they clicked
	    put it on a new splineset
	    select it
	endif

	and, if the old point is a tangent, we may need to adjust its control pt
*/
void CVMouseDownPoint(CharView *cv) {
    SplineSet *sel, *ss;
    SplinePoint *sp, *base = NULL;
    SplineChar *sc = cv->sc;
    enum pointtype ptype = (cv->active_tool==cvt_curve?pt_curve:
			    cv->active_tool==cvt_corner?pt_corner:
			    cv->active_tool==cvt_tangent?pt_tangent:
			    /*cv->active_tool==cvt_pen?*/pt_corner);

    cv->active_spl = NULL;
    cv->active_sp = NULL;

    sel = CVAnySelPointList(cv);
    if ( sel!=NULL ) {
	if ( sel->first->selected ) base = sel->first;
	else base = sel->last;
	if ( base==cv->p.sp )
return;			/* We clicked on the active point, that's a no-op */
    }
    CVPreserveState(cv);
    CVClearSel(cv);
    if ( sel!=NULL ) {
	sp = cv->p.sp;
	cv->lastselpt = base;
	ss = sel;
	if ( base->next!=NULL )
	    SplineSetReverse(sel);
	if ( base->next!=NULL )
	    GDrawIError("Base point not at end of splineset in CVMouseDownPoint");
	if ( sp==NULL || (sp->next!=NULL && sp->prev!=NULL) || sp==base ) {
	    /* Add a new point */
	    sp = chunkalloc(sizeof(SplinePoint));
	    sp->me.x = cv->p.cx;
	    sp->me.y = cv->p.cy;
	    sp->nextcp = sp->me;
	    sp->nonextcp = 1;
	    sp->nextcpdef = sp->prevcpdef = 1;
	    sp->pointtype = ptype;
	    sp->selected = true;
	    if ( cv->active_tool!=cvt_pen )
		SplineCharDefaultNextCP(base,sp);
	    SplineCharDefaultPrevCP(sp,base);
	    SplineMake(base,sp);
	    ss->last = sp;
	} else if ( cv->p.spl==sel ) {
	    /* Close the current spline set */
	    cv->joinvalid = true;
	    cv->joinpos = *sp; cv->joinpos.selected = false;
	    if ( cv->active_tool!=cvt_pen )
		SplineCharDefaultNextCP(base,sp);
	    SplineCharDefaultPrevCP(sp,base);
	    SplineMake(base,sp);
	    ss->last = sp;
	    if ( sp->pointtype==pt_tangent ) {
		SplineCharTangentNextCP(sp);
		if ( sp->next ) SplineRefigure(sp->next );
	    }
	} else {
	    /* Merge two spline sets */
	    cv->joinvalid = true;
	    cv->joinpos = *sp; cv->joinpos.selected = false;
	    if ( sp->prev!=NULL )
		SplineSetReverse(cv->p.spl);
	    if ( sp->prev!=NULL )
		GDrawIError("Base point not at start of splineset in CVMouseDownPoint");
	    /* remove the old spl entry from the chain */
	    if ( cv->p.spl==*cv->heads[cv->drawmode] )
		*cv->heads[cv->drawmode] = cv->p.spl->next;
	    else { SplineSet *temp;
		for ( temp = *cv->heads[cv->drawmode]; temp->next!=cv->p.spl; temp = temp->next );
		temp->next = cv->p.spl->next;
	    }
	    if ( cv->active_tool!=cvt_pen )
		SplineCharDefaultNextCP(base,sp);
	    SplineCharDefaultPrevCP(sp,base);
	    SplineMake(base,sp);
	    if ( sp->pointtype==pt_tangent ) {
		SplineCharTangentNextCP(sp);
		if ( sp->next ) SplineRefigure(sp->next );
	    }
	    ss->last = cv->p.spl->last;
	    chunkfree( cv->p.spl,sizeof(SplinePointList) );
	}
	sp->selected = true;
	if ( base->pointtype==pt_tangent ) {
	    SplineCharTangentPrevCP(base);
	    if ( base->prev!=NULL )
		SplineRefigure(base->prev);
	}
    } else if ( cv->p.spline!=NULL ) {
	sp = SplineBisect(cv->p.spline,cv->p.t);
	cv->joinvalid = true;
	cv->joinpos = *sp; cv->joinpos.selected = false;
	sp->pointtype = ptype;
	sp->selected = true;
	ss = cv->p.spl;
    } else {
	ss = chunkalloc(sizeof(SplineSet));
	sp = chunkalloc(sizeof(SplinePoint));
	ss->first = ss->last = sp;
	ss->next = *cv->heads[cv->drawmode];
	*cv->heads[cv->drawmode] = ss;
	sp->me.x = cv->p.cx;
	sp->me.y = cv->p.cy;
	sp->nextcp = sp->me;
	sp->prevcp = sp->me;
	sp->nonextcp = sp->noprevcp = 1;
	sp->nextcpdef = sp->prevcpdef = 1;
	sp->pointtype = ptype;
	sp->selected = true;
    }

    cv->active_spl = ss;
    cv->active_sp = sp;
    CVSetCharChanged(cv,true);
    CVInfoDraw(cv,cv->gw);
    SCUpdateAll(sc);
}

static void AdjustControls(SplinePoint *sp) {
    if ( sp->next!=NULL ) {
	SplineCharDefaultNextCP(sp,sp->next->to);	/* also fixes up tangents */
	SplineCharDefaultPrevCP(sp->next->to,sp);
	SplineRefigure(sp->next);
	if ( sp->next->to->pointtype==pt_tangent && sp->next->to->next!=NULL ) {
	    SplineCharTangentNextCP(sp->next->to);
	    SplineRefigure(sp->next->to->next);
	}
    }
    if ( sp->prev!=NULL ) {
	SplineCharDefaultPrevCP(sp,sp->prev->from);
	SplineCharDefaultNextCP(sp->prev->from,sp);
	SplineRefigure(sp->prev);
	if ( sp->prev->from->pointtype==pt_tangent && sp->prev->from->prev!=NULL ) {
	    SplineCharTangentPrevCP(sp->prev->from);
	    SplineRefigure(sp->prev->from->prev);
	}
    }
}

void CVAdjustPoint(CharView *cv, SplinePoint *sp) {

    if ( cv->info.x==sp->me.x && cv->info.y==sp->me.y )
return;
    sp->nextcp.x += (cv->info.x-sp->me.x);
    sp->nextcp.y += (cv->info.y-sp->me.y);
    sp->prevcp.x += (cv->info.x-sp->me.x);
    sp->prevcp.y += (cv->info.y-sp->me.y);
    sp->me.x = cv->info.x;
    sp->me.y = cv->info.y;
    AdjustControls(sp);
    CVSetCharChanged(cv,true);
}

void CVMergeSplineSets(CharView *cv, SplinePoint *active, SplineSet *activess,
	SplinePoint *merge, SplineSet *mergess) {
    SplinePointList *spl;

    cv->joinvalid = true;
    cv->joinpos = *merge; cv->joinpos.selected = false;

    if ( active->prev==NULL )
	SplineSetReverse(activess);
    if ( merge->next==NULL )
	SplineSetReverse(mergess);
    active->nextcp = merge->nextcp;
    active->nonextcp = merge->nonextcp;
    active->nextcpdef = merge->nextcpdef;
    active->next = merge->next;
    if ( merge->next!= NULL ) {
	active->next->from = active;
	activess->last = mergess->last;
    }
    merge->next = NULL;
    if ( mergess==activess ) {
	activess->first = activess->last = active;
	SplinePointFree(merge);
    } else {
	mergess->last = merge;
	if ( mergess==*cv->heads[cv->drawmode] )
	    *cv->heads[cv->drawmode] = mergess->next;
	else {
	    for ( spl = *cv->heads[cv->drawmode]; spl->next!=mergess; spl=spl->next );
	    spl->next = mergess->next;
	}
	SplinePointListFree(mergess);
    }
    AdjustControls(active);
}

/* We move the active point around following the mouse. */
/*  There's one special case. If the active point is an end point on its splineset */
/*  and we've just moved on top of another splineset end-point, then join the */
/*  two splinesets at the active point. Of course we might close up our own */
/*  spline set... */
void CVMouseMovePoint(CharView *cv, PressedOn *p) {
    SplinePoint *active = cv->active_sp, *merge = p->sp;
    SplineSet *activess = cv->active_spl;

    if ( active==NULL )
return;
    if ( cv->info.x==active->me.x && cv->info.y==active->me.y )
return;

    if ( !cv->recentchange ) CVPreserveState(cv);

    CVAdjustPoint(cv,active);
    if (( active->next==NULL || active->prev==NULL ) && merge!=NULL &&
	    merge!=active &&
	    (merge->next==NULL || merge->prev==NULL )) {
	CVMergeSplineSets(cv,active,activess,merge,p->spl);
    }
    SCUpdateAll(cv->sc);
}

void CVMouseMovePen(CharView *cv, PressedOn *p) {
    SplinePoint *active = cv->active_sp;

    if ( active==NULL )
return;
    if ( cv->info.x==active->nextcp.x && cv->info.y==active->nextcp.y )
return;
    cv->lastselpt = cv->active_sp;

    active->nextcp.x = cv->info.x;
    active->nextcp.y = cv->info.y;
    if ( active->nextcp.x==active->me.x && active->nextcp.y==active->me.y ) {
	active->prevcp = active->me;
	active->nonextcp = active->noprevcp = true;
	active->pointtype = pt_corner;
    } else {
	active->prevcp.x = active->me.x - (active->nextcp.x-active->me.x);
	active->prevcp.y = active->me.y - (active->nextcp.y-active->me.y);
	active->nonextcp = active->noprevcp = false;
	active->nextcpdef = active->prevcpdef = false;
	active->pointtype = pt_curve;
    }
    if ( active->prev!=NULL )
	SplineRefigure(active->prev);
    if ( active->next!=NULL )
	SplineRefigure(active->next);
    SCUpdateAll(cv->sc);
}

void CVMouseUpPoint(CharView *cv) {
    cv->lastselpt = cv->active_sp;
    cv->active_spl = NULL;
    cv->active_sp = NULL;
    cv->joinvalid = false;
    CVInfoDraw(cv,cv->gw);
}
