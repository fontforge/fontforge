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
#include "nomen.h"
#include <math.h>
#include <ustring.h>
#include <utype.h>

typedef struct searchitem {
    FontView dummy_fv;
    SplineFont dummy_sf;
    SplineChar sc_srch, sc_rpl;
    CharView cv_srch, cv_rpl;
/* ****** */
    FontView *fv;
    MetricsView *mv;
    SplineChar *curchar;
    SplineSet *path, *revpath, *replacepath, *revreplace;
    int pointcnt, rpointcnt;
    real fudge;
    real fudge_percent;			/* a value of .05 here represents 5% (we don't store the integer) */
    unsigned int tryreverse: 1;
    unsigned int tryflips: 1;
    unsigned int tryrotate: 1;
    unsigned int tryscale: 1;
    unsigned int doreplace: 1;
    unsigned int replaceall: 1;
    unsigned int findall: 1;
    unsigned int searchback: 1;
    unsigned int wrap: 1;
    unsigned int wasreversed: 1;
    SplineSet *matched_spl;
    SplinePoint *matched_sp;
    real matched_rot, matched_scale;
    double matched_co, matched_si;		/* Precomputed sin, cos */
    enum flipset { flip_none = 0, flip_x, flip_y, flip_xy } matched_flip;
} Search;

static int CoordMatches(real real_off, real search_off, Search *s) {
    if ( real_off >= search_off-s->fudge && real_off <= search_off+s->fudge )
return( true );
    if ( real_off >= search_off-s->fudge_percent*search_off &&
	    real_off <= search_off+s->fudge_percent*search_off )
return( true );
return( false );
}

static int BPMatches(BasePoint *sc_p1,BasePoint *sc_p2,BasePoint *p_p1,BasePoint *p_p2,
	int flip,real rot, real scale,Search *s) {
    real sxoff, syoff, pxoff, pyoff;

    sxoff = sc_p1->x-sc_p2->x;
    syoff = sc_p1->y-sc_p2->y;
    pxoff = p_p1->x-p_p2->x;
    pyoff = p_p1->y-p_p2->y;
    if ( flip&1 )
	sxoff = -sxoff;
    if ( flip&2 )
	syoff = -syoff;
    sxoff *= scale;
    syoff *= scale;
    if ( rot==0 )
return( CoordMatches(sxoff,pxoff,s) && CoordMatches(syoff,pyoff,s));

return( CoordMatches(sxoff*s->matched_co+syoff*s->matched_si,pxoff,s) &&
	CoordMatches(-sxoff*s->matched_si+syoff*s->matched_co,pyoff,s) );
}

static int SPMatches(SplinePoint *sp, Search *s, SplineSet *path) {
    SplinePoint *sc_sp, *nsc_sp, *p_sp, *np_sp;
    int flip, flipmax;
    double rot, scale;

    s->matched_sp = sp;
    for (sc_sp=sp, p_sp=path->first; ; ) {
	if ( p_sp->next==NULL )
return( true );
	np_sp = p_sp->next->to;
	if ( sc_sp->next==NULL )
return( false );
	nsc_sp = sc_sp->next->to;
	if ( !CoordMatches(sc_sp->nextcp.x-sc_sp->me.x,p_sp->nextcp.x-p_sp->me.x,s) ||
		!CoordMatches(sc_sp->nextcp.y-sc_sp->me.y,p_sp->nextcp.y-p_sp->me.y,s) ||
		!CoordMatches(nsc_sp->me.x-sc_sp->me.x,np_sp->me.x-p_sp->me.x,s) ||
		!CoordMatches(nsc_sp->me.y-sc_sp->me.y,np_sp->me.y-p_sp->me.y,s) ||
		!CoordMatches(nsc_sp->prevcp.x-nsc_sp->me.x,np_sp->prevcp.x-np_sp->me.x,s) ||
		!CoordMatches(nsc_sp->prevcp.y-nsc_sp->me.y,np_sp->prevcp.y-np_sp->me.y,s) )
return( false );
	if ( np_sp==path->first )
return( nsc_sp==sp );
	sc_sp = nsc_sp;
	p_sp = np_sp;
    }

    if ( s->tryflips ) for ( flip=flip_x ; flip<=flip_xy; ++flip ) {
	int xsign = (flip&1) ? -1 : 1, ysign = (flip&2) ? -1 : 1;
	for (sc_sp=sp, p_sp=path->first; ; ) {
	    if ( p_sp->next==NULL ) {
		s->matched_flip = flip;
return( true );
	    }
	    np_sp = p_sp->next->to;
	    if ( sc_sp->next==NULL )
return( false );
	    nsc_sp = sc_sp->next->to;
	    if ( !CoordMatches(sc_sp->nextcp.x-sc_sp->me.x,xsign*(p_sp->nextcp.x-p_sp->me.x),s) ||
		    !CoordMatches(sc_sp->nextcp.y-sc_sp->me.y,ysign*(p_sp->nextcp.y-p_sp->me.y),s) ||
		    !CoordMatches(nsc_sp->me.x-sc_sp->me.x,xsign*(np_sp->me.x-p_sp->me.x),s) ||
		    !CoordMatches(nsc_sp->me.y-sc_sp->me.y,ysign*(np_sp->me.y-p_sp->me.y),s) ||
		    !CoordMatches(nsc_sp->prevcp.x-nsc_sp->me.x,xsign*(np_sp->prevcp.x-np_sp->me.x),s) ||
		    !CoordMatches(nsc_sp->prevcp.y-nsc_sp->me.y,ysign*(np_sp->prevcp.y-np_sp->me.y),s) )
return( false );
	    if ( np_sp==path->first )
return( nsc_sp==sp );
	    sc_sp = nsc_sp;
	    p_sp = np_sp;
	}
    }

    if ( s->tryrotate || s->tryscale ) {
	p_sp = path->first;
	np_sp = p_sp->next->to;	/* if p_sp->next were NULL, we'd have returned by now */
	sc_sp = sp;
	if ( sc_sp->next==NULL )
return( false );
	nsc_sp = sc_sp->next->to;
	if ( p_sp->me.x==np_sp->me.x && p_sp->me.y==np_sp->me.y )
return( false );
	if ( sc_sp->me.x==nsc_sp->me.x && sc_sp->me.y==nsc_sp->me.y )
return( false );
	if ( !s->tryrotate ) {
	    if ( p_sp->me.x==np_sp->me.x )
		scale = (np_sp->me.y-p_sp->me.y) / (nsc_sp->me.y-sc_sp->me.y);
	    else if ( p_sp->me.y==np_sp->me.y )
		scale = (np_sp->me.x-p_sp->me.x) / (nsc_sp->me.x-sc_sp->me.x);
	    else {
		real yscale = (np_sp->me.y-p_sp->me.y) / (nsc_sp->me.y-sc_sp->me.y);
		scale = (np_sp->me.x-p_sp->me.x) / (nsc_sp->me.x-sc_sp->me.x);
		if ( scale<.99*yscale || scale>1.01*yscale )
return( false );
	    }
	    rot = 0;
	} else {
	    rot = atan2(nsc_sp->me.y-sc_sp->me.y,nsc_sp->me.x-sc_sp->me.x) -
		    atan2(np_sp->me.y-p_sp->me.y,np_sp->me.x-p_sp->me.x);
	    if ( !s->tryscale )
		scale = 1;
	    else
		scale = sqrt(  ((np_sp->me.y-p_sp->me.y)*(np_sp->me.y-p_sp->me.y) +
				(np_sp->me.x-p_sp->me.x)*(np_sp->me.x-p_sp->me.x))/
			       ((nsc_sp->me.y-sc_sp->me.y)*(nsc_sp->me.y-sc_sp->me.y) +
				(nsc_sp->me.x-sc_sp->me.x)*(nsc_sp->me.x-sc_sp->me.x))  );
	}
	if ( rot==0 )
	    s->matched_co=1,s->matched_si=0;
	else if ( rot>3.14159 && rot<3.141595 )
	    s->matched_co=-1,s->matched_si=0;
	else if ( rot>1.570793 && rot<1.570799 )
	    s->matched_co=0,s->matched_si=1;
	else if ( (rot>4.712386 && rot<4.712392 ) ||
		  (rot<-1.570793 && rot>-1.570799 ) )
	    s->matched_co=0,s->matched_si=-1;
	else
	    s->matched_co = cos(s->matched_rot), s->matched_si = sin(s->matched_rot);
	if ( s->tryflips )
	    flipmax = flip_xy;
	else
	    flipmax = flip_none;
	for ( flip=flip_none ; flip<flipmax; ++flip ) {
	    for (sc_sp=sp, p_sp=path->first; ; ) {
		if ( p_sp->next==NULL ) {
		    s->matched_flip = flip;
		    s->matched_rot = rot;
		    s->matched_scale = scale;
return( true );
		}
		np_sp = p_sp->next->to;
		if ( sc_sp->next==NULL )
return( false );
		nsc_sp = sc_sp->next->to;
		if ( !BPMatches(&sc_sp->nextcp,&sc_sp->me,&p_sp->nextcp,&p_sp->me,flip,rot,scale,s) ||
			!BPMatches(&nsc_sp->me,&sc_sp->me,&np_sp->me,&p_sp->me,flip,rot,scale,s) ||
			!BPMatches(&nsc_sp->prevcp,&nsc_sp->me,&np_sp->prevcp,&np_sp->me,flip,rot,scale,s) )
return( false );
		if ( np_sp==path->first )
return( nsc_sp==sp );
		sc_sp = nsc_sp;
		p_sp = np_sp;
	    }
	}
    }
return( false );
}

static int SCMatches(SplineChar *sc,Search *s) {
    /* don't look in refs because we can't do a replace there */
    SplineSet *spl;
    SplinePoint *sp;

    for ( spl=sc->splines; spl!=NULL; spl=spl->next ) {
	s->matched_spl = spl;
	for ( sp=spl->first; ; ) {
	    if ( SPMatches(sp,s,s->path))
return( true );
	    if ( s->tryreverse && SPMatches(sp,s,s->revpath)) {
		s->wasreversed = true;
return( true );
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
return( false );
}

static void AdjustBP(BasePoint *changeme,BasePoint *rel,
	BasePoint *shouldbe, BasePoint *shouldberel,
	BasePoint *search, BasePoint *searchrel,
	BasePoint *fudge,
	Search *s) {
    real xoff,yoff;

    if ( s->pointcnt==s->rpointcnt ) {
	xoff = (changeme->x-rel->x);
	yoff = (changeme->y-rel->y);
	if ( s->matched_flip&1 )
	    xoff=-xoff;
	if ( s->matched_flip&2 )
	    yoff =-yoff;
	xoff *= s->matched_scale;
	yoff *= s->matched_scale;
	fudge->x = xoff*s->matched_co + yoff*s->matched_si - (search->x-searchrel->x);
	fudge->y = yoff*s->matched_co - xoff*s->matched_si - (search->y-searchrel->y);
    }
    xoff = (shouldbe->x-shouldberel->x);
    yoff = (shouldbe->y-shouldberel->y);
    if ( s->matched_flip&1 )
	xoff=-xoff;
    if ( s->matched_flip&2 )
	yoff =-yoff;
    xoff *= s->matched_scale;
    yoff *= s->matched_scale;
    changeme->x = xoff*s->matched_co + yoff*s->matched_si + fudge->x  + rel->x;
    changeme->y = yoff*s->matched_co - xoff*s->matched_si + fudge->y  + rel->y;
}

static void FudgeFigure(SplineChar *sc,Search *s,SplineSet *path,BasePoint *fudge) {
    SplinePoint *search, *searchrel, *found, *foundrel;
    real xoff, yoff;

    fudge->x = fudge->y = 0;
    if ( path->first->prev!=NULL )		/* closed path, should end where it began */
return;						/*  => no fudge */

    foundrel = s->matched_sp; searchrel=path->first;
    for ( found=foundrel, search=searchrel ; ; ) {
	if ( found->next==NULL )
    break;
	found = found->next->to;
	search = search->next->to;
    }

    xoff = (found->me.x-foundrel->me.x);
    yoff = (found->me.y-foundrel->me.y);
    if ( s->matched_flip&1 )
	xoff=-xoff;
    if ( s->matched_flip&2 )
	yoff =-yoff;
    xoff *= s->matched_scale;
    yoff *= s->matched_scale;
    fudge->x = xoff*s->matched_co + yoff*s->matched_si - (search->me.x-searchrel->me.x);
    fudge->y = yoff*s->matched_co - xoff*s->matched_si - (search->me.y-searchrel->me.y);
}

static void DoReplace(SplineChar *sc,Search *s) {
    SplinePoint *sc_p, *nsc_p, *p_p, *np_p, *r_p, *nr_p;
    BasePoint fudge;
    SplineSet *path, *rpath;

    if ( s->wasreversed ) {
	path = s->revpath;
	rpath = s->revreplace;
    } else {
	path = s->path;
	rpath = s->replacepath;
    }

    if ( s->pointcnt!=s->rpointcnt )
	/* Total "fudge" amount should be spread evenly over each point */
	FudgeFigure(sc,s,path,&fudge);
    else
	/* else any "fudges" should be transfered point by point */;
    for ( sc_p = s->matched_sp, p_p = path->first, r_p = rpath->first; ; ) {
	if ( p_p->next==NULL )
return;		/* done */
	if ( sc_p->next==NULL || r_p->next==NULL ) {
	    GDrawIError("Unexpected point mismatch in replace");
return;
	}
	np_p = p_p->next->to; nsc_p = sc_p->next->to; nr_p = r_p->next->to;
	if ( p_p==path->first ) {
	    AdjustBP(&sc_p->nextcp,&sc_p->me,&r_p->nextcp,&r_p->me,&p_p->nextcp,&p_p->me,&fudge,s);
	    if ( p_p->prev!=NULL )
		AdjustBP(&sc_p->prevcp,&sc_p->me,&r_p->prevcp,&r_p->me,&p_p->prevcp,&p_p->me,&fudge,s);
	}
	if ( np_p==path->first )
return;
	if ( np_p->next!=NULL )
	    AdjustBP(&nsc_p->nextcp,&nsc_p->me,&nr_p->nextcp,&nr_p->me,&np_p->nextcp,&np_p->me,&fudge,s);
	AdjustBP(&nsc_p->prevcp,&nsc_p->me,&nr_p->prevcp,&nr_p->me,&np_p->prevcp,&np_p->me,&fudge,s);
	AdjustBP(&nsc_p->me,&sc_p->me,&nr_p->me,&r_p->me,&np_p->me,&p_p->me,&fudge,s);
	if ( nsc_p->next!=NULL )
	    SplineRefigure(nsc_p->next);
	if ( nsc_p->prev!=NULL )
	    SplineRefigure(nsc_p->prev);
    }
}
