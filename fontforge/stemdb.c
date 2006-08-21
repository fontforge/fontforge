/* Copyright (C) 2005,2006 by George Williams */
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

/* A diagonal end is like the top or bottom of a slash. Should we add a vertical stem at the end? */
/* A diagonal corner is like the bottom of circumflex. Should we add a horizontal stem */
enum { hint_diagonal_ends=1, hint_diagonal_corners=2 };
int hintcontrolflags = -1;

static const double slope_error = .05;
    /* If the dot vector of a slope and the normal of another slope is less */
    /*  than this error, then we say they are close enough to be colinear */
static double dist_error_hv = 3;
static double dist_error_diag = 6;
    /* It's easy to get horizontal/vertical lines aligned properly */
    /* it is more difficult to get diagonal ones done */
    /* The "A" glyph in Apple's Times.dfont(Roman) is off by 6 in one spot */

struct st {
    Spline *s;
    double st, lt;
};

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

    len = sqrt(average_dir.x*average_dir.x + average_dir.y*average_dir.y);
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
    len = sqrt(pd->nextunit.x*pd->nextunit.x + pd->nextunit.y*pd->nextunit.y);
    if ( len==0 )
	pd->nextzero = true;
    else {
	pd->nextlen = len;
	pd->nextunit.x /= len;
	pd->nextunit.y /= len;
	if ( sp->next!=NULL && !sp->next->knownlinear )
	    SplineFigureOpticalSlope(sp->next,true,&pd->nextunit);
	if ( pd->nextunit.x<slope_error && pd->nextunit.x>-slope_error ) {
	    pd->nextunit.x = 0; pd->nextunit.y = pd->nextunit.y>0 ? 1 : -1;
	} else if ( pd->nextunit.y<slope_error && pd->nextunit.y>-slope_error ) {
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
    len = sqrt(pd->prevunit.x*pd->prevunit.x + pd->prevunit.y*pd->prevunit.y);
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
    if ( (hintcontrolflags&hint_diagonal_corners) ) {
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
	struct stemdata *stem ) {
    int i,j;
    int eo;		/* I do horizontal/vertical by winding number */
			/* But figuring winding number with respect to a */
			/* diagonal line is hard. So I use even-odd */
			/* instead. */
    double pos, npos;

    eo = 0;
    for ( i=0; i<cnt; ++i ) {
	pos = (line->splines[0].c*stspace[i].lt + line->splines[0].d - stem->left.x)*stem->l_to_r.x +
		(line->splines[1].c*stspace[i].lt + line->splines[1].d - stem->left.y)*stem->l_to_r.y;
	npos = 99999999;
	if ( i+1<cnt )
	    npos = (line->splines[0].c*stspace[i+1].lt + line->splines[0].d - stem->left.x)*stem->l_to_r.x +
		    (line->splines[1].c*stspace[i+1].lt + line->splines[1].d - stem->left.y)*stem->l_to_r.y;

	if ( fabs(npos)<fabs(pos) )
	    /* Then npos is closer to the base of the stem and is what we want*/;
	else if ( pos>-dist_error_diag && pos<dist_error_diag ) {
	    if ( (eo&1) && i>0 )
		j = i-1;
	    else if ( !(eo&1) && i+1<cnt )
		j = i+1;
	    else
return( false );
	    pos = (line->splines[0].c*stspace[j].lt + line->splines[0].d - stem->left.x)*stem->l_to_r.x +
		    (line->splines[1].c*stspace[j].lt + line->splines[1].d - stem->left.y)*stem->l_to_r.y;
return( pos>=stem->width-dist_error_diag && pos<=stem->width+dist_error_diag );
	}
	if ( i+1<cnt && npos>-dist_error_diag && npos<dist_error_diag )
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
	int is_next, double *other_t ) {
    double test, t;
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
	if ( m->s == s && t>=m->tstart && t<=m->tend )
    break;
	winding += nw;
    }
    if ( space[i]==NULL ) {
	fprintf( stderr, "FindMatchinHVEdge didn't\n" );
return( NULL );
    }

    j = MatchWinding(space,i,nw,winding,which);
    if ( j!=-1 ) {
	*other_t = space[j]->t;
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

static Spline *FindMatchingEdge(struct glyphdata *gd, struct pointdata *pd,
	int is_next, int only_hv ) {
    BasePoint *dir, perturbed, diff;
    Spline myline;
    SplinePoint end1, end2;
    double *other_t = is_next==2 ? &pd->both_e_t : is_next ? &pd->next_e_t : &pd->prev_e_t;
    double t;
    Spline *s;
    int cnt;

    if ( ( is_next && (pd->next_hor || pd->next_ver)) ||
	    ( !is_next && ( pd->prev_hor || pd->prev_ver )) ||
	    is_next == 2 )
return( FindMatchingHVEdge(gd,pd,is_next,other_t));
    if ( only_hv ) {
	if (( is_next && ((pd->nextunit.y>-3*slope_error && pd->nextunit.y<3*slope_error) || (pd->nextunit.x>-3*slope_error && pd->nextunit.x<3*slope_error))) ||
		( !is_next && ((pd->prevunit.y>-3*slope_error && pd->prevunit.y<3*slope_error) || (pd->prevunit.x>-3*slope_error && pd->prevunit.x<3*slope_error))))
	    /* Close to vertical/horizontal */;
	else 
return( NULL );
    }

    if ( is_next ) {
	dir = &pd->nextunit;
	t = .001;
	s = pd->sp->next;
    } else {
	dir = &pd->prevunit;
	t = .999;
	s = pd->sp->prev;
    }
    if ( s==NULL )
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
    double n_s_err, p_s_err, off;

    dir = is_next ? &pd->nextunit : &pd->prevunit;
    dist_error = ( dir->x==0 || dir->y==0 ) ? dist_error_hv : dist_error_diag ;	/* Diagonals are harder to align */
    if ( dir->x==0 && dir->y==0 )
return( NULL );
    base = &pd->sp->me;

    for ( i= (pd - gd->points)+1; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd2 = &gd->points[i];
	off = (pd2->sp->me.x-base->x)*dir->y - (pd2->sp->me.y-base->y)*dir->x;
	if ( off>dist_error || off<-dist_error )
    continue;
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

static struct stem_chunk *AddToStem(struct stemdata *stem,struct pointdata *pd1,struct pointdata *pd2,
	int is_next1, int is_next2, struct pointdata *pd2potential ) {
    struct stem_chunk *chunk;
    BasePoint *dir = &stem->unit;
    BasePoint *test = &pd1->sp->me;
    double off, dist_error;
    int j;
    struct pointdata *pd1potential = pd1;

    dist_error = ( dir->x==0 || dir->y==0 ) ? dist_error_hv : dist_error_diag;	/* Diagonals are harder to align */

    off = (test->x-stem->left.x)*dir->y - (test->y-stem->left.y)*dir->x;
    if ( off>dist_error || off<-dist_error ) {
	struct pointdata *pd = pd1;
	int in = is_next1;
	pd1 = pd2; is_next1 = is_next2;
	pd2 = pd; is_next2 = in;
	pd1potential = pd2potential; pd2potential = NULL;
    }

    chunk = NULL;
    for ( j=stem->chunk_cnt-1; j>=0; --j ) {
	chunk = &stem->chunks[j];
	if ( chunk->l==pd1 && chunk->r==pd2 )
    break;
    }

    if ( j<0 ) {
	stem->chunks = grealloc(stem->chunks,(stem->chunk_cnt+1)*sizeof(struct stem_chunk));
	chunk = &stem->chunks[stem->chunk_cnt++];
	chunk->l = pd1;
	chunk->r = pd2;
	chunk->lpotential = pd1potential;
	chunk->rpotential = pd2potential;
	chunk->lnext = is_next1;
	chunk->rnext = is_next2;
	chunk->stemcheat = false;
    }
    if ( pd1!=NULL ) {
	if ( is_next1 ) {
	    if ( pd1->nextstem == NULL ) {
		pd1->nextstem = stem;
		pd1->next_is_l = true;
#if 0	/* They can be different, if so we need to figure which is better */
		if ( pd1->colinear )
		    pd1->prevstem = stem;
#endif
	    }
	} else {
	    if ( pd1->prevstem == NULL ) {
		pd1->prevstem = stem;
		pd1->prev_is_l = true;
	    }
	}
    }
    if ( pd2!=NULL ) {
	if ( is_next2 ) {
	    if ( pd2->nextstem == NULL ) {
		pd2->nextstem = stem;
		pd2->next_is_l = false;		/* It's r */
	    }
	} else {
	    if ( pd2->prevstem == NULL ) {
		pd2->prevstem = stem;
		pd2->prev_is_l = false;
	    }
	}
    }
return( chunk );
}

static int OnStem(struct stemdata *stem,BasePoint *test) {
    double dist_error, off;
    BasePoint *dir = &stem->unit;

    dist_error = ( dir->x==0 || dir->y==0 ) ? dist_error_hv : dist_error_diag;	/* Diagonals are harder to align */

    off = (test->x-stem->left.x)*dir->y - (test->y-stem->left.y)*dir->x;
    if ( off<dist_error && off>-dist_error )
return( true );

    off = (test->x-stem->right.x)*dir->y - (test->y-stem->right.y)*dir->x;
    if ( off<=dist_error && off>=-dist_error )
return( true );

return( false );
}

static struct stemdata *FindStem(struct glyphdata *gd,struct pointdata *pd,
	BasePoint *dir,SplinePoint *match, int is_next2) {
    int j;
    BasePoint *mdir;
    struct pointdata *pd2;
    struct stemdata *stem;
    double err;

    if ( match->ttfindex==0xffff )
return( NULL );			/* Point doesn't really exist */

    if ( gd->only_hv &&
	    (dir->x<-slope_error || dir->x>slope_error) &&
	    (dir->y<-slope_error || dir->y>slope_error)) {
	/* Not close enough to hor/vert, unless it's a serif */
	if ( pd->sp->next->to!=match && pd->sp->prev->from!=match )
return( NULL );
    }

    pd2 = &gd->points[match->ttfindex];

    mdir = is_next2 ? &pd2->nextunit : &pd2->prevunit;
    if ( mdir->x==0 && mdir->y==0 )
return( NULL );
    err = mdir->x*dir->y - mdir->y*dir->x;
    if (( err<slope_error && err>-slope_error ) ||
	    (( dir->x>.99 || dir->x<-.99 || dir->y>.99 || dir->y<-.99 ) &&
	       (pd->sp->next->to==match || pd->sp->prev->from==match) &&
	       err<3*slope_error && err>-3*slope_error )) {
	    /* Special check for cm serifs */
	stem = is_next2 ? pd2->nextstem : pd2->prevstem;
	if ( stem!=NULL && OnStem(stem,&pd->sp->me))
return( stem );

	for ( j=0; j<gd->stemcnt; ++j ) {
	    err = gd->stems[j].unit.x*dir->y - gd->stems[j].unit.y*dir->x;
	    if ( err<slope_error && err>-slope_error &&
		    OnStem(&gd->stems[j],&pd->sp->me) && OnStem(&gd->stems[j],&pd2->sp->me)) {
return( &gd->stems[j] );
	    }
	}
	if ( dir->y>.99 || dir->y<-.99 ) {
	    for ( j=0; j<gd->stemcnt; ++j ) {
		if ( (gd->stems[j].unit.y>.99 || gd->stems[j].unit.y<-.99) &&
			((match->me.x==gd->stems[j].left.x && pd->sp->me.x==gd->stems[j].right.x) ||
			 (match->me.x==gd->stems[j].right.x && pd->sp->me.x==gd->stems[j].left.x))) {
		    gd->stems[j].unit.x = gd->stems[j].l_to_r.y = 0;
		    gd->stems[j].unit.y = ( gd->stems[j].unit.y*dir->y<0 ) ? -1 : 1;
		    gd->stems[j].l_to_r.x = gd->stems[j].unit.y;
return( &gd->stems[j] );
		}
	    }
	} else if ( dir->x>.99 || dir->x<-.99 ) {
	    for ( j=0; j<gd->stemcnt; ++j ) {
		if (( gd->stems[j].unit.x>.99 || gd->stems[j].unit.x<-.99 ) &&
			((match->me.y==gd->stems[j].left.y && pd->sp->me.y==gd->stems[j].right.y) ||
			 (match->me.y==gd->stems[j].right.y && pd->sp->me.y==gd->stems[j].left.y))) {
		    gd->stems[j].unit.y = gd->stems[j].l_to_r.x = 0;
		    gd->stems[j].unit.x = ( gd->stems[j].unit.x*dir->x<0 ) ? -1 : 1;
		    gd->stems[j].l_to_r.y = gd->stems[j].unit.x;
return( &gd->stems[j] );
		}
	    }
	}
return( (struct stemdata *) (-1) );
    }
return( NULL );
}

static struct stemdata *NewStem(struct glyphdata *gd,BasePoint *dir,
	BasePoint *pos1, BasePoint *pos2) {
    struct stemdata * stem = &gd->stems[gd->stemcnt++];

    stem->unit = *dir;
    if ( dir->x+dir->y<0 ) {
	stem->unit.x = -stem->unit.x;
	stem->unit.y = -stem->unit.y;
    }
    if (( stem->unit.x>stem->unit.y && pos1->y<pos2->y ) ||
	    (stem->unit.y>=stem->unit.x && pos1->x<pos2->x )) {
	stem->left = *pos1;
	stem->right = *pos2;
    } else {
	stem->left = *pos2;
	stem->right = *pos1;
    }
    /* Guess at which normal we want */
    stem->l_to_r.x = dir->y; stem->l_to_r.y = -dir->x;
    /* If we guessed wrong, use the other */
    if ( (stem->right.x-stem->left.x)*stem->l_to_r.x +
	    (stem->right.y-stem->left.y)*stem->l_to_r.y < 0 ) {
	stem->l_to_r.x = -stem->l_to_r.x;
	stem->l_to_r.y = -stem->l_to_r.y;
    }
    stem->chunks = NULL;
    stem->chunk_cnt = 0;
return( stem );
}

static int ParallelToDir(SplinePoint *sp,int checknext, BasePoint *dir,
	BasePoint *opposite,SplinePoint *basesp) {
    BasePoint n, o, *base = &basesp->me;
    double len, err;

    if ( checknext ) {
	if ( sp->next!=NULL && sp->next->knownlinear ) {
	    n.x = sp->next->to->me.x - sp->me.x;
	    n.y = sp->next->to->me.y - sp->me.y;
	} else {
	    n.x = sp->nextcp.x - sp->me.x;
	    n.y = sp->nextcp.y - sp->me.y;
	}
	len = n.x*n.x + n.y*n.y;
	if ( len == 0 )
return( false );
	len = sqrt(len);
	n.x /= len;
	n.y /= len;
    } else {
	if ( sp->prev!=NULL && sp->prev->knownlinear ) {
	    n.x = sp->prev->from->me.x - sp->me.x;
	    n.y = sp->prev->from->me.y - sp->me.y;
	} else {
	    n.x = sp->prevcp.x - sp->me.x;
	    n.y = sp->prevcp.y - sp->me.y;
	}
	len = n.x*n.x + n.y*n.y;
	if ( len == 0 )
return( false );
	len = sqrt(len);
	n.x /= len;
	n.y /= len;
    }
    err = n.x*dir->y - n.y*dir->x;
    if (( dir->x>.99 || dir->x<-.99 || dir->y>.99 || dir->y<-.99 ) &&
	    (sp->next->to==basesp || sp->prev->from==basesp) ) {
	/* special check for serifs */
	if ( err>3*slope_error || err<-3*slope_error )
return( false );
    } else {
	if ( err>slope_error || err<-slope_error )
return( false );
    }

    /* Now sp must be on the same side of the spline as opposite */
    o.x = opposite->x-base->x; o.y = opposite->y-base->y;
    n.x = sp->me.x-base->x; n.y = sp->me.y-base->y;
    if ( ( o.x*dir->y - o.y*dir->x )*( n.x*dir->y - n.y*dir->x ) < 0 )
return( false );
    
return( true );
}

static int NearlyParallel(BasePoint *dir,Spline *other, double t) {
    BasePoint odir;
    double olen, dot;

    odir.x = (3*other->splines[0].a*t+2*other->splines[0].b)*t+other->splines[0].c;
    odir.y = (3*other->splines[1].a*t+2*other->splines[1].b)*t+other->splines[1].c;
    olen = sqrt(odir.x*odir.x + odir.y*odir.y);
    if ( olen==0 )
return( false );
    odir.x /= olen; odir.y /= olen;
    dot = dir->x*odir.x + dir->y*odir.y;
    if ( dot>.95 || dot<-.95 )		/* cos(18) = .95 */
return( true );

return( false );
}

static double NormalDist(BasePoint *to, BasePoint *from, BasePoint *perp) {
    double len = (to->x-from->x)*perp->y - (to->y-from->y)*perp->x;
    if ( len<0 ) len = -len;
return( len );
}

static int NewIsBetter(struct stemdata *stem,struct pointdata *pd,
	struct pointdata *newcomer, double width) {
    double stemwidth;
    int i, cnt;
    double newd, oldd;
    struct pointdata *former;

    stemwidth = (stem->left.x-stem->right.x)*stem->unit.y -
		(stem->left.y-stem->right.y)*stem->unit.x;
    if ( stemwidth<0 ) stemwidth = -stemwidth;

    if ( width>= stemwidth )
return( false );

    for ( i=cnt=0; i<stem->chunk_cnt; ++i ) {
	former = NULL;
	if ( stem->chunks[i].r==pd || stem->chunks[i].rpotential==pd )
	    former = stem->chunks[i].l==NULL ? stem->chunks[i].lpotential : stem->chunks[i].l;
	else if ( stem->chunks[i].l==pd || stem->chunks[i].lpotential==pd )
	    former = stem->chunks[i].r==NULL ? stem->chunks[i].rpotential : stem->chunks[i].r;
	if ( former!=NULL ) {
	    oldd = (former->sp->me.x-pd->sp->me.x)*stem->unit.x +
			(former->sp->me.y-pd->sp->me.y)*stem->unit.y;
	    newd = (newcomer->sp->me.x-pd->sp->me.x)*stem->unit.x +
			(newcomer->sp->me.y-pd->sp->me.y)*stem->unit.y;
	    if ( oldd<0 ) oldd = -oldd;
	    if ( newd<0 ) newd = -newd;
	    if ( newd>oldd ) {
		/* The old stem is wider, but the points are closer */
		/*  if they are much further appart than the improvement we get from making the width smaller... */
		if ( (newd-oldd) > 4*(stemwidth-width) )
return( false );
	    }
	}
	if ( stem->chunks[i].r==pd || stem->chunks[i].l==pd )
	    ++cnt;
    }

    if ( cnt>=2 )
return( false );
return( true );
}

static void MakePDPotential(struct stemdata *stem,struct pointdata *pd) {
    int i;

    for ( i=0; i<stem->chunk_cnt; ++i ) {
	if ( stem->chunks[i].l==pd ) {
	    stem->chunks[i].lpotential = pd;
	    stem->chunks[i].l=NULL;
	}
	if ( stem->chunks[i].r==pd ) {
	    stem->chunks[i].rpotential=pd;
	    stem->chunks[i].r=NULL;
	}
    }
}

static struct stemdata *TestStem(struct glyphdata *gd,struct pointdata *pd,
	BasePoint *dir,SplinePoint *match, int is_next, int is_next2) {
    struct pointdata *pd2, *pd2potential;
    struct stemdata *stem, *stem2 = (struct stemdata *) (-2);
    double width;

    if ( match->ttfindex==0xffff )
return( NULL );
    width = (match->me.x-pd->sp->me.x)*dir->y - (match->me.y-pd->sp->me.y)*dir->x;
    if ( width<0 ) width = -width;
    if ( width<.5 )
return( NULL );		/* Zero width stems aren't interesting */
    if ( (is_next && pd->sp->next->to==match) || (!is_next && pd->sp->prev->from==match) )
return( NULL );		/* Don't want a stem between two splines that intersect */

    stem = FindStem(gd,pd,dir,match,is_next2);
    if ( stem==NULL )
return( NULL );

    pd2 = &gd->points[match->ttfindex];
    stem2 = NULL;
    /* OK, we've got a valid stem between us and pd2. Now see if we get the */
    /*  same stem when we look at pd2. If we do, then all is happy and nice */
    /*  and we can add both points to the stem. If not, then we're sort of */
    /*  happy, but we can only add pd to the stem */
    if ( pd2->nextstem==stem )
	stem2 = stem;
    else if ( pd2->prevstem==stem )
	stem2 = stem;
    else if ( is_next2 && pd2->nextstem!=NULL && NewIsBetter(pd2->nextstem,pd2,pd,width) ) {
	MakePDPotential(pd2->nextstem,pd2);
	pd2->nextstem = NULL;
	stem2 = stem;
    } else if ( !is_next2 && pd2->prevstem!=NULL && NewIsBetter(pd2->prevstem,pd2,pd,width) ) {
	MakePDPotential(pd2->prevstem,pd2);
	pd2->prevstem = NULL;
	stem2 = stem;
    } else if ( (is_next2 && pd2->nextstem==NULL) || (!is_next2 && pd2->prevstem==NULL) ) {
	stem2 = stem;
    } else
	stem2 = NULL;

    pd2potential = pd2;
    if ( stem2==NULL )
	pd2 = NULL;

    if ( stem==(struct stemdata *) (-1) )
	stem = NewStem(gd,dir,&pd->sp->me,&match->me);

    AddToStem(stem,pd,pd2,is_next,is_next2,pd2potential);
return( stem );
}

static double FindSameSlope(Spline *s,BasePoint *dir,double close_to) {
    double a,b,c, desc;
    double t1,t2;
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
    double width, offset, err;
    BasePoint match;
    struct stemdata *stem = NULL;
    int j;

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
    offset = (match.x-pd->sp->me.x)*dir->x + (match.y-pd->sp->me.y)*dir->y;
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

    stem = NULL;
    for ( j=0; j<gd->stemcnt; ++j ) {
	err = gd->stems[j].unit.x*dir->y - gd->stems[j].unit.y*dir->x;
	if ( err<slope_error && err>-slope_error &&
		OnStem(&gd->stems[j],&pd->sp->me) && OnStem(&gd->stems[j],&match)) {
	    stem = &gd->stems[j];
    break;
	}
    }
    if ( stem==NULL )
	stem = NewStem(gd,dir,&pd->sp->me,&match);
    AddToStem(stem,pd,NULL,is_next,false,NULL);
return( stem );
}

static struct stemdata *FindOrMakeHVStem(struct glyphdata *gd,
	struct pointdata *pd, struct pointdata *pd2, int is_h) {
    int i;
    struct stemdata *stem;
    BasePoint dir;

    memset(&dir,0,sizeof(dir));
    if ( is_h ) {
	dir.x = 1;
	for ( i=0; i<gd->stemcnt; ++i ) {
	    stem = &gd->stems[i];
	    if ( stem->unit.x==1 &&
		    ((stem->left.y==pd->sp->me.y && stem->right.y==pd2->sp->me.y) ||
		     (stem->left.y==pd2->sp->me.y && stem->right.y==pd->sp->me.y)))
	break;
	}
	if ( i==gd->stemcnt ) stem=NULL;
    } else {
	dir.y = 1;
	for ( i=0; i<gd->stemcnt; ++i ) {
	    stem = &gd->stems[i];
	    if ( stem->unit.y==1 &&
		    ((stem->left.x==pd->sp->me.x && stem->right.x==pd2->sp->me.x) ||
		     (stem->left.x==pd2->sp->me.x && stem->right.x==pd->sp->me.x)))
	break;
	}
	if ( i==gd->stemcnt ) stem=NULL;
    }
    if ( stem==NULL )
	stem = NewStem(gd,&dir,&pd->sp->me,&pd2->sp->me);
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

    if ( !(hintcontrolflags&hint_diagonal_ends) )
return( NULL );

    if ( !gd->only_hv ||
	    pd->sp->next==NULL || pd->sp->next->to->next==NULL )
return( NULL );
    pd2 = &gd->points[pd->sp->next->to->ttfindex];
    if ( (pd->sp->me.x!=pd2->sp->me.x && pd->sp->me.y!=pd2->sp->me.y ) ||
	    pd->sp->prev==NULL || !pd->sp->prev->knownlinear ||
	    pd2->sp->next==NULL || !pd2->sp->next->knownlinear )
return( NULL );
    if ( !UnitsParallel(&pd2->nextunit,&pd->prevunit ) )
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

    stem = FindOrMakeHVStem(gd,pd,pd2,pd->sp->me.x==pd2->sp->me.x);
    chunk = AddToStem(stem,pd,pd2,false,true,NULL);
    chunk->stemcheat = true;
return( stem );
}

static int ConnectsAcross(struct glyphdata *gd,SplinePoint *sp,int is_next,Spline *findme) {
    struct pointdata *pd = &gd->points[sp->ttfindex];
    Spline *other, *test;
    BasePoint *dir;

    if ( is_next )
	other = pd->nextedge;
    else
	other = pd->prevedge;

    if ( other==findme )
return( true );
    if ( other==NULL )
return( false );

    dir = &gd->points[other->to->ttfindex].nextunit;
    test = other->to->next;
    while ( test!=NULL &&
	    gd->points[test->from->ttfindex].nextunit.x * dir->x +
	    gd->points[test->from->ttfindex].nextunit.y * dir->y >= 0 ) {
	if ( test==findme )
return( true );
	test = test->to->next;
    }
	    
    dir = &gd->points[other->from->ttfindex].prevunit;
    test = other->from->prev;
    while ( test!=NULL &&
	    gd->points[test->to->ttfindex].prevunit.x * dir->x +
	    gd->points[test->to->ttfindex].prevunit.y * dir->y >= 0 ) {
	if ( test==findme )
return( true );
	test = test->from->prev;
    }

return( false );
}

static void BuildStem(struct glyphdata *gd,struct pointdata *pd,int is_next ) {
    BasePoint *dir;
    Spline *other, *cur;
    double t;
    double tod, fromd;
    struct stemdata *stem;
    int tp, fp;
    BasePoint opposite;

    if ( is_next ) {
	dir = &pd->nextunit;
	other = pd->nextedge;
	cur = pd->sp->next;
	t = pd->next_e_t;
    } else {
	dir = &pd->prevunit;
	other = pd->prevedge;
	cur = pd->sp->prev;
	t = pd->prev_e_t;
    }
    if ( other==NULL )
return;

    opposite.x = ((other->splines[0].a*t+other->splines[0].b)*t+other->splines[0].c)*t+other->splines[0].d;
    opposite.y = ((other->splines[1].a*t+other->splines[1].b)*t+other->splines[1].c)*t+other->splines[1].d;

    tp = ParallelToDir(other->to,false,dir,&opposite,pd->sp);
    fp = ParallelToDir(other->from,true,dir,&opposite,pd->sp);
    stem = NULL;
    if ( is_next ) 
	stem = EdgeStem(gd,pd);
    if ( !tp && !fp )
return;

    /* We have several conflicting metrics for getting the "better" stem */
    /* Generally we prefer the stem with the smaller width (but not always. See tilde) */
    /* Generally we prefer the stem formed by the point closer to the intersection */
    tod = (1-t)*NormalDist(&other->to->me,&pd->sp->me,dir);
    fromd = t*NormalDist(&other->from->me,&pd->sp->me,dir);
    stem = NULL;
    if ( (tp && (tod<fromd)) ||
	    (tp && !fp && (tod<2*fromd)) ||
	    (tp && !fp && (ConnectsAcross(gd,other->to,false,cur) ||
			    NearlyParallel(dir,other,t))))
	stem = TestStem(gd,pd,dir, other->to, is_next, false);
    if ( stem==NULL && ((fp && (fromd<tod)) ||
	    (fp && !tp && (fromd<2*tod)) ||
	    (fp && !tp && (ConnectsAcross(gd,other->from,true,cur) ||
			    NearlyParallel(dir,other,t)))))
	stem = TestStem(gd,pd,dir, other->from, is_next,true);
    if ( stem==NULL && cur!=NULL && !other->knownlinear && !cur->knownlinear )
	stem = HalfStem(gd,pd,dir,other,t,is_next);
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
    chunk = AddToStem(stem,pd,pd2,false,true,NULL);
    if ( pd3==NULL )
	chunk->stemcheat = 2;
    else {
	chunk->stemcheat = 3;
	chunk = AddToStem(stem,pd,pd3,false,true,NULL);
	chunk->stemcheat = 3;
    }
}

static int swidth_cmp(const void *_p1, const void *_p2) {
    const struct stemdata * const *stpt1 = _p1, * const *stpt2 = _p2;
    const struct stemdata *s1 = *stpt1, *s2 = *stpt2;
    if ( s1->width>s2->width )
return( 1 );
    else if ( s1->width<s2->width )
return( -1 );

return( 0 );
}

static int IsL(BasePoint *bp,struct stemdata *stem) {
    double offl, offr;

    offl = (bp->x-stem->left.x)*stem->l_to_r.x + (bp->y-stem->left.y)*stem->l_to_r.y;
    if ( offl<0 ) offl = -offl;
    offr = (bp->x-stem->right.x)*stem->l_to_r.x + (bp->y-stem->right.y)*stem->l_to_r.y;
    if ( offr<0 ) offr = -offr;
return( offl < offr );
}

static struct pointdata *PotentialAdd(struct glyphdata *gd,struct stemdata *stem,
	struct pointdata *pd, int is_next ) {

    if ( is_next && (pd->nextstem==NULL || pd->nextstem->toobig )) {
	pd->nextstem = stem;
	pd->next_is_l = IsL(&pd->sp->me,stem);
return( pd );
    } else if ( !is_next && (pd->prevstem==NULL || pd->prevstem->toobig )) {
	pd->prevstem = stem;
	pd->prev_is_l = IsL(&pd->sp->me,stem);
return( pd );
    }
return( NULL );
}

static int IsClosestPotential(struct stemdata **sspace,int cur,int goodcnt,
	int is_l,struct stem_chunk *chunk) {
    struct pointdata *search;
    double mydist, dist;
    SplinePoint *sp1, *sp2;
    int i,j;

    if ( is_l ) {
	if ( chunk->r==NULL )
return( false );
	search = chunk->lpotential;
	sp1 = search->sp; sp2 = chunk->r->sp;
	mydist = (sp1->me.x-sp2->me.x)*(sp1->me.x-sp2->me.x) +
		(sp1->me.y-sp2->me.y)*(sp1->me.y-sp2->me.y);
    } else {
	if ( chunk->l==NULL )
return( false );
	search = chunk->rpotential;
	sp1 = search->sp; sp2 = chunk->l->sp;
	mydist = (sp1->me.x-sp2->me.x)*(sp1->me.x-sp2->me.x) +
		(sp1->me.y-sp2->me.y)*(sp1->me.y-sp2->me.y);
    }
    for ( i=cur+1; i<goodcnt; ++i ) {
	struct stemdata *stem = sspace[i];
	for ( j=0; j<stem->chunk_cnt; ++j ) {
	    struct stem_chunk *chunk = &stem->chunks[j];
	    if ( is_l && chunk->r!=NULL && chunk->l==NULL && chunk->lpotential==search ) {
		sp2 = chunk->r->sp;
		dist = (sp1->me.x-sp2->me.x)*(sp1->me.x-sp2->me.x) +
			(sp1->me.y-sp2->me.y)*(sp1->me.y-sp2->me.y);
		if ( dist<mydist )
return( false );
	    } else if ( !is_l && chunk->l!=NULL && chunk->r==NULL && chunk->rpotential==search ) {
		sp2 = chunk->l->sp;
		dist = (sp1->me.x-sp2->me.x)*(sp1->me.x-sp2->me.x) +
			(sp1->me.y-sp2->me.y)*(sp1->me.y-sp2->me.y);
		if ( dist<mydist )
return( false );
	    }
	}
    }
return( true );
}

int UnitsParallel(BasePoint *u1,BasePoint *u2) {
    double dot = u1->x*u2->x + u1->y*u2->y;
return( dot>.95 || dot < -.95 );
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
    len = sqrt( diff.x*diff.x + diff.y*diff.y );
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

static int StemIsActiveAt(struct glyphdata *gd,struct stemdata *stem,double stempos ) {
    BasePoint pos;
    int which;
    double test;
    struct monotonic **space, *m;
    int winding, nw, i,j;

    pos.x = stem->left.x + stempos*stem->unit.x;
    pos.y = stem->left.y + stempos*stem->unit.y;

    if ( stem->unit.x==0 || stem->unit.y==0 ) {
	which = stem->unit.x==0;
	MonotonicFindAt(gd->ms,which,((real *) &pos.x)[which],space = gd->space);
	test = ((real *) &pos.x)[!which] - dist_error_hv;

	winding = 0;
	for ( i=0; space[i]!=NULL; ++i ) {
	    m = space[i];
	    nw = ((&m->xup)[which] ? 1 : -1 );
	    if ( m->other>=test )
	break;
	    winding += nw;
	}
	if ( space[i]==NULL || m->other>test+2*dist_error_hv )
return( false );
	j = MatchWinding(space,i,nw,winding,which);
	if ( j==-1 )
return( false );
	m = space[j];
	if ( m->other>=((real *) &stem->right.x)[!which]-dist_error_hv &&
		 m->other<=((real *) &stem->right.x)[!which]+dist_error_hv )
return( true );
return( false );
    } else {
	Spline myline;
	SplinePoint end1, end2;
	int cnt;
	MakeVirtualLine(gd,&pos,&stem->unit,&myline,&end1,&end2);
	cnt = MonotonicOrder(gd->sspace,&myline,gd->stspace);
return( MonotonicFindStemBounds(&myline,gd->stspace,cnt,stem));
    }
}

static int WalkSpline(SplinePoint *sp,int gonext,struct stemdata *stem,BasePoint *res) {
    int i;
    double off,diff;
    double incr;
    double t=.5;
    Spline *s;
    BasePoint pos, good;
    /* Some splines have tiny control points and are almost flat */
    /*  think of them as lines then rather than treating them as curves */
    /*  figure out how long they remain within a few orthoganal units of */
    /*  the point */

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
	diff = (pos.x-sp->me.x)*stem->l_to_r.x +
		(pos.y-sp->me.y)*stem->l_to_r.y;
	if ( diff>=-dist_error_hv && diff<dist_error_hv ) {
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
    }
return( true );		/* Treat as curved */
}

static BasePoint *AdjustForImperfectSlopeMatch(SplinePoint *sp,BasePoint *pos,
	struct stemdata *stem,BasePoint *temp) {
    double off;

    off = (pos->x-sp->me.x)*stem->l_to_r.x +
	    (pos->y-sp->me.y)*stem->l_to_r.y;
    if ( off<0 ) off=-off;
    if ( off<=dist_error_hv )
return( pos );

    temp->x = sp->me.x + dist_error_hv*(pos->x-sp->me.x)/off;
    temp->y = sp->me.y + dist_error_hv*(pos->y-sp->me.y)/off;
return( temp );
}

static int AddLineSegment(struct stemdata *stem, struct segment *space, int cnt,
	struct pointdata *pd, struct glyphdata *gd ) {
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
    if ( UnitsParallel(&pd->nextunit,&stem->unit)) {
	if ( sp->next->knownlinear ) {
	    nsp = sp->next->to;
	    end = &nsp->me;
	    if ( nsp->next!=NULL && !nsp->next->knownlinear && !nsp->ticked &&
		    UnitsParallel(&gd->points[nsp->ttfindex].nextunit,&stem->unit)) {
		WalkSpline(nsp,true,stem,&etemp);
		end = &etemp;
		nsp->ticked = true;
	    }
	} else {
	    WalkSpline(sp,true,stem,&etemp);
	    ecurved = true;
	    end = &etemp;
	}
    } else {
	if ( !WalkSpline(sp,true,stem,&etemp))
	    end = &etemp;
    }
    if ( UnitsParallel(&pd->prevunit,&stem->unit)) {
	if ( sp->prev->knownlinear ) {
	    psp = sp->prev->from;
	    start = &psp->me;
	    if ( psp->prev!=NULL && !psp->prev->knownlinear && !psp->ticked &&
		    UnitsParallel(&gd->points[psp->ttfindex].prevunit,&stem->unit)) {
		WalkSpline(psp,false,stem,&stemp);
		start = &stemp;
		psp->ticked = true;
	    }
	} else {
	    WalkSpline(sp,false,stem,&stemp);
	    scurved = true;
	    start = &stemp;
	}
    } else {
	if ( !WalkSpline(sp,false,stem,&stemp))
	    start = &stemp;
    }

    sp->ticked = true;

    /* Now if the segment sp-start doesn't have exactly the right slope, then */
    /*  we can only use that bit of it which is less than a standard error */
    start = AdjustForImperfectSlopeMatch(sp,start,stem,&stemp2);
    end   = AdjustForImperfectSlopeMatch(sp,end,stem,&etemp2);

    s = (start->x-stem->left.x)*stem->unit.x +
	(start->y-stem->left.y)*stem->unit.y;
    e = (  end->x-stem->left.x)*stem->unit.x +
	(  end->y-stem->left.y)*stem->unit.y;
    b = 0;
    if ( scurved || ecurved ) {
	b = (sp->me.x-stem->left.x)*stem->unit.x +
		   (sp->me.y-stem->left.y)*stem->unit.y;
	if ( scurved )
	    s = b + (s-b)*1.3;
	if ( ecurved )
	    e = b + (e-b)*1.3;
    }
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
    space[cnt].end   = e;
    space[cnt].base  = b;
    space[cnt].curved= scurved | (ecurved?2:0);
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
	if ( projection<=segments[i].end )
return( projection>=segments[i].start );
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
		space[j].end = space[i+1].end;
		space[j].curved = (space[j].curved&1) | (space[i+1].curved&2);
	    }
	    ++i;
	}
    }
return( j );
}

static void FigureStemActive(struct glyphdata *gd, struct stemdata *stem) {
    int i, j, pcnt=0;
    struct pointdata *pd, **pspace = gd->pspace;
    struct segment *lspace = gd->lspace, *rspace = gd->rspace;
    struct segment *bothspace = gd->bothspace, *activespace = gd->activespace;
    int lcnt, rcnt, bcnt, bpos, acnt;
    double width, last, len, clen;

    width = (stem->left.x-stem->right.x)*stem->unit.y -
	    (stem->left.y-stem->right.y)*stem->unit.x;
    stem->width=width;

    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL )
	gd->points[i].sp->ticked = false;

    lcnt = rcnt = 0;
    for ( i=0; i<stem->chunk_cnt; ++i ) {
	lcnt = AddLineSegment(stem,lspace,lcnt,stem->chunks[i].l, gd );
	if ( stem->chunks[i].lpotential!=stem->chunks[i].l )
	    lcnt = AddLineSegment(stem,lspace,lcnt,stem->chunks[i].lpotential, gd );
	rcnt = AddLineSegment(stem,rspace,rcnt,stem->chunks[i].r, gd );
	if ( stem->chunks[i].rpotential!=stem->chunks[i].r )
	    rcnt = AddLineSegment(stem,rspace,rcnt,stem->chunks[i].rpotential, gd );
    }
    bcnt = 0;
    if ( lcnt!=0 && rcnt!=0 ) {
	if ( lcnt==1 && rcnt==1 && lspace[0].curved && rspace[0].curved ) {
	    double gap;
	    if ( lspace[0].start>rspace[0].end )
		gap = lspace[0].start-rspace[0].end;
	    else if ( rspace[0].start>lspace[0].end )
		gap = rspace[0].start-lspace[0].end;
	    else
		gap = 0;
	    if ( gap < (lspace[0].end-lspace[0].start + rspace[0].end-rspace[0].start)/2 ) {
		if ( lspace[0].base<rspace[0].start )
		    rspace[0].start = lspace[0].base;
		else if ( lspace[0].base>rspace[0].end )
		    rspace[0].end = lspace[0].base;
		if ( rspace[0].base<lspace[0].start )
		    lspace[0].start = rspace[0].base;
		else if ( rspace[0].base>lspace[0].end )
		    lspace[0].end = rspace[0].base;
	    }
	}
	qsort(lspace,lcnt,sizeof(struct segment),segment_cmp);
	qsort(rspace,rcnt,sizeof(struct segment),segment_cmp);
	lcnt = MergeSegments(lspace,lcnt);
	rcnt = MergeSegments(rspace,rcnt);
	for ( i=j=bcnt=0; i<lcnt && j<rcnt; ++i ) {
	    while ( j<rcnt && rspace[j].end<=lspace[i].start )
		++j;
	    while ( j<rcnt && rspace[j].start<=lspace[i].end ) {
		double s = lspace[i].start;
		double e = lspace[i].end;
		if ( rspace[j].start>s )
		    s = rspace[j].start;
		if ( rspace[j].end<e )
		    e = rspace[j].end;
		bothspace[bcnt  ].curved= rspace[j].curved || lspace[i].curved;
		bothspace[bcnt  ].start = s;
		bothspace[bcnt++].end   = e;
		if ( rspace[j].end>lspace[i].end )
	    break;
		++j;
	    }
	}
    }

    acnt = 0;
    if ( bcnt!=0 ) {
	for ( i=0; i<gd->pcnt; ++i ) if ( (pd = &gd->points[i])->sp!=NULL ) {
	    /* Let's say we have a stem. And then inside that stem we have */
	    /*  another rectangle. So our first stem isn't really a stem any */
	    /*  more (because we hit another edge first), yet it's still reasonable*/
	    /*  to align the original stem */
	    /* Now suppose the rectangle is rotated a bit so we can't make */
	    /*  a stem from it. What do we do here? */
	    double inverseproj = (pd->sp->me.x-stem->right.x)*stem->unit.y -
				 (pd->sp->me.y-stem->right.y)*stem->unit.x;
	    if ( (width>0 && inverseproj>=0 && inverseproj<=width) ||
		    (width<0 && inverseproj<=0 && inverseproj>=-width)) {
		pd->projection = (pd->sp->me.x - stem->left.x)*stem->unit.x +
				 (pd->sp->me.y - stem->left.y)*stem->unit.y;
		if ( InActive(pd->projection,bothspace,bcnt) )
		    pspace[pcnt++] = pd;
	    }
	}
	qsort(pspace,pcnt,sizeof(struct pointdata *),proj_cmp);

	bpos = i = 0;
	while ( bpos<bcnt ) {
	    if ( bothspace[bpos].curved ) {
		activespace[acnt++] = bothspace[bpos++];
	    } else {
		last = bothspace[bpos].start;
		while ( i<pcnt && pspace[i]->projection<bothspace[bpos].end ) {
		    if ( last<pspace[i]->projection ) { 
			if ( StemIsActiveAt(gd,stem,(1.001*last+pspace[i]->projection)/2.001) ) {
			    if ( acnt==0 || activespace[acnt-1].end!=last ) {
				activespace[acnt  ].curved = bothspace[bpos].curved;
				activespace[acnt++].start  = last;
			    }
			    activespace[acnt-1].end = pspace[i]->projection;
			}
			last = pspace[i]->projection;
		    }
		    ++i;
		}
		if ( StemIsActiveAt(gd,stem,(1.001*last+bothspace[bpos].end)/2.001) ) {
		    if ( acnt==0 || activespace[acnt-1].end!=last ) {
			activespace[acnt  ].curved = bothspace[bpos].curved;
			activespace[acnt++].start = last;
		    }
		    activespace[acnt-1].end = bothspace[bpos].end;
		}
		++bpos;
	    }
	}
    }

    for ( i=0; i<stem->chunk_cnt; ++i ) {
	struct stem_chunk *chunk = &stem->chunks[i];
	if ( chunk->stemcheat==3 && chunk->l!=NULL && chunk->r!=NULL &&
		i+1<stem->chunk_cnt &&
		stem->chunks[i+1].stemcheat==3 && chunk->l==stem->chunks[i+1].l ) {
	    SplinePoint *sp = chunk->l->sp;
	    double proj = (sp->me.x - stem->left.x) *stem->unit.x +
			  (sp->me.y - stem->left.y) *stem->unit.y;
	    SplinePoint *sp2 = chunk->r->sp, *sp3 = stem->chunks[i+1].r->sp;
	    double proj2 = (sp2->me.x - stem->left.x) *stem->unit.x +
			   (sp2->me.y - stem->left.y) *stem->unit.y;
	    double proj3 = (sp3->me.x - stem->left.x) *stem->unit.x +
			   (sp3->me.y - stem->left.y) *stem->unit.y;
	    activespace[acnt  ].curved = false;
	    if ( proj2<proj3 ) {
		activespace[acnt  ].start = proj2;
		activespace[acnt  ].end = proj3;
	    } else {
		activespace[acnt  ].start = proj3;
		activespace[acnt  ].end = proj2;
	    }
	    activespace[acnt++].base = proj;
	    ++i;
	} else if ( chunk->stemcheat && chunk->l!=NULL && chunk->r!=NULL ) {
	    SplinePoint *sp = chunk->l->sp;
	    double proj = (sp->me.x - stem->left.x) *stem->unit.x +
			  (sp->me.y - stem->left.y) *stem->unit.y, orig_proj=proj;
	    SplinePoint *other = chunk->lnext ? sp->next->to : sp->prev->from;
	    double len = (other->me.x-sp->me.x) * stem->unit.x +
			 (other->me.y-sp->me.y) * stem->unit.y;
	    double width = (stem->width<0) ? -stem->width : stem->width;
	    if ( chunk->stemcheat==2 )
		proj -= width/2;
	    else if ( len<0 )
		proj -= width;
	    activespace[acnt  ].curved = true;
	    activespace[acnt  ].start = proj;
	    activespace[acnt  ].end = proj+width;
	    activespace[acnt++].base = orig_proj;
	}
    }

    stem->activecnt = acnt;
    if ( acnt!=0 ) {
	stem->active = galloc(acnt*sizeof(struct segment));
	memcpy(stem->active,activespace,acnt*sizeof(struct segment));
    }
    if ( stem->width<0 )
	stem->width = -stem->width;

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
    int i;
    struct pointdata *pd;
    double width;

    for ( i=0; i<gd->stemcnt; ++i ) {
	struct stemdata *stem = &gd->stems[i];

	width = (stem->left.x-stem->right.x)*stem->unit.y -
		(stem->left.y-stem->right.y)*stem->unit.x;
	if ( width<0 ) width = -width;
	stem->width = width;
    }

    for ( i=0; i<gd->pcnt; ++i ) if ( (pd = &gd->points[i])->sp!=NULL && pd->colinear ) {
	if ( pd->nextstem!=NULL && pd->prevstem!=NULL &&
		pd->nextstem != pd->prevstem ) {
	    if ( pd->nextstem->width<pd->prevstem->width ) {
		MakePDPotential(pd->prevstem,pd);
		pd->prevstem = pd->nextstem;
	    } else {
		MakePDPotential(pd->nextstem,pd);
		pd->nextstem = pd->prevstem;
	    }
	}
    }
}

static void GDFindUnlikelyStems(struct glyphdata *gd) {
    double width;
    int i;
    double em = (gd->sc->parent->ascent+gd->sc->parent->descent);

    GDStemsFixupIntersects(gd);

    GDFindMismatchedParallelStems(gd);

    /* If a stem has straight edges, and it is wider than tall */
    /*  then it is unlikely to be a real stem */
    for ( i=0; i<gd->stemcnt; ++i ) {
	struct stemdata *stem = &gd->stems[i];

	width = stem->width;
	stem->toobig =  (               width<=em/8 && 2.0*stem->clen < width ) ||
			( width>em/8 && width<=em/4 && 1.5*stem->clen < width ) ||
			( width>em/4 &&                    stem->clen < width );
    }
}

static void AddOpposite(struct glyphdata *gd,struct stem_chunk *chunk,
	struct stemdata *stem) {
    Spline *s;
    double t;
    struct pointdata **other;

    if ( chunk->l!=NULL ) {
	if ( chunk->l->nextstem == stem ) {
	    s = chunk->l->nextedge;
	    t = chunk->l->next_e_t;
	} else {
	    s = chunk->l->prevedge;
	    t = chunk->l->prev_e_t;
	}
	other = &chunk->r;
    } else {
	if ( chunk->r->nextstem == stem ) {
	    s = chunk->r->nextedge;
	    t = chunk->r->next_e_t;
	} else {
	    s = chunk->r->prevedge;
	    t = chunk->r->prev_e_t;
	}
	other = &chunk->l;
    }
    if ( s!=NULL && s->knownlinear ) {
	struct pointdata *fpd = &gd->points[s->from->ttfindex];
	struct pointdata *tpd = &gd->points[s->to->ttfindex];
	if ( fpd->nextstem==NULL && OnStem(stem,&fpd->sp->me)) {
	    fpd->nextstem = stem;
	    fpd->next_is_l = chunk->l==NULL;
	    if ( t<.5 )
		*other = fpd;
	}
	if ( tpd->prevstem == NULL && OnStem(stem,&tpd->sp->me)) {
	    tpd->prevstem = stem;
	    tpd->prev_is_l = chunk->l==NULL;
	    if ( t>.5 )
		*other = tpd;
	}
    }
}    

struct glyphdata *GlyphDataBuild(SplineChar *sc, int only_hv) {
    struct glyphdata *gd;
    int i,j;
    SplineSet *ss;
    SplinePoint *sp;
    Monotonic *m;
    int cnt, goodcnt;
    struct stemdata **sspace;
    int em = sc->parent!=NULL ? sc->parent->ascent+sc->parent->descent : 1000;

    /* We can't hint type3 fonts, so the only layer we care about is ly_fore */
    /* We shan't try to hint references yet */
    if ( sc->layers[ly_fore].splines==NULL )
return( NULL );

    dist_error_hv = .003*em;
    dist_error_diag = .006*em;

    gd = gcalloc(1,sizeof(struct glyphdata));
    gd->only_hv = only_hv;

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
	    pd->nextedge = FindMatchingEdge(gd,pd,true,only_hv);
	if ( !pd->prevzero )
	    pd->prevedge = FindMatchingEdge(gd,pd,false,only_hv);
	if ( only_hv && (pd->symetrical_h || pd->symetrical_v ))
	    pd->bothedge = FindMatchingEdge(gd,pd,2,only_hv);
	
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

    /* There will never be more lines than there are points (counting next/prev as separate) */
    gd->lines = galloc(2*gd->pcnt*sizeof(struct linedata));
    gd->linecnt = 0;
    gd->pspace = galloc(gd->pcnt*sizeof(struct pointdata *));
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	struct pointdata *pd = &gd->points[i];
	if (( !only_hv || pd->next_hor || pd->next_ver ) && pd->nextline==NULL ) {
	    pd->nextline = BuildLine(gd,pd,true);
	    if ( pd->colinear )
		pd->prevline = pd->nextline;
	}
	if (( !only_hv || pd->prev_hor || pd->prev_ver ) && pd->prevline==NULL ) {
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
	if ( pd->nextedge!=NULL /*&& pd->nextstem==NULL*/ )
	    BuildStem(gd,pd,true);
	if ( pd->prevedge!=NULL /*&& pd->prevstem==NULL*/ )
	    BuildStem(gd,pd,false);
	if ( pd->bothedge!=NULL )
	    DiagonalCornerStem(gd,pd);
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

    sspace = galloc(gd->stemcnt*sizeof(struct stemdata *));
    for ( i=j=0; i<gd->stemcnt; ++i ) /*if ( !gd->stems[i].toobig )*/
	sspace[j++] = &gd->stems[i];
    goodcnt = j;
    qsort(sspace,goodcnt,sizeof(struct stemdata *),swidth_cmp);

    /* we were cautious about assigning points to stems, go back now and see */
    /*  if there are any low-quality matches which remain unassigned, and if */
    /*  so then assign them to the stem they almost fit on. */
    /* If there are multiple stems, find the one which is closest to this point */
    for ( i=0; i<goodcnt; ++i ) {
	struct stemdata *stem = sspace[i];
	for ( j=0; j<stem->chunk_cnt; ++j ) {
	    struct stem_chunk *chunk = &stem->chunks[j];
	    if ( chunk->l==NULL && chunk->lpotential!=NULL &&
		    IsClosestPotential(sspace,i,goodcnt,true,chunk))
		chunk->l = PotentialAdd(gd,stem,chunk->lpotential,chunk->lnext);
	    if ( chunk->r==NULL && chunk->rpotential!=NULL &&
		    IsClosestPotential(sspace,i,goodcnt,false,chunk))
		chunk->r = PotentialAdd(gd,stem,chunk->rpotential,chunk->rnext);
	}
    }
    for ( i=0; i<goodcnt; ++i ) {
	struct stemdata *stem = sspace[i];
	for ( j=0; j<stem->chunk_cnt; ++j ) {
	    struct stem_chunk *chunk = &stem->chunks[j];
	    if (( chunk->l!=NULL && chunk->r==NULL ) ||
		    ( chunk->l==NULL && chunk->r!=NULL ))
		AddOpposite(gd,chunk,stem);
	}
    }
    free(sspace);

    gd->lspace = galloc(gd->pcnt*sizeof(struct segment));
    gd->rspace = galloc(gd->pcnt*sizeof(struct segment));
    gd->bothspace = galloc(3*gd->pcnt*sizeof(struct segment));
    gd->activespace = galloc(3*gd->pcnt*sizeof(struct segment));
    for ( i=j=0; i<gd->stemcnt; ++i )
	FigureStemActive(gd,&gd->stems[i]);

    FreeMonotonics(gd->ms);	gd->ms = NULL;
    free(gd->space);		gd->space = NULL;
    free(gd->sspace);		gd->sspace = NULL;
    free(gd->stspace);		gd->stspace = NULL;
    free(gd->pspace);		gd->pspace = NULL;
    free(gd->lspace);		gd->lspace = NULL;
    free(gd->rspace);		gd->rspace = NULL;
    free(gd->bothspace);	gd->bothspace = NULL;
    free(gd->activespace);	gd->activespace = NULL;

    GDFindUnlikelyStems(gd);

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
