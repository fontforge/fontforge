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
#include "fontforgeui.h"
#include <math.h>
#include "splinefont.h"
#include "ustring.h"

int CVOneThingSel(CharView *cv, SplinePoint **sp, SplinePointList **_spl,
	RefChar **ref, ImageList **img, AnchorPoint **ap, spiro_cp **scp) {
    /* if there is exactly one thing selected return it */
    SplinePointList *spl, *found=NULL;
    Spline *spline;
    SplinePoint *foundsp=NULL;
    RefChar *refs, *foundref=NULL;
    ImageList *imgs, *foundimg=NULL;
    AnchorPoint *aps, *foundap=NULL;
    spiro_cp *foundcp = NULL;
    int i;

    *sp = NULL; *_spl=NULL; *ref=NULL; *img = NULL; *scp = NULL;
    if ( ap ) *ap = NULL;
    for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	if ( !cv->b.sc->inspiro || !hasspiro()) {
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
	} else {
	    for ( i=0; i<spl->spiro_cnt-1; ++i ) {
		if ( SPIRO_SELECTED(&spl->spiros[i])) {
		    if ( found )
return( 0 );
		    found = spl;
		    foundcp = &spl->spiros[i];
		}
	    }
	}
    }
    *sp = foundsp; *scp = foundcp; *_spl = found;

    if ( cv->b.drawmode!=dm_grid ) {
	for ( refs=cv->b.layerheads[cv->b.drawmode]->refs; refs!=NULL; refs = refs->next ) {
	    if ( refs->selected ) {
		if ( found!=NULL || foundref!=NULL )
return( 0 );
		foundref = refs;
	    }
	}
	*ref = foundref;
	if ( cv->showanchor && ap!=NULL ) {
	    for ( aps=cv->b.sc->anchor; aps!=NULL; aps=aps->next ) {
		if ( aps->selected ) {
		    if ( found!=NULL || foundref!=NULL || foundap!=NULL )
return( 0 );
		    foundap = aps;
		}
	    }
	    *ap = foundap;
	}
    }

    for ( imgs=cv->b.layerheads[cv->b.drawmode]->images; imgs!=NULL; imgs = imgs->next ) {
	if ( imgs->selected ) {
	    if ( found!=NULL || foundimg!=NULL )
return( 0 );
	    foundimg = imgs;
	}
    }
    *img = foundimg;

    if ( found )
return( foundimg==NULL && foundref==NULL && foundap==NULL );
    else if ( foundref || foundimg || foundap )
return( true );

return( false );
}

int CVOneContourSel(CharView *cv, SplinePointList **_spl,
	RefChar **ref, ImageList **img) {
    /* if there is exactly one contour/image/reg selected return it */
    SplinePointList *spl, *found=NULL;
    Spline *spline;
    RefChar *refs, *foundref=NULL;
    ImageList *imgs, *foundimg=NULL;
    int i;

    *_spl=NULL; *ref=NULL; *img = NULL;
    for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	if ( !cv->b.sc->inspiro || !hasspiro()) {
	    if ( spl->first->selected ) {
		if ( found!=NULL && found!=spl )
return( 0 );			/* At least two contours */
		found = spl;
	    }
	    for ( spline = spl->first->next; spline!=NULL ; spline=spline->to->next ) {
		if ( spline->to==spl->first )
	    break;
		if ( spline->to->selected ) {
		    if ( found!=NULL && found!=spl )
return( 0 );
		    found = spl;
		}
	    }
	} else {
	    for ( i=0; i<spl->spiro_cnt-1; ++i ) {
		if ( SPIRO_SELECTED(&spl->spiros[i])) {
		    if ( found!=NULL && found!=spl )
return( 0 );
		    found = spl;
		}
	    }
	}
    }
    *_spl = found;

    if ( cv->b.drawmode==dm_fore ) {
	for ( refs=cv->b.layerheads[cv->b.drawmode]->refs; refs!=NULL; refs = refs->next ) {
	    if ( refs->selected ) {
		if ( found!=NULL || foundref!=NULL )
return( 0 );
		foundref = refs;
	    }
	}
	*ref = foundref;
    }

    for ( imgs=cv->b.layerheads[cv->b.drawmode]->images; imgs!=NULL; imgs = imgs->next ) {
	if ( imgs->selected ) {
	    if ( found!=NULL || foundimg!=NULL )
return( 0 );
	    foundimg = imgs;
	}
    }
    *img = foundimg;

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
    int i;

    for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	if ( cv->b.sc->inspiro && hasspiro()) {
	    for ( i = 0; i<spl->spiro_cnt-1; ++i ) {
		if ( SPIRO_SELECTED(&spl->spiros[i])) {
		    /* Only interesting if the single selection is at the */
		    /* start/end of an open contour */
		    if (( i!=0 && i!=spl->spiro_cnt-2 ) ||
			    !SPIRO_SPL_OPEN(spl))
return( NULL );
		    else if ( found==NULL )
			found = spl;
		    else
return( NULL );
		}
	    }
	} else {
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
    }
return( found );
}

int CVAnySelPoint(CharView *cv,SplinePoint **sp, spiro_cp **cp) {
    /* if there is exactly one point selected */
    SplinePointList *spl;
    Spline *spline, *first; SplinePoint *found = NULL;
    spiro_cp *foundcp = NULL;
    int i;

    *sp = NULL; *cp = NULL;
    if ( cv->b.sc->inspiro && hasspiro()) {
	for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		if ( SPIRO_SELECTED( &spl->spiros[i]) ) {
		    if ( foundcp )
return( false );
		    foundcp = &spl->spiros[i];
		}
	}
	*cp = foundcp;
return( foundcp!=NULL );
    } else {
	for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	    if ( spl->first->selected ) {
		if ( found!=NULL )
return( false );			/* At least two points */
		found = spl->first;
	    }
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( spline->to->selected ) {
		    if ( found!=NULL )
return( false );
		    found = spline->to;
		}
		if ( first==NULL ) first = spline;
	    }
	}
	*sp = found;
return( found!=NULL );
    }
}

static void CVMergeSPLS(CharView *cv,SplineSet *ss, SplinePoint *base,SplinePoint *sp) {
    int order2 = cv->b.layerheads[cv->b.drawmode]->order2;

    cv->joinvalid = true;
    cv->joinpos = *sp; cv->joinpos.selected = false;
    if ( sp->prev!=NULL )
	SplineSetReverse(cv->p.spl);
    if ( sp->prev!=NULL )
	IError("Base point not at start of splineset in CVMouseDownPoint");
    /* remove the old spl entry from the chain */
    if ( cv->p.spl==cv->b.layerheads[cv->b.drawmode]->splines )
	cv->b.layerheads[cv->b.drawmode]->splines = cv->p.spl->next;
    else { SplineSet *temp;
	for ( temp = cv->b.layerheads[cv->b.drawmode]->splines; temp->next!=cv->p.spl; temp = temp->next );
	temp->next = cv->p.spl->next;
    }
    if ( order2 && (!RealNear(base->nextcp.x,sp->prevcp.x) ||
	    !RealNear(base->nextcp.y,sp->prevcp.y)) ) {
	base->nonextcp = sp->noprevcp = true;
	base->nextcp = base->me;
	sp->prevcp = sp->me;
    }
    SplineMake(base,sp,order2);
    SplineCharDefaultNextCP(base);
    SplineCharDefaultPrevCP(sp);
    if ( sp->pointtype==pt_tangent ) {
	SplineCharTangentNextCP(sp);
	if ( sp->next ) SplineRefigure(sp->next );
    }
    ss->last = cv->p.spl->last;
    if ( ss->spiros && cv->p.spl->spiros ) {
	if ( ss->spiro_cnt+cv->p.spl->spiro_cnt > ss->spiro_max )
	    ss->spiros = realloc(ss->spiros,
		    (ss->spiro_max = ss->spiro_cnt+cv->p.spl->spiro_cnt)*sizeof(spiro_cp));
	memcpy(ss->spiros+ss->spiro_cnt-1,
		cv->p.spl->spiros+1, (cv->p.spl->spiro_cnt-1)*sizeof(spiro_cp));
	ss->spiro_cnt += cv->p.spl->spiro_cnt-2;
    } else
	SplineSetSpirosClear(ss);
    cv->p.spl->last = cv->p.spl->first = NULL;
    cv->p.spl->spiros = 0;
    SplinePointListFree(cv->p.spl);
    cv->p.spl = NULL;
}

static void CVMouseDownSpiroPoint(CharView *cv, GEvent *event) {
    SplineSet *sel, *ss;
    SplineChar *sc = cv->b.sc;
    spiro_cp *base, *cp;
    int base_index, cp_index, i;
    char ty = (cv->active_tool==cvt_curve?SPIRO_G4:
			    cv->active_tool==cvt_hvcurve?SPIRO_G2:
			    cv->active_tool==cvt_corner?SPIRO_CORNER:
			    cv->active_tool==cvt_tangent?SPIRO_LEFT:
			    /*cv->active_tool==cvt_pen?*/SPIRO_RIGHT);

    cv->active_spl = NULL;
    cv->active_sp = NULL;

    sel = CVAnySelPointList(cv);
    if ( sel!=NULL ) {
	if ( SPIRO_SELECTED(&sel->spiros[0]) ) base_index = 0;
	else base_index = sel->spiro_cnt-2;
	base = &sel->spiros[base_index];
	if ( base==cv->p.spiro )
return;			/* We clicked on the active point, that's a no-op */
    }
    CVPreserveState(&cv->b);
    CVClearSel(cv);
    if ( sel!=NULL ) {
	if ( (cp = cv->p.spiro)!=NULL )
	    cp_index = cp-cv->p.spl->spiros;
	cv->lastselcp = base;
	ss = sel;
	if ( base_index!=sel->spiro_cnt-2 ) {
	    SplineSetReverse(sel);
	    base = &sel->spiros[sel->spiro_cnt-2];
	    if ( cv->p.spl==sel ) {
		cp_index = sel->spiro_cnt-2-cp_index;
		cp = &sel->spiros[cp_index];
	    }
	}
	if ( cp==NULL || (cp_index!=0 && cp_index!=cv->p.spl->spiro_cnt-2) ||
		cp==base || !SPIRO_SPL_OPEN(cv->p.spl)) {
	    /* Add a new point */
	    if ( sel->spiro_cnt>=sel->spiro_max )
		sel->spiros = realloc(sel->spiros,(sel->spiro_max += 10)*sizeof(spiro_cp));
	    cp = &sel->spiros[sel->spiro_cnt-1];
	    cp[1] = cp[0];		/* Move the final 'z' */
	    cp->x = cv->p.cx;
	    cp->y = cv->p.cy;
	    cp->ty = ty;
	    SPIRO_DESELECT(cp-1);
	    ++sel->spiro_cnt;
	} else if ( cv->p.spl==sel ) {
	    /* Close the current spline set */
	    sel->spiros[0].ty = ty;
	    cv->joinvalid = true;
	    cv->joincp = *cp; SPIRO_DESELECT(&cv->joincp);
	} else {
	    /* Merge two spline sets */
	    SplinePoint *sp = cp_index==0 ? cv->p.spl->first : cv->p.spl->last;
	    SplinePoint *basesp = base_index==0 ? sel->first : sel->last;
	    cv->joincp = *cp; SPIRO_DESELECT(&cv->joincp);
	    CVMergeSPLS(cv,ss,basesp,sp);
	}
    } else if ( cv->p.spline!=NULL ) {
	/* Add an intermediate point on an already existing spline */
	ss = cv->p.spl;
	if ( ss->spiro_cnt>=ss->spiro_max )
	    ss->spiros = realloc(ss->spiros,(ss->spiro_max += 10)*sizeof(spiro_cp));
	for ( i=ss->spiro_cnt-1; i>cv->p.spiro_index; --i )
	    ss->spiros[i+1] = ss->spiros[i];
	++ss->spiro_cnt;
	cp = &ss->spiros[cv->p.spiro_index+1];
	cp->x = cv->p.cx;
	cp->y = cv->p.cy;
	cp->ty = ty;
	ss = cv->p.spl;
	cv->joinvalid = true;
	cv->joincp = *cp; SPIRO_DESELECT(&cv->joincp);
    } else {
	/* A new point on a new (open) contour */
	ss = chunkalloc(sizeof(SplineSet));
	ss->next = cv->b.layerheads[cv->b.drawmode]->splines;
	cv->b.layerheads[cv->b.drawmode]->splines = ss;
	ss->spiros = malloc((ss->spiro_max=10)*sizeof(spiro_cp));
	cp = &ss->spiros[0];
	cp->x = cv->p.cx;
	cp->y = cv->p.cy;
	cp->ty = SPIRO_OPEN_CONTOUR;
	cp[1].x = cp[1].y = 0; cp[1].ty = 'z';
	ss->spiro_cnt = 2;
    }
    SPIRO_SELECT(cp);

    SSRegenerateFromSpiros(ss);

    cv->active_spl = ss;
    cv->active_cp = cp;
    CVSetCharChanged(cv,true);
    CVInfoDraw(cv,cv->gw);
    SCUpdateAll(sc);
}

/* When the user tries to add a point (by doing a mouse down with a point tool
  selected) there are several cases to be looked at:
	If there is a single point selected and it is at the begining/end of an open spline set
	    if we clicked on another point which is the begining/end of an open splineset
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

    With the introduction of spiro mode (Raph Levien's clothoid splines)
      we've got to worry about all the above cases for spiro points too.
*/
void CVMouseDownPoint(CharView *cv, GEvent *event) {
    SplineSet *sel, *ss;
    SplinePoint *sp, *base = NULL;
    SplineChar *sc = cv->b.sc;
    enum pointtype ptype = (cv->active_tool==cvt_curve?pt_curve:
			    cv->active_tool==cvt_hvcurve?pt_hvcurve:
			    cv->active_tool==cvt_corner?pt_corner:
			    cv->active_tool==cvt_tangent?pt_tangent:
			    /*cv->active_tool==cvt_pen?*/pt_corner);
    int order2 = cv->b.layerheads[cv->b.drawmode]->order2;
    int order2_style = (order2 && !(event->u.mouse.state&ksm_meta)) ||
		    (!order2 && (event->u.mouse.state&ksm_meta));

    cv->active_spl = NULL;
    cv->active_sp = NULL;

    if ( cv->b.sc->inspiro && hasspiro()) {
	CVMouseDownSpiroPoint(cv, event);
return;
    }

    sel = CVAnySelPointList(cv);
    if ( sel!=NULL ) {
	if ( sel->first->selected ) base = sel->first;
	else base = sel->last;
	if ( base==cv->p.sp )
return;			/* We clicked on the active point, that's a no-op */
    }
    CVPreserveState(&cv->b);
    CVClearSel(cv);
    if ( sel!=NULL ) {
	sp = cv->p.sp;
	cv->lastselpt = base;
	ss = sel;
	if ( base->next!=NULL )
	    SplineSetReverse(sel);
	if ( base->next!=NULL )
	    IError("Base point not at end of splineset in CVMouseDownPoint");
	if ( sp==NULL || (sp->next!=NULL && sp->prev!=NULL) || sp==base ) {
	    /* Add a new point */
	    SplineSetSpirosClear(sel);
	    sp = SplinePointCreate( cv->p.cx, cv->p.cy );
	    sp->noprevcp = sp->nonextcp = 1;
	    sp->nextcpdef = sp->prevcpdef = 1;
	    sp->pointtype = ptype;
	    sp->selected = true;
	    if ( !base->nonextcp && order2_style &&
		    cv->active_tool==cvt_pen ) {
		sp->prevcp = base->nextcp;
		sp->noprevcp = false;
		sp->me.x = ( sp->prevcp.x + sp->nextcp.x )/2;
		sp->me.y = ( sp->prevcp.y + sp->nextcp.y )/2;
		sp->nonextcp = false;
		sp->pointtype = pt_curve;
	    } else if ( order2 && !base->nonextcp ) {
		sp->prevcp = base->nextcp;
		sp->noprevcp = false;
		if (  cv->active_tool==cvt_pen ) {
		    sp->nextcp.x = sp->me.x - (sp->prevcp.x-sp->me.x);
		    sp->nextcp.y = sp->me.y - (sp->prevcp.y-sp->me.y);
		    sp->nonextcp = false;
		    sp->pointtype = pt_curve;
		}
	    }
	    if ( base->nonextcp )
		base->nextcpdef = true;
	    SplineMake(base,sp,order2);
	    if ( cv->active_tool!=cvt_pen ) {
		SplineCharDefaultNextCP(base);
		SplineCharDefaultPrevCP(sp);
	    }
	    ss->last = sp;
	} else if ( cv->p.spl==sel ) {
	    /* Close the current spline set */
	    SplineSetSpirosClear(sel);
	    cv->joinvalid = true;
	    cv->joinpos = *sp; cv->joinpos.selected = false;
	    if ( order2 ) {
		if ( base->nonextcp || sp->noprevcp ) {
		    base->nonextcp = sp->noprevcp = true;
		    base->nextcp = base->me;
		    sp->prevcp = sp->me;
		} else {
		    base->nextcp.x = sp->prevcp.x = (base->nextcp.x+sp->prevcp.x)/2;
		    base->nextcp.y = sp->prevcp.y = (base->nextcp.y+sp->prevcp.y)/2;
		}
		base->nextcpdef = sp->prevcpdef = true;
	    }
	    SplineMake(base,sp,order2);
	    if ( cv->active_tool!=cvt_pen )
		SplineCharDefaultNextCP(base);
	    SplineCharDefaultPrevCP(sp);
	    ss->last = sp;
	    if ( sp->pointtype==pt_tangent ) {
		SplineCharTangentNextCP(sp);
		if ( sp->next ) SplineRefigure(sp->next );
	    }
	} else {
	    /* Merge two spline sets */
	    SplineSetSpirosClear(sel);
	    CVMergeSPLS(cv,sel, base,sp);
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
	if (  cv->active_tool==cvt_pen )
	    ptype = pt_curve;
	sp->pointtype = ptype;
	if ( ptype==pt_hvcurve ) {
	    SPHVCurveForce(sp);
	}
	sp->selected = true;
	ss = cv->p.spl;
    } else {
	ss = chunkalloc(sizeof(SplineSet));
	sp = SplinePointCreate( cv->p.cx, cv->p.cy );
	
	ss->first = ss->last = sp;
	ss->next = cv->b.layerheads[cv->b.drawmode]->splines;
	cv->b.layerheads[cv->b.drawmode]->splines = ss;
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

    if ( cv->active_tool == cvt_pen )
	cv->p.constrain = sp->me;
}

void AdjustControls(SplinePoint *sp) {
    if ( sp->next!=NULL ) {
	SplineCharDefaultNextCP(sp);	/* also fixes up tangents */
	SplineCharDefaultPrevCP(sp->next->to);
	SplineRefigure(sp->next);
	if ( sp->next->to->pointtype==pt_tangent && sp->next->to->next!=NULL ) {
	    SplineCharTangentNextCP(sp->next->to);
	    SplineRefigure(sp->next->to->next);
	}
    }
    if ( sp->prev!=NULL ) {
	SplineCharDefaultPrevCP(sp);
	SplineCharDefaultNextCP(sp->prev->from);
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
	SplinePointMDFree(cv->b.sc,merge);
	if ( activess->spiro_cnt!=0 ) {
	    activess->spiros[0].ty = activess->spiros[activess->spiro_cnt-2].ty;
	    activess->spiros[activess->spiro_cnt-2] = activess->spiros[activess->spiro_cnt-1];
	    --activess->spiro_cnt;
	}
    } else {
	mergess->last = merge;
	if ( mergess==cv->b.layerheads[cv->b.drawmode]->splines )
	    cv->b.layerheads[cv->b.drawmode]->splines = mergess->next;
	else {
	    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl->next!=mergess; spl=spl->next );
	    spl->next = mergess->next;
	}
	if ( activess->spiros && mergess->spiros ) {
	    if ( activess->spiro_cnt+mergess->spiro_cnt > activess->spiro_max )
		activess->spiros = realloc(activess->spiros,
			(activess->spiro_max = activess->spiro_cnt+mergess->spiro_cnt)*sizeof(spiro_cp));
	    memcpy(activess->spiros+activess->spiro_cnt-1,
		    mergess->spiros+1, (mergess->spiro_cnt-1)*sizeof(spiro_cp));
	    activess->spiro_cnt += mergess->spiro_cnt-2;
	} else
	    SplineSetSpirosClear(activess);
	SplinePointListMDFree(cv->b.sc,mergess);
    }
    if (( active->pointtype==pt_curve || active->pointtype==pt_hvcurve ) &&
	    !active->nonextcp && !active->noprevcp &&
	    !active->nextcpdef && !active->prevcpdef &&
	    !BpColinear(&active->prevcp,&active->me,&active->nextcp))
	active->nextcpdef = active->prevcpdef = true;
    SplineSetJoinCpFixup(active);
}

static void CVMouseMoveSpiroPoint(CharView *cv, PressedOn *p) {
    spiro_cp *active = cv->active_cp, *merge = p->spiro;
    SplineSet *activess = cv->active_spl;
    int active_index;

    if ( active==NULL )
return;
    if ( cv->info.x==active->x && cv->info.y==active->y )
return;

    if ( !cv->recentchange ) CVPreserveState(&cv->b);

    active->x = cv->info.x;
    active->y = cv->info.y;
    CVSetCharChanged(cv,true);

    active_index = active-activess->spiros;

    if ( active!=merge && merge!=NULL && p->spl!=NULL &&
	    SPIRO_SPL_OPEN(activess) &&
	    SPIRO_SPL_OPEN(p->spl) &&
	    (active_index==0 || active_index==activess->spiro_cnt-2) &&
	    ((merge-p->spl->spiros)==0 || (merge-p->spl->spiros)==p->spl->spiro_cnt-2) ) {
	SplinePoint *activesp = active_index==0 ? activess->first : activess->last;
	SplinePoint *mergesp = (merge-p->spl->spiros)==0 ? p->spl->first : p->spl->last;
	CVMergeSplineSets(cv,activesp,activess,mergesp,p->spl);
    }
    SSRegenerateFromSpiros(activess);
    SCUpdateAll(cv->b.sc);
}

/* We move the active point around following the mouse. */
/*  There's one special case. If the active point is an end point on its splineset */
/*  and we've just moved on top of another splineset end-point, then join the */
/*  two splinesets at the active point. Of course we might close up our own */
/*  spline set... */
void CVMouseMovePoint(CharView *cv, PressedOn *p) {
    SplinePoint *active = cv->active_sp, *merge = p->sp;
    SplineSet *activess = cv->active_spl;

    if ( cv->b.sc->inspiro && hasspiro()) {
	CVMouseMoveSpiroPoint(cv,p);
return;
    }

    if ( active==NULL )
return;
    if ( cv->info.x==active->me.x && cv->info.y==active->me.y )
return;

    if ( !cv->recentchange ) CVPreserveState(&cv->b);

    CVAdjustPoint(cv,active);
    SplineSetSpirosClear(activess);
    if (( active->next==NULL || active->prev==NULL ) && merge!=NULL && p->spl!=NULL &&
	    merge!=active &&
	    (merge->next==NULL || merge->prev==NULL )) {
	CVMergeSplineSets(cv,active,activess,merge,p->spl);
    }
    SCUpdateAll(cv->b.sc);
}

void CVMouseMovePen(CharView *cv, PressedOn *p, GEvent *event) {
    SplinePoint *active = cv->active_sp;
    int order2 = cv->b.layerheads[cv->b.drawmode]->order2;
    int order2_style = (order2 && !(event->u.mouse.state&ksm_meta)) ||
		    (!order2 && (event->u.mouse.state&ksm_meta));

    if ( cv->b.sc->inspiro && hasspiro()) {
	CVMouseMoveSpiroPoint(cv,p);
return;
    }

    if ( active==NULL )
return;
    if ( cv->info.x==active->nextcp.x && cv->info.y==active->nextcp.y )
return;
    /* In order2 fonts when the user clicks with the pen tool we'd like to */
    /*  leave it with the default cp (ie. the cp which makes the current point*/
    /*  implicit) rather than moving the cp to the base point and losing the */
    /*  curve */
    if ( cv->info.x==active->me.x && cv->info.y==active->me.y &&
	    event->type==et_mouseup && cv->b.layerheads[cv->b.drawmode]->order2 )
return;
    SplineSetSpirosClear(cv->active_spl);
    cv->lastselpt = cv->active_sp;

    active->nextcp.x = cv->info.x;
    active->nextcp.y = cv->info.y;
    if ( order2_style && active->next==NULL ) {
	active->me.x = (active->nextcp.x + active->prevcp.x)/2;
	active->me.y = (active->nextcp.y + active->prevcp.y)/2;
	if ( active->me.x == active->nextcp.x && active->me.y == active->nextcp.y ) {
	    active->nonextcp = active->noprevcp = true;
	} else {
	    active->nonextcp = active->noprevcp = false;
	    active->pointtype = pt_curve;
	}
	if ( active->prev!=NULL )
	    SplineRefigure(active->prev);
	SCUpdateAll(cv->b.sc);
return;
    } else if ( active->nextcp.x==active->me.x && active->nextcp.y==active->me.y ) {
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
    if ( cv->b.layerheads[cv->b.drawmode]->order2 ) {
	if ( active->prev!=NULL ) {
	    if ( active->noprevcp )
		active->prev->from->nonextcp = true;
	    else {
		active->prev->from->nextcp = active->prevcp;
		active->prev->from->nonextcp = false;
	    }
	    SplinePointNextCPChanged2(active->prev->from);
	    SplineRefigureFixup(active->prev);
	}
	if ( active->next!=NULL ) {
	    if ( active->nonextcp )
		active->next->to->noprevcp = true;
	    else {
		active->next->to->prevcp = active->nextcp;
		active->next->to->noprevcp = false;
	    }
	    SplineRefigureFixup(active->next);
	}
    } else {
	if ( active->prev!=NULL )
	    SplineRefigure(active->prev);
	if ( active->next!=NULL )
	    SplineRefigure(active->next);
    }
    CPUpdateInfo(cv,event);
    SCUpdateAll(cv->b.sc);
}

void CVMouseUpPoint(CharView *cv,GEvent *event) {
    SplinePoint *active = cv->active_sp;
    cv->lastselpt = active;
    cv->active_spl = NULL;
    cv->active_sp = NULL;
    cv->active_cp = NULL;
    cv->joinvalid = false;
    CVInfoDraw(cv,cv->gw);
    CPEndInfo(cv);
    if ( event->u.mouse.clicks>1 )
	CVGetInfo(cv);
}
