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
#include <utype.h>
#include <math.h>
#include "collabclient.h"
extern void BackTrace( const char* msg );

int stop_at_join = false;
extern int interpCPsOnMotion;

int CVAnySel(CharView *cv, int *anyp, int *anyr, int *anyi, int *anya) {
    int anypoints = 0, anyrefs=0, anyimages=0, anyanchor=0;
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *il;
    AnchorPoint *ap;
    int i;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL && !anypoints; spl = spl->next ) {
	if ( cv->b.sc->inspiro && hasspiro()) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		if ( SPIRO_SELECTED(&spl->spiros[i])) {
		    anypoints = true;
	    break;
		}
	} else {
	    first = NULL;
	    if ( spl->first->selected ) anypoints = true;
	    for ( spline=spl->first->next; spline!=NULL && spline!=first && !anypoints; spline = spline->to->next ) {
		if ( spline->to->selected ) anypoints = true;
		if ( first == NULL ) first = spline;
	    }
	}
    }
    for ( rf=cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL && !anyrefs; rf=rf->next )
	if ( rf->selected ) anyrefs = true;
    if ( cv->b.drawmode==dm_fore ) {
	if ( cv->showanchor && anya!=NULL )
	    for ( ap=cv->b.sc->anchor; ap!=NULL && !anyanchor; ap=ap->next )
		if ( ap->selected ) anyanchor = true;
    }
    for ( il=cv->b.layerheads[cv->b.drawmode]->images; il!=NULL && !anyimages; il=il->next )
	if ( il->selected ) anyimages = true;
    if ( anyp!=NULL ) *anyp = anypoints;
    if ( anyr!=NULL ) *anyr = anyrefs;
    if ( anyi!=NULL ) *anyi = anyimages;
    if ( anya!=NULL ) *anya = anyanchor;
return( anypoints || anyrefs || anyimages || anyanchor );
}

int CVAnySelPoints(CharView *cv) {
    /* if there are any points selected */
    SplinePointList *spl;
    Spline *spline, *first;
    int i;

    for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next ) {
	if ( cv->b.sc->inspiro && hasspiro()) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		if ( SPIRO_SELECTED(&spl->spiros[i]))
return( true );
	} else {
	    if ( spl->first->selected )
return( true );
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( spline->to->selected )
return( true );
		if ( first==NULL ) first = spline;
	    }
	}
    }
return( false );
}

GList_Glib*
CVGetSelectedPoints(CharView *cv)
{
    GList_Glib* ret = 0;
    /* if there are any points selected */
    SplinePointList *spl;
    Spline *spline, *first;
    int i;

    for ( spl= cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl=spl->next )
    {
	if ( cv->b.sc->inspiro && hasspiro())
	{
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		if ( SPIRO_SELECTED(&spl->spiros[i]))
		    ret = g_list_append( ret, &spl->spiros[i] );
	}
	else
	{
	    if ( spl->first->selected )
		ret = g_list_append( ret, spl->first );
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next )
	    {
		if ( spline->to->selected )
		    ret = g_list_append( ret, spline->to );
		if ( first==NULL ) first = spline;
	    }
	}
    }
    return ret;
}



int CVClearSel(CharView *cv) {
    SplinePointList *spl;
    int i;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;
    int needsupdate = 0;
    AnchorPoint *ap;

    CVFreePreTransformSPL( cv );
    
    cv->lastselpt = NULL; cv->lastselcp = NULL;
    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next )
    {
	if ( spl->first->selected )
	{
	    needsupdate = true;
	    spl->first->selected = false;
	    spl->first->nextcpselected = false;
	    spl->first->prevcpselected = false;
	}
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next )
	{
	    if ( spline->to->selected )
	    {
		needsupdate = true;
		spline->to->selected = false;
		spline->to->nextcpselected = false;
		spline->to->prevcpselected = false;
	    }
	    if ( first==NULL )
		first = spline;
	}
	for ( i=0 ; i<spl->spiro_cnt-1; ++i )
	    if ( SPIRO_SELECTED(&spl->spiros[i]))
	    {
		needsupdate = true;
		SPIRO_DESELECT(&spl->spiros[i]);
	    }
    }
    for ( rf=cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL; rf = rf->next )
	if ( rf->selected )
	{
	    needsupdate = true;
	    rf->selected = false;
	}
    if ( cv->b.drawmode == dm_fore )
	for ( ap=cv->b.sc->anchor; ap!=NULL; ap = ap->next )
	    if ( ap->selected )
	    {
		if ( cv->showanchor )
		    needsupdate = true;
		ap->selected = false;
	    }
    for ( img=cv->b.layerheads[cv->b.drawmode]->images; img!=NULL; img = img->next )
	if ( img->selected )
	{
	    needsupdate = true;
	    img->selected = false;
	}
    if ( cv->p.nextcp || cv->p.prevcp || cv->widthsel || cv->vwidthsel ||
	    cv->icsel || cv->tah_sel )
    {
	needsupdate = true;
    }
    cv->p.nextcp = cv->p.prevcp = false;
    cv->widthsel = cv->vwidthsel = cv->lbearingsel = cv->icsel = cv->tah_sel = false;

    needsupdate = 1;
    return( needsupdate );
}

int CVSetSel(CharView *cv,int mask) {
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;
    int needsupdate = 0;
    AnchorPoint *ap;
    RefChar *usemymetrics = HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv));
    int i;

    cv->lastselpt = NULL; cv->lastselcp = NULL;
    if ( mask&1 ) {
	if ( !cv->b.sc->inspiro || !hasspiro()) {
	    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
		if ( !spl->first->selected ) { needsupdate = true; spl->first->selected = true; }
		first = NULL;
		for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		    if ( !spline->to->selected )
			{ needsupdate = true; spline->to->selected = true; }
		    cv->lastselpt = spline->to;
		    if ( first==NULL ) first = spline;
		}
	    }
	} else {
	    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
		for ( i=0; i<spl->spiro_cnt-1; ++i ) {
		    if ( !SPIRO_SELECTED(&spl->spiros[i])) {
			needsupdate = true;
			SPIRO_SELECT(&spl->spiros[i]);
		    }
		    cv->lastselcp = &spl->spiros[i];
		}
	    }
	}
	for ( rf=cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL; rf = rf->next )
	    if ( !rf->selected ) { needsupdate = true; rf->selected = true; }
	for ( img=cv->b.layerheads[cv->b.drawmode]->images; img!=NULL; img = img->next )
	    if ( !img->selected ) { needsupdate = true; img->selected = true; }
    }
    if ( (mask&2) && cv->showanchor ) {
	for ( ap=cv->b.sc->anchor; ap!=NULL; ap=ap->next )
	    if ( !ap->selected ) { needsupdate = true; ap->selected = true; }
    }
    if ( cv->p.nextcp || cv->p.prevcp )
	needsupdate = true;
    cv->p.nextcp = cv->p.prevcp = false;
    if ( cv->showhmetrics && !cv->widthsel && (mask&4) && usemymetrics==NULL ) {
	cv->widthsel = needsupdate = true;
	cv->oldwidth = cv->b.sc->width;
    }
    if ( cv->showvmetrics && cv->b.sc->parent->hasvmetrics && !cv->vwidthsel && (mask&4) && usemymetrics==NULL ) {
	cv->vwidthsel = needsupdate = true;
	cv->oldvwidth = cv->b.sc->vwidth;
    }
return( needsupdate );
}

void CVInvertSel(CharView *cv) {
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;
    int i;

    cv->lastselpt = NULL; cv->lastselcp = NULL;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	if ( cv->b.sc->inspiro && hasspiro()) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		spl->spiros[i].ty ^= 0x80;
	} else {
	    spl->first->selected = !spl->first->selected;
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		spline->to->selected = !spline->to->selected;
		if ( spline->to->selected )
		    cv->lastselpt = spline->to;
		if ( first==NULL ) first = spline;
	    }
	    /* in circular case, first point is toggled twice in above code	*/
	    /* so fix it here						*/
	    if ( spline==first && spline != NULL)
		spl->first->selected = !spl->first->selected;
	}
    }
    for ( rf=cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL; rf = rf->next )
        rf->selected = !rf->selected;
    for ( img=cv->b.layerheads[cv->b.drawmode]->images; img!=NULL; img = img->next )
        img->selected = !img->selected;
    cv->p.nextcp = cv->p.prevcp = false;
}

int CVAllSelected(CharView *cv) {
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;
    int i;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	if ( cv->b.sc->inspiro && hasspiro()) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		if ( !SPIRO_SELECTED(&spl->spiros[i]))
return( false );
	} else {
	    if ( !spl->first->selected )
return( false );
	    first = NULL;
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( !spline->to->selected )
return( false );
		if ( first==NULL ) first = spline;
	    }
	}
    }
    for ( rf=cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL; rf = rf->next )
	if ( !rf->selected )
return( false );
    for ( img=cv->b.layerheads[cv->b.drawmode]->images; img!=NULL; img = img->next )
	if ( !img->selected )
return( false );
return( true );
}

static void SplineSetFindSelBounds(SplinePointList *spl, DBounds *bounds,
	int nosel, int inspiro) {
    SplinePoint *sp, *first;
    int i;

    for ( ; spl!=NULL; spl = spl->next ) {
	if ( !inspiro ) {
	    first = NULL;
	    for ( sp = spl->first; sp!=first; sp = sp->next->to ) {
		if ( nosel || sp->selected ) {
		    if ( bounds->minx==0 && bounds->maxx==0 &&
			 bounds->miny==0 && bounds->maxy == 0 ) {
			bounds->minx = bounds->maxx = sp->me.x;
			bounds->miny = bounds->maxy = sp->me.y;
		    } else {
			if ( sp->me.x<bounds->minx ) bounds->minx = sp->me.x;
			if ( sp->me.x>bounds->maxx ) bounds->maxx = sp->me.x;
			if ( sp->me.y<bounds->miny ) bounds->miny = sp->me.y;
			if ( sp->me.y>bounds->maxy ) bounds->maxy = sp->me.y;
		    }
		}
		if ( first==NULL ) first = sp;
		if ( sp->next==NULL )
	    break;
	    }
	} else {
	    for ( i=0; i<spl->spiro_cnt-1; ++i ) {
		if ( nosel || SPIRO_SELECTED(&spl->spiros[i])) {
		    if ( bounds->minx==0 && bounds->maxx==0 &&
			 bounds->miny==0 && bounds->maxy == 0 ) {
			bounds->minx = bounds->maxx = spl->spiros[i].x;
			bounds->miny = bounds->maxy = spl->spiros[i].y;
		    } else {
			if ( spl->spiros[i].x<bounds->minx ) bounds->minx = spl->spiros[i].x;
			if ( spl->spiros[i].x>bounds->maxx ) bounds->maxx = spl->spiros[i].x;
			if ( spl->spiros[i].y<bounds->miny ) bounds->miny = spl->spiros[i].y;
			if ( spl->spiros[i].y>bounds->maxy ) bounds->maxy = spl->spiros[i].y;
		    }
		}
	    }
	}
    }
}

void CVFindCenter(CharView *cv, BasePoint *bp, int nosel) {
    DBounds b;
    ImageList *img;

    b.minx = b.miny = b.maxx = b.maxy = 0;
    SplineSetFindSelBounds(cv->b.layerheads[cv->b.drawmode]->splines,&b,nosel,cv->b.sc->inspiro&& hasspiro());
    if ( cv->b.drawmode==dm_fore ) {
	RefChar *rf;
	for ( rf=cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL; rf=rf->next ) {
	    if ( nosel || rf->selected ) {
		if ( b.minx==0 && b.maxx==0 )
		    b = rf->bb;
		else {
		    if ( rf->bb.minx<b.minx ) b.minx = rf->bb.minx;
		    if ( rf->bb.miny<b.miny ) b.miny = rf->bb.miny;
		    if ( rf->bb.maxx>b.maxx ) b.maxx = rf->bb.maxx;
		    if ( rf->bb.maxy>b.maxy ) b.maxy = rf->bb.maxy;
		}
	    }
	}
    }
    for ( img=cv->b.layerheads[cv->b.drawmode]->images; img!=NULL; img=img->next ) {
	if ( nosel || img->selected ) {
	    if ( b.minx==0 && b.maxx==0 )
		b = img->bb;
	    else {
		if ( img->bb.minx<b.minx ) b.minx = img->bb.minx;
		if ( img->bb.miny<b.miny ) b.miny = img->bb.miny;
		if ( img->bb.maxx>b.maxx ) b.maxx = img->bb.maxx;
		if ( img->bb.maxy>b.maxy ) b.maxy = img->bb.maxy;
	    }
	}
    }
    bp->x = (b.minx+b.maxx)/2;
    bp->y = (b.miny+b.maxy)/2;
}

static int OnBB(CharView *cv, DBounds *bb, real fudge) {

    if ( cv->info.y < bb->miny-fudge || cv->info.y > bb->maxy+fudge ||
	    cv->info.x < bb->minx-fudge || cv->info.x > bb->maxx+fudge )
return( ee_none );

    cv->expandorigin.x = (cv->info.x-bb->minx)<(bb->maxx-cv->info.x) ?
		bb->maxx : bb->minx;
    cv->expandorigin.y = (cv->info.y-bb->miny)<(bb->maxy-cv->info.y) ?
		bb->maxy : bb->miny;
    cv->expandwidth = cv->expandorigin.x==bb->maxx? bb->minx-bb->maxx : bb->maxx-bb->minx;
    cv->expandheight = cv->expandorigin.y==bb->maxy? bb->miny-bb->maxy : bb->maxy-bb->miny;

    if (( cv->info.x < bb->minx + fudge && cv->info.y < bb->miny+ 4*fudge ) ||
	    ( cv->info.x < bb->minx + 4*fudge && cv->info.y < bb->miny+ fudge )) {
return( ee_sw );
    }
    if (( cv->info.x < bb->minx + fudge && cv->info.y > bb->maxy- 4*fudge ) ||
	    ( cv->info.x < bb->minx + 4*fudge && cv->info.y > bb->maxy- fudge ))
return( ee_nw );
    if (( cv->info.x > bb->maxx - fudge && cv->info.y < bb->miny+ 4*fudge ) ||
	    ( cv->info.x > bb->maxx - 4*fudge && cv->info.y < bb->miny+ fudge ))
return( ee_se );
    if (( cv->info.x > bb->maxx - fudge && cv->info.y > bb->maxy- 4*fudge ) ||
	    ( cv->info.x > bb->maxx - 4*fudge && cv->info.y > bb->maxy- fudge ))
return( ee_ne );
    if ( cv->info.x < bb->minx + fudge )
return( ee_right );
    if ( cv->info.x > bb->maxx - fudge )
return( ee_left );
    if ( cv->info.y < bb->miny + fudge )
return( ee_down );
    if ( cv->info.y > bb->maxy - fudge )
return( ee_up );

return( ee_none );
}

static void SetCur(CharView *cv) {
    static GCursor cursors[ee_max];

    if ( cursors[ee_nw]==0 ) {
	cursors[ee_none] = ct_mypointer;
	cursors[ee_nw]   = cursors[ee_se] = ct_nwse; cursors[ee_ne] = cursors[ee_sw] = ct_nesw;
	cursors[ee_left] = cursors[ee_right] = ct_leftright;
	cursors[ee_up]   = cursors[ee_down] = ct_updown;
    }
    GDrawSetCursor(cv->v,cursors[cv->expandedge]);
}

static int NearCaret(SplineChar *sc,real x,real fudge ) {
    PST *pst;
    int i;

    for ( pst=sc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next );
    if ( pst==NULL )
return( -1 );
    for ( i=0; i<pst->u.lcaret.cnt; ++i ) {
	if ( x>pst->u.lcaret.carets[i]-fudge && x<pst->u.lcaret.carets[i]+fudge )
return( i );
    }
return( -1 );
}

int CVNearRBearingLine( CharView* cv, real x, real fudge )
{
    RefChar *usemymetrics = HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv));
    return( cv->showhmetrics
	    && x>cv->b.sc->width-fudge
	    && x<cv->b.sc->width+fudge
	    && !cv->b.container
	    && !usemymetrics );
}
int CVNearLBearingLine( CharView* cv, real x, real fudge )
{
    RefChar *usemymetrics = HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv));
    return( cv->showhmetrics
	    && x>0-fudge
	    && x<0+fudge
	    && !cv->b.container && !usemymetrics );
}


void CVCheckResizeCursors(CharView *cv) {
    RefChar *ref;
    ImageList *img;
    int old_ee = cv->expandedge;
    real fudge = 3.5/cv->scale;

    cv->expandedge = ee_none;
    if ( cv->b.drawmode!=dm_grid ) {
	for ( ref=cv->b.layerheads[cv->b.drawmode]->refs; ref!=NULL; ref=ref->next ) if ( ref->selected ) {
	    if (( cv->expandedge = OnBB(cv,&ref->bb,fudge))!=ee_none )
	break;
	}
	if ( cv->expandedge == ee_none ) {
	    RefChar *usemymetrics = HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv));
	    /* if ( cv->showhmetrics && cv->info.x > cv->b.sc->width-fudge && */
	    /* 	 cv->info.x<cv->b.sc->width+fudge && cv->b.container==NULL && */
	    /* 	 usemymetrics==NULL ) */
      if ( cv->showhmetrics && NearCaret(cv->b.sc,cv->info.x,fudge)!=-1 &&
		    usemymetrics==NULL )
		cv->expandedge = ee_right;
	    else if( CVNearRBearingLine( cv, cv->info.x, fudge ))
		cv->expandedge = ee_right;
	    else if( CVNearLBearingLine( cv, cv->info.x, fudge ))
		cv->expandedge = ee_left;
	    if ( cv->showvmetrics && cv->b.sc->parent->hasvmetrics && cv->b.container==NULL &&
		    cv->info.y > /*cv->b.sc->parent->vertical_origin*/-cv->b.sc->vwidth-fudge &&
		    cv->info.y < /*cv->b.sc->parent->vertical_origin*/-cv->b.sc->vwidth+fudge )
		cv->expandedge = ee_down;
	}
    }
    for ( img=cv->b.layerheads[cv->b.drawmode]->images; img!=NULL; img=img->next ) if ( img->selected ) {
	if (( cv->expandedge = OnBB(cv,&img->bb,fudge))!=ee_none )
    break;
    }
    if ( cv->expandedge!=old_ee )
	SetCur(cv);
}

Undoes *CVPreserveMaybeState(CharView *cv, int isTState) {
    if( isTState )
	return CVPreserveTState( cv );
    return CVPreserveState( &cv->b );
}

Undoes *CVPreserveTState(CharView *cv) {
    int anyrefs;

    cv->p.transany = CVAnySel(cv,NULL,&anyrefs,NULL,NULL);
    cv->p.transanyrefs = anyrefs;

return( _CVPreserveTState(&cv->b,&cv->p));
}

void CVRestoreTOriginalState(CharView *cv) {
    _CVRestoreTOriginalState(&cv->b,&cv->p);
}

void CVUndoCleanup(CharView *cv) {
    _CVUndoCleanup(&cv->b,&cv->p);
}

static int ImgRefEdgeSelected(CharView *cv, FindSel *fs,GEvent *event) {
    RefChar *ref;
    ImageList *img;
    int update;

    cv->expandedge = ee_none;
    /* Check the bounding box of references if meta is up, or if they didn't */
    /*  click on a reference edge. Point being to allow people to select */
    /*  macron or other reference which fills the bounding box */
    if ( !(event->u.mouse.state&ksm_meta) ||
	    (fs->p->ref!=NULL && !fs->p->ref->selected)) {
	for ( ref=cv->b.layerheads[cv->b.drawmode]->refs; ref!=NULL; ref=ref->next ) if ( ref->selected ) {
	    if (( cv->expandedge = OnBB(cv,&ref->bb,fs->fudge))!=ee_none ) {
		ref->selected = false;
		update = CVClearSel(cv);
		ref->selected = true;
		if ( update )
		    SCUpdateAll(cv->b.sc);
		CVPreserveTState(cv);
		cv->p.ref = ref;
		SetCur(cv);
return( true );
	    }
	}
    }
    for ( img=cv->b.layerheads[cv->b.drawmode]->images; img!=NULL; img=img->next ) if ( img->selected ) {
	if (( cv->expandedge = OnBB(cv,&img->bb,fs->fudge))!=ee_none ) {
	    img->selected = false;
	    update = CVClearSel(cv);
	    img->selected = true;
	    if ( update )
		SCUpdateAll(cv->b.sc);
	    CVPreserveTState(cv);
	    cv->p.img = img;
	    SetCur(cv);
return( true );
	}
    }
return( false );
}

void CVUnselectAllBCP( CharView *cv )
{
    CVFindAndVisitSelectedControlPoints( cv, false,
					 FE_unselectBCP, 0 );

    // This should happen, but it effects the single selection with mouse
    // codepaths in bad ways as at 2013.Aug
    /* cv->p.nextcp = 0; */
    /* cv->p.prevcp = 0; */

}

void CVMouseDownPointer(CharView *cv, FindSel *fs, GEvent *event) {
    int needsupdate = false;
    int dowidth, dovwidth, doic, dotah, nearcaret;
    int dolbearing;
    RefChar *usemymetrics = HasUseMyMetrics(cv->b.sc,CVLayer((CharViewBase *) cv));
    int i;

    cv->p.splineAdjacentPointsSelected = 0;
    if( cv->p.spline )
    {
	cv->p.splineAdjacentPointsSelected =
	    cv->p.spline->from && cv->p.spline->to
	    && cv->p.spline->from->selected && cv->p.spline->to->selected;
    }
    
    if ( cv->pressed==NULL )
	cv->pressed = GDrawRequestTimer(cv->v,200,100,NULL);
    cv->last_c.x = cv->info.x; cv->last_c.y = cv->info.y;
    /* don't clear the selection if the things we clicked on were already */
    /*  selected, or if the user held the shift key down */
    if ( ImgRefEdgeSelected(cv,fs,event))
return;
    dowidth    = CVNearRBearingLine( cv, cv->p.cx, fs->fudge );
    dolbearing = CVNearLBearingLine( cv, cv->p.cx, fs->fudge );
    doic = ( cv->showhmetrics && cv->b.sc->italic_correction!=TEX_UNDEF &&
		cv->b.sc->italic_correction!=0 &&
		cv->p.cx>cv->b.sc->width+cv->b.sc->italic_correction-fs->fudge &&
		cv->p.cx<cv->b.sc->width+cv->b.sc->italic_correction+fs->fudge &&
		cv->b.container==NULL );
    dotah = ( cv->showhmetrics && cv->b.sc->top_accent_horiz!=TEX_UNDEF &&
		cv->p.cx>cv->b.sc->top_accent_horiz-fs->fudge &&
		cv->p.cx<cv->b.sc->top_accent_horiz+fs->fudge &&
		cv->b.container==NULL );
    dovwidth = ( cv->showvmetrics && cv->b.sc->parent->hasvmetrics && cv->b.container == NULL &&
		cv->p.cy>/*cv->b.sc->parent->vertical_origin*/-cv->b.sc->vwidth-fs->fudge &&
		cv->p.cy</*cv->b.sc->parent->vertical_origin*/-cv->b.sc->vwidth+fs->fudge &&
		usemymetrics==NULL );
    cv->nearcaret = nearcaret = -1;
    if ( cv->showhmetrics ) nearcaret = NearCaret(cv->b.sc,cv->p.cx,fs->fudge);
    if ( (fs->p->sp==NULL    || !fs->p->sp->selected) &&
	 (fs->p->spiro==NULL || !SPIRO_SELECTED(fs->p->spiro)) &&
	 (fs->p->ref==NULL   || !fs->p->ref->selected) &&
	 (fs->p->img==NULL   || !fs->p->img->selected) &&
	 (fs->p->ap==NULL    || !fs->p->ap->selected) &&
	 (!dowidth    || !cv->widthsel) &&
	 (!dolbearing || !cv->lbearingsel) && 
	 (!dovwidth   || !cv->vwidthsel) &&
	 (!doic  || !cv->icsel) &&
	 (!dotah || !cv->tah_sel) &&
	 !(event->u.mouse.state&ksm_shift))
    {
	needsupdate = CVClearSel(cv);
    }

//    printf("CVMouseDownPointer() dowidth:%d dolbearing:%d\n", dowidth, dolbearing );

    if ( !fs->p->anysel )
    {
//	printf("mousedown !anysel dow:%d dov:%d doid:%d dotah:%d nearcaret:%d\n", dowidth, dovwidth, doic, dotah, nearcaret );
	/* Nothing else... unless they clicked on the width line, check that */
	if ( dowidth )
	{
	    if ( event->u.mouse.state&ksm_shift )
		cv->widthsel = !cv->widthsel;
	    else
		cv->widthsel = true;
	    if ( cv->widthsel ) {
		cv->oldwidth = cv->b.sc->width;
		fs->p->cx = cv->b.sc->width;
		CVInfoDraw(cv,cv->gw);
		fs->p->anysel = true;
		cv->expandedge = ee_right;
	    } else
		cv->expandedge = ee_none;
	    SetCur(cv);
	    needsupdate = true;
	}
	if ( nearcaret!=-1 )
	{
	    PST *pst;
	    for ( pst=cv->b.sc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next );
	    cv->lcarets = pst;
	    cv->nearcaret = nearcaret;
	    cv->expandedge = ee_right;
	    SetCur(cv);
	}
	else if ( dolbearing )
	{
	    if ( event->u.mouse.state&ksm_shift )
		cv->lbearingsel = !cv->lbearingsel;
	    else
		cv->lbearingsel = true;
	    if ( cv->lbearingsel ) {
//		cv->oldlbearing = cv->b.sc->lbearing;
		fs->p->cx = 0;;
		CVInfoDraw(cv,cv->gw);
		fs->p->anysel = true;
		cv->expandedge = ee_left;
	    } else
		cv->expandedge = ee_none;
	    SetCur(cv);
	    needsupdate = true;
	}
	else if ( dovwidth )
	{
	    if ( event->u.mouse.state&ksm_shift )
		cv->vwidthsel = !cv->vwidthsel;
	    else
		cv->vwidthsel = true;
	    if ( cv->vwidthsel ) {
		cv->oldvwidth = cv->b.sc->vwidth;
		fs->p->cy = /*cv->b.sc->parent->vertical_origin*/-cv->b.sc->vwidth;
		CVInfoDraw(cv,cv->gw);
		fs->p->anysel = true;
		cv->expandedge = ee_down;
	    } else
		cv->expandedge = ee_none;
	    SetCur(cv);
	    needsupdate = true;
	}
	else if ( doic )
	{
	    if ( event->u.mouse.state&ksm_shift )
		cv->icsel = !cv->icsel;
	    else
		cv->icsel = true;
	    if ( cv->icsel ) {
		cv->oldic = cv->b.sc->italic_correction+cv->b.sc->width;
		fs->p->cx = cv->b.sc->italic_correction+cv->b.sc->width;
		CVInfoDraw(cv,cv->gw);
		fs->p->anysel = true;
		cv->expandedge = ee_right;
	    } else
		cv->expandedge = ee_none;
	    SetCur(cv);
	    needsupdate = true;
	}
	else if ( dotah )
	{
	    if ( event->u.mouse.state&ksm_shift )
		cv->tah_sel = !cv->tah_sel;
	    else
		cv->tah_sel = true;
	    if ( cv->tah_sel ) {
		cv->oldtah = cv->b.sc->top_accent_horiz;
		fs->p->cx = cv->b.sc->top_accent_horiz;
		CVInfoDraw(cv,cv->gw);
		fs->p->anysel = true;
		cv->expandedge = ee_right;
	    } else
		cv->expandedge = ee_none;
	    SetCur(cv);
	    needsupdate = true;
	}
	else
	{
//	    printf("mousedown !anysel ELSE\n");
	    //
	    // Allow dragging a box around some points to send that information
	    // to the other clients in the collab session
	    //
	    if( collabclient_inSession( &cv->b ))
		CVPreserveState(&cv->b);
	}
    }
    else if ( event->u.mouse.clicks<=1 && !(event->u.mouse.state&ksm_shift))
    {
	/* printf("CVMouseDownPointer(2) not shifting\n"); */
	/* printf("CVMouseDownPointer(2) cv->p.sp:%p\n", cv->p.sp ); */
	/* printf("CVMouseDownPointer(2) n:%p p:%p sp:%p spline:%p ap:%p\n", */
	/*        fs->p->nextcp,fs->p->prevcp, fs->p->sp, fs->p->spline, fs->p->ap ); */
	/* printf("CVMouseDownPointer(2) spl:%p\n", fs->p->spl ); */
	/* SPLFirstVisit( fs->p->spl->first, SPLFirstVisitorDebugSelectionState, 0 ); */
	CVUnselectAllBCP( cv );

	if ( fs->p->nextcp || fs->p->prevcp ) {
	    CPStartInfo(cv,event);
	    /* Needs update to draw control points selected */
	    needsupdate = true;
	} else if ( fs->p->sp!=NULL ) {
	    if ( !fs->p->sp->selected ) needsupdate = true;
	    fs->p->sp->selected = true;
	} else if ( fs->p->spiro!=NULL ) {
	    if ( !SPIRO_SELECTED(fs->p->spiro) ) needsupdate = true;
	    SPIRO_SELECT( fs->p->spiro );
	} else if ( fs->p->spline!=NULL && (!cv->b.sc->inspiro || !hasspiro())) {
	    if ( !fs->p->spline->to->selected &&
		    !fs->p->spline->from->selected ) needsupdate = true;
	    fs->p->spline->to->selected = true;
	    fs->p->spline->from->selected = true;
	} else if ( fs->p->img!=NULL ) {
	    if ( !fs->p->img->selected ) needsupdate = true;
	    fs->p->img->selected = true;
	} else if ( fs->p->ref!=NULL ) {
	    if ( !fs->p->ref->selected ) needsupdate = true;
	    fs->p->ref->selected = true;
	} else if ( fs->p->ap!=NULL ) {
	    if ( !fs->p->ap->selected ) needsupdate = true;
	    fs->p->ap->selected = true;
	}
    }
    else if ( event->u.mouse.clicks<=1 )
    {
	/* printf("CVMouseDownPointer(3) with shift... n:%p p:%p sp:%p spline:%p ap:%p\n", */
	/*        fs->p->nextcp,fs->p->prevcp, fs->p->sp, fs->p->spline, fs->p->ap ); */
	/* printf("CVMouseDownPointer(3) spl:%p\n", fs->p->spl ); */
	/* SPLFirstVisit( fs->p->spl->first, SPLFirstVisitorDebugSelectionState, 0 ); */

	if ( fs->p->nextcp || fs->p->prevcp ) {
	    /* Needs update to draw control points selected */
	    needsupdate = true;
	} else if ( fs->p->sp!=NULL ) {
	    needsupdate = true;
	    fs->p->sp->selected = !fs->p->sp->selected;
	} else if ( fs->p->spiro!=NULL ) {
	    needsupdate = true;
	    fs->p->spiro->ty ^= 0x80;
	} else if ( fs->p->spline!=NULL && (!cv->b.sc->inspiro || !hasspiro())) {
	    needsupdate = true;
	    fs->p->spline->to->selected = !fs->p->spline->to->selected;
	    fs->p->spline->from->selected = !fs->p->spline->from->selected;
	} else if ( fs->p->img!=NULL ) {
	    needsupdate = true;
	    fs->p->img->selected = !fs->p->img->selected;
	} else if ( fs->p->ref!=NULL ) {
	    needsupdate = true;
	    fs->p->ref->selected = !fs->p->ref->selected;
	} else if ( fs->p->ap!=NULL ) {
	    needsupdate = true;
	    fs->p->ap->selected = !fs->p->ap->selected;
	}
    }
    else if ( event->u.mouse.clicks==2 )
    {
	/* printf("mouse down click==2\n"); */
	CPEndInfo(cv);
	if ( fs->p->spl!=NULL ) {
	    if ( cv->b.sc->inspiro && hasspiro()) {
		for ( i=0; i<fs->p->spl->spiro_cnt-1; ++i ) {
		    if ( !SPIRO_SELECTED(&fs->p->spl->spiros[i])) {
			needsupdate = true;
			SPIRO_SELECT(&fs->p->spl->spiros[i]);
		    }
		}
	    } else {
		Spline *spline, *first;
		if ( !fs->p->spl->first->selected ) { needsupdate = true; fs->p->spl->first->selected = true; }
		first = NULL;
		for ( spline = fs->p->spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		    if ( !spline->to->selected )
			{ needsupdate = true; spline->to->selected = true; }
		    if ( first==NULL ) first = spline;
		}
	    }
	} else if ( fs->p->ref!=NULL || fs->p->img!=NULL ) {
	    /* Double clicking on a referenced character doesn't do much */
	} else if ( fs->p->ap!=NULL ) {
	    /* Select all Anchor Points at this location */
	    AnchorPoint *ap;
	    for ( ap=cv->b.sc->anchor; ap!=NULL; ap=ap->next )
		if ( ap->me.x==fs->p->ap->me.x && ap->me.y==fs->p->ap->me.y )
		    if ( !ap->selected ) {
			ap->selected = true;
			needsupdate = true;
		    }
	}
    }
    else if ( event->u.mouse.clicks==3 )
    {
	/* printf("mouse down click==3\n"); */
	if ( CVSetSel(cv,1)) needsupdate = true;
		/* don't select width or anchor points for three clicks */
		/*  but select all points, refs */
    }
    else
    {
	/* printf("mouse down ELSE\n"); */
	/* Select everything */
	if ( CVSetSel(cv,-1)) needsupdate = true;
    }

    
    if ( needsupdate )
	SCUpdateAll(cv->b.sc);

    /* lastselpt is set by our caller */
}

static int CVRectSelect(CharView *cv, real newx, real newy) {
    int any=false;
    DBounds old, new;
    RefChar *rf;
    ImageList *img;
    Spline *spline, *first;
    SplinePointList *spl;
    BasePoint *bp;
    AnchorPoint *ap;
    DBounds bb;
    int i;

    if ( cv->p.cx<=cv->p.ex ) {
	old.minx = cv->p.cx;
	old.maxx = cv->p.ex;
    } else {
	old.minx = cv->p.ex;
	old.maxx = cv->p.cx;
    }
    if ( cv->p.cy<=cv->p.ey ) {
	old.miny = cv->p.cy;
	old.maxy = cv->p.ey;
    } else {
	old.miny = cv->p.ey;
	old.maxy = cv->p.cy;
    }

    if ( cv->p.cx<=newx ) {
	new.minx = cv->p.cx;
	new.maxx = newx;
    } else {
	new.minx = newx;
	new.maxx = cv->p.cx;
    }
    if ( cv->p.cy<=newy ) {
	new.miny = cv->p.cy;
	new.maxy = newy;
    } else {
	new.miny = newy;
	new.maxy = cv->p.cy;
    }

    for ( rf = cv->b.layerheads[cv->b.drawmode]->refs; rf!=NULL; rf=rf->next ) {
	if (( rf->bb.minx>=old.minx && rf->bb.maxx<old.maxx &&
		    rf->bb.miny>=old.miny && rf->bb.maxy<old.maxy ) !=
		( rf->bb.minx>=new.minx && rf->bb.maxx<new.maxx &&
		    rf->bb.miny>=new.miny && rf->bb.maxy<new.maxy )) {
	    rf->selected = !rf->selected;
	    any = true;
	}
    }
    if ( cv->b.drawmode==dm_fore ) {
	if ( cv->showanchor ) for ( ap=cv->b.sc->anchor ; ap!=NULL; ap=ap->next ) {
	    bp = &ap->me;
	    if (( bp->x>=old.minx && bp->x<old.maxx &&
			bp->y>=old.miny && bp->y<old.maxy ) !=
		    ( bp->x>=new.minx && bp->x<new.maxx &&
			bp->y>=new.miny && bp->y<new.maxy )) {
		ap->selected = !ap->selected;
		any = true;
	    }
	}
    }

    for ( img = cv->b.layerheads[cv->b.drawmode]->images; img!=NULL; img=img->next ) {
	bb.minx = img->xoff;
	bb.miny = img->yoff;
	bb.maxx = img->xoff+GImageGetWidth(img->image)*img->xscale;
	bb.maxy = img->yoff+GImageGetHeight(img->image)*img->yscale;
	if (( bb.minx>=old.minx && bb.maxx<old.maxx &&
		    bb.miny>=old.miny && bb.maxy<old.maxy ) !=
		( bb.minx>=new.minx && bb.maxx<new.maxx &&
		    bb.miny>=new.miny && bb.maxy<new.maxy )) {
	    img->selected = !img->selected;
	    any = true;
	}
    }

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL; spl = spl->next ) {
	if ( !cv->b.sc->inspiro || !hasspiro()) {
	    first = NULL;
	    if ( spl->first->prev==NULL ) {
		bp = &spl->first->me;
		if (( bp->x>=old.minx && bp->x<old.maxx &&
			    bp->y>=old.miny && bp->y<old.maxy ) !=
			( bp->x>=new.minx && bp->x<new.maxx &&
			    bp->y>=new.miny && bp->y<new.maxy )) {
		    spl->first->selected = !spl->first->selected;
		    if ( spl->first->selected )
			cv->lastselpt = spl->first;
		    else if ( spl->first==cv->lastselpt )
			cv->lastselpt = NULL;
		    any = true;
		}
	    }
	    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		bp = &spline->to->me;
		if (( bp->x>=old.minx && bp->x<old.maxx &&
			    bp->y>=old.miny && bp->y<old.maxy ) !=
			( bp->x>=new.minx && bp->x<new.maxx &&
			    bp->y>=new.miny && bp->y<new.maxy )) {
		    spline->to->selected = !spline->to->selected;
		    if ( spline->to->selected )
			cv->lastselpt = spline->to;
		    else if ( spline->to==cv->lastselpt )
			cv->lastselpt = NULL;
		    any = true;
		}
		if ( first==NULL ) first = spline;
	    }
	} else {
	    for ( i=0; i<spl->spiro_cnt-1; ++i ) {
		if (( spl->spiros[i].x>=old.minx && spl->spiros[i].x<old.maxx &&
			    spl->spiros[i].y>=old.miny && spl->spiros[i].y<old.maxy ) !=
			( spl->spiros[i].x>=new.minx && spl->spiros[i].x<new.maxx &&
			    spl->spiros[i].y>=new.miny && spl->spiros[i].y<new.maxy )) {
		    spl->spiros[i].ty ^= 0x80;
		    if ( SPIRO_SELECTED(&spl->spiros[i]))
			cv->lastselcp = &spl->spiros[i];
		    any = true;
		}
	    }
	}
    }
return( any );
}

void CVAdjustControl(CharView *cv,BasePoint *cp, BasePoint *to) {
    SplinePoint *sp = cv->p.sp;

    SPAdjustControl(sp,cp,to,cv->b.layerheads[cv->b.drawmode]->order2);
    CVSetCharChanged(cv,true);
}

bool isSplinePointPartOfGuide( SplineFont *sf, SplinePoint *sp )
{
    if( !sp || !sf )
	return 0;
    if( !sf->grid.splines )
	return 0;

    SplinePointList* spl = sf->grid.splines;
    return SplinePointListContainsPoint( spl, sp );
}

static void CVAdjustSpline(CharView *cv) {
    Spline *old = cv->p.spline;
    TPoint tp[5];
    real t;
    Spline1D *oldx = &old->splines[0], *oldy = &old->splines[1];
    int oldfrompointtype, oldtopointtype;

    if ( cv->b.layerheads[cv->b.drawmode]->order2 )
return;

    //
    // Click + drag on a guide moves the guide to where your mouse is at
    //
    if( cv->b.drawmode == dm_grid
	&& isSplinePointPartOfGuide( cv->b.sc->parent, cv->p.spline->from )
	&& isSplinePointPartOfGuide( cv->b.sc->parent, cv->p.spline->to ) )
    {
	if( 0 == cv->p.spline->splines[0].a
	    && 0 == cv->p.spline->splines[0].b
	    && 0 == cv->p.spline->splines[1].a
	    && 0 == cv->p.spline->splines[1].b
	    && ( (cv->p.spline->splines[0].c && cv->p.spline->from->me.y == cv->p.spline->to->me.y)
		 || (cv->p.spline->splines[1].c && cv->p.spline->from->me.x == cv->p.spline->to->me.x )))
	{
	    if( cv->p.spline->from->me.y == cv->p.spline->to->me.y )
	    {
		int newy = cv->info.y;
		cv->p.spline->from->me.y = newy;
		cv->p.spline->to->me.y = newy;
		cv->p.spline->splines[1].d = newy;
	    }
	    else
	    {
		int newx = cv->info.x;
		cv->p.spline->from->me.x = newx;
		cv->p.spline->to->me.x = newx;
		cv->p.spline->splines[0].d = newx;
	    }
	    CVSetCharChanged(cv,true);
	    return;
	}
    }


    tp[0].x = cv->info.x; tp[0].y = cv->info.y; tp[0].t = cv->p.t;
    t = cv->p.t/10;
    tp[1].x = ((oldx->a*t+oldx->b)*t+oldx->c)*t + oldx->d;
    tp[1].y = ((oldy->a*t+oldy->b)*t+oldy->c)*t + oldy->d;
    tp[1].t = t;
    t = 1-(1-cv->p.t)/10;
    tp[2].x = ((oldx->a*t+oldx->b)*t+oldx->c)*t + oldx->d;
    tp[2].y = ((oldy->a*t+oldy->b)*t+oldy->c)*t + oldy->d;
    tp[2].t = t;
    tp[3] = tp[0];		/* Give more weight to this point than to the others */
    tp[4] = tp[0];		/*  ditto */
    cv->p.spline = ApproximateSplineFromPoints(old->from,old->to,tp,5,old->order2);

    /* don't change hvcurves to corners */
    oldfrompointtype = old->from->pointtype;
    oldtopointtype = old->to->pointtype;
    old->from->pointtype = old->to->pointtype = pt_corner;
    if ( oldfrompointtype == pt_hvcurve )
        SPChangePointType(old->from, pt_hvcurve);
    if ( oldtopointtype == pt_hvcurve )
        SPChangePointType(old->to, pt_hvcurve);
    //
    // dont go changing pt_curve points into pt_corner without explicit consent.
    //
    if( oldfrompointtype == pt_curve || oldfrompointtype == pt_tangent )
    {
	old->from->pointtype = oldfrompointtype;
	SPTouchControl( old->from, &old->from->nextcp, cv->b.layerheads[cv->b.drawmode]->order2 );
    }
    if( oldtopointtype == pt_curve || oldtopointtype == pt_tangent )
    {
	old->to->pointtype = oldtopointtype;
	SPTouchControl( old->to, &old->to->prevcp, cv->b.layerheads[cv->b.drawmode]->order2 );
    }

    old->from->nextcpdef = old->to->prevcpdef = false;
    SplineFree(old);
    CVSetCharChanged(cv,true);
}

static int Nearish(real a,real fudge) {
return( a>-fudge && a<fudge );
}

/* Are any of the selected points open (that is are they missing either a next*/
/*  or a prev spline so that at least one direction is free to make a new link)*/
/*  And did any of those selected points move on top of a point which was not */
/*  selected but which was also open? if so then merge all cases where this */
/*  happened (could be more than one) */
/* However if two things merge we must start all over again because we will have */
/*  freed one of the splinesets in the merger */
/* This is very similar for spiros (because start and end points of open contours */
/*  are the same in both representations). The only complication is checking */
/*  that they are selected */
static int CVCheckMerges(CharView *cv ) {
    SplineSet *activess, *mergess;
    real fudge = 2/cv->scale;
    int cnt= -1;
    int firstsel, lastsel;
    int mfirstsel, mlastsel;
    int inspiro = cv->b.sc->inspiro && hasspiro();

  restart:
    ++cnt;
    for ( activess=cv->b.layerheads[cv->b.drawmode]->splines; activess!=NULL; activess=activess->next ) {
	if ( activess->first->prev!=NULL )
    continue;		/* Closed contour, can't merge with anything */
	firstsel = (inspiro && SPIRO_SELECTED(&activess->spiros[0])) ||
		    (!inspiro && activess->first->selected);
	lastsel = (inspiro && SPIRO_SELECTED(&activess->spiros[activess->spiro_cnt-2])) ||
		    (!inspiro && activess->last->selected);
	if ( firstsel || lastsel ) {
	    for ( mergess = cv->b.layerheads[cv->b.drawmode]->splines; mergess!=NULL; mergess=mergess->next ) {
		if ( mergess->first->prev!=NULL )
	    continue;		/* Closed contour, can't merge with anything */
		mfirstsel = (inspiro && SPIRO_SELECTED(&mergess->spiros[0])) ||
			    (!inspiro && mergess->first->selected);
		mlastsel = (inspiro && SPIRO_SELECTED(&mergess->spiros[mergess->spiro_cnt-2])) ||
			    (!inspiro && mergess->last->selected);
		if ( !mfirstsel || !mlastsel ) {
		    if ( !mfirstsel && firstsel &&
			    Nearish(mergess->first->me.x-activess->first->me.x,fudge) &&
			    Nearish(mergess->first->me.y-activess->first->me.y,fudge)) {
			CVMergeSplineSets(cv,activess->first,activess,
				mergess->first,mergess);
  goto restart;
		    }
		    if ( !mlastsel && firstsel &&
			    Nearish(mergess->last->me.x-activess->first->me.x,fudge) &&
			    Nearish(mergess->last->me.y-activess->first->me.y,fudge)) {
			CVMergeSplineSets(cv,activess->first,activess,
				mergess->last,mergess);
  goto restart;
		    }
		    if ( !mfirstsel && lastsel &&
			    Nearish(mergess->first->me.x-activess->last->me.x,fudge) &&
			    Nearish(mergess->first->me.y-activess->last->me.y,fudge)) {
			CVMergeSplineSets(cv,activess->last,activess,
				mergess->first,mergess);
  goto restart;
		    }
		    if ( !mlastsel && lastsel &&
			    Nearish(mergess->last->me.x-activess->last->me.x,fudge) &&
			    Nearish(mergess->last->me.y-activess->last->me.y,fudge)) {
			CVMergeSplineSets(cv,activess->last,activess,
				mergess->last,mergess);
  goto restart;
		    }
		}
	    }
	}
    }
return( cnt>0 && stop_at_join );
}

static void adjustLBearing( CharView *cv, SplineChar *sc, real val )
{
    DBounds bb;
    SplineCharFindBounds(sc,&bb);
    if ( val != 0 )
    {
	real transform[6];
	transform[0] = transform[3] = 1.0;
	transform[1] = transform[2] = transform[5] = 0;
	transform[4] = val;
	printf("adjustLBearing val:%f min:%f v-min:%f\n",val,bb.minx,(bb.minx+val));
	FVTrans( (FontViewBase *) cv->b.fv, sc, transform, NULL, 0 | fvt_alllayers );
	// We copy and adapt some code from FVTrans in order to adjust the CharView carets.
	// We omit the fvt_scalepstpos for FVTrans since other CharView code seems to skip updating the SplineChar.
	PST *pst;
	for ( pst = cv->b.sc->possub; pst!=NULL; pst=pst->next ) {
	    if ( pst->type == pst_lcaret ) {
		int j;
		for ( j=0; j<pst->u.lcaret.cnt; ++j )
		    pst->u.lcaret.carets[j] = rint(pst->u.lcaret.carets[j]+val);
	    }
	}
    }
}

/* Move the selection and return whether we did a merge */
int CVMoveSelection(CharView *cv, real dx, real dy, uint32 input_state) {
    real transform[6];
    RefChar *refs;
    ImageList *img;
    AnchorPoint *ap;
    double fudge;
    extern float snapdistance;
    int i,j;
    int changed = false, outlinechanged = false;
    SplinePointList *spl;
    SplinePoint *sp;

    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = 0.0;
    transform[4] = dx; transform[5] = dy;
    if ( transform[4]==0 && transform[5]==0 )
return(false);
    for ( spl=cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL && !outlinechanged; spl=spl->next ) {
	if ( cv->b.sc->inspiro && hasspiro()) {
	    for ( i=0; i<spl->spiro_cnt-1; ++i )
		if ( SPIRO_SELECTED(&spl->spiros[i])) {
		    outlinechanged = true;
	    break;
		}
	} else {
	    for ( sp=spl->first ;; ) {
		if ( sp->selected ) {
		    outlinechanged = true;
	    break;
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==spl->first )
	    break;
	    }
	}
    }

    enum transformPointMask tpmask = 0;
    tpmask |= tpmask_dontFixControlPoints;

    if ( cv->b.sc->inspiro && hasspiro())
	SplinePointListSpiroTransform(cv->b.layerheads[cv->b.drawmode]->splines,transform,false);
    else
    {
	bool interp = CVShouldInterpolateCPsOnMotion( cv );

	SplinePointListTransformExtended(cv->b.layerheads[cv->b.drawmode]->splines,transform,
					 interp ? tpt_OnlySelectedInterpCPs : tpt_OnlySelected,
					 tpmask );

	if( interp )
	{
	    bool preserveState = false;
	    CVVisitAllControlPoints( cv, preserveState,
	    			     FE_touchControlPoint,
	    			     (void*)(intptr_t)cv->b.layerheads[cv->b.drawmode]->order2 );
	}
    }
    

    for ( refs = cv->b.layerheads[cv->b.drawmode]->refs; refs!=NULL; refs=refs->next ) if ( refs->selected ) {
	refs->transform[4] += transform[4];
	refs->transform[5] += transform[5];
	refs->bb.minx += transform[4]; refs->bb.maxx += transform[4];
	refs->bb.miny += transform[5]; refs->bb.maxy += transform[5];
	for ( j=0; j<refs->layer_cnt; ++j )
	    SplinePointListTransform(refs->layers[j].splines,transform,tpt_AllPoints);
	outlinechanged = true;
    }
    if ( CVLayer( (CharViewBase *) cv) > ly_back ) {
	if ( cv->showanchor ) {
	    for ( ap=cv->b.sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->selected ) {
		ap->me.x += transform[4];
		ap->me.y += transform[5];
		changed = true;
	    }
	}
    }
    for ( img = cv->b.layerheads[cv->b.drawmode]->images; img!=NULL; img=img->next ) if ( img->selected ) {
	img->xoff += transform[4];
	img->yoff += transform[5];
	img->bb.minx += transform[4]; img->bb.maxx += transform[4];
	img->bb.miny += transform[5]; img->bb.maxy += transform[5];
	SCOutOfDateBackground(cv->b.sc);
	changed = true;
    }
    fudge = snapdistance/cv->scale/2;
    if ( cv->widthsel ) {
	if ( cv->b.sc->width+dx>0 && ((int16) (cv->b.sc->width+dx))<0 )
	    cv->b.sc->width = 32767;
	else if ( cv->b.sc->width+dx<0 && ((int16) (cv->b.sc->width+dx))>0 )
	    cv->b.sc->width = -32768;
	else
	    cv->b.sc->width += dx;
	if ( cv->b.sc->width>=-fudge && cv->b.sc->width<fudge )
	    cv->b.sc->width = 0;
	changed = true;
    }
    if ( cv->lbearingsel ) {

	printf("lbearing dx:%f\n", dx );
	adjustLBearing( cv, cv->b.sc, dx );
	changed = true;
    }
    if ( cv->vwidthsel ) {
	if ( cv->b.sc->vwidth-dy>0 && ((int16) (cv->b.sc->vwidth-dy))<0 )
	    cv->b.sc->vwidth = 32767;
	else if ( cv->b.sc->vwidth-dy<0 && ((int16) (cv->b.sc->vwidth-dy))>0 )
	    cv->b.sc->vwidth = -32768;
	else
	    cv->b.sc->vwidth -= dy;
	if ( cv->b.sc->vwidth>=-fudge && cv->b.sc->vwidth<fudge )
	    cv->b.sc->vwidth = 0;
	changed = true;
    }
    if ( cv->icsel ) {
	if ( cv->b.sc->italic_correction+dx>0 && ((int16) (cv->b.sc->italic_correction+dx))<0 )
	    cv->b.sc->italic_correction = 32767-1;
	else if ( cv->b.sc->italic_correction+dx<0 && ((int16) (cv->b.sc->italic_correction+dx))>0 )
	    cv->b.sc->italic_correction = -32768;
	else
	    cv->b.sc->italic_correction += dx;
	if ( cv->b.sc->italic_correction>=-fudge && cv->b.sc->italic_correction<fudge )
	    cv->b.sc->italic_correction = 0;
	changed = true;
    }
    if ( cv->tah_sel ) {
	if ( cv->b.sc->top_accent_horiz+dx>0 && ((int16) (cv->b.sc->top_accent_horiz+dx))<0 )
	    cv->b.sc->top_accent_horiz = 32767-1;
	else if ( cv->b.sc->top_accent_horiz+dx<0 && ((int16) (cv->b.sc->top_accent_horiz+dx))>0 )
	    cv->b.sc->top_accent_horiz = -32768;
	else
	    cv->b.sc->top_accent_horiz += dx;
	if ( cv->b.sc->top_accent_horiz>=-fudge && cv->b.sc->top_accent_horiz<fudge )
	    cv->b.sc->top_accent_horiz = 0;
	changed = true;
    }
    if ( outlinechanged )
	CVSetCharChanged(cv,true);
    else if ( changed )
	CVSetCharChanged(cv,2);
    if ( input_state&ksm_meta )
return( false );			/* Don't merge if the meta key is down */

return( CVCheckMerges( cv ));
}

static int CVExpandEdge(CharView *cv) {
    real transform[6];
    real xscale=1.0, yscale=1.0;

    CVRestoreTOriginalState(cv);
    if ( cv->expandedge != ee_up && cv->expandedge != ee_down )
	xscale = (cv->info.x-cv->expandorigin.x)/cv->expandwidth;
    if ( cv->expandedge != ee_left && cv->expandedge != ee_right )
	yscale = (cv->info.y-cv->expandorigin.y)/cv->expandheight;
    transform[0] = xscale; transform[3] = yscale;
    transform[1] = transform[2] = 0;
    transform[4] = (1-xscale)*cv->expandorigin.x;
    transform[5] = (1-yscale)*cv->expandorigin.y;
    CVSetCharChanged(cv,true);
    CVTransFunc(cv,transform,false);
return( true );
}

static void touchControlPointsVisitor ( void* key,
				 void* value,
				 SplinePoint* sp,
				 BasePoint *which,
				 bool isnext,
				 void* udata )
{
    SPTouchControl( sp, which, (int)(intptr_t)udata );
}

#ifndef MAX
#define MAX(x,y)   (((x) > (y)) ? (x) : (y))
#endif
#ifndef MIN
#define MIN(x,y)   (((x) < (y)) ? (x) : (y))
#endif

BasePoint nearest_point_on_line_segment(BasePoint p1, BasePoint p2, BasePoint p3) {
	double x_diff = p2.x - p1.x;
	double y_diff = p2.y - p1.y;
	double slope_1 = (p2.y - p1.y) / (p2.x - p1.x);
	double inverse_slope_1 = (p2.x - p1.x) / (p2.y - p1.y);
	BasePoint output_1;
	output_1.x = 0;
	output_1.y = 0;
	// Accuracy may be important, so we try to use the longer dimension.
	if (x_diff == 0) {
		// The line is vertical.
		output_1.x = p1.x;
		output_1.y = p3.y;
	} else if (y_diff == 0) {
		// The line is horizontal.
		output_1.x = p3.x;
		output_1.y = p1.y;
	} else {
		output_1.x = (p3.x/slope_1 + slope_1*p1.x + p3.y - p1.y)/(slope_1 + 1/slope_1);
		output_1.y = (p3.y/inverse_slope_1 + inverse_slope_1*p1.y + p3.x - p1.x)/(inverse_slope_1 + 1/inverse_slope_1);
	}
	// We are using the segment between p1 and p2, not the line through them, so we must crop.
	double min_x = MIN(p1.x, p2.x), max_x = MAX(p1.x, p2.x), min_y = MIN(p1.y, p2.y), max_y = MAX(p1.y, p2.y);
	if (output_1.x < min_x) output_1.x = min_x;
	if (output_1.x > max_x) output_1.x = max_x;
	if (output_1.y < min_y) output_1.y = min_y;
	if (output_1.y > max_y) output_1.y = max_y;
	// Note that it is not necessary to conform these points to the line since the corners of the box are on the line.
	return output_1;
}

int CVMouseMovePointer(CharView *cv, GEvent *event) {
    extern float arrowAmount;
    int needsupdate = false;
    int did_a_merge = false;
    int touch_control_points = false;

    
    /* if we haven't moved from the original location (ever) then this is a noop */
    if ( !cv->p.rubberbanding && !cv->recentchange &&
	    RealNear(cv->info.x,cv->p.cx) && RealNear(cv->info.y,cv->p.cy) )
return( false );

    /* This can happen if they depress on a control point, move it, then use */
    /*  the arrow keys to move the point itself, and then try to move the cp */
    /*  again (mouse still depressed) */
    if (( cv->p.nextcp || cv->p.prevcp ) && cv->p.sp==NULL )
	cv->p.nextcp = cv->p.prevcp = false;

    /* I used to have special cases for moving width lines, but that's now */
    /*  done by move selection */
    if ( cv->expandedge!=ee_none && !cv->widthsel && !cv->vwidthsel && !cv->lbearingsel
	 && cv->nearcaret==-1 && !cv->icsel && !cv->tah_sel )
    {
	if( !cv->changedActiveGlyph )
	{
	    needsupdate = CVExpandEdge(cv);
	}
    }
    else if ( cv->nearcaret!=-1 && cv->lcarets!=NULL ) {
	if ( cv->info.x!=cv->last_c.x ) {
	    if ( !cv->recentchange ) SCPreserveLayer(cv->b.sc,CVLayer((CharViewBase *) cv),2);
	    cv->lcarets->u.lcaret.carets[cv->nearcaret] += cv->info.x-cv->last_c.x;
	    if ( cv->lcarets->u.lcaret.carets[cv->nearcaret]<0 )
		cv->lcarets->u.lcaret.carets[cv->nearcaret] = 0;
	    needsupdate = true;
	    CVSetCharChanged(cv,true);
	}
    } else if ( !cv->p.anysel ) {
	if ( !cv->p.rubberbanding ) {
	    cv->p.ex = cv->p.cx;
	    cv->p.ey = cv->p.cy;
	}
	needsupdate = CVRectSelect(cv,cv->info.x,cv->info.y);
	if ( !needsupdate && cv->p.rubberbanding )
	    CVDrawRubberRect(cv->v,cv);
	/* printf("moving2 cx:%g cy:%g\n", cv->p.cx, cv->p.cy ); */
	cv->p.ex = cv->info.x;
	cv->p.ey = cv->info.y;
	cv->p.rubberbanding = true;
	if ( !needsupdate )
	    CVDrawRubberRect(cv->v,cv);
    }
    else if ( cv->p.nextcp )
    {
	if ( !cv->recentchange )
	    CVPreserveState(&cv->b);

	FE_adjustBCPByDeltaData d;
	memset( &d, 0, sizeof(FE_adjustBCPByDeltaData));
	d.cv = cv;
	d.dx = (cv->info.x - cv->p.sp->nextcp.x) * arrowAmount;
	d.dy = (cv->info.y - cv->p.sp->nextcp.y) * arrowAmount;
	visitSelectedControlPointsVisitor func = FE_adjustBCPByDelta;
	/* printf("move sp:%p ncp:%p \n", */
	/*        cv->p.sp, &(cv->p.sp->nextcp)  ); */
	/* printf("move me.x:%f me.y:%f\n", cv->p.sp->me.x, cv->p.sp->me.y ); */
	/* printf("move ncp.x:%f ncp.y:%f ix:%f iy:%f\n", */
	/*        cv->p.sp->nextcp.x, cv->p.sp->nextcp.y, */
	/*        cv->info.x, cv->info.y ); */
	/* printf("move dx:%f dy:%f\n",  d.dx, d.dy ); */
	/* printf("move dx:%f \n", cv->info.x - cv->p.sp->nextcp.x ); */
	if( cv->activeModifierAlt )
	    func = FE_adjustBCPByDeltaWhilePreservingBCPAngle;

	CVFindAndVisitSelectedControlPoints( cv, false, func, &d );
	CPUpdateInfo(cv,event);
	needsupdate = true;
    }
    else if ( cv->p.prevcp )
    {
	if ( !cv->recentchange )
	    CVPreserveState(&cv->b);

	FE_adjustBCPByDeltaData d;
	memset( &d, 0, sizeof(FE_adjustBCPByDeltaData));
	d.cv = cv;
	d.dx = (cv->info.x - cv->p.sp->prevcp.x) * arrowAmount;
	d.dy = (cv->info.y - cv->p.sp->prevcp.y) * arrowAmount;
	visitSelectedControlPointsVisitor func = FE_adjustBCPByDelta;
	
	if( cv->activeModifierAlt )
	    func = FE_adjustBCPByDeltaWhilePreservingBCPAngle;
	
	CVFindAndVisitSelectedControlPoints( cv, false, func, &d );
	CPUpdateInfo(cv,event);
	needsupdate = true;
    }
    else if ( cv->p.spline
		&& !cv->p.splineAdjacentPointsSelected
		&& (!cv->b.sc->inspiro || !hasspiro()))
    {
	if ( !cv->recentchange )
	    CVPreserveState(&cv->b);
	
	CVAdjustSpline(cv);
	CVSetCharChanged(cv,true);
	needsupdate = true;
	touch_control_points = true;
    }
    else
    {
	if ( !cv->recentchange )
	    CVPreserveState(&cv->b);

	BasePoint tmpp1;
	BasePoint tmpp2;
	tmpp1.x = cv->info.x;
	tmpp1.y = cv->info.y;
	tmpp2.x = cv->info.x;
	tmpp2.y = cv->info.y;
	BasePoint cachecp1 = (cv->p.sp ? cv->p.sp->prevcp : (BasePoint){0, 0});
	BasePoint cachecp2 = (cv->p.sp ? cv->p.sp->nextcp : (BasePoint){0, 0});
	double xadj = cv->info.x-cv->last_c.x;
	double yadj = cv->info.y-cv->last_c.y;
	touch_control_points = true;
	// The modifier is wrong.
	if (cv->p.anysel && cv->p.sp && event->u.mouse.state & ksm_control) {
		// Identify the individual point clicked. Find its control points. Move the selected point on a line between those control points.
		tmpp1 = nearest_point_on_line_segment((BasePoint){cv->p.sp->prevcp.x,cv->p.sp->prevcp.y}, \
			(BasePoint){cv->p.sp->nextcp.x,cv->p.sp->nextcp.y}, (BasePoint){cv->info.x, cv->info.y});
		// We also need to rebase the original point onto that line segment so that the movement is exactly along the line even if the original click is not.
		tmpp2 = nearest_point_on_line_segment((BasePoint){cv->p.sp->prevcp.x,cv->p.sp->prevcp.y}, \
			(BasePoint){cv->p.sp->nextcp.x,cv->p.sp->nextcp.y}, (BasePoint){cv->last_c.x, cv->last_c.y});
		xadj = tmpp1.x-tmpp2.x;
		yadj = tmpp1.y-tmpp2.y;
		touch_control_points = false; // We will need to move the control points back (but only for the point dragged).
	}
	
	did_a_merge = CVMoveSelection(cv,
		xadj,yadj,
		event->u.mouse.state);
	// Rather than create a new set of functions for moving points without their control points, we instead just restore them if we did not want them moved.
	if (cv->p.sp && touch_control_points == false) {
		cv->p.sp->prevcp = cachecp1;
		cv->p.sp->nextcp = cachecp2;
		touch_control_points = true;
    		AdjustControls(cv->p.sp);
	}
	needsupdate = true;

    }


    if ( needsupdate )
    {
	SCUpdateAll(cv->b.sc);
	CVGridHandlePossibleFitChar( cv );
    }

    if ( touch_control_points )
    {
	// We should really only need to visit the Adjacent CP
	// visiting all is a hammer left below in case it might be needed.
	CVVisitAdjacentToSelectedControlPoints( cv, false,
						touchControlPointsVisitor,
						(void*)(intptr_t)cv->b.layerheads[cv->b.drawmode]->order2 );
	/* CVVisitAllControlPoints( cv, false, */
	/* 			 touchControlPointsVisitor, */
	/* 			 (void*)cv->b.layerheads[cv->b.drawmode]->order2 ); */

	GDrawRequestExpose(cv->v,NULL,false);
    }

    cv->last_c.x = cv->info.x; cv->last_c.y = cv->info.y;
return( did_a_merge );
}




void CVMouseUpPointer(CharView *cv ) {
    static char *buts[3];
    buts[0] = _("_Yes");
    buts[1] = _("_No");
    buts[2] = NULL;

    if ( cv->widthsel )
    {
	if ( cv->b.sc->width<0 && cv->oldwidth>=0 ) {
	    if ( gwwv_ask(_("Negative Width"), (const char **) buts, 0, 1, _("Negative character widths are not allowed in TrueType.\nDo you really want a negative width?") )==1 )
		cv->b.sc->width = cv->oldwidth;
	}
	SCSynchronizeWidth(cv->b.sc,cv->b.sc->width,cv->oldwidth,NULL);
	cv->expandedge = ee_none;
	GDrawSetCursor(cv->v,ct_mypointer);
    }
    if ( cv->lbearingsel )
    {
	printf("oldlbearing:%f\n", cv->oldlbearing );
	cv->expandedge = ee_none;
	GDrawSetCursor(cv->v,ct_mypointer);
    }
    if ( cv->vwidthsel )
    {
	if ( cv->b.sc->vwidth<0 && cv->oldvwidth>=0 ) {
	    if ( gwwv_ask(_("Negative Width"), (const char **) buts, 0, 1, _("Negative character widths are not allowed in TrueType.\nDo you really want a negative width?") )==1 )
		cv->b.sc->vwidth = cv->oldvwidth;
	}
	cv->expandedge = ee_none;
	GDrawSetCursor(cv->v,ct_mypointer);
    }
    if ( cv->nearcaret!=-1 && cv->lcarets!=NULL )
    {
	cv->nearcaret = -1;
	cv->expandedge = ee_none;
	cv->lcarets = NULL;
	GDrawSetCursor(cv->v,ct_mypointer);
    }
    if( cv->changedActiveGlyph )
    {
	cv->changedActiveGlyph = 0;
    }
    else
    {
	if ( cv->expandedge!=ee_none )
	{
	    CVUndoCleanup(cv);
	    cv->expandedge = ee_none;
	    GDrawSetCursor(cv->v,ct_mypointer);
	}
	else if ( CVAllSelected(cv) && cv->b.drawmode==dm_fore && cv->p.spline==NULL
		  && !cv->p.prevcp && !cv->p.nextcp && cv->info.y==cv->p.cy )
	{
	    SCUndoSetLBearingChange(cv->b.sc,(int) rint(cv->info.x-cv->p.cx));
	    SCSynchronizeLBearing(cv->b.sc,cv->info.x-cv->p.cx,CVLayer((CharViewBase *) cv));
	}
    }
    CPEndInfo(cv);
}

/* ************************************************************************** */
/*  ************************** Select Point Dialog *************************  */
/* ************************************************************************** */

static spiro_cp *SpiroClosest(BasePoint *base,spiro_cp *sp1, spiro_cp *sp2) {
    double dx1, dy1, dx2, dy2;
    if ( sp1==NULL )
return( sp2 );
    if ( sp2==NULL )
return( sp1 );
    if ( (dx1 = sp1->x-base->x)<0 ) dx1 = -dx1;
    if ( (dy1 = sp1->y-base->y)<0 ) dy1 = -dy1;
    if ( (dx2 = sp2->x-base->x)<0 ) dx2 = -dx2;
    if ( (dy2 = sp2->y-base->y)<0 ) dy2 = -dy2;
    if ( dx1+dy1 < dx2+dy2 )
return( sp1 );
    else
return( sp2 );
}

static SplinePoint *Closest(BasePoint *base,SplinePoint *sp1, SplinePoint *sp2) {
    double dx1, dy1, dx2, dy2;
    if ( sp1==NULL )
return( sp2 );
    if ( sp2==NULL )
return( sp1 );
    if ( (dx1 = sp1->me.x-base->x)<0 ) dx1 = -dx1;
    if ( (dy1 = sp1->me.y-base->y)<0 ) dy1 = -dy1;
    if ( (dx2 = sp2->me.x-base->x)<0 ) dx2 = -dx2;
    if ( (dy2 = sp2->me.y-base->y)<0 ) dy2 = -dy2;
    if ( dx1+dy1 < dx2+dy2 )
return( sp1 );
    else
return( sp2 );
}

static int SelectPointsWithin(CharView *cv, BasePoint *base, double fuzz, BasePoint *bounds) {
    SplineSet *ss;
    SplinePoint *sp;
    SplinePoint *any = NULL;
    int i;
    spiro_cp *anycp = NULL;

    CVClearSel(cv);
    if ( cv->b.sc->inspiro && hasspiro()) {
	for ( ss= cv->b.layerheads[cv->b.drawmode]->splines; ss!=NULL; ss=ss->next ) {
	    for ( i=0; i<ss->spiro_cnt-1; ++i ) {
		spiro_cp *cp = &ss->spiros[i];
		if ( bounds!=NULL ) {
		    if ( cp->x >= base->x && cp->x <= base->x+bounds->x &&
			    cp->y >= base->y && cp->y <= base->y+bounds->y ) {
			SPIRO_SELECT(cp);
			anycp = cp;
		    }
		} else if ( fuzz>0 ) {
		    if ( RealWithin(cp->x,base->x,fuzz) && RealWithin(cp->y,base->y,fuzz)) {
			SPIRO_SELECT(cp);
			anycp = SpiroClosest(base,anycp,cp);
		    }
		} else {
		    if ( RealNear(cp->x,base->x) && RealNear(cp->y,base->y)) {
			SPIRO_SELECT(cp);
			anycp = SpiroClosest(base,anycp,cp);
      goto cpdone;
		    }
		}
	    }
	}
      cpdone:
	if ( any==NULL ) {
	    CVShowPoint(cv,base);
	    GDrawBeep(NULL);
	} else {
	    BasePoint here;
	    here.x = anycp->x;
	    here.y = anycp->y;
	    CVShowPoint(cv,&here);
	}
	SCUpdateAll(cv->b.sc);
return( anycp!=NULL );
    } else {
	for ( ss= cv->b.layerheads[cv->b.drawmode]->splines; ss!=NULL; ss=ss->next ) {
	    for ( sp=ss->first; ; ) {
		if ( bounds!=NULL ) {
		    if ( sp->me.x >= base->x && sp->me.x <= base->x+bounds->x &&
			    sp->me.y >= base->y && sp->me.y <= base->y+bounds->y ) {
			sp->selected = true;
			any = sp;
		    }
		} else if ( fuzz>0 ) {
		    if ( RealWithin(sp->me.x,base->x,fuzz) && RealWithin(sp->me.y,base->y,fuzz)) {
			sp->selected = true;
			any = Closest(base,any,sp);
		    }
		} else {
		    if ( RealNear(sp->me.x,base->x) && RealNear(sp->me.y,base->y)) {
			sp->selected = true;
			any = Closest(base,any,sp);
      goto done;
		    }
		}
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==ss->first )
	    break;
	    }
	}
      done:
	if ( any==NULL ) {
	    CVShowPoint(cv,base);
	    GDrawBeep(NULL);
	} else
	    CVShowPoint(cv,&any->me);
	SCUpdateAll(cv->b.sc);
return( any!=NULL );
    }
}

#define CID_X	1001
#define CID_Y	1002
#define CID_Width	1003
#define CID_Height	1004
#define CID_Fuzz	1005
#define CID_Exact	1006
#define CID_Within	1007
#define CID_Fuzzy	1008

struct selpt {
    int done;
    CharView *cv;
    GWindow gw;
};

static void SPA_DoCancel(struct selpt *pa) {
    pa->done = true;
}

static int SPA_OK(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	GWindow gw = GGadgetGetWindow(g);
	struct selpt *pa = GDrawGetUserData(gw);
	CharView *cv = pa->cv;
	BasePoint base, bounds;
	double fuzz;
	int err = false, ret;

/* GT: X is a coordinate */
	base.x = GetReal8(gw,CID_X,_("X"),&err);
/* GT: Y is a coordinate */
	base.y = GetReal8(gw,CID_Y,_("Y"),&err);
	if ( err )
return( true );
	if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Exact)) )
	    ret = SelectPointsWithin(cv, &base, 0, NULL );
	else if ( GGadgetIsChecked(GWidgetGetControl(gw,CID_Fuzzy)) ) {
	    fuzz = GetReal8(gw,CID_Fuzz,_("Search Radius"),&err);
	    if ( err )
return( true );
	    ret = SelectPointsWithin(cv, &base, fuzz, NULL );
	} else {
	    bounds.x = GetReal8(gw,CID_Width,_("Width"),&err);
	    bounds.y = GetReal8(gw,CID_Height,_("Height"),&err);
	    if ( err )
return( true );
	    if ( bounds.x<0 ) {
		base.x += bounds.x;
		bounds.x = -bounds.x;
	    }
	    if ( bounds.y<0 ) {
		base.y += bounds.y;
		bounds.y = -bounds.y;
	    }
	    ret = SelectPointsWithin(cv, &base, 0, &bounds );
	}
	pa->done = ret;
    }
return( true );
}

static int SPA_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	struct selpt *pa = GDrawGetUserData(GGadgetGetWindow(g));
	SPA_DoCancel(pa);
    }
return( true );
}

static int SPA_Radius(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadgetSetChecked(GWidgetGetControl(gw,CID_Fuzzy),true);
    }
return( true );
}

static int SPA_Rect(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent ) {
	GWindow gw = GGadgetGetWindow(g);
	GGadgetSetChecked(GWidgetGetControl(gw,CID_Within),true);
    }
return( true );
}

static int spa_e_h(GWindow gw, GEvent *event) {
    if ( event->type==et_close ) {
	SPA_DoCancel( GDrawGetUserData(gw));
    } else if ( event->type == et_char ) {
return( false );
    }
return( true );
}

void CVSelectPointAt(CharView *cv) {
    GRect pos;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[18];
    GTextInfo label[18];
    static struct selpt pa;
    int k;
    int first = false;

    pa.done = false;
    pa.cv = cv;

    if ( pa.gw==NULL ) {
	memset(&wattrs,0,sizeof(wattrs));
	wattrs.mask = wam_events|wam_cursor|wam_utf8_wtitle|wam_undercursor|wam_isdlg|wam_restrict;
	wattrs.event_masks = ~(1<<et_charup);
	wattrs.restrict_input_to_me = 1;
	wattrs.undercursor = 1;
	wattrs.cursor = ct_pointer;
	wattrs.utf8_window_title = _("Select Point(s) at...");
	wattrs.is_dlg = true;
	pos.x = pos.y = 0;
	pos.width = GGadgetScale(GDrawPointsToPixels(NULL,190));
	pos.height = GDrawPointsToPixels(NULL,175);
	pa.gw = gw = GDrawCreateTopWindow(NULL,&pos,spa_e_h,&pa,&wattrs);

	memset(&label,0,sizeof(label));
	memset(&gcd,0,sizeof(gcd));

	k = 0;
	label[k].text = (unichar_t *) _("_X:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 10; gcd[k].gd.pos.y = 8+4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = 30; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 40;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_X;
	gcd[k++].creator = GTextFieldCreate;

	label[k].text = (unichar_t *) _("_Y:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 80; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = 100; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;  gcd[k].gd.pos.width = 40;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Y;
	gcd[k++].creator = GTextFieldCreate;

	label[k].text = (unichar_t *) _("_Exact");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+23;
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_cb_on;
	gcd[k].gd.cid = CID_Exact;
	gcd[k++].creator = GRadioCreate;

	label[k].text = (unichar_t *) _("_Around");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+13;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Fuzzy;
	gcd[k++].creator = GRadioCreate;

	label[k].text = (unichar_t *) _("W_ithin Rectangle");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+16+24;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Within;
	gcd[k++].creator = GRadioCreate;

	label[k].text = (unichar_t *) _("_Radius:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y+17+4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	label[k].text = (unichar_t *) _("3");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 52; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 40;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Fuzz;
	gcd[k].gd.handle_controlevent = SPA_Radius;
	gcd[k++].creator = GTextFieldCreate;

	label[k].text = (unichar_t *) _("_Width:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 15; gcd[k].gd.pos.y = gcd[k-3].gd.pos.y+17+4;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = 52; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 40;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Width;
	gcd[k].gd.handle_controlevent = SPA_Rect;
	gcd[k++].creator = GTextFieldCreate;

	label[k].text = (unichar_t *) _("_Height:");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.pos.x = 100; gcd[k].gd.pos.y = gcd[k-2].gd.pos.y;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLabelCreate;

	gcd[k].gd.pos.x = 138; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y-4;  gcd[k].gd.pos.width = 40;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k].gd.cid = CID_Height;
	gcd[k].gd.handle_controlevent = SPA_Rect;
	gcd[k++].creator = GTextFieldCreate;

	gcd[k].gd.pos.x = 5; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+28;
	gcd[k].gd.pos.width = GDrawPixelsToPoints(gw,pos.width)-10;
	gcd[k].gd.flags = gg_enabled|gg_visible;
	gcd[k++].creator = GLineCreate;

	gcd[k].gd.pos.x = 20-3; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+8;
	gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_default;
	label[k].text = (unichar_t *) _("_OK");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = SPA_OK;
	gcd[k++].creator = GButtonCreate;

	gcd[k].gd.pos.x = -20; gcd[k].gd.pos.y = gcd[k-1].gd.pos.y+3;
	gcd[k].gd.pos.width = -1; gcd[k].gd.pos.height = 0;
	gcd[k].gd.flags = gg_visible | gg_enabled | gg_but_cancel;
	label[k].text = (unichar_t *) _("_Cancel");
	label[k].text_is_1byte = true;
	label[k].text_in_resource = true;
	gcd[k].gd.label = &label[k];
	gcd[k].gd.handle_controlevent = SPA_Cancel;
	gcd[k++].creator = GButtonCreate;

	gcd[k].gd.pos.x = 2; gcd[k].gd.pos.y = 2;
	gcd[k].gd.pos.width = pos.width-4; gcd[k].gd.pos.height = pos.height-4;
	gcd[k].gd.flags = gg_enabled|gg_visible|gg_pos_in_pixels;
	gcd[k++].creator = GGroupCreate;

	GGadgetsCreate(gw,gcd);
	first = true;
    } else {
	GTextFieldSelect(GWidgetGetControl(pa.gw,CID_X),0,-1);
    }
    GWidgetIndicateFocusGadget(GWidgetGetControl(pa.gw,CID_X));
    GDrawSetVisible(pa.gw,true);
    GDrawProcessOneEvent(NULL);
    if ( first )
	GGadgetSetChecked(GWidgetGetControl(gw,CID_Exact),true);
    while ( !pa.done )
	GDrawProcessOneEvent(NULL);
    GDrawSetVisible(pa.gw,false);

}
