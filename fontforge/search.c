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
#define _DEFINE_SEARCHVIEW_
#include "pfaeditui.h"
#include <math.h>
#include <ustring.h>
#include <utype.h>
#include <gkeysym.h>

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static SearchView *searcher=NULL;

#define CID_Allow	1000
#define CID_Flipping	1001
#define CID_Scaling	1002
#define CID_Rotating	1003
#define CID_Selected	1004
#define CID_Find	1005
#define CID_FindAll	1006
#define CID_Replace	1007
#define CID_ReplaceAll	1008
#define CID_Cancel	1009
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static int CoordMatches(real real_off, real search_off, SearchView *s) {
    real fudge;
    if ( real_off >= search_off-s->fudge && real_off <= search_off+s->fudge )
return( true );
    fudge = s->fudge_percent*search_off;
    if ( fudge<0 ) fudge = -fudge;
    if ( real_off >= search_off-fudge && real_off <= search_off+fudge )
return( true );
return( false );
}

static int BPMatches(BasePoint *sc_p1,BasePoint *sc_p2,BasePoint *p_p1,BasePoint *p_p2,
	int flip,real rot, real scale,SearchView *s) {
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

static int SPMatchesF(SplinePoint *sp, SearchView *s, SplineSet *path, int substring ) {
    SplinePoint *sc_sp, *nsc_sp, *p_sp, *np_sp;
    int flip, flipmax;
    double rot, scale;

    s->matched_sp = sp;
    for (sc_sp=sp, p_sp=path->first; ; ) {
	if ( p_sp->next==NULL ) {
	    if ( substring || sc_sp->next==NULL ) {
		s->last_sp = sc_sp;
return( true );
	    }
    break;
	}
	np_sp = p_sp->next->to;
	if ( sc_sp->next==NULL )
    break;
	nsc_sp = sc_sp->next->to;
	if ( !CoordMatches(sc_sp->nextcp.x-sc_sp->me.x,p_sp->nextcp.x-p_sp->me.x,s) ||
		!CoordMatches(sc_sp->nextcp.y-sc_sp->me.y,p_sp->nextcp.y-p_sp->me.y,s) ||
		!CoordMatches(nsc_sp->me.x-sc_sp->me.x,np_sp->me.x-p_sp->me.x,s) ||
		!CoordMatches(nsc_sp->me.y-sc_sp->me.y,np_sp->me.y-p_sp->me.y,s) ||
		!CoordMatches(nsc_sp->prevcp.x-nsc_sp->me.x,np_sp->prevcp.x-np_sp->me.x,s) ||
		!CoordMatches(nsc_sp->prevcp.y-nsc_sp->me.y,np_sp->prevcp.y-np_sp->me.y,s) )
    break;
	if ( np_sp==path->first ) {
	    if ( nsc_sp==sp ) {
		s->last_sp = sp;
return( true );
	    }
    break;
	}
	sc_sp = nsc_sp;
	p_sp = np_sp;
    }

    if ( s->tryflips ) for ( flip=flip_x ; flip<=flip_xy; ++flip ) {
	int xsign = (flip&1) ? -1 : 1, ysign = (flip&2) ? -1 : 1;
	for (sc_sp=sp, p_sp=path->first; ; ) {
	    if ( p_sp->next==NULL ) {
		if ( substring || sc_sp->next==NULL ) {
		    s->matched_flip = flip;
		    s->last_sp = sc_sp;
return( true );
		} else
    break;
	    }
	    np_sp = p_sp->next->to;
	    if ( sc_sp->next==NULL )
    break;
	    nsc_sp = sc_sp->next->to;
	    if ( !CoordMatches(sc_sp->nextcp.x-sc_sp->me.x,xsign*(p_sp->nextcp.x-p_sp->me.x),s) ||
		    !CoordMatches(sc_sp->nextcp.y-sc_sp->me.y,ysign*(p_sp->nextcp.y-p_sp->me.y),s) ||
		    !CoordMatches(nsc_sp->me.x-sc_sp->me.x,xsign*(np_sp->me.x-p_sp->me.x),s) ||
		    !CoordMatches(nsc_sp->me.y-sc_sp->me.y,ysign*(np_sp->me.y-p_sp->me.y),s) ||
		    !CoordMatches(nsc_sp->prevcp.x-nsc_sp->me.x,xsign*(np_sp->prevcp.x-np_sp->me.x),s) ||
		    !CoordMatches(nsc_sp->prevcp.y-nsc_sp->me.y,ysign*(np_sp->prevcp.y-np_sp->me.y),s) )
    break;
	    if ( np_sp==path->first ) {
		if ( nsc_sp==sp ) {
		    s->matched_flip = flip;
		    s->last_sp = sp;
return( true );
		} else
    break;
	    }
	    sc_sp = nsc_sp;
	    p_sp = np_sp;
	}
    }

    if ( s->tryrotate || s->tryscale ) {
	if ( s->tryflips )
	    flipmax = flip_xy;
	else
	    flipmax = flip_none;
	for ( flip=flip_none ; flip<flipmax; ++flip ) {	p_sp = path->first;
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
		int xsign = (flip&1)?-1:1, ysign=(flip&2)?-1:1;
		rot = atan2(xsign*(nsc_sp->me.y-sc_sp->me.y),ysign*(nsc_sp->me.x-sc_sp->me.x)) -
			atan2(np_sp->me.y-p_sp->me.y,np_sp->me.x-p_sp->me.x);
		if ( !s->tryscale )
		    scale = 1;
		else
		    scale = sqrt(  ((np_sp->me.y-p_sp->me.y)*(np_sp->me.y-p_sp->me.y) +
				    (np_sp->me.x-p_sp->me.x)*(np_sp->me.x-p_sp->me.x))/
				   ((nsc_sp->me.y-sc_sp->me.y)*(nsc_sp->me.y-sc_sp->me.y) +
				    (nsc_sp->me.x-sc_sp->me.x)*(nsc_sp->me.x-sc_sp->me.x))  );
	    }
	    if ( scale>-.00001 && scale<.00001 )
return( false );
	    s->matched_rot = rot;
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
		s->matched_co = cos(rot), s->matched_si = sin(rot);
	    for (sc_sp=sp, p_sp=path->first; ; ) {
		if ( p_sp->next==NULL ) {
		    if ( substring || sc_sp->next==NULL ) {
			s->matched_flip = flip;
			s->matched_rot = rot;
			s->matched_scale = scale;
			s->last_sp = sc_sp;
return( true );
		    } else
return( false );
		}
		np_sp = p_sp->next->to;
		if ( sc_sp->next==NULL )
return( false );
		nsc_sp = sc_sp->next->to;
		if ( !BPMatches(&sc_sp->nextcp,&sc_sp->me,&p_sp->nextcp,&p_sp->me,flip,rot,scale,s) ||
			!BPMatches(&nsc_sp->me,&sc_sp->me,&np_sp->me,&p_sp->me,flip,rot,scale,s) ||
			!BPMatches(&nsc_sp->prevcp,&nsc_sp->me,&np_sp->prevcp,&np_sp->me,flip,rot,scale,s) )
return( false );
		if ( np_sp==path->first ) {
		    if ( nsc_sp==sp ) {
			s->matched_flip = flip;
			s->matched_rot = rot;
			s->matched_scale = scale;
			s->last_sp = sp;
return( true );
		    } else
return( false );
		}
		sc_sp = nsc_sp;
		p_sp = np_sp;
	    }
	}
    }
return( false );
}

static int SPMatchesO(SplinePoint *sp, SearchView *s, SplineSet *path) {
    SplinePoint *sc_sp, *nsc_sp, *p_sp, *np_sp;

    s->matched_sp = sp;
    if ( s->matched_rot==0 && s->matched_scale==1 && s->matched_flip==flip_none ) {
	for (sc_sp=sp, p_sp=path->first; ; ) {
	    if ( p_sp->next==NULL )
return( sc_sp->next==NULL );
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
    } else if ( s->matched_rot==0 && s->matched_scale==1 ) {
	int xsign = (s->matched_flip&1) ? -1 : 1, ysign = (s->matched_flip&2) ? -1 : 1;
	for (sc_sp=sp, p_sp=path->first; ; ) {
	    if ( p_sp->next==NULL )
return( sc_sp->next==NULL );
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
    } else {
	p_sp = path->first;
	for (sc_sp=sp, p_sp=path->first; ; ) {
	    if ( p_sp->next==NULL )
return( sc_sp->next==NULL );
	    np_sp = p_sp->next->to;
	    if ( sc_sp->next==NULL )
return( false );
	    nsc_sp = sc_sp->next->to;
	    if ( !BPMatches(&sc_sp->nextcp,&sc_sp->me,&p_sp->nextcp,&p_sp->me,
			s->matched_flip,s->matched_rot,s->matched_scale,s) ||
		    !BPMatches(&nsc_sp->me,&sc_sp->me,&np_sp->me,&p_sp->me,
			s->matched_flip,s->matched_rot,s->matched_scale,s) ||
		    !BPMatches(&nsc_sp->prevcp,&nsc_sp->me,&np_sp->prevcp,&np_sp->me,
			s->matched_flip,s->matched_rot,s->matched_scale,s) )
return( false );
	    if ( np_sp==path->first )
return( nsc_sp==sp );
	    sc_sp = nsc_sp;
	    p_sp = np_sp;
	}
    }
return( false );
}

static void SVBuildTrans(SearchView *s,real transform[6]) {
    memset(transform,0,sizeof(real [6]));
    transform[0] = transform[3] = 1;
    if ( s->matched_flip&1 )
	transform[0] = -1;
    if ( s->matched_flip&2 )
	transform[3] = -1;
    transform[0] *= s->matched_scale;
    transform[3] *= s->matched_scale;
    transform[1] = -transform[0]*s->matched_si;
    transform[0] *= s->matched_co;
    transform[2] = transform[3]*s->matched_si;
    transform[3] *= s->matched_co;
    transform[4] = s->matched_x;
    transform[5] = s->matched_y;
}

static void SVFigureTranslation(SearchView *s,BasePoint *p,SplinePoint *sp) {
    real transform[6];
    BasePoint res;

    SVBuildTrans(s,transform);
    res.x = transform[0]*p->x + transform[2]*p->y + transform[4];
    res.y = transform[1]*p->x + transform[3]*p->y + transform[5];
    s->matched_x = sp->me.x - res.x;
    s->matched_y = sp->me.y - res.y;
}

static int SPMatches(SplinePoint *sp, SearchView *s, SplineSet *path, int oriented ) {
    real transform[6];
    BasePoint *p, res;
    if ( oriented ) {
	SVBuildTrans(s,transform);
	p = &path->first->me;
	res.x = transform[0]*p->x + transform[2]*p->y + transform[4];
	res.y = transform[1]*p->x + transform[3]*p->y + transform[5];
	if ( sp->me.x>res.x+.01 || sp->me.x<res.x-.01 ||
		sp->me.y>res.y+.01 || sp->me.y<res.y-.01 )
return( false );

return( SPMatchesO(sp,s,path));
    } else {
	if ( !SPMatchesF(sp,s,path,false))
return( false );
	SVFigureTranslation(s,&path->first->me,sp);
return( true );
    }
}

/* We are given a single, unclosed path to match. It is a match if it */
/*  corresponds to any subset of a path in the character */
static int SCMatchesIncomplete(SplineChar *sc,SearchView *s,int startafter) {
    /* don't look in refs because we can't do a replace there */
    SplineSet *spl;
    SplinePoint *sp;

    for ( spl=startafter?s->matched_spl:sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
	s->matched_spl = spl;
	for ( sp=startafter?s->last_sp:spl->first; ; ) {
	    if ( SPMatchesF(sp,s,s->path,true)) {
		SVFigureTranslation(s,&s->path->first->me,sp);
return( true );
	    } else if ( s->tryreverse && SPMatchesF(sp,s,s->revpath,true)) {
		SVFigureTranslation(s,&s->revpath->first->me,sp);
		s->wasreversed = true;
return( true );
	    }
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	startafter = false;
    }
return( false );
}

/* We are given several paths/refs to match. We have a match if each path */
/*  matches a path in the character exactly, and each ref also. A searched for*/
/*  path can not match a subset of a path */
static int SCMatchesFull(SplineChar *sc,SearchView *s) {
    /* don't look in refs because we can't do a replace there */
    SplineSet *spl, *s_spl, *s_r_spl;
    SplinePoint *sp;
    RefChar *r, *s_r;
    int i, first;

    s->matched_ss = s->matched_refs = 0;
    first = true;
    for ( s_r = s->sc_srch.layers[ly_fore].refs; s_r!=NULL; s_r = s_r->next ) {
	for ( r = sc->layers[ly_fore].refs, i=0; r!=NULL; r=r->next, ++i ) if ( !(s->matched_refs&(1<<i)) ) {
	    if ( r->sc == s_r->sc ) {
		/* I should check the transform to see if the tryflips (etc) flags would make this not a match */
		if ( r->transform[0]==1 && r->transform[1]==0 &&
			r->transform[2]==0 && r->transform[3]==1 ) {
		    if ( first ) {
			s->matched_scale = 1.0;
			s->matched_x = r->transform[4];
			s->matched_y = r->transform[5];
			first = false;
	break;
		    } else if ( r->transform[4]==s->matched_x &&
			    r->transform[5] == s->matched_y )
	break;
		}
	    }
	}
	if ( r==NULL )
return( false );
	s->matched_refs |= 1<<i;
    }

    for ( s_spl = s->path, s_r_spl=s->revpath; s_spl!=NULL; s_spl=s_spl->next, s_r_spl = s_r_spl->next ) {
	for ( spl=sc->layers[ly_fore].splines, i=0; spl!=NULL; spl=spl->next, ++i ) if ( !(s->matched_ss&(1<<i)) ) {
	    s->matched_spl = spl;
	    if ( spl->first->prev==NULL ) {	/* Open */
		if ( s_spl->first!=s_spl->last ) {
		    if ( SPMatches(spl->first,s,s_spl,1-first))
	break;
		    if ( s->tryreverse && SPMatches(spl->first,s,s_r_spl,1-first)) {
			s->wasreversed = true;
	break;
		    }
		}
	    } else {
		if ( s_spl->first==s_spl->last ) {
		    int found = false;
		    for ( sp=spl->first; ; ) {
			if ( SPMatches(sp,s,s_spl,1-first)) {
			    found = true;
		    break;
			}
			if ( s->tryreverse && SPMatches(sp,s,s_r_spl,1-first)) {
			    s->wasreversed = found = true;
		    break;
			}
			sp = sp->next->to;
			if ( sp==spl->first )
		    break;
		    }
		    if ( found )
	break;
		}
	    }
	}
	if ( spl==NULL )
return( false );
	s->matched_ss |= 1<<i;
	first = false;
    }
return( true );
}

static void AdjustBP(BasePoint *changeme,BasePoint *rel,
	BasePoint *shouldbe, BasePoint *shouldberel,
	BasePoint *fudge,
	SearchView *s) {
    real xoff,yoff;

    xoff = (shouldbe->x-shouldberel->x);
    yoff = (shouldbe->y-shouldberel->y);
    if ( s->matched_flip&1 )
	xoff=-xoff;
    if ( s->matched_flip&2 )
	yoff =-yoff;
    xoff *= s->matched_scale;
    yoff *= s->matched_scale;
    changeme->x = xoff*s->matched_co - yoff*s->matched_si + fudge->x  + rel->x;
    changeme->y = yoff*s->matched_co + xoff*s->matched_si + fudge->y  + rel->y;
}

static void AdjustAll(SplinePoint *change,BasePoint *rel,
	BasePoint *shouldbe, BasePoint *shouldberel,
	BasePoint *fudge,
	SearchView *s) {
    BasePoint old;

    old = change->me;
    AdjustBP(&change->me,rel,shouldbe,shouldberel,fudge,s);
    change->nextcp.x += change->me.x-old.x; change->nextcp.y += change->me.y-old.y;
    change->prevcp.x += change->me.x-old.x; change->prevcp.y += change->me.y-old.y;
}

static SplinePoint *RplInsertSP(SplinePoint *after,SplinePoint *rpl,SearchView *s, BasePoint *fudge) {
    SplinePoint *new = chunkalloc(sizeof(SplinePoint));
    real transform[6];

    SVBuildTrans(s,transform);
    transform[4] += fudge->x; transform[5] += fudge->y;
    new->me.x = transform[0]*rpl->me.x + transform[2]*rpl->me.y + transform[4];
    new->me.y = transform[1]*rpl->me.x + transform[3]*rpl->me.y + transform[5];
    new->prevcp.x = transform[0]*rpl->prevcp.x + transform[2]*rpl->prevcp.y + transform[4];
    new->prevcp.y = transform[1]*rpl->prevcp.x + transform[3]*rpl->prevcp.y + transform[5];
    new->nextcp.x = transform[0]*rpl->nextcp.x + transform[2]*rpl->nextcp.y + transform[4];
    new->nextcp.y = transform[1]*rpl->nextcp.x + transform[3]*rpl->nextcp.y + transform[5];
    new->pointtype = rpl->pointtype;
    new->selected = true;
    if ( after->next==NULL ) {
	SplineMake(after,new,s->fv->sf->order2);
	s->matched_spl->last = new;
    } else {
	SplinePoint *nsp = after->next->to;
	after->next->to = new;
	new->prev = after->next;
	SplineRefigure(after->next);
	SplineMake(new,nsp,s->fv->sf->order2);
    }
return( new );
}

static void FudgeFigure(SplineChar *sc,SearchView *s,SplineSet *path,BasePoint *fudge) {
    SplinePoint *search, *searchrel, *found, *foundrel;
    real xoff, yoff;

    fudge->x = fudge->y = 0;
    if ( path->first->prev!=NULL )		/* closed path, should end where it began */
return;						/*  => no fudge */

    foundrel = s->matched_sp; searchrel = path->first;
    for ( found=foundrel, search=searchrel ; ; ) {
	if ( found->next==NULL || search->next==NULL )
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

static void DoReplaceIncomplete(SplineChar *sc,SearchView *s) {
    SplinePoint *sc_p, *nsc_p, *p_p, *np_p, *r_p, *nr_p;
    BasePoint fudge;
    SplineSet *path, *rpath;
    real xoff, yoff, temp;

    if ( s->wasreversed ) {
	path = s->revpath;
	rpath = s->revreplace;
    } else {
	path = s->path;
	rpath = s->replacepath;
    }

    xoff = rpath->first->me.x-path->first->me.x;
    yoff = rpath->first->me.y-path->first->me.y;
    if ( s->matched_flip&1 )
	xoff=-xoff;
    if ( s->matched_flip&2 )
	yoff =-yoff;
    xoff *= s->matched_scale;
    yoff *= s->matched_scale;
    temp = xoff*s->matched_co - yoff*s->matched_si;
    yoff = yoff*s->matched_co + xoff*s->matched_si;
    xoff = temp;

    /* Total "fudge" amount should be spread evenly over each point */
    FudgeFigure(sc,s,path,&fudge);
    if ( s->pointcnt!=s->rpointcnt )
	MinimumDistancesFree(sc->md); sc->md = NULL;

    for ( sc_p = s->matched_sp, p_p = path->first, r_p = rpath->first; ; ) {
	if ( p_p->next==NULL && r_p->next==NULL ) {
	    s->last_sp = sc_p;
return;		/* done */
	} else if ( p_p->next==NULL ) {
	    /* The search pattern is shorter that the replace pattern */
	    /*  Need to add some extra points */
	    r_p = r_p->next->to;
	    sc_p = RplInsertSP(sc_p,r_p,s,&fudge);
	} else if ( r_p->next==NULL ) {
	    /* The replace pattern is shorter than the search pattern */
	    /*  Need to remove some points */
	    nsc_p = sc_p->next->to;
	    if ( nsc_p->next==NULL ) {
		SplinePointFree(nsc_p);
		SplineFree(sc_p->next);
		sc_p->next = NULL;
		s->matched_spl->last = sc_p;
		s->last_sp = sc_p;
return;
	    } else {
		nsc_p = nsc_p->next->to;
		SplinePointFree(sc_p->next->to);
		SplineFree(sc_p->next);
		sc_p->next = nsc_p->prev;
		nsc_p->prev->from = sc_p;
		SplineRefigure(nsc_p->prev);
	    }
	    p_p = p_p->next->to; sc_p = nsc_p;
	} else {
	    if ( sc_p->next==NULL ) {
		IError("Unexpected point mismatch in replace");
return;
	    }
	    np_p = p_p->next->to; nsc_p = sc_p->next->to; nr_p = r_p->next->to;
	    if ( p_p==path->first ) {
		AdjustBP(&sc_p->nextcp,&sc_p->me,&r_p->nextcp,&r_p->me,&fudge,s);
		if ( p_p->prev!=NULL )
		    AdjustBP(&sc_p->prevcp,&sc_p->me,&r_p->prevcp,&r_p->me,&fudge,s);
		sc_p->me.x += xoff; sc_p->me.y += yoff;
		sc_p->nextcp.x += xoff; sc_p->nextcp.y += yoff;
		sc_p->prevcp.x += xoff; sc_p->prevcp.y += yoff;
		if ( sc_p->prev!=NULL )
		    SplineRefigure(sc_p->prev);
	    }
	    if ( np_p==path->first )
return;
	    if ( np_p->next!=NULL )
		AdjustBP(&nsc_p->nextcp,&nsc_p->me,&nr_p->nextcp,&nr_p->me,&fudge,s);
	    AdjustBP(&nsc_p->prevcp,&nsc_p->me,&nr_p->prevcp,&nr_p->me,&fudge,s);
	    AdjustAll(nsc_p,&sc_p->me,&nr_p->me,&r_p->me,&fudge,s);
	    nsc_p->pointtype = nr_p->pointtype;
	    if ( nsc_p->next!=NULL )
		SplineRefigure(nsc_p->next);
	    if ( nsc_p->prev!=NULL )
		SplineRefigure(nsc_p->prev);
	    p_p = np_p; sc_p = nsc_p; r_p = nr_p;
	}
    }
}

static int HeuristiclyBadMatch(SplineChar *sc,SearchView *s) {
    /* Consider the case of a closed circle and an open circle, where the */
    /*  open circle looks just like the closed except that the open circle */
    /*  has an internal counter-clockwise contour. We don't want to match here*/
    /*  we'd get a reference and a counter-clockwise contour which would make */
    /*  no sense. */
    /* so if, after removing matched contours we are left with a single counter*/
    /*  clockwise contour, don't accept the match */
    int contour_cnt, i;
    SplineSet *spl;

    contour_cnt=0;
    for ( spl=sc->layers[ly_fore].splines, i=0; spl!=NULL; spl=spl->next, ++i ) {
	if ( !(s->matched_ss&(1<<i))) {
	    if ( SplinePointListIsClockwise(spl) )
return( false );
	    ++contour_cnt;
	}
    }
    if ( contour_cnt==0 )
return( false );		/* Exact match */

    /* Everything remaining is counter-clockwise */
return( true );
}

static void DoReplaceFull(SplineChar *sc,SearchView *s) {
    int i;
    RefChar *r, *rnext, *new;
    SplinePointList *spl, *snext, *sprev, *temp;
    real transform[6], subtrans[6];
    SplinePoint *sp;

    /* first remove those bits that matched */
    for ( r = sc->layers[ly_fore].refs, i=0; r!=NULL; r=rnext, ++i ) {
	rnext = r->next;
	if ( s->matched_refs&(1<<i))
	    SCRemoveDependent(sc,r);
    }
    sprev = NULL;
    for ( spl=sc->layers[ly_fore].splines, i=0; spl!=NULL; spl=snext, ++i ) {
	snext = spl->next;
	if ( s->matched_ss&(1<<i)) {
	    if ( sprev==NULL )
		sc->layers[ly_fore].splines = snext;
	    else
		sprev->next = snext;
	    spl->next = NULL;
	    SplinePointListMDFree(sc,spl);
	} else
	    sprev = spl;
    }

    /* Then insert the replace stuff */
    SVBuildTrans(s,transform);
    for ( r = s->sc_rpl.layers[ly_fore].refs; r!=NULL; r=r->next ) {
	subtrans[0] = transform[0]*r->transform[0] + transform[1]*r->transform[2];
	subtrans[1] = transform[0]*r->transform[1] + transform[1]*r->transform[3];
	subtrans[2] = transform[2]*r->transform[0] + transform[3]*r->transform[2];
	subtrans[3] = transform[2]*r->transform[1] + transform[3]*r->transform[3];
	subtrans[4] = transform[4]*r->transform[0] + transform[5]*r->transform[2] +
		r->transform[4];
	subtrans[5] = transform[4]*r->transform[1] + transform[5]*r->transform[3] +
		r->transform[5];
	new = RefCharCreate();
	*new = *r;
	memcpy(new->transform,subtrans,sizeof(subtrans));
#ifdef FONTFORGE_CONFIG_TYPE3
	new->layers = NULL;
	new->layer_cnt = 0;
#else
	new->layers[0].splines = NULL;
#endif
	new->next = sc->layers[ly_fore].refs;
	new->selected = true;
	sc->layers[ly_fore].refs = new;
	SCReinstanciateRefChar(sc,new);
	SCMakeDependent(sc,new->sc);
    }
    temp = SplinePointListTransform(SplinePointListCopy(s->sc_rpl.layers[ly_fore].splines),transform,true);
    if ( sc->layers[ly_fore].splines==NULL )
	sc->layers[ly_fore].splines = temp;
    else {
	for ( spl=sc->layers[ly_fore].splines; spl->next!=NULL; spl = spl->next );
	spl->next = temp;
	for ( ; temp!=NULL; temp=temp->next ) {
	    for ( sp=temp->first; ; ) {
		sp->selected = true;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
		if ( sp==temp->first )
	    break;
	    }
	}
    }
}

/* ************************************************************************** */

static void SVResetPaths(SearchView *sv) {
    SplineSet *spl;

    if ( sv->sc_srch.changed_since_autosave ) {
	sv->path = sv->sc_srch.layers[ly_fore].splines;
	SplinePointListsFree(sv->revpath);
	sv->revpath = SplinePointListCopy(sv->path);
	for ( spl=sv->revpath; spl!=NULL; spl=spl->next )
	    spl = SplineSetReverse(spl);
	sv->sc_srch.changed_since_autosave = false;
    }
    if ( sv->sc_rpl.changed_since_autosave ) {
	sv->replacepath = sv->sc_rpl.layers[ly_fore].splines;
	SplinePointListsFree(sv->revreplace);
	sv->revreplace = SplinePointListCopy(sv->replacepath);
	for ( spl=sv->revreplace; spl!=NULL; spl=spl->next )
	    spl = SplineSetReverse(spl);
	sv->sc_rpl.changed_since_autosave = false;
    }
}

static int SearchChar(SearchView *sv, int gid,int startafter) {

    sv->curchar = sv->fv->sf->glyphs[gid];

    sv->wasreversed = false;
    sv->matched_rot = 0; sv->matched_scale = 1;
    sv->matched_co = 1; sv->matched_si = 0;
    sv->matched_flip = flip_none;
    sv->matched_refs = sv->matched_ss = 0;
    sv->matched_x = sv->matched_y = 0;

    if ( sv->subpatternsearch )
return( SCMatchesIncomplete(sv->curchar,sv,startafter));
    else
return( SCMatchesFull(sv->curchar,sv));
}

static int DoRpl(SearchView *sv) {
    RefChar *r;

    /* Make sure we don't generate any self referential characters... */
    for ( r = sv->sc_rpl.layers[ly_fore].refs; r!=NULL; r=r->next ) {
	if ( SCDependsOnSC(r->sc,sv->curchar))
return(false);
    }

    if ( sv->replaceall && !sv->subpatternsearch &&
	    HeuristiclyBadMatch(sv->curchar,sv))
return(false);

    SCPreserveState(sv->curchar,false);
    if ( sv->subpatternsearch )
	DoReplaceIncomplete(sv->curchar,sv);
    else
	DoReplaceFull(sv->curchar,sv);
    SCCharChangedUpdate(sv->curchar);
return( true );
}

static int _DoFindAll(SearchView *sv) {
    int i, any=0, gid;
    SplineChar *startcur = sv->curchar;

    for ( i=0; i<sv->fv->map->enccount; ++i ) {
	if (( !sv->onlyselected || sv->fv->selected[i]) && (gid=sv->fv->map->map[i])!=-1 &&
		sv->fv->sf->glyphs[gid]!=NULL ) {
	    if ( (sv->fv->selected[i] = SearchChar(sv,gid,false)) ) {
		any = true;
		if ( sv->replaceall ) {
		    do {
			if ( !DoRpl(sv))
		    break;
		    } while ( (sv->subpatternsearch || sv->replacewithref) &&
			    SearchChar(sv,gid,true));
		}
	    }
	} else
	    sv->fv->selected[i] = false;
    }
    sv->curchar = startcur;
return( any );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
static void SVSelectSC(SearchView *sv) {
    SplineChar *sc = sv->curchar;
    SplinePointList *spl;
    SplinePoint *sp;
    RefChar *rf;
    int i;

    /* Deselect all */;
    for ( spl = sc->layers[ly_fore].splines; spl!=NULL; spl = spl->next ) {
	for ( sp=spl->first ;; ) {
	    sp->selected = false;
	    if ( sp->next == NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
    }
    for ( rf=sc->layers[ly_fore].refs; rf!=NULL; rf = rf->next )
	if ( rf->selected ) rf->selected = false;

    if ( sv->subpatternsearch ) {
	spl = sv->matched_spl;
	for ( sp = sv->matched_sp; ; ) {
	    sp->selected = true;
	    if ( sp->next == NULL || sp==sv->last_sp )
	break;
	    sp = sp->next->to;
	    /* Ok to wrap back to first */
	}
    } else {
	for ( rf=sc->layers[ly_fore].refs, i=0; rf!=NULL; rf=rf->next, ++i )
	    if ( sv->matched_refs&(1<<i) )
		rf->selected = true;
	for ( spl = sc->layers[ly_fore].splines,i=0; spl!=NULL; spl = spl->next, ++i ) {
	    if ( sv->matched_ss&(1<<i) ) {
		for ( sp=spl->first ;; ) {
		    sp->selected = true;
		    if ( sp->next == NULL )
		break;
		    sp = sp->next->to;
		    if ( sp==spl->first )
		break;
		}
	    }
	}
    }
    SCUpdateAll(sc);
    sc->changed_since_search = false;
}

static int DoFindOne(SearchView *sv,int startafter) {
    int i, gid;
    SplineChar *startcur = sv->curchar;

    /* It is possible that some idiot deleted the current character since */
    /*  the last search... do some mild checks */
    if ( sv->curchar!=NULL &&
	    sv->curchar->parent == sv->fv->sf &&
	    sv->curchar->orig_pos>=0 && sv->curchar->orig_pos<sv->fv->sf->glyphcnt &&
	    sv->curchar==sv->fv->sf->glyphs[sv->curchar->orig_pos] )
	/* Looks ok */;
    else
	sv->curchar=startcur=NULL;

    if ( !sv->subpatternsearch ) startafter = false;

    if ( sv->showsfindnext && sv->curchar!=NULL )
	i = sv->fv->map->backmap[sv->curchar->orig_pos]+1-startafter;
    else {
	startafter = false;
	if ( !sv->onlyselected )
	    i = 0;
	else {
	    for ( i=0; i<sv->fv->map->enccount; ++i )
		if ( sv->fv->selected[i] && (gid=sv->fv->map->map[i])!=-1 &&
			sv->fv->sf->glyphs[gid]!=NULL )
	    break;
	}
    }
 
    for ( ; i<sv->fv->map->enccount; ++i ) {
	if (( !sv->onlyselected || sv->fv->selected[i]) && (gid=sv->fv->map->map[i])!=-1 &&
		sv->fv->sf->glyphs[gid]!=NULL )
	    if ( SearchChar(sv,gid,startafter) )
    break;
	startafter = false;
    }
    if ( i>=sv->fv->map->enccount ) {
	gwwv_post_notice(_("Not Found"),sv->showsfindnext?_("The search pattern was not found again in the font %.100s"):_("The search pattern was not found in the font %.100s"),sv->fv->sf->fontname);
	sv->curchar = startcur;
	GGadgetSetTitle8(GWidgetGetControl(sv->gw,CID_Find),_("Find"));
	sv->showsfindnext = false;
return( false );
    }
    SVSelectSC(sv);
    if ( sv->lastcv!=NULL && sv->lastcv->sc==startcur && sv->lastcv->fv==sv->fv ) {
	CVChangeSC(sv->lastcv,sv->curchar);
	GDrawSetVisible(sv->lastcv->gw,true);
	GDrawRaise(sv->lastcv->gw);
    } else
	sv->lastcv = CharViewCreate(sv->curchar,sv->fv,-1);
    GGadgetSetTitle8(GWidgetGetControl(sv->gw,CID_Find),_("Find Next"));
    sv->showsfindnext = true;
return( true );
}

static void DoFindAll(SearchView *sv) {
    int any;

    any = _DoFindAll(sv);
    GDrawRequestExpose(sv->fv->v,NULL,false);
    if ( !any )
#if defined(FONTFORGE_CONFIG_GDRAW)
	gwwv_post_notice(_("Not Found"),_("The search pattern was not found in the font %.100s"),sv->fv->sf->fontname);
#elif defined(FONTFORGE_CONFIG_GTK)
	gwwv_post_notice(_("Not Found"),_("The search pattern was not found in the font %.100s"),sv->fv->sf->fontname);
#endif
}

static void SVParseDlg(SearchView *sv) {

    sv->tryreverse = true;
    sv->tryflips = GGadgetIsChecked(GWidgetGetControl(sv->gw,CID_Flipping));
    sv->tryscale = GGadgetIsChecked(GWidgetGetControl(sv->gw,CID_Scaling));
    sv->tryrotate = GGadgetIsChecked(GWidgetGetControl(sv->gw,CID_Rotating));
    sv->onlyselected = GGadgetIsChecked(GWidgetGetControl(sv->gw,CID_Selected));

    SVResetPaths(sv);

    /* Only do a sub pattern search if we have a single path and it is open */
    /*  and there is either no replace pattern, or it is also a single open */
    /*  path */
    sv->subpatternsearch = sv->path!=NULL && sv->path->next==NULL &&
	    sv->path->first->prev==NULL && sv->sc_srch.layers[ly_fore].refs==NULL;
    if ( sv->replacepath!=NULL && (sv->replacepath->next!=NULL ||
	    sv->replacepath->first->prev!=NULL ))
	sv->subpatternsearch = false;
    else if ( sv->sc_rpl.layers[ly_fore].refs!=NULL )
	sv->subpatternsearch = false;

    if ( sv->subpatternsearch ) {
	int i;
	SplinePoint *sp;
	for ( sp=sv->path->first, i=0; ; ) {
	    ++i;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	}
	sv->pointcnt = i;
	if ( sv->replacepath!=NULL ) {
	    for ( sp=sv->replacepath->first, i=0; ; ) {
		++i;
		if ( sp->next==NULL )
	    break;
		sp = sp->next->to;
	    }
	    sv->rpointcnt = i;
	}
    }
    sv->fudge = sv->tryrotate ? .01 : .001;
    sv->fudge_percent = sv->tryrotate ? .01 : .001;
}

static int SV_Find(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SearchView *sv = (SearchView *) GDrawGetUserData(GGadgetGetWindow(g));
	SVParseDlg(sv);
	sv->findall = sv->replaceall = false;
	DoFindOne(sv,false);
    }
return( true );
}

static int SV_FindAll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SearchView *sv = (SearchView *) GDrawGetUserData(GGadgetGetWindow(g));
	SVParseDlg(sv);
	sv->findall = true;
	sv->replaceall = false;
	DoFindAll(sv);
    }
return( true );
}

static int SV_RplFind(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SearchView *sv = (SearchView *) GDrawGetUserData(GGadgetGetWindow(g));
	RefChar *rf;
	SVParseDlg(sv);
	sv->findall = sv->replaceall = false;
	for ( rf=sv->sc_rpl.layers[ly_fore].refs; rf!=NULL; rf = rf->next ) {
	    if ( SCDependsOnSC(rf->sc,sv->curchar)) {
#if defined(FONTFORGE_CONFIG_GDRAW)
		gwwv_post_error(_("Self-referential glyph"),_("Attempt to make a glyph that refers to itself"));
#elif defined(FONTFORGE_CONFIG_GTK)
		gwwv_post_error(_("Self-referential character"),_("Attempt to make a character that refers to itself"));
#endif
return( true );
	    }
	}
	DoRpl(sv);
	DoFindOne(sv,sv->subpatternsearch);
    }
return( true );
}

static int SV_RplAll(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate ) {
	SearchView *sv = (SearchView *) GDrawGetUserData(GGadgetGetWindow(g));
	SVParseDlg(sv);
	sv->findall = false;
	sv->replaceall = true;
	DoFindAll(sv);
    }
return( true );
}

/* ************************************************************************** */

void SVMenuClose(GWindow gw,struct gmenuitem *mi,GEvent *e) {
    GDrawSetVisible(gw,false);
}

static int SV_Cancel(GGadget *g, GEvent *e) {
    if ( e->type==et_controlevent && e->u.control.subtype == et_buttonactivate )
	SVMenuClose(GGadgetGetWindow(g),NULL,e);
return( true );
}

static void SVResize(SearchView *sv, GEvent *event) {
    GRect size;
    int i, xoff, yoff;
    int width, height;

    if ( !event->u.resize.sized )
return;

    width = (event->u.resize.size.width-40)/2;
    height = (event->u.resize.size.height-sv->cv_y-sv->button_height-8);
    if ( width<70 || height<80 || event->u.resize.size.width<sv->button_width+10 ) {
	if ( width<70 ) width = 70;
	width = 2*width+40;
	if ( width<sv->button_width+10 ) width = sv->button_width+10;
	if ( height<80 ) height = 80;
	height += sv->cv_y+sv->button_height+8;
	GDrawResize(sv->gw,width,height);
return;
    }
    if ( width!=sv->cv_width || height!=sv->cv_height ) {
	GDrawResize(sv->cv_srch.gw,width,height);
	GDrawResize(sv->cv_rpl.gw,width,height);
	sv->cv_width = width; sv->cv_height = height;
	sv->rpl_x = 30+width;
	GDrawMove(sv->cv_rpl.gw,sv->rpl_x,sv->cv_y);
    }

    GGadgetGetSize(GWidgetGetControl(sv->gw,CID_Allow),&size);
    yoff = event->u.resize.size.height-sv->button_height-size.y;
    if ( yoff!=0 ) {
	for ( i=CID_Allow; i<=CID_Cancel; ++i ) {
	    GGadgetGetSize(GWidgetGetControl(sv->gw,i),&size);
	    GGadgetMove(GWidgetGetControl(sv->gw,i),size.x,size.y+yoff);
	}
    }
    xoff = (event->u.resize.size.width - sv->button_width)/2;
    GGadgetGetSize(GWidgetGetControl(sv->gw,CID_Find),&size);
    xoff -= size.x;
    if ( xoff!=0 ) {
	for ( i=CID_Find; i<=CID_Cancel; ++i ) {
	    GGadgetGetSize(GWidgetGetControl(sv->gw,i),&size);
	    GGadgetMove(GWidgetGetControl(sv->gw,i),size.x+xoff,size.y);
	}
    }
    GDrawSync(NULL);
    GDrawProcessPendingEvents(NULL);
    GDrawRequestExpose(sv->gw,NULL,false);
}

void SVMakeActive(SearchView *sv,CharView *cv) {
    GRect r;
    if ( sv==NULL )
return;
    sv->cv_srch.inactive = sv->cv_rpl.inactive = true;
    cv->inactive = false;
    GDrawRequestExpose(sv->cv_srch.v,NULL,false);
    GDrawRequestExpose(sv->cv_rpl.v,NULL,false);
    GDrawGetSize(sv->gw,&r);
    r.x = 0;
    r.y = sv->mbh;
    r.height = sv->fh+10;
    GDrawRequestExpose(sv->gw,&r,false);
}

void SVChar(SearchView *sv, GEvent *event) {
    if ( event->u.chr.keysym==GK_Tab || event->u.chr.keysym==GK_BackTab )
	SVMakeActive(sv,sv->cv_srch.inactive ? &sv->cv_srch : &sv->cv_rpl);
    else
	CVChar(sv->cv_srch.inactive ? &sv->cv_rpl : &sv->cv_srch,event);
}

static void SVDraw(SearchView *sv, GWindow pixmap, GEvent *event) {
    GRect r;

    GDrawSetLineWidth(pixmap,0);
    if ( sv->cv_srch.inactive )
	GDrawSetFont(pixmap,sv->plain);
    else
	GDrawSetFont(pixmap,sv->bold);
    GDrawDrawText8(pixmap,10,sv->mbh+5+sv->as,
	    _("Search Pattern:"),-1,NULL,0);
    if ( sv->cv_rpl.inactive )
	GDrawSetFont(pixmap,sv->plain);
    else
	GDrawSetFont(pixmap,sv->bold);
    GDrawDrawText8(pixmap,sv->rpl_x,sv->mbh+5+sv->as,
	    _("Replace Pattern:"),-1,NULL,0);
    r.x = 10-1; r.y=sv->cv_y-1;
    r.width = sv->cv_width+1; r.height = sv->cv_height+1;
    GDrawDrawRect(pixmap,&r,0);
    r.x = sv->rpl_x-1;
    GDrawDrawRect(pixmap,&r,0);
}

static void SVCheck(SearchView *sv) {
    int show = ( sv->sc_srch.layers[ly_fore].splines!=NULL || sv->sc_srch.layers[ly_fore].refs!=NULL );
    int showrplall=show, showrpl;

    if ( sv->sc_srch.changed_since_autosave && sv->showsfindnext ) {
	GGadgetSetTitle8(GWidgetGetControl(sv->gw,CID_Find),_("Find"));
	sv->showsfindnext = false;
    }
    if ( showrplall ) {
	if ( sv->sc_srch.layers[ly_fore].splines!=NULL && sv->sc_srch.layers[ly_fore].splines->next==NULL &&
		sv->sc_srch.layers[ly_fore].splines->first->prev==NULL &&
		sv->sc_rpl.layers[ly_fore].splines==NULL && sv->sc_rpl.layers[ly_fore].refs==NULL )
	    showrplall = false;
    }
    showrpl = showrplall;
    if ( !sv->showsfindnext || sv->curchar==NULL || sv->curchar->parent!=sv->fv->sf ||
	    sv->curchar->orig_pos<0 || sv->curchar->orig_pos>=sv->fv->sf->glyphcnt ||
	    sv->curchar!=sv->fv->sf->glyphs[sv->curchar->orig_pos] ||
	    sv->curchar->changed_since_search )
	showrpl = false;

    if ( sv->findenabled != show ) {
	GGadgetSetEnabled(GWidgetGetControl(sv->gw,CID_Find),show);
	GGadgetSetEnabled(GWidgetGetControl(sv->gw,CID_FindAll),show);
	sv->findenabled = show;
    }
    if ( sv->rplallenabled != showrplall ) {
	GGadgetSetEnabled(GWidgetGetControl(sv->gw,CID_ReplaceAll),showrplall);
	sv->rplallenabled = showrplall;
    }
    if ( sv->rplenabled != showrpl ) {
	GGadgetSetEnabled(GWidgetGetControl(sv->gw,CID_Replace),showrpl);
	sv->rplenabled = showrpl;
    }
}

static void SearchViewFree(SearchView *sv) {
    SplinePointListsFree(sv->sc_srch.layers[ly_fore].splines);
    SplinePointListsFree(sv->sc_rpl.layers[ly_fore].splines);
    RefCharsFree(sv->sc_srch.layers[ly_fore].refs);
    RefCharsFree(sv->sc_rpl.layers[ly_fore].refs);
    free(sv);
}

static int sv_e_h(GWindow gw, GEvent *event) {
    SearchView *sv = (SearchView *) GDrawGetUserData(gw);

    switch ( event->type ) {
      case et_expose:
	SVDraw(sv,gw,event);
      break;
      case et_resize:
	if ( event->u.resize.sized )
	    SVResize(sv,event);
      break;
      case et_char:
	SVChar(sv,event);
      break;
      case et_timer:
	SVCheck(sv);
      break;
      case et_close:
	SVMenuClose(gw,NULL,NULL);
      break;
      case et_create:
      break;
      case et_destroy:
	SearchViewFree(sv);
      break;
      case et_map:
	if ( event->u.map.is_visible )
	    CVPaletteActivate(sv->cv_srch.inactive?&sv->cv_rpl:&sv->cv_srch);
	else
	    CVPalettesHideIfMine(sv->cv_srch.inactive?&sv->cv_rpl:&sv->cv_srch);
	sv->isvisible = event->u.map.is_visible;
      break;
    }
return( true );
}

static void SVSetTitle(SearchView *sv) {
    char ubuf[150];
    sprintf(ubuf,_("Find in %.100s"),sv->fv->sf->fontname);
    GDrawSetWindowTitles8(sv->gw,ubuf,_("Find"));
}

int SVAttachFV(FontView *fv,int ask_if_difficult) {
    int i, doit, pos, any=0, gid;
    RefChar *r, *rnext, *rprev;

    if ( searcher==NULL )
return( false );

    if ( searcher->fv==fv )
return( true );
    if ( searcher->fv!=NULL && searcher->fv->sf==fv->sf ) {
	searcher->fv->sv = NULL;
	fv->sv = searcher;
	searcher->fv = fv;
	SVSetTitle(searcher);
	searcher->curchar = NULL;
return( true );
    }

    if ( searcher->dummy_sf.order2 != fv->sf->order2 ) {
	SCClearContents(&searcher->sc_srch);
	SCClearContents(&searcher->sc_rpl);
	for ( i=0; i<searcher->sc_srch.layer_cnt; ++i )
	    UndoesFree(searcher->sc_srch.layers[i].undoes);
	for ( i=0; i<searcher->sc_rpl.layer_cnt; ++i )
	    UndoesFree(searcher->sc_rpl.layers[i].undoes);
    }

    for ( doit=!ask_if_difficult; doit<=1; ++doit ) {
	for ( i=0; i<2; ++i ) {
	    rprev = NULL;
	    for ( r = searcher->chars[i]->layers[ly_fore].refs; r!=NULL; r=rnext ) {
		rnext = r->next;
		pos = SFFindSlot(fv->sf,fv->map,r->sc->unicodeenc,r->sc->name);
		if ( pos==-1 && !doit ) {
#if defined(FONTFORGE_CONFIG_GDRAW)
		    char *buttons[3];
		    buttons[0] = _("Yes");
		    buttons[1] = _("Cancel");
		    buttons[2] = NULL;
#elif defined(FONTFORGE_CONFIG_GTK)
		    static char *buttons[] = { GTK_STOCK_YES, GTK_STOCK_CANCEL, NULL };
#endif
		    if ( ask_if_difficult==2 && !searcher->isvisible )
return( false );
		    if ( gwwv_ask(_("Bad Reference"),(const char **) buttons,1,1,
			    _("The %1$s contains a reference to %2$.20hs which does not exist in the new font.\nShould I remove the reference?"),
				i==0?_("Search Pattern:"):_("Replace Pattern:"),
			        r->sc->name)==1 )
return( false );
		} else if ( !doit )
		    /* Do Nothing */;
		else if ( pos==-1 ) {
		    if ( rprev==NULL )
			searcher->chars[i]->layers[ly_fore].refs = rnext;
		    else
			rprev->next = rnext;
		    RefCharFree(r);
		    any = true;
		} else {
		    /*SplinePointListsFree(r->layers[0].splines); r->layers[0].splines = NULL;*/
		    gid = fv->map->map[pos];
		    r->sc = fv->sf->glyphs[gid];
		    r->orig_pos = gid;
		    SCReinstanciateRefChar(searcher->chars[i],r);
		    any = true;
		    rprev = r;
		}
	    }
	}
    }
    fv->sv = searcher;
    searcher->fv = fv;
    searcher->curchar = NULL;
    if ( any ) {
	GDrawRequestExpose(searcher->cv_srch.v,NULL,false);
	GDrawRequestExpose(searcher->cv_rpl.v,NULL,false);
    }
    SVSetTitle(searcher);
return( true );
}

void SVDetachFV(FontView *fv) {
    FontView *other;

    fv->sv = NULL;
    if ( searcher==NULL || searcher->fv!=fv )
return;
    SVMenuClose(searcher->gw,NULL,NULL);
    for ( other=fv_list; other!=NULL; other=other->next ) {
	if ( other!=fv ) {
	    SVAttachFV(fv,false);
return;
	}
    }
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static SearchView *SVFillup(SearchView *sv, FontView *fv) {

    sv->sc_srch.orig_pos = 0; sv->sc_srch.unicodeenc = -1; sv->sc_srch.name = "Search";
    sv->sc_rpl.orig_pos = 1; sv->sc_rpl.unicodeenc = -1; sv->sc_rpl.name = "Replace";
    sv->sc_srch.layer_cnt = sv->sc_rpl.layer_cnt = 2;
#ifdef FONTFORGE_CONFIG_TYPE3
    sv->sc_srch.layers = gcalloc(2,sizeof(Layer));
    sv->sc_rpl.layers = gcalloc(2,sizeof(Layer));
    LayerDefault(&sv->sc_srch.layers[0]);
    LayerDefault(&sv->sc_srch.layers[1]);
    LayerDefault(&sv->sc_rpl.layers[0]);
    LayerDefault(&sv->sc_rpl.layers[1]);
#endif
    sv->chars[0] = &sv->sc_srch;
    sv->chars[1] = &sv->sc_rpl;
    sv->dummy_sf.glyphs = sv->chars;
    sv->dummy_sf.glyphcnt = sv->dummy_sf.glyphmax = 2;
    sv->dummy_sf.pfminfo.fstype = -1;
    sv->dummy_sf.fontname = sv->dummy_sf.fullname = sv->dummy_sf.familyname = "dummy";
    sv->dummy_sf.weight = "Medium";
    sv->dummy_sf.origname = "dummy";
    sv->dummy_sf.ascent = fv->sf->ascent;
    sv->dummy_sf.descent = fv->sf->descent;
    sv->dummy_sf.order2 = fv->sf->order2;
    sv->dummy_sf.anchor = fv->sf->anchor;
    sv->sc_srch.width = sv->sc_srch.vwidth = sv->sc_rpl.vwidth = sv->sc_rpl.width =
	    sv->dummy_sf.ascent + sv->dummy_sf.descent;
    sv->sc_srch.parent = sv->sc_rpl.parent = &sv->dummy_sf;

    sv->dummy_sf.fv = &sv->dummy_fv;
    sv->dummy_fv.sf = &sv->dummy_sf;
    sv->dummy_fv.selected = sv->sel;
    sv->dummy_fv.cbw = sv->dummy_fv.cbh = default_fv_font_size+1;
    sv->dummy_fv.magnify = 1;

    sv->fv = fv;
    if ( fv!=NULL )
	fv->sv = sv;
return( sv );
}

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
SearchView *SVCreate(FontView *fv) {
    SearchView *sv;
    GRect pos, size;
    GWindow gw;
    GWindowAttrs wattrs;
    GGadgetCreateData gcd[12];
    GTextInfo label[12];
    FontRequest rq;
    int as, ds, ld;
    static unichar_t helv[] = { 'h', 'e', 'l', 'v', 'e', 't', 'i', 'c', 'a',',','c','a','l','i','b','a','n',',','c','l','e','a','r','l','y','u',',','u','n','i','f','o','n','t',  '\0' };

    if ( searcher!=NULL ) {
	if ( SVAttachFV(fv,true)) {
	    GDrawSetVisible(fv->sv->gw,true);
	    GDrawRaise(fv->sv->gw);
return( searcher );
	} else
return( NULL );
    }

    searcher = sv = SVFillup( gcalloc(1,sizeof(SearchView)), fv );

    memset(&wattrs,0,sizeof(wattrs));
    wattrs.mask = wam_events|wam_cursor|wam_isdlg/*|wam_icon*/;
    wattrs.is_dlg = true;
    wattrs.event_masks = -1;
    wattrs.event_masks = -1;
    wattrs.cursor = ct_pointer;
    /*wattrs.icon = icon;*/
    pos.width = 600;
    pos.height = 400;
    pos.x = GGadgetScale(104)+6;
    DefaultY(&pos);
    sv->gw = gw = GDrawCreateTopWindow(NULL,&pos,sv_e_h,sv,&wattrs);
    SVSetTitle(sv);

    memset(&rq,0,sizeof(rq));
    rq.family_name = helv;
    rq.point_size = 12;
    rq.weight = 400;
    sv->plain = GDrawInstanciateFont(NULL,&rq);
    rq.weight = 700;
    sv->bold = GDrawInstanciateFont(NULL,&rq);
    GDrawFontMetrics(sv->plain,&as,&ds,&ld);
    sv->fh = as+ds; sv->as = as;

    SVCharViewInits(sv);

    memset(&label,0,sizeof(label));
    memset(&gcd,0,sizeof(gcd));

    label[0].text = (unichar_t *) _("Allow:");
    label[0].text_is_1byte = true;
    gcd[0].gd.label = &label[0];
    gcd[0].gd.pos.x = 5; gcd[0].gd.pos.y = GDrawPixelsToPoints(NULL,sv->cv_y+sv->cv_height+8);
    gcd[0].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[0].gd.cid = CID_Allow;
    gcd[0].gd.popup_msg = (unichar_t *) _("Allow a match even if the search pattern has\nto be transformed by a combination of the\nfollowing transformations.");
    gcd[0].creator = GLabelCreate;

    label[1].text = (unichar_t *) _("Flipping");
    label[1].text_is_1byte = true;
    gcd[1].gd.label = &label[1];
    gcd[1].gd.pos.x = 35; gcd[1].gd.pos.y = gcd[0].gd.pos.y-3;
    gcd[1].gd.flags = gg_enabled|gg_visible|gg_cb_on|gg_utf8_popup;
    gcd[1].gd.cid = CID_Flipping;
    gcd[1].gd.popup_msg = (unichar_t *) _("Allow a match even if the search pattern has\nto be transformed by a combination of the\nfollowing transformations.");
    gcd[1].creator = GCheckBoxCreate;

    label[2].text = (unichar_t *) _("Scaling");
    label[2].text_is_1byte = true;
    gcd[2].gd.label = &label[2];
    gcd[2].gd.pos.x = 100; gcd[2].gd.pos.y = gcd[1].gd.pos.y; 
    gcd[2].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[2].gd.cid = CID_Scaling;
    gcd[2].gd.popup_msg = (unichar_t *) _("Allow a match even if the search pattern has\nto be transformed by a combination of the\nfollowing transformations.");
    gcd[2].creator = GCheckBoxCreate;

    label[3].text = (unichar_t *) _("Rotating");
    label[3].text_is_1byte = true;
    gcd[3].gd.label = &label[3];
    gcd[3].gd.pos.x = 170; gcd[3].gd.pos.y = gcd[1].gd.pos.y;
    gcd[3].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[3].gd.cid = CID_Rotating;
    gcd[3].gd.popup_msg = (unichar_t *) _("Allow a match even if the search pattern has\nto be transformed by a combination of the\nfollowing transformations.");
    gcd[3].creator = GCheckBoxCreate;

    label[4].text = (unichar_t *) _("Search Selected Chars");
    label[4].text_is_1byte = true;
    gcd[4].gd.label = &label[4];
    gcd[4].gd.pos.x = 5; gcd[4].gd.pos.y = gcd[1].gd.pos.y+18;
    gcd[4].gd.flags = gg_enabled|gg_visible|gg_utf8_popup;
    gcd[4].gd.cid = CID_Selected;
    gcd[4].gd.popup_msg = (unichar_t *) _("Only search selected characters in the fontview.\nNormally we search all characters in the font.");
    gcd[4].creator = GCheckBoxCreate;

    label[5].text = (unichar_t *) _("Find Next");	/* Start with this to allow sufficient space */
    label[5].text_is_1byte = true;
    gcd[5].gd.label = &label[5];
    gcd[5].gd.pos.x = 5; gcd[5].gd.pos.y = gcd[4].gd.pos.y+24;
    gcd[5].gd.flags = gg_visible|gg_but_default;
    gcd[5].gd.cid = CID_Find;
    gcd[5].gd.handle_controlevent = SV_Find;
    gcd[5].creator = GButtonCreate;

    label[6].text = (unichar_t *) _("Find All");
    label[6].text_is_1byte = true;
    gcd[6].gd.label = &label[6];
    gcd[6].gd.pos.x = 0; gcd[6].gd.pos.y = gcd[5].gd.pos.y+3;
    gcd[6].gd.flags = gg_visible;
    gcd[6].gd.cid = CID_FindAll;
    gcd[6].gd.handle_controlevent = SV_FindAll;
    gcd[6].creator = GButtonCreate;

    label[7].text = (unichar_t *) _("Replace");
    label[7].text_is_1byte = true;
    gcd[7].gd.label = &label[7];
    gcd[7].gd.pos.x = 0; gcd[7].gd.pos.y = gcd[6].gd.pos.y;
    gcd[7].gd.flags = gg_visible;
    gcd[7].gd.cid = CID_Replace;
    gcd[7].gd.handle_controlevent = SV_RplFind;
    gcd[7].creator = GButtonCreate;

    label[8].text = (unichar_t *) _("Replace All");
    label[8].text_is_1byte = true;
    gcd[8].gd.label = &label[8];
    gcd[8].gd.pos.x = 0; gcd[8].gd.pos.y = gcd[6].gd.pos.y;
    gcd[8].gd.flags = gg_visible;
    gcd[8].gd.cid = CID_ReplaceAll;
    gcd[8].gd.handle_controlevent = SV_RplAll;
    gcd[8].creator = GButtonCreate;

    label[9].text = (unichar_t *) _("_Cancel");
    label[9].text_is_1byte = true;
    label[9].text_in_resource = true;
    gcd[9].gd.label = &label[9];
    gcd[9].gd.pos.x = 0; gcd[9].gd.pos.y = gcd[6].gd.pos.y;
    gcd[9].gd.flags = gg_enabled|gg_visible|gg_but_cancel;
    gcd[9].gd.cid = CID_Cancel;
    gcd[9].gd.handle_controlevent = SV_Cancel;
    gcd[9].creator = GButtonCreate;

    GGadgetsCreate(gw,gcd);

    GGadgetSetTitle8(GWidgetGetControl(gw,CID_Find),_("Find"));
    sv->showsfindnext = false;
    GDrawRequestTimer(gw,1000,1000,NULL);

    GGadgetGetSize(GWidgetGetControl(gw,CID_Cancel),&size);
    pos.height = size.y+size.height+13;
    pos.width = size.x+size.width+5;
    if ( sv->rpl_x+sv->cv_width+20 > pos.width )
	pos.width = sv->rpl_x+sv->cv_width+20;
    GDrawResize(gw,pos.width,pos.height);
    sv->button_height = pos.height-(sv->cv_y+sv->cv_height+8);
    sv->button_width = size.x+size.width-5;

    GDrawSetVisible(sv->gw,true);
return( sv );
}
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */

static int IsASingleReferenceOrEmpty(SplineChar *sc) {
    int i, empty = true;

    for ( i = ly_fore; i<sc->layer_cnt; ++i ) {
	if ( sc->layers[i].splines!=NULL )
return( false );
	if ( sc->layers[i].images!=NULL )
return( false );
	if ( sc->layers[i].refs!=NULL ) {
	    if ( !empty )
return( false );
	    if ( sc->layers[i].refs->next!=NULL )
return( false );
	    empty = false;
	}
    }

return( true );
}

static void CV2SC(CharView *cv, SplineChar *sc, SearchView *sv) {
    cv->sc = sc;
    cv->layerheads[dm_fore] = &sc->layers[ly_fore];
    cv->layerheads[dm_back] = &sc->layers[ly_back];
    cv->layerheads[dm_grid] = NULL;
    cv->drawmode = dm_fore;
    cv->searcher = sv;
}

static void SVCopyToCV(FontView *fv,int i,CharView *cv,int full) {
    fv->selected[i] = true;
    FVCopy(fv,full);
    SCClearContents(cv->sc);
    PasteToCV(cv);
}

void FVReplaceOutlineWithReference( FontView *fv, double fudge ) {
    SearchView *sv;
    uint8 *selected, *changed;
    SplineFont *sf = fv->sf;
    int i, j, selcnt = 0, gid;
    SearchView *oldsv = fv->sv;

#if defined(FONTFORGE_CONFIG_GDRAW)
    if ( fv->v!=NULL )
	GDrawSetCursor(fv->v,ct_watch);
#elif defined(FONTFORGE_CONFIG_GTK)
    /* !!!! */
#endif

    sv = SVFillup( gcalloc(1,sizeof(SearchView)), fv);
    sv->fudge_percent = .001;
    sv->fudge = fudge;
    CV2SC(&sv->cv_srch,&sv->sc_srch,sv);
    CV2SC(&sv->cv_rpl,&sv->sc_rpl,sv);
    sv->replaceall = true;
    sv->replacewithref = true;

    selected = galloc(fv->map->enccount);
    memcpy(selected,fv->selected,fv->map->enccount);
    changed = gcalloc(fv->map->enccount,1);

    selcnt = 0;
    for ( i=0; i<fv->map->enccount; ++i ) if ( selected[i] && (gid=fv->map->map[i])!=-1 &&
	    sf->glyphs[gid]!=NULL )
	++selcnt;
#if defined(FONTFORGE_CONFIG_GDRAW)
    gwwv_progress_start_indicator(10,_("Replace with Reference"),
	    _("Replace with Reference"),0,selcnt,1);
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_start_indicator(10,_("Replace with Reference"),
	    _("Replace Outline with Reference",0,selcnt,1);
#endif

    for ( i=0; i<fv->map->enccount; ++i ) if ( selected[i] && (gid=fv->map->map[i])!=-1 &&
	    sf->glyphs[gid]!=NULL ) {
	if ( IsASingleReferenceOrEmpty(sf->glyphs[gid]))
    continue;		/* No point in replacing something which is itself a ref with a ref to a ref */
	memset(fv->selected,0,fv->map->enccount);
	SVCopyToCV(fv,i,&sv->cv_srch,true);
	SVCopyToCV(fv,i,&sv->cv_rpl,false);
	sv->sc_srch.changed_since_autosave = sv->sc_rpl.changed_since_autosave = true;
	SVResetPaths(sv);
	if ( !_DoFindAll(sv) && selcnt==1 )
#if defined(FONTFORGE_CONFIG_GDRAW)
	    gwwv_post_notice(_("Not Found"),_("The outlines of glyph %2$.30s were not found in the font %1$.60s"),
		    sf->fontname,sf->glyphs[gid]->name);
#elif defined(FONTFORGE_CONFIG_GTK)
	    gwwv_post_notice(_("Not Found"),_("The outlines of glyph %2$.30s were not found in the font %1$.60s"),
		    sf->fontname,sf->glyphs[gid]->name);
#endif
	for ( j=0; j<fv->map->enccount; ++j )
	    if ( fv->selected[j] )
		changed[j] = 1;
	CopyBufferFree();
#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
#if defined(FONTFORGE_CONFIG_GDRAW)
	if ( !gwwv_progress_next())
#elif defined(FONTFORGE_CONFIG_GTK)
	if ( !gwwv_progress_next())
#endif
    break;
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
    }
#if defined(FONTFORGE_CONFIG_GDRAW)
    gwwv_progress_end_indicator();
#elif defined(FONTFORGE_CONFIG_GTK)
    gwwv_progress_end_indicator();
#endif

    fv->sv = oldsv;

    SCClearContents(&sv->sc_srch);
    SCClearContents(&sv->sc_rpl);
    for ( i=0; i<sv->sc_srch.layer_cnt; ++i )
	UndoesFree(sv->sc_srch.layers[i].undoes);
    for ( i=0; i<sv->sc_rpl.layer_cnt; ++i )
	UndoesFree(sv->sc_rpl.layers[i].undoes);
#ifdef FONTFORGE_CONFIG_TYPE3
    free(sv->sc_srch.layers);
    free(sv->sc_rpl.layers);
#endif
    SplinePointListsFree(sv->revpath);
    free(sv);

    free(selected);
    memcpy(fv->selected,changed,fv->map->enccount);
    free(changed);

#ifndef FONTFORGE_CONFIG_NO_WINDOWING_UI
    if ( fv->v!=NULL ) {
	GDrawRequestExpose(fv->v,NULL,false);
	GDrawSetCursor(fv->v,ct_pointer);
    }
#endif		/* FONTFORGE_CONFIG_NO_WINDOWING_UI */
}
