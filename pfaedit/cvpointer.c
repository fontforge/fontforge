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
#include <utype.h>

/* if they changed the width, then change the width on all bitmap chars of */
/*  ours, and if we are a letter, then change the width on all chars linked */
/*  to us which had the same width that we used to have (so if we change the */
/*  width of A, we'll also change that of Á and Â and ... */
void SCSynchronizeWidth(SplineChar *sc,real newwidth, real oldwidth, FontView *fv) {
    BDFFont *bdf;
    struct splinecharlist *dlist;

    sc->widthset = true;
    if ( newwidth==oldwidth )
return;
    sc->width = newwidth;
    if ( sc->views!=NULL )
	fv = sc->parent->fv;
    for ( bdf=sc->parent->bitmaps; bdf!=NULL; bdf=bdf->next ) {
	BDFChar *bc = bdf->chars[sc->enc];
	if ( bc!=NULL ) {
	    int width = rint(sc->width*bdf->pixelsize / (real) (sc->parent->ascent+sc->parent->descent));
	    if ( bc->width!=width ) {
		/*BCPreserveWidth(bc);*/ /* Bitmaps can't set width, so no undo for it */
		bc->width = width;
		BCCharChangedUpdate(bc,fv);
	    }
	}
    }

    if ( !adjustwidth )
return;

    if ( sc->unicodeenc!=-1 && isalpha(sc->unicodeenc)) {
	for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	    if ( dlist->sc->width==oldwidth && !fv->selected[dlist->sc->enc]) {
		SCSynchronizeWidth(dlist->sc,newwidth,oldwidth,fv);
		if ( !dlist->sc->changed ) {
		    dlist->sc->changed = true;
		    if ( fv!=NULL )
			FVToggleCharChanged(fv,dlist->sc);
		}
		SCUpdateAll(dlist->sc);
	    }
	}
    }
}

/* If they change the left bearing of a character, then in all chars */
/*  that depend on it should be adjusted too. */
/* Also all vstem hints */
/* I deliberately don't set undoes in the dependants. The change is not */
/*  in them, after all */
void SCSynchronizeLBearing(SplineChar *sc,char *selected,real off) {
    struct splinecharlist *dlist;
    RefChar *ref;
    DStemInfo *d;
    StemInfo *h;
    HintInstance *hi;

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

    for ( dlist=sc->dependents; dlist!=NULL; dlist=dlist->next ) {
	if ( selected==NULL || !selected[dlist->sc->enc] ) {
	    SCPreserveState(dlist->sc,false);
	    SplinePointListShift(dlist->sc->splines,off,true);
	    for ( ref = dlist->sc->refs; ref!=NULL; ref=ref->next )
		    if ( ref->sc!=sc && (selected==NULL || !selected[ref->sc->enc] )) {
		SplinePointListShift(ref->splines,off,true);
		ref->transform[4] += off;
		ref->bb.minx += off; ref->bb.maxx += off;
	    }
	    SCUpdateAll(dlist->sc);
	    SCSynchronizeLBearing(dlist->sc,selected,off);
	}
    }
}

int CVAnySel(CharView *cv, int *anyp, int *anyr, int *anyi) {
    int anypoints = 0, anyrefs=0, anyimages=0;
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *il;

    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL && !anypoints; spl = spl->next ) {
	first = NULL;
	if ( spl->first->selected ) anypoints = true;
	for ( spline=spl->first->next; spline!=NULL && spline!=first && !anypoints; spline = spline->to->next ) {
	    if ( spline->to->selected ) anypoints = true;
	    if ( first == NULL ) first = spline;
	}
    }
    if ( cv->drawmode==dm_fore ) {
	for ( rf=cv->sc->refs; rf!=NULL && !anyrefs; rf=rf->next )
	    if ( rf->selected ) anyrefs = true;
    }
    if ( cv->drawmode==dm_back ) {
	for ( il=cv->sc->backimages; il!=NULL && !anyimages; il=il->next )
	    if ( il->selected ) anyimages = true;
    }
    if ( anyp!=NULL ) *anyp = anypoints;
    if ( anyr!=NULL ) *anyr = anyrefs;
    if ( anyi!=NULL ) *anyi = anyimages;
return( anypoints || anyrefs || anyimages );
}

int CVClearSel(CharView *cv) {
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;
    int needsupdate = 0;

    cv->lastselpt = NULL;
    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL; spl = spl->next ) {
	if ( spl->first->selected ) { needsupdate = true; spl->first->selected = false; }
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( spline->to->selected )
		{ needsupdate = true; spline->to->selected = false; }
	    if ( first==NULL ) first = spline;
	}
    }
    if ( cv->drawmode == dm_fore ) {
	for ( rf=cv->sc->refs; rf!=NULL; rf = rf->next )
	    if ( rf->selected ) { needsupdate = true; rf->selected = false; }
    }
    if ( cv->drawmode == dm_back ) {
	for ( img=cv->sc->backimages; img!=NULL; img = img->next )
	    if ( img->selected ) { needsupdate = true; img->selected = false; }
    }
return( needsupdate );
}

int CVSetSel(CharView *cv) {
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;
    int needsupdate = 0;

    cv->lastselpt = NULL;
    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL; spl = spl->next ) {
	if ( !spl->first->selected ) { needsupdate = true; spl->first->selected = true; }
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( !spline->to->selected )
		{ needsupdate = true; spline->to->selected = true; }
	    cv->lastselpt = spline->to;
	    if ( first==NULL ) first = spline;
	}
    }
    if ( cv->drawmode == dm_fore ) {
	for ( rf=cv->sc->refs; rf!=NULL; rf = rf->next )
	    if ( !rf->selected ) { needsupdate = true; rf->selected = true; }
    }
    if ( cv->drawmode == dm_back ) {
	for ( img=cv->sc->backimages; img!=NULL; img = img->next )
	    if ( !img->selected ) { needsupdate = true; img->selected = true; }
    }
return( needsupdate );
}

int CVAllSelected(CharView *cv) {
    SplinePointList *spl;
    Spline *spline, *first;
    RefChar *rf;
    ImageList *img;

    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL; spl = spl->next ) {
	if ( !spl->first->selected )
return( false );
	first = NULL;
	for ( spline = spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
	    if ( !spline->to->selected )
return( false );
	    if ( first==NULL ) first = spline;
	}
    }
    if ( cv->drawmode == dm_fore ) {
	for ( rf=cv->sc->refs; rf!=NULL; rf = rf->next )
	    if ( !rf->selected )
return( false );
    }
    if ( cv->drawmode == dm_back ) {
	for ( img=cv->sc->backimages; img!=NULL; img = img->next )
	    if ( !img->selected )
return( false );
    }
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

    b.minx = b.miny = b.maxx = b.maxy = 0;
    SplineSetFindSelBounds(*cv->heads[cv->drawmode],&b,nosel);
    if ( cv->drawmode==dm_fore ) {
	RefChar *rf;
	for ( rf=cv->sc->refs; rf!=NULL; rf=rf->next ) {
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
    if ( cv->drawmode==dm_fore ) {
	ImageList *img;
	for ( img=cv->sc->backimages; img!=NULL; img=img->next ) {
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

void CVCheckResizeCursors(CharView *cv) {
    RefChar *ref;
    ImageList *img;
    int old_ee = cv->expandedge;
    real fudge = 3.5/cv->scale;

    cv->expandedge = ee_none;
    if ( cv->drawmode==dm_fore ) {
	for ( ref=cv->sc->refs; ref!=NULL; ref=ref->next ) if ( ref->selected ) {
	    if (( cv->expandedge = OnBB(cv,&ref->bb,fudge))!=ee_none )
	break;
	}
	if ( cv->expandedge == ee_none )
	    if ( cv->info.x > cv->sc->width-fudge && cv->info.x<cv->sc->width+fudge )
		cv->expandedge = ee_right;
    }
    if ( cv->drawmode==dm_back ) {
	for ( img=cv->sc->backimages; img!=NULL; img=img->next ) if ( img->selected ) {
	    if (( cv->expandedge = OnBB(cv,&img->bb,fudge))!=ee_none )
    break;
	}
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
    if ( cv->drawmode==dm_fore && (!(event->u.chr.state&ksm_meta) ||
	    (fs->p->ref!=NULL && !fs->p->ref->selected))) {
	for ( ref=cv->sc->refs; ref!=NULL; ref=ref->next ) if ( ref->selected ) {
	    if (( cv->expandedge = OnBB(cv,&ref->bb,fs->fudge))!=ee_none ) {
		ref->selected = false;
		update = CVClearSel(cv);
		ref->selected = true;
		if ( update )
		    SCUpdateAll(cv->sc);
		CVPreserveTState(cv);
		SetCur(cv);
return( true );
	    }
	}
    }
    if ( cv->drawmode==dm_back ) {
	for ( img=cv->sc->backimages; img!=NULL; img=img->next ) if ( img->selected ) {
	    if (( cv->expandedge = OnBB(cv,&img->bb,fs->fudge))!=ee_none ) {
		img->selected = false;
		update = CVClearSel(cv);
		img->selected = true;
		if ( update )
		    SCUpdateAll(cv->sc);
		CVPreserveTState(cv);
		SetCur(cv);
return( true );
	    }
	}
    }
return( false );
}

void CVMouseDownPointer(CharView *cv, FindSel *fs, GEvent *event) {
    int needsupdate = false;

    if ( cv->pressed==NULL )
	cv->pressed = GDrawRequestTimer(cv->v,200,100,NULL);
    cv->last_c.x = cv->info.x; cv->last_c.y = cv->info.y;
    /* don't clear the selection if the things we clicked on were already */
    /*  selected, or if the user held the shift key down */
    if ( ImgRefEdgeSelected(cv,fs,event))
return;
    if ( (fs->p->sp==NULL || !fs->p->sp->selected) &&
	    (fs->p->ref==NULL || !fs->p->ref->selected) &&
	    (fs->p->img==NULL || !fs->p->img->selected) &&
	    !(event->u.mouse.state&ksm_shift))
	needsupdate = CVClearSel(cv);
    if ( !fs->p->anysel ) {
	/* Nothing else... unless they clicked on the width line, check that */
	if ( cv->p.cx>cv->sc->width-fs->fudge && cv->p.cx<cv->sc->width+fs->fudge ) {
	    fs->p->width = true;
	    fs->p->cx = cv->sc->width;
	    CVInfoDraw(cv,cv->gw);
	    fs->p->anysel = true;
	    cv->expandedge = ee_right;
	    SetCur(cv);
	}
    } else if ( event->u.mouse.clicks<=1 && !(event->u.mouse.state&ksm_shift)) {
	if ( fs->p->nextcp || fs->p->prevcp )
	    /* Nothing to do */;
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
	} else {
	    if ( !fs->p->ref->selected ) needsupdate = true;
	    fs->p->ref->selected = true;
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
	} else {
	    needsupdate = true;
	    fs->p->ref->selected = !fs->p->ref->selected;
	}
    } else if ( event->u.mouse.clicks==2 ) {
	if ( fs->p->spl!=NULL ) {
	    Spline *spline, *first;
	    if ( !fs->p->spl->first->selected ) { needsupdate = true; fs->p->spl->first->selected = true; }
	    first = NULL;
	    for ( spline = fs->p->spl->first->next; spline!=NULL && spline!=first; spline=spline->to->next ) {
		if ( !spline->to->selected )
		    { needsupdate = true; spline->to->selected = true; }
		if ( first==NULL ) first = spline;
	    }
	} else {
	    /* Double clicking on a referenced character doesn't do much */
	}
    } else {
	/* should triple clicking select all? */
	if ( CVSetSel(cv)) needsupdate = true;
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
	for ( rf = cv->sc->refs; rf!=NULL; rf=rf->next ) {
	    if (( rf->bb.minx>=old.minx && rf->bb.maxx<old.maxx &&
			rf->bb.miny>=old.miny && rf->bb.maxy<old.maxy ) !=
		    ( rf->bb.minx>=new.minx && rf->bb.maxx<new.maxx &&
			rf->bb.miny>=new.miny && rf->bb.maxy<new.maxy )) {
		rf->selected = !rf->selected;
		any = true;
	    }
	}
    }

    if ( cv->drawmode==dm_back ) {
	DBounds bb;
	for ( img = cv->sc->backimages; img!=NULL; img=img->next ) {
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
    }

    for ( spl = *cv->heads[cv->drawmode]; spl!=NULL; spl = spl->next ) {
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

static void CVAdjustControl(CharView *cv,BasePoint *cp) {
    SplinePoint *sp = cv->p.sp;
    BasePoint *othercp = cp==&sp->nextcp?&sp->prevcp:&sp->nextcp;

    if ( sp->pointtype==pt_corner ) {
	cp->x = cv->info.x;
	cp->y = cv->info.y;
    } else if ( sp->pointtype==pt_curve ) {
	cp->x = cv->info.x;
	cp->y = cv->info.y;
	if ( cp->x!=sp->me.x || cp->y!=sp->me.y ) {
	    real angle = atan2(cp->y-sp->me.y,cp->x-sp->me.x)-3.1415926535897932;
	    real len = sqrt((othercp->y-sp->me.y)*(othercp->y-sp->me.y) + (othercp->x-sp->me.x)*(othercp->x-sp->me.x));
	    /* There's just enough error in atan2 and sin/cos that horizontal */
	    /*  and vertical things don't quite match */
	    if ( cp->y-sp->me.y==0 ) {
		othercp->y = sp->me.y;
		othercp->x = sp->me.x + len*cos(angle);
	    } else if ( cp->x-sp->me.x==0 ) {
		othercp->x = sp->me.x;
		othercp->y = sp->me.y + len*sin(angle);
	    } else {
		othercp->x = sp->me.x + len*cos(angle);
		othercp->y = sp->me.y + len*sin(angle);
	    }
	    if ( sp->next!=NULL && othercp==&sp->nextcp )
		SplineRefigure(sp->next);
	    if ( sp->prev!=NULL && othercp==&sp->prevcp )
		SplineRefigure(sp->prev);
	} 
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
		    ((cv->info.x-sp->me.x)*(bp->x-sp->me.x) +
		     (cv->info.y-sp->me.y)*(bp->y-sp->me.y));
	    if ( len!=0 ) {
		dotprod /= len;
		if ( dotprod>0 ) dotprod = 0;
		cp->x = sp->me.x + dotprod*cos(angle);
		cp->y = sp->me.y + dotprod*sin(angle);
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

    if ( sp->next!=NULL && cp==&sp->nextcp )
	SplineRefigure(sp->next);
    if ( sp->prev!=NULL && cp==&sp->prevcp )
	SplineRefigure(sp->prev);
    CVSetCharChanged(cv,true);
}

static void CVAdjustSpline(CharView *cv) {
    Spline *old = cv->p.spline;
    TPoint tp[5];
    real t;
    Spline1D *oldx = &old->splines[0], *oldy = &old->splines[1];

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
    cv->p.spline = ApproximateSplineFromPoints(old->from,old->to,tp,5);
    old->from->pointtype = pt_corner; old->to->pointtype = pt_corner;
    old->from->nextcpdef = old->to->prevcpdef = false;
    SplineFree(old);
    CVSetCharChanged(cv,true);
}

void CVMoveSelection(CharView *cv, real dx, real dy) {
    real transform[6];
    RefChar *refs;
    ImageList *img;

    transform[0] = transform[3] = 1.0;
    transform[1] = transform[2] = 0.0;
    transform[4] = dx; transform[5] = dy;
    if ( transform[4]==0 && transform[5]==0 )
return;
    SplinePointListTransform(*cv->heads[cv->drawmode],transform,false);
    if ( cv->drawmode==dm_fore ) {
	for ( refs = cv->sc->refs; refs!=NULL; refs=refs->next ) if ( refs->selected ) {
	    refs->transform[4] += transform[4];
	    refs->transform[5] += transform[5];
	    refs->bb.minx += transform[4]; refs->bb.maxx += transform[4];
	    refs->bb.miny += transform[5]; refs->bb.maxy += transform[5];
	    SplinePointListTransform(refs->splines,transform,true);
	}
    } else if ( cv->drawmode==dm_back ) {
	for ( img = cv->sc->backimages; img!=NULL; img=img->next ) if ( img->selected ) {
	    img->xoff += transform[4];
	    img->yoff += transform[5];
	    img->bb.minx += transform[4]; img->bb.maxx += transform[4];
	    img->bb.miny += transform[5]; img->bb.maxy += transform[5];
	    SCOutOfDateBackground(cv->sc);
	}
    }
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
static void CVCheckMerges(CharView *cv ) {
    SplineSet *activess, *mergess;
    real fudge = 1/cv->scale;

  restart:
    for ( activess=*cv->heads[cv->drawmode]; activess!=NULL; activess=activess->next ) {
	if ( activess->first->prev==NULL && (activess->first->selected ||
		activess->last->selected)) {
	    for ( mergess = *cv->heads[cv->drawmode]; mergess!=NULL; mergess=mergess->next ) {
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
    CVTransFunc(cv,transform);
return( true );
}

void CVMouseMovePointer(CharView *cv ) {
    int needsupdate = false;

    /* if we haven't moved from the original location (ever) then this is a noop */
    if ( !cv->p.rubberbanding && !cv->recentchange &&
	    RealNear(cv->info.x,cv->p.cx) && RealNear(cv->info.y,cv->p.cy) )
return;

    if ( cv->p.width ) {
	if ( !cv->recentchange ) CVPreserveWidth(cv,cv->sc->width);
	cv->sc->width = cv->info.x;
	CVSetCharChanged(cv,true);
	needsupdate = true;
    } else if ( cv->expandedge!=ee_none )
	needsupdate = CVExpandEdge(cv);
    else if ( !cv->p.anysel ) {
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
	CVAdjustControl(cv,&cv->p.sp->nextcp);
	needsupdate = true;
    } else if ( cv->p.prevcp ) {
	if ( !cv->recentchange ) CVPreserveState(cv);
	CVAdjustControl(cv,&cv->p.sp->prevcp);
	needsupdate = true;
    } else if ( cv->p.spline!=NULL ) {
	if ( !cv->recentchange ) CVPreserveState(cv);
	CVAdjustSpline(cv);
	CVSetCharChanged(cv,true);
	needsupdate = true;
    } else {
	if ( !cv->recentchange ) CVPreserveState(cv);
	CVMoveSelection(cv,cv->info.x-cv->last_c.x,cv->info.y-cv->last_c.y);
	CVCheckMerges(cv);
	needsupdate = true;
    }
    if ( needsupdate )
	SCUpdateAll(cv->sc);
    cv->last_c.x = cv->info.x; cv->last_c.y = cv->info.y;
}

void CVMouseUpPointer(CharView *cv ) {
    if ( cv->p.width ) {
	cv->p.width = false;
	SCSynchronizeWidth(cv->sc,cv->sc->width,cv->p.cx,cv->fv);
	cv->expandedge = ee_none;
	GDrawSetCursor(cv->v,ct_mypointer);
    } else if ( cv->expandedge!=ee_none ) {
	CVUndoCleanup(cv);
	cv->expandedge = ee_none;
	GDrawSetCursor(cv->v,ct_mypointer);
    } else if ( CVAllSelected(cv) && cv->drawmode==dm_fore && cv->p.spline==NULL &&
	    !cv->p.prevcp && !cv->p.nextcp && cv->info.y==cv->p.cy ) {
	SCUndoSetLBearingChange(cv->sc,(int) rint(cv->info.x-cv->p.cx));
	SCSynchronizeLBearing(cv->sc,NULL,cv->info.x-cv->p.cx);
    }
}
