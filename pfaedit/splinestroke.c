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
#include "pfaedit.h"
#include "splinefont.h"
#include <math.h>
#include <gwidget.h>

typedef struct joininfo {
    SplinePoint *from, *to;
    real tprev;
    real tnext;
    BasePoint inter;
} JointPoint;


/* the plus point is where we go when we rotate the line's direction by +90degrees */
/*  and then move radius in that direction. minus is when we rotate -90 and */
/*  then move */	/* counter-clockwise */
static real SplineExpand(Spline *spline,real t,StrokeInfo *si,BasePoint *plus, BasePoint *minus) {
    Spline1D *xsp = &spline->splines[0], *ysp = &spline->splines[1];
    real xslope = (3*xsp->a*t+2*xsp->b)*t + xsp->c;
    real yslope = (3*ysp->a*t+2*ysp->b)*t + ysp->c;
    BasePoint base;
    real angle, lineangle, c,s;

    if ( xslope==0 && yslope==0 ) {
	real faket = (t>.5) ? t-.01 : t+.01;
	xslope = (3*xsp->a*faket+2*xsp->b)*faket + xsp->c;
	yslope = (3*ysp->a*faket+2*ysp->b)*faket + ysp->c;
    }

    base.x = ((xsp->a*t+xsp->b)*t+xsp->c)*t + xsp->d;
    base.y = ((ysp->a*t+ysp->b)*t+ysp->c)*t + ysp->d;

    lineangle = atan2(yslope,xslope);
    if ( !si->caligraphic )
	angle = lineangle;
    else
	angle = si->penangle-3.1415926535897932/2;
    c = si->radius*cos(angle+3.1415926535897932/2);
    s = si->radius*sin(angle+3.1415926535897932/2);
    plus->y = base.y+s;
    plus->x = base.x+c;
    minus->y = base.y-s;
    minus->x = base.x-c;
    if ( si->caligraphic ) {
#if 0
	if ( si->penangle>0 && si->penangle<=3.1415926535897932/4 ) {
	    plus->y += si->thickness;
	    minus->y -= si->thickness;
	} else if ( si->penangle>0 && si->penangle<=3.1415926535897932/2 ) {
	    plus->x += si->thickness;
	    minus->x -= si->thickness;
	} else if ( si->penangle<=0 && si->penangle>=-3.1415926535897932/4 ) {
	    plus->y -= si->thickness;
	    minus->y += si->thickness;
	} else if ( si->penangle<0 && si->penangle>=-3.1415926535897932/2 ) {
	    plus->x -= si->thickness;
	    minus->x += si->thickness;
	}
#endif
	angle = lineangle-si->penangle;
	if ( angle>3.1415926535897932 )
	    angle -= 2*3.1415926535897932;
	else if ( angle<-3.1415926535897932 )
	    angle += 2*3.1415926535897932;
	if ( angle>0 ) {
	    BasePoint t;
	    t = *plus;
	    *plus = *minus;
	    *minus = t;
	}
    }
return( angle );
}

static SplinePoint *makequartercircle(real x, real y, real radius,
	real xmul, real ymul,SplinePoint *prev) {
    SplinePoint *here = chunkalloc(sizeof(SplinePoint));

    here->me.x = x;
    here->me.y = y;

    if ( xmul==0 ) {
	here->nextcp.x = here->prevcp.x = x;
	here->nextcp.y = y + .552*ymul*radius;
	here->prevcp.y = y - .552*ymul*radius;
    } else {
	here->nextcp.y = here->prevcp.y = y;
	here->nextcp.x = x + .552*xmul*radius;
	here->prevcp.x = x - .552*xmul*radius;
    }
    if ( prev!=NULL )
	SplineMake(prev,here);
return( here );
}

static void StrokeEnd(SplinePoint *base, StrokeInfo *si, SplinePoint **_plus, SplinePoint **_minus) {
    SplinePoint *plus, *minus, *cur, *mid1, *mid2;
    real c,s;
    real angle;
    real sign;

    if ( base->next==NULL && base->prev==NULL ) {
	/* A single point, is kind of dull.
	    For a linecap of lc_butt it's still a point
	    For a linecap of lc_round it's a circle
	    For a linecap of lc_square it should be a square...
		but how does one orient that square? probably a circle is best
		here too
	*/
	if ( si->cap!=lc_butt ) {
	    plus = makequartercircle(base->me.x-si->radius,base->me.y,si->radius,0,1,NULL);
	    cur = makequartercircle(base->me.x,base->me.y+si->radius,si->radius,1,0,plus);
	    cur = makequartercircle(base->me.x+si->radius,base->me.y,si->radius,0,-1,cur);
	    cur = makequartercircle(base->me.x,base->me.y-si->radius,si->radius,-1,0,cur);
	    SplineMake(cur,plus);
	    *_plus = *_minus = plus;
	} else {
	    *_plus = *_minus = cur = chunkalloc(sizeof(SplinePoint));
	    *cur = *base;
	}
    } else {
	plus = chunkalloc(sizeof(SplinePoint));
	minus = chunkalloc(sizeof(SplinePoint));
	plus->pointtype = pt_corner; minus->pointtype = pt_corner;
	if ( base->next==NULL ) {	/* the prev spline moves toward base */
	    SplineIsLinearMake(base->prev);
	    angle = SplineExpand(base->prev,1,si,&plus->me,&minus->me);
	    sign = 1;
	} else {
	    SplineIsLinearMake(base->next);
	    angle = SplineExpand(base->next,0,si,&plus->me,&minus->me)+
		    3.1415926535897932;
	    sign = -1;
	}
	if ( si->cap!=lc_round ) {
	    plus->nextcp = plus->me;
	    minus->prevcp = minus->me;
	    plus->nonextcp = minus->nonextcp = true;
	}
	if ( si->cap==lc_butt ) {
	    SplineMake(plus,minus);		/* draw a line between */
	} else if ( si->cap==lc_square ) {
	    mid1 = chunkalloc(sizeof(SplinePoint));
	    mid1->me.x = plus->me.x+ sign*(plus->me.y-base->me.y);
	    mid1->me.y = plus->me.y- sign*(plus->me.x-base->me.x);
	    mid1->nextcp = mid1->prevcp = mid1->me;
	    mid1->nonextcp = mid1->noprevcp = true;
	    mid2 = chunkalloc(sizeof(SplinePoint));
	    mid2->me.x = minus->me.x+ sign*(plus->me.y-base->me.y);
	    mid2->me.y = minus->me.y- sign*(plus->me.x-base->me.x);
	    mid2->nextcp = mid2->prevcp = mid2->me;
	    mid2->nonextcp = mid2->noprevcp = true;
	    mid1->pointtype = pt_corner; mid2->pointtype = pt_corner;
	    SplineMake(plus,mid1);
	    SplineMake(mid1,mid2);
	    SplineMake(mid2,minus);
	} else if ( si->cap==lc_round ) {
	    mid1 = chunkalloc(sizeof(SplinePoint));
	    mid1->me.x = base->me.x+ sign*(plus->me.y-base->me.y);
	    mid1->me.y = base->me.y- sign*(plus->me.x-base->me.x);
	    mid1->pointtype = pt_curve;
	    c = .552*si->radius*cos(angle);
	    s = .552*si->radius*sin(angle);
	    plus->nextcp.x = plus->me.x + c;
	    plus->nextcp.y = plus->me.y + s;
	    minus->prevcp.x = minus->me.x +c;
	    minus->prevcp.y = minus->me.y +s;
	    mid1->prevcp.x = mid1->me.x - sign*s;
	    mid1->prevcp.y = mid1->me.y + sign*c;
	    mid1->nextcp.x = mid1->me.x + sign*s;
	    mid1->nextcp.y = mid1->me.y - sign*c;
	    SplineMake(plus,mid1);
	    SplineMake(mid1,minus);
	}
	*_plus = plus;
	*_minus = minus;
    }
}

/* Is this the inner intersection or the outer one (the inner one is on both splines) */
/*  the outer one is beyond both */
static int IntersectLines(JointPoint *inter,BasePoint *p1,real sx1, real sy1,
	BasePoint *p2, real sx2, real sy2) {
    real t1,t2;
    real denom;

    denom = (sx1*sy2-sx2*sy1);
    if ( denom==0 ) {
	/* Lines are parallel. Might be coincident, might not */
	inter->tprev = 1.0;
	inter->tnext = 0;
	inter->inter.x = (p1->x+p2->x)/2;
	inter->inter.y = (p1->y+p2->y)/2;
	if ( sy1==0 && p1->y==p2->y )
	    t1 = 0;
	else if ( sy1!=0 ) {
	    t2 = p2->y/sy2;
	    t1 = p1->y/sy1;
	    t1 = RealNear(p2->x-t2*sx2,p1->x-t1*sx1)?0: 1;
	} else
	    t1 = 1;
    } else {
	t2 = (sy1*(p2->x-p1->x)-sx1*(p2->y-p1->y))/denom;
	t1 = (sy2*(p2->x-p1->x)-sx2*(p2->y-p1->y))/denom;
	inter->inter.x = p1->x + t1*sx1;
	inter->inter.y = p1->y + t1*sy1;
	inter->tprev = 1+t1;
	inter->tnext = t2;
    }
return( t1<=0 );	/* if t1 < 0 then the intersection point is actually */
			/*  on both of the spline segments. if it isn't then */
			/*  it will be on the continuation of the spline */
			/*  but beyond its endpoint... */
}

static void CirclePoint(TPoint *tp,BasePoint *center,BasePoint *dir,real radius) {
    BasePoint off;
    off.x = dir->x-center->x;
    off.y = dir->y-center->y;
    radius /= sqrt(off.x*off.x+off.y*off.y);
    off.x *= radius;
    off.y *= radius;
    tp->x = center->x + off.x;
    tp->y = center->y + off.y;
}

static void MakeJoints(JointPoint *ret,StrokeInfo *si,
	BasePoint *_from,BasePoint *_to, BasePoint *center) {
    SplinePoint *from, *to, *mid;
    BasePoint temp;
    TPoint approx[4];

    from = chunkalloc(sizeof(SplinePoint));
    to = chunkalloc(sizeof(SplinePoint));
    from->pointtype = to->pointtype = pt_corner;
    from->me = *_from;
    to->me = *_to;
    ret->from = from; ret->to = to;
    if ( si->join==lj_bevel ) {
	from->nextcp = from->me;
	to->prevcp = to->me;
	from->nonextcp = to->noprevcp = true;
	SplineMake(from,to);
    } else if ( si->join == lj_miter ) {
	from->nextcp = from->me;
	to->prevcp = to->me;
	from->nonextcp = to->noprevcp = true;
	mid = chunkalloc(sizeof(SplinePoint));
	mid->me = ret->inter;
	mid->prevcp = mid->nextcp = mid->me;
	mid->noprevcp = mid->nonextcp = true;
	mid->pointtype = pt_corner;
	SplineMake(from,mid);
	SplineMake(mid,to);
    } else {
	from->pointtype = to->pointtype = pt_curve;
	CirclePoint(&approx[0],center,&ret->inter,si->radius); approx[0].t = .5;
	temp.x = (ret->inter.x+from->me.x)/2; temp.y = (ret->inter.y+from->me.y)/2;
	CirclePoint(&approx[1],center,&temp,si->radius); approx[1].t = .25;
	temp.x = (ret->inter.x+to->me.x)/2; temp.y = (ret->inter.y+to->me.y)/2;
	CirclePoint(&approx[2],center,&temp,si->radius); approx[2].t = .75;
	ApproximateSplineFromPoints(from,to,approx,3);
    }
}

/* Plus joins run from prev to next, minus joins run from next to prev */
/* This makes plus joins clockwise and minus joins counter */
static void StrokeJoint(SplinePoint *base,StrokeInfo *si,JointPoint *plus,JointPoint *minus) {
    BasePoint nplus, nminus, pplus,pminus;
    real nangle, pangle;
    int pinner;

    SplineIsLinearMake(base->prev);
    SplineIsLinearMake(base->next);

    pangle = SplineExpand(base->prev,1,si,&pplus,&pminus);
    nangle = SplineExpand(base->next,0,si,&nplus,&nminus);

    plus->tprev = minus->tprev = 1; plus->tnext = minus->tnext = 0;
    if (( base->pointtype==pt_curve && !base->nonextcp && !base->noprevcp ) ||
	    ( base->pointtype==pt_tangent && (!base->nonextcp || !base->noprevcp)) ||
	    RealNearish(pangle,nangle) ) {
	/* If the two splines are tangent at the base, then everything is */
	/*  simple, there is no join, things match up perfectly */
	plus->from = plus->to = chunkalloc(sizeof(SplinePoint));
	plus->from->pointtype = pt_curve;
	plus->from->me.x = (pplus.x + nplus.x)/2;
	plus->from->me.y = (pplus.y + nplus.y)/2;
	minus->from = minus->to = chunkalloc(sizeof(SplinePoint));
	minus->from->me.x = (pminus.x + nminus.x)/2;
	minus->from->me.y = (pminus.y + nminus.y)/2;
	minus->from->pointtype = pt_curve;
    } else {
	pinner = IntersectLines(plus,&pplus,
		 3*base->prev->splines[0].a+2*base->prev->splines[0].b+base->prev->splines[0].c,
		 3*base->prev->splines[1].a+2*base->prev->splines[1].b+base->prev->splines[1].c,
		&nplus,
		 base->next->splines[0].c,
		 base->next->splines[1].c);
	IntersectLines(minus,&pminus,
		 3*base->prev->splines[0].a+2*base->prev->splines[0].b+base->prev->splines[0].c,
		 3*base->prev->splines[1].a+2*base->prev->splines[1].b+base->prev->splines[1].c,
		&nminus,
		 base->next->splines[0].c,
		 base->next->splines[1].c);
	if ( !pinner ) {
	    SplineSet junk;
	    MakeJoints(plus,si,&pplus,&nplus,&base->me);
	    junk.first = plus->from; junk.last = plus->to;
	    SplineSetReverse(&junk);
	    plus->from = junk.first; plus->to = junk.last;
	    plus->tprev = 1; plus->tnext = 0;
	    minus->from = minus->to = chunkalloc(sizeof(SplinePoint));
	    minus->from->me = minus->inter;
	    minus->from->pointtype = pt_corner;
	} else {
	    SplineSet junk;
	    MakeJoints(minus,si,&nminus,&pminus,&base->me);
	    junk.first = minus->from; junk.last = minus->to;
	    SplineSetReverse(&junk);
	    minus->from = junk.first; minus->to = junk.last;
	    minus->tprev = 1; minus->tnext = 0;
	    plus->from = plus->to = chunkalloc(sizeof(SplinePoint));
	    plus->from->me = plus->inter;
	    plus->from->pointtype = pt_corner;
	}
    }
}

#if 0
static void SplineSetFixRidiculous(SplineSet *ss) {
    /* Make sure we don't have any splines with ridiculous control points */
    /* No control point, when projected onto the vector between the two */
    /*  end points should be far beyond either of the end points... */
    Spline *s, *first;
    double vx, vy, test, end;
    int unreasonable;

    first = NULL;
    for ( s=ss->first->next; s!=NULL && s!=first; s = s->to->next ) {
	if ( first==NULL ) first = s;
	vx = s->to->me.x-s->from->me.x; vy = s->to->me.y-s->from->me.y;
	end = vx*vx + vy*vy;
	unreasonable = false;
	test = vx*(s->from->nextcp.x-s->from->me.x) +
		vy*(s->from->nextcp.y-s->from->me.y);
	if ( test<-2*end || test>2*end ) {
	    s->from->nextcp = s->from->me;
	    s->from->nonextcp = true;
	    unreasonable = true;
	}
	test = vx*(s->to->prevcp.x-s->from->me.x) +
		vy*(s->to->prevcp.y-s->from->me.y);
	if ( test<-2*end || test>2*end ) {
	    s->to->prevcp = s->to->me;
	    s->to->noprevcp = true;
	    unreasonable = true;
	}
	if ( unreasonable )
	    SplineRefigure(s);
    }
}
#endif

SplineSet *SplineSetStroke(SplineSet *spl,StrokeInfo *si,SplineChar *sc) {
    JointPoint first_plus, first_minus, cur_plus, cur_minus;
    SplineSet *ssplus, *ssminus;
    SplinePoint *plus, *minus;		/* plus expects splines added on prev */
    					/* minus expects splines added on next*/
    SplinePoint *pto, *mto;
    TPoint pmids[4], mmids[4];
    real p_tlast, m_tlast, p_tcur, m_tcur;
    real t_start, t_end;
    int i;
    Spline *first, *spline;
    int changed = false;

    if ( spl->first==spl->last && spl->first->next!=NULL ) {
	/* My routine gets screwed up by counter-clockwise triangles */
	if ( !SplinePointListIsClockwise(spl))
	    SplineSetReverse(spl);
	/* It's a loop, we'll return two SplineSets */
	StrokeJoint(spl->first,si,&first_plus,&first_minus);
	plus = first_plus.from;
	minus = first_minus.to;
	p_tlast = first_plus.tprev;
	m_tlast = first_minus.tnext;
    } else if ( spl->first->next==NULL ) {
	/* Only one point in the SplineSet. */
	ssplus = chunkalloc(sizeof(SplineSet));
	StrokeEnd(spl->first,si,&ssplus->first,&ssplus->last);
return( ssplus );
    } else {
	StrokeEnd(spl->first,si,&plus,&minus);
	p_tlast = 1.0; m_tlast = 0;
    }

    first = NULL;
    for ( spline = spl->first->next; spline!=NULL && spline!=first; spline = spline->to->next ) {
	if ( spline->to->next!=NULL ) {
	    if ( spline->to == spl->first ) {
		cur_plus = first_plus;
		cur_minus = first_minus;
	    } else
		StrokeJoint(spline->to,si,&cur_plus,&cur_minus);
	    p_tcur = cur_plus.tnext;
	    m_tcur = cur_minus.tprev;
	    pto = cur_plus.to;
	    mto = cur_minus.from;
	} else {
	    SplineSet junk;
	    StrokeEnd(spline->to,si,&junk.first,&junk.last);
	    SplineSetReverse(&junk);
	    pto = junk.last;
	    mto = junk.first;
	    p_tcur = 0.0; m_tcur = 1.0;
	}
	t_start = (p_tcur>m_tlast)?p_tcur:m_tlast;
	t_end = (p_tlast<m_tcur)?p_tlast:m_tcur;
	if (( p_tcur>=p_tlast || m_tcur<=m_tlast ) && !si->toobigwarn ) {
	    si->toobigwarn = true;
	    GWidgetErrorR( _STR_BadStroke, _STR_StrokeWidthTooBig,
		    sc==NULL?"<nameless char>": sc->name );
	}
	if ( spline->knownlinear ||
/* 0 and 1 are valid values. They happen on circles for example */
		p_tcur<0 || m_tcur>1 || m_tlast<0 || p_tlast>1 ||
		m_tcur<=m_tlast || p_tcur>=p_tlast ) {
	    pto->nonextcp = plus->noprevcp = true;
	    minus->nonextcp = mto->noprevcp = true;
	    SplineMake(pto,plus);
	    SplineMake(minus,mto);
	    if ( plus->nonextcp && plus->noprevcp ) plus->pointtype = pt_corner;
	    if ( minus->nonextcp && minus->noprevcp ) minus->pointtype = pt_corner;
	} else {
	    for ( i=0; i<4; ++i ) {
		BasePoint p,m;
		real t = t_start + (i+1)*(t_end-t_start)/5;
		pmids[i].t = (t-p_tlast)/(p_tcur-p_tlast);
		mmids[i].t = (t-m_tlast)/(m_tcur-m_tlast);
		SplineExpand(spline,t,si,&p,&m);
		pmids[i].x = p.x; pmids[i].y = p.y;
		mmids[i].x = m.x; mmids[i].y = m.y;
	    }
	    ApproximateSplineFromPoints(pto,plus,pmids,4);
	    ApproximateSplineFromPoints(minus,mto,mmids,4);
	}
	if ( spline->to->next!=NULL ) {
	    plus = cur_plus.from;
	    minus = cur_minus.to;
	    p_tlast = cur_plus.tprev;
	    m_tlast = cur_minus.tnext;
	} else {
	    /* Done */
    break;
	}
	if ( first==NULL ) first = spline;
    }

    ssplus = chunkalloc(sizeof(SplineSet));
    ssplus->first = ssplus->last = plus;
    if ( spl->first==spl->last ) {
	ssminus = chunkalloc(sizeof(SplineSet));
	ssminus->first = ssminus->last = minus;
	/*SplineSetFixRidiculous(ssplus); SplineSetFixRidiculous(ssminus);*/
	if ( SplinePointListIsClockwise(ssplus))
	    SplineSetReverse(ssplus);
	ssplus->next = ssminus;
	SplineSetsCorrect(ssplus,&changed);
    } else {
	/*SplineSetFixRidiculous(ssplus);*/
	if ( !SplinePointListIsClockwise(ssplus))
	    SplineSetReverse(ssplus);
    }
return( ssplus );
}
