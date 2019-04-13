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

#include "fontforge-config.h"
#include "cvundoes.h"
#include "fontforgeui.h"
#include "spiro.h"
#include "splineutil.h"
#include <math.h>

#if defined(KNIFE_CONTINUOUS)	/* Use this code to do cuts as we move along. Probably a bad idea, let's wait till the end */
static void ProcessKnife(CharView *cv, PressedOn *p) {
    real dx, dy;
    SplinePoint *n;

    /* If we've already made a cut, don't make another cut too close to it */
    /*  ie. if the hand shakes over a cut let's not get about six tiny cut */
    /*  segments adjacent to one another */
    if ( (dx=cv->info.x-cv->lastknife.x)<0 ) dx=-dx;
    if ( (dy=cv->info.y-cv->lastknife.y)<0 ) dy=-dy;
    if ( (dx+dy)/cv->scale <= 6 )
return;
    if ( p->sp==NULL && p->spline==NULL )
return;					/* Nothing to cut */

    if ( p->spline!=NULL )
	p->sp = SplineBisect(p->spline,p->t);
    if ( p->spl==NULL )		/* Kanou says this can happen. It doesn't hurt to check for it */
return;
    if ( p->spl->first!=p->spl->last )
	if ( p->sp==p->spl->first || p->sp==p->spl->last )
return;					/* Already cut here */
    n = chunkalloc(sizeof(SplinePoint));
    p->sp->pointtype = pt_corner;
    *n = *p->sp;
    n->hintmask = NULL;
    p->sp->next = NULL;
    n->prev = NULL;
    n->next->from = n;
    if ( p->spl->first==p->spl->last ) {
	p->spl->first = n;
	p->spl->last = p->sp;
    } else {
	SplinePointList *nspl = chunkalloc(sizeof(SplinePointList));
	nspl->next = p->spl->next;
	p->spl->next = nspl;
	nspl->first = n;
	nspl->last = p->spl->last;
	p->spl->last = p->sp;
    }

    cv->lastknife.x = cv->info.x;
    cv->lastknife.y = cv->info.y;
    CVSetCharChanged(cv,true);
    SCUpdateAll(cv->b.sc);
}
#endif

void CVMouseDownKnife(CharView *cv) {
#if defined(KNIFE_CONTINUOUS)
    CVPreserveState(&cv->b);
    cv->lastknife.x = cv->lastknife.y = -9999;
    ProcessKnife(cv,&cv->p);
#else
    cv->p.rubberlining = true;
#endif
}

void CVMouseMoveKnife(CharView *cv, PressedOn *p) {
#if defined(KNIFE_CONTINUOUS)
    ProcessKnife(cv,p);
#else
    GDrawRequestExpose(cv->v,NULL,false);
#endif
}

#if !defined(KNIFE_CONTINUOUS)
static void ReorderSpirosAndAddAndCut(SplineSet *spl,int spiro_index) {
    /* We just cut a closed contour. It is now open. */
    /* If spl->spiros[spiro_index] == spl->first->me then they cut on top of */
    /*  a point which already existed */
    /*   If so, move that point to the head of the list */
    /*   copy it so that it's at the end of the list too */
    /*   mark the start as SPIRO_OPEN */
    /* Otherwise they cut between spiro_cps */
    /*  Insert 2 copies of spl->first->me after spiro_index */
    /*  rotate so that one of these is at the end of the list and the other at the start */
    /*  mark start as SPIRO_OPEN */
    spiro_cp *newspiros;

    if ( spiro_index!=spl->spiro_cnt-1 &&
	    spl->first->me.x == spl->spiros[spiro_index].x &&
	    spl->first->me.y == spl->spiros[spiro_index].y ) {
	newspiros = malloc((spl->spiro_cnt+1) * sizeof(spiro_cp));
	memcpy(newspiros,spl->spiros+spiro_index,(spl->spiro_cnt-1-spiro_index)*sizeof(spiro_cp));
	memcpy(newspiros+(spl->spiro_cnt-1-spiro_index),spl->spiros,spiro_index*sizeof(spiro_cp));
	memcpy(newspiros+spl->spiro_cnt-1,newspiros,sizeof(spiro_cp));
	memcpy(newspiros+spl->spiro_cnt,spl->spiros+spl->spiro_cnt-1,sizeof(spiro_cp));
	newspiros[0].ty = SPIRO_OPEN_CONTOUR;
	free(spl->spiros);
	spl->spiros = newspiros;
	++(spl->spiro_cnt);
	spl->spiro_max = spl->spiro_cnt;
    } else {
	newspiros = malloc((spl->spiro_cnt+2) * sizeof(spiro_cp));
	newspiros[0].x = spl->first->me.x;
	newspiros[0].y = spl->first->me.y;
	newspiros[0].ty = SPIRO_OPEN_CONTOUR;
	memcpy(newspiros+1,spl->spiros+spiro_index+1,(spl->spiro_cnt-1-(spiro_index+1))*sizeof(spiro_cp));
	memcpy(newspiros+1+(spl->spiro_cnt-1-(spiro_index+1)),spl->spiros,(spiro_index+1)*sizeof(spiro_cp));
	memcpy(newspiros+spl->spiro_cnt,newspiros,sizeof(spiro_cp));
	memcpy(newspiros+spl->spiro_cnt+1,spl->spiros+spl->spiro_cnt-1,sizeof(spiro_cp));
	newspiros[spl->spiro_cnt].ty = SPIRO_G4;
	free(spl->spiros);
	spl->spiros = newspiros;
	spl->spiro_cnt += 2;
	spl->spiro_max = spl->spiro_cnt;
    }
}

static void SplitSpirosAndAddAndCut(SplineSet *spl,SplineSet *spl2,int spiro_index) {
    /* OK, spl was an open contour and we just cut it at spiro_index */
    /* We don't need to rotate the spiros */
    /* The first half of spl remains in it. (We'll leave all spiros up to and including spiro_index) */
    /* If spl->spiros[spiro_index] != spl->first->me we must add spl->first here */
    /* In the spl2 we either start out with spl->spiros[spiro_index] or with spl->first */
    /* then add all spiros after spiro_index */

    spl2->spiros = malloc((spl->spiro_cnt-spiro_index+2) * sizeof(spiro_cp));
    spl2->spiro_max = spl->spiro_cnt-spiro_index+2;
    if ( spl2->first->me.x == spl->spiros[spiro_index].x &&
	    spl2->first->me.y == spl->spiros[spiro_index].y ) {
	memcpy(spl2->spiros,spl->spiros+spiro_index,(spl->spiro_cnt-spiro_index)*sizeof(spiro_cp));
	spl2->spiros[0].ty = SPIRO_OPEN_CONTOUR;
	memcpy(spl->spiros+spiro_index+1,spl->spiros+spl->spiro_cnt-1,sizeof(spiro_cp));
	spl2->spiro_cnt = spl->spiro_cnt-spiro_index;
	spl->spiro_cnt = spiro_index+2;
    } else {
	spl2->spiros[0].x = spl2->first->me.x;
	spl2->spiros[0].y = spl2->first->me.y;
	spl2->spiros[0].ty = SPIRO_OPEN_CONTOUR;
	memcpy(spl2->spiros+1,spl->spiros+spiro_index+1,(spl->spiro_cnt-(spiro_index+1))*sizeof(spiro_cp));
	spl2->spiro_cnt = spl->spiro_cnt-spiro_index;
	if ( spiro_index+3>spl->spiro_max )
	    spl->spiros = realloc(spl->spiros,(spl->spiro_max=spiro_index+3)*sizeof(spiro_cp));
	memcpy(spl->spiros+spiro_index+1,spl2->spiros,sizeof(spiro_cp));
	spl->spiros[spiro_index+1].ty = SPIRO_G4;
	memcpy(spl->spiros+spiro_index+2,spl->spiros+spl->spiro_cnt-1,sizeof(spiro_cp));
	spl->spiro_cnt = spiro_index+3;
    }
}
#endif

void CVMouseUpKnife(CharView *cv, GEvent *event)
{
#if !defined(KNIFE_CONTINUOUS)
    /* draw a line from (cv->p.cx,cv->p.cy) to (cv->info.x,cv->info.y) */
    /*  and cut anything intersected by it */
    SplineSet *spl, *spl2;
    Spline *s, *nexts;
    Spline dummy;
    SplinePoint dummyfrom, dummyto, *mid, *mid2;
    BasePoint inters[9];
    extended t1s[10], t2s[10];
    int foundsomething = true, ever = false;
    int i;
    int spiro_index = 0;

    memset(&dummy,0,sizeof(dummy));
    memset(&dummyfrom,0,sizeof(dummyfrom));
    memset(&dummyto,0,sizeof(dummyto));
    dummyfrom.me.x = cv->p.cx; dummyfrom.me.y = cv->p.cy;
    dummyto.me.x = cv->info.x; dummyto.me.y = cv->info.y;
    dummyfrom.nextcp = dummyfrom.prevcp = dummyfrom.me;
    dummyto.nextcp = dummyto.prevcp = dummyto.me;
    dummyfrom.nonextcp = dummyfrom.noprevcp = dummyto.nonextcp = dummyto.noprevcp = true;
    dummy.splines[0].d = cv->p.cx; dummy.splines[0].c = cv->info.x-cv->p.cx;
    dummy.splines[1].d = cv->p.cy; dummy.splines[1].c = cv->info.y-cv->p.cy;
    dummy.from = &dummyfrom; dummy.to = &dummyto;
    dummy.islinear = dummy.knownlinear = true;
    dummyfrom.next = dummyto.prev = &dummy;

    for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL ; spl = spl->next )
	spl->ticked = false;

    while ( foundsomething ) {
	foundsomething = false;
	for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL && !foundsomething; spl = spl->next ) {
	    for ( s = spl->first->next; s!=NULL ; ) {
		nexts = NULL;
		if ( s->to!=spl->first )
		    nexts = s->to->next;
		if ( SplinesIntersect(s,&dummy,inters,t1s,t2s)>0 ) {
		    if ( event->u.mouse.state&ksm_meta ) {
			for ( i=0; i<4 && t1s[i]!=-1 && (t1s[i]<.0001 || t1s[i]>1-.0001); ++i );
			if ( i<4 && t1s[i]!=-1 ) {
			    /* With meta down we just remove the spline rather than */
			    /* cutting it */
			    foundsomething = true;
			    nexts = NULL;
			    if ( !ever )
				CVPreserveState(&cv->b);
			    ever = true;
			    if ( spl->first==spl->last ) {
				spl->first = s->to;
				spl->last = s->from;
			    } else {
				spl2 = chunkalloc(sizeof(SplineSet));
				spl2->next = spl->next;
				spl->next = spl2;
				spl2->first = s->to;
				spl2->last = spl->last;
				spl->last = s->from;
			    }
			    s->to->prev = s->from->next = NULL;
			    SplineFree(s);
			    SplineSetSpirosClear(spl);
			}
		    } else {
			for ( i=0; i<4 && t1s[i]!=-1 &&
				(( t1s[i]<.001 && s->from->prev==NULL ) ||
			         ( t1s[i]>1-.001 && s->to->next == NULL ));
				++i );
			if ( i<4 && t1s[i]!=-1 ) {
			    foundsomething = true;
			    nexts = NULL;
			    if ( !ever )
				CVPreserveState(&cv->b);
			    ever = true;
			    spiro_index = -1;
			    if ( cv->b.sc->inspiro && hasspiro()) {
				if ( spl->spiro_cnt!=0 )
				    spiro_index = SplineT2SpiroIndex(s,t1s[i],spl);
			    } else
				SplineSetSpirosClear(spl);
			    if ( t1s[i]<.001 ) {
				mid = s->from;
			    } else if ( t1s[i]>1-.001 ) {
				mid = s->to;
			    } else
				mid = SplineBisect(s,t1s[i]);
			    /* if the intersection is close to an end point */
			    /*  cut at the end point, else break in the middle */
			    /*  Cut here, and then */
			    /*  start all over again (we may need to alter the */
			    /*  splineset structure so drastically that we just */
			    /*  can't continue these loops) */
			    mid->pointtype = pt_corner;
			    mid2 = chunkalloc(sizeof(SplinePoint));
			    *mid2 = *mid;
			    mid2->hintmask = NULL;
			    mid->next = NULL;
			    mid2->prev = NULL;
			    mid2->next->from = mid2;
			    spl->ticked = true;
			    if ( spl->first==spl->last ) {
				spl->first = mid2;
				spl->last = mid;
			        if ( spiro_index!=-1 )
				    ReorderSpirosAndAddAndCut(spl,spiro_index);
			    } else {
				spl2 = chunkalloc(sizeof(SplineSet));
				spl2->next = spl->next;
				spl->next = spl2;
				spl2->first = mid2;
				spl2->last = spl->last;
				spl->last = mid;
			        if ( spiro_index!=-1 )
				    SplitSpirosAndAddAndCut(spl,spl2,spiro_index);
				spl2->ticked = true;
			    }
			}
		    }
		}
		s = nexts;
	    }
	}
    }
    if ( ever ) {
	for ( spl = cv->b.layerheads[cv->b.drawmode]->splines; spl!=NULL ; spl = spl->next ) {
	    if ( spl->ticked && spl->spiros!=NULL && cv->b.sc->inspiro && hasspiro())
		SSRegenerateFromSpiros(spl);
	    spl->ticked = false;
	}
	CVCharChangedUpdate(   &cv->b );
    } else {
        GDrawRequestExpose(cv->v, NULL, false);
    }
#endif
}

