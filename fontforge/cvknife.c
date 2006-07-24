/* Copyright (C) 2000-2006 by George Williams */
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
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
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
    SCUpdateAll(cv->sc);
}
#endif

void CVMouseDownKnife(CharView *cv) {
#if defined(KNIFE_CONTINUOUS)
    CVPreserveState(cv);
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

void CVMouseUpKnife(CharView *cv) {
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

    while ( foundsomething ) {
	foundsomething = false;
	for ( spl = cv->layerheads[cv->drawmode]->splines; spl!=NULL && !foundsomething; spl = spl->next ) {
	    for ( s = spl->first->next; s!=NULL ; ) {
		nexts = NULL;
		if ( s->to!=spl->first )
		    nexts = s->to->next;
		if ( SplinesIntersect(s,&dummy,inters,t1s,t2s)>0 ) {
		    for ( i=0; i<4 && t1s[i]!=-1 && (t1s[i]<.0001 || t1s[i]>1-.0001); ++i );
		    if ( i<4 && t1s[i]!=-1 ) {
			/* There's at least one intersection point that isn't */
			/*  too close to an end point. Cut here, and then */
			/*  start all over again (we may need to alter the */
			/*  splineset structure so drastically that we just */
			/*  can't continue these loops) */
			foundsomething = true;
			nexts = NULL;
			if ( !ever )
			    CVPreserveState(cv);
			ever = true;
			/* Insert a break here */
			mid = SplineBisect(s,t1s[i]);
			mid2 = chunkalloc(sizeof(SplinePoint));
			*mid2 = *mid;
			mid2->hintmask = NULL;
			mid->next = NULL;
			mid2->prev = NULL;
			mid2->next->from = mid2;
			if ( spl->first==spl->last ) {
			    spl->first = mid2;
			    spl->last = mid;
			} else {
			    spl2 = chunkalloc(sizeof(SplineSet));
			    spl2->next = spl->next;
			    spl->next = spl2;
			    spl2->first = mid2;
			    spl2->last = spl->last;
			    spl->last = mid;
			}
		    }
		}
		s = nexts;
	    }
	}
    }
    if ( ever )
	CVCharChangedUpdate(cv);
#endif
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
