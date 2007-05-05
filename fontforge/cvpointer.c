/* Copyright (C) 2000-2007 by George Williams */
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
#include <utype.h>
#include <math.h>

extern int stop_at_join;

RefChar *HasUseMyMetrics(SplineChar *sc) {
    RefChar *r;

    for ( r=sc->layers[ly_fore].refs; r!=NULL; r=r->next )
	if ( r->use_my_metrics )
return( r );

return( NULL );
}

/* if they changed the width, then change the width on all bitmap chars of */
/*  ours, and if we are a letter, then change the width on all chars linked */
/*  to us which had the same width that we used to have (so if we change the */
/*  width of A, we'll also change that of À and Ä and ... */
void SCSynchronizeWidth(SplineChar *sc,real newwidth, real oldwidth, FontView *flagfv) {
    BDFFont *bdf;
    struct splinecharlist *dlist;
    FontView *fv = sc->parent->fv;
    RefChar *r = HasUseMyMetrics(sc);
    int isprobablybase;

    sc->widthset = true;
    if( r!=NULL ) {
	if ( oldwidth==r->sc->width ) {
	    sc->width = r->sc->width;
return;
	}
	newwidth = r->sc->width;
    }
    if ( newwidth==oldwidth )
return;
    sc->width = newwidth;
    for ( bdf=sc->parent->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	BDFChar *bc = bdf->glyphs[sc->orig_pos];
	if ( bc!=NULL ) {
	    int width = rint(sc->width*bdf->pixelsize / (real) (sc->parent->ascent+sc->parent->descent));
	    if ( bc->width!=width ) {
		/*BCPreserveWidth(bc);*/ /* Bitmaps can't set width, so no undo for it */
		bc->width = width;
		BCCharChangedUpdate(bc);
	    }
	}
    }

    if ( !adjustwidth )
return;

    isprobablybase = true;
    if ( sc->unicodeenc==-1 || sc->unicodeenc>=0x10000 ||
	    !isalpha(sc->unicodeenc) || iscombining(sc->unicodeenc))
	isprobablybase = false;

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	RefChar *metrics = HasUseMyMetrics(dlist->sc);
	if ( metrics!=NULL && metrics->sc!=sc )
    continue;
	else if ( metrics==NULL && !isprobablybase )
    continue;
	if ( dlist->sc->width==oldwidth &&
		(flagfv==NULL || !flagfv->selected[flagfv->map->backmap[dlist->sc->orig_pos]])) {
	    SCSynchronizeWidth(dlist->sc,newwidth,oldwidth,fv);
	    if ( !dlist->sc->changed ) {
		dlist->sc->changed = true;
		if ( fv!=NULL )
		    FVToggleCharChanged(dlist->sc);
	    }
	    SCUpdateAll(dlist->sc);
	}
    }
}

/* If they change the left bearing of a character, then in all chars */
/*  that depend on it should be adjusted too. */
/* Also all vstem hints */
/* I deliberately don't set undoes in the dependants. The change is not */
/*  in them, after all */
void SCSynchronizeLBearing(SplineChar *sc,real off) {
    struct splinecharlist *dlist;
    RefChar *ref;
    DStemInfo *d;
    StemInfo *h;
    HintInstance *hi;
    int isprobablybase;

    for ( h=sc->vstem; h !=NULL; h=h->next )
	h->start += off;
    for ( h=sc->hstem; h !=NULL; h=h->next )
	for ( hi = h->where; hi!=NULL; hi=hi->next ) {
	    hi->begin += off;
	    hi->end += off;
	}
    for ( d=sc->dstem; d !=NULL; d=d->next ) {
	d->leftedgetop.x += off;
	d->rightedgetop.x += off;
	d->leftedgebottom.x += off;
	d->rightedgebottom.x += off;
    }

    if ( !adjustlbearing )
return;

    isprobablybase = true;
    if ( sc->unicodeenc==-1 || sc->unicodeenc>=0x10000 ||
	    !isalpha(sc->unicodeenc) || iscombining(sc->unicodeenc))
	isprobablybase = false;

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	RefChar *metrics = HasUseMyMetrics(dlist->sc);
	if ( metrics!=NULL && metrics->sc!=sc )
    continue;
	else if ( metrics==NULL && !isprobablybase )
    continue;
	else if ( metrics==NULL && sc->width!=dlist->sc->width )
    continue;
	SCPreserveState(dlist->sc,false);
	SplinePointListShift(dlist->sc->layers[ly_fore].splines,off,true);
	for ( ref = dlist->sc->layers[ly_fore].refs; ref!=NULL; ref=ref->next )
		if ( ref->sc!=sc ) {
	    SplinePointListShift(ref->layers[0].splines,off,true);
	    ref->transform[4] += off;
	    ref->bb.minx += off; ref->bb.maxx += off;
	}
	SCUpdateAll(dlist->sc);
	SCSynchronizeLBearing(dlist->sc,off);
    }
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
int CVAnySel(CharView *cv, int *anyp, int *anyr, int *anyi, int *anya) {
    int anypoints = 0, anyrefs=0, anyimages=0, anyanchor=0;
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *il;
    AnchorPoint *ap;

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL && !anypoints; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) anypoints = true;
	for ( spline=spl->first->next; spline!=NULL && spline!=first && !anypoints; spline = spline->to->next ) {
	    if ( spline->to->selected ) anypoints = true;
	    if ( first == NULL ) first = spline;
	}
    }
    for ( rf=cv->layerheads[cv->drawmode]->refs; rf!=NULL && !anyrefs; rf=rf->next )
	if ( rf->selected ) anyrefs = true;
    if ( cv->drawmode==dm_fore ) {
	if ( cv->showanchor && anya!=NULL )
	    for ( ap=cv->sc->anchor; ap!=NULL && !anyanchor; ap=ap->next )
		if ( ap->selected ) anyanchor = true;
    }
    for ( il=cv->layerheads[cv->drawmode]->images; il!=NULL && !anyimages; il=il->next )
	if ( il->selected ) anyimages = true;
    if ( anyp!=NULL ) *anyp = anypoints;
    if ( anyr!=NULL ) *anyr = anyrefs;
    if ( anyi!=NULL ) *anyi = anyimages;
    if ( anya!=NULL ) *anya = anyanchor;
return( anypoints || anyrefs || anyimages || anyanchor );
}

SplinePoint *CVAnySelPoints(CharView *cv) {
    /* if there are any points selected */
    SplinePointList *spl;
    Spline *spline, *first;

    for ( spl= cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl=spl->next ) {
	if ( spl->first->selected );
return( spl->first );
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( spline->to->selected )
return( spline->to );
	    if ( first==NULL ) first = spline;
	}
    }
return( NULL );
}

int CVClearSel(CharView *cv) {
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;
    int needsupdate = 0;
    AnchorPoint *ap;

    cv->lastselpt = NULL;
    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	if ( spl->first->selected ) { needsupdate = true; spl->first->selected = false; }
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( spline->to->selected )
		{ needsupdate = true; spline->to->selected = false; }
	    if ( first==NULL ) first = spline;
	}
    }
    for ( rf=cv->layerheads[cv->drawmode]->refs; rf!=NULL; rf = rf->next )
	if ( rf->selected ) { needsupdate = true; rf->selected = false; }
    if ( cv->drawmode == dm_fore )
	for ( ap=cv->sc->anchor; ap!=NULL; ap = ap->next )
	    if ( ap->selected ) { if ( cv->showanchor ) needsupdate = true; ap->selected = false; }
    for ( img=cv->layerheads[cv->drawmode]->images; img!=NULL; img = img->next )
	if ( img->selected ) { needsupdate = true; img->selected = false; }
    if ( cv->p.nextcp || cv->p.prevcp || cv->widthsel || cv->vwidthsel )
	needsupdate = true;
    cv->p.nextcp = cv->p.prevcp = false;
    cv->widthsel = cv->vwidthsel = false;
return( needsupdate );
}

int CVSetSel(CharView *cv,int mask) {
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;
    int needsupdate = 0;
    AnchorPoint *ap;
    RefChar *usemymetrics = HasUseMyMetrics(cv->sc);

    cv->lastselpt = NULL;
    if ( mask&1 )
    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	if ( !spl->first->selected ) { needsupdate = true; spl->first->selected = true; }
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( !spline->to->selected )
		{ needsupdate = true; spline->to->selected = true; }
	    cv->lastselpt = spline->to;
	    if ( first==NULL ) first = spline;
	}
    }
    if ( mask&1 ) {
	for ( rf=cv->layerheads[cv->drawmode]->refs; rf!=NULL; rf = rf->next )
	    if ( !rf->selected ) { needsupdate = true; rf->selected = true; }
	for ( img=cv->layerheads[cv->drawmode]->images; img!=NULL; img = img->next )
	    if ( !img->selected ) { needsupdate = true; img->selected = true; }
    }
    if ( (mask&2) && cv->showanchor ) {
	for ( ap=cv->sc->anchor; ap!=NULL; ap=ap->next )
	    if ( !ap->selected ) { needsupdate = true; ap->selected = true; }
    }
    if ( cv->p.nextcp || cv->p.prevcp )
	needsupdate = true;
    cv->p.nextcp = cv->p.prevcp = false;
    if ( cv->showhmetrics && !cv->widthsel && (mask&4) && usemymetrics==NULL ) {
	cv->widthsel = needsupdate = true;
	cv->oldwidth = cv->sc->width;
    }
    if ( cv->showvmetrics && cv->sc->parent->hasvmetrics && !cv->vwidthsel && (mask&4) && usemymetrics==NULL ) {
	cv->vwidthsel = needsupdate = true;
	cv->oldvwidth = cv->sc->vwidth;
    }
return( needsupdate );
}

void CVInvertSel(CharView *cv) {
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;

    cv->lastselpt = NULL;

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
      	spl->first->selected = !spl->first->selected;
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    spline->to->selected = !spline->to->selected;
	    cv->lastselpt = spline->to;
	    if ( first==NULL ) first = spline;
	}
	/* in circular case, first point is toggled twice in above code	*/
	/* so fix it here						*/
	if ( spline==first && spline != NULL)
	    spl->first->selected = !spl->first->selected;

    }
    for ( rf=cv->layerheads[cv->drawmode]->refs; rf!=NULL; rf = rf->next )
        rf->selected = !rf->selected;
    for ( img=cv->layerheads[cv->drawmode]->images; img!=NULL; img = img->next )
        img->selected = !img->selected;
    cv->p.nextcp = cv->p.prevcp = false;
}

int CVAllSelected(CharView *cv) {
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
	if ( !spl->first->selected )
return( false );
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( !spline->to->selected )
return( false );
	    if ( first==NULL ) first = spline;
	}
    }
    for ( rf=cv->layerheads[cv->drawmode]->refs; rf!=NULL; rf = rf->next )
	if ( !rf->selected )
return( false );
    for ( img=cv->layerheads[cv->drawmode]->images; img!=NULL; img = img->next )
	if ( !img->selected )
return( false );
return( true );
}

static void SplineSetFindSelBounds(SplinePointList *spl, DBounds *bounds,
	int nosel) {
    SplinePoint *sp, *first;

    for ( ; spl!=NULL; spl = spl->next ) {
	first = NULL;
	for ( sp = spl->first; sp!=first; sp = sp->next->to ) {
	    if ( (nosel || sp->selected) &&
		    bounds->minx==0 && bounds->maxx==0 &&
		    bounds->miny==0 && bounds->maxy == 0 ) {
		bounds->minx = bounds->maxx = sp->me.x;
		bounds->miny = bounds->maxy = sp->me.y;
	    } else if ( nosel || sp->selected ) {
		if ( sp->me.x<bounds->minx ) bounds->minx = sp->me.x;
		if ( sp->me.x>bounds->maxx ) bounds->maxx = sp->me.x;
		if ( sp->me.y<bounds->miny ) bounds->miny = sp->me.y;
		if ( sp->me.y>bounds->maxy ) bounds->maxy = sp->me.y;
	    }
	    if ( first==NULL ) first = sp;
	    if ( sp->next==NULL )
	break;
	}
    }
}

void CVFindCenter(CharView *cv, BasePoint *bp, int nosel) {
    DBounds b;
    ImageList *img;

    b.minx = b.miny = b.maxx = b.maxy = 0;
    SplineSetFindSelBounds(cv->layerheads[cv->drawmode]->splines,&b,nosel);
    if ( cv->drawmode==dm_fore ) {
	RefChar *rf;
	for ( rf=cv->layerheads[cv->drawmode]->refs; rf!=NULL; rf=rf->next ) {
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
    for ( img=cv->layerheads[cv->drawmode]->images; img!=NULL; img=img->next ) {
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
	cursors[ee_nw] = cursors[ee_se] = ct_nwse; cursors[ee_ne] = cursors[ee_sw] = ct_nesw;
	cursors[ee_left] = cursors[ee_right] = ct_leftright;
	cursors[ee_up] = cursors[ee_down] = ct_updown;
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

void CVCheckResizeCursors(CharView *cv) {
    RefChar *ref;
    ImageList *img;
    int old_ee = cv->expandedge;
    real fudge = 3.5/cv->scale;

    cv->expandedge = ee_none;
    if ( cv->drawmode==dm_fore ) {
	for ( ref=cv->layerheads[cv->drawmode]->refs; ref!=NULL; ref=ref->next ) if ( ref->selected ) {
	    if (( cv->expandedge = OnBB(cv,&ref->bb,fudge))!=ee_none )
	break;
	}
	if ( cv->expandedge == ee_none ) {
	    RefChar *usemymetrics = HasUseMyMetrics(cv->sc);
	    if ( cv->showhmetrics && cv->info.x > cv->sc->width-fudge &&
		    cv->info.x<cv->sc->width+fudge && cv->searcher==NULL &&
		    usemymetrics==NULL )
		cv->expandedge = ee_right;
	    else if ( cv->showhmetrics && NearCaret(cv->sc,cv->info.x,fudge)!=-1 &&
		    usemymetrics==NULL )
		cv->expandedge = ee_right;
	    if ( cv->showvmetrics && cv->sc->parent->hasvmetrics && cv->searcher==NULL &&
		    cv->info.y > cv->sc->parent->vertical_origin-cv->sc->vwidth-fudge &&
		    cv->info.y < cv->sc->parent->vertical_origin-cv->sc->vwidth+fudge )
		cv->expandedge = ee_down;
	}
    }
    for ( img=cv->layerheads[cv->drawmode]->images; img!=NULL; img=img->next ) if ( img->selected ) {
	if (( cv->expandedge = OnBB(cv,&img->bb,fudge))!=ee_none )
    break;
    }
    if ( cv->expandedge!=old_ee )
	SetCur(cv);
}

static int ImgRefEdgeSelected(CharView *cv, FindSel *fs,GEvent *event) {
    RefChar *ref;
    ImageList *img;
    int update;

    cv->expandedge = ee_none;
    /* Check the bounding box of references if meta is up, or if they didn't */
    /*  click on a reference edge. Point being to allow people to select */
    /*  macron or other reference which fills the bounding box */
    if ( cv->drawmode==dm_fore && (!(event->u.mouse.state&ksm_alt) ||
	    (fs->p->ref!=NULL && !fs->p->ref->selected))) {
	for ( ref=cv->layerheads[cv->drawmode]->refs; ref!=NULL; ref=ref->next ) if ( ref->selected ) {
	    if (( cv->expandedge = OnBB(cv,&ref->bb,fs->fudge))!=ee_none ) {
		ref->selected = false;
		update = CVClearSel(cv);
		ref->selected = true;
		if ( update )
		    SCUpdateAll(cv->sc);
		CVPreserveTState(cv);
		cv->p.ref = ref;
		SetCur(cv);
return( true );
	    }
	}
    }
    for ( img=cv->layerheads[cv->drawmode]->images; img!=NULL; img=img->next ) if ( img->selected ) {
	if (( cv->expandedge = OnBB(cv,&img->bb,fs->fudge))!=ee_none ) {
	    img->selected = false;
	    update = CVClearSel(cv);
	    img->selected = true;
	    if ( update )
		SCUpdateAll(cv->sc);
	    CVPreserveTState(cv);
	    cv->p.img = img;
	    SetCur(cv);
return( true );
	}
    }
return( false );
}

void CVMouseDownPointer(CharView *cv, FindSel *fs, GEvent *event) {
    int needsupdate = false;
    int dowidth, dovwidth, nearcaret;
    RefChar *usemymetrics = HasUseMyMetrics(cv->sc);

    if ( cv->pressed==NULL )
	cv->pressed = GDrawRequestTimer(cv->v,200,100,NULL);
    cv->last_c.x = cv->info.x; cv->last_c.y = cv->info.y;
    /* don't clear the selection if the things we clicked on were already */
    /*  selected, or if the user held the shift key down */
    if ( ImgRefEdgeSelected(cv,fs,event))
return;
    dowidth = ( cv->showhmetrics && cv->p.cx>cv->sc->width-fs->fudge &&
		cv->p.cx<cv->sc->width+fs->fudge && cv->searcher==NULL &&
		usemymetrics==NULL );
    dovwidth = ( cv->showvmetrics && cv->sc->parent->hasvmetrics && cv->searcher == NULL &&
		cv->p.cy>cv->sc->parent->vertical_origin-cv->sc->vwidth-fs->fudge &&
		cv->p.cy<cv->sc->parent->vertical_origin-cv->sc->vwidth+fs->fudge &&
		usemymetrics==NULL );
    cv->nearcaret = nearcaret = -1;
    if ( cv->showhmetrics ) nearcaret = NearCaret(cv->sc,cv->p.cx,fs->fudge);
    if ( (fs->p->sp==NULL || !fs->p->sp->selected) &&
	    (fs->p->ref==NULL || !fs->p->ref->selected) &&
	    (fs->p->img==NULL || !fs->p->img->selected) &&
	    (fs->p->ap==NULL || !fs->p->ap->selected) &&
	    (!dowidth || !cv->widthsel) &&
	    (!dovwidth || !cv->vwidthsel) &&
	    !(event->u.mouse.state&ksm_shift))
	needsupdate = CVClearSel(cv);
    if ( !fs->p->anysel ) {
	/* Nothing else... unless they clicked on the width line, check that */
	if ( dowidth ) {
	    if ( event->u.mouse.state&ksm_shift )
		cv->widthsel = !cv->widthsel;
	    else
		cv->widthsel = true;
	    if ( cv->widthsel ) {
		cv->oldwidth = cv->sc->width;
		fs->p->cx = cv->sc->width;
		CVInfoDraw(cv,cv->gw);
		fs->p->anysel = true;
		cv->expandedge = ee_right;
	    } else
		cv->expandedge = ee_none;
	    SetCur(cv);
	    needsupdate = true;
	} else if ( dovwidth ) {
	    if ( event->u.mouse.state&ksm_shift )
		cv->vwidthsel = !cv->vwidthsel;
	    else
		cv->vwidthsel = true;
	    if ( cv->vwidthsel ) {
		cv->oldvwidth = cv->sc->vwidth;
		fs->p->cy = cv->sc->parent->vertical_origin-cv->sc->vwidth;
		CVInfoDraw(cv,cv->gw);
		fs->p->anysel = true;
		cv->expandedge = ee_down;
	    } else
		cv->expandedge = ee_none;
	    SetCur(cv);
	    needsupdate = true;
	} else if ( nearcaret!=-1 ) {
	    PST *pst;
	    for ( pst=cv->sc->possub; pst!=NULL && pst->type!=pst_lcaret; pst=pst->next );
	    cv->lcarets = pst;
	    cv->nearcaret = nearcaret;
	    cv->expandedge = ee_right;
	    SetCur(cv);
	}
    } else if ( event->u.mouse.clicks<=1 && !(event->u.mouse.state&ksm_shift)) {
	if ( fs->p->nextcp || fs->p->prevcp )
	    CPStartInfo(cv,event);
	else if ( fs->p->sp!=NULL ) {
	    if ( !fs->p->sp->selected ) needsupdate = true;
	    fs->p->sp->selected = true;
	} else if ( fs->p->spline!=NULL ) {
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
    } else if ( event->u.mouse.clicks<=1 ) {
	if ( fs->p->nextcp || fs->p->prevcp )
	    /* Nothing to do */;
	else if ( fs->p->sp!=NULL ) {
	    needsupdate = true;
	    fs->p->sp->selected = !fs->p->sp->selected;
	} else if ( fs->p->spline!=NULL ) {
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
    } else if ( event->u.mouse.clicks==2 ) {
	CPEndInfo(cv);
	if ( fs->p->spl!=NULL ) {
	    Spline *spline, *first;
	    if ( !fs->p->spl->first->selected ) { needsupdate = true; fs->p->spl->first->selected = true; }
	    first = NULL;
	    for ( spline = fs->p->spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( !spline->to->selected )
		    { needsupdate = true; spline->to->selected = true; }
		if ( first==NULL ) first = spline;
	    }
	} else if ( fs->p->ref!=NULL || fs->p->img!=NULL ) {
	    /* Double clicking on a referenced character doesn't do much */
	} else if ( fs->p->ap!=NULL ) {
	    /* Select all Anchor Points at this location */
	    AnchorPoint *ap;
	    for ( ap=cv->sc->anchor; ap!=NULL; ap=ap->next )
		if ( ap->me.x==fs->p->ap->me.x && ap->me.y==fs->p->ap->me.y )
		    if ( !ap->selected ) {
			ap->selected = true;
			needsupdate = true;
		    }
	}
    } else if ( event->u.mouse.clicks==3 ) {
	if ( CVSetSel(cv,1)) needsupdate = true;
		/* don't select width or anchor points for three clicks */
		/*  but select all points, refs */
    } else {
	/* Select everything */
	if ( CVSetSel(cv,-1)) needsupdate = true;
    }
    if ( needsupdate )
	SCUpdateAll(cv->sc);
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

    if ( cv->drawmode==dm_fore ) {
	for ( rf = cv->layerheads[cv->drawmode]->refs; rf!=NULL; rf=rf->next ) {
	    if (( rf->bb.minx>=old.minx && rf->bb.maxx<old.maxx &&
			rf->bb.miny>=old.miny && rf->bb.maxy<old.maxy ) !=
		    ( rf->bb.minx>=new.minx && rf->bb.maxx<new.maxx &&
			rf->bb.miny>=new.miny && rf->bb.maxy<new.maxy )) {
		rf->selected = !rf->selected;
		any = true;
	    }
	}
	if ( cv->showanchor ) for ( ap=cv->sc->anchor ; ap!=NULL; ap=ap->next ) {
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

    for ( img = cv->layerheads[cv->drawmode]->images; img!=NULL; img=img->next ) {
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

    for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL; spl = spl->next ) {
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
    }
return( any );
}

void CVAdjustControl(CharView *cv,BasePoint *cp, BasePoint *to) {
    SplinePoint *sp = cv->p.sp;
    BasePoint *othercp = cp==&sp->nextcp?&sp->prevcp:&sp->nextcp;
    int refig = false;

    if ( sp->ttfindex==0xffff && cv->fv->sf->order2 ) {
	/* If the point itself is implied, then it's the control points that */
	/*  are fixed. Moving a CP should move the implied point so that it */
	/*  continues to be in the right place */
	sp->me.x = (to->x+othercp->x)/2;
	sp->me.y = (to->y+othercp->y)/2;
	*cp = *to;
	refig = true;
    } else if ( sp->pointtype==pt_corner ) {
	cp->x = to->x;
	cp->y = to->y;
    } else if ( sp->pointtype==pt_curve ) {
	cp->x = to->x;
	cp->y = to->y;
	if (( cp->x!=sp->me.x || cp->y!=sp->me.y ) && !cv->sc->parent->order2 ) {
	    double len1, len2;
	    len1 = sqrt((cp->x-sp->me.x)*(cp->x-sp->me.x) +
			(cp->y-sp->me.y)*(cp->y-sp->me.y));
	    len2 = sqrt((othercp->x-sp->me.x)*(othercp->x-sp->me.x) +
			(othercp->y-sp->me.y)*(othercp->y-sp->me.y));
	    len2 /= len1;
	    othercp->x = len2 * (sp->me.x-cp->x) + sp->me.x;
	    othercp->y = len2 * (sp->me.y-cp->y) + sp->me.y;
	    if ( sp->next!=NULL && othercp==&sp->nextcp )
		SplineRefigure3(sp->next);
	    if ( sp->prev!=NULL && othercp==&sp->prevcp )
		SplineRefigure3(sp->prev);
	} 
	if ( cp==&sp->nextcp ) sp->prevcpdef = false;
	else sp->nextcpdef = false;
    } else {
	BasePoint *bp;
	if ( cp==&sp->prevcp && sp->next!=NULL )
	    bp = &sp->next->to->me;
	else if ( cp==&sp->nextcp && sp->prev!=NULL )
	    bp = &sp->prev->from->me;
	else
	    bp = NULL;
	if ( bp!=NULL ) {
	    real angle = atan2(bp->y-sp->me.y,bp->x-sp->me.x);
	    real len = sqrt((bp->x-sp->me.x)*(bp->x-sp->me.x) + (bp->y-sp->me.y)*(bp->y-sp->me.y));
	    real dotprod =
		    ((to->x-sp->me.x)*(bp->x-sp->me.x) +
		     (to->y-sp->me.y)*(bp->y-sp->me.y));
	    if ( len!=0 ) {
		dotprod /= len;
		if ( dotprod>0 ) dotprod = 0;
		cp->x = sp->me.x + dotprod*cos(angle);
		cp->y = sp->me.y + dotprod*sin(angle);
	    }
	}
    }

    if ( cv->fv->sf->order2 ) {
	if ( cp==&sp->nextcp && sp->next!=NULL ) {
	    SplinePoint *osp = sp->next->to;
	    if ( osp->ttfindex==0xffff ) {
		osp->prevcp = *to;
		osp->me.x = (to->x+osp->nextcp.x)/2;
		osp->me.y = (to->y+osp->nextcp.y)/2;
		SplineRefigure(osp->next);
	    }
	} else if ( cp==&sp->prevcp && sp->prev!=NULL ) {
	    SplinePoint *osp = sp->prev->from;
	    if ( osp->ttfindex==0xffff ) {
		osp->nextcp = *to;
		osp->me.x = (to->x+osp->prevcp.x)/2;
		osp->me.y = (to->y+osp->prevcp.y)/2;
		SplineRefigure(osp->prev);
	    }
	}
    }

    if ( cp->x==sp->me.x && cp->y==sp->me.y ) {
	if ( cp==&sp->nextcp ) sp->nonextcp = true;
	else sp->noprevcp = true;
    }  else {
	if ( cp==&sp->nextcp ) sp->nonextcp = false;
	else sp->noprevcp = false;
    }
    if ( cp==&sp->nextcp ) sp->nextcpdef = false;
    else sp->prevcpdef = false;

    if ( sp->next!=NULL && cp==&sp->nextcp ) {
	if ( sp->next->order2 && !sp->nonextcp ) {
	    sp->next->to->prevcp = *cp;
	    sp->next->to->noprevcp = false;
	}
	SplineRefigureFixup(sp->next);
    }
    if ( sp->prev!=NULL && cp==&sp->prevcp ) {
	if ( sp->prev->order2 && !sp->noprevcp ) {
	    sp->prev->from->nextcp = *cp;
	    sp->prev->from->nonextcp = false;
	}
	SplineRefigureFixup(sp->prev);
    }
    if ( refig ) {
	SplineRefigure(sp->prev);
	SplineRefigure(sp->next);
    }
    CVSetCharChanged(cv,true);
}

static void CVAdjustSpline(CharView *cv) {
    Spline *old = cv->p.spline;
    TPoint tp[5];
    real t;
    Spline1D *oldx = &old->splines[0], *oldy = &old->splines[1];

    if ( cv->sc->parent->order2 )
return;

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
    old->from->pointtype = pt_corner; old->to->pointtype = pt_corner;
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
static int CVCheckMerges(CharView *cv ) {
    SplineSet *activess, *mergess;
    real fudge = 1/cv->scale;
    int cnt= -1;

  restart:
    ++cnt;
    for ( activess=cv->layerheads[cv->drawmode]->splines; activess!=NULL; activess=activess->next ) {
	if ( activess->first->prev==NULL && (activess->first->selected ||
		activess->last->selected)) {
	    for ( mergess = cv->layerheads[cv->drawmode]->splines; mergess!=NULL; mergess=mergess->next ) {
		if ( mergess->first->prev==NULL && (!mergess->first->selected ||
			!mergess->last->selected)) {
		    if ( !mergess->first->selected && activess->first->selected &&
			    Nearish(mergess->first->me.x-activess->first->me.x,fudge) &&
			    Nearish(mergess->first->me.y-activess->first->me.y,fudge)) {
			CVMergeSplineSets(cv,activess->first,activess,
				mergess->first,mergess);
  goto restart;
		    }
		    if ( !mergess->last->selected && activess->first->selected &&
			    Nearish(mergess->last->me.x-activess->first->me.x,fudge) &&
			    Nearish(mergess->last->me.y-activess->first->me.y,fudge)) {
			CVMergeSplineSets(cv,activess->first,activess,
				mergess->last,mergess);
  goto restart;
		    }
		    if ( !mergess->first->selected && activess->last->selected &&
			    Nearish(mergess->first->me.x-activess->last->me.x,fudge) &&
			    Nearish(mergess->first->me.y-activess->last->me.y,fudge)) {
			CVMergeSplineSets(cv,activess->last,activess,
				mergess->first,mergess);
  goto restart;
		    }
		    if ( !mergess->last->selected && activess->last->selected &&
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

/* Move the selection and return whether we did a merge */
int CVMoveSelection(CharView *cv, real dx, real dy, uint32 input_state) {
    real transform[6];
    RefChar *refs;
    ImageList *img;
    AnchorPoint *ap;
    double fudge;
    extern float snapdistance;
    int j;
    int changed = false, outlinechanged = false;
    SplinePointList *spl;
    SplinePoint *sp;

    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = 0.0;
    transform[4] = dx; transform[5] = dy;
    if ( transform[4]==0 && transform[5]==0 )
return(false);
    for ( spl=cv->layerheads[cv->drawmode]->splines; spl!=NULL && !outlinechanged; spl=spl->next ) {
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

    SplinePointListTransform(cv->layerheads[cv->drawmode]->splines,transform,false);

    for ( refs = cv->layerheads[cv->drawmode]->refs; refs!=NULL; refs=refs->next ) if ( refs->selected ) {
	refs->transform[4] += transform[4];
	refs->transform[5] += transform[5];
	refs->bb.minx += transform[4]; refs->bb.maxx += transform[4];
	refs->bb.miny += transform[5]; refs->bb.maxy += transform[5];
	for ( j=0; j<refs->layer_cnt; ++j )
	    SplinePointListTransform(refs->layers[j].splines,transform,true);
	outlinechanged = true;
    }
    if ( cv->drawmode==dm_fore ) {
	if ( cv->showanchor ) {
	    for ( ap=cv->sc->anchor; ap!=NULL; ap=ap->next ) if ( ap->selected ) {
		ap->me.x += transform[4];
		ap->me.y += transform[5];
		changed = true;
	    }
	}
    }
    for ( img = cv->layerheads[cv->drawmode]->images; img!=NULL; img=img->next ) if ( img->selected ) {
	img->xoff += transform[4];
	img->yoff += transform[5];
	img->bb.minx += transform[4]; img->bb.maxx += transform[4];
	img->bb.miny += transform[5]; img->bb.maxy += transform[5];
	SCOutOfDateBackground(cv->sc);
	changed = true;
    }
    fudge = snapdistance/cv->scale/2;
    if ( cv->widthsel ) {
	if ( cv->sc->width+dx>0 && ((int16) (cv->sc->width+dx))<0 )
	    cv->sc->width = 32767;
	else if ( cv->sc->width+dx<0 && ((int16) (cv->sc->width+dx))>0 )
	    cv->sc->width = -32768;
	else
	    cv->sc->width += dx;
	if ( cv->sc->width>=-fudge && cv->sc->width<fudge )
	    cv->sc->width = 0;
	changed = true;
    }
    if ( cv->vwidthsel ) {
	if ( cv->sc->vwidth-dy>0 && ((int16) (cv->sc->vwidth-dy))<0 )
	    cv->sc->vwidth = 32767;
	else if ( cv->sc->vwidth-dy<0 && ((int16) (cv->sc->vwidth-dy))>0 )
	    cv->sc->vwidth = -32768;
	else
	    cv->sc->vwidth -= dy;
	if ( cv->sc->vwidth>=-fudge && cv->sc->vwidth<fudge )
	    cv->sc->vwidth = 0;
	changed = true;
    }
    if ( outlinechanged )
	CVSetCharChanged(cv,true);
    else if ( changed )
	CVSetCharChanged(cv,2);
    if ( input_state&ksm_alt )
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

int CVMouseMovePointer(CharView *cv, GEvent *event) {
    int needsupdate = false;
    int did_a_merge = false;

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
    if ( cv->expandedge!=ee_none && !cv->widthsel && !cv->vwidthsel && cv->nearcaret==-1 )
	needsupdate = CVExpandEdge(cv);
    else if ( cv->nearcaret!=-1 && cv->lcarets!=NULL ) {
	if ( cv->info.x!=cv->last_c.x ) {
	    if ( !cv->recentchange ) SCPreserveState(cv->sc,2);
	    cv->lcarets->u.lcaret.carets[cv->nearcaret] += cv->info.x-cv->last_c.x;
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
	cv->p.ex = cv->info.x;
	cv->p.ey = cv->info.y;
	cv->p.rubberbanding = true;
	if ( !needsupdate )
	    CVDrawRubberRect(cv->v,cv);
    } else if ( cv->p.nextcp ) {
	if ( !cv->recentchange ) CVPreserveState(cv);
	CVAdjustControl(cv,&cv->p.sp->nextcp,&cv->info);
	CPUpdateInfo(cv,event);
	needsupdate = true;
    } else if ( cv->p.prevcp ) {
	if ( !cv->recentchange ) CVPreserveState(cv);
	CVAdjustControl(cv,&cv->p.sp->prevcp,&cv->info);
	CPUpdateInfo(cv,event);
	needsupdate = true;
    } else if ( cv->p.spline!=NULL ) {
	if ( !cv->recentchange ) CVPreserveState(cv);
	CVAdjustSpline(cv);
	CVSetCharChanged(cv,true);
	needsupdate = true;
    } else {
	if ( !cv->recentchange ) CVPreserveState(cv);
	did_a_merge = CVMoveSelection(cv,
		cv->info.x-cv->last_c.x,cv->info.y-cv->last_c.y,
		event->u.mouse.state);
	needsupdate = true;
    }
    if ( needsupdate )
	SCUpdateAll(cv->sc);
    cv->last_c.x = cv->info.x; cv->last_c.y = cv->info.y;
return( did_a_merge );
}

void CVMouseUpPointer(CharView *cv ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
    static char *buts[3];
    buts[0] = _("_Yes");
    buts[1] = _("_No");
    buts[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
    static char *buts[] = { GTK_STOCK_YES, GTK_STOCK_NO, NULL };
#endif

    if ( cv->widthsel ) {
	if ( cv->sc->width<0 && cv->oldwidth>=0 ) {
	    if ( gwwv_ask(_("Negative Width"), (const char **) buts, 0, 1, _("Negative character widths are not allowed in TrueType.\nDo you really want a negative width?") )==1 )
		cv->sc->width = cv->oldwidth;
	}
	SCSynchronizeWidth(cv->sc,cv->sc->width,cv->oldwidth,NULL);
	cv->expandedge = ee_none;
	GDrawSetCursor(cv->v,ct_mypointer);
    }
    if ( cv->vwidthsel ) {
	if ( cv->sc->vwidth<0 && cv->oldvwidth>=0 ) {
	    if ( gwwv_ask(_("Negative Width"), (const char **) buts, 0, 1, _("Negative character widths are not allowed in TrueType.\nDo you really want a negative width?") )==1 )
		cv->sc->vwidth = cv->oldvwidth;
	}
	cv->expandedge = ee_none;
	GDrawSetCursor(cv->v,ct_mypointer);
    }
    if ( cv->nearcaret!=-1 && cv->lcarets!=NULL ) {
	cv->nearcaret = -1;
	cv->expandedge = ee_none;
	cv->lcarets = NULL;
	GDrawSetCursor(cv->v,ct_mypointer);
    }
    if ( cv->expandedge!=ee_none ) {
	CVUndoCleanup(cv);
	cv->expandedge = ee_none;
	GDrawSetCursor(cv->v,ct_mypointer);
    } else if ( CVAllSelected(cv) && cv->drawmode==dm_fore && cv->p.spline==NULL &&
	    !cv->p.prevcp && !cv->p.nextcp && cv->info.y==cv->p.cy ) {
	SCUndoSetLBearingChange(cv->sc,(int) rint(cv->info.x-cv->p.cx));
	SCSynchronizeLBearing(cv->sc,cv->info.x-cv->p.cx);
    }
    CPEndInfo(cv);
}

/* ************************************************************************** */
/*  ************************** Select Point Dialog *************************  */
/* ************************************************************************** */

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

    CVClearSel(cv);
    for ( ss= cv->layerheads[cv->drawmode]->splines; ss!=NULL; ss=ss->next ) {
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
    SCUpdateAll(cv->sc);
return( any!=NULL );
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
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
