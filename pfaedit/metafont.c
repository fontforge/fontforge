/* Copyright (C) 2001 by George Williams */
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
#include "edgelist.h"
#include <gwidget.h>
#include <ustring.h>
#include <math.h>

/* This module is designed to detect certain features of a character (like stems
 *  and counters, and then modify them in a useful way.
 * The most obvious examples are:
 *	To make all the stems thicker (thereby emboldening the font)
 *	To make all the counters narrower (thereby condensing the font)
 *	To change the xheight (thereby changing the feel of the font)
 *
 * So first we find all the stems (and I mean all, not just the horizontal/
 *  vertical stems). From this list (or perhaps from the hints?, yes from
 *  hints. Figuring horizontal/vertical is non-trivial.) we then look
 *  at just the hv stems and find the hv counters, based on this we figure
 *  how to position the centers of all stems.
 *	(we build a map based on horizontal stems and counters. We figure
 *	 how each stem and counter should expand, and based on that where
 *	 each should end up. We then extrapolate intermediate values.
 *	We do the same for the vertical stems, except in most cases they
 *	 won't move much. We may need to move the center of the xheight stem
 *	 down slightly so that the top of the stem is still at the xheight,
 *	 (and same for cap, while descent moves up) but that's about it.)
 *=====>Well that sounds all simple and doable. And then we realize that "A"
 *	 has no vertical stems, and no horizontal counters, while "B" has two
 *	 overlapping horizontal counters. Oh.
 *	  Ok, to handle "B" we just have two mapping regions, one for each
 *	    counter. And the boundary between them is the half-way between
 *	    the places the respective hints are valid.
 *	  Now "A" ("K", "M", "N" "R" "V" "W" "X" "Y") is harder. Do we even
 *	    want to expand the counters? Do we look at the diagonal stems
 *	    and figure counters between their end-points (and establish
 *	    seperate zones for top and bottom)?
 *	What do we do about Italic? Treat them as we did "A"? Include an
 *	 auto-deskew/reskew feature to bracket our processing?
 *	Heaven help us if we have a complicated CJK character with overlapping
 *	 counters.
 * We figure out how wide each stem is at all the points, and we figure where
 *  the stem's center lies there. We figure where the stem's center should be
 *  placed (based on the above map), we figure how much to expand the stem
 *  by (this gives us a final position for the point.
 *	At least that works if the point is not a corner point. If it's a
 *	corner point (so there are two non-parallel stems that intersect
 *	here) we figure the lines which are tangent to the stem edges, and
 *	then intersect them.
 * Since all points had better lie on stems, we should now have positioned
 *  every point. But we need to figure out the control points now.
 *	If a point has no control point, it continues not to.
 *	Otherwise find the x,y distance between this point and the next.
 *	 Find what it used to be.
 *	 Then interpolate new positions for the control points based on
 *	  how much these distances have expanded/contracted.
 * That should do it.
 *
 * Oh yes, it might be nice, before we start out, to make a copy of the old
 * character's shape and place it in the background so the user can compare
 * the two (and perhaps fix things more easily if we screwed up).
 */
/* Drat. Finding all diagonal stems is not possible with just horizontal and */
/*  vertical passes. Consider a rotated rectangle. The stem whose width is */
/*  the lesser will almost certainly show up, but the perp direction will not */
/*  be detected because no horizontal/vertical line will intersect both edges */
/* So... We could rotate the coordinate system for each slanted edge and */
/*  run a detection pass for it. (Ug) */
/* Or... we could just say that if we have to lose one of the stems, well we're*/
/*  losing the less important one (we lose the one that's wider), and this */
/*  algorithem only really works for hv stems anyway... */

/* ************ Data structures active during the entire command ************ */
enum counterchoices { cc_scale,	/* scale counters */
	cc_centerfixed,		/* stem centers remain fixed, stems expand around */
	cc_edgefixed,		/* outer edges are fixed, outer stems only expand */
				/*  inward, while inner stems have their centers */
			        /*  fixed */
	cc_zones		/* specify a mapping directly */
};

struct scale {
    double factor;
    double add;
};

struct zonemap {
    double from;
    double to;
};

struct stemcntl {
    unsigned int onlyh: 1;	/* this entry only controls hstems */
    unsigned int onlyv: 1;
    double small, wide;		/* Only controls stems whose width is between small and wide (inclusive) */
    double factor;
    double add;
};

typedef struct metafont {
    unsigned int scalewidth: 1;		/* If false then scale l/rbearing */
    struct scale width;			/* if scalewidth scale width by this */
    struct scale lbearing, rbearing;	/* if !scalewidth, do these */
    struct counters {
	unsigned int counterchoices: 2;
	struct scale counter;		/* if counterchoices==cc_scale */
	int zonecnt;
	struct zonemap *zones;
	double widthmin;		/* No counter may drop below this */
			/* Nor will a counter smaller than this be detected */
    } counters[2];			/* 0 is horizontal, 1 is vertical */
    int stemcnt;
    struct stemcntl *stems;
} MetaFontDlg;

/* ************ Data structures active on a per character basis ************* */

typedef struct splineinfo {
    Spline *spline1, *spline2;		/* spline1 will be leftmost (bottommost) */
    SplinePoint *from, *to;		/* Using the direction of spline1 */
    		/* Does spline1 or spline2 end first? so from will be either */
		/*  spline1->from, or spline2->to, and to either spline1->to */
		/*  or spline2->from */
    double fromlen, tolen;		/* The distance from spline1 to spline2*/
		/* (or spline2 to spline1 depending on whether from belongs */
		/*  to spline1 or spline2), perpendicular to the point there */
		/* if the len is -1 the we've decided that this doesn't */
		/*  qualify as a stem, probably because the two edges are */
		/*  nothing like parallel */
    BasePoint frommid, tomid;		/* The point mid-way between the two */
    		/* splines */
    unsigned int figured: 1;
    unsigned int freepasscnt: 1;
} SplineInfo;

typedef struct splinelist {
    SplineInfo *cur;
    struct splinelist *next;
} SplineList;

typedef struct pointinfo {
    SplinePoint *cur;
    BasePoint newme, newnext, newprev;
    SplineList *next/*, *prev*/;
    unsigned int mefigured: 1;
    unsigned int cpfigured: 1;	/* Probably don't need this bit */
} PointInfo;

struct countergroup {
    double bottom, top;		/* Range in the other coordinate over which this map is valid */
    int scnt, counternumber;
    StemInfo **stems;
    struct countergroup *next;
};

struct map {
    double bottom, top;		/* Range in the other coordinate over which this map is valid */
    int cnt;			/* number of entries in map */
    struct zonemap *mapping;
};

struct mapd {
    int mapcnt;			/* Count of the number of counter groups (maps), not the number of counters within the groups (zonemaps) */
    struct map *maps;
};

typedef struct scinfo {
    SplineChar *sc;
    PointInfo *pts;
    int ptcnt;
    struct mapd mapd[2];
    MetaFontDlg *meta;
    double rbearing;		/* Only used if !meta->scalewidth */
} SCI;



static SCI *SCIinit(SplineChar *sc,MetaFontDlg *meta) {
    /* Does five or six things:
	Sets the undoes
	Calculates hints (if we need to)
	Copies the forground to the background
	Closes any open paths (remove any single points)
	numbers the points
	creates the pointinfo list
    */
    SplinePointList *fore, *end, *ss, *ssnext, *prev;
    SplinePoint *sp;
    int cnt;
    SCI *sci;

    SCPreserveState(sc);
    SCPreserveBackground(sc);

    if ( sc->manualhints || sc->changedsincelasthinted )
	SplineCharAutoHint(sc,true);

    fore = SplinePointListCopy(sc->splines);
    if ( fore!=NULL ) {
	SplinePointListFree(sc->backgroundsplines);
	sc->backgroundsplines = NULL;
	for ( end = fore; end->next!=NULL; end = end->next );
	end->next = sc->backgroundsplines;
	sc->backgroundsplines = fore;
    }

    for ( prev=NULL, ss=sc->splines; ss!=NULL; ss = ssnext ) {
	ssnext = ss->next;
	if ( ss->first->next==NULL ) {	/* Single point */
	    if ( prev==NULL )
		sc->splines = ssnext;
	    else
		prev->next = ssnext;
	    SplinePointListFree(ss);
	} else
	    prev = ss;
    }
    for ( ss = sc->splines; ss!=NULL; ss=ss->next ) {
	if ( ss->first->prev==NULL ) {
	    /* It's not a loop, Close it! */
	    SplineMake( ss->last,ss->first );
	    ss->last = ss->first;
	}
    }

    for ( ss = sc->splines, cnt=0; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    sp->ptindex = cnt++;
	    sp = sp->next->to;		/* only closed paths, so no NULL splines */
	    if ( sp==ss->first )
	break;
	}
    }

    sci = gcalloc(1,sizeof(SCI));
    sci->sc = sc;
    sci->meta = meta;
    sci->ptcnt = cnt;
    sci->pts = gcalloc(cnt,sizeof(PointInfo));
    for ( ss = sc->splines, cnt=0; ss!=NULL; ss=ss->next ) {
	for ( sp=ss->first; ; ) {
	    sci->pts[cnt++].cur = sp;
	    sp = sp->next->to;
	    if ( sp==ss->first )
	break;
	}
    }
return( sci );
}

static void MapContentsFree(struct map *map) {
    free(map->mapping);
}

static void SCIFree(SCI *sci) {
    int i,j;
    SplineList *sl, *slnext;

    for ( i=0; i<sci->ptcnt; ++i ) {
	for ( sl = sci->pts[i].next; sl!=NULL; sl = slnext ) {
	    slnext = sl->next;
	    if ( sl->cur->freepasscnt ) {
		/* We'll see every SplineInfo twice, once from spline1, and */
		/*  once from spline2. We only free it on the second time. Can't */
		/*  free it the first time because then on the second go we */
		/*  have a dangling pointer and have no way to know that */
		free(sl->cur);
	    } else
		++sl->cur->freepasscnt;
	    free(sl);
	}
    }
    free(sci->pts);
    for ( j=0; j<2; ++j ) {
	for ( i=0; i<sci->mapd[j].mapcnt; ++i )
	    MapContentsFree(&sci->mapd[j].maps[i]);
	free(sci->mapd[j].maps);
    }
    free(sci);
}

static SplineInfo *SCIAddStem(SCI *sci,Spline *spline1,Spline *spline2) {
    int i = spline1->from->ptindex;
    SplineList *t;
    SplineInfo *cur;

    for ( t=sci->pts[i].next; t!=NULL &&
	    (t->cur->spline1!=spline1 || t->cur->spline2!=spline2) &&
	    (t->cur->spline1!=spline2 || t->cur->spline2!=spline1); t=t->next );
    if ( t!=NULL )
return(t->cur);
    cur = gcalloc(1,sizeof(SplineInfo));
    cur->spline1 = spline1;
    cur->spline2 = spline2;
    t = gcalloc(1,sizeof(SplineList));
    t->cur = cur;
    t->next = sci->pts[i].next;
    sci->pts[i].next = t;

    i = spline2->from->ptindex;
    t = gcalloc(1,sizeof(SplineList));
    t->cur = cur;
    t->next = sci->pts[i].next;
    sci->pts[i].next = t;
return(cur);
}

static void _SCIFindStems(SCI *sci,EIList *el, int major) {
    EI *active=NULL, *apt, *e, *p;
    int i, change, waschange;

    waschange = false;
    for ( i=0; i<el->cnt; ++i ) {
	active = EIActiveEdgesRefigure(el,active,i,major,&change);
	/* Change means something started, ended, crossed */
	/* I'd like to know when a crossing is going to happen the pixel before*/
	/*  it does. but that's too hard to compute */
	/* We also check every 16 pixels, mostly for cosmetic reasons */
	/*  (long almost horizontal/vert regions may appear to end to abruptly) */
	if ( !( waschange || change || el->ends[i] || el->ordered[i]!=NULL ||
		(i!=el->cnt-1 && (el->ends[i+1] || el->ordered[i+1]!=NULL)) ||
		(i&0xf)==0 ))
	    /* It's in the middle of everything. Nothing will have changed */
    continue;
	waschange = change;
	for ( apt=active; apt!=NULL; apt = e->aenext ) {
	    if ( !apt->hv && apt->aenext!=NULL && apt->aenext->hv &&
		    EISameLine(apt,apt->aenext,i+el->low,major))
		apt = apt->aenext;
	    e = p = EIActiveEdgesFindStem(apt, i+el->low, major);
	    if ( e==NULL )
	break;
	    SCIAddStem(sci,apt->spline,e->spline);
	    if ( EISameLine(p,p->aenext,i+el->low,major))	/* There's one case where this doesn't happen in FindStem */
		e = p->aenext;		/* If the e is horizontal and e->aenext is not */
	}
    }
}

static void SCIFindStems(SCI *sci) {
    EIList el;

    memset(&el,'\0',sizeof(el));
    ELFindEdges(sci->sc, &el);

    el.major = 0;
    ELOrder(&el,el.major);
    _SCIFindStems(sci,&el,el.major);
    free(el.ordered);
    free(el.ends);

    el.major = 1;
    ELOrder(&el,el.major);
    _SCIFindStems(sci,&el,el.major);
    free(el.ordered);
    free(el.ends);

    ElFreeEI(&el);
}

/* Generate a line through spline1[t1] perpendicular to it at that point */
/*  Intersect the line with spline2. If the intersection is on the spline */
/*  segment, then fill in the len and mid values */
static int FindPerpDistance(double t1,Spline *spline1, Spline *spline2,
	BasePoint *mid, double *len) {
    BasePoint pt, slope, pslope, end[3], ss2;
    double x, y, slope1, slope2, ts[3], lens[3], angle;
    Spline1D temp;
    int i,j;

    pt.x = ((spline1->splines[0].a*t1+spline1->splines[0].b)*t1+spline1->splines[0].c)*t1 + spline1->splines[0].d;
    pt.y = ((spline1->splines[1].a*t1+spline1->splines[1].b)*t1+spline1->splines[1].c)*t1 + spline1->splines[1].d;
    slope.x = (3*spline1->splines[0].a*t1+2*spline1->splines[0].b)*t1+spline1->splines[0].c;
    slope.y = (3*spline1->splines[1].a*t1+2*spline1->splines[1].b)*t1+spline1->splines[1].c;
    pslope.x = -slope.y; pslope.y = slope.x;

    if ( spline2->knownlinear ) {
	if ( pslope.x==0 ) {
	    if ( spline2->splines[0].c==0 )
return( false );			/* parallel */
	    ts[0] = (pt.x-spline2->splines[0].d)/spline2->splines[0].c;
	} else if ( spline2->splines[0].c==0 ) {
	    y = pt.y + (pslope.y/pslope.x)*(spline2->splines[0].d-pt.x);
	    ts[0] = (y-spline2->splines[1].d)/spline2->splines[1].c;
	} else {
	    /* pt.y + (slope.y/slope.x)*(x-pt.x) = spline2->splines[1].d + (spline2->splines[1].c/spline2->splines[0].c)*(x-spline2->splines[0].d) */
	    /* x*(slope1-slope2) = spline2->splines[1].d-pt.y+slope1*pt.x-slope2*spline2->splines[0].d */
	    slope1 = pslope.y/pslope.x;
	    slope2 = spline2->splines[1].c/spline2->splines[0].c;
	    if ( slope1==slope2 )
return( false );
	    x = (spline2->splines[1].d-pt.y+slope1*pt.x-slope2*spline2->splines[0].d)/(slope1-slope2);
	    ts[0] = (x-spline2->splines[0].d)/spline2->splines[0].c;
	}
	if ( ts[0]<-0.01 || ts[0]>1.01 )	/* Allow a little fudge for rounding */
return( false );
	ts[1] = ts[2] = -1;
    } else {
	if ( pslope.x==0 )
	    SplineSolveFull(&spline2->splines[0],pt.x,ts);
	else if ( pslope.y==0 )
	    SplineSolveFull(&spline2->splines[1],pt.y,ts);
	else {
	    slope1 = pslope.y/pslope.x;
	    temp.a = slope1*spline2->splines[0].a - spline2->splines[1].a;
	    temp.b = slope1*spline2->splines[0].b - spline2->splines[1].b;
	    temp.c = slope1*spline2->splines[0].c - spline2->splines[1].c;
	    temp.d = pt.y + slope1*(spline2->splines[0].d-pt.x) - spline2->splines[1].d;
	    SplineSolveFull(&temp,0,ts);
	}
    }

    /* There may be multiple solutions for a given spline. Find the closest */
    /*  one which is bigger than 0 (we exclude 0 in case spline1==spline2, */
    /*  it is possible for a spline to loop back on itself, and we don't want */
    /*  it finding the start point where the distance is zero) */
    for ( i=0; i<3 && ts[i]!=-1; ++i ) {
	end[i].x = ((spline2->splines[0].a*ts[i]+spline2->splines[0].b)*ts[i]+spline2->splines[0].c)*ts[i] + spline2->splines[0].d;
	end[i].y = ((spline2->splines[1].a*ts[i]+spline2->splines[1].b)*ts[i]+spline2->splines[1].c)*ts[i] + spline2->splines[1].d;
	lens[i] = sqrt( (pt.x-end[i].x)*(pt.x-end[i].x) + (pt.y-end[i].y)*(pt.y-end[i].y) );
    }
    if ( i==0 )
return( false );
    if ( i==1 )
	j = 0;
    else {
	j = 1;
	if ( (lens[0]>0 && lens[0]<lens[1]) || lens[1]==0 )
	    j = 0;
	if ( i==3 )
	    if ( lens[2]>0 && lens[3]<lens[j] )
		j = 2;
    }

    mid->x = (pt.x+end[j].x)/2;
    mid->y = (pt.y+end[j].y)/2;
    if ( lens[j]<.001 ) lens[j]=0;
    else {
	double len = rint(lens[j]);
	if ( lens[j]>len-.001 && lens[j]<len+.001 )
	    lens[j] = len;
    }

    /* The two splines should be vaguely parallel for the thing to count as */
    /*  a stem... */
    ss2.x = (3*spline2->splines[0].a*ts[j]+2*spline2->splines[0].b)*ts[j]+spline2->splines[0].c;
    ss2.y = (3*spline2->splines[1].a*ts[j]+2*spline2->splines[1].b)*ts[j]+spline2->splines[1].c;
    if (!(( slope.y==0 && ss2.y==0 ) || (slope.x==0 && ss2.x==0)) ) {
	angle = atan2(slope.y,slope.x)-atan2(ss2.y,ss2.x);
	while ( angle<0 ) angle += 3.1415926535897932;
	while ( angle>=3.1415926535897932 ) angle -= 3.1415926535897932;
	if ( angle>.5 && angle < 3.1415926535897932-.5 ) {
	    *len = -lens[j];
	    if ( *len==0 ) *len = -.01;		/* mark as bad */
return( -1 );
	}
    }
    *len = lens[j];
return( true );
}

static void SIFigureWidth(SplineInfo *si) {
    int foundfrom=false, foundto=false;
    int up1, up2;

 printf( "\nspline1=(%g,%g) -> (%g,%g), spline2=(%g,%g) -> (%g,%g)\n",
     si->spline1->from->me.x, si->spline1->from->me.y, si->spline1->to->me.x, si->spline1->to->me.y,
     si->spline2->from->me.x, si->spline2->from->me.y, si->spline2->to->me.x, si->spline2->to->me.y );

    si->fromlen = si->tolen = -1;
    if (( foundfrom = (FindPerpDistance(0,si->spline1,si->spline2,&si->frommid,&si->fromlen)>0) ))
	si->from = si->spline1->from;
    if (( foundto = (FindPerpDistance(1,si->spline1,si->spline2,&si->tomid,&si->tolen)>0) ))
	si->to = si->spline1->to;
    if ( true || !foundfrom || !foundto ) {
	if ( si->spline1->from->me.y==si->spline1->to->me.y ||
		si->spline2->from->me.y==si->spline2->to->me.y ) {
	    up1 = si->spline1->from->me.x<si->spline1->to->me.x;
	    up2 = si->spline2->from->me.x<si->spline2->to->me.x;
	} else {
	    up1 = si->spline1->from->me.y<si->spline1->to->me.y;
	    up2 = si->spline2->from->me.y<si->spline2->to->me.y;
	}
	if ( up1==up2 ) {
	    if ( !foundfrom ) {
		if ( FindPerpDistance(0,si->spline2,si->spline1,&si->frommid,&si->fromlen)>0 )
		    si->from = si->spline2->from;
	    }
	    if ( !foundto ) {
		if ( FindPerpDistance(1,si->spline2,si->spline1,&si->frommid,&si->fromlen)>0 )
		    si->to = si->spline2->to;
	    }
 printf( "(%g,%g) <-> (%g,%g) mid=(%g,%g) len=%g\n",
si->spline1->from->me.x, si->spline1->from->me.y,
si->spline2->from->me.x, si->spline2->from->me.y,
si->frommid.x, si->frommid.y, si->fromlen);
 printf( "(%g,%g) <-> (%g,%g) mid=(%g,%g) len=%g\n",
si->spline1->to->me.x, si->spline1->to->me.y,
si->spline2->to->me.x, si->spline2->to->me.y,
si->tomid.x, si->tomid.y, si->tolen);
	} else {
	    if ( !foundfrom ) {
		if ( FindPerpDistance(1,si->spline2,si->spline1,&si->frommid,&si->fromlen) )
		    si->from = si->spline2->to;
	    }
	    if ( !foundto ) {
		if ( FindPerpDistance(0,si->spline2,si->spline1,&si->frommid,&si->fromlen) )
		    si->to = si->spline2->from;
	    }
 printf( "(%g,%g) <-> (%g,%g) mid=(%g,%g) len=%g\n",
si->spline1->from->me.x, si->spline1->from->me.y,
si->spline2->to->me.x, si->spline2->to->me.y,
si->frommid.x, si->frommid.y, si->fromlen);
 printf( "(%g,%g) <-> (%g,%g) mid=(%g,%g) len=%g\n",
si->spline1->to->me.x, si->spline1->to->me.y,
si->spline2->from->me.x, si->spline2->from->me.y,
si->tomid.x, si->tomid.y, si->tolen);
	}
    }
    si->figured = true;

    if ( (si->fromlen>0 && si->tolen>=0 ) || (si->fromlen>=0 && si->tolen>0))
	si->spline1->touched = si->spline2->touched = true;
    else
	si->fromlen = si->tolen = -1;
}

/* We weren't able to find a matching edge for this spline (to make a stem) */
/*  by doing a horizontal or vertical traverse of the glyph. Let's try to */
/*  traverse perpendicular to the spline itself */
static void SplineFindOtherEdge(SCI *sci,Spline *spline) {
    double angle, s, c, xval, yval;
    BasePoint pt, mid;
    Spline *right, *left, *test, *first;
    SplineSet *ss;
    double rlen, llen, len;
    int bcnt;
    int bad, lbad, rbad;

    angle = atan2( (3*spline->splines[1].a*.5+2*spline->splines[1].b)*.5+spline->splines[1].c,
		    (3*spline->splines[0].a*.5+2*spline->splines[0].b)*.5+spline->splines[0].c) +
	3.1415926535897932/2;
    s = sin(angle); c = cos(angle);
    pt.x = ((spline->splines[0].a*.5+spline->splines[0].b)*.5+spline->splines[0].c)*.5 + spline->splines[0].d;
    pt.y = ((spline->splines[1].a*.5+spline->splines[1].b)*.5+spline->splines[1].c)*.5 + spline->splines[1].d;
    xval = pt.x*c + pt.y*s;
    yval = -pt.x*s + pt.y*c;
    /* Now look through all splines, find any whose bb includes our xval */
    /*  Need to count intersections left. If count even find closest inter */
    /*  right, if odd find closest left */
    /*  For any that do, find the intersection & length, select the one with */
    /*  the shortest length, build up an SI for it, insert into list, test */
    /*  it's end points, mark this guy as touched EVEN IF WE DIDN"T FIND ANYTHING */
    /*  if we repeat this process we won't find anything better */

    right = left = NULL; rlen = llen = 0x7fffffff; bcnt = 0;
    for ( ss = sci->sc->splines; ss!=NULL; ss=ss->next ) {
	first = NULL;
	for ( test = ss->first->next; test!=first; test = test->to->next ) {
	  if ( test!=spline ) {
	    double f,t,c1,c2;
	    f = -test->from->me.x*s + test->from->me.y*c;
	    c1 = -test->from->nextcp.x*s + test->from->nextcp.y*c;
	    t = -test->to->me.x*s + test->to->me.y*c;
	    c2 = -test->to->prevcp.x*s + test->to->prevcp.y*c;
	    if ( (f<yval && t<yval && c1<yval && c2<yval ) ||
		    (f>yval && t>yval && c1>yval && c2>yval ) )
	continue;		/* Can't intersect us */
	    if ( FindPerpDistance(.5,spline,test,&mid,&len)) {
		if ( (bad = len<0) ) len = -len;
		if ( mid.x*c + mid.y*s < xval ) {
		    /* If we intersect a spline right at its endpoint then */
		    /*  we can't add the full amount because we'll find another*/
		    /*  spline later (or earlier) that intersects at the */
		    /*  same place. So in that case we add half the normal */
		    /*  amount */
		    if ( DoubleNear(-test->from->me.x*s - test->from->me.y*c,yval) ||
			    DoubleNear(-test->to->me.x*s - test->to->me.y*c,yval) )
			++bcnt;
		    else
			bcnt += 2;
		    if ( left==NULL || len<llen ) {
			left = test;
			llen = len;
			lbad = bad;
		    }
		} else {
		    if ( right==NULL || len<rlen ) {
			right = test;
			rlen = len;
			rbad = bad;
		    }
		}
	    }
	  }
	    if ( first==NULL ) first = test;
	}
    }

    if ( bcnt&1 )
	GDrawIError( "Odd number of Point intersections in SplineFindOtherEdge" );
    if ( bcnt&2 ) {
	/* Odd number of splines left */
	right = left;
	rlen = llen;
	rbad = lbad;
    }
    if ( !rbad && right!=NULL ) {
	SplineInfo *si = SCIAddStem(sci,(bcnt&2)?right:spline,(bcnt&2)?spline:right);
	if ( si!=NULL && !si->figured )
	    SIFigureWidth(si);
    }
    spline->touched = true;		/* Even if we didn't find anything */
}

static void SCIFigureWidths(SCI *sci) {
    int i;
    SplineList *sl;

    for ( i=0; i<sci->ptcnt; ++i ) {
	for ( sl = sci->pts[i].next; sl!=NULL; sl = sl->next ) {
	    sl->cur->spline1->touched = false;
	    sl->cur->spline2->touched = false;
	}
    }

    for ( i=0; i<sci->ptcnt; ++i ) {
	for ( sl = sci->pts[i].next; sl!=NULL; sl = sl->next ) {
	    if ( !sl->cur->figured )
		SIFigureWidth(sl->cur);
	}
    }

    for ( i=0; i<sci->ptcnt; ++i ) {
	for ( sl = sci->pts[i].next; sl!=NULL; sl = sl->next ) {
	    if ( !sl->cur->spline1->touched )
		SplineFindOtherEdge(sci,sl->cur->spline1);
	    if ( !sl->cur->spline2->touched )
		SplineFindOtherEdge(sci,sl->cur->spline2);
	}
    }
}

static void CounterGroupsFree(struct countergroup *cg) {
    struct countergroup *next;

    for ( ; cg!=NULL; cg = next ) {
	next = cg->next;
	free(cg->stems);
	free(cg);
    }
}

static double MetaFontFindWidth(MetaFontDlg *meta,double width,int isvert) {
    int i;

    /* First look for matches where isvert matches */
    if ( isvert==0 || isvert==1 ) {
	for ( i=0; i<meta->stemcnt; ++i ) {
	    struct stemcntl *c = &meta->stems[i];
	    if ( ((isvert && c->onlyv) || (!isvert && c->onlyh)) &&
		width<=c->wide && width>=c->small )
return( c->factor*width + c->add );
	}
    }

    for ( i=0; i<meta->stemcnt; ++i ) {
	struct stemcntl *c = &meta->stems[i];
	if ( !c->onlyv && !c->onlyh && width<=c->wide && width>=c->small )
return( c->factor*width + c->add );
    }

return( width );	/* didn't match anything, return unchanged */
}

static int MetaRecognizedStemWidth(MetaFontDlg *meta,double width,int isvert) {
    int i;

    /* First look for matches where isvert matches */
    if ( isvert==0 || isvert==1 ) {
	for ( i=0; i<meta->stemcnt; ++i ) {
	    struct stemcntl *c = &meta->stems[i];
	    if ( ((isvert && c->onlyv) || (!isvert && c->onlyh)) &&
		width<=c->wide && width>=c->small )
return( true );
	}
    }

    for ( i=0; i<meta->stemcnt; ++i ) {
	struct stemcntl *c = &meta->stems[i];
	if ( !c->onlyv && !c->onlyh && width<=c->wide && width>=c->small )
return( true );
    }

return( false );
}

static struct map *MapFromZones(struct zonemap *zones, int cnt,
	struct zonemap *extras, int ecnt) {
    struct map *map = gcalloc(1,sizeof(struct map));
    int i;

    /* This map covers the entire character */
    map->bottom = -1e8;
    map->top = 1e8;

    map->cnt = cnt + ecnt;
    map->mapping = galloc(map->cnt*sizeof(struct zonemap));

    i = 0;
    if ( ecnt!=0 ) {
	memcpy(map->mapping,extras,ecnt*sizeof(struct zonemap));
	i = ecnt;
    }
    if ( cnt!=0 )
	memcpy(map->mapping+i,zones,cnt*sizeof(struct zonemap));
return( map );
}

static void MapFromCounterGroup(struct map *map,MetaFontDlg *meta,
	struct countergroup *cg, struct zonemap *extra, int ecnt, int isvert ) {
    double offset=0;
    int i=0,j;
    double newwidth, lastwidth, counterwidth, newcwidth;

    map->bottom = cg->bottom;
    map->top = cg->top;

    map->cnt = ecnt+2*cg->scnt;
    map->mapping = galloc(map->cnt*sizeof(struct zonemap));

    if ( ecnt!=0 ) {
	memcpy(map->mapping,extra,ecnt*sizeof(struct zonemap));
	i = ecnt;
	offset = extra[ecnt-1].to - extra[ecnt-1].from;
    }

    newwidth = 0;		/* Irrelevant, but it means compiler doesn't think newwidth uninitialized */
    for ( j=0; j<cg->scnt; ++j ) {
	lastwidth = newwidth;
	newwidth = MetaFontFindWidth(meta,cg->stems[j]->width,isvert);
	if ( j!=0 ) {
	    newcwidth = counterwidth =
		    cg->stems[j]->start - (cg->stems[j-1]->start+cg->stems[j-1]->width);
	    switch ( meta->counters[isvert].counterchoices ) {
	      case cc_scale:
		newcwidth = meta->counters[isvert].counter.factor*counterwidth +
			meta->counters[isvert].counter.add;
	      break;
	      case cc_centerfixed:
		newcwidth -= (lastwidth-cg->stems[j-1]->width)/2 +
			(newwidth - cg->stems[j]->width)/2;
	      break;
	      case cc_edgefixed:
		if ( j==1 )
		    newcwidth -= (lastwidth-cg->stems[j-1]->width);
		else
		    newcwidth -= (lastwidth-cg->stems[j-1]->width)/2;
		if ( cg->scnt==j+1 )
		    newcwidth -= (newwidth - cg->stems[j]->width);
		else
		    newcwidth -= (newwidth - cg->stems[j]->width)/2;
	      break;
	      default:
		GDrawIError("Shouldn't get here in MapFromCounterGroup" );
	      break;
	    }
	    if ( newcwidth<meta->counters[isvert].widthmin ) {
		GWidgetPostNoticeR(_STR_CounterTooSmallT,_STR_CounterTooSmall);
		newcwidth = meta->counters[isvert].widthmin;
	    }
	    offset += newcwidth-counterwidth;
	}
	map->mapping[i].from = cg->stems[j]->start;
	map->mapping[i++].to = rint(cg->stems[j]->start+offset);
	map->mapping[i].from = cg->stems[j]->start+cg->stems[j]->width;
	map->mapping[i].to = map->mapping[i-1].to+newwidth;
	++i;
	offset += newwidth-cg->stems[j]->width;
    }
}

static int CountIntersectingWheres(StemInfo *stems, HintInstance *match) {
    int cnt=0;
    HintInstance *hi;

    while ( stems!=NULL ) {
	for ( hi=stems->where; hi!=NULL; hi=hi->next ) {
	    if ( hi->end>=match->begin && hi->begin<=match->end ) {
		++cnt;
	break;
	    }
	}
	stems = stems->next;
    }
return( cnt );    
}

static void TouchIntersectingWheres(StemInfo *stems, HintInstance *match,
	struct countergroup *cg, StemInfo **store) {
    HintInstance *hi;

    while ( stems!=NULL ) {
	for ( hi=stems->where; hi!=NULL; hi=hi->next ) {
	    if ( hi->end>=match->begin && hi->begin<=match->end ) {
		hi->counternumber = match->counternumber;
		if ( cg->bottom>hi->begin ) cg->bottom = hi->begin;
		if ( cg->top<hi->end ) cg->top = hi->end;
		if ( store!=NULL ) *store++ = stems;
	break;
	    }
	}
	stems = stems->next;
    }
}

static int AllIntersectingWheresOnStemUsed(StemInfo *stems, HintInstance *match,
	HintInstance *sameas) {
    HintInstance *hi, *hi2;

    while ( stems!=NULL ) {
	for ( hi=stems->where; hi!=NULL; hi=hi->next ) {
	    if ( hi->end>=match->begin && hi->begin<=match->end ) {
		for ( hi2=stems->where; hi2!=NULL; hi2=hi2->next ) {
		    if ( hi2->counternumber==sameas->counternumber )
		break;
		}
		if ( hi2==NULL )
return( false );
	    }
	}
	stems = stems->next;
    }
return( true );
}

static struct countergroup *SCIFindCounterGroups(StemInfo *stems) {
    StemInfo *s;
    HintInstance *hi, *best;
    int cnt, allhi, ctest;
    struct countergroup *cg = NULL, *test, *cur, *prev, *next;
    int cnum=0;

    for ( s=stems; s!=NULL; s=s->next ) {
	for ( hi=s->where; hi!=NULL; hi=hi->next )
	    hi->counternumber = -1;
    }

    for ( s=stems; s!=NULL; ) {
	cnt = 0; allhi = true;
	for ( hi=s->where; hi!=NULL; hi=hi->next ) {
	    if ( hi->counternumber==-1 ) {
		if ( ( ctest = CountIntersectingWheres(s->next,hi)+1)>cnt ) {
		    cnt = ctest;
		    best = hi;
		}
	    } else
		allhi = false;
	}
	if ( cnt==0 ) {
	    s = s->next;
    continue;
	}
	cur = gcalloc(1,sizeof(struct countergroup));
	cur->scnt = cnt;
	cur->stems = galloc(cnt*sizeof(StemInfo *));
	cur->bottom = best->begin; cur->top = best->end;
	cur->counternumber = cnum++;
	if ( cg==NULL || cur->scnt>cg->scnt ) {
	    cur->next = cg;
	    cg = cur;
	} else {
	    for ( test=cg; test->next!=NULL && test->next->scnt>cg->scnt ;
		    test = test->next );
	    cur->next = test->next;
	    test->next = cur;
	}
	best->counternumber = cur->counternumber;
	cur->stems[0] = s;
	TouchIntersectingWheres(s->next,best,cur,cur->stems+1);
	for ( hi=s->where; hi!=NULL; hi=hi->next ) {
	    if ( hi->counternumber==-1 ) {
		if ( AllIntersectingWheresOnStemUsed(s->next,hi,best)) {
		    hi->counternumber = cur->counternumber;
		    TouchIntersectingWheres(s->next,hi,cur,NULL);
		    if ( cur->bottom>hi->begin ) cur->bottom = hi->begin;
		    if ( cur->top<hi->end ) cur->top = hi->end;
		}
	    }
	}
    }
    /* If we've got a stem, we should create one cg */
    if ( cg==NULL && stems!=NULL ) {
	cur = gcalloc(1,sizeof(struct countergroup));
	cur->scnt = 1;
	cur->stems = galloc(sizeof(StemInfo *));
	cur->stems[0] = stems;
	cur->bottom = -1e8; cur->top = 1e8;
	cur->counternumber = cnum++;
    }
    /* get rid of any overlapping groups, we keep the one with the most stems */
    /*  in it (it comes first in the list) */
    for ( cur=cg; cur!=NULL; cur=cur->next ) {
	for ( prev=cur, test=cur->next; test!=NULL; test = next ) {
	    next = test->next;
	    if ( test->top>=cur->bottom && test->bottom<=cur->top ) {
		prev->next = next;
		free(test->stems);
		free(test);
	    } else
		prev = test;
	}
    }
return(cg);
}

static struct map *MapFromDiags(MetaFontDlg *meta,StemInfo *vstem, DStemInfo *dstem,
	struct zonemap *extras, int ecnt) {
    struct map *map = gcalloc(1,sizeof(struct map));
    double minx, maxx, minxw, maxxw, olen, orthwidth, width;
    BasePoint orthvector;
    StemInfo dummystems[2];
    StemInfo *stems[3];
    struct countergroup cg;
    /* Create a counter group that has some bearing on the diagonal hints */
    /*  (ie. that two dummy stems, one at the min location of all the hints */
    /*   one at the max location of all) */
    /* That should handle A, V, X. Doesn't do W well, but better than nothing */
    /* If there's a vstem then toss it into the mixture. */
    /* That should handle K and cyrillic ZHE */

    minx = 1e8; maxx = -1e8;
    while ( dstem!=NULL ) {
	orthvector.x =  dstem->leftedgetop.y-dstem->leftedgebottom.y;
	orthvector.y = -dstem->leftedgetop.x+dstem->leftedgebottom.x;
	olen = sqrt( orthvector.x*orthvector.x + orthvector.y*orthvector.y );
	orthvector.x /= olen; orthvector.y /= olen;
	orthwidth = orthvector.x*(dstem->rightedgetop.x-dstem->leftedgetop.x) +
		orthvector.y*(dstem->rightedgetop.y-dstem->leftedgetop.y);
	if ( minx>dstem->leftedgetop.x ) {
	    minx = dstem->leftedgetop.x;
	    minxw = width;
	}
	if ( minx>dstem->leftedgebottom.x ) {
	    minx = dstem->leftedgebottom.x;
	    minxw = width;
	}
	if ( maxx<dstem->rightedgetop.x ) {
	    maxx = dstem->rightedgetop.x;
	    maxxw = width;
	}
	if ( maxx<dstem->rightedgebottom.x ) {
	    maxx = dstem->rightedgebottom.x;
	    maxxw = width;
	}
	dstem = dstem->next;
    }
    if ( vstem!=NULL ) {	/* At most one */
	if ( vstem->start+vstem->width >= minx && vstem->start < minx ) {
	    minx = vstem->start;
	    minxw = vstem->width;
	    vstem = NULL;
	}
	if ( vstem->start<=maxx && vstem->start+vstem->width > maxx ) {
	    maxx = vstem->start+vstem->width;
	    maxxw = vstem->width;
	    vstem = NULL;
	}
    }
    memset(dummystems,'\0',sizeof(dummystems));
    memset(&cg,'\0',sizeof(cg));
    dummystems[0].start = minx;
    dummystems[0].width = minxw;
    dummystems[1].start = maxx-maxxw;
    dummystems[1].width = maxxw;
    cg.scnt = 3;
    cg.stems = stems;
    cg.bottom = -1e8; cg.top = 1e8;
    stems[0] = dummystems;
    stems[1] = dummystems+1;
    stems[2] = dummystems+1;
    if ( vstem==NULL ) {
	cg.scnt = 2;
    } else if ( vstem->start<minx ) {
	stems[0] = vstem;
	stems[1] = dummystems;
    } else if ( vstem->start<maxx ) {
	stems[1] = vstem;
    } else {
	stems[2] = vstem;
    }
    MapFromCounterGroup(map,meta,&cg, extras,ecnt,false);
return( map );
}
    
static void SCIBuildMaps(SCI *sci, int isvert) {
    MetaFontDlg *meta = sci->meta;
    struct zonemap extras[2];
    int ecnt = 0;
    DBounds b;
    struct countergroup *countergroups, *cg;
    int i;

    if ( !isvert ) {
	SplineCharFindBounds(sci->sc,&b);
	extras[0].from = b.minx;
	if ( meta->scalewidth ) {
	    extras[0].to = extras[0].from;
	    sci->sc->width = meta->width.factor*sci->sc->width + meta->width.add;
	} else {
	    extras[0].to = meta->lbearing.factor*b.minx + meta->lbearing.add;
	    sci->rbearing = meta->rbearing.factor*(sci->sc->width-b.maxx) + meta->rbearing.add;
	}
	ecnt = 1;
    }

    if ( meta->counters[isvert].counterchoices==cc_zones ) {
	sci->mapd[isvert].mapcnt = 1;
	/* add the lbearing zone if it's before the first zone user specified */
	sci->mapd[isvert].maps = MapFromZones(
		meta->counters[isvert].zones,meta->counters[isvert].zonecnt,
		extras,
		(!isvert && (meta->counters[0].zonecnt==0 ||
			extras[0].from<meta->counters[0].zones[0].from))?1:0);
    } else if ( !isvert &&
	    (sci->sc->vstem==NULL || sci->sc->vstem->next==NULL) &&
	    sci->sc->dstem!=NULL ) {
	/* If we're looking for horizontal counters, but we've got no (or at */
	/*  most one) vertical stem, then see if we can generate any counters */
	/*  from the diagonal stems */
	sci->mapd[0].mapcnt = 1;
	sci->mapd[0].maps = MapFromDiags(sci->meta,sci->sc->vstem,
		sci->sc->dstem,extras,1);
    } else {
	/* Horizontal counters need vertical stems, and vice versa */
	countergroups = SCIFindCounterGroups(isvert ? sci->sc->hstem : sci->sc->vstem);
	for ( i=0, cg=countergroups; cg!=NULL; ++i, cg=cg->next );
	if ( i==0 ) i=1;
	sci->mapd[isvert].mapcnt = i;
	sci->mapd[isvert].maps = gcalloc(i,sizeof(struct map));
	for ( i=0, cg=countergroups; cg!=NULL; ++i, cg=cg->next )
	    MapFromCounterGroup(&sci->mapd[isvert].maps[i],meta,cg,
		    extras,ecnt,isvert);
	CounterGroupsFree(countergroups);
    }
}

static double _MapCoord(struct map *map, double coord) {
    int i;

    if ( map==NULL || map->cnt==0 )
return( coord );
    if ( coord<=map->mapping[0].from )
return( coord+map->mapping[0].to-map->mapping[0].from );
    for ( i=1; i<map->cnt; ++i ) {
	if ( coord<=map->mapping[i].from )
return( map->mapping[i].to + (coord-map->mapping[i].from)*
	(map->mapping[i].to-map->mapping[i-1].to)/
	(map->mapping[i].from-map->mapping[i-1].from) );
    }
    --i;
return( coord+map->mapping[i].to-map->mapping[i].from );
}

static double MapCoord(struct mapd *map, double coord, double other) {
    int i, top=-1, bottom=-1;
    double topdiff = 1e8, bottomdiff = 1e8;

    if ( map->mapcnt==1 )
return( _MapCoord(&map->maps[0],coord));
    for ( i=0; i<map->mapcnt; ++i ) {
	if ( other>=map->maps[i].bottom && other<=map->maps[i].top )
return( _MapCoord(&map->maps[i],coord));
	if ( other>map->maps[i].top && other-map->maps[i].top<topdiff ) {
	    top = i;
	    topdiff = other-map->maps[i].top;
	} else if ( other<map->maps[i].bottom && map->maps[i].bottom-other<bottomdiff ) {
	    bottom = i;
	    bottomdiff = map->maps[i].bottom-other;
	}
    }
    if ( bottom!=-1 && top!=-1 )
return( (_MapCoord(&map->maps[bottom],coord)+_MapCoord(&map->maps[top],coord))/2 );
    else if ( bottom!=-1 )
return( _MapCoord(&map->maps[bottom],coord));
    else if ( top!=-1 )
return( _MapCoord(&map->maps[top],coord));
    else
return( _MapCoord(&map->maps[0],coord));
}

static void SCIMapPoint(SCI *sci,BasePoint *pt) {
    pt->x = MapCoord(&sci->mapd[0],pt->x,pt->y);
    pt->y = MapCoord(&sci->mapd[1],pt->y,pt->x);
}

static int _IsOnKnownEdge(struct map *map, double coord) {
    int i;

    for ( i=0; i<map->cnt; ++i ) {
	if ( coord<map->mapping[0].from-.1 )
return( false );
	if ( coord<map->mapping[0].from+.1 )
return( true );
    }
return( false );
}

static int IsOnKnownEdge(struct mapd *map, double coord, double other) {
    int i;

    if ( map->mapcnt==1 )
return( _IsOnKnownEdge(&map->maps[0],coord));
    for ( i=0; i<map->mapcnt; ++i ) {
	if ( other>=map->maps[i].bottom && other<=map->maps[i].top )
return( _IsOnKnownEdge(&map->maps[i],coord));
    }
return( false );
}

static int SCIIsOnKnownEdge(SCI *sci,BasePoint *pt,int isvert) {
    if ( isvert==0 )
return( IsOnKnownEdge(&sci->mapd[0],pt->x,pt->y));
    else if ( isvert==1 )
return( IsOnKnownEdge(&sci->mapd[1],pt->y,pt->x));

return( false );
}

static int IsHVSpline(Spline *spline) {
    if ( !spline->knownlinear )
return( 2 );
    else if ( spline->from->me.x==spline->to->me.x )
return( 1 );
    else if ( spline->from->me.y==spline->to->me.y )
return( 0 );

return( 2 );
}

static double SCIFindMidPoint(SCI *sci,int pt, SplineList *spl, BasePoint *mid) {
    SplinePoint *sp = sci->pts[pt].cur;
    SplineList *sl;
    double len;
    SplineInfo *only, *exact;
    int isvert;
    BasePoint v;
    Spline *spline;

    if ( spl->cur->spline1==sp->next )
	isvert = IsHVSpline( spline = sp->next );
    else
	isvert = IsHVSpline( spline = sp->prev );

    /* First check to see if there's a Stem in the splinelist that's valid */
    len = -1; only = exact = NULL;
    for ( sl=spl; sl!=NULL; sl=sl->next ) {
	if ( MetaRecognizedStemWidth(sci->meta,sl->cur->fromlen,isvert) ||
		MetaRecognizedStemWidth(sci->meta,sl->cur->tolen,isvert)) {
	    if ( len==-1 ) {
		len = MetaRecognizedStemWidth(sci->meta,sl->cur->fromlen,isvert)?
			sl->cur->fromlen:sl->cur->tolen;
		only = sl->cur;
	    } else if ( DoubleApprox(len,sl->cur->fromlen) ||
		    DoubleApprox(len,sl->cur->tolen)) {
		/* Same size as last, it's ok */
	    } else
		len = -2;
	    if ( sl->cur->from == sp || sl->cur->to == sp )
		exact = sl->cur;
	}
    }
    if ( exact==NULL && len!=-2 ) exact = only;
    if ( exact!=NULL ) {
	if ( exact->from==sp ) {
	    *mid = exact->frommid;
	    len = exact->fromlen;
	} else if ( exact->to==sp ) {
	    *mid = exact->tomid;
	    len = exact->tolen;
	} else {
	    if ( exact->spline1->to==sp ) {
		v.x = exact->tomid.x-spline->to->me.x;
		v.y = exact->tomid.y-spline->to->me.y;
		len = exact->tolen;
	    } else {
		v.x = exact->frommid.x-spline->from->me.x;
		v.y = exact->frommid.y-spline->from->me.y;
		len = exact->fromlen;
	    }
	    mid->x = sp->me.x - v.x;
	    mid->y = sp->me.y - v.y;
	}
return( len );
    }

#if 0		/* I don't think this will gain us anything over MapPoint */
    /* check to see if it falls exactly on one of the hints */
    if ( s->knownlinear && s->from->me.x==s->to->me.x ) {	/* Vertical spline */
	for ( h=sci->sc->hstem; h!=NULL; h=h->next ) {
	    if ( h->start==sp->me.x || h->start+h->width==sp->me.x ) {
		mid->y = sp->me.y;
		mid->x = h->start+h->width/2;
return( h->width);
	    }
	}
    } else if ( s->knownlinear && s->from->me.y==s->to->me.y ) {/* Horizontal spline */
	for ( h=sci->sc->hstem; h!=NULL; h=h->next ) {
	    if ( h->start==sp->me.y || h->start+h->width==sp->me.y ) {
		mid->x = sp->me.x;
		mid->y = h->start+h->width/2;
return( h->width);
	    }
	}
    }
#endif

return( 0 );		/* Couldn't find a stem */
}

static void PositionFromMidLen(SCI *sci,BasePoint *new,BasePoint *mid,SplinePoint *sp,
	double len, Spline *s) {
    BasePoint v, other;
    int isvert = IsHVSpline(s);

    v.x = mid->x-sp->me.x; v.y = mid->y-sp->me.y;
    other.x = mid->x+v.x; other.y = mid->y+v.y;
    len = MetaFontFindWidth(sci->meta,len,isvert);
    len /= sqrt(v.x*v.x + v.y*v.y);
    v.x *= len; v.y *= len;
    if ( SCIIsOnKnownEdge(sci,&sp->me,isvert) ) {
	*new = sp->me;
	SCIMapPoint(sci,new);
	new->x = rint(new->x);
	new->y = rint(new->y);
    } else if ( SCIIsOnKnownEdge(sci,&other,isvert)) {
	SCIMapPoint(sci,&other);
	new->x = rint(other.x)-2*v.x;
	new->y = rint(other.y)-2*v.y;
    } else {
	SCIMapPoint(sci,mid);
	if ( v.y>0 || (v.x>0 && v.y==0)) {
	    new->x = rint(mid->x+v.x)-2*v.x;
	    new->y = rint(mid->y+v.y)-2*v.y;
	} else {
	    new->x = rint(mid->x-v.x);
	    new->y = rint(mid->y-v.y);
	}
    }
}

static void SCIPositionPts(SCI *sci) {
    int i;
    double len1, len2;
    BasePoint mid1, mid2;
    BasePoint v1,v2;
    BasePoint new, new1, new2;
    SplinePoint *sp;

    for ( i=0; i<sci->ptcnt; ++i ) {
	sp = sci->pts[i].cur;
	if ( !SplinePointIsACorner(sp)) {
	    len1 = SCIFindMidPoint(sci,i,sci->pts[i].next,&mid1);
	    if ( len1==0 ) {
		new = sp->me;
		SCIMapPoint(sci,&new);
	    } else {
		PositionFromMidLen(sci,&new,&mid1,sp,len1,sp->next);
	    }
	} else {
	    len1 = SCIFindMidPoint(sci,i,sci->pts[i].next,&mid1);
	    len2 = SCIFindMidPoint(sci,i,sci->pts[sp->prev->from->ptindex].next,&mid2);
	    if ( len1!=0 && len2!=0 ) {
		v1.y = mid1.x-sp->me.x; v1.x = -(mid1.y-sp->me.y);	/* This vector should be tangent to the spline */
		v2.y = mid2.x-sp->me.x; v2.x = -(mid2.y-sp->me.y);	/* (it's purp to something that's purp to the spline) */
		PositionFromMidLen(sci,&new1,&mid1,sp,len1,sp->next);
		PositionFromMidLen(sci,&new2,&mid2,sp,len2,sp->prev);
		new = new1;		/* This is a fallback for when we get errors */
		if ( v1.x==0 ) {
		    if ( v2.x==0 )
			GDrawIError("Two Parallel horizontal lines" );
		    else
			/* Inherit x from new1 */
			new.y = new2.y + v2.y*(new.x-new2.x)/v2.x;
		} else if ( v2.x==0 ) {
		    if ( v1.x==0 )
			GDrawIError("Two Parallel horizontal lines(2)" );
		    else {
			new.x = new2.x;
			new.y = new1.y + v1.y*(new.x-new1.x)/v1.x;
		    }
		} else if ( v2.y/v2.x == v1.y/v1.x )
			GDrawIError("Two Parallel lines" );
		else {
/* new1.y + v1.y*(X-new1.x)/v1.x = new2.y + v2.y*(X-new2.x)/v2.x */
/* new1.y-new2.y - v1.y/v1.x*new1.x + v2.y/v2.x*new2.x = (v2.y/v2.x-v1.y/v1.x)*X */
		    new.x = (new1.y-new2.y - v1.y/v1.x*new1.x + v2.y/v2.x*new2.x)/(v2.y/v2.x-v1.y/v1.x);
		    new.y = new1.y+ v1.y/v1.x*(new.x-new1.x);
		}
	    } else {
		new = sp->me;
		SCIMapPoint(sci,&new);
	    }
	}
	sci->pts[i].newme = new;
    }
}

static void SCIPositionControls(SCI *sci) {
    BasePoint scale;
    SplinePoint *sp, *nsp;
    int i,j;

    for ( i=0; i<sci->ptcnt; ++i ) {
	sp = sci->pts[i].cur;
	nsp = sp->next->to;
	j = nsp->ptindex;
	scale.x = scale.y = 1.0;
	if ( sp->me.x!=nsp->me.x )
	    scale.x = ( sci->pts[i].newme.x != sci->pts[j].newme.x )/(sp->me.x-nsp->me.x);
	if ( sp->me.y!=nsp->me.y )
	    scale.y = ( sci->pts[i].newme.y != sci->pts[j].newme.y )/(sp->me.y-nsp->me.y);
	sci->pts[i].newnext.x = sci->pts[i].newme.x + scale.x * (sp->nextcp.x-sp->me.x);
	sci->pts[i].newnext.y = sci->pts[i].newme.y + scale.y * (sp->nextcp.y-sp->me.y);
	sci->pts[j].newprev.x = sci->pts[j].newme.x + scale.x * (nsp->prevcp.x-nsp->me.x);
	sci->pts[j].newprev.y = sci->pts[j].newme.y + scale.y * (nsp->prevcp.y-nsp->me.y);
    }
}

static void SCISet(SCI *sci) {
    SplinePoint *sp;
    int i;

    for ( i=0; i<sci->ptcnt; ++i ) {
	sp = sci->pts[i].cur;
	sp->prevcp = sci->pts[i].newprev;
	sp->nextcp = sci->pts[i].newnext;
	sp->me = sci->pts[i].newme;
    }
}
	    
void MetaFont(FontView *fv,CharView *cv) {
    SCI *sci;
    MetaFontDlg meta;
    DBounds b;

    sci = SCIinit(cv->sc,&meta);
    SCIFindStems(sci);
    SCIFigureWidths(sci);
    SCIBuildMaps(sci,0);		/* Horizontal maps */
    SCIBuildMaps(sci,1);		/* Vertical maps */
    SCIPositionPts(sci);
    SCIPositionControls(sci);
    SCISet(sci);
    if ( !meta.scalewidth ) {
	SplineCharFindBounds(sci->sc,&b);
	sci->sc->width = sci->rbearing + b.maxx;
    }
    SCIFree(sci);
}
