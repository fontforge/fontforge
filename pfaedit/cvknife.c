/* Copyright (C) 2000-2002 by George Williams */
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
    if ( p->spl->first!=p->spl->last )
	if ( p->sp==p->spl->first || p->sp==p->spl->last )
return;					/* Already cut here */
    n = chunkalloc(sizeof(SplinePoint));
    p->sp->pointtype = pt_corner;
    *n = *p->sp;
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

void CVMouseDownKnife(CharView *cv) {
    CVPreserveState(cv);
    cv->lastknife.x = cv->lastknife.y = -9999;
    ProcessKnife(cv,&cv->p);
}

void CVMouseMoveKnife(CharView *cv, PressedOn *p) {
    ProcessKnife(cv,p);
}

void CVMouseUpKnife(CharView *cv) {
}
