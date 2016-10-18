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
#include "fontforgevw.h"
#include "fvfonts.h"
#include <math.h>
#include <ustring.h>
#include <utype.h>
#include "search.h"

static int CoordMatches(real real_off, real search_off, SearchData *s) {
    if ( real_off >= search_off-s->fudge && real_off <= search_off+s->fudge )
return( true );
    real fudge = fabs(s->fudge_percent*search_off);
    return real_off >= search_off-fudge && real_off <= search_off+fudge;
}

static int BPMatches(BasePoint *sc_p1,BasePoint *sc_p2,BasePoint *p_p1,BasePoint *p_p2,
	int flip,real rot, real scale,SearchData *s) {
    real sxoff = sc_p1->x-sc_p2->x;
    real syoff = sc_p1->y-sc_p2->y;
    real pxoff = p_p1->x-p_p2->x;
    real pyoff = p_p1->y-p_p2->y;
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

static int SPMatchesF(SplinePoint *sp, SearchData *s, SplineSet *path,
	SplinePoint *sc_path_first, int substring ) {
    SplinePoint *sc_sp, *nsc_sp, *p_sp, *np_sp;
    int flip, flipmax;
    bigreal rot, scale;
    int saw_sc_first = false, first_of_path;
    BasePoint p_unit, pend_unit, sc_unit;
    bigreal len, temp;

    s->matched_sp = sp;
    s->matched_rot = 0;
    s->matched_scale = 1.0;
    s->matched_flip = flip_none;
    s->matched_co = 1; s->matched_si=0;

    first_of_path = true;
    p_sp = path->first;
    if ( s->endpoints ) {
	SplinePoint *p_prevsp = p_sp;
	SplinePoint *psc_sp;
	p_sp = p_sp->next->to;
	p_unit.x = p_sp->me.x - p_prevsp->me.x; p_unit.y = p_sp->me.y - p_prevsp->me.y;
	len = sqrt(p_unit.x*p_unit.x + p_unit.y*p_unit.y);
	if ( len==0 )
return( false );
	p_unit.x /= len; p_unit.y /= len;
	if ( sp->prev==NULL )
return( false );
	psc_sp = sp->prev->from;
	if ( (sp->me.x-psc_sp->me.x)*(sp->me.x-psc_sp->me.x) +
		(sp->me.y-psc_sp->me.y)*(sp->me.y-psc_sp->me.y) < len*len )
return( false );
    }
    if ( s->endpoints ) {
	SplinePoint *p_nextsp = path->last;
	SplinePoint *p_end = p_nextsp->prev->from;
	if ( sp->next==NULL )
return( false );
	for ( p_end = p_sp, p_nextsp = p_end->next->to, sc_sp = sp, nsc_sp=sp->next->to ;; ) {
	    if ( p_nextsp->next==NULL )
	break;
	    if ( nsc_sp->next==NULL )
return( false );
	    p_end = p_nextsp;
	    sc_sp = nsc_sp;
	    p_nextsp = p_nextsp->next->to;
	    nsc_sp = nsc_sp->next->to;
	}
	pend_unit.x = p_nextsp->me.x - p_end->me.x; pend_unit.y = p_nextsp->me.y - p_end->me.y;
	len = sqrt(pend_unit.x*pend_unit.x + pend_unit.y*pend_unit.y);
	if ( len==0 )
return( false );
	pend_unit.x /= len; pend_unit.y /= len;
	if ( (sp->me.x-nsc_sp->me.x)*(sp->me.x-nsc_sp->me.x) +
		(sp->me.y-nsc_sp->me.y)*(sp->me.y-nsc_sp->me.y) < len*len )
return( false );
    }

/* ******************* Match with no transformations applied **************** */
    first_of_path = true;
    for (sc_sp=sp; ; ) {
	if ( sc_sp->ticked )		/* Don't search within stuff we have just replaced */
return( false );

	if ( p_sp->next==NULL ) {
	    if ( substring || sc_sp->next==NULL ) {
		s->last_sp = saw_sc_first ? NULL : sp;
return( true );
	    }
    break;
	}
	np_sp = p_sp->next->to;
	if ( sc_sp->next==NULL )
    break;
	nsc_sp = sc_sp->next->to;

	if ( first_of_path && s->endpoints ) {
	    SplinePoint *sc_prevsp;
	    if ( sc_sp->prev==NULL )
return( false );
	    sc_prevsp = sc_sp->prev->from;
	    if ( !p_sp->noprevcp ) {
		if ( sc_sp->noprevcp )
return( false );
		if ( !CoordMatches(sc_sp->prevcp.x-sc_sp->me.x,p_sp->prevcp.x-p_sp->me.x,s) ||
		     !CoordMatches(sc_sp->prevcp.y-sc_sp->me.y,p_sp->prevcp.y-p_sp->me.y,s) )
    break;	/* prev control points do not match, give up */
	    } else {
		if ( !sc_sp->noprevcp )
return( false );
		sc_unit.x = sc_sp->me.x - sc_prevsp->me.x; sc_unit.y = sc_sp->me.y - sc_prevsp->me.y;
		len = sqrt(sc_unit.x*sc_unit.x + sc_unit.y*sc_unit.y );
		if ( len==0 )
return( false );
		sc_unit.x /= len; sc_unit.y /= len;
		if ( !RealNear(sc_unit.x,p_unit.x) || !RealNear(sc_unit.y,p_unit.y))
    break;
	    }
	    first_of_path = false;
	}
	if ( np_sp->next==NULL && s->endpoints ) {
	    SplinePoint *sc_nextsp;
	    if ( sc_sp->next==NULL )
return( false );
	    sc_nextsp = sc_sp->next->to;
	    if ( !p_sp->nonextcp ) {
		if ( !CoordMatches(sc_sp->nextcp.x-sc_sp->me.x,p_sp->nextcp.x-p_sp->me.x,s) ||
		     !CoordMatches(sc_sp->nextcp.y-sc_sp->me.y,p_sp->nextcp.y-p_sp->me.y,s) )
    break;	/* prev control points do not match, give up */
	    } else {
		if ( !sc_sp->nonextcp )
return( false );
		sc_unit.x = sc_nextsp->me.x - sc_sp->me.x; sc_unit.y = sc_nextsp->me.y - sc_sp->me.y;
		len = sqrt(sc_unit.x*sc_unit.x + sc_unit.y*sc_unit.y );
		if ( len==0 )
return( false );
		sc_unit.x /= len; sc_unit.y /= len;
		if ( RealNear(sc_unit.x,pend_unit.x) && RealNear(sc_unit.y,pend_unit.y)) {
		    s->last_sp = saw_sc_first ? NULL : sp;
return( true );
		} else
    break;
	    }
	}

	if ( !CoordMatches(sc_sp->nextcp.x-sc_sp->me.x,p_sp->nextcp.x-p_sp->me.x,s) ||
		!CoordMatches(sc_sp->nextcp.y-sc_sp->me.y,p_sp->nextcp.y-p_sp->me.y,s) ||
		!CoordMatches(nsc_sp->me.x-sc_sp->me.x,np_sp->me.x-p_sp->me.x,s) ||
		!CoordMatches(nsc_sp->me.y-sc_sp->me.y,np_sp->me.y-p_sp->me.y,s) ||
		!CoordMatches(nsc_sp->prevcp.x-nsc_sp->me.x,np_sp->prevcp.x-np_sp->me.x,s) ||
		!CoordMatches(nsc_sp->prevcp.y-nsc_sp->me.y,np_sp->prevcp.y-np_sp->me.y,s) )
    break;
	if ( np_sp==path->first ) {
	    if ( nsc_sp==sp ) {
		s->last_sp = saw_sc_first ? NULL : sp;
return( true );
	    }
    break;
	}
	sc_sp = nsc_sp;
	if ( sc_sp == sc_path_first )
	    saw_sc_first = true;
	p_sp = np_sp;
    }

/* ************** Match with just flip transformations applied ************** */
    if ( s->tryflips ) for ( flip=flip_x ; flip<=flip_xy; ++flip ) {
	int xsign = (flip&1) ? -1 : 1, ysign = (flip&2) ? -1 : 1;
	saw_sc_first = false;
	p_sp = s->endpoints ? path->first->next->to : path->first;
	first_of_path = true;
	for (sc_sp=sp; ; ) {
	    if ( p_sp->next==NULL ) {
		if ( substring || sc_sp->next==NULL ) {
		    s->matched_flip = flip;
		    s->last_sp = saw_sc_first ? NULL : sp;
return( true );
		} else
    break;
	    }
	    np_sp = p_sp->next->to;
	    if ( sc_sp->next==NULL )
    break;
	    nsc_sp = sc_sp->next->to;

	    if ( first_of_path && s->endpoints ) {
		SplinePoint *sc_prevsp;
		/* if ( sc_sp->prev==NULL )*/	/* Already checked this above */
		sc_prevsp = sc_sp->prev->from;
		if ( !p_sp->noprevcp ) {
		    if ( !CoordMatches(sc_sp->prevcp.x-sc_sp->me.x,xsign*(p_sp->prevcp.x-p_sp->me.x),s) ||
			 !CoordMatches(sc_sp->prevcp.y-sc_sp->me.y,ysign*(p_sp->prevcp.y-p_sp->me.y),s) )
	break;	/* prev control points do not match, give up */
		} else {
		    sc_unit.x = sc_sp->me.x - sc_prevsp->me.x; sc_unit.y = sc_sp->me.y - sc_prevsp->me.y;
		    len = sqrt(sc_unit.x*sc_unit.x + sc_unit.y*sc_unit.y );
		    sc_unit.x /= len; sc_unit.y /= len;
		    if ( !RealNear(sc_unit.x,xsign * p_unit.x) || !RealNear(sc_unit.y,ysign*p_unit.y))
	break;
		}
		first_of_path = false;
	    }
	    if ( np_sp->next==NULL && s->endpoints ) {
		SplinePoint *sc_nextsp;
		if ( sc_sp->next==NULL )
return( false );
		sc_nextsp = sc_sp->next->to;
		if ( !p_sp->nonextcp ) {
		    if ( !CoordMatches(sc_sp->nextcp.x-sc_sp->me.x,xsign*(p_sp->nextcp.x-p_sp->me.x),s) ||
			 !CoordMatches(sc_sp->nextcp.y-sc_sp->me.y,ysign*(p_sp->nextcp.y-p_sp->me.y),s) )
	break;	/* prev control points do not match, give up */
		} else {
		    if ( !sc_sp->nonextcp )
return( false );
		    sc_unit.x = sc_nextsp->me.x - sc_sp->me.x; sc_unit.y = sc_nextsp->me.y - sc_sp->me.y;
		    len = sqrt(sc_unit.x*sc_unit.x + sc_unit.y*sc_unit.y );
		    if ( len==0 )
return( false );
		    sc_unit.x /= len; sc_unit.y /= len;
		    if ( RealNear(sc_unit.x,xsign*pend_unit.x) && RealNear(sc_unit.y,ysign*pend_unit.y)) {
			s->matched_flip = flip;
			s->last_sp = saw_sc_first ? NULL : sp;
return( true );
		    } else
	break;
		}
	    }

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
		    s->last_sp = saw_sc_first ? NULL : sp;
return( true );
		} else
    break;
	    }
	    sc_sp = nsc_sp;
	    if ( sc_sp == sc_path_first )
		saw_sc_first = true;
	    p_sp = np_sp;
	}
    }

/* ******* Match with rotate, scale and flip transformations applied ******** */
    if ( s->tryrotate || s->tryscale ) {
	if ( s->tryflips )
	    flipmax = flip_xy;
	else
	    flipmax = flip_none;
	for ( flip=flip_none ; flip<=flipmax; ++flip ) {
	    p_sp = s->endpoints ? path->first->next->to : path->first;
	    np_sp = p_sp->next->to;	/* if p_sp->next were NULL, we'd have returned by now */
	    sc_sp = sp;
	    if ( sc_sp->next==NULL )
return( false );
	    nsc_sp = sc_sp->next->to;
	    if ( p_sp->me.x==np_sp->me.x && p_sp->me.y==np_sp->me.y )
return( false );
	    if ( sc_sp->me.x==nsc_sp->me.x && sc_sp->me.y==nsc_sp->me.y )
return( false );
	    if ( s->tryrotate && s->endpoints && np_sp->next == NULL ) {
		int xsign = (flip&1)?-1:1, ysign=(flip&2)?-1:1;
		if ( !p_sp->noprevcp ) {
		    rot = atan2(xsign*(sc_sp->me.y-sc_sp->prevcp.y),ysign*(sc_sp->me.x-sc_sp->prevcp.x)) -
			  atan2(        p_sp->me.y- p_sp->prevcp.y,         p_sp->me.x- p_sp->prevcp.x);
		} else {
		    rot = atan2(xsign*(sc_unit.y),ysign*(sc_unit.x)) -
			  atan2(        p_unit.y,         p_unit.x);
		}
		scale = 1;
	    } else if ( s->endpoints && np_sp->next == NULL ) {
return( false );		/* Not enough info to make a guess */
	    } else if ( !s->tryrotate ) {
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
	    saw_sc_first = false;
	    first_of_path = true;
	    for (sc_sp=sp ; ; ) {
		if ( p_sp->next==NULL ) {
		    if ( substring || sc_sp->next==NULL ) {
			s->matched_flip = flip;
			s->matched_rot = rot;
			s->matched_scale = scale;
			s->last_sp = saw_sc_first ? NULL : sp;
return( true );
		    } else
return( false );
		}
		np_sp = p_sp->next->to;
		if ( sc_sp->next==NULL )
return( false );
		nsc_sp = sc_sp->next->to;

		if ( first_of_path && s->endpoints ) {
		    SplinePoint *sc_prevsp;
		    /* if ( sc_sp->prev==NULL )*/	/* Already checked this above */
		    sc_prevsp = sc_sp->prev->from;
		    if ( !p_sp->noprevcp ) {
			if ( !BPMatches(&sc_sp->prevcp,&sc_sp->me,&p_sp->prevcp,&p_sp->me,flip,rot,scale,s) )
	    break;
		    } else {
			sc_unit.x = sc_sp->me.x - sc_prevsp->me.x; sc_unit.y = sc_sp->me.y - sc_prevsp->me.y;
			len = sqrt(sc_unit.x*sc_unit.x + sc_unit.y*sc_unit.y );
			sc_unit.x /= len; sc_unit.y /= len;
			temp      =  sc_unit.x * s->matched_co + sc_unit.y * s->matched_si;
			sc_unit.y = -sc_unit.x * s->matched_si + sc_unit.y * s->matched_co;
			sc_unit.x = temp;
			if ( !RealNear(sc_unit.x,p_unit.x) || !RealNear(sc_unit.y,p_unit.y))
	    break;
		    }
		    first_of_path = false;
		}
		if ( np_sp->next==NULL && s->endpoints ) {
		    SplinePoint *sc_nextsp;
		    if ( sc_sp->next==NULL )
return( false );
		    sc_nextsp = sc_sp->next->to;
		    if ( !p_sp->nonextcp ) {
			if ( !BPMatches(&sc_sp->nextcp,&sc_sp->me,&p_sp->nextcp,&p_sp->me,flip,rot,scale,s) )
	    break;
		    } else {
			if ( !sc_sp->nonextcp )
return( false );
			sc_unit.x = sc_nextsp->me.x - sc_sp->me.x; sc_unit.y = sc_nextsp->me.y - sc_sp->me.y;
			len = sqrt(sc_unit.x*sc_unit.x + sc_unit.y*sc_unit.y );
			if ( len==0 )
return( false );
			sc_unit.x /= len; sc_unit.y /= len;
			temp      =  sc_unit.x * s->matched_co + sc_unit.y * s->matched_si;
			sc_unit.y = -sc_unit.x * s->matched_si + sc_unit.y * s->matched_co;
			sc_unit.x = temp;
			if ( RealNear(sc_unit.x,pend_unit.x) && RealNear(sc_unit.y,pend_unit.y)) {
			    s->matched_flip = flip;
			    s->matched_rot = rot;
			    s->matched_scale = scale;
			    s->last_sp = saw_sc_first ? NULL : sp;
return( true );
			} else
	    break;
		    }
		}
		
		if ( !BPMatches(&sc_sp->nextcp,&sc_sp->me,&p_sp->nextcp,&p_sp->me,flip,rot,scale,s) ||
			!BPMatches(&nsc_sp->me,&sc_sp->me,&np_sp->me,&p_sp->me,flip,rot,scale,s) ||
			!BPMatches(&nsc_sp->prevcp,&nsc_sp->me,&np_sp->prevcp,&np_sp->me,flip,rot,scale,s) )
return( false );
		if ( np_sp==path->first ) {
		    if ( nsc_sp==sp ) {
			s->matched_flip = flip;
			s->matched_rot = rot;
			s->matched_scale = scale;
			s->last_sp = saw_sc_first ? NULL : sp;
return( true );
		    } else
return( false );
		}
		sc_sp = nsc_sp;
		if ( sc_sp == sc_path_first )
		    saw_sc_first = true;
		p_sp = np_sp;
	    }
	}
    }
return( false );
}

static int SPMatchesO(SplinePoint *sp, SearchData *s, SplineSet *path) {
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

static void SVBuildTrans(SearchData *s,real transform[6]) {
    memset(transform,0,sizeof(real [6]));
    transform[0] = transform[3] = 1;
    if ( s->matched_flip&1 )
	transform[0] = -1;
    if ( s->matched_flip&2 )
	transform[3] = -1;
    transform[0] /= s->matched_scale;
    transform[3] /= s->matched_scale;
    transform[1] = -transform[0]*s->matched_si;
    transform[0] *= s->matched_co;
    transform[2] = transform[3]*s->matched_si;
    transform[3] *= s->matched_co;
    transform[4] = s->matched_x;
    transform[5] = s->matched_y;
}

static void SVFigureTranslation(SearchData *s,BasePoint *p,SplinePoint *sp) {
    real transform[6];
    BasePoint res;

    SVBuildTrans(s,transform);
    res.x = transform[0]*p->x + transform[2]*p->y + transform[4];
    res.y = transform[1]*p->x + transform[3]*p->y + transform[5];
    s->matched_x = sp->me.x - res.x;
    s->matched_y = sp->me.y - res.y;
}

static int SPMatches(SplinePoint *sp, SearchData *s, SplineSet *path,
	SplinePoint *sc_path_first, int oriented ) {
    real transform[6];
    BasePoint *p, res;
    if ( oriented ) {
	double fudge = s->fudge<.1 ? 10*s->fudge : s->fudge;
	SVBuildTrans(s,transform);
	p = &path->first->me;
	res.x = transform[0]*p->x + transform[2]*p->y + transform[4];
	res.y = transform[1]*p->x + transform[3]*p->y + transform[5];
	if ( sp->me.x>res.x+fudge || sp->me.x<res.x-fudge ||
		sp->me.y>res.y+fudge || sp->me.y<res.y-fudge )
return( false );

return( SPMatchesO(sp,s,path));
    } else {
	if ( !SPMatchesF(sp,s,path,sc_path_first,false))
return( false );
	SVFigureTranslation(s,&path->first->me,sp);
return( true );
    }
}

/* We are given a single, unclosed path to match. It is a match if it */
/*  corresponds to any subset of a path in the character */
static int SCMatchesIncomplete(SplineChar *sc,SearchData *s,int startafter) {
    /* don't look in refs because we can't do a replace there */
    SplineSet *spl;
    SplinePoint *sp;
    int layer = s->fv->active_layer;

    for ( spl=startafter?s->matched_spl:sc->layers[layer].splines; spl!=NULL; spl=spl->next ) {
	s->matched_spl = spl;
	for ( sp=startafter?s->last_sp:spl->first; sp!=NULL; ) {
	    if ( SPMatchesF(sp,s,s->path,spl->first,true)) {
		SVFigureTranslation(s,&s->path->first->me,sp);
return( true );
	    } else if ( s->tryreverse && SPMatchesF(sp,s,s->revpath,spl->first,true)) {
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
static int SCMatchesFull(SplineChar *sc,SearchData *s) {
    /* don't look to match paths in refs because we can't do a replace there */
    SplineSet *spl, *s_spl, *s_r_spl;
    SplinePoint *sp;
    RefChar *r, *s_r;
    int i, first, ref_first;
    int layer = s->fv->active_layer;

    s->matched_ss = s->matched_refs = s->matched_ss_start = 0;
    first = true;
    for ( s_r = s->sc_srch.layers[ly_fore].refs; s_r!=NULL; s_r = s_r->next ) {
	for ( r = sc->layers[layer].refs, i=0; r!=NULL; r=r->next, ++i ) if ( !(s->matched_refs&(1<<i)) ) {
	    if ( r->sc == s_r->sc ) {
		/* I should check the transform to see if the tryflips (etc) flags would make this not a match */
		if ( r->transform[0]==s_r->transform[0] && r->transform[1]==s_r->transform[1] &&
			r->transform[2]==s_r->transform[2] && r->transform[3]==s_r->transform[3] ) {
		    if ( first ) {
			s->matched_scale = 1.0;
			s->matched_x = r->transform[4]-s_r->transform[4];
			s->matched_y = r->transform[5]-s_r->transform[5];
			first = false;
	break;
		    } else if ( r->transform[4]-s_r->transform[4]==s->matched_x &&
			    r->transform[5]-s_r->transform[5] == s->matched_y )
	break;
		}
	    }
	}
	if ( r==NULL )
return( false );
	s->matched_refs |= 1<<i;
    }
    ref_first = first;

 retry:
    if ( ref_first )
	s->matched_x = s->matched_y = 0;
    s->matched_ss = s->matched_ss_start;	/* Don't use any of these contours, they don't work */
    first = ref_first;
    for ( s_spl = s->path, s_r_spl=s->revpath; s_spl!=NULL; s_spl=s_spl->next, s_r_spl = s_r_spl->next ) {
	for ( spl=sc->layers[layer].splines, i=0; spl!=NULL; spl=spl->next, ++i ) if ( !(s->matched_ss&(1<<i)) ) {
	    s->matched_spl = spl;
	    if ( spl->first->prev==NULL ) {	/* Open */
		if ( s_spl->first!=s_spl->last ) {
		    if ( SPMatches(spl->first,s,s_spl,spl->first,1-first))
	break;
		    if ( s->tryreverse && SPMatches(spl->first,s,s_r_spl,spl->first,1-first)) {
			s->wasreversed = true;
	break;
		    }
		}
	    } else {
		if ( s_spl->first==s_spl->last ) {
		    int found = false;
		    for ( sp=spl->first; ; ) {
			if ( SPMatches(sp,s,s_spl,spl->first,1-first)) {
			    found = true;
		    break;
			}
			if ( s->tryreverse && SPMatches(sp,s,s_r_spl,spl->first,1-first)) {
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
	if ( spl==NULL ) {
	    if ( s_spl == s->path )
return( false );
	    /* Ok, we could not find a match starting from the contour */
	    /*  we started with. However if there are multiple contours */
	    /*  in the search pattern, that might just mean that we a bad */
	    /*  transform, so start from scratch and try some other contours */
	    /*  but don't test anything we've already ruled out */
 goto retry;
	}
	if ( s_spl == s->path ) {
	    s->matched_ss_start |= 1<<i;
	    s->matched_ss = 1<<i;		/* Ok, the contours in matched_ss_start didn't work as starting points, but we still must test them with the current transform against other contours */
	} else
	    s->matched_ss |= 1<<i;
	first = false;
    }
return( true );
}

static int AdjustBP(BasePoint *changeme,BasePoint *rel,
	BasePoint *shouldbe, BasePoint *shouldberel,
	BasePoint *fudge,
	SearchData *s) {
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
return( changeme->x==rel->x && changeme->y==rel->y );
}

static void AdjustAll(SplinePoint *change,BasePoint *rel,
	BasePoint *shouldbe, BasePoint *shouldberel,
	BasePoint *fudge,
	SearchData *s) {
    BasePoint old;

    old = change->me;
    AdjustBP(&change->me,rel,shouldbe,shouldberel,fudge,s);
    change->nextcp.x += change->me.x-old.x; change->nextcp.y += change->me.y-old.y;
    change->prevcp.x += change->me.x-old.x; change->prevcp.y += change->me.y-old.y;

    change->nonextcp = (change->nextcp.x==change->me.x && change->nextcp.y==change->me.y);
    change->noprevcp = (change->prevcp.x==change->me.x && change->prevcp.y==change->me.y);
}

static SplinePoint *RplInsertSP(SplinePoint *after,SplinePoint *nrpl,SplinePoint *rpl,SearchData *s, BasePoint *fudge) {
    SplinePoint *new = chunkalloc(sizeof(SplinePoint));
    real transform[6];

    SVBuildTrans(s,transform);
    /*transform[4] += fudge->x; transform[5] += fudge->y;*/
    new->me.x = after->me.x + transform[0]*(nrpl->me.x-rpl->me.x) + transform[1]*(nrpl->me.y-rpl->me.y) + fudge->x;
    new->me.y = after->me.y + transform[2]*(nrpl->me.x-rpl->me.x) + transform[3]*(nrpl->me.y-rpl->me.y) + fudge->y;
    new->nextcp.x = after->me.x + transform[0]*(nrpl->nextcp.x-rpl->me.x) + transform[1]*(nrpl->nextcp.y-rpl->me.y) + fudge->x;
    new->nextcp.y = after->me.y + transform[2]*(nrpl->nextcp.x-rpl->me.x) + transform[3]*(nrpl->nextcp.y-rpl->me.y) + fudge->y;
    new->prevcp.x = after->me.x + transform[0]*(nrpl->prevcp.x-rpl->me.x) + transform[1]*(nrpl->prevcp.y-rpl->me.y) + fudge->x;
    new->prevcp.y = after->me.y + transform[2]*(nrpl->prevcp.x-rpl->me.x) + transform[3]*(nrpl->prevcp.y-rpl->me.y) + fudge->y;
    new->nonextcp = (new->nextcp.x==new->me.x && new->nextcp.y==new->me.y);
    new->noprevcp = (new->prevcp.x==new->me.x && new->prevcp.y==new->me.y);
    new->pointtype = rpl->pointtype;
    new->selected = true;
    new->ticked = true;
    if ( after->next==NULL ) {
	SplineMake(after,new,after->prev->order2);
	s->matched_spl->last = new;
    } else {
	SplinePoint *nsp = after->next->to;
	after->next->to = new;
	new->prev = after->next;
	SplineRefigure(after->next);
	SplineMake(new,nsp,after->next->order2);
    }
return( new );
}

static void FudgeFigure(SearchData *s,SplineSet *path,BasePoint *fudge) {
    SplinePoint *search, *searchrel, *found, *foundrel;
    real xoff, yoff;

    fudge->x = fudge->y = 0;
    if ( path->first->prev!=NULL )		/* closed path, should end where it began */
return;						/*  => no fudge */

    foundrel = s->matched_sp; searchrel = path->first;
    if ( s->endpoints ) searchrel = searchrel->next->to;
    for ( found=foundrel, search=searchrel ; ; ) {
	if ( found->next==NULL || search->next==NULL )
    break;
	if ( s->endpoints && search->next->to->next==NULL )
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

static void DoReplaceIncomplete(SplineChar *sc,SearchData *s) {
    SplinePoint *sc_p, *nsc_p, *p_p, *np_p, *r_p, *nr_p;
    BasePoint fudge;
    SplineSet *path, *rpath;
    SplinePoint dummy; Spline dummysp;

    if ( s->wasreversed ) {
	path = s->revpath;
	rpath = s->revreplace;
    } else {
	path = s->path;
	rpath = s->replacepath;
    }

    /* Total "fudge" amount should be spread evenly over each point */
    FudgeFigure(s,path,&fudge);
    if ( s->pointcnt!=s->rpointcnt )
	MinimumDistancesFree(sc->md); sc->md = NULL;

    sc_p = s->matched_sp; p_p = path->first, r_p = rpath->first;
    if ( s->endpoints ) {
	real xoff, yoff;
	memset(&dummy,0,sizeof(dummy));
	memset(&dummysp,0,sizeof(dummysp));
	dummysp.from = &dummy;
	dummysp.to = sc_p;
	dummysp.order2 = p_p->next->order2;
	np_p = p_p->next->to;
	xoff = (p_p->me.x-np_p->me.x);
	yoff = (p_p->me.y-np_p->me.y);
	if ( s->matched_flip&1 )
	    xoff=-xoff;
	if ( s->matched_flip&2 )
	    yoff =-yoff;
	xoff *= s->matched_scale;
	yoff *= s->matched_scale;
	dummy.me.x = sc_p->me.x + xoff*s->matched_co - yoff*s->matched_si;
	dummy.me.y = sc_p->me.y + yoff*s->matched_co + xoff*s->matched_si;
	dummy.nextcp = dummy.prevcp = dummy.me;
	dummy.nonextcp = dummy.noprevcp = true;
	dummy.next = &dummysp;
	SplineRefigure(&dummysp);
	sc_p = &dummy;
    }
    for ( ; ; ) {
	if ( (p_p->next==NULL && r_p->next==NULL) ||
		(s->endpoints && p_p->next->to->next==NULL && r_p->next->to->next == NULL )) {
	    s->last_sp = s->last_sp==NULL ? NULL : sc_p;	/* If we crossed the contour start, move to next contour */
return;		/* done */
	} else if ( p_p->next==NULL || (s->endpoints && p_p->next->to->next==NULL)) {
	    /* The search pattern is shorter that the replace pattern */
	    /*  Need to add some extra points */
	    nr_p = r_p->next->to;
	    sc_p = RplInsertSP(sc_p,nr_p,r_p,s,&fudge);
	    r_p = nr_p;
	} else if ( r_p->next==NULL || (s->endpoints && r_p->next->to->next==NULL)) {
	    /* The replace pattern is shorter than the search pattern */
	    /*  Need to remove some points */
	    nsc_p = sc_p->next->to;
	    if ( nsc_p->next==NULL ) {
		SplinePointFree(nsc_p);
		SplineFree(sc_p->next);
		sc_p->next = NULL;
		s->matched_spl->last = sc_p;
		s->last_sp = s->last_sp==NULL ? NULL : sc_p;
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
		sc_p->nonextcp = AdjustBP(&sc_p->nextcp,&sc_p->me,&r_p->nextcp,&r_p->me,&fudge,s);
		if ( p_p->prev!=NULL )
		    sc_p->noprevcp = AdjustBP(&sc_p->prevcp,&sc_p->me,&r_p->prevcp,&r_p->me,&fudge,s);
		if ( sc_p->prev!=NULL )
		    SplineRefigure(sc_p->prev);
		sc_p->ticked = true;
	    }
	    if ( np_p==path->first )
return;
	    if ( np_p->next!=NULL )
		nsc_p->nonextcp = AdjustBP(&nsc_p->nextcp,&nsc_p->me,&nr_p->nextcp,&nr_p->me,&fudge,s);
	    nsc_p->noprevcp = AdjustBP(&nsc_p->prevcp,&nsc_p->me,&nr_p->prevcp,&nr_p->me,&fudge,s);
	    AdjustAll(nsc_p,&sc_p->me,&nr_p->me,&r_p->me,&fudge,s);
	    nsc_p->ticked = true;
	    nsc_p->pointtype = nr_p->pointtype;
	    if ( nsc_p->next!=NULL ) {
		if ( nsc_p->next->order2 )
		    nsc_p->next->to->prevcp = nsc_p->nextcp;
		SplineRefigure(nsc_p->next);
	    }
	    if ( nsc_p->prev!=NULL ) {
		if ( nsc_p->prev->order2 )
		    nsc_p->prev->from->nextcp = nsc_p->prevcp;
		SplineRefigure(nsc_p->prev);
	    }
	    p_p = np_p; sc_p = nsc_p; r_p = nr_p;
	}
    }
}

static int HeuristiclyBadMatch(SplineChar *sc,SearchData *s) {
    /* Consider the case of a closed circle and an open circle, where the */
    /*  open circle looks just like the closed except that the open circle */
    /*  has an internal counter-clockwise contour. We don't want to match here*/
    /*  we'd get a reference and a counter-clockwise contour which would make */
    /*  no sense. */
    /* so if, after removing matched contours we are left with a single counter*/
    /*  clockwise contour, don't accept the match */
    int contour_cnt, i;
    SplineSet *spl;
    int layer = s->fv->active_layer;

    contour_cnt=0;
    for ( spl=sc->layers[layer].splines, i=0; spl!=NULL; spl=spl->next, ++i ) {
	if ( !(s->matched_ss&(1<<i))) {
	    if ( SplinePointListIsClockwise(spl)==1 )
return( false );
	    ++contour_cnt;
	}
    }
    if ( contour_cnt==0 )
return( false );		/* Exact match */

    /* Everything remaining is counter-clockwise */
return( true );
}

static void DoReplaceFull(SplineChar *sc,SearchData *s) {
    int i;
    RefChar *r, *rnext, *new;
    SplinePointList *spl, *snext, *sprev, *temp;
    real transform[6], subtrans[6];
    SplinePoint *sp;
    int layer = s->fv->active_layer;

    /* first remove those bits that matched */
    for ( r = sc->layers[layer].refs, i=0; r!=NULL; r=rnext, ++i ) {
	rnext = r->next;
	if ( s->matched_refs&(1<<i))
	    SCRemoveDependent(sc,r,layer);
    }
    sprev = NULL;
    for ( spl=sc->layers[layer].splines, i=0; spl!=NULL; spl=snext, ++i ) {
	snext = spl->next;
	if ( s->matched_ss&(1<<i)) {
	    if ( sprev==NULL )
		sc->layers[layer].splines = snext;
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
	free(new->layers);
	*new = *r;
	memcpy(new->transform,subtrans,sizeof(subtrans));
	new->layers = NULL;
	new->layer_cnt = 0;
	new->next = sc->layers[layer].refs;
	new->selected = true;
	sc->layers[layer].refs = new;
	SCReinstanciateRefChar(sc,new,layer);
	SCMakeDependent(sc,new->sc);
	if ( sc->layers[layer].order2 &&
		!sc->layers[layer].background &&
		!sc->instructions_out_of_date &&
		sc->ttf_instrs!=NULL ) {
	    /* The normal change check doesn't respond properly to pasting a reference */
	    SCClearInstrsOrMark(sc,layer,!s->already_complained);
	    s->already_complained = true;
	}
    }
    temp = SplinePointListTransform(SplinePointListCopy(s->sc_rpl.layers[ly_fore].splines),transform,tpt_AllPoints);
    if ( sc->layers[layer].splines==NULL )
	sc->layers[layer].splines = temp;
    else {
	for ( spl=sc->layers[layer].splines; spl->next!=NULL; spl = spl->next );
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

void SVResetPaths(SearchData *sv) {
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
}

static void SplinePointsUntick(SplineSet *spl) {
    SplinePoint *sp;

    while ( spl!=NULL ) {
	for ( sp = spl->first ; ; ) {
	    sp->ticked = false;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==spl->first )
	break;
	}
	spl = spl->next;
    }
}

void SCSplinePointsUntick(SplineChar *sc,int layer) {
    SplinePointsUntick(sc->layers[layer].splines);
}

int SearchChar(SearchData *sv, int gid,int startafter) {

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

int DoRpl(SearchData *sv) {
    RefChar *r;
    int layer = sv->fv->active_layer;

    /* Make sure we don't generate any self referential characters... */
    for ( r = sv->sc_rpl.layers[ly_fore].refs; r!=NULL; r=r->next ) {
	if ( SCDependsOnSC(r->sc,sv->curchar))
return(false);
    }

    if ( sv->replaceall && !sv->subpatternsearch &&
	    HeuristiclyBadMatch(sv->curchar,sv))
return(false);

    SCPreserveLayer(sv->curchar,layer,false);
    if ( sv->subpatternsearch )
	DoReplaceIncomplete(sv->curchar,sv);
    else
	DoReplaceFull(sv->curchar,sv);
    SCCharChangedUpdate(sv->curchar,layer);
return( true );
}

int _DoFindAll(SearchData *sv) {
    int i, any=0, gid;
    SplineChar *startcur = sv->curchar;

    for ( i=0; i<sv->fv->map->enccount; ++i ) {
	if (( !sv->onlyselected || sv->fv->selected[i]) && (gid=sv->fv->map->map[i])!=-1 &&
		sv->fv->sf->glyphs[gid]!=NULL ) {
	    SCSplinePointsUntick(sv->fv->sf->glyphs[gid],sv->fv->active_layer);
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

void SDDestroy(SearchData *sv) {
    int i;

    if ( sv==NULL )
return;

    SCClearContents(&sv->sc_srch,ly_fore);
    SCClearContents(&sv->sc_rpl,ly_fore);
    for ( i=0; i<sv->sc_srch.layer_cnt; ++i )
	UndoesFree(sv->sc_srch.layers[i].undoes);
    for ( i=0; i<sv->sc_rpl.layer_cnt; ++i )
	UndoesFree(sv->sc_rpl.layers[i].undoes);
    free(sv->sc_srch.layers);
    free(sv->sc_rpl.layers);
    SplinePointListsFree(sv->revpath);
}

SearchData *SDFillup(SearchData *sv, FontViewBase *fv) {

    sv->sc_srch.orig_pos = 0; sv->sc_srch.unicodeenc = -1; sv->sc_srch.name = "Search";
    sv->sc_rpl.orig_pos = 1; sv->sc_rpl.unicodeenc = -1; sv->sc_rpl.name = "Replace";
    sv->sc_srch.layer_cnt = sv->sc_rpl.layer_cnt = 2;
    sv->sc_srch.layers = calloc(2,sizeof(Layer));
    sv->sc_rpl.layers = calloc(2,sizeof(Layer));
    LayerDefault(&sv->sc_srch.layers[0]);
    LayerDefault(&sv->sc_srch.layers[1]);
    LayerDefault(&sv->sc_rpl.layers[0]);
    LayerDefault(&sv->sc_rpl.layers[1]);

    sv->fv = fv;
return( sv );
}

static int IsASingleReferenceOrEmpty(SplineChar *sc,int layer) {
    int i, empty = true, last, first;

    if ( sc->parent->multilayer ) {
	first = ly_fore;
	last = sc->layer_cnt-1;
    } else
	first = last = layer;
    for ( i = first; i<=last; ++i ) {
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

static void SDCopyToSC(SplineChar *checksc,SplineChar *into,enum fvcopy_type full) {
    int i;
    RefChar *ref;

    for ( i=0; i<into->layer_cnt; ++i ) {
	SplinePointListsFree(into->layers[i].splines);
	RefCharsFree(into->layers[i].refs);
	into->layers[i].splines = NULL;
	into->layers[i].refs = NULL;
    }
    if ( full==ct_fullcopy ) {
	into->layers[ly_fore].splines = SplinePointListCopy(checksc->layers[ly_fore].splines);
	into->layers[ly_fore].refs    = RefCharsCopyState(checksc,ly_fore);
    } else {
	into->layers[ly_fore].refs = ref = RefCharCreate();
	ref->unicode_enc = checksc->unicodeenc;
	ref->orig_pos = checksc->orig_pos;
	ref->adobe_enc = getAdobeEnc(checksc->name);
	ref->transform[0] = ref->transform[3] = 1.0;
	ref->sc = checksc;
    }
    /* This is used to fill up the search/rpl patterns, which can't have */
    /*  instructions, so I don't bother to check for instructions out of */
    /*  date */
}

void FVBReplaceOutlineWithReference( FontViewBase *fv, double fudge ) {
    SearchData *sv;
    uint8 *selected, *changed;
    SplineFont *sf = fv->sf;
    int i, j, selcnt = 0, gid;
    SplineChar *checksc;

    sv = SDFillup( calloc(1,sizeof(SearchData)), fv);
    sv->fudge_percent = .001;
    sv->fudge = fudge;
    sv->replaceall = true;
    sv->replacewithref = true;

    selected = malloc(fv->map->enccount);
    memcpy(selected,fv->selected,fv->map->enccount);
    changed = calloc(fv->map->enccount,1);

    selcnt = 0;
    for ( i=0; i<fv->map->enccount; ++i ) if ( selected[i] && (gid=fv->map->map[i])!=-1 &&
	    sf->glyphs[gid]!=NULL )
	++selcnt;
    ff_progress_start_indicator(10,_("Replace with Reference"),
	    _("Replace Outline with Reference"),0,selcnt,1);

    for ( i=0; i<fv->map->enccount; ++i ) if ( selected[i] && (gid=fv->map->map[i])!=-1 &&
	    (checksc=sf->glyphs[gid])!=NULL ) {
	if ( IsASingleReferenceOrEmpty(sf->glyphs[gid],fv->active_layer))
    continue;		/* No point in replacing something which is itself a ref with a ref to a ref */
	memset(fv->selected,0,fv->map->enccount);
	SDCopyToSC(checksc,&sv->sc_srch,ct_fullcopy);
	SDCopyToSC(checksc,&sv->sc_rpl,ct_reference);
	sv->sc_srch.changed_since_autosave = sv->sc_rpl.changed_since_autosave = true;
	SVResetPaths(sv);
	if ( !_DoFindAll(sv) && selcnt==1 )
	    ff_post_notice(_("Not Found"),_("The outlines of glyph %2$.30s were not found in the font %1$.60s"),
		    sf->fontname,sf->glyphs[gid]->name);
	for ( j=0; j<fv->map->enccount; ++j )
	    if ( fv->selected[j] )
		changed[j] = 1;
	if ( !ff_progress_next())
    break;
    }
    ff_progress_end_indicator();

    SDDestroy(sv);
    free(sv);

    free(selected);
    memcpy(fv->selected,changed,fv->map->enccount);
    free(changed);
}

/* This will free both the find and rpl contours */
int FVReplaceAll( FontViewBase *fv, SplineSet *find, SplineSet *rpl, double fudge, int flags ) {
    SearchData *sv;
    int ret;

    sv = SDFillup( calloc(1,sizeof(SearchData)), fv);
    sv->fudge_percent = .001;
    sv->fudge = fudge;
    sv->replaceall = true;

    sv->tryreverse = (flags&sv_reverse);
    sv->tryflips = (flags&sv_flips);
    sv->tryrotate = (flags&sv_rotate);
    sv->tryscale = (flags&sv_scale);

    sv->sc_srch.layers[ly_fore].splines = find;
    sv->sc_rpl .layers[ly_fore].splines = rpl;
    sv->sc_srch.changed_since_autosave = sv->sc_rpl.changed_since_autosave = true;
    SVResetPaths(sv);

    ret = _DoFindAll(sv);

    SDDestroy(sv);
    free(sv);
return( ret );
}

/* This will free both the find and rpl contours */
SearchData *SDFromContour( FontViewBase *fv, SplineSet *find, double fudge, int flags ) {
    SearchData *sv;

    sv = SDFillup( calloc(1,sizeof(SearchData)), fv);
    sv->fudge_percent = .001;
    sv->fudge = fudge;

    sv->tryreverse = (flags&sv_reverse) != 0;
    sv->tryflips = (flags&sv_flips) != 0;
    sv->tryrotate = (flags&sv_rotate) != 0;
    sv->tryscale = (flags&sv_scale) != 0;

    sv->sc_srch.layers[ly_fore].splines = find;
    sv->sc_srch.changed_since_autosave = sv->sc_rpl.changed_since_autosave = true;
    SVResetPaths(sv);

    sv->last_gid = -1;
return( sv );
}

SplineChar *SDFindNext(SearchData *sd) {
    int gid;
    FontViewBase *fv;

    if ( sd==NULL )
return( NULL );
    fv = sd->fv;

    for ( gid=sd->last_gid+1; gid<fv->sf->glyphcnt; ++gid ) {
	SCSplinePointsUntick(fv->sf->glyphs[gid],fv->active_layer);
	if ( SearchChar(sd,gid,false) ) {
	    sd->last_gid = gid;
return( fv->sf->glyphs[gid]);
	}
    }
return( NULL );
}

/* ************************************************************************** */
/* *************************** Correct References *************************** */
/* ************************************************************************** */

/* In TrueType a glyph can either be all contours or all references. FontForge*/
/*  supports mixed contours and references. This section goes through the font*/
/*  looking for such mixed glyphs, if it finds one it will create a new glyph */
/*  move the contours into it, and make a reference to the new glyph in the   */
/*  original.  This is designed to reduce the size of the TT output file      */

/* Similar problem... The transformation matrix of a truetype reference has   */
/*  certain restrictions placed on it (all entries must be less than 2 in abs */
/*  value, etc.  If we have a glyph with a ref with a bad transform, then     */
/*  go through a similar process to the above */

static SplineChar *RC_MakeNewGlyph(FontViewBase *fv,SplineChar *base, int index,
	const char *reason, const char *morereason) {
    char *namebuf;
    SplineFont *sf = fv->sf;
    int enc;
    SplineChar *ret;

    namebuf = malloc(strlen(base->name)+20);
    for (;;) {
	sprintf(namebuf, "%s.ref%d", base->name, index++ );
	if ( SFGetChar(sf,-1,namebuf)==NULL )
    break;
    }

    enc = SFFindSlot(sf, fv->map, -1, namebuf );
    if ( enc==-1 )
	enc = fv->map->enccount;
    ret = SFMakeChar(sf,fv->map,enc);
    free(ret->name);
    ret->name = namebuf;
    SFHashGlyph(sf,ret);

    ret->comment = malloc( strlen(reason)+strlen(ret->name)+strlen(morereason) + 2 );
    sprintf( ret->comment, reason, base->name, morereason );
    ret->color = 0xff8080;
return( ret );
}

static void AddRef(SplineChar *sc,SplineChar *rsc, int layer) {
    RefChar *r;

    r = RefCharCreate();
    free(r->layers);
    r->layers = NULL;
    r->layer_cnt = 0;
    r->sc = rsc;
    r->unicode_enc = rsc->unicodeenc;
    r->orig_pos = rsc->orig_pos;
    r->adobe_enc = getAdobeEnc(rsc->name);
    r->transform[0] = r->transform[3] = 1.0;
    r->next = NULL;
    SCMakeDependent(sc,rsc);
    SCReinstanciateRefChar(sc,r,layer);
    r->next = sc->layers[layer].refs;
    sc->layers[layer].refs = r;
}

static struct splinecharlist *DListRemove(struct splinecharlist *dependents,SplineChar *this_sc) {
    struct splinecharlist *dlist, *pd;

    if ( dependents==NULL )
return( NULL );
    else if ( dependents->sc==this_sc ) {
	dlist = dependents->next;
	chunkfree(dependents,sizeof(*dependents));
return( dlist );
    } else {
	for ( pd=dependents, dlist = pd->next; dlist!=NULL && dlist->sc!=this_sc; pd=dlist, dlist = pd->next );
	if ( dlist!=NULL ) {
	    pd->next = dlist->next;
	    chunkfree(dlist,sizeof(*dlist));
	}
return( dependents );
    }
}

void FVCorrectReferences(FontViewBase *fv) {
    int enc, gid, cnt;
    SplineFont *sf = fv->sf;
    SplineChar *sc, *rsc;
    RefChar *ref;
    int layer = fv->active_layer;
    int index;

    cnt = 0;
    for ( enc=0; enc<fv->map->enccount; ++enc ) {
	if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL )
	    ++cnt;
    }
    ff_progress_start_indicator(10,_("Correcting References"),
	_("Adding new glyphs and referring to them when a glyph contains a bad truetype reference"),NULL,cnt,1);
    for ( enc=0; enc<fv->map->enccount; ++enc ) {
	if ( (gid=fv->map->map[enc])!=-1 && fv->selected[enc] && (sc=sf->glyphs[gid])!=NULL ) {
	    index = 1;
	    if ( sc->layers[layer].splines!=NULL && sc->layers[layer].refs!=NULL ) {
		SCPreserveLayer(sc,layer,false);
		rsc = RC_MakeNewGlyph(fv,sc,index++,
		    _("%s had both contours and references, so the contours were moved "
		      "into this glyph, and a reference to it was added in the original."),
		    "");
		rsc->layers[layer].splines = sc->layers[layer].splines;
		sc->layers[layer].splines  = NULL;
		AddRef(sc,rsc,layer);
		/* I don't bother to check for instructions because there */
		/*  shouldn't be any in a mixed outline and reference glyph */
	    }
	    for ( ref=sc->layers[layer].refs; ref!=NULL; ref=ref->next ) {
		if ( ref->transform[0]>0x7fff/16384.0 ||
			ref->transform[1]>0x7fff/16384.0 ||
			ref->transform[2]>0x7fff/16384.0 ||
			ref->transform[3]>0x7fff/16384.0 ||
			ref->transform[0]<-2.0 ||	/* Numbers are asymetric, negative range slightly bigger */
			ref->transform[1]<-2.0 ||
			ref->transform[2]<-2.0 ||
			ref->transform[3]<-2.0 ) {
		    if ( index==1 )
			SCPreserveLayer(sc,layer,false);
		    rsc = RC_MakeNewGlyph(fv,sc,index++,
			_("%1$s had a reference, %2$s, with a bad transformation matrix (one of "
			  "the matrix elements was bigger than 2). I moved the transformed "
			  "contours into this glyph and made a reference to it, instead."),
			ref->sc->name);
		    rsc->layers[layer].splines = ref->layers[0].splines;
		    ref->layers[0].splines = NULL;
		    ref->sc->dependents = DListRemove(ref->sc->dependents,sc);
		    ref->sc = rsc;
		    memset(ref->transform,0,sizeof(ref->transform));
		    ref->transform[0] = ref->transform[3] = 1.0;
		    SCReinstanciateRefChar(sc,ref,layer);
		}
	    }
	    if ( index!=1 )
		SCCharChangedUpdate(sc,layer);
	    if ( !ff_progress_next())
    break;
	}
    }
    ff_progress_end_indicator();
}
