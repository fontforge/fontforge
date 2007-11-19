/* Copyright (C) 2005-2007 by George Williams and Alexey Kryukov */
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
#include "edgelist2.h"
#include "stemdb.h"
#include <math.h>
#define GLYPH_DATA_DEBUG 0

/* A diagonal end is like the top or bottom of a slash. Should we add a vertical stem at the end? */
/* A diagonal corner is like the bottom of circumflex. Should we add a horizontal stem? */
int hint_diagonal_ends = 0,
    hint_diagonal_intersections = 0,
    hint_bounding_boxes = 1,
    detect_diagonal_stems = 0;

static const double slope_error = .05;
    /* If the dot vector of a slope and the normal of another slope is less */
    /*  than this error, then we say they are close enough to be colinear */
static double dist_error_hv = 3.5;
static double dist_error_diag = 8;
    /* It's easy to get horizontal/vertical lines aligned properly */
    /* it is more difficult to get diagonal ones done */
    /* The "A" glyph in Apple's Times.dfont(Roman) is off by 6 in one spot */
static double dist_error_curve = 22;
    /* The maximum possible distance between the edge of an active zone for
    /* a curved spline segment and the actual spline */

struct st {
    Spline *s;
    double st, lt;
};

static int IsVectorHV( BasePoint *vec,double fudge,int check_zero ) {
    if ( check_zero ) {
        if ( vec->x >= -fudge && vec->x <= fudge )
return( 2 );
        else if ( vec->y >= -fudge && vec->y <= fudge )
return( 1 );
    } else {
        if ( vec->x >= 1-fudge || vec->x <= -1+fudge )
return( 1 );
        else if ( vec->y >= 1-fudge || vec->y <= -1+fudge )
return( 2 );
    }
return( 0 );
}

static int VectorCloserToHV( BasePoint *vec1,BasePoint *vec2 ) {
    int hv1, hv2;
    double test1=0, test2=0;
    
    hv1 = IsVectorHV( vec1,slope_error,false );
    hv2 = IsVectorHV( vec2,slope_error,false );
    
    if ( hv1 == 1 && hv2 == 1 ) {
        test1 = ( vec1->x > 0 ) ? vec1->x : -vec1->x;
        test2 = ( vec2->x > 0 ) ? vec2->x : -vec2->x;
    } else if ( hv1 == 2 && hv2 == 2 ) {
        test1 = ( vec1->y > 0 ) ? vec1->y : -vec1->y;
        test2 = ( vec2->y > 0 ) ? vec2->y : -vec2->y;
    }

    if ( test1 > test2 )
return( 1 );
    else if ( test1 < test2 )
return( -1 );

return( 0 );
}

static int UnitsOrthogonal( BasePoint *u1,BasePoint *u2,int strict ) {
    double dot;
    
    if ( strict ) {
        dot = u1->x*u2->x + u1->y*u2->y;
return( dot>-.05 && dot < .05 );
    } else {
        dot = u1->x*u2->y - u1->y*u2->x;
return( dot>.95 || dot < -.95 );
    }
}

int UnitsParallel( BasePoint *u1,BasePoint *u2,int strict ) {
    double dot;
    
    if ( strict ) {
        dot = u1->x*u2->y - u1->y*u2->x;
return( dot>-.05 && dot < .05 );
    } else {
        dot = u1->x*u2->x + u1->y*u2->y;
return( dot>.95 || dot < -.95 );
    }
}

static int SplineFigureOpticalSlope(Spline *s,int start_at_from,BasePoint *dir) {
    /* Sometimes splines have tiny control points, and to the eye the slope */
    /*  of the spline has nothing to do with that specified by the cps. */
    /* So see if the spline is straightish and figure the slope based on */
    /*  some average direction */
    /* dir is a input output parameter. */
    /*  it should be initialized to the unit vector determined by the appropriate cp */
    /*  if the function returns true, it will be set to a unit vector in the average direction */
    BasePoint pos, *base, average_dir, normal;
    double t, len, incr, off;

    /* The vector is already nearly vertical/horizontal, no need to modify*/
    if ( IsVectorHV( dir,slope_error,true ))
return( false );
    
    if ( start_at_from ) {
	incr = -.1;
	base = &s->from->me;
    } else {
	incr = .1;
	base = &s->to->me;
    }

    t = .5-incr;
    memset(&average_dir,0,sizeof(average_dir));
    while ( t>0 && t<1.0 ) {
	pos.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	pos.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;

	average_dir.x += (pos.x-base->x); average_dir.y += (pos.y-base->y);
	t += incr;
    }

    len = sqrt( pow( average_dir.x,2 ) + pow( average_dir.y,2 ));
    if ( len==0 )
return( false );
    average_dir.x /= len; average_dir.y /= len;
    normal.x = average_dir.y; normal.y = - average_dir.x;

    t = .5-incr;
    while ( t>0 && t<1.0 ) {
	pos.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	pos.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
	off = (pos.x-base->x)*normal.x + (pos.y-base->y)*normal.y;
	if ( off<-dist_error_hv || off>dist_error_hv )
return( false );
	t += incr;
    }

    off = dir->x*normal.x + dir->y*normal.y;
    if ( off<slope_error ) {
	/* prefer the direction which is closer to horizontal/vertical */
	double dx, dy, ax, ay, d, a;
	if ( (dx=dir->x)<0 ) dx = -dx;
	if ( (dy=dir->y)<0 ) dy = -dy;
	d = (dx<dy) ? dx : dy;
	if ( (ax=average_dir.x)<0 ) ax = -ax;
	if ( (ay=average_dir.y)<0 ) ay = -ay;
	a = (ax<ay) ? ax : ay;
	if ( d<a )
return( false );
    }

    *dir = average_dir;
return( true );
}

static void PointInit(struct glyphdata *gd,SplinePoint *sp, SplineSet *ss) {
    struct pointdata *pd;
    double len;

    if ( sp->ttfindex==0xffff || sp->ttfindex==0xfffe )
return;
    pd = &gd->points[sp->ttfindex];
    pd->sp = sp;
    pd->ss = ss;

    if ( sp->next==NULL ) {
	pd->nextunit.x = ss->first->me.x - sp->me.x;
	pd->nextunit.y = ss->first->me.y - sp->me.y;
	pd->nextlinear = true;
    } else if ( sp->next->knownlinear ) {
	pd->nextunit.x = sp->next->to->me.x - sp->me.x;
	pd->nextunit.y = sp->next->to->me.y - sp->me.y;
	pd->nextlinear = true;
    } else if ( sp->nonextcp ) {
	pd->nextunit.x = sp->next->to->prevcp.x - sp->me.x;
	pd->nextunit.y = sp->next->to->prevcp.y - sp->me.y;
    } else {
	pd->nextunit.x = sp->nextcp.x - sp->me.x;
	pd->nextunit.y = sp->nextcp.y - sp->me.y;
    }
    len = sqrt( pow( pd->nextunit.x,2 ) + pow( pd->nextunit.y,2 ));
    if ( len==0 )
	pd->nextzero = true;
    else {
	pd->nextlen = len;
	pd->nextunit.x /= len;
	pd->nextunit.y /= len;
	if ( sp->next!=NULL && !sp->next->knownlinear )
	    SplineFigureOpticalSlope(sp->next,true,&pd->nextunit);
	if ( IsVectorHV( &pd->nextunit,slope_error,true ) == 2 ) {
	    pd->nextunit.x = 0; pd->nextunit.y = pd->nextunit.y>0 ? 1 : -1;
	} else if ( IsVectorHV( &pd->nextunit,slope_error,true ) == 1 ) {
	    pd->nextunit.y = 0; pd->nextunit.x = pd->nextunit.x>0 ? 1 : -1;
	}
	if ( pd->nextunit.y==0 ) pd->next_hor = true;
	else if ( pd->nextunit.x==0 ) pd->next_ver = true;
    }

    if ( sp->prev==NULL ) {
	pd->prevunit.x = ss->last->me.x - sp->me.x;
	pd->prevunit.y = ss->last->me.y - sp->me.y;
	pd->prevlinear = true;
    } else if ( sp->prev->knownlinear ) {
	pd->prevunit.x = sp->prev->from->me.x - sp->me.x;
	pd->prevunit.y = sp->prev->from->me.y - sp->me.y;
	pd->prevlinear = true;
    } else if ( sp->noprevcp ) {
	pd->prevunit.x = sp->prev->from->nextcp.x - sp->me.x;
	pd->prevunit.y = sp->prev->from->nextcp.y - sp->me.y;
    } else {
	pd->prevunit.x = sp->prevcp.x - sp->me.x;
	pd->prevunit.y = sp->prevcp.y - sp->me.y;
    }
    len = sqrt( pow( pd->prevunit.x,2 ) + pow( pd->prevunit.y,2 ));
    if ( len==0 )
	pd->prevzero = true;
    else {
	pd->prevlen = len;
	pd->prevunit.x /= len;
	pd->prevunit.y /= len;
	if ( sp->prev!=NULL && !sp->prev->knownlinear )
	    SplineFigureOpticalSlope(sp->prev,false,&pd->prevunit);
	if ( pd->prevunit.x<slope_error && pd->prevunit.x>-slope_error ) {
	    pd->prevunit.x = 0; pd->prevunit.y = pd->prevunit.y>0 ? 1 : -1;
	} else if ( pd->prevunit.y<slope_error && pd->prevunit.y>-slope_error ) {
	    pd->prevunit.y = 0; pd->prevunit.x = pd->prevunit.x>0 ? 1 : -1;
	}
	if ( pd->prevunit.y==0 ) pd->prev_hor = true;
	else if ( pd->prevunit.x==0 ) pd->prev_ver = true;
    }
    {
	double same = pd->prevunit.x*pd->nextunit.x + pd->prevunit.y*pd->nextunit.y;
	if ( same<-.95 )
	    pd->colinear = true;
    }
    if ( hint_diagonal_intersections ) {
	if ( pd->prev_hor || pd->prev_ver )
	    /* It IS horizontal or vertical. No need to fake it */;
	else if ( RealNear(pd->nextunit.x,-pd->prevunit.x) &&
		RealNear(pd->nextunit.y,pd->prevunit.y) && !pd->nextzero)
	    pd->symetrical_h = true;
	else if ( RealNear(pd->nextunit.y,-pd->prevunit.y) &&
		RealNear(pd->nextunit.x,pd->prevunit.x) && !pd->nextzero)
	    pd->symetrical_v = true;
    }
}

static int BBoxIntersectsLine(Spline *s,Spline *line) {
    double t,x,y;
    DBounds b;

    b.minx = b.maxx = s->from->me.x;
    b.miny = b.maxy = s->from->me.y;
    if ( s->to->me.x<b.minx ) b.minx = s->to->me.x;
    else if ( s->to->me.x>b.maxx ) b.maxx = s->to->me.x;
    if ( s->to->me.y<b.miny ) b.miny = s->to->me.y;
    else if ( s->to->me.y>b.maxy ) b.maxy = s->to->me.y;
    if ( s->to->prevcp.x<b.minx ) b.minx = s->to->prevcp.x;
    else if ( s->to->prevcp.x>b.maxx ) b.maxx = s->to->prevcp.x;
    if ( s->to->prevcp.y<b.miny ) b.miny = s->to->prevcp.y;
    else if ( s->to->prevcp.y>b.maxy ) b.maxy = s->to->prevcp.y;
    if ( s->from->nextcp.x<b.minx ) b.minx = s->from->nextcp.x;
    else if ( s->from->nextcp.x>b.maxx ) b.maxx = s->from->nextcp.x;
    if ( s->from->nextcp.y<b.miny ) b.miny = s->from->nextcp.y;
    else if ( s->from->nextcp.y>b.maxy ) b.maxy = s->from->nextcp.y;

    if ( line->splines[0].c!=0 ) {
	t = (b.minx-line->splines[0].d)/line->splines[0].c;
	y = line->splines[1].c*t+line->splines[1].d;
	if ( y>=b.miny && y<=b.maxy )
return( true );
	t = (b.maxx-line->splines[0].d)/line->splines[0].c;
	y = line->splines[1].c*t+line->splines[1].d;
	if ( y>=b.miny && y<=b.maxy )
return( true );
    }
    if ( line->splines[1].c!=0 ) {
	t = (b.miny-line->splines[1].d)/line->splines[1].c;
	x = line->splines[0].c*t+line->splines[0].d;
	if ( x>=b.minx && x<=b.maxx )
return( true );
	t = (b.maxy-line->splines[1].d)/line->splines[1].c;
	x = line->splines[0].c*t+line->splines[0].d;
	if ( x>=b.minx && x<=b.maxx )
return( true );
    }
return( false );
}

static int stcmp(const void *_p1, const void *_p2) {
    const struct st *stpt1 = _p1, *stpt2 = _p2;
    if ( stpt1->lt>stpt2->lt )
return( 1 );
    else if ( stpt1->lt<stpt2->lt )
return( -1 );

return( 0 );
}

static int LineType(struct st *st,int i, int cnt,Spline *line) {
    SplinePoint *sp;
    BasePoint nextcp, prevcp, here;
    double dn, dp;

    if ( st[i].st>.01 && st[i].st<.99 )
return( 0 );		/* Not near an end-point, just a normal line */
    if ( i+1>=cnt )
return( 0 );		/* No following spline */
    if ( st[i+1].st>.01 && st[i+1].st<.99 )
return( 0 );		/* Following spline not near an end-point, can't */
			/*  match to this one, just a normal line */
    if ( st[i].st<.5 && st[i+1].st>.5 ) {
	if ( st[i+1].s->to->next!=st[i].s )
return( 0 );
	sp = st[i].s->from;
    } else if ( st[i].st>.5 && st[i+1].st<.5 ) {
	if ( st[i].s->to->next!=st[i+1].s )
return( 0 );
	sp = st[i].s->to;
    } else
return( 0 );

    if ( !sp->nonextcp )
	nextcp = sp->nextcp;
    else
	nextcp = sp->next->to->me;
    if ( !sp->noprevcp )
	prevcp = sp->prevcp;
    else
	prevcp = sp->prev->from->me;
    here.x = line->splines[0].c*(st[i].st+st[i+1].st)/2 + line->splines[0].d;
    here.y = line->splines[1].c*(st[i].st+st[i+1].st)/2 + line->splines[1].d;

    nextcp.x -= here.x; nextcp.y -= here.y;
    prevcp.x -= here.x; prevcp.y -= here.y;

    dn = nextcp.x*line->splines[1].c - nextcp.y*line->splines[0].c;
    dp = prevcp.x*line->splines[1].c - prevcp.y*line->splines[0].c;
    if ( dn*dp<0 )	/* splines away move on opposite sides of the line */
return( 1 );		/* Treat this line and the next as one */
			/* We assume that a rounding error gave us one erroneous intersection (or we went directly through the endpoint) */
    else
return( 2 );		/* Ignore both this line and the next */
			/* Intersects both in a normal fashion */
}

static int MonotonicOrder(Spline **sspace,Spline *line,struct st *stspace) {
    Spline *s;
    int i,j,k,cnt;
    BasePoint pts[9];
    extended lts[10], sts[10];

    for ( i=j=0; (s=sspace[j])!=NULL; ++j ) {
	if ( BBoxIntersectsLine(s,line) ) {
	    /* Lines parallel to the direction we are testing just get in the */
	    /*  way and don't add any useful info */
	    if ( s->islinear &&
		    RealNear(line->splines[0].c*s->splines[1].c,
			    line->splines[1].c*s->splines[0].c))
    continue;
	    if ( SplinesIntersect(line,s,pts,lts,sts)<=0 )
    continue;
	    for ( k=0; sts[k]!=-1; ++k ) {
		if ( sts[k]>=0 && sts[k]<=1 ) {
		    stspace[i].s    = s;
		    stspace[i].lt   = lts[k];
		    stspace[i++].st = sts[k];
		}
	    }
	}
    }
    stspace[i].s = NULL;
    cnt = i;
    qsort(stspace,cnt,sizeof(struct st),stcmp);
return( cnt );
}

static Spline *MonotonicFindAlong(Spline *line,struct st *stspace,int cnt,
	Spline *findme, double *other_t) {
    Spline *s;
    int i;
    int eo;		/* I do horizontal/vertical by winding number */
			/* But figuring winding number with respect to a */
			/* diagonal line is hard. So I use even-odd */
			/* instead. */

    eo = 0;
    for ( i=0; i<cnt; ++i ) {
	s = stspace[i].s;
	if ( s==findme ) {
	    if ( (eo&1) && i>0 ) {
		*other_t = stspace[i-1].st;
return( stspace[i-1].s );
	    } else if ( !(eo&1) && i+1<cnt ) {
		*other_t = stspace[i+1].st;
return( stspace[i+1].s );
	    }
	    fprintf( stderr, "MonotonicFindAlong: Ran out of intersections.\n" );
return( NULL );
	}
	if ( i+1<cnt && stspace[i+1].s==findme )
	    ++eo;
	else switch ( LineType(stspace,i,cnt,line) ) {
	  case 0:	/* Normal spline */
	    ++eo;
	  break;
	  case 1:	/* Intersects at end-point & next entry is other side */
	    ++eo;	/*  And the two sides continue in approximately the   */
	    ++i;	/*  same direction */
	  break;
	  case 2:	/* Intersects at end-point & next entry is other side */
	    ++i;	/*  And the two sides go in opposite directions */
	  break;
	}
    }
    fprintf( stderr, "MonotonicFindAlong: Never found our spline.\n" );
return( NULL );
}

static int MonotonicFindStemBounds(Spline *line,struct st *stspace,int cnt,
	double fudge,struct stemdata *stem ) {
    int i,j;
    int eo;		/* I do horizontal/vertical by winding number */
			/* But figuring winding number with respect to a */
			/* diagonal line is hard. So I use even-odd */
			/* instead. */
    double pos, npos;
    double width = stem->width;
    double lmin = ( stem->lmin < -fudge ) ? stem->lmin : -fudge;
    double lmax = ( stem->lmax > fudge ) ? stem->lmax : fudge;
    double rmin = ( stem->rmin < -fudge ) ? stem->rmin : -fudge;
    double rmax = ( stem->rmax > fudge ) ? stem->rmax : fudge;

    eo = 0;
    for ( i=0; i<cnt; ++i ) {
	pos =   (line->splines[0].c*stspace[i].lt + line->splines[0].d - stem->left.x)*stem->l_to_r.x +
	        (line->splines[1].c*stspace[i].lt + line->splines[1].d - stem->left.y)*stem->l_to_r.y;
	npos = 1e4;
	if ( i+1<cnt )
	    npos = (line->splines[0].c*stspace[i+1].lt + line->splines[0].d - stem->left.x)*stem->l_to_r.x +
		   (line->splines[1].c*stspace[i+1].lt + line->splines[1].d - stem->left.y)*stem->l_to_r.y;

	if ( pos>=lmin && pos<=lmax ) {
	    if ( (eo&1) && i>0 )
		j = i-1;
	    else if ( !(eo&1) && i+1<cnt )
		j = i+1;
	    else
return( false );
	    pos = (line->splines[0].c*stspace[j].lt + line->splines[0].d - stem->left.x)*stem->l_to_r.x +
		  (line->splines[1].c*stspace[j].lt + line->splines[1].d - stem->left.y)*stem->l_to_r.y;
            if ( pos >= width+rmin && pos <= width+rmax )
return( true );
	}
	if ( i+1 < cnt && npos >= lmin && npos <= lmax )
	    ++eo;
	else switch ( LineType(stspace,i,cnt,line) ) {
	  case 0:	/* Normal spline */
	    ++eo;
	  break;
	  case 1:	/* Intersects at end-point & next entry is other side */
	    ++eo;	/*  And the two sides continue in approximately the   */
	    ++i;	/*  same direction */
	  break;
	  case 2:	/* Intersects at end-point & next entry is other side */
	    ++i;	/*  And the two sides go in opposite directions */
	  break;
	}
    }
return( false );
}

static int MatchWinding(struct monotonic ** space,int i,int nw,int winding,int which) {
    struct monotonic *m;
    int j;

    if (( nw<0 && winding>0 ) || (nw>0 && winding<0)) {
	winding = nw;
	for ( j=i-1; j>=0; --j ) {
	    m = space[j];
	    winding += ((&m->xup)[which] ? 1 : -1 );
	    if ( winding==0 )
return( j );
	}
    } else {
	winding = nw;
	for ( j=i+1; space[j]!=NULL; ++j ) {
	    m = space[j];
	    winding += ((&m->xup)[which] ? 1 : -1 );
	    if ( winding==0 )
return( j );
	}
    }
return( -1 );
}

static Spline *FindMatchingHVEdge(struct glyphdata *gd, struct pointdata *pd,
	int is_next, double *other_t, double *dist ) {
    double test, t, start, end;
    int which;
    Spline *s;
    Monotonic *m;
    int winding, nw, i,j;
    struct monotonic **space;
    BasePoint *dir, d, hv;

    /* Things are difficult if we go exactly through the point. Move off */
    /*  to the side a tiny bit and hope that doesn't matter */
    if ( is_next==2 ) {
	/* Consider the case of the bottom of the circumflex (or a chevron) */
	/*  Think of it as a flattend breve. It is symetrical and we want to */
	/*  note the vertical distance between the two points that define */
	/*  the bottom, so treat them as a funky stem */
	/*                 \ \     / /              */
	/*                  \ \   / /               */
	/*                   \ \ / /                */
	/*                    \ + /                 */
	/*                     \ /                  */
	/*                      +                   */
	hv.x = pd->symetrical_h ? 1.0 : 0.0;
	hv.y = pd->symetrical_v ? 1.0 : 0.0;
	dir = &hv;
	t = .001;
	s = pd->sp->next;		/* Could just as easily be prev */
    } else if ( is_next ) {
	s = pd->sp->next;
	t = .001;
	dir = &pd->nextunit;
    } else {
	s = pd->sp->prev;
	t = .999;
	dir = &pd->prevunit;
    }
    if ( (d.x = dir->x)<0 ) d.x = -d.x;
    if ( (d.y = dir->y)<0 ) d.y = -d.y;
    which = d.x<d.y;		/* closer to vertical */

    if ( s==NULL )		/* Somehow we got an open contour? */
return( NULL );

    test = ((s->splines[which].a*t+s->splines[which].b)*t+s->splines[which].c)*t+s->splines[which].d;
    MonotonicFindAt(gd->ms,which,test,space = gd->space);

    winding = 0;
    for ( i=0; space[i]!=NULL; ++i ) {
	m = space[i];
	nw = ((&m->xup)[which] ? 1 : -1 );
	if ( m->s == s && t>=m->tstart && t<=m->tend ) {
            start = m->other;
    break;
        }
	winding += nw;
    }
    if ( space[i]==NULL ) {
	fprintf( stderr, "FindMatchinHVEdge didn't\n" );
return( NULL );
    }

    j = MatchWinding(space,i,nw,winding,which);
    if ( j!=-1 ) {
	*other_t = space[j]->t;
        end = space[j]->other;
        *dist = end - start;
        if ( *dist < 0 ) *dist = -(*dist);
return( space[j]->s );
    }

    fprintf( stderr, "FindMatchingHVEdge fell into an impossible position\n" );
return( NULL );
}

static void MakeVirtualLine(struct glyphdata *gd,BasePoint *perturbed,
	BasePoint *dir,Spline *myline,SplinePoint *end1, SplinePoint *end2) {
    BasePoint norm, absnorm;
    double t1, t2;

    if ( gd->stspace==NULL ) {
	int i, cnt;
	SplineSet *spl;
	Spline *s, *first;
	for ( i=0; i<2; ++i ) {
	    cnt = 0;
	    for ( spl=gd->sc->layers[ly_fore].splines; spl!=NULL; spl=spl->next ) {
		first = NULL;
		if ( spl->first->prev!=NULL ) {
		    for ( s=spl->first->next; s!=first; s=s->to->next ) {
			if ( first==NULL ) first = s;
			if ( i )
			    gd->sspace[cnt] = s;
			++cnt;
		    }
		}
	    }
	    if ( !i ) {
		gd->scnt = cnt;
		gd->sspace = galloc((cnt+1)*sizeof(Spline *));
	    } else
		gd->sspace[cnt] = NULL;
	}
	gd->stspace = galloc((3*cnt+2)*sizeof(struct st));
	SplineCharFindBounds(gd->sc,&gd->size);
	gd->size.minx -= 10; gd->size.miny -= 10;
	gd->size.maxx += 10; gd->size.maxy += 10;
    }

    norm.x = -dir->y;
    norm.y = dir->x;
    absnorm = norm;
    if ( absnorm.x<0 ) absnorm.x = -absnorm.x;
    if ( absnorm.y<0 ) absnorm.y = -absnorm.y;

    memset(myline,0,sizeof(*myline));
    memset(end1,0,sizeof(*end1));
    memset(end2,0,sizeof(*end2));
    myline->knownlinear = myline->islinear = true;

    if ( absnorm.x > absnorm.y ) {
	/* Greater change in x than in y */
	t1 = (gd->size.minx-perturbed->x)/norm.x;
	t2 = (gd->size.maxx-perturbed->x)/norm.x;
	myline->splines[0].d = gd->size.minx;
	myline->splines[0].c = gd->size.maxx-gd->size.minx;
	myline->splines[1].d = perturbed->y+t1*norm.y;
	myline->splines[1].c = (t2-t1)*norm.y;
    } else {
	t1 = (gd->size.miny-perturbed->y)/norm.y;
	t2 = (gd->size.maxy-perturbed->y)/norm.y;
	myline->splines[1].d = gd->size.miny;
	myline->splines[1].c = gd->size.maxy-gd->size.miny;
	myline->splines[0].d = perturbed->x+t1*norm.x;
	myline->splines[0].c = (t2-t1)*norm.x;
    }
    end1->me.x = myline->splines[0].d;
    end2->me.x = myline->splines[0].d + myline->splines[0].c;
    end1->me.y = myline->splines[1].d;
    end2->me.y = myline->splines[1].d + myline->splines[1].c;
    end1->nextcp = end1->prevcp = end1->me;
    end2->nextcp = end2->prevcp = end2->me;
    end1->nonextcp = end1->noprevcp = end2->nonextcp = end2->noprevcp = true;
    end1->next = myline; end2->prev = myline;
    myline->from = end1; myline->to = end2;
}

static Spline *FindMatchingEdge( struct glyphdata *gd, struct pointdata *pd,
	int is_next ) {
    BasePoint *dir, perturbed, diff;
    Spline myline;
    SplinePoint end1, end2;
    double *other_t = is_next==2 ? &pd->both_e_t : is_next ? &pd->next_e_t : &pd->prev_e_t;
    double *dist = is_next ? &pd->next_dist : &pd->prev_dist;
    double t;
    Spline *s;
    int cnt;

    *dist = 0;
    if (( is_next && ( pd->next_hor || pd->next_ver )) ||
        ( !is_next && ( pd->prev_hor || pd->prev_ver )) ||
        is_next == 2 )
return( FindMatchingHVEdge(gd,pd,is_next,other_t,dist));

    if ( is_next ) {
	dir = &pd->nextunit;
	t = .001;
	s = pd->sp->next;
    } else {
	dir = &pd->prevunit;
	t = .999;
	s = pd->sp->prev;
    }

    if ( s==NULL || ( gd->only_hv && !IsVectorHV( dir,slope_error,false )))
return( NULL );

    diff.x = s->to->me.x-s->from->me.x; diff.y = s->to->me.y-s->from->me.y;
    if ( diff.x<.03 && diff.x>-.03 && diff.y<.03 && diff.y>-.03 )
return( NULL );

    /* Don't base the line on the current point, we run into rounding errors */
    /*  where lines that should intersect it don't. Instead perturb it a tiny*/
    /*  bit in the direction along the spline */
    forever {
	perturbed.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	perturbed.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
	if ( !RealWithin(perturbed.x,pd->sp->me.x,.01) ||
		!RealWithin(perturbed.y,pd->sp->me.y,.01) )
    break;
	if ( t<.5 ) {
	    t *= 2;
	    if ( t>.5 )
    break;
	} else {
	    t = 1- 2*(1-t);
	    if ( t<.5 )
    break;
	}
    }

    MakeVirtualLine(gd,&perturbed,dir,&myline,&end1,&end2);
    /* prev_e_t = next_e_t = both_e_t =. This is where these guys are set */
    cnt = MonotonicOrder(gd->sspace,&myline,gd->stspace);
return( MonotonicFindAlong(&myline,gd->stspace,cnt,s,other_t));
}

static int StillStem(struct glyphdata *gd,double fudge,BasePoint *pos,struct stemdata *stem ) {
    Spline myline;
    SplinePoint end1, end2;
    int cnt, ret;

    MakeVirtualLine(gd,pos,&stem->unit,&myline,&end1,&end2);
    cnt = MonotonicOrder(gd->sspace,&myline,gd->stspace);
    ret = MonotonicFindStemBounds(&myline,gd->stspace,cnt,fudge,stem);
return( ret );
}

/* In TrueType I want to make sure that everything on a diagonal line remains */
/*  on the same line. Hence we compute the line. Also we are interested in */
/*  points that are on the intersection of two lines */
static struct linedata *BuildLine(struct glyphdata *gd,struct pointdata *pd,int is_next ) {
    int i;
    BasePoint *dir, *base;
    struct pointdata **pspace = gd->pspace;
    int pcnt=0;
    double dist_error;
    struct linedata *line;
    double n_s_err, p_s_err, off, lmin=0, lmax=0;

    dir = is_next ? &pd->nextunit : &pd->prevunit;
    dist_error = ( IsVectorHV( dir,0,true )) ? dist_error_hv : dist_error_diag ;	/* Diagonals are harder to align */
    if ( dir->x==0 && dir->y==0 )
return( NULL );
    base = &pd->sp->me;

    for ( i= (pd - gd->points)+1; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd2 = &gd->points[i];
	off = (pd2->sp->me.x-base->x)*dir->y - (pd2->sp->me.y-base->y)*dir->x;
	if ( off <= lmax - 2*dist_error || off >= lmin + 2*dist_error )
    continue;
        if ( off < 0 && off < lmin ) lmin = off;
        else if ( off > 0 && off > lmax ) lmax = off;
	n_s_err = dir->x*pd2->nextunit.y - dir->y*pd2->nextunit.x;
	p_s_err = dir->x*pd2->prevunit.y - dir->y*pd2->prevunit.x;
	if ( (n_s_err<slope_error && n_s_err>-slope_error && pd2->nextline==NULL ) ||
		(p_s_err<slope_error && p_s_err>-slope_error && pd2->prevline==NULL ))
	    pspace[pcnt++] = pd2;
    }

    if ( pcnt==0 )
return( NULL );
    if ( pcnt==1 ) {
	/* if the line consists of just these two points, only count it as */
	/*  a true line if the two form a line */
	if ( (pd->sp->next->to!=pspace[0]->sp || !pd->sp->next->knownlinear) &&
		( pd->sp->prev->from!=pspace[0]->sp || !pd->sp->prev->knownlinear ))
return( NULL );
    }

    line = &gd->lines[gd->linecnt++];
    line->pcnt = pcnt+1;
    line->points = galloc((pcnt+1)*sizeof(struct pointdata *));
    line->points[0] = pd;
    line->unitvector = *dir;
    line->online = *base;
    for ( i=0; i<pcnt; ++i ) {
	n_s_err = dir->x*pspace[i]->nextunit.y - dir->y*pspace[i]->nextunit.x;
	p_s_err = dir->x*pspace[i]->prevunit.y - dir->y*pspace[i]->prevunit.x;
	if ( n_s_err<slope_error && n_s_err>-slope_error && pspace[i]->nextline==NULL ) {
	    pspace[i]->nextline = line;
	    if ( pspace[i]->colinear )
		pspace[i]->prevline = line;
	}
	if ( p_s_err<slope_error && p_s_err>-slope_error && pspace[i]->prevline==NULL ) {
	    pspace[i]->prevline = line;
	    if ( pspace[i]->colinear )
		pspace[i]->nextline = line;
	}
	line->points[i+1] = pspace[i];
    }
return( line );
}

static int IsStubOrIntersection( struct glyphdata *gd, BasePoint *dir1, 
    struct pointdata *pd1, struct pointdata *pd2, int is_next1, int is_next2 ) {
    int i;
    int exc=0;
    double dist, off, ext, err, norm1, norm2, opp;
    SplinePoint *sp1, *sp2, *nsp;
    BasePoint *dir2, *odir1, *odir2;
    struct pointdata *npd;
    struct linedata *line;
    
    sp1 = pd1->sp; sp2 = pd2->sp;
    dir2 = ( is_next2 ) ? &pd2->nextunit : &pd2->prevunit;
    line = is_next2 ? pd2->nextline : pd2->prevline;
    if ( !IsVectorHV( dir2,slope_error,true ) && line != NULL )
        dir2 = &line->unitvector;

    odir1 = ( is_next1 ) ? &pd1->prevunit : &pd1->nextunit;
    odir2 = ( is_next2 ) ? &pd2->prevunit : &pd2->nextunit;
    
    err = dir2->x*dir1->y - dir2->y*dir1->x;
    if ( err > 3*slope_error || err < -3*slope_error )
return( 0 );

    /* First check if it is a slightly slanted line or a curve which joins
    /* a straight line under an angle close to 90 degrees. There are many
    /* glyphs where circles or curved features are intersected by or
    /* connected to vertical or horizontal straight stems (the most obvious
    /* cases are Greek Psi and Cyrillic Yu), and usually it is highly desired to
    /* mark such an intersection with a hint */
    norm1 = ( sp1->me.x - sp2->me.x ) * odir2->x +
            ( sp1->me.y - sp2->me.y ) * odir2->y;
    norm2 = ( sp2->me.x - sp1->me.x ) * odir1->x +
            ( sp2->me.y - sp1->me.y ) * odir1->y;
    /* if this is a real stub or intersection, then vectors on both sides
    /* of out going-to-be stem should point in the same direction. So
    /* the following value should be positive */
    opp = dir1->x * dir2->x + dir1->y * dir2->y;
    if ( opp > 0 && norm1 < 0 && norm2 < 0 && UnitsParallel( odir1,odir2,true ) && 
        ( UnitsOrthogonal( dir1,odir1,false ) || UnitsOrthogonal( dir2,odir1,false )))
return( 2 );
    if (opp > 0 && (( norm1 < 0 && pd1->colinear &&
        IsVectorHV( dir1,0,true ) && UnitsOrthogonal( dir1,odir2,false )) ||
        ( norm2 < 0 && pd2->colinear &&
        IsVectorHV( dir2,0,true ) && UnitsOrthogonal( dir2,odir1,false ))))
return( 3 );
    
    /* Now check if our 2 points form a serif termination or a feature stub
    /* The check is pretty dumb: it returns 'true' if all the following 
    /* conditions are met: 
    /* - both the points belong to the same contour;
    /* - there are no more than 3 other points between them;
    /* - anyone of those intermediate points is positioned by such a way
    /*   that it falls inside the stem formed by our 2 base point and
    /*   the vector we are checking and its distance from the first point
    /*   along that vector is not larger than the stem width;
    /* - none of the intermediate points is parallel to the vector direction
    /*   (otherwise we should have checked against that point instead)*/
    dist = ( sp1->me.x - sp2->me.x ) * dir1->y -
           ( sp1->me.y - sp2->me.y ) * dir1->x;
    nsp = sp1;

    for ( i=0; i<4; i++ ) {
        if (( is_next1 && nsp->prev == NULL ) || ( !is_next1 && nsp->next == NULL ))
return( 0 );

        nsp = ( is_next1 ) ? nsp->prev->from : nsp->next->to; 
        if ( ( i>0 && nsp == sp1 ) || nsp == sp2 )
    break;

        npd = &gd->points[nsp->ttfindex];
        if (UnitsParallel( &npd->nextunit,dir1,false ) || 
            UnitsParallel( &npd->prevunit,dir1,false ))
    break;

        ext = ( sp1->me.x - nsp->me.x ) * dir1->x +
              ( sp1->me.y - nsp->me.y ) * dir1->y;
        if ( ext < 0 ) ext = -ext;
        if (( dist > 0 && ext > dist ) || ( dist < 0 && ext < dist ))
    break;

        off = ( sp1->me.x - nsp->me.x ) * dir1->y -
              ( sp1->me.y - nsp->me.y ) * dir1->x;
        if (( dist > 0 && ( off <= 0 || off >= dist )) ||
            ( dist < 0 && ( off >= 0 || off <= dist )))
            exc++;
    }

    if ( nsp == sp2 && exc == 0 )
return( 1 );

return( 0 );
}

static struct stem_chunk *AddToStem( struct stemdata *stem,struct pointdata *pd1,struct pointdata *pd2,
    int is_next1, int is_next2, int cheat ) {
    
    int is_potential1=false, is_potential2=true;
    struct stem_chunk *chunk=NULL;
    BasePoint *dir = &stem->unit;
    BasePoint *test = &pd1->sp->me;
    double off, dist_error;
    double loff=0, roff=0;
    int j;

    if ( cheat ) is_potential2 = false;
    /* Diagonals are harder to align */
    dist_error = IsVectorHV( dir,0,true ) ? 2*dist_error_hv : 2*dist_error_diag;

    /* The following swaps "left" and "right" points in case we have
    /* started checking relatively to a wrong edge */
    off = (test->x-stem->left.x)*dir->y - (test->y-stem->left.y)*dir->x;
    if ( off < ( stem->lmax - dist_error ) || off > ( stem->lmin + dist_error ) ) {
	struct pointdata *pd = pd1; pd1 = pd2; pd2 = pd;
	int in = is_next1; is_next1 = is_next2; is_next2 = in;
	int ip = is_potential1; is_potential1 = is_potential2; is_potential2 = ip;
    }

    /* Now run through existing stem chunks and see if the chunk we are
    /* going to add doesn't duplicate an existing one.*/
    for ( j=stem->chunk_cnt-1; j>=0; --j ) {
	chunk = &stem->chunks[j];
        if ( chunk->l==pd1 && chunk->r==pd2 ) {
            if ( !is_potential1 ) chunk->lpotential = false;
            if ( !is_potential2 ) chunk->rpotential = false;
    break;
        }
    }

    if ( j<0 ) {
        stem->chunks = grealloc(stem->chunks,(stem->chunk_cnt+1)*sizeof(struct stem_chunk));
        chunk = &stem->chunks[stem->chunk_cnt++];
        chunk->parent = stem;

        chunk->l = pd1; chunk->lpotential = is_potential1;
        chunk->r = pd2; chunk->rpotential = is_potential2;

        chunk->lnext = is_next1;
        chunk->rnext = is_next2;
        chunk->ltick = false;
        chunk->rtick = false;
        chunk->stemcheat = cheat;
        chunk->stub = false;
        /* Diagonal edge stems should be assigned to the 'bothstem'
        /* property, as otherwise they may break normal diagonal stems
        /* handling. On the other hand, 'lnext' and 'rnext' chunk
        /* properties are important for this stem type. So we reset
        /* is_next1 and is_next2 to 2 as soon as those properties have been
        /* set */
        if ( cheat == 1 ) {
            is_next1 = 2; is_next2 =2;
        }
        
        if ( pd1 != NULL )
            loff = ( pd1->sp->me.x - stem->left.x ) * stem->unit.y -
                   ( pd1->sp->me.y - stem->left.y ) * stem->unit.x;
        if ( pd2 != NULL )
            roff = ( pd2->sp->me.x - stem->right.x ) * stem->unit.y -
                   ( pd2->sp->me.y - stem->right.y ) * stem->unit.x;
        
        if ( loff < stem->lmin ) stem->lmin = loff;
        else if ( loff > stem->lmax ) stem->lmax = loff;
        if ( roff < stem->rmin ) stem->rmin = roff;
        else if ( roff > stem->rmax ) stem->rmax = roff;
    }

    if ( pd1!=NULL ) {
	if ( is_next1==1 ) {
	    if ( pd1->nextstem == NULL || !is_potential1 ) {
		pd1->nextstem = stem;
		pd1->next_is_l = true;
	    }
	} else if ( is_next1==0 ) {
	    if ( pd1->prevstem == NULL || !is_potential1 ) {
		pd1->prevstem = stem;
		pd1->prev_is_l = true;
	    }
	} else if ( is_next1==2 ) {
	    if ( pd1->bothstem == NULL || !is_potential1 ) {
		pd1->bothstem = stem;
	    }
	}
    }
    if ( pd2!=NULL ) {
	if ( is_next2==1 ) {
	    if ( pd2->nextstem == NULL || !is_potential2 ) {
		pd2->nextstem = stem;
		pd2->next_is_l = false;		/* It's r */
	    }
	} else if ( is_next2==0 ) {
	    if ( pd2->prevstem == NULL || !is_potential2 ) {
		pd2->prevstem = stem;
		pd2->prev_is_l = false;
	    }
	} else if ( is_next2==2 ) {
	    if ( pd2->bothstem == NULL || !is_potential2 ) {
		pd2->bothstem = stem;
	    }
	}
    }
return( chunk );
}

static int StemFitsHV( struct stemdata *stem,int hv ) {
    int i,cnt,is_x;
    double loff,roff;
    double lmin=0,lmax=0,rmin=0,rmax=0;
    
    cnt = stem->chunk_cnt;
    
    if ( !stem->chunks[0].stub && !stem->chunks[cnt-1].stub )
return( false );    
    if ( stem->chunk_cnt == 1 )
return( true );
    
    is_x = ( hv == 1 ) ? 1 : 0;
    for ( i=0;i<cnt;i++ ) {
        struct stem_chunk *chunk = &stem->chunks[i];
        
        if ( chunk->l != NULL ) {
            loff = ( chunk->l->sp->me.x - stem->left.x ) * !is_x -
                   ( chunk->l->sp->me.y - stem->left.y ) * is_x;
            if ( loff < lmin ) lmin = loff;
            else if ( loff > lmax ) lmax = loff;
        }
        if ( chunk->r != NULL ) {
            roff = ( chunk->r->sp->me.x - stem->right.x ) * !is_x -
                   ( chunk->r->sp->me.y - stem->right.y ) * is_x;
            if ( roff < rmin ) rmin = roff;
            else if ( roff > rmax ) rmax = roff;
        }
    }
    if ((( lmax - lmin ) < 2*dist_error_hv ) && (( rmax - rmin ) < 2*dist_error_hv ))
return( true );
return( false );
}

static int OnStem( struct stemdata *stem,BasePoint *test,int left ) {
    double dist_error, off;
    BasePoint *dir = &stem->unit;
    double max, min;

    dist_error = IsVectorHV( dir,0,true ) ? 2*dist_error_hv : 2*dist_error_diag;	/* Diagonals are harder to align */
    if ( left ) {
        off = (test->x-stem->left.x)*dir->y - (test->y-stem->left.y)*dir->x;
        max = stem->lmax; min = stem->lmin;
    } else {
        off = (test->x-stem->right.x)*dir->y - (test->y-stem->right.y)*dir->x;
        max = stem->rmax; min = stem->rmin;
    }
    
    if ( off > ( max - dist_error ) && off < ( min + dist_error ) )
return( true );

return( false );
}

static int BothOnStem( struct stemdata *stem,BasePoint *test1,BasePoint *test2,int force_hv ) {
    double dist_error, off1, off2;
    BasePoint dir = stem->unit;
    int hv, hv_strict;
    double lmax, lmin, rmax, rmin;
    
    hv = ( force_hv ) ? IsVectorHV( &dir,slope_error,false ) : IsVectorHV( &dir,0,true );
    hv_strict = ( force_hv ) ? IsVectorHV( &dir,0,true ) : hv;
    if ( force_hv ) {
        if ( force_hv != hv )
return( false );
        if ( !hv_strict && !StemFitsHV( stem,hv ))
return( false );
        if ( !hv_strict ) {
            dir.x = ( force_hv == 2 ) ? 0 : 1;
            dir.y = ( force_hv == 2 ) ? 1 : 0;
        }
    }
    /* Diagonals are harder to align */
    dist_error = ( hv ) ? 2*dist_error_hv : 2*dist_error_diag;
    lmax = stem->lmax; lmin = stem->lmin;
    rmax = stem->rmax; rmin = stem->rmin;

    off1 = (test1->x-stem->left.x)*dir.y - (test1->y-stem->left.y)*dir.x;
    off2 = (test2->x-stem->right.x)*dir.y - (test2->y-stem->right.y)*dir.x;
    if (off1 > ( lmax - dist_error ) && off1 < ( lmin + dist_error ) &&
        off2 > ( rmax - dist_error ) && off2 < ( rmin + dist_error ))
return( true );

    off2 = (test2->x-stem->left.x)*dir.y - (test2->y-stem->left.y)*dir.x;
    off1 = (test1->x-stem->right.x)*dir.y - (test1->y-stem->right.y)*dir.x;
    if (off2 > ( lmax - dist_error ) && off2 < ( lmin + dist_error ) &&
        off1 > ( rmax - dist_error ) && off1 < ( rmin + dist_error ))
return( true );

return( false );
}

static struct stemdata *FindStem( struct glyphdata *gd,struct pointdata *pd,
	BasePoint *dir,struct pointdata *pd2, int is_next2, int is_stub ) {
    int j, test_left, hv;
    struct stemdata *stem;
    SplinePoint *match;
    double err;

    match = pd2->sp;
    test_left = is_next2 ? !pd2->next_is_l : !pd2->prev_is_l;
    stem = is_next2 ? pd2->nextstem : pd2->prevstem;
    if ( stem!=NULL && OnStem( stem,&pd->sp->me,test_left ))
return( stem );

    for ( j=0; j<gd->stemcnt; ++j ) {
        stem = &gd->stems[j];
	err = stem->unit.x*dir->y - stem->unit.y*dir->x;
	if ( err<slope_error && err>-slope_error &&
            BothOnStem( stem,&pd->sp->me,&pd2->sp->me,false )) {
return( stem );
        }
        hv = IsVectorHV( dir,slope_error,false );
        if ( hv && BothOnStem( stem,&pd->sp->me,&pd2->sp->me,hv )) {
            stem->unit.x = ( hv == 2 ) ? 0 : 1;
            stem->unit.y = ( hv == 2 ) ? 1 : 0;
            stem->l_to_r.x = ( hv == 2 ) ? 1 : 0;
            stem->l_to_r.y = ( hv == 2 ) ? 0 : -1;
return( stem );
	}
    }
return( NULL );
}

static void SetStemUnit( struct stemdata *stem,BasePoint dir ) {
    double width;
    
    width = ( stem->right.x - stem->left.x ) * dir.y -
            ( stem->right.y - stem->left.y ) * dir.x;
    if ( width < 0 ) {
        width = -width;
        dir.x = -dir.x;
        dir.y = -dir.y;
    }
    stem->unit = dir;
    stem->width = width;
    
    /* Guess at which normal we want */
    stem->l_to_r.x = dir.y; stem->l_to_r.y = -dir.x;
    /* If we guessed wrong, use the other */
    if (( stem->right.x-stem->left.x )*stem->l_to_r.x +
	( stem->right.y-stem->left.y )*stem->l_to_r.y < 0 ) {
	stem->l_to_r.x = -stem->l_to_r.x;
	stem->l_to_r.y = -stem->l_to_r.y;
    }
}

static struct stemdata *NewStem(struct glyphdata *gd,BasePoint *dir,
	BasePoint *pos1, BasePoint *pos2) {
    struct stemdata * stem = &gd->stems[gd->stemcnt++];
    double width;

    stem->unit = *dir;
    if ( dir->x+dir->y<0 ) {
	stem->unit.x = -stem->unit.x;
	stem->unit.y = -stem->unit.y;
    }
    width = ( pos2->x - pos1->x ) * stem->unit.y -
            ( pos2->y - pos1->y ) * stem->unit.x;
    if ( width > 0 ) {
	stem->left = *pos1;
	stem->right = *pos2;
        stem->width = width;
    } else {
	stem->left = *pos2;
	stem->right = *pos1;
        stem->width = -width;
    }
    /* Guess at which normal we want */
    stem->l_to_r.x = dir->y; stem->l_to_r.y = -dir->x;
    /* If we guessed wrong, use the other */
    if (( stem->right.x-stem->left.x )*stem->l_to_r.x +
	( stem->right.y-stem->left.y )*stem->l_to_r.y < 0 ) {
	stem->l_to_r.x = -stem->l_to_r.x;
	stem->l_to_r.y = -stem->l_to_r.y;
    }
    stem->leftline = stem->rightline = NULL;
    stem->lmin = stem->lmax = 0;
    stem->rmin = stem->rmax = 0;
    stem->chunks = NULL;
    stem->chunk_cnt = 0;
    stem->ghost = false;
    stem->blue = -1;
return( stem );
}

static int ParallelToDir( struct pointdata *pd,int checknext,BasePoint *dir,
	BasePoint *opposite,SplinePoint *basesp,int is_stub ) {
    BasePoint n, o, *base = &basesp->me;
    SplinePoint *sp;
    double err;

    sp = pd->sp;
    n = ( checknext ) ? pd->nextunit : pd->prevunit;

    err = n.x*dir->y - n.y*dir->x;
    if (( !is_stub && ( err > slope_error || err < -slope_error )) ||
        ( is_stub && ( err > 3*slope_error || err < -3*slope_error )))
return( false );

    /* Now sp must be on the same side of the spline as opposite */
    o.x = opposite->x-base->x; o.y = opposite->y-base->y;
    n.x = sp->me.x-base->x; n.y = sp->me.y-base->y;
    if ( ( o.x*dir->y - o.y*dir->x )*( n.x*dir->y - n.y*dir->x ) < 0 )
return( false );
    
return( true );
}

static int LineParallel( BasePoint *dir,SplinePoint *fp,SplinePoint *tp ) {
    BasePoint odir;
    double olen;

    odir.x = fp->me.x - tp->me.x;
    odir.y = fp->me.y - tp->me.y;
    olen = sqrt( pow( odir.x,2 ) + pow( odir.y,2 ));
    if ( olen==0 )
return( false );
    odir.x /= olen; odir.y /= olen;
return( UnitsParallel( dir,&odir,true ));
}

static int NearlyParallel(BasePoint *dir,Spline *other, double t) {
    BasePoint odir;
    double olen;

    odir.x = (3*other->splines[0].a*t+2*other->splines[0].b)*t+other->splines[0].c;
    odir.y = (3*other->splines[1].a*t+2*other->splines[1].b)*t+other->splines[1].c;
    olen = sqrt( pow( odir.x,2 ) + pow( odir.y,2 ));
    if ( olen==0 )
return( false );
    odir.x /= olen; odir.y /= olen;
return( UnitsParallel( dir,&odir,false ));
}

static double NormalDist(BasePoint *to, BasePoint *from, BasePoint *perp) {
    double len = (to->x-from->x)*perp->y - (to->y-from->y)*perp->x;
    if ( len<0 ) len = -len;
return( len );
}

static struct stemdata *TestStem( struct glyphdata *gd,struct pointdata *pd,
    BasePoint *dir,SplinePoint *match,int is_next,int is_next2,int require_existing,int is_stub ) {
    struct pointdata *pd2;
    struct stemdata *stem;
    struct stem_chunk *chunk;
    double width, err;
    struct linedata *line, *line2;
    BasePoint *mdir;
    
    width = (match->me.x-pd->sp->me.x)*dir->y - (match->me.y-pd->sp->me.y)*dir->x;
    if ( width<0 ) width = -width;
    if ( width<.5 )
return( NULL );		/* Zero width stems aren't interesting */
    if ( (is_next && pd->sp->next->to==match) || (!is_next && pd->sp->prev->from==match) )
return( NULL );		/* Don't want a stem between two splines that intersect */

    if ( match->ttfindex==0xffff )
return( NULL );         /* Not a real point */
    pd2 = &gd->points[match->ttfindex];

    line = is_next ? pd->nextline : pd->prevline;
    mdir = is_next2 ? &pd2->nextunit : &pd2->prevunit;
    line2 = is_next2 ? pd2->nextline : pd2->prevline;
    if ( !IsVectorHV( mdir,0,true ) && line2 != NULL )
        mdir = &line2->unitvector;
    if ( mdir->x==0 && mdir->y==0 )
return( NULL );         /* cannot determine the opposite point's direction */

    err = mdir->x*dir->y - mdir->y*dir->x;
    if (( err>slope_error || err<-slope_error ) && !is_stub )
return( NULL );         /* Cannot make a stem if edges are not parallel (unless it is a serif) */
    
    /* For serifs we prefer the vector which is closer to horizontal/vertical */
    if ( is_stub && VectorCloserToHV( mdir,dir ) == 1 )
        dir = mdir;

    stem = FindStem( gd,pd,dir,pd2,is_next2,is_stub );
    if ( stem == NULL && require_existing )
return( NULL );

    if ( stem == NULL )
	stem = NewStem( gd,dir,&pd->sp->me,&match->me );

    chunk = AddToStem( stem,pd,pd2,is_next,is_next2,false );
    if ( chunk != NULL ) {
        chunk->stub = is_stub;
    
        if ( line != NULL && chunk->l == pd && stem->leftline == NULL ) {
            stem->leftline = line;
            SetStemUnit( stem,line->unitvector );
        } else if ( line != NULL && chunk->r == pd && stem->rightline == NULL )
            stem->rightline = line;
        if ( line2 != NULL && chunk->l == pd2 && stem->leftline == NULL ) {
            stem->leftline = line2;
            SetStemUnit( stem,line2->unitvector );
        } else if ( line2 != NULL && chunk->r == pd2 && stem->rightline == NULL )
            stem->rightline = line2;
    }
return( stem );
}

static double FindSameSlope(Spline *s,BasePoint *dir,double close_to) {
    double a, b, c, desc;
    double t1, t2;
    double d1, d2;

    if ( s==NULL )
return( -1e4 );

    a = dir->x*s->splines[1].a*3 - dir->y*s->splines[0].a*3;
    b = dir->x*s->splines[1].b*2 - dir->y*s->splines[0].b*2;
    c = dir->x*s->splines[1].c   - dir->y*s->splines[0].c  ;
    if ( a!=0 ) {
	desc = b*b - 4*a*c;
	if ( desc<0 )
return( -1e4 );
	desc = sqrt(desc);
	t1 = (-b+desc)/(2*a);
	t2 = (-b-desc)/(2*a);
	if ( (d1=t1-close_to)<0 ) d1 = -d1;
	if ( (d2=t2-close_to)<0 ) d2 = -d2;
	if ( d2<d1 && t2>=-.001 && t2<=1.001 )
	    t1 = t2;
    } else if ( b!=0 )
	t1 = -c/b;
    else
return( -1e4 );

return( t1 );
}

static struct stemdata *HalfStem(struct glyphdata *gd,struct pointdata *pd,
	BasePoint *dir,Spline *other,double other_t,int is_next) {
    /* Find the spot on other where the slope is the same as dir */
    double t1;
    double width, err;
    BasePoint match;
    struct stemdata *stem = NULL;
    struct pointdata *pd2 = NULL;
    int i;

    t1 = FindSameSlope(other,dir,other_t);
    if ( t1==-1e4 )
return( NULL );
    if ( t1<0 && other->from->prev!=NULL && gd->points[other->from->ttfindex].colinear ) {
	other = other->from->prev;
	t1 = FindSameSlope(other,dir,1.0);
    } else if ( t1>1 && other->to->next!=NULL && gd->points[other->to->ttfindex].colinear ) {
	other = other->to->next;
	t1 = FindSameSlope(other,dir,0.0);
    }

    if ( t1<-.001 || t1>1.001 )
return( NULL );

    /* Ok. the opposite edge has the right slope at t1 */
    /* Now see if we can make a one sided stem out of these two */
    match.x = ((other->splines[0].a*t1+other->splines[0].b)*t1+other->splines[0].c)*t1+other->splines[0].d;
    match.y = ((other->splines[1].a*t1+other->splines[1].b)*t1+other->splines[1].c)*t1+other->splines[1].d;

    width = (match.x-pd->sp->me.x)*dir->y - (match.y-pd->sp->me.y)*dir->x;
    /* offset = (match.x-pd->sp->me.x)*dir->x + (match.y-pd->sp->me.y)*dir->y;*/
    if ( width<.5 && width>-.5 )
return( NULL );		/* Zero width stems aren't interesting */

    if ( isnan(t1))
	IError( "NaN value in HalfStem" );

    if ( is_next ) {
	pd->nextedge = other;
	pd->next_e_t = t1;
    } else {
	pd->prevedge = other;
	pd->prev_e_t = t1;
    }

    /* In my experience the only case where this function may be useful
    /* is when it occasionally finds a real spline point which for some
    /* reasons has been neglected by other tests and yet forms a valid
    /* pair for the first point. So run through points and see if we
    /* have actually got just a position on spline midway between to points,
    /* or it is a normal point allowing to make a normal stem chunk */
    for ( i=0; i<gd->pcnt; ++i ) {
        struct pointdata *tpd = &gd->points[i];
        if ( tpd->sp != NULL && tpd->sp->me.x == match.x && tpd->sp->me.y == match.y ) {
            pd2 = tpd;
    break;
        }
    }
    for ( i=0; i<gd->stemcnt; ++i ) {
        struct stemdata *tstem = &gd->stems[i];
        err = tstem->unit.x*dir->y - tstem->unit.y*dir->x;
        if (err<slope_error && err>-slope_error && BothOnStem( tstem,&pd->sp->me,&match,false )) {
	    stem = tstem;
    break;
	}
    }
    if ( stem==NULL )
	stem = NewStem(gd,dir,&pd->sp->me,&match);
    AddToStem( stem,pd,pd2,is_next,false,false );
return( stem );
}

static struct stemdata *FindOrMakeHVStem( struct glyphdata *gd,
	struct pointdata *pd, struct pointdata *pd2, int is_h ) {
    int i;
    struct stemdata *stem;
    BasePoint dir;

    dir.x = ( is_h ) ? 1 : 0;
    dir.y = ( is_h ) ? 0 : 1;
    
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( IsVectorHV( &stem->unit,slope_error,true ) &&
            ((pd2 == NULL && 
                ( OnStem( stem,&pd->sp->me,true ) || OnStem( stem,&pd->sp->me,false ))) ||
            ( pd2 != NULL && BothOnStem( stem,&pd->sp->me,&pd2->sp->me,false ))))
    break;
    }
    if ( i==gd->stemcnt ) stem=NULL;

    if ( stem == NULL && pd2 != NULL )
	stem = NewStem( gd,&dir,&pd->sp->me,&pd2->sp->me );
return( stem );
}

static struct stemdata *EdgeStem(struct glyphdata *gd,struct pointdata *pd) {
    struct pointdata *pd2;
    /* suppose we have something like */
    /*  *--*		*/
    /*   \  \		*/
    /*    \  \		*/
    /* Then let's create a vertical stem between the two points */
    /* (and a horizontal stem if the thing is rotated 90 degrees) */
    struct stemdata *stem = NULL;
    struct stem_chunk *chunk;
    double width, length1, length2;

    if ( !hint_diagonal_ends )
return( NULL );

    if ( pd->sp->next==NULL || pd->sp->next->to->next==NULL )
return( NULL );
    pd2 = &gd->points[pd->sp->next->to->ttfindex];
    if ( (pd->sp->me.x!=pd2->sp->me.x && pd->sp->me.y!=pd2->sp->me.y ) ||
	    pd->sp->prev==NULL || !pd->sp->prev->knownlinear ||
	    pd2->sp->next==NULL || !pd2->sp->next->knownlinear )
return( NULL );
    if ( !UnitsParallel(&pd2->nextunit,&pd->prevunit,true ) )
return( NULL );
    if ( pd->prevunit.x==0 || pd->prevunit.y==0 )	/* Must be diagonal */
return( NULL );

    length1 = (pd->sp->prev->from->me.x-pd->sp->me.x)*(pd->sp->prev->from->me.x-pd->sp->me.x) +
	    (pd->sp->prev->from->me.y-pd->sp->me.y)*(pd->sp->prev->from->me.y-pd->sp->me.y);
    length2 = (pd2->sp->next->to->me.x-pd2->sp->me.x)*(pd2->sp->next->to->me.x-pd2->sp->me.x) +
	    (pd2->sp->next->to->me.y-pd2->sp->me.y)*(pd2->sp->next->to->me.y-pd2->sp->me.y);
    if ( length2<length1 )
	length1 = length2;

    if ( pd->sp->me.x==pd2->sp->me.x )
	width = pd->sp->me.y-pd2->sp->me.y;
    else
	width = pd->sp->me.x-pd2->sp->me.x;

    if ( width*width>length1 )
return( NULL );

    stem = FindOrMakeHVStem( gd,pd,pd2,pd->sp->me.x==pd2->sp->me.x );
    chunk = AddToStem( stem,pd,pd2,1,0,1 );
return( stem );
}

static int ConnectsAcross(struct glyphdata *gd,SplinePoint *sp,int is_next,Spline *findme) {
    struct pointdata *pd = &gd->points[sp->ttfindex];
    Spline *other, *test;
    BasePoint *dir;

    other = ( is_next ) ? pd->nextedge : pd->prevedge;

    if ( other==findme )
return( true );
    if ( other==NULL )
return( false );

    dir = &gd->points[other->to->ttfindex].nextunit;
    test = other->to->next;
    while ( test!=NULL &&
	    gd->points[test->from->ttfindex].nextunit.x * dir->x +
	    gd->points[test->from->ttfindex].nextunit.y * dir->y > 0 ) {
	if ( test==findme )
return( true );
	test = test->to->next;
	if ( test==other )
    break;
    }
	    
    dir = &gd->points[other->from->ttfindex].prevunit;
    test = other->from->prev;
    while ( test!=NULL &&
	    gd->points[test->to->ttfindex].prevunit.x * dir->x +
	    gd->points[test->to->ttfindex].prevunit.y * dir->y > 0 ) {
	if ( test==findme )
return( true );
	test = test->from->prev;
	if ( test==other )
    break;
    }
return( false );
}

static double RecalcT( Spline *base,SplinePoint *from, SplinePoint *to, double curt ) {
    double baselen, fromlen, tolen, ret;
    Spline *cur;
    
    baselen = SplineLength( base );
    fromlen = baselen * curt;
    tolen = baselen * ( 1 - curt );
    
    cur = base->from->prev;
    while ( cur != NULL && cur->to != from ) {
        fromlen += SplineLength( cur );
        cur = cur->from->prev;
    }
    cur = base->to->next;
    while ( cur!= NULL && cur->from != to ) {
        tolen += SplineLength( cur );
        cur = cur->to->next;
    }
    ret = fromlen/( fromlen + tolen );
return( ret );
}

static void BuildStem( struct glyphdata *gd,struct pointdata *pd,int is_next,
    int test_further ) {
    BasePoint *dir;
    Spline *other, *cur;
    double t;
    double tod, fromd, dist;
    SplinePoint *testpt, *topt, *frompt;
    struct stemdata *stem;
    struct linedata *line;
    struct pointdata *testpd, *topd, *frompd;
    int tp, fp, tstub, fstub, t_needs_recalc=false;
    BasePoint opposite;

    if ( is_next ) {
	dir = &pd->nextunit;
	other = pd->nextedge;
	cur = pd->sp->next;
	t = pd->next_e_t;
        dist = pd->next_dist;
    } else {
	dir = &pd->prevunit;
	other = pd->prevedge;
	cur = pd->sp->prev;
	t = pd->prev_e_t;
        dist = pd->prev_dist;
    }
    topt = other->to; frompt = other->from;
    topd = &gd->points[topt->ttfindex];
    frompd = &gd->points[frompt->ttfindex];
    if ( test_further && frompd->nextstem != NULL && topd->prevstem != NULL )
return;
    
    line = is_next ? pd->nextline : pd->prevline;
    if ( !IsVectorHV( dir,slope_error,true ) && line != NULL)
        dir = &line->unitvector;

    if ( other==NULL )
return;

    opposite.x = ((other->splines[0].a*t+other->splines[0].b)*t+other->splines[0].c)*t+other->splines[0].d;
    opposite.y = ((other->splines[1].a*t+other->splines[1].b)*t+other->splines[1].c)*t+other->splines[1].d;

    tstub = IsStubOrIntersection( gd,dir,pd,topd,is_next,false );
    fstub = IsStubOrIntersection( gd,dir,pd,frompd,is_next,true );
    tp = ParallelToDir( topd,false,dir,&opposite,pd->sp,tstub );
    fp = ParallelToDir( frompd,true,dir,&opposite,pd->sp,fstub );
    
    /* if none of the opposite points is parallel to the needed vector, then
    /* give it one more chance by skipping those points and looking at the next
    /* and previous one. This can be useful in situations where the opposite
    /* edge cannot be correctly detected just because there are too many points
    /* on the spline (which is a very common situation for poorly designed
    /* fonts or fonts with quadratic splines).
    /* But do that only for colinear spline segments and ensure that there are
    /* no bends between two splines. */
    if ( !tp && topd->colinear && &other->to->next != NULL ) {
        testpt = other->to->next->to; 
        testpd = &gd->points[testpt->ttfindex];
        if (pd->sp != testpt &&
            testpd->prevunit.x * topd->prevunit.x +
            testpd->prevunit.y * topd->prevunit.y > 0 ) {

            topt = testpt; topd = testpd;
            t_needs_recalc = true;
            tp = ParallelToDir( topd,false,dir,&opposite,pd->sp,false );
        }
    }
    if ( !fp && frompd->colinear && &other->from->prev != NULL ) {
        testpt = other->from->prev->from; 
        testpd = &gd->points[testpt->ttfindex];
        if (pd->sp != testpt &&
            testpd->nextunit.x * frompd->nextunit.x +
            testpd->nextunit.y * frompd->nextunit.y > 0 ) {

            frompt = testpt; frompd = testpd;
            t_needs_recalc = true;
            fp = ParallelToDir( frompd,true,dir,&opposite,pd->sp,false );
        }
    }
    if ( t_needs_recalc )
        t = RecalcT( other,frompt,topt,t );
    stem = NULL;
    if ( is_next && !test_further ) 
	stem = EdgeStem( gd,pd );
    if ( !tp && !fp )
return;

    /* We have several conflicting metrics for getting the "better" stem */
    /* Generally we prefer the stem with the smaller width (but not always. See tilde) */
    /* Generally we prefer the stem formed by the point closer to the intersection */
    tod = (1-t)*NormalDist( &topt->me,&pd->sp->me,dir );
    fromd = t*NormalDist( &frompt->me,&pd->sp->me,dir );
    stem = NULL;
    
    if ( !test_further ) {
        if ( tp && (( tod<fromd ) ||
            ( !fp && ( tod<2*fromd || dist < topd->prev_dist || 
                ConnectsAcross( gd,frompt,true,cur ) || NearlyParallel( dir,other,t ))))) {
	    stem = TestStem( gd,pd,dir,topt,is_next,false,false,tstub );
        }
        if ( stem==NULL && fp && (( fromd<tod ) ||
            ( !tp && ( fromd<2*tod || dist < frompd->next_dist || 
                ConnectsAcross( gd,topt,false,cur ) || NearlyParallel( dir,other,t ))))) {
	    stem = TestStem( gd,pd,dir,frompt,is_next,true,false,fstub );
        }
        if ( stem==NULL && cur!=NULL && !other->knownlinear && !cur->knownlinear )
	    stem = HalfStem(gd,pd,dir,other,t,is_next);
    } else {
        /* Usually we are attempting to establish a connection between the closest
        /* points on the opposite stem edges. But things may be more complicated
        /* for straight lines (especially diagonal lines). Imagine a narrow 
        /* paralellogram where the points on the obtuse angles are closer to each
        /* other than to those on the sharp angles. Thus with the above algorithm
        /* those further points would never be assigned to the stem (which means the
        /* stem itself may not be correctly detected). This means we actually have 
        /* to check both sides of a linear segment parallel to the vector tested.
        /* But we are cautious and accept such segments only if there is already
        /* a stem they can be assigned to. */
        if ( tp && fp && ( tod<fromd ) && LineParallel( dir,frompt,topt )
            && frompd->nextstem==NULL )
            TestStem( gd,frompd,dir,pd->sp,true,is_next,true,tstub );
        else if ( tp && fp && ( fromd<tod ) && LineParallel( dir,frompt,topt ) 
            && topd->prevstem==NULL )
            TestStem( gd,topd,dir,pd->sp,false,is_next,true,fstub );
    }
}

static void DiagonalCornerStem(struct glyphdata *gd,struct pointdata *pd) {
    Spline *other = pd->bothedge;
    struct pointdata *pfrom = &gd->points[other->from->ttfindex],
		     *pto = &gd->points[other->to->ttfindex],
		     *pd2 = NULL, *pd3=NULL;
    double width, length;
    struct stemdata *stem;
    struct stem_chunk *chunk;

    if ( pd->symetrical_h && pto->symetrical_h && pd->both_e_t>.9 )
	pd2 = pto;
    else if ( pd->symetrical_h && pfrom->symetrical_h && pd->both_e_t<.1 )
	pd2 = pfrom;
    else if ( pd->symetrical_v && pto->symetrical_v && pd->both_e_t>.9 )
	pd2 = pto;
    else if ( pd->symetrical_v && pfrom->symetrical_v && pd->both_e_t<.1 )
	pd2 = pfrom;
    else if ( pd->symetrical_h && other->islinear && other->splines[1].c==0 ) {
	pd2 = pfrom;
	pd3 = pto;
    } else if ( pd->symetrical_v && other->islinear && other->splines[0].c==0 ) {
	pd2 = pfrom;
	pd3 = pto;
    } else
return;

    if ( pd->symetrical_v )
	width = (pd->sp->me.x-pd2->sp->me.x);
    else
	width = (pd->sp->me.y-pd2->sp->me.y);
    length = (pd->sp->next->to->me.x-pd->sp->me.x)*(pd->sp->next->to->me.x-pd->sp->me.x) +
	     (pd->sp->next->to->me.y-pd->sp->me.y)*(pd->sp->next->to->me.y-pd->sp->me.y);
    if ( width*width>length )
return;

    stem = FindOrMakeHVStem(gd,pd,pd2,pd->symetrical_h);
    
    if ( pd3 == NULL && stem != NULL )
        chunk = AddToStem( stem,pd,pd2,2,2,2 );
    else {
	chunk = AddToStem( stem,pd,pd2,2,2,3 );
	chunk = AddToStem( stem,pd,pd3,2,2,3 );
    }
}

static int chunk_cmp( const void *_p1, const void *_p2 ) {
    const struct stem_chunk *ch1 = _p1, *ch2 = _p2;
    
    struct stemdata *stem;
    double loff1=0,roff1=0,loff2=0,roff2=0;
    
    stem = ch1->parent;
    if ( stem==NULL )
return( 0 );

    if ( ch1->l != NULL )
        loff1 = ( ch1->l->sp->me.x - stem->left.x ) * stem->unit.x +
            ( ch1->l->sp->me.y - stem->left.y ) * stem->unit.y;
    if ( ch1->r != NULL )
        roff1 = ( ch1->r->sp->me.x - stem->right.x ) * stem->unit.x +
            ( ch1->r->sp->me.y - stem->right.y ) * stem->unit.y;
    if ( ch2->l != NULL )
        loff2 = ( ch2->l->sp->me.x - stem->left.x ) * stem->unit.x +
            ( ch2->l->sp->me.y - stem->left.y ) * stem->unit.y;
    if ( ch2->r != NULL )
        roff2 = ( ch2->r->sp->me.x - stem->right.x ) * stem->unit.x +
            ( ch2->r->sp->me.y - stem->right.y ) * stem->unit.y;
        
    if ( loff1>loff2 )
return( 1 );
    else if ( loff1<loff2 )
return( -1 );
    else {
        if ( roff1>roff2 )
return( 1 );
        else if ( roff1<roff2 )
return( -1 );
        else
return( 0 );
    }
}

static int IsL(BasePoint *bp,struct stemdata *stem) {
    double offl, offr;

    offl = (bp->x-stem->left.x)*stem->l_to_r.x + (bp->y-stem->left.y)*stem->l_to_r.y;
    if ( offl<0 ) offl = -offl;
    offr = (bp->x-stem->right.x)*stem->l_to_r.x + (bp->y-stem->right.y)*stem->l_to_r.y;
    if ( offr<0 ) offr = -offr;
return( offl < offr );
}

static void ReassignPotential( struct glyphdata *gd,
    struct stemdata *stem,struct stem_chunk *chunk,int is_l,int action ) {
    
    struct pointdata *pd=NULL;
    struct stemdata *stem2=NULL;
    int i, next;
    if ( !action )
return;
    
    if ( is_l ) {
        if ( chunk->l!=NULL && chunk->lpotential ) {
            pd = chunk->l;
            chunk->lpotential = false;
            next = chunk->lnext;
        }
    } else {
        if ( chunk->r!=NULL && chunk->rpotential ) {
            pd = chunk->r;
            chunk->rpotential = false;
            next = chunk->rnext;
        }
    }
    if ( pd == NULL || action == 2 )
return;
    stem2 = next ? pd->nextstem : pd->prevstem;
    
    if ( stem2!=NULL && stem2!=stem ) for ( i=0; i<stem2->chunk_cnt; i++ ) {
	struct stem_chunk *chunk2 = &stem2->chunks[i];
        
        if ( is_l && chunk2->l == pd ) {
            chunk2->lpotential = true;
        } else if ( !is_l && chunk2->r == pd ) {
            chunk2->rpotential = true;
        }
    }
    
    if ( pd->nextstem==stem2 ) {
	pd->nextstem = stem;
	pd->next_is_l = IsL( &pd->sp->me,stem );
    }
    if ( pd->prevstem==stem2 ) {
	pd->prevstem = stem;
	pd->prev_is_l = IsL( &pd->sp->me,stem );
    }
}

static void FixupT(struct stemdata *stem,struct pointdata *pd,int isnext) {
    /* When we calculated "next/prev_e_t" we deliberately did not use pd1->me */
    /*  (because things get hard at intersections) so our t is only an approx-*/
    /*  imation. We can do a lot better now */
    Spline *s;
    Spline myline;
    SplinePoint end1, end2;
    double width,t,sign, len, dot;
    BasePoint pts[9];
    extended lts[10], sts[10];
    BasePoint diff;

    width = (stem->right.x-stem->left.x)*stem->unit.y - (stem->right.y-stem->left.y)*stem->unit.x;
    s = isnext ? pd->nextedge : pd->prevedge;
    if ( s==NULL )
return;
    diff.x = s->to->me.x-s->from->me.x; diff.y = s->to->me.y-s->from->me.y;
    if ( diff.x<.001 && diff.x>-.001 && diff.y<.001 && diff.y>-.001 )
return;		/* Zero length splines give us NaNs */
    len = sqrt( pow( diff.x,2 ) + pow( diff.y,2 ));
    dot = (diff.x*stem->unit.x + diff.y*stem->unit.y)/len;
    if ( dot < .0004 && dot > -.0004 )
return;		/* It's orthogonal to our stem */

    if (( stem->unit.x==1 || stem->unit.x==-1 ) && s->knownlinear )
	t = (pd->sp->me.x-s->from->me.x)/(s->to->me.x-s->from->me.x);
    else if (( stem->unit.y==1 || stem->unit.y==-1 ) && s->knownlinear )
	t = (pd->sp->me.y-s->from->me.y)/(s->to->me.y-s->from->me.y);
    else {
	memset(&myline,0,sizeof(myline));
	memset(&end1,0,sizeof(end1));
	memset(&end2,0,sizeof(end2));
	sign = ( (isnext && pd->next_is_l) || (!isnext && pd->prev_is_l)) ? 1 : -1;
	myline.knownlinear = myline.islinear = true;
	end1.me = pd->sp->me;
	end2.me.x = pd->sp->me.x+1.1*sign*width*stem->l_to_r.x;
	end2.me.y = pd->sp->me.y+1.1*sign*width*stem->l_to_r.y;
	end1.nextcp = end1.prevcp = end1.me;
	end2.nextcp = end2.prevcp = end2.me;
	end1.nonextcp = end1.noprevcp = end2.nonextcp = end2.noprevcp = true;
	end1.next = &myline; end2.prev = &myline;
	myline.from = &end1; myline.to = &end2;
	myline.splines[0].d = end1.me.x;
	myline.splines[0].c = end2.me.x-end1.me.x;
	myline.splines[1].d = end1.me.y;
	myline.splines[1].c = end2.me.y-end1.me.y;
	if ( SplinesIntersect(&myline,s,pts,lts,sts)<=0 )
return;
	t = sts[0];
    }
    if ( isnan(t))
	IError( "NaN value in FixupT" );
    if ( isnext )
	pd->next_e_t = t;
    else
	pd->prev_e_t = t;
}

static int IsSplinePeak( struct glyphdata *gd,struct splinepoint *sp,int outer,int is_x ) {
    double base, next, prev, nextctl, prevctl;
    Spline *snext, *sprev;
    struct monotonic **space, *m;
    int winding=0, i, desired;
    
    base = ((real *) &sp->me.x)[!is_x];
    nextctl = sp->nonextcp ? base : ((real *) &sp->nextcp.x)[!is_x];
    prevctl = sp->noprevcp ? base : ((real *) &sp->prevcp.x)[!is_x];
    next = prev = base;
    snext = sp->next; sprev = sp->prev;
    
    if ( snext->to == NULL || sprev->from == NULL )
return( false );

    while ( snext->to->next != NULL && snext->to != sp && next == base ) {
        next = ((real *) &snext->to->me.x)[!is_x];
        snext = snext->to->next;
    }

    while ( sprev->from->prev != NULL && sprev->from != sp && prev == base ) {
        prev = ((real *) &sprev->from->me.x)[!is_x];
        sprev = sprev->from->prev;
    }
    
    if ( prev<base && next<base && nextctl<=base && prevctl<=base )
        desired = ( outer ) ? -1 : 1;
    else if ( prev>base && next>base && prevctl>=base && nextctl>=base )
        desired = ( outer ) ? 1 : -1;
    else
return( false );

    MonotonicFindAt( gd->ms,is_x,((real *) &sp->me.x)[is_x],space = gd->space );
    for ( i=0; space[i]!=NULL; ++i ) {
        m = space[i];
        Spline *s = m->s;
	winding = ((&m->xup)[is_x] ? 1 : -1 );

        if (( s->from == sp || s->to == sp ) && winding == desired )
return( winding );
    }

return( false );
}

static int CompareStems( struct glyphdata *gd,SplinePoint *sp,
    SplinePoint *sp1,SplinePoint *sp2,struct stemdata *stem1,struct stemdata *stem2,
    int is_next,int is_next1,int is_next2 ) {
    
    double dist1, dist2, fromproj, toproj;
    int peak=0, peak1=0, peak2=0, val1=0, val2=0;
    struct pointdata *frompd, *topd;
    Spline *sbase, *s1, *s2;
    
    dist1 = pow( sp->me.x-sp1->me.x,2 ) + pow( sp->me.y-sp1->me.y,2 );
    dist2 = pow( sp->me.x-sp2->me.x,2 ) + pow( sp->me.y-sp2->me.y,2 );
    
    sbase = ( is_next ) ? sp->next : sp->prev;
    s1 = ( is_next1 ) ? sp1->next : sp1->prev;
    s2 = ( is_next2 ) ? sp2->next : sp2->prev;
    
    /* If there are 2 conflicting chunks belonging to different stems but
    /* based on the same point, then we have to decide which stem is "better"
    /* for that point. We compare stems (or rather chunks) by assigning a 
    /* value to each of them and then prefer the stem whose value is positive. 
    /* A chunk gets a +1 value bonus in the following cases:
    /* - The stem is vertical/horizontal and splines are curved in the same
    /*   direction at both sides of the chunk;
    /* - A stem has both its width and the distance between the opposite points
    /*   smaller than another stem;
    /* - The common side of two stems is a straight line formed by two points
    /*   and the opposite point can be projected to line segment between those
    /*   two points. */
    if ( IsVectorHV( &stem1->unit,slope_error,true ) && !sbase->knownlinear ) { 
        int is_x = (int) rint( stem1->unit.y );
        if ( IsSplinePeak( gd,sp1,false,is_x ))
            peak1 = 1;
        else if ( IsSplinePeak( gd,sp1,true,is_x ))
            peak1 = 2;

        if ( IsSplinePeak( gd,sp2,false,is_x ))
            peak2 = 1;
        else if ( IsSplinePeak( gd,sp2,true,is_x ))
            peak2 = 2;

        if ( IsSplinePeak( gd,sp,false,is_x ))
            peak = 1;
        else if ( IsSplinePeak( gd,sp,true,is_x ))
            peak = 2;

        /* If both points are curved in the same direction, then check also 
        /* the "line of sight" between those points (if there are interventing
        /* splines, then it is not a real feature bend)*/
        if ( peak + peak1 == 3 && ConnectsAcross( gd,sp,is_next1,s1 ))
            val1++;
        if ( peak + peak2 == 3 && ConnectsAcross( gd,sp,is_next2,s2 ))
            val2++;
    }
    
    frompd = &gd->points[sbase->from->ttfindex];
    topd = &gd->points[sbase->to->ttfindex];
    
    if (( frompd->nextstem == stem1 || frompd->nextstem == stem2 ) &&
        ( topd->prevstem == stem1 || topd->prevstem == stem2 )) {
        fromproj = ( sp1->me.x-frompd->sp->me.x ) * stem1->unit.x +
            ( sp1->me.y-frompd->sp->me.y ) * stem1->unit.y;
        toproj = ( sp1->me.x-topd->sp->me.x ) * stem1->unit.x +
            ( sp1->me.y-topd->sp->me.y ) * stem1->unit.y;
        
        if (( fromproj < 0 && toproj > 0 ) ||
            ( toproj < 0 && fromproj > 0 ))
            val1++;
        
        fromproj = ( sp2->me.x-frompd->sp->me.x ) * stem2->unit.x +
            ( sp2->me.y-frompd->sp->me.y ) * stem2->unit.y;
        toproj = ( sp2->me.x-topd->sp->me.x ) * stem2->unit.x +
            ( sp2->me.y-topd->sp->me.y ) * stem2->unit.y;
        
        if (( fromproj < 0 && toproj > 0 ) ||
            ( toproj < 0 && fromproj > 0 ))
            val2++;
    }

    if ( dist1<0 ) dist1 = -dist1;
    if ( dist2<0 ) dist2 = -dist2;
    dist1 = sqrt( dist1 ); dist2 = sqrt( dist2 );
    if (( stem1->width < stem2->width ) && ( dist1 < dist2 ))
        val1++;
    else if (( stem2->width < stem1->width ) && ( dist2 < dist1 ))
        val2++;
    
    if ( val1 > 0 && val2 == 0 )
return( 1 );
    else if ( val2 > 0 && val1 == 0 )
return( -1 );
    else if ( val1 > 0 && val2 > 0 )
return( 2 );

return( 0 );
}

static struct pointdata *FindClosestOpposite( 
    struct stemdata *stem,struct stem_chunk **chunk,SplinePoint *sp,int *next ) {

    struct pointdata *pd, *ret=NULL;
    double test, proj=1e4;
    int i, is_l;
    
    for ( i=0; i<stem->chunk_cnt; ++i ) {
	struct stem_chunk *testchunk = &stem->chunks[i];
        pd = NULL;
	if ( testchunk->l != NULL && testchunk->l->sp==sp ) {
	    pd = testchunk->r;
            is_l = false;
	} else if ( testchunk->r != NULL && testchunk->r->sp==sp ) {
	    pd = testchunk->l;
            is_l = true;
	}
        
        if ( pd != NULL ) {
	    test = ( pd->sp->me.x-sp->me.x ) * stem->unit.x +
                   ( pd->sp->me.y-sp->me.y ) * stem->unit.y;
            if ( test < 0 ) test = -test;
            if ( test < proj ) {
                ret = pd;
                proj = test;
                *chunk = testchunk;
            }
        }
    }
    if ( ret != NULL )
        *next = ( is_l ) ? (*chunk)->lnext : (*chunk)->rnext;
return ret;
}

static int ChunkWorthOutputting( struct glyphdata *gd,struct stemdata *stem,
        int is_l,struct stem_chunk *chunk) {
    struct pointdata *alt, *pd1, *pd2, *base;
    struct stemdata *stem2;
    struct stem_chunk *chunk2;
    SplinePoint *sp1, *sp2;
    Spline *opposite;
    int next1, next2, nextalt;

    if ( stem->toobig )
return( -1 );
    
    if ( is_l ) {
	if ( chunk->r==NULL )
return( 0 );
	pd2 = chunk->l;
	pd1 = chunk->r;
        next2 = chunk->lnext;
        opposite = chunk->rnext ? chunk->r->nextedge : chunk->r->prevedge;
    } else {
	if ( chunk->l==NULL )
return( 0 );
	pd2 = chunk->r;
        pd1 = chunk->l;
        next2 = chunk->rnext;
        opposite = chunk->lnext ? chunk->l->nextedge : chunk->l->prevedge;
    }
    stem2 = ( next2 ) ? pd2->nextstem : pd2->prevstem;
    if ( stem2 == NULL || stem2 == stem || stem2->toobig ) 
return( 1 );
    
    sp1 = pd1->sp; sp2 = pd2->sp;
    base = FindClosestOpposite( stem,&chunk,sp2,&next1 );
    /* This is not the best chunk to check the relevance of this stem for the given point.
    /* So skip it and wait for a better one */
    if ( base != pd1 )
return( 0 );

    alt = FindClosestOpposite( stem2,&chunk2,sp2,&nextalt );
    if ( alt !=NULL )
return( CompareStems( gd,sp2,sp1,alt->sp,stem,stem2,next2,next1,nextalt ));
return( 1 );
}

static int StemIsActiveAt(struct glyphdata *gd,struct stemdata *stem,double stempos ) {
    BasePoint pos,cpos,mpos;
    int which;
    double test;
    double lmin, lmax, rmin, rmax, loff, roff, minoff, maxoff;
    struct monotonic **space, *m;
    int winding, nw, closest, i, j;

    pos.x = stem->left.x + stempos*stem->unit.x;
    pos.y = stem->left.y + stempos*stem->unit.y;

    if ( IsVectorHV( &stem->unit,0,true )) {
	which = stem->unit.x==0;
	MonotonicFindAt(gd->ms,which,((real *) &pos.x)[which],space = gd->space);
	test = ((real *) &pos.x)[!which];

        lmin = ( stem->lmin < -dist_error_hv ) ? stem->lmin : -dist_error_hv;
        lmax = ( stem->lmax > dist_error_hv ) ? stem->lmax : dist_error_hv;
        rmin = ( stem->rmin < -dist_error_hv ) ? stem->rmin : -dist_error_hv;
        rmax = ( stem->rmax > dist_error_hv ) ? stem->rmax : dist_error_hv;
        minoff = test + ( lmin * stem->unit.y - lmax * stem->unit.x );
        maxoff = test + ( lmax * stem->unit.y - lmin * stem->unit.x );

	winding = 0; closest = -1;
        for ( i=0; space[i]!=NULL; ++i ) {
	    m = space[i];
	    nw = ((&m->xup)[which] ? 1 : -1 );
            if ( m->other >= minoff && m->other <= maxoff && nw == 1 ) {
                closest = i;
        break;
            } else if ( m->other > maxoff )
        break;
	    winding += nw;
        }
        if ( closest < 0 )
return( false );

        cpos.x = ( which ) ? m->other : pos.x ;
        cpos.y = ( which ) ? pos.y : m->other ;
        loff = ( cpos.x - stem->left.x ) * stem->unit.y -
               ( cpos.y - stem->left.y ) * stem->unit.x;
	if ( loff > lmax || loff < lmin )
return( false );

	j = MatchWinding(space,i,nw,winding,which);
	if ( j==-1 )
return( false );
	m = space[j];

        mpos.x = ( which ) ? m->other : pos.x ;
        mpos.y = ( which ) ? pos.y : m->other ;
        roff = ( mpos.x - stem->right.x ) * stem->unit.y -
               ( mpos.y - stem->right.y ) * stem->unit.x;
        if ( roff >= rmin && roff <= rmax )
return( true );
return( false );
    } else {
return( StillStem( gd,dist_error_diag,&pos,stem ));
    }
}

/* This function now has 2 modes. In the first mode (if 'check_curved' is
/* false) it is used to determine if curved segments are flat enough
/* to treat them as lines. However, if a segment is obviously curved,
/* then we can use the same function to determine the extent where
/* the spline is still close enough to the hint edge to consider the
/* hint "active" at this zone. This method produces better effect than
/* any "magic" manipulations with control point coordinates, because it 
/* takes into account just the spline configuration rather than the 
/* positions of the points on that spline (well, unless they are positioned 
/* so closely that the spline segment we are checking is too short for 
/* making any reasonable conclusions) */
static int WalkSpline( struct glyphdata *gd, SplinePoint *sp,int gonext,
    int check_curved,struct stemdata *stem,int is_l,BasePoint *res ) {
    
    int i;
    double off,diff,min,max;
    double incr, err;
    double t=.5;
    Spline *s;
    BasePoint *base,pos,good;
    /* Some splines have tiny control points and are almost flat */
    /*  think of them as lines then rather than treating them as curves */
    /*  figure out how long they remain within a few orthoganal units of */
    /*  the point */

    base = ( is_l ) ? &stem->left : &stem->right;
    if ( check_curved ) {
        max = err = dist_error_curve;
        min = -dist_error_curve;
    } else {
        err = ( IsVectorHV( &stem->unit,0,true )) ? dist_error_hv : dist_error_diag;
        min = ( is_l ) ? stem->lmax - 2*err : stem->rmax - 2*err;
        max = ( is_l ) ? stem->lmin + 2*err : stem->rmin + 2*err;
    }
    
    if ( gonext ) {
	s = sp->next;
	incr = .25;
	*res = sp->nextcp;
    } else {
	s = sp->prev;
	incr = -.25;
	*res = sp->prevcp;
    }
    good = *res;
    for ( i=0; i<6; ++i ) {
	pos.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	pos.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
	diff = (pos.x - base->x)*stem->l_to_r.x +
               (pos.y - base->y)*stem->l_to_r.y;
	if ( diff > min && diff < max && StillStem( gd,err,&pos,stem )) {
	    good = pos;
	    t += incr;
	} else
	    t -= incr;
	incr/=2;
    }
    off = (res->x-sp->me.x)*stem->unit.x + (res->y-sp->me.y)*stem->unit.y;
    diff = (good.x-sp->me.x)*stem->unit.x + (good.y-sp->me.y)*stem->unit.y;
    if (( off>0 && diff>off ) || ( off<0 && diff<off )) {
	*res = good;
return( false );	/* Treat as a funny line */
    } else if ( check_curved )
	*res = good;
    
return( true );		/* Treat as curved */
}

static int AdjustForImperfectSlopeMatch( SplinePoint *sp,BasePoint *pos,
    BasePoint *newpos,struct stemdata *stem,int is_l ) {
   
    double off,err,min,max;
    BasePoint *base;
    
    base = ( is_l ) ? &stem->left : &stem->right;
    err = ( IsVectorHV( &stem->unit,0,true )) ? dist_error_hv : dist_error_diag;
    min = ( is_l ) ? stem->lmax - 2*err : stem->rmax - 2*err;
    max = ( is_l ) ? stem->lmin + 2*err : stem->rmin + 2*err;

    off = ( pos->x - base->x )*stem->l_to_r.x +
	  ( pos->y - base->y )*stem->l_to_r.y;
    if ( off > min && off < max )
return( false );
    if ( off < 0 ) off = -off;

    newpos->x = sp->me.x + err*( pos->x - base->x )/off;
    newpos->y = sp->me.y + err*( pos->y - base->y )/off;
return( true );
}

static int AddLineSegment( struct stemdata *stem,struct segment *space,int cnt,
	int is_l,struct pointdata *pd,struct glyphdata *gd ) {
    double s, e;
    BasePoint stemp, etemp, stemp2, etemp2;
    BasePoint *start, *end;
    int scurved = false, ecurved = false;
    SplinePoint *sp, *psp, *nsp;
    double b;
    
    if ( pd==NULL || (sp = pd->sp)==NULL || sp->ticked ||
	    sp->next==NULL || sp->prev==NULL )
return( cnt );
    end = &sp->me;
    start = &sp->me;
    if ( UnitsParallel( &pd->nextunit,&stem->unit,false )) {
	if ( sp->next->knownlinear ) {
	    nsp = sp->next->to;
	    end = &nsp->me;
	} else {
	    ecurved = WalkSpline( gd,sp,true,false,stem,is_l,&etemp );
            if ( ecurved ) {
                /* Second pass to get a proper end for a curved segment */
                WalkSpline( gd,sp,true,true,stem,is_l,&etemp );
            } else
                /* Can merge, but treat as curved relatively to projections */
                ecurved = 2;
            
	    end = &etemp;
	}
    } else {
	if ( !WalkSpline( gd,sp,true,false,stem,is_l,&etemp ))
	    end = &etemp;
    }
    if ( UnitsParallel( &pd->prevunit,&stem->unit,false )) {
	if ( sp->prev->knownlinear ) {
	    psp = sp->prev->from;
	    start = &psp->me;
	} else {
	    scurved = WalkSpline( gd,sp,false,false,stem,is_l,&stemp );
            if ( scurved ) {
                /* Second pass to get a proper start for a curved segment */
                WalkSpline( gd,sp,false,true,stem,is_l,&stemp );
            } else
                /* Can merge, but treat as curved relatively to projections */
                scurved = 2;
            
	    start = &stemp;
	}
    } else {
	if ( !WalkSpline(gd,sp,false,false,stem,is_l,&stemp))
	    start = &stemp;
    }
    sp->ticked = true;

    /* Now if the segment sp-start doesn't have exactly the right slope, then */
    /*  we can only use that bit of it which is less than a standard error */
    if ( !scurved && AdjustForImperfectSlopeMatch( sp,start,&stemp2,stem,is_l )) {
        start = &stemp2;
        scurved = true;
    }
    if ( !ecurved && AdjustForImperfectSlopeMatch( sp,end,&etemp2,stem,is_l )) {
        end = &etemp2;
        ecurved = true;
    }

    s = (start->x-stem->left.x)*stem->unit.x +
	(start->y-stem->left.y)*stem->unit.y;
    e = (  end->x-stem->left.x)*stem->unit.x +
	(  end->y-stem->left.y)*stem->unit.y;
    b = (sp->me.x-stem->left.x)*stem->unit.x +
	(sp->me.y-stem->left.y)*stem->unit.y;

    if ( s==e )
return( cnt );
    if ( s>e ) {
	double t = s;
	int c = scurved;
	s = e;
	e = t;
	scurved = ecurved;
	ecurved = c;
    }
    space[cnt].start = s;
    space[cnt].end = e;
    space[cnt].sbase = space[cnt].ebase = b;
    space[cnt].scurved = scurved;
    space[cnt].ecurved = ecurved;
    
    if ( stem->unit.x == 0 || stem->unit.y == 0 ) {
        /* For vertical/horizontal stems we assign a special meaning to
        /* the 'curved' field. It will be non-zero if the key point of
        /* this segment is positioned on a prominent curve: 
        /* 1 if the inner side of that curve is inside of the contour
        /* and 2 otherwise.
        /* Later, if we get a pair of "inner" and "outer curves, then
        /* we are probably dealing with a feature's bend which should be
        /* necessarily marked with a hint. Checks we apply for this type
        /* of curved segments should be less strict than in other cases. */
        if ( scurved && ecurved && IsSplinePeak( gd,sp,false,(int) rint( stem->unit.y )))
            space[cnt].curved = 1;
        else if ( scurved && ecurved && IsSplinePeak( gd,sp,true,(int) rint( stem->unit.y )))
            space[cnt].curved = 2;
        else
            space[cnt].curved = 0;
    } else {
        /* For diagonal stems we consider a segment "curved" if both its
        /* start and end are curved. Curved segments usually cannot be
        /* merged (unless scurved or ecurved is equal to 2) and are not
        /* checked for "projections". */
        space[cnt].curved = scurved && ecurved;
    }

return( cnt+1 );
}

static int segment_cmp(const void *_s1, const void *_s2) {
    const struct segment *s1 = _s1, *s2 = _s2;
    if ( s1->start<s2->start )
return( -1 );
    else if ( s1->start>s2->start )
return( 1 );

return( 0 );
}

static int proj_cmp(const void *_p1, const void *_p2) {
    struct pointdata * const *p1 = _p1, * const *p2 = _p2;
    if ( (*p1)->projection<(*p2)->projection )
return( -1 );
    else if ( (*p1)->projection>(*p2)->projection )
return( 1 );

return( 0 );
}

static int InActive(double projection,struct segment *segments, int cnt) {
    int i;

    for ( i=0; i<cnt; ++i ) {
	if ( projection>=segments[i].start && projection<=segments[i].end )
return( true );
    }
return( false );
}

static int MergeSegments(struct segment *space, int cnt) {
    int i,j;

    for ( i=j=0; i<cnt; ++i, ++j ) {
	if ( i!=j )
	    space[j] = space[i];
	while ( i+1<cnt && space[i+1].start<space[j].end ) {
	    if ( space[i+1].end>space[j].end ) {
                
		/* If there are 2 overlapping segments and neither the
                /* end of the first segment nor the start of the second
                /* one are curved we can merge them. Otherwise we have
                /* to preserve them both, but modify their start/end properties
                /* so that the overlap is removed */
                if ( space[j].ecurved != 1 && space[i+1].scurved != 1 ) {
                    space[j].end = space[i+1].end;
                    space[j].ebase = space[i+1].ebase;
		    space[j].ecurved = space[i+1].ecurved;
		    space[j].curved = false;
                } else if ( space[j].ecurved != 1 && space[i+1].scurved == 1 ) {
                    space[i+1].start = space[j].end;
                    --i;
                } else if ( space[j].ecurved == 1 && space[i+1].scurved != 1 ) {
                    space[j].end = space[i+1].start;
                    --i;
                } else {
                    double middle = (space[j].end + space[i+1].start)/2;
                    space[j].end = space[i+1].start = middle;
                    --i;
                }
	    }
	    ++i;
	}
    }
return( j );
}

static void FigureStemActive( struct glyphdata *gd, struct stemdata *stem ) {
    int i, j, pcnt=0;
    struct pointdata *pd, **pspace = gd->pspace;
    struct segment *lspace = gd->lspace, *rspace = gd->rspace;
    struct segment *bothspace = gd->bothspace, *activespace = gd->activespace;
    int lcnt, rcnt, bcnt, bpos, acnt;
    double middle, width, len, clen;

    width = stem->width;
    
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL )
	gd->points[i].sp->ticked = false;

    lcnt = rcnt = 0;
    for ( i=0; i<stem->chunk_cnt; ++i ) {
        if ( stem->chunks[i].stemcheat )
    continue;
	lcnt = AddLineSegment( stem,lspace,lcnt,true,stem->chunks[i].l,gd );
	rcnt = AddLineSegment( stem,rspace,rcnt,false,stem->chunks[i].r,gd );
    }
    bcnt = 0;
    if ( lcnt!=0 && rcnt!=0 ) {
        /* For curved segments we can extend left and right active segments 
        /* a bit to ensure that they do overlap and thus can be marked with an
        /* active zone */
	if ( rcnt == lcnt && stem->chunk_cnt == lcnt ) {
	    double gap, lseg, rseg;
            int cove;
            for ( i=0; i<lcnt; i++ ) {
                /* If it's a feature bend, then our tests should be more liberal */
                cove = (( rspace[i].curved + lspace[i].curved ) == 3 );
	        if ( lspace[i].start>rspace[i].end && lspace[i].scurved && rspace[i].ecurved )
		    gap = lspace[i].start-rspace[i].end;
	        else if ( rspace[i].start>lspace[i].end && rspace[i].scurved && lspace[i].ecurved )
		    gap = rspace[i].start-lspace[i].end;
	        else
	    continue;

                lseg = lspace[i].end - lspace[i].start;
                rseg = rspace[i].end - rspace[i].start;
	        if (( cove && gap < (lseg > rseg ? lseg : rseg )) ||
                    ( gap < ( lseg + rseg )/2 )) {
		    if ( lspace[i].ebase<rspace[i].start )
		        rspace[i].start = lspace[i].ebase;
		    else if ( lspace[i].sbase>rspace[i].end )
		        rspace[i].end = lspace[i].sbase;
		    if ( rspace[i].ebase<lspace[i].start )
		        lspace[i].start = rspace[i].ebase;
		    else if ( rspace[i].sbase>lspace[i].end )
		        lspace[i].end = rspace[i].sbase;
	        }
            }
	}
	qsort(lspace,lcnt,sizeof(struct segment),segment_cmp);
	qsort(rspace,rcnt,sizeof(struct segment),segment_cmp);
	lcnt = MergeSegments( lspace,lcnt );
	rcnt = MergeSegments( rspace,rcnt );
	for ( i=j=bcnt=0; i<lcnt && j<rcnt; ++i ) {
	    while ( j<rcnt && rspace[j].end<=lspace[i].start )
		++j;
	    while ( j<rcnt && rspace[j].start<=lspace[i].end ) {
                int cove = (( rspace[j].curved + lspace[i].curved ) == 3 );
                
                double s = ( rspace[j].start > lspace[i].start ) ?
                    rspace[j].start : lspace[i].start;
                double e = ( rspace[j].end < lspace[i].end ) ?
                    rspace[j].end : lspace[i].end;
                double sbase = ( rspace[j].start > lspace[i].start ) ?
                    lspace[i].sbase : rspace[j].sbase;
                double ebase = ( rspace[j].end < lspace[i].end ) ?
                    lspace[i].ebase : rspace[j].ebase;
                
                middle = ( lspace[i].start + rspace[j].start )/2;
                bothspace[bcnt].start = ( cove && middle < s ) ? middle : s;
                if ( rspace[j].start > lspace[i].start )
		    bothspace[bcnt].scurved = ( rspace[j].scurved || sbase < s ) ?
                        rspace[j].scurved : lspace[i].scurved;
                else
		    bothspace[bcnt].scurved = ( lspace[i].scurved || sbase < s ) ?
                        lspace[i].scurved : rspace[j].scurved;
                
                middle = ( lspace[i].end + rspace[j].end )/2;
                bothspace[bcnt].end = ( cove && middle > e ) ? middle : e;
                if ( rspace[j].end < lspace[i].end )
		    bothspace[bcnt].ecurved = ( rspace[j].ecurved || ebase > e ) ?
                        rspace[j].ecurved : lspace[i].ecurved;
                else
		    bothspace[bcnt].ecurved = ( lspace[i].ecurved || ebase > e ) ?
                        lspace[i].ecurved : rspace[j].ecurved;

		bothspace[bcnt++].curved = rspace[j].curved || lspace[i].curved;

		if ( rspace[j].end>lspace[i].end )
	    break;
		++j;
	    }
	}
    }
#if GLYPH_DATA_DEBUG
    fprintf( stderr, "Active zones for stem l=%f,%f r=%f,%f dir=%f,%f:\n",
        stem->left.x,stem->left.y,stem->right.x,stem->right.y,stem->unit.x,stem->unit.y );
    for ( i=0; i<lcnt; i++ )
        fprintf( stderr, "\tleft space start=%f,curved=%d,end=%f,curved=%d\n",
            lspace[i].start,lspace[i].scurved,lspace[i].end,lspace[i].ecurved );
    for ( i=0; i<rcnt; i++ )
        fprintf( stderr, "\tright space start=%f,curved=%d,end=%f,curved=%d\n",
            rspace[i].start,rspace[i].scurved,rspace[i].end,rspace[i].ecurved );
    for ( i=0; i<lcnt; i++ )
        fprintf( stderr, "\tboth space start=%f,curved=%d,end=%f,curved=%d\n",
            bothspace[i].start,bothspace[i].scurved,bothspace[i].end,bothspace[i].ecurved );
        fprintf( stderr, "\n" );
#endif

    double err = ( stem->unit.x == 0 || stem->unit.y == 0 ) ?
        dist_error_hv : dist_error_diag;
    double lmin = ( stem->lmin < -err ) ? stem->lmin : -err;
    double rmax = ( stem->rmax > err ) ? stem->rmax : err;
    acnt = 0;
    if ( bcnt!=0 ) {
	for ( i=0; i<gd->pcnt; ++i ) if ( (pd = &gd->points[i])->sp!=NULL ) {
	    /* Let's say we have a stem. And then inside that stem we have */
	    /*  another rectangle. So our first stem isn't really a stem any */
	    /*  more (because we hit another edge first), yet it's still reasonable*/
	    /*  to align the original stem */
	    /* Now suppose the rectangle is rotated a bit so we can't make */
	    /*  a stem from it. What do we do here? */
            double loff = ( pd->sp->me.x - stem->left.x ) * stem->unit.y -
                          ( pd->sp->me.y - stem->left.y ) * stem->unit.x;
            double roff = ( pd->sp->me.x - stem->right.x ) * stem->unit.y -
                          ( pd->sp->me.y - stem->right.y ) * stem->unit.x;
                
            if ( loff >= lmin && roff <= rmax ) {
		pd->projection = (pd->sp->me.x - stem->left.x)*stem->unit.x +
				 (pd->sp->me.y - stem->left.y)*stem->unit.y;
		if ( InActive(pd->projection,bothspace,bcnt) )
		    pspace[pcnt++] = pd;
	    }
	}
	qsort(pspace,pcnt,sizeof(struct pointdata *),proj_cmp);

        bpos = i = 0;
	while ( bpos<bcnt ) {
	    if ( bothspace[bpos].curved || pcnt==0 ) {
		activespace[acnt++] = bothspace[bpos++];
	    } else {
                double last = bothspace[bpos].start;
                int startset=false, endset=false;

 		if ( bothspace[bpos].scurved || 
                    StemIsActiveAt( gd,stem,bothspace[bpos].start+0.0015 )) {
                    
		    activespace[acnt].scurved = bothspace[bpos].scurved;
		    activespace[acnt].start  = bothspace[bpos].start;
                    startset = true;
                }
                
                /* If the stem is preceded by a curved segment, then skip
                /* the first point position and start from the next one.
                /* (Otherwise StemIsActiveAt() may consider the stem is
                /* "inactive" at the fragment between the start of the active
                /* space and the first point actually belonging to this stem)*/
                if ( bothspace[bpos].scurved ) {
                    while ( pcnt>i && pspace[i]->projection <= bothspace[bpos].start ) i++;
                    
                    if ( pcnt > i && pspace[i]->projection > bothspace[bpos].start ) {
                        last = activespace[acnt].end = pspace[i]->projection;
                        activespace[acnt].ecurved = false;
                        activespace[acnt].curved = false;
                        endset=true;
                    }
                }
                
		while ( i<pcnt && pspace[i]->projection<bothspace[bpos].end ) {
                    if ( last==activespace[acnt].start && pspace[i]->projection >= last ) {

			if ( !StemIsActiveAt( gd,stem,last+(( 1.001*pspace[i]->projection-last )/2.001 ))) {
			    last = activespace[acnt].start = pspace[i]->projection;
                            activespace[acnt].scurved = false;
                            startset = true; endset = false;
                        } else {
			    last = activespace[acnt].end = pspace[i]->projection;
                            activespace[acnt].ecurved = false;
                            activespace[acnt].curved = false;
                            endset = true;
                        }
                    } else if (( last==activespace[acnt].end || !startset )
                        && pspace[i]->projection >= last) {
                        
			if ( !StemIsActiveAt( gd,stem,last+(( 1.001*pspace[i]->projection-last )/2.001 )) || 
                            !startset ) {
                            
			    if ( startset ) acnt++;
                            last = activespace[acnt].start = pspace[i]->projection;
                            activespace[acnt].scurved = false;
                            startset = true; endset = false;
			} else {
			    last = activespace[acnt].end = pspace[i]->projection;
                            activespace[acnt].ecurved = false;
                            activespace[acnt].curved = false;
                            endset = true;
                        }
                    }
                    ++i;
		}
                
                if ( (bothspace[bpos].ecurved || 
                    StemIsActiveAt( gd,stem,bothspace[bpos].end-0.0015 )) &&
                    startset ) {

		    activespace[acnt].end = bothspace[bpos].end;
		    activespace[acnt].ecurved = bothspace[bpos].ecurved;
		    activespace[acnt].curved = bothspace[bpos].curved;
                    endset = true;
		}
		++bpos;
                if ( endset ) ++acnt;
	    }
	}
    }

    for ( i=0; i<stem->chunk_cnt; ++i ) {
	struct stem_chunk *chunk = &stem->chunks[i];
        /* stemcheat 1 -- diagonal edge stem;
        /*           2 -- diagonal corner stem with a sharp top;
        /*           3 -- diagonal corner stem with a flat top;
        /*           4 -- bounding box hint */
	if ( chunk->stemcheat==3 && chunk->l!=NULL && chunk->r!=NULL &&
		i+1<stem->chunk_cnt &&
		stem->chunks[i+1].stemcheat==3 && 
                ( chunk->l==stem->chunks[i+1].l ||
                chunk->r==stem->chunks[i+1].r )) {
            
	    SplinePoint *sp = chunk->l==stem->chunks[i+1].l ?
                chunk->l->sp : chunk->r->sp;
	    double proj = (sp->me.x - stem->left.x) *stem->unit.x +
			  (sp->me.y - stem->left.y) *stem->unit.y;

	    SplinePoint *sp2 = chunk->l==stem->chunks[i+1].l ?
                chunk->r->sp : chunk->l->sp;
	    SplinePoint *sp3 = chunk->l==stem->chunks[i+1].l ?
                stem->chunks[i+1].r->sp : stem->chunks[i+1].l->sp;
	    double proj2 = (sp2->me.x - stem->left.x) *stem->unit.x +
			   (sp2->me.y - stem->left.y) *stem->unit.y;
	    double proj3 = (sp3->me.x - stem->left.x) *stem->unit.x +
			   (sp3->me.y - stem->left.y) *stem->unit.y;

            if ( proj2>proj3 ) {
                double ptemp = proj2; proj2 = proj3; proj3 = ptemp;
            }
            
            if ( (proj3-proj2) < width ) {
	        activespace[acnt  ].curved = true;
                proj2 -= width/2;
                proj3 += width/2;
            } else {
	        activespace[acnt  ].curved = false;
            }
            
	    activespace[acnt].start = proj2;
	    activespace[acnt].end = proj3;
	    activespace[acnt].sbase = activespace[acnt].ebase = proj;
            acnt++;
	    ++i;
        /* The following is probably not needed. Bounding box hints don't 
        /* correspond to any actual glyph features, and their "active" zones 
        /* usually look ugly when displayed. So we don't attempt to calculate
        /* those faked "active" zones and instead just exclude bounding
        /* box hints from any validity checks based on the hint's "active"
        /* length */
	} else if ( chunk->stemcheat==4 && chunk->l!=NULL && chunk->r!=NULL ) {
#if 0
	    SplinePoint *sp = chunk->l->sp;
	    SplinePoint *sp2 = chunk->r->sp;
	    double proj = (sp->me.x - stem->left.x) *stem->unit.x +
			  (sp->me.y - stem->left.y) *stem->unit.y;
	    double proj2 = (sp2->me.x - stem->left.x) *stem->unit.x +
			   (sp2->me.y - stem->left.y) *stem->unit.y;
	    activespace[acnt  ].curved = false;
	    if ( proj2<proj ) {
		activespace[acnt].start = proj2;
		activespace[acnt].end = proj;
	    } else {
		activespace[acnt].start = proj;
		activespace[acnt].end = proj2;
	    }
	    activespace[acnt].sbase = activespace[acnt].ebase = proj;
            acnt++;
	    ++i;
#endif
	} else if ( chunk->stemcheat && chunk->l!=NULL && chunk->r!=NULL ) {
	    SplinePoint *sp = chunk->l->sp;
	    double proj = (sp->me.x - stem->left.x) * stem->unit.x +
			  (sp->me.y - stem->left.y) * stem->unit.y;
            double orig_proj = proj;
	    SplinePoint *other = chunk->lnext ? sp->prev->from : sp->next->to;
	    double len = (other->me.x - sp->me.x) * stem->unit.x +
			 (other->me.y - sp->me.y) * stem->unit.y;
            if ( chunk->stemcheat == 2 )
		proj -= width/2;
	    else if ( len<0 )
		proj -= width;
	    activespace[acnt].curved = true;
	    activespace[acnt].start = proj;
	    activespace[acnt].end = proj+width;
	    activespace[acnt].sbase = activespace[acnt].ebase = orig_proj;
            acnt++;
	}
    }

    stem->activecnt = acnt;
    if ( acnt!=0 ) {
	stem->active = galloc(acnt*sizeof(struct segment));
	memcpy(stem->active,activespace,acnt*sizeof(struct segment));
    }

    len = clen = 0;
    for ( i=0; i<acnt; ++i ) {
	if ( stem->active[i].curved )
	    clen += stem->active[i].end-stem->active[i].start;
	else
	    len += stem->active[i].end-stem->active[i].start;
    }
    stem->len = len; stem->clen = len+clen;
}

static void GDStemsFixupIntersects(struct glyphdata *gd) {
    int i,j;

    for ( i=0; i<gd->stemcnt; ++i ) {
	struct stemdata *stem = &gd->stems[i];
	for ( j=0; j<stem->chunk_cnt; ++j ) {
	    struct stem_chunk *chunk = &stem->chunks[j];
	    if ( chunk->l!=NULL && stem==chunk->l->nextstem )
		FixupT(stem,chunk->l,true);
	    if ( chunk->l!=NULL && stem==chunk->l->prevstem )
		FixupT(stem,chunk->l,false);
	    if ( chunk->r!=NULL && stem==chunk->r->nextstem )
		FixupT(stem,chunk->r,true);
	    if ( chunk->r!=NULL && stem==chunk->r->prevstem )
		FixupT(stem,chunk->r,false);
	}
    }
}

static void GDFindMismatchedParallelStems(struct glyphdata *gd) {
    int i, action1, action2, nextl, prevl, next1, next2;
    struct pointdata *pd, *pd1=NULL, *pd2=NULL;
    struct stem_chunk *nextchunk,*prevchunk;

    for ( i=0; i<gd->pcnt; ++i ) if ( (pd = &gd->points[i])->sp!=NULL && pd->colinear ) {
	if ( pd->nextstem!=NULL && pd->prevstem!=NULL &&
            pd->nextstem != pd->prevstem ) {
	    
            pd1 = FindClosestOpposite( pd->nextstem,&nextchunk,pd->sp,&next1 );
            pd2 = FindClosestOpposite( pd->prevstem,&prevchunk,pd->sp,&next2 );
            
            if ( pd1!=NULL && pd2!=NULL ) {
                nextl = ( nextchunk->l == pd );
                prevl = ( prevchunk->l == pd );

                action1 = CompareStems( gd,pd->sp,pd1->sp,pd2->sp,
                    pd->nextstem,pd->prevstem,false,next1,next2 );
                action2 = CompareStems( gd,pd->sp,pd1->sp,pd2->sp,
                    pd->nextstem,pd->prevstem,true,next1,next2 );

                if ( action1 == action2 && action1 == 1 )
                    ReassignPotential( gd,pd->nextstem,nextchunk,nextl,1 );
                else if ( action1 == action2 && action1 == -1 )
                    ReassignPotential( gd,pd->prevstem,prevchunk,prevl,1 );
            }
	}
    }
}

static int StemsWouldConflict( struct stemdata *stem1,struct stemdata *stem2 ) {
    double loff, roff, s1, s2, e1, e2;
    int acnt1, acnt2;
    
    if ( stem1 == stem2 || !UnitsParallel( &stem1->unit,&stem2->unit,true ))
return( false );

    loff = ( stem2->left.x - stem1->left.x ) * stem1->unit.y -
           ( stem2->left.y - stem1->left.y ) * stem1->unit.x;
    roff = ( stem2->right.x - stem1->left.x ) * stem1->unit.y -
           ( stem2->right.y - stem1->left.y ) * stem1->unit.x;
    if (!( loff >= 0 && loff <= stem1->width ) && !( roff >= 0 && roff <= stem1->width ))
return( false );

    acnt1 = stem1->activecnt;
    acnt2 = stem2->activecnt;
    if ( acnt1 == 0 || acnt2 == 0 )
return( false );
    s1 = stem1->active[0].start; e1 = stem1->active[acnt1-1].end;
    s2 = stem2->active[0].start; e2 = stem2->active[acnt2-1].end;
    
    loff = ( stem2->left.x - stem1->left.x ) * stem1->unit.x +
           ( stem2->left.y - stem1->left.y ) * stem1->unit.y;
    if (( s2+loff >= s1 && s2+loff <= e1 ) || ( e2+loff >= s1 && e2+loff <= e1 ) ||
        ( s2+loff <= s1 && e2+loff >= e1 ) || ( e2+loff <= s1 && s2+loff >= e1 ))
return( true );

return( false );
}

static void GDFindUnlikelyStems( struct glyphdata *gd ) {
    double width, minl;
    double em = (gd->sc->parent->ascent+gd->sc->parent->descent);
    int i, j;
    struct pointdata *lpd, *rpd;
    Spline *ls, *rs;
    SplinePoint *lsp, *rsp;
    BasePoint *lunit, *runit;
    struct stemdata *stem, *stem1, *lstem, *rstem, *tstem;

    GDStemsFixupIntersects(gd);
    
    /* If a stem has straight edges, and it is wider than tall */
    /*  then it is unlikely to be a real stem */
    for ( i=0; i<gd->stemcnt; ++i ) {
	struct stemdata *stem = &gd->stems[i];

	width = stem->width;
	stem->toobig =  (               width<=em/8 && 2.0*stem->clen < width ) ||
			( width>em/8 && width<=em/4 && 1.5*stem->clen < width ) ||
			( width>em/4 &&                    stem->clen < width );
    }

    /* One more check for curved stems. If a stem has just one active
    /* segment, this segment is curved and the stem has no conflicts,
    /* then select the active segment length which allows as to consider
    /* this stem suitable for PS output by such a way, that stems connecting
    /* the opposite sides of a circle are always accepted */
    for ( i=0; i<gd->stemcnt; ++i ) if ( gd->stems[i].toobig ) {
	stem = &gd->stems[i];
	width = stem->width;
        if ( stem->activecnt == 1 && stem->active[0].curved && 
            width/2 > dist_error_curve ) {
            
            for ( j=0; j<gd->stemcnt; ++j) {
	        stem1 = &gd->stems[j];

                if ( !stem1->toobig && StemsWouldConflict( stem,stem1 ))
            break;
            }

            if ( j == gd->stemcnt ) {
                minl = sqrt( pow( width/2,2 ) - pow( width/2 - dist_error_curve,2 ));
                if ( stem->clen >= minl ) stem->toobig = false;
            }
        }
    }

    /* And finally a check for stubs and feature terminations. We don't
    /* want such things to be controlled by any special hints, if there
    /* is already a hint controlling the middle of the same feature */
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
        
        if ( stem->chunk_cnt == 1 && stem->chunks[0].stub && !IsVectorHV( &stem->unit,0,true )) {
            struct stem_chunk *chunk = &stem->chunks[0];
            
            lsp = chunk->l->sp; lstem = tstem = NULL;
            do {
                ls = ( chunk->lnext ) ? lsp->next : lsp->prev;
                if ( ls == NULL )
            break;
                if ( tstem != NULL ) {
                    lstem = tstem;
            break;
                }
                lsp = ( chunk->lnext ) ? ls->to : ls->from;
                lpd = &gd->points[lsp->ttfindex];
                tstem = ( chunk->lnext ) ? lpd->prevstem : lpd->nextstem;
                lunit = ( chunk->lnext ) ? &lpd->prevunit : &lpd->nextunit;
            } while ( lsp != chunk->l->sp && lsp != chunk->r->sp &&
                UnitsParallel( lunit,&stem->unit,false ));

            rsp = chunk->r->sp; rstem = tstem = NULL;
            do {
                rs = ( chunk->rnext ) ? rsp->next : rsp->prev;
                if ( rs == NULL )
            break;
                if ( tstem != NULL ) {
                    rstem = tstem;
            break;
                }
                rsp = ( chunk->rnext ) ? rs->to : rs->from;
                rpd = &gd->points[rsp->ttfindex];
                tstem = ( chunk->rnext ) ? rpd->prevstem : rpd->nextstem;
                runit = ( chunk->rnext ) ? &rpd->prevunit : &rpd->nextunit;
            } while ( rsp != chunk->r->sp && rsp != chunk->l->sp &&
                UnitsParallel( runit,&stem->unit,false ));
            
            if ( lstem != NULL && rstem !=NULL && lstem == rstem &&
                !lstem->toobig && IsVectorHV( &lstem->unit,0,true )) {
                stem->toobig = true;
            } else if ( IsVectorHV( &stem->unit,slope_error,false )) {
                stem->unit.x = rint( stem->unit.x );
                stem->unit.y = rint( stem->unit.y );
                stem->l_to_r.y = stem->unit.x;
                stem->l_to_r.x = stem->unit.y;
                /* recalculate the stem width so that it matches the new vector */
                /* we don't care about left/right offsets: as the stem has just
                /* one chunk, they should be zero anyway */
                stem->width = ( stem->right.x - stem->left.x ) * stem->unit.y -
                              ( stem->right.y - stem->left.y ) * stem->unit.x;
            }
        /* One more check for intersections between a curved segment and a
        /* straight feature. Imagine a curve intersected by two bars, like in a Euro 
        /* glyph. Very probably we will get two chunks, one controlling the uppest
        /* two points of intersection, and another the lowest two, and most probably
        /* these two chunks will get merged into a single stem (so this stem will
        /* even get an exactly vertical vector). Yet we don't need this stem because
        /* there is already a stem controlling the middle of the curve (between two
        /* bars).*/
        } else if ( stem->chunk_cnt == 2 && 
            (( stem->chunks[0].stub > 0 && stem->chunks[1].stub > 1 ) ||
             ( stem->chunks[0].stub > 1 && stem->chunks[1].stub > 0 ))) {
            for ( j=0; j<gd->stemcnt; ++j) {
	        stem1 = &gd->stems[j];
                if ( !stem1->toobig && StemsWouldConflict( stem,stem1 ))
            break;
            }

            if ( j < gd->stemcnt )
                stem->toobig = true;
        }
    }
}

static void CheckForBoundingBoxHints(struct glyphdata *gd) {
    /* Adobe seems to add hints at the bounding boxes of glyphs with no hints */
    int hcnt=0, vcnt=0, i;
    struct stemdata *stem;
    SplinePoint *minx, *maxx, *miny, *maxy;
    SplineFont *sf;
    int hv, emsize;
    struct stem_chunk *chunk;

    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( stem->toobig )
    continue;
        hv = IsVectorHV( &stem->unit,slope_error,true );
        if ( hv == 1 )
	    ++hcnt;
	else if ( hv == 2 )
	    ++vcnt;
    }
    if ( hcnt!=0 && vcnt!=0 )
return;

    minx = maxx = miny = maxy = NULL;
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	SplinePoint *sp = gd->points[i].sp;
	if ( minx==NULL )
	    minx = maxx = miny = maxy = sp;
	else {
	    if ( minx->me.x>sp->me.x )
		minx = sp;
	    else if ( maxx->me.x<sp->me.x )
		maxx = sp;
	    if ( miny->me.y>sp->me.y )
		miny = sp;
	    else if ( maxy->me.y<sp->me.y )
		maxy = sp;
	}
    }

    if ( minx==NULL )		/* No contours */
return;

    sf = gd->sc->parent;
    emsize = sf->ascent+sf->descent;

    if ( hcnt==0 && maxy->me.y!=miny->me.y && maxy->me.y-miny->me.y<emsize/2 ) {
	struct pointdata *pd = &gd->points[maxy->ttfindex],
			 *pd2 = &gd->points[miny->ttfindex];
	stem = FindOrMakeHVStem( gd,pd,pd2,true );
	chunk = AddToStem( stem,pd,pd2,false,true,4 );
	FigureStemActive( gd,stem );
	stem->toobig = false;
	stem->clen += stem->width;
    }

    if ( vcnt==0 && maxx->me.x!=minx->me.x && maxx->me.x-minx->me.x<emsize/2 ) {
	struct pointdata *pd = &gd->points[maxx->ttfindex],
			 *pd2 = &gd->points[minx->ttfindex];
	stem = FindOrMakeHVStem( gd,pd,pd2,false );
	chunk = AddToStem( stem,pd,pd2,false,true,4 );
	FigureStemActive( gd,stem );
	stem->toobig = false;
	stem->clen += stem->width;
    }
}

static struct stemdata *FindOrMakeGhostStem( struct glyphdata *gd,
    SplinePoint *sp,int blue,double width ) {
    int i;
    struct stemdata *stem=NULL, *tstem;
    BasePoint dir,left,right;

    dir.x = 1; dir.y = 0;
    for ( i=0; i<gd->stemcnt; ++i ) {
	tstem = &gd->stems[i];
	if ( tstem->blue == blue && !tstem->toobig &&
            ((sp->me.y > 0 && sp->me.y <= tstem->left.y ) ||
            ( sp->me.y <= 0 && sp->me.y >= tstem->right.y ))) {
            stem = tstem;
    break;
        }
    }

    if ( stem==NULL ) {
        left.x = right.x = sp->me.x;
        left.y = ( width == 21 ) ? sp->me.y + 21 : sp->me.y;
        right.y = ( width == 21 ) ? sp->me.y : sp->me.y - 20;

	stem = NewStem( gd,&dir,&left,&right );
        stem->ghost = true;
        stem->width = width;
        stem->blue = blue;
    }
return( stem );
}

static int AddGhostSegment( struct pointdata *pd,int cnt,double base,struct segment *space ) {
    double s, e, temp, pos, spos, epos;
    SplinePoint *sp, *nsp, *nsp2, *psp, *psp2;
    
    sp = nsp = psp = pd->sp;
    pos = pd->sp->me.y;
    
    /* First check if there are points on the same line lying further
    /* in the desired direction */
    if (( sp->next != NULL ) && ( sp->next->to->me.y == pos ))
        nsp = sp->next->to;
    if (( sp->prev != NULL ) && ( sp->prev->from->me.y == pos ))
        psp = sp->prev->from;
    
    if ( psp != sp ) {
        s = psp->me.x;
    } else if ( psp->noprevcp ) {
        psp2 = psp->prev->from;
        if ( psp2->me.y != psp->me.y ) {
            s = ( psp->me.x - psp2->me.x )/( psp->me.y - psp2->me.y )*20.0;
            if ( s < 0 ) s = -s;
            if ( psp2->me.x<psp->me.x )
                s = ( psp->me.x-psp2->me.x < s ) ? psp2->me.x : psp->me.x-s;
            else
                s = ( psp2->me.x-psp->me.x < s ) ? psp2->me.x : psp->me.x+s;
        } else
            s = psp->me.x;
    } else {
        s = ( pd->sp->me.x + psp->prevcp.x )/2;
    }

    if ( nsp != sp ) {
        e = nsp->me.x;
    } else if ( nsp->nonextcp ) {
        nsp2 = nsp->next->to;
        if ( nsp2->me.y != nsp->me.y ) {
            e = ( nsp->me.x - nsp2->me.x )/( nsp->me.y - nsp2->me.y )*20.0;
            if ( e < 0 ) e = -e;
            if ( nsp2->me.x<nsp->me.x )
                e = ( nsp->me.x-nsp2->me.x < e ) ? nsp2->me.x : nsp->me.x-e;
            else
                e = ( nsp2->me.x-nsp->me.x < e )  ? nsp2->me.x : nsp->me.x+e;
        } else
            e = nsp->me.x;
    } else {
        e = ( pd->sp->me.x + nsp->nextcp.x )/2;
    }
    
    spos = psp->me.x; epos = nsp->me.x;
    if ( s>e ) {
        temp = s; s = e; e = temp;
        temp = spos; spos = epos; epos = temp;
    }
    
    space[cnt].start = s - base;
    space[cnt].end = e - base;
    space[cnt].sbase = spos - base;
    space[cnt].ebase = epos - base;
    space[cnt].ecurved = space[cnt].scurved = space[cnt].curved = ( false );
    
    return( cnt+1 );
}

static void CheckForGhostHints( struct glyphdata *gd, BlueData *bd ) {
    /* PostScript doesn't allow a hint to stretch from one alignment zone to */
    /*  another. (Alignment zones are the things in bluevalues).  */
    /* Oops, I got this wrong. PS doesn't allow a hint to start in a bottom */
    /*  zone and stretch to a top zone. Everything in OtherBlues is a bottom */
    /*  zone. The baseline entry in BlueValues is also a bottom zone. Every- */
    /*  thing else in BlueValues is a top-zone. */
    /* This means */
    /*  that we can't define a horizontal stem hint which stretches from */
    /*  the baseline to the top of a capital I, or the x-height of lower i */
    /*  If we find any such hints we must remove them, and replace them with */
    /*  ghost hints. The bottom hint has height -21, and the top -20 */
    BlueData _bd;
    struct stemdata *stem;
    struct stem_chunk *chunk;
    SplineFont *sf;
    SplinePoint *sp;
    real base, width, len;
    int i, j, leftfound, rightfound, acnt;
    struct segment *activespace = gd->activespace;
    struct pointdata *valid;

    /* Get the alignment zones */
    sf = gd->sc->parent;
    if ( bd == NULL ) {
	QuickBlues( sf,&_bd );
	bd = &_bd;
    }

    /* look for any stems stretching from one zone to another and remove them */
    /*  (I used to turn them into ghost hints here, but that didn't work (for */
    /*  example on "E" where we don't need any ghosts from the big stem because*/
    /*  the narrow stems provide the hints that PS needs */
    /* However, there are counter-examples. in Garamond-Pro the "T" character */
    /*  has a horizontal stem at the top which stretches between two adjacent */
    /*  bluezones. Removing it is wrong. Um... Thanks Adobe */
    /* I misunderstood. Both of these were top-zones */
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
        if ( IsVectorHV( &stem->unit,0,true ) != 1)
    continue;
        
	leftfound = rightfound = -1;
	for ( j=0; j<bd->bluecnt; ++j ) {
	    if ( stem->left.y>=bd->blues[j][0]-1 && stem->left.y<=bd->blues[j][1]+1 )
		leftfound = j;
	    else if ( stem->right.y>=bd->blues[j][0]-1 && stem->right.y<=bd->blues[j][1]+1 )
		rightfound = j;
	}
        /* Assign value 2 to indicate this stem should be ignored also for TTF instrs */
        if ( leftfound !=-1 && rightfound !=-1 && 
	    ( stem->left.y > 0 && stem->right.y <= 0 ))
            stem->toobig = 2;
        /* Otherwise mark the stem as controlling a specific blue zone */
        else if ( leftfound != -1 && stem->left.y > 0 )
            stem->blue = leftfound;
        else if ( rightfound != -1 && stem->right.y <= 0 )
            stem->blue = rightfound;
    }

    /* Now look and see if we can find any edges which lie in */
    /*  these zones.  Edges which are not currently in hints */
    /* Use the winding number to determine top or bottom */
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
        stem = gd->points[i].nextstem;
        if ( stem != NULL && !stem->toobig &&
            IsVectorHV( &stem->unit,0,true ) == 1 )
    continue;
        stem = gd->points[i].prevstem;
        if ( stem != NULL && !stem->toobig &&
            IsVectorHV( &stem->unit,0,true ) == 1)
    continue;
        stem = gd->points[i].bothstem;
        if ( stem != NULL && !stem->toobig &&
            IsVectorHV( &stem->unit,0,true ) == 1)
    continue;
        
	sp = gd->points[i].sp;
	base = sp->me.y;
	for ( j=0; j<bd->bluecnt; ++j ) {
	    if ( base>=bd->blues[j][0]-1 && base<=bd->blues[j][1]+1 ) {
                int peak = IsSplinePeak( gd,sp,false,false );
                if ( peak ) {
                    width = ( peak>0 ) ? 20 : 21;
                    stem = FindOrMakeGhostStem( gd,sp,j,width );
                    chunk = AddToStem( stem,&gd->points[i],NULL,false,false,false );
                }
	    }
        }
    }
    
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
        if ( !stem->ghost )
    continue;
        
        acnt = 0;
        for ( j=0; j<stem->chunk_cnt; ++j ) {
            valid = ( stem->chunks[j].l != NULL) ? 
                stem->chunks[j].l : stem->chunks[j].r;
	    acnt = AddGhostSegment( valid,acnt,stem->left.x,activespace );
        }
	qsort(activespace,acnt,sizeof(struct segment),segment_cmp);
        acnt = MergeSegments( activespace,acnt );
        stem->activecnt = acnt;
        if ( acnt!=0 ) {
	    stem->active = galloc(acnt*sizeof(struct segment));
	    memcpy( stem->active,activespace,acnt*sizeof( struct segment ));
        }

        for ( j=0; j<acnt; ++j ) {
	    len += stem->active[j].end-stem->active[j].start;
        }
        stem->clen = stem->len = len;
    }
}

static void NormalizeStems( struct glyphdata *gd ) {
    int i, j, k;
    int *lposval, *rposval;
    int pos, testpos;
    double loff, roff, lsht;
    BasePoint lnew, rnew;
    
    for ( i=0; i<gd->stemcnt; ++i ) {
        struct stemdata *stem = &gd->stems[i];
        /* First sort the stem chunks by their coordinates */
        qsort( stem->chunks,stem->chunk_cnt,sizeof( struct stem_chunk ),chunk_cmp );
        
        if ( IsVectorHV( &stem->unit,0,true )) {
            /* For HV stems we have to check all chunks once more in order
            /* to figure out "left" and "right" positions most typical
            /* for this stem. We perform this by assigning a value to
            /* left and right side of this chunk. Each position which repeats 
            /* more than once gets a plus 1 value bonus */
            lposval = galloc( stem->chunk_cnt * sizeof( int ));
            rposval = galloc( stem->chunk_cnt * sizeof( int ));

            for ( j=0; j<stem->chunk_cnt; ++j ) {
                struct stem_chunk *chunk = &stem->chunks[j];
                int is_x = (int) rint( stem->unit.y );
                
                lposval[j] = 0;
                if ( chunk->l != NULL && !chunk->lpotential  ) {
                    pos = ((real *) &chunk->l->sp->me.x)[!is_x];
                    int lval = 0;
                    for ( k=0; k<j; k++ ) if ( stem->chunks[k].l != NULL ) {
                        testpos = ((real *) &stem->chunks[k].l->sp->me.x)[!is_x];
                        if ( pos == testpos ) {
                            lval = lposval[k]; ++lposval[k];
                        }
                    }
                    lposval[j] = lval+1;
                }

                rposval[j] = 0;
                if ( chunk->r != NULL && !chunk->rpotential ) {
                    pos = ((real *) &chunk->r->sp->me.x)[!is_x];
                    int rval = 0;
                    for ( k=0; k<j; k++ ) if ( stem->chunks[k].r != NULL ) {
                        testpos = ((real *) &stem->chunks[k].r->sp->me.x)[!is_x];
                        if ( pos == testpos ) {
                            rval = rposval[k]; ++rposval[k];
                        }
                    }
                    rposval[j] = rval+1;
                }
            }
            
            
            int best = -1; int val = 0;
            for ( j=0; j<stem->chunk_cnt; ++j ) {
                if ( lposval[j] > 0 && rposval[j] > 0 &&
                    ( lposval[j] + rposval[j] ) > val ) {
                    best = j;
                    val = lposval[j] + rposval[j];
                }
            }
            if ( best > -1 ) {
                lnew = stem->chunks[best].l->sp->me;
                rnew = stem->chunks[best].r->sp->me;
                /* Now assign "left" and "right" properties of the stem
                /* to point coordinates taken from the most "typical" chunk
                /* of this stem. We also have to recalculate stem width and
                /* left/right offset values */
                loff = ( lnew.x - stem->left.x ) * stem->unit.y -
                       ( lnew.y - stem->left.y ) * stem->unit.x;
                roff = ( rnew.x - stem->right.x ) * stem->unit.y -
                       ( rnew.y - stem->right.y ) * stem->unit.x;
                lsht = ( stem->left.x - lnew.x ) * stem->unit.x +
                       ( stem->left.y - lnew.y ) * stem->unit.y;
                
                stem->lmin -= loff; stem->lmax -= loff;
                stem->rmin -= roff; stem->rmax -= roff;
                stem->left = lnew; stem->right = rnew;
                stem->width = ( stem->right.x - stem->left.x ) * stem->unit.y -
                              ( stem->right.y - stem->left.y ) * stem->unit.x;
                
                /* Recalculate active zones relatively to the new left base point */
                if ( lsht != 0 ) {
                    for ( j=0; j<stem->activecnt; j++ ) {
                        stem->active[j].start += lsht;
                        stem->active[j].end += lsht;
                    }
                }
            }
            
            free( lposval );
            free( rposval );
        }
    }
}

#if GLYPH_DATA_DEBUG
static void DumpGlyphData( struct glyphdata *gd ) {
    int i, j;

    if ( gd->linecnt > 0 )
        fprintf( stderr, "\nDumping line data for %s\n",gd->sc->name );
    for ( i=0; i<gd->linecnt; ++i ) {
	struct linedata *line = &gd->lines[i];
        fprintf( stderr, "line vector=%f,%f\n", 
            line->unitvector.x,line->unitvector.y);
        for( j=0; j<line->pcnt;++j) {
            fprintf( stderr, "\tpoint num=%d, x=%f, y=%f, prev=%d, next=%d\n",
                line->points[j]->sp->ttfindex, line->points[j]->sp->me.x,
                line->points[j]->sp->me.y, 
                line->points[j]->prevline==line, line->points[j]->nextline==line);
        }
        fprintf( stderr, "\n" );
    }
    
    if ( gd->stemcnt > 0 )
        fprintf( stderr, "\nDumping stem data for %s\n",gd->sc->name );
    for ( i=0; i<gd->stemcnt; ++i ) {
	struct stemdata *stem = &gd->stems[i];
        fprintf( stderr, "stem l=%f,%f r=%f,%f vector=%f,%f\n\twidth=%f chunk_cnt=%d len=%f clen=%f toobig=%d\n\tlmin=%f,lmax=%f,rmin=%f,rmax=%f\n",
            stem->left.x,stem->left.y,stem->right.x,stem->right.y,
            stem->unit.x,stem->unit.y,stem->width,stem->chunk_cnt,stem->len,stem->clen,stem->toobig,
            stem->lmin,stem->lmax,stem->rmin,stem->rmax );
        for ( j=0; j<stem->chunk_cnt; ++j ) {
	    struct stem_chunk *chunk = &stem->chunks[j];
	    if ( chunk->l!=NULL && chunk->r!=NULL )
                fprintf (stderr, "\tchunk l=%f,%f potential=%d r=%f,%f potential=%d stub=%d\n",
                    chunk->l->sp->me.x, chunk->l->sp->me.y, chunk->lpotential,
                    chunk->r->sp->me.x, chunk->r->sp->me.y, chunk->rpotential, chunk->stub );
	    else if ( chunk->l!=NULL )
                fprintf (stderr, "\tchunk l=%f,%f potential=%d\n",
                    chunk->l->sp->me.x, chunk->l->sp->me.y, chunk->lpotential);
	    else if ( chunk->r!=NULL )
                fprintf (stderr, "\tchunk r=%f,%f potential=%d\n",
                    chunk->r->sp->me.x, chunk->r->sp->me.y, chunk->rpotential);
        }
        fprintf( stderr, "\n" );
    }
}
#endif

struct glyphdata *GlyphDataInit( SplineChar *sc,int only_hv ) {
    struct glyphdata *gd;
    int i;
    SplineSet *ss;
    SplinePoint *sp;
    Monotonic *m;
    int cnt;
    int em = sc->parent!=NULL ? sc->parent->ascent+sc->parent->descent : 1000;

    /* We can't hint type3 fonts, so the only layer we care about is ly_fore */
    /* We shan't try to hint references yet */
    if ( sc->layers[ly_fore].splines==NULL )
return( NULL );

    dist_error_hv = .0035*em;
    dist_error_diag = .008*em;
    dist_error_curve = .022*em;

    gd = gcalloc(1,sizeof(struct glyphdata));
    
    /* Normally we use the DetectDiagonalStems flag (set via the Preferences
    /* dialog) to determine if diagonal stems should be generated. However,
    /* it is possible to imagine a situation where we would like to get
    /* (for some internal purposes) just information about vertical and
    /* horizontal stems, deliberately turning the diagonal stem detection off.
    /* For this case the only_hv argument still can be passed to this function. */
    gd->only_hv = only_hv || !detect_diagonal_stems;

    /* SSToMContours can clean up the splinesets (remove 0 length splines) */
    /*  so it must be called BEFORE everything else (even though logically */
    /*  that doesn't make much sense). Otherwise we might have a pointer */
    /*  to something since freed */
    gd->ms = SSsToMContours(sc->layers[ly_fore].splines,over_remove);	/* second argument is meaningless here */

    gd->realcnt = gd->pcnt = SCNumberPoints(sc);
    for ( i=0, ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next, ++i );
    gd->ccnt = i;
    gd->contourends = galloc((i+1)*sizeof(int));
    for ( i=0, ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next, ++i ) {
	SplinePoint *last;
	if ( ss->first->prev!=NULL )
	    last = ss->first->prev->from;
	else
	    last = ss->last;
	if ( last->ttfindex==0xffff )
	    gd->contourends[i] = last->nextcpindex;
	else
	    gd->contourends[i] = last->ttfindex;
    }
    gd->contourends[i] = -1;
    gd->sc = sc;
    gd->sf = sc->parent;
    gd->fudge = (sc->parent->ascent+sc->parent->descent)/500;

    /* Create temporary point numbers for the implied points. We need this */
    /*  for metafont if nothing else */
    for ( ss= sc->layers[ly_fore].splines; ss!=NULL; ss = ss->next ) {
	for ( sp = ss->first; ; ) {
	    if ( sp->ttfindex == 0xffff )
		sp->ttfindex = gd->pcnt++;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    gd->norefpcnt = gd->pcnt;
    /* And for 0xfffe points such as those used in glyphs with order2 glyphs */
    /*  with references. */
    for ( ss= sc->layers[ly_fore].splines; ss!=NULL; ss = ss->next ) {
	for ( sp = ss->first; ; ) {
	    if ( sp->ttfindex == 0xfffe )
		sp->ttfindex = gd->pcnt++;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }

    gd->points = gcalloc(gd->pcnt,sizeof(struct pointdata));
    for ( ss=sc->layers[ly_fore].splines; ss!=NULL; ss=ss->next ) if ( ss->first->prev!=NULL ) {
	for ( sp=ss->first; ; ) {
	    PointInit(gd,sp,ss);
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }

    /*gd->ms = SSsToMContours(sc->layers[ly_fore].splines,over_remove);*/	/* second argument is meaningless here */
    for ( m=gd->ms, cnt=0; m!=NULL; m=m->linked, ++cnt );
    gd->space = galloc((cnt+2)*sizeof(Monotonic*));
    gd->mcnt = cnt;

    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if ( !pd->nextzero )
	    pd->nextedge = FindMatchingEdge(gd,pd,true);
	if ( !pd->prevzero )
	    pd->prevedge = FindMatchingEdge(gd,pd,false);
	if ( (pd->symetrical_h || pd->symetrical_v ))
	    pd->bothedge = FindMatchingEdge(gd,pd,2);
    }

#if 0
    /* Consider the slash character. We have a diagonal stem capped with two */
    /*  horizontal lines. From the upper right corner, a line drawn normal to*/
    /*  the right diagonal stem will intersect the top cap, very close to the*/
    /*  upper right corner. That isn't interesting information */
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if ( pd->nextedge == pd->sp->prev && pd->next_e_t>.9 &&
		gd->points[pd->sp->prev->from->ttfindex].prevedge == pd->sp->next ) {
	    pd->nextedge = pd->sp->prev->from->prev;
	    pd->next_e_t = 1;
	}
	if ( pd->prevedge == pd->sp->next && pd->prev_e_t<.1 &&
		gd->points[pd->sp->next->to->ttfindex].nextedge == pd->sp->prev ) {
	    pd->prevedge = pd->sp->next->to->next;
	    pd->prev_e_t = 0;
	}
    }
#endif
return( gd );
}

struct glyphdata *GlyphDataBuild( SplineChar *sc,BlueData *bd,int only_hv ) {
    struct glyphdata *gd;
    int i, j;
    int action;
    
    gd = GlyphDataInit( sc,only_hv );
    if ( gd ==  NULL )
return( gd );

    /* There will never be more lines than there are points (counting next/prev as separate) */
    gd->lines = galloc(2*gd->pcnt*sizeof(struct linedata));
    gd->linecnt = 0;
    gd->pspace = galloc(gd->pcnt*sizeof(struct pointdata *));
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if (( !gd->only_hv || pd->next_hor || pd->next_ver ) && pd->nextline==NULL ) {
	    pd->nextline = BuildLine(gd,pd,true);
	    if ( pd->colinear )
		pd->prevline = pd->nextline;
	}
	if (( !gd->only_hv || pd->prev_hor || pd->prev_ver ) && pd->prevline==NULL ) {
	    pd->prevline = BuildLine(gd,pd,false);
	    if ( pd->colinear && pd->nextline == NULL )
		pd->nextline = pd->prevline;
	}
    }

    /* There will never be more stems than there are points (counting next/prev as separate) */
    gd->stems = gcalloc(2*gd->pcnt,sizeof(struct stemdata));
    gd->stemcnt = 0;			/* None used so far */
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if ( pd->prevedge!=NULL ) {
	    BuildStem( gd,pd,false,false );
        }
	if ( pd->nextedge!=NULL ) {
	    BuildStem( gd,pd,true,false );
        }
	if ( pd->bothedge!=NULL ) {
	    DiagonalCornerStem(gd,pd);
        }
    }

    /* A second pass to include some points which have been neglected in
    /* favor of other points lying on the same stem */
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if ( pd->prevedge!=NULL && pd->prevstem!=NULL ) {
	    BuildStem( gd,pd,false,true );
        }
	if ( pd->nextedge!=NULL && pd->nextstem!=NULL ) {
	    BuildStem( gd,pd,true,true );
        }
    }

    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if ( pd->nextstem==NULL && pd->colinear ) {
	    pd->nextstem = pd->prevstem;
	    pd->next_is_l = pd->prev_is_l;
	} else if ( pd->prevstem==NULL && pd->colinear ) {
	    pd->prevstem = pd->nextstem;
	    pd->prev_is_l = pd->next_is_l;
	}
    }

    /* Figure out active zones at the first order (as they are needed to
    /* determine which stems are undesired and they don't depend from
    /* the "potential" state of left/right points in chunks */
    gd->lspace = galloc(gd->pcnt*sizeof(struct segment));
    gd->rspace = galloc(gd->pcnt*sizeof(struct segment));
    gd->bothspace = galloc(3*gd->pcnt*sizeof(struct segment));
    gd->activespace = galloc(3*gd->pcnt*sizeof(struct segment));
#if GLYPH_DATA_DEBUG
    fprintf( stderr,"Going to calculate stem active zones for %s\n",gd->sc->name );
#endif
    for ( i=j=0; i<gd->stemcnt; ++i )
	FigureStemActive( gd,&gd->stems[i] );

    /* Check this before resolving stem conflicts, as otherwise we can
    /* occasionally prefer a stem which should be excluded from the list 
    /* for some other reasons */
    GDFindUnlikelyStems( gd );

    /* we were cautious about assigning points to stems, go back now and see */
    /*  if there are any low-quality matches which remain unassigned, and if */
    /*  so then assign them to the stem they almost fit on. */
    /* If there are multiple stems, find the one which is closest to this point */
    for ( i=0; i<gd->stemcnt; ++i ) {
        struct stemdata *stem = &gd->stems[i];
        for ( j=0; j<stem->chunk_cnt; ++j ) {
            struct stem_chunk *chunk = &stem->chunks[j];
            if ( chunk->l!=NULL && chunk->lpotential ) {
                action = ChunkWorthOutputting( gd,stem,true,chunk );
                if ( action>0 )
		    ReassignPotential( gd,stem,chunk,true,action );
            }
            if ( chunk->r!=NULL && chunk->rpotential ) {
                action = ChunkWorthOutputting( gd,stem,false,chunk );
                if ( action>0 )
                    ReassignPotential( gd,stem,chunk,false,action );
            }
        }
    }
    GDFindMismatchedParallelStems( gd );
    NormalizeStems( gd );

    /* These types of stems cannot be considered "too big" and
    /* don't require any normalization */
    if ( hint_bounding_boxes )
	CheckForBoundingBoxHints(gd);
    CheckForGhostHints( gd,bd );

#if GLYPH_DATA_DEBUG
    DumpGlyphData( gd );
#endif
    FreeMonotonics(gd->ms);	gd->ms = NULL;
    free(gd->space);		gd->space = NULL;
    free(gd->sspace);		gd->sspace = NULL;
    free(gd->stspace);		gd->stspace = NULL;
    free(gd->pspace);		gd->pspace = NULL;
    free(gd->lspace);		gd->lspace = NULL;
    free(gd->rspace);		gd->rspace = NULL;
    free(gd->bothspace);	gd->bothspace = NULL;
    free(gd->activespace);	gd->activespace = NULL;

return( gd );
}

void GlyphDataFree(struct glyphdata *gd) {
    int i;

    /* Restore implicit points */
    for ( i=gd->realcnt; i<gd->norefpcnt; ++i )
	gd->points[i].sp->ttfindex = 0xffff;
    for ( i=gd->norefpcnt; i<gd->pcnt; ++i )
	gd->points[i].sp->ttfindex = 0xfffe;

    for ( i=0; i<gd->linecnt; ++i )
	free(gd->lines[i].points);
    for ( i=0; i<gd->stemcnt; ++i ) {
	free(gd->stems[i].chunks);
	free(gd->stems[i].active);
    }
    free(gd->lines);
    free(gd->stems);
    free(gd->contourends);
    free(gd->points);
    free(gd);
}
