/* Copyright (C) 2005-2012 by George Williams and Alexey Kryukov */
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
#include "fontforge.h"
#include "edgelist2.h"
#include "stemdb.h"

#include <math.h>
#include <utype.h>

#define GLYPH_DATA_DEBUG 0
#define PI 3.14159265358979323846264338327

/* A diagonal end is like the top or bottom of a slash. Should we add a vertical stem at the end? */
/* A diagonal corner is like the bottom of circumflex. Should we add a horizontal stem? */
int     hint_diagonal_ends = 0,
	hint_diagonal_intersections = 0,
	hint_bounding_boxes = 1,
	detect_diagonal_stems = 0;

float   stem_slope_error = .05061454830783555773, /*  2.9 degrees */
	stub_slope_error = .317649923862967983;   /* 18.2 degrees */

static double dist_error_hv = 3.5;
static double dist_error_diag = 5.5;
/* It's easy to get horizontal/vertical lines aligned properly */
/* it is more difficult to get diagonal ones done */
/* The "A" glyph in Apple's Times.dfont(Roman) is off by 6 in one spot */
static double dist_error_curve = 22;
/* The maximum possible distance between the edge of an active zone for */
/* a curved spline segment and the spline itself */

struct st {
    Spline *s;
    double st, lt;
};

static int GetBlueFuzz(SplineFont *sf) {
    char *str, *end;

    if ( sf == NULL || sf->private == NULL || 
	(str=PSDictHasEntry( sf->private,"BlueFuzz" )) == NULL || !isdigit( str[0] ))
return 1;
return strtod( str, &end );
}

static int IsUnitHV( BasePoint *unit,int strict ) {
    double angle = atan2( unit->y,unit->x );
    double deviation = ( strict ) ? stem_slope_error : stub_slope_error;
    
    if ( fabs( angle ) >= PI/2 - deviation && fabs( angle ) <= PI/2 + deviation )
return( 2 );
    else if ( fabs( angle ) <= deviation || fabs( angle ) >= PI - deviation )
return( 1 );

return( 0 );
}

static int UnitCloserToHV( BasePoint *u1,BasePoint *u2 ) {
    double adiff1, adiff2;
    
    adiff1 = fabs( atan2( u1->y,u1->x ));
    adiff2 = fabs( atan2( u2->y,u2->x ));
    
    if ( adiff1 > PI*.25 && adiff1 < PI*.75 )
	adiff1 = fabs( adiff1 - PI*.5 );
    else if ( adiff1 >= PI*.75 )
	adiff1 = PI - adiff1;

    if ( adiff2 > PI*.25 && adiff2 < PI*.75 )
	adiff2 = fabs( adiff2 - PI*.5 );
    else if ( adiff2 >= PI*.75 )
	adiff2 = PI - adiff2;

    if ( adiff1 < adiff2 )
return( 1 );
    else if ( adiff1 > adiff2 )
return( -1 );
    else
return( 0 );
}

static double GetUnitAngle( BasePoint *u1,BasePoint *u2 ) {
    double dx, dy;

    dy = u1->x*u2->y - u1->y*u2->x;
    dx = u1->x*u2->x + u1->y*u2->y;
return( atan2( dy,dx ));
}

static int UnitsOrthogonal( BasePoint *u1,BasePoint *u2,int strict ) {
    double angle, deviation = ( strict ) ? stem_slope_error : stub_slope_error;
    
    angle = GetUnitAngle( u1,u2 );
    
return( fabs( angle ) >= PI/2 - deviation && fabs( angle ) <= PI/2 + deviation );
}

int UnitsParallel( BasePoint *u1,BasePoint *u2,int strict ) {
    double angle, deviation = ( strict ) ? stem_slope_error : stub_slope_error;
    
    angle = GetUnitAngle( u1,u2 );
    
return( fabs( angle ) <= deviation || fabs( angle ) >= PI - deviation );
}

static int IsInflectionPoint( struct glyphdata *gd,struct pointdata *pd ) {
    SplinePoint *sp = pd->sp;
    double CURVATURE_THRESHOLD = 1e-9;
    struct spline *prev, *next;
    double in, out;

    if ( sp->prev == NULL || sp->next == NULL || !pd->colinear )
return( false );

    /* point of a single-point contour can't be an inflection point. */
    if ( sp->prev->from == sp )
return( false );

    prev = sp->prev;
    in = 0;
    while ( prev != NULL && fabs(in) < CURVATURE_THRESHOLD ) {
	in = SplineCurvature( prev,1 );
	if ( fabs( in ) < CURVATURE_THRESHOLD ) in = SplineCurvature( prev, 0 );
	if ( fabs( in ) < CURVATURE_THRESHOLD ) prev = prev->from->prev;
	if ( gd->points[prev->to->ptindex].colinear )
    break;
    }

    next = sp->next;
    out = 0;
    while ( next != NULL && fabs( out ) < CURVATURE_THRESHOLD ) {
	out = SplineCurvature( next,0 );
	if ( fabs( out ) < CURVATURE_THRESHOLD ) out = SplineCurvature( next, 1 );
	if ( fabs( out ) < CURVATURE_THRESHOLD ) next = next->to->next;
	if ( gd->points[next->from->ptindex].colinear )
    break;
    }

    if ( in==0 || out==0 || ( prev != sp->prev && next != sp->next ))
return( false );

    in/=fabs(in);
    out/=fabs(out);

return ( in*out < 0 );
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
    double dx, dy, ax, ay, d, a;

    /* The vector is already nearly vertical/horizontal, no need to modify*/
    if ( IsUnitHV( dir,true ))
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

    if ( UnitsParallel( dir,&normal,true )) {
	/* prefer the direction which is closer to horizontal/vertical */
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

static int IsSplinePeak( struct glyphdata *gd,struct pointdata *pd,int outer,int is_x,int flags );

static void PointInit( struct glyphdata *gd,SplinePoint *sp, SplineSet *ss ) {
    struct pointdata *pd, *prevpd=NULL, *nextpd=NULL;
    double len, same;
    int hv;

    if ( sp->ptindex >= gd->pcnt )
return;
    pd = &gd->points[sp->ptindex];
    pd->sp = sp;
    pd->ss = ss;
    pd->x_extr = pd->y_extr = 0;
    pd->base = sp->me;
    pd->ttfindex = sp->ttfindex;
    pd->nextcnt = pd->prevcnt = 0;
    pd->nextstems = pd->prevstems = NULL;
    pd->next_is_l = pd->prev_is_l = NULL;
    
    if ( !sp->nonextcp && gd->order2 && sp->nextcpindex < gd->realcnt ) {
    
	nextpd = &gd->points[sp->nextcpindex];
	nextpd->ss = ss;
	nextpd->x_extr = nextpd->y_extr = 0;
	nextpd->base = sp->nextcp;
	nextpd->ttfindex = sp->nextcpindex;
    }
    if ( !sp->noprevcp && gd->order2 && sp->prev != NULL &&
	sp->prev->from->nextcpindex < gd->realcnt ) {
	
	nextpd = &gd->points[sp->prev->from->nextcpindex];
	nextpd->ss = ss;
	nextpd->x_extr = nextpd->y_extr = 0;
	nextpd->base = sp->prevcp;
	nextpd->ttfindex = sp->prev->from->nextcpindex;
    }

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
	hv = IsUnitHV( &pd->nextunit,true );
	if ( hv == 2 ) {
	    pd->nextunit.x = 0; pd->nextunit.y = pd->nextunit.y>0 ? 1 : -1;
	} else if ( hv == 1 ) {
	    pd->nextunit.y = 0; pd->nextunit.x = pd->nextunit.x>0 ? 1 : -1;
	}
	if ( pd->nextunit.y==0 ) pd->next_hor = true;
	else if ( pd->nextunit.x==0 ) pd->next_ver = true;
	
	if ( nextpd != NULL ) {
	    nextpd->prevunit.x = -pd->nextunit.x;
	    nextpd->prevunit.y = -pd->nextunit.y;
	}
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
	hv = IsUnitHV( &pd->prevunit,true );
	if ( hv == 2 ) {
	    pd->prevunit.x = 0; pd->prevunit.y = pd->prevunit.y>0 ? 1 : -1;
	} else if ( hv == 1 ) {
	    pd->prevunit.y = 0; pd->prevunit.x = pd->prevunit.x>0 ? 1 : -1;
	}
	if ( pd->prevunit.y==0 ) pd->prev_hor = true;
	else if ( pd->prevunit.x==0 ) pd->prev_ver = true;

	if ( prevpd != NULL ) {
	    prevpd->nextunit.x = -pd->prevunit.x;
	    prevpd->nextunit.y = -pd->prevunit.y;
	}
    }
    {
	same = pd->prevunit.x*pd->nextunit.x + pd->prevunit.y*pd->nextunit.y;
	if ( same<-.95 )
	    pd->colinear = true;
    }
    if (( pd->prev_hor || pd->next_hor ) && pd->colinear ) {
	if ( IsSplinePeak( gd,pd,false,false,1 )) pd->y_extr = 1;
	else if ( IsSplinePeak( gd,pd,true,false,1 )) pd->y_extr = 2;
    } else if (( pd->prev_ver || pd->next_ver ) && pd->colinear ) {
	if ( IsSplinePeak( gd,pd,true,true,1 )) pd->x_extr = 1;
	else if ( IsSplinePeak( gd,pd,false,true,1 )) pd->x_extr = 2;
    } else {
	if (( pd->nextunit.y < 0 && pd->prevunit.y < 0 ) || ( pd->nextunit.y > 0 && pd->prevunit.y > 0 )) {
	    if ( IsSplinePeak( gd,pd,false,false,2 )) pd->y_corner = 1;
	    else if ( IsSplinePeak( gd,pd,true,false,2 )) pd->y_corner = 2;
	}
	if (( pd->nextunit.x < 0 && pd->prevunit.x < 0 ) || ( pd->nextunit.x > 0 && pd->prevunit.x > 0 )) {
	    if ( IsSplinePeak( gd,pd,true,true,2 )) pd->x_corner = 1;
	    else if ( IsSplinePeak( gd,pd,false,true,2 )) pd->x_corner = 2;
	}
    }
    if ( hint_diagonal_intersections ) {
	if (( pd->y_corner || pd->y_extr ) && 
	    RealNear( pd->nextunit.x,-pd->prevunit.x ) &&
	    RealNear( pd->nextunit.y,pd->prevunit.y ) && !pd->nextzero)
	    pd->symetrical_h = true;
	else if (( pd->x_corner || pd->x_extr ) &&
	    RealNear( pd->nextunit.y,-pd->prevunit.y ) &&
	    RealNear( pd->nextunit.x,pd->prevunit.x ) && !pd->nextzero)
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

static int line_pt_cmp( const void *_p1, const void *_p2 ) {
    struct pointdata * const *pd1 = _p1, * const *pd2 = _p2;
    struct linedata *line;
    double ppos1=0,ppos2=0;
    
    if ( (*pd1)->prevline != NULL && 
	( (*pd1)->prevline == (*pd2)->prevline || (*pd1)->prevline == (*pd2)->nextline ))
	line = (*pd1)->prevline;
    else if ( (*pd1)->nextline != NULL && 
	( (*pd1)->nextline == (*pd2)->prevline || (*pd1)->nextline == (*pd2)->nextline ))
	line = (*pd1)->nextline;
    else
return( 0 );

    ppos1 = ( (*pd1)->sp->me.x - line->online.x ) * line->unit.x +
	    ( (*pd1)->sp->me.y - line->online.y ) * line->unit.y;
    ppos2 = ( (*pd2)->sp->me.x - line->online.x ) * line->unit.x +
	    ( (*pd2)->sp->me.y - line->online.y ) * line->unit.y;
	
    if ( ppos1>ppos2 )
return( 1 );
    else if ( ppos1<ppos2 )
return( -1 );
    else
return( 0 );
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

static void AssignStemToPoint( struct pointdata *pd,struct stemdata *stem,int is_next, int left ) {
    struct stemdata ***stems;
    int i, *stemcnt, **is_l;
    
    stems = ( is_next ) ? &pd->nextstems : &pd->prevstems;
    stemcnt = ( is_next ) ? &pd->nextcnt : &pd->prevcnt;
    is_l = ( is_next ) ? &pd->next_is_l : &pd->prev_is_l;
    for ( i=0; i<*stemcnt; i++ ) {
	if ((*stems)[i] == stem )
return;
    }
    
    *stems = realloc( *stems,( *stemcnt+1 )*sizeof( struct stemdata *));
    *is_l  = realloc( *is_l, ( *stemcnt+1 )*sizeof( int ));
    (*stems)[*stemcnt] = stem;
    (*is_l )[*stemcnt] = left;
    (*stemcnt)++;
}

int IsStemAssignedToPoint( struct pointdata *pd,struct stemdata *stem,int is_next ) {
    struct stemdata **stems;
    int i, stemcnt;
    
    stems = ( is_next ) ? pd->nextstems : pd->prevstems;
    stemcnt = ( is_next ) ? pd->nextcnt : pd->prevcnt;
    
    for ( i=0; i<stemcnt; i++ ) {
	if ( stems[i] == stem )
return( i );
    }
return( -1 );
}

static int GetValidPointDataIndex( struct glyphdata *gd,SplinePoint *sp,
    struct stemdata *stem ) {
    
    struct pointdata *tpd;
    
    if ( sp == NULL )
return( -1 );
    if ( sp->ttfindex < gd->realcnt )
return( sp->ttfindex );
    if ( !sp->nonextcp && sp->nextcpindex < gd->realcnt ) {
	tpd = &gd->points[sp->nextcpindex];
	if ( IsStemAssignedToPoint( tpd,stem,false ) != -1 )
return( sp->nextcpindex );
    }
    if ( !sp->noprevcp && sp->prev != NULL && 
	sp->prev->from->nextcpindex < gd->realcnt ) {
	tpd = &gd->points[sp->prev->from->nextcpindex];
	if ( IsStemAssignedToPoint( tpd,stem,true ) != -1 )
return( sp->prev->from->nextcpindex );
    }
return( -1 );
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
	  default:
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
    double lmin = ( stem->lmin < -fudge ) ? stem->lmin : -fudge;
    double lmax = ( stem->lmax > fudge ) ? stem->lmax : fudge;
    double rmin = ( stem->rmin < -fudge ) ? stem->rmin : -fudge;
    double rmax = ( stem->rmax > fudge ) ? stem->rmax : fudge;
    lmin -= .0001; lmax += .0001; rmin -= .0001; rmax += .0001;

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
	    pos = (line->splines[0].c*stspace[j].lt + line->splines[0].d - stem->right.x)*stem->l_to_r.x +
		  (line->splines[1].c*stspace[j].lt + line->splines[1].d - stem->right.y)*stem->l_to_r.y;
	    if ( pos >= rmin && pos <= rmax )
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
	  default:
	  break;
	}
    }
return( false );
}

static int MatchWinding(struct monotonic ** space,int i,int nw,int winding,int which,int idx) {
    struct monotonic *m;
    int j,cnt=0;

    if (( nw<0 && winding>0 ) || (nw>0 && winding<0)) {
	winding = nw;
	for ( j=i-1; j>=0; --j ) {
	    m = space[j];
	    winding += ((&m->xup)[which] ? 1 : -1 );
	    if ( winding==0 ) {
		if ( cnt == idx )
return( j );
		cnt++;
	    }
	}
    } else {
	winding = nw;
	for ( j=i+1; space[j]!=NULL; ++j ) {
	    m = space[j];
	    winding += ((&m->xup)[which] ? 1 : -1 );
	    if ( winding==0 ) {
		if ( cnt == idx )
return( j );
		cnt++;
	    }
	}
    }
return( -1 );
}

static int FindMatchingHVEdge( struct glyphdata *gd,struct pointdata *pd,
    int is_next,Spline **edges,double *other_t,double *dist ) {
    
    double test, t, start, end;
    int which;
    Spline *s;
    Monotonic *m;
    int winding, nw, i, j, ret=0;
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
    if (( d.x = dir->x )<0 ) d.x = -d.x;
    if (( d.y = dir->y )<0 ) d.y = -d.y;
    which = d.x<d.y;		/* closer to vertical */

    if ( s==NULL )		/* Somehow we got an open contour? */
return( 0 );

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
return( 0 );
    }

    j = MatchWinding(space,i,nw,winding,which,0);
    if ( j!=-1 ) {
	other_t[0] = space[j]->t;
	end = space[j]->other;
	dist[0] = end - start;
	if ( dist[0] < 0 ) dist[0] = -dist[0];
	edges[0] = space[j]->s;
	ret++;
    }
    if ( ret > 0 && is_next != 2 && ( pd->x_extr == 1 || pd->y_extr == 1 )) {
	j = MatchWinding(space,i,nw,winding,which,1);
	if ( j!=-1 ) {
	    other_t[ret] = space[j]->t;
	    end = space[j]->other;
	    dist[ret] = end - start;
	    if ( dist[ret] < 0 ) dist[ret] = -dist[ret];
	    edges[ret] = space[j]->s;
	    ret++;
	}
    }
return( ret );
}

static BasePoint PerturbAlongSpline( Spline *s,BasePoint *bp,double t ) {
    BasePoint perturbed;
    
    for (;;) {
	perturbed.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	perturbed.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
	if ( !RealWithin( perturbed.x,bp->x,.01 ) || !RealWithin( perturbed.y,bp->y,.01 ))
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
return( perturbed );
}

static void MakeVirtualLine(struct glyphdata *gd,BasePoint *perturbed,
    BasePoint *dir,Spline *myline,SplinePoint *end1, SplinePoint *end2) {
    
    BasePoint norm, absnorm;
    SplineSet *spl;
    Spline *s, *first;
    double t1, t2;
    int i, cnt;

    if ( gd->stspace==NULL ) {
	for ( i=0; i<2; ++i ) {
	    cnt = 0;
	    for ( spl=gd->sc->layers[gd->layer].splines; spl!=NULL; spl=spl->next ) {
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
		gd->sspace = malloc((cnt+1)*sizeof(Spline *));
	    } else
		gd->sspace[cnt] = NULL;
	}
	gd->stspace = malloc((3*cnt+2)*sizeof(struct st));
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

static int FindMatchingEdge( struct glyphdata *gd, struct pointdata *pd,
    int is_next,Spline **edges ) {
    
    BasePoint *dir, vert, perturbed, diff;
    Spline myline;
    SplinePoint end1, end2;
    double *other_t = is_next==2 ? &pd->both_e_t : is_next ? pd->next_e_t : pd->prev_e_t;
    double *dist = is_next ? pd->next_dist : pd->prev_dist;
    double t ;
    Spline *s;
    int cnt;

    dist[0] = 0; dist[1] = 0;
    if (( is_next && ( pd->next_hor || pd->next_ver )) ||
	( !is_next && ( pd->prev_hor || pd->prev_ver )) ||
	is_next == 2 )
return( FindMatchingHVEdge(gd,pd,is_next,edges,other_t,dist));

    if ( is_next ) {
	dir = &pd->nextunit;
	t = .001;
	s = pd->sp->next;
    } else {
	dir = &pd->prevunit;
	t = .999;
	s = pd->sp->prev;
    }
    /* For spline segments which have slope close enough to the font's italic */
    /* slant look for an opposite edge along the horizontal direction, rather */
    /* than along the normal for the point's next/previous unit. This allows  */
    /* us e. g. to detect serifs in italic fonts */
    if ( gd->has_slant ) {
	if ( UnitsParallel( dir,&gd->slant_unit,true )) {
	    vert.x = 0; vert.y = 1;
	    dir = &vert;
	}
    }

    if ( s==NULL || ( gd->only_hv && !IsUnitHV( dir,false )))
return( 0 );

    diff.x = s->to->me.x-s->from->me.x; diff.y = s->to->me.y-s->from->me.y;
    if ( diff.x<.03 && diff.x>-.03 && diff.y<.03 && diff.y>-.03 )
return( 0 );

    /* Don't base the line on the current point, we run into rounding errors */
    /*  where lines that should intersect it don't. Instead perturb it a tiny*/
    /*  bit in the direction along the spline */
    perturbed = PerturbAlongSpline( s,&pd->sp->me,t );

    MakeVirtualLine(gd,&perturbed,dir,&myline,&end1,&end2);
    /* prev_e_t = next_e_t = both_e_t =. This is where these guys are set */
    cnt = MonotonicOrder(gd->sspace,&myline,gd->stspace);
    edges[0] = MonotonicFindAlong(&myline,gd->stspace,cnt,s,other_t);
return( edges[0] != NULL );
}

static int StillStem(struct glyphdata *gd,double fudge,BasePoint *pos,struct stemdata *stem ) {
    Spline myline;
    SplinePoint end1, end2;
    int cnt, ret;

    MakeVirtualLine( gd,pos,&stem->unit,&myline,&end1,&end2 );
    cnt = MonotonicOrder( gd->sspace,&myline,gd->stspace );
    ret = MonotonicFindStemBounds( &myline,gd->stspace,cnt,fudge,stem );
return( ret );
}

static int CornerCorrectSide( struct pointdata *pd,int x_dir,int is_l ) {
    int corner = ( x_dir ) ? pd->x_corner : pd->y_corner;
    int start = (( x_dir && is_l ) || ( !x_dir && !is_l ));
    double unit_p, unit_n;
    
    unit_p = (&pd->prevunit.x)[!x_dir];
    unit_n = (&pd->nextunit.x)[!x_dir];
return( ( start && (
	( corner == 1 && unit_p > 0 && unit_n > 0 ) ||
	( corner == 2 && unit_p < 0 && unit_n < 0 ))) ||
	( !start && (
	( corner == 1 && unit_p < 0 && unit_n < 0 ) ||
	( corner == 2 && unit_p > 0 && unit_n > 0 ))));
}

static int IsCorrectSide( struct glyphdata *gd,struct pointdata *pd,
    int is_next,int is_l,BasePoint *dir ) {
    
    Spline *sbase, myline;
    SplinePoint *sp = pd->sp, end1, end2;
    BasePoint perturbed;
    int i, hv, is_x, ret = false, winding = 0, cnt, eo;
    double t, test;
    struct monotonic **space, *m;
    
    hv = IsUnitHV( dir,true );
    if (( hv == 2 && pd->x_corner ) || ( hv == 1 && pd->y_corner ))
return( CornerCorrectSide( pd,( hv == 2 ),is_l ));

    sbase = ( is_next ) ? sp->next : sp->prev;
    t = ( is_next ) ? 0.001 : 0.999;
    perturbed = PerturbAlongSpline( sbase,&sp->me,t );
    
    if ( hv ) {
	is_x = ( hv == 2 );
	test = ( is_x ) ? perturbed.y : perturbed.x;
	MonotonicFindAt( gd->ms,is_x,test,space = gd->space );
	for ( i=0; space[i]!=NULL; ++i ) {
	    m = space[i];
	    winding = ((&m->xup)[is_x] ? 1 : -1 );
	    if ( m->s == sbase )
	break;
	}
	if ( space[i]!=NULL )
	    ret = (( is_l && winding == 1 ) || ( !is_l && winding == -1 ));
    } else {
	MakeVirtualLine( gd,&perturbed,dir,&myline,&end1,&end2 );
	cnt = MonotonicOrder( gd->sspace,&myline,gd->stspace );
	eo = -1;
	is_x = fabs( dir->y ) > fabs( dir->x );
	/* If a diagonal stem is more vertical than horizontal, then our     */
	/* virtual line will go from left to right. It will first intersect  */
	/* the left side of the stem, if the stem also points north-east.    */
	/* In any other case the virtual line will first intersect the right */
	/* side. */
	i = ( is_x && dir->y > 0 ) ? 0 : cnt-1; 
	while ( i >= 0 && i <= cnt-1 ) {
	    eo = ( eo != 1 ) ? 1 : 0;
	    if ( gd->stspace[i].s == sbase )
	break;
	    if ( is_x && dir->y > 0 ) i++;
	    else i--;
	}
	ret = ( is_l == eo );
    }
return( ret );
}

/* In TrueType I want to make sure that everything on a diagonal line remains */
/*  on the same line. Hence we compute the line. Also we are interested in */
/*  points that are on the intersection of two lines */
static struct linedata *BuildLine(struct glyphdata *gd,struct pointdata *pd,int is_next ) {
    int i;
    BasePoint *dir, *base, *start, *end;
    struct pointdata **pspace = gd->pspace, *pd2;
    int pcnt=0, is_l, hv;
    double dist_error;
    struct linedata *line;
    double off, firstoff, lastoff, lmin=0, lmax=0;
    
    dir = is_next ? &pd->nextunit : &pd->prevunit;
    is_l = IsCorrectSide( gd,pd,is_next,true,dir );
    dist_error = ( IsUnitHV( dir,true )) ? dist_error_hv : dist_error_diag ;	/* Diagonals are harder to align */
    if ( dir->x==0 && dir->y==0 )
return( NULL );
    base = &pd->sp->me;
    
    for ( i= (pd - gd->points)+1; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	pd2 = &gd->points[i];
	off =  ( pd2->sp->me.x - base->x )*dir->y - 
	       ( pd2->sp->me.y - base->y )*dir->x;
	if ( off <= lmax - 2*dist_error || off >= lmin + 2*dist_error )
    continue;
	if ( off < 0 && off < lmin ) lmin = off;
	else if ( off > 0 && off > lmax ) lmax = off;

	if ((( UnitsParallel( dir,&pd2->nextunit,true ) && pd2->nextline==NULL ) &&
	    IsCorrectSide( gd,pd2,true,is_l,dir )) ||
	    (( UnitsParallel( dir,&pd2->prevunit,true ) && pd2->prevline==NULL ) &&
	    IsCorrectSide( gd,pd2,false,is_l,dir )))
	    pspace[pcnt++] = pd2;
    }
    
    if ( pcnt==0 )
return( NULL );
    if ( pcnt==1 ) {
	/* if the line consists of just these two points, only count it as */
	/*  a true line if the two immediately follow each other */
	if (( pd->sp->next->to != pspace[0]->sp || !pd->sp->next->knownlinear ) &&
	    ( pd->sp->prev->from != pspace[0]->sp || !pd->sp->prev->knownlinear ))
return( NULL );
    }
    
    line = &gd->lines[gd->linecnt++];
    line->pcnt = pcnt+1;
    line->points = malloc((pcnt+1)*sizeof(struct pointdata *));
    line->points[0] = pd;
    line->unit = *dir;
    line->is_left = is_l;
    if ( dir->x < 0 || dir->y == -1 ) {
	line->unit.x = -line->unit.x;
	line->unit.y = -line->unit.y;
    }
    line->online = *base;
    if ( is_next ) {
	pd->nextline = line;
	if ( pd->colinear ) pd->prevline = line;
    } else {
	pd->prevline = line;
	if ( pd->colinear ) pd->nextline = line;
    }
    for ( i=0; i<pcnt; ++i ) {
	if ( UnitsParallel( dir,&pspace[i]->nextunit,true ) && pspace[i]->nextline==NULL ) {
	    pspace[i]->nextline = line;
	    if ( pspace[i]->colinear )
		pspace[i]->prevline = line;
	}
	if ( UnitsParallel( dir,&pspace[i]->prevunit,true ) && pspace[i]->prevline==NULL ) {
	    pspace[i]->prevline = line;
	    if ( pspace[i]->colinear )
		pspace[i]->nextline = line;
	}
	line->points[i+1] = pspace[i];
    }
    qsort( line->points,line->pcnt,sizeof( struct pointdata * ),line_pt_cmp );
    start = &line->points[0]->sp->me;
    end = &line->points[pcnt]->sp->me;
    /* Now recalculate the line unit vector basing on its starting and */
    /* terminal points */
    line->unit.x = ( end->x - start->x );
    line->unit.y = ( end->y - start->y );
    line->length = sqrt( pow( line->unit.x,2 ) + pow( line->unit.y,2 ));
    line->unit.x /= line->length;
    line->unit.y /= line->length;
    hv = IsUnitHV( &line->unit,true );
    if ( hv == 2 ) {
	line->unit.x = 0; line->unit.y = 1;
    } else if ( hv == 1 ) {
	line->unit.x = 1; line->unit.y = 0;
    } else if ( gd->has_slant && UnitsParallel( &line->unit,&gd->slant_unit,true )) {
	firstoff =  ( start->x - base->x )*gd->slant_unit.y - 
		    ( start->y - base->y )*gd->slant_unit.x;
	lastoff =   ( end->x - base->x )*gd->slant_unit.y - 
		    ( end->y - base->y )*gd->slant_unit.x;
	if ( fabs( firstoff ) < 2*dist_error && fabs( lastoff ) < 2*dist_error )
	    line->unit = gd->slant_unit;
    }
return( line );
}

static BasePoint MiddleUnit( BasePoint *unit1, BasePoint *unit2 ) {
    BasePoint u1, u2, ret;
    double hyp;
    int hv;
    
    u1 = *unit1; u2 = *unit2;
    if ( u1.x*u2.x + u1.y*u2.y < 0 ) {
	u2.x = -u2.x; u2.y = -u2.y;
    }
    ret.x = ( u1.x + u2.x )/2;
    ret.y = ( u1.y + u2.y )/2;
    hyp = sqrt( pow( ret.x,2 ) + pow( ret.y,2 ));
    ret.x /= hyp;
    ret.y /= hyp;
    
    hv = IsUnitHV( &ret,true );
    if ( hv ) {
	ret.x = ( hv == 1 ) ? 1 : 0;
	ret.y = ( hv == 1 ) ? 0 : 1;
    }
return( ret );
}

static uint8 IsStubOrIntersection( struct glyphdata *gd, BasePoint *dir1, 
    struct pointdata *pd1, struct pointdata *pd2, int is_next1, int is_next2 ) {
    int i;
    int exc=0;
    double dist, off, ext, norm1, norm2, opp, angle;
    double mid_err = ( stem_slope_error + stub_slope_error )/2;
    SplinePoint *sp1, *sp2, *nsp;
    BasePoint hvdir, *dir2, *odir1, *odir2;
    struct pointdata *npd;
    struct linedata *line;
    
    sp1 = pd1->sp; sp2 = pd2->sp;
    dir2 = ( is_next2 ) ? &pd2->nextunit : &pd2->prevunit;
    hvdir.x = ( int ) rint( dir1->x );
    hvdir.y = ( int ) rint( dir1->y );
    
    line = is_next2 ? pd2->nextline : pd2->prevline;
    if ( !IsUnitHV( dir2,true ) && line != NULL )
	dir2 = &line->unit;

    odir1 = ( is_next1 ) ? &pd1->prevunit : &pd1->nextunit;
    odir2 = ( is_next2 ) ? &pd2->prevunit : &pd2->nextunit;
    
    angle = fabs( GetUnitAngle( dir1,dir2 ));
    if ( angle > (double)stub_slope_error*1.5 && angle < PI - (double)stub_slope_error*1.5 )
return( 0 );

    /* First check if it is a slightly slanted line or a curve which joins */
    /* a straight line under an angle close to 90 degrees. There are many */
    /* glyphs where circles or curved features are intersected by or */
    /* connected to vertical or horizontal straight stems (the most obvious */
    /* cases are Greek Psi and Cyrillic Yu), and usually it is highly desired to */
    /* mark such an intersection with a hint */
    norm1 = ( sp1->me.x - sp2->me.x ) * odir2->x +
	    ( sp1->me.y - sp2->me.y ) * odir2->y;
    norm2 = ( sp2->me.x - sp1->me.x ) * odir1->x +
	    ( sp2->me.y - sp1->me.y ) * odir1->y;
    /* if this is a real stub or intersection, then vectors on both sides */
    /* of out going-to-be stem should point in the same direction. So */
    /* the following value should be positive */
    opp = dir1->x * dir2->x + dir1->y * dir2->y;
    if (( angle <= mid_err || angle >= PI - mid_err ) && 
	opp > 0 && norm1 < 0 && norm2 < 0 && UnitsParallel( odir1,odir2,true ) && 
	( UnitsOrthogonal( dir1,odir1,false ) || UnitsOrthogonal( dir2,odir1,false )))
return( 2 );
    if (( angle <= mid_err || angle >= PI - mid_err ) &&
	opp > 0 && (( norm1 < 0 && pd1->colinear &&
	IsUnitHV( dir1,true ) && UnitsOrthogonal( dir1,odir2,false )) ||
	( norm2 < 0 && pd2->colinear &&
	IsUnitHV( dir2,true ) && UnitsOrthogonal( dir2,odir1,false ))))
return( 4 );
    
    /* Now check if our 2 points form a serif termination or a feature stub */
    /* The check is pretty dumb: it returns 'true' if all the following */
    /* conditions are met: */
    /* - both the points belong to the same contour; */
    /* - there are no more than 3 other points between them; */
    /* - anyone of those intermediate points is positioned by such a way */
    /*   that it falls inside the stem formed by our 2 base point and */
    /*   the vector we are checking and its distance from the first point */
    /*   along that vector is not larger than the stem width; */
    /* - none of the intermediate points is parallel to the vector direction */
    /*   (otherwise we should have checked against that point instead) */
    if ( !UnitsParallel( dir1,&hvdir,false ))
return( 0 );
    
    dist = ( sp1->me.x - sp2->me.x ) * dir1->y -
	   ( sp1->me.y - sp2->me.y ) * dir1->x;
    nsp = sp1;

    for ( i=0; i<4; i++ ) {
	if (( is_next1 && nsp->prev == NULL ) || ( !is_next1 && nsp->next == NULL ))
return( 0 );

	nsp = ( is_next1 ) ? nsp->prev->from : nsp->next->to; 
	if ( ( i>0 && nsp == sp1 ) || nsp == sp2 )
    break;

	npd = &gd->points[nsp->ptindex];
	if (UnitsParallel( &npd->nextunit,&hvdir,false ) || 
	    UnitsParallel( &npd->prevunit,&hvdir,false ))
    break;

	ext = ( sp1->me.x - nsp->me.x ) * hvdir.x +
	      ( sp1->me.y - nsp->me.y ) * hvdir.y;
	if ( ext < 0 ) ext = -ext;
	if (( dist > 0 && ext > dist ) || ( dist < 0 && ext < dist ))
    break;

	off = ( sp1->me.x - nsp->me.x ) * hvdir.y -
	      ( sp1->me.y - nsp->me.y ) * hvdir.x;
	if (( dist > 0 && ( off <= 0 || off >= dist )) ||
	    ( dist < 0 && ( off >= 0 || off <= dist )))
	    exc++;
    }

    if ( nsp == sp2 && exc == 0 )
return( 1 );

return( 0 );
}

/* We normalize all stem unit vectors so that they point between 90 and 270    */
/* degrees, as this range is optimal for sorting diagonal stems. This means    */
/* that vertical stems will normally point top to bottom, but for diagonal     */
/* stems (even if their angle is actually very close to vertical) the opposite */
/* direction is also possible. Sometimes we "normalize" such stems converting  */
/* them to vertical. In such a case we have to swap their edges too.  */
static void SwapEdges( struct glyphdata *gd,struct stemdata *stem ) {
    BasePoint tpos;
    struct pointdata *tpd;
    struct linedata *tl;
    struct stem_chunk *chunk;
    double toff;
    int i, j, temp;
    
    tpos = stem->left; stem->left = stem->right; stem->right = tpos;
    toff = stem->lmin; stem->lmin = stem->rmax; stem->rmax = toff;
    toff = stem->rmin; stem->rmin = stem->lmax; stem->lmax = toff;
    tl = stem->leftline; stem->leftline = stem->rightline; stem->rightline = tl;

    for ( i=0; i<stem->chunk_cnt; ++i ) {
	chunk = &stem->chunks[i];
	tpd = chunk->l; chunk->l = chunk->r; chunk->r = tpd;
	temp = chunk->lpotential; chunk->lpotential = chunk->rpotential; chunk->rpotential = temp;
	temp = chunk->lnext; chunk->lnext = chunk->rnext; chunk->rnext = temp;
	temp = chunk->ltick; chunk->ltick = chunk->rtick; chunk->rtick = temp;
	
	tpd = chunk->l;
	if ( tpd != NULL ) {
	    for ( j=0; j<tpd->nextcnt; j++ )
		if ( tpd->nextstems[j] == stem )
		    tpd->next_is_l[j] = true;
	    for ( j=0; j<tpd->prevcnt; j++ )
		if ( tpd->prevstems[j] == stem )
		    tpd->prev_is_l[j] = true;
	}

	tpd = chunk->r;
	if ( tpd != NULL ) {
	    for ( j=0; j<tpd->nextcnt; j++ )
		if ( tpd->nextstems[j] == stem )
		    tpd->next_is_l[j] = false;
	    for ( j=0; j<tpd->prevcnt; j++ )
		if ( tpd->prevstems[j] == stem )
		    tpd->prev_is_l[j] = false;
	}
    }
    
    /* In case of a quadratic contour invert assignments to stem sides */
    /* also for off-curve points */
    if ( gd->order2 ) {
	for ( i=0; i<gd->realcnt; i++ ) if ( gd->points[i].sp == NULL ) {
	    tpd = &gd->points[i];
	    for ( j=0; j<tpd->nextcnt; j++ )
		if ( tpd->nextstems[j] == stem )
		    tpd->next_is_l[j] = !tpd->next_is_l[j];
	    for ( j=0; j<tpd->prevcnt; j++ )
		if ( tpd->prevstems[j] == stem )
		    tpd->prev_is_l[j] = !tpd->prev_is_l[j];
	}
    }
}

static int StemFitsHV( struct stemdata *stem,int is_x,uint8 mask ) {
    int i,cnt;
    double loff,roff;
    double lmin=0,lmax=0,rmin=0,rmax=0;
    struct stem_chunk *chunk;
    
    cnt = stem->chunk_cnt;
    
    for ( i=0 ; i<stem->chunk_cnt; i++ ) {
	if( stem->chunks[i].stub & mask )
    break;
    }
    if ( i == stem->chunk_cnt )
return( false );    
    if ( stem->chunk_cnt == 1 )
return( true );
    
    for ( i=0;i<cnt;i++ ) {
	chunk = &stem->chunks[i];
	
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

static int LineFitsHV( struct linedata *line ) {
    int i,cnt,is_x,hv;
    double off,min=0,max=0;
    struct pointdata *pd;
    
    cnt = line->pcnt;
    hv = IsUnitHV( &line->unit,true );
    if ( hv )
return( true );

    hv = IsUnitHV( &line->unit,false );
    if ( !hv )
return( false );
    
    is_x = ( hv == 1 ) ? 1 : 0;
    for ( i=0;i<cnt;i++ ) {
	pd = line->points[i];
	
	off =   ( pd->base.x - line->online.x ) * !is_x -
		( pd->base.y - line->online.y ) * is_x;
	if ( off < min ) min = off;
	else if ( off > max ) max = off;
    }
    if (( max - min ) < 2*dist_error_hv )
return( true );
return( false );
}

static int OnStem( struct stemdata *stem,BasePoint *test,int left ) {
    double dist_error, off;
    BasePoint *dir = &stem->unit;
    double max=0, min=0;

    /* Diagonals are harder to align */
    dist_error = IsUnitHV( dir,true ) ? dist_error_hv : dist_error_diag;
    if ( !stem->positioned ) dist_error = dist_error * 2;
    if ( dist_error > stem->width/2 ) dist_error = stem->width/2;
    if ( left ) {
	off = (test->x - stem->left.x)*dir->y - (test->y - stem->left.y)*dir->x;
	max = stem->lmax; min = stem->lmin;
    } else {
	off = (test->x - stem->right.x)*dir->y - (test->y - stem->right.y)*dir->x;
	max = stem->rmax; min = stem->rmin;
    }
    
    if ( off > ( max - dist_error ) && off < ( min + dist_error ) )
return( true );

return( false );
}

static int BothOnStem( struct stemdata *stem,BasePoint *test1,BasePoint *test2,
    int force_hv,int strict,int cove ) {
    double dist_error, off1, off2;
    BasePoint dir = stem->unit;
    int hv, hv_strict;
    double lmax=0, lmin=0, rmax=0, rmin=0;
    
    hv = ( force_hv ) ? IsUnitHV( &dir,false ) : IsUnitHV( &dir,true );
    hv_strict = ( force_hv ) ? IsUnitHV( &dir,true ) : hv;
    if ( force_hv ) {
	if ( force_hv != hv )
return( false );
	if ( !hv_strict && !StemFitsHV( stem,( hv == 1 ),7 ))
return( false );
	if ( !hv_strict ) {
	    dir.x = ( force_hv == 2 ) ? 0 : 1;
	    dir.y = ( force_hv == 2 ) ? 1 : 0;
	}
    }
    /* Diagonals are harder to align */
    dist_error = ( hv ) ? dist_error_hv : dist_error_diag;
    if ( !strict ) {
	dist_error = dist_error * 2;
	lmax = stem->lmax; lmin = stem->lmin;
	rmax = stem->rmax; rmin = stem->rmin;
    }
    if ( dist_error > stem->width/2 ) dist_error = stem->width/2;

    off1 = (test1->x-stem->left.x)*dir.y - (test1->y-stem->left.y)*dir.x;
    off2 = (test2->x-stem->right.x)*dir.y - (test2->y-stem->right.y)*dir.x;
    if (off1 > ( lmax - dist_error ) && off1 < ( lmin + dist_error ) &&
	off2 > ( rmax - dist_error ) && off2 < ( rmin + dist_error )) {
	/* For some reasons in my patch from Feb 24 2008 I prohibited snapping */
	/* to stems point pairs which together form a bend, if at least */
	/* one point from the pair doesn't have exactly the same position as */
	/* the stem edge. Unfortunately I don't remember why I did this, but */
	/* this behavior has at least one obviously negative effect: it */
	/* prevents building a stem from chunks which describe an ark   */
	/* intersected by some straight lines, even if the intersections lie */
	/* closely enough to the ark extremum. So don't apply this test */
	/* at least if the force_hv flag is on (which means either the  */
	/* chunk or the stem itself is not exactly horizontal/vertical) */
	if ( !cove || force_hv || off1 == 0 || off2 == 0 )
return( true );
    }

    off2 = (test2->x-stem->left.x)*dir.y - (test2->y-stem->left.y)*dir.x;
    off1 = (test1->x-stem->right.x)*dir.y - (test1->y-stem->right.y)*dir.x;
    if (off2 > ( lmax - dist_error ) && off2 < ( lmin + dist_error ) &&
	off1 > ( rmax - dist_error ) && off1 < ( rmin + dist_error )) {
	if ( !cove || force_hv || off1 == 0 || off2 == 0 )
return( true );
    }

return( false );
}

static int RecalcStemOffsets( struct stemdata *stem,BasePoint *dir,int left,int right ) {
    double off, err;
    double lmin=0, lmax=0, rmin=0, rmax=0;
    struct stem_chunk *chunk;
    int i;
    
    if ( !left && !right )
return( false );
    err = ( IsUnitHV( dir,true )) ? dist_error_hv : dist_error_diag;

    if ( stem->chunk_cnt > 1 ) for ( i=0; i<stem->chunk_cnt; i++ ) {
	chunk = &stem->chunks[i];
	if ( left && chunk->l != NULL ) {
	    off =  ( chunk->l->sp->me.x - stem->left.x )*dir->y -
		   ( chunk->l->sp->me.y - stem->left.y )*dir->x;
	    if ( off < lmin ) lmin = off;
	    else if ( off > lmax ) lmax = off;
	}
	if ( right && chunk->r != NULL ) {
	    off =  ( chunk->r->sp->me.x - stem->right.x )*dir->y +
		   ( chunk->r->sp->me.y - stem->right.y )*dir->x;
	    if ( off < rmin ) rmin = off;
	    else if ( off > rmax ) rmax = off;
	}
    }
    if ( lmax - lmin < 2*err && rmax - rmin < 2*err ) {
	stem->lmin = lmin; stem->lmax = lmax;
	stem->rmin = rmin; stem->rmax = rmax;
return( true );
    }
return( false );
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
    
    /* Recalculate left/right offsets relatively to new vectors */
    RecalcStemOffsets( stem,&dir,true,true );
}

static struct stem_chunk *AddToStem( struct glyphdata *gd,struct stemdata *stem,
    struct pointdata *pd1,struct pointdata *pd2,int is_next1, int is_next2, int cheat ) {
    
    int is_potential1 = false, is_potential2 = true;
    struct stem_chunk *chunk=NULL;
    BasePoint *dir = &stem->unit;
    BasePoint *test;
    int lincr = 1, rincr = 1;
    double off, dist_error;
    double loff = 0, roff = 0;
    double min = 0, max = 0;
    int i, in, ip, cpidx;
    struct pointdata *pd, *npd, *ppd;

    if ( cheat || stem->positioned ) is_potential2 = false;
    /* Diagonals are harder to align */
    dist_error = IsUnitHV( dir,true ) ? 2*dist_error_hv : 2*dist_error_diag;
    if ( dist_error > stem->width/2 ) dist_error = stem->width/2;
    max = stem->lmax;
    min = stem->lmin;

    /* The following swaps "left" and "right" points in case we have */
    /* started checking relatively to a wrong edge */
    if ( pd1 != NULL ) {
	test = &pd1->base;
	off =   ( test->x - stem->left.x )*dir->y - 
		( test->y - stem->left.y )*dir->x;
	if (( !stem->ghost &&
	    ( off < ( max - dist_error ) || off > ( min + dist_error ))) ||
	    ( RealNear( stem->unit.x, 1) && stem->ghost && stem->width == 21 ) ||
	    ( RealNear( stem->unit.x,0 ) && stem->ghost && stem->width == 20 )) {
	    pd = pd1; pd1 = pd2; pd2 = pd;
	    in = is_next1; is_next1 = is_next2; is_next2 = in;
	    ip = is_potential1; is_potential1 = is_potential2; is_potential2 = ip;
	}
    }

    if ( pd1 == NULL ) lincr = 0;
    if ( pd2 == NULL ) rincr = 0;
    /* Now run through existing stem chunks and see if the chunk we are */
    /* going to add doesn't duplicate an existing one.*/
    for ( i=stem->chunk_cnt-1; i>=0; --i ) {
	chunk = &stem->chunks[i];
	if ( chunk->l == pd1 ) lincr = 0;
	if ( chunk->r == pd2 ) rincr = 0;
	
	if (( chunk->l == pd1 || pd1 == NULL ) && ( chunk->r == pd2 || pd2 == NULL )) {
	    if ( !is_potential1 ) chunk->lpotential = false;
	    if ( !is_potential2 ) chunk->rpotential = false;
    break;
	} else if (( chunk->l == pd1 && chunk->r == NULL ) || ( chunk->r == pd2 && chunk->l == NULL )) {
	    if ( chunk->l == NULL ) {
		chunk->l = pd1;
		chunk->lpotential = is_potential1;
		chunk->lnext = is_next1;
		chunk->ltick = lincr;
	    } else if ( chunk->r == NULL ) {
		chunk->r = pd2;
		chunk->rpotential = is_potential2;
		chunk->rnext = is_next2;
		chunk->rtick = rincr;
	    }
    break;
	}
    }

    if ( i<0 ) {
	stem->chunks = realloc(stem->chunks,(stem->chunk_cnt+1)*sizeof(struct stem_chunk));
	chunk = &stem->chunks[stem->chunk_cnt++];
	chunk->parent = stem;

	chunk->l = pd1; chunk->lpotential = is_potential1;
	chunk->r = pd2; chunk->rpotential = is_potential2;
	chunk->ltick = lincr; chunk->rtick = rincr;

	chunk->lnext = is_next1;
	chunk->rnext = is_next2;
	chunk->stemcheat = cheat;
	chunk->stub = chunk->is_ball = false;
	chunk->l_e_idx = chunk->r_e_idx = 0;
    }
	
    if ( pd1!=NULL ) {
	loff =  ( pd1->base.x - stem->left.x ) * stem->l_to_r.x +
		( pd1->base.y - stem->left.y ) * stem->l_to_r.y;
	if ( is_next1==1 || is_next1==2 || pd1->colinear ) {
	    AssignStemToPoint( pd1,stem,true,true );
	    /* For quadratic layers assign the stem not only to   */
	    /* spline points, but to their control points as well */
	    /* (this may be important for TTF instructions */
	    if ( gd->order2 && !pd1->sp->nonextcp && pd1->sp->nextcpindex < gd->realcnt ) {
		cpidx = pd1->sp->nextcpindex;
		npd = &gd->points[cpidx];
		if ( OnStem( stem,&npd->base,true ))
		    AssignStemToPoint( npd,stem,false,true );
	    }
	}
	if ( is_next1==0 || is_next1==2 || pd1->colinear  ) {
	    AssignStemToPoint( pd1,stem,false,true );
	    if ( gd->order2 && !pd1->sp->noprevcp && pd1->sp->prev != NULL &&
		pd1->sp->prev->from->nextcpindex < gd->realcnt ) {
		cpidx = pd1->sp->prev->from->nextcpindex;
		ppd = &gd->points[cpidx];
		if ( OnStem( stem,&ppd->base,true ))
		    AssignStemToPoint( ppd,stem,true,true );
	    }
	}
    }
    if ( pd2!=NULL ) {
	roff =  ( pd2->base.x - stem->right.x ) * stem->l_to_r.x +
		( pd2->base.y - stem->right.y ) * stem->l_to_r.y;
	if ( is_next2==1 || is_next2==2 || pd2->colinear ) {
	    AssignStemToPoint( pd2,stem,true,false );
	    if ( gd->order2 && !pd2->sp->nonextcp && pd2->sp->nextcpindex < gd->realcnt ) {
		cpidx = pd2->sp->nextcpindex;
		npd = &gd->points[cpidx];
		if ( OnStem( stem,&npd->base,false ))
		    AssignStemToPoint( npd,stem,false,false );
	    }
	}
	if ( is_next2==0 || is_next2==2 || pd2->colinear ) {
	    AssignStemToPoint( pd2,stem,false,false );
	    if ( gd->order2 && !pd2->sp->noprevcp && pd2->sp->prev != NULL &&
		pd2->sp->prev->from->nextcpindex < gd->realcnt ) {
		cpidx = pd2->sp->prev->from->nextcpindex;
		ppd = &gd->points[cpidx];
		if ( OnStem( stem,&ppd->base,false ))
		    AssignStemToPoint( ppd,stem,true,false );
	    }
	}
    }
    if ( loff < stem->lmin ) stem->lmin = loff;
    else if ( loff > stem->lmax ) stem->lmax = loff;
    if ( roff < stem->rmin ) stem->rmin = roff;
    else if ( roff > stem->rmax ) stem->rmax = roff;
    stem->lpcnt += lincr; stem->rpcnt += rincr;
return( chunk );
}

static struct stemdata *FindStem( struct glyphdata *gd,struct pointdata *pd,
    struct pointdata *pd2,BasePoint *dir,int is_next2,int de ) {
    
    int i, cove, test_left, hv, stemcnt;
    struct stemdata *stem;
    SplinePoint *match;
    BasePoint newdir;

    match = pd2->sp;
    stemcnt = ( is_next2 ) ? pd2->nextcnt : pd2->prevcnt;
    
    for ( i=0; i<stemcnt; i++ ) {
	stem = ( is_next2 ) ? pd2->nextstems[i] : pd2->prevstems[i];
	test_left = ( is_next2 ) ? !pd2->next_is_l[i] : !pd2->prev_is_l[i];

	if (UnitsParallel( &stem->unit,dir,true ) && 
	    OnStem( stem,&pd->sp->me,test_left ))
return( stem );
    }

    cove =  ( dir->x == 0 && pd->x_extr + pd2->x_extr == 3 ) || 
	    ( dir->y == 0 && pd->y_extr + pd2->y_extr == 3 );

    /* First pass to check for strict matches */
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	/* Ghost hints and BBox hits are usually generated after all other   */
	/* hint types, but we can get them here in case we are generating    */
	/* glyph data for a predefined hint layout. In this case they should */
	/* be excluded from the following tests */
	if ( stem->ghost || stem->bbox )
    continue;

	if ( UnitsParallel( &stem->unit,dir,true ) &&
	    BothOnStem( stem,&pd->sp->me,&pd2->sp->me,false,true,cove )) {
 return( stem );
	}
    }
    /* One more pass. At this stage larger deviations are allowed */
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( stem->ghost || stem->bbox )
    continue;

	if ( UnitsParallel( &stem->unit,dir,true ) &&
	    BothOnStem( stem,&pd->sp->me,&pd2->sp->me,false,false,cove )) {
return( stem );
	}
    }
    if ( de )
return( NULL );
    
    hv = IsUnitHV( dir,false );
    if ( !hv )
return( NULL );

    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( stem->ghost || stem->bbox )
    continue;
	if ( hv && BothOnStem( stem,&pd->base,&pd2->base,hv,false,cove )) {
	    newdir.x = ( hv == 2 ) ? 0 : 1;
	    newdir.y = ( hv == 2 ) ? 1 : 0;
	    if ( hv == 2 && stem->unit.y < 0 )
		SwapEdges( gd,stem );
	    if ( stem->unit.x != newdir.x )
		SetStemUnit( stem,newdir );
return( stem );
	}
    }
return( NULL );
}

static struct stemdata *NewStem( struct glyphdata *gd,BasePoint *dir,
    BasePoint *pos1, BasePoint *pos2 ) {
    
    struct stemdata * stem = &gd->stems[gd->stemcnt++];
    double width;

    stem->unit = *dir;
    if ( dir->x < 0 || dir->y == -1 ) {
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
    stem->leftidx = stem->rightidx = -1;
    stem->leftline = stem->rightline = NULL;
    stem->lmin = stem->lmax = 0;
    stem->rmin = stem->rmax = 0;
    stem->ldone = stem->rdone = false;
    stem->lpcnt = stem->rpcnt = 0;
    stem->chunks = NULL;
    stem->chunk_cnt = 0;
    stem->ghost = stem->bbox = false;
    stem->positioned = false;
    stem->blue = -1;
return( stem );
}

static int ParallelToDir( struct pointdata *pd,int checknext,BasePoint *dir,
    BasePoint *opposite,SplinePoint *basesp,uint8 is_stub ) {
    
    BasePoint n, o, *base = &basesp->me;
    SplinePoint *sp;
    double angle, mid_err = ( stem_slope_error + stub_slope_error )/2;

    sp = pd->sp;
    n = ( checknext ) ? pd->nextunit : pd->prevunit;

    angle = fabs( GetUnitAngle( dir,&n ));
    if (( !is_stub && angle > stem_slope_error && angle < PI - stem_slope_error ) ||
	( is_stub & 1 && angle > stub_slope_error*1.5 && angle < PI - stub_slope_error*1.5 ) ||
	( is_stub & 6 && angle > mid_err && angle < PI - mid_err ))
return( false );

    /* Now sp must be on the same side of the spline as opposite */
    o.x = opposite->x-base->x; o.y = opposite->y-base->y;
    n.x = sp->me.x-base->x; n.y = sp->me.y-base->y;
    if ( ( o.x*dir->y - o.y*dir->x )*( n.x*dir->y - n.y*dir->x ) < 0 )
return( false );
    
return( true );
}

static int NearlyParallel( BasePoint *dir,Spline *other, double t ) {
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

static double NormalDist( BasePoint *to, BasePoint *from, BasePoint *perp ) {
    double len = (to->x-from->x)*perp->y - (to->y-from->y)*perp->x;
    if ( len<0 ) len = -len;
return( len );
}

static struct stemdata *FindOrMakeHVStem( struct glyphdata *gd,
    struct pointdata *pd,struct pointdata *pd2,int is_h,int require_existing ) {
    int i,cove = false;
    struct stemdata *stem;
    BasePoint dir;

    dir.x = ( is_h ) ? 1 : 0;
    dir.y = ( is_h ) ? 0 : 1;
    if ( pd2 != NULL )
	cove =  ( dir.x == 0 && pd->x_extr + pd2->x_extr == 3 ) || 
		( dir.y == 0 && pd->y_extr + pd2->y_extr == 3 );
    
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( IsUnitHV( &stem->unit,true ) &&
	    ( pd2 != NULL && BothOnStem( stem,&pd->sp->me,&pd2->sp->me,false,false,cove )))
    break;
    }
    if ( i==gd->stemcnt ) stem=NULL;

    if ( stem == NULL && pd2 != NULL && !require_existing )
	stem = NewStem( gd,&dir,&pd->sp->me,&pd2->sp->me );
return( stem );
}

static int IsDiagonalEnd( struct glyphdata *gd,
    struct pointdata *pd1,struct pointdata *pd2,int is_next,int require_existing ) {
    /* suppose we have something like */
    /*  *--*		*/
    /*   \  \		*/
    /*    \  \		*/
    /* Then let's create a vertical stem between the two points */
    /* (and a horizontal stem if the thing is rotated 90 degrees) */
    double width, length1, length2, dist1, dist2;
    BasePoint *pt1, *pt2, *dir1, *dir2, *prevdir1, *prevdir2;
    SplinePoint *prevsp1, *prevsp2;
    struct pointdata *prevpd1, *prevpd2;
    int hv;

    if ( pd1->colinear || pd2->colinear )
return( false );
    pt1 = &pd1->sp->me; pt2 = &pd2->sp->me;
    /* Both key points of a diagonal end stem should have nearly the same */
    /* coordinate by x or y (otherwise we can't determine by which axis   */
    /* it should be hinted) */
    if ( pt1->x >= pt2->x - dist_error_hv &&  pt1->x <= pt2->x + dist_error_hv ) {
	width = pd1->sp->me.y - pd2->sp->me.y;
	hv = 1;
    } else if ( pt1->y >= pt2->y - dist_error_hv &&  pt1->y <= pt2->y + dist_error_hv ) {
	width = pd1->sp->me.x - pd2->sp->me.x;
	hv = 2;
    } else
return( false );

    dir1 = ( is_next ) ? &pd1->nextunit : &pd1->prevunit;
    dir2 = ( is_next ) ? &pd2->prevunit : &pd2->nextunit;
    if ( IsUnitHV( dir1,true ))	/* Must be diagonal */
return( false );
    prevsp1 = ( is_next ) ? pd1->sp->next->to : pd1->sp->prev->from;
    prevsp2 = ( is_next ) ? pd2->sp->prev->from : pd2->sp->next->to;
    prevpd1 = &gd->points[prevsp1->ptindex];
    prevpd2 = &gd->points[prevsp2->ptindex];
    prevdir1 = ( is_next ) ? &prevpd1->prevunit : &prevpd1->nextunit;
    prevdir2 = ( is_next ) ? &prevpd2->nextunit : &prevpd2->prevunit;
    /* Ensure we have got a real diagonal, i. e. its sides are parallel */
    if ( !UnitsParallel( dir1,dir2,true ) || !UnitsParallel( prevdir1,prevdir2,true ))
return( false );

    /* Diagonal width should be smaller than its length */
    length1 = pow(( prevsp1->me.x - pt1->x ),2 ) + pow(( prevsp1->me.y - pt1->y ),2 );
    length2 = pow(( prevsp2->me.x - pt2->x ),2 ) + pow(( prevsp2->me.y - pt2->y ),2 );
    if ( length2 < length1 ) length1 = length2;
    if ( pow( width,2 ) > length1 )
return( false );

    /* Finally exclude too short diagonals where the distance between key   */
    /* points of one edge at the direction orthogonal to the unit vector    */
    /* of the stem we are about to add is smaller than normal HV stem       */
    /* fudge. Such diagonals may be later turned into HV stems, and we will */
    /* result into getting two coincident hints */
    dist1 = ( hv == 1 ) ? prevsp1->me.y - pt1->y : prevsp1->me.x - pt1->x;
    dist2 = ( hv == 1 ) ? prevsp2->me.y - pt2->y : prevsp2->me.x - pt2->x;
    if ( dist1 < 0 ) dist1 = -dist1;
    if ( dist2 < 0 ) dist2 = -dist2;
    if ( dist1 < 2*dist_error_hv && dist2 < 2*dist_error_hv )
return( false );

return( hv );
}

static struct stemdata *TestStem( struct glyphdata *gd,struct pointdata *pd,
    BasePoint *dir,SplinePoint *match,int is_next,int is_next2,int require_existing,uint8 is_stub,int eidx ) {
    struct pointdata *pd2;
    struct stemdata *stem, *destem;
    struct stem_chunk *chunk;
    struct linedata *otherline;
    double width;
    struct linedata *line, *line2;
    BasePoint *mdir, middle;
    int de=false, hv, l_changed;
    
    width = ( match->me.x - pd->sp->me.x )*dir->y - 
	    ( match->me.y - pd->sp->me.y )*dir->x;
    if ( width < 0 ) width = -width;
    if ( width < .5 )
return( NULL );		/* Zero width stems aren't interesting */
    if (( is_next && pd->sp->next->to==match ) || ( !is_next && pd->sp->prev->from==match ))
return( NULL );		/* Don't want a stem between two splines that intersect */

    pd2 = &gd->points[match->ptindex];

    line = is_next ? pd->nextline : pd->prevline;
    mdir = is_next2 ? &pd2->nextunit : &pd2->prevunit;
    line2 = is_next2 ? pd2->nextline : pd2->prevline;
    if ( !IsUnitHV( mdir,true ) && line2 != NULL )
	mdir = &line2->unit;
    if ( mdir->x==0 && mdir->y==0 )
return( NULL );         /* cannot determine the opposite point's direction */

    if ( !UnitsParallel( mdir,dir,true ) && !is_stub )
return( NULL );         /* Cannot make a stem if edges are not parallel (unless it is a serif) */
    
    if ( is_stub & 1 && !IsUnitHV( dir,true )) {
	/* For serifs we prefer the vector which is closer to horizontal/vertical */
	middle = MiddleUnit( dir,mdir );
	if ( UnitCloserToHV( &middle,dir ) == 1  && UnitCloserToHV( &middle,mdir ) == 1 )
	    dir = &middle;
	else if ( UnitCloserToHV( mdir,dir ) == 1 )
	    dir = mdir;
	if ( !IsUnitHV( dir,true ) && 
	    ( hint_diagonal_ends || require_existing )) 
	    de = IsDiagonalEnd( gd,pd,pd2,is_next,require_existing );
    }

    stem = FindStem( gd,pd,pd2,dir,is_next2,de );
    destem = NULL;
    if ( de )
	destem = FindOrMakeHVStem( gd,pd,pd2,( de == 1 ),require_existing );

    if ( stem == NULL && !require_existing )
	stem = NewStem( gd,dir,&pd->sp->me,&match->me );
    if ( stem != NULL ) {
	chunk = AddToStem( gd,stem,pd,pd2,is_next,is_next2,false );
	if ( chunk != NULL ) {
	    chunk->stub = is_stub;
	    chunk->l_e_idx = chunk->r_e_idx = eidx;
	}

	if ( chunk != NULL && gd->linecnt > 0 ) {
	    hv = IsUnitHV( &stem->unit,true );
	    /* For HV stems allow assigning a line to a stem edge only */
	    /* if that line also has an exactly HV vector */
	    if ( line != NULL && (( !hv &&
		UnitsParallel( &stem->unit,&line->unit,true ) && 
		RecalcStemOffsets( stem,&line->unit,true,true )) || 
		( hv && line->unit.x == stem->unit.x && line->unit.y == stem->unit.y ))) {
		
		otherline = NULL; l_changed = false;
		if (( stem->leftline == NULL || 
		    stem->leftline->length < line->length ) && chunk->l == pd ) {
		    
		    stem->leftline = line;
		    l_changed = true;
		    otherline = stem->rightline;
		} else if (( stem->rightline == NULL ||
		    stem->rightline->length < line->length ) && chunk->r == pd ) {
		    
		    stem->rightline = line;
		    l_changed = true;
		    otherline = stem->leftline;
		}
		/* If lines are attached to both sides of a diagonal stem, */
		/* then prefer the longer line */
		if ( !hv && l_changed && !stem->positioned && 
		    ( otherline == NULL || ( otherline->length < line->length )))
		    SetStemUnit( stem,line->unit );
	    }
	    if ( line2 != NULL && (( !hv &&
		UnitsParallel( &stem->unit,&line2->unit,true ) && 
		RecalcStemOffsets( stem,&line2->unit,true,true )) || 
		( hv && line2->unit.x == stem->unit.x && line2->unit.y == stem->unit.y ))) {
		
		otherline = NULL; l_changed = false;
		if (( stem->leftline == NULL ||
		    stem->leftline->length < line2->length ) && chunk->l == pd2 ) {
		    
		    stem->leftline = line2;
		    l_changed = true;
		    otherline = stem->rightline;
		} else if (( stem->rightline == NULL ||
		    stem->rightline->length < line2->length ) && chunk->r == pd2 ) {
		    
		    stem->rightline = line2;
		    l_changed = true;
		    otherline = stem->leftline;
		}
		if ( !hv && l_changed && !stem->positioned && 
		    ( otherline == NULL || ( otherline->length < line2->length )))
		    SetStemUnit( stem,line2->unit );
	    }
	}
    }

    if ( destem != NULL )
	AddToStem( gd,destem,pd,pd2,is_next,!is_next,1 );
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

/* This function is used when generating stem data for preexisting */
/* stem hints. If we already know the desired hint position, then we */
/* can safely assign to this hint any points which meet other conditions */
/* but have no corresponding position at the opposite edge. */
static int HalfStemNoOpposite( struct glyphdata *gd,struct pointdata *pd,
    struct stemdata *stem,BasePoint *dir,int is_next ) {
    int i, ret=0, allowleft, allowright, hv, corner;
    struct stemdata *tstem;
    
    for ( i=0; i<gd->stemcnt; ++i ) {
	tstem = &gd->stems[i];
	if ( tstem->bbox || !tstem->positioned || tstem == stem )
    continue;
	allowleft = ( !tstem->ghost || tstem->width == 20 );
	allowright = ( !tstem->ghost || tstem->width == 21 );
	hv = IsUnitHV( &tstem->unit,true );
	corner = (( pd->x_corner && hv == 2 ) || ( pd->y_corner && hv == 1 ));

	if ( UnitsParallel( &tstem->unit,dir,true ) || tstem->ghost || corner ) {
	    if ( OnStem( tstem,&pd->sp->me,true ) && allowleft ) {
		if ( IsCorrectSide( gd,pd,is_next,true,&tstem->unit )) {
		    AddToStem( gd,tstem,pd,NULL,is_next,false,false );
		    ret++;
		}
	    } else if ( OnStem( tstem,&pd->sp->me,false ) && allowright ) {
		if ( IsCorrectSide( gd,pd,is_next,false,&tstem->unit )) {
		    AddToStem( gd,tstem,NULL,pd,false,is_next,false );
		    ret++;
		}
	    }
	}
    }
return( ret );
}

static struct stemdata *HalfStem( struct glyphdata *gd,struct pointdata *pd,
    BasePoint *dir,Spline *other,double other_t,int is_next,int eidx ) {
    /* Find the spot on other where the slope is the same as dir */
    double t1;
    double width;
    BasePoint match;
    struct stemdata *stem = NULL, *tstem;
    struct pointdata *pd2 = NULL, *tpd;
    int i;

    t1 = FindSameSlope( other,dir,other_t );
    if ( t1==-1e4 )
return( NULL );
    if ( t1<0 && other->from->prev!=NULL && gd->points[other->from->ptindex].colinear ) {
	other = other->from->prev;
	t1 = FindSameSlope(other,dir,1.0);
    } else if ( t1>1 && other->to->next!=NULL && gd->points[other->to->ptindex].colinear ) {
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
	pd->nextedges[eidx] = other;
	pd->next_e_t[eidx] = t1;
    } else {
	pd->prevedges[eidx] = other;
	pd->prev_e_t[eidx] = t1;
    }

    /* In my experience the only case where this function may be useful */
    /* is when it occasionally finds a real spline point which for some */
    /* reasons has been neglected by other tests and yet forms a valid  */
    /* pair for the first point. So run through points and see if we    */
    /* have actually got just a position on spline midway between to points, */
    /* or it is a normal point allowing to make a normal stem chunk */
    for ( i=0; i<gd->pcnt; ++i ) {
	tpd = &gd->points[i];
	if ( tpd->sp != NULL && tpd->sp->me.x == match.x && tpd->sp->me.y == match.y ) {
	    pd2 = tpd;
    break;
	}
    }
    for ( i=0; i<gd->stemcnt; ++i ) {
	tstem = &gd->stems[i];
	if ( UnitsParallel( &tstem->unit,dir,true ) && 
	    BothOnStem( tstem,&pd->base,&match,false,false,false )) {
	    stem = tstem;
    break;
	}
    }
    if ( stem == NULL )
	stem = NewStem(gd,dir,&pd->sp->me,&match);
    
    AddToStem( gd,stem,pd,pd2,is_next,false,false );
return( stem );
}

static int ConnectsAcross( struct glyphdata *gd,SplinePoint *sp,
    int is_next,Spline *findme,int eidx ) {
    struct pointdata *pd = &gd->points[sp->ptindex];
    Spline *other, *test;
    BasePoint dir;

    other = ( is_next ) ? pd->nextedges[eidx] : pd->prevedges[eidx];

    if ( other==findme )
return( true );
    if ( other==NULL )
return( false );

    dir.x = ( is_next ) ? -pd->nextunit.x : pd->prevunit.x;
    dir.y = ( is_next ) ? -pd->nextunit.y : pd->prevunit.y;
    test = other->to->next;
    while ( test!=NULL && test != other &&
	    gd->points[test->from->ptindex].nextunit.x * dir.x +
	    gd->points[test->from->ptindex].nextunit.y * dir.y > 0 ) {
	if ( test==findme )
return( true );
	test = test->to->next;
    }
	    
    dir.x = ( is_next ) ? pd->nextunit.x : -pd->prevunit.x;
    dir.y = ( is_next ) ? pd->nextunit.y : -pd->prevunit.y;
    test = other->from->prev;
    while ( test!=NULL && test != other &&
	    gd->points[test->to->ptindex].prevunit.x * dir.x +
	    gd->points[test->to->ptindex].prevunit.y * dir.y > 0 ) {
	if ( test==findme )
return( true );
	test = test->from->prev;
    }
return( false );
}

static int ConnectsAcrossToStem( struct glyphdata *gd,struct pointdata *pd,
    int is_next,struct stemdata *target,int is_l,int eidx ) {

    Spline *other, *test;
    BasePoint dir;
    struct pointdata *tpd;
    int ecnt, stemidx;

    ecnt = ( is_next ) ? pd->next_e_cnt : pd->prev_e_cnt;
    if ( ecnt < eidx + 1 )
return( false );
    other = ( is_next ) ? pd->nextedges[eidx] : pd->prevedges[eidx];

    test = other;
    dir.x = ( is_next ) ? pd->nextunit.x : -pd->prevunit.x;
    dir.y = ( is_next ) ? pd->nextunit.y : -pd->prevunit.y;
    do {
	tpd = &gd->points[test->to->ptindex];
	stemidx = IsStemAssignedToPoint( tpd,target,false );
	if ( stemidx != -1 && tpd->prev_is_l[stemidx] == !is_l &&
	    IsSplinePeak( gd,tpd,rint( target->unit.y ),rint( target->unit.y ),7 ))
return( true );
	
	test = test->to->next;
    } while ( test!=NULL && test != other && stemidx == -1 &&
	( tpd->prevunit.x * dir.x + tpd->prevunit.y * dir.y >= 0 ));
	    
    test = other;
    dir.x = ( is_next ) ? -pd->nextunit.x : pd->prevunit.x;
    dir.y = ( is_next ) ? -pd->nextunit.y : pd->prevunit.y;
    do {
	tpd = &gd->points[test->from->ptindex];
	stemidx = IsStemAssignedToPoint( tpd,target,true );
	if ( stemidx != -1 && tpd->next_is_l[stemidx] == !is_l &&
	    IsSplinePeak( gd,tpd,rint( target->unit.y ),rint( target->unit.y ),7 ))
return( true );

	test = test->from->prev;
    } while ( test!=NULL && test != other && stemidx == -1 &&
	( tpd->nextunit.x * dir.x + tpd->nextunit.y * dir.y >= 0 ));
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

static int BuildStem( struct glyphdata *gd,struct pointdata *pd,int is_next,
    int require_existing,int has_existing,int eidx ) {
    BasePoint *dir;
    Spline *other, *cur;
    double t;
    double tod, fromd, dist;
    SplinePoint *testpt, *topt, *frompt;
    struct linedata *line;
    struct pointdata *testpd, *topd, *frompd;
    int tp, fp, t_needs_recalc=false, ret=0;
    uint8 tstub=0, fstub=0;
    BasePoint opposite;
    struct stemdata *stem=NULL;

    if ( is_next ) {
	dir = &pd->nextunit;
	other = pd->nextedges[eidx];
	cur = pd->sp->next;
	t = pd->next_e_t[eidx];
	dist = pd->next_dist[eidx];
    } else {
	dir = &pd->prevunit;
	other = pd->prevedges[eidx];
	cur = pd->sp->prev;
	t = pd->prev_e_t[eidx];
	dist = pd->prev_dist[eidx];
    }
    topt = other->to; frompt = other->from;
    topd = &gd->points[topt->ptindex];
    frompd = &gd->points[frompt->ptindex];
    
    line = is_next ? pd->nextline : pd->prevline;
    if ( !IsUnitHV( dir,true ) && line != NULL)
	dir = &line->unit;

    if ( other==NULL )
return( 0 );

    opposite.x = ((other->splines[0].a*t+other->splines[0].b)*t+other->splines[0].c)*t+other->splines[0].d;
    opposite.y = ((other->splines[1].a*t+other->splines[1].b)*t+other->splines[1].c)*t+other->splines[1].d;

    if ( eidx == 0 ) tstub = IsStubOrIntersection( gd,dir,pd,topd,is_next,false );
    if ( eidx == 0 ) fstub = IsStubOrIntersection( gd,dir,pd,frompd,is_next,true );
    tp = ParallelToDir( topd,false,dir,&opposite,pd->sp,tstub );
    fp = ParallelToDir( frompd,true,dir,&opposite,pd->sp,fstub );
    
    /* if none of the opposite points is parallel to the needed vector, then    */
    /* give it one more chance by skipping those points and looking at the next */
    /* and previous one. This can be useful in situations where the opposite    */
    /* edge cannot be correctly detected just because there are too many points */
    /* on the spline (which is a very common situation for poorly designed      */
    /* fonts or fonts with quadratic splines). */
    /* But do that only for colinear spline segments and ensure that there are  */
    /* no bends between two splines. */
    if ( !tp && ( !fp || t > 0.5 ) &&
	topd->colinear && &other->to->next != NULL ) {
	testpt = topt->next->to; 
	testpd = &gd->points[testpt->ptindex];
	BasePoint *initdir = &topd->prevunit;
	while ( !tp && topd->colinear && pd->sp != testpt && other->from != testpt && (
	    testpd->prevunit.x * initdir->x +
	    testpd->prevunit.y * initdir->y > 0 )) {

	    topt = testpt; topd = testpd;
	    tp = ParallelToDir( topd,false,dir,&opposite,pd->sp,false );
	    testpt = topt->next->to; 
	    testpd = &gd->points[testpt->ptindex];
	}
	if ( tp ) t_needs_recalc = true;
    }
    if ( !fp && ( !fp || t < 0.5 ) &&
	frompd->colinear && &other->from->prev != NULL ) {
	testpt = frompt->prev->from; 
	testpd = &gd->points[testpt->ptindex];
	BasePoint *initdir = &frompd->prevunit;
	while ( !fp && frompd->colinear && pd->sp != testpt && other->to != testpt && (
	    testpd->prevunit.x * initdir->x +
	    testpd->prevunit.y * initdir->y > 0 )) {

	    frompt = testpt; frompd = testpd;
	    fp = ParallelToDir( frompd,true,dir,&opposite,pd->sp,false );
	    testpt = frompt->prev->from; 
	    testpd = &gd->points[testpt->ptindex];
	}
	if ( fp ) t_needs_recalc = true;
    }
    if ( t_needs_recalc )
	t = RecalcT( other,frompt,topt,t );
    if ( !tp && !fp ) {
	if ( has_existing )
	    ret = HalfStemNoOpposite( gd,pd,NULL,dir,is_next );
return( ret );
    }

    /* We have several conflicting metrics for getting the "better" stem */
    /* Generally we prefer the stem with the smaller width (but not always. See tilde) */
    /* Generally we prefer the stem formed by the point closer to the intersection */
    tod = (1-t)*NormalDist( &topt->me,&pd->sp->me,dir );
    fromd = t*NormalDist( &frompt->me,&pd->sp->me,dir );

    if ( tp && (( tod<fromd ) ||
	( !fp && ( tod<2*fromd || dist < topd->prev_dist[eidx] || 
	    ConnectsAcross( gd,frompt,true,cur,eidx ) || NearlyParallel( dir,other,t ))))) {
	stem = TestStem( gd,pd,dir,topt,is_next,false,require_existing,tstub,eidx );
    }
    if ( stem == NULL && fp && (( fromd<tod ) ||
	( !tp && ( fromd<2*tod || dist < frompd->next_dist[eidx] || 
	    ConnectsAcross( gd,topt,false,cur,eidx ) || NearlyParallel( dir,other,t ))))) {
	stem = TestStem( gd,pd,dir,frompt,is_next,true,require_existing,fstub,eidx );
    }
    if ( eidx == 0 && stem == NULL && !require_existing && cur!=NULL && 
	!other->knownlinear && !cur->knownlinear )
	stem = HalfStem( gd,pd,dir,other,t,is_next,eidx );
    if ( stem != NULL ) ret = 1;
    if ( has_existing )
	ret += HalfStemNoOpposite( gd,pd,stem,dir,is_next );
return( ret );
}

static void AssignLinePointsToStems( struct glyphdata *gd ) {
    struct pointdata *pd;
    struct stemdata *stem;
    struct linedata *line;
    struct stem_chunk *chunk;
    int i, j, stem_hv, line_hv, needs_hv=false;
    
    for ( i=0; i<gd->stemcnt; ++i ) if ( !gd->stems[i].toobig ) {
	stem = &gd->stems[i];
	stem_hv = IsUnitHV( &stem->unit,true );
	needs_hv = ( stem_hv || ( stem->chunk_cnt == 1 && 
	    stem->chunks[0].stub && IsUnitHV( &stem->unit,false )));
	
	if ( stem->leftline != NULL ) {
	    line = stem->leftline;
	    line_hv = ( needs_hv && LineFitsHV( line ));

	    if ( needs_hv && !line_hv )
		stem->leftline = NULL;
	    else {
		for ( j=0; j<line->pcnt; j++ ) {
		    pd = line->points[j];
		    if ( pd->prevline == line && OnStem( stem,&pd->base,true ) &&
			IsStemAssignedToPoint( pd,stem,false ) == -1) {
			chunk = AddToStem( gd,stem,pd,NULL,false,false,false );
			chunk->lpotential = true;
		    } if ( pd->nextline == line && OnStem( stem,&pd->base,true ) &&
			IsStemAssignedToPoint( pd,stem,true ) == -1 ) {
			chunk = AddToStem( gd,stem,pd,NULL,true,false,false );
			chunk->lpotential = true;
		    }
		}
	    }
	}
	if ( stem->rightline != NULL ) {
	    line = stem->rightline;
	    line_hv = ( needs_hv && LineFitsHV( line ));

	    if ( needs_hv && !line_hv )
		stem->rightline = NULL;
	    else {
		for ( j=0; j<line->pcnt; j++ ) {
		    pd = line->points[j];
		    if ( pd->prevline == line && OnStem( stem,&pd->base,false ) &&
			IsStemAssignedToPoint( pd,stem,false ) == -1 ) {
			chunk = AddToStem( gd,stem,NULL,pd,false,false,false );
			chunk->rpotential = true;
		    } if ( pd->nextline == line && OnStem( stem,&pd->base,false ) &&
			IsStemAssignedToPoint( pd,stem,true ) == -1 ) {
			chunk = AddToStem( gd,stem,NULL,pd,false,true,false );
			chunk->rpotential = true;
		    }
		}
	    }
	}
    }
}

static struct stemdata *DiagonalCornerStem( struct glyphdata *gd,
    struct pointdata *pd,int require_existing ) {
    Spline *other = pd->bothedge;
    struct pointdata *pfrom = NULL, *pto = NULL, *pd2 = NULL, *pd3=NULL;
    double width, length;
    struct stemdata *stem;
    struct stem_chunk *chunk;

    pfrom = &gd->points[other->from->ptindex];
    pto = &gd->points[other->to->ptindex];
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
return( NULL );

    if ( pd->symetrical_v )
	width = (pd->sp->me.x-pd2->sp->me.x);
    else
	width = (pd->sp->me.y-pd2->sp->me.y);
    length = (pd->sp->next->to->me.x-pd->sp->me.x)*(pd->sp->next->to->me.x-pd->sp->me.x) +
	     (pd->sp->next->to->me.y-pd->sp->me.y)*(pd->sp->next->to->me.y-pd->sp->me.y);
    if ( width*width>length )
return( NULL );

    stem = FindOrMakeHVStem(gd,pd,pd2,pd->symetrical_h,require_existing);
    
    if ( pd3 == NULL && stem != NULL )
	chunk = AddToStem( gd,stem,pd,pd2,2,2,2 );
    else if ( stem != NULL ) {
	chunk = AddToStem( gd,stem,pd,pd2,2,2,3 );
	chunk = AddToStem( gd,stem,pd,pd3,2,2,3 );
    }
return( stem );
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

static int stem_cmp( const void *_p1, const void *_p2 ) {
    struct stemdata * const *st1 = _p1, * const *st2 = _p2;
    double start1, end1, start2, end2;
    
    if ( fabs( (*st1)->unit.x ) > fabs( (*st1)->unit.y )) {
	start1 = (*st1)->right.y; end1 = (*st1)->left.y;
	start2 = (*st2)->right.y; end2 = (*st2)->left.y;
    } else {
	start1 = (*st1)->left.x; end1 = (*st1)->right.x;
	start2 = (*st2)->left.x; end2 = (*st2)->right.x;
    }
	
    if ( start1 > start2 )
return( 1 );
    else if ( start1 < start2 )
return( -1 );
    else {
	if ( end1 > end2 )
return( 1 );
	else if ( end1 < end2 )
return( -1 );
	else
return( 0 );
    }
}

static void FixupT( struct pointdata *pd,int stemidx,int isnext, int eidx ) {
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
    struct stemdata *stem ;
    
    if ( pd == NULL || stemidx == -1 )
return;
    stem = ( isnext ) ? pd->nextstems[stemidx] : pd->prevstems[stemidx];
    width = ( stem->right.x - stem->left.x )*stem->unit.y - 
	    ( stem->right.y-stem->left.y )*stem->unit.x;
    s = ( isnext ) ? pd->nextedges[eidx] : pd->prevedges[eidx];
    if ( s==NULL )
return;
    diff.x = s->to->me.x-s->from->me.x;
    diff.y = s->to->me.y-s->from->me.y;
    if ( diff.x<.001 && diff.x>-.001 && diff.y<.001 && diff.y>-.001 )
return;		/* Zero length splines give us NaNs */
    len = sqrt( pow( diff.x,2 ) + pow( diff.y,2 ));
    dot = ( diff.x*stem->unit.x + diff.y*stem->unit.y )/len;
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
	sign = (( isnext && pd->next_is_l[stemidx] ) || ( !isnext && pd->prev_is_l[stemidx] )) ? 1 : -1;
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
	pd->next_e_t[eidx] = t;
    else
	pd->prev_e_t[eidx] = t;
}

/* flags: 1 -- accept curved extrema, 2 -- accept angles, */
/*	4 -- analyze segments (not just single points)    */
static int IsSplinePeak( struct glyphdata *gd,struct pointdata *pd,
    int outer,int is_x,int flags ) {
    
    double base, next, prev, nextctl, prevctl, unit_p, unit_n;
    Spline *s, *snext, *sprev;
    struct monotonic **space, *m;
    int wprev, wnext, i, desired;
    SplinePoint *sp = pd->sp;
    
    base = ((real *) &sp->me.x)[!is_x];
    nextctl = sp->nonextcp ? base : ((real *) &sp->nextcp.x)[!is_x];
    prevctl = sp->noprevcp ? base : ((real *) &sp->prevcp.x)[!is_x];
    next = prev = base;
    snext = sp->next; sprev = sp->prev;
    
    if ( snext->to == NULL || sprev->from == NULL )
return( false );
    if (!( flags & 2) && ( sp->nonextcp || sp->noprevcp ))
return( false );
    else if (!( flags & 1 ) && ( pd->colinear ))
return( false );

    if ( flags & 4 ) {
	while ( snext->to->next != NULL && snext->to != sp && next == base ) {
	    next = ((real *) &snext->to->me.x)[!is_x];
	    snext = snext->to->next;
	}

	while ( sprev->from->prev != NULL && sprev->from != sp && prev == base ) {
	    prev = ((real *) &sprev->from->me.x)[!is_x];
	    sprev = sprev->from->prev;
	}
    } else {
	next = ((real *) &snext->to->me.x)[!is_x];
	prev = ((real *) &sprev->from->me.x)[!is_x];
    }
    
    if ( prev<base && next<base && nextctl<=base && prevctl<=base )
	desired = ( outer ) ? -1 : 1;
    else if ( prev>base && next>base && prevctl>=base && nextctl>=base )
	desired = ( outer ) ? 1 : -1;
    else
return( false );

    MonotonicFindAt( gd->ms,is_x,((real *) &sp->me.x)[is_x],space = gd->space );
    wprev = wnext = 0;
    for ( i=0; space[i]!=NULL; ++i ) {
	m = space[i];
	s = m->s;

	if ( s->from == sp )
	    wnext = ((&m->xup)[is_x] ? 1 : -1 );
	else if ( s->to == sp )
	    wprev = ((&m->xup)[is_x] ? 1 : -1 );
    }

    if ( wnext != 0 && wprev != 0 && wnext != wprev ) {
	unit_p = (&pd->prevunit.x)[!is_x];
	unit_n = (&pd->nextunit.x)[!is_x];
	if ( unit_p < unit_n && (
	    ( outer && wprev == 1 ) || ( !outer && wprev == -1 )))
return( desired );
	else if ( unit_p > unit_n && (
	    ( outer && wnext == 1 ) || ( !outer && wnext == -1 )))
return( desired );
    } else {
	if ( wnext == desired || wprev == desired )
return( desired );
    }

return( false );
}

static struct pointdata *FindClosestOpposite( 
    struct stemdata *stem,struct stem_chunk **chunk,SplinePoint *sp,int *next ) {

    struct pointdata *pd, *ret=NULL;
    struct stem_chunk *testchunk;
    double test, proj=1e4;
    int i, is_l;
    
    for ( i=0; i<stem->chunk_cnt; ++i ) {
	testchunk = &stem->chunks[i];
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
return( ret );
}

static int ValueChunk( struct glyphdata *gd,struct vchunk *vchunks,
    int chcnt,int idx,int l_base ) {
    
    int peak1=0, peak2=0, val=0; 
    int i, is_x, base_next, opp_next;
    struct pointdata *base, *opp, *frompd, *topd;
    struct stem_chunk *chunk = vchunks[idx].chunk, *tchunk;
    struct stemdata *stem = chunk->parent;
    double norm, dist;
    Spline *sbase, *sopp, *other;
    
    /* If a stem was already present before generating glyph data, */
    /* then it should always be preferred in case of a conflict    */
    if ( stem->positioned || chunk->stemcheat ) val++;
    
    if ( l_base ) {
	base = chunk->l; opp = chunk->r;
	base_next = chunk->lnext; opp_next = chunk->rnext;
    } else {
	base = chunk->r; opp = chunk->l;
	base_next = chunk->rnext; opp_next = chunk->lnext;
    }
    sbase = ( base_next ) ? base->sp->next : base->sp->prev;
    sopp = ( opp_next ) ? opp->sp->next : opp->sp->prev;
    other = ( opp_next ) ? opp->nextedges[0] : opp->prevedges[0];
    
    /* If there are 2 conflicting chunks belonging to different stems but       */
    /* based on the same point, then we have to decide which stem is "better"   */
    /* for that point. We compare stems (or rather chunks) by assigning a       */
    /* value to each of them and then prefer the stem whose value is positive.  */
    /* A chunk gets a +1 value bonus in the following cases:                    */
    /* - The stem is vertical/horizontal and splines are curved in the same     */
    /*   direction at both sides of the chunk;                                  */
    /* - A stem has both its width and the distance between the opposite points */
    /*   smaller than another stem;                                             */
    /* - The common side of two stems is a straight line formed by two points   */
    /*   and the opposite point can be projected to line segment between those  */
    /*   two points. */
    if ( IsUnitHV( &stem->unit,true ) && !sbase->knownlinear ) {
	is_x = (int) rint( stem->unit.y );
	peak1 = ( is_x ) ? base->x_extr : base->y_extr;
	peak2 = ( is_x ) ? opp->x_extr  : opp->y_extr;

	dist =  ( base->base.x - opp->base.x )*stem->unit.x +
		( base->base.y - opp->base.y )*stem->unit.y;
	
	/* Are there any stems attached to the same base point which     */
	/* are narrower than the distance between two points forming the */
	/* given chunk? */
	for ( i=0; i<chcnt; i++ ) {
	    tchunk = vchunks[i].chunk;
	    if ( tchunk == NULL || tchunk == chunk || chunk->l == NULL || chunk->r == NULL )
	continue;
	    norm = tchunk->parent->width;
	    if ( norm < fabs( dist ))
	break;
	}

	/* If both points are curved in the same direction, then check also    */
	/* the "line of sight" between those points (if there are interventing */
	/* splines, then it is not a real feature bend)*/
	if ( i == chcnt && peak1 + peak2 == 3 && ConnectsAcross( gd,base->sp,opp_next,sopp,0 ))
	    val++;
    }
    
    frompd = &gd->points[sbase->from->ptindex];
    topd = &gd->points[sbase->to->ptindex];
    
    if (IsStemAssignedToPoint( frompd,stem,true ) != -1 &&
	IsStemAssignedToPoint( topd,stem,false ) != -1 )
	if ( other == sbase ) val++;

    dist = vchunks[idx].dist;
    for ( i=0; i<chcnt; i++ ) {
	tchunk = vchunks[i].chunk;
	if ( tchunk == NULL || tchunk == chunk ||
	    ( vchunks[idx].parallel && !vchunks[i].parallel ))
    continue;
	if ( vchunks[i].dist <= dist || tchunk->parent->width <= stem->width )
    break;
    }
    if ( i==chcnt ) val++;

    /* If just one of the checked chunks has both its sides parallel            */
    /* to the stem direction, then we consider it is always worth to be output. */
    /* This check was introduced to avoid situations where a stem marking       */
    /* a feature termination can be preferred to another stem which controls    */
    /* the main part of the same feature */
    if ( vchunks[idx].parallel ) {
	for ( i=0; i<chcnt; i++ ) {
	    if ( vchunks[i].chunk == NULL || vchunks[i].chunk == chunk )
	continue;
	    if ( vchunks[i].parallel )
	break;
	}
	if ( i == chcnt ) val++;
    }
    
return( val );
}

static void CheckPotential( struct glyphdata *gd,struct pointdata *pd,int is_next ) {
    int i, j, is_l, next1, stemcnt, val;
    int val_cnt=0;
    BasePoint *lunit, *runit;
    struct stemdata **stems;
    struct vchunk *vchunks;
    struct stem_chunk *cur;
    
    stemcnt = ( is_next ) ? pd->nextcnt : pd->prevcnt;
    stems = ( is_next ) ? pd->nextstems : pd->prevstems;
    vchunks = calloc( stemcnt,sizeof( VChunk ));
    
    for ( i=0; i<stemcnt; i++ ) {
	is_l = ( is_next ) ? pd->next_is_l[i] : pd->prev_is_l[i];
	FindClosestOpposite( stems[i],&vchunks[i].chunk,pd->sp,&next1 );
	if ( vchunks[i].chunk == NULL )
    continue;
	cur = vchunks[i].chunk;
	if ( vchunks[i].value > 0 ) val_cnt++;
	vchunks[i].dist  =  pow( cur->l->base.x - cur->r->base.x,2 ) + 
			    pow( cur->l->base.y - cur->r->base.y,2 );
	lunit = ( cur->lnext ) ? &cur->l->nextunit : &cur->l->prevunit;
	runit = ( cur->rnext ) ? &cur->r->nextunit : &cur->r->prevunit;
	vchunks[i].parallel =   UnitsParallel( lunit,&stems[i]->unit,2 ) &&
				UnitsParallel( runit,&stems[i]->unit,2 );
    }
    
    for ( i=0; i<stemcnt; i++ ) if ( vchunks[i].chunk != NULL ) {
	vchunks[i].value = ValueChunk( gd,vchunks,stemcnt,i,is_l );
	if ( vchunks[i].value ) val_cnt++;
    }

    /* If we was unable to figure out any reasons for which at least */
    /* one of the checked chunks should really be output, then keep  */
    /* all the 'potential' flags as they are and do nothing */
    if ( val_cnt > 0 ) {
	for ( i=0; i<stemcnt; i++ ) if ( vchunks[i].chunk != NULL )  {
	    is_l = ( is_next ) ? pd->next_is_l[i] : pd->prev_is_l[i];
	    val = vchunks[i].value;
	    for ( j=0; j<stems[i]->chunk_cnt; j++ ) {
		cur = &stems[i]->chunks[j];
		if ( is_l && cur->l == pd ) {
		    if ( val > 0 ) cur->lpotential = false;
		    else cur->lpotential = true;
		} else if ( !is_l && cur->r == pd ) {
		    if ( val > 0 ) cur->rpotential = false;
		    else cur->rpotential = true;
		}
	    }
	}
    }
    free( vchunks );
}

static int StemIsActiveAt( struct glyphdata *gd,struct stemdata *stem,double stempos ) {
    BasePoint pos,cpos,mpos;
    int which;
    double test;
    double lmin, lmax, rmin, rmax, loff, roff, minoff, maxoff;
    struct monotonic **space, *m;
    int winding, nw, closest, i, j;

    pos.x = stem->left.x + stempos*stem->unit.x;
    pos.y = stem->left.y + stempos*stem->unit.y;

    if ( IsUnitHV( &stem->unit,true )) {
	which = stem->unit.x==0;
	MonotonicFindAt(gd->ms,which,((real *) &pos.x)[which],space = gd->space);
	test = ((real *) &pos.x)[!which];

	lmin = ( stem->lmax - 2*dist_error_hv < -dist_error_hv ) ? 
	    stem->lmax - 2*dist_error_hv : -dist_error_hv;
	lmax = ( stem->lmin + 2*dist_error_hv > dist_error_hv ) ? 
	    stem->lmin + 2*dist_error_hv : dist_error_hv;
	rmin = ( stem->rmax - 2*dist_error_hv < -dist_error_hv ) ? 
	    stem->rmax - 2*dist_error_hv : -dist_error_hv;
	rmax = ( stem->rmin + 2*dist_error_hv > dist_error_hv ) ? 
	    stem->rmin + 2*dist_error_hv : dist_error_hv;
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

	j = MatchWinding(space,i,nw,winding,which,0);
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

/* This function is used to check the distance between a hint's edge */
/* and a spline and determine the extet where this hint can be */
/* considered "active". */
static int WalkSpline( struct glyphdata *gd, struct pointdata *pd,int gonext,
    struct stemdata *stem,int is_l,int force_ac,BasePoint *res ) {
    
    int i, curved;
    double off, dist, min, max;
    double incr, err;
    double t, ratio, width;
    Spline *s;
    BasePoint *base, *nunit, pos, good;
    SplinePoint *sp, *nsp;
    struct pointdata *npd;

    err = ( IsUnitHV( &stem->unit,true )) ? dist_error_hv : dist_error_diag;
    width = stem->width;
    ratio = gd->emsize/( 6 * width );
    if ( err > width/2) err = width/2;

    sp = pd->sp;
    base = ( is_l ) ? &stem->left : &stem->right;
    min = ( is_l ) ? stem->lmax - 2*err : stem->rmax - 2*err;
    max = ( is_l ) ? stem->lmin + 2*err : stem->rmin + 2*err;
    
    s = ( gonext ) ? sp->next : sp->prev;
    nsp = ( gonext ) ? s->to : s->from;
    npd   = &gd->points[nsp->ptindex];
    nunit = ( gonext ) ? &npd->prevunit : &npd->nextunit;
    good = sp->me;
    
    off   = ( nsp->me.x - base->x )*stem->l_to_r.x +
	    ( nsp->me.y - base->y )*stem->l_to_r.y;
    /* Some splines have tiny control points and are almost flat */
    /*  think of them as lines then rather than treating them as curves */
    /*  figure out how long they remain within a few orthoganal units of */
    /*  the point */
    /* We used to check the distance between a control point and a spline */
    /* and consider the segment "flat" if this distance is smaller than   */
    /* the normal allowed "error" value. However this method doesn't produce */
    /* consistent results if the spline is not long enough (as usual for  */
    /* fonts with quadratic splines). So now we consider a spline "flat"  */
    /* only if it never deviates too far from the hint's edge and both    */
    /* its terminal points are snappable to the same hint */
    curved = ( IsStemAssignedToPoint( npd,stem,gonext ) == -1 &&
	( off < min || off > max || !UnitsParallel( &stem->unit,nunit,true )));

    /* If a spline does deviate from the edge too far to consider it flat, */
    /* then we calculate the extent where the spline and the edge are still */
    /* close enough to consider the hint active at this zone. If the hint is */
    /* still active at the end of the spline, we can check some subsequent splines */
    /* too. This method produces better effect than any "magic" manipulations */
    /* with control point coordinates, because it takes into account just the */
    /* spline configuration rather than point positions */
    if ( curved ) {
	max = err = dist_error_curve;
	min = -dist_error_curve;
	/* The following statement forces our code to detect an active zone */
	/* even if all checks actually fail. This makes sense for stems */
	/* marking arks and bends */
	if ( force_ac )
	    good = ( gonext ) ? sp->nextcp : sp->prevcp;
	/* If a spline is closer to the opposite stem edge than to the current edge, then we */
	/* can no longer consider the stem active at this point */
	if ( err > width/2 ) err = width/2;
	
	t = ( gonext ) ? 0.9999 : 0.0001;
	for ( ; ; s = ( gonext ) ? s->to->next : s->from->prev ) {
	    pos.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	    pos.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
	    off   = ( pos.x - base->x )*stem->l_to_r.x +
		    ( pos.y - base->y )*stem->l_to_r.y;
	    dist  = ( pos.x - sp->me.x )*stem->unit.x +
		    ( pos.y - sp->me.y )*stem->unit.y;
	    nsp   = ( gonext ) ? s->to : s->from;
	    npd   = &gd->points[nsp->ptindex];
	    if (fabs( off ) < max && fabs( dist ) <= ( width + width * ratio ) &&
		nsp != sp && npd->colinear && !npd->x_extr && !npd->y_extr && 
		StillStem( gd,err,&pos,stem ))
		good = pos;
	    else
	break;
	}
    }
    t = .5;
    incr = ( gonext ) ? .25 : -.25;
    for ( i=0; i<6; ++i ) {
	pos.x = ((s->splines[0].a*t+s->splines[0].b)*t+s->splines[0].c)*t+s->splines[0].d;
	pos.y = ((s->splines[1].a*t+s->splines[1].b)*t+s->splines[1].c)*t+s->splines[1].d;
	off   = ( pos.x - base->x )*stem->l_to_r.x +
		( pos.y - base->y )*stem->l_to_r.y;
	dist  = ( pos.x - sp->me.x )*stem->unit.x +
		( pos.y - sp->me.y )*stem->unit.y;
	/* Don't check StillStem for non-curved segments, as they are subject */
	/* to further projection-related tests anyway */
	if ( off > min && off < max && ( !curved || 
	    ( fabs( dist ) < ( width + width * ratio ) &&
	    StillStem( gd,err,&pos,stem )))) {
	    
	    good = pos;
	    t += incr;
	} else
	    t -= incr;
	incr/=2;
    }
    *res = good;
return( curved );
}

static int AdjustForImperfectSlopeMatch( SplinePoint *sp,BasePoint *pos,
    BasePoint *newpos,struct stemdata *stem,int is_l ) {
   
    double poff, err, min, max;
    BasePoint *base;
    
    base = ( is_l ) ? &stem->left : &stem->right;
    err = ( IsUnitHV( &stem->unit,true )) ? dist_error_hv : dist_error_diag;
    min = ( is_l ) ? stem->lmax - 2*err : stem->rmax - 2*err;
    max = ( is_l ) ? stem->lmin + 2*err : stem->rmin + 2*err;
    
    /* Possible if the stem unit has been attached to a line. It is */
    /* hard to prevent this */
    if ( min > max ) {
	min = stem->lmin; max = stem->lmax;
    }

    poff =  ( pos->x - base->x )*stem->l_to_r.x +
	    ( pos->y - base->y )*stem->l_to_r.y;
    if ( poff > min && poff < max ) {
	*newpos = *pos;
return( false );
    } else if ( poff <= min )
	err = fabs( min );
    else if ( poff >= max )
	err = fabs( max );

    newpos->x = sp->me.x + err*( pos->x - sp->me.x )/fabs( poff );
    newpos->y = sp->me.y + err*( pos->y - sp->me.y )/fabs( poff );
return( true );
}

static int AddLineSegment( struct stemdata *stem,struct segment *space,int cnt,
    int is_l,struct pointdata *pd,int base_next,struct glyphdata *gd ) {
    
    double s, e, t, dot;
    BasePoint stemp, etemp;
    BasePoint *start, *end, *par_unit;
    int same_dir, corner = 0, par;
    int scurved = false, ecurved = false, c, hv;
    SplinePoint *sp, *psp, *nsp;
    double b;
    uint8 extr;
    
    if ( pd==NULL || (sp = pd->sp)==NULL || sp->ticked ||
	    sp->next==NULL || sp->prev==NULL )
return( cnt );
    end = &sp->me;
    start = &sp->me;
    par_unit = ( base_next ) ? &pd->nextunit : &pd->prevunit;
    /* Do the spline and the stem unit point in the same direction ? */
    dot =   ( stem->unit.x * par_unit->x ) +
	    ( stem->unit.y * par_unit->y );
    same_dir = (( dot > 0 && base_next ) || ( dot < 0 && !base_next ));
    if ( stem->unit.x == 1 ) corner = pd->y_corner;
    else if ( stem->unit.y == 1 ) corner = pd->x_corner;
    
    dot =   ( stem->unit.x * pd->nextunit.x ) +
	    ( stem->unit.y * pd->nextunit.y );
    /* We used to apply normal checks only if the point's unit vector pointing */
    /* in the direction we are going to check is nearly parallel to the stem unit. */
    /* But this is not the best method, because a spline, "parallel" to our */
    /* stem, may actually have filled space at a wrong side. On the other hand, */
    /* sometimes it makes sense to calculate active space even for splines */
    /* connected to our base point under an angle which is too large to consider */
    /* the direction "parallel". So now we check the units' direction first */
    /* and then (just for straight splines) also their parallelity. */
    if (( dot > 0 && same_dir ) || ( dot < 0 && !same_dir )) {
	/* If the segment sp-start doesn't have exactly the right slope, then */
	/*  we can only use that bit of it which is less than a standard error */
	par = UnitsParallel( &stem->unit,&pd->nextunit,0 );
	if ( !sp->next->knownlinear ) {
	    ecurved = WalkSpline( gd,pd,true,stem,is_l,par,&etemp );
	    /* Can merge, but treat as curved relatively to projections */
	    if ( !ecurved ) ecurved = 2;
	    end = &etemp;
	} else if ( par || corner )  {
	    nsp = sp->next->to;
	    ecurved = AdjustForImperfectSlopeMatch( sp,&nsp->me,&etemp,stem,is_l );
	    end = &etemp;
	}
    }
    dot =   ( stem->unit.x * pd->prevunit.x ) +
	    ( stem->unit.y * pd->prevunit.y );
    if (( dot < 0 && same_dir ) || ( dot > 0 && !same_dir )) {
	par = UnitsParallel( &stem->unit,&pd->prevunit,0 );
	if ( !sp->prev->knownlinear ) {
	    scurved = WalkSpline( gd,pd,false,stem,is_l,par,&stemp );
	    if ( !scurved ) scurved = 2;
	    start = &stemp;
	} else if ( par || corner ) {
	    psp = sp->prev->from;
	    scurved = AdjustForImperfectSlopeMatch( sp,&psp->me,&stemp,stem,is_l );
	    start = &stemp;
	}
    }
    sp->ticked = true;

    s = (start->x-stem->left.x)*stem->unit.x +
	(start->y-stem->left.y)*stem->unit.y;
    e = (  end->x-stem->left.x)*stem->unit.x +
	(  end->y-stem->left.y)*stem->unit.y;
    b = (sp->me.x-stem->left.x)*stem->unit.x +
	(sp->me.y-stem->left.y)*stem->unit.y;

    if ( s == e )
return( cnt );
    if ( s > e ) {
	t = s; c = scurved;
	s = e; e = t;
	scurved = ecurved; ecurved = c;
    }
    space[cnt].start = s;
    space[cnt].end = e;
    space[cnt].sbase = space[cnt].ebase = b;
    space[cnt].scurved = scurved;
    space[cnt].ecurved = ecurved;
    
    hv = IsUnitHV( &stem->unit,true );
    if ( hv ) {
	/* For vertical/horizontal stems we assign a special meaning to  */
	/* the 'curved' field. It will be non-zero if the key point of   */
	/* this segment is positioned on a prominent curve:              */
	/* 1 if the inner side of that curve is inside of the contour    */
	/* and 2 otherwise. */
	/* Later, if we get a pair of "inner" and "outer" curves, then   */
	/* we are probably dealing with a feature's bend which should be */
	/* necessarily marked with a hint. Checks we apply for this type */
	/* of curved segments should be less strict than in other cases. */
	extr = ( hv == 1 ) ? pd->y_extr : pd->x_extr;
	space[cnt].curved = extr;
    } else {
	/* For diagonal stems we consider a segment "curved" if both its */
	/* start and end are curved. Curved segments usually cannot be   */
	/* merged (unless scurved or ecurved is equal to 2) and are not  */
	/* checked for "projections". */
	space[cnt].curved = scurved && ecurved;
    }
return( cnt+1 );
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
    double middle;

    for ( i=j=0; i<cnt; ++i, ++j ) {
	if ( i!=j )
	    space[j] = space[i];
	while ( i+1<cnt && space[i+1].start<space[j].end ) {
	    if ( space[i+1].end >= space[j].end ) {
		
		/* If there are 2 overlapping segments and neither the  */
		/* end of the first segment nor the start of the second */
		/* one are curved we can merge them. Otherwise we have  */
		/* to preserve them both, but modify their start/end properties */
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
		    middle = (space[j].end + space[i+1].start)/2;
		    space[j].end = space[i+1].start = middle;
		    --i;
		}
	    }
	    ++i;
	}
    }
return( j );
}

static int MergeSegmentsFinal( struct segment *space, int cnt ) {
    int i,j;

    for ( i=j=0; i<cnt; ++i, ++j ) {
	if ( i!=j )
	    space[j] = space[i];
	while ( i+1<cnt && space[i+1].start<=space[j].end ) {
	    if ( space[i+1].end>space[j].end ) {
		space[j].end = space[i+1].end;
		space[j].ebase = space[i+1].ebase;
		space[j].ecurved = space[i+1].ecurved;
		space[j].curved = false;
	    }
	    ++i;
	}
    }
return( j );
}

static void FigureStemActive( struct glyphdata *gd, struct stemdata *stem ) {
    int i, j, pcnt=0;
    struct pointdata *pd, **pspace = gd->pspace;
    struct stem_chunk *chunk;
    struct segment *lspace = gd->lspace, *rspace = gd->rspace;
    struct segment *bothspace = gd->bothspace, *activespace = gd->activespace;
    int lcnt, rcnt, bcnt, bpos, acnt, cove, startset, endset;
    double middle, width, len, clen, gap, lseg, rseg;
    double err, lmin, rmax, loff, roff, last, s, e, sbase, ebase;
    double proj, proj2, proj3, orig_proj, ptemp;

    width = stem->width;
    
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL )
	gd->points[i].sp->ticked = false;

    lcnt = rcnt = 0;
    for ( i=0; i<stem->chunk_cnt; ++i ) {
	chunk = &stem->chunks[i];
	if ( chunk->stemcheat )
    continue;
	lcnt = AddLineSegment( stem,lspace,lcnt,true ,chunk->l,chunk->lnext,gd );
	rcnt = AddLineSegment( stem,rspace,rcnt,false,chunk->r,chunk->rnext,gd );
    }
    bcnt = 0;
    if ( lcnt!=0 && rcnt!=0 ) {
	/* For curved segments we can extend left and right active segments    */
	/* a bit to ensure that they do overlap and thus can be marked with an */
	/* active zone */
	if ( rcnt == lcnt && stem->chunk_cnt == lcnt ) {
	    for ( i=0; i<lcnt; i++ ) {
		/* If it's a feature bend, then our tests should be more liberal */
		cove = (( rspace[i].curved + lspace[i].curved ) == 3 );
		gap = 0;
		if ( lspace[i].start>rspace[i].end && lspace[i].scurved && rspace[i].ecurved )
		    gap = lspace[i].start-rspace[i].end;
		else if ( rspace[i].start>lspace[i].end && rspace[i].scurved && lspace[i].ecurved )
		    gap = rspace[i].start-lspace[i].end;
		else if ( !cove )
	    continue;

		lseg = lspace[i].end - lspace[i].start;
		rseg = rspace[i].end - rspace[i].start;
		if (( cove && gap < (lseg > rseg ? lseg : rseg )) ||
		    ( gap < ( lseg + rseg )/2 && !stem->chunks[i].stub )) {
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
		cove = (( rspace[j].curved + lspace[i].curved ) == 3 );
		
		s = ( rspace[j].start > lspace[i].start ) ?
		    rspace[j].start : lspace[i].start;
		e = ( rspace[j].end < lspace[i].end ) ?
		    rspace[j].end : lspace[i].end;
		sbase = ( rspace[j].start > lspace[i].start ) ?
		    lspace[i].sbase : rspace[j].sbase;
		ebase = ( rspace[j].end < lspace[i].end ) ?
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
		
		sbase = ( rspace[j].sbase > lspace[i].sbase ) ?
		    rspace[j].sbase : lspace[i].sbase;
		ebase = ( rspace[j].ebase < lspace[i].ebase ) ?
		    rspace[j].ebase : lspace[i].ebase;
		if ( sbase > bothspace[bcnt].end )
		    sbase = ebase = bothspace[bcnt].end;
		else if ( ebase < bothspace[bcnt].start )
		    sbase = ebase = bothspace[bcnt].start;
		else if ( ebase < sbase )
		    ebase = sbase = ( ebase + sbase )/2;
		bothspace[bcnt].sbase = sbase;
		bothspace[bcnt].ebase = ebase;

		bothspace[bcnt++].curved = rspace[j].curved || lspace[i].curved;

		if ( rspace[j].end>lspace[i].end )
	    break;
		++j;
	    }
	}
    }
#if GLYPH_DATA_DEBUG
    fprintf( stderr, "Active zones for stem l=%.2f,%.2f r=%.2f,%.2f dir=%.2f,%.2f:\n",
	stem->left.x,stem->left.y,stem->right.x,stem->right.y,stem->unit.x,stem->unit.y );
    for ( i=0; i<lcnt; i++ ) {
	fprintf( stderr, "\tleft space curved=%d\n",lspace[i].curved );
	fprintf( stderr, "\t\tstart=%.2f,base=%.2f,curved=%d\n",
	    lspace[i].start,lspace[i].sbase,lspace[i].scurved );
	fprintf( stderr, "\t\tend=%.2f,base=%.2f,curved=%d\n",
	    lspace[i].end,lspace[i].ebase,lspace[i].ecurved );
    }
    for ( i=0; i<rcnt; i++ ) {
	fprintf( stderr, "\tright space curved=%d\n",rspace[i].curved );
	fprintf( stderr, "\t\tstart=%.2f,base=%.2f,curved=%d\n",
	    rspace[i].start,rspace[i].sbase,rspace[i].scurved );
	fprintf( stderr, "\t\tend=%.2f,base=%.2f,curved=%d\n",
	    rspace[i].end,rspace[i].ebase,rspace[i].ecurved );
    }
    for ( i=0; i<bcnt; i++ ) {
	fprintf( stderr, "\tboth space\n" );
	fprintf( stderr, "\t\tstart=%.2f,base=%.2f,curved=%d\n",
	    bothspace[i].start,bothspace[i].sbase,bothspace[i].scurved );
	fprintf( stderr, "\t\tend=%.2f,base=%.2f,curved=%d\n",
	    bothspace[i].end,bothspace[i].ebase,bothspace[i].ecurved );
    }
    fprintf( stderr,"\n" );
#endif

    err = ( stem->unit.x == 0 || stem->unit.y == 0 ) ?
	dist_error_hv : dist_error_diag;
    lmin = ( stem->lmin < -err ) ? stem->lmin : -err;
    rmax = ( stem->rmax > err ) ? stem->rmax : err;
    acnt = 0;
    if ( bcnt!=0 ) {
	for ( i=0; i<gd->pcnt; ++i ) if ( (pd = &gd->points[i])->sp!=NULL ) {
	    /* Let's say we have a stem. And then inside that stem we have */
	    /*  another rectangle. So our first stem isn't really a stem any */
	    /*  more (because we hit another edge first), yet it's still reasonable*/
	    /*  to align the original stem */
	    /* Now suppose the rectangle is rotated a bit so we can't make */
	    /*  a stem from it. What do we do here? */
	    loff =  ( pd->sp->me.x - stem->left.x ) * stem->unit.y -
		    ( pd->sp->me.y - stem->left.y ) * stem->unit.x;
	    roff =  ( pd->sp->me.x - stem->right.x ) * stem->unit.y -
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
		last = bothspace[bpos].start;
		startset = false; endset = false;

 		if ( bothspace[bpos].scurved || 
		    StemIsActiveAt( gd,stem,bothspace[bpos].start+0.0015 )) {
		    
		    activespace[acnt].scurved = bothspace[bpos].scurved;
		    activespace[acnt].start  = bothspace[bpos].start;
		    startset = true;
		}
		
		/* If the stem is preceded by a curved segment, then skip     */
		/* the first point position and start from the next one.      */
		/* (Otherwise StemIsActiveAt() may consider the stem is       */
		/* "inactive" at the fragment between the start of the active */
		/* space and the first point actually belonging to this stem) */
		if ( bothspace[bpos].scurved ) {
		    while ( pcnt>i && pspace[i]->projection < bothspace[bpos].sbase ) i++;
		    
		    if ( pcnt > i && pspace[i]->projection >= bothspace[bpos].sbase ) {
			last = activespace[acnt].end = pspace[i]->projection;
			activespace[acnt].ecurved = false;
			activespace[acnt].curved = false;
			endset=true;
		    }
		}
		
		while ( i<pcnt && (
		    ( !bothspace[bpos].ecurved && pspace[i]->projection<bothspace[bpos].end ) ||
		    ( bothspace[bpos].ecurved && pspace[i]->projection<=bothspace[bpos].ebase ))) {
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
		
		if (( bothspace[bpos].ecurved || 
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
	chunk = &stem->chunks[i];
	/* stemcheat 1 -- diagonal edge stem;
	 *           2 -- diagonal corner stem with a sharp top;
	 *           3 -- diagonal corner stem with a flat top;
	 *           4 -- bounding box hint */
	if ( chunk->stemcheat==3 && chunk->l!=NULL && chunk->r!=NULL &&
		i+1<stem->chunk_cnt &&
		stem->chunks[i+1].stemcheat==3 && 
		( chunk->l==stem->chunks[i+1].l ||
		chunk->r==stem->chunks[i+1].r )) {
	    
	    SplinePoint *sp = chunk->l==stem->chunks[i+1].l ?
		chunk->l->sp : chunk->r->sp;
	    proj =  (sp->me.x - stem->left.x) *stem->unit.x +
		    (sp->me.y - stem->left.y) *stem->unit.y;

	    SplinePoint *sp2 = chunk->l==stem->chunks[i+1].l ?
		chunk->r->sp : chunk->l->sp;
	    SplinePoint *sp3 = chunk->l==stem->chunks[i+1].l ?
		stem->chunks[i+1].r->sp : stem->chunks[i+1].l->sp;
	    proj2 = (sp2->me.x - stem->left.x) *stem->unit.x +
		    (sp2->me.y - stem->left.y) *stem->unit.y;
	    proj3 = (sp3->me.x - stem->left.x) *stem->unit.x +
		    (sp3->me.y - stem->left.y) *stem->unit.y;

	    if ( proj2>proj3 ) {
		ptemp = proj2; proj2 = proj3; proj3 = ptemp;
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
	} else if ( chunk->stemcheat && chunk->l!=NULL && chunk->r!=NULL ) {
	    SplinePoint *sp = chunk->l->sp;
	    proj =  ( sp->me.x - stem->left.x ) * stem->unit.x +
		    ( sp->me.y - stem->left.y ) * stem->unit.y;
	    orig_proj = proj;
	    SplinePoint *other = chunk->lnext ? sp->next->to : sp->prev->from;
	    len  =  (other->me.x - sp->me.x) * stem->unit.x +
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

    if ( acnt!=0 ) {
	stem->activecnt = MergeSegmentsFinal( activespace,acnt );
	stem->active = malloc(acnt*sizeof(struct segment));
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
    int i, j, stemidx;
    struct stemdata *stem;
    struct stem_chunk *chunk;

    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	for ( j=0; j<stem->chunk_cnt; ++j ) {
	    chunk = &stem->chunks[j];
	    if ( chunk->l!=NULL ) {
		stemidx = IsStemAssignedToPoint( chunk->l,stem,true );
		FixupT( chunk->l,stemidx,true,chunk->l_e_idx );
		stemidx = IsStemAssignedToPoint( chunk->l,stem,false );
		FixupT( chunk->l,stemidx,false,chunk->l_e_idx );
	    }
	    if ( chunk->r!=NULL ) {
		stemidx = IsStemAssignedToPoint( chunk->r,stem,true );
		FixupT( chunk->r,stemidx,true,chunk->r_e_idx );
		stemidx = IsStemAssignedToPoint( chunk->r,stem,false );
		FixupT( chunk->r,stemidx,false,chunk->r_e_idx );
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
    roff = ( stem2->right.x - stem1->right.x ) * stem1->unit.y -
	   ( stem2->right.y - stem1->right.y ) * stem1->unit.x;
    loff = fabs( loff ); roff = fabs( roff );
    if ( loff > stem1->width || roff > stem1->width )
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

/* Convert diagonal stems generated for stubs and intersections to horizontal */
/* or vertical, if they have just one chunk. This should be done before calculating */
/* active zones, as they are calculated against each stem's unit vector */
static void GDNormalizeStubs( struct glyphdata *gd ) {
    int i, j, hv;
    struct stemdata *stem;
    struct stem_chunk *chunk;
    BasePoint newdir;
    
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( stem->positioned )
    continue;
	
	if ( !IsUnitHV( &stem->unit,true )) {
	    hv = IsUnitHV( &stem->unit,false );
	    if ( hv && StemFitsHV( stem,( hv == 1 ),3 )) {
		if ( hv == 2 && stem->unit.y < 0 )
		    SwapEdges( gd,stem );

		newdir.x = fabs( rint( stem->unit.x ));
		newdir.y = fabs( rint( stem->unit.y ));
		SetStemUnit( stem,newdir );
		
		for ( j=0; j<stem->chunk_cnt && stem->leftidx == -1 && stem->rightidx == -1; j++ ) {
		    chunk = &stem->chunks[j];
		    
		    if ( stem->leftidx == -1 && chunk->l != NULL )
			stem->leftidx = GetValidPointDataIndex( gd,chunk->l->sp,stem );
		    if ( stem->rightidx == -1 && chunk->r != NULL )
			stem->rightidx = GetValidPointDataIndex( gd,chunk->r->sp,stem );
		}
	    }
	}
    }
}

static void GDFindUnlikelyStems( struct glyphdata *gd ) {
    double width, minl, ratio;
    int i, j, k, stem_cnt, ls_cnt, rs_cnt, ltick, rtick;
    struct pointdata *lpd, *rpd;
    Spline *ls, *rs;
    SplinePoint *lsp, *rsp;
    BasePoint *lunit, *runit, *slunit, *srunit, *sunit;
    struct stemdata *stem, *stem1, *tstem;
    struct stemdata **tstems, **lstems, **rstems;
    struct stem_chunk *chunk;

    GDStemsFixupIntersects( gd );
   
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];

	/* If stem had been already present in the spline char before we */
	/* started generating glyph data, then it should never be */
	/* considered "too big" */
	if ( stem->positioned )
    continue;

	/* If a stem has straight edges, and it is wider than tall */
	/*  then it is unlikely to be a real stem */
	width = stem->width;
	ratio = IsUnitHV( &stem->unit,true ) ? gd->emsize/( 6 * width ) : -0.25;
	stem->toobig =  ( stem->clen + stem->clen * ratio < width );
    }

    /* One more check for curved stems. If a stem has just one active */
    /* segment, this segment is curved and the stem has no conflicts, */
    /* then select the active segment length which allows us to consider */
    /* this stem suitable for PS output by such a way, that stems connecting */
    /* the opposite sides of a circle are always accepted */
    for ( i=0; i<gd->stemcnt; ++i ) if ( gd->stems[i].toobig ) {
	stem = &gd->stems[i];
	width = stem->width;

	if ( IsUnitHV( &stem->unit,true ) && stem->activecnt == 1 && 
	    stem->active[0].curved && width/2 > dist_error_curve ) {
	    
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

    /* And finally a check for stubs and feature terminations. We don't */
    /* want such things to be controlled by any special hints, if there */
    /* is already a hint controlling the middle of the same feature */
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( stem->positioned )
    continue;
	
	if ( stem->chunk_cnt == 1 && stem->chunks[0].stub & 3 ) {
	    chunk = &stem->chunks[0];
	    slunit = chunk->lnext ? &chunk->l->nextunit : &chunk->l->prevunit;
	    srunit = chunk->rnext ? &chunk->r->nextunit : &chunk->r->prevunit;
	    
	    /* This test is valid only for features which are not exactly horizontal/    */
	    /* vertical. But we can't check this using the stem unit, as it may have     */
	    /* already beeen reset to HV. So we use the units of this stem's base points */
	    /* instead. */
	    if ( IsUnitHV( slunit,true ) && IsUnitHV( srunit,true ))
    continue;
	    if ( UnitCloserToHV( srunit,slunit ) > 0 ) sunit = srunit;
	    else sunit = slunit;
	    
	    lpd = chunk->l; lsp = lpd->sp; lstems = tstems = NULL;
	    ls_cnt = 0;
	    do {
		stem_cnt = (( chunk->lnext && lpd == chunk->l ) ||
			    ( !chunk->lnext && lpd != chunk->l )) ? lpd->nextcnt : lpd->prevcnt;
		for ( j=0; j<stem_cnt; j++ ) {
		    tstems= (( chunk->lnext && lpd == chunk->l ) ||
			    ( !chunk->lnext && lpd != chunk->l )) ? lpd->nextstems : lpd->prevstems;
		    tstem = tstems[j];
		    if ( tstem != stem ) {
			lstems = tstems;
			ls_cnt = stem_cnt;
		break;
		    }
		}
		if( lstems != NULL )
	    break;
		ls = ( chunk->lnext ) ? lsp->next : lsp->prev;
		if ( ls == NULL )
	    break;
		lsp = ( chunk->lnext ) ? ls->to : ls->from;
		lpd = &gd->points[lsp->ptindex];
		lunit = ( chunk->lnext ) ? &lpd->prevunit : &lpd->nextunit;
	    } while ( lpd != chunk->l && lpd != chunk->r &&
		UnitsParallel( lunit,sunit,false ));

	    rpd = chunk->r; rsp = rpd->sp; rstems = tstems = NULL;
	    rs_cnt = 0;
	    do {
		stem_cnt = (( chunk->rnext && rpd == chunk->r ) ||
			    ( !chunk->rnext && rpd != chunk->r )) ? rpd->nextcnt : rpd->prevcnt;
		for ( j=0; j<stem_cnt; j++ ) {
		    tstems= (( chunk->rnext && rpd == chunk->r ) ||
			    ( !chunk->rnext && rpd != chunk->r )) ? rpd->nextstems : rpd->prevstems;
		    tstem = tstems[j];
		    if ( tstem != stem ) {
			rstems = tstems;
			rs_cnt = stem_cnt;
		break;
		    }
		}
		if( rstems != NULL )
	    break;
		rs = ( chunk->rnext ) ? rsp->next : rsp->prev;
		if ( rs == NULL )
	    break;
		rsp = ( chunk->rnext ) ? rs->to : rs->from;
		rpd = &gd->points[rsp->ptindex];
		runit = ( chunk->rnext ) ? &rpd->prevunit : &rpd->nextunit;
	    } while ( rpd != chunk->r && rpd != chunk->l &&
		UnitsParallel( runit,sunit,false ));
	    
	    if ( lstems != NULL && rstems !=NULL ) {
		for ( j=0; j<ls_cnt && !stem->toobig; j++ ) {
		    for ( k=0; k<rs_cnt && !stem->toobig; k++ ) {
			if ( lstems[j] == rstems[k] && IsUnitHV( &lstems[j]->unit,true )) {
			    stem->toobig = true;
			}
		    }
		}
	    }
	}

	/* One more check for intersections between a curved segment and a */
	/* straight feature. Imagine a curve intersected by two bars, like in a Euro */
	/* glyph. Very probably we will get two chunks, one controlling the uppest   */
	/* two points of intersection, and another the lowest two, and most probably */
	/* these two chunks will get merged into a single stem (so this stem will    */
	/* even get an exactly vertical vector). Yet we don't need this stem because */
	/* there is already a stem controlling the middle of the curve (between two  */
	/* bars).*/
	else if ( stem->chunk_cnt == 2 && 
	    (( stem->chunks[0].stub & 7 && stem->chunks[1].stub & 6 ) ||
	     ( stem->chunks[0].stub & 6 && stem->chunks[1].stub & 7 ))) {
	    for ( j=0; j<gd->stemcnt; ++j) {
		stem1 = &gd->stems[j];
		if ( !stem1->toobig && StemsWouldConflict( stem,stem1 ))
	    break;
	    }

	    if ( j < gd->stemcnt )
		stem->toobig = true;
	}
    }

    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( IsUnitHV( &stem->unit,true ))
    continue;

	/* If a diagonal stem doesn't have at least 2 points assigned to   */
	/* each edge, then we probably can't instruct it. However we don't */
	/* disable stems which have just one point on each side, if those  */
	/* points are inflection points, as such stems may be useful for   */
	/* metafont routines */
	if ( stem->lpcnt < 2 || stem->rpcnt < 2 ) {
	    lpd = rpd = NULL;
	    for ( j=0; j<stem->chunk_cnt && lpd == NULL && rpd == NULL; j++ ) {
		chunk = &stem->chunks[j];
		if ( chunk->l != NULL ) lpd = chunk->l;
		if ( chunk->r != NULL ) rpd = chunk->r;
	    }
	    if (lpd == NULL || rpd == NULL ||
		!IsInflectionPoint( gd,lpd ) || !IsInflectionPoint( gd,rpd ) || stem->clen < stem->width )
		stem->toobig = 2;
	} else if ( stem->activecnt >= stem->chunk_cnt )
	    stem->toobig = 2;
    }

    /* When using preexisting stem data, occasionally we can get two slightly      */
    /* different stems (one predefined, another recently detected) with nearly     */
    /* parallel vectors, sharing some points at both sides. Attempting to instruct */
    /* them both would lead to very odd effects. So we must disable one */
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( !stem->positioned || IsUnitHV( &stem->unit,true ))
    continue;
	
	for ( j=0; j<gd->stemcnt; ++j ) {
	    tstem = &gd->stems[j];
	    if ( tstem == stem || tstem->toobig || !UnitsParallel( &stem->unit,&tstem->unit,false ))
	continue;
	    
	    ltick = false; rtick = false;
	    for ( k=0; k<stem->chunk_cnt && ( !ltick || !rtick ); k++ ) {
		chunk =  &stem->chunks[k];
		
		if ( chunk->l != NULL &&
		    IsStemAssignedToPoint( chunk->l,stem ,chunk->lnext ) != -1 &&
		    IsStemAssignedToPoint( chunk->l,tstem,chunk->lnext ) != -1 )
		    ltick = true;
		if ( chunk->r != NULL &&
		    IsStemAssignedToPoint( chunk->r,stem ,chunk->rnext ) != -1 &&
		    IsStemAssignedToPoint( chunk->r,tstem,chunk->rnext ) != -1 )
		    rtick = true;
	    }
	    if ( ltick && rtick ) tstem->toobig = 2;
	}
    }
}

static int StemPointOnDiag( struct glyphdata *gd,struct stemdata *stem,
    struct pointdata *pd ) {
    
    struct stemdata *tstem;
    int i, is_next, stemcnt;
    
    if ( gd->only_hv || pd->colinear )
return( false );
    
    is_next = IsStemAssignedToPoint( pd,stem,false ) != -1;
    stemcnt = ( is_next ) ? pd->nextcnt : pd->prevcnt;
    
    for ( i=0; i<stemcnt; i++ ) {
	tstem = ( is_next ) ? pd->nextstems[i] : pd->prevstems[i];
	if ( !IsUnitHV( &tstem->unit,true ) &&
	    tstem->lpcnt >= 2 && tstem->rpcnt >=2 )
return( true );
    }
return( false );
}

static void FindRefPointsExisting( struct glyphdata *gd,struct stemdata *stem ) {
    int i;
    int pos, lbase, rbase, is_x;
    struct stem_chunk *chunk;
    struct pointdata *pd;

    is_x = (int) rint( stem->unit.y );
    lbase = ((real *) &stem->left.x)[!is_x];
    rbase = ((real *) &stem->right.x)[!is_x];

    for ( i=0; i<stem->chunk_cnt; ++i ) {
	chunk = &stem->chunks[i];

	if ( chunk->ltick ) {
	    pd = chunk->l;
	    pos = ((real *) &pd->sp->me.x)[!is_x];
	    if ( pos == lbase ) {
		pd->value++;
		if ( pd->sp->ptindex < gd->realcnt )
		    pd->value++;
		if ( StemPointOnDiag( gd,stem,pd ))
		    pd->value++;
	    }
	}

	if ( chunk->rtick ) {
	    pd = chunk->r;
	    pos = ((real *) &pd->sp->me.x)[!is_x];
	    if ( pos == rbase ) {
		pd->value++;
		if ( pd->sp->ptindex < gd->realcnt )
		    pd->value++;
		if ( StemPointOnDiag( gd,stem,pd ))
		    pd->value++;
	    }
	}
    }
}

static void FindRefPointsNew( struct glyphdata *gd,struct stemdata *stem ) {
    int i, j;
    int pos, lpos, rpos, testpos, is_x;
    int lval, rval;
    struct stem_chunk *chunk;
    struct pointdata *lmost1, *lmost2, *rmost1, *rmost2;
    double llen, prevllen, rlen, prevrlen;
    SplinePoint *sp, *tsp;
    uint8 *lextr, *rextr;

    is_x = (int) rint( stem->unit.y );
    lpos = ((real *) &stem->left.x)[!is_x];
    rpos = ((real *) &stem->right.x)[!is_x];

    lmost1 = rmost1 = lmost2 = rmost2 = NULL;
    llen = prevllen = rlen = prevrlen = 0;
    for ( i=0; i<stem->chunk_cnt; ++i ) {
	chunk = &stem->chunks[i];

	if ( chunk->ltick ) {
	    sp = chunk->l->sp;
	    pos = ((real *) &sp->me.x)[!is_x];
	    lval = 0;
	    for ( j=0; j<i; j++ ) if ( stem->chunks[j].ltick ) {
		tsp = stem->chunks[j].l->sp;
		testpos = ((real *) &tsp->me.x)[!is_x];
		if ( pos == testpos ) {
		    lval = stem->chunks[j].l->value;
		    stem->chunks[j].l->value++;
		    /* An additional bonus for points which form together */
		    /* a longer stem segment */
		    if ( sp->next->to == tsp || sp->prev->from == tsp ) {
			llen = fabs(( sp->me.x - tsp->me.x )*stem->unit.x +
				    ( sp->me.y - tsp->me.y )*stem->unit.y );
			if ( llen > prevllen ) {
			    lmost1 = stem->chunks[j].l;
			    lmost2 = chunk->l;
			    prevllen = llen;
			}
		    }
		}
	    }
	    chunk->l->value = lval+1;

	    if ( lval == 0 &&
		( stem->lmin - ( pos - lpos ) > -dist_error_hv ) &&
		( stem->lmax - ( pos - lpos ) < dist_error_hv ))
		chunk->l->value++;
	}

	if ( chunk->rtick ) {
	    sp = chunk->r->sp;
	    pos = ((real *) &sp->me.x)[!is_x];
	    rval = 0;
	    for ( j=0; j<i; j++ ) if ( stem->chunks[j].rtick ) {
		tsp = stem->chunks[j].r->sp;
		testpos = ((real *) &tsp->me.x)[!is_x];
		if ( pos == testpos ) {
		    rval = stem->chunks[j].r->value;
		    stem->chunks[j].r->value++;
		    if ( sp->next->to == tsp || sp->prev->from == tsp ) {
			rlen = fabs(( sp->me.x - tsp->me.x )*stem->unit.x +
				    ( sp->me.y - tsp->me.y )*stem->unit.y );
			if ( rlen > prevrlen ) {
			    rmost1 = stem->chunks[j].r;
			    rmost2 = chunk->r;
			    prevrlen = rlen;
			}
		    }
		}
	    }
	    chunk->r->value = rval+1;

	    if ( rval == 0 &&
		( stem->rmin - ( pos - rpos ) > -dist_error_hv ) &&
		( stem->rmax - ( pos - rpos ) < dist_error_hv ))
		chunk->r->value++;
	}
    }
    if ( lmost1 != NULL && lmost2 != NULL ) {
	lmost1->value++; lmost2->value++;
    }
    if ( rmost1 != NULL && rmost2 != NULL ) {
	rmost1->value++; rmost2->value++;
    }
    
    /* Extrema points get an additional value bonus. This should     */
    /* prevent us from preferring wrong points for stems controlling */
    /* curved segments */
    /* Third pass to assign bonuses to extrema points (especially    */
    /* to those extrema which are opposed to another extremum point) */
    for ( i=0; i<stem->chunk_cnt; ++i ) {
	chunk = &stem->chunks[i];
	if ( chunk->ltick ) {
	    lextr = ( is_x ) ? &chunk->l->x_extr : &chunk->l->y_extr;
	    if ( *lextr ) chunk->l->value++;
	}
	if ( chunk->rtick ) {
	    rextr = ( is_x ) ? &chunk->r->x_extr : &chunk->r->y_extr;
	    if ( *rextr ) chunk->r->value++;
	}

	if ( chunk->ltick && chunk->rtick ) {
	    lextr = ( is_x ) ? &chunk->l->x_extr : &chunk->l->y_extr;
	    rextr = ( is_x ) ? &chunk->r->x_extr : &chunk->r->y_extr;
	    if ( *lextr && *rextr ) {
		chunk->l->value++;
		chunk->r->value++;
	    }
	}
    }
}

static void NormalizeStem( struct glyphdata *gd,struct stemdata *stem ) {
    int i;
    int is_x, lval, rval, val, lset, rset, best;
    double loff=0, roff=0;
    BasePoint lold, rold;
    SplinePoint *lbest, *rbest;
    struct stem_chunk *chunk;
    
    /* First sort the stem chunks by their coordinates */
    if ( IsUnitHV( &stem->unit,true )) {
	qsort( stem->chunks,stem->chunk_cnt,sizeof( struct stem_chunk ),chunk_cmp );
	is_x = (int) rint( stem->unit.y );

	/* For HV stems we have to check all chunks once more in order */
	/* to figure out "left" and "right" positions most typical */
	/* for this stem. We perform this by assigning a value to */
	/* left and right side of this chunk. */

	/* First pass to determine some point properties necessary */
	/* for subsequent operations */
	for ( i=0; i<stem->chunk_cnt; ++i ) {
	    chunk = &stem->chunks[i];
	    if ( chunk->ltick )
		/* reset the point's "value" to zero */
		chunk->l->value = 0;
	    if ( chunk->rtick )
		chunk->r->value = 0;
	}

	/* Second pass to check which positions relative to stem edges are */
	/* most common for this stem. Each position which repeats */
	/* more than once gets a plus 1 value bonus */
	if ( stem->positioned ) FindRefPointsExisting( gd,stem );
	else FindRefPointsNew( gd,stem );

	best = -1; val = 0;
	for ( i=0; i<stem->chunk_cnt; ++i ) {
	    chunk = &stem->chunks[i];
	    lval = ( chunk->l != NULL ) ? chunk->l->value : 0;
	    rval = ( chunk->r != NULL ) ? chunk->r->value : 0;
	    if ((( chunk->l != NULL && chunk->l->value > 0 && 
		GetValidPointDataIndex( gd,chunk->l->sp,stem ) != -1 ) ||
		( stem->ghost && stem->width == 21 )) && 
		(( chunk->r != NULL && chunk->r->value > 0 &&
		GetValidPointDataIndex( gd,chunk->r->sp,stem ) != -1 ) ||
		( stem->ghost && stem->width == 20 )) && lval + rval > val ) {
		
		best = i;
		val = lval + rval;
	    }
	}
	if ( best > -1 ) {
	    if ( !stem->ghost || stem->width == 20 ) {
		lold = stem->left;
		lbest = stem->chunks[best].l->sp;
		stem->left = lbest->me;
		stem->leftidx = GetValidPointDataIndex( gd,lbest,stem );

		/* Now assign "left" and "right" properties of the stem     */
		/* to point coordinates taken from the most "typical" chunk */
		/* of this stem. We also have to recalculate stem width and */
		/* left/right offset values */
		loff = ( stem->left.x - lold.x ) * stem->unit.y -
		       ( stem->left.y - lold.y ) * stem->unit.x;
		stem->lmin -= loff; stem->lmax -= loff;
	    }
	    if ( !stem->ghost || stem->width == 21 ) {
		rold = stem->right;
		rbest = stem->chunks[best].r->sp;
		stem->right = rbest->me;
		stem->rightidx = GetValidPointDataIndex( gd,rbest,stem );
		roff = ( stem->right.x - rold.x ) * stem->unit.y -
		       ( stem->right.y - rold.y ) * stem->unit.x;
		stem->rmin -= roff; stem->rmax -= roff;
	    }
	    if ( !stem->ghost )
		stem->width = ( stem->right.x - stem->left.x ) * stem->unit.y -
			      ( stem->right.y - stem->left.y ) * stem->unit.x;
	} else {
	    for ( i=0; i<stem->chunk_cnt; ++i ) {
		chunk = &stem->chunks[i];
		if ( chunk->l != NULL && ( !stem->ghost || stem->width == 20 )) {
		    stem->leftidx = GetValidPointDataIndex( gd,chunk->l->sp,stem );
		}
		if ( chunk->r != NULL && ( !stem->ghost || stem->width == 21 )) {
		    stem->rightidx = GetValidPointDataIndex( gd,chunk->r->sp,stem );
		}
	    }
	}
    } else {
	qsort( stem->chunks,stem->chunk_cnt,sizeof( struct stem_chunk ),chunk_cmp );
	lset = false; rset = false;
	/* Search for a pair of points whose vectors are really parallel. */
	/* This check is necessary because a diagonal stem can start from */
	/* a feature termination, and our checks for such terminations    */
	/* are more "liberal" than in other cases. However we don't want  */
	/* considering such a pair of points basic for this stem */
	for ( i=0; i<stem->chunk_cnt; ++i ) {
	    chunk = &stem->chunks[i];
	    BasePoint *lu, *ru;
	    if ( chunk->l != NULL && chunk->r != NULL ) {
		lu = chunk->lnext ? &chunk->l->nextunit : &chunk->l->prevunit;
		ru = chunk->rnext ? &chunk->r->nextunit : &chunk->r->prevunit;
		if ( UnitsParallel( lu,ru,true )) {
		    loff =  ( chunk->l->sp->me.x - stem->left.x )*stem->l_to_r.x +
			    ( chunk->l->sp->me.y - stem->left.y )*stem->l_to_r.y;
		    roff =  ( chunk->r->sp->me.x - stem->right.x )*stem->l_to_r.x +
			    ( chunk->r->sp->me.y - stem->right.y )*stem->l_to_r.y;
		    stem->left = chunk->l->sp->me;
		    stem->right = chunk->r->sp->me;
		    RecalcStemOffsets( stem,&stem->unit,loff != 0,roff != 0 );
	break;
		}
	    }
	}
	/* If the above check fails, just select the first point (relatively) */
	/* to the stem direction both at the left and the right edge */
	if ( i == stem->chunk_cnt ) for ( i=0; i<stem->chunk_cnt; ++i ) {
	    chunk = &stem->chunks[i];
	    if ( !lset && chunk->l != NULL ) {
		loff =  ( chunk->l->sp->me.x - stem->left.x )*stem->l_to_r.x +
			( chunk->l->sp->me.y - stem->left.y )*stem->l_to_r.y;
		stem->left = chunk->l->sp->me;
		lset = true;
	    }
	    if ( !rset && chunk->r != NULL ) {
		roff =  ( chunk->r->sp->me.x - stem->right.x )*stem->l_to_r.x +
			( chunk->r->sp->me.y - stem->right.y )*stem->l_to_r.y;
		stem->right = chunk->r->sp->me;
		rset = true;
	    }
	    if ( lset && rset ) {
		RecalcStemOffsets( stem,&stem->unit,loff != 0,roff != 0 );
	break;
	    }
	}
    }
}

static void AssignPointsToBBoxHint( struct glyphdata *gd,DBounds *bounds,
    struct stemdata *stem,int is_v ) {
    
    double min, max, test, left, right;
    double dist, prevdist;
    int i, j;
    int lcnt=0, rcnt=0, closest;
    BasePoint dir;
    SplinePoint **lpoints, **rpoints;
    struct pointdata *pd, *pd1, *pd2;
    
    lpoints = calloc( gd->pcnt,sizeof( SplinePoint *));
    rpoints = calloc( gd->pcnt,sizeof( SplinePoint *));
    dir.x = !is_v; dir.y = is_v;
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	pd = &gd->points[i];
	min = ( is_v ) ? bounds->minx : bounds->miny;
	max = ( is_v ) ? bounds->maxx : bounds->maxy;
	test = ( is_v ) ? pd->base.x : pd->base.y;
	if ( test >= min && test < min + dist_error_hv && (
	    IsCorrectSide( gd,pd,true,is_v,&dir ) || IsCorrectSide( gd,pd,false,is_v,&dir )))
	    lpoints[lcnt++] = pd->sp;
	else if ( test > max - dist_error_hv && test <= max && (
	    IsCorrectSide( gd,pd,true,!is_v,&dir ) || IsCorrectSide( gd,pd,false,!is_v,&dir )))
	    rpoints[rcnt++] = pd->sp;
    }
    if ( lcnt > 0 && rcnt > 0 ) {
	if ( stem == NULL ) {
	    stem = NewStem( gd,&dir,&lpoints[0]->me,&rpoints[0]->me );
	    stem->bbox = true;
	    stem->len = stem->width;
	    stem->leftidx = GetValidPointDataIndex( gd,lpoints[0],stem );
	    stem->rightidx = GetValidPointDataIndex( gd,rpoints[0],stem );
	}
	for ( i=0; i<lcnt; ++i ) {
	    closest = -1;
	    dist = 1e4; prevdist = 1e4;
	    for ( j=0; j<rcnt; ++j ) {
		left = ( is_v ) ? lpoints[i]->me.y : lpoints[i]->me.x;
		right = ( is_v ) ? rpoints[j]->me.y : rpoints[j]->me.x;
		dist = fabs( left - right );
		if ( dist < prevdist ) {
		    closest = j;
		    prevdist = dist;
		}
	    }
	    pd1 = &gd->points[lpoints[i]->ptindex];
	    pd2 = &gd->points[rpoints[closest]->ptindex];
	    AddToStem( gd,stem,pd1,pd2,false,true,4 );
	}
	qsort( stem->chunks,stem->chunk_cnt,sizeof( struct stem_chunk ),chunk_cmp );
    }
    free( lpoints );
    free( rpoints );
}

static void CheckForBoundingBoxHints( struct glyphdata *gd ) {
    /* Adobe seems to add hints at the bounding boxes of glyphs with no hints */
    int i, hv;
    int hcnt=0, vcnt=0; 
    double cw, ch;
    struct stemdata *stem, *hstem=NULL,*vstem=NULL;
    DBounds bounds;
    
    SplineCharFindBounds( gd->sc,&bounds );

    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	hv = IsUnitHV( &stem->unit,true );
	if ( !hv )
    continue;
	if ( stem->toobig ) {
	    if ( stem->left.x == bounds.minx && stem->right.x == bounds.maxx )
		vstem = stem;
	    else if ( stem->right.y == bounds.miny && stem->left.y == bounds.maxy )
		hstem = stem;
    continue;
	}
	if ( hv == 1 ) {
	    if ( stem->bbox ) hstem = stem;
	    else ++hcnt;
	} else if ( hv == 2 ) {
	    if ( stem->bbox ) vstem = stem;
	    else ++vcnt;
	}
    }
    if ( hcnt!=0 && vcnt!=0 && 
	( hstem == NULL || !hstem->positioned ) && 
	( vstem == NULL || !vstem->positioned ))
return;

    ch = bounds.maxy - bounds.miny;
    cw = bounds.maxx - bounds.minx;
    
    if ( ch > 0 && (( hstem != NULL && hstem->positioned ) || 
	( hcnt == 0 && ch < gd->emsize/3 ))) {
	if ( hstem != NULL && hstem->toobig ) hstem->toobig = false;
	AssignPointsToBBoxHint( gd,&bounds,hstem,false );
	if ( hstem != NULL ) NormalizeStem( gd,hstem );
    }
    if ( cw > 0 && (( vstem != NULL && vstem->positioned ) || 
	( vcnt == 0 && cw < gd->emsize/3 ))) {
	if ( vstem != NULL && vstem->toobig ) vstem->toobig = false;
	AssignPointsToBBoxHint( gd,&bounds,vstem,true );
	if ( vstem != NULL ) NormalizeStem( gd,vstem );
    }
}

static struct stemdata *FindOrMakeGhostStem( struct glyphdata *gd,
    SplinePoint *sp,int blue,double width ) {
    int i, j, hasl, hasr;
    struct stemdata *stem=NULL, *tstem;
    struct stem_chunk *chunk;
    BasePoint dir,left,right;
    double min, max;

    dir.x = 1; dir.y = 0;
    for ( i=0; i<gd->stemcnt; ++i ) {
	tstem = &gd->stems[i];
	if ( tstem->blue == blue && tstem->ghost && tstem->width == width ) {
	    stem = tstem;
    break;
	/* If the stem controlling this blue zone is not for a ghost hint,    */
	/* then we check if it has both left and right points, to ensure that */
	/* we don't occasionally assign an additional point to a stem which   */
	/* has already been rejected in favor of another stem */
	} else if ( tstem->blue == blue && !tstem->ghost && !tstem->toobig ) {
	    min = ( width == 20 ) ? tstem->left.y - tstem->lmin - 2*dist_error_hv :
				    tstem->right.y - tstem->rmin - 2*dist_error_hv;
	    max = ( width == 20 ) ? tstem->left.y - tstem->lmax + 2*dist_error_hv :
				    tstem->right.y - tstem->rmax + 2*dist_error_hv;
	    
	    if ( sp->me.y <= min || sp->me.y >= max )
    continue;
	    
	    hasl = false; hasr = false; j = 0;
	    while ( j < tstem->chunk_cnt && ( !hasl || !hasr )) {
		chunk = &tstem->chunks[j];
		if ( chunk->l != NULL && !chunk->lpotential )
		    hasl = true;
		if ( chunk->r != NULL && !chunk->rpotential )
		    hasr = true;
		j++;
	    }
	    if ( hasl && hasr ) {
		stem = tstem;
    break;
	    }
	}
    }

    if ( stem == NULL ) {
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
    
    /* First check if there are points on the same line lying further */
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

static void FigureGhostActive( struct glyphdata *gd,struct stemdata *stem ) {
    int acnt, i;
    real len = 0;
    struct segment *activespace = gd->activespace;
    struct pointdata *valid;
    
    if ( !stem->ghost )
return;

    acnt = 0;
    for ( i=0; i<stem->chunk_cnt; ++i ) {
	valid = ( stem->chunks[i].l != NULL) ? 
	    stem->chunks[i].l : stem->chunks[i].r;
	acnt = AddGhostSegment( valid,acnt,stem->left.x,activespace );
    }
    qsort(activespace,acnt,sizeof(struct segment),segment_cmp);
    acnt = MergeSegments( activespace,acnt );
    stem->activecnt = acnt;
    if ( acnt!=0 ) {
	stem->active = malloc(acnt*sizeof(struct segment));
	memcpy( stem->active,activespace,acnt*sizeof( struct segment ));
    }

    for ( i=0; i<acnt; ++i ) {
	len += stem->active[i].end-stem->active[i].start;
    }
    stem->clen = stem->len = len;
}

static void CheckForGhostHints( struct glyphdata *gd ) {
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
    BlueData *bd = &gd->bd;
    struct stemdata *stem;
    struct stem_chunk *chunk;
    struct pointdata *pd;
    real base;
    int i, j, leftfound, rightfound, has_h, peak, fuzz;
    
    fuzz = gd->fuzz;

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
	if ( IsUnitHV( &stem->unit,true ) != 1)
    continue;
	
	leftfound = rightfound = -1;
	for ( j=0; j<bd->bluecnt; ++j ) {
	    if ( stem->left.y>=bd->blues[j][0]-fuzz && stem->left.y<=bd->blues[j][1]+fuzz )
		leftfound = j;
	    else if ( stem->right.y>=bd->blues[j][0]-fuzz && stem->right.y<=bd->blues[j][1]+fuzz )
		rightfound = j;
	}
	/* Assign value 2 to indicate this stem should be ignored also for TTF instrs */
	if ( leftfound !=-1 && rightfound !=-1 && 
	    ( stem->left.y > 0 && stem->right.y <= 0 ))
	    stem->toobig = 2;
	/* Otherwise mark the stem as controlling a specific blue zone */
	else if ( leftfound != -1 && ( rightfound == -1 || stem->left.y > 0 ))
	    stem->blue = leftfound;
	else if ( rightfound != -1 && ( leftfound == -1 || stem->right.y <= 0 ))
	    stem->blue = rightfound;
    }

    /* Now look and see if we can find any edges which lie in */
    /*  these zones.  Edges which are not currently in hints */
    /* Use the winding number to determine top or bottom */
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	has_h = false;
	for ( j=0; j<gd->points[i].prevcnt; j++ ) {
	    stem = gd->points[i].prevstems[j];
	    if ( !stem->toobig && IsUnitHV( &stem->unit,true ) == 1 ) {
		has_h = true;
	break;
	    }
	}
	for ( j=0; j<gd->points[i].nextcnt; j++ ) {
	    stem = gd->points[i].nextstems[j];
	    if ( !stem->toobig && IsUnitHV( &stem->unit,true ) == 1 ) {
		has_h = true;
	break;
	    }
	}
	if ( has_h )
    continue;
	
	pd = &gd->points[i];
	base = pd->sp->me.y;
	for ( j=0; j<bd->bluecnt; ++j ) {
	    if ( base>=bd->blues[j][0]-fuzz && base<=bd->blues[j][1]+fuzz ) {
		peak = IsSplinePeak( gd,pd,false,false,7 );
		if ( peak > 0 ) {
		    stem = FindOrMakeGhostStem( gd,pd->sp,j,20 );
		    chunk = AddToStem( gd,stem,pd,NULL,2,false,false );
		} else if ( peak < 0 ) {
		    stem = FindOrMakeGhostStem( gd,pd->sp,j,21 );
		    chunk = AddToStem( gd,stem,NULL,pd,2,false,false );
		}
	    }
	}
    }
    
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( !stem->ghost )
    continue;
	NormalizeStem( gd,stem );
	FigureGhostActive( gd,stem );
    }
}

static void MarkDStemCorner( struct glyphdata *gd,struct pointdata *pd ) {
    int x_dir = pd->x_corner;
    int hv, is_l, i, peak, has_stem = false;
    struct stemdata *stem;
    BasePoint left,right,unit;
    
    for ( i=0; i<pd->prevcnt && !has_stem; i++ ) {
	stem = pd->prevstems[i];
	hv = IsUnitHV( &stem->unit,true );
	if ( !stem->toobig && (
	    ( x_dir &&  hv == 1 ) ||
	    ( !x_dir && hv == 2 )))
	    has_stem = true;
    }
    for ( i=0; i<pd->nextcnt && !has_stem; i++ ) {
	stem = pd->nextstems[i];
	hv = IsUnitHV( &stem->unit,true );
	if ( !stem->toobig && (
	    ( x_dir &&  hv == 1 ) ||
	    ( !x_dir && hv == 2 )))
	    has_stem = true;
    }
    if ( has_stem )
return;
    
    peak = IsSplinePeak( gd,pd,x_dir,x_dir,2 );
    unit.x = !x_dir; unit.y = x_dir;
    
    if ( peak > 0 ) {
	left.x = x_dir ? pd->sp->me.x + 21 : pd->sp->me.x;
	right.x = x_dir ? pd->sp->me.x : pd->sp->me.x;
	left.y = x_dir ? pd->sp->me.y : pd->sp->me.y;
	right.y = x_dir ? pd->sp->me.y : pd->sp->me.y - 20;
	
    } else if ( peak < 0 ) {
	left.x = x_dir ? pd->sp->me.x : pd->sp->me.x;
	right.x = x_dir ? pd->sp->me.x - 20 : pd->sp->me.x;
	left.y = x_dir ? pd->sp->me.y : pd->sp->me.y + 21;
	right.y = x_dir ? pd->sp->me.y : pd->sp->me.y;
    }
    is_l = IsCorrectSide( gd,pd,true,true,&unit );
    for ( i=0; i<gd->stemcnt; i++ ) {
	stem = &gd->stems[i];
	if (!stem->toobig && UnitsParallel( &unit,&stem->unit,true ) && 
	    OnStem( stem,&pd->sp->me,is_l ))
    break;
    }
    if ( i == gd->stemcnt ) {
	stem = NewStem( gd,&unit,&left,&right );
	stem->ghost = 2;
    }
    AddToStem( gd,stem,pd,NULL,2,false,false );
}

static void MarkDStemCorners( struct glyphdata *gd ) {
    struct stemdata *stem;
    struct stem_chunk *schunk, *echunk;
    int i;
    
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( stem->toobig || IsUnitHV( &stem->unit,true ))
    continue;
        
	schunk = &stem->chunks[0];
	echunk = &stem->chunks[stem->chunk_cnt - 1];
	
	if ( schunk->l != NULL && schunk->r != NULL && 
	    fabs( schunk->l->base.x - schunk->r->base.x ) > dist_error_hv &&
            fabs( schunk->l->base.y - schunk->r->base.y ) > dist_error_hv && (
	    ( schunk->l->x_corner == 1 && schunk->r->y_corner == 1 ) ||
	    ( schunk->l->y_corner == 1 && schunk->r->x_corner == 1 ))) {
	    MarkDStemCorner( gd,schunk->l );
	    MarkDStemCorner( gd,schunk->r );
	}
	if ( echunk->l != NULL && echunk->r != NULL &&
	    fabs( echunk->l->base.x - echunk->r->base.x ) > dist_error_hv &&
            fabs( echunk->l->base.y - echunk->r->base.y ) > dist_error_hv && (
	    ( echunk->l->x_corner == 1 && echunk->r->y_corner == 1 ) ||
	    ( echunk->l->y_corner == 1 && echunk->r->x_corner == 1 ))) {
	    MarkDStemCorner( gd,echunk->l );
	    MarkDStemCorner( gd,echunk->r );
	}
    }
}

#if GLYPH_DATA_DEBUG
static void DumpGlyphData( struct glyphdata *gd ) {
    int i, j;
    struct stemdata *stem;
    struct linedata *line;
    struct stem_chunk *chunk;

    if ( gd->linecnt > 0 )
	fprintf( stderr, "\nDumping line data for %s\n",gd->sc->name );
    for ( i=0; i<gd->linecnt; ++i ) {
	line = &gd->lines[i];
	fprintf( stderr, "line vector=%.4f,%.4f base=%.2f,%.2f length=%.4f\n", 
	    line->unit.x,line->unit.y,line->online.x,line->online.y,line->length );
	for( j=0; j<line->pcnt;++j ) {
	    fprintf( stderr, "\tpoint num=%d, x=%.2f, y=%.2f, prev=%d, next=%d\n",
		line->points[j]->sp->ttfindex, line->points[j]->sp->me.x,
		line->points[j]->sp->me.y, 
		line->points[j]->prevline==line, line->points[j]->nextline==line );
	}
	fprintf( stderr, "\n" );
    }
    
    if ( gd->stemcnt > 0 )
	fprintf( stderr, "\nDumping stem data for %s\n",gd->sc->name );
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	fprintf( stderr, "stem l=%.2f,%.2f idx=%d r=%.2f,%.2f idx=%d vector=%.4f,%.4f\n\twidth=%.2f chunk_cnt=%d len=%.4f clen=%.4f ghost=%d blue=%d toobig=%d\n\tlmin=%.2f,lmax=%.2f,rmin=%.2f,rmax=%.2f,lpcnt=%d,rpcnt=%d\n",
	    stem->left.x,stem->left.y,stem->leftidx,
	    stem->right.x,stem->right.y,stem->rightidx,
	    stem->unit.x,stem->unit.y,stem->width,
	    stem->chunk_cnt,stem->len,stem->clen,stem->ghost,stem->blue,stem->toobig,
	    stem->lmin,stem->lmax,stem->rmin,stem->rmax,stem->lpcnt,stem->rpcnt );
	for ( j=0; j<stem->chunk_cnt; ++j ) {
	    chunk = &stem->chunks[j];
	    if ( chunk->l!=NULL && chunk->r!=NULL )
		fprintf (stderr, "\tchunk l=%.2f,%.2f potential=%d r=%.2f,%.2f potential=%d stub=%d\n",
		    chunk->l->sp->me.x, chunk->l->sp->me.y, chunk->lpotential,
		    chunk->r->sp->me.x, chunk->r->sp->me.y, chunk->rpotential, chunk->stub );
	    else if ( chunk->l!=NULL )
		fprintf (stderr, "\tchunk l=%.2f,%.2f potential=%d\n",
		    chunk->l->sp->me.x, chunk->l->sp->me.y, chunk->lpotential);
	    else if ( chunk->r!=NULL )
		fprintf (stderr, "\tchunk r=%.2f,%.2f potential=%d\n",
		    chunk->r->sp->me.x, chunk->r->sp->me.y, chunk->rpotential);
	}
	fprintf( stderr, "\n" );
    }

    if ( gd->hbundle != NULL || gd->vbundle != NULL )
	fprintf( stderr, "\nDumping HV stem bundles for %s\n",gd->sc->name );
    if ( gd->hbundle != NULL ) for ( i=0; i<gd->hbundle->cnt; i++ ) {
	stem = gd->hbundle->stemlist[i];
	fprintf( stderr, "H stem l=%.2f,%.2f r=%.2f,%.2f slave=%d\n",
	    stem->left.x,stem->left.y,stem->right.x,stem->right.y,stem->master!=NULL );
	if ( stem->dep_cnt > 0 ) for ( j=0; j<stem->dep_cnt; j++ ) {
	    fprintf( stderr, "\tslave l=%.2f,%.2f r=%.2f,%.2f mode=%c left=%d\n",
		stem->dependent[j].stem->left.x,stem->dependent[j].stem->left.y,
		stem->dependent[j].stem->right.x,stem->dependent[j].stem->right.y,
		stem->dependent[j].dep_type,stem->dependent[j].lbase );
	}
	if ( stem->serif_cnt > 0 ) for ( j=0; j<stem->serif_cnt; j++ ) {
	    fprintf( stderr, "\tserif l=%.2f,%.2f r=%.2f,%.2f ball=%d left=%d\n",
		stem->serifs[j].stem->left.x,stem->serifs[j].stem->left.y,
		stem->serifs[j].stem->right.x,stem->serifs[j].stem->right.y,
		stem->serifs[j].is_ball,stem->serifs[j].lbase );
	}
    }
    fprintf( stderr, "\n" );
    if ( gd->vbundle != NULL ) for ( i=0; i<gd->vbundle->cnt; i++ ) {
	stem = gd->vbundle->stemlist[i];
	fprintf( stderr, "V stem l=%.2f,%.2f r=%.2f,%.2f slave=%d\n",
	    stem->left.x,stem->left.y,stem->right.x,stem->right.y,stem->master!=NULL );
	if ( stem->dep_cnt > 0 ) for ( j=0; j<stem->dep_cnt; j++ ) {
	    fprintf( stderr, "\tslave l=%.2f,%.2f r=%.2f,%.2f mode=%c left=%d\n",
		stem->dependent[j].stem->left.x,stem->dependent[j].stem->left.y,
		stem->dependent[j].stem->right.x,stem->dependent[j].stem->right.y,
		stem->dependent[j].dep_type,stem->dependent[j].lbase );
	}
	if ( stem->serif_cnt > 0 ) for ( j=0; j<stem->serif_cnt; j++ ) {
	    fprintf( stderr, "\tserif l=%.2f,%.2f r=%.2f,%.2f ball=%d left=%d\n",
		stem->serifs[j].stem->left.x,stem->serifs[j].stem->left.y,
		stem->serifs[j].stem->right.x,stem->serifs[j].stem->right.y,
		stem->serifs[j].is_ball,stem->serifs[j].lbase );
	}
	if ( stem->prev_c_m != NULL ) {
	    fprintf( stderr,"\tprev counter master: l=%.2f r=%.2f\n",
		stem->prev_c_m->left.x,stem->prev_c_m->right.x );
	}
	if ( stem->next_c_m != NULL ) {
	    fprintf( stderr,"\tnext counter master: l=%.2f r=%.2f\n",
		stem->next_c_m->left.x,stem->next_c_m->right.x );
	}
    }
    fprintf( stderr, "\n" );

    if ( gd->ibundle != NULL ) for ( i=0; i<gd->ibundle->cnt; i++ ) {
	stem = gd->ibundle->stemlist[i];
	fprintf( stderr, "I stem l=%.2f,%.2f r=%.2f,%.2f slave=%d\n",
	    stem->left.x,stem->left.y,stem->right.x,stem->right.y,stem->master!=NULL );
	if ( stem->dep_cnt > 0 ) for ( j=0; j<stem->dep_cnt; j++ ) {
	    fprintf( stderr, "\tslave l=%.2f,%.2f r=%.2f,%.2f mode=%c left=%d\n",
		stem->dependent[j].stem->left.x,stem->dependent[j].stem->left.y,
		stem->dependent[j].stem->right.x,stem->dependent[j].stem->right.y,
		stem->dependent[j].dep_type,stem->dependent[j].lbase );
	}
	if ( stem->serif_cnt > 0 ) for ( j=0; j<stem->serif_cnt; j++ ) {
	    fprintf( stderr, "\tserif l=%.2f,%.2f r=%.2f,%.2f ball=%d left=%d\n",
		stem->serifs[j].stem->left.x,stem->serifs[j].stem->left.y,
		stem->serifs[j].stem->right.x,stem->serifs[j].stem->right.y,
		stem->serifs[j].is_ball,stem->serifs[j].lbase );
	}
    }
    fprintf( stderr, "\n" );
}
#endif

static void AssignPointsToStems( struct glyphdata *gd,int startnum,DBounds *bounds ) {
    int i;
    struct pointdata *pd;
    struct stemdata *stem = NULL;
    BasePoint dir;
    
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	pd = &gd->points[i];
	if ( pd->prev_e_cnt > 0 )
	    BuildStem( gd,pd,false,true,true,0 );
	else
	    HalfStemNoOpposite( gd,pd,stem,&pd->prevunit,false );

	if ( pd->next_e_cnt > 0 )
	    BuildStem( gd,pd,true,true,true,0 );
	else
	    HalfStemNoOpposite( gd,pd,stem,&pd->nextunit,true );

	if ( pd->x_corner ) {
	    if ( pd->bothedge!=NULL )
		stem = DiagonalCornerStem( gd,pd,true );
	    dir.x = 0; dir.y = 1;
	    HalfStemNoOpposite( gd,pd,stem,&dir,2 );
	} else if ( pd->y_corner ) {
	    if ( pd->bothedge!=NULL )
		stem = DiagonalCornerStem( gd,pd,true );
	    dir.x = 1; dir.y = 0;
	    HalfStemNoOpposite( gd,pd,stem,&dir,2 );
	}
    }
    gd->lspace = malloc(gd->pcnt*sizeof(struct segment));
    gd->rspace = malloc(gd->pcnt*sizeof(struct segment));
    gd->bothspace = malloc(3*gd->pcnt*sizeof(struct segment));
    gd->activespace = malloc(3*gd->pcnt*sizeof(struct segment));
#if GLYPH_DATA_DEBUG
    fprintf( stderr,"Going to calculate stem active zones for %s\n",gd->sc->name );
#endif
    for ( i=startnum; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	NormalizeStem( gd,stem );
	if ( gd->stems[i].ghost )
	    FigureGhostActive( gd,stem );
	else if ( gd->stems[i].bbox )
	    AssignPointsToBBoxHint( gd,bounds,stem,( stem->unit.y == 1 ));
	else
	    FigureStemActive( gd,&gd->stems[i] );
    }
#if GLYPH_DATA_DEBUG
    DumpGlyphData( gd );
#endif

    free(gd->lspace);		gd->lspace = NULL;
    free(gd->rspace);		gd->rspace = NULL;
    free(gd->bothspace);	gd->bothspace = NULL;
    free(gd->activespace);	gd->activespace = NULL;
}

static void _DStemInfoToStemData( struct glyphdata *gd,DStemInfo *dsi,int *startcnt ) {
    struct stemdata *stem;
    
    if ( gd->stems == NULL ) {
	gd->stems = calloc( 2*gd->pcnt,sizeof( struct stemdata ));
	gd->stemcnt = 0;
    }
    *startcnt = gd->stemcnt;
    while ( dsi != NULL ) {
	stem = NewStem( gd,&dsi->unit,&dsi->left,&dsi->right );
	stem->positioned = true;
	dsi = dsi->next;
    }
}

struct glyphdata *DStemInfoToStemData( struct glyphdata *gd,DStemInfo *dsi ) {
    int startcnt;
    
    if ( dsi == NULL )
return( gd );
    
    _DStemInfoToStemData( gd,dsi,&startcnt );
    AssignPointsToStems( gd,startcnt,NULL );
return( gd );
}

static void _StemInfoToStemData( struct glyphdata *gd,StemInfo *si,DBounds *bounds,int is_v,int *startcnt ) {
    struct stemdata *stem;
    BasePoint dir,left,right;
    
    dir.x = !is_v; dir.y = is_v;
    if ( gd->stems == NULL ) {
	gd->stems = calloc( 2*gd->pcnt,sizeof( struct stemdata ));
	gd->stemcnt = 0;
    }
    *startcnt = gd->stemcnt;

    while ( si != NULL ) {
	left.x = ( is_v ) ? si->start : 0;
	left.y = ( is_v ) ? 0 : si->start + si->width;
	right.x = ( is_v ) ? si->start + si->width : 0;
	right.y = ( is_v ) ? 0 : si->start;
	stem = NewStem( gd,&dir,&left,&right );
	stem->ghost = si->ghost;
	if (( is_v && 
		left.x >= bounds->minx && left.x < bounds->minx + dist_error_hv &&
		right.x > bounds->maxx - dist_error_hv && right.x <= bounds->maxx ) ||
	    ( !is_v && 
		right.y >= bounds->miny && right.y < bounds->miny + dist_error_hv &&
		left.y > bounds->maxy - dist_error_hv && left.y <= bounds->maxy ))
	    stem->bbox = true;
	stem->positioned = true;
	si = si->next;
    }
}

struct glyphdata *StemInfoToStemData( struct glyphdata *gd,StemInfo *si,int is_v ) {
    DBounds bounds;
    int startcnt;
    
    if ( si == NULL )
return( gd );
    
    SplineCharFindBounds( gd->sc,&bounds );
    _StemInfoToStemData( gd,si,&bounds,is_v,&startcnt );

    AssignPointsToStems( gd,startcnt,&bounds );
return( gd );
}

static int ValidConflictingStem( struct stemdata *stem1,struct stemdata *stem2 ) {
    int x_dir = fabs( stem1->unit.y ) > fabs( stem1->unit.x );
    double s1, e1, s2, e2, temp;
    
    s1 = (&stem1->left.x)[!x_dir] - 
	((&stem1->left.x)[x_dir] * (&stem1->unit.x)[!x_dir] )/(&stem1->unit.x)[x_dir];
    e1 = (&stem1->right.x)[!x_dir] - 
	((&stem1->right.x)[x_dir] * (&stem1->unit.x)[!x_dir] )/(&stem1->unit.x)[x_dir];
    s2 = (&stem2->left.x)[!x_dir] - 
	((&stem2->left.x)[x_dir] * (&stem2->unit.x)[!x_dir] )/(&stem2->unit.x)[x_dir];
    e2 = (&stem2->right.x)[!x_dir] - 
	((&stem2->right.x)[x_dir] * (&stem2->unit.x)[!x_dir] )/(&stem2->unit.x)[x_dir];

    if ( s1 > e1 ) {
	temp = s1; s1 = e1; e1 = temp;
    }
    if ( s2 > e2 ) {
	temp = s2; s2 = e2; e2 = temp;
    }
    /* If stems don't overlap, then there is no conflict here */
    if ( s2 >= e1 || s1 >= e2 )
return( false );
    
    /* Stems which have no points assigned cannot be valid masters for    */
    /* other stems (however there is a notable exception for ghost hints) */
    if (( stem1->lpcnt > 0 || stem1->rpcnt > 0 ) && 
	stem2->lpcnt == 0 && stem2->rpcnt == 0 && !stem2->ghost )
return( false );

    /* Bounding box stems are always preferred */
    if ( stem1->bbox && !stem2->bbox )
return( false );

    /* Stems associated with blue zones always preferred to any other stems */
    if ( stem1->blue >=0 && stem2->blue < 0 )
return( false );
    /* Don't attempt to handle together stems, linked to different zones */
    if ( stem1->blue >=0 && stem2->blue >= 0 && stem1->blue != stem2->blue )
return( false );
    /* If both stems are associated with a blue zone, but one of them is for */
    /* a ghost hint, then that stem is preferred */
    if ( stem1->ghost && !stem2->ghost )
return( false );

return( true );
}

static int HasDependentStem( struct stemdata *master,struct stemdata *slave ) {
    int i;
    struct stemdata *tstem;
    
    if ( slave->master != NULL && master->dep_cnt > 0 ) {
	for ( i=0; i<master->dep_cnt; i++ ) {
	    tstem = master->dependent[i].stem;
	    if ( tstem == slave || HasDependentStem( tstem,slave ))
return( true );
	}
    }
return( false );
}

static int PreferEndDep( struct stemdata *stem,
    struct stemdata *smaster,struct stemdata *emaster,char s_type,char e_type ) {
    
    int hv = IsUnitHV( &stem->unit,true );
    double sdist, edist;
    
    if ( !hv )
return( false );
    
    if (( s_type == 'a' && e_type != 'a' ) || ( s_type == 'm' && e_type == 'i' ))
return( false );
    else if (( e_type == 'a' && s_type != 'a' ) || ( e_type == 'm' && s_type == 'i' ))
return( true );
    
    if ( s_type == 'm' && s_type == e_type ) {
	sdist = ( hv==1 ) ? 
	    fabs( smaster->right.y - stem->right.y ) :
	    fabs( smaster->left.x - stem->left.x );
	edist = ( hv==1 ) ? 
	    fabs( emaster->left.y - stem->left.y ) :
	    fabs( emaster->right.x - stem->right.x );
return( edist < sdist );
    } else
return( emaster->clen > smaster->clen );
}

static void LookForMasterHVStem( struct stemdata *stem,BlueData *bd ) {
    struct stemdata *tstem, *smaster=NULL, *emaster=NULL;
    struct stembundle *bundle = stem->bundle;
    double start, end, tstart, tend;
    double ssdist, sedist, esdist, eedist;
    double smin, smax, emin, emax, tsmin, tsmax, temin, temax;
    int is_x, i, link_to_s, stype, etype, allow_s, allow_e;

    is_x = ( bundle->unit.x == 1 );
    if ( is_x ) {
	start = stem->right.y; end = stem->left.y;
	smin = start - stem->rmin - 2*dist_error_hv;
	smax = start - stem->rmax + 2*dist_error_hv;
	emin = end - stem->lmin - 2*dist_error_hv;
	emax = end - stem->lmax + 2* dist_error_hv;
    } else {
	start = stem->left.x; end = stem->right.x;
	smin = start + stem->lmax - 2*dist_error_hv;
	smax = start + stem->lmin + 2*dist_error_hv;
	emin = end + stem->rmax - 2*dist_error_hv;
	emax = end + stem->rmin + 2*dist_error_hv;
    }
    start = ( is_x ) ? stem->right.y : stem->left.x;
    end = ( is_x ) ? stem->left.y : stem->right.x;
    stype = etype = '\0';

    for ( i=0; i<bundle->cnt; i++ ) {
	tstem = bundle->stemlist[i];
	if ( is_x ) {
	    tstart = tstem->right.y; tend = tstem->left.y;
	    tsmin = tstart - tstem->rmin - 2*dist_error_hv;
	    tsmax = tstart - tstem->rmax + 2*dist_error_hv;
	    temin = tend - tstem->lmin - 2*dist_error_hv;
	    temax = tend - tstem->lmax + 2* dist_error_hv;
	} else {
	    tstart = tstem->left.x; tend = tstem->right.x;
	    tsmin = tstart + tstem->lmax - 2*dist_error_hv;
	    tsmax = tstart + tstem->lmin + 2*dist_error_hv;
	    temin = tend + tstem->rmax - 2*dist_error_hv;
	    temax = tend + tstem->rmin + 2*dist_error_hv;
	}
	tstart = ( is_x ) ? tstem->right.y : tstem->left.x;
	tend = ( is_x ) ? tstem->left.y : tstem->right.x;

	/* In this loop we are looking if the given stem has conflicts with */
	/* other stems and if anyone of those conflicting stems should      */
	/* take precedence over it */
	if ( stem == tstem || tend < start || tstart > end || 
	    !ValidConflictingStem( stem,tstem ) || HasDependentStem( stem,tstem ))
    continue;
	/* Usually in case of conflicts we prefer the stem with longer active */
	/* zones. However a stem linked to a blue zone is always preferred to */
	/* a stem which is not, and ghost hints are preferred to any other    */
	/* stems */
	if ( stem->clen > tstem->clen && ValidConflictingStem( tstem,stem ))
    continue;
    
	stem->confl_cnt++;
    
	/* If the master stem is for a ghost hint or both the stems are    */
	/* linked to the same blue zone, then we can link only to the edge */
	/* which fall into the blue zone */
	allow_s = ( !tstem->ghost || tstem->width == 21 ) &&
	    ( stem->blue == -1 || stem->blue != tstem->blue || bd->blues[stem->blue][0] < 0 );
	allow_e = ( !tstem->ghost || tstem->width == 20 ) &&
	    ( stem->blue == -1 || stem->blue != tstem->blue || bd->blues[stem->blue][0] > 0 );
	
	/* Assume there are two stems which have (almost) coincident left edges. */
	/* The hinting technique for this case is to merge all points found on   */
	/* those coincident edges together, position them, and then link to the  */
	/* opposite edges. */
	/* However we don't allow merging if both stems can be snapped to a blue    */
	/* zone, unless their edges are _exactly_ coincident, as shifting features  */
	/* relatively to each other instead of snapping them to the same zone would */
	/* obviously be wrong */
	if ( allow_s && tstart > smin && tstart < smax && start > tsmin && start < tsmax &&
	    ( stem->blue == -1 || RealNear( tstart,start ))) {
	    
	    if ( smaster == NULL || stype != 'a' || smaster->clen < tstem->clen ) {
		smaster = tstem;
		stype = 'a';
	    }
	/* The same case for right edges */
	} else if ( allow_e && tend > emin && tend < emax && end > temin && end < temax &&
	    ( stem->blue == -1 || RealNear( tend,end ))) {

	    if ( emaster == NULL || etype != 'a' || emaster->clen < tstem->clen ) {
		emaster = tstem;
		etype = 'a';
	    }
	    
	/* Nested stems. I first planned to handle them by positioning the      */
	/* narrower stem first, and then linking its edges to the opposed edges */
	/* of the nesting stem. But this works well only in those cases where   */
	/* maintaining the dependent stem width is not important. So now the    */
	/* situations where a narrower or a wider stem can be preferred         */
	/* (because it has longer active zones) are equally possible. In the    */
	/* first case I link to the master stem just one edge of the secondary  */
	/* stem, just like with overlapping stems */
	} else if ( tstart > start && tend < end ) {
	    if ( allow_s && ( smaster == NULL || stype == 'i' ||
		( stype == 'm' && smaster->clen < tstem->clen ))) {

		smaster = tstem;
		stype = 'm';
	    }
	    if ( allow_e && ( emaster == NULL || etype == 'i' ||
		( etype == 'm' && emaster->clen < tstem->clen ))) {

		emaster = tstem;
		etype = 'm';
	    }
	/* However if we have to prefer the nesting stem, we do as with      */
	/* overlapping stems which require interpolations, i. e. interpolate */
	/* one edge and link to another */
	} else if ( tstart < start && tend > end ) {
	    link_to_s = ( allow_s && ( start - tstart < tend - end ));
	    if ( link_to_s && ( smaster == NULL ||
		( stype == 'i' && smaster->clen < tstem->clen ))) {
		smaster = tstem;
		stype = 'i';
	    } else if ( !link_to_s && ( emaster == NULL ||
		( etype == 'i' && emaster->clen < tstem->clen ))) {
		emaster = tstem;
		etype = 'i';
	    }
	/* Overlapping stems. Here we first check all 4 distances between */
	/* 4 stem edges. If the closest distance is between left or right */
	/* edges, then the normal technique (in TrueType) is linking them */
	/* with MDRP without maintaining a minimum distance. Otherwise    */
	/* we interpolate an edge of the "slave" stem between already     */
	/* positioned edges of the "master" stem, and then gridfit it     */
	} else if (( tstart < start && start < tend && tend < end ) ||
	    ( start < tstart && tstart < end && end < tend )) {
		
	    ssdist = fabs( start - tstart );
	    sedist = fabs( start - tend );
	    esdist = fabs( end - tstart );
	    eedist = fabs( end - tend );

	    if ( allow_s && ( !allow_e ||
		( stem->width < tstem->width/3 && ssdist < eedist ) ||
		( ssdist <= eedist && ssdist <= sedist && ssdist <= esdist )) &&
		( smaster == NULL || ( stype == 'i' || 
		( stype == 'm' && smaster->clen < tstem->clen )))) {

		smaster = tstem;
		stype = 'm';
	    } else if ( allow_e && ( !allow_s ||
		( stem->width < tstem->width/3 && eedist < ssdist ) ||
		( eedist <= ssdist && eedist <= sedist && eedist <= esdist )) &&
		( emaster == NULL || ( etype == 'i' || 
		( etype == 'm' && emaster->clen < tstem->clen )))) {

		emaster = tstem;
		etype = 'm';
	    } else if ( allow_s && allow_e && ( smaster == NULL || 
		( stype == 'i' && smaster->clen < tstem->clen )) &&
		sedist <= esdist && sedist <= ssdist && sedist <= eedist ) {

		smaster = tstem;
		stype = 'i';
	    } else if ( allow_s && allow_e && ( emaster == NULL || 
		( etype == 'i' && emaster->clen < tstem->clen )) &&
		esdist <= sedist && esdist <= ssdist && esdist <= eedist ) {

		emaster = tstem;
		etype = 'i';
	    }
	}
    }
    if ( smaster != NULL && emaster != NULL ) {
	if ( PreferEndDep( stem,smaster,emaster,stype,etype ))
	    smaster = NULL;
	else
	    emaster = NULL;
    }
    
    if ( smaster != NULL ) {
	stem->master = smaster;
	if ( smaster->dependent == NULL )
	    smaster->dependent = calloc( bundle->cnt*2,sizeof( struct dependent_stem ));
	smaster->dependent[smaster->dep_cnt].stem = stem;
	smaster->dependent[smaster->dep_cnt].dep_type = stype;
	smaster->dependent[smaster->dep_cnt++].lbase = !is_x;
    } else if ( emaster != NULL ) {
	stem->master = emaster;
	if ( emaster->dependent == NULL )
	    emaster->dependent = calloc( bundle->cnt*2,sizeof( struct dependent_stem ));
	emaster->dependent[emaster->dep_cnt  ].stem = stem;
	emaster->dependent[emaster->dep_cnt  ].dep_type = etype;
	emaster->dependent[emaster->dep_cnt++].lbase = is_x;
    }
}

/* If a stem has been considered depending from another stem which in   */
/* its turn has its own "master", and the first stem doesn't conflict   */
/* with the "master" of the stem it overlaps (or any other stems), then */
/* this dependency is unneeded and processing it in the autoinstructor  */
/* can even lead to undesired effects. Unfortunately we can't prevent   */
/* detecting such dependecies in LookForMasterHVStem(), because we      */
/* need to know the whole stem hierarchy first. So look for undesired   */
/* dependencies and clean them now */
static void ClearUnneededDeps( struct stemdata *stem ) {
    struct stemdata *master;
    int i, j;
    
    if ( stem->confl_cnt == 1 && 
	( master = stem->master ) != NULL && master->master != NULL ) {
	
	stem->master = NULL;
	for ( i=j=0; i<master->dep_cnt; i++ ) {
	    if ( j<i )
		memcpy( &master->dependent[i-1],&master->dependent[i],
		    sizeof( struct dependent_stem ));
	    if ( master->dependent[i].stem != stem ) j++;
	}
	(master->dep_cnt)--;
    }
}

static void GDBundleStems( struct glyphdata *gd, int maxtoobig, int needs_deps ) {
    struct stemdata *stem, *tstem;
    int i, j, k, hv, hasl, hasr, stem_cnt;
    struct pointdata *lpd, *rpd;
    double dmove;
    DBounds bounds;
    
    /* Some checks for undesired stems which we couldn't do earlier  */
    
    /* First filter out HV stems which have only "potential" points  */
    /* on their left or right edge. Such stems aren't supposed to be */
    /* used for PS hinting, so we mark them as "too big" */
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	hasl = false; hasr = false;
	
	if ( IsUnitHV( &stem->unit,true ) && 
	    !stem->toobig && !stem->ghost && !stem->positioned ) {
	    for ( j=0; j<stem->chunk_cnt && ( !hasl || !hasr ); ++j ) {
		if ( stem->chunks[j].l!=NULL && !stem->chunks[j].lpotential ) 
		    hasl = true;
		if ( stem->chunks[j].r!=NULL && !stem->chunks[j].rpotential ) 
		    hasr = true;
	    }
	    if ( !hasl || !hasr )
		stem->toobig = true;
	}
    }

    /* Filter out HV stems which have both their edges controlled by */
    /* other, narrower HV stems */
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	hv = IsUnitHV( &stem->unit,true );

	if ( IsUnitHV( &stem->unit,true )) {
	    hasl = hasr = false;
	    for ( j=0; j<stem->chunk_cnt; ++j ) {
		lpd = stem->chunks[j].l;
		rpd = stem->chunks[j].r;
		if ( lpd != NULL ) {
		    stem_cnt = ( stem->chunks[j].lnext ) ? lpd->nextcnt : lpd->prevcnt;
		    for ( k=0; k<stem_cnt; k++ ) {
			tstem = ( stem->chunks[j].lnext ) ? 
			    lpd->nextstems[k] : lpd->prevstems[k];
			/* Used to test tstem->toobig <= stem->toobig, but got into troubles with */
			/* a weird terminal stem preventing a ball terminal from being properly detected, */
			/* because both the stems initially have toobig == 1. */
			/* See the "f" from Heuristica-Italic */
			if ( tstem != stem && 
			    !tstem->toobig && tstem->positioned >= stem->positioned &&
			    tstem->width < stem->width && hv == IsUnitHV( &tstem->unit,true )) {
			    hasl = true;
		    break;
			}
		    }
		}
		if ( rpd != NULL ) {
		    stem_cnt = ( stem->chunks[j].rnext ) ? rpd->nextcnt : rpd->prevcnt;
		    for ( k=0; k<stem_cnt; k++ ) {
			tstem = ( stem->chunks[j].rnext ) ? 
			    rpd->nextstems[k] : rpd->prevstems[k];
			if ( tstem != stem && 
			    !tstem->toobig && tstem->positioned >= stem->positioned &&
			    tstem->width < stem->width && hv == IsUnitHV( &tstem->unit,true )) {
			    hasr = true;
		    break;
			}
		    }
		}
		if ( hasl && hasr ) {
		    stem->toobig = 2;
	    break;
		}
	    }
	}
    }
    
    gd->hbundle = calloc( 1,sizeof( struct stembundle ));
    gd->hbundle->stemlist = calloc( gd->stemcnt,sizeof( struct stemdata *));
    gd->hbundle->unit.x = 1; gd->hbundle->unit.y = 0;
    gd->hbundle->l_to_r.x = 0; gd->hbundle->l_to_r.y = -1;

    gd->vbundle = calloc( 1,sizeof( struct stembundle ));
    gd->vbundle->stemlist = calloc( gd->stemcnt,sizeof( struct stemdata *));
    gd->vbundle->unit.x = 0; gd->vbundle->unit.y = 1;
    gd->vbundle->l_to_r.x = 1; gd->vbundle->l_to_r.y = 0;
    
    if ( gd->has_slant && !gd->only_hv ) {
	SplineCharFindBounds( gd->sc,&bounds );
	
	gd->ibundle = calloc( 1,sizeof( struct stembundle ));
	gd->ibundle->stemlist = calloc( gd->stemcnt,sizeof( struct stemdata *));
	gd->ibundle->unit.x = gd->slant_unit.x; 
	gd->ibundle->unit.y = gd->slant_unit.y;
	gd->ibundle->l_to_r.x = -gd->ibundle->unit.y; 
	gd->ibundle->l_to_r.y = gd->ibundle->unit.x;
    }

    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	if ( stem->toobig > maxtoobig )
    continue;
	hv = IsUnitHV( &stem->unit,true );
	
	if ( hv == 1 ) {
	    gd->hbundle->stemlist[(gd->hbundle->cnt)++] = stem;
	    stem->bundle = gd->hbundle;
	} else if ( hv == 2 ) {
	    gd->vbundle->stemlist[(gd->vbundle->cnt)++] = stem;
	    stem->bundle = gd->vbundle;
	} else if ( gd->has_slant && !gd->only_hv &&
	    RealNear( stem->unit.x,gd->slant_unit.x ) &&
	    RealNear( stem->unit.y,gd->slant_unit.y )) {
	    
	    /* Move base point coordinates to the baseline to simplify */
	    /* stem ordering and positioning relatively to each other  */
	    stem->left.x -= (( stem->left.y - bounds.miny ) * stem->unit.x )/stem->unit.y;
	    stem->right.x -= (( stem->right.y - bounds.miny ) * stem->unit.x )/stem->unit.y;
	    dmove = ( stem->left.y - bounds.miny ) / stem->unit.y;
	    stem->left.y = stem->right.y = bounds.miny;
	    for ( j=0; j<stem->activecnt; j++ ) {
		stem->active[j].start += dmove;
		stem->active[j].end += dmove;
	    }

	    gd->ibundle->stemlist[(gd->ibundle->cnt)++] = stem;
	    stem->bundle = gd->ibundle;
	    stem->italic = true;
	}
    }
    qsort( gd->hbundle->stemlist,gd->hbundle->cnt,sizeof( struct stemdata *),stem_cmp );
    qsort( gd->vbundle->stemlist,gd->vbundle->cnt,sizeof( struct stemdata *),stem_cmp );
    if ( gd->has_slant && !gd->only_hv )
	qsort( gd->ibundle->stemlist,gd->ibundle->cnt,sizeof( struct stemdata *),stem_cmp );
    
    if ( !needs_deps )
return;
    for ( i=0; i<gd->hbundle->cnt; i++ )
	LookForMasterHVStem( gd->hbundle->stemlist[i],&gd->bd );
    for ( i=0; i<gd->hbundle->cnt; i++ )
	ClearUnneededDeps( gd->hbundle->stemlist[i] );
    for ( i=0; i<gd->vbundle->cnt; i++ )
	LookForMasterHVStem( gd->vbundle->stemlist[i],&gd->bd );
    for ( i=0; i<gd->vbundle->cnt; i++ )
	ClearUnneededDeps( gd->vbundle->stemlist[i] );
}

static void AddSerifOrBall( struct glyphdata *gd,
    struct stemdata *master,struct stemdata *slave,int lbase,int is_ball ) {
    
    struct dependent_serif *tserif;
    struct pointdata *spd, *bpd=NULL;
    double width, min, max;
    int i, j, refidx, scnt, next;
    
    if ( lbase ) {
	width = fabs(
		( slave->right.x - master->left.x ) * master->unit.y -
		( slave->right.y - master->left.y ) * master->unit.x );
	max = width + slave->rmin + 2*dist_error_hv;
	min = width + slave->rmax - 2*dist_error_hv;
    } else {
	width = fabs(
		( master->right.x - slave->left.x ) * master->unit.y -
		( master->right.y - slave->left.y ) * master->unit.x );
	max = width - slave->lmax + 2*dist_error_hv;
	min = width - slave->lmin - 2*dist_error_hv;
    }
    
    scnt = master->serif_cnt;
    for ( i=0; i<scnt; i++ ) {
	tserif = &master->serifs[i];
	if ( tserif->stem == slave && tserif->lbase == lbase )
    break;
	else if ( tserif->width > min && tserif->width < max && tserif->lbase == lbase ) {
	    for ( j=0; j<slave->chunk_cnt; j++ ) {
		spd = ( lbase ) ? slave->chunks[j].r : slave->chunks[j].l;
		next = ( lbase ) ? slave->chunks[j].rnext : slave->chunks[j].lnext;
		if ( spd != NULL && IsStemAssignedToPoint( spd,tserif->stem,next ) == -1 )
		    AddToStem( gd,tserif->stem,spd,NULL,next,false,false );
	    }
    break;
	}
    }
    if ( i<master->serif_cnt )
return;
    
    refidx = ( lbase ) ? master->leftidx : master->rightidx;
    if ( refidx != -1 ) bpd = &gd->points[refidx];
    master->serifs = realloc(
	master->serifs,( scnt+1 )*sizeof( struct dependent_serif ));
    master->serifs[scnt].stem = slave;
    master->serifs[scnt].width = width;
    master->serifs[scnt].lbase = lbase;
    master->serifs[scnt].is_ball = is_ball;
    master->serif_cnt++;
    
    /* Mark the dependent stem as related with a bundle, although it */
    /* is not listed in that bundle itself */
    slave->bundle = master->bundle;
}

static int IsBall( struct glyphdata *gd,
    struct pointdata *pd,struct stemdata *master,int lbase ) {
    
    double max, min, dot, coord;
    BasePoint *lbp, *rbp, *dir;
    Spline *test;
    struct pointdata *nbase, *pbase, *tpd;
    struct stem_chunk *chunk;
    int i, is_x, peak_passed;
    
    if ( pd == NULL || ( pd->x_extr != 1 && pd->y_extr != 1 ))
return( false );

    is_x = ( IsUnitHV( &master->unit,true ) == 1 );
    lbp = ( lbase ) ? &master->left : &pd->base;
    rbp = ( lbase ) ? &pd->base : &master->right;
    min = ( is_x ) ? rbp->y : lbp->x;
    max = ( is_x ) ? lbp->y : rbp->x;
    
    peak_passed = false;
    nbase = pbase = NULL;
    test = pd->sp->next;
    dir = &pd->nextunit;

    if ( test != NULL ) do {
	tpd = &gd->points[test->to->ptindex];
	if ( IsStemAssignedToPoint( tpd,master,true ) != -1 ) {
	    nbase = tpd;
    break;
	}
	coord = ( is_x ) ? tpd->base.y : tpd->base.x;
	dot = tpd->nextunit.x * dir->x + tpd->nextunit.y * dir->y;
	if ( dot == 0 && !peak_passed ) {
	    dir = &tpd->nextunit;
	    dot = 1.0;
	    peak_passed = true;
	}
	test = test->to->next;
    } while ( test != NULL && test != pd->sp->next && dot > 0 &&
	coord >= min && coord <= max );

    peak_passed = false;
    test = pd->sp->prev;
    dir = &pd->prevunit;
    if ( test != NULL ) do {
	tpd = &gd->points[test->from->ptindex];
	if ( IsStemAssignedToPoint( tpd,master,false ) != -1 ) {
	    pbase = tpd;
    break;
	}
	coord = ( is_x ) ? tpd->base.y : tpd->base.x;
	dot = tpd->prevunit.x * dir->x + tpd->prevunit.y * dir->y;
	if ( dot == 0 && !peak_passed ) {
	    dir = &tpd->prevunit;
	    dot = 1.0;
	    peak_passed = true;
	}
	test = test->from->prev;
    } while ( test != NULL && test != pd->sp->prev && dot > 0 &&
	coord >= min && coord <= max );

    if ( nbase != NULL && pbase != NULL ) {
	for ( i=0; i<master->chunk_cnt; i++ ) {
	    chunk = &master->chunks[i];
	    if (( chunk->l == nbase && chunk->r == pbase ) ||
		( chunk->l == pbase && chunk->r == nbase ))
return( true );
	}
    }
return( false );
}

static void GetSerifData( struct glyphdata *gd,struct stemdata *stem ) {
    int i, j, is_x, stem_cnt;
    int snext, enext, eidx, allow_s, allow_e, s_ball, e_ball;
    struct stem_chunk *chunk;
    struct stemdata *tstem, *smaster=NULL, *emaster=NULL;
    struct pointdata *spd, *epd;
    struct stembundle *bundle;
    double start, end, tstart, tend, smend, emstart;
    
    is_x = ( IsUnitHV( &stem->unit,true ) == 1 );
    bundle = ( is_x ) ? gd->hbundle : gd->vbundle;
    start = ( is_x ) ? stem->right.y : stem->left.x;
    end = ( is_x ) ? stem->left.y : stem->right.x;

    allow_s = allow_e = true;
    s_ball = e_ball = 0;
    for ( i=0; i<stem->chunk_cnt && ( allow_s == true || allow_e == true ); i++ ) {
	chunk = &stem->chunks[i];
	spd = ( is_x ) ? chunk->r : chunk->l;
	snext = ( is_x ) ? chunk->rnext : chunk->lnext;
	epd = ( is_x ) ? chunk->l : chunk->r;
	enext = ( is_x ) ? chunk->lnext : chunk->rnext;
	
	if ( spd != NULL && allow_e ) {
	    stem_cnt = ( snext ) ? spd->nextcnt : spd->prevcnt;
	    for ( j=0; j<stem_cnt; j++ ) {
		tstem = ( snext ) ? spd->nextstems[j] : spd->prevstems[j];
		if (RealNear( tstem->unit.x,stem->unit.x ) && RealNear( tstem->unit.y,stem->unit.y ) &&
		    !tstem->toobig ) {
		    chunk->is_ball = e_ball = IsBall( gd,epd,tstem,!is_x );
		    if ( e_ball ) {
			chunk->ball_m = tstem;
			emaster = tstem;
			emstart = ( is_x ) ? tstem->right.y : tstem->left.x;
		    }
		    allow_s = false;
		}
	    }
	    
	}
	if ( epd != NULL && allow_s ) {
	    stem_cnt = ( enext ) ? epd->nextcnt : epd->prevcnt;
	    for ( j=0; j<stem_cnt; j++ ) {
		tstem = ( enext ) ? epd->nextstems[j] : epd->prevstems[j];
		if (tstem->unit.x == stem->unit.x && tstem->unit.y == stem->unit.y &&
		    !tstem->toobig ) {
		    chunk->is_ball = s_ball = IsBall( gd,spd,tstem,is_x );
		    if ( s_ball ) {
			chunk->ball_m = tstem;
			smaster = tstem;
			smend = ( is_x ) ? tstem->left.y : tstem->right.x;
		    }
		    allow_e = false;
		}
	    }
	    
	}
    }
    
    for ( i=0; i<bundle->cnt; i++ ) {
	tstem = bundle->stemlist[i];
	if (tstem->unit.x != stem->unit.x || tstem->unit.y != stem->unit.y ||
	    tstem->toobig || tstem->width >= stem->width )
    continue;
	    
	tstart = ( is_x ) ? tstem->right.y : tstem->left.x;
	tend = ( is_x ) ? tstem->left.y : tstem->right.x;

	if ( tstart >= start && tend <= end ) {
	    if ( allow_s && tstart > start ) {
		for ( j=0; j<tstem->chunk_cnt && smaster != tstem; j++ ) {
		    if ( is_x ) {
			spd = tstem->chunks[j].l;
			snext = tstem->chunks[j].lnext;
			eidx = tstem->chunks[j].l_e_idx;
		    } else {
			spd = tstem->chunks[j].r;
			snext = tstem->chunks[j].rnext;
			eidx = tstem->chunks[j].r_e_idx;
		    }
		    if ( spd != NULL && ConnectsAcrossToStem( gd,spd,snext,stem,is_x,eidx ) &&
			( smaster == NULL || smend - start > tend - start )) {
			smaster = tstem;
			smend = tend;
		    }
		}
	    }
	    if ( allow_e && tend < end ) {
		for ( j=0; j<tstem->chunk_cnt && emaster != tstem; j++ ) {
		    if ( is_x ) {
			epd = tstem->chunks[j].r;
			enext = tstem->chunks[j].rnext;
			eidx = tstem->chunks[j].r_e_idx;
		    } else {
			epd = tstem->chunks[j].l;
			enext = tstem->chunks[j].lnext;
			eidx = tstem->chunks[j].l_e_idx;
		    }
		    if ( epd != NULL && ConnectsAcrossToStem( gd,epd,enext,stem,!is_x,eidx ) &&
			( emaster == NULL || end - emstart > end - tstart )) {
			emaster = tstem;
			emstart = tstart;
		    }
		}
	    }
	}
    }
    if ( smaster != NULL )
	AddSerifOrBall( gd,smaster,stem,is_x,s_ball );
    if ( emaster != NULL )
	AddSerifOrBall( gd,emaster,stem,!is_x,e_ball );
}

static double ActiveOverlap( struct stemdata *stem1,struct stemdata *stem2 ) {
    int is_x, i, j = 0;
    double base1, base2, s1, e1, s2, e2, s, e, len = 0;
    
    is_x = ( IsUnitHV( &stem1->unit,true ) == 2 );
    base1 = ( &stem1->left.x )[is_x];
    base2 = ( &stem2->left.x )[is_x];
    
    for ( i=0; i<stem1->activecnt; i++ ) {
	s1 = base1 + stem1->active[i].start;
	e1 = base1 + stem1->active[i].end;
	for ( ; j<stem2->activecnt; j++ ) {
	    s2 = base2 + stem2->active[j].start;
	    e2 = base2 + stem2->active[j].end;
	    if ( s2 > e1 )
	break;

	    if ( e2 < s1 )
	continue;

	    s = s2 < s1 ? s1 : s2;
	    e = e2 > e1 ? e1 : e2;
	    if ( e<s )
	continue;		/* Shouldn't happen */
	    len += e - s;
	}
    }
return( len );
}

static int StemPairsSimilar( struct stemdata *s1, struct stemdata *s2,
    struct stemdata *ts1, struct stemdata *ts2 ) {
    
    int normal, reversed, ret = 0;
    double olen1, olen2;
    
    /* Stem widths in the second pair should be nearly the same as */
    /* stem widths in the first pair */
    normal = (  ts1->width >= s1->width - dist_error_hv && 
		ts1->width <= s1->width + dist_error_hv &&
		ts2->width >= s2->width - dist_error_hv && 
		ts2->width <= s2->width + dist_error_hv );
    reversed = (ts1->width >= s2->width - dist_error_hv && 
		ts1->width <= s2->width + dist_error_hv &&
		ts2->width >= s1->width - dist_error_hv && 
		ts2->width <= s1->width + dist_error_hv );

    if ( !normal && !reversed )
return( false );

    if ( normal ) {
	olen1 = ActiveOverlap( s1, ts1 );
	olen2 = ActiveOverlap( s2, ts2 );
	ret =   olen1 > s1->clen/3 && olen1 > ts1->clen/3 &&
		olen2 > s2->clen/3 && olen2 > ts2->clen/3;
    } else if ( reversed ) {
	olen1 = ActiveOverlap( s1, ts2 );
	olen2 = ActiveOverlap( s2, ts1 );
	ret =   olen1 > s1->clen/3 && olen1 > ts2->clen/3 &&
		olen2 > s2->clen/3 && olen2 > ts1->clen/3;
    }
return( ret );
}

static void FindCounterGroups( struct glyphdata *gd,int is_v ) {
    struct stembundle *bundle = is_v ? gd->vbundle : gd->hbundle;
    struct stemdata *curm, *prevm, *cur, *prev;
    int i, j;
    double mdist, dist;
    
    prevm = NULL;
    for ( i=0; i<bundle->cnt; i++ ) {
	curm = prev = bundle->stemlist[i];
	if ( curm->master != NULL )
    continue;
	if ( prevm == NULL || curm->prev_c_m != NULL ) {
	    prevm = curm;
    continue;
	}
	mdist = is_v ? curm->left.x - prevm->right.x : curm->right.y - prevm->left.y;
	for ( j=i+1; j<bundle->cnt; j++ ) {
	    cur = bundle->stemlist[j];
	    if ( cur->master != NULL )
	continue;
	    if ( cur->prev_c_m != NULL ) {
		prev = cur;
	continue;
	    }
	    
	    dist =  is_v ? cur->left.x - prev->right.x : cur->right.y - prev->left.y;
	    if ( mdist > dist - dist_error_hv && mdist < dist + dist_error_hv && 
		StemPairsSimilar( prevm,curm,prev,cur )) {
		prev->next_c_m = prevm;
		cur->prev_c_m = curm;
	    }
	    prev = cur;
	}
	prevm = curm;
    }
}

/* Normally we use the DetectDiagonalStems flag (set via the Preferences dialog) to determine */
/* if diagonal stems should be generated. However, sometimes it makes sense to reduce the */
/* processing time, deliberately turning the diagonal stem detection off: in particular we */
/* don't need any diagonal stems if we only want to assign points to some preexisting HV */
/* hints. For thisreason  the only_hv argument still can be passed to this function. */
struct glyphdata *GlyphDataInit( SplineChar *sc,int layer,double em_size,int only_hv ) {
    struct glyphdata *gd;
    struct pointdata *pd;
    int i;
    SplineSet *ss;
    SplinePoint *sp;
    Monotonic *m;
    int cnt;
    double iangle;

    if ( layer<0 || layer>=sc->layer_cnt )
        return( NULL );

    /* We only hint one layer at a time */
    /* We shan't try to hint references yet */
    if ( sc->layers[layer].splines==NULL )
return( NULL );

    gd = calloc( 1,sizeof( struct glyphdata ));
    gd->only_hv = only_hv;
    gd->layer = layer;
    
    gd->sc = sc;
    gd->sf = sc->parent;
    gd->emsize = em_size;
    gd->order2 = ( sc->parent != NULL ) ? sc->parent->layers[layer].order2 : false;
    gd->fuzz = GetBlueFuzz( sc->parent );
    
    dist_error_hv = .0035*gd->emsize;
    dist_error_diag = .0065*gd->emsize;
    dist_error_curve = .022*gd->emsize;

    if ( sc->parent != NULL && sc->parent->italicangle ) {
	iangle = ( 90 + sc->parent->italicangle );
	gd->has_slant = true;
	gd->slant_unit.x = cos( iangle * ( PI/180 ));
	gd->slant_unit.y = sin( iangle * ( PI/180 ));
    } else {
	gd->has_slant = false;
	gd->slant_unit.x = 0; gd->slant_unit.y = 1;
    }

    /* SSToMContours can clean up the splinesets (remove 0 length splines) */
    /*  so it must be called BEFORE everything else (even though logically */
    /*  that doesn't make much sense). Otherwise we might have a pointer */
    /*  to something since freed */
    gd->ms = SSsToMContours(sc->layers[layer].splines,over_remove);	/* second argument is meaningless here */

    gd->realcnt = gd->pcnt = SCNumberPoints( sc, layer );
    for ( i=0, ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next, ++i );
    gd->ccnt = i;
    gd->contourends = malloc((i+1)*sizeof(int));
    for ( i=0, ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next, ++i ) {
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

    /* Create temporary point numbers for the implied points. We need this */
    /*  for metafont if nothing else */
    for ( ss= sc->layers[layer].splines; ss!=NULL; ss = ss->next ) {
	for ( sp = ss->first; ; ) {
	    if ( sp->ttfindex < gd->realcnt )
		sp->ptindex = sp->ttfindex;
	    else if ( sp->ttfindex == 0xffff )
		sp->ptindex = gd->pcnt++;
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
    for ( ss = sc->layers[layer].splines; ss!=NULL; ss = ss->next ) {
	for ( sp = ss->first; ; ) {
	    if ( sp->ttfindex == 0xfffe )
		sp->ptindex = gd->pcnt++;
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
    gd->pspace = malloc( gd->pcnt*sizeof( struct pointdata *));

    /*gd->ms = SSsToMContours(sc->layers[layer].splines,over_remove);*/	/* second argument is meaningless here */
    for ( m=gd->ms, cnt=0; m!=NULL; m=m->linked, ++cnt );
    gd->space = malloc((cnt+2)*sizeof(Monotonic*));
    gd->mcnt = cnt;

    gd->points = calloc(gd->pcnt,sizeof(struct pointdata));
    for ( ss=sc->layers[layer].splines; ss!=NULL; ss=ss->next ) if ( ss->first->prev!=NULL ) {
	for ( sp=ss->first; ; ) {
	    PointInit( gd,sp,ss );
	    if ( sp->next==NULL )
	break;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }

    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	pd = &gd->points[i];
	if ( !pd->nextzero )
	    pd->next_e_cnt = FindMatchingEdge(gd,pd,true,pd->nextedges);
	if ( !pd->prevzero )
	    pd->prev_e_cnt = FindMatchingEdge(gd,pd,false,pd->prevedges);
	if (( pd->symetrical_h || pd->symetrical_v ) && ( pd->x_corner || pd->y_corner))
	    FindMatchingEdge(gd,pd,2,&pd->bothedge);
    }

return( gd );
}

struct glyphdata *GlyphDataBuild( SplineChar *sc,int layer, BlueData *bd,int use_existing ) {
    struct glyphdata *gd;
    struct pointdata *pd;
    struct stemdata *stem;
    BasePoint dir;
    struct stem_chunk *chunk;
    int i, j, only_hv, startcnt, stemcnt, ecnt, hv, has_h, has_v;
    double em_size;
    DBounds bounds;
    
    only_hv = ( !detect_diagonal_stems && ( !use_existing || sc->dstem == NULL ));
    em_size = ( sc->parent != NULL ) ? sc->parent->ascent + sc->parent->descent : 1000;

    gd = GlyphDataInit( sc,layer,em_size,only_hv );
    if ( gd == NULL )
return( gd );
    /* Get the alignment zones */
    if ( bd == NULL )
	QuickBlues( gd->sf,gd->layer,&gd->bd );
    else
	memcpy( &gd->bd,bd,sizeof( BlueData ));

    /* There will never be more lines than there are points (counting next/prev as separate) */
    gd->lines = malloc( 2*gd->pcnt*sizeof( struct linedata ));
    gd->linecnt = 0;
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	pd = &gd->points[i];
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
    gd->stems = calloc( 2*gd->pcnt,sizeof( struct stemdata ));
    gd->stemcnt = 0;			/* None used so far */

    if ( use_existing ) {
	SplineCharFindBounds( gd->sc,&bounds );
	if ( sc->vstem != NULL )
	    _StemInfoToStemData( gd,sc->vstem,&bounds,true,&startcnt );
	if ( sc->hstem != NULL )
	    _StemInfoToStemData( gd,sc->hstem,&bounds,false,&startcnt );
	if ( sc->dstem != NULL )
	    _DStemInfoToStemData( gd,sc->dstem,&startcnt );
    }

    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp!=NULL ) {
	pd = &gd->points[i];
	if ( pd->prev_e_cnt > 0 ) {
	    ecnt = BuildStem( gd,pd,false,false,use_existing,0 );
	    if ( ecnt == 0 && pd->prev_e_cnt > 1 )
		BuildStem( gd,pd,false,false,false,1 );
	}
	if ( pd->next_e_cnt > 0 ) {
	    ecnt = BuildStem( gd,pd,true,false,use_existing,0 );
	    if ( ecnt == 0 && pd->next_e_cnt > 1 )
		BuildStem( gd,pd,true,false,false,1 );
	}
	if ( pd->bothedge!=NULL ) {
	    DiagonalCornerStem( gd,pd,false );
	}
	
	/* Snap corner extrema to preexisting hints if they have not */
	/* already been. This is currently done only when preparing  */
	/* glyph data for the autoinstructor */
	if ( use_existing && ( pd->x_corner || pd->y_corner )) {
	    has_h = has_v = false;
	    for ( j=0; j<pd->prevcnt && (( pd->x_corner && !has_v ) || ( pd->y_corner && !has_h )); j++ ) {
		hv = IsUnitHV( &pd->prevstems[j]->unit,true );
		if ( hv == 1 ) has_h = true;
		else if ( hv == 2 ) has_v = true;
	    }
	    for ( j=0; j<pd->nextcnt && (( pd->x_corner && !has_v ) || ( pd->y_corner && !has_h )); j++ ) {
		hv = IsUnitHV( &pd->nextstems[j]->unit,true );
		if ( hv == 1 ) has_h = true;
		else if ( hv == 2 ) has_v = true;
	    }
	    if ( pd->x_corner && !has_v ) {
		dir.x = 0; dir.y = 1;
		HalfStemNoOpposite( gd,pd,NULL,&dir,2 );
	    } else if ( pd->y_corner && !has_h ) {
		dir.x = 1; dir.y = 0;
		HalfStemNoOpposite( gd,pd,NULL,&dir,2 );
	    }
	}
    }
    AssignLinePointsToStems( gd );

    /* Normalize stems before calculating active zones (as otherwise */
    /* we don't know exact positions of stem edges */
    for ( i=0; i<gd->stemcnt; ++i )
	NormalizeStem( gd,&gd->stems[i] );
    GDNormalizeStubs( gd );

    /* Figure out active zones at the first order (as they are needed to */
    /* determine which stems are undesired and they don't depend from */
    /* the "potential" state of left/right points in chunks */
    gd->lspace = malloc(gd->pcnt*sizeof(struct segment));
    gd->rspace = malloc(gd->pcnt*sizeof(struct segment));
    gd->bothspace = malloc(3*gd->pcnt*sizeof(struct segment));
    gd->activespace = malloc(3*gd->pcnt*sizeof(struct segment));
#if GLYPH_DATA_DEBUG
    fprintf( stderr,"Going to calculate stem active zones for %s\n",gd->sc->name );
#endif
    for ( i=0; i<gd->stemcnt; ++i )
	FigureStemActive( gd,&gd->stems[i] );

    /* Check this before resolving stem conflicts, as otherwise we can */
    /* occasionally prefer a stem which should be excluded from the list */
    /* for some other reasons */
    GDFindUnlikelyStems( gd );

    /* we were cautious about assigning points to stems, go back now and see */
    /*  if there are any low-quality matches which remain unassigned, and if */
    /*  so then assign them to the stem they almost fit on. */
    for ( i=0; i<gd->stemcnt; ++i ) {
	stem = &gd->stems[i];
	for ( j=0; j<stem->chunk_cnt; ++j ) {
	    chunk = &stem->chunks[j];
	    if ( chunk->l!=NULL && chunk->lpotential ) {
		stemcnt = ( chunk->lnext ) ? chunk->l->nextcnt : chunk->l->prevcnt;
		if ( stemcnt == 1 ) chunk->lpotential = false;
	    }
	    if ( chunk->r!=NULL && chunk->rpotential ) {
		stemcnt = ( chunk->rnext ) ? chunk->r->nextcnt : chunk->r->prevcnt;
		if ( stemcnt == 1 ) chunk->rpotential = false;
	    }
	}
    }
    /* If there are multiple stems, find the one which is closest to this point */
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp != NULL ) {
	pd = &gd->points[i];
	if ( pd->prevcnt > 1 ) CheckPotential( gd,pd,false );
	if ( pd->nextcnt > 1 ) CheckPotential( gd,pd,true );
    }

    if ( hint_bounding_boxes )
	CheckForBoundingBoxHints( gd );
    CheckForGhostHints( gd );
    if ( use_existing )
        MarkDStemCorners( gd );

    GDBundleStems( gd,0,use_existing );
    if ( use_existing ) {
	for ( i=0; i<gd->stemcnt; ++i ) {
	    stem = &gd->stems[i];
	    if ( stem->toobig == 1 && IsUnitHV( &stem->unit,true ))
		GetSerifData( gd,stem );
	}
	FindCounterGroups( gd,true );
    }

#if GLYPH_DATA_DEBUG
    DumpGlyphData( gd );
#endif
    free(gd->lspace);		gd->lspace = NULL;
    free(gd->rspace);		gd->rspace = NULL;
    free(gd->bothspace);	gd->bothspace = NULL;
    free(gd->activespace);	gd->activespace = NULL;
 
return( gd );
}

void GlyphDataFree(struct glyphdata *gd) {
    int i;
    if ( gd == NULL )
return;

    FreeMonotonics( gd->ms );	gd->ms = NULL;
    free( gd->space );		gd->space = NULL;
    free( gd->sspace );		gd->sspace = NULL;
    free( gd->stspace );	gd->stspace = NULL;
    free( gd->pspace );		gd->pspace = NULL;

    /* Clean up temporary point numbers */
    for ( i=0; i<gd->pcnt; ++i ) if ( gd->points[i].sp != NULL )
	gd->points[i].sp->ptindex = 0;

    if ( gd->hbundle != NULL ) {
	free( gd->hbundle->stemlist );
	free( gd->hbundle );
    }
    if ( gd->vbundle != NULL ) {
	free( gd->vbundle->stemlist );
	free( gd->vbundle );
    }
    if ( gd->ibundle != NULL ) {
	free( gd->ibundle->stemlist );
	free( gd->ibundle );
    }
    
    for ( i=0; i<gd->linecnt; ++i )
	free( gd->lines[i].points );
    for ( i=0; i<gd->stemcnt; ++i ) {
	free( gd->stems[i].chunks );
	free( gd->stems[i].dependent );
	free( gd->stems[i].serifs );
	free( gd->stems[i].active );
    }
    for ( i=0; i<gd->pcnt; ++i ) {
	free( gd->points[i].nextstems );
	free( gd->points[i].next_is_l );
	free( gd->points[i].prevstems );
	free( gd->points[i].prev_is_l );
    }
    free( gd->lines );
    free( gd->stems );
    free( gd->contourends );
    free( gd->points );
    free( gd );
}
